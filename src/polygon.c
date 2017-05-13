/*!
 * \file src/polygon.c
 *
 * \brief Special polygon editing routines.
 *
 * Here's a brief tour of the data and life of a polygon, courtesy of
 * Ben Jackson:
 *
 * A PCB PolygonType contains an array of points outlining the polygon.
 * This is what is manipulated by the UI and stored in the saved PCB.
 *
 * A PolygonType also contains a POLYAREA called 'Clipped' which is
 * computed dynamically by InitClip every time a board is loaded.
 * The point array is coverted to a POLYAREA by original_poly and then
 * holes are cut in it by clearPoly.
 * After that it is maintained dynamically as parts are added, moved or
 * removed (this is why sometimes bugs can be fixed by just re-loading
 * the board).
 *
 * A POLYAREA consists of a linked list of PLINE structures.
 * The head of that list is POLYAREA.contours.
 * The first contour is an outline of a filled region.
 * All of the subsequent PLINEs are holes cut out of that first contour.
 * POLYAREAs are in a doubly-linked list and each member of the list is
 * an independent (non-overlapping) area with its own outline and holes.
 * The function biggest() finds the largest POLYAREA so that
 * PolygonType.Clipped points to that shape.
 * The rest of the polygon still exists, it's just ignored when turning
 * the polygon into copper.
 *
 * The first POLYAREA in PolygonType.Clipped is what is used for the
 * vast majority of Polygon related tests.
 * The basic logic for an intersection is "is the target shape inside
 * POLYAREA.contours and NOT fully enclosed in any of
 * POLYAREA.contours.next... (the holes)".
 *
 * The polygon dicer (NoHolesPolygonDicer and r_NoHolesPolygonDicer)
 * emits a series of "simple" PLINE shapes.
 * That is, the PLINE isn't linked to any other "holes" oulines.
 * That's the meaning of the first test in r_NoHolesPolygonDicer.
 * It is testing to see if the PLINE contour (the first, making it a
 * solid outline) has a valid next pointer (which would point to one or
 * more holes).
 * The dicer works by recursively chopping the polygon in half through
 * the first hole it sees (which is guaranteed to eliminate at least
 * that one hole).
 * The dicer output is used for HIDs which cannot render things with
 * holes (which would require erasure).
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996,2010 Thomas Nau
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
 *
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *
 * Thomas.Nau@rz.uni-ulm.de
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <math.h>
#include <memory.h>
#include <setjmp.h>

#include "global.h"
#include "box.h"
#include "create.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "find.h"
#include "misc.h"
#include "move.h"
#include "pcb-printf.h"
#include "polygon.h"
#include "remove.h"
#include "rtree.h"
#include "search.h"
#include "set.h"
#include "thermal.h"
#include "undo.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define ROUND(x) ((long)(((x) >= 0 ? (x) + 0.5  : (x) - 0.5)))

#define UNSUBTRACT_BLOAT 10
#define SUBTRACT_PIN_VIA_BATCH_SIZE 100
#define SUBTRACT_LINE_BATCH_SIZE 20

static double rotate_circle_seg[4];

void
polygon_init (void)
{
  double cos_ang = cos (2.0 * M_PI / POLY_CIRC_SEGS_F);
  double sin_ang = sin (2.0 * M_PI / POLY_CIRC_SEGS_F);

  rotate_circle_seg[0] = cos_ang;  rotate_circle_seg[1] = -sin_ang;
  rotate_circle_seg[2] = sin_ang;  rotate_circle_seg[3] =  cos_ang;
}

Cardinal
polygon_point_idx (PolygonType *polygon, PointType *point)
{
  assert (point >= polygon->Points);
  assert (point <= polygon->Points + polygon->PointN);
  return ((char *)point - (char *)polygon->Points) / sizeof (PointType);
}

/*!
 * \brief Find contour number.
 *
 * 0 for outer,
 *
 * 1 for first hole etc..
 */
Cardinal
polygon_point_contour (PolygonType *polygon, Cardinal point)
{
  Cardinal i;
  Cardinal contour = 0;

  for (i = 0; i < polygon->HoleIndexN; i++)
    if (point >= polygon->HoleIndex[i])
      contour = i + 1;
  return contour;
}

Cardinal
next_contour_point (PolygonType *polygon, Cardinal point)
{
  Cardinal contour;
  Cardinal this_contour_start;
  Cardinal next_contour_start;

  contour = polygon_point_contour (polygon, point);

  this_contour_start = (contour == 0) ? 0 :
                                        polygon->HoleIndex[contour - 1];
  next_contour_start =
    (contour == polygon->HoleIndexN) ? polygon->PointN :
                                       polygon->HoleIndex[contour];

  /* Wrap back to the start of the contour we're in if we pass the end */
  if (++point == next_contour_start)
    point = this_contour_start;

  return point;
}

Cardinal
prev_contour_point (PolygonType *polygon, Cardinal point)
{
  Cardinal contour;
  Cardinal prev_contour_end;
  Cardinal this_contour_end;

  contour = polygon_point_contour (polygon, point);

  prev_contour_end = (contour == 0) ? 0 :
                                      polygon->HoleIndex[contour - 1];
  this_contour_end =
    (contour == polygon->HoleIndexN) ? polygon->PointN - 1:
                                       polygon->HoleIndex[contour] - 1;

  /* Wrap back to the start of the contour we're in if we pass the end */
  if (point == prev_contour_end)
    point = this_contour_end;
  else
    point--;

  return point;
}

static void
add_noholes_polyarea (PLINE *pline, void *user_data)
{
  PolygonType *poly = (PolygonType *)user_data;

  /* Prepend the pline into the NoHoles linked list */
  pline->next = poly->NoHoles;
  poly->NoHoles = pline;
}

void
ComputeNoHoles (PolygonType *poly)
{
  poly_FreeContours (&poly->NoHoles);
  if (poly->Clipped)
    NoHolesPolygonDicer (poly, NULL, add_noholes_polyarea, poly);
  else
    printf ("Compute_noholes caught poly->Clipped = NULL\n");
  poly->NoHolesValid = 1;
}

static POLYAREA *
biggest (POLYAREA * p)
{
  POLYAREA *n, *top = NULL;
  PLINE *pl;
  rtree_t *tree;
  double big = -1;
  if (!p)
    return NULL;
  n = p;
  do
    {
#if 0
      if (n->contours->area < PCB->IsleArea)
        {
          n->b->f = n->f;
          n->f->b = n->b;
          poly_DelContour (&n->contours);
          if (n == p)
            p = n->f;
          n = n->f;
          if (!n->contours)
            {
              free (n);
              return NULL;
            }
        }
#endif
      if (n->contours->area > big)
        {
          top = n;
          big = n->contours->area;
        }
    }
  while ((n = n->f) != p);
  assert (top);
  if (top == p)
    return p;
  pl = top->contours;
  tree = top->contour_tree;
  top->contours = p->contours;
  top->contour_tree = p->contour_tree;
  p->contours = pl;
  p->contour_tree = tree;
  assert (pl);
  assert (p->f);
  assert (p->b);
  return p;
}

POLYAREA *
ContourToPoly (PLINE * contour)
{
  POLYAREA *p;
  poly_PreContour (contour, TRUE);
  assert (contour->Flags.orient == PLF_DIR);
  if (!(p = poly_Create ()))
    return NULL;
  poly_InclContour (p, contour);
  assert (poly_Valid (p));
  return p;
}

static POLYAREA *
original_poly (PolygonType * p)
{
  PLINE *contour = NULL;
  POLYAREA *np = NULL;
  Cardinal n;
  Vector v;
  int hole = 0;

  if ((np = poly_Create ()) == NULL)
    return NULL;

  /* first make initial polygon contour */
  for (n = 0; n < p->PointN; n++)
    {
      /* No current contour? Make a new one starting at point */
      /*   (or) Add point to existing contour */

      v[0] = p->Points[n].X;
      v[1] = p->Points[n].Y;
      if (contour == NULL)
        {
          if ((contour = poly_NewContour (v)) == NULL)
            return NULL;
        }
      else
        {
          poly_InclVertex (contour->head.prev, poly_CreateNode (v));
        }

      /* Is current point last in contour? If so process it. */
      if (n == p->PointN - 1 ||
          (hole < p->HoleIndexN && n == p->HoleIndex[hole] - 1))
        {
          poly_PreContour (contour, TRUE);

          /* make sure it is a positive contour (outer) or negative (hole) */
          if (contour->Flags.orient != (hole ? PLF_INV : PLF_DIR))
            poly_InvContour (contour);
          assert (contour->Flags.orient == (hole ? PLF_INV : PLF_DIR));

          poly_InclContour (np, contour);
          contour = NULL;
          assert (poly_Valid (np));

          hole++;
        }
  }
  return biggest (np);
}

POLYAREA *
PolygonToPoly (PolygonType *p)
{
  return original_poly (p);
}

POLYAREA *
RectPoly (Coord x1, Coord x2, Coord y1, Coord y2)
{
  PLINE *contour = NULL;
  Vector v;

  /* Return NULL for zero or negatively sized rectangles */
  if (x2 <= x1 || y2 <= y1)
    return NULL;

  v[0] = x1;
  v[1] = y1;
  if ((contour = poly_NewContour (v)) == NULL)
    return NULL;
  v[0] = x2;
  v[1] = y1;
  poly_InclVertex (contour->head.prev, poly_CreateNode (v));
  v[0] = x2;
  v[1] = y2;
  poly_InclVertex (contour->head.prev, poly_CreateNode (v));
  v[0] = x1;
  v[1] = y2;
  poly_InclVertex (contour->head.prev, poly_CreateNode (v));
  return ContourToPoly (contour);
}

POLYAREA *
OctagonPoly (Coord x, Coord y, Coord radius)
{
  PLINE *contour = NULL;
  Vector v;

  v[0] = x + ROUND (radius * 0.5);
  v[1] = y + ROUND (radius * TAN_22_5_DEGREE_2);
  if ((contour = poly_NewContour (v)) == NULL)
    return NULL;
  v[0] = x + ROUND (radius * TAN_22_5_DEGREE_2);
  v[1] = y + ROUND (radius * 0.5);
  poly_InclVertex (contour->head.prev, poly_CreateNode (v));
  v[0] = x - (v[0] - x);
  poly_InclVertex (contour->head.prev, poly_CreateNode (v));
  v[0] = x - ROUND (radius * 0.5);
  v[1] = y + ROUND (radius * TAN_22_5_DEGREE_2);
  poly_InclVertex (contour->head.prev, poly_CreateNode (v));
  v[1] = y - (v[1] - y);
  poly_InclVertex (contour->head.prev, poly_CreateNode (v));
  v[0] = x - ROUND (radius * TAN_22_5_DEGREE_2);
  v[1] = y - ROUND (radius * 0.5);
  poly_InclVertex (contour->head.prev, poly_CreateNode (v));
  v[0] = x - (v[0] - x);
  poly_InclVertex (contour->head.prev, poly_CreateNode (v));
  v[0] = x + ROUND (radius * 0.5);
  v[1] = y - ROUND (radius * TAN_22_5_DEGREE_2);
  poly_InclVertex (contour->head.prev, poly_CreateNode (v));
  return ContourToPoly (contour);
}

/*!
 * \brief Add vertices in a fractional-circle starting from v 
 * centered at X, Y and going counter-clockwise.
 *
 * Does not include the first point.
 * last argument is 1 for a full circle.
 * 2 for a half circle.
 * or 4 for a quarter circle.
 */
void
frac_circle (PLINE * c, Coord X, Coord Y, Vector v, int fraction)
{
  double e1, e2, t1;
  int i, range;

  poly_InclVertex (c->head.prev, poly_CreateNode (v));
  /* move vector to origin */
  e1 = (v[0] - X) * POLY_CIRC_RADIUS_ADJ;
  e2 = (v[1] - Y) * POLY_CIRC_RADIUS_ADJ;

  /* NB: the caller adds the last vertex, hence the -1 */
  range = POLY_CIRC_SEGS / fraction - 1;
  for (i = 0; i < range; i++)
    {
      /* rotate the vector */
      t1 = rotate_circle_seg[0] * e1 + rotate_circle_seg[1] * e2;
      e2 = rotate_circle_seg[2] * e1 + rotate_circle_seg[3] * e2;
      e1 = t1;
      v[0] = X + ROUND (e1);
      v[1] = Y + ROUND (e2);
      poly_InclVertex (c->head.prev, poly_CreateNode (v));
    }
}

/*!
 * \brief Create a circle approximation from lines.
 */
POLYAREA *
CirclePoly (Coord x, Coord y, Coord radius)
{
  PLINE *contour;
  Vector v;

  if (radius <= 0)
    return NULL;
  v[0] = x + radius;
  v[1] = y;
  if ((contour = poly_NewContour (v)) == NULL)
    return NULL;
  frac_circle (contour, x, y, v, 1);
  contour->is_round = TRUE;
  contour->cx = x;
  contour->cy = y;
  contour->radius = radius;
  return ContourToPoly (contour);
}

/*!
 * \brief Make a rounded-corner rectangle with radius t beyond
 * x1,x2,y1,y2 rectangle.
 */
POLYAREA *
RoundRect (Coord x1, Coord x2, Coord y1, Coord y2, Coord t)
{
  PLINE *contour = NULL;
  Vector v;

  assert (x2 > x1);
  assert (y2 > y1);
  v[0] = x1 - t;
  v[1] = y1;
  if ((contour = poly_NewContour (v)) == NULL)
    return NULL;
  frac_circle (contour, x1, y1, v, 4);
  v[0] = x2;
  v[1] = y1 - t;
  poly_InclVertex (contour->head.prev, poly_CreateNode (v));
  frac_circle (contour, x2, y1, v, 4);
  v[0] = x2 + t;
  v[1] = y2;
  poly_InclVertex (contour->head.prev, poly_CreateNode (v));
  frac_circle (contour, x2, y2, v, 4);
  v[0] = x1;
  v[1] = y2 + t;
  poly_InclVertex (contour->head.prev, poly_CreateNode (v));
  frac_circle (contour, x1, y2, v, 4);
  return ContourToPoly (contour);
}

#define ARC_ANGLE 5
static POLYAREA *
ArcPolyNoIntersect (ArcType * a, Coord thick)
{
  PLINE *contour = NULL;
  POLYAREA *np = NULL;
  Vector v;
  BoxType *ends;
  int i, segs;
  double ang, da, rx, ry;
  long half;
  double radius_adj;

  if (thick <= 0)
    return NULL;
  if (a->Delta < 0)
    {
      a->StartAngle += a->Delta;
      a->Delta = -a->Delta;
    }
  half = (thick + 1) / 2;
  ends = GetArcEnds (a);
  /* start with inner radius */
  rx = MAX (a->Width - half, 0);
  ry = MAX (a->Height - half, 0);
  segs = 1;
  if(thick > 0)
    segs = MAX (segs, a->Delta * M_PI / 360 *
                      sqrt (hypot (rx, ry) /
                            POLY_ARC_MAX_DEVIATION / 2 / thick));
  segs = MAX(segs, a->Delta / ARC_ANGLE);

  ang = a->StartAngle;
  da = (1.0 * a->Delta) / segs;
  radius_adj = (M_PI*da/360)*(M_PI*da/360)/2;
  v[0] = a->X - rx * cos (ang * M180);
  v[1] = a->Y + ry * sin (ang * M180);
  if ((contour = poly_NewContour (v)) == NULL)
    return 0;
  for (i = 0; i < segs - 1; i++)
    {
      ang += da;
      v[0] = a->X - rx * cos (ang * M180);
      v[1] = a->Y + ry * sin (ang * M180);
      poly_InclVertex (contour->head.prev, poly_CreateNode (v));
    }
  /* find last point */
  ang = a->StartAngle + a->Delta;
  v[0] = a->X - rx * cos (ang * M180) * (1 - radius_adj);
  v[1] = a->Y + ry * sin (ang * M180) * (1 - radius_adj);
  /* add the round cap at the end */
  frac_circle (contour, ends->X2, ends->Y2, v, 2);
  /* and now do the outer arc (going backwards) */
  rx = (a->Width + half) * (1+radius_adj);
  ry = (a->Width + half) * (1+radius_adj);
  da = -da;
  for (i = 0; i < segs; i++)
    {
      v[0] = a->X - rx * cos (ang * M180);
      v[1] = a->Y + ry * sin (ang * M180);
      poly_InclVertex (contour->head.prev, poly_CreateNode (v));
      ang += da;
    }
  /* now add other round cap */
  ang = a->StartAngle;
  v[0] = a->X - rx * cos (ang * M180) * (1 - radius_adj);
  v[1] = a->Y + ry * sin (ang * M180) * (1 - radius_adj);
  frac_circle (contour, ends->X1, ends->Y1, v, 2);
  /* now we have the whole contour */
  if (!(np = ContourToPoly (contour)))
    return NULL;
  return np;
}

#define MIN_CLEARANCE_BEFORE_BISECT 10.
POLYAREA *
ArcPoly (ArcType * a, Coord thick)
{
  double delta;
  ArcType seg1, seg2;
  POLYAREA *tmp1, *tmp2, *res;

  delta = (a->Delta < 0) ? -a->Delta : a->Delta;

  /* If the arc segment would self-intersect, we need to construct it as the union of
     two non-intersecting segments */

  if (2 * M_PI * a->Width * (1. - (double)delta / 360.) - thick < MIN_CLEARANCE_BEFORE_BISECT)
    {
      int half_delta = a->Delta / 2;

      seg1 = seg2 = *a;
      seg1.Delta = half_delta;
      seg2.Delta -= half_delta;
      seg2.StartAngle += half_delta;

      tmp1 = ArcPolyNoIntersect (&seg1, thick);
      tmp2 = ArcPolyNoIntersect (&seg2, thick);
      poly_Boolean_free (tmp1, tmp2, &res, PBO_UNITE);
      return res;
    }

  return ArcPolyNoIntersect (a, thick);
}

POLYAREA *
LinePoly (LineType * L, Coord thick)
{
  PLINE *contour = NULL;
  POLYAREA *np = NULL;
  Vector v;
  double d, dx, dy;
  long half;LineType _l=*L,*l=&_l;

  if (thick <= 0)
    return NULL;
  half = (thick + 1) / 2;
  d = hypot (l->Point1.X - l->Point2.X, l->Point1.Y - l->Point2.Y);
  if (!TEST_FLAG (SQUAREFLAG,l))
    if (d == 0)                   /* line is a point */
      return CirclePoly (l->Point1.X, l->Point1.Y, half);
  if (d != 0)
    {
      d = half / d;
      dx = (l->Point1.Y - l->Point2.Y) * d;
      dy = (l->Point2.X - l->Point1.X) * d;
    }
  else
    {
      dx = half;
      dy = 0;
    }
  if (TEST_FLAG (SQUAREFLAG,l))/* take into account the ends */
    {
      l->Point1.X -= dy;
      l->Point1.Y += dx;
      l->Point2.X += dy;
      l->Point2.Y -= dx;
    }
  v[0] = l->Point1.X - dx;
  v[1] = l->Point1.Y - dy;
  if ((contour = poly_NewContour (v)) == NULL)
    return 0;
  v[0] = l->Point2.X - dx;
  v[1] = l->Point2.Y - dy;
  if (TEST_FLAG (SQUAREFLAG,l))
    poly_InclVertex (contour->head.prev, poly_CreateNode (v));
  else
    frac_circle (contour, l->Point2.X, l->Point2.Y, v, 2);
  v[0] = l->Point2.X + dx;
  v[1] = l->Point2.Y + dy;
  poly_InclVertex (contour->head.prev, poly_CreateNode (v));
  v[0] = l->Point1.X + dx;
  v[1] = l->Point1.Y + dy;
  if (TEST_FLAG (SQUAREFLAG,l))
    poly_InclVertex (contour->head.prev, poly_CreateNode (v));
  else
    frac_circle (contour, l->Point1.X, l->Point1.Y, v, 2);
  /* now we have the line contour */
  if (!(np = ContourToPoly (contour)))
    return NULL;
  return np;
}

/*!
 * \brief Make a rounded-corner rectangle.
 */
POLYAREA *
SquarePadPoly (PadType * pad, Coord clear)
{
  PLINE *contour = NULL;
  POLYAREA *np = NULL;
  Vector v;
  double d;
  double tx, ty;
  double cx, cy;
  PadType _t=*pad,*t=&_t;
  PadType _c=*pad,*c=&_c;
  int halfthick = (pad->Thickness + 1) / 2;
  int halfclear = (clear + 1) / 2;

  d = hypot (pad->Point1.X - pad->Point2.X, pad->Point1.Y - pad->Point2.Y);
  if (d != 0)
    {
      double a = halfthick / d;
      tx = (t->Point1.Y - t->Point2.Y) * a;
      ty = (t->Point2.X - t->Point1.X) * a;
      a = halfclear / d;
      cx = (c->Point1.Y - c->Point2.Y) * a;
      cy = (c->Point2.X - c->Point1.X) * a;

      t->Point1.X -= ty;
      t->Point1.Y += tx;
      t->Point2.X += ty;
      t->Point2.Y -= tx;
      c->Point1.X -= cy;
      c->Point1.Y += cx;
      c->Point2.X += cy;
      c->Point2.Y -= cx;
    }
  else
    {
      tx = halfthick;
      ty = 0;
      cx = halfclear;
      cy = 0;

      t->Point1.Y += tx;
      t->Point2.Y -= tx;
      c->Point1.Y += cx;
      c->Point2.Y -= cx;
    }

  v[0] = c->Point1.X - tx;
  v[1] = c->Point1.Y - ty;
  if ((contour = poly_NewContour (v)) == NULL)
    return 0;
  frac_circle (contour, (t->Point1.X - tx), (t->Point1.Y - ty), v, 4);

  v[0] = t->Point2.X - cx;
  v[1] = t->Point2.Y - cy;
  poly_InclVertex (contour->head.prev, poly_CreateNode (v));
  frac_circle (contour, (t->Point2.X - tx), (t->Point2.Y - ty), v, 4);

  v[0] = c->Point2.X + tx;
  v[1] = c->Point2.Y + ty;
  poly_InclVertex (contour->head.prev, poly_CreateNode (v));
  frac_circle (contour, (t->Point2.X + tx), (t->Point2.Y + ty), v, 4);

  v[0] = t->Point1.X + cx;
  v[1] = t->Point1.Y + cy;
  poly_InclVertex (contour->head.prev, poly_CreateNode (v));
  frac_circle (contour, (t->Point1.X + tx), (t->Point1.Y + ty), v, 4);

  /* now we have the line contour */
  if (!(np = ContourToPoly (contour)))
    return NULL;
  return np;
}

/*!
 * \brief Clear np1 from the polygon.
 */
static int
Subtract (POLYAREA * np1, PolygonType * p, bool fnp)
{
  POLYAREA *merged = NULL, *np = np1;
  int x;
  assert (np);
  assert (p);
  if (!p->Clipped)
    {
      if (fnp)
        poly_Free (&np);
      return 1;
    }
  assert (poly_Valid (p->Clipped));
  assert (poly_Valid (np));
  if (fnp)
    x = poly_Boolean_free (p->Clipped, np, &merged, PBO_SUB);
  else
    {
      x = poly_Boolean (p->Clipped, np, &merged, PBO_SUB);
      poly_Free (&p->Clipped);
    }
  assert (!merged || poly_Valid (merged));
  if (x != err_ok)
    {
      fprintf (stderr, "Error while clipping PBO_SUB: %d\n", x);
      poly_Free (&merged);
      p->Clipped = NULL;
      if (p->NoHoles) printf ("Just leaked in Subtract\n");
      p->NoHoles = NULL;
      return -1;
    }
  p->Clipped = biggest (merged);
  assert (!p->Clipped || poly_Valid (p->Clipped));
  if (!p->Clipped)
    Message ("Polygon cleared out of existence near (%d, %d)\n",
             (p->BoundingBox.X1 + p->BoundingBox.X2) / 2,
             (p->BoundingBox.Y1 + p->BoundingBox.Y2) / 2);
  return 1;
}

/*!
 * \brief Create a polygon of the pin clearance.
 */
POLYAREA *
PinPoly (PinType * pin, Coord thick, Coord clear)
{
  int size;

  if (TEST_FLAG (SQUAREFLAG, pin))
    {
      size = (thick + 1) / 2;
      return RoundRect (pin->X - size, pin->X + size, pin->Y - size,
                        pin->Y + size, (clear + 1) / 2);
    }
  else
    {
      size = (thick + clear + 1) / 2;
      if (TEST_FLAG (OCTAGONFLAG, pin))
        {
          return OctagonPoly (pin->X, pin->Y, size + size);
        }
    }
  return CirclePoly (pin->X, pin->Y, size);
}

POLYAREA *
BoxPolyBloated (BoxType *box, Coord bloat)
{
  return RectPoly (box->X1 - bloat, box->X2 + bloat,
                   box->Y1 - bloat, box->Y2 + bloat);
}

/*!
 * \brief Remove the pin clearance from the polygon.
 */
static int
SubtractPin (DataType * d, PinType * pin, LayerType * l, PolygonType * p)
{
  POLYAREA *np;
  Cardinal i;

  if (pin->Clearance == 0)
    return 0;
  i = GetLayerNumber (d, l);
  if (TEST_THERM (i, pin))
    {
      np = ThermPoly ((PCBType *) (d->pcb), pin, i);
      if (!np)
        return 0;
    }
  else
    {
      np = PinPoly (pin, PIN_SIZE (pin), pin->Clearance);
      if (!np)
        return -1;
    }
  return Subtract (np, p, TRUE);
}

static int
SubtractLine (LineType * line, PolygonType * p)
{
  POLYAREA *np;

  if (!TEST_FLAG (CLEARLINEFLAG, line))
    return 0;
  if (!(np = LinePoly (line, line->Thickness + line->Clearance)))
    return -1;
  return Subtract (np, p, true);
}

static int
SubtractArc (ArcType * arc, PolygonType * p)
{
  POLYAREA *np;

  if (!TEST_FLAG (CLEARLINEFLAG, arc))
    return 0;
  if (!(np = ArcPoly (arc, arc->Thickness + arc->Clearance)))
    return -1;
  return Subtract (np, p, true);
}

static int
SubtractText (TextType * text, PolygonType * p)
{
  POLYAREA *np;
  const BoxType *b = &text->BoundingBox;

  if (!TEST_FLAG (CLEARLINEFLAG, text))
    return 0;
  if (!(np = RoundRect (b->X1 + PCB->Bloat, b->X2 - PCB->Bloat,
                        b->Y1 + PCB->Bloat, b->Y2 - PCB->Bloat, PCB->Bloat)))
    return -1;
  return Subtract (np, p, true);
}

static int
SubtractPad (PadType * pad, PolygonType * p)
{
  POLYAREA *np = NULL;

  if (pad->Clearance == 0)
    return 0;
  if (TEST_FLAG (SQUAREFLAG, pad))
    {
      if (!
          (np = SquarePadPoly (pad, pad->Thickness + pad->Clearance)))
        return -1;
    }
  else
    {
      if (!
          (np = LinePoly ((LineType *) pad, pad->Thickness + pad->Clearance)))
        return -1;
    }
  return Subtract (np, p, true);
}

struct cpInfo
{
  const BoxType *other;
  DataType *data;
  LayerType *layer;
  PolygonType *polygon;
  bool bottom;
  POLYAREA *accumulate;
  int batch_size;
  jmp_buf env;
};

static void
subtract_accumulated (struct cpInfo *info, PolygonType *polygon)
{
  if (info->accumulate == NULL)
    return;
  Subtract (info->accumulate, polygon, true);
  info->accumulate = NULL;
  info->batch_size = 0;
}

static int
pin_sub_callback (const BoxType * b, void *cl)
{
  PinType *pin = (PinType *) b;
  struct cpInfo *info = (struct cpInfo *) cl;
  PolygonType *polygon;
  POLYAREA *np;
  POLYAREA *merged;
  Cardinal i;

  /* don't subtract the object that was put back! */
  if (b == info->other)
    return 0;
  polygon = info->polygon;

  i = GetLayerNumber (info->data, info->layer);

  if (VIA_IS_BURIED (pin) && (!VIA_ON_LAYER (pin, i)))
    return 0;

  if (pin->Clearance == 0)
    return 0;
  if (TEST_THERM (i, pin))
    {
      np = ThermPoly ((PCBType *) (info->data->pcb), pin, i);
      if (!np)
        return 1;
    }
  else
    {
      np = PinPoly (pin, PIN_SIZE (pin), pin->Clearance);
      if (!np)
        longjmp (info->env, 1);
    }

  poly_Boolean_free (info->accumulate, np, &merged, PBO_UNITE);
  info->accumulate = merged;

  info->batch_size ++;

  if (info->batch_size == SUBTRACT_PIN_VIA_BATCH_SIZE)
    subtract_accumulated (info, polygon);

  return 1;
}

static int
arc_sub_callback (const BoxType * b, void *cl)
{
  ArcType *arc = (ArcType *) b;
  struct cpInfo *info = (struct cpInfo *) cl;
  PolygonType *polygon;

  /* don't subtract the object that was put back! */
  if (b == info->other)
    return 0;
  if (!TEST_FLAG (CLEARLINEFLAG, arc))
    return 0;
  polygon = info->polygon;
  if (SubtractArc (arc, polygon) < 0)
    longjmp (info->env, 1);
  return 1;
}

static int
pad_sub_callback (const BoxType * b, void *cl)
{
  PadType *pad = (PadType *) b;
  struct cpInfo *info = (struct cpInfo *) cl;
  PolygonType *polygon;

  /* don't subtract the object that was put back! */
  if (b == info->other)
    return 0;
  if (pad->Clearance == 0)
    return 0;
  polygon = info->polygon;
  if (XOR (TEST_FLAG (ONSOLDERFLAG, pad), !info->bottom))
    {
      if (SubtractPad (pad, polygon) < 0)
        longjmp (info->env, 1);
      return 1;
    }
  return 0;
}

static int
line_sub_callback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  struct cpInfo *info = (struct cpInfo *) cl;
  PolygonType *polygon;
  POLYAREA *np;
  POLYAREA *merged;

  /* don't subtract the object that was put back! */
  if (b == info->other)
    return 0;
  if (!TEST_FLAG (CLEARLINEFLAG, line))
    return 0;
  polygon = info->polygon;

  if (!(np = LinePoly (line, line->Thickness + line->Clearance)))
    longjmp (info->env, 1);

  poly_Boolean_free (info->accumulate, np, &merged, PBO_UNITE);
  info->accumulate = merged;
  info->batch_size ++;

  if (info->batch_size == SUBTRACT_LINE_BATCH_SIZE)
    subtract_accumulated (info, polygon);

  return 1;
}

static int
text_sub_callback (const BoxType * b, void *cl)
{
  TextType *text = (TextType *) b;
  struct cpInfo *info = (struct cpInfo *) cl;
  PolygonType *polygon;

  /* don't subtract the object that was put back! */
  if (b == info->other)
    return 0;
  if (!TEST_FLAG (CLEARLINEFLAG, text))
    return 0;
  polygon = info->polygon;
  if (SubtractText (text, polygon) < 0)
    longjmp (info->env, 1);
  return 1;
}

static int
Group (DataType *Data, Cardinal layer)
{
  Cardinal i, j;
  for (i = 0; i < max_group; i++)
    for (j = 0; j < ((PCBType *) (Data->pcb))->LayerGroups.Number[i]; j++)
      if (layer == ((PCBType *) (Data->pcb))->LayerGroups.Entries[i][j])
        return i;
  return i;
}

static int
clearPoly (DataType *Data, LayerType *Layer, PolygonType * polygon,
           const BoxType * here, Coord expand)
{
  int r = 0;
  BoxType region;
  struct cpInfo info;
  Cardinal group;

  if (!TEST_FLAG (CLEARPOLYFLAG, polygon)
      || GetLayerNumber (Data, Layer) >= max_copper_layer)
    return 0;
  group = Group (Data, GetLayerNumber (Data, Layer));
  info.bottom = (group == Group (Data, bottom_silk_layer));
  info.data = Data;
  info.other = here;
  info.layer = Layer;
  info.polygon = polygon;
  if (here)
    region = clip_box (here, &polygon->BoundingBox);
  else
    region = polygon->BoundingBox;
  region = bloat_box (&region, expand);

  if (setjmp (info.env) == 0)
    {
      r = 0;
      info.accumulate = NULL;
      info.batch_size = 0;
      if (info.bottom || group == Group (Data, top_silk_layer))
	r += r_search (Data->pad_tree, &region, NULL, pad_sub_callback, &info);
      GROUP_LOOP (Data, group);
      {
        r +=
          r_search (layer->line_tree, &region, NULL, line_sub_callback,
                    &info);
        subtract_accumulated (&info, polygon);
        r +=
          r_search (layer->arc_tree, &region, NULL, arc_sub_callback, &info);
	r +=
          r_search (layer->text_tree, &region, NULL, text_sub_callback, &info);
      }
      END_LOOP;
      r += r_search (Data->via_tree, &region, NULL, pin_sub_callback, &info);
      r += r_search (Data->pin_tree, &region, NULL, pin_sub_callback, &info);
      subtract_accumulated (&info, polygon);
    }
  polygon->NoHolesValid = 0;
  return r;
}

static int
Unsubtract (POLYAREA * np1, PolygonType * p)
{
  POLYAREA *merged = NULL, *np = np1;
  POLYAREA *orig_poly, *clipped_np;
  int x;
  assert (np);
  assert (p && p->Clipped);

  orig_poly = original_poly (p);

  x = poly_Boolean_free (np, orig_poly, &clipped_np, PBO_ISECT);
  if (x != err_ok)
    {
      fprintf (stderr, "Error while clipping PBO_ISECT: %d\n", x);
      poly_Free (&clipped_np);
      goto fail;
    }

  x = poly_Boolean_free (p->Clipped, clipped_np, &merged, PBO_UNITE);
  if (x != err_ok)
    {
      fprintf (stderr, "Error while clipping PBO_UNITE: %d\n", x);
      poly_Free (&merged);
      goto fail;
    }
  p->Clipped = biggest (merged);
  assert (!p->Clipped || poly_Valid (p->Clipped));
  return 1;

fail:
  p->Clipped = NULL;
  if (p->NoHoles) printf ("Just leaked in Unsubtract\n");
  p->NoHoles = NULL;
  return 0;
}

static int
UnsubtractPin (PinType * pin, LayerType * l, PolygonType * p)
{
  POLYAREA *np;

  /* overlap a bit to prevent gaps from rounding errors */
  np = BoxPolyBloated (&pin->BoundingBox, UNSUBTRACT_BLOAT);

  if (!np)
    return 0;
  if (!Unsubtract (np, p))
    return 0;
  clearPoly (PCB->Data, l, p, (const BoxType *) pin, 2 * UNSUBTRACT_BLOAT);
  return 1;
}

static int
UnsubtractArc (ArcType * arc, LayerType * l, PolygonType * p)
{
  POLYAREA *np;

  if (!TEST_FLAG (CLEARLINEFLAG, arc))
    return 0;

  /* overlap a bit to prevent gaps from rounding errors */
  np = BoxPolyBloated (&arc->BoundingBox, UNSUBTRACT_BLOAT);

  if (!np)
    return 0;
  if (!Unsubtract (np, p))
    return 0;
  clearPoly (PCB->Data, l, p, (const BoxType *) arc, 2 * UNSUBTRACT_BLOAT);
  return 1;
}

static int
UnsubtractLine (LineType * line, LayerType * l, PolygonType * p)
{
  POLYAREA *np;

  if (!TEST_FLAG (CLEARLINEFLAG, line))
    return 0;

  /* overlap a bit to prevent notches from rounding errors */
  np = BoxPolyBloated (&line->BoundingBox, UNSUBTRACT_BLOAT);

  if (!np)
    return 0;
  if (!Unsubtract (np, p))
    return 0;
  clearPoly (PCB->Data, l, p, (const BoxType *) line, 2 * UNSUBTRACT_BLOAT);
  return 1;
}

static int
UnsubtractText (TextType * text, LayerType * l, PolygonType * p)
{
  POLYAREA *np;

  if (!TEST_FLAG (CLEARLINEFLAG, text))
    return 0;

  /* overlap a bit to prevent notches from rounding errors */
  np = BoxPolyBloated (&text->BoundingBox, UNSUBTRACT_BLOAT);

  if (!np)
    return -1;
  if (!Unsubtract (np, p))
    return 0;
  clearPoly (PCB->Data, l, p, (const BoxType *) text, 2 * UNSUBTRACT_BLOAT);
  return 1;
}

static int
UnsubtractPad (PadType * pad, LayerType * l, PolygonType * p)
{
  POLYAREA *np;

  /* overlap a bit to prevent notches from rounding errors */
  np = BoxPolyBloated (&pad->BoundingBox, UNSUBTRACT_BLOAT);

  if (!np)
    return 0;
  if (!Unsubtract (np, p))
    return 0;
  clearPoly (PCB->Data, l, p, (const BoxType *) pad, 2 * UNSUBTRACT_BLOAT);
  return 1;
}

static bool inhibit = false;

int
InitClip (DataType *Data, LayerType *layer, PolygonType * p)
{
  if (inhibit)
    return 0;
  if (p->Clipped)
    poly_Free (&p->Clipped);
  p->Clipped = original_poly (p);
  poly_FreeContours (&p->NoHoles);
  if (!p->Clipped)
    return 0;
  assert (poly_Valid (p->Clipped));
  if (TEST_FLAG (CLEARPOLYFLAG, p))
    clearPoly (Data, layer, p, NULL, 0);
  else
    p->NoHolesValid = 0;
  return 1;
}

/*!
 * \brief Remove redundant polygon points.
 *
 * Any point that lies on the straight line between the points on either
 * side of it is redundant.
 *
 * \return True if any points are removed.
 */
bool
RemoveExcessPolygonPoints (LayerType *Layer, PolygonType *Polygon)
{
  PointType *p;
  Cardinal n, prev, next;
  LineType line;
  bool changed = false;

  if (Undoing ())
    return (false);

  for (n = 0; n < Polygon->PointN; n++)
    {
      prev = prev_contour_point (Polygon, n);
      next = next_contour_point (Polygon, n);
      p = &Polygon->Points[n];

      line.Point1 = Polygon->Points[prev];
      line.Point2 = Polygon->Points[next];
      line.Thickness = 0;
      if (IsPointOnLine (p->X, p->Y, 0.0, &line))
        {
          RemoveObject (POLYGONPOINT_TYPE, Layer, Polygon, p);
          changed = true;
        }
    }
  return (changed);
}

/*!
 * \brief .
 *
 * \return The index of the polygon point which is the end point of the
 * segment with the lowest distance to the passed coordinates.
 */
Cardinal
GetLowestDistancePolygonPoint (PolygonType *Polygon, Coord X, Coord Y)
{
  double mindistance = (double) MAX_COORD * MAX_COORD;
  PointType *ptr1, *ptr2;
  Cardinal n, result = 0;

  /* we calculate the distance to each segment and choose the
   * shortest distance. If the closest approach between the
   * given point and the projected line (i.e. the segment extended)
   * is not on the segment, then the distance is the distance
   * to the segment end point.
   */

  for (n = 0; n < Polygon->PointN; n++)
    {
      double u, dx, dy;
      ptr1 = &Polygon->Points[prev_contour_point (Polygon, n)];
      ptr2 = &Polygon->Points[n];

      dx = ptr2->X - ptr1->X;
      dy = ptr2->Y - ptr1->Y;
      if (dx != 0.0 || dy != 0.0)
        {
          /* projected intersection is at P1 + u(P2 - P1) */
          u = ((X - ptr1->X) * dx + (Y - ptr1->Y) * dy) / (dx * dx + dy * dy);

          if (u < 0.0)
            {                   /* ptr1 is closest point */
              u = SQUARE (X - ptr1->X) + SQUARE (Y - ptr1->Y);
            }
          else if (u > 1.0)
            {                   /* ptr2 is closest point */
              u = SQUARE (X - ptr2->X) + SQUARE (Y - ptr2->Y);
            }
          else
            {                   /* projected intersection is closest point */
              u = SQUARE (X - ptr1->X * (1.0 - u) - u * ptr2->X) +
                SQUARE (Y - ptr1->Y * (1.0 - u) - u * ptr2->Y);
            }
          if (u < mindistance)
            {
              mindistance = u;
              result = n;
            }
        }
    }
  return (result);
}

/*!
 * \brief Go back to the  previous point of the polygon.
 */
void
GoToPreviousPoint (void)
{
  switch (Crosshair.AttachedPolygon.PointN)
    {
      /* do nothing if mode has just been entered */
    case 0:
      break;

      /* reset number of points and 'LINE_MODE' state */
    case 1:
      Crosshair.AttachedPolygon.PointN = 0;
      Crosshair.AttachedLine.State = STATE_FIRST;
      addedLines = 0;
      break;

      /* back-up one point */
    default:
      {
        PointType *points = Crosshair.AttachedPolygon.Points;
        Cardinal n = Crosshair.AttachedPolygon.PointN - 2;

        Crosshair.AttachedPolygon.PointN--;
        Crosshair.AttachedLine.Point1.X = points[n].X;
        Crosshair.AttachedLine.Point1.Y = points[n].Y;
        break;
      }
    }
}

/*!
 * \brief Close polygon if possible.
 */
void
ClosePolygon (void)
{
  Cardinal n = Crosshair.AttachedPolygon.PointN;

  /* check number of points */
  if (n >= 3)
    {
      /* if 45 degree lines are what we want do a quick check
       * if closing the polygon makes sense
       */
      if (!TEST_FLAG (ALLDIRECTIONFLAG, PCB))
        {
          Coord dx, dy;

          dx = abs (Crosshair.AttachedPolygon.Points[n - 1].X -
                    Crosshair.AttachedPolygon.Points[0].X);
          dy = abs (Crosshair.AttachedPolygon.Points[n - 1].Y -
                    Crosshair.AttachedPolygon.Points[0].Y);
          if (!(dx == 0 || dy == 0 || dx == dy))
            {
              Message
                (_
                 ("Cannot close polygon because 45 degree lines are requested.\n"));
              return;
            }
        }
      CopyAttachedPolygonToLayer ();
      Draw ();
    }
  else
    Message (_("A polygon has to have at least 3 points\n"));
}

/*!
 * \brief Moves the data of the attached (new) polygon to the current
 * layer.
 */
void
CopyAttachedPolygonToLayer (void)
{
  PolygonType *polygon;
  int saveID;

  /* move data to layer and clear attached struct */
  polygon = CreateNewPolygon (CURRENT, NoFlags ());
  saveID = polygon->ID;
  *polygon = Crosshair.AttachedPolygon;
  polygon->ID = saveID;
  SET_FLAG (CLEARPOLYFLAG, polygon);
  if (TEST_FLAG (NEWFULLPOLYFLAG, PCB))
    SET_FLAG (FULLPOLYFLAG, polygon);
  memset (&Crosshair.AttachedPolygon, 0, sizeof (PolygonType));
  SetPolygonBoundingBox (polygon);
  if (!CURRENT->polygon_tree)
    CURRENT->polygon_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (CURRENT->polygon_tree, (BoxType *) polygon, 0);
  InitClip (PCB->Data, CURRENT, polygon);
  DrawPolygon (CURRENT, polygon);
  SetChangedFlag (true);

  /* reset state of attached line */
  Crosshair.AttachedLine.State = STATE_FIRST;
  addedLines = 0;

  /* add to undo list */
  AddObjectToCreateUndoList (POLYGON_TYPE, CURRENT, polygon, polygon);
  IncrementUndoSerialNumber ();
}

/*!
 * \brief Find polygon holes in range, then call the callback function
 * for each hole.
 *
 * If the callback returns non-zero, stop the search.
 */
int
PolygonHoles (PolygonType *polygon, const BoxType *range,
              int (*callback) (PLINE *contour, void *user_data),
              void *user_data)
{
  POLYAREA *pa = polygon->Clipped;
  PLINE *pl;
  /* If this hole is so big the polygon doesn't exist, then it's not
   * really a hole.
   */
  if (!pa)
    return 0;
  for (pl = pa->contours->next; pl; pl = pl->next)
    {
      if (range != NULL &&
          (pl->xmin > range->X2 || pl->xmax < range->X1 ||
           pl->ymin > range->Y2 || pl->ymax < range->Y1))
        continue;
      if (callback (pl, user_data))
        {
          return 1;
        }
    }
  return 0;
}

struct plow_info
{
  int type;
  void *ptr1, *ptr2;
  LayerType *layer;
  DataType *data;
  int (*callback) (DataType *, LayerType *, PolygonType *, int, void *,
                   void *, void *);
  void *userdata;
};

static int
subtract_plow (DataType *Data, LayerType *Layer, PolygonType *Polygon,
               int type, void *ptr1, void *ptr2, void *userdata)
{
  PinType *via;
  int layer_n = GetLayerNumber (Data, Layer);

  if (!Polygon->Clipped)
    return 0;
  switch (type)
    {
    case PIN_TYPE:
      SubtractPin (Data, (PinType *) ptr2, Layer, Polygon);
      Polygon->NoHolesValid = 0;
      return 1;
    case VIA_TYPE:
      via = (PinType *) ptr2;
      if (!VIA_IS_BURIED (via) || VIA_ON_LAYER (via, layer_n))
        {
          SubtractPin (Data, via, Layer, Polygon);
          Polygon->NoHolesValid = 0;
          return 1;
	}
      break;
    case LINE_TYPE:
      SubtractLine ((LineType *) ptr2, Polygon);
      Polygon->NoHolesValid = 0;
      return 1;
    case ARC_TYPE:
      SubtractArc ((ArcType *) ptr2, Polygon);
      Polygon->NoHolesValid = 0;
      return 1;
    case PAD_TYPE:
      SubtractPad ((PadType *) ptr2, Polygon);
      Polygon->NoHolesValid = 0;
      return 1;
    case TEXT_TYPE:
      SubtractText ((TextType *) ptr2, Polygon);
      Polygon->NoHolesValid = 0;
      return 1;
    }
  return 0;
}

static int
add_plow (DataType *Data, LayerType *Layer, PolygonType *Polygon,
          int type, void *ptr1, void *ptr2, void *userdata)
{
  PinType *via;
  int layer_n = GetLayerNumber (Data, Layer);

  switch (type)
    {
    case PIN_TYPE:
      UnsubtractPin ((PinType *) ptr2, Layer, Polygon);
      return 1;
    case VIA_TYPE:
      via = (PinType *) ptr2;
      if (!VIA_IS_BURIED (via) || VIA_ON_LAYER (via, layer_n))
        {
          UnsubtractPin ((PinType *) ptr2, Layer, Polygon);
          return 1;
	}
	break;
    case LINE_TYPE:
      UnsubtractLine ((LineType *) ptr2, Layer, Polygon);
      return 1;
    case ARC_TYPE:
      UnsubtractArc ((ArcType *) ptr2, Layer, Polygon);
      return 1;
    case PAD_TYPE:
      UnsubtractPad ((PadType *) ptr2, Layer, Polygon);
      return 1;
    case TEXT_TYPE:
      UnsubtractText ((TextType *) ptr2, Layer, Polygon);
      return 1;
    }
  return 0;
}

static int
plow_callback (const BoxType * b, void *cl)
{
  struct plow_info *plow = (struct plow_info *) cl;
  PolygonType *polygon = (PolygonType *) b;

  if (TEST_FLAG (CLEARPOLYFLAG, polygon))
    return plow->callback (plow->data, plow->layer, polygon, plow->type,
                           plow->ptr1, plow->ptr2, plow->userdata);
  return 0;
}

int
PlowsPolygon (DataType * Data, int type, void *ptr1, void *ptr2,
              int (*call_back) (DataType *data, LayerType *lay,
                                PolygonType *poly, int type, void *ptr1,
                                void *ptr2, void *userdata),
              void *userdata)
{
  BoxType sb = ((PinType *) ptr2)->BoundingBox;
  int r = 0;
  struct plow_info info;

  info.type = type;
  info.ptr1 = ptr1;
  info.ptr2 = ptr2;
  info.data = Data;
  info.callback = call_back;
  info.userdata = userdata;
  switch (type)
    {
    case PIN_TYPE:
    case VIA_TYPE:
      if (type == PIN_TYPE || ptr1 == ptr2 || ptr1 == NULL)
        {
          LAYER_LOOP (Data, max_copper_layer);
          {
            info.layer = layer;
            r +=
              r_search (layer->polygon_tree, &sb, NULL, plow_callback, &info);
          }
          END_LOOP;
        }
      else
        {
          GROUP_LOOP (Data, GetLayerGroupNumberByNumber (GetLayerNumber (Data,
                                                                         ((LayerType *) ptr1))));
          {
            info.layer = layer;
            r +=
              r_search (layer->polygon_tree, &sb, NULL, plow_callback, &info);
          }
          END_LOOP;
        }
      break;
    case LINE_TYPE:
    case ARC_TYPE:
    case TEXT_TYPE:
      /* the cast works equally well for lines and arcs */
      if (!TEST_FLAG (CLEARLINEFLAG, (LineType *) ptr2))
        return 0;
      /* silk doesn't plow */
      if (GetLayerNumber (Data, (LayerType *)ptr1) >= max_copper_layer)
        return 0;
      GROUP_LOOP (Data, GetLayerGroupNumberByNumber (GetLayerNumber (Data,
                                                                     ((LayerType *) ptr1))));
      {
        info.layer = layer;
        r += r_search (layer->polygon_tree, &sb, NULL, plow_callback, &info);
      }
      END_LOOP;
      break;
    case PAD_TYPE:
      {
        Cardinal group = GetLayerGroupNumberByNumber (
                            TEST_FLAG (ONSOLDERFLAG, (PadType *) ptr2) ?
                              bottom_silk_layer : top_silk_layer);
        GROUP_LOOP (Data, group);
        {
          info.layer = layer;
          r +=
            r_search (layer->polygon_tree, &sb, NULL, plow_callback, &info);
        }
        END_LOOP;
      }
      break;

    case ELEMENT_TYPE:
      {
        PIN_LOOP ((ElementType *) ptr1);
        {
          PlowsPolygon (Data, PIN_TYPE, ptr1, pin, call_back, userdata);
        }
        END_LOOP;
        PAD_LOOP ((ElementType *) ptr1);
        {
          PlowsPolygon (Data, PAD_TYPE, ptr1, pad, call_back, userdata);
        }
        END_LOOP;
      }
      break;
    }
  return r;
}

void
RestoreToPolygon (DataType * Data, int type, void *ptr1, void *ptr2)
{
  if (!Data->polyClip)
    return;

  if (type == POLYGON_TYPE)
    InitClip (PCB->Data, (LayerType *) ptr1, (PolygonType *) ptr2);
  else
    PlowsPolygon (Data, type, ptr1, ptr2, add_plow, NULL);
}

void
ClearFromPolygon (DataType * Data, int type, void *ptr1, void *ptr2)
{
  if (!Data->polyClip)
    return;

  if (type == POLYGON_TYPE)
    InitClip (PCB->Data, (LayerType *) ptr1, (PolygonType *) ptr2);
  else
    PlowsPolygon (Data, type, ptr1, ptr2, subtract_plow, NULL);
}

bool
isects (POLYAREA * a, PolygonType *p, bool fr)
{
  POLYAREA *x;
  bool ans;
  ans = Touching (a, p->Clipped);
  /* argument may be register, so we must copy it */
  x = a;
  if (fr)
    poly_Free (&x);
  return ans;
}


bool
IsPointInPolygon (Coord X, Coord Y, Coord r, PolygonType *p)
{
  POLYAREA *c;
  Vector v;
  v[0] = X;
  v[1] = Y;
  if (poly_CheckInside (p->Clipped, v))
    return true;
  if (r < 1)
    return false;
  if (!(c = CirclePoly (X, Y, r)))
    return false;
  return isects (c, p, true);
}


bool
IsPointInPolygonIgnoreHoles (Coord X, Coord Y, PolygonType *p)
{
  Vector v;
  v[0] = X;
  v[1] = Y;
  return poly_InsideContour (p->Clipped->contours, v);
}

bool
IsRectangleInPolygon (Coord X1, Coord Y1, Coord X2, Coord Y2, PolygonType *p)
{
  POLYAREA *s;
  if (!
      (s = RectPoly (min (X1, X2), max (X1, X2), min (Y1, Y2), max (Y1, Y2))))
    return false;
  return isects (s, p, true);
}

/*!
 * \brief .
 *
 * \note This function will free the passed POLYAREA.
 * It must only be passed a single POLYAREA (pa->f == pa->b == pa).
 */
static void
r_NoHolesPolygonDicer (POLYAREA * pa,
                       void (*emit) (PLINE *, void *), void *user_data)
{
  PLINE *p = pa->contours;

  if (!pa->contours->next)                 /* no holes */
    {
      pa->contours = NULL; /* The callback now owns the contour */
      /* Don't bother removing it from the POLYAREA's rtree
         since we're going to free the POLYAREA below anyway */
      emit (p, user_data);
      poly_Free (&pa);
      return;
    }
  else
    {
      POLYAREA *poly2, *left, *right;

      /* make a rectangle of the left region slicing through the middle of the first hole */
      poly2 = RectPoly (p->xmin, (p->next->xmin + p->next->xmax) / 2,
                        p->ymin, p->ymax);
      poly_AndSubtract_free (pa, poly2, &left, &right);
      if (left)
        {
          POLYAREA *cur, *next;
          cur = left;
          do
            {
              next = cur->f;
              cur->f = cur->b = cur; /* Detach this polygon piece */
              r_NoHolesPolygonDicer (cur, emit, user_data);
              /* NB: The POLYAREA was freed by its use in the recursive dicer */
            }
          while ((cur = next) != left);
        }
      if (right)
        {
          POLYAREA *cur, *next;
          cur = right;
          do
            {
              next = cur->f;
              cur->f = cur->b = cur; /* Detach this polygon piece */
              r_NoHolesPolygonDicer (cur, emit, user_data);
              /* NB: The POLYAREA was freed by its use in the recursive dicer */
            }
          while ((cur = next) != right);
        }
    }
}

void
NoHolesPolygonDicer (PolygonType *p, const BoxType * clip,
                     void (*emit) (PLINE *, void *), void *user_data)
{
  POLYAREA *main_contour, *cur, *next;

  main_contour = poly_Create ();
  /* copy the main poly only */
  poly_Copy1 (main_contour, p->Clipped);
  /* clip to the bounding box */
  if (clip)
    {
      POLYAREA *cbox = RectPoly (clip->X1, clip->X2, clip->Y1, clip->Y2);
      poly_Boolean_free (main_contour, cbox, &main_contour, PBO_ISECT);
    }
  if (main_contour == NULL)
    return;
  /* Now dice it up.
   * NB: Could be more than one piece (because of the clip above)
   */
  cur = main_contour;
  do
    {
      next = cur->f;
      cur->f = cur->b = cur; /* Detach this polygon piece */
      r_NoHolesPolygonDicer (cur, emit, user_data);
      /* NB: The POLYAREA was freed by its use in the recursive dicer */
    }
  while ((cur = next) != main_contour);
}

/*!
 * \brief Make a polygon split into multiple parts into multiple
 * polygons.
 */
bool
MorphPolygon (LayerType *layer, PolygonType *poly)
{
  POLYAREA *p, *start;
  bool many = false;
  FlagType flags;

  if (!poly->Clipped || TEST_FLAG (LOCKFLAG, poly))
    return false;
  if (poly->Clipped->f == poly->Clipped)
    return false;
  ErasePolygon (poly);
  start = p = poly->Clipped;
  /* This is ugly. The creation of the new polygons can cause
   * all of the polygon pointers (including the one we're called
   * with to change if there is a realloc in GetPolygonMemory().
   * That will invalidate our original "poly" argument, potentially
   * inside the loop. We need to steal the Clipped pointer and
   * hide it from the Remove call so that it still exists while
   * we do this dirty work.
   */
  poly->Clipped = NULL;
  if (poly->NoHoles) printf ("Just leaked in MorpyPolygon\n");
  poly->NoHoles = NULL;
  flags = poly->Flags;
  RemovePolygon (layer, poly);
  inhibit = true;
  do
    {
      VNODE *v;
      PolygonType *newone;

      if (p->contours->area > PCB->IsleArea)
        {
          newone = CreateNewPolygon (layer, flags);
          if (!newone)
            return false;
          many = true;
          v = &p->contours->head;
          CreateNewPointInPolygon (newone, v->point[0], v->point[1]);
          for (v = v->next; v != &p->contours->head; v = v->next)
            CreateNewPointInPolygon (newone, v->point[0], v->point[1]);
          newone->BoundingBox.X1 = p->contours->xmin;
          newone->BoundingBox.X2 = p->contours->xmax + 1;
          newone->BoundingBox.Y1 = p->contours->ymin;
          newone->BoundingBox.Y2 = p->contours->ymax + 1;
          AddObjectToCreateUndoList (POLYGON_TYPE, layer, newone, newone);
          newone->Clipped = p;
          p = p->f;             /* go to next pline */
          newone->Clipped->b = newone->Clipped->f = newone->Clipped;     /* unlink from others */
          r_insert_entry (layer->polygon_tree, (BoxType *) newone, 0);
          DrawPolygon (layer, newone);
        }
      else
        {
          POLYAREA *t = p;

          p = p->f;
          poly_DelContour (&t->contours);
          free (t);
        }
    }
  while (p != start);
  inhibit = false;
  IncrementUndoSerialNumber ();
  return many;
}

void debug_pline (PLINE *pl)
{
  VNODE *v;
  pcb_fprintf (stderr, "\txmin %#mS xmax %#mS ymin %#mS ymax %#mS\n",
	   pl->xmin, pl->xmax, pl->ymin, pl->ymax);
  v = &pl->head;
  while (v)
    {
      pcb_fprintf(stderr, "\t\tvnode: %#mD\n", v->point[0], v->point[1]);
      v = v->next;
      if (v == &pl->head)
	break;
    }
}

void
debug_polyarea (POLYAREA *p)
{
  PLINE *pl;

  fprintf (stderr, "POLYAREA %p\n", p);
  for (pl=p->contours; pl; pl=pl->next)
    debug_pline (pl);
}

void
debug_polygon (PolygonType *p)
{
  Cardinal i;
  POLYAREA *pa;
  fprintf (stderr, "POLYGON %p  %d pts\n", p, p->PointN);
  for (i=0; i<p->PointN; i++)
    pcb_fprintf(stderr, "\t%d: %#mD\n", i, p->Points[i].X, p->Points[i].Y);
  if (p->HoleIndexN)
    {
      fprintf (stderr, "%d holes, starting at indices\n", p->HoleIndexN);
      for (i=0; i<p->HoleIndexN; i++)
        fprintf(stderr, "\t%d: %d\n", i, p->HoleIndex[i]);
    }
  else
    fprintf (stderr, "it has no holes\n");
  pa = p->Clipped;
  while (pa)
    {
      debug_polyarea (pa);
      pa = pa->f;
      if (pa == p->Clipped)
	break;
    }
}

/*!
 * \brief Convert a POLYAREA (and all linked POLYAREA) to raw PCB
 * polygons on the given layer.
 */
void
PolyToPolygonsOnLayer (DataType *Destination, LayerType *Layer,
                       POLYAREA *Input, FlagType Flags)
{
  PolygonType *Polygon;
  POLYAREA *pa;
  PLINE *pline;
  VNODE *node;
  bool outer;

  if (Input == NULL)
    return;

  pa = Input;
  do
    {
      Polygon = CreateNewPolygon (Layer, Flags);

      pline = pa->contours;
      outer = true;

      do
        {
          if (!outer)
            CreateNewHoleInPolygon (Polygon);
          outer = false;

          node = &pline->head;
          do
            {
              CreateNewPointInPolygon (Polygon, node->point[0],
                                                node->point[1]);
            }
          while ((node = node->next) != &pline->head);

        }
      while ((pline = pline->next) != NULL);

      InitClip (Destination, Layer, Polygon);
      SetPolygonBoundingBox (Polygon);
      if (!Layer->polygon_tree)
        Layer->polygon_tree = r_create_tree (NULL, 0, 0);
      r_insert_entry (Layer->polygon_tree, (BoxType *) Polygon, 0);

      DrawPolygon (Layer, Polygon);
      /* add to undo list */
      AddObjectToCreateUndoList (POLYGON_TYPE, Layer, Polygon, Polygon);
    }
  while ((pa = pa->f) != Input);

  SetChangedFlag (true);
}
