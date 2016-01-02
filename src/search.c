/*!
 * \file src/search.c
 *
 * \brief Search routines.
 *
 * Some of the functions use dummy parameters.
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

#include <math.h>
#include <setjmp.h>

#include "global.h"

#include "box.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "find.h"
#include "misc.h"
#include "polygon.h"
#include "rtree.h"
#include "search.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static double PosX, PosY;		/* search position for subroutines */
static Coord SearchRadius;
static BoxType SearchBox;
static LayerType *SearchLayer;

/* ---------------------------------------------------------------------------
 * some local prototypes.  The first parameter includes LOCKED_TYPE if we
 * want to include locked types in the search.
 */
static bool SearchLineByLocation (int, LayerType **, LineType **,
				     LineType **);
static bool SearchArcByLocation (int, LayerType **, ArcType **,
				    ArcType **);
static bool SearchRatLineByLocation (int, RatType **, RatType **,
					RatType **);
static bool SearchTextByLocation (int, LayerType **, TextType **,
				     TextType **);
static bool SearchPolygonByLocation (int, LayerType **, PolygonType **,
					PolygonType **);
static bool SearchPinByLocation (int, ElementType **, PinType **,
				    PinType **);
static bool SearchPadByLocation (int, ElementType **, PadType **,
				    PadType **, bool);
static bool SearchViaByLocation (int, PinType **, PinType **,
				    PinType **);
static bool SearchElementNameByLocation (int, ElementType **,
					    TextType **, TextType **,
					    bool);
static bool SearchLinePointByLocation (int, LayerType **, LineType **,
					  PointType **);
static bool SearchPointByLocation (int, LayerType **, PolygonType **,
				      PointType **);
static bool SearchElementByLocation (int, ElementType **,
					ElementType **, ElementType **,
					bool);

struct ans_info
{
  void **ptr1, **ptr2, **ptr3;
  bool BackToo;
  double area;
  jmp_buf env;
  int locked; /*!< This will be zero or \c LOCKFLAG. */
  bool found_anything;
  double nearest_sq_dist;
};

static int
pinorvia_callback (const BoxType * box, void *cl)
{
  struct ans_info *i = (struct ans_info *) cl;
  PinType *pin = (PinType *) box;
  AnyObjectType *ptr1 = pin->Element ? pin->Element : pin;

  if (TEST_FLAG (i->locked, ptr1))
    return 0;

  if (!IsPointOnPin (PosX, PosY, SearchRadius, pin))
    return 0;
  *i->ptr1 = ptr1;
  *i->ptr2 = *i->ptr3 = pin;
  longjmp (i->env, 1);
  return 1;			/* never reached */
}

/*!
 * \brief Searches a via.
 */
static bool
SearchViaByLocation (int locked, PinType ** Via, PinType ** Dummy1,
		     PinType ** Dummy2)
{
  struct ans_info info;

  /* search only if via-layer is visible */
  if (!PCB->ViaOn)
    return false;

  info.ptr1 = (void **) Via;
  info.ptr2 = (void **) Dummy1;
  info.ptr3 = (void **) Dummy2;
  info.locked = (locked & LOCKED_TYPE) ? 0 : LOCKFLAG;

  if (setjmp (info.env) == 0)
    {
      r_search (PCB->Data->via_tree, &SearchBox, NULL, pinorvia_callback,
		&info);
      return false;
    }
  return true;
}

/*!
 * \brief Searches a pin.
 *
 * Starts with the newest element.
 */
static bool
SearchPinByLocation (int locked, ElementType ** Element, PinType ** Pin,
		     PinType ** Dummy)
{
  struct ans_info info;

  /* search only if pin-layer is visible */
  if (!PCB->PinOn)
    return false;
  info.ptr1 = (void **) Element;
  info.ptr2 = (void **) Pin;
  info.ptr3 = (void **) Dummy;
  info.locked = (locked & LOCKED_TYPE) ? 0 : LOCKFLAG;

  if (setjmp (info.env) == 0)
    r_search (PCB->Data->pin_tree, &SearchBox, NULL, pinorvia_callback,
	      &info);
  else
    return true;
  return false;
}

static int
pad_callback (const BoxType * b, void *cl)
{
  PadType *pad = (PadType *) b;
  struct ans_info *i = (struct ans_info *) cl;
  AnyObjectType *ptr1 = pad->Element;
  double sq_dist;

  /* Reject locked pads, backside pads (if !BackToo), and non-hit pads */
  if (TEST_FLAG (i->locked, ptr1) ||
      (!FRONT (pad) && !i->BackToo) ||
      !IsPointInPad (PosX, PosY, SearchRadius, pad))
    return 0;

  /* Determine how close our test-position was to the center of the pad  */
  sq_dist = (PosX - (pad->Point1.X + (pad->Point2.X - pad->Point1.X) / 2)) *
            (PosX - (pad->Point1.X + (pad->Point2.X - pad->Point1.X) / 2)) +
            (PosY - (pad->Point1.Y + (pad->Point2.Y - pad->Point1.Y) / 2)) *
            (PosY - (pad->Point1.Y + (pad->Point2.Y - pad->Point1.Y) / 2));

  /* If this was the closest hit so far, record it */
  if (!i->found_anything || sq_dist < i->nearest_sq_dist)
    {
      *i->ptr1 = ptr1;
      *i->ptr2 = *i->ptr3 = pad;
      i->found_anything = true;
      i->nearest_sq_dist = sq_dist;
    }
  return 0;
}

/*!
 * \brief Searches a pad.
 *
 * Starts with the newest element.
 */
static bool
SearchPadByLocation (int locked, ElementType ** Element, PadType ** Pad,
		     PadType ** Dummy, bool BackToo)
{
  struct ans_info info;

  /* search only if pin-layer is visible */
  if (!PCB->PinOn)
    return (false);
  info.ptr1 = (void **) Element;
  info.ptr2 = (void **) Pad;
  info.ptr3 = (void **) Dummy;
  info.locked = (locked & LOCKED_TYPE) ? 0 : LOCKFLAG;
  info.BackToo = (BackToo && PCB->InvisibleObjectsOn);
  info.found_anything = false;
  r_search (PCB->Data->pad_tree, &SearchBox, NULL, pad_callback, &info);
  return info.found_anything;
}

struct line_info
{
  LineType **Line;
  PointType **Point;
  double least;
  jmp_buf env;
  int locked;
};

static int
line_callback (const BoxType * box, void *cl)
{
  struct line_info *i = (struct line_info *) cl;
  LineType *l = (LineType *) box;

  if (TEST_FLAG (i->locked, l))
    return 0;

  if (!IsPointInPad (PosX, PosY, SearchRadius, (PadType *)l))
    return 0;
  *i->Line = l;
  *i->Point = (PointType *) l;
  longjmp (i->env, 1);
  return 1;			/* never reached */
}


/*!
 * \brief Searches ordinary line on the SearchLayer.
 */
static bool
SearchLineByLocation (int locked, LayerType ** Layer, LineType ** Line,
		      LineType ** Dummy)
{
  struct line_info info;

  info.Line = Line;
  info.Point = (PointType **) Dummy;
  info.locked = (locked & LOCKED_TYPE) ? 0 : LOCKFLAG;

  *Layer = SearchLayer;
  if (setjmp (info.env) == 0)
    {
      r_search (SearchLayer->line_tree, &SearchBox, NULL, line_callback,
		&info);
      return false;
    }
  return (true);
}

static int
rat_callback (const BoxType * box, void *cl)
{
  LineType *line = (LineType *) box;
  struct ans_info *i = (struct ans_info *) cl;

  if (TEST_FLAG (i->locked, line))
    return 0;

  if (TEST_FLAG (VIAFLAG, line) ?
      (Distance (line->Point1.X, line->Point1.Y, PosX, PosY) <=
	   line->Thickness * 2 + SearchRadius) :
      IsPointOnLine (PosX, PosY, SearchRadius, line))
    {
      *i->ptr1 = *i->ptr2 = *i->ptr3 = line;
      longjmp (i->env, 1);
    }
  return 0;
}

/*!
 * \brief Searches rat lines if they are visible.
 */
static bool
SearchRatLineByLocation (int locked, RatType ** Line, RatType ** Dummy1,
			 RatType ** Dummy2)
{
  struct ans_info info;

  info.ptr1 = (void **) Line;
  info.ptr2 = (void **) Dummy1;
  info.ptr3 = (void **) Dummy2;
  info.locked = (locked & LOCKED_TYPE) ? 0 : LOCKFLAG;

  if (setjmp (info.env) == 0)
    {
      r_search (PCB->Data->rat_tree, &SearchBox, NULL, rat_callback, &info);
      return false;
    }
  return (true);
}

struct arc_info
{
  ArcType **Arc, **Dummy;
  PointType **Point;
  double least;
  jmp_buf env;
  int locked;
};

static int
arc_callback (const BoxType * box, void *cl)
{
  struct arc_info *i = (struct arc_info *) cl;
  ArcType *a = (ArcType *) box;

  if (TEST_FLAG (i->locked, a))
    return 0;

  if (!IsPointOnArc (PosX, PosY, SearchRadius, a))
    return 0;
  *i->Arc = a;
  *i->Dummy = a;
  longjmp (i->env, 1);
  return 1;			/* never reached */
}


/*!
 * \brief Searches arc on the SearchLayer.
 */
static bool
SearchArcByLocation (int locked, LayerType ** Layer, ArcType ** Arc,
		     ArcType ** Dummy)
{
  struct arc_info info;

  info.Arc = Arc;
  info.Dummy = Dummy;
  info.locked = (locked & LOCKED_TYPE) ? 0 : LOCKFLAG;

  *Layer = SearchLayer;
  if (setjmp (info.env) == 0)
    {
      r_search (SearchLayer->arc_tree, &SearchBox, NULL, arc_callback, &info);
      return false;
    }
  return (true);
}

static int
text_callback (const BoxType * box, void *cl)
{
  TextType *text = (TextType *) box;
  struct ans_info *i = (struct ans_info *) cl;

  if (TEST_FLAG (i->locked, text))
    return 0;

  if (POINT_IN_BOX (PosX, PosY, &text->BoundingBox))
    {
      *i->ptr2 = *i->ptr3 = text;
      longjmp (i->env, 1);
    }
  return 0;
}

/*!
 * \brief Searches text on the SearchLayer.
 */
static bool
SearchTextByLocation (int locked, LayerType ** Layer, TextType ** Text,
		      TextType ** Dummy)
{
  struct ans_info info;

  *Layer = SearchLayer;
  info.ptr2 = (void **) Text;
  info.ptr3 = (void **) Dummy;
  info.locked = (locked & LOCKED_TYPE) ? 0 : LOCKFLAG;

  if (setjmp (info.env) == 0)
    {
      r_search (SearchLayer->text_tree, &SearchBox, NULL, text_callback,
		&info);
      return false;
    }
  return (true);
}

static int
polygon_callback (const BoxType * box, void *cl)
{
  PolygonType *polygon = (PolygonType *) box;
  struct ans_info *i = (struct ans_info *) cl;

  if (TEST_FLAG (i->locked, polygon))
    return 0;

  if (IsPointInPolygon (PosX, PosY, SearchRadius, polygon))
    {
      *i->ptr2 = *i->ptr3 = polygon;
      longjmp (i->env, 1);
    }
  return 0;
}


/*!
 * \brief Searches a polygon on the SearchLayer.
 */
static bool
SearchPolygonByLocation (int locked, LayerType ** Layer,
			 PolygonType ** Polygon, PolygonType ** Dummy)
{
  struct ans_info info;

  *Layer = SearchLayer;
  info.ptr2 = (void **) Polygon;
  info.ptr3 = (void **) Dummy;
  info.locked = (locked & LOCKED_TYPE) ? 0 : LOCKFLAG;

  if (setjmp (info.env) == 0)
    {
      r_search (SearchLayer->polygon_tree, &SearchBox, NULL, polygon_callback,
		&info);
      return false;
    }
  return (true);
}

static int
linepoint_callback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  struct line_info *i = (struct line_info *) cl;
  int ret_val = 0;
  double d;

  if (TEST_FLAG (i->locked, line))
    return 0;

  /* some stupid code to check both points */
  d = Distance (PosX, PosY, line->Point1.X, line->Point1.Y);
  if (d < i->least)
    {
      i->least = d;
      *i->Line = line;
      *i->Point = &line->Point1;
      ret_val = 1;
    }

  d = Distance (PosX, PosY, line->Point2.X, line->Point2.Y);
  if (d < i->least)
    {
      i->least = d;
      *i->Line = line;
      *i->Point = &line->Point2;
      ret_val = 1;
    }
  return ret_val;
}

/*!
 * \brief Searches a line-point on all the search layer.
 */
static bool
SearchLinePointByLocation (int locked, LayerType ** Layer,
			   LineType ** Line, PointType ** Point)
{
  struct line_info info;
  *Layer = SearchLayer;
  info.Line = Line;
  info.Point = Point;
  *Point = NULL;
  info.least = MAX_LINE_POINT_DISTANCE + SearchRadius;
  info.locked = (locked & LOCKED_TYPE) ? 0 : LOCKFLAG;
  if (r_search
      (SearchLayer->line_tree, &SearchBox, NULL, linepoint_callback, &info))
    return true;
  return false;
}

static int
arcpoint_callback (const BoxType * b, void *cl)
{
  ArcType *arc = (ArcType *) b;
  struct arc_info *i = (struct arc_info *) cl;
  int ret_val = 0;
  double d;

  if (TEST_FLAG (i->locked, arc))
    return 0;

  d = Distance (PosX, PosY, arc->Point1.X, arc->Point1.Y);
  if (d < i->least)
    {
      i->least = d;
      *i->Arc = arc;
      *i->Point = &arc->Point1;
      ret_val = 1;
    }

  d = Distance (PosX, PosY, arc->Point2.X, arc->Point2.Y);
  if (d < i->least)
    {
      i->least = d;
      *i->Arc = arc;
      *i->Point = &arc->Point2;
      ret_val = 1;
    }
  return ret_val;
}

/*!
 * \brief Searches an arc-point on all the search layer.
 */
static bool
SearchArcPointByLocation (int locked, LayerType **Layer,
                          ArcType **arc, PointType **Point)
{
  struct arc_info info;
  *Layer = SearchLayer;
  info.Arc = arc;
  info.Point = Point;
  *Point = NULL;
  info.least = MAX_ARC_POINT_DISTANCE + SearchRadius;
  info.locked = (locked & LOCKED_TYPE) ? 0 : LOCKFLAG;
  if (r_search
      (SearchLayer->arc_tree, &SearchBox, NULL, arcpoint_callback, &info))
    return true;
  return false;
}
/*!
 * \brief Searches a polygon-point on all layers that are switched on
 * in layerstack order.
 */
static bool
SearchPointByLocation (int locked, LayerType ** Layer,
		       PolygonType ** Polygon, PointType ** Point)
{
  double d, least;
  bool found = false;

  least = SearchRadius + MAX_POLYGON_POINT_DISTANCE;
  *Layer = SearchLayer;
  POLYGON_LOOP (*Layer);
  {
    POLYGONPOINT_LOOP (polygon);
    {
      d = Distance (point->X, point->Y, PosX, PosY);
      if (d < least)
	{
	  least = d;
	  *Polygon = polygon;
	  *Point = point;
	  found = true;
	}
    }
    END_LOOP;
  }
  END_LOOP;
  if (found)
    return (true);
  return (false);
}

static int
name_callback (const BoxType * box, void *cl)
{
  TextType *text = (TextType *) box;
  struct ans_info *i = (struct ans_info *) cl;
  ElementType *element = (ElementType *) text->Element;
  double newarea;

  if (TEST_FLAG (i->locked, text))
    return 0;

  if ((FRONT (element) || i->BackToo) && !TEST_FLAG (HIDENAMEFLAG, element) &&
      POINT_IN_BOX (PosX, PosY, &text->BoundingBox))
    {
      /* use the text with the smallest bounding box */
      newarea = (text->BoundingBox.X2 - text->BoundingBox.X1) *
	(double) (text->BoundingBox.Y2 - text->BoundingBox.Y1);
      if (newarea < i->area)
	{
	  i->area = newarea;
	  *i->ptr1 = element;
	  *i->ptr2 = *i->ptr3 = text;
	}
      return 1;
    }
  return 0;
}

/*!
 * \brief Searches the name of an element.
 *
 * The search starts with the last element and goes back to the
 * beginning.
 */
static bool
SearchElementNameByLocation (int locked, ElementType ** Element,
			     TextType ** Text, TextType ** Dummy,
			     bool BackToo)
{
  struct ans_info info;

  /* package layer have to be switched on */
  if (PCB->ElementOn)
    {
      info.ptr1 = (void **) Element;
      info.ptr2 = (void **) Text;
      info.ptr3 = (void **) Dummy;
      info.area = SQUARE (MAX_COORD);
      info.BackToo = (BackToo && PCB->InvisibleObjectsOn);
      info.locked = (locked & LOCKED_TYPE) ? 0 : LOCKFLAG;
      if (r_search (PCB->Data->name_tree[NAME_INDEX (PCB)], &SearchBox, NULL,
		    name_callback, &info))
	return true;
    }
  return (false);
}

static int
element_callback (const BoxType * box, void *cl)
{
  ElementType *element = (ElementType *) box;
  struct ans_info *i = (struct ans_info *) cl;
  double newarea;

  if (TEST_FLAG (i->locked, element))
    return 0;

  if ((FRONT (element) || i->BackToo) &&
      POINT_IN_BOX (PosX, PosY, &element->VBox))
    {
      /* use the element with the smallest bounding box */
      newarea = (element->VBox.X2 - element->VBox.X1) *
	(double) (element->VBox.Y2 - element->VBox.Y1);
      if (newarea < i->area)
	{
	  i->area = newarea;
	  *i->ptr1 = *i->ptr2 = *i->ptr3 = element;
	  return 1;
	}
    }
  return 0;
}

/*!
 * \brief Searches an element.
 *
 * The search starts with the last element and goes back to the
 * beginning.
 *
 * If more than one element matches, the smallest one is taken.
 */
static bool
SearchElementByLocation (int locked,
			 ElementType ** Element,
			 ElementType ** Dummy1, ElementType ** Dummy2,
			 bool BackToo)
{
  struct ans_info info;

  /* Both package layers have to be switched on */
  if (PCB->ElementOn && PCB->PinOn)
    {
      info.ptr1 = (void **) Element;
      info.ptr2 = (void **) Dummy1;
      info.ptr3 = (void **) Dummy2;
      info.area = SQUARE (MAX_COORD);
      info.BackToo = (BackToo && PCB->InvisibleObjectsOn);
      info.locked = (locked & LOCKED_TYPE) ? 0 : LOCKFLAG;
      if (r_search
	  (PCB->Data->element_tree, &SearchBox, NULL, element_callback,
	   &info))
	return true;
    }
  return false;
}

/*!
 * \brief Checks if a point is on a pin.
 */
bool
IsPointOnPin (Coord X, Coord Y, Coord Radius, PinType *pin)
{
  Coord t = PIN_SIZE (pin) / 2;
  if (TEST_FLAG (SQUAREFLAG, pin))
    {
      BoxType b;

      b.X1 = pin->X - t;
      b.X2 = pin->X + t;
      b.Y1 = pin->Y - t;
      b.Y2 = pin->Y + t;
      if (IsPointInBox (X, Y, &b, Radius))
	return true;
    }
  else if (Distance (pin->X, pin->Y, X, Y) <= Radius + t)
    return true;
  return false;
}

/*!
 * \brief Checks if a rat-line end is on a PV.
 */
bool
IsPointOnLineEnd (Coord X, Coord Y, RatType *Line)
{
  if (((X == Line->Point1.X) && (Y == Line->Point1.Y)) ||
      ((X == Line->Point2.X) && (Y == Line->Point2.Y)))
    return (true);
  return (false);
}

/*!
 * \brief Checks if a line intersects with a PV.
 *
 * <pre>
 * let the point be (X,Y) and the line (X1,Y1)(X2,Y2)
 * the length of the line is
 *
 *   L = ((X2-X1)^2 + (Y2-Y1)^2)^0.5
 * 
 * let Q be the point of perpendicular projection of (X,Y) onto the line
 *
 *   QX = X1 + D1*(X2-X1) / L
 *   QY = Y1 + D1*(Y2-Y1) / L
 * 
 * with (from vector geometry)
 *
 *        (Y1-Y)(Y1-Y2)+(X1-X)(X1-X2)
 *   D1 = ---------------------------
 *                     L
 *
 *   D1 < 0   Q is on backward extension of the line
 *   D1 > L   Q is on forward extension of the line
 *   else     Q is on the line
 *
 * the signed distance from (X,Y) to Q is
 *
 *        (Y2-Y1)(X-X1)-(X2-X1)(Y-Y1)
 *   D2 = ----------------------------
 *                     L
 *
 * Finally, D1 and D2 are orthogonal, so we can sum them easily
 * by pythagorean theorem.
 * </pre>
 */
bool
IsPointOnLine (Coord X, Coord Y, Coord Radius, LineType *Line)
{
  double D1, D2, L;

  /* Get length of segment */
  L = Distance (Line->Point1.X, Line->Point1.Y, Line->Point2.X, Line->Point2.Y);
  if (L < 0.1)
    return Distance (X, Y, Line->Point1.X, Line->Point1.Y) < Radius + Line->Thickness / 2;

  /* Get distance from (X1, Y1) to Q (on the line) */
  D1 = ((double) (Y - Line->Point1.Y) * (Line->Point2.Y - Line->Point1.Y)
        + (double) (X - Line->Point1.X) * (Line->Point2.X - Line->Point1.X)) / L;
  /* Translate this into distance to Q from segment */
  if (D1 < 0)       D1 = -D1;
  else if (D1 > L)  D1 -= L;
  else              D1 = 0;
  /* Get distance from (X, Y) to Q */
  D2 = ((double) (X - Line->Point1.X) * (Line->Point2.Y - Line->Point1.Y)
        - (double) (Y - Line->Point1.Y) * (Line->Point2.X - Line->Point1.X)) / L;
  /* Total distance is then the pythagorean sum of these */
  return hypot (D1, D2) <= Radius + Line->Thickness / 2;
}

static int
is_point_on_line (Coord px, Coord py, Coord lx1, Coord ly1, Coord lx2, Coord ly2)
{
  /* ohh well... let's hope the optimizer does something clever with inlining... */
  LineType l;
  l.Point1.X = lx1;
  l.Point1.Y = ly1;
  l.Point2.X = lx2;
  l.Point2.Y = ly2;
  l.Thickness = 1;
  return IsPointOnLine (px, py, 1, &l);
}

/*!
 * \brief Checks if a line crosses a rectangle.
 */
bool
IsLineInRectangle (Coord X1, Coord Y1, Coord X2, Coord Y2, LineType *Line)
{
  LineType line;

  /* first, see if point 1 is inside the rectangle */
  /* in case the whole line is inside the rectangle */
  if (X1 < Line->Point1.X && X2 > Line->Point1.X &&
      Y1 < Line->Point1.Y && Y2 > Line->Point1.Y)
    return (true);
  /* construct a set of dummy lines and check each of them */
  line.Thickness = 0;
  line.Flags = NoFlags ();

  /* upper-left to upper-right corner */
  line.Point1.Y = line.Point2.Y = Y1;
  line.Point1.X = X1;
  line.Point2.X = X2;
  if (LineLineIntersect (&line, Line))
    return (true);

  /* upper-right to lower-right corner */
  line.Point1.X = X2;
  line.Point1.Y = Y1;
  line.Point2.Y = Y2;
  if (LineLineIntersect (&line, Line))
    return (true);

  /* lower-right to lower-left corner */
  line.Point1.Y = Y2;
  line.Point1.X = X1;
  line.Point2.X = X2;
  if (LineLineIntersect (&line, Line))
    return (true);

  /* lower-left to upper-left corner */
  line.Point2.X = X1;
  line.Point1.Y = Y1;
  line.Point2.Y = Y2;
  if (LineLineIntersect (&line, Line))
    return (true);

  return (false);
}

/*!
 * \brief Checks if a point (of null radius) is in a slanted rectangle.
 */
static int
IsPointInQuadrangle(PointType p[4], PointType *l)
{
  Coord dx, dy, x, y;
  double prod0, prod1;

  dx = p[1].X - p[0].X;
  dy = p[1].Y - p[0].Y;
  x = l->X - p[0].X;
  y = l->Y - p[0].Y;
  prod0 = (double) x * dx + (double) y * dy;
  x = l->X - p[1].X;
  y = l->Y - p[1].Y;
  prod1 = (double) x * dx + (double) y * dy;
  if (prod0 * prod1 <= 0)
    {
      dx = p[1].X - p[2].X;
      dy = p[1].Y - p[2].Y;
      prod0 = (double) x * dx + (double) y * dy;
      x = l->X - p[2].X;
      y = l->Y - p[2].Y;
      prod1 = (double) x * dx + (double) y * dy;
      if (prod0 * prod1 <= 0)
	return true;
    }
  return false;
}

/*!
 * \brief Checks if a line crosses a quadrangle: almost copied from
 * IsLineInRectangle().
 *
 * \note Actually this quadrangle is a slanted rectangle.
 */
bool
IsLineInQuadrangle (PointType p[4], LineType *Line)
{
  LineType line;

  /* first, see if point 1 is inside the rectangle */
  /* in case the whole line is inside the rectangle */
  if (IsPointInQuadrangle(p,&(Line->Point1)))
    return true;
  if (IsPointInQuadrangle(p,&(Line->Point2)))
    return true;
  /* construct a set of dummy lines and check each of them */
  line.Thickness = 0;
  line.Flags = NoFlags ();

  /* upper-left to upper-right corner */
  line.Point1.X = p[0].X; line.Point1.Y = p[0].Y;
  line.Point2.X = p[1].X; line.Point2.Y = p[1].Y;
  if (LineLineIntersect (&line, Line))
    return (true);

  /* upper-right to lower-right corner */
  line.Point1.X = p[2].X; line.Point1.Y = p[2].Y;
  if (LineLineIntersect (&line, Line))
    return (true);

  /* lower-right to lower-left corner */
  line.Point2.X = p[3].X; line.Point2.Y = p[3].Y;
  if (LineLineIntersect (&line, Line))
    return (true);

  /* lower-left to upper-left corner */
  line.Point1.X = p[0].X; line.Point1.Y = p[0].Y;
  if (LineLineIntersect (&line, Line))
    return (true);

  return (false);
}
/*!
 * \brief Checks if an arc crosses a square.
 */
bool
IsArcInRectangle (Coord X1, Coord Y1, Coord X2, Coord Y2, ArcType *Arc)
{
  LineType line;

  /* construct a set of dummy lines and check each of them */
  line.Thickness = 0;
  line.Flags = NoFlags ();

  /* upper-left to upper-right corner */
  line.Point1.Y = line.Point2.Y = Y1;
  line.Point1.X = X1;
  line.Point2.X = X2;
  if (LineArcIntersect (&line, Arc))
    return (true);

  /* upper-right to lower-right corner */
  line.Point1.X = line.Point2.X = X2;
  line.Point1.Y = Y1;
  line.Point2.Y = Y2;
  if (LineArcIntersect (&line, Arc))
    return (true);

  /* lower-right to lower-left corner */
  line.Point1.Y = line.Point2.Y = Y2;
  line.Point1.X = X1;
  line.Point2.X = X2;
  if (LineArcIntersect (&line, Arc))
    return (true);

  /* lower-left to upper-left corner */
  line.Point1.X = line.Point2.X = X1;
  line.Point1.Y = Y1;
  line.Point2.Y = Y2;
  if (LineArcIntersect (&line, Arc))
    return (true);

  return (false);
}

/*!
 * \brief Check if a circle of Radius with center at (X, Y) intersects
 * a Pad.
 *
 * Written to enable arbitrary pad directions; for rounded pads, too.
 */
bool
IsPointInPad (Coord X, Coord Y, Coord Radius, PadType *Pad)
{
  double r, Sin, Cos;
  Coord x; 
  Coord t2 = (Pad->Thickness + 1) / 2, range;
  PadType pad = *Pad;

  /* series of transforms saving range */
  /* move Point1 to the origin */
  X -= pad.Point1.X;
  Y -= pad.Point1.Y;

  pad.Point2.X -= pad.Point1.X;
  pad.Point2.Y -= pad.Point1.Y;
  /* so, pad.Point1.X = pad.Point1.Y = 0; */

  /* rotate round (0, 0) so that Point2 coordinates be (r, 0) */
  r = Distance (0, 0, pad.Point2.X, pad.Point2.Y);
  if (r < .1)
    {
      Cos = 1;
      Sin = 0;
    }
  else
    {
      Sin = pad.Point2.Y / r;
      Cos = pad.Point2.X / r;
    }
  x = X;
  X = X * Cos + Y * Sin;
  Y = Y * Cos - x * Sin;
  /* now pad.Point2.X = r; pad.Point2.Y = 0; */

  /* take into account the ends */
  if (TEST_FLAG (SQUAREFLAG, Pad))
    {
      r += Pad->Thickness;
      X += t2;
    }
  if (Y < 0)
    Y = -Y;	/* range value is evident now*/

  if (TEST_FLAG (SQUAREFLAG, Pad))
    {
      if (X <= 0)
	{
	  if (Y <= t2)
            range = -X;
          else
	    return Radius > Distance (0, t2, X, Y);
	}
      else if (X >= r)
	{
	  if (Y <= t2)
            range = X - r;
          else 
	    return Radius > Distance (r, t2, X, Y);
	}
      else
	range = Y - t2;
    }
  else/*Rounded pad: even more simple*/
    {
      if (X <= 0)
	return (Radius + t2) > Distance (0, 0, X, Y);
      else if (X >= r) 
	return (Radius + t2) > Distance (r, 0, X, Y);
      else
	range = Y - t2;
    }
  return range < Radius;
}

/*!
 * \brief .
 *
 * \note Assumes box has point1 with numerically lower X and Y
 * coordinates.
 */
bool
IsPointInBox (Coord X, Coord Y, BoxType *box, Coord Radius)
{
  Coord width, height, range;

  /* Compute coordinates relative to Point1 */
  X -= box->X1;
  Y -= box->Y1;

  width =  box->X2 - box->X1;
  height = box->Y2 - box->Y1;

  if (X <= 0)
    {
      if (Y < 0)
        return Radius > Distance (0, 0, X, Y);
      else if (Y > height)
        return Radius > Distance (0, height, X, Y);
      else
        range = -X;
    }
  else if (X >= width)
    {
      if (Y < 0)
        return Radius > Distance (width, 0, X, Y);
      else if (Y > height)
        return Radius > Distance (width, height, X, Y);
      else
        range = X - width;
    }
  else
    {
      if (Y < 0)
        range = -Y;
      else if (Y > height)
        range = Y - height;
      else
        return true;
    }

  return range < Radius;
}

/*!
 * \brief .
 *
 * \todo This code is BROKEN in the case of non-circular arcs, and in
 * the case that the arc thickness is greater than the radius.
 */
bool
IsPointOnArc (Coord X, Coord Y, Coord Radius, ArcType *Arc)
{
  /* Calculate angle of point from arc center */
  double p_dist = Distance (X, Y, Arc->X, Arc->Y);
  double p_cos = (X - Arc->X) / p_dist;
  Angle p_ang = acos (p_cos) * RAD_TO_DEG;
  Angle ang1, ang2;

  /* Convert StartAngle, Delta into bounding angles in [0, 720) */
  if (Arc->Delta > 0)
    {
      ang1 = NormalizeAngle (Arc->StartAngle);
      ang2 = NormalizeAngle (Arc->StartAngle + Arc->Delta);
    }
  else
    {
      ang1 = NormalizeAngle (Arc->StartAngle + Arc->Delta);
      ang2 = NormalizeAngle (Arc->StartAngle);
    }
  if (ang1 > ang2)
    ang2 += 360;
  /* Make sure full circles aren't treated as zero-length arcs */
  if (Arc->Delta == 360 || Arc->Delta == -360)
    ang2 = ang1 + 360;

  if (Y > Arc->Y)
    p_ang = -p_ang;
  p_ang += 180;

  /* Check point is outside arc range, check distance from endpoints */
  if (ang1 >= p_ang || ang2 <= p_ang)
    {
      Coord ArcX, ArcY;

      ArcX = Arc->X + Arc->Width *
              cos ((Arc->StartAngle + 180) / RAD_TO_DEG);
      ArcY = Arc->Y - Arc->Width *
              sin ((Arc->StartAngle + 180) / RAD_TO_DEG);
      if (Distance (X, Y, ArcX, ArcY) < Radius + Arc->Thickness / 2)
        return true;

      ArcX = Arc->X + Arc->Width *
              cos ((Arc->StartAngle + Arc->Delta + 180) / RAD_TO_DEG);
      ArcY = Arc->Y - Arc->Width *
              sin ((Arc->StartAngle + Arc->Delta + 180) / RAD_TO_DEG);
      if (Distance (X, Y, ArcX, ArcY) < Radius + Arc->Thickness / 2)
        return true;
      return false;
    }
  /* If point is inside the arc range, just compare it to the arc */
  return fabs (Distance (X, Y, Arc->X, Arc->Y) - Arc->Width) < Radius + Arc->Thickness / 2;
}

/*!
 * \brief Searches for any kind of object or for a set of object types
 * the calling routine passes two pointers to allocated memory for
 * storing the results.
 *
 * A type value is returned too which is NO_TYPE if no objects has been
 * found.
 *
 * A set of object types is passed in.
 *
 * The object is located by it's position.
 *
 * The layout is checked in the following order:
 *   polygon-point, pin, via, line, text, elementname, polygon, element
 *
 * \note That if Type includes LOCKED_TYPE, then the search includes
 * locked items. Otherwise, locked items are ignored.
 */
int
SearchObjectByLocation (unsigned Type,
			void **Result1, void **Result2, void **Result3,
			Coord X, Coord Y, Coord Radius)
{
  void *r1, *r2, *r3;
  void **pr1 = &r1, **pr2 = &r2, **pr3 = &r3;
  int i;
  double HigherBound = 0;
  int HigherAvail = NO_TYPE;
  int locked = Type & LOCKED_TYPE;
  /* setup variables used by local functions */
  PosX = X;
  PosY = Y;
  SearchRadius = Radius;
  if (Radius)
    {
      SearchBox.X1 = X - Radius;
      SearchBox.Y1 = Y - Radius;
      SearchBox.X2 = X + Radius;
      SearchBox.Y2 = Y + Radius;
    }
  else
    {
      SearchBox = point_box (X, Y);
    }

  if (TEST_FLAG (LOCKNAMESFLAG, PCB))
    {
      Type &= ~ (ELEMENTNAME_TYPE | TEXT_TYPE);
    }
  if (TEST_FLAG (HIDENAMESFLAG, PCB))
    {
      Type &= ~ELEMENTNAME_TYPE;
    }
  if (TEST_FLAG (ONLYNAMESFLAG, PCB))
    {
      Type &= (ELEMENTNAME_TYPE | TEXT_TYPE);
    }
  if (TEST_FLAG (THINDRAWFLAG, PCB) || TEST_FLAG (THINDRAWPOLYFLAG, PCB))
    {
      Type &= ~POLYGON_TYPE;
    }

  if (Type & RATLINE_TYPE && PCB->RatOn &&
      SearchRatLineByLocation (locked,
			       (RatType **) Result1,
			       (RatType **) Result2,
			       (RatType **) Result3))
    return (RATLINE_TYPE);

  if (Type & VIA_TYPE &&
      SearchViaByLocation (locked,
			   (PinType **) Result1,
			   (PinType **) Result2, (PinType **) Result3))
    return (VIA_TYPE);

  if (Type & PIN_TYPE &&
      SearchPinByLocation (locked,
			   (ElementType **) pr1,
			   (PinType **) pr2, (PinType **) pr3))
    HigherAvail = PIN_TYPE;

  if (!HigherAvail && Type & PAD_TYPE &&
      SearchPadByLocation (locked,
			   (ElementType **) pr1,
			   (PadType **) pr2, (PadType **) pr3, false))
    HigherAvail = PAD_TYPE;

  if (!HigherAvail && Type & ELEMENTNAME_TYPE &&
      SearchElementNameByLocation (locked,
				   (ElementType **) pr1,
				   (TextType **) pr2, (TextType **) pr3,
				   false))
    {
      BoxType *box = &((TextType *) r2)->BoundingBox;
      HigherBound = (double) (box->X2 - box->X1) * (double) (box->Y2 - box->Y1);
      HigherAvail = ELEMENTNAME_TYPE;
    }

  if (!HigherAvail && Type & ELEMENT_TYPE &&
      SearchElementByLocation (locked,
			       (ElementType **) pr1,
			       (ElementType **) pr2,
			       (ElementType **) pr3, false))
    {
      BoxType *box = &((ElementType *) r1)->BoundingBox;
      HigherBound = (double) (box->X2 - box->X1) * (double) (box->Y2 - box->Y1);
      HigherAvail = ELEMENT_TYPE;
    }

  for (i = -1; i < max_copper_layer + 1; i++)
    {
      if (i < 0)
	SearchLayer = &PCB->Data->SILKLAYER;
      else if (i < max_copper_layer)
	SearchLayer = LAYER_ON_STACK (i);
      else
	{
	  SearchLayer = &PCB->Data->BACKSILKLAYER;
	  if (!PCB->InvisibleObjectsOn)
	    continue;
	}
      if (SearchLayer->On)
	{
	  if ((HigherAvail & (PIN_TYPE | PAD_TYPE)) == 0 &&
	      Type & POLYGONPOINT_TYPE &&
	      SearchPointByLocation (locked,
				     (LayerType **) Result1,
				     (PolygonType **) Result2,
				     (PointType **) Result3))
	    return (POLYGONPOINT_TYPE);

	  if ((HigherAvail & (PIN_TYPE | PAD_TYPE)) == 0 &&
	      Type & LINEPOINT_TYPE &&
	      SearchLinePointByLocation (locked,
					 (LayerType **) Result1,
					 (LineType **) Result2,
					 (PointType **) Result3))
	    return (LINEPOINT_TYPE);

	  if ((HigherAvail & (PIN_TYPE | PAD_TYPE)) == 0 && Type & LINE_TYPE
	      && SearchLineByLocation (locked,
				       (LayerType **) Result1,
				       (LineType **) Result2,
				       (LineType **) Result3))
	    return (LINE_TYPE);

    if ((HigherAvail & (PIN_TYPE | PAD_TYPE)) == 0 &&
	      Type & ARCPOINT_TYPE &&
	      SearchArcPointByLocation (locked,
					(LayerType **) Result1,
					(ArcType **) Result2,
					(PointType **) Result3))
	    return (ARCPOINT_TYPE);

	  if ((HigherAvail & (PIN_TYPE | PAD_TYPE)) == 0 && Type & ARC_TYPE &&
	      SearchArcByLocation (locked,
				   (LayerType **) Result1,
				   (ArcType **) Result2,
				   (ArcType **) Result3))
	    return (ARC_TYPE);

	  if ((HigherAvail & (PIN_TYPE | PAD_TYPE)) == 0 && Type & TEXT_TYPE
	      && SearchTextByLocation (locked,
				       (LayerType **) Result1,
				       (TextType **) Result2,
				       (TextType **) Result3))
	    return (TEXT_TYPE);

	  if (Type & POLYGON_TYPE &&
	      SearchPolygonByLocation (locked,
				       (LayerType **) Result1,
				       (PolygonType **) Result2,
				       (PolygonType **) Result3))
	    {
	      if (HigherAvail)
		{
		  BoxType *box =
		    &(*(PolygonType **) Result2)->BoundingBox;
		  double area =
		    (double) (box->X2 - box->X1) * (double) (box->X2 - box->X1);
		  if (HigherBound < area)
		    break;
		  else
		    return (POLYGON_TYPE);
		}
	      else
		return (POLYGON_TYPE);
	    }
	}
    }
  /* return any previously found objects */
  if (HigherAvail & PIN_TYPE)
    {
      *Result1 = r1;
      *Result2 = r2;
      *Result3 = r3;
      return (PIN_TYPE);
    }

  if (HigherAvail & PAD_TYPE)
    {
      *Result1 = r1;
      *Result2 = r2;
      *Result3 = r3;
      return (PAD_TYPE);
    }

  if (HigherAvail & ELEMENTNAME_TYPE)
    {
      *Result1 = r1;
      *Result2 = r2;
      *Result3 = r3;
      return (ELEMENTNAME_TYPE);
    }

  if (HigherAvail & ELEMENT_TYPE)
    {
      *Result1 = r1;
      *Result2 = r2;
      *Result3 = r3;
      return (ELEMENT_TYPE);
    }

  /* search the 'invisible objects' last */
  if (!PCB->InvisibleObjectsOn)
    return (NO_TYPE);

  if (Type & PAD_TYPE &&
      SearchPadByLocation (locked,
			   (ElementType **) Result1,
			   (PadType **) Result2, (PadType **) Result3,
			   true))
    return (PAD_TYPE);

  if (Type & ELEMENTNAME_TYPE &&
      SearchElementNameByLocation (locked,
				   (ElementType **) Result1,
				   (TextType **) Result2,
				   (TextType **) Result3, true))
    return (ELEMENTNAME_TYPE);

  if (Type & ELEMENT_TYPE &&
      SearchElementByLocation (locked,
			       (ElementType **) Result1,
			       (ElementType **) Result2,
			       (ElementType **) Result3, true))
    return (ELEMENT_TYPE);

  return (NO_TYPE);
}

/*!
 * \brief Searches for a object by it's unique ID.
 *
 * It doesn't matter if the object is visible or not.
 *
 * The search is performed on a PCB, a buffer or on the remove list.
 *
 * The calling routine passes two pointers to allocated memory for
 * storing the results.
 *
 * \return A type value is returned too which is NO_TYPE if no objects
 * has been found.
 */
int
SearchObjectByID (DataType *Base,
		  void **Result1, void **Result2, void **Result3, int ID,
		  int type)
{
  if (type == LINE_TYPE || type == LINEPOINT_TYPE)
    {
      ALLLINE_LOOP (Base);
      {
	if (line->ID == ID)
	  {
	    *Result1 = (void *) layer;
	    *Result2 = *Result3 = (void *) line;
	    return (LINE_TYPE);
	  }
	if (line->Point1.ID == ID)
	  {
	    *Result1 = (void *) layer;
	    *Result2 = (void *) line;
	    *Result3 = (void *) &line->Point1;
	    return (LINEPOINT_TYPE);
	  }
	if (line->Point2.ID == ID)
	  {
	    *Result1 = (void *) layer;
	    *Result2 = (void *) line;
	    *Result3 = (void *) &line->Point2;
	    return (LINEPOINT_TYPE);
	  }
      }
      ENDALL_LOOP;
    }
  if (type == ARC_TYPE)
    {
      ALLARC_LOOP (Base);
      {
	if (arc->ID == ID)
	  {
	    *Result1 = (void *) layer;
	    *Result2 = *Result3 = (void *) arc;
	    return (ARC_TYPE);
	  }
      }
      ENDALL_LOOP;
    }

  if (type == TEXT_TYPE)
    {
      ALLTEXT_LOOP (Base);
      {
	if (text->ID == ID)
	  {
	    *Result1 = (void *) layer;
	    *Result2 = *Result3 = (void *) text;
	    return (TEXT_TYPE);
	  }
      }
      ENDALL_LOOP;
    }

  if (type == POLYGON_TYPE || type == POLYGONPOINT_TYPE)
    {
      ALLPOLYGON_LOOP (Base);
      {
	if (polygon->ID == ID)
	  {
	    *Result1 = (void *) layer;
	    *Result2 = *Result3 = (void *) polygon;
	    return (POLYGON_TYPE);
	  }
	if (type == POLYGONPOINT_TYPE)
	  POLYGONPOINT_LOOP (polygon);
	{
	  if (point->ID == ID)
	    {
	      *Result1 = (void *) layer;
	      *Result2 = (void *) polygon;
	      *Result3 = (void *) point;
	      return (POLYGONPOINT_TYPE);
	    }
	}
	END_LOOP;
      }
      ENDALL_LOOP;
    }
  if (type == VIA_TYPE)
    {
      VIA_LOOP (Base);
      {
	if (via->ID == ID)
	  {
	    *Result1 = *Result2 = *Result3 = (void *) via;
	    return (VIA_TYPE);
	  }
      }
      END_LOOP;
    }

  if (type == RATLINE_TYPE || type == LINEPOINT_TYPE)
    {
      RAT_LOOP (Base);
      {
	if (line->ID == ID)
	  {
	    *Result1 = *Result2 = *Result3 = (void *) line;
	    return (RATLINE_TYPE);
	  }
	if (line->Point1.ID == ID)
	  {
	    *Result1 = (void *) NULL;
	    *Result2 = (void *) line;
	    *Result3 = (void *) &line->Point1;
	    return (LINEPOINT_TYPE);
	  }
	if (line->Point2.ID == ID)
	  {
	    *Result1 = (void *) NULL;
	    *Result2 = (void *) line;
	    *Result3 = (void *) &line->Point2;
	    return (LINEPOINT_TYPE);
	  }
      }
      END_LOOP;
    }

  if (type == ELEMENT_TYPE || type == PAD_TYPE || type == PIN_TYPE
      || type == ELEMENTLINE_TYPE || type == ELEMENTNAME_TYPE
      || type == ELEMENTARC_TYPE)
    /* check pins and elementnames too */
    ELEMENT_LOOP (Base);
  {
    if (element->ID == ID)
      {
	*Result1 = *Result2 = *Result3 = (void *) element;
	return (ELEMENT_TYPE);
      }
    if (type == ELEMENTLINE_TYPE)
      ELEMENTLINE_LOOP (element);
    {
      if (line->ID == ID)
	{
	  *Result1 = (void *) element;
	  *Result2 = *Result3 = (void *) line;
	  return (ELEMENTLINE_TYPE);
	}
    }
    END_LOOP;
    if (type == ELEMENTARC_TYPE)
      ARC_LOOP (element);
    {
      if (arc->ID == ID)
	{
	  *Result1 = (void *) element;
	  *Result2 = *Result3 = (void *) arc;
	  return (ELEMENTARC_TYPE);
	}
    }
    END_LOOP;
    if (type == ELEMENTNAME_TYPE)
      ELEMENTTEXT_LOOP (element);
    {
      if (text->ID == ID)
	{
	  *Result1 = (void *) element;
	  *Result2 = *Result3 = (void *) text;
	  return (ELEMENTNAME_TYPE);
	}
    }
    END_LOOP;
    if (type == PIN_TYPE)
      PIN_LOOP (element);
    {
      if (pin->ID == ID)
	{
	  *Result1 = (void *) element;
	  *Result2 = *Result3 = (void *) pin;
	  return (PIN_TYPE);
	}
    }
    END_LOOP;
    if (type == PAD_TYPE)
      PAD_LOOP (element);
    {
      if (pad->ID == ID)
	{
	  *Result1 = (void *) element;
	  *Result2 = *Result3 = (void *) pad;
	  return (PAD_TYPE);
	}
    }
    END_LOOP;
  }
  END_LOOP;

  Message ("hace: Internal error, search for ID %d failed\n", ID);
  return (NO_TYPE);
}

/*!
 * \brief Searches for an element by its board name.
 *
 * \return The function returns a pointer to the element, NULL if not
 * found.
 */
ElementType *
SearchElementByName (DataType *Base, char *Name)
{
  ElementType *result = NULL;

  ELEMENT_LOOP (Base);
  {
    if (element->Name[1].TextString &&
	NSTRCMP (element->Name[1].TextString, Name) == 0)
      {
	result = element;
	return (result);
      }
  }
  END_LOOP;
  return result;
}

/*!
 * \brief Searches for an layer by its board name.
 *
 * \return The function returns an index of the layer, -1 if not
 * found.
 */
int
SearchLayerByName (DataType *Base, char *Name)
{
  int result = 0;

  LAYER_LOOP (Base, max_copper_layer);
  {
    if (layer->Name &&
	NSTRCMP (layer->Name, Name) == 0)
      {
	return result;
      }
    else
      result++;
  }
  END_LOOP;
  return -1;
}

/*!
 * \brief Searches the cursor position for the type.
 */
int
SearchScreen (Coord X, Coord Y, int Type, void **Result1,
	      void **Result2, void **Result3)
{
  int ans;

  ans = SearchObjectByLocation (Type, Result1, Result2, Result3,
				X, Y, SLOP * pixel_slop);
  return (ans);
}

int lines_intersect(Coord ax1, Coord ay1, Coord ax2, Coord ay2, Coord bx1, Coord by1, Coord bx2, Coord by2)
{
/* TODO: this should be long double if Coord is 64 bits */
	double ua, ub, xi, yi, X1, Y1, X2, Y2, X3, Y3, X4, Y4, tmp;

	int is_a_pt, is_b_pt;
 
	/* degenerate cases: a line is actually a point */
	is_a_pt = (ax1 == ax2) && (ay1 == ay2);
	is_b_pt = (bx1 == bx2) && (by1 == by2);

	if (is_a_pt && is_b_pt)
		return (ax1 == bx1) && (ay1 == by1);

	if (is_a_pt)
		return is_point_on_line(ax1, ay1,  bx1, by1, bx2, by2);
	if (is_b_pt)
		return is_point_on_line(bx1, by1,  ax1, ay1, ax2, ay2);

	/* maths from http://local.wasp.uwa.edu.au/~pbourke/geometry/lineline2d/ */

	X1 = ax1;
	X2 = ax2;
	X3 = bx1;
	X4 = bx2;
	Y1 = ay1;
	Y2 = ay2;
	Y3 = by1;
	Y4 = by2;

	tmp = ((Y4-Y3)*(X2-X1) - (X4-X3)*(Y2-Y1));
	ua=((X4-X3)*(Y1-Y3) - (Y4-Y3)*(X1-X3)) / tmp;
	ub=((X2-X1)*(Y1-Y3) - (Y2-Y1)*(X1-X3)) / tmp;
	xi = X1 + ua * (X2 - X1);
	yi = Y1 + ua * (Y2 - Y1);

#define check(e1, e2, i) \
	if (e1 < e2) { \
		if ((i < e1) || (i > e2)) \
			return 0; \
	} \
	else { \
		if ((i > e1) || (i < e2)) \
			return 0; \
	}

	check(ax1, ax2, xi);
	check(bx1, bx2, xi);
	check(ay1, ay2, yi);
	check(by1, by2, yi);
	return 1;
}
