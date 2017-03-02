#include <glib.h>

#include "step_id.h"
#include "vertex3d.h"

static int global_vertex3d_count;


vertex3d *
make_vertex3d (double x, double y, double z)
{
  vertex3d *v;

  v = g_new0 (vertex3d, 1);
  v->x = x;
  v->y = y;
  v->z = z;
  v->id = global_vertex3d_count++;

  return v;
}

void
destroy_vertex3d (vertex3d *v)
{
  g_free (v);
}
