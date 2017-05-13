/*!
 * \file src/move.c
 *
 * \brief Functions used to move pins, elements ...
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
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
 *
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *
 * Thomas.Nau@rz.uni-ulm.de
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

/*!
 * \brief Moves a element by +-X and +-Y.
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

/*!
 * \brief Moves all names of an element to a new position.
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

/*!
 * \brief Moves an element.
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

/*!
 * \brief Moves a via.
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

/*!
 * \brief Moves a line.
 */
static void *
MoveLine (LayerType *Layer, LineType *Line)
{
  if (Layer->On)
    EraseLine (Line);
  RestoreToPolygon (PCB->Data, LINE_TYPE, Layer, Line);
  r_delete_entry (Layer->line_tree, (BoxType *)Line);
  MOVE_LINE_LOWLEVEL (Line, DeltaX, DeltaY);
  r_insert_entry (Layer->line_tree, (BoxType *)Line, 0);
  ClearFromPolygon (PCB->Data, LINE_TYPE, Layer, Line);
  if (Layer->On)
    {
      DrawLine (Layer, Line);
      Draw ();
    }
  return (Line);
}

/*!
 * \brief Moves an arc.
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

/*!
 * \brief Moves a text object.
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

/*!
 * \brief Low level routine to move a polygon.
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

/*!
 * \brief Moves a polygon.
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

/*!
 * \brief Moves one end of a line.
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

/*!
 * \brief Moves a polygon-point.
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

/*!
 * \brief Moves a line between layers; lowlevel routines.
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

/*!
 * \brief Moves an arc between layers; lowlevel routines.
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


/*!
 * \brief Moves an arc between layers.
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

/*!
 * \brief Moves a line between layers.
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

/*!
 * \brief Moves a line between layers.
 */

struct via_info
{
  Coord X, Y;
  Cardinal layer_from, layer_to;
  jmp_buf env;
};

static int
moveline_callback (const BoxType * b, void *cl)
{
  struct via_info *i = (struct via_info *) cl;
  PinType *via;

  if ((via =
       CreateNewViaEx (PCB->Data, i->X, i->Y,
		     Settings.ViaThickness, 2 * Settings.Keepaway,
		     NOFLAG, Settings.ViaDrillingHole, NULL,
		     NoFlags (), i->layer_from, i->layer_to)) != NULL)
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

  if (TEST_FLAG (AUTOBURIEDVIASFLAG, PCB))
    {
      info.layer_from = GetLayerNumber (PCB->Data, Layer);
      info.layer_to = GetLayerNumber (PCB->Data, Dest);
    }
  else
    {
      info.layer_from = 0;
      info.layer_to = 0;
    }
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

/*!
 * \brief Moves a text object between layers; lowlevel routines.
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

  if (GetLayerGroupNumberBySide (BOTTOM_SIDE) ==
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

/*!
 * \brief Moves a text object between layers.
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

/*!
 * \brief Moves a polygon between layers; lowlevel routines.
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

/*!
 * \brief Moves a polygon between layers.
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

/*!
 * \brief Moves the object identified by its data pointers and the type
 * not we don't bump the undo serial number.
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

/*!
 * \brief Moves the object identified by its data pointers and the type
 * as well as all attached rubberband lines.
 */
void *
MoveObjectAndRubberband (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
			 Coord DX, Coord DY)
{
  RubberbandType *ptr;
  void *ptr2;

  /* setup offset */
  DeltaX = DX;
  DeltaY = DY;

  /* move all the lines... and reset the counter */
  ptr = Crosshair.AttachedObject.Rubberband;
  while (Crosshair.AttachedObject.RubberbandN)
    {
      /* first clear any marks that we made in the line flags */
      CLEAR_FLAG (RUBBERENDFLAG, ptr->Line);
      /* only update undo list if an actual movement happened */
      if (DX != 0 || DY != 0)
        {
          AddObjectToMoveUndoList (LINEPOINT_TYPE,
                                   ptr->Layer, ptr->Line,
                                   ptr->MovedPoint, DX, DY);
          MoveLinePoint (ptr->Layer, ptr->Line, ptr->MovedPoint);
        }
      Crosshair.AttachedObject.RubberbandN--;
      ptr++;
    }

  if (DX == 0 && DY == 0)
    return (NULL);

  AddObjectToMoveUndoList (Type, Ptr1, Ptr2, Ptr3, DX, DY);
  ptr2 = ObjectOperation (&MoveFunctions, Type, Ptr1, Ptr2, Ptr3);
  IncrementUndoSerialNumber ();
  return (ptr2);
}

/*!
 * \brief Moves the object identified by its data pointers and the type
 * to a new layer without changing it's position.
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

/*!
 * \brief Moves the selected objects to a new layer without changing
 * their positions.
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
LastNormalLayerInSideGroup (int side, int layer)
{
  int side_group = GetLayerGroupNumberBySide (side);
  int lgroup = GetLayerGroupNumberByNumber (layer);
  if (side_group == lgroup
      && PCB->LayerGroups.Number[lgroup] == 2)
    return 1;
  return 0;
}

/*!
 * \brief Moves the selected layers to a new index in the layer list.
 *
 * index is 0..MAX_ALL_LAYER-1.
 * If old_index is -1, a new layer is inserted at that index.
 * If new_index is -1, the specified layer is deleted.
 *
 * \return non-zero on error, zero if OK.
 */
int
MoveLayer (int old_index, int new_index)
{
  int group_of_layer[MAX_ALL_LAYER], l, g, i;
  LayerType saved_layer;
  int saved_group;

  AddLayerChangeToUndoList (old_index, new_index);

  if (old_index < -1 || old_index >= max_copper_layer)
    {
      Message ("Invalid old layer %d for move: must be -1..%d\n",
	       old_index, max_copper_layer - 1);
      return 1;
    }
  if (new_index < -1 || new_index > max_copper_layer ||
      new_index >= MAX_LAYER)
    {
      Message ("Invalid new layer %d for move: must be -1..%d\n",
	       new_index, max_copper_layer);
      return 1;
    }
  if (old_index == new_index)
    return 0;

  if (new_index == -1
      && LastNormalLayerInSideGroup (TOP_SIDE, old_index))
    {
      gui->confirm_dialog ("You can't delete the last top-side layer\n", "Ok", NULL);
      return 1;
    }

  if (new_index == -1
      && LastNormalLayerInSideGroup (BOTTOM_SIDE, old_index))
    {
      gui->confirm_dialog ("You can't delete the last bottom-side layer\n", "Ok", NULL);
      return 1;
    }

  for (l = 0; l < MAX_ALL_LAYER; l++)
    group_of_layer[l] = -1;

  for (g = 0; g < MAX_GROUP; g++)
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
	       (max_copper_layer + SILK_LAYER - new_index) * sizeof (LayerType));
      memmove (&group_of_layer[new_index + 1],
         &group_of_layer[new_index],
         (max_copper_layer + SILK_LAYER - new_index) * sizeof (int));
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

      if (!Undoing ())
        ChangeBuriedViasAfterLayerCreate (new_index);
    }
  else if (new_index == -1)
    {
      /* Delete the layer at old_index */
      memmove (&PCB->Data->Layer[old_index],
	       &PCB->Data->Layer[old_index + 1],
         (max_copper_layer + SILK_LAYER - old_index - 1) *
            sizeof (LayerType));
      memset (&PCB->Data->Layer[max_copper_layer + SILK_LAYER - 1],
              0, sizeof (LayerType));
      memmove (&group_of_layer[old_index],
	       &group_of_layer[old_index + 1],
	       (max_copper_layer + SILK_LAYER - old_index - 1) * sizeof (int));
      for (l = 0; l < max_copper_layer; l++)
	if (LayerStack[l] == old_index)
	  memmove (LayerStack + l,
		   LayerStack + l + 1,
		   (max_copper_layer - l - 1) * sizeof (LayerStack[0]));
      max_copper_layer--;
      for (l = 0; l < max_copper_layer; l++)
	if (LayerStack[l] > old_index)
	  LayerStack[l]--;

      if (!Undoing ())
        ChangeBuriedViasAfterLayerDelete (old_index);
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

      if (!Undoing ())
         ChangeBuriedViasAfterLayerMove (old_index, new_index);
    }

  IncrementUndoSerialNumber ();

  move_all_thermals(old_index, new_index);

  for (g = 0; g < MAX_GROUP; g++)
    PCB->LayerGroups.Number[g] = 0;
  for (l = 0; l < max_copper_layer + SILK_LAYER; l++)
    {
      g = group_of_layer[l];

      /* XXX: Should this ever happen? */
      if (g < 0)
        continue;

      i = PCB->LayerGroups.Number[g]++;
      PCB->LayerGroups.Entries[g][i] = l;
    }

  for (g = 1; g < MAX_GROUP; g++)
    if (PCB->LayerGroups.Number[g - 1] == 0)
      {
	memmove (&PCB->LayerGroups.Number[g - 1],
		 &PCB->LayerGroups.Number[g],
		 (MAX_GROUP - g) * sizeof (PCB->LayerGroups.Number[g]));
	memmove (&PCB->LayerGroups.Entries[g - 1],
		 &PCB->LayerGroups.Entries[g],
		 (MAX_GROUP - g) * sizeof (PCB->LayerGroups.Entries[g]));
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
