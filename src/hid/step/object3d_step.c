#include <stdio.h>
#include <stdbool.h>

#include <glib.h>

#include "hid/common/step_id.h"
#include "hid/common/quad.h"
#include "hid/common/vertex3d.h"
#include "hid/common/contour3d.h"
#include "hid/common/appearance.h"
#include "hid/common/face3d.h"
#include "hid/common/edge3d.h"
#include "hid/common/object3d.h"
#include "data.h"

#include "step_writer.h"

#include "pcb-printf.h"

#include "object3d_step.h"


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

void
object3d_export_to_step (object3d *object, const char *filename)
{
  FILE *f;
  step_file *step;
  time_t currenttime;
  struct tm utc;
  int geometric_representation_context_identifier;
  int shape_representation_identifier;
  int brep_identifier;
  int pcb_shell_identifier;
  int brep_style_identifier;
  GList *styled_item_identifiers = NULL;
  GList *shell_face_list = NULL;
  GList *face_iter;
  GList *edge_iter;
  GList *vertex_iter;
  GList *contour_iter;

  f = fopen (filename, "w");
  if (f == NULL)
    {
      perror (filename);
      return;
    }

  step = step_output_file (f);

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

  /* TEST */

  /* Setup the context of the "product" we are defining", and that it is a 'part' */

  fprintf (f, "#1 = APPLICATION_CONTEXT ( 'automotive_design' ) ;\n"
              "#2 = APPLICATION_PROTOCOL_DEFINITION ( 'draft international standard', 'automotive_design', 1998, #1 );\n"
              "#3 = PRODUCT_CONTEXT ( 'NONE', #1, 'mechanical' ) ;\n"
              "#4 = PRODUCT ('%s', '%s', '%s', (#3)) ;\n"
              "#5 = PRODUCT_RELATED_PRODUCT_CATEGORY ('part', $, (#4)) ;\n",
              "test_pcb_id", "test_pcb_name", "test_pcb_description");

  /* Setup the specific definition of the product we are defining */
  fprintf (f, "#6 = PRODUCT_DEFINITION_CONTEXT ( 'detailed design', #1, 'design' ) ;\n"
              "#7 = PRODUCT_DEFINITION_FORMATION_WITH_SPECIFIED_SOURCE ( 'ANY', '', #4, .NOT_KNOWN. ) ;\n"
              "#8 = PRODUCT_DEFINITION ( 'UNKNOWN', '', #7, #6 ) ;\n"
              "#9 = PRODUCT_DEFINITION_SHAPE ( 'NONE', 'NONE',  #8 ) ;\n");

  /* Need an anchor in 3D space to orient the shape */
  fprintf (f, "#10 =    CARTESIAN_POINT ( 'NONE', ( 0.0, 0.0, 0.0 ) ) ;\n"
              "#11 =          DIRECTION ( 'NONE', ( 0.0, 0.0, 1.0 ) ) ;\n"
              "#12 =          DIRECTION ( 'NONE', ( 1.0, 0.0, 0.0 ) ) ;\n"
              "#13 = AXIS2_PLACEMENT_3D ( 'NONE', #10, #11, #12 ) ;\n");

  /* Grr.. more boilerplate - this time unit definitions */

  fprintf (f, "#14 = UNCERTAINTY_MEASURE_WITH_UNIT (LENGTH_MEASURE( 1.0E-005 ), #15, 'distance_accuracy_value', 'NONE');\n"
              "#15 =( LENGTH_UNIT ( ) NAMED_UNIT ( * ) SI_UNIT ( .MILLI., .METRE. ) );\n"
              "#16 =( NAMED_UNIT ( * ) PLANE_ANGLE_UNIT ( ) SI_UNIT ( $, .RADIAN. ) );\n"
              "#17 =( NAMED_UNIT ( * ) SI_UNIT ( $, .STERADIAN. ) SOLID_ANGLE_UNIT ( ) );\n"
              "#18 =( GEOMETRIC_REPRESENTATION_CONTEXT ( 3 ) GLOBAL_UNCERTAINTY_ASSIGNED_CONTEXT ( ( #14 ) ) GLOBAL_UNIT_ASSIGNED_CONTEXT ( ( #15, #16, #17 ) ) REPRESENTATION_CONTEXT ( 'NONE', 'WORKASPACE' ) );\n");
  geometric_representation_context_identifier = 18;
  step->next_id = 19;

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

          face->surface_identifier =
            step_plane (step, "NONE",
                        step_axis2_placement_3d (step, "NONE",
                                                 step_cartesian_point (step, "NONE", ov->x,  /* A point on the plane. Defines 0,0 of the plane's parameterised coords. */
                                                                                     ov->y,      /* Set this to the origin vertex of the first edge */
                                                                                     ov->z),     /* this contour links to in the quad edge structure. */
                                                       step_direction (step, "NONE", face->nx, face->ny, face->nz), /* An axis direction normal to the the face - Gives z-axis */
                                                       step_direction (step, "NONE", dv->x - ov->x,     /* Reference x-axis, orthogonal to z-axis. */
                                                                                     dv->y - ov->y,         /* Define this to be along the first edge this */
                                                                                     dv->z - ov->z)));      /* contour links to in the quad edge structure */
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

          info->infinite_line_identifier =
            step_line (step, "NONE",
                       step_cartesian_point (step, "NONE", ov->x, ov->y, ov->z),  // <--- A point on the line (the origin vertex)
                       step_vector (step, "NONE",
                                    step_direction (step, "NONE", dv->x - ov->x,
                                                                  dv->y - ov->y,
                                                                  dv->z - ov->z),  // <--- Direction along the line
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
  brep_identifier = step_manifold_solid_brep (step, "PCB outline", pcb_shell_identifier);

  /* Body style */
  /* XXX: THERE MUST BE A BODY STYLE, CERTAINLY IF WE WANT TO OVER RIDE FACE COLOURS */
  brep_style_identifier = step_styled_item (step, "NONE", presentation_style_assignments_from_appearance (step, object->appear), brep_identifier);
  step_presentation_layer_assignment (step, "1", "Layer 1", make_step_id_list (1, brep_style_identifier));

  styled_item_identifiers = g_list_append (styled_item_identifiers, GINT_TO_POINTER (brep_style_identifier));

  /* Face styles */
  for (face_iter = object->faces; face_iter != NULL; face_iter = g_list_next (face_iter))
    {
      face3d *face = face_iter->data;

      if (face->appear != NULL)
        {
          step_id orsi = step_over_riding_styled_item (step, "NONE",
                                                       presentation_style_assignments_from_appearance (step, face->appear),
                                                       face->face_identifier, brep_style_identifier);
          styled_item_identifiers = g_list_append (styled_item_identifiers, GINT_TO_POINTER (orsi));
        }
    }

  /* Emit references to the styled and over_ridden styled items */
  step_mechanical_design_geometric_presentation_representation (step, "", styled_item_identifiers, geometric_representation_context_identifier);

  shape_representation_identifier =
    step_advanced_brep_shape_representation (step, "test_pcb_absr_name",
                                             make_step_id_list (2, brep_identifier, 13 /* XXX */), geometric_representation_context_identifier);

  step_shape_definition_representation (step, 9 /* XXX */, shape_representation_identifier);

#undef ORIENTED_EDGE_IDENTIFIER
#undef FWD
#undef REV

  fprintf (f, "ENDSEC;\n" );
  fprintf (f, "END-ISO-10303-21;\n" );

  fclose (f);
}
