/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
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

#define	SWAP_IDENT		SwapOutput
#define TO_SCREEN(a)	((Position)((a)*Local_Zoom))
#define XORIG dxo
#define YORIG dyo

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "global.h"
#include "data.h"
#include "draw.h"
#include "mymem.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID("$Id$");

LocationType dxo, dyo;
Boolean SwapOutput;
float Local_Zoom;
/* Clip the line to the clipBox
 * return True if something to be drawn
 * false if the whole thing is clipped
 */
static Boolean
ClipLine (PointType *p)
{
  float d, r;
  Boolean ans = True; 
  
  /* clip first point on left side */
  if (p[0].X < clipBox.X1)
    {
      if (p[1].X < clipBox.X1)
        {
	  ans = False;
	  p[0].X = p[1].X = clipBox.X1;
	  goto do_Y;
	}
      d = p[1].X - p[0].X;
      r = (clipBox.X1 - p[0].X)/d;
      p[0].X = clipBox.X1;
      p[0].Y += r * (p[1].Y - p[0].Y);
    }
  /* clip second point on left side */
  if (p[1].X < clipBox.X1)
    {
      d = p[0].X - p[1].X;
      r = (clipBox.X1 - p[1].X)/d;
      p[1].X = clipBox.X1;
      p[1].Y += r * (p[0].Y - p[1].Y);
    }
  /* clip first point on right side */
  if (p[0].X > clipBox.X2)
    {
      if (p[1].X > clipBox.X2)
        {
	  ans = False;
	  p[0].X = p[1].X = clipBox.X2;
	  goto do_Y;
	}
      d = p[1].X - p[0].X;
      r = (clipBox.X2 - p[0].X)/d;
      p[0].X = clipBox.X2;
      p[0].Y += r * (p[1].Y - p[0].Y);
    }
  /* clip second point on right side */
  if (p[1].X > clipBox.X2)
    {
      d = p[0].X - p[1].X;
      r = (clipBox.X2 - p[1].X)/d;
      p[1].X = clipBox.X2;
      p[1].Y += r * (p[0].Y - p[1].Y);
    }
do_Y:
  /* clip first point on top */
  if (p[0].Y < clipBox.Y1)
    {
      if (p[1].Y < clipBox.Y1)
        {
	  p[0].Y = p[1].Y = clipBox.Y1;
	  return False;
	}
      d = p[1].Y - p[0].Y;
      r = (clipBox.Y1 - p[0].Y)/d;
      p[0].Y = clipBox.Y1;
      p[0].X += r * (p[1].X - p[0].X);
    }
  /* clip second point on top */
  if (p[1].Y < clipBox.Y1)
    {
      d = p[0].Y - p[1].Y;
      r = (clipBox.Y1 - p[1].Y)/d;
      p[1].Y = clipBox.Y1;
      p[1].X += r * (p[0].X - p[1].X);
    }
  /* clip first point on bottom */
  if (p[0].Y > clipBox.Y2)
    {
      if (p[1].Y > clipBox.Y2)
        {
	  p[0].Y = p[1].Y = clipBox.Y2;
	  return False;
	}
      d = p[1].Y - p[0].Y;
      r = (clipBox.Y2 - p[0].Y)/d;
      p[0].Y = clipBox.Y2;
      p[0].X += r * (p[1].X - p[0].X);
    }
  /* clip second point on top */
  if (p[1].Y > clipBox.Y2)
    {
      d = p[0].Y - p[1].Y;
      r = (clipBox.Y2 - p[1].Y)/d;
      p[1].Y = clipBox.Y2;
      p[1].X += r * (p[0].X - p[1].X);
    }
  return ans;
}

/* clip a line to the displayed area */
void XDrawCLine (Display *dpy, Drawable d, GC gc, int x1,
                 int y1, int x2, int y2)
{
  PointType pts[2];

  pts[0].X = x1;
  pts[0].Y = y1;
  pts[1].X = x2;
  pts[1].Y = y2;

  if (ClipLine (pts))
    XDrawLine (dpy, d, gc, TO_DRAW_X (pts[0].X), TO_DRAW_Y (pts[0].Y),
               TO_DRAW_X (pts[1].X), TO_DRAW_Y (pts[1].Y));
}

/* clip an arc to the displayed area and draw it */
void XDrawCArc (Display *dpy, Drawable d, GC gc, int x, int y,
                unsigned int width, unsigned int height, int angle,
		int delta)
{
  int start = MIN(angle + delta, angle);
  int end = MAX (angle + delta, angle);
   /* force angles to standard range */
  while (end > 180*64)
    {
      start -= 360*64;
      end -= 360*64;
    }
  while (start < -180*64)
    {
      start += 360*64;
      end += 360*64;
    }
  if ((x + (LocationType)width) < clipBox.X1)
    return;
  if ((x - (LocationType)width) > clipBox.X2)
    return;
  if ((y + (LocationType)height) < clipBox.Y1)
    return;
  if ((y - (LocationType)height) > clipBox.Y2)
    return; /* nothing to draw */
    /* clip left edge */
  if (abs(2*(clipBox.X1 - x)) < width)
    {
      int theta = (64*RAD_TO_DEG) * acos (2.0*(clipBox.X1 - x)/(float)width);
      if (theta > start && theta < end)
        {
	  assert (start >= 0);
	  end = theta;
	}
      if ((-theta) > start && (-theta) < end)
        {
	  assert (start <= 0);
	  start = -theta;
	}
    }
    /* clip right edge */
  if (abs(2*(clipBox.X2 - x)) < width)
    {
      int theta = (64*RAD_TO_DEG) * acos (2.0*(clipBox.X2 - x)/(float)width);
      if (theta > start && theta < end)
        {
	  assert (start >= 0);
	  start = theta;
	}
      theta = - theta;
      if (theta > start && theta < end)
        {
	  assert (start <= 0);
	  end = theta;
	}
    }
    /* clip top */
  if (abs(2*(clipBox.Y1 - y)) < height)
    {
      int theta = (64*RAD_TO_DEG) * asin (2.0*( y - clipBox.Y1)/(float)height);
      if (theta > start && theta < end)
        end = theta;
      theta = SGN (theta) * 180*64 - theta;
      if (theta > start && theta < end)
	start = theta;
    }
    /* clip bottom */
  if (abs(2*(clipBox.Y2 - y)) < height)
    {
      int theta = (64*RAD_TO_DEG) * asin (2.0*(y - clipBox.Y2)/(float)height);
      if (theta > start && theta < end)
	start = theta;
      theta = SGN (theta) * 180*64 - theta;
      if (theta > start && theta < end)
	end = theta;
    }
  XDrawArc (dpy, d, gc, TO_DRAW_X(x) - TO_SCREEN(width)/2,
            TO_DRAW_Y(y) - TO_SCREEN(height)/2,
            TO_SCREEN(width), TO_SCREEN(height), TO_SCREEN_ANGLE(start),
	    TO_SCREEN_DELTA(end - start));
}

/* clip a polygon to the visible region and draw it */
void DrawCPolygon (Drawable Window, PolygonTypePtr Polygon)
{
  static int max = 0;
  static PointType *pts;
  static XPoint *data;
  Cardinal i, j = 0;

  /* allocate memory for data with screen coordinates */
  if (2*Polygon->PointN > max)
    {
      max = 2*Polygon->PointN + 20;
      pts = (PointType *) MyRealloc (pts, (max + 1) * sizeof (PointType),
				   "DrawPolygonLowLevel()");
      data = (XPoint *) MyRealloc (data, (max + 1) * sizeof (XPoint),
				   "DrawPolygonLowLevel()");
    }

  for (i = 0; i < Polygon->PointN - 1; i++)
    {
      pts[2*i] = Polygon->Points[i];
      pts[2*i+1] = Polygon->Points[i+1];
      ClipLine (&pts[2*i]);
    }
  pts[2*i] = Polygon->Points[i];
  pts[2*i+1] = Polygon->Points[0];
  ClipLine (&pts[2*i]);
  /* copy data to tmp array and convert it to screen coordinates */
  data[j].x = TO_DRAW_X (pts[0].X);
  data[j++].y = TO_DRAW_Y (pts[0].Y);
  for (i = 1; i < 2 * Polygon->PointN; i++)
    {
        /* skip duplicate points */
      if (pts[i-1].X != pts[i].X || pts[i-1].Y != pts[i].Y)
        {
	  data[j].x = TO_DRAW_X (pts[i].X);
          data[j++].y = TO_DRAW_Y (pts[i].Y);
	}
    }
  if (pts[i-1].X != pts[0].X || pts[i-1].Y != pts[0].Y)
    {
      data[j].x = TO_DRAW_X (pts[0].X);
      data[j].y = TO_DRAW_Y (pts[0].Y);
    }
  else
    j--;
  if (TEST_FLAG (THINDRAWFLAG, PCB))
    {
      data[Polygon->PointN] = data[0];
      XDrawLines (Dpy, Window, Output.fgGC,
    	         data, j + 1, CoordModeOrigin);
    }
  else
    XFillPolygon (Dpy, Window, Output.fgGC,
                 data, j, Complex, CoordModeOrigin);
}
