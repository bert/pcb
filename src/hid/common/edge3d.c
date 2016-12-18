#include <glib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <math.h>

#include "step_id.h"
#include "quad.h"
#include "vertex3d.h"
#include "edge3d.h"

#define CIRC_SEGS_D 64.0


edge_info *
make_edge_info (void)
{
  edge_info *info;

  info = g_slice_new0 (edge_info);

  /* Default this one, as it was added after most code was written */
  info->same_sense = true;

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

void
edge_info_set_round2 (edge_info *info,
                      double cx, double cy, double cz,
                      double nx, double ny, double nz,
                      double rx, double ry, double rz,
                      double radius)
{
  info->is_round = true;
  info->cx = cx;
  info->cy = cy;
  info->cz = cz;
  info->nx = nx;
  info->ny = ny;
  info->nz = nz;
  info->rx = rx;
  info->ry = ry;
  info->rz = rz;
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

static void
allocate_linearised_vertices (edge_ref e, int num_vertices)
{
  edge_info *info = UNDIR_DATA(e);

  info->num_linearised_vertices = 0;
  info->linearised_vertices = g_new0 (float, 3 * num_vertices);
}

static void
add_vertex (edge_ref e, float x, float y, float z)
{
  edge_info *info = UNDIR_DATA(e);

  info->linearised_vertices[info->num_linearised_vertices * 3 + 0] = x;
  info->linearised_vertices[info->num_linearised_vertices * 3 + 1] = y;
  info->linearised_vertices[info->num_linearised_vertices * 3 + 2] = z;

  info->num_linearised_vertices++;
}

#if 0
static void
evaluate_bspline (edge_info *info, double u, double *x, double *y, double *z)
{
//  info->
}
#endif

static void
sample_bspline (edge_ref e)
{
  edge_info *info = UNDIR_DATA(e);
#if 0
  double x1, y1, z1;
  double x2, y2, z2;
#endif
  double x, y, z;
  int i;

#if 0
  x1 = ((vertex3d *)ODATA(e))->x;
  y1 = ((vertex3d *)ODATA(e))->y;
  z1 = ((vertex3d *)ODATA(e))->z;

  x2 = ((vertex3d *)DDATA(e))->x;
  y2 = ((vertex3d *)DDATA(e))->y;
  z2 = ((vertex3d *)DDATA(e))->z;
#endif

#if 0
  for (i = 0; i < 20; i++)
    {
      evaluate_bspline (edge_info, i / 20.0, &x, &y, &z);

      add_vertex (x, y, z);
    }
#endif

  allocate_linearised_vertices (e, info->num_control_points);

  /* Just draw the control points for now... */
  for (i = 0; i < info->num_control_points; i++)
    {
      int cp_index;

      if (info->same_sense)
        cp_index = i;
      else
        cp_index = info->num_control_points - 1 - i;

      x = info->control_points[cp_index * 3 + 0];
      y = info->control_points[cp_index * 3 + 1];
      z = info->control_points[cp_index * 3 + 2];

      add_vertex (e, x, y, z);
    }
}

static void
sample_ellipse (edge_ref e)
{
  edge_info *info = UNDIR_DATA(e);
  int i;
  double x1, y1, z1;
  double x2, y2, z2;
  double cx, cy, cz;
  double nx, ny, nz;
  double rx, ry, rz;
  double startx, starty, startz;
  double endx, endy, endz;
  double ortx, orty, ortz;
  double cosa;
  double sina;
  double recip_length;
  double sa;
  double da;
  int segs;
  double angle_step;

  x1 = ((vertex3d *)ODATA(e))->x;
  y1 = ((vertex3d *)ODATA(e))->y;
  z1 = ((vertex3d *)ODATA(e))->z;

  x2 = ((vertex3d *)DDATA(e))->x;
  y2 = ((vertex3d *)DDATA(e))->y;
  z2 = ((vertex3d *)DDATA(e))->z;

  cx = ((edge_info *)UNDIR_DATA(e))->cx;
  cy = ((edge_info *)UNDIR_DATA(e))->cy;
  cz = ((edge_info *)UNDIR_DATA(e))->cz;

  nx = ((edge_info *)UNDIR_DATA(e))->nx;
  ny = ((edge_info *)UNDIR_DATA(e))->ny;
  nz = ((edge_info *)UNDIR_DATA(e))->nz;

  rx = ((edge_info *)UNDIR_DATA(e))->rx;
  ry = ((edge_info *)UNDIR_DATA(e))->ry;
  rz = ((edge_info *)UNDIR_DATA(e))->rz;

  if (!info->same_sense)
    {
      nx = -nx;
      ny = -ny;
      nz = -nz;
    }

  startx = x1 - cx;
  starty = y1 - cy;
  startz = z1 - cz;

  /* Normalise startx */
  recip_length = 1. / hypot (hypot (startx, starty), startz);
  startx *= recip_length;
  starty *= recip_length;
  startz *= recip_length;


  /* Find start angle (w.r.t. ellipse parameterisation */

  /* start cross normal */
  /* ort will be orthogonal to normal and r vector */
  ortx = ny * rz - nz * ry;
  orty = nz * rx - nx * rz;
  ortz = nx * ry - ny * rx;

  /* Cosine is dot product of start (normalised) and start (normalised) */
  cosa = rx * startx + ry * starty + rz * startz; // cos (phi)
  /* Sine is dot product of ort (normalised) and start (normalised) */
  sina = ortx * startx + orty * starty + ortz * startz; // sin (phi) = cos (phi - 90)

  /* Start angle */
  sa = atan2 (sina, cosa);

  if (sa < 0.0)
    sa += 2.0 * M_PI;

  endx = x2 - cx;
  endy = y2 - cy;
  endz = z2 - cz;

  /* Normalise endx */
  recip_length = 1. / hypot (hypot (endx, endy), endz);
  endx *= recip_length;
  endy *= recip_length;
  endz *= recip_length;

  /* Find delta angle */

  /* start cross normal */
  /* ort will be orthogonal to normal and start vector */
  ortx = ny * startz - nz * starty;
  orty = nz * startx - nx * startz;
  ortz = nx * starty - ny * startx;

  /* Cosine is dot product of start (normalised) and end (normalised) */
  cosa = startx * endx + starty * endy + startz * endz; // cos (phi)
  /* Sine is dot product of ort (normalised) and end (normalised) */
  sina = ortx * endx + orty * endy + ortz * endz; // sin (phi) = cos (phi - 90)

  if (x1 == x2 &&
      y1 == y2 &&
      z1 == z2)
    {
      da = 2.0 * M_PI;
    }
  else
    {
      /* Delta angled */
      da = atan2 (sina, cosa);

      if (da < 0.0)
        da += 2.0 * M_PI;
    }

  /* Scale up ref and ort to the actual vector length */
  rx *= info->radius;
  ry *= info->radius;
  rz *= info->radius;

  ortx *= info->radius2;
  orty *= info->radius2;
  ortz *= info->radius2;

  segs = CIRC_SEGS_D * da / (2.0 * M_PI);
  segs = MAX(segs, 1);
  angle_step = da / (double)segs;

  allocate_linearised_vertices (e, segs + 1);

  for (i = 0; i <= segs; i++)
    {
      cosa = cos (sa + i * angle_step);
      sina = sin (sa + i * angle_step);
      add_vertex (e, info->cx + rx * cosa + ortx * sina,
                     info->cy + ry * cosa + orty * sina,
                     info->cz + rz * cosa + ortz * sina);
    }

}

static void
sample_circle (edge_ref e)
{
  edge_info *info = UNDIR_DATA(e);
  int i;
  double x1, y1, z1;
  double x2, y2, z2;
  double cx, cy, cz;
  double nx, ny, nz;
  double refx, refy, refz;
  double endx, endy, endz;
  double ortx, orty, ortz;
  double cosa;
  double sina;
  double recip_length;
  double da;
  int segs;
  double angle_step;

  x1 = ((vertex3d *)ODATA(e))->x;
  y1 = ((vertex3d *)ODATA(e))->y;
  z1 = ((vertex3d *)ODATA(e))->z;

  x2 = ((vertex3d *)DDATA(e))->x;
  y2 = ((vertex3d *)DDATA(e))->y;
  z2 = ((vertex3d *)DDATA(e))->z;

  cx = ((edge_info *)UNDIR_DATA(e))->cx;
  cy = ((edge_info *)UNDIR_DATA(e))->cy;
  cz = ((edge_info *)UNDIR_DATA(e))->cz;

  nx = ((edge_info *)UNDIR_DATA(e))->nx;
  ny = ((edge_info *)UNDIR_DATA(e))->ny;
  nz = ((edge_info *)UNDIR_DATA(e))->nz;

  if (!info->same_sense)
    {
      nx = -nx;
      ny = -ny;
      nz = -nz;
    }

  /* STEP MAY ACTUALLY SPECIFY A DIFFERENT REF DIRECTION, BUT FOR NOW, LETS ASSUME IT POINTS
   * TOWARDS THE FIRST POINT. (We don't record the STEP ref direction in our data-structure at the moment).
   */
  refx = x1 - cx;
  refy = y1 - cy;
  refz = z1 - cz;

  /* Normalise refx */
  recip_length = 1. / hypot (hypot (refx, refy), refz);
  refx *= recip_length;
  refy *= recip_length;
  refz *= recip_length;

  endx = x2 - cx;
  endy = y2 - cy;
  endz = z2 - cz;

  /* Normalise endx */
  recip_length = 1. / hypot (hypot (endx, endy), endz);
  endx *= recip_length;
  endy *= recip_length;
  endz *= recip_length;

  /* ref cross normal */
  /* ort will be orthogonal to normal and ref vector */
  ortx = ny * refz - nz * refy;
  orty = nz * refx - nx * refz;
  ortz = nx * refy - ny * refx;

  /* Cosine is dot product of ref (normalised) and end (normalised) */
  cosa = refx * endx + refy * endy + refz * endz; // cos (phi)
  /* Sine is dot product of ort (normalised) and end (normalised) */
  sina = ortx * endx + orty * endy + ortz * endz; // sin (phi) = cos (phi - 90)

  if (x1 == x2 &&
      y1 == y2 &&
      z1 == z2)
    {
      da = 2.0 * M_PI;
    }
  else
    {
      /* Delta angled */
      da = atan2 (sina, cosa);

      if (da < 0.0)
        da += 2.0 * M_PI;
    }

  /* Scale up ref and ort to the actual vector length */
  refx *= info->radius;
  refy *= info->radius;
  refz *= info->radius;

  ortx *= info->radius;
  orty *= info->radius;
  ortz *= info->radius;

  segs = CIRC_SEGS_D * da / (2.0 * M_PI);
  segs = MAX(segs, 1);
  angle_step = da / (double)segs;

  allocate_linearised_vertices (e, segs + 1);

  for (i = 0; i <= segs; i++)
    {
      cosa = cos (i * angle_step);
      sina = sin (i * angle_step);
      add_vertex (e, info->cx + refx * cosa + ortx * sina,
                     info->cy + refy * cosa + orty * sina,
                     info->cz + refz * cosa + ortz * sina);
    }

}

static void
sample_line (edge_ref e)
{
  edge_info *info = UNDIR_DATA(e);
  double x, y, z;

  allocate_linearised_vertices (e, 2);

// NB: Commented, as the same_sense flag only affects parameter space traversal of the line.
//     Since we are sampling the line start and end-point (which have been given explicitly),
//     there is no need to swap the ordering when same_sense is false.
//  if (info->same_sense)
    {
      x = ((vertex3d *)ODATA(e))->x;
      y = ((vertex3d *)ODATA(e))->y;
      z = ((vertex3d *)ODATA(e))->z;

      add_vertex (e, x, y, z);

      x = ((vertex3d *)DDATA(e))->x;
      y = ((vertex3d *)DDATA(e))->y;
      z = ((vertex3d *)DDATA(e))->z;

      add_vertex (e, x, y, z);
    }
#if 0
  else
    {
      /* Unusual, but somtimes occurs */
      //printf ("****************************************************\n");

      x = ((vertex3d *)DDATA(e))->x;
      y = ((vertex3d *)DDATA(e))->y;
      z = ((vertex3d *)DDATA(e))->z;

      add_vertex (e, x, y, z);

      x = ((vertex3d *)ODATA(e))->x;
      y = ((vertex3d *)ODATA(e))->y;
      z = ((vertex3d *)ODATA(e))->z;

      add_vertex (e, x, y, z);
    }
#endif
}

void
edge_ensure_linearised (edge_ref edge)
{
  edge_info *info;

  /* Ensure we're looking at the forward edge */
  edge &= (uintptr_t) ~3;

  info = UNDIR_DATA(edge);

  /* Already cached, nothing to do */
  if (info->linearised_vertices != NULL)
    return;

  /* Can't do anything if we don't have the edge_info data */
  if (info == NULL)
    return;

  if (info->is_bspline)
    {
      sample_bspline (edge);
      return;
    }

  if (info->is_round)
    {
      sample_circle (edge);
      return;
    }

  if (info->is_ellipse)
    {
      sample_ellipse (edge);
      return;
    }

  /* Must be linear */
  sample_line (edge);
}
