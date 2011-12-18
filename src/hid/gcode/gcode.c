/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *
 *  GCODE export HID
 *  Copyright (C) 2010 Alberto Maccioni
 *  this code is based on the NELMA export HID, the PNG export HID,
 *  and potrace, a tracing program by Peter Selinger
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
 * This HID exports a PCB layout into:
 * one layer mask file (PNG format) per copper layer,
 * one G-CODE CNC drill file per drill size
 * one G-CODE CNC file per copper layer.
 * The latter is used by a CNC milling machine to mill the pcb.
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

#include "global.h"
#include "error.h" /* Message() */
#include "data.h"
#include "misc.h"
#include "rats.h"

#include "hid.h"
#include "../hidint.h"
#include <gd.h>
#include "hid/common/hidnogui.h"
#include "hid/common/draw_helpers.h"
#include "gcode.h"
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
struct color_struct
{
  /* the descriptor used by the gd library */
  int c;

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
  int faded;
  struct color_struct *color;
  gdImagePtr brush;
};

static struct color_struct *black = NULL, *white = NULL;
static int linewidth = -1;
static gdImagePtr lastbrush = (gdImagePtr)((void *) -1);
static int lastcolor = -1;

/* gd image and file for PNG export */
static gdImagePtr gcode_im = NULL;
static FILE *gcode_f = NULL, *gcode_f2 = NULL;

static int is_mask;
static int is_drill;
static int is_solder;

/*
 * Which groups of layers to export into PNG layer masks. 1 means export, 0
 * means do not export.
 */
static int gcode_export_group[MAX_LAYER];

/* Group that is currently exported. */
static int gcode_cur_group;

/* Filename prefix and suffix that will be used when saving files. */
static const char *gcode_basename = NULL;

/* Horizontal DPI (grid points per inch) */
static int gcode_dpi = -1;

static double gcode_cutdepth = 0;       /* milling depth (inch) */
static double gcode_drilldepth = 0;     /* drilling depth (inch) */
static double gcode_safeZ = 100;        /* safe Z (inch) */
static double gcode_toolradius = 0;     /* tool radius (1/100 mil) */
static char gcode_advanced = 0;
static int save_drill = 0;

/* structure to represent a single hole */
struct drill_hole
{
  double x;
  double y;
};

/* structure to represent all holes of a given size */
struct single_size_drills
{
  double diameter_inches;

  int n_holes;
  int n_holes_allocated;
  struct drill_hole* holes;
};

/* at the start we have no drills at all */
static struct single_size_drills* drills             = NULL;
static int                        n_drills           = 0;
static int                        n_drills_allocated = 0;

static const char *units[] = {
  "mm",
  "mil",
  "um",
  "inch",
  NULL
};

HID_Attribute gcode_attribute_list[] = {
  /* other HIDs expect this to be first.  */
  {"basename", "File name prefix and suffix,\n"
               "layer names will be inserted before the suffix",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_basename 0

  {"dpi", "Resolution of intermediate image (pixels/inch)",
   HID_Integer, 0, 2000, {600, 0, 0}, 0, 0},
#define HA_dpi 1

  {"mill-depth", "Milling depth",
   HID_Real, -1000, 1000, {0, 0, -0.05}, 0, 0},
#define HA_cutdepth 2

  {"safe-Z", "Safe Z for traverse move",
   HID_Real, -1000, 10000, {0, 0, 2}, 0, 0},
#define HA_safeZ 3

  {"tool-diameter", "Milling tool diameter, or twice the offset of the\n"
                    "G-code track from the resulting copper track",
   HID_Real, 0, 10000, {0, 0, 0.2}, 0, 0},
#define HA_tooldiameter 4

  {"drill-depth", "Drilling depth",
   HID_Real, -10000, 10000, {0, 0, -2}, 0, 0},
#define HA_drilldepth 5

  {"measurement-unit", "Measurement unit",
   HID_Unit, 0, 0, {-1, 0, 0}, units, 0},
#define HA_unit 6

  {"advanced-gcode", "wether to produce G-code for advanced interpreters,\n"
                     "like using variables or drill cycles. Not all\n"
                     "machine controllers understand this, but it allows\n"
                     "better hand-editing of the resulting files",
   HID_Boolean, 0, 0, {-1, 0, 0}, 0, 0},
#define HA_advanced 7
};

#define NUM_OPTIONS (sizeof(gcode_attribute_list)/sizeof(gcode_attribute_list[0]))

REGISTER_ATTRIBUTES (gcode_attribute_list)
     static HID_Attr_Val gcode_values[NUM_OPTIONS];

/* *** Utility funcions **************************************************** */

/* convert from default PCB units to gcode units */
static int pcb_to_gcode (int pcb)
{
  return round(COORD_TO_INCH(pcb) * gcode_dpi);
}

/* Fits the given layer name into basename, just before the suffix */
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

/* Sorts drills to produce a short tool path. I start with the hole nearest
 * (0,0) and for each subsequent one, find the hole nearest to the previous.
 * This isn't guaranteed to find the shortest path, but should be good enough.
 * Note that this is O(N^2). We can't use the O(N logN) sort, since our
 * shortest-distance origin changes with every point */
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

/* Populates gcode_export_group array */
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

static void
gcode_alloc_colors ()
{
  /*
   * Allocate white and black -- the first color allocated becomes the
   * background color
   */

  white = (struct color_struct *) malloc (sizeof (*white));
  white->r = white->g = white->b = 255;
  white->c = gdImageColorAllocate (gcode_im, white->r, white->g, white->b);

  black = (struct color_struct *) malloc (sizeof (*black));
  black->r = black->g = black->b = 0;
  black->c = gdImageColorAllocate (gcode_im, black->r, black->g, black->b);
}

static void
gcode_start_png (const char *layername)
{
  int h, w;
  char *png_filename, *buf;

  png_filename = (char *)malloc (MAXPATHLEN);
  gcode_get_filename (png_filename, layername);
  buf = g_strdup_printf ("%s.png", png_filename);
  free(png_filename);

  h = pcb_to_gcode (PCB->MaxHeight);
  w = pcb_to_gcode (PCB->MaxWidth);

  /* Nelma only works with true color images */
  gcode_im = gdImageCreate (w, h);
  gcode_f = fopen (buf, "wb");

  gcode_alloc_colors ();

  g_free (buf);
}

static void
gcode_finish_png ()
{
#ifdef HAVE_GDIMAGEPNG
  gdImagePng (gcode_im, gcode_f);
#else
  Message ("GCODE: PNG not supported by gd. Can't write layer mask.\n");
#endif
  gdImageDestroy (gcode_im);
  fclose (gcode_f);

  free (white);
  free (black);

  gcode_im = NULL;
  gcode_f = NULL;
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
  lastcolor = -1;

  hid_expose_callback (&gcode_hid, &region, 0);
}

static void
gcode_do_export (HID_Attr_Val * options)
{
  int save_ons[MAX_LAYER + 2];
  int i, idx;
  time_t t;
  const Unit *unit;
  double scale = 0, d = 0;
  int r, c, v, p, metric;
  char *filename;
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
  char variable_drilldepth[20];

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
  gcode_drilldepth = options[HA_drilldepth].real_value * scale;
  gcode_safeZ = options[HA_safeZ].real_value * scale;
  gcode_toolradius = metric
                   ? MM_TO_COORD(options[HA_tooldiameter].real_value / 2 * scale)
                   : INCH_TO_COORD(options[HA_tooldiameter].real_value / 2 * scale);
  gcode_advanced = options[HA_advanced].int_value;
  gcode_choose_groups ();
  if (gcode_advanced)
    {
      strcpy (variable_safeZ, "#100");
      strcpy (variable_cutdepth, "#101");
      strcpy (variable_drilldepth, "#102");
    }
  else
    {
      snprintf (variable_safeZ, 20, "%f", gcode_safeZ);
      snprintf (variable_cutdepth, 20, "%f", gcode_cutdepth);
      snprintf (variable_drilldepth, 20, "%f", gcode_drilldepth);
    }

  for (i = 0; i < MAX_LAYER; i++)
    {
      if (gcode_export_group[i])
        {
          gcode_cur_group = i;

          /* magic */
          idx = (i >= 0 && i < max_group) ?
            PCB->LayerGroups.Entries[i][0] : i;
          is_solder =
            (GetLayerGroupNumberByNumber (idx) ==
             GetLayerGroupNumberByNumber (solder_silk_layer)) ? 1 : 0;
          save_drill = is_solder; /* save drills for one layer only */
          gcode_start_png (layer_type_to_file_name (idx, FNS_fixed));
          hid_save_and_show_layer_ons (save_ons);
          gcode_start_png_export ();
          hid_restore_layer_ons (save_ons);

/* ***************** gcode conversion *************************** */
/* potrace uses a different kind of bitmap; for simplicity gcode_im is copied to this format */
          bm = bm_new (gdImageSX (gcode_im), gdImageSY (gcode_im));
          filename = (char *)malloc (MAXPATHLEN);
          plist = NULL;
          if (is_solder)
            { /* only for back layer */
              gdImagePtr temp_im =
                gdImageCreate (gdImageSX (gcode_im), gdImageSY (gcode_im));
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
          gcode_get_filename (filename, layer_type_to_file_name (idx, FNS_fixed));
          for (r = 0; r < gdImageSX (gcode_im); r++)
            {
              for (c = 0; c < gdImageSY (gcode_im); c++)
                {
                  v =
                    gdImageGetPixel (gcode_im, r,
                                     gdImageSY (gcode_im) - 1 - c);
                  p = (gcode_im->red[v] || gcode_im->green[v]
                       || gcode_im->blue[v]) ? 0 : 0xFFFFFF;
                  BM_PUT (bm, r, c, p);
                }
            }
          gcode_f2 = fopen (filename, "wb");
          if (!gcode_f2)
            {
              perror (filename);
              return;
            }
          fprintf (gcode_f2, "(Created by G-code exporter)\n");
          t = time (NULL);
          sprintf (filename, "%s", ctime (&t));
          filename[strlen (filename) - 1] = 0;
          fprintf (gcode_f2, "( %s )\n", filename);
          fprintf (gcode_f2, "(%d dpi)\n", gcode_dpi);
          fprintf (gcode_f2, "(Unit: %s)\n", metric ? "mm" : "inch");
          fprintf (gcode_f2, "(Tool diameter: %f %s)\n",
                   options[HA_tooldiameter].real_value * scale,
                   metric ? "mm" : "inch");
          if (metric)
            pcb_fprintf (gcode_f2, "(Board size: %.2mmx%.2mm mm)\n", PCB->MaxWidth, PCB->MaxHeight);
          else
            pcb_fprintf (gcode_f2, "(Board size: %.2mix%.2mi inches)\n", PCB->MaxWidth, PCB->MaxHeight);
          if (gcode_advanced)
            {
              fprintf (gcode_f2, "%s=%f  (safe Z)\n",
                       variable_safeZ, gcode_safeZ);
              fprintf (gcode_f2, "%s=%f  (cutting depth)\n",
                       variable_cutdepth, gcode_cutdepth);
              fprintf (gcode_f2, "(---------------------------------)\n");
              fprintf (gcode_f2, "G17 G%d G90 G64 P0.003 M3 S3000 M7 F%d\n",
                       metric ? 21 : 20, metric ? 25 : 1);
            }
          else
            {
              fprintf (gcode_f2, "(---------------------------------)\n");
              fprintf (gcode_f2, "G17\nG%d\nG90\nG64 P0.003\nM3 S3000\nM7\nF%d\n",
                       metric ? 21 : 20, metric ? 25 : 1);
            }
          fprintf (gcode_f2, "G0 Z%s\n", variable_safeZ);
          /* extract contour points from image */
          r = bm_to_pathlist (bm, &plist, &param_default);
          if (r)
            {
              fprintf (stderr, "ERROR: pathlist function failed\n");
              return;
            }
          /* generate best polygon and write vertices in g-code format */
          d = process_path (plist, &param_default, bm, gcode_f2,
                            metric ? 25.4 / gcode_dpi : 1.0 / gcode_dpi,
                            variable_cutdepth, variable_safeZ);
          if (d < 0)
            {
              fprintf (stderr, "ERROR: path process function failed\n");
              return;
            }
          if (metric)
            fprintf (gcode_f2, "(end, total distance %.2fmm = %.2fin)\n", d,
                     d * 1 / 25.4);
          else
            fprintf (gcode_f2, "(end, total distance %.2fmm = %.2fin)\n",
                     25.4 * d, d);
          if (gcode_advanced)
            fprintf (gcode_f2, "M5 M9 M2\n");
          else
            fprintf (gcode_f2, "M5\nM9\nM2\n");
          pathlist_free (plist);
          bm_free (bm);
          fclose (gcode_f2);
          if (save_drill)
            {
              for (int i_drill_file=0; i_drill_file < n_drills; i_drill_file++)
                {
                  struct single_size_drills* drill = &drills[i_drill_file];

                  d = 0;
                  sort_drill (drill->holes, drill->n_holes);

                  {
                    // get the filename with the drill size encoded in it
                    char layername[32];
                    snprintf(layername, sizeof(layername),
                             "%.4f.drill",
                             metric ?
                             drill->diameter_inches * 25.4 :
                             drill->diameter_inches);
                    gcode_get_filename (filename, layername);
                  }

                  gcode_f2 = fopen (filename, "wb");
                  if (!gcode_f2)
                    {
                      perror (filename);
                      return;
                    }
                  fprintf (gcode_f2, "(Created by G-code exporter)\n");
                  fprintf (gcode_f2, "(drill file: %d drills)\n", drill->n_holes);
                  sprintf (filename, "%s", ctime (&t));
                  filename[strlen (filename) - 1] = 0;
                  fprintf (gcode_f2, "( %s )\n", filename);
                  fprintf (gcode_f2, "(Unit: %s)\n", metric ? "mm" : "inch");
                  if (metric)
                    pcb_fprintf (gcode_f2, "(Board size: %.2mmx%.2mm mm)\n", PCB->MaxWidth, PCB->MaxHeight);
                  else
                    pcb_fprintf (gcode_f2, "(Board size: %.2mix%.2mi inches)\n", PCB->MaxWidth, PCB->MaxHeight);
                  if (gcode_advanced)
                    {
                      fprintf (gcode_f2, "%s=%f  (safe Z)\n",
                               variable_safeZ, gcode_safeZ);
                      fprintf (gcode_f2, "%s=%f  (drill depth)\n",
                               variable_drilldepth, gcode_drilldepth);
                      fprintf (gcode_f2, "(---------------------------------)\n");
                      fprintf (gcode_f2, "G17 G%d G90 G64 P0.003 M3 S3000 M7 F%d\n",
                               metric ? 21 : 20, metric ? 25 : 1);
                    }
                  else
                    {
                      fprintf (gcode_f2, "(---------------------------------)\n");
                      fprintf (gcode_f2, "G17\nG%d\nG90\nG64 P0.003\nM3 S3000\nM7\nF%d\n",
                               metric ? 21 : 20, metric ? 25 : 1);
                    }
                  fprintf (gcode_f2, "G0 Z%s\n", variable_safeZ);
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
                        fprintf (gcode_f2, "G81 X%f Y%f Z%s R%s\n", drillX, drillY,
                                 variable_drilldepth, variable_safeZ);
                      else
                        {
                          fprintf (gcode_f2, "G0 X%f Y%f\n", drillX, drillY);
                          fprintf (gcode_f2, "G1 Z%s\n", variable_drilldepth);
                          fprintf (gcode_f2, "G0 Z%s\n", variable_safeZ);
                        }
                      if (r > 0)
                        d += Distance(drill->holes[r - 1].x, drill->holes[r - 1].y,
                                      drill->holes[r    ].x, drill->holes[r    ].y);
                    }
                  if (gcode_advanced)
                    fprintf (gcode_f2, "M5 M9 M2\n");
                  else
                    fprintf (gcode_f2, "M5\nM9\nM2\n");
                  fprintf (gcode_f2, "(end, total distance %.2fmm = %.2fin)\n",
                           25.4 * d, d);
                  fclose (gcode_f2);

                  free(drills[i_drill_file].holes);
                }

              free (drills);
              drills = NULL;
              n_drills = n_drills_allocated = 0;
            }
          free (filename);

/* ******************* end gcode conversion **************************** */
          gcode_finish_png ();
        }
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
gcode_use_mask (int use_it)
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
  gc->faded = faded;
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
      char name[256];
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
      sprintf (name, "#%.2x%.2x%.2x_%c_%d", gc->color->r, gc->color->g,
               gc->color->b, type, r);

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
gcode_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  use_gc (gc);
  gdImageRectangle (gcode_im,
                    pcb_to_gcode (x1 - gcode_toolradius),
                    pcb_to_gcode (y1 - gcode_toolradius),
                    pcb_to_gcode (x2 + gcode_toolradius),
                    pcb_to_gcode (y2 + gcode_toolradius), gc->color->c);
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
                          pcb_to_gcode (y2 + gcode_toolradius), gc->color->c);
/*      printf("FillRect %d %d %d %d\n",x1,y1,x2,y2); */
}

static void
gcode_draw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  if (x1 == x2 && y1 == y2)
    {
      Coord w = gc->width / 2;
      gcode_fill_rect (gc, x1 - w, y1 - w, x1 + w, y1 + w);
      return;
    }
  use_gc (gc);

  gdImageSetThickness (gcode_im, 0);
  linewidth = 0;
  gdImageLine (gcode_im, pcb_to_gcode (x1), pcb_to_gcode (y1),
               pcb_to_gcode (x2), pcb_to_gcode (y2), gdBrushed);
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
  gdImageArc (gcode_im, pcb_to_gcode (cx), pcb_to_gcode (cy),
              pcb_to_gcode (2 * width + gcode_toolradius * 2),
              pcb_to_gcode (2 * height + gcode_toolradius * 2), sa, ea,
              gdBrushed);
}

/* given a hole size, return the structure that currently holds the data for
   that hole size. If there isn't one, make it */
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
  gdImageFilledEllipse (gcode_im, pcb_to_gcode (cx), pcb_to_gcode (cy),
                        pcb_to_gcode (2 * radius + gcode_toolradius * 2),
                        pcb_to_gcode (2 * radius + gcode_toolradius * 2),
                        gc->color->c);
  if (save_drill && is_drill)
    {
      double diameter_inches = COORD_TO_INCH(radius*2);

      struct single_size_drills* drill = get_drill (diameter_inches);
      add_hole (drill,
                COORD_TO_INCH(PCB->MaxWidth  - cx),  /* convert to inch, flip: will drill from bottom side */
                COORD_TO_INCH(PCB->MaxHeight - cy)); /* PCB reverses y axis */
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
  gdImageSetThickness (gcode_im, 0);
  linewidth = 0;
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

HID gcode_hid;

void
hid_gcode_init ()
{
  memset (&gcode_hid, 0, sizeof (HID));

  common_nogui_init (&gcode_hid);
  common_draw_helpers_init (&gcode_hid);

  gcode_hid.struct_size         = sizeof (HID);
  gcode_hid.name                = "gcode";
  gcode_hid.description         = "G-CODE export";
  gcode_hid.exporter            = 1;
  gcode_hid.poly_before         = 1;

  gcode_hid.get_export_options  = gcode_get_export_options;
  gcode_hid.do_export           = gcode_do_export;
  gcode_hid.parse_arguments     = gcode_parse_arguments;
  gcode_hid.set_layer           = gcode_set_layer;
  gcode_hid.make_gc             = gcode_make_gc;
  gcode_hid.destroy_gc          = gcode_destroy_gc;
  gcode_hid.use_mask            = gcode_use_mask;
  gcode_hid.set_color           = gcode_set_color;
  gcode_hid.set_line_cap        = gcode_set_line_cap;
  gcode_hid.set_line_width      = gcode_set_line_width;
  gcode_hid.set_draw_xor        = gcode_set_draw_xor;
  gcode_hid.set_draw_faded      = gcode_set_draw_faded;
  gcode_hid.draw_line           = gcode_draw_line;
  gcode_hid.draw_arc            = gcode_draw_arc;
  gcode_hid.draw_rect           = gcode_draw_rect;
  gcode_hid.fill_circle         = gcode_fill_circle;
  gcode_hid.fill_polygon        = gcode_fill_polygon;
  gcode_hid.fill_rect           = gcode_fill_rect;
  gcode_hid.calibrate           = gcode_calibrate;
  gcode_hid.set_crosshair       = gcode_set_crosshair;

  hid_register_hid (&gcode_hid);

#include "gcode_lists.h"
}

