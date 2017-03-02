#include <glib.h>
#include <stdbool.h>

#include "quad.h"
#include "contour3d.h"
#include "appearance.h"
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
  g_list_free_full (face->contours, (GDestroyNotify)destroy_contour3d);
  g_free (face);
}

void
face3d_add_contour (face3d *face, contour3d *contour)
{
  face->contours = g_list_append (face->contours, contour);
}

void
face3d_set_appearance (face3d *face, appearance *appear)
{
  face->appear = appear;
}

void
face3d_set_normal (face3d *face, double nx, double ny, double nz)
{
  face->nx = nx;
  face->ny = ny;
  face->nz = nz;
}

void
face3d_set_cylindrical (face3d *face, double cx, double cy, double cz, double ax, double ay, double az, double radius)
{
  face->is_cylindrical = true;
  face->cx = cx;
  face->cy = cy;
  face->cz = cz;
  face->ax = ax;
  face->ay = ay;
  face->az = az;
  face->radius = radius;
}

void
face3d_set_surface_orientation_reversed (face3d *face)
{
  face->surface_orientation_reversed = true;
}
