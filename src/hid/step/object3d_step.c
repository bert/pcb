#include <stdio.h>
#include <stdbool.h>

#include <glib.h>

#include "data.h"

#include "hid/common/step_id.h"
#include "hid/common/quad.h"
#include "hid/common/vertex3d.h"
#include "hid/common/contour3d.h"
#include "hid/common/appearance.h"
#include "hid/common/face3d.h"
#include "hid/common/edge3d.h"
#include "hid/common/object3d.h"

#include "step_writer.h"

#include "pcb-printf.h"

#include "object3d_step.h"


#define REVERSED_PCB_CONTOURS 1 /* PCB Contours are reversed from the expected CCW for outer ordering - once the Y-coordinate flip is taken into account */

#define EPSILON 1e-5 /* XXX: Unknown  what this needs to be */

#ifdef REVERSED_PCB_CONTOURS
#define STEP_X_TO_COORD(pcb, x) (MM_TO_COORD((x)))
#define STEP_Y_TO_COORD(pcb, y) ((pcb)->MaxHeight - MM_TO_COORD((y)))
#else
/* XXX: BROKEN UPSIDE DOWN OUTPUT */
#define STEP_X_TO_COORD(pcb, x) (MM_TO_COORD((x)))
#define STEP_Y_TO_COORD(pcb, y) (MM_TO_COORD((y)))
#endif


static step_id_list
presentation_style_assignments_from_appearance (step_file *step, appearance *appear)
{
  step_id colour = step_colour_rgb (step, "", appear->r, appear->g, appear->b);
  step_id fill_area_style = step_fill_area_style (step, "", make_step_id_list (1, step_fill_area_style_colour (step, "", colour)));
  step_id surface_side_style = step_surface_side_style (step, "", make_step_id_list (1, step_surface_style_fill_area (step, fill_area_style)));
  step_id_list styles_list = make_step_id_list (1, step_surface_style_usage (step, "BOTH", surface_side_style));
  step_id_list psa_list = make_step_id_list (1, step_presentation_style_assignment (step, styles_list));

  return psa_list;
}

static step_file *
start_ap214_file (const char *filename)
{
  FILE *f;
  time_t currenttime;
  struct tm utc;

  f = fopen (filename, "w");
  if (f == NULL)
    {
      perror (filename);
      return NULL;
    }

  currenttime = time (NULL);
  gmtime_r (&currenttime, &utc);

  fprintf (f, "ISO-10303-21;\n");
  fprintf (f, "HEADER;\n");
  fprintf (f, "FILE_DESCRIPTION (\n"
              "/* description */ ('STEP AP214 export of circuit board'),\n"
              "/* implementation level */ '1');\n");
  fprintf (f, "FILE_NAME (/* name */ '%s',\n"
              "/* time_stamp */ '%.4d-%.2d-%.2dT%.2d:%.2d:%.2d',\n"
              "/* author */ ( '' ),\n"
              "/* organisation */ ( '' ),\n"
              "/* preprocessor_version */ 'PCB STEP EXPORT',\n"
              "/* originating system */ '%s " VERSION "',\n"
              "/* authorisation */ '' );\n",
           filename,
           1900 + utc.tm_year, 1 + utc.tm_mon, utc.tm_mday, utc.tm_hour, utc.tm_min, utc.tm_sec,
           Progname);
  fprintf (f, "FILE_SCHEMA (( 'AUTOMOTIVE_DESIGN' ));\n");
  fprintf (f, "ENDSEC;\n");
  fprintf (f, "\n");
  fprintf (f, "DATA;\n");

  return step_output_file (f);
}

static void
finish_ap214_file (step_file *step)
{
  fprintf (step->f, "ENDSEC;\n" );
  fprintf (step->f, "END-ISO-10303-21;\n" );
  fclose (step->f);
}

static void
step_product_fragment (step_file *step, char *part_id, char *part_name, char *part_description,
                       step_id *geometric_representation_context,
                       step_id *product_definition_shape)
{
  step_id application_context_identifier;
  step_id product_identifier;
  step_id product_definition_identifier;
  step_id product_definition_shape_identifier;
  step_id geometric_representation_context_identifier;

  /* Setup the context of the "product" we are defining", and that it is a 'part' */
  application_context_identifier = step_application_context (step, "automotive_design");
  step_application_protocol_definition (step, "draft international standard", "automotive_design", "1998", application_context_identifier);
  product_identifier = step_product (step, part_id, part_name /* This one is picked up by freecad */, part_description,
                                     make_step_id_list (1, step_product_context (step, "NONE", application_context_identifier, "mechanical")));
  step_product_related_product_category (step, "part", NULL, make_step_id_list (1, product_identifier));

  /* Setup the specific definition of the product we are defining */
  product_definition_identifier = step_product_definition (step, "UNKNOWN", "",
                                                           step_product_definition_formation (step, "any", "", product_identifier), /* Versioning for the product */
                                                           step_product_definition_context (step, "detailed design", application_context_identifier, "design"));
  product_definition_shape_identifier = step_product_definition_shape (step, "NONE", "NONE", product_definition_identifier);

  geometric_representation_context_identifier = make_3d_metric_step_geometric_representation_context (step);

  if (geometric_representation_context != NULL)
    *geometric_representation_context = geometric_representation_context_identifier;

  if (product_definition_shape != NULL)
    *product_definition_shape = product_definition_shape_identifier;
}

static void
object3d_to_step_body_fragment (step_file *step,
                                object3d *object,
                                char *body_name,
                                step_id *brep,
                                step_id_list *styled_item_identifiers)
{
  step_id brep_identifier;
  step_id pcb_shell_identifier;
  step_id brep_style_identifier;
  GList *shell_face_list = NULL;
  GList *face_iter;
  GList *edge_iter;
  GList *vertex_iter;
  GList *contour_iter;

#define FWD 1
#define REV 2
#define ORIENTED_EDGE_IDENTIFIER(e) (((edge_info *)UNDIR_DATA (e))->edge_identifier + ((e & 2) ? REV : FWD))

  /* Define ininite planes corresponding to every planar face, and cylindrical surfaces for every cylindrical face */
  for (face_iter = object->faces; face_iter != NULL; face_iter = g_list_next (face_iter))
    {
      face3d *face = face_iter->data;

      if (face->is_cylindrical)
        {
          /* CYLINDRICAL SURFACE NORMAL POINTS OUTWARDS AWAY FROM ITS AXIS.
           * face->surface_orientation_reversed NEEDS TO BE SET FOR HOLES IN THE SOLID
           */
          face->surface_identifier =
            step_cylindrical_surface (step, "NONE",
                                      step_axis2_placement_3d (step, "NONE",
                                                               step_cartesian_point (step, "NONE", face->cx, face->cy, face->cz),
                                                                     step_direction (step, "NONE", face->ax, face->ay, face->az),
                                                                     step_direction (step, "NONE", face->nx, face->ny, face->nz)),
                                      face->radius);
        }
      else
        {
          contour3d *outer_contour = face->contours->data;
          vertex3d *ov = ODATA (outer_contour->first_edge);
          vertex3d *dv = DDATA (outer_contour->first_edge);

          double rx, ry, rz;

          rx = dv->x - ov->x;
          ry = dv->y - ov->y;
          rz = dv->z - ov->z;

          /* Catch the circular face case where the start and end vertices are identical */
          if (rx < EPSILON && -rx < EPSILON &&
              ry < EPSILON && -ry < EPSILON &&
              rz < EPSILON && -rz < EPSILON)
            {
              rx = 1., ry = 0., rz = 0.;
            }

          face->surface_identifier =
            step_plane (step, "NONE",
                        step_axis2_placement_3d (step, "NONE",
                                                 step_cartesian_point (step, "NONE", ov->x,  /* A point on the plane. Defines 0,0 of the plane's parameterised coords. */
                                                                                     ov->y,      /* Set this to the origin vertex of the first edge */
                                                                                     ov->z),     /* this contour links to in the quad edge structure. */
                                                       step_direction (step, "NONE", face->nx, face->ny, face->nz), /* An axis direction normal to the the face - Gives z-axis */
                                                       step_direction (step, "NONE", rx,     /* Reference x-axis, orthogonal to z-axis. */
                                                                                     ry,         /* Define this to be along the first edge this */
                                                                                     rz)));      /* contour links to in the quad edge structure */
        }
    }

  /* Define the infinite lines corresponding to every edge (either lines or circles)*/
  for (edge_iter = object->edges; edge_iter != NULL; edge_iter = g_list_next (edge_iter))
    {
      edge_ref edge = (edge_ref)edge_iter->data;
      edge_info *info = UNDIR_DATA (edge);

      if (info->is_round)
        {
          info->infinite_line_identifier =
            step_circle (step, "NONE",
                         step_axis2_placement_3d (step, "NONE",
                                                  step_cartesian_point (step, "NONE", info->cx, info->cy, info->cz),  // <--- Center of the circle
                                                        step_direction (step, "NONE", info->nx, info->ny, info->nz),  // <--- Normal of the circle
                                                        step_direction (step, "NONE", -1.0,     0.0,      0.0)),      // <--- Approximate X-axis direction of placement /* XXX: PULL FROM FACE DATA */
                                                        info->radius);
        }
      else
        {
          vertex3d *ov = ODATA (edge);
          vertex3d *dv = DDATA (edge);

          double dir_x, dir_y, dir_z;

          dir_x = dv->x - ov->x;
          dir_y = dv->y - ov->y;
          dir_z = dv->z - ov->z;

#if 1
          /* XXX: This avoids the test file step_outline_test.pcb failing to display properly in freecad when coordinates are slightly rounded */
          if (dir_x < EPSILON && -dir_x < EPSILON &&
              dir_y < EPSILON && -dir_y < EPSILON &&
              dir_z < EPSILON && -dir_z < EPSILON)
            {
              printf ("EDGE TOO SHORT TO DETERMINE DIRECTION - GUESSING! Coords (%f, %f)\n", ov->x, ov->y);
              pcb_printf ("Approx PCB coords of short edge: %#mr, %#mr\n", (Coord)STEP_X_TO_COORD (PCB, ov->x), (Coord)STEP_Y_TO_COORD (PCB, ov->y));
              dir_x = 1.0; /* DUMMY TO AVOID A ZERO LENGTH DIRECTION VECTOR */
            }
#endif

          info->infinite_line_identifier =
            step_line (step, "NONE",
                       step_cartesian_point (step, "NONE", ov->x, ov->y, ov->z),  // <--- A point on the line (the origin vertex)
                       step_vector (step, "NONE",
                                    step_direction (step, "NONE", dir_x, dir_y, dir_z), // <--- Direction along the line
                                    1000.0));     // <--- Arbitrary length in this direction for the parameterised coordinate "1".

        }
    }

  /* Define the vertices */
  for (vertex_iter = object->vertices; vertex_iter != NULL; vertex_iter = g_list_next (vertex_iter))
    {
      vertex3d *vertex = vertex_iter->data;

      vertex->vertex_identifier =
        step_vertex_point (step, "NONE", step_cartesian_point (step, "NONE", vertex->x, vertex->y, vertex->z));
    }

  /* Define the Edges */
  for (edge_iter = object->edges; edge_iter != NULL; edge_iter = g_list_next (edge_iter))
    {
      edge_ref edge = (edge_ref)edge_iter->data;
      edge_info *info = UNDIR_DATA (edge);
      step_id sv = ((vertex3d *)ODATA (edge))->vertex_identifier;
      step_id ev = ((vertex3d *)DDATA (edge))->vertex_identifier;

      /* XXX: The lookup of these edges by adding to info->edge_identifier requires the step_* functions to assign sequential identifiers */
      info->edge_identifier = step_edge_curve (step, "NONE", sv, ev, info->infinite_line_identifier, true);
      step_oriented_edge (step, "NONE", info->edge_identifier, true);  /* Add 1 to info->edge_identifier to find this (same) oriented edge */
      step_oriented_edge (step, "NONE", info->edge_identifier, false); /* Add 2 to info->edge_identifier to find this (back) oriented edge */
    }

  /* Define the faces */
  for (face_iter = object->faces; face_iter != NULL; face_iter = g_list_next (face_iter))
    {
      face3d *face = face_iter->data;
      bool outer_contour = true;
      step_id_list face_contour_list = NULL;

      for (contour_iter = face->contours;
           contour_iter != NULL;
           contour_iter = g_list_next (contour_iter), outer_contour = false)
        {
          contour3d *contour = contour_iter->data;
          edge_ref edge;
          step_id edge_loop;
          step_id_list edge_loop_edges = NULL;

          edge = contour->first_edge;
          do
            {
              edge_loop_edges = g_list_append (edge_loop_edges, GINT_TO_POINTER (ORIENTED_EDGE_IDENTIFIER (edge)));
            }
          while (edge = LNEXT (edge), edge != contour->first_edge);

          edge_loop = step_edge_loop (step, "NONE", edge_loop_edges);

          if (outer_contour)
            contour->face_bound_identifier = step_face_outer_bound (step, "NONE", edge_loop, true);
          else
            contour->face_bound_identifier = step_face_bound (step, "NONE", edge_loop, true);

          face_contour_list = g_list_append (face_contour_list, GINT_TO_POINTER (contour->face_bound_identifier));
        }

      face->face_identifier = step_advanced_face (step, "NONE", face_contour_list, face->surface_identifier, !face->surface_orientation_reversed);
      shell_face_list = g_list_append (shell_face_list, GINT_TO_POINTER (face->face_identifier));
    }

  /* Closed shell which bounds the brep solid */
  pcb_shell_identifier = step_closed_shell (step, "NONE", shell_face_list);
  brep_identifier = step_manifold_solid_brep (step, body_name /* This is picked up as the solid body name by Solidworks */, pcb_shell_identifier);

  /* Body style */
  /* XXX: THERE MUST BE A BODY STYLE, CERTAINLY IF WE WANT TO OVER RIDE FACE COLOURS */
  brep_style_identifier = step_styled_item (step, "NONE", presentation_style_assignments_from_appearance (step, object->appear), brep_identifier);
  step_presentation_layer_assignment (step, "1", "Layer 1", make_step_id_list (1, brep_style_identifier));

  *styled_item_identifiers = step_id_list_append (*styled_item_identifiers, brep_style_identifier);

  /* Face styles */
  for (face_iter = object->faces; face_iter != NULL; face_iter = g_list_next (face_iter))
    {
      face3d *face = face_iter->data;

      if (face->appear != NULL)
        {
          step_id orsi = step_over_riding_styled_item (step, "NONE",
                                                       presentation_style_assignments_from_appearance (step, face->appear),
                                                       face->face_identifier, brep_style_identifier);
          *styled_item_identifiers = step_id_list_append (*styled_item_identifiers, orsi);
        }
    }

  if (brep != NULL)
    *brep = brep_identifier;

#undef ORIENTED_EDGE_IDENTIFIER
#undef FWD
#undef REV
}


static void
step_absr_fragment (step_file *step,
                    step_id_list brep_list,
                    step_id_list styled_item_list,
                    step_id geometric_representation_context_identifier,
                    step_id product_definition_shape_identifier,
                    step_id *shape_representation,
                    step_id *shape_definition_representation,
                    step_id *placement_axis)
{
  step_id shape_representation_identifier;
  step_id anchor_axis_identifier;
  step_id shape_definition_representation_identifier;

  /* Need an anchor in 3D space to orient the shape */
  anchor_axis_identifier = step_axis2_placement_3d (step, "NONE",
                                                    step_cartesian_point (step, "NONE", 0.0, 0.0, 0.0),
                                                          step_direction (step, "NONE", 0.0, 0.0, 1.0),
                                                          step_direction (step, "NONE", 1.0, 0.0, 0.0)),

  shape_representation_identifier =
    step_advanced_brep_shape_representation (step, "test_pcb_absr_name",
                                             step_id_list_append (brep_list, anchor_axis_identifier),
                                             geometric_representation_context_identifier);

  shape_definition_representation_identifier =
  step_shape_definition_representation (step, product_definition_shape_identifier, shape_representation_identifier);

  /* Emit references to the styled and over_ridden styled items */
  step_mechanical_design_geometric_presentation_representation (step, "", styled_item_list, geometric_representation_context_identifier);

  if (shape_definition_representation != NULL)
    *shape_definition_representation = shape_definition_representation_identifier;

  if (placement_axis != NULL)
    *placement_axis = anchor_axis_identifier;
}

void
object3d_list_export_to_step_part (GList *objects, const char *filename)
{
  step_file *step;
  step_id geometric_representation_context;
  step_id product_definition_shape;
  step_id shape_representation;
  step_id shape_definition_representation;
  step_id placement_axis;
  step_id comp_brep;
  GList *object_iter;
  int part;
  bool multiple_bodies;
  GString *part_id;
  GString *part_name;
  step_id_list breps;
  step_id_list styled_items;

  multiple_bodies = (g_list_next (objects) != NULL);

  step = start_ap214_file (filename);

  part_id   = g_string_new ("part id");
  part_name = g_string_new ("part name");

  step_product_fragment (step, part_id->str, part_name->str, "PCB model",
                         &geometric_representation_context,
                         &product_definition_shape);

  g_string_free (part_id, true);
  g_string_free (part_name, true);

  breps = make_step_id_list (0);
  styled_items = make_step_id_list (0);

  for (object_iter = objects, part = 1;
       object_iter != NULL;
       object_iter = g_list_next (object_iter), part++)
    {

      object3d *object = object_iter->data;
      GString *body_name;

      body_name = g_string_new ("part body");
      if (multiple_bodies)
        g_string_append_printf (body_name, " - %i", part);

      object3d_to_step_body_fragment (step, object, body_name->str, &comp_brep, &styled_items);

      g_string_free (body_name, true);

      breps = step_id_list_append (breps, comp_brep);
    }

  step_absr_fragment (step,
                      breps,
                      styled_items,
                      geometric_representation_context,
                      product_definition_shape,
                      &shape_representation,
                      &shape_definition_representation,
                      &placement_axis);

  finish_ap214_file (step);
}

static void
object3d_to_step_fragment (step_file *step, object3d *object, char *part_id, char *part_name, char *part_description, char *body_name,
                           step_id *shape_definition_representation, step_id *placement_axis)
{
  step_id product_definition_shape_identifier;
  step_id geometric_representation_context_identifier;
  step_id brep_identifier;
  GList *styled_item_identifiers = NULL;

  step_product_fragment (step, part_id, part_name, part_description,
                         &geometric_representation_context_identifier,
                         &product_definition_shape_identifier);

  object3d_to_step_body_fragment (step, object, body_name, &brep_identifier, &styled_item_identifiers);

  step_absr_fragment (step,
                      make_step_id_list (1, brep_identifier),
                      styled_item_identifiers,
                      geometric_representation_context_identifier,
                      product_definition_shape_identifier,
                      NULL /* shape_representation */,
                      shape_definition_representation,
                      placement_axis);
}

void
object3d_list_export_to_step_assy (GList *objects, const char *filename)
{
  step_file *step;
  step_id comp_shape_definition_representation;
  step_id comp_placement_axis;
  GList *object_iter;
  int part;
  bool multiple_parts;

  multiple_parts = (g_list_next (objects) != NULL);

  step = start_ap214_file (filename);

  for (object_iter = objects, part = 1;
       object_iter != NULL;
       object_iter = g_list_next (object_iter), part++)
    {

      object3d *object = object_iter->data;
      GString *part_id;
      GString *part_name;
      GString *body_name;

      part_id   = g_string_new ("board");
      part_name = g_string_new ("PCB board");
      body_name = g_string_new ("PCB board body");

      if (multiple_parts)
        {
          g_string_append_printf (part_id, "-%i", part);
          g_string_append_printf (part_name, " - %i", part);
          g_string_append_printf (body_name, " - %i", part);
        }

      object3d_to_step_fragment (step, object, part_id->str, part_name->str, "PCB model", body_name->str,
                                 &comp_shape_definition_representation, &comp_placement_axis);

      g_string_free (part_id, true);
      g_string_free (part_name, true);
      g_string_free (body_name, true);
    }

  finish_ap214_file (step);

  /* XXX: TODO: MAKE AN ASSEMBLY PRODUCT AND GATHER THE ABOVE PIECES INSIDE IT */
}

void
object3d_export_to_step (object3d *object, const char *filename)
{
  step_file *step;

  step = start_ap214_file (filename);
  object3d_to_step_fragment (step, object, "board", "PCB board", "PCB model", "PCB board body", NULL, NULL);
  finish_ap214_file (step);
}
