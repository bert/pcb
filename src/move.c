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

#include <assert.h>
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
#include "rubberband.h"
#include "remove.h"
#include "search.h"
#include "select.h"
#include "thermal.h"
#include "undo.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define dprintf if(0)printf

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void *MoveElementName (ElementType *);
static void *MoveElement (ElementType *);
static void *MoveVia (PinType *);
static void *MoveLine (LayerType *, LineType *);
static void *MoveArc (LayerType *, ArcType *);
static void *MoveText (LayerType *, TextType *);
static void *MovePolygon (LayerType *, PolygonType *);
static void *MoveLinePoint (LayerType *, LineType *, PointType *);
static void *MovePolygonPoint (LayerType *, PolygonType *, PointType *);
static void *MoveLineToLayer (LayerType *, LineType *);
static void *MoveArcToLayer (LayerType *, ArcType *);
static void *MoveRatToLayer (RatType *);
static void *MoveTextToLayer (LayerType *, TextType *);
static void *MovePolygonToLayer (LayerType *, PolygonType *);

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static Coord DeltaX, DeltaY;	/* used by local routines as offset */
static LayerType *Dest;
static bool MoreToCome;
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
MoveElementLowLevel (DataType *Data, ElementType *Element,
		     Coord DX, Coord DY)
{
  if (Data)
    r_delete_entry (Data->element_tree, (BoxType *)Element);
  ELEMENTLINE_LOOP (Element);
  {
    MOVE_LINE_LOWLEVEL (line, DX, DY);
  }
  END_LOOP;
  PIN_LOOP (Element);
  {
    if (Data)
      {
	r_delete_entry (Data->pin_tree, (BoxType *)pin);
	RestoreToPolygon (Data, PIN_TYPE, Element, pin);
      }
    MOVE_PIN_LOWLEVEL (pin, DX, DY);
    if (Data)
      {
	r_insert_entry (Data->pin_tree, (BoxType *)pin, 0);
	ClearFromPolygon (Data, PIN_TYPE, Element, pin);
      }
  }
  END_LOOP;
  PAD_LOOP (Element);
  {
    if (Data)
      {
	r_delete_entry (Data->pad_tree, (BoxType *)pad);
	RestoreToPolygon (Data, PAD_TYPE, Element, pad);
      }
    MOVE_PAD_LOWLEVEL (pad, DX, DY);
    if (Data)
      {
	r_insert_entry (Data->pad_tree, (BoxType *)pad, 0);
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
      r_delete_entry (PCB->Data->name_tree[n], (BoxType *)text);
    MOVE_TEXT_LOWLEVEL (text, DX, DY);
    if (Data && Data->name_tree[n])
      r_insert_entry (PCB->Data->name_tree[n], (BoxType *)text, 0);
  }
  END_LOOP;
  MOVE_BOX_LOWLEVEL (&Element->BoundingBox, DX, DY);
  MOVE_BOX_LOWLEVEL (&Element->VBox, DX, DY);
  MOVE (Element->MarkX, Element->MarkY, DX, DY);
  if (Data)
    r_insert_entry (Data->element_tree, (BoxType *)Element, 0);
}

/* ----------------------------------------------------------------------
 * moves all names of an element to a new position
 */
static void *
MoveElementName (ElementType *Element)
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
      DrawElementName (Element);
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
MoveElement (ElementType *Element)
{
  bool didDraw = false;

  if (PCB->ElementOn && (FRONT (Element) || PCB->InvisibleObjectsOn))
    {
      EraseElement (Element);
      MoveElementLowLevel (PCB->Data, Element, DeltaX, DeltaY);
      DrawElementName (Element);
      DrawElementPackage (Element);
      didDraw = true;
    }
  else
    {
      if (PCB->PinOn)
	EraseElementPinsAndPads (Element);
      MoveElementLowLevel (PCB->Data, Element, DeltaX, DeltaY);
    }
  if (PCB->PinOn)
    {
      DrawElementPinsAndPads (Element);
      didDraw = true;
    }
  if (didDraw)
    Draw ();
  return (Element);
}

/* ---------------------------------------------------------------------------
 * moves a via
 */
static void *
MoveVia (PinType *Via)
{
  r_delete_entry (PCB->Data->via_tree, (BoxType *)Via);
  RestoreToPolygon (PCB->Data, VIA_TYPE, Via, Via);
  MOVE_VIA_LOWLEVEL (Via, DeltaX, DeltaY);
  if (PCB->ViaOn)
    EraseVia (Via);
  r_insert_entry (PCB->Data->via_tree, (BoxType *)Via, 0);
  ClearFromPolygon (PCB->Data, VIA_TYPE, Via, Via);
  if (PCB->ViaOn)
    {
      DrawVia (Via);
      Draw ();
    }
  return (Via);
}

/* ---------------------------------------------------------------------------
 * moves a line
 */
static void *
MoveLine (LayerType *Layer, LineType *Line)
{
  if (Layer->On)
    EraseLine (Line);
  RestoreToPolygon (PCB->Data, LINE_TYPE, Layer, Line);
  r_delete_entry (Layer->line_tree, (BoxType *)Line);
  MOVE_LINE_LOWLEVEL (Line, DeltaX, DeltaY);
  SetLineBoundingBox (Line);
  r_insert_entry (Layer->line_tree, (BoxType *)Line, 0);
  ClearFromPolygon (PCB->Data, LINE_TYPE, Layer, Line);
  if (Layer->On)
    {
      DrawLine (Layer, Line);
      Draw ();
    }
  return (Line);
}

/* ---------------------------------------------------------------------------
 * moves an arc
 */
static void *
MoveArc (LayerType *Layer, ArcType *Arc)
{
  RestoreToPolygon (PCB->Data, ARC_TYPE, Layer, Arc);
  r_delete_entry (Layer->arc_tree, (BoxType *)Arc);
  if (Layer->On)
    {
      EraseArc (Arc);
      MOVE_ARC_LOWLEVEL (Arc, DeltaX, DeltaY);
      DrawArc (Layer, Arc);
      Draw ();
    }
  else
    {
      MOVE_ARC_LOWLEVEL (Arc, DeltaX, DeltaY);
    }
  r_insert_entry (Layer->arc_tree, (BoxType *)Arc, 0);
  ClearFromPolygon (PCB->Data, ARC_TYPE, Layer, Arc);
  return (Arc);
}

/* ---------------------------------------------------------------------------
 * moves a text object
 */
static void *
MoveText (LayerType *Layer, TextType *Text)
{
  RestoreToPolygon (PCB->Data, TEXT_TYPE, Layer, Text);
  r_delete_entry (Layer->text_tree, (BoxType *)Text);
  if (Layer->On)
    {
      EraseText (Layer, Text);
      MOVE_TEXT_LOWLEVEL (Text, DeltaX, DeltaY);
      DrawText (Layer, Text);
      Draw ();
    }
  else
    MOVE_TEXT_LOWLEVEL (Text, DeltaX, DeltaY);
  r_insert_entry (Layer->text_tree, (BoxType *)Text, 0);
  ClearFromPolygon (PCB->Data, TEXT_TYPE, Layer, Text);
  return (Text);
}

/* ---------------------------------------------------------------------------
 * low level routine to move a polygon
 */
void
MovePolygonLowLevel (PolygonType *Polygon, Coord DeltaX, Coord DeltaY)
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
MovePolygon (LayerType *Layer, PolygonType *Polygon)
{
  if (Layer->On)
    {
      ErasePolygon (Polygon);
    }
  r_delete_entry (Layer->polygon_tree, (BoxType *)Polygon);
  MovePolygonLowLevel (Polygon, DeltaX, DeltaY);
  r_insert_entry (Layer->polygon_tree, (BoxType *)Polygon, 0);
  InitClip (PCB->Data, Layer, Polygon);
  if (Layer->On)
    {
      DrawPolygon (Layer, Polygon);
      Draw ();
    }
  return (Polygon);
}

/* ---------------------------------------------------------------------------
 * moves one end of a line
 */
static void *
MoveLinePoint (LayerType *Layer, LineType *Line, PointType *Point)
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
	  DrawLine (Layer, Line);
	  Draw ();
	}
      return (Line);
    }
  else				/* must be a rat */
    {
      if (PCB->RatOn)
	EraseRat ((RatType *) Line);
      r_delete_entry (PCB->Data->rat_tree, &Line->BoundingBox);
      MOVE (Point->X, Point->Y, DeltaX, DeltaY);
      SetLineBoundingBox (Line);
      r_insert_entry (PCB->Data->rat_tree, &Line->BoundingBox, 0);
      if (PCB->RatOn)
	{
	  DrawRat ((RatType *) Line);
	  Draw ();
	}
      return (Line);
    }
}

/* ---------------------------------------------------------------------------
 * moves a polygon-point
 */
static void *
MovePolygonPoint (LayerType *Layer, PolygonType *Polygon,
		  PointType *Point)
{
  if (Layer->On)
    {
      ErasePolygon (Polygon);
    }
  r_delete_entry (Layer->polygon_tree, (BoxType *)Polygon);
  MOVE (Point->X, Point->Y, DeltaX, DeltaY);
  SetPolygonBoundingBox (Polygon);
  r_insert_entry (Layer->polygon_tree, (BoxType *)Polygon, 0);
  RemoveExcessPolygonPoints (Layer, Polygon);
  InitClip (PCB->Data, Layer, Polygon);
  if (Layer->On)
    {
      DrawPolygon (Layer, Polygon);
      Draw ();
    }
  return (Point);
}

/* ---------------------------------------------------------------------------
 * moves a line between layers; lowlevel routines
 */
static void *
MoveLineToLayerLowLevel (LayerType *Source, LineType *line,
			 LayerType *Destination)
{
  r_delete_entry (Source->line_tree, (BoxType *)line);

  Source->Line = g_list_remove (Source->Line, line);
  Source->LineN --;
  Destination->Line = g_list_append (Destination->Line, line);
  Destination->LineN ++;

  if (!Destination->line_tree)
    Destination->line_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (Destination->line_tree, (BoxType *)line, 0);
  return line;
}

/* ---------------------------------------------------------------------------
 * moves an arc between layers; lowlevel routines
 */
static void *
MoveArcToLayerLowLevel (LayerType *Source, ArcType *arc,
			LayerType *Destination)
{
  r_delete_entry (Source->arc_tree, (BoxType *)arc);

  Source->Arc = g_list_remove (Source->Arc, arc);
  Source->ArcN --;
  Destination->Arc = g_list_append (Destination->Arc, arc);
  Destination->ArcN ++;

  if (!Destination->arc_tree)
    Destination->arc_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (Destination->arc_tree, (BoxType *)arc, 0);
  return arc;
}


/* ---------------------------------------------------------------------------
 * moves an arc between layers
 */
static void *
MoveArcToLayer (LayerType *Layer, ArcType *Arc)
{
  ArcType *newone;

  if (TEST_FLAG (LOCKFLAG, Arc))
    {
      Message (_("Sorry, the object is locked\n"));
      return NULL;
    }
  if (Dest == Layer && Layer->On)
    {
      DrawArc (Layer, Arc);
      Draw ();
    }
  if (((long int) Dest == -1) || Dest == Layer)
    return (Arc);
  AddObjectToMoveToLayerUndoList (ARC_TYPE, Layer, Arc, Arc);
  RestoreToPolygon (PCB->Data, ARC_TYPE, Layer, Arc);
  if (Layer->On)
    EraseArc (Arc);
  newone = (ArcType *)MoveArcToLayerLowLevel (Layer, Arc, Dest);
  ClearFromPolygon (PCB->Data, ARC_TYPE, Dest, Arc);
  if (Dest->On)
    DrawArc (Dest, newone);
  Draw ();
  return (newone);
}

/* ---------------------------------------------------------------------------
 * moves a line between layers
 */
static void *
MoveRatToLayer (RatType *Rat)
{
  LineType *newone;
  //Coord X1 = Rat->Point1.X, Y1 = Rat->Point1.Y;
  //Coord X1 = Rat->Point1.X, Y1 = Rat->Point1.Y;
  // if VIAFLAG
  //   if we're on a pin, add a thermal
  //   else make a via and a wire, but 0-length wire not good
  // else as before

  newone = CreateNewLineOnLayer (Dest, Rat->Point1.X, Rat->Point1.Y,
			      Rat->Point2.X, Rat->Point2.Y,
			      Settings.LineThickness, 2 * Settings.Keepaway,
			      Rat->Flags);
  if (TEST_FLAG (CLEARNEWFLAG, PCB))
    SET_FLAG (CLEARLINEFLAG, newone);
  if (!newone)
    return (NULL);
  AddObjectToCreateUndoList (LINE_TYPE, Dest, newone, newone);
  if (PCB->RatOn)
    EraseRat (Rat);
  MoveObjectToRemoveUndoList (RATLINE_TYPE, Rat, Rat, Rat);
  DrawLine (Dest, newone);
  Draw ();
  return (newone);
}

/* ---------------------------------------------------------------------------
 * moves a line between layers
 */

struct via_info
{
  Coord X, Y;
  jmp_buf env;
};

static int
moveline_callback (const BoxType * b, void *cl)
{
  struct via_info *i = (struct via_info *) cl;
  PinType *via;

  if ((via =
       CreateNewVia (PCB->Data, i->X, i->Y,
		     Settings.ViaThickness, 2 * Settings.Keepaway,
		     NOFLAG, Settings.ViaDrillingHole, NULL,
		     NoFlags ())) != NULL)
    {
      AddObjectToCreateUndoList (VIA_TYPE, via, via, via);
      DrawVia (via);
    }
  longjmp (i->env, 1);
}

static void *
MoveLineToLayer (LayerType *Layer, LineType *Line)
{
  struct via_info info;
  BoxType sb;
  LineType *newone;
  void *ptr1, *ptr2, *ptr3;

  if (TEST_FLAG (LOCKFLAG, Line))
    {
      Message (_("Sorry, the object is locked\n"));
      return NULL;
    }
  if (Dest == Layer && Layer->On)
    {
      DrawLine (Layer, Line);
      Draw ();
    }
  if (((long int) Dest == -1) || Dest == Layer)
    return (Line);

  AddObjectToMoveToLayerUndoList (LINE_TYPE, Layer, Line, Line);
  if (Layer->On)
    EraseLine (Line);
  RestoreToPolygon (PCB->Data, LINE_TYPE, Layer, Line);
  newone = (LineType *)MoveLineToLayerLowLevel (Layer, Line, Dest);
  Line = NULL;
  ClearFromPolygon (PCB->Data, LINE_TYPE, Dest, newone);
  if (Dest->On)
    DrawLine (Dest, newone);
  Draw ();
  if (!PCB->ViaOn || MoreToCome ||
      GetLayerGroupNumberByPointer (Layer) ==
      GetLayerGroupNumberByPointer (Dest) ||
      TEST_SILK_LAYER(Layer) ||
      TEST_SILK_LAYER(Dest))
    return (newone);
  /* consider via at Point1 */
  sb.X1 = newone->Point1.X - newone->Thickness / 2;
  sb.X2 = newone->Point1.X + newone->Thickness / 2;
  sb.Y1 = newone->Point1.Y - newone->Thickness / 2;
  sb.Y2 = newone->Point1.Y + newone->Thickness / 2;
  if ((SearchObjectByLocation (PIN_TYPES, &ptr1, &ptr2, &ptr3,
			       newone->Point1.X, newone->Point1.Y,
			       Settings.ViaThickness / 2) == NO_TYPE))
    {
      info.X = newone->Point1.X;
      info.Y = newone->Point1.Y;
      if (setjmp (info.env) == 0)
	r_search (Layer->line_tree, &sb, NULL, moveline_callback, &info);
    }
  /* consider via at Point2 */
  sb.X1 = newone->Point2.X - newone->Thickness / 2;
  sb.X2 = newone->Point2.X + newone->Thickness / 2;
  sb.Y1 = newone->Point2.Y - newone->Thickness / 2;
  sb.Y2 = newone->Point2.Y + newone->Thickness / 2;
  if ((SearchObjectByLocation (PIN_TYPES, &ptr1, &ptr2, &ptr3,
			       newone->Point2.X, newone->Point2.Y,
			       Settings.ViaThickness / 2) == NO_TYPE))
    {
      info.X = newone->Point2.X;
      info.Y = newone->Point2.Y;
      if (setjmp (info.env) == 0)
	r_search (Layer->line_tree, &sb, NULL, moveline_callback, &info);
    }
  Draw ();
  return (newone);
}

/* ---------------------------------------------------------------------------
 * moves a text object between layers; lowlevel routines
 */
static void *
MoveTextToLayerLowLevel (LayerType *Source, TextType *text,
			 LayerType *Destination)
{
  RestoreToPolygon (PCB->Data, TEXT_TYPE, Source, text);
  r_delete_entry (Source->text_tree, (BoxType *)text);

  Source->Text = g_list_remove (Source->Text, text);
  Source->TextN --;
  Destination->Text = g_list_append (Destination->Text, text);
  Destination->TextN ++;

  if (GetLayerGroupNumberByNumber (solder_silk_layer) ==
      GetLayerGroupNumberByPointer (Destination))
    SET_FLAG (ONSOLDERFLAG, text);
  else
    CLEAR_FLAG (ONSOLDERFLAG, text);

  /* re-calculate the bounding box (it could be mirrored now) */
  SetTextBoundingBox (&PCB->Font, text);
  if (!Destination->text_tree)
    Destination->text_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (Destination->text_tree, (BoxType *)text, 0);
  ClearFromPolygon (PCB->Data, TEXT_TYPE, Destination, text);

  return text;
}

/* ---------------------------------------------------------------------------
 * moves a text object between layers
 */
static void *
MoveTextToLayer (LayerType *layer, TextType *text)
{
  if (TEST_FLAG (LOCKFLAG, text))
    {
      Message (_("Sorry, the object is locked\n"));
      return NULL;
    }
  if (Dest != layer)
    {
      AddObjectToMoveToLayerUndoList (TEXT_TYPE, layer, text, text);
      if (layer->On)
	EraseText (layer, text);
      text = MoveTextToLayerLowLevel (layer, text, Dest);
      if (Dest->On)
	DrawText (Dest, text);
      if (layer->On || Dest->On)
	Draw ();
    }
  return text;
}

/* ---------------------------------------------------------------------------
 * moves a polygon between layers; lowlevel routines
 */
static void *
MovePolygonToLayerLowLevel (LayerType *Source, PolygonType *polygon,
			    LayerType *Destination)
{
  r_delete_entry (Source->polygon_tree, (BoxType *)polygon);

  Source->Polygon = g_list_remove (Source->Polygon, polygon);
  Source->PolygonN --;
  Destination->Polygon = g_list_append (Destination->Polygon, polygon);
  Destination->PolygonN ++;

  if (!Destination->polygon_tree)
    Destination->polygon_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (Destination->polygon_tree, (BoxType *)polygon, 0);

  return polygon;
}

struct mptlc
{
  Cardinal snum, dnum;
  int type;
  PolygonType *polygon;
} mptlc;

int
mptl_pin_callback (const BoxType *b, void *cl)
{
  struct mptlc *d = (struct mptlc *) cl;
  PinType *pin = (PinType *) b;
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
MovePolygonToLayer (LayerType *Layer, PolygonType *Polygon)
{
  PolygonType *newone;
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
  newone = (struct polygon_st *)MovePolygonToLayerLowLevel (Layer, Polygon, Dest);
  InitClip (PCB->Data, Dest, newone);
  if (Dest->On)
    {
      DrawPolygon (Dest, newone);
      Draw ();
    }
  return (newone);
}

/* ---------------------------------------------------------------------------
 * moves the object identified by its data pointers and the type
 * not we don't bump the undo serial number
 */
void *
MoveObject (int Type, void *Ptr1, void *Ptr2, void *Ptr3, Coord DX, Coord DY)
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
			 Coord DX, Coord DY)
{
  RubberbandType *ptr;
  void *ptr2;
  int   n;
  Coord PreMoveObjX, PreMoveObjY;

  dprintf("MoveObjectAndRubberband\n");
  if (Type == LINE_TYPE) {
    dprintf("  Line Type\n");
    RestrictMovementGivenRubberBandMode ((LineType *) Ptr3, &DX, &DY);
  }

  /* setup offset */
  DeltaX = DX;
  DeltaY = DY;

  /* regular rubberband move of attached point in attached lines */
  ptr = Crosshair.AttachedObject.Rubberband;
  n = Crosshair.AttachedObject.RubberbandN;
  while (n)
    {
      Coord dX, dY;

      dX = DX; dY = DY;

      /* 
        Centre each moved points in pin/pad.  This has the nice side effect
        or reducing the length of any stub tracks to zero so they will get
        deleted later.
      */
      dprintf("Point Number %d--------------------------------\n",n);
      dprintf("  MovedPoint (%ld,%ld)\n", ptr->MovedPoint->X, ptr->MovedPoint->Y);
      dprintf("  Line       (%ld,%ld) (%ld,%ld)\n", 
             ptr->Line->Point1.X, ptr->Line->Point1.Y,
             ptr->Line->Point2.X, ptr->Line->Point2.Y);

      if ((Type == PIN_TYPE) || (Type == VIA_TYPE)) {
        PinType *PinPtr = (PinType *) Ptr1;
        dprintf("       Pin   (%ld,%ld)\n", PinPtr->X, PinPtr->Y);
        dprintf("       dX dY (%ld,%ld)\n", dX, dY);
        dX += PinPtr->X - ptr->MovedPoint->X;
        dY += PinPtr->Y - ptr->MovedPoint->Y;
        dprintf("       dX dY (%ld,%ld)\n", dX, dY);
      }
      if (Type == PAD_TYPE) {
        PadType *PadPtr = (PadType *) Ptr1;
        PointType PadCentre;

        PadCentre.X = (PadPtr->Point1.X + PadPtr->Point1.X)/2;
        PadCentre.Y = (PadPtr->Point2.Y + PadPtr->Point2.Y)/2;
        dX += PadCentre.X - ptr->MovedPoint->X;
        dY += PadCentre.Y - ptr->MovedPoint->Y;
      }

      DeltaX = dX; DeltaY = dY;
      /* only update undo list if an actual movement happened */
      if (DX != 0 || DY != 0)
        {
          AddObjectToMoveUndoList (LINEPOINT_TYPE,
                               ptr->Layer, ptr->Line, ptr->MovedPoint, dX,
                               dY);
          MoveLinePoint (ptr->Layer, ptr->Line, ptr->MovedPoint);
        }
      n--;
      ptr++;
    }

  if (DX == 0 && DY == 0)
    return (NULL);

  /* move the actual object */
  DeltaX = DX;
  DeltaY = DY;
  AddObjectToMoveUndoList (Type, Ptr1, Ptr2, Ptr3, DX, DY);
  ptr2 = ObjectOperation (&MoveFunctions, Type, Ptr1, Ptr2, Ptr3);

  /* now we do fixups to maintain 45 deg angles during rubberband
     drags */
  switch (Type) {

  case LINE_TYPE:
    {

      /* In rubberband mode we nudge one end of the line we just moved 
         to keep joined lines aligned 45 degrees  */

      n = Crosshair.AttachedObject.RubberbandN;
      ptr = Crosshair.AttachedObject.Rubberband;
      while(n) {
        Coord nudgeX, nudgeY;
        LineType *line = (LineType *) Ptr3;
        PointType PointOut;
        PointType *nudge;

        dprintf("  Nudging a line attached to line %d\n", n);

        MovePointGivenRubberBandMode(&PointOut, ptr->MovedPoint, ptr->Line, 
                                     DX, DY, Crosshair.AttachedObject.Type,
                                     IsDiagonal(line));

        nudgeX = PointOut.X - ptr->MovedPoint->X - DX; 
        nudgeY = PointOut.Y - ptr->MovedPoint->Y - DY;
        dprintf("  nudgeX = %ld  nudgeY = %ld\n", nudgeX, nudgeY);
        DeltaX = nudgeX; DeltaY = nudgeY;

        /* nudge point on attached line */

        /* nudge point on line */
        if ((ptr->MovedPoint->X == line->Point1.X) && 
            (ptr->MovedPoint->Y == line->Point1.Y))
        {
          nudge = &line->Point1;
          dprintf("    nudge point1\n"); 
        }
        else
        {
          dprintf("    nudge point2\n"); 
          nudge = &line->Point2;
        }

        AddObjectToMoveUndoList (LINEPOINT_TYPE,
                                 ptr->Layer, ptr->Line, ptr->MovedPoint, nudgeX,
                                 nudgeY);
        MoveLinePoint (ptr->Layer, ptr->Line, ptr->MovedPoint);

        AddObjectToMoveUndoList (LINEPOINT_TYPE, Ptr1, Ptr2, nudge, 
                                 nudgeX,nudgeY);
        MoveLinePoint (Ptr1, Ptr2, nudge);
        ptr++;
        n--;
      }    
    }
    break;

  case VIA_TYPE:
  case PAD_TYPE:
  case PIN_TYPE:
  case ELEMENT_TYPE:
    {
      /* 
         Generalised rubber band dragging of lines attached objects.  At
         the start start of this code we have a non-45 line 'A' between
         p1-p3. At the end of this code we want something like:


              p2________p3
             /    A     
            /B         
           /         
          p1 

         To make nice angles we need an two lines.  If A is already
         attached to another line we use that as B.  If A is not
         attached we create a line.

         So the steps are (i) find or create B (ii) determine p2 and
         (iii) move B to p1-p2, and A to p2-p3.

         There are actually lots of ways to route lines between p1
         and p3 using an intermediate point p2.  If we wanted to
         be really clever we could test if B and A touch any other
         objects, if so try some other routing methods.
      */

      n = Crosshair.AttachedObject.RubberbandN;
      ptr = Crosshair.AttachedObject.Rubberband;
      while(n) {
        PointType p1, p2, p3;
        PointType *lineA_p2, *lineB_p2; /* inital p2 points before move */
        LineType *lineA = ptr->Line;
        LineType *lineB;
        Coord dX, dY;
        int ignore;
        PinType pin;

        ignore = 0;

        dprintf("  Line attached to an object %d---------------\n", n);    

        if (!TEST_FLAG (RUBBERBANDFLAG, PCB))
          ignore = 1;
        if (TEST_FLAG (ALLDIRECTIONSRUBBERBANDFLAG, PCB))
          ignore = 1;

        /* ignore any xero length objects */
        if ((lineA->Point1.X == lineA->Point2.X) &&
            (lineA->Point1.Y == lineA->Point2.Y))
          ignore = 1;

        if (!ignore) {

        /* map current points at end of line to our model */
          if ((ptr->MovedPoint->X == lineA->Point1.X) && 
              (ptr->MovedPoint->Y == lineA->Point1.Y)) 
            {
              p3 = lineA->Point1;
              lineA_p2 = &lineA->Point2;
            }
          else
            {
              lineA_p2 = &lineA->Point1;
              p3 = lineA->Point2;
            }

          dprintf("    Initial lineA\n");
          dprintf("      p3.X = %ld  p3.Y = %ld\n", p3.X, p3.Y);
          dprintf("      p2.X = %ld  p2.Y = %ld\n", lineA_p2->X, lineA_p2->Y);

          /* now lets find/create line B  */

          if ((lineB = FindLineAttachedToPoint (ptr->Layer, lineA, lineA_p2)) 
              != NULL)
            {
              dprintf("    lineB found!\n");
              if ((lineA_p2->X == lineB->Point1.X) && 
                (lineA_p2->Y == lineB->Point1.Y))
                {
                  p1 = lineB->Point2;
                  lineB_p2 = &lineB->Point1;
                }
              else
                {
                  p1 = lineB->Point1;	      
                  lineB_p2 = &lineB->Point2;
                }
            }
          else
            {
              /* No attached line found, lets create one, coords dont matter
                 for now as it will be moved shortly */

              dprintf("    lineB created!\n");
              lineB = CreateNewLineOnLayer (ptr->Layer, 
                                            lineA_p2->X, 
                                            lineA_p2->Y, p3.X, p3.Y,
                                            lineA->Thickness,
                                            2 * lineA->Clearance,
                                            lineA->Flags);
              AddObjectToCreateUndoList (LINE_TYPE, CURRENT, lineB, lineB);
              p1.X = lineA_p2->X; p1.Y = lineA_p2->Y;
              lineB_p2 = &lineB->Point2;
            }

          dprintf("    Initial lineB\n");
          dprintf("      p1.X = %ld  p1.Y = %ld\n", p1.X, p1.Y);
          dprintf("      p2.X = %ld  p2.Y = %ld\n", lineB_p2->X, lineB_p2->Y);

          /* OK, we now have lineA and lineB, p1, and p3 are set, need to
             determine p2 such that we are routing lines at 45 deg. */
          dY = abs(p3.Y - p1.Y);
          dX = abs(p3.X - p1.X);
          dprintf("    Final dX = %ld  dY = %ld\n", dX, dY);

          if (dX < dY) {
            p2.X = p1.X + SGN(p3.X - p1.X)*dX;
            p2.Y = p1.Y + SGN(p3.Y - p1.Y)*dX;	
          }
          else {
            p2.X = p1.X + SGN(p3.X - p1.X)*dY;
            p2.Y = p1.Y + SGN(p3.Y - p1.Y)*dY;	
          }

          dprintf("    New p2:\n");
          dprintf("      p2.X = %ld  p2.Y = %ld\n", p2.X, p2.Y);

          /* Move lineA so it starts at p2 */
          DeltaX = p2.X - lineA_p2->X; DeltaY = p2.Y - lineA_p2->Y;
          dprintf("    lineA:\n");
          dprintf("      DeltaX = %ld   DeltaY = %ld\n", DeltaX,  DeltaY);
          AddObjectToMoveUndoList (LINEPOINT_TYPE,
                                   ptr->Layer, lineA, lineA_p2, DeltaX,
                                   DeltaY);
          MoveLinePoint (ptr->Layer, lineA, lineA_p2); 

          /* Move lineB so it ends at p2 */
          DeltaX = p2.X - lineB_p2->X; DeltaY = p2.Y - lineB_p2->Y;
          dprintf("    lineB:\n");
          dprintf("      DeltaX = %ld   DeltaY = %ld\n", DeltaX,  DeltaY);
          AddObjectToMoveUndoList (LINEPOINT_TYPE,
                                   ptr->Layer, lineB, lineB_p2, DeltaX,
                                   DeltaY);
          MoveLinePoint (ptr->Layer, lineB, lineB_p2); 
        }

        /* next attached line to object */
        ptr++;
        n--;
      }
    }
  }

  /* final loop to reset counter and clear flags */

  ptr = Crosshair.AttachedObject.Rubberband;
  n = Crosshair.AttachedObject.RubberbandN;
  while (n)
    {
      LineType *line = (LineType *) ptr->Line;
      CLEAR_FLAG (RUBBERENDFLAG, ptr->Line);
      Crosshair.AttachedObject.RubberbandN--;

      /* clean up - delete any zero length lines */
      if ((line->Point1.X == line->Point2.X) &&
          (line->Point1.Y == line->Point2.Y))
        {
          RemoveLine (ptr->Layer, line);
        }

      n--;
      ptr++;
    }

  IncrementUndoSerialNumber ();
  return (ptr2);
}

/* ---------------------------------------------------------------------------
 * moves the object identified by its data pointers and the type
 * to a new layer without changing it's position
 */
void *
MoveObjectToLayer (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
		   LayerType *Target, bool enmasse)
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
bool
MoveSelectedObjectsToLayer (LayerType *Target)
{
  bool changed;

  /* setup global identifiers */
  Dest = Target;
  MoreToCome = true;
  changed = SelectedOperation (&MoveToLayerFunctions, true, ALL_TYPES);
  /* passing true to above operation causes Undoserial to auto-increment */
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

static int
LastLayerInComponentGroup (int layer)
{
  int cgroup = GetLayerGroupNumberByNumber(max_group + COMPONENT_LAYER);
  int lgroup = GetLayerGroupNumberByNumber(layer);
  if (cgroup == lgroup
      && PCB->LayerGroups.Number[lgroup] == 2)
    return 1;
  return 0;
}

static int
LastLayerInSolderGroup (int layer)
{
  int sgroup = GetLayerGroupNumberByNumber(max_group + SOLDER_LAYER);
  int lgroup = GetLayerGroupNumberByNumber(layer);
  if (sgroup == lgroup
      && PCB->LayerGroups.Number[lgroup] == 2)
    return 1;
  return 0;
}

int
MoveLayer (int old_index, int new_index)
{
  int group_of_layer[MAX_LAYER + 2], l, g, i;
  LayerType saved_layer;
  int saved_group;

  AddLayerChangeToUndoList (old_index, new_index);
  IncrementUndoSerialNumber ();

  if (old_index < -1 || old_index >= max_copper_layer)
    {
      Message ("Invalid old layer %d for move: must be -1..%d\n",
	       old_index, max_copper_layer - 1);
      return 1;
    }
  if (new_index < -1 || new_index > max_copper_layer || new_index >= MAX_LAYER)
    {
      Message ("Invalid new layer %d for move: must be -1..%d\n",
	       new_index, max_copper_layer);
      return 1;
    }
  if (old_index == new_index)
    return 0;

  if (new_index == -1
      && LastLayerInComponentGroup (old_index))
    {
      gui->confirm_dialog ("You can't delete the last top-side layer\n", "Ok", NULL);
      return 1;
    }

  if (new_index == -1
      && LastLayerInSolderGroup (old_index))
    {
      gui->confirm_dialog ("You can't delete the last bottom-side layer\n", "Ok", NULL);
      return 1;
    }

  for (l = 0; l < MAX_LAYER+2; l++)
    group_of_layer[l] = -1;

  for (g = 0; g < MAX_LAYER; g++)
    for (i = 0; i < PCB->LayerGroups.Number[g]; i++)
      group_of_layer[PCB->LayerGroups.Entries[g][i]] = g;

  if (old_index == -1)
    {
      LayerType *lp;
      if (max_copper_layer == MAX_LAYER)
	{
	  Message ("No room for new layers\n");
	  return 1;
	}
      /* Create a new layer at new_index. */
      lp = &PCB->Data->Layer[new_index];
      memmove (&PCB->Data->Layer[new_index + 1],
	       &PCB->Data->Layer[new_index],
	       (max_copper_layer + 2 - new_index) * sizeof (LayerType));
      memmove (&group_of_layer[new_index + 1],
	       &group_of_layer[new_index],
	       (max_copper_layer + 2 - new_index) * sizeof (int));
      max_copper_layer++;
      memset (lp, 0, sizeof (LayerType));
      lp->On = 1;
      lp->Name = strdup ("New Layer");
      lp->Color = Settings.LayerColor[new_index];
      lp->SelectedColor = Settings.LayerSelectedColor[new_index];
      for (l = 0; l < max_copper_layer; l++)
	if (LayerStack[l] >= new_index)
	  LayerStack[l]++;
      LayerStack[max_copper_layer - 1] = new_index;
    }
  else if (new_index == -1)
    {
      /* Delete the layer at old_index */
      memmove (&PCB->Data->Layer[old_index],
	       &PCB->Data->Layer[old_index + 1],
	       (max_copper_layer + 2 - old_index - 1) * sizeof (LayerType));
      memset (&PCB->Data->Layer[max_copper_layer + 2 - 1], 0, sizeof (LayerType));
      memmove (&group_of_layer[old_index],
	       &group_of_layer[old_index + 1],
	       (max_copper_layer + 2 - old_index - 1) * sizeof (int));
      for (l = 0; l < max_copper_layer; l++)
	if (LayerStack[l] == old_index)
	  memmove (LayerStack + l,
		   LayerStack + l + 1,
		   (max_copper_layer - l - 1) * sizeof (LayerStack[0]));
      max_copper_layer--;
      for (l = 0; l < max_copper_layer; l++)
	if (LayerStack[l] > old_index)
	  LayerStack[l]--;
    }
  else
    {
      /* Move an existing layer */
      memcpy (&saved_layer, &PCB->Data->Layer[old_index], sizeof (LayerType));
      saved_group = group_of_layer[old_index];
      if (old_index < new_index)
	{
	  memmove (&PCB->Data->Layer[old_index],
		   &PCB->Data->Layer[old_index + 1],
		   (new_index - old_index) * sizeof (LayerType));
	  memmove (&group_of_layer[old_index],
		   &group_of_layer[old_index + 1],
		   (new_index - old_index) * sizeof (int));
	}
      else
	{
	  memmove (&PCB->Data->Layer[new_index + 1],
		   &PCB->Data->Layer[new_index],
		   (old_index - new_index) * sizeof (LayerType));
	  memmove (&group_of_layer[new_index + 1],
		   &group_of_layer[new_index],
		   (old_index - new_index) * sizeof (int));
	}
      memcpy (&PCB->Data->Layer[new_index], &saved_layer, sizeof (LayerType));
      group_of_layer[new_index] = saved_group;
    }

  move_all_thermals(old_index, new_index);

  for (g = 0; g < MAX_LAYER; g++)
    PCB->LayerGroups.Number[g] = 0;
  for (l = 0; l < max_copper_layer + 2; l++)
    {
      g = group_of_layer[l];

      /* XXX: Should this ever happen? */
      if (g < 0)
        continue;

      i = PCB->LayerGroups.Number[g]++;
      PCB->LayerGroups.Entries[g][i] = l;
    }

  for (g = 1; g < MAX_LAYER; g++)
    if (PCB->LayerGroups.Number[g - 1] == 0)
      {
	memmove (&PCB->LayerGroups.Number[g - 1],
		 &PCB->LayerGroups.Number[g],
		 (MAX_LAYER - g) * sizeof (PCB->LayerGroups.Number[g]));
	memmove (&PCB->LayerGroups.Entries[g - 1],
		 &PCB->LayerGroups.Entries[g],
		 (MAX_LAYER - g) * sizeof (PCB->LayerGroups.Entries[g]));
      }

  hid_action ("LayersChanged");
  gui->invalidate_all ();
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char movelayer_syntax[] = "MoveLayer(old,new)";

static const char movelayer_help[] = "Moves/Creates/Deletes Layers.";

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
MoveLayerAction (int argc, char **argv, Coord x, Coord y)
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
      if (new_index >= max_copper_layer)
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
      if (new_top >= max_copper_layer)
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
