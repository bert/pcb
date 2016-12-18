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

#include "hidgl_shaders.h"

#ifdef WIN32
#define _GLUfuncptr void *
#endif

/* NB: triangle_buffer is a private type, only defined here to enable inlining of geometry creation */
#define TRIANGLE_ARRAY_SIZE 30000
typedef struct {
  GLfloat *triangle_array;
  unsigned int triangle_count;
  unsigned int coord_comp_count;
  unsigned int vertex_count;
  GLuint vbo_id;
  bool use_vbo;
  bool use_map;
} triangle_buffer;

/* NB: hidgl_priv is a private type, only defined here to enable inlining of geometry creation */
typedef struct {
  /* Triangle management */
  triangle_buffer buffer;

  /* Stencil management */
  GLint stencil_bits;
  int dirty_bits;
  int assigned_bits;

  /* Shaders */
//  hidgl_shader *circular_program;

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

extern hidgl_shader *circular_program;
extern hidgl_shader *resistor_program;

void hidgl_flush_triangles (hidgl_instance *hidgl);
void hidgl_ensure_vertex_space (hidGC gc, unsigned int count);
void hidgl_ensure_triangle_space (hidGC gc, unsigned int count);


static inline void
hidgl_add_vertex_3D_tex (hidGC gc,
                         GLfloat x, GLfloat y, GLfloat z,
                         GLfloat s, GLfloat t)
{
  hidglGC hidgl_gc = (hidglGC)gc;
  hidgl_instance *hidgl = hidgl_gc->hidgl;
  hidgl_priv *priv = hidgl->priv;

  priv->buffer.triangle_array [priv->buffer.coord_comp_count++] = x;
  priv->buffer.triangle_array [priv->buffer.coord_comp_count++] = y;
  priv->buffer.triangle_array [priv->buffer.coord_comp_count++] = z;
  priv->buffer.triangle_array [priv->buffer.coord_comp_count++] = s;
  priv->buffer.triangle_array [priv->buffer.coord_comp_count++] = t;
  priv->buffer.vertex_count++;
}

static inline void
hidgl_add_vertex_tex (hidGC gc,
                      GLfloat x, GLfloat y,
                      GLfloat s, GLfloat t)
{
  hidglGC hidgl_gc = (hidglGC)gc;

  hidgl_add_vertex_3D_tex (gc, x, y, hidgl_gc->depth, s, t);
}

static inline void
hidgl_add_triangle_3D_tex (hidGC gc,
                           GLfloat x1, GLfloat y1, GLfloat z1, GLfloat s1, GLfloat t1,
                           GLfloat x2, GLfloat y2, GLfloat z2, GLfloat s2, GLfloat t2,
                           GLfloat x3, GLfloat y3, GLfloat z3, GLfloat s3, GLfloat t3)
{
  /* NB: Repeated first virtex to separate from other tri-strip */
  hidgl_add_vertex_3D_tex (gc, x1, y1, z1, s1, t1);
  hidgl_add_vertex_3D_tex (gc, x1, y1, z1, s1, t1);
  hidgl_add_vertex_3D_tex (gc, x2, y2, z2, s2, t2);
  hidgl_add_vertex_3D_tex (gc, x3, y3, z3, s3, t3);
  hidgl_add_vertex_3D_tex (gc, x3, y3, z3, s3, t3);
  /* NB: Repeated last virtex to separate from other tri-strip */
}

static inline void
hidgl_add_triangle_3D (hidGC gc,
                       GLfloat x1, GLfloat y1, GLfloat z1,
                       GLfloat x2, GLfloat y2, GLfloat z2,
                       GLfloat x3, GLfloat y3, GLfloat z3)
{
  hidgl_add_triangle_3D_tex (gc, x1, y1, z1, 0., 0.,
                                 x2, y2, z2, 0., 0.,
                                 x3, y3, z3, 0., 0.);
}

static inline void
hidgl_add_triangle_tex (hidGC gc,
                        GLfloat x1, GLfloat y1, GLfloat s1, GLfloat t1,
                        GLfloat x2, GLfloat y2, GLfloat s2, GLfloat t2,
                        GLfloat x3, GLfloat y3, GLfloat s3, GLfloat t3)
{
  hidglGC hidgl_gc = (hidglGC)gc;

  hidgl_add_triangle_3D_tex (gc, x1, y1, hidgl_gc->depth, s1, t1,
                                 x2, y2, hidgl_gc->depth, s2, t2,
                                 x3, y3, hidgl_gc->depth, s3, t3);
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
void hidgl_fill_circle (hidGC gc, Coord vx, Coord vy, Coord vr);
void hidgl_fill_polygon (hidGC gc, int n_coords, Coord *x, Coord *y);
void hidgl_fill_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box);
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

/* hidgl_pacakge_acy_resistor.c */
void hidgl_draw_acy_resistor (ElementType *element, float surface_depth, float board_thickness);
void hidgl_draw_800mil_resistor (ElementType *element, float surface_depth, float board_thickness);
void hidgl_draw_2300mil_resistor (ElementType *element, float surface_depth, float board_thickness);
void hidgl_draw_700mil_diode_smd (ElementType *element, float surface_depth, float board_thickness);
void hidgl_draw_1650mil_cap (ElementType *element, float surface_depth, float board_thickness);
void hidgl_draw_350x800mil_cap (ElementType *element, float surface_depth, float board_thickness);

#endif /* PCB_HID_COMMON_HIDGL_H  */
