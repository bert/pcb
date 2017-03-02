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


void
object3d_export_to_step (object3d *object, const char *filename)
{
  FILE *f;
  time_t currenttime;
  struct tm utc;
  //int next_step_identifier;

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

  //next_step_identifier = 21;

  /* TODO.. EXPORT FROM A QUAD DATA-STRUCTURE */
#if 0
#define FWD 1
#define REV 2
static void
quad_emit_board_contour_step (FILE *f, PLINE *contour)
{
  int ncontours;
  int npoints;

  int brep_identifier;

  int bottom_plane_identifier;
  int top_plane_identifier;
  int *side_plane_identifier;

  int *bottom_infinite_line_identifier;
  int *top_infinite_line_identifier;
  int *side_infinite_line_identifier;

  int *bottom_vertex_identifier;
  int *top_vertex_identifier;

  int *bottom_edge_identifier;
  int *top_edge_identifier;
  int *side_edge_identifier;

  int *bottom_face_bound_identifier;
  int *top_face_bound_identifier;

  int bottom_face_identifier;
  int top_face_identifier;
  int *side_face_identifier;

  int pcb_shell_identifier;

  int i;

  PLINE *ct;

  ncontours = 0;
  npoints = 0;
  ct = contour;
  while (ct != NULL)
    {
      ncontours ++;
      npoints += get_contour_npoints (ct);
      ct = ct->next;
    }

  /* TODO: Avoid needing to store these identifiers by nailing down our usage pattern of identifiers */
  /* Allocate some storage for identifiers */

            side_plane_identifier = malloc (sizeof (int) * npoints);
  bottom_infinite_line_identifier = malloc (sizeof (int) * npoints);
     top_infinite_line_identifier = malloc (sizeof (int) * npoints);
    side_infinite_line_identifier = malloc (sizeof (int) * npoints);
         bottom_vertex_identifier = malloc (sizeof (int) * npoints);
            top_vertex_identifier = malloc (sizeof (int) * npoints);
           bottom_edge_identifier = malloc (sizeof (int) * npoints);
              top_edge_identifier = malloc (sizeof (int) * npoints);
             side_edge_identifier = malloc (sizeof (int) * npoints);
             side_face_identifier = malloc (sizeof (int) * npoints);

     bottom_face_bound_identifier = malloc (sizeof (int) * ncontours);
        top_face_bound_identifier = malloc (sizeof (int) * ncontours);

  /* For a n-sided outline, we need: */

  // PLANES:               2 + n
  // 2 bottom + top planes
  // n side planes

  // INFINITE LINES:       3n
  // n for the bottom (in the bottom plane)
  // n for the top (in the top plane)
  // n for the sides (joining the top + bottom vertex of the extruded shape (n sided outline = n vertices)

  // VERTICES:             2n
  // n for the bottom (in the bottom plane)
  // n for the top (in the top plane)

  // EDGES:                3n          (6n oriented edges)
  // n for the bottom
  // n for the top
  // n for the sides

  // FACES:                2 + n
  // 2 bottom + top faces
  // n side faces

  // A consistent numbering scheme will avoid needing complex data-structures here!

  /* Save a place for the brep identifier */
  brep_identifier = next_step_identifier++;

  /* Define the bottom and top planes */
  fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
              "#%i = DIRECTION ( 'NONE', (  0.0,  0.0,  1.0 ) ) ; "
              "#%i = DIRECTION ( 'NONE', (  1.0,  0.0,  0.0 ) ) ; "
              "#%i = AXIS2_PLACEMENT_3D ( 'NONE', #%i, #%i, #%i ) ; "
              "#%i = PLANE ( 'NONE',  #%i ) ;\n",
           next_step_identifier, 0.0, 0.0, -COORD_TO_MM (HACK_BOARD_THICKNESS) / 2.0,
           next_step_identifier + 1,
           next_step_identifier + 2,
           next_step_identifier + 3, next_step_identifier, next_step_identifier + 1, next_step_identifier + 2,
           next_step_identifier + 4, next_step_identifier + 3);
  bottom_plane_identifier = next_step_identifier + 4;
  next_step_identifier = next_step_identifier + 5;

  fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
              "#%i = DIRECTION ( 'NONE', (  0.0,  0.0, -1.0 ) ) ; "
              "#%i = DIRECTION ( 'NONE', ( -1.0,  0.0,  0.0 ) ) ; "
              "#%i = AXIS2_PLACEMENT_3D ( 'NONE', #%i, #%i, #%i ) ; "
              "#%i = PLANE ( 'NONE',  #%i ) ;\n",
           next_step_identifier, 0.0, 0.0, COORD_TO_MM (HACK_BOARD_THICKNESS) / 2.0,
           next_step_identifier + 1,
           next_step_identifier + 2,
           next_step_identifier + 3, next_step_identifier, next_step_identifier + 1, next_step_identifier + 2,
           next_step_identifier + 4, next_step_identifier + 3);
  top_plane_identifier = next_step_identifier + 4;
  next_step_identifier = next_step_identifier + 5;

  /* Define the side planes */
  for (i = 0; i < npoints; i++)
    {
      double x1, y1, x2, y2;

      /* Walk through the contours until we find the right one to look at */
      PLINE *ct = contour;
      int adjusted_i = i;

      while (adjusted_i >= get_contour_npoints (ct))
        {
          adjusted_i -= get_contour_npoints (ct);
          ct = ct->next;
        }

      if (ct->is_round)
        {
          /* HACK SPECIAL CASE FOR ROUND CONTOURS (Surface edges bounded by a cylindrical surface, not n-planes) */

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

          side_plane_identifier[i] = next_step_identifier + 4;
          next_step_identifier = next_step_identifier + 5;
        }
      else
        {
          get_contour_coord_n_in_mm (ct, adjusted_i,     &x1, &y1);
          get_contour_coord_n_in_mm (ct, adjusted_i + 1, &x2, &y2);

          fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
                      "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                      "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                      "#%i = AXIS2_PLACEMENT_3D ( 'NONE', #%i, #%i, #%i ) ; "
                      "#%i = PLANE ( 'NONE',  #%i ) ;\n",
                   next_step_identifier,     /* A point on the plane                      */ x1, y1, 0.0,
                   next_step_identifier + 1, /* An axis direction pointing into the shape */ -(y2 - y1), (x2 - x1), 0.0, // <--- NOT SURE IF I NEED TO NORMALISE THIS, OR FLIP THE DIRECTION??
                   next_step_identifier + 2, /* A reference direction pointing.. "meh"?   */ 0.0, 0.0, 1.0,
                   next_step_identifier + 3, next_step_identifier, next_step_identifier + 1, next_step_identifier + 2,
                   next_step_identifier + 4, next_step_identifier + 3);
          side_plane_identifier[i] = next_step_identifier + 4;
          next_step_identifier = next_step_identifier + 5;
        }
    }

    /* Define the infinite lines */
    for (i = 0; i < npoints; i++)
      {
        double x1, y1, x2, y2;

        /* Walk through the contours until we find the right one to look at */
        PLINE *ct = contour;
        int adjusted_i = i;

        while (adjusted_i >= get_contour_npoints (ct))
          {
            adjusted_i -= get_contour_npoints (ct);
            ct = ct->next;
          }

        get_contour_coord_n_in_mm (ct, adjusted_i,     &x1, &y1);
        get_contour_coord_n_in_mm (ct, adjusted_i + 1, &x2, &y2);

        if (ct->is_round)
          {
            /* HACK SPECIAL CASE FOR ROUND CONTOURS (Top and bottom faces bounded a circular contour, not n-lines) */

            /* Bottom */
            fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
                        "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                        "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                        "#%i = AXIS2_PLACEMENT_3D ( 'NONE', #%i,  #%i,  #%i ) ;"
                        "#%i = CIRCLE ( 'NONE', #%i, %f ) ;\n",
                     next_step_identifier,     /* Center of the circle   */ COORD_TO_MM (ct->cx), COORD_TO_MM (ct->cy), -COORD_TO_MM (HACK_BOARD_THICKNESS) / 2.0,
                     next_step_identifier + 1, /* Normal of circle?      */ 0.0, 0.0, -1.0, // <--- NOT SURE IF I NEED TO FLIP THE DIRECTION??
                     next_step_identifier + 2, /* ??????                 */ -1.0, 0.0, 0.0, // NOT SURE WHAT THIS IS!
                     next_step_identifier + 3, next_step_identifier, next_step_identifier + 1, next_step_identifier + 2,
                     next_step_identifier + 4, next_step_identifier + 3, COORD_TO_MM (ct->radius));
            bottom_infinite_line_identifier[i] = next_step_identifier + 4;
            next_step_identifier = next_step_identifier + 5;

            /* Top */
            fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
                        "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                        "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                        "#%i = AXIS2_PLACEMENT_3D ( 'NONE', #%i,  #%i,  #%i ) ;"
                        "#%i = CIRCLE ( 'NONE', #%i, %f ) ;\n",
                     next_step_identifier,     /* Center of the circle   */ COORD_TO_MM (ct->cx), COORD_TO_MM (ct->cy), COORD_TO_MM (HACK_BOARD_THICKNESS) / 2.0,
                     next_step_identifier + 1, /* Normal of circle?      */ 0.0, 0.0, -1.0, // <--- NOT SURE IF I NEED TO FLIP THE DIRECTION??
                     next_step_identifier + 2, /* ??????                 */ -1.0, 0.0, 0.0, // NOT SURE WHAT THIS IS!
                     next_step_identifier + 3, next_step_identifier, next_step_identifier + 1, next_step_identifier + 2,
                     next_step_identifier + 4, next_step_identifier + 3, COORD_TO_MM (ct->radius));
            top_infinite_line_identifier[i] = next_step_identifier + 4;
            next_step_identifier = next_step_identifier + 5;
          }
        else
          {
            /* Bottom */
            fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
                        "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                        "#%i = VECTOR ( 'NONE', #%i, 1000.0 ) ; "
                        "#%i = LINE ( 'NONE', #%i, #%i ) ;\n",
                     next_step_identifier,     /* A point on the line         */ x1, y1, -COORD_TO_MM (HACK_BOARD_THICKNESS) / 2.0,
                     next_step_identifier + 1, /* A direction along the line  */ (x2 - x1), (y2 - y1), 0.0, // <--- NOT SURE IF I NEED TO NORMALISE THIS, OR FLIP THE DIRECTION??
                     next_step_identifier + 2, next_step_identifier + 1,
                     next_step_identifier + 3, next_step_identifier, next_step_identifier + 2);
            bottom_infinite_line_identifier[i] = next_step_identifier + 3;
            next_step_identifier = next_step_identifier + 4;

            /* Top */
            fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
                        "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                        "#%i = VECTOR ( 'NONE', #%i, 1000.0 ) ; "
                        "#%i = LINE ( 'NONE', #%i, #%i ) ;\n",
                     next_step_identifier,     /* A point on the line         */ x1, y1, COORD_TO_MM (HACK_BOARD_THICKNESS) / 2.0,
                     next_step_identifier + 1, /* A direction along the line  */ (x2 - x1), (y2 - y1), 0.0, // <--- NOT SURE IF I NEED TO NORMALISE THIS, OR FLIP THE DIRECTION??
                     next_step_identifier + 2, next_step_identifier + 1,
                     next_step_identifier + 3, next_step_identifier, next_step_identifier + 2);
            top_infinite_line_identifier[i] = next_step_identifier + 3;
            next_step_identifier = next_step_identifier + 4;
          }

        /* Side */
        fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
                    "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                    "#%i = VECTOR ( 'NONE', #%i, 1000.0 ) ; "
                    "#%i = LINE ( 'NONE', #%i, #%i ) ;\n",
                 next_step_identifier,     /* A point on the line         */ x1, y1, -COORD_TO_MM (HACK_BOARD_THICKNESS) / 2.0,
                 next_step_identifier + 1, /* A direction along the line  */ 0.0, 0.0, 1.0, // <--- NOT SURE IF I NEED TO NORMALISE THIS, OR FLIP THE DIRECTION??
                 next_step_identifier + 2, next_step_identifier + 1,
                 next_step_identifier + 3, next_step_identifier, next_step_identifier + 2);
        side_infinite_line_identifier[i] = next_step_identifier + 3;
        next_step_identifier = next_step_identifier + 4;
      }

    /* Define the vertices */
    for (i = 0; i < npoints; i++)
      {
        double x1, y1;

        /* Walk through the contours until we find the right one to look at */
        PLINE *ct = contour;
        int adjusted_i = i;

        while (adjusted_i >= get_contour_npoints (ct))
          {
            adjusted_i -= get_contour_npoints (ct);
            ct = ct->next;
          }

        get_contour_coord_n_in_mm (ct, adjusted_i, &x1, &y1);

        /* Bottom */
        fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
                    "#%i = VERTEX_POINT ( 'NONE', #%i ) ;\n",
                 next_step_identifier,     /* Vertex coordinate  */ x1, y1, -COORD_TO_MM (HACK_BOARD_THICKNESS) / 2.0,
                 next_step_identifier + 1, next_step_identifier);
        bottom_vertex_identifier[i] = next_step_identifier + 1;
        next_step_identifier = next_step_identifier + 2;

        /* Top */
        fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
                    "#%i = VERTEX_POINT ( 'NONE', #%i ) ;\n",
                 next_step_identifier,     /* Vertex coordinate  */ x1, y1, COORD_TO_MM (HACK_BOARD_THICKNESS) / 2.0,
                 next_step_identifier + 1, next_step_identifier);
        top_vertex_identifier[i] = next_step_identifier + 1;
        next_step_identifier = next_step_identifier + 2;
      }

    /* Define the Edges */
    for (i = 0; i < npoints; i++)
      {

        /* Walk through the contours until we find the right one to look at */
        PLINE *ct = contour;
        int adjusted_i = i;
        int i_start = 0;

        while (adjusted_i >= get_contour_npoints (ct))
          {
            adjusted_i -= get_contour_npoints (ct);
            i_start += get_contour_npoints (ct);
            ct = ct->next;
          }

        /* Due to the way the index wrapping works, this works for circular cutouts as well as n-sided */

        /* Bottom */
        fprintf (f, "#%i = EDGE_CURVE ( 'NONE', #%i, #%i, #%i,   .T. ) ; "
                    "#%i = ORIENTED_EDGE ( 'NONE', *, *, #%i, .F. ) ; "
                    "#%i = ORIENTED_EDGE ( 'NONE', *, *, #%i, .T. ) ;\n",
                 next_step_identifier, bottom_vertex_identifier[i], bottom_vertex_identifier[i_start + (adjusted_i + 1) % get_contour_npoints (ct)], bottom_infinite_line_identifier[i],      // <-- MIGHT NEED TO REVERSE THIS???
                 next_step_identifier + 1, next_step_identifier,
                 next_step_identifier + 2, next_step_identifier);
        bottom_edge_identifier[i] = next_step_identifier; /* Add 1 for same oriented, add 2 for back oriented */
        next_step_identifier = next_step_identifier + 3;

        /* Top */
        fprintf (f, "#%i = EDGE_CURVE ( 'NONE', #%i, #%i, #%i,   .T. ) ; "
                    "#%i = ORIENTED_EDGE ( 'NONE', *, *, #%i, .F. ) ; "
                    "#%i = ORIENTED_EDGE ( 'NONE', *, *, #%i, .T. ) ;\n",
                 next_step_identifier, top_vertex_identifier[i], top_vertex_identifier[i_start + (adjusted_i + 1) % get_contour_npoints (ct)], top_infinite_line_identifier[i],                 // <-- MIGHT NEED TO REVERSE THIS???
                 next_step_identifier + 1, next_step_identifier,
                 next_step_identifier + 2, next_step_identifier);
        top_edge_identifier[i] = next_step_identifier; /* Add 1 for same oriented, add 2 for back oriented */
        next_step_identifier = next_step_identifier + 3;

        /* Side */
        fprintf (f, "#%i = EDGE_CURVE ( 'NONE', #%i, #%i, #%i,   .T. ) ; "
                    "#%i = ORIENTED_EDGE ( 'NONE', *, *, #%i, .F. ) ; "
                    "#%i = ORIENTED_EDGE ( 'NONE', *, *, #%i, .T. ) ;\n",
                 next_step_identifier, bottom_vertex_identifier[i], top_vertex_identifier[i], side_infinite_line_identifier[i],
                 next_step_identifier + 1, next_step_identifier,
                 next_step_identifier + 2, next_step_identifier);
        side_edge_identifier[i] = next_step_identifier; /* Add 1 for same oriented, add 2 for back oriented */
        next_step_identifier = next_step_identifier + 3;
      }

    /* Define the faces */

    /* Bottom */
    {
      PLINE *ct = contour;
      int icont;
      int start_i;

      start_i = 0;
      for (icont = 0; icont < ncontours; icont++, start_i += get_contour_npoints (ct), ct = ct->next)
        {

          fprintf (f, "#%i = EDGE_LOOP ( 'NONE', ( ",
                   next_step_identifier);
          for (i = start_i + get_contour_npoints (ct) - 1; i > start_i; i--)
            fprintf (f, "#%i, ", bottom_edge_identifier[i] + FWD);
          fprintf (f, "#%i ) ) ; "
                      "#%i = FACE_%sBOUND ( 'NONE', #%i, .T. ) ; \n",
                   bottom_edge_identifier[start_i] + FWD,
                   next_step_identifier + 1, icont > 0 ? "" : "OUTER_", next_step_identifier);
          bottom_face_bound_identifier[icont] = next_step_identifier + 1;
          next_step_identifier = next_step_identifier + 2;
        }

      fprintf (f, "#%i = ADVANCED_FACE ( 'NONE', ( ",
               next_step_identifier);
      for (icont = 0; icont < ncontours - 1; icont++)
        fprintf (f, "#%i, ",
                 bottom_face_bound_identifier[icont]);
      fprintf (f, "#%i ), #%i, .F. ) ;\n",
               bottom_face_bound_identifier[ncontours - 1], bottom_plane_identifier);
      bottom_face_identifier = next_step_identifier;
      next_step_identifier = next_step_identifier + 1;
    }

    /* Top */
    {
      PLINE *ct = contour;
      int icont;
      int start_i;

      start_i = 0;
      for (icont = 0; icont < ncontours; icont++, start_i += get_contour_npoints (ct), ct = ct->next)
        {
          fprintf (f, "#%i = EDGE_LOOP ( 'NONE', ( ",
                   next_step_identifier);
          for (i = start_i; i < start_i + get_contour_npoints (ct) - 1; i++)
            fprintf (f, "#%i, ", top_edge_identifier[i] + REV);
          fprintf (f, "#%i ) ) ; "
                      "#%i = FACE_%sBOUND ( 'NONE', #%i, .T. ) ; \n",
                   top_edge_identifier[start_i + get_contour_npoints (ct) - 1] + REV,
                   next_step_identifier + 1, icont > 0 ? "" : "OUTER_", next_step_identifier);
          top_face_bound_identifier[icont] = next_step_identifier + 1;
          next_step_identifier = next_step_identifier + 2;
        }

      fprintf (f, "#%i = ADVANCED_FACE ( 'NONE', ( ",
               next_step_identifier);
      for (icont = 0; icont < ncontours - 1; icont++)
        fprintf (f, "#%i, ",
                 top_face_bound_identifier[icont]);
      fprintf (f, "#%i ), #%i, .F. ) ;\n",
               top_face_bound_identifier[ncontours - 1], top_plane_identifier);
      top_face_identifier = next_step_identifier;
      next_step_identifier = next_step_identifier + 1;
    }

    /* Sides */
    for (i = 0; i < npoints; i++)
      {

        /* Walk through the contours until we find the right one to look at */
        PLINE *ct = contour;
        int adjusted_i = i;
        int i_start = 0;

        while (adjusted_i >= get_contour_npoints (ct))
          {
            adjusted_i -= get_contour_npoints (ct);
            i_start += get_contour_npoints (ct);
            ct = ct->next;
          }

        fprintf (f, "#%i = EDGE_LOOP ( 'NONE', ( #%i, #%i, #%i, #%i ) ) ; "
                    "#%i = FACE_OUTER_BOUND ( 'NONE', #%i, .T. ) ; "
                    "#%i = ADVANCED_FACE ( 'NONE', ( #%i ), #%i, .F. ) ;\n",
                 next_step_identifier, side_edge_identifier[i_start + (adjusted_i + 1) % get_contour_npoints (ct)] + REV, top_edge_identifier[i] + FWD, side_edge_identifier[i] + FWD, bottom_edge_identifier[i] + REV,
                 next_step_identifier + 1, next_step_identifier,
                 next_step_identifier + 2, next_step_identifier + 1, side_plane_identifier[i]);
        side_face_identifier[i] = next_step_identifier + 2;
        next_step_identifier = next_step_identifier + 3;
      }

    /* Closed shell which bounds the brep solid */
    pcb_shell_identifier = next_step_identifier;
    next_step_identifier++;
    fprintf (f, "#%i = CLOSED_SHELL ( 'NONE', ( #%i, #%i, ", pcb_shell_identifier, bottom_face_identifier, top_face_identifier);
    for (i = 0; i < npoints - 1; i++)
      {
        fprintf (f, "#%i, ", side_face_identifier[i]);
      }
    fprintf (f, "#%i) ) ;\n",
             side_face_identifier[npoints - 1]);

    /* Finally emit the brep solid definition */
    fprintf (f, "#%i = MANIFOLD_SOLID_BREP ( 'PCB outline', #%i ) ;\n", brep_identifier, pcb_shell_identifier);

    free (side_plane_identifier);
    free (bottom_infinite_line_identifier);
    free (top_infinite_line_identifier);
    free (side_infinite_line_identifier);
    free (bottom_vertex_identifier);
    free (top_vertex_identifier);
    free (bottom_edge_identifier);
    free (top_edge_identifier);
    free (side_edge_identifier);
    free (side_face_identifier);
    free (bottom_face_bound_identifier);
    free (top_face_bound_identifier);
  }
#undef FWD
#undef REV
#endif

  fprintf (f, "ENDSEC;\n" );
  fprintf (f, "END-ISO-10303-21;\n" );

  fclose (f);
}
