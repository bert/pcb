/*!
 * \file src/puller.c
 *
 * \brief .
 *
 * \todo Things that need to be fixed before this is "perfect".\n
 * Add to this list as we find things.
 * - respect the outline layer.
 * - don't consider points that are perpendicular to our start_arc.
 *   I.e. when we have busses going around corners, we have a *lot* of
 *   arcs and endpoints that are all in the same direction and all
 *   equally "good", but rounding the arc angles to integers causes
 *   all sorts of tiny differences that result in bumps, reversals,
 *   and other messes.
 * - Store the X,Y values in our shadow struct so we don't fill up the
 *   undo buffer with all our line reversals.
 * - at least check the other layers in our layer group.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2006 DJ Delorie
 *
 * Copyright (C) 2011 PCB Contributers (See ChangeLog for details)
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
 * DJ Delorie, 334 North Road, Deerfield NH 03037-1110, USA
 * dj@delorie.com
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"

#include <math.h>
#include <memory.h>
#include <limits.h>
#include <setjmp.h>


#include "create.h"
#include "data.h"
#include "draw.h"
#include "misc.h"
#include "move.h"
#include "pcb-printf.h"
#include "remove.h"
#include "rtree.h"
#include "strflags.h"
#include "undo.h"
#include "error.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define abort1() fprintf(stderr, "abort at line %d\n", __LINE__), abort()

#define TRACE0 0
#define TRACE1 0

/*! Sine of one degree. */
#define SIN1D	0.0174524064372835

static jmp_buf abort_buf;
static int multi, line_exact, arc_exact;
static LineType *the_line;
static ArcType *the_arc;
static double arc_dist;

/* We canonicalize the arc and line such that the point to be moved is
   always Point2 for the line, and at start+delta for the arc.  */

static Coord x, y;		/* the point we're moving */
static Coord cx, cy;		/* centerpoint of the arc */
static Coord ex, ey;		/* fixed end of the line */

/* 0 is left (-x), 90 is down (+y), 180 is right (+x), 270 is up (-y) */

static Coord
within (Coord x1, Coord y1, Coord x2, Coord y2, Coord r)
{
  return Distance (x1, y1, x2, y2) <= r / 2;
}

static int
arc_endpoint_is (ArcType *a, int angle, Coord x, Coord y)
{
  Coord ax = a->X, ay = a->Y;

  if (angle % 90 == 0)
    {
      int ai = (int) (angle / 90) & 3;
      switch (ai)
	{
	case 0:
	  ax -= a->Width;
	  break;
	case 1:
	  ay += a->Height;
	  break;
	case 2:
	  ax += a->Width;
	  break;
	case 3:
	  ay -= a->Height;
	  break;
	}
    }
  else
    {
      double rad = angle * M_PI / 180;
      ax -= a->Width * cos (rad);
      ay += a->Width * sin (rad);
    }
#if TRACE1
  pcb_printf (" - arc endpoint %#mD\n", ax, ay);
#endif
  arc_dist = Distance (ax, ay, x, y);
  if (arc_exact)
    return arc_dist < 2;
  return arc_dist < a->Thickness / 2;
}

/*!
 * brief Cross c->u and c->v, return the magnitude.
 */
static double
cross2d (Coord cx, Coord cy, Coord ux, Coord uy, Coord vx, Coord vy)
{
  ux -= cx;
  uy -= cy;
  vx -= cx;
  vy -= cy;
  return (double)ux * vy - (double)uy * vx;
}

/*!
 * \brief Likewise, for dot product.
 */
static double
dot2d (Coord cx, Coord cy, Coord ux, Coord uy, Coord vx, Coord vy)
{
  ux -= cx;
  uy -= cy;
  vx -= cx;
  vy -= cy;
  return (double)ux * vx + (double)uy * vy;
}

#if 0
/*!
 * \brief .
 *
 * angle of c->v, relative to c->u, in radians.
 * Range is -pi..pi.
 */
static double
angle2d (Coord cx, Coord cy, Coord ux, Coord uy, Coord vx, Coord vy)
{
  double cross;
  double magu, magv, sintheta;
#if TRACE1
  pcb_printf("angle2d %mD %mD %mD\n", cx, cy, ux, uy, vx, vy);
#endif
  ux -= cx;
  uy -= cy;
  vx -= cx;
  vy -= cy;
#if TRACE1
  pcb_printf(" = %mD %mD\n", ux, uy, vx, vy);
#endif
  cross = (double)ux * vy - (double)uy * vx;
  magu = hypot(ux, uy);
  magv = hypot(vx, vy);
  sintheta = cross / (magu * magv);
#if TRACE1
  printf(" = %f / (%f * %f) = %f\n", cross, magu, magv, sintheta);
#endif
  return asin (sintheta);
}
#endif

static int
same_sign (double a, double b)
{
  return (a * b >= 0);
}

static double
r2d (double r)
{
  return 180.0 * r / M_PI;
}

static double
d2r (double d)
{
  return M_PI * d / 180.0;
}

/*!
 * \brief .
 *
 * | a b |
 *
 * | c d |
 */
static double
det (double a, double b, double c, double d)
{
  return a * d - b * c;
}

/*!
 * \brief .
 *
 * The lines are \f$ x_1y_1-x_2y_2 \f$ and \f$ x_3y_3-x_4y_4 \f$.
 *
 * \return True if they intersect.
 */
static int
intersection_of_lines (Coord x1, Coord y1, Coord x2, Coord y2,
                       Coord x3, Coord y3, Coord x4, Coord y4,
                       Coord *xr, Coord *yr)
{
  double x, y, d;
  d = det (x1 - x2, y1 - y2, x3 - x4, y3 - y4);
  if (!d)
    return 0;
  x = (det (det (x1, y1, x2, y2), x1 - x2,
	    det (x3, y3, x4, y4), x3 - x4) / d);
  y = (det (det (x1, y1, x2, y2), y1 - y2,
	    det (x3, y3, x4, y4), y3 - y4) / d);
  *xr = (Coord) (x + 0.5);
  *yr = (Coord) (y + 0.5);
  return 1;
}

/*!
 * \brief Same, for line segments.
 *
 * \return True if they intersect.
 *
 * For this function, \c xr and \c yr may be \c NULL if you don't need
 * the values.
 */
static int
intersection_of_linesegs (Coord x1, Coord y1, Coord x2, Coord y2,
                          Coord x3, Coord y3, Coord x4, Coord y4,
                          Coord *xr, Coord *yr)
{
  double x, y, d;
  d = det (x1 - x2, y1 - y2, x3 - x4, y3 - y4);
  if (!d)
    return 0;
  x = (det (det (x1, y1, x2, y2), x1 - x2,
	    det (x3, y3, x4, y4), x3 - x4) / d);
  y = (det (det (x1, y1, x2, y2), y1 - y2,
	    det (x3, y3, x4, y4), y3 - y4) / d);
  if (MIN (x1, x2) > x || x > MAX (x1, x2)
      || MIN (y1, y2) > y || y > MAX (y1, y2))
    return 0;
  if (MIN (x3, x4) > x || x > MAX (x3, x4)
      || MIN (y3, y4) > y || y > MAX (y3, y4))
    return 0;
  if (xr)
    *xr = (Coord) (x + 0.5);
  if (yr)
    *yr = (Coord) (y + 0.5);
  return 1;
}

/*!
 * \brief Distance between a line and a point.
 */
static double
dist_lp (Coord x1, Coord y1, Coord x2, Coord y2, Coord px, Coord py)
{
  double den = Distance (x1, y1, x2, y2);
  double rv = (fabs (((double)x2 - x1) * ((double)y1 - py)
		     - ((double)x1 - px) * ((double)y2 - y1))
	       / den);
#if TRACE1
  pcb_printf("dist %#mD-%#mD to %#mD is %f\n",
	 x1, y1, x2, y2, px, py, rv);
#endif
  return rv;
}

/*!
 * \brief Distance between a line segment and a point.
 */
static double
dist_lsp (Coord x1, Coord y1, Coord x2, Coord y2, Coord px, Coord py)
{
  double d;
  if (dot2d (x1, y1, x2, y2, px, py) < 0)
    return Distance (x1, y1, px, py);
  if (dot2d (x2, y2, x1, y1, px, py) < 0)
    return Distance (x2, y2, px, py);
  d = (fabs (((double)x2 - x1) * ((double)y1 - py)
	     - ((double)x1 - px) * ((double)y2 - y1))
       / Distance (x1, y1, x2, y2));
  return d;
}

/*                       Single Point Puller                                 */

static int
line_callback (const BoxType * b, void *cl)
{
  /* LayerType *layer = (LayerType *)cl; */
  LineType *l = (LineType *) b;
  double d1, d2, t;
#if TRACE1
  pcb_printf ("line %#mD .. %#mD\n",
	  l->Point1.X, l->Point1.Y, l->Point2.X, l->Point2.Y);
#endif
  d1 = Distance (l->Point1.X, l->Point1.Y, x, y);
  d2 = Distance (l->Point2.X, l->Point2.Y, x, y);
  if ((d1 < 2 || d2 < 2) && !line_exact)
    {
      line_exact = 1;
      the_line = 0;
    }
  t = line_exact ? 2 : l->Thickness / 2;
  if (d1 < t || d2 < t)
    {
      if (the_line)
	multi = 1;
      the_line = l;
#if TRACE1
      printf ("picked, exact %d\n", line_exact);
#endif
    }
  return 1;
}

static int
arc_callback (const BoxType * b, void *cl)
{
  /* LayerType *layer = (LayerType *) cl; */
  ArcType *a = (ArcType *) b;

#if TRACE1
  pcb_printf ("arc a %#mD r %#mS sa %ld d %ld\n", a->X, a->Y, a->Width,
	  a->StartAngle, a->Delta);
#endif
  if (!arc_endpoint_is (a, a->StartAngle, x, y)
      && !arc_endpoint_is (a, a->StartAngle + a->Delta, x, y))
    return 1;
  if (arc_dist < 2)
    {
      if (!arc_exact)
	{
	  arc_exact = 1;
	  the_arc = 0;
	}
      if (the_arc)
	multi = 1;
      the_arc = a;
#if TRACE1
      printf ("picked, exact %d\n", arc_exact);
#endif
    }
  else if (!arc_exact)
    {
      if (the_arc)
	multi = 1;
      the_arc = a;
#if TRACE1
      printf ("picked, exact %d\n", arc_exact);
#endif
    }
  return 1;
}

static int
find_pair (Coord Px, Coord Py)
{
  BoxType spot;

#if TRACE1
  pcb_printf ("\nPuller find_pair at %#mD\n", Px, Py);
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


static const char puller_syntax[] = "Puller()";

static const char puller_help[] = "Pull an arc-line junction tight.";

/* %start-doc actions Puller

The @code{Puller()} action is a special-purpose optimization.  When
invoked while the crosshair is over the junction of an arc and a line,
it will adjust the arc's angle and the connecting line's endpoint such
that the line intersects the arc at a tangent.  In the example below,
the left side is ``before'' with the black target marking where to put
the crosshair:

@center @image{puller,,,Example of how puller works,png}

The right side is ``after'' with the black target marking where the
arc-line intersection was moved to.

%end-doc */

static int
Puller (int argc, char **argv, Coord Ux, Coord Uy)
{
  double arc_angle, base_angle;
#if TRACE1
  double line_angle, rel_angle;
#endif
  double tangent;
  int new_delta_angle;

  if (!find_pair (Crosshair.X, Crosshair.Y))
    if (!find_pair (Ux, Uy))
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
#if TRACE1
      printf ("Line endpoint not at cursor\n");
#endif
      return 1;
    }
  ex = the_line->Point1.X;
  ey = the_line->Point1.Y;

  cx = the_arc->X;
  cy = the_arc->Y;
  if (arc_endpoint_is (the_arc, the_arc->StartAngle, x, y))
    {
      ChangeArcAngles (CURRENT, the_arc, the_arc->StartAngle + the_arc->Delta,
		       -the_arc->Delta);
    }
  else if (!arc_endpoint_is (the_arc, the_arc->StartAngle + the_arc->Delta,
			     x, y))
    {
#if TRACE1
      printf ("arc not endpoints\n");
#endif
      return 1;
    }

  if (within (cx, cy, ex, ey, the_arc->Width * 2))
    {
#if TRACE1
      printf ("line ends inside arc\n");
#endif
      return 1;
    }

  if (the_arc->Delta > 0)
    arc_angle = the_arc->StartAngle + the_arc->Delta + 90;
  else
    arc_angle = the_arc->StartAngle + the_arc->Delta - 90;
  base_angle = r2d (atan2 (ey - cy, cx - ex));

  tangent = r2d (acos (the_arc->Width / Distance (cx, cy, ex, ey)));

#if TRACE1
  line_angle = r2d (atan2 (ey - y, x - ex));
  rel_angle = line_angle - arc_angle;
  printf ("arc %g line %g rel %g base %g\n", arc_angle, line_angle, rel_angle,
	  base_angle);
  printf ("tangent %g\n", tangent);

  printf ("arc was start %ld end %ld\n", the_arc->StartAngle,
	  the_arc->StartAngle + the_arc->Delta);
#endif

  if (the_arc->Delta > 0)
    arc_angle = base_angle - tangent;
  else
    arc_angle = base_angle + tangent;
#if TRACE1
  printf ("new end angle %g\n", arc_angle);
#endif

  new_delta_angle = arc_angle - the_arc->StartAngle;
  if (new_delta_angle > 180)
    new_delta_angle -= 360;
  if (new_delta_angle < -180)
    new_delta_angle += 360;
  ChangeArcAngles (CURRENT, the_arc, the_arc->StartAngle, new_delta_angle);

#if TRACE1
  printf ("arc now start %ld end %ld\n", the_arc->StartAngle,
	  the_arc->StartAngle + new_delta_angle);
#endif

  arc_angle = the_arc->StartAngle + the_arc->Delta;
  x = the_arc->X - the_arc->Width * cos (d2r (arc_angle)) + 0.5;
  y = the_arc->Y + the_arc->Height * sin (d2r (arc_angle)) + 0.5;

  MoveObject (LINEPOINT_TYPE, CURRENT, the_line, &(the_line->Point2),
	      x - the_line->Point2.X, y - the_line->Point2.Y);

  gui->invalidate_all ();
  IncrementUndoSerialNumber ();

  return 1;
}

/*                          Global Puller                                    */

static const char globalpuller_syntax[] =
"GlobalPuller()";

static const char globalpuller_help[] =
"Pull all traces tight.";

/* %start-doc actions GlobalPuller

%end-doc */

/* Ok, here's the deal.  We look for the intersection of two traces.
   The triangle formed by those traces is searched for things we need
   to avoid. From the other two corners of the triangle, we compute
   the angle to each obstacle, and remember the ones closest to the
   start angles.  If the two traces hit the same obstacle, we put in
   the arc and we're done.  Else, we bring the traces up to the
   obstacles and start again.

   Note that we assume each start point is a tangent to an arc.  We
   start with a radius of zero, but future steps use the arcs we
   create as we go.

   For obstacles, we list each round pin, pad, via, and line/arc
   endpoints as points with a given radius.  For each square pin, pad,
   via, and polygon points, we list each corner with a zero radius.
   We also list arcs from their centerpoint.

   We don't currently do anything to move vias, or intersections of
   three or more traces.  In the future, three-way intersections will
   be handles similarly to two-way - calculate the range of angles
   valid from each of the three other endpoints, choose the angle
   closest to making 120 degree angles at the center.  For four-way or
   more intersections, we break them up into multiple three-way
   intersections.

   For simplicity, we only do the current layer at this time.  We will
   also edit the lines and arcs in place so that the intersection is
   always on the second point, and the other ends are always at
   start+delta for arcs.

   We also defer intersections which are blocked by other
   intersections yet to be moved; the idea is to wait until those have
   been moved so we don't end up with arcs that no longer wrap around
   things.  At a later point, we may choose to pull arced corners in
   also.

   You'll see lots of variables of the form "foo_sign" which keep
   track of which way things are pointing.  This is because everything
   is relative to corners and arcs, not absolute directions.
*/

static int nloops, npulled;

static void
status ()
{
  Message ("%6d loops, %d pulled   \r", nloops, npulled);
}

/*!
 * \brief Extra data we need to temporarily attach to all lines and
 * arcs.
 */
typedef struct End {
  /* These point to "multi_next" if there are more than one.  */
  struct Extra *next;
  void *pin;
  unsigned char in_pin:1;
  unsigned char at_pin:1;
  unsigned char is_pad:1;
  unsigned char pending:1; /* set if this may be moved later */
  Coord x, y; /* arc endpoint */
  /* If not NULL, points to End with pending==1 we're blocked on. */
  struct End *waiting_for;
} End;

typedef struct Extra {
  End start;
  End end;
  unsigned char found:1;
  unsigned char deleted:1;
  int type;
  union {
    LineType *line;
    ArcType *arc;
  } parent;
} Extra;

static Extra multi_next;
static GHashTable *lines;
static GHashTable *arcs;
static int did_something;
static int current_is_top, current_is_bottom;

/* If set, these are the pins/pads/vias that this path ends on.  */
/* static void *start_pin_pad, *end_pin_pad; */

#if TRACE1
static void trace_paths ();
#endif
static void mark_line_for_deletion (LineType *);

#define LINE2EXTRA(l)    ((Extra *)g_hash_table_lookup (lines, l))
#define ARC2EXTRA(a)     ((Extra *)g_hash_table_lookup (arcs, a))
#define EXTRA2LINE(e)    (e->parent.line)
#define EXTRA2ARC(e)     (e->parent.arc)
#define EXTRA_IS_LINE(e) (e->type == LINE_TYPE)
#define EXTRA_IS_ARC(e)  (e->type == ARC_TYPE)

static void
unlink_end (Extra *x, Extra **e)
{
  if (*e)
    {
      if ((*e)->start.next == x)
	{
#if TRACE1
	  printf("%d: unlink_end, was %p\n", __LINE__, (*e)->start.next);
#endif
	  (*e)->start.next = &multi_next;
	}
      if ((*e)->end.next == x)
	{
#if TRACE1
	  printf("%d: unlink_end, was %p\n", __LINE__, (*e)->start.next);
#endif
	  (*e)->end.next = &multi_next;
	}
    }
#if TRACE1
  printf("%d: unlink_end, was %p\n", __LINE__, (*e));
#endif
  (*e) = &multi_next;
}

#if TRACE1

static void
clear_found_cb (AnyObjectType *ptr, Extra *extra, void *userdata)
{
  extra->found = 0;
}

static void
clear_found ()
{
  g_hash_table_foreach (lines, (GHFunc)clear_found_cb, NULL);
  g_hash_table_foreach (arcs, (GHFunc)clear_found_cb, NULL);
}
#endif

static void
fix_arc_extra (ArcType *a, Extra *e)
{
#if TRACE1
  printf("new arc angles %ld %ld\n", a->StartAngle, a->Delta);
#endif
  e->start.x = a->X - (a->Width * cos (d2r (a->StartAngle)) + 0.5);
  e->start.y = a->Y + (a->Height * sin (d2r (a->StartAngle)) + 0.5);
  e->end.x = a->X - (a->Width * cos (d2r (a->StartAngle+a->Delta)) + 0.5);
  e->end.y = a->Y + (a->Height * sin (d2r (a->StartAngle+a->Delta)) + 0.5);
#if TRACE1
  pcb_printf("new X,Y is %#mD to %#mD\n", e->start.x, e->start.y, e->end.x, e->end.y);
#endif
}

typedef struct {
  void *me;
  Coord x, y;
  int is_arc;
  Extra **extra_ptr;
} FindPairCallbackStruct;

#define NEAR(a,b) ((a) <= (b) + 2 && (a) >= (b) - 2)

static int
find_pair_line_callback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
#if TRACE1
  Extra *e = LINE2EXTRA (line);
#endif
  FindPairCallbackStruct *fpcs = (FindPairCallbackStruct *) cl;

  if (line == fpcs->me)
    return 0;
#ifdef CHECK_LINE_PT_NEG
  if (line->Point1.X < 0)
    abort1();
#endif
#if TRACE1
  pcb_printf(" - %p line %#mD or %#mD\n", e, line->Point1.X, line->Point1.Y,
	 line->Point2.X, line->Point2.Y);
#endif
  if ((NEAR (line->Point1.X, fpcs->x) && NEAR (line->Point1.Y, fpcs->y))
      || (NEAR (line->Point2.X, fpcs->x) && NEAR (line->Point2.Y, fpcs->y)))
    {
      if (* fpcs->extra_ptr)
	{
#if TRACE1
	  printf("multiple, was %p\n", *fpcs->extra_ptr);
#endif
	  *fpcs->extra_ptr = & multi_next;
	}
      else
	{
	  *fpcs->extra_ptr = LINE2EXTRA (line);
#if TRACE1
	  printf(" - next now %p\n", *fpcs->extra_ptr);
#endif
	}
    }
  return 0;
}

static int
find_pair_arc_callback (const BoxType * b, void *cl)
{
  ArcType *arc = (ArcType *) b;
  Extra *e = ARC2EXTRA (arc);
  FindPairCallbackStruct *fpcs = (FindPairCallbackStruct *) cl;

  if (arc == fpcs->me)
    return 0;
#if TRACE1
  pcb_printf(" - %p arc %#mD or %#mD\n", e, e->start.x, e->start.y, e->end.x, e->end.y);
#endif
  if ((NEAR (e->start.x, fpcs->x) && NEAR (e->start.y, fpcs->y))
      || (NEAR (e->end.x, fpcs->x) && NEAR (e->end.y, fpcs->y)))
    {
      if (* fpcs->extra_ptr)
	{
#if TRACE1
	  printf("multiple, was %p\n", *fpcs->extra_ptr);
#endif
	  *fpcs->extra_ptr = & multi_next;
	}
      else
	*fpcs->extra_ptr = e;
    }
  return 0;
}

static void
find_pairs_1 (void *me, Extra **e, Coord x, Coord y)
{
  FindPairCallbackStruct fpcs;
  BoxType b;

  if (*e)
    return;

  fpcs.me = me;
  fpcs.extra_ptr = e;
  fpcs.x = x;
  fpcs.y = y;
#if TRACE1
  pcb_printf("looking for %#mD\n", x, y);
#endif
  b.X1 = x - 10;
  b.X2 = x + 10;
  b.Y1 = y - 10;
  b.Y2 = y + 10;
  r_search(CURRENT->line_tree, &b, NULL, find_pair_line_callback, &fpcs);
  r_search(CURRENT->arc_tree, &b, NULL, find_pair_arc_callback, &fpcs);
}

static int
check_point_in_pin (PinType *pin, Coord x, Coord y, End *e)
{
  int inside_p;
  Coord t = (PIN_SIZE(pin)+1)/2;
  if (TEST_FLAG (SQUAREFLAG, pin))
    inside_p = (x >= pin->X - t && x <= pin->X + t
		&& y >= pin->Y - t && y <= pin->Y + t);
  else
    inside_p = (Distance (pin->X, pin->Y, x, y) <= t);

  if (inside_p)
    {
      e->in_pin = 1;
      if (pin->X == x && pin->Y == y)
	e->at_pin = 1;
      e->pin = pin;
      return 1;
    }
  return 0;
}

static int
find_pair_pinline_callback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  PinType *pin = (PinType *) cl;
  Extra *e = LINE2EXTRA (line);
  int hits;

#ifdef CHECK_LINE_PT_NEG
  if (line->Point1.X < 0)
    abort1();
#endif

  hits = check_point_in_pin (pin, line->Point1.X, line->Point1.Y, &(e->start));
  hits += check_point_in_pin (pin, line->Point2.X, line->Point2.Y, &(e->end));

  if (hits)
    return 0;

  /* See if the line passes through this pin.  */
  /*! \todo This assumes round pads, but it's good enough for square
   * ones for now. */
  if (dist_lsp (line->Point1.X, line->Point1.Y,
		line->Point2.X, line->Point2.Y,
		pin->X, pin->Y) <= PIN_SIZE(pin)/2)
    {
#if TRACE1
      pcb_printf("splitting line %#mD-%#mD because it passes through pin %#mD r%d\n",
	     line->Point1.X, line->Point1.Y,
	     line->Point2.X, line->Point2.Y,
	      pin->X, pin->Y, PIN_SIZE(pin)/2);
#endif
      unlink_end (e, &e->start.next);
      unlink_end (e, &e->end.next);
    }
  return 0;
}

static int
find_pair_pinarc_callback (const BoxType * b, void *cl)
{
  ArcType *arc = (ArcType *) b;
  PinType *pin = (PinType *) cl;
  Extra *e = ARC2EXTRA (arc);
  int hits;

  hits = check_point_in_pin (pin, e->start.x, e->start.y, &(e->start));
  hits += check_point_in_pin (pin, e->end.x, e->end.y, &(e->end));
  return 0;
}

static int
check_point_in_pad (PadType *pad, Coord x, Coord y, End *e)
{
  int inside_p;
  Coord t;

  pcb_printf("pad %#mD - %#mD t %#mS  vs  %#mD\n", pad->Point1.X, pad->Point1.Y,
	 pad->Point2.X, pad->Point2.Y, pad->Thickness, x, y);
  t = (pad->Thickness+1)/2;
  if (TEST_FLAG (SQUAREFLAG, pad))
    {
    inside_p = (x >= MIN (pad->Point1.X - t, pad->Point2.X - t)
		&& x <= MAX (pad->Point1.X + t, pad->Point2.X + t)
		&& y >= MIN (pad->Point1.Y - t, pad->Point2.Y - t)
		&& y <= MAX (pad->Point1.Y + t, pad->Point2.Y + t));
    printf(" - inside_p = %d\n", inside_p);
    }
  else
    {
      if (pad->Point1.X == pad->Point2.X)
	{
	  inside_p = (x >= pad->Point1.X - t
		      && x <= pad->Point1.X + t
		      && y >= MIN (pad->Point1.Y, pad->Point2.Y)
		      && y <= MAX (pad->Point1.Y, pad->Point2.Y));
	}
      else
	{
	  inside_p = (x >= MIN (pad->Point1.X, pad->Point2.X)
		      && x <= MAX (pad->Point1.X, pad->Point2.X)
		      && y >= pad->Point1.Y - t
		      && y <= pad->Point1.Y + t);
	}
      if (!inside_p)
	{
	  if (Distance (pad->Point1.X, pad->Point1.Y, x, y) <= t
	      || Distance (pad->Point2.X, pad->Point2.Y, x, y) <= t)
	    inside_p = 1;
	}
    }

  if (inside_p)
    {
      e->in_pin = 1;
      if (pad->Point1.X == x && pad->Point1.Y == y)
	e->at_pin = 1;
      if (pad->Point2.X == x && pad->Point2.Y == y)
	e->at_pin = 1;
      e->pin = pad;
      e->is_pad = 1;
      return 1;
    }
  return 0;
}

static int
find_pair_padline_callback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  PadType *pad = (PadType *) cl;
  Extra *e = LINE2EXTRA (line);
  int hits;
  double t;
  int intersect;
  double p1_d, p2_d;

  if (TEST_FLAG (ONSOLDERFLAG, pad))
    {
      if (!current_is_bottom)
	return 0;
    }
  else
    {
      if (!current_is_top)
	return 0;
    }

#ifdef CHECK_LINE_PT_NEG
  if (line->Point1.X < 0)
    abort1();
#endif

  hits = check_point_in_pad (pad, line->Point1.X, line->Point1.Y, &(e->start));
  hits += check_point_in_pad (pad, line->Point2.X, line->Point2.Y, &(e->end));

  if (hits)
    return 0;

  /* Ok, something strange.  The line intersects our space, but
     doesn't end in our space.  See if it just passes through us, and
     mark it anyway.  */

  t = (pad->Thickness + 1)/2;
  /*! \todo This is for round pads. Good enough for now, but add
   * square pad support later. */
  intersect = intersection_of_linesegs (pad->Point1.X, pad->Point1.Y,
					 pad->Point2.X, pad->Point2.Y,
					 line->Point1.X, line->Point1.Y,
					 line->Point2.X, line->Point2.Y,
					 NULL, NULL);
  p1_d = dist_lsp(line->Point1.X, line->Point1.Y,
		  line->Point2.X, line->Point2.Y,
		  pad->Point1.X, pad->Point1.Y);
  p2_d = dist_lsp(line->Point1.X, line->Point1.Y,
		  line->Point2.X, line->Point2.Y,
		  pad->Point2.X, pad->Point2.Y);

  if (intersect || p1_d < t || p2_d < t)
    {
      /* It does.  */
      /*! \todo We should split the line.  */
#if TRACE1
      pcb_printf("splitting line %#mD-%#mD because it passes through pad %#mD-%#mD r %#mS\n",
	     line->Point1.X, line->Point1.Y,
	     line->Point2.X, line->Point2.Y,
	     pad->Point1.X, pad->Point1.Y,
	     pad->Point2.X, pad->Point2.Y,
	     pad->Thickness/2);
#endif
      unlink_end (e, &e->start.next);
      unlink_end (e, &e->end.next);
    }

  return 0;
}

static int
find_pair_padarc_callback (const BoxType * b, void *cl)
{
  ArcType *arc = (ArcType *) b;
  PadType *pad = (PadType *) cl;
  Extra *e = ARC2EXTRA (arc);
  int hits;

  if (TEST_FLAG (ONSOLDERFLAG, pad))
    {
      if (!current_is_bottom)
	return 0;
    }
  else
    {
      if (!current_is_top)
	return 0;
    }

  hits = check_point_in_pad (pad, e->start.x, e->start.y, &(e->start));
  hits += check_point_in_pad (pad, e->end.x, e->end.y, &(e->end));
  return 0;
}

static void
null_multi_next_ends (AnyObjectType *ptr, Extra *extra, void *userdata)
{
  if (extra->start.next == &multi_next)
    extra->start.next = NULL;

  if (extra->end.next == &multi_next)
    extra->end.next = NULL;
}

static Extra *
new_line_extra (LineType *line)
{
  Extra *extra = g_slice_new0 (Extra);
  g_hash_table_insert (lines, line, extra);
  extra->parent.line = line;
  extra->type = LINE_TYPE;
  return extra;
}

static Extra *
new_arc_extra (ArcType *arc)
{
  Extra *extra = g_slice_new0 (Extra);
  g_hash_table_insert (arcs, arc, extra);
  extra->parent.arc = arc;
  extra->type = ARC_TYPE;
  return extra;
}

static void
find_pairs ()
{
  ARC_LOOP (CURRENT); {
    Extra *e = new_arc_extra (arc);
    fix_arc_extra (arc, e);
  } END_LOOP;

  LINE_LOOP (CURRENT); {
    new_line_extra (line);
  } END_LOOP;

  LINE_LOOP (CURRENT); {
    Extra *e = LINE2EXTRA (line);
    if (line->Point1.X >= 0)
      {
	find_pairs_1 (line, & e->start.next, line->Point1.X, line->Point1.Y);
	find_pairs_1 (line, & e->end.next, line->Point2.X, line->Point2.Y);
      }
  } END_LOOP;

  ARC_LOOP (CURRENT); {
    Extra *e = ARC2EXTRA (arc);
    if (!e->deleted)
      {
	find_pairs_1 (arc, & e->start.next, e->start.x, e->start.y);
	find_pairs_1 (arc, & e->end.next, e->end.x, e->end.y);
      }
  } END_LOOP;

  ALLPIN_LOOP (PCB->Data); {
    BoxType box;
    box.X1 = pin->X - PIN_SIZE(pin)/2;
    box.Y1 = pin->Y - PIN_SIZE(pin)/2;
    box.X2 = pin->X + PIN_SIZE(pin)/2;
    box.Y2 = pin->Y + PIN_SIZE(pin)/2;
    r_search (CURRENT->line_tree, &box, NULL, find_pair_pinline_callback, pin);
    r_search (CURRENT->arc_tree, &box, NULL, find_pair_pinarc_callback, pin);
  } ENDALL_LOOP;

  VIA_LOOP (PCB->Data); {
    BoxType box;
    box.X1 = via->X - PIN_SIZE(via)/2;
    box.Y1 = via->Y - PIN_SIZE(via)/2;
    box.X2 = via->X + PIN_SIZE(via)/2;
    box.Y2 = via->Y + PIN_SIZE(via)/2;
    r_search (CURRENT->line_tree, &box, NULL, find_pair_pinline_callback, via);
    r_search (CURRENT->arc_tree, &box, NULL, find_pair_pinarc_callback, via);
  } END_LOOP;

  ALLPAD_LOOP (PCB->Data); {
    BoxType box;
    box.X1 = MIN(pad->Point1.X, pad->Point2.X) - pad->Thickness/2;
    box.Y1 = MIN(pad->Point1.Y, pad->Point2.Y) - pad->Thickness/2;
    box.X2 = MAX(pad->Point1.X, pad->Point2.X) + pad->Thickness/2;
    box.Y2 = MAX(pad->Point1.Y, pad->Point2.Y) + pad->Thickness/2;
    r_search (CURRENT->line_tree, &box, NULL, find_pair_padline_callback, pad);
    r_search (CURRENT->arc_tree, &box, NULL, find_pair_padarc_callback, pad);
    
  } ENDALL_LOOP;

  g_hash_table_foreach (lines, (GHFunc)null_multi_next_ends, NULL);
  g_hash_table_foreach (arcs, (GHFunc)null_multi_next_ends, NULL);
}

#define PROP_NEXT(e,n,f) 		\
      if (f->next->start.next == e) {	\
	e = f->next;			\
	n = & e->start;			\
	f = & e->end;			\
      } else {				\
	e = f->next;			\
	n = & e->end;			\
	f = & e->start; }

static void
propogate_ends_at (Extra *e, End *near, End *far)
{
  while (far->in_pin && far->pin == near->pin)
    {
      far->in_pin = 0;
      if (!far->next)
	return;
      PROP_NEXT (e, near, far);
      near->in_pin = 0;
    }
}

static void
propogate_end_pin (Extra *e, End *near, End *far)
{
  void *pinpad = near->pin;
  int ispad = near->is_pad;
  while (far->next)
    {
      PROP_NEXT (e, near, far);
      if (near->pin == pinpad)
	break;
      near->pin = pinpad;
      near->is_pad = ispad;
    }
}

static void
propogate_end_step1_cb (AnyObjectType *ptr, Extra *extra, void *userdata)
{
  if (extra->start.next != NULL &&
      extra->start.next == extra->end.next)
    {
      extra->end.next = NULL;
      mark_line_for_deletion ((LineType *)ptr);
    }

  if (extra->start.at_pin)
    propogate_ends_at (extra, &extra->start, &extra->end);

  if (extra->end.at_pin)
    propogate_ends_at (extra, &extra->end, &extra->start);
}

static void
propogate_end_step2_cb (AnyObjectType *ptr, Extra *extra, void *userdata)
{
  if (extra->start.in_pin)
    {
#if TRACE1
      printf("MULTI at %d: was %p\n", __LINE__, extra->start.next);
#endif
      extra->start.next = NULL;
    }
  if (extra->end.in_pin)
    {
#if TRACE1
      printf("MULTI at %d: was %p\n", __LINE__, extra->end.next);
#endif
      extra->end.next = NULL;
    }
}

static void
propogate_end_step3_cb (AnyObjectType *ptr, Extra *extra, void *userdata)
{
  if (extra->start.next)
    propogate_end_pin (extra, &extra->end, &extra->start);
  if (extra->end.next)
    propogate_end_pin (extra, &extra->start, &extra->end);
}

static void
propogate_ends ()
{
  /* First, shut of "in pin" when we have an "at pin".  We also clean
     up zero-length lines.  */
  g_hash_table_foreach (lines, (GHFunc)propogate_end_step1_cb, NULL);

  /* Now end all paths at pins/pads.  */
  g_hash_table_foreach (lines, (GHFunc)propogate_end_step2_cb, NULL);

  /* Now, propogate the pin/pad/vias along paths.  */
  g_hash_table_foreach (lines, (GHFunc)propogate_end_step3_cb, NULL);
}

static Extra *last_pextra = 0;

static void
print_extra (Extra *e, Extra *prev)
{
  int which = 0;
  if (e->start.next == last_pextra)
    which = 1;
  else if (e->end.next == last_pextra)
    which = 2;
  switch (which) {
  case 0:
    printf("%10p %10p %10p :", e, e->start.next, e->end.next);
    break;
  case 1:
    printf("%10p \033[33m%10p\033[0m %10p :", e, e->start.next, e->end.next);
    break;
  case 2:
    printf("%10p %10p \033[33m%10p\033[0m :", e, e->start.next, e->end.next);
    break;
  }
  last_pextra = e;
  printf(" %c%c",
	 e->deleted ? 'd' : '-',
	 e->found   ? 'f' : '-');
  printf(" s:%s%s%s%s",
	 e->start.in_pin ? "I" : "-",
	 e->start.at_pin ? "A" : "-",
	 e->start.is_pad ? "P" : "-",
	 e->start.pending ? "p" : "-");
  printf(" e:%s%s%s%s ",
	 e->end.in_pin ? "I" : "-",
	 e->end.at_pin ? "A" : "-",
	 e->end.is_pad ? "P" : "-",
	 e->end.pending ? "p" : "-");
	 
  if (EXTRA_IS_LINE (e))
    {
      LineType *line = EXTRA2LINE (e);
      pcb_printf(" %p L %#mD-%#mD", line, line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y);
      printf("  %s %p %s %p\n",
	     e->start.is_pad ? "pad" : "pin", e->start.pin,
	     e->end.is_pad ? "pad" : "pin", e->end.pin);
    }
  else if (EXTRA_IS_ARC (e))
    {
      ArcType *arc = EXTRA2ARC (e);
      pcb_printf(" %p A %#mD-%#mD", arc, e->start.x, e->start.y, e->end.x, e->end.y);
      pcb_printf(" at %#mD ang %ld,%ld\n", arc->X, arc->Y, arc->StartAngle, arc->Delta);
    }
  else if (e == &multi_next)
    {
      printf("-- Multi-next\n");
    }
  else
    {
      printf("-- Unknown extra: %p\n", e);
    }
}

#if TRACE1
static void
trace_path (Extra *e)
{
  Extra *prev = 0;
  if ((e->start.next && e->end.next)
      || (!e->start.next && !e->end.next))
    return;
  if (e->found)
    return;
  printf("- path -\n");
  last_pextra = 0;
  while (e)
    {
      e->found = 1;
      print_extra (e, prev);
      if (e->start.next == prev)
	{
	  prev = e;
	  e = e->end.next;
	}
      else
	{
	  prev = e;
	  e = e->start.next;
	}
    }
}

static void
trace_paths ()
{
  Extra *e;

  clear_found ();
  LINE_LOOP (CURRENT); {
    e = LINE2EXTRA (line);
    trace_path (e);
  } END_LOOP;
  ARC_LOOP (CURRENT); {
    e = ARC2EXTRA (arc);
    trace_path (e);
  } END_LOOP;
}
#endif

static void
reverse_line (LineType *line)
{
  Extra *e = LINE2EXTRA (line);
  Coord x, y;
  End etmp;

  x = line->Point1.X;
  y = line->Point1.Y;
#if 1
  MoveObject (LINEPOINT_TYPE, CURRENT, line, &(line->Point1),
	      line->Point2.X - line->Point1.X,
	      line->Point2.Y - line->Point1.Y);
  MoveObject (LINEPOINT_TYPE, CURRENT, line, &(line->Point2),
	      x - line->Point2.X,
	      y - line->Point2.Y);
#else
  /* In theory, we should be using the above so that undo works.  */
  line->Point1.X = line->Point2.X;
  line->Point1.Y = line->Point2.Y;
  line->Point2.X = x;
  line->Point2.Y = y;
#endif
  memcpy (&etmp, &e->start, sizeof (End));
  memcpy (&e->start, &e->end, sizeof (End));
  memcpy (&e->end, &etmp, sizeof (End));
}

static void
reverse_arc (ArcType *arc)
{
  Extra *e = ARC2EXTRA (arc);
  End etmp;

#if 1
  ChangeArcAngles (CURRENT, arc,
		   arc->StartAngle + arc->Delta, -arc->Delta);
#else
  /* Likewise, see above.  */
  arc->StartAngle += arc->Delta;
  arc->Delta *= -1;
#endif
  memcpy (&etmp, &e->start, sizeof (End));
  memcpy (&e->start, &e->end, sizeof (End));
  memcpy (&e->end, &etmp, sizeof (End));
}

static void
expand_box (BoxType *b, Coord x, Coord y, Coord t)
{
  b->X1 = MIN (b->X1, x-t);
  b->X2 = MAX (b->X2, x+t);
  b->Y1 = MIN (b->Y1, y-t);
  b->Y2 = MAX (b->Y2, y+t);
}

/* ---------------------------------------------------------------------- */
/* These are the state variables for the intersection we're currently
   working on. */

/* what we're working with */
static ArcType *start_arc;
static LineType *start_line;
static LineType *end_line;
static ArcType *end_arc;
static Extra *start_extra, *end_extra;
static Extra *sarc_extra, *earc_extra;
static void *start_pinpad, *end_pinpad;
static Coord thickness;

/* Pre-computed values.  Note that all values are computed according
   to CARTESIAN coordinates, not PCB coordinates.  Do an up-down board
   flip before wrapping your brain around the math.  */

/* se_sign is positive when you make a right turn going from start to end. */
/* sa_sign is positive when start's arc is on the same side of start as end.  */
/* ea_sign is positive when end's arc is on the same side of end as start.  */
/* sa_sign and ea_sign may be zero if there's no arc.  */
static double se_sign, sa_sign, ea_sign;

static double best_angle, start_angle, end_dist;
/* arc radii are positive when they're on the same side as the things
   we're interested in. */
static Coord sa_r, ea_r;
static Coord sa_x, sa_y; /* start "arc" point */

/* what we've found so far */
static Coord fx, fy, fr;
static int fp;
static End *fp_end;
static double fa; /* relative angle */

#define gp_point(x,y,t,e) gp_point_2(x,y,t,e,0,0,__FUNCTION__)

static int
gp_point_force (Coord x, Coord y, Coord t, End *e, int esa, int eda, int force, const char *name)
{
  double r, a, d;
  Coord scx, scy, sr;
  double base_angle, rel_angle, point_angle;

#if TRACE1
  pcb_printf("\033[34mgp_point_force %#mD %#mS via %s\033[0m\n", x, y, t, name);
#endif

  if (start_arc)
    {
      scx = start_arc->X;
      scy = start_arc->Y;
      sr = start_arc->Width;
    }
  else
    {
      scx = start_line->Point1.X;
      scy = start_line->Point1.Y;
      sr = 0;
    }

  r = t + thickness;

  /* See if the point is inside our start arc. */
  d = Distance (scx, scy, x, y);
#if TRACE1
  pcb_printf("%f = dist #mD to %#mD\n", d, scx, scy, x, y);
  pcb_printf("sr %#mS r %f d %f\n", sr, r, d);
#endif
  if (d  < sr - r)
    {
#if TRACE1
      printf("inside start arc, %f < %f\n", d, sr-r);
#endif
      return 0;
    }
  if (sr == 0 && d < r)
    {
#if TRACE1
      printf("start is inside arc, %f < %f\n", d, r);
#endif
      return 0;
    }

  /* Now for the same tricky math we needed for the single puller.
     sr and r are the radii for the two points scx,scy and x,y.  */

  /* angle between points (NOT pcb arc angles) */
  base_angle = atan2 (y - scy, x - scx);
#if TRACE1
  pcb_printf("%.1f = atan2 (%#mS-%#mS = %#mS, %#mS-%#mS = %#mS)\n",
	     r2d(base_angle), y, scy, y-scy, x, scx, x-scx);
#endif

  if ((sa_sign * sr - r) / d > 1
      || (sa_sign * sr - r) / d < -1)
    return 0;

  /* Angle of tangent, relative to the angle between point centers.  */
  rel_angle = se_sign * asin ((sa_sign * sr - r) / d);
#if TRACE1
  printf("%.1f = %d * asin ((%d * %d - %f) / %f)\n",
	 r2d(rel_angle), (int)se_sign, (int)sa_sign, sr, r, d);
#endif

  /* Absolute angle of tangent.  */
  point_angle = base_angle + rel_angle;
#if TRACE1
  printf("base angle %.1f rel_angle %.1f point_angle %.1f\n",
	 r2d(base_angle),
	 r2d(rel_angle),
	 r2d(point_angle));
#endif

  if (eda)
    {
      /* Check arc angles */
      double pa = point_angle;
      double sa = d2r(180-esa);
      double da = d2r(-eda);

      if (da < 0)
	{
	  sa = sa + da;
	  da = -da;
	}

      pa -= se_sign * M_PI/2;
      while (sa+da < pa)
	sa += M_PI*2;
      while (sa > pa)
	sa -= M_PI*2;
      if (sa+da < pa)
	{
#if TRACE1
	  printf("arc doesn't apply: sa %.1f da %.1f pa %.1f\n",
		 r2d(sa), r2d(da), r2d(pa));
#endif
	  return 0;
	}
    }

  a = point_angle - start_angle;
  while (a > M_PI)
    a -= M_PI*2;
  while (a < -M_PI)
    a += M_PI*2;
#if TRACE1
  printf(" - angle relative to S-E baseline is %.1f\n", r2d(a));
#endif

  if (!force && a * se_sign < -0.007)
    {
      double new_r;
#if TRACE1
      printf("skipping, would increase angle (%f * %f)\n", a, se_sign);
#endif
      new_r = dist_lp (start_line->Point1.X, start_line->Point1.Y,
		       start_line->Point2.X, start_line->Point2.Y,
		       x, y);
#if TRACE1
      pcb_printf("point %#mD dist %#mS vs thickness %#mS\n", x, y, new_r, thickness);
#endif
      new_r -= thickness;
      new_r = (int)new_r - 1;
#if TRACE1
      pcb_printf(" - new thickness %f old %#mS\n", new_r, t);
#endif
      if (new_r < t)
	gp_point_force (x, y, new_r, e, esa, eda, 1, __FUNCTION__);
      return 0;
    }

#if TRACE1
  printf("%f * %f < %f * %f ?\n", a, se_sign, best_angle, se_sign);
#endif
  if (a * se_sign == best_angle * se_sign)
    {
      double old_d = Distance (start_line->Point1.X, start_line->Point1.Y,
			   fx, fy);
      double new_d = Distance (start_line->Point1.X, start_line->Point1.Y,
			   x, y);
      if (new_d > old_d)
	{
	  best_angle = a;
	  fx = x;
	  fy = y;
	  fr = r;
	  fa = a;
	  fp = e ? e->pending : 0;
	  fp_end = e;
	}
    }
  else if (a * se_sign < best_angle * se_sign)
    {
      best_angle = a;
      fx = x;
      fy = y;
      fr = r;
      fa = a;
      fp = e ? e->pending : 0;
      fp_end = e;
    }

  return 1;
}
static int
gp_point_2 (Coord x, Coord y, Coord t, End *e, int esa, int eda, const char *func)
{
  double sc, ec;
  double sd, ed;

  if (x == sa_x && y ==sa_y)
    return 0;

#if TRACE1
  pcb_printf("\033[34mgp_point %#mD %#mS via %s\033[0m\n", x, y, t, func);
#endif

  /* There are two regions we care about.  For points inside our
     triangle, we check the crosses against start_line and end_line to
     make sure the point is "inside" the triangle.  For points on the
     other side of the s-e line of the triangle, we check the dots to
     make sure it's between our endpoints.  */

  /* See what side of the s-e line we're on */
  sc = cross2d (start_line->Point1.X, start_line->Point1.Y,
		end_line->Point2.X, end_line->Point2.Y,
		x, y);
#if TRACE1
  printf("s-e cross = %f\n", sc);
#endif
  if (t >= 0)
    {
      if (same_sign (sc, se_sign))
	{
	  /* Outside, check dots.  */

	  /* Ok, is it "in front of" our vectors? */
	  sd = dot2d (start_line->Point1.X, start_line->Point1.Y,
		      end_line->Point2.X, end_line->Point2.Y,
		      x, y);
#if TRACE1
	  printf("sd = %f\n", sd);
#endif
	  if (sd <= 0)
	    return 0;

	  ed = dot2d (end_line->Point2.X, end_line->Point2.Y,
		      start_line->Point1.X, start_line->Point1.Y,
		      x, y);
#if TRACE1
	  printf("ed = %f\n", ed);
#endif
	  if (ed <= 0)
	    return 0;

	  sd = dist_lp (start_line->Point1.X, start_line->Point1.Y,
			end_line->Point2.X, end_line->Point2.Y,
			x, y);
	  if (sd > t + thickness)
	    return 0;
	}
      else
	{
	  /* Inside, check crosses.  */

	  /* First off, is it on the correct side of the start line? */
	  sc = cross2d (start_line->Point1.X, start_line->Point1.Y,
			start_line->Point2.X, start_line->Point2.Y,
			x, y);
#if TRACE1
	  printf("sc = %f\n", sc);
#endif
	  if (! same_sign (sc, se_sign))
	    return 0;

	  /* Ok, is it on the correct side of the end line? */
	  ec = cross2d (end_line->Point1.X, end_line->Point1.Y,
			end_line->Point2.X, end_line->Point2.Y,
			x, y);
#if TRACE1
	  printf("ec = %f\n", ec);
#endif
	  if (! same_sign (ec, se_sign))
	    return 0;
	}
    }

#if TRACE1
  printf("in range!\n");
#endif

  return gp_point_force (x, y, t, e, esa, eda, 0, func);
}

static int
gp_line_cb (const BoxType *b, void *cb)
{
  const LineType *l = (LineType *) b;
  Extra *e = LINE2EXTRA(l);
  if (l == start_line || l == end_line)
    return 0;
  if (e->deleted)
    return 0;
#ifdef CHECK_LINE_PT_NEG
  if (l->Point1.X < 0)
    abort1();
#endif
  if (! e->start.next
      || ! EXTRA_IS_ARC (e->start.next))
    gp_point (l->Point1.X, l->Point1.Y, l->Thickness/2, &e->start);
  if (! e->end.next
      || ! EXTRA_IS_ARC (e->end.next))
    gp_point (l->Point2.X, l->Point2.Y, l->Thickness/2, &e->end);
  return 0;
}

static int
gp_arc_cb (const BoxType *b, void *cb)
{
  const ArcType *a = (ArcType *) b;
  Extra *e = ARC2EXTRA(a);
  if (a == start_arc || a == end_arc)
    return 0;
  if (e->deleted)
    return 0;
  gp_point_2 (a->X, a->Y, a->Width + a->Thickness/2, 0, a->StartAngle, a->Delta, __FUNCTION__);
  if (start_arc
      && a->X == start_arc->X
      && a->Y == start_arc->Y)
    return 0;
  if (end_arc
      && a->X != end_arc->X
      && a->Y != end_arc->Y)
     return 0;

  if (e->start.next || e->end.next)
    return 0;
    
  gp_point (e->start.x, e->start.y, a->Thickness/2, 0);
  gp_point (e->end.x, e->end.y, a->Thickness/2, 0);
  return 0;
}

static int
gp_text_cb (const BoxType *b, void *cb)
{
  const TextType *t = (TextType *) b;
  /* FIXME: drop in the actual text-line endpoints later. */
  gp_point (t->BoundingBox.X1, t->BoundingBox.Y1, 0, 0);
  gp_point (t->BoundingBox.X1, t->BoundingBox.Y2, 0, 0);
  gp_point (t->BoundingBox.X2, t->BoundingBox.Y2, 0, 0);
  gp_point (t->BoundingBox.X2, t->BoundingBox.Y1, 0, 0);
  return 0;
}

static int
gp_poly_cb (const BoxType *b, void *cb)
{
  int i;
  const PolygonType *p = (PolygonType *) b;
  for (i=0; i<p->PointN; i++)
    gp_point (p->Points[i].X, p->Points[i].Y, 0, 0);
  return 0;
}

static int
gp_pin_cb (const BoxType *b, void *cb)
{
  const PinType *p = (PinType *) b;
  Coord t2 = (PIN_SIZE(p)+1)/2;

  if (p == start_pinpad || p == end_pinpad)
    return 0;

  /*! \todo We lump octagonal pins in with square; safe, but not
   * optimal. */
  if (TEST_FLAG (SQUAREFLAG, p) || TEST_FLAG (OCTAGONFLAG, p))
    {
      gp_point (p->X - t2, p->Y - t2, 0, 0);
      gp_point (p->X - t2, p->Y + t2, 0, 0);
      gp_point (p->X + t2, p->Y + t2, 0, 0);
      gp_point (p->X + t2, p->Y - t2, 0, 0);
    }
  else
    {
      gp_point (p->X, p->Y, t2, 0);
    }
  return 0;
}

static int
gp_pad_cb (const BoxType *b, void *cb)
{
  const PadType *p = (PadType *) b;
  Coord t2 = (p->Thickness+1)/2;

  if (p == start_pinpad || p == end_pinpad)
    return 0;

  if (TEST_FLAG (ONSOLDERFLAG, p))
    {
      if (!current_is_bottom)
	return 0;
    }
  else
    {
      if (!current_is_top)
	return 0;
    }

  /*! \todo We lump octagonal pads in with square; safe, but not
     optimal. I don't think we even support octagonal pads. */
  if (TEST_FLAG (SQUAREFLAG, p) || TEST_FLAG (OCTAGONFLAG, p))
    {
      if (p->Point1.X == p->Point2.X)
	{
	  Coord y1 = MIN (p->Point1.Y, p->Point2.Y) - t2;
	  Coord y2 = MAX (p->Point1.Y, p->Point2.Y) + t2;

	  gp_point (p->Point1.X - t2, y1, 0, 0);
	  gp_point (p->Point1.X - t2, y2, 0, 0);
	  gp_point (p->Point1.X + t2, y1, 0, 0);
	  gp_point (p->Point1.X + t2, y2, 0, 0);
	}
      else
	{
	  Coord x1 = MIN (p->Point1.X, p->Point2.X) - t2;
	  Coord x2 = MAX (p->Point1.X, p->Point2.X) + t2;

	  gp_point (x1, p->Point1.Y - t2, 0, 0);
	  gp_point (x2, p->Point1.Y - t2, 0, 0);
	  gp_point (x1, p->Point1.Y + t2, 0, 0);
	  gp_point (x2, p->Point1.Y + t2, 0, 0);
	}
    }
  else
    {
      gp_point (p->Point1.X, p->Point1.Y, t2, 0);
      gp_point (p->Point2.X, p->Point2.Y, t2, 0);
    }
  return 0;
}

static LineType *
create_line (LineType *sample, Coord x1, Coord y1, Coord x2, Coord y2)
{
#if TRACE1
  Extra *e;
  pcb_printf("create_line from %#mD to %#mD\n", x1, y1, x2, y2);
#endif
  LineType *line = CreateNewLineOnLayer (CURRENT, x1, y1, x2, y2,
					   sample->Thickness, sample->Clearance, sample->Flags);
  AddObjectToCreateUndoList (LINE_TYPE, CURRENT, line, line);

#if TRACE1
  e =
#endif
    new_line_extra (line);
#if TRACE1
  printf(" - line extra is %p\n", e);
#endif
  return line;
}

static ArcType *
create_arc (LineType *sample, Coord x, Coord y, Coord r, Coord sa, Coord da)
{
  Extra *e;
  ArcType *arc;

  if (r % 100 == 1)
    r--;
  if (r % 100 == 99)
    r++;
#if TRACE1
  pcb_printf("create_arc at %#mD r %#mS sa %d delta %d\n", x, y, r, sa, da);
#endif
  arc = CreateNewArcOnLayer (CURRENT, x, y, r, r, sa, da,
					sample->Thickness, sample->Clearance, sample->Flags);
  if (arc == 0)
    {
      arc = CreateNewArcOnLayer (CURRENT, x, y, r, r, sa, da*2,
				 sample->Thickness, sample->Clearance, sample->Flags);
    }
  AddObjectToCreateUndoList (ARC_TYPE, CURRENT, arc, arc);

  if (!arc)
    longjmp (abort_buf, 1);

  e = new_arc_extra (arc);
#if TRACE1
  printf(" - arc extra is %p\n", e);
#endif
  fix_arc_extra (arc, e);
  return arc;
}

static void
unlink_extras (Extra *e)
{
#if TRACE1
  fprintf(stderr, "unlink %p\n", e);
  print_extra(e,0);
#endif
  if (e->start.next)
    {
#if TRACE1
      print_extra(e->start.next, 0);
#endif
      if (e->start.next->start.next == e)
	{
#if TRACE1
	  fprintf(stderr, " - %p->start points to me\n", e->start.next);
#endif
	  e->start.next->start.next = e->end.next;
	}
      else if (e->start.next->end.next == e)
	{
#if TRACE1
	  fprintf(stderr, " - %p->end points to me\n", e->start.next);
#endif
	  e->start.next->end.next = e->end.next;
	}
      else
	{
	  fprintf(stderr, " - %p doesn't point to me!\n", e->start.next);
	  abort();
	}
    }
  if (e->end.next)
    {
#if TRACE1
      print_extra(e->end.next, 0);
#endif
      if (e->end.next->start.next == e)
	{
#if TRACE1
	  fprintf(stderr, " - %p->end points to me\n", e->end.next);
#endif
	  e->end.next->start.next = e->start.next;
	}
      else if (e->end.next->end.next == e)
	{
#if TRACE1
	  fprintf(stderr, " - %p->end points to me\n", e->end.next);
#endif
	  e->end.next->end.next = e->start.next;
	}
      else
	{
	  fprintf(stderr, " - %p doesn't point to me!\n", e->end.next);
	  abort();
	}
    }
  e->start.next = e->end.next = 0;
}

static void
mark_line_for_deletion (LineType *l)
{
  Extra *e = LINE2EXTRA(l);
  if (e->deleted)
    {
      fprintf(stderr, "double delete?\n");
      abort();
    }
  e->deleted = 1;
  unlink_extras (e);
#if TRACE1
  pcb_printf("Marked line %p for deletion %#mD to %#mD\n",
	 e, l->Point1.X, l->Point1.Y, l->Point2.X, l->Point2.Y);
#endif
#if 0
  if (l->Point1.X < 0)
    {
      fprintf(stderr, "double neg move?\n");
      abort();
    }
  MoveObject (LINEPOINT_TYPE, CURRENT, l, &(l->Point1),
	      -1 - l->Point1.X,
	      -1 - l->Point1.Y);
  MoveObject (LINEPOINT_TYPE, CURRENT, l, &(l->Point2),
	      -1 - l->Point2.X,
	      -1 - l->Point2.Y);
#endif
}

static void
mark_arc_for_deletion (ArcType *a)
{
  Extra *e = ARC2EXTRA(a);
  e->deleted = 1;
  unlink_extras (e);
#if TRACE1
  printf("Marked arc %p for deletion %ld < %ld\n",
	 e, a->StartAngle, a->Delta);
#endif
}

/*!
 * \brief .
 *
 * Given a starting line, which may be attached to an arc, and which
 * intersects with an ending line, which also may be attached to an
 * arc, maybe pull them.\n
 * We assume start_line is attached to the arc via Point1, and attached
 * to the end line via Point2.\n
 * Likewise, we make end_line attach to the start_line via Point1 and
 * the arc via Point 2.\n
 * We also make the arcs attach on the Delta end, not the Start end.\n
 * Here's a picture:
 * <pre>
     S            S+D  P1            P2   P1          P2  S+D          S
   *--- start_arc ---*--- start_line ---*--- end_line ---*--- end_arc ---*
     S             E   S              E   S            E   E           S
 * </pre>
 */
static void
maybe_pull_1 (LineType *line)
{
  BoxType box;
  /* Line half-thicknesses, including line space */
  Coord ex, ey;
  LineType *new_line;
  Extra *new_lextra;
  ArcType *new_arc;
  Extra *new_aextra;
  double abs_angle;

  start_line = line;
  start_extra = LINE2EXTRA (start_line);
  end_extra = start_extra->end.next;
  end_line = EXTRA2LINE (end_extra);
  if (end_extra->deleted)
    {
      start_extra->end.pending = 0;
      return;
    }

  if (end_extra->end.next == start_extra)
    reverse_line (end_line);

  if (start_extra->start.next
      && EXTRA_IS_ARC (start_extra->start.next))
    {
      sarc_extra = start_extra->start.next;
      start_arc = EXTRA2ARC (sarc_extra);
      if (sarc_extra->start.next == start_extra)
	reverse_arc (start_arc);
    }
  else
    {
      start_arc = 0;
      sarc_extra = 0;
    }

  if (end_extra->end.next
      && EXTRA_IS_ARC (end_extra->end.next))
    {
      earc_extra = end_extra->end.next;
      end_arc = EXTRA2ARC (earc_extra);
      if (earc_extra->start.next == end_extra)
	reverse_arc (end_arc);
    }
  else
    {
      end_arc = 0;
      earc_extra = 0;
    }

#if TRACE1
  printf("maybe_pull_1 %p %p %p %p\n", sarc_extra, start_extra, end_extra, earc_extra);
  if (sarc_extra)
    print_extra(sarc_extra,0);
  print_extra(start_extra,0);
  print_extra(end_extra,0);
  if (earc_extra)
    print_extra(earc_extra,0);

  if (start_extra->deleted
      || end_extra->deleted
      || (sarc_extra && sarc_extra->deleted)
      || (earc_extra && earc_extra->deleted))
    {
      printf(" one is deleted?\n");
      fflush(stdout);
      abort();
    }
#endif

  if (!start_extra->end.pending)
    return;
#if 0
  if (start_extra->end.waiting_for
      && start_extra->end.waiting_for->pending)
    return;
#endif
  
  if (start_line->Thickness != end_line->Thickness)
    return;
  thickness = (start_line->Thickness + 1)/2 + PCB->Bloat;

  /* At this point, our expectations are all met.  */

  box.X1 = start_line->Point1.X - thickness;
  box.X2 = start_line->Point1.X + thickness;
  box.Y1 = start_line->Point1.Y - thickness;
  box.Y2 = start_line->Point1.Y + thickness;
  expand_box (&box, start_line->Point2.X, start_line->Point2.Y, thickness);
  expand_box (&box, end_line->Point2.X, end_line->Point2.Y, thickness);
  if (start_arc)
    expand_box (&box, sarc_extra->start.x, sarc_extra->start.y, start_arc->Thickness/2);
  if (end_arc)
    expand_box (&box, earc_extra->start.x, earc_extra->start.y, end_arc->Thickness/2);


  se_sign = copysign (1, cross2d (start_line->Point1.X, start_line->Point1.Y,
				  start_line->Point2.X, start_line->Point2.Y,
				  end_line->Point2.X, end_line->Point2.Y));
  best_angle = se_sign * M_PI;
  if (start_arc)
    {
      sa_sign = copysign (1, -start_arc->Delta);
      sa_sign *= se_sign;
    }
  else
    sa_sign = 0;
  if (end_arc)
    {
      ea_sign = copysign (1, -end_arc->Delta);
      ea_sign *= -se_sign;
    }
  else
    ea_sign = 0;

  start_angle = atan2 (start_line->Point2.Y - start_line->Point1.Y,
		       start_line->Point2.X - start_line->Point1.X);
#if TRACE1
  printf("se_sign %f sa_sign %f ea_sign %f best_angle %f start_angle %f\n",
	 se_sign, sa_sign, ea_sign, r2d (best_angle), r2d(start_angle));
#endif

  if (start_arc)
    {
      sa_x = start_arc->X;
      sa_y = start_arc->Y;
      if (same_sign (start_arc->Delta, se_sign))
	sa_r = - start_arc->Width;
      else
	sa_r = start_arc->Width;
    }
  else
    {
      sa_x = start_line->Point1.X;
      sa_y = start_line->Point1.Y;
      sa_r = 0;
    }

  if (end_arc)
    {
      if (ea_sign < 0)
	ea_r = end_arc->Width;
      else
	ea_r = - end_arc->Width;
    }
  else
    ea_r = 0;

#if TRACE1
  trace_path (sarc_extra ? sarc_extra : start_extra);
#endif

  if (end_arc)
    {
      gp_point_force (end_arc->X, end_arc->Y, -ea_r-thickness, 0, 0, 0, 1, "end arc");
      ex = end_arc->X;
      ey = end_arc->Y;
    }
  else
    {
      gp_point_force (end_line->Point2.X, end_line->Point2.Y, -thickness, 0, 0, 0, 1, "end arc");
      ex = end_line->Point2.X;
      ey = end_line->Point2.Y;
    }

  fx = ex;
  fy = ey;
  if (fx < 0)
    {
      pcb_fprintf(stderr, "end line corrupt? f is %#mD\n", fx, fy);
      print_extra (end_extra, 0);
      if (earc_extra)
	print_extra(earc_extra, 0);
      abort();
    }

  end_dist = Distance (end_line->Point1.X, end_line->Point1.Y,
		   end_line->Point2.X, end_line->Point2.Y);

  start_pinpad = start_extra->start.pin;
  end_pinpad = start_extra->end.pin;
  fp = 0;

  r_search(CURRENT->line_tree, &box, NULL, gp_line_cb, 0);
  r_search(CURRENT->arc_tree, &box, NULL, gp_arc_cb, 0);
  r_search(CURRENT->text_tree, &box, NULL, gp_text_cb, 0);
  r_search(CURRENT->polygon_tree, &box, NULL, gp_poly_cb, 0);
  r_search(PCB->Data->pin_tree, &box, NULL, gp_pin_cb, 0);
  r_search(PCB->Data->via_tree, &box, NULL, gp_pin_cb, 0);
  r_search(PCB->Data->pad_tree, &box, NULL, gp_pad_cb, 0);

  /* radians, absolute angle of (at the moment) the start_line */
  abs_angle = fa + start_angle;

#if TRACE1
  pcb_printf("\033[43;30mBest: at %#mD r %#mS, angle %.1f fp %d\033[0m\n", fx, fy, fr, r2d(fa), fp);
#endif

#if 0
  if (fa > M_PI/2 || fa < -M_PI/2)
    {
      SET_FLAG (FOUNDFLAG, line);
      longjmp (abort_buf, 1);
    }
#endif

  if (fp)
    {
      start_extra->end.waiting_for = fp_end;
      return;
    }
  start_extra->end.pending = 0;

  /* Step 0: check for merged arcs (special case).  */

  if (fx == ex && fy == ey
      && start_arc && end_arc
      && start_arc->X == end_arc->X && start_arc->Y == end_arc->Y)
    {
      /* Merge arcs */
      int new_delta;

      new_delta = end_arc->StartAngle - start_arc->StartAngle;
      if (start_arc->Delta > 0)
	{
	  while (new_delta > 360)
	    new_delta -= 360;
	  while (new_delta < 0)
	    new_delta += 360;
	}
      else
	{
	  while (new_delta < -360)
	    new_delta += 360;
	  while (new_delta > 0)
	    new_delta -= 360;
	}
#if TRACE1
      pcb_printf("merging arcs at %#mS nd %d\n", start_arc->X, start_arc->Y, new_delta);
      print_extra(sarc_extra, 0);
      print_extra(earc_extra, 0);
#endif
      mark_arc_for_deletion (end_arc);
      mark_line_for_deletion (start_line);
      mark_line_for_deletion (end_line);
      ChangeArcAngles (CURRENT, start_arc, start_arc->StartAngle, new_delta);
      fix_arc_extra (start_arc, sarc_extra);
      did_something ++;
      return;
    }

  /* Step 1: adjust start_arc's angles and move start_line's Point1 to
     match it. */

  if (start_arc)
    {
      double new_delta;
      int del_arc = 0;

      /* We must always round towards "larger arcs", whichever way that is. */
      new_delta = 180 - r2d(abs_angle);
#if TRACE1
      printf("new_delta starts at %d vs %d < %d\n",
	     (int)new_delta, (int)start_arc->StartAngle, (int)start_arc->Delta);
#endif
      if (start_arc->Delta < 0)
	new_delta = (int)(new_delta) + 90;
      else
	new_delta = (int)new_delta - 90;
      new_delta =  new_delta - start_arc->StartAngle;
      while (new_delta > start_arc->Delta+180)
	new_delta -= 360;
      while (new_delta < start_arc->Delta-180)
	new_delta += 360;
#if TRACE1
      printf("new_delta adjusts to %d\n", (int)new_delta);
      printf("fa = %f, new_delta ends at %.1f vs start %d\n",
	     fa, new_delta, (int)start_arc->StartAngle);
#endif

      if (new_delta * start_arc->Delta <= 0)
	del_arc = 1;

      ChangeArcAngles (CURRENT, start_arc, start_arc->StartAngle, new_delta);
      fix_arc_extra (start_arc, sarc_extra);
      MoveObject (LINEPOINT_TYPE, CURRENT, start_line, &(start_line->Point1),
		  sarc_extra->end.x - start_line->Point1.X,
		  sarc_extra->end.y - start_line->Point1.Y);

      if (del_arc)
	{
	  MoveObject (LINEPOINT_TYPE, CURRENT, start_line, &(start_line->Point1),
		      sarc_extra->start.x - start_line->Point1.X,
		      sarc_extra->start.y - start_line->Point1.Y);
	  mark_arc_for_deletion (start_arc);
	}
    }

  /* Step 1.5: If the "obstacle" is right at the end, ignore it.  */

  {
    double oa = start_angle+fa - M_PI/2*se_sign;
    double ox = fx + fr * cos(oa);
    double oy = fy + fr * sin(oa);
#if TRACE1
    pcb_printf("obstacle at %#mD angle %d = arc starts at %#mD\n",
	   fx, fy, (int)r2d(oa), (Coord)ox, (Coord)oy);
#endif

    if (Distance (ox, oy, end_line->Point2.X, end_line->Point2.Y)
	< fr * SIN1D)
      {
	/* Pretend it doesn't exist.  */
	fx = ex;
	fy = ey;
      }
  }

  /* Step 2: If we have no obstacles, connect start and end.  */

#if TRACE1
  pcb_printf("fx %#mS ex %#mS fy %#mS ey %#mS\n", fx, ex, fy, ey);
#endif
  if (fx == ex && fy == ey)
    {
      /* No obstacles.  */
#if TRACE1
      printf("\033[32mno obstacles\033[0m\n");
#endif
      if (end_arc)
	{
	  int del_arc = 0;
	  int new_delta, end_angle, pcb_fa, end_change;

	  end_angle = end_arc->StartAngle + end_arc->Delta;
	  if (end_arc->Delta < 0)
	    end_angle -= 90;
	  else
	    end_angle += 90;

	  /* We must round so as to make the larger arc.  */
	  if (end_arc->Delta < 0)
	    pcb_fa = - r2d(start_angle + fa);
	  else
	    pcb_fa = 1 - r2d(start_angle + fa);
	  end_change = pcb_fa - end_angle;

	  while (end_change > 180)
	    end_change -= 360;
	  while (end_change < -180)
	    end_change += 360;
	  new_delta = end_arc->Delta + end_change;

	  if (new_delta * end_arc->Delta <= 0)
	    del_arc = 1;

	  ChangeArcAngles (CURRENT, end_arc, end_arc->StartAngle, new_delta);
	  fix_arc_extra (end_arc, earc_extra);
	  MoveObject (LINEPOINT_TYPE, CURRENT, start_line, &(start_line->Point2),
		      earc_extra->end.x - start_line->Point2.X,
		      earc_extra->end.y - start_line->Point2.Y);

	  if (del_arc)
	    {
	      MoveObject (LINEPOINT_TYPE, CURRENT, start_line, &(start_line->Point2),
			  earc_extra->start.x - start_line->Point2.X,
			  earc_extra->start.y - start_line->Point2.Y);
	      mark_arc_for_deletion (end_arc);
	    }
	}
      else
	{
	  MoveObject (LINEPOINT_TYPE, CURRENT, start_line, &(start_line->Point2),
		      end_line->Point2.X - start_line->Point2.X,
		      end_line->Point2.Y - start_line->Point2.Y);
	}
      mark_line_for_deletion (end_line);
      start_extra->end.pending = 1;

#if TRACE1
      printf("\033[35mdid_something: no obstacles\033[0m\n");
#endif
      did_something ++;
      npulled ++;
      status();
      return;
    }

  /* Step 3: Compute the new intersection of start_line and end_line.  */

  ex = start_line->Point1.X + cos(start_angle + fa) * 10000.0;
  ey = start_line->Point1.Y + sin(start_angle + fa) * 10000.0;
#if TRACE1
  pcb_printf("temp point %#mS\n", ex, ey);
  pcb_printf("intersect %#mS-%#mS with %#mS-%#mS\n",
	 start_line->Point1.X, start_line->Point1.Y,
	 ex, ey,
	 end_line->Point1.X, end_line->Point1.Y,
	 end_line->Point2.X, end_line->Point2.Y);
#endif
  if (! intersection_of_lines (start_line->Point1.X, start_line->Point1.Y,
			       ex, ey,
			       end_line->Point1.X, end_line->Point1.Y,
			       end_line->Point2.X, end_line->Point2.Y,
			       &ex, &ey))
    {
      ex = end_line->Point2.X;
      ey = end_line->Point2.Y;
    }	
#if TRACE1
  pcb_printf("new point %#mS\n", ex, ey);
#endif
  MoveObject (LINEPOINT_TYPE, CURRENT, end_line, &(end_line->Point1),
	      ex - end_line->Point1.X,
	      ey - end_line->Point1.Y);

  /* Step 4: Split start_line at the obstacle and insert a zero-delta
     arc at it.  */

  new_arc = create_arc (start_line, fx, fy, fr,
			90-(int)(r2d(start_angle+fa)+0.5) + 90 + 90*se_sign, -se_sign);
  new_aextra = ARC2EXTRA (new_arc);

  if (start_arc) sarc_extra = ARC2EXTRA (start_arc);
  if (end_arc) earc_extra = ARC2EXTRA (end_arc);

  MoveObject (LINEPOINT_TYPE, CURRENT, start_line, &(start_line->Point2),
	      new_aextra->start.x - start_line->Point2.X,
	      new_aextra->start.y - start_line->Point2.Y);

  new_line = create_line (start_line, new_aextra->end.x, new_aextra->end.y, ex, ey);
  start_extra = LINE2EXTRA (start_line);
  new_lextra = LINE2EXTRA (new_line);
  end_extra = LINE2EXTRA (end_line);

  new_lextra->start.pin = start_extra->start.pin;
  new_lextra->end.pin = start_extra->end.pin;
  new_lextra->start.pending = 1;
  new_lextra->end.pending = 1;

  start_extra->end.next = new_aextra;
  new_aextra->start.next = start_extra;
  new_aextra->end.next = new_lextra;
  new_lextra->start.next = new_aextra;
  new_lextra->end.next = end_extra;
  end_extra->start.next = new_lextra;

  /* Step 5: Recurse.  */

  did_something ++;
  npulled ++;
  status();
#if TRACE0
  printf("\033[35mdid_something: recursing\033[0m\n");
  {
    int i = gui->confirm_dialog("recurse?", 0);
    printf("confirm = %d\n", i);
    if (i == 0)
      return;
  }
  printf("\n\033[33mRECURSING\033[0m\n\n");
  IncrementUndoSerialNumber();
#endif
  maybe_pull_1 (new_line);
}

/*!
 * \brief Given a line with a end_next, attempt to pull both ends.
 */
static void
maybe_pull (LineType *line, Extra *e)
{
#if TRACE0
  printf("maybe_pull: ");
  print_extra (e, 0);
#endif
  if (e->end.next && EXTRA_IS_LINE (e->end.next))
    {
      maybe_pull_1 (line);
    }
  else
    {
      e->end.pending = 0;
      if (e->start.next && EXTRA_IS_LINE (e->start.next))
	{
	  reverse_line (line);
	  maybe_pull_1 (line);
	}
      else
	e->start.pending = 0;
    }
}

static void
validate_pair (Extra *e, End *end)
{
  if (!end->next)
    return;
  if (end->next->start.next == e)
    return;
  if (end->next->end.next == e)
    return;
  fprintf(stderr, "no backlink!\n");
  print_extra (e, 0);
  print_extra (end->next, 0);
  abort();
}

static void
validate_pair_cb (AnyObjectType *ptr, Extra *extra, void *userdata)
{
  validate_pair (extra, &extra->start);
  validate_pair (extra, &extra->end);
}

static void
validate_pairs ()
{
  g_hash_table_foreach (lines, (GHFunc)validate_pair_cb, NULL);
#if TRACE1
  printf("\narcs\n");
#endif
  g_hash_table_foreach (arcs, (GHFunc)validate_pair_cb, NULL);
}

static void
FreeExtra (Extra *extra)
{
  g_slice_free (Extra, extra);
}

static void
mark_ends_pending (LineType *line, Extra *extra, void *userdata)
{
  int *select_flags = userdata;
  if (TEST_FLAGS (*select_flags, line))
    {
      extra->start.pending = 1;
      extra->end.pending = 1;
    }
}

#if TRACE1
static void
trace_print_extra (AnyObjectType *ptr, Extra *extra, void *userdata)
{
    last_pextra = (Extra *)1;
    print_extra(extra, 0);
}

static void
trace_print_lines_arcs (void)
{
  printf("\nlines\n");
  g_hash_table_foreach (lines, (GHFunc)trace_print_extra, NULL);

  printf("\narcs\n");
  g_hash_table_foreach (arcs, (GHFunc)trace_print_extra, NULL);

  printf("\n");
}
#endif

static int
GlobalPuller(int argc, char **argv, Coord x, Coord y)
{
  int select_flags = 0;

  setbuf(stdout, 0);
  nloops = 0;
  npulled = 0;
  Message ("puller! %s\n", argc > 0 ? argv[0] : "");

  if (argc > 0 && strcasecmp (argv[0], "selected") == 0)
    select_flags = SELECTEDFLAG;
  if (argc > 0 && strcasecmp (argv[0], "found") == 0)
    select_flags = FOUNDFLAG;

  Message ("optimizing...\n");
  /* This canonicalizes all the lines, and cleans up near-misses.  */
  /* hid_actionl ("djopt", "puller", 0); */

  current_is_bottom = (GetLayerGroupNumberByPointer(CURRENT)
                       == GetLayerGroupNumberBySide (BOTTOM_SIDE));
  current_is_top = (GetLayerGroupNumberByPointer(CURRENT)
                    == GetLayerGroupNumberBySide (TOP_SIDE));

  lines = g_hash_table_new_full (NULL, NULL, NULL, (GDestroyNotify)FreeExtra);
  arcs  = g_hash_table_new_full (NULL, NULL, NULL, (GDestroyNotify)FreeExtra);

  Message ("pairing...\n");
  find_pairs ();
  validate_pairs ();

  g_hash_table_foreach (lines, (GHFunc)mark_ends_pending, &select_flags);

#if TRACE1
  trace_print_lines_arcs ();
#endif

  propogate_ends ();

#if TRACE1
  trace_print_lines_arcs ();
  trace_paths ();
#endif

  Message ("pulling...\n");
  if (setjmp(abort_buf) == 0)
    {
#if TRACE0
      int old_did_something = -1;
#endif
      did_something = 1;
      while (did_something)
	{
	  nloops ++;
	  status();
	  did_something = 0;
	  LINE_LOOP (CURRENT); {
	    Extra *e = LINE2EXTRA (line);
	    if (e->deleted)
	      continue;
#ifdef CHECK_LINE_PT_NEG
	    if (line->Point1.X < 0)
	      abort1();
#endif
	    if (e->start.next || e->end.next)
	      maybe_pull (line, e);
#if TRACE0
	    if (did_something != old_did_something)
	      {
		IncrementUndoSerialNumber();
		old_did_something = did_something;
		if (gui->confirm_dialog("more?", 0) == 0)
		  {
		    did_something = 0;
		    break;
		  }
	      }
#endif
	    /*gui->progress(0,0,0);*/
	  } END_LOOP;
	}
    }

#if TRACE0
  printf("\nlines\n");
  g_hash_table_foreach (lines, (GHFunc)trace_print_extra, NULL);
  printf("\narcs\n");
  g_hash_table_foreach (arcs, (GHFunc)trace_print_extra, NULL);
  printf("\n");
  printf("\nlines\n");
#endif

  LINE_LOOP (CURRENT);
    {
      if (LINE2EXTRA (line)->deleted)
	RemoveLine (CURRENT, line);
    }
  END_LOOP;

  ARC_LOOP (CURRENT);
    {
      if (ARC2EXTRA (arc)->deleted)
	RemoveArc (CURRENT, arc);
    }
  END_LOOP;

  g_hash_table_unref (lines);
  g_hash_table_unref (arcs);

  IncrementUndoSerialNumber();
  return 0;
}

/*                             Actions                                       */

HID_Action puller_action_list[] = {
  {"Puller", "Click on a line-arc intersection or line segment", Puller,
   puller_help, puller_syntax},
  {"GlobalPuller", 0, GlobalPuller,
   globalpuller_help, globalpuller_syntax}
};

REGISTER_ACTIONS (puller_action_list)
