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

#include "rtree.h"
#include "rotate.h"

#include "pcb-printf.h"

#define PERFECT_ROUND_CONTOURS

#define REVERSED_PCB_CONTOURS 1 /* PCB Contours are reversed from the expected CCW for outer ordering - once the Y-coordinate flip is taken into account */

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


#define HACK_BOARD_THICKNESS MM_TO_COORD(1.6)

static GList *object3d_test_objects = NULL;

void
object3d_test_init (void)
{
  object3d_test_objects = object3d_from_board_outline ();
  object3d_test_board_outline ();
}

object3d *
make_object3d (char *name)
{
  static int object3d_count = 0;
  object3d *object;

  object = g_new0 (object3d, 1);
  object->id = object3d_count++;
  name = g_strdup (name);

  return object;
}

void
destroy_object3d (object3d *object)
{
  g_list_free_full (object->vertices, (GDestroyNotify)destroy_vertex3d);
  g_list_free_full (object->edges, (GDestroyNotify)destroy_edge);
  g_list_free_full (object->faces, (GDestroyNotify)destroy_face3d);
  g_free (object->name);
  g_free (object);
}

void
object3d_set_appearance (object3d *object, appearance *appear)
{
  object->appear = appear;
}

void
object3d_add_edge (object3d *object, edge_ref edge)
{
  object->edges = g_list_append (object->edges, (void *)edge);
}

void
object3d_add_vertex (object3d *object, vertex3d *vertex)
{
  object->vertices = g_list_append (object->vertices, vertex);
}

void
object3d_add_face (object3d *object, face3d *face)
{
  object->faces = g_list_append (object->faces, face);
}


float colors[12][3] = {{1., 0., 0.},
                       {1., 1., 0.},
                       {0., 1., 0.},
                       {0., 1., 1.},
                       {0.5, 0., 0.},
                       {0.5, 0.5, 0.},
                       {0., 0.5, 0.},
                       {0., 0.5, 0.5},
                       {1., 0.5, 0.5},
                       {1., 1., 0.5},
                       {0.5, 1., 0.5},
                       {0.5, 1., 1.}};


#define CIRC_SEGS 64

static void
draw_quad_edge (edge_ref e, void *data)
{
#if 0
  int id = ID(e) % 12;

  glColor3f (colors[id][0], colors[id][1], colors[id][2]);
#else
  glColor3f (1., 1., 1.);
#endif

  if (UNDIR_DATA(e) != NULL)
    {
      edge_info *info = UNDIR_DATA(e);
//      if (info->is_stitch)
//        return;
      if (info->is_round)
        {
          int i;
          glBegin (GL_LINES);
          for (i = 0; i < CIRC_SEGS; i++)
            {
              /* XXX: THIS ASSUMES THE CIRCLE LIES IN THE X-Y PLANE */
              glVertex3f (STEP_X_TO_COORD (PCB, info->cx + info->radius * cos (i * 2. * M_PI / (double)CIRC_SEGS)),
                          STEP_Y_TO_COORD (PCB, info->cy + info->radius * sin (i * 2. * M_PI / (double)CIRC_SEGS)),
                          STEP_Z_TO_COORD (PCB, info->cz));
              glVertex3f (STEP_X_TO_COORD (PCB, info->cx + info->radius * cos ((i + 1) * 2. * M_PI / (double)CIRC_SEGS)),
                          STEP_Y_TO_COORD (PCB, info->cy + info->radius * sin ((i + 1) * 2. * M_PI / (double)CIRC_SEGS)),
                          STEP_Z_TO_COORD (PCB, info->cz));
            }
          glEnd ();
          return;
        }
    }

  glBegin (GL_LINES);
  glVertex3f (STEP_X_TO_COORD (PCB, ((vertex3d *)ODATA(e))->x),
              STEP_Y_TO_COORD (PCB, ((vertex3d *)ODATA(e))->y),
              STEP_X_TO_COORD (PCB, ((vertex3d *)ODATA(e))->z));
  glVertex3f (STEP_X_TO_COORD (PCB, ((vertex3d *)DDATA(e))->x),
              STEP_Y_TO_COORD (PCB, ((vertex3d *)DDATA(e))->y),
              STEP_X_TO_COORD (PCB, ((vertex3d *)DDATA(e))->z));
  glEnd ();
}

static void
object3d_draw_debug_single (object3d *object, void *user_data)
{
  g_return_if_fail (object->edges != NULL);

//  quad_enum ((edge_ref)object->edges->data, draw_quad_edge, NULL);
  g_list_foreach (object->edges, (GFunc)draw_quad_edge, NULL);
}

void
object3d_draw_debug (void)
{
  g_list_foreach (object3d_test_objects, (GFunc)object3d_draw_debug_single, NULL);
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
#ifdef PERFECT_ROUND_CONTOURS
  if (contour->is_round)
    {
      /* HACK SPECIAL CASE FOR ROUND CONTOURS */
      return true;
    }
#endif

  return false;
}

static void
get_contour_edge_n_round_geometry_in_step_mm (PLINE *contour, int n, double *cx, double *cy, double *r, bool *cw)
{
#ifdef PERFECT_ROUND_CONTOURS
  if (contour->is_round)
    {
      /* HACK SPECIAL CASE FOR ROUND CONTOURS */
      *cx = COORD_TO_STEP_X (PCB, contour->cx);
      *cy = COORD_TO_STEP_Y (PCB, contour->cy);
      *r = COORD_TO_MM (contour->radius);
      *cw = (contour->Flags.orient == PLF_INV);
    }
#endif
}

GList *
object3d_from_contours (const POLYAREA *contours,
                        double zbot,
                        double ztop,
                        const appearance *master_object_appearance,
                        const appearance *master_top_bot_appearance)
{
  GList *objects = NULL;
  object3d *object;
  appearance *object_appearance = NULL;
  appearance *top_bot_appearance = NULL;
  const POLYAREA *pa;
  PLINE *contour;
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

  if (contours == NULL)
    return NULL;

  /* Loop over all board outline pieces */
  pa = contours;
  do
    {

      contour = pa->contours;
      ncontours = 0;
      npoints = 0;

      ct = contour;
      while (ct != NULL)
        {
          ncontours ++;
          npoints += get_contour_npoints (ct);
          ct = ct->next;
        }

      object = make_object3d (PCB->Name);

      if (master_object_appearance != NULL)
        {
          object_appearance = make_appearance ();
          appearance_set_appearance (object_appearance, master_object_appearance);
        }

      if (master_top_bot_appearance != NULL)
        {
          top_bot_appearance = make_appearance ();
          appearance_set_appearance (top_bot_appearance, master_top_bot_appearance);
        }

      object3d_set_appearance (object, object_appearance);

      vertices = malloc (sizeof (vertex3d *) * 2 * npoints); /* (n-bottom, n-top) */
      edges    = malloc (sizeof (edge_ref  ) * 3 * npoints); /* (n-bottom, n-top, n-sides) */
      faces    = malloc (sizeof (face3d *) * (npoints + 2)); /* (n-sides, 1-bottom, 1-top */

      /* Define the vertices */
      ct = contour;
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
          faces[i] = make_face3d ();

          object3d_add_face (object, faces[i]);
          /* Pick one of the upright edges which is within this face outer contour loop, and link it to the face */
#ifdef REVERSED_PCB_CONTOURS
          face3d_add_contour (faces[i], make_contour3d (edges[2 * npoints + i]));
#else
          face3d_add_contour (faces[i], make_contour3d (SYM(edges[2 * npoints + i])));
#endif
        }

      faces[npoints] = make_face3d (); /* bottom_face */
#ifdef REVERSED_PCB_CONTOURS
      face3d_set_normal (faces[npoints], 0., 0., -1.);
#else
      face3d_set_normal (faces[npoints], 0., 0., 1.);
#endif
      face3d_set_appearance (faces[npoints], top_bot_appearance);
      object3d_add_face (object, faces[npoints]);

      faces[npoints + 1] = make_face3d (); /* top_face */
#ifdef REVERSED_PCB_CONTOURS
      face3d_set_normal (faces[npoints + 1], 0., 0., 1.);
#else
      face3d_set_normal (faces[npoints + 1], 0., 0., -1.);
#endif
      face3d_set_appearance (faces[npoints + 1], top_bot_appearance);
      object3d_add_face (object, faces[npoints + 1]);

      /* Pick the first bottom / top edge within the bottom / top face outer contour loop, and link it to the face */
#ifdef REVERSED_PCB_CONTOURS
      face3d_add_contour (faces[npoints], make_contour3d (edges[0]));
      face3d_add_contour (faces[npoints + 1], make_contour3d (SYM(edges[npoints])));
#else
      face3d_add_contour (faces[npoints], make_contour3d (SYM(edges[0])));
      face3d_add_contour (faces[npoints + 1], make_contour3d (edges[npoints]));
#endif

      ct = contour;
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
#ifdef REVERSED_PCB_CONTOURS
              face3d_add_contour (faces[npoints], make_contour3d (edges[i]));
              face3d_add_contour (faces[npoints + 1], make_contour3d (SYM(edges[npoints + i])));
#else
              face3d_add_contour (faces[npoints], make_contour3d (SYM(edges[i])));
              face3d_add_contour (faces[npoints + 1], make_contour3d (edges[npoints + i]));
#endif
            }

          next_i_around_ct = start_of_ct + (offset_in_ct + 1) % ct_npoints;
          prev_i_around_ct = start_of_ct + (offset_in_ct + ct_npoints - 1) % ct_npoints;

          /* Setup the face normals for the edges along the contour extrusion (top and bottom are handled separaetely) */
          /* Define the (non-normalized) face normal to point to the outside of the contour */
#if REVERSED_PCB_CONTOURS
          /* Vertex ordering of the edge we're finding the normal to is reversed in this case */
          face3d_set_normal (faces[i], -(vertices[next_i_around_ct]->y - vertices[i]->y),
                                        (vertices[next_i_around_ct]->x - vertices[i]->x), 0.);
#else
          face3d_set_normal (faces[i],  (vertices[next_i_around_ct]->y - vertices[i]->y),
                                       -(vertices[next_i_around_ct]->x - vertices[i]->x), 0.);
#endif

          /* Assign the appropriate vertex geometric data to each edge end */
          ODATA (edges[              i]) = vertices[i];
          DDATA (edges[              i]) = vertices[next_i_around_ct];
          ODATA (edges[1 * npoints + i]) = vertices[1 * npoints + i];
          DDATA (edges[1 * npoints + i]) = vertices[1 * npoints + next_i_around_ct];
          ODATA (edges[2 * npoints + i]) = vertices[i];
          DDATA (edges[2 * npoints + i]) = vertices[1 * npoints + i];
#if REVERSED_PCB_CONTOURS
          RDATA (edges[              i]) = faces[i];
          LDATA (edges[              i]) = faces[npoints];
          RDATA (edges[1 * npoints + i]) = faces[npoints + 1];
          LDATA (edges[1 * npoints + i]) = faces[i];
          RDATA (edges[2 * npoints + i]) = faces[prev_i_around_ct];
          LDATA (edges[2 * npoints + i]) = faces[i];
#else
          LDATA (edges[              i]) = faces[i];
          RDATA (edges[              i]) = faces[npoints];
          LDATA (edges[1 * npoints + i]) = faces[npoints + 1];
          RDATA (edges[1 * npoints + i]) = faces[i];
          LDATA (edges[2 * npoints + i]) = faces[prev_i_around_ct];
          RDATA (edges[2 * npoints + i]) = faces[i];
#endif

          /* NB: Contours are counter clockwise in XY plane.
           *     edges[          0-npoints-1] are the base of the extrusion, following in the counter clockwise order
           *     edges[1*npoints-2*npoints-1] are the top  of the extrusion, following in the counter clockwise order
           *     edges[2*npoints-3*npoints-1] are the upright edges, oriented from bottom to top
           */

#ifdef REVERSED_PCB_CONTOURS  /* UNDERLYING DATA HAS CW CONTOURS FOR OUTER, CCW FOR INNER - E.g. PCB's polygons when translated into STEP coordinates */
          /* Link edges orbiting around each bottom vertex i (0 <= i < npoints) */
          splice (SYM(edges[prev_i_around_ct]), edges[2 * npoints + i]);
          splice (edges[2 * npoints + i], edges[i]);

          /* Link edges orbiting around each top vertex (npoints + i) (0 <= i < npoints) */
          splice (edges[npoints + i], SYM(edges[2 * npoints + i]));
          splice (SYM(edges[2 * npoints + i]), SYM(edges[npoints + prev_i_around_ct]));
#else /* UNDERLYING DATA HAS CCW CONTOURS FOR OUTER, CW FOR INNER. E.g. PCB's raw coordinates in X, Y */
          /* Link edges orbiting around each bottom vertex i (0 <= i < npoints) */
          splice (edges[i], edges[2 * npoints + i]);
          splice (edges[2 * npoints + i], SYM(edges[prev_i_around_ct]));

          /* Link edges orbiting around each top vertex (npoints + i) (0 <= i < npoints) */
          splice (SYM(edges[npoints + prev_i_around_ct]), SYM(edges[2 * npoints + i]));
          splice (SYM(edges[2 * npoints + i]),  edges[npoints + i]);
#endif

          if (get_contour_edge_n_is_round (ct, offset_in_ct))
            {
              double cx;
              double cy;
              double radius;
              double normal_z;
              bool cw;

              get_contour_edge_n_round_geometry_in_step_mm (ct, offset_in_ct, &cx, &cy, &radius, &cw);

              face3d_set_cylindrical (faces[i], cx, cy, 0., /* A point on the axis of the cylinder */
                                                0., 0., 1., /* Direction of the cylindrical axis */
                                                radius);

              /* XXX: DEPENDS ON INSIDE / OUTSIDE CORNER!! */
              if (ct->Flags.orient == PLF_INV)
                face3d_set_surface_orientation_reversed (faces[i]);

              face3d_set_normal (faces[i], 1., 0., 0.);  /* A normal to the axis direction */
                                        /* XXX: ^^^ Could line this up with the direction to the vertex in the corresponding circle edge */

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

        }

      if (0)
        {
          /* Cylinder centers on 45x45mm, stitch vertex is at 40x45mm. Radius is thus 5mm */

          edge_ref cylinder_edges[3];
          vertex3d *cylinder_vertices[2];
          face3d *cylinder_faces[2];

          /* Edge on top of board */
          cylinder_edges[0] = make_edge ();
          UNDIR_DATA (cylinder_edges[0]) = make_edge_info ();
#ifdef REVERSED_PCB_CONTOURS
          edge_info_set_round (UNDIR_DATA (cylinder_edges[0]),
                               COORD_TO_STEP_X (PCB, MM_TO_COORD (45.)), COORD_TO_STEP_Y (PCB, MM_TO_COORD (45.)), 0., /* Center of circle */
                                0.,   0., 1., /* Normal */
                                5.);          /* Radius */
#else
          edge_info_set_round (UNDIR_DATA (cylinder_edges[0]),
                               COORD_TO_STEP_X (PCB, MM_TO_COORD (45.)), COORD_TO_STEP_Y (PCB, MM_TO_COORD (45.)), 0., /* Center of circle */
                                0.,   0., 1., /* Normal */
                                5.);         /* Radius */
#endif
          object3d_add_edge (object, cylinder_edges[0]);

          /* Edge on top of cylinder */
          cylinder_edges[1] = make_edge ();
          UNDIR_DATA (cylinder_edges[1]) = make_edge_info ();
          edge_info_set_round (UNDIR_DATA (cylinder_edges[1]),
                               COORD_TO_STEP_X (PCB, MM_TO_COORD (45.)), COORD_TO_STEP_Y (PCB, MM_TO_COORD (45.)), 10., /* Center of circle */
                                0.,   0., 1.,  /* Normal */
                                5.);          /* Radius */
          object3d_add_edge (object, cylinder_edges[1]);

          /* Edge stitching cylinder */
          cylinder_edges[2] = make_edge ();
          UNDIR_DATA (cylinder_edges[2]) = make_edge_info ();
          edge_info_set_stitch (UNDIR_DATA (cylinder_edges[2]));
          object3d_add_edge (object, cylinder_edges[2]);

          /* Vertex on board top surface */
          cylinder_vertices[0] = make_vertex3d (COORD_TO_STEP_X (PCB, MM_TO_COORD (40.)), COORD_TO_STEP_Y (PCB, MM_TO_COORD (45.)), 0.); /* Bottom */
          object3d_add_vertex (object, cylinder_vertices[0]);

          /* Vertex on cylinder top surface */
          cylinder_vertices[1] = make_vertex3d (COORD_TO_STEP_X (PCB, MM_TO_COORD (40.)), COORD_TO_STEP_Y (PCB, MM_TO_COORD (45.)), 10.); /* Top */
          object3d_add_vertex (object, cylinder_vertices[1]);

          /* Cylindrical face */
          cylinder_faces[0] = make_face3d ();
          face3d_set_cylindrical (cylinder_faces[0], COORD_TO_STEP_X (PCB, MM_TO_COORD (45.)), COORD_TO_STEP_Y (PCB, MM_TO_COORD (45.)), 0., /* A point on the axis of the cylinder */
                                            0., 0., 1.,             /* Direction of the cylindrical axis */
                                            5.);                   /* Radius of cylinder */
          face3d_set_normal (cylinder_faces[0], 1., 0., 0.);       /* A normal to the axis direction */
                                       /* XXX: ^^^ Could line this up with the direction to the vertex in the corresponding circle edge */
          object3d_add_face (object, cylinder_faces[0]);
          face3d_add_contour (cylinder_faces[0], make_contour3d (cylinder_edges[0]));

          /* Top face of cylinder */
          cylinder_faces[1] = make_face3d (); /* top face of cylinder */
          face3d_set_normal (cylinder_faces[1], 0., 0., 1.);
          face3d_set_appearance (cylinder_faces[1], top_bot_appearance);
          object3d_add_face (object, cylinder_faces[1]);
          face3d_add_contour (cylinder_faces[1], make_contour3d (cylinder_edges[1]));

          /* Splice onto board */
          face3d_add_contour (faces[npoints + 1], make_contour3d (SYM(cylinder_edges[0])));

          /* Assign the appropriate vertex geometric data to each edge end */
          ODATA (cylinder_edges[0]) = cylinder_vertices[0];
          DDATA (cylinder_edges[0]) = cylinder_vertices[0];
          ODATA (cylinder_edges[1]) = cylinder_vertices[1];
          DDATA (cylinder_edges[1]) = cylinder_vertices[1];
          ODATA (cylinder_edges[2]) = cylinder_vertices[0];
          DDATA (cylinder_edges[2]) = cylinder_vertices[1];
          LDATA (cylinder_edges[0]) = cylinder_faces[0];
          RDATA (cylinder_edges[0]) = faces[npoints + 1]; /* TOP OF BOARD FACE */
          LDATA (cylinder_edges[1]) = cylinder_faces[1];
          RDATA (cylinder_edges[1]) = cylinder_faces[0];
          LDATA (cylinder_edges[2]) = cylinder_faces[0];
          RDATA (cylinder_edges[2]) = cylinder_faces[0];

          /* Splice things together.... */

          /* Link edges orbiting the cylinder bottom vertex */
          splice (cylinder_edges[0], cylinder_edges[2]);
          splice (cylinder_edges[2], SYM(cylinder_edges[0]));

          /* Link edges orbiting the cylinder top vertex */
          splice (SYM(cylinder_edges[2]), cylinder_edges[1]);
          splice (cylinder_edges[1], SYM(cylinder_edges[1]));
        }

      objects = g_list_append (objects, object);

    }
  while (pa = pa->f, pa != contours);

  return objects;
}

GList *
object3d_from_board_outline (void)
{
  POLYAREA *board_outline = board_outline_poly (true);
  appearance *board_appearance;
  appearance *top_bot_appearance;
  GList *objects;

  board_appearance = make_appearance ();
  top_bot_appearance = make_appearance ();
  appearance_set_color (board_appearance,   1.0, 1.0, 0.6);
  appearance_set_color (top_bot_appearance, 0.2, 0.8, 0.2);

  objects = object3d_from_contours (board_outline,
#ifdef REVERSED_PCB_CONTOURS
                                    -HACK_BOARD_THICKNESS, /* Bottom */
                                    0                    ,  /* Top */
#else
                                     HACK_BOARD_THICKNESS / 2, /* Bottom */
                                    -HACK_BOARD_THICKNESS / 2, /* Top */
#endif
                                    board_appearance,
                                    top_bot_appearance);

  destroy_appearance (board_appearance);
  destroy_appearance (top_bot_appearance);

  poly_Free (&board_outline);

  return objects;
}

void
object3d_test_board_outline (void)
{
  GList *objects;

  objects = object3d_from_board_outline ();
  object3d_list_export_to_step_assy (objects, "object3d_test.step");
  //object3d_export_to_step (objects->data, "object3d_test.step");

  g_list_free_full (objects, (GDestroyNotify)destroy_object3d);
}
