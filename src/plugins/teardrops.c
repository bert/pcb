/*!
 * \file teardrops.c
 *
 * \brief Teardrops plug-in for PCB.
 *
 * \author Copyright (C) 2006 DJ Delorie <dj@delorie.com>
 *
 * \copyright Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * http://www.delorie.com/pcb/teardrops/
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
/* 20120220 joe changed MAX_DISTANCE to 0.5mm below */
 /* 0.5mm */
/* #define MAX_DISTANCE 500000 */
/* #define MAX_DISTANCE 2000000 */
 /* 1 mm */
/* #define MAX_DISTANCE 1000000 */

static PinType * pin;
static PadType * pad;
static int layer;
static int px, py;
static LayerType * silk;
/* 20120220 joe added the below line */
static Coord thickness;
static ElementType *element;

static int new_arcs = 0;

int
distance_between_points (int x1, int y1, int x2, int y2)
{
  /* int a; */
  /* int b; */
  int distance;
  /* a = (x1-x2); */
  /* b = (y1-y2); */
  distance = sqrt ((pow ( x1 - x2, 2 )) + (pow ( y1 - y2 , 2)));
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

  fprintf (stderr, "...Line ((%.6f, %.6f), (%.6f, %.6f)): ",
    COORD_TO_MM(l->Point1.X),
    COORD_TO_MM(l->Point1.Y),
    COORD_TO_MM(l->Point2.X),
    COORD_TO_MM(l->Point2.Y));

  /* if our line is to short ignore it */
  if (distance_between_points (l->Point1.X, l->Point1.Y, l->Point2.X, l->Point2.Y) < MIN_LINE_LENGTH )
  {
    fprintf (stderr, "not within max line length\n");
    return 1;
  }

  fprintf (stderr, "......Point (%.6f, %.6f): ",
    COORD_TO_MM(px),
    COORD_TO_MM(py));

  if (distance_between_points (l->Point1.X, l->Point1.Y, px, py) < MAX_DISTANCE) 
  {
    x1 = l->Point1.X;
    y1 = l->Point1.Y;
    x2 = l->Point2.X;
    y2 = l->Point2.Y;
  } 
  else if (distance_between_points (l->Point2.X, l->Point2.Y, px, py) < MAX_DISTANCE)
  {
    x1 = l->Point2.X;
    y1 = l->Point2.Y;
    x2 = l->Point1.X;
    y2 = l->Point1.Y;
  } 
  else 
  {
    fprintf (stderr, "not within max distance\n");
    return 1;
  }

  /* r = pin->Thickness / 2.0; */
  r = thickness / 2.0;
  t = l->Thickness / 2.0;

  if (t > r)
  {
    fprintf (stderr, "t > r: t = %3.6f, r = %3.6f\n",
      COORD_TO_MM(t),
      COORD_TO_MM(r));
    return 1;
  }

  a = 1;
  b = 4 * t - 2 * r;
  c = 2 * t * t - r * r;

  x = (-b + sqrt (b * b - 4 * a * c)) / (2 * a);

  len = sqrt (((double) x2-x1)*(x2-x1) + ((double) y2-y1)*(y2-y1));

  if (len > (x+t))
  {
    adist = ldist = x + t;
    radius = x + t;
    delta = 45;

    if (radius < r || radius < t)
    {
      fprintf (stderr,
        "(radius < r || radius < t): radius = %3.6f, r = %3.6f, t = %3.6f\n",
        COORD_TO_MM(radius),
        COORD_TO_MM(r),
        COORD_TO_MM(t));
      return 1;
    }
  }
  else if (len > r + t)
  {
    /* special "short teardrop" code */

    x = (len * len - r * r + t * t) / (2 * (r - t));
    ldist = len;
    adist = x + t;
    radius = x + t;
    delta = atan2 (len, x + t) * 180.0 / M_PI;
  }
  else
    return 1;

  dx = ((double) x2 - x1) / len;
  dy = ((double) y2 - y1) / len;
  theta = atan2 (y2 - y1, x1 - x2) * 180.0 / M_PI;

  lx = px + dx * ldist;
  ly = py + dy * ldist;

  /* We need one up front to determine how many segments it will take
     to fill.  */
  ax = lx - dy * adist;
  ay = ly + dx * adist;
  vl = sqrt (r * r - t * t);
  vx = px + dx * vl;
  vy = py + dy * vl;
  vx -= dy * t;
  vy += dx * t;
  vr = sqrt ((ax-vx) * (ax-vx) + (ay-vy) * (ay-vy));

  aoffset = 0;
  count = 0;
  do
  {
    if (++count > 5)
    {
      fprintf(stderr,"......a %d,%d v %d,%d adist %g radius %g vr %g\n",
        (int) ax,
        (int) ay,
        (int) vx,
        (int) vy,
        adist,
        radius,
        vr);
      printf("a %d,%d v %d,%d adist %g radius %g vr %g\n",
        (int) ax,
        (int) ay,
        (int) vx,
        (int) vy,
        adist,
        radius,
        vr);
      return 1;
    }

    ax = lx - dy * adist;
    ay = ly + dx * adist;

    arc = CreateNewArcOnLayer (lay, (int) ax, (int) ay, (int) radius,
      (int) radius, (int) theta + 90 + aoffset, delta - aoffset,
      l->Thickness, l->Clearance, l->Flags);
    if (arc)
    AddObjectToCreateUndoList (ARC_TYPE, lay, arc, arc);

    ax = lx + dy * (x+t);
    ay = ly - dx * (x+t);

    arc = CreateNewArcOnLayer (lay, (int) ax, (int) ay, (int) radius,
      (int) radius, (int) theta - 90 - aoffset, - delta + aoffset,
      l->Thickness, l->Clearance, l->Flags);
    if (arc)
      AddObjectToCreateUndoList (ARC_TYPE, lay, arc, arc);

    radius += t * 1.9;
    aoffset = acos ((double) adist / radius) * 180.0 / M_PI;

    new_arcs ++;
  } while (vr > radius - t);

  fprintf (stderr,"done arc'ing\n");
  return 1;
}

static void
check_pin (PinType * _pin)
{
  BoxType spot;

  pin = _pin;

  px = pin->X;
  py = pin->Y;
   /* 20120220 joe added the below line */
  thickness = pin->Thickness;

  spot.X1 = px - 10;
  spot.Y1 = py - 10;
  spot.X2 = px + 10;
  spot.Y2 = py + 10;

  element = (ElementType *) pin->Element;

  fprintf (stderr,
    "Pin %s (%s) at %.6f, %.6f (element %s, %s, %s)\n",
    EMPTY (pin->Number), EMPTY (pin->Name),
    /* 0.01 * pin->X, 0.01 * pin->Y, */
    COORD_TO_MM(pin->X), COORD_TO_MM(pin->Y),
    EMPTY (NAMEONPCB_NAME (element)),
    EMPTY (VALUE_NAME (element)),
    EMPTY (DESCRIPTION_NAME (element)));

  for (layer = 0; layer < max_copper_layer; layer ++)
  {
    LayerType * l = &(PCB->Data->Layer[layer]);
    r_search (l->line_tree, &spot, NULL, check_line_callback, l);
  }
}

static void
check_via (PinType * _pin)
{
  BoxType spot;

  pin = _pin;

  px = pin->X;
  py = pin->Y;

  spot.X1 = px - 10;
  spot.Y1 = py - 10;
  spot.X2 = px + 10;
  spot.Y2 = py + 10;

  fprintf (stderr,
    "Via at %.6f, %.6f\n",
    COORD_TO_MM(pin->X), COORD_TO_MM(pin->Y));

  for (layer = 0; layer < max_copper_layer; layer ++)
  {
    LayerType * l = &(PCB->Data->Layer[layer]);
    r_search (l->line_tree, &spot, NULL, check_line_callback, l);
  }
}

/* 20120220 joe added this function to draw teardrops for pads */
static void
check_pad (PadType * _pad)
{
  pad = _pad;

  px = (pad->BoundingBox.X1 + pad->BoundingBox.X2)/2;
  py = (pad->BoundingBox.Y1 + pad->BoundingBox.Y2)/2;
  thickness = pad->Thickness;
  element = (ElementType *)pad->Element;

  fprintf(stderr,
    "Pad %s (%s) at %.6f, %.6f (element %s, %s, %s) \n",
    EMPTY (pad->Number), EMPTY (pad->Name),
    COORD_TO_MM((pad->BoundingBox.X1 + pad->BoundingBox.X2)/2),
    COORD_TO_MM((pad->BoundingBox.Y1 + pad->BoundingBox.Y2)/2),
    EMPTY (NAMEONPCB_NAME (element)),
    EMPTY (VALUE_NAME (element)),
    EMPTY (DESCRIPTION_NAME (element)));

  /* fprintf(stderr, */
  /*   "Pad %s (%s) at ((%.6f, %.6f), (%.6f, %.6f)) (element %s, %s, %s) \n", */
  /*           EMPTY (pad->Number), EMPTY (pad->Name), */
  /*           COORD_TO_MM(pad->BoundingBox.X1), */
  /*           COORD_TO_MM(pad->BoundingBox.Y1), */
  /*           COORD_TO_MM(pad->BoundingBox.X2), */
  /*           COORD_TO_MM(pad->BoundingBox.Y2), */
  /*           EMPTY (NAMEONPCB_NAME (element)), */
  /*           EMPTY (VALUE_NAME (element)), */
  /*           EMPTY (DESCRIPTION_NAME (element))); */

  for (layer = 0; layer < max_copper_layer; layer ++)
  {
    LayerType * l = &(PCB->Data->Layer[layer]);
    r_search (l->line_tree, &(pad->BoundingBox), NULL, check_line_callback, l);
  }
}

static int
teardrops (int argc, char **argv, Coord x, Coord y)
{
  silk = & PCB->Data->SILKLAYER;

  new_arcs = 0;

  VIA_LOOP (PCB->Data);
  {
    check_via (via);
  }
  END_LOOP;

  ALLPIN_LOOP (PCB->Data);
  {
    check_pin (pin);
  }
  ENDALL_LOOP;

  ALLPAD_LOOP (PCB->Data);
  {
    check_pad (pad);
  }
  ENDALL_LOOP;

  gui->invalidate_all ();

  if (new_arcs)
    IncrementUndoSerialNumber ();

  return 0;
}

static HID_Action teardrops_action_list[] =
{
  {"Teardrops", NULL, teardrops, NULL, NULL}
};

REGISTER_ACTIONS (teardrops_action_list)

void
hid_teardrops_init ()
{
  register_teardrops_action_list ();
}


