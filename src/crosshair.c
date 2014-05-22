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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */


/* crosshair stuff
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
#include "rubberband.h"
#include "search.h"
#include "polygon.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define dprintf if(0)printf

typedef struct
{
  int x, y;
} point;

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void XORPolygon (PolygonType *, Coord, Coord);
static void XORDrawElement (ElementType *, Coord, Coord);
static void XORDrawBuffer (BufferType *);
static void XORDrawInsertPointObject (void);
static void XORDrawMoveOrCopyObject (void);
static void XORDrawAttachedLine (Coord, Coord, Coord, Coord, Coord);
static void XORDrawAttachedArc (Coord);

static void
thindraw_moved_pv (PinType *pv, Coord x, Coord y)
{
  /* Make a copy of the pin structure, moved to the correct position */
  PinType moved_pv = *pv;
  moved_pv.X += x;
  moved_pv.Y += y;

  gui->graphics->thindraw_pcb_pv (Crosshair.GC, Crosshair.GC, &moved_pv, true, false);
}

/* ---------------------------------------------------------------------------
 * creates a tmp polygon with coordinates converted to screen system
 */
static void
XORPolygon (PolygonType *polygon, Coord dx, Coord dy)
{
  Cardinal i;
  for (i = 0; i < polygon->PointN; i++)
    {
      Cardinal next = next_contour_point (polygon, i);
      gui->graphics->draw_line (Crosshair.GC,
                                polygon->Points[i].X + dx,
                                polygon->Points[i].Y + dy,
                                polygon->Points[next].X + dx,
                                polygon->Points[next].Y + dy);
    }
}

/*-----------------------------------------------------------
 * Draws the outline of an arc
 */
static void
XORDrawAttachedArc (Coord thick)
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
  gui->graphics->draw_arc (Crosshair.GC, arc.X, arc.Y, wy + wid, wy + wid, sa, dir);
  if (wid > pixel_slop)
    {
      gui->graphics->draw_arc (Crosshair.GC, arc.X, arc.Y, wy - wid, wy - wid, sa, dir);
      gui->graphics->draw_arc (Crosshair.GC, bx->X1, bx->Y1, wid, wid, sa,      -180 * SGN (dir));
      gui->graphics->draw_arc (Crosshair.GC, bx->X2, bx->Y2, wid, wid, sa + dir, 180 * SGN (dir));
    }
}

/*-----------------------------------------------------------
 * Draws the outline of a line
 */
static void
XORDrawAttachedLine (Coord x1, Coord y1, Coord x2, Coord y2, Coord thick)
{
  Coord dx, dy, ox, oy;
  double h;

  dx = x2 - x1;
  dy = y2 - y1;
  if (dx != 0 || dy != 0)
    h = 0.5 * thick / sqrt (SQUARE (dx) + SQUARE (dy));
  else
    h = 0.0;
  ox = dy * h + 0.5 * SGN (dy);
  oy = -(dx * h + 0.5 * SGN (dx));
  gui->graphics->draw_line (Crosshair.GC, x1 + ox, y1 + oy, x2 + ox, y2 + oy);
  if (abs (ox) >= pixel_slop || abs (oy) >= pixel_slop)
    {
      Angle angle = atan2 (dx, dy) * 57.295779;
      gui->graphics->draw_line (Crosshair.GC, x1 - ox, y1 - oy, x2 - ox, y2 - oy);
      gui->graphics->draw_arc (Crosshair.GC, x1, y1, thick / 2, thick / 2, angle - 180, 180);
      gui->graphics->draw_arc (Crosshair.GC, x2, y2, thick / 2, thick / 2, angle, 180);
    }
}

/* ---------------------------------------------------------------------------
 * draws the elements of a loaded circuit which is to be merged in
 */
static void
XORDrawElement (ElementType *Element, Coord DX, Coord DY)
{
  /* if no silkscreen, draw the bounding box */
  if (Element->ArcN == 0 && Element->LineN == 0)
    {
      gui->graphics->draw_line (Crosshair.GC,
                                DX + Element->BoundingBox.X1,
                                DY + Element->BoundingBox.Y1,
                                DX + Element->BoundingBox.X1,
                                DY + Element->BoundingBox.Y2);
      gui->graphics->draw_line (Crosshair.GC,
                                DX + Element->BoundingBox.X1,
                                DY + Element->BoundingBox.Y2,
                                DX + Element->BoundingBox.X2,
                                DY + Element->BoundingBox.Y2);
      gui->graphics->draw_line (Crosshair.GC,
                                DX + Element->BoundingBox.X2,
                                DY + Element->BoundingBox.Y2,
                                DX + Element->BoundingBox.X2,
                                DY + Element->BoundingBox.Y1);
      gui->graphics->draw_line (Crosshair.GC,
                                DX + Element->BoundingBox.X2,
                                DY + Element->BoundingBox.Y1,
                                DX + Element->BoundingBox.X1,
                                DY + Element->BoundingBox.Y1);
    }
  else
    {
      ELEMENTLINE_LOOP (Element);
      {
        gui->graphics->draw_line (Crosshair.GC,
                                  DX + line->Point1.X,
                                  DY + line->Point1.Y,
                                  DX + line->Point2.X,
                                  DY + line->Point2.Y);
      }
      END_LOOP;

      /* arc coordinates and angles have to be converted to X11 notation */
      ARC_LOOP (Element);
      {
        gui->graphics->draw_arc (Crosshair.GC,
                                 DX + arc->X,
                                 DY + arc->Y,
                                 arc->Width, arc->Height, arc->StartAngle, arc->Delta);
      }
      END_LOOP;
    }
  /* pin coordinates and angles have to be converted to X11 notation */
  PIN_LOOP (Element);
  {
    thindraw_moved_pv (pin, DX, DY);
  }
  END_LOOP;

  /* pads */
  PAD_LOOP (Element);
  {
    if (PCB->InvisibleObjectsOn ||
        (TEST_FLAG (ONSOLDERFLAG, pad) != 0) == Settings.ShowSolderSide)
      {
        /* Make a copy of the pad structure, moved to the correct position */
        PadType moved_pad = *pad;
        moved_pad.Point1.X += DX; moved_pad.Point1.Y += DY;
        moved_pad.Point2.X += DX; moved_pad.Point2.Y += DY;

        gui->graphics->thindraw_pcb_pad (Crosshair.GC, &moved_pad, false, false);
      }
  }
  END_LOOP;
  /* mark */
  gui->graphics->draw_line (Crosshair.GC,
                            Element->MarkX + DX - EMARK_SIZE,
                            Element->MarkY + DY,
                            Element->MarkX + DX,
                            Element->MarkY + DY - EMARK_SIZE);
  gui->graphics->draw_line (Crosshair.GC,
                            Element->MarkX + DX + EMARK_SIZE,
                            Element->MarkY + DY,
                            Element->MarkX + DX,
                            Element->MarkY + DY - EMARK_SIZE);
  gui->graphics->draw_line (Crosshair.GC,
                            Element->MarkX + DX - EMARK_SIZE,
                            Element->MarkY + DY,
                            Element->MarkX + DX,
                            Element->MarkY + DY + EMARK_SIZE);
  gui->graphics->draw_line (Crosshair.GC,
                            Element->MarkX + DX + EMARK_SIZE,
                            Element->MarkY + DY,
                            Element->MarkX + DX,
                            Element->MarkY + DY + EMARK_SIZE);
}

/* ---------------------------------------------------------------------------
 * draws all visible and attached objects of the pastebuffer
 */
static void
XORDrawBuffer (BufferType *Buffer)
{
  Cardinal i;
  Coord x, y;

  /* set offset */
  x = Crosshair.X - Buffer->X;
  y = Crosshair.Y - Buffer->Y;

  /* draw all visible layers */
  for (i = 0; i < max_copper_layer + 2; i++)
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
	gui->graphics->draw_line (Crosshair.GC,
	                          x + line->Point1.X, y + line->Point1.Y,
	                          x + line->Point2.X, y + line->Point2.Y);
	}
	END_LOOP;
	ARC_LOOP (layer);
	{
	  gui->graphics->draw_arc (Crosshair.GC,
	                           x + arc->X,
	                           y + arc->Y,
	                           arc->Width,
	                           arc->Height, arc->StartAngle, arc->Delta);
	}
	END_LOOP;
	TEXT_LOOP (layer);
	{
	  BoxType *box = &text->BoundingBox;
	  gui->graphics->draw_rect (Crosshair.GC,
	                            x + box->X1, y + box->Y1, x + box->X2, y + box->Y2);
	}
	END_LOOP;
	/* the tmp polygon has n+1 points because the first
	 * and the last one are set to the same coordinates
	 */
	POLYGON_LOOP (layer);
	{
	  XORPolygon (polygon, x, y);
	}
	END_LOOP;
      }

  /* draw elements if visible */
  if (PCB->PinOn && PCB->ElementOn)
    ELEMENT_LOOP (Buffer->Data);
  {
    if (FRONT (element) || PCB->InvisibleObjectsOn)
      XORDrawElement (element, x, y);
  }
  END_LOOP;

  /* and the vias */
  if (PCB->ViaOn)
    VIA_LOOP (Buffer->Data);
  {
    thindraw_moved_pv (via, x, y);
  }
  END_LOOP;
}

/* ---------------------------------------------------------------------------
 * draws the rubberband to insert points into polygons/lines/...
 */
static void
XORDrawInsertPointObject (void)
{
  LineType *line = (LineType *) Crosshair.AttachedObject.Ptr2;
  PointType *point = (PointType *) Crosshair.AttachedObject.Ptr3;

  if (Crosshair.AttachedObject.Type != NO_TYPE)
    {
      gui->graphics->draw_line (Crosshair.GC, point->X, point->Y, line->Point1.X, line->Point1.Y);
      gui->graphics->draw_line (Crosshair.GC, point->X, point->Y, line->Point2.X, line->Point2.Y);
    }
}

/* ---------------------------------------------------------------------------
 * draws the attached object while in MOVE_MODE or COPY_MODE
 */
static void
XORDrawMoveOrCopyObject (void)
{
  RubberbandType *ptr;
  Cardinal i;
  LineType  LineOut;
  PointType PointOut;
  Coord dx = Crosshair.X - Crosshair.AttachedObject.X,
        dy = Crosshair.Y - Crosshair.AttachedObject.Y;

  switch (Crosshair.AttachedObject.Type)
    {
    case VIA_TYPE:
      {
        PinType *via = (PinType *) Crosshair.AttachedObject.Ptr1;
        thindraw_moved_pv (via, dx, dy);
        break;
      }

    case LINE_TYPE:
      {
        LineType *line = (LineType *) Crosshair.AttachedObject.Ptr2;

        dprintf("line->Point1.X = %d line->Point1.Y = %d RubberbandN = %d\n",
                line->Point1.X, line->Point1.Y, 
                Crosshair.AttachedObject.RubberbandN);

        /* work out coords of line to draw given rubber mode */

        RestrictMovementGivenRubberBandMode (line, &dx, &dy);
        MoveLineGivenRubberBandMode (&LineOut, line, dx, dy, Crosshair);

        XORDrawAttachedLine (LineOut.Point1.X, LineOut.Point1.Y,
                             LineOut.Point2.X, LineOut.Point2.Y,
                             LineOut.Thickness);
	break;
      }

    case ARC_TYPE:
      {
	ArcType *Arc = (ArcType *) Crosshair.AttachedObject.Ptr2;

	gui->graphics->draw_arc (Crosshair.GC,
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
	XORPolygon (polygon, dx, dy);
	break;
      }

    case LINEPOINT_TYPE:
      {
	LineType *line;
	PointType *point;

	line = (LineType *) Crosshair.AttachedObject.Ptr2;
	point = (PointType *) Crosshair.AttachedObject.Ptr3;
	if (point == &line->Point1)
	  XORDrawAttachedLine (point->X + dx,
			       point->Y + dy, line->Point2.X,
			       line->Point2.Y, line->Thickness);
	else
	  XORDrawAttachedLine (point->X + dx,
			       point->Y + dy, line->Point1.X,
			       line->Point1.Y, line->Thickness);
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
	gui->graphics->draw_line (Crosshair.GC,
	                          polygon->Points[prev].X, polygon->Points[prev].Y,
	                          point->X + dx, point->Y + dy);
	gui->graphics->draw_line (Crosshair.GC,
	                          point->X + dx, point->Y + dy,
	                          polygon->Points[next].X, polygon->Points[next].Y);
	break;
      }

    case ELEMENTNAME_TYPE:
      {
	/* locate the element "mark" and draw an association line from crosshair to it */
	ElementType *element =
	  (ElementType *) Crosshair.AttachedObject.Ptr1;

	gui->graphics->draw_line (Crosshair.GC,
	                          element->MarkX,
	                          element->MarkY, Crosshair.X, Crosshair.Y);
	/* fall through to move the text as a box outline */
      }
    case TEXT_TYPE:
      {
	TextType *text = (TextType *) Crosshair.AttachedObject.Ptr2;
	BoxType *box = &text->BoundingBox;
	gui->graphics->draw_rect (Crosshair.GC,
	                          box->X1 + dx,
	                          box->Y1 + dy, box->X2 + dx, box->Y2 + dy);
	break;
      }

      /* pin/pad movements result in moving an element */
    case PAD_TYPE:
    case PIN_TYPE:
    case ELEMENT_TYPE:
      XORDrawElement ((ElementType *) Crosshair.AttachedObject.Ptr2, dx, dy);
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

          MovePointGivenRubberBandMode (&PointOut, point2, ptr->Line, dx, dy,
                                        Crosshair.AttachedObject.Type, 0);

          XORDrawAttachedLine (point1->X, point1->Y, 
                               PointOut.X, PointOut.Y,
                               ptr->Line->Thickness);

	}
      else if (ptr->MovedPoint == &ptr->Line->Point1)
	XORDrawAttachedLine (ptr->Line->Point1.X + dx,
			     ptr->Line->Point1.Y + dy,
			     ptr->Line->Point2.X + dx,
			     ptr->Line->Point2.Y + dy, ptr->Line->Thickness);

      ptr++;
      i--;
    }
}

/* ---------------------------------------------------------------------------
 * draws additional stuff that follows the crosshair
 */
void
DrawAttached (void)
{
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

        gui->graphics->thindraw_pcb_pv (Crosshair.GC, Crosshair.GC, &via, true, false);

        if (TEST_FLAG (SHOWDRCFLAG, PCB))
          {
            /* XXX: Naughty cheat - use the mask to draw DRC clearance! */
            via.Mask = Settings.ViaThickness + PCB->Bloat * 2;
            gui->graphics->set_color (Crosshair.GC, Settings.CrossColor);
            gui->graphics->thindraw_pcb_pv (Crosshair.GC, Crosshair.GC, &via, false, true);
            gui->graphics->set_color (Crosshair.GC, Settings.CrosshairColor);
          }
        break;
      }

      /* the attached line is used by both LINEMODE, POLYGON_MODE and POLYGONHOLE_MODE*/
    case POLYGON_MODE:
    case POLYGONHOLE_MODE:
      /* draw only if starting point is set */
      if (Crosshair.AttachedLine.State != STATE_FIRST)
        gui->graphics->draw_line (Crosshair.GC,
                                  Crosshair.AttachedLine.Point1.X,
                                  Crosshair.AttachedLine.Point1.Y,
                                  Crosshair.AttachedLine.Point2.X,
                                  Crosshair.AttachedLine.Point2.Y);

      /* draw attached polygon only if in POLYGON_MODE or POLYGONHOLE_MODE */
      if (Crosshair.AttachedPolygon.PointN > 1)
	{
	  XORPolygon (&Crosshair.AttachedPolygon, 0, 0);
	}
      break;

    case ARC_MODE:
      if (Crosshair.AttachedBox.State != STATE_FIRST)
	{
	  XORDrawAttachedArc (Settings.LineThickness);
	  if (TEST_FLAG (SHOWDRCFLAG, PCB))
	    {
	      gui->graphics->set_color (Crosshair.GC, Settings.CrossColor);
	      XORDrawAttachedArc (Settings.LineThickness +
				  2 * (PCB->Bloat + 1));
	      gui->graphics->set_color (Crosshair.GC, Settings.CrosshairColor);
	    }

	}
      break;

    case LINE_MODE:
      /* draw only if starting point exists and the line has length */
      if (Crosshair.AttachedLine.State != STATE_FIRST &&
	  Crosshair.AttachedLine.draw)
	{
	  XORDrawAttachedLine (Crosshair.AttachedLine.Point1.X,
			       Crosshair.AttachedLine.Point1.Y,
			       Crosshair.AttachedLine.Point2.X,
			       Crosshair.AttachedLine.Point2.Y,
			       PCB->RatDraw ? 10 : Settings.LineThickness);
	  /* draw two lines ? */
	  if (PCB->Clipping)
	    XORDrawAttachedLine (Crosshair.AttachedLine.Point2.X,
				 Crosshair.AttachedLine.Point2.Y,
				 Crosshair.X, Crosshair.Y,
				 PCB->RatDraw ? 10 : Settings.LineThickness);
	  if (TEST_FLAG (SHOWDRCFLAG, PCB))
	    {
	      gui->graphics->set_color (Crosshair.GC, Settings.CrossColor);
	      XORDrawAttachedLine (Crosshair.AttachedLine.Point1.X,
				   Crosshair.AttachedLine.Point1.Y,
				   Crosshair.AttachedLine.Point2.X,
				   Crosshair.AttachedLine.Point2.Y,
				   PCB->RatDraw ? 10 : Settings.LineThickness
				   + 2 * (PCB->Bloat + 1));
	      if (PCB->Clipping)
		XORDrawAttachedLine (Crosshair.AttachedLine.Point2.X,
				     Crosshair.AttachedLine.Point2.Y,
				     Crosshair.X, Crosshair.Y,
				     PCB->RatDraw ? 10 : Settings.
				     LineThickness + 2 * (PCB->Bloat + 1));
	      gui->graphics->set_color (Crosshair.GC, Settings.CrosshairColor);
	    }
	}
      break;

    case PASTEBUFFER_MODE:
      XORDrawBuffer (PASTEBUFFER);
      break;

    case COPY_MODE:
    case MOVE_MODE:
      XORDrawMoveOrCopyObject ();
      break;

    case INSERTPOINT_MODE:
      XORDrawInsertPointObject ();
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
      gui->graphics->draw_rect (Crosshair.GC, x1, y1, x2, y2);
    }
}


/* --------------------------------------------------------------------------
 * draw the marker position
 */
void
DrawMark (void)
{
  /* Mark is not drawn when it is not set */
  if (!Marked.status)
    return;

  gui->graphics->draw_line (Crosshair.GC,
                  Marked.X - MARK_SIZE,
                  Marked.Y - MARK_SIZE,
                  Marked.X + MARK_SIZE, Marked.Y + MARK_SIZE);
  gui->graphics->draw_line (Crosshair.GC,
                  Marked.X + MARK_SIZE,
                  Marked.Y - MARK_SIZE,
                  Marked.X - MARK_SIZE, Marked.Y + MARK_SIZE);
}

/* ---------------------------------------------------------------------------
 * Returns the nearest grid-point to the given Coord
 */
Coord
GridFit (Coord x, Coord grid_spacing, Coord grid_offset)
{
  x -= grid_offset;
  x = grid_spacing * round ((double) x / grid_spacing);
  x += grid_offset;
  return x;
}


/* ---------------------------------------------------------------------------
 * notify the GUI that data relating to the crosshair is being changed.
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


/* ---------------------------------------------------------------------------
 * notify the GUI that data relating to the mark is being changed.
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


/* ---------------------------------------------------------------------------
 * Convenience for plugins using the old {Hide,Restore}Crosshair API.
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

/* ---------------------------------------------------------------------------
 * Returns the square of the given number
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

/* Snap to a given location if it is the closest thing we found so far.
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
  ans = SearchScreenGridSlop (Crosshair.X, Crosshair.Y,
                              LINE_TYPE, &ptr1, &ptr2, &ptr3);

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

/* ---------------------------------------------------------------------------
 * recalculates the passed coordinates to fit the current grid setting
 */
void
FitCrosshairIntoGrid (Coord X, Coord Y)
{
  Coord nearest_grid_x, nearest_grid_y;
  void *ptr1, *ptr2, *ptr3;
  struct snap_data snap_data;
  int ans;

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
  snap_data.nearest_sq_dist =
    crosshair_sq_dist (&Crosshair, nearest_grid_x, nearest_grid_y);
  snap_data.nearest_is_grid = true;
  snap_data.x = nearest_grid_x;
  snap_data.y = nearest_grid_y;

  ans = NO_TYPE;
  if (!PCB->RatDraw)
    ans = SearchScreenGridSlop (Crosshair.X, Crosshair.Y,
                                ELEMENT_TYPE, &ptr1, &ptr2, &ptr3);

  if (ans & ELEMENT_TYPE)
    {
      ElementType *el = (ElementType *) ptr1;
      check_snap_object (&snap_data, el->MarkX, el->MarkY, false);
    }

  ans = NO_TYPE;
  if (PCB->RatDraw || TEST_FLAG (SNAPPINFLAG, PCB))
    ans = SearchScreenGridSlop (Crosshair.X, Crosshair.Y,
                                PAD_TYPE, &ptr1, &ptr2, &ptr3);

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
      Cardinal SLayer, CLayer;
      int found_our_layer = false;

      desired_layer = CURRENT;
      if (Settings.Mode == MOVE_MODE &&
          Crosshair.AttachedObject.Type == LINEPOINT_TYPE)
        {
          desired_layer = (LayerType *)Crosshair.AttachedObject.Ptr1;
        }

      /* find layer groups of the component side and solder side */
      SLayer = GetLayerGroupNumberByNumber (solder_silk_layer);
      CLayer = GetLayerGroupNumberByNumber (component_silk_layer);
      desired_group = TEST_FLAG (ONSOLDERFLAG, pad) ? SLayer : CLayer;

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
    ans = SearchScreenGridSlop (Crosshair.X, Crosshair.Y,
                                PIN_TYPE, &ptr1, &ptr2, &ptr3);

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
    ans = SearchScreenGridSlop (Crosshair.X, Crosshair.Y,
                                VIA_TYPE, &ptr1, &ptr2, &ptr3);

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
    ans = SearchScreenGridSlop (Crosshair.X, Crosshair.Y,
                                LINEPOINT_TYPE | ARCPOINT_TYPE,
                                &ptr1, &ptr2, &ptr3);

  if (ans != NO_TYPE)
    {
      PointType *pnt = (PointType *)ptr3;
      check_snap_object (&snap_data, pnt->X, pnt->Y, true);
    }

  check_snap_offgrid_line (&snap_data, nearest_grid_x, nearest_grid_y);

  ans = NO_TYPE;
  if (TEST_FLAG (SNAPPINFLAG, PCB))
    ans = SearchScreenGridSlop (Crosshair.X, Crosshair.Y,
                                POLYGONPOINT_TYPE, &ptr1, &ptr2, &ptr3);

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
      ans = SearchScreenGridSlop (Crosshair.X, Crosshair.Y,
                                  LINEPOINT_TYPE | ARCPOINT_TYPE,
                                  &ptr1, &ptr2, &ptr3);
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

/* ---------------------------------------------------------------------------
 * move crosshair absolute
 * return true if the crosshair was moved from its existing position
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

/* ---------------------------------------------------------------------------
 * sets the valid range for the crosshair cursor
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

/* ---------------------------------------------------------------------------
 * initializes crosshair stuff
 * clears the struct, allocates to graphical contexts
 */
void
InitCrosshair (void)
{
  Crosshair.GC = gui->graphics->make_gc ();

  gui->graphics->set_color (Crosshair.GC, Settings.CrosshairColor);
  gui->graphics->set_draw_xor (Crosshair.GC, 1);
  gui->graphics->set_line_cap (Crosshair.GC, Trace_Cap);
  gui->graphics->set_line_width (Crosshair.GC, 1);

  /* set initial shape */
  Crosshair.shape = Basic_Crosshair_Shape;

  /* set default limits */
  Crosshair.MinX = Crosshair.MinY = 0;
  Crosshair.MaxX = PCB->MaxWidth;
  Crosshair.MaxY = PCB->MaxHeight;

  /* clear the mark */
  Marked.status = false;
}

/* ---------------------------------------------------------------------------
 * exits crosshair routines, release GCs
 */
void
DestroyCrosshair (void)
{
  FreePolygonMemory (&Crosshair.AttachedPolygon);
  gui->graphics->destroy_gc (Crosshair.GC);
}
