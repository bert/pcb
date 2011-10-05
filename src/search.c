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


/* search routines
 * some of the functions use dummy parameters
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

RCSID ("$Id$");


/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static double PosX, PosY;		/* search position for subroutines */
static Coord SearchRadius;
static BoxType SearchBox;
static LayerTypePtr SearchLayer;

/* ---------------------------------------------------------------------------
 * some local prototypes.  The first parameter includes LOCKED_TYPE if we
 * want to include locked types in the search.
 */
static bool SearchLineByLocation (int, LayerTypePtr *, LineTypePtr *,
				     LineTypePtr *);
static bool SearchArcByLocation (int, LayerTypePtr *, ArcTypePtr *,
				    ArcTypePtr *);
static bool SearchRatLineByLocation (int, RatTypePtr *, RatTypePtr *,
					RatTypePtr *);
static bool SearchTextByLocation (int, LayerTypePtr *, TextTypePtr *,
				     TextTypePtr *);
static bool SearchPolygonByLocation (int, LayerTypePtr *, PolygonTypePtr *,
					PolygonTypePtr *);
static bool SearchPinByLocation (int, ElementTypePtr *, PinTypePtr *,
				    PinTypePtr *);
static bool SearchPadByLocation (int, ElementTypePtr *, PadTypePtr *,
				    PadTypePtr *, bool);
static bool SearchViaByLocation (int, PinTypePtr *, PinTypePtr *,
				    PinTypePtr *);
static bool SearchElementNameByLocation (int, ElementTypePtr *,
					    TextTypePtr *, TextTypePtr *,
					    bool);
static bool SearchLinePointByLocation (int, LayerTypePtr *, LineTypePtr *,
					  PointTypePtr *);
static bool SearchPointByLocation (int, LayerTypePtr *, PolygonTypePtr *,
				      PointTypePtr *);
static bool SearchElementByLocation (int, ElementTypePtr *,
					ElementTypePtr *, ElementTypePtr *,
					bool);

/* ---------------------------------------------------------------------------
 * searches a via
 */
struct ans_info
{
  void **ptr1, **ptr2, **ptr3;
  bool BackToo;
  double area;
  jmp_buf env;
  int locked;			/* This will be zero or LOCKFLAG */
};

static int
pinorvia_callback (const BoxType * box, void *cl)
{
  struct ans_info *i = (struct ans_info *) cl;
  PinTypePtr pin = (PinTypePtr) box;
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

static bool
SearchViaByLocation (int locked, PinTypePtr * Via, PinTypePtr * Dummy1,
		     PinTypePtr * Dummy2)
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

/* ---------------------------------------------------------------------------
 * searches a pin
 * starts with the newest element
 */
static bool
SearchPinByLocation (int locked, ElementTypePtr * Element, PinTypePtr * Pin,
		     PinTypePtr * Dummy)
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
  PadTypePtr pad = (PadTypePtr) b;
  struct ans_info *i = (struct ans_info *) cl;
  AnyObjectType *ptr1 = pad->Element;

  if (TEST_FLAG (i->locked, ptr1))
    return 0;

  if (FRONT (pad) || i->BackToo)
    {
      if (IsPointInPad (PosX, PosY, SearchRadius, pad))
	    {
	      *i->ptr1 = ptr1;
	      *i->ptr2 = *i->ptr3 = pad;
	      longjmp (i->env, 1);
	    }
	}
  return 0;
}

/* ---------------------------------------------------------------------------
 * searches a pad
 * starts with the newest element
 */
static bool
SearchPadByLocation (int locked, ElementTypePtr * Element, PadTypePtr * Pad,
		     PadTypePtr * Dummy, bool BackToo)
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
  if (setjmp (info.env) == 0)
    r_search (PCB->Data->pad_tree, &SearchBox, NULL, pad_callback, &info);
  else
    return true;
  return false;
}

/* ---------------------------------------------------------------------------
 * searches ordinary line on the SearchLayer 
 */

struct line_info
{
  LineTypePtr *Line;
  PointTypePtr *Point;
  double least;
  jmp_buf env;
  int locked;
};

static int
line_callback (const BoxType * box, void *cl)
{
  struct line_info *i = (struct line_info *) cl;
  LineTypePtr l = (LineTypePtr) box;

  if (TEST_FLAG (i->locked, l))
    return 0;

  if (!IsPointInPad (PosX, PosY, SearchRadius, (PadTypePtr)l))
    return 0;
  *i->Line = l;
  *i->Point = (PointTypePtr) l;
  longjmp (i->env, 1);
  return 1;			/* never reached */
}


static bool
SearchLineByLocation (int locked, LayerTypePtr * Layer, LineTypePtr * Line,
		      LineTypePtr * Dummy)
{
  struct line_info info;

  info.Line = Line;
  info.Point = (PointTypePtr *) Dummy;
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
  LineTypePtr line = (LineTypePtr) box;
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

/* ---------------------------------------------------------------------------
 * searches rat lines if they are visible
 */
static bool
SearchRatLineByLocation (int locked, RatTypePtr * Line, RatTypePtr * Dummy1,
			 RatTypePtr * Dummy2)
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

/* ---------------------------------------------------------------------------
 * searches arc on the SearchLayer 
 */
struct arc_info
{
  ArcTypePtr *Arc, *Dummy;
  PointTypePtr *Point;
  double least;
  jmp_buf env;
  int locked;
};

static int
arc_callback (const BoxType * box, void *cl)
{
  struct arc_info *i = (struct arc_info *) cl;
  ArcTypePtr a = (ArcTypePtr) box;

  if (TEST_FLAG (i->locked, a))
    return 0;

  if (!IsPointOnArc (PosX, PosY, SearchRadius, a))
    return 0;
  *i->Arc = a;
  *i->Dummy = a;
  longjmp (i->env, 1);
  return 1;			/* never reached */
}


static bool
SearchArcByLocation (int locked, LayerTypePtr * Layer, ArcTypePtr * Arc,
		     ArcTypePtr * Dummy)
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
  TextTypePtr text = (TextTypePtr) box;
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

/* ---------------------------------------------------------------------------
 * searches text on the SearchLayer
 */
static bool
SearchTextByLocation (int locked, LayerTypePtr * Layer, TextTypePtr * Text,
		      TextTypePtr * Dummy)
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
  PolygonTypePtr polygon = (PolygonTypePtr) box;
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


/* ---------------------------------------------------------------------------
 * searches a polygon on the SearchLayer 
 */
static bool
SearchPolygonByLocation (int locked, LayerTypePtr * Layer,
			 PolygonTypePtr * Polygon, PolygonTypePtr * Dummy)
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
  LineTypePtr line = (LineTypePtr) b;
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

/* ---------------------------------------------------------------------------
 * searches a line-point on all the search layer
 */
static bool
SearchLinePointByLocation (int locked, LayerTypePtr * Layer,
			   LineTypePtr * Line, PointTypePtr * Point)
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
  ArcTypePtr arc = (ArcTypePtr) b;
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

/* ---------------------------------------------------------------------------
 * searches an arc-point on all the search layer
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
/* ---------------------------------------------------------------------------
 * searches a polygon-point on all layers that are switched on
 * in layerstack order
 */
static bool
SearchPointByLocation (int locked, LayerTypePtr * Layer,
		       PolygonTypePtr * Polygon, PointTypePtr * Point)
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
  TextTypePtr text = (TextTypePtr) box;
  struct ans_info *i = (struct ans_info *) cl;
  ElementTypePtr element = (ElementTypePtr) text->Element;
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

/* ---------------------------------------------------------------------------
 * searches the name of an element
 * the search starts with the last element and goes back to the beginning
 */
static bool
SearchElementNameByLocation (int locked, ElementTypePtr * Element,
			     TextTypePtr * Text, TextTypePtr * Dummy,
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
  ElementTypePtr element = (ElementTypePtr) box;
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

/* ---------------------------------------------------------------------------
 * searches an element
 * the search starts with the last element and goes back to the beginning
 * if more than one element matches, the smallest one is taken
 */
static bool
SearchElementByLocation (int locked,
			 ElementTypePtr * Element,
			 ElementTypePtr * Dummy1, ElementTypePtr * Dummy2,
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

/* ---------------------------------------------------------------------------
 * checks if a point is on a pin
 */
bool
IsPointOnPin (Coord X, Coord Y, Coord Radius, PinTypePtr pin)
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

/* ---------------------------------------------------------------------------
 * checks if a rat-line end is on a PV
 */
bool
IsPointOnLineEnd (Coord X, Coord Y, RatTypePtr Line)
{
  if (((X == Line->Point1.X) && (Y == Line->Point1.Y)) ||
      ((X == Line->Point2.X) && (Y == Line->Point2.Y)))
    return (true);
  return (false);
}

/* ---------------------------------------------------------------------------
 * checks if a line intersects with a PV
 *
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
 */
bool
IsPointOnLine (Coord X, Coord Y, Coord Radius, LineTypePtr Line)
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
  return sqrt (D1*D1 + D2*D2) <= Radius + Line->Thickness / 2;
}

/* ---------------------------------------------------------------------------
 * checks if a line crosses a rectangle
 */
bool
IsLineInRectangle (Coord X1, Coord Y1, Coord X2, Coord Y2, LineTypePtr Line)
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

static int /*checks if a point (of null radius) is in a slanted rectangle*/
IsPointInQuadrangle(PointType p[4], PointTypePtr l)
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
/* ---------------------------------------------------------------------------
 * checks if a line crosses a quadrangle: almost copied from IsLineInRectangle()
 * Note: actually this quadrangle is a slanted rectangle
 */
bool
IsLineInQuadrangle (PointType p[4], LineTypePtr Line)
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
/* ---------------------------------------------------------------------------
 * checks if an arc crosses a square
 */
bool
IsArcInRectangle (Coord X1, Coord Y1, Coord X2, Coord Y2, ArcTypePtr Arc)
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

/* ---------------------------------------------------------------------------
 * Check if a circle of Radius with center at (X, Y) intersects a Pad.
 * Written to enable arbitrary pad directions; for rounded pads, too.
 */
bool
IsPointInPad (Coord X, Coord Y, Coord Radius, PadTypePtr Pad)
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

bool
IsPointInBox (Coord X, Coord Y, BoxTypePtr box, Coord Radius)
{
  Coord width, height, range;

  /* NB: Assumes box has point1 with numerically lower X and Y coordinates */

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

/* TODO: this code is BROKEN in the case of non-circular arcs,
 *       and in the case that the arc thickness is greater than
 *       the radius.
 */
bool
IsPointOnArc (Coord X, Coord Y, Coord Radius, ArcTypePtr Arc)
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

/* ---------------------------------------------------------------------------
 * searches for any kind of object or for a set of object types
 * the calling routine passes two pointers to allocated memory for storing
 * the results. 
 * A type value is returned too which is NO_TYPE if no objects has been found.
 * A set of object types is passed in.
 * The object is located by it's position.
 *
 * The layout is checked in the following order:
 *   polygon-point, pin, via, line, text, elementname, polygon, element
 *
 * Note that if Type includes LOCKED_TYPE, then the search includes
 * locked items.  Otherwise, locked items are ignored.
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
			       (RatTypePtr *) Result1,
			       (RatTypePtr *) Result2,
			       (RatTypePtr *) Result3))
    return (RATLINE_TYPE);

  if (Type & VIA_TYPE &&
      SearchViaByLocation (locked,
			   (PinTypePtr *) Result1,
			   (PinTypePtr *) Result2, (PinTypePtr *) Result3))
    return (VIA_TYPE);

  if (Type & PIN_TYPE &&
      SearchPinByLocation (locked,
			   (ElementTypePtr *) pr1,
			   (PinTypePtr *) pr2, (PinTypePtr *) pr3))
    HigherAvail = PIN_TYPE;

  if (!HigherAvail && Type & PAD_TYPE &&
      SearchPadByLocation (locked,
			   (ElementTypePtr *) pr1,
			   (PadTypePtr *) pr2, (PadTypePtr *) pr3, false))
    HigherAvail = PAD_TYPE;

  if (!HigherAvail && Type & ELEMENTNAME_TYPE &&
      SearchElementNameByLocation (locked,
				   (ElementTypePtr *) pr1,
				   (TextTypePtr *) pr2, (TextTypePtr *) pr3,
				   false))
    {
      BoxTypePtr box = &((TextTypePtr) r2)->BoundingBox;
      HigherBound = (double) (box->X2 - box->X1) * (double) (box->Y2 - box->Y1);
      HigherAvail = ELEMENTNAME_TYPE;
    }

  if (!HigherAvail && Type & ELEMENT_TYPE &&
      SearchElementByLocation (locked,
			       (ElementTypePtr *) pr1,
			       (ElementTypePtr *) pr2,
			       (ElementTypePtr *) pr3, false))
    {
      BoxTypePtr box = &((ElementTypePtr) r1)->BoundingBox;
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
				     (LayerTypePtr *) Result1,
				     (PolygonTypePtr *) Result2,
				     (PointTypePtr *) Result3))
	    return (POLYGONPOINT_TYPE);

	  if ((HigherAvail & (PIN_TYPE | PAD_TYPE)) == 0 &&
	      Type & LINEPOINT_TYPE &&
	      SearchLinePointByLocation (locked,
					 (LayerTypePtr *) Result1,
					 (LineTypePtr *) Result2,
					 (PointTypePtr *) Result3))
	    return (LINEPOINT_TYPE);

	  if ((HigherAvail & (PIN_TYPE | PAD_TYPE)) == 0 && Type & LINE_TYPE
	      && SearchLineByLocation (locked,
				       (LayerTypePtr *) Result1,
				       (LineTypePtr *) Result2,
				       (LineTypePtr *) Result3))
	    return (LINE_TYPE);

	    if ((HigherAvail & (PIN_TYPE | PAD_TYPE)) == 0 &&
	      Type & ARCPOINT_TYPE &&
	      SearchArcPointByLocation (locked,
					(LayerTypePtr *) Result1,
					(ArcTypePtr *) Result2,
					(PointTypePtr *) Result3))
	    return (ARCPOINT_TYPE);

	  if ((HigherAvail & (PIN_TYPE | PAD_TYPE)) == 0 && Type & ARC_TYPE &&
	      SearchArcByLocation (locked,
				   (LayerTypePtr *) Result1,
				   (ArcTypePtr *) Result2,
				   (ArcTypePtr *) Result3))
	    return (ARC_TYPE);

	  if ((HigherAvail & (PIN_TYPE | PAD_TYPE)) == 0 && Type & TEXT_TYPE
	      && SearchTextByLocation (locked,
				       (LayerTypePtr *) Result1,
				       (TextTypePtr *) Result2,
				       (TextTypePtr *) Result3))
	    return (TEXT_TYPE);

	  if (Type & POLYGON_TYPE &&
	      SearchPolygonByLocation (locked,
				       (LayerTypePtr *) Result1,
				       (PolygonTypePtr *) Result2,
				       (PolygonTypePtr *) Result3))
	    {
	      if (HigherAvail)
		{
		  BoxTypePtr box =
		    &(*(PolygonTypePtr *) Result2)->BoundingBox;
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
			   (ElementTypePtr *) Result1,
			   (PadTypePtr *) Result2, (PadTypePtr *) Result3,
			   true))
    return (PAD_TYPE);

  if (Type & ELEMENTNAME_TYPE &&
      SearchElementNameByLocation (locked,
				   (ElementTypePtr *) Result1,
				   (TextTypePtr *) Result2,
				   (TextTypePtr *) Result3, true))
    return (ELEMENTNAME_TYPE);

  if (Type & ELEMENT_TYPE &&
      SearchElementByLocation (locked,
			       (ElementTypePtr *) Result1,
			       (ElementTypePtr *) Result2,
			       (ElementTypePtr *) Result3, true))
    return (ELEMENT_TYPE);

  return (NO_TYPE);
}

/* ---------------------------------------------------------------------------
 * searches for a object by it's unique ID. It doesn't matter if
 * the object is visible or not. The search is performed on a PCB, a
 * buffer or on the remove list.
 * The calling routine passes two pointers to allocated memory for storing
 * the results. 
 * A type value is returned too which is NO_TYPE if no objects has been found.
 */
int
SearchObjectByID (DataTypePtr Base,
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

/* ---------------------------------------------------------------------------
 * searches for an element by its board name.
 * The function returns a pointer to the element, NULL if not found
 */
ElementTypePtr
SearchElementByName (DataTypePtr Base, char *Name)
{
  ElementTypePtr result = NULL;

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

/* ---------------------------------------------------------------------------
 * searches the cursor position for the type 
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

/* ---------------------------------------------------------------------------
 * searches the cursor position for the type
 */
int
SearchScreenGridSlop (Coord X, Coord Y, int Type, void **Result1,
	      void **Result2, void **Result3)
{
  int ans;

  ans = SearchObjectByLocation (Type, Result1, Result2, Result3,
				X, Y, PCB->Grid / 2);
  return (ans);
}
