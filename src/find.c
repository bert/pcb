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

static char *rcsid = "$Id$";

/*
 * short description:
 * - all pin and via pointers are copied to a single array to have linear
 *   access to them. All pointers refer to this list.
 * - two fields, with pointers to line data, are build for each layer
 *   sorted by:
 *     -  the lower x line coordinates in decending order and
 *     -  the higher x line coordinates in asscending order.
 *   Similar fields for arcs and pads are created.
 *   They are used to have fewer accesses when looking for line connections.
 * - lists for pins and vias, lines, arcs, pads and for polygons are created.
 *   Every object that has to be checked is added to its list.
 * - there's no 'speed-up' mechanism for polygons because they are not used
 *   as often as lines and are much easier to handle
 * - the maximum distance between line and pin ... would depend on the angle
 *   between them. To speed up computation the limit is set to one half
 *   of the thickness of the objects (cause of square pins).
 *
 * PV:  means pin or via (objects that connect layers)
 * LO:  all non PV objects (layer objects like lines, arcs, polygons, pads)
 *
 * 1. first, the PV at the given coordinates is looked up
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
 * - There is a maximum difference between the inner circle
 *   of a PV and the outer one of 8.2% of its radius.
 *   This difference is ignored which means that lines that end
 *   right at the corner of a PV are not recognized.
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
#include <sys/times.h>

#include "global.h"

#include "data.h"
#include "dialog.h"
#include "draw.h"
#include "error.h"
#include "find.h"
#include "gui.h"
#include "mymem.h"
#include "misc.h"
#include "netlist.h"
#include "search.h"
#include "set.h"
#include "undo.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some local macros
 */
#define	SEPERATE(FP)							\
	{											\
		int	i;									\
		fputc('#', (FP));						\
		for (i = Settings.CharPerLine; i; i--)	\
			fputc('=', (FP));					\
		fputc('\n', (FP));						\
	}

#define	PADLIST_ENTRY(L,I)	\
	(((PadDataTypePtr *)PadList[(L)].Data)[(I)])

#define	LINELIST_ENTRY(L,I)	\
	(((LineTypePtr *)LineList[(L)].Data)[(I)])

#define	ARCLIST_ENTRY(L,I)	\
	(((ArcTypePtr *)ArcList[(L)].Data)[(I)])

#define RATLIST_ENTRY(I)	\
	(((RatTypePtr *)RatList.Data)[(I)])

#define	POLYGONLIST_ENTRY(L,I)	\
	(((PolygonTypePtr *)PolygonList[(L)].Data)[(I)])

#define	PVLIST_ENTRY(I)	\
	(((PVDataTypePtr *)PVList.Data)[(I)])

#define	IS_PV_ON_LINE(PV,Line)	\
	(TEST_FLAG(SQUAREFLAG, (PV)) ? \
		IsLineInRectangle( \
			(PV)->X -((PV)->Thickness+1)/2, (PV)->Y -((PV)->Thickness+1)/2, \
			(PV)->X +((PV)->Thickness+1)/2, (PV)->Y +((PV)->Thickness+1)/2, \
			(Line)) : \
		IsPointOnLine((PV)->X,(PV)->Y,MAX((PV)->Thickness/2.0 + fBloat,0.0), (Line)))

/* BUG two square pins could still touch - need a third test */
#define PV_TOUCH_PV(PV1, PV2) \
        (IsPointOnPin((PV1)->X,(PV1)->Y, \
	 (PV1)->Thickness/2.0 + fBloat, (PV2)) || \
         IsPointOnPin((PV2)->X,(PV2)->Y, \
	 (PV2)->Thickness/2.0 + fBloat, (PV1)))

#define IS_PV_ON_RAT(PV, Rat) \
	(IsPointOnLineEnd((PV)->X,(PV)->Y, (Rat)))

#define IS_PV_ON_ARC(PV, Arc)	\
	(TEST_FLAG(SQUAREFLAG, (PV)) ? \
		IsArcInRectangle( \
			(PV)->X -MAX(((PV)->Thickness+1)/2 +Bloat,0), (PV)->Y -MAX(((PV)->Thickness+1)/2 +Bloat,0), \
			(PV)->X +MAX(((PV)->Thickness+1)/2 +Bloat,0), (PV)->Y +MAX(((PV)->Thickness+1)/2 +Bloat,0), \
			(Arc)) : \
		IsPointOnArc((PV)->X,(PV)->Y,MAX((PV)->Thickness/2.0 + fBloat,0.0), (Arc)))

#define	IS_PV_IN_POLYGON(PV,Polygon)	\
	((TEST_FLAG(SQUAREFLAG, (PV)) ? \
		IsRectangleInPolygon( \
			(PV)->X -MAX(((PV)->Thickness+1)/2 +Bloat,0), (PV)->Y -MAX(((PV)->Thickness+1)/2 +Bloat,0), \
			(PV)->X +MAX(((PV)->Thickness+1)/2 +Bloat,0), (PV)->Y +MAX(((PV)->Thickness+1)/2 +Bloat,0), \
			(Polygon)) : \
		IsPointInPolygon((PV)->X,(PV)->Y,MAX((PV)->Thickness/2.0 + fBloat,0.0), (Polygon))))

#define	IS_PV_ON_PAD(PV,Pad) \
	((TEST_FLAG(SQUAREFLAG, (Pad))) ? \
			IsPointInSquarePad((PV)->X, (PV)->Y, MAX((PV)->Thickness/2 +Bloat,0), (Pad)) : \
			IS_PV_ON_LINE((PV), (LineTypePtr) (Pad)))

#define	ADD_PV_TO_LIST(Ptr)					\
{								\
	if (User)								\
		AddObjectToFlagUndoList((Ptr)->Element ? PIN_TYPE : VIA_TYPE, \
		(Ptr)->Element, (Ptr)->Data, (Ptr)->Data);		\
	SET_FLAG(TheFlag, (Ptr)->Data);		\
	PVLIST_ENTRY(PVList.Number) = (Ptr);	\
	PVList.Number++;			\
	if (drc && !TEST_FLAG(SELECTEDFLAG, (Ptr)->Data))	\
		return(True);			\
}

#define	ADD_PAD_TO_LIST(L,Ptr)						\
{									\
	if (User)							\
		AddObjectToFlagUndoList(PAD_TYPE, (Ptr)->Element,	\
		(Ptr)->Data, (Ptr)->Data);						\
	SET_FLAG(TheFlag, (Ptr)->Data);					\
	PADLIST_ENTRY((L),PadList[(L)].Number) = (Ptr);		\
	PadList[(L)].Number++;					\
	if (drc && !TEST_FLAG(SELECTEDFLAG, (Ptr)->Data))	\
		return(True);					\
}

#define	ADD_LINE_TO_LIST(L,Ptr)							\
{										\
	if (User)								\
		AddObjectToFlagUndoList(LINE_TYPE, &PCB->Data->Layer[(L)], \
		(Ptr), (Ptr));							\
	SET_FLAG(TheFlag, (Ptr));							\
	LINELIST_ENTRY((L),LineList[(L)].Number) = (Ptr);	\
	LineList[(L)].Number++;								\
	if (drc && !TEST_FLAG(SELECTEDFLAG, (Ptr)))	\
		return(True);				\
}

#define ADD_ARC_TO_LIST(L,Ptr)						\
{													\
	if (User)										\
		AddObjectToFlagUndoList(ARC_TYPE, &PCB->Data->Layer[(L)], \
		(Ptr), (Ptr));								\
	SET_FLAG(TheFlag, (Ptr));						\
	ARCLIST_ENTRY((L),ArcList[(L)].Number) = (Ptr);	\
	ArcList[(L)].Number++;							\
	if (drc && !TEST_FLAG(SELECTEDFLAG, (Ptr)))	\
		return(True);				\
}

#define ADD_RAT_TO_LIST(Ptr)							\
{										\
	if (User)								\
		AddObjectToFlagUndoList(RATLINE_TYPE, (Ptr), (Ptr), (Ptr));	\
	SET_FLAG(TheFlag, (Ptr));						\
	RATLIST_ENTRY(RatList.Number) = (Ptr);					\
	RatList.Number++;							\
	if (drc && !TEST_FLAG(SELECTEDFLAG, (Ptr)))				\
		return(True);							\
}

#define	ADD_POLYGON_TO_LIST(L,Ptr)							\
{															\
	if (User)												\
		AddObjectToFlagUndoList(POLYGON_TYPE, &PCB->Data->Layer[(L)],	\
		(Ptr), (Ptr));										\
	SET_FLAG(TheFlag, (Ptr));								\
	POLYGONLIST_ENTRY((L), PolygonList[(L)].Number) = (Ptr);\
	PolygonList[(L)].Number++;								\
	if (drc && !TEST_FLAG(SELECTEDFLAG, (Ptr)))	\
		return(True);				\
}

/* ---------------------------------------------------------------------------
 * some local types
 *
 * the two 'dummy' structs for PVs and Pads are necessary for creating
 * connection lists which include the elments name
 */
typedef struct
{
  void **Data;			/* pointer to index data */
  Cardinal Position,		/* currently used position */
    DrawPosition, Number;	/* number of objects in list */
}
ListType, *ListTypePtr;

typedef struct			/* holds a copy of all pins and vias */
{				/* plus a pointer to the according element */
  PinTypePtr Data;
  ElementTypePtr Element;
}
PVDataType, *PVDataTypePtr;

typedef struct			/* holds a copy of all pads of a layer */
{				/* plus a pointer to the according element */
  PadTypePtr Data;
  ElementTypePtr Element;
}
PadDataType, *PadDataTypePtr;

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static float fBloat = 0.0;
static Position Bloat = 0;
static int TheFlag = FOUNDFLAG;
static int OldFlag = FOUNDFLAG;
static Boolean User = False;	/* user action causing this */
static Boolean drc = False;	/* whether to stop if finding something not found */
static LineTypePtr *LineSortedByLowX[MAX_LAYER],	/* sorted array of */
 *LineSortedByHighX[MAX_LAYER];	/* line pointers */
static ArcTypePtr *ArcSortedByLowX[MAX_LAYER],	/* sorted array of */
 *ArcSortedByHighX[MAX_LAYER];	/* arc pointers */
static RatTypePtr *RatSortedByLowX, *RatSortedByHighX;
static PadDataTypePtr PadData[2];
static PadDataTypePtr *PadSortedByLowX[2], *PadSortedByHighX[2];
static PVDataTypePtr PSortedByX, VSortedByX;	/* same for PV */
static Cardinal TotalP, TotalV,	/* total number of PV */
  NumberOfPads[2];
static ListType LineList[MAX_LAYER],	/* list of objects to */
  PolygonList[MAX_LAYER], ArcList[MAX_LAYER], PadList[2], RatList, PVList;

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static int ComparePadByLowX (const void *, const void *);
static int ComparePadByHighX (const void *, const void *);
static int CompareLineByLowX (const void *, const void *);
static int CompareLineByHighX (const void *, const void *);
static int CompareArcByHighX (const void *, const void *);
static int CompareArcByLowX (const void *, const void *);
static int CompareRatByHighX (const void *, const void *);
static int CompareRatByLowX (const void *, const void *);
static int ComparePVByX (const void *, const void *);
static Cardinal GetPadByLowX (Position, Cardinal);
static Cardinal GetPadByHighX (Position, Cardinal);
static Cardinal GetLineByLowX (Position, Cardinal);
static Cardinal GetLineByHighX (Position, Cardinal);
static Cardinal GetArcByLowX (Position, Cardinal);
static Cardinal GetArcByHighX (Position, Cardinal);
static Cardinal GetRatByLowX (Position);
static Cardinal GetRatByHighX (Position);
static Cardinal GetPVByX (int, Position);
static PadDataTypePtr *GetIndexOfPads (Position, Position,
				       Cardinal, Cardinal *);
static LineTypePtr *GetIndexOfLines (Position, Position,
				     Cardinal, Cardinal *);
static PVDataType *LookupPVByCoordinates (int, Position, Position);
static PadDataTypePtr LookupPadByAddress (PadTypePtr);
static Boolean LookupLOConnectionsToPVList (Boolean);
static Boolean LookupLOConnectionsToLOList (Boolean);
static Boolean LookupPVConnectionsToLOList (Boolean);
static Boolean LookupPVConnectionsToPVList (void);
static Boolean LookupLOConnectionsToLine (LineTypePtr, Cardinal, Boolean);
static Boolean LookupLOConnectionsToPad (PadTypePtr, Cardinal);
static Boolean LookupLOConnectionsToPolygon (PolygonTypePtr, Cardinal);
static Boolean LookupLOConnectionsToArc (ArcTypePtr, Cardinal);
static Boolean LookupLOConnectionsToRatEnd (PointTypePtr, Cardinal);
static Boolean IsArcInPolygon (ArcTypePtr, PolygonTypePtr);
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

/* ---------------------------------------------------------------------------
 * some of the 'pad' routines are the same as for lines because the 'pad'
 * struct starts with a line struct. See global.h for details
 */
#define	LinePadIntersect(Line, Pad)			\
	LineLineIntersect((Line), (LineTypePtr) (Pad))
#define ArcPadIntersect(Pad, Arc)		\
	TEST_FLAG(SQUAREFLAG,Pad) ?		\
	IsArcInRectangle( \
		MIN((Pad)->Point1.X, (Pad)->Point2.X) -MAX(((Pad)->Thickness+1)/2 +Bloat,0), \
		MIN((Pad)->Point1.Y, (Pad)->Point2.Y) -MAX(((Pad)->Thickness+1)/2 +Bloat,0), \
		MAX((Pad)->Point1.X, (Pad)->Point2.X) +MAX(((Pad)->Thickness+1)/2 +Bloat,0), \
		MAX((Pad)->Point1.Y, (Pad)->Point2.Y) +MAX(((Pad)->Thickness+1)/2 +Bloat,0), \
			(Arc)) : \
	LineArcIntersect((LineTypePtr) (Pad), (Arc))

/* ---------------------------------------------------------------------------
 * quicksort compare function for pad coordinate X1
 * pad field is sorted in decending order of X1
 */
static int
ComparePadByLowX (const void *Index1, const void *Index2)
{
  PadDataTypePtr ptr1 = *((PadDataTypePtr *) Index1),
    ptr2 = *((PadDataTypePtr *) Index2);

  return ((int) (MIN (ptr2->Data->Point1.X, ptr2->Data->Point2.X) -
		 MIN (ptr1->Data->Point1.X, ptr1->Data->Point2.X)));
}

/* ---------------------------------------------------------------------------
 * quicksort compare function for pad coordinate X2
 * pad field is sorted in ascending order of X2
 */
static int
ComparePadByHighX (const void *Index1, const void *Index2)
{
  PadDataTypePtr ptr1 = *((PadDataTypePtr *) Index1),
    ptr2 = *((PadDataTypePtr *) Index2);

  return ((int) (MAX (ptr1->Data->Point1.X, ptr1->Data->Point2.X) -
		 MAX (ptr2->Data->Point1.X, ptr2->Data->Point2.X)));
}

/* ---------------------------------------------------------------------------
 * quicksort compare function for line coordinate X1
 * line field is sorted in decending order of X1
 */
static int
CompareLineByLowX (const void *Index1, const void *Index2)
{
  LineTypePtr ptr1 = *((LineTypePtr *) Index1),
    ptr2 = *((LineTypePtr *) Index2);

  return ((int)
	  (MIN (ptr2->Point1.X, ptr2->Point2.X) -
	   MIN (ptr1->Point1.X, ptr1->Point2.X)));
}

/* ---------------------------------------------------------------------------
 * quicksort compare function for line coordinate X2
 * line field is sorted in ascending order of X2
 */
static int
CompareLineByHighX (const void *Index1, const void *Index2)
{
  LineTypePtr ptr1 = *((LineTypePtr *) Index1),
    ptr2 = *((LineTypePtr *) Index2);

  return ((int)
	  (MAX (ptr1->Point1.X, ptr1->Point2.X) -
	   MAX (ptr2->Point1.X, ptr2->Point2.X)));
}

/* ---------------------------------------------------------------------------
 * quicksort compare function for arc coordinate X2
 * arc field is sorted in ascending order of X2
 */
static int
CompareArcByLowX (const void *Index1, const void *Index2)
{
  ArcTypePtr ptr1 = *((ArcTypePtr *) Index1), ptr2 = *((ArcTypePtr *) Index2);

  return ((int) (ptr2->BoundingBox.X1 - ptr1->BoundingBox.X1));
}

/* ---------------------------------------------------------------------------
 * quicksort compare function for arc coordinate X2
 * arc field is sorted in ascending order of X2
 */
static int
CompareArcByHighX (const void *Index1, const void *Index2)
{
  ArcTypePtr ptr1 = *((ArcTypePtr *) Index1), ptr2 = *((ArcTypePtr *) Index2);

  return ((int) (ptr1->BoundingBox.X2 - ptr2->BoundingBox.X2));
}

/* ---------------------------------------------------------------------------
 * quicksort compare function for rat coordinate X2
 * rat field is sorted in ascending order of X2
 */
static int
CompareRatByLowX (const void *Index1, const void *Index2)
{
  RatTypePtr ptr1 = *((RatTypePtr *) Index1), ptr2 = *((RatTypePtr *) Index2);

  return ((int)
	  (MIN (ptr2->Point1.X, ptr2->Point2.X) -
	   MIN (ptr1->Point1.X, ptr1->Point2.X)));
}

/* ---------------------------------------------------------------------------
 * quicksort compare function for rat coordinate X2
 * rat field is sorted in ascending order of X2
 */
static int
CompareRatByHighX (const void *Index1, const void *Index2)
{
  RatTypePtr ptr1 = *((RatTypePtr *) Index1), ptr2 = *((RatTypePtr *) Index2);

  return ((int)
	  (MAX (ptr1->Point1.X, ptr1->Point2.X) -
	   MAX (ptr2->Point1.X, ptr2->Point2.X)));
}

/* ---------------------------------------------------------------------------
 * quicksort compare function for pin (via) coordinate X
 * for sorting the PV field in ascending order of X
 */
static int
ComparePVByX (const void *Index1, const void *Index2)
{
  PinTypePtr ptr1 = ((PVDataTypePtr) Index1)->Data,
    ptr2 = ((PVDataTypePtr) Index2)->Data;

  return ((int) (ptr1->X - ptr2->X));
}

/* ---------------------------------------------------------------------------
 * returns the minimum index which matches
 * lowX(pad[index]) <= X for all index >= 'found one'
 * the field is sorted in a descending order
 */
static Cardinal
GetPadByLowX (Position X, Cardinal Layer)
{
  PadDataTypePtr *ptr = PadSortedByLowX[Layer];
  int left = 0, right = NumberOfPads[Layer] - 1, position;

  while (left < right)
    {
      position = (left + right) / 2;
      if (MIN (ptr[position]->Data->Point1.X, ptr[position]->Data->Point2.X)
	  <= X)
	right = position;
      else
	left = position + 1;
    }
  return ((Cardinal) left);
}

/* ---------------------------------------------------------------------------
 * returns the minimum index which matches
 * highX(pad[index]) >= X for all index >= 'found one'
 * the field is sorted in a ascending order
 */
static Cardinal
GetPadByHighX (Position X, Cardinal Layer)
{
  PadDataTypePtr *ptr = PadSortedByHighX[Layer];
  int left = 0, right = NumberOfPads[Layer] - 1, position;

  while (left < right)
    {
      position = (left + right) / 2;
      if (MAX (ptr[position]->Data->Point1.X, ptr[position]->Data->Point2.X)
	  >= X)
	right = position;
      else
	left = position + 1;
    }
  return ((Cardinal) left);
}

/* ---------------------------------------------------------------------------
 * returns the minimum index which matches
 * lowX(line[index]) <= X for all index >= 'found one'
 * the field is sorted in a descending order
 */
static Cardinal
GetLineByLowX (Position X, Cardinal Layer)
{
  LineTypePtr *ptr = LineSortedByLowX[Layer];
  int left = 0, right = PCB->Data->Layer[Layer].LineN - 1, position;

  while (left < right)
    {
      position = (left + right) / 2;
      if (MIN (ptr[position]->Point1.X, ptr[position]->Point2.X) <= X)
	right = position;
      else
	left = position + 1;
    }
  return ((Cardinal) left);
}

/* ---------------------------------------------------------------------------
 * returns the minimum index which matches
 * highX(line[index]) >= X for all index >= 'found one'
 * the field is sorted in a ascending order
 */
static Cardinal
GetLineByHighX (Position X, Cardinal Layer)
{
  LineTypePtr *ptr = LineSortedByHighX[Layer];
  int left = 0, right = PCB->Data->Layer[Layer].LineN - 1, position;

  while (left < right)
    {
      position = (left + right) / 2;
      if (MAX (ptr[position]->Point1.X, ptr[position]->Point2.X) >= X)
	right = position;
      else
	left = position + 1;
    }
  return ((Cardinal) left);
}

/* ---------------------------------------------------------------------------
 * returns the minimum index which matches
 * highX(rat[index]) >= X for all index >= 'found one'
 * the field is sorted in a ascending order
 */
static Cardinal
GetRatByHighX (Position X)
{
  RatTypePtr *ptr = RatSortedByHighX;
  int left = 0, right = PCB->Data->RatN - 1, position;

  while (left < right)
    {
      position = (left + right) / 2;
      if (MAX (ptr[position]->Point1.X, ptr[position]->Point2.X) >= X)
	right = position;
      else
	left = position + 1;
    }
  return ((Cardinal) left);
}

/* ---------------------------------------------------------------------------
 * returns the minimum index which matches
 * lowX(rat[index]) <= X for all index >= 'found one'
 * the field is sorted in a descending order
 */
static Cardinal
GetRatByLowX (Position X)
{
  RatTypePtr *ptr = RatSortedByLowX;
  int left = 0, right = PCB->Data->RatN - 1, position;

  while (left < right)
    {
      position = (left + right) / 2;
      if (MIN (ptr[position]->Point1.X, ptr[position]->Point2.X) <= X)
	right = position;
      else
	left = position + 1;
    }
  return ((Cardinal) left);
}

/* ---------------------------------------------------------------------------
 * returns the minimum index which matches
 * lowX(arc[index]) <= X for all index >= 'found one'
 * the field is sorted in a descending order
 */
static Cardinal
GetArcByLowX (Position X, Cardinal Layer)
{
  ArcTypePtr *ptr = ArcSortedByLowX[Layer];
  int left = 0, right = PCB->Data->Layer[Layer].ArcN - 1, position;

  while (left < right)
    {
      position = (left + right) / 2;
      if (ptr[position]->BoundingBox.X1 <= X)
	right = position;
      else
	left = position + 1;
    }
  return ((Cardinal) left);
}

/* ---------------------------------------------------------------------------
 * returns the minimum index which matches
 * highX(arc[index]) >= X for all index >= 'found one'
 * the field is sorted in a descending order
 */
static Cardinal
GetArcByHighX (Position X, Cardinal Layer)
{
  ArcTypePtr *ptr = ArcSortedByHighX[Layer];
  int left = 0, right = PCB->Data->Layer[Layer].ArcN - 1, position;

  while (left < right)
    {
      position = (left + right) / 2;
      if (ptr[position]->BoundingBox.X2 >= X)
	right = position;
      else
	left = position + 1;
    }
  return ((Cardinal) left);
}

/* ---------------------------------------------------------------------------
 * returns the minimum index which matches
 * X(PV[index]) >= X for all index >= 'found one'
 * the field is sorted in a ascending order
 * kind = 1 means pins, kind = 0 means vias
 */
static Cardinal
GetPVByX (int kind, Position X)
{
  int left = 0, right, position;

  if (kind)
    right = TotalP - 1;
  else
    right = TotalV - 1;
  while (left < right)
    {
      position = (left + right) / 2;
      if (kind)
	{
	  if (PSortedByX[position].Data->X >= X)
	    right = position;
	  else
	    left = position + 1;
	}
      else
	{
	  if (VSortedByX[position].Data->X >= X)
	    right = position;
	  else
	    left = position + 1;
	}
    }
  return ((Cardinal) left);
}

/* ---------------------------------------------------------------------------
 * releases all allocated memory
 */
void
FreeLayoutLookupMemory (void)
{
  Cardinal i;

  for (i = 0; i < MAX_LAYER; i++)
    {
      MyFree ((char **) &LineSortedByLowX[i]);
      MyFree ((char **) &LineSortedByHighX[i]);
      MyFree ((char **) &LineList[i].Data);
      MyFree ((char **) &ArcSortedByLowX[i]);
      MyFree ((char **) &ArcSortedByHighX[i]);
      MyFree ((char **) &ArcList[i].Data);
      MyFree ((char **) &PolygonList[i].Data);
    }
  MyFree ((char **) &VSortedByX);
  MyFree ((char **) &PVList.Data);
  MyFree ((char **) &RatSortedByLowX);
  MyFree ((char **) &RatSortedByHighX);
  MyFree ((char **) &RatList.Data);
}

void
FreeComponentLookupMemory (void)
{
  Cardinal i;

  for (i = 0; i < 2; i++)
    {
      MyFree ((char **) &PadData[i]);
      MyFree ((char **) &PadSortedByLowX[i]);
      MyFree ((char **) &PadSortedByHighX[i]);
      MyFree ((char **) &PadList[i].Data);
    }
  MyFree ((char **) &PSortedByX);
}

/* ---------------------------------------------------------------------------
 * allocates memory for component related stacks ...
 * initializes index and sorts it by X1 and X2
 */
void
InitComponentLookup (void)
{
  Cardinal i, pos;

  /* initialize pad data; start by counting the total number
   * on each of the two possible layers
   */
  NumberOfPads[COMPONENT_LAYER] = NumberOfPads[SOLDER_LAYER] = 0;
  ALLPAD_LOOP (PCB->Data,
	       if (TEST_FLAG (ONSOLDERFLAG, pad))
	       NumberOfPads[SOLDER_LAYER]++;
	       else
	       NumberOfPads[COMPONENT_LAYER]++;);
  for (i = 0; i < 2; i++)
    {
      if (NumberOfPads[i])
	{
	  int count = 0;

	  /* copy pad and element pointers to a list */
	  PadData[i] = (PadDataTypePtr) MyCalloc (NumberOfPads[i],
						  sizeof (PadDataType),
						  "InitConnectionLookup()");
	  ALLPAD_LOOP (PCB->Data,
		       if ((TEST_FLAG (ONSOLDERFLAG, pad) != 0) ==
			   (i == SOLDER_LAYER))
		       {
		       PadData[i][count].Data = pad;
		       PadData[i][count].Element = element; count++;}
	  );

	  /* create two sorted lists of pointers */
	  PadSortedByLowX[i] = (PadDataTypePtr *) MyCalloc (NumberOfPads[i],
							    sizeof
							    (PadDataTypePtr),
							    "InitConnectionLookup()");
	  PadSortedByHighX[i] =
	    (PadDataTypePtr *) MyCalloc (NumberOfPads[i],
					 sizeof (PadDataTypePtr),
					 "InitConnectionLookup()");
	  for (count = 0; count < NumberOfPads[i]; count++)
	    PadSortedByLowX[i][count] = PadSortedByHighX[i][count] =
	      &PadData[i][count];
	  qsort (PadSortedByLowX[i], NumberOfPads[i], sizeof (PadDataTypePtr),
		 ComparePadByLowX);
	  qsort (PadSortedByHighX[i], NumberOfPads[i],
		 sizeof (PadDataTypePtr), ComparePadByHighX);

	  /* allocate memory for working list */
	  PadList[i].Data = (void **) MyCalloc (NumberOfPads[i],
						sizeof (PadDataTypePtr),
						"InitConnectionLookup()");
	}

      /* clear some struct members */
      PadList[i].Position = 0;
      PadList[i].DrawPosition = 0;
      PadList[i].Number = 0;
    }
  /* pin and via data; start with counting their total number,
   * then allocate memory and copy the data to a tmp field
   * set number of the according element in tmp field too
   */
  TotalP = 0;
  ELEMENT_LOOP (PCB->Data, TotalP += element->PinN;);
  PSortedByX = (PVDataTypePtr) MyCalloc (TotalP, sizeof (PVDataType),
					 "InitConnectionLookup()");

  pos = 0;
  ELEMENT_LOOP (PCB->Data,
		PIN_LOOP (element,
			  PSortedByX[pos].Data = pin;
			  PSortedByX[pos++].Element = element;
		);
    );
  qsort (PSortedByX, TotalP, sizeof (PVDataType), ComparePVByX);
}

/* ---------------------------------------------------------------------------
 * allocates memory for component related stacks ...
 * initializes index and sorts it by X1 and X2
 */
void
InitLayoutLookup (void)
{
  Cardinal i, pos;

  /* copy via data; field is initialized with NULL */
  pos = 0;
  TotalV = PCB->Data->ViaN;
  VSortedByX = (PVDataTypePtr) MyCalloc (TotalV, sizeof (PVDataType),
					 "InitConnectionLookup()");
  VIA_LOOP (PCB->Data, VSortedByX[pos].Element = NULL;
	    VSortedByX[pos++].Data = via;
    );

  /* sort array by X */
  qsort (VSortedByX, TotalV, sizeof (PVDataType), ComparePVByX);


  /* initialize line arc and polygon data */
  for (i = 0; i < MAX_LAYER; i++)
    {
      LayerTypePtr layer = &PCB->Data->Layer[i];

      if (layer->LineN)
	{
	  /* allocate memory for line pointer lists */
	  LineSortedByLowX[i] = (LineTypePtr *) MyCalloc (layer->LineN,
							  sizeof
							  (LineTypePtr),
							  "InitConnectionLookup()");
	  LineSortedByHighX[i] =
	    (LineTypePtr *) MyCalloc (layer->LineN, sizeof (LineTypePtr),
				      "InitConnectionLookup()");
	  LineList[i].Data =
	    (void **) MyCalloc (layer->LineN, sizeof (LineTypePtr),
				"InitConnectionLookup()");

	  /* copy addresses to arrays and sort them */
	  LINE_LOOP (layer,
		     LineSortedByLowX[i][n] = LineSortedByHighX[i][n] =
		     line;);
	  qsort (LineSortedByLowX[i], layer->LineN, sizeof (LineTypePtr),
		 CompareLineByLowX);
	  qsort (LineSortedByHighX[i], layer->LineN, sizeof (LineTypePtr),
		 CompareLineByHighX);

	}
      if (layer->ArcN)
	{
	  /* allocate memory for arc pointer lists */
	  ArcSortedByLowX[i] = (ArcTypePtr *) MyCalloc (layer->ArcN,
							sizeof (ArcTypePtr),
							"InitConnectionLookup()");
	  ArcSortedByHighX[i] =
	    (ArcTypePtr *) MyCalloc (layer->ArcN, sizeof (ArcTypePtr),
				     "InitConnectionLookup()");
	  ArcList[i].Data =
	    (void **) MyCalloc (layer->ArcN, sizeof (ArcTypePtr),
				"InitConnectionLookup()");

	  ARC_LOOP (layer,
		    ArcSortedByLowX[i][n] = ArcSortedByHighX[i][n] = arc;);
	  qsort (ArcSortedByLowX[i], layer->ArcN, sizeof (ArcTypePtr),
		 CompareArcByLowX);
	  qsort (ArcSortedByHighX[i], layer->ArcN, sizeof (ArcTypePtr),
		 CompareArcByHighX);
	}


      /* allocate memory for polygon list */
      if (layer->PolygonN)
	PolygonList[i].Data = (void **) MyCalloc (layer->PolygonN,
						  sizeof (PolygonTypePtr),
						  "InitConnectionLookup()");

      /* clear some struct members */
      LineList[i].Position = 0;
      LineList[i].DrawPosition = 0;
      LineList[i].Number = 0;
      ArcList[i].Position = 0;
      ArcList[i].DrawPosition = 0;
      ArcList[i].Number = 0;
      PolygonList[i].Position = 0;
      PolygonList[i].DrawPosition = 0;
      PolygonList[i].Number = 0;
    }

  /* allocate memory for 'new PV to check' list and clear struct */
  PVList.Data = (void **) MyCalloc (TotalP + TotalV, sizeof (PVDataTypePtr),
				    "InitConnectionLookup()");
  PVList.Position = 0;
  PVList.DrawPosition = 0;
  PVList.Number = 0;
  /* Initialize ratline data */
  if (PCB->Data->RatN)
    {
      /* allocate memory for rat pointer lists */
      RatSortedByLowX = (RatTypePtr *) MyCalloc (PCB->Data->RatN,
						 sizeof (RatTypePtr),
						 "InitConnectionLookup()");
      RatSortedByHighX =
	(RatTypePtr *) MyCalloc (PCB->Data->RatN, sizeof (RatTypePtr),
				 "InitConnectionLookup()");
      RatList.Data =
	(void **) MyCalloc (PCB->Data->RatN, sizeof (RatTypePtr),
			    "InitConnectionLookup()");

      /* copy addresses to arrays and sort them */
      RAT_LOOP (PCB->Data, RatSortedByLowX[n] = RatSortedByHighX[n] = line;
	);
      qsort (RatSortedByLowX, PCB->Data->RatN, sizeof (RatTypePtr),
	     CompareRatByLowX);
      qsort (RatSortedByHighX, PCB->Data->RatN, sizeof (RatTypePtr),
	     CompareRatByHighX);

    }
  RatList.Position = 0;
  RatList.DrawPosition = 0;
  RatList.Number = 0;
}

/* ---------------------------------------------------------------------------
 * returns a pointer into the sorted list with the highest index and the
 * number of entries left to check
 * All list entries starting from the pointer position to the end
 * may match the specified x coordinate range.
 */
static PadDataTypePtr *
GetIndexOfPads (Position Xlow, Position Xhigh,
		Cardinal Layer, Cardinal * Number)
{
  Cardinal index1, index2;

  /* get index of the first pad that may match the coordinates
   * see GetPadByLowX(), GetPadByHighX()
   * take the field with less entries to speed up searching
   */
  index1 = GetPadByLowX (Xhigh, Layer);
  index2 = GetPadByHighX (Xlow, Layer);
  if (index1 > index2)
    {
      *Number = NumberOfPads[Layer] - index1;
      return (&PadSortedByLowX[Layer][index1]);
    }
  *Number = NumberOfPads[Layer] - index2;
  return (&PadSortedByHighX[Layer][index2]);
}

/* ---------------------------------------------------------------------------
 * returns a pointer into the sorted list with the highest index and the
 * number of entries left to check
 * All list entries starting from the pointer position to the end
 * may match the specified x coordinate range.
 */
static LineTypePtr *
GetIndexOfLines (Position Xlow, Position Xhigh,
		 Cardinal Layer, Cardinal * Number)
{
  Cardinal index1, index2;

  /* get index of the first line that may match the coordinates
   * see GetLineByLowX(), GetLineByHighX()
   * take the field with less entries to speed up searching
   */
  index1 = GetLineByLowX (Xhigh, Layer);
  index2 = GetLineByHighX (Xlow, Layer);
  if (index1 > index2)
    {
      *Number = PCB->Data->Layer[Layer].LineN - index1;
      return (&LineSortedByLowX[Layer][index1]);
    }
  *Number = PCB->Data->Layer[Layer].LineN - index2;
  return (&LineSortedByHighX[Layer][index2]);
}

/* ---------------------------------------------------------------------------
 * returns a pointer into the sorted list with the highest index and the
 * number of entries left to check
 * All list entries starting from the pointer position to the end
 * may match the specified x coordinate range.
 */
static RatTypePtr *
GetIndexOfRats (Position Xlow, Position Xhigh, Cardinal * Number)
{
  Cardinal index1, index2;

  /* get index of the first rat-line that may match the coordinates
   * take the field with less entries to speed up searching
   */
  index1 = GetRatByLowX (Xhigh);
  index2 = GetRatByHighX (Xlow);
  if (index1 > index2)
    {
      *Number = PCB->Data->RatN - index1;
      return (&RatSortedByLowX[index1]);
    }
  *Number = PCB->Data->RatN - index2;
  return (&RatSortedByHighX[index2]);
}

/* ---------------------------------------------------------------------------
 * returns a pointer into the sorted list with the highest index and the
 * number of entries left to check
 * All list entries starting from the pointer position to the end
 * may match the specified x coordinate range.
 */
static ArcTypePtr *
GetIndexOfArcs (Position Xlow, Position Xhigh,
		Cardinal Layer, Cardinal * Number)
{
  Cardinal index1, index2;

  /* get index of the first arc that may match the coordinates
   * see GetArcByLowX(), GetArcByHighX()
   * take the field with less entries to speed up searching
   */
  index1 = GetArcByLowX (Xhigh, Layer);
  index2 = GetArcByHighX (Xlow, Layer);
  if (index1 > index2)
    {
      *Number = PCB->Data->Layer[Layer].ArcN - index1;
      return (&ArcSortedByLowX[Layer][index1]);
    }
  *Number = PCB->Data->Layer[Layer].ArcN - index2;
  return (&ArcSortedByHighX[Layer][index2]);
}


/* ---------------------------------------------------------------------------
 * finds a pad by it's address in the sorted list
 * A pointer to the list entry or NULL is returned
 */
static PadDataTypePtr
LookupPadByAddress (PadTypePtr Pad)
{
  Cardinal i, layer;
  PadDataTypePtr ptr;

  layer = TEST_FLAG (ONSOLDERFLAG, Pad) ? SOLDER_LAYER : COMPONENT_LAYER;
  i = NumberOfPads[layer];
  ptr = PadData[layer];
  while (i)
    {
      if (ptr->Data == Pad)
	return (ptr);
      i--;
      ptr++;
    }
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * finds a Pin by it's coordinates in the sorted list
 * A pointer to the list entry or NULL is returned
 */
static PVDataTypePtr
LookupPVByCoordinates (int kind, Position X, Position Y)
{
  Cardinal i, limit;

  /* return if their are no PVs */
  if (kind && !TotalP)
    return (NULL);
  if (!kind && !TotalV)
    return (NULL);

  /* get absolute lower/upper boundary */
  i = GetPVByX (kind, X - MAX_PINORVIASIZE);
  limit = GetPVByX (kind, X + MAX_PINORVIASIZE + 1);
  while (i <= limit)
    {
      PinTypePtr ptr;

      if (kind)
	ptr = PSortedByX[i].Data;
      else
	ptr = VSortedByX[i].Data;

      /* check which one matches */
      if (abs (ptr->X - X) <= ptr->Thickness / 4 &&
	  abs (ptr->Y - Y) <= ptr->Thickness / 4)
	return (kind ? &PSortedByX[i] : &VSortedByX[i]);
      i++;
    }
  return (NULL);
}

Boolean
viaClear (PinTypePtr pv)
{
  Cardinal layer, i, j, limit;
  Dimension distance;

  /* check pads (which are lines) */
  for (layer = 0; layer < 2; layer++)
    {
      Dimension distance = MAX ((MAX_PADSIZE + pv->Thickness) / 2 + Bloat, 0);
      PadDataTypePtr *sortedptr;
      Cardinal i;

      /* get the lowest data pointer of pads which may have
       * a connection to the PV ### pad->Point1.X <= pad->Point2.X ###
       */
      sortedptr = GetIndexOfPads (pv->X - distance, pv->X + distance,
				  layer, &i);
      for (; i; i--, sortedptr++)
	if (!TEST_FLAG (TheFlag, (*sortedptr)->Data) &&
	    IS_PV_ON_PAD (pv, (*sortedptr)->Data))
	  return (False);
    }

  /* now all lines, arcs and polygons of the several layers */
  for (layer = 0; layer < MAX_LAYER; layer++)
    {
      Dimension distance =
	MAX ((MAX_LINESIZE + pv->Thickness) / 2 + Bloat, 0);
      LineTypePtr *sortedptr;
      ArcTypePtr *sortedarc;
      Cardinal i;

      /* get the lowest data pointer of lines which may have
       * a connection to the PV
       * ### line->Point1.X <= line->Point2.X ###
       */
      sortedptr = GetIndexOfLines (pv->X - distance, pv->X + distance,
				   layer, &i);
      for (; i; i--, sortedptr++)
	if (!TEST_FLAG (TheFlag, *sortedptr) &&
	    IS_PV_ON_LINE (pv, *sortedptr))
	  return (False);

      sortedarc = GetIndexOfArcs (pv->X - distance, pv->X + distance,
				  layer, &i);
      for (; i; i--, sortedarc++)
	if (!TEST_FLAG (TheFlag, *sortedarc) && IS_PV_ON_ARC (pv, *sortedarc))
	  return (False);
    }
  /* now look for PVs that may touch */
  distance = MAX_PINORVIASIZE + pv->Thickness + Bloat;
  for (j = 0; j < 2; j++)
    {
      if ((j == 0 && TotalV == 0) || (j == 1 && TotalP == 0))
	continue;
      i = GetPVByX (j, pv->X - distance);
      limit = GetPVByX (j, pv->X + distance + 1);
      for (; i <= limit; i++)
	{
	  PinTypePtr pvs = ((j) ? PSortedByX[i].Data : VSortedByX[i].Data);

	  /* we also test those already "found"
	   * since touching of these types leads to
	   * the danger of hole overlap
	   */
	  if ((pvs->Thickness + pv->Thickness) *
	      (pvs->Thickness + pv->Thickness) / 4 > ((pvs->X - pv->X) *
						      (pvs->X - pv->X) +
						      (pvs->Y -
						       pv->Y) * (pvs->Y -
								 pv->Y)))
	    return (False);
	}
    }
  return (True);
}

/* ---------------------------------------------------------------------------
 * checks if a PV is connected to LOs, if it is, the LO is added to
 * the appropriate list and the 'used' flag is set
 */
static Boolean
LookupLOConnectionsToPVList (Boolean AndRats)
{
  PinTypePtr pv;
  Cardinal layer;

  /* loop over all PVs currently on list */
  while (PVList.Position < PVList.Number)
    {
      /* get pointer to data */
      pv = PVLIST_ENTRY (PVList.Position)->Data;

      /* check pads (which are lines) */
      for (layer = 0; layer < 2; layer++)
	{
	  Dimension distance =
	    MAX ((MAX_PADSIZE + pv->Thickness) / 2 + Bloat, 0);
	  PadDataTypePtr *sortedptr;
	  Cardinal i;

	  /* get the lowest data pointer of pads which may have
	   * a connection to the PV ### pad->Point1.X <= pad->Point2.X ###
	   */
	  sortedptr = GetIndexOfPads (pv->X - distance, pv->X + distance,
				      layer, &i);
	  for (; i; i--, sortedptr++)
	    if (!TEST_FLAG (TheFlag, (*sortedptr)->Data) &&
		IS_PV_ON_PAD (pv, (*sortedptr)->Data))
	      ADD_PAD_TO_LIST (layer, *sortedptr)}

	      /* now all lines, arcs and polygons of the several layers */
	      for (layer = 0; layer < MAX_LAYER; layer++)
		{
		  PolygonTypePtr polygon = PCB->Data->Layer[layer].Polygon;
		  Dimension distance =
		    MAX ((MAX_LINESIZE + pv->Thickness) / 2 + Bloat, 0);
		  LineTypePtr *sortedptr;
		  ArcTypePtr *sortedarc;
		  Cardinal i;
		  int Myflag;

		  /* get the lowest data pointer of lines which may have
		   * a connection to the PV
		   * ### line->Point1.X <= line->Point2.X ###
		   */
		  sortedptr =
		    GetIndexOfLines (pv->X - distance, pv->X + distance,
				     layer, &i);
		  for (; i; i--, sortedptr++)
		    if (!TEST_FLAG (TheFlag, *sortedptr) &&
			IS_PV_ON_LINE (pv, *sortedptr))
		      ADD_LINE_TO_LIST (layer, *sortedptr)
			sortedarc =
			GetIndexOfArcs (pv->X - distance, pv->X + distance,
					layer, &i);
		  for (; i; i--, sortedarc++)
		    if (!TEST_FLAG (TheFlag, *sortedarc) &&
			IS_PV_ON_ARC (pv, *sortedarc))
		      ADD_ARC_TO_LIST (layer, *sortedarc)
			/* check all polygons */
			for (i = 0; i < PCB->Data->Layer[layer].PolygonN;
			     i++, polygon++)
			{
			  Myflag = L0THERMFLAG << layer;
			  if ((TEST_FLAG (Myflag, pv)
			       || !TEST_FLAG (CLEARPOLYFLAG, polygon))
			      && !TEST_FLAG (TheFlag, polygon)
			      && IS_PV_IN_POLYGON (pv, polygon))
			    ADD_POLYGON_TO_LIST (layer, polygon)}
			    }
			    /* Check for rat-lines that may intersect the PV */
			    if (AndRats)
			      {
				RatTypePtr *sortedptr;
				Cardinal i;

				/* probably don't need distance offsets here */
				/* since rat-lines only connect at the end points */
				sortedptr = GetIndexOfRats (pv->X, pv->X, &i);
				for (; i; i--, sortedptr++)
				  if (!TEST_FLAG (TheFlag, *sortedptr) &&
				      IS_PV_ON_RAT (pv, *sortedptr))
				    ADD_RAT_TO_LIST (*sortedptr)}
				    PVList.Position++;
			      }
			  return (False);
			}

/* ---------------------------------------------------------------------------
 * find all connections between LO at the current list position and new LOs
 */
		  static Boolean LookupLOConnectionsToLOList (Boolean AndRats)
		  {
		    Boolean done;
		    Cardinal i, group, layer, ratposition,
		      lineposition[MAX_LAYER],
		      polyposition[MAX_LAYER],
		      arcposition[MAX_LAYER], padposition[2];

		    /* copy the current LO list positions; the original data is changed
		     * by 'LookupPVConnectionsToLOList()' which has to check the same
		     * list entries plus the new ones
		     */
		    for (i = 0; i < MAX_LAYER; i++)
		      {
			lineposition[i] = LineList[i].Position;
			polyposition[i] = PolygonList[i].Position;
			arcposition[i] = ArcList[i].Position;
		      }
		    for (i = 0; i < 2; i++)
		      padposition[i] = PadList[i].Position;
		    ratposition = RatList.Position;

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
				    (&(RATLIST_ENTRY (*position)->Point1),
				     group))
				  return (True);
				group = RATLIST_ENTRY (*position)->group2;
				if (LookupLOConnectionsToRatEnd
				    (&(RATLIST_ENTRY (*position)->Point2),
				     group))
				  return (True);
			      }
			  }
			/* loop over all layergroups */
			for (group = 0; group < MAX_LAYER; group++)
			  {
			    Cardinal entry;

			    for (entry = 0;
				 entry < PCB->LayerGroups.Number[group];
				 entry++)
			      {
				layer =
				  PCB->LayerGroups.Entries[group][entry];

				/* be aware that the layer number equal MAX_LAYER
				 * and MAX_LAYER+1 have a special meaning for pads
				 */
				if (layer < MAX_LAYER)
				  {
				    /* try all new lines */
				    position = &lineposition[layer];
				    for (; *position < LineList[layer].Number;
					 (*position)++)
				      if (LookupLOConnectionsToLine
					  (LINELIST_ENTRY (layer, *position),
					   group, True))
					return (True);

				    /* try all new arcs */
				    position = &arcposition[layer];
				    for (; *position < ArcList[layer].Number;
					 (*position)++)
				      if (LookupLOConnectionsToArc
					  (ARCLIST_ENTRY (layer, *position),
					   group))
					return (True);

				    /* try all new polygons */
				    position = &polyposition[layer];
				    for (;
					 *position <
					 PolygonList[layer].Number;
					 (*position)++)
				      if (LookupLOConnectionsToPolygon
					  (POLYGONLIST_ENTRY (layer,
							      *position),
					   group))
					return (True);
				  }
				else
				  {
				    /* try all new pads */
				    layer -= MAX_LAYER;
				    position = &padposition[layer];
				    for (; *position < PadList[layer].Number;
					 (*position)++)
				      if (LookupLOConnectionsToPad
					  (PADLIST_ENTRY (layer,
							  *position)->Data,
					   group))
					return (True);
				  }
			      }
			  }

			/* check if all lists are done; Later for-loops
			 * may have changed the prior lists
			 */
			done = !AndRats || ratposition >= RatList.Number;
			for (layer = 0; layer < MAX_LAYER + 2; layer++)
			  {
			    if (layer < MAX_LAYER)
			      done = done &&
				lineposition[layer] >= LineList[layer].Number
				&& arcposition[layer] >= ArcList[layer].Number
				&& polyposition[layer] >=
				PolygonList[layer].Number;
			    else
			      done = done
				&& padposition[layer - MAX_LAYER] >=
				PadList[layer - MAX_LAYER].Number;
			  }
		      }
		    while (!done);
		    return (False);
		  }

/* ---------------------------------------------------------------------------
 * searches for new PVs that are connected to PVs on the list
 */
		  static Boolean LookupPVConnectionsToPVList (void)
		  {
		    Dimension distance;
		    PinTypePtr pv;
		    Cardinal i, j, limit, save_place;


		    /* loop over all PVs on list */
		      save_place = PVList.Position;
		    while (PVList.Position < PVList.Number)
		      {
			/* get pointer to data */
			pv = PVLIST_ENTRY (PVList.Position)->Data;
			distance =
			  MAX ((MAX_PINORVIASIZE + pv->Thickness) / 2 + Bloat,
			       0);
			for (j = 0; j < 2; j++)
			  {
			    if ((j == 0 && TotalV == 0)
				|| (j == 1 && TotalP == 0))
			      continue;
			    i = GetPVByX (j, pv->X - distance);
			    limit = GetPVByX (j, pv->X + distance + 1);
			    for (; i <= limit; i++)
			      {
				PinTypePtr pv2;

				if (j)
				    pv2 = PSortedByX[i].Data;
				else
				    pv2 = VSortedByX[i].Data;

				if (!TEST_FLAG (TheFlag, pv2)
				    && PV_TOUCH_PV (pv, pv2))
				  {
				    if (TEST_FLAG (HOLEFLAG, pv2))
				      {
					SET_FLAG (WARNFLAG, pv2);
					Settings.RatWarn = True;
					Message
					  ("WARNING: Hole touches pin or via.\n");
				      }
				    ADD_PV_TO_LIST (j ? &PSortedByX[i] :
						    &VSortedByX[i])}
			      }
			  }
			PVList.Position++;
		      }
		    PVList.Position = save_place;
		    return (False);
		  }




/* ---------------------------------------------------------------------------
 * searches for new PVs that are connected to NEW LOs on the list
 * This routine updates the position counter of the lists too.
 */
		  static Boolean LookupPVConnectionsToLOList (Boolean AndRats)
		  {
		    Cardinal layer, i, j, limit;
		    Dimension distance;
		    int Myflag;

		    /* loop over all layers */
		    for (layer = 0; layer < MAX_LAYER; layer++)
		      {
			/* do nothing if there are no PV's */
			if (TotalP + TotalV == 0)
			  {
			    LineList[layer].Position = LineList[layer].Number;
			    ArcList[layer].Position = ArcList[layer].Number;
			    PolygonList[layer].Position =
			      PolygonList[layer].Number;
			    continue;
			  }

			/* check all lines */
			while (LineList[layer].Position <
			       LineList[layer].Number)
			  {
			    LineTypePtr line = LINELIST_ENTRY (layer,
							       LineList
							       [layer].
							       Position);

			    /* get the positions in sorted field to speed up searching
			     * ### line->Point1.X <= line->Point2.X ###
			     * the '+1' in the second call of GetPVByX()
			     * makes sure that 'limit' is realy the position outside the
			     * range.
			     */
			    distance =
			      MAX ((MAX_PINORVIASIZE + line->Thickness) / 2 +
				   Bloat, 0);
			    for (j = 0; j < 2; j++)
			      {
				if ((j == 0 && TotalV == 0)
				    || (j == 1 && TotalP == 0))
				  continue;
				i =
				  GetPVByX (j,
					    MIN (line->Point1.X,
						 line->Point2.X) - distance);
				limit =
				  GetPVByX (j,
					    MAX (line->Point1.X,
						 line->Point2.X) + distance +
					    1);
				for (; i <= limit; i++)
				  {
				    PinTypePtr pv;

				    if (j)
				      pv = PSortedByX[i].Data;
				    else
				      pv = VSortedByX[i].Data;

				    if (!TEST_FLAG (TheFlag, pv)
					&& IS_PV_ON_LINE (pv, line))
				      {
					if (TEST_FLAG (HOLEFLAG, pv))
					  {
					    SET_FLAG (WARNFLAG, pv);
					    Settings.RatWarn = True;
					    Message
					      ("WARNING: Hole touches line.\n");
					  }
					ADD_PV_TO_LIST (j ? &PSortedByX[i] :
							&VSortedByX[i])}
				  }
			      }
			    LineList[layer].Position++;
			  }

			/* check all arcs */
			while (ArcList[layer].Position <
			       ArcList[layer].Number)
			  {
			    Position X1, X2;
			    ArcTypePtr arc =
			      ARCLIST_ENTRY (layer, ArcList[layer].Position);

			    /* get the positions in sorted field to speed up searching
			     * the '+1' in the second call of GetPVByX()
			     * makes sure that 'limit' is realy the position outside the
			     * range
			     */
			    distance =
			      MAX ((MAX_PINORVIASIZE + arc->Thickness) / 2 +
				   Bloat, 0);
			    X1 =
			      arc->X -
			      arc->Width * cos (M180 * arc->StartAngle);
			    X2 =
			      arc->X -
			      arc->Width * cos (M180 *
						(arc->StartAngle +
						 arc->Delta));
			    for (j = 0; j < 2; j++)
			      {
				if ((j == 0 && TotalV == 0)
				    || (j == 1 && TotalP == 0))
				  continue;
				i = GetPVByX (j, MIN (X1, X2) - distance);
				limit =
				  GetPVByX (j, MAX (X1, X2) + distance + 1);
				for (; i <= limit; i++)
				  {
				    PinTypePtr pv;

				    if (j)
				      pv = PSortedByX[i].Data;
				    else
				      pv = VSortedByX[i].Data;

				    if (!TEST_FLAG (TheFlag, pv)
					&& IS_PV_ON_ARC (pv, arc))
				      {
					if (TEST_FLAG (HOLEFLAG, pv))
					  {
					    SET_FLAG (WARNFLAG, pv);
					    Settings.RatWarn = True;
					    Message
					      ("WARNING: Hole touches arc.\n");
					  }
					ADD_PV_TO_LIST (j ? &PSortedByX[i] :
							&VSortedByX[i])}
				  }
			      }
			    ArcList[layer].Position++;
			  }

			/* now all polygons */
			while (PolygonList[layer].Position <
			       PolygonList[layer].Number)
			  {
			    PolygonTypePtr polygon;

			    polygon =
			      POLYGONLIST_ENTRY (layer,
						 PolygonList[layer].Position);

			    /* get the positions in sorted field to speed up searching */
			    distance = MAX_PINORVIASIZE / 2;
			    Myflag = L0THERMFLAG << layer;
			    for (j = 0; j < 2; j++)
			      {
				if ((j == 0 && TotalV == 0)
				    || (j == 1 && TotalP == 0))
				  continue;
				i =
				  GetPVByX (j,
					    polygon->BoundingBox.X1 -
					    distance);
				limit =
				  GetPVByX (j,
					    polygon->BoundingBox.X2 +
					    distance + 1);
				for (; i <= limit; i++)
				  {
				    PinTypePtr pv;

				    if (j)
				      pv = PSortedByX[i].Data;
				    else
				      pv = VSortedByX[i].Data;

				    /* note that holes in polygons are ok */
				    if ((TEST_FLAG (Myflag, pv)
					 || !TEST_FLAG (CLEARPOLYFLAG,
							polygon))
					&& !TEST_FLAG (TheFlag, pv)
					&& IS_PV_IN_POLYGON (pv, polygon))
				      ADD_PV_TO_LIST (j ? &PSortedByX[i] :
						      &VSortedByX[i])}
				      }
				      PolygonList[layer].Position++;
				  }
			      }

			    /* loop over all pad-layers */
			    for (layer = 0; layer < 2; layer++)
			      {
				/* do nothing if there are no PV's */
				if (TotalP + TotalV == 0)
				  {
				    PadList[layer].Position =
				      PadList[layer].Number;
				    continue;
				  }

				/* check all pads; for a detailed description see
				 * the handling of lines in this subroutine
				 */
				while (PadList[layer].Position <
				       PadList[layer].Number)
				  {
				    PadTypePtr pad;

				    pad =
				      PADLIST_ENTRY (layer,
						     PadList
						     [layer].Position)->Data;
				    distance =
				      MAX ((MAX_PINORVIASIZE + pad->Thickness)
					   / 2 + Bloat, 0);
				    for (j = 0; j < 2; j++)
				      {
					if ((j == 0 && TotalV == 0)
					    || (j == 1 && TotalP == 0))
					  continue;
					i =
					  GetPVByX (j,
						    MIN (pad->Point1.X,
							 pad->Point2.X) -
						    distance);
					limit =
					  GetPVByX (j,
						    MAX (pad->Point1.X,
							 pad->Point2.X) +
						    distance + 1);
					for (; i <= limit; i++)
					  {
					    PinTypePtr pv;

					    if (j)
					      pv = PSortedByX[i].Data;
					    else
					      pv = VSortedByX[i].Data;

					    if (!TEST_FLAG (TheFlag, pv)
						&& IS_PV_ON_PAD (pv, pad))
					      {
						if (TEST_FLAG (HOLEFLAG, pv))
						  {
						    SET_FLAG (WARNFLAG, pv);
						    Settings.RatWarn = True;
						    Message
						      ("WARNING: Hole touches pad.\n");
						  }
						ADD_PV_TO_LIST (j ?
								&PSortedByX
								[i] :
								&VSortedByX
								[i])}
					  }
				      }
				    PadList[layer].Position++;
				  }
			      }

			    /* do nothing if there are no PV's */
			    if (TotalP + TotalV == 0)
			      RatList.Position = RatList.Number;

			    /* check all rat-lines */
			    if (AndRats)
			      {
				while (RatList.Position < RatList.Number)
				  {
				    RatTypePtr rat;

				    rat = RATLIST_ENTRY (RatList.Position);
				    distance = (MAX_PINORVIASIZE) / 2;
				    for (j = 0; j < 2; j++)
				      {
					if ((j == 0 && TotalV == 0)
					    || (j == 1 && TotalP == 0))
					  continue;
					i =
					  GetPVByX (j,
						    MIN (rat->Point1.X,
							 rat->Point2.X) -
						    distance);
					limit =
					  GetPVByX (j,
						    MAX (rat->Point1.X,
							 rat->Point2.X) +
						    distance + 1);
					for (; i <= limit; i++)
					  {
					    PinTypePtr pv;

					    if (j)
					      pv = PSortedByX[i].Data;
					    else
					      pv = VSortedByX[i].Data;

					    if (!TEST_FLAG (TheFlag, pv)
						&& IS_PV_ON_RAT (pv, rat))
					      ADD_PV_TO_LIST (j ?
							      &PSortedByX[i] :
							      &VSortedByX[i])}
					      }
					      RatList.Position++;
					  }
				      }
				    return (False);
				  }

				static Boolean PVTouchesLine (LineTypePtr
							      line)
				{
				  Cardinal i, j, limit;
				  Dimension distance;

				  /* do nothing if there are no PV's */
				  if (TotalP + TotalV == 0)
				      return (False);

				  /* get the positions in sorted field to speed up searching
				   * ### line->Point1.X <= line->Point2.X ###
				   * the '+1' in the second call of GetPVByX()
				   * makes sure that 'limit' is realy the position outside the
				   * range
				   */


				    distance =
				    MAX ((MAX_PINORVIASIZE + line->Thickness)
					 / 2 + Bloat, 0);
				  for (j = 0; j < 2; j++)
				    {
				      if ((j == 0 && TotalV == 0)
					  || (j == 1 && TotalP == 0))
					continue;
				      i =
					GetPVByX (j,
						  MIN (line->Point1.X,
						       line->Point2.X) -
						  distance);
				      limit =
					GetPVByX (j,
						  MAX (line->Point1.X,
						       line->Point2.X) +
						  distance + 1);
				      for (; i <= limit; i++)
					{
					  PinTypePtr pv =
					    ((j) ? PSortedByX[i].Data :
					     VSortedByX[i].Data);

					  if (!TEST_FLAG (TheFlag, pv)
					      && IS_PV_ON_LINE (pv, line))
					      return (True);
					}
				    }
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
				static Boolean ArcArcIntersect (ArcTypePtr
								Arc1,
								ArcTypePtr
								Arc2)
				{
				  register float x, y, dx, dy, r1, r2, a, d,
				    l, t, t2;
				  register Position pdx, pdy;
				  BoxTypePtr box;
				  BoxType box1, box2;

				    pdx = Arc2->X - Arc1->X;
				    pdy = Arc2->Y - Arc1->Y;
				    l = pdx * pdx + pdy * pdy;
				    t =
				    MAX (0.5 * Arc1->Thickness + fBloat, 0.0);
				    t2 = 0.5 * Arc2->Thickness;
				  /* concentric arcs, simpler intersection conditions */
				  if (l == 0.0)
				    {
				      if ((Arc1->Width - t >= Arc2->Width - t2
					   && Arc1->Width - t <=
					   Arc2->Width + t2)
					  || (Arc1->Width + t >=
					      Arc2->Width - t2
					      && Arc1->Width + t <=
					      Arc2->Width + t2))
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
						  && Arc2->BoundingBox.Y2 <=
						  Arc1->BoundingBox.Y2))
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
				      && y - dx >= Arc2->BoundingBox.Y1
				      && y - dx <= Arc2->BoundingBox.Y2)
				    return (True);

				  if (x - dy >= Arc1->BoundingBox.X1
				      && x - dy <= Arc1->BoundingBox.X2
				      && y + dx >= Arc1->BoundingBox.Y1
				      && y + dx <= Arc1->BoundingBox.Y2
				      && x - dy >= Arc2->BoundingBox.X1
				      && x - dy <= Arc2->BoundingBox.X2
				      && y + dx >= Arc2->BoundingBox.Y1
				      && y + dx <= Arc2->BoundingBox.Y2)
				    return (True);

				  /* try the end points in case the ends don't actually pierce the outer radii */
				  box = GetArcEnds (Arc1);
				  box1 = *box;
				  box = GetArcEnds (Arc2);
				  box2 = *box;
				  if (IsPointOnArc
				      ((float) box1.X1, (float) box1.Y1, t,
				       Arc2)
				      || IsPointOnArc ((float) box1.X2,
						       (float) box1.Y2, t,
						       Arc2))
				    return (True);

				  if (IsPointOnArc
				      ((float) box2.X1, (float) box2.Y1,
				       MAX (t2 + fBloat, 0.0), Arc1)
				      || IsPointOnArc ((float) box2.X2,
						       (float) box2.Y2,
						       MAX (t2 + fBloat, 0.0),
						       Arc1))
				    return (True);
				  return (False);
				}
/* ---------------------------------------------------------------------------
 * Tests if point is same as line end point
 */
				static Boolean
				  IsRatPointOnLineEnd (PointTypePtr Point,
						       LineTypePtr Line)
				{
				  if ((Point->X == Line->Point1.X
				       && Point->Y == Line->Point1.Y)
				      || (Point->X == Line->Point2.X
					  && Point->Y == Line->Point2.Y))
				    return (True);
				  return (False);
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
				Boolean LineLineIntersect (LineTypePtr Line1,
							   LineTypePtr Line2)
				{
				  register float dx, dy, dx1, dy1, s, r;

				  if (MAX (Line1->Point1.Y, Line1->Point2.Y) +
				      Line1->Thickness + Line2->Thickness <
				      MIN (Line2->Point1.Y, Line2->Point2.Y)
				      || MIN (Line1->Point1.Y,
					      Line1->Point2.Y) -
				      Line1->Thickness - Line2->Thickness >
				      MAX (Line2->Point1.Y, Line2->Point2.Y))
				      return False;
				  /* setup some constants */
				    dx =
				    (float) (Line1->Point2.X -
					     Line1->Point1.X);
				    dy =
				    (float) (Line1->Point2.Y -
					     Line1->Point1.Y);
				    dx1 =
				    (float) (Line1->Point1.X -
					     Line2->Point1.X);
				    dy1 =
				    (float) (Line1->Point1.Y -
					     Line2->Point1.Y);
				    s = dy1 * dx - dx1 * dy;




				    r =
				    dx * (float) (Line2->Point2.Y -
						  Line2->Point1.Y) -
				    dy * (float) (Line2->Point2.X -
						  Line2->Point1.X);

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



					  return (TEST_FLAG (SQUAREFLAG, Line2)
						  ?
						  IsPointInSquarePad
						  (Line1->Point1.X,
						   Line1->Point1.Y,
						   MAX (Line1->Thickness / 2 +
							Bloat, 0),
						   (PadTypePtr) Line2) :
						  IsPointOnLine
						  (Line1->Point1.X,
						   Line1->Point1.Y,
						   MAX (Line1->Thickness / 2 +
							Bloat, 0), Line2));
				        s = s * s / (dx * dx + dy * dy);


				        distance =
					MAX ((float) 0.5 *
					     (Line1->Thickness +
					      Line2->Thickness) + fBloat,
					     0.0);
				        distance *= distance;
				      if (s > distance)
					  return (False);
				      if (TEST_FLAG (SQUAREFLAG, Line1) &&
					  (IsPointInSquarePad (Line2->Point1.
							       X,
							       Line2->Point1.
							       Y,
							       MAX (Line2->
								    Thickness
								    / 2 +
								    Bloat, 0),
							       (PadTypePtr)
							       Line1)
					   || IsPointInSquarePad (Line2->
								  Point2.X,
								  Line2->
								  Point2.Y,
								  MAX (Line2->
								       Thickness
								       / 2 +
								       Bloat,
								       0),
								  (PadTypePtr)
								  Line1)))
					  return (True);
				      if (TEST_FLAG (SQUAREFLAG, Line2) &&
					  (IsPointInSquarePad (Line1->Point1.
							       X,
							       Line1->Point1.
							       Y,
							       MAX (Line1->
								    Thickness
								    / 2 +
								    Bloat, 0),
							       (PadTypePtr)
							       Line2)
					   || IsPointInSquarePad (Line1->
								  Point2.X,
								  Line1->
								  Point2.Y,
								  MAX (Line1->
								       Thickness
								       / 2 +
								       Bloat,
								       0),
								  (PadTypePtr)
								  Line2)))
					  return (True);
				        return (IsPointOnLine (Line1->Point1.X,
							       Line1->Point1.
							       Y,
							       MAX (0.5 *
								    Line1->
								    Thickness
								    + fBloat,
								    0.0),
							       Line2)
						|| IsPointOnLine (Line1->
								  Point2.X,
								  Line1->
								  Point2.Y,
								  MAX (0.5 *
								       Line1->
								       Thickness
								       +
								       fBloat,
								       0.0),
								  Line2)
						|| IsPointOnLine (Line2->
								  Point1.X,
								  Line2->
								  Point1.Y,
								  MAX (0.5 *
								       Line2->
								       Thickness
								       +
								       fBloat,
								       0.0),
								  Line1)
						|| IsPointOnLine (Line2->
								  Point2.X,
								  Line2->
								  Point2.Y,
								  MAX (0.5 *
								       Line2->
								       Thickness
								       +
								       fBloat,
								       0.0),
								  Line1));
				    }
				  else
				    {
				      register float radius;

				      s /= r;
				      r =
					(dy1 *
					 (float) (Line2->Point2.X -
						  Line2->Point1.X) -
					 dx1 * (float) (Line2->Point2.Y -
							Line2->Point1.Y)) / r;

				      /* intersection is at least on AB */
				      if (r >= 0.0 && r <= 1.0)
					{
					  if (s >= 0.0 && s <= 1.0)
					    return (True);

					  /* intersection on AB and extension of CD */
					  return (s < 0.0 ?
						  IsPointOnLine
						  (Line2->Point1.X,
						   Line2->Point1.Y,
						   MAX (0.5 *
							Line2->Thickness +
							fBloat, 0.0),
						   Line1) :
						  IsPointOnLine
						  (Line2->Point2.X,
						   Line2->Point2.Y,
						   MAX (0.5 *
							Line2->Thickness +
							fBloat, 0.0), Line1));
					}

				      /* intersection is at least on CD */
				      if (s >= 0.0 && s <= 1.0)
					{
					  /* intersection on CD and extension of AB */
					  return (r < 0.0 ?
						  IsPointOnLine
						  (Line1->Point1.X,
						   Line1->Point1.Y,
						   MAX (Line1->Thickness /
							2.0 + fBloat, 0.0),
						   Line2) :
						  IsPointOnLine
						  (Line1->Point2.X,
						   Line1->Point2.Y,
						   MAX (Line1->Thickness /
							2.0 + fBloat, 0.0),
						   Line2));
					}

				      /* no intersection of zero-width lines but maybe of thick lines;
				       * BEEP! Must check each end point to find the nearest two
				       */
				      radius =
					MAX (0.5 *
					     (Line1->Thickness +
					      Line2->Thickness) + fBloat,
					     0.0);
				      radius *= radius;
				      dx =
					(float) Line1->Point1.X -
					Line2->Point1.X;
				      dy =
					(float) Line1->Point1.Y -
					Line2->Point1.Y;
				      if (dx * dx + dy * dy <= radius)
					return (True);
				      dx =
					(float) Line1->Point1.X -
					Line2->Point2.X;
				      dy =
					(float) Line1->Point1.Y -
					Line2->Point2.Y;
				      if (dx * dx + dy * dy <= radius)
					return (True);
				      dx =
					(float) Line1->Point2.X -
					Line2->Point1.X;
				      dy =
					(float) Line1->Point2.Y -
					Line2->Point1.Y;
				      if (dx * dx + dy * dy <= radius)
					return (True);
				      dx =
					(float) Line1->Point2.X -
					Line2->Point2.X;
				      dy =
					(float) Line1->Point2.Y -
					Line2->Point2.Y;
				      if (dx * dx + dy * dy <= radius)
					return (True);
				      return (False);
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
 * To do this, we tranlate along the line from the point Q
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
				Boolean LineArcIntersect (LineTypePtr Line,
							  ArcTypePtr Arc)
				{
				  register float dx, dy, dx1, dy1, l, d, r,
				    r2, Radius;

				    dx =
				    (float) (Line->Point2.X - Line->Point1.X);
				    dy =
				    (float) (Line->Point2.Y - Line->Point1.Y);
				    dx1 = (float) (Line->Point1.X - Arc->X);
				    dy1 = (float) (Line->Point1.Y - Arc->Y);
				    l = dx * dx + dy * dy;
				    d = dx * dy1 - dy * dx1;
				    d *= d;

				  /* use the larger diameter circle first */


				    Radius =
				    Arc->Width +
				    MAX (0.5 *
					 (Arc->Thickness + Line->Thickness) +
					 fBloat, 0.0);
				    Radius *= Radius;
				    r2 = Radius * l - d;
				  /* projection doesn't even intersect circle when r2 < 0 */
				  if (r2 < 0)
				      return (False);
				  /* check the ends of the line in case the projected point */
				  /* of intersection is beyond the line end */
				  if (IsPointOnArc
				      (Line->Point1.X, Line->Point1.Y,
				       MAX (0.5 * Line->Thickness + fBloat,
					    0.0), Arc))
				      return (True);
				  if (IsPointOnArc
				      (Line->Point2.X, Line->Point2.Y,
				       MAX (0.5 * Line->Thickness + fBloat,
					    0.0), Arc))
				      return (True);
				  if (l == 0.0)
				      return (False);
				    r2 = sqrt (r2);
				    Radius = -(dx * dx1 + dy * dy1);
				    r = (Radius + r2) / l;
				  if (r >= 0 && r <= 1
				      && IsPointOnArc (Line->Point1.X +
						       r * dx,
						       Line->Point1.Y +
						       r * dy,
						       MAX (0.5 *
							    Line->Thickness +
							    fBloat, 0.0),
						       Arc))
				      return (True);
				    r = (Radius - r2) / l;
				  if (r >= 0 && r <= 1
				      && IsPointOnArc (Line->Point1.X +
						       r * dx,
						       Line->Point1.Y +
						       r * dy,
						       MAX (0.5 *
							    Line->Thickness +
							    fBloat, 0.0),
						       Arc))
				      return (True);
				    return (False);
				}

/* ---------------------------------------------------------------------------
 * searches all LOs that are connected to the given arc on the given
 * layergroup. All found connections are added to the list
 *
 * the notation that is used is:
 * Xij means Xj at arc i
 */
				static Boolean
				  LookupLOConnectionsToArc (ArcTypePtr Arc,
							    Cardinal
							    LayerGroup)
				{
				  Cardinal entry;
				  Position xlow, xhigh;

				  /* the maximum possible distance */


				    xlow =
				    Arc->BoundingBox.X1 - MAX (MAX_PADSIZE,
							       MAX_LINESIZE) /
				    2;
				    xhigh =
				    Arc->BoundingBox.X2 + MAX (MAX_PADSIZE,
							       MAX_LINESIZE) /
				    2;

				  /* loop over all layers of the group */
				  for (entry = 0;
				       entry <
				       PCB->LayerGroups.Number[LayerGroup];
				       entry++)
				    {
				      Cardinal layer, i;




				        layer =
					PCB->LayerGroups.Entries[LayerGroup]
					[entry];

				      /* handle normal layers */
				      if (layer < MAX_LAYER)
					{
					  PolygonTypePtr polygon;
					  LineTypePtr *sortedptr;
					  ArcTypePtr *sortedarc;

					  /* get index of the first line that may match the coordinates */


					    sortedptr =
					    GetIndexOfLines (xlow, xhigh,
							     layer, &i);

					  /* loop till the end of the data is reached
					   * DONT RETRY LINES THAT HAVE BEEN FOUND
					   */
					  for (; i; i--, sortedptr++)
					    if (!TEST_FLAG
						(TheFlag, *sortedptr))
					      {
						if (LineArcIntersect
						    (*sortedptr, Arc))
						  ADD_LINE_TO_LIST (layer,
								    *sortedptr)}
						  sortedarc =
						    GetIndexOfArcs (xlow,
								    xhigh,
								    layer,
								    &i);
						for (; i; i--, sortedarc++)
						  if (!TEST_FLAG
						      (TheFlag, *sortedarc)
						      && ArcArcIntersect (Arc,
									  *sortedarc))
						    ADD_ARC_TO_LIST (layer,
								     *sortedarc)
						      /* now check all polygons */
						      i = 0;
						polygon =
						  PCB->Data->Layer[layer].
						  Polygon;
						for (;
						     i <
						     PCB->Data->Layer[layer].
						     PolygonN; i++, polygon++)
						  if (!TEST_FLAG
						      (TheFlag, polygon)
						      && IsArcInPolygon (Arc,
									 polygon))
						    ADD_POLYGON_TO_LIST
						      (layer, polygon)}
						    else
						    {
						      /* handle special 'pad' layers */
						      PadDataTypePtr
							* sortedptr;

						      layer -= MAX_LAYER;
						      sortedptr =
							GetIndexOfPads (xlow,
									xhigh,
									layer,
									&i);
						      for (; i;
							   i--, sortedptr++)
							if (!TEST_FLAG
							    (TheFlag,
							     (*sortedptr)->
							     Data)
							    &&
							    ArcPadIntersect (
									     (*sortedptr)->Data,
									     Arc))
							  ADD_PAD_TO_LIST
							    (layer,
							     *sortedptr)}
							  }
							  return (False);
						    }

/* ---------------------------------------------------------------------------
 * searches all LOs that are connected to the given line on the given
 * layergroup. All found connections are added to the list
 *
 * the notation that is used is:
 * Xij means Xj at line i
 */
	static Boolean
	  LookupLOConnectionsToLine
	  (LineTypePtr Line,
	   Cardinal LayerGroup,
	   Boolean PolysTo)
	{
	  Cardinal entry;
	  Position xlow, xhigh;
	  RatTypePtr *sortedrat;
	  Cardinal i;

	  /* the maximum possible distance */


	    xlow =
	    MIN (Line->Point1.X,
		 Line->Point2.X) -
	    MAX (Line->Thickness +
		 MAX (MAX_PADSIZE,
		      MAX_LINESIZE) /
		 2 - Bloat, 0);
	    xhigh =
	    MAX (Line->Point1.X,
		 Line->Point2.X) +
	    MAX (Line->Thickness +
		 MAX (MAX_PADSIZE,
		      MAX_LINESIZE) /
		 2 + Bloat, 0);

	  /* add the new rat lines */


	    sortedrat =
	    GetIndexOfRats (xlow,
			    xhigh,
			    &i);
	  for (; i; i--, sortedrat++)
	    if (!TEST_FLAG
		(TheFlag, *sortedrat))
	      {
		RatTypePtr rat =
		  *sortedrat;

		if ((rat->group1 ==
		     LayerGroup)
		    &&
		    IsRatPointOnLineEnd
		    (&rat->Point1,
		     Line))

		    ADD_RAT_TO_LIST
		    (rat)
		  else
		  if ((rat->group2 ==
		       LayerGroup)
		      &&
		      IsRatPointOnLineEnd
		      (&rat->Point2,
		       Line))

		    ADD_RAT_TO_LIST
		    (rat)}

		  /* loop over all layers of the group */
		  for (entry = 0;
		       entry <
		       PCB->
		       LayerGroups.
		       Number
		       [LayerGroup];
		       entry++)
		    {
		      Cardinal layer;

		      layer =
			PCB->
			LayerGroups.
			Entries
			[LayerGroup]
			[entry];

		      /* handle normal layers */
		      if (layer <
			  MAX_LAYER)
			{
			  PolygonTypePtr
			    polygon;
			  LineTypePtr
			    *
			    sortedptr;
			  ArcTypePtr
			    *
			    sortedarc;

			  /* get index of the first line that may match the coordinates */
			  sortedptr =
			    GetIndexOfLines
			    (xlow,
			     xhigh,
			     layer,
			     &i);

			  /* loop till the end of the data is reached
			   * DONT RETRY LINES THAT HAVE BEEN FOUND
			   */
			  for (; i;
			       i--,
			       sortedptr++)
			    if
			      (!TEST_FLAG
			       (TheFlag,
				*sortedptr))
			      {
				if
				  (LineLineIntersect
				   (Line,
				    *sortedptr))
				  ADD_LINE_TO_LIST
				    (layer,
				     *sortedptr)}
				  sortedarc
				    =
				    GetIndexOfArcs
				    (xlow,
				     xhigh,
				     layer,
				     &i);
				for (;
				     i;
				     i--,
				     sortedarc++)
				  if
				    (!TEST_FLAG
				     (TheFlag,
				      *sortedarc)
				     &&
				     LineArcIntersect
				     (Line,
				      *sortedarc))
				    ADD_ARC_TO_LIST
				      (layer,
				       *sortedarc)
				      /* now check all polygons */
				      if
				      (PolysTo)
				      {
					i
					  =
					  0;
					polygon
					  =
					  PCB->
					  Data->
					  Layer
					  [layer].
					  Polygon;
					for
					  (;
					   i
					   <
					   PCB->
					   Data->
					   Layer
					   [layer].
					   PolygonN;
					   i++,
					   polygon++)
					  if
					    (!TEST_FLAG
					     (TheFlag,
					      polygon)
					     &&
					     IsLineInPolygon
					     (Line,
					      polygon))
					    ADD_POLYGON_TO_LIST
					      (layer,
					       polygon)}
					    }
					    else
					    {
					      /* handle special 'pad' layers */
					      PadDataTypePtr
						*
						sortedptr;

					      layer
						-=
						MAX_LAYER;
					      sortedptr
						=
						GetIndexOfPads
						(xlow,
						 xhigh,
						 layer,
						 &i);
					      for
						(;
						 i;
						 i--,
						 sortedptr++)
						if
						  (!TEST_FLAG
						   (TheFlag,
						    (*sortedptr)->Data)
						   &&
						   LinePadIntersect
						   (Line,
						    (*sortedptr)->Data))
						  ADD_PAD_TO_LIST
						    (layer,
						     *sortedptr)}
						  }
						  return
						    (False);
					    }

static
  Boolean
  LOTouchesLine
  (LineTypePtr
   Line,
   Cardinal
   LayerGroup)
{
  Cardinal
    entry;
  Position
    xlow,
    xhigh;
  Cardinal
    i;

  /* the maximum possible distance */


  xlow
    =
    MIN
    (Line->
     Point1.
     X,
     Line->
     Point2.
     X)
    -
    MAX
    (Line->
     Thickness
     +
     MAX
     (MAX_PADSIZE,
      MAX_LINESIZE)
     /
     2
     -
     Bloat,
     0);


  xhigh
    =
    MAX
    (Line->
     Point1.
     X,
     Line->
     Point2.
     X)
    +
    MAX
    (Line->
     Thickness
     +
     MAX
     (MAX_PADSIZE,
      MAX_LINESIZE)
     /
     2
     +
     Bloat,
     0);

  /* loop over all layers of the group */
  for
    (entry
     =
     0;
     entry
     <
	     PCB->
	     LayerGroups.
	     Number
	     [LayerGroup];
	     entry++)
	    {
	      Cardinal
		layer;



		      layer
			=
			PCB->
			
			LayerGroups.
			
			Entries
			[LayerGroup]
			[entry];

		      /* handle normal layers */
		      if
			(layer
			 <
			 MAX_LAYER)
			{
			  PolygonTypePtr
			    polygon;
			  LineTypePtr
			    *
			    sortedptr;
			  ArcTypePtr
			    *
			    sortedarc;

			  /* get index of the first line that may match the coordinates */


			  sortedptr
			    =
			    GetIndexOfLines
			    (xlow,
			     xhigh,
			     layer,
			     &i);

			  /* loop till the end of the data is reached
			   * DONT RETRY LINES THAT HAVE BEEN FOUND
			   */
			  for
			    (;
			     i;
			     i--,
			     sortedptr++)
			    if
			      (!TEST_FLAG
			       (TheFlag,
				*sortedptr))
			      {
				if
				  (LineLineIntersect
				   (Line,
				    *sortedptr))
				  return
				    (True);
			      }
			  sortedarc
			    =
			    GetIndexOfArcs
			    (xlow,
			     xhigh,
			     layer,
			     &i);
			  for
			    (;
			     i;
			     i--,
			     sortedarc++)
			    if
			      (!TEST_FLAG
			       (TheFlag,
				*sortedarc)
			       &&
			       LineArcIntersect
			       (Line,
				*sortedarc))
			      return
				(True);

			  /* now check all polygons */
			  i
			    =
			    0;
			  polygon
			    =
			    PCB->
			    Data->
			    Layer
			    [layer].
			    Polygon;
			  for
			    (;
			     i
			     <
			     PCB->
			     Data->
			     Layer
			     [layer].
			     PolygonN;
			     i++,
			     polygon++)
			    if
			      (!TEST_FLAG
			       (TheFlag,
				polygon)
			       &&
			       IsLineInPolygon
			       (Line,
				polygon))
			      return
				(True);
			}
		      else
			{
							  /* handle special 'pad' layers */
							  PadDataTypePtr
											    *
											    sortedptr;

											  layer
											    -=
											    MAX_LAYER;
											  sortedptr
											    =
											    GetIndexOfPads
											    (xlow,
											     xhigh,
											     layer,
											     &i);
											  for
											    (;
											     i;
											     i--,
											     sortedptr++)
											    if
											      (!TEST_FLAG
											       (TheFlag,
												(*sortedptr)->Data)
											       &&
											       LinePadIntersect
											       (Line,
												(*sortedptr)->Data))
											      return
												(True);
											}
										    }
										  return
										    (False);
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
 /* loop over all layers of this group */
  for (entry = 0; entry < PCB-> LayerGroups.Number[LayerGroup]; entry++)
    {
      Cardinal layer, i;

      layer = PCB-> LayerGroups.  Entries [LayerGroup] [entry]; 
										      /* handle normal layers 
	  rats don't ever touch polygons
        or arcs by definition
       */

										      if (layer < MAX_LAYER)
        {
        LineTypePtr * sortedptr;
/* get index of the first line that may match the coordinates */ 
        sortedptr = GetIndexOfLines (Point-> X, Point-> X, layer, &i); 
											  /* loop till the end of the data is reached
	   * DONT RETRY LINES THAT HAVE BEEN FOUND
	   */
	  for (; i; i--, sortedptr++)
	    if (!TEST_FLAG (TheFlag, *sortedptr) &&
                ( ( (*sortedptr)->Point1.X == Point->X &&
                (*sortedptr)->Point1.Y == Point->Y) ||
                ((*sortedptr)->Point2.X == Point->X &&
                (*sortedptr)->Point2.Y == Point->Y)))
             ADD_LINE_TO_LIST (layer, *sortedptr)
        }
        else
        { /* handle special 'pad' layers */
												PadDataTypePtr * sortedptr; 
												layer -= MAX_LAYER;
                sortedptr = GetIndexOfPads (Point-> X, Point-> X, layer, &i);
		for (; i; i--, sortedptr++)
		  /* rats always touch points of a pad */
		  /* if they touch at all */
		  if (!TEST_FLAG (TheFlag, (*sortedptr)->Data) &&
                     ((((*sortedptr)->Data->Point1.X == Point->X &&
                     (*sortedptr)->Data->Point1.Y == Point->Y)) ||
                     (((*sortedptr)->Data->Point2.X == Point->X &&
                     (*sortedptr)->Data->Point2.Y == Point->Y))))
												    ADD_PAD_TO_LIST (layer, *sortedptr)
          }
    }
    return (False);
}

/* ---------------------------------------------------------------------------
 * searches all LOs that are connected to the given pad on the given
 * layergroup. All found connections are added to the list
 *
 * the notation that is used is:
 * Xij means Xj at line i
 */
	  static
	    Boolean LookupLOConnectionsToPad (PadTypePtr Pad, Cardinal LayerGroup)
	  {
	    Cardinal entry, i;
	    Position xlow, xhigh;
	    RatTypePtr * sortedrat;
	    LineTypePtr Line = (LineTypePtr) Pad;

	    if (!TEST_FLAG (SQUAREFLAG, Pad)) 
	      return (LookupLOConnectionsToLine (Line, LayerGroup, False));

	    /* add the new rat lines */

	    sortedrat = GetIndexOfRats (Pad-> Point1.  X, Pad-> Point2.  X, &i);
	    for (; i; i--, sortedrat++)
	      if (!TEST_FLAG (TheFlag, *sortedrat))
		{
		  RatTypePtr rat = *sortedrat;

		  if (rat->group1 == LayerGroup &&
                      (rat->Point1.X == Pad->Point1.X || rat->Point1.X == Pad->Point2.X) &&
                      (rat->Point1.Y == Pad->Point1.Y || rat->Point1.Y == Pad->Point2.Y))

		    ADD_RAT_TO_LIST (rat)
		  else
		    if (rat-> group2 == LayerGroup &&
                      (rat->Point2.X == Pad->Point1.X || rat->Point2.X == Pad->Point2.X) &&
                      (rat->Point2.Y == Pad->Point1.Y || rat->Point2.Y == Pad->Point2.Y))

		    ADD_RAT_TO_LIST (rat)
                }

		    /* the maximum possible distance */
		    xlow = MIN (Line-> Point1.  X, Line-> Point2.  X) -
                           MAX (Line-> Thickness + MAX (MAX_PADSIZE, MAX_LINESIZE) / 2 - Bloat, 0);
		  xhigh = MAX (Line-> Point1.  X, Line-> Point2.  X) +
                           MAX (Line-> Thickness + MAX (MAX_PADSIZE, MAX_LINESIZE) / 2 + Bloat, 0);

		  /* loop over all layers of the group */
		  for (entry = 0; entry < PCB-> LayerGroups.  Number [LayerGroup]; entry++)
		    {
		      Cardinal layer, i;

		      layer = PCB-> LayerGroups.  Entries [LayerGroup] [entry]; 
		      /* handle normal layers */
		      if (layer < MAX_LAYER)
			{
			  LineTypePtr * sortedptr;
			  ArcTypePtr * sortedarc;

			  /* get index of the first line that may match the coordinates */
			  sortedptr = GetIndexOfLines (xlow, xhigh, layer, &i);

			  /* loop till the end of the data is reached
			   * DONT RETRY LINES THAT HAVE BEEN FOUND
			   */
			  for (; i; i--, sortedptr++)
			    if (!TEST_FLAG (TheFlag, *sortedptr))
			      if (LinePadIntersect (*sortedptr, Line))
				ADD_LINE_TO_LIST (layer, *sortedptr)
			  sortedarc = GetIndexOfArcs (xlow, xhigh, layer, &i);
			  for (; i; i--, sortedarc++)
			    if (!TEST_FLAG (TheFlag, *sortedarc) &&
                                ArcPadIntersect (Line, *sortedarc))
			      ADD_ARC_TO_LIST (layer, *sortedarc);
			}
		      else
			{
			  /* handle special 'pad' layers */
			  PadDataTypePtr * sortedptr;

			  layer -= MAX_LAYER;
			  sortedptr = GetIndexOfPads (xlow, xhigh, layer, &i);
			  for (; i; i--, sortedptr++)
			    if (!TEST_FLAG (TheFlag, (*sortedptr)->Data) &&
			       LinePadIntersect (Line, (*sortedptr)->Data))
			      ADD_PAD_TO_LIST (layer, *sortedptr);
			}
		    }
		  return
		    (False);
		}

/* ---------------------------------------------------------------------------
 * looks up LOs that are connected to the given polygon
 * on the given layergroup. All found connections are added to the list
 */
    static Boolean LookupLOConnectionsToPolygon (PolygonTypePtr
	  Polygon, Cardinal LayerGroup)
  {
    Cardinal entry;
/* loop over all layers of the group */
    for (entry = 0; entry < PCB-> LayerGroups.  Number [LayerGroup]; entry++)
	{
	  Cardinal layer, i;

	  layer = PCB-> LayerGroups.  Entries [LayerGroup] [entry];

	  /* handle normal layers */
	  if (layer < MAX_LAYER)
	    {
	      PolygonTypePtr polygon;
	      LineTypePtr * sortedptr;
	      ArcTypePtr * sortedarc;

	      /* check all polygons */

	      polygon = PCB-> Data-> Layer [layer].  Polygon;
	      for (i = 0; i < PCB-> Data-> Layer [layer].  PolygonN; i++, polygon++)
		if (!TEST_FLAG (TheFlag, polygon) && IsPolygonInPolygon (polygon, Polygon))
		  ADD_POLYGON_TO_LIST (layer, polygon);

	      /* and now check all lines, first reduce the number of lines by
	       * evaluating the coordinates from the sorted lists
	       */

	      sortedptr = GetIndexOfLines (Polygon-> BoundingBox.  X1 - MAX_LINESIZE / 2,
                          Polygon-> BoundingBox.  X2 + MAX_LINESIZE / 2, layer, &i);

	      /* now check all lines that match the condition */
	      for (; i; i--, sortedptr++)
		if (!TEST_FLAG (TheFlag, *sortedptr) && IsLineInPolygon (*sortedptr, Polygon))
		  ADD_LINE_TO_LIST (layer, *sortedptr);

	      /* and now check all arcs, first reduce the number of arcs by
	       * evaluating the coordinates from the sorted lists
	       */

	      sortedarc = GetIndexOfArcs (Polygon-> BoundingBox.  X1 - MAX_LINESIZE / 2,
		 Polygon-> BoundingBox.  X2 + MAX_LINESIZE / 2, layer, &i);

	      /* now check all arcs that match the condition */
	      for (; i; i--, sortedarc++) if (!TEST_FLAG (TheFlag, *sortedarc) && IsArcInPolygon (*sortedarc, Polygon))
		  ADD_ARC_TO_LIST (layer, *sortedarc);
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
    static Boolean IsArcInPolygon (ArcTypePtr Arc, PolygonTypePtr Polygon)
    {
      BoxTypePtr Box;

      /* arcs with clearance never touch polys */
      if (TEST_FLAG (CLEARLINEFLAG, Arc)) 
	return (False);

      Box = GetObjectBoundingBox (ARC_TYPE, (void *) Arc, (void *) Arc, (void *) Arc);
      if (Box->X1 <= Polygon->BoundingBox.X2 && Box->X2 >= Polygon->BoundingBox.X1
	 && Box->Y1 <= Polygon->BoundingBox.Y2 && Box->Y2 >= Polygon->BoundingBox.Y1)
	{
	  LineType line; 

	  Box = GetArcEnds (Arc);
	  if (IsPointInPolygon (Box->X1, Box->Y1, MAX (0.5 * Arc->Thickness + fBloat, 0.0), Polygon)
	     || IsPointInPolygon (Box->X2, Box->Y2, MAX (0.5 * Arc->Thickness + fBloat, 0.0), Polygon))
	    return (True);

	  /* check all lines, start with the connection of the first-last
	   * polygon point; POLYGONPOINT_LOOP decrements the pointers !!!
	   */

	  line.  Point1 = Polygon-> Points [0];
	  line.  Thickness = 0; 
	  line.  Flags = NOFLAG;

	  POLYGONPOINT_LOOP (Polygon,
	     line.  Point2.  X = point-> X;
	     line.  Point2.  Y = point-> Y;
	     if (LineArcIntersect (&line, Arc))
	     return (True);
	     line.  Point1.  X = line.  Point2.  X;
	     line.  Point1.  Y = line.  Point2.  Y;);
	}
      return
	(False);
    }

/* ---------------------------------------------------------------------------
 * checks if a line has a connection to a polygon
 *
 * - first check if the line can intersect with the polygon by
 *   evaluating the bounding boxes
 * - check the two end points of the line. If none of them matches
 * - check all segments of the polygon against the line.
 */
    Boolean IsLineInPolygon (LineTypePtr Line, PolygonTypePtr Polygon)
    {
      Position minx = MIN (Line-> Point1.  X, Line-> Point2.  X)
	- MAX (Line-> Thickness + Bloat, 0),
	maxx = MAX (Line-> Point1.  X, Line-> Point2.  X)
	+ MAX (Line-> Thickness + Bloat, 0),
	miny = MIN (Line-> Point1.  Y, Line-> Point2.  Y)
	- MAX (Line-> Thickness + Bloat, 0),
	maxy = MAX (Line-> Point1.  Y, Line-> Point2.  Y)
	+ MAX (Line-> Thickness + Bloat, 0);
      /* lines with clearance never touch polygons */
      if (TEST_FLAG (CLEARLINEFLAG, Line))
	return (False);
      if (minx <= Polygon->BoundingBox.X2 && maxx >= Polygon->BoundingBox.X1
	 && miny <= Polygon->BoundingBox.Y2 && maxy >= Polygon->BoundingBox.Y1)
	{
	  LineType line; 
	  if (IsPointInPolygon (Line->Point1.X, Line->Point1.Y,
	      MAX (0.5 * Line->Thickness + fBloat, 0.0), Polygon)
	     || IsPointInPolygon (Line->Point2.X, Line->Point2.Y,
	      MAX (0.5 * Line->Thickness + fBloat, 0.0), Polygon))
	    return (True);

	  /* check all lines, start with the connection of the first-last
	   * polygon point; POLYGONPOINT_LOOP decrements the pointers !!!
	   */

	  line.  Point1 = Polygon-> Points [0];
	  line.  Thickness = 0; 
	  line.  Flags = NOFLAG;

	  POLYGONPOINT_LOOP (Polygon,
	     line.  Point2.  X = point-> X;
	     line.  Point2.  Y = point-> Y;
	     if (LineLineIntersect (Line, &line))
	     return (True);
	     line.  Point1.  X = line.  Point2.  X;
	     line.  Point1.  Y = line.  Point2.  Y;);
	}
      return (False);
    }

/* ---------------------------------------------------------------------------
 * checks if a polygon has a connection to a second one
 *
 * First check all points out of P1 against P2 and vice versa.
 * If both fail check all lines of P1 agains the ones of P2
 */
											    Boolean
											      IsPolygonInPolygon
											      (PolygonTypePtr
											       P1,
											       PolygonTypePtr
											       P2)
											    {
											      /* first check if both bounding boxes intersect */
											      if
												(P1->BoundingBox.X1
												 <=
												 P2->BoundingBox.X2
												 &&
												 P1->BoundingBox.X2
												 >=
												 P2->BoundingBox.X1
												 &&
												 P1->BoundingBox.Y1
												 <=
												 P2->BoundingBox.Y2
												 &&
												 P1->BoundingBox.Y2
												 >=
												 P2->BoundingBox.Y1)
												{
												  LineType
												    line;


												  POLYGONPOINT_LOOP
												    (P1,
												     if
												     (IsPointInPolygon
												      (point->
												       X,
												       point->
												       Y,
												       0,
												       P2))
												     return
												     (True););

												  POLYGONPOINT_LOOP
												    (P2,
												     if
												     (IsPointInPolygon
												      (point->
												       X,
												       point->
												       Y,
												       0,
												       P1))
												     return
												     (True););

												  /* check all lines of P1 agains P2;
												   * POLYGONPOINT_LOOP decrements the pointer !!!
												   */


												  line.
												    Point1.
												    X
												    =
												    P1->
												    Points
												    [0].
												    X;


												  line.
												    Point1.
												    Y
												    =
												    P1->
												    Points
												    [1].
												    Y;


												  line.
												    Thickness
												    =
												    0;


												  line.
												    Flags
												    =
												    NOFLAG;

												  POLYGONPOINT_LOOP
												    (P1,
												     line.
												     Point2.
												     X
												     =
												     point->
												     X;
												     line.
												     Point2.
												     Y
												     =
												     point->
												     Y;
												     if
												     (IsLineInPolygon
												      (&line,
												       P2))
												     return
												     (True);
												     line.
												     Point1.
												     X
												     =
												     line.
												     Point2.
												     X;
												     line.
												     Point1.
												     Y
												     =
												     line.
												     Point2.
												     Y;);
												}
											      return
												(False);
											    }

/* ---------------------------------------------------------------------------
 * writes the several names of an element to a file
 */
											    static
											      void
											      PrintElementNameList
											      (ElementTypePtr
											       Element,
											       FILE
											       *
											       FP)
											    {
											      static
												DynamicStringType
												cname,
												pname,
												vname;



											        CreateQuotedString
												(&cname,
												 EMPTY
												 (DESCRIPTION_NAME
												  (Element)));


											        CreateQuotedString
												(&pname,
												 EMPTY
												 (NAMEONPCB_NAME
												  (Element)));


											        CreateQuotedString
												(&vname,
												 EMPTY
												 (VALUE_NAME
												  (Element)));


											        fprintf
												(FP,
												 "(%s %s %s)\n",
												 cname.
												 Data,
												 pname.
												 Data,
												 vname.
												 Data);
											    }

/* ---------------------------------------------------------------------------
 * writes the several names of an element to a file
 */
											    static
											      void
											      PrintConnectionElementName
											      (ElementTypePtr
											       Element,
											       FILE
											       *
											       FP)
											    {
											      fputs
												("Element",
												 FP);
											      PrintElementNameList
												(Element,
												 FP);
											      fputs
												("{\n",
												 FP);
											    }

/* ---------------------------------------------------------------------------
 * prints one {pin,pad,via}/element entry of connection lists
 */
											    static
											      void
											      PrintConnectionListEntry
											      (char
											       *ObjName,
											       ElementTypePtr
											       Element,
											       Boolean
											       FirstOne,
											       FILE
											       *
											       FP)
											    {
											      static
												DynamicStringType
												oname;



											        CreateQuotedString
												(&oname,
												 ObjName);
											      if
												(FirstOne)


												  fprintf
												  (FP,
												   "\t%s\n\t{\n",
												   oname.
												   Data);
											      else
												{
												  fprintf
												    (FP,
												     "\t\t%s ",
												     oname.
												     Data);
												  if
												    (Element)
												    PrintElementNameList
												      (Element,
												       FP);
												  else
												    fputs
												      ("(__VIA__)\n",
												       FP);
												}
											    }

/* ---------------------------------------------------------------------------
 * prints all found connections of a pads to file FP
 * the connections are stacked in 'PadList'
 */
											    static
											      void
											      PrintPadConnections
											      (Cardinal
											       Layer,
											       FILE
											       *
											       FP,
											       Boolean
											       IsFirst)
											    {
											      Cardinal
												i;
											      PadDataTypePtr
												ptr;

											      if
												(!PadList
												 [Layer].Number)
												return;

											      /* the starting pad */
											      if
												(IsFirst)
												{
												  ptr
												    =
												    PADLIST_ENTRY
												    (Layer,
												     0);
												  PrintConnectionListEntry
												    (UNKNOWN
												     (ptr->
												      Data->
												      Name),
												     NULL,
												     True,
												     FP);
												}

											      /* we maybe have to start with i=1 if we are handling the
											       * starting-pad itself
											       */
											      for
												(i
												 =
												 IsFirst
												 ?
												 1
												 :
												 0;
												 i
												 <
												 PadList
												 [Layer].
												 Number;
												 i++)
												{
												  ptr
												    =
												    PADLIST_ENTRY
												    (Layer,
												     i);
												  PrintConnectionListEntry
												    (EMPTY
												     (ptr->
												      Data->
												      Name),
												     ptr->
												     Element,
												     False,
												     FP);
												}
											    }

/* ---------------------------------------------------------------------------
 * prints all found connections of a pin to file FP
 * the connections are stacked in 'PVList'
 */
											    static
											      void
											      PrintPinConnections
											      (FILE
											       *
											       FP,
											       Boolean
											       IsFirst)
											    {
											      Cardinal
												i;
											      PVDataTypePtr
												pv;

											      if
												(!PVList.Number)
												return;

											      if
												(IsFirst)
												{
												  /* the starting pin */
												  pv
												    =
												    PVLIST_ENTRY
												    (0);
												  PrintConnectionListEntry
												    (EMPTY
												     (pv->
												      Data->
												      Name),
												     NULL,
												     True,
												     FP);
												}

											      /* we maybe have to start with i=1 if we are handling the
											       * starting-pin itself
											       */
											      for
												(i
												 =
												 IsFirst
												 ?
												 1
												 :
												 0;
												 i
												 <
												 PVList.
												 Number;
												 i++)
												{
												  /* get the elements name or assume that its a via */
												  pv
												    =
												    PVLIST_ENTRY
												    (i);
												  PrintConnectionListEntry
												    (EMPTY
												     (pv->
												      Data->
												      Name),
												     pv->
												     Element,
												     False,
												     FP);
												}
											    }

/* ---------------------------------------------------------------------------
 * checks if all lists of new objects are handled
 */
    static
      Boolean
      ListsEmpty
      (Boolean
       AndRats)
    {
      Boolean
	empty;
      int
	i;



        empty
	=
	(PVList.
	 Position
	 >=
	 PVList.
	 Number);
      if
	(AndRats)
	  empty
	  =
	  empty
	  &&
	  (RatList.
	   Position
	   >=
	   RatList.
	   Number);
      for
	(i
	 =
	 0;
	 i
	 <
	 MAX_LAYER
	 &&
	 empty;
	 i++)


	  empty
	  =
	  empty
	  &&
	  LineList
	  [i].
	  Position >= LineList[i].Number
	  &&
	  ArcList
	  [i].
	  
	  Position
	  >=
	  ArcList
	  [i].
	  Number && PolygonList[i].Position >=
	  PolygonList
	  [i].
	  
	  Number;



        return
	(empty);
    }

/* ---------------------------------------------------------------------------
 * loops till no more connections are found 
 */
    static Boolean DoIt (Boolean AndRats, Boolean AndDraw)
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
      lineClear
      (LineTypePtr
       line,
       Cardinal
       group)
    {
      if
	(LOTouchesLine
	 (line,
	  group))
	return
	  (False);
      if
	(PVTouchesLine
	 (line))
	return
	  (False);
      return
	(True);
    }

/* ---------------------------------------------------------------------------
 * prints all unused pins of an element to file FP
 */
    static
      Boolean
      PrintAndSelectUnusedPinsAndPadsOfElement
      (ElementTypePtr
       Element,
       FILE
       *
       FP)
    {
      Boolean
	first
	=
	True;
      Cardinal
	number;
      static
	DynamicStringType
	oname;

      /* check all pins in element */



        PIN_LOOP
	(Element,
	 if
	 (!TEST_FLAG
	  (HOLEFLAG,
	   pin))
	 {
	 PVDataTypePtr
	 entry;
	 /* lookup pin in list */
	 entry
	 =
	 LookupPVByCoordinates
	 (1,
	  pin->
	  X,
	  pin->
	  Y);
	 /* pin might has bee checked before, add to list if not */
	 if
	 (!TEST_FLAG
	  (TheFlag,
	   entry->
	   Data)
	  &&
	  FP)
	 {
	 int
	 i;
	 ADD_PV_TO_LIST
	 (entry);
	 DoIt
	 (True,
	  True);
	 number
	 =
	 PadList
	 [COMPONENT_LAYER].
	 Number
	 +
	 PadList
	 [SOLDER_LAYER].
	 Number
	 +
	 PVList.
	 Number;
	 /* the pin has no connection if it's the only
	  * list entry; don't count vias
	  */
	 for
	 (i
	  =
	  0;
	  i
	  <
	  PVList.
	  Number;
	  i++)
	 if
	 (!PVLIST_ENTRY
	  (i)->
	  Element)
	 number--;
	 if
	 (number
	  ==
	  1)
	 {
	 /* output of element name if not already done */
	 if
	 (first)
	 {
	 PrintConnectionElementName
	 (Element,
	  FP);
	 first
	 =
	 False;}

	 /* write name to list and draw selected object */
	 CreateQuotedString
	 (&oname,
	  EMPTY
	  (entry->
	   Data->
	   Name));
	 fprintf
	 (FP,
	  "\t%s\n",
	  oname.
	  Data);
	 SET_FLAG
	 (SELECTEDFLAG,
	  entry->
	  Data);
	 DrawPin
	 (entry->
	  Data,
	  0);}

	 /* reset found objects for the next pin */
	 if
	 (PrepareNextLoop
	  (FP))
	 return
	 (True);}
	 }
      );

      /* check all pads in element */
      PAD_LOOP
	(Element,
	 {
	 PadDataTypePtr
	 entry;
	 /* lookup pin in list */
	 entry
	 =
	 LookupPadByAddress
	 (pad);
	 /* pin might has bee checked before, add to list if not */
	 if
	 (!TEST_FLAG
	  (TheFlag,
	   entry->
	   Data)
	  &&
	  FP)
	 {
	 int
	 i;
	 ADD_PAD_TO_LIST
	 (TEST_FLAG
	  (ONSOLDERFLAG,
	   pad)
	  ?
	  SOLDER_LAYER
	  :
	  COMPONENT_LAYER,
	  entry);
	 DoIt
	 (True,
	  True);
	 number
	 =
	 PadList
	 [COMPONENT_LAYER].
	 Number
	 +
	 PadList
	 [SOLDER_LAYER].
	 Number
	 +
	 PVList.
	 Number;
	 /* the pin has no connection if it's the only
	  * list entry; don't count vias
	  */
	 for
	 (i
	  =
	  0;
	  i
	  <
	  PVList.
	  Number;
	  i++)
	 if
	 (!PVLIST_ENTRY
	  (i)->
	  Element)
	 number--;
	 if
	 (number
	  ==
	  1)
	 {
	 /* output of element name if not already done */
	 if
	 (first)
	 {
	 PrintConnectionElementName
	 (Element,
	  FP);
	 first
	 =
	 False;}

	 /* write name to list and draw selected object */
	 CreateQuotedString
	 (&oname,
	  EMPTY
	  (entry->
	   Data->
	   Name));
	 fprintf
	 (FP,
	  "\t%s\n",
	  oname.
	  Data);
	 SET_FLAG
	 (SELECTEDFLAG,
	  entry->
	  Data);
	 DrawPad
	 (entry->
	  Data,
	  0);}

	 /* reset found objects for the next pin */
	 if
	 (PrepareNextLoop
	  (FP))
	 return
	 (True);}
	 }
      );

      /* print seperator if element has unused pins or pads */
      if
	(!first)
	{
	  fputs
	    ("}\n\n",
	     FP);
	  SEPERATE
	    (FP);
	}
      return
	(False);
    }

/* ---------------------------------------------------------------------------
 * resets some flags for looking up the next pin/pad
 */
											    static
											      Boolean
											      PrepareNextLoop
											      (FILE
											       *
											       FP)
											    {
											      Cardinal
												layer;

											      /* reset found LOs for the next pin */
											      for
												(layer
												 =
												 0;
												 layer
												 <
												 MAX_LAYER;
												 layer++)
												{
												  LineList
												    [layer].
												    Position
												    =
												    LineList
												    [layer].
												    Number
												    =
												    0;
												  PolygonList
												    [layer].
												    Position =
												    PolygonList
												    [layer].
												    Number =
												    0;
												}

											      /* reset found pads */
											      for
												(layer
												 =
												 0;
												 layer
												 <
												 2;
												 layer++)
												PadList
												  [layer].
												  Position
												  =
												  PadList
												  [layer].
												  Number
												  =
												  0;

											      /* reset PVs */
											      PVList.
												Number
												=
												PVList.
												Position
												=
												0;

											      /* check if abort buttons has been pressed */
											      if
												(CheckAbort
												 ())
												{
												  if
												    (FP)
												    fputs
												      ("\n\nABORTED...\n",
												       FP);
												  return
												    (True);
												}
											      return
												(False);
											    }

/* ---------------------------------------------------------------------------
 * finds all connections to the pins of the passed element.
 * The result is written to filedescriptor 'FP'
 * Returns True if operation was aborted
 */
											    static
											      Boolean
											      PrintElementConnections
											      (ElementTypePtr
											       Element,
											       FILE
											       *
											       FP,
											       Boolean
											       AndDraw)
											    {
											      PrintConnectionElementName
												(Element,
												 FP);

											      /* check all pins in element */
											      PIN_LOOP
												(Element,
												 {
												 PVDataTypePtr
												 entry;
												 /* lookup pin in list */
												 entry
												 =
												 LookupPVByCoordinates
												 (1,
												  pin->
												  X,
												  pin->
												  Y);
												 /* pin might have been checked before, add to list if not */
												 if
												 (TEST_FLAG
												  (TheFlag,
												   entry->
												   Data))
												 {
												 PrintConnectionListEntry
												 (EMPTY
												  (pin->
												   Name),
												  NULL,
												  True,
												  FP);
												 fputs
												 ("\t\t__CHECKED_BEFORE__\n\t}\n",
												  FP);
												 continue;}
												 ADD_PV_TO_LIST
												 (entry);
												 DoIt
												 (True,
												  AndDraw);
												 /* printout all found connections */
												 PrintPinConnections
												 (FP,
												  True);
												 PrintPadConnections
												 (COMPONENT_LAYER,
												  FP,
												  False);
												 PrintPadConnections
												 (SOLDER_LAYER,
												  FP,
												  False);
												 fputs
												 ("\t}\n",
												  FP);
												 if
												 (PrepareNextLoop
												  (FP))
												 return
												 (True);}
											      );

											      /* check all pads in element */
											      PAD_LOOP
												(Element,
												 {
												 PadDataTypePtr
												 entry;
												 Cardinal
												 layer;
												 /* pad might has bee checked before, add to list if not */
												 if
												 (TEST_FLAG
												  (TheFlag,
												   pad))
												 {
												 PrintConnectionListEntry
												 (EMPTY
												  (pad->
												   Name),
												  NULL,
												  True,
												  FP);
												 fputs
												 ("\t\t__CHECKED_BEFORE__\n\t}\n",
												  FP);
												 continue;}
												 entry
												 =
												 LookupPadByAddress
												 (pad);
												 layer
												 =
												 TEST_FLAG
												 (ONSOLDERFLAG,
												  pad)
												 ?
												 SOLDER_LAYER
												 :
												 COMPONENT_LAYER;
												 ADD_PAD_TO_LIST
												 (layer,
												  entry);
												 DoIt
												 (True,
												  AndDraw);
												 /* print all found connections */
												 PrintPadConnections
												 (layer,
												  FP,
												  True);
												 PrintPadConnections
												 (layer
												  ==
												  COMPONENT_LAYER
												  ?
												  SOLDER_LAYER
												  :
												  COMPONENT_LAYER,
												  FP,
												  False);
												 PrintPinConnections
												 (FP,
												  False);
												 fputs
												 ("\t}\n",
												  FP);
												 if
												 (PrepareNextLoop
												  (FP))
												 return
												 (True);}
											      );
											      fputs
												("}\n\n",
												 FP);
											      return
												(False);
											    }

/* ---------------------------------------------------------------------------
 * draws all new connections which have been found since the
 * routine was called the last time
 */
											    static
											      void
											      DrawNewConnections
											      (void)
											    {
											      int
												i;
											        Cardinal
												position;

											      /* decrement 'i' to keep layerstack order */
											      for
												(i
												 =
												 MAX_LAYER
												 -
												 1;
												 i
												 !=
												 -1;
												 i--)
												{
												  Cardinal
												    layer
												    =
												    LayerStack
												    [i];

												  if
												    (PCB->Data->Layer
												     [layer].On)
												    {
												      /* draw all new lines */
												      position
													=
													LineList
													[layer].
													DrawPosition;
												      for
													(;
													 position
													 <
													 LineList
													 [layer].
													 Number;
													 position++)
													DrawLine
													  (&PCB->
													   Data->
													   Layer
													   [layer],
													   LINELIST_ENTRY
													   (layer,
													    position),
													   0);
												      LineList
													[layer].
													DrawPosition =
													LineList
													[layer].
													
													Number;

												      /* draw all new arcs */
												      position
													=
													ArcList
													[layer].
													
													DrawPosition;
												      for
													(;
													 position
													 <
													 ArcList
													 [layer].
													 Number;
													 position++)
													DrawArc
													  (&PCB->
													   Data->
													   Layer
													   [layer],
													   ARCLIST_ENTRY
													   (layer,
													    position),
													   0);
												      ArcList
													[layer].
													DrawPosition =
													ArcList
													[layer].
													
													Number;

												      /* draw all new polygons */
												      position
													=
													PolygonList
													[layer].
													
													DrawPosition;
												      for
													(;
													 position
													 <
													 PolygonList
													 [layer].
													 Number;
													 position++)
													DrawPolygon
													  (&PCB->
													   Data->
													   Layer
													   [layer],
													   POLYGONLIST_ENTRY
													   (layer,
													    position),
													   0);
												      PolygonList
													[layer].
													DrawPosition =
													PolygonList
													[layer].
													
													Number;
												    }
												}

											      /* draw all new pads */
											      if
												(PCB->PinOn)
												for
												  (i
												   =
												   0;
												   i
												   <
												   2;
												   i++)
												  {
												    position
												      =
												      PadList
												      [i].
												      DrawPosition;

												    for
												      (;
												       position
												       <
												       PadList
												       [i].
												       Number;
												       position++)
												      DrawPad
													(PADLIST_ENTRY
													 (i,
													  position)->
													 Data,
													 0);
												    PadList
												      [i].
												      DrawPosition
												      =
												      PadList
												      [i].
												      Number;
												  }

											      /* draw all new PVs; 'PVList' holds a list of pointers to the
											       * sorted array pointers to PV data
											       */
											      while
												(PVList.
												 DrawPosition
												 <
												 PVList.
												 Number)
												{
												  PVDataTypePtr
												    pv
												    =
												    PVLIST_ENTRY
												    (PVList.
												     DrawPosition);

												  if
												    (TEST_FLAG
												     (PINFLAG,
												      pv->Data))
												    {
												      if
													(PCB->PinOn)
													DrawPin
													  (pv->
													   Data,
													   0);
												    }
												  else
												    if
												    (PCB->
												     ViaOn)
												    DrawVia
												      (pv->
												       Data,
												       0);
												  PVList.
												    DrawPosition++;
												}
											      /* draw the new rat-lines */
											      if
												(PCB->RatOn)
												{
												  position
												    =
												    RatList.
												    DrawPosition;
												  for
												    (;
												     position
												     <
												     RatList.
												     Number;
												     position++)
												    DrawRat
												      (RATLIST_ENTRY
												       (position),
												       0);
												  RatList.
												    DrawPosition
												    =
												    RatList.
												    Number;
												}
											    }

/* ---------------------------------------------------------------------------
 * find all connections to pins within one element
 */
											    void
											      LookupElementConnections
											      (ElementTypePtr
											       Element,
											       FILE
											       *
											       FP)
											    {
											      /* reset all currently marked connections */
											      CreateAbortDialog
												("Press button to abort connection scan");
											      User
												=
												True;
											      ResetConnections
												(True);
											      InitConnectionLookup
												();
											      PrintElementConnections
												(Element,
												 FP,
												 True);
											      SetChangedFlag
												(True);
											      EndAbort
												();
											      if
												(Settings.RingBellWhenFinished)
												Beep
												  (Settings.
												   Volume);
											      FreeConnectionLookupMemory
												();
											      IncrementUndoSerialNumber
												();
											      User
												=
												False;
											      Draw
												();
											    }

/* ---------------------------------------------------------------------------
 * find all connections to pins of all element
 */
											    void
											      LookupConnectionsToAllElements
											      (FILE
											       *
											       FP)
											    {
											      /* reset all currently marked connections */
											      User
												=
												False;
											      CreateAbortDialog
												("Press button to abort connection scan");
											      ResetConnections
												(False);
											      InitConnectionLookup
												();

											      ELEMENT_LOOP
												(PCB->
												 Data,
												 /* break if abort dialog returned True */
												 if
												 (PrintElementConnections
												  (element,
												   FP,
												   False))
												 break;
												 SEPERATE
												 (FP);
												 if
												 (Settings.
												  ResetAfterElement
												  &&
												  n
												  !=
												  1)
												 ResetConnections
												 (False););
											      EndAbort
												();
											      if
												(Settings.RingBellWhenFinished)
												Beep
												  (Settings.
												   Volume);
											      ResetConnections
												(False);
											      FreeConnectionLookupMemory
												();
											      RedrawOutput
												();
											    }

/*---------------------------------------------------------------------------
 * add the starting object to the list of found objects
 */
											    static
											      Boolean
											      ListStart
											      (int
											       type,
											       void
											       *ptr1,
											       void
											       *ptr2,
											       void
											       *ptr3)
											    {
											      switch
												(type)
												{
												  case
												PIN_TYPE:
												  {
												    PVDataTypePtr
												      entry;

												    /* use center coordinates else LookupPVByCoordinates()
												     * may fail;
												     * bug-fix by Ulrich Pegelow (ulrpeg@bigcomm.gun.de)
												     */


												    entry
												      =
												      LookupPVByCoordinates
												      (1,
												       (
													(PinTypePtr)
													ptr2)->X,
												       (
													(PinTypePtr)
													ptr2)->Y);

												    ADD_PV_TO_LIST
												      (entry);
												    break;
												  }

												  case
												    VIA_TYPE:
												  {
												    PVDataTypePtr
												      entry;

												    entry
												      =
												      LookupPVByCoordinates
												      (0,
												       (
													(PinTypePtr)
													ptr2)->X,
												       (
													(PinTypePtr)
													ptr2)->Y);
												    ADD_PV_TO_LIST
												      (entry);
												    break;
												  }

												  case
												RATLINE_TYPE:
												  {
												    ADD_RAT_TO_LIST
												      (
												       (RatTypePtr)
												       ptr1);
												    break;
												  }

												  case
												LINE_TYPE:
												  {
												    int
												      layer
												      =
												      GetLayerNumber
												      (PCB->
												       Data,
												       (LayerTypePtr)
												       ptr1);

												    ADD_LINE_TO_LIST
												      (layer,
												       (LineTypePtr)
												       ptr2);
												    break;
												  }

												  case
												ARC_TYPE:
												  {
												    int
												      layer
												      =
												      GetLayerNumber
												      (PCB->
												       Data,
												       (LayerTypePtr)
												       ptr1);

												    ADD_ARC_TO_LIST
												      (layer,
												       (ArcTypePtr)
												       ptr2);
												    break;
												  }

												  case
												POLYGON_TYPE:
												  {
												    int
												      layer
												      =
												      GetLayerNumber
												      (PCB->
												       Data,
												       (LayerTypePtr)
												       ptr1);

												    ADD_POLYGON_TO_LIST
												      (layer,
												       (PolygonTypePtr)
												       ptr2);
												    break;
												  }

												  case
												PAD_TYPE:
												  {
												    PadTypePtr
												      pad
												      =
												      (PadTypePtr)
												      ptr2;
												    PadDataTypePtr
												      entry;

												    entry
												      =
												      LookupPadByAddress
												      (pad);
												    ADD_PAD_TO_LIST
												      (TEST_FLAG
												       (ONSOLDERFLAG,
													pad)
												       ?
												       SOLDER_LAYER
												       :
												       COMPONENT_LAYER,
												       entry);
												    break;
												  }
												}
											      return
												(False);
											    }


/* ---------------------------------------------------------------------------
 * looks up all connections from the object at the given coordinates
 * the TheFlag (normally 'FOUNDFLAG') is set for all objects found
 * the objects are re-drawn if AndDraw is true
 * also the action is marked as undoable if AndDraw is true
 */
											    void
											      LookupConnection
											      (Position
											       X,
											       Position
											       Y,
											       Boolean
											       AndDraw,
											       Dimension
											       Range)
											    {
											      void
												*ptr1,
												*ptr2,
												*ptr3;
											      int
												type;

											      /* check if there are any pins or pads at that position */


											        type
												=
												SearchObjectByPosition
												(LOOKUP_FIRST,
												 &ptr1,
												 &ptr2,
												 &ptr3,
												 X,
												 Y,
												 Range);
											      if
												(type
												 ==
												 NO_TYPE)
												{
												  type
												    =
												    SearchObjectByPosition
												    (LOOKUP_MORE,
												     &ptr1,
												     &ptr2,
												     &ptr3,
												     X,
												     Y,
												     Range);
												  if
												    (type
												     ==
												     NO_TYPE)
												    return;
												  if
												    (type
												     &
												     SILK_TYPE)
												    {
												      int
													laynum
													=
													GetLayerNumber
													(PCB->
													 Data,
													 (LayerTypePtr)
													 ptr1);

												      /* don't mess with silk objects! */
												      if
													(laynum
													 >=
													 MAX_LAYER)
													  return;
												    }
												}
											      else


												  SetNetlist
												  (type,
												   ptr1,
												   ptr2,
												   True);
											      TheFlag
												=
												FOUNDFLAG;
											      User
												=
												AndDraw;
											      InitConnectionLookup
												();

											      /* now add the object to the appropriate list and start scanning
											       * This is step (1) from the description
											       */
											      ListStart
												(type,
												 ptr1,
												 ptr2,
												 ptr3);
											      DoIt
												(True,
												 AndDraw);
											      if
												(User)
												IncrementUndoSerialNumber
												  ();
											      User
												=
												False;

											      /* we are done */
											      if
												(AndDraw)
												Draw
												  ();
											      if
												(AndDraw
												 &&
												 Settings.RingBellWhenFinished)
												Beep
												  (Settings.
												   Volume);
											      FreeConnectionLookupMemory
												();
											    }

/* ---------------------------------------------------------------------------
 * find connections for rats nesting
 * assumes InitConnectionLookup() has already been done
 */
											    void
											      RatFindHook
											      (int
											       type,
											       void
											       *ptr1,
											       void
											       *ptr2,
											       void
											       *ptr3,
											       Boolean
											       undo,
											       Boolean
											       AndRats)
											    {
											      User
												=
												undo;
											      DumpList
												();
											      ListStart
												(type,
												 ptr1,
												 ptr2,
												 ptr3);
											      DoIt
												(AndRats,
												 False);
											      User
												=
												False;
											    }

/* ---------------------------------------------------------------------------
 * find all unused pins of all element
 */
											    void
											      LookupUnusedPins
											      (FILE
											       *
											       FP)
											    {
											      /* reset all currently marked connections */
											      User
												=
												True;
											      SaveUndoSerialNumber
												();
											      CreateAbortDialog
												("Press button to abort unused pin scan");
											      ResetConnections
												(True);
											      RestoreUndoSerialNumber
												();
											      InitConnectionLookup
												();

											      ELEMENT_LOOP
												(PCB->
												 Data,
												 /* break if abort dialog returned True;
												  * passing NULL as filedescriptor discards the normal output
												  */
												 if
												 (PrintAndSelectUnusedPinsAndPadsOfElement
												  (element,
												   FP))
												 break;);
											      EndAbort
												();

											      if
												(Settings.RingBellWhenFinished)
												Beep
												  (Settings.
												   Volume);
											      FreeConnectionLookupMemory
												();
											      IncrementUndoSerialNumber
												();
											      User
												=
												False;
											      Draw
												();
											    }

/* ---------------------------------------------------------------------------
 * resets all used flags of pins and vias
 */
											    void
											      ResetFoundPinsViasAndPads
											      (Boolean
											       AndDraw)
											    {
											      Boolean
												change
												=
												False;


											      VIA_LOOP
												(PCB->
												 Data,
												 if
												 (TEST_FLAG
												  (TheFlag,
												   via))
												 {
												 if
												 (AndDraw)
												 AddObjectToFlagUndoList
												 (VIA_TYPE,
												  via,
												  via,
												  via);
												 CLEAR_FLAG
												 (TheFlag,
												  via);
												 if
												 (AndDraw)
												 DrawVia
												 (via,
												  0);
												 change
												 =
												 True;}
											      );
											      ELEMENT_LOOP
												(PCB->
												 Data,
												 PIN_LOOP
												 (element,
												  if
												  (TEST_FLAG
												   (TheFlag,
												    pin))
												  {
												  if
												  (AndDraw)
												  AddObjectToFlagUndoList
												  (PIN_TYPE,
												   element,
												   pin,
												   pin);
												  CLEAR_FLAG
												  (TheFlag,
												   pin);
												  if
												  (AndDraw)
												  DrawPin
												  (pin,
												   0);
												  change
												  =
												  True;}
												 );
												 PAD_LOOP
												 (element,
												  if
												  (TEST_FLAG
												   (TheFlag,
												    pad))
												  {
												  if
												  (AndDraw)
												  AddObjectToFlagUndoList
												  (PAD_TYPE,
												   element,
												   pad,
												   pad);
												  CLEAR_FLAG
												  (TheFlag,
												   pad);
												  if
												  (AndDraw)
												  DrawPad
												  (pad,
												   0);
												  change
												  =
												  True;}
												 ););
											      if
												(change)
												{
												  SetChangedFlag
												    (True);
												  if
												    (AndDraw)
												    {
												      IncrementUndoSerialNumber
													();
												      Draw
													();
												    }
												}
											    }

/* ---------------------------------------------------------------------------
 * resets all used flags of LOs
 */
											    void
											      ResetFoundLinesAndPolygons
											      (Boolean
											       AndDraw)
											    {
											      Boolean
												change
												=
												False;


											      RAT_LOOP
												(PCB->
												 Data,
												 if
												 (TEST_FLAG
												  (TheFlag,
												   line))
												 {
												 if
												 (AndDraw)
												 AddObjectToFlagUndoList
												 (RATLINE_TYPE,
												  line,
												  line,
												  line);
												 CLEAR_FLAG
												 (TheFlag,
												  line);
												 if
												 (AndDraw)
												 DrawRat
												 (line,
												  0);
												 change
												 =
												 True;}
											      );
											      COPPERLINE_LOOP
												(PCB->
												 Data,
												 if
												 (TEST_FLAG
												  (TheFlag,
												   line))
												 {
												 if
												 (AndDraw)
												 AddObjectToFlagUndoList
												 (LINE_TYPE,
												  layer,
												  line,
												  line);
												 CLEAR_FLAG
												 (TheFlag,
												  line);
												 if
												 (AndDraw)
												 DrawLine
												 (layer,
												  line,
												  0);
												 change
												 =
												 True;}
											      );
											      COPPERARC_LOOP
												(PCB->
												 Data,
												 if
												 (TEST_FLAG
												  (TheFlag,
												   arc))
												 {
												 if
												 (AndDraw)
												 AddObjectToFlagUndoList
												 (ARC_TYPE,
												  layer,
												  arc,
												  arc);
												 CLEAR_FLAG
												 (TheFlag,
												  arc);
												 if
												 (AndDraw)
												 DrawArc
												 (layer,
												  arc,
												  0);
												 change
												 =
												 True;}
											      );
											      COPPERPOLYGON_LOOP
												(PCB->
												 Data,
												 if
												 (TEST_FLAG
												  (TheFlag,
												   polygon))
												 {
												 if
												 (AndDraw)
												 AddObjectToFlagUndoList
												 (POLYGON_TYPE,
												  layer,
												  polygon,
												  polygon);
												 CLEAR_FLAG
												 (TheFlag,
												  polygon);
												 if
												 (AndDraw)
												 DrawPolygon
												 (layer,
												  polygon,
												  0);
												 change
												 =
												 True;}
											      );
											      if
												(change)
												{
												  SetChangedFlag
												    (True);
												  if
												    (AndDraw)
												    {
												      IncrementUndoSerialNumber
													();
												      Draw
													();
												    }
												}
											    }

/* ---------------------------------------------------------------------------
 * resets all found connections
 */
											    static
											      void
											      ResetConnections
											      (Boolean
											       AndDraw)
											    {
											      if
												(AndDraw)
												SaveUndoSerialNumber
												  ();
											      ResetFoundPinsViasAndPads
												(AndDraw);
											      if
												(AndDraw)
												RestoreUndoSerialNumber
												  ();
											      ResetFoundLinesAndPolygons
												(AndDraw);
											    }

/*----------------------------------------------------------------------------
 * Dumps the list contents
 */
											    static
											      void
											      DumpList
											      (void)
											    {
											      Cardinal
												i;

											      for
												(i
												 =
												 0;
												 i
												 <
												 2;
												 i++)
												{
												  PadList
												    [i].
												    Number
												    =
												    0;
												  PadList
												    [i].
												    Position =
												    0;
												}

											      PVList.
												Number =
												0;
											      PVList.
												Position
												=
												0;

											      for
												(i
												 =
												 0;
												 i
												 <
												 MAX_LAYER;
												 i++)
												{
												  LineList
												    [i].
												    Position
												    =
												    0;
												  LineList
												    [i].
												    Number
												    =
												    0;
												  ArcList
												    [i].
												    Position
												    =
												    0;
												  ArcList
												    [i].
												    Number
												    =
												    0;
												  PolygonList
												    [i].
												    Position
												    =
												    0;
												  PolygonList
												    [i].
												    Number
												    =
												    0;
												}
											      RatList.
												Number
												=
												0;
											      RatList.
												Position
												=
												0;
											    }

/*-----------------------------------------------------------------------------
 * Check for DRC violations on a single net starting from the pad or pin
 * sees if the connectivity changes when everything is bloated, or shrunk
 */
											    static
											      Boolean
											      DRCFind
											      (int
											       What,
											       void
											       *ptr1,
											       void
											       *ptr2,
											       void
											       *ptr3)
											    {
											      Bloat
												=
												-Settings.
												
												Shrink;
											      fBloat
												=
												(float)
												-Settings.
												
												Shrink;
											      TheFlag
												=
												DRCFLAG
												|
												SELECTEDFLAG;
											      ListStart
												(What,
												 ptr1,
												 ptr2,
												 ptr3);
											      DoIt
												(True,
												 False);
											      /* ok now the shrunk net has the SELECTEDFLAG set */
											      DumpList
												();
											      TheFlag
												=
												FOUNDFLAG;
											      ListStart
												(What,
												 ptr1,
												 ptr2,
												 ptr3);
											      Bloat
												=
												0;
											      fBloat
												=
												0.0;
											      drc
												= True;	/* abort the search if we find anything not already found */
											      if
												(DoIt
												 (True,
												  False))
												{
												  DumpList
												    ();
												  Message
												    ("WARNING!!  Design Rule Error - potential for broken trace!\n");
												  /* make the flag changes undoable */
												  TheFlag
												    =
												    FOUNDFLAG
												    |
												    SELECTEDFLAG;
												  ResetConnections
												    (False);
												  User
												    =
												    True;
												  drc
												    =
												    False;
												  Bloat
												    =
												    -Settings.
												    
												    Shrink;
												  fBloat
												    =
												    (float)
												    -Settings.
												    
												    Shrink;
												  TheFlag
												    =
												    SELECTEDFLAG;
												  RestoreUndoSerialNumber
												    ();
												  ListStart
												    (What,
												     ptr1,
												     ptr2,
												     ptr3);
												  DoIt
												    (True,
												     True);
												  DumpList
												    ();
												  ListStart
												    (What,
												     ptr1,
												     ptr2,
												     ptr3);
												  TheFlag
												    =
												    FOUNDFLAG;
												  Bloat
												    =
												    0;
												  fBloat
												    =
												    0.0;
												  drc
												    =
												    True;
												  DoIt
												    (True,
												     True);
												  DumpList
												    ();
												  User
												    =
												    False;
												  drc
												    =
												    False;
												  return
												    (True);
												}
											      DumpList
												();
											      /* now check the bloated condition */
											      drc
												=
												False;
											      ResetConnections
												(False);
											      ListStart
												(What,
												 ptr1,
												 ptr2,
												 ptr3);
											      Bloat
												=
												Settings.
												Bloat;
											      fBloat
												=
												(float)
												Settings.
												Bloat;
											      drc
												=
												True;
											      if
												(DoIt
												 (True,
												  False))
												{
												  DumpList
												    ();
												  Message
												    ("WARNING!!!  Design Rule error - copper areas too close!\n");
												  /* make the flag changes undoable */
												  TheFlag
												    =
												    FOUNDFLAG
												    |
												    SELECTEDFLAG;
												  ResetConnections
												    (False);
												  User
												    =
												    True;
												  drc
												    =
												    False;
												  Bloat
												    =
												    0;
												  fBloat
												    =
												    0.0;
												  RestoreUndoSerialNumber
												    ();
												  TheFlag
												    =
												    SELECTEDFLAG;
												  ListStart
												    (What,
												     ptr1,
												     ptr2,
												     ptr3);
												  DoIt
												    (True,
												     True);
												  DumpList
												    ();
												  TheFlag
												    =
												    FOUNDFLAG;
												  ListStart
												    (What,
												     ptr1,
												     ptr2,
												     ptr3);
												  Bloat
												    =
												    Settings.
												    Bloat;
												  fBloat
												    =
												    (float)
												    Settings.
												    Bloat;
												  drc
												    =
												    True;
												  DoIt
												    (True,
												     True);
												  DumpList
												    ();
												  User
												    =
												    False;
												  drc
												    =
												    False;
												  return
												    (True);
												}
											      drc
												=
												False;
											      DumpList
												();
											      TheFlag
												=
												FOUNDFLAG
												|
												SELECTEDFLAG;
											      ResetConnections
												(False);
											      return
												(False);
											    }

/*----------------------------------------------------------------------------
 * set up a temporary flag to use
 */
											    void
											      SaveFindFlag
											      (int
											       NewFlag)
											    {
											      OldFlag
												=
												TheFlag;
											      TheFlag
												=
												NewFlag;
											    }

/*----------------------------------------------------------------------------
 * restore flag
 */
											    void
											      RestoreFindFlag
											      (void)
											    {
											      TheFlag
												=
												OldFlag;
											    }

/*-----------------------------------------------------------------------------
 * Check for DRC violations
 * see if the connectivity changes when everything is bloated, or shrunk
 */
											    Boolean
											      DRCAll
											      (void)
											    {
											      Boolean
												IsBad
												=
												False;


											      InitConnectionLookup
												();


											      TheFlag
												=
												FOUNDFLAG
												|
												DRCFLAG
												|
												SELECTEDFLAG;

											      ResetConnections
												(True);


											      User
												=
												False;

											      ELEMENT_LOOP
												(PCB->
												 Data,
												 PIN_LOOP
												 (element,
												  if
												  (!TEST_FLAG
												   (DRCFLAG,
												    pin)
												   &&
												   DRCFind
												   (PIN_TYPE,
												    (void
												     *)
												    element,
												    (void
												     *)
												    pin,
												    (void
												     *)
												    pin))
												  {
												  IsBad
												  =
												  True;
												  break;}
												 );
												 if
												 (IsBad)
												 break;
												 PAD_LOOP
												 (element,
												  if
												  (!TEST_FLAG
												   (DRCFLAG,
												    pad)
												   &&
												   DRCFind
												   (PAD_TYPE,
												    (void
												     *)
												    element,
												    (void
												     *)
												    pad,
												    (void
												     *)
												    pad))
												  {
												  IsBad
												  =
												  True;
												  break;}
												 );
												 if
												 (IsBad)
												 break;);
											      if
												(!IsBad)
												VIA_LOOP
												  (PCB->
												   Data,
												   if
												   (!TEST_FLAG
												    (DRCFLAG,
												     via)
												    &&
												    DRCFind
												    (VIA_TYPE,
												     (void
												      *)
												     via,
												     (void
												      *)
												     via,
												     (void
												      *)
												     via))
												   {
												   IsBad
												   =
												   True;
												   break;}
											      );
											      TheFlag
												=
												(IsBad)
												?
												DRCFLAG
												:
												(FOUNDFLAG
												 |
												 DRCFLAG
												 |
												 SELECTEDFLAG);
											      ResetConnections
												(False);
											      FreeConnectionLookupMemory
												();
											      TheFlag
												=
												FOUNDFLAG;
											      Bloat
												=
												0;
											      fBloat
												=
												0.0;
											      if
												(IsBad)
												{
												  IncrementUndoSerialNumber
												    ();
												  GotoError
												    ();
												}
											      return
												(IsBad);
											    }

/*----------------------------------------------------------------------------
 * Searches the pcb for object with FOUNDFLAG but no SELECTEDFLAG
 */
											    static
											      void
											      GotoError
											      (void)
											    {
											      Position
												X,
												Y;


											      COPPERLINE_LOOP
												(PCB->
												 Data,
												 if
												 (!TEST_FLAG
												  (SELECTEDFLAG,
												   line)
												  &&
												  TEST_FLAG
												  (FOUNDFLAG,
												   line))
												 {
												 X
												 =
												 (line->
												  Point1.
												  X
												  +
												  line->
												  Point2.
												  X)
												 /
												 2;
												 Y
												 =
												 (line->
												  Point1.
												  Y
												  +
												  line->
												  Point2.
												  Y)
												 /
												 2;
												 goto
												 gotcha;}
											      );
											      COPPERARC_LOOP
												(PCB->
												 Data,
												 if
												 (!TEST_FLAG
												  (SELECTEDFLAG,
												   arc)
												  &&
												  TEST_FLAG
												  (FOUNDFLAG,
												   arc))
												 {
												 X
												 =
												 arc->
												 X;
												 Y
												 =
												 arc->
												 Y;
												 goto
												 gotcha;}
											      );
											      COPPERPOLYGON_LOOP
												(PCB->
												 Data,
												 if
												 (!TEST_FLAG
												  (SELECTEDFLAG,
												   polygon)
												  &&
												  TEST_FLAG
												  (FOUNDFLAG,
												   polygon))
												 {
												 X
												 =
												 (polygon->
												  BoundingBox.
												  X1
												  +
												  polygon->
												  BoundingBox.
												  X2)
												 /
												 2;
												 Y
												 =
												 (polygon->
												  BoundingBox.
												  Y1
												  +
												  polygon->
												  BoundingBox.
												  Y2)
												 /
												 2;
												 goto
												 gotcha;}
											      );
											      ALLPIN_LOOP
												(PCB->
												 Data,
												 if
												 (!TEST_FLAG
												  (SELECTEDFLAG,
												   pin)
												  &&
												  TEST_FLAG
												  (FOUNDFLAG,
												   pin))
												 {
												 X
												 =
												 pin->
												 X;
												 Y
												 =
												 pin->
												 Y;
												 goto
												 gotcha;}
											      );
											      ALLPAD_LOOP
												(PCB->
												 Data,
												 if
												 (!TEST_FLAG
												  (SELECTEDFLAG,
												   pad)
												  &&
												  TEST_FLAG
												  (FOUNDFLAG,
												   pad))
												 {
												 X
												 =
												 (pad->
												  Point1.
												  X
												  +
												  pad->
												  Point2.
												  X)
												 /
												 2;
												 Y
												 =
												 (pad->
												  Point1.
												  Y
												  +
												  pad->
												  Point2.
												  Y)
												 /
												 2;
												 goto
												 gotcha;}
											      );
											      VIA_LOOP
												(PCB->
												 Data,
												 if
												 (!TEST_FLAG
												  (SELECTEDFLAG,
												   via)
												  &&
												  TEST_FLAG
												  (FOUNDFLAG,
												   via))
												 {
												 X
												 =
												 via->
												 X;
												 Y
												 =
												 via->
												 Y;
												 goto
												 gotcha;}
											      );
											      return;
											    gotcha:
											      Message
												("near location (%d,%d)\n",
												 X,
												 Y);
											      CenterDisplay
												(TO_SCREEN_X
												 (X),
												 TO_SCREEN_Y
												 (Y),
												 False);
											      RedrawOutput
												();
											    }

											    void
											      InitConnectionLookup
											      (void)
											    {
											      InitComponentLookup
												();
											      InitLayoutLookup
												();
											    }

											    void
											      FreeConnectionLookupMemory
											      (void)
											    {
											      FreeComponentLookupMemory
												();
											      FreeLayoutLookupMemory
												();
											    }
