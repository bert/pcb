#include <glib.h>
#include <stdbool.h>

#include "step_id.h"
#include "quad.h"
#include "contour3d.h"

contour3d *
make_contour3d (edge_ref first_edge)
{
  contour3d *contour;

  contour = g_new0 (contour3d, 1);
  contour->first_edge = first_edge;

  return contour;
}

void
destroy_contour3d (contour3d *contour)
{
  g_free (contour);
}
