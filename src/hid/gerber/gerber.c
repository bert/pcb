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

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented Gerber function %s.\n", __FUNCTION__); abort()

/*----------------------------------------------------------------------------*/
/* Function prototypes                                                        */
/*----------------------------------------------------------------------------*/

static HID_Attribute * gerber_get_export_options (int *n);
static void gerber_do_export (HID_Attr_Val * options);
static void gerber_parse_arguments (int *argc, char ***argv);
static int gerber_set_layer (const char *name, int group, int empty);
static hidGC gerber_make_gc (void);
static void gerber_destroy_gc (hidGC gc);
static void gerber_use_mask (enum mask_mode mode);
static void gerber_set_color (hidGC gc, const char *name);
static void gerber_set_line_cap (hidGC gc, EndCapStyle style);
static void gerber_set_line_width (hidGC gc, Coord width);
static void gerber_set_draw_xor (hidGC gc, int _xor);
static void gerber_draw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);
static void gerber_draw_arc (hidGC gc, Coord cx, Coord cy, Coord width, Coord height, Angle start_angle, Angle delta_angle);
static void gerber_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);
static void gerber_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius);
static void gerber_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);
static void gerber_calibrate (double xval, double yval);
static void gerber_set_crosshair (int x, int y, int action);
static void gerber_fill_polygon (hidGC gc, int n_coords, Coord *x, Coord *y);

/*----------------------------------------------------------------------------*/
/* Utility routines                                                           */
/*----------------------------------------------------------------------------*/

/* These are for films */
#define gerberX(pcb, x) ((Coord) (x))
#define gerberY(pcb, y) ((Coord) ((pcb)->MaxHeight - (y)))
#define gerberXOffset(pcb, x) ((Coord) (x))
#define gerberYOffset(pcb, y) ((Coord) (-(y)))

/* These are for drills */
#define gerberDrX(pcb, x) ((Coord) (x))
#define gerberDrY(pcb, y) ((Coord) ((pcb)->MaxHeight - (y)))

/*----------------------------------------------------------------------------*/
/* Private data structures                                                    */
/*----------------------------------------------------------------------------*/

static int verbose;
static int all_layers;
static int metric;
static char *x_convspec, *y_convspec;
static int is_mask, was_drill;
static int is_drill;
static enum mask_mode current_mask;
static int flash_drills;
static int copy_outline_mode;
static int name_style;
static LayerType *outline_layer;

#define print_xcoord(file, pcb, val)\
	pcb_fprintf(file, x_convspec, gerberX(pcb, val))

#define print_ycoord(file, pcb, val)\
	pcb_fprintf(file, y_convspec, gerberY(pcb, val))

enum ApertureShape
{
  ROUND,			/* Shaped like a circle */
  OCTAGON,			/* octagonal shape */
  SQUARE,			/* Shaped like a square */
  ROUNDCLEAR,			/* clearance in negatives */
  SQUARECLEAR,
  THERMAL			/* negative thermal relief */
};
typedef enum ApertureShape ApertureShape;

/* This is added to the global aperture array indexes to get gerber
   dcode and macro numbers.  */
#define DCODE_BASE 11

typedef struct aperture
{
  int dCode;			/* The RS-274X D code */
  Coord width;		/* Size in pcb units */
  ApertureShape shape;		/* ROUND/SQUARE etc */
  struct aperture *next;
}
Aperture;

typedef struct 
{
  Aperture *data;
  int count;
} ApertureList;

static ApertureList *layer_aptr_list;
static ApertureList *curr_aptr_list;
static int layer_list_max;
static int layer_list_idx;

typedef struct
{
  Coord diam;
  Coord x;
  Coord y;
} PendingDrills;
PendingDrills *pending_drills = NULL;
int n_pending_drills = 0, max_pending_drills = 0;

/*----------------------------------------------------------------------------*/
/* Defined Constants                                                          */
/*----------------------------------------------------------------------------*/
#define AUTO_OUTLINE_WIDTH MIL_TO_COORD(8)       /* Auto-geneated outline width of 8 mils */

/*----------------------------------------------------------------------------*/
/* Aperture Routines                                                          */
/*----------------------------------------------------------------------------*/

/* Initialize aperture list */
static void
initApertureList (ApertureList *list)
{
  list->data = NULL;
  list->count = 0;
}

static void
deinitApertureList (ApertureList *list)
{
  Aperture *search = list->data;
  Aperture *next;
  while (search)
    {
      next = search->next;
      free(search);
      search = next;
    }
  initApertureList (list);
}

static int aperture_count;

static void resetApertures()
{
  int i;
  for (i = 0; i < layer_list_max; ++i)
    deinitApertureList (&layer_aptr_list[i]);
  free (layer_aptr_list);
  layer_aptr_list = NULL;
  curr_aptr_list  = NULL;
  layer_list_max = 0;
  layer_list_idx = 0;
  aperture_count = 0;
}

/* Create and add a new aperture to the list */
static Aperture *
addAperture (ApertureList *list, Coord width, ApertureShape shape)
{

  Aperture *app = (Aperture *) malloc (sizeof *app);
  if (app == NULL)
    return NULL;

  app->width = width;
  app->shape = shape;
  app->dCode = DCODE_BASE + aperture_count++;
  app->next  = list->data;

  list->data = app;
  ++list->count;

  return app;
}

/* Fetch an aperture from the list with the specified
 *  width/shape, creating a new one if none exists */
static Aperture *
findAperture (ApertureList *list, Coord width, ApertureShape shape)
{
  Aperture *search;

  /* we never draw zero-width lines */
  if (width == 0)
    return NULL;

  /* Search for an appropriate aperture. */
  for (search = list->data; search; search = search->next)
    if (search->width == width && search->shape == shape)
      return search;

  /* Failing that, create a new one */
  return addAperture (list, width, shape);
}

/* Output aperture data to the file */
static void
fprintAperture (FILE *f, Aperture *aptr)
{
  switch (aptr->shape)
    {
    case ROUND:
      pcb_fprintf (f, metric ? "%%ADD%dC,%.3`mm*%%\r\n" : "%%ADD%dC,%.4`mi*%%\r\n", aptr->dCode, aptr->width);
      break;
    case SQUARE:
      pcb_fprintf (f, metric ? "%%ADD%dR,%.3`mmX%.3`mm*%%\r\n" : "%%ADD%dR,%.4`miX%.4`mi*%%\r\n", aptr->dCode, aptr->width, aptr->width);
      break;
    case OCTAGON:
      pcb_fprintf (f, metric ? "%%AMOCT%d*5,0,8,0,0,%.3`mm,22.5*%%\r\n"
	       "%%ADD%dOCT%d*%%\r\n" : "%%AMOCT%d*5,0,8,0,0,%.3`mm,22.5*%%\r\n"
	       "%%ADD%dOCT%d*%%\r\n", aptr->dCode,
	       (Coord) ((double) aptr->width / COS_22_5_DEGREE), aptr->dCode,
	       aptr->dCode);
      break;
#if 0
    case THERMAL:
      fprintf (f, "%%AMTHERM%d*7,0,0,%.4f,%.4f,%.4f,45*%%\r\n"
	       "%%ADD%dTHERM%d*%%\r\n", dCode, gap / 100000.0,
	       width / 100000.0, finger / 100000.0, dCode, dCode);
      break;
    case ROUNDCLEAR:
      fprintf (f, "%%ADD%dC,%.4fX%.4f*%%\r\n",
	       dCode, gap / 100000.0, width / 100000.0);
      break;
    case SQUARECLEAR:
      fprintf (f, "%%ADD%dR,%.4fX%.4fX%.4fX%.4f*%%\r\n",
	       dCode, gap / 100000.0, gap / 100000.0,
	       width / 100000.0, width / 100000.0);
      break;
#else
    default:
      break;
#endif
    }
}

/* Set the aperture list for the current layer,
 * expanding the list buffer if needed  */
static ApertureList *
setLayerApertureList (int layer_idx)
{
  if (layer_idx >= layer_list_max)
    {
      int i = layer_list_max;
      layer_list_max  = 2 * (layer_idx + 1);
      layer_aptr_list = (ApertureList *)
                        realloc (layer_aptr_list, layer_list_max * sizeof (*layer_aptr_list));
      for (; i < layer_list_max; ++i)
        initApertureList (&layer_aptr_list[i]);
    }
  curr_aptr_list = &layer_aptr_list[layer_idx];
  return curr_aptr_list;
}

/* --------------------------------------------------------------------------- */

static HID gerber_hid;
static HID_DRAW gerber_graphics;

typedef struct hid_gc_struct
{
  EndCapStyle cap;
  int width;
  int color;
  int erase;
  int drill;
} hid_gc_struct;

static FILE *f = NULL;
static char *filename = NULL;
static char *filesuff = NULL;
static char *layername = NULL;
static int lncount = 0;

static int finding_apertures = 0;
static int pagecount = 0;
static int linewidth = -1;
static int lastgroup = -1;
static int lastcap = -1;
static int print_group[MAX_GROUP];
static int print_layer[MAX_ALL_LAYER];
static int lastX, lastY;	/* the last X and Y coordinate */

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

static const char *name_style_names[] = {
#define NAME_STYLE_FIXED 0
  "fixed",
#define NAME_STYLE_SINGLE 1
  "single",
#define NAME_STYLE_FIRST 2
  "first",
#define NAME_STYLE_EAGLE 3
  "eagle",
#define NAME_STYLE_HACKVANA 4
  "hackvana",
#define NAME_STYLE_OSHPARK 5
  "oshpark",
  NULL
};

static HID_Attribute gerber_options[] = {

/* %start-doc options "90 Gerber Export"
@ftable @code
@item --gerberfile <string>
Gerber output file prefix. Parameter @code{<string>} can include a path.
@end ftable
%end-doc
*/
  {"gerberfile", "Gerber output file base",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_gerberfile 0

/* %start-doc options "90 Gerber Export"
@ftable @code
@item --all-layers
Output contains all layers, even empty ones.
@end ftable
%end-doc
*/
  {"all-layers", "Output all layers, even empty ones",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_all_layers 1

/* %start-doc options "90 Gerber Export"
@ftable @code
@item --verbose
Print file names and aperture counts on stdout.
@end ftable
%end-doc
*/
  {"verbose", "Print file names and aperture counts on stdout",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_verbose 2

/* %start-doc options "90 Gerber Export"
@ftable @code
@item --metric
Generate metric Gerber and drill files
@end ftable
%end-doc
*/
  {"metric", "Generate metric Gerber and drill files",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_metric 3

/* %start-doc options "90 Gerber Export"
@ftable @code
@item --copy-outline <string>
Copy the outline onto other layers.
Parameter @code{<string>} can be @samp{none}, @samp{mask},
@samp{silk} or @samp{all}.
@end ftable
%end-doc
*/
  {"copy-outline", "Copy outline onto other layers",
   HID_Enum, 0, 0, {0, 0, 0}, copy_outline_names, 0},
#define HA_copy_outline 4

/* %start-doc options "90 Gerber Export"
@ftable @code
@item --name-style <string>
Naming style for individual gerber files.
Parameter @code{<string>} can be @samp{fixed}, @samp{single},
@samp{first}, @samp{eagle}, @samp{hackvana} or @samp{oshpark}.
@end ftable
%end-doc
*/
  {"name-style", "Naming style for individual gerber files",
   HID_Enum, 0, 0, {0, 0, 0}, name_style_names, 0},
#define HA_name_style 5
};

#define NUM_OPTIONS (sizeof(gerber_options)/sizeof(gerber_options[0]))

static HID_Attr_Val gerber_values[NUM_OPTIONS];

static HID_Attribute *
gerber_get_export_options (int *n)
{
  static char *last_made_filename = NULL;
  if (PCB) derive_default_filename(PCB->Filename, &gerber_options[HA_gerberfile], "", &last_made_filename);

  if (n)
    *n = NUM_OPTIONS;
  return gerber_options;
}

static int
layer_stack_sort (const void *va, const void *vb)
{
  int a_layer = *(int *) va;
  int b_layer = *(int *) vb;
  int a_group = GetLayerGroupNumberByNumber (a_layer);
  int b_group = GetLayerGroupNumberByNumber (b_layer);

  if (b_group != a_group)
    return b_group - a_group;

  return b_layer - a_layer;
}

static void
maybe_close_f (FILE *f)
{
  if (f)
    {
      if (was_drill)
	fprintf (f, "M30\r\n");
      else
	fprintf (f, "M02*\r\n");
      fclose (f);
    }
}

static BoxType region;

/* Very similar to layer_type_to_file_name() but appends only a
   three-character suffix compatible with Eagle's defaults.  */
static void
assign_eagle_file_suffix (char *dest, int idx)
{
  int group;
  int nlayers;
  char *suff = "out";

  switch (idx)
    {
    case SL (SILK,      TOP):    suff = "plc"; break;
    case SL (SILK,      BOTTOM): suff = "pls"; break;
    case SL (MASK,      TOP):    suff = "stc"; break;
    case SL (MASK,      BOTTOM): suff = "sts"; break;
    case SL (PDRILL,    0):      suff = "drd"; break;
    case SL (UDRILL,    0):      suff = "dru"; break;
    case SL (PASTE,     TOP):    suff = "crc"; break;
    case SL (PASTE,     BOTTOM): suff = "crs"; break;
    case SL (INVISIBLE, 0):      suff = "inv"; break;
    case SL (FAB,       0):      suff = "fab"; break;
    case SL (ASSY,      TOP):    suff = "ast"; break;
    case SL (ASSY,      BOTTOM): suff = "asb"; break;

    default:
      group = GetLayerGroupNumberByNumber(idx);
      nlayers = PCB->LayerGroups.Number[group];
      if (group == GetLayerGroupNumberBySide(TOP_SIDE)) /* Component */
	{
	  suff = "cmp";
	}
      else if (group == GetLayerGroupNumberBySide(BOTTOM_SIDE)) /* Solder */
	{
	  suff = "sol";
	}
      else if (nlayers == 1
	       && (strcmp (PCB->Data->Layer[idx].Name, "route") == 0 ||
		   strcmp (PCB->Data->Layer[idx].Name, "outline") == 0))
	{
	  suff = "oln";
	}
      else
	{
	  static char buf[20];
	  sprintf (buf, "ly%d", group);
	  suff = buf;
	}
      break;
    }

  strcpy (dest, suff);
}

/* Very similar to layer_type_to_file_name() but appends only a
   three-character suffix compatible with Hackvana's naming requirements  */
static void
assign_hackvana_file_suffix (char *dest, int idx)
{
  int group;
  int nlayers;
  char *suff = "defau.out";

  switch (idx)
    {
    case SL (SILK,      TOP):    suff = "gto"; break;
    case SL (SILK,      BOTTOM): suff = "gbo"; break;
    case SL (MASK,      TOP):    suff = "gts"; break;
    case SL (MASK,      BOTTOM): suff = "gbs"; break;
    case SL (PDRILL,    0):      suff = "drl"; break;
    case SL (UDRILL,    0):
      suff = "_NPTH.drl";
      break;
    case SL (PASTE,     TOP):    suff = "gtp"; break;
    case SL (PASTE,     BOTTOM): suff = "gbp"; break;
    case SL (INVISIBLE, 0):      suff = "inv"; break;
    case SL (FAB,       0):      suff = "fab"; break;
    case SL (ASSY,      TOP):    suff = "ast"; break;
    case SL (ASSY,      BOTTOM): suff = "asb"; break;

    default:
      group = GetLayerGroupNumberByNumber(idx);
      nlayers = PCB->LayerGroups.Number[group];
      if (group == GetLayerGroupNumberBySide(TOP_SIDE))
      {
        suff = "gtl";
      }
      else if (group == GetLayerGroupNumberBySide(BOTTOM_SIDE))
      {
        suff = "gbl";
      }
      else if (nlayers == 1
        && (strcmp (PCB->Data->Layer[idx].Name, "route") == 0 ||
            strcmp (PCB->Data->Layer[idx].Name, "outline") == 0))
      {
        suff = "gm1";
      }
      else
      {
        static char buf[20];
        sprintf (buf, "g%d", group);
        suff = buf;
      }
      break;
    }

  strcpy (dest, suff);
}

/*!
 * \brief Very similar to layer_type_to_file_name() but appends only a
 * three-character suffix compatible with OSH Park's naming requirements.
 *
 * \note The unplated drill file with the ".TXT" extension will be
 * merged when the zip file is processed by OSH Park (after uploading).
 *
 * \warning Blind and buried vias are not supported by OSH Park.
 *
 * \warning Currently 4 layer boards is the maximum OSH Park supports.
 */
static void
assign_oshpark_file_suffix (char *dest, int idx)
{
  int group;
  int nlayers;
  char *suff = "default.out";

  switch (idx)
    {
    case SL (SILK,      TOP):    suff = "GTO"; break;
    case SL (SILK,      BOTTOM): suff = "GBO"; break;
    case SL (MASK,      TOP):    suff = "GTS"; break;
    case SL (MASK,      BOTTOM): suff = "GBS"; break;
    case SL (PDRILL,    0):      suff = "XLN"; break;
    case SL (UDRILL,    0):      suff = "TXT"; break;
    case SL (PASTE,     TOP):    suff = "gtp"; break;
    case SL (PASTE,     BOTTOM): suff = "gbp"; break;
    case SL (INVISIBLE, 0):      suff = "inv"; break;
    case SL (FAB,       0):      suff = "fab"; break;
    case SL (ASSY,      TOP):    suff = "ast"; break;
    case SL (ASSY,      BOTTOM): suff = "asb"; break;

    default:
      group = GetLayerGroupNumberByNumber(idx);
      nlayers = PCB->LayerGroups.Number[group];
      if (group == GetLayerGroupNumberBySide(TOP_SIDE))
      {
        suff = "GTL";
      }
      else if (group == GetLayerGroupNumberBySide(BOTTOM_SIDE))
      {
        suff = "GBL";
      }
      else if (nlayers == 1
        && (strcmp (PCB->Data->Layer[idx].Name, "route") == 0 ||
            strcmp (PCB->Data->Layer[idx].Name, "outline") == 0))
      {
        suff = "GKO";
      }
      else
      {
        static char buf[20];
        sprintf (buf, "G%dL", group);
        suff = buf;
      }
      break;
    }

  strcpy (dest, suff);
}

static void
assign_file_suffix (char *dest, int idx, const char *layer_name)
{
  int fns_style;
  const char *sext = ".gbr";

  switch (name_style)
    {
    default:
    case NAME_STYLE_FIXED:  fns_style = FNS_fixed;  break;
    case NAME_STYLE_SINGLE: fns_style = FNS_single; break;
    case NAME_STYLE_FIRST:  fns_style = FNS_first;  break;
    case NAME_STYLE_EAGLE:
      assign_eagle_file_suffix (dest, idx);
      return;
    case NAME_STYLE_HACKVANA:
      assign_hackvana_file_suffix (dest, idx);
      return;
    case NAME_STYLE_OSHPARK:
      assign_oshpark_file_suffix (dest, idx);
      return;
    }

  switch (idx)
    {
    case SL (PDRILL, 0):
      sext = ".cnc";
      break;
    case SL (UDRILL, 0):
      sext = ".cnc";
      break;
    }

  strcpy (dest, layer_type_to_file_name_ex (idx, fns_style, layer_name));
  strcat (dest, sext);
}

static void
gerber_do_export (HID_Attr_Val * options)
{
  const char *fnbase;
  int i;
  static int saved_layer_stack[MAX_LAYER];
  int save_ons[MAX_ALL_LAYER];
  FlagType save_thindraw;

  save_thindraw = PCB->Flags;
  CLEAR_FLAG(THINDRAWFLAG, PCB);
  CLEAR_FLAG(THINDRAWPOLYFLAG, PCB);
  CLEAR_FLAG(CHECKPLANESFLAG, PCB);

  if (!options)
    {
      gerber_get_export_options (NULL);
      for (i = 0; i < NUM_OPTIONS; i++)
	gerber_values[i] = gerber_options[i].default_val;
      options = gerber_values;
    }

  fnbase = options[HA_gerberfile].str_value;
  if (!fnbase)
    fnbase = "pcb-out";

  verbose = options[HA_verbose].int_value;
  metric = options[HA_metric].int_value;
  if (metric) {
	  x_convspec = "X%.0mu";
	  y_convspec = "Y%.0mu";
  } else {
	  x_convspec = "X%.0mc";
	  y_convspec = "Y%.0mc";
  }
  all_layers = options[HA_all_layers].int_value;

  copy_outline_mode = options[HA_copy_outline].int_value;
  name_style = options[HA_name_style].int_value;

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
  filename = (char *)realloc (filename, i + 40);
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
  print_group[GetLayerGroupNumberBySide (BOTTOM_SIDE)] = 1;
  print_group[GetLayerGroupNumberBySide (TOP_SIDE)] = 1;
  for (i = 0; i < max_copper_layer; i++)
    if (print_group[GetLayerGroupNumberByNumber (i)])
      print_layer[i] = 1;

  memcpy (saved_layer_stack, LayerStack, sizeof (LayerStack));
  qsort (LayerStack, max_copper_layer, sizeof (LayerStack[0]), layer_stack_sort);
  linewidth = -1;
  lastcap = -1;
  lastgroup = -1;

  region.X1 = 0;
  region.Y1 = 0;
  region.X2 = PCB->MaxWidth;
  region.Y2 = PCB->MaxHeight;

  pagecount = 1;
  resetApertures ();

  lastgroup = -1;
  layer_list_idx = 0;
  finding_apertures = 1;
  hid_expose_callback (&gerber_hid, &region, 0);

  layer_list_idx = 0;
  finding_apertures = 0;
  hid_expose_callback (&gerber_hid, &region, 0);

  memcpy (LayerStack, saved_layer_stack, sizeof (LayerStack));

  maybe_close_f (f);
  f = NULL;
  hid_restore_layer_ons (save_ons);
  PCB->Flags = save_thindraw;
}

static void
gerber_parse_arguments (int *argc, char ***argv)
{
  hid_register_attributes (gerber_options, NUM_OPTIONS);
  hid_parse_command_line (argc, argv);
}

static int
drill_sort (const void *va, const void *vb)
{
  PendingDrills *a = (PendingDrills *) va;
  PendingDrills *b = (PendingDrills *) vb;
  if (a->diam != b->diam)
    return a->diam - b->diam;
  if (a->x != b->x)
    return a->x - b->x;
  return a->y - b->y;
}

static int
gerber_set_layer (const char *name, int group, int empty)
{
  int want_outline;
  char *cp;
  int idx = (group >= 0
	     && group <
	     max_group) ? PCB->LayerGroups.Entries[group][0] : group;

  if (name == NULL)
    name = PCB->Data->Layer[idx].Name;

  if (idx >= 0 && idx < max_copper_layer && !print_layer[idx])
    return 0;

  if (strcmp (name, "invisible") == 0)
    return 0;
  if (SL_TYPE (idx) == SL_ASSY)
    return 0;

  flash_drills = 0;
  if (strcmp (name, "outline") == 0 ||
      strcmp (name, "route") == 0)
    flash_drills = 1;

  if (is_drill && n_pending_drills)
    {
      int i;
      /* dump pending drills in sequence */
      qsort (pending_drills, n_pending_drills, sizeof (pending_drills[0]),
	     drill_sort);
      for (i = 0; i < n_pending_drills; i++)
	{
	  if (i == 0 || pending_drills[i].diam != pending_drills[i - 1].diam)
	    {
	      Aperture *ap = findAperture (curr_aptr_list, pending_drills[i].diam, ROUND);
	      fprintf (f, "T%02d\r\n", ap->dCode);
	    }
	  pcb_fprintf (f, metric ? "X%06.0muY%06.0mu\r\n" : "X%06.0mtY%06.0mt\r\n",
		   gerberDrX (PCB, pending_drills[i].x),
		   gerberDrY (PCB, pending_drills[i].y));
	}
      free (pending_drills);
      n_pending_drills = max_pending_drills = 0;
      pending_drills = NULL;
    }

  is_drill = (SL_TYPE (idx) == SL_PDRILL || SL_TYPE (idx) == SL_UDRILL);
  is_mask = (SL_TYPE (idx) == SL_MASK);
  current_mask = HID_MASK_OFF;
#if 0
  printf ("Layer %s group %d drill %d mask %d\n", name, group, is_drill,
	  is_mask);
#endif

  if (group < 0 || group != lastgroup)
    {
      time_t currenttime;
      char utcTime[64];
#ifdef HAVE_GETPWUID
      struct passwd *pwentry;
#endif
      ApertureList *aptr_list;
      Aperture *search;

      lastgroup = group;
      lastX = -1;
      lastY = -1;
      linewidth = -1;
      lastcap = -1;

      aptr_list = setLayerApertureList (layer_list_idx++);

      if (finding_apertures)
	goto emit_outline;

      if (aptr_list->count == 0 && !all_layers)
	return 0;

      maybe_close_f (f);
      f = NULL;

      pagecount++;
      assign_file_suffix (filesuff, idx, name);
      f = fopen (filename, "wb");   /* Binary needed to force CR-LF */
      if (f == NULL) 
	{
	  Message ( "Error:  Could not open %s for writing.\n", filename);
	  return 1;
	}

      was_drill = is_drill;

      if (verbose)
	{
	  int c = aptr_list->count;
	  printf ("Gerber: %d aperture%s in %s\n", c,
		  c == 1 ? "" : "s", filename);
	}

      if (is_drill)
	{
	  /* We omit the ,TZ here because we are not omitting trailing zeros.  Our format is
	     always six-digit 0.1 mil or µm resolution (i.e. 001100 = 0.11" or 1.1mm)*/
	  fprintf (f, "M48\r\n");
	  fprintf (f, metric ? "METRIC,000.000\r\n" : "INCH\r\n");
	  for (search = aptr_list->data; search; search = search->next)
		  pcb_fprintf (f, metric ? "T%02dC%.3`mm\r\n" : "T%02dC%.3`mi\r\n", search->dCode, search->width);
	  fprintf (f, "%%\r\n");
	  /* FIXME */
	  return 1;
	}

      fprintf (f, "G04 start of page %d for group %d idx %d *\r\n",
	       pagecount, group, idx);

      /* Create a portable timestamp. */
      currenttime = time (NULL);
      {
	/* avoid gcc complaints */
	const char *fmt = "%c UTC";
	strftime (utcTime, sizeof utcTime, fmt, gmtime (&currenttime));
      }
      /* Print a cute file header at the beginning of each file. */
      fprintf (f, "G04 Title: %s, %s *\r\n", UNKNOWN (PCB->Name),
	       UNKNOWN (name));
      fprintf (f, "G04 Creator: %s " VERSION " *\r\n", Progname);
      fprintf (f, "G04 CreationDate: %s *\r\n", utcTime);

#ifdef HAVE_GETPWUID
      /* ID the user. */
      pwentry = getpwuid (getuid ());
      fprintf (f, "G04 For: %s *\r\n", pwentry->pw_name);
#endif

      fprintf (f, "G04 Format: Gerber/RS-274X *\r\n");
      pcb_fprintf (f, metric ? "G04 PCB-Dimensions (mm): %.2mm %.2mm *\r\n" :
	       "G04 PCB-Dimensions (mil): %.2ml %.2ml *\r\n",
	       PCB->MaxWidth, PCB->MaxHeight);
      fprintf (f, "G04 PCB-Coordinate-Origin: lower left *\r\n");

      /* Signal data in inches. */
      fprintf (f, metric ? "%%MOMM*%%\r\n" : "%%MOIN*%%\r\n");

      /* Signal Leading zero suppression, Absolute Data, 2.5 format in inch, 4.3 in mm */
      fprintf (f, metric ? "%%FSLAX43Y43*%%\r\n" : "%%FSLAX25Y25*%%\r\n");

      /* build a legal identifier. */
      if (layername)
	free (layername);
      layername = strdup (filesuff);
      if (strrchr (layername, '.'))
	* strrchr (layername, '.') = 0;

      for (cp=layername; *cp; cp++)
	{
	  if (isalnum((int) *cp))
	    *cp = toupper((int) *cp);
	  else
	    *cp = '_';
	}
      fprintf (f, "%%LN%s*%%\r\n", layername);
      lncount = 1;

      for (search = aptr_list->data; search; search = search->next)
        fprintAperture(f, search);
      if (aptr_list->count == 0)
	/* We need to put *something* in the file to make it be parsed
	   as RS-274X instead of RS-274D. */
	fprintf (f, "%%ADD11C,0.0100*%%\r\n");
    }

 emit_outline:
  /* If we're printing a copper layer other than the outline layer,
     and we want to "print outlines", and we have an outline layer,
     print the outline layer on this layer also.  */
  want_outline = 0;
  if (copy_outline_mode == COPY_OUTLINE_MASK
      && SL_TYPE (idx) == SL_MASK)
    want_outline = 1;
  if (copy_outline_mode == COPY_OUTLINE_SILK
      && SL_TYPE (idx) == SL_SILK)
    want_outline = 1;
  if (copy_outline_mode == COPY_OUTLINE_ALL
      && (SL_TYPE (idx) == SL_SILK
	  || SL_TYPE (idx) == SL_MASK
	  || SL_TYPE (idx) == SL_FAB
	  || SL_TYPE (idx) == SL_ASSY
	  || SL_TYPE (idx) == 0))
    want_outline = 1;

  if (want_outline
      && strcmp (name, "outline")
      && strcmp (name, "route"))
    {
      if (outline_layer
	  && outline_layer != PCB->Data->Layer+idx)
	DrawLayer (outline_layer, &region);
      else if (!outline_layer)
	{
	  hidGC gc = gui->graphics->make_gc ();
	  printf("name %s idx %d\n", name, idx);
	  if (SL_TYPE (idx) == SL_SILK)
	    gui->graphics->set_line_width (gc, PCB->minSlk);
	  else if (group >= 0)
	    gui->graphics->set_line_width (gc, PCB->minWid);
	  else
	    gui->graphics->set_line_width (gc, AUTO_OUTLINE_WIDTH);
	  gui->graphics->draw_line (gc, 0, 0, PCB->MaxWidth, 0);
	  gui->graphics->draw_line (gc, 0, 0, 0, PCB->MaxHeight);
	  gui->graphics->draw_line (gc, PCB->MaxWidth, 0, PCB->MaxWidth, PCB->MaxHeight);
	  gui->graphics->draw_line (gc, 0, PCB->MaxHeight, PCB->MaxWidth, PCB->MaxHeight);
	  gui->graphics->destroy_gc (gc);
	}
    }

  return 1;
}

static hidGC
gerber_make_gc (void)
{
  hidGC rv = (hidGC) calloc (1, sizeof (*rv));
  rv->cap = Trace_Cap;
  return rv;
}

static void
gerber_destroy_gc (hidGC gc)
{
  free (gc);
}

static void
gerber_use_mask (enum mask_mode mode)
{
  current_mask = mode;
}

static void
gerber_set_color (hidGC gc, const char *name)
{
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
}

static void
gerber_set_line_cap (hidGC gc, EndCapStyle style)
{
  gc->cap = style;
}

static void
gerber_set_line_width (hidGC gc, Coord width)
{
  gc->width = width;
}

static void
gerber_set_draw_xor (hidGC gc, int xor_)
{
  ;
}

static void
use_gc (hidGC gc, int radius)
{
  if (radius)
    {
      radius *= 2;
      if (radius != linewidth || lastcap != Round_Cap)
	{
	  Aperture *aptr = findAperture (curr_aptr_list, radius, ROUND);
	  if (aptr == NULL)
	    pcb_fprintf (stderr, "error: aperture for radius %$mS type ROUND is null\n", radius);
	  else if (f && !is_drill)
	    fprintf (f, "G54D%d*", aptr->dCode);
	  linewidth = radius;
	  lastcap = Round_Cap;
	}
    }
  else if (linewidth != gc->width || lastcap != gc->cap)
    {
      Aperture *aptr;
      ApertureShape shape;

      linewidth = gc->width;
      lastcap = gc->cap;
      switch (gc->cap)
	{
	case Round_Cap:
	case Trace_Cap:
	  shape = ROUND;
	  break;
	default:
	case Square_Cap:
	  shape = SQUARE;
	  break;
	}
      aptr = findAperture (curr_aptr_list, linewidth, shape);
      if (aptr == NULL)
        pcb_fprintf (stderr, "error: aperture for width %$mS type %s is null\n",
                 linewidth, shape == ROUND ? "ROUND" : "SQUARE");
      else if (f)
	fprintf (f, "G54D%d*", aptr->dCode);
    }
}

static void
gerber_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  gerber_draw_line (gc, x1, y1, x1, y2);
  gerber_draw_line (gc, x1, y1, x2, y1);
  gerber_draw_line (gc, x1, y2, x2, y2);
  gerber_draw_line (gc, x2, y1, x2, y2);
}

static void
gerber_draw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  bool m = false;

  if (x1 != x2 && y1 != y2 && gc->cap == Square_Cap)
    {
      Coord x[5], y[5];
      double tx, ty, theta;

      theta = atan2 (y2-y1, x2-x1);

      /* T is a vector half a thickness long, in the direction of
	 one of the corners.  */
      tx = gc->width / 2.0 * cos (theta + M_PI/4) * sqrt(2.0);
      ty = gc->width / 2.0 * sin (theta + M_PI/4) * sqrt(2.0);

      x[0] = x1 - tx;      y[0] = y1 - ty;
      x[1] = x2 + ty;      y[1] = y2 - tx;
      x[2] = x2 + tx;      y[2] = y2 + ty;
      x[3] = x1 - ty;      y[3] = y1 + tx;

      x[4] = x[0]; y[4] = y[0];
      gerber_fill_polygon (gc, 5, x, y);
      return;
    }

  use_gc (gc, 0);
  if (!f)
    return;

  if (x1 != lastX)
    {
      m = true;
      lastX = x1;
      print_xcoord (f, PCB, lastX);
    }
  if (y1 != lastY)
    {
      m = true;
      lastY = y1;
      print_ycoord (f, PCB, lastY);
    }
  if ((x1 == x2) && (y1 == y2))
    fprintf (f, "D03*\r\n");
  else
    {
      if (m)
	fprintf (f, "D02*");
      if (x2 != lastX)
	{
	  lastX = x2;
	  print_xcoord (f, PCB, lastX);
	}
      if (y2 != lastY)
	{
	  lastY = y2;
	  print_ycoord (f, PCB, lastY);
	}
      fprintf (f, "D01*\r\n");
    }

}

static void
gerber_draw_arc (hidGC gc, Coord cx, Coord cy, Coord width, Coord height,
		 Angle start_angle, Angle delta_angle)
{
  bool m = false;
  double arcStartX, arcStopX, arcStartY, arcStopY;

  /* we never draw zero-width lines */
  if (gc->width == 0)
    return;

  use_gc (gc, 0);
  if (!f)
    return;

  arcStartX = cx - width * cos (TO_RADIANS (start_angle));
  arcStartY = cy + height * sin (TO_RADIANS (start_angle));

  /* I checked three different gerber viewers, and they all disagreed
     on how ellipses should be drawn.  The spec just calls G74/G75
     "circular interpolation" so there's a chance it just doesn't
     support ellipses at all.  Thus, we draw them out with line
     segments.  Note that most arcs in pcb are circles anyway.  */
  if (width != height)
    {
      double step, angle;
      Coord max = width > height ? width : height;
      Coord minr = max - gc->width / 10;
      int nsteps;
      Coord x0, y0, x1, y1;

      if (minr >= max)
	minr = max - 1;
      step = acos((double)minr/(double)max) * 180.0/M_PI;
      if (step > 5)
	step = 5;
      nsteps = abs(delta_angle) / step + 1;
      step = (double)delta_angle / nsteps;

      x0 = arcStartX;
      y0 = arcStartY;
      angle = start_angle;
      while (nsteps > 0)
	{
	  nsteps --;
	  x1 = cx - width * cos (TO_RADIANS (angle+step));
	  y1 = cy + height * sin (TO_RADIANS (angle+step));
	  gerber_draw_line (gc, x0, y0, x1, y1);
	  x0 = x1;
	  y0 = y1;
	  angle += step;
	}
      return;
    }

  arcStopX = cx - width * cos (TO_RADIANS (start_angle + delta_angle));
  arcStopY = cy + height * sin (TO_RADIANS (start_angle + delta_angle));
  if (arcStartX != lastX)
    {
      m = true;
      lastX = arcStartX;
      print_xcoord (f, PCB, lastX);
    }
  if (arcStartY != lastY)
    {
      m = true;
      lastY = arcStartY;
      print_ycoord (f, PCB, lastY);
    }
  if (m)
    fprintf (f, "D02*");
  pcb_fprintf (f,
	   metric ? "G75*G0%1dX%.0muY%.0muI%.0muJ%.0muD01*G01*\r\n" :
	   "G75*G0%1dX%.0mcY%.0mcI%.0mcJ%.0mcD01*G01*\r\n",
	   (delta_angle < 0) ? 2 : 3,
	   gerberX (PCB, arcStopX), gerberY (PCB, arcStopY),
	   gerberXOffset (PCB, cx - arcStartX),
	   gerberYOffset (PCB, cy - arcStartY));
  lastX = arcStopX;
  lastY = arcStopY;
}

static void
gerber_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius)
{
  if (radius <= 0)
    return;
  if (is_drill)
    radius = 50 * round (radius / 50.0);
  use_gc (gc, radius);
  if (!f)
    return;
  if (is_drill)
    {
      if (n_pending_drills >= max_pending_drills)
	{
	  max_pending_drills += 100;
	  pending_drills = (PendingDrills *) realloc(pending_drills,
	                                             max_pending_drills *
	                                             sizeof (pending_drills[0]));
	}
      pending_drills[n_pending_drills].x = cx;
      pending_drills[n_pending_drills].y = cy;
      pending_drills[n_pending_drills].diam = radius * 2;
      n_pending_drills++;
      return;
    }
  else if (gc->drill && !flash_drills)
    return;
  if (cx != lastX)
    {
      lastX = cx;
      print_xcoord (f, PCB, lastX);
    }
  if (cy != lastY)
    {
      lastY = cy;
      print_ycoord (f, PCB, lastY);
    }
  fprintf (f, "D03*\r\n");
}

static void
gerber_fill_polygon (hidGC gc, int n_coords, Coord *x, Coord *y)
{
  bool m = false;
  int i;
  int firstTime = 1;
  Coord startX = 0, startY = 0;

  if (is_mask && current_mask == HID_MASK_BEFORE)
    return;

  use_gc (gc, 10 * 100);
  if (!f)
    return;
  fprintf (f, "G36*\r\n");
  for (i = 0; i < n_coords; i++)
    {
      if (x[i] != lastX)
	{
	  m = true;
	  lastX = x[i];
	  print_xcoord (f, PCB, lastX);
	}
      if (y[i] != lastY)
	{
	  m = true;
	  lastY = y[i];
	  print_ycoord (f, PCB, lastY);
	}
      if (firstTime)
	{
	  firstTime = 0;
	  startX = x[i];
	  startY = y[i];
	  if (m)
	    fprintf (f, "D02*");
	}
      else if (m)
	fprintf (f, "D01*\r\n");
      m = false;
    }
  if (startX != lastX)
    {
      m = true;
      lastX = startX;
      print_xcoord (f, PCB, startX);
    }
  if (startY != lastY)
    {
      m = true;
      lastY = startY;
      print_ycoord (f, PCB, lastY);
    }
  if (m)
    fprintf (f, "D01*\r\n");
  fprintf (f, "G37*\r\n");
}

static void
gerber_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
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
  gerber_fill_polygon (gc, 5, x, y);
}

static void
gerber_calibrate (double xval, double yval)
{
  CRASH;
}

static void
gerber_set_crosshair (int x, int y, int action)
{
}

void
hid_gerber_init ()
{
  memset (&gerber_hid, 0, sizeof (gerber_hid));
  memset (&gerber_graphics, 0, sizeof (gerber_graphics));

  common_nogui_init (&gerber_hid);
  common_draw_helpers_init (&gerber_graphics);

  gerber_hid.struct_size         = sizeof (gerber_hid);
  gerber_hid.name                = "gerber";
  gerber_hid.description         = "RS-274X (Gerber) export";
  gerber_hid.exporter            = 1;

  gerber_hid.get_export_options  = gerber_get_export_options;
  gerber_hid.do_export           = gerber_do_export;
  gerber_hid.parse_arguments     = gerber_parse_arguments;
  gerber_hid.set_layer           = gerber_set_layer;
  gerber_hid.calibrate           = gerber_calibrate;
  gerber_hid.set_crosshair       = gerber_set_crosshair;

  gerber_hid.graphics            = &gerber_graphics;

  gerber_graphics.make_gc        = gerber_make_gc;
  gerber_graphics.destroy_gc     = gerber_destroy_gc;
  gerber_graphics.use_mask       = gerber_use_mask;
  gerber_graphics.set_color      = gerber_set_color;
  gerber_graphics.set_line_cap   = gerber_set_line_cap;
  gerber_graphics.set_line_width = gerber_set_line_width;
  gerber_graphics.set_draw_xor   = gerber_set_draw_xor;
  gerber_graphics.draw_line      = gerber_draw_line;
  gerber_graphics.draw_arc       = gerber_draw_arc;
  gerber_graphics.draw_rect      = gerber_draw_rect;
  gerber_graphics.fill_circle    = gerber_fill_circle;
  gerber_graphics.fill_polygon   = gerber_fill_polygon;
  gerber_graphics.fill_rect      = gerber_fill_rect;

  hid_register_hid (&gerber_hid);
}
