#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <glib.h>

#include "quad.h"
#include "vertex3d.h"
#include "appearance.h"
#include "face3d.h"
#include "edge3d.h"
#include "object3d.h"
#include "polygon.h"
#include "data.h"



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

static object3d *object3d_test_object = NULL;

static void
print_edge_id (edge_ref e)
{
  printf ("ID %i.%i", ID(e), (unsigned int)e & 3u);
}

static void
debug_print_edge (edge_ref e, void *data)
{
  printf ("Edge ID %i.%i\n", ID(e), (int)e & 3u);

  printf ("Edge ONEXT is "); print_edge_id (ONEXT(e)); printf ("\n");
  printf ("Edge OPREV is "); print_edge_id (OPREV(e)); printf ("\n");
  printf ("Edge DNEXT is "); print_edge_id (DNEXT(e)); printf ("\n");
  printf ("Edge DPREV is "); print_edge_id (DPREV(e)); printf ("\n");
  printf ("Edge RNEXT is "); print_edge_id (RNEXT(e)); printf ("\n");
  printf ("Edge RPREV is "); print_edge_id (RPREV(e)); printf ("\n");
  printf ("Edge LNEXT is "); print_edge_id (LNEXT(e)); printf ("\n");
  printf ("Edge LPREV is "); print_edge_id (LPREV(e)); printf ("\n");
}

void
object3d_test_init (void)
{
  //object3d_test_object = object3d_create_test_cube ();
  object3d_test_object = object3d_from_board_outline ();
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

#define XOFFSET 50
#define YOFFSET 50
#define ZOFFSET 0
#define SCALE  10
object3d *
object3d_create_test_cube (void)
{
  object3d *object;
  vertex3d *cube_vertices[8];
  edge_ref cube_edges[12];
  face3d *faces[6];
  int i;

  object = make_object3d ("TEST CUBE");

  cube_vertices[0] = make_vertex3d (XOFFSET + SCALE * 0., YOFFSET + SCALE * 0., ZOFFSET + SCALE *  0.);
  cube_vertices[1] = make_vertex3d (XOFFSET + SCALE * 1., YOFFSET + SCALE * 0., ZOFFSET + SCALE *  0.);
  cube_vertices[2] = make_vertex3d (XOFFSET + SCALE * 1., YOFFSET + SCALE * 0., ZOFFSET + SCALE * -1.);
  cube_vertices[3] = make_vertex3d (XOFFSET + SCALE * 0., YOFFSET + SCALE * 0., ZOFFSET + SCALE * -1.);
  cube_vertices[4] = make_vertex3d (XOFFSET + SCALE * 0., YOFFSET + SCALE * 1., ZOFFSET + SCALE *  0.);
  cube_vertices[5] = make_vertex3d (XOFFSET + SCALE * 1., YOFFSET + SCALE * 1., ZOFFSET + SCALE *  0.);
  cube_vertices[6] = make_vertex3d (XOFFSET + SCALE * 1., YOFFSET + SCALE * 1., ZOFFSET + SCALE * -1.);
  cube_vertices[7] = make_vertex3d (XOFFSET + SCALE * 0., YOFFSET + SCALE * 1., ZOFFSET + SCALE * -1.);

  for (i = 0; i < 8; i++)
    object3d_add_vertex (object, cube_vertices[i]);

  for (i = 0; i < 12; i++)
    {
      cube_edges[i] = make_edge ();
      object3d_add_edge (object, cube_edges[i]);
    }

  for (i = 0; i < 6; i++)
    {
      faces[i] = make_face3d ();
      object3d_add_face (object, faces[i]);
    }

  for (i = 0; i < 4; i++)
    {
      int next_vertex = (i + 1) % 4;
      int prev_vertex = (i + 3) % 4;

      /* Assign bottom edge endpoints */
      ODATA (cube_edges[i]) = cube_vertices[i];
      DDATA (cube_edges[i]) = cube_vertices[next_vertex];

      /* Assign top edge endpoints */
      ODATA (cube_edges[4 + i]) = cube_vertices[4 + i];
      DDATA (cube_edges[4 + i]) = cube_vertices[4 + next_vertex];

      /* Assign side edge endpoints */
      ODATA (cube_edges[8 + i]) = cube_vertices[i];
      DDATA (cube_edges[8 + i]) = cube_vertices[4 + i];

      /* Link up edges orbiting around each bottom vertex */
      splice (cube_edges[i], cube_edges[8 + i]);
      splice (cube_edges[8 + i], SYM(cube_edges[prev_vertex]));

      /* Link up edges orbiting around each bottom top */
      splice (cube_edges[4 + i], SYM(cube_edges[4 + prev_vertex]));
      splice (SYM(cube_edges[4 + prev_vertex]), SYM(cube_edges[8 + i]));

    }

  quad_enum (cube_edges[0], debug_print_edge, NULL);


  return object;
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
      if (info->is_stitch)
        return;
      if (info->is_round)
        {
          int i;
          glBegin (GL_LINES);
          for (i = 0; i < CIRC_SEGS; i++)
            {
              glVertex3f (MM_TO_COORD (info->cx + info->radius * cos (i * 2. * M_PI / (double)CIRC_SEGS)),
                          MM_TO_COORD (info->cy + info->radius * sin (i * 2. * M_PI / (double)CIRC_SEGS)),
                          MM_TO_COORD (((vertex3d *)ODATA(e))->z));
              glVertex3f (MM_TO_COORD (info->cx + info->radius * cos ((i + 1) * 2. * M_PI / (double)CIRC_SEGS)),
                          MM_TO_COORD (info->cy + info->radius * sin ((i + 1) * 2. * M_PI / (double)CIRC_SEGS)),
                          MM_TO_COORD (((vertex3d *)ODATA(e))->z));
            }
          glEnd ();
          return;
        }
    }

  glBegin (GL_LINES);
  glVertex3f (MM_TO_COORD (((vertex3d *)ODATA(e))->x), MM_TO_COORD (((vertex3d *)ODATA(e))->y), MM_TO_COORD (((vertex3d *)ODATA(e))->z));
  glVertex3f (MM_TO_COORD (((vertex3d *)DDATA(e))->x), MM_TO_COORD (((vertex3d *)DDATA(e))->y), MM_TO_COORD (((vertex3d *)DDATA(e))->z));
  glEnd ();
}

void
object3d_draw_debug (void)
{
  g_return_if_fail (object3d_test_object->edges != NULL);

  quad_enum ((edge_ref)object3d_test_object->edges->data, draw_quad_edge, NULL);
}

/*********************************************************************************************************/

static int
get_contour_npoints (PLINE *contour)
{
  /* HACK FOR ROUND CONTOURS */
  if (contour->is_round)
    return 1;

  return contour->Count;
}

static void
get_contour_coord_n_in_mm (PLINE *contour, int n, double *x, double *y)
{
  VNODE *vertex = &contour->head;

  if (contour->is_round)
    {
      /* HACK SPECIAL CASE FOR ROUND CONTOURS */

      /* We define an arbitrary point on the contour. This is used, for example,
       * to define a coordinate system along the contour, and coincides with where
       * we add a straight edge down the side of an extruded cylindrical shape.
       */
      *x = COORD_TO_MM (contour->cx - contour->radius);
      *y = COORD_TO_MM (contour->cy); /* FIXME: PCB's coordinate system has y increasing downwards */

      return;
    }

  while (n > 0)
    {
      vertex = vertex->next; /* The VNODE structure is circularly linked, so wrapping is OK */
      n--;
    }

  *x = COORD_TO_MM (vertex->point[0]);
  *y = COORD_TO_MM (vertex->point[1]); /* FIXME: PCB's coordinate system has y increasing downwards */
}

object3d *
object3d_from_board_outline (void)
{
  object3d *object;
  appearance *board_appearance;
  POLYAREA *outline;
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

  outline = board_outline_poly (true);
  //outline = board_outline_poly (false); /* (FOR NOW - just the outline, no holes) */
  ncontours = 0;
  npoints = 0;

  /* XXX: There can be more than one contour, but for now we restrict ourselves to the first one */
  contour = outline->contours;

  if (1)
    {
      ct = contour;
      while (ct != NULL)
        {
          ncontours ++;
          npoints += get_contour_npoints (ct);
          ct = ct->next;
        }

      object = make_object3d (PCB->Name);
      board_appearance = make_appearance ();
      appearance_set_color (board_appearance, 1., 1., 0.);

      object3d_set_appearance (object, board_appearance);

      vertices = malloc (sizeof (vertex3d *) * 2 * npoints); /* (n-bottom, n-top) */
      edges    = malloc (sizeof (edge_ref  ) * 3 * npoints); /* (n-bottom, n-top, n-sides) */
      faces    = malloc (sizeof (face3d *) * (npoints + 2)); /* (n-sides, 1-bottom, 1-top */

      /* Define the vertices */
      ct = contour;
      start_of_ct = 0;
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

          get_contour_coord_n_in_mm (ct, offset_in_ct, &x1, &y1);
          vertices[i]           = make_vertex3d (x1, y1, -COORD_TO_MM (HACK_BOARD_THICKNESS)); /* Bottom */
          vertices[npoints + i] = make_vertex3d (x1, y1, 0); /* Top */

          object3d_add_vertex (object, vertices[i]);
          object3d_add_vertex (object, vertices[npoints + i]);
        }

      /* Define the edges */
      for (i = 0; i < 3 * npoints; i++)
        {
          edges[i] = make_edge ();
          object3d_add_edge (object, edges[i]);
        }

      /* Define the faces */
      for (i = 0; i < npoints; i++)
        {
          faces[i] = make_face3d ();
          /* Pick one of the upright edges which is within this face outer contour loop, and link it to the face */
          face3d_add_contour (faces[i], edges[2 * npoints + i]);
        }
      faces[npoints] = make_face3d (); /* bottom_face */
      faces[npoints + 1] = make_face3d (); /* top_face */

      /* Pick the first bottom / top edge which within the bottom / top face outer contour loop, and link it to the face */
      face3d_add_contour (faces[npoints], edges[0]);
      face3d_add_contour (faces[npoints + 1], edges[npoints]);

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
              face3d_add_contour (faces[npoints], edges[i]);
              face3d_add_contour (faces[npoints + 1], edges[npoints + i]);
            }

          next_i_around_ct = start_of_ct + (offset_in_ct + 1) % ct_npoints;
          prev_i_around_ct = start_of_ct + (offset_in_ct + ct_npoints - 1) % ct_npoints;

          /* Assign the appropriate vertex geometric data to each edge end */
          ODATA (edges[              i]) = vertices[i];
          DDATA (edges[              i]) = vertices[next_i_around_ct];
          ODATA (edges[1 * npoints + i]) = vertices[1 * npoints + i];
          DDATA (edges[1 * npoints + i]) = vertices[1 * npoints + next_i_around_ct];
          ODATA (edges[2 * npoints + i]) = vertices[i];
          DDATA (edges[2 * npoints + i]) = vertices[1 * npoints + i];
          LDATA (edges[              i]) = faces[i];
          RDATA (edges[              i]) = faces[npoints];
          LDATA (edges[1 * npoints + i]) = faces[npoints + 1];
          RDATA (edges[1 * npoints + i]) = faces[i];
          LDATA (edges[2 * npoints + i]) = faces[prev_i_around_ct];
          RDATA (edges[2 * npoints + i]) = faces[i];

          /* Link edges orbiting around each bottom vertex i (0 <= i < npoints) */
          splice (edges[i], edges[npoints + i]);
          splice (edges[npoints + i], SYM(edges[next_i_around_ct]));

          /* Link edges orbiting around each top vertex (npoints + i) (0 <= i < npoints) */
          splice (edges[npoints + i], SYM(edges[npoints + next_i_around_ct]));
          splice (SYM(edges[npoints + next_i_around_ct]), SYM(edges[2 * npoints + i]));

          if (ct->is_round)
            {
              UNDIR_DATA (edges[0 * npoints + i]) = make_edge_info (false, true, COORD_TO_MM (ct->cx), COORD_TO_MM (ct->cy), COORD_TO_MM (ct->radius));
              UNDIR_DATA (edges[1 * npoints + i]) = make_edge_info (false, true, COORD_TO_MM (ct->cx), COORD_TO_MM (ct->cy), COORD_TO_MM (ct->radius));
              UNDIR_DATA (edges[2 * npoints + i]) = make_edge_info (true,  true, COORD_TO_MM (ct->cx), COORD_TO_MM (ct->cy), COORD_TO_MM (ct->radius));
            }
        }
    }

  poly_Free (&outline);

  return object;
}
