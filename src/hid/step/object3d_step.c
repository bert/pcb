#include <stdio.h>
#include <stdbool.h>

#include <glib.h>

#include "hid/common/step_id.h"
#include "hid/common/quad.h"
#include "hid/common/vertex3d.h"
#include "hid/common/contour3d.h"
#include "hid/common/appearance.h"
#include "hid/common/face3d.h"
#include "hid/common/object3d.h"
#include "data.h"

#include "object3d_step.h"


static void
fprint_idlist (FILE *f, int *ids, int num_ids)
{
  int i;
  fprintf (f, "(");
  for (i = 0; i < num_ids - 1; i++)
    fprintf (f, "#%i, ", ids[i]);
  fprintf (f, "#%i) ) ;\n", ids[i]);
}

void
object3d_export_to_step (object3d *object, const char *filename)
{
  FILE *f;
  time_t currenttime;
  struct tm utc;
  int next_step_identifier;
  int brep_identifier;
  int pcb_shell_identifier;

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

  /* BREP STUFF FROM #21 onwards say? */
  fprintf (f, "#19 = ADVANCED_BREP_SHAPE_REPRESENTATION ( '%s', ( /* Manifold_solid_brep */ #21, #13 ), #18 ) ;\n"
              "#20 = SHAPE_DEFINITION_REPRESENTATION ( #9, #19 ) ;\n",
              "test_pcb_absr_name");

  next_step_identifier = 21;

  /* TODO.. EXPORT FROM A QUAD DATA-STRUCTURE */
#if 1
#define FWD 1
#define REV 2

  /* Save a place for the brep identifier */
  brep_identifier = next_step_identifier++;

  /* Define ininite planes corresponding to every planar face, and cylindrical surfaces for every cylindrical face */
  /* XXX: ENUMERATE OVER SPATIAL DATA-STRUCTURE */
  for (;;)
    {
      if (ct->is_round)
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
                   next_step_identifier, /* A point on the axis of the cylinder */ COORD_TO_MM (ct->cx), COORD_TO_MM (ct->cy), 0.0,
                   next_step_identifier + 1, /* Direction of surface axis... not sure if the sign of the direction matters */ 0.0, 0.0, 1.0,
                   next_step_identifier + 2, /* URM???? NOT SURE WHAT THIS DIRECTION IS FOR                                */ 1.0, 0.0, 0.0,
                   next_step_identifier + 3, next_step_identifier, next_step_identifier + 1, next_step_identifier + 2,
                   next_step_identifier + 4, next_step_identifier + 3, COORD_TO_MM (ct->radius));

          plane_identifiers[i] = next_step_identifier + 4;
          next_step_identifier = next_step_identifier + 5;
        }
      else
        {
          /* FOR CONSISTENCY WITH ABOVE, DEFINE PLANE NORMAL TO BE POINTING INSIDE THE SHAPE.
           * THIS ALLOWS TO FLIP THE ORIENTATION OF THE UNDERLYING SURFACE WHEN DEFINING EVERY ADVANCED_FACE
           */
          fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; ", next_step_identifier, x1, y1, 0.0);    // <-- A locating point on the plane. Forms 0,0 of its parameterised coords.
          fprintf (f, "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; ", next_step_identifier + 1,  -(y2 - y1), (x2 - x1), 0.0);  /* An axis direction pointing into the shape */ // <-- Or is this the z-axis of the coordinate placement -> plane normal?
          fprintf (f, "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; ", next_step_identifier + 2, 0.0, 0.0, 1.0);          // <-- Reference x-axis, should be orthogonal to the z-axis above.
          fprintf (f, "#%i = AXIS2_PLACEMENT_3D ( 'NONE', #%i, #%i, #%i ) ; "
                   next_step_identifier + 3, next_step_identifier, next_step_identifier + 1, next_step_identifier + 2);
          fprintf (f, "#%i = PLANE ( 'NONE',  #%i ) ;\n",
                   next_step_identifier + 4, next_step_identifier + 3);
          plane_identifiers[i] = next_step_identifier + 4;
          next_step_identifier = next_step_identifier + 5;
        }
    }

  /* Define the infinite lines corresponding to every edge (either lines or circles)*/
  /* XXX: ENUMERATE OVER SPATIAL DATA-STRUCTURE */
  for (;;)
    {

      if (ct->is_round)
        {
          fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
                      "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                      "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                      "#%i = AXIS2_PLACEMENT_3D ( 'NONE', #%i,  #%i,  #%i ) ;"
                      "#%i = CIRCLE ( 'NONE', #%i, %f ) ;\n",
                   next_step_identifier,     /* Center of the circle   */ edge_info->cx, edge_info->cy, edge_info->cz, // <--- Center of coordinate placement
                   next_step_identifier + 1, /* Normal of circle?      */ 0.0, 0.0, -1.0, // <--- Z-axis direction of placement             /* XXX: PULL FROM FACE DATA */
                   next_step_identifier + 2, /* ??????                 */ -1.0, 0.0, 0.0, // <--- Approximate X-axis direction of placement /* XXX: PULL FROM FACE DATA */
                   next_step_identifier + 3, next_step_identifier, next_step_identifier + 1, next_step_identifier + 2,
                   next_step_identifier + 4, next_step_identifier + 3, edge_info->radius);
          infinite_line_identifiers[i] = next_step_identifier + 4;
          next_step_identifier = next_step_identifier + 5;
        }
      else
        {
          double dx, dy, dz;

          dx = end_v->x - start_v->x;
          dy = end_v->y - start_v->y;
          dz = end_v->z - start_v->z;

          fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
                      "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                      "#%i = VECTOR ( 'NONE', #%i, 1000.0 ) ; "
                      "#%i = LINE ( 'NONE', #%i, #%i ) ;\n",
                   next_step_identifier,     /* A point on the line         */ start_v->x, start_v->y, start_v->z,
                   next_step_identifier + 1, /* A direction along the line  */ dx, dy, dz,
                   next_step_identifier + 2, next_step_identifier + 1,
                   next_step_identifier + 3, next_step_identifier, next_step_identifier + 2);
          infinite_line_identifiers[i] = next_step_identifier + 3;
          next_step_identifier = next_step_identifier + 4;
        }
    }

  /* Define the vertices */
  /* XXX: ENUMERATE OVER SPATIAL DATA-STRUCTURE */
  for (;;)
    {
      fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; ", next_step_identifier,     /* Vertex coordinate  */ x, y, z);
      fprintf (f, "#%i = VERTEX_POINT ( 'NONE', #%i ) ;\n",             next_step_identifier + 1, next_step_identifier);
      vertex_identifiers[i] = next_step_identifier + 1;
      next_step_identifier = next_step_identifier + 2;
    }

  /* Define the Edges */
  /* XXX: ENUMERATE OVER SPATIAL DATA-STRUCTURE */
  for (;;)
    {
      fprintf (f, "#%i = EDGE_CURVE ( 'NONE', #%i, #%i, #%i,   .T. ) ; ", next_step_identifier, start_vertex_identifiers[i], end_vertex_identifiers[i], infinite_line_identifier[i]);
      fprintf (f, "#%i = ORIENTED_EDGE ( 'NONE', *, *, #%i, .F. ) ; ",    next_step_identifier + 1, next_step_identifier);
      fprintf (f, "#%i = ORIENTED_EDGE ( 'NONE', *, *, #%i, .T. ) ;\n",   next_step_identifier + 2, next_step_identifier);
      edge_identifiers[i] = next_step_identifier; /* Add 1 for same oriented, add 2 for back oriented */
      next_step_identifier = next_step_identifier + 3;
    }

  /* Define the faces */
  /* XXX: ENUMERATE OVER SPATIAL DATA-STRUCTURE (ESPECIALLY FOR CORRECT ORDERING!)*/
  for (;;)
    {
      start_i = 0;
      for (icont = 0; icont < ncontours; icont++, start_i += get_contour_npoints (ct), ct = ct->next)
        {

          /* XXX: FWD / BWD NEEDS TO BE FUDGED IN HERE PERHAPS? */
          fprintf (f, "#%i = EDGE_LOOP ( 'NONE', ", next_step_identifier); fprint_idlist (f, face_edge_identifiers[i], face_contour_npoints[i]); fprintf (f, " ) ; ");
          fprintf (f, "#%i = FACE_%sBOUND ( 'NONE', #%i, .T. ) ; \n", next_step_identifier + 1, icont > 0 ? "" : "OUTER_", next_step_identifier);
          face_bound_identifiers[icont] = next_step_identifier + 1;
          next_step_identifier = next_step_identifier + 2;
        }

      fprintf (f, "#%i = ADVANCED_FACE ( 'NONE', ", next_step_identifier); fprint_idlist (f, face_bound_identifiers, ncontours);  fprintf (f, ", #%i, .F. ) ;\n", plane_identifiers[i]);
      face_identifiers[i] = next_step_identifier;
      next_step_identifier = next_step_identifier + 1;
    }

  /* Closed shell which bounds the brep solid */
  pcb_shell_identifier = next_step_identifier;
  next_step_identifier++;
  fprintf (f, "#%i = CLOSED_SHELL ( 'NONE', ", pcb_shell_identifier); fprint_idlist (f, face_identifiers, nfaces); fprintf (f, " ) ;\n");

  /* Finally emit the brep solid definition */
  fprintf (f, "#%i = MANIFOLD_SOLID_BREP ( 'PCB outline', #%i ) ;\n", brep_identifier, pcb_shell_identifier);

#undef FWD
#undef REV
#endif

  fprintf (f, "ENDSEC;\n" );
  fprintf (f, "END-ISO-10303-21;\n" );

  fclose (f);
}
