/* $Id$ */

/*
 *
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */


/*
 * short description:
 * - lists for pins and vias, lines, arcs, pads and for polygons are created.
 *   Every object that has to be checked is added to its list.
 *   Coarse searching is accomplished with the data rtrees.
 * - there's no 'speed-up' mechanism for polygons because they are not used
 *   as often as other objects 
 * - the maximum distance between line and pin ... would depend on the angle
 *   between them. To speed up computation the limit is set to one half
 *   of the thickness of the objects (cause of square pins).
 *
 * PV:  means pin or via (objects that connect layers)
 * LO:  all non PV objects (layer objects like lines, arcs, polygons, pads)
 *
 * 1. first, the LO or PV at the given coordinates is looked up
 * 2. all LO connections to that PV are looked up next
 * 3. lookup of all LOs connected to LOs from (2).
 *    This step is repeated until no more new connections are found.
 * 4. lookup all PVs connected to the LOs from (2) and (3)
 * 5. start again with (1) for all new PVs from (4)
 *
 * Intersection of line <--> circle:
 * - calculate the signed distance from the line to the center,
 *   return false if abs(distance) > R
 * - get the distance from the line <--> distancevector intersection to
 *   (X1,Y1) in range [0,1], return true if 0 <= distance <= 1
 * - depending on (r > 1.0 or r < 0.0) check the distance of X2,Y2 or X1,Y1
 *   to X,Y
 *
 * Intersection of line <--> line:
 * - see the description of 'LineLineIntersect()'
 */

/* routines to find connections between pins, vias, lines...
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <math.h>
#include <setjmp.h>
#include <assert.h>

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif

#include "global.h"

#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "find.h"
#include "mymem.h"
#include "misc.h"
#include "rtree.h"
#include "polygon.h"
#include "search.h"
#include "set.h"
#include "undo.h"
#include "rats.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#undef DEBUG

RCSID ("$Id$");




/* ---------------------------------------------------------------------------
 * some local macros
 */

#define EXPAND_BOUNDS(p) if (Bloat > 0) {\
       (p)->BoundingBox.X1 -= Bloat; \
       (p)->BoundingBox.X2 += Bloat; \
       (p)->BoundingBox.Y1 -= Bloat; \
       (p)->BoundingBox.Y2 += Bloat;}

#define	SEPARATE(FP)							\
	{											\
		int	i;									\
		fputc('#', (FP));						\
		for (i = Settings.CharPerLine; i; i--)	\
			fputc('=', (FP));					\
		fputc('\n', (FP));						\
	}

#define	PADLIST_ENTRY(L,I)	\
	(((PadTypePtr *)PadList[(L)].Data)[(I)])

#define	LINELIST_ENTRY(L,I)	\
	(((LineTypePtr *)LineList[(L)].Data)[(I)])

#define	ARCLIST_ENTRY(L,I)	\
	(((ArcTypePtr *)ArcList[(L)].Data)[(I)])

#define RATLIST_ENTRY(I)	\
	(((RatTypePtr *)RatList.Data)[(I)])

#define	POLYGONLIST_ENTRY(L,I)	\
	(((PolygonTypePtr *)PolygonList[(L)].Data)[(I)])

#define	PVLIST_ENTRY(I)	\
	(((PinTypePtr *)PVList.Data)[(I)])

#define IS_PV_ON_RAT(PV, Rat) \
	(IsPointOnLineEnd((PV)->X,(PV)->Y, (Rat)))

#define IS_PV_ON_ARC(PV, Arc)	\
	(TEST_FLAG(SQUAREFLAG, (PV)) ? \
		IsArcInRectangle( \
			(PV)->X -MAX(((PV)->Thickness+1)/2 +Bloat,0), (PV)->Y -MAX(((PV)->Thickness+1)/2 +Bloat,0), \
			(PV)->X +MAX(((PV)->Thickness+1)/2 +Bloat,0), (PV)->Y +MAX(((PV)->Thickness+1)/2 +Bloat,0), \
			(Arc)) : \
		IsPointOnArc((PV)->X,(PV)->Y,MAX((PV)->Thickness/2.0 + fBloat,0.0), (Arc)))

#define	IS_PV_ON_PAD(PV,Pad) \
	( IsPointInPad((PV)->X, (PV)->Y, MAX((PV)->Thickness/2 +Bloat,0), (Pad)))

/*
 * message when asked about continuing DRC checks after first 
 * violation is found.
 */
#define DRC_CONTINUE _("Press Next to continue DRC checking")
#define DRC_NEXT _("Next")
#define DRC_CANCEL _("Cancel")

/* ---------------------------------------------------------------------------
 * some local types
 *
 * the two 'dummy' structs for PVs and Pads are necessary for creating
 * connection lists which include the element's name
 */
typedef struct
{
  void **Data;                  /* pointer to index data */
  Cardinal Location,            /* currently used position */
    DrawLocation, Number,       /* number of objects in list */
    Size;
}
ListType, *ListTypePtr;

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static float fBloat = 0.0;
static LocationType Bloat = 0;
static int TheFlag = FOUNDFLAG;
static int OldFlag = FOUNDFLAG;
static void *thing_ptr1, *thing_ptr2, *thing_ptr3;
static int thing_type;
static Boolean User = False;    /* user action causing this */
static Boolean drc = False;     /* whether to stop if finding something not found */
static Boolean IsBad = False;
static Cardinal drcerr_count;   /* count of drc errors */
static Cardinal TotalP, TotalV, NumberOfPads[2];
static ListType LineList[MAX_LAYER],    /* list of objects to */
  PolygonList[MAX_LAYER], ArcList[MAX_LAYER], PadList[2], RatList, PVList;

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static Boolean LookupLOConnectionsToPVList (Boolean);
static Boolean LookupLOConnectionsToLOList (Boolean);
static Boolean LookupPVConnectionsToLOList (Boolean);
static Boolean LookupPVConnectionsToPVList (void);
static Boolean LookupLOConnectionsToLine (LineTypePtr, Cardinal, Boolean);
static Boolean LookupLOConnectionsToPad (PadTypePtr, Cardinal);
static Boolean LookupLOConnectionsToPolygon (PolygonTypePtr, Cardinal);
static Boolean LookupLOConnectionsToArc (ArcTypePtr, Cardinal);
static Boolean LookupLOConnectionsToRatEnd (PointTypePtr, Cardinal);
static Boolean IsRatPointOnLineEnd (PointTypePtr, LineTypePtr);
static Boolean ArcArcIntersect (ArcTypePtr, ArcTypePtr);
static Boolean PrepareNextLoop (FILE *);
static Boolean PrintElementConnections (ElementTypePtr, FILE *, Boolean);
static Boolean ListsEmpty (Boolean);
static Boolean DoIt (Boolean, Boolean);
static void PrintElementNameList (ElementTypePtr, FILE *);
static void PrintConnectionElementName (ElementTypePtr, FILE *);
static void PrintConnectionListEntry (char *, ElementTypePtr,
                                      Boolean, FILE *);
static void PrintPadConnections (Cardinal, FILE *, Boolean);
static void PrintPinConnections (FILE *, Boolean);
static Boolean PrintAndSelectUnusedPinsAndPadsOfElement (ElementTypePtr,
                                                         FILE *);
static void DrawNewConnections (void);
static void ResetConnections (Boolean);
static void DumpList (void);
static void GotoError (void);
static Boolean DRCFind (int, void *, void *, void *);
static Boolean ListStart (int, void *, void *, void *);
static Boolean LOTouchesLine (LineTypePtr Line, Cardinal LayerGroup);
static Boolean PVTouchesLine (LineTypePtr line);
static Boolean SetThing (int, void *, void *, void *);

/* ---------------------------------------------------------------------------
 * some of the 'pad' routines are the same as for lines because the 'pad'
 * struct starts with a line struct. See global.h for details
 */
Boolean
LinePadIntersect (LineTypePtr Line, PadTypePtr Pad)
{
  return LineLineIntersect ((Line), (LineTypePtr)Pad);
}

Boolean
ArcPadIntersect (ArcTypePtr Arc, PadTypePtr Pad)
{
  return LineArcIntersect ((LineTypePtr) (Pad), (Arc));
}

static Boolean
ADD_PV_TO_LIST (PinTypePtr Pin)
{
  if (User)
    AddObjectToFlagUndoList (Pin->Element ? PIN_TYPE : VIA_TYPE,
                             Pin->Element ? Pin->Element : Pin, Pin, Pin);
  SET_FLAG (TheFlag, Pin);
  PVLIST_ENTRY (PVList.Number) = Pin;
  PVList.Number++;
#ifdef DEBUG
  if (PVList.Number > PVList.Size)
    printf ("ADD_PV_TO_LIST overflow! num=%d size=%d\n", PVList.Number,
            PVList.Size);
#endif
  if (drc && !TEST_FLAG (SELECTEDFLAG, Pin))
    return (SetThing (PIN_TYPE, Pin->Element, Pin, Pin));
  return False;
}

static Boolean
ADD_PAD_TO_LIST (Cardinal L, PadTypePtr Pad)
{
  if (User)
    AddObjectToFlagUndoList (PAD_TYPE, Pad->Element, Pad, Pad);
  SET_FLAG (TheFlag, Pad);
  PADLIST_ENTRY ((L), PadList[(L)].Number) = Pad;
  PadList[(L)].Number++;
#ifdef DEBUG
  if (PadList[(L)].Number > PadList[(L)].Size)
    printf ("ADD_PAD_TO_LIST overflow! lay=%d, num=%d size=%d\n", L,
            PadList[(L)].Number, PadList[(L)].Size);
#endif
  if (drc && !TEST_FLAG (SELECTEDFLAG, Pad))
    return (SetThing (PAD_TYPE, Pad->Element, Pad, Pad));
  return False;
}

static Boolean
ADD_LINE_TO_LIST (Cardinal L, LineTypePtr Ptr)
{
  if (User)
    AddObjectToFlagUndoList (LINE_TYPE, LAYER_PTR (L), (Ptr), (Ptr));
  SET_FLAG (TheFlag, (Ptr));
  LINELIST_ENTRY ((L), LineList[(L)].Number) = (Ptr);
  LineList[(L)].Number++;
#ifdef DEBUG
  if (LineList[(L)].Number > LineList[(L)].Size)
    printf ("ADD_LINE_TO_LIST overflow! lay=%d, num=%d size=%d\n", L,
            LineList[(L)].Number, LineList[(L)].Size);
#endif
  if (drc && !TEST_FLAG (SELECTEDFLAG, (Ptr)))
    return (SetThing (LINE_TYPE, LAYER_PTR (L), (Ptr), (Ptr)));
  return False;
}

static Boolean
ADD_ARC_TO_LIST (Cardinal L, ArcTypePtr Ptr)
{
  if (User)
    AddObjectToFlagUndoList (ARC_TYPE, LAYER_PTR (L), (Ptr), (Ptr));
  SET_FLAG (TheFlag, (Ptr));
  ARCLIST_ENTRY ((L), ArcList[(L)].Number) = (Ptr);
  ArcList[(L)].Number++;
#ifdef DEBUG
  if (ArcList[(L)].Number > ArcList[(L)].Size)
    printf ("ADD_ARC_TO_LIST overflow! lay=%d, num=%d size=%d\n", L,
            ArcList[(L)].Number, ArcList[(L)].Size);
#endif
  if (drc && !TEST_FLAG (SELECTEDFLAG, (Ptr)))
    return (SetThing (ARC_TYPE, LAYER_PTR (L), (Ptr), (Ptr)));
  return False;
}

static Boolean
ADD_RAT_TO_LIST (RatTypePtr Ptr)
{
  if (User)
    AddObjectToFlagUndoList (RATLINE_TYPE, (Ptr), (Ptr), (Ptr));
  SET_FLAG (TheFlag, (Ptr));
  RATLIST_ENTRY (RatList.Number) = (Ptr);
  RatList.Number++;
#ifdef DEBUG
  if (RatList.Number > RatList.Size)
    printf ("ADD_RAT_TO_LIST overflow! num=%d size=%d\n",
            RatList.Number, RatList.Size);
#endif
  if (drc && !TEST_FLAG (SELECTEDFLAG, (Ptr)))
    return (SetThing (RATLINE_TYPE, (Ptr), (Ptr), (Ptr)));
  return False;
}

static Boolean
ADD_POLYGON_TO_LIST (Cardinal L, PolygonTypePtr Ptr)
{
  if (User)
    AddObjectToFlagUndoList (POLYGON_TYPE, LAYER_PTR (L), (Ptr), (Ptr));
  SET_FLAG (TheFlag, (Ptr));
  POLYGONLIST_ENTRY ((L), PolygonList[(L)].Number) = (Ptr);
  PolygonList[(L)].Number++;
#ifdef DEBUG
  if (PolygonList[(L)].Number > PolygonList[(L)].Size)
    printf ("ADD_ARC_TO_LIST overflow! lay=%d, num=%d size=%d\n", L,
            PolygonList[(L)].Number, PolygonList[(L)].Size);
#endif
  if (drc && !TEST_FLAG (SELECTEDFLAG, (Ptr)))
    return (SetThing (POLYGON_TYPE, LAYER_PTR (L), (Ptr), (Ptr)));
  return False;
}

Boolean
PinLineIntersect (PinTypePtr PV, LineTypePtr Line)
{
  /* IsLineInRectangle already has Bloat factor */
  return TEST_FLAG (SQUAREFLAG,
                    PV) ? IsLineInRectangle (PV->X - (PV->Thickness + 1) / 2,
                                             PV->Y - (PV->Thickness + 1) / 2,
                                             PV->X + (PV->Thickness + 1) / 2,
                                             PV->Y + (PV->Thickness + 1) / 2,
                                             Line) : IsPointInPad (PV->X,
                                                                    PV->Y,
                                                                    MAX (PV->
                                                                         Thickness
                                                                         /
                                                                         2.0 +
                                                                         fBloat,
                                                                         0.0),
                                                                    (PadTypePtr)Line);
}


Boolean
SetThing (int type, void *ptr1, void *ptr2, void *ptr3)
{
  thing_ptr1 = ptr1;
  thing_ptr2 = ptr2;
  thing_ptr3 = ptr3;
  thing_type = type;
  if (type == PIN_TYPE && ptr1 == NULL)
    {
      thing_ptr1 = ptr3;
      thing_type = VIA_TYPE;
    }
  return True;
}

Boolean
BoxBoxIntersection (BoxTypePtr b1, BoxTypePtr b2)
{
  if (b2->X2 < b1->X1 || b2->X1 > b1->X2)
    return False;
  if (b2->Y2 < b1->Y1 || b2->Y1 > b1->Y2)
    return False;
  return True;
}

static Boolean
PadPadIntersect (PadTypePtr p1, PadTypePtr p2)
{
  return LinePadIntersect ((LineTypePtr) p1, p2);
}
      
static inline Boolean
PV_TOUCH_PV (PinTypePtr PV1, PinTypePtr PV2)
{
  float t1, t2;
  BoxType b1, b2;

  t1 = MAX (PV1->Thickness / 2.0 + fBloat, 0);
  t2 = MAX (PV2->Thickness / 2.0 + fBloat, 0);
  if (IsPointOnPin (PV1->X, PV1->Y, t1, PV2)
      || IsPointOnPin (PV2->X, PV2->Y, t2, PV1))
    return True;
  if (!TEST_FLAG (SQUAREFLAG, PV1) || !TEST_FLAG (SQUAREFLAG, PV2))
    return False;
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

/* ---------------------------------------------------------------------------
 * releases all allocated memory
 */
void
FreeLayoutLookupMemory (void)
{
  Cardinal i;

  for (i = 0; i < max_layer; i++)
    {
      MYFREE (LineList[i].Data);
      MYFREE (ArcList[i].Data);
      MYFREE (PolygonList[i].Data);
    }
  MYFREE (PVList.Data);
  MYFREE (RatList.Data);
}

void
FreeComponentLookupMemory (void)
{
  MYFREE (PadList[0].Data);
  MYFREE (PadList[1].Data);
}

/* ---------------------------------------------------------------------------
 * allocates memory for component related stacks ...
 * initializes index and sorts it by X1 and X2
 */
void
InitComponentLookup (void)
{
  Cardinal i;

  /* initialize pad data; start by counting the total number
   * on each of the two possible layers
   */
  NumberOfPads[COMPONENT_LAYER] = NumberOfPads[SOLDER_LAYER] = 0;
  ALLPAD_LOOP (PCB->Data);
  {
    if (TEST_FLAG (ONSOLDERFLAG, pad))
      NumberOfPads[SOLDER_LAYER]++;
    else
      NumberOfPads[COMPONENT_LAYER]++;
  }
  ENDALL_LOOP;
  for (i = 0; i < 2; i++)
    {
      /* allocate memory for working list */
      PadList[i].Data =
        (void **) MyCalloc (NumberOfPads[i], sizeof (PadTypePtr),
                            "InitComponentLookup()");

      /* clear some struct members */
      PadList[i].Location = 0;
      PadList[i].DrawLocation = 0;
      PadList[i].Number = 0;
      PadList[i].Size = NumberOfPads[i];
    }
}

/* ---------------------------------------------------------------------------
 * allocates memory for component related stacks ...
 * initializes index and sorts it by X1 and X2
 */
void
InitLayoutLookup (void)
{
  Cardinal i;

  /* initialize line arc and polygon data */
  for (i = 0; i < max_layer; i++)
    {
      LayerTypePtr layer = LAYER_PTR (i);

      if (layer->LineN)
        {
          /* allocate memory for line pointer lists */
          LineList[i].Data =
            (void **) MyCalloc (layer->LineN, sizeof (LineTypePtr),
                                "InitLayoutLookup()");
          LineList[i].Size = layer->LineN;
        }
      if (layer->ArcN)
        {
          ArcList[i].Data =
            (void **) MyCalloc (layer->ArcN, sizeof (ArcTypePtr),
                                "InitLayoutLookup()");
          ArcList[i].Size = layer->ArcN;
        }


      /* allocate memory for polygon list */
      if (layer->PolygonN)
        {
          PolygonList[i].Data = (void **) MyCalloc (layer->PolygonN,
                                                    sizeof (PolygonTypePtr),
                                                    "InitLayoutLookup()");
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
  PVList.Data = (void **) MyCalloc (TotalP + TotalV, sizeof (PinTypePtr),
                                    "InitLayoutLookup()");
  PVList.Size = TotalP + TotalV;
  PVList.Location = 0;
  PVList.DrawLocation = 0;
  PVList.Number = 0;
  /* Initialize ratline data */
  RatList.Data = (void **) MyCalloc (PCB->Data->RatN, sizeof (RatTypePtr),
                                     "InitLayoutLookup()");
  RatList.Size = PCB->Data->RatN;
  RatList.Location = 0;
  RatList.DrawLocation = 0;
  RatList.Number = 0;
}

struct pv_info
{
  Cardinal layer;
  PinType pv;
  jmp_buf env;
};

static int
LOCtoPVline_callback (const BoxType * b, void *cl)
{
  LineTypePtr line = (LineTypePtr) b;
  struct pv_info *i = (struct pv_info *) cl;

  if (!TEST_FLAG (TheFlag, line) && PinLineIntersect (&i->pv, line))
    {
      if (ADD_LINE_TO_LIST (i->layer, line))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPVarc_callback (const BoxType * b, void *cl)
{
  ArcTypePtr arc = (ArcTypePtr) b;
  struct pv_info *i = (struct pv_info *) cl;

  if (!TEST_FLAG (TheFlag, arc) && IS_PV_ON_ARC (&i->pv, arc))
    {
      if (ADD_ARC_TO_LIST (i->layer, arc))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPVpad_callback (const BoxType * b, void *cl)
{
  PadTypePtr pad = (PadTypePtr) b;
  struct pv_info *i = (struct pv_info *) cl;

  if (!TEST_FLAG (TheFlag, pad) && IS_PV_ON_PAD (&i->pv, pad) &&
      ADD_PAD_TO_LIST (TEST_FLAG (ONSOLDERFLAG, pad) ? SOLDER_LAYER :
                       COMPONENT_LAYER, pad))
    longjmp (i->env, 1);
  return 0;
}

static int
LOCtoPVrat_callback (const BoxType * b, void *cl)
{
  RatTypePtr rat = (RatTypePtr) b;
  struct pv_info *i = (struct pv_info *) cl;

  if (!TEST_FLAG (TheFlag, rat) && IS_PV_ON_RAT (&i->pv, rat) &&
      ADD_RAT_TO_LIST (rat))
    longjmp (i->env, 1);
  return 0;
}
static int
LOCtoPVpoly_callback (const BoxType * b, void *cl)
{
  PolygonTypePtr polygon = (PolygonTypePtr) b;
  struct pv_info *i = (struct pv_info *) cl;

  /* if the pin doesn't have a therm and polygon is clearing
   * then it can't touch due to clearance, so skip the expensive
   * test. If it does have a therm, you still need to test
   * because it might not be inside the polygon, or it could
   * be on an edge such that it doesn't actually touch.
   */
  if (!TEST_FLAG (TheFlag, polygon) && (TEST_THERM (i->layer, &i->pv)
                                        || !TEST_FLAG (CLEARPOLYFLAG,
                                                       polygon)
                                        || !i->pv.Clearance))
    {
      float wide = 0.5 * i->pv.Thickness + fBloat;
      wide = MAX (wide, 0);
      if (TEST_FLAG (SQUAREFLAG, &i->pv))
        {
          LocationType x1 = i->pv.X - (i->pv.Thickness + 1 + Bloat) / 2;
          LocationType x2 = i->pv.X + (i->pv.Thickness + 1 + Bloat) / 2;
          LocationType y1 = i->pv.Y - (i->pv.Thickness + 1 + Bloat) / 2;
          LocationType y2 = i->pv.Y + (i->pv.Thickness + 1 + Bloat) / 2;
          if (IsRectangleInPolygon (x1, y1, x2, y2, polygon)
              && ADD_POLYGON_TO_LIST (i->layer, polygon))
            longjmp (i->env, 1);
        }
      else if (TEST_FLAG (OCTAGONFLAG, &i->pv))
        {
          POLYAREA *oct = OctagonPoly (i->pv.X, i->pv.Y, i->pv.Thickness / 2);
          if (isects (oct, polygon, True)
              && ADD_POLYGON_TO_LIST (i->layer, polygon))
            longjmp (i->env, 1);
        }
      else if (IsPointInPolygon (i->pv.X, i->pv.Y, wide,
                                 polygon)
               && ADD_POLYGON_TO_LIST (i->layer, polygon))
        longjmp (i->env, 1);
    }
  return 0;
}

/* ---------------------------------------------------------------------------
 * checks if a PV is connected to LOs, if it is, the LO is added to
 * the appropriate list and the 'used' flag is set
 */
static Boolean
LookupLOConnectionsToPVList (Boolean AndRats)
{
  Cardinal layer;
  struct pv_info info;

  /* loop over all PVs currently on list */
  while (PVList.Location < PVList.Number)
    {
      /* get pointer to data */
      info.pv = *(PVLIST_ENTRY (PVList.Location));
      EXPAND_BOUNDS (&info.pv);

      /* check pads */
      if (setjmp (info.env) == 0)
        r_search (PCB->Data->pad_tree, (BoxType *) & info.pv, NULL,
                  LOCtoPVpad_callback, &info);
      else
        return True;

      /* now all lines, arcs and polygons of the several layers */
      for (layer = 0; layer < max_layer; layer++)
        {
          info.layer = layer;
          /* add touching lines */
          if (setjmp (info.env) == 0)
            r_search (LAYER_PTR (layer)->line_tree, (BoxType *) & info.pv,
                      NULL, LOCtoPVline_callback, &info);
          else
            return True;
          /* add touching arcs */
          if (setjmp (info.env) == 0)
            r_search (LAYER_PTR (layer)->arc_tree, (BoxType *) & info.pv,
                      NULL, LOCtoPVarc_callback, &info);
          else
            return True;
          /* check all polygons */
          if (setjmp (info.env) == 0)
            r_search (LAYER_PTR (layer)->polygon_tree, (BoxType *) & info.pv,
                      NULL, LOCtoPVpoly_callback, &info);
          else
            return True;
        }
      /* Check for rat-lines that may intersect the PV */
      if (AndRats)
        {
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->rat_tree, (BoxType *) & info.pv, NULL,
                      LOCtoPVrat_callback, &info);
          else
            return True;
        }
      PVList.Location++;
    }
  return False;
}

/* ---------------------------------------------------------------------------
 * find all connections between LO at the current list position and new LOs
 */
static Boolean
LookupLOConnectionsToLOList (Boolean AndRats)
{
  Boolean done;
  Cardinal i, group, layer, ratposition,
    lineposition[MAX_LAYER],
    polyposition[MAX_LAYER], arcposition[MAX_LAYER], padposition[2];

  /* copy the current LO list positions; the original data is changed
   * by 'LookupPVConnectionsToLOList()' which has to check the same
   * list entries plus the new ones
   */
  for (i = 0; i < max_layer; i++)
    {
      lineposition[i] = LineList[i].Location;
      polyposition[i] = PolygonList[i].Location;
      arcposition[i] = ArcList[i].Location;
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
                  (&(RATLIST_ENTRY (*position)->Point1), group))
                return (True);
              group = RATLIST_ENTRY (*position)->group2;
              if (LookupLOConnectionsToRatEnd
                  (&(RATLIST_ENTRY (*position)->Point2), group))
                return (True);
            }
        }
      /* loop over all layergroups */
      for (group = 0; group < max_layer; group++)
        {
          Cardinal entry;

          for (entry = 0; entry < PCB->LayerGroups.Number[group]; entry++)
            {
              layer = PCB->LayerGroups.Entries[group][entry];

              /* be aware that the layer number equal max_layer
               * and max_layer+1 have a special meaning for pads
               */
              if (layer < max_layer)
                {
                  /* try all new lines */
                  position = &lineposition[layer];
                  for (; *position < LineList[layer].Number; (*position)++)
                    if (LookupLOConnectionsToLine
                        (LINELIST_ENTRY (layer, *position), group, True))
                      return (True);

                  /* try all new arcs */
                  position = &arcposition[layer];
                  for (; *position < ArcList[layer].Number; (*position)++)
                    if (LookupLOConnectionsToArc
                        (ARCLIST_ENTRY (layer, *position), group))
                      return (True);

                  /* try all new polygons */
                  position = &polyposition[layer];
                  for (; *position < PolygonList[layer].Number; (*position)++)
                    if (LookupLOConnectionsToPolygon
                        (POLYGONLIST_ENTRY (layer, *position), group))
                      return (True);
                }
              else
                {
                  /* try all new pads */
                  layer -= max_layer;
                  if (layer > 1)
                    {
                      Message ("bad layer number %d max_layer=%d in find.c\n",
                               layer, max_layer);
                      return False;
                    }
                  position = &padposition[layer];
                  for (; *position < PadList[layer].Number; (*position)++)
                    if (LookupLOConnectionsToPad
                        (PADLIST_ENTRY (layer, *position), group))
                      return (True);
                }
            }
        }

      /* check if all lists are done; Later for-loops
       * may have changed the prior lists
       */
      done = !AndRats || ratposition >= RatList.Number;
      for (layer = 0; layer < max_layer + 2; layer++)
        {
          if (layer < max_layer)
            done = done &&
              lineposition[layer] >= LineList[layer].Number
              && arcposition[layer] >= ArcList[layer].Number
              && polyposition[layer] >= PolygonList[layer].Number;
          else
            done = done
              && padposition[layer - max_layer] >=
              PadList[layer - max_layer].Number;
        }
    }
  while (!done);
  return (False);
}

static int
pv_pv_callback (const BoxType * b, void *cl)
{
  PinTypePtr pin = (PinTypePtr) b;
  struct pv_info *i = (struct pv_info *) cl;

  if (!TEST_FLAG (TheFlag, pin) && PV_TOUCH_PV (&i->pv, pin))
    {
      if (TEST_FLAG (HOLEFLAG, pin))
        {
          SET_FLAG (WARNFLAG, pin);
          Settings.RatWarn = True;
          if (pin->Element)
            Message (_("WARNING: Hole too close to pin.\n"));
          else
            Message (_("WARNING: Hole too close to via.\n"));
        }
      if (ADD_PV_TO_LIST (pin))
        longjmp (i->env, 1);
    }
  return 0;
}

/* ---------------------------------------------------------------------------
 * searches for new PVs that are connected to PVs on the list
 */
static Boolean
LookupPVConnectionsToPVList (void)
{
  Cardinal save_place;
  struct pv_info info;


  /* loop over all PVs on list */
  save_place = PVList.Location;
  while (PVList.Location < PVList.Number)
    {
      /* get pointer to data */
      info.pv = *(PVLIST_ENTRY (PVList.Location));
      EXPAND_BOUNDS (&info.pv);
      if (setjmp (info.env) == 0)
        r_search (PCB->Data->via_tree, (BoxType *) & info.pv, NULL,
                  pv_pv_callback, &info);
      else
        return True;
      if (setjmp (info.env) == 0)
        r_search (PCB->Data->pin_tree, (BoxType *) & info.pv, NULL,
                  pv_pv_callback, &info);
      else
        return True;
      PVList.Location++;
    }
  PVList.Location = save_place;
  return (False);
}

struct lo_info
{
  Cardinal layer;
  LineType line;
  PadType pad;
  ArcType arc;
  PolygonType polygon;
  RatType rat;
  jmp_buf env;
};

static int
pv_line_callback (const BoxType * b, void *cl)
{
  PinTypePtr pv = (PinTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (TheFlag, pv) && PinLineIntersect (pv, &i->line))
    {
      if (TEST_FLAG (HOLEFLAG, pv))
        {
          SET_FLAG (WARNFLAG, pv);
          Settings.RatWarn = True;
          Message (_("WARNING: Hole too close to line.\n"));
        }
      if (ADD_PV_TO_LIST (pv))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
pv_pad_callback (const BoxType * b, void *cl)
{
  PinTypePtr pv = (PinTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (TheFlag, pv) && IS_PV_ON_PAD (pv, &i->pad))
    {
      if (TEST_FLAG (HOLEFLAG, pv))
        {
          SET_FLAG (WARNFLAG, pv);
          Settings.RatWarn = True;
          Message (_("WARNING: Hole too close to pad.\n"));
        }
      if (ADD_PV_TO_LIST (pv))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
pv_arc_callback (const BoxType * b, void *cl)
{
  PinTypePtr pv = (PinTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (TheFlag, pv) && IS_PV_ON_ARC (pv, &i->arc))
    {
      if (TEST_FLAG (HOLEFLAG, pv))
        {
          SET_FLAG (WARNFLAG, pv);
          Settings.RatWarn = True;
          Message (_("WARNING: Hole touches arc.\n"));
        }
      if (ADD_PV_TO_LIST (pv))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
pv_poly_callback (const BoxType * b, void *cl)
{
  PinTypePtr pv = (PinTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  /* note that holes in polygons are ok */
  if (!TEST_FLAG (TheFlag, pv) && (TEST_THERM (i->layer, pv) ||
                                   !TEST_FLAG (CLEARPOLYFLAG, &i->polygon) ||
                                   !pv->Clearance))
    {
      if (TEST_FLAG (SQUAREFLAG, pv))
        {
          LocationType x1, x2, y1, y2;
          x1 = pv->X - (pv->Thickness + 1 + Bloat) / 2;
          x2 = pv->X + (pv->Thickness + 1 + Bloat) / 2;
          y1 = pv->Y - (pv->Thickness + 1 + Bloat) / 2;
          y2 = pv->Y + (pv->Thickness + 1 + Bloat) / 2;
          if (IsRectangleInPolygon (x1, y1, x2, y2, &i->polygon)
              && ADD_PV_TO_LIST (pv))
            longjmp (i->env, 1);
        }
      else if (TEST_FLAG (OCTAGONFLAG, pv))
        {
          POLYAREA *oct = OctagonPoly (pv->X, pv->Y, pv->Thickness / 2);
          if (isects (oct, &i->polygon, True) && ADD_PV_TO_LIST (pv))
            longjmp (i->env, 1);
        }
      else
        {
          if (IsPointInPolygon
              (pv->X, pv->Y, pv->Thickness * 0.5 + fBloat, &i->polygon)
              && ADD_PV_TO_LIST (pv))
            longjmp (i->env, 1);
        }
    }
  return 0;
}

static int
pv_rat_callback (const BoxType * b, void *cl)
{
  PinTypePtr pv = (PinTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  /* rats can't cause DRC so there is no early exit */
  if (!TEST_FLAG (TheFlag, pv) && IS_PV_ON_RAT (pv, &i->rat))
    ADD_PV_TO_LIST (pv);
  return 0;
}

/* ---------------------------------------------------------------------------
 * searches for new PVs that are connected to NEW LOs on the list
 * This routine updates the position counter of the lists too.
 */
static Boolean
LookupPVConnectionsToLOList (Boolean AndRats)
{
  Cardinal layer;
  struct lo_info info;

  /* loop over all layers */
  for (layer = 0; layer < max_layer; layer++)
    {
      /* do nothing if there are no PV's */
      if (TotalP + TotalV == 0)
        {
          LineList[layer].Location = LineList[layer].Number;
          ArcList[layer].Location = ArcList[layer].Number;
          PolygonList[layer].Location = PolygonList[layer].Number;
          continue;
        }

      /* check all lines */
      while (LineList[layer].Location < LineList[layer].Number)
        {
          info.line = *(LINELIST_ENTRY (layer, LineList[layer].Location));
          EXPAND_BOUNDS (&info.line);
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->via_tree, (BoxType *) & info.line, NULL,
                      pv_line_callback, &info);
          else
            return True;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pin_tree, (BoxType *) & info.line, NULL,
                      pv_line_callback, &info);
          else
            return True;
          LineList[layer].Location++;
        }

      /* check all arcs */
      while (ArcList[layer].Location < ArcList[layer].Number)
        {
          info.arc = *(ARCLIST_ENTRY (layer, ArcList[layer].Location));
          EXPAND_BOUNDS (&info.arc);
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->via_tree, (BoxType *) & info.arc, NULL,
                      pv_arc_callback, &info);
          else
            return True;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pin_tree, (BoxType *) & info.arc, NULL,
                      pv_arc_callback, &info);
          else
            return True;
          ArcList[layer].Location++;
        }

      /* now all polygons */
      info.layer = layer;
      while (PolygonList[layer].Location < PolygonList[layer].Number)
        {
          info.polygon =
            *(POLYGONLIST_ENTRY (layer, PolygonList[layer].Location));
          EXPAND_BOUNDS (&info.polygon);
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->via_tree, (BoxType *) & info.polygon, NULL,
                      pv_poly_callback, &info);
          else
            return True;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pin_tree, (BoxType *) & info.polygon, NULL,
                      pv_poly_callback, &info);
          else
            return True;
          PolygonList[layer].Location++;
        }
    }

  /* loop over all pad-layers */
  for (layer = 0; layer < 2; layer++)
    {
      /* do nothing if there are no PV's */
      if (TotalP + TotalV == 0)
        {
          PadList[layer].Location = PadList[layer].Number;
          continue;
        }

      /* check all pads; for a detailed description see
       * the handling of lines in this subroutine
       */
      while (PadList[layer].Location < PadList[layer].Number)
        {
          info.pad = *(PADLIST_ENTRY (layer, PadList[layer].Location));
          EXPAND_BOUNDS (&info.pad);
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->via_tree, (BoxType *) & info.pad, NULL,
                      pv_pad_callback, &info);
          else
            return True;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pin_tree, (BoxType *) & info.pad, NULL,
                      pv_pad_callback, &info);
          else
            return True;
          PadList[layer].Location++;
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
          info.rat = *(RATLIST_ENTRY (RatList.Location));
          r_search_pt (PCB->Data->via_tree, & info.rat.Point1, 1, NULL,
                    pv_rat_callback, &info);
          r_search_pt (PCB->Data->via_tree, & info.rat.Point2, 1, NULL,
                    pv_rat_callback, &info);
          r_search_pt (PCB->Data->pin_tree, & info.rat.Point1, 1, NULL,
                    pv_rat_callback, &info);
          r_search_pt (PCB->Data->pin_tree, & info.rat.Point2, 1, NULL,
                    pv_rat_callback, &info);

          RatList.Location++;
        }
    }
  return (False);
}

int
pv_touch_callback (const BoxType * b, void *cl)
{
  PinTypePtr pin = (PinTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (TheFlag, pin) && PinLineIntersect (pin, &i->line))
    longjmp (i->env, 1);
  return 0;
}

static Boolean
PVTouchesLine (LineTypePtr line)
{
  struct lo_info info;

  info.line = *line;
  EXPAND_BOUNDS (&info.line);
  if (setjmp (info.env) == 0)
    r_search (PCB->Data->via_tree, (BoxType *) & info.line, NULL,
              pv_touch_callback, &info);
  else
    return True;
  if (setjmp (info.env) == 0)
    r_search (PCB->Data->pin_tree, (BoxType *) & info.line, NULL,
              pv_touch_callback, &info);
  else
    return True;

  return (False);
}

/* ---------------------------------------------------------------------------
 * check if two arcs intersect
 * first we check for circle intersections,
 * then find the actual points of intersection
 * and test them to see if they are on both arcs
 *
 * consider a, the distance from the center of arc 1
 * to the point perpendicular to the intersecting points.
 *
 *  a = (r1^2 - r2^2 + l^2)/(2l)
 *
 * the perpendicular distance to the point of intersection
 * is then
 *
 * d = sqrt(r1^2 - a^2)
 *
 * the points of intersection would then be
 *
 * x = X1 + a/l dx +- d/l dy
 * y = Y1 + a/l dy -+ d/l dx
 *
 * where dx = X2 - X1 and dy = Y2 - Y1
 *
 *
 */
static Boolean
ArcArcIntersect (ArcTypePtr Arc1, ArcTypePtr Arc2)
{
  register float x, y, dx, dy, r1, r2, a, d, l, t, t2;
  register LocationType pdx, pdy;
  BoxTypePtr box;
  BoxType box1, box2;

  pdx = Arc2->X - Arc1->X;
  pdy = Arc2->Y - Arc1->Y;
  l = pdx * pdx + pdy * pdy;
  t = MAX (0.5 * Arc1->Thickness + fBloat, 0.0);
  t2 = 0.5 * Arc2->Thickness;
  /* concentric arcs, simpler intersection conditions */
  if (l == 0.0)
    {
      if ((Arc1->Width - t >= Arc2->Width - t2
           && Arc1->Width - t <=
           Arc2->Width + t2)
          || (Arc1->Width + t >=
              Arc2->Width - t2 && Arc1->Width + t <= Arc2->Width + t2))
        {
          if ((Arc2->BoundingBox.X1 +
               MAX (Arc2->Thickness + Bloat,
                    0) >= Arc1->BoundingBox.X1
               && Arc2->BoundingBox.X1 <=
               Arc1->BoundingBox.X2
               && Arc2->BoundingBox.Y1 +
               MAX (Arc2->Thickness + Bloat,
                    0) >= Arc1->BoundingBox.Y1
               && Arc2->BoundingBox.Y1 <=
               Arc1->BoundingBox.Y2)
              || (Arc2->BoundingBox.X2 +
                  MAX (Arc2->Thickness +
                       Bloat,
                       0) >=
                  Arc1->BoundingBox.X1
                  && Arc2->BoundingBox.X2 <=
                  Arc1->BoundingBox.X2
                  && Arc2->BoundingBox.Y2 +
                  MAX (Arc2->Thickness +
                       Bloat,
                       0) >=
                  Arc1->BoundingBox.Y1
                  && Arc2->BoundingBox.Y2 <= Arc1->BoundingBox.Y2))
            return (True);
        }
      return (False);
    }
  r1 = Arc1->Width + t;
  r1 *= r1;
  r2 = Arc2->Width + t2;
  r2 *= r2;
  a = 0.5 * (r1 - r2 + l) / l;
  r1 = r1 / l;
  /* add a tiny positive number for round-off problems */
  d = r1 - a * a + 1e-5;
  /* the circles are too far apart to touch */
  if (d < 0)
    return (False);
  d = sqrt (d);
  x = Arc1->X + a * pdx;
  y = Arc1->Y + a * pdy;
  dx = d * pdx;
  dy = d * pdy;
  if (x + dy >= Arc1->BoundingBox.X1
      && x + dy <= Arc1->BoundingBox.X2
      && y - dx >= Arc1->BoundingBox.Y1
      && y - dx <= Arc1->BoundingBox.Y2
      && x + dy >= Arc2->BoundingBox.X1
      && x + dy <= Arc2->BoundingBox.X2
      && y - dx >= Arc2->BoundingBox.Y1 && y - dx <= Arc2->BoundingBox.Y2)
    return (True);

  if (x - dy >= Arc1->BoundingBox.X1
      && x - dy <= Arc1->BoundingBox.X2
      && y + dx >= Arc1->BoundingBox.Y1
      && y + dx <= Arc1->BoundingBox.Y2
      && x - dy >= Arc2->BoundingBox.X1
      && x - dy <= Arc2->BoundingBox.X2
      && y + dx >= Arc2->BoundingBox.Y1 && y + dx <= Arc2->BoundingBox.Y2)
    return (True);

  /* try the end points in case the ends don't actually pierce the outer radii */
  box = GetArcEnds (Arc1);
  box1 = *box;
  box = GetArcEnds (Arc2);
  box2 = *box;
  if (IsPointOnArc
      ((float) box1.X1, (float) box1.Y1, t,
       Arc2) || IsPointOnArc ((float) box1.X2, (float) box1.Y2, t, Arc2))
    return (True);

  if (IsPointOnArc
      ((float) box2.X1, (float) box2.Y1,
       MAX (t2 + fBloat, 0.0), Arc1)
      || IsPointOnArc ((float) box2.X2,
                       (float) box2.Y2, MAX (t2 + fBloat, 0.0), Arc1))
    return (True);
  return (False);
}

/* ---------------------------------------------------------------------------
 * Tests if point is same as line end point
 */
static Boolean
IsRatPointOnLineEnd (PointTypePtr Point, LineTypePtr Line)
{
  if ((Point->X == Line->Point1.X
       && Point->Y == Line->Point1.Y)
      || (Point->X == Line->Point2.X && Point->Y == Line->Point2.Y))
    return (True);
  return (False);
}

static void 
form_slanted_rectangle(PointType p[4],LineTypePtr l)
/* writes vertices of a squared line */
{
   int dX= l->Point2.X - l->Point1.X, dY = l->Point2.Y - l->Point1.Y, 
     w = l->Thickness;
   double dwx, dwy;
   if (dY == 0)
     {
       dwx = w / 2; dwy = 0;
     }
    else if (dX == 0)
     {
       dwx = 0; dwy = w / 2;
     }
    else 
     {double r = sqrt (dX * (double) dX + dY * (double) dY) * 2;
       dwx = w / r * dX; dwy =  w / r * dY;
     }
    p[0].X = l->Point1.X - dwx + dwy; p[0].Y = l->Point1.Y - dwy - dwx;
    p[1].X = l->Point1.X - dwx - dwy; p[1].Y = l->Point1.Y - dwy + dwx;
    p[2].X = l->Point2.X + dwx - dwy; p[2].Y = l->Point2.Y + dwy + dwx;
    p[3].X = l->Point2.X + dwx + dwy; p[3].Y = l->Point2.Y + dwy - dwx;
}
/* ---------------------------------------------------------------------------
 * checks if two lines intersect
 * from news FAQ:
 *
 *  Let A,B,C,D be 2-space position vectors.  Then the directed line
 *  segments AB & CD are given by:
 *
 *      AB=A+r(B-A), r in [0,1]
 *      CD=C+s(D-C), s in [0,1]
 *
 *  If AB & CD intersect, then
 *
 *      A+r(B-A)=C+s(D-C), or
 *
 *      XA+r(XB-XA)=XC+s(XD-XC)
 *      YA+r(YB-YA)=YC+s(YD-YC)  for some r,s in [0,1]
 *
 *  Solving the above for r and s yields
 *
 *          (YA-YC)(XD-XC)-(XA-XC)(YD-YC)
 *      r = -----------------------------  (eqn 1)
 *          (XB-XA)(YD-YC)-(YB-YA)(XD-XC)
 *
 *          (YA-YC)(XB-XA)-(XA-XC)(YB-YA)
 *      s = -----------------------------  (eqn 2)
 *          (XB-XA)(YD-YC)-(YB-YA)(XD-XC)
 *
 *  Let I be the position vector of the intersection point, then
 *
 *      I=A+r(B-A) or
 *
 *      XI=XA+r(XB-XA)
 *      YI=YA+r(YB-YA)
 *
 *  By examining the values of r & s, you can also determine some
 *  other limiting conditions:
 *
 *      If 0<=r<=1 & 0<=s<=1, intersection exists
 *          r<0 or r>1 or s<0 or s>1 line segments do not intersect
 *
 *      If the denominator in eqn 1 is zero, AB & CD are parallel
 *      If the numerator in eqn 1 is also zero, AB & CD are coincident
 *
 *  If the intersection point of the 2 lines are needed (lines in this
 *  context mean infinite lines) regardless whether the two line
 *  segments intersect, then
 *
 *      If r>1, I is located on extension of AB
 *      If r<0, I is located on extension of BA
 *      If s>1, I is located on extension of CD
 *      If s<0, I is located on extension of DC
 *
 *  Also note that the denominators of eqn 1 & 2 are identical.
 *
 */
Boolean
LineLineIntersect (LineTypePtr Line1, LineTypePtr Line2)
{
  register float dx, dy, dx1, dy1, s, r;
  if (TEST_FLAG (SQUAREFLAG, Line1))/* pretty reckless recursion */
    {
      PointType p[4];form_slanted_rectangle(p,Line1);
      return IsLineInQuadrangle(p,Line2);
    }
  /* here come only round Line1 because IsLineInQuadrangle()
     calls LineLineIntersect() with first argument rounded*/
  if (TEST_FLAG (SQUAREFLAG, Line2))
    {
      PointType p[4];form_slanted_rectangle(p,Line2);
      return IsLineInQuadrangle(p,Line1);
    }
  /* now all lines are round */

#if 0
  if (Line1->BoundingBox.X1 - Bloat > Line2->BoundBoxing.X2
      || Line1->BoundingBox.X2 + Bloat < Line2->BoundingBox.X1
      || Line1->BoundingBox.Y1 - Bloat < Line2->BoundingBox.Y2
      || Line1->BoundingBox.Y2 + Bloat < Line2->BoundingBox.Y1)
    return False;
#endif

  /* setup some constants */
  dx = (float) (Line1->Point2.X - Line1->Point1.X);
  dy = (float) (Line1->Point2.Y - Line1->Point1.Y);
  dx1 = (float) (Line1->Point1.X - Line2->Point1.X);
  dy1 = (float) (Line1->Point1.Y - Line2->Point1.Y);
  s = dy1 * dx - dx1 * dy;

  r =
    dx * (float) (Line2->Point2.Y -
                  Line2->Point1.Y) -
    dy * (float) (Line2->Point2.X - Line2->Point1.X);

  /* handle parallel lines */
  if (r == 0.0)
    {
      /* at least one of the two end points of one segment
       * has to have a minimum distance to the other segment
       *
       * a first quick check is to see if the distance between
       * the two lines is less then their half total thickness
       */
      register float distance;

      /* perhaps line 1 is really just a point */
      if ((dx == 0) && (dy == 0))
        return IsPointInPad
                (Line1->Point1.X,
                 Line1->Point1.Y,
                 MAX (Line1->Thickness / 2 +
                      Bloat, 0),
                 (PadTypePtr) Line2);
      s = s * s / (dx * dx + dy * dy);


      distance =
        MAX ((float) 0.5 *
             (Line1->Thickness + Line2->Thickness) + fBloat, 0.0);
      distance *= distance;
      if (s > distance)
        return (False);
      if (IsPointInPad (Line2->Point1.
                               X,
                               Line2->Point1.
                               Y,
                               MAX (Line2->
                                    Thickness
                                    / 2 +
                                    Bloat, 0),
                               (PadTypePtr)
                               Line1)
           || IsPointInPad (Line2->
                                  Point2.X,
                                  Line2->
                                  Point2.Y,
                                  MAX (Line2->
                                       Thickness
                                       / 2 + Bloat, 0), (PadTypePtr) Line1))
        return (True);
      return ((IsPointInPad (Line1->Point1.
                               X,
                               Line1->Point1.
                               Y,
                               MAX (Line1->
                                    Thickness
                                    / 2 +
                                    Bloat, 0),
                               (PadTypePtr)
                               Line2)
           || IsPointInPad (Line1->
                                  Point2.X,
                                  Line1->
                                  Point2.Y,
                                  MAX (Line1->
                                       Thickness
                                       / 2 + Bloat, 0), (PadTypePtr) Line2)));
    }
  else
    {
      s /= r;
      r =
        (dy1 *
         (float) (Line2->Point2.X -
                  Line2->Point1.X) -
         dx1 * (float) (Line2->Point2.Y - Line2->Point1.Y)) / r;

      /* intersection is at least on AB */
      if (r >= 0.0 && r <= 1.0)
        {
          if (s >= 0.0 && s <= 1.0)
            return (True);

          /* intersection on AB and extension of CD */
          return (s < 0.0 ?
                  IsPointInPad
                  (Line2->Point1.X,
                   Line2->Point1.Y,
                   MAX (0.5 *
                        Line2->Thickness +
                        fBloat, 0.0),
                   (PadTypePtr)Line1) :
                  IsPointInPad
                  (Line2->Point2.X,
                   Line2->Point2.Y,
                   MAX (0.5 * Line2->Thickness + fBloat, 0.0), (PadTypePtr)Line1));
        }

      /* intersection is at least on CD */
      if (s >= 0.0 && s <= 1.0)
        {
          /* intersection on CD and extension of AB */
          return (r < 0.0 ?
                  IsPointInPad
                  (Line1->Point1.X,
                   Line1->Point1.Y,
                   MAX (Line1->Thickness /
                        2.0 + fBloat, 0.0),
                   (PadTypePtr)Line2) :
                  IsPointInPad
                  (Line1->Point2.X,
                   Line1->Point2.Y,
                   MAX (Line1->Thickness / 2.0 + fBloat, 0.0), (PadTypePtr)Line2));
        }

      /* no intersection of zero-width lines but maybe of thick lines;
       * Must check each end point to exclude intersection
       */
      if (IsPointInPad (Line1->Point1.X, Line1->Point1.Y,
                         Line1->Thickness / 2.0 + fBloat, (PadTypePtr)Line2))
        return True;
      if (IsPointInPad (Line1->Point2.X, Line1->Point2.Y,
                         Line1->Thickness / 2.0 + fBloat, (PadTypePtr)Line2))
        return True;
      if (IsPointInPad (Line2->Point1.X, Line2->Point1.Y,
                         Line2->Thickness / 2.0 + fBloat, (PadTypePtr)Line1))
        return True;
      return IsPointInPad (Line2->Point2.X, Line2->Point2.Y,
                            Line2->Thickness / 2.0 + fBloat, (PadTypePtr)Line1);
    }
}

/*---------------------------------------------------
 *
 * Check for line intersection with an arc
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
 * find the point Q
 *
 * The projection is now of the form
 *
 *      Px = X1 + (r +- r2)(X2 - X1)
 *      Py = Y1 + (r +- r2)(Y2 - Y1)
 *
 * Where r2 sqrt(Radius^2 l^2 - d^2)/l^2
 * note that this is the variable d, not the symbol d described in IsPointOnLine
 * (variable d = symbol d * l)
 *
 * The end points are hell so they are checked individually
 */
Boolean
LineArcIntersect (LineTypePtr Line, ArcTypePtr Arc)
{
  register float dx, dy, dx1, dy1, l, d, r, r2, Radius;
  BoxTypePtr box;

  dx = (float) (Line->Point2.X - Line->Point1.X);
  dy = (float) (Line->Point2.Y - Line->Point1.Y);
  dx1 = (float) (Line->Point1.X - Arc->X);
  dy1 = (float) (Line->Point1.Y - Arc->Y);
  l = dx * dx + dy * dy;
  d = dx * dy1 - dy * dx1;
  d *= d;

  /* use the larger diameter circle first */
  Radius =
    Arc->Width + MAX (0.5 * (Arc->Thickness + Line->Thickness) + fBloat, 0.0);
  Radius *= Radius;
  r2 = Radius * l - d;
  /* projection doesn't even intersect circle when r2 < 0 */
  if (r2 < 0)
    return (False);
  /* check the ends of the line in case the projected point */
  /* of intersection is beyond the line end */
  if (IsPointOnArc
      (Line->Point1.X, Line->Point1.Y,
       MAX (0.5 * Line->Thickness + fBloat, 0.0), Arc))
    return (True);
  if (IsPointOnArc
      (Line->Point2.X, Line->Point2.Y,
       MAX (0.5 * Line->Thickness + fBloat, 0.0), Arc))
    return (True);
  if (l == 0.0)
    return (False);
  r2 = sqrt (r2);
  Radius = -(dx * dx1 + dy * dy1);
  r = (Radius + r2) / l;
  if (r >= 0 && r <= 1
      && IsPointOnArc (Line->Point1.X + r * dx,
                       Line->Point1.Y + r * dy,
                       MAX (0.5 * Line->Thickness + fBloat, 0.0), Arc))
    return (True);
  r = (Radius - r2) / l;
  if (r >= 0 && r <= 1
      && IsPointOnArc (Line->Point1.X + r * dx,
                       Line->Point1.Y + r * dy,
                       MAX (0.5 * Line->Thickness + fBloat, 0.0), Arc))
    return (True);
  /* check arc end points */
  box = GetArcEnds (Arc);
  if (IsPointInPad (box->X1, box->Y1, Arc->Thickness * 0.5 + fBloat, (PadTypePtr)Line))
    return True;
  if (IsPointInPad (box->X2, box->Y2, Arc->Thickness * 0.5 + fBloat, (PadTypePtr)Line))
    return True;
  return False;
}

static int
LOCtoArcLine_callback (const BoxType * b, void *cl)
{
  LineTypePtr line = (LineTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (TheFlag, line) && LineArcIntersect (line, &i->arc))
    {
      if (ADD_LINE_TO_LIST (i->layer, line))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoArcArc_callback (const BoxType * b, void *cl)
{
  ArcTypePtr arc = (ArcTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!arc->Thickness)
    return 0;
  if (!TEST_FLAG (TheFlag, arc) && ArcArcIntersect (&i->arc, arc))
    {
      if (ADD_ARC_TO_LIST (i->layer, arc))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoArcPad_callback (const BoxType * b, void *cl)
{
  PadTypePtr pad = (PadTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (TheFlag, pad) && i->layer ==
      (TEST_FLAG (ONSOLDERFLAG, pad) ? SOLDER_LAYER : COMPONENT_LAYER)
      && ArcPadIntersect (&i->arc, pad) && ADD_PAD_TO_LIST (i->layer, pad))
    longjmp (i->env, 1);
  return 0;
}

/* ---------------------------------------------------------------------------
 * searches all LOs that are connected to the given arc on the given
 * layergroup. All found connections are added to the list
 *
 * the notation that is used is:
 * Xij means Xj at arc i
 */
static Boolean
LookupLOConnectionsToArc (ArcTypePtr Arc, Cardinal LayerGroup)
{
  Cardinal entry;
  LocationType xlow, xhigh;
  struct lo_info info;

  /* the maximum possible distance */
  xlow = Arc->BoundingBox.X1 - MAX (MAX_PADSIZE, MAX_LINESIZE) / 2;
  xhigh = Arc->BoundingBox.X2 + MAX (MAX_PADSIZE, MAX_LINESIZE) / 2;

  info.arc = *Arc;
  EXPAND_BOUNDS (&info.arc);
  /* loop over all layers of the group */
  for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++)
    {
      Cardinal layer, i;

      layer = PCB->LayerGroups.Entries[LayerGroup][entry];

      /* handle normal layers */
      if (layer < max_layer)
        {
          PolygonTypePtr polygon;

          info.layer = layer;
          /* add arcs */
          if (setjmp (info.env) == 0)
            r_search (LAYER_PTR (layer)->line_tree, &info.arc.BoundingBox,
                      NULL, LOCtoArcLine_callback, &info);
          else
            return True;

          if (setjmp (info.env) == 0)
            r_search (LAYER_PTR (layer)->arc_tree, &info.arc.BoundingBox,
                      NULL, LOCtoArcArc_callback, &info);
          else
            return True;

          /* now check all polygons */
          i = 0;
          polygon = PCB->Data->Layer[layer].Polygon;
          for (; i < PCB->Data->Layer[layer].PolygonN; i++, polygon++)
            if (!TEST_FLAG (TheFlag, polygon) && IsArcInPolygon (Arc, polygon)
                && ADD_POLYGON_TO_LIST (layer, polygon))
              return True;
        }
      else
        {
          info.layer = layer - max_layer;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pad_tree, &info.arc.BoundingBox, NULL,
                      LOCtoArcPad_callback, &info);
          else
            return True;
        }
    }
  return (False);
}

static int
LOCtoLineLine_callback (const BoxType * b, void *cl)
{
  LineTypePtr line = (LineTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (TheFlag, line) && LineLineIntersect (&i->line, line))
    {
      if (ADD_LINE_TO_LIST (i->layer, line))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoLineArc_callback (const BoxType * b, void *cl)
{
  ArcTypePtr arc = (ArcTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!arc->Thickness)
    return 0;
  if (!TEST_FLAG (TheFlag, arc) && LineArcIntersect (&i->line, arc))
    {
      if (ADD_ARC_TO_LIST (i->layer, arc))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoLineRat_callback (const BoxType * b, void *cl)
{
  RatTypePtr rat = (RatTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (TheFlag, rat))
    {
      if ((rat->group1 == i->layer)
          && IsRatPointOnLineEnd (&rat->Point1, &i->line))
        {
          if (ADD_RAT_TO_LIST (rat))
            longjmp (i->env, 1);
        }
      else if ((rat->group2 == i->layer)
               && IsRatPointOnLineEnd (&rat->Point2, &i->line))
        {
          if (ADD_RAT_TO_LIST (rat))
            longjmp (i->env, 1);
        }
    }
  return 0;
}

static int
LOCtoLinePad_callback (const BoxType * b, void *cl)
{
  PadTypePtr pad = (PadTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (TheFlag, pad) && i->layer ==
      (TEST_FLAG (ONSOLDERFLAG, pad) ? SOLDER_LAYER : COMPONENT_LAYER)
      && LinePadIntersect (&i->line, pad) && ADD_PAD_TO_LIST (i->layer, pad))
    longjmp (i->env, 1);
  return 0;
}

/* ---------------------------------------------------------------------------
 * searches all LOs that are connected to the given line on the given
 * layergroup. All found connections are added to the list
 *
 * the notation that is used is:
 * Xij means Xj at line i
 */
static Boolean
LookupLOConnectionsToLine (LineTypePtr Line, Cardinal LayerGroup,
                           Boolean PolysTo)
{
  Cardinal entry;
  struct lo_info info;

  info.line = *Line;
  info.layer = LayerGroup;
  EXPAND_BOUNDS (&info.line)
  /* add the new rat lines */
  if (setjmp (info.env) == 0)
    r_search (PCB->Data->rat_tree, &info.line.BoundingBox, NULL,
              LOCtoLineRat_callback, &info);
  else
    return True;

  /* loop over all layers of the group */
  for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++)
    {
      Cardinal layer;

      layer = PCB->LayerGroups.Entries[LayerGroup][entry];

      /* handle normal layers */
      if (layer < max_layer)
        {
          PolygonTypePtr polygon;

          info.layer = layer;
          /* add lines */
          if (setjmp (info.env) == 0)
            r_search (LAYER_PTR (layer)->line_tree, (BoxType *) & info.line,
                      NULL, LOCtoLineLine_callback, &info);
          else
            return True;
          /* add arcs */
          if (setjmp (info.env) == 0)
            r_search (LAYER_PTR (layer)->arc_tree, (BoxType *) & info.line,
                      NULL, LOCtoLineArc_callback, &info);
          else
            return True;
          /* now check all polygons */
          if (PolysTo)
            {
              Cardinal i = 0;
              polygon = PCB->Data->Layer[layer].Polygon;
              for (; i < PCB->Data->Layer[layer].PolygonN; i++, polygon++)
                if (!TEST_FLAG
                    (TheFlag, polygon) && IsLineInPolygon (Line, polygon)
                    && ADD_POLYGON_TO_LIST (layer, polygon))
                  return True;
            }
        }
      else
        {
          /* handle special 'pad' layers */
          info.layer = layer - max_layer;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pad_tree, &info.line.BoundingBox, NULL,
                      LOCtoLinePad_callback, &info);
          else
            return True;
        }
    }
  return (False);
}

static int
LOT_Linecallback (const BoxType * b, void *cl)
{
  LineTypePtr line = (LineTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (TheFlag, line) && LineLineIntersect (&i->line, line))
    longjmp (i->env, 1);
  return 0;
}

static int
LOT_Arccallback (const BoxType * b, void *cl)
{
  ArcTypePtr arc = (ArcTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!arc->Thickness)
    return 0;
  if (!TEST_FLAG (TheFlag, arc) && LineArcIntersect (&i->line, arc))
    longjmp (i->env, 1);
  return 0;
}

static int
LOT_Padcallback (const BoxType * b, void *cl)
{
  PadTypePtr pad = (PadTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (TheFlag, pad) && i->layer ==
      (TEST_FLAG (ONSOLDERFLAG, pad) ? SOLDER_LAYER : COMPONENT_LAYER)
      && LinePadIntersect (&i->line, pad))
    longjmp (i->env, 1);
  return 0;
}

static Boolean
LOTouchesLine (LineTypePtr Line, Cardinal LayerGroup)
{
  Cardinal entry;
  Cardinal i;
  struct lo_info info;


  /* the maximum possible distance */

  info.line = *Line;
  EXPAND_BOUNDS (&info.line);

  /* loop over all layers of the group */
  for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++)
    {
      Cardinal layer = PCB->LayerGroups.Entries[LayerGroup][entry];

      /* handle normal layers */
      if (layer < max_layer)
        {
          PolygonTypePtr polygon;

          /* find the first line that touches coordinates */

          if (setjmp (info.env) == 0)
            r_search (LAYER_PTR (layer)->line_tree, (BoxType *) & info.line,
                      NULL, LOT_Linecallback, &info);
          else
            return (True);
          if (setjmp (info.env) == 0)
            r_search (LAYER_PTR (layer)->arc_tree, (BoxType *) & info.line,
                      NULL, LOT_Arccallback, &info);
          else
            return (True);

          /* now check all polygons */
          i = 0;
          polygon = PCB->Data->Layer[layer].Polygon;
          for (; i < PCB->Data->Layer[layer].PolygonN; i++, polygon++)
            if (!TEST_FLAG (TheFlag, polygon)
                && IsLineInPolygon (Line, polygon))
              return (True);
        }
      else
        {
          /* handle special 'pad' layers */
          info.layer = layer - max_layer;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pad_tree, &info.line.BoundingBox, NULL,
                      LOT_Padcallback, &info);
          else
            return True;
        }
    }
  return (False);
}

struct rat_info
{
  Cardinal layer;
  PointTypePtr Point;
  jmp_buf env;
};

static int
LOCtoRat_callback (const BoxType * b, void *cl)
{
  LineTypePtr line = (LineTypePtr) b;
  struct rat_info *i = (struct rat_info *) cl;

  if (!TEST_FLAG (TheFlag, line) &&
      ((line->Point1.X == i->Point->X &&
        line->Point1.Y == i->Point->Y) ||
       (line->Point2.X == i->Point->X && line->Point2.Y == i->Point->Y)))
    {
      if (ADD_LINE_TO_LIST (i->layer, line))
        longjmp (i->env, 1);
    }
  return 0;
}
static int
PolygonToRat_callback (const BoxType * b, void *cl)
{
  PolygonTypePtr polygon = (PolygonTypePtr) b;
  struct rat_info *i = (struct rat_info *) cl;

  if (!TEST_FLAG (TheFlag, polygon) && polygon->Clipped &&
      (i->Point->X == polygon->Clipped->contours->head.point[0]) &&
      (i->Point->Y == polygon->Clipped->contours->head.point[1]))
    {
      if (ADD_POLYGON_TO_LIST (i->layer, polygon))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPad_callback (const BoxType * b, void *cl)
{
  PadTypePtr pad = (PadTypePtr) b;
  struct rat_info *i = (struct rat_info *) cl;

  if (!TEST_FLAG (TheFlag, pad) && i->layer ==
      (TEST_FLAG (ONSOLDERFLAG, pad) ? SOLDER_LAYER : COMPONENT_LAYER)
      && (((pad->Point1.X == i->Point->X && pad->Point1.Y == i->Point->Y)) ||
          ((pad->Point2.X == i->Point->X && pad->Point2.Y == i->Point->Y)))
      && ADD_PAD_TO_LIST (i->layer, pad))
    longjmp (i->env, 1);
  return 0;
}

/* ---------------------------------------------------------------------------
 * searches all LOs that are connected to the given rat-line on the given
 * layergroup. All found connections are added to the list
 *
 * the notation that is used is:
 * Xij means Xj at line i
 */
static Boolean
LookupLOConnectionsToRatEnd (PointTypePtr Point, Cardinal LayerGroup)
{
  Cardinal entry;
  struct rat_info info;

  info.Point = Point;
  /* loop over all layers of this group */
  for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++)
    {
      Cardinal layer;

      layer = PCB->LayerGroups.Entries[LayerGroup][entry];
      /* handle normal layers 
         rats don't ever touch
         arcs by definition
       */

      if (layer < max_layer)
        {
          info.layer = layer;
          if (setjmp (info.env) == 0)
            r_search_pt (LAYER_PTR (layer)->line_tree, Point, 1, NULL,
                      LOCtoRat_callback, &info);
          else
            return True;
          if (setjmp (info.env) == 0)
            r_search_pt (LAYER_PTR (layer)->polygon_tree, Point, 1,
                      NULL, PolygonToRat_callback, &info);
        }
      else
        {
          /* handle special 'pad' layers */
          info.layer = layer - max_layer;
          if (setjmp (info.env) == 0)
            r_search_pt (PCB->Data->pad_tree, Point, 1, NULL,
                      LOCtoPad_callback, &info);
          else
            return True;
        }
    }
  return (False);
}

static int
LOCtoPadLine_callback (const BoxType * b, void *cl)
{
  LineTypePtr line = (LineTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (TheFlag, line) && LinePadIntersect (line, &i->pad))
    {
      if (ADD_LINE_TO_LIST (i->layer, line))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPadArc_callback (const BoxType * b, void *cl)
{
  ArcTypePtr arc = (ArcTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!arc->Thickness)
    return 0;
  if (!TEST_FLAG (TheFlag, arc) && ArcPadIntersect (arc, &i->pad))
    {
      if (ADD_ARC_TO_LIST (i->layer, arc))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPadPoly_callback (const BoxType * b, void *cl)
{
  PolygonTypePtr polygon = (PolygonTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;


  if (!TEST_FLAG (TheFlag, polygon) && !TEST_FLAG (CLEARPOLYFLAG, polygon))
    {
      if (IsPadInPolygon (&i->pad, polygon) &&
          ADD_POLYGON_TO_LIST (i->layer, polygon))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPadRat_callback (const BoxType * b, void *cl)
{
  RatTypePtr rat = (RatTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (TheFlag, rat))
    {
      if (rat->group1 == i->layer &&
          ((rat->Point1.X == i->pad.Point1.X
            && rat->Point1.Y == i->pad.Point1.Y)
           || (rat->Point1.X == i->pad.Point2.X
               && rat->Point1.Y == i->pad.Point2.Y)))
        {
          if (ADD_RAT_TO_LIST (rat))
            longjmp (i->env, 1);
        }
      else if (rat->group2 == i->layer &&
               ((rat->Point2.X == i->pad.Point1.X
                 && rat->Point2.Y == i->pad.Point1.Y)
                || (rat->Point2.X == i->pad.Point2.X
                    && rat->Point2.Y == i->pad.Point2.Y)))
        {
          if (ADD_RAT_TO_LIST (rat))
            longjmp (i->env, 1);
        }
    }
  return 0;
}

static int
LOCtoPadPad_callback (const BoxType * b, void *cl)
{
  PadTypePtr pad = (PadTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (TheFlag, pad) && i->layer ==
      (TEST_FLAG (ONSOLDERFLAG, pad) ? SOLDER_LAYER : COMPONENT_LAYER)
      && PadPadIntersect (pad, &i->pad) && ADD_PAD_TO_LIST (i->layer, pad))
    longjmp (i->env, 1);
  return 0;
}

/* ---------------------------------------------------------------------------
 * searches all LOs that are connected to the given pad on the given
 * layergroup. All found connections are added to the list
 */
static Boolean
LookupLOConnectionsToPad (PadTypePtr Pad, Cardinal LayerGroup)
{
  Cardinal entry;
  struct lo_info info;

  if (!TEST_FLAG (SQUAREFLAG, Pad))
    return (LookupLOConnectionsToLine ((LineTypePtr) Pad, LayerGroup, False));

  info.pad = *Pad;
  EXPAND_BOUNDS (&info.pad);
  /* add the new rat lines */
  info.layer = LayerGroup;
  if (setjmp (info.env) == 0)
    r_search (PCB->Data->rat_tree, &info.pad.BoundingBox, NULL,
              LOCtoPadRat_callback, &info);
  else
    return True;

  /* loop over all layers of the group */
  for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++)
    {
      Cardinal layer;

      layer = PCB->LayerGroups.Entries[LayerGroup][entry];
      /* handle normal layers */
      if (layer < max_layer)
        {
          info.layer = layer;
          /* add lines */
          if (setjmp (info.env) == 0)
            r_search (LAYER_PTR (layer)->line_tree, &info.pad.BoundingBox,
                      NULL, LOCtoPadLine_callback, &info);
          else
            return True;
          /* add arcs */
          if (setjmp (info.env) == 0)
            r_search (LAYER_PTR (layer)->arc_tree, &info.pad.BoundingBox,
                      NULL, LOCtoPadArc_callback, &info);
          else
            return True;
          /* add polygons */
          if (setjmp (info.env) == 0)
            r_search (LAYER_PTR (layer)->polygon_tree, &info.pad.BoundingBox,
                      NULL, LOCtoPadPoly_callback, &info);
          else
            return True;
        }
      else
        {
          /* handle special 'pad' layers */
          info.layer = layer - max_layer;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pad_tree, (BoxType *) & info.pad, NULL,
                      LOCtoPadPad_callback, &info);
          else
            return True;
        }

    }
  return (False);
}

static int
LOCtoPolyLine_callback (const BoxType * b, void *cl)
{
  LineTypePtr line = (LineTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (TheFlag, line) && IsLineInPolygon (line, &i->polygon))
    {
      if (ADD_LINE_TO_LIST (i->layer, line))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPolyArc_callback (const BoxType * b, void *cl)
{
  ArcTypePtr arc = (ArcTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!arc->Thickness)
    return 0;
  if (!TEST_FLAG (TheFlag, arc) && IsArcInPolygon (arc, &i->polygon))
    {
      if (ADD_ARC_TO_LIST (i->layer, arc))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPolyPad_callback (const BoxType * b, void *cl)
{
  PadTypePtr pad = (PadTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (TheFlag, pad) && i->layer ==
      (TEST_FLAG (ONSOLDERFLAG, pad) ? SOLDER_LAYER : COMPONENT_LAYER)
      && IsPadInPolygon (pad, &i->polygon))
    {
      if (ADD_PAD_TO_LIST (i->layer, pad))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPolyRat_callback (const BoxType * b, void *cl)
{
  RatTypePtr rat = (RatTypePtr) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (TheFlag, rat))
    {
      if ((rat->Point1.X == (i->polygon.Clipped->contours->head.point[0]) &&
           rat->Point1.Y == (i->polygon.Clipped->contours->head.point[1]) &&
           rat->group1 == i->layer) ||
          (rat->Point2.X == (i->polygon.Clipped->contours->head.point[0]) &&
           rat->Point2.Y == (i->polygon.Clipped->contours->head.point[1]) &&
           rat->group2 == i->layer))
        if (ADD_RAT_TO_LIST (rat))
          longjmp (i->env, 1);
    }
  return 0;
}


/* ---------------------------------------------------------------------------
 * looks up LOs that are connected to the given polygon
 * on the given layergroup. All found connections are added to the list
 */
static Boolean
LookupLOConnectionsToPolygon (PolygonTypePtr Polygon, Cardinal LayerGroup)
{
  Cardinal entry;
  struct lo_info info;

  if (!Polygon->Clipped)
    return False;
  info.polygon = *Polygon;
  EXPAND_BOUNDS (&info.polygon);
  info.layer = LayerGroup;
  /* check rats */
  if (setjmp (info.env) == 0)
    r_search (PCB->Data->rat_tree, (BoxType *) & info.polygon, NULL,
              LOCtoPolyRat_callback, &info);
  else
    return True;
/* loop over all layers of the group */
  for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++)
    {
      Cardinal layer, i;

      layer = PCB->LayerGroups.Entries[LayerGroup][entry];

      /* handle normal layers */
      if (layer < max_layer)
        {
          PolygonTypePtr polygon;

          /* check all polygons */

          polygon = PCB->Data->Layer[layer].Polygon;
          for (i = 0; i < PCB->Data->Layer[layer].PolygonN; i++, polygon++)
            if (!TEST_FLAG (TheFlag, polygon)
                && IsPolygonInPolygon (polygon, Polygon)
                && ADD_POLYGON_TO_LIST (layer, polygon))
              return True;

          info.layer = layer;
          /* check all lines */
          if (setjmp (info.env) == 0)
            r_search (LAYER_PTR (layer)->line_tree,
                      (BoxType *) & info.polygon, NULL,
                      LOCtoPolyLine_callback, &info);
          else
            return True;
          /* check all arcs */
          if (setjmp (info.env) == 0)
            r_search (LAYER_PTR (layer)->arc_tree, (BoxType *) & info.polygon,
                      NULL, LOCtoPolyArc_callback, &info);
          else
            return True;
        }
      else
        {
          if (!TEST_FLAG (CLEARPOLYFLAG, Polygon))
            {
              info.layer = layer - max_layer;
              if (setjmp (info.env) == 0)
                r_search (PCB->Data->pad_tree, (BoxType *) & info.polygon,
                          NULL, LOCtoPolyPad_callback, &info);
              else
                return True;
            }
        }
    }
  return (False);
}

/* ---------------------------------------------------------------------------
 * checks if an arc has a connection to a polygon
 *
 * - first check if the arc can intersect with the polygon by
 *   evaluating the bounding boxes
 * - check the two end points of the arc. If none of them matches
 * - check all segments of the polygon against the arc.
 */
Boolean
IsArcInPolygon (ArcTypePtr Arc, PolygonTypePtr Polygon)
{
  BoxTypePtr Box = (BoxType *) Arc;

  /* arcs with clearance never touch polys */
  if (TEST_FLAG (CLEARPOLYFLAG, Polygon) && TEST_FLAG (CLEARLINEFLAG, Arc))
    return False;
  if (!Polygon->Clipped)
    return False;
  if (Box->X1 <= Polygon->Clipped->contours->xmax + Bloat
      && Box->X2 >= Polygon->Clipped->contours->xmin - Bloat
      && Box->Y1 <= Polygon->Clipped->contours->ymax + Bloat
      && Box->Y2 >= Polygon->Clipped->contours->ymin - Bloat)
    {
      POLYAREA *ap;

      if (!(ap = ArcPoly (Arc, Arc->Thickness + Bloat)))
        return False;           /* error */
      return isects (ap, Polygon, True);
    }
  return False;
}

/* ---------------------------------------------------------------------------
 * checks if a line has a connection to a polygon
 *
 * - first check if the line can intersect with the polygon by
 *   evaluating the bounding boxes
 * - check the two end points of the line. If none of them matches
 * - check all segments of the polygon against the line.
 */
Boolean
IsLineInPolygon (LineTypePtr Line, PolygonTypePtr Polygon)
{
  BoxTypePtr Box = (BoxType *) Line;
  POLYAREA *lp;

  /* lines with clearance never touch polygons */
  if (TEST_FLAG (CLEARPOLYFLAG, Polygon) && TEST_FLAG (CLEARLINEFLAG, Line))
    return False;
  if (!Polygon->Clipped)
    return False;
  if (TEST_FLAG(SQUAREFLAG,Line)&&(Line->Point1.X==Line->Point2.X||Line->Point1.Y==Line->Point2.Y))
     {
       BDimension wid = (Line->Thickness + Bloat + 1) / 2;
       LocationType x1, x2, y1, y2;

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
      return isects (lp, Polygon, True);
    }
  return False;
}

/* ---------------------------------------------------------------------------
 * checks if a pad connects to a non-clearing polygon
 *
 * The polygon is assumed to already have been proven non-clearing
 */
Boolean
IsPadInPolygon (PadTypePtr pad, PolygonTypePtr polygon)
{
    return IsLineInPolygon ((LineTypePtr) pad, polygon);
}

/* ---------------------------------------------------------------------------
 * checks if a polygon has a connection to a second one
 *
 * First check all points out of P1 against P2 and vice versa.
 * If both fail check all lines of P1 against the ones of P2
 */
Boolean
IsPolygonInPolygon (PolygonTypePtr P1, PolygonTypePtr P2)
{
  if (!P1->Clipped || !P2->Clipped)
    return False;
  assert (P1->Clipped->contours);
  assert (P2->Clipped->contours);
  /* first check if both bounding boxes intersect */
  if (P1->Clipped->contours->xmin - Bloat <= P2->Clipped->contours->xmax &&
      P1->Clipped->contours->xmax + Bloat >= P2->Clipped->contours->xmin &&
      P1->Clipped->contours->ymin - Bloat <= P2->Clipped->contours->ymax &&
      P1->Clipped->contours->ymax + Bloat >= P2->Clipped->contours->ymin)
    {
      PLINE *c;

      /* first check un-bloated case */
      if (isects (P1->Clipped, P2, False))
        return TRUE;
      if (Bloat > 0)
        {
          /* now the difficult case of bloated */
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
                  line.Thickness = 2 * Bloat;
                  line.Clearance = 0;
                  line.Flags = NoFlags ();
                  for (v = v->next; v != &c->head; v = v->next)
                    {
                      line.Point2.X = v->point[0];
                      line.Point2.Y = v->point[1];
                      SetLineBoundingBox (&line);
                      if (IsLineInPolygon (&line, P2))
                        return (True);
                      line.Point1.X = line.Point2.X;
                      line.Point1.Y = line.Point2.Y;
                    }
                }
            }
        }
    }
  return (False);
}

/* ---------------------------------------------------------------------------
 * writes the several names of an element to a file
 */
static void
PrintElementNameList (ElementTypePtr Element, FILE * FP)
{
  static DynamicStringType cname, pname, vname;

  CreateQuotedString (&cname, EMPTY (DESCRIPTION_NAME (Element)));
  CreateQuotedString (&pname, EMPTY (NAMEONPCB_NAME (Element)));
  CreateQuotedString (&vname, EMPTY (VALUE_NAME (Element)));
  fprintf (FP, "(%s %s %s)\n", cname.Data, pname.Data, vname.Data);
}

/* ---------------------------------------------------------------------------
 * writes the several names of an element to a file
 */
static void
PrintConnectionElementName (ElementTypePtr Element, FILE * FP)
{
  fputs ("Element", FP);
  PrintElementNameList (Element, FP);
  fputs ("{\n", FP);
}

/* ---------------------------------------------------------------------------
 * prints one {pin,pad,via}/element entry of connection lists
 */
static void
PrintConnectionListEntry (char *ObjName, ElementTypePtr Element,
                          Boolean FirstOne, FILE * FP)
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

/* ---------------------------------------------------------------------------
 * prints all found connections of a pads to file FP
 * the connections are stacked in 'PadList'
 */
static void
PrintPadConnections (Cardinal Layer, FILE * FP, Boolean IsFirst)
{
  Cardinal i;
  PadTypePtr ptr;

  if (!PadList[Layer].Number)
    return;

  /* the starting pad */
  if (IsFirst)
    {
      ptr = PADLIST_ENTRY (Layer, 0);
      if (ptr != NULL)
        PrintConnectionListEntry (UNKNOWN (ptr->Name), NULL, True, FP);
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
        PrintConnectionListEntry (EMPTY (ptr->Name), ptr->Element, False, FP);
      else
        printf ("Skipping NULL ptr in 2nd part of PrintPadConnections\n");
    }
}

/* ---------------------------------------------------------------------------
 * prints all found connections of a pin to file FP
 * the connections are stacked in 'PVList'
 */
static void
PrintPinConnections (FILE * FP, Boolean IsFirst)
{
  Cardinal i;
  PinTypePtr pv;

  if (!PVList.Number)
    return;

  if (IsFirst)
    {
      /* the starting pin */
      pv = PVLIST_ENTRY (0);
      PrintConnectionListEntry (EMPTY (pv->Name), NULL, True, FP);
    }

  /* we maybe have to start with i=1 if we are handling the
   * starting-pin itself
   */
  for (i = IsFirst ? 1 : 0; i < PVList.Number; i++)
    {
      /* get the elements name or assume that its a via */
      pv = PVLIST_ENTRY (i);
      PrintConnectionListEntry (EMPTY (pv->Name), pv->Element, False, FP);
    }
}

/* ---------------------------------------------------------------------------
 * checks if all lists of new objects are handled
 */
static Boolean
ListsEmpty (Boolean AndRats)
{
  Boolean empty;
  int i;

  empty = (PVList.Location >= PVList.Number);
  if (AndRats)
    empty = empty && (RatList.Location >= RatList.Number);
  for (i = 0; i < max_layer && empty; i++)
    empty = empty && LineList[i].Location >= LineList[i].Number
      && ArcList[i].Location >= ArcList[i].Number
      && PolygonList[i].Location >= PolygonList[i].Number;
  return (empty);
}

/* ---------------------------------------------------------------------------
 * loops till no more connections are found 
 */
static Boolean
DoIt (Boolean AndRats, Boolean AndDraw)
{
  Boolean new = False;
  do
    {
      /* lookup connections; these are the steps (2) to (4)
       * from the description
       */
      new = LookupPVConnectionsToPVList ();
      if (!new)
        new = LookupLOConnectionsToPVList (AndRats);
      if (!new)
        new = LookupLOConnectionsToLOList (AndRats);
      if (!new)
        new = LookupPVConnectionsToLOList (AndRats);
      if (AndDraw)
        DrawNewConnections ();
    }
  while (!new && !ListsEmpty (AndRats));
  if (AndDraw)
    Draw ();
  return (new);
}

/* returns True if nothing un-found touches the passed line
 * returns False if it would touch something not yet found
 * doesn't include rat-lines in the search
 */

Boolean
lineClear (LineTypePtr line, Cardinal group)
{
  if (LOTouchesLine (line, group))
    return (False);
  if (PVTouchesLine (line))
    return (False);
  return (True);
}

/* ---------------------------------------------------------------------------
 * prints all unused pins of an element to file FP
 */
static Boolean
PrintAndSelectUnusedPinsAndPadsOfElement (ElementTypePtr Element, FILE * FP)
{
  Boolean first = True;
  Cardinal number;
  static DynamicStringType oname;

  /* check all pins in element */

  PIN_LOOP (Element);
  {
    if (!TEST_FLAG (HOLEFLAG, pin))
      {
        /* pin might have bee checked before, add to list if not */
        if (!TEST_FLAG (TheFlag, pin) && FP)
          {
            int i;
            if (ADD_PV_TO_LIST (pin))
              return True;
            DoIt (True, True);
            number = PadList[COMPONENT_LAYER].Number
              + PadList[SOLDER_LAYER].Number + PVList.Number;
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
                    first = False;
                  }

                /* write name to list and draw selected object */
                CreateQuotedString (&oname, EMPTY (pin->Name));
                fprintf (FP, "\t%s\n", oname.Data);
                SET_FLAG (SELECTEDFLAG, pin);
                DrawPin (pin, 0);
              }

            /* reset found objects for the next pin */
            if (PrepareNextLoop (FP))
              return (True);
          }
      }
  }
  END_LOOP;

  /* check all pads in element */
  PAD_LOOP (Element);
  {
    /* lookup pad in list */
    /* pad might has bee checked before, add to list if not */
    if (!TEST_FLAG (TheFlag, pad) && FP)
      {
        int i;
        if (ADD_PAD_TO_LIST (TEST_FLAG (ONSOLDERFLAG, pad)
                             ? SOLDER_LAYER : COMPONENT_LAYER, pad))
          return True;
        DoIt (True, True);
        number = PadList[COMPONENT_LAYER].Number
          + PadList[SOLDER_LAYER].Number + PVList.Number;
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
                first = False;
              }

            /* write name to list and draw selected object */
            CreateQuotedString (&oname, EMPTY (pad->Name));
            fprintf (FP, "\t%s\n", oname.Data);
            SET_FLAG (SELECTEDFLAG, pad);
            DrawPad (pad, 0);
          }

        /* reset found objects for the next pin */
        if (PrepareNextLoop (FP))
          return (True);
      }
  }
  END_LOOP;

  /* print separator if element has unused pins or pads */
  if (!first)
    {
      fputs ("}\n\n", FP);
      SEPARATE (FP);
    }
  return (False);
}

/* ---------------------------------------------------------------------------
 * resets some flags for looking up the next pin/pad
 */
static Boolean
PrepareNextLoop (FILE * FP)
{
  Cardinal layer;

  /* reset found LOs for the next pin */
  for (layer = 0; layer < max_layer; layer++)
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

  return (False);
}

/* ---------------------------------------------------------------------------
 * finds all connections to the pins of the passed element.
 * The result is written to filedescriptor 'FP'
 * Returns True if operation was aborted
 */
static Boolean
PrintElementConnections (ElementTypePtr Element, FILE * FP, Boolean AndDraw)
{
  PrintConnectionElementName (Element, FP);

  /* check all pins in element */
  PIN_LOOP (Element);
  {
    /* pin might have been checked before, add to list if not */
    if (TEST_FLAG (TheFlag, pin))
      {
        PrintConnectionListEntry (EMPTY (pin->Name), NULL, True, FP);
        fputs ("\t\t__CHECKED_BEFORE__\n\t}\n", FP);
        continue;
      }
    if (ADD_PV_TO_LIST (pin))
      return True;
    DoIt (True, AndDraw);
    /* printout all found connections */
    PrintPinConnections (FP, True);
    PrintPadConnections (COMPONENT_LAYER, FP, False);
    PrintPadConnections (SOLDER_LAYER, FP, False);
    fputs ("\t}\n", FP);
    if (PrepareNextLoop (FP))
      return (True);
  }
  END_LOOP;

  /* check all pads in element */
  PAD_LOOP (Element);
  {
    Cardinal layer;
    /* pad might have been checked before, add to list if not */
    if (TEST_FLAG (TheFlag, pad))
      {
        PrintConnectionListEntry (EMPTY (pad->Name), NULL, True, FP);
        fputs ("\t\t__CHECKED_BEFORE__\n\t}\n", FP);
        continue;
      }
    layer = TEST_FLAG (ONSOLDERFLAG, pad) ? SOLDER_LAYER : COMPONENT_LAYER;
    if (ADD_PAD_TO_LIST (layer, pad))
      return True;
    DoIt (True, AndDraw);
    /* print all found connections */
    PrintPadConnections (layer, FP, True);
    PrintPadConnections (layer ==
                         (COMPONENT_LAYER ? SOLDER_LAYER : COMPONENT_LAYER),
                         FP, False);
    PrintPinConnections (FP, False);
    fputs ("\t}\n", FP);
    if (PrepareNextLoop (FP))
      return (True);
  }
  END_LOOP;
  fputs ("}\n\n", FP);
  return (False);
}

/* ---------------------------------------------------------------------------
 * draws all new connections which have been found since the
 * routine was called the last time
 */
static void
DrawNewConnections (void)
{
  int i;
  Cardinal position;

  /* decrement 'i' to keep layerstack order */
  for (i = max_layer - 1; i != -1; i--)
    {
      Cardinal layer = LayerStack[i];

      if (PCB->Data->Layer[layer].On)
        {
          /* draw all new lines */
          position = LineList[layer].DrawLocation;
          for (; position < LineList[layer].Number; position++)
            DrawLine (LAYER_PTR (layer), LINELIST_ENTRY (layer, position), 0);
          LineList[layer].DrawLocation = LineList[layer].Number;

          /* draw all new arcs */
          position = ArcList[layer].DrawLocation;
          for (; position < ArcList[layer].Number; position++)
            DrawArc (LAYER_PTR (layer), ARCLIST_ENTRY (layer, position), 0);
          ArcList[layer].DrawLocation = ArcList[layer].Number;

          /* draw all new polygons */
          position = PolygonList[layer].DrawLocation;
          for (; position < PolygonList[layer].Number; position++)
            DrawPolygon
              (LAYER_PTR (layer), POLYGONLIST_ENTRY (layer, position), 0);
          PolygonList[layer].DrawLocation = PolygonList[layer].Number;
        }
    }

  /* draw all new pads */
  if (PCB->PinOn)
    for (i = 0; i < 2; i++)
      {
        position = PadList[i].DrawLocation;

        for (; position < PadList[i].Number; position++)
          DrawPad (PADLIST_ENTRY (i, position), 0);
        PadList[i].DrawLocation = PadList[i].Number;
      }

  /* draw all new PVs; 'PVList' holds a list of pointers to the
   * sorted array pointers to PV data
   */
  while (PVList.DrawLocation < PVList.Number)
    {
      PinTypePtr pv = PVLIST_ENTRY (PVList.DrawLocation);

      if (TEST_FLAG (PINFLAG, pv))
        {
          if (PCB->PinOn)
            DrawPin (pv, 0);
        }
      else if (PCB->ViaOn)
        DrawVia (pv, 0);
      PVList.DrawLocation++;
    }
  /* draw the new rat-lines */
  if (PCB->RatOn)
    {
      position = RatList.DrawLocation;
      for (; position < RatList.Number; position++)
        DrawRat (RATLIST_ENTRY (position), 0);
      RatList.DrawLocation = RatList.Number;
    }
}

/* ---------------------------------------------------------------------------
 * find all connections to pins within one element
 */
void
LookupElementConnections (ElementTypePtr Element, FILE * FP)
{
  /* reset all currently marked connections */
  User = True;
  TheFlag = FOUNDFLAG;
  ResetConnections (True);
  InitConnectionLookup ();
  PrintElementConnections (Element, FP, True);
  SetChangedFlag (True);
  if (Settings.RingBellWhenFinished)
    gui->beep ();
  FreeConnectionLookupMemory ();
  IncrementUndoSerialNumber ();
  User = False;
  Draw ();
}

/* ---------------------------------------------------------------------------
 * find all connections to pins of all element
 */
void
LookupConnectionsToAllElements (FILE * FP)
{
  /* reset all currently marked connections */
  User = False;
  TheFlag = FOUNDFLAG;
  ResetConnections (False);
  InitConnectionLookup ();

  ELEMENT_LOOP (PCB->Data);
  {
    /* break if abort dialog returned True */
    if (PrintElementConnections (element, FP, False))
      break;
    SEPARATE (FP);
    if (Settings.ResetAfterElement && n != 1)
      ResetConnections (False);
  }
  END_LOOP;
  if (Settings.RingBellWhenFinished)
    gui->beep ();
  ResetConnections (False);
  FreeConnectionLookupMemory ();
  ClearAndRedrawOutput ();
}

/*---------------------------------------------------------------------------
 * add the starting object to the list of found objects
 */
static Boolean
ListStart (int type, void *ptr1, void *ptr2, void *ptr3)
{
  DumpList ();
  switch (type)
    {
    case PIN_TYPE:
    case VIA_TYPE:
      {
        if (ADD_PV_TO_LIST ((PinTypePtr) ptr2))
          return True;
        break;
      }

    case RATLINE_TYPE:
      {
        if (ADD_RAT_TO_LIST ((RatTypePtr) ptr1))
          return True;
        break;
      }

    case LINE_TYPE:
      {
        int layer = GetLayerNumber (PCB->Data,
                                    (LayerTypePtr) ptr1);

        if (ADD_LINE_TO_LIST (layer, (LineTypePtr) ptr2))
          return True;
        break;
      }

    case ARC_TYPE:
      {
        int layer = GetLayerNumber (PCB->Data,
                                    (LayerTypePtr) ptr1);

        if (ADD_ARC_TO_LIST (layer, (ArcTypePtr) ptr2))
          return True;
        break;
      }

    case POLYGON_TYPE:
      {
        int layer = GetLayerNumber (PCB->Data,
                                    (LayerTypePtr) ptr1);

        if (ADD_POLYGON_TO_LIST (layer, (PolygonTypePtr) ptr2))
          return True;
        break;
      }

    case PAD_TYPE:
      {
        PadTypePtr pad = (PadTypePtr) ptr2;
        if (ADD_PAD_TO_LIST
            (TEST_FLAG
             (ONSOLDERFLAG, pad) ? SOLDER_LAYER : COMPONENT_LAYER, pad))
          return True;
        break;
      }
    }
  return (False);
}


/* ---------------------------------------------------------------------------
 * looks up all connections from the object at the given coordinates
 * the TheFlag (normally 'FOUNDFLAG') is set for all objects found
 * the objects are re-drawn if AndDraw is true
 * also the action is marked as undoable if AndDraw is true
 */
void
LookupConnection (LocationType X, LocationType Y, Boolean AndDraw,
                  BDimension Range, int which_flag)
{
  void *ptr1, *ptr2, *ptr3;
  char *name;
  int type;

  /* check if there are any pins or pads at that position */


  type
    = SearchObjectByLocation (LOOKUP_FIRST, &ptr1, &ptr2, &ptr3, X, Y, Range);
  if (type == NO_TYPE)
    {
      type
        =
        SearchObjectByLocation
        (LOOKUP_MORE, &ptr1, &ptr2, &ptr3, X, Y, Range);
      if (type == NO_TYPE)
        return;
      if (type & SILK_TYPE)
        {
          int laynum = GetLayerNumber (PCB->Data,
                                       (LayerTypePtr) ptr1);

          /* don't mess with silk objects! */
          if (laynum >= max_layer)
            return;
        }
    }
  else
    {
      name = ConnectionName (type, ptr1, ptr2);
      hid_actionl ("NetlistShow", name, NULL);
    }

  TheFlag = which_flag;
  User = AndDraw;
  InitConnectionLookup ();

  /* now add the object to the appropriate list and start scanning
   * This is step (1) from the description
   */
  ListStart (type, ptr1, ptr2, ptr3);
  DoIt (True, AndDraw);
  if (User)
    IncrementUndoSerialNumber ();
  User = False;

  /* we are done */
  if (AndDraw)
    Draw ();
  if (AndDraw && Settings.RingBellWhenFinished)
    gui->beep ();
  FreeConnectionLookupMemory ();
}

/* ---------------------------------------------------------------------------
 * find connections for rats nesting
 * assumes InitConnectionLookup() has already been done
 */
void
  RatFindHook
  (int type, void *ptr1, void *ptr2, void *ptr3, Boolean undo,
   Boolean AndRats)
{
  User = undo;
  DumpList ();
  ListStart (type, ptr1, ptr2, ptr3);
  DoIt (AndRats, False);
  User = False;
}

/* ---------------------------------------------------------------------------
 * find all unused pins of all element
 */
void
LookupUnusedPins (FILE * FP)
{
  /* reset all currently marked connections */
  User = True;
  SaveUndoSerialNumber ();
  ResetConnections (True);
  RestoreUndoSerialNumber ();
  InitConnectionLookup ();

  ELEMENT_LOOP (PCB->Data);
  {
    /* break if abort dialog returned True;
     * passing NULL as filedescriptor discards the normal output
     */
    if (PrintAndSelectUnusedPinsAndPadsOfElement (element, FP))
      break;
  }
  END_LOOP;

  if (Settings.RingBellWhenFinished)
    gui->beep ();
  FreeConnectionLookupMemory ();
  IncrementUndoSerialNumber ();
  User = False;
  Draw ();
}

/* ---------------------------------------------------------------------------
 * resets all used flags of pins and vias
 */
void
ResetFoundPinsViasAndPads (Boolean AndDraw)
{
  Boolean change = False;


  VIA_LOOP (PCB->Data);
  {
    if (TEST_FLAG (TheFlag, via))
      {
        if (AndDraw)
          AddObjectToFlagUndoList (VIA_TYPE, via, via, via);
        CLEAR_FLAG (TheFlag, via);
        if (AndDraw)
          DrawVia (via, 0);
        change = True;
      }
  }
  END_LOOP;
  ELEMENT_LOOP (PCB->Data);
  {
    PIN_LOOP (element);
    {
      if (TEST_FLAG (TheFlag, pin))
        {
          if (AndDraw)
            AddObjectToFlagUndoList (PIN_TYPE, element, pin, pin);
          CLEAR_FLAG (TheFlag, pin);
          if (AndDraw)
            DrawPin (pin, 0);
          change = True;
        }
    }
    END_LOOP;
    PAD_LOOP (element);
    {
      if (TEST_FLAG (TheFlag, pad))
        {
          if (AndDraw)
            AddObjectToFlagUndoList (PAD_TYPE, element, pad, pad);
          CLEAR_FLAG (TheFlag, pad);
          if (AndDraw)
            DrawPad (pad, 0);
          change = True;
        }
    }
    END_LOOP;
  }
  END_LOOP;
  if (change)
    {
      SetChangedFlag (True);
      if (AndDraw)
        {
          IncrementUndoSerialNumber ();
          Draw ();
        }
    }
}

/* ---------------------------------------------------------------------------
 * resets all used flags of LOs
 */
void
ResetFoundLinesAndPolygons (Boolean AndDraw)
{
  Boolean change = False;


  RAT_LOOP (PCB->Data);
  {
    if (TEST_FLAG (TheFlag, line))
      {
        if (AndDraw)
          AddObjectToFlagUndoList (RATLINE_TYPE, line, line, line);
        CLEAR_FLAG (TheFlag, line);
        if (AndDraw)
          DrawRat (line, 0);
        change = True;
      }
  }
  END_LOOP;
  COPPERLINE_LOOP (PCB->Data);
  {
    if (TEST_FLAG (TheFlag, line))
      {
        if (AndDraw)
          AddObjectToFlagUndoList (LINE_TYPE, layer, line, line);
        CLEAR_FLAG (TheFlag, line);
        if (AndDraw)
          DrawLine (layer, line, 0);
        change = True;
      }
  }
  ENDALL_LOOP;
  COPPERARC_LOOP (PCB->Data);
  {
    if (TEST_FLAG (TheFlag, arc))
      {
        if (AndDraw)
          AddObjectToFlagUndoList (ARC_TYPE, layer, arc, arc);
        CLEAR_FLAG (TheFlag, arc);
        if (AndDraw)
          DrawArc (layer, arc, 0);
        change = True;
      }
  }
  ENDALL_LOOP;
  COPPERPOLYGON_LOOP (PCB->Data);
  {
    if (TEST_FLAG (TheFlag, polygon))
      {
        if (AndDraw)
          AddObjectToFlagUndoList (POLYGON_TYPE, layer, polygon, polygon);
        CLEAR_FLAG (TheFlag, polygon);
        if (AndDraw)
          DrawPolygon (layer, polygon, 0);
        change = True;
      }
  }
  ENDALL_LOOP;
  if (change)
    {
      SetChangedFlag (True);
      if (AndDraw)
        {
          IncrementUndoSerialNumber ();
          Draw ();
        }
    }
}

/* ---------------------------------------------------------------------------
 * resets all found connections
 */
static void
ResetConnections (Boolean AndDraw)
{
  if (AndDraw)
    SaveUndoSerialNumber ();
  ResetFoundPinsViasAndPads (AndDraw);
  if (AndDraw)
    RestoreUndoSerialNumber ();
  ResetFoundLinesAndPolygons (AndDraw);
}

/*----------------------------------------------------------------------------
 * Dumps the list contents
 */
static void
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

  for (i = 0; i < max_layer; i++)
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

/*-----------------------------------------------------------------------------
 * Check for DRC violations on a single net starting from the pad or pin
 * sees if the connectivity changes when everything is bloated, or shrunk
 */
static Boolean
DRCFind (int What, void *ptr1, void *ptr2, void *ptr3)
{
  if (PCB->Shrink != 0)
    {
      Bloat = -PCB->Shrink;
      fBloat = (float) -PCB->Shrink;
      TheFlag = DRCFLAG | SELECTEDFLAG;
      ListStart (What, ptr1, ptr2, ptr3);
      DoIt (True, False);
      /* ok now the shrunk net has the SELECTEDFLAG set */
      DumpList ();
      TheFlag = FOUNDFLAG;
      ListStart (What, ptr1, ptr2, ptr3);
      Bloat = 0;
      fBloat = 0.0;
      drc = True;               /* abort the search if we find anything not already found */
      if (DoIt (True, False))
        {
          DumpList ();
          Message
            (_
             ("WARNING!!  Design Rule Error - potential for broken trace!\n"));
          /* make the flag changes undoable */
          TheFlag = FOUNDFLAG | SELECTEDFLAG;
          ResetConnections (False);
          User = True;
          drc = False;
          Bloat = -PCB->Shrink;
          fBloat = (float) -PCB->Shrink;
          TheFlag = SELECTEDFLAG;
          RestoreUndoSerialNumber ();
          ListStart (What, ptr1, ptr2, ptr3);
          DoIt (True, True);
          DumpList ();
          ListStart (What, ptr1, ptr2, ptr3);
          TheFlag = FOUNDFLAG;
          Bloat = 0;
          fBloat = 0.0;
          drc = True;
          DoIt (True, True);
          DumpList ();
          User = False;
          drc = False;
          drcerr_count++;
          GotoError ();
          if (!gui->confirm_dialog (DRC_CONTINUE, DRC_CANCEL, DRC_NEXT))
            return (True);
          IncrementUndoSerialNumber ();
          Undo (True);
        }
      DumpList ();
    }
  /* now check the bloated condition */
  drc = False;
  ResetConnections (False);
  TheFlag = FOUNDFLAG;
  ListStart (What, ptr1, ptr2, ptr3);
  Bloat = PCB->Bloat;
  fBloat = (float) PCB->Bloat;
  drc = True;
  while (DoIt (True, False))
    {
      DumpList ();
      Message (_("WARNING!  Design Rule error - copper areas too close!\n"));
      /* make the flag changes undoable */
      TheFlag = FOUNDFLAG | SELECTEDFLAG;
      ResetConnections (False);
      User = True;
      drc = False;
      Bloat = 0;
      fBloat = 0.0;
      RestoreUndoSerialNumber ();
      TheFlag = SELECTEDFLAG;
      ListStart (What, ptr1, ptr2, ptr3);
      DoIt (True, True);
      DumpList ();
      TheFlag = FOUNDFLAG;
      ListStart (What, ptr1, ptr2, ptr3);
      Bloat = PCB->Bloat;
      fBloat = (float) PCB->Bloat;
      drc = True;
      DoIt (True, True);
      DumpList ();
      drcerr_count++;
      GotoError ();
      User = False;
      drc = False;
      if (!gui->confirm_dialog (DRC_CONTINUE, DRC_CANCEL, DRC_NEXT))
        return (True);
      IncrementUndoSerialNumber ();
      Undo (True);
      /* highlight the rest of the encroaching net so it's not reported again */
      TheFlag |= SELECTEDFLAG;
      Bloat = 0;
      fBloat = 0.0;
      ListStart (thing_type, thing_ptr1, thing_ptr2, thing_ptr3);
      DoIt (True, True);
      DumpList ();
      drc = True;
      Bloat = PCB->Bloat;
      fBloat = (float) PCB->Bloat;
      ListStart (What, ptr1, ptr2, ptr3);
    }
  drc = False;
  DumpList ();
  TheFlag = FOUNDFLAG | SELECTEDFLAG;
  ResetConnections (False);
  return (False);
}

/*----------------------------------------------------------------------------
 * set up a temporary flag to use
 */
void
SaveFindFlag (int NewFlag)
{
  OldFlag = TheFlag;
  TheFlag = NewFlag;
}

/*----------------------------------------------------------------------------
 * restore flag
 */
void
RestoreFindFlag (void)
{
  TheFlag = OldFlag;
}

/* DRC clearance callback */

static int
drc_callback (DataTypePtr data, LayerTypePtr layer, PolygonTypePtr polygon,
              int type, void *ptr1, void *ptr2)
{
  LineTypePtr line = (LineTypePtr) ptr2;
  ArcTypePtr arc = (ArcTypePtr) ptr2;
  PinTypePtr pin = (PinTypePtr) ptr2;
  PadTypePtr pad = (PadTypePtr) ptr2;

  thing_type = type;
  thing_ptr1 = ptr1;
  thing_ptr2 = ptr2;
  thing_ptr3 = ptr2;
  switch (type)
    {
    case LINE_TYPE:
      if (line->Clearance < 2 * PCB->Bloat)
        {
          AddObjectToFlagUndoList (type, ptr1, ptr2, ptr2);
          SET_FLAG (TheFlag, line);
          Message (_("Line with insufficient clearance inside polygon\n"));
          goto doIsBad;
        }
      break;
    case ARC_TYPE:
      if (arc->Clearance < 2 * PCB->Bloat)
        {
          AddObjectToFlagUndoList (type, ptr1, ptr2, ptr2);
          SET_FLAG (TheFlag, arc);
          Message (_("Arc with insufficient clearance inside polygon\n"));
          goto doIsBad;
        }
      break;
    case PAD_TYPE:
      if (pad->Clearance < 2 * PCB->Bloat)
	if (IsPadInPolygon(pad,polygon))
	  {
	    AddObjectToFlagUndoList (type, ptr1, ptr2, ptr2);
	    SET_FLAG (TheFlag, pad);
	    Message (_("Pad with insufficient clearance inside polygon\n"));
	    goto doIsBad;
	  }
      break;
    case PIN_TYPE:
      if (pin->Clearance < 2 * PCB->Bloat)
        {
          AddObjectToFlagUndoList (type, ptr1, ptr2, ptr2);
          SET_FLAG (TheFlag, pin);
          Message (_("Pin with insufficient clearance inside polygon\n"));
          goto doIsBad;
        }
      break;
    case VIA_TYPE:
      if (pin->Clearance && pin->Clearance < 2 * PCB->Bloat)
        {
          AddObjectToFlagUndoList (type, ptr1, ptr2, ptr2);
          SET_FLAG (TheFlag, pin);
          Message (_("Via with insufficient clearance inside polygon\n"));
          goto doIsBad;
        }
      break;
    default:
      Message ("hace: Bad Plow object in callback\n");
    }
  return 0;

doIsBad:
  AddObjectToFlagUndoList (POLYGON_TYPE, layer, polygon, polygon);
  SET_FLAG (FOUNDFLAG, polygon);
  DrawPolygon (layer, polygon, 0);
  DrawObject (type, ptr1, ptr2, 0);
  drcerr_count++;
  GotoError ();
  if (!gui->confirm_dialog (DRC_CONTINUE, DRC_CANCEL, DRC_NEXT))
    {
      IsBad = True;
      return 1;
    }
  IncrementUndoSerialNumber ();
  Undo (True);
  return 0;
}

/*-----------------------------------------------------------------------------
 * Check for DRC violations
 * see if the connectivity changes when everything is bloated, or shrunk
 */
int
DRCAll (void)
{
  int tmpcnt;
  int nopastecnt = 0;

  IsBad = False;
  drcerr_count = 0;
  SaveStackAndVisibility ();
  ResetStackAndVisibility ();
  hid_action ("LayersChanged");
  InitConnectionLookup ();

  TheFlag = FOUNDFLAG | DRCFLAG | SELECTEDFLAG;

  ResetConnections (True);

  User = False;

  ELEMENT_LOOP (PCB->Data);
  {
    PIN_LOOP (element);
    {
      if (!TEST_FLAG (DRCFLAG, pin)
          && DRCFind (PIN_TYPE, (void *) element, (void *) pin, (void *) pin))
        {
          IsBad = True;
          break;
        }
    }
    END_LOOP;
    if (IsBad)
      break;
    PAD_LOOP (element);
    {

      /* count up how many pads have no solderpaste openings */
      if (TEST_FLAG (NOPASTEFLAG, pad))
	nopastecnt++;

      if (!TEST_FLAG (DRCFLAG, pad)
          && DRCFind (PAD_TYPE, (void *) element, (void *) pad, (void *) pad))
        {
          IsBad = True;
          break;
        }
    }
    END_LOOP;
    if (IsBad)
      break;
  }
  END_LOOP;
  if (!IsBad)
    VIA_LOOP (PCB->Data);
  {
    if (!TEST_FLAG (DRCFLAG, via)
        && DRCFind (VIA_TYPE, (void *) via, (void *) via, (void *) via))
      {
        IsBad = True;
        break;
      }
  }
  END_LOOP;

  TheFlag = (IsBad) ? DRCFLAG : (FOUNDFLAG | DRCFLAG | SELECTEDFLAG);
  ResetConnections (False);
  TheFlag = SELECTEDFLAG;
  /* check minimum widths and polygon clearances */
  if (!IsBad)
    {
      COPPERLINE_LOOP (PCB->Data);
      {
        /* check line clearances in polygons */
        PlowsPolygon (PCB->Data, LINE_TYPE, layer, line, drc_callback);
        if (IsBad)
          break;
        if (line->Thickness < PCB->minWid)
          {
            AddObjectToFlagUndoList (LINE_TYPE, layer, line, line);
            SET_FLAG (TheFlag, line);
            Message (_("Line is too thin\n"));
            DrawLine (layer, line, 0);
            drcerr_count++;
            SetThing (LINE_TYPE, layer, line, line);
            GotoError ();
            if (!gui->confirm_dialog (DRC_CONTINUE, DRC_CANCEL, DRC_NEXT))
              {
                IsBad = True;
                break;
              }
            IncrementUndoSerialNumber ();
            Undo (False);
          }
      }
      ENDALL_LOOP;
    }
  if (!IsBad)
    {
      COPPERARC_LOOP (PCB->Data);
      {
        PlowsPolygon (PCB->Data, ARC_TYPE, layer, arc, drc_callback);
        if (IsBad)
          break;
        if (arc->Thickness < PCB->minWid)
          {
            AddObjectToFlagUndoList (ARC_TYPE, layer, arc, arc);
            SET_FLAG (TheFlag, arc);
            Message (_("Arc is too thin\n"));
            DrawArc (layer, arc, 0);
            drcerr_count++;
            SetThing (ARC_TYPE, layer, arc, arc);
            GotoError ();
            if (!gui->confirm_dialog (DRC_CONTINUE, DRC_CANCEL, DRC_NEXT))
              {
                IsBad = True;
                break;
              }
            IncrementUndoSerialNumber ();
            Undo (False);
          }
      }
      ENDALL_LOOP;
    }
  if (!IsBad)
    {
      ALLPIN_LOOP (PCB->Data);
      {
        PlowsPolygon (PCB->Data, PIN_TYPE, element, pin, drc_callback);
        if (IsBad)
          break;
        if (!TEST_FLAG (HOLEFLAG, pin) &&
            pin->Thickness - pin->DrillingHole < 2 * PCB->minWid)
          {
            AddObjectToFlagUndoList (PIN_TYPE, element, pin, pin);
            SET_FLAG (TheFlag, pin);
            Message (_
                     ("Pin annular ring is too small based on minimum copper width\n"));
            DrawPin (pin, 0);
            drcerr_count++;
            SetThing (PIN_TYPE, element, pin, pin);
            GotoError ();
            if (!gui->confirm_dialog (DRC_CONTINUE, DRC_CANCEL, DRC_NEXT))
              {
                IsBad = True;
                break;
              }
            IncrementUndoSerialNumber ();
            Undo (False);
          }
        if (!TEST_FLAG (HOLEFLAG, pin) &&
            pin->Thickness - pin->DrillingHole < 2 * PCB->minRing)
          {
            AddObjectToFlagUndoList (PIN_TYPE, element, pin, pin);
            SET_FLAG (TheFlag, pin);
            Message (_
                     ("Pin annular ring is too small based on minimum annular ring\n"));
            DrawPin (pin, 0);
            drcerr_count++;
            SetThing (PIN_TYPE, element, pin, pin);
            GotoError ();
            if (!gui->confirm_dialog (DRC_CONTINUE, DRC_CANCEL, DRC_NEXT))
              {
                IsBad = True;
                break;
              }
            IncrementUndoSerialNumber ();
            Undo (False);
          }
        if (pin->DrillingHole < PCB->minDrill)
          {
            AddObjectToFlagUndoList (PIN_TYPE, element, pin, pin);
            SET_FLAG (TheFlag, pin);
            Message (_("Pin drill size is too small\n"));
            DrawPin (pin, 0);
            drcerr_count++;
            SetThing (PIN_TYPE, element, pin, pin);
            GotoError ();
            if (!gui->confirm_dialog (DRC_CONTINUE, DRC_CANCEL, DRC_NEXT))
              {
                IsBad = True;
                break;
              }
            IncrementUndoSerialNumber ();
            Undo (False);
          }
      }
      ENDALL_LOOP;
    }
  if (!IsBad)
    {
      ALLPAD_LOOP (PCB->Data);
      {
        PlowsPolygon (PCB->Data, PAD_TYPE, element, pad, drc_callback);
        if (IsBad)
          break;
        if (pad->Thickness < PCB->minWid)
          {
            AddObjectToFlagUndoList (PAD_TYPE, element, pad, pad);
            SET_FLAG (TheFlag, pad);
            Message (_("Pad is too thin\n"));
            DrawPad (pad, 0);
            drcerr_count++;
            SetThing (PAD_TYPE, element, pad, pad);
            GotoError ();
            if (!gui->confirm_dialog (DRC_CONTINUE, DRC_CANCEL, DRC_NEXT))
              {
                IsBad = True;
                break;
              }
            IncrementUndoSerialNumber ();
            Undo (False);
          }
      }
      ENDALL_LOOP;
    }
  if (!IsBad)
    {
      VIA_LOOP (PCB->Data);
      {
        PlowsPolygon (PCB->Data, VIA_TYPE, via, via, drc_callback);
        if (IsBad)
          break;
        if (!TEST_FLAG (HOLEFLAG, via) &&
            via->Thickness - via->DrillingHole < 2 * PCB->minWid)
          {
            AddObjectToFlagUndoList (VIA_TYPE, via, via, via);
            SET_FLAG (TheFlag, via);
            Message (_
                     ("Via annular ring is too small based on minimum copper width\n"));
            DrawVia (via, 0);
            drcerr_count++;
            SetThing (VIA_TYPE, via, via, via);
            GotoError ();
            if (!gui->confirm_dialog (DRC_CONTINUE, DRC_CANCEL, DRC_NEXT))
              {
                IsBad = True;
                break;
              }
            IncrementUndoSerialNumber ();
            Undo (False);
          }
        if (!TEST_FLAG (HOLEFLAG, via) &&
            via->Thickness - via->DrillingHole < PCB->minRing)
          {
            AddObjectToFlagUndoList (VIA_TYPE, via, via, via);
            SET_FLAG (TheFlag, via);
            Message (_
                     ("Via annular ring is too small based on minimum annular ring\n"));
            DrawVia (via, 0);
            drcerr_count++;
            SetThing (VIA_TYPE, via, via, via);
            GotoError ();
            if (!gui->confirm_dialog (DRC_CONTINUE, DRC_CANCEL, DRC_NEXT))
              {
                IsBad = True;
                break;
              }
            IncrementUndoSerialNumber ();
            Undo (False);
          }
        if (via->DrillingHole < PCB->minDrill)
          {
            AddObjectToFlagUndoList (VIA_TYPE, via, via, via);
            SET_FLAG (TheFlag, via);
            Message (_("Via drill size is too small\n"));
            DrawVia (via, 0);
            drcerr_count++;
            SetThing (VIA_TYPE, via, via, via);
            GotoError ();
            if (!gui->confirm_dialog (DRC_CONTINUE, DRC_CANCEL, DRC_NEXT))
              {
                IsBad = True;
                break;
              }
            IncrementUndoSerialNumber ();
            Undo (False);
          }
      }
      END_LOOP;
    }

  FreeConnectionLookupMemory ();
  TheFlag = FOUNDFLAG;
  Bloat = 0;
  fBloat = 0.0;

  /* check silkscreen minimum widths outside of elements */
  /* XXX - need to check text and polygons too! */
  TheFlag = SELECTEDFLAG;
  if (!IsBad)
    {
      SILKLINE_LOOP (PCB->Data);
      {
        if (line->Thickness < PCB->minSlk)
          {
            SET_FLAG (TheFlag, line);
            Message (_("Silk line is too thin\n"));
            DrawLine (layer, line, 0);
            drcerr_count++;
            SetThing (LINE_TYPE, layer, line, line);
            GotoError ();
            if (!gui->confirm_dialog (DRC_CONTINUE, DRC_CANCEL, DRC_NEXT))
              {
                IsBad = True;
                break;
              }
          }
      }
      ENDALL_LOOP;
    }

  /* check silkscreen minimum widths inside of elements */
  /* XXX - need to check text and polygons too! */
  TheFlag = SELECTEDFLAG;
  if (!IsBad)
    {
      ELEMENT_LOOP (PCB->Data);
      {
        tmpcnt = 0;
        ELEMENTLINE_LOOP (element);
        {
          if (line->Thickness < PCB->minSlk)
            tmpcnt++;
        }
        END_LOOP;
        if (tmpcnt > 0)
          {
            SET_FLAG (TheFlag, element);
            Message (_("Element %s has %d silk lines which are too thin\n"),
                     UNKNOWN (NAMEONPCB_NAME (element)), tmpcnt);
            DrawElement (element, 0);
            drcerr_count++;
            SetThing (ELEMENT_TYPE, element, element, element);
            GotoError ();
            if (!gui->confirm_dialog (DRC_CONTINUE, DRC_CANCEL, DRC_NEXT))
              {
                IsBad = True;
                break;
              }
          }
      }
      END_LOOP;
    }


  if (IsBad)
    {
      IncrementUndoSerialNumber ();
    }


  RestoreStackAndVisibility ();
  hid_action ("LayersChanged");

  if (nopastecnt > 0) 
    {
      Message ("Warning:  %d pad%s the nopaste flag set.\n",
	       nopastecnt,
	       nopastecnt > 1 ? "s have" : " has");
    }
  return IsBad ? -drcerr_count : drcerr_count;
}

/*----------------------------------------------------------------------------
 * center the display to show the offending item (thing)
 */
static void
GotoError (void)
{
  LocationType X, Y;

  switch (thing_type)
    {
    case LINE_TYPE:
      {
        LineTypePtr line = (LineTypePtr) thing_ptr3;
        X = (line->Point1.X + line->Point2.X) / 2;
        Y = (line->Point1.Y + line->Point2.Y) / 2;
        break;
      }
    case ARC_TYPE:
      {
        ArcTypePtr arc = (ArcTypePtr) thing_ptr3;
        X = arc->X;
        Y = arc->Y;
        break;
      }
    case POLYGON_TYPE:
      {
        PolygonTypePtr polygon = (PolygonTypePtr) thing_ptr3;
        X =
          (polygon->Clipped->contours->xmin +
           polygon->Clipped->contours->xmax) / 2;
        Y =
          (polygon->Clipped->contours->ymin +
           polygon->Clipped->contours->ymax) / 2;
        break;
      }
    case PIN_TYPE:
    case VIA_TYPE:
      {
        PinTypePtr pin = (PinTypePtr) thing_ptr3;
        X = pin->X;
        Y = pin->Y;
        break;
      }
    case PAD_TYPE:
      {
        PadTypePtr pad = (PadTypePtr) thing_ptr3;
        X = (pad->Point1.X + pad->Point2.X) / 2;
        Y = (pad->Point1.Y + pad->Point2.Y) / 2;
        break;
      }
    case ELEMENT_TYPE:
      {
        ElementTypePtr element = (ElementTypePtr) thing_ptr3;
        X = element->MarkX;
        Y = element->MarkY;
        break;
      }
    default:
      return;
    }
  Message (_("near location (%d.%02d,%d.%02d)\n"), X / 100, X % 100, Y / 100,
           Y % 100);
  switch (thing_type)
    {
    case LINE_TYPE:
    case ARC_TYPE:
    case POLYGON_TYPE:
      ChangeGroupVisibility (GetLayerNumber
                             (PCB->Data, (LayerTypePtr) thing_ptr1), True,
                             True);
    }
  CenterDisplay (X, Y, False);
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
