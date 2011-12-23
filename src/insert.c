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


/* functions used to insert points into objects
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"

#include "copy.h"
#include "create.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "insert.h"
#include "line.h"
#include "misc.h"
#include "move.h"
#include "polygon.h"
#include "rtree.h"
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
static void *InsertPointIntoLine (LayerTypePtr, LineTypePtr);
static void *InsertPointIntoPolygon (LayerTypePtr, PolygonTypePtr);
static void *InsertPointIntoRat (RatTypePtr);

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static Coord InsertX, InsertY;	/* used by local routines as offset */
static Cardinal InsertAt;
static bool InsertLast;
static bool Forcible;
static ObjectFunctionType InsertFunctions = {
  InsertPointIntoLine,
  NULL,
  InsertPointIntoPolygon,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  InsertPointIntoRat
};

/* ---------------------------------------------------------------------------
 * inserts a point into a rat-line
 */
static void *
InsertPointIntoRat (RatTypePtr Rat)
{
  LineTypePtr newone;

  newone = CreateDrawnLineOnLayer (CURRENT, Rat->Point1.X, Rat->Point1.Y,
				InsertX, InsertY, Settings.LineThickness,
				2 * Settings.Keepaway, Rat->Flags);
  if (!newone)
    return newone;
  AddObjectToCreateUndoList (LINE_TYPE, CURRENT, newone, newone);
  EraseRat (Rat);
  DrawLine (CURRENT, newone);
  newone = CreateDrawnLineOnLayer (CURRENT, Rat->Point2.X, Rat->Point2.Y,
				InsertX, InsertY, Settings.LineThickness,
				2 * Settings.Keepaway, Rat->Flags);
  if (newone)
    {
      AddObjectToCreateUndoList (LINE_TYPE, CURRENT, newone, newone);
      DrawLine (CURRENT, newone);
    }
  MoveObjectToRemoveUndoList (RATLINE_TYPE, Rat, Rat, Rat);
  Draw ();
  return (newone);
}

/* ---------------------------------------------------------------------------
 * inserts a point into a line
 */
static void *
InsertPointIntoLine (LayerTypePtr Layer, LineTypePtr Line)
{
  LineTypePtr line;
  Coord X, Y;

  if (((Line->Point1.X == InsertX) && (Line->Point1.Y == InsertY)) ||
      ((Line->Point2.X == InsertX) && (Line->Point2.Y == InsertY)))
    return (NULL);
  X = Line->Point2.X;
  Y = Line->Point2.Y;
  AddObjectToMoveUndoList (LINEPOINT_TYPE, Layer, Line, &Line->Point2,
			   InsertX - X, InsertY - Y);
  EraseLine (Line);
  r_delete_entry (Layer->line_tree, (BoxTypePtr) Line);
  RestoreToPolygon (PCB->Data, LINE_TYPE, Layer, Line);
  Line->Point2.X = InsertX;
  Line->Point2.Y = InsertY;
  SetLineBoundingBox (Line);
  r_insert_entry (Layer->line_tree, (BoxTypePtr) Line, 0);
  ClearFromPolygon (PCB->Data, LINE_TYPE, Layer, Line);
  DrawLine (Layer, Line);
  /* we must create after playing with Line since creation may
   * invalidate the line pointer
   */
  if ((line = CreateDrawnLineOnLayer (Layer, InsertX, InsertY,
				      X, Y,
				      Line->Thickness, Line->Clearance,
				      Line->Flags)))
    {
      AddObjectToCreateUndoList (LINE_TYPE, Layer, line, line);
      DrawLine (Layer, line);
      ClearFromPolygon (PCB->Data, LINE_TYPE, Layer, line);
      /* creation call adds it to the rtree */
    }
  Draw ();
  return (line);
}

/* ---------------------------------------------------------------------------
 * inserts a point into a polygon
 */
static void *
InsertPointIntoPolygon (LayerTypePtr Layer, PolygonTypePtr Polygon)
{
  PointType save;
  Cardinal n;
  LineType line;

  if (!Forcible)
    {
      /*
       * first make sure adding the point is sensible
       */
      line.Thickness = 0;
      line.Point1 = Polygon->Points[prev_contour_point (Polygon, InsertAt)];
      line.Point2 = Polygon->Points[InsertAt];
      if (IsPointOnLine ((float) InsertX, (float) InsertY, 0.0, &line))
	return (NULL);
    }
  /*
   * second, shift the points up to make room for the new point
   */
  ErasePolygon (Polygon);
  r_delete_entry (Layer->polygon_tree, (BoxTypePtr) Polygon);
  save = *CreateNewPointInPolygon (Polygon, InsertX, InsertY);
  for (n = Polygon->PointN - 1; n > InsertAt; n--)
    Polygon->Points[n] = Polygon->Points[n - 1];

  /* Shift up indices of any holes */
  for (n = 0; n < Polygon->HoleIndexN; n++)
    if (Polygon->HoleIndex[n] > InsertAt ||
	(InsertLast && Polygon->HoleIndex[n] == InsertAt))
      Polygon->HoleIndex[n]++;

  Polygon->Points[InsertAt] = save;
  SetChangedFlag (true);
  AddObjectToInsertPointUndoList (POLYGONPOINT_TYPE, Layer, Polygon,
				  &Polygon->Points[InsertAt]);

  SetPolygonBoundingBox (Polygon);
  r_insert_entry (Layer->polygon_tree, (BoxType *) Polygon, 0);
  InitClip (PCB->Data, Layer, Polygon);
  if (Forcible || !RemoveExcessPolygonPoints (Layer, Polygon))
    {
      DrawPolygon (Layer, Polygon);
      Draw ();
    }
  return (&Polygon->Points[InsertAt]);
}

/* ---------------------------------------------------------------------------
 * inserts point into objects
 */
void *
InsertPointIntoObject (int Type, void *Ptr1, void *Ptr2, Cardinal * Ptr3,
		       Coord DX, Coord DY, bool Force,
		       bool insert_last)
{
  void *ptr;

  /* setup offset */
  InsertX = DX;
  InsertY = DY;
  InsertAt = *Ptr3;
  InsertLast = insert_last;
  Forcible = Force;

  /* the operation insert the points to the undo-list */
  ptr = ObjectOperation (&InsertFunctions, Type, Ptr1, Ptr2, Ptr3);
  if (ptr != NULL)
    IncrementUndoSerialNumber ();
  return (ptr);
}

/* ---------------------------------------------------------------------------
 *  adjusts the insert point to make 45 degree lines as necessary
 */
PointTypePtr
AdjustInsertPoint (void)
{
  static PointType InsertedPoint;
  double m;
  Coord x, y, m1, m2;
  LineTypePtr line = (LineTypePtr) Crosshair.AttachedObject.Ptr2;

  if (Crosshair.AttachedObject.State == STATE_FIRST)
    return NULL;
  Crosshair.AttachedObject.Ptr3 = &InsertedPoint;
  if (gui->shift_is_pressed ())
    {
      AttachedLineType myline;
      /* only force 45 degree for nearest point */
      if (Distance (Crosshair.X, Crosshair.Y, line->Point1.X, line->Point1.Y) <
          Distance (Crosshair.X, Crosshair.Y, line->Point2.X, line->Point2.Y))
	myline.Point1 = myline.Point2 = line->Point1;
      else
	myline.Point1 = myline.Point2 = line->Point2;
      FortyFiveLine (&myline);
      InsertedPoint.X = myline.Point2.X;
      InsertedPoint.Y = myline.Point2.Y;
      return &InsertedPoint;
    }
  if (TEST_FLAG (ALLDIRECTIONFLAG, PCB))
    {
      InsertedPoint.X = Crosshair.X;
      InsertedPoint.Y = Crosshair.Y;
      return &InsertedPoint;
    }
  if (Crosshair.X == line->Point1.X)
    m1 = 2;			/* 2 signals infinite slope */
  else
    {
      m = (double) (Crosshair.Y - line->Point1.Y) / (Crosshair.X - line->Point1.X);
      m1 = 0;
      if (m > TAN_30_DEGREE)
	m1 = (m > TAN_60_DEGREE) ? 2 : 1;
      else if (m < -TAN_30_DEGREE)
	m1 = (m < -TAN_60_DEGREE) ? 2 : -1;
    }
  if (Crosshair.X == line->Point2.X)
    m2 = 2;			/* 2 signals infinite slope */
  else
    {
      m = (double) (Crosshair.Y - line->Point2.Y) / (Crosshair.X - line->Point2.X);
      m2 = 0;
      if (m > TAN_30_DEGREE)
	m2 = (m > TAN_60_DEGREE) ? 2 : 1;
      else if (m < -TAN_30_DEGREE)
	m2 = (m < -TAN_60_DEGREE) ? 2 : -1;
    }
  if (m1 == m2)
    {
      InsertedPoint.X = line->Point1.X;
      InsertedPoint.Y = line->Point1.Y;
      return &InsertedPoint;
    }
  if (m1 == 2)
    {
      x = line->Point1.X;
      y = line->Point2.Y + m2 * (line->Point1.X - line->Point2.X);
    }
  else if (m2 == 2)
    {
      x = line->Point2.X;
      y = line->Point1.Y + m1 * (line->Point2.X - line->Point1.X);
    }
  else
    {
      x = (line->Point2.Y - line->Point1.Y + m1 * line->Point1.X
	   - m2 * line->Point2.X) / (m1 - m2);
      y = (m1 * line->Point2.Y - m1 * m2 * line->Point2.X
	   - m2 * line->Point1.Y + m1 * m2 * line->Point1.X) / (m1 - m2);
    }
  InsertedPoint.X = x;
  InsertedPoint.Y = y;
  return &InsertedPoint;
}
