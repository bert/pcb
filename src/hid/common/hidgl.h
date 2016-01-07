/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2009-2011 PCB Contributors (See ChangeLog for details).
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

#ifndef PCB_HID_COMMON_HIDGL_H
#define PCB_HID_COMMON_HIDGL_H

#ifdef WIN32
#define _GLUfuncptr void *
#endif

/* NB: triangle_buffer is a private type, only defined here to enable inlining of geometry creation */
#define TRIANGLE_ARRAY_SIZE 5461
typedef struct {
  GLfloat triangle_array [3 * 3 * TRIANGLE_ARRAY_SIZE];
  unsigned int triangle_count;
  unsigned int coord_comp_count;
} triangle_buffer;

/* NB: hidgl_priv is a private type, only defined here to enable inlining of geometry creation */
typedef struct {
  /* Triangle management */
  triangle_buffer buffer;

  /* Stencil management */
  GLint stencil_bits;
  int dirty_bits;
  int assigned_bits;

} hidgl_priv;

/* NB: hidgl_instance is a public type, intended to be used as an opaque pointer */
typedef struct {
  hidgl_priv *priv;

} hidgl_instance;

/* NB: hidglGC is a semi-private type, only defined here to enable inlining of geometry creation, and for derived GUIs to extend */
typedef struct hidgl_gc_struct {
  struct hid_gc_struct gc; /* Parent */

  hidgl_instance *hidgl;

  float depth;

} *hidglGC;

void hidgl_flush_triangles (hidgl_instance *hidgl);
void hidgl_ensure_triangle_space (hidGC gc, int count);


static inline void
hidgl_add_triangle_3D (hidGC gc,
                       GLfloat x1, GLfloat y1, GLfloat z1,
                       GLfloat x2, GLfloat y2, GLfloat z2,
                       GLfloat x3, GLfloat y3, GLfloat z3)
{
  hidglGC hidgl_gc = (hidglGC)gc;
  hidgl_instance *hidgl = hidgl_gc->hidgl;
  hidgl_priv *priv = hidgl->priv;

  priv->buffer.triangle_array [priv->buffer.coord_comp_count++] = x1;
  priv->buffer.triangle_array [priv->buffer.coord_comp_count++] = y1;
  priv->buffer.triangle_array [priv->buffer.coord_comp_count++] = z1;
  priv->buffer.triangle_array [priv->buffer.coord_comp_count++] = x2;
  priv->buffer.triangle_array [priv->buffer.coord_comp_count++] = y2;
  priv->buffer.triangle_array [priv->buffer.coord_comp_count++] = z2;
  priv->buffer.triangle_array [priv->buffer.coord_comp_count++] = x3;
  priv->buffer.triangle_array [priv->buffer.coord_comp_count++] = y3;
  priv->buffer.triangle_array [priv->buffer.coord_comp_count++] = z3;
  priv->buffer.triangle_count++;
}

static inline void
hidgl_add_triangle (hidGC gc,
                    GLfloat x1, GLfloat y1,
                    GLfloat x2, GLfloat y2,
                    GLfloat x3, GLfloat y3)
{
  hidglGC hidgl_gc = (hidglGC)gc;

  hidgl_add_triangle_3D (gc, x1, y1, hidgl_gc->depth,
                             x2, y2, hidgl_gc->depth,
                             x3, y3, hidgl_gc->depth);
}

void hidgl_draw_grid (hidGC gc, BoxType *drawn_area);
void hidgl_set_depth (hidGC gc, float depth);
void hidgl_draw_line (hidGC gc, int cap, Coord width, Coord x1, Coord y1, Coord x2, Coord y2, double scale);
void hidgl_draw_arc (hidGC gc, Coord width, Coord vx, Coord vy, Coord vrx, Coord vry, Angle start_angle, Angle delta_angle, double scale);
void hidgl_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);
void hidgl_fill_circle (hidGC gc, Coord vx, Coord vy, Coord vr, double scale);
void hidgl_fill_polygon (hidGC gc, int n_coords, Coord *x, Coord *y);
void hidgl_fill_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box, double scale);
void hidgl_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);

void hidgl_init (void);
hidgl_instance *hidgl_new_instance (void);
void hidgl_free_instance (hidgl_instance *hidgl);
void hidgl_init_gc (hidgl_instance *hidgl, hidGC gc);
void hidgl_finish_gc (hidGC gc);
void hidgl_start_render (hidgl_instance *hidgl);
void hidgl_finish_render (hidgl_instance *hidgl);
int hidgl_stencil_bits (hidgl_instance *hidgl);
int hidgl_assign_clear_stencil_bit (hidgl_instance *hidgl);
void hidgl_return_stencil_bit (hidgl_instance *hidgl, int bit);
void hidgl_reset_stencil_usage (hidgl_instance *hidgl);

#endif /* PCB_HID_COMMON_HIDGL_H  */
