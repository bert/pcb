/*!
 * \file jostle.c
 *
 * \brief jostle plug-in for PCB.
 *
 * \author Copyright (C) 2007 Ben Jackson <ben@ben.com>
 *
 * \copyright Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Pushes lines out of the way.
 */

#include <stdio.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "global.h"
#include "data.h"
#include "hid.h"
#include "hid_draw.h"
#include "misc.h"
#include "create.h"
#include "rtree.h"
#include "undo.h"
#include "rats.h"
#include "polygon.h"
#include "remove.h"
#include "error.h"
#include "set.h"
#include "pcb-printf.h"

//#define DEBUG_POLYAREA

double vect_dist2 (Vector v1, Vector v2);
#define Vcpy2(r,a)              {(r)[0] = (a)[0]; (r)[1] = (a)[1];}
#define Vswp2(a,b) { long t; \
        t = (a)[0], (a)[0] = (b)[0], (b)[0] = t; \
        t = (a)[1], (a)[1] = (b)[1], (b)[1] = t; \
}

//{if (!Marked.status && side==NORTHWEST) { DrawMark(True); Marked.status = True; Marked.X = p[0]; Marked.Y = p[1]; DrawMark(False);} }

enum
{
  NORTH,
  NORTHEAST,
  EAST,
  SOUTHEAST,
  SOUTH,
  SOUTHWEST,
  WEST,
  NORTHWEST
};

const char *dirnames[] =
{
  "NORTH",
  "NORTHEAST",
  "EAST",
  "SOUTHEAST",
  "SOUTH",
  "SOUTHWEST",
  "WEST",
  "NORTHWEST"
};

#define ARG(n) (argc > (n) ? argv[n] : 0)

static const char jostle_syntax[] = "Jostle(diameter)";

/* DEBUG */
static void
DebugPOLYAREA (POLYAREA *s, char *color)
{
  int *x, *y, n, i = 0;
  PLINE *pl;
  VNODE *v;
  POLYAREA *p;

#ifndef DEBUG_POLYAREA
  return;
#endif
  p = s;
  do
  {
    for (pl = p->contours; pl; pl = pl->next)
    {
      n = pl->Count;
      x = (int *) malloc (n * sizeof (int));
      y = (int *) malloc (n * sizeof (int));
      for (v = &pl->head; i < n; v = v->next)
      {
        x[i] = v->point[0];
        y[i++] = v->point[1];
      }
      if (1)
      {
        gui->graphics->set_color (Output.fgGC, color ? color : PCB->ConnectedColor);
        gui->graphics->set_line_width (Output.fgGC, 1);
        for (i = 0; i < n - 1; i++)
        {
          gui->graphics->draw_line (Output.fgGC, x[i], y[i], x[i + 1], y[i + 1]);
          //  gui->fill_circle (Output.fgGC, x[i], y[i], 30);
        }
        gui->graphics->draw_line (Output.fgGC, x[n - 1], y[n - 1], x[0], y[0]);
      }
      free (x);
      free (y);
    }
  } while ((p = p->f) != s);
  hid_action ("Busy");
  sleep (3);
}


#if 0
enum
{
  K_X,
  K_Y,
  K_Lefts,
  K_Rights,
  K_Tops,
  K_Bottoms,
  K_Centers,
  K_Marks,
  K_Gaps,
  K_First,
  K_Last,
  K_Average,
  K_Crosshair,
  K_Gridless,
  K_none,
  K_align,
  K_distribute
};

static const char *keywords[] =
{
  [K_X] "X",
  [K_Y] "Y",
  [K_Lefts] "Lefts",
  [K_Rights] "Rights",
  [K_Tops] "Tops",
  [K_Bottoms] "Bottoms",
  [K_Centers] "Centers",
  [K_Marks] "Marks",
  [K_Gaps] "Gaps",
  [K_First] "First",
  [K_Last] "Last",
  [K_Average] "Average",
  [K_Crosshair] "Crosshair",
  [K_Gridless] "Gridless",
};

static int
keyword (const char *s)
{
  int i;

  if (! s)
  {
    return K_none;
  }
  for (i = 0; i < ENTRIES(keywords); ++i)
  {
    if (keywords[i] && strcasecmp(s, keywords[i]) == 0)
      return i;
  }
  return -1;
}
#endif

/*!
 * \brief Find the bounding box of a POLYAREA.
 *
 * POLYAREAs linked by ->f/b are outlines.\n
 * n->contours->next would be the start of the inner holes (irrelevant
 * for bounding box).
 */
static BoxType
POLYAREA_boundingBox (POLYAREA *a)
{
  POLYAREA *n;
  PLINE *pa;
  BoxType box;
  int first = 1;

  n = a;
  do
  {
    pa = n->contours;
    if (first)
    {
      box.X1 = pa->xmin;
      box.X2 = pa->xmax + 1;
      box.Y1 = pa->ymin;
      box.Y2 = pa->ymax + 1;
      first = 0;
    }
    else
    {
      MAKEMIN (box.X1, pa->xmin);
      MAKEMAX (box.X2, pa->xmax + 1);
      MAKEMIN (box.Y1, pa->ymin);
      MAKEMAX (box.Y2, pa->ymax + 1);
    }
  } while ((n = n->f) != a);
  return box;
}

/*!
 * Given a polygon and a side of it (a direction north/northeast/etc),
 * find a line tangent to that side, offset by clearance, and return it
 * as a pair of vectors PQ.\n
 * Make it long so it will intersect everything in the area.
 */
static void
POLYAREA_findXmostLine (POLYAREA *a, int side, Vector p, Vector q, int clearance)
{
  p[0] = p[1] = 0;
  q[0] = q[1] = 0;
  int extra = a->contours->xmax - a->contours->xmin
    + a->contours->ymax - a->contours->ymin;
  switch (side)
  {
    case NORTH:
      p[1] = q[1] = a->contours->ymin - clearance;
      p[0] = a->contours->xmin - extra;
      q[0] = a->contours->xmax + extra;
      break;
    case SOUTH:
      p[1] = q[1] = a->contours->ymax + clearance;
      p[0] = a->contours->xmin - extra;
      q[0] = a->contours->xmax + extra;
      break;
    case EAST:
      p[0] = q[0] = a->contours->xmax + clearance;
      p[1] = a->contours->ymin - extra;
      q[1] = a->contours->ymax + extra;
      break;
    case WEST:
      p[0] = q[0] = a->contours->xmin - clearance;
      p[1] = a->contours->ymin - extra;
      q[1] = a->contours->ymax + extra;
      break;
    default: /* diagonal case */
    {
      int kx, ky, minmax, dq, ckx, cky;
      Coord mm[2] = {MAX_COORD, -MAX_COORD};
      Vector mmp[2];
      VNODE *v;

      switch (side)
      {
        case NORTHWEST:
          kx = 1; /* new_x = kx * x + ky * y */
          ky = 1;
          dq = -1; /* extend line in +x, dq*y */
          ckx = cky = -1; /* clear line in ckx*clear, cky*clear */
          minmax = 0; /* min or max */
          break;
        case SOUTHWEST:
          kx = 1;
          ky = -1;
          dq = 1;
          ckx = -1;
          cky = 1;
          minmax = 0;
          break;
        case NORTHEAST:
          kx = 1;
          ky = -1;
          dq = 1;
          ckx = 1;
          cky = -1;
          minmax = 1;
          break;
        case SOUTHEAST:
          kx = ky = 1;
          dq = -1;
          ckx = cky = 1;
          minmax = 1;
          break;
        default:
          Message("bjj: aiee, what side?");
          return;
      }
    v = &a->contours->head;
    do
    {
      int test = kx * v->point[0] + ky * v->point[1];
      if (test < mm[0])
      {
        mm[0] = test;
        mmp[0][0] = v->point[0];
        mmp[0][1] = v->point[1];
      }
      if (test > mm[1])
      {
        mm[1] = test;
        mmp[1][0] = v->point[0];
        mmp[1][1] = v->point[1];
      }
    } while ((v = v->next) != &a->contours->head);
    Vcpy2 (p, mmp[minmax]);
    /* add clearance in the right direction */
    clearance *= 0.707123; /* = cos(45) = sqrt(2)/2 */
    p[0] += ckx * clearance;
    p[1] += cky * clearance;
    /* now create a tangent line to that point */
    Vcpy2 (q, p);
    p[0] += -extra;
    p[1] += -extra * dq;
    q[0] += extra;
    q[1] += extra * dq;
    }
  }
}

/*!
 * Given a 'side' from the NORTH/SOUTH/etc enum, rotate it by n.
 */
static int
rotateSide (int side, int n)
{
  return (side + n + 8) % 8;
}

/*!
 * Wrapper for CreateNewLineOnLayer that takes vectors and deals with Undo
 */
static LineType *
CreateVectorLineOnLayer (LayerType *layer, Vector a, Vector b, int thickness, int clearance, FlagType flags)
{
  LineType *line;

  line = CreateNewLineOnLayer (layer, a[0], a[1], b[0], b[1], thickness, clearance, flags);
  if (line)
  {
    AddObjectToCreateUndoList (LINE_TYPE, layer, line, line);
  }
  return line;
}

static LineType *
MakeBypassLine (LayerType *layer, Vector a, Vector b, LineType *orig, POLYAREA **expandp)
{
  LineType *line;

  line = CreateVectorLineOnLayer (layer, a, b,
    orig->Thickness, orig->Clearance, orig->Flags);
  if (line && expandp)
  {
    POLYAREA *p = LinePoly (line, line->Thickness + line->Clearance);
    poly_Boolean_free (*expandp, p, expandp, PBO_UNITE);
  }
  return line;
}

/*!
 * Given a 'brush' that's pushing things out of the way (possibly already
 * cut down to just the part relevant to our line) and a line that
 * intersects it on some layer, find the 45/90 lines required to go around
 * the brush on the named side.  Create them and remove the original.
 *
 * Imagine side = north:
 * <pre>
                 /      \
            ----b##FLAT##c----
               Q          P
   lA-ORIG####a            d####ORIG-lB
             /              \
 * </pre>
 * First find the extended three lines that go around the brush.
 * Then intersect them with each other and the original to find
 * points a, b, c, d.  Finally connect the dots and remove the
 * old straight line.
 */
static int
MakeBypassingLines (POLYAREA *brush, LayerType *layer, LineType *line, int side, POLYAREA **expandp)
{
  Vector pA, pB, flatA, flatB, qA, qB;
  Vector lA, lB;
  Vector a, b, c, d, junk;
  int hits;

  SET_FLAG (DRCFLAG, line); /* will cause sublines to inherit */
  lA[0] = line->Point1.X;
  lA[1] = line->Point1.Y;
  lB[0] = line->Point2.X;
  lB[1] = line->Point2.Y;

  POLYAREA_findXmostLine (brush, side, flatA, flatB, line->Thickness / 2);
  POLYAREA_findXmostLine (brush, rotateSide(side, 1), pA, pB, line->Thickness / 2);
  POLYAREA_findXmostLine (brush, rotateSide(side, -1), qA, qB, line->Thickness / 2);
  hits = vect_inters2 (lA, lB, qA, qB, a, junk) + 
    vect_inters2 (qA, qB, flatA, flatB, b, junk) +
    vect_inters2 (pA, pB, flatA, flatB, c, junk) +
    vect_inters2 (lA, lB, pA, pB, d, junk);
  if (hits != 4)
  {
    return 0;
  }
  /* flip the line endpoints to match up with a/b */
  if (vect_dist2 (lA, d) < vect_dist2 (lA, a))
  {
    Vswp2 (lA, lB);
  }
  MakeBypassLine (layer, lA, a, line, NULL);
  MakeBypassLine (layer, a, b, line, expandp);
  MakeBypassLine (layer, b, c, line, expandp);
  MakeBypassLine (layer, c, d, line, expandp);
  MakeBypassLine (layer, d, lB, line, NULL);
  RemoveLine (layer, line);
  return 1;
}

struct info
{
  BoxType box;
  POLYAREA *brush;
  LayerType *layer;
  POLYAREA *smallest;
    /*!< after cutting brush with line, the smallest chunk, which we
     * will go around on 'side'.
     */
  LineType *line;
  int side;
  double centroid;
    /*!< smallest difference between slices of brush after cutting with
     * line, trying to find the line closest to the centroid to process
     * first
     */
};

/*!
 * Process lines that intersect our 'brush'.
 */
static int
jostle_callback (const BoxType *targ, void *private)
{
  LineType *line = (LineType *) targ;
  struct info *info = private;
  POLYAREA *lp, *copy, *tmp, *n, *smallest = NULL;
  Vector p;
  int inside = 0, side, r;
  double small, big;
  int nocentroid = 0;

  if (TEST_FLAG (DRCFLAG, line))
  {
    return 0;
  }
  fprintf (stderr, "hit! %p\n", line);
  p[0] = line->Point1.X;
  p[1] = line->Point1.Y;
  if (poly_InsideContour (info->brush->contours, p))
  {
    pcb_fprintf (stderr, "\tinside1 %ms,%ms\n", p[0],p[1]);
    inside++;
  }
  p[0] = line->Point2.X;
  p[1] = line->Point2.Y;
  if (poly_InsideContour (info->brush->contours, p))
  {
    pcb_fprintf (stderr, "\tinside2 %ms,%ms\n", p[0],p[1]);
    inside++;
  }
  lp = LinePoly (line, line->Thickness);
  if (!Touching (lp, info->brush))
  {
    /* not a factor */
    return 0;
  }
  poly_Free (&lp);
  if (inside)
  {
    // XXX not done!
    // XXX if this is part of a series of lines passing
    // XXX through, need to process as a group.
    // XXX if it just ends in here, shorten it??
    return 0;
  }
  /*
   * Cut the brush with the line to figure out which side to go
   * around.  Use a very fine line.  XXX can still graze.
   */
  lp = LinePoly (line, 1);
  if (!poly_M_Copy0 (&copy, info->brush))
    return 0;
  r = poly_Boolean_free (copy, lp, &tmp, PBO_SUB);
  if (r != err_ok)
  {
    pcb_fprintf (stderr, "Error while jostling PBO_SUB: %d\n", r);
    return 0;
  }
  if (tmp == tmp->f)
  {
    /* it didn't slice, must have glanced. intersect instead
     * to get the glancing sliver??
     */
    pcb_fprintf (stderr, "try isect??\n");
    lp = LinePoly (line, line->Thickness);
    r = poly_Boolean_free (tmp, lp, &tmp, PBO_ISECT);
    if (r != err_ok)
    {
      fprintf (stderr, "Error while jostling PBO_ISECT: %d\n", r);
      return 0;
    }
    nocentroid = 1;
  }
  /* XXX if this operation did not create two chunks, bad things are about to happen */
  if (! tmp)
    return 0;
  n = tmp;
  small = big = tmp->contours->area;
  do
  {
    pcb_fprintf (stderr, "\t\tarea %g, %ms,%ms %ms,%ms\n", n->contours->area, n->contours->xmin,n->contours->ymin, n->contours->xmax,n->contours->ymax);
    if (n->contours->area <= small)
    {
      smallest = n;
      small = n->contours->area;
    }
    if (n->contours->area >= big)
    {
      big = n->contours->area;
    }
  } while((n = n->f) != tmp);
  if (line->Point1.X == line->Point2.X)
  { /* | */
    if (info->box.X2 - smallest->contours->xmax >
      smallest->contours->xmin - info->box.X1)
    {
      side = WEST;
    }
    else
    {
      side = EAST;
    }
  }
  else if (line->Point1.Y == line->Point2.Y)
  { /* - */
    if (info->box.Y2 - smallest->contours->ymax >
      smallest->contours->ymin - info->box.Y1)
    {
      side = NORTH;
    }
    else
    {
      side = SOUTH;
    }
  }
  else if ((line->Point1.X > line->Point2.X) ==
    (line->Point1.Y > line->Point2.Y))
  { /* \ */
    if (info->box.X2 - smallest->contours->xmax >
      smallest->contours->xmin - info->box.X1)
    {
      side = SOUTHWEST;
    }
    else
    {
      side = NORTHEAST;
    }
  }
  else
  { /* / */
    if (info->box.X2 - smallest->contours->xmax >
      smallest->contours->xmin - info->box.X1)
    {
      side = NORTHWEST;
    }
    else
    {
      side = SOUTHEAST;
    }
  }
  pcb_fprintf (stderr, "\t%s\n", dirnames[side]);
  if (info->line == NULL ||
    (!nocentroid && (big - small) < info->centroid))
  {
    pcb_fprintf (stderr, "\tkeep it!\n");
    if (info->smallest)
    {
      poly_Free (&info->smallest);
    }
    info->centroid = nocentroid ? DBL_MAX : (big - small);
    info->side = side;
    info->line = line;
    info->smallest = smallest;
    return 1;
  }
  return 0;
}

static int
jostle (int argc, char **argv, Coord x, Coord y)
{
  bool rel;
  POLYAREA *expand;
  float value;
  struct info info;
  int found;

  if (argc == 2)
  {
    value = GetValue (ARG(0), ARG(1), &rel);
  }
  else
  {
    value = Settings.ViaThickness + (PCB->Bloat + 1) * 2 + 50;
  }
  x = Crosshair.X;
  y = Crosshair.Y;
  fprintf (stderr, "%d, %d, %f\n", (int)x, (int)y, value);
  info.brush = CirclePoly (x, y, value / 2);
  info.layer = CURRENT;
  LINE_LOOP (info.layer);
  {
    CLEAR_FLAG (DRCFLAG, line);
  }
  END_LOOP;
  do
  {
    info.box = POLYAREA_boundingBox (info.brush);
    DebugPOLYAREA (info.brush, NULL);
    pcb_fprintf (stderr, "search (%ms,%ms)->(%ms,%ms):\n", info.box.X1,info.box.Y1, info.box.X2,info.box.Y2);
    info.line = NULL;
    info.smallest = NULL;
    found = r_search (info.layer->line_tree, &info.box, NULL, jostle_callback, &info);
    if (found)
    {
      expand = NULL;
      MakeBypassingLines (info.smallest, info.layer, info.line,
        info.side, &expand);
      poly_Free (&info.smallest);
      poly_Boolean_free (info.brush, expand, &info.brush, PBO_UNITE);
    }
  } while (found);
  SetChangedFlag (true);
  IncrementUndoSerialNumber ();
  return 0;
}

static HID_Action jostle_action_list[] =
{
  {"jostle", NULL, jostle, "Move lines out of the way", jostle_syntax},
};

REGISTER_ACTIONS (jostle_action_list)

void
hid_jostle_init ()
{
  register_jostle_action_list ();
}
