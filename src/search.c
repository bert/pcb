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

/* search routines
 * some of the functions use dummy parameters
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>

#include "global.h"

#include "data.h"
#include "draw.h"
#include "error.h"
#include "find.h"
#include "misc.h"
#include "search.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static Position PosX,		/* search position for subroutines */
  PosY;
static Dimension SearchRadius;
static LayerTypePtr SearchLayer;
static Boolean ScreenOnly = False;

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static Boolean SearchLineByPosition (LayerTypePtr *, LineTypePtr *,
				     LineTypePtr *);
static Boolean SearchArcByPosition (LayerTypePtr *, ArcTypePtr *,
				    ArcTypePtr *);
static Boolean SearchRatLineByPosition (RatTypePtr *, RatTypePtr *,
					RatTypePtr *);
static Boolean SearchTextByPosition (LayerTypePtr *, TextTypePtr *,
				     TextTypePtr *);
static Boolean SearchPolygonByPosition (LayerTypePtr *, PolygonTypePtr *,
					PolygonTypePtr *);
static Boolean SearchPinByPosition (ElementTypePtr *, PinTypePtr *,
				    PinTypePtr *);
static Boolean SearchPadByPosition (ElementTypePtr *, PadTypePtr *,
				    PadTypePtr *, Boolean);
static Boolean SearchViaByPosition (PinTypePtr *, PinTypePtr *, PinTypePtr *);
static Boolean SearchElementNameByPosition (ElementTypePtr *, TextTypePtr *,
					    TextTypePtr *, Boolean);
static Boolean SearchLinePointByPosition (LayerTypePtr *, LineTypePtr *,
					  PointTypePtr *);
static Boolean SearchPointByPosition (LayerTypePtr *, PolygonTypePtr *,
				      PointTypePtr *);
static Boolean SearchElementByPosition (ElementTypePtr *, ElementTypePtr *,
					ElementTypePtr *, Boolean);
static Boolean IsPointInBox (Position, Position, BoxTypePtr, Cardinal);

/* ---------------------------------------------------------------------------
 * searches a via
 */
static Boolean
SearchViaByPosition (PinTypePtr * Via, PinTypePtr * Dummy1,
		     PinTypePtr * Dummy2)
{
  /* search only if via-layer is visible */
  if (PCB->ViaOn)
    VIA_LOOP (PCB->Data, 
      {
	if (ScreenOnly && !VVIA (via))
	  continue;
	if (IsPointOnPin (PosX, PosY, SearchRadius, (PinTypePtr) via))
	  {
	    *Via = *Dummy1 = *Dummy2 = via;
	    return (True);
	  }
      }
  );
  return (False);
}

/* ---------------------------------------------------------------------------
 * searches a pin
 * starts with the newest element
 */
static Boolean
SearchPinByPosition (ElementTypePtr * Element, PinTypePtr * Pin,
		     PinTypePtr * Dummy)
{
  /* search only if pin-layer is visible */
  if (PCB->PinOn)
    ELEMENT_LOOP (PCB->Data, 
      {
	if (ScreenOnly && !VELEMENT (element))
	  continue;
	PIN_LOOP (element, 
	    {
	      if (IsPointOnPin (PosX, PosY, SearchRadius, pin))
		{
		  *Element = element;
		  *Pin = *Dummy = pin;
		  return (True);
		}
	    }
	);
      }
  );
  return (False);
}

/* ---------------------------------------------------------------------------
 * searches a pad
 * starts with the newest element
 */
static Boolean
SearchPadByPosition (ElementTypePtr * Element, PadTypePtr * Pad,
		     PadTypePtr * Dummy, Boolean BackToo)
{
  /* search only if pin-layer is visible */
  if (!PCB->PinOn)
    return (False);
  ELEMENT_LOOP (PCB->Data, 
      {
	if (ScreenOnly && !VELEMENT (element))
	  continue;
	PAD_LOOP (element, 
	    {
	      if (FRONT (pad) || (BackToo & PCB->InvisibleObjectsOn))
		{
		  if (TEST_FLAG (SQUAREFLAG, pad))
		    {
		      if (IsPointInSquarePad (PosX, PosY, SearchRadius, pad))
			{
			  *Element = element;
			  *Pad = *Dummy = pad;
			  return (True);
			}
		    }
		  else
		    {
		      /* the cast isn't very nice but working, check
		       * global.h for details
		       */
		      if (IsPointOnLine
			  (PosX, PosY, SearchRadius, (LineTypePtr) pad))
			{
			  *Element = element;
			  *Pad = *Dummy = pad;
			  return (True);
			}
		    }
		}
	    }
	);
      }
  );
  return (False);
}

/* ---------------------------------------------------------------------------
 * searches ordinary line on the SearchLayer 
 */
static Boolean
SearchLineByPosition (LayerTypePtr * Layer, LineTypePtr * Line,
		      LineTypePtr * Dummy)
{
  *Layer = SearchLayer;
  LINE_LOOP (*Layer, 
      {
	if (ScreenOnly && !VLINE (line))
	  continue;
	if (IsPointOnLine (PosX, PosY, SearchRadius, line))
	  {
	    *Line = *Dummy = line;
	    return (True);
	  }
      }
  );
  return (False);
}

/* ---------------------------------------------------------------------------
 * searches rat lines if they are visible
 */
static Boolean
SearchRatLineByPosition (RatTypePtr * Line, RatTypePtr * Dummy1,
			 RatTypePtr * Dummy2)
{
  RAT_LOOP (PCB->Data, 
      {
	if (ScreenOnly && !VLINE (line))
	  continue;
	if (IsPointOnLine (PosX, PosY, SearchRadius, (LineTypePtr) line))
	  {
	    *Line = *Dummy1 = *Dummy2 = line;
	    return (True);
	  }
      }
  );
  return (False);
}

/* ---------------------------------------------------------------------------
 * searches arc on the SearchLayer 
 */
static Boolean
SearchArcByPosition (LayerTypePtr * Layer, ArcTypePtr * Arc,
		     ArcTypePtr * Dummy)
{
  *Layer = SearchLayer;
  ARC_LOOP (*Layer, 
      {
	if (ScreenOnly && !VARC (arc))
	  continue;
	if (IsPointOnArc (PosX, PosY, SearchRadius, arc))
	  {
	    *Arc = *Dummy = arc;
	    return (True);
	  }
      }
  );
  return (False);
}

/* ---------------------------------------------------------------------------
 * searches text on the SearchLayer
 */
static Boolean
SearchTextByPosition (LayerTypePtr * Layer, TextTypePtr * Text,
		      TextTypePtr * Dummy)
{
  *Layer = SearchLayer;
  TEXT_LOOP (*Layer, 
      {
	if (ScreenOnly && !VTEXT (text))
	  continue;
	if (TEXT_IS_VISIBLE (PCB, *Layer, text) &&
	    POINT_IN_BOX (PosX, PosY, &text->BoundingBox))
	  {
	    *Text = *Dummy = text;
	    return (True);
	  }
      }
  );
  return (False);
}

/* ---------------------------------------------------------------------------
 * searches a polygon on the SearchLayer 
 */
static Boolean
SearchPolygonByPosition (LayerTypePtr * Layer,
			 PolygonTypePtr * Polygon, PolygonTypePtr * Dummy)
{
  *Layer = SearchLayer;
  POLYGON_LOOP (*Layer, 
      {
	if (ScreenOnly && !VPOLY (polygon))
	  continue;
	if (IsPointInPolygon (PosX, PosY, SearchRadius, polygon))
	  {
	    *Polygon = *Dummy = polygon;
	    return (True);
	  }
      }
  );
  return (False);
}

/* ---------------------------------------------------------------------------
 * searches a line-point on all the search layer
 */
static Boolean
SearchLinePointByPosition (LayerTypePtr * Layer, LineTypePtr * Line,
			   PointTypePtr * Point)
{
  float d, least;
  Boolean found = False;

  least =
    (MAX_LINE_POINT_DISTANCE + SearchRadius) * (MAX_LINE_POINT_DISTANCE +
						SearchRadius);
  *Layer = SearchLayer;
  LINE_LOOP (*Layer, 
      {
	if (ScreenOnly && !VLINE (line))
	  continue;
	/* some stupid code to check both points */
	d = (PosX - line->Point1.X) * (PosX - line->Point1.X) +
	  (PosY - line->Point1.Y) * (PosY - line->Point1.Y);
	if (d < least)
	  {
	    least = d;
	    *Line = line;
	    *Point = &line->Point1;
	    found = True;
	  }

	d = (PosX - line->Point2.X) * (PosX - line->Point2.X) +
	  (PosY - line->Point2.Y) * (PosY - line->Point2.Y);
	if (d < least)
	  {
	    least = d;
	    *Line = line;
	    *Point = &line->Point2;
	    found = True;
	  }
      }
  );
  /* return with nearest */
  if (found)
    return (True);
  return (False);
}

/* ---------------------------------------------------------------------------
 * searches a polygon-point on all layers that are switched on
 * in layerstack order
 */
static Boolean
SearchPointByPosition (LayerTypePtr * Layer,
		       PolygonTypePtr * Polygon, PointTypePtr * Point)
{
  float d, least;
  Boolean found = False;

  least =
    (SearchRadius + MAX_POLYGON_POINT_DISTANCE) * (SearchRadius +
						   MAX_POLYGON_POINT_DISTANCE);
  *Layer = SearchLayer;
  POLYGON_LOOP (*Layer, 
      {
	if (ScreenOnly && !VPOLY (polygon))
	  continue;
	POLYGONPOINT_LOOP (polygon, 
	    {
		d =
		  (point->X - PosX) * (point->X -
				       PosX) +
		  (point->Y - PosY) * (point->Y - PosY);
		if (d < least)
		  {
		    least = d;
		    *Polygon = polygon;
		    *Point = point;
		    found = True;
		  }
	    }
	);
      }
  );
  if (found)
    return (True);
  return (False);
}

/* ---------------------------------------------------------------------------
 * searches the name of an element
 * the search starts with the last element and goes back to the beginning
 */
static Boolean
SearchElementNameByPosition (ElementTypePtr * Element,
			     TextTypePtr * Text, TextTypePtr * Dummy,
			     Boolean BackToo)
{
  TextTypePtr text;

  /* package layer have to be switched on */
  if (PCB->ElementOn)
    {
      ELEMENT_LOOP (PCB->Data, 
	  {
	    if (ScreenOnly && !VELTEXT (element))
	      continue;
	    if ((FRONT (element)
		 || (BackToo && PCB->InvisibleObjectsOn))
		&& !TEST_FLAG (HIDENAMEFLAG, element))
	      {
		text = &ELEMENT_TEXT (PCB, element);
		if (POINT_IN_BOX (PosX, PosY, &text->BoundingBox))
		  {
		    *Element = element;
		    *Text = *Dummy = text;
		    return (True);
		  }
	      }
	  }
      );
    }
  return (False);
}

/* ---------------------------------------------------------------------------
 * searches an element
 * the search starts with the last element and goes back to the beginning
 * if more than one element matches, the smallest one is taken
 */
static Boolean
SearchElementByPosition (ElementTypePtr * Element,
			 ElementTypePtr * Dummy1, ElementTypePtr * Dummy2,
			 Boolean BackToo)
{
  ElementTypePtr save = NULL;
  long area = 0;
  Boolean found;

  /* Both package layers have to be switched on */
  if (PCB->ElementOn && PCB->PinOn)
    {
      /* the element names bounding box is not necessarily
       * a part of the elements bounding box;
       * we have to check all of them
       */
      ELEMENT_LOOP (PCB->Data, 
	  {
	    if (ScreenOnly && !VELEMENT (element))
	      continue;
	    if (FRONT (element) || (BackToo && PCB->InvisibleObjectsOn))
	      {
		found = POINT_IN_BOX (PosX, PosY, &element->BoundingBox);
		if (!TEST_FLAG (HIDENAMEFLAG, element))
		  found |= POINT_IN_BOX (PosX, PosY,
					 &element->
					 Name[NAME_INDEX (PCB)].BoundingBox);
		if (found)
		  {
		    long newarea;
		    /* use the element with the smallest bounding box */
		    newarea =
		      (element->BoundingBox.X2 -
		       element->BoundingBox.X1) * (element->BoundingBox.Y2 -
						   element->BoundingBox.Y1);
		    if (!save || newarea < area)
		      {
			area = newarea;
			save = element;
		      }
		  }
	      }
	  }
      );
    }
  *Element = *Dummy1 = *Dummy2 = save;
  return (save != NULL);
}

/* ---------------------------------------------------------------------------
 * checks if a point is on a pin
 */
Boolean
IsPointOnPin (float X, float Y, float Radius, PinTypePtr pin)
{
  if (TEST_FLAG (SQUAREFLAG, pin))
    {
      BoxType b;
      Dimension t = pin->Thickness / 2;

      b.X1 = pin->X - t;
      b.X2 = pin->X + t;
      b.Y1 = pin->Y - t;
      b.Y2 = pin->Y + t;
      if (IsPointInBox (X, Y, &b, Radius))
	return (True);
    }
  else if (SQUARE (pin->X - X) + SQUARE (pin->Y - Y) <=
	   SQUARE (pin->Thickness / 2 + Radius))
    return (True);
  return (False);
}

/* ---------------------------------------------------------------------------
 * checks if a rat-line end is on a PV
 */
Boolean
IsPointOnLineEnd (Position X, Position Y, RatTypePtr Line)
{
  if (((X == Line->Point1.X) && (Y == Line->Point1.Y)) ||
      ((X == Line->Point2.X) && (Y == Line->Point2.Y)))
    return (True);
  return (False);
}

/* ---------------------------------------------------------------------------
 * checks if a line intersects with a PV
 * constant recognition by the optimizer is assumed
 *
 * let the point be (X,Y) and the line (X1,Y1)(X2,Y2)
 * the length of the line is
 *
 *   l = ((X2-X1)^2 + (Y2-Y1)^2)^0.5
 * 
 * let Q be the point of perpendicular projection of (X,Y) onto the line
 *
 *   QX = X1 +r*(X2-X1)
 *   QY = Y1 +r*(Y2-Y1)
 * 
 * with (from vector geometry)
 *
 *       (Y1-Y)(Y1-Y2)+(X1-X)(X1-X2)
 *   r = ---------------------------
 *                   l*l
 *
 *   r < 0     Q is on backward extension of the line
 *   r > 1     Q is on forward extension of the line
 *   else      Q is on the line
 *
 * the signed distance from (X,Y) to Q is
 *
 *       (Y2-Y1)(X-X1)-(X2-X1)(Y-Y1)
 *   d = ----------------------------
 *                    l
 */
Boolean
IsPointOnLine (float X, float Y, float Radius, LineTypePtr Line)
{
  register float dx, dy, dx1, dy1, l, d, r;
  Radius += ((float) Line->Thickness + 1.) / 2.0;
  if (Y + Radius < MIN (Line->Point1.Y, Line->Point2.Y) ||
      Y - Radius > MAX (Line->Point1.Y, Line->Point2.Y))
    return False;
  dx = (float) (Line->Point2.X - Line->Point1.X);
  dy = (float) (Line->Point2.Y - Line->Point1.Y);
  dx1 = (float) (Line->Point1.X - X);
  dy1 = (float) (Line->Point1.Y - Y);
  d = dx * dy1 - dy * dx1;

  /* check distance from PV to line */
  Radius *= Radius;
  if ((l = dx * dx + dy * dy) == 0.0)
    {
      l =
	(Line->Point1.X - X) * (Line->Point1.X - X) + (Line->Point1.Y -
						       Y) * (Line->Point1.Y -
							     Y);
      return ((l <= Radius) ? True : False);
    }
  if (d * d > Radius * l)
    return (False);

  /* they intersect if Q is on line */
  r = -(dx * dx1 + dy * dy1);
  if (r >= 0 && r <= l)
    return (True);

  /* we have to check P1 or P2 depending on the sign of r */
  if (r < 0.0)
    return ((dx1 * dx1 + dy1 * dy1) <= Radius);
  dx1 = Line->Point2.X - X;
  dy1 = Line->Point2.Y - Y;
  return ((dx1 * dx1 + dy1 * dy1) <= Radius);
}

/* ---------------------------------------------------------------------------
 * checks if a line crosses a rectangle
 */
Boolean
IsLineInRectangle (Position X1, Position Y1,
		   Position X2, Position Y2, LineTypePtr Line)
{
  LineType line;
  Dimension thick = Line->Thickness / 2 + 1;
  Position minx = MIN (Line->Point1.X, Line->Point2.X) - thick,
    miny = MIN (Line->Point1.Y, Line->Point2.Y) - thick,
    maxx = MAX (Line->Point1.X, Line->Point2.X) + thick,
    maxy = MAX (Line->Point1.Y, Line->Point2.Y) + thick;

  if (miny > MAX (Y1, Y2) || maxy < MIN (Y1, Y2))
    return (False);
  if (minx > MAX (X1, X2) || maxx < MIN (X1, X2))
    return (False);
  /* first, see if point 1 is inside the rectangle */
  /* in case the whole line is inside the rectangle */
  if (X1 < Line->Point1.X && X2 > Line->Point1.X &&
      Y1 < Line->Point1.Y && Y2 > Line->Point1.Y)
    return (True);
  /* construct a set of dummy lines and check each of them */
  line.Thickness = 0;
  line.Flags = NOFLAG;

  /* upper-left to upper-right corner */
  if (miny <= Y1 && maxy >= Y1)
    {
      line.Point1.Y = line.Point2.Y = Y1;
      line.Point1.X = X1;
      line.Point2.X = X2;
      if (LineLineIntersect (&line, Line))
	return (True);
    }

  /* upper-right to lower-right corner */
  if (minx <= X2 && maxx >= X2)
    {
      line.Point1.X = line.Point2.X = X2;
      line.Point1.Y = Y1;
      line.Point2.Y = Y2;
      if (LineLineIntersect (&line, Line))
	return (True);
    }

  /* lower-right to lower-left corner */
  if (miny <= Y2 && maxy >= Y2)
    {
      line.Point1.Y = line.Point2.Y = Y2;
      line.Point1.X = X1;
      line.Point2.X = X2;
      if (LineLineIntersect (&line, Line))
	return (True);
    }

  /* lower-left to upper-left corner */
  if (minx <= X1 && maxx >= X1)
    {
      line.Point1.X = line.Point2.X = X1;
      line.Point1.Y = Y1;
      line.Point2.Y = Y2;
      if (LineLineIntersect (&line, Line))
	return (True);
    }

  return (False);
}

/* ---------------------------------------------------------------------------
 * checks if an arc crosses a square
 */
Boolean
IsArcInRectangle (Position X1, Position Y1,
		  Position X2, Position Y2, ArcTypePtr Arc)
{
  LineType line;

  /* construct a set of dummy lines and check each of them */
  line.Thickness = 0;
  line.Flags = NOFLAG;

  /* upper-left to upper-right corner */
  line.Point1.Y = line.Point2.Y = Y1;
  line.Point1.X = X1;
  line.Point2.X = X2;
  if (LineArcIntersect (&line, Arc))
    return (True);

  /* upper-right to lower-right corner */
  line.Point1.X = line.Point2.X = X2;
  line.Point1.Y = Y1;
  line.Point2.Y = Y2;
  if (LineArcIntersect (&line, Arc))
    return (True);

  /* lower-right to lower-left corner */
  line.Point1.Y = line.Point2.Y = Y2;
  line.Point1.X = X1;
  line.Point2.X = X2;
  if (LineArcIntersect (&line, Arc))
    return (True);

  /* lower-left to upper-left corner */
  line.Point1.X = line.Point2.X = X1;
  line.Point1.Y = Y1;
  line.Point2.Y = Y2;
  if (LineArcIntersect (&line, Arc))
    return (True);

  return (False);
}

/* ---------------------------------------------------------------------------
 * check if a point lies inside a square Pad
 * fixed 10/30/98 - radius can't expand both edges of a square box
 */
Boolean
IsPointInSquarePad (Position X, Position Y, Cardinal Radius, PadTypePtr Pad)
{
  register Dimension t2 = Pad->Thickness / 2;
  BoxType padbox;

  padbox.X1 = MIN (Pad->Point1.X, Pad->Point2.X) - t2;
  padbox.X2 = MAX (Pad->Point1.X, Pad->Point2.X) + t2;
  padbox.Y1 = MIN (Pad->Point1.Y, Pad->Point2.Y) - t2;
  padbox.Y2 = MAX (Pad->Point1.Y, Pad->Point2.Y) + t2;
  return (IsPointInBox (X, Y, &padbox, Radius));
}

static Boolean
IsPointInBox (Position X, Position Y, BoxTypePtr box, Cardinal Radius)
{
  LineType line;

  if (POINT_IN_BOX (X, Y, box))
    return (True);
  line.Thickness = 0;
  line.Flags = NOFLAG;

  line.Point1.X = box->X1;
  line.Point1.Y = box->Y1;
  line.Point2.X = box->X2;
  line.Point2.Y = box->Y1;
  if (IsPointOnLine (X, Y, Radius, &line))
    return (True);
  line.Point2.X = box->X1;
  line.Point2.Y = box->Y2;
  if (IsPointOnLine (X, Y, Radius, &line))
    return (True);
  line.Point1.X = box->X2;
  line.Point1.Y = box->Y2;
  if (IsPointOnLine (X, Y, Radius, &line))
    return (True);
  line.Point2.X = box->X2;
  line.Point2.Y = box->Y1;
  return (IsPointOnLine (X, Y, Radius, &line));
}

/* ---------------------------------------------------------------------------
 * checks if a point lies inside a polygon
 * the code assumes that the last point isn't equal to the first one
 * The algorithm fails if the point is equal to a corner
 * from news FAQ:
 *   This code is from Wm. Randolph Franklin, wrf@ecse.rpi.edu, a
 *   professor at RPI.
 *
 * extentions:
 * check if the distance between the polygon lines and the point is less
 * or equal to the radius.
 */
Boolean
IsPointInPolygon (float X, float Y, float Radius, PolygonTypePtr Polygon)
{
  Boolean inside = False;
  BoxType boundingbox = Polygon->BoundingBox;

  /* increment the size of the bounding box by the radius */
  boundingbox.X1 -= (Position) Radius;
  boundingbox.Y1 -= (Position) Radius;
  boundingbox.X2 += (Position) Radius;
  boundingbox.Y2 += (Position) Radius;

  /* quick check if the point may lay inside */
  if (POINT_IN_BOX (X, Y, &boundingbox))
    {
      PointTypePtr ptr = &Polygon->Points[0];

      /* POLYGONPOINT_LOOP decrements pointers !!! */
      POLYGONPOINT_LOOP (Polygon, 
	  {
	    if ((((ptr->Y <= Y) && (Y < point->Y)) ||
		 ((point->Y <= Y) && (Y < ptr->Y))) &&
		(X <
		 ((float) (point->X - ptr->X) *
		  (float) (Y - ptr->Y) / (float) (point->Y - ptr->Y) +
		  ptr->X)))
	      inside = !inside;
	    ptr = point;
          }
      );

      /* check the distance between the lines of the
       * polygon and the point itself
       *
       * check is done by contructing a dummy line
       */
      if (!inside)
	{
	  LineType line;

	  line.Point1 = Polygon->Points[0];
	  line.Thickness = 0;
	  line.Flags = NOFLAG;

	  /* POLYGONPOINT_LOOP decrements pointers !!! */
	  POLYGONPOINT_LOOP (Polygon, 
	      {
		line.Point2 = *point;
		if (IsPointOnLine (X, Y, Radius, &line))
		  return (True);
		line.Point1 = *point;
	      }
	  );

	}
    }
  return (inside);
}

/* ---------------------------------------------------------------------------
 * checks if a polygon intersects with a square
 */
Boolean
IsRectangleInPolygon (Position X1, Position Y1,
		      Position X2, Position Y2, PolygonTypePtr Polygon)
{
  PolygonType polygon;
  PointType points[4];

  /* construct a dummy polygon and check it */
  polygon.BoundingBox.X1 = points[0].X = points[3].X = X1;
  polygon.BoundingBox.X2 = points[1].X = points[2].X = X2;
  polygon.BoundingBox.Y1 = points[0].Y = points[1].Y = Y1;
  polygon.BoundingBox.Y2 = points[2].Y = points[3].Y = Y2;
  polygon.Points = points;
  polygon.PointN = 4;
  return (IsPolygonInPolygon (&polygon, Polygon));
}

Boolean
IsPointOnArc (float X, float Y, float Radius, ArcTypePtr Arc)
{

  register float x, y, dx, dy, r1, r2, a, d, l;
  register Position pdx, pdy;
  int ang1, ang2, delta;
  int startAngle, arcDelta;

  pdx = X - Arc->X;
  pdy = Y - Arc->Y;
  l = pdx * pdx + pdy * pdy;
  /* concentric arcs, simpler intersection conditions */
  if (l == 0.0)
    {
      if (Arc->Width <= Radius + 0.5 * Arc->Thickness)
	return (True);
      else
	return (False);
    }
  r1 = Arc->Width;
  r1 *= r1;
  r2 = Radius + 0.5 * Arc->Thickness;
  r2 *= r2;
  a = 0.5 * (r1 - r2 + l) / l;
  r1 = r1 / l;
  /* add a tiny positive number for round-off problems */
  d = r1 - a * a + 1e-5;
  /* the circles are too far apart to touch */
  if (d < 0)
    return (False);
  /* project the points of intersection */
  d = sqrt (d);
  x = a * pdx;
  y = a * pdy;
  dy = -d * pdx;
  dx = d * pdy;
  /* arrgh! calculate the angles, and put them in a standard range */
  startAngle = Arc->StartAngle;
  arcDelta = Arc->Delta;
  while (startAngle < 0)
    startAngle += 360;
  if (arcDelta < 0)
    {
      startAngle += arcDelta;
      arcDelta = -arcDelta;
      while (startAngle < 0)
	startAngle += 360;
    }
  ang1 = RAD_TO_DEG * atan2 ((y + dy), -(x + dx));
  if (ang1 < 0)
    ang1 += 360;
  ang2 = RAD_TO_DEG * atan2 ((y - dy), -(x - dx));
  if (ang2 < 0)
    ang2 += 360;
  delta = ang2 - ang1;
  if (delta > 180)
    delta -= 360;
  else if (delta < -180)
    delta += 360;
  if (delta < 0)
    {
      ang1 += delta;
      delta = -delta;
      while (ang1 < 0)
	ang1 += 360;
    }
  if (ang1 >= startAngle && ang1 <= startAngle + arcDelta)
    return (True);
  if (startAngle >= ang1 && startAngle <= ang1 + delta)
    return (True);
  if (startAngle + arcDelta >= 360 && ang1 <= startAngle + arcDelta - 360)
    return (True);
  if (ang1 + delta >= 360 && startAngle <= ang1 + delta - 360)
    return (True);
  return (False);
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
 */
int
SearchObjectByPosition (int Type,
			void **Result1, void **Result2, void **Result3,
			Position X, Position Y, Dimension Radius)
{
  void *r1, *r2, *r3;
  int i;
  int HigherBound = 0;
  int HigherAvail = NO_TYPE;
  /* setup local identifiers */
  PosX = X;
  PosY = Y;
  SearchRadius = Radius;

  if (Type & VIA_TYPE &&
      SearchViaByPosition ((PinTypePtr *) Result1,
			   (PinTypePtr *) Result2, (PinTypePtr *) Result3))
    return (VIA_TYPE);

  if (Type & PIN_TYPE &&
      SearchPinByPosition ((ElementTypePtr *) & r1,
			   (PinTypePtr *) & r2, (PinTypePtr *) & r3))
    HigherAvail = PIN_TYPE;

  if (!HigherAvail && Type & PAD_TYPE &&
      SearchPadByPosition ((ElementTypePtr *) & r1,
			   (PadTypePtr *) & r2, (PadTypePtr *) & r3, False))
    HigherAvail = PAD_TYPE;

  if (!HigherAvail && Type & ELEMENTNAME_TYPE &&
      SearchElementNameByPosition ((ElementTypePtr *) & r1,
				   (TextTypePtr *) & r2, (TextTypePtr *) & r3,
				   False))
    {
      BoxTypePtr box = &((TextTypePtr) r2)->BoundingBox;
      HigherBound = (box->X2 - box->X1) * (box->Y2 - box->Y1);
      HigherAvail = ELEMENTNAME_TYPE;
    }

  if (!HigherAvail && Type & ELEMENT_TYPE &&
      SearchElementByPosition ((ElementTypePtr *) & r1,
			       (ElementTypePtr *) & r2,
			       (ElementTypePtr *) & r3, False))
    {
      BoxTypePtr box = &((ElementTypePtr) r1)->BoundingBox;
      HigherBound = (box->X2 - box->X1) * (box->Y2 - box->Y1);
      HigherAvail = ELEMENT_TYPE;
    }

  for (i = -1; i < MAX_LAYER + 1; i++)
    {
      if (i < 0)
	SearchLayer = &PCB->Data->SILKLAYER;
      else if (i < MAX_LAYER)
	SearchLayer = LAYER_ON_STACK (i);
      else
	SearchLayer = &PCB->Data->BACKSILKLAYER;
      if (SearchLayer->On)
	{
	  if (Type & POLYGONPOINT_TYPE &&
	      SearchPointByPosition ((LayerTypePtr *) Result1,
				     (PolygonTypePtr *) Result2,
				     (PointTypePtr *) Result3))
	    return (POLYGONPOINT_TYPE);

	  if (Type & LINEPOINT_TYPE &&
	      SearchLinePointByPosition ((LayerTypePtr *) Result1,
					 (LineTypePtr *) Result2,
					 (PointTypePtr *) Result3))
	    return (LINEPOINT_TYPE);

	  if ((HigherAvail & (PIN_TYPE | PAD_TYPE)) == 0 && Type & LINE_TYPE
	      && SearchLineByPosition ((LayerTypePtr *) Result1,
				       (LineTypePtr *) Result2,
				       (LineTypePtr *) Result3))
	    return (LINE_TYPE);

	  if ((HigherAvail & (PIN_TYPE | PAD_TYPE)) == 0 && Type & ARC_TYPE &&
	      SearchArcByPosition ((LayerTypePtr *) Result1,
				   (ArcTypePtr *) Result2,
				   (ArcTypePtr *) Result3))
	    return (ARC_TYPE);

	  if ((HigherAvail & (PIN_TYPE | PAD_TYPE)) == 0 && Type & TEXT_TYPE
	      && SearchTextByPosition ((LayerTypePtr *) Result1,
				       (TextTypePtr *) Result2,
				       (TextTypePtr *) Result3))
	    return (TEXT_TYPE);

	  if (Type & POLYGON_TYPE &&
	      SearchPolygonByPosition ((LayerTypePtr *) Result1,
				       (PolygonTypePtr *) Result2,
				       (PolygonTypePtr *) Result3))
	    {
	      if (HigherAvail)
		{
		  BoxTypePtr box =
		    &(*(PolygonTypePtr *) Result2)->BoundingBox;
		  int area = (box->X2 - box->X1) * (box->X2 - box->X1);
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

  if (Type & RATLINE_TYPE &&
      SearchRatLineByPosition ((RatTypePtr *) Result1,
			       (RatTypePtr *) Result2,
			       (RatTypePtr *) Result3))
    return (RATLINE_TYPE);

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
      SearchPadByPosition ((ElementTypePtr *) Result1,
			   (PadTypePtr *) Result2, (PadTypePtr *) Result3,
			   True))
    return (PAD_TYPE);

  if (Type & ELEMENTNAME_TYPE &&
      SearchElementNameByPosition ((ElementTypePtr *) Result1,
				   (TextTypePtr *) Result2,
				   (TextTypePtr *) Result3, True))
    return (ELEMENTNAME_TYPE);

  if (Type & ELEMENT_TYPE &&
      SearchElementByPosition ((ElementTypePtr *) Result1,
			       (ElementTypePtr *) Result2,
			       (ElementTypePtr *) Result3, True))
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
      ALLLINE_LOOP (Base, 
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
      );
    }
  if (type == ARC_TYPE)
    {
      ALLARC_LOOP (Base, 
	  {
	    if (arc->ID == ID)
	      {
		*Result1 = (void *) layer;
		*Result2 = *Result3 = (void *) arc;
		return (ARC_TYPE);
	      }
	  }
      );
    }

  if (type == TEXT_TYPE)
    {
      ALLTEXT_LOOP (Base, 
	  {
	    if (text->ID == ID)
	      {
		*Result1 = (void *) layer;
		*Result2 = *Result3 = (void *) text;
		return (TEXT_TYPE);
	      }
	  }
      );
    }

  if (type == POLYGON_TYPE || type == POLYGONPOINT_TYPE)
    {
      ALLPOLYGON_LOOP (Base, 
	  {
	    if (polygon->ID == ID)
	      {
		*Result1 = (void *) layer;
		*Result2 = *Result3 = (void *) polygon;
		return (POLYGON_TYPE);
	      }
	    POLYGONPOINT_LOOP (polygon, 
		{
		  if (point->ID == ID)
		    {
		      *Result1 = (void *) layer;
		      *Result2 = (void *) polygon;
		      *Result3 = (void *) point;
		      return (POLYGONPOINT_TYPE);
		    }
		}
	    );
	  }
      );
    }
  if (type == VIA_TYPE)
    {
      VIA_LOOP (Base, 
	  {
	    if (via->ID == ID)
	      {
		*Result1 = *Result2 = *Result3 = (void *) via;
		return (VIA_TYPE);
	      }
	  }
      );
    }

  if (type == RATLINE_TYPE || type == LINEPOINT_TYPE)
    {
      RAT_LOOP (Base, 
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
      );
    }

  if (type == ELEMENT_TYPE || type == PAD_TYPE || type == PIN_TYPE
      || type == ELEMENTLINE_TYPE || type == ELEMENTNAME_TYPE
      || type == ELEMENTARC_TYPE)
    /* check pins and elementnames too */
    ELEMENT_LOOP (Base, 
      {
	if (element->ID == ID)
	  {
	    *Result1 = *Result2 = *Result3 = (void *) element;
	    return (ELEMENT_TYPE);
	  }
	if (type == ELEMENTLINE_TYPE)
	  ELEMENTLINE_LOOP (element, 
	    {
	      if (line->ID == ID)
		{
		  *Result1 = (void *) element;
		  *Result2 = *Result3 = (void *) line;
		  return (ELEMENTLINE_TYPE);
		}
	    }
	);
	if (type == ELEMENTARC_TYPE)
	  ARC_LOOP (element, 
	    {
	      if (arc->ID == ID)
		{
		  *Result1 = (void *) element;
		  *Result2 = *Result3 = (void *) arc;
		  return (ELEMENTARC_TYPE);
		}
	    }
	);
	if (type == ELEMENTNAME_TYPE)
	  ELEMENTTEXT_LOOP (element, 
	    {
	      if (text->ID == ID)
		{
		  *Result1 = (void *) element;
		  *Result2 = *Result3 = (void *) text;
		  return (ELEMENTNAME_TYPE);
		}
	    }
	);
	if (type == PIN_TYPE)
	  PIN_LOOP (element, 
	    {
	      if (pin->ID == ID)
		{
		  *Result1 = (void *) element;
		  *Result2 = *Result3 = (void *) pin;
		  return (PIN_TYPE);
		}
	    }
	);
	if (type == PAD_TYPE)
	  PAD_LOOP (element, 
	    {
	      if (pad->ID == ID)
		{
		  *Result1 = (void *) element;
		  *Result2 = *Result3 = (void *) pad;
		  return (PAD_TYPE);
		}
	    }
	);
      }
  );

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

  ELEMENT_LOOP (Base, 
      {
	if (element->Name[1].TextString &&
	    strcmp (element->Name[1].TextString, Name) == 0)
	  {
	    result = element;
	    return (result);
	  }
      }
  );
  return result;
}

/* ---------------------------------------------------------------------------
 * searches the cursor position for the type 
 */
int
SearchScreen (Position X, Position Y, int Type, void **Result1,
	      void **Result2, void **Result3)
{
  int ans;

  ScreenOnly = True;
  ans = SearchObjectByPosition (Type, Result1, Result2, Result3,
				X, Y, TO_PCB (SLOP));
  ScreenOnly = False;
  return (ans);
}
