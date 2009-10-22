/* $Id$ */

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


/*

Here's a brief tour of the data and life of a polygon, courtesy of Ben
Jackson:

A PCB PolygonType contains an array of points outlining the polygon.
This is what is manipulated by the UI and stored in the saved PCB.

A PolygonType also contains a POLYAREA called 'Clipped' which is
computed dynamically by InitClip every time a board is loaded.  The
point array is coverted to a POLYAREA by original_poly and then holes
are cut in it by clearPoly.  After that it is maintained dynamically
as parts are added, moved or removed (this is why sometimes bugs can
be fixed by just re-loading the board).

A POLYAREA consists of a linked list of PLINE structures.  The head of
that list is POLYAREA.contours.  The first contour is an outline of a
filled region.  All of the subsequent PLINEs are holes cut out of that
first contour.  POLYAREAs are in a doubly-linked list and each member
of the list is an independent (non-overlapping) area with its own
outline and holes.  The function biggest() finds the largest POLYAREA
so that PolygonType.Clipped points to that shape.  The rest of the
polygon still exists, it's just ignored when turning the polygon into
copper.

The first POLYAREA in PolygonType.Clipped is what is used for the vast
majority of Polygon related tests.  The basic logic for an
intersection is "is the target shape inside POLYAREA.contours and NOT
fully enclosed in any of POLYAREA.contours.next... (the holes)".

The polygon dicer (NoHolesPolygonDicer and r_NoHolesPolygonDicer)
emits a series of "simple" PLINE shapes.  That is, the PLINE isn't
linked to any other "holes" oulines).  That's the meaning of the first
test in r_NoHolesPolygonDicer.  It is testing to see if the PLINE
contour (the first, making it a solid outline) has a valid next
pointer (which would point to one or more holes).  The dicer works by
recursively chopping the polygon in half through the first hole it
sees (which is guaranteed to eliminate at least that one hole).  The
dicer output is used for HIDs which cannot render things with holes
(which would require erasure).

*/

/* special polygon editing routines
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

RCSID ("$Id$");

#define ROUND(x) ((long)(((x) >= 0 ? (x) + 0.5  : (x) - 0.5)))

#define UNSUBTRACT_BLOAT 10

/* ---------------------------------------------------------------------------
 * local prototypes
 */

#define CIRC_SEGS 36
static double circleVerticies[] = {
  1.0, 0.0,
  0.98480775301221, 0.17364817766693,
};

static void
add_noholes_polyarea (PLINE *pline, void *user_data)
{
  PolygonType *poly = user_data;

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
  top->contours = p->contours;
  p->contours = pl;
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
  Vector v;

  /* first make initial polygon contour */
  POLYGONPOINT_LOOP (p);
  {
    v[0] = point->X;
    v[1] = point->Y;
    if (contour == NULL)
      {
        if ((contour = poly_NewContour (v)) == NULL)
          return NULL;
      }
    else
      {
        poly_InclVertex (contour->head.prev, poly_CreateNode (v));
      }
  }
  END_LOOP;
  poly_PreContour (contour, TRUE);
  /* make sure it is a positive contour */
  if ((contour->Flags.orient) != PLF_DIR)
    poly_InvContour (contour);
  assert ((contour->Flags.orient) == PLF_DIR);
  if ((np = poly_Create ()) == NULL)
    return NULL;
  poly_InclContour (np, contour);
  assert (poly_Valid (np));
  return biggest (np);
}

static int
ClipOriginal (PolygonType * poly)
{
  POLYAREA *p, *result;
  int r;

  p = original_poly (poly);
  r = poly_Boolean_free (poly->Clipped, p, &result, PBO_ISECT);
  if (r != err_ok)
    {
      fprintf (stderr, "Error while clipping PBO_ISECT: %d\n", r);
      poly_Free (&result);
      poly->Clipped = NULL;
      if (poly->NoHoles) printf ("Just leaked in ClipOriginal\n");
      poly->NoHoles = NULL;
      return 0;
    }
  poly->Clipped = biggest (result);
  assert (!poly->Clipped || poly_Valid (poly->Clipped));
  return 1;
}

POLYAREA *
RectPoly (LocationType x1, LocationType x2, LocationType y1, LocationType y2)
{
  PLINE *contour = NULL;
  Vector v;

  assert (x2 > x1);
  assert (y2 > y1);
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
OctagonPoly (LocationType x, LocationType y, BDimension radius)
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

/* add verticies in a fractional-circle starting from v 
 * centered at X, Y and going counter-clockwise
 * does not include the first point
 * last argument is 1 for a full circle
 * 2 for a half circle
 * or 4 for a quarter circle
 */
void
frac_circle (PLINE * c, LocationType X, LocationType Y, Vector v, int range)
{
  double e1, e2, t1;
  int i;

  poly_InclVertex (c->head.prev, poly_CreateNode (v));
  /* move vector to origin */
  e1 = v[0] - X;
  e2 = v[1] - Y;

  range = range == 1 ? CIRC_SEGS-1 : (CIRC_SEGS / range);
  for (i = 0; i < range; i++)
    {
      /* rotate the vector */
      t1 = e1 * circleVerticies[2] - e2 * circleVerticies[3];
      e2 = e1 * circleVerticies[3] + e2 * circleVerticies[2];
      e1 = t1;
      v[0] = X + ROUND (e1);
      v[1] = Y + ROUND (e2);
      poly_InclVertex (c->head.prev, poly_CreateNode (v));
    }
}

#define COARSE_CIRCLE 0
/* create a 35-vertex circle approximation */
POLYAREA *
CirclePoly (LocationType x, LocationType y, BDimension radius)
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
  return ContourToPoly (contour);
}

/* make a rounded-corner rectangle with radius t beyond x1,x2,y1,y2 rectangle */
POLYAREA *
RoundRect (LocationType x1, LocationType x2, LocationType y1, LocationType y2,
           BDimension t)
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
ArcPolyNoIntersect (ArcType * a, BDimension thick)
{
  PLINE *contour = NULL;
  POLYAREA *np = NULL;
  Vector v;
  BoxType *ends;
  int i, segs;
  double ang, da, rx, ry;
  long half;

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
  segs = a->Delta / ARC_ANGLE;
  ang = a->StartAngle;
  da = (1.0 * a->Delta) / segs;
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
  v[0] = a->X - rx * cos (ang * M180);
  v[1] = a->Y + ry * sin (ang * M180);
  /* add the round cap at the end */
  frac_circle (contour, ends->X2, ends->Y2, v, 2);
  /* and now do the outer arc (going backwards) */
  rx = a->Width + half;
  ry = a->Width + half;
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
  v[0] = a->X - rx * cos (ang * M180);
  v[1] = a->Y + ry * sin (ang * M180);
  frac_circle (contour, ends->X1, ends->Y1, v, 2);
  /* now we have the whole contour */
  if (!(np = ContourToPoly (contour)))
    return NULL;
  return np;
}

#define MIN_CLEARANCE_BEFORE_BISECT 10.
POLYAREA *
ArcPoly (ArcType * a, BDimension thick)
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
LinePoly (LineType * L, BDimension thick)
{
  PLINE *contour = NULL;
  POLYAREA *np = NULL;
  Vector v;
  double d, dx, dy;
  long half;LineType _l=*L,*l=&_l;

  if (thick <= 0)
    return NULL;
  half = (thick + 1) / 2;
  d =
    sqrt (SQUARE (l->Point1.X - l->Point2.X) +
          SQUARE (l->Point1.Y - l->Point2.Y));
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

/* make a rounded-corner rectangle */
POLYAREA *
SquarePadPoly (PadType * pad, BDimension clear)
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

  d =
    sqrt (SQUARE (pad->Point1.X - pad->Point2.X) +
          SQUARE (pad->Point1.Y - pad->Point2.Y));
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

/* clear np1 from the polygon */
static int
Subtract (POLYAREA * np1, PolygonType * p, Boolean fnp)
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

/* create a polygon of the pin clearance */
POLYAREA *
PinPoly (PinType * pin, BDimension thick, BDimension clear)
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
BoxPolyBloated (BoxType *box, BDimension bloat)
{
  return RectPoly (box->X1 - bloat, box->X2 + bloat,
                   box->Y1 - bloat, box->Y2 + bloat);
}

/* remove the pin clearance from the polygon */
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
      np = ThermPoly ((PCBTypePtr) (d->pcb), pin, i);
      if (!np)
        return 0;
    }
  else
    {
      np = PinPoly (pin, pin->Thickness, pin->Clearance);
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
  return Subtract (np, p, True);
}

static int
SubtractArc (ArcType * arc, PolygonType * p)
{
  POLYAREA *np;

  if (!TEST_FLAG (CLEARLINEFLAG, arc))
    return 0;
  if (!(np = ArcPoly (arc, arc->Thickness + arc->Clearance)))
    return -1;
  return Subtract (np, p, True);
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
  return Subtract (np, p, True);
}

static int
SubtractPad (PadType * pad, PolygonType * p)
{
  POLYAREA *np = NULL;

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
  return Subtract (np, p, True);
}

struct cpInfo
{
  const BoxType *other;
  DataType *data;
  LayerType *layer;
  PolygonType *polygon;
  Boolean solder;
  jmp_buf env;
};

static int
pin_sub_callback (const BoxType * b, void *cl)
{
  PinTypePtr pin = (PinTypePtr) b;
  struct cpInfo *info = (struct cpInfo *) cl;
  PolygonTypePtr polygon;

  /* don't subtract the object that was put back! */
  if (b == info->other)
    return 0;
  polygon = info->polygon;
  if (SubtractPin (info->data, pin, info->layer, polygon) < 0)
    longjmp (info->env, 1);
  return 1;
}

static int
arc_sub_callback (const BoxType * b, void *cl)
{
  ArcTypePtr arc = (ArcTypePtr) b;
  struct cpInfo *info = (struct cpInfo *) cl;
  PolygonTypePtr polygon;

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
  PadTypePtr pad = (PadTypePtr) b;
  struct cpInfo *info = (struct cpInfo *) cl;
  PolygonTypePtr polygon;

  /* don't subtract the object that was put back! */
  if (b == info->other)
    return 0;
  polygon = info->polygon;
  if (XOR (TEST_FLAG (ONSOLDERFLAG, pad), !info->solder))
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
  LineTypePtr line = (LineTypePtr) b;
  struct cpInfo *info = (struct cpInfo *) cl;
  PolygonTypePtr polygon;

  /* don't subtract the object that was put back! */
  if (b == info->other)
    return 0;
  if (!TEST_FLAG (CLEARLINEFLAG, line))
    return 0;
  polygon = info->polygon;
  if (SubtractLine (line, polygon) < 0)
    longjmp (info->env, 1);
  return 1;
}

static int
text_sub_callback (const BoxType * b, void *cl)
{
  TextTypePtr text = (TextTypePtr) b;
  struct cpInfo *info = (struct cpInfo *) cl;
  PolygonTypePtr polygon;

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
Group (DataTypePtr Data, Cardinal layer)
{
  Cardinal i, j;
  for (i = 0; i < max_layer; i++)
    for (j = 0; j < ((PCBType *) (Data->pcb))->LayerGroups.Number[i]; j++)
      if (layer == ((PCBType *) (Data->pcb))->LayerGroups.Entries[i][j])
        return i;
  return i;
}

static int
clearPoly (DataTypePtr Data, LayerTypePtr Layer, PolygonType * polygon,
           const BoxType * here, BDimension expand)
{
  int r = 0;
  BoxType region;
  struct cpInfo info;
  Cardinal group;

  if (!TEST_FLAG (CLEARPOLYFLAG, polygon)
      || GetLayerNumber (Data, Layer) >= max_layer)
    return 0;
  group = Group (Data, GetLayerNumber (Data, Layer));
  info.solder = (group == Group (Data, max_layer + SOLDER_LAYER));
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
      r = r_search (Data->via_tree, &region, NULL, pin_sub_callback, &info);
      r += r_search (Data->pin_tree, &region, NULL, pin_sub_callback, &info);
      GROUP_LOOP (Data, group);
      {
        r +=
          r_search (layer->line_tree, &region, NULL, line_sub_callback,
                    &info);
        r +=
          r_search (layer->arc_tree, &region, NULL, arc_sub_callback, &info);
	r +=
          r_search (layer->text_tree, &region, NULL, text_sub_callback, &info);
      }
      END_LOOP;
      if (info.solder || group == Group (Data, max_layer + COMPONENT_LAYER))
	r += r_search (Data->pad_tree, &region, NULL, pad_sub_callback, &info);
    }
  polygon->NoHolesValid = 0;
  return r;
}

static int
Unsubtract (POLYAREA * np1, PolygonType * p)
{
  POLYAREA *merged = NULL, *np = np1;
  int x;
  assert (np);
  assert (p && p->Clipped);
  x = poly_Boolean_free (p->Clipped, np, &merged, PBO_UNITE);
  if (x != err_ok)
    {
      fprintf (stderr, "Error while clipping PBO_UNITE: %d\n", x);
      poly_Free (&merged);
      p->Clipped = NULL;
      if (p->NoHoles) printf ("Just leaked in Unsubtract\n");
      p->NoHoles = NULL;
      return 0;
    }
  p->Clipped = biggest (merged);
  assert (!p->Clipped || poly_Valid (p->Clipped));
  return ClipOriginal (p);
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

static Boolean inhibit = False;

int
InitClip (DataTypePtr Data, LayerTypePtr layer, PolygonType * p)
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

/* --------------------------------------------------------------------------
 * remove redundant polygon points. Any point that lies on the straight
 * line between the points on either side of it is redundant.
 * returns true if any points are removed
 */
Boolean
RemoveExcessPolygonPoints (LayerTypePtr Layer, PolygonTypePtr Polygon)
{
  PointTypePtr pt1, pt2, pt3;
  Cardinal n;
  LineType line;
  Boolean changed = False;

  if (Undoing ())
    return (False);
  /* there are always at least three points in a polygon */
  pt1 = &Polygon->Points[Polygon->PointN - 1];
  pt2 = &Polygon->Points[0];
  pt3 = &Polygon->Points[1];
  for (n = 0; n < Polygon->PointN; n++, pt1++, pt2++, pt3++)
    {
      /* wrap around polygon */
      if (n == 1)
        pt1 = &Polygon->Points[0];
      if (n == Polygon->PointN - 1)
        pt3 = &Polygon->Points[0];
      line.Point1 = *pt1;
      line.Point2 = *pt3;
      line.Thickness = 0;
      if (IsPointOnLine ((float) pt2->X, (float) pt2->Y, 0.0, &line))
        {
          RemoveObject (POLYGONPOINT_TYPE, (void *) Layer, (void *) Polygon,
                        (void *) pt2);
          changed = True;
        }
    }
  return (changed);
}

/* ---------------------------------------------------------------------------
 * returns the index of the polygon point which is the end
 * point of the segment with the lowest distance to the passed
 * coordinates
 */
Cardinal
GetLowestDistancePolygonPoint (PolygonTypePtr Polygon, LocationType X,
                               LocationType Y)
{
  double mindistance = (double) MAX_COORD * MAX_COORD;
  PointTypePtr ptr1 = &Polygon->Points[Polygon->PointN - 1],
    ptr2 = &Polygon->Points[0];
  Cardinal n, result = 0;

  /* we calculate the distance to each segment and choose the
   * shortest distance. If the closest approach between the
   * given point and the projected line (i.e. the segment extended)
   * is not on the segment, then the distance is the distance
   * to the segment end point.
   */

  for (n = 0; n < Polygon->PointN; n++, ptr2++)
    {
      register double u, dx, dy;
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
      ptr1 = ptr2;
    }
  return (result);
}

/* ---------------------------------------------------------------------------
 * go back to the  previous point of the polygon
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
        PointTypePtr points = Crosshair.AttachedPolygon.Points;
        Cardinal n = Crosshair.AttachedPolygon.PointN - 2;

        Crosshair.AttachedPolygon.PointN--;
        Crosshair.AttachedLine.Point1.X = points[n].X;
        Crosshair.AttachedLine.Point1.Y = points[n].Y;
        break;
      }
    }
}

/* ---------------------------------------------------------------------------
 * close polygon if possible
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
          BDimension dx, dy;

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

/* ---------------------------------------------------------------------------
 * moves the data of the attached (new) polygon to the current layer
 */
void
CopyAttachedPolygonToLayer (void)
{
  PolygonTypePtr polygon;
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
  DrawPolygon (CURRENT, polygon, 0);
  SetChangedFlag (True);

  /* reset state of attached line */
  Crosshair.AttachedLine.State = STATE_FIRST;
  addedLines = 0;

  /* add to undo list */
  AddObjectToCreateUndoList (POLYGON_TYPE, CURRENT, polygon, polygon);
  IncrementUndoSerialNumber ();
}

/* find polygon holes in range, then call the callback function for
 * each hole. If the callback returns non-zero, stop
 * the search.
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
  LayerTypePtr layer;
  DataTypePtr data;
  int (*callback) (DataTypePtr, LayerTypePtr, PolygonTypePtr, int, void *,
                   void *);
};

static int
subtract_plow (DataTypePtr Data, LayerTypePtr Layer, PolygonTypePtr Polygon,
               int type, void *ptr1, void *ptr2)
{
  if (!Polygon->Clipped)
    return 0;
  switch (type)
    {
    case PIN_TYPE:
    case VIA_TYPE:
      SubtractPin (Data, (PinTypePtr) ptr2, Layer, Polygon);
      Polygon->NoHolesValid = 0;
      return 1;
    case LINE_TYPE:
      SubtractLine ((LineTypePtr) ptr2, Polygon);
      Polygon->NoHolesValid = 0;
      return 1;
    case ARC_TYPE:
      SubtractArc ((ArcTypePtr) ptr2, Polygon);
      Polygon->NoHolesValid = 0;
      return 1;
    case PAD_TYPE:
      SubtractPad ((PadTypePtr) ptr2, Polygon);
      Polygon->NoHolesValid = 0;
      return 1;
    case TEXT_TYPE:
      SubtractText ((TextTypePtr) ptr2, Polygon);
      Polygon->NoHolesValid = 0;
      return 1;
    }
  return 0;
}

static int
add_plow (DataTypePtr Data, LayerTypePtr Layer, PolygonTypePtr Polygon,
          int type, void *ptr1, void *ptr2)
{
  switch (type)
    {
    case PIN_TYPE:
    case VIA_TYPE:
      UnsubtractPin ((PinTypePtr) ptr2, Layer, Polygon);
      return 1;
    case LINE_TYPE:
      UnsubtractLine ((LineTypePtr) ptr2, Layer, Polygon);
      return 1;
    case ARC_TYPE:
      UnsubtractArc ((ArcTypePtr) ptr2, Layer, Polygon);
      return 1;
    case PAD_TYPE:
      UnsubtractPad ((PadTypePtr) ptr2, Layer, Polygon);
      return 1;
    case TEXT_TYPE:
      UnsubtractText ((TextTypePtr) ptr2, Layer, Polygon);
      return 1;
    }
  return 0;
}

static int
plow_callback (const BoxType * b, void *cl)
{
  struct plow_info *plow = (struct plow_info *) cl;
  PolygonTypePtr polygon = (PolygonTypePtr) b;

  if (TEST_FLAG (CLEARPOLYFLAG, polygon))
    return plow->callback (plow->data, plow->layer, polygon, plow->type,
                           plow->ptr1, plow->ptr2);
  return 0;
}

int
PlowsPolygon (DataType * Data, int type, void *ptr1, void *ptr2,
              int (*call_back) (DataTypePtr data, LayerTypePtr lay,
                                PolygonTypePtr poly, int type, void *ptr1,
                                void *ptr2))
{
  BoxType sb = ((PinTypePtr) ptr2)->BoundingBox;
  int r = 0;
  struct plow_info info;

  info.type = type;
  info.ptr1 = ptr1;
  info.ptr2 = ptr2;
  info.data = Data;
  info.callback = call_back;
  switch (type)
    {
    case PIN_TYPE:
    case VIA_TYPE:
      if (type == PIN_TYPE || ptr1 == ptr2 || ptr1 == NULL)
        {
          LAYER_LOOP (Data, max_layer);
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
                                                                         ((LayerTypePtr) ptr1))));
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
      if (!TEST_FLAG (CLEARLINEFLAG, (LineTypePtr) ptr2))
        return 0;
      /* silk doesn't plow */
      if (GetLayerNumber (Data, ptr1) >= max_layer)
        return 0;
      GROUP_LOOP (Data, GetLayerGroupNumberByNumber (GetLayerNumber (Data,
                                                                     ((LayerTypePtr) ptr1))));
      {
        info.layer = layer;
        r += r_search (layer->polygon_tree, &sb, NULL, plow_callback, &info);
      }
      END_LOOP;
      break;
    case PAD_TYPE:
      {
        Cardinal group = TEST_FLAG (ONSOLDERFLAG,
                                    (PadType *) ptr2) ? SOLDER_LAYER :
          COMPONENT_LAYER;
        group = GetLayerGroupNumberByNumber (max_layer + group);
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
          PlowsPolygon (Data, PIN_TYPE, ptr1, pin, call_back);
        }
        END_LOOP;
        PAD_LOOP ((ElementType *) ptr1);
        {
          PlowsPolygon (Data, PAD_TYPE, ptr1, pad, call_back);
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
  if (type == POLYGON_TYPE)
    InitClip (PCB->Data, (LayerTypePtr) ptr1, (PolygonTypePtr) ptr2);
  else
    PlowsPolygon (Data, type, ptr1, ptr2, add_plow);
}

void
ClearFromPolygon (DataType * Data, int type, void *ptr1, void *ptr2)
{
  if (type == POLYGON_TYPE)
    InitClip (PCB->Data, (LayerTypePtr) ptr1, (PolygonTypePtr) ptr2);
  else
    PlowsPolygon (Data, type, ptr1, ptr2, subtract_plow);
}

Boolean
isects (POLYAREA * a, PolygonTypePtr p, Boolean fr)
{
  POLYAREA *x;
  Boolean ans;
  ans = Touching (a, p->Clipped);
  /* argument may be register, so we must copy it */
  x = a;
  if (fr)
    poly_Free (&x);
  return ans;
}


Boolean
IsPointInPolygon (LocationType X, LocationType Y, BDimension r,
                  PolygonTypePtr p)
{
  POLYAREA *c;
  Vector v;
  v[0] = X;
  v[1] = Y;
  if (poly_CheckInside (p->Clipped, v))
    return True;
  if (r < 1)
    return False;
  if (!(c = CirclePoly (X, Y, r)))
    return False;
  return isects (c, p, True);
}


Boolean
IsPointInPolygonIgnoreHoles (LocationType X, LocationType Y, PolygonTypePtr p)
{
  Vector v;
  v[0] = X;
  v[1] = Y;
  return poly_InsideContour (p->Clipped->contours, v);
}

Boolean
IsRectangleInPolygon (LocationType X1, LocationType Y1, LocationType X2,
                      LocationType Y2, PolygonTypePtr p)
{
  POLYAREA *s;
  if (!
      (s = RectPoly (min (X1, X2), max (X1, X2), min (Y1, Y2), max (Y1, Y2))))
    return False;
  return isects (s, p, True);
}

/* NB: This function will free the passed POLYAREA.
 *     It must only be passed a single POLYAREA (pa->f == pa->b == pa)
 */
static void
r_NoHolesPolygonDicer (POLYAREA * pa,
                       void (*emit) (PLINE *, void *), void *user_data)
{
  PLINE *p = pa->contours;

  if (!pa->contours->next)                 /* no holes */
    {
      pa->contours = NULL; /* The callback now owns the contour */
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
NoHolesPolygonDicer (PolygonTypePtr p, const BoxType * clip,
                     void (*emit) (PLINE *, void *), void *user_data)
{
  POLYAREA *save, *ans, *cur, *next;

  ans = save = poly_Create ();
  /* copy the main poly only */
  poly_Copy1 (save, p->Clipped);
  /* clip to the bounding box */
  if (clip)
    {
      POLYAREA *cbox = RectPoly (clip->X1, clip->X2, clip->Y1, clip->Y2);
      if (cbox)
        {
          int r = poly_Boolean_free (save, cbox, &ans, PBO_ISECT);
          save = ans;
          if (r != err_ok)
            save = NULL;
        }
    }
  if (!save)
    return;
  /* Now dice it up.
   * NB: Could be more than one piece (because of the clip above)
   */
  cur = save;
  do
    {
      next = cur->f;
      cur->f = cur->b = cur; /* Detach this polygon piece */
      r_NoHolesPolygonDicer (cur, emit, user_data);
      /* NB: The POLYAREA was freed by its use in the recursive dicer */
    }
  while ((cur = next) != save);
}

/* make a polygon split into multiple parts into multiple polygons */
Boolean
MorphPolygon (LayerTypePtr layer, PolygonTypePtr poly)
{
  POLYAREA *p, *start;
  Boolean many = False;
  FlagType flags;

  if (!poly->Clipped || TEST_FLAG (LOCKFLAG, poly))
    return False;
  if (poly->Clipped->f == poly->Clipped)
    return False;
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
  inhibit = True;
  do
    {
      VNODE *v;
      PolygonTypePtr new;

      if (p->contours->area > PCB->IsleArea)
        {
          new = CreateNewPolygon (layer, flags);
          if (!new)
            return False;
          many = True;
          v = &p->contours->head;
          CreateNewPointInPolygon (new, v->point[0], v->point[1]);
          for (v = v->next; v != &p->contours->head; v = v->next)
            CreateNewPointInPolygon (new, v->point[0], v->point[1]);
          new->BoundingBox.X1 = p->contours->xmin;
          new->BoundingBox.X2 = p->contours->xmax + 1;
          new->BoundingBox.Y1 = p->contours->ymin;
          new->BoundingBox.Y2 = p->contours->ymax + 1;
          AddObjectToCreateUndoList (POLYGON_TYPE, layer, new, new);
          new->Clipped = p;
          p = p->f;             /* go to next pline */
          new->Clipped->b = new->Clipped->f = new->Clipped;     /* unlink from others */
          r_insert_entry (layer->polygon_tree, (BoxType *) new, 0);
          DrawPolygon (layer, new, 0);
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
  inhibit = False;
  IncrementUndoSerialNumber ();
  return many;
}

void debug_pline (PLINE *pl)
{
  VNODE *v;
  fprintf (stderr, "\txmin %d xmax %d ymin %d ymax %d\n",
	   pl->xmin, pl->xmax, pl->ymin, pl->ymax);
  v = &pl->head;
  while (v)
    {
      fprintf(stderr, "\t\tvnode: %d,%d\n", v->point[0], v->point[1]);
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
  int i;
  POLYAREA *pa;
  fprintf (stderr, "POLYGON %p  %d pts\n", p, p->PointN);
  for (i=0; i<p->PointN; i++)
    fprintf(stderr, "\t%d: %d, %d\n", i, p->Points[i].X, p->Points[i].Y);
  pa = p->Clipped;
  while (pa)
    {
      debug_polyarea (pa);
      pa = pa->f;
      if (pa == p->Clipped)
	break;
    }
}
