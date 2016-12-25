#include <glib.h>

#include "appearance.h"


#ifndef WIN32
/* The Linux OpenGL ABI 1.0 spec requires that we define
 * GL_GLEXT_PROTOTYPES before including gl.h or glx.h for extensions
 * in order to get prototypes:
 *   http://www.opengl.org/registry/ABI/
 */
#   define GL_GLEXT_PROTOTYPES 1
#endif

#ifdef HAVE_OPENGL_GL_H
#   include <OpenGL/gl.h>
#else
#   include <GL/gl.h>
#endif



appearance
*make_appearance (void)
{
  appearance *appear = g_new0 (appearance, 1);

  appear->a = 1.0f;

  return appear;
}

void
destroy_appearance (appearance *appear)
{
  g_free (appear);
}

void
appearance_set_color (appearance *appear, float r, float g, float b)
{
  appear->r = r;
  appear->g = g;
  appear->b = b;
}

void
appearance_set_alpha (appearance *appear, float a)
{
  appear->a = a;
}

void
appearance_set_appearance (appearance *appear, const appearance *from)
{
  *appear = *from;
}

void
appearance_apply_gl (appearance *appear)
{
//  glColor3f (appear->r, appear->g, appear->b);
  glColor4f (appear->r, appear->g, appear->b, appear->a);
}
