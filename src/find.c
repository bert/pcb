/*!
 * \file src/find.c
 *
 * \brief Routines to find connections between pins, vias, lines ...
 *
 * Short description:\n
 * <ul>
 * <li> Lists for pins and vias, lines, arcs, pads and for polygons are
 *   created.\n
 *   Every object that has to be checked is added to its list.\n
 *   Coarse searching is accomplished with the data rtrees.</li>
 * <li> There's no 'speed-up' mechanism for polygons because they are
 *   not used as often as other objects.</li> 
 * <li> The maximum distance between line and pin ... would depend on
 *   the angle between them. To speed up computation the limit is set
 *   to one half of the thickness of the objects (cause of square
 *   pins).</li>
 * </ul>
 *
 * PV:  means pin or via (objects that connect layers).\n
 * LO:  all non PV objects (layer objects like lines, arcs, polygons,
 * pads).
 *
 * <ol>
 * <li> First, the LO or PV at the given coordinates is looked
 *   up.</li>
 * <li> All LO connections to that PV are looked up next.</li>
 * <li> Lookup of all LOs connected to LOs from (2).\n
 *   This step is repeated until no more new connections are found.</li>
 * <li> Lookup all PVs connected to the LOs from (2) and (3).</li>
 * <li> Start again with (1) for all new PVs from (4).</li>
 * </ol>
 *
 * Intersection of line <--> circle:\n
 * <ul>
 * <li> Calculate the signed distance from the line to the center,
 *   return false if abs(distance) > R.</li>
 * <li> Get the distance from the line <--> distancevector intersection
 *   to (X1,Y1) in range [0,1], return true if 0 <= distance <= 1.</li>
 * <li> Depending on (r > 1.0 or r < 0.0) check the distance of X2,Y2 or
 *   X1,Y1 to X,Y.</li>
 * </ul>
 *
 * Intersection of line <--> line:\n
 * <ul>
 * <li> See the description of 'LineLineIntersect()'.</li>
 * </ul>
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <setjmp.h>
#include <assert.h>

#include "global.h"

#include "data.h"
#include "draw.h"
#include "drc/drc.h"
#include "error.h"
#include "find.h"
#include "flags.h"
#include "misc.h"
#include "rtree.h"
#include "object_list.h"
#include "polygon.h"
#include "pcb-printf.h"
#include "search.h"
#include "set.h"
#include "undo.h"
#include "rats.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#undef DEBUG

/* ---------------------------------------------------------------------------
 * some local macros
 */

#define	SEPARATE(FP)							\
	{											\
		int	i;									\
		fputc('#', (FP));						\
		for (i = Settings.CharPerLine; i; i--)	\
			fputc('=', (FP));					\
		fputc('\n', (FP));						\
	}

#define LIST_ENTRY(list,I)      (((AnyObjectType **)list->Data)[(I)])
#define PADLIST_ENTRY(L,I)      (((PadType **)PadList[(L)].Data)[(I)])
#define LINELIST_ENTRY(L,I)     (((LineType **)LineList[(L)].Data)[(I)])
#define ARCLIST_ENTRY(L,I)      (((ArcType **)ArcList[(L)].Data)[(I)])
#define RATLIST_ENTRY(I)        (((RatType **)RatList.Data)[(I)])
#define POLYGONLIST_ENTRY(L,I)  (((PolygonType **)PolygonList[(L)].Data)[(I)])
#define PVLIST_ENTRY(I)         (((PinType **)PVList.Data)[(I)])

#define IS_PV_ON_RAT(PV, Rat) \
	(IsPointOnLineEnd((PV)->X,(PV)->Y, (Rat)))

#define IS_PV_ON_ARC(PV, Arc)	\
	(TEST_FLAG(SQUAREFLAG, (PV)) ? \
		IsArcInRectangle( \
			(PV)->X -MAX(((PV)->Thickness+1)/2 +Bloat,0), (PV)->Y -MAX(((PV)->Thickness+1)/2 +Bloat,0), \
			(PV)->X +MAX(((PV)->Thickness+1)/2 +Bloat,0), (PV)->Y +MAX(((PV)->Thickness+1)/2 +Bloat,0), \
			(Arc)) : \
		IsPointOnArc((PV)->X,(PV)->Y,MAX((PV)->Thickness/2.0 + Bloat,0.0), (Arc)))

#define	IS_PV_ON_PAD(PV,Pad) \
	( IsPointInPad((PV)->X, (PV)->Y, MAX((PV)->Thickness/2 +Bloat,0), (Pad)))


/*!
 * \brief Some local types.
 *
 * The two 'dummy' structs for PVs and Pads are necessary for creating
 * connection lists which include the element's name.
 */
typedef struct
{
  void **Data;                  /*!< Pointer to index data. */
  Cardinal Location,            /*!< Currently used position. */
    DrawLocation, Number,       /*!< Number of objects in list. */
    Size;
} ListType;

/* ---------------------------------------------------------------------------
 * some local identifiers
 */

/* Bloat is used to change the size of objects before checking for overlaps.
 * This is used in the DRC check to detect things that are too close, or
 * don't overlap enough.*/
static Coord Bloat = 0;

static bool drc = false;     /*!< Whether to stop if finding something not found. */
static Cardinal TotalP, TotalV;
static ListType LineList[MAX_LAYER],    /*!< List of objects to. */
  PolygonList[MAX_LAYER], ArcList[MAX_LAYER], PadList[2], RatList, PVList;

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static bool LookupLOConnectionsToLine (LineType *, Cardinal, int, bool, bool);
static bool LookupLOConnectionsToPad (PadType *, Cardinal, int, bool);
static bool LookupLOConnectionsToPolygon (PolygonType *, Cardinal, int, bool);
static bool LookupLOConnectionsToArc (ArcType *, Cardinal, int, bool);
static bool LookupLOConnectionsToRatEnd (PointType *, Cardinal, int);
static bool IsRatPointOnLineEnd (PointType *, LineType *);
static bool ArcArcIntersect (ArcType *, ArcType *);
static bool PrepareNextLoop (FILE *);
static void DrawNewConnections (void);

/*!
 * \brief.
 *
 * Some of the 'pad' routines are the same as for lines because the 'pad'
 * struct starts with a line struct. See global.h for details.
 */
bool
LinePadIntersect (LineType *Line, PadType *Pad)
{
  return LineLineIntersect ((Line), (LineType *)Pad);
}

bool
ArcPadIntersect (ArcType *Arc, PadType *Pad)
{
  return LineArcIntersect ((LineType *) (Pad), (Arc));
}

/*
 * Add an object to the specified list.
 *
 * Returning true will (always?) abort the algorithm, false will allow it to
 * continue.
 *
 */
static bool
add_object_to_list (ListType *list, int type, void *ptr1, void *ptr2, void *ptr3, int flag)
{
  AnyObjectType *object = (AnyObjectType *)ptr2;

  /* Set the appropriate flag to indicate the object appears in one of the
   * lists. This is how we later compare runs.
   */
  AddObjectToFlagUndoList (type, ptr1, ptr2, ptr3);
  SET_FLAG (flag, object);

  /* Add the object to the list. */  
  LIST_ENTRY (list, list->Number) = object;
  list->Number++;

#ifdef DEBUG
  if (list->Number > list->Size)
    printf ("add_object_to_list overflow! type=%i num=%d size=%d\n", type, list->Number, list->Size);
#endif

  /* if drc is true, then we want to abort the algorithm if a new object is
   * found. The first time through, the SELECTEDFLAG is set on all objects
   * that are found. So, if SELECTEDFLAG is set, then the object is already
   * known. 
   *
   * TODO: This is not very flexible and requires pre-ordained knowledge of
   * how to use the SELECTEDFLAG.
   */
  if (drc && !TEST_FLAG (SELECTEDFLAG, object))
    return (SetThing (type, ptr1, ptr2, ptr3));
  return false;
}

static bool
ADD_PV_TO_LIST (PinType *Pin, int flag)
{
  return add_object_to_list (&PVList, Pin->Element ? PIN_TYPE : VIA_TYPE,
                             Pin->Element ? Pin->Element : Pin, Pin, Pin, flag);
}

static bool
ADD_PAD_TO_LIST (Cardinal L, PadType *Pad, int flag)
{
  return add_object_to_list (&PadList[L], PAD_TYPE, Pad->Element, Pad, Pad, flag);
}

static bool
ADD_LINE_TO_LIST (Cardinal L, LineType *Ptr, int flag)
{
  return add_object_to_list (&LineList[L], LINE_TYPE, LAYER_PTR (L), Ptr, Ptr, flag);
}

static bool
ADD_ARC_TO_LIST (Cardinal L, ArcType *Ptr, int flag)
{
  return add_object_to_list (&ArcList[L], ARC_TYPE, LAYER_PTR (L), Ptr, Ptr, flag);
}

static bool
ADD_RAT_TO_LIST (RatType *Ptr, int flag)
{
  return add_object_to_list (&RatList, RATLINE_TYPE, Ptr, Ptr, Ptr, flag);
}

static bool
ADD_POLYGON_TO_LIST (Cardinal L, PolygonType *Ptr, int flag)
{
  return add_object_to_list (&PolygonList[L], POLYGON_TYPE, LAYER_PTR (L), Ptr, Ptr, flag);
}

static BoxType
expand_bounds (BoxType *box_in)
{
  BoxType box_out = *box_in;

  if (Bloat > 0)
    {
      box_out.X1 -= Bloat;
      box_out.X2 += Bloat;
      box_out.Y1 -= Bloat;
      box_out.Y2 += Bloat;
    }

  return box_out;
}

bool
PinLineIntersect (PinType *PV, LineType *Line)
{
  /* IsLineInRectangle already has Bloat factor */
  return TEST_FLAG (SQUAREFLAG,
                    PV) ? IsLineInRectangle (PV->X - (PIN_SIZE (PV) + 1) / 2,
                                             PV->Y - (PIN_SIZE (PV) + 1) / 2,
                                             PV->X + (PIN_SIZE (PV) + 1) / 2,
                                             PV->Y + (PIN_SIZE (PV) + 1) / 2,
                                             Line) : IsPointInPad (PV->X,
                                                                    PV->Y,
								   MAX (PIN_SIZE (PV)
                                                                         /
                                                                         2.0 +
                                                                         Bloat,
                                                                         0.0),
                                                                    (PadType *)Line);
}

bool
BoxBoxIntersection (BoxType *b1, BoxType *b2)
{
  if (b2->X2 < b1->X1 || b2->X1 > b1->X2)
    return false;
  if (b2->Y2 < b1->Y1 || b2->Y1 > b1->Y2)
    return false;
  return true;
}

static bool
PadPadIntersect (PadType *p1, PadType *p2)
{
  return LinePadIntersect ((LineType *) p1, p2);
}

static inline bool
PV_TOUCH_PV (PinType *PV1, PinType *PV2)
{
  double t1, t2;
  BoxType b1, b2;

  t1 = MAX (PV1->Thickness / 2.0 + Bloat, 0);
  t2 = MAX (PV2->Thickness / 2.0 + Bloat, 0);
  if (IsPointOnPin (PV1->X, PV1->Y, t1, PV2)
      || IsPointOnPin (PV2->X, PV2->Y, t2, PV1))
    return true;
  if (!TEST_FLAG (SQUAREFLAG, PV1) || !TEST_FLAG (SQUAREFLAG, PV2))
    return false;
  /* check for square/square overlap */
  b1.X1 = PV1->X - t1;
  b1.X2 = PV1->X + t1;
  b1.Y1 = PV1->Y - t1;
  b1.Y2 = PV1->Y + t1;
  t2 = PV2->Thickness / 2.0;
  b2.X1 = PV2->X - t2;
  b2.X2 = PV2->X + t2;
  b2.Y1 = PV2->Y - t2;
  b2.Y2 = PV2->Y + t2;
  return BoxBoxIntersection (&b1, &b2);
}

/*!
 * \brief Releases all allocated memory.
 */
static void
FreeLayoutLookupMemory (void)
{
  Cardinal i;

  for (i = 0; i < max_copper_layer; i++)
    {
      free (LineList[i].Data);
      LineList[i].Data = NULL;
      free (ArcList[i].Data);
      ArcList[i].Data = NULL;
      free (PolygonList[i].Data);
      PolygonList[i].Data = NULL;
    }
  free (PVList.Data);
  PVList.Data = NULL;
  free (RatList.Data);
  RatList.Data = NULL;
}

static void
FreeComponentLookupMemory (void)
{
  free (PadList[0].Data);
  PadList[0].Data = NULL;
  free (PadList[1].Data);
  PadList[1].Data = NULL;
}

/*!
 * \brief Allocates memory for component related stacks ...
 *
 * Initializes index and sorts it by X1 and X2.
 */
static void
InitComponentLookup (void)
{
  Cardinal NumberOfPads[2];
  Cardinal i;

  /* initialize pad data; start by counting the total number
   * on each of the two possible layers
   */
  NumberOfPads[TOP_SIDE] = NumberOfPads[BOTTOM_SIDE] = 0;
  ALLPAD_LOOP (PCB->Data);
  {
    if (TEST_FLAG (ONSOLDERFLAG, pad))
      NumberOfPads[BOTTOM_SIDE]++;
    else
      NumberOfPads[TOP_SIDE]++;
  }
  ENDALL_LOOP;
  for (i = 0; i < 2; i++)
    {
      /* allocate memory for working list */
      PadList[i].Data = (void **)calloc (NumberOfPads[i], sizeof (PadType *));

      /* clear some struct members */
      PadList[i].Location = 0;
      PadList[i].DrawLocation = 0;
      PadList[i].Number = 0;
      PadList[i].Size = NumberOfPads[i];
    }
}

/*!
 * \brief Allocates memory for layout related stacks ...
 *
 * Initializes index and sorts it by X1 and X2.
 */
static void
InitLayoutLookup (void)
{
  Cardinal i;

  /* initialize line arc and polygon data */
  for (i = 0; i < max_copper_layer; i++)
    {
      LayerType *layer = LAYER_PTR (i);

      if (layer->LineN)
        {
          /* allocate memory for line pointer lists */
          LineList[i].Data = (void **)calloc (layer->LineN, sizeof (LineType *));
          LineList[i].Size = layer->LineN;
        }
      if (layer->ArcN)
        {
          ArcList[i].Data = (void **)calloc (layer->ArcN, sizeof (ArcType *));
          ArcList[i].Size = layer->ArcN;
        }


      /* allocate memory for polygon list */
      if (layer->PolygonN)
        {
          PolygonList[i].Data = (void **)calloc (layer->PolygonN, sizeof (PolygonType *));
          PolygonList[i].Size = layer->PolygonN;
        }

      /* clear some struct members */
      LineList[i].Location = 0;
      LineList[i].DrawLocation = 0;
      LineList[i].Number = 0;
      ArcList[i].Location = 0;
      ArcList[i].DrawLocation = 0;
      ArcList[i].Number = 0;
      PolygonList[i].Location = 0;
      PolygonList[i].DrawLocation = 0;
      PolygonList[i].Number = 0;
    }

  if (PCB->Data->pin_tree)
    TotalP = PCB->Data->pin_tree->size;
  else
    TotalP = 0;
  if (PCB->Data->via_tree)
    TotalV = PCB->Data->via_tree->size;
  else
    TotalV = 0;
  /* allocate memory for 'new PV to check' list and clear struct */
  PVList.Data = (void **)calloc (TotalP + TotalV, sizeof (PinType *));
  PVList.Size = TotalP + TotalV;
  PVList.Location = 0;
  PVList.DrawLocation = 0;
  PVList.Number = 0;
  /* Initialize ratline data */
  RatList.Data = (void **)calloc (PCB->Data->RatN, sizeof (RatType *));
  RatList.Size = PCB->Data->RatN;
  RatList.Location = 0;
  RatList.DrawLocation = 0;
  RatList.Number = 0;
}

struct pv_info
{
  Cardinal layer;
  PinType *pv;
  int flag;
  jmp_buf env;
};

static int
LOCtoPVline_callback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  struct pv_info *i = (struct pv_info *) cl;

  if (!ViaIsOnLayerGroup (i->pv, GetLayerGroupNumberByNumber (i->layer)))
    return 0;

  if (!TEST_FLAG (i->flag, line) && PinLineIntersect (i->pv, line) &&
      !TEST_FLAG (HOLEFLAG, i->pv))
    {
      if (ADD_LINE_TO_LIST (i->layer, line, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPVarc_callback (const BoxType * b, void *cl)
{
  ArcType *arc = (ArcType *) b;
  struct pv_info *i = (struct pv_info *) cl;

  if (!ViaIsOnLayerGroup (i->pv, GetLayerGroupNumberByNumber (i->layer)))
    return 0;

  if (!TEST_FLAG (i->flag, arc) && IS_PV_ON_ARC (i->pv, arc) &&
      !TEST_FLAG (HOLEFLAG, i->pv))
    {
      if (ADD_ARC_TO_LIST (i->layer, arc, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPVpad_callback (const BoxType * b, void *cl)
{
  PadType *pad = (PadType *) b;
  struct pv_info *i = (struct pv_info *) cl;

  if (!ViaIsOnLayerGroup (i->pv, GetLayerGroupNumberBySide (TEST_FLAG (ONSOLDERFLAG, pad) ? BOTTOM_SIDE : TOP_SIDE)))
    return 0;

  if (!TEST_FLAG (i->flag, pad) && IS_PV_ON_PAD (i->pv, pad) &&
      !TEST_FLAG (HOLEFLAG, i->pv) &&
      ADD_PAD_TO_LIST (TEST_FLAG (ONSOLDERFLAG, pad) ? BOTTOM_SIDE :
                       TOP_SIDE, pad, i->flag))
    longjmp (i->env, 1);
  return 0;
}

static int
LOCtoPVrat_callback (const BoxType * b, void *cl)
{
  RatType *rat = (RatType *) b;
  struct pv_info *i = (struct pv_info *) cl;

  if (!TEST_FLAG (i->flag, rat) && IS_PV_ON_RAT (i->pv, rat) &&
      ADD_RAT_TO_LIST (rat, i->flag))
    longjmp (i->env, 1);
  return 0;
}

static int
LOCtoPVpoly_callback (const BoxType * b, void *cl)
{
  PolygonType *polygon = (PolygonType *) b;
  struct pv_info *i = (struct pv_info *) cl;

  if (!ViaIsOnLayerGroup (i->pv, GetLayerGroupNumberByNumber (i->layer)))
    return 0;

  /* if the pin doesn't have a therm and polygon is clearing
   * then it can't touch due to clearance, so skip the expensive
   * test. If it does have a therm, you still need to test
   * because it might not be inside the polygon, or it could
   * be on an edge such that it doesn't actually touch.
   */
  if (!TEST_FLAG (i->flag, polygon) && !TEST_FLAG (HOLEFLAG, i->pv) &&
                                       (TEST_THERM (i->layer, i->pv) ||
                                        !TEST_FLAG (CLEARPOLYFLAG,
                                                    polygon)
                                        || !i->pv->Clearance))
    {
      double wide = MAX (0.5 * i->pv->Thickness + Bloat, 0);
      if (TEST_FLAG (SQUAREFLAG, i->pv))
        {
          Coord x1 = i->pv->X - (i->pv->Thickness + 1 + Bloat) / 2;
          Coord x2 = i->pv->X + (i->pv->Thickness + 1 + Bloat) / 2;
          Coord y1 = i->pv->Y - (i->pv->Thickness + 1 + Bloat) / 2;
          Coord y2 = i->pv->Y + (i->pv->Thickness + 1 + Bloat) / 2;
          if (IsRectangleInPolygon (x1, y1, x2, y2, polygon)
              && ADD_POLYGON_TO_LIST (i->layer, polygon, i->flag))
            longjmp (i->env, 1);
        }
      else if (TEST_FLAG (OCTAGONFLAG, i->pv))
        {
          POLYAREA *oct = OctagonPoly (i->pv->X, i->pv->Y, i->pv->Thickness / 2);
          if (isects (oct, polygon, true)
              && ADD_POLYGON_TO_LIST (i->layer, polygon, i->flag))
            longjmp (i->env, 1);
        }
      else if (IsPointInPolygon (i->pv->X, i->pv->Y, wide,
                                 polygon)
               && ADD_POLYGON_TO_LIST (i->layer, polygon, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

/*!
 * \brief Checks if a PV is connected to LOs, if it is, the LO is added
 * to the appropriate list and the 'used' flag is set.
 */
static bool
LookupLOConnectionsToPVList (int flag, bool AndRats)
{
  Cardinal layer_no;
  struct pv_info info;

  info.flag = flag;

  /* loop over all PVs currently on list */
  while (PVList.Location < PVList.Number)
    {
      BoxType search_box;

      /* get pointer to data */
      info.pv = PVLIST_ENTRY (PVList.Location);
      search_box = expand_bounds (&info.pv->BoundingBox);

      /* check pads */
      if (setjmp (info.env) == 0)
        r_search (PCB->Data->pad_tree, &search_box, NULL,
                  LOCtoPVpad_callback, &info);
      else
        return true;

      /* now all lines, arcs and polygons of the several layers */
      for (layer_no = 0; layer_no < max_copper_layer; layer_no++)
        {
          LayerType *layer = LAYER_PTR (layer_no);

          if (layer->no_drc)
             continue;

          info.layer = layer_no;

          /* add touching lines */
          if (setjmp (info.env) == 0)
            r_search (layer->line_tree, &search_box,
                      NULL, LOCtoPVline_callback, &info);
          else
            return true;
          /* add touching arcs */
          if (setjmp (info.env) == 0)
            r_search (layer->arc_tree, &search_box,
                      NULL, LOCtoPVarc_callback, &info);
          else
            return true;
          /* check all polygons */
          if (setjmp (info.env) == 0)
            r_search (layer->polygon_tree, &search_box,
                      NULL, LOCtoPVpoly_callback, &info);
          else
            return true;
        }
      /* Check for rat-lines that may intersect the PV */
      if (AndRats)
        {
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->rat_tree, &search_box, NULL,
                      LOCtoPVrat_callback, &info);
          else
            return true;
        }
      PVList.Location++;
    }
  return false;
}

/*!
 * \brief Find all connections between LO at the current list position
 * and new LOs.
 */
static bool
LookupLOConnectionsToLOList (int flag, bool AndRats)
{
  bool done;
  Cardinal i, group, layer, ratposition,
    lineposition[MAX_LAYER],
    polyposition[MAX_LAYER], arcposition[MAX_LAYER], padposition[2];

  /* copy the current LO list positions; the original data is changed
   * by 'LookupPVConnectionsToLOList()' which has to check the same
   * list entries plus the new ones
   */
  for (i = 0; i < max_copper_layer; i++)
    {
      lineposition[i] = LineList[i].Location;
      polyposition[i] = PolygonList[i].Location;
      arcposition[i]  = ArcList[i].Location;
    }
  for (i = 0; i < 2; i++)
    padposition[i] = PadList[i].Location;
  ratposition = RatList.Location;

  /* loop over all new LOs in the list; recurse until no
   * more new connections in the layergroup were found
   */
  do
    {
      Cardinal *position;

      if (AndRats)
        {
          position = &ratposition;
          for (; *position < RatList.Number; (*position)++)
            {
              group = RATLIST_ENTRY (*position)->group1;
              if (LookupLOConnectionsToRatEnd
                  (&(RATLIST_ENTRY (*position)->Point1), group, flag))
                return (true);
              group = RATLIST_ENTRY (*position)->group2;
              if (LookupLOConnectionsToRatEnd
                  (&(RATLIST_ENTRY (*position)->Point2), group, flag))
                return (true);
            }
        }
      /* loop over all layergroups */
      for (group = 0; group < max_group; group++)
        {
          Cardinal entry;

          for (entry = 0; entry < PCB->LayerGroups.Number[group]; entry++)
            {
              layer = PCB->LayerGroups.Entries[group][entry];

              /* be aware that the layer number equal max_copper_layer
               * and max_copper_layer+1 have a special meaning for pads
               */
              if (layer < max_copper_layer)
                {
                  /* try all new lines */
                  position = &lineposition[layer];
                  for (; *position < LineList[layer].Number; (*position)++)
                    if (LookupLOConnectionsToLine
                        (LINELIST_ENTRY (layer, *position), group, flag, true, AndRats))
                      return (true);

                  /* try all new arcs */
                  position = &arcposition[layer];
                  for (; *position < ArcList[layer].Number; (*position)++)
                    if (LookupLOConnectionsToArc
                        (ARCLIST_ENTRY (layer, *position), group, flag, AndRats))
                      return (true);

                  /* try all new polygons */
                  position = &polyposition[layer];
                  for (; *position < PolygonList[layer].Number; (*position)++)
                    if (LookupLOConnectionsToPolygon
                        (POLYGONLIST_ENTRY (layer, *position), group, flag, AndRats))
                      return (true);
                }
              else
                {
                  /* try all new pads */
                  layer -= max_copper_layer;
                  if (layer > 1)
                    {
                      Message (_("bad layer number %d max_copper_layer=%d in find.c\n"),
                               layer, max_copper_layer);
                      return false;
                    }
                  position = &padposition[layer];
                  for (; *position < PadList[layer].Number; (*position)++)
                    if (LookupLOConnectionsToPad
                        (PADLIST_ENTRY (layer, *position), group, flag, AndRats))
                      return (true);
                }
            }
        }

      /* check if all lists are done; Later for-loops
       * may have changed the prior lists
       */
      done = !AndRats || ratposition >= RatList.Number;
      done = done && padposition[0] >= PadList[0].Number &&
                     padposition[1] >= PadList[1].Number;
      for (layer = 0; layer < max_copper_layer; layer++)
        done = done &&
               lineposition[layer] >= LineList[layer].Number &&
               arcposition[layer]  >= ArcList[layer].Number &&
               polyposition[layer] >= PolygonList[layer].Number;
    }
  while (!done);
  return (false);
}

/*
 * This function is an r_search callback. It's called to check if a pin or
 * via is overlapping with another pin or via.
 */
static int
pv_pv_callback (const BoxType * b, void *cl)
{
  /* Cast the object found by the r_search, it's known to be a pin */
  PinType *pin = (PinType *) b;
  struct pv_info *i = (struct pv_info *) cl;
  bool pv_overlap = false;
  Cardinal l;

  /* If both vias are buried, check if the layers of the found via (pin)
   * overlap with the layers of the original via (i->pv).
   *
   * TODO: Why isn't PinPinIntersect called here?
   * TODO: Why aren't we checking the other flags, like we do below?
   */
  if (VIA_IS_BURIED (pin) && VIA_IS_BURIED (i->pv))
    {
      for (l = pin->BuriedFrom ; l <= pin->BuriedTo; l++)
        {
	   if (ViaIsOnLayerGroup (i->pv, GetLayerGroupNumberByNumber (l)))
	     {
	       pv_overlap = true;
	       break;
	     }
	}
      if (!pv_overlap)
	return 0;
    }

  /* If either of the vias is a thru via, there is potential overlap. */
  if (!TEST_FLAG (i->flag, pin) && PV_TOUCH_PV (i->pv, pin))
    {
	  /* If it's only a hole (no copper) then just issue a warning to the
	   * log, and highlight the pin. It doesn't affect the netlist.
	   */
      if (TEST_FLAG (HOLEFLAG, pin) || TEST_FLAG (HOLEFLAG, i->pv))
        {
          SET_FLAG (WARNFLAG, pin);
          Settings.RatWarn = true;
          if (pin->Element)
            Message (_("WARNING: Hole too close to pin.\n"));
          else
            Message (_("WARNING: Hole too close to via.\n"));
        }
      else if (ADD_PV_TO_LIST (pin, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

/*!
 * \brief Searches for new PVs that are connected to PVs on the list.
 */
static bool
LookupPVConnectionsToPVList (int flag)
{
  Cardinal save_place;
  struct pv_info info;

  info.flag = flag;

  /* loop over all PVs on list */
  save_place = PVList.Location;
  while (PVList.Location < PVList.Number)
    {
      BoxType search_box;

      /* get pointer to data */
      info.pv = PVLIST_ENTRY (PVList.Location);
      search_box = expand_bounds ((BoxType *)info.pv);

      if (setjmp (info.env) == 0)
        r_search (PCB->Data->via_tree, &search_box, NULL,
                  pv_pv_callback, &info);
      else
        return true;
      if (setjmp (info.env) == 0)
        r_search (PCB->Data->pin_tree, &search_box, NULL,
                  pv_pv_callback, &info);
      else
        return true;
      PVList.Location++;
    }
  PVList.Location = save_place;
  return (false);
}

struct lo_info
{
  Cardinal layer;
  LineType *line;
  PadType *pad;
  ArcType *arc;
  PolygonType *polygon;
  RatType *rat;
  int flag;
  jmp_buf env;
};

static int
pv_line_callback (const BoxType * b, void *cl)
{
  PinType *pv = (PinType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!ViaIsOnLayerGroup (pv, GetLayerGroupNumberByNumber (i->layer)))
    return 0;

  if (!TEST_FLAG (i->flag, pv) && PinLineIntersect (pv, i->line))
    {
      if (TEST_FLAG (HOLEFLAG, pv))
        {
          SET_FLAG (WARNFLAG, pv);
          Settings.RatWarn = true;
          Message (_("WARNING: Hole too close to line.\n"));
        }
      else if (ADD_PV_TO_LIST (pv, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
pv_pad_callback (const BoxType * b, void *cl)
{
  PinType *pv = (PinType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!ViaIsOnLayerGroup (pv, GetLayerGroupNumberBySide (i->layer)))
    return 0;

  if (!TEST_FLAG (i->flag, pv) && IS_PV_ON_PAD (pv, i->pad))
    {
      if (TEST_FLAG (HOLEFLAG, pv))
        {
          SET_FLAG (WARNFLAG, pv);
          Settings.RatWarn = true;
          Message (_("WARNING: Hole too close to pad.\n"));
        }
      else if (ADD_PV_TO_LIST (pv, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
pv_arc_callback (const BoxType * b, void *cl)
{
  PinType *pv = (PinType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!ViaIsOnLayerGroup (pv, GetLayerGroupNumberByNumber (i->layer)))
    return 0;

  if (!TEST_FLAG (i->flag, pv) && IS_PV_ON_ARC (pv, i->arc))
    {
      if (TEST_FLAG (HOLEFLAG, pv))
        {
          SET_FLAG (WARNFLAG, pv);
          Settings.RatWarn = true;
          Message (_("WARNING: Hole touches arc.\n"));
        }
      else if (ADD_PV_TO_LIST (pv, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
pv_poly_callback (const BoxType * b, void *cl)
{
  PinType *pv = (PinType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!ViaIsOnLayerGroup (pv, GetLayerGroupNumberByNumber (i->layer)))
    return 0;

  /* note that holes in polygons are ok, so they don't generate warnings. */
  if (!TEST_FLAG (i->flag, pv) && !TEST_FLAG (HOLEFLAG, pv) &&
                                  (TEST_THERM (i->layer, pv) ||
                                   !TEST_FLAG (CLEARPOLYFLAG, i->polygon) ||
                                   !pv->Clearance))
    {
      if (TEST_FLAG (SQUAREFLAG, pv))
        {
          Coord x1, x2, y1, y2;
          x1 = pv->X - (PIN_SIZE (pv) + 1 + Bloat) / 2;
          x2 = pv->X + (PIN_SIZE (pv) + 1 + Bloat) / 2;
          y1 = pv->Y - (PIN_SIZE (pv) + 1 + Bloat) / 2;
          y2 = pv->Y + (PIN_SIZE (pv) + 1 + Bloat) / 2;
          if (IsRectangleInPolygon (x1, y1, x2, y2, i->polygon)
              && ADD_PV_TO_LIST (pv, i->flag))
            longjmp (i->env, 1);
        }
      else if (TEST_FLAG (OCTAGONFLAG, pv))
        {
          POLYAREA *oct = OctagonPoly (pv->X, pv->Y, PIN_SIZE (pv) / 2);
          if (isects (oct, i->polygon, true) && ADD_PV_TO_LIST (pv, i->flag))
            longjmp (i->env, 1);
        }
      else
        {
          if (IsPointInPolygon
              (pv->X, pv->Y, PIN_SIZE (pv) * 0.5 + Bloat, i->polygon)
              && ADD_PV_TO_LIST (pv, i->flag))
            longjmp (i->env, 1);
        }
    }
  return 0;
}

static int
pv_rat_callback (const BoxType * b, void *cl)
{
  PinType *pv = (PinType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  /* rats can't cause DRC so there is no early exit */
  if (!TEST_FLAG (i->flag, pv) && IS_PV_ON_RAT (pv, i->rat))
    ADD_PV_TO_LIST (pv, i->flag);
  return 0;
}

/*!
 * \brief Searches for new PVs that are connected to NEW LOs on the list.
 *
 * This routine updates the position counter of the lists too.
 */
static bool
LookupPVConnectionsToLOList (int flag, bool AndRats)
{
  Cardinal layer_no;
  struct lo_info info;

  info.flag = flag;

  /* loop over all layers */
  for (layer_no = 0; layer_no < max_copper_layer; layer_no++)
    {
      LayerType *layer = LAYER_PTR (layer_no);

      if (layer->no_drc)
                       continue;
      /* do nothing if there are no PV's */
      if (TotalP + TotalV == 0)
        {
          LineList[layer_no].Location = LineList[layer_no].Number;
          ArcList[layer_no].Location = ArcList[layer_no].Number;
          PolygonList[layer_no].Location = PolygonList[layer_no].Number;
          continue;
        }

      info.layer = layer_no;
      /* check all lines */
      while (LineList[layer_no].Location < LineList[layer_no].Number)
        {
          BoxType search_box;

          info.line = LINELIST_ENTRY (layer_no, LineList[layer_no].Location);
          search_box = expand_bounds ((BoxType *)info.line);

          if (setjmp (info.env) == 0)
            r_search (PCB->Data->via_tree, &search_box, NULL,
                      pv_line_callback, &info);
          else
            return true;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pin_tree, &search_box, NULL,
                      pv_line_callback, &info);
          else
            return true;
          LineList[layer_no].Location++;
        }

      /* check all arcs */
      while (ArcList[layer_no].Location < ArcList[layer_no].Number)
        {
          BoxType search_box;

          info.arc = ARCLIST_ENTRY (layer_no, ArcList[layer_no].Location);
          search_box = expand_bounds ((BoxType *)info.arc);

          if (setjmp (info.env) == 0)
            r_search (PCB->Data->via_tree, &search_box, NULL,
                      pv_arc_callback, &info);
          else
            return true;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pin_tree, &search_box, NULL,
                      pv_arc_callback, &info);
          else
            return true;
          ArcList[layer_no].Location++;
        }

      /* now all polygons */
      info.layer = layer_no;
      while (PolygonList[layer_no].Location < PolygonList[layer_no].Number)
        {
          BoxType search_box;

          info.polygon = POLYGONLIST_ENTRY (layer_no, PolygonList[layer_no].Location);
          search_box = expand_bounds ((BoxType *)info.polygon);

          if (setjmp (info.env) == 0)
            r_search (PCB->Data->via_tree, &search_box, NULL,
                      pv_poly_callback, &info);
          else
            return true;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pin_tree, &search_box, NULL,
                      pv_poly_callback, &info);
          else
            return true;
          PolygonList[layer_no].Location++;
        }
    }

  /* loop over all pad-layers */
  for (layer_no = 0; layer_no < 2; layer_no++)
    {
      /* do nothing if there are no PV's */
      if (TotalP + TotalV == 0)
        {
          PadList[layer_no].Location = PadList[layer_no].Number;
          continue;
        }

      /* check all pads; for a detailed description see
       * the handling of lines in this subroutine
       */
      while (PadList[layer_no].Location < PadList[layer_no].Number)
        {
          BoxType search_box;

          info.layer = layer_no;
          info.pad = PADLIST_ENTRY (layer_no, PadList[layer_no].Location);
          search_box = expand_bounds ((BoxType *)info.pad);

          if (setjmp (info.env) == 0)
            r_search (PCB->Data->via_tree, &search_box, NULL,
                      pv_pad_callback, &info);
          else
            return true;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pin_tree, &search_box, NULL,
                      pv_pad_callback, &info);
          else
            return true;
          PadList[layer_no].Location++;
        }
    }

  /* do nothing if there are no PV's */
  if (TotalP + TotalV == 0)
    RatList.Location = RatList.Number;

  /* check all rat-lines */
  if (AndRats)
    {
      while (RatList.Location < RatList.Number)
        {
          info.rat = RATLIST_ENTRY (RatList.Location);
          r_search_pt (PCB->Data->via_tree, & info.rat->Point1, 1, NULL,
                    pv_rat_callback, &info);
          r_search_pt (PCB->Data->via_tree, & info.rat->Point2, 1, NULL,
                    pv_rat_callback, &info);
          r_search_pt (PCB->Data->pin_tree, & info.rat->Point1, 1, NULL,
                    pv_rat_callback, &info);
          r_search_pt (PCB->Data->pin_tree, & info.rat->Point2, 1, NULL,
                    pv_rat_callback, &info);

          RatList.Location++;
        }
    }
  return (false);
}

/*!
 * \brief Reduce arc start angle and delta to 0..360.
 */
static void
normalize_angles (Angle *sa, Angle *d)
{
  if (*d < 0)
    {
      *sa += *d;
      *d = - *d;
    }
  if (*d > 360) /* full circle */
    *d = 360;
  *sa = NormalizeAngle (*sa);
}

static int
radius_crosses_arc (double x, double y, ArcType *arc)
{
  double alpha = atan2 (y - arc->Y, -x + arc->X) * RAD_TO_DEG;
  Angle sa = arc->StartAngle, d = arc->Delta;

  normalize_angles (&sa, &d);
  if (alpha < 0)
    alpha += 360;
  if (sa <= alpha)
    return (sa + d) >= alpha;
  return (sa + d - 360) >= alpha;
}

static void
get_arc_ends (Coord *box, ArcType *arc)
{
  box[0] = arc->X - arc->Width  * cos (M180 * arc->StartAngle);
  box[1] = arc->Y + arc->Height * sin (M180 * arc->StartAngle);
  box[2] = arc->X - arc->Width  * cos (M180 * (arc->StartAngle + arc->Delta));
  box[3] = arc->Y + arc->Height * sin (M180 * (arc->StartAngle + arc->Delta));
}

/*!
 * \brief Check if two arcs intersect.
 *
 * First we check for circle intersections,
 * then find the actual points of intersection
 * and test them to see if they are on arcs.
 *
 * Consider a, the distance from the center of arc 1
 * to the point perpendicular to the intersecting points.
 *
 * \ta = (r1^2 - r2^2 + l^2)/(2l)
 *
 * The perpendicular distance to the point of intersection
 * is then:
 *
 * \td = sqrt(r1^2 - a^2)
 *
 * The points of intersection would then be:
 *
 * \tx = X1 + a/l dx +- d/l dy
 *
 * \ty = Y1 + a/l dy -+ d/l dx
 *
 * Where dx = X2 - X1 and dy = Y2 - Y1.
 */
static bool
ArcArcIntersect (ArcType *Arc1, ArcType *Arc2)
{
  double x, y, dx, dy, r1, r2, a, d, l, t, t1, t2, dl;
  Coord pdx, pdy;
  Coord box[8];

  t  = 0.5 * Arc1->Thickness + Bloat;
  t2 = 0.5 * Arc2->Thickness;
  t1 = t2 + Bloat;

  /* too thin arc */
  if (t < 0 || t1 < 0)
    return false;

  /* try the end points first */
  get_arc_ends (&box[0], Arc1);
  get_arc_ends (&box[4], Arc2);
  if (IsPointOnArc (box[0], box[1], t, Arc2)
      || IsPointOnArc (box[2], box[3], t, Arc2)
      || IsPointOnArc (box[4], box[5], t, Arc1)
      || IsPointOnArc (box[6], box[7], t, Arc1))
    return true;

  pdx = Arc2->X - Arc1->X;
  pdy = Arc2->Y - Arc1->Y;
  dl = Distance (Arc1->X, Arc1->Y, Arc2->X, Arc2->Y);
  /* concentric arcs, simpler intersection conditions */
  if (dl < 0.5)
    {
      if ((Arc1->Width - t >= Arc2->Width - t2
           && Arc1->Width - t <= Arc2->Width + t2)
          || (Arc1->Width + t >= Arc2->Width - t2
              && Arc1->Width + t <= Arc2->Width + t2))
        {
	  Angle sa1 = Arc1->StartAngle, d1 = Arc1->Delta;
	  Angle sa2 = Arc2->StartAngle, d2 = Arc2->Delta;
	  /* NB the endpoints have already been checked,
	     so we just compare the angles */

	  normalize_angles (&sa1, &d1);
	  normalize_angles (&sa2, &d2);
	  /* sa1 == sa2 was caught when checking endpoints */
	  if (sa1 > sa2)
            if (sa1 < sa2 + d2 || sa1 + d1 - 360 > sa2)
              return true;
	  if (sa2 > sa1)
	    if (sa2 < sa1 + d1 || sa2 + d2 - 360 > sa1)
              return true;
        }
      return false;
    }
  r1 = Arc1->Width;
  r2 = Arc2->Width;
  /* arcs centerlines are too far or too near */
  if (dl > r1 + r2 || dl + r1 < r2 || dl + r2 < r1)
    {
      /* check the nearest to the other arc's center point */
      dx = pdx * r1 / dl;
      dy = pdy * r1 / dl;
      if (dl + r1 < r2) /* Arc1 inside Arc2 */
	{
	  dx = - dx;
	  dy = - dy;
	}

      if (radius_crosses_arc (Arc1->X + dx, Arc1->Y + dy, Arc1)
	  && IsPointOnArc (Arc1->X + dx, Arc1->Y + dy, t, Arc2))
	return true;

      dx = - pdx * r2 / dl;
      dy = - pdy * r2 / dl;
      if (dl + r2 < r1) /* Arc2 inside Arc1 */
	{
	  dx = - dx;
	  dy = - dy;
	}

      if (radius_crosses_arc (Arc2->X + dx, Arc2->Y + dy, Arc2)
	  && IsPointOnArc (Arc2->X + dx, Arc2->Y + dy, t1, Arc1))
	return true;
      return false;
    }

  l = dl * dl;
  r1 *= r1;
  r2 *= r2;
  a = 0.5 * (r1 - r2 + l) / l;
  r1 = r1 / l;
  d = r1 - a * a;
  /* the circles are too far apart to touch or probably just touch:
     check the nearest point */
  if (d < 0)
    d = 0;
  else
    d = sqrt (d);
  x = Arc1->X + a * pdx;
  y = Arc1->Y + a * pdy;
  dx = d * pdx;
  dy = d * pdy;
  if (radius_crosses_arc (x + dy, y - dx, Arc1)
      && IsPointOnArc (x + dy, y - dx, t, Arc2))
    return true;
  if (radius_crosses_arc (x + dy, y - dx, Arc2)
      && IsPointOnArc (x + dy, y - dx, t1, Arc1))
    return true;

  if (radius_crosses_arc (x - dy, y + dx, Arc1)
      && IsPointOnArc (x - dy, y + dx, t, Arc2))
    return true;
  if (radius_crosses_arc (x - dy, y + dx, Arc2)
      && IsPointOnArc (x - dy, y + dx, t1, Arc1))
    return true;
  return false;
}

/*!
 * \brief Tests if point is same as line end point.
 */
static bool
IsRatPointOnLineEnd (PointType *Point, LineType *Line)
{
  if ((Point->X == Line->Point1.X
       && Point->Y == Line->Point1.Y)
      || (Point->X == Line->Point2.X && Point->Y == Line->Point2.Y))
    return (true);
  return (false);
}

/*!
 * \brief Writes vertices of a squared line.
 */
static void 
form_slanted_rectangle (PointType p[4], LineType *l)
{
   double dwx = 0, dwy = 0;
   if (l->Point1.Y == l->Point2.Y)
     dwx = l->Thickness / 2.0;
   else if (l->Point1.X == l->Point2.X)
     dwy = l->Thickness / 2.0;
   else 
     {
       Coord dX = l->Point2.X - l->Point1.X;
       Coord dY = l->Point2.Y - l->Point1.Y;
       double r = Distance (l->Point1.X, l->Point1.Y, l->Point2.X, l->Point2.Y);
       dwx = l->Thickness / 2.0 / r * dX;
       dwy = l->Thickness / 2.0 / r * dY;
     }
    p[0].X = l->Point1.X - dwx + dwy; p[0].Y = l->Point1.Y - dwy - dwx;
    p[1].X = l->Point1.X - dwx - dwy; p[1].Y = l->Point1.Y - dwy + dwx;
    p[2].X = l->Point2.X + dwx - dwy; p[2].Y = l->Point2.Y + dwy + dwx;
    p[3].X = l->Point2.X + dwx + dwy; p[3].Y = l->Point2.Y + dwy - dwx;
}

/*!
 * \brief Checks if two lines intersect.
 *
 * <pre>
 * From news FAQ:
 *
 * Let A,B,C,D be 2-space position vectors.
 *
 * Then the directed line segments AB & CD are given by:
 *
 *      AB=A+r(B-A), r in [0,1]
 *
 *      CD=C+s(D-C), s in [0,1]
 *
 * If AB & CD intersect, then
 *
 *      A+r(B-A)=C+s(D-C), or
 *
 *      XA+r(XB-XA)=XC+s*(XD-XC)
 *
 *      YA+r(YB-YA)=YC+s(YD-YC)  for some r,s in [0,1]
 *
 * Solving the above for r and s yields
 *
 *          (YA-YC)(XD-XC)-(XA-XC)(YD-YC)
 *      r = -----------------------------  (eqn 1)
 *          (XB-XA)(YD-YC)-(YB-YA)(XD-XC)
 *
 *          (YA-YC)(XB-XA)-(XA-XC)(YB-YA)
 *      s = -----------------------------  (eqn 2)
 *          (XB-XA)(YD-YC)-(YB-YA)(XD-XC)
 *
 * Let I be the position vector of the intersection point, then:
 *
 *      I=A+r(B-A) or
 *
 *      XI=XA+r(XB-XA)
 *
 *      YI=YA+r(YB-YA)
 *
 * By examining the values of r & s, you can also determine some
 * other limiting conditions:
 *
 *      If 0<=r<=1 & 0<=s<=1, intersection exists
 *
 *          r<0 or r>1 or s<0 or s>1 line segments do not intersect
 *
 *      If the denominator in eqn 1 is zero, AB & CD are parallel
 *
 *      If the numerator in eqn 1 is also zero, AB & CD are coincident
 *
 * If the intersection point of the 2 lines are needed (lines in this
 * context mean infinite lines) regardless whether the two line
 * segments intersect, then
 *
 *      If r>1, I is located on extension of AB
 *      If r<0, I is located on extension of BA
 *      If s>1, I is located on extension of CD
 *      If s<0, I is located on extension of DC
 *
 * Also note that the denominators of eqn 1 & 2 are identical.
 * </pre>
 */
bool
LineLineIntersect (LineType *Line1, LineType *Line2)
{
  double s, r;
  double line1_dx, line1_dy, line2_dx, line2_dy,
         point1_dx, point1_dy;
  if (TEST_FLAG (SQUAREFLAG, Line1))/* pretty reckless recursion */
    {
      PointType p[4];
      form_slanted_rectangle (p, Line1);
      return IsLineInQuadrangle (p, Line2);
    }
  /* here come only round Line1 because IsLineInQuadrangle()
     calls LineLineIntersect() with first argument rounded*/
  if (TEST_FLAG (SQUAREFLAG, Line2))
    {
      PointType p[4];
      form_slanted_rectangle (p, Line2);
      return IsLineInQuadrangle (p, Line1);
    }
  /* now all lines are round */

  /* Check endpoints: this provides a quick exit, catches
   *  cases where the "real" lines don't intersect but the
   *  thick lines touch, and ensures that the dx/dy business
   *  below does not cause a divide-by-zero. */
  if (IsPointInPad (Line2->Point1.X, Line2->Point1.Y,
                    MAX (Line2->Thickness / 2 + Bloat, 0),
                    (PadType *) Line1)
       || IsPointInPad (Line2->Point2.X, Line2->Point2.Y,
                        MAX (Line2->Thickness / 2 + Bloat, 0),
                        (PadType *) Line1)
       || IsPointInPad (Line1->Point1.X, Line1->Point1.Y,
                        MAX (Line1->Thickness / 2 + Bloat, 0),
                        (PadType *) Line2)
       || IsPointInPad (Line1->Point2.X, Line1->Point2.Y,
                        MAX (Line1->Thickness / 2 + Bloat, 0),
                        (PadType *) Line2))
    return true;

  /* setup some constants */
  line1_dx = Line1->Point2.X - Line1->Point1.X;
  line1_dy = Line1->Point2.Y - Line1->Point1.Y;
  line2_dx = Line2->Point2.X - Line2->Point1.X;
  line2_dy = Line2->Point2.Y - Line2->Point1.Y;
  point1_dx = Line1->Point1.X - Line2->Point1.X;
  point1_dy = Line1->Point1.Y - Line2->Point1.Y;

  /* If either line is a point, we have failed already, since the
   *   endpoint check above will have caught an "intersection". */
  if ((line1_dx == 0 && line1_dy == 0)
      || (line2_dx == 0 && line2_dy == 0))
    return false;

  /* set s to cross product of Line1 and the line
   *   Line1.Point1--Line2.Point1 (as vectors) */
  s = point1_dy * line1_dx - point1_dx * line1_dy;

  /* set r to cross product of both lines (as vectors) */
  r = line1_dx * line2_dy - line1_dy * line2_dx;

  /* No cross product means parallel lines, or maybe Line2 is
   *  zero-length. In either case, since we did a bounding-box
   *  check before getting here, the above IsPointInPad() checks
   *  will have caught any intersections. */
  if (r == 0.0)
    return false;

  s /= r;
  r = (point1_dy * line2_dx - point1_dx * line2_dy) / r;

  /* intersection is at least on AB */
  if (r >= 0.0 && r <= 1.0)
    return (s >= 0.0 && s <= 1.0);

  /* intersection is at least on CD */
  /* [removed this case since it always returns false --asp] */
  return false;
}

/*!
 * \brief Check for line intersection with an arc.
 *
 * Mostly this is like the circle/line intersection
 * found in IsPointOnLine (search.c) see the detailed
 * discussion for the basics there.
 *
 * Since this is only an arc, not a full circle we need
 * to find the actual points of intersection with the
 * circle, and see if they are on the arc.
 *
 * To do this, we translate along the line from the point Q
 * plus or minus a distance delta = sqrt(Radius^2 - d^2)
 * but it's handy to normalize with respect to l, the line
 * length so a single projection is done (e.g. we don't first
 * find the point Q.
 *
 * <pre>
 * The projection is now of the form:
 *
 *      Px = X1 + (r +- r2)(X2 - X1)
 *      Py = Y1 + (r +- r2)(Y2 - Y1)
 * </pre>
 *
 * Where r2 sqrt(Radius^2 l^2 - d^2)/l^2
 * note that this is the variable d, not the symbol d described in
 * IsPointOnLine (variable d = symbol d * l).
 *
 * The end points are hell so they are checked individually.
 */
bool
LineArcIntersect (LineType *Line, ArcType *Arc)
{
  double dx, dy, dx1, dy1, l, d, r, r2, Radius;
  BoxType *box;

  dx = Line->Point2.X - Line->Point1.X;
  dy = Line->Point2.Y - Line->Point1.Y;
  dx1 = Line->Point1.X - Arc->X;
  dy1 = Line->Point1.Y - Arc->Y;
  l = dx * dx + dy * dy;
  d = dx * dy1 - dy * dx1;
  d *= d;

  /* use the larger diameter circle first */
  Radius =
    Arc->Width + MAX (0.5 * (Arc->Thickness + Line->Thickness) + Bloat, 0.0);
  Radius *= Radius;
  r2 = Radius * l - d;
  /* projection doesn't even intersect circle when r2 < 0 */
  if (r2 < 0)
    return (false);
  /* check the ends of the line in case the projected point */
  /* of intersection is beyond the line end */
  if (IsPointOnArc
      (Line->Point1.X, Line->Point1.Y,
       MAX (0.5 * Line->Thickness + Bloat, 0.0), Arc))
    return (true);
  if (IsPointOnArc
      (Line->Point2.X, Line->Point2.Y,
       MAX (0.5 * Line->Thickness + Bloat, 0.0), Arc))
    return (true);
  if (l == 0.0)
    return (false);
  r2 = sqrt (r2);
  Radius = -(dx * dx1 + dy * dy1);
  r = (Radius + r2) / l;
  if (r >= 0 && r <= 1
      && IsPointOnArc (Line->Point1.X + r * dx,
                       Line->Point1.Y + r * dy,
                       MAX (0.5 * Line->Thickness + Bloat, 0.0), Arc))
    return (true);
  r = (Radius - r2) / l;
  if (r >= 0 && r <= 1
      && IsPointOnArc (Line->Point1.X + r * dx,
                       Line->Point1.Y + r * dy,
                       MAX (0.5 * Line->Thickness + Bloat, 0.0), Arc))
    return (true);
  /* check arc end points */
  box = GetArcEnds (Arc);
  if (IsPointInPad (box->X1, box->Y1, Arc->Thickness * 0.5 + Bloat, (PadType *)Line))
    return true;
  if (IsPointInPad (box->X2, box->Y2, Arc->Thickness * 0.5 + Bloat, (PadType *)Line))
    return true;
  return false;
}

static int
LOCtoArcLine_callback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, line) && LineArcIntersect (line, i->arc))
    {
      if (ADD_LINE_TO_LIST (i->layer, line, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoArcArc_callback (const BoxType * b, void *cl)
{
  ArcType *arc = (ArcType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!arc->Thickness)
    return 0;
  if (!TEST_FLAG (i->flag, arc) && ArcArcIntersect (i->arc, arc))
    {
      if (ADD_ARC_TO_LIST (i->layer, arc, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoArcPad_callback (const BoxType * b, void *cl)
{
  PadType *pad = (PadType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, pad) && i->layer ==
      (TEST_FLAG (ONSOLDERFLAG, pad) ? BOTTOM_SIDE : TOP_SIDE)
      && ArcPadIntersect (i->arc, pad) && ADD_PAD_TO_LIST (i->layer, pad, i->flag))
    longjmp (i->env, 1);
  return 0;
}

/*!
 * \brief Searches all LOs that are connected to the given arc on the
 * given layergroup.
 *
 * All found connections are added to the list.
 *
 * The notation that is used is:\n
 * Xij means Xj at arc i.
 */
static bool
LookupLOConnectionsToArc (ArcType *Arc, Cardinal LayerGroup, int flag, bool AndRats)
{
  Cardinal entry;
  struct lo_info info;
  BoxType search_box;

  info.flag = flag;
  info.arc = Arc;
  search_box = expand_bounds ((BoxType *)info.arc);

  /* loop over all layers of the group */
  for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++)
    {
      Cardinal layer_no;
      LayerType *layer;
      GList *i;

      layer_no = PCB->LayerGroups.Entries[LayerGroup][entry];
      layer = LAYER_PTR (layer_no);

      /* handle normal layers */
      if (layer_no < max_copper_layer)
        {
          info.layer = layer_no;
          /* add arcs */
          if (setjmp (info.env) == 0)
            r_search (layer->line_tree, &search_box,
                      NULL, LOCtoArcLine_callback, &info);
          else
            return true;

          if (setjmp (info.env) == 0)
            r_search (layer->arc_tree, &search_box,
                      NULL, LOCtoArcArc_callback, &info);
          else
            return true;

          /* now check all polygons */
          for (i = layer->Polygon; i != NULL; i = g_list_next (i))
            {
              PolygonType *polygon = i->data;
              if (!TEST_FLAG (flag, polygon) && IsArcInPolygon (Arc, polygon)
                  && ADD_POLYGON_TO_LIST (layer_no, polygon, flag))
                return true;
            }
        }
      else
        {
          info.layer = layer_no - max_copper_layer;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pad_tree, &search_box, NULL,
                      LOCtoArcPad_callback, &info);
          else
            return true;
        }
    }
  return (false);
}

static int
LOCtoLineLine_callback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, line) && LineLineIntersect (i->line, line))
    {
      if (ADD_LINE_TO_LIST (i->layer, line, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoLineArc_callback (const BoxType * b, void *cl)
{
  ArcType *arc = (ArcType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!arc->Thickness)
    return 0;
  if (!TEST_FLAG (i->flag, arc) && LineArcIntersect (i->line, arc))
    {
      if (ADD_ARC_TO_LIST (i->layer, arc, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoLineRat_callback (const BoxType * b, void *cl)
{
  RatType *rat = (RatType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, rat))
    {
      if ((rat->group1 == i->layer)
          && IsRatPointOnLineEnd (&rat->Point1, i->line))
        {
          if (ADD_RAT_TO_LIST (rat, i->flag))
            longjmp (i->env, 1);
        }
      else if ((rat->group2 == i->layer)
               && IsRatPointOnLineEnd (&rat->Point2, i->line))
        {
          if (ADD_RAT_TO_LIST (rat, i->flag))
            longjmp (i->env, 1);
        }
    }
  return 0;
}

static int
LOCtoLinePad_callback (const BoxType * b, void *cl)
{
  PadType *pad = (PadType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, pad) && i->layer ==
      (TEST_FLAG (ONSOLDERFLAG, pad) ? BOTTOM_SIDE : TOP_SIDE)
      && LinePadIntersect (i->line, pad) && ADD_PAD_TO_LIST (i->layer, pad, i->flag))
    longjmp (i->env, 1);
  return 0;
}

/*!
 * \brief Searches all LOs that are connected to the given line on the
 * given layergroup.
 *
 * All found connections are added to the list.
 *
 * The notation that is used is:
 * Xij means Xj at line i.
 */
static bool
LookupLOConnectionsToLine (LineType *Line, Cardinal LayerGroup,
                           int flag, bool PolysTo, bool AndRats)
{
  Cardinal entry;
  struct lo_info info;
  BoxType search_box;

  info.flag = flag;
  info.layer = LayerGroup;
  info.line = Line;
  search_box = expand_bounds ((BoxType *)info.line);

  if (AndRats)
    {
      /* add the new rat lines */
      if (setjmp (info.env) == 0)
        r_search (PCB->Data->rat_tree, &search_box, NULL,
                  LOCtoLineRat_callback, &info);
      else
        return true;
    }

  /* loop over all layers of the group */
  for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++)
    {
      Cardinal layer_no;
      LayerType *layer;

      layer_no = PCB->LayerGroups.Entries[LayerGroup][entry];
      layer = LAYER_PTR (layer_no);

      /* handle normal layers */
      if (layer_no < max_copper_layer)
        {
          info.layer = layer_no;
          /* add lines */
          if (setjmp (info.env) == 0)
            r_search (layer->line_tree, &search_box,
                      NULL, LOCtoLineLine_callback, &info);
          else
            return true;
          /* add arcs */
          if (setjmp (info.env) == 0)
            r_search (layer->arc_tree, &search_box,
                      NULL, LOCtoLineArc_callback, &info);
          else
            return true;
          /* now check all polygons */
          if (PolysTo)
            {
              GList *i;
              for (i = layer->Polygon; i != NULL; i = g_list_next (i))
                {
                  PolygonType *polygon = i->data;
                  if (!TEST_FLAG (flag, polygon) && IsLineInPolygon (Line, polygon)
                      && ADD_POLYGON_TO_LIST (layer_no, polygon, flag))
                    return true;
                }
            }
        }
      else
        {
          /* handle special 'pad' layers */
          info.layer = layer_no - max_copper_layer;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pad_tree, &search_box, NULL,
                      LOCtoLinePad_callback, &info);
          else
            return true;
        }
    }
  return (false);
}

struct rat_info
{
  Cardinal layer;
  PointType *Point;
  int flag;
  jmp_buf env;
};

static int
LOCtoRat_callback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  struct rat_info *i = (struct rat_info *) cl;

  if (!TEST_FLAG (i->flag, line) &&
      ((line->Point1.X == i->Point->X &&
        line->Point1.Y == i->Point->Y) ||
       (line->Point2.X == i->Point->X && line->Point2.Y == i->Point->Y)))
    {
      if (ADD_LINE_TO_LIST (i->layer, line, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}
static int
PolygonToRat_callback (const BoxType * b, void *cl)
{
  PolygonType *polygon = (PolygonType *) b;
  struct rat_info *i = (struct rat_info *) cl;

  if (!TEST_FLAG (i->flag, polygon) && polygon->Clipped &&
      (i->Point->X == polygon->Clipped->contours->head.point[0]) &&
      (i->Point->Y == polygon->Clipped->contours->head.point[1]))
    {
      if (ADD_POLYGON_TO_LIST (i->layer, polygon, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPad_callback (const BoxType * b, void *cl)
{
  PadType *pad = (PadType *) b;
  struct rat_info *i = (struct rat_info *) cl;

  if (!TEST_FLAG (i->flag, pad) && i->layer ==
	(TEST_FLAG (ONSOLDERFLAG, pad) ? BOTTOM_SIDE : TOP_SIDE) &&
      ((pad->Point1.X == i->Point->X && pad->Point1.Y == i->Point->Y) ||
       (pad->Point2.X == i->Point->X && pad->Point2.Y == i->Point->Y) ||
       ((pad->Point1.X + pad->Point2.X) / 2 == i->Point->X &&
        (pad->Point1.Y + pad->Point2.Y) / 2 == i->Point->Y)) &&
      ADD_PAD_TO_LIST (i->layer, pad, i->flag))
    longjmp (i->env, 1);
  return 0;
}

/*!
 * \brief Searches all LOs that are connected to the given rat-line on
 * the given layergroup.
 *
 * All found connections are added to the list.
 *
 * The notation that is used is:
 * Xij means Xj at line i.
 */
static bool
LookupLOConnectionsToRatEnd (PointType *Point, Cardinal LayerGroup, int flag)
{
  Cardinal entry;
  struct rat_info info;

  info.flag = flag;
  info.Point = Point;
  /* loop over all layers of this group */
  for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++)
    {
      Cardinal layer_no;
      LayerType *layer;

      layer_no = PCB->LayerGroups.Entries[LayerGroup][entry];
      layer = LAYER_PTR (layer_no);
      /* handle normal layers 
         rats don't ever touch
         arcs by definition
       */

      if (layer_no < max_copper_layer)
        {
          info.layer = layer_no;
          if (setjmp (info.env) == 0)
            r_search_pt (layer->line_tree, Point, 1, NULL,
                      LOCtoRat_callback, &info);
          else
            return true;
          if (setjmp (info.env) == 0)
            r_search_pt (layer->polygon_tree, Point, 1,
                      NULL, PolygonToRat_callback, &info);
        }
      else
        {
          /* handle special 'pad' layers */
          info.layer = layer_no - max_copper_layer;
          if (setjmp (info.env) == 0)
            r_search_pt (PCB->Data->pad_tree, Point, 1, NULL,
                      LOCtoPad_callback, &info);
          else
            return true;
        }
    }
  return (false);
}

static int
LOCtoPadLine_callback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, line) && LinePadIntersect (line, i->pad))
    {
      if (ADD_LINE_TO_LIST (i->layer, line, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPadArc_callback (const BoxType * b, void *cl)
{
  ArcType *arc = (ArcType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!arc->Thickness)
    return 0;
  if (!TEST_FLAG (i->flag, arc) && ArcPadIntersect (arc, i->pad))
    {
      if (ADD_ARC_TO_LIST (i->layer, arc, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPadPoly_callback (const BoxType * b, void *cl)
{
  PolygonType *polygon = (PolygonType *) b;
  struct lo_info *i = (struct lo_info *) cl;


  if (!TEST_FLAG (i->flag, polygon) &&
      (!TEST_FLAG (CLEARPOLYFLAG, polygon) || !i->pad->Clearance))
    {
      if (IsPadInPolygon (i->pad, polygon) &&
          ADD_POLYGON_TO_LIST (i->layer, polygon, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPadRat_callback (const BoxType * b, void *cl)
{
  RatType *rat = (RatType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, rat))
    {
      if (rat->group1 == i->layer &&
	  ((rat->Point1.X == i->pad->Point1.X && rat->Point1.Y == i->pad->Point1.Y) ||
	   (rat->Point1.X == i->pad->Point2.X && rat->Point1.Y == i->pad->Point2.Y) ||
	   (rat->Point1.X == (i->pad->Point1.X + i->pad->Point2.X) / 2 &&
	    rat->Point1.Y == (i->pad->Point1.Y + i->pad->Point2.Y) / 2)))
        {
          if (ADD_RAT_TO_LIST (rat, i->flag))
            longjmp (i->env, 1);
        }
      else if (rat->group2 == i->layer &&
	       ((rat->Point2.X == i->pad->Point1.X && rat->Point2.Y == i->pad->Point1.Y) ||
		(rat->Point2.X == i->pad->Point2.X && rat->Point2.Y == i->pad->Point2.Y) ||
		(rat->Point2.X == (i->pad->Point1.X + i->pad->Point2.X) / 2 &&
		 rat->Point2.Y == (i->pad->Point1.Y + i->pad->Point2.Y) / 2)))
        {
          if (ADD_RAT_TO_LIST (rat, i->flag))
            longjmp (i->env, 1);
        }
    }
  return 0;
}

static int
LOCtoPadPad_callback (const BoxType * b, void *cl)
{
  PadType *pad = (PadType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, pad) && i->layer ==
      (TEST_FLAG (ONSOLDERFLAG, pad) ? BOTTOM_SIDE : TOP_SIDE)
      && PadPadIntersect (pad, i->pad) && ADD_PAD_TO_LIST (i->layer, pad, i->flag))
    longjmp (i->env, 1);
  return 0;
}

/*!
 * \brief Searches all LOs that are connected to the given pad on the
 * given layergroup.
 *
 * All found connections are added to the list.
 */
static bool
LookupLOConnectionsToPad (PadType *Pad, Cardinal LayerGroup, int flag, bool AndRats)
{
  Cardinal entry;
  struct lo_info info;
  BoxType search_box;

  if (!TEST_FLAG (SQUAREFLAG, Pad))
    return (LookupLOConnectionsToLine ((LineType *) Pad, LayerGroup, flag, false, AndRats));

  info.flag = flag;
  info.pad = Pad;
  search_box = expand_bounds ((BoxType *)info.pad);

  /* add the new rat lines */
  info.layer = LayerGroup;

  if (AndRats)
    {
      if (setjmp (info.env) == 0)
        r_search (PCB->Data->rat_tree, &search_box, NULL,
                  LOCtoPadRat_callback, &info);
      else
        return true;
    }

  /* loop over all layers of the group */
  for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++)
    {
      Cardinal layer_no;
      LayerType *layer;

      layer_no = PCB->LayerGroups.Entries[LayerGroup][entry];
      layer = LAYER_PTR (layer_no);
      /* handle normal layers */
      if (layer_no < max_copper_layer)
        {
          info.layer = layer_no;
          /* add lines */
          if (setjmp (info.env) == 0)
            r_search (layer->line_tree, &search_box,
                      NULL, LOCtoPadLine_callback, &info);
          else
            return true;
          /* add arcs */
          if (setjmp (info.env) == 0)
            r_search (layer->arc_tree, &search_box,
                      NULL, LOCtoPadArc_callback, &info);
          else
            return true;
          /* add polygons */
          if (setjmp (info.env) == 0)
            r_search (layer->polygon_tree, &search_box,
                      NULL, LOCtoPadPoly_callback, &info);
          else
            return true;
        }
      else
        {
          /* handle special 'pad' layers */
          info.layer = layer_no - max_copper_layer;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pad_tree, &search_box, NULL,
                      LOCtoPadPad_callback, &info);
          else
            return true;
        }

    }
  return (false);
}

static int
LOCtoPolyLine_callback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, line) && IsLineInPolygon (line, i->polygon))
    {
      if (ADD_LINE_TO_LIST (i->layer, line, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPolyArc_callback (const BoxType * b, void *cl)
{
  ArcType *arc = (ArcType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!arc->Thickness)
    return 0;
  if (!TEST_FLAG (i->flag, arc) && IsArcInPolygon (arc, i->polygon))
    {
      if (ADD_ARC_TO_LIST (i->layer, arc, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPolyPad_callback (const BoxType * b, void *cl)
{
  PadType *pad = (PadType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, pad) && i->layer ==
      (TEST_FLAG (ONSOLDERFLAG, pad) ? BOTTOM_SIDE : TOP_SIDE)
      && IsPadInPolygon (pad, i->polygon))
    {
      if (ADD_PAD_TO_LIST (i->layer, pad, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPolyRat_callback (const BoxType * b, void *cl)
{
  RatType *rat = (RatType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, rat))
    {
      if ((rat->Point1.X == (i->polygon->Clipped->contours->head.point[0]) &&
           rat->Point1.Y == (i->polygon->Clipped->contours->head.point[1]) &&
           rat->group1 == i->layer) ||
          (rat->Point2.X == (i->polygon->Clipped->contours->head.point[0]) &&
           rat->Point2.Y == (i->polygon->Clipped->contours->head.point[1]) &&
           rat->group2 == i->layer))
        if (ADD_RAT_TO_LIST (rat, i->flag))
          longjmp (i->env, 1);
    }
  return 0;
}


/*!
 * \brief Looks up LOs that are connected to the given polygon on the
 * given layergroup.
 *
 * All found connections are added to the list.
 */
static bool
LookupLOConnectionsToPolygon (PolygonType *Polygon, Cardinal LayerGroup, int flag, bool AndRats)
{
  Cardinal entry;
  struct lo_info info;
  BoxType search_box;

  if (!Polygon->Clipped)
    return false;

  info.flag = flag;
  info.polygon = Polygon;
  search_box = expand_bounds ((BoxType *)info.polygon);

  info.layer = LayerGroup;

  /* check rats */
  if (AndRats)
    {
      if (setjmp (info.env) == 0)
        r_search (PCB->Data->rat_tree, &search_box, NULL,
                  LOCtoPolyRat_callback, &info);
      else
        return true;
    }

/* loop over all layers of the group */
  for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++)
    {
      Cardinal layer_no;
      LayerType *layer;

      layer_no = PCB->LayerGroups.Entries[LayerGroup][entry];
      layer = LAYER_PTR (layer_no);

      /* handle normal layers */
      if (layer_no < max_copper_layer)
        {
          GList *i;

          /* check all polygons */
          for (i = layer->Polygon; i != NULL; i = g_list_next (i))
            {
              PolygonType *polygon = i->data;
              if (!TEST_FLAG (flag, polygon)
                  && IsPolygonInPolygon (polygon, Polygon)
                  && ADD_POLYGON_TO_LIST (layer_no, polygon, flag))
                return true;
            }

          info.layer = layer_no;
          /* check all lines */
          if (setjmp (info.env) == 0)
            r_search (layer->line_tree, &search_box,
                      NULL, LOCtoPolyLine_callback, &info);
          else
            return true;
          /* check all arcs */
          if (setjmp (info.env) == 0)
            r_search (layer->arc_tree, &search_box,
                      NULL, LOCtoPolyArc_callback, &info);
          else
            return true;
        }
      else
        {
          info.layer = layer_no - max_copper_layer;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pad_tree, &search_box,
                      NULL, LOCtoPolyPad_callback, &info);
          else
            return true;
        }
    }
  return (false);
}

/*!
 * \brief Checks if an arc has a connection to a polygon.
 *
 * - first check if the arc can intersect with the polygon by
 *   evaluating the bounding boxes.
 * - check the two end points of the arc. If none of them matches
 * - check all segments of the polygon against the arc.
 */
/*static*/ bool
IsArcInPolygon (ArcType *Arc, PolygonType *Polygon)
{
  BoxType *Box = (BoxType *) Arc;

  /* arcs with clearance never touch polys */
  if (TEST_FLAG (CLEARPOLYFLAG, Polygon) && TEST_FLAG (CLEARLINEFLAG, Arc))
    return false;
  if (!Polygon->Clipped)
    return false;
  if (Box->X1 <= Polygon->Clipped->contours->xmax + Bloat
      && Box->X2 >= Polygon->Clipped->contours->xmin - Bloat
      && Box->Y1 <= Polygon->Clipped->contours->ymax + Bloat
      && Box->Y2 >= Polygon->Clipped->contours->ymin - Bloat)
    {
      POLYAREA *ap;

      if (!(ap = ArcPoly (Arc, Arc->Thickness + Bloat)))
        return false;           /* error */
      return isects (ap, Polygon, true);
    }
  return false;
}

/*!
 * \brief Checks if a line has a connection to a polygon.
 *
 * - first check if the line can intersect with the polygon by
 *   evaluating the bounding boxes
 * - check the two end points of the line. If none of them matches
 * - check all segments of the polygon against the line.
 */
/*static*/ bool
IsLineInPolygon (LineType *Line, PolygonType *Polygon)
{
  BoxType *Box = (BoxType *) Line;
  POLYAREA *lp;

  /* lines with clearance never touch polygons */
  if (TEST_FLAG (CLEARPOLYFLAG, Polygon) && TEST_FLAG (CLEARLINEFLAG, Line))
    return false;
  if (!Polygon->Clipped)
    return false;
  if (TEST_FLAG(SQUAREFLAG,Line)&&(Line->Point1.X==Line->Point2.X||Line->Point1.Y==Line->Point2.Y))
     {
       Coord wid = (Line->Thickness + Bloat + 1) / 2;
       Coord x1, x2, y1, y2;

       x1 = MIN (Line->Point1.X, Line->Point2.X) - wid;
       y1 = MIN (Line->Point1.Y, Line->Point2.Y) - wid;
       x2 = MAX (Line->Point1.X, Line->Point2.X) + wid;
       y2 = MAX (Line->Point1.Y, Line->Point2.Y) + wid;
       return IsRectangleInPolygon (x1, y1, x2, y2, Polygon);
     }
  if (Box->X1 <= Polygon->Clipped->contours->xmax + Bloat
      && Box->X2 >= Polygon->Clipped->contours->xmin - Bloat
      && Box->Y1 <= Polygon->Clipped->contours->ymax + Bloat
      && Box->Y2 >= Polygon->Clipped->contours->ymin - Bloat)
    {
      if (!(lp = LinePoly (Line, Line->Thickness + Bloat)))
        return FALSE;           /* error */
      return isects (lp, Polygon, true);
    }
  return false;
}

/*!
 * \brief Checks if a pad connects to a non-clearing polygon.
 *
 * The polygon is assumed to already have been proven non-clearing.
 */
/*static*/ bool
IsPadInPolygon (PadType *pad, PolygonType *polygon)
{
    return IsLineInPolygon ((LineType *) pad, polygon);
}

/*!
 * \brief Checks if a polygon has a connection to a second one.
 *
 * First check all points out of P1 against P2 and vice versa.
 * If both fail check all lines of P1 against the ones of P2.
 */
/*static*/ bool
IsPolygonInPolygon (PolygonType *P1, PolygonType *P2)
{
  if (!P1->Clipped || !P2->Clipped)
    return false;
  assert (P1->Clipped->contours);
  assert (P2->Clipped->contours);

  /* first check if both bounding boxes intersect. If not, return quickly */
  if (P1->Clipped->contours->xmin - Bloat > P2->Clipped->contours->xmax ||
      P1->Clipped->contours->xmax + Bloat < P2->Clipped->contours->xmin ||
      P1->Clipped->contours->ymin - Bloat > P2->Clipped->contours->ymax ||
      P1->Clipped->contours->ymax + Bloat < P2->Clipped->contours->ymin)
    return false;

  /* first check un-bloated case */
  if (isects (P1->Clipped, P2, false))
    return TRUE;

  /* now the difficult case of bloated */
  if (Bloat > 0)
    {
      PLINE *c;
      for (c = P1->Clipped->contours; c; c = c->next)
        {
          LineType line;
          VNODE *v = &c->head;
          if (c->xmin - Bloat <= P2->Clipped->contours->xmax &&
              c->xmax + Bloat >= P2->Clipped->contours->xmin &&
              c->ymin - Bloat <= P2->Clipped->contours->ymax &&
              c->ymax + Bloat >= P2->Clipped->contours->ymin)
            {

              line.Point1.X = v->point[0];
              line.Point1.Y = v->point[1];
              line.Thickness = Bloat;
              /* Another Bloat is added by IsLineInPolygon, making the correct
               * 2x Bloat. Perhaps we should change it there, but doing so
               * breaks some other DRC checks which rely on the behaviour
               * in IsLineInPolygon.
               */
              line.Clearance = 0;
              line.Flags = NoFlags ();
              for (v = v->next; v != &c->head; v = v->next)
                {
                  line.Point2.X = v->point[0];
                  line.Point2.Y = v->point[1];
                  SetLineBoundingBox (&line);
                  if (IsLineInPolygon (&line, P2))
                    return (true);
                  line.Point1.X = line.Point2.X;
                  line.Point1.Y = line.Point2.Y;
                }
            }
        }
    }

  return (false);
}

/*!
 * \brief Writes the several names of an element to a file.
 */
static void
PrintElementNameList (ElementType *Element, FILE * FP)
{
  static DynamicStringType cname, pname, vname;

  CreateQuotedString (&cname, (char *)EMPTY (DESCRIPTION_NAME (Element)));
  CreateQuotedString (&pname, (char *)EMPTY (NAMEONPCB_NAME (Element)));
  CreateQuotedString (&vname, (char *)EMPTY (VALUE_NAME (Element)));
  fprintf (FP, "(%s %s %s)\n", cname.Data, pname.Data, vname.Data);
}

/*!
 * \brief Writes the several names of an element to a file.
 */
static void
PrintConnectionElementName (ElementType *Element, FILE * FP)
{
  fputs ("Element", FP);
  PrintElementNameList (Element, FP);
  fputs ("{\n", FP);
}

/*!
 * \brief Prints one {pin,pad,via}/element entry of connection lists.
 */
static void
PrintConnectionListEntry (char *ObjName, ElementType *Element,
                          bool FirstOne, FILE * FP)
{
  static DynamicStringType oname;

  CreateQuotedString (&oname, ObjName);
  if (FirstOne)
    fprintf (FP, "\t%s\n\t{\n", oname.Data);
  else
    {
      fprintf (FP, "\t\t%s ", oname.Data);
      if (Element)
        PrintElementNameList (Element, FP);
      else
        fputs ("(__VIA__)\n", FP);
    }
}

/*!
 * \brief Prints all found connections of a pads to file FP
 * the connections are stacked in 'PadList'.
 */
static void
PrintPadConnections (Cardinal Layer, FILE * FP, bool IsFirst)
{
  Cardinal i;
  PadType *ptr;

  if (!PadList[Layer].Number)
    return;

  /* the starting pad */
  if (IsFirst)
    {
      ptr = PADLIST_ENTRY (Layer, 0);
      if (ptr != NULL)
        PrintConnectionListEntry ((char *)UNKNOWN (ptr->Name), NULL, true, FP);
      else
        printf ("Skipping NULL ptr in 1st part of PrintPadConnections\n");
    }

  /* we maybe have to start with i=1 if we are handling the
   * starting-pad itself
   */
  for (i = IsFirst ? 1 : 0; i < PadList[Layer].Number; i++)
    {
      ptr = PADLIST_ENTRY (Layer, i);
      if (ptr != NULL)
        PrintConnectionListEntry ((char *)EMPTY (ptr->Name), (ElementType *)ptr->Element, false, FP);
      else
        printf ("Skipping NULL ptr in 2nd part of PrintPadConnections\n");
    }
}

/*!
 * \brief Prints all found connections of a pin to file FP
 * the connections are stacked in 'PVList'.
 */
static void
PrintPinConnections (FILE * FP, bool IsFirst)
{
  Cardinal i;
  PinType *pv;

  if (!PVList.Number)
    return;

  if (IsFirst)
    {
      /* the starting pin */
      pv = PVLIST_ENTRY (0);
      PrintConnectionListEntry ((char *)EMPTY (pv->Name), NULL, true, FP);
    }

  /* we maybe have to start with i=1 if we are handling the
   * starting-pin itself
   */
  for (i = IsFirst ? 1 : 0; i < PVList.Number; i++)
    {
      /* get the elements name or assume that its a via */
      pv = PVLIST_ENTRY (i);
      PrintConnectionListEntry ((char *)EMPTY (pv->Name), (ElementType *)pv->Element, false, FP);
    }
}

/*!
 * \brief Checks if all lists of new objects are handled.
 */
static bool
ListsEmpty (bool AndRats)
{
  bool empty;
  int i;

  empty = (PVList.Location >= PVList.Number);
  if (AndRats)
    empty = empty && (RatList.Location >= RatList.Number);
  for (i = 0; i < max_copper_layer && empty; i++)
    if (!LAYER_PTR (i)->no_drc)
      empty = empty && LineList[i].Location >= LineList[i].Number
        && ArcList[i].Location >= ArcList[i].Number
        && PolygonList[i].Location >= PolygonList[i].Number;
  return (empty);
}

static void
reassign_no_drc_flags (void)
{
  int layer;

  for (layer = 0; layer < max_copper_layer; layer++)
    {
      LayerType *l = LAYER_PTR (layer);
      l->no_drc = AttributeGet (l, "PCB::skip-drc") != NULL;
    }
}

/*!
 * \brief Loops till no more connections are found.
 */
/*static*/ bool
DoIt (int flag, Coord bloat, bool AndRats, bool AndDraw, bool is_drc)
{
  bool newone = false;
  Bloat = bloat;
  drc = is_drc;
  reassign_no_drc_flags ();
  do
    {
      /* lookup connections; these are the steps (2) to (4)
       * from the description
       *
       * If anything is added to any of the lists, newone will be true. Any
       * new additions to the lists mean that there are potentially more things
       * to add to the list, things that might overlap with only the new
       * objects.
	   */
      newone = LookupPVConnectionsToPVList (flag) ||
               LookupLOConnectionsToPVList (flag, AndRats) ||
               LookupLOConnectionsToLOList (flag, AndRats) ||
               LookupPVConnectionsToLOList (flag, AndRats);
      if (AndDraw)
        DrawNewConnections ();
    }
  /* Keep executing the lookup until no new objects are found. */
  while (!newone && !ListsEmpty (AndRats));
  if (AndDraw)
    Draw ();
  return (newone);
}

/*!
 * \brief Prints all unused pins of an element to file FP.
 */
static bool
PrintAndSelectUnusedPinsAndPadsOfElement (ElementType *Element, FILE * FP, int flag)
{
  bool first = true;
  Cardinal number;
  static DynamicStringType oname;

  /* check all pins in element */

  PIN_LOOP (Element);
  {
    if (!TEST_FLAG (HOLEFLAG, pin))
      {
        /* pin might have bee checked before, add to list if not */
        if (!TEST_FLAG (flag, pin) && FP)
          {
            int i;
            if (ADD_PV_TO_LIST (pin, flag))
              return true;
            DoIt (flag, 0, true, true, false);
            number = PadList[TOP_SIDE].Number
              + PadList[BOTTOM_SIDE].Number + PVList.Number;
            /* the pin has no connection if it's the only
             * list entry; don't count vias
             */
            for (i = 0; i < PVList.Number; i++)
              if (!PVLIST_ENTRY (i)->Element)
                number--;
            if (number == 1)
              {
                /* output of element name if not already done */
                if (first)
                  {
                    PrintConnectionElementName (Element, FP);
                    first = false;
                  }

                /* write name to list and draw selected object */
                CreateQuotedString (&oname, (char *)EMPTY (pin->Name));
                fprintf (FP, "\t%s\n", oname.Data);
                SET_FLAG (SELECTEDFLAG, pin);
                DrawPin (pin);
              }

            /* reset found objects for the next pin */
            if (PrepareNextLoop (FP))
              return (true);
          }
      }
  }
  END_LOOP;

  /* check all pads in element */
  PAD_LOOP (Element);
  {
    /* lookup pad in list */
    /* pad might has bee checked before, add to list if not */
    if (!TEST_FLAG (flag, pad) && FP)
      {
        int i;
        if (ADD_PAD_TO_LIST (TEST_FLAG (ONSOLDERFLAG, pad)
                             ? BOTTOM_SIDE : TOP_SIDE, pad, flag))
          return true;
        DoIt (flag, 0, true, true, false);
        number = PadList[TOP_SIDE].Number
          + PadList[BOTTOM_SIDE].Number + PVList.Number;
        /* the pin has no connection if it's the only
         * list entry; don't count vias
         */
        for (i = 0; i < PVList.Number; i++)
          if (!PVLIST_ENTRY (i)->Element)
            number--;
        if (number == 1)
          {
            /* output of element name if not already done */
            if (first)
              {
                PrintConnectionElementName (Element, FP);
                first = false;
              }

            /* write name to list and draw selected object */
            CreateQuotedString (&oname, (char *)EMPTY (pad->Name));
            fprintf (FP, "\t%s\n", oname.Data);
            SET_FLAG (SELECTEDFLAG, pad);
            DrawPad (pad);
          }

        /* reset found objects for the next pin */
        if (PrepareNextLoop (FP))
          return (true);
      }
  }
  END_LOOP;

  /* print separator if element has unused pins or pads */
  if (!first)
    {
      fputs ("}\n\n", FP);
      SEPARATE (FP);
    }
  return (false);
}

/*!
 * \brief Resets some flags for looking up the next pin/pad.
 */
static bool
PrepareNextLoop (FILE * FP)
{
  Cardinal layer;

  /* reset found LOs for the next pin */
  for (layer = 0; layer < max_copper_layer; layer++)
    {
      LineList[layer].Location = LineList[layer].Number = 0;
      ArcList[layer].Location = ArcList[layer].Number = 0;
      PolygonList[layer].Location = PolygonList[layer].Number = 0;
    }

  /* reset found pads */
  for (layer = 0; layer < 2; layer++)
    PadList[layer].Location = PadList[layer].Number = 0;

  /* reset PVs */
  PVList.Number = PVList.Location = 0;
  RatList.Number = RatList.Location = 0;

  return (false);
}

/*!
 * \brief Finds all connections to the pins of the passed element.
 *
 * The result is written to file FP.
 *
 * \return true if operation was aborted.
 */
static bool
PrintElementConnections (ElementType *Element, FILE * FP, int flag, bool AndDraw)
{
  PrintConnectionElementName (Element, FP);

  /* check all pins in element */
  PIN_LOOP (Element);
  {
    /* pin might have been checked before, add to list if not */
    if (TEST_FLAG (flag, pin))
      {
        PrintConnectionListEntry ((char *)EMPTY (pin->Name), NULL, true, FP);
        fputs ("\t\t__CHECKED_BEFORE__\n\t}\n", FP);
        continue;
      }
    if (ADD_PV_TO_LIST (pin, flag))
      return true;
    DoIt (flag, 0, true, AndDraw, false);
    /* printout all found connections */
    PrintPinConnections (FP, true);
    PrintPadConnections (TOP_SIDE, FP, false);
    PrintPadConnections (BOTTOM_SIDE, FP, false);
    fputs ("\t}\n", FP);
    if (PrepareNextLoop (FP))
      return (true);
  }
  END_LOOP;

  /* check all pads in element */
  PAD_LOOP (Element);
  {
    Cardinal layer;
    /* pad might have been checked before, add to list if not */
    if (TEST_FLAG (flag, pad))
      {
        PrintConnectionListEntry ((char *)EMPTY (pad->Name), NULL, true, FP);
        fputs ("\t\t__CHECKED_BEFORE__\n\t}\n", FP);
        continue;
      }
    layer = TEST_FLAG (ONSOLDERFLAG, pad) ? BOTTOM_SIDE : TOP_SIDE;
    if (ADD_PAD_TO_LIST (layer, pad, flag))
      return true;
    DoIt (flag, 0, true, AndDraw, false);
    /* print all found connections */
    PrintPadConnections (layer, FP, true);
    PrintPadConnections (layer ==
                         (TOP_SIDE ? BOTTOM_SIDE : TOP_SIDE),
                         FP, false);
    PrintPinConnections (FP, false);
    fputs ("\t}\n", FP);
    if (PrepareNextLoop (FP))
      return (true);
  }
  END_LOOP;
  fputs ("}\n\n", FP);
  return (false);
}

/*!
 * \brief Draws all new connections which have been found since the
 * routine was called the last time.
 */
static void
DrawNewConnections (void)
{
  int i;
  Cardinal position;

  /* decrement 'i' to keep layerstack order */
  for (i = max_copper_layer - 1; i != -1; i--)
    {
      Cardinal layer = LayerStack[i];

      if (PCB->Data->Layer[layer].On)
        {
          /* draw all new lines */
          position = LineList[layer].DrawLocation;
          for (; position < LineList[layer].Number; position++)
            DrawLine (LAYER_PTR (layer), LINELIST_ENTRY (layer, position));
          LineList[layer].DrawLocation = LineList[layer].Number;

          /* draw all new arcs */
          position = ArcList[layer].DrawLocation;
          for (; position < ArcList[layer].Number; position++)
            DrawArc (LAYER_PTR (layer), ARCLIST_ENTRY (layer, position));
          ArcList[layer].DrawLocation = ArcList[layer].Number;

          /* draw all new polygons */
          position = PolygonList[layer].DrawLocation;
          for (; position < PolygonList[layer].Number; position++)
            DrawPolygon (LAYER_PTR (layer), POLYGONLIST_ENTRY (layer, position));
          PolygonList[layer].DrawLocation = PolygonList[layer].Number;
        }
    }

  /* draw all new pads */
  if (PCB->PinOn)
    for (i = 0; i < 2; i++)
      {
        position = PadList[i].DrawLocation;

        for (; position < PadList[i].Number; position++)
          DrawPad (PADLIST_ENTRY (i, position));
        PadList[i].DrawLocation = PadList[i].Number;
      }

  /* draw all new PVs; 'PVList' holds a list of pointers to the
   * sorted array pointers to PV data
   */
  while (PVList.DrawLocation < PVList.Number)
    {
      PinType *pv = PVLIST_ENTRY (PVList.DrawLocation);

      if (TEST_FLAG (PINFLAG, pv))
        {
          if (PCB->PinOn)
            DrawPin (pv);
        }
      else if (PCB->ViaOn)
        DrawVia (pv);
      PVList.DrawLocation++;
    }
  /* draw the new rat-lines */
  if (PCB->RatOn)
    {
      position = RatList.DrawLocation;
      for (; position < RatList.Number; position++)
        DrawRat (RATLIST_ENTRY (position));
      RatList.DrawLocation = RatList.Number;
    }
}

/*!
 * \brief Find all connections to pins within one element.
 */
void
LookupElementConnections (ElementType *Element, FILE * FP)
{
  /* reset all currently marked connections */
  ClearFlagOnAllObjects (FOUNDFLAG, true);
  InitConnectionLookup ();
  PrintElementConnections (Element, FP, FOUNDFLAG, true);
  SetChangedFlag (true);
  if (Settings.RingBellWhenFinished)
    gui->beep ();
  FreeConnectionLookupMemory ();
  IncrementUndoSerialNumber ();
  Draw ();
}

/*!
 * \brief Find all connections to pins of all element.
 *
 * This is called by an action to write out a list of all of the pins and
 * pads connected to each pin/pad of every element on the board. Kind of
 * an element oriented netlist.
 *
 * Since this runs to completion without any user interaction, and should
 * leave the database in the same state it was found, there's no reason to
 * add all of the overhead of making everything undo-able.
 */
void
LookupConnectionsToAllElements (FILE * FP)
{
  LockUndo();
  /* reset all currently marked connections */
  ClearFlagOnAllObjects (FOUNDFLAG, false);
  
  InitConnectionLookup ();

  ELEMENT_LOOP (PCB->Data);
  {
    /* break if abort dialog returned true */
    if (PrintElementConnections (element, FP, FOUNDFLAG, false))
      break;
    SEPARATE (FP);
    if (Settings.ResetAfterElement && n != 1)
      ClearFlagOnAllObjects (FOUNDFLAG, false);
  }
  END_LOOP;
  if (Settings.RingBellWhenFinished)
    gui->beep ();
  ClearFlagOnAllObjects (FOUNDFLAG, false);
  UnlockUndo();
  FreeConnectionLookupMemory ();
  Redraw ();
}

/*!
 * \brief Add the starting object to the list of found objects.
 */
/*static*/ bool
ListStart (int type, void *ptr1, void *ptr2, void *ptr3, int flag)
{
  DumpList ();
  switch (type)
    {
    case PIN_TYPE:
    case VIA_TYPE:
      {
        if (ADD_PV_TO_LIST ((PinType *) ptr2, flag))
          return true;
        break;
      }

    case RATLINE_TYPE:
      {
        if (ADD_RAT_TO_LIST ((RatType *) ptr1, flag))
          return true;
        break;
      }

    case LINE_TYPE:
      {
        int layer = GetLayerNumber (PCB->Data,
                                    (LayerType *) ptr1);

        if (ADD_LINE_TO_LIST (layer, (LineType *) ptr2, flag))
          return true;
        break;
      }

    case ARC_TYPE:
      {
        int layer = GetLayerNumber (PCB->Data,
                                    (LayerType *) ptr1);

        if (ADD_ARC_TO_LIST (layer, (ArcType *) ptr2, flag))
          return true;
        break;
      }

    case POLYGON_TYPE:
      {
        int layer = GetLayerNumber (PCB->Data,
                                    (LayerType *) ptr1);

        if (ADD_POLYGON_TO_LIST (layer, (PolygonType *) ptr2, flag))
          return true;
        break;
      }

    case PAD_TYPE:
      {
        PadType *pad = (PadType *) ptr2;
        if (ADD_PAD_TO_LIST
            (TEST_FLAG
             (ONSOLDERFLAG, pad) ? BOTTOM_SIDE : TOP_SIDE, pad, flag))
          return true;
        break;
      }
    }
  return (false);
}


/*!
 * \brief Set the specified flag on all objects that touch the object at
 * the given coordinates.
 *
 * The objects are re-drawn if AndDraw is true.
 */
void
LookupConnection (Coord X, Coord Y, bool AndDraw, Coord Range, int flag,
                  bool AndRats)
{
  void *ptr1, *ptr2, *ptr3;
  char *name;
  int type;

  /* check if there are any pins or pads at that position */

	reassign_no_drc_flags ();

  /* LOOKUP_FIRST = PIN_TYPE | PAD_TYPE */
  /* LOOKUP_MORE = Vias, lines, ratlines, polygons, arcs */
  /* SILK_TYPE = lines, arcs, polygons */
  type
    = SearchObjectByLocation (LOOKUP_FIRST, &ptr1, &ptr2, &ptr3, X, Y, Range);
  if (type == NO_TYPE)
    {
      type = SearchObjectByLocation (
        LOOKUP_MORE & ~(AndRats ? 0 : RATLINE_TYPE),
        &ptr1, &ptr2, &ptr3, X, Y, Range);
      if (type == NO_TYPE)
        return;
      if (type & SILK_TYPE)
        {
          int laynum = GetLayerNumber (PCB->Data,
                                       (LayerType *) ptr1);

          /* don't mess with non-conducting objects! */
          if (laynum >= max_copper_layer || ((LayerType *)ptr1)->no_drc)
            return;
        }
    }

  /* If the netlist window is open, this will highlight the entry
   * corresponding to net of the pin or pad we just found. It does not open
   * the window it is closed.
   */
  name = ConnectionName (type, ptr1, ptr2);
  hid_actionl ("NetlistShow", name, NULL);

  InitConnectionLookup ();

  /* now add the object to the appropriate list and start scanning
   * This is step (1) from the description
   */
  ListStart (type, ptr1, ptr2, ptr3, flag);
  DoIt (flag, 0, AndRats, AndDraw, false);

  /* we are done */
  if (AndDraw)
    Draw ();
  if (AndDraw && Settings.RingBellWhenFinished)
    gui->beep ();
  FreeConnectionLookupMemory ();
}

void 
LookupConnectionByPin (int type, void *ptr1)
{
/*  int TheFlag = FOUNDFLAG; */

  LockUndo();
  InitConnectionLookup ();
  ListStart (type, NULL, ptr1, NULL, FOUNDFLAG);
  DoIt (FOUNDFLAG, 0, true, false, false);
  FreeConnectionLookupMemory ();
  UnlockUndo();
}

/*!
 * \brief Find connections for rats nesting.
 *
 * Assumes InitConnectionLookup() has already been done.
 */
void
RatFindHook (int type, void *ptr1, void *ptr2, void *ptr3,
             bool undo, int flag, bool AndRats)
{
  if(!undo) LockUndo();
  DumpList ();
  ListStart (type, ptr1, ptr2, ptr3, flag);
  DoIt (flag, 0, AndRats, false, false);
  /* This is potentially problematic if there is a higher level function
   * that has locked the undo system. */
  UnlockUndo();
}

/*!
 * \brief Find all unused pins of all elements.
 */
void
LookupUnusedPins (FILE * FP)
{
  /* reset all currently marked connections */
  ClearFlagOnAllObjects (FOUNDFLAG, true);
  InitConnectionLookup ();

  ELEMENT_LOOP (PCB->Data);
  {
    /* break if abort dialog returned true;
     * passing NULL as filedescriptor discards the normal output
     */
    if (PrintAndSelectUnusedPinsAndPadsOfElement (element, FP, FOUNDFLAG))
      break;
  }
  END_LOOP;

  if (Settings.RingBellWhenFinished)
    gui->beep ();
  FreeConnectionLookupMemory ();
  IncrementUndoSerialNumber ();
  Draw ();
}

/*!
 * \brief Dumps the list contents.
 */
/* static */ void
DumpList (void)
{
  Cardinal i;

  for (i = 0; i < 2; i++)
    {
      PadList[i].Number = 0;
      PadList[i].Location = 0;
      PadList[i].DrawLocation = 0;
    }

  PVList.Number = 0;
  PVList.Location = 0;

  for (i = 0; i < max_copper_layer; i++)
    {
      LineList[i].Location = 0;
      LineList[i].DrawLocation = 0;
      LineList[i].Number = 0;
      ArcList[i].Location = 0;
      ArcList[i].DrawLocation = 0;
      ArcList[i].Number = 0;
      PolygonList[i].Location = 0;
      PolygonList[i].DrawLocation = 0;
      PolygonList[i].Number = 0;
    }
  RatList.Number = 0;
  RatList.Location = 0;
  RatList.DrawLocation = 0;
}


/* static */ void
start_do_it_and_dump (int type, void *ptr1, void *ptr2, void *ptr3,
                      int flag, bool AndDraw,
                      Coord bloat, bool is_drc)
{
  ListStart (type, ptr1, ptr2, ptr3, flag);
  DoIt (flag, bloat, true, AndDraw, is_drc);
  DumpList ();
}

void
InitConnectionLookup (void)
{
  InitComponentLookup ();
  InitLayoutLookup ();
}

void
FreeConnectionLookupMemory (void)
{
  FreeComponentLookupMemory ();
  FreeLayoutLookupMemory ();
}
