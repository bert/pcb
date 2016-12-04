#include <glib.h>
#include <stdbool.h>

#include "step_id.h"
#include "edge3d.h"

edge_info *
make_edge_info (void)
{
  edge_info *info;

  info = g_slice_new0 (edge_info);

  return info;
}

void
edge_info_set_round (edge_info *info, double cx, double cy, double cz, double nx, double ny, double nz, double radius)
{
  info->is_round = true;
  info->cx = cx;
  info->cy = cy;
  info->cz = cz;
  info->nx = nx;
  info->ny = ny;
  info->nz = nz;
  info->radius = radius;
}

void edge_info_set_stitch (edge_info *info)
{
  info->is_stitch = true;
}

void
destroy_edge_info (edge_info *info)
{
  g_slice_free (edge_info, info);
}
