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

/* functions used to remove vias, pins ...
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <memory.h>

#include "global.h"

#include "data.h"
#include "draw.h"
#include "error.h"
#include "misc.h"
#include "move.h"
#include "mymem.h"
#include "polygon.h"
#include "rats.h"
#include "remove.h"
#include "search.h"
#include "select.h"
#include "set.h"
#include "undo.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void *DestroyVia (PinTypePtr);
static void *DestroyRat (RatTypePtr);
static void *DestroyLine (LayerTypePtr, LineTypePtr);
static void *DestroyArc (LayerTypePtr, ArcTypePtr);
static void *DestroyText (LayerTypePtr, TextTypePtr);
static void *DestroyPolygon (LayerTypePtr, PolygonTypePtr);
static void *DestroyElement (ElementTypePtr);
static void *RemoveVia (PinTypePtr);
static void *RemoveRat (RatTypePtr);
static void *DestroyPolygonPoint (LayerTypePtr, PolygonTypePtr, PointTypePtr);
static void *RemovePolygonPoint (LayerTypePtr, PolygonTypePtr, PointTypePtr);
static void *RemoveLinePoint (LayerTypePtr, LineTypePtr, PointTypePtr);

/* ---------------------------------------------------------------------------
 * some local types
 */
static ObjectFunctionType RemoveFunctions = {
  RemoveLine,
  RemoveText,
  RemovePolygon,
  RemoveVia,
  RemoveElement,
  NULL,
  NULL,
  NULL,
  RemoveLinePoint,
  RemovePolygonPoint,
  RemoveArc,
  RemoveRat
};
static ObjectFunctionType DestroyFunctions = {
  DestroyLine,
  DestroyText,
  DestroyPolygon,
  DestroyVia,
  DestroyElement,
  NULL,
  NULL,
  NULL,
  NULL,
  DestroyPolygonPoint,
  DestroyArc,
  DestroyRat
};
static DataTypePtr DestroyTarget;
static Boolean Bulk = False;

/* ---------------------------------------------------------------------------
 * remove PCB
 */
void
RemovePCB (PCBTypePtr Ptr)
{
  ClearUndoList (True);
  FreePCBMemory (Ptr);
  SaveFree (Ptr);
}

/* ---------------------------------------------------------------------------
 * destroys a via
 */
static void *
DestroyVia (PinTypePtr Via)
{
  MyFree (&Via->Name);
  *Via = DestroyTarget->Via[--DestroyTarget->ViaN];
  memset (&DestroyTarget->Via[DestroyTarget->ViaN], 0, sizeof (PinType));
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * destroys a line from a layer 
 */
static void *
DestroyLine (LayerTypePtr Layer, LineTypePtr Line)
{
  MyFree (&Line->Number);
  *Line = Layer->Line[--Layer->LineN];
  memset (&Layer->Line[Layer->LineN], 0, sizeof (LineType));
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * destroys an arc from a layer 
 */
static void *
DestroyArc (LayerTypePtr Layer, ArcTypePtr Arc)
{
  *Arc = Layer->Arc[--Layer->ArcN];
  memset (&Layer->Arc[Layer->ArcN], 0, sizeof (ArcType));
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * destroys a rectangle from a layer
 */
static void *
DestroyPolygon (LayerTypePtr Layer, PolygonTypePtr Polygon)
{
  FreePolygonMemory (Polygon);
  *Polygon = Layer->Polygon[--Layer->PolygonN];
  memset (&Layer->Polygon[Layer->PolygonN], 0, sizeof (PolygonType));
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * removes a polygon-point from a polygon and destroys the data
 */
static void *
DestroyPolygonPoint (LayerTypePtr Layer,
		     PolygonTypePtr Polygon, PointTypePtr Point)
{
  PointTypePtr ptr;

  for (ptr = Point + 1; ptr != &Polygon->Points[Polygon->PointN]; ptr++)
    {
      *Point = *ptr;
      Point = ptr;
    }
  Polygon->PointN--;
  SetPolygonBoundingBox (Polygon);
  UpdatePIPFlags (NULL, NULL, Layer, NULL, True);
  return (Polygon);
}

/* ---------------------------------------------------------------------------
 * destroys a text from a layer
 */
static void *
DestroyText (LayerTypePtr Layer, TextTypePtr Text)
{
  MyFree (&Text->TextString);
  *Text = Layer->Text[--Layer->TextN];
  memset (&Layer->Text[Layer->TextN], 0, sizeof (TextType));
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * destroys a element
 */
static void *
DestroyElement (ElementTypePtr Element)
{
  FreeElementMemory (Element);
  *Element = DestroyTarget->Element[--DestroyTarget->ElementN];
  memset (&DestroyTarget->Element[DestroyTarget->ElementN], 0,
	  sizeof (ElementType));
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * destroys a rat
 */
static void *
DestroyRat (RatTypePtr Rat)
{
  *Rat = DestroyTarget->Rat[--DestroyTarget->RatN];
  memset (&DestroyTarget->Rat[DestroyTarget->RatN], 0, sizeof (RatType));
  return (NULL);
}


/* ---------------------------------------------------------------------------
 * removes a via
 */
static void *
RemoveVia (PinTypePtr Via)
{
  /* erase from screen and memory */
  if (PCB->ViaOn)
    {
      EraseVia (Via);
      if (!Bulk)
	Draw ();
    }
  MoveObjectToRemoveUndoList (VIA_TYPE, Via, Via, Via);
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * removes a rat
 */
static void *
RemoveRat (RatTypePtr Rat)
{
  /* erase from screen and memory */
  if (PCB->RatOn)
    {
      EraseRat (Rat);
      if (!Bulk)
	Draw ();
    }
  MoveObjectToRemoveUndoList (RATLINE_TYPE, Rat, Rat, Rat);
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * removes a line point 
 */
static void *
RemoveLinePoint (LayerTypePtr Layer, LineTypePtr Line, PointTypePtr Point)
{
  PointType oldPoint;

  if (&Line->Point1 == Point)
    oldPoint = Line->Point2;
  else
    oldPoint = Line->Point1;
  LINE_LOOP (Layer, 
      {
	{
	  if (line == Line)
	    continue;
	  if ((line->Point1.X == Point->X) && (line->Point1.Y == Point->Y))
	    {
	      MoveObject (LINEPOINT_TYPE, Layer, line, &line->Point1,
			  oldPoint.X - line->Point1.X,
			  oldPoint.Y - line->Point1.Y);
	    }
	  if ((line->Point2.X == Point->X) && (line->Point2.Y == Point->Y))
	    {
	      MoveObject (LINEPOINT_TYPE, Layer, line, &line->Point2,
			  oldPoint.X - line->Point2.X,
			  oldPoint.Y - line->Point2.Y);
	    }
	  break;
	}
      }
  );
  return (RemoveLine (Layer, Line));
}

/* ---------------------------------------------------------------------------
 * removes a line from a layer 
 */
void *
RemoveLine (LayerTypePtr Layer, LineTypePtr Line)
{
  /* erase from screen */
  if (Layer->On)
    {
      EraseLine (Line);
      if (!Bulk)
	Draw ();
    }
  MoveObjectToRemoveUndoList (LINE_TYPE, Layer, Line, Line);
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * removes an arc from a layer 
 */
void *
RemoveArc (LayerTypePtr Layer, ArcTypePtr Arc)
{
  /* erase from screen */
  if (Layer->On)
    {
      EraseArc (Arc);
      if (!Bulk)
	Draw ();
    }
  MoveObjectToRemoveUndoList (ARC_TYPE, Layer, Arc, Arc);
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * removes a text from a layer
 */
void *
RemoveText (LayerTypePtr Layer, TextTypePtr Text)
{
  /* erase from screen */
  if (Layer->On)
    {
      EraseText (Text);
      if (!Bulk)
	Draw ();
    }
  MoveObjectToRemoveUndoList (TEXT_TYPE, Layer, Text, Text);
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * removes a polygon from a layer
 */
void *
RemovePolygon (LayerTypePtr Layer, PolygonTypePtr Polygon)
{
  /* erase from screen */
  if (Layer->On)
    {
      ErasePolygon (Polygon);
      if (!Bulk)
	Draw ();
    }
  MoveObjectToRemoveUndoList (POLYGON_TYPE, Layer, Polygon, Polygon);
  UpdatePIPFlags (NULL, NULL, Layer, NULL, True);
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * removes a polygon-point from a polygon
 */
static void *
RemovePolygonPoint (LayerTypePtr Layer,
		    PolygonTypePtr Polygon, PointTypePtr Point)
{
  PointTypePtr ptr;
  Cardinal index = 0;

  /* insert the polygon-point into the undo list */
  POLYGONPOINT_LOOP (Polygon, 
      {
	if (point == Point)
	  {
	    index = n;
	    break;
	  }
      }
  );
  AddObjectToRemovePointUndoList (POLYGONPOINT_TYPE, Layer, Polygon, index);

  if (Layer->On)
    ErasePolygon (Polygon);

  /* remove point from list, keep point order */
  for (ptr = Point + 1; ptr != &Polygon->Points[Polygon->PointN]; ptr++)
    {
      *Point = *ptr;
      Point = ptr;
    }
  Polygon->PointN--;
  SetPolygonBoundingBox (Polygon);
  RemoveExcessPolygonPoints (Layer, Polygon);
  UpdatePIPFlags (NULL, NULL, Layer, NULL, True);
  /* redraw polygon if necessary */
  if (Layer->On)
    {
      DrawPolygon (Layer, Polygon, 0);
      if (!Bulk)
	Draw ();
    }
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * removes an element
 */
void *
RemoveElement (ElementTypePtr Element)
{
  /* erase from screen */
  if ((PCB->ElementOn || PCB->PinOn) &&
      (FRONT (Element) || PCB->InvisibleObjectsOn))
    {
      EraseElement (Element);
      if (!Bulk)
	Draw ();
    }
  MoveObjectToRemoveUndoList (ELEMENT_TYPE, Element, Element, Element);
  return (NULL);
}

/* ----------------------------------------------------------------------
 * removes all selected and visible objects
 * returns True if any objects have been removed
 */
Boolean
RemoveSelected (void)
{
  Bulk = True;
  if (SelectedOperation (&RemoveFunctions, False, ALL_TYPES))
    {
      IncrementUndoSerialNumber ();
      Draw ();
      Bulk = False;
      return (True);
    }
  Bulk = False;
  return (False);
}

/* ---------------------------------------------------------------------------
 * remove object as referred by pointers and type,
 * allocated memory is passed to the 'remove undo' list
 */
void *
RemoveObject (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  void *ptr = ObjectOperation (&RemoveFunctions, Type, Ptr1, Ptr2, Ptr3);
  return (ptr);
}

/* ---------------------------------------------------------------------------
 * DeleteRats - deletes rat lines only
 * can delete all rat lines, or only selected one
 */

Boolean
DeleteRats (Boolean selected)
{
  Boolean changed = False;

  Bulk = True;
  RAT_LOOP (PCB->Data, 
      {
	if ((!selected) || TEST_FLAG (SELECTEDFLAG, line))
	  {
	    changed = True;
	    RemoveRat (line);
	  }
      }
  );
  Bulk = False;
  if (changed)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (changed);
}

/* ---------------------------------------------------------------------------
 * remove object as referred by pointers and type
 * allocated memory is destroyed assumed to already be erased
 */
void *
DestroyObject (DataTypePtr Target, int Type, void *Ptr1, void *Ptr2,
	       void *Ptr3)
{
  DestroyTarget = Target;
  return (ObjectOperation (&DestroyFunctions, Type, Ptr1, Ptr2, Ptr3));
}
