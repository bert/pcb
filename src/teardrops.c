/*!
 * \file src/teardrops.c
 *
 * \brief Functions for handling the teardrop property of pins and vias.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2006 DJ Delorie <dj@delorie.com>
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
 */

#include <stdio.h>
#include <math.h>

#include "config.h"
#include "global.h"
#include "data.h"
#include "hid.h"
#include "misc.h"
#include "create.h"
#include "rtree.h"
#include "undo.h"

#define MIN_LINE_LENGTH 700
#define MAX_DISTANCE 700

static PinType * pin;
static int layer;
static int px, py;
static LayerType * silk;

static int new_arcs = 0;

int
distance_between_points(int x1,int y1, int x2, int y2)
{
  int a;
  int b;
  int distance;
  a = (x1-x2);
  b = (y1-y2);
  distance = hypot (a, b);
  return distance; 
}

static int
check_line_callback (const BoxType * box, void *cl)
{
  LayerType * lay = & PCB->Data->Layer[layer];
  LineType * l = (LineType *) box;
  int x1, x2, y1, y2;
  double a, b, c, x, r, t;
  double dx, dy, len;
  double ax, ay, lx, ly, theta;
  double ldist, adist, radius;
  double vx, vy, vr, vl;
  int delta, aoffset, count;
  ArcType * arc;

  /* if our line is to short ignore it */
  if (distance_between_points(l->Point1.X,l->Point1.Y,l->Point2.X,l->Point2.Y) < MIN_LINE_LENGTH )
    {
      return 1;
    }

  if (distance_between_points(l->Point1.X,l->Point1.Y,px,py) < MAX_DISTANCE) 
    {
      x1 = l->Point1.X;
      y1 = l->Point1.Y;
      x2 = l->Point2.X;
      y2 = l->Point2.Y;
    } 
  else if (distance_between_points(l->Point2.X,l->Point2.Y,px,py) < MAX_DISTANCE)
    {
      x1 = l->Point2.X;
      y1 = l->Point2.Y;
      x2 = l->Point1.X;
      y2 = l->Point1.Y;
    } 
  else
    return 1;

  r = pin->Thickness / 2.0;
  t = l->Thickness / 2.0;

  if (t > r)
    return 1;

  a = 1;
  b = 4 * t - 2 * r;
  c = 2 * t * t - r * r;

  x = (-b + sqrt (b * b - 4 * a * c)) / (2 * a);

  len = sqrt (((double)x2-x1)*(x2-x1) + ((double)y2-y1)*(y2-y1));

  if (len > (x+t))
    {
      adist = ldist = x + t;
      radius = x + t;
      delta = 45;

      if (radius < r || radius < t)
	return 1;
    }
  else if (len > r + t)
    {
      /* special "short teardrop" code */

      x = (len*len - r*r + t*t) / (2 * (r - t));
      ldist = len;
      adist = x + t;
      radius = x + t;
      delta = atan2 (len, x + t) * 180.0/M_PI;
    }
  else
    return 1;

  dx = ((double)x2 - x1) / len;
  dy = ((double)y2 - y1) / len;
  theta = atan2 (y2 - y1, x1 - x2) * 180.0/M_PI;

  lx = px + dx * ldist;
  ly = py + dy * ldist;

  /* We need one up front to determine how many segments it will take
     to fill.  */
  ax = lx - dy * adist;
  ay = ly + dx * adist;
  vl = sqrt (r*r - t*t);
  vx = px + dx * vl;
  vy = py + dy * vl;
  vx -= dy * t;
  vy += dx * t;
  vr = sqrt ((ax-vx) * (ax-vx) + (ay-vy) * (ay-vy));

  aoffset = 0;
  count = 0;
  do {
    if (++count > 5)
      {
	printf("a %d,%d v %d,%d adist %g radius %g vr %g\n",
	       (int)ax, (int)ay, (int)vx, (int)vy, adist, radius, vr);
	return 1;
      }

    ax = lx - dy * adist;
    ay = ly + dx * adist;

    arc = CreateNewArcOnLayer (lay, (int)ax, (int)ay, (int)radius, (int)radius,
			       (int)theta+90+aoffset, delta-aoffset,
			       l->Thickness, l->Clearance, l->Flags);
    if (arc)
      AddObjectToCreateUndoList (ARC_TYPE, lay, arc, arc);

    ax = lx + dy * (x+t);
    ay = ly - dx * (x+t);

    arc = CreateNewArcOnLayer (lay, (int)ax, (int)ay, (int)radius, (int)radius,
			       (int)theta-90-aoffset, -delta+aoffset,
			       l->Thickness, l->Clearance, l->Flags);
    if (arc)
      AddObjectToCreateUndoList (ARC_TYPE, lay, arc, arc);

    radius += t*1.9;
    aoffset = acos ((double)adist / radius) * 180.0 / M_PI;

    new_arcs ++;
  } while (vr > radius - t);

  return 1;
}

static void
check_pin (PinType * _pin)
{
  BoxType spot;

  pin = _pin;

  px = pin->X;
  py = pin->Y;

  spot.X1 = px - 10;
  spot.Y1 = py - 10;
  spot.X2 = px + 10;
  spot.Y2 = py + 10;

  for (layer = 0; layer < max_copper_layer; layer ++)
    {
      LayerType * l = &(PCB->Data->Layer[layer]);
      r_search (l->line_tree, &spot, NULL, check_line_callback, l);
    }
}

/* %start-doc actions Teardrops

The @code{Teardrops()} action adds teardrops to the intersections
between traces and pins/vias.

This is a simplistic test, so there are cases where you'd think it would
add them but doesn't.

If the trace doesn't end at @emph{exactly} the same point as the pin/via, it
will be skipped.

This often happens with metric parts on an Imperial grid or visa-versa.

If a trace passes through a pin/via but doesn't end there, there won't
be any teardrops.

Use @code{:djopt(none)} to split those lines into two segments, each of which
ends at the pin/via.

Usage:

This action takes no parameters.

To invoke it, use the command window, usually by typing ":".

Example:

@code{:Teardrops()}

@center @image{td_ex1,,,Example of how Teardrops works,png}

With the lesstif HID you can add this action to your menu or a hotkey by
editing $HOME/.pcb/pcb-menu.res (grab a copy from the pcb source if you
haven't one there yet). 

Known Bugs:

Square pins are teardropped too.

Refdes silk is no longer visible.

%end-doc */
static int
teardrops (int argc, char **argv, Coord x, Coord y)
{
  silk = & PCB->Data->SILKLAYER;

  new_arcs = 0;

  VIA_LOOP (PCB->Data);
  {
    check_pin (via);
  }
  END_LOOP;

  ALLPIN_LOOP (PCB->Data);
  {
    check_pin (pin);
  }
  ENDALL_LOOP;

  gui->invalidate_all ();

  if (new_arcs)
    IncrementUndoSerialNumber ();

  return 0;
}

static HID_Action teardrops_action_list[] = {
  {"Teardrops", NULL, teardrops,
   NULL, NULL}
};

REGISTER_ACTIONS (teardrops_action_list)

void
hid_teardrops_init()
{
  register_teardrops_action_list();
}
