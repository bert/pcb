/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2010 PCB Contributors (See ChangeLog for details).
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

#ifndef __HIDGL_SHADERS_INCLUDED__
#define __HIDGL_SHADERS_INCLUDED__

typedef struct _hidgl_shader hidgl_shader;

bool hidgl_shader_init_shaders (void);

hidgl_shader *hidgl_shader_new (char *name, char *vs_source, char *fs_source);
GLuint hidgl_shader_get_program (hidgl_shader *shader);
void hidgl_shader_free (hidgl_shader *shader);
void hidgl_shader_activate (hidgl_shader *shader);

#endif /* __HIDGL_SHADERS_INCLUDED__  */
