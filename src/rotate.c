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

/* functions used to rotate pins, elements ...
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "global.h"

#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "misc.h"
#include "polygon.h"
#include "rotate.h"
#include "search.h"
#include "select.h"
#include "undo.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void *RotateText (LayerTypePtr, TextTypePtr);
static void *RotateArc (LayerTypePtr, ArcTypePtr);
static void *RotateElement (ElementTypePtr);
static void *RotateElementName (ElementTypePtr);
static void *RotateLinePoint (LayerTypePtr, LineTypePtr, PointTypePtr);

/* ----------------------------------------------------------------------
 * some local identifiers
 */
static Position CenterX,	/* center of rotation */
  CenterY;
static BYTE Number;		/* number of rotations */
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

/* ---------------------------------------------------------------------------
 * rotates a point in 90 degree steps
 */
void
RotatePointLowLevel (PointTypePtr Point, Position X, Position Y, BYTE Number)
{
  ROTATE (Point->X, Point->Y, X, Y, Number);
}

/* ---------------------------------------------------------------------------
 * rotates a line in 90 degree steps
 */
void
RotateLineLowLevel (LineTypePtr Line, Position X, Position Y, BYTE Number)
{
  ROTATE (Line->Point1.X, Line->Point1.Y, X, Y, Number);
  ROTATE (Line->Point2.X, Line->Point2.Y, X, Y, Number);
  /* keep horizontal, vertical Point2 > Point1 */
  if (Line->Point1.X == Line->Point2.X)
    {
      if (Line->Point1.Y > Line->Point2.Y)
	{
	  Position t;
	  t = Line->Point1.Y;
	  Line->Point1.Y = Line->Point2.Y;
	  Line->Point2.Y = t;
	}
    }
  else if (Line->Point1.Y == Line->Point2.Y)
    {
      if (Line->Point1.X > Line->Point2.X)
	{
	  Position t;
	  t = Line->Point1.X;
	  Line->Point1.X = Line->Point2.X;
	  Line->Point2.X = t;
	}
    }

}

/* ---------------------------------------------------------------------------
 * rotates a text in 90 degree steps 
 * only the bounding box is rotated, text rotation itself
 * is done by the drawing routines
 */
void
RotateTextLowLevel (TextTypePtr Text, Position X, Position Y, BYTE Number)
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

/* ---------------------------------------------------------------------------
 * rotates a polygon in 90 degree steps
 */
void
RotatePolygonLowLevel (PolygonTypePtr Polygon,
		       Position X, Position Y, BYTE Number)
{
  POLYGONPOINT_LOOP (Polygon, 
    {
      ROTATE (point->X, point->Y, X, Y, Number);
    }
  );
  RotateBoxLowLevel (&Polygon->BoundingBox, X, Y, Number);
}

/* ---------------------------------------------------------------------------
 * rotates a text object and redraws it
 */
static void *
RotateText (LayerTypePtr Layer, TextTypePtr Text)
{
  EraseText (Text);
  RotateTextLowLevel (Text, CenterX, CenterY, Number);
  DrawText (Layer, Text, 0);
  Draw ();
  return (Text);
}

/* ---------------------------------------------------------------------------
 * rotates an arc
 */
void
RotateArcLowLevel (ArcTypePtr Arc, Position X, Position Y, BYTE Number)
{
  Dimension save;

  /* add Number*90 degrees to the startangle and check for overflow */
  Arc->StartAngle = (Arc->StartAngle + Number * 90) % 360;
  ROTATE (Arc->X, Arc->Y, X, Y, Number);

  /* now change width and height */
  if (Number == 1 || Number == 3)
    {
      save = Arc->Width;
      Arc->Width = Arc->Height;
      Arc->Height = save;
    }
  SetArcBoundingBox (Arc);
}

/* ---------------------------------------------------------------------------
 * rotate an element in 90 degree steps
 */
void
RotateElementLowLevel (ElementTypePtr Element,
		       Position X, Position Y, BYTE Number)
{
  Boolean OnLayout = False;

  ELEMENT_LOOP (PCB->Data, 
    {
      if (element == Element)
	OnLayout = True;
    }
  );
  /* solder side objects need a different orientation */

  /* the text subroutine decides by itself if the direction
   * is to be corrected
   */
  ELEMENTTEXT_LOOP (Element, 
    {
      RotateTextLowLevel (text, X, Y, Number);
    }
  );
  ELEMENTLINE_LOOP (Element, 
    {
      RotateLineLowLevel (line, X, Y, Number);
    }
  );
  PIN_LOOP (Element, 
    {
      ROTATE_PIN_LOWLEVEL (pin, X, Y, Number);
      if (OnLayout)
	UpdatePIPFlags (pin, Element, NULL, NULL, True);
    }
  );
  PAD_LOOP (Element, 
    {
      ROTATE_PAD_LOWLEVEL (pad, X, Y, Number);
    }
  );
  ARC_LOOP (Element, 
    {
      RotateArcLowLevel (arc, X, Y, Number);
    }
  );
  ROTATE (Element->MarkX, Element->MarkY, X, Y, Number);
  SetElementBoundingBox (Element, &PCB->Font);
}

/* ---------------------------------------------------------------------------
 * rotates a line's point
 */
static void *
RotateLinePoint (LayerTypePtr Layer, LineTypePtr Line, PointTypePtr Point)
{
  EraseLine (Line);
  RotatePointLowLevel (Point, CenterX, CenterY, Number);
  DrawLine (Layer, Line, 0);
  Draw ();
  return (Line);
}

/* ---------------------------------------------------------------------------
 * rotates an arc
 */
static void *
RotateArc (LayerTypePtr Layer, ArcTypePtr Arc)
{
  EraseArc (Arc);
  RotateArcLowLevel (Arc, CenterX, CenterY, Number);
  DrawArc (Layer, Arc, 0);
  Draw ();
  return (Arc);
}

/* ---------------------------------------------------------------------------
 * rotates an element
 */
static void *
RotateElement (ElementTypePtr Element)
{
  EraseElement (Element);
  RotateElementLowLevel (Element, CenterX, CenterY, Number);
  DrawElement (Element, 0);
  Draw ();
  return (Element);
}

/* ----------------------------------------------------------------------
 * rotates the name of an element
 */
static void *
RotateElementName (ElementTypePtr Element)
{
  EraseElementName (Element);
  ELEMENTTEXT_LOOP (Element, 
    {
      RotateTextLowLevel (text, CenterX, CenterY, Number);
    }
  );
  DrawElementName (Element, 0);
  Draw ();
  return (Element);
}

/* ---------------------------------------------------------------------------
 * rotates a box in 90 degree steps 
 */
void
RotateBoxLowLevel (BoxTypePtr Box, Position X, Position Y, BYTE Number)
{
  Position x1 = Box->X1, y1 = Box->Y1, x2 = Box->X2, y2 = Box->Y2;

  ROTATE (x1, y1, X, Y, Number);
  ROTATE (x2, y2, X, Y, Number);
  Box->X1 = MIN (x1, x2);
  Box->Y1 = MIN (y1, y2);
  Box->X2 = MAX (x1, x2);
  Box->Y2 = MAX (y1, y2);
}

/* ----------------------------------------------------------------------
 * rotates an objects at the cursor position as identified by its ID
 * the center of rotation is determined by the current cursor location
 */
void *
RotateObject (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
	      Position X, Position Y, BYTE Steps)
{
  RubberbandTypePtr ptr;
  void *ptr2;
  Boolean changed = False;

  /* setup default  global identifiers */
  Number = Steps;
  CenterX = X;
  CenterY = Y;

  /* move all the rubberband lines... and reset the counter */
  ptr = Crosshair.AttachedObject.Rubberband;
  while (Crosshair.AttachedObject.RubberbandN)
    {
      changed = True;
      ptr->Line->Flags &= ~RUBBERENDFLAG;
      AddObjectToRotateUndoList (LINEPOINT_TYPE, ptr->Layer, ptr->Line,
				 ptr->MovedPoint, CenterX, CenterY, Steps);
      EraseLine (ptr->Line);
      RotatePointLowLevel (ptr->MovedPoint, CenterX, CenterY, Steps);
      DrawLine (ptr->Layer, ptr->Line, 0);
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
