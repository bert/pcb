/* $Id$ */

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
#include "../ps/ps.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented EPS function %s.\n", __FUNCTION__); abort()
static HID eps_hid;

typedef struct hid_gc_struct
{
  EndCapStyle cap;
  int width;
  int color;
  int erase;
} hid_gc_struct;

static FILE *f = 0;
static int linewidth = -1;
static int lastgroup = -1;
static int lastcap = -1;
static int lastcolor = -1;
static int print_group[MAX_LAYER];
static int print_layer[MAX_LAYER];
static int fast_erase = -1;

static HID_Attribute eps_attribute_list[] = {
  /* other HIDs expect this to be first.  */
  {"eps-file", "Encapsulated Postscript output file",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_psfile 0
  {"eps-scale", "EPS scale",
   HID_Real, 0, 100, {0, 0, 1.0}, 0, 0},
#define HA_scale 1
  {"as-shown", "Export layers as shown on screen",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_as_shown 2
  {"monochrome", "Convert to monochrome (like the ps export)",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_mono 3
  {"only-visible", "Limit the bounds of the EPS file to the visible items",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_only_visible 4
};

#define NUM_OPTIONS (sizeof(eps_attribute_list)/sizeof(eps_attribute_list[0]))

REGISTER_ATTRIBUTES (eps_attribute_list)

static HID_Attr_Val eps_values[NUM_OPTIONS];

static HID_Attribute *
eps_get_export_options (int *n)
{
  static char *last_made_filename = 0;

  if (PCB) derive_default_filename(PCB->Filename, &eps_attribute_list[HA_psfile], ".eps", &last_made_filename);

  if (n)
    *n = NUM_OPTIONS;
  return eps_attribute_list;
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
eps_hid_export_to_file (FILE * the_file, HID_Attr_Val * options)
{
  int i;
  static int saved_layer_stack[MAX_LAYER];
  BoxType region;
  FlagType save_thindraw;

  save_thindraw = PCB->Flags;
  CLEAR_FLAG(THINDRAWFLAG, PCB);
  CLEAR_FLAG(THINDRAWPOLYFLAG, PCB);

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

  /* Figure out which layers actually have stuff on them.  */
  for (i = 0; i < max_layer; i++)
    {
      LayerType *layer = PCB->Data->Layer + i;
      if (layer->On)
	if (layer->LineN || layer->TextN || layer->ArcN || layer->PolygonN)
	  print_group[GetLayerGroupNumberByNumber (i)] = 1;
    }

  /* Now, if only one layer has real stuff on it, we can use the fast
     erase logic.  Otherwise, we have to use the expensive multi-mask
     erase.  */
  fast_erase = 0;
  for (i = 0; i < max_layer; i++)
    if (print_group[i])
      fast_erase ++;

  /* If NO layers had anything on them, at least print the component
     layer to get the pins.  */
  if (fast_erase == 0)
    {
      print_group[GetLayerGroupNumberByNumber (max_layer + COMPONENT_LAYER)] = 1;
      fast_erase = 1;
    }

  /* "fast_erase" is 1 if we can just paint white to erase.  */
  fast_erase = fast_erase == 1 ? 1 : 0;

  /* Now, for each group we're printing, mark its layers for
     printing.  */
  for (i = 0; i < max_layer; i++)
    if (print_group[GetLayerGroupNumberByNumber (i)])
      print_layer[i] = 1;

  if (fast_erase) {
    eps_hid.poly_before = 1;
    eps_hid.poly_after = 0;
  } else {
    eps_hid.poly_before = 0;
    eps_hid.poly_after = 1;
  }

  memcpy (saved_layer_stack, LayerStack, sizeof (LayerStack));
  as_shown = options[HA_as_shown].int_value;
  if (!options[HA_as_shown].int_value)
    {
      comp_layer = GetLayerGroupNumberByNumber (max_layer + COMPONENT_LAYER);
      solder_layer = GetLayerGroupNumberByNumber (max_layer + SOLDER_LAYER);
      qsort (LayerStack, max_layer, sizeof (LayerStack[0]), layer_sort);
    }
  fprintf (f, "%%!PS-Adobe-3.0 EPSF-3.0\n");
  linewidth = -1;
  lastcap = -1;
  lastgroup = -1;
  lastcolor = -1;

  in_mono = options[HA_mono].int_value;

#define pcb2em(x) (int)((x) / 100000.0 * 72.0 * options[HA_scale].real_value + 1)
  fprintf (f, "%%%%BoundingBox: 0 0 %d %d\n",
	   pcb2em (bounds->X2 - bounds->X1),
	   pcb2em (bounds->Y2 - bounds->Y1));
  fprintf (f, "%%%%Pages: 1\n");
  fprintf (f,
	   "save countdictstack mark newpath /showpage {} def /setpagedevice {pop} def\n");
  fprintf (f, "%%%%EndProlog\n");
  fprintf (f, "%%%%Page: 1 1\n");
  fprintf (f, "%%%%BeginDocument: %s\n\n", filename);

  fprintf (f, "72 72 scale\n");
  fprintf (f, "0.00001 dup neg scale\n");
  fprintf (f, "%g dup scale\n", options[HA_scale].real_value);
  fprintf (f, "%d %d translate\n", -bounds->X1, -bounds->Y2);
  if (options[HA_as_shown].int_value
      && Settings.ShowSolderSide)
    {
      fprintf (f, "-1 1 scale %d 0 translate\n",
	       bounds->X1 - bounds->X2);
    }
  linewidth = -1;
  lastcap = -1;
  lastcolor = -1;
#define Q 1000
  fprintf (f,
	   "/nclip { %d %d moveto %d %d lineto %d %d lineto %d %d lineto %d %d lineto eoclip newpath } def\n",
	   bounds->X1 - Q, bounds->Y1 - Q, bounds->X1 - Q, bounds->Y2 + Q,
	   bounds->X2 + Q, bounds->Y2 + Q, bounds->X2 + Q, bounds->Y1 - Q,
	   bounds->X1 - Q, bounds->Y1 - Q);
#undef Q
  fprintf (f, "/t { moveto lineto stroke } bind def\n");
  fprintf (f, "/tc { moveto lineto strokepath nclip } bind def\n");
  fprintf (f, "/r { /y2 exch def /x2 exch def /y1 exch def /x1 exch def\n");
  fprintf (f,
	   "     x1 y1 moveto x1 y2 lineto x2 y2 lineto x2 y1 lineto closepath fill } bind def\n");
  fprintf (f, "/c { 0 360 arc fill } bind def\n");
  fprintf (f, "/cc { 0 360 arc nclip } bind def\n");
  fprintf (f,
	   "/a { gsave setlinewidth translate scale 0 0 1 5 3 roll arc stroke grestore} bind def\n");

  lastgroup = -1;
  hid_expose_callback (&eps_hid, bounds, 0);

  fprintf (f, "showpage\n");

  fprintf (f, "%%%%EndDocument\n");
  fprintf (f, "%%%%Trailer\n");
  fprintf (f, "cleartomark countdictstack exch sub { end } repeat restore\n");
  fprintf (f, "%%%%EOF\n");

  memcpy (LayerStack, saved_layer_stack, sizeof (LayerStack));
  PCB->Flags = save_thindraw;
}

static void
eps_do_export (HID_Attr_Val * options)
{
  int i;
  int save_ons[MAX_LAYER + 2];

  if (!options)
    {
      eps_get_export_options (0);
      for (i = 0; i < NUM_OPTIONS; i++)
	eps_values[i] = eps_attribute_list[i].default_val;
      options = eps_values;
    }

  filename = options[HA_psfile].str_value;
  if (!filename)
    filename = "pcb-out.eps";

  f = fopen (filename, "w");
  if (!f)
    {
      perror (filename);
      return;
    }

  if (!options[HA_as_shown].int_value)
    hid_save_and_show_layer_ons (save_ons);
  eps_hid_export_to_file (f, options);
  if (!options[HA_as_shown].int_value)
    hid_restore_layer_ons (save_ons);

  fclose (f);
}

extern void hid_parse_command_line (int *argc, char ***argv);

static void
eps_parse_arguments (int *argc, char ***argv)
{
  hid_register_attributes (eps_attribute_list,
			   sizeof (eps_attribute_list) /
			   sizeof (eps_attribute_list[0]));
  hid_parse_command_line (argc, argv);
}

static int is_mask;
static int is_paste;
static int is_drill;

static int
eps_set_layer (const char *name, int group)
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

  is_drill = (SL_TYPE (idx) == SL_PDRILL || SL_TYPE (idx) == SL_UDRILL || SL_TYPE (idx) == SL_SPDRILL);
  is_mask = (SL_TYPE (idx) == SL_MASK);
  is_paste = (SL_TYPE (idx) == SL_PASTE);

  if (is_mask || is_paste)
    return 0;
#if 0
  printf ("Layer %s group %d drill %d mask %d\n", name, group, is_drill,
	  is_mask);
#endif
  fprintf (f, "%% Layer %s group %d drill %d mask %d\n", name, group,
	   is_drill, is_mask);

  if (as_shown)
    {
      switch (idx)
	{
	case SL (SILK, TOP):
	case SL (SILK, BOTTOM):
	  if (SL_MYSIDE (idx))
	    return PCB->ElementOn;
	  else
	    return 0;
	}
    }
  else
    {
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
eps_make_gc (void)
{
  hidGC rv = (hidGC) malloc (sizeof (hid_gc_struct));
  rv->cap = Trace_Cap;
  rv->width = 0;
  rv->color = 0;
  return rv;
}

static void
eps_destroy_gc (hidGC gc)
{
  free (gc);
}

static void
eps_use_mask (int use_it)
{
  static int mask_pending = 0;
  switch (use_it)
    {
    case HID_MASK_CLEAR:
      if (!mask_pending)
	{
	  mask_pending = 1;
	  fprintf (f, "gsave\n");
	}
      break;
    case HID_MASK_AFTER:
      break;
    case HID_MASK_OFF:
      if (mask_pending)
	{
	  mask_pending = 0;
	  fprintf (f, "grestore\n");
	  lastcolor = -1;
	}
      break;
    }
}

static void
eps_set_color (hidGC gc, const char *name)
{
  static void *cache = 0;
  hidval cval;

  if (strcmp (name, "erase") == 0)
    {
      gc->color = 0xffffff;
      gc->erase = fast_erase ? 0 : 1;
      return;
    }
  if (strcmp (name, "drill") == 0)
    {
      gc->color = 0xffffff;
      gc->erase = 0;
      return;
    }
  gc->erase = 0;
  if (hid_cache_color (0, name, &cval, &cache))
    {
      gc->color = cval.lval;
    }
  else if (in_mono)
    {
      gc->color = 0;
    }
  else if (name[0] == '#')
    {
      unsigned int r, g, b;
      sscanf (name + 1, "%2x%2x%2x", &r, &g, &b);
      gc->color = (r << 16) + (g << 8) + b;
    }
  else
    gc->color = 0;
}

static void
eps_set_line_cap (hidGC gc, EndCapStyle style)
{
  gc->cap = style;
}

static void
eps_set_line_width (hidGC gc, int width)
{
  gc->width = width;
}

static void
eps_set_draw_xor (hidGC gc, int xor)
{
  ;
}

static void
eps_set_draw_faded (hidGC gc, int faded)
{
  ;
}

static void
eps_set_line_cap_angle (hidGC gc, int x1, int y1, int x2, int y2)
{
  CRASH;
}

static void
use_gc (hidGC gc)
{
  if (linewidth != gc->width)
    {
      fprintf (f, "%d setlinewidth\n", gc->width);
      linewidth = gc->width;
    }
  if (lastcap != gc->cap)
    {
      int c;
      switch (gc->cap)
	{
	case Round_Cap:
	case Trace_Cap:
	  c = 1;
	  break;
	default:
	case Square_Cap:
	  c = 2;
	  break;
	}
      fprintf (f, "%d setlinecap\n", c);
      lastcap = gc->cap;
    }
  if (lastcolor != gc->color)
    {
      int c = gc->color;
#define CV(x,b) (((x>>b)&0xff)/255.0)
      fprintf (f, "%g %g %g setrgbcolor\n", CV (c, 16), CV (c, 8), CV (c, 0));
      lastcolor = gc->color;
    }
}

static void eps_fill_rect (hidGC gc, int x1, int y1, int x2, int y2);
static void eps_fill_circle (hidGC gc, int cx, int cy, int radius);

static void
eps_draw_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  use_gc (gc);
  fprintf (f, "%d %d %d %d r\n", x1, y1, x2, y2);
}

static void
eps_draw_line (hidGC gc, int x1, int y1, int x2, int y2)
{
  int w = gc->width / 2;
  if (x1 == x2 && y1 == y2)
    {
      if (gc->cap == Square_Cap)
	eps_fill_rect (gc, x1 - w, y1 - w, x1 + w, y1 + w);
      else
	eps_fill_circle (gc, x1, y1, w);
      return;
    }
  use_gc (gc);
  if (gc->erase && gc->cap != Square_Cap)
    {
      double ang = atan2 (y2 - y1, x2 - x1);
      double dx = w * sin (ang);
      double dy = -w * cos (ang);
      double deg = ang * 180.0 / M_PI;
      int vx1 = x1 + dx;
      int vy1 = y1 + dy;

      fprintf (f, "%d %d moveto ", vx1, vy1);
      fprintf (f, "%d %d %d %g %g arc\n", x2, y2, w, deg - 90, deg + 90);
      fprintf (f, "%d %d %d %g %g arc\n", x1, y1, w, deg + 90, deg + 270);
      fprintf (f, "nclip\n");

      return;
    }
  fprintf (f, "%d %d %d %d %s\n", x1, y1, x2, y2, gc->erase ? "tc" : "t");
}

static void
eps_draw_arc (hidGC gc, int cx, int cy, int width, int height,
	      int start_angle, int delta_angle)
{
  int sa, ea;
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
#if 0
  printf ("draw_arc %d,%d %dx%d %d..%d %d..%d\n",
	  cx, cy, width, height, start_angle, delta_angle, sa, ea);
#endif
  use_gc (gc);
  fprintf (f, "%d %d %d %d %d %d %g a\n",
	   sa, ea, -width, height, cx, cy, (double) linewidth / width);
}

static void
eps_fill_circle (hidGC gc, int cx, int cy, int radius)
{
  use_gc (gc);
  fprintf (f, "%d %d %d %s\n", cx, cy, radius, gc->erase ? "cc" : "c");
}

static void
eps_fill_polygon (hidGC gc, int n_coords, int *x, int *y)
{
  int i;
  char *op = "moveto";
  use_gc (gc);
  for (i = 0; i < n_coords; i++)
    {
      fprintf (f, "%d %d %s\n", x[i], y[i], op);
      op = "lineto";
    }
  fprintf (f, "fill\n");
}

static void
eps_fill_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  use_gc (gc);
  fprintf (f, "%d %d %d %d r\n", x1, y1, x2, y2);
}

static void
eps_calibrate (double xval, double yval)
{
  CRASH;
}

static void
eps_set_crosshair (int x, int y, int action)
{
}

static HID eps_hid = {
  sizeof (HID),
  "eps",
  "Encapsulated Postscript",
  0, 0, 1, 0, 1, 0,
  eps_get_export_options,
  eps_do_export,
  eps_parse_arguments,
  0 /* eps_invalidate_wh */ ,
  0 /* eps_invalidate_lr */ ,
  0 /* eps_invalidate_all */ ,
  eps_set_layer,
  eps_make_gc,
  eps_destroy_gc,
  eps_use_mask,
  eps_set_color,
  eps_set_line_cap,
  eps_set_line_width,
  eps_set_draw_xor,
  eps_set_draw_faded,
  eps_set_line_cap_angle,
  eps_draw_line,
  eps_draw_arc,
  eps_draw_rect,
  eps_fill_circle,
  eps_fill_polygon,
  eps_fill_rect,
  eps_calibrate,
  0 /* eps_shift_is_pressed */ ,
  0 /* eps_control_is_pressed */ ,
  0 /* eps_get_coords */ ,
  eps_set_crosshair,
  0 /* eps_add_timer */ ,
  0 /* eps_stop_timer */ ,
  0 /* eps_log */ ,
  0 /* eps_logv */ ,
  0 /* eps_confirm_dialog */ ,
  0 /* eps_close_confirm_dialog */ ,
  0 /* eps_report_dialog */ ,
  0 /* eps_prompt_for */ ,
  0 /* eps_attribute_dialog */ ,
  0 /* eps_show_item */ ,
  0				/* eps_beep */
};

void
hid_eps_init ()
{
  apply_default_hid (&eps_hid, 0);
  hid_register_hid (&eps_hid);
}
