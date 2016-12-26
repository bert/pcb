#include <glib.h>
#include <stdbool.h>

#include "step_id.h"
#include "quad.h"
#include "edge3d.h"
#include "vertex3d.h"
#include "contour3d.h"
#include "appearance.h"

#include "data.h"
#include "hid.h"
#include "sweep.h"
#include "polygon.h"

#define REVERSED_PCB_CONTOURS 1 /* PCB Contours are reversed from the expected CCW for outer ordering - once the Y-coordinate flip is taken into account */
//#undef REVERSED_PCB_CONTOURS

#ifdef REVERSED_PCB_CONTOURS
#define COORD_TO_STEP_X(pcb, x) (COORD_TO_MM(                   (x)))
#define COORD_TO_STEP_Y(pcb, y) (COORD_TO_MM((pcb)->MaxHeight - (y)))
#define COORD_TO_STEP_Z(pcb, z) (COORD_TO_MM(                   (z)))

#define STEP_X_TO_COORD(pcb, x) (MM_TO_COORD((x)))
#define STEP_Y_TO_COORD(pcb, y) ((pcb)->MaxHeight - MM_TO_COORD((y)))
#define STEP_Z_TO_COORD(pcb, z) (MM_TO_COORD((z)))
#else
/* XXX: BROKEN UPSIDE DOWN OUTPUT */
#define COORD_TO_STEP_X(pcb, x) (COORD_TO_MM((x)))
#define COORD_TO_STEP_Y(pcb, y) (COORD_TO_MM((y)))
#define COORD_TO_STEP_Z(pcb, z) (COORD_TO_MM((z)))

#define STEP_X_TO_COORD(pcb, x) (MM_TO_COORD((x)))
#define STEP_Y_TO_COORD(pcb, y) (MM_TO_COORD((y)))
#define STEP_Z_TO_COORD(pcb, z) (MM_TO_COORD((z)))
#endif


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

#include "data.h"
#include "hid_draw.h"
#include "hidgl.h"

#include "face3d.h"
#include "face3d_gl.h"

//#define MEMCPY_VERTEX_DATA

#define CIRC_SEGS_D 64.0


#define BUFFER_STRIDE 6 /* 3x vertex + 3x normal */

static void
emit_lines (face3d *face)
{
  GLfloat *data_pointer = NULL;

//  CHECK_IS_IN_CONTEXT ();

  if (face->tristrip_num_vertices == 0)
    return;

  data_pointer = face->tristrip_vertices;
  glVertexPointer   (3, GL_FLOAT, sizeof(GL_FLOAT) * BUFFER_STRIDE, data_pointer + 0);
  glNormalPointer   (GL_FLOAT, sizeof(GL_FLOAT) * BUFFER_STRIDE, data_pointer + 3);

//  data_pointer = face->line_indices;
//  glIndexPointer   (GL_INT, sizeof(GL_UNSIGNED_INT), data_pointer + 0);

  glEnableClientState (GL_VERTEX_ARRAY);
  glEnableClientState (GL_NORMAL_ARRAY);

  glTexCoord2f (0.0f, 0.0f);

  glPushAttrib (GL_CURRENT_BIT);
  glColor4f (1., 1., 1., 1.);
  glDrawElements (GL_LINES, face->line_num_indices, GL_UNSIGNED_INT, face->line_indices);
  glPopAttrib ();

  glDisableClientState (GL_VERTEX_ARRAY);
  glDisableClientState (GL_NORMAL_ARRAY);
}


static void
emit_tristrip (face3d *face)
{
  GLfloat *data_pointer = NULL;

//  CHECK_IS_IN_CONTEXT ();

  if (face->tristrip_num_vertices == 0)
    return;

//  if (priv->buffer.use_vbo) {
//    glBindBuffer (GL_ARRAY_BUFFER, priv->buffer.vbo_id);

//    if (priv->buffer.use_map) {
//      glUnmapBuffer (GL_ARRAY_BUFFER);
//      priv->buffer.triangle_array = NULL;
//    } else {
//      glBufferData (GL_ARRAY_BUFFER,
//                    BUFFER_STRIDE * priv->buffer.vertex_count,
//                    priv->buffer.triangle_array,
//                    GL_STREAM_DRAW);
//    }
//  } else {
    data_pointer = face->tristrip_vertices;
//  }

  glVertexPointer   (3, GL_FLOAT, sizeof(GL_FLOAT) * BUFFER_STRIDE, data_pointer + 0);
  glNormalPointer   (GL_FLOAT, sizeof(GL_FLOAT) * BUFFER_STRIDE, data_pointer + 3);
//  glTexCoordPointer (2, GL_FLOAT, BUFFER_STRIDE, data_pointer + 3);

  glEnableClientState (GL_VERTEX_ARRAY);
  glEnableClientState (GL_NORMAL_ARRAY);
//  glEnableClientState (GL_TEXTURE_COORD_ARRAY);

  glTexCoord2f (0.0f, 0.0f);

  glDrawArrays (GL_TRIANGLE_STRIP, 0, face->tristrip_num_vertices);
#if 0
  glPushAttrib (GL_CURRENT_BIT);
  glColor4f (1., 1., 1., 1.);
  glDrawArrays (GL_LINE_STRIP, 0, priv->buffer.vertex_count);
  glPopAttrib ();
#endif
  glDisableClientState (GL_VERTEX_ARRAY);
  glDisableClientState (GL_NORMAL_ARRAY);
//  glDisableClientState (GL_TEXTURE_COORD_ARRAY);
}

static void
toroid_xyz_to_uv (face3d *face, double x, double y, double z, double *u, double *v)
{
  double refx, refy, refz;
  double ortx, orty, ortz;
  double rayx, rayy, rayz;
  double dot;
  double vx, vy, vz;
  double mx, my, mz;
  double recip_length;
  double cosu, sinu;
  double cosv, sinv;

  refx = face->rx;
  refy = face->ry;
  refz = face->rz;

  ortx = face->ay * face->rz - face->az * face->ry;
  orty = face->az * face->rx - face->ax * face->rz;
  ortz = face->ax * face->ry - face->ay * face->rx;

  /* Magnitude of vector component between toroid center and (x,y,z) in axis direction */
  dot = (x - face->ox) * face->ax +
        (y - face->oy) * face->ay +
        (z - face->oz) * face->az;

  /* Find the vector to x,y,z in the plane bisecting the toroid slice at z=0 */
  rayx = x - face->ox - dot * face->ax;
  rayy = y - face->oy - dot * face->ay;
  rayz = z - face->oz - dot * face->az;

  /* Normalise v */
  recip_length = 1. / hypot (hypot (rayx, rayy), rayz);
  rayx *= recip_length;
  rayy *= recip_length;
  rayz *= recip_length;

  /* Cosine is dot product of ref (normalised) and ray (normalised) */
  cosu = refx * rayx + refy * rayy + refz * rayz; // cos (phi)
  /* Sine is dot product of ort (normalised) and ray (normalised) */
  sinu = ortx * rayx + orty * rayy + ortz * rayz; // sin (phi) = cos (phi - 90)

  /* U is the angle */
  *u = atan2 (sinu, cosu);

  /* Point at u, middle of toroid cross-section */
  mx = face->ox + rayx * face->radius;
  my = face->oy + rayy * face->radius;
  mz = face->oz + rayz * face->radius;

  /* Find the vector to x,y,z in the plane cutting the toroid center, and passing through u */
  vx = x - mx;
  vy = y - my;
  vz = z - mz;

  /* Normalise v */
  recip_length = 1. / hypot (hypot (vx, vy), vz);
  vx *= recip_length;
  vy *= recip_length;
  vz *= recip_length;

  /* Now, our reference directions are different when calculating the v angle...
   * what was ref above, points in direction from o->m. (ray)
   * what was ort above, points in our axis direction
   */

  /* Cosine is dot product of ref (normalised) and v (normalised) */
  cosv = rayx * vx + rayy * vy + rayz * vz; // cos (phi)
  /* Sine is dot product of ort (normalised) and v (normalised) */
  sinv = face->ax * vx + face->ay * vy + face->az * vz; // sin (phi) = cos (phi - 90)

  /* V is the angle */
  *v = atan2 (sinv, cosv);

  if (*u < 0.0)
    *u += 2.0 * M_PI;

  /* Convert to degrees */
  *u *= 180. / M_PI;

  if (*v < 0.0)
    *v += 2.0 * M_PI;

  /* Convert to degrees */
  *v *= 180. / M_PI;
}

static void
toroid_uv_to_xyz_and_normal (face3d *face,
                             double u, double v,
                             float *x, float *y, float *z,
                             float *nx, float *ny, float *nz)
{
  float ortx, orty, ortz;
  double Rr;
  double cosu, sinu;
  double cosv, sinv;
  double Rrcosu, Rrsinu;
  double rcosv, rsinv;

  ortx = face->ay * face->rz - face->az * face->ry;
  orty = face->az * face->rx - face->ax * face->rz;
  ortz = face->ax * face->ry - face->ay * face->rx;

  cosu = cos(u / 180. * M_PI);
  sinu = sin(u / 180. * M_PI);
  cosv = cos(v / 180. * M_PI);
  sinv = sin(v / 180. * M_PI);

  rcosv = face->minor_radius * cosv;
  rsinv = face->minor_radius * sinv;

  Rr = face->radius + rcosv;
  Rrcosu = Rr * cosu;
  Rrsinu = Rr * sinu;

  *x = STEP_X_TO_COORD(PCB, face->ox + Rrcosu * face->rx + Rrsinu * ortx + rsinv * face->ax);
  *y = STEP_Y_TO_COORD(PCB, face->oy + Rrcosu * face->ry + Rrsinu * orty + rsinv * face->ay);
  *z = STEP_Z_TO_COORD(PCB, face->oz + Rrcosu * face->rz + Rrsinu * ortz + rsinv * face->az);

  *nx =  (cosv * cosu * face->rx + cosv * sinu * ortx + sinv * face->ax);
  *ny = -(cosv * cosu * face->ry + cosv * sinu * orty + sinv * face->ay); /* XXX: Note this is minus, presumably due to PCB's coordinate space */
  *nz =  (cosv * cosu * face->rz + cosv * sinu * ortz + sinv * face->az);

  if (face->surface_orientation_reversed)
    {
      *nx = -*nx;
      *ny = -*ny;
      *nz = -*nz;
    }
}

static void
toroid_bo_add_edge_uwrap (borast_t *bo,
                          double lu, double lv,
                          double  u, double  v,
                          bool is_outer)
{
  double minu;

  /* XXX: Not absolutely sure about this! */
  if (fabs (u - lu) > fabs (u + 360.0 - lu))
    {
      bo_add_edge (bo,
                   MM_TO_COORD (lu        ), MM_TO_COORD (lv),
                   MM_TO_COORD ( u + 360.0), MM_TO_COORD ( v),
                   is_outer);
      bo_add_edge (bo,
                   MM_TO_COORD (lu - 360.0), MM_TO_COORD (lv),
                   MM_TO_COORD ( u        ), MM_TO_COORD ( v),
                   is_outer);
      /* Extra edge to keep the in/out regions correct */
//      minu = MIN (lu - 360.0, u);
      printf ("Wrapped u edge, lu = %f, u=%f, lu-360=%f, minu=%f\n",
             lu, u, lu - 360.0, minu);
      minu = 0;
      bo_add_edge (bo,
                   MM_TO_COORD (minu), MM_TO_COORD (lv),
                   MM_TO_COORD (minu), MM_TO_COORD (v),
                   is_outer);
    }
  else if (fabs (u - lu) > fabs (u - 360.0 - lu))
    {
      bo_add_edge (bo,
                   MM_TO_COORD (lu        ), MM_TO_COORD (lv),
                   MM_TO_COORD ( u - 360.0), MM_TO_COORD ( v),
                   is_outer);
      bo_add_edge (bo,
                   MM_TO_COORD (lu + 360.0), MM_TO_COORD (lv),
                   MM_TO_COORD ( u        ), MM_TO_COORD ( v),
                   is_outer);
      /* Extra edge to keep the in/out regions correct */
      minu = MIN (lu, u - 360.0);
      printf ("Wrapped u edge, lu = %f, u=%f, u-360=%f, minu=%f\n",
             lu, u, u - 360.0, minu);
      minu = 0;
      bo_add_edge (bo,
                   MM_TO_COORD (minu), MM_TO_COORD (lv),
                   MM_TO_COORD (minu), MM_TO_COORD (v),
                   is_outer);
    }
  else
    {
      bo_add_edge (bo,
                   MM_TO_COORD (lu), MM_TO_COORD (lv),
                   MM_TO_COORD ( u), MM_TO_COORD ( v),
                   is_outer);
    }

}

static void
toroid_bo_add_edge (borast_t *bo,
                  double lu, double lv,
                  double  u, double  v,
                  bool is_outer)
{
  /* XXX: Not absolutely sure about this! */
  if (fabs (v - lv) > fabs (v + 360.0 - lv))
    {
      toroid_bo_add_edge_uwrap (bo,
                                lu, lv,
                                 u,  v + 360.0,
                                is_outer);
      toroid_bo_add_edge_uwrap (bo,
                                lu, lv - 360.0,
                                 u,  v,
                                is_outer);
    }
  else if (fabs (v - lv) > fabs (v - 360.0 - lv))
    {
      toroid_bo_add_edge_uwrap (bo,
                                lu, lv,
                                 u,  v - 360.0,
                                is_outer);
      toroid_bo_add_edge_uwrap (bo,
                                lu, lv + 360.0,
                                 u,  v,
                                is_outer);
    }
  else
    {
      toroid_bo_add_edge_uwrap (bo,
                                lu, lv,
                                 u,  v,
                                is_outer);
    }

}

static void plane_uv_to_xyz_and_normal (face3d *face, float u, float v, float *x, float *y, float *z, float *nx, float *ny, float *nz);

static void
toroid_ensure_tristrip (face3d *face)
{
  GList *c_iter;
  int num_uv_points;
  double *uv_points;
  int num_line_indices = 0;
  unsigned int *line_indices;
  int i, j;
  int vertex_comp;
  contour3d *contour;
  edge_ref e;
  int x1, x2, x3, x4, y_top, y_bot;
  borast_t *bo;
  borast_traps_t traps;
  int edge_count = 0;

  /* Nothing to do if vertices are already cached */
  if (face->tristrip_vertices != NULL)
    return;

  /* Don't waste time if we failed last time */
  if (face->triangulate_failed)
    return;

  if (!face->is_toroidal)
    return;

  /* Count up the number of edges space is required for */
  for (c_iter = face->contours; c_iter != NULL; c_iter = g_list_next (c_iter))
    {
      contour = c_iter->data;
      e = contour->first_edge;

      do
        {
          edge_info *info = UNDIR_DATA (e);

          edge_ensure_linearised (e);
          edge_count += info->num_linearised_vertices;
        }
      while ((e = LNEXT(e)) != contour->first_edge);

    }

#if 1
  /* Worst case, we need 2x number of edges, since we repeat any which span the u=0, u=360 wrap-around. */
  bo = bo_init (2 * edge_count + 2 * 37  + 2); /* NB +37 is kludge for our vertical bars */

#if 1
  {
    int i;

#if 0
    /* Add a line at the start of our bounds to get things started */
    bo_add_edge (bo,
                 MM_TO_COORD(0), MM_TO_COORD(0.0),
                 MM_TO_COORD(0), MM_TO_COORD(360.0),
                 false);
#endif

#if 0
    bo_add_edge (bo,
                 MM_TO_COORD(360.0), MM_TO_COORD(0.0),
                 MM_TO_COORD(360.0), MM_TO_COORD(360.0),
                 false);
#endif

    for (i = 0; i <= 360; i += 10)
      {
        for (j = 0; j < 360; j += 10)
          {
            bo_add_edge (bo,
                         MM_TO_COORD(i), MM_TO_COORD(j),
                         MM_TO_COORD(i), MM_TO_COORD(j + 10.0),
                         true);

#if 0
            bo_add_edge (bo,
                         MM_TO_COORD(i + 0.01), MM_TO_COORD(j),
                         MM_TO_COORD(i + 0.01), MM_TO_COORD(j + 10.0),
                         false);
#endif
          }
      }
  }
#endif

  /* Throw the edges to the rasteriser */
  for (c_iter = face->contours; c_iter != NULL; c_iter = g_list_next (c_iter))
    {
      double fu = 0.0, fv = 0.0;
      double lu = 0.0, lv = 0.0;
      double u, v;
      bool first_vertex = true;
      bool is_outer;

      /* XXX: How can we tell if a contour is inner or outer??? */
      is_outer = true;

      contour = c_iter->data;
      e = contour->first_edge;

      do
        {
          edge_info *info = UNDIR_DATA (e);
          bool backwards_edge;

          /* XXX: Do this without breaking abstraction? */
          /* Detect SYM edges, reverse the circle normal */
          backwards_edge = ((e & 2) == 2);

          edge_ensure_linearised (e);

          for (i = 0; i < info->num_linearised_vertices - 1; i++)
            {
              int vertex_idx = i;
              float x, y, z, nx, ny, nz;

              if (backwards_edge)
                vertex_idx = info->num_linearised_vertices - 1 - i;

              toroid_xyz_to_uv (face,
                                info->linearised_vertices[vertex_idx * 3 + 0],
                                info->linearised_vertices[vertex_idx * 3 + 1],
                                info->linearised_vertices[vertex_idx * 3 + 2],
                                &u, &v);

#if 0
              toroid_uv_to_xyz_and_normal (face, u, v, &x, &y, &z, &nx, &ny, &nz);
              x = COORD_TO_STEP_X(PCB, x);
              y = COORD_TO_STEP_Y(PCB, y);
              z = COORD_TO_STEP_Z(PCB, z);

              printf ("(%f, %f, %f) -> (%f, %f) -> (%f, %f, %f)  DELTA: (%f, %f, %f)\n",
                      (double)info->linearised_vertices[vertex_idx * 3 + 0],
                      (double)info->linearised_vertices[vertex_idx * 3 + 1],
                      (double)info->linearised_vertices[vertex_idx * 3 + 2],
                      u, v, x, y, z,
                      (double)x - info->linearised_vertices[vertex_idx * 3 + 0],
                      (double)y - info->linearised_vertices[vertex_idx * 3 + 1],
                      (double)z - info->linearised_vertices[vertex_idx * 3 + 2]);
#endif

              if (first_vertex)
                {
                  fu = u;
                  fv = v;
                }
              else
                {
                  toroid_bo_add_edge (bo,
                                      lu, lv,
                                       u,  v,
                                      is_outer);
                }

              lu = u;
              lv = v;
              first_vertex = false;
            }

        }
      while ((e = LNEXT(e)) != contour->first_edge);

      toroid_bo_add_edge (bo,
                          lu, lv,
                          fu, fv,
                          is_outer);
    }

  _borast_traps_init (&traps);
  bo_tesselate_to_traps (bo, false /* Don't combine adjacent y traps */,  &traps);

  bo_free (bo);

  num_uv_points = 0;

  for (i = 0; i < traps.num_traps; i++) {
    y_top = traps.traps[i].top;
    y_bot = traps.traps[i].bottom;

    x1 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_top);
    x2 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_top);
    x3 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_bot);
    x4 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_bot);

    if ((x1 == x2) || (x3 == x4)) {
      num_uv_points += 5 + 1; /* Three vertices + repeated start and end, extra repeat to sync backface culling */
    } else {
      num_uv_points += 6; /* Four vertices + repeated start and end */
    }
  }

  if (num_uv_points == 0) {
    printf ("Strange, contour didn't tesselate\n");
    face->triangulate_failed = true;
    return;
  }

  uv_points = g_new0 (double, 2 * num_uv_points + 8);
  line_indices = g_new0 (int, 10 * traps.num_traps + 8);

  vertex_comp = 0;
  num_uv_points = 0;

  for (i = 0; i < traps.num_traps; i++) {
    y_top = traps.traps[i].top;
    y_bot = traps.traps[i].bottom;

    /* NB: ybot > ytop, as this is all derived from a screen-space rasteriser with 0,0 in the top left */

#if 0
    /* Exclude strips entirely above or below the 0 <= u <= 360 range */
    if (y_bot < MM_TO_COORD (0.0f))
      continue;

    if (y_top > MM_TO_COORD (360.0f))
      continue;
#endif

    /* Clamp evaluation coordinates otherwise (strips straddling the boundary)
     * NB: Due to input parameter-space geometry duplication, the bit we trim
     *     here will be duplicated on the other side of the wrap-around anyway
     */
    y_top = MAX(MM_TO_COORD(0.0f), y_top);
    y_bot = MIN(y_bot, MM_TO_COORD(360.0f));


    if (face->surface_orientation_reversed)
      {
        x2 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_top);
        x1 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_top);
        x4 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_bot);
        x3 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_bot);
      }
    else
      {
        x1 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_top);
        x2 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_top);
        x3 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_bot);
        x4 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_bot);
      }

#if 1
    /* Skip pieces which wrap around the u boundary */
    if (x1 < MM_TO_COORD(0.0) ||
        x2 < MM_TO_COORD(0.0) ||
        x3 < MM_TO_COORD(0.0) ||
        x4 < MM_TO_COORD(0.0) ||
        x1 > MM_TO_COORD(360.0) ||
        x2 > MM_TO_COORD(360.0) ||
        x3 > MM_TO_COORD(360.0) ||
        x4 > MM_TO_COORD(360.0))
      continue;
#endif

    if (x1 == x2) {
      /* NB: Repeated first virtex to separate from other tri-strip */
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      /* NB: Repeated last virtex to separate from other tri-strip */
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      /* NB: Extra repeated vertex to keep backface culling in sync */

      num_uv_points += 6;
    } else if (x3 == x4) {
      /* NB: Repeated first virtex to separate from other tri-strip */
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x2;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      /* NB: Repeated last virtex to separate from other tri-strip */
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      /* NB: Extra repeated vertex to keep backface culling in sync */

      num_uv_points += 6;
    } else {
      /* NB: Repeated first virtex to separate from other tri-strip */
      uv_points[vertex_comp++] = x2;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x2;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      /* NB: Repeated last virtex to separate from other tri-strip */

      line_indices[num_line_indices++] = num_uv_points + 1;
      line_indices[num_line_indices++] = num_uv_points + 2;

      line_indices[num_line_indices++] = num_uv_points + 1;
      line_indices[num_line_indices++] = num_uv_points + 3;

      /* Central line */
      line_indices[num_line_indices++] = num_uv_points + 2;
      line_indices[num_line_indices++] = num_uv_points + 3;

      line_indices[num_line_indices++] = num_uv_points + 3;
      line_indices[num_line_indices++] = num_uv_points + 4;

      line_indices[num_line_indices++] = num_uv_points + 4;
      line_indices[num_line_indices++] = num_uv_points + 2;

      num_uv_points += 6;
    }
  }

  uv_points[vertex_comp++] = MM_TO_COORD(0.0  );  uv_points[vertex_comp++] = MM_TO_COORD(0.0  );
  uv_points[vertex_comp++] = MM_TO_COORD(0.0  );  uv_points[vertex_comp++] = MM_TO_COORD(360.0);
  uv_points[vertex_comp++] = MM_TO_COORD(360.0);  uv_points[vertex_comp++] = MM_TO_COORD(360.0);
  uv_points[vertex_comp++] = MM_TO_COORD(360.0);  uv_points[vertex_comp++] = MM_TO_COORD(0.0  );

  line_indices[num_line_indices++] = num_uv_points + 0;
  line_indices[num_line_indices++] = num_uv_points + 1;
  line_indices[num_line_indices++] = num_uv_points + 1;
  line_indices[num_line_indices++] = num_uv_points + 2;
  line_indices[num_line_indices++] = num_uv_points + 2;
  line_indices[num_line_indices++] = num_uv_points + 3;
  line_indices[num_line_indices++] = num_uv_points + 3;
  line_indices[num_line_indices++] = num_uv_points + 0;

  face->line_indices = line_indices;
  face->line_num_indices = num_line_indices;

  _borast_traps_fini (&traps);
#endif

#if 0
  num_uv_points = 6 * 36 * 36;
  uv_points = g_new0 (double, 2 * num_uv_points);

  vertex_comp = 0;
  num_uv_points = 0;

  {
    int i, j;

    for (i = 0; i < 360; i += 10) {
      for (j = 0; j < 360; j += 10) {
        float x1, x2, x3, x4, y_top, y_bot;
        float x, y, z, nx, ny, nz;
        double u, v;

        x1 = MM_TO_COORD(i);  x2 = MM_TO_COORD(i + 10);
        x4 = MM_TO_COORD(i);  x3 = MM_TO_COORD(i + 10);
        y_top = MM_TO_COORD(j);
        y_bot = MM_TO_COORD(j + 10);

        uv_points[vertex_comp++] = x2;  uv_points[vertex_comp++] = y_top;
        uv_points[vertex_comp++] = x2;  uv_points[vertex_comp++] = y_top;
        uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
        uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
        uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
        uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
        num_uv_points += 6;


        toroid_uv_to_xyz_and_normal (face, i, j, &x, &y, &z, &nx, &ny, &nz);
        x = COORD_TO_STEP_X(PCB, x);
        y = COORD_TO_STEP_Y(PCB, y);
        z = COORD_TO_STEP_Z(PCB, z);

        toroid_xyz_to_uv (face, x, y, z, &u, &v);

        printf ("(%f, %f) -> (%f, %f, %f) -> (%f, %f)  DELTA: (%f, %f)\n",
                (double)i, (double)j,
                (double)x,
                (double)y,
                (double)z,
                (double)u, (double)v,
                (double)(i - u),
                (double)(j - v));

      }
    }
  }
#endif

  /* XXX: Would it be better to use the original vertices?
   *      Rather than converting to u-v coordinates and back.
   *      Probably at least need to use the u-v points to
   *      perform the triangulation.
   */

  face->tristrip_num_vertices = num_uv_points;

  num_uv_points += 4;
  face->tristrip_vertices = g_new0 (float, BUFFER_STRIDE * num_uv_points);

  vertex_comp = 0;
  for (i = 0; i < num_uv_points; i++)
    {
#if 1
      plane_uv_to_xyz_and_normal (face,
                                  COORD_TO_MM (uv_points[2 * i + 0] * 0.1),
                                  COORD_TO_MM (uv_points[2 * i + 1] * 0.1),
                                  &face->tristrip_vertices[vertex_comp + 0],
                                  &face->tristrip_vertices[vertex_comp + 1],
                                  &face->tristrip_vertices[vertex_comp + 2],
                                  /* Vertex normal */
                                  &face->tristrip_vertices[vertex_comp + 3],
                                  &face->tristrip_vertices[vertex_comp + 4],
                                  &face->tristrip_vertices[vertex_comp + 5]);

      vertex_comp += BUFFER_STRIDE;
#else
      toroid_uv_to_xyz_and_normal (face,
                                   /* uv */
                                   COORD_TO_MM (uv_points[2 * i + 0]), /* Inverse of arbitrary transformation above */
                                   COORD_TO_MM (uv_points[2 * i + 1]), /* Inverse of arbitrary transformation above */
                                   /* xyz */
                                   &face->tristrip_vertices[vertex_comp + 0],
                                   &face->tristrip_vertices[vertex_comp + 1],
                                   &face->tristrip_vertices[vertex_comp + 2],
                                   /* Vertex normal */
                                   &face->tristrip_vertices[vertex_comp + 3],
                                   &face->tristrip_vertices[vertex_comp + 4],
                                   &face->tristrip_vertices[vertex_comp + 5]);

      vertex_comp += BUFFER_STRIDE;
#endif
    }

  g_free (uv_points);
}




static void
cone_xyz_to_uv (face3d *face, float x, float y, float z, float *u, float *v)
{
  double refx, refy, refz;
  double ortx, orty, ortz;
  double vx, vy, vz;
  double recip_length;
  double cosa, sina;

  refx = face->rx;
  refy = face->ry;
  refz = face->rz;

  ortx = face->ay * face->rz - face->az * face->ry;
  orty = face->az * face->rx - face->ax * face->rz;
  ortz = face->ax * face->ry - face->ay * face->rx;

  /* v is dot product of vector from surface origin to point, and the axis direction */
  *v = (x - face->ox) * face->ax +
       (y - face->oy) * face->ay +
       (z - face->oz) * face->az;

  /* Find the vector to x,y,z in the plane of the cone slice at *v */
  vx = x - face->ox - *v * face->ax;
  vy = y - face->oy - *v * face->ay;
  vz = z - face->oz - *v * face->az;

  /* Normalise v */
  recip_length = 1. / hypot (hypot (vx, vy), vz);
  vx *= recip_length;
  vy *= recip_length;
  vz *= recip_length;

  /* Cosine is dot product of ref (normalised) and v (normalised) */
  cosa = refx * vx + refy * vy + refz * vz; // cos (phi)
  /* Sine is dot product of ort (normalised) and v (normalised) */
  sina = ortx * vx + orty * vy + ortz * vz; // sin (phi) = cos (phi - 90)

  /* U is the angle */
  *u = atan2 (sina, cosa);

  if (*u < 0.0)
    *u += 2.0 * M_PI;

  /* Convert to degrees */
  *u *= 180. / M_PI;
}

static void
cone_uv_to_xyz_and_normal (face3d *face, float u, float v, float *x, float *y, float *z,
                           float *nx, float *ny, float *nz)
{
  float ortx, orty, ortz;
  double cosu, sinu;
  double rcosu, rsinu;
  double tana;
  double radius;
  double recip_length;

  ortx = face->ay * face->rz - face->az * face->ry;
  orty = face->az * face->rx - face->ax * face->rz;
  ortz = face->ax * face->ry - face->ay * face->rx;

  cosu = cos(u / 180. * M_PI);
  sinu = sin(u / 180. * M_PI);
  tana = tan(face->semi_angle / 180.0 * M_PI);

  radius = face->radius + v * tana;
  rcosu = radius * cosu;
  rsinu = radius * sinu;

  *x = STEP_X_TO_COORD(PCB, face->ox + rcosu * face->rx + rsinu * ortx + v * face->ax);
  *y = STEP_Y_TO_COORD(PCB, face->oy + rcosu * face->ry + rsinu * orty + v * face->ay);
  *z = STEP_Z_TO_COORD(PCB, face->oz + rcosu * face->rz + rsinu * ortz + v * face->az);

  recip_length = 1.0 / hypot (1.0, tana);

  *nx =  (cosu * face->rx + sinu * ortx - tana * face->ax) * recip_length;
  *ny = -(cosu * face->ry + sinu * orty - tana * face->ay) * recip_length; /* XXX: Note this is minus, presumably due to PCB's coordinate space */
  *nz =  (cosu * face->rz + sinu * ortz - tana * face->az) * recip_length;

  if (face->surface_orientation_reversed != (face->radius + v * tana < 0.0))
    {
      *nx = -*nx;
      *ny = -*ny;
      *nz = -*nz;
    }
}

static void
cone_bo_add_edge (borast_t *bo,
                  double lu, double lv,
                  double  u, double  v,
                  bool is_outer)
{
  /* XXX: Not absolutely sure about this! */
  if (fabs (u - lu) > fabs (u + 360.0f - lu))
    {
#if 1
      bo_add_edge (bo,
                   MM_TO_COORD (lv), MM_TO_COORD (lu),
                   MM_TO_COORD ( v), MM_TO_COORD ( u + 360.0f),
                   is_outer);
#endif
#if 1
      bo_add_edge (bo,
                   MM_TO_COORD (lv), MM_TO_COORD (lu - 360.0f),
                   MM_TO_COORD ( v), MM_TO_COORD ( u),
                   is_outer);
#endif
    }
  else if (fabs (u - lu) > fabs (u - 360.0f - lu))
    {
#if 1
      bo_add_edge (bo,
                   MM_TO_COORD (lv), MM_TO_COORD (lu),
                   MM_TO_COORD ( v), MM_TO_COORD ( u - 360.0f),
                   is_outer);
#endif
#if 1
      bo_add_edge (bo,
                   MM_TO_COORD (lv), MM_TO_COORD (lu + 360.0f),
                   MM_TO_COORD ( v), MM_TO_COORD ( u),
                   is_outer);
#endif
    }
  else
    {
      bo_add_edge (bo,
                   MM_TO_COORD (lv), MM_TO_COORD (lu),
                   MM_TO_COORD ( v), MM_TO_COORD ( u),
                   is_outer);
    }

}
static void
cone_ensure_tristrip (face3d *face)
{
  GList *c_iter;
  int num_uv_points;
  float *uv_points;
  int i;
  int vertex_comp;
  contour3d *contour;
  edge_ref e;
  int x1, x2, x3, x4, y_top, y_bot;
  borast_t *bo;
  borast_traps_t traps;
  int edge_count = 0;

  /* Nothing to do if vertices are already cached */
  if (face->tristrip_vertices != NULL)
    return;

  /* Don't waste time if we failed last time */
  if (face->triangulate_failed)
    return;

  if (!face->is_conical)
    return;

  /* Count up the number of edges space is required for */
  for (c_iter = face->contours; c_iter != NULL; c_iter = g_list_next (c_iter))
    {
      contour = c_iter->data;
      e = contour->first_edge;

      do
        {
          edge_info *info = UNDIR_DATA (e);

          edge_ensure_linearised (e);
          edge_count += info->num_linearised_vertices;
        }
      while ((e = LNEXT(e)) != contour->first_edge);

    }

  /* Worst case, we need 2x number of edges, since we repeat any which span the u=0, u=360 wrap-around. */
  bo = bo_init (2 * edge_count);

  /* Throw the edges to the rasteriser */
  for (c_iter = face->contours; c_iter != NULL; c_iter = g_list_next (c_iter))
    {
      float fu = 0.0f, fv = 0.0f;
      float lu = 0.0f, lv = 0.0f;
      float u, v;
      bool first_vertex = true;
      bool is_outer;

      /* XXX: How can we tell if a contour is inner or outer??? */
      is_outer = true;

      contour = c_iter->data;
      e = contour->first_edge;

      do
        {
          edge_info *info = UNDIR_DATA (e);
          bool backwards_edge;

          /* XXX: Do this without breaking abstraction? */
          /* Detect SYM edges, reverse the circle normal */
          backwards_edge = ((e & 2) == 2);

          edge_ensure_linearised (e);

          for (i = 0; i < info->num_linearised_vertices - 1; i++)
            {
              int vertex_idx = i;

              if (backwards_edge)
                vertex_idx = info->num_linearised_vertices - 1 - i;

              cone_xyz_to_uv (face,
                              info->linearised_vertices[vertex_idx * 3 + 0],
                              info->linearised_vertices[vertex_idx * 3 + 1],
                              info->linearised_vertices[vertex_idx * 3 + 2],
                              &u, &v);

              if (first_vertex)
                {
                  fu = u;
                  fv = v;
                }
              else
                {
                  cone_bo_add_edge (bo,
                                    lu, lv,
                                     u,  v,
                                    is_outer);
                }

              lu = u;
              lv = v;
              first_vertex = false;
            }

        }
      while ((e = LNEXT(e)) != contour->first_edge);

      cone_bo_add_edge (bo,
                            lu, lv,
                            fu, fv,
                            is_outer);
    }

  _borast_traps_init (&traps);
  bo_tesselate_to_traps (bo, false /* Don't combine adjacent y traps */,  &traps);

  bo_free (bo);

  num_uv_points = 0;

  for (i = 0; i < traps.num_traps; i++) {
    y_top = traps.traps[i].top;
    y_bot = traps.traps[i].bottom;

    x1 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_top);
    x2 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_top);
    x3 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_bot);
    x4 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_bot);

    if ((x1 == x2) || (x3 == x4)) {
      num_uv_points += 5 + 1; /* Three vertices + repeated start and end, extra repeat to sync backface culling */
    } else {
      num_uv_points += 6; /* Four vertices + repeated start and end */
    }
  }

  if (num_uv_points == 0) {
    printf ("Strange, contour didn't tesselate\n");
    face->triangulate_failed = true;
    return;
  }

  uv_points = g_new0 (float, 2 * num_uv_points);

  vertex_comp = 0;
  num_uv_points = 0;

  for (i = 0; i < traps.num_traps; i++) {
    y_top = traps.traps[i].top;
    y_bot = traps.traps[i].bottom;

    /* NB: ybot > ytop, as this is all derived from a screen-space rasteriser with 0,0 in the top left */

    /* Exclude strips entirely above or below the 0 <= u <= 360 range */
    if (y_bot < MM_TO_COORD (0.0f))
      continue;

    if (y_top > MM_TO_COORD (360.0f))
      continue;

    /* Clamp evaluation coordinates otherwise (strips straddling the boundary)
     * NB: Due to input parameter-space geometry duplication, the bit we trim
     *     here will be duplicated on the other side of the wrap-around anyway
     */
    y_top = MAX(MM_TO_COORD(0.0f), y_top);
    y_bot = MIN(y_bot, MM_TO_COORD(360.0f));


    if (face->surface_orientation_reversed)
      {
        x2 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_top);
        x1 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_top);
        x4 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_bot);
        x3 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_bot);
      }
    else
      {
        x1 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_top);
        x2 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_top);
        x3 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_bot);
        x4 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_bot);
      }

    if (x1 == x2) {
      /* NB: Repeated first virtex to separate from other tri-strip */
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      /* NB: Repeated last virtex to separate from other tri-strip */
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      /* NB: Extra repeated vertex to keep backface culling in sync */

      num_uv_points += 6;
    } else if (x3 == x4) {
      /* NB: Repeated first virtex to separate from other tri-strip */
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x2;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      /* NB: Repeated last virtex to separate from other tri-strip */
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      /* NB: Extra repeated vertex to keep backface culling in sync */

      num_uv_points += 6;
    } else {
      /* NB: Repeated first virtex to separate from other tri-strip */
      uv_points[vertex_comp++] = x2;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x2;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      /* NB: Repeated last virtex to separate from other tri-strip */

      num_uv_points += 6;
    }
  }

  _borast_traps_fini (&traps);

  /* XXX: Would it be better to use the original vertices?
   *      Rather than converting to u-v coordinates and back.
   *      Probably at least need to use the u-v points to
   *      perform the triangulation.
   */

  face->tristrip_num_vertices = num_uv_points;
  face->tristrip_vertices = g_new0 (float, BUFFER_STRIDE * num_uv_points);

  vertex_comp = 0;
  for (i = 0; i < num_uv_points; i++)
    {
      cone_uv_to_xyz_and_normal(face,
                                /* uv */
                                COORD_TO_MM (uv_points[2 * i + 1]), /* Inverse of arbitrary transformation above */
                                COORD_TO_MM (uv_points[2 * i + 0]), /* Inverse of arbitrary transformation above */
                                /* xyz */
                                &face->tristrip_vertices[vertex_comp + 0],
                                &face->tristrip_vertices[vertex_comp + 1],
                                &face->tristrip_vertices[vertex_comp + 2],
                                /* Vertex normal */
                                &face->tristrip_vertices[vertex_comp + 3],
                                &face->tristrip_vertices[vertex_comp + 4],
                                &face->tristrip_vertices[vertex_comp + 5]);

      vertex_comp += BUFFER_STRIDE;
    }

  g_free (uv_points);
}


static void
cylinder_xyz_to_uv (face3d *face, float x, float y, float z, float *u, float *v)
{
  double refx, refy, refz;
  double ortx, orty, ortz;
  double vx, vy, vz;
  double recip_length;
  double cosa, sina;

  refx = face->rx;
  refy = face->ry;
  refz = face->rz;

  ortx = face->ay * face->rz - face->az * face->ry;
  orty = face->az * face->rx - face->ax * face->rz;
  ortz = face->ax * face->ry - face->ay * face->rx;

  /* v is dot product of vector from surface origin to point, and the axis direction */
  *v = (x - face->ox) * face->ax +
       (y - face->oy) * face->ay +
       (z - face->oz) * face->az;

  /* Find the vector to x,y,z in the plane of the cylinder slice at *v */
  vx = x - face->ox - *v * face->ax;
  vy = y - face->oy - *v * face->ay;
  vz = z - face->oz - *v * face->az;

  /* Normalise v */
  recip_length = 1. / hypot (hypot (vx, vy), vz);
  vx *= recip_length;
  vy *= recip_length;
  vz *= recip_length;

  /* Cosine is dot product of ref (normalised) and v (normalised) */
  cosa = refx * vx + refy * vy + refz * vz; // cos (phi)
  /* Sine is dot product of ort (normalised) and v (normalised) */
  sina = ortx * vx + orty * vy + ortz * vz; // sin (phi) = cos (phi - 90)

  /* U is the angle */
  *u = atan2 (sina, cosa);

  if (*u < 0.0)
    *u += 2.0 * M_PI;

  /* Convert to degrees */
  *u *= 180. / M_PI;
}

static void
cylinder_uv_to_xyz_and_normal (face3d *face, float u, float v, float *x, float *y, float *z,
                               float *nx, float *ny, float *nz)
{
  float ortx, orty, ortz;
  double cosu, sinu;
  double rcosu, rsinu;

  ortx = face->ay * face->rz - face->az * face->ry;
  orty = face->az * face->rx - face->ax * face->rz;
  ortz = face->ax * face->ry - face->ay * face->rx;

  cosu = cos(u / 180. * M_PI);
  sinu = sin(u / 180. * M_PI);

  rcosu = face->radius * cosu;
  rsinu = face->radius * sinu;

  *x = STEP_X_TO_COORD(PCB, face->ox + rcosu * face->rx + rsinu * ortx + v * face->ax);
  *y = STEP_Y_TO_COORD(PCB, face->oy + rcosu * face->ry + rsinu * orty + v * face->ay);
  *z = STEP_Z_TO_COORD(PCB, face->oz + rcosu * face->rz + rsinu * ortz + v * face->az);

  if (face->surface_orientation_reversed)
    {
      *nx = -(cosu * face->rx + sinu * ortx);
      *ny = cosu * face->ry + sinu * orty; /* XXX: Note this is not negated, presumably due to PCB's coordinate space */
      *nz = -(cosu * face->rz + sinu * ortz);
    }
  else
    {
      *nx = cosu * face->rx + sinu * ortx;
      *ny = -(cosu * face->ry + sinu * orty); /* XXX: Note this is minus, presumably due to PCB's coordinate space */
      *nz = cosu * face->rz + sinu * ortz;
    }
}

static void
cylinder_bo_add_edge (borast_t *bo,
                      double lu, double lv,
                      double  u, double  v,
                      bool is_outer)
{
  /* XXX: Not absolutely sure about this! */
  if (fabs (u - lu) > fabs (u + 360.0f - lu))
    {
#if 1
      bo_add_edge (bo,
                   MM_TO_COORD (lv), MM_TO_COORD (lu),
                   MM_TO_COORD ( v), MM_TO_COORD ( u + 360.0f),
                   is_outer);
#endif
#if 1
      bo_add_edge (bo,
                   MM_TO_COORD (lv), MM_TO_COORD (lu - 360.0f),
                   MM_TO_COORD ( v), MM_TO_COORD ( u),
                   is_outer);
#endif
    }
  else if (fabs (u - lu) > fabs (u - 360.0f - lu))
    {
#if 1
      bo_add_edge (bo,
                   MM_TO_COORD (lv), MM_TO_COORD (lu),
                   MM_TO_COORD ( v), MM_TO_COORD ( u - 360.0f),
                   is_outer);
#endif
#if 1
      bo_add_edge (bo,
                   MM_TO_COORD (lv), MM_TO_COORD (lu + 360.0f),
                   MM_TO_COORD ( v), MM_TO_COORD ( u),
                   is_outer);
#endif
    }
  else
    {
      bo_add_edge (bo,
                   MM_TO_COORD (lv), MM_TO_COORD (lu),
                   MM_TO_COORD ( v), MM_TO_COORD ( u),
                   is_outer);
    }

}
static void
cylinder_ensure_tristrip (face3d *face)
{
  GList *c_iter;
  int num_uv_points;
  float *uv_points;
  int i;
  int vertex_comp;
  contour3d *contour;
  edge_ref e;
  int x1, x2, x3, x4, y_top, y_bot;
//  Vector p_v;
//  VNODE *node;
//  PLINE *p_contour = NULL;
//  POLYAREA *poly;
//  PLINE *dummy_contour;
  borast_t *bo;
  borast_traps_t traps;
//  bool found_outer_contour = false;
//  float u, v;
  int edge_count = 0;

  /* Nothing to do if vertices are already cached */
  if (face->tristrip_vertices != NULL)
    return;

  /* Don't waste time if we failed last time */
  if (face->triangulate_failed)
    return;

  if (!face->is_cylindrical)
    return;

#if 0
  poly = poly_Create ();
  if (poly == NULL)
    return;
#endif

  /* Count up the number of edges space is required for */
  for (c_iter = face->contours; c_iter != NULL; c_iter = g_list_next (c_iter))
    {
      contour = c_iter->data;
      e = contour->first_edge;

      do
        {
          edge_info *info = UNDIR_DATA (e);

          edge_ensure_linearised (e);
          edge_count += info->num_linearised_vertices;
        }
      while ((e = LNEXT(e)) != contour->first_edge);

    }

  /* Worst case, we need 2x number of edges, since we repeat any which span the u=0, u=360 wrap-around. */
  bo = bo_init (2 * edge_count);

  /* Throw the edges to the rasteriser */
  for (c_iter = face->contours; c_iter != NULL; c_iter = g_list_next (c_iter))
    {
      float fu = 0.0f, fv = 0.0f;
      float lu = 0.0f, lv = 0.0f;
      float u, v;
      bool first_vertex = true;
      bool is_outer;

      /* XXX: How can we tell if a contour is inner or outer??? */
      is_outer = true;

      contour = c_iter->data;
      e = contour->first_edge;

      do
        {
          edge_info *info = UNDIR_DATA (e);
          bool backwards_edge;

          /* XXX: Do this without breaking abstraction? */
          /* Detect SYM edges, reverse the circle normal */
          backwards_edge = ((e & 2) == 2);

          edge_ensure_linearised (e);

          for (i = 0; i < info->num_linearised_vertices - 1; i++)
            {
              int vertex_idx = i;

              if (backwards_edge)
                vertex_idx = info->num_linearised_vertices - 1 - i;

              cylinder_xyz_to_uv (face,
                                  info->linearised_vertices[vertex_idx * 3 + 0],
                                  info->linearised_vertices[vertex_idx * 3 + 1],
                                  info->linearised_vertices[vertex_idx * 3 + 2],
                                  &u, &v);

              if (first_vertex)
                {
                  fu = u;
                  fv = v;
                }
              else
                {

                  cylinder_bo_add_edge (bo,
                                        lu, lv,
                                         u,  v,
                                        is_outer);
                }

              lu = u;
              lv = v;
              first_vertex = false;
            }

        }
      while ((e = LNEXT(e)) != contour->first_edge);


      cylinder_bo_add_edge (bo,
                            lu, lv,
                            fu, fv,
                            is_outer);

//      if (!face->surface_orientation_reversed)
//        poly_InvContour (p_contour);

    }

  /* XXX: Need to tesselate the polygon */

  _borast_traps_init (&traps);
  bo_tesselate_to_traps (bo, false /* Don't combine adjacent y traps */,  &traps);

  bo_free (bo);

  num_uv_points = 0;

  for (i = 0; i < traps.num_traps; i++) {
    y_top = traps.traps[i].top;
    y_bot = traps.traps[i].bottom;

    x1 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_top);
    x2 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_top);
    x3 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_bot);
    x4 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_bot);

    if ((x1 == x2) || (x3 == x4)) {
      num_uv_points += 5 + 1; /* Three vertices + repeated start and end, extra repeat to sync backface culling */
    } else {
      num_uv_points += 6; /* Four vertices + repeated start and end */
    }
  }

  if (num_uv_points == 0) {
    printf ("Strange, contour didn't tesselate\n");
    face->triangulate_failed = true;
    return;
  }

//  printf ("Tesselated with %i uv points\n", num_uv_points);

  uv_points = g_new0 (float, 2 * num_uv_points);

  vertex_comp = 0;
  num_uv_points = 0;

  for (i = 0; i < traps.num_traps; i++) {
    y_top = traps.traps[i].top;
    y_bot = traps.traps[i].bottom;

    /* NB: ybot > ytop, as this is all derived from a screen-space rasteriser with 0,0 in the top left */

    /* Exclude strips entirely above or below the 0 <= u <= 360 range */
    if (y_bot < MM_TO_COORD (0.0f))
      continue;

    if (y_top > MM_TO_COORD (360.0f))
      continue;

    /* Clamp evaluation coordinates otherwise (strips straddling the boundary)
     * NB: Due to input parameter-space geometry duplication, the bit we trim
     *     here will be duplicated on the other side of the wrap-around anyway
     */
    y_top = MAX(MM_TO_COORD(0.0f), y_top);
    y_bot = MIN(y_bot, MM_TO_COORD(360.0f));


    if (face->surface_orientation_reversed)
      {
        x2 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_top);
        x1 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_top);
        x4 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_bot);
        x3 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_bot);
      }
    else
      {
        x1 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_top);
        x2 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_top);
        x3 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_bot);
        x4 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_bot);
      }

    if (x1 == x2) {
      /* NB: Repeated first virtex to separate from other tri-strip */
#if 1
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      /* NB: Repeated last virtex to separate from other tri-strip */
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      /* NB: Extra repeated vertex to keep backface culling in sync */

      num_uv_points += 6;
#endif
    } else if (x3 == x4) {
      /* NB: Repeated first virtex to separate from other tri-strip */
#if 1
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x2;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      /* NB: Repeated last virtex to separate from other tri-strip */
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      /* NB: Extra repeated vertex to keep backface culling in sync */

      num_uv_points += 6;
#endif
    } else {
      /* NB: Repeated first virtex to separate from other tri-strip */
      uv_points[vertex_comp++] = x2;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x2;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      /* NB: Repeated last virtex to separate from other tri-strip */

      num_uv_points += 6;
    }
  }

  _borast_traps_fini (&traps);

  /* XXX: Would it be better to use the original vertices?
   *      Rather than converting to u-v coordinates and back.
   *      Probably at least need to use the u-v points to
   *      perform the triangulation.
   */

  face->tristrip_num_vertices = num_uv_points;
  face->tristrip_vertices = g_new0 (float, BUFFER_STRIDE * num_uv_points);

  vertex_comp = 0;
  for (i = 0; i < num_uv_points; i++)
    {
      cylinder_uv_to_xyz_and_normal(face,
                                    /* uv */
                                    COORD_TO_MM (uv_points[2 * i + 1]), /* Inverse of arbitrary transformation above */
                                    COORD_TO_MM (uv_points[2 * i + 0]), /* Inverse of arbitrary transformation above */
                                    /* xyz */
                                    &face->tristrip_vertices[vertex_comp + 0],
                                    &face->tristrip_vertices[vertex_comp + 1],
                                    &face->tristrip_vertices[vertex_comp + 2],
                                    /* Vertex normal */
                                    &face->tristrip_vertices[vertex_comp + 3],
                                    &face->tristrip_vertices[vertex_comp + 4],
                                    &face->tristrip_vertices[vertex_comp + 5]);

      vertex_comp += BUFFER_STRIDE;
    }

  g_free (uv_points);
}

#if 0
static void
cylinder_ensure_tristrip (face3d *face)
{
  GList *c_iter;
  int num_uv_points;
  float *uv_points;
  int i;
  int vertex_comp;
  contour3d *contour;
  edge_ref e;
  int x1, x2, x3, x4, y_top, y_bot;
  Vector p_v;
  VNODE *node;
  PLINE *p_contour = NULL;
  POLYAREA *poly;
  PLINE *dummy_contour;
  borast_traps_t traps;
  bool found_outer_contour = false;
  float u, v;

  /* Nothing to do if vertices are already cached */
  if (face->tristrip_vertices != NULL)
    return;

  /* Don't waste time if we failed last time */
  if (face->triangulate_failed)
    return;

  if (!face->is_cylindrical)
    return;

  poly = poly_Create ();
  if (poly == NULL)
    return;

#if 1
  /* Create a dummy outer contour (so we don't have to worry about the order below..
   * when we encounter the outer contour, we substitute this dummy one for it.
   */
  p_v[0] = 0;
  p_v[1] = 0;
  node = poly_CreateNode (p_v);
  dummy_contour = poly_NewContour (node);
  dummy_contour->Flags.orient = PLF_DIR;
  poly_InclContour (poly, dummy_contour);

  for (c_iter = face->contours; c_iter != NULL; c_iter = g_list_next (c_iter))
    {
      float lost_phase = 0.0;
      bool first_iteration = true;

      int wobble = 0;

      contour = c_iter->data;

      e = contour->first_edge;

      do
        {
          edge_info *info = UNDIR_DATA (e);
          float lu, lv;
          float u, v;
          bool backwards_edge;

          /* XXX: Do this without breaking abstraction? */
          /* Detect SYM edges, reverse the circle normal */
          backwards_edge = ((e & 2) == 2);

          edge_ensure_linearised (e);

          for (i = 0; i < info->num_linearised_vertices - 1; i++)
            {
              int vertex_idx = i;

              if (backwards_edge)
                vertex_idx = info->num_linearised_vertices - 1 - i;

              cylinder_xyz_to_uv (face,
                                  info->linearised_vertices[vertex_idx * 3 + 0],
                                  info->linearised_vertices[vertex_idx * 3 + 1],
                                  info->linearised_vertices[vertex_idx * 3 + 2],
                                  &u, &v);

              /* Add back on any wrapped phase */
              u += lost_phase;

              if (!first_iteration)
                {
//                  printf ("u = %f, delta since last u = %f\n", (double)u, (double)(u - lu));

                  if (fabs (u - lu) > fabs (u + 360.0f - lu))
                    {
//                      printf ("Adding 360 degrees to lost phase\n");
                      lost_phase += 360.0f;
                      u += 360.0f;
                    }
                  else if (fabs (u - lu) > fabs (u - 360.0f - lu))
                    {
//                      printf ("Subtracting 360 degrees to lost phase\n");
                      lost_phase -= 360.0f;
                      u -= 360.0f;
                    }
                }


              wobble = 10 - wobble;

              p_v[0] = MM_TO_COORD (v) + wobble;
              p_v[1] = MM_TO_COORD (u);
              node = poly_CreateNode (p_v);

              if (p_contour == NULL)
                {
                  if ((p_contour = poly_NewContour (node)) == NULL)
                    return;
                }
              else
                {
                  poly_InclVertex (p_contour->head.prev, node);
                }

              lu = u;
              lv = v;
              first_iteration = false;
            }

        }
      while ((e = LNEXT(e)) != contour->first_edge);

      poly_PreContour (p_contour, FALSE);

      /* make sure it is a positive contour (outer) or negative (hole) */
//      if (p_contour->Flags.orient != (hole ? PLF_INV : PLF_DIR))


//      if (p_contour->Flags.orient != PLF_DIR)
      if (!face->surface_orientation_reversed)
        poly_InvContour (p_contour);

      if (p_contour->Flags.orient == PLF_DIR)
        {
          PLINE *old_outer;

          /* Found the outer contour */
          if (found_outer_contour)
            {
              printf ("FOUND TWO OUTER CONTOURS FOR CYLINDRICAL.. NEED TO HANDLE THIS..!\n");
#if 1
              face->triangulate_failed = true;
              return;
#endif
            }

          p_contour->next = poly->contours->next;
          old_outer = poly->contours;
          poly->contours = p_contour;

          found_outer_contour = true;
        }
      else
        {
          if (!poly_InclContour (poly, p_contour))
            {
              printf ("Contour dropped - oops!\n");
              poly_DelContour (&p_contour);
            }
        }
      p_contour = NULL;

      /* XXX: Assumption of outline first, holes second seems to be false! */
//      hole = true;
    }

  if (!found_outer_contour)
    {
      printf ("(CYLINDER) DID NOT FIND OUTER CONTOUR... BADNESS #%i\n", face->face_identifier);
      face->triangulate_failed = true;
      face->is_debug = true;
      return;
    }

  poly_DelContour (&dummy_contour);
#endif

  /* XXX: Need to tesselate the polygon */
  _borast_traps_init (&traps);
  bo_poly_to_traps_no_draw (poly, &traps);

  num_uv_points = 0;

  for (i = 0; i < traps.num_traps; i++) {
    y_top = traps.traps[i].top;
    y_bot = traps.traps[i].bottom;

    x1 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_top);
    x2 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_top);
    x3 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_bot);
    x4 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_bot);

    if ((x1 == x2) || (x3 == x4)) {
      num_uv_points += 5 + 1; /* Three vertices + repeated start and end, extra repeat to sync backface culling */
    } else {
      num_uv_points += 6; /* Four vertices + repeated start and end */
    }
  }

  poly_Free (&poly);

  if (num_uv_points == 0) {
    printf ("Strange, contour didn't tesselate\n");
    face->triangulate_failed = true;
    return;
  }

//  printf ("Tesselated with %i uv points\n", num_uv_points);

  uv_points = g_new0 (float, 2 * num_uv_points);

  vertex_comp = 0;
  for (i = 0; i < traps.num_traps; i++) {
    y_top = traps.traps[i].top;
    y_bot = traps.traps[i].bottom;

    if (face->surface_orientation_reversed)
      {
        x2 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_top) - COORD_TO_MM(y_top); /* Subtract of COORD_TO_MM(y_top) undoes hack above which kept the tesselator from combining strips across y */
        x1 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_top) - COORD_TO_MM(y_top);
        x4 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_bot) - COORD_TO_MM(y_bot);
        x3 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_bot) - COORD_TO_MM(y_bot);
      }
    else
      {
        x1 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_top) - COORD_TO_MM(y_top); /* Subtract of COORD_TO_MM(y_top) undoes hack above which kept the tesselator from combining strips across y */
        x2 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_top) - COORD_TO_MM(y_top);
        x3 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_bot) - COORD_TO_MM(y_bot);
        x4 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_bot) - COORD_TO_MM(y_bot);
      }

    if (x1 == x2) {
      /* NB: Repeated first virtex to separate from other tri-strip */
#if 1
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      /* NB: Repeated last virtex to separate from other tri-strip */
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      /* NB: Extra repeated vertex to keep backface culling in sync */
#endif
    } else if (x3 == x4) {
      /* NB: Repeated first virtex to separate from other tri-strip */
#if 1
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x2;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      /* NB: Repeated last virtex to separate from other tri-strip */
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      /* NB: Extra repeated vertex to keep backface culling in sync */
#endif
    } else {
      /* NB: Repeated first virtex to separate from other tri-strip */
      uv_points[vertex_comp++] = x2;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x2;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      /* NB: Repeated last virtex to separate from other tri-strip */
    }
  }

  _borast_traps_fini (&traps);

  /* XXX: Would it be better to use the original vertices?
   *      Rather than converting to u-v coordinates and back.
   *      Probably at least need to use the u-v points to
   *      perform the triangulation.
   */

  face->tristrip_num_vertices = num_uv_points;
  face->tristrip_vertices = g_new0 (float, BUFFER_STRIDE * num_uv_points);

  vertex_comp = 0;
  for (i = 0; i < num_uv_points; i++)
    {
      cylinder_uv_to_xyz_and_normal(face,
                                    /* uv */
                                    COORD_TO_MM (uv_points[2 * i + 1]), /* Inverse of arbitrary transformation above */
                                    COORD_TO_MM (uv_points[2 * i + 0]), /* Inverse of arbitrary transformation above */
                                    /* xyz */
                                    &face->tristrip_vertices[vertex_comp + 0],
                                    &face->tristrip_vertices[vertex_comp + 1],
                                    &face->tristrip_vertices[vertex_comp + 2],
                                    /* Vertex normal */
                                    &face->tristrip_vertices[vertex_comp + 3],
                                    &face->tristrip_vertices[vertex_comp + 4],
                                    &face->tristrip_vertices[vertex_comp + 5]);

#if 0
      printf ("%f, %f  ->  %f, %f, %f\n",
                         COORD_TO_MM (uv_points[2 * i + 1]), /* Inverse of arbitrary transformation above */
                         COORD_TO_MM (uv_points[2 * i + 0]), /* Inverse of arbitrary transformation above */
                         (double)face->tristrip_vertices[vertex_comp + 0],
                         (double)face->tristrip_vertices[vertex_comp + 1],
                         (double)face->tristrip_vertices[vertex_comp + 2]);
#endif
      vertex_comp += BUFFER_STRIDE;
    }

  g_free (uv_points);
}
#endif

static void
plane_xyz_to_uv (face3d *face, float x, float y, float z, float *u, float *v)
{
  double ortx, orty, ortz;

  ortx = face->ay * face->rz - face->az * face->ry;
  orty = face->az * face->rx - face->ax * face->rz;
  ortz = face->ax * face->ry - face->ay * face->rx;

  *u = (x - face->ox) * face->rx +
       (y - face->oy) * face->ry +
       (z - face->oz) * face->rz;

  *v = (x - face->ox) * ortx +
       (y - face->oy) * orty +
       (z - face->oz) * ortz;
}

static void
plane_uv_to_xyz_and_normal (face3d *face, float u, float v, float *x, float *y, float *z,
                            float *nx, float *ny, float *nz)
{
  float ortx, orty, ortz;

  ortx = face->ay * face->rz - face->az * face->ry;
  orty = face->az * face->rx - face->ax * face->rz;
  ortz = face->ax * face->ry - face->ay * face->rx;

  *x = STEP_X_TO_COORD(PCB, face->ox + u * face->rx + v * ortx);
  *y = STEP_Y_TO_COORD(PCB, face->oy + u * face->ry + v * orty);
  *z = STEP_Z_TO_COORD(PCB, face->oz + u * face->rz + v * ortz);

  *nx =  face->ax;
  *ny = -face->ay; /* XXX: Note this is minus, presumably due to PCB's coordinate space */
  *nz =  face->az;

  if (face->surface_orientation_reversed)
    {
      *nx = -*nx;
      *ny = -*ny;
      *nz = -*nz;
    }
}

static void
plane_ensure_tristrip (face3d *face)
{
  GList *c_iter;
  int num_uv_points;
  float *uv_points;
  int i;
  int vertex_comp;
  contour3d *contour;
  edge_ref e;
  int x1, x2, x3, x4, y_top, y_bot;
  Vector p_v;
  VNODE *node;
  PLINE *p_contour = NULL;
  POLYAREA *poly;
  PLINE *dummy_contour;
  borast_traps_t traps;
  bool found_outer_contour = false;

  /* Nothing to do if vertices are already cached */
  if (face->tristrip_vertices != NULL)
    return;

  /* Don't waste time if we failed last time */
  if (face->triangulate_failed)
    return;

  if (!face->is_planar)
    return;

  poly = poly_Create ();
  if (poly == NULL)
    return;

  /* Create a dummy outer contour (so we don't have to worry about the order below..
   * when we encounter the outer contour, we substitute this dummy one for it.
   */
  p_v[0] = 0;
  p_v[1] = 0;
  node = poly_CreateNode (p_v);
  dummy_contour = poly_NewContour (node);
  dummy_contour->Flags.orient = PLF_DIR;
  poly_InclContour (poly, dummy_contour);

  for (c_iter = face->contours; c_iter != NULL; c_iter = g_list_next (c_iter))
    {
      contour = c_iter->data;

      e = contour->first_edge;

      do
        {
          edge_info *info = UNDIR_DATA (e);
          float u, v;
          bool backwards_edge;

          /* XXX: Do this without breaking abstraction? */
          /* Detect SYM edges, reverse the circle normal */
          backwards_edge = ((e & 2) == 2);

          edge_ensure_linearised (e);

          for (i = 0; i < info->num_linearised_vertices - 1; i++)
            {
              int vertex_idx = i;

              if (backwards_edge)
                vertex_idx = info->num_linearised_vertices - 1 - i;

              plane_xyz_to_uv (face,
                               info->linearised_vertices[vertex_idx * 3 + 0],
                               info->linearised_vertices[vertex_idx * 3 + 1],
                               info->linearised_vertices[vertex_idx * 3 + 2],
                               &u, &v);

              p_v[0] = MM_TO_COORD (u);
              p_v[1] = MM_TO_COORD (v);
              node = poly_CreateNode (p_v);

              if (p_contour == NULL)
                {
                  if ((p_contour = poly_NewContour (node)) == NULL)
                    return;
                }
              else
                {
                  poly_InclVertex (p_contour->head.prev, node);
                }
            }
        }
      while ((e = LNEXT(e)) != contour->first_edge);

      poly_PreContour (p_contour, FALSE);

      /* make sure it is a positive contour (outer) or negative (hole) */
//      if (p_contour->Flags.orient != (hole ? PLF_INV : PLF_DIR))
      if (face->surface_orientation_reversed)
        poly_InvContour (p_contour);

      if (p_contour->Flags.orient == PLF_DIR)
        {
//          PLINE *old_outer;

          /* Found the outer contour */
          if (found_outer_contour)
            {
              printf ("FOUND TWO OUTER CONTOURS FOR PLANAR FACE.. WILL END BADLY!\n");
              face->triangulate_failed = true;
              return;
            }

          p_contour->next = poly->contours->next;
//          old_outer = poly->contours;
          poly->contours = p_contour;

          found_outer_contour = true;
        }
      else
        {
          if (!poly_InclContour (poly, p_contour))
            {
              printf ("Contour dropped - oops!\n");
              poly_DelContour (&p_contour);
            }
        }
      p_contour = NULL;

      /* XXX: Assumption of outline first, holes second seems to be false! */
//      hole = true;
    }

  if (!found_outer_contour)
    {
      printf ("(PLANE) DID NOT FIND OUTER CONTOUR... BADNESS\n");
      face->triangulate_failed = true;
      return;
    }

  poly_DelContour (&dummy_contour);

  /* XXX: Need to tesselate the polygon */
  _borast_traps_init (&traps);
  bo_poly_to_traps_no_draw (poly, &traps);

  num_uv_points = 0;

  for (i = 0; i < traps.num_traps; i++) {
    y_top = traps.traps[i].top;
    y_bot = traps.traps[i].bottom;

    x1 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_top);
    x2 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_top);
    x3 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_bot);
    x4 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_bot);

    if ((x1 == x2) || (x3 == x4)) {
      num_uv_points += 5 + 1; /* Three vertices + repeated start and end, extra repeat to sync backface culling */
    } else {
      num_uv_points += 6; /* Four vertices + repeated start and end */
    }
  }

  poly_Free (&poly);

  if (num_uv_points == 0) {
//    printf ("Strange, contour didn't tesselate\n");
    face->triangulate_failed = true;
    return;
  }

//  printf ("Tesselated with %i uv points\n", num_uv_points);

  uv_points = g_new0 (float, 2 * num_uv_points);

  vertex_comp = 0;
  for (i = 0; i < traps.num_traps; i++) {
    y_top = traps.traps[i].top;
    y_bot = traps.traps[i].bottom;

    if (face->surface_orientation_reversed)
      {
        x2 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_top);
        x1 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_top);
        x4 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_bot);
        x3 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_bot);
      }
    else
      {
        x1 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_top);
        x2 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_top);
        x3 = _line_compute_intersection_x_for_y (&traps.traps[i].right, y_bot);
        x4 = _line_compute_intersection_x_for_y (&traps.traps[i].left,  y_bot);
      }

    if (x1 == x2) {
      /* NB: Repeated first virtex to separate from other tri-strip */
#if 1
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      /* NB: Repeated last virtex to separate from other tri-strip */
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      /* NB: Extra repeated vertex to keep backface culling in sync */
#endif
    } else if (x3 == x4) {
      /* NB: Repeated first virtex to separate from other tri-strip */
#if 1
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x2;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x2;  uv_points[vertex_comp++] = y_top;
      /* NB: Repeated last virtex to separate from other tri-strip */
      uv_points[vertex_comp++] = x2;  uv_points[vertex_comp++] = y_top;
      /* NB: Extra repeated vertex to keep backface culling in sync */
#endif
    } else {
      /* NB: Repeated first virtex to separate from other tri-strip */
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x3;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x2;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x4;  uv_points[vertex_comp++] = y_bot;
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      uv_points[vertex_comp++] = x1;  uv_points[vertex_comp++] = y_top;
      /* NB: Repeated last virtex to separate from other tri-strip */
    }
  }

  _borast_traps_fini (&traps);

  /* XXX: Would it be better to use the original vertices?
   *      Rather than converting to u-v coordinates and back.
   *      Probably at least need to use the u-v points to
   *      perform the triangulation.
   */

  face->tristrip_num_vertices = num_uv_points;
  face->tristrip_vertices = g_new0 (float, BUFFER_STRIDE * num_uv_points);

  vertex_comp = 0;
  for (i = 0; i < num_uv_points; i++)
    {
      plane_uv_to_xyz_and_normal (face,
                                  COORD_TO_MM (uv_points[2 * i + 0]),
                                  COORD_TO_MM (uv_points[2 * i + 1]),
                                  &face->tristrip_vertices[vertex_comp + 0],
                                  &face->tristrip_vertices[vertex_comp + 1],
                                  &face->tristrip_vertices[vertex_comp + 2],
                                  /* Vertex normal */
                                  &face->tristrip_vertices[vertex_comp + 3],
                                  &face->tristrip_vertices[vertex_comp + 4],
                                  &face->tristrip_vertices[vertex_comp + 5]);

      vertex_comp += BUFFER_STRIDE;
    }

  g_free (uv_points);
}

void
face3d_fill(hidGC gc, face3d *face, bool selected)
{
  hidglGC hidgl_gc = (hidglGC)gc;
  hidgl_instance *hidgl = hidgl_gc->hidgl;
#ifdef MEMCPY_VERTEX_DATA
  hidgl_priv *priv = hidgl->priv;
#endif

  hidgl_flush_triangles (hidgl);

  if (face->is_planar)
    {
      plane_ensure_tristrip (face);
    }
  else if (face->is_cylindrical)
    {
      cylinder_ensure_tristrip (face);
    }
  else if (face->is_conical)
    {
      cone_ensure_tristrip (face);
    }
  else if (face->is_toroidal)
    {
//      toroid_ensure_tristrip (face);
    }
  else
    {
      /* We only know how to deal with planar and cylindrical faces for now */
      return;
    }

#if 0
  if (face->is_debug)
    glColor4f (1.0f, 0.0f, 0.0f, 0.5f);
  else if (selected)
    glColor4f (0.0f, 1.0f, 1.0f, 0.5f);
//  else if (face->is_cylindrical)
//    glColor4f (0.0f, 1.0f, 0.0f, 0.5f);
  else
    glColor4f (0.8f, 0.8f, 0.8f, 1.0f);
//    glColor4f (0.8f, 0.8f, 0.8f, 0.3f);
#endif

  emit_tristrip (face);

  if (face->is_debug)
    emit_lines (face);
}
