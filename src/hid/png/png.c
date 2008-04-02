/* $Id$ */
/*Sept 2007: patch to enable slanted squared lines*/
/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2006 Dan McMahill
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

/*
 *  Heavily based on the ps HID written by DJ Delorie
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "global.h"
#include "data.h"
#include "misc.h"

#include "hid.h"
#include "../hidint.h"
#include "png.h"

/* the gd library which makes this all so easy */
#include <gd.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented PNG function %s.\n", __FUNCTION__); abort()

static void *color_cache = NULL;
static void *brush_cache = NULL;

double scale = 1;
int x_shift = 0;
int y_shift = 0;
#define SCALE(x)   ((int)((x)/scale + 0.5))
#define SCALE_X(x) ((int)(((x) - x_shift)/scale))
#define SCALE_Y(x) ((int)(((x) - y_shift)/scale))

typedef struct color_struct
{
  /* the descriptor used by the gd library */
  int c;

  /* so I can figure out what rgb value c refers to */
  unsigned int r, g, b, a;

} color_struct;

typedef struct hid_gc_struct
{
  HID *me_pointer;
  EndCapStyle cap;
  int width;
  unsigned char r, g, b;
  int erase;
  int faded;
  color_struct *color;
  gdImagePtr brush;
} hid_gc_struct;

static color_struct *black = NULL, *white = NULL;
static gdImagePtr im = NULL;
static FILE *f = 0;
static int linewidth = -1;
static int lastgroup = -1;
static gdImagePtr lastbrush = (void *) -1;
static int lastcap = -1;
static int lastcolor = -1;
static int print_group[MAX_LAYER];
static int print_layer[MAX_LAYER];

#define FMT_gif "GIF"
#define FMT_jpg "JPEG"
#define FMT_png "PNG"

static const char *filetypes[] = {
#ifdef HAVE_GDIMAGEGIF
  FMT_gif,
#endif

#ifdef HAVE_GDIMAGEJPEG
  FMT_jpg,
#endif

#ifdef HAVE_GDIMAGEPNG
  FMT_png,
#endif

  NULL
};

HID_Attribute png_attribute_list[] = {
  /* other HIDs expect this to be first.  */
  {"outfile", "Graphics output file",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_pngfile 0

  {"dpi", "Scale factor (pixels/inch). 0 to scale to fix specified size",
   HID_Integer, 0, 1000, {100, 0, 0}, 0, 0},
#define HA_dpi 1

  {"x-max", "Maximum width (pixels).  0 to not constrain.",
   HID_Integer, 0, 10000, {0, 0, 0}, 0, 0},
#define HA_xmax 2

  {"y-max", "Maximum height (pixels).  0 to not constrain.",
   HID_Integer, 0, 10000, {0, 0, 0}, 0, 0},
#define HA_ymax 3

  {"xy-max", "Maximum width and height (pixels).  0 to not constrain.",
   HID_Integer, 0, 10000, {0, 0, 0}, 0, 0},
#define HA_xymax 4

  {"as-shown", "Export layers as shown on screen",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_as_shown 5

  {"monochrome", "Convert to monochrome",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_mono 6

  {"only-visible", "Limit the bounds of the PNG file to the visible items",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_only_visible 7

  {"use-alpha", "Make the background and any holes transparent",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_use_alpha 8

  {"format", "Graphics file format",
   HID_Enum, 0, 0, {2, 0, 0}, filetypes, 0},
#define HA_filetype 9
};

#define NUM_OPTIONS (sizeof(png_attribute_list)/sizeof(png_attribute_list[0]))

REGISTER_ATTRIBUTES (png_attribute_list)

static HID_Attr_Val png_values[NUM_OPTIONS];

static const char *get_file_suffix(void)
{
	const char *fmt;
	const char *result;
	fmt = filetypes[png_attribute_list[HA_filetype].default_val.int_value];
	/* or is it filetypes[png_attribute_list[HA_filetype].default_val.int_value]; ? */
	     if (strcmp (fmt, FMT_gif) == 0)  result=".gif";
	else if (strcmp (fmt, FMT_jpg) == 0)  result=".jpg";
	else if (strcmp (fmt, FMT_png) == 0)  result=".png";
	else {
		fprintf (stderr, "Error:  Invalid graphic file format\n");
		result=".???";
	}
	return result;
}

static HID_Attribute *
png_get_export_options (int *n)
{
  static char *last_made_filename = 0;
  const char *suffix = get_file_suffix();

  if (PCB) derive_default_filename(PCB->Filename, &png_attribute_list[HA_pngfile], suffix, &last_made_filename);

  if (n)
    *n = NUM_OPTIONS;
  return png_attribute_list;
}

static int comp_layer, solder_layer;

static int
group_for_layer (int l)
{
  if (l < max_layer + 2 && l >= 0)
    return GetLayerGroupNumberByNumber (l);
  /* else something unique */
  return max_layer + 3 + l;
}

static int
layer_sort (const void *va, const void *vb)
{
  int a = *(int *) va;
  int b = *(int *) vb;
  int al = group_for_layer (a);
  int bl = group_for_layer (b);
  int d = bl - al;

  if (a >= 0 && a <= max_layer + 1)
    {
      int aside = (al == solder_layer ? 0 : al == comp_layer ? 2 : 1);
      int bside = (bl == solder_layer ? 0 : bl == comp_layer ? 2 : 1);
      if (bside != aside)
	return bside - aside;
    }
  if (d)
    return d;
  return b - a;
}

static char *filename;
static BoxType *bounds;
static int in_mono, as_shown;

void
png_hid_export_to_file (FILE * the_file, HID_Attr_Val * options)
{
  int i;
  static int saved_layer_stack[MAX_LAYER];
  BoxType region;

  f = the_file;

  region.X1 = 0;
  region.Y1 = 0;
  region.X2 = PCB->MaxWidth;
  region.Y2 = PCB->MaxHeight;

  if (options[HA_only_visible].int_value)
    bounds = GetDataBoundingBox (PCB->Data);
  else
    bounds = &region;

  memset (print_group, 0, sizeof (print_group));
  memset (print_layer, 0, sizeof (print_layer));

  for (i = 0; i < max_layer; i++)
    {
      LayerType *layer = PCB->Data->Layer + i;
      if (layer->LineN || layer->TextN || layer->ArcN || layer->PolygonN)
	print_group[GetLayerGroupNumberByNumber (i)] = 1;
    }
  print_group[GetLayerGroupNumberByNumber (max_layer)] = 1;
  print_group[GetLayerGroupNumberByNumber (max_layer + 1)] = 1;
  for (i = 0; i < max_layer; i++)
    if (print_group[GetLayerGroupNumberByNumber (i)])
      print_layer[i] = 1;

  memcpy (saved_layer_stack, LayerStack, sizeof (LayerStack));
  as_shown = options[HA_as_shown].int_value;
  if (!options[HA_as_shown].int_value)
    {
      comp_layer = GetLayerGroupNumberByNumber (max_layer + COMPONENT_LAYER);
      solder_layer = GetLayerGroupNumberByNumber (max_layer + SOLDER_LAYER);
      qsort (LayerStack, max_layer, sizeof (LayerStack[0]), layer_sort);
    }
  linewidth = -1;
  lastbrush = (void *) -1;
  lastcap = -1;
  lastgroup = -1;
  lastcolor = -1;
  lastgroup = -1;

  in_mono = options[HA_mono].int_value;

  hid_expose_callback (&png_hid, bounds, 0);

  memcpy (LayerStack, saved_layer_stack, sizeof (LayerStack));
}

static void
png_do_export (HID_Attr_Val * options)
{
  int save_ons[MAX_LAYER + 2];
  int i;
  BoxType *bbox;
  int w, h;
  int xmax, ymax, dpi;
  const char *fmt;

  if (color_cache)
    {
      free (color_cache);
      color_cache = NULL;
    }

  if (brush_cache)
    {
      free (brush_cache);
      brush_cache = NULL;
    }

  if (!options)
    {
      png_get_export_options (0);
      for (i = 0; i < NUM_OPTIONS; i++)
	png_values[i] = png_attribute_list[i].default_val;
      options = png_values;
    }

  filename = options[HA_pngfile].str_value;
  if (!filename)
    filename = "pcb-out.png";

  /* figure out width and height of the board */
  if (options[HA_only_visible].int_value)
    {
      bbox = GetDataBoundingBox (PCB->Data);
      x_shift = bbox->X1;
      y_shift = bbox->Y1;
      h = bbox->Y2 - bbox->Y1;
      w = bbox->X2 - bbox->X1;
    }
  else
    {
      x_shift = 0;
      y_shift = 0;
      h = PCB->MaxHeight;
      w = PCB->MaxWidth;
    }

  /*
   * figure out the scale factor we need to make the image
   * fit in our specified PNG file size
   */
  xmax = ymax = dpi = 0;
  if (options[HA_dpi].int_value != 0)
    {
      dpi = options[HA_dpi].int_value;
      if (dpi < 0)
	{
	  fprintf (stderr, "ERROR:  dpi may not be < 0\n");
	  return;
	}
    }

  if (options[HA_xmax].int_value > 0)
    {
      xmax = options[HA_xmax].int_value;
      dpi = 0;
    }

  if (options[HA_ymax].int_value > 0)
    {
      ymax = options[HA_ymax].int_value;
      dpi = 0;
    }

  if (options[HA_xymax].int_value > 0)
    {
      dpi = 0;
      if (options[HA_xymax].int_value < xmax || xmax == 0)
	xmax = options[HA_xymax].int_value;
      if (options[HA_xymax].int_value < ymax || ymax == 0)
	ymax = options[HA_xymax].int_value;
    }

  if (xmax < 0 || ymax < 0)
    {
      fprintf (stderr, "ERROR:  xmax and ymax may not be < 0\n");
      return;
    }
    
  if (dpi > 0)
    {
      /*
       * a scale of 1 means 1 pixel is 1/100 mil 
       * a scale of 100,000 means 1 pixel is 1 inch
       * FIXME -- need to use a macro to go from PCB units
       * so if we ever change pcb's internal units, this 
       * will get updated.
       */
      scale = 100000.0 / dpi;
      w = w / scale;
      h = h / scale;
    }
  else if( xmax == 0 && ymax == 0)
    {
      fprintf(stderr, "ERROR:  You may not set both xmax, ymax,"
	      "and xy-max to zero\n");
      return;
    }
  else
    {
      if (ymax == 0 
	  || ( (xmax > 0) 
	       && ((w / xmax) > (h / ymax)) ) )
	{
	  h = (h * xmax) / w;
	  scale = w / xmax;
	  w = xmax;
	}
      else
	{
	  w = (w * ymax) / h;
	  scale = h / ymax;
	  h = ymax;
	}
    }

  im = gdImageCreate (w, h);

  /* 
   * Allocate white and black -- the first color allocated
   * becomes the background color
   */

  white = (color_struct *) malloc (sizeof (color_struct));
  white->r = white->g = white->b = 255;
  if (options[HA_use_alpha].int_value)
    white->a = 127;
  else
    white->a = 0;
  white->c = gdImageColorAllocateAlpha (im, white->r, white->g, white->b, white->a);

  black = (color_struct *) malloc (sizeof (color_struct));
  black->r = black->g = black->b = black->a = 0;
  black->c = gdImageColorAllocate (im, black->r, black->g, black->b);

  f = fopen (filename, "wb");
  if (!f)
    {
      perror (filename);
      return;
    }

  if (!options[HA_as_shown].int_value)
    hid_save_and_show_layer_ons (save_ons);

  png_hid_export_to_file (f, options);

  if (!options[HA_as_shown].int_value)
    hid_restore_layer_ons (save_ons);

  /* actually write out the image */
  fmt = filetypes[options[HA_filetype].int_value];
  
  if (strcmp (fmt, FMT_gif) == 0)
#ifdef HAVE_GDIMAGEGIF
    gdImageGif (im, f);
#else
    {
      gdImageDestroy (im);
      return;
    }
#endif
  else if (strcmp (fmt, FMT_jpg) == 0)
#ifdef HAVE_GDIMAGEJPEG
    gdImageJpeg (im, f, -1);
#else
    {
      gdImageDestroy (im);
      return;
    }
#endif
  else if (strcmp (fmt, FMT_png) == 0)
#ifdef HAVE_GDIMAGEPNG
    gdImagePng (im, f);
#else
    {
      gdImageDestroy (im);
      return;
    }
#endif
  else
    fprintf (stderr, "Error:  Invalid graphic file format."
	     "  This is a bug.  Please report it.\n");

  fclose (f);

  gdImageDestroy (im);
}

extern void hid_parse_command_line (int *argc, char ***argv);

static void
png_parse_arguments (int *argc, char ***argv)
{
  hid_register_attributes (png_attribute_list,
			   sizeof (png_attribute_list) /
			   sizeof (png_attribute_list[0]));
  hid_parse_command_line (argc, argv);
}


static int is_mask;
static int is_drill;

static int
png_set_layer (const char *name, int group)
{
  int idx = (group >= 0
	     && group <
	     max_layer) ? PCB->LayerGroups.Entries[group][0] : group;
  if (name == 0)
    name = PCB->Data->Layer[idx].Name;

  if (idx >= 0 && idx < max_layer && !print_layer[idx])
    return 0;
  if (SL_TYPE (idx) == SL_ASSY || SL_TYPE (idx) == SL_FAB)
    return 0;

  if (strcmp (name, "invisible") == 0)
    return 0;

  is_drill = (SL_TYPE (idx) == SL_PDRILL || SL_TYPE (idx) == SL_UDRILL);
  is_mask = (SL_TYPE (idx) == SL_MASK);

  if (SL_TYPE (idx) == SL_PASTE)
    return 0;

  if (as_shown)
    {
      switch (idx)
	{
	case SL (SILK, TOP):
	case SL (SILK, BOTTOM):
	  if (SL_MYSIDE (idx))
	    return PCB->ElementOn;
	  return 0;

	case SL (MASK, TOP):
	case SL (MASK, BOTTOM):
	  return TEST_FLAG (SHOWMASKFLAG, PCB) && SL_MYSIDE (idx);
	}
    }
  else
    {
      if (is_mask)
	return 0;

      switch (idx)
	{
	case SL (SILK, TOP):
	  return 1;
	case SL (SILK, BOTTOM):
	  return 0;
	}
    }

  return 1;
}


static hidGC
png_make_gc (void)
{
  hidGC rv = (hidGC) malloc (sizeof (hid_gc_struct));
  rv->me_pointer = &png_hid;
  rv->cap = Trace_Cap;
  rv->width = 1;
  rv->color = (color_struct *) malloc (sizeof (color_struct));
  rv->color->r = rv->color->g = rv->color->b = rv->color->a = 0;
  rv->color->c = 0;
  return rv;
}

static void
png_destroy_gc (hidGC gc)
{
  free (gc);
}

static void
png_use_mask (int use_it)
{
  /* does nothing */
}

static void
png_set_color (hidGC gc, const char *name)
{
  hidval cval;

  if (im == NULL)
    return;

  if (name == NULL)
    name = "#ff0000";

  if (strcmp (name, "erase") == 0 || strcmp (name, "drill") == 0)
    {
      /* FIXME -- should be background, not white */
      gc->color = white;
      gc->erase = 1;
      return;
    }
  gc->erase = 0;

  if (in_mono || (strcmp (name, "#000000") == 0))
    {
      gc->color = black;
      return;
    }

  if (hid_cache_color (0, name, &cval, &color_cache))
    {
      gc->color = cval.ptr;
    }
  else if (name[0] == '#')
    {
      gc->color = (color_struct *) malloc (sizeof (color_struct));
      sscanf (name + 1, "%2x%2x%2x", &(gc->color->r), &(gc->color->g),
	      &(gc->color->b));
      gc->color->c =
	gdImageColorAllocate (im, gc->color->r, gc->color->g, gc->color->b);
      cval.ptr = gc->color;
      hid_cache_color (1, name, &cval, &color_cache);
    }
  else
    {
      printf ("WE SHOULD NOT BE HERE!!!\n");
      gc->color = black;
    }

}

static void
png_set_line_cap (hidGC gc, EndCapStyle style)
{
  gc->cap = style;
}

static void
png_set_line_width (hidGC gc, int width)
{
  gc->width = width;
}

static void
png_set_draw_xor (hidGC gc, int xor)
{
  ;
}

static void
png_set_draw_faded (hidGC gc, int faded)
{
  gc->faded = faded;
}

static void
png_set_line_cap_angle (hidGC gc, int x1, int y1, int x2, int y2)
{
  CRASH;
}

static void
use_gc (hidGC gc)
{
  int need_brush = 0;

  if (gc->me_pointer != &png_hid)
    {
      fprintf (stderr, "Fatal: GC from another HID passed to png HID\n");
      abort ();
    }

  if (linewidth != gc->width)
    {
      /* Make sure the scaling doesn't erase lines completely */
      if (SCALE (gc->width) == 0 && gc->width > 0)
	gdImageSetThickness (im, 1);
      else
	gdImageSetThickness (im, SCALE (gc->width));
      linewidth = gc->width;
      need_brush = 1;
    }

  if (lastbrush != gc->brush || need_brush)
    {
      hidval bval;
      char name[256];
      char type;
      int r;

      switch (gc->cap)
        {
        case Round_Cap:
        case Trace_Cap:
          type = 'C';
          break;
        default:
        case Square_Cap:
          type = 'S';
          break;
        }
      r = SCALE (gc->width);
      if (r == 0 && gc->width > 0)
	r = 1;
      sprintf (name, "#%.2x%.2x%.2x_%c_%d", gc->color->r, gc->color->g,
	       gc->color->b, type, r);

      if (hid_cache_color (0, name, &bval, &brush_cache))
	{
	  gc->brush = bval.ptr;
	}
      else
	{
	  int bg, fg;
	  gc->brush = gdImageCreate (r, r);
	  bg = gdImageColorAllocate (gc->brush, 255, 255, 255);
	  fg =
	    gdImageColorAllocateAlpha (gc->brush, gc->color->r, gc->color->g,
				  gc->color->b, gc->color->a);
	  gdImageColorTransparent (gc->brush, bg);

	  /*
	   * if we shrunk to a radius/box width of zero, then just use
	   * a single pixel to draw with.
	   */
	  if (r <= 1)
	    gdImageFilledRectangle (gc->brush, 0, 0, 0, 0, fg);
	  else
	    {
	      if (type == 'C')
		{
		  gdImageFilledEllipse (gc->brush, r/2, r/2, r, r, fg);
		  /* Make sure the ellipse is the right exact size.  */
		  gdImageSetPixel (gc->brush, 0, r/2, fg);
		  gdImageSetPixel (gc->brush, r-1, r/2, fg);
		  gdImageSetPixel (gc->brush, r/2, 0, fg);
		  gdImageSetPixel (gc->brush, r/2, r-1, fg);
		}
	      else
		gdImageFilledRectangle (gc->brush, 0, 0, r-1, r-1, fg);
	    }
	  bval.ptr = gc->brush;
	  hid_cache_color (1, name, &bval, &brush_cache);
	}

      gdImageSetBrush (im, gc->brush);
      lastbrush = gc->brush;

    }

#define CBLEND(gc) (((gc->r)<<24)|((gc->g)<<16)|((gc->b)<<8)|(gc->faded))
  if (lastcolor != CBLEND (gc))
    {
      if (is_drill || is_mask)
	{
#ifdef FIXME
	  fprintf (f, "%d gray\n", gc->erase ? 0 : 1);
#endif
	  lastcolor = 0;
	}
      else
	{
	  double r, g, b;
	  r = gc->r;
	  g = gc->g;
	  b = gc->b;
	  if (gc->faded)
	    {
	      r = 0.8 * 255 + 0.2 * r;
	      g = 0.8 * 255 + 0.2 * g;
	      b = 0.8 * 255 + 0.2 * b;
	    }
#ifdef FIXME
	  if (gc->r == gc->g && gc->g == gc->b)
	    fprintf (f, "%g gray\n", r / 255.0);
	  else
	    fprintf (f, "%g %g %g rgb\n", r / 255.0, g / 255.0, b / 255.0);
#endif
	  lastcolor = CBLEND (gc);
	}
    }
}

static void
png_draw_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  use_gc (gc);
  gdImageRectangle (im,
		    SCALE_X (x1), SCALE_Y (y1),
		    SCALE_X (x2), SCALE_Y (y2), gc->color->c);
}

static void
png_fill_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  use_gc (gc);
  gdImageSetThickness (im, 0);
  linewidth = 0;
  gdImageFilledRectangle (im, SCALE_X (x1), SCALE_Y (y1),
			  SCALE_X (x2)-1, SCALE_Y (y2)-1, gc->color->c);
}

static void
png_draw_line (hidGC gc, int x1, int y1, int x2, int y2)
{
  if (x1 == x2 && y1 == y2)
    {
      int w = gc->width / 2;
      png_fill_rect (gc, x1 - w, y1 - w, x1 + w, y1 + w);
      return;
    }
  use_gc (gc);

  gdImageSetThickness (im, 0);
  linewidth = 0;
  if(gc->cap != Square_Cap || x1 == x2 || y1 == y2 )
    {
      gdImageLine (im, SCALE_X (x1), SCALE_Y (y1),
		   SCALE_X (x2), SCALE_Y (y2), gdBrushed);
    }
  else
    {
      /*
       * if we are drawing a line with a square end cap and it is
       * not purely horizontal or vertical, then we need to draw
       * it as a filled polygon.
       */
      int fg = gdImageColorResolve (im, gc->color->r, gc->color->g,
				    gc->color->b),
	w = gc->width, dx = x2 - x1, dy = y2 - y1, dwx, dwy;
      gdPoint p[4];
      double l = sqrt (dx * dx + dy * dy) * 2 * scale;
      dwx = -w / l * dy; dwy =  w / l * dx;
      p[0].x = SCALE_X (x1) + dwx - dwy; p[0].y = SCALE_Y(y1) + dwy + dwx;
      p[1].x = SCALE_X (x1) - dwx - dwy; p[1].y = SCALE_Y(y1) - dwy + dwx;
      p[2].x = SCALE_X (x2) - dwx + dwy; p[2].y = SCALE_Y(y2) - dwy - dwx;
      p[3].x = SCALE_X (x2) + dwx + dwy; p[3].y = SCALE_Y(y2) + dwy - dwx;
      gdImageFilledPolygon (im, p, 4, fg);
    }
}

static void
png_draw_arc (hidGC gc, int cx, int cy, int width, int height,
	      int start_angle, int delta_angle)
{
  int sa, ea;

  /* 
   * in gdImageArc, 0 degrees is to the right and +90 degrees is down
   * in pcb, 0 degrees is to the left and +90 degrees is down
   */
  start_angle = 180 - start_angle;
  delta_angle = -delta_angle;
  if (delta_angle > 0)
    {
      sa = start_angle;
      ea = start_angle + delta_angle;
    }
  else
    {
      sa = start_angle + delta_angle;
      ea = start_angle;
    }

  /* 
   * make sure we start between 0 and 360 otherwise gd does
   * strange things
   */
  while (sa < 0)
    {
      sa += 360;
      ea += 360;
    }
  while (sa >= 360)
    {
      sa -= 360;
      ea -= 360;
    }

#if 0
  printf ("draw_arc %d,%d %dx%d %d..%d %d..%d\n",
	  cx, cy, width, height, start_angle, delta_angle, sa, ea);
  printf ("gdImageArc (%p, %d, %d, %d, %d, %d, %d, %d)\n",
	  im, SCALE_X (cx), SCALE_Y (cy),
	  SCALE (width), SCALE (height), sa, ea, gc->color->c);
#endif
  use_gc (gc);
  gdImageSetThickness (im, 0);
  linewidth = 0;
  gdImageArc (im, SCALE_X (cx), SCALE_Y (cy),
	      SCALE (2 * width), SCALE (2 * height), sa, ea, gdBrushed);
}

static void
png_fill_circle (hidGC gc, int cx, int cy, int radius)
{
  use_gc (gc);

  gdImageSetThickness (im, 0);
  linewidth = 0;
  gdImageFilledEllipse (im, SCALE_X (cx), SCALE_Y (cy),
			SCALE (2 * radius), SCALE (2 * radius), gc->color->c);

}

static void
png_fill_polygon (hidGC gc, int n_coords, int *x, int *y)
{
  int i;
  gdPoint *points;

  points = (gdPoint *) malloc (n_coords * sizeof (gdPoint));
  if (points == NULL)
    {
      fprintf (stderr, "ERROR:  png_fill_polygon():  malloc failed\n");
      exit (1);
    }

  use_gc (gc);
  for (i = 0; i < n_coords; i++)
    {
      points[i].x = SCALE_X (x[i]);
      points[i].y = SCALE_Y (y[i]);
    }
  gdImageSetThickness (im, 0);
  linewidth = 0;
  gdImageFilledPolygon (im, points, n_coords, gc->color->c);
  free (points);
}

static void
png_calibrate (double xval, double yval)
{
  CRASH;
}

static void
png_set_crosshair (int x, int y, int a)
{
}

HID png_hid = {
  sizeof (HID),
  "png",
  "GIF/JPEG/PNG export.",
  0,				/* gui */
  0,				/* printer */
  1,				/* exporter */
  1,				/* poly before */
  0,				/* poly after */
  0,				/* poly dicer */
  png_get_export_options,
  png_do_export,
  png_parse_arguments,
  0 /* png_invalidate_wh */ ,
  0 /* png_invalidate_lr */ ,
  0 /* png_invalidate_all */ ,
  png_set_layer,
  png_make_gc,
  png_destroy_gc,
  png_use_mask,
  png_set_color,
  png_set_line_cap,
  png_set_line_width,
  png_set_draw_xor,
  png_set_draw_faded,
  png_set_line_cap_angle,
  png_draw_line,
  png_draw_arc,
  png_draw_rect,
  png_fill_circle,
  png_fill_polygon,
  png_fill_rect,
  png_calibrate,
  0 /* png_shift_is_pressed */ ,
  0 /* png_control_is_pressed */ ,
  0 /* png_get_coords */ ,
  png_set_crosshair,
  0 /* png_add_timer */ ,
  0 /* png_stop_timer */ ,
  0 /* png_log */ ,
  0 /* png_logv */ ,
  0 /* png_confirm_dialog */ ,
  0 /* png_report_dialog */ ,
  0 /* png_prompt_for */ ,
  0 /* png_fileselect */ ,
  0 /* png_attribute_dialog */ ,
  0 /* png_show_item */ ,
  0 /* png_beep */ ,
  0 /* png_progress */
};

#include "dolists.h"

void
hid_png_init ()
{
  apply_default_hid (&png_hid, 0);
  hid_register_hid (&png_hid);

#include "png_lists.h"
}
