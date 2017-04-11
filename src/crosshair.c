/*!
 * \file src/crosshair.c
 *
 * \brief Crosshair stuff.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <memory.h>
#include <math.h>

#include "global.h"
#include "hid_draw.h"

#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "line.h"
#include "misc.h"
#include "mymem.h"
#include "search.h"
#include "polygon.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

typedef struct
{
  int x, y;
} point;


/*!
 * \brief Make a copy of the pin structure, moved to the correct
 * position
 */
static void
thindraw_moved_pv (hidGC gc, PinType *pv, Coord x, Coord y)
{
  PinType moved_pv = *pv;
  moved_pv.X += x;
  moved_pv.Y += y;

  hid_draw_thin_pcb_pv (gc, gc, &moved_pv, true, false);
}

/*!
 * \brief Draw a dashed line.
 */
static void
draw_dashed_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  /*! \todo we need a real geometrical library, using double here is
   * plain wrong. */
  double dx = x2-x1;
  double dy = y2-y1;
  double len_squared = dx*dx + dy*dy;
  int n;
  const int segs = 11; /* must be odd */

  if (len_squared < 1000000)
  {
    /*! \todo line too short, just draw it -> magic value;
     * with a proper geo lib this would be gone anyway. */
    hid_draw_line (gc, x1, y1, x2, y2);
    return;
  }

  /* first seg is drawn from x1, y1 with no rounding error due to n-1 == 0 */
  for (n = 1; n < segs; n += 2)
    hid_draw_line (gc, x1 + (dx * (double) (n-1) / (double) segs),
                       y1 + (dy * (double) (n-1) / (double) segs),
                       x1 + (dx * (double) n / (double) segs),
                       y1 + (dy * (double) n / (double) segs));

  /* make sure the last segment is drawn properly to x2 and y2,
   * don't leave room for rounding errors. */
  hid_draw_line (gc, x2 - (dx / (double) segs),
                     y2 - (dy / (double) segs),
                     x2,
                     y2);
}

/*!
 * \brief Creates a tmp polygon with coordinates converted to screen
 * system.
 */
static void
XORPolygon (hidGC gc, PolygonType *polygon, Coord dx, Coord dy, int dash_last)
{
  Cardinal i;
  for (i = 0; i < polygon->PointN; i++)
    {
      Cardinal next = next_contour_point (polygon, i);

      if (next == 0)
        { /* last line: sometimes the implicit closing line */
          if (i == 1) /* corner case: don't draw two lines on top of
                       * each other - with XOR it looks bad */
            continue;

        if (dash_last)
          {
            draw_dashed_line (gc,
                              polygon->Points[i].X + dx,
                              polygon->Points[i].Y + dy,
                              polygon->Points[next].X + dx,
                              polygon->Points[next].Y + dy);
            break; /* skip normal line draw below */
          }
        }

      /* normal contour line */
      hid_draw_line (gc, polygon->Points[i].X + dx,
                         polygon->Points[i].Y + dy,
                         polygon->Points[next].X + dx,
                         polygon->Points[next].Y + dy);
    }
}

/*!
 * \brief Draws the outline of an arc.
 */
static void
XORDrawAttachedArc (hidGC gc, Coord thick)
{
  ArcType arc;
  BoxType *bx;
  Coord wx, wy;
  Angle sa, dir;
  Coord wid = thick / 2;

  wx = Crosshair.X - Crosshair.AttachedBox.Point1.X;
  wy = Crosshair.Y - Crosshair.AttachedBox.Point1.Y;
  if (wx == 0 && wy == 0)
    return;
  arc.X = Crosshair.AttachedBox.Point1.X;
  arc.Y = Crosshair.AttachedBox.Point1.Y;
  if (XOR (Crosshair.AttachedBox.otherway, abs (wy) > abs (wx)))
    {
      arc.X = Crosshair.AttachedBox.Point1.X + abs (wy) * SGNZ (wx);
      sa = (wx >= 0) ? 0 : 180;
#ifdef ARC45
      if (abs (wy) >= 2 * abs (wx))
	dir = (SGNZ (wx) == SGNZ (wy)) ? 45 : -45;
      else
#endif
	dir = (SGNZ (wx) == SGNZ (wy)) ? 90 : -90;
    }
  else
    {
      arc.Y = Crosshair.AttachedBox.Point1.Y + abs (wx) * SGNZ (wy);
      sa = (wy >= 0) ? -90 : 90;
#ifdef ARC45
      if (abs (wx) >= 2 * abs (wy))
	dir = (SGNZ (wx) == SGNZ (wy)) ? -45 : 45;
      else
#endif
	dir = (SGNZ (wx) == SGNZ (wy)) ? -90 : 90;
      wy = wx;
    }
  wy = abs (wy);
  arc.StartAngle = sa;
  arc.Delta = dir;
  arc.Width = arc.Height = wy;
  bx = GetArcEnds (&arc);
  /*  sa = sa - 180; */
  hid_draw_arc (gc, arc.X, arc.Y, wy + wid, wy + wid, sa, dir);
  if (wid > pixel_slop)
    {
      hid_draw_arc (gc, arc.X, arc.Y, wy - wid, wy - wid, sa, dir);
      hid_draw_arc (gc, bx->X1, bx->Y1, wid, wid, sa,      -180 * SGN (dir));
      hid_draw_arc (gc, bx->X2, bx->Y2, wid, wid, sa + dir, 180 * SGN (dir));
    }
}

/*!
 * \brief Draws the outline of a line.
 */
static void
XORDrawAttachedLine (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2, Coord thick)
{
  Coord dx, dy, ox, oy;
  double h;

  dx = x2 - x1;
  dy = y2 - y1;
  if (dx != 0 || dy != 0)
    h = 0.5 * thick / hypot (dx, dy);
  else
    h = 0.0;
  ox = dy * h + 0.5 * SGN (dy);
  oy = -(dx * h + 0.5 * SGN (dx));
  hid_draw_line (gc, x1 + ox, y1 + oy, x2 + ox, y2 + oy);
  if (abs (ox) >= pixel_slop || abs (oy) >= pixel_slop)
    {
      Angle angle = atan2 (dx, dy) * 57.295779;
      hid_draw_line (gc, x1 - ox, y1 - oy, x2 - ox, y2 - oy);
      hid_draw_arc (gc, x1, y1, thick / 2, thick / 2, angle - 180, 180);
      hid_draw_arc (gc, x2, y2, thick / 2, thick / 2, angle, 180);
    }
}

/*!
 * \brief Draws the elements of a loaded circuit which is to be merged
 * in.
 */
static void
XORDrawElement (hidGC gc, ElementType *Element, Coord DX, Coord DY)
{
  /* if no silkscreen, draw the bounding box */
  if (Element->ArcN == 0 && Element->LineN == 0)
    {
      hid_draw_line (gc, DX + Element->BoundingBox.X1,
                         DY + Element->BoundingBox.Y1,
                         DX + Element->BoundingBox.X1,
                         DY + Element->BoundingBox.Y2);
      hid_draw_line (gc, DX + Element->BoundingBox.X1,
                         DY + Element->BoundingBox.Y2,
                         DX + Element->BoundingBox.X2,
                         DY + Element->BoundingBox.Y2);
      hid_draw_line (gc, DX + Element->BoundingBox.X2,
                         DY + Element->BoundingBox.Y2,
                         DX + Element->BoundingBox.X2,
                         DY + Element->BoundingBox.Y1);
      hid_draw_line (gc, DX + Element->BoundingBox.X2,
                         DY + Element->BoundingBox.Y1,
                         DX + Element->BoundingBox.X1,
                         DY + Element->BoundingBox.Y1);
    }
  else
    {
      ELEMENTLINE_LOOP (Element);
      {
        hid_draw_line (gc, DX + line->Point1.X,
                           DY + line->Point1.Y,
                           DX + line->Point2.X,
                           DY + line->Point2.Y);
      }
      END_LOOP;

      /* arc coordinates and angles have to be converted to X11 notation */
      ARC_LOOP (Element);
      {
        hid_draw_arc (gc, DX + arc->X,
                          DY + arc->Y,
                          arc->Width, arc->Height, arc->StartAngle, arc->Delta);
      }
      END_LOOP;
    }
  /* pin coordinates and angles have to be converted to X11 notation */
  PIN_LOOP (Element);
  {
    thindraw_moved_pv (gc, pin, DX, DY);
  }
  END_LOOP;

  /* pads */
  PAD_LOOP (Element);
  {
    if (PCB->InvisibleObjectsOn ||
        (TEST_FLAG (ONSOLDERFLAG, pad) != 0) == Settings.ShowBottomSide)
      {
        /* Make a copy of the pad structure, moved to the correct position */
        PadType moved_pad = *pad;
        moved_pad.Point1.X += DX; moved_pad.Point1.Y += DY;
        moved_pad.Point2.X += DX; moved_pad.Point2.Y += DY;

        hid_draw_thin_pcb_pad (gc, &moved_pad, false, false);
      }
  }
  END_LOOP;
  /* mark */
  hid_draw_line (gc, Element->MarkX + DX - EMARK_SIZE,
                     Element->MarkY + DY,
                     Element->MarkX + DX,
                     Element->MarkY + DY - EMARK_SIZE);
  hid_draw_line (gc, Element->MarkX + DX + EMARK_SIZE,
                     Element->MarkY + DY,
                     Element->MarkX + DX,
                     Element->MarkY + DY - EMARK_SIZE);
  hid_draw_line (gc, Element->MarkX + DX - EMARK_SIZE,
                     Element->MarkY + DY,
                     Element->MarkX + DX,
                     Element->MarkY + DY + EMARK_SIZE);
  hid_draw_line (gc, Element->MarkX + DX + EMARK_SIZE,
                     Element->MarkY + DY,
                     Element->MarkX + DX,
                     Element->MarkY + DY + EMARK_SIZE);
}

/*!
 * \brief Draws all visible and attached objects of the pastebuffer.
 */
static void
XORDrawBuffer (hidGC gc, BufferType *Buffer)
{
  Cardinal i;
  Coord x, y;

  /* set offset */
  x = Crosshair.X - Buffer->X;
  y = Crosshair.Y - Buffer->Y;

  /* draw all visible layers */
  for (i = 0; i < max_copper_layer + EXTRA_LAYERS; i++)
    if (PCB->Data->Layer[i].On)
      {
	LayerType *layer = &Buffer->Data->Layer[i];

	LINE_LOOP (layer);
	{
/*
				XORDrawAttachedLine(x +line->Point1.X,
					y +line->Point1.Y, x +line->Point2.X,
					y +line->Point2.Y, line->Thickness);
*/
	hid_draw_line (gc, x + line->Point1.X, y + line->Point1.Y,
	                   x + line->Point2.X, y + line->Point2.Y);
	}
	END_LOOP;
	ARC_LOOP (layer);
	{
	  hid_draw_arc (gc, x + arc->X,
	                    y + arc->Y,
	                    arc->Width, arc->Height, arc->StartAngle, arc->Delta);
	}
	END_LOOP;
	TEXT_LOOP (layer);
	{
	  BoxType *box = &text->BoundingBox;
	  hid_draw_rect (gc, x + box->X1, y + box->Y1, x + box->X2, y + box->Y2);
	}
	END_LOOP;
	/* the tmp polygon has n+1 points because the first
	 * and the last one are set to the same coordinates
	 */
	POLYGON_LOOP (layer);
	{
	  XORPolygon (gc, polygon, x, y, 0);
	}
	END_LOOP;
      }

  /* draw elements if visible */
  if (PCB->PinOn && PCB->ElementOn)
    ELEMENT_LOOP (Buffer->Data);
  {
    if (FRONT (element) || PCB->InvisibleObjectsOn)
      XORDrawElement (gc, element, x, y);
  }
  END_LOOP;

  /* and the vias */
  if (PCB->ViaOn)
    VIA_LOOP (Buffer->Data);
  {
    thindraw_moved_pv (gc, via, x, y);
  }
  END_LOOP;
}

/*!
 * \brief Draws the rubberband to insert points into polygons/lines/...
 */
static void
XORDrawInsertPointObject (hidGC gc)
{
  LineType *line = (LineType *) Crosshair.AttachedObject.Ptr2;
  PointType *point = (PointType *) Crosshair.AttachedObject.Ptr3;

  if (Crosshair.AttachedObject.Type != NO_TYPE)
    {
      hid_draw_line (gc, point->X, point->Y, line->Point1.X, line->Point1.Y);
      hid_draw_line (gc, point->X, point->Y, line->Point2.X, line->Point2.Y);
    }
}

/*!
 * \brief Determine the intersection point of two line segments
 * Return FALSE if the lines don't intersect
 *
 * Based upon code from http://paulbourke.net/geometry/lineline2d/
 */
#define EPS 1e-6
static bool
line_line_intersect (double x1, double y1, double x2, double y2,
                     double x3, double y3, double x4, double y4,
                     double *x, double *y, double *multiplier)
{
  double mua,mub;
  double denom,numera,numerb;

  if (x != NULL) *x = 0.0;
  if (y != NULL) *y = 0.0;
  if (multiplier != NULL) *multiplier = 0.0;

  denom  = (y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1);
  numera = (x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3);
  numerb = (x2 - x1) * (y1 - y3) - (y2 - y1) * (x1 - x3);

  /* Are the lines coincident or parallel? */
  if (fabs (denom) < EPS)
    {
      /* Are the line coincident? */
      if (fabs (numera) < EPS && fabs (numerb) < EPS)
        {
          if (x != NULL) *x = (x1 + x2) / 2;
          if (y != NULL) *y = (y1 + y2) / 2;
          if (multiplier != NULL) *multiplier = 0.5;
          return true;
        }
      /* The line parallel */
      return false;
    }

  /* Is the intersection along the the segments */
  mua = numera / denom;
  mub = numerb / denom;
//  if (mua < 0.0 || 1.0 < mua || mub < 0.0 || 1.0 < mub)
//    return false;

  if (x != NULL) *x = x1 + mua * (x2 - x1);
  if (y != NULL) *y = y1 + mua * (y2 - y1);
  if (multiplier != NULL) *multiplier = mua;

  if (mua < 0.0 || 1.0 < mua || mub < 0.0 || 1.0 < mub)
    return false;
  return true;
}

/*!
 * \brief Draws the attached object while in MOVE_MODE or COPY_MODE.
 */
static void
XORDrawMoveOrCopyObject (hidGC gc)
{
  RubberbandType *ptr;
  Cardinal i;
  Coord dx = Crosshair.X - Crosshair.AttachedObject.X,
    dy = Crosshair.Y - Crosshair.AttachedObject.Y;

#if 0
  /* Kludgy demo */
  if (dx > 500000)
    dx =   500000;
  dy = dx;
  /* Kludgy demo */
#endif

  switch (Crosshair.AttachedObject.Type)
    {
    case VIA_TYPE:
      {
        PinType *via = (PinType *) Crosshair.AttachedObject.Ptr1;
        thindraw_moved_pv (gc, via, dx, dy);
        break;
      }

    case LINE_TYPE:
      {
//      while (0) {
        LineType *moving_line = Crosshair.AttachedObject.Ptr2;
        bool first_pass = true;
        double min_multiplier = 0.0;
        double max_multiplier = 0.0;
        bool found_line_at_0_end = false;
        bool found_line_at_1_end = false;

        /* draw the attached rubberband lines too */
        i = Crosshair.AttachedObject.RubberbandN;
        ptr = Crosshair.AttachedObject.Rubberband;

        for (; i; ptr++, i--)
          {
            if (!TEST_FLAG (VIAFLAG, ptr->Line) &&
                TEST_FLAG (RUBBERENDFLAG, ptr->Line) &&
                ptr->Layer != NULL)
              {
                double multiplier;

                line_line_intersect (moving_line->Point1.X, moving_line->Point1.Y,
                                     moving_line->Point2.X, moving_line->Point2.Y,
                                     ptr->Line->Point1.X,   ptr->Line->Point1.Y,
                                     ptr->Line->Point2.X,   ptr->Line->Point2.Y,
                                     NULL, NULL, &multiplier);

                if (fabs (multiplier - 0.0) < EPS)
                  found_line_at_0_end = true;

                if (fabs (multiplier - 1.0) < EPS)
                  found_line_at_1_end = true;

                line_line_intersect (moving_line->Point1.X + dx, moving_line->Point1.Y + dy,
                                     moving_line->Point2.X + dx, moving_line->Point2.Y + dy,
                                     ptr->Line->Point1.X,        ptr->Line->Point1.Y,
                                     ptr->Line->Point2.X,        ptr->Line->Point2.Y,
                                     NULL, NULL, &multiplier);
                if (first_pass)
                  {
                    min_multiplier = multiplier;
                    max_multiplier = multiplier;
                    first_pass = false;
                  }
                min_multiplier = MIN (min_multiplier, multiplier);
                max_multiplier = MAX (max_multiplier, multiplier);
              }
          }


        if (!found_line_at_0_end)
          min_multiplier = MIN (min_multiplier, 0.0);

        if (!found_line_at_1_end)
          max_multiplier = MAX (max_multiplier, 1.0);

        /* If no constraints from the rubber band lines, then keep the old endpoints */
        if (min_multiplier == max_multiplier)
          {
            /* TODO: Restore free end-point? */
          }
#if 0
        if (!set_min)
          min_multiplier = 0.0;

        if (!set_max)
          max_multiplier = 1.0;
#endif

        XORDrawAttachedLine (gc,
                             moving_line->Point1.X + dx + min_multiplier * (moving_line->Point2.X - moving_line->Point1.X),
                             moving_line->Point1.Y + dy + min_multiplier * (moving_line->Point2.Y - moving_line->Point1.Y),
                             moving_line->Point1.X + dx + max_multiplier * (moving_line->Point2.X - moving_line->Point1.X),
                             moving_line->Point1.Y + dy + max_multiplier * (moving_line->Point2.Y - moving_line->Point1.Y),
                             moving_line->Thickness);

        break;

      }

    case ARC_TYPE:
      {
	ArcType *Arc = (ArcType *) Crosshair.AttachedObject.Ptr2;

	hid_draw_arc (gc, Arc->X + dx,
	                  Arc->Y + dy,
	                  Arc->Width, Arc->Height, Arc->StartAngle, Arc->Delta);
	break;
      }

    case POLYGON_TYPE:
      {
	PolygonType *polygon =
	  (PolygonType *) Crosshair.AttachedObject.Ptr2;

	/* the tmp polygon has n+1 points because the first
	 * and the last one are set to the same coordinates
	 */
	XORPolygon (gc, polygon, dx, dy, 0);
	break;
      }

    case LINEPOINT_TYPE:
      {
	LineType *line;
	PointType *point;

	line = (LineType *) Crosshair.AttachedObject.Ptr2;
	point = (PointType *) Crosshair.AttachedObject.Ptr3;
	if (point == &line->Point1)
	  XORDrawAttachedLine (gc, point->X + dx, point->Y + dy,
	                       line->Point2.X, line->Point2.Y, line->Thickness);
	else
	  XORDrawAttachedLine (gc, point->X + dx, point->Y + dy,
			       line->Point1.X, line->Point1.Y, line->Thickness);
	break;
      }

    case POLYGONPOINT_TYPE:
      {
	PolygonType *polygon;
	PointType *point;
	Cardinal point_idx, prev, next;

	polygon = (PolygonType *) Crosshair.AttachedObject.Ptr2;
	point = (PointType *) Crosshair.AttachedObject.Ptr3;
	point_idx = polygon_point_idx (polygon, point);

	/* get previous and following point */
	prev = prev_contour_point (polygon, point_idx);
	next = next_contour_point (polygon, point_idx);

	/* draw the two segments */
	hid_draw_line (gc, polygon->Points[prev].X, polygon->Points[prev].Y,
	                   point->X + dx, point->Y + dy);
	hid_draw_line (gc, point->X + dx, point->Y + dy,
	                   polygon->Points[next].X, polygon->Points[next].Y);
	break;
      }

    case ELEMENTNAME_TYPE:
      {
	/* locate the element "mark" and draw an association line from crosshair to it */
	ElementType *element =
	  (ElementType *) Crosshair.AttachedObject.Ptr1;

	hid_draw_line (gc, element->MarkX, element->MarkY,
	                   Crosshair.X, Crosshair.Y);
	/* fall through to move the text as a box outline */
      }
    case TEXT_TYPE:
      {
	TextType *text = (TextType *) Crosshair.AttachedObject.Ptr2;
	BoxType *box = &text->BoundingBox;
	hid_draw_rect (gc, box->X1 + dx, box->Y1 + dy, box->X2 + dx, box->Y2 + dy);
	break;
      }

      /* pin/pad movements result in moving an element */
    case PAD_TYPE:
    case PIN_TYPE:
    case ELEMENT_TYPE:
      XORDrawElement (gc, (ElementType *) Crosshair.AttachedObject.Ptr2, dx, dy);
      break;
    }

  /* draw the attached rubberband lines too */
  i = Crosshair.AttachedObject.RubberbandN;
  ptr = Crosshair.AttachedObject.Rubberband;
//  while (i)
  for (; i; ptr++, i--)
    {
      PointType *point1, *point2;

      /* this is a rat going to a polygon.  do not draw for rubberband */;
      if (TEST_FLAG (VIAFLAG, ptr->Line))
        continue;

      if (TEST_FLAG (RUBBERENDFLAG, ptr->Line))
	{
          if (Crosshair.AttachedObject.Type == LINE_TYPE)
            {
              LineType *moving_line = Crosshair.AttachedObject.Ptr2;
              PointType *fixed_point;
              double x, y;

              line_line_intersect (moving_line->Point1.X + dx, moving_line->Point1.Y + dy,
                                   moving_line->Point2.X + dx, moving_line->Point2.Y + dy,
                                   ptr->Line->Point1.X,        ptr->Line->Point1.Y,
                                   ptr->Line->Point2.X,        ptr->Line->Point2.Y,
                                   &x,                         &y,
                                   NULL);

              fixed_point = (ptr->MovedPoint == &ptr->Line->Point1) ?
                              &ptr->Line->Point2 : &ptr->Line->Point1;

              XORDrawAttachedLine (gc, fixed_point->X, fixed_point->Y, x, y,
                                   ptr->Line->Thickness);
            }
          else
            {
              /* TODO: Project the extension or contraction of ptr->Line, such that it
               *       intersects with the extended or contracted version of the
               *       line(s) being moved
               */

              /* 'point1' is always the fix-point */
              if (ptr->MovedPoint == &ptr->Line->Point1)
                {
                  point1 = &ptr->Line->Point2;
                  point2 = &ptr->Line->Point1;
                }
              else
                {
                  point1 = &ptr->Line->Point1;
                  point2 = &ptr->Line->Point2;
                }
              XORDrawAttachedLine (gc, point1->X,      point1->Y,
                                       point2->X + dx, point2->Y + dy,
                                   ptr->Line->Thickness);
            }
        }
#if 1
      else if (ptr->MovedPoint == &ptr->Line->Point1)
        XORDrawAttachedLine (gc,
                             ptr->Line->Point1.X + dx,
                             ptr->Line->Point1.Y + dy,
                             ptr->Line->Point2.X + dx,
                             ptr->Line->Point2.Y + dy, ptr->Line->Thickness);
#endif
    }
}

/*!
 * \brief Draws additional stuff that follows the crosshair.
 */
void
DrawAttached (hidGC gc)
{
  hid_draw_set_color (gc, Settings.CrosshairColor);
  hid_draw_set_draw_xor (gc, 1);
  hid_draw_set_line_cap (gc, Trace_Cap);
  hid_draw_set_line_width (gc, 1);

  switch (Settings.Mode)
    {
    case VIA_MODE:
      {
        /* Make a dummy via structure to draw from */
        PinType via;
        via.X = Crosshair.X;
        via.Y = Crosshair.Y;
        via.Thickness = Settings.ViaThickness;
        via.Clearance = 2 * Settings.Keepaway;
        via.DrillingHole = Settings.ViaDrillingHole;
        via.Mask = 0;
        via.Flags = NoFlags ();

        hid_draw_thin_pcb_pv (gc, gc, &via, true, false);

        if (TEST_FLAG (SHOWDRCFLAG, PCB))
          {
            Coord mask_r = Settings.ViaThickness / 2 + PCB->Bloat;
            hid_draw_set_color (gc, Settings.CrossColor);
            hid_draw_set_line_cap (gc, Round_Cap);
            hid_draw_set_line_width (gc, 0);
            hid_draw_arc (gc, via.X, via.Y, mask_r, mask_r, 0, 360);
            hid_draw_set_color (gc, Settings.CrosshairColor);
          }
        break;
      }

      /* the attached line is used by both LINEMODE, POLYGON_MODE and POLYGONHOLE_MODE*/
    case POLYGON_MODE:
    case POLYGONHOLE_MODE:
      /* draw only if starting point is set */
      if (Crosshair.AttachedLine.State != STATE_FIRST)
        hid_draw_line (gc, Crosshair.AttachedLine.Point1.X,
                           Crosshair.AttachedLine.Point1.Y,
                           Crosshair.AttachedLine.Point2.X,
                           Crosshair.AttachedLine.Point2.Y);

      /* draw attached polygon only if in POLYGON_MODE or POLYGONHOLE_MODE */
      if (Crosshair.AttachedPolygon.PointN > 1)
	{
	  XORPolygon (gc, &Crosshair.AttachedPolygon, 0, 0, 1);
	}
      break;

    case ARC_MODE:
      if (Crosshair.AttachedBox.State != STATE_FIRST)
	{
	  XORDrawAttachedArc (gc, Settings.LineThickness);
	  if (TEST_FLAG (SHOWDRCFLAG, PCB))
	    {
	      hid_draw_set_color (gc, Settings.CrossColor);
	      XORDrawAttachedArc (gc, Settings.LineThickness + 2 * PCB->Bloat);
	      hid_draw_set_color (gc, Settings.CrosshairColor);
	    }

	}
      break;

    case LINE_MODE:
      /* draw only if starting point exists and the line has length */
      if (Crosshair.AttachedLine.State != STATE_FIRST &&
	  Crosshair.AttachedLine.draw)
	{
	  XORDrawAttachedLine (gc, Crosshair.AttachedLine.Point1.X,
	                           Crosshair.AttachedLine.Point1.Y,
	                           Crosshair.AttachedLine.Point2.X,
	                           Crosshair.AttachedLine.Point2.Y,
	                           PCB->RatDraw ? 10 : Settings.LineThickness);
	  /* draw two lines ? */
	  if (PCB->Clipping && !TEST_FLAG (ALLDIRECTIONFLAG, PCB))
	    XORDrawAttachedLine (gc, Crosshair.AttachedLine.Point2.X,
	                             Crosshair.AttachedLine.Point2.Y,
	                             Crosshair.AttachedLine.Point3.X,
	                             Crosshair.AttachedLine.Point3.Y,
	                         PCB->RatDraw ? 10 : Settings.LineThickness);
	  if (TEST_FLAG (SHOWDRCFLAG, PCB))
	    {
	      hid_draw_set_color (gc, Settings.CrossColor);
	      XORDrawAttachedLine (gc, Crosshair.AttachedLine.Point1.X,
	                               Crosshair.AttachedLine.Point1.Y,
	                           Crosshair.AttachedLine.Point2.X,
	                           Crosshair.AttachedLine.Point2.Y,
	                           PCB->RatDraw ? 10 : Settings.LineThickness
	                           + 2 * PCB->Bloat);
	      if (PCB->Clipping)
		XORDrawAttachedLine (gc, Crosshair.AttachedLine.Point2.X,
		                         Crosshair.AttachedLine.Point2.Y,
		                         Crosshair.AttachedLine.Point3.X,
		                         Crosshair.AttachedLine.Point3.Y,
		                     PCB->RatDraw ? 10 : Settings.
		                     LineThickness + 2 * PCB->Bloat);
	      hid_draw_set_color (gc, Settings.CrosshairColor);
	    }
	}
      break;

    case PASTEBUFFER_MODE:
      XORDrawBuffer (gc, PASTEBUFFER);
      break;

    case COPY_MODE:
    case MOVE_MODE:
      XORDrawMoveOrCopyObject (gc);
      break;

    case INSERTPOINT_MODE:
      XORDrawInsertPointObject (gc);
      break;
    }

  /* an attached box does not depend on a special mode */
  if (Crosshair.AttachedBox.State == STATE_SECOND ||
      Crosshair.AttachedBox.State == STATE_THIRD)
    {
      Coord x1, y1, x2, y2;

      x1 = Crosshair.AttachedBox.Point1.X;
      y1 = Crosshair.AttachedBox.Point1.Y;
      x2 = Crosshair.AttachedBox.Point2.X;
      y2 = Crosshair.AttachedBox.Point2.Y;
      hid_draw_rect (gc, x1, y1, x2, y2);
    }
}


/*!
 * \brief Draw the marker position.
 */
void
DrawMark (hidGC gc)
{
  hid_draw_set_color (gc, Settings.CrosshairColor);
  hid_draw_set_draw_xor (gc, 1);
  hid_draw_set_line_cap (gc, Trace_Cap);
  hid_draw_set_line_width (gc, 1);

  /* Mark is not drawn when it is not set */
  if (!Marked.status)
    return;

  hid_draw_line (gc, Marked.X - MARK_SIZE, Marked.Y - MARK_SIZE,
                     Marked.X + MARK_SIZE, Marked.Y + MARK_SIZE);
  hid_draw_line (gc, Marked.X + MARK_SIZE, Marked.Y - MARK_SIZE,
                     Marked.X - MARK_SIZE, Marked.Y + MARK_SIZE);
}

/*!
 * \brief Returns the nearest grid-point to the given Coord.
 */
Coord
GridFit (Coord x, Coord grid_spacing, Coord grid_offset)
{
  x -= grid_offset;
  x = grid_spacing * round ((double) x / grid_spacing);
  x += grid_offset;
  return x;
}


/*!
 * \brief Notify the GUI that data relating to the crosshair is being
 * changed.
 *
 * The argument passed is false to notify "changes are about to happen",
 * and true to notify "changes have finished".
 *
 * Each call with a 'false' parameter must be matched with a following one
 * with a 'true' parameter. Unmatched 'true' calls are currently not permitted,
 * but might be allowed in the future.
 *
 * GUIs should not complain if they receive extra calls with 'true' as parameter.
 * They should initiate a redraw of the crosshair attached objects - which may
 * (if necessary) mean repainting the whole screen if the GUI hasn't tracked the
 * location of existing attached drawing.
 */
void
notify_crosshair_change (bool changes_complete)
{
  if (gui->notify_crosshair_change)
    gui->notify_crosshair_change (changes_complete);
}


/*!
 * \brief Notify the GUI that data relating to the mark is being changed.
 *
 * The argument passed is false to notify "changes are about to happen",
 * and true to notify "changes have finished".
 *
 * Each call with a 'false' parameter must be matched with a following one
 * with a 'true' parameter. Unmatched 'true' calls are currently not permitted,
 * but might be allowed in the future.
 *
 * GUIs should not complain if they receive extra calls with 'true' as parameter.
 * They should initiate a redraw of the mark - which may (if necessary) mean
 * repainting the whole screen if the GUI hasn't tracked the mark's location.
 */
void
notify_mark_change (bool changes_complete)
{
  if (gui->notify_mark_change)
    gui->notify_mark_change (changes_complete);
}


/*!
 * \brief Convenience for plugins using the old {Hide,Restore}Crosshair
 * API.
 *
 * This links up to notify the GUI of the expected changes using the new APIs.
 *
 * Use of this old API is deprecated, as the names don't necessarily reflect
 * what all GUIs may do in response to the notifications. Keeping these APIs
 * is aimed at easing transition to the newer API, they will emit a harmless
 * warning at the time of their first use.
 *
 */
void
HideCrosshair (void)
{
  static bool warned_old_api = false;
  if (!warned_old_api)
    {
      Message (_("WARNING: A plugin is using the deprecated API HideCrosshair().\n"
                 "         This API may be removed in a future release of PCB.\n"));
      warned_old_api = true;
    }

  notify_crosshair_change (false);
  notify_mark_change (false);
}

void
RestoreCrosshair (void)
{
  static bool warned_old_api = false;
  if (!warned_old_api)
    {
      Message (_("WARNING: A plugin is using the deprecated API RestoreCrosshair().\n"
                 "         This API may be removed in a future release of PCB.\n"));
      warned_old_api = true;
    }

  notify_crosshair_change (true);
  notify_mark_change (true);
}

/*!
 * \brief Returns the square of the given number.
 */
static double
square (double x)
{
  return x * x;
}

static double
crosshair_sq_dist (CrosshairType *crosshair, Coord x, Coord y)
{
  return square (x - crosshair->X) + square (y - crosshair->Y);
}

struct snap_data {
  CrosshairType *crosshair;
  double nearest_sq_dist;
  bool nearest_is_grid;
  Coord x, y;
};

/*!
 * \brief Snap to a given location if it is the closest thing we found
 * so far.
 *
 * If "prefer_to_grid" is set, the passed location will take preference
 * over a closer grid points we already snapped to UNLESS the user is
 * pressing the SHIFT key. If the SHIFT key is pressed, the closest object
 * (including grid points), is always preferred.
 */
static void
check_snap_object (struct snap_data *snap_data, Coord x, Coord y,
                   bool prefer_to_grid)
{
  double sq_dist;

  sq_dist = crosshair_sq_dist (snap_data->crosshair, x, y);
  if (sq_dist <= snap_data->nearest_sq_dist ||
      (prefer_to_grid && snap_data->nearest_is_grid && !gui->shift_is_pressed()))
    {
      snap_data->x = x;
      snap_data->y = y;
      snap_data->nearest_sq_dist = sq_dist;
      snap_data->nearest_is_grid = false;
    }
}

static void
check_snap_offgrid_line (struct snap_data *snap_data,
                         Coord nearest_grid_x,
                         Coord nearest_grid_y)
{
  void *ptr1, *ptr2, *ptr3;
  int ans;
  LineType *line;
  Coord try_x, try_y;
  double dx, dy;
  double dist;

  if (!TEST_FLAG (SNAPPINFLAG, PCB))
    return;

  /* Code to snap at some sensible point along a line */
  /* Pick the nearest grid-point in the x or y direction
   * to align with, then adjust until we hit the line
   */
  ans = SearchObjectByLocation (LINE_TYPE, &ptr1, &ptr2, &ptr3,
                                Crosshair.X, Crosshair.Y, PCB->Grid / 2);


  if (ans == NO_TYPE)
    return;

  line = (LineType *)ptr2;

  /* Allow snapping to off-grid lines when drawing new lines (on
   * the same layer), and when moving a line end-point
   * (but don't snap to the same line)
   */
  if (!(Settings.Mode == LINE_MODE && CURRENT == ptr1) && /* Snap in line mode when current layer matches */
      !(Settings.Mode == MOVE_MODE &&                     /* Snap in move mode when...                    */
        Crosshair.AttachedObject.Ptr1 == ptr1 &&          /* the snapping object is on the same layer... */
        Crosshair.AttachedObject.Type == LINEPOINT_TYPE && /* and the point we're moving is a line end-point */
       1) && // Crosshair.AttachedObject.Ptr2 != line) &&         /* and we're not snapping to the same line */
      (Settings.Mode == LINE_MODE || Settings.Mode == MOVE_MODE)) /* Only restrict line and move modes */
    return;

#if 0
  if ((Settings.Mode != LINE_MODE || CURRENT != ptr1) &&
      (Settings.Mode != MOVE_MODE ||
       Crosshair.AttachedObject.Ptr1 != ptr1 ||
       Crosshair.AttachedObject.Type != LINEPOINT_TYPE ||
       Crosshair.AttachedObject.Ptr2 == line))
    return;
#endif

  dx = line->Point2.X - line->Point1.X;
  dy = line->Point2.Y - line->Point1.Y;

  /* Try snapping along the X axis */
  if (dy != 0.)
    {
      /* Move in the X direction until we hit the line */
      try_x = (nearest_grid_y - line->Point1.Y) / dy * dx + line->Point1.X;
      try_y = nearest_grid_y;
      check_snap_object (snap_data, try_x, try_y, true);
    }

  /* Try snapping along the Y axis */
  if (dx != 0.)
    {
      try_x = nearest_grid_x;
      try_y = (nearest_grid_x - line->Point1.X) / dx * dy + line->Point1.Y;
      check_snap_object (snap_data, try_x, try_y, true);
    }

  if (dx != dy) /* If line not parallel with dX = dY direction.. */
    {
      /* Try snapping diagonally towards the line in the dX = dY direction */

      if (dy == 0)
        dist = line->Point1.Y - nearest_grid_y;
      else
        dist = ((line->Point1.X - nearest_grid_x) -
                (line->Point1.Y - nearest_grid_y) * dx / dy) / (1 - dx / dy);

      try_x = nearest_grid_x + dist;
      try_y = nearest_grid_y + dist;

      check_snap_object (snap_data, try_x, try_y, true);
    }

  if (dx != -dy) /* If line not parallel with dX = -dY direction.. */
    {
      /* Try snapping diagonally towards the line in the dX = -dY direction */

      if (dy == 0)
        dist = nearest_grid_y - line->Point1.Y;
      else
        dist = ((line->Point1.X - nearest_grid_x) -
                (line->Point1.Y - nearest_grid_y) * dx / dy) / (1 + dx / dy);

      try_x = nearest_grid_x + dist;
      try_y = nearest_grid_y - dist;

      check_snap_object (snap_data, try_x, try_y, true);
    }
}

/*!
 * \brief Recalculates the passed coordinates to fit the current grid
 * setting.
 */
void
FitCrosshairIntoGrid (Coord X, Coord Y)
{
  Coord nearest_grid_x, nearest_grid_y;
  void *ptr1, *ptr2, *ptr3;
  struct snap_data snap_data;
  int ans;

#if 0
  Coord old_x, old_y;

  old_x = Crosshair.X;
  old_y = Crosshair.Y;
#endif

  Crosshair.X = CLAMP (X, Crosshair.MinX, Crosshair.MaxX);
  Crosshair.Y = CLAMP (Y, Crosshair.MinY, Crosshair.MaxY);

  if (PCB->RatDraw)
    {
      nearest_grid_x = -MIL_TO_COORD (6);
      nearest_grid_y = -MIL_TO_COORD (6);
    }
  else
    {
      nearest_grid_x = GridFit (Crosshair.X, PCB->Grid, PCB->GridOffsetX);
      nearest_grid_y = GridFit (Crosshair.Y, PCB->Grid, PCB->GridOffsetY);

      if (Marked.status && TEST_FLAG (ORTHOMOVEFLAG, PCB))
	{
	  Coord dx = Crosshair.X - Marked.X;
	  Coord dy = Crosshair.Y - Marked.Y;
	  if (ABS (dx) > ABS (dy))
	    nearest_grid_y = Marked.Y;
	  else
	    nearest_grid_x = Marked.X;
	}

    }

  snap_data.crosshair = &Crosshair;
  snap_data.nearest_is_grid = true;
  snap_data.x = nearest_grid_x;
  snap_data.y = nearest_grid_y;
#if 0
  snap_data.nearest_sq_dist = crosshair_sq_dist (&Crosshair, snap_data.x, snap_data.y);

  if (snap_data.nearest_sq_dist > PCB->Grid / 3 * PCB->Grid / 3)
    {
      snap_data.x = old_x;
      snap_data.y = old_y;
    }
#endif

#if 0
  if (labs (nearest_grid_x - Crosshair.X) > PCB->Grid / 3)
    snap_data.x = old_x;

  if (labs (nearest_grid_y - Crosshair.Y) > PCB->Grid / 3)
    snap_data.y = old_y;
#endif

  snap_data.nearest_sq_dist = crosshair_sq_dist (&Crosshair, snap_data.x, snap_data.y);

  ans = NO_TYPE;
  if (!PCB->RatDraw)
    ans = SearchObjectByLocation (ELEMENT_TYPE, &ptr1, &ptr2, &ptr3,
                                  Crosshair.X, Crosshair.Y, PCB->Grid / 2);

  if (ans & ELEMENT_TYPE)
    {
      ElementType *el = (ElementType *) ptr1;
      check_snap_object (&snap_data, el->MarkX, el->MarkY, false);
    }

  ans = NO_TYPE;
  if (PCB->RatDraw || TEST_FLAG (SNAPPINFLAG, PCB))
    ans = SearchObjectByLocation (PAD_TYPE, &ptr1, &ptr2, &ptr3,
                                  Crosshair.X, Crosshair.Y, 0);

  /* Avoid self-snapping when moving */
  if (ans != NO_TYPE &&
      Settings.Mode == MOVE_MODE &&
      Crosshair.AttachedObject.Type == ELEMENT_TYPE &&
      ptr1 == Crosshair.AttachedObject.Ptr1)
    ans = NO_TYPE;

  if (ans != NO_TYPE &&
      ( Settings.Mode == LINE_MODE ||
       (Settings.Mode == MOVE_MODE &&
        Crosshair.AttachedObject.Type == LINEPOINT_TYPE)))
    {
      PadType *pad = (PadType *) ptr2;
      LayerType *desired_layer;
      Cardinal desired_group;
      Cardinal bottom_group, top_group;
      int found_our_layer = false;

      desired_layer = CURRENT;
      if (Settings.Mode == MOVE_MODE &&
          Crosshair.AttachedObject.Type == LINEPOINT_TYPE)
        {
          desired_layer = (LayerType *)Crosshair.AttachedObject.Ptr1;
        }

      /* find layer groups of the top and bottom sides */
      top_group = GetLayerGroupNumberBySide (TOP_SIDE);
      bottom_group = GetLayerGroupNumberBySide (BOTTOM_SIDE);
      desired_group = TEST_FLAG (ONSOLDERFLAG, pad) ? bottom_group : top_group;

      GROUP_LOOP (PCB->Data, desired_group);
      {
        if (layer == desired_layer)
          {
            found_our_layer = true;
            break;
          }
      }
      END_LOOP;

      if (found_our_layer == false)
        ans = NO_TYPE;
    }

  if (ans != NO_TYPE)
    {
      PadType *pad = (PadType *)ptr2;
      check_snap_object (&snap_data, pad->Point1.X + (pad->Point2.X - pad->Point1.X) / 2,
                                     pad->Point1.Y + (pad->Point2.Y - pad->Point1.Y) / 2,
                         true);
    }

  ans = NO_TYPE;
  if (PCB->RatDraw || TEST_FLAG (SNAPPINFLAG, PCB))
    ans = SearchObjectByLocation (PIN_TYPE, &ptr1, &ptr2, &ptr3,
                                  Crosshair.X, Crosshair.Y, 0);

  /* Avoid self-snapping when moving */
  if (ans != NO_TYPE &&
      Settings.Mode == MOVE_MODE &&
      Crosshair.AttachedObject.Type == ELEMENT_TYPE &&
      ptr1 == Crosshair.AttachedObject.Ptr1)
    ans = NO_TYPE;

  if (ans != NO_TYPE)
    {
      PinType *pin = (PinType *)ptr2;
      check_snap_object (&snap_data, pin->X, pin->Y, true);
    }

  ans = NO_TYPE;
  if (TEST_FLAG (SNAPPINFLAG, PCB))
    ans = SearchObjectByLocation (VIA_TYPE, &ptr1, &ptr2, &ptr3,
                                  Crosshair.X, Crosshair.Y, 0);

  /* Avoid snapping vias to any other vias */
  if (Settings.Mode == MOVE_MODE &&
      Crosshair.AttachedObject.Type == VIA_TYPE &&
      (ans & PIN_TYPES))
    ans = NO_TYPE;

  if (ans != NO_TYPE)
    {
      PinType *pin = (PinType *)ptr2;
      check_snap_object (&snap_data, pin->X, pin->Y, true);
    }

  ans = NO_TYPE;
  if (TEST_FLAG (SNAPPINFLAG, PCB))
    ans = SearchObjectByLocation (LINEPOINT_TYPE | ARCPOINT_TYPE,
                                  &ptr1, &ptr2, &ptr3,
                                  Crosshair.X, Crosshair.Y, PCB->Grid / 2);

  if (ans != NO_TYPE)
    {
      PointType *pnt = (PointType *)ptr3;
      check_snap_object (&snap_data, pnt->X, pnt->Y, true);
    }

  check_snap_offgrid_line (&snap_data, nearest_grid_x, nearest_grid_y);

  ans = NO_TYPE;
  if (TEST_FLAG (SNAPPINFLAG, PCB))
    ans = SearchObjectByLocation (POLYGONPOINT_TYPE, &ptr1, &ptr2, &ptr3,
                                  Crosshair.X, Crosshair.Y, PCB->Grid / 2);

  if (ans != NO_TYPE)
    {
      PointType *pnt = (PointType *)ptr3;
      check_snap_object (&snap_data, pnt->X, pnt->Y, true);
    }

  if (snap_data.x >= 0 && snap_data.y >= 0)
    {
      Crosshair.X = snap_data.x;
      Crosshair.Y = snap_data.y;
    }

  if (Settings.Mode == ARROW_MODE)
    {
      ans = SearchObjectByLocation (LINEPOINT_TYPE | ARCPOINT_TYPE,
                                    &ptr1, &ptr2, &ptr3,
                                    Crosshair.X, Crosshair.Y, PCB->Grid / 2);
      if (ans != NO_TYPE && !TEST_FLAG(SELECTEDFLAG, (LineType *)ptr2)) {
        if (gui->endpoint_cursor != NULL)
          gui->endpoint_cursor ();
        goto common_tail;
      }

      if (TEST_FLAG (RUBBERBANDFLAG, PCB))
        {

          ans = SearchObjectByLocation (LINE_TYPE,
                                        &ptr1, &ptr2, &ptr3,
                                        Crosshair.X, Crosshair.Y, PCB->Grid / 2);
          if (ans != NO_TYPE) {
            double angle;
            int octant;
            int dir;
            int allowed_directions = 0;
            LineType *line = ptr2;

            /* XXX: FIX THIS FOR DEGENERATE (POINT) LINES */
            angle = atan2 (-(line->Point2.Y - line->Point1.Y),
                           line->Point2.X - line->Point1.X);
    //      octant = nearbyint (angle / M_PI * 4.) * M_PI / 4.; /* Round to multiples of 45 degrees */
            octant = lrint (angle / M_PI * 4.); /* Lies between -4 and 4 */
            dir = (octant + 8) % 4;

            /* Assume constraints are simple, and that we don't blank off motion directions,
             * this means the user can drag the line perpendicular to its axis
             */

            switch (dir) {
              case 0:
                allowed_directions |= ALLOWED_DIR_90_DEGREES;
                allowed_directions |= ALLOWED_DIR_270_DEGREES;
                break;
              case 1:
                allowed_directions |= ALLOWED_DIR_135_DEGREES;
                allowed_directions |= ALLOWED_DIR_315_DEGREES;
                break;
              case 2:
                allowed_directions |= ALLOWED_DIR_0_DEGREES;
                allowed_directions |= ALLOWED_DIR_180_DEGREES;
                break;
              case 3:
                allowed_directions |= ALLOWED_DIR_45_DEGREES;
                allowed_directions |= ALLOWED_DIR_225_DEGREES;
                break;
            }

            if (gui->grip_cursor != NULL)
              gui->grip_cursor (allowed_directions);
            goto common_tail;
          }
        }

    }
  if (gui->normal_cursor != NULL)
    gui->normal_cursor ();

common_tail:

  if (Settings.Mode == LINE_MODE
      && Crosshair.AttachedLine.State != STATE_FIRST
     )//      && TEST_FLAG (AUTODRCFLAG, PCB))
    EnforceLineDRC ();

  gui->set_crosshair (Crosshair.X, Crosshair.Y, HID_SC_DO_NOTHING);
}

/*!
 * \brief Move crosshair absolute.
 *
 * \return true if the crosshair was moved from its existing position.
 */
bool
MoveCrosshairAbsolute (Coord X, Coord Y)
{
  Coord old_x = Crosshair.X;
  Coord old_y = Crosshair.Y;

  FitCrosshairIntoGrid (X, Y);

  if (Crosshair.X != old_x || Crosshair.Y != old_y)
    {
      Coord new_x = Crosshair.X;
      Coord new_y = Crosshair.Y;

      /* back up to old position to notify the GUI
       * (which might want to erase the old crosshair) */
      Crosshair.X = old_x;
      Crosshair.Y = old_y;
      notify_crosshair_change (false); /* Our caller notifies when it has done */

      /* now move forward again */
      Crosshair.X = new_x;
      Crosshair.Y = new_y;
      return true;
    }
  return false;
}

/*!
 * \brief Sets the valid range for the crosshair cursor.
 */
void
SetCrosshairRange (Coord MinX, Coord MinY, Coord MaxX, Coord MaxY)
{
  Crosshair.MinX = MAX (0, MinX);
  Crosshair.MinY = MAX (0, MinY);
  Crosshair.MaxX = MIN (PCB->MaxWidth, MaxX);
  Crosshair.MaxY = MIN (PCB->MaxHeight, MaxY);

  /* force update of position */
  FitCrosshairIntoGrid (Crosshair.X, Crosshair.Y);
}

/*!
 * \brief Initializes crosshair stuff.
 *
 * Clears the struct, allocates to graphical contexts.
 */
void
InitCrosshair (void)
{
  /* set initial shape */
  Crosshair.shape = Basic_Crosshair_Shape;

  /* set default limits */
  Crosshair.MinX = Crosshair.MinY = 0;
  Crosshair.MaxX = PCB->MaxWidth;
  Crosshair.MaxY = PCB->MaxHeight;

  /* clear the mark */
  Marked.status = false;
}

/*!
 * \brief Exits crosshair routines, release GCs.
 */
void
DestroyCrosshair (void)
{
  FreePolygonMemory (&Crosshair.AttachedPolygon);
}
