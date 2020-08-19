/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *
 *  Breadboard export HID
 *  This code is based on the GERBER and OpenSCAD export HID
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

#include <cairo.h>

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented FreeCAD function %s.\n", __FUNCTION__); abort()
#define CRASH2 fprintf(stderr, "HID error: Internal error in %s.\n", __FUNCTION__); abort()

static cairo_surface_t *bboard_cairo_sfc;
static cairo_t *bboard_cairo_ctx;
#define ASSERT_CAIRO  if ((bboard_cairo_ctx == NULL) || (bboard_cairo_sfc == NULL)) \
                         return;

#define DPI_SCALE	150

#define BBEXT		".png"
#define MODELBASE	"models"
#define BBOARDBASE	"bboard"

/****************************************************************************************************
* Breakboard export filter parameters and options
****************************************************************************************************/

static HID bboard_hid;

static struct
{
  int draw;
  int exp;
  float z_offset;
  int solder;
  int component;
} group_data[MAX_LAYER];


#define HA_bboardfile 		0
#define HA_bgcolor	 	1
#define HA_skipsolder	 	2
#define HA_antialias 		3

static HID_Attribute bboard_options[] = {
/*
%start-doc options "Breadboard Export"
@ftable @code
@item --bbfile <string>
Name of breadboard image output file.
@end ftable
%end-doc
*/
  {
   "bbfile", "Braedboard file name", HID_String, 0, 0,
   {
    0, 0, 0}, 0, 0},
/*
%start-doc options "Breadboard Export"
@ftable @code
@item --bgcolor <string>
Background color. It can be HTML color code (RRGGBB); if left empty or set to 'transparent', transparent background is used.
@end ftable
%end-doc
*/
  {
   "bgcolor", "Background color (rrggbb)", HID_String, 0, 0,
   {
    0, 0, 0}, 0, 0},
/*
%start-doc options "Breadboard Export"
@ftable @code
@item --skipsolder <bool>
The solder layer will be ommited (if set to true) from output.
@end ftable
%end-doc
*/
  {
   "skipsolder", "Ignore solder layer", HID_Boolean, 0, 0,
   {
    1, 0, 0}, 0, 0},
/*
%start-doc options "Breadboard Export"
@ftable @code
@item --antialias <bool>
Connections are antialiased. Antialiasing applies only to wires, models are not antialiased..
@end ftable
%end-doc
*/
  {
   "antialias", "Antialiasing", HID_Boolean, 0, 0,
   {
    1, 0, 0}, 0, 0},
};

#define NUM_OPTIONS (sizeof(bboard_options)/sizeof(bboard_options[0]))

static HID_Attr_Val bboard_values[NUM_OPTIONS];

/****************************************************************************************************/
static const char *bboard_filename = 0;
static const char *bboard_bgcolor = 0;

Coord
bboard_scale_coord (Coord x)
{
  return ((x * DPI_SCALE / 254 / 10000) + 0) / 10;
}


/*******************************************
* Export filter implementation starts here
********************************************/

static HID_Attribute *
bboard_get_export_options (int *n)
{
  static char *last_made_filename = 0;
  if (PCB)
    derive_default_filename (PCB->Filename, &bboard_options[HA_bboardfile],
			     ".png", &last_made_filename);

  bboard_options[HA_bgcolor].default_val.str_value = strdup ("#FFFFFF");

  if (n)
    *n = NUM_OPTIONS;
  return bboard_options;
}

static int
bboard_validate_layer (const char *name, int group, int skipsolder)
{
  int idx = (group >= 0
	     && group <
	     max_group) ? PCB->LayerGroups.Entries[group][0] : group;

  if (name == 0)
    name = PCB->Data->Layer[idx].Name;

  if (strcmp (name, "invisible") == 0)
    return 0;

  if (SL_TYPE (idx) == SL_ASSY)
    return 0;

  if (strcmp (name, "route") == 0)
    return 0;

  if (strcmp (name, "outline") == 0)
    return 0;

  if (group_data[group].solder && skipsolder)
    return 0;


  if (group >= 0 && group < max_group)
    {
      if (!group_data[group].draw)
	return 0;
      group_data[group].exp = 1;
      return 1;
    }
  else
    return 0;
}

static void
bboard_get_layer_color (LayerType * layer, int *clr_r, int *clr_g, int *clr_b)
{
  char *clr;
  int r, g, b;

  if ((clr =
       AttributeGetFromList (&(layer->Attributes),
			     "BBoard::LayerColor")) != NULL)
    {
      if (clr[0] == '#')
	{
	  if (sscanf (&(clr[1]), "%02x%02x%02x", &r, &g, &b) == 3)
	    goto ok;
	}
    }

  if (layer->Color && (layer->Color[0] == '#'))
    {
      if (sscanf (&(layer->Color[1]), "%02x%02x%02x", &r, &g, &b) == 3)
	goto ok;
    }

  /* default color */
  *clr_r = 0xff;
  *clr_g = 0xff;
  *clr_b = 0xff;
  return;

ok:
  *clr_r = r;
  *clr_g = g;
  *clr_b = b;
  return;
}


static void
bboard_set_color_cairo (int r, int g, int b)
{
  ASSERT_CAIRO;

  cairo_set_source_rgb (bboard_cairo_ctx, ((float) r) / 255.,
			((float) g) / 255., ((float) b) / 255.);
}


static void
bboard_draw_line_cairo (Coord x1, Coord y1, Coord x2, Coord y2,
			Coord thickness)
{
  ASSERT_CAIRO;

  cairo_set_line_cap (bboard_cairo_ctx, CAIRO_LINE_CAP_ROUND);

  cairo_move_to (bboard_cairo_ctx, bboard_scale_coord (x1),
		 bboard_scale_coord (y1));
  cairo_line_to (bboard_cairo_ctx, bboard_scale_coord (x2),
		 bboard_scale_coord (y2));

  cairo_set_line_width (bboard_cairo_ctx, bboard_scale_coord (thickness));
  cairo_stroke (bboard_cairo_ctx);
}

static void
bboard_draw_arc_cairo (Coord x1, Coord y1, Coord x2, Coord y2, Coord x,
		       Coord y, Coord w, Coord h, Angle sa, Angle a,
		       Coord thickness)
{
  ASSERT_CAIRO;

  cairo_set_line_cap (bboard_cairo_ctx, CAIRO_LINE_CAP_ROUND);

  cairo_save (bboard_cairo_ctx);
  cairo_translate (bboard_cairo_ctx, bboard_scale_coord (x),
		   bboard_scale_coord (y));
  cairo_scale (bboard_cairo_ctx, bboard_scale_coord (w),
	       -bboard_scale_coord (h));
  if (a < 0)
    {
      cairo_arc_negative (bboard_cairo_ctx, 0., 0., 1.,
			  (sa + 180.) * M_PI / 180.,
			  (a + sa + 180.) * M_PI / 180.);
    }
  else
    {
      cairo_arc (bboard_cairo_ctx, 0., 0., 1., (sa + 180.) * M_PI / 180.,
		 (a + sa + 180.) * M_PI / 180.);
    }
  cairo_restore (bboard_cairo_ctx);

  cairo_set_line_width (bboard_cairo_ctx, bboard_scale_coord (thickness));
  cairo_stroke (bboard_cairo_ctx);
}

static bool
bboard_init_board_cairo (Coord x1, Coord y1, const char *color, int antialias)
{
  int r, g, b;
  float tr = 1.;		/* background transparency */

  if (color)
    {
      if (strlen (color) == 0 || !strcmp (color, "transparent"))
	{
	  tr = 0.;
	  r = g = b = 0xff;
	}
      else
	{
	  if ((color[0] != '#')
	      || sscanf (&(color[1]), "%02x%02x%02x", &r, &g, &b) != 3)
	    {
	      Message ("BBExport: Invalid background color \"%s\"", color);
	      r = g = b = 0xff;
	    }

	}
    }

  bboard_cairo_sfc = NULL;
  bboard_cairo_ctx = NULL;
  bboard_cairo_sfc =
    cairo_image_surface_create (CAIRO_FORMAT_ARGB32, bboard_scale_coord (x1),
				bboard_scale_coord (y1));
  if (bboard_cairo_sfc)
    {
      bboard_cairo_ctx = cairo_create (bboard_cairo_sfc);
      if (!bboard_cairo_ctx)
	{
	  cairo_surface_destroy (bboard_cairo_sfc);
	  bboard_cairo_sfc = NULL;
	}

    }

  if ((bboard_cairo_ctx != NULL) && (bboard_cairo_sfc != NULL))
    {
      cairo_set_antialias (bboard_cairo_ctx,
			   (antialias) ? CAIRO_ANTIALIAS_DEFAULT :
			   CAIRO_ANTIALIAS_NONE);
      cairo_rectangle (bboard_cairo_ctx, 0, 0, bboard_scale_coord (x1),
		       bboard_scale_coord (y1));
      cairo_set_source_rgba (bboard_cairo_ctx, ((float) r) / 255.,
			     ((float) g) / 255., ((float) b) / 255., tr);
      cairo_fill (bboard_cairo_ctx);
      return 1;
    }

  return 0;
}

static void
bboard_finish_board_cairo (void)
{
  ASSERT_CAIRO;

  cairo_surface_write_to_png (bboard_cairo_sfc, bboard_filename);

  cairo_destroy (bboard_cairo_ctx);
  cairo_surface_destroy (bboard_cairo_sfc);
}

static char *
bboard_get_model_filename (char *basename, char *value, bool nested)
{
  char *s;

  s =
    Concat (pcblibdir, PCB_DIR_SEPARATOR_S, MODELBASE, PCB_DIR_SEPARATOR_S,
	    BBOARDBASE, PCB_DIR_SEPARATOR_S, basename, (value
							&& nested) ?
	    PCB_DIR_SEPARATOR_S : "", (value
				       && nested) ? basename : "",
	    (value) ? "-" : "", (value) ? value : "", BBEXT, NULL);
  if (s)
    {
      if (!FileExists (s))
	{
	  free (s);
	  s = NULL;
	}
    }
  return s;
}


static int
bboard_parse_offset (char *s, Coord * ox, Coord * oy)
{
  Coord xx = 0, yy = 0;
  int n = 0, ln = 0;
  char val[32];

  while (sscanf (s, "%30s%n", val, &ln) >= 1)
    {
      switch (n)
	{
	case 0:
	  xx = GetValueEx (val, NULL, NULL, NULL, "mm");
	  break;
	case 1:
	  yy = GetValueEx (val, NULL, NULL, NULL, "mm");
	  break;
	}
      s = s + ln;
      n++;
    }
  if (n == 2)
    {
      *ox = xx;
      *oy = yy;
      return true;
    }
  else
    {
      return false;
    }

}


static void
bboard_export_element_cairo (ElementType * element, bool onsolder)
{
  cairo_surface_t *sfc;
  Coord ex, ey;
  Coord ox = 0, oy = 0;
  int w, h;
  Angle tmp_angle = 0.0;
  char *model_angle, *s = 0, *s1, *s2, *fname = NULL;
  bool offset_in_model = false;

  ASSERT_CAIRO;

  s1 = AttributeGetFromList (&(element->Attributes), "BBoard::Model");
  if (s1)
    {
      s = strdup (s1);
      if (!s)
	return;

      if ((s2 = strchr (s, ' ')) != NULL)
	{
	  *s2 = 0;
	  offset_in_model = bboard_parse_offset (s2 + 1, &ox, &oy);
	}
      if (!EMPTY_STRING_P (VALUE_NAME (element)))
	{
	  fname = bboard_get_model_filename (s, VALUE_NAME (element), true);
	  if (!fname)
	    fname =
	      bboard_get_model_filename (s, VALUE_NAME (element), false);
	}
      if (!fname)
	fname = bboard_get_model_filename (s, NULL, false);

      if (s)
	free (s);
    }
  if (!fname)
    {
      /* invalidate offset from BBoard::Model, if such model does not exist */
      offset_in_model = false;

      s = AttributeGetFromList (&(element->Attributes), "Footprint::File");
      if (s)
	{
	  if (!EMPTY_STRING_P (VALUE_NAME (element)))
	    {
	      fname =
		bboard_get_model_filename (s, VALUE_NAME (element), true);
	      if (!fname)
		fname =
		  bboard_get_model_filename (s, VALUE_NAME (element), false);
	    }
	  if (!fname)
	    fname = bboard_get_model_filename (s, NULL, false);
	}
    }
  if (!fname)
    {
      s = DESCRIPTION_NAME (element);
      if (!EMPTY_STRING_P (DESCRIPTION_NAME (element)))
	{
	  if (!EMPTY_STRING_P (VALUE_NAME (element)))
	    {
	      fname =
		bboard_get_model_filename (DESCRIPTION_NAME (element),
					   VALUE_NAME (element), true);
	      if (!fname)
		fname =
		  bboard_get_model_filename (DESCRIPTION_NAME (element),
					     VALUE_NAME (element), false);

	    }
	  if (!fname)
	    fname =
	      bboard_get_model_filename (DESCRIPTION_NAME (element), NULL,
					 false);
	}
    }

  if (!fname)
    return;

  sfc = cairo_image_surface_create_from_png (fname);

  free (fname);

  if (sfc)
    {
      w = cairo_image_surface_get_width (sfc);
      h = cairo_image_surface_get_height (sfc);

      // read offest from attribute
      if (!offset_in_model)
	{
	  s = AttributeGetFromList (&(element->Attributes), "BBoard::Offset");

	  /* Parse values with units... */
	  if (s)
	    {
	      bboard_parse_offset (s, &ox, &oy);
	    }
	}
      ex = bboard_scale_coord (element->MarkX);
      ey = bboard_scale_coord (element->MarkY);

      cairo_save (bboard_cairo_ctx);

      if ((model_angle =
	   AttributeGetFromList (&(element->Attributes),
				 "Footprint::RotationTracking")) != NULL)
	{
	  sscanf (model_angle, "%lf", &tmp_angle);
	}


      cairo_translate (bboard_cairo_ctx, ex, ey);
      if (TEST_FLAG (ONSOLDERFLAG, (element)))
	{
	  cairo_scale (bboard_cairo_ctx, 1, -1);
	}
      cairo_rotate (bboard_cairo_ctx, -tmp_angle * M_PI / 180.);
      cairo_set_source_surface (bboard_cairo_ctx, sfc,
				bboard_scale_coord (ox) - w / 2,
				bboard_scale_coord (oy) - h / 2);
      cairo_paint (bboard_cairo_ctx);

      cairo_restore (bboard_cairo_ctx);

      cairo_surface_destroy (sfc);
    }
}


static void
bboard_do_export (HID_Attr_Val * options)
{
  int i;
  int clr_r, clr_g, clr_b;
  LayerType *layer;


  if (!options)
    {
      bboard_get_export_options (0);
      for (i = 0; i < NUM_OPTIONS; i++)
	bboard_values[i] = bboard_options[i].default_val;
      options = bboard_values;
    }
  bboard_filename = options[HA_bboardfile].str_value;
  if (!bboard_filename)
    bboard_filename = "unknown.png";

  bboard_bgcolor = options[HA_bgcolor].str_value;
  if (!bboard_bgcolor)
    bboard_bgcolor = "FFFFFF";

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
      if (layer->LineN)
	group_data[GetLayerGroupNumberByNumber (i)].draw = 1;
    }

  bboard_init_board_cairo (PCB->MaxWidth, PCB->MaxHeight, bboard_bgcolor,
			   options[HA_antialias].int_value);

  /* write out components on solder side */
  ELEMENT_LOOP (PCB->Data);
  if (TEST_FLAG (ONSOLDERFLAG, (element)))
    {
      bboard_export_element_cairo (element, 1);
    }
  END_LOOP;

  /* write out components on component side */
  ELEMENT_LOOP (PCB->Data);
  if (!TEST_FLAG (ONSOLDERFLAG, (element)))
    {
      bboard_export_element_cairo (element, 0);
    }
  END_LOOP;

  /* draw all wires from all valid layers */
  for (i = max_copper_layer - 1; i >= 0; i--)
    {
      if (bboard_validate_layer
	  (PCB->Data->Layer[i].Name, GetLayerGroupNumberByNumber (i),
	   options[HA_skipsolder].int_value))
	{
	  bboard_get_layer_color (&(PCB->Data->Layer[i]), &clr_r, &clr_g,
				  &clr_b);
	  bboard_set_color_cairo (clr_r, clr_g, clr_b);
	  LINE_LOOP (&(PCB->Data->Layer[i]));
	  {
	    bboard_draw_line_cairo (line->Point1.X, line->Point1.Y,
				    line->Point2.X, line->Point2.Y,
				    line->Thickness);
	  }
	  END_LOOP;
	  ARC_LOOP (&(PCB->Data->Layer[i]));
	  {
	    bboard_draw_arc_cairo (arc->Point1.X, arc->Point1.Y,
				   arc->Point2.X, arc->Point2.Y,
				   arc->X, arc->Y,
				   arc->Width, arc->Height,
				   arc->StartAngle, arc->Delta,
				   arc->Thickness);
	  }
	  END_LOOP;
	}
    }

  bboard_finish_board_cairo ();

}

static void
bboard_parse_arguments (int *argc, char ***argv)
{
  hid_register_attributes (bboard_options,
			   sizeof (bboard_options) /
			   sizeof (bboard_options[0]));
  hid_parse_command_line (argc, argv);
}


static void
bboard_calibrate (double xval, double yval)
{
  CRASH;
}

static void
bboard_set_crosshair (int x, int y, int action)
{
}

static HID bboard_hid;

void
hid_bboard_init ()
{
  memset (&bboard_hid, 0, sizeof (bboard_hid));

  common_nogui_init (&bboard_hid);

  bboard_hid.struct_size = sizeof (bboard_hid);
  bboard_hid.name = "bboard";
  bboard_hid.description = "Breadboard export";
  bboard_hid.exporter = 1;

  bboard_hid.get_export_options = bboard_get_export_options;
  bboard_hid.do_export = bboard_do_export;
  bboard_hid.parse_arguments = bboard_parse_arguments;
  bboard_hid.calibrate = bboard_calibrate;
  bboard_hid.set_crosshair = bboard_set_crosshair;
  hid_register_hid (&bboard_hid);
}
