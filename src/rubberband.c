/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */


/* functions used by 'rubberband moves'
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <memory.h>
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "global.h"

#include "create.h"
#include "data.h"
#include "error.h"
#include "misc.h"
#include "polygon.h"
#include "rubberband.h"
#include "rtree.h"
#include "search.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define dprintf if(0)printf

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
void CheckPadForRubberbandConnection (PadType *);
static void CheckPinForRubberbandConnection (PinType *);
static void CheckLinePointForRubberbandConnection (LayerType *,
						   LineType *,
						   PointType *,
						   bool);
static void CheckPolygonForRubberbandConnection (LayerType *,
						 PolygonType *);
static void CheckLinePointForRat (LayerType *, PointType *);
static int rubber_callback (const BoxType * b, void *cl);
static int rb_sign (Coord x);
static int LineAttachedCallback (const BoxType * b, void *cl);

struct rubber_info
{
  Coord radius;
  Coord X, Y;
  LineType *line;
  BoxType box;
  LayerType *layer;
};

struct AttachedInfo
{
  int radius;
  Coord X, Y;
  LineType *line;
  BoxType box;
  LayerType *layer;
  int nAttached;
  LineType *AttachedLine;
};

static int
rubber_callback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  struct rubber_info *i = (struct rubber_info *) cl;
  double x, y, rad, dist1, dist2;
  Coord t;
  int Point1touches = 0, Point2touches = 0;

  t = line->Thickness / 2;

  if (TEST_FLAG (LOCKFLAG, line))
    return 0;
  if (line == i->line)
    return 0;
  /* 
   * Check to see if the line touches a rectangular region.
   * To do this we need to look for the intersection of a circular
   * region and a rectangular region.
   */
  if (i->radius == 0)
    {
      int found = 0;

      if (line->Point1.X + t >= i->box.X1 && line->Point1.X - t <= i->box.X2
	  && line->Point1.Y + t >= i->box.Y1
	  && line->Point1.Y - t <= i->box.Y2)
	{
	  if (((i->box.X1 <= line->Point1.X) &&
	       (line->Point1.X <= i->box.X2)) ||
	      ((i->box.Y1 <= line->Point1.Y) &&
	       (line->Point1.Y <= i->box.Y2)))
	    {
	      /* 
	       * The circle is positioned such that the closest point
	       * on the rectangular region boundary is not at a corner
	       * of the rectangle.  i.e. the shortest line from circle
	       * center to rectangle intersects the rectangle at 90
	       * degrees.  In this case our first test is sufficient
	       */
              CreateNewRubberbandEntry (i->layer, line, &line->Point1);
              Point1touches = 1;
	    }
	  else
	    {
	      /* 
	       * Now we must check the distance from the center of the
	       * circle to the corners of the rectangle since the
	       * closest part of the rectangular region is the corner.
	       */
	      x = MIN (abs (i->box.X1 - line->Point1.X),
		       abs (i->box.X2 - line->Point1.X));
	      x *= x;
	      y = MIN (abs (i->box.Y1 - line->Point1.Y),
		       abs (i->box.Y2 - line->Point1.Y));
	      y *= y;
	      x = x + y - (t * t);

              if (x <= 0)
              {
                CreateNewRubberbandEntry (i->layer, line, &line->Point1);
                Point2touches = 1;
	      }
	  }
        }
 
     if (line->Point2.X + t >= i->box.X1 && line->Point2.X - t <= i->box.X2
	  && line->Point2.Y + t >= i->box.Y1
	  && line->Point2.Y - t <= i->box.Y2)
	{
	  if (((i->box.X1 <= line->Point2.X) &&
	       (line->Point2.X <= i->box.X2)) ||
	      ((i->box.Y1 <= line->Point2.Y) &&
	       (line->Point2.Y <= i->box.Y2)))
	    {
              CreateNewRubberbandEntry (i->layer, line, &line->Point2);
              Point2touches = 1;
	    }
	  else
	    {
	      x = MIN (abs (i->box.X1 - line->Point2.X),
		       abs (i->box.X2 - line->Point2.X));
	      x *= x;
	      y = MIN (abs (i->box.Y1 - line->Point2.Y),
		       abs (i->box.Y2 - line->Point2.Y));
	      y *= y;
	      x = x + y - (t * t);

              if (x <= 0)
                {
                  CreateNewRubberbandEntry (i->layer, line, &line->Point2);
                  Point2touches = 1;
                }
	    }
	}

      if (Point1touches || Point2touches)
        {
          return 1;
        }

      return found;
    }
  /* circular search region */
  if (i->radius < 0)
    rad = 0;  /* require exact match */
  else
    rad = SQUARE(i->radius + t);

  x = (i->X - line->Point1.X);
  x *= x;
  y = (i->Y - line->Point1.Y);
  y *= y;
  dist1 = x + y - rad;

  x = (i->X - line->Point2.X);
  x *= x;
  y = (i->Y - line->Point2.Y);
  y *= y;
  dist2 = x + y - rad;

  if (dist1 > 0 && dist2 > 0)
    return 0;

#ifdef CLOSEST_ONLY	/* keep this to remind me */
  if (dist1 < dist2)
    CreateNewRubberbandEntry (i->layer, line, &line->Point1);
  else
    CreateNewRubberbandEntry (i->layer, line, &line->Point2);
#else
  if (dist1 <= 0)
    CreateNewRubberbandEntry (i->layer, line, &line->Point1);
  if (dist2 <= 0)
    CreateNewRubberbandEntry (i->layer, line, &line->Point2);
#endif
  return 1;
}

/* ---------------------------------------------------------------------------
 * checks all visible lines which belong to the same layergroup as the
 * passed pad. If one of the endpoints of the line lays inside the pad,
 * the line is added to the 'rubberband' list
 */
void
CheckPadForRubberbandConnection (PadType *Pad)
{
  Coord half = Pad->Thickness / 2;
  Cardinal i, group;
  struct rubber_info info;

  info.box.X1 = MIN (Pad->Point1.X, Pad->Point2.X) - half;
  info.box.Y1 = MIN (Pad->Point1.Y, Pad->Point2.Y) - half;
  info.box.X2 = MAX (Pad->Point1.X, Pad->Point2.X) + half;
  info.box.Y2 = MAX (Pad->Point1.Y, Pad->Point2.Y) + half;
  info.radius = 0;
  info.line = NULL;
  i = TEST_FLAG (ONSOLDERFLAG, Pad) ? solder_silk_layer : component_silk_layer;
  group = GetLayerGroupNumberByNumber (i);

  /* check all visible layers in the same group */
  GROUP_LOOP (PCB->Data, group);
  {
    /* check all visible lines of the group member */
    info.layer = layer;
    if (info.layer->On)
      {
	r_search (info.layer->line_tree, &info.box, NULL, rubber_callback,
		  &info);
      }
  }
  END_LOOP;
}

struct rinfo
{
  int type;
  Cardinal group;
  PinType *pin;
  PadType *pad;
  PointType *point;
};

static int
rat_callback (const BoxType * box, void *cl)
{
  RatType *rat = (RatType *) box;
  struct rinfo *i = (struct rinfo *) cl;

  switch (i->type)
    {
    case PIN_TYPE:
      if (rat->Point1.X == i->pin->X && rat->Point1.Y == i->pin->Y)
	CreateNewRubberbandEntry (NULL, (LineType *) rat, &rat->Point1);
      else if (rat->Point2.X == i->pin->X && rat->Point2.Y == i->pin->Y)
	CreateNewRubberbandEntry (NULL, (LineType *) rat, &rat->Point2);
      break;
    case PAD_TYPE:
      if (rat->Point1.X == i->pad->Point1.X &&
	  rat->Point1.Y == i->pad->Point1.Y && rat->group1 == i->group)
	CreateNewRubberbandEntry (NULL, (LineType *) rat, &rat->Point1);
      else
	if (rat->Point2.X == i->pad->Point1.X &&
	    rat->Point2.Y == i->pad->Point1.Y && rat->group2 == i->group)
	CreateNewRubberbandEntry (NULL, (LineType *) rat, &rat->Point2);
      else
	if (rat->Point1.X == i->pad->Point2.X &&
	    rat->Point1.Y == i->pad->Point2.Y && rat->group1 == i->group)
	CreateNewRubberbandEntry (NULL, (LineType *) rat, &rat->Point1);
      else
	if (rat->Point2.X == i->pad->Point2.X &&
	    rat->Point2.Y == i->pad->Point2.Y && rat->group2 == i->group)
	CreateNewRubberbandEntry (NULL, (LineType *) rat, &rat->Point2);
      else
	if (rat->Point1.X == (i->pad->Point1.X + i->pad->Point2.X) / 2 &&
	    rat->Point1.Y == (i->pad->Point1.Y + i->pad->Point2.Y) / 2 &&
	    rat->group1 == i->group)
	CreateNewRubberbandEntry (NULL, (LineType *) rat, &rat->Point1);
      else
	if (rat->Point2.X == (i->pad->Point1.X + i->pad->Point2.X) / 2 &&
	    rat->Point2.Y == (i->pad->Point1.Y + i->pad->Point2.Y) / 2 &&
	    rat->group2 == i->group)
	CreateNewRubberbandEntry (NULL, (LineType *) rat, &rat->Point2);
      break;
    case LINEPOINT_TYPE:
      if (rat->group1 == i->group &&
	  rat->Point1.X == i->point->X && rat->Point1.Y == i->point->Y)
	CreateNewRubberbandEntry (NULL, (LineType *) rat, &rat->Point1);
      else
	if (rat->group2 == i->group &&
	    rat->Point2.X == i->point->X && rat->Point2.Y == i->point->Y)
	CreateNewRubberbandEntry (NULL, (LineType *) rat, &rat->Point2);
      break;
    default:
      Message ("hace: bad rubber-rat lookup callback\n");
    }
  return 0;
}

static void
CheckPadForRat (PadType *Pad)
{
  struct rinfo info;
  Cardinal i;

  i = TEST_FLAG (ONSOLDERFLAG, Pad) ? solder_silk_layer : component_silk_layer;
  info.group = GetLayerGroupNumberByNumber (i);
  info.pad = Pad;
  info.type = PAD_TYPE;

  r_search (PCB->Data->rat_tree, &Pad->BoundingBox, NULL, rat_callback,
	    &info);
}

static void
CheckPinForRat (PinType *Pin)
{
  struct rinfo info;

  info.type = PIN_TYPE;
  info.pin = Pin;
  r_search (PCB->Data->rat_tree, &Pin->BoundingBox, NULL, rat_callback,
	    &info);
}

static void
CheckLinePointForRat (LayerType *Layer, PointType *Point)
{
  struct rinfo info;
  info.group = GetLayerGroupNumberByPointer (Layer);
  info.point = Point;
  info.type = LINEPOINT_TYPE;

  r_search (PCB->Data->rat_tree, (BoxType *) Point, NULL, rat_callback,
	    &info);
}

/* ---------------------------------------------------------------------------
 * checks all visible lines. If one of the endpoints of the line lays
 * inside the pin, the line is added to the 'rubberband' list
 *
 * Square pins are handled as if they were round. Speed
 * and readability is more important then the few %
 * of failures that are immediately recognized
 */
static void
CheckPinForRubberbandConnection (PinType *Pin)
{
  struct rubber_info info;
  Cardinal n;
  Coord t = Pin->Thickness / 2;

  info.box.X1 = Pin->X - t;
  info.box.X2 = Pin->X + t;
  info.box.Y1 = Pin->Y - t;
  info.box.Y2 = Pin->Y + t;
  info.line = NULL;
  if (TEST_FLAG (SQUAREFLAG, Pin))
    info.radius = 0;
  else
    {
      info.radius = t;
      info.X = Pin->X;
      info.Y = Pin->Y;
    }

  for (n = 0; n < max_copper_layer; n++)
    {
      info.layer = LAYER_PTR (n);
      r_search (info.layer->line_tree, &info.box, NULL, rubber_callback,
		&info);
    }
}

/* ---------------------------------------------------------------------------
 * checks all visible lines which belong to the same group as the passed line.
 * If one of the endpoints of the line lays * inside the passed line,
 * the scanned line is added to the 'rubberband' list
 */
static void
CheckLinePointForRubberbandConnection (LayerType *Layer,
				       LineType *Line,
				       PointType *LinePoint,
				       bool Exact)
{
  Cardinal group;
  struct rubber_info info;
  Coord t = Line->Thickness / 2;

  /* lookup layergroup and check all visible lines in this group */
  info.radius = Exact ? -1 : MAX(Line->Thickness / 2, 1);
  info.box.X1 = LinePoint->X - t;
  info.box.X2 = LinePoint->X + t;
  info.box.Y1 = LinePoint->Y - t;
  info.box.Y2 = LinePoint->Y + t;
  info.line = Line;
  info.X = LinePoint->X;
  info.Y = LinePoint->Y;
  group = GetLayerGroupNumberByPointer (Layer);
  GROUP_LOOP (PCB->Data, group);
  {
    /* check all visible lines of the group member */
    if (layer->On)
      {
	info.layer = layer;
	r_search (layer->line_tree, &info.box, NULL, rubber_callback, &info);
      }
  }
  END_LOOP;
}

/* ---------------------------------------------------------------------------
 * checks all visible lines which belong to the same group as the passed polygon.
 * If one of the endpoints of the line lays inside the passed polygon,
 * the scanned line is added to the 'rubberband' list
 */
static void
CheckPolygonForRubberbandConnection (LayerType *Layer,
				     PolygonType *Polygon)
{
  Cardinal group;

  /* lookup layergroup and check all visible lines in this group */
  group = GetLayerGroupNumberByPointer (Layer);
  GROUP_LOOP (PCB->Data, group);
  {
    if (layer->On)
      {
	Coord thick;

	/* the following code just stupidly compares the endpoints
	 * of the lines
	 */
	LINE_LOOP (layer);
	{
	  if (TEST_FLAG (LOCKFLAG, line))
	    continue;
	  if (TEST_FLAG (CLEARLINEFLAG, line))
	    continue;
	  thick = (line->Thickness + 1) / 2;
	  if (IsPointInPolygon (line->Point1.X, line->Point1.Y,
				thick, Polygon))
	    CreateNewRubberbandEntry (layer, line, &line->Point1);
	  if (IsPointInPolygon (line->Point2.X, line->Point2.Y,
				thick, Polygon))
	    CreateNewRubberbandEntry (layer, line, &line->Point2);
	}
	END_LOOP;
      }
  }
  END_LOOP;
}

/* ---------------------------------------------------------------------------
 * lookup all lines that are connected to an object and save the
 * data to 'Crosshair.AttachedObject.Rubberband'
 * lookup is only done for visible layers
 */
void
LookupRubberbandLines (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{

  /* the function is only supported for some types
   * check all visible lines;
   * it is only necessary to check if one of the endpoints
   * is connected
   */
  switch (Type)
    {
    case ELEMENT_TYPE:
      {
	ElementType *element = (ElementType *) Ptr1;

	/* square pins are handled as if they are round. Speed
	 * and readability is more important then the few %
	 * of failures that are immediately recognized
	 */
	PIN_LOOP (element);
	{
	  CheckPinForRubberbandConnection (pin);
	}
	END_LOOP;
	PAD_LOOP (element);
	{
	  CheckPadForRubberbandConnection (pad);
	}
	END_LOOP;
	break;
      }

    case LINE_TYPE:
      {
	LayerType *layer = (LayerType *) Ptr1;
	LineType *line = (LineType *) Ptr2;
	if (GetLayerNumber (PCB->Data, layer) < max_copper_layer)
	  {
	    CheckLinePointForRubberbandConnection (layer, line,
						   &line->Point1, false);
	    CheckLinePointForRubberbandConnection (layer, line,
						   &line->Point2, false);
	  }
	break;
      }

    case LINEPOINT_TYPE:
      if (GetLayerNumber (PCB->Data, (LayerType *) Ptr1) < max_copper_layer)
	CheckLinePointForRubberbandConnection ((LayerType *) Ptr1,
					       (LineType *) Ptr2,
					       (PointType *) Ptr3, true);
      break;

    case VIA_TYPE:
      CheckPinForRubberbandConnection ((PinType *) Ptr1);
      break;

    case POLYGON_TYPE:
      if (GetLayerNumber (PCB->Data, (LayerType *) Ptr1) < max_copper_layer)
	CheckPolygonForRubberbandConnection ((LayerType *) Ptr1,
					     (PolygonType *) Ptr2);
      break;
    }
}

void
LookupRatLines (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  switch (Type)
    {
    case ELEMENT_TYPE:
      {
	ElementType *element = (ElementType *) Ptr1;

	PIN_LOOP (element);
	{
	  CheckPinForRat (pin);
	}
	END_LOOP;
	PAD_LOOP (element);
	{
	  CheckPadForRat (pad);
	}
	END_LOOP;
	break;
      }

    case LINE_TYPE:
      {
	LayerType *layer = (LayerType *) Ptr1;
	LineType *line = (LineType *) Ptr2;

	CheckLinePointForRat (layer, &line->Point1);
	CheckLinePointForRat (layer, &line->Point2);
	break;
      }

    case LINEPOINT_TYPE:
      CheckLinePointForRat ((LayerType *) Ptr1, (PointType *) Ptr3);
      break;

    case VIA_TYPE:
      CheckPinForRat ((PinType *) Ptr1);
      break;
    }
}

/* centre zero rubber band sign function */
int 
rbsgn (Coord x) 
{
  if (x > 0) return  1;
  if (x < 0) return -1;
  return 0;
}

int
IsHorizontal (LineType *Line) 
{
  if (Line->Point1.Y == Line->Point2.Y)
    return 1;
  else
    return 0;
}

int
IsVertical (LineType *Line) 
{
  if (Line->Point1.X == Line->Point2.X)
    return 1;
  else  
    return 0;
}

int 
IsDiagonal (LineType *Line) 
{
  if (!(IsHorizontal(Line) || IsVertical(Line)))
    return 1;
  else  
    return 0;
}

void 
MovePointGivenRubberBandMode (PointType *PointOut, 
                              PointType *Point, 
                              LineType *Line, 
                              Coord dx, 
                              Coord dy,
                              int Type,
                              int Diagonal)
{
  PointType *point1, *point2;

  dprintf("MovePointGivenRubberBandMode\n");

  /* default move */
  memmove (PointOut, Point, sizeof (PointType));
  PointOut->X += dx;
  PointOut->Y += dy;

  /* return default when rubberband mode off */
  if (!TEST_FLAG (RUBBERBANDFLAG, PCB))
  {
    return;
  }
    
  /* or when in rubber band mode and all directions enabled */
  if (TEST_FLAG (ALLDIRECTIONSRUBBERBANDFLAG, PCB))
  {
    return;
  }

  /* or if the object being moved is not a line, as we can't handle
     general objects (yet), only lines */
  if (Type != LINE_TYPE)
    return;
  
  /* or if this line is not horizontal or vertical, we dont handle
     diagonals */
  if (Diagonal)
    return;
  
  /* 
     Determine:
       - point1 is the fixed point of the attached line
       - point2 is where the attached line and line being moved join
  */
  if ((Point->X == Line->Point1.X) && (Point->Y == Line->Point2.Y))
    {
      point1 = &Line->Point2;
      point2 = &Line->Point1;
    }
  else
    {
      point1 = &Line->Point1;
      point2 = &Line->Point2;
    }

  /* Then adjust according to relative direction of joined lines */
  dprintf("  dx: %ld dy : %ld\n", dx, dy);
  dprintf("  point2->X %ld point1->X %ld corr: %ld\n", point2->X, point1->X, 
          dx*rbsgn(point2->X - point1->X));
  dprintf("  point2->Y %ld point1->Y %ld corr: %ld\n", point2->Y, point1->Y, 
          dx*rbsgn(point2->Y - point1->Y));
  dprintf(" Before: Point1->X %ld Point1->Y %ld\n", PointOut->X, 
          PointOut->Y);

  /* I am sure there is some elegant matrix algebra going on here, but
     I used the "get something simple going and iterate" algorithm,
     and by lots of scribbling of bent lines on paper! */
  PointOut->Y += dx*rbsgn(point2->X - point1->X)*rbsgn(point2->Y - point1->Y);
  PointOut->X += dy*rbsgn(point2->Y - point1->Y)*rbsgn(point2->X - point1->X);

  #ifdef RUBBER_DEBUG
  dprintf(" After.: PointOut->X %d PointOut->Y %d\n", PointOut->X, 
          PointOut->Y);
  #endif
}

void 
MoveLineGivenRubberBandMode (LineType *LineOut, 
                             LineType *Line, 
                             Coord dx, 
                             Coord dy,
                             CrosshairType CrossHair)
{
  PointType *point1, *point2, *nudge;
  RubberbandType *AttachedLine;
  int n;

  dprintf("MoveLineGivenRubberBandMode\n");
  dprintf("  dx: %ld dy : %ld\n", dx, dy);

  /* default move */
  memmove (LineOut, Line, sizeof (LineType));
  LineOut->Point1.X += dx;
  LineOut->Point1.Y += dy;
  LineOut->Point2.X += dx;
  LineOut->Point2.Y += dy;

  /* return default when rubberband mode off */
  if (!TEST_FLAG (RUBBERBANDFLAG, PCB))
  {
    return;
  }
    
  /* or when in rubber band mode and all directions enabled */
  if (TEST_FLAG (ALLDIRECTIONSRUBBERBANDFLAG, PCB))
  {
    return;
  }

  /* or if this line is not horizontal or vertical, we dont handle
     diagonals */
  if (IsDiagonal (Line))
    return;

  /* we should return unless there are attached lines */
  if (Crosshair.AttachedObject.RubberbandN == 0)
    return;

  dprintf("    Line->Point1.X = %ld\n", Line->Point1.X);
  dprintf("    Line->Point1.Y = %ld\n", Line->Point1.Y );
  dprintf("    Line->Point2.X = %ld\n", Line->Point2.X);
  dprintf("    Line->Point2.Y = %ld\n", Line->Point2.Y);

  n = Crosshair.AttachedObject.RubberbandN;
  AttachedLine = Crosshair.AttachedObject.Rubberband;
  while (n)
  {
    dprintf("  %d attached Lines\n", n);

    /* 
       Determine:
       - point1 is the fixed point of the attached line
       - point2 is where the attached line and line being moved join
    */

    if (AttachedLine->MovedPoint == &AttachedLine->Line->Point1)
      {
        point1 = &AttachedLine->Line->Point2;
        point2 = &AttachedLine->Line->Point1;
      }
    else
      {
        point1 = &AttachedLine->Line->Point1;
        point2 = &AttachedLine->Line->Point2;
      }

    /* work out which point of moved line to nudge */
    dprintf("    MovedPoint.X   = %ld\n", AttachedLine->MovedPoint->X);
    dprintf("    MovedPoint.Y   = %ld\n", AttachedLine->MovedPoint->Y);
    dprintf("    point2->Y      = %ld\n", point2->Y);
    dprintf("    point2->X      = %ld\n", point2->X);
    dprintf("    point2->Y      = %ld\n", point2->Y);

    if ((point2->X == Line->Point1.X) && (point2->Y == Line->Point1.Y))
    {
      nudge = &LineOut->Point1;
      dprintf("    nudge point1\n"); 
    }
    else
    {
      dprintf("    nudge point2\n"); 
      nudge = &LineOut->Point2;
    }

    /* Then adjust to preserve 45 deg angles */
    nudge->Y += dx*rbsgn(point2->X - point1->X)*rbsgn(point2->Y - point1->Y);
    nudge->X += dy*rbsgn(point2->Y - point1->Y)*rbsgn(point2->X - point1->X);

    AttachedLine++;
    n--;
  }
}

void
RestrictMovementGivenRubberBandMode (LineType *Line,
                                     Coord *dx, 
                                     Coord *dy)
{
  /* return default when rubberband mode off */
  if (!TEST_FLAG (RUBBERBANDFLAG, PCB))
  {
    return;
  }
    
  /* or when in rubber band mode and all directions enabled */
  if (TEST_FLAG (ALLDIRECTIONSRUBBERBANDFLAG, PCB))
  {
    return;
  }

  if (IsHorizontal(Line)) *dx = 0;
  if (IsVertical(Line)) *dy = 0;
}

static int
LineAttachedCallback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  struct AttachedInfo *info = (struct AttachedInfo *) cl;

  dprintf("  LineAttachedCallback\n");

  if (line == info->line)
  {
    dprintf("    ourself\n");
    return 0;
  }

  /* note we only store last attached line found, not sure how to
     handle multiple attached lines yet */
  info->nAttached++;
  info->AttachedLine = line;
  dprintf("    (%ld,%ld) - (%ld, %ld)\n", line->Point1.X, line->Point1.Y,
    line->Point2.X, line->Point2.Y);
  return 1;
}

LineType *
FindLineAttachedToPoint (LayerType *Layer,
                         LineType *Line,
                         PointType *LinePoint)
{
  LineType *AttachedLine;
  Cardinal group;
  struct AttachedInfo info;
  Coord t = Line->Thickness / 2;

  /* lookup layergroup and check all visible lines in this group */
  info.radius = Line->Thickness / 2;
  info.box.X1 = LinePoint->X - t;
  info.box.X2 = LinePoint->X + t;;
  info.box.Y1 = LinePoint->Y - t;
  info.box.Y2 = LinePoint->Y + t;
  info.line = Line;
  info.X = LinePoint->X;
  info.Y = LinePoint->Y;
  info.nAttached = 0;
  group = GetLayerGroupNumberByPointer (Layer);

  dprintf("FindLineAttachedToPoint\n");

  GROUP_LOOP (PCB->Data, group);
  {
    /* check all visible lines of the group member */
    if (layer->On)
    {
      info.layer = layer;
      r_search (layer->line_tree, &info.box, NULL, LineAttachedCallback, &info);
    }
  }
  END_LOOP;

  dprintf("info.nAttached = %d\n", info.nAttached); 
  AttachedLine = info.AttachedLine;

  if (info.nAttached)
  {
    /* Make sure the AttachedPoint is at one end of the line (e.g. not in the
       middle) */
    if ((LinePoint->X == AttachedLine->Point1.X) && 
        (LinePoint->Y == AttachedLine->Point1.Y))
      {
        return AttachedLine;
      }
  
    if ((LinePoint->X == AttachedLine->Point2.X) && 
        (LinePoint->Y == AttachedLine->Point2.Y))
      {
        return AttachedLine;
      }
  }

  return NULL; /* no attached line at LinePoint */
}

int
PointInsidePin (PinType *Pin, Coord x, Coord y)
{
  float dist;
  Coord x1, x2, y1, y2;
  Coord t = Pin->Thickness / 2;

  dprintf("  PointInsidePin\n");
  dprintf("    (%ld,%ld) r = %ld\n", Pin->X, Pin->Y, t); 

  if (TEST_FLAG (SQUAREFLAG, Pin))
  {
    dprintf("  square\n");

    x1 = Pin->X - t;
    x2 = Pin->X + t;
    y1 = Pin->Y - t;
    y2 = Pin->Y + t;

    if ((x >= x1) && (x <= x2) &&
        (y >= y1) && (y <= y2))
      
      return 1;
    else
      return 0;
  }
  else
  {    
    dprintf("    round - ");

    dist = sqrt(pow(x - Pin->X, 2.0) + 
                pow(y - Pin->Y, 2.0));

    dprintf("dist %f  ", dist);
    if (dist < t)
    {
      dprintf("YES\n");
      return 1;
    }
    else
    {
      dprintf("NO\n");
      return 0;
    }
  }

  return 0;
}

int
BothEndsWithinPad (PadType *Pad, LineType *Line)
{
#ifdef LATER
    /* make sure x2 > x1 and y2 > y1 */

    if (x1 > x2)
    {
      tmp = x2;
      x2 = x1;
      x1 = tmp;
    }
    if (y1 > y2)
    {
      tmp = y2;
      y2 = y1;
      y1 = tmp;
    }
#endif
  return 0;
}

/*
int
PointIsInsidePin (PinType *Pin)
{
  struct rubber_info info;
  Cardinal n;
  BDimension t = Pin->Thickness / 2;

  info.box.X1 = Pin->X - t;
  info.box.X2 = Pin->X + t;
  info.box.Y1 = Pin->Y - t;
  info.box.Y2 = Pin->Y + t;
  info.line = NULL;
  if (TEST_FLAG (SQUAREFLAG, Pin))
    info.radius = 0;
*/
