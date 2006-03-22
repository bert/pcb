/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2006 DJ Delorie
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
 *  DJ Delorie, 334 North Road, Deerfield NH 03037-1110, USA
 *  dj@delorie.com
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"

#include <math.h>
#include <memory.h>
#include <limits.h>


#include "create.h"
#include "data.h"
#include "draw.h"
#include "misc.h"
#include "move.h"
#include "remove.h"
#include "rtree.h"
#include "strflags.h"
#include "undo.h"

RCSID("$Id$");

#define sqr(x) (1.0*(x)*(x))

static int multi, line_exact, arc_exact;
static LineTypePtr the_line;
static ArcTypePtr the_arc;
static double arc_dist;

/* We canonicalize the arc and line such that the point to be moved is
   always Point2 for the line, and at start+delta for the arc.  */

static int x, y; /* the point we're moving */
static int cx, cy; /* centerpoint of the arc */
static int ex, ey; /* fixed end of the line */

/* 0 is left (-x), 90 is down (+y), 180 is right (+x), 270 is up (-y) */

static double
dist (int x1, int y1, int x2, int y2)
{
  double dx = x1 - x2;
  double dy = y1 - y2;
  double dist = sqrt(dx*dx+dy*dy);
  return dist;
}

static int
within (int x1, int y1, int x2, int y2, int r)
{
  return dist(x1, y1, x2, y2) <= r/2;
}

static int
arc_endpoint_is (ArcTypePtr a, int angle, int x, int y)
{
  int ax = a->X, ay = a->Y;

  if (angle % 90 == 0)
    {
      int ai = (int)(angle / 90) & 3;
      switch (ai)
	{
	case 0: ax -= a->Width; break;
	case 1: ay += a->Height; break;
	case 2: ax += a->Width; break;
	case 3: ay -= a->Height; break;
	}
    }
  else
    {
      double rad = angle * M_PI / 180;
      ax -= a->Width * cos (rad);
      ay += a->Width * sin (rad);
     }
#if 0
  printf(" - arc endpoint %d,%d\n", ax, ay);
#endif
  arc_dist = dist (ax, ay, x, y);
  if (arc_exact)
    return arc_dist < 2;
  return arc_dist < a->Thickness/2;
}

static int
line_callback (const BoxType * b, void *cl)
{
  /* LayerTypePtr layer = (LayerTypePtr)cl; */
  LineTypePtr l = (LineTypePtr)b;
  double d1, d2, t;
#if 0
  printf("line %d,%d .. %d,%d\n",
	 l->Point1.X, l->Point1.Y,
	 l->Point2.X, l->Point2.Y);
#endif
  d1 = dist (l->Point1.X, l->Point1.Y, x, y);
  d2 = dist (l->Point2.X, l->Point2.Y, x, y);
  if ((d1 < 2 || d2 < 2) && !line_exact)
    {
      line_exact = 1;
      the_line = 0;
    }
  t = line_exact ? 2 : l->Thickness/2;
  if (d1 < t || d2 < t)
    {
      if (the_line)
	multi = 1;
      the_line = l;
#if 0
      printf("picked, exact %d\n", line_exact);
#endif
    }
  return 1;
}

static int
arc_callback (const BoxType * b, void *cl)
{
  /* LayerTypePtr layer = (LayerTypePtr) cl; */
  ArcTypePtr a = (ArcTypePtr) b;

#if 0
  printf("arc a %d,%d r %d sa %d d %d\n", a->X, a->Y, a->Width, a->StartAngle, a->Delta);
#endif
  if (!arc_endpoint_is (a, a->StartAngle, x, y)
      && !arc_endpoint_is (a, a->StartAngle + a->Delta, x, y))
    return 1;
  if (arc_dist < 2)
    {
      if (! arc_exact)
	{
	  arc_exact = 1;
	  the_arc = 0;
	}
      if (the_arc)
	multi = 1;
      the_arc = a;
#if 0
      printf("picked, exact %d\n", arc_exact);
#endif
    }
  else if (!arc_exact)
    {
      if (the_arc)
	multi = 1;
      the_arc = a;
#if 0
      printf("picked, exact %d\n", arc_exact);
#endif
    }
  return 1;
}

static int
find_pair (int Px, int Py)
{
  BoxType spot;

#if 0
  printf("\nPuller find_pair at %d,%d\n", Crosshair.X, Crosshair.Y);
#endif

  x = Px;
  y = Py;
  multi = 0;
  line_exact = arc_exact = 0;
  the_line = 0;
  the_arc = 0;
  spot.X1 = x - 1;
  spot.Y1 = y - 1;
  spot.X2 = x + 1;
  spot.Y2 = y + 1;
  r_search (CURRENT->line_tree, &spot, NULL, line_callback, CURRENT);
  r_search (CURRENT->arc_tree, &spot, NULL, arc_callback, CURRENT);
  if (the_line && the_arc && !multi)
    return 1;
  x = Px;
  y = Py;
  return 0;
}

static int
Puller (int argc, char **argv, int Ux, int Uy)
{
  double arc_angle, line_angle, rel_angle, base_angle;
  double tangent;
  int new_delta_angle;

  if (!find_pair(Crosshair.X, Crosshair.Y))
    if (!find_pair(Ux, Uy))
      return 0;

  if (within (the_line->Point1.X, the_line->Point1.Y,
	      x, y, the_line->Thickness))
    {
      ex = the_line->Point2.X;
      ey = the_line->Point2.Y;
      the_line->Point2.X = the_line->Point1.X;
      the_line->Point2.Y = the_line->Point1.Y;
      the_line->Point1.X = ex;
      the_line->Point1.Y = ey;
    }
  else if (!within (the_line->Point2.X, the_line->Point2.Y,
		   x, y, the_line->Thickness))
    {
#if 0
      printf("Line endpoint not at cursor\n");
#endif
      return 1;
    }
  ex = the_line->Point1.X;
  ey = the_line->Point1.Y;

  cx = the_arc->X;
  cy = the_arc->Y;
  if (arc_endpoint_is (the_arc, the_arc->StartAngle, x, y))
    {
      ChangeArcAngles(CURRENT, the_arc, the_arc->StartAngle + the_arc->Delta,
		      -the_arc->Delta);
    }
  else if (! arc_endpoint_is (the_arc, the_arc->StartAngle + the_arc->Delta,
			      x, y))
    {
#if 0
      printf("arc not endpoints\n");
#endif
      return 1;
    }

  if (within (cx, cy, ex, ey, the_arc->Width*2))
    {
#if 0
      printf("line ends inside arc\n");
#endif
      return 1;
    }

  if (the_arc->Delta > 0)
    arc_angle = the_arc->StartAngle + the_arc->Delta + 90;
  else
    arc_angle = the_arc->StartAngle + the_arc->Delta - 90;
  line_angle = 180.0/M_PI * atan2(ey - y, x - ex);
  rel_angle = line_angle - arc_angle;
  base_angle = 180.0/M_PI * atan2(ey - cy, cx - ex);

  tangent = 180.0/M_PI * acos (the_arc->Width / dist(cx, cy, ex, ey));

#if 0
  printf("arc %g line %g rel %g base %g\n", arc_angle, line_angle, rel_angle, base_angle);
  printf("tangent %g\n", tangent);

  printf("arc was start %ld end %ld\n", the_arc->StartAngle, the_arc->StartAngle + the_arc->Delta);
#endif

  if (the_arc->Delta > 0)
    arc_angle = base_angle - tangent;
  else
    arc_angle = base_angle + tangent;
#if 0
  printf("new end angle %g\n", arc_angle);
#endif

  new_delta_angle = arc_angle - the_arc->StartAngle;
  if (new_delta_angle > 180)
    new_delta_angle -= 360;
  if (new_delta_angle < -180)
    new_delta_angle += 360;
  ChangeArcAngles(CURRENT, the_arc, the_arc->StartAngle, new_delta_angle);

#if 0
  printf("arc now start %ld end %ld\n", the_arc->StartAngle, the_arc->StartAngle + new_delta_angle);
#endif

  arc_angle = the_arc->StartAngle + the_arc->Delta;
  x = the_arc->X - the_arc->Width * cos(M_PI/180.0*arc_angle);
  y = the_arc->Y + the_arc->Height * sin(M_PI/180.0*arc_angle);

  MoveObject (LINEPOINT_TYPE, CURRENT, the_line, &(the_line->Point2),
	      x - the_line->Point2.X, y - the_line->Point2.Y);

  gui->invalidate_all();
  IncrementUndoSerialNumber();

  return 1;
}

HID_Action puller_action_list[] = {
  { "Puller", 1, "Click on a line-arc intersection or line segment", Puller }
};
REGISTER_ACTIONS(puller_action_list)
