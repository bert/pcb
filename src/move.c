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

/* functions used to move pins, elements ...
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <setjmp.h>
#include <stdlib.h>

#include "global.h"

#include "create.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "misc.h"
#include "move.h"
#include "mymem.h"
#include "polygon.h"
#include "rtree.h"
#include "search.h"
#include "select.h"
#include "undo.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void *MoveElementName (ElementTypePtr);
static void *MoveElement (ElementTypePtr);
static void *MoveVia (PinTypePtr);
static void *MoveLine (LayerTypePtr, LineTypePtr);
static void *MoveArc (LayerTypePtr, ArcTypePtr);
static void *MoveText (LayerTypePtr, TextTypePtr);
static void *MovePolygon (LayerTypePtr, PolygonTypePtr);
static void *MoveLinePoint (LayerTypePtr, LineTypePtr, PointTypePtr);
static void *MovePolygonPoint (LayerTypePtr, PolygonTypePtr, PointTypePtr);
static void *MoveLineToLayer (LayerTypePtr, LineTypePtr);
static void *MoveArcToLayer (LayerTypePtr, ArcTypePtr);
static void *MoveRatToLayer (RatTypePtr);
static void *MoveTextToLayer (LayerTypePtr, TextTypePtr);
static void *MovePolygonToLayer (LayerTypePtr, PolygonTypePtr);

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static Location DeltaX,		/* used by local routines as offset */
  DeltaY;
static LayerTypePtr Dest;
static Boolean MoreToCome;
static ObjectFunctionType MoveFunctions = {
  MoveLine,
  MoveText,
  MovePolygon,
  MoveVia,
  MoveElement,
  MoveElementName,
  NULL,
  NULL,
  MoveLinePoint,
  MovePolygonPoint,
  MoveArc,
  NULL
}, MoveToLayerFunctions =

{
MoveLineToLayer,
    MoveTextToLayer,
    MovePolygonToLayer,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, MoveArcToLayer, MoveRatToLayer};

/* ---------------------------------------------------------------------------
 * moves a element by +-X and +-Y
 */
void
MoveElementLowLevel (DataTypePtr Data, ElementTypePtr Element, Location DX,
		     Location DY)
{
  if (Data)
    r_delete_entry (Data->element_tree, (BoxType *) Element);
  ELEMENTLINE_LOOP (Element);
  {
    MOVE_LINE_LOWLEVEL (line, DX, DY);
  }
  END_LOOP;
  PIN_LOOP (Element);
  {
    if (Data)
      r_delete_entry (Data->pin_tree, (BoxType *) pin);
    MOVE_PIN_LOWLEVEL (pin, DX, DY);
    if (Data)
      r_insert_entry (Data->pin_tree, (BoxType *) pin, 0);
  }
  END_LOOP;
  PAD_LOOP (Element);
  {
    if (Data)
      r_delete_entry (Data->pad_tree, (BoxType *) pad);
    MOVE_PAD_LOWLEVEL (pad, DX, DY);
    if (Data)
      r_insert_entry (Data->pad_tree, (BoxType *) pad, 0);
  }
  END_LOOP;
  ARC_LOOP (Element);
  {
    MOVE_ARC_LOWLEVEL (arc, DX, DY);
  }
  END_LOOP;
  ELEMENTTEXT_LOOP (Element);
  {
    if (Data->name_tree[n])
      r_delete_entry (PCB->Data->name_tree[n], (BoxType *)text);
    MOVE_TEXT_LOWLEVEL (text, DeltaX, DeltaY);
    if (Data->name_tree[n])
      r_insert_entry (PCB->Data->name_tree[n], (BoxType *)text, 0);
  }
  END_LOOP;
  MOVE_BOX_LOWLEVEL (&Element->BoundingBox, DX, DY);
  MOVE_BOX_LOWLEVEL (&Element->VBox, DX, DY);
  MOVE (Element->MarkX, Element->MarkY, DX, DY);
  if (Data)
    r_insert_entry (Data->element_tree, (BoxType *) Element, 0);
}

/* ----------------------------------------------------------------------
 * moves all names of an element to a new position
 */
static void *
MoveElementName (ElementTypePtr Element)
{
  if (PCB->ElementOn && (FRONT (Element) || PCB->InvisibleObjectsOn))
    {
      EraseElementName (Element);
      ELEMENTTEXT_LOOP (Element);
      {
        if (PCB->Data->name_tree[n])
	  r_delete_entry (PCB->Data->name_tree[n], (BoxType *)text);
	MOVE_TEXT_LOWLEVEL (text, DeltaX, DeltaY);
        if (PCB->Data->name_tree[n])
	  r_insert_entry (PCB->Data->name_tree[n], (BoxType *)text, 0);
      }
      END_LOOP;
      DrawElementName (Element, 0);
      Draw ();
    }
  else
    {
      ELEMENTTEXT_LOOP (Element);
      {
        if (PCB->Data->name_tree[n])
	  r_delete_entry (PCB->Data->name_tree[n], (BoxType *)text);
	MOVE_TEXT_LOWLEVEL (text, DeltaX, DeltaY);
        if (PCB->Data->name_tree[n])
	  r_insert_entry (PCB->Data->name_tree[n], (BoxType *)text, 0);
      }
      END_LOOP;
    }
  return (Element);
}

/* ---------------------------------------------------------------------------
 * moves an element
 */
static void *
MoveElement (ElementTypePtr Element)
{
  Boolean didDraw = False;

  if (PCB->ElementOn && (FRONT (Element) || PCB->InvisibleObjectsOn))
    {
      EraseElement (Element);
      MoveElementLowLevel (PCB->Data, Element, DeltaX, DeltaY);
      DrawElementName (Element, 0);
      DrawElementPackage (Element, 0);
      didDraw = True;
    }
  else
    {
      if (PCB->PinOn)
	EraseElementPinsAndPads (Element);
      MoveElementLowLevel (PCB->Data, Element, DeltaX, DeltaY);
    }
  UpdatePIPFlags (NULL, Element, NULL, NULL, True);
  if (PCB->PinOn)
    {
      DrawElementPinsAndPads (Element, 0);
      didDraw = True;
    }
  if (didDraw)
    Draw ();
  return (Element);
}

/* ---------------------------------------------------------------------------
 * moves a via
 */
static void *
MoveVia (PinTypePtr Via)
{
  r_delete_entry (PCB->Data->via_tree, (BoxTypePtr) Via);
  if (PCB->ViaOn)
    {
      EraseVia (Via);
      MOVE_VIA_LOWLEVEL (Via, DeltaX, DeltaY);
    }
  else
    MOVE_VIA_LOWLEVEL (Via, DeltaX, DeltaY);
  UpdatePIPFlags (Via, (ElementTypePtr) Via, NULL, NULL, True);
  r_insert_entry (PCB->Data->via_tree, (BoxTypePtr) Via, 0);
  if (PCB->ViaOn)
    {
      DrawVia (Via, 0);
      Draw ();
    }
  return (Via);
}

/* ---------------------------------------------------------------------------
 * moves a line
 */
static void *
MoveLine (LayerTypePtr Layer, LineTypePtr Line)
{
  r_delete_entry (Layer->line_tree, (BoxTypePtr) Line);
  if (Layer->On)
    {
      EraseLine (Line);
      MOVE_LINE_LOWLEVEL (Line, DeltaX, DeltaY);
      DrawLine (Layer, Line, 0);
      Draw ();
    }
  else
    MOVE_LINE_LOWLEVEL (Line, DeltaX, DeltaY);
  r_insert_entry (Layer->line_tree, (BoxTypePtr) Line, 0);
  return (Line);
}

/* ---------------------------------------------------------------------------
 * moves an arc
 */
static void *
MoveArc (LayerTypePtr Layer, ArcTypePtr Arc)
{
  r_delete_entry (Layer->arc_tree, (BoxTypePtr) Arc);
  if (Layer->On)
    {
      EraseArc (Arc);
      MOVE_ARC_LOWLEVEL (Arc, DeltaX, DeltaY);
      DrawArc (Layer, Arc, 0);
      Draw ();
    }
  else
    {
      MOVE_ARC_LOWLEVEL (Arc, DeltaX, DeltaY);
    }
  r_insert_entry (Layer->arc_tree, (BoxTypePtr) Arc, 0);
  return (Arc);
}

/* ---------------------------------------------------------------------------
 * moves a text object
 */
static void *
MoveText (LayerTypePtr Layer, TextTypePtr Text)
{
  r_delete_entry (Layer->text_tree, (BoxTypePtr) Text);
  if (Layer->On)
    {
      EraseText (Text);
      MOVE_TEXT_LOWLEVEL (Text, DeltaX, DeltaY);
      DrawText (Layer, Text, 0);
      Draw ();
    }
  else
    MOVE_TEXT_LOWLEVEL (Text, DeltaX, DeltaY);
  r_insert_entry (Layer->text_tree, (BoxTypePtr) Text, 0);
  return (Text);
}

/* ---------------------------------------------------------------------------
 * low level routine to move a polygon
 */
void
MovePolygonLowLevel (PolygonTypePtr Polygon, Location DeltaX, Location DeltaY)
{
  POLYGONPOINT_LOOP (Polygon);
  {
    MOVE (point->X, point->Y, DeltaX, DeltaY);
  }
  END_LOOP;
  MOVE_BOX_LOWLEVEL (&Polygon->BoundingBox, DeltaX, DeltaY);
}

/* ---------------------------------------------------------------------------
 * moves a polygon
 */
static void *
MovePolygon (LayerTypePtr Layer, PolygonTypePtr Polygon)
{
  if (Layer->On)
    {
      ErasePolygon (Polygon);
      MovePolygonLowLevel (Polygon, DeltaX, DeltaY);
      DrawPolygon (Layer, Polygon, 0);
      Draw ();
    }
  else
    MovePolygonLowLevel (Polygon, DeltaX, DeltaY);
  UpdatePIPFlags (NULL, NULL, Layer, Polygon, True);
  return (Polygon);
}

/* ---------------------------------------------------------------------------
 * moves one end of a line
 */
static void *
MoveLinePoint (LayerTypePtr Layer, LineTypePtr Line, PointTypePtr Point)
{
  if (Layer)
    {
      if (Layer->On)
        EraseLine (Line);
      r_delete_entry (Layer->line_tree, &Line->BoundingBox);
      MOVE (Point->X, Point->Y, DeltaX, DeltaY);
      SetLineBoundingBox (Line);
      r_insert_entry (Layer->line_tree, &Line->BoundingBox, 0);
      if (Layer->On)
        {
	  DrawLine (Layer, Line, 0);
	  Draw ();
	}
      return (Line);
    }
  else				/* must be a rat */
    {
      if (PCB->RatOn)
	  EraseRat ((RatTypePtr) Line);
      r_delete_entry (PCB->Data->rat_tree, &Line->BoundingBox);
      MOVE (Point->X, Point->Y, DeltaX, DeltaY);
      SetLineBoundingBox (Line);
      r_insert_entry (PCB->Data->rat_tree, &Line->BoundingBox, 0);
      if (PCB->RatOn)
        {
	  DrawRat ((RatTypePtr) Line, 0);
	  Draw ();
	}
      return (Line);
    }
}

/* ---------------------------------------------------------------------------
 * moves a polygon-point
 */
static void *
MovePolygonPoint (LayerTypePtr Layer, PolygonTypePtr Polygon,
		  PointTypePtr Point)
{
  if (Layer->On)
    {
      ErasePolygon (Polygon);
      MOVE (Point->X, Point->Y, DeltaX, DeltaY);
      if (!RemoveExcessPolygonPoints (Layer, Polygon))
	{
	  SetPolygonBoundingBox (Polygon);
	  DrawPolygon (Layer, Polygon, 0);
	  Draw ();
	}
    }
  else
    {
      MOVE (Point->X, Point->Y, DeltaX, DeltaY);
      if (!RemoveExcessPolygonPoints (Layer, Polygon))
	SetPolygonBoundingBox (Polygon);
    }
  UpdatePIPFlags (NULL, NULL, Layer, Polygon, True);
  return (Point);
}

/* ---------------------------------------------------------------------------
 * moves a line between layers; lowlevel routines
 */
void *
MoveLineToLayerLowLevel (LayerTypePtr Source, LineTypePtr Line,
			 LayerTypePtr Destination)
{
  LineTypePtr new = GetLineMemory (Destination);

  r_delete_entry (Source->line_tree, (BoxTypePtr) Line);
  /* copy the data and remove it from the former layer */
  *new = *Line;
  *Line = Source->Line[--Source->LineN];
  r_substitute (Source->line_tree, (BoxType *) & Source->Line[Source->LineN],
		(BoxType *) Line);
  memset (&Source->Line[Source->LineN], 0, sizeof (LineType));
  if (!Destination->line_tree)
    Destination->line_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (Destination->line_tree, (BoxTypePtr) new, 0);
  return (new);
}

/* ---------------------------------------------------------------------------
 * moves an arc between layers; lowlevel routines
 */
void *
MoveArcToLayerLowLevel (LayerTypePtr Source, ArcTypePtr Arc,
			LayerTypePtr Destination)
{
  ArcTypePtr new = GetArcMemory (Destination);

  r_delete_entry (Source->arc_tree, (BoxTypePtr) Arc);
  /* copy the data and remove it from the former layer */
  *new = *Arc;
  *Arc = Source->Arc[--Source->ArcN];
  r_substitute (Source->arc_tree, (BoxType *) & Source->Arc[Source->ArcN],
		(BoxType *) Arc);
  memset (&Source->Arc[Source->ArcN], 0, sizeof (ArcType));
  if (!Destination->arc_tree)
    Destination->arc_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (Destination->arc_tree, (BoxTypePtr) new, 0);
  return (new);
}


/* ---------------------------------------------------------------------------
 * moves an arc between layers
 */
static void *
MoveArcToLayer (LayerTypePtr Layer, ArcTypePtr Arc)
{
  ArcTypePtr new;

  if (TEST_FLAG (LOCKFLAG, Arc))
    {
      Message ("Sorry that's locked\n");
      return NULL;
    }
  if (Dest == Layer && Layer->On)
    {
      DrawArc (Layer, Arc, 0);
      Draw ();
    }
  if (((long int) Dest == -1) || Dest == Layer)
    return (Arc);
  AddObjectToMoveToLayerUndoList (ARC_TYPE, Layer, Arc, Arc);
  if (Layer->On)
    EraseArc (Arc);
  new = MoveArcToLayerLowLevel (Layer, Arc, Dest);
  if (Dest->On)
    DrawArc (Dest, new, 0);
  Draw ();
  return (new);
}

/* ---------------------------------------------------------------------------
 * moves a line between layers
 */
static void *
MoveRatToLayer (RatTypePtr Rat)
{
  LineTypePtr new;

  new = CreateNewLineOnLayer (Dest, Rat->Point1.X, Rat->Point1.Y,
			      Rat->Point2.X, Rat->Point2.Y,
			      Settings.LineThickness, 2 * Settings.Keepaway,
			      (Rat->Flags & ~RATFLAG) |
			      (TEST_FLAG (CLEARNEWFLAG, PCB) ? CLEARLINEFLAG :
			       0));
  if (!new)
    return (NULL);
  AddObjectToCreateUndoList (LINE_TYPE, Dest, new, new);
  if (PCB->RatOn)
    EraseRat (Rat);
  MoveObjectToRemoveUndoList (RATLINE_TYPE, Rat, Rat, Rat);
  DrawLine (Dest, new, 0);
  Draw ();
  return (new);
}

/* ---------------------------------------------------------------------------
 * moves a line between layers
 */

struct via_info
{
  Location X, Y;
  jmp_buf env;
};

static int
moveline_callback (const BoxType * b, void *cl)
{
  struct via_info *i = (struct via_info *) cl;
  PinTypePtr via;

  if ((via =
       CreateNewVia (PCB->Data, i->X, i->Y,
		     Settings.ViaThickness, 2 * Settings.Keepaway,
		     NOFLAG, Settings.ViaDrillingHole, NULL,
		     VIAFLAG)) != NULL)
    {
      UpdatePIPFlags (via, (ElementTypePtr) via, NULL, NULL, False);
      AddObjectToCreateUndoList (VIA_TYPE, via, via, via);
      DrawVia (via, 0);
    }
  longjmp (i->env, 1);
}

static void *
MoveLineToLayer (LayerTypePtr Layer, LineTypePtr Line)
{
  struct via_info info;
  BoxType sb;
  LineTypePtr new;
  void *ptr1, *ptr2, *ptr3;

  if (TEST_FLAG (LOCKFLAG, Line))
    {
      Message ("Sorry that's locked\n");
      return NULL;
    }
  if (Dest == Layer && Layer->On)
    {
      DrawLine (Layer, Line, 0);
      Draw ();
    }
  if (((long int) Dest == -1) || Dest == Layer)
    return (Line);

  AddObjectToMoveToLayerUndoList (LINE_TYPE, Layer, Line, Line);
  if (Layer->On)
    EraseLine (Line);
  new = MoveLineToLayerLowLevel (Layer, Line, Dest);
  if (Dest->On)
    DrawLine (Dest, new, 0);
  Draw ();
  if (!PCB->ViaOn || MoreToCome ||
      GetLayerGroupNumberByPointer (Layer) ==
      GetLayerGroupNumberByPointer (Dest))
    return (new);
  /* consider via at Point1 */
  sb.X1 = new->Point1.X - new->Thickness / 2;
  sb.X2 = new->Point1.X + new->Thickness / 2;
  sb.Y1 = new->Point1.Y - new->Thickness / 2;
  sb.Y2 = new->Point1.Y + new->Thickness / 2;
  if ((SearchObjectByLocation (PIN_TYPES, &ptr1, &ptr2, &ptr3,
			       new->Point1.X, new->Point1.Y,
			       Settings.ViaThickness / 2) == NO_TYPE))
    {
      info.X = new->Point1.X;
      info.Y = new->Point1.Y;
      if (setjmp (info.env) == 0)
	r_search (Layer->line_tree, &sb, NULL, moveline_callback, &info);
    }
  /* consider via at Point2 */
  sb.X1 = new->Point2.X - new->Thickness / 2;
  sb.X2 = new->Point2.X + new->Thickness / 2;
  sb.Y1 = new->Point2.Y - new->Thickness / 2;
  sb.Y2 = new->Point2.Y + new->Thickness / 2;
  if ((SearchObjectByLocation (PIN_TYPES, &ptr1, &ptr2, &ptr3,
			       new->Point2.X, new->Point2.Y,
			       Settings.ViaThickness / 2) == NO_TYPE))
    {
      info.X = new->Point2.X;
      info.Y = new->Point2.Y;
      if (setjmp (info.env) == 0)
	r_search (Layer->line_tree, &sb, NULL, moveline_callback, &info);
    }
  Draw ();
  return (new);
}

/* ---------------------------------------------------------------------------
 * moves a text object between layers; lowlevel routines
 */
void *
MoveTextToLayerLowLevel (LayerTypePtr Source, TextTypePtr Text,
			 LayerTypePtr Destination)
{
  TextTypePtr new = GetTextMemory (Destination);

  r_delete_entry (Source->text_tree, (BoxTypePtr) Text);
  /* copy the data and remove it from the former layer */
  *new = *Text;
  *Text = Source->Text[--Source->TextN];
  r_substitute (Source->text_tree, (BoxType *) & Source->Text[Source->TextN],
		(BoxType *) Text);
  memset (&Source->Text[Source->TextN], 0, sizeof (TextType));
  if (GetLayerGroupNumberByNumber (MAX_LAYER + SOLDER_LAYER) ==
      GetLayerGroupNumberByPointer (Destination))
    SET_FLAG (ONSOLDERFLAG, new);
  else
    CLEAR_FLAG (ONSOLDERFLAG, new);
  /* re-calculate the bounding box (it could be mirrored now) */
  SetTextBoundingBox (&PCB->Font, new);
  if (!Destination->text_tree)
    Destination->text_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (Destination->text_tree, (BoxTypePtr) new, 0);
  return (new);
}

/* ---------------------------------------------------------------------------
 * moves a text object between layers
 */
static void *
MoveTextToLayer (LayerTypePtr Layer, TextTypePtr Text)
{
  TextTypePtr new;

  if (TEST_FLAG (LOCKFLAG, Text))
    {
      Message ("Sorry that's locked\n");
      return NULL;
    }
  if (Dest != Layer)
    {
      AddObjectToMoveToLayerUndoList (TEXT_TYPE, Layer, Text, Text);
      if (Layer->On)
	EraseText (Text);
      new = MoveTextToLayerLowLevel (Layer, Text, Dest);
      if (Dest->On)
	DrawText (Dest, new, 0);
      if (Layer->On || Dest->On)
	Draw ();
      return (new);
    }
  return (Text);
}

/* ---------------------------------------------------------------------------
 * moves a polygon between layers; lowlevel routines
 */
void *
MovePolygonToLayerLowLevel (LayerTypePtr Source, PolygonTypePtr Polygon,
			    LayerTypePtr Destination)
{
  PolygonTypePtr new = GetPolygonMemory (Destination);

  /* copy the data and remove it from the former layer */
  *new = *Polygon;
  *Polygon = Source->Polygon[--Source->PolygonN];
  memset (&Source->Polygon[Source->PolygonN], 0, sizeof (PolygonType));
  return (new);
}

/* ---------------------------------------------------------------------------
 * moves a polygon between layers
 */
static void *
MovePolygonToLayer (LayerTypePtr Layer, PolygonTypePtr Polygon)
{
  PolygonTypePtr new;
  int LayerThermFlag, DestThermFlag;

  if (TEST_FLAG (LOCKFLAG, Polygon))
    {
      Message ("Sorry that's locked\n");
      return NULL;
    }
  if (((long int) Dest == -1) || (Layer == Dest))
    return (Polygon);
  AddObjectToMoveToLayerUndoList (POLYGON_TYPE, Layer, Polygon, Polygon);
  if (Layer->On)
    ErasePolygon (Polygon);
  /* Move all of the thermals with the polygon */
  LayerThermFlag = L0THERMFLAG << GetLayerNumber (PCB->Data, Layer);
  DestThermFlag = L0THERMFLAG << GetLayerNumber (PCB->Data, Dest);
  ALLPIN_LOOP (PCB->Data);
  {
    if (TEST_FLAG (LayerThermFlag, pin) &&
	IsPointInPolygon (pin->X, pin->Y, 0, Polygon))
      {
	AddObjectToFlagUndoList (PIN_TYPE, Layer, pin, pin);
	CLEAR_FLAG (LayerThermFlag, pin);
	SET_FLAG (DestThermFlag, pin);
      }
  }
  ENDALL_LOOP;
  VIA_LOOP (PCB->Data);
  {
    if (TEST_FLAG (LayerThermFlag, via) &&
	IsPointInPolygon (via->X, via->Y, 0, Polygon))
      {
	AddObjectToFlagUndoList (VIA_TYPE, Layer, via, via);
	CLEAR_FLAG (LayerThermFlag, via);
	SET_FLAG (DestThermFlag, via);
      }
  }
  END_LOOP;
  new = MovePolygonToLayerLowLevel (Layer, Polygon, Dest);
  UpdatePIPFlags (NULL, NULL, Layer, NULL, True);
  UpdatePIPFlags (NULL, NULL, Dest, new, True);
  if (Dest->On)
    {
      DrawPolygon (Dest, new, 0);
      Draw ();
    }
  return (new);
}

/* ---------------------------------------------------------------------------
 * moves the object identified by its data pointers and the type
 * not we don't bump the undo serial number
 */
void *
MoveObject (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
	    Location DX, Location DY)
{
  void *result;
  /* setup offset */
  DeltaX = DX;
  DeltaY = DY;
  AddObjectToMoveUndoList (Type, Ptr1, Ptr2, Ptr3, DX, DY);
  result = ObjectOperation (&MoveFunctions, Type, Ptr1, Ptr2, Ptr3);
  return (result);
}

/* ---------------------------------------------------------------------------
 * moves the object identified by its data pointers and the type
 * as well as all attached rubberband lines
 */
void *
MoveObjectAndRubberband (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
			 Location DX, Location DY)
{
  RubberbandTypePtr ptr;
  void *ptr2;

  /* setup offset */
  DeltaX = DX;
  DeltaY = DY;
  if (DX == 0 && DY == 0)
    return (NULL);

  /* move all the lines... and reset the counter */
  ptr = Crosshair.AttachedObject.Rubberband;
  while (Crosshair.AttachedObject.RubberbandN)
    {
      /* first clear any marks that we made in the line flags */
      ptr->Line->Flags &= ~RUBBERENDFLAG;
      AddObjectToMoveUndoList (LINEPOINT_TYPE,
			       ptr->Layer, ptr->Line, ptr->MovedPoint, DX,
			       DY);
      MoveLinePoint (ptr->Layer, ptr->Line, ptr->MovedPoint);
      Crosshair.AttachedObject.RubberbandN--;
      ptr++;
    }

  AddObjectToMoveUndoList (Type, Ptr1, Ptr2, Ptr3, DX, DY);
  ptr2 = ObjectOperation (&MoveFunctions, Type, Ptr1, Ptr2, Ptr3);
  IncrementUndoSerialNumber ();
  return (ptr2);
}

/* ---------------------------------------------------------------------------
 * moves the object identified by its data pointers and the type
 * to a new layer without changing it's position
 */
void *
MoveObjectToLayer (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
		   LayerTypePtr Target, Boolean enmasse)
{
  void *result;

  /* setup global identifiers */
  Dest = Target;
  MoreToCome = enmasse;
  result = ObjectOperation (&MoveToLayerFunctions, Type, Ptr1, Ptr2, Ptr3);
  IncrementUndoSerialNumber ();
  return (result);
}

/* ---------------------------------------------------------------------------
 * moves the selected objects to a new layer without changing their
 * positions
 */
Boolean
MoveSelectedObjectsToLayer (LayerTypePtr Target)
{
  Boolean changed;

  /* setup global identifiers */
  Dest = Target;
  MoreToCome = True;
  changed = SelectedOperation (&MoveToLayerFunctions, True, ALL_TYPES);
  /* passing True to above operation causes Undoserial to auto-increment */
  return (changed);
}
