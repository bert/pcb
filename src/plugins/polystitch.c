/*!
 * \file polystitch.c
 *
 * \brief PolyStitch plug-in for PCB.
 *
 * \author Copyright (C) 2010 DJ Delorie <dj@delorie.com>
 *
 * \copyright Licensed under the terms of the GNU General Public 
 * License, version 2 or later.
 *
 * http://www.delorie.com/pcb/polystitch.c
 *
 * Compile like this:
 *
 * gcc -I$HOME/geda/pcb-cvs/src -I$HOME/geda/pcb-cvs -O2 -shared polystitch.c -o polystitch.so
 *
 * The resulting polystitch.so goes in $HOME/.pcb/plugins/polystitch.so.
 *
 * Usage: PolyStitch()
 *
 * The polygon under the cursor (based on closest-corner) is stitched
 * together with the polygon surrounding it on the same layer.
 * Use with pstoedit conversions where there's a "hole" in the shape -
 * select the hole.
 */

#include <stdio.h>
#include <math.h>

#include "config.h"
#include "global.h"
#include "data.h"
#include "macro.h"
#include "create.h"
#include "remove.h"
#include "hid.h"
#include "error.h"
#include "rtree.h"
#include "draw.h"
#include "set.h"
#include "polygon.h"
#include "misc.h"

static PolygonType *inner_poly, *outer_poly;
static LayerType *poly_layer;

static double
ATAN2 (PointType a, PointType b)
{
  if (a.X == b.X && a.Y == b.Y)
    return 0;
  return atan2 ((double)b.Y - a.Y, (double)b.X - a.X);
}

static double
poly_winding (PolygonType *poly)
{
  double winding, turn;
  double prev_angle, this_angle;
  int i, n;

  winding = 0;

  prev_angle = ATAN2(poly->Points[0], poly->Points[1]);
  n = poly->PointN;
  for (i=1; i<=n; i++)
    {
      this_angle = ATAN2 (poly->Points[i%n], poly->Points[(i+1)%n]);
      turn = this_angle - prev_angle;
      if (turn < -M_PI) turn += 2*M_PI;
      if (turn >  M_PI) turn -= 2*M_PI;
      winding += turn;
      prev_angle = this_angle;
    }
  return winding;
}

/*!
 * \brief Given the X,Y, find the polygon and set inner_poly and
 * poly_layer.
 */
static void
find_crosshair_poly (int x, int y)
{
  double best = 0, dist;

  inner_poly = NULL;
  poly_layer = NULL;
  
  VISIBLEPOLYGON_LOOP (PCB->Data);
  {
    /* layer, polygon */
    POLYGONPOINT_LOOP (polygon);
    {
      /* point */
      int dx = x - point->X;
      int dy = y - point->Y;
      dist = (double)dx*dx + (double)dy*dy;
      if (dist < best || inner_poly == NULL)
        {
          inner_poly = polygon;
          poly_layer = layer;
          best = dist;
        }
    }
    END_LOOP;
  }
  ENDALL_LOOP;
  if (!inner_poly)
    {
      Message("Cannot find any polygons");
      return;
    }
}

/*!
 * \brief Set outer_poly to the enclosing poly. We assume there's only
 * one.
 */
static void
find_enclosing_poly ()
{
  outer_poly = NULL;

  POLYGON_LOOP (poly_layer);
  {
    if (polygon == inner_poly)
      continue;
    if (polygon->BoundingBox.X1 <= inner_poly->BoundingBox.X1
        && polygon->BoundingBox.X2 >= inner_poly->BoundingBox.X2
        && polygon->BoundingBox.Y1 <= inner_poly->BoundingBox.Y1
        && polygon->BoundingBox.Y2 >= inner_poly->BoundingBox.Y2)
      {
        outer_poly = polygon;
        return;
      }
  }
  END_LOOP;
  Message("Cannot find a polygon enclosing the one you selected");
}

static void
check_windings ()
{
  double iw, ow;
  int i, j;

  iw = poly_winding (inner_poly);
  ow = poly_winding (outer_poly);
  if (iw * ow > 0)
    {
      /* Wound in same direction, must reverse one.  */
      for (i=0, j=inner_poly->PointN-1;
           i<j;
           i++, j--)
        {
          PointType x = inner_poly->Points[i];
          inner_poly->Points[i] = inner_poly->Points[j];
          inner_poly->Points[j] = x;
        }
    }
}

/*!
 * \brief Rotate the polygon point list around so that point N is the
 * first one in the list.
 */
static void
rotate_points (PolygonType *poly, int n)
{
  PointType *np;
  int n2 = poly->PointN - n;

  np = (PointType *) malloc (poly->PointN * sizeof(PointType));
  memcpy (np, poly->Points + n, n2 * sizeof (PointType));
  memcpy (np+n2, poly->Points, n * sizeof (PointType));
  memcpy (poly->Points, np, poly->PointN * sizeof(PointType));
  free (np);
}

/*!
 * \brief Make sure the first and last point of the polygon are the same
 * point, so we can stitch them properly.
 */
static void
dup_endpoints (PolygonType *poly)
{
  int n = poly->PointN;
  if (poly->Points[0].X == poly->Points[n-1].X
      && poly->Points[0].Y == poly->Points[n-1].Y)
    return;
  CreateNewPointInPolygon (poly, poly->Points[0].X, poly->Points[0].Y);
}

/*!
 * \brief Find the two closest points between those polygons, and
 * connect them. We assume pstoedit winds the two polygons in opposite
 * directions.
 */
static void
stitch_them ()
{
  int i, o;
  int ii, oo;
  double best = -1, dist;

  ErasePolygon (inner_poly);
  ErasePolygon (outer_poly);

  /* This is O(n^2) but there's not a lot we can do about that.  */
  for (i=0; i<inner_poly->PointN; i++)
    for (o=0; o<outer_poly->PointN; o++)
      {
        int dx = inner_poly->Points[i].X - outer_poly->Points[o].X;
        int dy = inner_poly->Points[i].Y - outer_poly->Points[o].Y;
        dist = (double)dx*dx + (double)dy*dy;
        if (dist < best || best < 0)
          {
            ii = i;
            oo = o;
            best = dist;
          }
      }
  if (ii != 0)
    rotate_points (inner_poly, ii);
  if (oo != 0)
    rotate_points (outer_poly, oo);
  dup_endpoints (inner_poly);
  dup_endpoints (outer_poly);

  r_delete_entry (poly_layer->polygon_tree, (BoxType *)inner_poly);
  r_delete_entry (poly_layer->polygon_tree, (BoxType *)outer_poly);

  for (i=0; i<inner_poly->PointN; i++)
    CreateNewPointInPolygon (outer_poly, inner_poly->Points[i].X, inner_poly->Points[i].Y);

  SetChangedFlag (true);

  outer_poly->NoHolesValid = 0;
  SetPolygonBoundingBox (outer_poly);
  r_insert_entry (poly_layer->polygon_tree, (BoxType *)outer_poly, 0);
  RemoveExcessPolygonPoints (poly_layer, outer_poly);
  InitClip (PCB->Data, poly_layer, outer_poly);
  DrawPolygon (poly_layer, outer_poly);
  Draw ();

  RemovePolygon (poly_layer, inner_poly);
}

static int
polystitch (int argc, char **argv, Coord x, Coord y)
{
  find_crosshair_poly (x, y);
  if (inner_poly)
    {
      find_enclosing_poly ();
      if (outer_poly)
        {
          check_windings ();
          stitch_them ();
        }
    }
  return 0;
}

static HID_Action polystitch_action_list[] = {
  {"PolyStitch", "Select a corner on the inner polygon", polystitch,
   NULL, NULL}
};

REGISTER_ACTIONS (polystitch_action_list)

void
pcb_plugin_init()
{
  register_polystitch_action_list();
}
