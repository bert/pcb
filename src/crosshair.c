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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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

#include "buffer.h"
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

  gui->graphics->thindraw_pcb_pv (gc, gc, &moved_pv, true, false);
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
    gui->graphics->draw_line (gc, x1, y1, x2, y2);
    return;
  }

  /* first seg is drawn from x1, y1 with no rounding error due to n-1 == 0 */
  for (n = 1; n < segs; n += 2)
    gui->graphics->draw_line (gc,
                              x1 + (dx * (double) (n-1) / (double) segs),
                              y1 + (dy * (double) (n-1) / (double) segs),
                              x1 + (dx * (double) n / (double) segs),
                              y1 + (dy * (double) n / (double) segs));

  /* make sure the last segment is drawn properly to x2 and y2,
   * don't leave room for rounding errors. */
  gui->graphics->draw_line (gc,
                            x2 - (dx / (double) segs),
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
      gui->graphics->draw_line (gc,
                                polygon->Points[i].X + dx,
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
  gui->graphics->draw_arc (gc, arc.X, arc.Y, wy + wid, wy + wid, sa, dir);
  if (wid > pixel_slop)
    {
      gui->graphics->draw_arc (gc, arc.X, arc.Y, wy - wid, wy - wid, sa, dir);
      gui->graphics->draw_arc (gc, bx->X1, bx->Y1, wid, wid, sa,      -180 * SGN (dir));
      gui->graphics->draw_arc (gc, bx->X2, bx->Y2, wid, wid, sa + dir, 180 * SGN (dir));
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
  gui->graphics->draw_line (gc, x1 + ox, y1 + oy, x2 + ox, y2 + oy);
  if (abs (ox) >= pixel_slop || abs (oy) >= pixel_slop)
    {
      Angle angle = atan2 (dx, dy) * 57.295779;
      gui->graphics->draw_line (gc, x1 - ox, y1 - oy, x2 - ox, y2 - oy);
      gui->graphics->draw_arc (gc, x1, y1, thick / 2, thick / 2, angle - 180, 180);
      gui->graphics->draw_arc (gc, x2, y2, thick / 2, thick / 2, angle, 180);
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
      gui->graphics->draw_line (gc,
                                DX + Element->BoundingBox.X1,
                                DY + Element->BoundingBox.Y1,
                                DX + Element->BoundingBox.X1,
                                DY + Element->BoundingBox.Y2);
      gui->graphics->draw_line (gc,
                                DX + Element->BoundingBox.X1,
                                DY + Element->BoundingBox.Y2,
                                DX + Element->BoundingBox.X2,
                                DY + Element->BoundingBox.Y2);
      gui->graphics->draw_line (gc,
                                DX + Element->BoundingBox.X2,
                                DY + Element->BoundingBox.Y2,
                                DX + Element->BoundingBox.X2,
                                DY + Element->BoundingBox.Y1);
      gui->graphics->draw_line (gc,
                                DX + Element->BoundingBox.X2,
                                DY + Element->BoundingBox.Y1,
                                DX + Element->BoundingBox.X1,
                                DY + Element->BoundingBox.Y1);
    }
  else
    {
      ELEMENTLINE_LOOP (Element);
      {
        gui->graphics->draw_line (gc,
                                  DX + line->Point1.X,
                                  DY + line->Point1.Y,
                                  DX + line->Point2.X,
                                  DY + line->Point2.Y);
      }
      END_LOOP;

      /* arc coordinates and angles have to be converted to X11 notation */
      ARC_LOOP (Element);
      {
        gui->graphics->draw_arc (gc,
                                 DX + arc->X,
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

        gui->graphics->thindraw_pcb_pad (gc, &moved_pad, false, false);
      }
  }
  END_LOOP;
  /* mark */
  gui->graphics->draw_line (gc,
                            Element->MarkX + DX - EMARK_SIZE,
                            Element->MarkY + DY,
                            Element->MarkX + DX,
                            Element->MarkY + DY - EMARK_SIZE);
  gui->graphics->draw_line (gc,
                            Element->MarkX + DX + EMARK_SIZE,
                            Element->MarkY + DY,
                            Element->MarkX + DX,
                            Element->MarkY + DY - EMARK_SIZE);
  gui->graphics->draw_line (gc,
                            Element->MarkX + DX - EMARK_SIZE,
                            Element->MarkY + DY,
                            Element->MarkX + DX,
                            Element->MarkY + DY + EMARK_SIZE);
  gui->graphics->draw_line (gc,
                            Element->MarkX + DX + EMARK_SIZE,
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
  for (i = 0; i < max_copper_layer + SILK_LAYER; i++)
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
	gui->graphics->draw_line (gc,
	                          x + line->Point1.X, y + line->Point1.Y,
	                          x + line->Point2.X, y + line->Point2.Y);
	}
	END_LOOP;
	ARC_LOOP (layer);
	{
	  gui->graphics->draw_arc (gc,
	                           x + arc->X,
	                           y + arc->Y,
	                           arc->Width,
	                           arc->Height, arc->StartAngle, arc->Delta);
	}
	END_LOOP;
	TEXT_LOOP (layer);
	{
	  BoxType *box = &text->BoundingBox;
	  gui->graphics->draw_rect (gc,
	                            x + box->X1, y + box->Y1, x + box->X2, y + box->Y2);
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
      gui->graphics->draw_line (gc, point->X, point->Y, line->Point1.X, line->Point1.Y);
      gui->graphics->draw_line (gc, point->X, point->Y, line->Point2.X, line->Point2.Y);
    }
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
	LineType *line = (LineType *) Crosshair.AttachedObject.Ptr2;

	XORDrawAttachedLine (gc, line->Point1.X + dx, line->Point1.Y + dy,
	                         line->Point2.X + dx, line->Point2.Y + dy,
	                     line->Thickness);
	break;
      }

    case ARC_TYPE:
      {
	ArcType *Arc = (ArcType *) Crosshair.AttachedObject.Ptr2;

	gui->graphics->draw_arc (gc,
	                         Arc->X + dx,
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
	gui->graphics->draw_line (gc,
	                          polygon->Points[prev].X, polygon->Points[prev].Y,
	                          point->X + dx, point->Y + dy);
	gui->graphics->draw_line (gc,
	                          point->X + dx, point->Y + dy,
	                          polygon->Points[next].X, polygon->Points[next].Y);
	break;
      }

    case ELEMENTNAME_TYPE:
      {
	/* locate the element "mark" and draw an association line from crosshair to it */
	ElementType *element =
	  (ElementType *) Crosshair.AttachedObject.Ptr1;

	gui->graphics->draw_line (gc,
	                          element->MarkX,
	                          element->MarkY, Crosshair.X, Crosshair.Y);
	/* fall through to move the text as a box outline */
      }
    case TEXT_TYPE:
      {
	TextType *text = (TextType *) Crosshair.AttachedObject.Ptr2;
	BoxType *box = &text->BoundingBox;
	gui->graphics->draw_rect (gc,
	                          box->X1 + dx,
	                          box->Y1 + dy, box->X2 + dx, box->Y2 + dy);
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
  while (i)
    {
      PointType *point1, *point2;

      if (TEST_FLAG (VIAFLAG, ptr->Line))
	{
	  /* this is a rat going to a polygon.  do not draw for rubberband */;
	}
      else if (TEST_FLAG (RUBBERENDFLAG, ptr->Line))
	{
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
	  XORDrawAttachedLine (gc, point1->X, point1->Y,
	                       point2->X + dx, point2->Y + dy,
	                       ptr->Line->Thickness);
	}
      else if (ptr->MovedPoint == &ptr->Line->Point1)
	XORDrawAttachedLine (gc,
	                     ptr->Line->Point1.X + dx,
	                     ptr->Line->Point1.Y + dy,
	                     ptr->Line->Point2.X + dx,
	                     ptr->Line->Point2.Y + dy, ptr->Line->Thickness);

      ptr++;
      i--;
    }
}

/*!
 * \brief Draws additional stuff that follows the crosshair.
 */
void
DrawAttached (hidGC gc)
{
  gui->graphics->set_color (gc, Settings.CrosshairColor);
  gui->graphics->set_draw_xor (gc, 1);
  gui->graphics->set_line_cap (gc, Trace_Cap);
  gui->graphics->set_line_width (gc, 1);

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

        gui->graphics->thindraw_pcb_pv (gc, gc, &via, true, false);

        if (TEST_FLAG (SHOWDRCFLAG, PCB))
          {
            Coord mask_r = Settings.ViaThickness / 2 + PCB->Bloat;
            gui->graphics->set_color (gc, Settings.CrossColor);
            gui->graphics->set_line_cap (gc, Round_Cap);
            gui->graphics->set_line_width (gc, 0);
            gui->graphics->draw_arc (gc, via.X, via.Y, mask_r, mask_r, 0, 360);
            gui->graphics->set_color (gc, Settings.CrosshairColor);
          }
        break;
      }

      /* the attached line is used by both LINEMODE, POLYGON_MODE and POLYGONHOLE_MODE*/
    case POLYGON_MODE:
    case POLYGONHOLE_MODE:
      /* draw only if starting point is set */
      if (Crosshair.AttachedLine.State != STATE_FIRST)
        gui->graphics->draw_line (gc,
                                  Crosshair.AttachedLine.Point1.X,
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
	      gui->graphics->set_color (gc, Settings.CrossColor);
	      XORDrawAttachedArc (gc, Settings.LineThickness + 2 * PCB->Bloat);
	      gui->graphics->set_color (gc, Settings.CrosshairColor);
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
	  if (PCB->Clipping)
	    XORDrawAttachedLine (gc, Crosshair.AttachedLine.Point2.X,
	                             Crosshair.AttachedLine.Point2.Y,
	                         Crosshair.X, Crosshair.Y,
	                         PCB->RatDraw ? 10 : Settings.LineThickness);
	  if (TEST_FLAG (SHOWDRCFLAG, PCB))
	    {
	      gui->graphics->set_color (gc, Settings.CrossColor);
	      XORDrawAttachedLine (gc, Crosshair.AttachedLine.Point1.X,
	                               Crosshair.AttachedLine.Point1.Y,
	                           Crosshair.AttachedLine.Point2.X,
	                           Crosshair.AttachedLine.Point2.Y,
	                           PCB->RatDraw ? 10 : Settings.LineThickness
	                           + 2 * PCB->Bloat);
	      if (PCB->Clipping)
		XORDrawAttachedLine (gc, Crosshair.AttachedLine.Point2.X,
		                         Crosshair.AttachedLine.Point2.Y,
		                     Crosshair.X, Crosshair.Y,
		                     PCB->RatDraw ? 10 : Settings.
		                     LineThickness + 2 * PCB->Bloat);
	      gui->graphics->set_color (gc, Settings.CrosshairColor);
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
      gui->graphics->draw_rect (gc, x1, y1, x2, y2);
    }
}


/*!
 * \brief Draw the marker position.
 */
void
DrawMark (hidGC gc)
{
  gui->graphics->set_color (gc, Settings.CrosshairColor);
  gui->graphics->set_draw_xor (gc, 1);
  gui->graphics->set_line_cap (gc, Trace_Cap);
  gui->graphics->set_line_width (gc, 1);

  /* Mark is not drawn when it is not set */
  if (!Marked.status)
    return;

  gui->graphics->draw_line (gc,
                  Marked.X - MARK_SIZE,
                  Marked.Y - MARK_SIZE,
                  Marked.X + MARK_SIZE, Marked.Y + MARK_SIZE);
  gui->graphics->draw_line (gc,
                  Marked.X + MARK_SIZE,
                  Marked.Y - MARK_SIZE,
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
  if ((Settings.Mode != LINE_MODE || CURRENT != ptr1) &&
      (Settings.Mode != MOVE_MODE ||
       Crosshair.AttachedObject.Ptr1 != ptr1 ||
       Crosshair.AttachedObject.Type != LINEPOINT_TYPE ||
       Crosshair.AttachedObject.Ptr2 == line))
    return;

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

  /* limit the crosshair location to the board area */
  Crosshair.X = CLAMP (X, Crosshair.MinX, Crosshair.MaxX);
  Crosshair.Y = CLAMP (Y, Crosshair.MinY, Crosshair.MaxY);

  /*
   * GRID SNAPPING
   *
   * First figure out where the nearest grid point that would snap to is. Then
   * if we don't find an object that we would prefer, snap to the grid point.
   *
   */
  
  /* if we're drawing rats, don't snap to the grid */
  if (PCB->RatDraw)
    {
    /* set the "nearest grid" somewhere off the board so that everything else
     is closer */
      nearest_grid_x = -MIL_TO_COORD (6);
      nearest_grid_y = -MIL_TO_COORD (6);
    }
  else
    {
    /* not drawing rats */
    /* find the closest grid point */
      nearest_grid_x = GridFit (Crosshair.X, PCB->Grid, PCB->GridOffsetX);
      nearest_grid_y = GridFit (Crosshair.Y, PCB->Grid, PCB->GridOffsetY);

    /* if something is marked and we're forcing orthogonal moves... */
      if (Marked.status && TEST_FLAG (ORTHOMOVEFLAG, PCB))
	{
	  Coord dx = Crosshair.X - Marked.X;
	  Coord dy = Crosshair.Y - Marked.Y;
      /* restrict motion along the x or y axis
       this assumes we're always snapped to a grid point... */
	  if (ABS (dx) > ABS (dy))
	    nearest_grid_y = Marked.Y;
	  else
	    nearest_grid_x = Marked.X;
	}

    }

  /* The snap_data structure contains all of the information about where we're
   * going to snap to. */
  snap_data.crosshair = &Crosshair;
  snap_data.nearest_sq_dist =
    crosshair_sq_dist (&Crosshair, nearest_grid_x, nearest_grid_y);
  snap_data.nearest_is_grid = true;
  snap_data.x = nearest_grid_x;
  snap_data.y = nearest_grid_y;

  ans = NO_TYPE;
  /* if we're not drawing rats, check for elements first */
  if (!PCB->RatDraw)
    ans = SearchObjectByLocation (ELEMENT_TYPE, &ptr1, &ptr2, &ptr3,
                                  Crosshair.X, Crosshair.Y, PCB->Grid / 2);

  if (ans & ELEMENT_TYPE)
    {
    /* if we found an element, check to see if we should snap to it */
      ElementType *el = (ElementType *) ptr1;
      check_snap_object (&snap_data, el->MarkX, el->MarkY, false);
    }

  /* try snapping to a pad if we're drawing rats, or pad snapping is turned on */
  ans = NO_TYPE;
  if (PCB->RatDraw || TEST_FLAG (SNAPPINFLAG, PCB))
    ans = SearchObjectByLocation (PAD_TYPE, &ptr1, &ptr2, &ptr3,
                                  Crosshair.X, Crosshair.Y, PCB->Grid / 2);

  /* Avoid self-snapping when moving */
  if (ans != NO_TYPE &&
      Settings.Mode == MOVE_MODE &&
      Crosshair.AttachedObject.Type == ELEMENT_TYPE &&
      ptr1 == Crosshair.AttachedObject.Ptr1)
    ans = NO_TYPE;

  if (ans != NO_TYPE &&                  /* we found a pad */
      ( Settings.Mode == LINE_MODE ||    /* we're in line drawing mode, or... */
        Settings.Mode == ARC_MODE  ||    /* we're in arc drawing mode, or... */
       (Settings.Mode == MOVE_MODE &&    /* we're moving something and ... */
        ((Crosshair.AttachedObject.Type == LINEPOINT_TYPE) || /* it's a line */
         (Crosshair.AttachedObject.Type == ARCPOINT_TYPE))))) /* it's an arc */
    {
    /* we found a pad and we're drawing or moving a line */
      PadType *pad = (PadType *) ptr2;
      LayerType *desired_layer;
      Cardinal desired_group;
      Cardinal bottom_group, top_group;
      int found_our_layer = false;

    /* the layer we'd like to snap to, start with the current layer */
      desired_layer = CURRENT;
      if (Settings.Mode == MOVE_MODE &&
          Crosshair.AttachedObject.Type == LINEPOINT_TYPE)
        {
      /* if we're moving something, the desired layer is the layer it's on */
          desired_layer = (LayerType *)Crosshair.AttachedObject.Ptr1;
        }

    /* pads must be on the top or bottom sides.
     * find layer groups of the top and bottom sides */
      top_group = GetLayerGroupNumberBySide (TOP_SIDE);
      bottom_group = GetLayerGroupNumberBySide (BOTTOM_SIDE);
      desired_group = TEST_FLAG (ONSOLDERFLAG, pad) ? bottom_group : top_group;

      GROUP_LOOP (PCB->Data, desired_group);
      {
        if (layer == desired_layer)
          {
        /* the layer the pad is on is the same as the layer of the line
         * that we're moving. */
            found_our_layer = true;
            break;
          }
      }
      END_LOOP;

      if (found_our_layer == false)
    /* different layers, so don't snap */
        ans = NO_TYPE;
    }

  if (ans != NO_TYPE)
    {
    /* we found a pad we want to snap to, check to see if we should */
      PadType *pad = (PadType *)ptr2;
      check_snap_object (&snap_data, pad->Point1.X + (pad->Point2.X - pad->Point1.X) / 2,
                                     pad->Point1.Y + (pad->Point2.Y - pad->Point1.Y) / 2,
                         true);
    }

  /* try snapping to a pin
   * similar to snapping to a pad, but without the layer restriction */
  ans = NO_TYPE;
  if (PCB->RatDraw || TEST_FLAG (SNAPPINFLAG, PCB))
    ans = SearchObjectByLocation (PIN_TYPE, &ptr1, &ptr2, &ptr3,
                                  Crosshair.X, Crosshair.Y, PCB->Grid / 2);

  /* Avoid self-snapping when moving */
  if (ans != NO_TYPE &&
      Settings.Mode == MOVE_MODE &&
      Crosshair.AttachedObject.Type == ELEMENT_TYPE &&
      ptr1 == Crosshair.AttachedObject.Ptr1)
    ans = NO_TYPE;

  if (ans != NO_TYPE)
    {
    /* we found a pin, try to snap to it */
      PinType *pin = (PinType *)ptr2;
      check_snap_object (&snap_data, pin->X, pin->Y, true);
    }

  /* if snapping to pins and pads is turned on, try snapping to vias */
  ans = NO_TYPE;
  if (TEST_FLAG (SNAPPINFLAG, PCB))
    ans = SearchObjectByLocation (VIA_TYPE, &ptr1, &ptr2, &ptr3,
                                  Crosshair.X, Crosshair.Y, PCB->Grid / 2);

  /* Avoid snapping vias to any other vias */
  if (Settings.Mode == MOVE_MODE &&
      Crosshair.AttachedObject.Type == VIA_TYPE &&
      (ans & PIN_TYPES))
    ans = NO_TYPE;

  if (ans != NO_TYPE)
    {
    /* found a via, try snapping to it */
      PinType *pin = (PinType *)ptr2;
      check_snap_object (&snap_data, pin->X, pin->Y, true);
    }

  /* try snapping to the end points of lines and arcs */
  ans = NO_TYPE;
  if (TEST_FLAG (SNAPPINFLAG, PCB))
    ans = SearchObjectByLocation (LINEPOINT_TYPE | ARCPOINT_TYPE,
                                  &ptr1, &ptr2, &ptr3,
                                  Crosshair.X, Crosshair.Y, PCB->Grid / 2);

  if (ans != NO_TYPE)
    {
    /* found an end point, try to snap to it */
      PointType *pnt = (PointType *)ptr3;
      check_snap_object (&snap_data, pnt->X, pnt->Y, true);
    }

  /* try snapping to a point on a line that's not on the grid */
  check_snap_offgrid_line (&snap_data, nearest_grid_x, nearest_grid_y);

  /* try snapping to a point defining a polygon */
  ans = NO_TYPE;
  if (TEST_FLAG (SNAPPINFLAG, PCB))
    ans = SearchObjectByLocation (POLYGONPOINT_TYPE, &ptr1, &ptr2, &ptr3,
                                  Crosshair.X, Crosshair.Y, PCB->Grid / 2);

  if (ans != NO_TYPE)
    {
    /* found a polygon point, try snapping to it */
      PointType *pnt = (PointType *)ptr3;
      check_snap_object (&snap_data, pnt->X, pnt->Y, true);
    }

  /* if snap_data.[x,y] are >= 0, then we found something,
   * so move the crosshair */
  if (snap_data.x >= 0 && snap_data.y >= 0)
    {
      Crosshair.X = snap_data.x;
      Crosshair.Y = snap_data.y;
    }

  /* If we're in arrow mode and we're over the end point of a line,
   * change the cursor to the "draped box" to indicate that clicking would
   * grab the line endpoint */
  if (Settings.Mode == ARROW_MODE)
    {
      ans = SearchObjectByLocation (LINEPOINT_TYPE | ARCPOINT_TYPE,
                                    &ptr1, &ptr2, &ptr3,
                                    Crosshair.X, Crosshair.Y, PCB->Grid / 2);
      if (ans == NO_TYPE)
        hid_action("PointCursor");
      else if (!TEST_FLAG(SELECTEDFLAG, (LineType *)ptr2))
        hid_actionl("PointCursor","True", NULL);
    }

  if (Settings.Mode == LINE_MODE
      && Crosshair.AttachedLine.State != STATE_FIRST
      && TEST_FLAG (AUTODRCFLAG, PCB))
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
 * \brief Update the valid range for the crosshair cursor
 */

void
crosshair_update_range(void)
{
  Coord xmin, xmax, ymin, ymax;
  BoxType *box;
  
  /* limit the crosshair to being on the working area */
  xmin = 0;  xmax = PCB->MaxWidth;
  ymin = 0;  ymax = PCB->MaxHeight;
  
  /* limit the crosshair so attached objects stay on the working area */
  /* note: There are presently two ways this is done in the code, the first uses
   *       the pastebuffer bounding box, the second uses the attached object
   *       bounding box.
   */
  if (Settings.Mode == PASTEBUFFER_MODE) {
	SetBufferBoundingBox(PASTEBUFFER);
    xmin = MAX(xmin, PASTEBUFFER->X - PASTEBUFFER->BoundingBox.X1);
    ymin = MAX(ymin, PASTEBUFFER->Y - PASTEBUFFER->BoundingBox.Y1);
    xmax = MIN(xmax, PCB->MaxWidth - (PASTEBUFFER->BoundingBox.X2 - PASTEBUFFER->X));
    ymax = MIN(ymax, PCB->MaxHeight - (PASTEBUFFER->BoundingBox.Y2 - PASTEBUFFER->Y));
  }
  
  /* if there's an attached object, */
  if (Crosshair.AttachedObject.Type != NO_TYPE) {
    box = GetObjectBoundingBox (Crosshair.AttachedObject.Type,
                                Crosshair.AttachedObject.Ptr1,
                                Crosshair.AttachedObject.Ptr2,
                                Crosshair.AttachedObject.Ptr3);
    xmin = MAX(xmin, Crosshair.AttachedObject.X - box->X1);
    ymin = MAX(ymin, Crosshair.AttachedObject.Y - box->Y1);
    xmax = MIN(xmax, PCB->MaxWidth - (box->X2 - Crosshair.AttachedObject.X));
    ymax = MIN(ymax, PCB->MaxHeight - (box->Y2 - Crosshair.AttachedObject.Y));
  }
  
  SetCrosshairRange(xmin, ymin, xmax, ymax);
  
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
