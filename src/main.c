/*!
 * \file src/main.c
 *
 * \brief Main program, initializes some stuff and handles user input.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996, 2004 Thomas Nau
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
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h> /* Seed for srand() */

#include "global.h"
#include "data.h"
#include "buffer.h"
#include "create.h"
#include "crosshair.h"
#include "draw.h"
#include "error.h"
#include "file.h"
#include "set.h"
#include "action.h"
#include "misc.h"
#include "lrealpath.h"
#include "free_atexit.h"
#include "polygon.h"
#include "gettext.h"
#include "pcb-printf.h"
#include "strflags.h"

#include "hid/common/actions.h"

/* This next one is so we can print the help messages. */
#include "hid/hidint.h"

#ifdef HAVE_DBUS
#include "dbus.h"
#endif

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define PCBLIBPATH ".:" PCBLIBDIR


#ifdef HAVE_LIBSTROKE
extern void stroke_init (void);
#endif


/*!
 * \brief Initialize signal and error handlers.
 */
static void
InitHandler (void)
{
/*
	signal(SIGHUP, CatchSignal);
	signal(SIGQUIT, CatchSignal);
	signal(SIGABRT, CatchSignal);
	signal(SIGSEGV, CatchSignal);
	signal(SIGTERM, CatchSignal);
	signal(SIGINT, CatchSignal);
*/

  /* calling external program by popen() may cause a PIPE signal,
   * so we ignore it
   */
#ifdef SIGPIPE
  signal (SIGPIPE, SIG_IGN);
#endif
}


  /* ----------------------------------------------------------------------
     |  command line and rc file processing.
   */
static char *command_line_pcb;

/*!
 * \brief Print the copyright notice.
 */
void
copyright (void)
{
  printf ("\n"
	  "                COPYRIGHT for %s version %s\n\n"
	  "    PCB, interactive printed circuit board design\n"
	  "    Copyright (C) 1994,1995,1996,1997 Thomas Nau\n"
	  "    Copyright (C) 1998, 1999, 2000 Harry Eaton\n\n"
	  "    This program is free software; you can redistribute it and/or modify\n"
	  "    it under the terms of the GNU General Public License as published by\n"
	  "    the Free Software Foundation; either version 2 of the License, or\n"
	  "    (at your option) any later version.\n\n"
	  "    This program is distributed in the hope that it will be useful,\n"
	  "    but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	  "    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	  "    GNU General Public License for more details.\n\n"
	  "    You should have received a copy of the GNU General Public License along\n"
	  "    with this program; if not, write to the Free Software Foundation, Inc.,\n"
	  "    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.\n\n",
	  Progname, VERSION);
  exit (0);
}

static inline void
u (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  fputc ('\n', stderr);
  va_end (ap);
}

typedef struct UsageNotes {
  struct UsageNotes *next;
  HID_Attribute *seen;
} UsageNotes;

static UsageNotes *usage_notes = NULL;

static void
usage_attr (HID_Attribute * a)
{
  int i, n;
  const Unit *unit_list;
  static char buf[200];

  if (a->help_text == ATTR_UNDOCUMENTED)
    return;

  switch (a->type)
    {
    case HID_Label:
      return;
    case HID_Integer:
    case HID_Real:
      sprintf (buf, "--%s <num>", a->name);
      break;
    case HID_Coord:
      sprintf (buf, "--%s <measure>", a->name);
      break;
    case HID_String:
      sprintf (buf, "--%s <string>", a->name);
      break;
    case HID_Boolean:
      sprintf (buf, "--%s", a->name);
      break;
    case HID_Mixed:
    case HID_Enum:
      sprintf (buf, "--%s ", a->name);
      if (a->type == HID_Mixed)
	strcat (buf, " <val>");
      for (i = 0; a->enumerations[i]; i++)
	{
	  strcat (buf, i ? "|" : "<");
	  strcat (buf, a->enumerations[i]);
	}
      strcat (buf, ">");
      break;
    case HID_Path:
      sprintf (buf, "--%s <path>", a->name);
      break;
    case HID_Unit:
      unit_list = get_unit_list ();
      n = get_n_units ();
      sprintf (buf, "--%s ", a->name);
      for (i = 0; i < n; i++)
	{
	  strcat (buf, i ? "|" : "<");
	  strcat (buf, unit_list[i].suffix);
	}
      strcat (buf, ">");
      break;
    }

  if (strlen (buf) <= 30)
    {
      if (a->help_text)
	fprintf (stderr, " %-30s\t%s\n", buf, a->help_text);
      else
	fprintf (stderr, " %-30s\n", buf);
    }
  else if (a->help_text && strlen (a->help_text) + strlen (buf) < 72)
    fprintf (stderr, " %s\t%s\n", buf, a->help_text);
  else if (a->help_text)
    fprintf (stderr, " %s\n\t\t\t%s\n", buf, a->help_text);
  else
    fprintf (stderr, " %s\n", buf);
}

static void
usage_hid (HID * h)
{
  HID_Attribute *attributes;
  int i, n_attributes = 0;
  UsageNotes *note;

  if (h->gui)
    {
      fprintf (stderr, "\n%s gui options:\n", h->name);
      attributes = h->get_export_options (&n_attributes);
    }
  else
    {
      fprintf (stderr, "\n%s options:\n", h->name);
      exporter = h;
      attributes = exporter->get_export_options (&n_attributes);
      exporter = NULL;
    }

  note = (UsageNotes *)malloc (sizeof (UsageNotes));
  note->next = usage_notes;
  note->seen = attributes;
  usage_notes = note;
  
  for (i = 0; i < n_attributes; i++)
    usage_attr (attributes + i);
}

static void
usage (void)
{
  HID **hl = hid_enumerate ();
  HID_AttrNode *ha;
  UsageNotes *note;
  int i;
  int n_printer = 0, n_gui = 0, n_exporter = 0;

  for (i = 0; hl[i]; i++)
    {
      if (hl[i]->gui)
	n_gui++;
      if (hl[i]->printer)
	n_printer++;
      if (hl[i]->exporter)
	n_exporter++;
    }

  u ("PCB Printed Circuit Board editing program, http://pcb.geda-project.org");
  u ("%s [-h|-V|--copyright]\t\t\tHelp, version, copyright", Progname);
  u ("%s [gui options] <pcb file>\t\tto edit", Progname);
  u ("Available GUI hid%s:", n_gui == 1 ? "" : "s");
  for (i = 0; hl[i]; i++)
    if (hl[i]->gui)
      fprintf (stderr, "\t%-8s %s\n", hl[i]->name, hl[i]->description);
  u ("%s -p [printing options] <pcb file>\tto print", Progname);
  u ("Available printing hid%s:", n_printer == 1 ? "" : "s");
  for (i = 0; hl[i]; i++)
    if (hl[i]->printer)
      fprintf (stderr, "\t%-8s %s\n", hl[i]->name, hl[i]->description);
  u ("%s -x hid [export options] <pcb file>\tto export", Progname);
  u ("Available export hid%s:", n_exporter == 1 ? "" : "s");
  for (i = 0; hl[i]; i++)
    if (hl[i]->exporter)
      fprintf (stderr, "\t%-8s %s\n", hl[i]->name, hl[i]->description);

  for (i = 0; hl[i]; i++)
    if (hl[i]->gui)
      usage_hid (hl[i]);
  for (i = 0; hl[i]; i++)
    if (hl[i]->printer)
      usage_hid (hl[i]);
  for (i = 0; hl[i]; i++)
    if (hl[i]->exporter)
      usage_hid (hl[i]);

  u ("\nCommon options:");
  for (ha = hid_attr_nodes; ha; ha = ha->next)
    {
      for (note = usage_notes; note && note->seen != ha->attributes; note = note->next)
	;
      if (note)
	continue;
      for (i = 0; i < ha->n; i++)
	{
	  usage_attr (ha->attributes + i);
	}
    }  

  exit (1);
}

static void
print_defaults_1 (HID_Attribute * a, void *value)
{
  int i;
  Coord c;
  double d;
  const char *s;

  /* Remember, at this point we've parsed the command line, so they
     may be in the global variable instead of the default_val.  */
  switch (a->type)
    {
    case HID_Integer:
      i = value ? *(int *) value : a->default_val.int_value;
      fprintf (stderr, "%s %d\n", a->name, i);
      break;
    case HID_Boolean:
      i = value ? *(char *) value : a->default_val.int_value;
      fprintf (stderr, "%s %s\n", a->name, i ? "yes" : "no");
      break;
    case HID_Real:
      d = value ? *(double *) value : a->default_val.real_value;
      fprintf (stderr, "%s %g\n", a->name, d);
      break;
    case HID_Coord:
      c = value ? *(Coord *) value : a->default_val.coord_value;
      pcb_fprintf (stderr, "%s %$mS\n", a->name, c);
      break;
    case HID_String:
    case HID_Path:
      s = value ? *(char **) value : a->default_val.str_value;
      fprintf (stderr, "%s \"%s\"\n", a->name, s);
      break;
    case HID_Enum:
      i = value ? *(int *) value : a->default_val.int_value;
      fprintf (stderr, "%s %s\n", a->name, a->enumerations[i]);
      break;
    case HID_Mixed:
      i = value ?
        ((HID_Attr_Val*)value)->int_value  : a->default_val.int_value;
      d = value ?
        ((HID_Attr_Val*)value)->real_value : a->default_val.real_value;
      fprintf (stderr, "%s %g%s\n", a->name, d, a->enumerations[i]);
      break;
    case HID_Label:
      break;
    case HID_Unit:
      i = value ? *(int *) value : a->default_val.int_value;
      fprintf (stderr, "%s %s\n", a->name, get_unit_list()[i].suffix);
    }
}

static void
print_defaults ()
{
  HID **hl = hid_enumerate ();
  HID_Attribute *e;
  int i, n, hi;

  for (hi = 0; hl[hi]; hi++)
    {
      HID *h = hl[hi];
      if (h->gui)
	{
	  HID_AttrNode *ha;
	  fprintf (stderr, "\ngui defaults:\n");
	  for (ha = hid_attr_nodes; ha; ha = ha->next)
	    for (i = 0; i < ha->n; i++)
	      print_defaults_1 (ha->attributes + i, ha->attributes[i].value);
	}
      else
	{
	  fprintf (stderr, "\n%s defaults:\n", h->name);
	  exporter = h;
	  e = exporter->get_export_options (&n);
	  exporter = NULL;
	  if (e)
	    for (i = 0; i < n; i++)
	      print_defaults_1 (e + i, 0);
	}
    }
  exit (1);
}

#define SSET(F,D,N,H) { N, H, \
	HID_String,  0, 0, { 0, D, 0 }, 0, &Settings.F }
#define ISET(F,D,N,H) { N, H, \
	HID_Integer, 0, 0, { D, 0, 0 }, 0, &Settings.F }
#define BSET(F,D,N,H) { N, H, \
	HID_Boolean, 0, 0, { D, 0, 0 }, 0, &Settings.F }
#define RSET(F,D,N,H) { N, H, \
	HID_Real,    0, 0, { 0, 0, D }, 0, &Settings.F }
#define CSET(F,D,N,H) { N, H, \
	HID_Coord,    0, 0, { 0, 0, 0, D }, 0, &Settings.F }

#define COLOR(F,D,N,H) { N, H, \
	HID_String, 0, 0, { 0, D, 0 }, 0, &Settings.F }
#define LAYERCOLOR(N,D) { "layer-color-" #N, "Color for layer " #N, \
	HID_String, 0, 0, { 0, D, 0 }, 0, &Settings.LayerColor[N-1] }
#define LAYERNAME(N,D) { "layer-name-" #N, "Name for layer " #N, \
	HID_String, 0, 0, { 0, D, 0 }, 0, &Settings.DefaultLayerName[N-1] }
#define LAYERSELCOLOR(N) { "layer-selected-color-" #N, "Color for layer " #N " when selected", \
	HID_String, 0, 0, { 0, "#00ffff", 0 }, 0, &Settings.LayerSelectedColor[N-1] }

static int show_help = 0;
static int show_version = 0;
static int show_copyright = 0;
static int show_defaults = 0;
static int show_actions = 0;
static int do_dump_actions = 0;
static char *grid_units;
static Increments increment_mm  = { 0 };
static Increments increment_mil = { 0 };

void save_increments (const Increments *mm, const Increments *mil)
{
  memcpy (&increment_mm,  mm,  sizeof (*mm));
  memcpy (&increment_mil, mil, sizeof (*mil));
}

HID_Attribute main_attribute_list[] = {

/* %start-doc options "1 General Options"
@ftable @code
@item --help
Show help on command line options.
@end ftable
%end-doc
*/
  {"help", "Show help on command line options", HID_Boolean, 0, 0, {0, 0, 0}, 0,
  &show_help},

/* %start-doc options "1 General Options"
@ftable @code
@item --version
Show version.
@end ftable
%end-doc
*/
  {"version", "Show version", HID_Boolean, 0, 0, {0, 0, 0}, 0, &show_version},

/* %start-doc options "1 General Options"
@ftable @code
@item --verbose
Be verbose on stdout.
@end ftable
%end-doc
*/
  {"verbose", "Be verbose on stdout", HID_Boolean, 0, 0, {0, 0, 0}, 0,
   &Settings.verbose},

/* %start-doc options "1 General Options"
@ftable @code
@item --copyright
Show copyright.
@end ftable
%end-doc
*/
  {"copyright", "Show Copyright", HID_Boolean, 0, 0, {0, 0, 0}, 0,
   &show_copyright},

/* %start-doc options "1 General Options"
@ftable @code
@item --show-defaults
Show option defaults.
@end ftable
%end-doc
*/
  {"show-defaults", "Show option defaults", HID_Boolean, 0, 0, {0, 0, 0}, 0,
   &show_defaults},

/* %start-doc options "1 General Options"
@ftable @code
@item --show-actions
Show available actions and exit.
@end ftable
%end-doc
*/
  {"show-actions", "Show available actions", HID_Boolean, 0, 0, {0, 0, 0}, 0,
   &show_actions},

/* %start-doc options "1 General Options"
@ftable @code
@item --dump-actions
Dump actions (for documentation).
@end ftable
%end-doc
*/
  {"dump-actions", "Dump actions (for documentation)", HID_Boolean, 0, 0,
   {0, 0, 0}, 0, &do_dump_actions},

/* %start-doc options "1 General Options"
@ftable @code
@item --grid-units-mm <string>
Set default grid units. Can be mm or mil. Defaults to mil.
@end ftable
%end-doc
*/
  {"grid-units", "Default grid units (mm|mil)", HID_String, 0, 0, {0, "mil", 0},
  0, &grid_units},

/* %start-doc options "1 General Options"
@ftable @code
@item --clear-increment-mm <string>
Set default clear increment (amount to change when user presses k or K)
when user is using a metric grid unit.
@end ftable
%end-doc
*/
  {"clear-increment-mm", "Default clear increment amount (metric)", HID_Coord, 0, 0, {0, 0, 0},
  0, &increment_mm.clear},

/* %start-doc options "1 General Options"
@ftable @code
@item --grid-increment-mm <string>
Set default grid increment (amount to change when user presses g or G)
when user is using a metric grid unit.
@end ftable
%end-doc
*/
  {"grid-increment-mm", "Default grid increment amount (metric)", HID_Coord, 0, 0, {0, 0, 0},
  0, &increment_mm.grid},

/* %start-doc options "1 General Options"
@ftable @code
@item --line-increment-mm <string>
Set default line increment (amount to change when user presses l or L)
when user is using a metric grid unit.
@end ftable
%end-doc
*/
  {"line-increment-mm", "Default line increment amount (metric)", HID_Coord, 0, 0, {0, 0, 0},
  0, &increment_mm.line},

/* %start-doc options "1 General Options"
@ftable @code
@item --size-increment-mm <string>
Set default size increment (amount to change when user presses s or S)
when user is using a metric grid unit.
@end ftable
%end-doc
*/
  {"size-increment-mm", "Default size increment amount (metric)", HID_Coord, 0, 0, {0, 0, 0},
  0, &increment_mm.size},

/* %start-doc options "1 General Options"
@ftable @code
@item --clear-increment-mil <string>
Set default clear increment (amount to change when user presses k or K)
when user is using an imperial grid unit.
@end ftable
%end-doc
*/
  {"clear-increment-mil", "Default clear increment amount (imperial)", HID_Coord, 0, 0, {0, 0, 0},
  0, &increment_mil.clear},

/* %start-doc options "1 General Options"
@ftable @code
@item --grid-increment-mil <string>
Set default grid increment (amount to change when user presses g or G)
when user is using a imperial grid unit.
@end ftable
%end-doc
*/
  {"grid-increment-mil", "Default grid increment amount (imperial)", HID_Coord, 0, 0, {0, 0, 0},
  0, &increment_mil.grid},

/* %start-doc options "1 General Options"
@ftable @code
@item --line-increment-mil <string>
Set default line increment (amount to change when user presses l or L)
when user is using a imperial grid unit.
@end ftable
%end-doc
*/
  {"line-increment-mil", "Default line increment amount (imperial)", HID_Coord, 0, 0, {0, 0, 0},
  0, &increment_mil.line},

/* %start-doc options "1 General Options"
@ftable @code
@item --size-increment-mil <string>
Set default size increment (amount to change when user presses s or S)
when user is using a imperial grid unit.
@end ftable
%end-doc
*/
  {"size-increment-mil", "Default size increment amount (imperial)", HID_Coord, 0, 0, {0, 0, 0},
  0, &increment_mil.size},

/* %start-doc options "3 Colors"
@ftable @code
@item --black-color <string>
Color value for black. Default: @samp{#000000}
@end ftable
%end-doc
*/
  COLOR (BlackColor, "#000000", "black-color", "color value of 'black'"),

/* %start-doc options "3 Colors"
@ftable @code
@item --black-color <string>
Color value for white. Default: @samp{#ffffff}
@end ftable
%end-doc
*/
  COLOR (WhiteColor, "#ffffff", "white-color", "color value of 'white'"),

/* %start-doc options "3 Colors"
@ftable @code
@item --background-color <string>
Background color of the canvas. Default: @samp{#e5e5e5}
@end ftable
%end-doc
*/
  COLOR (BackgroundColor, "#e5e5e5", "background-color",
	 "color for background"),

/* %start-doc options "3 Colors"
@ftable @code
@item --crosshair-color <string>
Color of the crosshair. Default: @samp{#ff0000}
@end ftable
%end-doc
*/
  COLOR (CrosshairColor, "#ff0000", "crosshair-color",
	 "color for the crosshair"),

/* %start-doc options "3 Colors"
@ftable @code
@item --cross-color <string>
Color of the cross. Default: @samp{#cdcd00}
@end ftable
%end-doc
*/
  COLOR (CrossColor, "#cdcd00", "cross-color", "color of the cross"),

/* %start-doc options "3 Colors"
@ftable @code
@item --via-color <string>
Color of vias. Default: @samp{#7f7f7f}
@end ftable
%end-doc
*/
  COLOR (ViaColor, "#7f7f7f", "via-color", "color of vias"),

/* %start-doc options "3 Colors"
@ftable @code
@item --via-selected-color <string>
Color of selected vias. Default: @samp{#00ffff}
@end ftable
%end-doc
*/
  COLOR (ViaSelectedColor, "#00ffff", "via-selected-color",
	 "color for selected vias"),

/* %start-doc options "3 Colors"
@ftable @code
@item --pin-color <string>
Color of pins. Default: @samp{#4d4d4d}
@end ftable
%end-doc
*/
  COLOR (PinColor, "#4d4d4d", "pin-color", "color of pins"),

/* %start-doc options "3 Colors"
@ftable @code
@item --pin-selected-color <string>
Color of selected pins. Default: @samp{#00ffff}
@end ftable
%end-doc
*/
  COLOR (PinSelectedColor, "#00ffff", "pin-selected-color",
	 "color of selected pins"),

/* %start-doc options "3 Colors"
@ftable @code
@item --pin-name-color <string>
Color of pin names and pin numbers. Default: @samp{#ff0000}
@end ftable
%end-doc
*/
  COLOR (PinNameColor, "#ff0000", "pin-name-color",
	 "color for pin names and pin numbers"),

/* %start-doc options "3 Colors"
@ftable @code
@item --element-color <string>
Color of components. Default: @samp{#000000}
@end ftable
%end-doc
*/
  COLOR (ElementColor, "#000000", "element-color", "color of components"),

/* %start-doc options "3 Colors"
@ftable @code
@item --rat-color <string>
Color of ratlines. Default: @samp{#b8860b}
@end ftable
%end-doc
*/
  COLOR (RatColor, "#b8860b", "rat-color", "color of ratlines"),

/* %start-doc options "3 Colors"
@ftable @code
@item --invisible-objects-color <string>
Color of invisible objects. Default: @samp{#cccccc}
@end ftable
%end-doc
*/
  COLOR (InvisibleObjectsColor, "#cccccc", "invisible-objects-color",
	 "color of invisible objects"),

/* %start-doc options "3 Colors"
@ftable @code
@item --invisible-mark-color <string>
Color of invisible marks. Default: @samp{#cccccc}
@end ftable
%end-doc
*/
  COLOR (InvisibleMarkColor, "#cccccc", "invisible-mark-color",
	 "color of invisible marks"),

/* %start-doc options "3 Colors"
@ftable @code
@item --element-selected-color <string>
Color of selected components. Default: @samp{#00ffff}
@end ftable
%end-doc
*/
  COLOR (ElementSelectedColor, "#00ffff", "element-selected-color",
	 "color of selected components"),

/* %start-doc options "3 Colors"
@ftable @code
@item --rat-selected-color <string>
Color of selected rats. Default: @samp{#00ffff}
@end ftable
%end-doc
*/
  COLOR (RatSelectedColor, "#00ffff", "rat-selected-color",
	 "color of selected rats"),

/* %start-doc options "3 Colors"
@ftable @code
@item --connected-color <string>
Color to indicate physical connections. Default: @samp{#00ff00}
@end ftable
%end-doc
*/
  COLOR (ConnectedColor, "#00ff00", "connected-color",
	 "color to indicate physically connected objects"),

/* %start-doc options "3 Colors"
@ftable @code
@item --found-color <string>
Color to indicate logical connections. Default: @samp{#ff00ff}
@end ftable
%end-doc
*/
  COLOR (FoundColor, "#ff00ff", "found-color",
	 "color to indicate logically connected objects"),

/* %start-doc options "3 Colors"
@ftable @code
@item --off-limit-color <string>
Color of off-canvas area. Default: @samp{#cccccc}
@end ftable
%end-doc
*/
  COLOR (OffLimitColor, "#cccccc", "off-limit-color",
	 "color of off-canvas area"),

/* %start-doc options "3 Colors"
@ftable @code
@item --grid-color <string>
Color of the grid. Default: @samp{#ff0000}
@end ftable
%end-doc
*/
  COLOR (GridColor, "#ff0000", "grid-color", "color of the grid"),

/* %start-doc options "3 Colors"
@ftable @code
@item --layer-color-<n> <string>
Color of layer @code{<n>}, where @code{<n>} is an integer from 1 to 16.
@end ftable
%end-doc
*/
  LAYERCOLOR (1, "#8b2323"),
  LAYERCOLOR (2, "#3a5fcd"),
  LAYERCOLOR (3, "#104e8b"),
  LAYERCOLOR (4, "#cd3700"),
  LAYERCOLOR (5, "#548b54"),
  LAYERCOLOR (6, "#8b7355"),
  LAYERCOLOR (7, "#00868b"),
  LAYERCOLOR (8, "#228b22"),
  LAYERCOLOR (9, "#8b2323"),
  LAYERCOLOR (10, "#3a5fcd"),
  LAYERCOLOR (11, "#104e8b"),
  LAYERCOLOR (12, "#cd3700"),
  LAYERCOLOR (13, "#548b54"),
  LAYERCOLOR (14, "#8b7355"),
  LAYERCOLOR (15, "#00868b"),
  LAYERCOLOR (16, "#228b22"),
/* %start-doc options "3 Colors"
@ftable @code
@item --layer-selected-color-<n> <string>
Color of layer @code{<n>}, when selected. @code{<n>} is an integer from 1 to 16.
@end ftable
%end-doc
*/
  LAYERSELCOLOR (1),
  LAYERSELCOLOR (2),
  LAYERSELCOLOR (3),
  LAYERSELCOLOR (4),
  LAYERSELCOLOR (5),
  LAYERSELCOLOR (6),
  LAYERSELCOLOR (7),
  LAYERSELCOLOR (8),
  LAYERSELCOLOR (9),
  LAYERSELCOLOR (10),
  LAYERSELCOLOR (11),
  LAYERSELCOLOR (12),
  LAYERSELCOLOR (13),
  LAYERSELCOLOR (14),
  LAYERSELCOLOR (15),
  LAYERSELCOLOR (16),

/* %start-doc options "3 Colors"
@ftable @code
@item --warn-color <string>
Color of offending objects during DRC. Default value is @code{"#ff8000"}
@end ftable
%end-doc
*/
  COLOR (WarnColor, "#ff8000", "warn-color", "color of offending objects during DRC"),

/* %start-doc options "3 Colors"
@ftable @code
@item --mask-color <string>
Color of the mask layer. Default value is @code{"#ff0000"}
@end ftable
%end-doc
*/
  COLOR (MaskColor, "#ff0000", "mask-color", "color for solder mask"),


/* %start-doc options "5 Sizes"
All parameters should be given with an unit. If no unit is given, 1/100 mil
(cmil) will be used. Write units without space to the
number like @code{3mm}, not @code{3 mm}.
Valid Units are:
 @table @samp
   @item km
    Kilometer
   @item m
    Meter
   @item cm
    Centimeter
   @item mm
    Millimeter
   @item um
    Micrometer
   @item nm
    Nanometer
   @item in
    Inch (1in = 0.0254m)
   @item mil
    Mil (1000mil = 1in)
   @item cmil
    Centimil (1/100 mil)
@end table

@ftable @code
@item --via-thickness <num>
Default diameter of vias. Default value is @code{60mil}.
@end ftable
%end-doc
*/
  CSET (ViaThickness, MIL_TO_COORD(60), "via-thickness",
  "default diameter of vias in 1/100 mil"),

/* %start-doc options "5 Sizes"
@ftable @code
@item --via-drilling-hole <num>
Default diameter of holes. Default value is @code{28mil}.
@end ftable
%end-doc
*/
  CSET (ViaDrillingHole, MIL_TO_COORD(28), "via-drilling-hole",
  "default diameter of holes"),

/* %start-doc options "5 Sizes"
@ftable @code
@item --line-thickness <num>
Default thickness of new lines. Default value is @code{10mil}.
@end ftable
%end-doc
*/
  CSET (LineThickness, MIL_TO_COORD(10), "line-thickness",
	"initial thickness of new lines"),

/* %start-doc options "5 Sizes"
@ftable @code
@item --rat-thickness <num><unit>
Thickness of rats. If no unit is given, PCB units are assumed (i.e. 100 
means "1 nm"). This option allows for a special unit @code{px} which 
sets the rat thickness to a fixed value in terms of screen pixels.
Maximum fixed thickness is 100px. Minimum saling rat thickness is 101nm.  
Default value is @code{10mil}.
@end ftable
%end-doc
*/
  CSET (RatThickness, MIL_TO_COORD(10), "rat-thickness", "thickness of rat lines"),

/* %start-doc options "5 Sizes"
@ftable @code
@item --keepaway <num>
Default minimum distance between a track and adjacent copper.
Default value is @code{10mil}.
@end ftable
%end-doc
*/
  CSET (Keepaway, MIL_TO_COORD(10), "keepaway", "minimum distance between adjacent copper"),

/* %start-doc options "5 Sizes"
@ftable @code
@item --default-PCB-width <num>
Default width of the canvas. Default value is @code{6000mil}.
@end ftable
%end-doc
*/
  CSET (MaxWidth, MIL_TO_COORD(6000), "default-PCB-width",
  "default width of the canvas"),

/* %start-doc options "5 Sizes"
@ftable @code
@item --default-PCB-height <num>
Default height of the canvas. Default value is @code{5000mil}.
@end ftable
%end-doc
*/
  CSET (MaxHeight, MIL_TO_COORD(5000), "default-PCB-height",
  "default height of the canvas"),

/* %start-doc options "5 Sizes"
@ftable @code
@item --text-scale <num>
Default text scale. This value is in percent. Default value is @code{100}.
@end ftable
%end-doc
*/
  ISET (TextScale, 100, "text-scale", "default text scale in percent"),

/* %start-doc options "5 Sizes"
@ftable @code
@item --alignment-distance <num>
Specifies the distance between the board outline and alignment targets.
Default value is @code{2mil}.
@end ftable
%end-doc
*/
  CSET (AlignmentDistance, MIL_TO_COORD(2), "alignment-distance",
  "distance between the boards outline and alignment targets"),

/* %start-doc options "7 DRC Options"
All parameters should be given with an unit. If no unit is given, 1/100 mil
(cmil) will be used for backward compability. Valid units are given in section
@ref{Sizes}.
%end-doc
*/


/* %start-doc options "7 DRC Options"
@ftable @code
@item --bloat <num>
Minimum spacing. Default value is @code{10mil}.
@end ftable
%end-doc
*/
  CSET (Bloat, MIL_TO_COORD(10), "bloat", "DRC minimum spacing in 1/100 mil"),

/* %start-doc options "7 DRC Options"
@ftable @code
@item --shrink <num>
Minimum touching overlap. Default value is @code{10mil}.
@end ftable
%end-doc
*/
  CSET (Shrink, MIL_TO_COORD(10), "shrink", "DRC minimum overlap in 1/100 mils"),

/* %start-doc options "7 DRC Options"
@ftable @code
@item --min-width <num>
Minimum width of copper. Default value is @code{10mil}.
@end ftable
%end-doc
*/
  CSET (minWid, MIL_TO_COORD(10), "min-width", "DRC minimum copper spacing"),

/* %start-doc options "7 DRC Options"
@ftable @code
@item --min-silk <num>
Minimum width of lines in silk. Default value is @code{10mil}.
@end ftable
%end-doc
*/
  CSET (minSlk, MIL_TO_COORD(10), "min-silk", "DRC minimum silk width"),

/* %start-doc options "7 DRC Options"
@ftable @code
@item --min-drill <num>
Minimum diameter of holes. Default value is @code{15mil}.
@end ftable
%end-doc
*/
  CSET (minDrill, MIL_TO_COORD(15), "min-drill", "DRC minimum drill diameter"),

/* %start-doc options "7 DRC Options"
@ftable @code
@item --min-ring <num>
Minimum width of annular ring. Default value is @code{10mil}.
@end ftable
%end-doc
*/
  CSET (minRing, MIL_TO_COORD(10), "min-ring", "DRC minimum annular ring"),


/* %start-doc options "5 Sizes"
@ftable @code
@item --grid <num>
Initial grid size. Default value is @code{10mil}.
@end ftable
%end-doc
*/
  CSET (Grid, MIL_TO_COORD(10), "grid", "Initial grid size in 1/100 mil"),

/* %start-doc options "5 Sizes"
@ftable @code
@item --minimum polygon area <num>
Minimum polygon area.
@end ftable
%end-doc
*/
  RSET (IsleArea, MIL_TO_COORD(100) * MIL_TO_COORD(100), "minimum polygon area", 0),


/* %start-doc options "1 General Options"
@ftable @code
@item --backup-interval
Time between automatic backups in seconds. Set to @code{0} to disable.
The default value is @code{60}.
@end ftable
%end-doc
*/
  ISET (BackupInterval, 60, "backup-interval",
  "Time between automatic backups in seconds. Set to 0 to disable"),

/* %start-doc options "4 Layer Names"
@ftable @code
@item --layer-name-1 <string>
Name of the 1st Layer. Default is @code{"top"}.
@end ftable
%end-doc
*/
  LAYERNAME (1, "top"),

/* %start-doc options "4 Layer Names"
@ftable @code
@item --layer-name-2 <string>
Name of the 2nd Layer. Default is @code{"ground"}.
@end ftable
%end-doc
*/
  LAYERNAME (2, "ground"),

/* %start-doc options "4 Layer Names"
@ftable @code
@item --layer-name-3 <string>
Name of the 3nd Layer. Default is @code{"signal2"}.
@end ftable
%end-doc
*/
  LAYERNAME (3, "signal2"),

/* %start-doc options "4 Layer Names"
@ftable @code
@item --layer-name-4 <string>
Name of the 4rd Layer. Default is @code{"signal3"}.
@end ftable
%end-doc
*/
  LAYERNAME (4, "signal3"),

/* %start-doc options "4 Layer Names"
@ftable @code
@item --layer-name-5 <string>
Name of the 5rd Layer. Default is @code{"power"}.
@end ftable
%end-doc
*/
  LAYERNAME (5, "power"),

/* %start-doc options "4 Layer Names"
@ftable @code
@item --layer-name-6 <string>
Name of the 6rd Layer. Default is @code{"bottom"}.
@end ftable
%end-doc
*/
  LAYERNAME (6, "bottom"),

/* %start-doc options "4 Layer Names"
@ftable @code
@item --layer-name-7 <string>
Name of the 7rd Layer. Default is @code{"outline"}.
@end ftable
%end-doc
*/
  LAYERNAME (7, "outline"),

/* %start-doc options "4 Layer Names"
@ftable @code
@item --layer-name-8 <string>
Name of the 8rd Layer. Default is @code{"spare"}.
@end ftable
%end-doc
*/
  LAYERNAME (8, "spare"),

/* %start-doc options "1 General Options"
@ftable @code
@item --groups <string>
Layer group string. Defaults to @code{"1,c:2:3:4:5:6,s:7:8"}.
@end ftable
%end-doc
*/
  SSET (Groups, "1,c:2:3:4:5:6,s:7:8", "groups", "Layer group string"),


/* %start-doc options "6 Commands"
pcb uses external commands for input output operations. These commands can be
configured at start-up to meet local requirements. The command string may include
special sequences @code{%f}, @code{%p} or @code{%a}. These are replaced when the
command is called. The sequence @code{%f} is replaced by the file name,
@code{%p} gets the path and @code{%a} indicates a package name.
%end-doc
*/

/* %start-doc options "6 Commands"
@ftable @code
@item --font-command <string>
Command to load a font.
@end ftable
%end-doc
*/
  SSET (FontCommand, "", "font-command", "Command to load a font"),

/* %start-doc options "6 Commands"
@ftable @code
@item --file-command <string>
Command to read a file.
@end ftable
%end-doc
*/
  SSET (FileCommand, "", "file-command", "Command to read a file"),

/* %start-doc options "6 Commands"
@ftable @code
@item --element-command <string>
Command to read a footprint. @*
Defaults to @code{"M4PATH='%p';export M4PATH;echo 'include(%f)' | m4"}
@end ftable
%end-doc
*/
  SSET (ElementCommand,
	"M4PATH='%p';export M4PATH;echo 'include(%f)' | " GNUM4,
	"element-command", "Command to read a footprint"),

/* %start-doc options "6 Commands"
@ftable @code
@item --print-file <string>
Command to print to a file.
@end ftable
%end-doc
*/
  SSET (PrintFile, "%f.output", "print-file", "Command to print to a file"),

/* %start-doc options "6 Commands"
@ftable @code
@item --lib-command-dir <string>
Path to the command that queries the library.
@end ftable
%end-doc
*/
  SSET (LibraryCommandDir, PCBLIBDIR, "lib-command-dir",
       "Path to the command that queries the library"),

/* %start-doc options "6 Commands"
@ftable @code
@item --lib-command <string>
Command to query the library. @*
Defaults to @code{"QueryLibrary.sh '%p' '%f' %a"}
@end ftable
%end-doc
*/
  SSET (LibraryCommand, "QueryLibrary.sh '%p' '%f' %a",
       "lib-command", "Command to query the library"),

/* %start-doc options "6 Commands"
@ftable @code
@item --lib-contents-command <string>
Command to query the contents of the library. @*
Defaults to @code{"ListLibraryContents.sh %p %f"} or,
on Windows builds, an empty string (to disable this feature).
@end ftable
%end-doc
*/
  SSET (LibraryContentsCommand,
#ifdef __WIN32__
	"",
#else
	"ListLibraryContents.sh '%p' '%f'",
#endif
	"lib-contents-command", "Command to query the contents of the library"),

/* %start-doc options "5 Paths"
@ftable @code
@item --lib-newlib <string>
Top level directory for the newlib style library.
@end ftable
%end-doc
*/
  SSET (LibraryTree, PCBTREEPATH, "lib-newlib",
	"Top level directory for the newlib style library"),

/* %start-doc options "6 Commands"
@ftable @code
@item --save-command <string>
Command to save to a file.
@end ftable
%end-doc
*/
  SSET (SaveCommand, "", "save-command", "Command to save to a file"),

/* %start-doc options "5 Paths"
@ftable @code
@item --lib-name <string>
The default filename for the library.
@end ftable
%end-doc
*/
  SSET (LibraryFilename, LIBRARYFILENAME, "lib-name",
				"The default filename for the library"),

/* %start-doc options "5 Paths"
@ftable @code
@item --default-font <string>
The name of the default font.
@end ftable
%end-doc
*/
  SSET (FontFile, "default_font", "default-font",
	"File name of default font"),

/* %start-doc options "1 General Options"
@ftable @code
@item --route-styles <string>
A string that defines the route styles. Defaults to @*
@code{"Signal,1000,3600,2000,1000:Power,2500,6000,3500,1000
	:Fat,4000,6000,3500,1000:Skinny,600,2402,1181,600"}
@end ftable
%end-doc
*/
  SSET (Routes, "Signal,1000,3600,2000,1000:Power,2500,6000,3500,1000"
	":Fat,4000,6000,3500,1000:Skinny,600,2402,1181,600", "route-styles",
	"A string that defines the route styles"),

/* %start-doc options "5 Paths"
@ftable @code
@item --file-path <string>
A colon separated list of directories or commands (starts with '|'). The path
is passed to the program specified in @option{--file-command} together with the selected
filename.
@end ftable
%end-doc
*/
  SSET (FilePath, "", "file-path", 0),

/* %start-doc options "6 Commands"
@ftable @code
@item --rat-command <string>
Command for reading a netlist. Sequence @code{%f} is replaced by the netlist filename.
@end ftable
%end-doc
*/
  SSET (RatCommand, "", "rat-command", "Command for reading a netlist"),

/* %start-doc options "5 Paths"
@ftable @code
@item --font-path <string>
A colon separated list of directories to search the default font. Defaults to
the default library path.
@end ftable
%end-doc
*/
  SSET (FontPath, PCBLIBPATH, "font-path",
       "Colon separated list of directories to search the default font"),

/* %start-doc options "1 General Options"
@ftable @code
@item --element-path <string>
A colon separated list of directories or commands (starts with '|').
The path is passed to the program specified in @option{--element-command}.
@end ftable
%end-doc
*/
  SSET(ElementPath, PCBLIBPATH, "element-path",
      "A colon separated list of directories or commands (starts with '|')"),

/* %start-doc options "5 Paths"
@ftable @code
@item --lib-path <string>
A colon separated list of directories that will be passed to the commands specified
by @option{--element-command} and @option{--element-contents-command}.
@end ftable
%end-doc
*/
  SSET (LibraryPath, PCBLIBPATH, "lib-path",
       "A colon separated list of directories"),

/* %start-doc options "1 General Options"
@ftable @code
@item --action-script <string>
If set, this file is executed at startup.
@end ftable
%end-doc
*/
  SSET (ScriptFilename, 0, "action-script",
	     "If set, this file is executed at startup"),

/* %start-doc options "1 General Options"
@ftable @code
@item --action-string <string>
If set, this string of actions is executed at startup.
@end ftable
%end-doc
*/
  SSET (ActionString, 0, "action-string",
       "If set, this is executed at startup"),

/* %start-doc options "1 General Options"
@ftable @code
@item --fab-author <string>
Name of author to be put in the Gerber files.
@end ftable
%end-doc
*/
  SSET (FabAuthor, "", "fab-author",
       "Name of author to be put in the Gerber files"),

/* %start-doc options "1 General Options"
@ftable @code
@item --layer-stack <string>
Initial layer stackup, for setting up an export. A comma separated list of layer
names, layer numbers and layer groups.
@end ftable
%end-doc
*/
  SSET (InitialLayerStack, "", "layer-stack",
	"Initial layer stackup, for setting up an export."),

  SSET (MakeProgram, NULL, "make-program",
	"Sets the name and optionally full path to a make(3) program"),
  SSET (GnetlistProgram, NULL, "gnetlist",
	"Sets the name and optionally full path to the gnetlist(3) program"),

/* %start-doc options "2 General GUI Options"
@ftable @code
@item --pinout-offset-x <num>
Horizontal offset of the pin number display. Defaults to @code{100mil}.
@end ftable
%end-doc
*/
  CSET (PinoutOffsetX, MIL_TO_COORD(1), "pinout-offset-x",
       "Horizontal offset of the pin number display in mil"),

/* %start-doc options "2 General GUI Options"
@ftable @code
@item --pinout-offset-y <num>
Vertical offset of the pin number display. Defaults to @code{100mil}.
@end ftable
%end-doc
*/
  CSET (PinoutOffsetY, MIL_TO_COORD(1), "pinout-offset-y",
       "Vertical offset of the pin number display in mil"),

/* %start-doc options "2 General GUI Options"
@ftable @code
@item --pinout-text-offset-x <num>
Horizontal offset of the pin name display. Defaults to @code{800mil}.
@end ftable
%end-doc
*/
  CSET (PinoutTextOffsetX, MIL_TO_COORD(8), "pinout-text-offset-x",
       "Horizontal offset of the pin name display in mil"),

/* %start-doc options "2 General GUI Options"
@ftable @code
@item --pinout-text-offset-y <num>
Vertical offset of the pin name display. Defaults to @code{-100mil}.
@end ftable
%end-doc
*/
  CSET (PinoutTextOffsetY, MIL_TO_COORD(-1), "pinout-text-offset-y",
       "Vertical offset of the pin name display in mil"),

/* %start-doc options "2 General GUI Options"
@ftable @code
@item --draw-grid
If set, draw the grid at start-up.
@end ftable
%end-doc
*/
  BSET (DrawGrid, 0, "draw-grid", "If set, draw the grid at start-up"),

/* %start-doc options "2 General GUI Options"
@ftable @code
@item --clear-line
If set, new lines clear polygons.
@end ftable
%end-doc
*/
  BSET (ClearLine, 1, "clear-line", "If set, new lines clear polygons"),

/* %start-doc options "2 General GUI Options"
@ftable @code
@item --full-poly
If set, new polygons are full ones.
@end ftable
%end-doc
*/
  BSET (FullPoly, 0, "full-poly", 0),

/* %start-doc options "2 General GUI Options"
@ftable @code
@item --unique-names
If set, you will not be permitted to change the name of an component to match that
of another component.
@end ftable
%end-doc
*/
  BSET (UniqueNames, 1, "unique-names", "Prevents identical component names"),

/* %start-doc options "2 General GUI Options"
@ftable @code
@item --snap-pin
If set, pin centers and pad end points are treated as additional grid points
that the cursor can snap to.
@end ftable
%end-doc
*/
  BSET (SnapPin, 1, "snap-pin",
       "If set, the cursor snaps to pads and pin centers"),

/* %start-doc options "1 General Options"
@ftable @code
@item --save-last-command
If set, the last user command is saved.
@end ftable
%end-doc
*/
  BSET (SaveLastCommand, 0, "save-last-command", 0),

/* %start-doc options "1 General Options"
@ftable @code
@item --save-in-tmp
If set, all data which would otherwise be lost are saved in a temporary file
@file{/tmp/PCB.%i.save} . Sequence @samp{%i} is replaced by the process ID.
@end ftable
%end-doc
*/
  BSET (SaveInTMP, 0, "save-in-tmp",
       "When set, all data which would otherwise be lost are saved in /tmp"),

/* %start-doc options "1 General Options"
@ftable @code
@item --save-metric-only
If set, save pcb files using only mm unit suffix rather than 'smart' mil/mm.
@end ftable
%end-doc
*/
  BSET (SaveMetricOnly, 0, "save-metric-only",
        "If set, save pcb files using only mm unit suffix rather than 'smart' mil/mm."),

/* %start-doc options "2 General GUI Options"
@ftable @code
@item --all-direction-lines
Allow all directions, when drawing new lines.
@end ftable
%end-doc
*/
  BSET (AllDirectionLines, 0, "all-direction-lines",
       "Allow all directions, when drawing new lines"),

/* %start-doc options "2 General GUI Options"
@ftable @code
@item --show-number
Pinout shows number.
@end ftable
%end-doc
*/
  BSET (ShowNumber, 0, "show-number", "Pinout shows number"),

/* %start-doc options "1 General Options"
@ftable @code
@item --reset-after-element
If set, all found connections are reset before a new component is scanned.
@end ftable
%end-doc
*/
  BSET (ResetAfterElement, 1, "reset-after-element",
       "If set, all found connections are reset before a new component is scanned"),

/* %start-doc options "1 General Options"
 * @ftable @code
 * @item --auto-buried-vias
 * Enables automatically created vias (during line moves and layer switch) to be buriad/blind vias
 * @end ftable
 * %end-doc
 * */
  BSET (AutoBuriedVias, 0, "auto-buried-vias",
       "Enables automatically created vias to be buriad/blind vias"),

/* %start-doc options "1 General Options"
@ftable @code
@item --ring-bell-finished
Execute the bell command when all rats are routed.
@end ftable
%end-doc
*/
  BSET (RingBellWhenFinished, 0, "ring-bell-finished",
       "Execute the bell command when all rats are routed"),
};

REGISTER_ATTRIBUTES (main_attribute_list)
/* ---------------------------------------------------------------------- 
 * post-process settings.
 */
     static void settings_post_process ()
{
  char *tmps;

  if (Settings.LibraryCommand != NULL &&
      Settings.LibraryCommand[0] != '\0' &&
      Settings.LibraryCommand[0] != PCB_DIR_SEPARATOR_C &&
      Settings.LibraryCommand[0] != '.')
    {
      Settings.LibraryCommand
	=
	Concat (Settings.LibraryCommandDir, PCB_DIR_SEPARATOR_S, 
		Settings.LibraryCommand,
		NULL);
    }
  if (Settings.LibraryContentsCommand != NULL &&
      Settings.LibraryContentsCommand[0] != '\0' &&
      Settings.LibraryContentsCommand[0] != PCB_DIR_SEPARATOR_C &&
      Settings.LibraryContentsCommand[0] != '.')
    {
      Settings.LibraryContentsCommand
	=
	Concat (Settings.LibraryCommandDir, PCB_DIR_SEPARATOR_S,
		Settings.LibraryContentsCommand, NULL);
    }

  if (Settings.LineThickness > MAX_LINESIZE
      || Settings.LineThickness < MIN_LINESIZE)
    Settings.LineThickness = MIL_TO_COORD(10);

  if (Settings.ViaThickness > MAX_PINORVIASIZE
      || Settings.ViaThickness < MIN_PINORVIASIZE)
    Settings.ViaThickness = MIL_TO_COORD(40);

  if (Settings.ViaDrillingHole <= 0)
    Settings.ViaDrillingHole =
      DEFAULT_DRILLINGHOLE * Settings.ViaThickness / 100;

  Settings.MaxWidth  = CLAMP (Settings.MaxWidth, MIN_SIZE, MAX_COORD);
  Settings.MaxHeight = CLAMP (Settings.MaxHeight, MIN_SIZE, MAX_COORD);

  ParseRouteString (Settings.Routes, &Settings.RouteStyle[0], "cmil");

  /*
   * Make sure we have settings for some various programs we may wish
   * to call
   */
  if (Settings.MakeProgram == NULL) {
    tmps = getenv ("PCB_MAKE_PROGRAM");
    if (tmps != NULL)
      Settings.MakeProgram = strdup (tmps);
  }
  if (Settings.MakeProgram == NULL) {
    Settings.MakeProgram = strdup ("make");
  }

  if (Settings.GnetlistProgram == NULL) {
    tmps = getenv ("PCB_GNETLIST");
    if (tmps != NULL)
      Settings.GnetlistProgram = strdup (tmps);
  }
  if (Settings.GnetlistProgram == NULL) {
    Settings.GnetlistProgram = strdup ("gnetlist");
  }

  if (grid_units)
    Settings.grid_unit = get_unit_struct (grid_units);
  if (!grid_units || Settings.grid_unit == NULL)
    Settings.grid_unit = get_unit_struct ("mil");

  copy_nonzero_increments (get_increments_struct (METRIC), &increment_mm);
  copy_nonzero_increments (get_increments_struct (IMPERIAL), &increment_mil);

  Settings.increments = get_increments_struct (Settings.grid_unit->family);
}

/*!
 * \brief Print help or version messages.
 */
static void
print_version ()
{
  printf ("PCB version %s\n", VERSION);
  exit (0);
}

/* ----------------------------------------------------------------------
 * Figure out the canonical name of the executed program
 * and fix up the defaults for various paths
 */
char *bindir = NULL;
char *exec_prefix = NULL;
char *pcblibdir = NULL;
char *pcblibpath = NULL;
char *pcbtreedir = NULL;
char *pcbtreepath = NULL;
char *homedir = NULL;

/*!
 * \brief See if argv0 has enough of a path to let lrealpath give the
 * real path.
 *
 * This should be the case if you invoke pcb with something like
 * /usr/local/bin/pcb or ./pcb or ./foo/pcb but if you just use pcb and
 * it exists in your path, you'll just get back pcb again.
 */
static void
InitPaths (char *argv0)
{
  size_t l;
  int i;
  int haspath;
  char *t1, *t2;
  int found_bindir = 0;

  haspath = 0;
  for (i = 0; i < strlen (argv0) ; i++)
    {
      if (argv0[i] == PCB_DIR_SEPARATOR_C) 
	haspath = 1;
    }

#ifdef DEBUG
  printf ("InitPaths (%s): haspath = %d\n", argv0, haspath);
#endif

  if (haspath)
    {
      bindir = strdup (lrealpath (argv0));
      found_bindir = 1;
    }
  else
    {
      char *path, *p, *tmps;
      struct stat sb;
      int r;

      tmps = getenv ("PATH");

      if (tmps != NULL) 
	{
	  path = strdup (tmps);

	  /* search through the font path for a font file */
	  for (p = strtok (path, PCB_PATH_DELIMETER); p && *p;
	       p = strtok (NULL, PCB_PATH_DELIMETER))
	    {
#ifdef DEBUG
	      printf ("Looking for %s in %s\n", argv0, p);
#endif
	      if ( (tmps = (char *)malloc ( (strlen (argv0) + strlen (p) + 2) * sizeof (char))) == NULL )
		{
		  fprintf (stderr, "InitPaths():  malloc failed\n");
		  exit (1);
		}
	      sprintf (tmps, "%s%s%s", p, PCB_DIR_SEPARATOR_S, argv0);
	      r = stat (tmps, &sb);
	      if (r == 0)
		{
#ifdef DEBUG
		  printf ("Found it:  \"%s\"\n", tmps);
#endif
		  bindir = lrealpath (tmps);
		  found_bindir = 1;
		  free (tmps);
		  break;
		}  
	      free (tmps);
	    }
	  free (path);
	}
    }

#ifdef DEBUG
  printf ("InitPaths():  bindir = \"%s\"\n", bindir);
#endif

  if (found_bindir)
    {
      /* strip off the executible name leaving only the path */
      t2 = NULL;
      t1 = strchr (bindir, PCB_DIR_SEPARATOR_C);
      while (t1 != NULL && *t1 != '\0')
        {
          t2 = t1;
          t1 = strchr (t2 + 1, PCB_DIR_SEPARATOR_C);
        }
      if (t2 != NULL)
        *t2 = '\0';

#ifdef DEBUG
      printf ("After stripping off the executible name, we found\n");
      printf ("bindir = \"%s\"\n", bindir);
#endif
    }
  else
    {
      /* we have failed to find out anything from argv[0] so fall back to the original
       * install prefix
       */
       bindir = strdup (BINDIR);
    }

  /* now find the path to exec_prefix */
  l = strlen (bindir) + 1 + strlen (BINDIR_TO_EXECPREFIX) + 1;
  if ( (exec_prefix = (char *) malloc (l * sizeof (char) )) == NULL )
    {
      fprintf (stderr, "InitPaths():  malloc failed\n");
      exit (1);
    }
  sprintf (exec_prefix, "%s%s%s", bindir, PCB_DIR_SEPARATOR_S, 
	   BINDIR_TO_EXECPREFIX);

  /* now find the path to PCBLIBDIR */
  l = strlen (bindir) + 1 + strlen (BINDIR_TO_PCBLIBDIR) + 1;
  if ( (pcblibdir = (char *) malloc (l * sizeof (char) )) == NULL )
    {
      fprintf (stderr, "InitPaths():  malloc failed\n");
      exit (1);
    }
  sprintf (pcblibdir, "%s%s%s", bindir, PCB_DIR_SEPARATOR_S, 
	   BINDIR_TO_PCBLIBDIR);

  /* and the path to PCBTREEDIR */
  l = strlen (bindir) + 1 + strlen (BINDIR_TO_PCBTREEDIR) + 1;
  if ( (pcbtreedir = (char *) malloc (l * sizeof (char) )) == NULL )
    {
      fprintf (stderr, "InitPaths():  malloc failed\n");
      exit (1);
    }
  sprintf (pcbtreedir, "%s%s%s", bindir, PCB_DIR_SEPARATOR_S, 
	   BINDIR_TO_PCBTREEDIR);

  /* and the search path including PCBLIBDIR */
  l = strlen (pcblibdir) + 3;
  if ( (pcblibpath = (char *) malloc (l * sizeof (char) )) == NULL )
    {
      fprintf (stderr, "InitPaths():  malloc failed\n");
      exit (1);
    }
  sprintf (pcblibpath, ".%s%s", PCB_PATH_DELIMETER, pcblibdir);

  /* and the newlib search path */
  l = strlen (pcblibdir) + 1 + strlen (pcbtreedir) 
    + strlen ("pcblib-newlib") + 2;
  if ( (pcbtreepath = (char *) malloc (l * sizeof (char) )) == NULL )
    {
      fprintf (stderr, "InitPaths():  malloc failed\n");
      exit (1);
    }
  sprintf (pcbtreepath, "%s%s%s%spcblib-newlib", pcbtreedir, 
	PCB_PATH_DELIMETER, pcblibdir,
	PCB_DIR_SEPARATOR_S);
  
#ifdef DEBUG
  printf ("bindir      = %s\n", bindir);
  printf ("pcblibdir   = %s\n", pcblibdir);
  printf ("pcblibpath  = %s\n", pcblibpath);
  printf ("pcbtreedir  = %s\n", pcbtreedir);
  printf ("pcbtreepath = %s\n", pcbtreepath);
#endif

  l = sizeof (main_attribute_list) / sizeof (main_attribute_list[0]);
  for (i = 0; i < l ; i++) 
    {
      if (NSTRCMP (main_attribute_list[i].name, "lib-command-dir") == 0)
	{
	  main_attribute_list[i].default_val.str_value = pcblibdir;
	}

      if ( (NSTRCMP (main_attribute_list[i].name, "font-path") == 0) 
	   || (NSTRCMP (main_attribute_list[i].name, "element-path") == 0)
	   || (NSTRCMP (main_attribute_list[i].name, "lib-path") == 0) )
	{
	  main_attribute_list[i].default_val.str_value = pcblibpath;
	}

      if (NSTRCMP (main_attribute_list[i].name, "lib-newlib") == 0)
	{
	  main_attribute_list[i].default_val.str_value = pcbtreepath;
	}

    }

    {
      char *tmps;

      tmps = getenv ("HOME");

      if (tmps == NULL) {
          tmps = getenv ("USERPROFILE");
      }

      if (tmps != NULL) {
          homedir = strdup (tmps);
      } else {
          homedir = NULL;
      }

    }
}

/* ---------------------------------------------------------------------- 
 * main program
 */

char *program_name = 0;
char *program_basename = 0;
char *program_directory = 0;

#include "dolists.h"

/*!
 * \brief Free up memory allocated to the PCB.
 *
 * Why bother when we're about to exit ?\n
 * Because it removes some false positives from heap bug detectors such
 * as lib dmalloc.
 */
void
pcb_main_uninit (void)
{
  if (gui->uninit != NULL)
    gui->uninit (gui);

  hid_uninit ();

  UninitBuffers ();

  FreePCBMemory (PCB);
  free (PCB);
  PCB = NULL;

  /*! 
   * \warning Please do not free the below variables like:
   * \code
  for (i = 0; i < MAX_LAYER; i++)
    free (Settings.DefaultLayerName[i]);

  if (Settings.FontFile != NULL)
    {
      free (Settings.FontFile);
      Settings.FontFile = NULL;
    }
   * \endcode
   * as these are initialized to static strings and freeing them
   * causes segfaults when terminating the program.
   */

  uninit_strflags_buf ();
  uninit_strflags_layerlist ();

#define free0(ptr) \
  do \
    { \
      if (ptr != NULL) \
        { \
          free (ptr); \
          ptr = 0; \
        } \
    } while (0)

  free0 (pcblibdir);
  free0 (homedir);
  free0 (bindir);
  free0 (exec_prefix);
  free0 (program_directory);
  free0 (Settings.MakeProgram);
  free0 (Settings.GnetlistProgram);

#undef free0
}

/*!
 * \brief Main program.
 *
 * Init application:
 *
 * - make program name available for error handlers
 * - evaluate special options
 * - initialize toplevel shell and resources
 * - create an empty PCB with default symbols
 * - initialize all other widgets
 * - update screen and get size of drawing area
 * - evaluate command-line arguments
 * - register 'call on exit()' function
 */
int
main (int argc, char *argv[])
{
  int i;

#include "core_lists.h"
  setbuf (stdout, 0);
  InitPaths (argv[0]);

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  textdomain (GETTEXT_PACKAGE);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

#ifdef ENABLE_NLS
  setlocale (LC_ALL, "");
  setlocale (LC_NUMERIC, "C");
#endif

  srand ( time(NULL) ); /* Set seed for rand() */

  initialize_units();
  polygon_init ();
  hid_init ();

  hid_load_settings ();

  program_name = argv[0];
  program_basename = strrchr (program_name, PCB_DIR_SEPARATOR_C);
  if (program_basename)
    {
      program_directory = strdup (program_name);
      *strrchr (program_directory, PCB_DIR_SEPARATOR_C) = 0;
      program_basename++;
    }
  else
    {
      program_directory = strdup (".");
      program_basename = program_name;
    }
  Progname = program_basename;

  /* Print usage or version if requested.  Then exit.  */  
  if (argc > 1 &&
      (strcmp (argv[1], "-h") == 0 ||
       strcmp (argv[1], "-?") == 0 ||
       strcmp (argv[1], "--help") == 0))
    usage ();
  if (argc > 1 && strcmp (argv[1], "-V") == 0)
    print_version ();
  /* Export pcb from command line if requested.  */
  if (argc > 1 && strcmp (argv[1], "-p") == 0)
    {
      exporter = gui = hid_find_printer ();
      argc--;
      argv++;
    }
  else if (argc > 2 && strcmp (argv[1], "-x") == 0)
    {
      exporter = gui = hid_find_exporter (argv[2]);
      argc -= 2;
      argv += 2;
    }
    /* Otherwise start GUI. */
  else
    gui = hid_find_gui ();

  /* Exit with error if GUI failed to start. */
  if (!gui)
    exit (1);

  /* Set up layers. */
  for (i = 0; i < MAX_LAYER; i++)
    {
      char buf[20];
      sprintf (buf, "signal%d", i + 1);
      Settings.DefaultLayerName[i] = strdup (buf);
      Settings.LayerColor[i] = "#c49350";
      Settings.LayerSelectedColor[i] = "#00ffff";
    }

  gui->parse_arguments (&argc, &argv);

  if (show_help || (argc > 1 && argv[1][0] == '-'))
    usage ();
  if (show_version)
    print_version ();
  if (show_defaults)
    print_defaults ();
  if (show_copyright)
    copyright ();

  settings_post_process ();


  if (show_actions)
    {
      print_actions ();
      exit (0);
    }

  if (do_dump_actions)
    {
      extern void dump_actions (void);
      dump_actions ();
      exit (0);
    }

  /* Create a new PCB object in memory */
  PCB = CreateNewPCB ();
  ParseGroupString (Settings.Groups, &PCB->LayerGroups, &PCB->Data->LayerN);
  /* Add silk layers to newly created PCB */
  CreateNewPCBPost (PCB, 1);
  if (argc > 1)
    command_line_pcb = argv[1];

  ResetStackAndVisibility ();

  if (gui->gui)
    InitCrosshair ();
  InitHandler ();
  InitBuffers ();
  SetMode (ARROW_MODE);

  if (command_line_pcb)
    {
      if (access(command_line_pcb, F_OK))
        {
	   /* File does not exist, save the filename and continue with empty board */
	   PCB->Filename = strdup (command_line_pcb);
	} else {
	   /* Hard fail if file exists and fails to load */
           if (LoadPCB (command_line_pcb))
             {
	       fprintf(stderr, "LoadPCB: Failed to load existing file \"%s\". Is it supported PCB file?\n", command_line_pcb);
	       exit(1);
	     }
        }
    }

  if (Settings.InitialLayerStack
      && Settings.InitialLayerStack[0])
    {
      LayerStringToLayerStack (Settings.InitialLayerStack);
    }

  /* This must be called before any other atexit functions
   * are registered, as it configures an atexit function to
   * clean up and free various items of allocated memory,
   * and must be the last last atexit function to run.
   */
  leaky_init ();

  /* Register a function to be called when the program terminates.
   * This makes sure that data is saved even if LEX/YACC routines
   * abort the program.
   * If the OS doesn't have at least one of them,
   * the critical sections will be handled by parse_l.l
   */
  atexit (EmergencySave);

  /* read the library file and display it if it's not empty
   */
  if (!ReadLibraryContents () && Library.MenuN)
    hid_action ("LibraryChanged");

#ifdef HAVE_LIBSTROKE
  stroke_init ();
#endif

  if (Settings.ScriptFilename)
    {
      Message (_("Executing startup script file %s\n"),
	       Settings.ScriptFilename);
      hid_actionl ("ExecuteFile", Settings.ScriptFilename, NULL);
    }
  if (Settings.ActionString)
    {
      Message (_("Executing startup action %s\n"), Settings.ActionString);
      hid_parse_actions (Settings.ActionString);
    }

  if (gui->printer || gui->exporter)
    {
      gui->do_export (0);
      exit (0);
    }

#if HAVE_DBUS
  pcb_dbus_setup();
#endif

  EnableAutosave ();

#ifdef DEBUG
  printf ("Settings.LibraryCommandDir = \"%s\"\n",
          Settings.LibraryCommandDir);
  printf ("Settings.FontPath          = \"%s\"\n", 
          Settings.FontPath);
  printf ("Settings.ElementPath       = \"%s\"\n", 
          Settings.ElementPath);
  printf ("Settings.LibraryPath       = \"%s\"\n", 
          Settings.LibraryPath);
  printf ("Settings.LibraryTree       = \"%s\"\n", 
          Settings.LibraryTree);
  printf ("Settings.MakeProgram = \"%s\"\n",
          UNKNOWN (Settings.MakeProgram));
  printf ("Settings.GnetlistProgram = \"%s\"\n",
          UNKNOWN (Settings.GnetlistProgram));
#endif

  gui->do_export (0);
#if HAVE_DBUS
  pcb_dbus_finish();
#endif

  pcb_main_uninit ();

  return (0);
}

