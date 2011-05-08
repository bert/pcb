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

#ifndef __HIDGL_INCLUDED__
#define __HIDGL_INCLUDED__

#define TRIANGLE_ARRAY_SIZE 5461
typedef struct {
  GLfloat triangle_array [3 * 3 * TRIANGLE_ARRAY_SIZE];
  unsigned int triangle_count;
  unsigned int coord_comp_count;
} triangle_buffer;

extern triangle_buffer buffer;
extern float global_depth;

void hidgl_init_triangle_array (triangle_buffer *buffer);
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
void hidgl_draw_line (int cap, double width, int x1, int y1, int x2, int y2, double scale);
void hidgl_draw_arc (double width, int vx, int vy, int vrx, int vry, int start_angle, int delta_angle, double scale);
void hidgl_draw_rect (int x1, int y1, int x2, int y2);
void hidgl_fill_circle (int vx, int vy, int vr, double scale);
void hidgl_fill_polygon (int n_coords, int *x, int *y);
void hidgl_fill_pcb_polygon (PolygonType *poly, const BoxType *clip_box, double scale);
void hidgl_fill_rect (int x1, int y1, int x2, int y2);

void hidgl_init (void);
int hidgl_stencil_bits (void);
int hidgl_assign_clear_stencil_bit (void);
void hidgl_return_stencil_bit (int bit);
void hidgl_reset_stencil_usage (void);

#endif /* __HIDGL_INCLUDED__  */
