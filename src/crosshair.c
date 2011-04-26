/* $Id$ */
/* 15 Oct 2008 Ineiev: add different crosshair shapes */

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

RCSID ("$Id$");

#if !defined(ABS)
#define ABS(x) (((x)<0)?-(x):(x))
#endif

typedef struct
{
  int x, y;
} point;

/* ---------------------------------------------------------------------------
 * some local identifiers
 */

/* This is a stack for HideCrosshair() and RestoreCrosshair() calls. They
 * must always be matched. */
static bool CrosshairStack[MAX_CROSSHAIRSTACK_DEPTH];
static int CrosshairStackLocation = 0;

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void XORPolygon (PolygonTypePtr, LocationType, LocationType);
static void XORDrawElement (ElementTypePtr, LocationType, LocationType);
static void XORDrawBuffer (BufferTypePtr);
static void XORDrawInsertPointObject (void);
static void XORDrawMoveOrCopyObject (void);
static void XORDrawAttachedLine (LocationType, LocationType, LocationType,
				 LocationType, BDimension);
static void XORDrawAttachedArc (BDimension);

static void
thindraw_moved_pv (PinType *pv, int x, int y)
{
  /* Make a copy of the pin structure, moved to the correct position */
  PinType moved_pv = *pv;
  moved_pv.X += x;
  moved_pv.Y += y;

  gui->thindraw_pcb_pv (Crosshair.GC, Crosshair.GC, &moved_pv, true, false);
}

/* ---------------------------------------------------------------------------
 * creates a tmp polygon with coordinates converted to screen system
 */
static void
XORPolygon (PolygonTypePtr polygon, LocationType dx, LocationType dy)
{
  Cardinal i;
  for (i = 0; i < polygon->PointN; i++)
    {
      Cardinal next = next_contour_point (polygon, i);
      gui->draw_line (Crosshair.GC,
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
XORDrawAttachedArc (BDimension thick)
{
  ArcType arc;
  BoxTypePtr bx;
  LocationType wx, wy;
  int sa, dir;
  BDimension wid = thick / 2;

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
  gui->draw_arc (Crosshair.GC, arc.X, arc.Y, wy + wid, wy + wid, sa, dir);
  if (wid > pixel_slop)
    {
      gui->draw_arc (Crosshair.GC, arc.X, arc.Y, wy - wid, wy - wid, sa, dir);
      gui->draw_arc (Crosshair.GC, bx->X1, bx->Y1,
		     wid, wid, sa, -180 * SGN (dir));
      gui->draw_arc (Crosshair.GC, bx->X2, bx->Y2,
		     wid, wid, sa + dir, 180 * SGN (dir));
    }
}

/*-----------------------------------------------------------
 * Draws the outline of a line
 */
static void
XORDrawAttachedLine (LocationType x1, LocationType y1, LocationType x2,
		     LocationType y2, BDimension thick)
{
  LocationType dx, dy, ox, oy;
  float h;

  dx = x2 - x1;
  dy = y2 - y1;
  if (dx != 0 || dy != 0)
    h = 0.5 * thick / sqrt (SQUARE (dx) + SQUARE (dy));
  else
    h = 0.0;
  ox = dy * h + 0.5 * SGN (dy);
  oy = -(dx * h + 0.5 * SGN (dx));
  gui->draw_line (Crosshair.GC, x1 + ox, y1 + oy, x2 + ox, y2 + oy);
  if (abs (ox) >= pixel_slop || abs (oy) >= pixel_slop)
    {
      LocationType angle = atan2 ((float) dx, (float) dy) * 57.295779;
      gui->draw_line (Crosshair.GC, x1 - ox, y1 - oy, x2 - ox, y2 - oy);
      gui->draw_arc (Crosshair.GC,
		     x1, y1, thick / 2, thick / 2, angle - 180, 180);
      gui->draw_arc (Crosshair.GC, x2, y2, thick / 2, thick / 2, angle, 180);
    }
}

/* ---------------------------------------------------------------------------
 * draws the elements of a loaded circuit which is to be merged in
 */
static void
XORDrawElement (ElementTypePtr Element, LocationType DX, LocationType DY)
{
  /* if no silkscreen, draw the bounding box */
  if (Element->ArcN == 0 && Element->LineN == 0)
    {
      gui->draw_line (Crosshair.GC,
		      DX + Element->BoundingBox.X1,
		      DY + Element->BoundingBox.Y1,
		      DX + Element->BoundingBox.X1,
		      DY + Element->BoundingBox.Y2);
      gui->draw_line (Crosshair.GC,
		      DX + Element->BoundingBox.X1,
		      DY + Element->BoundingBox.Y2,
		      DX + Element->BoundingBox.X2,
		      DY + Element->BoundingBox.Y2);
      gui->draw_line (Crosshair.GC,
		      DX + Element->BoundingBox.X2,
		      DY + Element->BoundingBox.Y2,
		      DX + Element->BoundingBox.X2,
		      DY + Element->BoundingBox.Y1);
      gui->draw_line (Crosshair.GC,
		      DX + Element->BoundingBox.X2,
		      DY + Element->BoundingBox.Y1,
		      DX + Element->BoundingBox.X1,
		      DY + Element->BoundingBox.Y1);
    }
  else
    {
      ELEMENTLINE_LOOP (Element);
      {
	gui->draw_line (Crosshair.GC,
			DX + line->Point1.X,
			DY + line->Point1.Y,
			DX + line->Point2.X, DY + line->Point2.Y);
      }
      END_LOOP;

      /* arc coordinates and angles have to be converted to X11 notation */
      ARC_LOOP (Element);
      {
	gui->draw_arc (Crosshair.GC,
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

        gui->thindraw_pcb_pad (Crosshair.GC, &moved_pad, false, false);
      }
  }
  END_LOOP;
  /* mark */
  gui->draw_line (Crosshair.GC,
		  Element->MarkX + DX - EMARK_SIZE,
		  Element->MarkY + DY,
		  Element->MarkX + DX, Element->MarkY + DY - EMARK_SIZE);
  gui->draw_line (Crosshair.GC,
		  Element->MarkX + DX + EMARK_SIZE,
		  Element->MarkY + DY,
		  Element->MarkX + DX, Element->MarkY + DY - EMARK_SIZE);
  gui->draw_line (Crosshair.GC,
		  Element->MarkX + DX - EMARK_SIZE,
		  Element->MarkY + DY,
		  Element->MarkX + DX, Element->MarkY + DY + EMARK_SIZE);
  gui->draw_line (Crosshair.GC,
		  Element->MarkX + DX + EMARK_SIZE,
		  Element->MarkY + DY,
		  Element->MarkX + DX, Element->MarkY + DY + EMARK_SIZE);
}

/* ---------------------------------------------------------------------------
 * draws all visible and attached objects of the pastebuffer
 */
static void
XORDrawBuffer (BufferTypePtr Buffer)
{
  Cardinal i;
  LocationType x, y;

  /* set offset */
  x = Crosshair.X - Buffer->X;
  y = Crosshair.Y - Buffer->Y;

  /* draw all visible layers */
  for (i = 0; i < max_copper_layer + 2; i++)
    if (PCB->Data->Layer[i].On)
      {
	LayerTypePtr layer = &Buffer->Data->Layer[i];

	LINE_LOOP (layer);
	{
/*
				XORDrawAttachedLine(x +line->Point1.X,
					y +line->Point1.Y, x +line->Point2.X,
					y +line->Point2.Y, line->Thickness);
*/
	  gui->draw_line (Crosshair.GC,
			  x + line->Point1.X, y + line->Point1.Y,
			  x + line->Point2.X, y + line->Point2.Y);
	}
	END_LOOP;
	ARC_LOOP (layer);
	{
	  gui->draw_arc (Crosshair.GC,
			 x + arc->X,
			 y + arc->Y,
			 arc->Width,
			 arc->Height, arc->StartAngle, arc->Delta);
	}
	END_LOOP;
	TEXT_LOOP (layer);
	{
	  BoxTypePtr box = &text->BoundingBox;
	  gui->draw_rect (Crosshair.GC,
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
  LineTypePtr line = (LineTypePtr) Crosshair.AttachedObject.Ptr2;
  PointTypePtr point = (PointTypePtr) Crosshair.AttachedObject.Ptr3;

  if (Crosshair.AttachedObject.Type != NO_TYPE)
    {
      gui->draw_line (Crosshair.GC,
		      point->X, point->Y, line->Point1.X, line->Point1.Y);
      gui->draw_line (Crosshair.GC,
		      point->X, point->Y, line->Point2.X, line->Point2.Y);
    }
}

/* ---------------------------------------------------------------------------
 * draws the attached object while in MOVE_MODE or COPY_MODE
 */
static void
XORDrawMoveOrCopyObject (void)
{
  RubberbandTypePtr ptr;
  Cardinal i;
  LocationType dx = Crosshair.X - Crosshair.AttachedObject.X,
    dy = Crosshair.Y - Crosshair.AttachedObject.Y;

  switch (Crosshair.AttachedObject.Type)
    {
    case VIA_TYPE:
      {
        PinTypePtr via = (PinTypePtr) Crosshair.AttachedObject.Ptr1;
        thindraw_moved_pv (via, dx, dy);
        break;
      }

    case LINE_TYPE:
      {
	LineTypePtr line = (LineTypePtr) Crosshair.AttachedObject.Ptr2;

	XORDrawAttachedLine (line->Point1.X + dx, line->Point1.Y + dy,
			     line->Point2.X + dx, line->Point2.Y + dy,
			     line->Thickness);
	break;
      }

    case ARC_TYPE:
      {
	ArcTypePtr Arc = (ArcTypePtr) Crosshair.AttachedObject.Ptr2;

	gui->draw_arc (Crosshair.GC,
		       Arc->X + dx,
		       Arc->Y + dy,
		       Arc->Width, Arc->Height, Arc->StartAngle, Arc->Delta);
	break;
      }

    case POLYGON_TYPE:
      {
	PolygonTypePtr polygon =
	  (PolygonTypePtr) Crosshair.AttachedObject.Ptr2;

	/* the tmp polygon has n+1 points because the first
	 * and the last one are set to the same coordinates
	 */
	XORPolygon (polygon, dx, dy);
	break;
      }

    case LINEPOINT_TYPE:
      {
	LineTypePtr line;
	PointTypePtr point;

	line = (LineTypePtr) Crosshair.AttachedObject.Ptr2;
	point = (PointTypePtr) Crosshair.AttachedObject.Ptr3;
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
	PolygonTypePtr polygon;
	PointTypePtr point;
	Cardinal point_idx, prev, next;

	polygon = (PolygonTypePtr) Crosshair.AttachedObject.Ptr2;
	point = (PointTypePtr) Crosshair.AttachedObject.Ptr3;
	point_idx = polygon_point_idx (polygon, point);

	/* get previous and following point */
	prev = prev_contour_point (polygon, point_idx);
	next = next_contour_point (polygon, point_idx);

	/* draw the two segments */
	gui->draw_line (Crosshair.GC,
			polygon->Points[prev].X, polygon->Points[prev].Y,
			point->X + dx, point->Y + dy);
	gui->draw_line (Crosshair.GC,
			point->X + dx, point->Y + dy,
			polygon->Points[next].X, polygon->Points[next].Y);
	break;
      }

    case ELEMENTNAME_TYPE:
      {
	/* locate the element "mark" and draw an association line from crosshair to it */
	ElementTypePtr element =
	  (ElementTypePtr) Crosshair.AttachedObject.Ptr1;

	gui->draw_line (Crosshair.GC,
			element->MarkX,
			element->MarkY, Crosshair.X, Crosshair.Y);
	/* fall through to move the text as a box outline */
      }
    case TEXT_TYPE:
      {
	TextTypePtr text = (TextTypePtr) Crosshair.AttachedObject.Ptr2;
	BoxTypePtr box = &text->BoundingBox;
	gui->draw_rect (Crosshair.GC,
			box->X1 + dx,
			box->Y1 + dy, box->X2 + dx, box->Y2 + dy);
	break;
      }

      /* pin/pad movements result in moving an element */
    case PAD_TYPE:
    case PIN_TYPE:
    case ELEMENT_TYPE:
      XORDrawElement ((ElementTypePtr) Crosshair.AttachedObject.Ptr2, dx, dy);
      break;
    }

  /* draw the attached rubberband lines too */
  i = Crosshair.AttachedObject.RubberbandN;
  ptr = Crosshair.AttachedObject.Rubberband;
  while (i)
    {
      PointTypePtr point1, point2;

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
	  XORDrawAttachedLine (point1->X,
			       point1->Y, point2->X + dx,
			       point2->Y + dy, ptr->Line->Thickness);
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
  if (!Crosshair.On)
    return;

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

        gui->thindraw_pcb_pv (Crosshair.GC, Crosshair.GC, &via, true, false);

        if (TEST_FLAG (SHOWDRCFLAG, PCB))
          {
            /* XXX: Naughty cheat - use the mask to draw DRC clearance! */
            via.Mask = Settings.ViaThickness + PCB->Bloat * 2;
            gui->set_color (Crosshair.GC, Settings.CrossColor);
            gui->thindraw_pcb_pv (Crosshair.GC, Crosshair.GC, &via, false, true);
            gui->set_color (Crosshair.GC, Settings.CrosshairColor);
          }
        break;
      }

      /* the attached line is used by both LINEMODE, POLYGON_MODE and POLYGONHOLE_MODE*/
    case POLYGON_MODE:
    case POLYGONHOLE_MODE:
      /* draw only if starting point is set */
      if (Crosshair.AttachedLine.State != STATE_FIRST)
	gui->draw_line (Crosshair.GC,
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
	      gui->set_color (Crosshair.GC, Settings.CrossColor);
	      XORDrawAttachedArc (Settings.LineThickness +
				  2 * (PCB->Bloat + 1));
	      gui->set_color (Crosshair.GC, Settings.CrosshairColor);
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
	      gui->set_color (Crosshair.GC, Settings.CrossColor);
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
	      gui->set_color (Crosshair.GC, Settings.CrosshairColor);
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
      LocationType x1, y1, x2, y2;

      x1 = Crosshair.AttachedBox.Point1.X;
      y1 = Crosshair.AttachedBox.Point1.Y;
      x2 = Crosshair.AttachedBox.Point2.X;
      y2 = Crosshair.AttachedBox.Point2.Y;
      gui->draw_rect (Crosshair.GC, x1, y1, x2, y2);
    }
}


/* --------------------------------------------------------------------------
 * draw the marker position
 */
void
DrawMark (void)
{
  /* Mark is not drawn when the crosshair is off, or when it is not set */
  if (!Crosshair.On || !Marked.status)
    return;

  gui->draw_line (Crosshair.GC,
                  Marked.X - MARK_SIZE,
                  Marked.Y - MARK_SIZE,
                  Marked.X + MARK_SIZE, Marked.Y + MARK_SIZE);
  gui->draw_line (Crosshair.GC,
                  Marked.X + MARK_SIZE,
                  Marked.Y - MARK_SIZE,
                  Marked.X - MARK_SIZE, Marked.Y + MARK_SIZE);
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
  if (changes_complete)
    {
      /* fprintf(stderr, "RestoreCrosshair stack %d\n", CrosshairStackLocation); */
      if (CrosshairStackLocation <= 0)
        {
          fprintf(stderr, "Error: CrosshairStackLocation underflow\n");
          return;
        }

      CrosshairStackLocation--;

      if (CrosshairStack[CrosshairStackLocation])
        CrosshairOn ();
      else
        CrosshairOff ();
    }
  else
    {
      /* fprintf(stderr, "HideCrosshair stack %d\n", CrosshairStackLocation); */
      if (CrosshairStackLocation >= MAX_CROSSHAIRSTACK_DEPTH)
        {
          fprintf(stderr, "Error: CrosshairStackLocation overflow\n");
          return;
        }

      CrosshairStack[CrosshairStackLocation] = Crosshair.On;
      CrosshairStackLocation++;

      CrosshairOff ();
    }
}


/* ---------------------------------------------------------------------------
 * switches crosshair on
 */
void
CrosshairOn (void)
{
  if (!Crosshair.On)
    {
      Crosshair.On = true;
      DrawAttached ();
      DrawMark ();
    }
}

/* ---------------------------------------------------------------------------
 * switches crosshair off
 */
void
CrosshairOff (void)
{
  if (Crosshair.On)
    {
      DrawAttached ();
      DrawMark ();
      Crosshair.On = false;
    }
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
}

/* ---------------------------------------------------------------------------
 * Returns the square of the given number
 */
static double
square (double x)
{
  return x * x;
}

/* ---------------------------------------------------------------------------
 * recalculates the passed coordinates to fit the current grid setting
 */
void
FitCrosshairIntoGrid (LocationType X, LocationType Y)
{
  LocationType x2, y2, x0, y0;
  void *ptr1, *ptr2, *ptr3;
  double nearest, sq_dist;
  int ans;

  x0 = 0;
  y0 = 0;
  x2 = PCB->MaxWidth;
  y2 = PCB->MaxHeight;
  Crosshair.X = MIN (Crosshair.MaxX, MAX (Crosshair.MinX, X));
  Crosshair.Y = MIN (Crosshair.MaxY, MAX (Crosshair.MinY, Y));

  if (PCB->RatDraw)
    {
      x0 = -600;
      y0 = -600;
    }
  else
    {
      /* check if new position is inside the output window
       * This might not be true after the window has been resized.
       * In this case we just set it to the center of the window or
       * with respect to the grid (if possible)
       */
      if (Crosshair.X < x0 || Crosshair.X > x2)
	{
	  if (x2 + 1 >= PCB->Grid)
	    /* there must be a point that matches the grid 
	     * so we just have to look for it with some integer
	     * calculations
	     */
	    x0 = GRIDFIT_X (PCB->Grid, PCB->Grid);
	  else
	    x0 = (x2) / 2;
	}
      else
	/* check if the new position matches the grid */
	x0 = GRIDFIT_X (Crosshair.X, PCB->Grid);

      /* do the same for the second coordinate */
      if (Crosshair.Y < y0 || Crosshair.Y > y2)
	{
	  if (y2 + 1 >= PCB->Grid)
	    y0 = GRIDFIT_Y (PCB->Grid, PCB->Grid);
	  else
	    y0 = (y2) / 2;
	}
      else
	y0 = GRIDFIT_Y (Crosshair.Y, PCB->Grid);

      if (Marked.status && TEST_FLAG (ORTHOMOVEFLAG, PCB))
	{
	  int dx = Crosshair.X - Marked.X;
	  int dy = Crosshair.Y - Marked.Y;
	  if (ABS (dx) > ABS (dy))
	    y0 = Marked.Y;
	  else
	    x0 = Marked.X;
	}

    }

  nearest = -1;

  if (PCB->RatDraw || TEST_FLAG (SNAPPINFLAG, PCB))
    ans = SearchScreenGridSlop (Crosshair.X, Crosshair.Y,
                                PAD_TYPE, &ptr1, &ptr2, &ptr3);
  else
    ans = NO_TYPE;

  /* Avoid self-snapping when moving */
  if (ans && Settings.Mode == MOVE_MODE &&
      Crosshair.AttachedObject.Type == ELEMENT_TYPE &&
      ptr1 == Crosshair.AttachedObject.Ptr1)
    ans = NO_TYPE;

  if (ans && (Settings.Mode == LINE_MODE ||
              (Settings.Mode == MOVE_MODE &&
               Crosshair.AttachedObject.Type == LINEPOINT_TYPE)))
    {
      PadTypePtr pad = (PadTypePtr) ptr2;
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

  if (ans)
    {
      PadTypePtr pad = (PadTypePtr) ptr2;
      LocationType px, py;

      px = (pad->Point1.X + pad->Point2.X) / 2;
      py = (pad->Point1.Y + pad->Point2.Y) / 2;

      sq_dist = square (px - Crosshair.X) + square (py - Crosshair.Y);

      if (!gui->shift_is_pressed() ||
          square (x0 - Crosshair.X) + square (y0 - Crosshair.Y) > sq_dist)
        {
          x0 = px;
          y0 = py;
          nearest = sq_dist;
        }
    }

  if (PCB->RatDraw || TEST_FLAG (SNAPPINFLAG, PCB))
    ans = SearchScreenGridSlop (Crosshair.X, Crosshair.Y,
                                PIN_TYPE, &ptr1, &ptr2, &ptr3);
  else
    ans = NO_TYPE;

  /* Avoid self-snapping when moving */
  if (ans && Settings.Mode == MOVE_MODE &&
      Crosshair.AttachedObject.Type == ELEMENT_TYPE &&
      ptr1 == Crosshair.AttachedObject.Ptr1)
    ans = NO_TYPE;

  if (ans)
    {
      PinTypePtr pin = (PinTypePtr) ptr2;
      sq_dist = square (pin->X - Crosshair.X) + square (pin->Y - Crosshair.Y);
      if ((nearest == -1 || sq_dist < nearest) &&
          (!gui->shift_is_pressed() ||
           square (x0 - Crosshair.X) + square (y0 - Crosshair.Y) > sq_dist))
        {
          x0 = pin->X;
          y0 = pin->Y;
          nearest = sq_dist;
        }
    }

  if (TEST_FLAG (SNAPPINFLAG, PCB))
    ans = SearchScreenGridSlop (Crosshair.X, Crosshair.Y,
                                VIA_TYPE, &ptr1, &ptr2, &ptr3);
  else
    ans = NO_TYPE;

  /* Avoid snapping vias to any other vias */
  if (Settings.Mode == MOVE_MODE &&
      Crosshair.AttachedObject.Type == VIA_TYPE)
    {
        if (ans & PIN_TYPES)
          ans = NO_TYPE;
    }

  if (ans)
    {
      PinTypePtr pin = (PinTypePtr) ptr2;
      sq_dist = square (pin->X - Crosshair.X) + square (pin->Y - Crosshair.Y);
      if ((nearest == -1 || sq_dist < nearest) &&
          (!gui->shift_is_pressed() ||
           square (x0 - Crosshair.X) + square (y0 - Crosshair.Y) > sq_dist))
        {
          x0 = pin->X;
          y0 = pin->Y;
          nearest = sq_dist;
        }
    }

  if (TEST_FLAG (SNAPPINFLAG, PCB))
    ans = SearchScreenGridSlop (Crosshair.X, Crosshair.Y,
                                LINEPOINT_TYPE, &ptr1, &ptr2, &ptr3);
  else
    ans = NO_TYPE;

  if (ans)
    {
      PointTypePtr pnt = (PointTypePtr) ptr3;
      sq_dist = square (pnt->X - Crosshair.X) + square (pnt->Y - Crosshair.Y);
      if ((nearest == -1 || sq_dist < nearest) &&
          (!gui->shift_is_pressed() ||
           square (x0 - Crosshair.X) + square (y0 - Crosshair.Y) > sq_dist))
        {
          x0 = pnt->X;
          y0 = pnt->Y;
          nearest = sq_dist;
        }
    }

  if (TEST_FLAG (SNAPPINFLAG, PCB))
    ans = SearchScreenGridSlop (Crosshair.X, Crosshair.Y,
                                POLYGONPOINT_TYPE, &ptr1, &ptr2, &ptr3);
  else
    ans = NO_TYPE;

  if (ans)
    {
      PointTypePtr pnt = (PointTypePtr) ptr3;
      sq_dist = square (pnt->X - Crosshair.X) + square (pnt->Y - Crosshair.Y);
      if ((nearest == -1 || sq_dist < nearest) &&
          (!gui->shift_is_pressed() ||
           square (x0 - Crosshair.X) + square (y0 - Crosshair.Y) > sq_dist))
        {
          x0 = pnt->X;
          y0 = pnt->Y;
          nearest = sq_dist;
        }
    }


  if (PCB->RatDraw || TEST_FLAG (SNAPPINFLAG, PCB))
    ans = SearchScreenGridSlop (Crosshair.X, Crosshair.Y,
                                ELEMENT_TYPE, &ptr1, &ptr2, &ptr3);
  else
    ans = NO_TYPE;

  if (ans & ELEMENT_TYPE)
    {
      ElementTypePtr el = (ElementTypePtr) ptr1;
      sq_dist = square (el->MarkX - Crosshair.X) + square (el->MarkY - Crosshair.Y);
      if ((nearest == -1 || sq_dist < nearest) &&
           square (x0 - Crosshair.X) + square (y0 - Crosshair.Y) > sq_dist)
        {
          x0 = el->MarkX;
          y0 = el->MarkY;
          nearest = sq_dist;
        }
    }

  if (x0 >= 0 && y0 >= 0)
    {
      Crosshair.X = x0;
      Crosshair.Y = y0;
    }

  if (Settings.Mode == ARROW_MODE)
    {
      ans = SearchScreenGridSlop (Crosshair.X, Crosshair.Y,
                                  LINEPOINT_TYPE, &ptr1, &ptr2, &ptr3);
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
 * move crosshair relative (has to be switched off)
 */
void
MoveCrosshairRelative (LocationType DeltaX, LocationType DeltaY)
{
  FitCrosshairIntoGrid (Crosshair.X + DeltaX, Crosshair.Y + DeltaY);
}

/* ---------------------------------------------------------------------------
 * move crosshair absolute
 * return true if the crosshair was moved from its existing position
 */
bool
MoveCrosshairAbsolute (LocationType X, LocationType Y)
{
  LocationType x, y, z;
  x = Crosshair.X;
  y = Crosshair.Y;
  FitCrosshairIntoGrid (X, Y);
  if (Crosshair.X != x || Crosshair.Y != y)
    {
      /* back up to old position to notify the GUI
       * (which might want to erase the old crosshair) */
      z = Crosshair.X;
      Crosshair.X = x;
      x = z;
      z = Crosshair.Y;
      Crosshair.Y = y;
      notify_crosshair_change (false); /* Our caller notifies when it has done */
      /* now move forward again */
      Crosshair.X = x;
      Crosshair.Y = z;
      return (true);
    }
  return (false);
}

/* ---------------------------------------------------------------------------
 * sets the valid range for the crosshair cursor
 */
void
SetCrosshairRange (LocationType MinX, LocationType MinY, LocationType MaxX,
		   LocationType MaxY)
{
  Crosshair.MinX = MAX (0, MinX);
  Crosshair.MinY = MAX (0, MinY);
  Crosshair.MaxX = MIN ((LocationType) PCB->MaxWidth, MaxX);
  Crosshair.MaxY = MIN ((LocationType) PCB->MaxHeight, MaxY);

  /* force update of position */
  MoveCrosshairRelative (0, 0);
}

/* ---------------------------------------------------------------------------
 * initializes crosshair stuff
 * clears the struct, allocates to graphical contexts and
 * initializes the stack
 */
void
InitCrosshair (void)
{
  Crosshair.GC = gui->make_gc ();

  gui->set_color (Crosshair.GC, Settings.CrosshairColor);
  gui->set_draw_xor (Crosshair.GC, 1);
  gui->set_line_cap (Crosshair.GC, Trace_Cap);
  gui->set_line_width (Crosshair.GC, 1);

  /* fake a crosshair off entry on stack */
  CrosshairStackLocation = 0;
  CrosshairStack[CrosshairStackLocation++] = true;
  Crosshair.On = true;

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
  gui->destroy_gc (Crosshair.GC);
}
