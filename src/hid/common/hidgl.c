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

#ifdef WIN32
#   include "glext.h"

PFNGLGENBUFFERSPROC         glGenBuffers        = NULL;
PFNGLDELETEBUFFERSPROC      glDeleteBuffers     = NULL;
PFNGLBINDBUFFERPROC         glBindBuffer        = NULL;
PFNGLBUFFERDATAPROC         glBufferData        = NULL;
PFNGLBUFFERSUBDATAPROC      glBufferSubData     = NULL;
PFNGLMAPBUFFERPROC          glMapBuffer         = NULL;
PFNGLUNMAPBUFFERPROC        glUnmapBuffer       = NULL;

PFNGLATTACHSHADERPROC       glAttachShader      = NULL;
PFNGLCOMPILESHADERPROC      glCompileShader     = NULL;
PFNGLCREATEPROGRAMPROC      glCreateProgram     = NULL;
PFNGLCREATESHADERPROC       glCreateShader      = NULL;
PFNGLDELETEPROGRAMPROC      glDeleteProgram     = NULL;
PFNGLDELETESHADERPROC       glDeleteShader      = NULL;
PFNGLGETPROGRAMINFOLOGPROC  glGetProgramInfoLog = NULL;
PFNGLGETPROGRAMIVPROC       glGetProgramiv      = NULL;
PFNGLGETSHADERINFOLOGPROC   glGetShaderInfoLog  = NULL;
PFNGLGETSHADERIVPROC        glGetShaderiv       = NULL;
PFNGLISSHADERPROC           glIsShader          = NULL;
PFNGLLINKPROGRAMPROC        glLinkProgram       = NULL;
PFNGLSHADERSOURCEPROC       glShaderSource      = NULL;
PFNGLUSEPROGRAMPROC         glUseProgram        = NULL;

PFNGLMULTITEXCOORD1FPROC    glMultiTexCoord1f    = NULL;
PFNGLMULTITEXCOORD2FPROC    glMultiTexCoord2f    = NULL;
PFNGLMULTITEXCOORD3FPROC    glMultiTexCoord3f    = NULL;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = NULL;
PFNGLUNIFORM1IPROC          glUniform1i          = NULL;
PFNGLACTIVETEXTUREARBPROC   glActiveTextureARB   = NULL;
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

//#define MEMCPY_VERTEX_DATA 1

hidgl_shader *circular_program = NULL;
hidgl_shader *resistor_program = NULL;

static bool in_context = false;

#define CHECK_IS_IN_CONTEXT(retcode) \
  do { \
    if (!in_context) { \
      fprintf (stderr, "hidgl: Drawing called out of context in function %s\n", \
             __FUNCTION__); \
      return retcode; \
    } \
  } while (0)


#define BUFFER_STRIDE (5 * sizeof (GLfloat))
#define BUFFER_SIZE (BUFFER_STRIDE * 3 * TRIANGLE_ARRAY_SIZE)

/* NB: If using VBOs, the caller must ensure the VBO is bound to the GL_ARRAY_BUFFER */
static void
hidgl_reset_triangle_array (hidgl_instance *hidgl)
{
  hidgl_priv *priv = hidgl->priv;

  if (priv->buffer.use_map) {
    /* Hint to the driver that we're done with the previous buffer contents */
    glBufferData (GL_ARRAY_BUFFER, BUFFER_SIZE, NULL, GL_STREAM_DRAW);
    /* Map the new memory to upload vertices into. */
    priv->buffer.triangle_array = glMapBuffer (GL_ARRAY_BUFFER, GL_WRITE_ONLY);
  }

  /* If mapping the VBO fails (or if we aren't using VBOs) fall back to
   * local storage.
   */
  if (priv->buffer.triangle_array == NULL) {
    priv->buffer.triangle_array = malloc (BUFFER_SIZE);
    priv->buffer.use_map = false;
  }

  /* Don't want this bound for now */
  glBindBuffer (GL_ARRAY_BUFFER, 0);

  priv->buffer.triangle_count = 0;
  priv->buffer.coord_comp_count = 0;
  priv->buffer.vertex_count = 0;
}

static void
hidgl_init_triangle_array (hidgl_instance *hidgl)
{
  hidgl_priv *priv = hidgl->priv;

  CHECK_IS_IN_CONTEXT ();

  priv->buffer.use_vbo = true;
  // priv->buffer.use_vbo = false;

  if (priv->buffer.use_vbo) {
    glGenBuffers (1, &priv->buffer.vbo_id);
    glBindBuffer (GL_ARRAY_BUFFER, priv->buffer.vbo_id);
  }

  if (priv->buffer.vbo_id == 0)
    priv->buffer.use_vbo = false;

  priv->buffer.use_map = priv->buffer.use_vbo;

  /* NB: Mapping the whole buffer can be expensive since we ask the driver
   *     to discard previous data and give us a "new" buffer to write into
   *     each time. If it is still rendering from previous buffer, we end
   *     up causing a lot of unnecessary allocation in the driver this way.
   *
   *     On intel drivers at least, glBufferSubData does not block. It uploads
   *     into a temporary buffer and queues a GPU copy of the uploaded data
   *     for when the "main" buffer has finished rendering.
   */
  priv->buffer.use_map = false;

  priv->buffer.triangle_array = NULL;
  hidgl_reset_triangle_array (hidgl);
}

static void
hidgl_finish_triangle_array (hidgl_instance *hidgl)
{
  hidgl_priv *priv = hidgl->priv;

  if (priv->buffer.use_map) {
    glBindBuffer (GL_ARRAY_BUFFER, priv->buffer.vbo_id);
    glUnmapBuffer (GL_ARRAY_BUFFER);
    glBindBuffer (GL_ARRAY_BUFFER, 0);
  } else {
    free (priv->buffer.triangle_array);
  }

  if (priv->buffer.use_vbo) {
    glDeleteBuffers (1, &priv->buffer.vbo_id);
    priv->buffer.vbo_id = 0;
  }
}

void
hidgl_flush_triangles (hidgl_instance *hidgl)
{
  hidgl_priv *priv = hidgl->priv;
  GLfloat *data_pointer = NULL;

  CHECK_IS_IN_CONTEXT ();

  if (priv->buffer.vertex_count == 0)
    return;

  if (priv->buffer.use_vbo) {
    glBindBuffer (GL_ARRAY_BUFFER, priv->buffer.vbo_id);

    if (priv->buffer.use_map) {
      glUnmapBuffer (GL_ARRAY_BUFFER);
      priv->buffer.triangle_array = NULL;
    } else {
      glBufferData (GL_ARRAY_BUFFER,
                    BUFFER_STRIDE * priv->buffer.vertex_count,
                    priv->buffer.triangle_array,
                    GL_STREAM_DRAW);
    }
  } else {
    data_pointer = priv->buffer.triangle_array;
  }

  glTexCoordPointer (2, GL_FLOAT, BUFFER_STRIDE, data_pointer + 3);
  glVertexPointer   (3, GL_FLOAT, BUFFER_STRIDE, data_pointer + 0);

  glEnableClientState (GL_TEXTURE_COORD_ARRAY);
  glEnableClientState (GL_VERTEX_ARRAY);
  glDrawArrays (GL_TRIANGLE_STRIP, 0, priv->buffer.vertex_count);
#if 0
  glPushAttrib (GL_CURRENT_BIT);
  glColor4f (1., 1., 1., 1.);
  glDrawArrays (GL_LINE_STRIP, 0, priv->buffer.vertex_count);
  glPopAttrib ();
#endif
  glDisableClientState (GL_VERTEX_ARRAY);
  glDisableClientState (GL_TEXTURE_COORD_ARRAY);

  hidgl_reset_triangle_array (hidgl);
}

void
hidgl_ensure_vertex_space (hidGC gc, unsigned int count)
{
  hidglGC hidgl_gc = (hidglGC)gc;
  hidgl_instance *hidgl = hidgl_gc->hidgl;
  hidgl_priv *priv = hidgl->priv;

  CHECK_IS_IN_CONTEXT ();

  if (count > 3 * TRIANGLE_ARRAY_SIZE)
    {
      fprintf (stderr, "Not enough space in vertex buffer\n");
      fprintf (stderr, "Requested %i vertices, %i available\n",
                       count, 3 * TRIANGLE_ARRAY_SIZE);
      exit (1);
    }
  if (count > 3 * TRIANGLE_ARRAY_SIZE - priv->buffer.vertex_count)
    hidgl_flush_triangles (hidgl);
}

void
hidgl_ensure_triangle_space (hidGC gc, unsigned int count)
{
  CHECK_IS_IN_CONTEXT ();

  /* NB: 5 = 3 + 2 extra vertices to separate from other triangle strips */
  hidgl_ensure_vertex_space (gc, count * 5);
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

static void draw_cap (hidGC gc, Coord width, Coord x, Coord y, Angle angle)
{
  float radius = width / 2.;

  CHECK_IS_IN_CONTEXT ();

  hidgl_ensure_vertex_space (gc, 6);

  /* FIXME: Should draw an offset rectangle at the appropriate angle,
   *        avoiding relying on the subcompositing between layers to
   *        stop us creating an artaefact by drawing a full circle.
   */
  /* NB: Repeated first virtex to separate from other tri-strip */
  hidgl_add_vertex_tex (gc, x - radius, y - radius, -1.0, -1.0);
  hidgl_add_vertex_tex (gc, x - radius, y - radius, -1.0, -1.0);
  hidgl_add_vertex_tex (gc, x - radius, y + radius, -1.0,  1.0);
  hidgl_add_vertex_tex (gc, x + radius, y - radius,  1.0, -1.0);
  hidgl_add_vertex_tex (gc, x + radius, y + radius,  1.0,  1.0);
  hidgl_add_vertex_tex (gc, x + radius, y + radius,  1.0,  1.0);
  /* NB: Repeated last virtex to separate from other tri-strip */
}

void
hidgl_draw_line (hidGC gc, int cap, Coord width, Coord x1, Coord y1, Coord x2, Coord y2, double scale)
{
  double deltax, deltay, length;
  float wdx, wdy;
  float cosine, sine;
  int circular_caps = 0;
  int hairline = 0;

  CHECK_IS_IN_CONTEXT ();
  if (width == 0.0)
    hairline = 1;

  if (width < scale)
    width = scale;

  deltax = x2 - x1;
  deltay = y2 - y1;

  length = hypot (deltax, deltay);

  if (length == 0) {
    /* Assume the orientation of the line is horizontal */
    cosine = 1.0;
    sine   = 0.0;
  } else {
    cosine = deltax / length;
    sine   = deltay / length;
  }

  wdy =  width / 2. * cosine;
  wdx = -width / 2. * sine;

  switch (cap) {
    case Trace_Cap:
    case Round_Cap:
      circular_caps = 1;
      break;

    case Square_Cap:
    case Beveled_Cap:
      /* Use wdx and wdy (which already have the correct numbers), just in
       * case the compiler doesn't spot it can avoid recomputing these. */
      x1 -= wdy; /* x1 -= width / 2. * cosine;   */
      y1 += wdx; /* y1 -= width / 2. * sine;     */
      x2 += wdy; /* x2 += width / 2. * cosine;   */
      y2 -= wdx; /* y2 += width / 2. / sine;     */
      break;
  }

  /* Don't bother capping hairlines */
  if (circular_caps && !hairline)
    {
      if (length == 0)
        {
          hidgl_fill_circle (gc, x1, y1, width / 2.);
        }
      else
        {
          float capx = deltax * width / 2. / length;
          float capy = deltay * width / 2. / length;

          hidgl_ensure_vertex_space (gc, 10);

          /* NB: Repeated first virtex to separate from other tri-strip */
          hidgl_add_vertex_tex (gc, x1 - wdx - capx, y1 - wdy - capy, -1.0, -1.0);
          hidgl_add_vertex_tex (gc, x1 - wdx - capx, y1 - wdy - capy, -1.0, -1.0);
          hidgl_add_vertex_tex (gc, x1 + wdx - capx, y1 + wdy - capy, -1.0,  1.0);
          hidgl_add_vertex_tex (gc, x1 - wdx,        y1 - wdy,         0.0, -1.0);
          hidgl_add_vertex_tex (gc, x1 + wdx,        y1 + wdy,         0.0,  1.0);

          hidgl_add_vertex_tex (gc, x2 - wdx,        y2 - wdy,         0.0, -1.0);
          hidgl_add_vertex_tex (gc, x2 + wdx,        y2 + wdy,         0.0,  1.0);
          hidgl_add_vertex_tex (gc, x2 - wdx + capx, y2 - wdy + capy,  1.0, -1.0);
          hidgl_add_vertex_tex (gc, x2 + wdx + capx, y2 + wdy + capy,  1.0,  1.0);
          hidgl_add_vertex_tex (gc, x2 + wdx + capx, y2 + wdy + capy,  1.0,  1.0);
          /* NB: Repeated last virtex to separate from other tri-strip */
        }
    }
  else
    {
      hidgl_ensure_vertex_space (gc, 6);

      /* NB: Repeated first virtex to separate from other tri-strip */
      hidgl_add_vertex_tex (gc, x1 - wdx, y1 - wdy, 0.0, -1.0);
      hidgl_add_vertex_tex (gc, x1 - wdx, y1 - wdy, 0.0, -1.0);
      hidgl_add_vertex_tex (gc, x1 + wdx, y1 + wdy, 0.0,  1.0);

      hidgl_add_vertex_tex (gc, x2 - wdx, y2 - wdy, 0.0, -1.0);
      hidgl_add_vertex_tex (gc, x2 + wdx, y2 + wdy, 0.0,  1.0);
      hidgl_add_vertex_tex (gc, x2 + wdx, y2 + wdy, 0.0,  1.0);
      /* NB: Repeated last virtex to separate from other tri-strip */
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

  CHECK_IS_IN_CONTEXT ();
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
                       start_angle);
  draw_cap (gc, width, x + rx * -cosf (start_angle_rad + delta_angle_rad),
                       y + rx *  sinf (start_angle_rad + delta_angle_rad),
                       start_angle + delta_angle + 180.);
}

void
hidgl_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  hidglGC hidgl_gc = (hidglGC)gc;

  CHECK_IS_IN_CONTEXT ();

  glBegin (GL_LINE_LOOP);
  glVertex3f (x1, y1, hidgl_gc->depth);
  glVertex3f (x1, y2, hidgl_gc->depth);
  glVertex3f (x2, y2, hidgl_gc->depth);
  glVertex3f (x2, y1, hidgl_gc->depth);
  glEnd ();
}


void
hidgl_fill_circle (hidGC gc, Coord x, Coord y, Coord radius)
{
  CHECK_IS_IN_CONTEXT ();

  hidgl_ensure_vertex_space (gc, 6);

  /* NB: Repeated first virtex to separate from other tri-strip */
  hidgl_add_vertex_tex (gc, x - radius, y - radius, -1.0, -1.0);
  hidgl_add_vertex_tex (gc, x - radius, y - radius, -1.0, -1.0);
  hidgl_add_vertex_tex (gc, x - radius, y + radius, -1.0,  1.0);
  hidgl_add_vertex_tex (gc, x + radius, y - radius,  1.0, -1.0);
  hidgl_add_vertex_tex (gc, x + radius, y + radius,  1.0,  1.0);
  hidgl_add_vertex_tex (gc, x + radius, y + radius,  1.0,  1.0);
  /* NB: Repeated last virtex to separate from other tri-strip */
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

  CHECK_IS_IN_CONTEXT ();

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

static inline void
stash_vertex (PLINE *contour, int *vertex_comp,
              float x, float y, float z, float r, float s)
{
  contour->tristrip_vertices[(*vertex_comp)++] = x;
  contour->tristrip_vertices[(*vertex_comp)++] = y;
#if MEMCPY_VERTEX_DATA
  contour->tristrip_vertices[(*vertex_comp)++] = z;
  contour->tristrip_vertices[(*vertex_comp)++] = r;
  contour->tristrip_vertices[(*vertex_comp)++] = s;
#endif
  contour->tristrip_num_vertices ++;
}

static void
fill_contour (hidGC gc, PLINE *contour)
{
  hidglGC hidgl_gc = (hidglGC)gc;
#if MEMCPY_VERTEX_DATA
  hidgl_instance *hidgl = hidgl_gc->hidgl;
  hidgl_priv *priv = hidgl->priv;
#endif
  int i;
  int vertex_comp;
  borast_traps_t traps;

  /* If the contour is round, then call hidgl_fill_circle to draw it. */
#if 0
  if (contour->is_round) {
    hidgl_fill_circle (gc, contour->cx, contour->cy, contour->radius);
    return;
  }
#endif

  /* If we don't have a cached set of tri-strips, compute them */
  if (contour->tristrip_vertices == NULL) {
    int tristrip_space;
    int x1, x2, x3, x4, y_top, y_bot;

    _borast_traps_init (&traps);
    bo_contour_to_traps_no_draw (contour, &traps);

    tristrip_space = 0;

    for (i = 0; i < traps.num_traps; i++) {
      y_top = traps.traps[i].top;
      y_bot = traps.traps[i].bottom;

      x1 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_top);
      x2 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_top);
      x3 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_bot);
      x4 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_bot);

      if ((x1 == x2) || (x3 == x4)) {
        tristrip_space += 5; /* Three vertices + repeated start and end */
      } else {
        tristrip_space += 6; /* Four vertices + repeated start and end */
      }
    }

    if (tristrip_space == 0) {
      printf ("Strange, contour didn't tesselate\n");
      return;
    }

#if MEMCPY_VERTEX_DATA
    /* NB: MEMCPY of vertex data causes a problem with depth being cached at the wrong level! */
    contour->tristrip_vertices = malloc (sizeof (float) * 5 * tristrip_space);
#else
    contour->tristrip_vertices = malloc (sizeof (float) * 2 * tristrip_space);
#endif
    contour->tristrip_num_vertices = 0;

    vertex_comp = 0;
    for (i = 0; i < traps.num_traps; i++) {
      y_top = traps.traps[i].top;
      y_bot = traps.traps[i].bottom;

      x1 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_top);
      x2 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_top);
      x3 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_bot);
      x4 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_bot);

      if (x1 == x2) {
        /* NB: Repeated first virtex to separate from other tri-strip */
        stash_vertex (contour, &vertex_comp, x1, y_top, hidgl_gc->depth, 0.0, 0.0);
        stash_vertex (contour, &vertex_comp, x1, y_top, hidgl_gc->depth, 0.0, 0.0);
        stash_vertex (contour, &vertex_comp, x3, y_bot, hidgl_gc->depth, 0.0, 0.0);
        stash_vertex (contour, &vertex_comp, x4, y_bot, hidgl_gc->depth, 0.0, 0.0);
        stash_vertex (contour, &vertex_comp, x4, y_bot, hidgl_gc->depth, 0.0, 0.0);
        /* NB: Repeated last virtex to separate from other tri-strip */
      } else if (x3 == x4) {
        /* NB: Repeated first virtex to separate from other tri-strip */
        stash_vertex (contour, &vertex_comp, x1, y_top, hidgl_gc->depth, 0.0, 0.0);
        stash_vertex (contour, &vertex_comp, x1, y_top, hidgl_gc->depth, 0.0, 0.0);
        stash_vertex (contour, &vertex_comp, x2, y_top, hidgl_gc->depth, 0.0, 0.0);
        stash_vertex (contour, &vertex_comp, x3, y_bot, hidgl_gc->depth, 0.0, 0.0);
        stash_vertex (contour, &vertex_comp, x3, y_bot, hidgl_gc->depth, 0.0, 0.0);
        /* NB: Repeated last virtex to separate from other tri-strip */
      } else {
        /* NB: Repeated first virtex to separate from other tri-strip */
        stash_vertex (contour, &vertex_comp, x2, y_top, hidgl_gc->depth, 0.0, 0.0);
        stash_vertex (contour, &vertex_comp, x2, y_top, hidgl_gc->depth, 0.0, 0.0);
        stash_vertex (contour, &vertex_comp, x3, y_bot, hidgl_gc->depth, 0.0, 0.0);
        stash_vertex (contour, &vertex_comp, x1, y_top, hidgl_gc->depth, 0.0, 0.0);
        stash_vertex (contour, &vertex_comp, x4, y_bot, hidgl_gc->depth, 0.0, 0.0);
        stash_vertex (contour, &vertex_comp, x4, y_bot, hidgl_gc->depth, 0.0, 0.0);
        /* NB: Repeated last virtex to separate from other tri-strip */
      }
    }

    _borast_traps_fini (&traps);
  }

  if (contour->tristrip_num_vertices == 0)
    return;

  hidgl_ensure_vertex_space (gc, contour->tristrip_num_vertices);

#if MEMCPY_VERTEX_DATA
  memcpy (&priv->buffer.triangle_array[priv->buffer.coord_comp_count],
          contour->tristrip_vertices,
          sizeof (float) * 5 * contour->tristrip_num_vertices);
  priv->buffer.coord_comp_count += 5 * contour->tristrip_num_vertices;
  priv->buffer.vertex_count += contour->tristrip_num_vertices;

#else
  vertex_comp = 0;
  for (i = 0; i < contour->tristrip_num_vertices; i++) {
    int x, y;
    x = contour->tristrip_vertices[vertex_comp++];
    y = contour->tristrip_vertices[vertex_comp++];
    hidgl_add_vertex_tex (gc, x, y, 0.0, 0.0);
  }
#endif

}

static int
do_hole (const BoxType *b, void *cl)
{
  PLINE *curc = (PLINE *) b;
  hidGC gc = (hidGC)cl;

  /* Ignore the outer contour - we draw it first explicitly*/
  if (curc->Flags.orient == PLF_DIR) {
    return 0;
  }

  fill_contour (gc, curc);
  return 1;
}

static bool
polygon_contains_user_holes (PolygonType *polygon)
{
  return (polygon->HoleIndexN > 0);
}

static void
fill_polyarea (hidGC gc, POLYAREA *pa, const BoxType *clip_box, bool use_new_stencil)
{
  hidglGC hidgl_gc = (hidglGC)gc;
  hidgl_instance *hidgl = hidgl_gc->hidgl;
  hidgl_priv *priv = hidgl->priv;
  int stencil_bit;

  CHECK_IS_IN_CONTEXT ();

  /* Special case non-holed polygons which don't require a stencil bit */
  if (pa->contour_tree->size == 1) {
    fill_contour (gc, pa->contours);
    return;
  }

  /* Polygon has holes.. does it have any user-drawn holes? (caller tells us)
   * If so, it must be masked with a _new_ stencil bit.
   */
  if (use_new_stencil)
    {
      stencil_bit = hidgl_assign_clear_stencil_bit (hidgl);
      if (!stencil_bit)
        {
          printf ("hidgl_fill_pcb_polygon: No free stencil bits, aborting polygon\n");
          /* XXX: Could use the GLU tesselator or the full BO polygon tesselator */
          return;
        }
    }

  /* Flush out any existing geoemtry to be rendered */
  hidgl_flush_triangles (hidgl);

  glPushAttrib (GL_STENCIL_BUFFER_BIT |                 /* Resave the stencil write-mask etc.., and */
                GL_COLOR_BUFFER_BIT |                   /* the colour buffer write mask etc.. for part way restore */
                GL_DEPTH_BUFFER_BIT);
  glEnable (GL_STENCIL_TEST);                           /* Enable the stencil test, just in case it wasn't already on */
//=======
//<<<<<<< current
//                GL_COLOR_BUFFER_BIT);                   /* the colour buffer write mask etc.. for part way restore */
//  glEnable (GL_STENCIL_TEST);                           /* Enable the stencil test, just in case it wasn't already on */
//=======
//                GL_COLOR_BUFFER_BIT |                   /* the colour buffer write mask etc.. for part way restore */
//                GL_DEPTH_BUFFER_BIT);
//>>>>>>> patched
  glColorMask (0, 0, 0, 0);                             /* Disable writting in color buffer */
  glDepthFunc (GL_ALWAYS);
  glDepthMask (GL_FALSE);

  if (use_new_stencil)
    {
      glStencilMask (stencil_bit);                            /* Only write to our stencil bit */
      glStencilFunc (GL_ALWAYS, stencil_bit, stencil_bit);    /* Always pass stencil test, ref value is our bit */
      glStencilOp (GL_KEEP, GL_KEEP, GL_REPLACE);             /* Stencil pass => replace stencil value */
    }

  /* Drawing operations now set our reference bit in the stencil buffer */

  r_search (pa->contour_tree, clip_box, NULL, do_hole, gc);
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
  fill_contour (gc, pa->contours);
  hidgl_flush_triangles (hidgl);

  /* Unassign our stencil buffer bit */
  if (use_new_stencil)
    hidgl_return_stencil_bit (hidgl, stencil_bit);

  glPopAttrib ();                               /* Restore the stencil buffer op and function */
}

void
hidgl_fill_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box)
{
  bool use_new_stencil;

  if (poly->Clipped == NULL)
    return;

  use_new_stencil = polygon_contains_user_holes (poly) ||
                    TEST_FLAG (FULLPOLYFLAG, poly);

  fill_polyarea (gc, poly->Clipped, clip_box, use_new_stencil);

  if (TEST_FLAG (FULLPOLYFLAG, poly))
    {
      POLYAREA *pa;

      for (pa = poly->Clipped->f; pa != poly->Clipped; pa = pa->f)
        fill_polyarea (gc, pa, clip_box, use_new_stencil);
    }
}

void
hidgl_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  CHECK_IS_IN_CONTEXT ();

  hidgl_ensure_vertex_space (gc, 6);

  /* NB: Repeated first virtex to separate from other tri-strip */
  hidgl_add_vertex_tex (gc, x1, y1, 0.0, 0.0);
  hidgl_add_vertex_tex (gc, x1, y1, 0.0, 0.0);
  hidgl_add_vertex_tex (gc, x1, y2, 0.0, 0.0);
  hidgl_add_vertex_tex (gc, x2, y1, 0.0, 0.0);
  hidgl_add_vertex_tex (gc, x2, y2, 0.0, 0.0);
  hidgl_add_vertex_tex (gc, x2, y2, 0.0, 0.0);
  /* NB: Repeated last virtex to separate from other tri-strip */
}

static void
load_built_in_shaders (void)
{
  char *circular_fs_source =
          "void main()\n"
          "{\n"
          "  float sqdist;\n"
          "  sqdist = dot (gl_TexCoord[0].st, gl_TexCoord[0].st);\n"
          "  if (sqdist > 1.0)\n"
          "    discard;\n"
          "  gl_FragColor = gl_Color;\n"
          "}\n";

  char *resistor_fs_source =
          "uniform sampler1D detail_tex;\n"
          "uniform sampler2D bump_tex;\n"
          "\n"
          "void main()\n"
          "{\n"
          "  vec3 bumpNormal = texture2D (bump_tex, gl_TexCoord[1].st).rgb;\n"
          "  vec3 detailColor = texture1D (detail_tex, gl_TexCoord[0].s).rgb;\n"
          "\n"
          "  /* Uncompress vectors ([0, 1] -> [-1, 1]) */\n"
          "  vec3 lightVectorFinal = -1.0 + 2.0 * gl_Color.rgb;\n"
          "  vec3 halfVectorFinal = -1.0 + 2.0 * gl_TexCoord[2].xyz;\n"
          "  vec3 bumpNormalVectorFinal = -1.0 + 2.0 * bumpNormal;\n"
          "\n"
          "  /* Compute diffuse factor */\n"
          "  float diffuse = clamp(dot(bumpNormalVectorFinal,\n"
          "                            lightVectorFinal),0.0, 1.0);\n"
          "  float specular = pow(clamp(dot(bumpNormalVectorFinal,\n"
          "                                 halfVectorFinal), 0.0, 1.0),\n"
          "                       2.0);\n"
          "  specular *= 0.4;\n"
          "\n"
          "   gl_FragColor = vec4(detailColor * (0.3 + 0.7 * diffuse) + \n"
          "                    vec3(specular, specular, specular), 1.0);\n"
          "}\n";

  /*priv->*/circular_program = hidgl_shader_new ("circular_rendering", NULL, circular_fs_source);
  /*priv->*/resistor_program = hidgl_shader_new ("resistor_rendering", NULL, resistor_fs_source);
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

#ifdef WIN32
  glGenBuffers        = (PFNGLGENBUFFERSPROC)        wglGetProcAddress ("glGenBuffers");
  glDeleteBuffers     = (PFNGLDELETEBUFFERSPROC)     wglGetProcAddress ("glDeleteBuffers");
  glBindBuffer        = (PFNGLBINDBUFFERPROC)        wglGetProcAddress ("glBindBuffer");
  glBufferData        = (PFNGLBUFFERDATAPROC)        wglGetProcAddress ("glBufferData");
  glBufferSubData     = (PFNGLBUFFERSUBDATAPROC)     wglGetProcAddress ("glBufferSubData");
  glMapBuffer         = (PFNGLMAPBUFFERPROC)         wglGetProcAddress ("glMapBuffer");
  glUnmapBuffer       = (PFNGLUNMAPBUFFERPROC)       wglGetProcAddress ("glUnmapBuffer");

  glAttachShader      = (PFNGLATTACHSHADERPROC)      wglGetProcAddress ("glAttachShader");
  glCompileShader     = (PFNGLCOMPILESHADERPROC)     wglGetProcAddress ("glCompileShader");
  glCreateProgram     = (PFNGLCREATEPROGRAMPROC)     wglGetProcAddress ("glCreateProgram");
  glCreateShader      = (PFNGLCREATESHADERPROC)      wglGetProcAddress ("glCreateShader");
  glDeleteProgram     = (PFNGLDELETEPROGRAMPROC)     wglGetProcAddress ("glDeleteProgram");
  glDeleteShader      = (PFNGLDELETESHADERPROC)      wglGetProcAddress ("glDeleteShader");
  glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC) wglGetProcAddress ("glGetProgramInfoLog");
  glGetProgramiv      = (PFNGLGETPROGRAMIVPROC)      wglGetProcAddress ("glGetProgramiv");
  glGetShaderInfoLog  = (PFNGLGETSHADERINFOLOGPROC)  wglGetProcAddress ("glGetShaderInfoLog");
  glGetShaderiv       = (PFNGLGETSHADERIVPROC)       wglGetProcAddress ("glGetShaderiv");
  glIsShader          = (PFNGLISSHADERPROC)          wglGetProcAddress ("glIsShader");
  glLinkProgram       = (PFNGLLINKPROGRAMPROC)       wglGetProcAddress ("glLinkProgram");
  glShaderSource      = (PFNGLSHADERSOURCEPROC)      wglGetProcAddress ("glShaderSource");
  glUseProgram        = (PFNGLUSEPROGRAMPROC)        wglGetProcAddress ("glUseProgram");

  glMultiTexCoord1f    = (PFNGLMULTITEXCOORD1FPROC)    wglGetProcAddress ("glMultiTexCoord1f");
  glMultiTexCoord2f    = (PFNGLMULTITEXCOORD2FPROC)    wglGetProcAddress ("glMultiTexCoord2f");
  glMultiTexCoord3f    = (PFNGLMULTITEXCOORD3FPROC)    wglGetProcAddress ("glMultiTexCoord3f");
  glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC) wglGetProcAddress ("glGetUniformLocation");
  glUniform1i          = (PFNGLUNIFORM1IPROC)          wglGetProcAddress ("glUniform1i");
  glActiveTextureARB   = (PFNGLACTIVETEXTUREARBPROC)   wglGetProcAddress ("glActiveTextureARB");

#endif

#if 0 /* Need to initialise shaders with a current GL context */
  if (hidgl_shader_init_shaders ())
    load_built_in_shaders ();
  else
    printf ("Failed to initialise shader support\n");
#endif

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
//  hidgl_init_triangle_array (hidgl);

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

  CHECK_IS_IN_CONTEXT ();

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
  static bool called = false;

  if (in_context)
    fprintf (stderr, "hidgl: hidgl_start_render() - Already in rendering context!\n");

  in_context = true;

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

#if 1 /* Need to initialise shaders with a current GL context */
  if (!called)
    {
      if (hidgl_shader_init_shaders ())
        load_built_in_shaders ();
      else
        printf ("Failed to initialise shader support\n");
      called = true;
    }
#endif

  hidgl_init_triangle_array (hidgl);
  hidgl_shader_activate (/*priv->*/circular_program);
}

void
hidgl_finish_render (hidgl_instance *hidgl)
{
  if (!in_context)
    fprintf (stderr, "hidgl: hidgl_finish_render() - Not currently in rendering context!\n");

  hidgl_finish_triangle_array (hidgl);
  hidgl_shader_activate (NULL);
  in_context = false;
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

  CHECK_IS_IN_CONTEXT ();

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
