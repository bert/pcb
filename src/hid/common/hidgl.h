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

#define TRIANGLE_ARRAY_SIZE 5461
typedef struct {
  GLfloat triangle_array [3 * 3 * TRIANGLE_ARRAY_SIZE];
  unsigned int triangle_count;
  unsigned int coord_comp_count;
} triangle_buffer;

extern triangle_buffer buffer;
extern float global_depth;

void hidgl_flush_triangles (triangle_buffer *buffer);
void hidgl_ensure_triangle_space (triangle_buffer *buffer, int count);

static inline void
hidgl_add_triangle_3D (triangle_buffer *buffer,
                       GLfloat x1, GLfloat y1, GLfloat z1,
                       GLfloat x2, GLfloat y2, GLfloat z2,
                       GLfloat x3, GLfloat y3, GLfloat z3)
{
  buffer->triangle_array [buffer->coord_comp_count++] = x1;
  buffer->triangle_array [buffer->coord_comp_count++] = y1;
  buffer->triangle_array [buffer->coord_comp_count++] = z1;
  buffer->triangle_array [buffer->coord_comp_count++] = x2;
  buffer->triangle_array [buffer->coord_comp_count++] = y2;
  buffer->triangle_array [buffer->coord_comp_count++] = z2;
  buffer->triangle_array [buffer->coord_comp_count++] = x3;
  buffer->triangle_array [buffer->coord_comp_count++] = y3;
  buffer->triangle_array [buffer->coord_comp_count++] = z3;
  buffer->triangle_count++;
}

static inline void
hidgl_add_triangle (triangle_buffer *buffer,
                    GLfloat x1, GLfloat y1,
                    GLfloat x2, GLfloat y2,
                    GLfloat x3, GLfloat y3)
{
  hidgl_add_triangle_3D (buffer, x1, y1, global_depth,
                                 x2, y2, global_depth,
                                 x3, y3, global_depth);
}

void hidgl_draw_grid (BoxType *drawn_area);
void hidgl_set_depth (float depth);
void hidgl_draw_line (int cap, Coord width, Coord x1, Coord y1, Coord x2, Coord y2, double scale);
void hidgl_draw_arc (Coord width, Coord vx, Coord vy, Coord vrx, Coord vry, Angle start_angle, Angle delta_angle, double scale);
void hidgl_draw_rect (Coord x1, Coord y1, Coord x2, Coord y2);
void hidgl_fill_circle (Coord vx, Coord vy, Coord vr, double scale);
void hidgl_fill_polygon (int n_coords, Coord *x, Coord *y);
void hidgl_fill_pcb_polygon (PolygonType *poly, const BoxType *clip_box, double scale);
void hidgl_fill_rect (Coord x1, Coord y1, Coord x2, Coord y2);

void hidgl_init (void);
void hidgl_start_render (void);
void hidgl_finish_render (void);
int hidgl_stencil_bits (void);
int hidgl_assign_clear_stencil_bit (void);
void hidgl_return_stencil_bit (int bit);
void hidgl_reset_stencil_usage (void);

#endif /* PCB_HID_COMMON_HIDGL_H  */
