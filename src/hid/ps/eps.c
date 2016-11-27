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
#include "pcb-printf.h"

#include "hid.h"
#include "hid_draw.h"
#include "../hidint.h"
#include "hid/common/hidnogui.h"
#include "hid/common/draw_helpers.h"
#include "../ps/ps.h"
#include "hid/common/hidinit.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented EPS function %s.\n", __FUNCTION__); abort()

/*----------------------------------------------------------------------------*/
/* Function prototypes                                                        */
/*----------------------------------------------------------------------------*/
static HID_Attribute * eps_get_export_options (int *n);
static void eps_do_export (HID_Attr_Val * options);
static void eps_parse_arguments (int *argc, char ***argv);
static int eps_set_layer (const char *name, int group, int empty);
static hidGC eps_make_gc (void);
static void eps_destroy_gc (hidGC gc);
static void eps_use_mask (enum mask_mode mode);
static void eps_set_color (hidGC gc, const char *name);
static void eps_set_line_cap (hidGC gc, EndCapStyle style);
static void eps_set_line_width (hidGC gc, Coord width);
static void eps_set_draw_xor (hidGC gc, int _xor);
static void eps_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);
static void eps_draw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);
static void eps_draw_arc (hidGC gc, Coord cx, Coord cy, Coord width, Coord height, Angle start_angle, Angle delta_angle);
static void eps_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);
static void eps_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius);
static void eps_fill_polygon (hidGC gc, int n_coords, Coord *x, Coord *y);
static void eps_calibrate (double xval, double yval);
static void eps_set_crosshair (int x, int y, int action);
/*----------------------------------------------------------------------------*/

typedef struct hid_gc_struct
{
  EndCapStyle cap;
  Coord width;
  int color;
  int erase;
} hid_gc_struct;

static HID eps_hid;
static HID_DRAW eps_graphics;

static FILE *f = 0;
static Coord linewidth = -1;
static int lastcap = -1;
static int lastcolor = -1;
static int print_group[MAX_GROUP];
static int print_layer[MAX_ALL_LAYER];
static int fast_erase = -1;

static HID_Attribute eps_attribute_list[] = {
  /* other HIDs expect this to be first.  */

/* %start-doc options "92 Encapsulated Postscript Export"
@ftable @code
@item --eps-file <string>
Name of the encapsulated postscript output file. Can contain a path.
@end ftable
%end-doc
*/
  {"eps-file", "Encapsulated Postscript output file",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_psfile 0

/* %start-doc options "92 Encapsulated Postscript Export"
@ftable @code
@item --eps-scale <num>
Scale EPS output by the parameter @samp{num}.
@end ftable
%end-doc
*/
  {"eps-scale", "EPS scale",
   HID_Real, 0, 100, {0, 0, 1.0}, 0, 0},
#define HA_scale 1

/* %start-doc options "92 Encapsulated Postscript Export"
@ftable @code
@cindex screen-layer-order (EPS)
@item --screen-layer-order
Export layers as shown on screen.
@end ftable
%end-doc
*/
  {"screen-layer-order", "Export layers in the order shown on screen",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_as_shown 2

/* %start-doc options "92 Encapsulated Postscript Export"
@ftable @code
@item --monochrome
Convert output to monochrome.
@end ftable
%end-doc
*/
  {"monochrome", "Convert to monochrome",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_mono 3

/* %start-doc options "92 Encapsulated Postscript Export"
@ftable @code
@cindex only-visible
@item --only-visible
Limit the bounds of the EPS file to the visible items.
@end ftable
%end-doc
*/
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
  CLEAR_FLAG(CHECKPLANESFLAG, PCB);

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
  for (i = 0; i < max_copper_layer; i++)
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
  for (i = 0; i < max_group; i++)
    if (print_group[i])
      fast_erase ++;

  /* If NO layers had anything on them, at least print the component
     layer to get the pins.  */
  if (fast_erase == 0)
    {
      print_group[GetLayerGroupNumberBySide (TOP_SIDE)] = 1;
      fast_erase = 1;
    }

  /* "fast_erase" is 1 if we can just paint white to erase.  */
  fast_erase = fast_erase == 1 ? 1 : 0;

  /* Now, for each group we're printing, mark its layers for
     printing.  */
  for (i = 0; i < max_copper_layer; i++)
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
      top_group = GetLayerGroupNumberBySide (TOP_SIDE);
      bottom_group = GetLayerGroupNumberBySide (BOTTOM_SIDE);
      qsort (LayerStack, max_copper_layer, sizeof (LayerStack[0]), layer_stack_sort);
    }
  fprintf (f, "%%!PS-Adobe-3.0 EPSF-3.0\n");
  linewidth = -1;
  lastcap = -1;
  lastcolor = -1;

  in_mono = options[HA_mono].int_value;

#define pcb2em(x) 1 + COORD_TO_INCH (x) * 72.0 * options[HA_scale].real_value
  fprintf (f, "%%%%BoundingBox: 0 0 %lli %lli\n",
	   llrint (pcb2em (bounds->X2 - bounds->X1)),
	   llrint (pcb2em (bounds->Y2 - bounds->Y1)) );
  fprintf (f, "%%%%HiResBoundingBox: 0.000000 0.000000 %.6f %.6f\n",
	   pcb2em (bounds->X2 - bounds->X1),
	   pcb2em (bounds->Y2 - bounds->Y1));
#undef pcb2em
  fprintf (f, "%%%%Pages: 1\n");
  fprintf (f,
	   "save countdictstack mark newpath /showpage {} def /setpagedevice {pop} def\n");
  fprintf (f, "%%%%EndProlog\n");
  fprintf (f, "%%%%Page: 1 1\n");
  fprintf (f, "%%%%BeginDocument: %s\n\n", filename);

  fprintf (f, "72 72 scale\n");
  fprintf (f, "1 dup neg scale\n");
  fprintf (f, "%g dup scale\n", options[HA_scale].real_value);
  pcb_fprintf (f, "%mi %mi translate\n", -bounds->X1, -bounds->Y2);
  if (options[HA_as_shown].int_value && Settings.ShowBottomSide)
    pcb_fprintf (f, "-1 1 scale %mi 0 translate\n", bounds->X1 - bounds->X2);
  linewidth = -1;
  lastcap = -1;
  lastcolor = -1;
#define Q (Coord) MIL_TO_COORD(10)
  pcb_fprintf (f,
	   "/nclip { %mi %mi moveto %mi %mi lineto %mi %mi lineto %mi %mi lineto %mi %mi lineto eoclip newpath } def\n",
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
  int save_ons[MAX_ALL_LAYER];

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
eps_set_layer (const char *name, int group, int empty)
{
  int idx = (group >= 0
	     && group <
	     max_group) ? PCB->LayerGroups.Entries[group][0] : group;
  if (name == 0)
    name = PCB->Data->Layer[idx].Name;

  if (idx >= 0 && idx < max_copper_layer && !print_layer[idx])
    return 0;
  if (SL_TYPE (idx) == SL_ASSY || SL_TYPE (idx) == SL_FAB)
    return 0;

  if (strcmp (name, "invisible") == 0)
    return 0;

  is_drill = (SL_TYPE (idx) == SL_PDRILL || SL_TYPE (idx) == SL_UDRILL);
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
eps_use_mask (enum mask_mode mode)
{
  static int mask_pending = 0;
  switch (mode)
    {
    case HID_MASK_CLEAR:
      if (!mask_pending)
	{
	  mask_pending = 1;
	  fprintf (f, "gsave\n");
	}
      break;
    case HID_MASK_BEFORE:
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
eps_set_line_width (hidGC gc, Coord width)
{
  gc->width = width;
}

static void
eps_set_draw_xor (hidGC gc, int xor_)
{
  ;
}

static void
use_gc (hidGC gc)
{
  if (linewidth != gc->width)
    {
      pcb_fprintf (f, "%mi setlinewidth\n", gc->width);
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

static void eps_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);
static void eps_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius);

static void
eps_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  use_gc (gc);
  pcb_fprintf (f, "%mi %mi %mi %mi r\n", x1, y1, x2, y2);
}

static void
eps_draw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  Coord w = gc->width / 2;
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
      Coord vx1 = x1 + dx;
      Coord vy1 = y1 + dy;

      pcb_fprintf (f, "%mi %mi moveto ", vx1, vy1);
      pcb_fprintf (f, "%mi %mi %mi %g %g arc\n", x2, y2, w, deg - 90, deg + 90);
      pcb_fprintf (f, "%mi %mi %mi %g %g arc\n", x1, y1, w, deg + 90, deg + 270);
      fprintf (f, "nclip\n");

      return;
    }
  pcb_fprintf (f, "%mi %mi %mi %mi %s\n", x1, y1, x2, y2, gc->erase ? "tc" : "t");
}

static void
eps_draw_arc (hidGC gc, Coord cx, Coord cy, Coord width, Coord height,
	      Angle start_angle, Angle delta_angle)
{
  Angle sa, ea;
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
  pcb_fprintf (f, "%ma %ma %mi %mi %mi %mi %g a\n",
	   sa, ea, -width, height, cx, cy, (double) linewidth / width);
}

static void
eps_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius)
{
  use_gc (gc);
  pcb_fprintf (f, "%mi %mi %mi %s\n", cx, cy, radius, gc->erase ? "cc" : "c");
}

static void
eps_fill_polygon (hidGC gc, int n_coords, Coord *x, Coord *y)
{
  int i;
  char *op = "moveto";
  use_gc (gc);
  for (i = 0; i < n_coords; i++)
    {
      pcb_fprintf (f, "%mi %mi %s\n", x[i], y[i], op);
      op = "lineto";
    }
  fprintf (f, "fill\n");
}

static void
eps_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  use_gc (gc);
  pcb_fprintf (f, "%mi %mi %mi %mi r\n", x1, y1, x2, y2);
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

void
hid_eps_init ()
{
  memset (&eps_hid, 0, sizeof (HID));
  memset (&eps_graphics, 0, sizeof (HID_DRAW));

  common_nogui_init (&eps_hid);
  common_draw_helpers_init (&eps_graphics);

  eps_hid.struct_size         = sizeof (HID);
  eps_hid.name                = "eps";
  eps_hid.description         = "Encapsulated Postscript";
  eps_hid.exporter            = 1;
  eps_hid.poly_after          = 1;

  eps_hid.get_export_options  = eps_get_export_options;
  eps_hid.do_export           = eps_do_export;
  eps_hid.parse_arguments     = eps_parse_arguments;
  eps_hid.set_layer           = eps_set_layer;
  eps_hid.calibrate           = eps_calibrate;
  eps_hid.set_crosshair       = eps_set_crosshair;

  eps_hid.graphics            = &eps_graphics;

  eps_graphics.make_gc        = eps_make_gc;
  eps_graphics.destroy_gc     = eps_destroy_gc;
  eps_graphics.use_mask       = eps_use_mask;
  eps_graphics.set_color      = eps_set_color;
  eps_graphics.set_line_cap   = eps_set_line_cap;
  eps_graphics.set_line_width = eps_set_line_width;
  eps_graphics.set_draw_xor   = eps_set_draw_xor;
  eps_graphics.draw_line      = eps_draw_line;
  eps_graphics.draw_arc       = eps_draw_arc;
  eps_graphics.draw_rect      = eps_draw_rect;
  eps_graphics.fill_circle    = eps_fill_circle;
  eps_graphics.fill_polygon   = eps_fill_polygon;
  eps_graphics.fill_rect      = eps_fill_rect;

  hid_register_hid (&eps_hid);
}
