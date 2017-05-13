/*!
 * \file src/remove.c
 *
 * \brief Functions used to remove vias, pins ...
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design.
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
static void *DestroyVia (PinType *);
static void *DestroyRat (RatType *);
static void *DestroyLine (LayerType *, LineType *);
static void *DestroyArc (LayerType *, ArcType *);
static void *DestroyText (LayerType *, TextType *);
static void *DestroyPolygon (LayerType *, PolygonType *);
static void *DestroyElement (ElementType *);
static void *RemoveVia (PinType *);
static void *RemoveRat (RatType *);
static void *DestroyPolygonPoint (LayerType *, PolygonType *, PointType *);
static void *RemovePolygonContour (LayerType *, PolygonType *, Cardinal);
static void *RemovePolygonPoint (LayerType *, PolygonType *, PointType *);
static void *RemoveLinePoint (LayerType *, LineType *, PointType *);

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
static DataType *DestroyTarget;
static bool Bulk = false;

/*!
 * \brief Remove PCB.
 */
void
RemovePCB (PCBType *Ptr)
{
  ClearUndoList (true);
  FreePCBMemory (Ptr);
  free (Ptr);
}

/*!
 * \brief Destroys a via.
 */
static void *
DestroyVia (PinType *Via)
{
  r_delete_entry (DestroyTarget->via_tree, (BoxType *) Via);
  free (Via->Name);

  DestroyTarget->Via = g_list_remove (DestroyTarget->Via, Via);
  DestroyTarget->ViaN --;

  g_slice_free (PinType, Via);

  return NULL;
}

/*!
 * \brief Destroys a line from a layer.
 */
static void *
DestroyLine (LayerType *Layer, LineType *Line)
{
  r_delete_entry (Layer->line_tree, (BoxType *) Line);
  free (Line->Number);

  Layer->Line = g_list_remove (Layer->Line, Line);
  Layer->LineN --;

  g_slice_free (LineType, Line);

  return NULL;
}

/*!
 * \brief Destroys an arc from a layer.
 */
static void *
DestroyArc (LayerType *Layer, ArcType *Arc)
{
  r_delete_entry (Layer->arc_tree, (BoxType *) Arc);

  Layer->Arc = g_list_remove (Layer->Arc, Arc);
  Layer->ArcN --;

  g_slice_free (ArcType, Arc);

  return NULL;
}

/*!
 * \brief Destroys a polygon from a layer.
 */
static void *
DestroyPolygon (LayerType *Layer, PolygonType *Polygon)
{
  r_delete_entry (Layer->polygon_tree, (BoxType *) Polygon);
  FreePolygonMemory (Polygon);

  Layer->Polygon = g_list_remove (Layer->Polygon, Polygon);
  Layer->PolygonN --;

  g_slice_free (PolygonType, Polygon);

  return NULL;
}

/*!
 * \brief Removes a polygon-point from a polygon and destroys the data.
 */
static void *
DestroyPolygonPoint (LayerType *Layer,
		     PolygonType *Polygon, PointType *Point)
{
  Cardinal point_idx;
  Cardinal i;
  Cardinal contour;
  Cardinal contour_start, contour_end, contour_points;

  point_idx = polygon_point_idx (Polygon, Point);
  contour = polygon_point_contour (Polygon, point_idx);
  contour_start = (contour == 0) ? 0 : Polygon->HoleIndex[contour - 1];
  contour_end = (contour == Polygon->HoleIndexN) ? Polygon->PointN :
                                                   Polygon->HoleIndex[contour];
  contour_points = contour_end - contour_start;

  if (contour_points <= 3)
    return RemovePolygonContour (Layer, Polygon, contour);

  r_delete_entry (Layer->polygon_tree, (BoxType *) Polygon);

  /* remove point from list, keep point order */
  for (i = point_idx; i < Polygon->PointN - 1; i++)
    Polygon->Points[i] = Polygon->Points[i + 1];
  Polygon->PointN--;

  /* Shift down indices of any holes */
  for (i = 0; i < Polygon->HoleIndexN; i++)
    if (Polygon->HoleIndex[i] > point_idx)
      Polygon->HoleIndex[i]--;

  SetPolygonBoundingBox (Polygon);
  r_insert_entry (Layer->polygon_tree, (BoxType *) Polygon, 0);
  InitClip (PCB->Data, Layer, Polygon);
  return (Polygon);
}

/*!
 * \brief Destroys a text from a layer.
 */
static void *
DestroyText (LayerType *Layer, TextType *Text)
{
  free (Text->TextString);
  r_delete_entry (Layer->text_tree, (BoxType *) Text);

  Layer->Text = g_list_remove (Layer->Text, Text);
  Layer->TextN --;

  g_slice_free (TextType, Text);

  return NULL;
}

/* ---------------------------------------------------------------------------
 * destroys a element
 */
static void *
DestroyElement (ElementType *Element)
{
  if (DestroyTarget->element_tree)
    r_delete_entry (DestroyTarget->element_tree, (BoxType *) Element);
  if (DestroyTarget->pin_tree)
    {
      PIN_LOOP (Element);
      {
	r_delete_entry (DestroyTarget->pin_tree, (BoxType *) pin);
      }
      END_LOOP;
    }
  if (DestroyTarget->pad_tree)
    {
      PAD_LOOP (Element);
      {
	r_delete_entry (DestroyTarget->pad_tree, (BoxType *) pad);
      }
      END_LOOP;
    }
  ELEMENTTEXT_LOOP (Element);
  {
    if (DestroyTarget->name_tree[n])
      r_delete_entry (DestroyTarget->name_tree[n], (BoxType *) text);
  }
  END_LOOP;
  FreeElementMemory (Element);

  DestroyTarget->Element = g_list_remove (DestroyTarget->Element, Element);
  DestroyTarget->ElementN --;

  g_slice_free (ElementType, Element);

  return NULL;
}

/*!
 * \brief Destroys a rat.
 */
static void *
DestroyRat (RatType *Rat)
{
  if (DestroyTarget->rat_tree)
    r_delete_entry (DestroyTarget->rat_tree, &Rat->BoundingBox);

  DestroyTarget->Rat = g_list_remove (DestroyTarget->Rat, Rat);
  DestroyTarget->RatN --;

  g_slice_free (RatType, Rat);

  return NULL;
}


/*!
 * \brief Removes a via.
 */
static void *
RemoveVia (PinType *Via)
{
  /* erase from screen and memory */
  if (PCB->ViaOn)
    {
      EraseVia (Via);
      if (!Bulk)
	Draw ();
    }
  MoveObjectToRemoveUndoList (VIA_TYPE, Via, Via, Via);
  return NULL;
}

/*!
 * \brief Removes a rat.
 */
static void *
RemoveRat (RatType *Rat)
{
  /* erase from screen and memory */
  if (PCB->RatOn)
    {
      EraseRat (Rat);
      if (!Bulk)
	Draw ();
    }
  MoveObjectToRemoveUndoList (RATLINE_TYPE, Rat, Rat, Rat);
  return NULL;
}

struct rlp_info
{
  jmp_buf env;
  LineType *line;
  PointType *point;
};
static int
remove_point (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  struct rlp_info *info = (struct rlp_info *) cl;
  if (line == info->line)
    return 0;
  if ((line->Point1.X == info->point->X)
      && (line->Point1.Y == info->point->Y))
    {
      info->line = line;
      info->point = &line->Point1;
      longjmp (info->env, 1);
    }
  else
    if ((line->Point2.X == info->point->X)
	&& (line->Point2.Y == info->point->Y))
    {
      info->line = line;
      info->point = &line->Point2;
      longjmp (info->env, 1);
    }
  return 0;
}

/*!
 * \brief Removes a line point, or a line if the selected point is the
 * end.
 */
static void *
RemoveLinePoint (LayerType *Layer, LineType *Line, PointType *Point)
{
  PointType other;
  struct rlp_info info;
  if (&Line->Point1 == Point)
    other = Line->Point2;
  else
    other = Line->Point1;
  info.line = Line;
  info.point = Point;
  if (setjmp (info.env) == 0)
    {
      r_search (Layer->line_tree, (const BoxType *) Point, NULL, remove_point,
		&info);
      return RemoveLine (Layer, Line);
    }
  MoveObject (LINEPOINT_TYPE, Layer, info.line, info.point,
	      other.X - Point->X, other.Y - Point->Y);
  return (RemoveLine (Layer, Line));
}

/*!
 * \brief Removes a line from a layer.
 */
void *
RemoveLine (LayerType *Layer, LineType *Line)
{
  /* erase from screen */
  if (Layer->On)
    {
      EraseLine (Line);
      if (!Bulk)
	Draw ();
    }
  MoveObjectToRemoveUndoList (LINE_TYPE, Layer, Line, Line);
  return NULL;
}

/*!
 * \brief Removes an arc from a layer.
 */
void *
RemoveArc (LayerType *Layer, ArcType *Arc)
{
  /* erase from screen */
  if (Layer->On)
    {
      EraseArc (Arc);
      if (!Bulk)
	Draw ();
    }
  MoveObjectToRemoveUndoList (ARC_TYPE, Layer, Arc, Arc);
  return NULL;
}

/*!
 * \brief Removes a text from a layer.
 */
void *
RemoveText (LayerType *Layer, TextType *Text)
{
  /* erase from screen */
  if (Layer->On)
    {
      EraseText (Layer, Text);
      r_delete_entry (Layer->text_tree, (BoxType *) Text);
      if (!Bulk)
	Draw ();
    }
  MoveObjectToRemoveUndoList (TEXT_TYPE, Layer, Text, Text);
  return NULL;
}

/*!
 * \brief Removes a polygon from a layer.
 */
void *
RemovePolygon (LayerType *Layer, PolygonType *Polygon)
{
  /* erase from screen */
  if (Layer->On)
    {
      ErasePolygon (Polygon);
      if (!Bulk)
	Draw ();
    }
  MoveObjectToRemoveUndoList (POLYGON_TYPE, Layer, Polygon, Polygon);
  return NULL;
}

/*!
 * \brief Removes a contour from a polygon.
 *
 * If removing the outer contour, it removes the whole polygon.
 */
static void *
RemovePolygonContour (LayerType *Layer,
                      PolygonType *Polygon,
                      Cardinal contour)
{
  Cardinal contour_start, contour_end, contour_points;
  Cardinal i;

  if (contour == 0)
    return RemovePolygon (Layer, Polygon);

  if (Layer->On)
    {
      ErasePolygon (Polygon);
      if (!Bulk)
        Draw ();
    }

  /* Copy the polygon to the undo list */
  AddObjectToRemoveContourUndoList (POLYGON_TYPE, Layer, Polygon);

  contour_start = (contour == 0) ? 0 : Polygon->HoleIndex[contour - 1];
  contour_end = (contour == Polygon->HoleIndexN) ? Polygon->PointN :
                                                   Polygon->HoleIndex[contour];
  contour_points = contour_end - contour_start;

  /* remove points from list, keep point order */
  for (i = contour_start; i < Polygon->PointN - contour_points; i++)
    Polygon->Points[i] = Polygon->Points[i + contour_points];
  Polygon->PointN -= contour_points;

  /* remove hole from list and shift down remaining indices */
  for (i = contour; i < Polygon->HoleIndexN; i++)
    Polygon->HoleIndex[i - 1] = Polygon->HoleIndex[i] - contour_points;
  Polygon->HoleIndexN--;

  InitClip (PCB->Data, Layer, Polygon);
  /* redraw polygon if necessary */
  if (Layer->On)
    {
      DrawPolygon (Layer, Polygon);
      if (!Bulk)
        Draw ();
    }
  return NULL;
}

/*!
 * \brief Removes a polygon-point from a polygon.
 */
static void *
RemovePolygonPoint (LayerType *Layer,
		    PolygonType *Polygon, PointType *Point)
{
  Cardinal point_idx;
  Cardinal i;
  Cardinal contour;
  Cardinal contour_start, contour_end, contour_points;

  point_idx = polygon_point_idx (Polygon, Point);
  contour = polygon_point_contour (Polygon, point_idx);
  contour_start = (contour == 0) ? 0 : Polygon->HoleIndex[contour - 1];
  contour_end = (contour == Polygon->HoleIndexN) ? Polygon->PointN :
                                                   Polygon->HoleIndex[contour];
  contour_points = contour_end - contour_start;

  if (contour_points <= 3)
    return RemovePolygonContour (Layer, Polygon, contour);

  if (Layer->On)
    ErasePolygon (Polygon);

  /* insert the polygon-point into the undo list */
  AddObjectToRemovePointUndoList (POLYGONPOINT_TYPE, Layer, Polygon, point_idx);
  r_delete_entry (Layer->polygon_tree, (BoxType *) Polygon);

  /* remove point from list, keep point order */
  for (i = point_idx; i < Polygon->PointN - 1; i++)
    Polygon->Points[i] = Polygon->Points[i + 1];
  Polygon->PointN--;

  /* Shift down indices of any holes */
  for (i = 0; i < Polygon->HoleIndexN; i++)
    if (Polygon->HoleIndex[i] > point_idx)
      Polygon->HoleIndex[i]--;

  SetPolygonBoundingBox (Polygon);
  r_insert_entry (Layer->polygon_tree, (BoxType *) Polygon, 0);
  RemoveExcessPolygonPoints (Layer, Polygon);
  InitClip (PCB->Data, Layer, Polygon);

  /* redraw polygon if necessary */
  if (Layer->On)
    {
      DrawPolygon (Layer, Polygon);
      if (!Bulk)
	Draw ();
    }
  return NULL;
}

/*!
 * \brief Removes an element.
 */
void *
RemoveElement (ElementType *Element)
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
  return NULL;
}

/*!
 * \brief Removes all selected and visible objects.
 *
 * \return true if any objects have been removed.
 */
bool
RemoveSelected (void)
{
  Bulk = true;
  if (SelectedOperation (&RemoveFunctions, false, ALL_TYPES))
    {
      IncrementUndoSerialNumber ();
      Draw ();
      Bulk = false;
      return (true);
    }
  Bulk = false;
  return (false);
}

void
RemoveDegradedVias (void)
{
  Bulk = true;
  VIA_LOOP (PCB->Data);
    {
      if (VIA_IS_BURIED (via)
          && (via->BuriedFrom == via->BuriedTo))
        RemoveVia (via);
    }
  END_LOOP;
  Bulk = false;
}

/*!
 * \brief Remove object as referred by pointers and type,
 * allocated memory is passed to the 'remove undo' list.
 */
void *
RemoveObject (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  void *ptr = ObjectOperation (&RemoveFunctions, Type, Ptr1, Ptr2, Ptr3);
  return (ptr);
}

/*!
 * \brief Deletes rat lines only.
 *
 * Can delete all rat lines, or only selected one.
 */
bool
DeleteRats (bool selected)
{
  bool changed = false;
  Bulk = true;
  RAT_LOOP (PCB->Data);
  {
    if ((!selected) || TEST_FLAG (SELECTEDFLAG, line))
      {
	changed = true;
	RemoveRat (line);
      }
  }
  END_LOOP;
  Bulk = false;
  if (changed)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (changed);
}

/*!
 * \brief Remove object as referred by pointers and type.
 *
 * Allocated memory is destroyed assumed to already be erased.
 */
void *
DestroyObject (DataType *Target, int Type, void *Ptr1,
	       void *Ptr2, void *Ptr3)
{
  DestroyTarget = Target;
  return (ObjectOperation (&DestroyFunctions, Type, Ptr1, Ptr2, Ptr3));
}
