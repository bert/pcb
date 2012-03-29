/* $Id$ */

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
#include "../hidint.h"
#include "hid/common/hidnogui.h"
#include "hid/common/draw_helpers.h"
#include "hid/common/hidinit.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#include "vrml.h"


static const char *outlines[] = {
  "No outline",
  "Outline Layer",
  "Shape Layer",
  "Board size",
  0
};


static const char *colors[] = {
  "Copper",
  "Gold plated",
  "HAL",
  0
};

FILE *vrml_output;

static HID vrml_hid;

static int color_silk_emited, color_hole_emited, color_copper_emited,
  color_mask_emited;
static int color_emited[MAX_LAYER + 2];
static int silk_layer, drill_layer, outline_layer, shape_layer;
static int layer_open;

static struct
{
  int draw;
  int z_offset;
  int solder;
  int component;
} group_data[MAX_LAYER];


static struct color_table_struct color_table[MAX_LAYER_COLORS];
static int color_table_size;

static const char *vrml_filename = 0;
static int opt_exp_mask;
static int opt_exp_silk;
static int opt_exp_component;
static int opt_exp_inner_layers;
static int opt_use_layer_colors;
static int opt_outline_type;
static int opt_copper_color;
static int opt_compact;

static int lastseq = 0;
static int current_mask;
static int current_offset;
static int current_layer_thickness;

static int n_alloc_outline_segments, n_outline_segments;
static t_outline_segment *outline_segments;



static void vrml_fill_polygon (hidGC gc, int n_coords, Coord * x, Coord * y);
static void vrml_emit_polygon (hidGC gc, int n_coords, Coord * x, Coord * y,
			       int thickness, int offset, int color_index);


static int
check_poly_orientation (int n_coords, Coord * x, Coord * y)
{
  int i, j, n;
  int64_t z;

  n = n_coords;
  if (x[n_coords - 1] == x[0] && y[n_coords - 1] == y[0])
    n--;

  if (n < 3)
    return POLY_CCW;

  z = 0;
  for (i = 0; i < n; i++)
    {
      j = (i + 1) % n;
      z += (int64_t) x[i] * (int64_t) y[j] - (int64_t) x[j] * (int64_t) y[i];
    }
  if (z > 0)
    return POLY_CCW;
  else
    return POLY_CW;
}

static int
check_poly_convex (int n_coords, Coord * x, Coord * y)
{
  int i, j, k, f, n;
  int64_t z;

  n = n_coords;
  if (x[n_coords - 1] == x[0] && y[n_coords - 1] == y[0])
    n--;

  if (n < 3)
    return POLY_CONVEX;

  z = 0;
  f = 0;
  for (i = 0; i < n; i++)
    {
      j = (i + 1) % n;
      k = (i + 2) % n;
      z =
	((int64_t) x[j] - (int64_t) x[i]) * ((int64_t) y[k] -
					     (int64_t) y[j]) -
	((int64_t) y[j] - (int64_t) y[i]) * ((int64_t) x[k] - (int64_t) x[j]);
      if (z < 0)
	f |= 1;
      else if (z > 0)
	f |= 2;
      if (f == 3)
	return POLY_CONCAVE;
    }
  return POLY_CONVEX;
}

static void
vrml_emit_color (FILE * f, int color_index)
{
  if (color_index == VRML_HOLE_COLOR_INDEX)
    {
      if (color_hole_emited)
	{
	  fprintf (vrml_output, "appearance USE HoleColor\n");
	}
      else
	{
	  fprintf (vrml_output, "appearance DEF HoleColor Appearance {\n");
	  fprintf (vrml_output, "material Material {\n");
	  if (opt_outline_type != VRML_OUTLINE_NONE)
	    {
	      fprintf (vrml_output, "diffuseColor %s\n",
		       VRML_BOARD_HOLE_COLOR);
	    }
	  else
	    {
	      fprintf (vrml_output, "diffuseColor %s\n", VRML_HOLE_COLOR);
	    }
	  fprintf (vrml_output, "}}\n");
	  color_hole_emited = 1;
	}
    }
  else if (color_index == VRML_SILK_COLOR_INDEX)
    {
      if (color_silk_emited)
	{
	  fprintf (vrml_output, "appearance USE SilkColor\n");
	}
      else
	{
	  fprintf (vrml_output, "appearance DEF SilkColor Appearance {\n");
	  fprintf (vrml_output, "material Material {\n");
	  fprintf (vrml_output, "diffuseColor %s\n", VRML_SILK_COLOR);
	  fprintf (vrml_output, "}}\n");
	  color_silk_emited = 1;
	}
    }
  else if (color_index == VRML_BOARD_COLOR_INDEX)
    {
      fprintf (vrml_output, "appearance Appearance {\n");
      fprintf (vrml_output, "material Material {\n");
      fprintf (vrml_output, "diffuseColor %s\n",
	       (opt_exp_mask
		&& opt_outline_type ==
		VRML_OUTLINE_SHAPE) ? VRML_BOARD_COLOR_MASKED :
	       VRML_BOARD_COLOR);
      fprintf (vrml_output, "}}\n");
    }
  else if (color_index == VRML_MASK_COLOR_INDEX)
    {
      if (color_mask_emited)
	{
	  fprintf (vrml_output, "appearance USE MaskColor\n");
	}
      else
	{
	  fprintf (vrml_output, "appearance DEF MaskColor Appearance {\n");
	  fprintf (vrml_output, "material Material {\n");
	  fprintf (vrml_output, "diffuseColor %s\n", VRML_MASK_COLOR);
	  fprintf (vrml_output, "}}\n");
	  color_mask_emited = 1;
	}
    }
  else if (color_index == VRML_COPPER_COLOR_INDEX)
    {
      if (color_copper_emited)
	{
	  fprintf (vrml_output, "appearance USE CopperColor\n");
	}
      else
	{
	  fprintf (vrml_output, "appearance DEF CopperColor Appearance {\n");
	  fprintf (vrml_output, "material Material {\n");
	  if (opt_copper_color == VRML_COPPER_GOLD)
	    fprintf (vrml_output, "diffuseColor %s\n", (opt_exp_mask
							&& opt_outline_type ==
							VRML_OUTLINE_SHAPE) ?
		     VRML_COPPER_COLOR_GOLD_MASKED : VRML_COPPER_COLOR_GOLD);
	  else if (opt_copper_color == VRML_COPPER_TIN)
	    fprintf (vrml_output, "diffuseColor %s\n", (opt_exp_mask
							&& opt_outline_type ==
							VRML_OUTLINE_SHAPE) ?
		     VRML_COPPER_COLOR_TIN_MASKED : VRML_COPPER_COLOR_TIN);
	  else
	    fprintf (vrml_output, "diffuseColor %s\n", (opt_exp_mask
							&& opt_outline_type ==
							VRML_OUTLINE_SHAPE) ?
		     VRML_COPPER_COLOR_MASKED : VRML_COPPER_COLOR);
	  fprintf (vrml_output, "}}\n");
	  color_copper_emited = 1;
	}
    }
}

static void
vrml_emit_layer_color (FILE * f, hidGC gc)
{
  int i;

  if (opt_use_layer_colors)
    {
      for (i = 0; i < color_table_size; i++)
	{
	  if (color_table[i].r == gc->r && color_table[i].g == gc->g
	      && color_table[i].b == gc->b)
	    {
	      fprintf (vrml_output, "appearance USE LayerColor%03d\n", i);
	      return;
	    }
	}
      if (color_table_size < MAX_LAYER_COLORS)
	{
	  fprintf (vrml_output,
		   "appearance DEF LayerColor%03d Appearance {\n ",
		   color_table_size);
	  fprintf (vrml_output, "material Material {\n ");
	  fprintf (vrml_output, "diffuseColor %f %f %f\n ",
		   (float) gc->r / 255., (float) gc->g / 255.,
		   (float) gc->b / 255.);
	  color_table[color_table_size].r = gc->r;
	  color_table[color_table_size].g = gc->g;
	  color_table[color_table_size].b = gc->b;
	  color_table_size++;

	}
      else
	{
	  fprintf (vrml_output, "appearance Appearance {\n ");
	  fprintf (vrml_output, "material Material {\n ");
	  fprintf (vrml_output, "diffuseColor %f %f %f\n ",
		   (float) gc->r / 255., (float) gc->g / 255.,
		   (float) gc->b / 255.);
	}
      fprintf (vrml_output, "}}\n");
    }
  else
    {
      if (silk_layer)
	{
	  vrml_emit_color (vrml_output, VRML_SILK_COLOR_INDEX);
	}
      else
	{
	  vrml_emit_color (vrml_output, VRML_COPPER_COLOR_INDEX);
	}
    }
}


#define HA_vrmlfile 		0
#define HA_exp_silk 		1
#define HA_exp_component 	2
#define HA_exp_inner_layers 	3
#define HA_use_layer_colors 	4
#define HA_outline_type 	5
#define HA_exp_mask 		6
#define HA_copper_color		7
//#define HA_compact            8

static HID_Attribute vrml_options[] = {

  {
   "vrml_file", "VRML output file name", HID_String, 0, 0,
   {
    0, 0, 0}, 0, 0},
  {
   "silk_layers", "Export silk layers", HID_Boolean, 0, 0,
   {
    1, 0, 0}, 0, 0},
  {
   "components", "Export components", HID_Boolean, 0, 0,
   {
    1, 0, 0}, 0, 0},
  {
   "innner_layers", "Export inner layers", HID_Boolean, 0, 0,
   {
    0, 0, 0}, 0, 0},
  {
   "layer_colors", "Use layer colors for copper tracks",
   HID_Boolean,
   0, 0,
   {
    0, 0, 0}, 0, 0},
  {
   "board_outline", "Board outline",
   HID_Enum,
   0, 0,
   {
    3, 0, 0}, outlines, 0},
  {
   "solder_mask", "Export solder mask", HID_Boolean, 0, 0,
   {
    1, 0, 0}, 0, 0},
  {
   "copper_finish", "Surface finish of copper",
   HID_Enum,
   0, 0,
   {
    0, 0, 0}, colors, 0},
//  {
//   "compact_output", "All leading spaces and tabs are omitted",
//   HID_Boolean,
//   0, 0,
//   {
//    1, 0, 0}, 0, 0},
};

#define NUM_OPTIONS (sizeof(vrml_options)/sizeof(vrml_options[0]))

static HID_Attr_Val vrml_values[NUM_OPTIONS];

static HID_Attribute *
vrml_get_export_options (int *n)
{
  static char *last_made_filename = 0;
  if (PCB)
    derive_default_filename (PCB->Filename, &vrml_options[HA_vrmlfile],
			     ".wrl", &last_made_filename);

  if (n)
    *n = NUM_OPTIONS;
  return vrml_options;
}

static void
vrml_do_export (HID_Attr_Val * options)
{
  int i;
  int inner_layers, layer_spacing, current_offset;
  BoxType region;
  FlagType save_thindraw;
  LayerType *layer;
  Cardinal n_max;
  Coord *x, *y;

  save_thindraw = PCB->Flags;
  CLEAR_FLAG (THINDRAWFLAG, PCB);
  CLEAR_FLAG (THINDRAWPOLYFLAG, PCB);

  if (!options)
    {
      vrml_get_export_options (0);
      for (i = 0; i < NUM_OPTIONS; i++)
	vrml_values[i] = vrml_options[i].default_val;
      options = vrml_values;
    }
  opt_exp_mask = options[HA_exp_mask].int_value;
  opt_exp_silk = options[HA_exp_silk].int_value;
  opt_exp_component = options[HA_exp_component].int_value;
  opt_exp_inner_layers = options[HA_exp_inner_layers].int_value;
  opt_use_layer_colors = options[HA_use_layer_colors].int_value;
  opt_outline_type = options[HA_outline_type].int_value;
  opt_copper_color = options[HA_copper_color].int_value;
//  opt_compact = options[HA_compact].int_value;
  opt_compact = 1;

  vrml_filename = options[HA_vrmlfile].str_value;
  if (!vrml_filename)
    vrml_filename = "unknown.wrl";

  vrml_output = fopen (vrml_filename, "w");
  if (vrml_output == NULL)
    {
      Message (" Error: Could not open %s for writing.\n", vrml_filename);
      return;
    }
  memset (color_emited, 0, sizeof (color_emited));
  shape_layer = -1;
  color_silk_emited = 0;
  color_hole_emited = 0;
  color_copper_emited = 0;
  color_mask_emited = 0;
  color_table_size = 0;


  fprintf (vrml_output, "#VRML V2.0 utf8\n\n");
  fprintf (vrml_output, "PointLight {\n");
  fprintf (vrml_output, "on                TRUE\n");
  fprintf (vrml_output, "intensity         1\n");
  fprintf (vrml_output, "ambientIntensity  1\n");
  fprintf (vrml_output, "color             1 1 1\n");
  fprintf (vrml_output, "}\n\n");
  fprintf (vrml_output, "Viewpoint {\n");
  fprintf (vrml_output, "description \"Default\"\n");
  fprintf (vrml_output, "position %f %f %f\n",
	   (float) PCB->MaxWidth / 2. * FINAL_SCALE,
	   (float) PCB->MaxHeight * FINAL_SCALE,
	   (float) PCB->MaxHeight * 1.5 * FINAL_SCALE);
  fprintf (vrml_output, "orientation 1 0 0 -0.7854\n");
  fprintf (vrml_output, "}\n\n");
  fprintf (vrml_output, "Transform {\n");
  fprintf (vrml_output, "scale %f %f %f\n", FINAL_SCALE,
	   FINAL_SCALE, FINAL_SCALE);
  fprintf (vrml_output, "rotation 1 0 0 -1.5708\n");
  fprintf (vrml_output, "children [\n");
  memset (group_data, 0, sizeof (group_data));
  group_data[GetLayerGroupNumberByNumber
	     (max_copper_layer + SOLDER_LAYER)].solder = 1;
  group_data[GetLayerGroupNumberByNumber
	     (max_copper_layer + COMPONENT_LAYER)].component = 1;
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

  layer_spacing = BOARD_THICKNESS / (inner_layers + 1);
  current_offset = BOARD_THICKNESS / 2 - layer_spacing;
  for (i = 0; i < max_group; i++)
    {
      if (group_data[i].component)
	{
	  group_data[i].z_offset =
	    (BOARD_THICKNESS / 2) + (OUTER_COPPER_THICKNESS / 2);
	}
      else if (group_data[i].solder)
	{
	  group_data[i].z_offset =
	    -(BOARD_THICKNESS / 2) - (OUTER_COPPER_THICKNESS / 2);
	}
      else if (group_data[i].draw)
	{
	  group_data[i].z_offset = current_offset;
	  current_offset -= layer_spacing;
	}
    }

  region.X1 = 0;
  region.Y1 = 0;
  region.X2 = PCB->MaxWidth;
  region.Y2 = PCB->MaxHeight;

  layer_open = 0;

  hid_expose_callback (&vrml_hid, &region, 0);

// And now .... Board outlines

  if (opt_outline_type == VRML_OUTLINE_SIZE)
    {
      fprintf (vrml_output, "Transform {\n");
      fprintf (vrml_output, "translation %d %d 0\n",
	       (int) (PCB->MaxWidth / 2), -(int) (PCB->MaxHeight / 2));
      fprintf (vrml_output, "children [\n");
      fprintf (vrml_output, "Shape {\n");
      vrml_emit_color (vrml_output, VRML_BOARD_COLOR_INDEX);
      fprintf (vrml_output, "geometry Box {\n");
      fprintf (vrml_output, "size %d %d %d\n", (int) PCB->MaxWidth,
	       (int) PCB->MaxHeight, BOARD_THICKNESS);
      fprintf (vrml_output, "}}]}\n");
      if (opt_exp_mask)
	{
	  fprintf (vrml_output, "Transform {\n");
	  fprintf (vrml_output, "translation %d %d %d\n",
		   (int) (PCB->MaxWidth / 2), -(int) (PCB->MaxHeight / 2),
		   (BOARD_THICKNESS + MASK_THICKNESS) / 2);
	  fprintf (vrml_output, "children [\n");
	  fprintf (vrml_output, "Shape {\n");
	  vrml_emit_color (vrml_output, VRML_MASK_COLOR_INDEX);
	  fprintf (vrml_output, "geometry Box {\n");
	  fprintf (vrml_output, "size %d %d %d\n", (int) PCB->MaxWidth,
		   (int) PCB->MaxHeight, MASK_THICKNESS);
	  fprintf (vrml_output, "}}]}\n");
	  fprintf (vrml_output, "Transform {\n");
	  fprintf (vrml_output, "translation %d %d %d\n",
		   (int) (PCB->MaxWidth / 2), -(int) (PCB->MaxHeight / 2),
		   -(BOARD_THICKNESS + MASK_THICKNESS) / 2);
	  fprintf (vrml_output, "children [\n");
	  fprintf (vrml_output, "Shape {\n");
	  vrml_emit_color (vrml_output, VRML_MASK_COLOR_INDEX);
	  fprintf (vrml_output, "geometry Box {\n");
	  fprintf (vrml_output, "size %d %d %d\n", (int) PCB->MaxWidth,
		   (int) PCB->MaxHeight, MASK_THICKNESS);
	  fprintf (vrml_output, "}}]}\n");
	}
    }
  else if (opt_outline_type == VRML_OUTLINE_SHAPE && shape_layer >= 0)
    {

      n_max = 0;
      layer = PCB->Data->Layer + shape_layer;
      POLYGON_LOOP (layer);
      if (polygon->PointN > n_max)
	n_max = polygon->PointN;
      END_LOOP;

      x = malloc (n_max * sizeof (Coord));
      y = malloc (n_max * sizeof (Coord));

      if (x != NULL && y != NULL)
	{
	  POLYGON_LOOP (layer);
	  for (i = 0; i < polygon->PointN; i++)
	    {
	      x[i] = polygon->Points[i].X;
	      y[i] = polygon->Points[i].Y;
	    }
	  vrml_emit_polygon (NULL, polygon->PointN, x, y, BOARD_THICKNESS, 0,
			     VRML_BOARD_COLOR_INDEX);
	  END_LOOP;
	}
      else
	{
	  Message
	    (" Error: Cannot allocate more memory for board outline. Board outline will be incomplete.\n");
	}
      if (x)
	free (x);
      if (y)
	free (y);
    }
  else if (opt_outline_type ==
	   VRML_OUTLINE_OUTLINE /* and collected lines */ )
    {
      if (outline_segments && n_outline_segments)
	{
	  n_max = 0;

	  x = malloc (n_outline_segments * 2 * sizeof (int));
	  y = malloc (n_outline_segments * 2 * sizeof (int));
	  if (x != NULL && y != NULL)
	    {

	      int64_t mdiff, tdiff;
	      int xdiff, wdiff, n_processed;

	      x[0] = outline_segments[0].x1;
	      y[0] = outline_segments[0].y1;
	      x[1] = outline_segments[0].x2;
	      y[1] = outline_segments[0].y2;
	      n_max = 2;
	      n_processed = 1;

	      while (n_processed < n_outline_segments)
		{
		  mdiff = (-1);
		  xdiff = 0;
		  wdiff = 1;
		  for (i = 1; i < n_outline_segments; i++)
		    {
		      if (!outline_segments[i].processed)
			{
			  tdiff =
			    (int64_t) (x[n_max - 1] -
				       outline_segments[i].x1) *
			    (int64_t) (x[n_max - 1] -
				       outline_segments[i].x1) +
			    (int64_t) (y[n_max - 1] -
				       outline_segments[i].y1) *
			    (int64_t) (y[n_max - 1] - outline_segments[i].y1);
			  if (mdiff < 0)
			    {
			      mdiff = tdiff;
			      wdiff = 1;
			      xdiff = i;
			    }
			  else
			    {
			      if (tdiff < mdiff)
				{
				  mdiff = tdiff;
				  wdiff = 1;
				  xdiff = i;
				}
			    }
			  tdiff =
			    (int64_t) (x[n_max - 1] -
				       outline_segments[i].x2) *
			    (int64_t) (x[n_max - 1] -
				       outline_segments[i].x2) +
			    (int64_t) (y[n_max - 1] -
				       outline_segments[i].y2) *
			    (int64_t) (y[n_max - 1] - outline_segments[i].y2);
			  if (tdiff < mdiff)
			    {
			      mdiff = tdiff;
			      wdiff = 2;
			      xdiff = i;
			    }

			}
		    }
		  if (wdiff == 1)
		    {
		      if (x[n_max - 1] != outline_segments[xdiff].x1
			  && y[n_max - 1] != outline_segments[xdiff].y1)
			{
			  x[n_max] = outline_segments[xdiff].x1;
			  y[n_max] = outline_segments[xdiff].y1;
			  n_max++;
			}
		      x[n_max] = outline_segments[xdiff].x2;
		      y[n_max] = outline_segments[xdiff].y2;
		      n_max++;
		    }
		  else
		    {
		      if (x[n_max - 1] != outline_segments[xdiff].x2
			  && y[n_max - 1] != outline_segments[xdiff].y2)
			{
			  x[n_max] = outline_segments[xdiff].x2;
			  y[n_max] = outline_segments[xdiff].y2;
			  n_max++;
			}
		      x[n_max] = outline_segments[xdiff].x1;
		      y[n_max] = outline_segments[xdiff].y1;
		      n_max++;
		    }
		  outline_segments[xdiff].processed = 1;
		  n_processed++;
		}

	      vrml_emit_polygon (NULL, n_max, x, y, BOARD_THICKNESS, 0,
				 VRML_BOARD_COLOR_INDEX);
	      if (opt_exp_mask)
		{
		  vrml_emit_polygon (NULL, n_max, x, y, MASK_THICKNESS,
				     (BOARD_THICKNESS + MASK_THICKNESS) / 2,
				     VRML_MASK_COLOR_INDEX);
		  vrml_emit_polygon (NULL, n_max, x, y, MASK_THICKNESS,
				     -(BOARD_THICKNESS + MASK_THICKNESS) / 2,
				     VRML_MASK_COLOR_INDEX);
		}
	    }
	  else
	    {
	      Message
		(" Error: Cannot allocate more memory for board outline. Board outline will be incomplete.\n");
	    }
	  if (x)
	    free (x);
	  if (y)
	    free (y);
	  free (outline_segments);
	}
    }

  if (layer_open)
    {
      fprintf (vrml_output, "]}\n");
    }
  if (opt_exp_component)
    {
      vrml_process_components ();
    }
  fprintf (vrml_output, "]}\n");
  fclose (vrml_output);
  PCB->Flags = save_thindraw;
}

static void
vrml_parse_arguments (int *argc, char ***argv)
{
  hid_register_attributes (vrml_options,
			   sizeof (vrml_options) / sizeof (vrml_options[0]));
  hid_parse_command_line (argc, argv);
}

static int
vrml_set_layer (const char *name, int group, int empty)
{
  int offset = 0;
  int idx = (group >= 0
	     && group <
	     max_group) ? PCB->LayerGroups.Entries[group][0] : group;
  int inner_layer_ok;

  if (layer_open)
    {
      fprintf (vrml_output, "]}\n");
      layer_open = 0;
    }



  if (name == 0)
    name = PCB->Data->Layer[idx].Name;

  silk_layer = 0;
  drill_layer = 0;
  outline_layer = 0;

  if (strcmp (name, "invisible") == 0)
    return 0;

  if (SL_TYPE (idx) == SL_ASSY)
    return 0;

  if (strcmp (name, "route") == 0)
    return 0;

  if (group >= 0 && group < max_group)
    {
      inner_layer_ok = (opt_exp_inner_layers || group_data[group].component
			|| group_data[group].solder);
    }
  else
    {
      inner_layer_ok = 1;
    }

  if (strcmp (name, "outline") == 0)
    {
      if (opt_outline_type == VRML_OUTLINE_OUTLINE)
	{
	  outline_layer = 1;
	  inner_layer_ok = 1;
	  n_alloc_outline_segments = 0;
	  n_outline_segments = 0;
	}
      else
	return 0;
    }

  if (strcmp (name, "shape") == 0)
    {
      if (opt_outline_type == VRML_OUTLINE_SHAPE)
	{
	  inner_layer_ok = 1;
	  shape_layer = idx;
	}
      return 0;
    }

  if (!inner_layer_ok)
    return 0;

  if (group >= 0 && group < max_group)
    {
      if (!group_data[group].draw)
	return 0;
      current_offset = group_data[group].z_offset;
      current_layer_thickness = (group_data[group].solder
				 || group_data[group].component) ?
	OUTER_COPPER_THICKNESS : INNER_COPPER_THICKNESS;
    }
  else
    {
      if (SL_TYPE (group) == SL_PDRILL || SL_TYPE (group) == SL_UDRILL)
	{
	  drill_layer = SL_TYPE (group);
	}
      else if (SL_TYPE (group) == SL_SILK)
	{
	  if (!opt_exp_silk)
	    return 0;
	  current_layer_thickness = SILK_LAYER_THICKNESS;
	  silk_layer = SL_TYPE (group);
	  if (SL_SIDE (group) == SL_TOP_SIDE)
	    {
	      current_offset = (opt_exp_mask
				&& opt_outline_type !=
				VRML_OUTLINE_SHAPE) ? SILK_LAYER_OFFSET2 :
		SILK_LAYER_OFFSET;
	    }
	  else
	    {
	      current_offset = (opt_exp_mask
				&& opt_outline_type !=
				VRML_OUTLINE_SHAPE) ? -SILK_LAYER_OFFSET2 :
		-SILK_LAYER_OFFSET;
	    }
	}
      else
	{
	  return 0;
	}
    }

  layer_open = 1;

  if (drill_layer)
    {
      fprintf (vrml_output, "Group {       # %s\n", name);
    }
  else
    {
      fprintf (vrml_output, "Transform {      # %s\n", name);
      fprintf (vrml_output, "translation 0 0 %d\n", offset);

    }
  fprintf (vrml_output, "children [\n");


  return 1;
}

static hidGC
vrml_make_gc (void)
{
  hidGC rv = (hidGC) calloc (1, sizeof (hid_gc_struct));
  rv->cap = Trace_Cap;
  rv->seq = lastseq++;
  return rv;
}

static void
vrml_destroy_gc (hidGC gc)
{
  free (gc);
}

static void
vrml_use_mask (int use_it)
{
  current_mask = use_it;
}

static void
vrml_set_color (hidGC gc, const char *name)
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
	  gc->r = VRML_FALLBACK_COLOR_R;
	  gc->g = VRML_FALLBACK_COLOR_G;
	  gc->b = VRML_FALLBACK_COLOR_B;
	}
      gc->erase = 0;
      gc->drill = 0;
    }
}

static void
vrml_set_line_cap (hidGC gc, EndCapStyle style)
{
  gc->cap = style;
}

static void
vrml_set_line_width (hidGC gc, Coord width)
{
  gc->width = width;
}

static void
vrml_set_draw_xor (hidGC gc, int xor)
{
}

//static void
//vrml_set_draw_faded (hidGC gc, int faded)
//{
//}

//static void
//vrml_set_line_cap_angle (hidGC gc, int x1, int y1, int x2, int y2)
//{
//  CRASH;
//}

static void
vrml_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
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
  vrml_fill_polygon (gc, 5, x, y);
}

#define VRML_EL_STANDARD	0
#define VRML_EL_POLY		1
#define VRML_EL_LASTPOLY	2

static void
vrml_emit_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2, Coord mode)
{
  int zero_length;
  float angle = 0., length = 0.;
  float cx, cy;

  if (outline_layer)
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
					       (n_alloc_outline_segments +
						50));

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
      return;
    }



  zero_length = (x1 == x2 && y1 == y2 && gc->cap != Square_Cap) ? 1 : 0;
  // pozor na CAP

  cx = ((float) (x1 + x2)) / 2.;
  cy = ((float) (y1 + y2)) / 2.;


  if (!zero_length)
    {
      angle = -atan2 (-(float) (y2 - y1), -(float) (x2 - x1));
      length =
	sqrt ((float) (x2 - x1) * (float) (x2 - x1) +
	      (float) (y2 - y1) * (float) (y2 - y1));
      if (gc->cap == Square_Cap)
	{
	  length += (float) gc->width;
	}
    }

  fprintf (vrml_output, "Transform {\n");
  if (!zero_length)
    fprintf (vrml_output, "rotation 0 0 1 %f\n", angle);
  fprintf (vrml_output, "translation %f %f %d\n", cx, -cy, current_offset);
  fprintf (vrml_output, "children [\n");
  if (!zero_length)
    {
      fprintf (vrml_output, "Shape {\n");
      vrml_emit_layer_color (vrml_output, gc);
      fprintf (vrml_output, "geometry Box {\n");
      fprintf (vrml_output, "size %f %d %d\n", length,
	       gc->width, current_layer_thickness);
      fprintf (vrml_output, "}}\n");
    }
  if (gc->cap == Trace_Cap || gc->cap == Round_Cap || mode == VRML_EL_LASTPOLY
      || mode == VRML_EL_POLY)
    {
      fprintf (vrml_output, "Transform {\n");
      fprintf (vrml_output, "rotation 1 0 0 1.5708\n");
      fprintf (vrml_output, "translation %f 0 0\n", length / 2.);
      fprintf (vrml_output, "children [\n");
      fprintf (vrml_output, "Shape {\n");
      vrml_emit_layer_color (vrml_output, gc);
      fprintf (vrml_output, "geometry Cylinder {\n");
      fprintf (vrml_output, "height %d\n", current_layer_thickness);
      fprintf (vrml_output, "radius %f\n", (float) gc->width / 2.);
      fprintf (vrml_output, "}}]}\n");
      if (!zero_length && (mode != VRML_EL_POLY))
	{
	  fprintf (vrml_output, "Transform {\n");
	  fprintf (vrml_output, "rotation 1 0 0 1.5708\n");
	  fprintf (vrml_output, "translation %f 0 0\n", -length / 2.);
	  fprintf (vrml_output, "children [\n");
	  fprintf (vrml_output, "Shape {\n");
	  vrml_emit_layer_color (vrml_output, gc);
	  fprintf (vrml_output, "geometry Cylinder {\n");
	  fprintf (vrml_output, "height %d\n", current_layer_thickness);
	  fprintf (vrml_output, "radius %f\n", (float) gc->width / 2.);
	  fprintf (vrml_output, "}}]}\n");
	}
    }
  fprintf (vrml_output, "]}\n");
}

static void
vrml_draw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  if (drill_layer)
    return;

  vrml_emit_line (gc, x1, y1, x2, y2, VRML_EL_STANDARD);
}


static void
vrml_draw_arc (hidGC gc, Coord cx, Coord cy, Coord width, Coord height,
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
	  vrml_emit_line (gc, ox, oy, x, y,
			  (i == (n_steps)) ? VRML_EL_LASTPOLY : VRML_EL_POLY);

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

static void
vrml_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius)
{
  int i;
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

  if (drill_layer == SL_PDRILL)
    {
      fprintf (vrml_output, "Transform {\n");
      fprintf (vrml_output, "rotation 1 0 0 1.5708\n");
      fprintf (vrml_output, "translation %d %d 0\n", (int) cx, -(int) cy);
      fprintf (vrml_output, "children [\n");
      fprintf (vrml_output, "Shape {\n");
      vrml_emit_color (vrml_output, VRML_COPPER_COLOR_INDEX);
      fprintf (vrml_output, "geometry Cylinder {\n");
      fprintf (vrml_output, "height %d\n",
	       BOARD_THICKNESS + 2 * OUTER_COPPER_THICKNESS);
      fprintf (vrml_output, "radius %d\n", (int) radius);
      fprintf (vrml_output, "}}]}\n");
      fprintf (vrml_output, "Transform {\n");
      fprintf (vrml_output, "rotation 1 0 0 1.5708\n");
      fprintf (vrml_output, "translation %d %d 0\n", (int) cx, -(int) cy);
      fprintf (vrml_output, "children [\n");
      fprintf (vrml_output, "Shape {\n");
      vrml_emit_color (vrml_output, VRML_HOLE_COLOR_INDEX);
      fprintf (vrml_output, "geometry Cylinder {\n");
      fprintf (vrml_output, "height %d\n", HOLE_THICKNESS);
      fprintf (vrml_output, "radius %d\n",
	       (int) (radius - INNER_COPPER_THICKNESS));
      fprintf (vrml_output, "}}]}\n");
    }
  else if (drill_layer == SL_UDRILL)
    {
      if (opt_outline_type != VRML_OUTLINE_NONE && !opt_exp_inner_layers)
	{
	  fprintf (vrml_output, "Transform {\n");
	  fprintf (vrml_output, "rotation 1 0 0 1.5708\n");
	  fprintf (vrml_output, "translation %d %d 0\n", (int) cx, -(int) cy);
	  fprintf (vrml_output, "children [\n");
	  fprintf (vrml_output, "Shape {\n");
	  vrml_emit_color (vrml_output, VRML_HOLE_COLOR_INDEX);
	  fprintf (vrml_output, "geometry Cylinder {\n");
	  fprintf (vrml_output, "height %d\n", HOLE_THICKNESS);
	  fprintf (vrml_output, "radius %d\n", (int) radius);
	  fprintf (vrml_output, "}}]}\n");
	}
      else
	{
	  for (i = 0; i < max_group; i++)
	    {
	      if (group_data[i].draw)
		{
		  fprintf (vrml_output, "Transform {\n");
		  fprintf (vrml_output, "rotation 1 0 0 1.5708\n");
		  fprintf (vrml_output, "translation %d %d %d\n", (int) cx,
			   -(int) cy, group_data[i].z_offset);
		  fprintf (vrml_output, "children [\n");
		  fprintf (vrml_output, "Shape {\n");
		  vrml_emit_color (vrml_output, VRML_HOLE_COLOR_INDEX);
		  fprintf (vrml_output, "geometry Cylinder {\n");
		  fprintf (vrml_output, "height %d\n",
			   (group_data[i].solder
			    || group_data[i].
			    component) ? OUTER_COPPER_THICKNESS :
			   INNER_COPPER_THICKNESS);
		  fprintf (vrml_output, "radius %d\n", (int) radius);
		  fprintf (vrml_output, "}}]}\n");
		}
	    }
	}
    }
  else
    {
      fprintf (vrml_output, "Transform {\n");
      fprintf (vrml_output, "rotation 1 0 0 1.5708\n");
      fprintf (vrml_output, "translation %d %d %d\n", (int) cx, -(int) cy,
	       current_offset);
      fprintf (vrml_output, "children [\n");
      fprintf (vrml_output, "Shape {\n");
      vrml_emit_layer_color (vrml_output, gc);
      fprintf (vrml_output, "geometry Cylinder {\n");
      fprintf (vrml_output, "height %d\n", current_layer_thickness);
      fprintf (vrml_output, "radius %d\n", (int) radius);
      fprintf (vrml_output, "}}]}\n");
    }
}


static void
vrml_emit_polygon (hidGC gc, int n_coords, Coord * x, Coord * y,
		   int thickness, int offset, int color_index)
{
  int i, cw, cx, n;

  cw = check_poly_orientation (n_coords, x, y);
  cx = check_poly_convex (n_coords, x, y);


  fprintf (vrml_output, "Transform {\n");
  if (offset)
    fprintf (vrml_output, "translation 0 0 %d\n", offset);
  fprintf (vrml_output, "children [\n");
  fprintf (vrml_output, "Shape {\n");
  if (color_index < 0)
    {
      vrml_emit_color (vrml_output, color_index);
    }
  else
    {
      if (!gc)
	{
	  CRASH2;
	};
      vrml_emit_layer_color (vrml_output, gc);
    }
  fprintf (vrml_output, "geometry Extrusion {\n");
  fprintf (vrml_output, "beginCap TRUE\n");
  fprintf (vrml_output, "endCap TRUE\n");
  if (cw == POLY_CCW)
    {
      fprintf (vrml_output, "ccw FALSE\n");
    }
  else
    {
      fprintf (vrml_output, "ccw TRUE\n");
    }
  if (cx != POLY_CONVEX)
    {
      fprintf (vrml_output, "convex FALSE\n");
    }
  fprintf (vrml_output, "crossSection [");

  n = n_coords;
  if (x[n_coords - 1] != x[0] || y[n_coords - 1] != y[0])
    n++;

  for (i = 0; i < n; i++)
    {
      if (!(i % 10))
	fprintf (vrml_output, "\n");

      fprintf (vrml_output, "%d %d%s", (int) x[i % n_coords],
	       (int) y[i % n_coords], (i < (n - 1)) ? "," : "");
    }
  fprintf (vrml_output, " ]\n");
  fprintf (vrml_output, "spine [0 0 %d, 0 0 %d]\n", -thickness / 2,
	   thickness / 2);
  fprintf (vrml_output, "}}]}\n");
}

static void
vrml_fill_polygon (hidGC gc, int n_coords, Coord * x, Coord * y)
{
  if (outline_layer)
    return;

  vrml_emit_polygon (gc, n_coords, x, y, current_layer_thickness,
		     current_offset, VRML_LAYER_COLOR_INDEX);
}

static void
vrml_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
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
  vrml_fill_polygon (gc, 5, x, y);
}

static void
vrml_calibrate (double xval, double yval)
{
  CRASH;
}

static void
vrml_set_crosshair (int x, int y, int action)
{
}

static HID vrml_hid;

void
hid_vrml_init ()
{
  common_nogui_init (&vrml_hid);
  common_draw_helpers_init (&vrml_hid);


  vrml_hid.struct_size = sizeof (vrml_hid);
  vrml_hid.name = "vrml";
  vrml_hid.description = "VRML 3D export";
  vrml_hid.exporter = 1;

  vrml_hid.get_export_options = vrml_get_export_options;
  vrml_hid.do_export = vrml_do_export;
  vrml_hid.parse_arguments = vrml_parse_arguments;
  vrml_hid.set_layer = vrml_set_layer;
  vrml_hid.make_gc = vrml_make_gc;
  vrml_hid.destroy_gc = vrml_destroy_gc;
  vrml_hid.use_mask = vrml_use_mask;
  vrml_hid.set_color = vrml_set_color;
  vrml_hid.set_line_cap = vrml_set_line_cap;
  vrml_hid.set_line_width = vrml_set_line_width;
  vrml_hid.set_draw_xor = vrml_set_draw_xor;

  vrml_hid.draw_line = vrml_draw_line;
  vrml_hid.draw_arc = vrml_draw_arc;
  vrml_hid.draw_rect = vrml_draw_rect;
  vrml_hid.fill_circle = vrml_fill_circle;
  vrml_hid.fill_polygon = vrml_fill_polygon;

  vrml_hid.fill_rect = vrml_fill_rect;
  vrml_hid.calibrate = vrml_calibrate;
  vrml_hid.set_crosshair = vrml_set_crosshair;


  hid_register_hid (&vrml_hid);
}
