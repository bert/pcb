/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

/*!
 * \file dxf.c
 *
 * \author Copyright (C) 2007 .. 2011 by Bert Timmerman <bert.timmerman@xs4all.nl>
 * for DXF relevant code.\n
 *
 * \brief This file contains the DXF HID (Human Interface Device) exporter to
 * generate a DXF file containing Xref's of components and a series of DXF
 * files, one for every pcb layer.
 *
 * <hr>
 * <b>Drawing eXchange Format (DXF)</b>\n
 * DXF is a defacto industry standard for the exchange of drawing files
 * between various Computer Aided Drafting programs.\n
 * DXF is designed by Autodesk(TM).\n
 * For more details see http://www.autodesk.com .
 * <hr>
 * <h1><b>Warning.</b></h1>\n
 * Some notes about Coordinate systems:\n
 * <ul>
 * <li>PCB follows a "left handed" Cartesian Coordinate System.\n
 * Positive X is right, positive Y is down.\n
 * Angles are increasing counter clockwise (CCW) in degrees, with 0 degrees being left
 * (negative X-axis) and 90 degrees being down (positive Y-axis).\n
 * <li>AutoCAD DXF follows a "right handed" Cartesian Coordinate system.\n
 * Positive X is right, positive Y is up.\n
 * Angles are counter clockwise (CCW) in degrees, with 0 being right
 * (positive X) and 90 being up (positive Y).\n
 * <li>Autocad coordinates are either in mil (default) or in mm (see checkbox
 * in the DXF HID dialog).\n
 * <li>Lookup Wikipedia with keywords "Cartesian Coordinates" if you want more
 * information on this subject.\n
 * </ul>
 * Filled polygons are drawn with a hatch entity.\n
 * Elliptic arcs are drawn with an ellipse entity.\n
 * Successful importing the generated dxf files with these entities
 * requires AutoCAD version R14 or higher, or any other mechanical CAD
 * program with the same level of compatibility with regard to the DXF
 * format.\n
 * <hr>
 * <p>
 * Please send remarks, bugs, improvements, relevant opinions to
 * bert.timmerman@xs4all.nl or to the geda-dev mailing list. \n
 * Donations to speed up the development of the gEDA suite best go to
 * www.gedaconsulting.com. \n
 * <li>Have fun ;-)
 * </ul>
 * <hr>
 * <p>
 * <h1><b>The DXF HID function treeview.</b></h1>\n
 * The following function hierarchy, with the status of the functions in
 * between [], exists:
 * <ul>
 * <li>dxf_get_export_options [stable]
 * <li>dxf_do_export [stable]
 *   <ul>
 *   <li>dxf_get_export_options [stable]
 *   <li>dxf_export_xref_file [exp]
 *     <ul>
 *     <li>dxf_write_comment [stable]
 *     <li>dxf_write_header [stable]
 *       <ul>
 *       <li>dxf_write_header_metric_new [stable]
 *       <li>dxf_write_header_imperial_new [todo]
 *       </ul>
 *     <li>dxf_write_block_record [todo]
 *     <li>dxf_write_block [stable]
 *     <li>dxf_write_endsection [stable]
 *     <li>dxf_write_section [stable]
 *     <li>dxf_insert [stable]
 *     <li>dxf_write_eof [stable]
 *     </ul>
 *   <li>dxf_maybe_close_file [exp]
 *     <ul>
 *     <li>dxf_write_tail [todo]
 *     <li>dxf_write_tail_metric_new [todo]
 *     <li>dxf_write_tail_imperial_new [todo]
 *     <li>dxf_write_eof [stable]
 *     </ul>
 *   </ul>
 * <li>dxf_parse_arguments [stable]
 * <li>dxf_set_layer [exp]
 *     <ul>
 *     <li>dxf_maybe_close_file [stable]
 *     <li>dxf_write_header [stable]
 *       <ul>
 *       <li>dxf_write_header_metric_new [stable]
 *       <li>dxf_write_header_imperial_new [todo]
 *       </ul>
 *     <li>dxf_write_block_record [todo]
 *     <li>dxf_write_block [stable]
 *     <li>dxf_write_endsection [stable]
 *     <li>dxf_write_section [stable]
 *     </ul>
 * <li>dxf_make_gc [stable]
 * <li>dxf_destroy_gc [stable]
 * <li>dxf_use_mask [stable]
 * <li>dxf_set_color [stable]
 * <li>dxf_set_line_cap [stable]
 * <li>dxf_set_line_width [stable]
 * <li>dxf_draw_line [exp]
 *   <ul>
 *   <li>dxf_write_polyline [exp]
 *   <li>dxf_write_vertex [stable]
 *   <li>dxf_write_endseq [stable]
 *   </ul>
 * <li>dxf_draw_arc [exp]
 *   <ul>
 *   <li>dxf_write_ellipse [exp]
 *   </ul>
 * <li>dxf_draw_rect [exp]
 *   <ul>
 *   <li>dxf_write_polyline [exp]
 *   <li>dxf_write_vertex [stable]
 *   <li>dxf_write_endseq [stable]
 *   </ul>
 * <li>dxf_fill_circle [exp]
 *   <ul>
 *   <li>dxf_write_circle [exp]
 *   </ul>
 * <li>dxf_fill_polygon [exp]
 *   <ul>
 *   <li>dxf_write_hatch [stable]
 *   <li>dxf_write_hatch_boundary_path_polyline [stable]
 *   <li>dxf_write_hatch_boundary_path_polyline_vertex [stable]
 *   </ul>
 * <li>dxf_fill_rect [stable]
 *   <ul>
 *   <li>dxf_write_solid [stable]
 *   </ul>
 * <li>dxf_calibrate [stable]
 * <li>dxf_set_crosshair [stable]
 * </ul>
 * <br>
 * Notes:\n
 * [todo] denotes a function not yet implemented.\n
 * [exp] denotes a function not yet fully implemented.\n
 * [stable] denotes a function fully implemented.\n
 * <hr>
 * \n\n
 * <b>Include files</b>
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "macro.h"
#include "global.h"
#include "data.h"
#include "misc.h"
#include "error.h"
#include "draw.h"
#include "pcb-printf.h"

#include "hid.h"
#include "../hidint.h"
#include "hid/common/hidnogui.h"
#include "hid/common/draw_helpers.h"
#include "hid/common/hidinit.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif


/*!
 * \brief RCS identification string.
 */
RCSID ("$Id$");


/*!
 * \brief List with string data.
 */
typedef struct _StringList
{
  char *str;
    /*!< String value. */
  struct _StringList *next;
    /*!< Pointer to next item. */
} StringList;


/*!
 * \brief List with pcb element header data.
 */
typedef struct _DxfList
{
  char *descr;
    /*!< description of pcb element. */
  char *value;
    /*!< value of pcb element. */
  int num;
    /*!< amount of elements per unique pcb element. */
  StringList *refdes;
    /*!< list of reference designators (refdes's) of pcb elements. */
  struct _DxfList *next;
    /*!< pointer to next (unique) element header data. */
} DxfList;


#define NOSHAPE 0
#define ROUND 1 /* shaped like a circle */
#define OCTAGON 2 /* shape like an octagon */
#define SQUARE 3 /* shaped like a square */
#define ROUNDCLEAR 4 /* round clearance in negatives */
#define SQUARECLEAR 5 /* square clearance in negatives */
#define THERMAL 6 /* negative thermal relief */


/*
 * Function prototypes.
 */
static void dxf_beep (void);
static void dxf_calibrate (double xval, double yval);
static char *dxf_clean_string (char *in);
static void dxf_destroy_gc (hidGC gc);
static void dxf_do_export (HID_Attr_Val * options);
static void dxf_draw_arc (hidGC gc, int cx, int cy, int width,
  int height, int start_angle, int delta_angle);
static void dxf_draw_line (hidGC gc, int x1, int y1, int x2, int y2);
static void dxf_draw_rect (hidGC gc, int x1, int y1, int x2, int y2);
static int dxf_drill_sort (const void *va, const void *vb);
static int dxf_export_xref_file (void);
static void dxf_fill_circle (hidGC gc, int cx, int cy, int radius);
static void dxf_fill_polygon (hidGC gc, int n_coords, int *x, int *y);
static void dxf_fill_rect (hidGC gc, int x1, int y1, int x2, int y2);
static HID_Attribute *dxf_get_export_options (int *n);
static int dxf_group_for_layer (int l);
static DxfList * dxf_insert (char *refdes, char *descr, char *value,
  DxfList *dxf);
static int dxf_layer_sort (const void *va, const void *vb);
static hidGC dxf_make_gc (void);
static void dxf_maybe_close_file ();
static void dxf_parse_arguments (int *argc, char ***argv);
static void dxf_progress (int dxf_so_far, int dxf_total,
  const char *dxf_message);
static void dxf_set_color (hidGC gc, const char *name);
static void dxf_set_crosshair (int x, int y);
static int dxf_set_layer (const char *name, int group);
static void dxf_set_line_cap (hidGC gc, EndCapStyle style);
static void dxf_set_line_width (hidGC gc, int width);
static void dxf_show_item (void *item);
static StringList * dxf_string_insert (char *str, StringList * list);
static void dxf_use_gc (hidGC gc, int radius);
static void dxf_use_mask (int use_it);
static void dxf_write_block (FILE *fp, int id_code, char *xref_name,
  char *block_name, char *linetype, char *layer, double x0,
  double y0, double z0, double thickness, int color,
  int paperspace, int block_type);
static void dxf_write_circle (FILE *fp, int id_code, char *linetype,
  char *layer, double x0, double y0, double z0, double extr_x0,
  double extr_y0, double extr_z0, double thickness, double radius,
  int color, int paperspace);
static void dxf_write_comment (FILE *fp, char *comment_string);
static void dxf_write_ellipse (FILE *fp, int id_code, char *linetype,
  char *layer, double x0, double y0, double z0, double x1,
  double y1, double z1, double extr_x0, double extr_y0,
  double extr_z0, double thickness, double ratio,
  double start_angle, double end_angle, int color, int paperspace);
static void dxf_write_endsection (FILE *fp);
static void dxf_write_eof (FILE *fp);
static void dxf_write_hatch (FILE *fp, char *pattern_name, int id_code,
  char *linetype, char *layer, double x0, double y0, double z0,
  double extr_x0, double extr_y0, double extr_z0, double thickness,
  double pattern_scale, double pixel_size, double pattern_angle,
  int color, int paperspace, int solid_fill, int associative,
  int style, int pattern_style, int pattern_double,
  int pattern_def_lines, int boundary_paths, int seed_points,
  double *seed_x0, double *seed_y0);
static void dxf_write_hatch_boundary_path_polyline (FILE *fp,
  int type_flag, int polyline_has_bulge, int polyline_is_closed,
  int polyline_vertices);
static void dxf_write_hatch_boundary_path_polyline_vertex (FILE *fp,
  double x0, double y0, double bulge);
static void dxf_write_header ();
static void dxf_write_header_imperial_new ();
static void dxf_write_header_metric_new ();
static void dxf_write_insert (FILE *fp, int id_code, char *block_name,
  char *linetype, char *layer, double x0, double y0, double z0,
  double thickness, double rel_x_scale, double rel_y_scale,
  double rel_z_scale, double column_spacing, double row_spacing,
  double rot_angle, int color, int attribute_follows,
  int paperspace, int columns, int rows);
static void dxf_write_polyline (FILE *fp, int id_code, char *linetype,
  char *layer, double x0, double y0, double z0,
  double extr_x0, double extr_y0, double extr_z0,
  double thickness, double start_width, double end_width,
  int color, int vertices_follow, int paperspace, int flag,
  int polygon_mesh_M_vertex_count, int polygon_mesh_N_vertex_count,
  int smooth_M_surface_density, int smooth_N_surface_density,
  int surface_type);
static void dxf_write_section (FILE *fp, char *section_name);
static void dxf_write_solid (FILE *fp, int id_code, char *linetype,
  char *layer, double x0, double y0, double z0, double x1,
  double y1, double z1, double x2, double y2, double z2,
  double x3, double y3, double z3, double thickness, int color,
  int paperspace);
static void dxf_write_table_block_record (FILE *fp);
static void dxf_write_vertex (FILE *fp, int id_code, char *linetype,
  char *layer, double x0, double y0, double z0, double thickness,
  double start_width, double end_width, double bulge,
  double curve_fit_tangent_direction, int color, int paperspace,
  int flag);
static double dxf_xy_to_angle (double x, double y);
void hid_dxf_init ();


/*!
 * \brief Debugging on/off switch.
 */
#define DEBUG 1

/*!
 * \brief Error handling of not yet implemented functions.
 */
#define CRASH fprintf(stderr, "DXF error: pcb called an unimplemented DXF-HID function %s.\n", __FUNCTION__); abort()

/*!
 * pcb dxf HID version string
 */
#define PCB_DXF_HID_VERSION "PCB DXF HID - Version 0.0.1 (2011-10-23)"

/*!
 * \brief Dxf X-coordinate (in mil).
 */
#define DXF_X(pcb,x) (x)

/*!
 * \brief Dxf Y-coordinate (in mil).
 */
#define DXF_Y(pcb,y) ((pcb)->MaxHeight-(y))

/*!
 * \brief Dxf X offset (in mil).
 */
#define DXF_XOffset(pcb,x) (x)

/*!
 * \brief Dxf Y offset (in mil).
 */
#define DXF_YOffset(pcb,y) (-(y))

/*!
 * \brief Round of value to the nearest multiple of 100.
 */
#define DXF_ROUND(x) ((int)(((x)+50)/100)*100)

/*!
 * \brief DXF color definition, entities with this color follow the color
 * definition of the block in which it lives.
 */
#define DXF_COLOR_BYBLOCK 0

/*!
 * \brief DXF color definition, pen number "1" in the virtual pen-plotter.
 */
#define DXF_COLOR_RED 1

/*!
 * \brief DXF color definition, pen number "2" in the virtual pen-plotter.
 */
#define DXF_COLOR_YELLOW 2

/*!
 * \brief DXF color definition, pen number "3" in the virtual pen-plotter.
 */
#define DXF_COLOR_GREEN 3

/*!
 * \brief DXF color definition, pen number "4" in the virtual pen-plotter.
 */
#define DXF_COLOR_CYAN 4

/*!
 * \brief DXF color definition, pen number "5" in the virtual pen-plotter.
 */
#define DXF_COLOR_BLUE 5

/*!
 * \brief DXF color definition, pen number "6" in the virtual pen-plotter.
 */
#define DXF_COLOR_MAGENTA 6

/*!
 * \brief DXF color definition, pen number "7" in the virtual pen-plotter.
 */
#define DXF_COLOR_WHITE 7

/*!
 * \brief DXF color definition, pen number "8" in the virtual pen-plotter.
 */
#define DXF_COLOR_GREY 8

/*!
 * \brief DXF color definition, color of the entity follows the color
 * definition of the layer on which it lives.
 */
#define DXF_COLOR_BYLAYER 256

/*!
 * \brief This is where our hardware is going to live, default value, can be
 * ommitted in dxf output.
 */
#define DXF_MODELSPACE 0

/*!
 * \brief This is where your annotation (papersheet templates, fab notes and
 * such) should live, has to be included in DXF output for any entity to live
 * on paperspace.
 */
#define DXF_PAPERSPACE 1

/*!
 * \brief There is <b>always</b> a layer "0" defined, it's reasonably safe to
 * assume that this is a valid layername.
 */
#define DXF_DEFAULT_LAYER "0"

/*!
 * \brief There is <b>always</b> a linetype "BYLAYER" defined, it's reasonably
 * safe to assume that this is a valid linetype.
 */
#define DXF_DEFAULT_LINETYPE "BYLAYER"

/*!
 * \brief There is <b>always</b> a textstyle "STANDARD" defined, it's
 * reasonably safe to assume that this is a valid text style.
 */
#define DXF_DEFAULT_TEXTSTYLE "STANDARD"

/*!
 * \brief Directory where 3D models of parts live.
 *
 * For now the dxf_xref_pathname is set to "parts", in AutoCAD one can
 * configure default search directories for xrefs.
 */
#define DXF_DEFAULT_XREF_PATH_NAME "parts"

/*!
 * \brief Directory separator character (back slash).
 *
 * We target an Autodesk universe with all of it's quirks.
 *
 * \todo This has to be solved in a more elegant manner if we want to
 * use DXF files for *nix based CAD software.
 */
#define DXF_DIR_SEPARATOR "\\"

/*!
 * \brief Default hatch pattern for pcb polygons.
 *
 * \todo For now the hatch pattern name is set to "SOLID".\n
 * If pcb is ever to have thieving implemented, a hatch pattern name field
 * has to be added to the graphic context.
 */
#define DXF_DEFAULT_HATCH_PATTERN_NAME "SOLID"

/*!
 * \brief File pointer for DXF layer files.
 */
static FILE *fp;

/*!
 * \brief File name of layer DXF files.
 */
static char *dxf_filename;

/*!
 * \brief File name of DXF header template file.
 */
static char *dxf_header_filename;

/*!
 * \brief File name suffix for layer files.
 */
static char *dxf_filesuffix;

/*!
 * \brief .
 */
static int dxf_finding_apertures = 0;

/*!
 * \brief Layer name.
 */
static char *dxf_layername = 0;

/*!
 * \brief Line count ??.
 */
static int lncount = 0;

/*!
 * \brief DXF file with xrefs needed.
 */
static int dxf_xrefs;

/*!
 * \brief File name of Xref (blocks) DXF file.
 */
static char *dxf_xref_filename;

/*!
 * \brief DXF file with output in mm (not mils).
 */
static int dxf_metric;

/*!
 * \brief DXF file with layer color BYBLOCK (or by layer number).
 */
static int dxf_color_is_byblock;

/*!
 * \brief DXF file with verbose output (to contain DXF comments).
 */
static int dxf_verbose;

/*!
 * \brief export all PCB layers to DXF files.
 */
static int dxf_export_all_layers;

/*!
 * \brief Every DXF entity has a unique identifier (per DXF file).
 */
static int dxf_id_code = 0;

/*!
 * \brief Record with all values of the DXF HID.
 */
static HID dxf_hid;

/*!
 * \brief Drill (hole) properties.
 */
typedef struct
{
  int diam;
    /*!< drill diameter. */
  int x;
    /*!< X-coordinate. */
  int y;
    /*!< Y-coordinate. */
} DxfPendingDrills;

/*!
 * \brief Layer is a mask.
 */
static int is_mask;

/*!
 * \brief Current mask.
 */
static int current_mask;

/*!
 * \brief Entity is a drill (hole).
 */
static int is_drill;

static int was_drill;

static int n_layerapps = 0;

static int c_layerapps = 0;

/*!
 * \brief Pending drill (holes).
 */
DxfPendingDrills *dxf_pending_drills = 0;

/*!
 * \brief Number of pending drill (holes).
 */
int dxf_n_pending_drills = 0;

/*!
 * \brief Maximum number of pending drills (holes).
 */
int dxf_max_pending_drills = 0;

/*!
 * \brief Definition of graphic context for the dxf HID.
 *
 * This graphics context is an opaque pointer defined by the HID.\n
 * GCs are HID-specific; attempts to use one HID's GC for a different
 * HID  will result in a fatal error.\n
 *
 * \todo In pcb the cap <b>always</b> extends beyond the coordinates
 * given, by half the width of the line.\n
 * With DXF polylines the cap is by default 0, and thus doesn't extend
 * beyond the coordinates.\n
 * A round cap could be implemented by drawing a donut (a series of
 * polyline vertices) with an inner radius of zero at the start and
 * endpoint of a line, this doesn't solve the problem for other aperture
 * shapes, and is thus not yet implemented.
 */
typedef struct hid_gc_struct
{
  EndCapStyle cap;
    /*!< end cap style. */
  int width;
    /*!< width. */
  int color;
    /*!< color. */
  int erase;
    /*!< erase. */
  int drill;
    /*!< drill. */
} hid_gc_struct;

/*!
 * \brief Definition of options the user can select in the DXF exporter
 * dialog.
 */
static HID_Attribute dxf_options[] =
{
/*
%start-doc options "DXF Export"
@ftable @code
@item --dxffile <string>
DXF output file prefix. Can include a path.
@end ftable
%end-doc
*/
  {"dxffile", "DXF layer filename base",
  HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_dxffile 0

/*
%start-doc options "DXF Export"
@ftable @code
@item --metric <boolean>
Export DXF files in mm. Default is mil
@end ftable
%end-doc
*/
  {"metric", "export DXF files in mm",
  HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_metric 1

/*
%start-doc options "DXF Export"
@ftable @code
@item --layer-color-BYBLOCK <boolean>
Export entities in color BYBLOCK. Default is BYLAYER.
@end ftable
%end-doc
*/
  {"layer-color-BYBLOCK", "export entities in color BYBLOCK",
  HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_color_byblock 2

/*
%start-doc options "DXF Export"
@ftable @code
@item --xrefs <boolean>
Export a DXF file with XREFS. Default is no XREFS.
@end ftable
%end-doc
*/
  {"xrefs", "export a DXF file with xrefs",
  HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_xrefs 3

/*
%start-doc options "DXF Export"
@ftable @code
@item --xreffile <string>
DXF Xrefs filename. Can include a path.
@end ftable
%end-doc
*/
  {"xreffile", "DXF Xrefs filename",
  HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_xreffile 4

/*
%start-doc options "DXF Export"
@ftable @code
@item --verbose <boolean>
Verbose output to stderr (comments).
@end ftable
%end-doc
*/
  {"be verbose", "verbose output to stderr (comments)",
  HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_verbose 5

/*
%start-doc options "DXF Export"
@ftable @code
@item --export-all-layers <boolean>
Export all layers.
@end ftable
%end-doc
*/
  {"export all layers", "export all layers",
  HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_export_all_layers 6
};

#define NUM_OPTIONS (sizeof(dxf_options)/sizeof(dxf_options[0]))

/*!
 * \brief Used for HID attributes (exporting and printing, mostly).
 *
 * HA_boolean uses int_value, HA_enum sets int_value to the index and
 * str_value to the enumeration string.\n
 * HID_Label just shows the default str_value.\n
 * HID_Mixed is a real_value followed by an enum, like 0.5in or 100mm.
 */
static HID_Attr_Val dxf_values[NUM_OPTIONS];

static int pagecount = 0;

static int linewidth = -1;

static int lastgroup = -1;

static int lastcap = -1;

static int lastcolor = -1;

static int print_group[MAX_LAYER];

static int print_layer[MAX_LAYER];

/*!
 * \brief The last X coordinate.
 */
static int dxf_lastX;

/*!
 * \brief The last Y coordinate.
 */
static int dxf_lastY;

/*!
 * \brief Find a group for a given layer ??.
 */
static int
dxf_group_for_layer
(
  int l
)
{
  if ((l < max_copper_layer + 2) && (l >= 0))
  {
    return GetLayerGroupNumberByNumber (l);
  }
  /* else something unique */
  return max_group + 3 + l;
}

/*!
 * \brief Sort layers ??.
 */
static int
dxf_layer_sort
(
  const void *va,
  const void *vb
)
{
  int a;
  int b;
  int d ;

  a = *(int *) va;
  b = *(int *) vb;
  d = dxf_group_for_layer (b) - dxf_group_for_layer (a);
  if (d) return d;
  return b - a;
}

/*!
 * \brief Convert pcb x,y coordinates to an angle relative to (0.0, 0.0).
 */
static double
dxf_xy_to_angle
(
  double x,
  double y
)
{
  double theta;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_xy_to_angle () function.\n",
    __FILE__, __LINE__);
#endif
  if ((x > 0.0) && (y >= 0.0)) theta = 180.0;
  else if ((x <= 0.0) && (y > 0.0)) theta = 90.0;
  else if ((x < 0.0) && (y <= 0.0)) theta = 0.0;
  else if ((x >= 0.0) && (y < 0.0)) theta = 270.0;
  else
  {
    theta = 0.0;
    Message ("DXF Warning: in dxf_xy_to_angle ():\n"
             "     unable to figure out angle of element\n"
             "     because the pin is at the centroid of the part.\n"
             "     This is a BUG!!!\n"
             "     Setting to %g degrees\n", theta);
    if (dxf_verbose)
    {
      fprintf (stderr, "DXF Warning: in dxf_xy_to_angle ():\n"
                       "\tunable to figure out the angle of the element because the pin is at the centroid of the part.\n"
                       "\tThis is considered to be a BUG!!!\n"
                       "\tSetting to %f degrees\n", theta);
    }
  }
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_xy_to_angle () function.\n",
    __FILE__, __LINE__);
#endif
  return (theta);
}


/*!
 * \brief Clean up a string.
 *
 * Copy over input string to output string with some character conversions.\n
 * Go all the way to end of string to get the termination character.
 */
static char *
dxf_clean_string
(
  char *in
)
{
  char *out;
  int i;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_clean_string () function.\n",
    __FILE__, __LINE__);
#endif
  if ((out = malloc ((strlen (in) + 1) * sizeof (char))) == NULL)
  {
    Message ("DXF Error: in dxf_clean_string (): malloc () failed.\n");
    if (dxf_verbose)
    {
      fprintf (stderr, "DXF Error: in dxf_clean_string (): malloc () failed.\n");
    }
    exit (1);
  }
  for (i = 0; i <= strlen (in); i++)
  {
    switch (in[i])
    {
      case '"':
        out[i] = '\'';
        break;
      case ' ':
        out[i] = '_';
        break;
      default:
        out[i] = in[i];
        break;
    }
  }
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_clean_string () function.\n",
    __FILE__, __LINE__);
#endif
  return (out);
}


/*!
 * \brief Insert the string to the list of strings.
 */
static StringList *
dxf_string_insert
(
  char *str,
  StringList * list
)
{
  StringList *new;
  StringList *cur;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_string_insert () function.\n",
    __FILE__, __LINE__);
#endif
  if ((new = (StringList *) malloc (sizeof (StringList))) == NULL)
  {
    Message ("DXF Error: in dxf_string_insert (): malloc () failed.\n");
    if (dxf_verbose)
    {
      fprintf (stderr, "DXF Error: in dxf_string_insert (): malloc () failed.\n");
    }
    exit (1);
  }
  new->next = NULL;
  new->str = strdup (str);
  if (list == NULL)
  {
    return (new);
  }
  cur = list;
  while (cur->next != NULL) cur = cur->next;
  cur->next = new;
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_string_insert () function.\n",
    __FILE__, __LINE__);
#endif
  return (list);
}


/*!
 * \brief Write DXF output to a file for a block entity.
 *
 * The <c>BLOCKS</c> section of the DXF file contains all the block
 * definitions.\n
 * It contains the entities that make up the blocks used in the drawing,
 * including anonymous blocks generated by the HATCH command and by
 * associative dimensioning.\n
 * The format of the entities in this section is identical to those in the
 * <c>ENTITIES</c> section.\n
 * All entities in the <c>BLOCKS</c> section appear between block and endblk
 * entities.\n
 * Block and endblk entities appear only in the <c>BLOCKS</c> section.\n
 * Block definitions are never nested (that is, no block or endblk entity ever
 * appears within another block-endblk pair), although a block definition can
 * contain an insert entity.\n
 * \n
 * External references are written in the DXF file as block definitions,
 * except that they also include a string (group code 1) that specifies the
 * path and file name of the external reference.\n
 * \n
 * The block table handle, along with any xdata and persistent reactors,
 * appears in each block definition immediately following the <c>BLOCK</c>
 * record, which contains all of the specific information that a block table
 * record stores.\n
 * \n
 * The UCS in effect when a block definition is created becomes the WCS for
 * all entities in the block definition.\n
 * The new origin for these entities is shifted to match the base point
 * defined for the block definition.\n
 * All entity data is translated to fit this new WCS.\n
 * \n
 * <c>*MODEL_SPACE</c> and <c>*PAPER_SPACE</c> Block Definition.\n
 * Now, there are always two extra, empty definitions in the BLOCKS section,
 * titled <c>*MODEL_SPACE</c> and <c>*PAPER_SPACE</c>.\n
 * These definitions manifest the new representation of model space and paper
 * space as block definitions internally.\n
 * The entities contained in these definitions still appear in the
 * <c>ENTITIES</c> section for compatibility.\n
 * \n
 * Model Space and Paper Space Entity Segregation.\n
 * The interleaving between model space and paper space will no longer occurs,
 * because of internal organization.\n
 * Instead, all paper space entities are output, followed by model space
 * entities.\n
 * The flag distinguishing them is the group code 67.\n
 *
 * \todo Add group code 102 stuff.
 */
static void
dxf_write_block
(
  FILE *fp,
    /*!< file pointer to output file (or device). */
  int id_code,
    /*!< group code = 5. */
  char *xref_name,
    /*!< group code = 1. */
  char *block_name,
    /*!< group code = 2 and 3. */
  char *linetype,
    /*!< group code = 6\n
     * optional, if omitted defaults to BYLAYER. */
  char *layer,
    /*!< group code = 8. */
  double x0,
    /*!< group code = 10\n
     * base point. */
  double y0,
    /*!< group code = 20\n
     * base point. */
  double z0,
    /*!< group code = 30\n
     * base point. */
  double thickness,
    /*!< group code = 39\n
     * optional, if omitted defaults to 0.0. */
  int color,
    /*!< group code = 62\n
     * optional, if omitted defaults to BYLAYER. */
  int paperspace,
    /*!< group code = 67\n
     * optional, if omitted defaults to 0 (modelspace). */
  int block_type
    /*!< group code = 70\n
     * bit codes:\n
     * 1 = this is an anonymous Block generated by hatching,
     * associative dimensioning, other internal operations, or an
     * application\n
     * 2 = this Block has Attributes\n
     * 4 = this Block is an external reference (Xref)\n
     * 8 = not used\n
     * 16 = this Block is externally dependent\n
     * 32 = this is a resolved external reference, or dependent
     * of an external reference\n
     * 64 = this definition is referenced. */
)
{
  char *dxf_entity_name;
        
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_write_block () function.\n",
    __FILE__, __LINE__);
  fprintf (stderr, "[DXF entity with code %x]\n", id_code);
#endif
  dxf_entity_name = strdup ("BLOCK");
  if (strcmp (block_name, "") == 0)
  {
    if (dxf_verbose)
    {
      fprintf (stderr, "DXF Warning: empty block name string for the %s entity with id-code: %x\n", dxf_entity_name, id_code);
      fprintf (stderr, "\t%s entity is discarded from output.\n", dxf_entity_name);
    }
    return;
  }
  if (strcmp (xref_name, "") == 0)
  {
    if (dxf_verbose)
    {
      fprintf (stderr, "DXF Warning: empty xref name string for the %s entity with id-code: %x\n", dxf_entity_name, id_code);
      fprintf (stderr, "\t%s entity is discarded from output.\n", dxf_entity_name);
    }
    return;
  }
  if (strcmp (layer, "") == 0)
  {
    if (dxf_verbose)
    {
      fprintf (stderr, "DXF Warning: empty layer string for the %s entity with id-code: %x\n", dxf_entity_name, id_code);
      fprintf (stderr, "\t%s entity is relocated to layer 0.\n", dxf_entity_name);
    }
    layer = strdup (DXF_DEFAULT_LAYER);
  }
  fprintf (fp, "  0\n%s\n", dxf_entity_name);
  if (id_code != -1)
  {
    fprintf (fp, "  5\n%x\n", id_code);
  }
  /* group code 102 stuff goes here  */
  fprintf (fp, "100\nAcDbEntity\n");
  fprintf (fp, "  8\n%s\n", layer);
  fprintf (fp, "100\nAcDbBlockBegin\n");
  fprintf (fp, "  2\n%s\n", block_name);
  fprintf (fp, " 70\n%d\n", block_type);
  fprintf (fp, " 10\n%f\n", x0);
  fprintf (fp, " 20\n%f\n", y0);
  fprintf (fp, " 30\n%f\n", z0);
  fprintf (fp, "  3\n%s\n", block_name);
  if ((block_type && 4) || (block_type && 32))
  {
    fprintf (fp, "  1\n%s%s%s.dwg\n", xref_name, DXF_DIR_SEPARATOR, block_name);
  }
  if (paperspace == DXF_PAPERSPACE)
  {
    fprintf (fp, " 67\n%d\n", DXF_PAPERSPACE);
  }
  /* now write an end block marker */
  fprintf (fp, "  0\nENDBLK\n");
  if (id_code != -1)
  {
    fprintf (fp, "  5\n%x\n", id_code);
  }
  /* group code 102 stuff goes here */
  fprintf (fp, "100\nAcDbBlockEnd\n");
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_write_block () function.\n",
    __FILE__, __LINE__);
#endif
}


/*!
 * \brief Write DXF output to a file for a <c>BLOCK_RECORD</c> table.
 *
 * The DXF <c>BLOCK_RECORD</c> table is part of the <c>TABLES</c> section of
 * the header that comes after the <c>DIMSTYLE</c> table and before the
 * <c>ENTITIES</c> section.\n
 * The <c>BLOCK_RECORD</c> table contains record definitions of all
 * <c>BLOCK</c> entities, either regular <c>BLOCK</c> entities or external
 * referenced <c>BLOCK</c> entities, the so calld XREFS.\n
 * \todo Code it !! Make it happen !!
 */
static void
dxf_write_table_block_record
(
  FILE *fp
    /*!< file pointer to output file (or device). */
)
{
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_write_table_block_record () function.\n",
    __FILE__, __LINE__);
#endif
  /*! \todo Add code. */
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_write_table_block_record () function.\n",
    __FILE__, __LINE__);
#endif
}


/*!
 * \brief Write DXF output to a file for a <c>CIRCLE</c> entity.
 *
 * \todo The <c>CIRCLE</c> entity has to be replaced by a <c>POLYLINE</c>
 * entity  with the correct line width (trace width).
 * \todo The <c>CIRCLE</c> entity is to be used for (unplated) drill holes only.
 * In the mechanical CAD program these circles (drill holes) can be extruded
 * to the board thickness and subtracted from the extruded pcb outline.
 */
static void
dxf_write_circle
(
  FILE *fp,
    /*!< file pointer to output file (or device). */
  int id_code,
    /*!< group code = 5. */
  char *linetype,
    /*!< group code = 6\n
     * optional, defaults to BYLAYER. */
  char *layer,
    /*!< group code = 8. */
  double x0,
    /*!< group code = 10\n
     * base point. */
  double y0,
    /*!< group code = 20\n
     * base point. */
  double z0,
    /*!< group code = 30\n
     * base point. */
  double extr_x0,
    /*!< group code = 210\n
     * extrusion direction\n
     * optional, if ommited defaults to 0.0. */
  double extr_y0,
    /*!< group code = 220\n
     * extrusion direction\n
     * optional, if ommited defaults to 0.0. */
  double extr_z0,
    /*!< group code = 230\n
     * extrusion direction\n
     * optional, if ommited defaults to 1.0. */
  double thickness,
    /*!< group code = 39\n
     * optional, defaults to 0.0. */
  double radius,
    /*!< group code = 40. */
  int color,
    /*!< group code = 62\n
     * optional, defaults to BYLAYER. */
  int paperspace
    /*!< group code = 67\n
     * optional, defaults to 0 (modelspace). */
)
{
  char *dxf_entity_name;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_write_circle () function.\n",
    __FILE__, __LINE__);
  fprintf (stderr, "[DXF entity with code %x]\n", id_code);
#endif
  dxf_entity_name = strdup ("CIRCLE");
  if (radius == 0.0)
  {
    if (dxf_verbose)
    {
      fprintf (stderr, "Error: radius value equals 0.0 for the %s entity with id-code: %x\n", dxf_entity_name, id_code);
      return;
    }
  }
  if (strcmp (layer, "") == 0)
  {
    layer = strdup (DXF_DEFAULT_LAYER);
    if (dxf_verbose)
    {
      fprintf (stderr, "Warning: empty layer string for the %s entity with id-code: %x\n", dxf_entity_name, id_code);
      fprintf (stderr, "    %s entity is relocated to layer 0", dxf_entity_name);
    }
  }
  fprintf (fp, "  0\n%s\n", dxf_entity_name);
  fprintf (fp, "100\nAcDbCircle\n");
  if (id_code != -1)
  {
    fprintf (fp, "  5\n%x\n", id_code);
  }
  if (strcmp (linetype, DXF_DEFAULT_LINETYPE) != 0)
  {
    fprintf (fp, "  6\n%s\n", linetype);
  }
  fprintf (fp, "  8\n%s\n", layer);
  fprintf (fp, " 10\n%f\n", x0);
  fprintf (fp, " 20\n%f\n", y0);
  fprintf (fp, " 30\n%f\n", z0);
  fprintf (fp, " 210\n%f\n", extr_x0);
  fprintf (fp, " 220\n%f\n", extr_y0);
  fprintf (fp, " 230\n%f\n", extr_z0);
  if (thickness != 0.0)
  {
    fprintf (fp, " 39\n%f\n", thickness);
  }
  fprintf (fp, " 40\n%f\n", radius);
  if (color != DXF_COLOR_BYLAYER)
  {
    fprintf (fp, " 62\n%d\n", color);
  }
  if (paperspace == DXF_PAPERSPACE)
  {
    fprintf (fp, " 67\n%d\n", DXF_PAPERSPACE);
  }
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_write_circle () function.\n",
    __FILE__, __LINE__);
#endif
  return;
}


/*!
 * \brief Write DXF output for a comment string with line termination.
 *
 * The group code "999" indicates that the following line is a comment
 * string.\n
 * The AutoCAD command "DXFOUT" does not currently include such groups in a
 * DXF output file.\n
 * The AutoCAD command "DXFIN" honors them and ignores the comments.\n
 * Thus, you can use the 999 group to include comments in a DXF file you've
 * created.
 */
static void
dxf_write_comment
(
  FILE *fp,
    /*!< file pointer to output file (or device). */
  char *comment_string
    /*!< comment string. */
)
{
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_write_comment () function.\n",
    __FILE__, __LINE__);
#endif
  if (strcmp (comment_string ,"") == 0)
  {
    /* no use in writing an empty comment string to file */
    return;
  }
  fprintf (fp, "999\n%s\n", comment_string);
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_write_comment () function.\n",
    __FILE__, __LINE__);
#endif
}


/*!
 * \brief Write DXF output to a file for an ellipse entity.
 *
 * This entity requires AutoCAD version R14 or higher.\n
 * The ellipse entity is currently used to draw arcs.\n
 * Since the pcb file format allows for a height and a width value, it is thus
 * possible to draw an elliptical arc.
 *
 * \todo The ellipse entity has to be replaced by a polyline with the correct
 * line width (trace width).
 * When we figured out how to draw a polyline (with vertices) for an arc
 * (part of a circle) or an elliptical arc (part of an ellipse).
 * This function will then become obsolete.
 */
static void
dxf_write_ellipse
(
  FILE *fp,
    /*!< file pointer to output file (or device). */
  int id_code,
    /*!< group code = 5. */
  char *linetype,
    /*!< group code = 6\n
     * optional, defaults to BYLAYER. */
  char *layer,
    /*!< group code = 8. */
  double x0,
    /*!< group code = 10\n
     * base point. */
  double y0,
    /*!< group code = 20\n
     * base point. */
  double z0,
    /*!< group code = 30\n
     * base point. */
  double x1,
    /*!< group code = 11\n
     * endpoint of major axis, relative to the center (in WCS). */
  double y1,
    /*!< group code = 21\n
     * endpoint of major axis, relative to the center (in WCS). */
  double z1,
    /*!< group code = 31\n
     * endpoint of major axis, relative to the center (in WCS). */
  double extr_x0,
    /*!< group code = 210\n
     * extrusion direction\n
     * optional, if ommited defaults to 0.0. */
  double extr_y0,
    /*!< group code = 220\n
     * extrusion direction\n
     * optional, if ommited defaults to 0.0. */
  double extr_z0,
    /*!< group code = 230\n
     * extrusion direction\n
     * optional, if ommited defaults to 1.0. */
  double thickness,
    /*!< group code = 39\n
     * optional, defaults to 0.0. */
  double ratio,
    /*!< group code = 40\n
     * ratio of minor axis to major axis. */
  double start_angle,
    /*!< group code = 41\n
     * start parameter (this value is 0.0 for a full ellipse). */
  double end_angle,
    /*!< group code = 42\n
     * end parameter (this value is 2*pi for a full ellipse). */
  int color,
    /*!< group code = 62\n
     * optional, defaults to BYLAYER. */
  int paperspace
    /*!< group code = 67\n
     * optional, defaults to 0 (modelspace). */
)
{
  char *dxf_entity_name;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_write_ellipse () function.\n", __FILE__, __LINE__);
#endif
  dxf_entity_name = strdup ("ELLIPSE");
  if (ratio == 0.0)
  {
    if (dxf_verbose)
    {
      fprintf (stderr, "Error: ratio value equals 0.0 for the %s entity with id-code: %x\n", dxf_entity_name, id_code);
    }
    return;
  }
  if (strcmp (layer, "") == 0)
  {
    layer = strdup (DXF_DEFAULT_LAYER);
    if (dxf_verbose)
    {
      fprintf (stderr, "Warning: empty layer string for the %s entity with id-code: %x\n", dxf_entity_name, id_code);
      fprintf (stderr, "    %s entity is relocated to layer 0", dxf_entity_name);
    }
  }
  fprintf (fp, "  0\n%s\n", dxf_entity_name);
  fprintf (fp, "100\nAcDbEllipse\n");
  if (id_code != -1)
  {
    fprintf (fp, "  5\n%x\n", id_code);
  }
  if (strcmp (linetype, DXF_DEFAULT_LINETYPE) != 0)
  {
    fprintf (fp, "  6\n%s\n", linetype);
  }
  fprintf (fp, "  8\n%s\n", layer);
  fprintf (fp, " 10\n%f\n", x0);
  fprintf (fp, " 20\n%f\n", y0);
  fprintf (fp, " 30\n%f\n", z0);
  fprintf (fp, " 11\n%f\n", x1);
  fprintf (fp, " 21\n%f\n", y1);
  fprintf (fp, " 31\n%f\n", z1);
  fprintf (fp, " 210\n%f\n", extr_x0);
  fprintf (fp, " 220\n%f\n", extr_y0);
  fprintf (fp, " 230\n%f\n", extr_z0);
  if (thickness != 0.0)
  {
    fprintf (fp, " 39\n%f\n", thickness);
  }
  fprintf (fp, " 40\n%f\n", ratio);
  fprintf (fp, " 41\n%f\n", start_angle);
  fprintf (fp, " 42\n%f\n", end_angle);
  if (color != DXF_COLOR_BYLAYER)
  {
    fprintf (fp, " 62\n%d\n", color);
  }
  if (paperspace == DXF_PAPERSPACE)
  {
    fprintf (fp, " 67\n%d\n", DXF_PAPERSPACE);
  }
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_write_ellipse () function.\n", __FILE__, __LINE__);
#endif
  return;
}


/*!
 * \brief Write DXF output to a file for an end of section marker.
 */
static void
dxf_write_endsection
(
  FILE *fp
    /*!< file pointer to output file (or device) */
)
{
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_write_endsection () function.\n", __FILE__, __LINE__);
#endif
  fprintf (fp, "  0\nENDSEC\n");
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_write_endsection () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Write DXF output to a file for an end of sequence marker.
 *
 * No fields.\n
 * This marks the end of:\n
 * <ul>
 * <li>vertices (one or more "Vertex" entity) of a Polyline
 * <li>attribute entities ("Attrib" entity) to an "Insert" entity that has
 * Attributes (indicated by 66 group present and nonzero in Insert
 * entity).\n
 * </ul>
 */
static void
dxf_write_endseq
(
  FILE *fp
    /*!< file pointer to output file (or device).  */
)
{
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_write_endseq () function.\n", __FILE__, __LINE__);
#endif
  fprintf (fp, "  0\nENDSEQ\n");
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_write_endseq () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Write dxf output to a file for an EOF marker.
 */
static void
dxf_write_eof
(
  FILE *fp
    /*!< file pointer to output file (or device).  */
)
{
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_write_eof () function.\n", __FILE__, __LINE__);
#endif
  fprintf (fp, "  0\nEOF\n");
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_write_eof () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Write DXF output to a file for a hatch entity.
 *
 * This entity requires AutoCAD version R14 or higher.
 *
 * \todo writing a hatch with a number of hatch_seed_points > 0 is creating a
 * segmentation fault in the dxf hid (and thus pcb).
 */
static void
dxf_write_hatch
(
  FILE *fp,
    /*!< file pointer to output file (or device). */
  char *pattern_name,
    /*!< group code = 2. */
  int id_code,
    /*!< group code = 5. */
  char *linetype,
    /*!< group code = 6\n
                 * optional, defaults to BYLAYER. */
  char *layer,
    /*!< group code = 8. */
  double x0,
    /*!< group code = 10\n
     * base point. */
  double y0,
    /*!< group code = 20\n
     * base point.  */
  double z0,
    /*!< group code = 30\n
     * base point. */
  double extr_x0,
    /*!< group code = 210\n
     * extrusion direction\n
     * optional, if ommited defaults to 0.0. */
  double extr_y0,
    /*!< group code = 220\n
     * extrusion direction\n
     * optional, if ommited defaults to 0.0.  */
  double extr_z0,
    /*!< group code = 230\n
     * extrusion direction\n
     * optional, if ommited defaults to 1.0.  */
  double thickness,
    /*!< group code = 39\n
     * optional, defaults to 0.0.  */
  double pattern_scale,
    /*!< group code 41\n
     * pattern fill only. */
  double pixel_size,
    /*!< group code 47. */
  double pattern_angle,
    /*!< group code 52\n
     * pattern fill only. */
  int color,
    /*!< group code = 62\n
     * optional, defaults to \c BYLAYER. */
  int paperspace,
    /*!< group code = 67\n
     * optional, defaults to 0 (modelspace). */
  int solid_fill,
    /*!< group code = 70\n
     * 0 = pattern fill\n
     * 1 = solid fill. */
  int associative,
    /*!< group code = 71\n
     * 0 = non-associative\n
     * 1 = associative. */
  int style,
    /*!< group code = 75\n
     * 0 = hatch "odd parity" area (Normal style)\n
     * 1 = hatch outermost area only (Outer style)\n
     * 2 = hatch through entire area (Ignore style). */
  int pattern_style,
    /*!< group code = 76\n
     * 0 = user defined\n
     * 1 = predefined\n
     * 2 = custom. */
  int pattern_double,
    /*!< group code = 77\n
     * pattern fill only\n
     * 0 = not double\n
     * 1 = double. */
  int pattern_def_lines,
    /*!< group code = 78\n
     * number of pattern definition lines. */
  int boundary_paths,
    /*!< group code = 91\n
     * number of boundary paths (loops). */
  int seed_points,
    /*!< group code = 98\n
     * number of seed points.  */
  double *seed_x0,
    /*!< group code = 10\n
     * seed point X-value. */
  double *seed_y0
    /*!< group code = 20\n
     * seed point Y-value.  */
)
{
  char *dxf_entity_name;
  int i;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_write_hatch () function.\n", __FILE__, __LINE__);
  fprintf (stderr, "[DXF entity with code %x]\n", id_code);
#endif
  dxf_entity_name = strdup ("HATCH");
  if (strcmp (layer, "") == 0)
  {
    fprintf (stderr, "Warning: empty layer string for the %s entity with id-code: %x\n", dxf_entity_name, id_code);
    fprintf (stderr, "    %s entity is relocated to layer 0", dxf_entity_name);
    layer = strdup (DXF_DEFAULT_LAYER);
  }
  fprintf (fp, "  0\n%s\n", dxf_entity_name);
  fprintf (fp, "100\nAcDbHatch\n");
  fprintf (fp, "  2\n%s\n", pattern_name);
  if (id_code != -1)
  {
    fprintf (fp, "  5\n%x\n", id_code);
  }
  if (strcmp (linetype, DXF_DEFAULT_LINETYPE) != 0)
  {
    fprintf (fp, "  6\n%s\n", linetype);
  }
  fprintf (fp, "  8\n%s\n", layer);
  fprintf (fp, " 10\n%f\n", x0);
  fprintf (fp, " 20\n%f\n", y0);
  fprintf (fp, " 30\n%f\n", z0);
  fprintf (fp, "210\n%f\n", extr_x0);
  fprintf (fp, "220\n%f\n", extr_y0);
  fprintf (fp, "230\n%f\n", extr_z0);
  if (thickness != 0.0)
  {
    fprintf (fp, " 39\n%f\n", thickness);
  }
  if (!solid_fill)
  {
    fprintf (fp, " 42\n%f\n", pattern_scale);
  }
  fprintf (fp, " 47\n%f\n", pixel_size);
  if (!solid_fill)
  {
    fprintf (fp, " 52\n%f\n", pattern_angle);
  }
  if (color != DXF_COLOR_BYLAYER)
  {
    fprintf (fp, " 62\n%d\n", color);
  }
  if (paperspace == DXF_PAPERSPACE)
  {
    fprintf (fp, " 67\n%d\n", DXF_PAPERSPACE);
  }
  fprintf (fp, " 70\n%d\n", solid_fill);
  fprintf (fp, " 71\n%d\n", associative);
  fprintf (fp, " 75\n%d\n", style);
  if (!solid_fill)
  {
    fprintf (fp, " 77\n%d\n", pattern_double);
  }
  fprintf (fp, " 78\n%d\n", pattern_def_lines);
  fprintf (fp, " 98\n%d\n", seed_points);
  for (i = 0; i < seed_points; i++)
  {
    fprintf (fp, " 10\n%f\n", seed_x0[i]);
    fprintf (fp, " 20\n%f\n", seed_y0[i]);
  }
  fprintf (fp, " 91\n%d\n", boundary_paths);
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_write_hatch () function.\n", __FILE__, __LINE__);
#endif
  return;
}


/*!
 * \brief Write DXF output to a file for a hatch boundary path polyline.
 *
 * This entity requires AutoCAD version R14 or higher.
 */
static void
dxf_write_hatch_boundary_path_polyline
(
  FILE *fp,
    /*!< file pointer to output file (or device). */
  int type_flag,
    /*!< group code 92.  */
  int polyline_has_bulge,
    /*!< group code = 72\n
     * polyline boundary data group only. */
  int polyline_is_closed,
    /*!< group code = 73\n
     * polyline boundary data group only. */
  int polyline_vertices
    /*!< group code 93\n
     * number of polyline vertices to follow. */
)
{
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_write_hatch_boundary_path_polyline () function.\n", __FILE__, __LINE__);
#endif
  fprintf (fp, " 92\n%d\n", type_flag);
  fprintf (fp, " 72\n%d\n", polyline_has_bulge);
  fprintf (fp, " 73\n%d\n", polyline_is_closed);
  fprintf (fp, " 93\n%d\n", polyline_vertices);
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_write_hatch_boundary_path_polyline () function.\n", __FILE__, __LINE__);
#endif
  return;
}


/*!
 * \brief Write DXF output to a file for a hatch boundary polyline entity.
 *
 * This entity requires AutoCAD version R14 or higher.
 */
static void
dxf_write_hatch_boundary_path_polyline_vertex
(
  FILE *fp,
    /*!< file pointer to output file (or device). */
  double x0,
    /*!< group code = 10\n
     * X-value of vertex point. */
  double y0,
    /*!< group code = 20\n
     * Y-value of vertex point. */
  double bulge
    /*!< group code 42\n
     * bulge of polyline vertex\n
     * optional, defaults to 0.0. */
)
{
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_write_hatch_boundary_polyline_vertex () function.\n", __FILE__, __LINE__);
#endif
  fprintf (fp, " 10\n%f\n", x0);
  fprintf (fp, " 20\n%f\n", y0);
  if (bulge != 0.0)
  {
    fprintf (fp, " 42\n%f\n", bulge);
  }
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_write_hatch_boundary_polyline_vertex () function.\n", __FILE__, __LINE__);
#endif
  return;
}


/*!
 * \brief Write DXF output to a file for a imperial DXF header.
 *
 * Fall back for if no default imperial header template file exists in the
 * pcb/src/hid/dxf/template directory.\n
 * Write down a DXF header from scratch based on imperial values.\n
 * Included sections and tables are:\n
 * <ul>
* <li>HEADER section
 * <li>CLASSES section
 * <li>TABLES section
 *   <ul>
 *   <li>VPORT table
 *   <li>LTYPE table
 *   <li>LAYER table
 *   <li>STYLE table
 *   <li>VIEW table
 *   <li>UCS table
 *   <li>APPID table
 *   <li>DIMSTYLE table
 *   </ul>
 * </ul>
 */
static void
dxf_write_header_imperial_new ()
{
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_write_header_imperial_new () function.\n", __FILE__, __LINE__);
#endif
  /* write an imperial HEADER section */
  fprintf (fp, "  0\nSECTION\n");
  fprintf (fp, "  2\nHEADER\n"); /* Section name. */
  fprintf (fp, "  9\n$ACADVER\n  1\nAC1014\n"); /* AutoCAD drawing database version number. */
  fprintf (fp, "  9\n$ACADMAINTVER\n 70\n     0\n"); /* Maintenance version number. */
  fprintf (fp, "  9\n$DWGCODEPAGE\n  3\nANSI_1252\n"); /* Drawing code page. */
  fprintf (fp, "  9\n$INSBASE\n 10\n0.0\n 20\n0.0\n 30\n0.0\n"); /* Insertion base set by BASE command (in WCS). */
  fprintf (fp, "  9\n$EXTMIN\n0 10\n-0.012816\n 20\n-0.009063\n 30\n-0.001526\n"); /* X, Y, and Z drawing extents lower-left corner (in WCS). */
  fprintf (fp, "  9\n$EXTMAX\n 10\n88.01056\n 20\n35.022217\n 30\n0.0\n"); /* X, Y, and Z drawing extents upper-right corner (in WCS). */
  fprintf (fp, "  9\n$LIMMIN\n 10\n0.0\n 20\n0.0\n"); /* XY drawing limits lower-left corner (in WCS). */
  fprintf (fp, "  9\n$LIMMAX\n 10\n420.0\n 20\n297.0\n"); /* XY drawing limits upper-right corner (in WCS). */
  fprintf (fp, "  9\n$ORTHOMODE\n 70\n     0\n"); /* Ortho mode on if nonzero. */
  fprintf (fp, "  9\n$REGENMODE\n 70\n     1\n"); /* REGENAUTO mode on if nonzero. */
  fprintf (fp, "  9\n$FILLMODE\n 70\n     1\n"); /* Fill mode on if nonzero. */
  fprintf (fp, "  9\n$QTEXTMODE\n 70\n     0\n"); /* Quick text mode on if nonzero. */
  fprintf (fp, "  9\n$MIRRTEXT\n 70\n     1\n"); /* Mirror text if nonzero. */
  fprintf (fp, "  9\n$DRAGMODE\n 70\n     2\n"); /* 0 = off, 1 = on, 2 = auto. */
  fprintf (fp, "  9\n$LTSCALE\n 40\n1.0\n"); /* Global linetype scale. */
  fprintf (fp, "  9\n$OSMODE\n 70\n   125\n"); /* Running object snap modes. */
  fprintf (fp, "  9\n$ATTMODE\n 70\n     1\n"); /* Attribute visibility: 0 = none, 1 = normal, 2 = all. */
  fprintf (fp, "  9\n$TEXTSIZE\n 40\n0.1\n"); /* Default text height = 100 mil. */
  fprintf (fp, "  9\n$TRACEWID\n 40\n1.0\n"); /* Default trace width. */
  fprintf (fp, "  9\n$TEXTSTYLE\n  7\nSTANDARD\n"); /* Current text style name. */
  fprintf (fp, "  9\n$CLAYER\n  8\n0\n"); /* Current layer name. */
  fprintf (fp, "  9\n$CELTYPE\n  6\nBYLAYER\n"); /* Entity linetype name, or BYBLOCK or BYLAYER. */
  fprintf (fp, "  9\n$CECOLOR\n 62\n   256\n"); /* Current entity color number: 0 = BYBLOCK, 256 = BYLAYER. */
  fprintf (fp, "  9\n$CELTSCALE\n 40\n1.0\n"); /* Current entity linetype scale. */
  fprintf (fp, "  9\n$DELOBJ\n 70\n     1\n"); /* Controls object deletion: 0=deleted, 1=retained. */
  fprintf (fp, "  9\n$DISPSILH\n 70\n     0\n"); /* Controls the display of silhouette curves of body objects in wire-frame mode: 0=Off, 1=On. */
  fprintf (fp, "  9\n$DIMSCALE\n 40\n1.0\n"); /* Overall dimensioning scale factor. */
  fprintf (fp, "  9\n$DIMASZ\n 40\n0.1\n"); /* Dimensioning arrow size = 100 mil. */
  fprintf (fp, "  9\n$DIMEXO\n 40\n0.01\n"); /* Extension line offset = 10 mil. */
  fprintf (fp, "  9\n$DIMDLI\n 40\n0.1\n"); /* Dimension line increment = 100 mil. */
  fprintf (fp, "  9\n$DIMRND\n 40\n0.0\n"); /* Rounding value for dimension distances. */
  fprintf (fp, "  9\n$DIMDLE\n 40\n0.0\n"); /* Dimension line extension */
  fprintf (fp, "  9\n$DIMEXE\n 40\n0.05\n"); /* Extension line extension = 50 mil. */
  fprintf (fp, "  9\n$DIMTP\n 40\n0.0\n"); /* Plus tolerance. */
  fprintf (fp, "  9\n$DIMTM\n 40\n0.0\n"); /* Minus tolerance. */
  fprintf (fp, "  9\n$DIMTXT\n 40\n0.1\n"); /* Dimensioning text height = 100 mil. */
  fprintf (fp, "  9\n$DIMCEN\n 40\n0.01\n"); /* Size of center mark/lines = 10 mil. */
  fprintf (fp, "  9\n$DIMTSZ\n 40\n0.01\n"); /* Dimensioning tick size = 10 mil; 0 = no ticks. */
  fprintf (fp, "  9\n$DIMTOL\n 70\n     0\n"); /* Dimension tolerances generated if nonzero. */
  fprintf (fp, "  9\n$DIMLIM\n 70\n     0\n"); /* Dimension limits generated if nonzero. */
  fprintf (fp, "  9\n$DIMTIH\n 70\n     0\n"); /* Text inside horizontal if nonzero. */
  fprintf (fp, "  9\n$DIMTOH\n 70\n     0\n"); /* Text outside horizontal if nonzero. */
  fprintf (fp, "  9\n$DIMSE1\n 70\n     0\n"); /* First extension line suppressed if nonzero. */
  fprintf (fp, "  9\n$DIMSE2\n 70\n     0\n"); /* Second extension line suppressed if nonzero. */
  fprintf (fp, "  9\n$DIMTAD\n 70\n     1\n"); /* Text above dimension line if nonzero. */
  fprintf (fp, "  9\n$DIMZIN\n 70\n     8\n"); /* Controls suppression of zeros for primary unit values. */
  fprintf (fp, "  9\n$DIMBLK\n  1\n\n"); /* Arrow block name. */
  fprintf (fp, "  9\n$DIMASO\n 70\n     1\n"); /* 1 = create associative dimensioning, 0 = draw individual entities. */
  fprintf (fp, "  9\n$DIMSHO\n 70\n     1\n"); /* 1 = Recompute dimensions while dragging, 0 = drag original image. */
  fprintf (fp, "  9\n$DIMPOST\n  1\n\n"); /* General dimensioning suffix. */
  fprintf (fp, "  9\n$DIMAPOST\n  1\n\n"); /* Alternate dimensioning suffix. */
  fprintf (fp, "  9\n$DIMALT\n 70\n     0\n"); /* Alternate unit dimensioning performed if nonzero. */
  fprintf (fp, "  9\n$DIMALTD\n 70\n     4\n"); /* Alternate unit decimal places. */
  fprintf (fp, "  9\n$DIMALTF\n 40\n0.0254\n"); /* Alternate unit scale factor = inch --> mm. */
  fprintf (fp, "  9\n$DIMLFAC\n 40\n1.0\n"); /* Linear measurements scale factor. */
  fprintf (fp, "  9\n$DIMTOFL\n 70\n     1\n"); /* If text outside extensions, force line extensions between extensions if nonzero. */
  fprintf (fp, "  9\n$DIMTVP\n 40\n0.0\n"); /* Text vertical position. */
  fprintf (fp, "  9\n$DIMTIX\n 70\n     0\n"); /* Force text inside extensions if nonzero. */
  fprintf (fp, "  9\n$DIMSOXD\n 70\n     0\n"); /* Suppress outside-extensions dimension lines if nonzero. */
  fprintf (fp, "  9\n$DIMSAH\n 70\n     0\n"); /* Use separate arrow blocks if nonzero. */
  fprintf (fp, "  9\n$DIMBLK1\n  1\n\n"); /* First arrow block name. */
  fprintf (fp, "  9\n$DIMBLK2\n  1\n\n"); /* Second arrow block name. */
  fprintf (fp, "  9\n$DIMSTYLE\n  2\nSTANDARD\n"); /* Dimension style name. */
  fprintf (fp, "  9\n$DIMCLRD\n 70\n     0\n"); /*Dimension line color: range is 0 = BYBLOCK, 256 = BYLAYER. */
  fprintf (fp, "  9\n$DIMCLRE\n 70\n     0\n"); /* Dimension extension line color:  range is 0 = BYBLOCK, 256 = BYLAYER. */
  fprintf (fp, "  9\n$DIMCLRT\n 70\n     0\n"); /* Dimension text color:  range is 0 = BYBLOCK, 256 = BYLAYER. */
  fprintf (fp, "  9\n$DIMTFAC\n 40\n1.0\n"); /* Dimension tolerance display scale factor. */
  fprintf (fp, "  9\n$DIMGAP\n 40\n25.0\n"); /* Dimension line gap = 25 mil. */
  fprintf (fp, "  9\n$DIMJUST\n 70\n     0\n"); /* Horizontal dimension text position: 0=above dimension line and center-justified between extension lines, 1=above dimension line and next to first extension line, 2=above dimension line and next to second extension line, 3=above and center-justified to first extension line, 4=above and center-justified to second extension line. */
  fprintf (fp, "  9\n$DIMSD1\n 70\n     0\n"); /* Suppression of first extension line: 0=not suppressed, 1=suppressed. */
  fprintf (fp, "  9\n$DIMSD2\n 70\n     0\n"); /* Suppression of second extension line: 0=not suppressed, 1=suppressed. */
  fprintf (fp, "  9\n$DIMTOLJ\n 70\n     1\n"); /* Vertical justification for tolerance values: 0=Top, 1=Middle, 2=Bottom.*/
  fprintf (fp, "  9\n$DIMTZIN\n 70\n     0\n"); /* Controls suppression of zeros for tolerance values: 0 = Suppresses zero feet and precisely zero inches; 1 = Includes zero feet and precisely zero inches; 2 = Includes zero feet and suppresses zero inches; 3 = Includes zero inches and suppresses zero feet. */
  fprintf (fp, "  9\n$DIMALTZ\n 70\n     0\n"); /* Controls suppression of zeros for alternate unit dimension values: 0 = Suppresses zero feet and precisely zero inches; 1 = Includes zero feet and precisely zero inches; 2 = Includes zero feet and suppresses zero inches; 3 = Includes zero inches and suppresses zero feet. */
  fprintf (fp, "  9\n$DIMALTTZ\n 70\n     0\n"); /* Controls suppression of zeros for alternate tolerance values: 0 = Suppresses zero feet and precisely zero inches; 1 = Includes zero feet and precisely zero inches; 2 = Includes zero feet and suppresses zero inches; 3 = Includes zero inches and suppresses zero feet. */
  fprintf (fp, "  9\n$DIMFIT\n 70\n     3\n"); /* Placement of text and arrowheads; Possible values: 0 through 3. */
  fprintf (fp, "  9\n$DIMUPT\n 70\n     0\n"); /* Cursor functionality for user positioned text:  0=controls only the dimension line location, 1=controls the text position as well as the dimension line location. */
  fprintf (fp, "  9\n$DIMUNIT\n 70\n     2\n"); /* Units format for all dimension style family members except angular: 1 = Scientific; 2 = Decimal; 3 = Engineering; 4 = Architectural (stacked); 5 = Fractional (stacked); 6 = Architectural; 7 = Fractional. */
  fprintf (fp, "  9\n$DIMDEC\n 70\n     4\n"); /* Number of decimal places for the tolerance values of a primary units dimension. */
  fprintf (fp, "  9\n$DIMTDEC\n 70\n     4\n"); /* Number of decimal places to display the tolerance values. */
  fprintf (fp, "  9\n$DIMALTU\n 70\n     2\n"); /*Units format for alternate units of all dimension style family members except angular: 1 = Scientific; 2 = Decimal; 3 = Engineering; 4 = Architectural (stacked); 5 = Fractional (stacked); 6 = Architectural; 7 = Fractional. */
  fprintf (fp, "  9\n$DIMALTTD\n 70\n     2\n"); /*Number of decimal places for tolerance values of an alternate units dimension. */
  fprintf (fp, "  9\n$DIMTXSTY\n  7\nSTANDARD\n"); /* Dimension text style. */
  fprintf (fp, "  9\n$DIMAUNIT\n 70\n     0\n"); /* Angle format for angular dimensions: 0=Decimal degrees, 1=Degrees/minutes/seconds, 2=Gradians, 3=Radians, 4=Surveyor's units. */
  fprintf (fp, "  9\n$LUNITS\n 70\n     2\n"); /* Units format for coordinates and distances. */
  fprintf (fp, "  9\n$LUPREC\n 70\n     4\n"); /* Units precision for coordinates and distances */
  fprintf (fp, "  9\n$SKETCHINC\n 40\n1.0\n"); /* Sketch record increment. */
  fprintf (fp, "  9\n$FILLETRAD\n 40\n1.0\n"); /* Fillet radius. */
  fprintf (fp, "  9\n$AUNITS\n 70\n     0\n"); /* Units format for angles. */
  fprintf (fp, "  9\n$AUPREC\n 70\n     0\n"); /* Units precision for angles. */
  fprintf (fp, "  9\n$MENU\n  1\n.\n"); /* Name of menu file. */
  fprintf (fp, "  9\n$ELEVATION\n 40\n0.0\n"); /* Current elevation set by ELEV command. */
  fprintf (fp, "  9\n$PELEVATION\n 40\n0.0\n"); /* Current paper space elevation. */
  fprintf (fp, "  9\n$THICKNESS\n 40\n0.0\n"); /* Current thickness set by ELEV command. */
  fprintf (fp, "  9\n$LIMCHECK\n 70\n     0\n"); /* Nonzero if limits checking is on. */
  fprintf (fp, "  9\n$BLIPMODE\n 70\n     0\n"); /* Blip mode on if nonzero. */
  fprintf (fp, "  9\n$CHAMFERA\n 40\n10.0\n"); /* First chamfer distance. */
  fprintf (fp, "  9\n$CHAMFERB\n 40\n10.0\n"); /* Second chamfer distance. */
  fprintf (fp, "  9\n$CHAMFERC\n 40\n0.0\n"); /* Chamfer length. */
  fprintf (fp, "  9\n$CHAMFERD\n 40\n0.0\n"); /* Chamfer angle. */
  fprintf (fp, "  9\n$SKPOLY\n 70\n     0\n"); /* 0 = sketch lines, 1 = sketch polylines. */
  fprintf (fp, "  9\n$TDCREATE\n 40\n0.0\n"); /* Date/time of drawing creation. */
  fprintf (fp, "  9\n$TDUPDATE\n 40\n0.0\n"); /* Date/time of last drawing update. */
  fprintf (fp, "  9\n$TDINDWG\n 40\n0.0\n"); /* Cumulative editing time for this drawing. */
  fprintf (fp, "  9\n$TDUSRTIMER\n 40\n0.0\n"); /* User elapsed timer. */
  fprintf (fp, "  9\n$USRTIMER\n 70\n     1\n"); /* 0 = timer off, 1 = timer on. */
  fprintf (fp, "  9\n$ANGBASE\n 50\n0.0\n"); /* Angle 0 direction . */
  fprintf (fp, "  9\n$ANGDIR\n 70\n     0\n"); /* 1 = clockwise angles, 0 = counterclockwise. */
  fprintf (fp, "  9\n$PDMODE\n 70\n    98\n"); /* Point display mode. */
  fprintf (fp, "  9\n$PDSIZE\n 40\n0.0\n"); /* Point display size. */
  fprintf (fp, "  9\n$PLINEWID\n 40\n0.0\n"); /* Default polyline width. */
  fprintf (fp, "  9\n$COORDS\n 70\n     2\n"); /* Coordinate display: 0 = static, 1 = continuous update, 2 = "d<a" format. */
  fprintf (fp, "  9\n$SPLFRAME\n 70\n     0\n"); /* Spline control polygon display: 1 = on, 0 = off. */
  fprintf (fp, "  9\n$SPLINETYPE\n 70\n     6\n"); /* Spline curve type for PEDIT Spline. */
  fprintf (fp, "  9\n$SPLINESEGS\n 70\n     8\n"); /* Number of line segments per spline patch. */
  fprintf (fp, "  9\n$ATTDIA\n 70\n     0\n"); /* Attribute entry dialogs: 1 = on, 0 = off. */
  fprintf (fp, "  9\n$ATTREQ\n 70\n     1\n"); /* Attribute prompting during INSERT: 1 = on, 0 = off. */
  fprintf (fp, "  9\n$HANDLING\n 70\n     1\n"); /* Next available handle. */
  fprintf (fp, "  9\n$HANDSEED\n  5\n262\n"); /* Next available handle. */
  fprintf (fp, "  9\n$SURFTAB1\n 70\n     6\n"); /* Number of mesh tabulations in first direction. */
  fprintf (fp, "  9\n$SURFTAB2\n 70\n     6\n"); /* Number of mesh tabulations in second direction. */
  fprintf (fp, "  9\n$SURFTYPE\n 70\n     6\n"); /* Surface type for PEDIT Smooth. */
  fprintf (fp, "  9\n$SURFU\n 70\n     6\n"); /* Surface density (for PEDIT Smooth) in M direction. */
  fprintf (fp, "  9\n$SURFV\n 70\n     6\n"); /* Surface density (for PEDIT Smooth) in N direction. */
  fprintf (fp, "  9\n$UCSNAME\n  2\n\n"); /* Name of current UCS. */
  fprintf (fp, "  9\n$UCSORG\n 10\n0.0\n 20\n0.0\n 30\n0.0\n"); /* Origin of current UCS (in WCS). */
  fprintf (fp, "  9\n$UCSXDIR\n 10\n1.0\n 20\n0.0\n 30\n0.0\n"); /* Direction of current UCS's X axis (in WCS). */
  fprintf (fp, "  9\n$UCSYDIR\n 10\n0.0\n 20\n1.0\n 30\n0.0\n"); /* Direction of current UCS's Y axis (in WCS). */
  fprintf (fp, "  9\n$PUCSNAME\n  2\n\n"); /* Current paper space UCS name. */
  fprintf (fp, "  9\n$PUCSORG\n 10\n0.0\n 20\n0.0\n 30\n0.0\n"); /* Current paper space UCS origin. */
  fprintf (fp, "  9\n$PUCSXDIR\n 10\n1.0\n 20\n0.0\n 30\n0.0\n"); /* Current paper space UCS X axis. */
  fprintf (fp, "  9\n$PUCSYDIR\n 10\n0.0\n 20\n1.0\n 30\n0.0\n"); /* Current paper space UCS Y axis. */
  fprintf (fp, "  9\n$USERI1\n 70\n     0\n"); /* Five integer variables intended for use by third-party developers. */
  fprintf (fp, "  9\n$USERI2\n 70\n     0\n");
  fprintf (fp, "  9\n$USERI3\n 70\n     0\n");
  fprintf (fp, "  9\n$USERI4\n 70\n     0\n");
  fprintf (fp, "  9\n$USERI5\n 70\n     0\n");
  fprintf (fp, "  9\n$USERR1\n 40\n0.0\n"); /* Five real variables intended for use by third-party developers. */
  fprintf (fp, "  9\n$USERR2\n 40\n0.0\n");
  fprintf (fp, "  9\n$USERR3\n 40\n0.0\n");
  fprintf (fp, "  9\n$USERR4\n 40\n0.0\n");
  fprintf (fp, "  9\n$USERR5\n 40\n0.0\n");
  fprintf (fp, "  9\n$WORLDVIEW\n 70\n     1\n"); /*1 = set UCS to WCS during DVIEW/VPOINT, 0 = don't change UCS. */
  fprintf (fp, "  9\n$SHADEDGE\n 70\n     3\n"); /* 0 = faces shaded, edges not highlighted; 1 = faces shaded, edges highlighted in black; 2 = faces not filled, edges in entity color; 3 = faces in entity color, edges in black. */
  fprintf (fp, "  9\n$SHADEDIF\n 70\n    70\n"); /* Percent ambient/diffuse light, range 1-100, default 70. */
  fprintf (fp, "  9\n$TILEMODE\n 70\n     1\n"); /* 1 for previous release compatibility mode, 0 otherwise. */
  fprintf (fp, "  9\n$MAXACTVP\n 70\n    48\n"); /* Sets maximum number of viewports to be regenerated. */
  fprintf (fp, "  9\n$PINSBASE\n 10\n0.0\n 20\n0.0\n 30\n0.0\n"); /* Paper space insertion base point. */
  fprintf (fp, "  9\n$PLIMCHECK\n 70\n     0\n"); /* Limits checking in paper space when nonzero. */
  fprintf (fp, "  9\n$PEXTMIN\n 10\n1.000000E+20\n 20\n1.000000E+20\n 30\n1.000000E+20\n");  /* Minimum X, Y, and Z extents for paper space. */
  fprintf (fp, "  9\n$PEXTMAX\n 10\n-1.000000E+20\n 20\n-1.000000E+20\n 30\n-1.000000E+20\n"); /* Maximum X, Y, and Z extents for paper space. */
  fprintf (fp, "  9\n$PLIMMIN\n 10\n0.0\n 20\n0.0\n"); /* Minimum X and Y limits in paper space. */
  fprintf (fp, "  9\n$PLIMMAX\n 10\n11000.0\n 20\n8500.0\n"); /* Maximum X and Y limits in paper space. */
  fprintf (fp, "  9\n$UNITMODE\n 70\n     0\n"); /* Low bit set = display fractions, feet-and-inches, and surveyor's angles in input format. */
  fprintf (fp, "  9\n$VISRETAIN\n 70\n     1\n"); /* 0 = don't retain xref-dependent visibility settings, 1 = retain; xref-dependent visibility settings. */
  fprintf (fp, "  9\n$PLINEGEN\n 70\n     0\n"); /* Governs the generation of linetype patterns around the vertices of a 2D polyline: 1 = linetype is generated in a continuous pattern around vertices of the polyline; 0 = each segment of the polyline starts and ends with a dash. */
  fprintf (fp, "  9\n$PSLTSCALE\n 70\n     1\n"); /* Controls paper space linetype scaling: 1 = no special linetype scaling; 0 = viewport scaling governs linetype scaling. */
  fprintf (fp, "  9\n$TREEDEPTH\n 70\n  3020\n"); /* Specifies the maximum depth of the spatial index. */
  fprintf (fp, "  9\n$PICKSTYLE\n 70\n     1\n"); /* Controls group selection and associative hatch selection: 0= No group selection or associative hatch selection; 1= Group selection; 2 = Associative hatch selection; 3 = Group selection and associative hatch selection. */
  fprintf (fp, "  9\n$CMLSTYLE\n  2\nSTANDARD\n"); /* Current multiline style name. */
  fprintf (fp, "  9\n$CMLJUST\n 70\n     0\n"); /* Current multiline justification: 0 = Top; 1 = Middle; 2 = Bottom. */
  fprintf (fp, "  9\n$CMLSCALE\n 40\n1.0\n"); /* Current multiline scale. */
  fprintf (fp, "  9\n$PROXYGRAPHICS\n 70\n     1\n"); /* Controls the saving of proxy object images. */
  fprintf (fp, "  9\n$MEASUREMENT\n 70\n     0\n"); /* Sets drawing units. 0 = English; 1 = Metric. */
  fprintf (fp, "  0\nENDSEC\n");
  /* write a CLASSES section */
  fprintf (fp, "  0\nSECTION\n");
  fprintf (fp, "  2\nCLASSES\n"); /* Section name. */
  fprintf (fp, "  0\nENDSEC\n");
  /* write a TABLES section */
  fprintf (fp, "  0\nSECTION\n");
  fprintf (fp, "  2\nTABLES\n"); /* Section name. */
  /* write a VPORT (viewport) table entry */
  fprintf (fp, "  0\nTABLE\n");
  fprintf (fp, "  2\nVPORT\n"); /* Table name. */
  fprintf (fp, "  5\n23A\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTable\n");
  fprintf (fp, " 70\n     2\n"); /* Standard flag values. */
  fprintf (fp, "  0\nVPORT\n");
  fprintf (fp, "  5\n261\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbViewportTableRecord\n");
  fprintf (fp, "  2\n*ACTIVE\n"); /* Viewport name. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, " 10\n0.0\n 20\n0.0\n"); /* Lower-left corner of viewport. */
  fprintf (fp, " 11\n1.0\n 21\n1.0\n"); /* Upper-right corner of viewport. */
  fprintf (fp, " 12\n43.998872\n 22\n17.506577\n"); /* View center point (in DCS). */
  fprintf (fp, " 13\n0.0\n 23\n0.0\n"); /* Snap base point. */
  fprintf (fp, " 14\n1.0\n 24\n1.0\n"); /* Snap spacing X and Y. */
  fprintf (fp, " 15\n10.0\n 25\n10.0\n"); /* Grid spacing X and Y. */
  fprintf (fp, " 16\n0.0\n 26\n0.0\n 36\n1.0\n"); /* View direction from target point (in WCS). */
  fprintf (fp, " 17\n0.0\n 27\n0.0\n 37\n0.0\n"); /* View target point (in WCS). */
  fprintf (fp, " 40\n47.164502\n"); /* View height. */
  fprintf (fp, " 41\n1.882514\n"); /* Viewport aspect ratio. */
  fprintf (fp, " 42\n50.0\n"); /* Lens length. */
  fprintf (fp, " 43\n0.0\n"); /* Front clipping plane (offset from target point). */
  fprintf (fp, " 44\n0.0\n"); /* Back clipping plane (offset from target point). */
  fprintf (fp, " 50\n0.0\n"); /* Snap rotation angle. */
  fprintf (fp, " 51\n0.0\n"); /* View twist angle. */
  fprintf (fp, " 71\n     0\n"); /* View mode (see VIEWMODE system variable). */
  fprintf (fp, " 72\n   100\n"); /* Circle zoom percent. */
  fprintf (fp, " 73\n     1\n"); /* Fast zoom setting. */
  fprintf (fp, " 74\n     3\n"); /* UCSICON setting. */
  fprintf (fp, " 75\n     0\n"); /* Snap on/off.*/
  fprintf (fp, " 76\n     0\n"); /* Grid on/off. */
  fprintf (fp, " 77\n     0\n"); /* Snap style. */
  fprintf (fp, " 78\n     0\n"); /* Snap isopair. */
  fprintf (fp, "  0\nENDTAB\n");
  /* write LTYPE (linetype) table entries */
  fprintf (fp, "  0\nTABLE\n");
  fprintf (fp, "  2\nLTYPE\n"); /* Table name. */
  fprintf (fp, "  5\n237\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTable\n");
  fprintf (fp, " 70\n     1\n"); /* Standard flag values. */
  /* write a record entry for a BYBLOCK linetype */
  fprintf (fp, "  0\nLTYPE\n");
  fprintf (fp, "  5\n244\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbLinetypeTableRecord\n");
  fprintf (fp, "  2\nBYBLOCK\n"); /* Linetype name. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, "  3\n\n"); /* Descriptive text for linetype. */
  fprintf (fp, " 72\n    65\n"); /* Alignment code; value is always 65, the ASCII code for A. */
  fprintf (fp, " 73\n     0\n"); /* The number of linetype elements. */
  fprintf (fp, " 40\n0.0\n"); /* Total pattern length. */
  /* write a record entry for a BYLAYER linetype */
  fprintf (fp, "  0\nLTYPE\n");
  fprintf (fp, "  5\n245\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbLinetypeTableRecord\n");
  fprintf (fp, "  2\nBYLAYER\n"); /* Linetype name. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, "  3\n\n"); /* Descriptive text for linetype. */
  fprintf (fp, " 72\n    65\n"); /* Alignment code; value is always 65, the ASCII code for A. */
  fprintf (fp, " 73\n     0\n"); /* The number of linetype elements. */
  fprintf (fp, " 40\n0.0\n"); /* Total pattern length. */
  /* write a record entry for a CONTINUOUS linetype */
  fprintf (fp, "  0\nLTYPE\n");
  fprintf (fp, "  5\n246\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbLinetypeTableRecord\n");
  fprintf (fp, "  2\nCONTINUOUS\n"); /* Linetype name. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, "  3\nSolid line\n"); /* Descriptive text for linetype. */
  fprintf (fp, " 72\n    65\n"); /* Alignment code; value is always 65, the ASCII code for A. */
  fprintf (fp, " 73\n     0\n"); /* The number of linetype elements. */
  fprintf (fp, " 40\n0.0\n"); /* Total pattern length. */
  fprintf (fp, "  0\nENDTAB\n");
  /* write LAYER table entries */
  fprintf (fp, "  0\nTABLE\n");
  fprintf (fp, "  2\nLAYER\n"); /* Table name. */
  fprintf (fp, "  5\n234\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTable\n");
  fprintf (fp, " 70\n     2\n"); /* Standard flag values. */
  /* write a record entry for layer "0" */
  fprintf (fp, "  0\nLAYER\n");
  fprintf (fp, "  5\n240\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbLayerTableRecord\n");
  fprintf (fp, "  2\n0\n"); /* Layer name. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, " 62\n     7\n"); /* Color number (if negative, layer is Off). */
  fprintf (fp, "  6\nCONTINUOUS\n"); /* Linetype name. */
  /* * write a record entry for a layer "ASHADE" */
  fprintf (fp, "  0\nLAYER\n");
  fprintf (fp, "  5\n251\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbLayerTableRecord\n");
  fprintf (fp, "  2\nASHADE\n"); /* Layer name. */
  fprintf (fp, " 70\n     4\n"); /* Standard flag values. */
  fprintf (fp, " 62\n     7\n"); /* Color number (if negative, layer is Off). */
  fprintf (fp, "  6\nCONTINUOUS\n"); /* Linetype name. */
  fprintf (fp, "  0\nENDTAB\n");
  /* write STYLE table entries */
  fprintf (fp, "  0\nTABLE\n");
  fprintf (fp, "  2\nSTYLE\n"); /* Table name. */
  fprintf (fp, "  5\n235\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTable\n");
  fprintf (fp, " 70\n     2\n"); /* Standard flag values. */
  /* write a record entry for a style "STANDARD" */
  fprintf (fp, "  0\nSTYLE\n");
  fprintf (fp, "  5\n241\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbTextStyleTableRecord\n");
  fprintf (fp, "  2\nSTANDARD\n"); /* Style name. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, " 40\n0.0\n"); /* Fixed text height; 0 if not fixed/ */
  fprintf (fp, " 41\n1.0\n"); /* Width factor. */
  fprintf (fp, " 50\n0.0\n"); /* Oblique angle. */
  fprintf (fp, " 71\n     0\n"); /* Text generation flags; 2 = Text is backward (mirrored in X); 4 = Text is upside down (mirrored in Y). */
  fprintf (fp, " 42\n2.5\n"); /* Last height used. */
  fprintf (fp, "  3\ntxt\n"); /* Primary font file name. */
  fprintf (fp, "  4\n\n"); /* Bigfont file name; blank if none. */
  /* write a record entry for a style "ASHADE" */
  fprintf (fp, "  0\nSTYLE\n");
  fprintf (fp, "  5\n252\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbTextStyleTableRecord\n");
  fprintf (fp, "  2\nASHADE\n"); /* Style name. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, " 40\n0.2\n"); /* Fixed text height; 0 if not fixed/ */
  fprintf (fp, " 41\n1.0\n"); /* Width factor. */
  fprintf (fp, " 50\n0.0\n"); /* Oblique angle. */
  fprintf (fp, " 71\n     0\n"); /* Text generation flags; 2 = Text is backward (mirrored in X); 4 = Text is upside down (mirrored in Y). */
  fprintf (fp, " 42\n2.5\n"); /* Last height used. */
  fprintf (fp, "  3\nsimplex.shx\n"); /* Primary font file name. */
  fprintf (fp, "  4\n\n"); /* Bigfont file name; blank if none. */
  fprintf (fp, "  0\nENDTAB\n");
  /* write a VIEW table entry */
  fprintf (fp, "  0\nTABLE\n");
  fprintf (fp, "  2\nVIEW\n"); /* Table name. */
  fprintf (fp, "  5\n238\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTable\n");
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, "  0\nENDTAB\n");
  /* write a UCS (User Coordinate System) table entry */
  fprintf (fp, "  0\nTABLE\n");
  fprintf (fp, "  2\nUCS\n"); /* Table name. */
  fprintf (fp, "  5\n239\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTable\n");
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, "  0\nENDTAB\n");
  /* write a APPID (APPlication ID) table entry */
  fprintf (fp, "  0\nTABLE\n");
  fprintf (fp, "  2\nAPPID\n"); /* Table name. */
  fprintf (fp, "  5\n23B\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTable\n");
  fprintf (fp, " 70\n     6\n"); /* Standard flag values. */
  /* write a record entry for a appid "ACAD" */
  fprintf (fp, "  0\nAPPID\n");
  fprintf (fp, "  5\n242\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbRegAppTableRecord\n");
  fprintf (fp, "  2\nACAD\n"); /* User-supplied (or application-supplied) application name (for extended data). These table entries maintain a set of names for all registered applications. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  /* write a record entry for a appid "AVE_RENDER" */
  fprintf (fp, "  0\nAPPID\n");
  fprintf (fp, "  5\n253\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbRegAppTableRecord\n");
  fprintf (fp, "  2\nAVE_RENDER\n"); /* User-supplied (or application-supplied) application name (for extended data). These table entries maintain a set of names for all registered applications. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  /* write a record entry for a appid "AVE_ENTITY_MATERIAL" */
  fprintf (fp, "  0\nAPPID\n");
  fprintf (fp, "  5\n254\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbRegAppTableRecord\n");
  fprintf (fp, "  2\nAVE_ENTITY_MATERIAL\n"); /* User-supplied (or application-supplied) application name (for extended data). These table entries maintain a set of names for all registered applications. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  /* write a record entry for a appid "AVE_FINISH" */
  fprintf (fp, "  0\nAPPID\n");
  fprintf (fp, "  5\n255\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbRegAppTableRecord\n");
  fprintf (fp, "  2\nAVE_FINISH\n"); /* User-supplied (or application-supplied) application name (for extended data). These table entries maintain a set of names for all registered applications. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  /* write a record entry for a appid "AVE_MATERIAL" */
  fprintf (fp, "  0\nAPPID\n");
  fprintf (fp, "  5\n256\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbRegAppTableRecord\n");
  fprintf (fp, "  2\nAVE_MATERIAL\n"); /* User-supplied (or application-supplied) application name (for extended data). These table entries maintain a set of names for all registered applications. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  /* write a record entry for a appid "AVE_GLOBAL" */
  fprintf (fp, "  0\nAPPID\n");
  fprintf (fp, "  5\n257\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbRegAppTableRecord\n");
  fprintf (fp, "  2\nAVE_GLOBAL\n"); /* User-supplied (or application-supplied) application name (for extended data). These table entries maintain a set of names for all registered applications. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, "  0\nENDTAB\n");
  /* write a DIMSTYLE (DIMensioning STYLE) table entry */
  fprintf (fp, "  0\nTABLE\n");
  fprintf (fp, "  2\nDIMSTYLE\n"); /* Table name. */
  fprintf (fp, "  5\n23C\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTable\n");
  fprintf (fp, " 70\n     1\n"); /* Standard flag values. */
  /* write a record entry for a dimstyle "STANDARD" */
  fprintf (fp, "  0\nDIMSTYLE\n");
  fprintf (fp, "105\n258\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbDimStyleTableRecord\n");
  fprintf (fp, "  2\nSTANDARD\n"); /* Dimension style name. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, "  3\n\n"); /* DIMPOST */
  fprintf (fp, "  4\n\n"); /* DIMAPOST */
  fprintf (fp, "  5\n\n"); /* DIMBLK */
  fprintf (fp, "  6\n\n"); /* DIMBLK1 */
  fprintf (fp, "  7\n\n"); /* DIMBLK2 */
  fprintf (fp, " 40\n1.0\n"); /* DIMSCALE */
  fprintf (fp, " 41\n0.18\n"); /* DIMASZ */
  fprintf (fp, " 42\n0.0625\n"); /* DIMEXO */
  fprintf (fp, " 43\n0.38\n"); /* DIMDLI */
  fprintf (fp, " 44\n0.18\n"); /* DIMEXE */
  fprintf (fp, " 45\n0.0\n"); /* DIMRND */
  fprintf (fp, " 46\n0.0\n"); /* DIMDLE */
  fprintf (fp, " 47\n0.0\n"); /* DIMTP */
  fprintf (fp, " 48\n0.0\n"); /* DIMTM */
  fprintf (fp, "140\n0.18\n"); /* DIMTXT */
  fprintf (fp, "141\n0.09\n"); /* DIMCEN */
  fprintf (fp, "142\n0.0\n"); /* DIMTSZ */
  fprintf (fp, "143\n25.4\n"); /* DIMALTF */
  fprintf (fp, "144\n1.0\n"); /* DIMLFAC */
  fprintf (fp, "145\n0.0\n"); /* DIMTVP */
  fprintf (fp, "146\n1.0\n"); /* DIMTFAC */
  fprintf (fp, "147\n0.09\n"); /* DIMGAP */
  fprintf (fp, " 71\n     0\n"); /* DIMTOL */
  fprintf (fp, " 72\n     0\n"); /* DIMLIM */
  fprintf (fp, " 73\n     1\n"); /* DIMTIH */
  fprintf (fp, " 74\n     1\n"); /* DIMTOH */
  fprintf (fp, " 75\n     0\n"); /* DIMSE1 */
  fprintf (fp, " 76\n     0\n"); /* DIMSE2 */
  fprintf (fp, " 77\n     0\n"); /* DIMTAD */
  fprintf (fp, " 78\n     0\n"); /* DIMZIN */
  fprintf (fp, "170\n     0\n"); /* DIMALT */
  fprintf (fp, "171\n     2\n"); /* DIMALTD */
  fprintf (fp, "172\n     0\n"); /* DIMTOFL */
  fprintf (fp, "173\n     0\n"); /* DIMSAH */
  fprintf (fp, "174\n     0\n"); /* DIMTIX */
  fprintf (fp, "175\n     0\n"); /* DIMSOXD */
  fprintf (fp, "176\n     0\n"); /* DIMDLRD */
  fprintf (fp, "177\n     0\n"); /* DIMCLRE */
  fprintf (fp, "178\n     0\n"); /* DIMCLRT */
  fprintf (fp, "270\n     2\n"); /* DIMUNIT */
  fprintf (fp, "271\n     4\n"); /* DIMDEC */
  fprintf (fp, "272\n     4\n"); /* DIMTDEC */
  fprintf (fp, "273\n     2\n"); /* DIMALTU */
  fprintf (fp, "274\n     2\n"); /* DIMALTTD */
  fprintf (fp, "340\n241\n"); /* Handle of referenced STYLE object (used instead of storing DIMTXSTY value). */
  fprintf (fp, "275\n     0\n"); /* DIMAUNIT */
  fprintf (fp, "280\n     0\n"); /* DIMJUST */
  fprintf (fp, "281\n     0\n"); /* DIMSD1 */
  fprintf (fp, "282\n     0\n"); /* DIMSD2 */
  fprintf (fp, "283\n     1\n"); /* DIMTOLJ */
  fprintf (fp, "284\n     0\n"); /* DIMTZIN */
  fprintf (fp, "285\n     0\n"); /* DIMALTZ */
  fprintf (fp, "286\n     0\n"); /* DIMALTTZ */
  fprintf (fp, "287\n     3\n"); /* DIMFIT */
  fprintf (fp, "288\n     0\n"); /* DIMUPT */
  fprintf (fp, "  0\nENDTAB\n");
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_write_header_imperial_new () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Write DXF output to a file for a metric DXF header.
 *
 * Fall back for if no default metric header template file exists in the
 * pcb/src/hid/dxf/template directory.\n
 * Write down a DXF header from scratch based on metric values.\n
 * Included sections and tables are:\n
 * <ul>
* <li>HEADER section
 * <li>CLASSES section
 * <li>TABLES section
 *   <ul>
 *   <li>VPORT table
 *   <li>LTYPE table
 *   <li>LAYER table
 *   <li>STYLE table
 *   <li>VIEW table
 *   <li>UCS table
 *   <li>APPID table
 *   <li>DIMSTYLE table
 *   </ul>
 * </ul>
 */
static void
dxf_write_header_metric_new ()
{
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_write_header_metric_new () function.\n", __FILE__, __LINE__);
#endif
  /* write a metric HEADER section */
  fprintf (fp, "  0\nSECTION\n");
  fprintf (fp, "  2\nHEADER\n"); /* Section name. */
  fprintf (fp, "  9\n$ACADVER\n  1\nAC1014\n"); /* AutoCAD drawing database version number. */
  fprintf (fp, "  9\n$ACADMAINTVER\n 70\n     0\n"); /* Maintenance version number. */
  fprintf (fp, "  9\n$DWGCODEPAGE\n  3\nANSI_1252\n"); /* Drawing code page. */
  fprintf (fp, "  9\n$INSBASE\n 10\n0.0\n 20\n0.0\n 30\n0.0\n"); /* Insertion base set by BASE command (in WCS). */
  fprintf (fp, "  9\n$EXTMIN\n 10\n0.0\n 20\n0.0\n 30\n0.0\n"); /* X, Y, and Z drawing extents lower-left corner (in WCS). */
  fprintf (fp, "  9\n$EXTMAX\n 10\n88.01056\n 20\n35.022217\n 30\n0.0\n"); /* X, Y, and Z drawing extents upper-right corner (in WCS). */
  fprintf (fp, "  9\n$LIMMIN\n 10\n0.0\n 20\n0.0\n"); /* XY drawing limits lower-left corner (in WCS). */
  fprintf (fp, "  9\n$LIMMAX\n 10\n420.0\n 20\n297.0\n"); /* XY drawing limits upper-right corner (in WCS). */
  fprintf (fp, "  9\n$ORTHOMODE\n 70\n     0\n"); /* Ortho mode on if nonzero. */
  fprintf (fp, "  9\n$REGENMODE\n 70\n     1\n"); /* REGENAUTO mode on if nonzero. */
  fprintf (fp, "  9\n$FILLMODE\n 70\n     1\n"); /* Fill mode on if nonzero. */
  fprintf (fp, "  9\n$QTEXTMODE\n 70\n     0\n"); /* Quick text mode on if nonzero. */
  fprintf (fp, "  9\n$MIRRTEXT\n 70\n     1\n"); /* Mirror text if nonzero. */
  fprintf (fp, "  9\n$DRAGMODE\n 70\n     2\n"); /* 0 = off, 1 = on, 2 = auto. */
  fprintf (fp, "  9\n$LTSCALE\n 40\n1.0\n"); /* Global linetype scale. */
  fprintf (fp, "  9\n$OSMODE\n 70\n   125\n"); /* Running object snap modes. */
  fprintf (fp, "  9\n$ATTMODE\n 70\n     1\n"); /* Attribute visibility: 0 = none, 1 = normal, 2 = all. */
  fprintf (fp, "  9\n$TEXTSIZE\n 40\n2.5\n"); /* Default text height = 2.5 mm. */
  fprintf (fp, "  9\n$TRACEWID\n 40\n1.0\n"); /* Default trace width. */
  fprintf (fp, "  9\n$TEXTSTYLE\n  7\nSTANDARD\n"); /* Current text style name. */
  fprintf (fp, "  9\n$CLAYER\n  8\n0\n"); /* Current layer name. */
  fprintf (fp, "  9\n$CELTYPE\n  6\nBYLAYER\n"); /* Entity linetype name, or BYBLOCK or BYLAYER. */
  fprintf (fp, "  9\n$CECOLOR\n 62\n   256\n"); /* Current entity color number: 0 = BYBLOCK, 256 = BYLAYER. */
  fprintf (fp, "  9\n$CELTSCALE\n 40\n1.0\n"); /* Current entity linetype scale. */
  fprintf (fp, "  9\n$DELOBJ\n 70\n     1\n"); /* Controls object deletion: 0 = deleted, 1 = retained. */
  fprintf (fp, "  9\n$DISPSILH\n 70\n     0\n"); /* Controls the display of silhouette curves of body objects in wire-frame mode: 0 = Off, 1 = On. */
  fprintf (fp, "  9\n$DIMSCALE\n 40\n1.0\n"); /* Overall dimensioning scale factor. */
  fprintf (fp, "  9\n$DIMASZ\n 40\n2.5\n"); /* Dimensioning arrow size = 2.5 mm. */
  fprintf (fp, "  9\n$DIMEXO\n 40\n0.625\n"); /* Extension line offset = 0.625 mm. */
  fprintf (fp, "  9\n$DIMDLI\n 40\n3.75\n"); /* Dimension line increment = 3.75 mm. */
  fprintf (fp, "  9\n$DIMRND\n 40\n0.0\n"); /* Rounding value for dimension distances. */
  fprintf (fp, "  9\n$DIMDLE\n 40\n0.0\n"); /* Dimension line extension */
  fprintf (fp, "  9\n$DIMEXE\n 40\n1.25\n"); /* Extension line extension = 1.25 mm. */
  fprintf (fp, "  9\n$DIMTP\n 40\n0.0\n"); /* Plus tolerance. */
  fprintf (fp, "  9\n$DIMTM\n 40\n0.0\n"); /* Minus tolerance. */
  fprintf (fp, "  9\n$DIMTXT\n 40\n2.5\n"); /* Dimensioning text height = 2.5 mm. */
  fprintf (fp, "  9\n$DIMCEN\n 40\n2.5\n"); /* Size of center mark/lines = 2.5 mm. */
  fprintf (fp, "  9\n$DIMTSZ\n 40\n0.0\n"); /* Dimensioning tick size; 0 = no ticks. */
  fprintf (fp, "  9\n$DIMTOL\n 70\n     0\n"); /* Dimension tolerances generated if nonzero. */
  fprintf (fp, "  9\n$DIMLIM\n 70\n     0\n"); /* Dimension limits generated if nonzero. */
  fprintf (fp, "  9\n$DIMTIH\n 70\n     0\n"); /* Text inside horizontal if nonzero. */
  fprintf (fp, "  9\n$DIMTOH\n 70\n     0\n"); /* Text outside horizontal if nonzero. */
  fprintf (fp, "  9\n$DIMSE1\n 70\n     0\n"); /* First extension line suppressed if nonzero. */
  fprintf (fp, "  9\n$DIMSE2\n 70\n     0\n"); /* Second extension line suppressed if nonzero. */
  fprintf (fp, "  9\n$DIMTAD\n 70\n     1\n"); /* Text above dimension line if nonzero. */
  fprintf (fp, "  9\n$DIMZIN\n 70\n     8\n"); /* Controls suppression of zeros for primary unit values. */
  fprintf (fp, "  9\n$DIMBLK\n  1\n\n"); /* Arrow block name. */
  fprintf (fp, "  9\n$DIMASO\n 70\n     1\n"); /* 1 = create associative dimensioning, 0 = draw individual entities. */
  fprintf (fp, "  9\n$DIMSHO\n 70\n     1\n"); /* 1 = Recompute dimensions while dragging, 0 = drag original image. */
  fprintf (fp, "  9\n$DIMPOST\n  1\n\n"); /* General dimensioning suffix. */
  fprintf (fp, "  9\n$DIMAPOST\n  1\n\n"); /* Alternate dimensioning suffix. */
  fprintf (fp, "  9\n$DIMALT\n 70\n     0\n"); /* Alternate unit dimensioning performed if nonzero. */
  fprintf (fp, "  9\n$DIMALTD\n 70\n     4\n") /* Alternate unit decimal places. */;
  fprintf (fp, "  9\n$DIMALTF\n 40\n0.0394\n") /* Alternate unit scale factor (mm --> mil). */;
  fprintf (fp, "  9\n$DIMLFAC\n 40\n1.0\n"); /* Linear measurements scale factor. */
  fprintf (fp, "  9\n$DIMTOFL\n 70\n     1\n"); /* If text outside extensions, force line extensions between extensions if nonzero. */
  fprintf (fp, "  9\n$DIMTVP\n 40\n0.0\n"); /* Text vertical position. */
  fprintf (fp, "  9\n$DIMTIX\n 70\n     0\n"); /* Force text inside extensions if nonzero. */
  fprintf (fp, "  9\n$DIMSOXD\n 70\n     0\n"); /* Suppress outside-extensions dimension lines if nonzero. */
  fprintf (fp, "  9\n$DIMSAH\n 70\n     0\n"); /* Use separate arrow blocks if nonzero. */
  fprintf (fp, "  9\n$DIMBLK1\n  1\n\n"); /* First arrow block name. */
  fprintf (fp, "  9\n$DIMBLK2\n  1\n\n"); /* Second arrow block name. */
  fprintf (fp, "  9\n$DIMSTYLE\n  2\nSTANDARD\n"); /* Dimension style name. */
  fprintf (fp, "  9\n$DIMCLRD\n 70\n     0\n"); /* Dimension line color: range is 0 = BYBLOCK, 256 = BYLAYER. */
  fprintf (fp, "  9\n$DIMCLRE\n 70\n     0\n"); /* Dimension extension line color:  range is 0 = BYBLOCK, 256 = BYLAYER. */
  fprintf (fp, "  9\n$DIMCLRT\n 70\n     0\n"); /* Dimension text color:  range is 0 = BYBLOCK, 256 = BYLAYER. */
  fprintf (fp, "  9\n$DIMTFAC\n 40\n1.0\n"); /* Dimension tolerance display scale factor. */
  fprintf (fp, "  9\n$DIMGAP\n 40\n0.625\n") /* Dimension line gap. */;
  fprintf (fp, "  9\n$DIMJUST\n 70\n     0\n"); /* Horizontal dimension text position: 0 = above dimension line and center-justified between extension lines, 1 = above dimension line and next to first extension line, 2=above dimension line and next to second extension line, 3 = above and center-justified to first extension line, 4 = above and center-justified to second extension line. */
  fprintf (fp, "  9\n$DIMSD1\n 70\n     0\n"); /* Suppression of first extension line: 0 = not suppressed, 1 = suppressed. */
  fprintf (fp, "  9\n$DIMSD2\n 70\n     0\n"); /* Suppression of second extension line: 0 = not suppressed, 1 = suppressed. */
  fprintf (fp, "  9\n$DIMTOLJ\n 70\n     1\n"); /* Vertical justification for tolerance values: 0 = Top, 1 = Middle, 2 = Bottom. */
  fprintf (fp, "  9\n$DIMTZIN\n 70\n     0\n"); /* Controls suppression of zeros for tolerance values: 0 = Suppresses zero feet and precisely zero inches; 1 = Includes zero feet and precisely zero inches; 2 = Includes zero feet and suppresses zero inches; 3 = Includes zero inches and suppresses zero feet. */
  fprintf (fp, "  9\n$DIMALTZ\n 70\n     0\n"); /* Controls suppression of zeros for alternate unit dimension values: 0 = Suppresses zero feet and precisely zero inches; 1 = Includes zero feet and precisely zero inches; 2 = Includes zero feet and suppresses zero inches; 3 = Includes zero inches and suppresses zero feet. */
  fprintf (fp, "  9\n$DIMALTTZ\n 70\n     0\n") /* Controls suppression of zeros for alternate tolerance values: 0 = Suppresses zero feet and precisely zero inches; 1 = Includes zero feet and precisely zero inches; 2 = Includes zero feet and suppresses zero inches; 3 = Includes zero inches and suppresses zero feet. */;
  fprintf (fp, "  9\n$DIMFIT\n 70\n     3\n"); /* Placement of text and arrowheads; Possible values: 0 through 3. */
  fprintf (fp, "  9\n$DIMUPT\n 70\n     0\n"); /* Cursor functionality for user positioned text:  0=controls only the dimension line location, 1=controls the text position as well as the dimension line location. */
  fprintf (fp, "  9\n$DIMUNIT\n 70\n     2\n"); /* Units format for all dimension style family members except angular: 1 = Scientific; 2 = Decimal; 3 = Engineering; 4 = Architectural (stacked); 5 = Fractional (stacked); 6 = Architectural; 7 = Fractional. */
  fprintf (fp, "  9\n$DIMDEC\n 70\n     4\n"); /* Number of decimal places for the tolerance values of a primary units dimension. */
  fprintf (fp, "  9\n$DIMTDEC\n 70\n     4\n"); /* Number of decimal places to display the tolerance values. */
  fprintf (fp, "  9\n$DIMALTU\n 70\n     2\n"); /*Units format for alternate units of all dimension style family members except angular: 1 = Scientific; 2 = Decimal; 3 = Engineering; 4 = Architectural (stacked); 5 = Fractional (stacked); 6 = Architectural; 7 = Fractional. */
  fprintf (fp, "  9\n$DIMALTTD\n 70\n     2\n"); /*Number of decimal places for tolerance values of an alternate units dimension. */
  fprintf (fp, "  9\n$DIMTXSTY\n  7\nSTANDARD\n"); /* Dimension text style. */
  fprintf (fp, "  9\n$DIMAUNIT\n 70\n     0\n"); /* Angle format for angular dimensions: 0 = Decimal degrees, 1 = Degrees/minutes/seconds, 2 = Gradians, 3=Radians, 4 = Surveyor's units. */
  fprintf (fp, "  9\n$LUNITS\n 70\n     2\n"); /* Units format for coordinates and distances. */
  fprintf (fp, "  9\n$LUPREC\n 70\n     4\n"); /* Units precision for coordinates and distances. */
  fprintf (fp, "  9\n$SKETCHINC\n 40\n1.0\n"); /* Sketch record increment. */
  fprintf (fp, "  9\n$FILLETRAD\n 40\n1.0\n"); /* Fillet radius. */
  fprintf (fp, "  9\n$AUNITS\n 70\n     0\n"); /* Units format for angles. */
  fprintf (fp, "  9\n$AUPREC\n 70\n     0\n"); /* Units precision for angles. */
  fprintf (fp, "  9\n$MENU\n  1\n.\n"); /* Name of menu file. */
  fprintf (fp, "  9\n$ELEVATION\n 40\n0.0\n"); /* Current elevation set by ELEV command. */
  fprintf (fp, "  9\n$PELEVATION\n 40\n0.0\n") /* Current paper space elevation. */;
  fprintf (fp, "  9\n$THICKNESS\n 40\n0.0\n"); /* Current thickness set by ELEV command. */
  fprintf (fp, "  9\n$LIMCHECK\n 70\n     0\n"); /* Nonzero if limits checking is on. */
  fprintf (fp, "  9\n$BLIPMODE\n 70\n     0\n"); /* Blip mode on if nonzero. */
  fprintf (fp, "  9\n$CHAMFERA\n 40\n10.0\n"); /* First chamfer distance. */
  fprintf (fp, "  9\n$CHAMFERB\n 40\n10.0\n"); /* Second chamfer distance. */
  fprintf (fp, "  9\n$CHAMFERC\n 40\n0.0\n"); /* Chamfer length. */
  fprintf (fp, "  9\n$CHAMFERD\n 40\n0.0\n"); /* Chamfer angle. */
  fprintf (fp, "  9\n$SKPOLY\n 70\n     0\n"); /* 0 = sketch lines, 1 = sketch polylines. */
  fprintf (fp, "  9\n$TDCREATE\n 40\n0.0\n"); /* Date/time of drawing creation. */
  fprintf (fp, "  9\n$TDUPDATE\n 40\n0.0\n"); /* Date/time of last drawing update. */
  fprintf (fp, "  9\n$TDINDWG\n 40\n0.0\n"); /* Cumulative editing time for this drawing. */
  fprintf (fp, "  9\n$TDUSRTIMER\n 40\n0.0\n"); /* User elapsed timer. */
  fprintf (fp, "  9\n$USRTIMER\n 70\n     1\n"); /* 0 = timer off, 1 = timer on. */
  fprintf (fp, "  9\n$ANGBASE\n 50\n0.0\n"); /* Angle 0 direction . */
  fprintf (fp, "  9\n$ANGDIR\n 70\n     0\n"); /* 1 = clockwise angles, 0 = counterclockwise. */
  fprintf (fp, "  9\n$PDMODE\n 70\n    98\n"); /* Point display mode. */
  fprintf (fp, "  9\n$PDSIZE\n 40\n0.0\n"); /* Point display size. */
  fprintf (fp, "  9\n$PLINEWID\n 40\n0.0\n"); /* Default polyline width. */
  fprintf (fp, "  9\n$COORDS\n 70\n     2\n"); /* Coordinate display: 0 = static, 1 = continuous update, 2 = "d<a" format. */
  fprintf (fp, "  9\n$SPLFRAME\n 70\n     0\n"); /* Spline control polygon display: 1 = on, 0 = off. */
  fprintf (fp, "  9\n$SPLINETYPE\n 70\n     6\n"); /* Spline curve type for PEDIT Spline. */
  fprintf (fp, "  9\n$SPLINESEGS\n 70\n     8\n"); /* Number of line segments per spline patch. */
  fprintf (fp, "  9\n$ATTDIA\n 70\n     0\n"); /* Attribute entry dialogs: 1 = on, 0 = off. */
  fprintf (fp, "  9\n$ATTREQ\n 70\n     1\n"); /* Attribute prompting during INSERT: 1 = on, 0 = off. */
  fprintf (fp, "  9\n$HANDLING\n 70\n     1\n"); /* Next available handle. */
  fprintf (fp, "  9\n$HANDSEED\n  5\n262\n"); /* Next available handle. */
  fprintf (fp, "  9\n$SURFTAB1\n 70\n     6\n"); /* Number of mesh tabulations in first direction. */
  fprintf (fp, "  9\n$SURFTAB2\n 70\n     6\n"); /* Number of mesh tabulations in second direction. */
  fprintf (fp, "  9\n$SURFTYPE\n 70\n     6\n"); /* Surface type for PEDIT Smooth. */
  fprintf (fp, "  9\n$SURFU\n 70\n     6\n"); /* Surface density (for PEDIT Smooth) in M direction. */
  fprintf (fp, "  9\n$SURFV\n 70\n     6\n"); /* Surface density (for PEDIT Smooth) in N direction. */
  fprintf (fp, "  9\n$UCSNAME\n  2\n\n"); /* Name of current UCS. */
  fprintf (fp, "  9\n$UCSORG\n 10\n0.0\n 20\n0.0\n 30\n0.0\n"); /* Origin of current UCS (in WCS). */
  fprintf (fp, "  9\n$UCSXDIR\n 10\n1.0\n 20\n0.0\n 30\n0.0\n"); /* Direction of current UCS's X axis (in WCS). */
  fprintf (fp, "  9\n$UCSYDIR\n 10\n0.0\n 20\n1.0\n 30\n0.0\n"); /* Direction of current UCS's Y axis (in WCS). */
  fprintf (fp, "  9\n$PUCSNAME\n  2\n\n"); /* Current paper space UCS name. */
  fprintf (fp, "  9\n$PUCSORG\n 10\n0.0\n 20\n0.0\n 30\n0.0\n"); /* Current paper space UCS origin. */
  fprintf (fp, "  9\n$PUCSXDIR\n 10\n1.0\n 20\n0.0\n 30\n0.0\n"); /* Current paper space UCS X axis. */
  fprintf (fp, "  9\n$PUCSYDIR\n 10\n0.0\n 20\n1.0\n 30\n0.0\n"); /* Current paper space UCS Y axis. */
  fprintf (fp, "  9\n$USERI1\n 70\n     0\n"); /* Five integer variables intended for use by third-party developers. */
  fprintf (fp, "  9\n$USERI2\n 70\n     0\n");
  fprintf (fp, "  9\n$USERI3\n 70\n     0\n");
  fprintf (fp, "  9\n$USERI4\n 70\n     0\n");
  fprintf (fp, "  9\n$USERI5\n 70\n     0\n");
  fprintf (fp, "  9\n$USERR1\n 40\n0.0\n"); /* Five real variables intended for use by third-party developers. */
  fprintf (fp, "  9\n$USERR2\n 40\n0.0\n");
  fprintf (fp, "  9\n$USERR3\n 40\n0.0\n");
  fprintf (fp, "  9\n$USERR4\n 40\n0.0\n");
  fprintf (fp, "  9\n$USERR5\n 40\n0.0\n");
  fprintf (fp, "  9\n$WORLDVIEW\n 70\n     1\n"); /*1 = set UCS to WCS during DVIEW/VPOINT, 0 = don't change UCS. */
  fprintf (fp, "  9\n$SHADEDGE\n 70\n     3\n"); /* 0 = faces shaded, edges not highlighted; 1 = faces shaded, edges highlighted in black; 2 = faces not filled, edges in entity color; 3 = faces in entity color, edges in black. */
  fprintf (fp, "  9\n$SHADEDIF\n 70\n    70\n"); /* Percent ambient/diffuse light, range 1-100, default 70. */
  fprintf (fp, "  9\n$TILEMODE\n 70\n     1\n"); /* 1 for previous release compatibility mode, 0 otherwise. */
  fprintf (fp, "  9\n$MAXACTVP\n 70\n    48\n"); /* Sets maximum number of viewports to be regenerated. */
  fprintf (fp, "  9\n$PINSBASE\n 10\n0.0\n 20\n0.0\n 30\n0.0\n"); /* Paper space insertion base point. */
  fprintf (fp, "  9\n$PLIMCHECK\n 70\n     0\n"); /* Limits checking in paper space when nonzero. */
  fprintf (fp, "  9\n$PEXTMIN\n 10\n1.000000E+20\n 20\n1.000000E+20\n 30\n1.000000E+20\n");  /* Minimum X, Y, and Z extents for paper space. */
  fprintf (fp, "  9\n$PEXTMAX\n 10\n-1.000000E+20\n 20\n-1.000000E+20\n 30\n-1.000000E+20\n"); /* Maximum X, Y, and Z extents for paper space. */
  fprintf (fp, "  9\n$PLIMMIN\n 10\n0.0\n 20\n0.0\n"); /* Minimum X and Y limits in paper space. */
  fprintf (fp, "  9\n$PLIMMAX\n 10\n420.0\n 20\n297.0\n"); /* Maximum X and Y limits in paper space. */
  fprintf (fp, "  9\n$UNITMODE\n 70\n     0\n"); /* Low bit set = display fractions, feet-and-inches, and surveyor's angles in input format. */
  fprintf (fp, "  9\n$VISRETAIN\n 70\n     1\n"); /* 0 = don't retain xref-dependent visibility settings, 1 = retain; xref-dependent visibility settings. */
  fprintf (fp, "  9\n$PLINEGEN\n 70\n     0\n"); /* Governs the generation of linetype patterns around the vertices of a 2D polyline: 1 = linetype is generated in a continuous pattern around vertices of the polyline; 0 = each segment of the polyline starts and ends with a dash. */
  fprintf (fp, "  9\n$PSLTSCALE\n 70\n     1\n"); /* Controls paper space linetype scaling: 1 = no special linetype scaling; 0 = viewport scaling governs linetype scaling. */
  fprintf (fp, "  9\n$TREEDEPTH\n 70\n  3020\n"); /* Specifies the maximum depth of the spatial index. */
  fprintf (fp, "  9\n$PICKSTYLE\n 70\n     1\n"); /* Controls group selection and associative hatch selection: 0= No group selection or associative hatch selection; 1= Group selection; 2 = Associative hatch selection; 3 = Group selection and associative hatch selection. */
  fprintf (fp, "  9\n$CMLSTYLE\n  2\nSTANDARD\n"); /* Current multiline style name. */
  fprintf (fp, "  9\n$CMLJUST\n 70\n     0\n"); /* Current multiline justification: 0 = Top; 1 = Middle; 2 = Bottom. */
  fprintf (fp, "  9\n$CMLSCALE\n 40\n1.0\n"); /* Current multiline scale. */
  fprintf (fp, "  9\n$PROXYGRAPHICS\n 70\n     1\n"); /* Controls the saving of proxy object images. */
  fprintf (fp, "  9\n$MEASUREMENT\n 70\n     0\n"); /* Sets drawing units. 0 = Imperial; 1 = Metric. */
  fprintf (fp, "  0\nENDSEC\n");
  /* write a CLASSES section */
  fprintf (fp, "  0\nSECTION\n");
  fprintf (fp, "  2\nCLASSES\n"); /* Section name. */
  fprintf (fp, "  0\nENDSEC\n");
  /* write a TABLES section */
  fprintf (fp, "  0\nSECTION\n");
  fprintf (fp, "  2\nTABLES\n"); /* Section name. */
  /* write a VPORT (viewport) table entry */
  fprintf (fp, "  0\nTABLE\n");
  fprintf (fp, "  2\nVPORT\n"); /* Table name. */
  fprintf (fp, "  5\n23A\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTable\n");
  fprintf (fp, " 70\n     2\n"); /* Standard flag values. */
  fprintf (fp, "  0\nVPORT\n");
  fprintf (fp, "  5\n261\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbViewportTableRecord\n");
  fprintf (fp, "  2\n*ACTIVE\n"); /* Viewport name. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, " 10\n0.0\n 20\n0.0\n"); /* Lower-left corner of viewport. */
  fprintf (fp, " 11\n1.0\n 21\n1.0\n"); /* Upper-right corner of viewport. */
  fprintf (fp, " 12\n43.998872\n 22\n17.506577\n"); /* View center point (in DCS). */
  fprintf (fp, " 13\n0.0\n 23\n0.0\n"); /* Snap base point. */
  fprintf (fp, " 14\n1.0\n 24\n1.0\n"); /* Snap spacing X and Y. */
  fprintf (fp, " 15\n10.0\n 25\n10.0\n"); /* Grid spacing X and Y. */
  fprintf (fp, " 16\n0.0\n 26\n0.0\n 36\n1.0\n"); /* View direction from target point (in WCS). */
  fprintf (fp, " 17\n0.0\n 27\n0.0\n 37\n0.0\n"); /* View target point (in WCS). */
  fprintf (fp, " 40\n47.164502\n"); /* View height. */
  fprintf (fp, " 41\n1.882514\n"); /* Viewport aspect ratio. */
  fprintf (fp, " 42\n50.0\n"); /* Lens length. */
  fprintf (fp, " 43\n0.0\n"); /* Front clipping plane (offset from target point). */
  fprintf (fp, " 44\n0.0\n"); /* Back clipping plane (offset from target point). */
  fprintf (fp, " 50\n0.0\n"); /* Snap rotation angle. */
  fprintf (fp, " 51\n0.0\n"); /* View twist angle. */
  fprintf (fp, " 71\n     0\n"); /* View mode (see VIEWMODE system variable). */
  fprintf (fp, " 72\n   100\n"); /* Circle zoom percent. */
  fprintf (fp, " 73\n     1\n"); /* Fast zoom setting. */
  fprintf (fp, " 74\n     3\n"); /* UCSICON setting. */
  fprintf (fp, " 75\n     0\n"); /* Snap on/off.*/
  fprintf (fp, " 76\n     0\n"); /* Grid on/off. */
  fprintf (fp, " 77\n     0\n"); /* Snap style. */
  fprintf (fp, " 78\n     0\n"); /* Snap isopair. */
  fprintf (fp, "  0\nENDTAB\n");
  /* write LTYPE (linetype) table entries */
  fprintf (fp, "  0\nTABLE\n");
  fprintf (fp, "  2\nLTYPE\n"); /* Table name. */
  fprintf (fp, "  5\n237\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTable\n");
  fprintf (fp, " 70\n     1\n"); /* Standard flag values. */
  /* write a record entry for a BYBLOCK linetype */
  fprintf (fp, "  0\nLTYPE\n");
  fprintf (fp, "  5\n244\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbLinetypeTableRecord\n");
  fprintf (fp, "  2\nBYBLOCK\n"); /* Linetype name. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, "  3\n\n"); /* Descriptive text for linetype. */
  fprintf (fp, " 72\n    65\n"); /* Alignment code; value is always 65, the ASCII code for A. */
  fprintf (fp, " 73\n     0\n"); /* The number of linetype elements. */
  fprintf (fp, " 40\n0.0\n"); /* Total pattern length. */
  /* write a record entry for a BYLAYER linetype */
  fprintf (fp, "  0\nLTYPE\n");
  fprintf (fp, "  5\n245\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbLinetypeTableRecord\n");
  fprintf (fp, "  2\nBYLAYER\n"); /* Linetype name. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, "  3\n\n"); /* Descriptive text for linetype. */
  fprintf (fp, " 72\n    65\n"); /* Alignment code; value is always 65, the ASCII code for A. */
  fprintf (fp, " 73\n     0\n"); /* The number of linetype elements. */
  fprintf (fp, " 40\n0.0\n"); /* Total pattern length. */
  /* write a record entry for a CONTINUOUS linetype */
  fprintf (fp, "  0\nLTYPE\n");
  fprintf (fp, "  5\n246\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbLinetypeTableRecord\n");
  fprintf (fp, "  2\nCONTINUOUS\n"); /* Linetype name. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, "  3\nSolid line\n"); /* Descriptive text for linetype. */
  fprintf (fp, " 72\n    65\n"); /* Alignment code; value is always 65, the ASCII code for A. */
  fprintf (fp, " 73\n     0\n"); /* The number of linetype elements. */
  fprintf (fp, " 40\n0.0\n"); /* Total pattern length. */
  fprintf (fp, "  0\nENDTAB\n");
  /* write LAYER table entries */
  fprintf (fp, "  0\nTABLE\n");
  fprintf (fp, "  2\nLAYER\n"); /* Table name. */
  fprintf (fp, "  5\n234\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTable\n");
  fprintf (fp, " 70\n     2\n"); /* Standard flag values. */
  /* write a record entry for layer "0" */
  fprintf (fp, "  0\nLAYER\n");
  fprintf (fp, "  5\n240\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbLayerTableRecord\n");
  fprintf (fp, "  2\n0\n"); /* Layer name. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, " 62\n     7\n"); /* Color number (if negative, layer is Off). */
  fprintf (fp, "  6\nCONTINUOUS\n"); /* Linetype name. */
  /* * write a record entry for a layer "ASHADE" */
  fprintf (fp, "  0\nLAYER\n");
  fprintf (fp, "  5\n251\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbLayerTableRecord\n");
  fprintf (fp, "  2\nASHADE\n"); /* Layer name. */
  fprintf (fp, " 70\n     4\n"); /* Standard flag values. */
  fprintf (fp, " 62\n     7\n"); /* Color number (if negative, layer is Off). */
  fprintf (fp, "  6\nCONTINUOUS\n"); /* Linetype name. */
  fprintf (fp, "  0\nENDTAB\n");
  /* write STYLE table entries */
  fprintf (fp, "  0\nTABLE\n");
  fprintf (fp, "  2\nSTYLE\n"); /* Table name. */
  fprintf (fp, "  5\n235\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTable\n");
  fprintf (fp, " 70\n     2\n"); /* Standard flag values. */
  /* write a record entry for a style "STANDARD" */
  fprintf (fp, "  0\nSTYLE\n");
  fprintf (fp, "  5\n241\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbTextStyleTableRecord\n");
  fprintf (fp, "  2\nSTANDARD\n"); /* Style name. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, " 40\n0.0\n"); /* Fixed text height; 0 if not fixed/ */
  fprintf (fp, " 41\n1.0\n"); /* Width factor. */
  fprintf (fp, " 50\n0.0\n"); /* Oblique angle. */
  fprintf (fp, " 71\n     0\n"); /* Text generation flags; 2 = Text is backward (mirrored in X); 4 = Text is upside down (mirrored in Y). */
  fprintf (fp, " 42\n2.5\n"); /* Last height used. */
  fprintf (fp, "  3\ntxt\n"); /* Primary font file name. */
  fprintf (fp, "  4\n\n"); /* Bigfont file name; blank if none. */
  /* write a record entry for a style "ASHADE" */
  fprintf (fp, "  0\nSTYLE\n");
  fprintf (fp, "  5\n252\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbTextStyleTableRecord\n");
  fprintf (fp, "  2\nASHADE\n"); /* Style name. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, " 40\n0.2\n"); /* Fixed text height; 0 if not fixed/ */
  fprintf (fp, " 41\n1.0\n"); /* Width factor. */
  fprintf (fp, " 50\n0.0\n"); /* Oblique angle. */
  fprintf (fp, " 71\n     0\n"); /* Text generation flags; 2 = Text is backward (mirrored in X); 4 = Text is upside down (mirrored in Y). */
  fprintf (fp, " 42\n2.5\n"); /* Last height used. */
  fprintf (fp, "  3\nsimplex.shx\n"); /* Primary font file name. */
  fprintf (fp, "  4\n\n"); /* Bigfont file name; blank if none. */
  fprintf (fp, "  0\nENDTAB\n");
  /* write a VIEW table entry */
  fprintf (fp, "  0\nTABLE\n");
  fprintf (fp, "  2\nVIEW\n"); /* Table name. */
  fprintf (fp, "  5\n238\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTable\n");
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, "  0\nENDTAB\n");
  /* write a UCS (User Coordinate System) table entry */
  fprintf (fp, "  0\nTABLE\n");
  fprintf (fp, "  2\nUCS\n"); /* Table name. */
  fprintf (fp, "  5\n239\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTable\n");
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, "  0\nENDTAB\n");
  /* write a APPID (APPlication ID) table entry */
  fprintf (fp, "  0\nTABLE\n");
  fprintf (fp, "  2\nAPPID\n"); /* Table name. */
  fprintf (fp, "  5\n23B\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTable\n");
  fprintf (fp, " 70\n     6\n"); /* Standard flag values. */
  /* write a record entry for a appid "ACAD" */
  fprintf (fp, "  0\nAPPID\n");
  fprintf (fp, "  5\n242\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbRegAppTableRecord\n");
  fprintf (fp, "  2\nACAD\n"); /* User-supplied (or application-supplied) application name (for extended data). These table entries maintain a set of names for all registered applications. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  /* write a record entry for a appid "AVE_RENDER" */
  fprintf (fp, "  0\nAPPID\n");
  fprintf (fp, "  5\n253\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbRegAppTableRecord\n");
  fprintf (fp, "  2\nAVE_RENDER\n"); /* User-supplied (or application-supplied) application name (for extended data). These table entries maintain a set of names for all registered applications. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  /* write a record entry for a appid "AVE_ENTITY_MATERIAL" */
  fprintf (fp, "  0\nAPPID\n");
  fprintf (fp, "  5\n254\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbRegAppTableRecord\n");
  fprintf (fp, "  2\nAVE_ENTITY_MATERIAL\n"); /* User-supplied (or application-supplied) application name (for extended data). These table entries maintain a set of names for all registered applications. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  /* write a record entry for a appid "AVE_FINISH" */
  fprintf (fp, "  0\nAPPID\n");
  fprintf (fp, "  5\n255\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbRegAppTableRecord\n");
  fprintf (fp, "  2\nAVE_FINISH\n"); /* User-supplied (or application-supplied) application name (for extended data). These table entries maintain a set of names for all registered applications. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  /* write a record entry for a appid "AVE_MATERIAL" */
  fprintf (fp, "  0\nAPPID\n");
  fprintf (fp, "  5\n256\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbRegAppTableRecord\n");
  fprintf (fp, "  2\nAVE_MATERIAL\n"); /* User-supplied (or application-supplied) application name (for extended data). These table entries maintain a set of names for all registered applications. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  /* write a record entry for a appid "AVE_GLOBAL" */
  fprintf (fp, "  0\nAPPID\n");
  fprintf (fp, "  5\n257\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbRegAppTableRecord\n");
  fprintf (fp, "  2\nAVE_GLOBAL\n"); /* User-supplied (or application-supplied) application name (for extended data). These table entries maintain a set of names for all registered applications. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, "  0\nENDTAB\n");
  /* write a DIMSTYLE (DIMensioning STYLE) table entry */
  fprintf (fp, "  0\nTABLE\n");
  fprintf (fp, "  2\nDIMSTYLE\n"); /* Table name. */
  fprintf (fp, "  5\n23C\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTable\n");
  fprintf (fp, " 70\n     1\n"); /* Standard flag values. */
  /* write a record entry for a dimstyle "STANDARD" */
  fprintf (fp, "  0\nDIMSTYLE\n");
  fprintf (fp, "105\n258\n"); /* Handle. */
  fprintf (fp, "100\nAcDbSymbolTableRecord\n");
  fprintf (fp, "100\nAcDbDimStyleTableRecord\n");
  fprintf (fp, "  2\nSTANDARD\n"); /* Dimension style name. */
  fprintf (fp, " 70\n     0\n"); /* Standard flag values. */
  fprintf (fp, "  3\n\n"); /* DIMPOST */
  fprintf (fp, "  4\n\n"); /* DIMAPOST */
  fprintf (fp, "  5\n\n"); /* DIMBLK */
  fprintf (fp, "  6\n\n"); /* DIMBLK1 */
  fprintf (fp, "  7\n\n"); /* DIMBLK2 */
  fprintf (fp, " 40\n1.0\n"); /* DIMSCALE */
  fprintf (fp, " 41\n0.18\n"); /* DIMASZ */
  fprintf (fp, " 42\n0.0625\n"); /* DIMEXO */
  fprintf (fp, " 43\n0.38\n"); /* DIMDLI */
  fprintf (fp, " 44\n0.18\n"); /* DIMEXE */
  fprintf (fp, " 45\n0.0\n"); /* DIMRND */
  fprintf (fp, " 46\n0.0\n"); /* DIMDLE */
  fprintf (fp, " 47\n0.0\n"); /* DIMTP */
  fprintf (fp, " 48\n0.0\n"); /* DIMTM */
  fprintf (fp, "140\n0.18\n"); /* DIMTXT */
  fprintf (fp, "141\n0.09\n"); /* DIMCEN */
  fprintf (fp, "142\n0.0\n"); /* DIMTSZ */
  fprintf (fp, "143\n25.4\n"); /* DIMALTF */
  fprintf (fp, "144\n1.0\n"); /* DIMLFAC */
  fprintf (fp, "145\n0.0\n"); /* DIMTVP */
  fprintf (fp, "146\n1.0\n"); /* DIMTFAC */
  fprintf (fp, "147\n0.09\n"); /* DIMGAP */
  fprintf (fp, " 71\n     0\n"); /* DIMTOL */
  fprintf (fp, " 72\n     0\n"); /* DIMLIM */
  fprintf (fp, " 73\n     1\n"); /* DIMTIH */
  fprintf (fp, " 74\n     1\n"); /* DIMTOH */
  fprintf (fp, " 75\n     0\n"); /* DIMSE1 */
  fprintf (fp, " 76\n     0\n"); /* DIMSE2 */
  fprintf (fp, " 77\n     0\n"); /* DIMTAD */
  fprintf (fp, " 78\n     0\n"); /* DIMZIN */
  fprintf (fp, "170\n     0\n"); /* DIMALT */
  fprintf (fp, "171\n     2\n"); /* DIMALTD */
  fprintf (fp, "172\n     0\n"); /* DIMTOFL */
  fprintf (fp, "173\n     0\n"); /* DIMSAH */
  fprintf (fp, "174\n     0\n"); /* DIMTIX */
  fprintf (fp, "175\n     0\n"); /* DIMSOXD */
  fprintf (fp, "176\n     0\n"); /* DIMDLRD */
  fprintf (fp, "177\n     0\n"); /* DIMCLRE */
  fprintf (fp, "178\n     0\n"); /* DIMCLRT */
  fprintf (fp, "270\n     2\n"); /* DIMUNIT */
  fprintf (fp, "271\n     4\n"); /* DIMDEC */
  fprintf (fp, "272\n     4\n"); /* DIMTDEC */
  fprintf (fp, "273\n     2\n"); /* DIMALTU */
  fprintf (fp, "274\n     2\n"); /* DIMALTTD */
  fprintf (fp, "340\n241\n"); /* Handle of referenced STYLE object (used instead of storing DIMTXSTY value). */
  fprintf (fp, "275\n     0\n"); /* DIMAUNIT */
  fprintf (fp, "280\n     0\n"); /* DIMJUST */
  fprintf (fp, "281\n     0\n"); /* DIMSD1 */
  fprintf (fp, "282\n     0\n"); /* DIMSD2 */
  fprintf (fp, "283\n     1\n"); /* DIMTOLJ */
  fprintf (fp, "284\n     0\n"); /* DIMTZIN */
  fprintf (fp, "285\n     0\n"); /* DIMALTZ */
  fprintf (fp, "286\n     0\n"); /* DIMALTTZ */
  fprintf (fp, "287\n     3\n"); /* DIMFIT */
  fprintf (fp, "288\n     0\n"); /* DIMUPT */
  fprintf (fp, "  0\nENDTAB\n");
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_write_header_metric_new () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Write DXF output to a file for a DXF header derived from a default
 * header template file if available. If no default header template file is
 * available, generate one depending on the dxf_metric variable.
 *
 * Write down a DXF header based on values derived from a default header
 * template file (in pcb/src/hid/dxf/template directory).\n
 * Included sections and tables are:\n
 * <ul>
 * <li>HEADER section
 * <li>CLASSES section
 * <li>TABLES section
 *   <ul>
 *   <li>VPORT table
 *   <li>LTYPE table
 *   <li>LAYER table
 *   <li>STYLE table
 *   <li>VIEW table
 *   <li>UCS table
 *   <li>APPID table
 *   <li>DIMSTYLE table
 *   </ul>
 * </ul>
 * Continue from here with writing a BLOCK_RECORD table and close the section
 * with an ENDSEC marker.
 */
static void
dxf_write_header ()
{
  FILE *f_temp;
  char *temp = NULL;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_write_header () function.\n", __FILE__, __LINE__);
#endif
  if (dxf_metric)
  {
    dxf_header_filename = strdup ("hid/dxf/template/metric_header.dxf");
  }
  else
  {
#if 0
    dxf_header_filename = strdup ("hid/dxf/template/imperial_header.dxf");
#endif
  }
  /* check if template metric header file exists and open file
   * read-only  */
  f_temp = fopen (dxf_header_filename, "r");
  if (f_temp)
  {
    /* do until EOF of the template file:
     * copy line by line from template file (f_temp) to
     * destination file (fp) */
    while (f_temp != EOF)
    {
      fscanf (f_temp, "%s", temp);
      fprintf (fp, "%s", temp);
    }
    /* when we're done close the template file */
    fclose (f_temp);
  }
  else
  {
    gui->log ("Error in dxf_write_header_from_template (): cannot open file %s for reading.\n", dxf_header_filename);
    if (dxf_metric)
    {
      dxf_write_header_metric_new ();
    }
    else
    {
      dxf_write_header_imperial_new ();
    }
  }
  /* write a block record table */
  dxf_write_table_block_record (fp);
  /* write ENDSEC marker to close the header */
  dxf_write_endsection (fp);
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_write_header () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Write DXF output to a file for an insert entity.
 */
static void
dxf_write_insert
(
  FILE *fp,
    /*!< file pointer to output file (or device). */
  int id_code,
    /*!< group code = 5. */
  char *block_name,
    /*!< group code = 2. */
  char *linetype,
    /*!< group code = 6\n
     * optional, if omitted defaults to \c BYLAYER. */
  char *layer,
    /*!< group code = 8. */
  double x0,
    /*!< group code = 10\n
     * base point. */
  double y0,
    /*!< group code = 20\n
     * base point. */
  double z0,
    /*!< group code = 30\n
     * base point. */
  double thickness,
    /*!< group code = 39\n
     * optional, if omitted defaults to 0.0. */
  double rel_x_scale,
    /*!< group code = 41\n
     * optional, if omitted defaults to 1.0. */
  double rel_y_scale,
    /*!< group code = 42\n
     * optional, if omitted defaults to 1.0. */
  double rel_z_scale,
    /*!< group code = 43\n
     * optional, if omitted defaults to 1.0. */
  double column_spacing,
    /*!< group code = 44\n
     * optional, if omitted defaults to 0.0. */
  double row_spacing,
    /*!< group code = 45\n
     * optional, if omitted defaults to 0.0. */
  double rot_angle,
    /*!< group code = 50\n
     * optional, if omitted defaults to 0.0. */
  int color,
    /*!< group code = 62\n
     * optional, if omitted defaults to \c BYLAYER. */
  int attribute_follows,
    /*!< group code = 66\n
     * optional, if omitted defaults to 0. */
  int paperspace,
    /*!< group code = 67\n
     * optional, if omitted defaults to 0 (modelspace). */
  int columns,
    /*!< group code = 70\n
     * optional, if omitted defaults to 1. */
  int rows
    /*!< group code = 71\n
     * optional, if omitted defaults to 1. */
)
{
  char *dxf_entity_name;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_write_insert () function.\n", __FILE__, __LINE__);
  fprintf (stderr, "[DXF entity with ID code %x]\n", id_code);
#endif
  dxf_entity_name = strdup ("INSERT");
  if (strcmp (block_name, "") == 0)
  {
    fprintf (stderr, "Warning: empty block name string for the %s entity with id-code: %x\n", dxf_entity_name, id_code);
    fprintf (stderr, "   %s entity is discarded from output.\n", dxf_entity_name);
    return;
  }
  if (strcmp (layer, "") == 0)
  {
    fprintf (stderr, "Warning: empty layer string for the %s entity with id-code: %x\n", dxf_entity_name, id_code);
    fprintf (stderr, "    %s entity is relocated to layer 0.\n", dxf_entity_name);
    layer = strdup (DXF_DEFAULT_LAYER);
  }
  if (rel_x_scale == 0.0)
  {
    fprintf (stderr, "Warning: relative X-scale factor has a value of 0.0 for the %s entity with id-code: %x\n", dxf_entity_name, id_code);
    fprintf (stderr, "    default relative X-scale of 1.0 applied to %s entity.\n", dxf_entity_name);
    rel_x_scale = 1.0;
  }
  if (rel_y_scale == 0.0)
  {
    fprintf (stderr, "Warning: relative Y-scale factor has a value of 0.0 for the %s entity with id-code: %x\n", dxf_entity_name, id_code);
    fprintf (stderr, "    default relative Y-scale of 1.0 applied to %s entity.\n", dxf_entity_name);
    rel_y_scale = 1.0;
  }
  if (rel_z_scale == 0.0)
  {
    fprintf (stderr, "Warning: relative Z-scale factor has a value of 0.0 for the %s entity with id-code: %x\n", dxf_entity_name, id_code);
    fprintf (stderr, "    default relative Z-scale of 1.0 applied to %s entity.\n", dxf_entity_name);
    rel_z_scale = 1.0;
  }
  if ((columns > 1) && (column_spacing == 0.0))
  {
    fprintf (stderr, "Warning: number of columns is greater than 1 and the column spacing has a value of 0.0 for the %s entity with id-code: %x\n", dxf_entity_name, id_code);
    fprintf (stderr, "    default number of columns value of 1 applied to %s entity.\n", dxf_entity_name);
    columns = 1;
  }
  if ((rows > 1) && (row_spacing == 0.0))
  {
    fprintf (stderr, "Warning: number of rows is greater than 1 and the row spacing has a value of 0.0 for the %s entity with id-code: %x\n", dxf_entity_name, id_code);
    fprintf (stderr, "    default number of rows value of 1 applied to %s entity.\n", dxf_entity_name);
    rows = 1;
  }
  fprintf (fp, "  0\n%s\n", dxf_entity_name);
  fprintf (fp, "  2\n%s\n", block_name);
  if (id_code != -1)
  {
    fprintf (fp, "  5\n%x\n", id_code);
  }
  if (strcmp (linetype, DXF_DEFAULT_LINETYPE) != 0)
  {
    fprintf (fp, "  6\n%s\n", linetype);
  }
  fprintf (fp, "  8\n%s\n", layer);
  fprintf (fp, " 10\n%f\n", x0);
  fprintf (fp, " 20\n%f\n", y0);
  fprintf (fp, " 30\n%f\n", z0);
  if (thickness != 0.0)
  {
    fprintf (fp, " 39\n%f\n", thickness);
  }
  if (rel_x_scale != 1.0)
  {
    fprintf (fp, " 41\n%f\n", rel_x_scale);
  }
  if (rel_y_scale != 1.0)
  {
    fprintf (fp, " 42\n%f\n", rel_y_scale);
  }
  if (rel_z_scale != 1.0)
  {
    fprintf (fp, " 43\n%f\n", rel_z_scale);
  }
  if ((columns > 1) && (column_spacing > 0.0))
  {
    fprintf (fp, " 44\n%f\n", column_spacing);
  }
  if ((rows > 1) && (row_spacing > 0.0))
  {
    fprintf (fp, " 45\n%f\n", row_spacing);
  }
  if (rot_angle != 0.0)
  {
    fprintf (fp, " 50\n%f\n", rot_angle);
  }
  if (color != DXF_COLOR_BYLAYER)
  {
    fprintf (fp, " 62\n%d\n", color);
  }
  if (attribute_follows != 0)
  {
    fprintf (fp, " 66\n%d\n", attribute_follows);
  }
  if (paperspace == DXF_PAPERSPACE)
  {
    fprintf (fp, " 67\n%d\n", DXF_PAPERSPACE);
  }
  if (columns > 1)
  {
    fprintf (fp, " 70\n%d\n", columns);
  }
  if (rows > 1)
  {
    fprintf (fp, " 71\n%d\n", rows);
  }
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_write_insert () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Write DXF output to a file for a polyline entity.
 *
 * Following the Polyline header is a sequence of <c>Vertex</c> entities that
 * specify the vertex coordinates and faces that compose the mesh.\n
 * Vertices such as these are described in the following subsection on
 * Vertex.\n
 * \n
 * Applications might want to represent polygons with an arbitrarily large
 * number of sides in polyface meshes.\n
 * However, the AutoCAD entity structure imposes a limit on the number of
 * vertices that a given face entity can specify.\n
 * You can represent more complex polygons by decomposing them into triangular
 * wedges.\n
 */
static void
dxf_write_polyline
(
  FILE *fp,
    /*!< file pointer to output device  */
  int id_code,
    /*!< group code = 5  */
  char *linetype,
    /*!< group code = 6 \n optional, if omitted defaults to BYLAYER  */
  char *layer,
    /*!< group code = 8  */
  double x0,
    /*!< group code = 10 \n if omitted defaults to 0.0  */
  double y0,
    /*!< group code = 20 \n if omitted defaults to 0.0  */
  double z0,
    /*!< group code = 30 \n default elevation for vertices  */
  double extr_x0,
    /*!< group code = 210 \n extrusion direction \n optional, if ommited defaults to 0.0  */
  double extr_y0,
    /*!< group code = 220 \n extrusion direction \n optional, if ommited defaults to 0.0  */
  double extr_z0,
    /*!< group code = 230 \n extrusion direction \n optional, if ommited defaults to 1.0  */
  double thickness,
    /*!< group code = 39 \n optional, if omitted defaults to 0.0  */
  double start_width,
    /*!< group code = 40 \n optional, if omitted defaults to 0.0  */
  double end_width,
    /*!< group code = 41 \n optional, if omitted defaults to 0.0  */
  int color,
    /*!< group code = 62 \n optional, if omitted defaults to BYLAYER  */
  int vertices_follow,
    /*!< group code = 66 \n mandatory, always 1 (one or more vertices make up a polyline)  */
  int paperspace,
    /*!< group code = 67 \n optional, if omitted defaults to 0 (modelspace)  */
  int flag,
    /*!< group code = 70 \n optional, if omitted defaults to 0  */
    /*!< 1 = This is a closed Polyline (or a polygon mesh closed in the M direction) \n  */
    /*!< 2 = Curve-fit vertices have been added \n  */
    /*!< 4 = Spline-fit vertices have been added \n  */
    /*!< 8 = This is a 3D Polyline \n  */
    /*!< 16 = This is a 3D polygon mesh. \n  */
    /*!< 32 = The polygon mesh is closed in the N direction \n  */
    /*!< 64 = This Polyline is a polyface mesh \n  */
    /*!< 128 = The linetype pattern is generated continuously around the vertices of this Polyline  */
  int polygon_mesh_M_vertex_count,
    /*!< group code = 71 \n optional, if omitted defaults to 0  */
  int polygon_mesh_N_vertex_count,
    /*!< group code = 72 \n optional, if omitted defaults to 0  */
  int smooth_M_surface_density,
    /*!< group code = 73 \n optional, if omitted defaults to 0  */
  int smooth_N_surface_density,
    /*!< group code = 74 \n optional, if omitted defaults to 0  */
  int surface_type
    /*!< group code = 75 \n optional, if omitted defaults to 0 \n integer coded, not bit-coded: \n  */
    /*!< 0 = no smooth surface fitted \n  */
    /*!< 5 = quadratic B-spline surface \n  */
    /*!< 6 = cubic B-spline surface \n  */
    /*!< 8 = Bezier surface \n  */
)
{
  char *dxf_entity_name;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_write_polyline () function.\n", __FILE__, __LINE__);
  fprintf (stderr, "[DXF entity with code %x]\n", id_code);
#endif
  dxf_entity_name = strdup ("POLYLINE");
  if (x0 != 0.0)
  {
    fprintf (stderr, "Warning: start point has an invalid X-value for the %s entity with id-code: %x\n", dxf_entity_name, id_code);
    fprintf (stderr, "   %s entity is discarded from output.\n", dxf_entity_name);
    return;
  }
  if (y0 != 0.0)
  {
    fprintf (stderr, "Warning: start point has an invalid Y-value for the %s entity with id-code: %x\n", dxf_entity_name, id_code);
    fprintf (stderr, "   %s entity is discarded from output.\n", dxf_entity_name);
    return;
  }
  if (vertices_follow != 1)
  {
    fprintf (stderr, "Warning: vertices follow flag has an invalid value for the %s entity with id-code: %x\n", dxf_entity_name, id_code);
    fprintf (stderr, "   %s entity is discarded from output.\n", dxf_entity_name);
    return;
  }
  if (strcmp (layer, "") == 0)
  {
    fprintf (stderr, "Warning: empty layer string for the %s entity with id-code: %x\n", dxf_entity_name, id_code);
    fprintf (stderr, "   %s entity is relocated to layer 0\n", dxf_entity_name);
    layer = strdup (DXF_DEFAULT_LAYER);
  }
  fprintf (fp, "  0\n%s\n", dxf_entity_name);
  fprintf (fp, "100\nAcDb3dPolyline\n");
  if (id_code != -1)
  {
    fprintf (fp, "  5\n%x\n", id_code);
  }
  if (strcmp (linetype, DXF_DEFAULT_LINETYPE) != 0)
  {
    fprintf (fp, "  6\n%s\n", linetype);
  }
  fprintf (fp, "  8\n%s\n", layer);
  fprintf (fp, " 10\n%f\n", x0);
  fprintf (fp, " 20\n%f\n", y0);
  fprintf (fp, " 30\n%f\n", z0);
  fprintf (fp, "210\n%f\n", extr_x0);
  fprintf (fp, "220\n%f\n", extr_y0);
  fprintf (fp, "230\n%f\n", extr_z0);
  if (thickness != 0.0)
  {
    fprintf (fp, " 39\n%f\n", thickness);
  }
  if (start_width != 0.0)
  {
    fprintf (fp, " 40\n%f\n", start_width);
  }
  if (end_width != 0.0)
  {
    fprintf (fp, " 41\n%f\n", end_width);
  }
  if (color != DXF_COLOR_BYLAYER)
  {
    fprintf (fp, " 62\n%d\n", color);
  }
  fprintf (fp, " 66\n%d\n", vertices_follow);
  if (paperspace == DXF_PAPERSPACE)
  {
    fprintf (fp, " 67\n%d\n", DXF_PAPERSPACE);
  }
  fprintf (fp, " 70\n%d\n", flag);
  fprintf (fp, " 71\n%d\n", polygon_mesh_M_vertex_count);
  fprintf (fp, " 72\n%d\n", polygon_mesh_N_vertex_count);
  fprintf (fp, " 73\n%d\n", smooth_M_surface_density);
  fprintf (fp, " 74\n%d\n", smooth_N_surface_density);
  fprintf (fp, " 75\n%d\n", surface_type);
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_write_polyline () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Write DXF output to a file for a section marker.
 */
static void
dxf_write_section
(
  FILE *fp,
    /*!< file pointer to output device  */
  char *section_name
    /*!< section name  */
)
{
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_write_section () function.\n", __FILE__, __LINE__);
#endif
  /* no use in writing an empty string to file */
  if (strcmp (section_name, "") == 0)
  {
    return;
  }
  fprintf (fp, "  0\nSECTION\n  2\n%s\n", section_name);
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_write_section () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Write DXF output to a file for a solid entity.
 */
static void
dxf_write_solid
(
  FILE *fp,
    /*!< file pointer to output device  */
  int id_code,
    /*!< group code = 5  */
  char *linetype,
    /*!< group code = 6 \n optional, defaults to BYLAYER  */
  char *layer,
    /*!< group code = 8  */
  double x0,
    /*!< group code = 10 \n base point X-value, bottom left  */
  double y0,
    /*!< group code = 20 \n base point Y-value, bottom left  */
  double z0,
    /*!< group code = 30 \n base point Z-value, bottom left  */
  double x1,
    /*!< group code = 11 \n alignment point X-vaule, bottom right  */
  double y1,
    /*!< group code = 21 \n alignment point Y-vaule, bottom right  */
  double z1,
    /*!< group code = 31 \n alignment point Z-vaule, bottom right  */
  double x2,
    /*!< group code = 12 \n alignment point X-value, top left  */
  double y2,
    /*!< group code = 22 \n alignment point Y-value, top left  */
  double z2,
    /*!< group code = 32 \n alignment point Z-value, top left  */
  double x3,
    /*!< group code = 13 \n alignment point X-value, top right  */
  double y3,
    /*!< group code = 23 \n alignment point Y-value, top right  */
  double z3,
    /*!< group code = 33 \n alignment point Z-value, top right  */
  double thickness,
    /*!< group code = 39 \n optional, defaults to 0.0  */
  int color,
    /*!< group code = 62 \n optional, defaults to BYLAYER  */
  int paperspace
    /*!< group code = 67 \n optional, defaults to 0 (modelspace)  */
)
{
  char *dxf_entity_name;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_write_solid () function.\n", __FILE__, __LINE__);
  fprintf (stderr, "[DXF entity with code %x]\n", id_code);
#endif
  dxf_entity_name = strdup ("SOLID");
  if (strcmp (layer, "") == 0)
  {
    fprintf (stderr, "Warning: empty layer string for the %s entity with id-code: %x\n", dxf_entity_name, id_code);
    fprintf (stderr, "    %s entity is relocated to layer 0", dxf_entity_name);
    layer = strdup (DXF_DEFAULT_LAYER);
  }
  fprintf (fp, "  0\n%s\n", dxf_entity_name);
  if (id_code != -1)
  {
    fprintf (fp, "  5\n%x\n", id_code);
  }
  if (strcmp (linetype, DXF_DEFAULT_LINETYPE) != 0)
  {
    fprintf (fp, "  6\n%s\n", linetype);
  }
  fprintf (fp, "  8\n%s\n", layer);
  fprintf (fp, " 10\n%f\n", x0);
  fprintf (fp, " 20\n%f\n", y0);
  fprintf (fp, " 30\n%f\n", z0);
  fprintf (fp, " 11\n%f\n", x1);
  fprintf (fp, " 21\n%f\n", y1);
  fprintf (fp, " 31\n%f\n", z1);
  fprintf (fp, " 12\n%f\n", x2);
  fprintf (fp, " 22\n%f\n", y2);
  fprintf (fp, " 32\n%f\n", z2);
  fprintf (fp, " 13\n%f\n", x3);
  fprintf (fp, " 23\n%f\n", y3);
  fprintf (fp, " 33\n%f\n", z3);
  if (thickness != 0.0)
  {
    fprintf (fp, " 39\n%f\n", thickness);
  }
  if (color != DXF_COLOR_BYLAYER)
  {
    fprintf (fp, " 62\n%d\n", color);
  }
  if (paperspace == DXF_PAPERSPACE)
  {
    fprintf (fp, " 67\n%d\n", DXF_PAPERSPACE);
  }
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_write_solid () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Write DXF output to a file for a polyline vertex entity.
 */
static void
dxf_write_vertex
(
  FILE *fp,
    /*!< file pointer to output device */
  int id_code,
    /*!< group code = 5 */
  char *linetype,
    /*!< group code = 6 \n
     * optional, if omitted defaults to BYLAYER */
  char *layer,
    /*!< group code = 8 */
  double x0,
    /*!< group code = 10 \n */
  double y0,
    /*!< group code = 20 \n */
  double z0,
    /*!< group code = 30 \n */
  double thickness,
    /*!< group code = 39 \n
     * optional, if omitted defaults to 0.0 */
  double start_width,
    /*!< group code = 40 \n
     * optional, if omitted defaults to 0.0 */
  double end_width,
    /*!< group code = 41 \n
     * optional, if omitted defaults to 0.0 */
  double bulge,
    /*!< group code = 42 \n
     * optional, if omitted defaults to 0.0 \n
     * The bulge is the tangent of 1/4 of the included angle
     * for an arc segment. \n
     * Made negative if the arc goes clockwise from the start
     * point to the endpoint. \n
     * A bulge of 0 indicates a straight segment, and a bulge
     * of 1 is a semicircle. \n */
  double curve_fit_tangent_direction,
    /*!< group code = 50 \n optional, a curve-fit tangent direction of 0.0 may be omitted from the DXF output, but is significant if the flag bit is set. \n */
  int color,
    /*!< group code = 62 \n optional, if omitted defaults to BYLAYER */
  int paperspace,
    /*!< group code = 67 \n optional, if omitted defaults to 0 (modelspace) */
  int flag
    /*!< group code = 70 \n optional, if omitted defaults to 0
     *  bit coded: \n
     * 1 = extra vertex created by curve-fitting \n
     * 2 = curve-fit tangent defined for this vertex. \n
     * 4 = unused (never set in DXF files) \n
     * 8 = spline vertex created by spline-fitting \n
     * 16 = spline frame control point \n
     * 32 = 3D Polyline vertex \n
     * 64 = 3D polygon mesh vertex \n
     * 128 = polyface mesh vertex */
)
{
  char *dxf_entity_name;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_write_vertex () function.\n", __FILE__, __LINE__);
  fprintf (stderr, "[DXF entity with code %x]\n", id_code);
#endif
  dxf_entity_name = strdup ("VERTEX");
  if (strcmp (layer, "") == 0)
  {
    fprintf (stderr, "Warning: empty layer string for the %s entity with id-code: %x\n", dxf_entity_name, id_code);
    fprintf (stderr, "    %s entity is relocated to layer 0", dxf_entity_name);
    layer = strdup (DXF_DEFAULT_LAYER);
  }

  fprintf (fp, "  0\n%s\n", dxf_entity_name);
  if (id_code != -1)
  {
    fprintf (fp, "  5\n%x\n", id_code);
  }
  if (strcmp (linetype, DXF_DEFAULT_LINETYPE) != 0)
  {
    fprintf (fp, "  6\n%s\n", linetype);
  }
  fprintf (fp, "  8\n%s\n", layer);
  fprintf (fp, " 10\n%f\n", x0);
  fprintf (fp, " 20\n%f\n", y0);
  fprintf (fp, " 30\n%f\n", z0);
  if (thickness != 0.0)
  {
    fprintf (fp, " 39\n%f\n", thickness);
  }
  if (start_width != 0.0)
  {
    fprintf (fp, " 40\n%f\n", start_width);
  }
  if (end_width != 0.0)
  {
    fprintf (fp, " 41\n%f\n", end_width);
  }
  if (bulge != 0.0)
  {
    fprintf (fp, " 42\n%f\n", bulge);
  }
  if (curve_fit_tangent_direction != 0.0)
  {
    fprintf (fp, " 50\n%f\n", curve_fit_tangent_direction);
  }
  if (color != DXF_COLOR_BYLAYER)
  {
    fprintf (fp, " 62\n%d\n", color);
  }
  if (paperspace == DXF_PAPERSPACE)
  {
    fprintf (fp, " 67\n%d\n", DXF_PAPERSPACE);
  }
  fprintf (fp, " 70\n%d\n", flag);
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_write_vertex () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Get export options such as filename and filename base.
 *
 * Returns a set of resources describing options the export or print HID
 * supports.\n
 * In GUI mode, the print/export dialogs use this to set up the selectable
 * options.\n
 * In command line mode, these are used to interpret command line options.\n
 * If n_ret is non-NULL, the number of attributes is stored there.
 */
static HID_Attribute *
dxf_get_export_options (int *n)
{
  static char *last_dxf_filename;
  static char *last_dxf_xref_filename;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_get_export_options () function.\n", __FILE__, __LINE__);
#endif
  last_dxf_filename = 0;
  last_dxf_xref_filename = 0;
  if (PCB)
  {
    derive_default_filename (PCB->Filename, &dxf_options[HA_dxffile], "", &last_dxf_filename);
    derive_default_filename (PCB->Filename, &dxf_options[HA_xreffile], "", &last_dxf_xref_filename);
  }
  if (n)
  {
    *n = NUM_OPTIONS;
  }
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_get_export_options () function.\n", __FILE__, __LINE__);
#endif
  return dxf_options;
}


/*!
 * \brief Insert an element in the list of elements.
 */
static DxfList *
dxf_insert
(
  char *refdes,
    /*!< reference designator. */
  char *descr,
    /*!< description or footprint. */
  char *value,
    /*!< element value. */
  DxfList * dxf
    /*!< next item in list. */
)
{
  DxfList *new;
  DxfList *cur;
  DxfList *prev;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_insert () function.\n", __FILE__, __LINE__);
#endif
  prev = NULL;
  if (dxf == NULL)
  {
  /*
   * this is the first element so automatically create an entry.
   */
  if ((new = (DxfList *) malloc (sizeof (DxfList))) == NULL)
  {
    fprintf (stderr, "Error in dxf.c|dxf_insert (): malloc() failed.\n");
    exit (1);
  }
  new->next = NULL;
  new->descr = strdup (descr);
  new->value = strdup (value);
  new->num = 1;
  new->refdes = dxf_string_insert (refdes, NULL);
  return (new);
  }
  /*
   * search and see if we already have used one of these components.
   */
  cur = dxf;
  while (cur != NULL)
  {
    if ((NSTRCMP (descr, cur->descr) == 0) && (NSTRCMP (value, cur->value) == 0))
    {
      cur->num++;
      cur->refdes = dxf_string_insert (refdes, cur->refdes);
      break;
    }
    prev = cur;
    cur = cur->next;
  }
  if (cur == NULL)
  {
    if ((new = (DxfList *) malloc (sizeof (DxfList))) == NULL)
    {
      fprintf (stderr, "Error in dxf.c|dxf_insert (): malloc() failed.\n");
      exit (1);
    }
    prev->next = new;
    new->next = NULL;
    new->descr = strdup (descr);
    new->value = strdup (value);
    new->num = 1;
    new->refdes = dxf_string_insert (refdes, NULL);
  }
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_insert () function.\n", __FILE__, __LINE__);
#endif
  return (dxf);
}


/*!
 * \brief Print Xrefs to DXF file.
 *
 * Generate a file in the AutoCAD R14 DXF format for insertion of 3D models as
 * external references (Xref's).\n
 * An external reference is a reference to an external drawing block which is
 * loaded at runtime (of the mechanical CAD software; for example AutoCAD)
 * during the loading of the toplevel drawing model (the dxf file) in a
 * mechanical CAD program or during separate insertions after the initial
 * loading whilst in drawing mode.\n
 * Note that for most mechanical CAD software the inserted block cannot be a
 * DXF file.\n
 * In most cases a DXF file representing a (3D) model must first be converted
 * to a ".dwg" file, or any other file format native to the mechanical CAD
 * software used.\n
 * The filename of the 3D model inserted in the dxf file is:\n
 * "Description or footprint name" + ".dwg" (file extension).\n
 * Any element without a valid description or footprint name is inserted in the
 * dxf file with a 3D model with a text "undefined part" and this has to be
 * manually inserted in the toplevel model after the initial loading of the dxf
 * file.\n
 * This is to prevent unnoticed ommissions of parts in the toplevel 3D model.
 */
static int
dxf_export_xref_file (void)
{
  char *name;
  char utcTime[64];
  double x;
  double y;
  double theta;
  double sumx;
  double sumy;
  double pin1x;
  double pin1y;
  double pin1angle;
  double pin2x;
  double pin2y;
  double pin2angle;
  int found_pin1;
  int found_pin2;
  int pin_cnt;
  time_t currenttime;
  DxfList *dxf;
  DxfList *lastb;
  char *dxf_block_name;
  char *dxf_xref_name;
  double dxf_x0;
  double dxf_y0;
  double dxf_rot_angle;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_export_xref_file () function.\n", __FILE__, __LINE__);
#endif
  name = NULL;
  theta = 0.0;
  pin1x = 0.0;
  pin1y = 0.0;
  pin1angle = 0.0;
  pin2x = 0.0;
  pin2y = 0.0;
  dxf = NULL;
  dxf_block_name = NULL;
  dxf_xref_name = NULL;
  dxf_x0 = 0.0;
  dxf_y0 = 0.0;
  dxf_rot_angle = 0.0;

  fp = fopen (dxf_xref_filename, "w");
  if (!fp)
  {
    gui->log ("Error in dxf.c|dxf_export_xref_file (): cannot open file %s for writing.\n", dxf_xref_filename);
    return 1;
  }
  /*
   * create a portable timestamp.
   */
  currenttime = time (NULL);
  strftime (utcTime, sizeof (utcTime), "%c UTC", gmtime (&currenttime));
  if (dxf_verbose)
  {
    /* report at the beginning of each file */
    fprintf (stderr, "DXF: Board Name: %s, %s \n", UNKNOWN (PCB->Name),
      UNKNOWN (name));
    fprintf (stderr, "DXF: Created by: %s.\n", PCB_DXF_HID_VERSION);
    fprintf (stderr, "DXF: Creation date: %s \n", utcTime);
    fprintf (stderr, "DXF: File Format according to: AutoCAD R14.\n");
    if (dxf_metric)
    {
      fprintf (stderr, "DXF using Metric coordinates [mm].\n");
      pcb_fprintf (stderr, "PCB Dimensions: %.0mm x %.0mm.\n",
        PCB->MaxWidth, PCB->MaxHeight);
    }
    else
    {
      fprintf (stderr, "DXF using Imperial coordinates [mil].\n");
      pcb_fprintf (stderr, "PCB Dimensions: %.0mil x %.0mil.\n",
        PCB->MaxWidth, PCB->MaxHeight);
    }
    fprintf (stderr, "PCB Coordinate Origin: lower left.\n");
    fprintf (stderr, "DXF: Now processing Xrefs file.\n");
  }
  /* write version info as a dxf comment */
  dxf_write_comment (fp, PCB_DXF_HID_VERSION);
  /* write dxf header information */
  dxf_write_header (fp);
  dxf_write_section (fp, "BLOCKS");
  /*
   * lookup all elements on pcb and insert element in the list of elements.
   */
  ELEMENT_LOOP (PCB->Data);
  {
    /*
     * insert the elements into the dxf list.
     */
    dxf = dxf_insert (UNKNOWN (NAMEONPCB_NAME (element)),
          UNKNOWN (DESCRIPTION_NAME (element)),
          UNKNOWN (VALUE_NAME (element)), dxf);
  }
  END_LOOP; /* End of ELEMENT_LOOP  */
  /*
   * now write a single block definition for every unique element to
   * the BLOCKS section of the DXF file.
   * since these are all supposed to be Xref blocks they are not to
   * contain entities, just the path and filename (including extension).
   * write a section BLOCKS marker to the DXF file.
   */
  while (dxf != NULL)
  {
    dxf_block_name = strdup (dxf_clean_string (dxf->descr));
    dxf_xref_name = DXF_DEFAULT_XREF_PATH_NAME;
    dxf_write_block
    (
      fp,
      dxf_id_code,
      dxf_xref_name,
      dxf_block_name,
      DXF_DEFAULT_LINETYPE, /* linetype, */
      DXF_DEFAULT_LAYER, /* layer, */
      0.0, /* dxf_x0, */
      0.0, /* dxf_y0, */
      0.0, /* dxf_z0, */
      0.0, /* dxf_thickness, */
      DXF_COLOR_BYLAYER, /* dxf_color, */
      0, /* dxf_paperspace, */
      36 /* dxf_block_type*/
    );
    dxf_id_code++;
    lastb = dxf;
    dxf = dxf->next;
    free (lastb);
  }
  /* write an ENDSEC marker to the DXF file */
  dxf_write_endsection (fp);
  /*
   * write a section ENTITIES marker to the DXF file.
   */
  dxf_write_section (fp, "ENTITIES");
  /*
   * for each element we calculate the centroid of the footprint.
   * in addition, we need to extract some notion of rotation.
   */
  ELEMENT_LOOP (PCB->Data);
  {
    /*
     * initialize our pin count and our totals for finding the
     * centroid.
     */
    pin_cnt = 0;
    sumx = 0.0;
    sumy = 0.0;
    found_pin1 = 0;
    found_pin2 = 0;
    /*
     * iterate over the pins and pads keeping a running count of
     * many pins/pads total and the sum of x and y coordinates.
     * While we're at it, store the location of pin/pad #1 and #2
     * if we can find them.
     */
    PIN_LOOP (element);
    {
      sumx += (double) pin->X;
      sumy += (double) pin->Y;
      pin_cnt++;
      if (NSTRCMP (pin->Number, "1") == 0)
      {
        pin1x = (double) pin->X;
        pin1y = (double) pin->Y;
        pin1angle = 0.0;
        found_pin1 = 1;
      }
      else if (NSTRCMP (pin->Number, "2") == 0)
      {
        pin2x = (double) pin->X;
        pin2y = (double) pin->Y;
        pin2angle = 0.0;
        found_pin2 = 1;
      }
    }
    END_LOOP; /* End of PIN_LOOP  */
    PAD_LOOP (element);
    {
      sumx += (pad->Point1.X + pad->Point2.X) / 2.0;
      sumy += (pad->Point1.Y + pad->Point2.Y) / 2.0;
      pin_cnt++;
      if (NSTRCMP (pad->Number, "1") == 0)
      {
        pin1x = (double) (pad->Point1.X + pad->Point2.X) / 2.0;
        pin1y = (double) (pad->Point1.Y + pad->Point2.Y) / 2.0;
        /*
         * NOTE: we swap the Y points, because in PCB
         * the Y-axis is inverted, and increasing Y
         * moves down.
         * we want to deal with a right-handed
         * Cartesian Coordinate System where
         * increasing Y moves up.
         */
        pin1angle = (180.0 / M_PI) * atan2 (pad->Point1.Y - pad->Point2.Y, pad->Point2.X - pad->Point1.X);
        found_pin1 = 1;
      }
      else if (NSTRCMP (pad->Number, "2") == 0)
      {
        pin2x = (double) (pad->Point1.X + pad->Point2.X) / 2.0;
        pin2y = (double) (pad->Point1.Y + pad->Point2.Y) / 2.0;
        pin2angle = (180.0 / M_PI) * atan2 (pad->Point1.Y - pad->Point2.Y, pad->Point2.X - pad->Point1.X);
        found_pin2 = 1;
      }
    }
    END_LOOP; /* End of PAD_LOOP  */
    if (pin_cnt > 0)
    {
      x = sumx / (double) pin_cnt;
      y = sumy / (double) pin_cnt;
      if (found_pin1)
      {
        /*
         * recenter pin #1 onto the axis which cross
         * at the part centroid.
         */
        pin1x -= x;
        pin1y -= y;
        pin1y = -1.0 * pin1y;
        /* if only 1 pin, use pin 1's angle */
        if (pin_cnt == 1) theta = pin1angle;
        else
        {
        /*
         * if pin #1 is at (0,0) use pin #2 for
         * rotation
         */
          if ((pin1x == 0.0) && (pin1y == 0.0))
          {
            if (found_pin2) theta = dxf_xy_to_angle (pin2x, pin2y);
            else
            {
              Message  ("Warning in dxf.c|dxf_export_xref_file ():\n"
                  "     unable to figure out angle of element\n"
                  "     %s because pin #1 is at the centroid of the part\n"
                  "     and I could not find pin #2's location.\n"
                  "     Setting to %g degrees.\n", UNKNOWN (NAMEONPCB_NAME (element)), theta);
            }
          }
          else theta = dxf_xy_to_angle (pin1x, pin1y);
        }
      }
      /* we did not find pin #1 */
      else
      {
        theta = 0.0;
        Message  ("Warning in dxf.c|dxf_export_xref_file ():\n"
            "     unable to figure out angle because I could\n"
            "     not find pin #1 of element %s.\n"
            "     Setting to %g degrees.\n", UNKNOWN (NAMEONPCB_NAME (element)), theta);
      }
      dxf_block_name = strdup (dxf_clean_string (UNKNOWN (DESCRIPTION_NAME (element))));
      if (dxf_metric)
      {
        /* convert mils to mm */
        dxf_x0 = COORD_TO_MM (x);
        /* convert mils to mm and a right handed
         * Cartesian Coordinate System */
        dxf_y0 = COORD_TO_MM (PCB->MaxHeight - y);
      }
      else
      {
        /*
         * no need to convert, some things remain the
         * same.
         */
        dxf_x0 = COORD_TO_MIL (x);
        /*
         * only convert to a right handed Cartesian
         * Coordinate System.
         */
        dxf_y0 = COORD_TO_MIL (PCB->MaxHeight - y);
      }
#if 0
      /*
       * convert the rotation angle as well:
       * theta -> CW, DXF -> CCW.
       */
      /*!
       * \todo for now we only support Cardinal angles
       * [North, East, South, West]
       */
      if (theta == 0.0) dxf_rot_angle = 90.0;
      else if (theta == 90.0) dxf_rot_angle = 0.0;
      else if (theta == 180.0) dxf_rot_angle = 270.0;
      else if (theta == 270.0) dxf_rot_angle = 180.0;
      else
      {
        dxf_rot_angle = 0.0;
        Message  ("Warning in dxf.c|dxf_export_xref_file ():\n"
            "     unable to figure out angle of dxf block\n"
            "     %s because pcb angle theta is not Cardinal [0.0, 90.0, 180.0, 270.0].\n"
            "     Setting dxf_rot_angle to %g degrees\n", UNKNOWN (NAMEONPCB_NAME (element)), dxf_rot_angle);
           }
#endif
           dxf_write_insert
           (
             fp,
             dxf_id_code,
             dxf_block_name,
             DXF_DEFAULT_LINETYPE, /* dxf_linetype, */
             DXF_DEFAULT_LAYER, /* dxf_layer, */
             dxf_x0,
             dxf_y0,
             0.0, /* dxf_z0, */
             0.0, /* dxf_thickness, */
             1.0, /* dxf_rel_x_scale, */
             1.0, /* dxf_rel_y_scale, */
             1.0, /* dxf_rel_z_scale, */
             0.0, /* dxf_column_spacing, */
             0.0, /* dxf_row_spacing, */
             dxf_rot_angle,
             DXF_COLOR_BYLAYER, /* dxf_color, */
             0, /* dxf_attribute_follows, */
             0, /* dxf_paperspace, */
             1, /* dxf_columns, */
             1 /* dxf_rows */
      );
    }
    dxf_id_code++;
  }
  END_LOOP; /* End of ELEMENT_LOOP  */
  /*
   * write an ENDSEC marker to the DXF file.
   */
  dxf_write_endsection(fp);
  /*
   * write an EOF marker and close the DXF file.
   */
  dxf_write_eof (fp);
  fclose (fp);
  fp = NULL;
  dxf_id_code = 0;
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_export_xref_file () function.\n", __FILE__, __LINE__);
#endif
  return (0);
}


/*!
 * \brief Close DXF layer file.
 */
static void
dxf_maybe_close_file ()
{
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_maybe_close_file () function.\n", __FILE__, __LINE__);
#endif
  if (fp)
  {
    /* write an EOF marker and close the DXF file */
    dxf_write_eof (fp);
    fclose (fp);
  }
  dxf_id_code = 0;
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_maybe_close_file () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Export (or print) the current PCB.
 *
 * The options given represent the choices made from the options returned
 * from dxf_get_export_options.\n
 * Call with options == NULL to start the primary GUI (create a main window,
 * print, export, etc).\n
 * \n
 * First get the export options.\n
 * Do export all the DXF files required.\n
 * <ul>
 * <li>Export the DXF file with Xref blocks to a seperate dxf file if required.\n
 * <li>Export a DXF file for every PCB layer.\n
 * </ul>
 */
static void
dxf_do_export (HID_Attr_Val * options)
{
  char *dxf_fnbase;
  int i;
  static int saved_layer_stack[MAX_LAYER];
  BoxType region;
  int save_ons[MAX_LAYER + 2];

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_do_export () function.\n", __FILE__, __LINE__);
#endif
  dxf_fnbase = NULL;

  if (!options)
  {
    dxf_get_export_options (0);
    for (i = 0; i < NUM_OPTIONS; i++)
    {
      dxf_values[i] = dxf_options[i].default_val;
    }
    options = dxf_values;
  }
  /*
   * verbose output (dxf files to contain comments).
   */
  dxf_verbose = options[HA_verbose].int_value;
  /*
   * if all layers needs to be exported.
   */
  dxf_export_all_layers = options[HA_export_all_layers].int_value;
  /*
   * output to be in in mils or mm.
   */
  dxf_metric = options[HA_metric].int_value;
  /*
   * entity color to be BYBLOCK (or by layer number).
   */
  dxf_color_is_byblock = options[HA_color_byblock].int_value;
  /*
   * if xrefs DXF file needs to be exported.
   */
  dxf_xrefs = options[HA_xrefs].int_value;
  if (dxf_xrefs)
  {
    /*
     * determine a file name for the xref file.
     */
    dxf_xref_filename = options[HA_xreffile].str_value;
    if (!dxf_xref_filename)
    {
      strcat (dxf_xref_filename, "pcb-out_xrefs");
    }
    i = strlen (dxf_xref_filename);
    dxf_xref_filename = (char *)realloc (dxf_xref_filename, i + 40);
    strcat (dxf_xref_filename, "_xrefs.dxf");
    dxf_filesuffix = dxf_xref_filename + strlen (dxf_xref_filename);
    dxf_export_xref_file ();
  }
  /*
   * determine a file name base for the DXF layer files.
   */
  dxf_fnbase = options[HA_dxffile].str_value;
  if (!dxf_fnbase)
  {
    dxf_fnbase = "pcb_layers";
  }
  i = strlen (dxf_fnbase);
  dxf_filename = (char *)realloc (dxf_filename, i + 40);
  strcpy (dxf_filename, dxf_fnbase);
  strcat (dxf_filename, "_");
  dxf_filesuffix = dxf_filename + strlen (dxf_filename);
  memset (print_group, 0, sizeof (print_group));
  memset (print_layer, 0, sizeof (print_layer));
  /*
   * use this to temporarily enable all layers.
   */
  hid_save_and_show_layer_ons (save_ons);
  for (i = 0; i < max_copper_layer; i++)
  {
    LayerType *layer = PCB->Data->Layer + i;
    if (layer->LineN || layer->TextN || layer->ArcN || layer->PolygonN)
    {
      print_group[GetLayerGroupNumberByNumber (i)] = 1;
    }
  }
  print_group[GetLayerGroupNumberByNumber (solder_silk_layer)] = 1;
  print_group[GetLayerGroupNumberByNumber (component_silk_layer)] = 1;
  for (i = 0; i < max_copper_layer; i++)
  {
    if (print_group[GetLayerGroupNumberByNumber (i)])
    {
      print_layer[i] = 1;
    }
  }
  memcpy (saved_layer_stack, LayerStack, sizeof (LayerStack));
  qsort (LayerStack, max_copper_layer, sizeof (LayerStack[0]),
    dxf_layer_sort);
  linewidth = -1;
  lastcap = -1;
  lastgroup = -1;
  lastcolor = -1;
  region.X1 = 0;
  region.Y1 = 0;
  region.X2 = PCB->MaxWidth;
  region.Y2 = PCB->MaxHeight;
  pagecount = 1;
//  dxf_init_apertures ();
  lastgroup = -1;
  c_layerapps = 0;
  dxf_finding_apertures = 1;
  hid_expose_callback (&dxf_hid, &region, 0);
  c_layerapps = 0;
  dxf_finding_apertures = 0;
  hid_expose_callback (&dxf_hid, &region, 0);
  memcpy (LayerStack, saved_layer_stack, sizeof (LayerStack));
  dxf_maybe_close_file ();
  hid_restore_layer_ons (save_ons);
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_do_export () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Parse the command line.
 *
 * Parse HID register attributes and HID command line arguments.\n
 * Call this early for whatever HID will be the primary HID, as it will set
 * all the registered attributes.\n
 * The HID should remove all arguments, leaving any possible file names
 * behind.
 */
static void
dxf_parse_arguments (int *argc, char ***argv)
{
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_parse_arguments () function.\n", __FILE__, __LINE__);
#endif
  hid_register_attributes (dxf_options, sizeof (dxf_options) / sizeof (dxf_options[0]));
  hid_parse_command_line (argc, argv);
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_parse_arguments () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Sort drills (holes).
 */
static int
dxf_drill_sort
(
  const void *va,
  const void *vb
)
{
  DxfPendingDrills *a;
  DxfPendingDrills *b;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_drill_sort () function.\n", __FILE__, __LINE__);
#endif
  a = (DxfPendingDrills *) va;
  b = (DxfPendingDrills *) vb;
  if (a->diam != b->diam) return a->diam - b->diam;
  if (a->x != b->x) return a->x - a->x;
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_drill_sort () function.\n", __FILE__, __LINE__);
#endif
  return b->y - b->y;
}


/*!
 * \brief Set the layer with <c>name</c> for DXF export.
 *
 * During redraw or print/export cycles, this is called once per layer
 * (or layer group, for copper layers).\n
 * If it returns false (zero), the HID does not want that layer, and none of
 * the drawing functions should be called.\n
 * If it returns true (nonzero), the items in that layer [group] should be
 * drawn using the various drawing functions.\n
 * In addition to the MAX_LAYERS copper layer groups, you may select layers
 * indicated by the macros SL_* defined, or any others with an index of -1.\n
 * For copper layer groups, you may pass NULL for name to have a name fetched
 * from the PCB struct.\n
 * \n
 * All copper containing layers are set for DXF export.\n
 * All assembly layers are set for DXF export.\n
 * Exceptions are: \n
 * <ul>
 * <li>Layers with the name "invisible" are not set for DXF export.
 * <li>Layers with the name "keepout" are not set for DXF export.
 * <li>Layers without exportable items are not set for DXF export.
 * </ul>
 */
static int
dxf_set_layer
(
  const char *name,
  int group
)
{
  char *cp;
  int idx;
  const char *fmt;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_set_layer () function.\n",
    __FILE__, __LINE__);
#endif
  idx = (group >= 0 && group < max_group) ?
    PCB->LayerGroups.Entries[group][0] : group;

  if (name == 0)
  {
    /* if none given, get the layer name from pcb */
    name = PCB->Data->Layer[idx].Name;
  }
  if (dxf_verbose)
  {
    fprintf (stderr, "DXF: now processing Layer %s group %d\n", name, group);
  }
  if (dxf_export_all_layers)
  {
    /* do nothing here to export all layers */
  }
  else
  {
    if (idx >= 0 && idx < max_copper_layer && !print_layer[idx])
    {
      /* do not export empty layers */
      if (dxf_verbose)
      {
        fprintf (stderr, "DXF: Warning, Layer %s contains no exportable items and is not set.\n", name);
        fprintf (stderr, "[File: %s: line: %d] Leaving dxf_set_layer () function.\n", __FILE__, __LINE__);
      }
      return 0;
    }
    if (strcmp (name, "invisible") == 0)
    {
      /* do not export the layer with the name "invisible" */
      if (dxf_verbose)
      {
        fprintf (stderr, "DXF: Warning, Layer %s not set.\n", name);
        fprintf (stderr, "[File: %s: line: %d] Leaving dxf_set_layer () function.\n",
          __FILE__, __LINE__);
      }
      return 0;
    }
    if (strcmp (name, "keepout") == 0)
    {
      /* do not export the layer with the name "keepout" */
      if (dxf_verbose)
      {
        fprintf (stderr, "DXF: Warning, Layer %s not set.\n", name);
        fprintf (stderr, "[File: %s: line: %d] Leaving dxf_set_layer () function.\n",
          __FILE__, __LINE__);
      }
      return 0;
    }
    if (SL_TYPE (idx) == SL_ASSY)
    {
      /* do not export the layers with the type SL_ASSY */
      if (dxf_verbose)
      {
        fprintf (stderr, "DXF: Warning, Layer %s with type SL_ASSY not set.\n", name);
        fprintf (stderr, "[File: %s: line: %d] Leaving dxf_set_layer () function.\n", __FILE__, __LINE__);
      }
      return 0;
    }
  }
  if (is_drill && dxf_n_pending_drills)
  {
    int i;
    /* dump pending drills in sequence */
    qsort (dxf_pending_drills, dxf_n_pending_drills,
      sizeof (DxfPendingDrills), dxf_drill_sort);
    for (i = 0; i < dxf_n_pending_drills; i++)
    {
      if (i == 0 || dxf_pending_drills[i].diam != dxf_pending_drills[i - 1].diam)
      {
        if (dxf_verbose)
        {
          /*!
           * \todo this output should go to file in
           * whatever form instead of being put on stderr.
           */
//          fprintf (stderr,
//            "DXF: T%02d\015\012", ap);
        }
      }
      if (dxf_verbose)
      {
        /*!
         * \todo this output should go to file in
         * whatever form instead of being put on stderr.
         */
        fprintf (stderr, "DXF: X:%06d Y:%06ld\n",
          DXF_X(PCB, dxf_pending_drills[i].x),
          DXF_Y(PCB, dxf_pending_drills[i].y));
      }
    }
    free (dxf_pending_drills);
    dxf_n_pending_drills = dxf_max_pending_drills = 0;
    dxf_pending_drills = 0;
  }
  is_drill = (SL_TYPE (idx) == SL_PDRILL || SL_TYPE (idx) == SL_UDRILL);
  is_mask = (SL_TYPE (idx) == SL_MASK);
  current_mask = 0;
  if (group < 0 || group != lastgroup)
  {
    time_t currenttime;
    char utcTime[64];
#ifdef HAVE_GETPWUID
    struct passwd *pwentry;
#endif
    char *sext = "_layer.dxf";
    lastgroup = group;
    dxf_lastX = -1;
    dxf_lastY = -1;
    lastcolor = 0;
    linewidth = -1;
    lastcap = -1;
//    dxf_set_app_layer (c_layerapps);
    c_layerapps++;
    if (dxf_finding_apertures)
    {
      return 1;
    }
//    if (!curapp->nextAperture)
//    {
//      return 0;
//    }
    dxf_maybe_close_file ();
    pagecount++;
    switch (idx)
    {
      case SL (PDRILL, 0):
        sext = ".dxf";
        break;
      case SL (UDRILL, 0):
        sext = ".dxf";
        break;
    }
    strcpy (dxf_filesuffix, layer_type_to_file_name (idx, FNS_first));
    strcat (dxf_filesuffix, sext);
    fp = fopen (dxf_filename, "w");
    if (fp == NULL)
    {
      Message ("DXF: Error, could not open %s for writing.\n", dxf_filename);
      if (dxf_verbose)
      {
        fprintf (stderr, "DXF: Error, could not open %s for writing.\n", dxf_filename);
      }
      return 1;
    }
    /* write version info as a dxf comment */
    dxf_write_comment (fp, PCB_DXF_HID_VERSION);
    /* write dxf header information */
    dxf_write_header ();
    /* write a section ENTITIES marker to the DXF file */
    dxf_write_section (fp, "ENTITIES");
    was_drill = is_drill;
    /*!
     * \todo this output should go to file in
     * whatever form instead of being put on stderr.
     */
    if (dxf_verbose)
    {
      fprintf (stderr, "DXF: Start of page %d for group %d idx %d\n",
        pagecount, group, idx);
    }
    if (group < 0 || group != lastgroup)
    {
      /* create a portable timestamp */
      currenttime = time (NULL);
      /* avoid gcc complaints */
      fmt = strdup ("%c UTC");
      strftime (utcTime, sizeof utcTime, fmt, gmtime (&currenttime));
    }
#ifdef HAVE_GETPWUID
    /* ID the user */
    pwentry = getpwuid (getuid ());
    /*!
     * \todo fprintf (stderr, "For: %s \n", pwentry->pw_name);
     * doesn't behave properly !
     */
#endif
    if (dxf_verbose)
    {
      /* report at the beginning of each file */
      fprintf (stderr, "DXF: Board Name: %s, %s \n", UNKNOWN (PCB->Name),
        UNKNOWN (name));
      fprintf (stderr, "DXF: Created by: %s.\n", PCB_DXF_HID_VERSION);
      fprintf (stderr, "DXF: Creation date: %s \n", utcTime);
      fprintf (stderr, "DXF: File Format according to: AutoCAD R14.\n");
      if (dxf_metric)
      {
        fprintf (stderr, "DXF using Metric coordinates [mm].\n");
        pcb_fprintf (stderr, "PCB Dimensions: %.0mm x %.0mm.\n", PCB->MaxWidth,
          PCB->MaxHeight);
      }
      else
      {
        fprintf (stderr, "DXF using Imperial coordinates [mil].\n");
        pcb_fprintf (stderr, "PCB Dimensions: %.0ml x %.0ml.\n", PCB->MaxWidth,
          PCB->MaxHeight);
      }
      fprintf (stderr, "PCB Coordinate Origin: lower left.\n");
      fprintf (stderr, "DXF: Now processing Layer %s group %d drill %d mask %d\n", name, group,
        is_drill, is_mask);
    }
    /* build a legal identifier */
    if (dxf_layername)
    {
      free (dxf_layername);
    }
    dxf_layername = strdup (dxf_filesuffix);
    dxf_layername[strlen (dxf_layername) - strlen (sext)] = 0;
    /* remove all non-alpha-nummerical characters and change all to upper characters */
    for (cp = dxf_layername; *cp; cp++)
    {
      if (isalnum ((int) *cp))
      {
        *cp = toupper (*cp);
      }
      else
      {
        *cp = '_';
      }
    }
    lncount = 1;
    if (dxf_verbose)
    {
      fprintf (stderr, "DXF: Setting Layer %s.\n", dxf_layername);
//      fprintf (stderr, "DXF: Aperture Data %s.\n", curapp->appList.Data);
    }
  }
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_set_layer () function.\n",
    __FILE__, __LINE__);
#endif
  return 1;
}


/*!
 * \brief Constructor for the graphic context.
 */
static hidGC
dxf_make_gc (void)
{
  hidGC rv;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_make_gc () function.\n", __FILE__, __LINE__);
#endif
  rv = (hidGC) calloc (1, sizeof (hid_gc_struct));
  rv->cap = Trace_Cap;
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_make_gc () function.\n", __FILE__, __LINE__);
#endif
  return rv;
}


/*!
 * \brief Destructor for the graphic context.
 */
static void
dxf_destroy_gc (hidGC gc)
{
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_destroy_gc () function.\n", __FILE__, __LINE__);
#endif
  free (gc);
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_destroy_gc () function.\n", __FILE__, __LINE__);
#endif
}

/*!
 * Special note about the "erase" color: To use this color, you must use this
 * function to tell the HID when you're using it.\n
 * At the beginning of a layer redraw cycle (i.e. after set_layer), call
 * use_mask() to redirect output to a buffer.\n
 * Draw to the buffer (using regular HID calls) using regular and "erase"
 * colors.\n
 * Then call use_mask(HID_MASK_OFF) to flush the buffer to the HID.\n
 * If you use the "erase" color when use_mask is disabled, it simply draws in
 * the background color.\n
 * Values:\n
 * <ul>
 *   <li> <c>HID_MASK_OFF == 0 </c> Flush the buffer and return to non-mask operation.
 *   <li> <c>HID_MASK_BEFORE == 1 </c> Polygons being drawn before clears.
 *   <li> <c>HID_MASK_CLEAR == 2 </c> Clearances being drawn.
 *   <li> <c>HID_MASK_AFTER == 3 </c> Polygons being drawn after clears.
 * </ul>
 */
static void
dxf_use_mask
(
  int use_it
)
{
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_use_mask () function.\n", __FILE__, __LINE__);
#endif
  current_mask = use_it;
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_use_mask () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Set a color.
 *
 * Set the color of the entity.
 * Names can be like "red" or "#rrggbb" or special names like "erase".
 * <b>Always</b> use the "erase" color for removing ink (like polygon reliefs
 * or thermals), as you cannot rely on knowing the background color or special
 * needs of the HID.\n
 * Always use the "drill" color to draw holes.\n
 * You may assume this is cheap enough to call inside the redraw callback,
 * but not cheap enough to call for each item drawn.
 */
static void
dxf_set_color
(
  hidGC gc, /*!< graphic context  */
  const char *name
)
{
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_set_color () function.\n", __FILE__, __LINE__);
#endif
  if (strcmp (name, "erase") == 0)
  {
    gc->color = 1;
    gc->erase = 1;
    gc->drill = 0;
  }
  else if (strcmp (name, "drill") == 0)
  {
    gc->color = 1;
    gc->erase = 0;
    gc->drill = 1;
  }
  else
  {
    gc->color = 0;
    gc->erase = 0;
    gc->drill = 0;
  }
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_set_color () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Set the line style.
 *
 * Set the line cap style in the graphic context.\n
 * While calling this is cheap, calling it with different values each time
 * may be expensive, so grouping items by line style is helpful.
*/
static void
dxf_set_line_cap
(
  hidGC gc,
  EndCapStyle style
)
{
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_set_line_cap () function.\n", __FILE__, __LINE__);
#endif
  gc->cap = style;
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_set_line_cap () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Set the line width.
 *
 * Set the line width in the graphic context.\n
 * While calling this is cheap, calling it with different values each time
 * may be expensive, so grouping items by line width is helpful.
 */
static void
dxf_set_line_width
(
  hidGC gc,
  int width
)
{
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_set_line_width () function.\n", __FILE__, __LINE__);
#endif
  gc->width = width;
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_set_line_width () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Use the graphic context.
 */
static void
dxf_use_gc
(
  hidGC gc,
  int radius
)
{
  int c;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_use_gc () function.\n", __FILE__, __LINE__);
#endif
  if (radius)
  {
    radius *= 2;
    if (radius != linewidth || lastcap != Round_Cap)
    {
//      c = dxf_find_aperture_code (radius, ROUND);
      if (c <= 0)
      {
        fprintf (stderr, "DXF: Error, aperture for radius %d type ROUND is %d\n", radius, c);
      }
      if (fp && !is_drill)
      {
        fprintf (stderr, "DXF: is not a drill %d.\n", c);
      }
      linewidth = radius;
      lastcap = Round_Cap;
    }
  }
  else if (linewidth != gc->width || lastcap != gc->cap)
  {
    linewidth = gc->width;
    lastcap = gc->cap;
    switch (gc->cap)
    {
      case Round_Cap:
      case Trace_Cap:
        c = ROUND;
        break;
      default:
      case Square_Cap:
        c = SQUARE;
        break;
    }
    if (fp)
    {
//      fprintf (stderr, "DXF: aperture %d.\n ", ap);
    }
  }
#if 0
  if (lastcolor != gc->color)
  {
    c = gc->color;
    if (is_drill) return;
    if (is_mask) c = (gc->erase ? 0 : 1);
    lastcolor = gc->color;
    if (fp)
    {
      if (c)
      {
        /*!
         * \todo this output should go to file in
         * whatever form instead of being put on stderr.
         */
        fprintf (stderr, "%%LN%s_C%d*%%\015\012", layername, lncount++);
        fprintf (stderr, "%%LPC*%%\015\012");
      }
      else
      {
        fprintf (stderr, "%%LN%s_D%d*%%\015\012", layername, lncount++);
        fprintf (stderr, "%%LPD*%%\015\012");
      }
    }
  }
#endif
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leavinging dxf_use_gc () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Draw a rectangle.
 *
 * The usual drawing functions.\n
 * "draw" means to use segments of the given width, whereas "fill" means to
 * fill to a zero-width outline.\n
 * We draw the rectangle counter clockwise (CCW) with 5 vertices
 * (XY-coordinates).\n
 * It is assumed that the first XY-coordinate pair (x1, y1) contains the
 * bottom left corner values and that the second XY-coordinate pair (x2, y2)
 * contains the top right corner values. \n
 * The rectangle is not filled, use dxf_fill_rect () for a filled rectangle.
 */
static void
dxf_draw_rect
(
  hidGC gc,
    /*!< graphic context  */
  int x1,
    /*!< X-value bottom left ?? point  */
  int y1,
    /*!< Y-value bottom left ?? point  */
  int x2,
    /*!< X-value top right ?? point  */
  int y2
    /*!< Y-value top right ?? point  */
)
{
  double dxf_start_width;
  double dxf_end_width;
  int dxf_color;
  double dxf_x0;
  double dxf_y0;
  double dxf_x1;
  double dxf_y1;
  double dxf_x2;
  double dxf_y2;
  double dxf_x3;
  double dxf_y3;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_draw_rect () function.\n", __FILE__, __LINE__);
#endif
  /*
   * return if no valid file pointer exists.
   */
  if (!fp)
  {
    fprintf (stderr, "Warning: no valid file pointer exists.\n");
    return;
  }
  if ((x1 == x2) && (y1 == y2))
  {
    fprintf (stderr, "Warning: start point and end point are identical for the entity with id-code: %x\n", dxf_id_code);
    fprintf (stderr, "   entity is discarded from output.\n");
    return;
  }
  dxf_start_width = (double) gc->width;
  dxf_end_width = (double) gc->width;
  if (dxf_color_is_byblock)
  {
    dxf_color = DXF_COLOR_BYBLOCK;
  }
  else dxf_color = gc->color;
  dxf_x0 = DXF_X(PCB, x1);
  dxf_y0 = DXF_Y(PCB, y1);
  dxf_x1 = DXF_X(PCB, x2);
  dxf_y1 = DXF_Y(PCB, y1);
  dxf_x2 = DXF_X(PCB, x1);
  dxf_y2 = DXF_Y(PCB, y2);
  dxf_x3 = DXF_X(PCB, x2);
  dxf_y3 = DXF_Y(PCB, y2);
  /*
   * write polyline sequence.
   */
  dxf_write_polyline
  (
    fp,
    dxf_id_code,
    DXF_DEFAULT_LINETYPE, /* linetype, */
    DXF_DEFAULT_LAYER, /* layer, */
    0.0, /* x0, */ /* the polyline entity <b>always</b> remains on 0.0, 0.0, 0.0  */
    0.0, /* y0, */
    0.0, /* z0, */
    0.0, /* extr_x0, */ /* the polyline extrusion vector <b>always</b> is 0.0, 0.0, 1.0  */
    0.0, /* extr_y0, */
    1.0, /* extr_z0, */
    0.0, /* thickness, */ /* copper weight ??  */
    dxf_start_width,
    dxf_end_width,
    dxf_color, /* color, */
    1, /* vertices_follow, */
    0, /* modelspace, */
    0, /* flag, */
    0, /* polygon_mesh_M_vertex_count, */
    0, /* polygon_mesh_N_vertex_count, */
    0, /* smooth_M_surface_density, */
    0, /* smooth_N_surface_density, */
    0 /* surface_type */
  );
  dxf_id_code++;
  /*
   * write first XY-coordinate (base point, bottom left corner).
   */
  dxf_write_vertex
  (
    fp,
    dxf_id_code,
    DXF_DEFAULT_LINETYPE, /* linetype, */
    DXF_DEFAULT_LAYER, /* layer, */
    dxf_x0,
    dxf_y0,
    0.0, /* z0, */ /* stacked, curved or flexable pcb's ??  */
    0.0, /* thickness, */ /* copper weight ??  */
    dxf_start_width,
    dxf_end_width,
    0.0, /* bulge, */
    0.0, /* curve_fit_tangent_direction, */
    dxf_color,
    0, /* modelspace, */
    0 /* flag */
  );
  dxf_id_code++;
  /*
   * write second XY-coordinate (bottom right corner).
   */
  dxf_write_vertex
  (
    fp,
    dxf_id_code,
    DXF_DEFAULT_LINETYPE, /* linetype, */
    DXF_DEFAULT_LAYER, /* layer, */
    dxf_x1,
    dxf_y0,
    0.0, /* z0, */ /* stacked, curved or flexable pcb's ??  */
    0.0, /* thickness, */ /* copper weight ??  */
    dxf_start_width,
    dxf_end_width,
    0.0, /* bulge, */
    0.0, /* curve_fit_tangent_direction, */
    dxf_color,
    0, /* modelspace, */
    0 /* flag */
  );
  dxf_id_code++;
  /*
   * write third XY-coordinate (top right left corner).
   */
  dxf_write_vertex
  (
    fp,
    dxf_id_code,
    DXF_DEFAULT_LINETYPE, /* linetype, */
    DXF_DEFAULT_LAYER, /* layer, */
    dxf_x1,
    dxf_y1,
    0.0, /* z0, */ /* stacked, curved or flexable pcb's ??  */
    0.0, /* thickness, */ /* copper weight ??  */
    dxf_start_width,
    dxf_end_width,
    0.0, /* bulge, */
    0.0, /* curve_fit_tangent_direction, */
    dxf_color,
    0, /* modelspace, */
    0 /* flag */
  );
  dxf_id_code++;
  /*
   * write fourth XY-coordinate (top left corner).
   */
  dxf_write_vertex
  (
    fp,
    dxf_id_code,
    DXF_DEFAULT_LINETYPE, /* linetype, */
    DXF_DEFAULT_LAYER, /* layer, */
    dxf_x0,
    dxf_y1,
    0.0, /* z0, */ /* stacked, curved or flexable pcb's ??  */
    0.0, /* thickness, */ /* copper weight ??  */
    dxf_start_width,
    dxf_end_width,
    0.0, /* bulge, */
    0.0, /* curve_fit_tangent_direction, */
    dxf_color,
    0, /* modelspace, */
    0 /* flag */
  );
  dxf_id_code++;
  /*
   * write fifth XY-coordinate (again the bottom left corner, to close
   * the rectangle).
   */
  dxf_write_vertex
  (
    fp,
    dxf_id_code,
    DXF_DEFAULT_LINETYPE, /* linetype, */
    DXF_DEFAULT_LAYER, /* layer, */
    dxf_x0,
    dxf_y0,
    0.0, /* z0, */ /* stacked, curved or flexable pcb's ??  */
    0.0, /* thickness, */ /* copper weight ??  */
    dxf_start_width,
    dxf_end_width,
    0.0, /* bulge, */
    0.0, /* curve_fit_tangent_direction, */
    dxf_color,
    0, /* modelspace, */
    0 /* flag */
  );
  dxf_id_code++;
  /*
   * end of polyline sequence.
   */
  dxf_write_endseq (fp);
  dxf_lastX = dxf_x1;
  dxf_lastY = dxf_y1;
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_draw_rect () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Draw a line.
 *
 * The usual drawing functions.\n
 * "draw" means to use segments of the given width, whereas "fill" means to
 * fill to a zero-width outline.\n
 * Translate the pcb X,Y-coordinates of lines and trace segments to dxf
 * X,Y,Z-coordinates.\n
 * Add layer, linetype, color and width values.\n
 * Write a series of polylines and vertices by calling low level functions.\n
 * If the endcap style is ROUND add a donut at the begin and end coordinates
 * of the line segment.\n
 * If the endcap style is SQUARE elongate the line segment with half its
 * width.\n
 * Remarks:\n
 * <ul>
 * <li>We do not draw lines of 1 mil wide or smaller.\n
 * <li>We do not draw lines with identical start and end XY-coordinates (zero
 * length).\n
 * <li>We draw every trace segment as a single AutoCAD entity (polyline).\n
 * </ul>
 *
 * \todo In case of a series of trace segments, we have to continue with a
 * vertex from the last XY-coordinates.\n
 * While the conditions for starting or continuing are simple to determine:\n
 * <c>if ((dxf_x1, dxf_y1) == dxf_lastX, dxf_lastY)) ... </c>\n
 * The caveat is how to determine when to close the polyline sequence
 * (with an ENDSEQ marker) after the last vertex (endpoint of the last trace
 * segment).\n
 * One approach could be to close the series when the start coordinates of the
 * new (to be drawn) trace segment do not coincide with the endpoint of the
 * previously used trace segment (dxf_last[X. Y] values).\n
 * This however would not be a solution for a branching trace segment or the
 * last trace segment to be drawn on that particular layer.\n
 * For the last trace segment to be drawn on a particular layer, we would have
 * to check if the layer didn't change since the last trace segment was
 * drawn.\n
 */
static void
dxf_draw_line
(
  hidGC gc,
    /*!< graphic context  */
  int x1,
    /*!< X-value start point  */
  int y1,
    /*!< Y-value start point  */
  int x2,
    /*!< X-value end point  */
  int y2
    /*!< Y-value end point  */
)
{
  bool m;
  double dxf_x0; /* start point */
  double dxf_y0; /* start point */
  double dxf_x1; /* end point */
  double dxf_y1; /* end point */
  double dxf_start_width; /* trace width */
  double dxf_end_width; /* trace width */
  int dxf_color;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_draw_line () function.\n", __FILE__, __LINE__);
#endif
  m = false;
  if (!fp)
  {
    /* return if no valid file pointer exists */
    fprintf (stderr, "Warning: no valid file pointer exists.\n");
    return;
  }
  if (gc->width == 1)
  {
    /* we do not draw 1 mil width traces */
    fprintf (stderr, "Warning: lines with a width == 1 mil will not be drawn.\n");
    fprintf (stderr, "   entity is discarded from output.\n");
    return;
  }
  if ((x1 == x2) && (y1 == y2))
  {
    /* we do not draw zero length traces */
    fprintf (stderr, "Warning: start point and end point are identical for the entity with id-code: %x.\n", dxf_id_code);
    fprintf (stderr, "   entity is discarded from output.\n");
    return;
  }
  dxf_use_gc (gc, 0);
  /* determine the polyline widths */
  if (dxf_metric)
  {
    dxf_start_width = (double) COORD_TO_MM (gc->width);
    dxf_end_width = (double) COORD_TO_MM (gc->width);
  }
  else
  {
    dxf_start_width = (double) COORD_TO_MIL (gc->width);
    dxf_end_width = (double) COORD_TO_MIL (gc->width);
  }
  /* determine polyline color */
  if (dxf_color_is_byblock)
  {
    dxf_color = DXF_COLOR_BYBLOCK;
  }
  else dxf_color = gc->color;
  /*
   * determine start and end point X,Y-values w.r.t. metric or imperial.
   */
  if (dxf_metric)
  {
    dxf_x0 = COORD_TO_MM (DXF_X(PCB, x1));
    dxf_y0 = COORD_TO_MM (DXF_Y(PCB, y1));
    dxf_x1 = COORD_TO_MM (DXF_X(PCB, x2));
    dxf_y1 = COORD_TO_MM (DXF_Y(PCB, y2));
  }
  else
  {
    dxf_x0 = COORD_TO_MIL (DXF_X(PCB, x1));
    dxf_y0 = COORD_TO_MIL (DXF_Y(PCB, y1));
    dxf_x1 = COORD_TO_MIL (DXF_X(PCB, x2));
    dxf_y1 = COORD_TO_MIL (DXF_Y(PCB, y2));
  }
  /*!
   * \todo Someday we have to do something here with multiple trace
   * segments here, the problem for now is how to determine when the
   * last trace segment was passed.
   */
  if ((dxf_x0 == dxf_lastX) && (dxf_y0 == dxf_lastY))
  {
    m = true;
  }
  /*
   * This is just a dirty hack for AutoCAD doesn't have endcap styles.
   * Donuts can not be implementend in the trace polyline since donuts
   * are a closed polyline themselves.
   */
  if (gc->cap == ROUND)
  {
    /* place a donut at the start of the trace segment */
    dxf_write_polyline
    (
      fp,
      dxf_id_code,
      DXF_DEFAULT_LINETYPE, /* linetype, */
      DXF_DEFAULT_LAYER, /* layer, */
      0.0, /* x0, */
      0.0, /* y0, */
      0.0, /* z0, */
      0.0, /* extr_x0, */
      0.0, /* extr_y0, */
      1.0, /* extr_z0, */
      0.0, /* thickness, copper weight ??  */
      0.5 * dxf_start_width,
      0.5 * dxf_end_width,
      dxf_color, /* color, */
      1, /* vertices_follow, */
      0, /* modelspace, */
      1, /* flag, */
      0, /* polygon_mesh_M_vertex_count, */
      0, /* polygon_mesh_N_vertex_count, */
      0, /* smooth_M_surface_density, */
      0, /* smooth_N_surface_density, */
      0 /* surface_type */
    );
    dxf_id_code++;
    /*
     * write first XY-coordinate (at the start of trace segment).
     */
    dxf_write_vertex
    (
      fp,
      dxf_id_code,
      DXF_DEFAULT_LINETYPE, /* linetype, */
      DXF_DEFAULT_LAYER, /* layer, */
      dxf_x0 - (0.25 * dxf_start_width),
      dxf_y0,
      0.0, /* z0, */ /* stacked, curved or flexable pcb's ?? */
      0.0, /* thickness, */ /* copper weight ?? */
      0.5 * dxf_start_width,
      0.5 * dxf_end_width,
      1.0, /* bulge, */
      0.0, /* curve_fit_tangent_direction, */
      dxf_color,
      0, /* modelspace, */
      0 /* flag */
    );
    dxf_id_code++;
    /*
     * write second XY-coordinate (at the start of trace segment).
     */
    dxf_write_vertex
    (
      fp,
      dxf_id_code,
      DXF_DEFAULT_LINETYPE, /* linetype, */
      DXF_DEFAULT_LAYER, /* layer, */
      dxf_x0 + (0.25 * dxf_start_width),
      dxf_y0,
      0.0, /* z0, */ /* stacked, curved or flexable pcb's ??  */
      0.0, /* thickness, */ /* copper weight ?? */
      0.5 * dxf_start_width,
      0.5 * dxf_end_width,
      1.0, /* bulge, */
      0.0, /* curve_fit_tangent_direction, */
      dxf_color,
      0, /* modelspace, */
      0 /* flag */
    );
    dxf_id_code++;
    /* write the end of polyline sequence marker */
    dxf_write_endseq (fp);
    /* place a donut at the end of the trace segment */
    dxf_write_polyline
    (
      fp,
      dxf_id_code,
      DXF_DEFAULT_LINETYPE, /* linetype, */
      DXF_DEFAULT_LAYER, /* layer, */
      0.0, /* x0, */
      0.0, /* y0, */
      0.0, /* z0, */
      0.0, /* extr_x0, */
      0.0, /* extr_y0, */
      1.0, /* extr_z0, */
      0.0, /* thickness, */ /* copper weight ?? */
      0.5 * dxf_start_width,
      0.5 * dxf_end_width,
      dxf_color, /* color, */
      1, /* vertices_follow, */
      0, /* modelspace, */
      1, /* flag, */
      0, /* polygon_mesh_M_vertex_count, */
      0, /* polygon_mesh_N_vertex_count, */
      0, /* smooth_M_surface_density, */
      0, /* smooth_N_surface_density, */
      0 /* surface_type */
    );
    dxf_id_code++;
    /*
     * write first XY-coordinate (at the end of trace segment).
     */
    dxf_write_vertex
    (
      fp,
      dxf_id_code,
      DXF_DEFAULT_LINETYPE, /* linetype, */
      DXF_DEFAULT_LAYER, /* layer, */
      dxf_x1 - (0.25 * dxf_start_width),
      dxf_y1,
      0.0, /* z0, */ /* stacked, curved or flexable pcb's ?? */
      0.0, /* thickness, */ /* copper weight ??  */
      0.5 * dxf_start_width,
      0.5 * dxf_end_width,
      1.0, /* bulge, */
      0.0, /* curve_fit_tangent_direction, */
      dxf_color,
      0, /* modelspace, */
      0 /* flag */
    );
    dxf_id_code++;
    /*
     * write second XY-coordinate (at the end of trace segment).
     */
    dxf_write_vertex
    (
      fp,
      dxf_id_code,
      DXF_DEFAULT_LINETYPE, /* linetype, */
      DXF_DEFAULT_LAYER, /* layer, */
      dxf_x1 + (0.25 * dxf_start_width),
      dxf_y1,
      0.0, /* z0, */ /* stacked, curved or flexable pcb's ??  */
      0.0, /* thickness, */ /* copper weight ??  */
      0.5 * dxf_start_width,
      0.5 * dxf_end_width,
      1.0, /* bulge, */
      0.0, /* curve_fit_tangent_direction, */
      dxf_color,
      0, /* modelspace, */
      0 /* flag */
    );
    dxf_id_code++;
    /*
     * write the end of polyline sequence marker.
     */
    dxf_write_endseq (fp);
  }
  /* if the end cap style is an OCTAGON: ?? */
  if (gc->cap == OCTAGON)
  {
    /*!
     * \todo This end cap style has yet to be implemented at the
     * start and end point of a trace.
     * Note: done for ROUND and SQUARE.
     */
  }
  /*
   * if the end cap style is SQUARE: recompute the start and end
   * coordinates, that is, elongate the trace with half of the width.
   */
  if (gc->cap == SQUARE)
  {
    double length; /* trace length */
    double dxf_x0_1; /* extended start point */
    double dxf_y0_1; /* extended start point */
    double dxf_x1_1; /* extended end point */
    double dxf_y1_1; /* extended end point */
    length = sqrt ((dxf_y1 - dxf_y0) * (dxf_y1 - dxf_y0) +
      (dxf_x1 - dxf_x0) * (dxf_x1 - dxf_x0));
    dxf_x0_1 = dxf_x0 - ((dxf_x1 - dxf_x0) / length) * 0.5 * dxf_start_width;
    dxf_y0_1 = dxf_y0 - ((dxf_y1 - dxf_y0) / length) * 0.5 * dxf_start_width;
    dxf_x1_1 = dxf_x1 + ((dxf_x1 - dxf_x0) / length) * 0.5 * dxf_end_width;
    dxf_y1_1 = dxf_y1 + ((dxf_y1 - dxf_y0) / length) * 0.5 * dxf_end_width;
    dxf_x0 = dxf_x0_1;
    dxf_y0 = dxf_y0_1;
    dxf_x1 = dxf_x1_1;
    dxf_y1 = dxf_y1_1;
  }
  /* write polyline sequence for the trace */
  dxf_write_polyline
  (
    fp,
    dxf_id_code,
    DXF_DEFAULT_LINETYPE, /* linetype, */
    DXF_DEFAULT_LAYER, /* layer, */
    0.0, /* x0, */
    0.0, /* y0, */
    0.0, /* z0, */
    0.0, /* extr_x0, */
    0.0, /* extr_y0, */
    1.0, /* extr_z0, */
    0.0, /* thickness, */ /* copper weight ?? */
    dxf_start_width,
    dxf_end_width,
    dxf_color, /* color, */
    1, /* vertices_follow, */
    0, /* modelspace, */
    0, /* flag, */
    0, /* polygon_mesh_M_vertex_count, */
    0, /* polygon_mesh_N_vertex_count, */
    0, /* smooth_M_surface_density, */
    0, /* smooth_N_surface_density, */
    0 /* surface_type */
  );
  dxf_id_code++;
  /* write first XY-coordinate (start of trace segment) */
  dxf_write_vertex
  (
    fp,
    dxf_id_code,
    DXF_DEFAULT_LINETYPE, /* linetype, */
    DXF_DEFAULT_LAYER, /* layer, */
    dxf_x0,
    dxf_y0,
    0.0, /* z0, */ /* stacked, curved or flexable pcb's ??  */
    0.0, /* thickness, */ /* copper weight ??  */
    dxf_start_width,
    dxf_end_width,
    0.0, /* bulge, */
    0.0, /* curve_fit_tangent_direction, */
    dxf_color,
    0, /* modelspace, */
    0 /* flag */
  );
  dxf_id_code++;
  /* write second XY-coordinate (end of trace segment) */
  dxf_write_vertex
  (
    fp,
    dxf_id_code,
    DXF_DEFAULT_LINETYPE, /* linetype, */
    DXF_DEFAULT_LAYER, /* layer, */
    dxf_x1,
    dxf_y1,
    0.0, /* z0, */ /* stacked, curved or flexable pcb's ??  */
    0.0, /* thickness, */ /* copper weight ??  */
    dxf_start_width,
    dxf_end_width,
    0.0, /* bulge, */
    0.0, /* curve_fit_tangent_direction, */
    dxf_color,
    0, /* modelspace, */
    0 /* flag */
  );
  dxf_id_code++;
  /* write the end of polyline sequence marker */
  dxf_write_endseq (fp);
  dxf_lastX = dxf_x1;
  dxf_lastY = dxf_y1;
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_draw_line () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Draw an (elliptic ?) arc.
 *
 * The usual drawing functions.\n
 * "draw" means to use segments of the given width, whereas "fill" means to
 * fill to a zero-width outline.\n
 * For now we draw an (elliptic) arc with the assumption that the width is
 * along the X-axis and the height is along the Y-axis.\n
 * Thus the major axis of the ellipse has a size of 2 * the maximum of the
 * greatest value of [width, height], and the minor axis has a size of 2 * the
 * minimum value of [width, height].\n
 * An elliptic arc with a line width of 0 is implemented for now.
 *
 * \todo The elliptic arc entity has to be replaced by a polyline with the
 * correct line width (trace width).
 * \todo In the case of a series of trace segments, continue with a vertex
 * from the last XY-coordinates.\n
 * While the conditions for starting or continuing are simple to determine:\n
 * <c>if ((x1, y1) == dxf_lastX, dxf_lastY)) ... </c> \n
 * The caveat is how to determine when to close the polyline sequence (with
 * an ENDSEQ marker) after the last vertex (endpoint of the last trace
 * segment).\n
 * \todo The end cap style has to be implemented at the start and end point of
 * a trace.
 */
static void
dxf_draw_arc
(
  hidGC gc,
    /*!< graphic context  */
  int cx,
    /*!< X-value center point  */
  int cy,
    /*!< X-value center point  */
  int width,
    /*!< length of major axis  */
  int height,
    /*!< length of minor axis  */
  int start_angle,
    /*!< start angle of elliptic arc  */
  int delta_angle
    /*!< relative angle to end angle  */
)
{
  float arcStartX;
  float arcStopX;
  float arcStartY;
  float arcStopY;
  double dxf_x0; /* center point */
  double dxf_y0; /* center point */
  double dxf_x1; /* end point major axis */
  double dxf_y1; /* end point major axis */
  double dxf_arcstart_x;
  double dxf_arcstart_y;
  double dxf_arcstop_x;
  double dxf_arcstop_y;
  double dxf_start_width; /* trace width */
  double dxf_end_width; /* trace width */
  double dxf_width; /* arc width */
  double dxf_height; /* arc height */
  double dxf_ratio;
  double dxf_start_angle;
  double dxf_end_angle;
  int dxf_color;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_draw_arc () function.\n", __FILE__, __LINE__);
#endif
#if 0
  bool m = False;
#endif
  if (!fp)
  {
    /* return if no valid file pointer exists */
    fprintf (stderr, "Warning: no valid file pointer exists.\n");
    return;
  }
  if (gc->width == 0)
  {
    /* we do not draw 0 mil wide traces */
    fprintf (stderr, "Warning: arcs with a width == 0 mil will not be drawn.\n");
    fprintf (stderr, "         entity is discarded from output.\n");
    return;
  }
  dxf_use_gc (gc, 0);
  arcStartX = cx - width * cos (TO_RADIANS (start_angle));
  arcStartY = cy + height * sin (TO_RADIANS (start_angle));
  arcStopX = cx - width * cos (TO_RADIANS (start_angle + delta_angle));
  arcStopY = cy + height * sin (TO_RADIANS (start_angle + delta_angle));
  if (dxf_metric)
  {
    /* use metric (mm) */
    dxf_x0 = COORD_TO_MM (DXF_X(PCB, cx));
    dxf_y0 = COORD_TO_MM (DXF_Y(PCB, cy));
    dxf_start_width = COORD_TO_MM (DXF_X(PCB, width));
    dxf_end_width = COORD_TO_MM (DXF_X(PCB, width));
    dxf_height = COORD_TO_MM (DXF_X(PCB, height));
    dxf_width = COORD_TO_MM (DXF_X(PCB, width));
    if (dxf_width > dxf_height)
    {
      /*
       * the major axis of the ellipse coincides with the
       * X-axis.
       */
      dxf_x1 = COORD_TO_MM (DXF_X(PCB, (cx + width)));
      dxf_y1 = COORD_TO_MM (DXF_Y(PCB, cy));
      /*
       * the dxf_ratio is the minor axis length over major
       * axis length, and is always <= 1.0
       */
      dxf_ratio = dxf_height / dxf_width;
    }
    else
    {
      /*
       * the major axis of the ellipse coincides with the
       * Y-axis.
       */
      dxf_x1 = COORD_TO_MM (DXF_X(PCB, cx));
      dxf_y1 = COORD_TO_MM (DXF_Y(PCB, (cy + height)));
      /*
       * the dxf_ratio is the minor axis length over major
       * axis length, and is always <= 1.0
       */
      dxf_ratio = dxf_width / dxf_height;
    }
  }
  else
  {
    /* use imperial (mil) */
    dxf_x0 = COORD_TO_MIL (DXF_X(PCB, cx));
    dxf_y0 = COORD_TO_MIL (DXF_Y(PCB, cy));
    dxf_start_width = COORD_TO_MIL (DXF_X(PCB, width));
    dxf_end_width = COORD_TO_MIL (DXF_X(PCB, width));
    dxf_height = COORD_TO_MIL (DXF_Y(PCB, height));
    dxf_width = COORD_TO_MIL (DXF_Y(PCB, width));
    if (dxf_width > dxf_height)
    {
      /*
       * the major axis of the ellipse coincides with the
       * X-axis.
       */
      dxf_x1 = DXF_X(PCB, (cx + width));
      dxf_y1 = DXF_Y(PCB, cy);
      /*
       * the dxf_ratio is the minor axis length over major
       * axis length, and is always <= 1.0
       */
      dxf_ratio = dxf_height / dxf_width;
    }
    else
    {
      /*
       * the major axis of the ellipse coincides with the
       * Y-axis.
       */
      dxf_x1 = DXF_X(PCB, cx);
      dxf_y1 = DXF_Y(PCB, (cy + height));
      /*
       * the dxf_ratio is the minor axis length over major
       * axis length, and is always <= 1.0
       */
      dxf_ratio = dxf_width / dxf_height;
    }
  }
  /*
   * we have to add 180 degrees for start_angle and end_angle because
   * in the pcb universe 0 degrees (the negative X-axis) is to the left,
   * and in the dxf universe 0 degrees is to the right.
   */
  dxf_start_angle = TO_RADIANS (start_angle + 180);
  dxf_end_angle = TO_RADIANS (start_angle + delta_angle + 180);
  if (dxf_start_angle >= (2 * M_PI))
  {
    dxf_start_angle = dxf_start_angle - (2 * M_PI);
  }
  if (dxf_end_angle >= (2 * M_PI))
  {
    dxf_end_angle = dxf_end_angle - (2 * M_PI);
  }
  if (dxf_color_is_byblock)
  {
    dxf_color = DXF_COLOR_BYBLOCK;
  }
  else dxf_color = gc->color;
  dxf_arcstart_x = DXF_X(PCB, arcStartX);
  dxf_arcstart_y = DXF_Y(PCB, arcStartY);
  dxf_arcstop_x = DXF_X(PCB, arcStopX);
  dxf_arcstop_y = DXF_Y(PCB, arcStopY);
  /*
   * This is just a dirty hack for AutoCAD doesn't have endcap styles.
   * Donuts can not be implemented in the trace polyline since donuts
   * are a closed polyline themselves.
   */
  if (gc->cap == ROUND)
  {
    /* place a donut at the start of the trace segment */
    dxf_write_polyline
    (
      fp,
      dxf_id_code,
      DXF_DEFAULT_LINETYPE, /* linetype, */
      DXF_DEFAULT_LAYER, /* layer, */
      0.0, /* x0, */
      0.0, /* y0, */
      0.0, /* z0, */
      0.0, /* extr_x0, */
      0.0, /* extr_y0, */
      1.0, /* extr_z0, */
      0.0, /* thickness, */ /* copper weight ?? */
      0.5 * dxf_start_width,
      0.5 * dxf_end_width,
      dxf_color, /* color, */
      1, /* vertices_follow, */
      0, /* modelspace, */
      1, /* flag, */
      0, /* polygon_mesh_M_vertex_count, */
      0, /* polygon_mesh_N_vertex_count, */
      0, /* smooth_M_surface_density, */
      0, /* smooth_N_surface_density, */
      0 /* surface_type */
    );
    dxf_id_code++;
    /*
     * write first XY-coordinate (at the start of trace segment).
     */
    dxf_write_vertex
    (
      fp,
      dxf_id_code,
      DXF_DEFAULT_LINETYPE, /* linetype, */
      DXF_DEFAULT_LAYER, /* layer, */
      dxf_arcstart_x - (0.25 * dxf_start_width),
      dxf_arcstart_y,
      0.0, /* z0, */ /* stacked, curved or flexable pcb's ?? */
      0.0, /* thickness, */ /* copper weight ?? */
      0.5 * dxf_start_width,
      0.5 * dxf_end_width,
      1.0, /* bulge, */
      0.0, /* curve_fit_tangent_direction, */
      dxf_color,
      0, /* modelspace, */
      0 /* flag */
    );
    dxf_id_code++;
    /*
     * write second XY-coordinate (at the start of trace segment).
     */
    dxf_write_vertex
    (
      fp,
      dxf_id_code,
      DXF_DEFAULT_LINETYPE, /* linetype, */
      DXF_DEFAULT_LAYER, /* layer, */
      dxf_arcstart_x + (0.25 * dxf_start_width),
      dxf_arcstart_y,
      0.0, /* z0, */ /* stacked, curved or flexable pcb's ?? */
      0.0, /* thickness, */ /* copper weight ?? */
      0.5 * dxf_start_width,
      0.5 * dxf_end_width,
      1.0, /* bulge, */
      0.0, /* curve_fit_tangent_direction, */
      dxf_color,
      0, /* modelspace, */
      0 /* flag */
    );
    dxf_id_code++;
    /*
     * write the end of polyline sequence marker.
     */
    dxf_write_endseq (fp);
    /*
     * place a donut at the end of the trace segment.
     */
    dxf_write_polyline
    (
      fp,
      dxf_id_code,
      DXF_DEFAULT_LINETYPE, /* linetype, */
      DXF_DEFAULT_LAYER, /* layer, */
      0.0, /* x0, */
      0.0, /* y0, */
      0.0, /* z0, */
      0.0, /* extr_x0, */
      0.0, /* extr_y0, */
      1.0, /* extr_z0, */
      0.0, /* thickness, */ /* copper weight ?? */
      0.5 * dxf_start_width,
      0.5 * dxf_end_width,
      dxf_color, /* color, */
      1, /* vertices_follow, */
      0, /* modelspace, */
      1, /* flag, */
      0, /* polygon_mesh_M_vertex_count, */
      0, /* polygon_mesh_N_vertex_count, */
      0, /* smooth_M_surface_density, */
      0, /* smooth_N_surface_density, */
      0 /* surface_type */
    );
    dxf_id_code++;
    /*
     * write first XY-coordinate (at the end of trace segment).
     */
    dxf_write_vertex
    (
      fp,
      dxf_id_code,
      DXF_DEFAULT_LINETYPE, /* linetype, */
      DXF_DEFAULT_LAYER, /* layer, */
      dxf_arcstop_x - (0.25 * dxf_start_width),
      dxf_arcstop_y,
      0.0, /* z0, */ /* stacked, curved or flexable pcb's ?? */
      0.0, /* thickness, */ /* copper weight ?? */
      0.5 * dxf_start_width,
      0.5 * dxf_end_width,
      1.0, /* bulge, */
      0.0, /* curve_fit_tangent_direction, */
      dxf_color,
      0, /* modelspace, */
      0 /* flag */
    );
    dxf_id_code++;
    /*
     * write second XY-coordinate (at the end of trace segment).
     */
    dxf_write_vertex
    (
      fp,
      dxf_id_code,
      DXF_DEFAULT_LINETYPE, /* linetype, */
      DXF_DEFAULT_LAYER, /* layer, */
      dxf_arcstop_x + (0.25 * dxf_start_width),
      dxf_arcstop_y,
      0.0, /* z0, */ /* stacked, curved or flexable pcb's ?? */
      0.0, /* thickness, */ /* copper weight ?? */
      0.5 * dxf_start_width,
      0.5 * dxf_end_width,
      1.0, /* bulge, */
      0.0, /* curve_fit_tangent_direction, */
      dxf_color,
      0, /* modelspace, */
      0 /* flag */
    );
    dxf_id_code++;
    /*
     * write the end of polyline sequence marker.
     */
    dxf_write_endseq (fp);
  }
  /*
   * write an ellipse for the trace.
   */
  dxf_write_ellipse
  (
    fp,
    dxf_id_code,
    DXF_DEFAULT_LINETYPE, /* linetype, */
    DXF_DEFAULT_LAYER, /* layer, */
    dxf_x0,
    dxf_y0,
    0.0, /* z0, */
    dxf_x1,
    dxf_y1,
    0.0, /* z1, */ /* stacked, curved or flexable pcb's ?? */
    0.0, /* dxf_extr_x0, */
    0.0, /* dxf_extr_y0, */
    1.0, /* dxf_extr_z0, */
    0.0, /* thickness, */ /* copper weight ?? */
    dxf_ratio,
    dxf_start_angle,
    dxf_end_angle,
    dxf_color,
    0 /* modelspace */
  );
  dxf_id_code++;
  dxf_lastX = arcStopX;
  dxf_lastY = arcStopY;
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_draw_arc () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Draw a filled circle.
 *
 * The usual drawing functions.\n
 * "draw" means to use segments of the given width, whereas "fill" means to
 * fill to a zero-width outline.\n
 * \todo Implement a donut (polyline) instead of a circle.
 */
static void
dxf_fill_circle
(
  hidGC gc,
    /*!< graphic context. */
  int cx,
    /*!< X-value center point. */
  int cy,
    /*!< Y-value center point. */
  int radius
    /*!< radius of circle. */
)
{
  double dxf_x0;
  double dxf_y0;
  double dxf_radius;
  int dxf_color;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_fill_circle () function.\n", __FILE__, __LINE__);
#endif
  /*
   * return if no valid file pointer exists.
   */
  if (!fp)
  {
    fprintf (stderr, "Warning: no valid file pointer exists.\n");
    return;
  }
  /*
   * drill sizes increase per 2 mil ?
   */
  if (is_drill)
  {
    radius = DXF_ROUND(radius*2) / 2;
  }
  dxf_use_gc (gc, radius);
  if (is_drill)
  {
    if (dxf_n_pending_drills >= dxf_max_pending_drills)
    {
      dxf_max_pending_drills += 100;
      /*
       * re-allocate for another 100 pending drills.
       */
      dxf_pending_drills = (DxfPendingDrills *) realloc (dxf_pending_drills, dxf_max_pending_drills * sizeof (DxfPendingDrills));
    }
    dxf_pending_drills[dxf_n_pending_drills].x = cx;
    dxf_pending_drills[dxf_n_pending_drills].y = cy;
    dxf_pending_drills[dxf_n_pending_drills].diam = radius * 2;
    dxf_n_pending_drills++;
    return;
  }
  else if (gc->drill) return;
  if (dxf_metric) /* use metric mm */
  {
    dxf_x0 = COORD_TO_MM (DXF_X(PCB, cx));
    dxf_y0 = COORD_TO_MM (DXF_Y(PCB, cy));
    dxf_radius = COORD_TO_MM (DXF_X(PCB, radius));
  }
  else /* use imperial mil */
  {
    dxf_x0 = COORD_TO_MIL (DXF_X(PCB, cx));
    dxf_y0 = COORD_TO_MIL (DXF_Y(PCB, cy));
    dxf_radius = COORD_TO_MIL (DXF_X(PCB, radius));
  }
  if (dxf_color_is_byblock)
  {
    dxf_color = DXF_COLOR_BYBLOCK;
  }
  else dxf_color = gc->color;
  dxf_write_circle
  (
    fp,
    dxf_id_code,
    DXF_DEFAULT_LINETYPE, /* linetype, */
    DXF_DEFAULT_LAYER, /* layer, */
    dxf_x0,
    dxf_y0,
    0.0, /* z0, */ /* curved or flexable pcb's ??  */
    0.0, /* dxf_extr_x0, */
    0.0, /* dxf_extr_y0, */
    1.0, /* dxf_extr_z0, */
    0.0, /* thickness, */ /* copper weight ??  */
    dxf_radius,
    dxf_color,
    0 /* modelspace */
  );
  dxf_id_code++;
  dxf_lastX = dxf_x0;
  dxf_lastY = dxf_y0;
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_fill_circle () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Draw a filled polygon.
 *
 * The usual drawing functions.\n
 * "draw" means to use segments of the given width, whereas "fill" means to
 * fill to a zero-width outline.\n
 * A polygon is drawn with a solid fill pattern.
 *
 * The filled polygon is by drawn by a (closed) polyline sequence with
 * (n_coords + 1) vertices and add a SOLID hatch pattern to this polyline.\n
 * This solution would allow for thieving if it were ever implemented in pcb
 * (select a hatch pattern, create a boundary path, apply a scale and all the
 * other stuff that is needed).
 */
static void
dxf_fill_polygon
(
  hidGC gc,
    /*!< graphic context. */
  int n_coords,
    /*!< number of XY-coordinates. */
  int *x,
    /*!< pointer to array of X-values of coordinates. */
  int *y
    /*!< pointer to array of Y-values of coordinates. */
)
{
  bool m;
  int i;
  double dxf_x0;
  double dxf_y0;
  int dxf_color;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_fill_polygon () function.\n", __FILE__, __LINE__);
#endif
  dxf_x0 = 0.0;
  dxf_y0 = 0.0;
  dxf_color = DXF_COLOR_BYLAYER;

  m = false;
  if (is_mask && current_mask == HID_MASK_BEFORE)
  {
    return;
  }
  dxf_use_gc (gc, 10 * 100);
  /*
   * return if no valid file pointer exists.
   */
  if (!fp)
  {
    fprintf (stderr, "Warning: no valid file pointer exists.\n");
    return;
  }
  if (dxf_color_is_byblock)
  {
    dxf_color = DXF_COLOR_BYBLOCK;
  }
  else dxf_color = gc->color;
  /*
   * write hatch sequence.
   */
  dxf_write_hatch
  (
    fp,
    DXF_DEFAULT_HATCH_PATTERN_NAME, /* pattern_name, */
    dxf_id_code,
    DXF_DEFAULT_LINETYPE, /* linetype, */
    DXF_DEFAULT_LAYER, /* layer, */
    dxf_x0,
    dxf_y0,
    0.0, /* z0, */ /* stacked, curved or flexable pcb's ?? */
    0.0, /* extr_x0, */
    0.0, /* extr_y0, */
    1.0, /* extr_z0, */
    0.0, /* thickness, */ /* copper weight ?? */
    1.0, /* pattern_scale, */
    0.0, /* pixel_size, */
    45.0, /* pattern_angle, */
    dxf_color,
    0, /* modelspace, */
    1, /* solid_fill, */
    1, /* associative, */
    0, /* style, */
    1, /* pattern_style, */
    0, /* pattern_double, */
    0, /* pattern_def_lines, */
    1, /* boundary_paths, */
    0, /* seed_points, */
    0, /* seed_x0, */
    0  /* seed_y0, */
  );
  /*
   * draw hatch boundary path polyline.
   */
  dxf_write_hatch_boundary_path_polyline
  (
    fp,
    2, /* path_type_flag, */ /* 2 = polyline  */
    0, /* polyline_has_bulge, */ /* 0 = polygons have sharp angles  */
    1, /* polyline_is_closed, */ /* 1 = closed  */
    n_coords + 1 /* polyline_vertices, */ /* number of polyline vertices to follow  */
  );
  /*
   * draw hatch boundary polyline vertices, write (n_coords)
   * XY-coordinates.
   */
  for (i = 0; i < n_coords; i++)
  {
    dxf_x0 = DXF_X(PCB, x[i]);
    dxf_y0 = DXF_Y(PCB, y[i]);
    dxf_write_hatch_boundary_path_polyline_vertex
    (
      fp,
      dxf_x0,
      dxf_y0,
      0.0 /* dxf_z0 */
    );
    /*
     * close polyline with first coordinate X-Y pair.
     */
    dxf_x0 = DXF_X(PCB, x[0]);
    dxf_y0 = DXF_Y(PCB, y[0]);
    dxf_write_hatch_boundary_path_polyline_vertex
    (
      fp,
      dxf_x0,
      dxf_y0,
      0.0 /* dxf_z0 */
    );
  }
  dxf_id_code++;
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_fill_polygon () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief Draw a filled rectangle.
 *
 * The usual drawing functions.\n
 * "draw" means to use segments of the given width, whereas "fill" means to
 * fill to a zero-width outline.\n
 */
static void
dxf_fill_rect
(
  hidGC gc,
    /*!< graphic context. */
  int x1,
    /*!< X-value bottom left ?? point. */
  int y1,
    /*!< Y-value bottom left ?? point. */
  int x2,
    /*!< X-value top right ?? point. */
  int y2
    /*!< Y-value top right ?? point. */
)
{
  int dxf_color;
  double dxf_x0;
  double dxf_y0;
  double dxf_x1;
  double dxf_y1;
  double dxf_x2;
  double dxf_y2;
  double dxf_x3;
  double dxf_y3;

#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Entering dxf_fill_rect () function.\n", __FILE__, __LINE__);
#endif
  /*
   * return if no valid file pointer exists.
   */
  if (!fp)
  {
    fprintf (stderr, "Warning: no valid file pointer exists.\n");
    return;
  }
  if ((x1 == x2) && (y1 == y2))
  {
    fprintf (stderr, "Warning: start point and end point are identical for the entity with id-code: %x\n", dxf_id_code);
    fprintf (stderr, "   entity is discarded from output.\n");
    return;
  }
  if (dxf_color_is_byblock)
  {
    dxf_color = DXF_COLOR_BYBLOCK;
  }
  else dxf_color = gc->color;
  dxf_x0 = DXF_X(PCB, x1);
  dxf_y0 = DXF_Y(PCB, y1);
  dxf_x1 = DXF_X(PCB, x2);
  dxf_y1 = DXF_Y(PCB, y1);
  dxf_x2 = DXF_X(PCB, x1);
  dxf_y2 = DXF_Y(PCB, y2);
  dxf_x3 = DXF_X(PCB, x2);
  dxf_y3 = DXF_Y(PCB, y2);
  dxf_write_solid
  (
    fp,
    dxf_id_code,
    DXF_DEFAULT_LINETYPE, /* linetype, */
    DXF_DEFAULT_LAYER, /* layer, */
    dxf_x0, /* base point, bottom left  */
    dxf_y0,
    0.0, /* z0, */
    dxf_x1, /* alignment point, bottom right  */
    dxf_y1,
    0.0, /* z1, */
    dxf_x2, /* alignment point, top left  */
    dxf_y2,
    0.0, /* z2, */
    dxf_x3, /* alignment point, top right  */
    dxf_y3,
    0.0, /* z3, */
    0.0, /* thickness, */
    dxf_color,
    0 /* modelspace */
  );
  dxf_id_code++;
#if DEBUG
  fprintf (stderr, "[File: %s: line: %d] Leaving dxf_fill_rect () function.\n", __FILE__, __LINE__);
#endif
}


/*!
 * \brief This is for the printer.
 *
 * If you call this for the GUI, xval and yval are ignored, and a dialog pops
 * up to lead you through the calibration procedure.\n
 * For the printer, if xval and yval are zero, a calibration page is printed
 * with instructions for calibrating your printer.\n
 * After calibrating, nonzero xval and yval are passed according to the
 * instructions.\n
 * Metric is nonzero if the user prefers metric units, else inches are used.\n
 * Calibrate a DXF file ?.\n
 * Since we do not calibrate a DXF file, we ignore this one.
 */
static void
dxf_calibrate
(
  double xval,
    /*!< X-value. */
  double yval
    /*!< Y-value. */
)
{
  /* Intentionally: do nothing here */
}


/*!
 * \brief Sets the crosshair.
 *
 * Which may differ from the pointer depending on grid and pad snap.\n
 * Note that the HID is responsible for hiding, showing, redrawing, etc.\n
 * The core just tells it what coordinates it's actually using.\n
 * Note that this routine may need to know what "pcb units" are so it can
 * display them in mm or mils accordingly.\n
 * Set a crosshair in a DXF file ?.\n
 * Since it is useless to set a crosshair in a DXF file, we ignore this one.
 */
static void
dxf_set_crosshair
(
  int x,
    /*!< X-value of coordinate. */
  int y
    /*!< Y-value of coordinate. */
)
{
  /* Intentionally: do nothing here */
}


/*!
 * \brief Show item ?.
 */
static void
dxf_show_item (void *item)
{
}


/*!
 * \brief Send beep signal to stdout ?.
 */
static void
dxf_beep (void)
{
  putchar (7);
  fflush (stdout);
}


/*!
 * \brief Show progress ?.
 */
static void
dxf_progress (int dxf_so_far, int dxf_total, const char *dxf_message)
{
}


/*!
 * \brief Call this as soon as possible from main().
 *
 * Initialise and register the DXF HID.
 * No other HID calls are valid until this is called.
 */
void
hid_dxf_init ()
{
  memset (&dxf_hid, 0, sizeof (HID));

  common_nogui_init (&dxf_hid);
  common_draw_helpers_init (&dxf_hid);
  dxf_hid.struct_size         = sizeof (HID);
  dxf_hid.name                = "dxf";
  dxf_hid.description         = "DXF export";
  dxf_hid.exporter            = 1;
  dxf_hid.poly_before         = 1;

  dxf_hid.get_export_options = dxf_get_export_options;
  dxf_hid.do_export = dxf_do_export;
  dxf_hid.parse_arguments = dxf_parse_arguments;
  dxf_hid.set_layer = dxf_set_layer;
  dxf_hid.make_gc = dxf_make_gc;
  dxf_hid.destroy_gc = dxf_destroy_gc;
  dxf_hid.use_mask = dxf_use_mask;
  dxf_hid.set_color = dxf_set_color;
  dxf_hid.set_line_cap = dxf_set_line_cap;
  dxf_hid.set_line_width = dxf_set_line_width;
  dxf_hid.draw_line = dxf_draw_line;
  dxf_hid.draw_arc = dxf_draw_arc;
  dxf_hid.draw_rect = dxf_draw_rect;
  dxf_hid.fill_circle = dxf_fill_circle;
  dxf_hid.fill_polygon = dxf_fill_polygon;
  dxf_hid.fill_rect = dxf_fill_rect;
  dxf_hid.calibrate = dxf_calibrate;
  dxf_hid.set_crosshair = dxf_set_crosshair;
  dxf_hid.show_item = dxf_show_item;
  dxf_hid.beep = dxf_beep;
  dxf_hid.progress = dxf_progress;
  hid_register_hid (&dxf_hid);
}


/* EOF */
