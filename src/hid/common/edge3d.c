#include <glib.h>
#include <stdbool.h>

#include "edge3d.h"

edge_info *
make_edge_info (bool is_stitch, bool is_round, double cx, double cy, double radius)
{
  edge_info *info;

  info = g_new0 (edge_info, 1);
  info->is_stitch = is_stitch;
  info->is_round = is_round;
  info->cx = cx;
  info->cy = cy;
  info->radius = radius;

  return info;
}

void
destroy_edge_info (edge_info *info)
{
  g_free (info);
}
