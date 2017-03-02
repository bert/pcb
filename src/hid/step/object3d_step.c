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

#include "object3d_step.h"


void
object3d_export_to_step (object3d *object, const char *filename)
{
  FILE *f;
  time_t currenttime;
  struct tm utc;
  int next_step_identifier;
  int geometric_representation_context_identifier;
  int shape_representation_identifier;
  int brep_identifier;
  int pcb_shell_identifier;
  int brep_style_identifier;
  GList *styled_item_identifiers = NULL;
  GList *styled_item_iter;
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

  /* Save a place for the advanced_brep_shape_representation identifier */
  next_step_identifier = 19;
  shape_representation_identifier = next_step_identifier++;

  fprintf (f, "#20 = SHAPE_DEFINITION_REPRESENTATION ( #9, #%i ) ;\n", shape_representation_identifier);

  /* Save a place for the brep identifier */
  next_step_identifier = 21;
  brep_identifier = next_step_identifier++;

  /* Body style */
  fprintf (f, "#22 = COLOUR_RGB ( '', %f, %f, %f ) ;\n", object->appear->r, object->appear->g, object->appear->b);
  fprintf (f, "#23 = FILL_AREA_STYLE_COLOUR ( '', #22 ) ;\n"
              "#24 = FILL_AREA_STYLE ('', ( #23 ) ) ;\n"
              "#25 = SURFACE_STYLE_FILL_AREA ( #24 ) ;\n"
              "#26 = SURFACE_SIDE_STYLE ('', ( #25 ) ) ;\n"
              "#27 = SURFACE_STYLE_USAGE ( .BOTH. , #26 ) ;\n"
              "#28 = PRESENTATION_STYLE_ASSIGNMENT ( ( #27 ) ) ;\n");
  fprintf (f, "#29 = STYLED_ITEM ( 'NONE', ( #28 ), #%i ) ;\n", brep_identifier);
  brep_style_identifier = 29;
  fprintf (f, "#30 = PRESENTATION_LAYER_ASSIGNMENT (  '1', 'Layer 1', ( #%i ) ) ;\n", brep_style_identifier);

  next_step_identifier = 31;
  styled_item_identifiers = g_list_append (styled_item_identifiers, GINT_TO_POINTER (brep_style_identifier));

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
           * BECAUSE OUR ROUND CONTOURS ARE (CURRENTLY) ALWAYS HOLES IN THE SOLID,
           * THIS MEANS THE CYLINDER NORMAL POINTS INTO THE OBJECT
           */
          fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
                      "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                      "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                      "#%i = AXIS2_PLACEMENT_3D ( 'NONE', #%i, #%i, #%i ) ; "
                      "#%i = CYLINDRICAL_SURFACE ( 'NONE', #%i, %f ) ;\n",
                   next_step_identifier,     /* A point on the axis of the cylinder */ face->cx, face->cy, face->cz,
                   next_step_identifier + 1, /* Direction of the cylindrical axis */   face->ax, face->ay, face->az,
                   next_step_identifier + 2, /* A normal to the axis direction */      face->nx, face->ny, face->nz,
                   next_step_identifier + 3, next_step_identifier, next_step_identifier + 1, next_step_identifier + 2,
                   next_step_identifier + 4, next_step_identifier + 3, face->radius);

          face->surface_identifier = next_step_identifier + 4;
          next_step_identifier = next_step_identifier + 5;
        }
      else
        {
          contour3d *outer_contour = face->contours->data;
          edge_ref first_edge = outer_contour->first_edge;

          double ox, oy, oz;
          double nx, ny, nz;
          double rx, ry, rz;

          /* Define 0,0 of the face coordinate system to arbitraily correspond to the
             origin vertex of the edge this contour links to in the quad edge structure.
           */
          ox = ((vertex3d *)ODATA (first_edge))->x;
          oy = ((vertex3d *)ODATA (first_edge))->y;
          oz = ((vertex3d *)ODATA (first_edge))->z;

          nx = face->nx;
          ny = face->ny;
          nz = face->nz;

          /* Define the reference x-axis of the face coordinate system to be along the
             edge this contour links to in the quad edge data structure.
           */

          rx = ((vertex3d *)DDATA (first_edge))->x - ox;
          ry = ((vertex3d *)DDATA (first_edge))->y - oy;
          rz = ((vertex3d *)DDATA (first_edge))->z - oz;

          fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
                      "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                      "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                      "#%i = AXIS2_PLACEMENT_3D ( 'NONE', #%i, #%i, #%i ) ; "
                      "#%i = PLANE ( 'NONE',  #%i ) ;\n",
                   next_step_identifier,     /* A point on the plane. Forms 0,0 of its parameterised coords. */ ox, oy, oz,
                   next_step_identifier + 1, /* An axis direction normal to the the face - Gives z-axis */      nx, ny, nz,
                   next_step_identifier + 2, /* Reference x-axis, orthogonal to z-axis above */                 rx, ry, rz,
                   next_step_identifier + 3, next_step_identifier, next_step_identifier + 1, next_step_identifier + 2,
                   next_step_identifier + 4, next_step_identifier + 3);

          face->surface_identifier = next_step_identifier + 4;
          next_step_identifier = next_step_identifier + 5;
        }
    }

  /* Define the infinite lines corresponding to every edge (either lines or circles)*/
  for (edge_iter = object->edges; edge_iter != NULL; edge_iter = g_list_next (edge_iter))
    {
      edge_ref edge = (edge_ref)edge_iter->data;
      edge_info *info = UNDIR_DATA (edge);

      if (info->is_round)
        {
          fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
                      "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                      "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                      "#%i = AXIS2_PLACEMENT_3D ( 'NONE', #%i,  #%i,  #%i ) ; "
                      "#%i = CIRCLE ( 'NONE', #%i, %f ) ;\n",
                   next_step_identifier,     /* Center of the circle   */ info->cx, info->cy, info->cz, // <--- Center of coordinate placement
                   next_step_identifier + 1, /* Normal of circle?      */ info->nx, info->ny, info->nz, // <--- Z-axis direction of placement             /* XXX: PULL FROM FACE DATA */
//                   next_step_identifier + 1, /* Normal of circle?      */ 0.0, 0.0, -1.0, // <--- Z-axis direction of placement             /* XXX: PULL FROM FACE DATA */
                   next_step_identifier + 2, /* ??????                 */ -1.0, 0.0, 0.0, // <--- Approximate X-axis direction of placement /* XXX: PULL FROM FACE DATA */
                   next_step_identifier + 3, next_step_identifier, next_step_identifier + 1, next_step_identifier + 2,
                   next_step_identifier + 4, next_step_identifier + 3, info->radius);
          info->infinite_line_identifier = next_step_identifier + 4;
          next_step_identifier = next_step_identifier + 5;
        }
      else
        {
          double  x,  y,  z;
          double dx, dy, dz;

          x = ((vertex3d *)ODATA (edge))->x;
          y = ((vertex3d *)ODATA (edge))->y;
          z = ((vertex3d *)ODATA (edge))->z;

          dx = ((vertex3d *)DDATA (edge))->x - x;
          dy = ((vertex3d *)DDATA (edge))->y - y;
          dz = ((vertex3d *)DDATA (edge))->z - z;

          fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
                      "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                      "#%i = VECTOR ( 'NONE', #%i, 1000.0 ) ; "
                      "#%i = LINE ( 'NONE', #%i, #%i ) ;\n",
                   next_step_identifier,     /* A point on the line         */  x,  y,  z,
                   next_step_identifier + 1, /* A direction along the line  */ dx, dy, dz,
                   next_step_identifier + 2, next_step_identifier + 1,
                   next_step_identifier + 3, next_step_identifier, next_step_identifier + 2);
          info->infinite_line_identifier = next_step_identifier + 3;
          next_step_identifier = next_step_identifier + 4;
        }
    }

  /* Define the vertices */
  for (vertex_iter = object->vertices; vertex_iter != NULL; vertex_iter = g_list_next (vertex_iter))
    {
      vertex3d *vertex = vertex_iter->data;

      fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; ", next_step_identifier, vertex->x, vertex->y, vertex->z); /* Vertex coordinate  */
      fprintf (f, "#%i = VERTEX_POINT ( 'NONE', #%i ) ;\n",             next_step_identifier + 1, next_step_identifier);
      vertex->vertex_identifier = next_step_identifier + 1;
      next_step_identifier = next_step_identifier + 2;
    }

  /* Define the Edges */
  for (edge_iter = object->edges; edge_iter != NULL; edge_iter = g_list_next (edge_iter))
    {
      edge_ref edge = (edge_ref)edge_iter->data;
      edge_info *info = UNDIR_DATA (edge);

      int sv = ((vertex3d *)ODATA (edge))->vertex_identifier;
      int ev = ((vertex3d *)DDATA (edge))->vertex_identifier;

      fprintf (f, "#%i = EDGE_CURVE ( 'NONE', #%i, #%i, #%i, .T. ) ; ", next_step_identifier, sv, ev, info->infinite_line_identifier);
      fprintf (f, "#%i = ORIENTED_EDGE ( 'NONE', *, *, #%i, .T. ) ; ",    next_step_identifier + 1, next_step_identifier);
      fprintf (f, "#%i = ORIENTED_EDGE ( 'NONE', *, *, #%i, .F. ) ;\n",   next_step_identifier + 2, next_step_identifier);
      info->edge_identifier = next_step_identifier; /* Add 1 for same oriented, add 2 for back oriented */
      next_step_identifier = next_step_identifier + 3;
    }

  /* Define the faces */
  for (face_iter = object->faces; face_iter != NULL; face_iter = g_list_next (face_iter))
    {
      face3d *face = face_iter->data;
      bool outer_contour = true;

      for (contour_iter = face->contours;
           contour_iter != NULL;
           contour_iter = g_list_next (contour_iter), outer_contour = false)
        {
          contour3d *contour = contour_iter->data;
          edge_ref edge;

          fprintf (f, "#%i = EDGE_LOOP ( 'NONE', ", next_step_identifier);

          /* Emit the edges.. */
          fprintf (f, "(");
          for (edge = contour->first_edge;
               edge != LPREV (contour->first_edge);
               edge = LNEXT (edge))
            {
              fprintf (f, "#%i, ", ORIENTED_EDGE_IDENTIFIER(edge)); /* XXX: IS ORIENTATION GOING TO BE CORRECT?? */
            }
        fprintf (f, "#%i)", ORIENTED_EDGE_IDENTIFIER(edge)); /* XXX: IS ORIENTATION GOING TO BE CORRECT?? */
        fprintf (f, " ) ; ");

        fprintf (f, "#%i = FACE_%sBOUND ( 'NONE', #%i, .T. ) ;\n", next_step_identifier + 1, outer_contour ? "OUTER_" : "", next_step_identifier);
        contour->face_bound_identifier = next_step_identifier + 1;
        next_step_identifier = next_step_identifier + 2;
      }

    fprintf (f, "#%i = ADVANCED_FACE ( 'NONE', ", next_step_identifier);
    fprintf (f, "(");
    for (contour_iter = face->contours;
         contour_iter != NULL && g_list_next (contour_iter) != NULL;
         contour_iter = g_list_next (contour_iter))
      {
        fprintf (f, "#%i, ", ((contour3d *)contour_iter->data)->face_bound_identifier);
      }
    fprintf (f, "#%i)", ((contour3d *)contour_iter->data)->face_bound_identifier);
    fprintf (f, ", #%i, %s ) ;\n", face->surface_identifier, face->surface_orientation_reversed ? ".F." : ".T.");
    face->face_identifier = next_step_identifier;
    next_step_identifier = next_step_identifier + 1;

    if (face->appear != NULL)
      {
        /* Face styles */
        fprintf (f, "#%i = COLOUR_RGB ( '', %f, %f, %f ) ;\n",             next_step_identifier, face->appear->r, face->appear->g, face->appear->b);
        fprintf (f, "#%i = FILL_AREA_STYLE_COLOUR ( '', #%i ) ;\n",        next_step_identifier + 1, next_step_identifier);
        fprintf (f, "#%i = FILL_AREA_STYLE ('', ( #%i ) ) ;\n",            next_step_identifier + 2, next_step_identifier + 1);
        fprintf (f, "#%i = SURFACE_STYLE_FILL_AREA ( #%i ) ;\n",           next_step_identifier + 3, next_step_identifier + 2);
        fprintf (f, "#%i = SURFACE_SIDE_STYLE ('', ( #%i ) ) ;\n",         next_step_identifier + 4, next_step_identifier + 3);
        fprintf (f, "#%i = SURFACE_STYLE_USAGE ( .BOTH. , #%i ) ;\n",      next_step_identifier + 5, next_step_identifier + 4);
        fprintf (f, "#%i = PRESENTATION_STYLE_ASSIGNMENT ( ( #%i ) ) ;\n", next_step_identifier + 6, next_step_identifier + 5);
        fprintf (f, "#%i = OVER_RIDING_STYLED_ITEM ( 'NONE', ( #%i ), #%i, #%i ) ;\n",
                 next_step_identifier + 7, next_step_identifier + 6, face->face_identifier, brep_style_identifier);
        styled_item_identifiers = g_list_append (styled_item_identifiers, GINT_TO_POINTER (next_step_identifier + 7));
        next_step_identifier = next_step_identifier + 8;
      }
  }

  /* Closed shell which bounds the brep solid */
  pcb_shell_identifier = next_step_identifier;
  next_step_identifier++;
  fprintf (f, "#%i = CLOSED_SHELL ( 'NONE', ", pcb_shell_identifier);
  /* Emit the faces.. */
  fprintf (f, "(");
  for (face_iter = object->faces;
       face_iter != NULL && g_list_next (face_iter) != NULL;
       face_iter = g_list_next (face_iter))
    {
      fprintf (f, "#%i, ", ((face3d *)face_iter->data)->face_identifier);
    }
  fprintf (f, "#%i)", ((face3d *)face_iter->data)->face_identifier);
  fprintf (f, " ) ;\n");

  /* Finally emit the brep solid definition */
  fprintf (f, "#%i = MANIFOLD_SOLID_BREP ( 'PCB outline', #%i ) ;\n", brep_identifier, pcb_shell_identifier);

  /* Emit references to the styled and over_ridden styled items */
  fprintf (f, "#%i = MECHANICAL_DESIGN_GEOMETRIC_PRESENTATION_REPRESENTATION (  '', ", next_step_identifier);
  fprintf (f, "(");
  for (styled_item_iter = styled_item_identifiers;
       styled_item_iter != NULL && g_list_next (styled_item_iter) != NULL;
       styled_item_iter = g_list_next (styled_item_iter))
    {
      fprintf (f, "#%i, ", GPOINTER_TO_INT (styled_item_iter->data));
    }
  fprintf (f, "#%i)", GPOINTER_TO_INT (styled_item_iter->data));
  fprintf (f, ", #%i ) ;\n", geometric_representation_context_identifier);
  next_step_identifier = next_step_identifier + 1;

  fprintf (f, "#%i = ADVANCED_BREP_SHAPE_REPRESENTATION ( '%s', ( #%i, #13 ), #%i ) ;\n",
           shape_representation_identifier, "test_pcb_absr_name", brep_identifier, geometric_representation_context_identifier);

#undef ORIENTED_EDGE_IDENTIFIER
#undef FWD
#undef REV

  fprintf (f, "ENDSEC;\n" );
  fprintf (f, "END-ISO-10303-21;\n" );

  fclose (f);
}
