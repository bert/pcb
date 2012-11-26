/**
 * \file src/hid/html5/html5.c
 *
 * \brief HTML5 export HID.
 *
 * \author Copyright (C) 2012 Bert Timmerman <bert.timmerman@xs4all.nl>\n
 * This code is based on the Gerber HID exporter.
 *
 * What is the HTML5 HID exporter exporting?\n
 * The HTML5 HID exporter creates a HTML page per layer(group).\n
 * This HTML page contains a single canvas on which all entities of that
 * layer are drawn upon.\n
 * \n
 * What is a Canvas?\n
 * A canvas is a rectangular area on an HTML page, and it is specified
 * with the \<canvas\> element.\n
 *
 * The HTML5 <canvas> element is used to draw graphics, on the fly, via
 * scripting (usually JavaScript).\n
 * The \<canvas\> element is only a container for graphics.\n
 * You must use a script to actually draw the graphics.\n
 * Canvas has several methods for drawing paths, boxes, circles,
 * characters, and adding images.\n
 *
 * \note By default, the \<canvas\> element has no border and no
 * content.\n
 *
 * Browser Support.\n
 * Internet Explorer 9, Firefox, Opera, Chrome, and Safari support the
 * \<canvas\> element.\n
 *
 * \note Internet Explorer 8 and earlier versions, do not support the
 * \<canvas\> element.\n
 *
 * \n
 * <h3><c>COPYRIGHT<\c></h3>\n
 * \n
 * PCB, interactive printed circuit board design.\n
 * \n
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.\n
 * \n
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.\n
 * \n
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 * MA  02110-1301 USA\n
 * \n
 * Contact addresses for paper mail and Email:\n
 * Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA\n
 * haceaton@aplcomm.jhuapl.edu\n
 * \n
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

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#include <time.h>

#include "config.h"
#include "global.h"
#include "data.h"
#include "misc.h"
#include "error.h"
#include "draw.h"
#include "pcb-printf.h"

#include "hid.h"
#include "hid_draw.h"
#include "../hidint.h"
#include "hid/common/hidnogui.h"
#include "hid/common/draw_helpers.h"

#include "hid/common/hidinit.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented HTML5 function %s.\n", __FUNCTION__); abort()


/* Function prototypes. */
static HID_Attribute * html5_get_export_options (int *n);
static void html5_do_export (HID_Attr_Val * options);
static void html5_parse_arguments (int *argc, char ***argv);
static int html5_set_layer (const char *name, int group, int empty);
static hidGC html5_make_gc (void);
static void html5_destroy_gc (hidGC gc);
static void html5_use_mask (int use_it);
static void html5_set_color (hidGC gc, const char *name);
static void html5_set_line_cap (hidGC gc, EndCapStyle style);
static void html5_set_line_width (hidGC gc, Coord width);
static void html5_set_draw_xor (hidGC gc, int _xor);
static void html5_draw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);
static void html5_draw_arc (hidGC gc, Coord cx, Coord cy, Coord width, Coord height, Angle start_angle, Angle delta_angle);
static void html5_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);
static void html5_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius);
static void html5_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);
static void html5_calibrate (double xval, double yval);
static void html5_set_crosshair (int x, int y, int action);
static void html5_fill_polygon (hidGC gc, int n_coords, Coord *x, Coord *y);

/* Utility routines. */

/**
 * Maximum canvas size in the X-direction is 2^16 pixels (0 .. 65536).
 */
#define HTML5_CANVAS_MAX_X 65535
/**
 * Maximum canvas size in the Y-direction is based on a 4:3 aspect
 * ratio (0 .. 49151).
 */
#define HTML5_CANVAS_MAX_Y 49151

/**
 * Arcs are drawn CounterClockWise (CCW) by default.
 */
#define CCW true

/* Private data structures. */
static int verbose;
static int all_layers;
static int metric;
static char *x_convspec, *y_convspec;
static int is_mask, was_drill;
static int is_drill;
static int current_mask;
static int flash_drills;
static int copy_outline_mode;
static LayerType *outline_layer;
static void *color_cache = NULL;

static int layer_list_idx;

/* Auto-geneated outline width of 1 mil. */
#define AUTO_OUTLINE_WIDTH MIL_TO_COORD(1)

static HID html5_hid;
static HID_DRAW html5_graphics;

/* The result of a failed ColorAllocate() call */
#define BADCOLOR -1

typedef struct hid_gc_struct
{
  EndCapStyle cap;
  int width;
  int r;
  int g;
  int b;
  int color;
  int erase;
  int drill;
} hid_gc_struct;

static FILE *f = NULL;
static char *filename = NULL;
static char *filesuff = NULL;
static char *layername = NULL;
static int lncount = 0;
static int pagecount = 0;
static int linewidth = -1;
static int lastgroup = -1;
static int lastcap = -1;
static int print_group[MAX_LAYER];
static int print_layer[MAX_LAYER];
static int lastX, lastY; /* the last X and Y coordinate */

static const char *copy_outline_names[] = {
#define COPY_OUTLINE_NONE 0
  "none",
#define COPY_OUTLINE_MASK 1
  "mask",
#define COPY_OUTLINE_SILK 2
  "silk",
#define COPY_OUTLINE_ALL 3
  "all",
  NULL
};

static HID_Attribute html5_options[] =
{

/* %start-doc options "99 HTML5 Export"
@ftable @code
@item --html5-file <string>
HTML5 output file prefix. Can include a path.
@end ftable
%end-doc
*/
  {"html5-file", "HTML5 output file base",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_html5_file 0

/* %start-doc options "99 HTML5 Export"
@ftable @code
@item --all-layers
Output contains all layers, even empty ones.
@end ftable
%end-doc
*/
  {"all-layers", "Output all layers, even empty ones",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_all_layers 1

/* %start-doc options "99 HTML Export"
@ftable @code
@item --verbose
Print file names and counts on stdout.
@end ftable
%end-doc
*/
  {"verbose", "Print file names and aperture counts on stdout",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_verbose 2

/* %start-doc options "99 HTML5 Export"
@ftable @code
@item --metric
generate metric HTML5 and drill files
@end ftable
%end-doc
*/
  {"metric", "Generate metric HTML5 files",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_metric 3

  {"copy-outline", "Copy outline onto other layers",
   HID_Enum, 0, 0, {0, 0, 0}, copy_outline_names, 0},
#define HA_copy_outline 4
};

#define NUM_OPTIONS (sizeof(html5_options)/sizeof(html5_options[0]))


static HID_Attr_Val html5_values[NUM_OPTIONS];


static HID_Attribute *
html5_get_export_options (int *n)
{
  static char *last_made_filename = NULL;
  if (PCB)
  {
    derive_default_filename (PCB->Filename,
      &html5_options[HA_html5_file], "", &last_made_filename);
  }

  if (n)
  {
    *n = NUM_OPTIONS;
  }

  return html5_options;
}


static int
group_for_layer (int l)
{
  if (l < max_copper_layer + 2 && l >= 0)
  {
    return GetLayerGroupNumberByNumber (l);
  }

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
  {
    return d;
  }

  return b - a;
}


static void
maybe_close_f (FILE *f)
{
  if (f)
  {
      pcb_fprintf (f, "\r\n");
      pcb_fprintf (f, "// click on canvas to save to PNG file\r\n");
      pcb_fprintf (f, "\r\n");
      pcb_fprintf (f, "    canvas.onclick = function () {\r\n");
      pcb_fprintf (f, "      window.location = canvas.toDataURL('image/png');\r\n");
      pcb_fprintf (f, "\r\n");
      pcb_fprintf (f, "    };\r\n");
      pcb_fprintf (f, "  </script>\r\n");
      pcb_fprintf (f, "  <noscript>\r\n");
      pcb_fprintf (f, "    <p class=\"error\">\r\n");
      pcb_fprintf (f, "      Your browser has javascript disabled or is not javascript-capable.\r\n");
      pcb_fprintf (f, "    </p>\r\n");
      pcb_fprintf (f, "  <noscript>\r\n");
      pcb_fprintf (f, "  </head>\r\n");

      pcb_fprintf (f, "  <body>\r\n");
      pcb_fprintf (f, "    <canvas id=\"c\" width=\"65535\" height=\"49151\"></canvas>\r\n");
      pcb_fprintf (f, "    <style>canvas {border: 1px solid %s;}</style>\r\n",
        Settings.OffLimitColor);

      pcb_fprintf (f, "  </body>\r\n");
      pcb_fprintf (f, "</html>\r\n");

      fflush (f);
      fclose (f);
  }
}


static BoxType region;


static void
html5_assign_file_suffix (char *dest, int idx)
{
  int fns_style;
  const char *sext = ".html";

  switch (idx)
  {
    case SL (PDRILL, 0):
      sext = ".htm";
      break;
    case SL (UDRILL, 0):
      sext = ".htm";
      break;
  }
  strcpy (dest, layer_type_to_file_name (idx, fns_style));
  strcat (dest, sext);
}


static void
html5_do_export (HID_Attr_Val * options)
{
  const char *fnbase;
  int i;
  static int saved_layer_stack[MAX_LAYER];
  int save_ons[MAX_LAYER + 2];
  FlagType save_thindraw;

  save_thindraw = PCB->Flags;
  CLEAR_FLAG(THINDRAWFLAG, PCB);
  CLEAR_FLAG(THINDRAWPOLYFLAG, PCB);
  CLEAR_FLAG(CHECKPLANESFLAG, PCB);

  if (!options)
  {
    html5_get_export_options (NULL);
    for (i = 0; i < NUM_OPTIONS; i++)
      html5_values[i] = html5_options[i].default_val;

    options = html5_values;
  }

  fnbase = options[HA_html5_file].str_value;

  if (!fnbase)
  {
    fnbase = "pcb-out";
  }

  verbose = options[HA_verbose].int_value;
  metric = options[HA_metric].int_value;

  if (metric)
  {
    x_convspec = "X%.0mu";
    y_convspec = "Y%.0mu";
  }
  else
  {
    x_convspec = "X%.0mc";
    y_convspec = "Y%.0mc";
  }

  all_layers = options[HA_all_layers].int_value;

  copy_outline_mode = options[HA_copy_outline].int_value;

  outline_layer = NULL;

  for (i = 0; i < max_copper_layer; i++)
  {
    LayerType *layer = PCB->Data->Layer + i;
    if (strcmp (layer->Name, "outline") == 0 ||
      strcmp (layer->Name, "route") == 0)
    {
      outline_layer = layer;
    }
  }

  i = strlen (fnbase);
  filename = (char *) realloc (filename, i + 40);
  strcpy (filename, fnbase);
  strcat (filename, ".");
  filesuff = filename + strlen (filename);

  if (all_layers)
  {
    memset (print_group, 1, sizeof (print_group));
    memset (print_layer, 1, sizeof (print_layer));
  }
  else
  {
    memset (print_group, 0, sizeof (print_group));
    memset (print_layer, 0, sizeof (print_layer));
  }

  hid_save_and_show_layer_ons (save_ons);

  for (i = 0; i < max_copper_layer; i++)
  {
    LayerType *layer = PCB->Data->Layer + i;
    if (layer->LineN || layer->TextN || layer->ArcN || layer->PolygonN)
      print_group[GetLayerGroupNumberByNumber (i)] = 1;
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
  qsort (LayerStack, max_copper_layer, sizeof (LayerStack[0]), layer_sort);
  linewidth = -1;
  lastcap = -1;
  lastgroup = -1;

  region.X1 = 0;
  region.Y1 = 0;
  region.X2 = PCB->MaxWidth;
  region.Y2 = PCB->MaxHeight;

  pagecount = 1;

  lastgroup = -1;
  layer_list_idx = 0;
  hid_expose_callback (&html5_hid, &region, 0);

  layer_list_idx = 0;
  hid_expose_callback (&html5_hid, &region, 0);

  memcpy (LayerStack, saved_layer_stack, sizeof (LayerStack));

  maybe_close_f (f);
  f = NULL;
  hid_restore_layer_ons (save_ons);
  PCB->Flags = save_thindraw;
}


static void
html5_parse_arguments (int *argc, char ***argv)
{
  hid_register_attributes (html5_options, NUM_OPTIONS);
  hid_parse_command_line (argc, argv);
}


static int
html5_set_layer (const char *name, int group, int empty)
{
  char *cp;
  int idx = (group >= 0
    && group < max_group) ? PCB->LayerGroups.Entries[group][0] : group;

  if (name == NULL)
  {
    name = PCB->Data->Layer[idx].Name;
  }

  if (idx >= 0 && idx < max_copper_layer && !print_layer[idx])
  {
    return 0;
  }

  if (strcmp (name, "invisible") == 0)
  {
    return 0;
  }

  if (SL_TYPE (idx) == SL_ASSY)
  {
    return 0;
  }

  flash_drills = 0;

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

    lastgroup = group;
    lastX = -1;
    lastY = -1;
    linewidth = -1;
    lastcap = -1;

    maybe_close_f (f);
    f = NULL;

    pagecount++;
    html5_assign_file_suffix (filesuff, idx);
    f = fopen (filename, "wb");   /* Binary needed to force CR-LF */

    if (f == NULL) 
    {
      Message ( "Error:  Could not open %s for writing.\n", filename);
      return 1;
    }

    was_drill = is_drill;

    if (verbose)
    {
      printf ("Now creating HTML5 file : %s\n", filename);
    }

    /* Create a portable timestamp. */
    currenttime = time (NULL);
    {
      /* avoid gcc complaints */
      const char *fmt = "%c UTC";
      strftime (utcTime, sizeof utcTime, fmt, gmtime (&currenttime));
    }
    /* Print a cute file header at the beginning of each file. */
    pcb_fprintf (f, "<!DOCTYPE html>\r\n");
    pcb_fprintf (f, "<!-- PCB name: %s -->\r\n", UNKNOWN (PCB->Name));
    pcb_fprintf (f, "<!-- Layer name: %s -->\r\n", UNKNOWN (name));
    pcb_fprintf (f, "<!-- Generated with: %s " VERSION " -->\r\n", Progname);
    pcb_fprintf (f, "<!-- Creation date: %s -->\r\n", utcTime);

#ifdef HAVE_GETPWUID
    /* ID the user. */
    pwentry = getpwuid (getuid ());
    pcb_fprintf (f, "<!-- Made by: %s -->\r\n", pwentry->pw_name);
#endif

    pcb_fprintf (f, "<!-- Format: HTML5 -->\r\n");
    pcb_fprintf (f, metric ?
      "<!-- PCB Dimensions (mm): %.2mm %.2mm -->\r\n" :
      "<!-- PCB Dimensions (mil): %.2ml %.2ml -->\r\n",
      PCB->MaxWidth, PCB->MaxHeight);
    pcb_fprintf (f, "<!-- PCB max coordinates : %mn,%mn -->\r\n",
      PCB->MaxWidth, PCB->MaxHeight);
    fprintf (f, "<!-- Coordinate origin: lower left -->\r\n");

    /* build a legal identifier. */
    if (layername)
    {
      free (layername);
    }

    layername = strdup (filesuff);
    if (strrchr (layername, '.'))
    {
      * strrchr (layername, '.') = 0;
    }

    for (cp=layername; *cp; cp++)
    {
      if (isalnum((int) *cp))
        *cp = toupper((int) *cp);
      else
        *cp = '_';
    }

    pcb_fprintf (f, "<html>\r\n");
    pcb_fprintf (f, "  <head>\r\n");
    pcb_fprintf (f, "  <meta charset=utf-8 />\r\n");
    pcb_fprintf (f, "  <title>%s</title>\r\n", UNKNOWN (name));
    pcb_fprintf (f, "  <script type=\"text/javascript\">\r\n");
    pcb_fprintf (f, "    window.onload = function()\r\n");
    pcb_fprintf (f, "    {\r\n");
    pcb_fprintf (f, "      var canvas = document.getElementById(\"c\");\r\n");
    pcb_fprintf (f, "      var context = canvas.getContext(\"2d\");\r\n");
    pcb_fprintf (f, "\r\n");

    if (verbose)
    {
      pcb_fprintf (f, "// draw a background\r\n");
    }

    pcb_fprintf (f, "      context.fillStyle = \"%s\";\r\n",
      Settings.BackgroundColor);
    pcb_fprintf (f, "      context.fillRect(0,0,%mn,%mn);\r\n",
      PCB->MaxWidth, PCB->MaxHeight);

    if (verbose)
    {
      pcb_fprintf (f, "\r\n");
      pcb_fprintf (f, "// draw entities\r\n");
      pcb_fprintf (f, "\r\n");
    }

    lncount = 1;
  }

  return 1;
}


static hidGC
html5_make_gc (void)
{
  hidGC rv = (hidGC) calloc (1, sizeof (*rv));
  rv->cap = Trace_Cap;
  return rv;
}


static void
html5_destroy_gc (hidGC gc)
{
  free (gc);
}


static void
html5_use_mask (int use_it)
{
  current_mask = use_it;
}


static void
html5_set_color (hidGC gc, const char *name)
{
  hidval cval;

  if (name == NULL)
  {
    name = Settings.WarnColor;
  }

  if (strcmp (name, "erase") == 0 || strcmp (name, "drill") == 0)
  {
    gc->color = 0x000000;
    gc->erase = 1;
    return;
  }

  gc->erase = 0;

  if (strcmp (name, "#000000") == 0)
  {
    gc->color = 0;
    return;
  }

  if (hid_cache_color (0, name, &cval, &color_cache))
  {
    gc->color = cval.lval;
  }
  else if (name[0] == '#')
  {
    sscanf (name + 1, "%02x%02x%02x", &(gc->r), &(gc->g), &(gc->b));
    gc->color = (gc->r << 16) + (gc->g << 8) + gc->b;

    if (gc->color == BADCOLOR) 
    {
      Message ("%s():  Bad color found, aborting export.\n", __FUNCTION__);
      return;
    }

    cval.ptr = (void *) gc->color; /*! \todo FIXME */
    hid_cache_color (1, name, &cval, &color_cache);
  }
  else
  {
    fprintf (stderr, "ERROR: in %s() WE SHOULD NOT BE HERE!!!", __FUNCTION__);
    gc->color = 0;
  }
}


static void
html5_set_line_cap (hidGC gc, EndCapStyle style)
{
  gc->cap = style;
}


static void
html5_set_line_width (hidGC gc, Coord width)
{
  gc->width = width;
}


static void
html5_set_draw_xor (hidGC gc, int xor_)
{
  /* do nothing. */
}


static void
html5_use_gc (hidGC gc, int radius)
{
  if (radius)
  {
    radius *= 2;

    if (radius != linewidth || lastcap != Round_Cap)
    {
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
        break;
      case Trace_Cap:
        break;
      case Square_Cap:
        break;
      default:
        break;
    }
  }
}


static void
html5_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  if (!f)
  {
    return;
  }

  /* we never draw zero-width lines */
  if (gc->width == 0)
  {
    return;
  }

  if (verbose)
  {
    pcb_fprintf (f, "// html5_draw_rect begin\r\n");
  }

  html5_draw_line (gc, x1, y1, x1, y2);
  html5_draw_line (gc, x1, y1, x2, y1);
  html5_draw_line (gc, x1, y2, x2, y2);
  html5_draw_line (gc, x2, y1, x2, y2);

  if (verbose)
  {
    pcb_fprintf (f, "// html5_draw_rect end\r\n\r\n");
  }

}


static void
html5_draw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  if (!f)
  {
    return;
  }

  /* we never draw zero-width lines. */
  if (gc->width == 0)
  {
    return;
  }

  if (x1 != x2 && y1 != y2 && gc->cap == Square_Cap)
  {
    /* handle lines with a non-zero (discrete) length with square end cap
     * style. */
    Coord x[5];
    Coord y[5];
    double tx;
    double ty;
    double theta;

    if (verbose)
    {
      pcb_fprintf (f, "// html5_draw_line begin (case: non-zero length, square cap)\r\n");
    }

    theta = atan2 (y2-y1, x2-x1);

    /* T is a vector half a thickness long, in the direction of
     * one of the corners. */
    tx = gc->width / 2.0 * cos (theta + M_PI/4) * sqrt(2.0);
    ty = gc->width / 2.0 * sin (theta + M_PI/4) * sqrt(2.0);

    x[0] = x1 - tx;
    y[0] = y1 - ty;
    x[1] = x2 + ty;
    y[1] = y2 - tx;
    x[2] = x2 + tx;
    y[2] = y2 + ty;
    x[3] = x1 - ty;
    y[3] = y1 + tx;
    x[4] = x[0];
    y[4] = y[0];
    html5_fill_polygon (gc, 5, x, y);

    if (verbose)
    {
      pcb_fprintf (f, "// html5_draw_line end (case: non-zero length, suare cap)\r\n\r\n");
    }

    return;
  }
  else if ((x1 == x2) && (y1 == y2))
  {
    /* handle zero length lines. 
     * these may be {round, square, octagonal} pins.
     */
    Coord w;

    if (verbose)
    {
      pcb_fprintf (f, "// html5_draw_line begin (case: zero length)\r\n");
    }

    html5_use_gc (gc, 0);

    w = gc->width / 2;

    if ((gc->cap == Trace_Cap) || (gc->cap == Round_Cap))
    {
      html5_fill_circle (gc, x1, y1, w);
    }
    else if (gc->cap == Square_Cap)
    {
      html5_fill_rect (gc, x1 - w, y1 - w, x1 + w, y1 + w);
    }
    else if (gc->cap == Beveled_Cap)
    {
      /*! \todo Add code here ! */
    }

    if (verbose)
    {
      pcb_fprintf (f, "// html5_draw_line end (case: zero length)\r\n\r\n");
    }

    return;
  }
  else
  {
    /* handle lines with a non-zero (discrete) length. */

    if (verbose)
    {
      pcb_fprintf (f, "// html5_draw_line begin (case: non-zero lenght)\r\n");
    }
    html5_use_gc (gc, 0);

    /* save context state. */
    pcb_fprintf (f, "      context.save();\r\n");

    pcb_fprintf (f, "      context.moveTo(%mn,%mn);\r\n", x1, y1);
    pcb_fprintf (f, "      context.lineTo(%mn,%mn);\r\n", x2, y2);
    pcb_fprintf (f, "      context.lineWidth = %mn;\r\n", gc->width);
    pcb_fprintf (f, "      context.strokeStyle = \"#%06X\";\r\n", gc->color);

    if (gc->cap == Trace_Cap)
    {
      pcb_fprintf (f, "      context.lineCap = \"round\";\r\n");
    }
    else if (gc->cap == Square_Cap)
    {
      pcb_fprintf (f, "      context.lineCap = \"square\";\r\n");
    }

    pcb_fprintf (f, "      context.stroke();\r\n");

    /* restore to original context state. */
    pcb_fprintf (f, "      context.restore();\r\n");

    if (verbose)
    {
      pcb_fprintf (f, "// html5_draw_line end (case: non-zero length)\r\n\r\n");
    }
  }

}


static void
html5_draw_arc (hidGC gc, Coord cx, Coord cy, Coord width, Coord height,
  Angle start_angle, Angle delta_angle)
{
  double arcStartX;
  double arcStopX;
  double arcStartY;
  double arcStopY;

  if (!f)
  {
    return;
  }

  /* we never draw zero-width arclines */
  if (gc->width == 0)
  {
    return;
  }

  html5_use_gc (gc, 0);

  arcStartX = cx - width * cos (TO_RADIANS (start_angle));
  arcStartY = cy + height * sin (TO_RADIANS (start_angle));
  arcStopX = cx - width * cos (TO_RADIANS (start_angle + delta_angle));
  arcStopY = cy + height * sin (TO_RADIANS (start_angle + delta_angle));
  lastX = arcStopX;
  lastY = arcStopY;

  if (width != height)
  {
    /* draw an ellipse made out of line segments. */
    double step;
    double angle;
    int nsteps;
    Coord max;
    Coord minr;
    Coord x0;
    Coord y0;
    Coord x1;
    Coord y1;

    if (verbose)
    {
      pcb_fprintf (f, "// html5_draw_arc begin (case: ellipse)\r\n");
    }

    max = width > height ? width : height;
    minr = max - gc->width / 10;

    if (minr >= max)
    {
      minr = max - 1;
    }

    step = acos((double)minr / (double)max) * 180.0 / M_PI;

    if (step > 5)
    {
      step = 5;
    }

    nsteps = abs(delta_angle) / step + 1;
    step = (double)delta_angle / nsteps;

    x0 = arcStartX;
    y0 = arcStartY;
    angle = start_angle;

    while (nsteps > 0)
    {
      nsteps--;
      x1 = cx - width * cos (TO_RADIANS (angle + step));
      y1 = cy + height * sin (TO_RADIANS (angle + step));
      html5_draw_line (gc, x0, y0, x1, y1);
      x0 = x1;
      y0 = y1;
      angle += step;
    }

    if (verbose)
    {
      pcb_fprintf (f, "// html5_draw_arc end (case: ellipse)\r\n\r\n");
    }

    return;
  }
  else if (width == height)
  {
    /* draw a circle. */
    Angle end_angle;

    if (verbose)
    {
      pcb_fprintf (f, "// html5_draw_arc begin (case: circle)\r\n");
    }

    end_angle = start_angle + delta_angle;

    /* save context state. */
    pcb_fprintf (f, "      context.save();\r\n");

    pcb_fprintf (f, "      context.moveTo(%mn,%mn);\r\n", cx, cy);
    pcb_fprintf (f, "      context.arc(%mn,%mn,%mn,%mn,%mn,%s);\r\n",
      cx, cy, width, start_angle, end_angle, CCW);
    pcb_fprintf (f, "      context.lineWidth = %mn;\r\n", gc->width);
    pcb_fprintf (f, "      context.strokeStyle = \"#%06X\";\r\n", gc->color);

    if (gc->cap == Trace_Cap)
    {
      pcb_fprintf (f, "      context.lineCap = \"round\";\r\n");
    }
    else if (gc->cap == Square_Cap)
    {
      pcb_fprintf (f, "      context.lineCap = \"square\";\r\n");
    }

    pcb_fprintf (f, "      context.stroke();\r\n");

    /* restore to original context state. */
    pcb_fprintf (f, "      context.restore();\r\n");

    if (verbose)
    {
      pcb_fprintf (f, "// html5_draw_arc end (case: circle)\r\n\r\n");
    }

  }
  else
  {
    fprintf (stderr, "ERROR: in %s() WE SHOULD NOT BE HERE!!!",
      __FUNCTION__);
  }
}


static void
html5_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius)
{
  if (!f)
  {
    return;
  }

  if (radius <= 0)
  {
    return;
  }

  if (is_drill)
  {
    radius = 50 * round (radius / 50.0);
  }

  html5_use_gc (gc, radius);

  if (verbose)
  {
    pcb_fprintf (f, "// html5_fill_circle begin\r\n");
  }

  /* save context state. */
  pcb_fprintf (f, "      context.save();\r\n");

  pcb_fprintf (f, "      context.beginPath();\r\n");
  pcb_fprintf (f, "      context.arc(%mn,%mn,%mn,0,2*Math.PI,true);\r\n", cx, cy, radius);
  pcb_fprintf (f, "      context.closePath();\r\n");
  pcb_fprintf (f, "      context.fillStyle=\"#%06X\";\r\n", gc->color);
  pcb_fprintf (f, "      context.fill();\r\n");

  /* restore to original context state. */
  pcb_fprintf (f, "      context.restore();\r\n");

  if (verbose)
  {
    pcb_fprintf (f, "// html5_fill_circle end\r\n\r\n");
  }

}


static void
html5_fill_polygon (hidGC gc, int n_coords, Coord *x, Coord *y)
{
  int i;

  if (!f)
  {
    return;
  }

  if (is_mask && current_mask == HID_MASK_BEFORE)
  {
    return;
  }

  html5_use_gc (gc, 10 * 100);

  if (verbose)
  {
    pcb_fprintf (f, "// html5_fill_polygon begin\r\n");
  }

  pcb_fprintf (f, "      context.save();\r\n");
  pcb_fprintf (f, "      context.beginPath ();\r\n");
  pcb_fprintf (f, "      context.moveTo(%mn,%mn);\r\n", x[0], y[0]);

  for (i = 1; i < n_coords; i++)
  {
    pcb_fprintf (f, "      context.lineTo(%mn,%mn);\r\n", x[i], y[i]);
  }

  pcb_fprintf (f, "      context.closePath();\r\n");
  pcb_fprintf (f, "      context.fillStyle=\"#%06X\";\r\n", gc->color);
  pcb_fprintf (f, "      context.fill();\r\n");

  /* restore to original context state. */
  pcb_fprintf (f, "      context.restore();\r\n");

  if (verbose)
  {
    fprintf (f, "// html5_fill_polygon end\r\n\r\n");
  }

}


static void
html5_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  if (!f)
  {
    return;
  }

  if (verbose)
  {
    pcb_fprintf (f, "// html5_fill_rect begin\r\n");
  }

  /* save context state. */
  pcb_fprintf (f, "      context.save();\r\n");

  pcb_fprintf (f, "      context.rect(%mn,%mn,%mn,%mn);\r\n", x1, y1, x2, y2);
  pcb_fprintf (f, "      context.fillStyle=\"#%06X\";\r\n", gc->color);
  pcb_fprintf (f, "      context.fill();\r\n");

  /* restore context to original state. */
  pcb_fprintf (f, "      context.restore();\r\n");

  if (verbose)
  {
    pcb_fprintf (f, "// html5_fill_rect end\r\n\r\n");
  }
}


static void
html5_calibrate (double xval, double yval)
{
  CRASH;
}


static void
html5_set_crosshair (int x, int y, int action)
{
}


void
hid_html5_init ()
{
  memset (&html5_hid, 0, sizeof (html5_hid));
  memset (&html5_graphics, 0, sizeof (html5_graphics));

  common_nogui_init (&html5_hid);
  common_draw_helpers_init (&html5_hid);

  html5_hid.struct_size         = sizeof (html5_hid);
  html5_hid.name                = "HTML5";
  html5_hid.description         = "HTML5 Exporter";
  html5_hid.exporter            = 1;

  html5_hid.get_export_options  = html5_get_export_options;
  html5_hid.do_export           = html5_do_export;
  html5_hid.parse_arguments     = html5_parse_arguments;
  html5_hid.set_layer           = html5_set_layer;
  html5_hid.calibrate           = html5_calibrate;
  html5_hid.set_crosshair       = html5_set_crosshair;

  html5_hid.graphics            = &html5_graphics;

  html5_graphics.make_gc        = html5_make_gc;
  html5_graphics.destroy_gc     = html5_destroy_gc;
  html5_graphics.use_mask       = html5_use_mask;
  html5_graphics.set_color      = html5_set_color;
  html5_graphics.set_line_cap   = html5_set_line_cap;
  html5_graphics.set_line_width = html5_set_line_width;
  html5_graphics.set_draw_xor   = html5_set_draw_xor;
  html5_graphics.draw_line      = html5_draw_line;
  html5_graphics.draw_arc       = html5_draw_arc;
  html5_graphics.draw_rect      = html5_draw_rect;
  html5_graphics.fill_circle    = html5_fill_circle;
  html5_graphics.fill_polygon   = html5_fill_polygon;
  html5_graphics.fill_rect      = html5_fill_rect;

  hid_register_hid (&html5_hid);
}
