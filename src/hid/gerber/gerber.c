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

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#include <time.h>

#include "config.h"
#include "global.h"
#include "data.h"
#include "misc.h"
#include "error.h"

#include "hid.h"
#include "../hidint.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented Gerber function %s.\n", __FUNCTION__); abort()

static HID gerber_hid;

static void gerber_fill_polygon (hidGC gc, int n_coords, int *x, int *y);

/*----------------------------------------------------------------------------*/
/* Utility routines                                                           */
/*----------------------------------------------------------------------------*/

#define gerberX(pcb, x) ((long) ((x)/10))
#define gerberY(pcb, y) ((long) (((pcb)->MaxHeight - (y)))/10)
#define gerberXOffset(pcb, x) ((long) ((x) /10))
#define gerberYOffset(pcb, y) ((long) (-(y) /10))

/*----------------------------------------------------------------------------*/
/* Private data structures                                                    */
/*----------------------------------------------------------------------------*/

static int is_mask, was_drill;
static int is_drill;
static int current_mask;

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

typedef struct
{
  DynamicStringType appList;
  int lastdCode;
  int lastTherm;
  int nextAperture;		/* Number of apertures */
  Aperture aperture[GBX_MAXAPERTURECOUNT];
} Apertures;

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
/* Aperture Routines                                                          */
/*----------------------------------------------------------------------------*/



static int
findApertureCode (int width, ApertureShape shape)
{
  int i;
  Aperture *ap;
  char appMacro[256];

  /* we never draw zero-width lines */
  if (width == 0)
    return (0);

  /* Search for an appropriate aperture. */

  for (i = 0; i < curapp->nextAperture; i++)
    {
      ap = &(curapp->aperture[i]);

      if (ap->apertureSize == width && ap->apertureShape == shape)
	return (ap->dCode);
    }
  appMacro[0] = '\0';

  /* Not found, create a new aperture and add it to the list */
  if (curapp->nextAperture < GBX_MAXAPERTURECOUNT)
    {
      i = curapp->nextAperture++;
      ap = &(curapp->aperture[i]);
      ap->dCode = curapp->lastdCode++;
      ap->apertureSize = width;
      ap->apertureShape = shape;
      switch (shape)
	{
	case ROUND:
	  sprintf (appMacro, "%%ADD%dC,%.4f*%%\015\012", ap->dCode,
		   width / 100000.0);
	  break;
	case SQUARE:
	  sprintf (appMacro, "%%ADD%dR,%.4fX%.4f*%%\015\012",
		   ap->dCode, width / 100000.0, width / 100000.0);
	  break;
	case OCTAGON:
	  sprintf (appMacro, "%%AMOCT%d*5,0,8,0,0,%.4f,22.5*%%\015\012"
		   "%%ADD%dOCT%d*%%\015\012", curapp->lastTherm,
		   width / (100000.0 * COS_22_5_DEGREE), ap->dCode,
		   curapp->lastTherm);
	  curapp->lastTherm++;
	  break;
#if 0
	case THERMAL:
	  sprintf (appMacro, "%%AMTHERM%d*7,0,0,%.4f,%.4f,%.4f,45*%%\015\012"
		   "%%ADD%dTHERM%d*%%\015\012", lastTherm, gap / 100000.0,
		   width / 100000.0, finger / 100000.0, ap->dCode, lastTherm);
	  lastTherm++;
	  break;
	case ROUNDCLEAR:
	  sprintf (appMacro, "%%ADD%dC,%.4fX%.4f*%%\015\012",
		   ap->dCode, gap / 100000.0, width / 100000.0);
	  break;
	case SQUARECLEAR:
	  sprintf (appMacro, "%%ADD%dR,%.4fX%.4fX%.4fX%.4f*%%\015\012",
		   ap->dCode, gap / 100000.0, gap / 100000.0,
		   width / 100000.0, width / 100000.0);
	  break;
#else
	default:
	  break;
#endif
	}

      DSAddString (&(curapp->appList), appMacro);
      return (ap->dCode);
    }
  else
    {
      Message (_("Error, too many apertures needed for Gerber file.\n"));
      return (10);
    }
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
      layerapps =
	MyRealloc (layerapps, n_layerapps * sizeof (Apertures),
		   "SetAppLayer");
      curapp = layerapps + prev;
      while (curapp < layerapps + n_layerapps)
	{
	  curapp->appList.Data = NULL;
	  curapp->appList.MaxLength = 0;
	  curapp->lastdCode = 11;
	  curapp->lastTherm = 1;
	  curapp->nextAperture = 0;
	  curapp++;
	}
    }
  curapp = layerapps + l;
}

/* --------------------------------------------------------------------------- */

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

static HID_Attribute gerber_options[] = {
  {"gerberfile", "Gerber output file base",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_gerberfile 0
};

#define NUM_OPTIONS (sizeof(gerber_options)/sizeof(gerber_options[0]))

static HID_Attr_Val gerber_values[NUM_OPTIONS];

static HID_Attribute *
gerber_get_export_options (int *n)
{
  char *buf = 0;

  if (PCB && PCB->Filename)
    {
      buf = (char *) malloc (strlen (PCB->Filename) + 4);
      if (buf)
	{
	  strcpy (buf, PCB->Filename);
	  if (strcmp (buf + strlen (buf) - 4, ".pcb") == 0)
	    buf[strlen (buf) - 4] = 0;
	  if (gerber_options[HA_gerberfile].default_val.str_value)
	    free (gerber_options[HA_gerberfile].default_val.str_value);
	  gerber_options[HA_gerberfile].default_val.str_value = buf;
	}
    }

  if (n)
    *n = NUM_OPTIONS;
  return gerber_options;
}

static int
group_for_layer (int l)
{
  if (l < MAX_LAYER + 2 && l >= 0)
    return GetLayerGroupNumberByNumber (l);
  /* else something unique */
  return MAX_LAYER + 3 + l;
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

static void
gerber_do_export (HID_Attr_Val * options)
{
  char *fnbase;
  int i;
  static int saved_layer_stack[MAX_LAYER];
  BoxType region;
  int save_ons[MAX_LAYER + 2];

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

  i = strlen (fnbase);
  filename = MyRealloc (filename, i + 40, "gerber");
  strcpy (filename, fnbase);
  strcat (filename, ".");
  filesuff = filename + strlen (filename);

  memset (print_group, 0, sizeof (print_group));
  memset (print_layer, 0, sizeof (print_layer));

  hid_save_and_show_layer_ons (save_ons);
  for (i = 0; i < MAX_LAYER; i++)
    {
      LayerType *layer = PCB->Data->Layer + i;
      if (layer->LineN || layer->TextN || layer->ArcN || layer->PolygonN)
	print_group[GetLayerGroupNumberByNumber (i)] = 1;
    }
  print_group[GetLayerGroupNumberByNumber (MAX_LAYER)] = 1;
  print_group[GetLayerGroupNumberByNumber (MAX_LAYER + 1)] = 1;
  for (i = 0; i < MAX_LAYER; i++)
    if (print_group[GetLayerGroupNumberByNumber (i)])
      print_layer[i] = 1;

  memcpy (saved_layer_stack, LayerStack, sizeof (LayerStack));
  qsort (LayerStack, MAX_LAYER, sizeof (LayerStack[0]), layer_sort);
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
}

extern void hid_parse_command_line (int *argc, char ***argv);

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
gerber_set_layer (const char *name, int group)
{
  char *cp;
  int idx = (group >= 0
	     && group <
	     MAX_LAYER) ? PCB->LayerGroups.Entries[group][0] : group;
  if (name == 0)
    name = PCB->Data->Layer[idx].Name;

  if (idx >= 0 && idx < MAX_LAYER && !print_layer[idx])
    return 0;

  if (strcmp (name, "invisible") == 0)
    return 0;
  if (SL_TYPE (idx) == SL_ASSY)
    return 0;

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
		   gerberX (PCB, pending_drills[i].x), gerberY (PCB,
								pending_drills
								[i].y));
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
      char *spat, *sext=".gbr";
      int i;

      lastgroup = group;
      lastX = -1;
      lastY = -1;
      lastcolor = 0;
      linewidth = -1;
      lastcap = -1;

      SetAppLayer (c_layerapps);
      c_layerapps++;

      if (finding_apertures)
	return 1;

      if (!curapp->nextAperture)
	return 0;

      maybe_close_f ();

      pagecount++;
      switch (idx)
	{
	case SL (SILK, TOP):
	  spat = "frontsilk";
	  break;
	case SL (SILK, BOTTOM):
	  spat = "backsilk";
	  break;
	case SL (MASK, TOP):
	  spat = "frontmask";
	  break;
	case SL (MASK, BOTTOM):
	  spat = "backmask";
	  break;
	case SL (PASTE, TOP):
	  spat = "frontpaste";
	  break;
	case SL (PASTE, BOTTOM):
	  spat = "backpaste";
	  break;
	case SL (PDRILL, 0):
	  spat = "plated-drill";
	  sext = ".cnc";
	  break;
	case SL (UDRILL, 0):
	  spat = "unplated-drill";
	  sext = ".cnc";
	  break;
	case SL (FAB, 0):
	  spat = "fab";
	  break;
	case SL (ASSY, TOP):
	  spat = "frontassembly";
	  break;
	case SL (ASSY, BOTTOM):
	  spat = "backassembly";
	  break;
	default:
	  if (group == GetLayerGroupNumberByNumber(MAX_LAYER+COMPONENT_LAYER))
	    spat = "front";
	  else if (group == GetLayerGroupNumberByNumber(MAX_LAYER+SOLDER_LAYER))
	    spat = "back";
	  else
	    spat = "group%d";
	  break;
	}
      sprintf (filesuff, spat, group);
      strcat (filesuff, sext);
      f = fopen (filename, "w");
      was_drill = is_drill;

#if 0
      printf ("Gerber: %d aperture%s in %s\n", curapp->nextAperture,
	      curapp->nextAperture == 1 ? "" : "s", filename);
#endif
      if (is_drill)
	{
	  fprintf (f, "M48\015\012" "INCH,TZ\015\012");
	  for (i = 0; i < curapp->nextAperture; i++)
	    fprintf (f, "T%02dC%.3f\015\012",
		     curapp->aperture[i].dCode,
		     curapp->aperture[i].apertureSize / 100000.0);
	  fprintf (f, "%%\015\012");
	  /* FIXME */
	  return 1;
	}

      fprintf (f, "G04 start of page %d for group %d idx %d\015\012",
	       pagecount, group, idx);

      /* Create a portable timestamp. */
      currenttime = time (NULL);
      strftime (utcTime, sizeof utcTime, "%c UTC", gmtime (&currenttime));

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

      /* Signal Leading zero suppression, Absolute Data, 2.3 format */
      fprintf (f, "%%FSLAX24Y24*%%\015\012");

      /* build a legal identifier. */
      if (layername)
	free (layername);
      layername = strdup (filesuff);
      layername[strlen(layername) - strlen(sext)] = 0;
      for (cp=layername; *cp; cp++)
	{
	  if (isalnum(*cp))
	    *cp = toupper(*cp);
	  else
	    *cp = '_';
	}
      fprintf (f, "%%LN%s*%%\015\012", layername);
      lncount = 1;

      fprintf (f, "%s", curapp->appList.Data);
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
gerber_set_draw_xor (hidGC gc, int xor)
{
  ;
}

static void
gerber_set_draw_faded (hidGC gc, int faded)
{
  ;
}

static void
gerber_set_line_cap_angle (hidGC gc, int x1, int y1, int x2, int y2)
{
  CRASH;
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
      ap = findApertureCode (linewidth, c);
      if (ap <= 0)
	{
	  fprintf (stderr, "error: aperture for width %d type %s is %d\n",
		   linewidth, c == ROUND ? "ROUND" : "SQUARE", ap);
	}
      if (f)
	fprintf (f, "G54D%d*", ap);
    }

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
}

static void
gerber_draw_rect (hidGC gc, int x1, int y1, int x2, int y2)
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
gerber_draw_line (hidGC gc, int x1, int y1, int x2, int y2)
{
  Boolean m = False;
  use_gc (gc, 0);
  if (!f)
    return;
  if (x1 != lastX)
    {
      m = True;
      lastX = x1;
      fprintf (f, "X%ld", gerberX (PCB, lastX));
    }
  if (y1 != lastY)
    {
      m = True;
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
  Boolean m = False;
  float arcStartX, arcStopX, arcStartY, arcStopY;

  if (gc->width == 1)
    return;

  use_gc (gc, 0);
  if (!f)
    return;

  arcStartX = cx - width * cos (TO_RADIANS (start_angle));
  arcStartY = cy + height * sin (TO_RADIANS (start_angle));
  arcStopX = cx - width * cos (TO_RADIANS (start_angle + delta_angle));
  arcStopY = cy + height * sin (TO_RADIANS (start_angle + delta_angle));
  if (arcStartX != lastX)
    {
      m = True;
      lastX = arcStartX;
      fprintf (f, "X%ld", gerberX (PCB, lastX));
    }
  if (arcStartY != lastY)
    {
      m = True;
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
  else if (gc->drill)
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
  Boolean m = False;
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
	  m = True;
	  lastX = x[i];
	  fprintf (f, "X%ld", gerberX (PCB, lastX));
	}
      if (y[i] != lastY)
	{
	  m = True;
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
      m = False;
    }
  if (startX != lastX)
    {
      m = True;
      lastX = startX;
      fprintf (f, "X%ld", gerberX (PCB, startX));
    }
  if (startY != lastY)
    {
      m = True;
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
gerber_set_crosshair (int x, int y)
{
}

static HID gerber_hid = {
  "gerber",
  "RS-274X (Gerber) export.",
  0, 0, 1, 1, 0,
  gerber_get_export_options,
  gerber_do_export,
  gerber_parse_arguments,
  0 /* gerber_invalidate_wh */ ,
  0 /* gerber_invalidate_lr */ ,
  0 /* gerber_invalidate_all */ ,
  gerber_set_layer,
  gerber_make_gc,
  gerber_destroy_gc,
  gerber_use_mask,
  gerber_set_color,
  gerber_set_line_cap,
  gerber_set_line_width,
  gerber_set_draw_xor,
  gerber_set_draw_faded,
  gerber_set_line_cap_angle,
  gerber_draw_line,
  gerber_draw_arc,
  gerber_draw_rect,
  gerber_fill_circle,
  gerber_fill_polygon,
  gerber_fill_rect,
  gerber_calibrate,
  0 /* gerber_shift_is_pressed */ ,
  0 /* gerber_control_is_pressed */ ,
  0 /* gerber_get_coords */ ,
  gerber_set_crosshair,
  0 /* gerber_add_timer */ ,
  0 /* gerber_stop_timer */ ,
  0 /* gerber_log */ ,
  0 /* gerber_logv */ ,
  0 /* gerber_confirm_dialog */ ,
  0 /* gerber_report_dialog */ ,
  0 /* gerber_prompt_for */ ,
  0 /* gerber_attribute_dialog */ ,
  0 /* gerber_show_item */ ,
  0				/* gerber_beep */
};

void
hid_gerber_init ()
{
  apply_default_hid (&gerber_hid, 0);
  hid_register_hid (&gerber_hid);
}
