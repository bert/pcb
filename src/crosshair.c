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

static char *rcsid =
  "$Id$";

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
#include "gui.h"
#include "misc.h"
#include "mymem.h"
#include "search.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define ABS(x) (((x)<0)?-(x):(x))

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static Boolean CrosshairStack[MAX_CROSSHAIRSTACK_DEPTH];
static int CrosshairStackLocation = 0;
static XPoint *Points = NULL;	/* data of tmp polygon */
static Cardinal MaxPoints = 0;	/* number of points */

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void CreateTMPPolygon (PolygonTypePtr, Location, Location);
static void DrawCrosshair (void);
static void XORDrawElement (ElementTypePtr, Location, Location);
static void XORDrawBuffer (BufferTypePtr);
static void XORDrawInsertPointObject (void);
static void XORDrawMoveOrCopyObject (void);
static void XORDrawAttachedLine (Location, Location, Location, Location,
				 BDimension);
static void XORDrawAttachedArc (BDimension);
static void DrawAttached (Boolean);

/* ---------------------------------------------------------------------------
 * creates a tmp polygon with coordinates converted to screen system
 */
static void
CreateTMPPolygon (PolygonTypePtr Polygon, Location DX, Location DY)
{
  /* allocate memory for data with screen coordinates */
  if (Polygon->PointN >= MaxPoints)
    {
      /* allocate memory for one additional point */
      MaxPoints = Polygon->PointN + 1;
      Points = (XPoint *) MyRealloc (Points,
				     MaxPoints * sizeof (XPoint),
				     "CreateTMPPolygon()");
    }

  /* copy data to tmp array and convert it to screen coordinates */
  POLYGONPOINT_LOOP (Polygon, 
    {
      Points[n].x = TO_SCREEN_X (point->X + DX);
      Points[n].y = TO_SCREEN_Y (point->Y + DY);
    }
  );

  /* the last point is identical to the first one */
  Points[Polygon->PointN].x = Points[0].x;
  Points[Polygon->PointN].y = Points[0].y;
}

/* ---------------------------------------------------------------------------
 * draws the crosshair 
 * don't transform MAX_COORD to screen coordinates, it is
 * already the maximum of screen and pcb-coordinates
 */
static void
DrawCrosshair (void)
{
  XSetForeground (Dpy, Crosshair.GC, Settings.CrossColor);
  XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
	     TO_SCREEN_X (Crosshair.X), 0,
	     TO_SCREEN_X (Crosshair.X), MAX_COORD / 100);

  XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
	     0, TO_SCREEN_Y (Crosshair.Y), MAX_COORD / 100,
	     TO_SCREEN_Y (Crosshair.Y));
  XSetForeground (Dpy, Crosshair.GC, Settings.CrosshairColor);
}

/*-----------------------------------------------------------
 * Draws the outline of an arc
 */
static void
XORDrawAttachedArc (BDimension thick)
{
  ArcType arc;
  BoxTypePtr bx;
  Location wx, wy;
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
  arc.X -= wy;
  arc.Y -= TO_SCREEN_SIGN_Y (wy);
  sa = (sa - 180) * 64;
  dir *= 64;
  XDrawArc (Dpy, Output.OutputWindow, Crosshair.GC,
	    TO_SCREEN_X (arc.X - wid),
	    TO_SCREEN_Y (arc.Y - TO_SCREEN_SIGN_Y (wid)),
	    TO_SCREEN (2 * wy + thick), TO_SCREEN (2 * wy + thick),
	    TO_SCREEN_ANGLE (sa), TO_SCREEN_DELTA (dir));
  if (TO_SCREEN (wid) && (2 * wy - thick) > 0 && TO_SCREEN (2 * wy - thick))
    {
      XDrawArc (Dpy, Output.OutputWindow, Crosshair.GC,
		TO_SCREEN_X (arc.X + wid),
		TO_SCREEN_Y (arc.Y + TO_SCREEN_SIGN_Y (wid)),
		TO_SCREEN (2 * wy - thick), TO_SCREEN (2 * wy - thick),
		TO_SCREEN_ANGLE (sa), TO_SCREEN_DELTA (dir));
      XDrawArc (Dpy, Output.OutputWindow, Crosshair.GC,
		TO_SCREEN_X (bx->X1 - wid),
		TO_SCREEN_Y (bx->Y1 - TO_SCREEN_SIGN_Y (wid)),
		TO_SCREEN (thick), TO_SCREEN (thick), TO_SCREEN_ANGLE (sa),
		TO_SCREEN_DELTA (-11520 * SGN (dir)));
      XDrawArc (Dpy, Output.OutputWindow, Crosshair.GC,
		TO_SCREEN_X (bx->X2 - wid),
		TO_SCREEN_Y (bx->Y2 - TO_SCREEN_SIGN_Y (wid)),
		TO_SCREEN (thick), TO_SCREEN (thick),
		TO_SCREEN_ANGLE (sa + dir),
		TO_SCREEN_DELTA (11520 * SGN (dir)));
    }
}

/*-----------------------------------------------------------
 * Draws the outline of a line
 */
static void
XORDrawAttachedLine (Location x1, Location y1, Location x2,
		     Location y2, BDimension thick)
{
  Location dx, dy, ox, oy;
  float h;
  BDimension wid = thick / 2;

  dx = x2 - x1;
  dy = y2 - y1;
  if (dx != 0 || dy != 0)
    h = 0.5 * thick / sqrt ((float) ((float) dx * dx + (float) dy * dy));
  else
    h = 0.0;
  ox = dy * h + 0.5 * SGN (dy);
  oy = -(dx * h + 0.5 * SGN (dx));
  XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
	     TO_SCREEN_X (x1 + ox), TO_SCREEN_Y (y1 + oy),
	     TO_SCREEN_X (x2 + ox), TO_SCREEN_Y (y2 + oy));
  if (TO_SCREEN (abs (ox)) || TO_SCREEN (abs (oy)))
    {
      Location angle =
	TO_SCREEN_ANGLE (atan2 ((float) dx, (float) dy) * 3666.9298888);
      XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
		 TO_SCREEN_X (x1 - ox), TO_SCREEN_Y (y1 - oy),
		 TO_SCREEN_X (x2 - ox), TO_SCREEN_Y (y2 - oy));
      XDrawArc (Dpy, Output.OutputWindow, Crosshair.GC,
		TO_SCREEN_X (x1 - wid),
		TO_SCREEN_Y (y1 - TO_SCREEN_SIGN_Y (wid)), TO_SCREEN (thick),
		TO_SCREEN (thick), angle, TO_SCREEN_DELTA (11520));
      XDrawArc (Dpy, Output.OutputWindow, Crosshair.GC,
		TO_SCREEN_X (x2 - wid),
		TO_SCREEN_Y (y2 - TO_SCREEN_SIGN_Y (wid)), TO_SCREEN (thick),
		TO_SCREEN (thick), angle, TO_SCREEN_DELTA (-11520));
    }
}

/* ---------------------------------------------------------------------------
 * draws the elements of a loaded circuit which is to be merged in
 */
static void
XORDrawElement (ElementTypePtr Element, Location DX, Location DY)
{
  /* if no silkscreen, draw the bounding box */
  if (Element->ArcN == 0 && Element->LineN == 0)
    {
      XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
		 TO_SCREEN_X (DX + Element->BoundingBox.X1),
		 TO_SCREEN_Y (DY + Element->BoundingBox.Y1),
		 TO_SCREEN_X (DX + Element->BoundingBox.X1),
		 TO_SCREEN_Y (DY + Element->BoundingBox.Y2));
      XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
		 TO_SCREEN_X (DX + Element->BoundingBox.X1),
		 TO_SCREEN_Y (DY + Element->BoundingBox.Y2),
		 TO_SCREEN_X (DX + Element->BoundingBox.X2),
		 TO_SCREEN_Y (DY + Element->BoundingBox.Y2));
      XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
		 TO_SCREEN_X (DX + Element->BoundingBox.X2),
		 TO_SCREEN_Y (DY + Element->BoundingBox.Y2),
		 TO_SCREEN_X (DX + Element->BoundingBox.X2),
		 TO_SCREEN_Y (DY + Element->BoundingBox.Y1));
      XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
		 TO_SCREEN_X (DX + Element->BoundingBox.X2),
		 TO_SCREEN_Y (DY + Element->BoundingBox.Y1),
		 TO_SCREEN_X (DX + Element->BoundingBox.X1),
		 TO_SCREEN_Y (DY + Element->BoundingBox.Y1));
    }
  else
    {
      ELEMENTLINE_LOOP (Element, 
	{
	  XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
		     TO_SCREEN_X (DX + line->Point1.X),
		     TO_SCREEN_Y (DY + line->Point1.Y),
		     TO_SCREEN_X (DX + line->Point2.X),
		     TO_SCREEN_Y (DY + line->Point2.Y));
	}
      );

      /* arc coordinates and angles have to be converted to X11 notation */
      ARC_LOOP (Element, 
	{
	  XDrawArc (Dpy, Output.OutputWindow, Crosshair.GC,
		    TO_SCREEN_X (DX + arc->X) - TO_SCREEN (arc->Width),
		    TO_SCREEN_Y (DY + arc->Y) - TO_SCREEN (arc->Height),
		    TO_SCREEN (2 * arc->Width),
		    TO_SCREEN (2 * arc->Height),
		    (TO_SCREEN_ANGLE (arc->StartAngle) + 180) * 64,
		    TO_SCREEN_DELTA (arc->Delta) * 64);
	}
      );
    }
  /* pin coordinates and angles have to be converted to X11 notation */
  PIN_LOOP (Element, 
    {
      XDrawArc (Dpy, Output.OutputWindow, Crosshair.GC,
		TO_SCREEN_X (DX + pin->X) -
		TO_SCREEN (pin->Thickness / 2),
		TO_SCREEN_Y (DY + pin->Y) -
		TO_SCREEN (pin->Thickness / 2),
		TO_SCREEN (pin->Thickness), TO_SCREEN (pin->Thickness),
		0, 360 * 64);
    }
  );

  /* pads */
  PAD_LOOP (Element, 
    {
      if ((TEST_FLAG (ONSOLDERFLAG, pad) != 0) ==
	  Settings.ShowSolderSide || PCB->InvisibleObjectsOn)
	XDrawLine (Dpy,
		   Output.OutputWindow,
		   Crosshair.GC,
		   TO_SCREEN_X (DX +
				pad->Point1.
				X),
		   TO_SCREEN_Y (DY +
				pad->Point1.
				Y),
		   TO_SCREEN_X (DX +
				pad->Point2.
				X), TO_SCREEN_Y (DY + pad->Point2.Y));
    }
  );
  /* mark */
  XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
	     TO_SCREEN_X (Element->MarkX + DX - EMARK_SIZE),
	     TO_SCREEN_Y (Element->MarkY + DY),
	     TO_SCREEN_X (Element->MarkX + DX),
	     TO_SCREEN_Y (Element->MarkY + DY - EMARK_SIZE));
  XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
	     TO_SCREEN_X (Element->MarkX + DX + EMARK_SIZE),
	     TO_SCREEN_Y (Element->MarkY + DY),
	     TO_SCREEN_X (Element->MarkX + DX),
	     TO_SCREEN_Y (Element->MarkY + DY - EMARK_SIZE));
  XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
	     TO_SCREEN_X (Element->MarkX + DX - EMARK_SIZE),
	     TO_SCREEN_Y (Element->MarkY + DY),
	     TO_SCREEN_X (Element->MarkX + DX),
	     TO_SCREEN_Y (Element->MarkY + DY + EMARK_SIZE));
  XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
	     TO_SCREEN_X (Element->MarkX + DX + EMARK_SIZE),
	     TO_SCREEN_Y (Element->MarkY + DY),
	     TO_SCREEN_X (Element->MarkX + DX),
	     TO_SCREEN_Y (Element->MarkY + DY + EMARK_SIZE));
}

/* ---------------------------------------------------------------------------
 * draws all visible and attached objects of the pastebuffer
 */
static void
XORDrawBuffer (BufferTypePtr Buffer)
{
  Cardinal i;
  Location x, y;

  /* set offset */
  x = Crosshair.X - Buffer->X;
  y = Crosshair.Y - Buffer->Y;

  /* draw all visible layers */
  for (i = 0; i < MAX_LAYER + 2; i++)
    if (PCB->Data->Layer[i].On)
      {
	LayerTypePtr layer = &Buffer->Data->Layer[i];

	LINE_LOOP (layer, 
	  {
/*
				XORDrawAttachedLine(x +line->Point1.X,
					y +line->Point1.Y, x +line->Point2.X,
					y +line->Point2.Y, line->Thickness);
*/
	    XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
		       TO_SCREEN_X (x + line->Point1.X),
		       TO_SCREEN_Y (y + line->Point1.Y),
		       TO_SCREEN_X (x + line->Point2.X),
		       TO_SCREEN_Y (y + line->Point2.Y));
	  }
	);
	ARC_LOOP (layer, 
	  {
	    XDrawArc (Dpy, Output.OutputWindow, Crosshair.GC,
		      TO_SCREEN_X (x + arc->X) - TO_SCREEN (arc->Width),
		      TO_SCREEN_Y (y + arc->Y) -
		      TO_SCREEN (arc->Height),
		      TO_SCREEN (2 * arc->Width),
		      TO_SCREEN (2 * arc->Height),
		      (TO_SCREEN_ANGLE (arc->StartAngle) - 180) * 64,
		      TO_SCREEN_DELTA (arc->Delta) * 64);
	  }
	);
	TEXT_LOOP (layer, 
	  {
	    BoxTypePtr box = &text->BoundingBox;
	    Location y0;
	    y0 = Settings.ShowSolderSide ? box->Y2 : box->Y1;
	    XDrawRectangle (Dpy, Output.OutputWindow, Crosshair.GC,
			    TO_SCREEN_X (x + box->X1),
			    TO_SCREEN_Y (y + y0),
			    TO_SCREEN (box->X2 - box->X1),
			    TO_SCREEN (box->Y2 - box->Y1));
	  }
	);
	/* the tmp polygon has n+1 points because the first
	 * and the last one are set to the same coordinates
	 */
	POLYGON_LOOP (layer, 
	  {
	    CreateTMPPolygon (polygon, x, y);
	    XDrawLines (Dpy, Output.OutputWindow, Crosshair.GC,
			Points, polygon->PointN + 1, CoordModeOrigin);
	  }
	);
      }

  /* draw elements if visible */
  if (PCB->PinOn && PCB->ElementOn)
    ELEMENT_LOOP (Buffer->Data, 
    {
      if (FRONT (element) || PCB->InvisibleObjectsOn)
	XORDrawElement (element, x, y);
    }
  );

  /* and the vias, move offset by thickness/2 */
  if (PCB->ViaOn)
    VIA_LOOP (Buffer->Data, 
    {
      XDrawArc (Dpy, Output.OutputWindow, Crosshair.GC,
		TO_SCREEN_X (x + via->X - via->Thickness / 2),
		TO_SCREEN_Y (y + via->Y -
			     TO_SCREEN_SIGN_Y (via->Thickness / 2)),
		TO_SCREEN (via->Thickness),
		TO_SCREEN (via->Thickness), 0, 360 * 64);
    }
  );
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
      XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
		 TO_SCREEN_X (point->X), TO_SCREEN_Y (point->Y),
		 TO_SCREEN_X (line->Point1.X), TO_SCREEN_Y (line->Point1.Y));
      XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
		 TO_SCREEN_X (point->X), TO_SCREEN_Y (point->Y),
		 TO_SCREEN_X (line->Point2.X), TO_SCREEN_Y (line->Point2.Y));
    }

}

/* ---------------------------------------------------------------------------
 * draws the attched object while in MOVE_MODE or COPY_MODE
 */
static void
XORDrawMoveOrCopyObject (void)
{
  RubberbandTypePtr ptr;
  Cardinal i;
  Location dx = Crosshair.X - Crosshair.AttachedObject.X,
    dy = Crosshair.Y - Crosshair.AttachedObject.Y;

  switch (Crosshair.AttachedObject.Type)
    {
    case VIA_TYPE:
      {
	PinTypePtr via = (PinTypePtr) Crosshair.AttachedObject.Ptr1;

	XDrawArc (Dpy, Output.OutputWindow, Crosshair.GC,
		  TO_SCREEN_X (via->X + dx - via->Thickness / 2),
		  TO_SCREEN_Y (via->Y + dy) - TO_SCREEN (via->Thickness / 2),
		  TO_SCREEN (via->Thickness),
		  TO_SCREEN (via->Thickness), 0, 360 * 64);
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

	XDrawArc (Dpy, Output.OutputWindow, Crosshair.GC,
		  TO_SCREEN_X (Arc->X + dx) - TO_SCREEN (Arc->Width),
		  TO_SCREEN_Y (Arc->Y + dy) - TO_SCREEN (Arc->Height),
		  TO_SCREEN (2 * Arc->Width),
		  TO_SCREEN (2 * Arc->Height),
		  (TO_SCREEN_ANGLE (Arc->StartAngle) - 180) * 64,
		  TO_SCREEN_DELTA (Arc->Delta) * 64);
	break;
      }

    case POLYGON_TYPE:
      {
	PolygonTypePtr polygon =
	  (PolygonTypePtr) Crosshair.AttachedObject.Ptr2;

	/* the tmp polygon has n+1 points because the first
	 * and the last one are set to the same coordinates
	 */
	CreateTMPPolygon (polygon, dx, dy);
	XDrawLines (Dpy, Output.OutputWindow, Crosshair.GC,
		    Points, polygon->PointN + 1, CoordModeOrigin);
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
	PointTypePtr point, previous, following;

	polygon = (PolygonTypePtr) Crosshair.AttachedObject.Ptr2;
	point = (PointTypePtr) Crosshair.AttachedObject.Ptr3;

	/* get previous and following point */
	if (point == polygon->Points)
	  {
	    previous = &polygon->Points[polygon->PointN - 1];
	    following = point + 1;
	  }
	else if (point == &polygon->Points[polygon->PointN - 1])
	  {
	    previous = point - 1;
	    following = &polygon->Points[0];
	  }
	else
	  {
	    previous = point - 1;
	    following = point + 1;
	  }

	/* draw the two segments */
	XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
		   TO_SCREEN_X (previous->X),
		   TO_SCREEN_Y (previous->Y),
		   TO_SCREEN_X (point->X + dx), TO_SCREEN_Y (point->Y + dy));
	XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
		   TO_SCREEN_X (point->X + dx),
		   TO_SCREEN_Y (point->Y + dy),
		   TO_SCREEN_X (following->X), TO_SCREEN_Y (following->Y));
	break;
      }

    case ELEMENTNAME_TYPE:
      {
	/* locate the element "mark" and draw an association line from crosshair to it */
	ElementTypePtr element =
	  (ElementTypePtr) Crosshair.AttachedObject.Ptr1;

	XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
		   TO_SCREEN_X (element->MarkX),
		   TO_SCREEN_Y (element->MarkY),
		   TO_SCREEN_X (Crosshair.X), TO_SCREEN_Y (Crosshair.Y));
	/* fall through to move the text as a box outline */
      }
    case TEXT_TYPE:
      {
	TextTypePtr text = (TextTypePtr) Crosshair.AttachedObject.Ptr2;
	BoxTypePtr box = &text->BoundingBox;
	int x0, y0;

	x0 = box->X1;
	y0 = Settings.ShowSolderSide ? box->Y2 : box->Y1;
	XDrawRectangle (Dpy, Output.OutputWindow, Crosshair.GC,
			TO_SCREEN_X (x0 + dx),
			TO_SCREEN_Y (y0 + dy),
			TO_SCREEN (box->X2 - box->X1),
			TO_SCREEN (box->Y2 - box->Y1));
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

      if (ptr->Line->Flags & RUBBERENDFLAG)
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
static void
DrawAttached (Boolean BlockToo)
{
  BDimension s;
  DrawCrosshair ();
  switch (Settings.Mode)
    {
    case VIA_MODE:
      XDrawArc (Dpy, Output.OutputWindow, Crosshair.GC,
		TO_SCREEN_X (Crosshair.X - Settings.ViaThickness / 2),
		TO_SCREEN_Y (Crosshair.Y) -
		TO_SCREEN (Settings.ViaThickness / 2),
		TO_SCREEN (Settings.ViaThickness),
		TO_SCREEN (Settings.ViaThickness), 0, 360 * 64);
      if (TEST_FLAG (SHOWDRCFLAG, PCB))
        {
          s = Settings.ViaThickness + 2*(Settings.Bloat+1);
          XSetForeground (Dpy, Crosshair.GC, Settings.CrossColor);
          XDrawArc (Dpy, Output.OutputWindow, Crosshair.GC,
		    TO_SCREEN_X (Crosshair.X - s / 2),
		    TO_SCREEN_Y (Crosshair.Y) -
		    TO_SCREEN (s / 2),
		    TO_SCREEN (s),
		    TO_SCREEN (s), 0, 360 * 64);
          XSetForeground (Dpy, Crosshair.GC, Settings.CrosshairColor);
        }
      break;

      /* the attached line is used by both LINEMODE and POLYGON_MODE */
    case POLYGON_MODE:
      /* draw only if starting point is set */
      if (Crosshair.AttachedLine.State != STATE_FIRST)
	XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
		   TO_SCREEN_X (Crosshair.AttachedLine.Point1.X),
		   TO_SCREEN_Y (Crosshair.AttachedLine.Point1.Y),
		   TO_SCREEN_X (Crosshair.AttachedLine.Point2.X),
		   TO_SCREEN_Y (Crosshair.AttachedLine.Point2.Y));

      /* draw attached polygon only if in POLYGON_MODE */
      if (Crosshair.AttachedPolygon.PointN > 1)
	{
	  CreateTMPPolygon (&Crosshair.AttachedPolygon, 0, 0);
	  XDrawLines (Dpy, Output.OutputWindow, Crosshair.GC,
		      Points, Crosshair.AttachedPolygon.PointN,
		      CoordModeOrigin);
	}
      break;

    case ARC_MODE:
      if (Crosshair.AttachedBox.State != STATE_FIRST)
        {
	  XORDrawAttachedArc (Settings.LineThickness);
          if (TEST_FLAG (SHOWDRCFLAG, PCB))
	    {
              XSetForeground (Dpy, Crosshair.GC, Settings.CrossColor);
	      XORDrawAttachedArc (Settings.LineThickness + 2*(Settings.Bloat+1));
              XSetForeground (Dpy, Crosshair.GC, Settings.CrosshairColor);
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
              XSetForeground (Dpy, Crosshair.GC, Settings.CrossColor);
	      XORDrawAttachedLine (Crosshair.AttachedLine.Point1.X,
			           Crosshair.AttachedLine.Point1.Y,
			           Crosshair.AttachedLine.Point2.X,
			           Crosshair.AttachedLine.Point2.Y,
			           PCB->RatDraw ? 10 : Settings.LineThickness
				   + 2*(Settings.Bloat+1));
	      if (PCB->Clipping)
	        XORDrawAttachedLine (Crosshair.AttachedLine.Point2.X,
				     Crosshair.AttachedLine.Point2.Y,
				     Crosshair.X, Crosshair.Y,
				     PCB->RatDraw ? 10 : Settings.LineThickness +
				     2*(Settings.Bloat+1));
              XSetForeground (Dpy, Crosshair.GC, Settings.CrosshairColor);
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
      (BlockToo && Crosshair.AttachedBox.State == STATE_THIRD))
    {
      Location x1, y1, x2, y2, y0;

      x1 =
	MIN (Crosshair.AttachedBox.Point1.X, Crosshair.AttachedBox.Point2.X);
      y1 =
	MIN (Crosshair.AttachedBox.Point1.Y, Crosshair.AttachedBox.Point2.Y);
      x2 =
	MAX (Crosshair.AttachedBox.Point1.X, Crosshair.AttachedBox.Point2.X);
      y2 =
	MAX (Crosshair.AttachedBox.Point1.Y, Crosshair.AttachedBox.Point2.Y);
      y0 = Settings.ShowSolderSide ? y2 : y1;
      XDrawRectangle (Dpy, Output.OutputWindow, Crosshair.GC,
		      TO_SCREEN_X (x1),
		      TO_SCREEN_Y (y0),
		      TO_SCREEN (x2 - x1), TO_SCREEN (y2 - y1));
    }
}

/* ---------------------------------------------------------------------------
 * switches crosshair on
 */
void
CrosshairOn (Boolean BlockToo)
{
  if (!Crosshair.On)
    {
      Crosshair.On = True;
      XSync (Dpy, False);
      DrawAttached (BlockToo);
      DrawMark (True);
    }
}

/* ---------------------------------------------------------------------------
 * switches crosshair off
 */
void
CrosshairOff (Boolean BlockToo)
{
  if (Crosshair.On)
    {
      Crosshair.On = False;
      XSync (Dpy, False);
      DrawAttached (BlockToo);
      DrawMark (True);
    }
}

/* ---------------------------------------------------------------------------
 * saves crosshair state (on/off) and hides him
 */
void
HideCrosshair (Boolean BlockToo)
{
  CrosshairStack[CrosshairStackLocation++] = Crosshair.On;
  if (CrosshairStackLocation >= MAX_CROSSHAIRSTACK_DEPTH)
    CrosshairStackLocation--;
  CrosshairOff (BlockToo);
}

/* ---------------------------------------------------------------------------
 * restores last crosshair state
 */
void
RestoreCrosshair (Boolean BlockToo)
{
  if (CrosshairStackLocation)
    {
      if (CrosshairStack[--CrosshairStackLocation])
	CrosshairOn (BlockToo);
      else
	CrosshairOff (BlockToo);
    }
}

/* ---------------------------------------------------------------------------
 * recalculates the passed coordinates to fit the current grid setting
 */
void
FitCrosshairIntoGrid (Location X, Location Y)
{
  Location x2, y2, x0, y0;
  void *ptr1, *ptr2, *ptr3;
  int ans;

  /* get PCB coordinates from visible display size.
   * If the bottom view mode is active, y2 might be less then y1
   */
  y0 = TO_PCB_Y (0);
  y2 = TO_PCB_Y (Output.Height - 1);
  if (y2 < y0)
    {
      x0 = y0;
      y0 = y2;
      y2 = x0;
    }
  x0 = TO_PCB_X (0);
  x2 = TO_PCB_X (Output.Width - 1);

  /* check position agains window size and against valid
   * coordinates determined by the size of an attached
   * object or buffer
   */
  Crosshair.X = MIN (x2, MAX (x0, X));
  Crosshair.Y = MIN (y2, MAX (y0, Y));
  Crosshair.X = MIN (Crosshair.MaxX, MAX (Crosshair.MinX, Crosshair.X));
  Crosshair.Y = MIN (Crosshair.MaxY, MAX (Crosshair.MinY, Crosshair.Y));

  if (PCB->RatDraw || TEST_FLAG (SNAPPINFLAG, PCB))
    {
      ans =
	SearchScreen (Crosshair.X, Crosshair.Y,
		      PAD_TYPE | PIN_TYPE, &ptr1, &ptr2, &ptr3);
      if (ans == NO_TYPE && !PCB->RatDraw)
	ans = SearchScreen (Crosshair.X, Crosshair.Y, VIA_TYPE | LINEPOINT_TYPE,
			    &ptr1, &ptr2, &ptr3);
    }
  else
    ans = NO_TYPE;

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
  if (ans & PAD_TYPE)
    {
      PadTypePtr pad = (PadTypePtr) ptr2;
      if (pad->Point1.X == pad->Point2.X)
	{
	  if (abs (pad->Point1.X - Crosshair.X) < abs (x0 - Crosshair.X))
	    x0 = pad->Point1.X;
	  if (abs (pad->Point1.Y - Crosshair.Y) < abs (y0 - Crosshair.Y))
	    y0 = pad->Point1.Y;
	  if (abs (pad->Point2.Y - Crosshair.Y) < abs (y0 - Crosshair.Y))
	    y0 = pad->Point2.Y;
	}
      else
	{
	  if (abs (pad->Point1.Y - Crosshair.Y) < abs (y0 - Crosshair.Y))
	    y0 = pad->Point1.Y;
	  if (abs (pad->Point1.X - Crosshair.X) < abs (x0 - Crosshair.X))
	    x0 = pad->Point1.X;
	  if (abs (pad->Point2.X - Crosshair.X) < abs (x0 - Crosshair.X))
	    x0 = pad->Point2.X;
	}
    }
  else if (ans & (PIN_TYPE | VIA_TYPE))
    {
      PinTypePtr pin = (PinTypePtr) ptr2;
      if (((x0 - Crosshair.X) * (x0 - Crosshair.X) +
	   (y0 - Crosshair.Y) * (y0 - Crosshair.Y)) >
	  ((pin->X - Crosshair.X) * (pin->X - Crosshair.X) +
	   (pin->Y - Crosshair.Y) * (pin->Y - Crosshair.Y)))
	{
	  x0 = pin->X;
	  y0 = pin->Y;
	}
    }
  else if (ans & LINEPOINT_TYPE)
    {
      PointTypePtr pnt = (PointTypePtr) ptr3;
      if (((x0 - Crosshair.X) * (x0 - Crosshair.X) +
	   (y0 - Crosshair.Y) * (y0 - Crosshair.Y)) >
	  ((pnt->X - Crosshair.X) * (pnt->X - Crosshair.X) +
	   (pnt->Y - Crosshair.Y) * (pnt->Y - Crosshair.Y)))
	{
	  x0 = pnt->X;
	  y0 = pnt->Y;
	}
    }
  if (x0 >= 0 && y0 >= 0)
    {
      Crosshair.X = x0;
      Crosshair.Y = y0;
    }
}

/* ---------------------------------------------------------------------------
 * move crosshair relative (has to be switched off)
 */
void
MoveCrosshairRelative (Location DeltaX, Location DeltaY)
{
  FitCrosshairIntoGrid (Crosshair.X + DeltaX, Crosshair.Y + DeltaY);
}

/* ---------------------------------------------------------------------------
 * move crosshair absolute switched off if it moved
 * return True if it switched off
 */
Boolean
MoveCrosshairAbsolute (Location X, Location Y)
{
  Location x, y, z;
  x = Crosshair.X;
  y = Crosshair.Y;
  FitCrosshairIntoGrid (X, Y);
  if (Crosshair.X != x || Crosshair.Y != y)
    {
      /* back up to old position and erase crosshair */
      z = Crosshair.X;
      Crosshair.X = x;
      x = z;
      z = Crosshair.Y;
      Crosshair.Y = y;
      HideCrosshair (False);
      /* now move forward again */
      Crosshair.X = x;
      Crosshair.Y = z;
      return (True);
    }
  return (False);
}

/* ---------------------------------------------------------------------------
 * sets the valid range for the crosshair cursor
 */
void
SetCrosshairRange (Location MinX, Location MinY, Location MaxX, Location MaxY)
{
  Crosshair.MinX = MAX (0, MinX);
  Crosshair.MinY = MAX (0, MinY);
  Crosshair.MaxX = MIN ((Location) PCB->MaxWidth, MaxX);
  Crosshair.MaxY = MIN ((Location) PCB->MaxHeight, MaxY);

  /* force update of position */
  MoveCrosshairRelative (0, 0);
}

/* --------------------------------------------------------------------------
 * draw the marker position
 * if argument is True, draw only if it is visible, otherwise draw it regardless
 */
void
DrawMark (Boolean ifvis)
{
  XSync (Dpy, False);
  if (Marked.status || !ifvis)
    {
      XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
		 TO_SCREEN_X (Marked.X - MARK_SIZE),
		 TO_SCREEN_Y (Marked.Y - MARK_SIZE),
		 TO_SCREEN_X (Marked.X + MARK_SIZE),
		 TO_SCREEN_Y (Marked.Y + MARK_SIZE));
      XDrawLine (Dpy, Output.OutputWindow, Crosshair.GC,
		 TO_SCREEN_X (Marked.X + MARK_SIZE),
		 TO_SCREEN_Y (Marked.Y - MARK_SIZE),
		 TO_SCREEN_X (Marked.X - MARK_SIZE),
		 TO_SCREEN_Y (Marked.Y + MARK_SIZE));
    }
}

/* ---------------------------------------------------------------------------
 * initializes crosshair stuff
 * clears the struct, allocates to graphical contexts and
 * initializes the stack
 */
void
InitCrosshair (void)
{
  /* clear struct */
  memset (&Crosshair, 0, sizeof (CrosshairType));

  Crosshair.GC = XCreateGC (Dpy, Output.OutputWindow, 0, NULL);
  if (!VALID_GC ((long int) Crosshair.GC))
    MyFatal ("can't create default crosshair GC\n");
  XSetState (Dpy, Crosshair.GC, Settings.CrosshairColor, Settings.bgColor,
	     GXxor, AllPlanes);
  XSetLineAttributes (Dpy, Crosshair.GC, 1, LineSolid, CapButt, JoinMiter);

  /* fake an crosshair off entry on stack */
  CrosshairStackLocation = 0;
  CrosshairStack[CrosshairStackLocation++] = True;
  Crosshair.On = False;

  /* set default limits */
  Crosshair.MinX = Crosshair.MinY = 0;
  Crosshair.MaxX = PCB->MaxWidth;
  Crosshair.MaxY = PCB->MaxHeight;

  /* clear the mark */
  Marked.status = False;
}

/* ---------------------------------------------------------------------------
 * exits crosshair routines, release GCs
 */
void
DestroyCrosshair (void)
{
  CrosshairOff (True);
  FreePolygonMemory (&Crosshair.AttachedPolygon);
  XFreeGC (Dpy, Crosshair.GC);
}
