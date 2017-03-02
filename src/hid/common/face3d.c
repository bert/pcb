#include <glib.h>

#include "quad.h"
#include "face3d.h"

face3d *
make_face3d (void)
{
  face3d *face;

  face = g_new0 (face3d, 1);

  return face;
}

void
destroy_face3d (face3d *face)
{
  g_list_free (face->contours);
  g_free (face);
}

void
face3d_add_contour (face3d *face, edge_ref contour)
{
  face->contours = g_list_append (face->contours, (void *)contour);
}
