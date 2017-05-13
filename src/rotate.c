/*!
 * \file src/rotate.c
 *
 * \brief Functions used to rotate pins, elements ...
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

#include <stdlib.h>

#include "global.h"

#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "misc.h"
#include "polygon.h"
#include "rotate.h"
#include "rtree.h"
#include "rubberband.h"
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
static void *RotateText (LayerType *, TextType *);
static void *RotateArc (LayerType *, ArcType *);
static void *RotateElement (ElementType *);
static void *RotateElementName (ElementType *);
static void *RotateLinePoint (LayerType *, LineType *, PointType *);

/* ----------------------------------------------------------------------
 * some local identifiers
 */
static Coord CenterX, CenterY;	/* center of rotation */
static unsigned Number;		/* number of rotations */
static ObjectFunctionType RotateFunctions = {
  NULL,
  RotateText,
  NULL,
  NULL,
  RotateElement,
  RotateElementName,
  NULL,
  NULL,
  RotateLinePoint,
  NULL,
  RotateArc,
  NULL
};

/*!
 * \brief Rotates a point in 90 degree steps.
 */
void
RotatePointLowLevel (PointType *Point, Coord X, Coord Y, unsigned Number)
{
  ROTATE (Point->X, Point->Y, X, Y, Number);
}

/*!
 * \brief Rotates a line in 90 degree steps.
 */
void
RotateLineLowLevel (LineType *Line, Coord X, Coord Y, unsigned Number)
{
  ROTATE (Line->Point1.X, Line->Point1.Y, X, Y, Number);
  ROTATE (Line->Point2.X, Line->Point2.Y, X, Y, Number);
  /* keep horizontal, vertical Point2 > Point1 */
  if (Line->Point1.X == Line->Point2.X)
    {
      if (Line->Point1.Y > Line->Point2.Y)
	{
	  Coord t;
	  t = Line->Point1.Y;
	  Line->Point1.Y = Line->Point2.Y;
	  Line->Point2.Y = t;
	}
    }
  else if (Line->Point1.Y == Line->Point2.Y)
    {
      if (Line->Point1.X > Line->Point2.X)
	{
	  Coord t;
	  t = Line->Point1.X;
	  Line->Point1.X = Line->Point2.X;
	  Line->Point2.X = t;
	}
    }
  /* instead of rotating the bounding box, the call updates both end points too */
  SetLineBoundingBox (Line);
}

/*!
 * \brief Rotates a text in 90 degree steps.
 *
 * Only the bounding box is rotated, text rotation itself is done by the
 * drawing routines.
 */
void
RotateTextLowLevel (TextType *Text, Coord X, Coord Y, unsigned Number)
{
  BYTE number;

  number = TEST_FLAG (ONSOLDERFLAG, Text) ? (4 - Number) & 3 : Number;
  RotateBoxLowLevel (&Text->BoundingBox, X, Y, Number);
  ROTATE (Text->X, Text->Y, X, Y, Number);

  /* set new direction, 0..3,
   * 0-> to the right, 1-> straight up,
   * 2-> to the left, 3-> straight down
   */
  Text->Direction = ((Text->Direction + number) & 0x03);
}

/*!
 * \brief Rotates a polygon in 90 degree steps.
 */
void
RotatePolygonLowLevel (PolygonType *Polygon, Coord X, Coord Y, unsigned Number)
{
  POLYGONPOINT_LOOP (Polygon);
  {
    ROTATE (point->X, point->Y, X, Y, Number);
  }
  END_LOOP;
  RotateBoxLowLevel (&Polygon->BoundingBox, X, Y, Number);
}

/*!
 * \brief Rotates a text object and redraws it.
 */
static void *
RotateText (LayerType *Layer, TextType *Text)
{
  EraseText (Layer, Text);
  RestoreToPolygon (PCB->Data, TEXT_TYPE, Layer, Text);
  r_delete_entry (Layer->text_tree, (BoxType *) Text);
  RotateTextLowLevel (Text, CenterX, CenterY, Number);
  r_insert_entry (Layer->text_tree, (BoxType *) Text, 0);
  ClearFromPolygon (PCB->Data, TEXT_TYPE, Layer, Text);
  DrawText (Layer, Text);
  Draw ();
  return (Text);
}

/*!
 * \brief Rotates an arc.
 */
void
RotateArcLowLevel (ArcType *Arc, Coord X, Coord Y, unsigned Number)
{
  Coord save;

  /* add Number*90 degrees (i.e., Number quarter-turns) */
  Arc->StartAngle = NormalizeAngle (Arc->StartAngle + Number * 90);
  ROTATE (Arc->X, Arc->Y, X, Y, Number);

  /* now change width and height */
  if (Number == 1 || Number == 3)
    {
      save = Arc->Width;
      Arc->Width = Arc->Height;
      Arc->Height = save;
    }
  RotateBoxLowLevel (&Arc->BoundingBox, X, Y, Number);
  ROTATE (Arc->Point1.X, Arc->Point1.Y, X, Y, Number);
  ROTATE (Arc->Point2.X, Arc->Point2.Y, X, Y, Number);
}

/*!
 * \brief Rotate an element in 90 degree steps.
 */
void
RotateElementLowLevel (DataType *Data, ElementType *Element,
		       Coord X, Coord Y, unsigned Number)
{
  /* solder side objects need a different orientation */

  /* the text subroutine decides by itself if the direction
   * is to be corrected
   */
  ELEMENTTEXT_LOOP (Element);
  {
    if (Data && Data->name_tree[n])
      r_delete_entry (Data->name_tree[n], (BoxType *) text);
    RotateTextLowLevel (text, X, Y, Number);
  }
  END_LOOP;
  ELEMENTLINE_LOOP (Element);
  {
    RotateLineLowLevel (line, X, Y, Number);
  }
  END_LOOP;
  PIN_LOOP (Element);
  {
    /* pre-delete the pins from the pin-tree before their coordinates change */
    if (Data)
      r_delete_entry (Data->pin_tree, (BoxType *) pin);
    RestoreToPolygon (Data, PIN_TYPE, Element, pin);
    ROTATE_PIN_LOWLEVEL (pin, X, Y, Number);
  }
  END_LOOP;
  PAD_LOOP (Element);
  {
    /* pre-delete the pads before their coordinates change */
    if (Data)
      r_delete_entry (Data->pad_tree, (BoxType *) pad);
    RestoreToPolygon (Data, PAD_TYPE, Element, pad);
    ROTATE_PAD_LOWLEVEL (pad, X, Y, Number);
  }
  END_LOOP;
  ARC_LOOP (Element);
  {
    RotateArcLowLevel (arc, X, Y, Number);
  }
  END_LOOP;
  ROTATE (Element->MarkX, Element->MarkY, X, Y, Number);
  /* SetElementBoundingBox reenters the rtree data */
  SetElementBoundingBox (Data, Element, &PCB->Font);
  ClearFromPolygon (Data, ELEMENT_TYPE, Element, Element);
}

/*!
 * \brief Rotates a line's point.
 */
static void *
RotateLinePoint (LayerType *Layer, LineType *Line, PointType *Point)
{
  EraseLine (Line);
  if (Layer)
    {
      RestoreToPolygon (PCB->Data, LINE_TYPE, Layer, Line);
      r_delete_entry (Layer->line_tree, (BoxType *) Line);
    }
  else
    r_delete_entry (PCB->Data->rat_tree, (BoxType *) Line);
  RotatePointLowLevel (Point, CenterX, CenterY, Number);
  SetLineBoundingBox (Line);
  if (Layer)
    {
      r_insert_entry (Layer->line_tree, (BoxType *) Line, 0);
      ClearFromPolygon (PCB->Data, LINE_TYPE, Layer, Line);
      DrawLine (Layer, Line);
    }
  else
    {
      r_insert_entry (PCB->Data->rat_tree, (BoxType *) Line, 0);
      DrawRat ((RatType *) Line);
    }
  Draw ();
  return (Line);
}

/*!
 * \brief Rotates an arc.
 */
static void *
RotateArc (LayerType *Layer, ArcType *Arc)
{
  EraseArc (Arc);
  r_delete_entry (Layer->arc_tree, (BoxType *) Arc);
  RestoreToPolygon (PCB->Data, ARC_TYPE, Layer, Arc);
  RotateArcLowLevel (Arc, CenterX, CenterY, Number);
  r_insert_entry (Layer->arc_tree, (BoxType *) Arc, 0);
  ClearFromPolygon (PCB->Data, ARC_TYPE, Layer, Arc);

  DrawArc (Layer, Arc);
  Draw ();
  return (Arc);
}

/*!
 * \brief Rotates an element.
 */
static void *
RotateElement (ElementType *Element)
{
  EraseElement (Element);
  RotateElementLowLevel (PCB->Data, Element, CenterX, CenterY, Number);
  DrawElement (Element);
  Draw ();
  return (Element);
}

/*!
 * \brief Rotates the name of an element.
 */
static void *
RotateElementName (ElementType *Element)
{
  EraseElementName (Element);
  ELEMENTTEXT_LOOP (Element);
  {
    r_delete_entry (PCB->Data->name_tree[n], (BoxType *) text);
    RotateTextLowLevel (text, CenterX, CenterY, Number);
    r_insert_entry (PCB->Data->name_tree[n], (BoxType *) text, 0);
  }
  END_LOOP;
  DrawElementName (Element);
  Draw ();
  return (Element);
}

/*!
 * \brief Rotates a box in 90 degree steps.
 */
void
RotateBoxLowLevel (BoxType *Box, Coord X, Coord Y, unsigned Number)
{
  Coord x1 = Box->X1, y1 = Box->Y1, x2 = Box->X2, y2 = Box->Y2;

  ROTATE (x1, y1, X, Y, Number);
  ROTATE (x2, y2, X, Y, Number);
  Box->X1 = MIN (x1, x2);
  Box->Y1 = MIN (y1, y2);
  Box->X2 = MAX (x1, x2);
  Box->Y2 = MAX (y1, y2);
}

/*!
 * \brief Rotates an objects at the cursor position as identified by its
 * ID.
 *
 * The center of rotation is determined by the current cursor location.
 */
void *
RotateObject (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
	      Coord X, Coord Y, unsigned Steps)
{
  RubberbandType *ptr;
  void *ptr2;
  bool changed = false;

  /* setup default  global identifiers */
  Number = Steps;
  CenterX = X;
  CenterY = Y;

  /* move all the rubberband lines... and reset the counter */
  ptr = Crosshair.AttachedObject.Rubberband;
  while (Crosshair.AttachedObject.RubberbandN)
    {
      changed = true;
      CLEAR_FLAG (RUBBERENDFLAG, ptr->Line);
      AddObjectToRotateUndoList (LINEPOINT_TYPE, ptr->Layer, ptr->Line,
				 ptr->MovedPoint, CenterX, CenterY, Steps);
      EraseLine (ptr->Line);
      if (ptr->Layer)
	{
	  RestoreToPolygon (PCB->Data, LINE_TYPE, ptr->Layer, ptr->Line);
	  r_delete_entry (ptr->Layer->line_tree, (BoxType *) ptr->Line);
	}
      else
	r_delete_entry (PCB->Data->rat_tree, (BoxType *) ptr->Line);
      RotatePointLowLevel (ptr->MovedPoint, CenterX, CenterY, Steps);
      SetLineBoundingBox (ptr->Line);
      if (ptr->Layer)
	{
	  r_insert_entry (ptr->Layer->line_tree, (BoxType *) ptr->Line, 0);
	  ClearFromPolygon (PCB->Data, LINE_TYPE, ptr->Layer, ptr->Line);
	  DrawLine (ptr->Layer, ptr->Line);
	}
      else
	{
	  r_insert_entry (PCB->Data->rat_tree, (BoxType *) ptr->Line, 0);
	  DrawRat ((RatType *) ptr->Line);
	}
      Crosshair.AttachedObject.RubberbandN--;
      ptr++;
    }
  AddObjectToRotateUndoList (Type, Ptr1, Ptr2, Ptr3, CenterX, CenterY,
			     Number);
  ptr2 = ObjectOperation (&RotateFunctions, Type, Ptr1, Ptr2, Ptr3);
  changed |= (ptr2 != NULL);
  if (changed)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (ptr2);
}

void
RotateScreenObject (Coord X, Coord Y, unsigned Steps)
{
  int type;
  void *ptr1, *ptr2, *ptr3;
  if ((type = SearchScreen (X, Y, ROTATE_TYPES, &ptr1, &ptr2,
			    &ptr3)) != NO_TYPE)
    {
      if (TEST_FLAG (LOCKFLAG, (ArcType *) ptr2))
	{
	  Message (_("Sorry, the object is locked\n"));
	  return;
	}
      Crosshair.AttachedObject.RubberbandN = 0;
      if (TEST_FLAG (RUBBERBANDFLAG, PCB))
	LookupRubberbandLines (type, ptr1, ptr2, ptr3);
      if (type == ELEMENT_TYPE)
	LookupRatLines (type, ptr1, ptr2, ptr3);
      RotateObject (type, ptr1, ptr2, ptr3, X, Y, Steps);
      SetChangedFlag (true);
    }
}
