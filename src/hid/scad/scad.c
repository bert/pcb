/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *
 *  OpenSCAD export HID
 *  This code is based on the GERBER and VRML export HID
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>

#include <time.h>

#include "config.h"
#include "global.h"
#include "data.h"
#include "misc.h"
#include "error.h"
#include "buffer.h"
#include "create.h"

#include "hid.h"
#include "hid_draw.h"
#include "../hidint.h"
#include "hid/common/hidnogui.h"
#include "hid/common/draw_helpers.h"
#include "hid/common/hidinit.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#include "scad.h"

/****************************************************************************************************
* VRML export filter parameters and options
****************************************************************************************************/

static const char *export_modes[] = {
  "None",
  "Boxes",
  "Simple",
  "Realistic",
  0
};

static const char *board_outlines[] = {
  "None",
  "Outline",
  "Board",
  0
};

static const char *copper_colors[] = {
  "Copper",
  "Gold",
  "HAL",
  0
};

static const char *mask_colors[] = {
  "None",
  "Green",
  "Blue",
  "Red",
  0
};

static const char *board_cuts[] = {
  "All",
  "Top",
  "Top_only",
  "Bottom",
  "Bottom_only"
};

static char *mask_color_table[] = {
  "", SCAD_MASK_COLOR_G, SCAD_MASK_COLOR_B, SCAD_MASK_COLOR_R
};

static char *finish_color_table[] = {
  SCAD_COPPER_COLOR, SCAD_COPPER_COLOR_GOLD, SCAD_COPPER_COLOR_TIN
};




static HID scad_hid;

static int silk_layer, drill_layer, outline_layer, mask_layer;
static int layer_open, fresh_layer;
static char layer_id[64];

static struct
{
  int draw;
  int exp;
  float z_offset;
  int solder;
  int component;
} group_data[MAX_LAYER];


#define HA_scadfile 		0
#define HA_outline_type 	1
#define HA_exp_component 	2
#define HA_exp_copper 		3
#define HA_exp_silk 		4
#define HA_exp_inner_layers 	5
#define HA_mask_color 		6
#define HA_copper_color		7
#define HA_minimal_drill        9
#define HA_board_cut            10

static HID_Attribute scad_options[] = {
/*
%start-doc options "Advanced OpenSCAD Export"
@ftable @code
@item --scad-file <string>
Name of the OpenSCAD model file.
@end ftable
%end-doc
*/
  {
   "scad_file", "SCAD file name", HID_String, 0, 0,
   {
    0, 0, 0, 0}, 0, 0},
/*
%start-doc options "Advanced OpenSCAD Export"
@ftable @code
@item --board_outline <string>
How the board outline is created. @samp{None} - no board outline (inner layers copper is visible), @samp{Outline} - board outline is constructed from @samp{outline} layer,
@samp{Board} - board outline ha rectangular shape, dimensions are taken from board properties. Default: @samp{Board}.
@end ftable
%end-doc
*/
  {
   "board_outline", "Board outline",
   HID_Enum,
   0, 0,
   {
    2, 0, 0, 0}, board_outlines, 0},
/*
%start-doc options "Advanced OpenSCAD Export"
@ftable @code
@item --components <string>
Populates board with components.  Values: @samp{None}, @samp{Boxes}, @samp{Simple}, @samp{Realistic}. Default: @samp{Realistic}.
@end ftable
%end-doc
*/
  {
   "components", "Export components",
   HID_Enum,
   0, 0,
   {
    3, 0, 0, 0}, export_modes, 0},
/*
%start-doc options "Advanced OpenSCAD Export"
@ftable @code
@item --copper <boolean>
Exports copper tracks, pads, pins and vias. Default: @samp{false}.
@end ftable
%end-doc
*/
  {
   "copper", "Export tracks and copper planes", HID_Boolean, 0, 0,
   {
    0, 0, 0, 0}, 0, 0},
/*
%start-doc options "Advanced OpenSCAD Export"
@ftable @code
@item --silk_layers <boolean>
Exports silk screen layers. Default: @samp{false}.
@end ftable
%end-doc
*/
  {
   "silk_layers", "Export silk layers", HID_Boolean, 0, 0,
   {
    0, 0, 0, 0}, 0, 0},
/*
%start-doc options "Advanced OpenSCAD Export"
@ftable @code
@item --inner_layers <boolean>
Exports inner layers (normally invisible, hidden inside board). Default: @samp{false}.
@end ftable
%end-doc
*/
  {
   "innner_layers", "Export inner layers", HID_Boolean, 0, 0,
   {
    0, 0, 0, 0}, 0, 0},
/*
%start-doc options "Advanced OpenSCAD Export"
@ftable @code
@item --solder_mask <string>
Type of solder mask. Available options: @samp{None}, @samp{Green}, @samp{Red} and @samp{Blue}. Default: @samp{None}.
@end ftable
%end-doc
*/
  {
   "solder_mask", "Solder mask color", HID_Enum, 0, 0,
   {
    0, 0, 0, 0}, mask_colors, 0},
  {
/*
%start-doc options "Advanced OpenSCAD Export"
@ftable @code
@item --copper_finish <string>
Type of copper finish. Values: @samp{Copper}, @samp{Gold} and @samp{HAL}. Default: @samp{Copper}.
@end ftable
%end-doc
*/
   "copper_finish", "Surface finish of copper",
   HID_Enum,
   0, 0,
   {
    0, 0, 0, 0}, copper_colors, 0},

  {
   " ", " ", HID_Label, 0, 0,
   {
    0, 0, 0, 0}, 0, 0},
/*
%start-doc options "Advanced OpenSCAD Export"
@ftable @code
@item --min_drill <measure>
Minimal drill to be exported. Default: @samp{1mm}.
@end ftable
%end-doc
*/
  {
   "min_drill", "Minimal drill to export", HID_Coord, MM_TO_COORD (0),
   MM_TO_COORD (10),
   {
    0, 0, 0, MM_TO_COORD (1)}, 0, 0},
/*
%start-doc options "Advanced OpenSCAD Export"
@ftable @code
@item --board cut <string>
Type of board cut. Values: @samp{All}, @samp{Top}, @samp{Top only}, @samp{Bottom}, @samp{Bottom only}. Default: @samp{None}.
@end ftable
%end-doc
*/
  {
   "board_cut", "Section of board to be exported (3D print support)",
   HID_Enum,
   0, 0,
   {
    0, 0, 0, 0}, board_cuts, 0},
};

#define NUM_OPTIONS (sizeof(scad_options)/sizeof(scad_options[0]))

static HID_Attr_Val scad_values[NUM_OPTIONS];

/****************************************************************************************************/

FILE *scad_output;

static const char *scad_filename = 0;

static int opt_exp_silk;
static int opt_exp_component;
static int opt_exp_inner_layers;
static int opt_exp_copper;
static int opt_outline_type;
static int opt_copper_color;
static int opt_mask_color;
static Coord opt_minimal_drill;
static int opt_board_cut;

static int lastseq = 0;
static int current_mask;

static float scaled_layer_thickness;

static int n_alloc_outline_segments, n_outline_segments;
static t_outline_segment *outline_segments;

static void scad_fill_polygon (hidGC gc, int n_coords, Coord * x, Coord * y);
static void scad_emit_polygon (hidGC gc, int n_coords, Coord * x, Coord * y,
			       float thickness);


/* scaling function - all output is in milimeters */

float
scad_scale_coord (float x)
{
  return x * METRIC_SCALE;
}



static void
scad_close_layer ()
{

  if (!outline_layer)
    {
      if (drill_layer)
	{
	  fprintf (scad_output, "];\n\n");
	}
      else
	{
	  fprintf (scad_output, "}\n}\n\n");
	}

      fprintf (scad_output, "\n// END_OF_LAYER %s\n\n", layer_id);
    }
  layer_open = 0;
}


/*******************************************
* Export filter implementation starts here
********************************************/

static HID_Attribute *
scad_get_export_options (int *n)
{
  static char *last_made_filename = 0;
  if (PCB)
    derive_default_filename (PCB->Filename, &scad_options[HA_scadfile],
			     ".scad", &last_made_filename);

//    scad_options[HA_minimal_drill].coord_value = scad_options[HA_minimal_drill].
  if (n)
    *n = NUM_OPTIONS;
  return scad_options;
}

/*******************************************
* main export function
* - collect information about used layers and their grouping
* - calculates positions olaers, dependiing on options and number of layer groups
* - calls the export, exports the board layout
* - generates the board outline, based on (depends on user selection):
*   - physical dimensions set in preferences
*   - lines drawn on "Outline" layer
*   - polygons drawn on "Shape" layer
* - populates the board with components
********************************************/

static void
init_outline ()
{
  outline_segments = 0;
  n_alloc_outline_segments = 0;
  n_outline_segments = 0;
}

static void
add_outline_segment (Coord x1, Coord y1, Coord x2, Coord y2)
{
  if (!n_alloc_outline_segments)
    {
      outline_segments =
	(t_outline_segment *) malloc (sizeof (t_outline_segment) * 50);
      n_alloc_outline_segments = 50;
      if (!outline_segments)
	{
	  Message
	    (" Error: Cannot allocate memory for board outline. Board outline cannot be created.\n");
	  return;
	}
    }
  else
    {
      if (n_alloc_outline_segments == n_outline_segments)
	{
	  t_outline_segment *os =
	    (t_outline_segment *) realloc (outline_segments,
					   sizeof (t_outline_segment) *
					   (n_alloc_outline_segments + 50));

	  if (os)
	    {
	      outline_segments = os;
	      n_alloc_outline_segments = n_alloc_outline_segments + 50;
	    }
	  else
	    {
	      Message
		(" Error: Cannot allocate more memory for board outline. Board outline will be incomplete.\n");
	      return;
	    }
	}
    }

  outline_segments[n_outline_segments].processed = 0;
  outline_segments[n_outline_segments].x1 = x1;
  outline_segments[n_outline_segments].y1 = y1;
  outline_segments[n_outline_segments].x2 = x2;
  outline_segments[n_outline_segments].y2 = y2;
  n_outline_segments++;
}


typedef struct
{
  Coord x;
  Coord y;
  int marker;
} t_OutlinePoint;

static int
is_same_point (Coord x1, Coord y1, Coord x2, Coord y2)
{
  return (abs (x1 - x2) < SCAD_MIN_OUTLINE_DIST)
    && (abs (y1 - y2) < SCAD_MIN_OUTLINE_DIST);
}


void
scad_process_outline ()
{
  int i, j, n;
  t_OutlinePoint *op;

  if (outline_segments && n_outline_segments)
    {

      op = malloc (n_outline_segments * 2 * sizeof (t_OutlinePoint));

      if (op != NULL)
	{

	  n = 0;
	  for (i = 0; i < n_outline_segments; i++)
	    {

	      if (!outline_segments[i].processed)
		{
		  outline_segments[i].processed = 1;
		  op[n].x = outline_segments[i].x1;
		  op[n].y = outline_segments[i].y1;
		  op[n].marker = 1;
		  op[n + 1].x = outline_segments[i].x2;
		  op[n + 1].y = outline_segments[i].y2;
		  op[n + 1].marker = 0;
		  n += 2;;
		  do
		    {
		      for (j = i + 1; j < n_outline_segments; j++)
			{
			  if (!outline_segments[j].processed)
			    {
			      if (is_same_point
				  (op[n - 1].x, op[n - 1].y,
				   outline_segments[j].x1,
				   outline_segments[j].y1))
				{
				  op[n - 1].x =
				    (op[n - 1].x +
				     outline_segments[j].x1) / 2;
				  op[n - 1].y =
				    (op[n - 1].y +
				     outline_segments[j].y1) / 2;
				  op[n].x = outline_segments[j].x2;
				  op[n].y = outline_segments[j].y2;
				  n++;
				  outline_segments[j].processed = 1;
				  break;
				}
			      else
				if (is_same_point
				    (op[n - 1].x, op[n - 1].y,
				     outline_segments[j].x2,
				     outline_segments[j].y2))
				{
				  op[n - 1].x =
				    (op[n - 1].x +
				     outline_segments[j].x2) / 2;
				  op[n - 1].y =
				    (op[n - 1].y +
				     outline_segments[j].y2) / 2;
				  op[n].x = outline_segments[j].x1;
				  op[n].y = outline_segments[j].y1;
				  n++;
				  outline_segments[j].processed = 1;
				  break;
				}
			    }
			}
		    }
		  while (j < n_outline_segments);
		}
	    }

	  fprintf (scad_output, "module board_outline () {\n\tpolygon([");

	  /* Outline points */
	  for (i = 0; i < n; i++)
	    {
	      fprintf (scad_output, "\t\t[%f, %f]%s\n",
		       scad_scale_coord ((float) op[i].x),
		       -scad_scale_coord ((float) op[i].y),
		       (i < (n - 1)) ? ", " : "");
	    }

	  /* Outline paths */
	  fprintf (scad_output, "\t],[\n\t\t");

	  fprintf (scad_output, "\t\t[");
	  for (i = 0; i < n; i++)
	    {
//            if (!(i % 10) && i)
//              fprintf (scad_output, "\n\t\t");
	      if (i > 0 && op[i].marker)
		fprintf (scad_output, "],\n\t\t[");
//            fprintf (scad_output, "%d%s", i, (i < (n - 1)) ? ", " : "");
	      fprintf (scad_output, "%d%s", i,
		       ((i < (n - 1)) && op[i + 1].marker == 0) ? ", " : "");
	    }
	  fprintf (scad_output, "\t]]);\n");
	  fprintf (scad_output, "}\n\n");
	}
      else
	{
	  Message
	    (" Error: Cannot allocate more memory for board outline. Board outline will be incomplete.\n");
	}
      if (op)
	free (op);
      free (outline_segments);
    }
}

static void
scad_do_export (HID_Attr_Val * options)
{
  int i;
  int inner_layers;
  float layer_spacing, layer_offset, cut_offset = 0.;
  BoxType region;
  FlagType save_thindraw;
  LayerType *layer;

  save_thindraw = PCB->Flags;
  CLEAR_FLAG (THINDRAWFLAG, PCB);
  CLEAR_FLAG (THINDRAWPOLYFLAG, PCB);

  if (!options)
    {
      scad_get_export_options (0);
      for (i = 0; i < NUM_OPTIONS; i++)
	scad_values[i] = scad_options[i].default_val;
      options = scad_values;
    }
  opt_mask_color = options[HA_mask_color].int_value;
  opt_exp_silk = options[HA_exp_silk].int_value;
  opt_exp_component = options[HA_exp_component].int_value;
  opt_exp_inner_layers = options[HA_exp_inner_layers].int_value;
  opt_exp_copper = options[HA_exp_copper].int_value;
  opt_outline_type = options[HA_outline_type].int_value;
  opt_copper_color = options[HA_copper_color].int_value;
  opt_minimal_drill = options[HA_minimal_drill].coord_value;
  opt_board_cut = options[HA_board_cut].int_value;

  scad_filename = options[HA_scadfile].str_value;
  if (!scad_filename)
    scad_filename = "unknown.scad";

  scad_output = fopen (scad_filename, "w");
  if (scad_output == NULL)
    {
      Message (" Error: Could not open %s for writing.\n", scad_filename);
      return;
    }

  scad_write_prologue (PCB->Filename);

  memset (group_data, 0, sizeof (group_data));
#ifdef SOLDER_LAYER
  group_data[GetLayerGroupNumberByNumber
	     (max_copper_layer + SOLDER_LAYER)].solder = 1;
  group_data[GetLayerGroupNumberByNumber
	     (max_copper_layer + COMPONENT_LAYER)].component = 1;
#else
  group_data[GetLayerGroupNumberByNumber
	     (max_copper_layer + BOTTOM_SIDE)].solder = 1;
  group_data[GetLayerGroupNumberByNumber
	     (max_copper_layer + TOP_SIDE)].component = 1;
#endif
  for (i = 0; i < max_copper_layer; i++)
    {
      layer = PCB->Data->Layer + i;
      if (layer->LineN || layer->TextN || layer->ArcN || layer->PolygonN)
	group_data[GetLayerGroupNumberByNumber (i)].draw = 1;
    }

  inner_layers = 0;
  for (i = 0; i < max_group; i++)
    {
      if (group_data[i].draw
	  && !(group_data[i].component || group_data[i].solder))
	{
	  inner_layers++;
	}
    }

  layer_spacing = BOARD_THICKNESS / ((float) inner_layers + 1);
  layer_offset = BOARD_THICKNESS / 2. - layer_spacing;
  for (i = 0; i < max_group; i++)
    {
      if (group_data[i].component)
	{
	  group_data[i].z_offset =
	    (BOARD_THICKNESS / 2.) + (OUTER_COPPER_THICKNESS / 2.);
	}
      else if (group_data[i].solder)
	{
	  group_data[i].z_offset =
	    -(BOARD_THICKNESS / 2.) - (OUTER_COPPER_THICKNESS / 2.);
	}
      else if (group_data[i].draw)
	{
	  group_data[i].z_offset = layer_offset;
	  layer_offset -= layer_spacing;
	}
    }

  region.X1 = 0;
  region.Y1 = 0;
  region.X2 = PCB->MaxWidth;
  region.Y2 = PCB->MaxHeight;

  layer_open = 0;

  hid_expose_callback (&scad_hid, &region, 0);

// And now .... Board outlines

  if (opt_outline_type == SCAD_OUTLINE_SIZE)
    {
      fprintf (scad_output, "module board_outline () {\n\tpolygon(");
      fprintf (scad_output, "[[0,0],[0,%f],[%f,%f],[%f,0]],\n",
	       -scad_scale_coord ((float) PCB->MaxHeight),
	       scad_scale_coord ((float) PCB->MaxWidth),
	       -scad_scale_coord ((float) PCB->MaxHeight),
	       scad_scale_coord ((float) PCB->MaxWidth));
      fprintf (scad_output, "[[0,1,2,3]]);\n");
      fprintf (scad_output, "}\n\n");

    }
  else if (opt_outline_type ==
	   SCAD_OUTLINE_OUTLINE /* and collected lines */ )
    {
      scad_process_outline ();
    }

  if (layer_open)
    {
      scad_close_layer ();
    }

  scad_generate_holes ();
  if (opt_exp_copper)
    scad_generate_plating ();

  if (opt_outline_type != SCAD_OUTLINE_NONE)
    scad_generate_board ();

  if (opt_mask_color != SCAD_MASK_NONE
      && opt_outline_type != SCAD_OUTLINE_NONE)
    scad_generate_mask ();


  if (EXPORT_COMPONENTS)
    {
      fprintf (scad_output,
	       "/***************************************************/\n");
      fprintf (scad_output,
	       "/*                                                 */\n");
      fprintf (scad_output,
	       "/* Components                                      */\n");
      fprintf (scad_output,
	       "/*                                                 */\n");
      fprintf (scad_output,
	       "/***************************************************/\n");
      scad_process_components (opt_exp_component);
    }


  fprintf (scad_output,
	   "/***************************************************/\n");
  fprintf (scad_output,
	   "/*                                                 */\n");
  fprintf (scad_output,
	   "/* Final board assembly                            */\n");
  fprintf (scad_output,
	   "/* Here is the complete board built from           */\n");
  fprintf (scad_output,
	   "/* pre-generated modules                           */\n");
  fprintf (scad_output,
	   "/*                                                 */\n");
  fprintf (scad_output,
	   "/***************************************************/\n");

  if (opt_board_cut != SCAD_CUT_COMPLETE)
    {
      fprintf (scad_output, "intersection () {\n\tunion () {\n");
    }

  if (opt_exp_copper)
    {
      for (i = 0; i < max_group; i++)
	{
	  if (group_data[i].exp)
	    {
//        printf("%d\n",i);

	      if (group_data[i].component || group_data[i].solder
		  || opt_exp_inner_layers)
		{
		  fprintf (scad_output, "\t\tcolor (%s)\n",
			   finish_color_table[opt_copper_color]);
		  fprintf (scad_output, "\t\t\tdifference() {\n");
		  fprintf (scad_output, "\t\t\t\tlayer_%02d_body(%f);\n", i,
			   group_data[i].z_offset);
		  fprintf (scad_output, "\t\t\t\tall_holes();\n");
		  fprintf (scad_output, "\t\t\t}\n\n");
		}

	    }
	}
    }

  if (opt_exp_silk)
    {
      fprintf (scad_output, "\t\tcolor (%s)\n", SCAD_SILK_COLOR);
      fprintf (scad_output, "\t\t\tlayer_topsilk_body(%f);\n\n",
	       (opt_mask_color != SCAD_MASK_NONE
		&& opt_outline_type !=
		SCAD_OUTLINE_NONE) ? SILK_LAYER_OFFSET2 : SILK_LAYER_OFFSET);
      fprintf (scad_output, "\t\tcolor (%s)\n", SCAD_SILK_COLOR);
      fprintf (scad_output, "\t\t\tlayer_bottomsilk_body(%f);\n\n",
	       (opt_mask_color != SCAD_MASK_NONE
		&& opt_outline_type !=
		SCAD_OUTLINE_NONE) ? -SILK_LAYER_OFFSET2 :
	       -SILK_LAYER_OFFSET);
    }

  if (opt_exp_copper)
    {
      fprintf (scad_output, "\t\tcolor (%s)\n",
	       finish_color_table[opt_copper_color]);
      fprintf (scad_output, "\t\t\tall_plating();\n\n");
    }

  if (opt_outline_type != SCAD_OUTLINE_NONE)
    {
      fprintf (scad_output, "\t\tcolor (%s)\n", SCAD_BOARD_COLOR);
      fprintf (scad_output, "\t\t\tdifference() {\n");
      fprintf (scad_output, "\t\t\t\tboard_body();\n");
      fprintf (scad_output, "\t\t\t\tall_holes();\n");
      fprintf (scad_output, "\t\t\t}\n\n");
    }
  if (opt_mask_color != SCAD_MASK_NONE
      && opt_outline_type != SCAD_OUTLINE_NONE)
    {
      fprintf (scad_output, "\t\tcolor (%s) translate ([0,0,%f])\n",
	       mask_color_table[opt_mask_color],
	       (BOARD_THICKNESS + MASK_THICKNESS) / 2.);
      fprintf (scad_output, "\t\t\tdifference() {\n");
      fprintf (scad_output, "\t\t\t\tmask_surface();\n");
      fprintf (scad_output, "\t\t\t\tlayer_topmask_body();\n");
      fprintf (scad_output, "\t\t\t}\n\n");
      fprintf (scad_output, "\t\tcolor (%s) translate ([0,0,%f])\n",
	       mask_color_table[opt_mask_color],
	       -(BOARD_THICKNESS + MASK_THICKNESS) / 2.);
      fprintf (scad_output, "\t\t\tdifference() {\n");
      fprintf (scad_output, "\t\t\t\tmask_surface();\n");
      fprintf (scad_output, "\t\t\t\tlayer_bottommask_body();\n");
      fprintf (scad_output, "\t\t\t}\n\n");
    }

  if (EXPORT_COMPONENTS)
    fprintf (scad_output, "\t\tall_components();\n");

  if (opt_board_cut != SCAD_CUT_COMPLETE)
    {
      switch (opt_board_cut)
	{
	case SCAD_CUT_TOP:
	  cut_offset = 0.;
	  break;
	case SCAD_CUT_TOP_ONLY:
	  cut_offset = -BOARD_THICKNESS / 2.;
	  break;
	case SCAD_CUT_BOTTOM:
	  cut_offset = -100.;
	  break;
	case SCAD_CUT_BOTTOM_ONLY:
	  cut_offset = -100. + BOARD_THICKNESS / 2.;
	  break;
	}
      fprintf (scad_output, "\t}\n");
      fprintf (scad_output, "\t\t translate ([%f,%f,%f]) ", -25.,
	       -scad_scale_coord ((float) PCB->MaxHeight) / 2. - 25.,
	       cut_offset);
      fprintf (scad_output, "\t\t cube ([%f,%f,100]);\n",
	       scad_scale_coord ((float) PCB->MaxWidth) + 50.,
	       scad_scale_coord ((float) PCB->MaxHeight));
      fprintf (scad_output, "}\n");
    }

  fprintf (scad_output, "// END_OF_BOARD\n");
  fclose (scad_output);
  PCB->Flags = save_thindraw;
}

static void
scad_parse_arguments (int *argc, char ***argv)
{
  hid_register_attributes (scad_options,
			   sizeof (scad_options) / sizeof (scad_options[0]));
  hid_parse_command_line (argc, argv);
}

static int
scad_set_layer (const char *name, int group, int empty)
{
  int idx = (group >= 0
	     && group <
	     max_group) ? PCB->LayerGroups.Entries[group][0] : group;
  int layer_ok;

  if (layer_open)
    {
      scad_close_layer ();
    }



  if (name == 0)
    name = PCB->Data->Layer[idx].Name;

//  printf("%s\n",name);

  silk_layer = 0;
  drill_layer = 0;
  mask_layer = 0;
  outline_layer = 0;
  fresh_layer = 1;

  if (strcmp (name, "invisible") == 0)
    return 0;

  if (SL_TYPE (idx) == SL_ASSY)
    return 0;

  if (strcmp (name, "route") == 0)
    return 0;

  if (group >= 0 && group < max_group)
    {
      layer_ok = (opt_exp_inner_layers || group_data[group].component
		  || group_data[group].solder) && opt_exp_copper;
    }
  else
    {
      layer_ok = 1;
    }

  if (strcmp (name, "outline") == 0)
    {
      if (opt_outline_type == SCAD_OUTLINE_OUTLINE)
	{
	  outline_layer = 1;
	  layer_ok = 1;
	  init_outline ();
	  n_alloc_outline_segments = 0;
	  n_outline_segments = 0;
	}
      else
	return 0;
    }

  if (!layer_ok)
    return 0;

  if (group >= 0 && group < max_group)
    {
      if (!group_data[group].draw)
	return 0;
      scaled_layer_thickness = (group_data[group].solder
				|| group_data[group].component) ?
	OUTER_COPPER_THICKNESS : INNER_COPPER_THICKNESS;
      sprintf (layer_id, "layer_%02d", group);
      if (!outline_layer)
	{
	  group_data[group].exp = 1;
	}
    }
  else
    {
      if (SL_TYPE (group) == SL_PDRILL)
	{
	  drill_layer = SL_TYPE (group);
	  strcpy (layer_id, "layer_pdrill");
	}
      else if (SL_TYPE (group) == SL_UDRILL)
	{
	  drill_layer = SL_TYPE (group);
	  strcpy (layer_id, "layer_udrill");
	}
      else if (SL_TYPE (group) == SL_SILK)
	{
	  if (!opt_exp_silk)
	    return 0;
	  scaled_layer_thickness = SILK_LAYER_THICKNESS;
	  silk_layer = SL_TYPE (group);
	  if (SL_SIDE (group) == SL_TOP_SIDE)
	    {
	      strcpy (layer_id, "layer_topsilk");
	    }
	  else
	    {
	      strcpy (layer_id, "layer_bottomsilk");
	    }
	}
      else if (SL_TYPE (group) == SL_MASK)
	{
	  if (opt_mask_color == SCAD_MASK_NONE
	      || opt_outline_type == SCAD_OUTLINE_NONE)
	    return 0;
	  scaled_layer_thickness = MASK_THICKNESS * 2.;
	  silk_layer = SL_TYPE (group);
	  if (SL_SIDE (group) == SL_TOP_SIDE)
	    {
	      strcpy (layer_id, "layer_topmask");
	    }
	  else
	    {
	      strcpy (layer_id, "layer_bottommask");
	    }
	}
      else
	{
	  return 0;
	}
    }

  layer_open = 1;

  if (!outline_layer)
    {
      fprintf (scad_output, "// START_OF_LAYER: %s\n", name);
      if (drill_layer)
	{
	  fprintf (scad_output, "%s_list=[\n", layer_id);
	}
      else
	{
	  fprintf (scad_output,
		   "module %s_body (offset) {\ntranslate ([0, 0, offset]) union () {\n",
		   layer_id);
	}
    }
  return 1;
}

static hidGC
scad_make_gc (void)
{
  hidGC rv = (hidGC) calloc (1, sizeof (hid_gc_struct));
  rv->cap = Trace_Cap;
  rv->seq = lastseq++;
  return rv;
}

static void
scad_destroy_gc (hidGC gc)
{
  free (gc);
}

static void
scad_use_mask (enum mask_mode use_it)
{
  current_mask = use_it;
}

static void
scad_set_color (hidGC gc, const char *name)
{

  if (strcmp (name, "erase") == 0)
    {
      gc->erase = 1;
      gc->drill = 0;
    }
  else if (strcmp (name, "drill") == 0)
    {
      gc->erase = 0;
      gc->drill = 1;
    }
  else
    {
      if (name[0] == '#')
	{
	  sscanf (name + 1, "%02x%02x%02x", &(gc->r), &(gc->g), &(gc->b));
	}
      else
	{
	  gc->r = 1;
	  gc->g = 1;
	  gc->b = 1;
	}
      gc->erase = 0;
      gc->drill = 0;
    }
}

static void
scad_set_line_cap (hidGC gc, EndCapStyle style)
{
  gc->cap = style;
}

static void
scad_set_line_width (hidGC gc, Coord width)
{
  gc->width = width;
}

static void
scad_set_draw_xor (hidGC gc, int xor)
{
}


static void
scad_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  Coord x[5];
  Coord y[5];
  x[0] = x[4] = x1;
  y[0] = y[4] = y1;
  x[1] = x1;
  y[1] = y2;
  x[2] = x2;
  y[2] = y2;
  x[3] = x2;
  y[3] = y1;
  scad_fill_polygon (gc, 5, x, y);
}

/* Helper function - draws the line or collects the line segments
*  - on outline layer the line segments are collected and later used to draw board outline
*  - otherwise it is drawn as rotated box with circle at the end(s), depending on mode:
*    -- on single line segment or last polyline segment caps are drawn on both ends
*    -- on polyline segment cap is drawn only on beginning
*/

static void
scad_emit_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2, int mode)
{
  int zero_length;
  float angle = 0., length = 0.;
  float cx, cy;
  int bd = 0, c1 = 0, c2 = 0;

  if (outline_layer)
    {
      add_outline_segment (x1, y1, x2, y2);
      return;
    }



  zero_length = (x1 == x2 && y1 == y2 && gc->cap != Square_Cap) ? 1 : 0;

  cx = ((float) (x1 + x2)) / 2.;
  cy = ((float) (y1 + y2)) / 2.;


  if (!zero_length)
    {
      angle = -(atan2 (-(float) (y2 - y1), -(float) (x2 - x1))) / M_PI * 180.;
      length =
	sqrt ((float) (x2 - x1) * (float) (x2 - x1) +
	      (float) (y2 - y1) * (float) (y2 - y1));
      if (gc->cap == Square_Cap)
	{
	  length += (float) gc->width;
	}
    }


  if (!zero_length)
    {
      bd = 1;
    }
  if (gc->cap == Trace_Cap || gc->cap == Round_Cap || mode == SCAD_EL_LASTPOLY
      || mode == SCAD_EL_POLY)
    {
      c2 = 1;
      if (!zero_length && (mode != SCAD_EL_POLY))
	{
	  c1 = 1;
	}
    }

  if (c1 || c2)
    {
      fprintf (scad_output, "\tline_segment_r(%f,%f,%f,%f,%f,%f,%d,%d,%d);\n",
	       scad_scale_coord (length),
	       scad_scale_coord ((float) gc->width), scaled_layer_thickness,
	       scad_scale_coord (cx), scad_scale_coord (-cy), angle, bd, c1,
	       c2);
    }
  else
    {
      if (bd)
	fprintf (scad_output, "\tline_segment(%f,%f,%f,%f,%f,%f);\n",
		 scad_scale_coord (length),
		 scad_scale_coord ((float) gc->width), scaled_layer_thickness,
		 scad_scale_coord (cx), scad_scale_coord (-cy), angle);
    }

}

static void
scad_draw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  if (drill_layer)
    return;

  scad_emit_line (gc, x1, y1, x2, y2, SCAD_EL_STANDARD);
}


static void
scad_draw_arc (hidGC gc, Coord cx, Coord cy, Coord width, Coord height,
	       Angle start_angle, Angle delta_angle)
{
  int i, n_steps, x, y, ox = 0, oy = 0, sa;
  float angle;

  if (drill_layer)
    return;

  n_steps = ((delta_angle < 0) ? (-delta_angle) : delta_angle + 4) / 5;
  sa = start_angle + 180;
  if (sa > 360)
    sa -= 360;

  for (i = 0; i <= n_steps; i++)
    {

      angle =
	((float) (sa) +
	 ((float) delta_angle * (float) i / (float) n_steps)) * M_PI / 180.;

      if (i)
	{
	  x = (int) ((float) width * cos (angle)) + cx;
	  y = -(int) ((float) height * sin (angle)) + cy;
	  scad_emit_line (gc, ox, oy, x, y,
			  (i == (n_steps)) ? SCAD_EL_LASTPOLY : SCAD_EL_POLY);

	  ox = x;
	  oy = y;
	}
      else
	{
	  ox = (int) ((float) width * cos (angle)) + cx;
	  oy = -(int) ((float) height * sin (angle)) + cy;
	}
    }
}


/* Emit the cylinder - its appearance depends on layer it is located on:
*    - as plated or unplated drills it creates vector of holes
*    - otherwise it is drawn as simple
*/
static void
scad_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius)
{
//  int i;
  if (outline_layer)
    return;

  if (drill_layer && !gc->drill)
    {
      return;
    }
  if (!drill_layer && gc->drill)
    {
      return;
    }

  if (radius <= 0)
    return;

  if (drill_layer && ((2 * radius) < opt_minimal_drill))
    return;

  if (drill_layer)
    {
      fprintf (scad_output, "\t[%f, [%f, %f]],\n",
	       scad_scale_coord ((float) radius), scad_scale_coord (cx),
	       -scad_scale_coord (cy));
    }
  else
    {
      fprintf (scad_output,
	       "\ttranslate ([%f, %f, 0]) cylinder (r=%f, h=%f, center=true, $fn=30);\n",
	       scad_scale_coord ((float) cx), -scad_scale_coord ((float) cy),
	       scad_scale_coord ((float) radius), scaled_layer_thickness);
    }
}

/*
* Helper function - creates extruded polygon
*/

static void
scad_emit_polygon (hidGC gc, int n_coords, Coord * x, Coord * y,
		   float thickness)
{
  int i, n;
//  int cw, cx;


  fprintf (scad_output,
	   "\ttranslate ([0, 0, %f]) linear_extrude(height=%f) polygon ([",
	   -thickness / 2., thickness);


/* Normalize polygon - remove last point, if it is equal to first point */

  n = n_coords;
  if (x[n_coords - 1] == x[0] && y[n_coords - 1] == y[0])
    n--;

/* Polygon points*/

  for (i = 0; i < n; i++)
    {
      fprintf (scad_output, "\t\t[%f, %f]%s\n",
	       scad_scale_coord ((float) x[i]),
	       -scad_scale_coord ((float) y[i]), (i < (n - 1)) ? ", " : "");
    }

  fprintf (scad_output, "\t],[[\n\t\t");

  for (i = 0; i < n; i++)
    {
      if (!(i % 10) && i)
	fprintf (scad_output, "\n\t\t");
      fprintf (scad_output, "%d%s", i, (i < (n - 1)) ? ", " : "");
    }
  fprintf (scad_output, "]]);\n");

}

static void
scad_fill_polygon (hidGC gc, int n_coords, Coord * x, Coord * y)
{
  if (outline_layer)
    return;

  scad_emit_polygon (gc, n_coords, x, y, scaled_layer_thickness);
}

static void
scad_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  Coord x[5];
  Coord y[5];

  if (scad_output)
    fprintf (scad_output, "// Fill rect\n");

  x[0] = x[4] = x1;
  y[0] = y[4] = y1;
  x[1] = x1;
  y[1] = y2;
  x[2] = x2;
  y[2] = y2;
  x[3] = x2;
  y[3] = y1;
  scad_fill_polygon (gc, 5, x, y);
}

static void
scad_calibrate (double xval, double yval)
{
  if (scad_output)
    fprintf (scad_output, "// Calibrate\n");
  CRASH;
}

static void
scad_set_crosshair (int x, int y, int action)
{
  if (scad_output)
    fprintf (scad_output, "// Set CrossHair\n");
}

static HID scad_hid;
static HID_DRAW scad_graphics;

void
hid_scad_init ()
{
  memset (&scad_hid, 0, sizeof (scad_hid));
  memset (&scad_graphics, 0, sizeof (scad_graphics));

  common_nogui_init (&scad_hid);
  common_draw_helpers_init (&scad_graphics);


  scad_hid.struct_size = sizeof (scad_hid);
  scad_hid.name = "scad";
  scad_hid.description = "OpenSCAD script export";
  scad_hid.exporter = 1;

  scad_hid.graphics = &scad_graphics;

  scad_hid.get_export_options = scad_get_export_options;
  scad_hid.do_export = scad_do_export;
  scad_hid.parse_arguments = scad_parse_arguments;
  scad_hid.set_layer = scad_set_layer;
  scad_hid.calibrate = scad_calibrate;
  scad_hid.set_crosshair = scad_set_crosshair;

  scad_graphics.make_gc = scad_make_gc;
  scad_graphics.destroy_gc = scad_destroy_gc;
  scad_graphics.use_mask = scad_use_mask;
  scad_graphics.set_color = scad_set_color;
  scad_graphics.set_line_cap = scad_set_line_cap;
  scad_graphics.set_line_width = scad_set_line_width;
  scad_graphics.set_draw_xor = scad_set_draw_xor;

  scad_graphics.draw_line = scad_draw_line;
  scad_graphics.draw_arc = scad_draw_arc;
  scad_graphics.draw_rect = scad_draw_rect;
  scad_graphics.fill_circle = scad_fill_circle;
  scad_graphics.fill_polygon = scad_fill_polygon;
  scad_graphics.fill_rect = scad_fill_rect;

  hid_register_hid (&scad_hid);
}
