/* TODO ITEMS:
 *
 * Add PLINE simplification operation to consolidate co-circular segments for reduced geometry output.
 * Look at whether arc-* intersections can be re-constructed back to original geometry, not fall back to line-line.
 * Work on snap-rounding any edge which passes through the pixel square containing an any vertex (or intersection).
 * Avoid self-touching output in contours, where that self-touching instance creates two otherwise distinct contours or holes.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <glib.h>

#include "data.h"
#include "step_id.h"
#include "quad.h"
#include "vertex3d.h"
#include "contour3d.h"
#include "appearance.h"
#include "face3d.h"
#include "edge3d.h"
#include "object3d.h"
#include "polygon.h"
#include "rats.h"

#include "rtree.h"
#include "rotate.h"

#include "pcb-printf.h"
#include "misc.h"
#include "hid/hidint.h"

#define PERFECT_ROUND_CONTOURS
#undef SUM_PINS_VIAS_ONCE /* Cannot do this for BB Vias, each inter-layer is unique */
#define HASH_OBJECTS

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

static Coord board_thickness;
#define HACK_BOARD_THICKNESS board_thickness
//#define HACK_BOARD_THICKNESS MM_TO_COORD(1.6)
#define HACK_COPPER_THICKNESS MM_TO_COORD(0.035)
#define HACK_PLATED_BARREL_THICKNESS MM_TO_COORD(0.08)
#define HACK_MASK_THICKNESS MM_TO_COORD(0.01)
#define HACK_SILK_THICKNESS MM_TO_COORD(0.01)

object3d *
make_object3d (char *name)
{
  static int object3d_count = 0;
  object3d *object;

  object = g_slice_new0 (object3d);
  object->id = object3d_count++;
  object->name = g_strdup (name);

  return object;
}

void
destroy_object3d (object3d *object)
{
  g_list_free_full (object->vertices, (GDestroyNotify)destroy_vertex3d);
  g_list_free_full (object->edges, (GDestroyNotify)destroy_edge);
  g_list_free_full (object->faces, (GDestroyNotify)destroy_face3d);
  g_free (object->name);
  g_slice_free (object3d, object);
}

void
object3d_set_appearance (object3d *object, appearance *appear)
{
  object->appear = appear;
}

void
object3d_add_edge (object3d *object, edge_ref edge)
{
  object->edges = g_list_prepend (object->edges, (void *)edge);
}

void
object3d_add_vertex (object3d *object, vertex3d *vertex)
{
  object->vertices = g_list_prepend (object->vertices, vertex);
}

void
object3d_add_face (object3d *object, face3d *face)
{
  object->faces = g_list_prepend (object->faces, face);
}


/*********************************************************************************************************/

static int
get_contour_npoints (PLINE *contour)
{
#ifdef PERFECT_ROUND_CONTOURS
  /* HACK FOR ROUND CONTOURS */
  if (contour->is_round)
    return 1;
#endif

  return contour->Count;
}

static void
get_contour_coord_n_in_step_mm (PLINE *contour, int n, double *x, double *y)
{
  VNODE *vertex = &contour->head;

#ifdef PERFECT_ROUND_CONTOURS
  if (contour->is_round)
    {
      /* HACK SPECIAL CASE FOR ROUND CONTOURS */

      /* We define an arbitrary point on the contour. This is used, for example,
       * to define a coordinate system along the contour, and coincides with where
       * we add a straight edge down the side of an extruded cylindrical shape.
       */
      *x = COORD_TO_STEP_X (PCB, contour->cx - contour->radius);
      *y = COORD_TO_STEP_Y (PCB, contour->cy);

      return;
    }
#endif

  while (n > 0)
    {
      vertex = vertex->next; /* The VNODE structure is circularly linked, so wrapping is OK */
      n--;
    }

  *x = COORD_TO_STEP_X (PCB, vertex->point[0]);
  *y = COORD_TO_STEP_Y (PCB, vertex->point[1]);
}

static bool
get_contour_edge_n_is_round (PLINE *contour, int n)
{
  VNODE *edge = &contour->head;

#ifdef PERFECT_ROUND_CONTOURS
  if (contour->is_round)
    {
      /* HACK SPECIAL CASE FOR ROUND CONTOURS */
      return true;
    }
#endif

  while (n > 0)
    {
      edge = edge->next; /* The VNODE structure is circularly linked, so wrapping is OK */
      n--;
    }

  return edge->is_round;
}

/* Copied from polygon1.c */
#define Vsub2(r,a,b)	{(r)[0] = (a)[0] - (b)[0]; (r)[1] = (a)[1] - (b)[1];}
#define EDGE_BACKWARD_VERTEX(e) ((e))
#define EDGE_FORWARD_VERTEX(e) ((e)->next)

static int compare_ccw_cw (Vector a, Vector b, Vector c)
{
  double cross;
  Vector ab;
  Vector ac;

  Vsub2 (ab, b, a);
  Vsub2 (ac, c, a);

  cross = (double) ab[0] * ac[1] - (double) ac[0] * ab[1];
  if (cross > 0.0)
    return 1;
  else if (cross < 0.0)
    return -1;
  else
    return 0;
}

static void
get_contour_edge_n_round_geometry_in_step_mm (PLINE *contour, int n, double *cx, double *cy, double *r, bool *cw)
{
  VNODE *edge = &contour->head;
  Vector center;

#ifdef PERFECT_ROUND_CONTOURS
  if (contour->is_round)
    {
      /* HACK SPECIAL CASE FOR ROUND CONTOURS */
      *cx = COORD_TO_STEP_X (PCB, contour->cx);
      *cy = COORD_TO_STEP_Y (PCB, contour->cy);
      *r = COORD_TO_MM (contour->radius);
      *cw = (contour->Flags.orient == PLF_INV);
      return;
    }
#endif

  while (n > 0)
    {
      edge = edge->next; /* The VNODE structure is circularly linked, so wrapping is OK */
      n--;
    }

  center[0] = edge->cx;
  center[1] = edge->cy;

  *cx = COORD_TO_STEP_X (PCB, edge->cx);
  *cy = COORD_TO_STEP_Y (PCB, edge->cy);
  *r = COORD_TO_MM (edge->radius);
  *cw = (compare_ccw_cw (EDGE_BACKWARD_VERTEX (edge)->point, center, EDGE_FORWARD_VERTEX (edge)->point) > 0);
}

static char *
make_object_name (char *base_name, char *suffix_name)
{
  if (base_name == NULL)
    {
      return g_strdup (suffix_name);
    }
  else
    {
      if (suffix_name == NULL)
        return g_strdup (base_name);
      else
        return g_strdup_printf ("%s - %s", base_name, suffix_name);
    }
}

typedef struct
{
  object3d *object;
  face3d *top_face;
  face3d *bottom_face;
} polygon_3d_link;

/* NOTE: This function sets the user_data pointer on POLYAREA it
 *       converts, to point at a polygon_3d_link structure
 *       referencing the generated object and upper/lower faces
 */
GList *
object3d_from_contours (POLYAREA *contours,
                        double zbot,
                        double ztop,
                        const appearance *master_object_appearance,
                        const appearance *master_top_bot_appearance,
                        bool extrude_inverted,
                        char *base_name)
{
  GList *objects = NULL;
  object3d *object;
  appearance *object_appearance = NULL;
  appearance *top_bot_appearance = NULL;
  POLYAREA *pa;
  PLINE *outer_contour;
  PLINE *ct;
  int ncontours;
  int npoints;
  int i;
  vertex3d **vertices;
  edge_ref *edges;
  face3d **faces;
  int start_of_ct;
  int offset_in_ct;
  int ct_npoints;
  polygon_3d_link *link;
  bool invert_face_normals;
  double length;
  double nx, ny;
  char *object_name;

#ifdef REVERSED_PCB_CONTOURS
  invert_face_normals = extrude_inverted ? false : true;
#else
  invert_face_normals = extrude_inverted ? true : false;
#endif

  if (contours == NULL)
    return NULL;

  /* Loop over all board outline pieces */
  pa = contours;
  do
    {

      outer_contour = pa->simple_contours;
      ncontours = 0;
      npoints = 0;

      ct = outer_contour;
      while (ct != NULL) {
        ncontours ++;
        npoints += get_contour_npoints (ct);
        ct = ct->next;
      }

      object_name = make_object_name (base_name, pa->simple_contours->name);
      object = make_object3d (object_name);
      g_free (object_name);

#if 0
      /* XXX: REF-COUNTING WOULD BE WAY BETTER! */
      if (master_object_appearance != NULL)
        {
          object_appearance = make_appearance ();
          appearance_set_appearance (object_appearance, master_object_appearance);
        }

      /* XXX: REF-COUNTING WOULD BE WAY BETTER! */
      if (master_top_bot_appearance != NULL)
        {
          top_bot_appearance = make_appearance ();
          appearance_set_appearance (top_bot_appearance, master_top_bot_appearance);
        }
#else
#endif
      object_appearance = master_object_appearance;
      top_bot_appearance = master_top_bot_appearance;

      object3d_set_appearance (object, object_appearance);

      vertices = malloc (sizeof (vertex3d *) * 2 * npoints); /* (n-bottom, n-top) */
      edges    = malloc (sizeof (edge_ref  ) * 3 * npoints); /* (n-bottom, n-top, n-sides) */
      faces    = malloc (sizeof (face3d *) * (npoints + 2)); /* (n-sides, 1-bottom, 1-top */

      /* Define the vertices */
      ct = outer_contour;
      offset_in_ct = 0;
      ct_npoints = get_contour_npoints (ct);

      for (i = 0; i < npoints; i++, offset_in_ct++)
        {
          double x1, y1;

          /* Update which contour we're looking at */
          if (offset_in_ct == ct_npoints)
            {
              offset_in_ct = 0;
              ct = ct->next;
              ct_npoints = get_contour_npoints (ct);
            }

          get_contour_coord_n_in_step_mm (ct, offset_in_ct, &x1, &y1);

          vertices[i]           = make_vertex3d (x1, y1, COORD_TO_STEP_Z (PCB, zbot)); /* Bottom */
          vertices[npoints + i] = make_vertex3d (x1, y1, COORD_TO_STEP_Z (PCB, ztop)); /* Top */

          object3d_add_vertex (object, vertices[i]);
          object3d_add_vertex (object, vertices[npoints + i]);
        }

      /* Define the edges */
      for (i = 0; i < 3 * npoints; i++)
        {
          edges[i] = make_edge ();
          UNDIR_DATA (edges[i]) = make_edge_info ();
          object3d_add_edge (object, edges[i]);
        }

      /* Define the faces */
      for (i = 0; i < npoints; i++)
        {
          faces[i] = make_face3d (NULL);

          object3d_add_face (object, faces[i]);
          /* Pick one of the upright edges which is within this face outer contour loop, and link it to the face */
          if (!extrude_inverted)
            face3d_add_contour (faces[i], make_contour3d (edges[2 * npoints + i]));
          else
            face3d_add_contour (faces[i], make_contour3d (SYM(edges[2 * npoints + i])));
        }

      faces[npoints    ] = make_face3d ("Bottom"); /* bottom_face */
      faces[npoints + 1] = make_face3d ("Top");    /* top_face */
      if (invert_face_normals)
        {
          face3d_set_normal (faces[npoints    ], 0., 0., -1.); /* bottom_face */
          face3d_set_normal (faces[npoints + 1], 0., 0.,  1.); /* top_face */
        }
      else
        {
          face3d_set_normal (faces[npoints    ], 0., 0.,  1.); /* bottom_face */ /* PCB bottom is at positive Z in this scheme */
          face3d_set_normal (faces[npoints + 1], 0., 0., -1.); /* top_face */    /* PCB top is at negative Z in this scheme */
        }
      face3d_set_appearance (faces[npoints    ], top_bot_appearance);
      face3d_set_appearance (faces[npoints + 1], top_bot_appearance);
      object3d_add_face (object, faces[npoints    ]);
      object3d_add_face (object, faces[npoints + 1]);

      /* Pick the first bottom / top edge within the bottom / top face outer contour loop, and link it to the face */
      if (!extrude_inverted)
        {
          face3d_add_contour (faces[npoints    ], make_contour3d (edges[0]));
          face3d_add_contour (faces[npoints + 1], make_contour3d (SYM(edges[npoints])));
        }
      else
        {
          face3d_add_contour (faces[npoints    ], make_contour3d (SYM(edges[0])));
          face3d_add_contour (faces[npoints + 1], make_contour3d (edges[npoints]));
        }

      ct = outer_contour;
      start_of_ct = 0;
      offset_in_ct = 0;
      ct_npoints = get_contour_npoints (ct);

      for (i = 0; i < npoints; i++, offset_in_ct++)
        {
          int next_i_around_ct;
          int prev_i_around_ct;

          /* Update which contour we're looking at */
          if (offset_in_ct == ct_npoints)
            {
              start_of_ct = i;
              offset_in_ct = 0;
              ct = ct->next;
              ct_npoints = get_contour_npoints (ct);

              /* If there is more than one contour, it will be an inner contour of the bottom and top faces. Refer to it here */
              /* XXX: Haven't properly thought through how (if) inverting works with multiple contours */
              if (!extrude_inverted)
                {
                  face3d_add_contour (faces[npoints    ], make_contour3d (edges[i]));
                  face3d_add_contour (faces[npoints + 1], make_contour3d (SYM(edges[npoints + i])));
                }
              else
                {
                  face3d_add_contour (faces[npoints    ], make_contour3d (SYM(edges[i])));
                  face3d_add_contour (faces[npoints + 1], make_contour3d (edges[npoints + i]));
                }
            }

          next_i_around_ct = start_of_ct + (offset_in_ct + 1) % ct_npoints;
          prev_i_around_ct = start_of_ct + (offset_in_ct + ct_npoints - 1) % ct_npoints;

          /* Setup the face normals for the edges along the contour extrusion (top and bottom are handled separaetely) */
          /* Define the (non-normalized) face normal to point to the outside of the contour */
          /* Vertex ordering of the edge we're finding the normal to is reversed in this case */

          nx =  (vertices[next_i_around_ct]->y - vertices[i]->y);
          ny = -(vertices[next_i_around_ct]->x - vertices[i]->x);
          length = hypot (nx, ny);
          nx /= length;
          ny /= length;

          if (invert_face_normals)
            {
              nx = -nx;
              ny = -ny;
            }

          face3d_set_normal (faces[i], nx, ny, 0.);

          /* Assign the appropriate vertex geometric data to each edge end */
          ODATA (edges[              i]) = vertices[i];
          DDATA (edges[              i]) = vertices[next_i_around_ct];
          ODATA (edges[1 * npoints + i]) = vertices[1 * npoints + i];
          DDATA (edges[1 * npoints + i]) = vertices[1 * npoints + next_i_around_ct];
          ODATA (edges[2 * npoints + i]) = vertices[i];
          DDATA (edges[2 * npoints + i]) = vertices[1 * npoints + i];

          if (get_contour_edge_n_is_round (ct, offset_in_ct))
            {
              double cx;
              double cy;
              double radius;
              double normal_z;
              bool cw;

              get_contour_edge_n_round_geometry_in_step_mm (ct, offset_in_ct, &cx, &cy, &radius, &cw);

              /* NOTE: Axis directon not depend on whether we invert the top/bot contour.. the edge loop is appropriate already */
              face3d_set_cylindrical (faces[i], cx, cy, 0., /* A point on the axis of the cylinder */
                                                0., 0., 1., /* Direction of the cylindrical axis */ /* XXX HAD THIS AT -1 when last testing with Solidworks? */
                                                radius);

              /* NOTE: Surface orientation is only fixed up during emission if we flag the need here..
               *       cylindrical surface orientation is always pointing outward from its axis, so
               *       orientation reversed is used for holes and convave contour segments.
               */
              if (cw != extrude_inverted)
                face3d_set_surface_orientation_reversed (faces[i]);

              face3d_set_normal (faces[i], 1., 0., 0.);  /* A normal to the axis direction */
                                        /* XXX: ^^^ Could line this up with the direction to the vertex in the corresponding circle edge */

              /* DOES NOT DEPEND ON WHETHER WE INVERT THE CONTOUR.. THE EDGE TRAVERSAL IS REVERSED DURING EMISSION.
               * Only depends on the coordinate system transform, and what Z values it requires to create a
               * clockwise / counterclockwise circular edge consistent with the the polygon data.
               */
#ifdef REVERSED_PCB_CONTOURS
              normal_z = cw ? 1. : -1.; /* NORMAL POINTING TO -VE Z MAKES CIRCLE CLOCKWISE */
#else
              normal_z = cw ? -1. : 1.; /* NORMAL POINTING TO -VE Z MAKES CIRCLE CLOCKWISE */
#endif

              edge_info_set_round (UNDIR_DATA (edges[i]),
                                   cx, cy, COORD_TO_STEP_Z (PCB, zbot), /* Center of circle */ /* BOTTOM */
                                   0., 0., normal_z, /* Normal */ radius);
              edge_info_set_round (UNDIR_DATA (edges[npoints + i]),
                                   cx, cy, COORD_TO_STEP_Z (PCB, ztop), /* Center of circle */ /* TOP */
                                   0., 0., normal_z, /* Normal */ radius);
              if (ct->is_round)
                edge_info_set_stitch (UNDIR_DATA (edges[2 * npoints + i]));
            }

          /* NB: Contours are counter clockwise in XY plane.
           *     edges[          0-npoints-1] are the base of the extrusion, following in the counter clockwise order
           *     edges[1*npoints-2*npoints-1] are the top  of the extrusion, following in the counter clockwise order
           *     edges[2*npoints-3*npoints-1] are the upright edges, oriented from bottom to top
           */

          if (extrude_inverted)
            {
              LDATA (edges[              i]) = faces[i];
              RDATA (edges[              i]) = faces[npoints];
              LDATA (edges[1 * npoints + i]) = faces[npoints + 1];
              RDATA (edges[1 * npoints + i]) = faces[i];
              LDATA (edges[2 * npoints + i]) = faces[prev_i_around_ct];
              RDATA (edges[2 * npoints + i]) = faces[i];

              /* Link edges orbiting around each bottom vertex i (0 <= i < npoints) */
              splice (SYM(edges[prev_i_around_ct]), edges[i]);
              splice (edges[i], edges[2 * npoints + i]);
              /* Link edges orbiting around each top vertex (npoints + i) (0 <= i < npoints) */
              splice (edges[npoints + i], SYM(edges[npoints + prev_i_around_ct]));
              splice (SYM(edges[npoints + prev_i_around_ct]), SYM(edges[2 * npoints + i]));
            }
          else
            {
              RDATA (edges[              i]) = faces[i];
              LDATA (edges[              i]) = faces[npoints];
              RDATA (edges[1 * npoints + i]) = faces[npoints + 1];
              LDATA (edges[1 * npoints + i]) = faces[i];
              RDATA (edges[2 * npoints + i]) = faces[prev_i_around_ct];
              LDATA (edges[2 * npoints + i]) = faces[i];

              /* Link edges orbiting around each bottom vertex i (0 <= i < npoints) */
              splice (SYM(edges[prev_i_around_ct]), edges[2 * npoints + i]);
              splice (edges[2 * npoints + i], edges[i]);
              /* Link edges orbiting around each top vertex (npoints + i) (0 <= i < npoints) */
              splice (edges[npoints + i], SYM(edges[2 * npoints + i]));
              splice (SYM(edges[2 * npoints + i]), SYM(edges[npoints + prev_i_around_ct]));
            }
        }

#ifndef NDEBUG
      ct = outer_contour;
      start_of_ct = 0;
      offset_in_ct = 0;
      ct_npoints = get_contour_npoints (ct);

      for (i = 0; i < npoints; i++, offset_in_ct++)
        {
          int next_i_around_ct;
          int prev_i_around_ct;

          /* Update which contour we're looking at */
          if (offset_in_ct == ct_npoints)
            {
              start_of_ct = i;
              offset_in_ct = 0;
              ct = ct->next;
              ct_npoints = get_contour_npoints (ct);
            }

          next_i_around_ct = start_of_ct + (offset_in_ct + 1) % ct_npoints;
          prev_i_around_ct = start_of_ct + (offset_in_ct + ct_npoints - 1) % ct_npoints;

          if (!extrude_inverted)
            {
              g_assert (RDATA (edges[              i]) == faces[i]);
              g_assert (LDATA (edges[              i]) == faces[npoints]);
              g_assert (RDATA (edges[1 * npoints + i]) == faces[npoints + 1]);
              g_assert (LDATA (edges[1 * npoints + i]) == faces[i]);
              g_assert (RDATA (edges[2 * npoints + i]) == faces[prev_i_around_ct]);
              g_assert (LDATA (edges[2 * npoints + i]) == faces[i]);

              g_assert (              ONEXT (edges[              i])   == SYM (edges[prev_i_around_ct]));
              g_assert (       ONEXT (ONEXT (edges[              i]))  == edges[2 * npoints + i]);
              g_assert (ONEXT (ONEXT (ONEXT (edges[              i]))) ==      edges[              i]);
              g_assert (              ONEXT (edges[1 * npoints + i])   == SYM (edges[2 * npoints + i]));
              g_assert (       ONEXT (ONEXT (edges[1 * npoints + i]))  == SYM (edges[1 * npoints + prev_i_around_ct]));
              g_assert (ONEXT (ONEXT (ONEXT (edges[1 * npoints + i]))) ==      edges[1 * npoints + i]);

              g_assert (LNEXT (edges[              i]) ==      edges[0 * npoints + next_i_around_ct]);
              g_assert (LNEXT (edges[1 * npoints + i]) == SYM (edges[2 * npoints + next_i_around_ct]));
              g_assert (LNEXT (edges[2 * npoints + i]) ==      edges[1 * npoints + i]);
            }
          else
            {
              /* XXX: No debug checks for this yet. LDATA and RDATA should be swapped from the
               *      above case, and ONEXT order should be reversed. It works, so have not
               *      written in the debug checks.
               */
            }
        }
#endif

      objects = g_list_prepend (objects, object);

      link = g_slice_new0 (polygon_3d_link);
      pa->user_data = link;
      link->object = object;
      link->bottom_face = faces[npoints];
      link->top_face = faces[npoints + 1];

      object->user_data = g_list_prepend ((GList *)object->user_data, link);
    }
  while (pa = pa->f, pa != contours);

  return objects;
}

GList *
object3d_from_board_outline (void)
{
  POLYAREA *board_outline = board_outline_poly (true);
  POLYAREA *pa;

#if 0
  return object3d_from_soldermask_within_area (board_outline, TOP_SIDE);
#else

  if (board_outline == NULL)
    return NULL;

  /* Erase outer contour naming from the polygon - we don't want all the
   * hole names that get combined into the outer name! */
  pa = board_outline;
  do
    {
      free (pa->contours->name);
      pa->contours->name = NULL;
    }
  while ((pa = pa->f) != board_outline);

  appearance *board_appearance;
  appearance *top_bot_appearance;
  GList *objects;

  board_appearance = make_appearance ();
  top_bot_appearance = NULL;
  appearance_set_color (board_appearance,   1.0, 1.0, 0.6);

  poly_Simplify (board_outline);

  objects = object3d_from_contours (board_outline,
#ifdef REVERSED_PCB_CONTOURS
                                    -HACK_BOARD_THICKNESS, /* Bottom */
                                    0                    ,  /* Top */
#else
                                     HACK_BOARD_THICKNESS / 2, /* Bottom */
                                    -HACK_BOARD_THICKNESS / 2, /* Top */
#endif
                                    board_appearance,
                                    top_bot_appearance,
                                    false, /* Don't invert */
                                    PCB->Name);

//  destroy_appearance (board_appearance); /* XXX: HANGING ON TO THIS FOR NOW */
//  destroy_appearance (top_bot_appearance); /* XXX: HANGING ON TO THIS FOR NOW */

  poly_Free (&board_outline);

  return objects;
#endif
}

struct mask_info {
  POLYAREA *poly;
  int side;
};

static POLYAREA *
TextToPoly (TextType *Text, Coord min_line_width)
{
  POLYAREA *np, *res;
  Coord x = 0;
  unsigned char *string = (unsigned char *) Text->TextString;
  Cardinal n;
  FontType *font = &PCB->Font;

  res = NULL;

  while (string && *string)
    {
      /* draw lines if symbol is valid and data is present */
      if (*string <= MAX_FONTPOSITION && font->Symbol[*string].Valid)
        {
          LineType *line = font->Symbol[*string].Line;
          LineType newline;

          for (n = font->Symbol[*string].LineN; n; n--, line++)
            {
              /* create one line, scale, move, rotate and swap it */
              newline = *line;
              newline.Point1.X = SCALE_TEXT (newline.Point1.X + x, Text->Scale);
              newline.Point1.Y = SCALE_TEXT (newline.Point1.Y, Text->Scale);
              newline.Point2.X = SCALE_TEXT (newline.Point2.X + x, Text->Scale);
              newline.Point2.Y = SCALE_TEXT (newline.Point2.Y, Text->Scale);
              newline.Thickness = SCALE_TEXT (newline.Thickness, Text->Scale / 2);
              if (newline.Thickness < min_line_width)
                newline.Thickness = min_line_width;

              RotateLineLowLevel (&newline, 0, 0, Text->Direction);

              /* the labels of SMD objects on the bottom
               * side haven't been swapped yet, only their offset
               */
              if (TEST_FLAG (ONSOLDERFLAG, Text))
                {
                  newline.Point1.X = SWAP_SIGN_X (newline.Point1.X);
                  newline.Point1.Y = SWAP_SIGN_Y (newline.Point1.Y);
                  newline.Point2.X = SWAP_SIGN_X (newline.Point2.X);
                  newline.Point2.Y = SWAP_SIGN_Y (newline.Point2.Y);
                }
              /* add offset and draw line */
              newline.Point1.X += Text->X;
              newline.Point1.Y += Text->Y;
              newline.Point2.X += Text->X;
              newline.Point2.Y += Text->Y;

              np = LinePoly (&newline, newline.Thickness, NULL);
              poly_Boolean_free (res, np, &res, PBO_UNITE);
            }

          /* move on to next cursor position */
          x += (font->Symbol[*string].Width + font->Symbol[*string].Delta);
        }
      else
        {
          /* the default symbol is a filled box */
          BoxType defaultsymbol = PCB->Font.DefaultSymbol;
          Coord size = (defaultsymbol.X2 - defaultsymbol.X1) * 6 / 5;

          defaultsymbol.X1 = SCALE_TEXT (defaultsymbol.X1 + x, Text->Scale);
          defaultsymbol.Y1 = SCALE_TEXT (defaultsymbol.Y1, Text->Scale);
          defaultsymbol.X2 = SCALE_TEXT (defaultsymbol.X2 + x, Text->Scale);
          defaultsymbol.Y2 = SCALE_TEXT (defaultsymbol.Y2, Text->Scale);

          RotateBoxLowLevel (&defaultsymbol, 0, 0, Text->Direction);

          /* add offset and draw box */
          defaultsymbol.X1 += Text->X;
          defaultsymbol.Y1 += Text->Y;
          defaultsymbol.X2 += Text->X;
          defaultsymbol.Y2 += Text->Y;

          np = RectPoly (defaultsymbol.X1, defaultsymbol.X2,
                         defaultsymbol.Y1, defaultsymbol.Y2);
          poly_Boolean_free (res, np, &res, PBO_UNITE);

          /* move on to next cursor position */
          x += size;
        }
      string++;
    }

  return res;
}

static int
line_mask_callback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  struct mask_info *info = (struct mask_info *) cl;
  POLYAREA *np, *res;

  if (!(np = LinePoly (line, line->Thickness, NULL)))
    return 0;

  poly_Boolean_free (info->poly, np, &res, PBO_SUB);
  info->poly = res;

  return 1;
}

static int
arc_mask_callback (const BoxType * b, void *cl)
{
  ArcType *arc = (ArcType *) b;
  struct mask_info *info = (struct mask_info *) cl;
  POLYAREA *np, *res;

  if (!(np = ArcPoly (arc, arc->Thickness, NULL)))
    return 0;

  poly_Boolean_free (info->poly, np, &res, PBO_SUB);
  info->poly = res;

  return 1;
}


static int
text_mask_callback (const BoxType * b, void *cl)
{
  TextType *text = (TextType *) b;
  struct mask_info *info = (struct mask_info *) cl;
  POLYAREA *np, *res;

  if (!(np = TextToPoly (text, PCB->minWid))) /* XXX: Min mask cutout width should be separate */
    return 0;

  poly_Boolean_free (info->poly, np, &res, PBO_SUB);
  info->poly = res;

  return 1;
}

static int
polygon_mask_callback (const BoxType * b, void *cl)
{
  PolygonType *poly = (PolygonType *) b;
  struct mask_info *info = (struct mask_info *) cl;
  POLYAREA *np, *res;

  if (!(np = PolygonToPoly (poly)))
    return 0;

  poly_Boolean_free (info->poly, np, &res, PBO_SUB);
  info->poly = res;

  return 1;
}

static int
pad_mask_callback (const BoxType * b, void *cl)
{
  PadType *pad = (PadType *) b;
  struct mask_info *info = (struct mask_info *) cl;
  POLYAREA *np, *res;

  if (pad->Mask == 0)
    return 0;

  if (XOR (TEST_FLAG (ONSOLDERFLAG, pad), (info->side == BOTTOM_SIDE)))
    return 0;

  if (!(np = PadPoly (pad, pad->Mask)))
    return 0;

  poly_Boolean_free (info->poly, np, &res, PBO_SUB);
  info->poly = res;

  return 1;
}

static int
pv_mask_callback (const BoxType * b, void *cl)
{
  PinType *pv = (PinType *)b;
  struct mask_info *info = cl;
  POLYAREA *np, *res;

  if (!(np = PinPoly (pv, pv->Mask)))
    return 0;

  poly_Boolean_free (info->poly, np, &res, PBO_SUB);
  info->poly = res;

  return 1;
}


GList *
object3d_from_soldermask_within_area (POLYAREA *area, int side)
{
  appearance *mask_appearance;
  GList *objects;
  struct mask_info info;
  BoxType bounds;
  LayerType *layer;
  POLYAREA *pa;

//  return NULL;

  poly_Copy0 (&info.poly, area);
  info.side = side;

  bounds.X1 = area->contours->xmin;
  bounds.X2 = area->contours->xmax;
  bounds.Y1 = area->contours->ymin;
  bounds.Y2 = area->contours->ymax;

  layer = LAYER_PTR ((side == TOP_SIDE) ? top_soldermask_layer : bottom_soldermask_layer);

  r_search (layer->line_tree, &bounds, NULL, line_mask_callback, &info);
  r_search (layer->arc_tree,  &bounds, NULL, arc_mask_callback, &info);
  r_search (layer->text_tree, &bounds, NULL, text_mask_callback, &info);
  r_search (layer->polygon_tree, &bounds, NULL, polygon_mask_callback, &info);

  r_search (PCB->Data->pad_tree, &bounds, NULL, pad_mask_callback, &info);
  r_search (PCB->Data->pin_tree, &bounds, NULL, pv_mask_callback, &info);
  r_search (PCB->Data->via_tree, &bounds, NULL, pv_mask_callback, &info);

  if (info.poly == NULL)
    return NULL;

  /* Erase outer contour naming from the polygon - we don't want all the
   * hole names that get combined into the outer name! */
  pa = info.poly;
  do
    {
      free (pa->contours->name);
      pa->contours->name = NULL;
    }
  while ((pa = pa->f) != info.poly);

  mask_appearance = make_appearance ();
  appearance_set_color (mask_appearance, 0.2, 0.8, 0.2);

  poly_Simplify (info.poly);

  objects = object3d_from_contours (info.poly,
#ifdef REVERSED_PCB_CONTOURS
                                    (side == TOP_SIDE) ? 0                   + HACK_COPPER_THICKNESS : -HACK_BOARD_THICKNESS - HACK_COPPER_THICKNESS - HACK_MASK_THICKNESS, /* Bottom */
                                    (side == TOP_SIDE) ? HACK_MASK_THICKNESS + HACK_COPPER_THICKNESS : -HACK_BOARD_THICKNESS - HACK_COPPER_THICKNESS,                       /* Top */
#else
                                    (side == TOP_SIDE) ? -HACK_BOARD_THICKNESS / 2 - HACK_COPPER_THICKNESS                       : HACK_BOARD_THICKNESS / 2 + HACK_COPPER_THICKNESS + HACK_MASK_THICKNESS, /* Bottom */
                                    (side == TOP_SIDE) ? -HACK_BOARD_THICKNESS / 2 - HACK_COPPER_THICKNESS - HACK_MASK_THICKNESS : HACK_BOARD_THICKNESS / 2 + HACK_COPPER_THICKNESS, /* Top */
#endif
                                    mask_appearance,
                                    NULL,  /* top_bot_appearance */
                                    false, /* Don't invert */
                                    (side == TOP_SIDE) ? "Top soldermask" : "Bottom soldermask"); /* Name */

//  destroy_appearance (mask_appearance); /* XXX: HANGING ON TO THIS FOR NOW */

  poly_Free (&info.poly);

  return objects;
}

static Coord
compute_depth (int group)
{
  int top_group;
  int bottom_group;
  int min_copper_group;
  int max_copper_group;
  int num_copper_groups;
  int depth;

  top_group = GetLayerGroupNumberBySide (TOP_SIDE);
  bottom_group = GetLayerGroupNumberBySide (BOTTOM_SIDE);

  min_copper_group = MIN (bottom_group, top_group);
  max_copper_group = MAX (bottom_group, top_group);
  num_copper_groups = max_copper_group - min_copper_group;// + 1;

  if (group >= 0 && group < max_group)
    {
      if (group >= min_copper_group && group <= max_copper_group)
        {
          /* XXX: IS THIS INCORRECT FOR REVERSED GROUP ORDERINGS? */
          depth = -(group - min_copper_group) * (HACK_BOARD_THICKNESS + HACK_COPPER_THICKNESS) / num_copper_groups;
        }
      else
        {
          depth = 0;
        }
#if 1
    }
  else if (SL_TYPE (group) == SL_MASK)
    {
      if (SL_SIDE (group) == SL_TOP_SIDE)
        {
          depth = HACK_COPPER_THICKNESS;
        }
      else
        {
          depth = -HACK_BOARD_THICKNESS - HACK_BOARD_THICKNESS - HACK_MASK_THICKNESS;
        }
    }
  else if (SL_TYPE (group) == SL_SILK)
    {
      if (SL_SIDE (group) == SL_TOP_SIDE)
        {
          depth = HACK_COPPER_THICKNESS + HACK_SILK_THICKNESS;
        }
      else
        {
          depth = -HACK_BOARD_THICKNESS - HACK_COPPER_THICKNESS - HACK_MASK_THICKNESS - HACK_SILK_THICKNESS;
        }
    }
  else if (SL_TYPE (group) == SL_INVISIBLE)
    {
      /* Same as silk, but for the back-side layer */
      if (Settings.ShowBottomSide)
        {
          depth = HACK_COPPER_THICKNESS + HACK_SILK_THICKNESS;
        }
      else
        {
          depth = -HACK_BOARD_THICKNESS - HACK_COPPER_THICKNESS - HACK_MASK_THICKNESS - HACK_SILK_THICKNESS;
        }
#endif
    }
  else
    {
      /* DEFAULT CASE */
      printf ("Unknown layer group to set depth for: %i\n", group);
      depth = 0.0;
    }

  return depth;
}

struct copper_info {
  POLYAREA *poly;
  int from_layer_group;
  int to_layer_group;
  int current_group;
  int side;
};


static int
pin_drill_callback (const BoxType * b, void *cl)
{
  PinType *pin = (PinType *)b;
  struct copper_info *info = cl;
  POLYAREA *np, *res;

  if (TEST_FLAG (HOLEFLAG, pin))
    return 0;

  if (!(np = CirclePoly (pin->X, pin->Y, (pin->DrillingHole + 1) / 2, NULL)))
    return 0;

  poly_Boolean_free (info->poly, np, &res, PBO_UNITE);
  info->poly = res;

  return 1;
}

static int
via_drill_callback (const BoxType * b, void *cl)
{
  PinType *via = (PinType *)b;
  struct copper_info *info = cl;
  POLYAREA *np, *res;

  if (TEST_FLAG (HOLEFLAG, via))
    return 0;

  if (info->from_layer_group == -1 && info->to_layer_group == -1)
    {
      if (!VIA_IS_BURIED (via))
        goto via_ok;
    }
  else
    {
      if (VIA_IS_BURIED (via))
        {
          if (info->from_layer_group == GetLayerGroupNumberByNumber (via->BuriedFrom) &&
              info->to_layer_group   == GetLayerGroupNumberByNumber (via->BuriedTo))
            goto via_ok;
        }
    }

  return 1;

via_ok:

  if (!(np = CirclePoly (via->X, via->Y, (via->DrillingHole + 1) / 2, NULL)))
    return 0;

  poly_Boolean_free (info->poly, np, &res, PBO_UNITE);
  info->poly = res;

  return 1;
}

static int
pin_barrel_callback (const BoxType * b, void *cl)
{
  PinType *pin = (PinType *)b;
  struct copper_info *info = cl;
  POLYAREA *np, *res;

  if (TEST_FLAG (HOLEFLAG, pin))
    return 0;

  if (!(np = CirclePoly (pin->X, pin->Y, (pin->DrillingHole + HACK_PLATED_BARREL_THICKNESS + 1) / 2, NULL)))
    return 0;

  poly_Boolean_free (info->poly, np, &res, PBO_UNITE);
  info->poly = res;

  return 1;
}

static int
via_barrel_callback (const BoxType * b, void *cl)
{
  PinType *via = (PinType *)b;
  struct copper_info *info = cl;
  POLYAREA *np, *res;

  if (TEST_FLAG (HOLEFLAG, via))
    return 0;

  if (!ViaIsOnLayerGroup (via, info->from_layer_group) ||
      !ViaIsOnLayerGroup (via, info->to_layer_group))
    return 0;

  if (!(np = CirclePoly (via->X, via->Y, (via->DrillingHole + HACK_PLATED_BARREL_THICKNESS + 1) / 2, NULL)))
    return 0;

  poly_Boolean_free (info->poly, np, &res, PBO_UNITE);
  info->poly = res;

  return 1;
}


static int
line_copper_callback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  struct copper_info *info = (struct copper_info *) cl;
  POLYAREA *np, *res;

  if (!(np = LinePoly (line, line->Thickness, NULL)))
    return 0;

  poly_Boolean_free (info->poly, np, &res, PBO_UNITE);
  info->poly = res;

  return 1;
}

static int
arc_copper_callback (const BoxType * b, void *cl)
{
  ArcType *arc = (ArcType *) b;
  struct copper_info *info = (struct copper_info *) cl;
  POLYAREA *np, *res;

  if (!(np = ArcPoly (arc, arc->Thickness, NULL)))
    return 0;

  poly_Boolean_free (info->poly, np, &res, PBO_UNITE);
  info->poly = res;

  return 1;
}


static int
text_copper_callback (const BoxType * b, void *cl)
{
  TextType *text = (TextType *) b;
  struct copper_info *info = (struct copper_info *) cl;
  POLYAREA *np, *res;

  if (!(np = TextToPoly (text, PCB->minWid)))
    return 0;

  poly_Boolean_free (info->poly, np, &res, PBO_UNITE);
  info->poly = res;

  return 1;
}

static int
polygon_copper_callback (const BoxType * b, void *cl)
{
  PolygonType *poly = (PolygonType *) b;
  struct copper_info *info = (struct copper_info *) cl;
  POLYAREA *np, *res;
  POLYAREA *pa;

  if (poly->Clipped == NULL)
    {
      fprintf (stderr, "poly_copper_callback(): polygon->Clipped == NULL\n");
      return 0;
    }

  if (TEST_FLAG (FULLPOLYFLAG, poly))
    poly_M_Copy0 (&np, poly->Clipped); /* Copy the whole polygon */
  else
    poly_Copy0 (&np, poly->Clipped); /* Just copy the first polygon piece */

  if (np == NULL)
    return 0;

  /* Erase outer contour naming from the polygon - we don't want all the
   * hole names that get combined into the outer name! */
  pa = np;
  do
    {
      free (pa->contours->name);
      pa->contours->name = NULL;
    }
  while ((pa = pa->f) != np);

  poly_Boolean_free (info->poly, np, &res, PBO_UNITE);
  info->poly = res;

  return 1;
}

static
char *netname_from_connection_name (int type, ElementType *ele, void *ptr2)
{
  char *netname = NULL;
  char *connection_name;

  if (ele == NULL || NAMEONPCB_NAME (ele) == NULL)
    return NULL;

  connection_name = ConnectionName (type, ele, ptr2);

  /* ConnectionName uses a static buffer - URGH!! */
  /*
   * if (connection_name == NULL)
   *   return NULL;
   */

  /* Find netlist entry */
  MENU_LOOP (&PCB->NetlistLib);
  {
    if (!menu->Name)
    continue;

    ENTRY_LOOP (menu);
    {
      if (!entry->ListEntry)
        continue;

      if (strcmp (entry->ListEntry, connection_name) == 0) {
        netname = g_strdup (menu->Name);
        /* For some reason, the netname has spaces in front of it, strip them */
        g_strstrip (netname);
        break;
      }
    }
    END_LOOP;

    if (netname != NULL)
      break;
  }
  END_LOOP;

  /* ConnectionName uses a static buffer - URGH!! */
  /*
   * free (connection_name);
   */

  return netname;
}

static
char *netname_from_pin (PinType *pin)
{
  return netname_from_connection_name (PIN_TYPE, pin->Element, pin);
}

static
char *netname_from_pad (PadType *pad)
{
  return netname_from_connection_name (PAD_TYPE, pad->Element, pad);
}

static int
pad_copper_callback (const BoxType * b, void *cl)
{
  PadType *pad = (PadType *) b;
  struct copper_info *info = (struct copper_info *) cl;
  POLYAREA *np, *res;
  char *netname;

  if (XOR (TEST_FLAG (ONSOLDERFLAG, pad), (info->side == BOTTOM_SIDE)))
    return 0;

  if (!(np = PadPoly (pad, pad->Thickness)))
    return 0;

  netname = netname_from_pad (pad);
  if (netname != NULL)
    {
      np->contours->name = strdup (netname);
      g_free (netname);
    }

  poly_Boolean_free (info->poly, np, &res, PBO_UNITE);
  info->poly = res;

  return 1;
}

static int
pin_copper_callback (const BoxType * b, void *cl)
{
  PinType *pin = (PinType *)b;
  struct copper_info *info = cl;
  POLYAREA *np, *res;
  char *netname;

  if (TEST_FLAG (HOLEFLAG, pin))
    return 0;

  if (!(np = PinPoly (pin, PIN_SIZE (pin))))
    return 0;

  netname = netname_from_pin (pin);
  if (netname != NULL)
    {
      np->contours->name = strdup (netname);
      g_free (netname);
    }

  poly_Boolean_free (info->poly, np, &res, PBO_UNITE);
  info->poly = res;

  return 1;
}

static int
via_copper_callback (const BoxType * b, void *cl)
{
  PinType *via = (PinType *)b;
  struct copper_info *info = cl;
  POLYAREA *np, *res;

  if (TEST_FLAG (HOLEFLAG, via))
    return 0;

  if (!ViaIsOnLayerGroup (via, info->current_group))
    return 0;

  if (!(np = PinPoly (via, PIN_SIZE (via))))
    return 0;

  poly_Boolean_free (info->poly, np, &res, PBO_UNITE);
  info->poly = res;

  return 1;
}


static int
pin_unplated_hole_sub_copper_callback (const BoxType * b, void *cl)
{
  PinType *pin = (PinType *)b;
  struct copper_info *info = cl;
  POLYAREA *np, *res;

  if (!TEST_FLAG (HOLEFLAG, pin))
    return 0;

  if (!(np = CirclePoly (pin->X, pin->Y, (pin->DrillingHole + 1) / 2, NULL)))
    return 0;

  poly_Boolean_free (info->poly, np, &res, PBO_SUB);
  info->poly = res;

  return 1;
}

static int
via_unplated_hole_sub_copper_callback (const BoxType * b, void *cl)
{
  PinType *via = (PinType *)b;
  struct copper_info *info = cl;
  POLYAREA *np, *res;

  if (!TEST_FLAG (HOLEFLAG, via))
    return 0;

  if (!ViaIsOnLayerGroup (via, info->current_group))
    return 0;

  if (!(np = CirclePoly (via->X, via->Y, (via->DrillingHole + 1) / 2, NULL)))
    return 0;

  poly_Boolean_free (info->poly, np, &res, PBO_SUB);
  info->poly = res;

  return 1;
}


static void
steal_object_geometry (object3d *src, object3d *dst)
{
  g_assert (dst != src);

  /* Prepend the src chunks, as src is likely to be a lot smaller then dst once we get going */
  dst->faces    = g_list_concat (src->faces,    dst->faces);
  dst->edges    = g_list_concat (src->edges,    dst->edges);
  dst->vertices = g_list_concat (src->vertices, dst->vertices);
  src->faces = NULL;
  src->edges = NULL;
  src->vertices = NULL;

  /* Join up their link data */
  dst->user_data = g_list_concat (dst->user_data, src->user_data);
  src->user_data = NULL;
}

static void
update_object_pointers (/*POLYAREA **group_m_poly, int touched_group, */object3d *old_object, object3d *new_object)
{
  GList *iter;

  for (iter = old_object->user_data; iter != NULL; iter = g_list_next (iter))
    {
      polygon_3d_link *link = iter->data;

      g_warn_if_fail (link->object == old_object);
      link->object = new_object;
    }

#if 0
  int group;
  int top_group;
  int bottom_group;
  int min_copper_group;
  int max_copper_group;
  POLYAREA *pa;

  top_group =    GetLayerGroupNumberBySide (TOP_SIDE);
  bottom_group = GetLayerGroupNumberBySide (BOTTOM_SIDE);

  min_copper_group = MIN (bottom_group, top_group);
  max_copper_group = MAX (bottom_group, top_group);

  for (group = min_copper_group; group <= touched_group + 1 /*max_copper_group*/; group++)
    {

      /* Skip this group if it isn't one we've touched, or will be working on, and isn't
       * the top / bottom
       * groups (which will be required to be correct for inserting drill holes
       */
      if (group != min_copper_group &&
          group != max_copper_group &&
          group != touched_group &&
          group != touched_group + 1)
        continue;

      pa = group_m_poly[group];
      do
        {
          polygon_3d_link *link = pa->user_data;

          if (link->object == old_object)
            link->object = new_object;

        }
      while ((pa = pa->f) != group_m_poly[group]);
    }
#endif
}

/* Returns a string allocated with g_malloc family of functions */
static char *
merge_contour_name (char *old, const char *new)
{
  char *combined;

  if (old == NULL)
    return g_strdup (new);

  if (new == NULL)
    return old;

  if (strcmp (old, new) == 0)
    return old;

  combined = g_strdup_printf ("%s_%s", old, new);
  g_free (old);

  return combined;
}

#ifdef HASH_OBJECTS
static void
copy_glist_into_hash (GHashTable *hash, GList *items)
{
  GList *iter;
  for (iter = items; iter != NULL; iter = g_list_next (iter))
    g_hash_table_insert (hash, iter->data, iter);
}
#endif

GList *
object3d_from_copper_layers_within_area (POLYAREA *area)
{
  appearance *copper_appearance;
  GList *group_objects;
  struct copper_info info;
  BoxType bounds;

  int group;
  int top_group;
  int bottom_group;
  int min_copper_group;
  int max_copper_group;
  int group_step = 1;

  POLYAREA **group_m_polyarea;
  POLYAREA *barrel_m_polyarea;
  POLYAREA *drill_m_polyarea;
#ifdef SUM_PINS_VIAS_ONCE
  POLYAREA *pinvia_m_polyarea;
  POLYAREA *temp;
#endif
#ifdef HASH_OBJECTS
  GHashTable *object_hash;
#endif
  int g_from;
  int g_to;

//  poly_Copy0 (&info.poly, area);

  copper_appearance = make_appearance ();
//  appearance_set_color (copper_appearance, 0.5, 0.1, 0.1);
  appearance_set_color (copper_appearance, 0.76, 0.49, 0.0);
//  appearance_set_color (copper_appearance, 0.8, 0.8, 0.8);

  bounds.X1 = area->contours->xmin;
  bounds.X2 = area->contours->xmax;
  bounds.Y1 = area->contours->ymin;
  bounds.Y2 = area->contours->ymax;

  top_group =    GetLayerGroupNumberBySide (TOP_SIDE);
  bottom_group = GetLayerGroupNumberBySide (BOTTOM_SIDE);

  min_copper_group = MIN (bottom_group, top_group);
  max_copper_group = MAX (bottom_group, top_group);
//  group_step = max_copper_group - min_copper_group; // OUTER LAYERS ONLY

  group_m_polyarea = calloc (max_copper_group + 1, sizeof (POLYAREA *));

  group_objects = NULL;
#ifdef HASH_OBJECTS
  object_hash = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);
#endif

#ifdef SUM_PINS_VIAS_ONCE
  info.poly = NULL;
  fprintf (stderr, "Accumulating master pin + via polygon\n");
  r_search (PCB->Data->pin_tree, &bounds, NULL, pin_copper_callback, &info);
  r_search (PCB->Data->via_tree, &bounds, NULL, /* Deliberate pin_-> */pin_copper_callback, &info);
  pinvia_m_polyarea = info.poly;
#endif

  for (group = min_copper_group; group <= max_copper_group; group += group_step)
    {
      GList *new_objects;
      Coord depth = compute_depth (group);

      info.poly = NULL;
      info.current_group = group;

      fprintf (stderr, "Computing copper geometry for group %i\n", group);

      GROUP_LOOP (PCB->Data, group);
        {
          fprintf (stderr, "  Accumulating copper from layer %i\n", GetLayerNumber (PCB->Data, layer));

          r_search (layer->line_tree, &bounds, NULL, line_copper_callback, &info);
          r_search (layer->arc_tree,  &bounds, NULL, arc_copper_callback, &info);
          r_search (layer->text_tree, &bounds, NULL, text_copper_callback, &info);
          r_search (layer->polygon_tree, &bounds, NULL, polygon_copper_callback, &info);
        }
      END_LOOP;

      fprintf (stderr, "  Accumulating pin + via pads\n");

#ifdef SUM_PINS_VIAS_ONCE
      poly_M_Copy0 (&temp, pinvia_m_polyarea);
      poly_Boolean_free (info.poly, temp, &info.poly, PBO_UNITE);
#else
      r_search (PCB->Data->pin_tree, &bounds, NULL, pin_copper_callback, &info);
      r_search (PCB->Data->via_tree, &bounds, NULL, via_copper_callback, &info);
#endif

      if (group == top_group ||
          group == bottom_group)
        {
          info.side = (group == top_group) ? TOP_SIDE : BOTTOM_SIDE;
          fprintf (stderr, "  Accumulating SMT pads for side %i\n", info.side);
          r_search (PCB->Data->pad_tree, &bounds, NULL, pad_copper_callback, &info);
        }

      fprintf (stderr, "  Subtracting non-plated holes\n");
      r_search (PCB->Data->pin_tree, &bounds, NULL, pin_unplated_hole_sub_copper_callback, &info);
      r_search (PCB->Data->via_tree, &bounds, NULL, via_unplated_hole_sub_copper_callback, &info);

      /* TODO: Inter-layer features
       *
       * Accumulate a circular polygon for each plated hole we may cut, ensuring
       * the finished polygon contour extends to include the via barrel extents.
       *
       * Subtract non-plated hole contours from the polygons.
       *
       * For each hole, add the via-barrel between layers... removing the Object3D
       * from the list of objects as they become joined with some other. (The final
       * list of objects should match 1:1 with resultant bodies, and contain no
       * duplicates.
       *
       * To accomodate overlapping drill holes, accumulate all via-barrels into a
       * polygon, and subtract that from the positive copper polygon. As we already
       * added via-barrels to each copper layer, each barrel extrusion contour
       * should match only one body of copper on a given layer.
       *
       * Remove the drilled hole down each via by extruding the additional faces.
       *
       * To accomodate overlapping drill holes, accumulate all drills into a polygon,
       * and subtract that from the positive copper polygon. Any subtracted contour
       * should at this point match to one body of copper on a given layer.
       *
       */

#if 0
      fprintf (stderr, "Subtracting pin + via drills\n");
      r_search (PCB->Data->pin_tree, &bounds, NULL, pv_drill_callback, &info);
      r_search (PCB->Data->via_tree, &bounds, NULL, pv_drill_callback, &info);
#endif

      if (info.poly == NULL)
        {
          fprintf (stderr, "Skipping layer group %i, info.poly was NULL\n", group);
          continue;
        }

      if (0)
        {
          POLYAREA *pa = info.poly;
          do
            {
              printf ("Polygon piece with outer contour named %s\n",
                      pa->contours->name == NULL ? "NULL" : pa->contours->name);
            }
          while ((pa = pa->f) != info.poly);
        }

      group_m_polyarea[group] = info.poly;
      poly_Simplify (group_m_polyarea[group]);

      new_objects =
        object3d_from_contours (group_m_polyarea[group],
#ifdef REVERSED_PCB_CONTOURS
                                depth,                         /* Bottom */
                                depth + HACK_COPPER_THICKNESS, /* Top */
#else
                                -depth - HACK_BOARD_THICKNESS / 2,                         /* Bottom */
                                -depth - HACK_BOARD_THICKNESS / 2 - HACK_COPPER_THICKNESS, /* Top */
#endif
                                copper_appearance,
                                NULL,  /* top_bot_appearance */
                                false, /* Don't invert */
                                "Net"); /* Name */
#ifdef HASH_OBJECTS
      copy_glist_into_hash (object_hash, new_objects);
#endif
      group_objects = g_list_concat (group_objects, new_objects);
    }

#ifdef SUM_PINS_VIAS_ONCE
  poly_Free (&pinvia_m_polyarea);
#endif

  // Extrude barrel?
  // Grab top-face of barrel. Delete the face, stealing its contour.. find which top-side copper Object3D to paste in into, link it up.
  // Grab bottom-face of barrel. Delete the face, stealing its contour.. find which next-side copper Object3D to paste it into, link it up. (Might already be the same object as in step above.. how to find it?)
  // Repeat for each inter-layer barrel segment
  // Repeat for each contour in the accumulated barrel M_POLYAERA

  for (group = min_copper_group; group < max_copper_group; group += group_step)
    {
      Coord top_depth;
      Coord bottom_depth;
      POLYAREA *pa;
      GList *barrel_objects;

      info.poly = NULL;
      info.from_layer_group = group;
      info.to_layer_group = group + group_step;

      fprintf (stderr, "Accumulating pin + via barrel outers\n");
      r_search (PCB->Data->pin_tree, &bounds, NULL, pin_barrel_callback, &info);
      r_search (PCB->Data->via_tree, &bounds, NULL, via_barrel_callback, &info);

      barrel_m_polyarea = info.poly;
      info.poly = NULL;

      poly_Simplify (barrel_m_polyarea);

      /* HACK - LET US EMIT BLANK BOARDS.. SHOULD CHECK BEFORE WE START TO LOOP? */
      if (barrel_m_polyarea == NULL)
        break;

      /* Extrude barrel from group to group + 1 */
      fprintf (stderr, "Extruding barrels from layer group %i to %i\n", group, group + group_step);

      g_assert (group_m_polyarea[group] != NULL);
      g_assert (group_m_polyarea[group + group_step] != NULL);

      /* Depth is the bottom? of the layer? */
      top_depth = compute_depth (group);
      bottom_depth = compute_depth (group + group_step);

      barrel_objects = object3d_from_contours (barrel_m_polyarea,
#ifdef REVERSED_PCB_CONTOURS
                                               bottom_depth + HACK_COPPER_THICKNESS, /* Bottom */
                                               top_depth,                            /* Top */
#else
                                               -bottom_depth - HACK_BOARD_THICKNESS / 2 - HACK_COPPER_THICKNESS, /* Bottom */
                                               -top_depth    - HACK_BOARD_THICKNESS / 2,                         /* Top */
#endif
                                               copper_appearance,
                                               NULL,  /* top_bot_appearance */
                                               false, /* Don't invert */
                                               NULL); /* Name */

/* Connect the via barrels in this block of code */
#if 1
      /* Loop over all barrel outline pieces */
      pa = barrel_m_polyarea;
      do
        {
          /* For each barrel, we want to find the Polyarea it hits on group, and group+1.. this tracks to the objects and faces we must manipulate */

          polygon_3d_link *top_link    ;
          polygon_3d_link *barrel_link ;
          polygon_3d_link *bottom_link ;

          object3d *top_group_object    ;
          object3d *barrel_object       ;
          object3d *bottom_group_object ;

          face3d *top_group_face     ;
          face3d *barrel_top_face    ;
          face3d *barrel_bottom_face ;
          face3d *bottom_group_face  ;

          edge_ref barrel_top_face_first_edge    ;
          edge_ref barrel_bottom_face_first_edge ;
          edge_ref e;

          POLYAREA *top_pa    = cntr_in_M_POLYAREA (pa->contours, group_m_polyarea[group    ] , false);
          POLYAREA *bottom_pa = cntr_in_M_POLYAREA (pa->contours, group_m_polyarea[group + group_step] , false);

          /* These conditions should not occur, and can likely only happen because some other
           * bug in the polygon processing code has created an inconsistent m_polyarea somewhere.
           * We need to check for them though, since sadly - bad polygon boolean operations are
           * still far too common.
           */
          g_warn_if_fail (top_pa != NULL);
          g_warn_if_fail (bottom_pa != NULL);
          if (top_pa == NULL)
              continue;

          if (bottom_pa == NULL)
            continue;

          top_link    = top_pa->user_data;
          barrel_link = pa->user_data;
          bottom_link = bottom_pa->user_data;

          top_group_object    = top_link->object;
          barrel_object       = barrel_link->object;
          bottom_group_object = bottom_link->object;

          top_group_face     = top_link->bottom_face;
          barrel_top_face    = barrel_link->top_face;
          barrel_bottom_face = barrel_link->bottom_face;
          bottom_group_face  = bottom_link->top_face;

          barrel_top_face_first_edge = ((contour3d *)barrel_top_face->contours->data)->first_edge;
          barrel_bottom_face_first_edge = ((contour3d *)barrel_bottom_face->contours->data)->first_edge;

          /* Do some magic to join the objects */

          g_assert (g_list_length (barrel_top_face->contours) == 1);
          g_assert (g_list_length (barrel_bottom_face->contours) == 1);

          face3d_add_contour (top_group_face,    make_contour3d (barrel_top_face_first_edge));
          face3d_add_contour (bottom_group_face, make_contour3d (barrel_bottom_face_first_edge));

          /* Need to walk around the top / bottom edge contours, and re-connect with the linked up copper groups */

          e = barrel_top_face_first_edge;
          do
            {
              /* Check and reassign the edge */
              g_assert (LDATA (e) == barrel_top_face);
              LDATA (e) = top_group_face;
            }
          while ((e = LNEXT (e)) != barrel_top_face_first_edge);

          e = barrel_bottom_face_first_edge;
          do
            {
              /* Check and reassign the edge */
              g_assert (LDATA (e) == barrel_bottom_face);
              LDATA (e) = bottom_group_face;
            }
          while ((e = LNEXT (e)) != barrel_bottom_face_first_edge);

          /* XXX: What about destroying the barrel top / bottom face appearance (if any?) */

          barrel_object->faces = g_list_remove (barrel_object->faces, barrel_top_face);
          barrel_object->faces = g_list_remove (barrel_object->faces, barrel_bottom_face);
          destroy_face3d (barrel_top_face);    /* This leaves the edges, vertices etc.. it only deletes the face contour list */
          destroy_face3d (barrel_bottom_face); /* This leaves the edges, vertices etc.. it only deletes the face contour list */
          /* No vertices should be deleted */
          /* All edges must end up in the top object, so we leave them */

          /* Steal the data from the barrel object */
          update_object_pointers (barrel_object, top_group_object);
          steal_object_geometry (barrel_object, top_group_object);
          destroy_object3d (barrel_object);

          if (top_group_object != bottom_group_object)
            { /* Top object and bottom object were previously distinct */
              GList *link;

              /* Update any remaining link pointers to the previous bottom object we are about to delete */
              update_object_pointers (/*group_m_polyarea, group, */bottom_group_object, top_group_object);

              /* Remove the old bottom object from the list of output objects */
#ifdef HASH_OBJECTS
              link = g_hash_table_lookup (object_hash, bottom_group_object);
              group_objects = g_list_delete_link (group_objects, link);
              g_hash_table_remove (object_hash, bottom_group_object);
#else
              group_objects = g_list_remove (group_objects, bottom_group_object);
#endif

              /* Steal the data from the old bottom object */
              steal_object_geometry (bottom_group_object, top_group_object);

//              printf ("Merging object with name %s and %s\n", top_group_object->name, bottom_group_object->name);
              top_group_object->name = merge_contour_name (top_group_object->name, bottom_group_object->name);

              /* Delete the old bottom object */
              destroy_object3d (bottom_group_object);
            }

//          g_slice_free (polygon_3d_link, pa->user_data);
        }
      while (pa = pa->f, pa != barrel_m_polyarea);

      poly_Free (&barrel_m_polyarea);
      g_list_free (barrel_objects);
#else
      group_objects = g_list_concat (group_objects, barrel_objects);
#endif
    }


  /* Now need to punch drill-holes through the inter-layers..
   * We construct a polygon of drill-holes, so any overlapping are taken into account
   */


  /* XXX: This does not work for overlapping BB vias.. normally one wouldn't create
   *      these, but if they are created - bad 3D objects will likely result
   */


  /* Test each from/to layer combination in turn */

  for (g_from = min_copper_group; g_from < max_copper_group; g_from += group_step)
    for (g_to = g_from + 1;       g_to  <= max_copper_group; g_to   += group_step)
      {
        int plated;
        int unplated;
        bool is_through_grouping = (g_from == min_copper_group && g_to == max_copper_group);

        info.poly = NULL;

        /* BB Vias */
        CountHolesEx (&plated, &unplated, NULL, g_from, g_to);
        if (plated > 0 || is_through_grouping)
            printf ("Accumulating via drills from group %i to group %i\n", g_from, g_to);

        if (plated > 0)
          {
            printf ("  Subtracting BB vias\n");

            info.from_layer_group = g_from;
            info.to_layer_group = g_to;
            r_search (PCB->Data->pin_tree, &bounds, NULL, pin_drill_callback, &info);
            r_search (PCB->Data->via_tree, &bounds, NULL, via_drill_callback, &info);
          }

        /* Through vias */
        if (is_through_grouping)
          {
            CountHoles (&plated, &unplated, NULL);
            if (plated > 0)
              {
                printf ("  Subtracting through vias\n");

                info.from_layer_group = -1;
                info.to_layer_group = -1;
                r_search (PCB->Data->pin_tree, &bounds, NULL, pin_drill_callback, &info);
                r_search (PCB->Data->via_tree, &bounds, NULL, via_drill_callback, &info);
              }
          }

        drill_m_polyarea = info.poly;
        info.poly = NULL;
        poly_Simplify (drill_m_polyarea);

        if (drill_m_polyarea != NULL) /* Drill holes */
          {
            Coord top_depth;
            Coord bottom_depth;
            POLYAREA *pa;
            GList *drill_objects;

            /* Extrude drill hole */
            fprintf (stderr, "  Extruding drill holes\n");

            /* Depth is the bottom? of the layer? */
            top_depth = compute_depth (g_from);
            bottom_depth = compute_depth (g_to);

            drill_objects = object3d_from_contours (drill_m_polyarea,
#ifdef REVERSED_PCB_CONTOURS
                                                     bottom_depth,                      /* Bottom */
                                                     top_depth + HACK_COPPER_THICKNESS, /* Top */
#else
                                                    -bottom_depth - HACK_BOARD_THICKNESS / 2,                         /* Bottom */
                                                    -top_depth    - HACK_BOARD_THICKNESS / 2 - HACK_COPPER_THICKNESS, /* Top */
#endif
                                                    copper_appearance,
                                                    NULL,  /* top_bot_appearance */
                                                    true,  /* Invert */
                                                    NULL); /* Name */

      /* Connect the via drill holes in this block of code */
#if 1
            /* Loop over all barrel outline pieces */
            pa = drill_m_polyarea;
            do
              {
#if 1
                /* Quick spot check for drills not landing on copper */
                if (cntr_in_M_POLYAREA (pa->contours, group_m_polyarea[g_from], false) == NULL ||
                    cntr_in_M_POLYAREA (pa->contours, group_m_polyarea[g_to], false) == NULL)
                  continue;
#endif

                /* For each drill hole, we want to find the Polyarea it hits on min_copper_group, and max_copper_group.. this tracks to the objects and faces we must manipulate */
                polygon_3d_link *top_link    = cntr_in_M_POLYAREA (pa->contours, group_m_polyarea[g_from], false)->user_data;
                polygon_3d_link *drill_link  = pa->user_data;
                polygon_3d_link *bottom_link = cntr_in_M_POLYAREA (pa->contours, group_m_polyarea[g_to], false)->user_data;

                object3d *top_group_object    = top_link->object;
                object3d *drill_object        = drill_link->object;
                object3d *bottom_group_object = bottom_link->object;

                face3d *top_group_face    = top_link->top_face;
                face3d *drill_top_face    = drill_link->top_face;
                face3d *drill_bottom_face = drill_link->bottom_face;
                face3d *bottom_group_face = bottom_link->bottom_face;

                edge_ref drill_top_face_first_edge = ((contour3d *)drill_top_face->contours->data)->first_edge;
                edge_ref drill_bottom_face_first_edge = ((contour3d *)drill_bottom_face->contours->data)->first_edge;
                edge_ref e;

                g_assert (top_group_object == bottom_group_object);

                /* Do some magic to join the objects */

                g_assert (g_list_length (drill_top_face->contours) == 1);
                g_assert (g_list_length (drill_bottom_face->contours) == 1);

                face3d_add_contour (top_group_face,    make_contour3d (drill_top_face_first_edge));
                face3d_add_contour (bottom_group_face, make_contour3d (drill_bottom_face_first_edge));

                /* Need to walk around the top / bottom edge contours, and re-connect with the linked up copper groups */

                e = drill_top_face_first_edge;
                do
                  {
                    /* Check and reassign the edge */
                    g_assert (LDATA (e) == drill_top_face);
                    LDATA (e) = top_group_face;
                  }
                while ((e = LNEXT (e)) != drill_top_face_first_edge);

                e = drill_bottom_face_first_edge;
                do
                  {
                    /* Check and reassign the edge */
                    g_assert (LDATA (e) == drill_bottom_face);
                    LDATA (e) = bottom_group_face;
                  }
                while ((e = LNEXT (e)) != drill_bottom_face_first_edge);

                /* XXX: What about destroying the barrel top / bottom face appearance (if any?) */

                drill_object->faces = g_list_remove (drill_object->faces, drill_top_face);
                drill_object->faces = g_list_remove (drill_object->faces, drill_bottom_face);
                destroy_face3d (drill_top_face);    /* This leaves the edges, vertices etc.. it only deletes the face contour list */
                destroy_face3d (drill_bottom_face); /* This leaves the edges, vertices etc.. it only deletes the face contour list */
                /* No vertices should be deleted */
                /* All edges must end up in the top object, so we leave them */

                /* Steal the data from the drill object */
                steal_object_geometry (drill_object, top_group_object);
                destroy_object3d (drill_object);

                g_slice_free (polygon_3d_link, pa->user_data);
              }
            while (pa = pa->f, pa != drill_m_polyarea);

            g_list_free (drill_objects);
#else
            group_objects = g_list_concat (group_objects, drill_objects);
#endif
          }

      }

//  destroy_appearance (copper_appearance); /* XXX: HANGING ON TO THIS FOR NOW */

  /* DEBUG */
//  poly_M_Copy0 (&PCB->Data->outline, info.poly);
//  PCB->Data->outline_valid = true;
//  gui->invalidate_all ();

  /* ASSUME THERE IS A POLYGON WHERE WE KNOW WE PUT ONE... */
//  ((PolygonType *)PCB->Data->Layer[1].Polygon->data)->Clipped = info.poly;
//  gui->invalidate_all ();

//  poly_Free (&info.poly);

  poly_Free (&drill_m_polyarea);

  for (group = min_copper_group; group <= max_copper_group; group += group_step)
    {
      if (group_m_polyarea[group] != NULL)
        {
          g_slice_free (polygon_3d_link, group_m_polyarea[group]->user_data);
          poly_Free (&group_m_polyarea[group]);
        }
    }

#ifdef HASH_OBJECTS
  g_hash_table_destroy (object_hash);
#endif
  return group_objects;
}


struct silk_info {
  POLYAREA *poly;
  int side;
};

static int
line_silk_callback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  struct silk_info *info = (struct silk_info *) cl;
  POLYAREA *np, *res;

  if (!(np = LinePoly (line, line->Thickness, NULL)))
    return 0;

  poly_Boolean_free (info->poly, np, &res, PBO_UNITE);
  info->poly = res;

  return 1;
}

static int
arc_silk_callback (const BoxType * b, void *cl)
{
  ArcType *arc = (ArcType *) b;
  struct silk_info *info = (struct silk_info *) cl;
  POLYAREA *np, *res;

  if (!(np = ArcPoly (arc, arc->Thickness, NULL)))
    return 0;

  poly_Boolean_free (info->poly, np, &res, PBO_UNITE);
  info->poly = res;

  return 1;
}


static int
text_silk_callback (const BoxType * b, void *cl)
{
  TextType *text = (TextType *) b;
  struct silk_info *info = (struct silk_info *) cl;
  POLYAREA *np, *res;

  if (!(np = TextToPoly (text, PCB->minWid))) /* XXX: Min silk cutout width should be separate */
    return 0;

  poly_Boolean_free (info->poly, np, &res, PBO_UNITE);
  info->poly = res;

  return 1;
}

static int
polygon_silk_callback (const BoxType * b, void *cl)
{
  PolygonType *poly = (PolygonType *) b;
  struct silk_info *info = (struct silk_info *) cl;
  POLYAREA *np, *res;

  if (!(np = PolygonToPoly (poly)))
    return 0;

  poly_Boolean_free (info->poly, np, &res, PBO_UNITE);
  info->poly = res;

  return 1;
}

static int
element_silk_callback (const BoxType * b, void *cl)
{
  ElementType *element = (ElementType *) b;
  struct silk_info *info = (struct silk_info *) cl;
  POLYAREA *np, *res;

  if (!ON_SIDE (element, info->side))
    return 0;

  /* draw lines, arcs, text and pins */
  ELEMENTLINE_LOOP (element);
  {
    if (!(np = LinePoly (line, line->Thickness, NULL)))
      return 0;

    poly_Boolean_free (info->poly, np, &res, PBO_UNITE);
    info->poly = res;
  }
  END_LOOP;
  ARC_LOOP (element);
  {
    if (!(np = ArcPoly (arc, arc->Thickness, NULL)))
      return 0;

    poly_Boolean_free (info->poly, np, &res, PBO_UNITE);
    info->poly = res;
  }
  END_LOOP;

  return 1;
}

static int
name_silk_callback (const BoxType * b, void *cl)
{
  TextType *text = (TextType *) b;
  ElementType *element = text->Element;
  struct silk_info *info = (struct silk_info *) cl;
  POLYAREA *np, *res;

  if (!ON_SIDE (element, info->side))
    return 0;

  if (TEST_FLAG (HIDENAMEFLAG, element))
    return 0;

  if (!(np = TextToPoly (text, PCB->minWid)))
    return 0;

  poly_Boolean_free (info->poly, np, &res, PBO_UNITE);
  info->poly = res;

  return 1;
}

GList *
object3d_from_silk_within_area (POLYAREA *area, int side)
{
  appearance *silk_appearance;
  GList *objects;
  struct silk_info info;
  BoxType bounds;
  LayerType *layer;
  POLYAREA *pa;

  info.poly = NULL;
  info.side = side;

  bounds.X1 = area->contours->xmin;
  bounds.X2 = area->contours->xmax;
  bounds.Y1 = area->contours->ymin;
  bounds.Y2 = area->contours->ymax;

  layer = LAYER_PTR ((side == TOP_SIDE) ? top_silk_layer : bottom_silk_layer);

  r_search (layer->line_tree, &bounds, NULL, line_silk_callback, &info);
  r_search (layer->arc_tree,  &bounds, NULL, arc_silk_callback, &info);
  r_search (layer->text_tree, &bounds, NULL, text_silk_callback, &info);
  r_search (layer->polygon_tree, &bounds, NULL, polygon_silk_callback, &info);
  r_search (PCB->Data->element_tree, &bounds, NULL, element_silk_callback, &info);
  r_search (PCB->Data->name_tree[NAME_INDEX (PCB)], &bounds, NULL, name_silk_callback, &info);

#if 0
  ELEMENT_LOOP (&PCB->Data);
  {
    ELEMNETTEXT_LOOP (element);
    {
    }
    END_LOOP;

    ELEMNETLINE_LOOP (element);
    {
    }
    END_LOOP;

    ELEMNETARC_LOOP (element);
    {
    }
    END_LOOP;
  }
  END_LOOP;
#endif

  if (info.poly == NULL)
    return NULL;

  /* Erase outer contour naming from the polygon - we don't want all the
   * hole names that get combined into the outer name! */
  pa = info.poly;
  do
    {
      free (pa->contours->name);
      pa->contours->name = NULL;
    }
  while ((pa = pa->f) != info.poly);

  silk_appearance = make_appearance ();
  appearance_set_color (silk_appearance, 1.0, 1.0, 1.0);

  poly_Simplify (info.poly);

  objects = object3d_from_contours (info.poly,
#ifdef REVERSED_PCB_CONTOURS
                                    (side == TOP_SIDE) ? 0                   + HACK_MASK_THICKNESS + HACK_COPPER_THICKNESS : -HACK_BOARD_THICKNESS - HACK_COPPER_THICKNESS - HACK_MASK_THICKNESS - HACK_SILK_THICKNESS, /* Bottom */
                                    (side == TOP_SIDE) ? HACK_SILK_THICKNESS + HACK_MASK_THICKNESS + HACK_COPPER_THICKNESS : -HACK_BOARD_THICKNESS - HACK_COPPER_THICKNESS - HACK_MASK_THICKNESS,                       /* Top */
#else
                                    (side == TOP_SIDE) ? -HACK_BOARD_THICKNESS / 2 - HACK_COPPER_THICKNESS - HACK_MASK_THICKNESS                       : HACK_BOARD_THICKNESS / 2 + HACK_COPPER_THICKNESS + HACK_MASK_THICKNESS + HACK_SILK_THICKNESS, /* Bottom */
                                    (side == TOP_SIDE) ? -HACK_BOARD_THICKNESS / 2 - HACK_COPPER_THICKNESS - HACK_MASK_THICKNESS - HACK_SILK_THICKNESS : HACK_BOARD_THICKNESS / 2 + HACK_COPPER_THICKNESS + HACK_MASK_THICKNESS, /* Top */
#endif
                                    silk_appearance,
                                    NULL,  /* top_bot_appearance */
                                    false, /* Don't invert */
                                    (side == TOP_SIDE) ? "Top silk" : "Bottom silk"); /* Name */

//  destroy_appearance (silk_appearance); /* XXX: HANGING ON TO THIS FOR NOW */

  poly_Free (&info.poly);

  return objects;
}

void
object3d_set_board_thickness (Coord thickness)
{
  board_thickness = thickness;
}
