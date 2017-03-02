#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
//#include <stdarg.h> /* not used */
//#include <stdlib.h>
//#include <string.h>
//#include <assert.h> /* not used */
#include <time.h>

#include "global.h"
#include "data.h"
#include "misc.h"
//#include "error.h"
//#include "draw.h"
#include "pcb-printf.h"

#include "hid.h"
#include "hid_draw.h"
#include "../hidint.h"
#include "hid/common/hidnogui.h"
#include "hid/common/draw_helpers.h"
#include "step.h"
#include "hid/common/hidinit.h"
#include "polygon.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented STEP function %s.\n", __FUNCTION__); abort()

#define HACK_BOARD_THICKNESS MM_TO_COORD(1.6)

static int step_set_layer (const char *name, int group, int empty);
static void use_gc (hidGC gc);

typedef struct step_gc_struct
{
  struct hid_gc_struct hid_gc; /* Parent */

  EndCapStyle cap;
  Coord width;
  unsigned char r, g, b;
  int erase;
  int faded;
} *stepGC;

HID step_hid;
static HID_DRAW step_graphics;
static HID_DRAW_CLASS step_graphics_class;

HID_Attribute step_attribute_list[] = {
  /* other HIDs expect this to be first.  */

/* %start-doc options "91 STEP Export"
@ftable @code
@item --stepfile <string>
Name of the STEP output file. Can contain a path.
@end ftable
%end-doc
*/
  {"stepfile", "STEP output file",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_stepfile 0
};

#define NUM_OPTIONS (sizeof(step_attribute_list)/sizeof(step_attribute_list[0]))

REGISTER_ATTRIBUTES (step_attribute_list)

/* All file-scope data is in global struct */
static struct {

  FILE *f;
  bool print_group[MAX_LAYER];
  bool print_layer[MAX_LAYER];

  const char *filename;

  LayerType *outline_layer;

  HID_Attr_Val step_values[NUM_OPTIONS];

  bool is_mask;
  bool is_drill;
  bool is_assy;
  bool is_copper;
  bool is_paste;

  int next_identifier;
} global;

static HID_Attribute *
step_get_export_options (int *n)
{
  static char *last_made_filename = 0;
  if (PCB)
    derive_default_filename(PCB->Filename, &step_attribute_list[HA_stepfile], ".step", &last_made_filename);

  if (n)
    *n = NUM_OPTIONS;
  return step_attribute_list;
}

static int
group_for_layer (int l)
{
  if (l < max_copper_layer + 2 && l >= 0)
    return GetLayerGroupNumberByNumber (l);
  /* else something unique */
  return max_group + 3 + l;
}

static int
layer_sort (const void *va, const void *vb)
{
  int a = *(int *) va;
  int b = *(int *) vb;
  int d = group_for_layer (b) - group_for_layer (a);
  if (d)
    return d;
  return b - a;
}

void
step_start_file (FILE *f, const char *filename)
{
  time_t currenttime = time (NULL);
  struct tm utc;

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
  fprintf (f, "#10 =    CARTESIAN_POINT ( 'NONE',  ( 0.0, 0.0, 0.0 ) ) ;\n"
              "#11 =          DIRECTION ( 'NONE',  ( 0.0, 0.0, 1.0 ) ) ;\n"
              "#12 =          DIRECTION ( 'NONE',  ( 1.0, 0.0, 0.0 ) ) ;\n"
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

  global.next_identifier = 21;
}

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

  while (n > 0) {
    vertex = vertex->next; /* The VNODE structure is circularly linked, so wrapping is OK */
    n--;
  }

  *x = COORD_TO_MM (vertex->point[0]);
  *y = COORD_TO_MM (vertex->point[1]); /* FIXME: PCB's coordinate system has y increasing downwards */
}

#define FWD 1
#define REV 2
static void
step_emit_board_contour (FILE *f, PLINE *contour)
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
  while (ct != NULL) {
    ncontours ++;
    npoints += get_contour_npoints (ct);
    ct = ct->next;
  }

  /* TODO: Avoid needing to store these identifiers by nailing down our usage pattern of identifiers */
  /* Allocate some storage for identifiers */

            side_plane_identifier = g_malloc (sizeof (int) * npoints);
  bottom_infinite_line_identifier = g_malloc (sizeof (int) * npoints);
     top_infinite_line_identifier = g_malloc (sizeof (int) * npoints);
    side_infinite_line_identifier = g_malloc (sizeof (int) * npoints);
         bottom_vertex_identifier = g_malloc (sizeof (int) * npoints);
            top_vertex_identifier = g_malloc (sizeof (int) * npoints);
           bottom_edge_identifier = g_malloc (sizeof (int) * npoints);
              top_edge_identifier = g_malloc (sizeof (int) * npoints);
             side_edge_identifier = g_malloc (sizeof (int) * npoints);
             side_face_identifier = g_malloc (sizeof (int) * npoints);

     bottom_face_bound_identifier = g_malloc (sizeof (int) * ncontours);
        top_face_bound_identifier = g_malloc (sizeof (int) * ncontours);

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
  brep_identifier = global.next_identifier++;

  /* Define the bottom and top planes */
  fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
              "#%i = DIRECTION ( 'NONE', (  0.0,  0.0,  1.0 ) ) ; "
              "#%i = DIRECTION ( 'NONE', (  1.0,  0.0,  0.0 ) ) ; "
              "#%i = AXIS2_PLACEMENT_3D ( 'NONE', #%i, #%i, #%i ) ; "
              "#%i = PLANE ( 'NONE',  #%i ) ;\n",
           global.next_identifier, 0.0, 0.0, -COORD_TO_MM (HACK_BOARD_THICKNESS) / 2.0,
           global.next_identifier + 1,
           global.next_identifier + 2,
           global.next_identifier + 3, global.next_identifier, global.next_identifier + 1, global.next_identifier + 2,
           global.next_identifier + 4, global.next_identifier + 3);
  bottom_plane_identifier = global.next_identifier + 4;
  global.next_identifier = global.next_identifier + 5;

  fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
              "#%i = DIRECTION ( 'NONE', (  0.0,  0.0, -1.0 ) ) ; "
              "#%i = DIRECTION ( 'NONE', ( -1.0,  0.0,  0.0 ) ) ; "
              "#%i = AXIS2_PLACEMENT_3D ( 'NONE', #%i, #%i, #%i ) ; "
              "#%i = PLANE ( 'NONE',  #%i ) ;\n",
           global.next_identifier, 0.0, 0.0, COORD_TO_MM (HACK_BOARD_THICKNESS) / 2.0,
           global.next_identifier + 1,
           global.next_identifier + 2,
           global.next_identifier + 3, global.next_identifier, global.next_identifier + 1, global.next_identifier + 2,
           global.next_identifier + 4, global.next_identifier + 3);
  top_plane_identifier = global.next_identifier + 4;
  global.next_identifier = global.next_identifier + 5;

  /* Define the side planes */
  for (i = 0; i < npoints; i++) {
    double x1, y1, x2, y2;

    /* Walk through the contours until we find the right one to look at */
    PLINE *ct = contour;
    int adjusted_i = i;

    while (adjusted_i >= get_contour_npoints (ct)) {
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
                 global.next_identifier, /* A point on the axis of the cylinder */ COORD_TO_MM (ct->cx), COORD_TO_MM (ct->cy), 0.0,
                 global.next_identifier + 1, /* Direction of surface axis... not sure if the sign of the direction matters */ 0.0, 0.0, 1.0,
                 global.next_identifier + 2, /* URM???? NOT SURE WHAT THIS DIRECTION IS FOR                                */ 1.0, 0.0, 0.0,
                 global.next_identifier + 3, global.next_identifier, global.next_identifier + 1, global.next_identifier + 2,
                 global.next_identifier + 4, global.next_identifier + 3, COORD_TO_MM (ct->radius));

        side_plane_identifier[i] = global.next_identifier + 4;
        global.next_identifier = global.next_identifier + 5;
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
                 global.next_identifier,     /* A point on the plane                      */ x1, y1, 0.0,
                 global.next_identifier + 1, /* An axis direction pointing into the shape */ -(y2 - y1), (x2 - x1), 0.0, // <--- NOT SURE IF I NEED TO NORMALISE THIS, OR FLIP THE DIRECTION??
                 global.next_identifier + 2, /* A reference direction pointing.. "meh"?   */ 0.0, 0.0, 1.0,
                 global.next_identifier + 3, global.next_identifier, global.next_identifier + 1, global.next_identifier + 2,
                 global.next_identifier + 4, global.next_identifier + 3);
        side_plane_identifier[i] = global.next_identifier + 4;
        global.next_identifier = global.next_identifier + 5;
      }
  }

  /* Define the infinite lines */
  for (i = 0; i < npoints; i++) {
    double x1, y1, x2, y2;

    /* Walk through the contours until we find the right one to look at */
    PLINE *ct = contour;
    int adjusted_i = i;

    while (adjusted_i >= get_contour_npoints (ct)) {
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
                 global.next_identifier,     /* Center of the circle   */ COORD_TO_MM (ct->cx), COORD_TO_MM (ct->cy), -COORD_TO_MM (HACK_BOARD_THICKNESS) / 2.0,
                 global.next_identifier + 1, /* Normal of circle?      */ 0.0, 0.0, -1.0, // <--- NOT SURE IF I NEED TO FLIP THE DIRECTION??
                 global.next_identifier + 2, /* ??????                 */ -1.0, 0.0, 0.0, // NOT SURE WHAT THIS IS!
                 global.next_identifier + 3, global.next_identifier, global.next_identifier + 1, global.next_identifier + 2,
                 global.next_identifier + 4, global.next_identifier + 3, COORD_TO_MM (ct->radius));
        bottom_infinite_line_identifier[i] = global.next_identifier + 4;
        global.next_identifier = global.next_identifier + 5;

        /* Top */
        fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
                    "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                    "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                    "#%i = AXIS2_PLACEMENT_3D ( 'NONE', #%i,  #%i,  #%i ) ;"
                    "#%i = CIRCLE ( 'NONE', #%i, %f ) ;\n",
                 global.next_identifier,     /* Center of the circle   */ COORD_TO_MM (ct->cx), COORD_TO_MM (ct->cy), COORD_TO_MM (HACK_BOARD_THICKNESS) / 2.0,
                 global.next_identifier + 1, /* Normal of circle?      */ 0.0, 0.0, -1.0, // <--- NOT SURE IF I NEED TO FLIP THE DIRECTION??
                 global.next_identifier + 2, /* ??????                 */ -1.0, 0.0, 0.0, // NOT SURE WHAT THIS IS!
                 global.next_identifier + 3, global.next_identifier, global.next_identifier + 1, global.next_identifier + 2,
                 global.next_identifier + 4, global.next_identifier + 3, COORD_TO_MM (ct->radius));
        top_infinite_line_identifier[i] = global.next_identifier + 4;
        global.next_identifier = global.next_identifier + 5;
      }
    else
      {
        /* Bottom */
        fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
                    "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                    "#%i = VECTOR ( 'NONE', #%i, 1000.0 ) ; "
                    "#%i = LINE ( 'NONE', #%i, #%i ) ;\n",
                 global.next_identifier,     /* A point on the line         */ x1, y1, -COORD_TO_MM (HACK_BOARD_THICKNESS) / 2.0,
                 global.next_identifier + 1, /* A direction along the line  */ (x2 - x1), (y2 - y1), 0.0, // <--- NOT SURE IF I NEED TO NORMALISE THIS, OR FLIP THE DIRECTION??
                 global.next_identifier + 2, global.next_identifier + 1,
                 global.next_identifier + 3, global.next_identifier, global.next_identifier + 2);
        bottom_infinite_line_identifier[i] = global.next_identifier + 3;
        global.next_identifier = global.next_identifier + 4;

        /* Top */
        fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
                    "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                    "#%i = VECTOR ( 'NONE', #%i, 1000.0 ) ; "
                    "#%i = LINE ( 'NONE', #%i, #%i ) ;\n",
                 global.next_identifier,     /* A point on the line         */ x1, y1, COORD_TO_MM (HACK_BOARD_THICKNESS) / 2.0,
                 global.next_identifier + 1, /* A direction along the line  */ (x2 - x1), (y2 - y1), 0.0, // <--- NOT SURE IF I NEED TO NORMALISE THIS, OR FLIP THE DIRECTION??
                 global.next_identifier + 2, global.next_identifier + 1,
                 global.next_identifier + 3, global.next_identifier, global.next_identifier + 2);
        top_infinite_line_identifier[i] = global.next_identifier + 3;
        global.next_identifier = global.next_identifier + 4;
      }

    /* Side */
    fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
                "#%i =       DIRECTION ( 'NONE', ( %f, %f, %f )) ; "
                "#%i = VECTOR ( 'NONE', #%i, 1000.0 ) ; "
                "#%i = LINE ( 'NONE', #%i, #%i ) ;\n",
             global.next_identifier,     /* A point on the line         */ x1, y1, -COORD_TO_MM (HACK_BOARD_THICKNESS) / 2.0,
             global.next_identifier + 1, /* A direction along the line  */ 0.0, 0.0, 1.0, // <--- NOT SURE IF I NEED TO NORMALISE THIS, OR FLIP THE DIRECTION??
             global.next_identifier + 2, global.next_identifier + 1,
             global.next_identifier + 3, global.next_identifier, global.next_identifier + 2);
    side_infinite_line_identifier[i] = global.next_identifier + 3;
    global.next_identifier = global.next_identifier + 4;
  }

  /* Define the vertices */
  for (i = 0; i < npoints; i++) {
    double x1, y1;

    /* Walk through the contours until we find the right one to look at */
    PLINE *ct = contour;
    int adjusted_i = i;

    while (adjusted_i >= get_contour_npoints (ct)) {
      adjusted_i -= get_contour_npoints (ct);
      ct = ct->next;
    }

    get_contour_coord_n_in_mm (ct, adjusted_i, &x1, &y1);

    /* Bottom */
    fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
                "#%i = VERTEX_POINT ( 'NONE', #%i ) ;\n",
             global.next_identifier,     /* Vertex coordinate  */ x1, y1, -COORD_TO_MM (HACK_BOARD_THICKNESS) / 2.0,
             global.next_identifier + 1, global.next_identifier);
    bottom_vertex_identifier[i] = global.next_identifier + 1;
    global.next_identifier = global.next_identifier + 2;

    /* Top */
    fprintf (f, "#%i = CARTESIAN_POINT ( 'NONE', ( %f, %f, %f )) ; "
                "#%i = VERTEX_POINT ( 'NONE', #%i ) ;\n",
             global.next_identifier,     /* Vertex coordinate  */ x1, y1, COORD_TO_MM (HACK_BOARD_THICKNESS) / 2.0,
             global.next_identifier + 1, global.next_identifier);
    top_vertex_identifier[i] = global.next_identifier + 1;
    global.next_identifier = global.next_identifier + 2;
  }

  /* Define the Edges */
  for (i = 0; i < npoints; i++) {

    /* Walk through the contours until we find the right one to look at */
    PLINE *ct = contour;
    int adjusted_i = i;
    int i_start = 0;

    while (adjusted_i >= get_contour_npoints (ct)) {
      adjusted_i -= get_contour_npoints (ct);
      i_start += get_contour_npoints (ct);
      ct = ct->next;
    }

    /* Due to the way the index wrapping works, this works for circular cutouts as well as n-sided */

    /* Bottom */
    fprintf (f, "#%i = EDGE_CURVE ( 'NONE', #%i, #%i, #%i,   .T. ) ; "
                "#%i = ORIENTED_EDGE ( 'NONE', *, *, #%i, .F. ) ; "
                "#%i = ORIENTED_EDGE ( 'NONE', *, *, #%i, .T. ) ;\n",
             global.next_identifier, bottom_vertex_identifier[i], bottom_vertex_identifier[i_start + (adjusted_i + 1) % get_contour_npoints (ct)], bottom_infinite_line_identifier[i],      // <-- MIGHT NEED TO REVERSE THIS???
             global.next_identifier + 1, global.next_identifier,
             global.next_identifier + 2, global.next_identifier);
    bottom_edge_identifier[i] = global.next_identifier; /* Add 1 for same oriented, add 2 for back oriented */
    global.next_identifier = global.next_identifier + 3;

    /* Top */
    fprintf (f, "#%i = EDGE_CURVE ( 'NONE', #%i, #%i, #%i,   .T. ) ; "
                "#%i = ORIENTED_EDGE ( 'NONE', *, *, #%i, .F. ) ; "
                "#%i = ORIENTED_EDGE ( 'NONE', *, *, #%i, .T. ) ;\n",
             global.next_identifier, top_vertex_identifier[i], top_vertex_identifier[i_start + (adjusted_i + 1) % get_contour_npoints (ct)], top_infinite_line_identifier[i],                 // <-- MIGHT NEED TO REVERSE THIS???
             global.next_identifier + 1, global.next_identifier,
             global.next_identifier + 2, global.next_identifier);
    top_edge_identifier[i] = global.next_identifier; /* Add 1 for same oriented, add 2 for back oriented */
    global.next_identifier = global.next_identifier + 3;

    /* Side */
    fprintf (f, "#%i = EDGE_CURVE ( 'NONE', #%i, #%i, #%i,   .T. ) ; "
                "#%i = ORIENTED_EDGE ( 'NONE', *, *, #%i, .F. ) ; "
                "#%i = ORIENTED_EDGE ( 'NONE', *, *, #%i, .T. ) ;\n",
             global.next_identifier, bottom_vertex_identifier[i], top_vertex_identifier[i], side_infinite_line_identifier[i],
             global.next_identifier + 1, global.next_identifier,
             global.next_identifier + 2, global.next_identifier);
    side_edge_identifier[i] = global.next_identifier; /* Add 1 for same oriented, add 2 for back oriented */
    global.next_identifier = global.next_identifier + 3;
  }

  /* Define the faces */

  /* Bottom */
  {
    PLINE *ct = contour;
    int icont;
    int start_i;

    start_i = 0;
    for (icont = 0; icont < ncontours; icont++, start_i += get_contour_npoints (ct), ct = ct->next) {

      fprintf (f, "#%i = EDGE_LOOP ( 'NONE', ( ",
               global.next_identifier);
      for (i = start_i + get_contour_npoints (ct) - 1; i > start_i; i--)
        fprintf (f, "#%i, ", bottom_edge_identifier[i] + FWD);
      fprintf (f, "#%i ) ) ; "
                  "#%i = FACE_%sBOUND ( 'NONE', #%i, .T. ) ; \n",
               bottom_edge_identifier[start_i] + FWD,
               global.next_identifier + 1, icont > 0 ? "" : "OUTER_", global.next_identifier);
      bottom_face_bound_identifier[icont] = global.next_identifier + 1;
      global.next_identifier = global.next_identifier + 2;
    }

    fprintf (f, "#%i = ADVANCED_FACE ( 'NONE', ( ",
             global.next_identifier);
    for (icont = 0; icont < ncontours - 1; icont++)
      fprintf (f, "#%i, ",
               bottom_face_bound_identifier[icont]);
    fprintf (f, "#%i ), #%i, .F. ) ;\n",
             bottom_face_bound_identifier[ncontours - 1], bottom_plane_identifier);
    bottom_face_identifier = global.next_identifier;
    global.next_identifier = global.next_identifier + 1;
  }

  /* Top */
  {
    PLINE *ct = contour;
    int icont;
    int start_i;

    start_i = 0;
    for (icont = 0; icont < ncontours; icont++, start_i += get_contour_npoints (ct), ct = ct->next) {
      fprintf (f, "#%i = EDGE_LOOP ( 'NONE', ( ",
               global.next_identifier);
      for (i = start_i; i < start_i + get_contour_npoints (ct) - 1; i++)
        fprintf (f, "#%i, ", top_edge_identifier[i] + REV);
      fprintf (f, "#%i ) ) ; "
                  "#%i = FACE_%sBOUND ( 'NONE', #%i, .T. ) ; \n",
               top_edge_identifier[start_i + get_contour_npoints (ct) - 1] + REV,
               global.next_identifier + 1, icont > 0 ? "" : "OUTER_", global.next_identifier);
      top_face_bound_identifier[icont] = global.next_identifier + 1;
      global.next_identifier = global.next_identifier + 2;
    }

    fprintf (f, "#%i = ADVANCED_FACE ( 'NONE', ( ",
             global.next_identifier);
    for (icont = 0; icont < ncontours - 1; icont++)
      fprintf (f, "#%i, ",
               top_face_bound_identifier[icont]);
    fprintf (f, "#%i ), #%i, .F. ) ;\n",
             top_face_bound_identifier[ncontours - 1], top_plane_identifier);
    top_face_identifier = global.next_identifier;
    global.next_identifier = global.next_identifier + 1;
  }

  /* Sides */
  for (i = 0; i < npoints; i++) {

    /* Walk through the contours until we find the right one to look at */
    PLINE *ct = contour;
    int adjusted_i = i;
    int i_start = 0;

    while (adjusted_i >= get_contour_npoints (ct)) {
      adjusted_i -= get_contour_npoints (ct);
      i_start += get_contour_npoints (ct);
      ct = ct->next;
    }

    fprintf (f, "#%i = EDGE_LOOP ( 'NONE', ( #%i, #%i, #%i, #%i ) ) ; "
                "#%i = FACE_OUTER_BOUND ( 'NONE', #%i, .T. ) ; "
                "#%i = ADVANCED_FACE ( 'NONE', ( #%i ), #%i, .F. ) ;\n",
             global.next_identifier, side_edge_identifier[i_start + (adjusted_i + 1) % get_contour_npoints (ct)] + REV, top_edge_identifier[i] + FWD, side_edge_identifier[i] + FWD, bottom_edge_identifier[i] + REV,
             global.next_identifier + 1, global.next_identifier,
             global.next_identifier + 2, global.next_identifier + 1, side_plane_identifier[i]);
    side_face_identifier[i] = global.next_identifier + 2;
    global.next_identifier = global.next_identifier + 3;
  }

  /* Closed shell which bounds the brep solid */
  pcb_shell_identifier = global.next_identifier;
  global.next_identifier++;
  fprintf (f, "#%i = CLOSED_SHELL ( 'NONE', ( #%i, #%i, ", pcb_shell_identifier, bottom_face_identifier, top_face_identifier);
  for (i = 0; i < npoints - 1; i++) {
    fprintf (f, "#%i, ", side_face_identifier[i]);
  }
  fprintf (f, "#%i) ) ;\n",
           side_face_identifier[npoints - 1]);

  /* Finally emit the brep solid definition */
  fprintf (f, "#%i = MANIFOLD_SOLID_BREP ( 'PCB outline', #%i ) ;\n", brep_identifier, pcb_shell_identifier);

  g_free (side_plane_identifier);
  g_free (bottom_infinite_line_identifier);
  g_free (top_infinite_line_identifier);
  g_free (side_infinite_line_identifier);
  g_free (bottom_vertex_identifier);
  g_free (top_vertex_identifier);
  g_free (bottom_edge_identifier);
  g_free (top_edge_identifier);
  g_free (side_edge_identifier);
  g_free (side_face_identifier);
  g_free (bottom_face_bound_identifier);
  g_free (top_face_bound_identifier);
}
#undef FWD
#undef REV

static void
step_end_file (FILE *f)
{
  fprintf (f, "ENDSEC;\n" );
  fprintf (f, "END-ISO-10303-21;\n" );
}

static void
step_hid_export_to_file (FILE * the_file, HID_Attr_Val * options)
{
  int i;
  static int saved_layer_stack[MAX_LAYER];
  FlagType save_thindraw;
  POLYAREA *outline;

  save_thindraw = PCB->Flags;
  CLEAR_FLAG(THINDRAWFLAG, PCB);
  CLEAR_FLAG(THINDRAWPOLYFLAG, PCB);
  CLEAR_FLAG(CHECKPLANESFLAG, PCB);

  global.f = the_file;

  step_start_file (global.f, global.filename);

  outline = board_outline_poly (true);
  step_emit_board_contour (global.f, outline->contours);
  poly_Free (&outline);

  memset (global.print_group, 0, sizeof (global.print_group));
  memset (global.print_layer, 0, sizeof (global.print_layer));

  global.outline_layer = NULL;

  for (i = 0; i < max_copper_layer; i++)
    {
      LayerType *layer = PCB->Data->Layer + i;
      if (layer->LineN || layer->TextN || layer->ArcN || layer->PolygonN)
        global.print_group[GetLayerGroupNumberByNumber (i)] = 1;

      if (strcmp (layer->Name, "outline") == 0 ||
          strcmp (layer->Name, "route") == 0)
        {
          global.outline_layer = layer;
        }
    }
  global.print_group[GetLayerGroupNumberByNumber (bottom_silk_layer)] = 1;
  global.print_group[GetLayerGroupNumberByNumber (top_silk_layer)] = 1;
  for (i = 0; i < max_copper_layer; i++)
    if (global.print_group[GetLayerGroupNumberByNumber (i)])
      global.print_layer[i] = 1;

  memcpy (saved_layer_stack, LayerStack, sizeof (LayerStack));
  qsort (LayerStack, max_copper_layer, sizeof (LayerStack[0]), layer_sort);

  /* reset static vars */
  step_set_layer (NULL, 0, -1);
  use_gc (NULL);

//  hid_expose_callback (&step_hid, NULL, 0);

  step_set_layer (NULL, 0, -1);  /* reset static vars */
//  hid_expose_callback (&step_hid, NULL, 0);

  memcpy (LayerStack, saved_layer_stack, sizeof (LayerStack));
  PCB->Flags = save_thindraw;
}

static void
step_do_export (HID_Attr_Val * options)
{
  FILE *fh;
  int save_ons[MAX_LAYER + 2];
  int i;

  if (!options)
    {
      step_get_export_options (0);
      for (i = 0; i < NUM_OPTIONS; i++)
        global.step_values[i] = step_attribute_list[i].default_val;
      options = global.step_values;
    }

  global.filename = options[HA_stepfile].str_value;
  if (!global.filename)
    global.filename = "pcb-out.step";

  fh = fopen (global.filename, "w");
  if (fh == NULL)
    {
      perror (global.filename);
      return;
    }

  hid_save_and_show_layer_ons (save_ons);
  step_hid_export_to_file (fh, options);
  hid_restore_layer_ons (save_ons);

  step_end_file (fh);
  fclose (fh);
}

static void
step_parse_arguments (int *argc, char ***argv)
{
  hid_register_attributes (step_attribute_list, NUM_OPTIONS);
  hid_parse_command_line (argc, argv);
}

static int
step_set_layer (const char *name, int group, int empty)
{
  static int lastgroup = -1;
  int idx = (group >= 0 && group < max_group)
            ? PCB->LayerGroups.Entries[group][0]
            : group;
  if (name == 0)
    name = PCB->Data->Layer[idx].Name;

  if (empty == -1)
    lastgroup = -1;
  if (empty)
    return 0;

  if (idx >= 0 && idx < max_copper_layer && !global.print_layer[idx])
    return 0;

  if (strcmp (name, "invisible") == 0)
    return 0;

  global.is_drill = (SL_TYPE (idx) == SL_PDRILL || SL_TYPE (idx) == SL_UDRILL);
  global.is_mask  = (SL_TYPE (idx) == SL_MASK);
  global.is_assy  = (SL_TYPE (idx) == SL_ASSY);
  global.is_copper = (SL_TYPE (idx) == 0);
  global.is_paste  = (SL_TYPE (idx) == SL_PASTE);

  if (group < 0 || group != lastgroup)
    {
      lastgroup = group;

      use_gc (NULL);  /* reset static vars */
    }

  return 1;
}

static hidGC
step_make_gc (void)
{
  hidGC gc = (hidGC) calloc (1, sizeof (struct step_gc_struct));
  stepGC step_gc = (stepGC)gc;

  gc->hid = &step_hid;
  gc->hid_draw = &step_graphics;

  step_gc->cap = Trace_Cap;

  return gc;
}

static void
step_destroy_gc (hidGC gc)
{
  free (gc);
}

static void
step_use_mask (enum mask_mode mode)
{
  /* does nothing */
}

static void
step_set_color (hidGC gc, const char *name)
{
  stepGC step_gc = (stepGC)gc;

  if (strcmp (name, "erase") == 0 || strcmp (name, "drill") == 0)
    {
      step_gc->r = step_gc->g = step_gc->b = 255;
      step_gc->erase = 1;
    }
  else
    {
      int r, g, b;
      sscanf (name + 1, "%02x%02x%02x", &r, &g, &b);
      step_gc->r = r;
      step_gc->g = g;
      step_gc->b = b;
      step_gc->erase = 0;
    }
}

static void
step_set_line_cap (hidGC gc, EndCapStyle style)
{
  stepGC step_gc = (stepGC)gc;

  step_gc->cap = style;
}

static void
step_set_line_width (hidGC gc, Coord width)
{
  stepGC step_gc = (stepGC)gc;

  step_gc->width = width;
}

static void
step_set_draw_xor (hidGC gc, int xor_)
{
}

static void
step_set_draw_faded (hidGC gc, int faded)
{
}

static void
use_gc (hidGC gc)
{
  stepGC step_gc = (stepGC)gc;

  static int lastcap = -1;
  static int lastcolor = -1;

  if (gc == NULL)
    {
      lastcap = lastcolor = -1;
      return;
    }
  if (gc->hid != &step_hid)
    {
      fprintf (stderr, "Fatal: GC from another HID passed to step HID\n");
      abort ();
    }
  if (lastcap != step_gc->cap)
    {
      fprintf (global.f, "%%d setlinecap %%d setlinejoin\n");
      lastcap = step_gc->cap;
    }
#define CBLEND(gc) (((step_gc->r)<<24)|((step_gc->g)<<16)|((step_gc->b)<<8))
  if (lastcolor != CBLEND (gc))
    {
      double r, g, b;
      r = step_gc->r;
      g = step_gc->g;
      b = step_gc->b;
      if (step_gc->r == step_gc->g && step_gc->g == step_gc->b)
        fprintf (global.f, "%g gray\n", r / 255.0);
      else
        fprintf (global.f, "%g %g %g rgb\n", r / 255.0, g / 255.0, b / 255.0);
      lastcolor = CBLEND (gc);
    }
}

static void
step_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
}

static void
step_draw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
}

static void
step_draw_arc (hidGC gc, Coord cx, Coord cy, Coord width, Coord height,
               Angle start_angle, Angle delta_angle)
{
}

static void
step_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius)
{
}

static void
step_fill_polygon (hidGC gc, int n_coords, Coord *x, Coord *y)
{
}

static void
fill_polyarea (hidGC gc, POLYAREA * pa, const BoxType * clip_box)
{
}

static void
step_draw_pcb_polygon (hidGC gc, PolygonType * poly, const BoxType * clip_box)
{
  fill_polyarea (gc, poly->Clipped, clip_box);
  if (TEST_FLAG (FULLPOLYFLAG, poly))
    {
      POLYAREA *pa;

      for (pa = poly->Clipped->f; pa != poly->Clipped; pa = pa->f)
        fill_polyarea (gc, pa, clip_box);
    }
}

static void
step_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
}

static void
step_set_crosshair (int x, int y, int action)
{
}

#include "dolists.h"

void step_step_init (HID *hid)
{
  hid->get_export_options = step_get_export_options;
  hid->do_export          = step_do_export;
  hid->parse_arguments    = step_parse_arguments;
  hid->set_crosshair      = step_set_crosshair;
}

void step_step_graphics_class_init (HID_DRAW_CLASS *klass)
{
  klass->set_layer          = step_set_layer;
  klass->make_gc            = step_make_gc;
  klass->destroy_gc         = step_destroy_gc;
  klass->use_mask           = step_use_mask;
  klass->set_color          = step_set_color;
  klass->set_line_cap       = step_set_line_cap;
  klass->set_line_width     = step_set_line_width;
  klass->set_draw_xor       = step_set_draw_xor;
  klass->set_draw_faded     = step_set_draw_faded;
  klass->draw_line          = step_draw_line;
  klass->draw_arc           = step_draw_arc;
  klass->draw_rect          = step_draw_rect;
  klass->fill_circle        = step_fill_circle;
  klass->fill_polygon       = step_fill_polygon;
  klass->fill_rect          = step_fill_rect;

  klass->draw_pcb_polygon   = step_draw_pcb_polygon;
}

void step_step_graphics_init (HID_DRAW *hid_draw)
{
  hid_draw->poly_before = true;
}

void
hid_step_init ()
{
  memset (&step_hid, 0, sizeof (HID));
  memset (&step_graphics, 0, sizeof (HID_DRAW));

  common_nogui_init (&step_hid);
  step_step_init (&step_hid);

  step_hid.struct_size        = sizeof (HID);
  step_hid.name               = "step";
  step_hid.description        = "STEP AP214 export";
  step_hid.exporter           = 1;

  common_draw_helpers_class_init (&step_graphics_class);
  step_step_graphics_class_init (&step_graphics_class);

  common_nogui_graphics_init (&step_graphics);
  common_draw_helpers_init (&step_graphics);

  hid_register_hid (&step_hid);

#include "step_lists.h"
}
