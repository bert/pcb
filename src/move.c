/* $Id$ */

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
#include "thermal.h"
#include "undo.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");




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
static LocationType DeltaX,	/* used by local routines as offset */
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
MoveElementLowLevel (DataTypePtr Data, ElementTypePtr Element,
		     LocationType DX, LocationType DY)
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
      {
	r_delete_entry (Data->pin_tree, (BoxType *) pin);
	RestoreToPolygon (Data, PIN_TYPE, Element, pin);
      }
    MOVE_PIN_LOWLEVEL (pin, DX, DY);
    if (Data)
      {
	r_insert_entry (Data->pin_tree, (BoxType *) pin, 0);
	ClearFromPolygon (Data, PIN_TYPE, Element, pin);
      }
  }
  END_LOOP;
  PAD_LOOP (Element);
  {
    if (Data)
      {
	r_delete_entry (Data->pad_tree, (BoxType *) pad);
	RestoreToPolygon (Data, PAD_TYPE, Element, pad);
      }
    MOVE_PAD_LOWLEVEL (pad, DX, DY);
    if (Data)
      {
	r_insert_entry (Data->pad_tree, (BoxType *) pad, 0);
	ClearFromPolygon (Data, PAD_TYPE, Element, pad);
      }
  }
  END_LOOP;
  ARC_LOOP (Element);
  {
    MOVE_ARC_LOWLEVEL (arc, DX, DY);
  }
  END_LOOP;
  ELEMENTTEXT_LOOP (Element);
  {
    if (Data && Data->name_tree[n])
      r_delete_entry (PCB->Data->name_tree[n], (BoxType *) text);
    MOVE_TEXT_LOWLEVEL (text, DX, DY);
    if (Data && Data->name_tree[n])
      r_insert_entry (PCB->Data->name_tree[n], (BoxType *) text, 0);
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
	  r_delete_entry (PCB->Data->name_tree[n], (BoxType *) text);
	MOVE_TEXT_LOWLEVEL (text, DeltaX, DeltaY);
	if (PCB->Data->name_tree[n])
	  r_insert_entry (PCB->Data->name_tree[n], (BoxType *) text, 0);
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
	  r_delete_entry (PCB->Data->name_tree[n], (BoxType *) text);
	MOVE_TEXT_LOWLEVEL (text, DeltaX, DeltaY);
	if (PCB->Data->name_tree[n])
	  r_insert_entry (PCB->Data->name_tree[n], (BoxType *) text, 0);
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
  RestoreToPolygon (PCB->Data, VIA_TYPE, Via, Via);
  MOVE_VIA_LOWLEVEL (Via, DeltaX, DeltaY);
  if (PCB->ViaOn)
    EraseVia (Via);
  r_insert_entry (PCB->Data->via_tree, (BoxTypePtr) Via, 0);
  ClearFromPolygon (PCB->Data, VIA_TYPE, Via, Via);
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
  if (Layer->On)
    EraseLine (Line);
  RestoreToPolygon (PCB->Data, LINE_TYPE, Layer, Line);
  r_delete_entry (Layer->line_tree, (BoxTypePtr) Line);
  MOVE_LINE_LOWLEVEL (Line, DeltaX, DeltaY);
  r_insert_entry (Layer->line_tree, (BoxTypePtr) Line, 0);
  ClearFromPolygon (PCB->Data, LINE_TYPE, Layer, Line);
  if (Layer->On)
    {
      DrawLine (Layer, Line, 0);
      Draw ();
    }
  return (Line);
}

/* ---------------------------------------------------------------------------
 * moves an arc
 */
static void *
MoveArc (LayerTypePtr Layer, ArcTypePtr Arc)
{
  RestoreToPolygon (PCB->Data, ARC_TYPE, Layer, Arc);
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
  ClearFromPolygon (PCB->Data, ARC_TYPE, Layer, Arc);
  return (Arc);
}

/* ---------------------------------------------------------------------------
 * moves a text object
 */
static void *
MoveText (LayerTypePtr Layer, TextTypePtr Text)
{
  RestoreToPolygon (PCB->Data, TEXT_TYPE, Layer, Text);
  r_delete_entry (Layer->text_tree, (BoxTypePtr) Text);
  if (Layer->On)
    {
      EraseText (Layer, Text);
      MOVE_TEXT_LOWLEVEL (Text, DeltaX, DeltaY);
      DrawText (Layer, Text, 0);
      Draw ();
    }
  else
    MOVE_TEXT_LOWLEVEL (Text, DeltaX, DeltaY);
  r_insert_entry (Layer->text_tree, (BoxTypePtr) Text, 0);
  ClearFromPolygon (PCB->Data, TEXT_TYPE, Layer, Text);
  return (Text);
}

/* ---------------------------------------------------------------------------
 * low level routine to move a polygon
 */
void
MovePolygonLowLevel (PolygonTypePtr Polygon, LocationType DeltaX,
		     LocationType DeltaY)
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
    }
  r_delete_entry (Layer->polygon_tree, (BoxType *) Polygon);
  MovePolygonLowLevel (Polygon, DeltaX, DeltaY);
  r_insert_entry (Layer->polygon_tree, (BoxType *) Polygon, 0);
  InitClip (PCB->Data, Layer, Polygon);
  if (Layer->On)
    {
      DrawPolygon (Layer, Polygon, 0);
      Draw ();
    }
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
      RestoreToPolygon (PCB->Data, LINE_TYPE, Layer, Line);
      r_delete_entry (Layer->line_tree, &Line->BoundingBox);
      MOVE (Point->X, Point->Y, DeltaX, DeltaY);
      SetLineBoundingBox (Line);
      r_insert_entry (Layer->line_tree, &Line->BoundingBox, 0);
      ClearFromPolygon (PCB->Data, LINE_TYPE, Layer, Line);
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
    }
  r_delete_entry (Layer->polygon_tree, (BoxType *) Polygon);
  MOVE (Point->X, Point->Y, DeltaX, DeltaY);
  SetPolygonBoundingBox (Polygon);
  r_insert_entry (Layer->polygon_tree, (BoxType *) Polygon, 0);
  RemoveExcessPolygonPoints (Layer, Polygon);
  InitClip (PCB->Data, Layer, Polygon);
  if (Layer->On)
    {
      DrawPolygon (Layer, Polygon, 0);
      Draw ();
    }
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
      Message (_("Sorry, the object is locked\n"));
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
  RestoreToPolygon (PCB->Data, ARC_TYPE, Layer, Arc);
  if (Layer->On)
    EraseArc (Arc);
  new = MoveArcToLayerLowLevel (Layer, Arc, Dest);
  ClearFromPolygon (PCB->Data, ARC_TYPE, Dest, Arc);
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
  //LocationType X1 = Rat->Point1.X, Y1 = Rat->Point1.Y;
  //LocationType X1 = Rat->Point1.X, Y1 = Rat->Point1.Y;
  // if VIAFLAG
  //   if we're on a pin, add a thermal
  //   else make a via and a wire, but 0-length wire not good
  // else as before

  new = CreateNewLineOnLayer (Dest, Rat->Point1.X, Rat->Point1.Y,
			      Rat->Point2.X, Rat->Point2.Y,
			      Settings.LineThickness, 2 * Settings.Keepaway,
			      Rat->Flags);
  if (TEST_FLAG (CLEARNEWFLAG, PCB))
    SET_FLAG (CLEARLINEFLAG, new);
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
  LocationType X, Y;
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
		     NoFlags ())) != NULL)
    {
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
      Message (_("Sorry, the object is locked\n"));
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
  RestoreToPolygon (PCB->Data, LINE_TYPE, Layer, Line);
  new = MoveLineToLayerLowLevel (Layer, Line, Dest);
  Line = NULL;
  ClearFromPolygon (PCB->Data, LINE_TYPE, Dest, new);
  if (Dest->On)
    DrawLine (Dest, new, 0);
  Draw ();
  if (!PCB->ViaOn || MoreToCome ||
      GetLayerGroupNumberByPointer (Layer) ==
      GetLayerGroupNumberByPointer (Dest) ||
      TEST_SILK_LAYER(Layer) ||
      TEST_SILK_LAYER(Dest))
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

  RestoreToPolygon (PCB->Data, TEXT_TYPE, Source, Text);
  r_delete_entry (Source->text_tree, (BoxTypePtr) Text);
  /* copy the data and remove it from the former layer */
  *new = *Text;
  *Text = Source->Text[--Source->TextN];
  r_substitute (Source->text_tree, (BoxType *) & Source->Text[Source->TextN],
		(BoxType *) Text);
  memset (&Source->Text[Source->TextN], 0, sizeof (TextType));
  if (GetLayerGroupNumberByNumber (max_layer + SOLDER_LAYER) ==
      GetLayerGroupNumberByPointer (Destination))
    SET_FLAG (ONSOLDERFLAG, new);
  else
    CLEAR_FLAG (ONSOLDERFLAG, new);
  /* re-calculate the bounding box (it could be mirrored now) */
  SetTextBoundingBox (&PCB->Font, new);
  if (!Destination->text_tree)
    Destination->text_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (Destination->text_tree, (BoxTypePtr) new, 0);
  ClearFromPolygon (PCB->Data, TEXT_TYPE, Destination, new);
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
      Message (_("Sorry, the object is locked\n"));
      return NULL;
    }
  if (Dest != Layer)
    {
      AddObjectToMoveToLayerUndoList (TEXT_TYPE, Layer, Text, Text);
      if (Layer->On)
	EraseText (Layer, Text);
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

  r_delete_entry (Source->polygon_tree, (BoxType *) Polygon);
  /* copy the data and remove it from the former layer */
  *new = *Polygon;
  *Polygon = Source->Polygon[--Source->PolygonN];
  r_substitute (Source->polygon_tree,
		(BoxType *) & Source->Polygon[Source->PolygonN],
		(BoxType *) Polygon);
  memset (&Source->Polygon[Source->PolygonN], 0, sizeof (PolygonType));
  if (!Destination->polygon_tree)
    Destination->polygon_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (Destination->polygon_tree, (BoxType *) new, 0);
  return (new);
}

struct mptlc
{
  Cardinal snum, dnum;
  int type;
  PolygonTypePtr polygon;
} mptlc;

int
mptl_pin_callback (const BoxType *b, void *cl)
{
  struct mptlc *d = (struct mptlc *) cl;
  PinTypePtr pin = (PinTypePtr) b;
  if (!TEST_THERM (d->snum, pin) || !
	IsPointInPolygon (pin->X, pin->Y, pin->Thickness + pin->Clearance + 2,
			  d->polygon))
			  return 0;
  if (d->type == PIN_TYPE)
    AddObjectToFlagUndoList (PIN_TYPE, pin->Element, pin, pin);
  else
    AddObjectToFlagUndoList (VIA_TYPE, pin, pin, pin);
  ASSIGN_THERM (d->dnum, GET_THERM (d->snum, pin), pin);
  CLEAR_THERM (d->snum, pin);
  return 1;
}

/* ---------------------------------------------------------------------------
 * moves a polygon between layers
 */
static void *
MovePolygonToLayer (LayerTypePtr Layer, PolygonTypePtr Polygon)
{
  PolygonTypePtr new;
  struct mptlc d;

  if (TEST_FLAG (LOCKFLAG, Polygon))
    {
      Message (_("Sorry, the object is locked\n"));
      return NULL;
    }
  if (((long int) Dest == -1) || (Layer == Dest))
    return (Polygon);
  AddObjectToMoveToLayerUndoList (POLYGON_TYPE, Layer, Polygon, Polygon);
  if (Layer->On)
    ErasePolygon (Polygon);
  /* Move all of the thermals with the polygon */
  d.snum = GetLayerNumber (PCB->Data, Layer);
  d.dnum = GetLayerNumber (PCB->Data, Dest);
  d.polygon = Polygon;
  d.type = PIN_TYPE;
  r_search (PCB->Data->pin_tree, &Polygon->BoundingBox, NULL, mptl_pin_callback, &d);
  d.type = VIA_TYPE;
  r_search (PCB->Data->via_tree, &Polygon->BoundingBox, NULL, mptl_pin_callback, &d);
  new = MovePolygonToLayerLowLevel (Layer, Polygon, Dest);
  InitClip (PCB->Data, Dest, new);
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
	    LocationType DX, LocationType DY)
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
			 LocationType DX, LocationType DY)
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
      CLEAR_FLAG (RUBBERENDFLAG, ptr->Line);
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

/* ---------------------------------------------------------------------------
 * moves the selected layers to a new index in the layer list.
 */

static void
move_one_thermal (int old_index, int new_index, PinType *pin)
{
  int t1=0, i;
  int oi=old_index, ni=new_index;

  if (old_index != -1)
    t1 = GET_THERM (old_index, pin);

  if (oi == -1)
    oi = MAX_LAYER-1; /* inserting a layer */
  if (ni == -1)
    ni = MAX_LAYER-1; /* deleting a layer */

  if (oi < ni)
    {
      for (i=oi; i<ni; i++)
	ASSIGN_THERM (i, GET_THERM (i+1, pin), pin);
    }
  else
    {
      for (i=oi; i>ni; i--)
	ASSIGN_THERM (i, GET_THERM (i-1, pin), pin);
    }

  if (new_index != -1)
    ASSIGN_THERM (new_index, t1, pin);
  else
    ASSIGN_THERM (ni, 0, pin);
}

static void
move_all_thermals (int old_index, int new_index)
{
  VIA_LOOP (PCB->Data);
    {
      move_one_thermal (old_index, new_index, via);
    }
  END_LOOP;

  ALLPIN_LOOP (PCB->Data);
    {
      move_one_thermal (old_index, new_index, pin);
    }
  ENDALL_LOOP;
}


int
MoveLayer (int old_index, int new_index)
{
  int groups[MAX_LAYER + 2], l, g;
  LayerType saved_layer;
  int saved_group;

  AddLayerChangeToUndoList (old_index, new_index);
  IncrementUndoSerialNumber ();

  if (old_index < -1 || old_index >= max_layer)
    {
      Message ("Invalid old layer %d for move: must be -1..%d\n",
	       old_index, max_layer - 1);
      return 1;
    }
  if (new_index < -1 || new_index > max_layer || new_index >= MAX_LAYER)
    {
      Message ("Invalid new layer %d for move: must be -1..%d\n",
	       new_index, max_layer);
      return 1;
    }
  if (old_index == new_index)
    return 0;

  for (g = 0; g < MAX_LAYER; g++)
    for (l = 0; l < PCB->LayerGroups.Number[g]; l++)
      groups[PCB->LayerGroups.Entries[g][l]] = g;

  if (old_index == -1)
    {
      LayerTypePtr lp;
      if (max_layer == MAX_LAYER)
	{
	  Message ("No room for new layers\n");
	  return 1;
	}
      /* Create a new layer at new_index. */
      lp = &PCB->Data->Layer[new_index];
      memmove (&PCB->Data->Layer[new_index + 1],
	       &PCB->Data->Layer[new_index],
	       (max_layer - new_index + 2) * sizeof (LayerType));
      memmove (&groups[new_index + 1],
	       &groups[new_index],
	       (max_layer - new_index + 2) * sizeof (int));
      max_layer++;
      memset (lp, 0, sizeof (LayerType));
      lp->On = 1;
      lp->Name = MyStrdup ("New Layer", "MoveLayer");
      lp->Color = Settings.LayerColor[new_index];
      lp->SelectedColor = Settings.LayerSelectedColor[new_index];
      for (l = 0; l < max_layer; l++)
	if (LayerStack[l] >= new_index)
	  LayerStack[l]++;
      LayerStack[max_layer - 1] = new_index;
    }
  else if (new_index == -1)
    {
      /* Delete the layer at old_index */
      memmove (&PCB->Data->Layer[old_index],
	       &PCB->Data->Layer[old_index + 1],
	       (max_layer - old_index + 2 - 1) * sizeof (LayerType));
      memset (&PCB->Data->Layer[max_layer + 1], 0, sizeof (LayerType));
      memmove (&groups[old_index],
	       &groups[old_index + 1],
	       (max_layer - old_index + 2 - 1) * sizeof (int));
      for (l = 0; l < max_layer; l++)
	if (LayerStack[l] == old_index)
	  memmove (LayerStack + l,
		   LayerStack + l + 1,
		   (max_layer - l - 1) * sizeof (LayerStack[0]));
      max_layer--;
      for (l = 0; l < max_layer; l++)
	if (LayerStack[l] > old_index)
	  LayerStack[l]--;
    }
  else
    {
      /* Move an existing layer */
      memcpy (&saved_layer, &PCB->Data->Layer[old_index], sizeof (LayerType));
      saved_group = groups[old_index];
      if (old_index < new_index)
	{
	  memmove (&PCB->Data->Layer[old_index],
		   &PCB->Data->Layer[old_index + 1],
		   (new_index - old_index) * sizeof (LayerType));
	  memmove (&groups[old_index],
		   &groups[old_index + 1],
		   (new_index - old_index) * sizeof (int));
	}
      else
	{
	  memmove (&PCB->Data->Layer[new_index + 1],
		   &PCB->Data->Layer[new_index],
		   (old_index - new_index) * sizeof (LayerType));
	  memmove (&groups[new_index + 1],
		   &groups[new_index],
		   (old_index - new_index) * sizeof (int));
	}
      memcpy (&PCB->Data->Layer[new_index], &saved_layer, sizeof (LayerType));
      groups[new_index] = saved_group;
    }

  move_all_thermals(old_index, new_index);

  for (g = 0; g < MAX_LAYER; g++)
    PCB->LayerGroups.Number[g] = 0;
  for (l = 0; l < max_layer + 2; l++)
    {
      int i;
      g = groups[l];
      i = PCB->LayerGroups.Number[g]++;
      PCB->LayerGroups.Entries[g][i] = l;
    }

  for (g = 0; g < MAX_LAYER; g++)
    if (PCB->LayerGroups.Number[g] == 0)
      {
	memmove (&PCB->LayerGroups.Number[g],
		 &PCB->LayerGroups.Number[g + 1],
		 (MAX_LAYER - g - 1) * sizeof (PCB->LayerGroups.Number[g]));
	memmove (&PCB->LayerGroups.Entries[g],
		 &PCB->LayerGroups.Entries[g + 1],
		 (MAX_LAYER - g - 1) * sizeof (PCB->LayerGroups.Entries[g]));
      }

  hid_action ("LayersChanged");
  gui->invalidate_all ();
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char movelayer_syntax[] = "MoveLayer(old,new)";

static const char movelayer_help[] = "Moves/Creates/Deletes Layers";

/* %start-doc actions MoveLayer

Moves a layer, creates a new layer, or deletes a layer.

@table @code

@item old
The is the layer number to act upon.  Allowed values are:
@table @code

@item c
Currently selected layer.

@item -1
Create a new layer.

@item number
An existing layer number.

@end table

@item new
Specifies where to move the layer to.  Allowed values are:
@table @code
@item -1
Deletes the layer.

@item up
Moves the layer up.

@item down
Moves the layer down.

@item c
Creates a new layer.

@end table

@end table

%end-doc */

int
MoveLayerAction (int argc, char **argv, int x, int y)
{
  int old_index, new_index;
  int new_top = -1;

  if (argc != 2)
    {
      Message ("Usage; MoveLayer(old,new)");
      return 1;
    }

  if (strcmp (argv[0], "c") == 0)
    old_index = INDEXOFCURRENT;
  else
    old_index = atoi (argv[0]);

  if (strcmp (argv[1], "c") == 0)
    {
      new_index = INDEXOFCURRENT;
      if (new_index < 0)
	new_index = 0;
    }
  else if (strcmp (argv[1], "up") == 0)
    {
      new_index = INDEXOFCURRENT - 1;
      if (new_index < 0)
	return 1;
      new_top = new_index;
    }
  else if (strcmp (argv[1], "down") == 0)
    {
      new_index = INDEXOFCURRENT + 1;
      if (new_index >= max_layer)
	return 1;
      new_top = new_index;
    }
  else
    new_index = atoi (argv[1]);

  if (MoveLayer (old_index, new_index))
    return 1;

  if (new_index == -1)
    {
      new_top = old_index;
      if (new_top >= max_layer)
	new_top--;
      new_index = new_top;
    }
  if (old_index == -1)
    new_top = new_index;

  if (new_top != -1)
    ChangeGroupVisibility (new_index, 1, 1);

  return 0;
}

HID_Action move_action_list[] = {
  {"MoveLayer", 0, MoveLayerAction,
   movelayer_help, movelayer_syntax}
};

REGISTER_ACTIONS (move_action_list)
