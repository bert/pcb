/*!
 * \file src/hid/gcode/gcode.c
 *
 * \brief GCODE export HID.
 *
 * This HID exports a PCB layout into:
 * - one layer mask file (PNG format) per copper layer,
 * - one G-CODE CNC drill file per drill size
 * - one G-CODE CNC file per copper layer.
 *
 * The latter is used by a CNC milling machine to mill the pcb.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (c) 2010 Alberto Maccioni
 *
 * Copyright (c) 2012 Markus Hitter (mah@jump-ing.de)
 *
 * This code is based on the NELMA export HID, the PNG export HID,
 * and potrace, a tracing program by Peter Selinger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <time.h>

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include "global.h"
#include "error.h" /* Message() */
#include "data.h"
#include "misc.h"
#include "rats.h"

#include "hid.h"
#include "hid_draw.h"
#include "../hidint.h"
#include <gd.h>
#include "hid/common/hidnogui.h"
#include "hid/common/draw_helpers.h"
#include "bitmap.h"
#include "curve.h"
#include "potracelib.h"
#include "trace.h"
#include "decompose.h"
#include "pcb-printf.h"

#include "hid/common/hidinit.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented GCODE function %s.\n", __FUNCTION__); abort()

static HID gcode_hid;
static HID_DRAW gcode_graphics;

struct color_struct
{
  int c; /*!< The descriptor used by the gd library. */

  /* so I can figure out what rgb value c refers to */
  unsigned int r, g, b;
};

struct hid_gc_struct
{
  HID *me_pointer;
  EndCapStyle cap;
  int width;
  unsigned char r, g, b;
  int erase;
  struct color_struct *color;
  gdImagePtr brush;
};

static struct color_struct *black = NULL, *white = NULL;
static int linewidth = -1;
static gdImagePtr lastbrush = (gdImagePtr)((void *) -1);

static gdImagePtr gcode_im = NULL; /*!< gd image and file for PNG export */
static FILE *gcode_f = NULL;

static int is_mask;
static int is_drill;
static int is_bottom;

static int gcode_export_group[MAX_GROUP];
        /*!< Which groups of layers to export into PNG layer masks.\n
         * 1 means export, 0 means do not export. */

static int gcode_cur_group; /*!< Group that is currently exported. */

static const char *gcode_basename = NULL;
        /*!< Filename prefix and suffix that will be used when saving files. */

static int gcode_dpi = -1; /*!< Horizontal DPI (grid points per inch). */

static double gcode_cutdepth = 0;       /*!< Milling depth (inch). */
static double gcode_isoplunge = 0;      /*!< Isolation milling plunge feedrate. */
static double gcode_isofeedrate = 0;    /*!< Isolation milling feedrate. */
static char gcode_predrill;
static double gcode_drilldepth = 0;     /*!< Drilling depth (mm or in). */
static double gcode_drillfeedrate = 0;  /*!< Drilling feedrate. */
static double gcode_safeZ = 100;        /*!< Safe Z (mm or in). */
static int gcode_toolradius = 0;        /*!< Iso-mill tool radius (1/100 mil). */
static char gcode_drillmill = 0;        /*!< Wether to drill with the mill tool. */
static double gcode_milldepth = 0;      /*!< Outline milling depth (mm or in). */
static double gcode_milltoolradius = 0; /*!< Outline-mill tool radius (mm or in). */
static double gcode_millplunge = 0;     /*!< Outline-milling plunge feedrate. */
static double gcode_millfeedrate = 0;   /*!< Outline-milling feedrate. */
static char gcode_advanced = 0;
static int save_drill = 0;

/*!
 * \brief Structure to represent a single hole.
 */
struct drill_hole
{
  double x;
  double y;
};

/*!
 * \brief Structure to represent all holes of a given size.
 */
struct single_size_drills
{
  double diameter_inches;

  int n_holes;
  int n_holes_allocated;
  struct drill_hole* holes;
};

static struct single_size_drills* drills             = NULL;
        /*!< At the start we have no drills at all */
static int                        n_drills           = 0;
static int                        n_drills_allocated = 0;

HID_Attribute gcode_attribute_list[] = {
  /* other HIDs expect this to be first.  */
  {"basename", "File name prefix and suffix,\n"
               "layer names will be inserted before the suffix.",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_basename 0

  {"measurement-unit", "Measurement unit used in the G-code output.",
   HID_Unit, 0, 0, {3, 0, 0}, NULL, 0},
#define HA_unit 1

  {"dpi", "Accuracy of the mill path generation in pixels/inch.",
   HID_Integer, 0, 2000, {600, 0, 0}, 0, 0},
#define HA_dpi 2

  {"safe-Z", "Safe Z for traverse movements of all operations.",
   HID_Real, -1000, 10000, {0, 0, 2}, 0, 0},
#define HA_safeZ 3

  {"iso-mill-depth", "Isolation milling depth.",
   HID_Real, -1000, 1000, {0, 0, -0.05}, 0, 0},
#define HA_cutdepth 4

  {"iso-tool-diameter", "Isolation milling tool diameter.",
   HID_Real, 0, 10000, {0, 0, 0.2}, 0, 0},
#define HA_tooldiameter 5

  {"iso-tool-plunge", "Isolation milling feedrate when plunging into\n"
                      "the material.",
   HID_Real, 0.1, 10000, {0, 0, 25.}, 0, 0},
#define HA_isoplunge 6

  {"iso-tool-feedrate", "Isolation milling feedrate.",
   HID_Real, 0.1, 10000, {0, 0, 50.}, 0, 0},
#define HA_isofeedrate 7

  {"predrill", "Wether to pre-drill all drill spots with the isolation milling\n"
               "tool. Drill depth is iso-mill-depth here. This feature eases\n"
               "and enhances accuracy of manual drilling.",
   HID_Boolean, 0, 0, {1, 0, 0}, 0, 0},
#define HA_predrill 8

  {"drill-depth", "Drilling depth.",
   HID_Real, -10000, 10000, {0, 0, -2}, 0, 0},
#define HA_drilldepth 9

  {"drill-feedrate", "Drilling feedrate.",
   HID_Real, 0.1, 10000, {0, 0, 50.}, 0, 0},
#define HA_drillfeedrate 10

  {"drill-mill", "Wether to produce drill holes equal or bigger than the\n"
                 "milling tool diameter with the milling tool.\n"
                 "With the milling tool bigger holes can be accurately sized\n"
                 "without changing the tool",
   HID_Boolean, 0, 0, {1, 0, 0}, 0, 0},
#define HA_drillmill 11

  {"outline-mill-depth", "Milling depth when milling the outline.\n"
                         "Currently, only the rectangular extents of the\n"
                         "board are milled, no polygonal outlines or holes.",
   HID_Real, -10000, 10000, {0, 0, -1}, 0, 0},
#define HA_milldepth 12

  {"outline-tool-diameter", "Diameter of the tool used for outline milling.",
   HID_Real, 0, 10000, {0, 0, 1}, 0, 0},
#define HA_milltooldiameter 13

  {"outline-mill-plunge", "Outline milling feedrate when plunging into\n"
                          "the material",
   HID_Real, 0.1, 10000, {0, 0, 25.}, 0, 0},
#define HA_millplunge 14

  {"outline-mill-feedrate", "Outline milling feedrate",
   HID_Real, 0.1, 10000, {0, 0, 50.}, 0, 0},
#define HA_millfeedrate 15

  {"advanced-gcode", "Wether to produce G-code for advanced interpreters,\n"
                     "like using variables or drill cycles. Not all\n"
                     "machine controllers understand this, but it allows\n"
                     "better hand-editing of the resulting files.",
   HID_Boolean, 0, 0, {-1, 0, 0}, 0, 0},
#define HA_advanced 16
};

#define NUM_OPTIONS (sizeof(gcode_attribute_list)/sizeof(gcode_attribute_list[0]))

REGISTER_ATTRIBUTES (gcode_attribute_list)
     static HID_Attr_Val gcode_values[NUM_OPTIONS];

/* *** Utility funcions **************************************************** */

/*!
 * \brief Convert from default PCB units to gcode units.
 */
static int pcb_to_gcode (int pcb)
{
  return round(COORD_TO_INCH(pcb) * gcode_dpi);
}

/*!
 * \brief Fits the given layer name into basename, just before the
 * suffix.
 */
static void
gcode_get_filename (char *filename, const char *layername)
{
  char *pt;
  char suffix[MAXPATHLEN];

  suffix[0] = '\0';
  pt = strrchr (gcode_basename, '.');
  if (pt && pt > strrchr (gcode_basename, '/'))
    strcpy (suffix, pt);
  else
    pt = NULL;

  strcpy (filename, gcode_basename);
  if (pt)
    *(filename + (pt - gcode_basename)) = '\0';
  strcat (filename, "-");
  strcat (filename, layername);
  strcat (filename, suffix);

  // result is in char *filename
}

/*!
 * \brief Sorts drills to produce a short tool path.
 *
 * I start with the hole nearest (0,0) and for each subsequent one, find
 * the hole nearest to the previous.
 *
 * This isn't guaranteed to find the shortest path, but should be good
 * enough.
 *
 * \note This is O(N^2).
 * We can't use the O(N logN) sort, since our shortest-distance origin
 * changes with every point.
 */
static void
sort_drill (struct drill_hole *drill, int n_drill)
{
  /* I start out by looking for points closest to (0,0) */
  struct drill_hole nearest_target = { 0, 0 };

  /* I sort my list by finding the correct point to fill each slot. I don't need
     to look at the last one, since it'll be in the right place automatically */
  for (int j = 0; j < n_drill-1; j++)
    {
      double dmin = 1e20;
      int    imin = 0;
      /* look through remaining elements to find the next drill point. This is
         the one nearest to nearest_target */
      for (int i = j; i < n_drill; i++)
        {
          double d =
            (drill[i].x - nearest_target.x) * (drill[i].x - nearest_target.x) +
            (drill[i].y - nearest_target.y) * (drill[i].y - nearest_target.y);
          if (d < dmin)
            {
              imin = i;
              dmin = d;
            }
        }
      /* printf("j=%d imin=%d dmin=%f nearest_target=(%f,%f)\n",j,imin,dmin,
                nearest_target.x,nearest_target.y); */
      if (j != imin)
        {
          struct drill_hole tmp;
          tmp         = drill[j];
          drill[j]    = drill[imin];
          drill[imin] = tmp;
        }

      nearest_target = drill[j];
    }
}

/* *** Main export callback ************************************************ */

static void
gcode_parse_arguments (int *argc, char ***argv)
{
  hid_register_attributes (gcode_attribute_list,
                           sizeof (gcode_attribute_list) /
                           sizeof (gcode_attribute_list[0]));
  hid_parse_command_line (argc, argv);
}

static HID_Attribute *
gcode_get_export_options (int *n)
{
  static char *last_made_filename = 0;
  static int last_unit_value = -1;

  if (gcode_attribute_list[HA_unit].default_val.int_value == last_unit_value)
    {
      if (Settings.grid_unit)
        gcode_attribute_list[HA_unit].default_val.int_value = Settings.grid_unit->index;
      else
        gcode_attribute_list[HA_unit].default_val.int_value = get_unit_struct ("mil")->index;
      last_unit_value = gcode_attribute_list[HA_unit].default_val.int_value;
    }

  if (PCB)
    {
      derive_default_filename (PCB->Filename,
                               &gcode_attribute_list[HA_basename],
                               ".gcode", &last_made_filename);
    }
  if (n)
    {
      *n = NUM_OPTIONS;
    }
  return gcode_attribute_list;
}

/*!
 * \brief Populates gcode_export_group array.
 */
static void
gcode_choose_groups ()
{
  int n, m;
  LayerType *layer;

  /* Set entire array to 0 (don't export any layer groups by default */
  memset (gcode_export_group, 0, sizeof (gcode_export_group));

  for (n = 0; n < max_copper_layer; n++)
    {
      layer = &PCB->Data->Layer[n];

      if (layer->LineN || layer->TextN || layer->ArcN || layer->PolygonN)
        {
          /* layer isn't empty */

          /*
           * is this check necessary? It seems that special
           * layers have negative indexes?
           */

          if (SL_TYPE (n) == 0)
            {
              /* layer is a copper layer */
              m = GetLayerGroupNumberByNumber (n);

              /* the export layer */
              gcode_export_group[m] = 1;
            }
        }
    }
}

/*!
 * \brief Allocate white and black.
 *
 * The first color allocated becomes the background color.
 */
static void
gcode_alloc_colors ()
{
  white = (struct color_struct *) malloc (sizeof (*white));
  white->r = white->g = white->b = 255;
  white->c = gdImageColorAllocate (gcode_im, white->r, white->g, white->b);

  black = (struct color_struct *) malloc (sizeof (*black));
  black->r = black->g = black->b = 0;
  black->c = gdImageColorAllocate (gcode_im, black->r, black->g, black->b);
}

static void
gcode_start_png ()
{
  gcode_im = gdImageCreate (pcb_to_gcode (PCB->MaxWidth),
                            pcb_to_gcode (PCB->MaxHeight));
  gcode_alloc_colors ();
}

static void
gcode_finish_png (const char *layername)
{
#ifdef HAVE_GDIMAGEPNG
  char *pngname, *filename;
  FILE *file = NULL;

  pngname = (char *)malloc (MAXPATHLEN);
  gcode_get_filename (pngname, layername);
  filename = g_strdup_printf ("%s.png", pngname);
  free(pngname);

  file = fopen (filename, "wb");
  g_free (filename);

  gdImagePng (gcode_im, file);
  fclose (file);
#else
  Message ("GCODE: PNG not supported by gd. Can't write layer mask.\n");
#endif
  gdImageDestroy (gcode_im);

  free (white);
  free (black);
  gcode_im = NULL;
}

static void
gcode_start_png_export ()
{
  BoxType region;

  region.X1 = 0;
  region.Y1 = 0;
  region.X2 = PCB->MaxWidth;
  region.Y2 = PCB->MaxHeight;

  linewidth = -1;
  lastbrush = (gdImagePtr)((void *) -1);

  hid_expose_callback (&gcode_hid, &region, 0);
}

static FILE *
gcode_start_gcode (const char *layername, bool metric)
{
  FILE *file = NULL;
  char buffer[MAXPATHLEN];
  time_t t;

  gcode_get_filename (buffer, layername);
  file = fopen (buffer, "wb");
  if ( ! file)
    {
      perror (buffer);
      Message ("Can't open file %s\n", buffer);
      return NULL;
    }
  fprintf (file, "(Created by G-code exporter)\n");
  t = time (NULL);
  snprintf (buffer, sizeof(buffer), "%s", ctime (&t));
  buffer[strlen (buffer) - 1] = '\0'; // drop the newline
  fprintf (file, "(%s)\n", buffer);
  fprintf (file, "(Units: %s)\n", metric ? "mm" : "inch");
  if (metric)
    pcb_fprintf (file, "(Board size: %.2`mm x %.2`mm mm)\n",
                 PCB->MaxWidth, PCB->MaxHeight);
  else
    pcb_fprintf (file, "(Board size: %.2`mi x %.2`mi inches)\n",
                 PCB->MaxWidth, PCB->MaxHeight);

  return file;
}

static void
gcode_do_export (HID_Attr_Val * options)
{
  int save_ons[MAX_ALL_LAYER];
  int i, idx;
  const Unit *unit;
  double scale = 0, d = 0;
  int r, c, v, p, metric;
  path_t *plist = NULL;
  potrace_bitmap_t *bm = NULL;
  potrace_param_t param_default = {
    2,                           /* turnsize */
    POTRACE_TURNPOLICY_MINORITY, /* turnpolicy */
    1.0,                         /* alphamax */
    1,                           /* opticurve */
    0.2,                         /* opttolerance */
    {
     NULL,                       /* callback function */
     NULL,                       /* callback data */
     0.0, 1.0,                   /* progress range  */
     0.0,                        /* granularity  */
     },
  };
  char variable_safeZ[20], variable_cutdepth[20];
  char variable_isoplunge[20], variable_isofeedrate[20];
  char variable_drilldepth[20], variable_milldepth[20];
  char variable_millplunge[20], variable_millfeedrate[20];

  if (!options)
    {
      gcode_get_export_options (0);
      for (i = 0; i < NUM_OPTIONS; i++)
        {
          gcode_values[i] = gcode_attribute_list[i].default_val;
        }
      options = gcode_values;
    }
  gcode_basename = options[HA_basename].str_value;
  if (!gcode_basename)
    {
      gcode_basename = "pcb-out.gcode";
    }
  gcode_dpi = options[HA_dpi].int_value;
  if (gcode_dpi < 0)
    {
      fprintf (stderr, "ERROR:  dpi may not be < 0\n");
      return;
    }
  unit = &(get_unit_list() [options[HA_unit].int_value]);
  metric = (unit->family == METRIC);
  scale = metric ? 1.0 / coord_to_unit (unit, MM_TO_COORD (1.0))
                 : 1.0 / coord_to_unit (unit, INCH_TO_COORD (1.0));

  gcode_cutdepth = options[HA_cutdepth].real_value * scale;
  gcode_isoplunge = options[HA_isoplunge].real_value * scale;
  gcode_isofeedrate = options[HA_isofeedrate].real_value * scale;
  gcode_predrill = options[HA_predrill].int_value;
  gcode_drilldepth = options[HA_drilldepth].real_value * scale;
  gcode_drillfeedrate = options[HA_drillfeedrate].real_value * scale;
  gcode_safeZ = options[HA_safeZ].real_value * scale;
  gcode_toolradius = metric
                   ? MM_TO_COORD(options[HA_tooldiameter].real_value / 2 * scale)
                   : INCH_TO_COORD(options[HA_tooldiameter].real_value / 2 * scale);
  gcode_drillmill = options[HA_drillmill].int_value;
  gcode_milldepth = options[HA_milldepth].real_value * scale;
  gcode_milltoolradius = options[HA_milltooldiameter].real_value / 2 * scale;
  gcode_millplunge = options[HA_millplunge].real_value * scale;
  gcode_millfeedrate = options[HA_millfeedrate].real_value * scale;
  gcode_advanced = options[HA_advanced].int_value;
  gcode_choose_groups ();
  if (gcode_advanced)
    {
      /* give each variable distinct names, even if they don't appear
         together in a file. This allows to join output files */
      strcpy (variable_safeZ, "#100");
      strcpy (variable_cutdepth, "#101");
      strcpy (variable_isoplunge, "#102");
      strcpy (variable_isofeedrate, "#103");
      strcpy (variable_drilldepth, "#104");
      strcpy (variable_milldepth, "#105");
      strcpy (variable_millplunge, "#106");
      strcpy (variable_millfeedrate, "#107");
    }
  else
    {
      snprintf (variable_safeZ, 20, "%f", gcode_safeZ);
      snprintf (variable_cutdepth, 20, "%f", gcode_cutdepth);
      snprintf (variable_isoplunge, 20, "%f", gcode_isoplunge);
      snprintf (variable_isofeedrate, 20, "%f", gcode_isofeedrate);
      snprintf (variable_drilldepth, 20, "%f", gcode_drilldepth);
      snprintf (variable_milldepth, 20, "%f", gcode_milldepth);
      snprintf (variable_millplunge, 20, "%f", gcode_millplunge);
      snprintf (variable_millfeedrate, 20, "%f", gcode_millfeedrate);
    }

  for (i = 0; i < MAX_GROUP; i++)
    {
      if (gcode_export_group[i])
        {
          gcode_cur_group = i;

          /* magic */
          idx = (i >= 0 && i < max_group) ?
            PCB->LayerGroups.Entries[i][0] : i;
          is_bottom =
            (GetLayerGroupNumberByNumber (idx) ==
             GetLayerGroupNumberBySide (BOTTOM_SIDE)) ? 1 : 0;
          save_drill = is_bottom; /* save drills for one layer only */
          gcode_start_png ();
          hid_save_and_show_layer_ons (save_ons);
          gcode_start_png_export ();
          hid_restore_layer_ons (save_ons);

/* ***************** gcode conversion *************************** */
/* potrace uses a different kind of bitmap; for simplicity gcode_im is
   copied to this format and flipped as needed along the way */
          bm = bm_new (gdImageSX (gcode_im), gdImageSY (gcode_im));
          for (r = 0; r < gdImageSX (gcode_im); r++)
            {
              for (c = 0; c < gdImageSY (gcode_im); c++)
                {
                  if (is_bottom)
                    v =  /* flip vertically and horizontally */
                      gdImageGetPixel (gcode_im, gdImageSX (gcode_im) - 1 - r,
                                       gdImageSY (gcode_im) - 1 - c);
                  else
                    v =  /* flip only vertically */
                      gdImageGetPixel (gcode_im, r,
                                       gdImageSY (gcode_im) - 1 - c);
                  p = (gcode_im->red[v] || gcode_im->green[v]
                       || gcode_im->blue[v]) ? 0 : 0xFFFFFF;
                  BM_PUT (bm, r, c, p);
                }
            }
          if (is_bottom)
            { /* flip back layer, used only for PNG output */
              gdImagePtr temp_im =
                gdImageCreate (gdImageSX (gcode_im), gdImageSY (gcode_im));
              gdImageColorAllocate (temp_im, white->r, white->g, white->b);
              gdImageColorAllocate (temp_im, black->r, black->g, black->b);
              gdImageCopy (temp_im, gcode_im, 0, 0, 0, 0,
                           gdImageSX (gcode_im), gdImageSY (gcode_im));
              for (r = 0; r < gdImageSX (gcode_im); r++)
                {
                  for (c = 0; c < gdImageSY (gcode_im); c++)
                    {
                      gdImageSetPixel (gcode_im, r, c,
                                       gdImageGetPixel (temp_im,
                                                        gdImageSX (gcode_im) -
                                                        1 - r, c));
                    }
                }
              gdImageDestroy (temp_im);
            }
          gcode_finish_png (layer_type_to_file_name (idx, FNS_fixed));
          plist = NULL;
          gcode_f = gcode_start_gcode (layer_type_to_file_name (idx, FNS_fixed),
                                       metric);
          if (!gcode_f)
            {
              bm_free (bm);
              return;
            }
          fprintf (gcode_f, "(Accuracy %d dpi)\n", gcode_dpi);
          pcb_fprintf (gcode_f, "(Tool diameter: %`f %s)\n",
                       options[HA_tooldiameter].real_value * scale,
                       metric ? "mm" : "inch");
          if (gcode_advanced)
            {
              pcb_fprintf (gcode_f, "%s=%`f  (safe Z)\n",
                           variable_safeZ, gcode_safeZ);
              pcb_fprintf (gcode_f, "%s=%`f  (cutting depth)\n",
                           variable_cutdepth, gcode_cutdepth);
              pcb_fprintf (gcode_f, "%s=%`f  (plunge feedrate)\n",
                           variable_isoplunge, gcode_isoplunge);
              pcb_fprintf (gcode_f, "%s=%`f  (feedrate)\n",
                           variable_isofeedrate, gcode_isofeedrate);
            }
          if (gcode_predrill && save_drill)
            fprintf (gcode_f, "(with predrilling)\n");
          else
            fprintf (gcode_f, "(no predrilling)\n");
          fprintf (gcode_f, "(---------------------------------)\n");
          if (gcode_advanced)
              fprintf (gcode_f, "G17 G%d G90 G64 P0.003 M3 S3000 M7\n",
                       metric ? 21 : 20);
          else
              fprintf (gcode_f, "G17\nG%d\nG90\nG64 P0.003\nM3 S3000\nM7\n",
                       metric ? 21 : 20);
          fprintf (gcode_f, "G0 Z%s\n", variable_safeZ);
          /* extract contour points from image */
          r = bm_to_pathlist (bm, &plist, &param_default);
          if (r)
            {
              fprintf (stderr, "ERROR: pathlist function failed\n");
              return;
            }
          /* generate best polygon and write vertices in g-code format */
          d = process_path (plist, &param_default, bm, gcode_f,
                            metric ? 25.4 / gcode_dpi : 1.0 / gcode_dpi,
                            variable_cutdepth, variable_safeZ,
                            variable_isoplunge, variable_isofeedrate);
          if (d < 0)
            {
              fprintf (stderr, "ERROR: path process function failed\n");
              return;
            }
          if (gcode_predrill && save_drill)
            {
              int n_all_drills = 0;
              struct drill_hole* all_drills = NULL;
              /* count all drills to be predrilled */
              for (int i_drill_sets = 0; i_drill_sets < n_drills; i_drill_sets++)
                {
                  struct single_size_drills* drill_set = &drills[i_drill_sets];

                  /* don't predrill drillmill holes */
                  if (gcode_drillmill) {
                    double radius = metric ?
                                    drill_set->diameter_inches * 25.4 / 2:
                                    drill_set->diameter_inches / 2;

                    if (gcode_milltoolradius < radius)
                      continue;
                  }

                  n_all_drills += drill_set->n_holes;
                }
              /* for sorting regardless of size, copy all drills to be
                 predrilled into one new structure */
              all_drills = (struct drill_hole *)
                           malloc (n_all_drills * sizeof (struct drill_hole));
              for (int i_drill_sets = 0; i_drill_sets < n_drills; i_drill_sets++)
                {
                  struct single_size_drills* drill_set = &drills[i_drill_sets];

                  /* don't predrill drillmill holes */
                  if (gcode_drillmill) {
                    double radius = metric ?
                                    drill_set->diameter_inches * 25.4 / 2:
                                    drill_set->diameter_inches / 2;

                    if (gcode_milltoolradius < radius)
                      continue;
                  }

                  memcpy(&all_drills[r], drill_set->holes,
                         drill_set->n_holes * sizeof(struct drill_hole));
                }
              sort_drill(all_drills, n_all_drills);
              /* write that (almost the same code as writing the drill file) */
              fprintf (gcode_f, "(predrilling)\n");
              fprintf (gcode_f, "F%s\n", variable_isoplunge);

              for (r = 0; r < n_all_drills; r++)
                {
                  double drillX, drillY;

                  if (metric)
                    {
                      drillX = all_drills[r].x * 25.4;
                      drillY = all_drills[r].y * 25.4;
                    }
                  else
                    {
                      drillX = all_drills[r].x;
                      drillY = all_drills[r].y;
                    }
                  if (gcode_advanced)
                    pcb_fprintf (gcode_f, "G81 X%`f Y%`f Z%s R%s\n",
                                 drillX, drillY, variable_cutdepth,
                                 variable_safeZ);
                  else
                    {
                      pcb_fprintf (gcode_f, "G0 X%`f Y%`f\n", drillX, drillY);
                      pcb_fprintf (gcode_f, "G1 Z%s\n", variable_cutdepth);
                      pcb_fprintf (gcode_f, "G0 Z%s\n", variable_safeZ);
                    }
                }
              fprintf (gcode_f, "(%d predrills)\n", n_all_drills);
              free(all_drills);
            }
          if (metric)
            pcb_fprintf (gcode_f, "(milling distance %`.2fmm = %`.2fin)\n", d,
                         d * 1 / 25.4);
          else
            pcb_fprintf (gcode_f, "(milling distance %`.2fmm = %`.2fin)\n",
                         25.4 * d, d);
          if (gcode_advanced)
            fprintf (gcode_f, "M5 M9 M2\n");
          else
            fprintf (gcode_f, "M5\nM9\nM2\n");
          pathlist_free (plist);
          bm_free (bm);
          fclose (gcode_f);
          gcode_f = NULL;
          if (save_drill)
            {
              for (int i_drill_file=0; i_drill_file < n_drills; i_drill_file++)
                {
                  struct single_size_drills* drill = &drills[i_drill_file];

                  /* don't drill drillmill holes */
                  if (gcode_drillmill) {
                    double radius = metric ?
                                    drill->diameter_inches * 25.4 / 2:
                                    drill->diameter_inches / 2;

                    if (gcode_milltoolradius < radius)
                      continue;
                  }

                  d = 0;
                  sort_drill (drill->holes, drill->n_holes);

                  {
                    // get the filename with the drill size encoded in it
                    char layername[32];
                    pcb_snprintf(layername, sizeof(layername),
                                 "%`.4f.drill",
                                 metric ?
                                 drill->diameter_inches * 25.4 :
                                 drill->diameter_inches);
                    gcode_f = gcode_start_gcode(layername, metric);
                  }
                  if (!gcode_f)
                    return;
                  fprintf (gcode_f, "(Drill file: %d drills)\n", drill->n_holes);
                  if (metric)
                    pcb_fprintf (gcode_f, "(Drill diameter: %`f mm)\n",
                                 drill->diameter_inches * 25.4);
                  else
                    pcb_fprintf (gcode_f, "(Drill diameter: %`f inch)\n",
                                 drill->diameter_inches);
                  if (gcode_advanced)
                    {
                      pcb_fprintf (gcode_f, "%s=%`f  (safe Z)\n",
                                   variable_safeZ, gcode_safeZ);
                      pcb_fprintf (gcode_f, "%s=%`f  (drill depth)\n",
                                   variable_drilldepth, gcode_drilldepth);
                      fprintf (gcode_f, "(---------------------------------)\n");
                      fprintf (gcode_f, "G17 G%d G90 G64 P0.003 M3 ",
                                     metric ? 21 : 20);
                      pcb_fprintf (gcode_f, "S3000 M7 F%`f\n",
                                   gcode_drillfeedrate);
                    }
                  else
                    {
                      fprintf (gcode_f, "(---------------------------------)\n");
                      fprintf (gcode_f, "G17\nG%d\nG90\nG64 P0.003\nM3 ",
                               metric ? 21 : 20);
                      pcb_fprintf (gcode_f, "S3000\nM7\nF%`f\n",
                                   gcode_drillfeedrate);
                    }
                  fprintf (gcode_f, "G0 Z%s\n", variable_safeZ);
                  for (r = 0; r < drill->n_holes; r++)
                    {
                      double drillX, drillY;

                      if (metric)
                        {
                          drillX = drill->holes[r].x * 25.4;
                          drillY = drill->holes[r].y * 25.4;
                        }
                      else
                        {
                          drillX = drill->holes[r].x;
                          drillY = drill->holes[r].y;
                        }
                      if (gcode_advanced)
                        pcb_fprintf (gcode_f, "G81 X%`f Y%`f Z%s R%s\n",
                                     drillX, drillY, variable_drilldepth,
                                     variable_safeZ);
                      else
                        {
                          pcb_fprintf (gcode_f, "G0 X%`f Y%`f\n",
                                       drillX, drillY);
                          fprintf (gcode_f, "G1 Z%s\n", variable_drilldepth);
                          fprintf (gcode_f, "G0 Z%s\n", variable_safeZ);
                        }
                      if (r > 0)
                        d += Distance(drill->holes[r - 1].x, drill->holes[r - 1].y,
                                      drill->holes[r    ].x, drill->holes[r    ].y);
                    }
                  if (gcode_advanced)
                    fprintf (gcode_f, "M5 M9 M2\n");
                  else
                    fprintf (gcode_f, "M5\nM9\nM2\n");
                  pcb_fprintf (gcode_f,
                    "(end, total distance %`.2fmm = %`.2fin)\n", 25.4 * d, d);
                  fclose (gcode_f);
                }

/* ******************* handle drill-milling **************************** */
              if (save_drill && gcode_drillmill)
                {
                  int n_drillmill_drills = 0;
                  struct drill_hole* drillmill_drills = NULL;
                  double* drillmill_radiuss = NULL;
                  double mill_radius, inaccuracy;

                  /* count drillmill drills */
                  for (int i_drill_sets = 0; i_drill_sets < n_drills; i_drill_sets++)
                    {
                      struct single_size_drills* drill_set = &drills[i_drill_sets];
                      double radius = metric ?
                                      drill_set->diameter_inches * 25.4 / 2:
                                      drill_set->diameter_inches / 2;

                      if (gcode_milltoolradius <= radius)
                        n_drillmill_drills += drill_set->n_holes;
                    }
                  if (n_drillmill_drills > 0) {
                    /* for sorting regardless of size, copy all available drills
                       into one new structure */
                    drillmill_drills = (struct drill_hole *)
                        malloc (n_drillmill_drills * sizeof (struct drill_hole));
                    drillmill_radiuss = (double *)
                        malloc (n_drillmill_drills * sizeof (double));
                    r = 0;
                    for (int i_drill_sets = 0; i_drill_sets < n_drills; i_drill_sets++)
                      {
                        struct single_size_drills* drill_set = &drills[i_drill_sets];
                        double radius = metric ?
                                        drill_set->diameter_inches * 25.4 / 2:
                                        drill_set->diameter_inches / 2;

                        if (gcode_milltoolradius <= radius)
                          {
                            memcpy(&drillmill_drills[r], drill_set->holes,
                                   drill_set->n_holes * sizeof(struct drill_hole));
                            for (int r2 = r; r2 < r + drill_set->n_holes; r2++)
                              drillmill_radiuss[r2] = radius;
                            r += drill_set->n_holes;
                          }
                      }
                    sort_drill(drillmill_drills, n_drillmill_drills);

                    gcode_f = gcode_start_gcode("drillmill", metric);
                    if (!gcode_f)
                      return;
                    fprintf (gcode_f, "(Drillmill file)\n");
                    pcb_fprintf (gcode_f, "(Tool diameter: %`f %s)\n",
                                 gcode_milltoolradius * 2,
                                 metric ? "mm" : "inch");
                    if (gcode_advanced)
                      {
                        pcb_fprintf (gcode_f, "%s=%`f  (safe Z)\n",
                                     variable_safeZ, gcode_safeZ);
                        pcb_fprintf (gcode_f, "%s=%`f  (mill depth)\n",
                                     variable_milldepth, gcode_milldepth);
                        pcb_fprintf (gcode_f, "%s=%`f  (mill plunge feedrate)\n",
                                     variable_millplunge, gcode_millplunge);
                        pcb_fprintf (gcode_f, "%s=%`f  (mill feedrate)\n",
                                     variable_millfeedrate, gcode_millfeedrate);
                        fprintf (gcode_f, "(---------------------------------)\n");
                        fprintf (gcode_f, "G17 G%d G90 G64 P0.003 M3 S3000 M7\n",
                                       metric ? 21 : 20);
                      }
                    else
                      {
                        fprintf (gcode_f, "(---------------------------------)\n");
                        fprintf (gcode_f, "G17\nG%d\nG90\nG64 P0.003\nM3 S3000\nM7\n",
                                 metric ? 21 : 20);
                      }
                    for (r = 0; r < n_drillmill_drills; r++)
                      {
                        double drillX, drillY;

                        if (metric)
                          {
                            drillX = drillmill_drills[r].x * 25.4;
                            drillY = drillmill_drills[r].y * 25.4;
                          }
                        else
                          {
                            drillX = drillmill_drills[r].x;
                            drillY = drillmill_drills[r].y;
                          }
                        pcb_fprintf (gcode_f, "G0 X%`f Y%`f\n", drillX, drillY);
                        fprintf (gcode_f, "G1 Z%s F%s\n",
                                 variable_milldepth, variable_millplunge);

                        mill_radius = drillmill_radiuss[r] - gcode_milltoolradius;
                        inaccuracy = (metric ? 25.4 : 1.) / gcode_dpi;
                        if (mill_radius > inaccuracy)
                          {
                            int n_sides;

                            /* calculate how many polygon sides we need to stay
                               within our accuracy while avoiding a G02/G03 */
                            n_sides = M_PI / acos (mill_radius /
                                                    (mill_radius + inaccuracy));
                            if (n_sides < 4)
                              n_sides = 4;
                            fprintf (gcode_f, "F%s\n", variable_millfeedrate);
                            for (i = 0; i <= n_sides; i++)
                              {
                                double angle = M_PI * 2 * i / n_sides;
                                pcb_fprintf (gcode_f, "G1 X%`f Y%`f\n",
                                         drillX + mill_radius * cos (angle),
                                         drillY + mill_radius * sin (angle));
                              }
                            pcb_fprintf (gcode_f, "G0 X%`f Y%`f\n",
                                           drillX, drillY);
                          }
                        fprintf (gcode_f, "G0 Z%s\n", variable_safeZ);
                      }
                    if (gcode_advanced)
                      fprintf (gcode_f, "M5 M9 M2\n");
                    else
                      fprintf (gcode_f, "M5\nM9\nM2\n");
                    fclose (gcode_f);

                    free(drillmill_radiuss);
                    free(drillmill_drills);
                  }
                }
/* ******************* end of per-layer writing ************************ */

              for (int i_drill_file=0; i_drill_file < n_drills; i_drill_file++)
                free(drills[i_drill_file].holes);
              free (drills);
              drills = NULL;
              n_drills = n_drills_allocated = 0;
            }
        }
    }
    /*
     * General milling. Put this aside from the above code, as paths
     * are generated without taking line or curve thickness into account.
     * Accordingly, we need an entirely different approach.
     */
    /*
     * Currently this is a rather simple implementation, which mills
     * the rectangular extents of the board and nothing else. This should
     * be sufficient for many use cases.
     *
     * A better implementation would have to group the lines and polygons
     * on the outline layer by outer polygon and inner holes, then offset
     * all of them to the right side and mill that.
     */
    /* a better implementation might look like this:
    LAYER_TYPE_LOOP (PCB->Data, max_copper_layer, LT_OUTLINE);
      {
        LINE_LOOP (layer);
          {
            ... calculate the offset for all lines and polygons of this layer,
            mirror it if is_bottom, then mill it ...
          }
        END_LOOP;
      }
    END_LOOP;

    for now: */
    { /* unconditional */
      double lowerX = 0., lowerY = 0., upperX = 0., upperY = 0.;
      double mill_distance = 0.;

      gcode_f = gcode_start_gcode("outline", metric);
      if (!gcode_f)
        return;
      fprintf (gcode_f, "(Outline mill file)\n");
      pcb_fprintf (gcode_f, "(Tool diameter: %`f %s)\n",
                     gcode_milltoolradius * 2, metric ? "mm" : "inch");
      if (gcode_advanced)
        {
          pcb_fprintf (gcode_f, "%s=%`f  (safe Z)\n",
                       variable_safeZ, gcode_safeZ);
          pcb_fprintf (gcode_f, "%s=%`f  (mill depth)\n",
                       variable_milldepth, gcode_milldepth);
          pcb_fprintf (gcode_f, "%s=%`f  (mill plunge feedrate)\n",
                       variable_millplunge, gcode_millplunge);
          pcb_fprintf (gcode_f, "%s=%`f  (mill feedrate)\n",
                       variable_millfeedrate, gcode_millfeedrate);
          fprintf (gcode_f, "(---------------------------------)\n");
          fprintf (gcode_f, "G17 G%d G90 G64 P0.003 M3 S3000 M7\n",
                   metric ? 21 : 20);
        }
      else
        {
          fprintf (gcode_f, "(---------------------------------)\n");
          fprintf (gcode_f, "G17\nG%d\nG90\nG64 P0.003\nM3 S3000\nM7\n",
                   metric ? 21 : 20);
        }
      if (metric)
        {
          upperX = COORD_TO_MM(PCB->MaxWidth);
          upperY = COORD_TO_MM(PCB->MaxHeight);
        }
      else
        {
          upperX = COORD_TO_INCH(PCB->MaxWidth);
          upperY = COORD_TO_INCH(PCB->MaxHeight);
        }
      lowerX -= gcode_milltoolradius;
      lowerY -= gcode_milltoolradius;
      upperX += gcode_milltoolradius;
      upperY += gcode_milltoolradius;

      fprintf (gcode_f, "G0 Z%s\n", variable_safeZ);
      /* mill the two edges adjectant to 0,0 first to disconnect the
         workpiece from the raw material last */
      pcb_fprintf (gcode_f, "G0 X%`f Y%`f\n", upperX, lowerY);
      pcb_fprintf (gcode_f, "G1 Z%s F%s\n",
                   variable_milldepth, variable_millplunge);
      pcb_fprintf (gcode_f, "G1 X%`f Y%`f F%s\n",
                   lowerX, lowerY, variable_millfeedrate);
      pcb_fprintf (gcode_f, "G1 X%`f Y%`f\n", lowerX, upperY);
      pcb_fprintf (gcode_f, "G1 X%`f Y%`f\n", upperX, upperY);
      pcb_fprintf (gcode_f, "G1 X%`f Y%`f\n", upperX, lowerY);
      pcb_fprintf (gcode_f, "G0 Z%s\n", variable_safeZ);

      if (gcode_advanced)
        fprintf (gcode_f, "M5 M9 M2\n");
      else
        fprintf (gcode_f, "M5\nM9\nM2\n");
      mill_distance = abs(gcode_safeZ - gcode_milldepth);
      if (metric)
        mill_distance /= 25.4;
      pcb_fprintf (gcode_f, "(end, total distance G0 %`.2f mm = %`.2f in)\n",
                   mill_distance * 25.4, mill_distance);
      mill_distance = (upperX - lowerX + upperY - lowerY) * 2;
      mill_distance += abs(gcode_safeZ - gcode_milldepth);
      if (metric)
        mill_distance /= 25.4;
      pcb_fprintf (gcode_f, "(     total distance G1 %`.2f mm = %`.2f in)\n",
                   mill_distance * 25.4, mill_distance);
      fclose (gcode_f);
    }
}

/* *** PNG export (slightly modified code from PNG export HID) ************* */

static int
gcode_set_layer (const char *name, int group, int empty)
{
  int idx = (group >= 0 && group < max_group) ?
    PCB->LayerGroups.Entries[group][0] : group;

  if (name == 0)
    {
      name = PCB->Data->Layer[idx].Name;
    }
  if (strcmp (name, "invisible") == 0)
    {
      return 0;
    }
  is_drill = (SL_TYPE (idx) == SL_PDRILL || SL_TYPE (idx) == SL_UDRILL);
  is_mask = (SL_TYPE (idx) == SL_MASK);

  if (is_mask)
    {
      /* Don't print masks */
      return 0;
    }
  if (is_drill)
    {
      /*
       * Print 'holes', so that we can fill gaps in the copper
       * layer
       */
      return 1;
    }
  if (group == gcode_cur_group)
    {
      return 1;
    }
  return 0;
}

static hidGC
gcode_make_gc (void)
{
  hidGC rv = (hidGC) malloc (sizeof (struct hid_gc_struct));
  rv->me_pointer = &gcode_hid;
  rv->cap = Trace_Cap;
  rv->width = 1;
  rv->color = (struct color_struct *) malloc (sizeof (*rv->color));
  rv->color->r = rv->color->g = rv->color->b = 0;
  rv->color->c = 0;
  return rv;
}

static void
gcode_destroy_gc (hidGC gc)
{
  free (gc);
}

static void
gcode_use_mask (enum mask_mode mode)
{
  /* does nothing */
}

static void
gcode_set_color (hidGC gc, const char *name)
{
  if (gcode_im == NULL)
    {
      return;
    }
  if (name == NULL)
    {
      name = "#ff0000";
    }
  if (!strcmp (name, "drill"))
    {
      gc->color = black;
      gc->erase = 0;
      return;
    }
  if (!strcmp (name, "erase"))
    {
      /* FIXME -- should be background, not white */
      gc->color = white;
      gc->erase = 1;
      return;
    }
  gc->color = black;
  gc->erase = 0;
  return;
}

static void
gcode_set_line_cap (hidGC gc, EndCapStyle style)
{
  gc->cap = style;
}

static void
gcode_set_line_width (hidGC gc, Coord width)
{
  gc->width = width;
}

static void
gcode_set_draw_xor (hidGC gc, int xor_)
{
  ;
}

static void
gcode_set_draw_faded (hidGC gc, int faded)
{
}

static void
use_gc (hidGC gc)
{
  int need_brush = 0;

  if (gc->me_pointer != &gcode_hid)
    {
      fprintf (stderr, "Fatal: GC from another HID passed to gcode HID\n");
      abort ();
    }
  if (linewidth != gc->width)
    {
      /* Make sure the scaling doesn't erase lines completely */
      /*
         if (SCALE (gc->width) == 0 && gc->width > 0)
         gdImageSetThickness (im, 1);
         else
       */
      gdImageSetThickness (gcode_im,
                           pcb_to_gcode (gc->width + 2 * gcode_toolradius));
      linewidth = gc->width;
      need_brush = 1;
    }
  if (lastbrush != gc->brush || need_brush)
    {
      static void *bcache = 0;
      hidval bval;
      const size_t name_len = 256;
      char name[name_len];
      char type;
      int r;

      switch (gc->cap)
        {
        case Round_Cap:
        case Trace_Cap:
          type = 'C';
          r = pcb_to_gcode (gc->width / 2 + gcode_toolradius);
          break;
        default:
        case Square_Cap:
          r = pcb_to_gcode (gc->width + gcode_toolradius * 2);
          type = 'S';
          break;
        }
      snprintf (name, name_len, "#%.2x%.2x%.2x_%c_%d", gc->color->r,
                gc->color->g, gc->color->b, type, r);

      if (hid_cache_color (0, name, &bval, &bcache))
        {
          gc->brush = (gdImagePtr)bval.ptr;
        }
      else
        {
          int bg, fg;
          if (type == 'C')
            gc->brush = gdImageCreate (2 * r + 1, 2 * r + 1);
          else
            gc->brush = gdImageCreate (r + 1, r + 1);
          bg = gdImageColorAllocate (gc->brush, 255, 255, 255);
          fg =
            gdImageColorAllocate (gc->brush, gc->color->r, gc->color->g,
                                  gc->color->b);
          gdImageColorTransparent (gc->brush, bg);

          /*
           * if we shrunk to a radius/box width of zero, then just use
           * a single pixel to draw with.
           */
          if (r == 0)
            gdImageFilledRectangle (gc->brush, 0, 0, 0, 0, fg);
          else
            {
              if (type == 'C')
                gdImageFilledEllipse (gc->brush, r, r, 2 * r, 2 * r, fg);
              else
                gdImageFilledRectangle (gc->brush, 0, 0, r, r, fg);
            }
          bval.ptr = gc->brush;
          hid_cache_color (1, name, &bval, &bcache);
        }

      gdImageSetBrush (gcode_im, gc->brush);
      lastbrush = gc->brush;

    }
}

static void
gcode_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  use_gc (gc);
  gdImageRectangle (gcode_im,
                    pcb_to_gcode (x1 - gcode_toolradius),
                    pcb_to_gcode (y1 - gcode_toolradius),
                    pcb_to_gcode (x2 + gcode_toolradius),
                    pcb_to_gcode (y2 + gcode_toolradius),
                    gc->color->c);
/*      printf("Rect %d %d %d %d\n",x1,y1,x2,y2); */
}

static void
gcode_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  use_gc (gc);
  gdImageSetThickness (gcode_im, 0);
  linewidth = 0;
  gdImageFilledRectangle (gcode_im,
                      pcb_to_gcode (x1 - gcode_toolradius),
                      pcb_to_gcode (y1 - gcode_toolradius),
                      pcb_to_gcode (x2 + gcode_toolradius),
                      pcb_to_gcode (y2 + gcode_toolradius),
                      gc->color->c);
/*      printf("FillRect %d %d %d %d\n",x1,y1,x2,y2); */
}

static void
gcode_draw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  if (x1 == x2 && y1 == y2)
    {
      Coord w = gc->width / 2;
      gcode_fill_rect (gc,
                       x1 - w, y1 - w,
                       x1 + w, y1 + w);
      return;
    }
  use_gc (gc);

  gdImageSetThickness (gcode_im, 0);
  linewidth = 0;
  gdImageLine (gcode_im,
               pcb_to_gcode (x1),
               pcb_to_gcode (y1),
               pcb_to_gcode (x2),
               pcb_to_gcode (y2),
               gdBrushed);
}

static void
gcode_draw_arc (hidGC gc, Coord cx, Coord cy, Coord width, Coord height,
                Angle start_angle, Angle delta_angle)
{
  Angle sa, ea;

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
   * make sure we start between 0 and 360 otherwise gd does strange
   * things
   */
  sa = NormalizeAngle (sa);
  ea = NormalizeAngle (ea);

#if 0
  printf ("draw_arc %d,%d %dx%d %d..%d %d..%d\n",
          cx, cy, width, height, start_angle, delta_angle, sa, ea);
  printf ("gdImageArc (%p, %d, %d, %d, %d, %d, %d, %d)\n",
          im, SCALE_X (cx), SCALE_Y (cy),
          SCALE (width), SCALE (height), sa, ea, gc->color->c);
#endif
  use_gc (gc);
  gdImageSetThickness (gcode_im, 0);
  linewidth = 0;
  gdImageArc (gcode_im,
              pcb_to_gcode (cx),
              pcb_to_gcode (cy),
              pcb_to_gcode (2 * width + gcode_toolradius * 2),
              pcb_to_gcode (2 * height + gcode_toolradius * 2), sa, ea,
              gdBrushed);
}

/*!
 * \brief Given a hole size, return the structure that currently holds
 * the data for that hole size.
 *
 * If there isn't one, make it.
 */
static int _drill_size_comparator(const void* _size0, const void* _size1)
{
  double size0 = ((const struct single_size_drills*)_size0)->diameter_inches;
  double size1 = ((const struct single_size_drills*)_size1)->diameter_inches;
  if (size0 == size1)
    return 0;

  if (size0 < size1)
    return -1;

  return 1;
}

static struct single_size_drills*
get_drill(double diameter_inches)
{
  /* see if we already have this size. If so, return that structure */
  struct single_size_drills* drill =
    bsearch (&diameter_inches,
             drills, n_drills, sizeof (drills[0]),
             _drill_size_comparator);
  if (drill != NULL)
    return drill;

  /* haven't seen this hole size before, so make a new structure for it */
  if (n_drills == n_drills_allocated)
    {
      n_drills_allocated += 100;
      drills =
        (struct single_size_drills *) realloc (drills,
                                               n_drills_allocated *
                                               sizeof (struct single_size_drills));
    }

  /* I now add the structure to the list, making sure to keep the list
   * sorted. Ideally the bsearch() call above would have given me the location
   * to insert this element while keeping things sorted, but it doesn't. For
   * simplicity I manually lsearch() to find this location myself */
  {
    int i = 0;
    for (; i<n_drills; i++)
      if (drills[i].diameter_inches >= diameter_inches)
        break;

    if (n_drills != i)
      memmove (&drills[i+1], &drills[i],
               (n_drills-i) * sizeof (struct single_size_drills));

    drills[i].diameter_inches   = diameter_inches;
    drills[i].n_holes           = 0;
    drills[i].n_holes_allocated = 0;
    drills[i].holes             = NULL;
    n_drills++;

    return &drills[i];
  }
}

static void
add_hole (struct single_size_drills* drill,
          double cx_inches, double cy_inches)
{
  if (drill->n_holes == drill->n_holes_allocated)
    {
      drill->n_holes_allocated += 100;
      drill->holes =
        (struct drill_hole *) realloc (drill->holes,
                                       drill->n_holes_allocated *
                                       sizeof (struct drill_hole));
    }

  drill->holes[ drill->n_holes ].x = cx_inches;
  drill->holes[ drill->n_holes ].y = cy_inches;
  drill->n_holes++;
}

static void
gcode_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius)
{
  use_gc (gc);

  gdImageSetThickness (gcode_im, 0);
  linewidth = 0;
  gdImageFilledEllipse (gcode_im,
                        pcb_to_gcode (cx),
                        pcb_to_gcode (cy),
                        pcb_to_gcode (2 * radius + gcode_toolradius * 2),
                        pcb_to_gcode (2 * radius + gcode_toolradius * 2),
                        gc->color->c);
  if (save_drill && is_drill)
    {
      double diameter_inches = COORD_TO_INCH(radius*2);

      struct single_size_drills* drill = get_drill (diameter_inches);
      add_hole (drill,
                /* convert to inch, flip: will drill from bottom side */
                COORD_TO_INCH(PCB->MaxWidth  - cx),
                /* PCB reverses y axis */
                COORD_TO_INCH(PCB->MaxHeight - cy));
    }
}

static void
gcode_fill_polygon (hidGC gc, int n_coords, Coord *x, Coord *y)
{
  int i;
  gdPoint *points;

  points = (gdPoint *) malloc (n_coords * sizeof (gdPoint));
  if (points == NULL)
    {
      fprintf (stderr, "ERROR:  gcode_fill_polygon():  malloc failed\n");
      exit (1);
    }
  use_gc (gc);
  for (i = 0; i < n_coords; i++)
    {
      points[i].x = pcb_to_gcode (x[i]);
      points[i].y = pcb_to_gcode (y[i]);
    }
  linewidth = pcb_to_gcode (2 * gcode_toolradius);
  gdImageSetThickness (gcode_im, linewidth);
  gdImageFilledPolygon (gcode_im, points, n_coords, gc->color->c);
  free (points);
/*      printf("FillPoly\n"); */
}

static void
gcode_calibrate (double xval, double yval)
{
  CRASH;
}

static void
gcode_set_crosshair (int x, int y, int a)
{
}

/* *** Miscellaneous ******************************************************* */

#include "dolists.h"

void
hid_gcode_init ()
{
  memset (&gcode_hid, 0, sizeof (HID));
  memset (&gcode_graphics, 0, sizeof (HID_DRAW));

  common_nogui_init (&gcode_hid);
  common_draw_helpers_init (&gcode_graphics);

  gcode_hid.struct_size         = sizeof (HID);
  gcode_hid.name                = "gcode";
  gcode_hid.description         = "G-CODE export";
  gcode_hid.exporter            = 1;
  gcode_hid.poly_before         = 1;

  gcode_hid.get_export_options  = gcode_get_export_options;
  gcode_hid.do_export           = gcode_do_export;
  gcode_hid.parse_arguments     = gcode_parse_arguments;
  gcode_hid.set_layer           = gcode_set_layer;
  gcode_hid.calibrate           = gcode_calibrate;
  gcode_hid.set_crosshair       = gcode_set_crosshair;

  gcode_hid.graphics            = &gcode_graphics;

  gcode_graphics.make_gc        = gcode_make_gc;
  gcode_graphics.destroy_gc     = gcode_destroy_gc;
  gcode_graphics.use_mask       = gcode_use_mask;
  gcode_graphics.set_color      = gcode_set_color;
  gcode_graphics.set_line_cap   = gcode_set_line_cap;
  gcode_graphics.set_line_width = gcode_set_line_width;
  gcode_graphics.set_draw_xor   = gcode_set_draw_xor;
  gcode_graphics.set_draw_faded = gcode_set_draw_faded;
  gcode_graphics.draw_line      = gcode_draw_line;
  gcode_graphics.draw_arc       = gcode_draw_arc;
  gcode_graphics.draw_rect      = gcode_draw_rect;
  gcode_graphics.fill_circle    = gcode_fill_circle;
  gcode_graphics.fill_polygon   = gcode_fill_polygon;
  gcode_graphics.fill_rect      = gcode_fill_rect;

  hid_register_hid (&gcode_hid);

#include "gcode_lists.h"
}

