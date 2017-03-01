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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* The Linux OpenGL ABI 1.0 spec requires that we define
 * GL_GLEXT_PROTOTYPES before including gl.h or glx.h for extensions
 * in order to get prototypes:
 *   http://www.opengl.org/registry/ABI/
 */
#define GL_GLEXT_PROTOTYPES 1

/* This follows autoconf's recommendation for the AX_CHECK_GL macro
   https://www.gnu.org/software/autoconf-archive/ax_check_gl.html */
#if defined HAVE_WINDOWS_H && defined _WIN32
#  define WIN32_LEAN_AND_MEAN
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

#include "hidgl_shaders.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#ifdef WIN32
#   include "glext.h"

extern PFNGLGENBUFFERSPROC         glGenBuffers;
extern PFNGLDELETEBUFFERSPROC      glDeleteBuffers;
extern PFNGLBINDBUFFERPROC         glBindBuffer;
extern PFNGLBUFFERDATAPROC         glBufferData;
extern PFNGLBUFFERSUBDATAPROC      glBufferSubData;
extern PFNGLMAPBUFFERPROC          glMapBuffer;
extern PFNGLUNMAPBUFFERPROC        glUnmapBuffer;

extern PFNGLATTACHSHADERPROC       glAttachShader;
extern PFNGLCOMPILESHADERPROC      glCompileShader;
extern PFNGLCREATEPROGRAMPROC      glCreateProgram;
extern PFNGLCREATESHADERPROC       glCreateShader;
extern PFNGLDELETEPROGRAMPROC      glDeleteProgram;
extern PFNGLDELETESHADERPROC       glDeleteShader;
extern PFNGLGETPROGRAMINFOLOGPROC  glGetProgramInfoLog;
extern PFNGLGETPROGRAMIVPROC       glGetProgramiv;
extern PFNGLGETSHADERINFOLOGPROC   glGetShaderInfoLog;
extern PFNGLGETSHADERIVPROC        glGetShaderiv;
extern PFNGLISSHADERPROC           glIsShader;
extern PFNGLLINKPROGRAMPROC        glLinkProgram;
extern PFNGLSHADERSOURCEPROC       glShaderSource;
extern PFNGLUSEPROGRAMPROC         glUseProgram;
#endif

/* Opaque data-structure keeping a shader object */
struct _hidgl_shader {
  char *name;
  GLuint program;
  GLuint vs;
  GLuint fs;
};

bool
hidgl_shader_init_shaders (void)
{
  /* XXX: Check for required functionality in the GL driver */
  return true;
}

/* From http://gpwiki.org/index.php/OpenGL:Codes:Simple_GLSL_example */
static void
print_log (GLuint obj)
{
  int infologLength = 0;
  int maxLength;
  char *infoLog;

  if (glIsShader (obj))
    glGetShaderiv (obj, GL_INFO_LOG_LENGTH, &maxLength);
  else
    glGetProgramiv (obj, GL_INFO_LOG_LENGTH, &maxLength);

  infoLog = malloc (maxLength);

  if (glIsShader (obj))
    glGetShaderInfoLog (obj, maxLength, &infologLength, infoLog);
  else
    glGetProgramInfoLog (obj, maxLength, &infologLength, infoLog);

  if (infologLength > 0)
    printf ("%s\n", infoLog);

  free (infoLog);
}


/* If either vs or fs is NULL, used the fixed function pipeline for that */
hidgl_shader *
hidgl_shader_new (char *name, char *vs_source, char *fs_source)
{
  hidgl_shader *shader;
  const char *source;
  int source_len;

  shader = malloc (sizeof (hidgl_shader));

  if (shader == NULL)
    return NULL;

  shader->name = strdup (name);
  shader->program = 0;
  shader->vs = 0;
  shader->fs = 0;

  if (fs_source == NULL && vs_source == NULL)
    return shader;

  shader->program = glCreateProgram ();

  if (vs_source != NULL) {
    source = vs_source;
    source_len = -1; /* The string is '\0' terminated */
    shader->vs = glCreateShader (GL_VERTEX_SHADER);
    glShaderSource (shader->vs, 1, &source, &source_len);
    glCompileShader (shader->vs);
    print_log (shader->vs);
    glAttachShader (shader->program, shader->vs);
  }

  if (fs_source != NULL) {
    source = fs_source;
    source_len = -1; /* The string is '\0' terminated */
    shader->fs = glCreateShader (GL_FRAGMENT_SHADER);
    glShaderSource (shader->fs, 1, &source, &source_len);
    glCompileShader (shader->fs);
    print_log (shader->fs);
    glAttachShader (shader->program, shader->fs);
  }

  glLinkProgram (shader->program);
  print_log (shader->program);
  return shader;
}


GLuint
hidgl_shader_get_program (hidgl_shader *shader)
{
  return shader->program;
}


/* Delete the passed shader. */
void
hidgl_shader_free (hidgl_shader *shader)
{
  /* NB: These calls all silently ignore 0 or NULL arguments */
  glDeleteShader (shader->vs);
  glDeleteShader (shader->fs);
  glDeleteProgram (shader->program);
  free (shader->name);
  free (shader);
}


/* Activate the given shader program, or deactivate if NULL passed */
void
hidgl_shader_activate (hidgl_shader *shader)
{
  if (shader == NULL)
    glUseProgram (0);
  else
    glUseProgram (shader->program);
}
