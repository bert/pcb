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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
#include <math.h>

#include "global.h"
#include "data.h"
#include "error.h"
#include "misc.h"

#include "hid.h"
#include "hid_draw.h"
#include "../hidint.h"
#include "hid/common/hidnogui.h"
#include "hid/common/draw_helpers.h"
#include "png.h"

/* the gd library which makes this all so easy */
#include <gd.h>

#include "hid/common/hidinit.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented PNG function %s.\n", __FUNCTION__); abort()

static HID png_hid;
static HID_DRAW png_graphics;

static void *color_cache = NULL;
static void *brush_cache = NULL;

static double bloat = 0;
static double scale = 1;
static Coord x_shift = 0;
static Coord y_shift = 0;
static int show_bottom_side;
#define SCALE(w)   ((int)round((w)/scale))
#define SCALE_X(x) ((int)round(((x) - x_shift)/scale))
#define SCALE_Y(y) ((int)round(((show_bottom_side ? (PCB->MaxHeight-(y)) : (y)) - y_shift)/scale))
#define SWAP_IF_SOLDER(a,b) do { Coord c; if (show_bottom_side) { c=a; a=b; b=c; }} while (0)

/* Used to detect non-trivial outlines */
#define NOT_EDGE_X(x) ((x) != 0 && (x) != PCB->MaxWidth)
#define NOT_EDGE_Y(y) ((y) != 0 && (y) != PCB->MaxHeight)
#define NOT_EDGE(x,y) (NOT_EDGE_X(x) || NOT_EDGE_Y(y))

static void png_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius);

/* The result of a failed gdImageColorAllocate() call */
#define BADC -1

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
  color_struct *color;
  gdImagePtr brush;
  int is_erase;
} hid_gc_struct;

static color_struct *black = NULL, *white = NULL;
static gdImagePtr im = NULL, master_im, mask_im = NULL;
static FILE *f = 0;
static int linewidth = -1;
static int lastgroup = -1;
static gdImagePtr lastbrush = (gdImagePtr)((void *) -1);
static int lastcap = -1;
static int print_group[MAX_GROUP];
static int print_layer[MAX_ALL_LAYER];

/* For photo-mode we need the following layers as monochrome masks:

   top soldermask
   top silk
   copper layers
   drill
*/

#define PHOTO_FLIP_X	1
#define PHOTO_FLIP_Y	2

static int photo_mode, photo_flip;
static gdImagePtr photo_copper[MAX_ALL_LAYER];
static gdImagePtr photo_silk, photo_mask, photo_drill, *photo_im;
static gdImagePtr photo_outline;
static int photo_groups[MAX_ALL_LAYER], photo_ngroups;
static int photo_has_inners;

static int doing_outline, have_outline;

#define FMT_gif "GIF"
#define FMT_jpg "JPEG"
#define FMT_png "PNG"

/* If this table has no elements in it, then we have no reason to
   register this HID and will refrain from doing so at the end of this
   file.  */

#undef HAVE_SOME_FORMAT

static const char *filetypes[] = {
#ifdef HAVE_GDIMAGEPNG
  FMT_png,
#define HAVE_SOME_FORMAT 1
#endif

#ifdef HAVE_GDIMAGEGIF
  FMT_gif,
#define HAVE_SOME_FORMAT 1
#endif

#ifdef HAVE_GDIMAGEJPEG
  FMT_jpg,
#define HAVE_SOME_FORMAT 1
#endif

  NULL
};


static const char *mask_colour_names[] = {
  "green",
  "red",
  "blue",
  "purple",
  "black",
  "white",
  NULL
};

// These values were arrived at through trial and error.
// One potential improvement (especially for white) is
// to use separate color_structs for the multiplication
// and addition parts of the mask math.
static const color_struct mask_colours[] = {
#define MASK_COLOUR_GREEN 0
  {.r = 60, .g = 160, .b = 60},
#define MASK_COLOUR_RED 1
  {.r = 140, .g = 25, .b = 25},
#define MASK_COLOUR_BLUE 2
  {.r = 50, .g = 50, .b = 160},
#define MASK_COLOUR_PURPLE 3
  {.r = 60, .g = 20, .b = 70},
#define MASK_COLOUR_BLACK 4
  {.r = 20, .g = 20, .b = 20},
#define MASK_COLOUR_WHITE 5
  {.r = 167, .g = 230, .b = 162}, // <-- needs improvement over FR4
  {}
};


static const char *plating_type_names[] = {
#define PLATING_TIN 0
  "tinned",
#define PLATING_GOLD 1
  "gold",
#define PLATING_SILVER 2
  "silver",
#define PLATING_COPPER 3
  "copper",
  NULL
};



static const char *silk_colour_names[] = {
  "white",
  "black",
  "yellow",
  NULL
};

static const color_struct silk_colours[] = {
#define SILK_COLOUR_WHITE 0
  {.r = 224, .g = 224, .b = 224},
#define SILK_COLOUR_BLACK 1
  {.r = 14, .g = 14, .b = 14},
#define SILK_COLOUR_YELLOW 2
  {.r = 185, .g = 185, .b = 10},
  {}
};

static const color_struct silk_top_shadow = {.r = 21, .g = 21, .b = 21};
static const color_struct silk_bottom_shadow = {.r = 14, .g = 14, .b = 14};

HID_Attribute png_attribute_list[] = {
  /* other HIDs expect this to be first.  */

/* %start-doc options "93 PNG Options"
@ftable @code
@item --outfile <string>
Name of the file to be exported to.
Parameter @code{<string>} can include a path.
@end ftable
%end-doc
*/
  {"outfile", "Graphics output file",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_pngfile 0

/* %start-doc options "93 PNG Options"
@ftable @code
@item --dpi
Scale factor in pixels/inch. Set to 0 to scale to size specified in the layout.
@end ftable
%end-doc
*/
  {"dpi", "Scale factor (pixels/inch). 0 to scale to specified size",
   HID_Integer, 0, 10000, {100, 0, 0}, 0, 0},
#define HA_dpi 1

/* %start-doc options "93 PNG Options"
@ftable @code
@item --x-max
Width of the png image in pixels. No constraint, when set to 0.
@end ftable
%end-doc
*/
  {"x-max", "Maximum width (pixels).  0 to not constrain",
   HID_Integer, 0, 10000, {0, 0, 0}, 0, 0},
#define HA_xmax 2

/* %start-doc options "93 PNG Options"
@ftable @code
@item --y-max
Height of the png output in pixels. No constraint, when set to 0.
@end ftable
%end-doc
*/
  {"y-max", "Maximum height (pixels).  0 to not constrain",
   HID_Integer, 0, 10000, {0, 0, 0}, 0, 0},
#define HA_ymax 3

/* %start-doc options "93 PNG Options"
@ftable @code
@item --xy-max
Maximum width and height of the PNG output in pixels. No constraint, when set to 0.
@end ftable
%end-doc
*/
  {"xy-max", "Maximum width and height (pixels).  0 to not constrain",
   HID_Integer, 0, 10000, {0, 0, 0}, 0, 0},
#define HA_xymax 4

/* %start-doc options "93 PNG Options"
@ftable @code
@item --screen-layer-order
Export layers in the order shown on screen.
@end ftable
%end-doc
*/
  {"screen-layer-order", "Export layers in the order shown on screen",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_as_shown 5

/* %start-doc options "93 PNG Options"
@ftable @code
@item --monochrome
Convert output to monochrome.
@end ftable
%end-doc
*/
  {"monochrome", "Convert to monochrome",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_mono 6

/* %start-doc options "93 PNG Options"
@ftable @code
@item --only-visible
Limit the bounds of the exported PNG image to the visible items.
@end ftable
%end-doc
*/
  {"only-visible", "Limit the bounds of the PNG image to the visible items",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_only_visible 7

/* %start-doc options "93 PNG Options"
@ftable @code
@item --use-alpha
Make the background and any holes transparent.
@end ftable
%end-doc
*/
  {"use-alpha", "Make the background and any holes transparent",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_use_alpha 8

/* %start-doc options "93 PNG Options"
@ftable @code
@item --fill-holes
Drill holes in pins/pads are filled, not hollow.
@end ftable
%end-doc
*/
  {"fill-holes", "Drill holes in pins/pads are filled, not hollow",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_fill_holes 9

/* %start-doc options "93 PNG Options"
@ftable @code
@item --format <string>
File format to be exported. Parameter @code{<string>} can be @samp{PNG},
@samp{GIF}, or @samp{JPEG}.
@end ftable
%end-doc
*/
  {"format", "Export file format",
   HID_Enum, 0, 0, {0, 0, 0}, filetypes, 0},
#define HA_filetype 10

/* %start-doc options "93 PNG Options"
@ftable @code
@item --png-bloat <num><dim>
Amount of extra thickness to add to traces, pads, or pin edges. The parameter
@samp{<num><dim>} is a number, appended by a dimension @samp{mm}, @samp{mil}, or
@samp{pix}. If no dimension is given, the default dimension is 1/100 mil.
@end ftable
%end-doc
*/
  {"png-bloat", "Amount (in/mm/mil/pix) to add to trace/pad/pin edges (1 = 1/100 mil)",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_bloat 11

/* %start-doc options "93 PNG Options"
@ftable @code
@cindex photo-mode
@item --photo-mode
Export a photo realistic image of the layout.
@end ftable
%end-doc
*/
  {"photo-mode", "Photo-realistic export mode",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_photo_mode 12

/* %start-doc options "93 PNG Options"
@ftable @code
@item --photo-flip-x
In photo-realistic mode, export the reverse side of the layout. Left-right flip.
@end ftable
%end-doc
*/
  {"photo-flip-x", "Show reverse side of the board, left-right flip",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_photo_flip_x 13

/* %start-doc options "93 PNG Options"
@ftable @code
@item --photo-flip-y
In photo-realistic mode, export the reverse side of the layout. Up-down flip.
@end ftable
%end-doc
*/
  {"photo-flip-y", "Show reverse side of the board, up-down flip",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_photo_flip_y 14

/* %start-doc options "93 PNG Options"
@ftable @code
@cindex photo-mask-colour
@item --photo-mask-colour <colour>
In photo-realistic mode, export the solder mask as this colour.
Parameter @code{<colour>} can be @samp{green}, @samp{red}, @samp{blue},
@samp{purple}, @samp{black}, or @samp{white}.
@end ftable
%end-doc
*/
  {"photo-mask-colour", "Colour for the exported colour mask",
   HID_Enum, 0, 0, {0, 0, 0}, mask_colour_names, 0},
#define HA_photo_mask_colour 15

/* %start-doc options "93 PNG Options"
@ftable @code
@cindex photo-plating
@item --photo-plating <colour>
In photo-realistic mode, export the exposed copper as though it has this type
of plating. Parameter @code{<colour>} can be @samp{tinned}, @samp{gold},
@samp{silver}, or @samp{copper}.
@end ftable
%end-doc
*/
  {"photo-plating", "Type of plating applied to exposed copper in photo-mode",
   HID_Enum, 0, 0, {0, 0, 0}, plating_type_names, 0},
#define HA_photo_plating 16

/* %start-doc options "93 PNG Options"
@ftable @code
@cindex photo-silk-colour
@item --photo-silk-colour <colour>
In photo-realistic mode, export the silk screen as this colour. Parameter
@code{<colour>} can be @samp{white}, @samp{black}, or @samp{yellow}.
@end ftable
%end-doc
*/
  {"photo-silk-colour", "Colour for the exported colour mask",
   HID_Enum, 0, 0, {0, 0, 0}, silk_colour_names, 0},
#define HA_photo_silk_colour 17

  {"ben-mode", ATTR_UNDOCUMENTED,
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_ben_mode 12

  {"ben-flip-x", ATTR_UNDOCUMENTED,
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_ben_flip_x 13

  {"ben-flip-y", ATTR_UNDOCUMENTED,
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_ben_flip_y 14
};

#define NUM_OPTIONS (sizeof(png_attribute_list)/sizeof(png_attribute_list[0]))

REGISTER_ATTRIBUTES (png_attribute_list)

static HID_Attr_Val png_values[NUM_OPTIONS];

static const char *get_file_suffix(void)
{
  const char *result = NULL;
  const char *fmt;

  fmt = filetypes[png_attribute_list[HA_filetype].default_val.int_value];

  if (fmt == NULL)
    ; /* Do nothing */
  else if (strcmp (fmt, FMT_gif) == 0)
    result=".gif";
  else if (strcmp (fmt, FMT_jpg) == 0)
    result=".jpg";
  else if (strcmp (fmt, FMT_png) == 0)
    result=".png";

  if (result == NULL)
    {
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

  if (PCB)
    derive_default_filename (PCB->Filename,
                             &png_attribute_list[HA_pngfile],
                             suffix,
                             &last_made_filename);

  if (n)
    *n = NUM_OPTIONS;
  return png_attribute_list;
}

static int top_group, bottom_group;

static int
layer_stack_sort (const void *va, const void *vb)
{
  int a_layer = *(int *) va;
  int b_layer = *(int *) vb;
  int a_group = GetLayerGroupNumberByNumber (a_layer);
  int b_group = GetLayerGroupNumberByNumber (b_layer);
  int aside = (a_group == bottom_group ? 0 : a_group == top_group ? 2 : 1);
  int bside = (b_group == bottom_group ? 0 : b_group == top_group ? 2 : 1);

  if (bside != aside)
    return bside - aside;

  if (b_group != a_group)
    return b_group - a_group;

  return b_layer - a_layer;
}

static const char *filename;
static BoxType *bounds;
static int in_mono, as_shown, fill_holes;

static void
parse_bloat (const char *str)
{
  UnitList extra_units = {
    { "pix", scale, 0 },
    { "px", scale, 0 },
    { "", 0, 0 }
  };
  if (str == NULL)
    return;
  bloat = GetValueEx (str, NULL, NULL, extra_units, "");
}

void
png_hid_export_to_file (FILE * the_file, HID_Attr_Val * options)
{
  int i;
  static int saved_layer_stack[MAX_LAYER];
  int saved_show_bottom_side;
  BoxType region;
  FlagType save_flags;

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

  for (i = 0; i < max_copper_layer; i++)
    {
      LayerType *layer = PCB->Data->Layer + i;
      if (layer->LineN || layer->TextN || layer->ArcN || layer->PolygonN)
	print_group[GetLayerGroupNumberByNumber (i)] = 1;
    }
  print_group[GetLayerGroupNumberBySide (BOTTOM_SIDE)] = 1;
  print_group[GetLayerGroupNumberBySide (TOP_SIDE)] = 1;
  for (i = 0; i < max_copper_layer; i++)
    if (print_group[GetLayerGroupNumberByNumber (i)])
      print_layer[i] = 1;

  memcpy (saved_layer_stack, LayerStack, sizeof (LayerStack));
  save_flags = PCB->Flags;
  saved_show_bottom_side = Settings.ShowBottomSide;

  as_shown = options[HA_as_shown].int_value;
  fill_holes = options[HA_fill_holes].int_value;

  if (!options[HA_as_shown].int_value)
    {
      CLEAR_FLAG (SHOWMASKFLAG, PCB);
      Settings.ShowBottomSide = 0;

      top_group = GetLayerGroupNumberBySide (TOP_SIDE);
      bottom_group = GetLayerGroupNumberBySide (BOTTOM_SIDE);
      qsort (LayerStack, max_copper_layer, sizeof (LayerStack[0]), layer_stack_sort);

      CLEAR_FLAG(THINDRAWFLAG, PCB);
      CLEAR_FLAG(THINDRAWPOLYFLAG, PCB);

      if (photo_mode)
	{
	  int i, n=0;
	  SET_FLAG (SHOWMASKFLAG, PCB);
	  photo_has_inners = 0;
	  if (top_group < bottom_group)
	    for (i = top_group; i <= bottom_group; i++)
	      {
		photo_groups[n++] = i;
		if (i != top_group && i != bottom_group
		    && ! IsLayerGroupEmpty (i))
		  photo_has_inners = 1;
	      }
	  else
	    for (i = top_group; i >= bottom_group; i--)
	      {
		photo_groups[n++] = i;
		if (i != top_group && i != bottom_group
		    && ! IsLayerGroupEmpty (i))
		  photo_has_inners = 1;
	      }
	  if (!photo_has_inners)
	    {
	      photo_groups[1] = photo_groups[n - 1];
	      n = 2;
	    }
	  photo_ngroups = n;

	  if (photo_flip)
	    {
	      for (i=0, n=photo_ngroups-1; i<n; i++, n--)
		{
		  int tmp = photo_groups[i];
		  photo_groups[i] = photo_groups[n];
		  photo_groups[n] = tmp;
		}
      } // end if (photo_flip)
  } // end if (photo_mode)
    } // end if (!options[HA_as_shown].int_value)
  linewidth = -1;
  lastbrush = (gdImagePtr)((void *) -1);
  lastcap = -1;
  lastgroup = -1;
  show_bottom_side = Settings.ShowBottomSide;

  in_mono = options[HA_mono].int_value;

  if (!photo_mode && Settings.ShowBottomSide)
    {
      int i, j;
      for (i=0, j=max_copper_layer-1; i<j; i++, j--)
	{
	  int k = LayerStack[i];
	  LayerStack[i] = LayerStack[j];
	  LayerStack[j] = k;
	}
    }

  hid_expose_callback (&png_hid, bounds, 0);

  memcpy (LayerStack, saved_layer_stack, sizeof (LayerStack));
  PCB->Flags = save_flags;
  Settings.ShowBottomSide = saved_show_bottom_side;
}

static void
clip (color_struct *dest, color_struct *source)
{
#define CLIP(var) \
  dest->var = source->var;	\
  if (dest->var > 255) dest->var = 255;	\
  if (dest->var < 0)   dest->var = 0;

  CLIP (r);
  CLIP (g);
  CLIP (b);
#undef CLIP
}

static void
blend (color_struct *dest, double a_amount, color_struct *a, color_struct *b)
{
  dest->r = a->r * a_amount + b->r * (1 - a_amount);
  dest->g = a->g * a_amount + b->g * (1 - a_amount);
  dest->b = a->b * a_amount + b->b * (1 - a_amount);
}

static void
multiply (color_struct *dest, color_struct *a, color_struct *b)
{
  dest->r = (a->r * b->r) / 255;
  dest->g = (a->g * b->g) / 255;
  dest->b = (a->b * b->b) / 255;
}

static void
add (color_struct *dest, double a_amount, const color_struct *a, double b_amount, const color_struct *b)
{
  dest->r = a->r * a_amount + b->r * b_amount;
  dest->g = a->g * a_amount + b->g * b_amount;
  dest->b = a->b * a_amount + b->b * b_amount;

  clip (dest, dest);
}

static void
subtract (color_struct *dest, double a_amount, const color_struct *a, double b_amount, const color_struct *b)
{
  dest->r = a->r * a_amount - b->r * b_amount;
  dest->g = a->g * a_amount - b->g * b_amount;
  dest->b = a->b * a_amount - b->b * b_amount;

  clip (dest, dest);
}

static void
rgb (color_struct *dest, int r, int g, int b)
{
  dest->r = r;
  dest->g = g;
  dest->b = b;
}

static int smshadows[3][3] = {
  {  1,  20,   1 },
  { 10,   0, -10 },
  { -1, -20,  -1 },
};

static int shadows[5][5] = {
  {  1,  1,   1,   1, -1 },
  {  1,  1,   1,  -1, -1 },
  {  1,  1,   0,  -1, -1 },
  {  1, -1,  -1,  -1, -1 },
  { -1, -1,  -1,  -1, -1 },
};

/* black and white are 0 and 1 */
#define TOP_SHADOW 2
#define BOTTOM_SHADOW 3

static void
ts_bs (gdImagePtr im)
{
  int x, y, sx, sy, si;
  for (x=0; x<gdImageSX(im); x++)
    for (y=0; y<gdImageSY(im); y++)
      {
	si = 0;
	for (sx=-2; sx<3; sx++)
	  for (sy=-2; sy<3; sy++)
	    if (!gdImageGetPixel (im, x+sx, y+sy))
	      si += shadows[sx+2][sy+2];
	if (gdImageGetPixel (im, x, y))
	  {
	    if (si > 1)
	      gdImageSetPixel (im, x, y, TOP_SHADOW);
	    else if (si < -1)
	      gdImageSetPixel (im, x, y, BOTTOM_SHADOW);
	  }
      }
}

static void
ts_bs_sm (gdImagePtr im)
{
  int x, y, sx, sy, si;
  for (x=0; x<gdImageSX(im); x++)
    for (y=0; y<gdImageSY(im); y++)
      {
	si = 0;
	for (sx=-1; sx<2; sx++)
	  for (sy=-1; sy<2; sy++)
	    if (!gdImageGetPixel (im, x+sx, y+sy))
	      si += smshadows[sx+1][sy+1];
	if (gdImageGetPixel (im, x, y))
	  {
	    if (si > 1)
	      gdImageSetPixel (im, x, y, TOP_SHADOW);
	    else if (si < -1)
	      gdImageSetPixel (im, x, y, BOTTOM_SHADOW);
	  }
      }
}

static void
png_do_export (HID_Attr_Val * options)
{
  int save_ons[MAX_ALL_LAYER];
  int i;
  BoxType *bbox;
  Coord w, h;
  Coord xmax, ymax;
  int dpi;
  const char *fmt;
  bool format_error = false;

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

  if (options[HA_photo_mode].int_value
      || options[HA_ben_mode].int_value)
    {
      photo_mode = 1;
      options[HA_mono].int_value = 1;
      options[HA_as_shown].int_value = 0;
      memset (photo_copper, 0, sizeof(photo_copper));
      photo_silk = photo_mask = photo_drill = 0;
      photo_outline = 0;
      if (options[HA_photo_flip_x].int_value
	  || options[HA_ben_flip_x].int_value)
	photo_flip = PHOTO_FLIP_X;
      else if (options[HA_photo_flip_y].int_value
	       || options[HA_ben_flip_y].int_value)
	photo_flip = PHOTO_FLIP_Y;
      else
	photo_flip = 0;
    }
  else
    photo_mode = 0;

  filename = options[HA_pngfile].str_value;
  if (!filename)
    filename = "pcb-out.png";

  /* figure out width and height of the board */
  if (options[HA_only_visible].int_value)
    {
      bbox = GetDataBoundingBox (PCB->Data);
      if (bbox == NULL)
        {
          fprintf (stderr, _("ERROR:  Unable to determine bounding box limits\n"));
          fprintf (stderr, _("ERROR:  Does the file contain any data?\n"));
          return;
        }
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
       * a scale of 1  means 1 pixel is 1 inch
       * a scale of 10 means 1 pixel is 10 inches
       */
      scale = round(INCH_TO_COORD(1) / (double) dpi);
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
	  scale = w / xmax;
	  h = h / scale;
	  w = xmax;
	}
      else
	{
	  scale = h / ymax;
	  w = w / scale;
	  h = ymax;
	}
    }

  im = gdImageCreate (w, h);
  if (im == NULL) 
    {
      Message ("%s():  gdImageCreate(%d, %d) returned NULL.  Aborting export.\n", __FUNCTION__, w, h);
      return;
    }
  
  master_im = im;

  parse_bloat (options[HA_bloat].str_value);

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
  if (white->c == BADC) 
    {
      Message ("%s():  gdImageColorAllocateAlpha() returned NULL.  Aborting export.\n", __FUNCTION__);
      return;
    }

  gdImageFilledRectangle (im, 0, 0, gdImageSX (im), gdImageSY (im), white->c);

  black = (color_struct *) malloc (sizeof (color_struct));
  black->r = black->g = black->b = black->a = 0;
  black->c = gdImageColorAllocate (im, black->r, black->g, black->b);
  if (black->c == BADC) 
    {
      Message ("%s():  gdImageColorAllocateAlpha() returned NULL.  Aborting export.\n", __FUNCTION__);
      return;
    }

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

  if (photo_mode)
    {
      int x, y;
      color_struct white, black, fr4;

      rgb (&white, 255, 255, 255);
      rgb (&black, 0, 0, 0);
      rgb (&fr4, 70, 70, 70);

      im = master_im;

      if (photo_copper[photo_groups[0]])
        ts_bs (photo_copper[photo_groups[0]]);
      if (photo_silk)
        ts_bs (photo_silk);
      if (photo_mask)
        ts_bs_sm (photo_mask);

      if (photo_outline && have_outline) {
	int black=gdImageColorResolve(photo_outline, 0x00, 0x00, 0x00);

	// go all the way around the image, trying to fill the outline
	for (x=0; x<gdImageSX(im); x++) {
	  gdImageFillToBorder(photo_outline, x, 0, black, black);
	  gdImageFillToBorder(photo_outline, x, gdImageSY(im)-1, black, black);
	}
	for (y=1; y<gdImageSY(im)-1; y++) {
	  gdImageFillToBorder(photo_outline, 0, y, black, black);
	  gdImageFillToBorder(photo_outline, gdImageSX(im)-1, y, black, black);

	}
      }


      for (x=0; x<gdImageSX (im); x++) 
	{
	  for (y=0; y<gdImageSY (im); y++)
	    {
	      color_struct p, cop;
	      color_struct mask_colour, silk_colour;
	      int cc, mask, silk;
	      int transparent;
	     
	      if (photo_outline && have_outline) {
		transparent=gdImageGetPixel(photo_outline, x, y);	      
	      } else {
		transparent=0;
	      }

	      mask = photo_mask ? gdImageGetPixel (photo_mask, x, y) : 0;
	      silk = photo_silk ? gdImageGetPixel (photo_silk, x, y) : 0;

	      if (photo_copper[photo_groups[1]]
		  && gdImageGetPixel (photo_copper[photo_groups[1]], x, y))
		rgb (&cop, 40, 40, 40);
	      else
		rgb (&cop, 100, 100, 110);

	      if (photo_ngroups == 2)
		blend (&cop, 0.3, &cop, &fr4);
	      
              if (photo_copper[photo_groups[0]])
                cc = gdImageGetPixel (photo_copper[photo_groups[0]], x, y);
              else
                cc = 0;

	      if (cc)
		{
		  int r;
		  
		  if (mask)
		    rgb (&cop, 220, 145, 230);
		  else
		    {
		      if (options[HA_photo_plating].int_value == PLATING_GOLD)
			{
			  // ENIG
			  rgb (&cop, 185, 146, 52);

			  // increase top shadow to increase shininess
			  if (cc == TOP_SHADOW)
			    blend (&cop, 0.7, &cop, &white);
			}
		      else if (options[HA_photo_plating].int_value == PLATING_TIN)
			{
			  // tinned
			  rgb (&cop, 140, 150, 160);

			  // add some variation to make it look more matte
			  r = (rand() % 5 - 2) * 2;
			  cop.r += r;
			  cop.g += r;
			  cop.b += r;
			}
		      else if (options[HA_photo_plating].int_value == PLATING_SILVER)
			{
			  // silver
			  rgb (&cop, 192, 192, 185);

			  // increase top shadow to increase shininess
			  if (cc == TOP_SHADOW)
			    blend (&cop, 0.7, &cop, &white);
			}
		      else if (options[HA_photo_plating].int_value == PLATING_COPPER)
			{
			  // copper
			  rgb (&cop, 184, 115, 51);

			  // increase top shadow to increase shininess
			  if (cc == TOP_SHADOW)
			    blend (&cop, 0.7, &cop, &white);
			}
		    }
		  
		  if (cc == TOP_SHADOW)
		    blend (&cop, 0.7, &cop, &white);
		  if (cc == BOTTOM_SHADOW)
		    blend (&cop, 0.7, &cop, &black);
		}

	      if (photo_drill && !gdImageGetPixel (photo_drill, x, y)) 
		{		
		  rgb (&p, 0, 0, 0);
		  transparent=1;
		}
	      else if (silk)
		{
		  silk_colour = silk_colours[options[HA_photo_silk_colour].int_value];
		  blend (&p, 1.0, &silk_colour, &silk_colour);
		  if (silk == TOP_SHADOW)
		    add (&p, 1.0, &p, 1.0, &silk_top_shadow);
		  else if (silk == BOTTOM_SHADOW)
		    subtract (&p, 1.0, &p, 1.0, &silk_bottom_shadow);
		}
	      else if (mask)
		{
		  p = cop;
		  mask_colour = mask_colours[options[HA_photo_mask_colour].int_value];
		  multiply (&p, &p, &mask_colour);
		  add (&p, 1, &p, 0.2, &mask_colour);
		  if (mask == TOP_SHADOW)
		    blend (&p, 0.7, &p, &white);
		  if (mask == BOTTOM_SHADOW)
		    blend (&p, 0.7, &p, &black);
		}
	      else
		p = cop;
	      
	      if (options[HA_use_alpha].int_value) {

		cc = (transparent)?\
		  gdImageColorResolveAlpha(im, 0, 0, 0, 127):\
		  gdImageColorResolveAlpha(im, p.r, p.g, p.b, 0);

	      } else {
		cc = (transparent)?\
		  gdImageColorResolve(im, 0, 0, 0):\
		  gdImageColorResolve(im, p.r, p.g, p.b);
	      }		  

	      if (photo_flip == PHOTO_FLIP_X)
		gdImageSetPixel (im, gdImageSX (im) - x - 1, y, cc);
	      else if (photo_flip == PHOTO_FLIP_Y)
		gdImageSetPixel (im, x, gdImageSY (im) - y - 1, cc);
	      else
		gdImageSetPixel (im, x, y, cc);
	    }
	}
    }

  /* actually write out the image */
  fmt = filetypes[options[HA_filetype].int_value];

  if (fmt == NULL)
    format_error = true;
  else if (strcmp (fmt, FMT_gif) == 0)
#ifdef HAVE_GDIMAGEGIF
    gdImageGif (im, f);
#else
    format_error = true;
#endif
  else if (strcmp (fmt, FMT_jpg) == 0)
#ifdef HAVE_GDIMAGEJPEG
    gdImageJpeg (im, f, -1);
#else
    format_error = true;
#endif
  else if (strcmp (fmt, FMT_png) == 0)
#ifdef HAVE_GDIMAGEPNG
    gdImagePng (im, f);
#else
    format_error = true;
#endif
  else
    format_error = true;

  if (format_error)
    fprintf (stderr, "Error:  Invalid graphic file format."
                     "  This is a bug.  Please report it.\n");

  fclose (f);

  gdImageDestroy (im);
}

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
static int is_copper;

static int
png_set_layer (const char *name, int group, int empty)
{
  int idx = (group >= 0
	     && group <
	     max_group) ? PCB->LayerGroups.Entries[group][0] : group;
  if (name == 0)
    name = PCB->Data->Layer[idx].Name;

  doing_outline = 0;

  if (idx >= 0 && idx < max_copper_layer && !print_layer[idx])
    return 0;
  if (SL_TYPE (idx) == SL_ASSY || SL_TYPE (idx) == SL_FAB)
    return 0;

  if (strcmp (name, "invisible") == 0)
    return 0;

  is_drill = (SL_TYPE (idx) == SL_PDRILL || SL_TYPE (idx) == SL_UDRILL);
  is_mask = (SL_TYPE (idx) == SL_MASK);
  is_copper = (SL_TYPE (idx) == 0);

  if (is_drill && fill_holes)
    return 0;

  if (SL_TYPE (idx) == SL_PASTE)
    return 0;

  if (photo_mode)
    {
      switch (idx)
	{
	case SL (SILK, TOP):
	  if (photo_flip)
	    return 0;
	  photo_im = &photo_silk;
	  break;
	case SL (SILK, BOTTOM):
	  if (!photo_flip)
	    return 0;
	  photo_im = &photo_silk;
	  break;

	case SL (MASK, TOP):
	  if (photo_flip)
	    return 0;
	  photo_im = &photo_mask;
	  break;
	case SL (MASK, BOTTOM):
	  if (!photo_flip)
	    return 0;
	  photo_im = &photo_mask;
	  break;

	case SL (PDRILL, 0):
	case SL (UDRILL, 0):
	  photo_im = &photo_drill;
	  break;

	default:
	  if (idx < 0)
	    return 0;

	  if (strcmp (name, "outline") == 0)
	    {
	      doing_outline = 1;
	      have_outline = 0;
	      photo_im = &photo_outline;
	    }
	  else
	    photo_im = photo_copper + group;

	  break;
	}

      if (! *photo_im)
	{
	  static color_struct *black = NULL, *white = NULL;
	  *photo_im = gdImageCreate (gdImageSX (im), gdImageSY (im));
          if (photo_im == NULL) 
	    {
	      Message ("%s():  gdImageCreate(%d, %d) returned NULL.  Aborting export.\n", __FUNCTION__, 
		       gdImageSX (im), gdImageSY (im));
	      return 0;
	    }


	  white = (color_struct *) malloc (sizeof (color_struct));
	  white->r = white->g = white->b = 255;
	  white->a = 0;
	  white->c = gdImageColorAllocate (*photo_im, white->r, white->g, white->b);
	  if (white->c == BADC) 
	    {
	      Message ("%s():  gdImageColorAllocate() returned NULL.  Aborting export.\n", __FUNCTION__);
	      return 0;
	    }

	  black = (color_struct *) malloc (sizeof (color_struct));
	  black->r = black->g = black->b = black->a = 0;
	  black->c = gdImageColorAllocate (*photo_im, black->r, black->g, black->b);
	  if (black->c == BADC) 
	    {
	      Message ("%s(): gdImageColorAllocate() returned NULL.  Aborting export.\n", __FUNCTION__);
	      return 0;
	    }

	  if (idx == SL (PDRILL, 0)
	      || idx == SL (UDRILL, 0))
	    gdImageFilledRectangle (*photo_im, 0, 0, gdImageSX (im), gdImageSY (im), black->c);
	}
      im = *photo_im;
      return 1;
    }

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
  rv->is_erase = 0;
  return rv;
}

static void
png_destroy_gc (hidGC gc)
{
  free (gc);
}

static void
png_use_mask (enum mask_mode mode)
{
  if (photo_mode)
    return;

  if (mode == HID_MASK_CLEAR)
    {
      return;
    }
  if (mode != HID_MASK_OFF)
    {
      if (mask_im == NULL)
	{
	  mask_im = gdImageCreate (gdImageSX (im), gdImageSY (im));
	  if (!mask_im)
	    {
	      Message ("%s():  gdImageCreate(%d, %d) returned NULL.  Corrupt export!\n",
		       __FUNCTION__, gdImageSY (im), gdImageSY (im));
	      return;
	    }
	  gdImagePaletteCopy (mask_im, im);
	}
      im = mask_im;
      gdImageFilledRectangle (mask_im, 0, 0, gdImageSX (mask_im), gdImageSY (mask_im), white->c);
    }
  else
    {
      int x, y, c;

      im = master_im;

      for (x=0; x<gdImageSX (im); x++)
	for (y=0; y<gdImageSY (im); y++)
	  {
	    c = gdImageGetPixel (mask_im, x, y);
	    if (c)
	      gdImageSetPixel (im, x, y, c);
	  }
    }
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
      gc->color = white;
      gc->is_erase = 1;
      return;
    }
  gc->is_erase = 0;

  if (in_mono || (strcmp (name, "#000000") == 0))
    {
      gc->color = black;
      return;
    }

  if (hid_cache_color (0, name, &cval, &color_cache))
    {
      gc->color = (color_struct *)cval.ptr;
    }
  else if (name[0] == '#')
    {
      gc->color = (color_struct *) malloc (sizeof (color_struct));
      sscanf (name + 1, "%2x%2x%2x", &(gc->color->r), &(gc->color->g),
	      &(gc->color->b));
      gc->color->c =
	gdImageColorAllocate (master_im, gc->color->r, gc->color->g, gc->color->b);
      if (gc->color->c == BADC) 
	{
	  Message ("%s():  gdImageColorAllocate() returned NULL.  Aborting export.\n", __FUNCTION__);
	  return;
	}
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
png_set_line_width (hidGC gc, Coord width)
{
  gc->width = width;
}

static void
png_set_draw_xor (hidGC gc, int xor_)
{
  ;
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
	gdImageSetThickness (im, SCALE (gc->width + 2*bloat));
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
      if (gc->width)
	r = SCALE (gc->width + 2*bloat);
      else
	r = 1;

      /* do not allow a brush size that is zero width.  In this case limit to a single pixel. */
      if (r == 0)
	{
	  r = 1;
        }

      sprintf (name, "#%.2x%.2x%.2x_%c_%d", gc->color->r, gc->color->g,
	       gc->color->b, type, r);

      if (hid_cache_color (0, name, &bval, &brush_cache))
	{
	  gc->brush = (gdImagePtr)bval.ptr;
	}
      else
	{
	  int bg, fg;
	  gc->brush = gdImageCreate (r, r);
	  if (gc->brush == NULL) 
	    {
	      Message ("%s():  gdImageCreate(%d, %d) returned NULL.  Aborting export.\n", __FUNCTION__, r, r);
	      return;
	    }

	  bg = gdImageColorAllocate (gc->brush, 255, 255, 255);
	  if (bg == BADC) 
	    {
	      Message ("%s():  gdImageColorAllocate() returned NULL.  Aborting export.\n", __FUNCTION__);
	      return;
	    }
	  fg =
	    gdImageColorAllocateAlpha (gc->brush, gc->color->r, gc->color->g,
				       gc->color->b, 0); 
	  if (fg == BADC) 
	    {
	      Message ("%s():  gdImageColorAllocate() returned NULL.  Aborting export.\n", __FUNCTION__);
	      return;
	    }
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
}

static void
png_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  use_gc (gc);
  gdImageRectangle (im,
		    SCALE_X (x1), SCALE_Y (y1),
		    SCALE_X (x2), SCALE_Y (y2), gc->color->c);
}

static void
png_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  use_gc (gc);
  gdImageSetThickness (im, 0);
  linewidth = 0;

  y1 -= bloat;
  y2 += bloat;
  SWAP_IF_SOLDER (y1, y2);

  gdImageFilledRectangle (im, SCALE_X (x1-bloat), SCALE_Y (y1),
			  SCALE_X (x2+bloat)-1, SCALE_Y (y2)-1, gc->color->c);
  have_outline |= doing_outline;
}

static void
png_draw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  if (x1 == x2 && y1 == y2)
    {
      Coord w = gc->width / 2;
      if (gc->cap != Square_Cap)
	png_fill_circle (gc, x1, y1, w);
      else
	png_fill_rect (gc, x1 - w, y1 - w, x1 + w, y1 + w);
      return;
    }
  use_gc (gc);

  if (NOT_EDGE (x1, y1) || NOT_EDGE (x2, y2))
    have_outline |= doing_outline;
  if (doing_outline)
    {
      /* Special case - lines drawn along the bottom or right edges
	 are brought in by a pixel to make sure we have contiguous
	 outlines.  */
      if (x1 == PCB->MaxWidth && x2 == PCB->MaxWidth)
	{
	  x1 -= scale/2;
	  x2 -= scale/2;
	}
      if (y1 == PCB->MaxHeight && y2 == PCB->MaxHeight)
	{
	  y1 -= scale/2;
	  y2 -= scale/2;
	}
    }

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
				    gc->color->b);
      Coord w = gc->width;
      Coord dwx, dwy;

      gdPoint p[4];
      double l = Distance(x1, y1, x2, y2) * 2;

      w += 2 * bloat;
      dwx = -w / l * (y2 - y1); dwy =  w / l * (x2 - x1);
      p[0].x = SCALE_X (x1 + dwx - dwy); p[0].y = SCALE_Y(y1 + dwy + dwx);
      p[1].x = SCALE_X (x1 - dwx - dwy); p[1].y = SCALE_Y(y1 - dwy + dwx);
      p[2].x = SCALE_X (x2 - dwx + dwy); p[2].y = SCALE_Y(y2 - dwy - dwx);
      p[3].x = SCALE_X (x2 + dwx + dwy); p[3].y = SCALE_Y(y2 + dwy - dwx);
      gdImageFilledPolygon (im, p, 4, fg);
    }
}

static void
png_draw_arc (hidGC gc, Coord cx, Coord cy, Coord width, Coord height,
	      Angle start_angle, Angle delta_angle)
{
  Angle sa, ea;

  /*
   * zero angle arcs need special handling as gd will output either
   * nothing at all or a full circle when passed delta angle of 0 or 360.
   */
  if (delta_angle == 0) {
    Coord x = (width * cos (start_angle * M_PI / 180));
    Coord y = (width * sin (start_angle * M_PI / 180));
    x = cx - x;
    y = cy + y;
    png_fill_circle (gc, x, y, gc->width / 2);
    return;
  }

  /* 
   * in gdImageArc, 0 degrees is to the right and +90 degrees is down
   * in pcb, 0 degrees is to the left and +90 degrees is down
   */
  start_angle = 180 - start_angle;
  delta_angle = -delta_angle;
  if (show_bottom_side)
    {
      start_angle = - start_angle;
      delta_angle = -delta_angle;
    }
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
  sa = NormalizeAngle (sa);
  ea = NormalizeAngle (ea);

  have_outline |= doing_outline;

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
png_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius)
{
  Coord my_bloat;

  use_gc (gc);

  if (fill_holes && gc->is_erase && is_copper)
    return;

  if (gc->is_erase)
    my_bloat = -2 * bloat;
  else
    my_bloat = 2 * bloat;


  have_outline |= doing_outline;

  gdImageSetThickness (im, 0);
  linewidth = 0;
  gdImageFilledEllipse (im, SCALE_X (cx), SCALE_Y (cy),
			SCALE (2 * radius + my_bloat), SCALE (2 * radius + my_bloat), gc->color->c);

}

static void
png_fill_polygon (hidGC gc, int n_coords, Coord *x, Coord *y)
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
      if (NOT_EDGE (x[i], y[i]))
	have_outline |= doing_outline;
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

#include "dolists.h"

void
hid_png_init ()
{
  memset (&png_hid, 0, sizeof (HID));
  memset (&png_graphics, 0, sizeof (HID_DRAW));

  common_nogui_init (&png_hid);
  common_draw_helpers_init (&png_graphics);

  png_hid.struct_size = sizeof (HID);
  png_hid.name        = "png";
  png_hid.description = "GIF/JPEG/PNG export";
  png_hid.exporter    = 1;
  png_hid.poly_before = 1;

  png_hid.get_export_options  = png_get_export_options;
  png_hid.do_export           = png_do_export;
  png_hid.parse_arguments     = png_parse_arguments;
  png_hid.set_layer           = png_set_layer;
  png_hid.calibrate           = png_calibrate;
  png_hid.set_crosshair       = png_set_crosshair;

  png_hid.graphics            = &png_graphics;

  png_graphics.make_gc        = png_make_gc;
  png_graphics.destroy_gc     = png_destroy_gc;
  png_graphics.use_mask       = png_use_mask;
  png_graphics.set_color      = png_set_color;
  png_graphics.set_line_cap   = png_set_line_cap;
  png_graphics.set_line_width = png_set_line_width;
  png_graphics.set_draw_xor   = png_set_draw_xor;
  png_graphics.draw_line      = png_draw_line;
  png_graphics.draw_arc       = png_draw_arc;
  png_graphics.draw_rect      = png_draw_rect;
  png_graphics.fill_circle    = png_fill_circle;
  png_graphics.fill_polygon   = png_fill_polygon;
  png_graphics.fill_rect      = png_fill_rect;

#ifdef HAVE_SOME_FORMAT
  hid_register_hid (&png_hid);

#include "png_lists.h"
#endif
}
