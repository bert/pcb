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

#include "hid.h"
#include "../hidint.h"
#include "hid/common/hidnogui.h"
#include "hid/common/draw_helpers.h"
#include "hid/common/hidinit.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

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
static void gerber_use_mask (int use_it);
static void gerber_set_color (hidGC gc, const char *name);
static void gerber_set_line_cap (hidGC gc, EndCapStyle style);
static void gerber_set_line_width (hidGC gc, int width);
static void gerber_set_draw_xor (hidGC gc, int _xor);
static void gerber_draw_line (hidGC gc, int x1, int y1, int x2, int y2);
static void gerber_draw_arc (hidGC gc, int cx, int cy, int width, int height, int start_angle, int delta_angle);
static void gerber_draw_rect (hidGC gc, int x1, int y1, int x2, int y2);
static void gerber_fill_circle (hidGC gc, int cx, int cy, int radius);
static void gerber_fill_rect (hidGC gc, int x1, int y1, int x2, int y2);
static void gerber_calibrate (double xval, double yval);
static void gerber_set_crosshair (int x, int y, int action);
static void gerber_fill_polygon (hidGC gc, int n_coords, int *x, int *y);

/*----------------------------------------------------------------------------*/
/* Utility routines                                                           */
/*----------------------------------------------------------------------------*/

/* These are for films */
#define gerberX(pcb, x) ((long) ((x)))
#define gerberY(pcb, y) ((long) (((pcb)->MaxHeight - (y))))
#define gerberXOffset(pcb, x) ((long) ((x)))
#define gerberYOffset(pcb, y) ((long) (-(y)))

/* These are for drills */
#define gerberDrX(pcb, x) ((long) ((x)/10))
#define gerberDrY(pcb, y) ((long) (((pcb)->MaxHeight - (y)))/10)
#define gerberDrXOffset(pcb, x) ((long) ((x)/10))
#define gerberDrYOffset(pcb, y) ((long) (-(y))/10)

/*----------------------------------------------------------------------------*/
/* Private data structures                                                    */
/*----------------------------------------------------------------------------*/

static int verbose;
static int all_layers;
static int is_mask, was_drill;
static int is_drill;
static int current_mask;
static int flash_drills;
static int copy_outline_mode;
static LayerTypePtr outline_layer;

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

typedef struct Aperture
{
  int dCode;			/* The RS-274X D code */
  int apertureSize;		/* Size in mils */
  ApertureShape apertureShape;	/* ROUND/SQUARE etc */
}
Aperture;

/* This is added to the global aperture array indexes to get gerber
   dcode and macro numbers.  */
#define DCODE_BASE 11

typedef struct
{
  int some_apertures;
  int aperture_used[GBX_MAXAPERTURECOUNT];
} Apertures;

static int global_aperture_count;
static int global_aperture_sizes[GBX_MAXAPERTURECOUNT];
static ApertureShape global_aperture_shapes[GBX_MAXAPERTURECOUNT];

static Apertures *layerapps = 0;
static Apertures *curapp;
static int n_layerapps = 0;
static int c_layerapps = 0;

typedef struct
{
  int diam;
  int x;
  int y;
} PendingDrills;
PendingDrills *pending_drills = 0;
int n_pending_drills = 0, max_pending_drills = 0;

/*----------------------------------------------------------------------------*/
/* Defined Constants                                                          */
/*----------------------------------------------------------------------------*/
#define AUTO_OUTLINE_WIDTH MIL_TO_COORD(8)       /* Auto-geneated outline width of 8 mils */


/*----------------------------------------------------------------------------*/
/* Aperture Routines                                                          */
/*----------------------------------------------------------------------------*/



static int
findApertureCode (int width, ApertureShape shape)
{
  int i;

  /* we never draw zero-width lines */
  if (width == 0)
    return (0);

  /* Search for an appropriate aperture. */

  for (i = 0; i < global_aperture_count; i++)
    {
      if (global_aperture_sizes[i] == width
	  && global_aperture_shapes[i] == shape)
	{
	  curapp->aperture_used[i] = 1;
	  curapp->some_apertures = 1;
	  return i + DCODE_BASE;
	}
    }

  /* Not found, create a new aperture and add it to the list */
  if (global_aperture_count < GBX_MAXAPERTURECOUNT)
    {
      i = global_aperture_count ++;
      global_aperture_sizes[i] = width;
      global_aperture_shapes[i] = shape;
      curapp->aperture_used[i] = 1;
      curapp->some_apertures = 1;
      return i + DCODE_BASE;
    }
  else
    {
      Message (_("Error, too many apertures needed for Gerber file.\n"));
      return (10);
    }
}

static void
printAperture(FILE *f, int i)
{
  int dCode = i + DCODE_BASE;
  int width = global_aperture_sizes[i];

  switch (global_aperture_shapes[i])
    {
    case ROUND:
      fprintf (f, "%%ADD%dC,%.4f*%%\015\012", dCode,
	       COORD_TO_INCH(width));
      break;
    case SQUARE:
      fprintf (f, "%%ADD%dR,%.4fX%.4f*%%\015\012",
	       dCode, COORD_TO_INCH(width), COORD_TO_INCH(width));
      break;
    case OCTAGON:
      fprintf (f, "%%AMOCT%d*5,0,8,0,0,%.4f,22.5*%%\015\012"
	       "%%ADD%dOCT%d*%%\015\012", dCode,
	       COORD_TO_INCH(width) / COS_22_5_DEGREE, dCode,
	       dCode);
      break;
#if 0
    case THERMAL:
      fprintf (f, "%%AMTHERM%d*7,0,0,%.4f,%.4f,%.4f,45*%%\015\012"
	       "%%ADD%dTHERM%d*%%\015\012", dCode, gap / 100000.0,
	       width / 100000.0, finger / 100000.0, dCode, dCode);
      break;
    case ROUNDCLEAR:
      fprintf (f, "%%ADD%dC,%.4fX%.4f*%%\015\012",
	       dCode, gap / 100000.0, width / 100000.0);
      break;
    case SQUARECLEAR:
      fprintf (f, "%%ADD%dR,%.4fX%.4fX%.4fX%.4f*%%\015\012",
	       dCode, gap / 100000.0, gap / 100000.0,
	       width / 100000.0, width / 100000.0);
      break;
#else
    default:
      break;
#endif
    }
}

static int
countApertures (Apertures *ap)
{
  int i, rv=0;
  for (i=0; i<GBX_MAXAPERTURECOUNT; i++)
    if (ap->aperture_used[i])
      rv ++;
  return rv;
}

static void
initApertures ()
{
  layerapps = 0;
  n_layerapps = 0;
}

static void
SetAppLayer (int l)
{
  if (l >= n_layerapps)
    {
      int prev = n_layerapps;
      n_layerapps = l + 1;
      layerapps = (Apertures *)realloc (layerapps, n_layerapps * sizeof (Apertures));
      curapp = layerapps + prev;
      while (curapp < layerapps + n_layerapps)
	{
	  int i;
	  curapp->some_apertures = 0;
	  for (i=0; i<GBX_MAXAPERTURECOUNT; i++)
	    curapp->aperture_used[i] = 0;
	  curapp++;
	}
    }
  curapp = layerapps + l;
}

/* --------------------------------------------------------------------------- */

static HID gerber_hid;

typedef struct hid_gc_struct
{
  EndCapStyle cap;
  int width;
  int color;
  int erase;
  int drill;
} hid_gc_struct;

static FILE *f = 0;
static char *filename = 0;
static char *filesuff;
static char *layername = 0;
static int lncount = 0;

static int finding_apertures = 0;
static int pagecount = 0;
static int linewidth = -1;
static int lastgroup = -1;
static int lastcap = -1;
static int lastcolor = -1;
static int print_group[MAX_LAYER];
static int print_layer[MAX_LAYER];
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
  0
};

static HID_Attribute gerber_options[] = {
  {"gerberfile", "Gerber output file base",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_gerberfile 0
  {"all-layers", "Print all layers, even empty ones",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_all_layers 1
  {"verbose", "print file names and aperture counts",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_verbose 2
  {"copy-outline", "Copy outline onto other layers",
   HID_Enum, 0, 0, {0, 0, 0}, copy_outline_names, 0},
#define HA_copy_outline 3
};

#define NUM_OPTIONS (sizeof(gerber_options)/sizeof(gerber_options[0]))

static HID_Attr_Val gerber_values[NUM_OPTIONS];

static HID_Attribute *
gerber_get_export_options (int *n)
{
  static char *last_made_filename = 0;
  if (PCB) derive_default_filename(PCB->Filename, &gerber_options[HA_gerberfile], "", &last_made_filename);

  if (n)
    *n = NUM_OPTIONS;
  return gerber_options;
}

static int
group_for_layer (int l)
{
  if (l < max_copper_layer + 2 && l >= 0)
    return GetLayerGroupNumberByNumber (l);
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
    return d;
  return b - a;
}

static void
maybe_close_f ()
{
  if (f)
    {
      if (was_drill)
	fprintf (f, "M30\015\012");
      else
	fprintf (f, "M02*\015\012");
      fclose (f);
    }
  f = 0;
}

static BoxType region;

static void
gerber_do_export (HID_Attr_Val * options)
{
  char *fnbase;
  int i;
  static int saved_layer_stack[MAX_LAYER];
  int save_ons[MAX_LAYER + 2];
  FlagType save_thindraw;

  save_thindraw = PCB->Flags;
  CLEAR_FLAG(THINDRAWFLAG, PCB);
  CLEAR_FLAG(THINDRAWPOLYFLAG, PCB);

  if (!options)
    {
      gerber_get_export_options (0);
      for (i = 0; i < NUM_OPTIONS; i++)
	gerber_values[i] = gerber_options[i].default_val;
      options = gerber_values;
    }

  fnbase = options[HA_gerberfile].str_value;
  if (!fnbase)
    fnbase = "pcb-out";

  verbose = options[HA_verbose].int_value;
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
  print_group[GetLayerGroupNumberByNumber (solder_silk_layer)] = 1;
  print_group[GetLayerGroupNumberByNumber (component_silk_layer)] = 1;
  for (i = 0; i < max_copper_layer; i++)
    if (print_group[GetLayerGroupNumberByNumber (i)])
      print_layer[i] = 1;

  memcpy (saved_layer_stack, LayerStack, sizeof (LayerStack));
  qsort (LayerStack, max_copper_layer, sizeof (LayerStack[0]), layer_sort);
  linewidth = -1;
  lastcap = -1;
  lastgroup = -1;
  lastcolor = -1;

  region.X1 = 0;
  region.Y1 = 0;
  region.X2 = PCB->MaxWidth;
  region.Y2 = PCB->MaxHeight;

  pagecount = 1;
  initApertures ();

  f = 0;
  lastgroup = -1;
  c_layerapps = 0;
  finding_apertures = 1;
  hid_expose_callback (&gerber_hid, &region, 0);

  c_layerapps = 0;
  finding_apertures = 0;
  hid_expose_callback (&gerber_hid, &region, 0);

  memcpy (LayerStack, saved_layer_stack, sizeof (LayerStack));

  maybe_close_f ();
  hid_restore_layer_ons (save_ons);
  PCB->Flags = save_thindraw;
}

static void
gerber_parse_arguments (int *argc, char ***argv)
{
  hid_register_attributes (gerber_options,
			   sizeof (gerber_options) /
			   sizeof (gerber_options[0]));
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
    return a->x - a->x;
  return b->y - b->y;
}

static int
gerber_set_layer (const char *name, int group, int empty)
{
  int want_outline;
  char *cp;
  int idx = (group >= 0
	     && group <
	     max_group) ? PCB->LayerGroups.Entries[group][0] : group;

  if (name == 0)
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
      qsort (pending_drills, n_pending_drills, sizeof (PendingDrills),
	     drill_sort);
      for (i = 0; i < n_pending_drills; i++)
	{
	  if (i == 0 || pending_drills[i].diam != pending_drills[i - 1].diam)
	    {
	      int ap = findApertureCode (pending_drills[i].diam, ROUND);
	      fprintf (f, "T%02d\015\012", ap);
	    }
	  fprintf (f, "X%06ldY%06ld\015\012",
		   gerberDrX (PCB, pending_drills[i].x),
		   gerberDrY (PCB, pending_drills[i].y));
	}
      free (pending_drills);
      n_pending_drills = max_pending_drills = 0;
      pending_drills = 0;
    }

  is_drill = (SL_TYPE (idx) == SL_PDRILL || SL_TYPE (idx) == SL_UDRILL);
  is_mask = (SL_TYPE (idx) == SL_MASK);
  current_mask = 0;
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
      char *sext=".gbr";
      int i;
      int some_apertures = 0;

      lastgroup = group;
      lastX = -1;
      lastY = -1;
      lastcolor = 0;
      linewidth = -1;
      lastcap = -1;

      SetAppLayer (c_layerapps);
      c_layerapps++;

      if (finding_apertures)
	goto emit_outline;

      if (!curapp->some_apertures && !all_layers)
	return 0;

      maybe_close_f ();

      pagecount++;
      switch (idx)
	{
	case SL (PDRILL, 0):
	  sext = ".cnc";
	  break;
	case SL (UDRILL, 0):
	  sext = ".cnc";
	  break;
	}
      strcpy (filesuff, layer_type_to_file_name (idx));
      strcat (filesuff, sext);
      f = fopen (filename, "w");
      if (f == NULL) 
	{
	  Message ( "Error:  Could not open %s for writing.\n", filename);
	  return 1;
	}

      was_drill = is_drill;

      if (verbose)
	{
	  int c = countApertures (curapp);
	  printf ("Gerber: %d aperture%s in %s\n", c,
		  c == 1 ? "" : "s", filename);
	}

      if (is_drill)
	{
	  /* We omit the ,TZ here because we are not omitting trailing zeros.  Our format is
	     always six-digit 0.1 mil resolution (i.e. 001100 = 0.11")*/
	  fprintf (f, "M48\015\012" "INCH\015\012");
	  for (i = 0; i < GBX_MAXAPERTURECOUNT; i++)
	    if (curapp->aperture_used[i])
	      fprintf (f, "T%02dC%.3f\015\012",
		       i + DCODE_BASE,
		       COORD_TO_INCH(global_aperture_sizes[i]));
	  fprintf (f, "%%\015\012");
	  /* FIXME */
	  return 1;
	}

      fprintf (f, "G04 start of page %d for group %d idx %d *\015\012",
	       pagecount, group, idx);

      /* Create a portable timestamp. */
      currenttime = time (NULL);
      {
	/* avoid gcc complaints */
	const char *fmt = "%c UTC";
	strftime (utcTime, sizeof utcTime, fmt, gmtime (&currenttime));
      }
      /* Print a cute file header at the beginning of each file. */
      fprintf (f, "G04 Title: %s, %s *\015\012", UNKNOWN (PCB->Name),
	       UNKNOWN (name));
      fprintf (f, "G04 Creator: %s " VERSION " *\015\012", Progname);
      fprintf (f, "G04 CreationDate: %s *\015\012", utcTime);

#ifdef HAVE_GETPWUID
      /* ID the user. */
      pwentry = getpwuid (getuid ());
      fprintf (f, "G04 For: %s *\015\012", pwentry->pw_name);
#endif

      fprintf (f, "G04 Format: Gerber/RS-274X *\015\012");
      fprintf (f, "G04 PCB-Dimensions: %d %d *\015\012",
	       PCB->MaxWidth, PCB->MaxHeight);
      fprintf (f, "G04 PCB-Coordinate-Origin: lower left *\015\012");

      /* Signal data in inches. */
      fprintf (f, "%%MOIN*%%\015\012");

      /* Signal Leading zero suppression, Absolute Data, 2.5 format */
      fprintf (f, "%%FSLAX25Y25*%%\015\012");

      /* build a legal identifier. */
      if (layername)
	free (layername);
      layername = strdup (filesuff);
      layername[strlen(layername) - strlen(sext)] = 0;
      for (cp=layername; *cp; cp++)
	{
	  if (isalnum((int) *cp))
	    *cp = toupper((int) *cp);
	  else
	    *cp = '_';
	}
      fprintf (f, "%%LN%s*%%\015\012", layername);
      lncount = 1;

      for (i=0; i<GBX_MAXAPERTURECOUNT; i++)
	if (curapp->aperture_used[i])
	  {
	    some_apertures ++;
	    printAperture(f, i);
	  }
      if (!some_apertures)
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
	  hidGC gc = gui->make_gc ();
	  printf("name %s idx %d\n", name, idx);
	  if (SL_TYPE (idx) == SL_SILK)
	    gui->set_line_width (gc, PCB->minSlk);
	  else if (group >= 0)
	    gui->set_line_width (gc, PCB->minWid);
	  else
	    gui->set_line_width (gc, AUTO_OUTLINE_WIDTH);
	  gui->draw_line (gc, 0, 0, PCB->MaxWidth, 0);
	  gui->draw_line (gc, 0, 0, 0, PCB->MaxHeight);
	  gui->draw_line (gc, PCB->MaxWidth, 0, PCB->MaxWidth, PCB->MaxHeight);
	  gui->draw_line (gc, 0, PCB->MaxHeight, PCB->MaxWidth, PCB->MaxHeight);
	  gui->destroy_gc (gc);
	}
    }

  return 1;
}

static hidGC
gerber_make_gc (void)
{
  hidGC rv = (hidGC) calloc (1, sizeof (hid_gc_struct));
  rv->cap = Trace_Cap;
  return rv;
}

static void
gerber_destroy_gc (hidGC gc)
{
  free (gc);
}

static void
gerber_use_mask (int use_it)
{
  current_mask = use_it;
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
gerber_set_line_width (hidGC gc, int width)
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
  int c;
  if (radius)
    {
      radius *= 2;
      if (radius != linewidth || lastcap != Round_Cap)
	{
	  c = findApertureCode (radius, ROUND);
	  if (c <= 0)
	    {
	      fprintf (stderr,
		       "error: aperture for radius %d type ROUND is %d\n",
		       radius, c);
	    }
	  if (f && !is_drill)
	    fprintf (f, "G54D%d*", c);
	  linewidth = radius;
	  lastcap = Round_Cap;
	}
    }
  else if (linewidth != gc->width || lastcap != gc->cap)
    {
      int ap;
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
      ap = findApertureCode (linewidth, (ApertureShape)c);
      if (ap <= 0)
	{
	  fprintf (stderr, "error: aperture for width %d type %s is %d\n",
		   linewidth, c == ROUND ? "ROUND" : "SQUARE", ap);
	}
      if (f)
	fprintf (f, "G54D%d*", ap);
    }
#if 0
  if (lastcolor != gc->color)
    {
      c = gc->color;
      if (is_drill)
	return;
      if (is_mask)
	c = (gc->erase ? 0 : 1);
      lastcolor = gc->color;
      if (f)
	{
	  if (c)
	    {
	      fprintf (f, "%%LN%s_C%d*%%\015\012", layername, lncount++);
	      fprintf (f, "%%LPC*%%\015\012");
	    }
	  else
	    {
	      fprintf (f, "%%LN%s_D%d*%%\015\012", layername, lncount++);
	      fprintf (f, "%%LPD*%%\015\012");
	    }
	}
    }
#endif
}

static void
gerber_draw_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  gerber_draw_line (gc, x1, y1, x1, y2);
  gerber_draw_line (gc, x1, y1, x2, y1);
  gerber_draw_line (gc, x1, y2, x2, y2);
  gerber_draw_line (gc, x2, y1, x2, y2);
}

static void
gerber_draw_line (hidGC gc, int x1, int y1, int x2, int y2)
{
  bool m = false;

  if (x1 != x2 && y1 != y2 && gc->cap == Square_Cap)
    {
      int x[5], y[5];
      float tx, ty, theta;

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
      fprintf (f, "X%ld", gerberX (PCB, lastX));
    }
  if (y1 != lastY)
    {
      m = true;
      lastY = y1;
      fprintf (f, "Y%ld", gerberY (PCB, lastY));
    }
  if ((x1 == x2) && (y1 == y2))
    fprintf (f, "D03*\015\012");
  else
    {
      if (m)
	fprintf (f, "D02*");
      if (x2 != lastX)
	{
	  lastX = x2;
	  fprintf (f, "X%ld", gerberX (PCB, lastX));
	}
      if (y2 != lastY)
	{
	  lastY = y2;
	  fprintf (f, "Y%ld", gerberY (PCB, lastY));

	}
      fprintf (f, "D01*\015\012");
    }

}

static void
gerber_draw_arc (hidGC gc, int cx, int cy, int width, int height,
		 int start_angle, int delta_angle)
{
  bool m = false;
  float arcStartX, arcStopX, arcStartY, arcStopY;

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
      int max = width > height ? width : height;
      int minr = max - gc->width / 10;
      int nsteps;
      int x0, y0, x1, y1;

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
      fprintf (f, "X%ld", gerberX (PCB, lastX));
    }
  if (arcStartY != lastY)
    {
      m = true;
      lastY = arcStartY;
      fprintf (f, "Y%ld", gerberY (PCB, lastY));
    }
  if (m)
    fprintf (f, "D02*");
  fprintf (f,
	   "G75*G0%1dX%ldY%ldI%ldJ%ldD01*G01*\015\012",
	   (delta_angle < 0) ? 2 : 3,
	   gerberX (PCB, arcStopX), gerberY (PCB, arcStopY),
	   gerberXOffset (PCB, cx - arcStartX),
	   gerberYOffset (PCB, cy - arcStartY));
  lastX = arcStopX;
  lastY = arcStopY;
}

#define ROUND(x) ((int)(((x)+50)/100)*100)

static void
gerber_fill_circle (hidGC gc, int cx, int cy, int radius)
{
  if (radius <= 0)
    return;
  if (is_drill)
    radius = ROUND(radius*2)/2;
  use_gc (gc, radius);
  if (!f)
    return;
  if (is_drill)
    {
      if (n_pending_drills >= max_pending_drills)
	{
	  max_pending_drills += 100;
	  pending_drills = (PendingDrills *) realloc (pending_drills,
						      max_pending_drills *
						      sizeof (PendingDrills));
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
      fprintf (f, "X%ld", gerberX (PCB, lastX));
    }
  if (cy != lastY)
    {
      lastY = cy;
      fprintf (f, "Y%ld", gerberY (PCB, lastY));
    }
  fprintf (f, "D03*\015\012");
}

static void
gerber_fill_polygon (hidGC gc, int n_coords, int *x, int *y)
{
  bool m = false;
  int i;
  int firstTime = 1;
  LocationType startX = 0, startY = 0;

  if (is_mask && current_mask == HID_MASK_BEFORE)
    return;

  use_gc (gc, 10 * 100);
  if (!f)
    return;
  fprintf (f, "G36*\015\012");
  for (i = 0; i < n_coords; i++)
    {
      if (x[i] != lastX)
	{
	  m = true;
	  lastX = x[i];
	  fprintf (f, "X%ld", gerberX (PCB, lastX));
	}
      if (y[i] != lastY)
	{
	  m = true;
	  lastY = y[i];
	  fprintf (f, "Y%ld", gerberY (PCB, lastY));
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
	fprintf (f, "D01*\015\012");
      m = false;
    }
  if (startX != lastX)
    {
      m = true;
      lastX = startX;
      fprintf (f, "X%ld", gerberX (PCB, startX));
    }
  if (startY != lastY)
    {
      m = true;
      lastY = startY;
      fprintf (f, "Y%ld", gerberY (PCB, lastY));
    }
  if (m)
    fprintf (f, "D01*\015\012");
  fprintf (f, "G37*\015\012");
}

static void
gerber_fill_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  int x[5];
  int y[5];
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
  memset (&gerber_hid, 0, sizeof (HID));

  common_nogui_init (&gerber_hid);
  common_draw_helpers_init (&gerber_hid);

  gerber_hid.struct_size         = sizeof (HID);
  gerber_hid.name                = "gerber";
  gerber_hid.description         = "RS-274X (Gerber) export.";
  gerber_hid.exporter            = 1;

  gerber_hid.get_export_options  = gerber_get_export_options;
  gerber_hid.do_export           = gerber_do_export;
  gerber_hid.parse_arguments     = gerber_parse_arguments;
  gerber_hid.set_layer           = gerber_set_layer;
  gerber_hid.make_gc             = gerber_make_gc;
  gerber_hid.destroy_gc          = gerber_destroy_gc;
  gerber_hid.use_mask            = gerber_use_mask;
  gerber_hid.set_color           = gerber_set_color;
  gerber_hid.set_line_cap        = gerber_set_line_cap;
  gerber_hid.set_line_width      = gerber_set_line_width;
  gerber_hid.set_draw_xor        = gerber_set_draw_xor;
  gerber_hid.draw_line           = gerber_draw_line;
  gerber_hid.draw_arc            = gerber_draw_arc;
  gerber_hid.draw_rect           = gerber_draw_rect;
  gerber_hid.fill_circle         = gerber_fill_circle;
  gerber_hid.fill_polygon        = gerber_fill_polygon;
  gerber_hid.fill_rect           = gerber_fill_rect;
  gerber_hid.calibrate           = gerber_calibrate;
  gerber_hid.set_crosshair       = gerber_set_crosshair;

  hid_register_hid (&gerber_hid);
}
