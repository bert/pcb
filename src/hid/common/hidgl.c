/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2009-2011 PCB Contributers (See ChangeLog for details)
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <math.h>
#include <assert.h>

/* The Linux OpenGL ABI 1.0 spec requires that we define
 * GL_GLEXT_PROTOTYPES before including gl.h or glx.h for extensions
 * in order to get prototypes:
 *   http://www.opengl.org/registry/ABI/
 */
#define GL_GLEXT_PROTOTYPES 1

/* This follows autoconf's recommendation for the AX_CHECK_GL macro
   https://www.gnu.org/software/autoconf-archive/ax_check_gl.html */
#if defined HAVE_WINDOWS_H && defined _WIN32
#  include <windows.h>
#endif
#if defined HAVE_GL_GL_H
#  include <GL/gl.h>
#elif defined HAVE_OPENGL_GL_H
#  include <OpenGL/gl.h>
#else
#  error autoconf couldnt find gl.h
#endif

/* This follows autoconf's recommendation for the AX_CHECK_GLU macro
   https://www.gnu.org/software/autoconf-archive/ax_check_glu.html */
#if defined HAVE_GL_GLU_H
#  include <GL/glu.h>
#elif defined HAVE_OPENGL_GLU_H
#  include <OpenGL/glu.h>
#else
#  error autoconf couldnt find glu.h
#endif

#include "action.h"
#include "crosshair.h"
#include "data.h"
#include "error.h"
#include "global.h"
#include "mymem.h"
#include "clip.h"

#include "hid.h"
#include "hid_draw.h"
#include "hidgl.h"
#include "rtree.h"
#include "sweep.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif


static void
hidgl_reset_triangle_array (hidgl_instance *hidgl)
{
  hidgl_priv *priv = hidgl->priv;

  priv->buffer.triangle_count = 0;
  priv->buffer.coord_comp_count = 0;
}

static void
hidgl_init_triangle_array (hidgl_instance *hidgl)
{
  hidgl_reset_triangle_array (hidgl);
}

void
hidgl_flush_triangles (hidgl_instance *hidgl)
{
  hidgl_priv *priv = hidgl->priv;

  if (priv->buffer.triangle_count == 0)
    return;

  glEnableClientState (GL_VERTEX_ARRAY);
  glVertexPointer   (3, GL_FLOAT, 0, priv->buffer.triangle_array);
  glDrawArrays (GL_TRIANGLES, 0, priv->buffer.triangle_count * 3);
  glDisableClientState (GL_VERTEX_ARRAY);

  hidgl_reset_triangle_array (hidgl);
}

void
hidgl_ensure_triangle_space (hidGC gc, int count)
{
  hidglGC hidgl_gc = (hidglGC)gc;
  hidgl_instance *hidgl = hidgl_gc->hidgl;
  hidgl_priv *priv = hidgl->priv;

  if (count > TRIANGLE_ARRAY_SIZE)
    {
      fprintf (stderr, "Not enough space in vertex buffer\n");
      fprintf (stderr, "Requested %i triangles, %i available\n",
                       count, TRIANGLE_ARRAY_SIZE);
      exit (1);
    }
  if (count > TRIANGLE_ARRAY_SIZE - priv->buffer.triangle_count)
    hidgl_flush_triangles (hidgl);
}

void
hidgl_set_depth (hidGC gc, float depth)
{
  hidglGC hidgl_gc = (hidglGC)gc;

  hidgl_gc->depth = depth;
}

/*!
 * \brief Draw the grid on the 3D canvas
 */
void
hidgl_draw_grid (hidGC gc, BoxType *drawn_area)
{
  hidglGC hidgl_gc = (hidglGC)gc;

  static GLfloat *points = 0;
  static int npoints = 0;
  Coord x1, y1, x2, y2, n, i;
  double x, y;

  if (!Settings.DrawGrid)
    return; /* grid hidden */

  /* Find the bounding grid points, all others lay between them */
  x1 = GridFit (MAX (0, drawn_area->X1), PCB->Grid, PCB->GridOffsetX);
  y1 = GridFit (MAX (0, drawn_area->Y1), PCB->Grid, PCB->GridOffsetY);
  x2 = GridFit (MIN (PCB->MaxWidth,  drawn_area->X2), PCB->Grid, PCB->GridOffsetX);
  y2 = GridFit (MIN (PCB->MaxHeight, drawn_area->Y2), PCB->Grid, PCB->GridOffsetY);

  if (x1 > x2)
    {
      Coord tmp = x1;
      x1 = x2;
      x2 = tmp;
    }

  if (y1 > y2)
    {
      Coord tmp = y1;
      y1 = y2;
      y2 = tmp;
    }

  n = (int) ((x2 - x1) / PCB->Grid + 0.5) + 1; /* Number of points in one row */
  if (n > npoints)
    { /* [n]points are static, reallocate if we need more memory */
      npoints = n + 10;
      points = realloc (points, npoints * 3 * sizeof (GLfloat));
    }

  glEnableClientState (GL_VERTEX_ARRAY);
  glVertexPointer (3, GL_FLOAT, 0, points);

  n = 0;
  for (x = x1; x <= x2; x += PCB->Grid)
    { /* compute all the x coordinates */
      points[3 * n + 0] = x;
      points[3 * n + 2] = hidgl_gc->depth;
      n++;
    }
  for (y = y1; y <= y2; y += PCB->Grid)
    { /* reuse the row of points at each y */
      for (i = 0; i < n; i++)
        points[3 * i + 1] = y;
      /* draw all the points in a row for a given y */
      glDrawArrays (GL_POINTS, 0, n);
    }

  glDisableClientState (GL_VERTEX_ARRAY);
}

#define MAX_PIXELS_ARC_TO_CHORD 0.5
#define MIN_SLICES 6
int calc_slices (float pix_radius, float sweep_angle)
{
  float slices;

  if (pix_radius <= MAX_PIXELS_ARC_TO_CHORD)
    return MIN_SLICES;

  slices = sweep_angle / acosf (1 - MAX_PIXELS_ARC_TO_CHORD / pix_radius) / 2.;
  return (int)ceilf (slices);
}

#define MIN_TRIANGLES_PER_CAP 3
#define MAX_TRIANGLES_PER_CAP 90
static void draw_cap (hidGC gc, Coord width, Coord x, Coord y, Angle angle, double scale)
{
  float last_capx, last_capy;
  float capx, capy;
  float radius = width / 2.;
  int slices = calc_slices (radius / scale, M_PI);
  int i;

  if (slices < MIN_TRIANGLES_PER_CAP)
    slices = MIN_TRIANGLES_PER_CAP;

  if (slices > MAX_TRIANGLES_PER_CAP)
    slices = MAX_TRIANGLES_PER_CAP;

  hidgl_ensure_triangle_space (gc, slices);

  last_capx =  radius * cosf (angle * M_PI / 180.) + x;
  last_capy = -radius * sinf (angle * M_PI / 180.) + y;
  for (i = 0; i < slices; i++) {
    capx =  radius * cosf (angle * M_PI / 180. + ((float)(i + 1)) * M_PI / (float)slices) + x;
    capy = -radius * sinf (angle * M_PI / 180. + ((float)(i + 1)) * M_PI / (float)slices) + y;
    hidgl_add_triangle (gc, last_capx, last_capy, capx, capy, x, y);
    last_capx = capx;
    last_capy = capy;
  }
}

void
hidgl_draw_line (hidGC gc, int cap, Coord width, Coord x1, Coord y1, Coord x2, Coord y2, double scale)
{
  double angle;
  double deltax, deltay, length;
  float wdx, wdy;
  int circular_caps = 0;
  int hairline = 0;

  if (width == 0.0)
    hairline = 1;

  if (width < scale)
    width = scale;

  deltax = x2 - x1;
  deltay = y2 - y1;

  length = hypot (deltax, deltay);

  if (length == 0) {
    /* Assume the orientation of the line is horizontal */
    wdx = -width / 2.;
    wdy = 0;
    length = 1.;
    deltax = 1.;
    deltay = 0.;
  } else {
    wdy = deltax * width / 2. / length;
    wdx = -deltay * width / 2. / length;
  }

  angle = -180. / M_PI * atan2 (deltay, deltax);

  switch (cap) {
    case Trace_Cap:
    case Round_Cap:
      circular_caps = 1;
      break;

    case Square_Cap:
    case Beveled_Cap:
      x1 -= deltax * width / 2. / length;
      y1 -= deltay * width / 2. / length;
      x2 += deltax * width / 2. / length;
      y2 += deltay * width / 2. / length;
      break;
  }

  hidgl_ensure_triangle_space (gc, 2);
  hidgl_add_triangle (gc, x1 - wdx, y1 - wdy,
                          x2 - wdx, y2 - wdy,
                          x2 + wdx, y2 + wdy);
  hidgl_add_triangle (gc, x1 - wdx, y1 - wdy,
                          x2 + wdx, y2 + wdy,
                          x1 + wdx, y1 + wdy);

  /* Don't bother capping hairlines */
  if (circular_caps && !hairline)
    {
      draw_cap (gc, width, x1, y1, angle + 90., scale);
      draw_cap (gc, width, x2, y2, angle - 90., scale);
    }
}

#define MIN_SLICES_PER_ARC 6
#define MAX_SLICES_PER_ARC 360
void
hidgl_draw_arc (hidGC gc, Coord width, Coord x, Coord y, Coord rx, Coord ry,
                Angle start_angle, Angle delta_angle, double scale)
{
  float last_inner_x, last_inner_y;
  float last_outer_x, last_outer_y;
  float inner_x, inner_y;
  float outer_x, outer_y;
  float inner_r;
  float outer_r;
  float cos_ang, sin_ang;
  float start_angle_rad;
  float delta_angle_rad;
  float angle_incr_rad;
  int slices;
  int i;
  int hairline = 0;

  if (width == 0.0)
    hairline = 1;

  if (width < scale)
    width = scale;

  inner_r = rx - width / 2.;
  outer_r = rx + width / 2.;

  if (delta_angle < 0) {
    start_angle += delta_angle;
    delta_angle = - delta_angle;
  }

  start_angle_rad = start_angle * M_PI / 180.;
  delta_angle_rad = delta_angle * M_PI / 180.;

  slices = calc_slices ((rx + width / 2.) / scale, delta_angle_rad);

  if (slices < MIN_SLICES_PER_ARC)
    slices = MIN_SLICES_PER_ARC;

  if (slices > MAX_SLICES_PER_ARC)
    slices = MAX_SLICES_PER_ARC;

  hidgl_ensure_triangle_space (gc, 2 * slices);

  angle_incr_rad = delta_angle_rad / (float)slices;

  cos_ang = cosf (start_angle_rad);
  sin_ang = sinf (start_angle_rad);
  last_inner_x = -inner_r * cos_ang + x;  last_inner_y = inner_r * sin_ang + y;
  last_outer_x = -outer_r * cos_ang + x;  last_outer_y = outer_r * sin_ang + y;
  for (i = 1; i <= slices; i++) {
    cos_ang = cosf (start_angle_rad + ((float)(i)) * angle_incr_rad);
    sin_ang = sinf (start_angle_rad + ((float)(i)) * angle_incr_rad);
    inner_x = -inner_r * cos_ang + x;  inner_y = inner_r * sin_ang + y;
    outer_x = -outer_r * cos_ang + x;  outer_y = outer_r * sin_ang + y;
    hidgl_add_triangle (gc, last_inner_x, last_inner_y,
                            last_outer_x, last_outer_y,
                            outer_x, outer_y);
    hidgl_add_triangle (gc, last_inner_x, last_inner_y,
                            inner_x, inner_y,
                            outer_x, outer_y);
    last_inner_x = inner_x;  last_inner_y = inner_y;
    last_outer_x = outer_x;  last_outer_y = outer_y;
  }

  /* Don't bother capping hairlines */
  if (hairline)
    return;

  draw_cap (gc, width, x + rx * -cosf (start_angle_rad),
                       y + rx *  sinf (start_angle_rad),
                       start_angle, scale);
  draw_cap (gc, width, x + rx * -cosf (start_angle_rad + delta_angle_rad),
                       y + rx *  sinf (start_angle_rad + delta_angle_rad),
                       start_angle + delta_angle + 180., scale);
}

void
hidgl_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  hidglGC hidgl_gc = (hidglGC)gc;

  glBegin (GL_LINE_LOOP);
  glVertex3f (x1, y1, hidgl_gc->depth);
  glVertex3f (x1, y2, hidgl_gc->depth);
  glVertex3f (x2, y2, hidgl_gc->depth);
  glVertex3f (x2, y1, hidgl_gc->depth);
  glEnd ();
}


void
hidgl_fill_circle (hidGC gc, Coord vx, Coord vy, Coord vr, double scale)
{
#define MIN_TRIANGLES_PER_CIRCLE 6
#define MAX_TRIANGLES_PER_CIRCLE 360
  float last_x, last_y;
  float radius = vr;
  int slices;
  int i;

  slices = calc_slices (vr / scale, 2 * M_PI);

  if (slices < MIN_TRIANGLES_PER_CIRCLE)
    slices = MIN_TRIANGLES_PER_CIRCLE;

  if (slices > MAX_TRIANGLES_PER_CIRCLE)
    slices = MAX_TRIANGLES_PER_CIRCLE;

  hidgl_ensure_triangle_space (gc, slices);

  last_x = vx + vr;
  last_y = vy;

  for (i = 0; i < slices; i++) {
    float x, y;
    x = radius * cosf (((float)(i + 1)) * 2. * M_PI / (float)slices) + vx;
    y = radius * sinf (((float)(i + 1)) * 2. * M_PI / (float)slices) + vy;
    hidgl_add_triangle (gc, vx, vy, last_x, last_y, x, y);
    last_x = x;
    last_y = y;
  }
}

#define MAX_COMBINED_MALLOCS 2500
static void *combined_to_free [MAX_COMBINED_MALLOCS];
static int combined_num_to_free = 0;

static GLenum tessVertexType;
static int stashed_vertices;
static int triangle_comp_idx;

#ifndef CALLBACK
#define CALLBACK
#endif

static void CALLBACK
myError (GLenum errno)
{
  printf ("gluTess error: %s\n", gluErrorString (errno));
}

static void CALLBACK
myCombine ( GLdouble coords[3], void *vertex_data[4], GLfloat weight[4], void **dataOut )
{
#define MAX_COMBINED_VERTICES 2500
  static GLdouble combined_vertices [3 * MAX_COMBINED_VERTICES];
  static int num_combined_vertices = 0;

  GLdouble *new_vertex;

  if (num_combined_vertices < MAX_COMBINED_VERTICES)
    {
      new_vertex = &combined_vertices [3 * num_combined_vertices];
      num_combined_vertices ++;
    }
  else
    {
      new_vertex = malloc (3 * sizeof (GLdouble));

      if (combined_num_to_free < MAX_COMBINED_MALLOCS)
        combined_to_free [combined_num_to_free ++] = new_vertex;
      else
        printf ("myCombine leaking %lu bytes of memory\n",
                    (unsigned long)3 * sizeof (GLdouble));
    }

  new_vertex[0] = coords[0];
  new_vertex[1] = coords[1];
  new_vertex[2] = coords[2];

  *dataOut = new_vertex;
}

static void CALLBACK
myBegin (GLenum type)
{
  tessVertexType = type;
  stashed_vertices = 0;
  triangle_comp_idx = 0;
}

static double global_scale;
static hidGC tesselator_gc = NULL;

static void CALLBACK
myVertex (GLdouble *vertex_data)
{
  static GLfloat triangle_vertices [2 * 3];
  hidGC gc = tesselator_gc;

  if (tessVertexType == GL_TRIANGLE_STRIP ||
      tessVertexType == GL_TRIANGLE_FAN)
    {
      if (stashed_vertices < 2)
        {
          triangle_vertices [triangle_comp_idx ++] = vertex_data [0];
          triangle_vertices [triangle_comp_idx ++] = vertex_data [1];
          stashed_vertices ++;
        }
      else
        {
          hidgl_ensure_triangle_space (gc, 1);
          hidgl_add_triangle (gc,
                              triangle_vertices [0], triangle_vertices [1],
                              triangle_vertices [2], triangle_vertices [3],
                              vertex_data [0], vertex_data [1]);

          if (tessVertexType == GL_TRIANGLE_STRIP)
            {
              /* STRIP saves the last two vertices for re-use in the next triangle */
              triangle_vertices [0] = triangle_vertices [2];
              triangle_vertices [1] = triangle_vertices [3];
            }
          /* Both FAN and STRIP save the last vertex for re-use in the next triangle */
          triangle_vertices [2] = vertex_data [0];
          triangle_vertices [3] = vertex_data [1];
        }
    }
  else if (tessVertexType == GL_TRIANGLES)
    {
      triangle_vertices [triangle_comp_idx ++] = vertex_data [0];
      triangle_vertices [triangle_comp_idx ++] = vertex_data [1];
      stashed_vertices ++;
      if (stashed_vertices == 3)
        {
          hidgl_ensure_triangle_space (gc, 1);
          hidgl_add_triangle (gc,
                              triangle_vertices [0], triangle_vertices [1],
                              triangle_vertices [2], triangle_vertices [3],
                              triangle_vertices [4], triangle_vertices [5]);
          triangle_comp_idx = 0;
          stashed_vertices = 0;
        }
    }
  else
    printf ("Vertex received with unknown type\n");
}

static void
myFreeCombined ()
{
  while (combined_num_to_free)
    free (combined_to_free [-- combined_num_to_free]);
}

void
hidgl_fill_polygon (hidGC gc, int n_coords, Coord *x, Coord *y)
{
  int i;
  GLUtesselator *tobj;
  GLdouble *vertices;

  assert (n_coords > 0);

  vertices = malloc (sizeof(GLdouble) * n_coords * 3);

  tesselator_gc = gc;
  tobj = gluNewTess ();
  gluTessCallback(tobj, GLU_TESS_BEGIN,   (_GLUfuncptr)myBegin);
  gluTessCallback(tobj, GLU_TESS_VERTEX,  (_GLUfuncptr)myVertex);
  gluTessCallback(tobj, GLU_TESS_COMBINE, (_GLUfuncptr)myCombine);
  gluTessCallback(tobj, GLU_TESS_ERROR,   (_GLUfuncptr)myError);

  gluTessBeginPolygon (tobj, NULL);
  gluTessBeginContour (tobj);

  for (i = 0; i < n_coords; i++)
    {
      vertices [0 + i * 3] = x[i];
      vertices [1 + i * 3] = y[i];
      vertices [2 + i * 3] = 0.;
      gluTessVertex (tobj, &vertices [i * 3], &vertices [i * 3]);
    }

  gluTessEndContour (tobj);
  gluTessEndPolygon (tobj);
  gluDeleteTess (tobj);
  tesselator_gc = NULL;

  myFreeCombined ();
  free (vertices);
}

static void
fill_contour (hidGC gc, PLINE *contour, double scale)
{
  borast_traps_t traps;

  /* If the contour is round, and hidgl_fill_circle would use
   * less slices than we have vertices to draw it, then call
   * hidgl_fill_circle to draw this contour.
   */
  if (contour->is_round) {
    double slices = calc_slices (contour->radius / scale, 2 * M_PI);
    if (slices < contour->Count) {
      hidgl_fill_circle (gc, contour->cx, contour->cy, contour->radius, scale);
      return;
    }
  }

  _borast_traps_init (&traps);
  bo_contour_to_traps (gc, contour, &traps);
  _borast_traps_fini (&traps);
}

struct do_hole_info {
  hidGC gc;
  double scale;
};

static int
do_hole (const BoxType *b, void *cl)
{
  struct do_hole_info *info = cl;
  PLINE *curc = (PLINE *) b;

  /* Ignore the outer contour - we draw it first explicitly*/
  if (curc->Flags.orient == PLF_DIR) {
    return 0;
  }

  fill_contour (info->gc, curc, info->scale);
  return 1;
}

static void
fill_polyarea (hidGC gc, POLYAREA *pa, const BoxType *clip_box, double scale)
{
  hidglGC hidgl_gc = (hidglGC)gc;
  hidgl_instance *hidgl = hidgl_gc->hidgl;
  hidgl_priv *priv = hidgl->priv;
  struct do_hole_info info;
  int stencil_bit;

  info.gc = gc;
  info.scale = scale;
  global_scale = scale;

  /* Special case non-holed polygons which don't require a stencil bit */
  if (pa->contour_tree->size == 1) {
    fill_contour (gc, pa->contours, scale);
    return;
  }

  /* Polygon has holes */

  stencil_bit = hidgl_assign_clear_stencil_bit (hidgl);
  if (!stencil_bit)
    {
      printf ("hidgl_fill_pcb_polygon: No free stencil bits, aborting polygon\n");
      /* XXX: Could use the GLU tesselator or the full BO polygon tesselator */
      return;
    }

  /* Flush out any existing geoemtry to be rendered */
  hidgl_flush_triangles (hidgl);

  glPushAttrib (GL_STENCIL_BUFFER_BIT |                 /* Resave the stencil write-mask etc.., and */
                GL_COLOR_BUFFER_BIT);                   /* the colour buffer write mask etc.. for part way restore */
  glStencilMask (stencil_bit);                          /* Only write to our stencil bit */
  glStencilFunc (GL_ALWAYS, stencil_bit, stencil_bit);  /* Always pass stencil test, ref value is our bit */
  glEnable (GL_STENCIL_TEST);                           /* Enable the stencil test, just in case it wasn't already on */
  glColorMask (0, 0, 0, 0);                             /* Disable writting in color buffer */

  glStencilOp (GL_KEEP, GL_KEEP, GL_REPLACE);           /* Stencil pass => replace stencil value */

  /* Drawing operations now set our reference bit in the stencil buffer */

  r_search (pa->contour_tree, clip_box, NULL, do_hole, &info);
  hidgl_flush_triangles (hidgl);

  glPopAttrib ();                               /* Restore the colour and stencil buffer write-mask etc.. */
  glPushAttrib (GL_STENCIL_BUFFER_BIT);         /* Save the stencil op and function */
  glEnable (GL_STENCIL_TEST);                   /* Enable the stencil test, just in case it wasn't already on */

  glStencilOp (GL_KEEP, GL_KEEP, GL_INVERT);    /* This allows us to toggle the bit on the subcompositing bitplane */
                                                /* If the stencil test has passed, we know that bit is 0, so we're */
                                                /* effectively just setting it to 1. */

  glStencilFunc (GL_GEQUAL, 0, priv->assigned_bits);  /* Pass stencil test if all assigned bits clear, */
                                                /* reference is all assigned bits so we set */
                                                /* any bits permitted by the stencil writemask */

  /* Drawing operations as masked to areas where the stencil buffer is '0' */

  /* Draw the polygon outer */
  fill_contour (gc, pa->contours, scale);
  hidgl_flush_triangles (hidgl);

  /* Unassign our stencil buffer bit */
  hidgl_return_stencil_bit (hidgl, stencil_bit);

  glPopAttrib ();                               /* Restore the stencil buffer op and function */
}

void
hidgl_fill_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box, double scale)
{
  if (poly->Clipped == NULL)
    return;

  fill_polyarea (gc, poly->Clipped, clip_box, scale);

  if (TEST_FLAG (FULLPOLYFLAG, poly))
    {
      POLYAREA *pa;

      for (pa = poly->Clipped->f; pa != poly->Clipped; pa = pa->f)
        fill_polyarea (gc, pa, clip_box, scale);
    }
}

void
hidgl_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  hidgl_ensure_triangle_space (gc, 2);
  hidgl_add_triangle (gc, x1, y1, x1, y2, x2, y2);
  hidgl_add_triangle (gc, x2, y1, x2, y2, x1, y1);
}

void
hidgl_init (void)
{
  static bool called = false;

  if (called == true)
    {
      fprintf (stderr, "ERROR: hidgl_init() called multiple times\n");
      return;
    }

  /* Any one-time (hopefully!) hidgl setup goes in here */

  called = true;
}

hidgl_instance *
hidgl_new_instance (void)
{
  hidgl_instance *hidgl;
  hidgl_priv *priv;

  hidgl = calloc (1, sizeof (hidgl_instance));
  priv = calloc (1, sizeof (hidgl_priv));
  hidgl->priv = priv;

#if 0
  glGetIntegerv (GL_STENCIL_BITS, &priv->stencil_bits);

  if (priv->stencil_bits == 0)
    {
      printf ("No stencil bits available.\n"
              "Cannot mask polygon holes or subcomposite layers\n");
      /* TODO: Flag this to the HID so it can revert to the dicer? */
    }
  else if (priv->stencil_bits == 1)
    {
      printf ("Only one stencil bitplane avilable\n"
              "Cannot use stencil buffer to sub-composite layers.\n");
      /* Do we need to disable that somewhere? */
    }

  hidgl_reset_stencil_usage (hidgl);
#endif
  hidgl_init_triangle_array (hidgl);

  return hidgl;
}

void
hidgl_free_instance (hidgl_instance *hidgl)
{
  free (hidgl->priv);
  free (hidgl);
}

void
hidgl_init_gc (hidgl_instance *hidgl, hidGC gc)
{
  hidglGC hidgl_gc = (hidglGC)gc;

  hidgl_gc->hidgl = hidgl;
  hidgl_gc->depth = 0.0;
}

void
hidgl_finish_gc (hidGC gc)
{
}

void
hidgl_start_render (hidgl_instance *hidgl)
{
  hidgl_priv *priv = hidgl->priv;

//  hidgl_init ();
  glGetIntegerv (GL_STENCIL_BITS, &priv->stencil_bits);

  if (priv->stencil_bits == 0)
    {
      printf ("No stencil bits available.\n"
              "Cannot mask polygon holes or subcomposite layers\n");
      /* TODO: Flag this to the HID so it can revert to the dicer? */
    }
  else if (priv->stencil_bits == 1)
    {
      printf ("Only one stencil bitplane avilable\n"
              "Cannot use stencil buffer to sub-composite layers.\n");
      /* Do we need to disable that somewhere? */
    }

  hidgl_reset_stencil_usage (hidgl);
  hidgl_init_triangle_array (hidgl);
}

void
hidgl_finish_render (hidgl_instance *hidgl)
{
}

int
hidgl_stencil_bits (hidgl_instance *hidgl)
{
  hidgl_priv *priv = hidgl->priv;

  return priv->stencil_bits;
}

static void
hidgl_clean_unassigned_stencil (hidgl_instance *hidgl)
{
  hidgl_priv *priv = hidgl->priv;

  glPushAttrib (GL_STENCIL_BUFFER_BIT);
  glStencilMask (~priv->assigned_bits);
  glClearStencil (0);
  glClear (GL_STENCIL_BUFFER_BIT);
  glPopAttrib ();
}

int
hidgl_assign_clear_stencil_bit (hidgl_instance *hidgl)
{
  hidgl_priv *priv = hidgl->priv;

  int stencil_bitmask = (1 << priv->stencil_bits) - 1;
  int test;
  int first_dirty = 0;

  if (priv->assigned_bits == stencil_bitmask)
    {
      printf ("No more stencil bits available, total of %i already assigned\n",
              priv->stencil_bits);
      return 0;
    }

  /* Look for a bitplane we don't have to clear */
  for (test = 1; test & stencil_bitmask; test <<= 1)
    {
      if (!(test & priv->dirty_bits))
        {
          priv->assigned_bits |= test;
          priv->dirty_bits    |= test;
          return test;
        }
      else if (!first_dirty && !(test & priv->assigned_bits))
        {
          first_dirty = test;
        }
    }

  /* Didn't find any non dirty planes. Clear those dirty ones which aren't in use */
  hidgl_clean_unassigned_stencil (hidgl);
  priv->assigned_bits |= first_dirty;
  priv->dirty_bits = priv->assigned_bits;

  return first_dirty;
}

void
hidgl_return_stencil_bit (hidgl_instance *hidgl, int bit)
{
  hidgl_priv *priv = hidgl->priv;

  priv->assigned_bits &= ~bit;
}

void
hidgl_reset_stencil_usage (hidgl_instance *hidgl)
{
  hidgl_priv *priv = hidgl->priv;

  priv->assigned_bits = 0;
  priv->dirty_bits = 0;
}
