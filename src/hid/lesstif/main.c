/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include "xincludes.h"

#include "global.h"
#include "data.h"
#include "action.h"
#include "crosshair.h"
#include "mymem.h"
#include "misc.h"

#include "hid.h"
#include "../hidint.h"
#include "lesstif.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

#ifndef XtRDouble
#define XtRDouble "Double"
#endif

typedef struct hid_gc_struct
{
  HID *me_pointer;
  Pixel color;
  const char *colorname;
  int width;
  EndCapStyle cap;
  char xor;
  char erase;
} hid_gc_struct;

extern HID lesstif_gui;
extern HID lesstif_extents;

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented GUI function %s\n", __FUNCTION__), abort()

XtAppContext app_context;
Widget appwidget;
Display *display;
static Window window = 0;
static Cursor my_cursor = 0;

/* The first is the "current" pixmap.  The main_ is the real one we
   usually use, the mask_ are the ones for doing polygon masks.  The
   pixmap is the saved pixels, the bitmap is for the "erase" color.
   We set pixmap to point to main_pixmap or mask_pixmap as needed.  */
static Pixmap pixmap = 0;
static Pixmap main_pixmap = 0;
static Pixmap mask_pixmap = 0;
static Pixmap mask_bitmap = 0;
static int use_mask = 0;

static int pixmap_w = 0, pixmap_h = 0;
Screen *screen_s;
int screen;
static Colormap colormap;
static GC my_gc = 0, bg_gc, clip_gc = 0, bset_gc = 0, bclear_gc = 0, mask_gc =
  0;
static Pixel bgcolor, offlimit_color, grid_color;
static int bgred, bggreen, bgblue;

/* These are for the pinout windows. */
typedef struct PinoutData
{
  struct PinoutData *prev, *next;
  Widget form;
  Window window;
  int left, right, top, bottom;	/* PCB extents of item */
  int x, y;			/* PCB coordinates of upper right corner of window */
  double zoom;			/* PCB units per screen pixel */
  int v_width, v_height;	/* pixels */
  void *item;
} PinoutData;

/* Linked list of all pinout windows.  */
static PinoutData *pinouts = 0;
/* If set, we are currently updating this pinout window.  */
static PinoutData *pinout = 0;

static int crosshair_x = 0, crosshair_y = 0;
static int in_move_event = 0;

Widget work_area, messages, command, hscroll, vscroll;
static Widget m_mark, m_crosshair, m_grid, m_zoom, m_mode, m_status;
Widget lesstif_m_layer;
Widget m_click;

/* This is the size, in pixels, of the viewport.  */
static int view_width, view_height;
/* This is the PCB location represented by the upper left corner of
   the viewport.  Note that PCB coordinates put 0,0 in the upper left,
   much like X does.  */
static int view_left_x = 0, view_top_y = 0;
/* Denotes PCB units per screen pixel.  Larger numbers mean zooming
   out - the largest value means you are looking at the whole
   board.  */
static double view_zoom = 1000;
static int flip_x = 0, flip_y = 0;
static int thindraw = 0;
static int thindrawpoly = 0;
static int autofade = 0;

static int
flag_thindraw (int x)
{
  return thindraw;
}
static int
flag_thindrawpoly (int x)
{
  return thindrawpoly;
}
static int
flag_flipx (int x)
{
  return flip_x;
}
static int
flag_flipy (int x)
{
  return flip_y;
}

HID_Flag lesstif_main_flag_list[] = {
  {"thindraw", flag_thindraw, 0},
  {"thindrawpoly", flag_thindrawpoly, 0},
  {"flip_x", flag_flipx, 0},
  {"flip_y", flag_flipy, 0}
};

REGISTER_FLAGS (lesstif_main_flag_list)

/* This is the size of the current PCB work area.  */
/* Use PCB->MaxWidth, PCB->MaxHeight.  */
/* static int pcb_width, pcb_height; */

static Arg args[30];
static int n;
#define stdarg(t,v) XtSetArg(args[n], t, v), n++


static int use_private_colormap = 0;
static int stdin_listen = 0;
static char *background_image_file = 0;

HID_Attribute lesstif_attribute_list[] = {
  {"install", "Install private colormap",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, &use_private_colormap},
#define HA_colormap 0

  {"listen", "Listen on standard input for actions",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, &stdin_listen},
#define HA_listen 1

  {"bg-image", "Background Image",
   HID_String, 0, 0, {0, 0, 0}, 0, &background_image_file},
#define HA_bg_image 1

};

REGISTER_ATTRIBUTES (lesstif_attribute_list)

static void lesstif_use_mask (int use_it);
static void zoom_to (double factor, int x, int y);
static void zoom_by (double factor, int x, int y);
static void pinout_callback (Widget, PinoutData *,
			     XmDrawingAreaCallbackStruct *);
static void pinout_unmap (Widget, PinoutData *, void *);

/* Px converts view->pcb, Vx converts pcb->view */

static inline int
Vx (int x)
{
  int rv = (x - view_left_x) / view_zoom + 0.5;
  if (flip_x)
    rv = view_width - rv;
  return rv;
}

static inline int
Vy (int y)
{
  int rv = (y - view_top_y) / view_zoom + 0.5;
  if (flip_y)
    rv = view_height - rv;
  return rv;
}

static inline int
Vz (int z)
{
  return z / view_zoom + 0.5;
}

static inline int
Px (int x)
{
  if (flip_x)
    x = view_width - x;
  return x * view_zoom + view_left_x;
}

static inline int
Py (int y)
{
  if (flip_y)
    y = view_height - y;
  return y * view_zoom + view_top_y;
}

static inline int
Pz (int z)
{
  return z * view_zoom;
}

void
lesstif_coords_to_pcb (int vx, int vy, int *px, int *py)
{
  *px = Px (vx);
  *py = Py (vy);
}

Pixel
lesstif_parse_color (char *value)
{
  XColor color;
  if (XParseColor (display, colormap, value, &color))
    if (XAllocColor (display, colormap, &color))
      return color.pixel;
  return 0;
}

static void
do_color (char *value, char *which)
{
  XColor color;
  if (XParseColor (display, colormap, value, &color))
    if (XAllocColor (display, colormap, &color))
      {
	stdarg (which, color.pixel);
      }
}

/* ------------------------------------------------------------ */

static char *
cur_clip ()
{
  if (TEST_FLAG (ORTHOMOVEFLAG, PCB))
    return "+";
  if (TEST_FLAG (ALLDIRECTIONFLAG, PCB))
    return "*";
  if (PCB->Clipping == 0)
    return "X";
  if (PCB->Clipping == 1)
    return "_/";
  return "\\_";
}

/* ---------------------------------------------------------------------- */

/* Local actions.  */

static int
PCBChanged (int argc, char **argv, int x, int y)
{
  if (work_area == 0)
    return 0;
  /*printf("PCB Changed! %d x %d\n", PCB->MaxWidth, PCB->MaxHeight); */
  n = 0;
  stdarg (XmNminimum, 0);
  stdarg (XmNvalue, 0);
  stdarg (XmNsliderSize, PCB->MaxWidth ? PCB->MaxWidth : 1);
  stdarg (XmNmaximum, PCB->MaxWidth ? PCB->MaxWidth : 1);
  XtSetValues (hscroll, args, n);
  n = 0;
  stdarg (XmNminimum, 0);
  stdarg (XmNvalue, 0);
  stdarg (XmNsliderSize, PCB->MaxHeight ? PCB->MaxHeight : 1);
  stdarg (XmNmaximum, PCB->MaxHeight ? PCB->MaxHeight : 1);
  XtSetValues (vscroll, args, n);
  zoom_by (1000000, 0, 0);

  hid_action ("NetlistChanged");
  hid_action ("LayersChanged");
  hid_action ("RouteStylesChanged");
  lesstif_sizes_reset ();
  lesstif_update_layer_groups ();
  while (pinouts)
    pinout_unmap (0, pinouts, 0);
  if (PCB->Filename)
    {
      char *cp = strrchr (PCB->Filename, '/');
      n = 0;
      stdarg (XmNtitle, cp ? cp + 1 : PCB->Filename);
      XtSetValues (appwidget, args, n);
    }
  return 0;
}


static const char setunits_syntax[] =
"SetUnits(mm|mil)";

static const char setunits_help[] =
"Set the default measurement units.";

/* %start-doc actions SetUnits

@table @code

@item mil
Sets the display units to mils (1/1000 inch).

@item mm
Sets the display units to millimeters.

@end table

%end-doc */

static int
SetUnits (int argc, char **argv, int x, int y)
{
  if (argc == 0)
    return 0;
  if (strcmp (argv[0], "mil") == 0)
    Settings.grid_units_mm = 0;
  if (strcmp (argv[0], "mm") == 0)
    Settings.grid_units_mm = 1;
  lesstif_sizes_reset ();
  lesstif_styles_update_values ();
  return 0;
}

static const char zoom_syntax[] =
"Zoom()\n"
"Zoom(factor)";

static const char zoom_help[] =
"Various zoom factor changes.";

/* %start-doc actions Zoom

Changes the zoom (magnification) of the view of the board.  If no
arguments are passed, the view is scaled such that the board just fits
inside the visible window (i.e. ``view all'').  Otherwise,
@var{factor} specifies a change in zoom factor.  It may be prefixed by
@code{+}, @code{-}, or @code{=} to change how the zoom factor is
modified.  The @var{factor} is a floating point number, such as
@code{1.5} or @code{0.75}.

@table @code

@item +@var{factor}
Values greater than 1.0 cause the board to be drawn smaller; more of
the board will be visible.  Values between 0.0 and 1.0 cause the board
to be drawn bigger; less of the board will be visible.

@item -@var{factor}
Values greater than 1.0 cause the board to be drawn bigger; less of
the board will be visible.  Values between 0.0 and 1.0 cause the board
to be drawn smaller; more of the board will be visible.

@item =@var{factor}

The @var{factor} is an absolute zoom factor; the unit for this value
is "PCB units per screen pixel".  Since PCB units are 0.01 mil, a
@var{factor} of 1000 means 10 mils (0.01 in) per pixel, or 100 DPI,
about the actual resolution of most screens - resulting in an "actual
size" board.  Similarly, a @var{factor} of 100 gives you a 10x actual
size.

@end table

Note that zoom factors of zero are silently ignored.

%end-doc */

static int
ZoomAction (int argc, char **argv, int x, int y)
{
  const char *vp;
  double v;
  if (x == 0 && y == 0)
    {
      x = view_width / 2;
      y = view_height / 2;
    }
  else
    {
      x = Vx (x);
      y = Vy (y);
    }
  if (argc < 1)
    {
      zoom_to (1000000, 0, 0);
      return 0;
    }
  vp = argv[0];
  if (*vp == '+' || *vp == '-' || *vp == '=')
    vp++;
  v = strtod (vp, 0);
  if (v <= 0)
    return 1;
  switch (argv[0][0])
    {
    case '-':
      zoom_by (1 / v, x, y);
      break;
    default:
    case '+':
      zoom_by (v, x, y);
      break;
    case '=':
      zoom_to (v, x, y);
      break;
    }
  return 0;
}


static const char thindraw_syntax[] =
"ThinDraw()\n"
"ThinDraw(1|0)";

static const char thindraw_help[] =
"Sets the global thin-draw flag.";

/* %start-doc actions ThinDraw

When the thindraw flag is set, all board objects are drawin using
``thin'' lines.  Traces are drawn as single lines along their
centerlines, other objects are drawn as outlines.  If you pass
@code{0}, thin draw is disabled.  If you pass @code{1}, thin draw is
enabled.  If you pass nothing, thin draw is toggled.

%end-doc */

static int
ThinDraw (int argc, char **argv, int x, int y)
{
  PinoutData *pd;
  if (argc == 0)
    thindraw = !thindraw;
  else if (argv[0][0] == '0')
    thindraw = 0;
  else
    thindraw = 1;
  lesstif_invalidate_all ();
  for (pd = pinouts; pd; pd = pd->next)
    pinout_callback (0, pd, 0);
  return 0;
}

static const char thindrawpoly_syntax[] =
"ThinDrawPoly()\n"
"ThinDrawPoly(0|1)";

static const char thindrawpoly_help[] =
"Sets the thin-draw flag for polygons.";

/* %start-doc actions ThinDrawPoly

When the polygon thindraw flag is set, all polygonss are drawin using
``thin'' lines along their outlines.  If you pass @code{0}, polygon
thin draw is disabled.  If you pass @code{1}, polygon thin draw is
enabled.  If you pass nothing, polygon thin draw is toggled.

%end-doc */

static int
ThinDrawPoly (int argc, char **argv, int x, int y)
{
  PinoutData *pd;
  if (argc == 0)
    thindrawpoly = !thindrawpoly;
  else if (argv[0][0] == '0')
    thindrawpoly = 0;
  else
    thindrawpoly = 1;
  lesstif_invalidate_all ();
  for (pd = pinouts; pd; pd = pd->next)
    pinout_callback (0, pd, 0);
  return 0;
}

static const char swapsides_syntax[] =
"SwapSides(|v|h|r)";

static const char swapsides_help[] =
"Swaps the side of the board you're looking at.";

/* %start-doc actions SwapSides

This action changes the way you view the board.

@table @code

@item v
Flips the board over vertically (up/down).

@item h
Flips the board over horizontally (left/right), like flipping pages in
a book.

@item r
Rotates the board 180 degrees without changing sides.

@end table

If no argument is given, the board isn't moved but the opposite side
is shown.

Normally, this action changes which pads and silk layer are drawn as
true silk, and which are drawn as the "invisible" layer.  It also
determines which solder mask you see.

As a special case, if the layer group for the side you're looking at
is visible and currently active, and the layer group for the opposite
is not visible (i.e. disabled), then this action will also swap which
layer group is visible and active, effectively swapping the ``working
side'' of the board.

%end-doc */

static int
SwapSides (int argc, char **argv, int x, int y)
{
  int comp_group = GetLayerGroupNumberByNumber (max_layer + COMPONENT_LAYER);
  int solder_group = GetLayerGroupNumberByNumber (max_layer + SOLDER_LAYER);
  int active_group = GetLayerGroupNumberByNumber (LayerStack[0]);
  int comp_showing =
    PCB->Data->Layer[PCB->LayerGroups.Entries[comp_group][0]].On;
  int solder_showing =
    PCB->Data->Layer[PCB->LayerGroups.Entries[solder_group][0]].On;

  if (argc > 0)
    {
      switch (argv[0][0]) {
      case 'h':
      case 'H':
	flip_x = ! flip_x;
	break;
      case 'v':
      case 'V':
	flip_y = ! flip_y;
	break;
      case 'r':
      case 'R':
	flip_x = ! flip_x;
	flip_y = ! flip_y;
	break;
      default:
	return 1;
      }
      /* SwapSides will swap this */
      Settings.ShowSolderSide = (flip_x == flip_y);
    }

  Settings.ShowSolderSide = !Settings.ShowSolderSide;
  if (Settings.ShowSolderSide)
    {
      if (active_group == comp_group && comp_showing && !solder_showing)
	{
	  ChangeGroupVisibility (PCB->LayerGroups.Entries[comp_group][0], 0,
				 0);
	  ChangeGroupVisibility (PCB->LayerGroups.Entries[solder_group][0], 1,
				 1);
	}
    }
  else
    {
      if (active_group == solder_group && solder_showing && !comp_showing)
	{
	  ChangeGroupVisibility (PCB->LayerGroups.Entries[solder_group][0], 0,
				 0);
	  ChangeGroupVisibility (PCB->LayerGroups.Entries[comp_group][0], 1,
				 1);
	}
    }
  lesstif_invalidate_all ();
  return 0;
}

static Widget m_cmd = 0, m_cmd_label;

static void
command_parse (char *s)
{
  int n = 0, ws = 1;
  char *cp;
  char **argv;

  for (cp = s; *cp; cp++)
    {
      if (isspace ((int) *cp))
	ws = 1;
      else
	{
	  n += ws;
	  ws = 0;
	}
    }
  argv = (char **) malloc ((n + 1) * sizeof (char *));

  n = 0;
  ws = 1;
  for (cp = s; *cp; cp++)
    {
      if (isspace ((int) *cp))
	{
	  ws = 1;
	  *cp = 0;
	}
      else
	{
	  if (ws)
	    argv[n++] = cp;
	  ws = 0;
	}
    }
  argv[n] = 0;
  lesstif_call_action (argv[0], n - 1, argv + 1);
}

static void
command_callback (Widget w, XtPointer uptr, XmTextVerifyCallbackStruct * cbs)
{
  char *s;
  switch (cbs->reason)
    {
    case XmCR_ACTIVATE:
      s = XmTextGetString (w);
      lesstif_show_crosshair (0);
      if (strchr (s, '('))
	hid_parse_actions (s, lesstif_call_action);
      else
	command_parse (s);
      XtFree (s);
      XmTextSetString (w, "");
    case XmCR_LOSING_FOCUS:
      XtUnmanageChild (m_cmd);
      XtUnmanageChild (m_cmd_label);
      break;
    }
}

static void
command_event_handler (Widget w, XtPointer p, XEvent * e, Boolean * cont)
{
  char buf[10];
  KeySym sym;
  int slen;

  switch (e->type)
    {
    case KeyPress:
      slen = XLookupString ((XKeyEvent *)e, buf, sizeof (buf), &sym, NULL);
      switch (sym)
	{
	case XK_Escape:
	  XtUnmanageChild (m_cmd);
	  XtUnmanageChild (m_cmd_label);
	  XmTextSetString (w, "");
	  *cont = False;
	  break;
	}
      break;
    }
}

static const char command_syntax[] =
"Command()";

static const char command_help[] =
"Displays the command line input window.";

/* %start-doc actions Command

The command window allows the user to manually enter actions to be
executed.  Action syntax can be done one of two ways:

@table @code

@item
Follow the action name by an open parenthesis, arguments separated by
commas, end with a close parenthesis.  Example: @code{Abc(1,2,3)}

@item
Separate the action name and arguments by spaces.  Example: @code{Abc
1 2 3}.

@end table

The first option allows you to have arguments with spaces in them,
but the second is more ``natural'' to type for most people.

Note that action names are not case sensitive, but arguments normally
are.  However, most actions will check for ``keywords'' in a case
insensitive way.

There are three ways to finish with the command window.  If you press
the @code{Enter} key, the command is invoked, the window goes away,
and the next time you bring up the command window it's empty.  If you
press the @code{Esc} key, the window goes away without invoking
anything, and the next time you bring up the command window it's
empty.  If you change focus away from the command window (i.e. click
on some other window), the command window goes away but the next time
you bring it up it resumes entering the command you were entering
before.

%end-doc */

static int
Command (int argc, char **argv, int x, int y)
{
  XtManageChild (m_cmd_label);
  XtManageChild (m_cmd);
  XmProcessTraversal (m_cmd, XmTRAVERSE_CURRENT);
  return 0;
}

static const char benchmark_syntax[] =
"Benchmark()";

static const char benchmark_help[] =
"Benchmark the GUI speed.";

/* %start-doc actions Benchmark

This action is used to speed-test the Lesstif graphics subsystem.  It
redraws the current screen as many times as possible in ten seconds.
It reports the amount of time needed to draw the screen once.

%end-doc */

static int
Benchmark (int argc, char **argv, int x, int y)
{
  int i = 0;
  time_t start, end;
  BoxType region;
  Drawable save_main;

  save_main = main_pixmap;
  main_pixmap = window;

  region.X1 = 0;
  region.Y1 = 0;
  region.X2 = PCB->MaxWidth;
  region.Y2 = PCB->MaxHeight;

  pixmap = window;
  XSync (display, 0);
  time (&start);
  do
    {
      XFillRectangle (display, pixmap, bg_gc, 0, 0, view_width, view_height);
      hid_expose_callback (&lesstif_gui, &region, 0);
      XSync (display, 0);
      time (&end);
      i++;
    }
  while (end - start < 10);

  printf ("%g redraws per second\n", i / 10.0);

  main_pixmap = save_main;
  return 0;
}

HID_Action lesstif_main_action_list[] = {
  {"PCBChanged", 0, PCBChanged,
   pcbchanged_help, pcbchanged_syntax},
  {"SetUnits", 0, SetUnits,
   setunits_help, setunits_syntax},
  {"Zoom", 0, ZoomAction,
   zoom_help, zoom_syntax},
  {"Thindraw", 0, ThinDraw,
   thindraw_help, thindraw_syntax},
  {"ThindrawPoly", 0, ThinDrawPoly,
   thindrawpoly_help, thindrawpoly_syntax},
  {"SwapSides", 0, SwapSides,
   swapsides_help, swapsides_syntax},
  {"Command", 0, Command,
   command_help, command_syntax},
  {"Benchmark", 0, Benchmark,
   benchmark_help, benchmark_syntax}
};

REGISTER_ACTIONS (lesstif_main_action_list)


/* ---------------------------------------------------------------------- 
 * redraws the background image
 */

static int bg_w, bg_h, bgi_w, bgi_h;
static Pixel **bg = 0;
static XImage *bgi = 0;
static enum {
  PT_unknown,
  PT_RGB565,
  PT_RGB888
} pixel_type = PT_unknown;

static void
LoadBackgroundFile (FILE *f, char *filename)
{
  XVisualInfo vinfot, *vinfo;
  Visual *vis;
  int c, r, b;
  int i, nret;
  int p[3], rows, cols, maxval;

  if (fgetc(f) != 'P')
    {
      printf("bgimage: %s signature not P6\n", filename);
      return;
    }
  if (fgetc(f) != '6')
    {
      printf("bgimage: %s signature not P6\n", filename);
      return;
    }
  for (i=0; i<3; i++)
    {
      do {
	b = fgetc(f);
	if (feof(f))
	  return;
	if (b == '#')
	  while (!feof(f) && b != '\n')
	    b = fgetc(f);
      } while (!isdigit(b));
      p[i] = b - '0';
      while (isdigit(b = fgetc(f)))
	p[i] = p[i]*10 + b - '0';
    }
  bg_w = cols = p[0];
  bg_h = rows = p[1];
  maxval = p[2];

  setbuf(stdout, 0);
  bg = (Pixel **) malloc (rows * sizeof (Pixel *));
  if (!bg)
    {
      printf("Out of memory loading %s\n", filename);
      return;
    }
  for (i=0; i<rows; i++)
    {
      bg[i] = (Pixel *) malloc (cols * sizeof (Pixel));
      if (!bg[i])
	{
	  printf("Out of memory loading %s\n", filename);
	  while (--i >= 0)
	    free (bg[i]);
	  free (bg);
	  bg = 0;
	  return;
	}
    }

  vis = DefaultVisual (display, DefaultScreen(display));

  vinfot.visualid = XVisualIDFromVisual(vis);
  vinfo = XGetVisualInfo (display, VisualIDMask, &vinfot, &nret);

#if 0
  /* If you want to support more visuals below, you'll probably need
     this. */
  printf("vinfo: rm %04x gm %04x bm %04x depth %d class %d\n",
	 vinfo->red_mask, vinfo->green_mask, vinfo->blue_mask,
	 vinfo->depth, vinfo->class);
#endif

  if (vinfo->class == TrueColor
      && vinfo->depth == 16
      && vinfo->red_mask == 0xf800
      && vinfo->green_mask == 0x07e0
      && vinfo->blue_mask == 0x001f)
    pixel_type = PT_RGB565;

  if (vinfo->class == TrueColor
      && vinfo->depth == 24
      && vinfo->red_mask == 0xff0000
      && vinfo->green_mask == 0x00ff00
      && vinfo->blue_mask == 0x0000ff)
    pixel_type = PT_RGB888;

  for (r=0; r<rows; r++)
    {
      for (c=0; c<cols; c++)
	{
	  XColor pix;
	  unsigned int pr = (unsigned)fgetc(f);
	  unsigned int pg = (unsigned)fgetc(f);
	  unsigned int pb = (unsigned)fgetc(f);

	  switch (pixel_type)
	    {
	    case PT_unknown:
	      pix.red = pr * 65535 / maxval;
	      pix.green = pg * 65535 / maxval;
	      pix.blue = pb * 65535 / maxval;
	      pix.flags = DoRed | DoGreen | DoBlue;
	      XAllocColor (display, colormap, &pix);
	      bg[r][c] = pix.pixel;
	      break;
	    case PT_RGB565:
	      bg[r][c] = (pr>>3)<<11 | (pg>>2)<<5 | (pb>>3);
	      break;
	    case PT_RGB888:
	      bg[r][c] = (pr << 16) | (pg << 8) | (pb);
	      break;
	    }
	}
    }
}

void
LoadBackgroundImage (char *filename)
{
  FILE *f = fopen(filename, "rb");
  if (!f)
    {
      if (NSTRCMP (filename, "pcb-background.ppm"))
	perror(filename);
      return;
    }
  LoadBackgroundFile (f, filename);
  fclose(f);
}

static void
DrawBackgroundImage ()
{
  int x, y, w, h;
  double xscale, yscale;
  int pcbwidth = PCB->MaxWidth / view_zoom;
  int pcbheight = PCB->MaxHeight / view_zoom;

  if (!window || !bg)
    return;

  if (!bgi || view_width != bgi_w || view_height != bgi_h)
    {
      if (bgi)
	XDestroyImage (bgi);
      /* Cheat - get the image, which sets up the format too.  */
      bgi = XGetImage (XtDisplay(work_area),
		       window,
		       0, 0, view_width, view_height,
		       -1, ZPixmap);
      bgi_w = view_width;
      bgi_h = view_height;
    }

  w = MIN (view_width, pcbwidth);
  h = MIN (view_height, pcbheight);

  xscale = (double)bg_w / PCB->MaxWidth;
  yscale = (double)bg_h / PCB->MaxHeight;

  for (y=0; y<h; y++)
    {
      int pr = Py(y);
      int ir = pr * yscale;
      for (x=0; x<w; x++)
	{
	  int pc = Px(x);
	  int ic = pc * xscale;
	  XPutPixel (bgi, x, y, bg[ir][ic]);
	}
    }
  XPutImage(display, main_pixmap, bg_gc,
	    bgi,
	    0, 0, 0, 0, w, h);
}
/* ---------------------------------------------------------------------- */

static HID_Attribute *
lesstif_get_export_options (int *n)
{
  return 0;
}

static void
set_scroll (Widget s, int pos, int view, int pcb)
{
  int sz = view * view_zoom;
  if (sz > pcb)
    sz = pcb;
  n = 0;
  stdarg (XmNvalue, pos);
  stdarg (XmNsliderSize, sz);
  stdarg (XmNincrement, view_zoom);
  stdarg (XmNpageIncrement, sz);
  stdarg (XmNmaximum, pcb);
  XtSetValues (s, args, n);
}

void
lesstif_pan_fixup ()
{
  if (view_left_x > PCB->MaxWidth - (view_width * view_zoom))
    view_left_x = PCB->MaxWidth - (view_width * view_zoom);
  if (view_top_y > PCB->MaxHeight - (view_height * view_zoom))
    view_top_y = PCB->MaxHeight - (view_height * view_zoom);
  if (view_left_x < 0)
    view_left_x = 0;
  if (view_top_y < 0)
    view_top_y = 0;
  if (view_width * view_zoom > PCB->MaxWidth
      && view_height * view_zoom > PCB->MaxHeight)
    {
      zoom_by (1, 0, 0);
      return;
    }

  set_scroll (hscroll, view_left_x, view_width, PCB->MaxWidth);
  set_scroll (vscroll, view_top_y, view_height, PCB->MaxHeight);

  lesstif_invalidate_all ();
}

static void
zoom_to (double new_zoom, int x, int y)
{
  double max_zoom, xfrac, yfrac;
  int cx, cy;

  xfrac = (double) x / (double) view_width;
  yfrac = (double) y / (double) view_height;

  if (flip_x)
    xfrac = 1-xfrac;
  if (flip_y)
    yfrac = 1-yfrac;

  max_zoom = PCB->MaxWidth / view_width;
  if (max_zoom < PCB->MaxHeight / view_height)
    max_zoom = PCB->MaxHeight / view_height;

  if (new_zoom < 1)
    new_zoom = 1;
  if (new_zoom > max_zoom)
    new_zoom = max_zoom;

  cx = view_left_x + view_width * xfrac * view_zoom;
  cy = view_top_y + view_height * yfrac * view_zoom;

  if (view_zoom != new_zoom)
    {
      view_zoom = new_zoom;
      pixel_slop = view_zoom;

      view_left_x = cx - view_width * xfrac * view_zoom;
      view_top_y = cy - view_height * yfrac * view_zoom;
    }
  lesstif_pan_fixup ();
}

void
zoom_by (double factor, int x, int y)
{
  zoom_to (view_zoom * factor, x, y);
}

static int panning = 0;
static int shift_pressed;
static int ctrl_pressed;

static void
Pan (int mode, int x, int y)
{
  static int ox, oy;
  static int opx, opy;
  panning = mode;
  if (ctrl_pressed)
    {
      opx = x * PCB->MaxWidth / view_width;
      opy = y * PCB->MaxHeight / view_height;
      if (flip_x)
	opx = PCB->MaxWidth - opx;
      if (flip_y)
	opy = PCB->MaxHeight - opy;
      view_left_x = opx - view_width / 2 * view_zoom;
      view_top_y = opy - view_height / 2 * view_zoom;
      lesstif_pan_fixup ();
    }
  else if (mode == 1)
    {
      ox = x;
      oy = y;
      opx = view_left_x;
      opy = view_top_y;
    }
  else
    {
      if (flip_x)
	view_left_x = opx + (x - ox) * view_zoom;
      else
	view_left_x = opx - (x - ox) * view_zoom;
      if (flip_y)
	view_top_y = opy + (y - oy) * view_zoom;
      else
	view_top_y = opy - (y - oy) * view_zoom;
      lesstif_pan_fixup ();
    }
}

static void
mod_changed (XKeyEvent * e, int set)
{
  switch (XKeycodeToKeysym (display, e->keycode, 0))
    {
    case XK_Shift_L:
    case XK_Shift_R:
      shift_pressed = set;
      break;
    case XK_Control_L:
    case XK_Control_R:
      ctrl_pressed = set;
      break;
    default:
      return;
    }
  in_move_event = 1;
  HideCrosshair (1);
  if (panning)
    Pan (2, e->x, e->y);
  EventMoveCrosshair (Px (e->x), Py (e->y));
  AdjustAttachedObjects ();
  RestoreCrosshair (1);
  in_move_event = 0;
}

static void
work_area_input (Widget w, XtPointer v, XEvent * e, Boolean * ctd)
{
  static int pressed_button = 0;
  static int ignore_release = 0;
  show_crosshair (0);
  switch (e->type)
    {
    case KeyPress:
      mod_changed (&(e->xkey), 1);
      if (lesstif_key_event (&(e->xkey)))
	return;
      break;
    case KeyRelease:
      mod_changed (&(e->xkey), 0);
      break;
    case ButtonPress:
      if (pressed_button)
	return;
      /*printf("click %d\n", e->xbutton.button); */
      if (lesstif_button_event (w, e))
	{
	  ignore_release = 1;
	  return;
	}
      ignore_release = 0;

      HideCrosshair (True);
      pressed_button = e->xbutton.button;
      shift_pressed = (e->xbutton.state & ShiftMask);
      ctrl_pressed = (e->xbutton.state & ControlMask);
      switch (e->xbutton.button)
	{
	case 1:
	  hid_actionl ("Mode", "Notify", NULL);
	  break;
	case 2:
	  break;
	case 3:
	  Pan (1, e->xbutton.x, e->xbutton.y);
	  break;
	case 4:
	  zoom_by (0.8, e->xbutton.x, e->xbutton.y);
	  break;
	case 5:
	  zoom_by (1.25, e->xbutton.x, e->xbutton.y);
	  break;
	}
      RestoreCrosshair (True);
      break;
    case ButtonRelease:
      if (e->xbutton.button != pressed_button)
	return;
      HideCrosshair (True);
      pressed_button = 0;
      /*printf("release %d\n", e->xbutton.button); */
      switch (e->xbutton.button)
	{
	case 1:
	  hid_actionl ("Mode", "Release", NULL);
	  break;
	case 2:
	  break;
	case 3:
	  Pan (0, e->xbutton.x, e->xbutton.y);
	  break;
	case 4:
	  break;
	case 5:
	  break;
	}
      RestoreCrosshair (True);
      break;
    case MotionNotify:
      {
	Window root, child;
	unsigned int keys_buttons;
	int root_x, root_y, pos_x, pos_y;
	while (XCheckMaskEvent (display, PointerMotionMask, e));
	XQueryPointer (display, e->xmotion.window, &root, &child,
		       &root_x, &root_y, &pos_x, &pos_y, &keys_buttons);
	shift_pressed = (keys_buttons & ShiftMask);
	ctrl_pressed = (keys_buttons & ControlMask);
	/*printf("m %d %d\n", Px(e->xmotion.x), Py(e->xmotion.y)); */
	in_move_event = 1;
	if (panning)
	  Pan (2, pos_x, pos_y);
	EventMoveCrosshair (Px (pos_x), Py (pos_y));
	in_move_event = 0;
      }
      break;
    case LeaveNotify:
      crosshair_x = crosshair_y = -1;
      CrosshairOff (1);
      need_idle_proc ();
      break;
    case EnterNotify:
      in_move_event = 1;
      EventMoveCrosshair (Px (e->xcrossing.x), Py (e->xcrossing.y));
      in_move_event = 0;
      CrosshairOn (1);
      need_idle_proc ();
      break;
    default:
      printf ("work_area: unknown event %d\n", e->type);
      break;
    }
}

void
lesstif_show_crosshair (int show)
{
  static int showing = 0;
  static int sx, sy;
  static GC xor_gc = 0;

  if (crosshair_x < 0 || !window)
    return;
  if (xor_gc == 0)
    {
      xor_gc = XCreateGC (display, window, 0, 0);
      XSetFunction (display, xor_gc, GXinvert);
    }
  if (show == showing)
    return;
  if (show)
    {
      sx = Vx (crosshair_x);
      sy = Vy (crosshair_y);
    }
  else
    need_idle_proc ();
  XDrawLine (display, window, xor_gc, 0, sy, view_width, sy);
  XDrawLine (display, window, xor_gc, sx, 0, sx, view_height);
  showing = show;
}

static void
work_area_expose (Widget work_area, void *me,
		  XmDrawingAreaCallbackStruct * cbs)
{
  XExposeEvent *e;

  show_crosshair (0);
  e = &(cbs->event->xexpose);
  XSetFunction (display, my_gc, GXcopy);
  XCopyArea (display, main_pixmap, window, my_gc,
	     e->x, e->y, e->width, e->height, e->x, e->y);
  show_crosshair (1);
}

static void
scroll_callback (Widget scroll, int *view_dim,
		 XmScrollBarCallbackStruct * cbs)
{
  *view_dim = cbs->value;
  lesstif_invalidate_all ();
}

static void
work_area_resize (Widget work_area, void *me,
		  XmDrawingAreaCallbackStruct * cbs)
{
  XColor color;
  Dimension width, height;

  show_crosshair (0);

  n = 0;
  stdarg (XtNwidth, &width);
  stdarg (XtNheight, &height);
  stdarg (XmNbackground, &bgcolor);
  XtGetValues (work_area, args, n);
  view_width = width;
  view_height = height;

  color.pixel = bgcolor;
  XQueryColor (display, colormap, &color);
  bgred = color.red;
  bggreen = color.green;
  bgblue = color.blue;

  if (!window)
    return;

#if 0
  if (!pixmap || view_width > pixmap_w || view_height > pixmap_h)
    {
      if (pixmap_w < view_width)
#endif
	pixmap_w = view_width;
#if 0
      if (pixmap_h < view_height)
#endif
	pixmap_h = view_height;

      if (main_pixmap)
	XFreePixmap (display, main_pixmap);
      main_pixmap =
	XCreatePixmap (display, window, pixmap_w, pixmap_h,
		       XDefaultDepth (display, screen));

      if (mask_pixmap)
	XFreePixmap (display, mask_pixmap);
      mask_pixmap =
	XCreatePixmap (display, window, pixmap_w, pixmap_h,
		       XDefaultDepth (display, screen));

      if (mask_bitmap)
	XFreePixmap (display, mask_bitmap);
      mask_bitmap = XCreatePixmap (display, window, pixmap_w, pixmap_h, 1);

      pixmap = use_mask ? main_pixmap : mask_pixmap;
#if 0
    }
#endif

  zoom_by (1, 0, 0);
}

static void
work_area_first_expose (Widget work_area, void *me,
			XmDrawingAreaCallbackStruct * cbs)
{
  Dimension width, height;

  window = XtWindow (work_area);
  my_gc = XCreateGC (display, window, 0, 0);

  n = 0;
  stdarg (XtNwidth, &width);
  stdarg (XtNheight, &height);
  stdarg (XmNbackground, &bgcolor);
  XtGetValues (work_area, args, n);
  view_width = width;
  view_height = height;

  offlimit_color = lesstif_parse_color (Settings.OffLimitColor);
  grid_color = lesstif_parse_color (Settings.GridColor);

  bg_gc = XCreateGC (display, window, 0, 0);
  XSetForeground (display, bg_gc, bgcolor);

  main_pixmap =
    XCreatePixmap (display, window, width, height,
		   XDefaultDepth (display, screen));
  mask_pixmap =
    XCreatePixmap (display, window, width, height,
		   XDefaultDepth (display, screen));
  mask_bitmap = XCreatePixmap (display, window, width, height, 1);
  pixmap = main_pixmap;
  pixmap_w = width;
  pixmap_h = height;

  clip_gc = XCreateGC (display, window, 0, 0);
  bset_gc = XCreateGC (display, mask_bitmap, 0, 0);
  XSetForeground (display, bset_gc, 1);
  bclear_gc = XCreateGC (display, mask_bitmap, 0, 0);
  XSetForeground (display, bclear_gc, 0);

  XtRemoveCallback (work_area, XmNexposeCallback,
		    (XtCallbackProc) work_area_first_expose, 0);
  XtAddCallback (work_area, XmNexposeCallback,
		 (XtCallbackProc) work_area_expose, 0);
  lesstif_invalidate_all ();
}

static Widget
make_message (char *name, Widget left, int resizeable)
{
  Widget w, f;
  n = 0;
  if (left)
    {
      stdarg (XmNleftAttachment, XmATTACH_WIDGET);
      stdarg (XmNleftWidget, left);
    }
  else
    {
      stdarg (XmNleftAttachment, XmATTACH_FORM);
    }
  stdarg (XmNtopAttachment, XmATTACH_FORM);
  stdarg (XmNbottomAttachment, XmATTACH_FORM);
  stdarg (XmNshadowType, XmSHADOW_IN);
  stdarg (XmNshadowThickness, 1);
  stdarg (XmNalignment, XmALIGNMENT_CENTER);
  stdarg (XmNmarginWidth, 4);
  stdarg (XmNmarginHeight, 1);
  if (!resizeable)
    stdarg (XmNresizePolicy, XmRESIZE_GROW);
  f = XmCreateForm (messages, name, args, n);
  XtManageChild (f);
  n = 0;
  stdarg (XmNtopAttachment, XmATTACH_FORM);
  stdarg (XmNbottomAttachment, XmATTACH_FORM);
  stdarg (XmNleftAttachment, XmATTACH_FORM);
  stdarg (XmNrightAttachment, XmATTACH_FORM);
  w = XmCreateLabel (f, name, args, n);
  XtManageChild (w);
  return w;
}

static void
lesstif_do_export (HID_Attr_Val * options)
{
  Dimension width, height;
  Widget menu;
  Widget work_area_frame;

  n = 0;
  stdarg (XtNwidth, &width);
  stdarg (XtNheight, &height);
  XtGetValues (appwidget, args, n);

  if (width < 1)
    width = 400;
  if (width > XDisplayWidth (display, screen))
    width = XDisplayWidth (display, screen);
  if (height < 1)
    height = 300;
  if (height > XDisplayHeight (display, screen))
    height = XDisplayHeight (display, screen);

  n = 0;
  stdarg (XmNwidth, width);
  stdarg (XmNheight, height);
  XtSetValues (appwidget, args, n);

  stdarg (XmNspacing, 0);
  mainwind = XmCreateMainWindow (appwidget, "mainWind", args, n);
  XtManageChild (mainwind);

  n = 0;
  stdarg (XmNmarginWidth, 0);
  stdarg (XmNmarginHeight, 0);
  menu = lesstif_menu (mainwind, "menubar", args, n);
  XtManageChild (menu);

  n = 0;
  stdarg (XmNshadowType, XmSHADOW_IN);
  work_area_frame =
    XmCreateFrame (mainwind, "work_area_frame", args, n);
  XtManageChild (work_area_frame);

  n = 0;
  do_color (Settings.BackgroundColor, XmNbackground);
  work_area = XmCreateDrawingArea (work_area_frame, "work_area", args, n);
  XtManageChild (work_area);
  XtAddCallback (work_area, XmNexposeCallback,
		 (XtCallbackProc) work_area_first_expose, 0);
  XtAddCallback (work_area, XmNresizeCallback,
		 (XtCallbackProc) work_area_resize, 0);
  /* A regular callback won't work here, because lesstif swallows any
     Ctrl<Button>1 event.  */
  XtAddEventHandler (work_area,
		     ButtonPressMask | ButtonReleaseMask
		     | PointerMotionMask | PointerMotionHintMask
		     | KeyPressMask | KeyReleaseMask
		     | EnterWindowMask | LeaveWindowMask,
		     0, work_area_input, 0);

  n = 0;
  stdarg (XmNorientation, XmVERTICAL);
  stdarg (XmNprocessingDirection, XmMAX_ON_BOTTOM);
  stdarg (XmNmaximum, PCB->MaxHeight ? PCB->MaxHeight : 1);
  vscroll = XmCreateScrollBar (mainwind, "vscroll", args, n);
  XtAddCallback (vscroll, XmNvalueChangedCallback,
		 (XtCallbackProc) scroll_callback, (XtPointer) & view_top_y);
  XtAddCallback (vscroll, XmNdragCallback, (XtCallbackProc) scroll_callback,
		 (XtPointer) & view_top_y);
  XtManageChild (vscroll);

  n = 0;
  stdarg (XmNorientation, XmHORIZONTAL);
  stdarg (XmNmaximum, PCB->MaxWidth ? PCB->MaxWidth : 1);
  hscroll = XmCreateScrollBar (mainwind, "hscroll", args, n);
  XtAddCallback (hscroll, XmNvalueChangedCallback,
		 (XtCallbackProc) scroll_callback, (XtPointer) & view_left_x);
  XtAddCallback (hscroll, XmNdragCallback, (XtCallbackProc) scroll_callback,
		 (XtPointer) & view_left_x);
  XtManageChild (hscroll);

  n = 0;
  stdarg (XmNresize, True);
  stdarg (XmNresizePolicy, XmRESIZE_ANY);
  messages = XmCreateForm (mainwind, "messages", args, n);
  XtManageChild (messages);

  n = 0;
  stdarg (XmNtopAttachment, XmATTACH_FORM);
  stdarg (XmNbottomAttachment, XmATTACH_FORM);
  stdarg (XmNleftAttachment, XmATTACH_FORM);
  stdarg (XmNrightAttachment, XmATTACH_FORM);
  stdarg (XmNalignment, XmALIGNMENT_CENTER);
  stdarg (XmNshadowThickness, 2);
  m_click = XmCreateLabel (messages, "click", args, n);

  n = 0;
  stdarg (XmNtopAttachment, XmATTACH_FORM);
  stdarg (XmNbottomAttachment, XmATTACH_FORM);
  stdarg (XmNleftAttachment, XmATTACH_FORM);
  stdarg (XmNlabelString, XmStringCreateLocalized ("Command: "));
  m_cmd_label = XmCreateLabel (messages, "command", args, n);

  n = 0;
  stdarg (XmNtopAttachment, XmATTACH_FORM);
  stdarg (XmNbottomAttachment, XmATTACH_FORM);
  stdarg (XmNleftAttachment, XmATTACH_WIDGET);
  stdarg (XmNleftWidget, m_cmd_label);
  stdarg (XmNrightAttachment, XmATTACH_FORM);
  stdarg (XmNshadowThickness, 1);
  stdarg (XmNhighlightThickness, 0);
  stdarg (XmNmarginWidth, 2);
  stdarg (XmNmarginHeight, 2);
  m_cmd = XmCreateTextField (messages, "command", args, n);
  XtAddCallback (m_cmd, XmNactivateCallback,
		 (XtCallbackProc) command_callback, 0);
  XtAddCallback (m_cmd, XmNlosingFocusCallback,
		 (XtCallbackProc) command_callback, 0);
  XtAddEventHandler (m_cmd, KeyPressMask, 0, command_event_handler, 0);

  m_mark = make_message ("m_mark", 0, 0);
  XtUnmanageChild (XtParent (m_mark));
  m_crosshair = make_message ("m_crosshair", m_mark, 0);
  m_grid = make_message ("m_grid", m_crosshair, 1);
  m_zoom = make_message ("m_zoom", m_grid, 1);
  lesstif_m_layer = make_message ("m_layer", m_zoom, 0);
  m_mode = make_message ("m_mode", lesstif_m_layer, 1);
  m_status = make_message ("m_status", m_mode, 1);

  n = 0;
  stdarg (XmNrightAttachment, XmATTACH_FORM);
  XtSetValues (XtParent (m_status), args, n);

  /* We'll use this later.  */
  n = 0;
  stdarg (XmNleftWidget, XtParent (m_mark));
  XtSetValues (XtParent (m_crosshair), args, n);

  n = 0;
  stdarg (XmNmessageWindow, messages);
  XtSetValues (mainwind, args, n);

  if (background_image_file)
    LoadBackgroundImage (background_image_file);

  XtRealizeWidget (appwidget);

  while (!window)
    {
      XEvent e;
      XtAppNextEvent (app_context, &e);
      XtDispatchEvent (&e);
    }

  PCBChanged (0, 0, 0, 0);

  XtAppMainLoop (app_context);
}

#if 0
XrmOptionDescRec lesstif_options[] = {
};

XtResource lesstif_resources[] = {
};
#endif

typedef union
{
  int i;
  double f;
  char *s;
} val_union;

static Boolean
cvtres_string_to_double (Display * d, XrmValue * args, Cardinal * num_args,
			 XrmValue * from, XrmValue * to,
			 XtPointer * converter_data)
{
  static double rv;
  rv = strtod ((char *) from->addr, 0);
  if (to->addr)
    *(double *) to->addr = rv;
  else
    to->addr = (XPointer) & rv;
  to->size = sizeof (rv);
  return True;
}

static void
mainwind_delete_cb ()
{
  hid_action ("Quit");
}

static void
lesstif_listener_cb (XtPointer client_data, int *fid, XtInputId *id)
{
  char buf[BUFSIZ];
  int nbytes;

  if ((nbytes = read (*fid, buf, BUFSIZ)) == -1)
    perror ("lesstif_listener_cb");

  if (nbytes)
    {
      buf[nbytes] = '\0';
      hid_parse_actions (buf, NULL);
    }
}


static void
lesstif_parse_arguments (int *argc, char ***argv)
{
  Atom close_atom;
  HID_AttrNode *ha;
  int acount = 0, amax;
  int rcount = 0, rmax;
  int i;
  XrmOptionDescRec *new_options;
  XtResource *new_resources;
  val_union *new_values;

  XtSetTypeConverter (XtRString,
		      XtRDouble,
		      cvtres_string_to_double, NULL, 0, XtCacheAll, NULL);

  for (ha = hid_attr_nodes; ha; ha = ha->next)
    for (i = 0; i < ha->n; i++)
      {
	HID_Attribute *a = ha->attributes + i;
	switch (a->type)
	  {
	  case HID_Integer:
	  case HID_Real:
	  case HID_String:
	  case HID_Path:
	  case HID_Boolean:
	    acount++;
	    rcount++;
	    break;
	  default:
	    break;
	  }
      }

#if 0
  amax = acount + XtNumber (lesstif_options);
#else
  amax = acount;
#endif

  new_options = malloc ((amax + 1) * sizeof (XrmOptionDescRec));

#if 0
  memcpy (new_options + acount, lesstif_options, sizeof (lesstif_options));
#endif
  acount = 0;

#if 0
  rmax = rcount + XtNumber (lesstif_resources);
#else
  rmax = rcount;
#endif

  new_resources = malloc ((rmax + 1) * sizeof (XtResource));
  new_values = malloc ((rmax + 1) * sizeof (val_union));
#if 0
  memcpy (new_resources + acount, lesstif_resources,
	  sizeof (lesstif_resources));
#endif
  rcount = 0;

  for (ha = hid_attr_nodes; ha; ha = ha->next)
    for (i = 0; i < ha->n; i++)
      {
	HID_Attribute *a = ha->attributes + i;
	XrmOptionDescRec *o = new_options + acount;
	char *tmpopt, *tmpres;
	XtResource *r = new_resources + rcount;

	tmpopt = (char *) malloc (strlen (a->name) + 3);
	tmpopt[0] = tmpopt[1] = '-';
	strcpy (tmpopt + 2, a->name);
	o->option = tmpopt;

	tmpres = (char *) malloc (strlen (a->name) + 2);
	tmpres[0] = '*';
	strcpy (tmpres + 1, a->name);
	o->specifier = tmpres;

	switch (a->type)
	  {
	  case HID_Integer:
	  case HID_Real:
	  case HID_String:
	  case HID_Path:
	    o->argKind = XrmoptionSepArg;
	    o->value = 0;
	    acount++;
	    break;
	  case HID_Boolean:
	    o->argKind = XrmoptionNoArg;
	    o->value = "True";
	    acount++;
	    break;
	  default:
	    break;
	  }

	r->resource_name = a->name;
	r->resource_class = a->name;
	r->resource_offset = sizeof (val_union) * rcount;

	switch (a->type)
	  {
	  case HID_Integer:
	    r->resource_type = XtRInt;
	    r->default_type = XtRInt;
	    r->resource_size = sizeof (int);
	    r->default_addr = &(a->default_val.int_value);
	    rcount++;
	    break;
	  case HID_Real:
	    r->resource_type = XtRDouble;
	    r->default_type = XtRDouble;
	    r->resource_size = sizeof (double);
	    r->default_addr = &(a->default_val.real_value);
	    rcount++;
	    break;
	  case HID_String:
	  case HID_Path:
	    r->resource_type = XtRString;
	    r->default_type = XtRString;
	    r->resource_size = sizeof (char *);
	    r->default_addr = a->default_val.str_value;
	    rcount++;
	    break;
	  case HID_Boolean:
	    r->resource_type = XtRBoolean;
	    r->default_type = XtRInt;
	    r->resource_size = sizeof (int);
	    r->default_addr = &(a->default_val.int_value);
	    rcount++;
	    break;
	  default:
	    break;
	  }
      }
#if 0
  for (i = 0; i < XtNumber (lesstif_resources); i++)
    {
      XtResource *r = new_resources + rcount;
      r->resource_offset = sizeof (val_union) * rcount;
      rcount++;
    }
#endif

  n = 0;
  stdarg (XmNdeleteResponse, XmDO_NOTHING);

  appwidget = XtAppInitialize (&app_context,
			       "Pcb",
			       new_options, amax, argc, *argv, 0, args, n);

  display = XtDisplay (appwidget);
  screen_s = XtScreen (appwidget);
  screen = XScreenNumberOfScreen (screen_s);
  colormap = XDefaultColormap (display, screen);

  close_atom = XmInternAtom (display, "WM_DELETE_WINDOW", 0);
  XmAddWMProtocolCallback (appwidget, close_atom,
			   (XtCallbackProc) mainwind_delete_cb, 0);

  /*  XSynchronize(display, True); */

  XtGetApplicationResources (appwidget, new_values, new_resources,
			     rmax, 0, 0);

  rcount = 0;
  for (ha = hid_attr_nodes; ha; ha = ha->next)
    for (i = 0; i < ha->n; i++)
      {
	HID_Attribute *a = ha->attributes + i;
	val_union *v = new_values + rcount;
	switch (a->type)
	  {
	  case HID_Integer:
	    if (a->value)
	      *(int *) a->value = v->i;
	    else
	      a->default_val.int_value = v->i;
	    rcount++;
	    break;
	  case HID_Boolean:
	    if (a->value)
	      *(char *) a->value = v->i;
	    else
	      a->default_val.int_value = v->i;
	    rcount++;
	    break;
	  case HID_Real:
	    if (a->value)
	      *(double *) a->value = v->f;
	    else
	      a->default_val.real_value = v->f;
	    rcount++;
	    break;
	  case HID_String:
	  case HID_Path:
	    if (a->value)
	      *(char **) a->value = v->s;
	    else
	      a->default_val.str_value = v->s;
	    rcount++;
	    break;
	  default:
	    break;
	  }
      }

  /* redefine colormap, if requested via "-install" */
  if (use_private_colormap)
    {
      colormap = XCopyColormapAndFree (display, colormap);
      XtVaSetValues (appwidget, XtNcolormap, colormap, NULL);
    }

  /* listen on standard input for actions */
  if (stdin_listen)
    {
      XtAppAddInput (app_context, fileno (stdin), (XtPointer) XtInputReadMask,
		     lesstif_listener_cb, NULL);
    }
}

static void
draw_grid ()
{
  static XPoint *points = 0;
  static int npoints = 0;
  int x1, y1, x2, y2, n, prevx;
  double x, y;
  static GC grid_gc = 0;

  if (!Settings.DrawGrid)
    return;
  if (Vz (PCB->Grid) < MIN_GRID_DISTANCE)
    return;
  if (!grid_gc)
    {
      grid_gc = XCreateGC (display, window, 0, 0);
      XSetFunction (display, grid_gc, GXxor);
      XSetForeground (display, grid_gc, grid_color);
    }
  if (flip_x)
    {
      x2 = GRIDFIT_X (Px (0), PCB->Grid);
      x1 = GRIDFIT_X (Px (view_width), PCB->Grid);
      if (Vx (x2) < 0)
	x2 -= PCB->Grid;
      if (Vx (x1) >= view_width)
	x1 += PCB->Grid;
    }
  else
    {
      x1 = GRIDFIT_X (Px (0), PCB->Grid);
      x2 = GRIDFIT_X (Px (view_width), PCB->Grid);
      if (Vx (x1) < 0)
	x1 += PCB->Grid;
      if (Vx (x2) >= view_width)
	x2 -= PCB->Grid;
    }
  if (flip_y)
    {
      y2 = GRIDFIT_Y (Py (0), PCB->Grid);
      y1 = GRIDFIT_Y (Py (view_height), PCB->Grid);
      if (Vy (y2) < 0)
	y2 -= PCB->Grid;
      if (Vy (y1) >= view_height)
	y1 += PCB->Grid;
    }
  else
    {
      y1 = GRIDFIT_Y (Py (0), PCB->Grid);
      y2 = GRIDFIT_Y (Py (view_height), PCB->Grid);
      if (Vy (y1) < 0)
	y1 += PCB->Grid;
      if (Vy (y2) >= view_height)
	y2 -= PCB->Grid;
    }
  n = (int) ((x2 - x1) / PCB->Grid + 0.5) + 1;
  if (n > npoints)
    {
      npoints = n + 10;
      points =
	MyRealloc (points, npoints * sizeof (XPoint), "lesstif_draw_grid");
    }
  n = 0;
  prevx = 0;
  for (x = x1; x <= x2; x += PCB->Grid)
    {
      int temp = Vx (x);
      points[n].x = temp;
      if (n)
	{
	  points[n].x -= prevx;
	  points[n].y = 0;
	}
      prevx = temp;
      n++;
    }
  for (y = y1; y <= y2; y += PCB->Grid)
    {
      int i;
      int vy = Vy (y);
      points[0].y = vy;
      XDrawPoints (display, pixmap, grid_gc, points, n, CoordModePrevious);
    }
}

static int
coords_to_widget (int x, int y, Widget w, int prev_state)
{
  int this_state = prev_state;
  static char buf[60];
  double dx, dy, g;
  int frac = 0;
  XmString ms;

  if (Settings.grid_units_mm)
    {
      /* MM */
      dx = PCB_TO_MM (x);
      dy = PCB_TO_MM (y);
      g = PCB_TO_MM (PCB->Grid);
    }
  else
    {
      /* Mils */
      dx = PCB_TO_MIL (x);
      dy = PCB_TO_MIL (y);
      g = PCB_TO_MIL (PCB->Grid);
    }
  if (x < 0 && prev_state >= 0)
    buf[0] = 0;
  else if (((int) (g * 10000 + 0.5) % 10000) == 0)
    {
      const char *fmt = prev_state < 0 ? "%+d, %+d" : "%d, %d";
      sprintf (buf, fmt, (int) (dx + 0.5), (int) (dy + 0.5));
      this_state = 2 + Settings.grid_units_mm;
      frac = 0;
    }
  else if (PCB->Grid <= 20 && Settings.grid_units_mm)
    {
      const char *fmt = prev_state < 0 ? "%+.3f, %+.3f" : "%.3f, %.3f";
      sprintf (buf, fmt, dx, dy);
      this_state = 4 + Settings.grid_units_mm;
      frac = 1;
    }
  else
    {
      const char *fmt = prev_state < 0 ? "%+.2f, %+.2f" : "%.2f, %.2f";
      sprintf (buf, fmt, dx, dy);
      this_state = 4 + Settings.grid_units_mm;
      frac = 1;
    }
  if (prev_state < 0 && (x || y))
    {
      int angle = atan2 (dy, -dx) * 180 / M_PI;
      double dist = sqrt (dx * dx + dy * dy);
      if (frac)
	sprintf (buf + strlen (buf), " (%.2f", dist);
      else
	sprintf (buf + strlen (buf), " (%d", (int) (dist + 0.5));
      sprintf (buf + strlen (buf), ", %d\260)", angle);
    }
  ms = XmStringCreateLocalized (buf);
  n = 0;
  stdarg (XmNlabelString, ms);
  XtSetValues (w, args, n);
  return this_state;
}

static char *
pcb2str (int pcbval)
{
  static char buf[20][20];
  static int bufp = 0;
  double d;

  bufp = (bufp + 1) % 20;
  if (Settings.grid_units_mm)
    d = PCB_TO_MM (pcbval);
  else
    d = PCB_TO_MIL (pcbval);

  if ((int) (d * 100 + 0.5) == (int) (d + 0.005) * 100)
    sprintf (buf[bufp], "%d", (int) d);
  else
    sprintf (buf[bufp], "%.2f", d);
  return buf[bufp];
}

#define u(x) pcb2str(x)
#define S Settings

void
lesstif_update_status_line ()
{
  char buf[100];
  char *s45 = cur_clip ();
  XmString xs;

  buf[0] = 0;
  switch (Settings.Mode)
    {
    case VIA_MODE:
      sprintf (buf, "%s/%s \370=%s", u (S.ViaThickness),
	       u (S.Keepaway), u (S.ViaDrillingHole));
      break;
    case LINE_MODE:
    case ARC_MODE:
      sprintf (buf, "%s/%s %s", u (S.LineThickness), u (S.Keepaway), s45);
      break;
    case RECTANGLE_MODE:
    case POLYGON_MODE:
      sprintf (buf, "%s %s", u (S.Keepaway), s45);
      break;
    case TEXT_MODE:
      sprintf (buf, "%s", u (S.TextScale));
      break;
    case MOVE_MODE:
    case COPY_MODE:
    case INSERTPOINT_MODE:
    case RUBBERBANDMOVE_MODE:
      sprintf (buf, "%s", s45);
      break;
    case NO_MODE:
    case PASTEBUFFER_MODE:
    case ROTATE_MODE:
    case REMOVE_MODE:
    case THERMAL_MODE:
    case ARROW_MODE:
    case LOCK_MODE:
      break;
    }

  xs = XmStringCreateLocalized (buf);
  n = 0;
  stdarg (XmNlabelString, xs);
  XtSetValues (m_status, args, n);
}

#undef u
#undef S

static int idle_proc_set = 0;
static int need_redraw = 0;

static Boolean
idle_proc (XtPointer dummy)
{
  if (need_redraw)
    {
      int mx, my;
      BoxType region;
      lesstif_use_mask (0);
      Crosshair.On = 0;
      pixmap = main_pixmap;
      mx = view_width;
      my = view_height;
      region.X1 = Px (0);
      region.Y1 = Py (0);
      region.X2 = Px (view_width);
      region.Y2 = Py (view_height);
      if (flip_x)
	{
	  int tmp = region.X1;
	  region.X1 = region.X2;
	  region.X2 = tmp;
	}
      if (flip_y)
	{
	  int tmp = region.Y1;
	  region.Y1 = region.Y2;
	  region.Y2 = tmp;
	}
      XSetForeground (display, bg_gc, bgcolor);
      XFillRectangle (display, main_pixmap, bg_gc, 0, 0, mx, my);
      if (region.X2 > PCB->MaxWidth || region.Y2 > PCB->MaxHeight)
	{
	  XSetForeground (display, bg_gc, offlimit_color);
	  if (region.X2 > PCB->MaxWidth)
	    {
	      mx = Vx (PCB->MaxWidth);
	      if (flip_x)
		XFillRectangle (display, main_pixmap, bg_gc, 0, 0,
				mx-1, my);
	      else
		XFillRectangle (display, main_pixmap, bg_gc, mx+1, 0,
				view_width - mx + 1, my);

	    }
	  if (region.Y2 > PCB->MaxHeight)
	    {
	      my = Vy (PCB->MaxHeight) + 1;
	      if (flip_y)
		XFillRectangle (display, main_pixmap, bg_gc, 0, 0, mx,
				my-1);
	      else
		XFillRectangle (display, main_pixmap, bg_gc, 0, my, mx,
				view_height - my + 1);
	    }
	}
      DrawBackgroundImage();
      hid_expose_callback (&lesstif_gui, &region, 0);
      draw_grid ();
      lesstif_use_mask (0);
      XSetFunction (display, my_gc, GXcopy);
      XCopyArea (display, main_pixmap, window, my_gc, 0, 0, view_width,
		 view_height, 0, 0);
      pixmap = window;
      CrosshairOn (0);
      need_redraw = 0;
    }

  {
    static int c_x = -2, c_y = -2;
    static MarkType saved_mark;
    if (crosshair_x != c_x || crosshair_y != c_y
	|| memcmp (&saved_mark, &Marked, sizeof (MarkType)))
      {
	static int last_state = 0;
	static int this_state = 0;

	c_x = crosshair_x;
	c_y = crosshair_y;

	this_state =
	  coords_to_widget (crosshair_x, crosshair_y, m_crosshair,
			    this_state);
	if (Marked.status)
	  coords_to_widget (crosshair_x - Marked.X, crosshair_y - Marked.Y,
			    m_mark, -1);

	if (Marked.status != saved_mark.status)
	  {
	    if (Marked.status)
	      {
		XtManageChild (XtParent (m_mark));
		XtManageChild (m_mark);
		n = 0;
		stdarg (XmNleftAttachment, XmATTACH_WIDGET);
		stdarg (XmNleftWidget, XtParent (m_mark));
		XtSetValues (XtParent (m_crosshair), args, n);
	      }
	    else
	      {
		n = 0;
		stdarg (XmNleftAttachment, XmATTACH_FORM);
		XtSetValues (XtParent (m_crosshair), args, n);
		XtUnmanageChild (XtParent (m_mark));
	      }
	    last_state = this_state + 100;
	  }
	memcpy (&saved_mark, &Marked, sizeof (MarkType));

	/* This is obtuse.  We want to enable XmRESIZE_ANY long enough
	   to shrink to fit the new format (if any), then switch it
	   back to XmRESIZE_GROW to prevent it from shrinking due to
	   changes in the number of actual digits printed.  Thus, when
	   you switch from a small grid and %.2f formats to a large
	   grid and %d formats, you aren't punished with a wide and
	   mostly white-space label widget.  "this_state" indicates
	   which of the above formats we're using.  "last_state" is
	   either zero (when resizing) or the same as "this_state"
	   (when grow-only), or a non-zero but not "this_state" which
	   means we need to start a resize cycle.  */
	if (this_state != last_state && last_state)
	  {
	    n = 0;
	    stdarg (XmNresizePolicy, XmRESIZE_ANY);
	    XtSetValues (XtParent (m_mark), args, n);
	    XtSetValues (XtParent (m_crosshair), args, n);
	    last_state = 0;
	  }
	else if (this_state != last_state)
	  {
	    n = 0;
	    stdarg (XmNresizePolicy, XmRESIZE_GROW);
	    XtSetValues (XtParent (m_mark), args, n);
	    XtSetValues (XtParent (m_crosshair), args, n);
	    last_state = this_state;
	  }
      }
  }

  {
    static double old_grid = -1;
    static int old_gx, old_gy, old_mm;
	 XmString ms;
    if (PCB->Grid != old_grid
	|| PCB->GridOffsetX != old_gx
	|| PCB->GridOffsetY != old_gy || Settings.grid_units_mm != old_mm)
      {
	static char buf[100];
	double g, x, y;
	char *u;
	old_grid = PCB->Grid;
	old_gx = PCB->GridOffsetX;
	old_gy = PCB->GridOffsetY;
	old_mm = Settings.grid_units_mm;
	if (old_grid == 1)
	  {
	    strcpy (buf, "No Grid");
	  }
	else
	  {
	    if (Settings.grid_units_mm)
	      {
		g = PCB_TO_MM (old_grid);
		x = PCB_TO_MM (old_gx);
		y = PCB_TO_MM (old_gy);
		u = "mm";
	      }
	    else
	      {
		g = PCB_TO_MIL (old_grid);
		x = PCB_TO_MIL (old_gx);
		y = PCB_TO_MIL (old_gy);
		u = "mil";
	      }
	    if (x || y)
	      sprintf (buf, "%g %s @%g,%g", g, u, x, y);
	    else
	      sprintf (buf, "%g %s", g, u);
	  }
	ms = XmStringCreateLocalized (buf);
	n = 0;
	stdarg (XmNlabelString, ms);
	XtSetValues (m_grid, args, n);
      }
  }

  {
    static double old_zoom = -1;
    static int old_zoom_units = -1;
    if (view_zoom != old_zoom || Settings.grid_units_mm != old_zoom_units)
      {
	static char buf[100];
	double g;
	const char *units;
	XmString ms;

	old_zoom = view_zoom;
	old_zoom_units = Settings.grid_units_mm;

	if (Settings.grid_units_mm)
	  {
	    g = PCB_TO_MM (view_zoom);
	    units = "mm";
	  }
	else
	  {
	    g = PCB_TO_MIL (view_zoom);
	    units = "mil";
	  }
	if ((int) (g * 100 + 0.5) == (int) (g + 0.005) * 100)
	  sprintf (buf, "%d %s/pix", (int) (g + 0.005), units);
	else
	  sprintf (buf, "%.2f %s/pix", g, units);
	ms = XmStringCreateLocalized (buf);
	n = 0;
	stdarg (XmNlabelString, ms);
	XtSetValues (m_zoom, args, n);
      }
  }

  {
    static int old_mode = -1;
    if (old_mode != Settings.Mode)
      {
	char *s = "None";
	XmString ms;
	int cursor = -1;
	static int free_cursor = 0;

	old_mode = Settings.Mode;
	switch (Settings.Mode)
	  {
	  case NO_MODE:
	    s = "None";
	    cursor = XC_X_cursor;
	    break;
	  case VIA_MODE:
	    s = "Via";
	    cursor = -1;
	    break;
	  case LINE_MODE:
	    s = "Line";
	    cursor = XC_pencil;
	    break;
	  case RECTANGLE_MODE:
	    s = "Rectangle";
	    cursor = XC_ul_angle;
	    break;
	  case POLYGON_MODE:
	    s = "Polygon";
	    cursor = XC_sb_up_arrow;
	    break;
	  case PASTEBUFFER_MODE:
	    s = "Paste";
	    cursor = XC_hand1;
	    break;
	  case TEXT_MODE:
	    s = "Text";
	    cursor = XC_xterm;
	    break;
	  case ROTATE_MODE:
	    s = "Rotate";
	    cursor = XC_exchange;
	    break;
	  case REMOVE_MODE:
	    s = "Remove";
	    cursor = XC_pirate;
	    break;
	  case MOVE_MODE:
	    s = "Move";
	    cursor = XC_crosshair;
	    break;
	  case COPY_MODE:
	    s = "Copy";
	    cursor = XC_crosshair;
	    break;
	  case INSERTPOINT_MODE:
	    s = "Insert";
	    cursor = XC_dotbox;
	    break;
	  case RUBBERBANDMOVE_MODE:
	    s = "RBMove";
	    cursor = XC_top_left_corner;
	    break;
	  case THERMAL_MODE:
	    s = "Thermal";
	    cursor = XC_iron_cross;
	    break;
	  case ARC_MODE:
	    s = "Arc";
	    cursor = XC_question_arrow;
	    break;
	  case ARROW_MODE:
	    s = "Arrow";
	    cursor = XC_left_ptr;
	    break;
	  case LOCK_MODE:
	    s = "Lock";
	    cursor = XC_hand2;
	    break;
	  }
	ms = XmStringCreateLocalized (s);
	n = 0;
	stdarg (XmNlabelString, ms);
	XtSetValues (m_mode, args, n);

	if (free_cursor)
	  {
	    XFreeCursor (display, my_cursor);
	    free_cursor = 0;
	  }
	if (cursor == -1)
	  {
	    static Pixmap nocur_source = 0;
	    static Pixmap nocur_mask = 0;
	    static Cursor nocursor = 0;
	    if (nocur_source == 0)
	      {
		XColor fg, bg;
		nocur_source =
		  XCreateBitmapFromData (display, window, "\0", 1, 1);
		nocur_mask =
		  XCreateBitmapFromData (display, window, "\0", 1, 1);

		fg.red = fg.green = fg.blue = 65535;
		bg.red = bg.green = bg.blue = 0;
		fg.flags = bg.flags = DoRed | DoGreen | DoBlue;
		nocursor = XCreatePixmapCursor (display, nocur_source,
						nocur_mask, &fg, &bg, 0, 0);
	      }
	    my_cursor = nocursor;
	  }
	else
	  {
	    my_cursor = XCreateFontCursor (display, cursor);
	    free_cursor = 1;
	  }
	XDefineCursor (display, window, my_cursor);
	lesstif_update_status_line ();
      }
  }
  {
    static char *old_clip = 0;
    static int old_tscale = -1;
    char *new_clip = cur_clip ();

    if (new_clip != old_clip || Settings.TextScale != old_tscale)
      {
	lesstif_update_status_line ();
	old_clip = new_clip;
	old_tscale = Settings.TextScale;
      }
  }

  lesstif_update_widget_flags ();

  show_crosshair (1);
  idle_proc_set = 0;
  return True;
}

void
lesstif_need_idle_proc ()
{
  if (idle_proc_set || window == 0)
    return;
  XtAppAddWorkProc (app_context, idle_proc, 0);
  idle_proc_set = 1;
}

static void
lesstif_invalidate_wh (int x, int y, int width, int height, int last)
{
  if (!last || !window)
    return;

  need_redraw = 1;
  need_idle_proc ();
}

static void
lesstif_invalidate_lr (int l, int r, int t, int b, int last)
{
  lesstif_invalidate_wh (l, t, r - l + 1, b - t + 1, last);
}

void
lesstif_invalidate_all (void)
{
  lesstif_invalidate_wh (0, 0, PCB->MaxWidth, PCB->MaxHeight, 1);
}

static int
lesstif_set_layer (const char *name, int group)
{
  int idx = group;
  if (idx >= 0 && idx < max_layer)
    {
      idx = PCB->LayerGroups.Entries[idx][0];
#if 0
      if (idx == LayerStack[0]
	  || GetLayerGroupNumberByNumber (idx) ==
	  GetLayerGroupNumberByNumber (LayerStack[0]))
	autofade = 0;
      else
	autofade = 1;
#endif
    }
#if 0
  else
    autofade = 0;
#endif
  if (idx >= 0 && idx < max_layer + 2)
    return pinout ? 1 : PCB->Data->Layer[idx].On;
  if (idx < 0)
    {
      switch (SL_TYPE (idx))
	{
	case SL_INVISIBLE:
	  return pinout ? 0 : PCB->InvisibleObjectsOn;
	case SL_MASK:
	  if (SL_MYSIDE (idx) && !pinout)
	    return TEST_FLAG (SHOWMASKFLAG, PCB);
	  return 0;
	case SL_SILK:
	  if (SL_MYSIDE (idx) || pinout)
	    return PCB->ElementOn;
	  return 0;
	case SL_ASSY:
	  return 0;
	case SL_UDRILL:
	case SL_PDRILL:
	  return 1;
	}
    }
  return 0;
}

static hidGC
lesstif_make_gc (void)
{
  hidGC rv = malloc (sizeof (hid_gc_struct));
  memset (rv, 0, sizeof (hid_gc_struct));
  rv->me_pointer = &lesstif_gui;
  return rv;
}

static void
lesstif_destroy_gc (hidGC gc)
{
  free (gc);
}

static void
lesstif_use_mask (int use_it)
{
  if (thindraw || thindrawpoly)
    use_it = 0;
  if ((use_it == 0) == (use_mask == 0))
    return;
  use_mask = use_it;
  if (pinout)
    return;
  if (!window)
    return;
  /*  printf("use_mask(%d)\n", use_it); */
  if (!mask_pixmap)
    {
      mask_pixmap =
	XCreatePixmap (display, window, pixmap_w, pixmap_h,
		       XDefaultDepth (display, screen));
      mask_bitmap = XCreatePixmap (display, window, pixmap_w, pixmap_h, 1);
    }
  if (use_it)
    {
      pixmap = mask_pixmap;
      XSetForeground (display, my_gc, 0);
      XSetFunction (display, my_gc, GXcopy);
      XFillRectangle (display, mask_pixmap, my_gc,
		      0, 0, view_width, view_height);
      XFillRectangle (display, mask_bitmap, bclear_gc,
		      0, 0, view_width, view_height);
    }
  else
    {
      XSetClipMask (display, clip_gc, mask_bitmap);
      pixmap = main_pixmap;
      XCopyArea (display, mask_pixmap, main_pixmap, clip_gc,
		 0, 0, view_width, view_height, 0, 0);
    }
}

static void
lesstif_set_color (hidGC gc, const char *name)
{
  static void *cache = 0;
  hidval cval;
  static XColor color, exact_color;

  if (!display)
    return;
  if (!name)
    name = "red";
  gc->colorname = name;
  if (strcmp (name, "erase") == 0)
    {
      gc->color = bgcolor;
      gc->erase = 1;
    }
  else if (strcmp (name, "drill") == 0)
    {
      gc->color = offlimit_color;
      gc->erase = 0;
    }
  else if (hid_cache_color (0, name, &cval, &cache))
    {
      gc->color = cval.lval;
      gc->erase = 0;
    }
  else
    {
      if (!XAllocNamedColor (display, colormap, name, &color, &exact_color))
	color.pixel = WhitePixel (display, screen);
#if 0
      printf ("lesstif_set_color `%s' %08x rgb/%d/%d/%d\n",
	      name, color.pixel, color.red, color.green, color.blue);
#endif
      cval.lval = gc->color = color.pixel;
      hid_cache_color (1, name, &cval, &cache);
      gc->erase = 0;
    }
  if (autofade)
    {
      static int lastcolor = -1, lastfade = -1;
      if (gc->color == lastcolor)
	gc->color = lastfade;
      else
	{
	  lastcolor = gc->color;
	  color.pixel = gc->color;

	  XQueryColor (display, colormap, &color);
	  color.red = (bgred + color.red) / 2;
	  color.green = (bggreen + color.green) / 2;
	  color.blue = (bgblue + color.blue) / 2;
	  XAllocColor (display, colormap, &color);
	  lastfade = gc->color = color.pixel;
	}
    }
}

static void
set_gc (hidGC gc)
{
  int cap, join, width;
  if (gc->me_pointer != &lesstif_gui)
    {
      fprintf (stderr, "Fatal: GC from another HID passed to lesstif HID\n");
      abort ();
    }
#if 0
  printf ("set_gc c%s %08lx w%d c%d x%d e%d\n",
	  gc->colorname, gc->color, gc->width, gc->cap, gc->xor, gc->erase);
#endif
  switch (gc->cap)
    {
    case Square_Cap:
      cap = CapProjecting;
      join = JoinMiter;
      break;
    case Trace_Cap:
    case Round_Cap:
      cap = CapRound;
      join = JoinRound;
      break;
    case Beveled_Cap:
      cap = CapProjecting;
      join = JoinBevel;
      break;
    default:
      cap = CapProjecting;
      join = JoinBevel;
      break;
    }
  if (gc->xor)
    {
      XSetFunction (display, my_gc, GXxor);
      XSetForeground (display, my_gc, gc->color ^ bgcolor);
    }
  else if (gc->erase)
    {
      XSetFunction (display, my_gc, GXcopy);
      XSetForeground (display, my_gc, offlimit_color);
    }
  else
    {
      XSetFunction (display, my_gc, GXcopy);
      XSetForeground (display, my_gc, gc->color);
    }
  width = Vz (gc->width);
  if (width < 0)
    width = 0;
  XSetLineAttributes (display, my_gc, thindraw ? 1 : width, LineSolid, cap,
		      join);
  if (use_mask)
    {
      if (gc->erase)
	mask_gc = bclear_gc;
      else
	mask_gc = bset_gc;
      XSetLineAttributes (display, mask_gc, Vz (gc->width), LineSolid, cap,
			  join);
    }
}

static void
lesstif_set_line_cap (hidGC gc, EndCapStyle style)
{
  gc->cap = style;
}

static void
lesstif_set_line_width (hidGC gc, int width)
{
  gc->width = width;
}

static void
lesstif_set_draw_xor (hidGC gc, int xor)
{
  gc->xor = xor;
}

static void
lesstif_set_draw_faded (hidGC gc, int faded)
{
  /* We don't use this */
}

static void
lesstif_set_line_cap_angle (hidGC gc, int x1, int y1, int x2, int y2)
{
  CRASH;
}

#define ISORT(a,b) if (a>b) { a^=b; b^=a; a^=b; }

static void
lesstif_draw_line (hidGC gc, int x1, int y1, int x2, int y2)
{
  int vw = Vz (gc->width);
  if ((pinout || thindraw || thindrawpoly) && gc->erase)
    return;
#if 0
  printf ("draw_line %d,%d %d,%d @%d", x1, y1, x2, y2, gc->width);
#endif
  x1 = Vx (x1);
  y1 = Vy (y1);
  x2 = Vx (x2);
  y2 = Vy (y2);
#if 0
  printf (" = %d,%d %d,%d %s\n", x1, y1, x2, y2, gc->colorname);
#endif
  if (x1 < -vw && x2 < -vw)
    return;
  if (y1 < -vw && y2 < -vw)
    return;
  if (x1 > view_width + vw && x2 > view_width + vw)
    return;
  if (y1 > view_height + vw && y2 > view_height + vw)
    return;
  set_gc (gc);
  if (thindraw)
    {
      switch (gc->cap)
	{
	case Trace_Cap:
	default:
	  XDrawLine (display, pixmap, my_gc, x1, y1, x2, y2);
	  break;
	case Square_Cap:
	  ISORT (x1, x2);
	  ISORT (y1, y2);
	  x1 -= vw / 2;
	  y1 -= vw / 2;
	  x2 += vw / 2;
	  y2 += vw / 2;
	  XDrawRectangle (display, pixmap, my_gc, x1, y1, x2 - x1, y2 - y1);
	  return;
	case Round_Cap:
	  {
	    double dx, dy, a, l;
	    vw &= ~1;
	    dx = x2 - x1;
	    dy = y2 - y1;
	    l = sqrt (dx * dx + dy * dy);
	    dx *= vw / 2 / l;
	    dy *= vw / 2 / l;
	    a = atan2 ((float) dx, (float) dy) * 57.295779;
	    a = a * 64;
	    XDrawLine (display, pixmap, my_gc, x1 + dy, y1 - dx, x2 + dy,
		       y2 - dx);
	    XDrawLine (display, pixmap, my_gc, x1 - dy, y1 + dx, x2 - dy,
		       y2 + dx);
	    XDrawArc (display, pixmap, my_gc, x1 - vw / 2, y1 - vw / 2, vw,
		      vw, a, 180 * 64);
	    XDrawArc (display, pixmap, my_gc, x2 - vw / 2, y2 - vw / 2, vw,
		      vw, a + 180 * 64, 180 * 64);
	  }
	}
    }
  else if (gc->cap == Square_Cap && x1 == x2 && y1 == y2)
    {
      XFillRectangle (display, pixmap, my_gc, x1 - vw / 2, y1 - vw / 2, vw,
		      vw);
      if (use_mask)
	XFillRectangle (display, mask_bitmap, mask_gc, x1 - vw / 2,
			y1 - vw / 2, vw, vw);
    }
  else
    {
      XDrawLine (display, pixmap, my_gc, x1, y1, x2, y2);
      if (use_mask)
	XDrawLine (display, mask_bitmap, mask_gc, x1, y1, x2, y2);
    }
}

static void
lesstif_draw_arc (hidGC gc, int cx, int cy, int width, int height,
		  int start_angle, int delta_angle)
{
  if ((pinout || thindraw) && gc->erase)
    return;
#if 0
  printf ("draw_arc %d,%d %dx%d", cx, cy, width, height);
#endif
  width = Vz (width);
  height = Vz (height);
  cx = Vx (cx) - width;
  cy = Vy (cy) - height;
  if (flip_x)
    {
      start_angle = 180 - start_angle;
      delta_angle = - delta_angle;
    }
  if (flip_y)
    {
      start_angle = - start_angle;
      delta_angle = - delta_angle;					
    }
#if 0
  printf (" = %d,%d %dx%d %d %s\n", cx, cy, width, height, gc->width,
	  gc->colorname);
#endif
  set_gc (gc);
  XDrawArc (display, pixmap, my_gc, cx, cy,
	    width * 2, height * 2, (start_angle + 180) * 64,
	    delta_angle * 64);
  if (use_mask && !thindraw)
    XDrawArc (display, mask_bitmap, mask_gc, cx, cy,
	      width * 2, height * 2, (start_angle + 180) * 64,
	      delta_angle * 64);
}

static void
lesstif_draw_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  int vw = Vz (gc->width);
  if ((pinout || thindraw) && gc->erase)
    return;
  x1 = Vx (x1);
  y1 = Vy (y1);
  x2 = Vx (x2);
  y2 = Vy (y2);
  if (x1 < -vw && x2 < -vw)
    return;
  if (y1 < -vw && y2 < -vw)
    return;
  if (x1 > view_width + vw && x2 > view_width + vw)
    return;
  if (y1 > view_height + vw && y2 > view_height + vw)
    return;
  set_gc (gc);
  XDrawRectangle (display, pixmap, my_gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  if (use_mask)
    XDrawRectangle (display, mask_bitmap, mask_gc, x1, y1, x2 - x1 + 1,
		    y2 - y1 + 1);
}

static void
lesstif_fill_circle (hidGC gc, int cx, int cy, int radius)
{
  if (pinout && use_mask && gc->erase)
    return;
  if ((thindraw || thindrawpoly) && gc->erase)
    return;
#if 0
  printf ("fill_circle %d,%d %d", cx, cy, radius);
#endif
  radius = Vz (radius);
  cx = Vx (cx) - radius;
  cy = Vy (cy) - radius;
  if (cx < -2 * radius || cx > view_width)
    return;
  if (cy < -2 * radius || cy > view_height)
    return;
#if 0
  printf (" = %d,%d %d %lx %s\n", cx, cy, radius, gc->color, gc->colorname);
#endif
  set_gc (gc);
  if (thindraw)
    {
      XDrawArc (display, pixmap, my_gc, cx, cy,
		radius * 2, radius * 2, 0, 360 * 64);
    }
  else
    {
      XFillArc (display, pixmap, my_gc, cx, cy,
		radius * 2, radius * 2, 0, 360 * 64);
      if (use_mask)
	XFillArc (display, mask_bitmap, mask_gc, cx, cy,
		  radius * 2, radius * 2, 0, 360 * 64);
    }
}

static void
lesstif_fill_polygon (hidGC gc, int n_coords, int *x, int *y)
{
  static XPoint *p = 0;
  static int maxp = 0;
  int i;

  if (maxp < n_coords)
    {
      maxp = n_coords + 10;
      if (p)
	p = (XPoint *) realloc (p, maxp * sizeof (XPoint));
      else
	p = (XPoint *) malloc (maxp * sizeof (XPoint));
    }

  for (i = 0; i < n_coords; i++)
    {
      p[i].x = Vx (x[i]);
      p[i].y = Vy (y[i]);
    }
#if 0
  printf ("fill_polygon %d pts\n", n_coords);
#endif
  if (thindraw || thindrawpoly)
    {
      int save_thindraw = thindraw;
      thindraw = 1;
      set_gc (gc);
      XDrawLines (display, pixmap, my_gc, p, n_coords, CoordModeOrigin);
      if (x[0] != x[n_coords - 1] || y[0] != y[n_coords - 1])
	XDrawLine (display, pixmap, my_gc, p[n_coords - 1].x,
		   p[n_coords - 1].y, p[0].x, p[0].y);
      thindraw = save_thindraw;
    }
  else
    {
      set_gc (gc);
      XFillPolygon (display, pixmap, my_gc, p, n_coords, Complex,
		    CoordModeOrigin);
      if (use_mask)
	XFillPolygon (display, mask_bitmap, mask_gc, p, n_coords, Complex,
		      CoordModeOrigin);
    }
}

static void
lesstif_fill_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  int vw = Vz (gc->width);
  if ((pinout || thindraw) && gc->erase)
    return;
  x1 = Vx (x1);
  y1 = Vy (y1);
  x2 = Vx (x2);
  y2 = Vy (y2);
  if (x1 < -vw && x2 < -vw)
    return;
  if (y1 < -vw && y2 < -vw)
    return;
  if (x1 > view_width + vw && x2 > view_width + vw)
    return;
  if (y1 > view_height + vw && y2 > view_height + vw)
    return;
  set_gc (gc);
  if (thindraw)
    {
      XDrawRectangle (display, pixmap, my_gc, x1, y1, x2 - x1 + 1,
		      y2 - y1 + 1);
      if (use_mask)
	XDrawRectangle (display, mask_bitmap, mask_gc, x1, y1, x2 - x1 + 1,
			y2 - y1 + 1);
    }
  else
    {
      XFillRectangle (display, pixmap, my_gc, x1, y1, x2 - x1 + 1,
		      y2 - y1 + 1);
      if (use_mask)
	XFillRectangle (display, mask_bitmap, mask_gc, x1, y1, x2 - x1 + 1,
			y2 - y1 + 1);
    }
}

static void
lesstif_calibrate (double xval, double yval)
{
  CRASH;
}

static int
lesstif_shift_is_pressed (void)
{
  return shift_pressed;
}

static int
lesstif_control_is_pressed (void)
{
  return ctrl_pressed;
}

extern void lesstif_get_coords (const char *msg, int *x, int *y);

static void
lesstif_set_crosshair (int x, int y)
{
  if (crosshair_x != x || crosshair_y != y)
    {
      crosshair_x = x;
      crosshair_y = y;
      need_idle_proc ();

      if (mainwind
	  && !in_move_event
	  && (x < view_left_x
	      || x > view_left_x + view_width * view_zoom
	      || y < view_top_y || y > view_top_y + view_height * view_zoom))
	{
	  view_left_x = x - (view_width * view_zoom) / 2;
	  view_top_y = y - (view_height * view_zoom) / 2;
	  lesstif_pan_fixup ();
	}

    }
}

typedef struct
{
  void (*func) (hidval);
  hidval user_data;
  XtIntervalId id;
} TimerStruct;

static void
lesstif_timer_cb (XtPointer * p, XtIntervalId * id)
{
  TimerStruct *ts = (TimerStruct *) p;
  ts->func (ts->user_data);
  free (ts);
}

static hidval
lesstif_add_timer (void (*func) (hidval user_data),
		   unsigned long milliseconds, hidval user_data)
{
  TimerStruct *t;
  hidval rv;
  t = (TimerStruct *) malloc (sizeof (TimerStruct));
  rv.ptr = t;
  t->func = func;
  t->user_data = user_data;
  t->id = XtAppAddTimeOut (app_context, milliseconds, (XtTimerCallbackProc)lesstif_timer_cb, t);
  return rv;
}

static void
lesstif_stop_timer (hidval hv)
{
  TimerStruct *ts = (TimerStruct *) hv.ptr;
  XtRemoveTimeOut (ts->id);
  free (ts);
}

extern void lesstif_log (const char *fmt, ...);

extern void lesstif_logv (const char *fmt, va_list ap);

extern int lesstif_confirm_dialog (char *msg, ...);

extern void lesstif_report_dialog (char *title, char *msg);

extern int
lesstif_attribute_dialog (HID_Attribute * attrs,
			  int n_attrs, HID_Attr_Val * results);

static void
pinout_callback (Widget da, PinoutData * pd,
		 XmDrawingAreaCallbackStruct * cbs)
{
  BoxType region;
  int save_vx, save_vy, save_vw, save_vh;
  int save_fx, save_fy;
  double save_vz;
  Pixmap save_px;
  int reason = cbs ? cbs->reason : 0;

  if (pd->window == 0 && reason == XmCR_RESIZE)
    return;
  if (pd->window == 0 || reason == XmCR_RESIZE)
    {
      Dimension w, h;
      double z;

      n = 0;
      stdarg (XmNwidth, &w);
      stdarg (XmNheight, &h);
      XtGetValues (da, args, n);

      pd->window = XtWindow (da);
      pd->v_width = w;
      pd->v_height = h;
      pd->zoom = (pd->right - pd->left + 1) / (double) w;
      z = (pd->bottom - pd->top + 1) / (double) h;
      if (pd->zoom < z)
	pd->zoom = z;

      pd->x = (pd->left + pd->right) / 2 - pd->v_width * pd->zoom / 2;
      pd->y = (pd->top + pd->bottom) / 2 - pd->v_height * pd->zoom / 2;
    }

  save_vx = view_left_x;
  save_vy = view_top_y;
  save_vz = view_zoom;
  save_vw = view_width;
  save_vh = view_height;
  save_fx = flip_x;
  save_fy = flip_y;
  save_px = pixmap;
  pinout = pd;
  pixmap = pd->window;
  view_left_x = pd->x;
  view_top_y = pd->y;
  view_zoom = pd->zoom;
  view_width = pd->v_width;
  view_height = pd->v_height;
  use_mask = 0;
  flip_x = flip_y = 0;

  region.X1 = 0;
  region.Y1 = 0;
  region.X2 = PCB->MaxWidth;
  region.Y2 = PCB->MaxHeight;

  XFillRectangle (display, pixmap, bg_gc, 0, 0, pd->v_width, pd->v_height);
  hid_expose_callback (&lesstif_gui, &region, pd->item);

  pinout = 0;
  view_left_x = save_vx;
  view_top_y = save_vy;
  view_zoom = save_vz;
  view_width = save_vw;
  view_height = save_vh;
  pixmap = save_px;
  flip_x = save_fx;
  flip_y = save_fy;
}

static void
pinout_unmap (Widget w, PinoutData * pd, void *v)
{
  if (pd->prev)
    pd->prev->next = pd->next;
  else
    pinouts = pd->next;
  if (pd->next)
    pd->next->prev = pd->prev;
  XtDestroyWidget (XtParent (pd->form));
  free (pd);
}

static void
lesstif_show_item (void *item)
{
  double scale;
  Widget da;
  BoxType *extents;
  PinoutData *pd;

  for (pd = pinouts; pd; pd = pd->next)
    if (pd->item == item)
      return;
  if (!mainwind)
    return;

  pd = (PinoutData *) MyCalloc (1, sizeof (PinoutData), "lesstif_show_item");

  pd->item = item;

  extents = hid_get_extents (item);
  pd->left = extents->X1;
  pd->right = extents->X2;
  pd->top = extents->Y1;
  pd->bottom = extents->Y2;

  if (pd->left > pd->right)
    {
      free (&pd);
      return;
    }
  pd->prev = 0;
  pd->next = pinouts;
  if (pd->next)
    pd->next->prev = pd;
  pinouts = pd;
  pd->zoom = 0;

  n = 0;
  pd->form = XmCreateFormDialog (mainwind, "pinout", args, n);
  pd->window = 0;
  XtAddCallback (pd->form, XmNunmapCallback, (XtCallbackProc) pinout_unmap,
		 (XtPointer) pd);

  scale =
    sqrt (200.0 * 200.0 /
	  ((pd->right - pd->left + 1.0) * (pd->bottom - pd->top + 1.0)));

  n = 0;
  stdarg (XmNwidth, (int) (scale * (pd->right - pd->left + 1)));
  stdarg (XmNheight, (int) (scale * (pd->bottom - pd->top + 1)));
  stdarg (XmNleftAttachment, XmATTACH_FORM);
  stdarg (XmNrightAttachment, XmATTACH_FORM);
  stdarg (XmNtopAttachment, XmATTACH_FORM);
  stdarg (XmNbottomAttachment, XmATTACH_FORM);
  da = XmCreateDrawingArea (pd->form, "pinout", args, n);
  XtManageChild (da);

  XtAddCallback (da, XmNexposeCallback, (XtCallbackProc) pinout_callback,
		 (XtPointer) pd);
  XtAddCallback (da, XmNresizeCallback, (XtCallbackProc) pinout_callback,
		 (XtPointer) pd);

  XtManageChild (pd->form);
  pinout = 0;
}

static void
lesstif_beep (void)
{
  putchar (7);
  fflush (stdout);
}

HID lesstif_gui = {
  "lesstif",
  "LessTif - a Motif clone for X/Unix",
  1,				/* gui */
  0,				/* printer */
  0,				/* exporter */
  1,				/* poly before */
  0,				/* poly after */

  lesstif_get_export_options,
  lesstif_do_export,
  lesstif_parse_arguments,

  lesstif_invalidate_wh,
  lesstif_invalidate_lr,
  lesstif_invalidate_all,
  lesstif_set_layer,
  lesstif_make_gc,
  lesstif_destroy_gc,
  lesstif_use_mask,
  lesstif_set_color,
  lesstif_set_line_cap,
  lesstif_set_line_width,
  lesstif_set_draw_xor,
  lesstif_set_draw_faded,
  lesstif_set_line_cap_angle,
  lesstif_draw_line,
  lesstif_draw_arc,
  lesstif_draw_rect,
  lesstif_fill_circle,
  lesstif_fill_polygon,
  lesstif_fill_rect,

  lesstif_calibrate,
  lesstif_shift_is_pressed,
  lesstif_control_is_pressed,
  lesstif_get_coords,
  lesstif_set_crosshair,
  lesstif_add_timer,
  lesstif_stop_timer,

  lesstif_log,
  lesstif_logv,
  lesstif_confirm_dialog,
  lesstif_report_dialog,
  lesstif_prompt_for,
  lesstif_attribute_dialog,
  lesstif_show_item,
  lesstif_beep
};

#include "dolists.h"

void
hid_lesstif_init ()
{
  hid_register_hid (&lesstif_gui);
#include "lesstif_lists.h"
}
