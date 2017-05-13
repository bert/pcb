/*!
 * \file src/autoplace.c
 *
 * \brief Functions used to autoplace elements.
 *
 * \author This module, autoplace.c, was written by and is
 * Copyright (c) 2001 C. Scott Ananian
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
 *
 * Copyright (C) 1998,1999,2000,2001 harry eaton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Contact addresses for paper mail and Email:
 * harry eaton, 6697 Buttonhole Ct, Columbia, MD 21044 USA
 * haceaton@aplcomm.jhuapl.edu
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <math.h>
#include <memory.h>
#include <stdlib.h>

#include "global.h"

#include "autoplace.h"
#include "box.h"
#include "compat.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "intersect.h"
#include "rtree.h"
#include "macro.h"
#include "mirror.h"
#include "misc.h"
#include "move.h"
#include "mymem.h"
#include "rats.h"
#include "remove.h"
#include "rotate.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define EXPANDRECTXY(r1, x1, y1, x2, y2) { \
  r1->X1=MIN(r1->X1, x1); r1->Y1=MIN(r1->Y1, y1); \
  r1->X2=MAX(r1->X2, x2); r1->Y2=MAX(r1->Y2, y2); \
}
#define EXPANDRECT(r1, r2) EXPANDRECTXY(r1, r2->X1, r2->Y1, r2->X2, r2->Y2)

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static double ComputeCost (NetListType *Nets, double T0, double T);

/* ---------------------------------------------------------------------------
 * some local types
 */
const struct
{
  double via_cost;
  double congestion_penalty;	/* penalty length / unit area */
  double overlap_penalty_min;	/* penalty length / unit area at start */
  double overlap_penalty_max;	/* penalty length / unit area at end */
  double out_of_bounds_penalty;	/* assessed for each component oob */
  double overall_area_penalty;	/* penalty length / unit area */
  double matching_neighbor_bonus;	/* length bonus per same-type neigh. */
  double aligned_neighbor_bonus;	/* length bonus per aligned neigh. */
  double oriented_neighbor_bonus;	/* length bonus per same-rot neigh. */
#if 0
  double pin_alignment_bonus;	/* length bonus per exact alignment */
  double bound_alignment_bonus;	/* length bonus per exact alignment */
#endif
  double m;			/* annealing stage cutoff constant */
  double gamma;			/* annealing schedule constant */
  int good_ratio;		/* ratio of moves to good moves for halting */
  bool fast;			/* ignore SMD/pin conflicts */
  Coord large_grid_size;	/* snap perturbations to this grid when T is high */
  Coord small_grid_size;	/* snap to this grid when T is small. */
}
/*!
 * \brief Wire cost is manhattan distance (in mils), thus 1 inch = 1000.
 */
CostParameter =
{
  3e3,				/* via cost */
    2e-2,			/* congestion penalty */
    1e-2,			/* initial overlap penalty */
    1e2,			/* final overlap penalty */
    1e3,			/* out of bounds penalty */
    1e0,			/* penalty for total area used */
    1e0,			/* subtract 1000 from cost for every same-type neighbor */
    1e0,			/* subtract 1000 from cost for every aligned neighbor */
    1e0,			/* subtract 1000 from cost for every same-rotation neighbor */
    20,				/* move on when each module has been profitably moved 20 times */
    0.75,			/* annealing schedule constant: 0.85 */
    40,				/* halt when there are 60 times as many moves as good moves */
    false,			/* don't ignore SMD/pin conflicts */
    MIL_TO_COORD (100),		/* coarse grid is 100 mils */
    MIL_TO_COORD (10),		/* fine grid is 10 mils */
};

typedef struct
{
  ElementType **element;
  Cardinal elementN;
}
ElementPtrListType;

enum ewhich
  { SHIFT, ROTATE, EXCHANGE };

typedef struct
{
  ElementType *element;
  enum ewhich which;
  Coord DX, DY;			/* for shift */
  unsigned rotate;		/* for rotate/flip */
  ElementType *other;		/* for exchange */
}
PerturbationType;

/* ---------------------------------------------------------------------------
 * some local identifiers
 */

/*!
 * \brief Update the X, Y and group position information stored in the
 * NetList after elements have possibly been moved, rotated, flipped,
 * etc.
 */
static void
UpdateXY (NetListType *Nets)
{
  Cardinal top_group, bottom_group;
  Cardinal i, j;
  /* find layer groups of the top and bottom sides */
  top_group = GetLayerGroupNumberBySide (TOP_SIDE);
  bottom_group = GetLayerGroupNumberBySide (BOTTOM_SIDE);
  /* update all nets */
  for (i = 0; i < Nets->NetN; i++)
    {
      for (j = 0; j < Nets->Net[i].ConnectionN; j++)
	{
	  ConnectionType *c = &(Nets->Net[i].Connection[j]);
	  switch (c->type)
	    {
	    case PAD_TYPE:
	      c->group = TEST_FLAG (ONSOLDERFLAG, (ElementType *) c->ptr1)
	                  ? bottom_group : top_group;
	      c->X = ((PadType *) c->ptr2)->Point1.X;
	      c->Y = ((PadType *) c->ptr2)->Point1.Y;
	      break;
	    case PIN_TYPE:
	      c->group = bottom_group;  /* any layer will do */
	      c->X = ((PinType *) c->ptr2)->X;
	      c->Y = ((PinType *) c->ptr2)->Y;
	      break;
	    default:
	      Message ("Odd connection type encountered in " "UpdateXY");
	      break;
	    }
	}
    }
}

/*!
 * \brief Create a list of selected elements.
 */
static PointerListType
collectSelectedElements ()
{
  PointerListType list = { 0, 0, NULL };
  ELEMENT_LOOP (PCB->Data);
  {
    if (TEST_FLAG (SELECTEDFLAG, element))
      {
	ElementType **epp = (ElementType **) GetPointerMemory (&list);
	*epp = element;
      }
  }
  END_LOOP;
  return list;
}

#if 0				/* only for debugging box lists */
#include "create.h"
/*!
 * \brief Makes a line on the bottom silk layer surrounding all boxes in
 * blist
 */
static void
showboxes (BoxListType *blist)
{
  Cardinal i;
  LayerType *layer = &(PCB->Data->Layer[bottom_silk_layer]);
  for (i = 0; i < blist->BoxN; i++)
    {
      CreateNewLineOnLayer (layer, blist->Box[i].X1, blist->Box[i].Y1,
			    blist->Box[i].X2, blist->Box[i].Y1, 1, 1, 0);
      CreateNewLineOnLayer (layer, blist->Box[i].X1, blist->Box[i].Y2,
			    blist->Box[i].X2, blist->Box[i].Y2, 1, 1, 0);
      CreateNewLineOnLayer (layer, blist->Box[i].X1, blist->Box[i].Y1,
			    blist->Box[i].X1, blist->Box[i].Y2, 1, 1, 0);
      CreateNewLineOnLayer (layer, blist->Box[i].X2, blist->Box[i].Y1,
			    blist->Box[i].X2, blist->Box[i].Y2, 1, 1, 0);
    }
}
#endif

/*!
 * \brief Helper function to compute "closest neighbor" for a box in a
 *  rtree.
 *
 * The closest neighbor on a certain side is the closest one in a
 * trapezoid emanating from that side.
 */
struct r_neighbor_info
{
  const BoxType *neighbor;
  BoxType trap;
  direction_t search_dir;
};

#define ROTATEBOX(box) { Coord t;\
    t = (box).X1; (box).X1 = - (box).Y1; (box).Y1 = t;\
    t = (box).X2; (box).X2 = - (box).Y2; (box).Y2 = t;\
    t = (box).X1; (box).X1 =   (box).X2; (box).X2 = t;\
}

/*!
 * \brief Helper methods for __r_find_neighbor.
 *
 * <pre>
  ______________ __ trap.y1     __
  \            /               |__| query rect.
   \__________/  __ trap.y2
   |          |
   trap.x1    trap.x2   sides at 45-degree angle

 * </pre>
 */
static int
__r_find_neighbor_reg_in_sea (const BoxType * region, void *cl)
{
  struct r_neighbor_info *ni = (struct r_neighbor_info *) cl;
  BoxType query = *region;
  ROTATEBOX_TO_NORTH (query, ni->search_dir);
  return (query.Y2 > ni->trap.Y1) && (query.Y1 < ni->trap.Y2) &&
    (query.X2 + ni->trap.Y2 > ni->trap.X1 + query.Y1) &&
    (query.X1 + query.Y1 < ni->trap.X2 + ni->trap.Y2);
}

/*!
 * \brief .
 *
 * <pre>

  ______________ __ trap.y1     __
  \            /               |__| query rect.
   \__________/  __ trap.y2
   |          |
   trap.x1    trap.x2   sides at 45-degree angle

 * </pre>
 */
static int
__r_find_neighbor_rect_in_reg (const BoxType * box, void *cl)
{
  struct r_neighbor_info *ni = (struct r_neighbor_info *) cl;
  BoxType query = *box;
  int r;
  ROTATEBOX_TO_NORTH (query, ni->search_dir);
  r = (query.Y2 > ni->trap.Y1) && (query.Y1 < ni->trap.Y2) &&
    (query.X2 + ni->trap.Y2 > ni->trap.X1 + query.Y1) &&
    (query.X1 + query.Y1 < ni->trap.X2 + ni->trap.Y2);
  r = r && (query.Y2 <= ni->trap.Y2);
  if (r)
    {
      ni->trap.Y1 = query.Y2;
      ni->neighbor = box;
    }
  return r;
}

/*!
 * \brief main r_find_neighbor routine.
 *
 * Returns NULL if no neighbor in the requested direction.
 */
static const BoxType *
r_find_neighbor (rtree_t * rtree, const BoxType * box,
		 direction_t search_direction)
{
  struct r_neighbor_info ni;
  BoxType bbox;

  ni.neighbor = NULL;
  ni.trap = *box;
  ni.search_dir = search_direction;

  bbox.X1 = bbox.Y1 = 0;
  bbox.X2 = PCB->MaxWidth;
  bbox.Y2 = PCB->MaxHeight;
  /* rotate so that we can use the 'north' case for everything */
  ROTATEBOX_TO_NORTH (bbox, search_direction);
  ROTATEBOX_TO_NORTH (ni.trap, search_direction);
  /* shift Y's such that trap contains full bounds of trapezoid */
  ni.trap.Y2 = ni.trap.Y1;
  ni.trap.Y1 = bbox.Y1;
  /* do the search! */
  r_search (rtree, NULL,
	    __r_find_neighbor_reg_in_sea, __r_find_neighbor_rect_in_reg, &ni);
  return ni.neighbor;
}

/*!
 * \brief Compute cost function.
 *
 * Note that area overlap cost is correct for SMD devices: SMD devices on
 * opposite sides of the board don't overlap.
 *
 * Algorithms follow those described in sections 4.1 of
 * "Placement and Routing of Electronic Modules" edited by Michael Pecht
 * Marcel Dekker, Inc. 1993.  ISBN: 0-8247-8916-4 TK7868.P7.P57 1993
 */
static double
ComputeCost (NetListType *Nets, double T0, double T)
{
  double W = 0;			/* wire cost */
  double delta1 = 0;		/* wire congestion penalty function */
  double delta2 = 0;		/* module overlap penalty function */
  double delta3 = 0;		/* out of bounds penalty */
  double delta4 = 0;		/* alignment bonus */
  double delta5 = 0;		/* total area penalty */
  Cardinal i, j;
  Coord minx, maxx, miny, maxy;
  bool allpads, allsameside;
  Cardinal thegroup;
  BoxListType bounds = { 0, 0, NULL };	/* save bounding rectangles here */
  BoxListType solderside = { 0, 0, NULL };	/* solder side component bounds */
  BoxListType componentside = { 0, 0, NULL };	/* component side bounds */
  /* make sure the NetList have the proper updated X and Y coords */
  UpdateXY (Nets);
  /* wire length term.  approximated by half-perimeter of minimum
   * rectangle enclosing the net.  Note that we penalize vias in
   * all-SMD nets by making the rectangle a cube and weighting
   * the "layer height" of the net. */
  for (i = 0; i < Nets->NetN; i++)
    {
      NetType *n = &Nets->Net[i];
      if (n->ConnectionN < 2)
	continue;		/* no cost to go nowhere */
      minx = maxx = n->Connection[0].X;
      miny = maxy = n->Connection[0].Y;
      thegroup = n->Connection[0].group;
      allpads = (n->Connection[0].type == PAD_TYPE);
      allsameside = true;
      for (j = 1; j < n->ConnectionN; j++)
	{
	  ConnectionType *c = &(n->Connection[j]);
	  MAKEMIN (minx, c->X);
	  MAKEMAX (maxx, c->X);
	  MAKEMIN (miny, c->Y);
	  MAKEMAX (maxy, c->Y);
	  if (c->type != PAD_TYPE)
	    allpads = false;
	  if (c->group != thegroup)
	    allsameside = false;
	}
      /* save bounding rectangle */
      {
	BoxType *box = GetBoxMemory (&bounds);
	box->X1 = minx;
	box->Y1 = miny;
	box->X2 = maxx;
	box->Y2 = maxy;
      }
      /* okay, add half-perimeter to cost! */
      W += COORD_TO_MIL(maxx - minx) + COORD_TO_MIL(maxy - miny) +
	((allpads && !allsameside) ? CostParameter.via_cost : 0);
    }
  /* now compute penalty function Wc which is proportional to
   * amount of overlap and congestion. */
  /* delta1 is congestion penalty function */
  delta1 = CostParameter.congestion_penalty *
    sqrt (fabs (ComputeIntersectionArea (&bounds)));
#if 0
  printf ("Wire Congestion Area: %f\n", ComputeIntersectionArea (&bounds));
#endif
  /* free bounding rectangles */
  FreeBoxListMemory (&bounds);
  /* now collect module areas (bounding rect of pins/pads) */
  /* two lists for solder side / component side. */

  ELEMENT_LOOP (PCB->Data);
  {
    BoxListType *thisside;
    BoxListType *otherside;
    BoxType *box;
    BoxType *lastbox = NULL;
    Coord thickness;
    Coord clearance;
    if (TEST_FLAG (ONSOLDERFLAG, element))
      {
	thisside = &solderside;
	otherside = &componentside;
      }
    else
      {
	thisside = &componentside;
	otherside = &solderside;
      }
    box = GetBoxMemory (thisside);
    /* protect against elements with no pins/pads */
    if (element->PinN == 0 && element->PadN == 0)
      continue;
    /* initialize box so that it will take the dimensions of
     * the first pin/pad */
    box->X1 = MAX_COORD;
    box->Y1 = MAX_COORD;
    box->X2 = -MAX_COORD;
    box->Y2 = -MAX_COORD;
    PIN_LOOP (element);
    {
      thickness = pin->Thickness / 2;
      clearance = pin->Clearance * 2;
    EXPANDRECTXY (box,
		    pin->X - (thickness + clearance),
		    pin->Y - (thickness + clearance),
		    pin->X + (thickness + clearance),
		    pin->Y + (thickness + clearance))}
    END_LOOP;
    PAD_LOOP (element);
    {
      thickness = pad->Thickness / 2;
      clearance = pad->Clearance * 2;
    EXPANDRECTXY (box,
		    MIN (pad->Point1.X,
			   pad->Point2.X) - (thickness +
					       clearance),
		    MIN (pad->Point1.Y,
			   pad->Point2.Y) - (thickness +
					       clearance),
		    MAX (pad->Point1.X,
			   pad->Point2.X) + (thickness +
					       clearance),
		    MAX (pad->Point1.Y,
			   pad->Point2.Y) + (thickness + clearance))}
    END_LOOP;
    /* add a box for each pin to the "opposite side":
     * surface mount components can't sit on top of pins */
    if (!CostParameter.fast)
      PIN_LOOP (element);
    {
      box = GetBoxMemory (otherside);
      thickness = pin->Thickness / 2;
      clearance = pin->Clearance * 2;
      /* we ignore clearance here */
      /* (otherwise pins don't fit next to each other) */
      box->X1 = pin->X - thickness;
      box->Y1 = pin->Y - thickness;
      box->X2 = pin->X + thickness;
      box->Y2 = pin->Y + thickness;
      /* speed hack! coalesce with last box if we can */
      if (lastbox != NULL &&
	  ((lastbox->X1 == box->X1 &&
	    lastbox->X2 == box->X2 &&
	    MIN (abs (lastbox->Y1 - box->Y2),
		 abs (box->Y1 - lastbox->Y2)) <
	    clearance) || (lastbox->Y1 == box->Y1
			   && lastbox->Y2 == box->Y2
			   &&
			   MIN (abs
				(lastbox->X1 -
				 box->X2),
				abs (box->X1 - lastbox->X2)) < clearance)))
	{
	  EXPANDRECT (lastbox, box);
	  otherside->BoxN--;
	}
      else
	lastbox = box;
    }
    END_LOOP;
    /* assess out of bounds penalty */
    if (element->VBox.X1 < 0 ||
	element->VBox.Y1 < 0 ||
	element->VBox.X2 > PCB->MaxWidth || element->VBox.Y2 > PCB->MaxHeight)
      delta3 += CostParameter.out_of_bounds_penalty;
  }
  END_LOOP;
  /* compute intersection area of module areas box list */
  delta2 = sqrt (fabs (ComputeIntersectionArea (&solderside) +
		       ComputeIntersectionArea (&componentside))) *
    (CostParameter.overlap_penalty_min +
     (1 - (T / T0)) * CostParameter.overlap_penalty_max);
#if 0
  printf ("Module Overlap Area (solder): %f\n",
	  ComputeIntersectionArea (&solderside));
  printf ("Module Overlap Area (component): %f\n",
	  ComputeIntersectionArea (&componentside));
#endif
  FreeBoxListMemory (&solderside);
  FreeBoxListMemory (&componentside);
  /* reward pin/pad x/y alignment */
  /* score higher if pins/pads belong to same *type* of component */
  /* XXX: subkey should be *distance* from thing aligned with, so that
   * aligning to something far away isn't profitable */
  {
    /* create r tree */
    PointerListType seboxes = { 0, 0, NULL }
    , ceboxes =
    {
    0, 0, NULL};
    struct ebox
    {
      BoxType box;
      ElementType *element;
    };
    direction_t dir[4] = { NORTH, EAST, SOUTH, WEST };
    struct ebox **boxpp, *boxp;
    rtree_t *rt_s, *rt_c;
    int factor;
    ELEMENT_LOOP (PCB->Data);
    {
      boxpp = (struct ebox **)
	GetPointerMemory (TEST_FLAG (ONSOLDERFLAG, element) ?
			  &seboxes : &ceboxes);
      *boxpp = (struct ebox *)malloc (sizeof (**boxpp));
      if (*boxpp == NULL ) 
	{
	  fprintf (stderr, "malloc() failed in %s\n", __FUNCTION__);
	  exit (1);
	}

      (*boxpp)->box = element->VBox;
      (*boxpp)->element = element;
    }
    END_LOOP;
    rt_s = r_create_tree ((const BoxType **) seboxes.Ptr, seboxes.PtrN, 1);
    rt_c = r_create_tree ((const BoxType **) ceboxes.Ptr, ceboxes.PtrN, 1);
    FreePointerListMemory (&seboxes);
    FreePointerListMemory (&ceboxes);
    /* now, for each element, find its neighbor on all four sides */
    delta4 = 0;
    for (i = 0; i < 4; i++)
      ELEMENT_LOOP (PCB->Data);
    {
      boxp = (struct ebox *)
	r_find_neighbor (TEST_FLAG (ONSOLDERFLAG, element) ?
			 rt_s : rt_c, &element->VBox, dir[i]);
      /* score bounding box alignments */
      if (!boxp)
	continue;
      factor = 1;
      if (element->Name[0].TextString &&
	  boxp->element->Name[0].TextString &&
	  0 == NSTRCMP (element->Name[0].TextString,
			boxp->element->Name[0].TextString))
	{
	  delta4 += CostParameter.matching_neighbor_bonus;
	  factor++;
	}
      if (element->Name[0].Direction == boxp->element->Name[0].Direction)
	delta4 += factor * CostParameter.oriented_neighbor_bonus;
      if (element->VBox.X1 == boxp->element->VBox.X1 ||
	  element->VBox.X1 == boxp->element->VBox.X2 ||
	  element->VBox.X2 == boxp->element->VBox.X1 ||
	  element->VBox.X2 == boxp->element->VBox.X2 ||
	  element->VBox.Y1 == boxp->element->VBox.Y1 ||
	  element->VBox.Y1 == boxp->element->VBox.Y2 ||
	  element->VBox.Y2 == boxp->element->VBox.Y1 ||
	  element->VBox.Y2 == boxp->element->VBox.Y2)
	delta4 += factor * CostParameter.aligned_neighbor_bonus;
    }
    END_LOOP;
    /* free k-d tree memory */
    r_destroy_tree (&rt_s);
    r_destroy_tree (&rt_c);
  }
  /* penalize total area used by this layout */
  {
    Coord minX = MAX_COORD, minY = MAX_COORD;
    Coord maxX = -MAX_COORD, maxY = -MAX_COORD;
    ELEMENT_LOOP (PCB->Data);
    {
      MAKEMIN (minX, element->VBox.X1);
      MAKEMIN (minY, element->VBox.Y1);
      MAKEMAX (maxX, element->VBox.X2);
      MAKEMAX (maxY, element->VBox.Y2);
    }
    END_LOOP;
    if (minX < maxX && minY < maxY)
      delta5 = CostParameter.overall_area_penalty *
	sqrt (COORD_TO_MIL (maxX - minX) * COORD_TO_MIL (maxY - minY));
  }
  if (T == 5)
    {
      T = W + delta1 + delta2 + delta3 - delta4 + delta5;
      printf ("cost components are %.3f %.3f %.3f %.3f %.3f %.3f\n",
	      W / T, delta1 / T, delta2 / T, delta3 / T, -delta4 / T,
	      delta5 / T);
    }
  /* done! */
  return W + (delta1 + delta2 + delta3 - delta4 + delta5);
}

/*!
 * \brief .
 *
 * Perturb:
 * 1) flip SMD from solder side to component side or vice-versa.\n
 * 2) rotate component 90, 180, or 270 degrees.\n
 * 3) shift component random + or - amount in random direction.\n
 *    (magnitude of shift decreases over time)\n
 * -- Only perturb selected elements (need count/list of selected?) --
 */
PerturbationType
createPerturbation (PointerListType *selected, double T)
{
  PerturbationType pt = { 0 };
  /* pick element to perturb */
  pt.element = (ElementType *) selected->Ptr[random () % selected->PtrN];
  /* exchange, flip/rotate or shift? */
  switch (random () % ((selected->PtrN > 1) ? 3 : 2))
    {
    case 0:
      {				/* shift! */
	Coord grid;
	double scaleX = CLAMP (sqrt (T), MIL_TO_COORD (2.5), PCB->MaxWidth / 3);
	double scaleY = CLAMP (sqrt (T), MIL_TO_COORD (2.5), PCB->MaxHeight / 3);
	pt.which = SHIFT;
	pt.DX = scaleX * 2 * ((((double) random ()) / RAND_MAX) - 0.5);
	pt.DY = scaleY * 2 * ((((double) random ()) / RAND_MAX) - 0.5);
	/* snap to grid. different grids for "high" and "low" T */
	grid = (T > MIL_TO_COORD (10)) ? CostParameter.large_grid_size :
	  CostParameter.small_grid_size;
	/* (round away from zero) */
	pt.DX = ((pt.DX / grid) + SGN (pt.DX)) * grid;
	pt.DY = ((pt.DY / grid) + SGN (pt.DY)) * grid;
	/* limit DX/DY so we don't fall off board */
	pt.DX = MAX (pt.DX, -pt.element->VBox.X1);
	pt.DX = MIN (pt.DX, PCB->MaxWidth - pt.element->VBox.X2);
	pt.DY = MAX (pt.DY, -pt.element->VBox.Y1);
	pt.DY = MIN (pt.DY, PCB->MaxHeight - pt.element->VBox.Y2);
	/* all done but the movin' */
	break;
      }
    case 1:
      {				/* flip/rotate! */
	/* only flip if it's an SMD component */
	bool isSMD = pt.element->PadN != 0;
	pt.which = ROTATE;
	pt.rotate = isSMD ? (random () & 3) : (1 + (random () % 3));
	/* 0 - flip; 1-3, rotate. */
	break;
      }
    case 2:
      {				/* exchange! */
	pt.which = EXCHANGE;
	pt.other = (ElementType *)
	  selected->Ptr[random () % (selected->PtrN - 1)];
	if (pt.other == pt.element)
	  pt.other = (ElementType *) selected->Ptr[selected->PtrN - 1];
	/* don't allow exchanging a solderside-side SMD component
	 * with a non-SMD component. */
	if ((pt.element->PinN != 0 /* non-SMD */  &&
	     TEST_FLAG (ONSOLDERFLAG, pt.other)) ||
	    (pt.other->PinN != 0 /* non-SMD */  &&
	     TEST_FLAG (ONSOLDERFLAG, pt.element)))
	  return createPerturbation (selected, T);
	break;
      }
    default:
      assert (0);
    }
  return pt;
}

void
doPerturb (PerturbationType * pt, bool undo)
{
  Coord bbcx, bbcy;
  /* compute center of element bounding box */
  bbcx = (pt->element->VBox.X1 + pt->element->VBox.X2) / 2;
  bbcy = (pt->element->VBox.Y1 + pt->element->VBox.Y2) / 2;
  /* do exchange, shift or flip/rotate */
  switch (pt->which)
    {
    case SHIFT:
      {
	Coord DX = pt->DX, DY = pt->DY;
	if (undo)
	  {
	    DX = -DX;
	    DY = -DY;
	  }
	MoveElementLowLevel (PCB->Data, pt->element, DX, DY);
	return;
      }
    case ROTATE:
      {
	unsigned b = pt->rotate;
	if (undo)
	  b = (4 - b) & 3;
	/* 0 - flip; 1-3, rotate. */
	if (b)
	  RotateElementLowLevel (PCB->Data, pt->element, bbcx, bbcy, b);
	else
	  {
	    Coord y = pt->element->VBox.Y1;
	    MirrorElementCoordinates (PCB->Data, pt->element, 0);
	    /* mirroring moves the element.  move it back. */
	    MoveElementLowLevel (PCB->Data, pt->element, 0,
				 y - pt->element->VBox.Y1);
	  }
	return;
      }
    case EXCHANGE:
      {
	/* first exchange positions */
	Coord x1 = pt->element->VBox.X1;
	Coord y1 = pt->element->VBox.Y1;
	Coord x2 = pt->other->BoundingBox.X1;
	Coord y2 = pt->other->BoundingBox.Y1;
	MoveElementLowLevel (PCB->Data, pt->element, x2 - x1, y2 - y1);
	MoveElementLowLevel (PCB->Data, pt->other, x1 - x2, y1 - y2);
	/* then flip both elements if they are on opposite sides */
	if (TEST_FLAG (ONSOLDERFLAG, pt->element) !=
	    TEST_FLAG (ONSOLDERFLAG, pt->other))
	  {
	    PerturbationType mypt;
	    mypt.element = pt->element;
	    mypt.which = ROTATE;
	    mypt.rotate = 0;	/* flip */
	    doPerturb (&mypt, undo);
	    mypt.element = pt->other;
	    doPerturb (&mypt, undo);
	  }
	/* done */
	return;
      }
    default:
      assert (0);
    }
}

/*!
 * \brief Auto-place selected components.
 */
bool
AutoPlaceSelected (void)
{
  NetListType *Nets;
  PointerListType Selected = { 0, 0, NULL };
  PerturbationType pt;
  double C0, T0;
  bool changed = false;

  /* (initial netlist processing copied from AddAllRats) */
  /* the netlist library has the text form
   * ProcNetlist fills in the Netlist
   * structure the way the final routing
   * is supposed to look
   */
  Nets = ProcNetlist (&PCB->NetlistLib);
  if (!Nets)
    {
      Message (_("Can't add rat lines because no netlist is loaded.\n"));
      goto done;
    }

  Selected = collectSelectedElements ();
  if (Selected.PtrN == 0)
    {
      Message (_("No elements selected to autoplace.\n"));
      goto done;
    }

  /* simulated annealing */
  {				/* compute T0 by doing a random series of moves. */
    const int TRIALS = 10;
    const double Tx = MIL_TO_COORD (300), P = 0.95;
    double Cs = 0.0;
    int i;
    C0 = ComputeCost (Nets, Tx, Tx);
    for (i = 0; i < TRIALS; i++)
      {
	pt = createPerturbation (&Selected, INCH_TO_COORD (1));
	doPerturb (&pt, false);
	Cs += fabs (ComputeCost (Nets, Tx, Tx) - C0);
	doPerturb (&pt, true);
      }
    T0 = -(Cs / TRIALS) / log (P);
    printf ("Initial T: %f\n", T0);
  }
  /* now anneal in earnest */
  {
    double T = T0;
    long steps = 0;
    int good_moves = 0, moves = 0;
    const int good_move_cutoff = CostParameter.m * Selected.PtrN;
    const int move_cutoff = 2 * good_move_cutoff;
    printf ("Starting cost is %.0f\n", ComputeCost (Nets, T0, 5));
    C0 = ComputeCost (Nets, T0, T);
    while (1)
      {
	double Cprime;
	pt = createPerturbation (&Selected, T);
	doPerturb (&pt, false);
	Cprime = ComputeCost (Nets, T0, T);
	if (Cprime < C0)
	  {			/* good move! */
	    C0 = Cprime;
	    good_moves++;
	    steps++;
	  }
	else if ((random () / (double) RAND_MAX) <
		 exp (MIN (MAX (-20, (C0 - Cprime) / T), 20)))
	  {
	    /* not good but keep it anyway */
	    C0 = Cprime;
	    steps++;
	  }
	else
	  doPerturb (&pt, true);	/* undo last change */
	moves++;
	/* are we at the end of a stage? */
	if (good_moves >= good_move_cutoff || moves >= move_cutoff)
	  {
	    printf ("END OF STAGE: COST %.0f\t"
		    "GOOD_MOVES %d\tMOVES %d\t"
		    "T: %.1f\n", C0, good_moves, moves, T);
	    /* is this the end? */
	    if (T < 5 || good_moves < moves / CostParameter.good_ratio)
	      break;
	    /* nope, adjust T and continue */
	    moves = good_moves = 0;
	    T *= CostParameter.gamma;
	    /* cost is T dependent, so recompute */
	    C0 = ComputeCost (Nets, T0, T);
	  }
      }
    changed = (steps > 0);
  }
done:
  if (changed)
    {
      DeleteRats (false);
      AddAllRats (false, NULL);
      Redraw ();
    }
  FreePointerListMemory (&Selected);
  return (changed);
}
