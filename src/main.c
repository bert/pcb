/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2004 Thomas Nau
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */


/* main program, initializes some stuff and handles user input
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

/* This next one is so we can print the help messages. */
#include "hid/hidint.h"

#ifdef HAVE_DBUS
#include "dbus.h"
#endif

#if ENABLE_NLS
#include <libintl.h>
#endif

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");


#define PCBLIBPATH ".:" PCBLIBDIR


#ifdef HAVE_LIBSTROKE
extern void stroke_init (void);
#endif


/* ----------------------------------------------------------------------
 * initialize signal and error handlers
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
	  "    You should have received a copy of the GNU General Public License\n"
	  "    along with this program; if not, write to the Free Software\n"
	  "    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n\n",
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

static void
usage_attr (HID_Attribute * a)
{
  int i;
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
  HID_Attribute *e;
  int i, n;

  if (h->gui)
    {
      HID **hl = hid_enumerate ();
      HID_AttrNode *ha;
      fprintf (stderr, "\ngui options:\n");
      for (ha = hid_attr_nodes; ha; ha = ha->next)
	{
	  for (i = 0; hl[i]; i++)
	    if (ha->attributes == hl[i]->get_export_options (0))
	      goto skip_this_one;
	  for (i = 0; i < ha->n; i++)
	    usage_attr (ha->attributes + i);
	skip_this_one:;
	}
      return;
    }
  fprintf (stderr, "\n%s options:\n", h->name);
  exporter = h;
  e = h->get_export_options (&n);
  if (!e)
    return;
  for (i = 0; i < n; i++)
    usage_attr (e + i);
  exporter = NULL;
}

static void
usage (void)
{
  HID **hl = hid_enumerate ();
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

  u ("PCB Printed Circuit Board editing program, http://pcb.sourceforge.net");
  u ("%s [-h|-V|--copyright]\t\t\tHelp, version, copyright", Progname);
  u ("%s [gui options] <pcb file>\t\tto edit", Progname);
  u ("Available GUI hid%s:", n_gui == 1 ? "" : "s");
  for (i = 0; hl[i]; i++)
    if (hl[i]->gui)
      fprintf (stderr, "\t%s\t%s\n", hl[i]->name, hl[i]->description);
  u ("%s -p [printing options] <pcb file>\tto print", Progname);
  u ("Available printing hid%s:", n_printer == 1 ? "" : "s");
  for (i = 0; hl[i]; i++)
    if (hl[i]->printer)
      fprintf (stderr, "\t%s\t%s\n", hl[i]->name, hl[i]->description);
  u ("%s -x hid [export options] <pcb file>\tto export", Progname);
  u ("Available export hid%s:", n_exporter == 1 ? "" : "s");
  for (i = 0; hl[i]; i++)
    if (hl[i]->exporter)
      fprintf (stderr, "\t%s\t%s\n", hl[i]->name, hl[i]->description);

  for (i = 0; hl[i]; i++)
    if (hl[i]->gui)
      usage_hid (hl[i]);
  for (i = 0; hl[i]; i++)
    if (hl[i]->printer)
      usage_hid (hl[i]);
  for (i = 0; hl[i]; i++)
    if (hl[i]->exporter)
      usage_hid (hl[i]);
  exit (1);
}

static void
print_defaults_1 (HID_Attribute * a, void *value)
{
  int i;
  double d;
  char *s;

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
      i = value ? *(int *) value : a->default_val.int_value;
      d = value ? *(double *) value : a->default_val.real_value;
      fprintf (stderr, "%s %g%s\n", a->name, d, a->enumerations[i]);
      break;
    case HID_Label:
      break;
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
	  e = h->get_export_options (&n);
	  if (e)
	    for (i = 0; i < n; i++)
	      print_defaults_1 (e + i, 0);
	}
    }
  exit (1);
}

/* in hid/common/actions.c */
extern void print_actions ();


#define SSET(F,D,N,H) { N, H, \
	HID_String,  0, 0, { 0, D, 0 }, 0, &Settings.F }
#define ISET(F,D,N,H) { N, H, \
	HID_Integer, 0, 0, { D, 0, 0 }, 0, &Settings.F }
#define BSET(F,D,N,H) { N, H, \
	HID_Boolean, 0, 0, { D, 0, 0 }, 0, &Settings.F }
#define RSET(F,D,N,H) { N, H, \
	HID_Real,    0, 0, { 0, 0, D }, 0, &Settings.F }

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

HID_Attribute main_attribute_list[] = {
  {"help", "Show Help", HID_Boolean, 0, 0, {0, 0, 0}, 0, &show_help},
  {"version", "Show Version", HID_Boolean, 0, 0, {0, 0, 0}, 0, &show_version},
  {"verbose", "Be verbose", HID_Boolean, 0, 0, {0, 0, 0}, 0,
   &Settings.verbose},
  {"copyright", "Show Copyright", HID_Boolean, 0, 0, {0, 0, 0}, 0,
   &show_copyright},
  {"show-defaults", "Show option defaults", HID_Boolean, 0, 0, {0, 0, 0}, 0,
   &show_defaults},
  {"show-actions", "Show actions", HID_Boolean, 0, 0, {0, 0, 0}, 0,
   &show_actions},
  {"dump-actions", "Dump actions (for documentation)", HID_Boolean, 0, 0,
   {0, 0, 0}, 0,
   &do_dump_actions},

  BSET (grid_units_mm, 0, "grid-units-mm", 0),

  COLOR (BlackColor, "#000000", "black-color", "color for black"),
  COLOR (WhiteColor, "#ffffff", "white-color", "color for white"),
  COLOR (BackgroundColor, "#e5e5e5", "background-color",
	 "color for background"),
  COLOR (CrosshairColor, "#ff0000", "crosshair-color",
	 "color for the crosshair"),
  COLOR (CrossColor, "#cdcd00", "cross-color", "color for cross"),
  COLOR (ViaColor, "#7f7f7f", "via-color", "color for vias"),
  COLOR (ViaSelectedColor, "#00ffff", "via-selected-color",
	 "color for selected vias"),
  COLOR (PinColor, "#4d4d4d", "pin-color", "color for pins"),
  COLOR (PinSelectedColor, "#00ffff", "pin-selected-color",
	 "color for selected pins"),
  COLOR (PinNameColor, "#ff0000", "pin-name-color",
	 "color for pin names and numbers"),
  COLOR (ElementColor, "#000000", "element-color", "color for elements"),
  COLOR (RatColor, "#b8860b", "rat-color", "color for ratlines"),
  COLOR (InvisibleObjectsColor, "#cccccc", "invisible-objects-color",
	 "color for invisible objects"),
  COLOR (InvisibleMarkColor, "#cccccc", "invisible-mark-color",
	 "color for invisible marks"),
  COLOR (ElementSelectedColor, "#00ffff", "element-selected-color",
	 "color for selected elements"),
  COLOR (RatSelectedColor, "#00ffff", "rat-selected-color",
	 "color for selected rats"),
  COLOR (ConnectedColor, "#00ff00", "connected-color",
	 "color for connections"),
  COLOR (OffLimitColor, "#cccccc", "off-limit-color",
	 "color for off-limits areas"),
  COLOR (GridColor, "#ff0000", "grid-color", "color for the grid"),
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
  COLOR (WarnColor, "#ff8000", "warn-color", "color for warnings"),
  COLOR (MaskColor, "#ff0000", "mask-color", "color for solder mask"),

  ISET (ViaThickness, 6000, "via-thickness", 0),
  ISET (ViaDrillingHole, 2800, "via-drilling-hole", 0),
  ISET (LineThickness, 1000, "line-thickness",
	"Initial thickness of new lines."),
  ISET (RatThickness, 1000, "rat-thickness", 0),
  ISET (Keepaway, 1000, "keepaway", 0),
  ISET (MaxWidth, 600000, "default-PCB-width", 0),
  ISET (MaxHeight, 500000, "default-PCB-height", 0),
  ISET (TextScale, 100, "text-scale", 0),
  ISET (AlignmentDistance, 200, "alignment-distance", 0),
  ISET (Bloat, 1000, "bloat", 0),
  ISET (Shrink, 1000, "shrink", 0),
  ISET (minWid, 1000, "min-width", "DRC minimum copper spacing"),
  ISET (minSlk, 1000, "min-silk", "DRC minimum silk width"),
  ISET (minDrill, 1500, "min-drill", "DRC minimum drill diameter"),
  ISET (minRing, 1000, "min-ring", "DRC minimum annular ring"),

  RSET (Grid, 1000, "grid", 0),
  RSET (grid_increment_mm, 0.1, "grid-increment-mm", 0),
  RSET (grid_increment_mil, 5.0, "grid-increment-mil", 0),
  RSET (size_increment_mm, 0.2, "size-increment-mm", 0),
  RSET (size_increment_mil, 10.0, "size-increment-mil", 0),
  RSET (line_increment_mm, 0.1, "line-increment-mm", 0),
  RSET (line_increment_mil, 5.0, "line-increment-mil", 0),
  RSET (clear_increment_mm, 0.05, "clear-increment-mm", 0),
  RSET (clear_increment_mil, 2.0, "clear-increment-mil", 0),
  RSET (IsleArea, 200000000, "minimum polygon area", 0),

  ISET (BackupInterval, 60, "backup-interval", 0),

  LAYERNAME (1, "component"),
  LAYERNAME (2, "solder"),
  LAYERNAME (3, "GND"),
  LAYERNAME (4, "power"),
  LAYERNAME (5, "signal1"),
  LAYERNAME (6, "signal2"),
  LAYERNAME (7, "signal3"),
  LAYERNAME (8, "signal4"),

  SSET (FontCommand, "",
	"font-command", 0),
  SSET (FileCommand, "", "file-command", "Command to read a file."),
  SSET (ElementCommand,
	"M4PATH='%p';export M4PATH;echo 'include(%f)' | " GNUM4,
	"element-command", 0),
  SSET (PrintFile, "%f.output", "print-file", 0),
  SSET (LibraryCommandDir, PCBLIBDIR, "lib-command-dir", 0),
  SSET (LibraryCommand, "QueryLibrary.sh '%p' '%f' %a",
	"lib-command", 0),
  SSET (LibraryContentsCommand, "ListLibraryContents.sh '%p' '%f'",
	"lib-contents-command", 0),
  SSET (LibraryTree, PCBTREEPATH, "lib-newlib",
	"Top level directory for the newlib style library"),
  SSET (SaveCommand, "", "save-command", 0),
  SSET (LibraryFilename, LIBRARYFILENAME, "lib-name", 0),
  SSET (FontFile, "default_font", "default-font",
	"File name of default font."),
  SSET (Groups, "1,c:2,s:3:4:5:6:7:8", "groups", 0),
  SSET (Routes, "Signal,1000,3600,2000,1000:Power,2500,6000,3500,1000"
	":Fat,4000,6000,3500,1000:Skinny,600,2402,1181,600", "route-styles",
	0),
  SSET (FilePath, "", "file-path", 0),
  SSET (RatCommand, "", "rat-command", 0),
  SSET (FontPath, PCBLIBPATH, "font-path", 0),
  SSET (ElementPath, PCBLIBPATH, "element-path", 0),
  SSET (LibraryPath, PCBLIBPATH, "lib-path", 0),
  SSET (MenuFile, "pcb-menu.res", "menu-file", 0),
  SSET (ScriptFilename, 0, "action-script",
	"If set, this file is executed at startup."),
  SSET (ActionString, 0, "action-string",
	"If set, this is executed at startup."),
  SSET (FabAuthor, "", "fab-author", 0),
  SSET (InitialLayerStack, "", "layer-stack",
	"Initial layer stackup, for setting up an export."),

  ISET (PinoutOffsetX, 100, "pinout-offset-x", 0),
  ISET (PinoutOffsetY, 100, "pinout-offset-y", 0),
  ISET (PinoutTextOffsetX, 800, "pinout-text-offset-x", 0),
  ISET (PinoutTextOffsetY, -100, "pinout-text-offset-y", 0),

  BSET (DrawGrid, 0, "draw-grid", "default to drawing the grid at startup"),
  BSET (ClearLine, 1, "clear-line", 0),
  BSET (FullPoly, 0, "full-poly", 0),
  BSET (UniqueNames, 1, "unique-names", 0),
  BSET (SnapPin, 1, "snap-pin", 0),
  BSET (SaveLastCommand, 0, "save-last-command", 0),
  BSET (SaveInTMP, 0, "save-in-tmp", 0),
  BSET (AllDirectionLines, 0, "all-direction-lines", 0),
  BSET (ShowNumber, 0, "show-number", 0),
  BSET (ResetAfterElement, 1, "reset-after-element", 0),
  BSET (RingBellWhenFinished, 0, "ring-bell-finished", 0),
};

REGISTER_ATTRIBUTES (main_attribute_list)
/* ---------------------------------------------------------------------- 
 * post-process settings.
 */
     static void settings_post_process ()
{
  if (Settings.LibraryCommand[0] != PCB_DIR_SEPARATOR_C && Settings.LibraryCommand[0] != '.')
    {
      Settings.LibraryCommand
	=
	Concat (Settings.LibraryCommandDir, PCB_DIR_SEPARATOR_S, 
		Settings.LibraryCommand,
		NULL);
    }
  if (Settings.LibraryContentsCommand[0] != PCB_DIR_SEPARATOR_C
      && Settings.LibraryContentsCommand[0] != '.')
    {
      Settings.LibraryContentsCommand
	=
	Concat (Settings.LibraryCommandDir, PCB_DIR_SEPARATOR_S,
		Settings.LibraryContentsCommand, NULL);
    }

  if (Settings.LineThickness > MAX_LINESIZE
      || Settings.LineThickness < MIN_LINESIZE)
    Settings.LineThickness = 1000;

  if (Settings.ViaThickness > MAX_PINORVIASIZE
      || Settings.ViaThickness < MIN_PINORVIASIZE)
    Settings.ViaThickness = 4000;

  if (Settings.ViaDrillingHole <= 0)
    Settings.ViaDrillingHole =
      DEFAULT_DRILLINGHOLE * Settings.ViaThickness / 100;

  Settings.MaxWidth = MIN (MAX_COORD, MAX (Settings.MaxWidth, MIN_SIZE));
  Settings.MaxHeight = MIN (MAX_COORD, MAX (Settings.MaxHeight, MIN_SIZE));

  ParseRouteString (Settings.Routes, &Settings.RouteStyle[0], 1);

}

/* ---------------------------------------------------------------------- 
 * Print help or version messages.
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

static void
InitPaths (char *argv0)
{
  size_t l;
  int i;
  int haspath;
  char *t1, *t2;
  int found_bindir = 0;

  /* see if argv0 has enough of a path to let lrealpath give the
   * real path.  This should be the case if you invoke pcb with
   * something like /usr/local/bin/pcb or ./pcb or ./foo/pcb
   * but if you just use pcb and it exists in your path, you'll
   * just get back pcb again.
   */

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
	      if ( (tmps = malloc ( (strlen (argv0) + strlen (p) + 2) * sizeof (char))) == NULL )
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

int
main (int argc, char *argv[])
{
  int i;

  /* init application:
   * - make program name available for error handlers
   * - evaluate special options
   * - initialize toplevel shell and resources
   * - create an empty PCB with default symbols
   * - initialize all other widgets
   * - update screen and get size of drawing area
   * - evaluate command-line arguments
   * - register 'call on exit()' function
   */

#include "core_lists.h"
  setbuf (stdout, 0);
  InitPaths (argv[0]);

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

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
      program_directory = ".";
      program_basename = program_name;
    }
  Progname = program_basename;
  
  if (argc > 1 && strcmp (argv[1], "-h") == 0)
    usage ();
  if (argc > 1 && strcmp (argv[1], "-V") == 0)
    print_version ();
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
  else
    gui = hid_find_gui ();

  if (!gui)
    exit (1);

  for (i = 0; i < MAX_LAYER; i++)
    {
      char buf[20];
      sprintf (buf, "signal%d", i + 1);
      Settings.DefaultLayerName[i] = MyStrdup (buf, "DefaultLayerNames");
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

  Output.bgGC = gui->make_gc ();
  Output.fgGC = gui->make_gc ();
  Output.pmGC = gui->make_gc ();
  Output.GridGC = gui->make_gc ();

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

  PCB = CreateNewPCB (True);
  PCB->Data->LayerN = DEF_LAYER;
  ParseGroupString (Settings.Groups, &PCB->LayerGroups, DEF_LAYER);
  CreateNewPCBPost (PCB, 1);
  if (argc > 1)
    command_line_pcb = argv[1];

  ResetStackAndVisibility ();

  CreateDefaultFont ();
  if (gui->gui)
    InitCrosshair ();
  InitHandler ();
  InitBuffers ();
  SetMode (ARROW_MODE);

  if (command_line_pcb)
    {
      /* keep filename even if initial load command failed;
       * file might not exist
       */
      if (LoadPCB (command_line_pcb))
	PCB->Filename = MyStrdup (command_line_pcb, "main()");
    }

  if (Settings.InitialLayerStack
      && Settings.InitialLayerStack[0])
    {
      LayerStringToLayerStack (Settings.InitialLayerStack);
    }

  if (gui->printer || gui->exporter)
    {
      gui->do_export (0);
      exit (0);
    }

  /*    FIX_ME
     LoadBackgroundImage (Settings.BackgroundImage); */

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
  /*
   * Set this flag to zero.  Then if we have a startup
   * action which includes Quit(), the flag will be set
   * to -1 and we can avoid ever calling gtk_main();
   */
  Settings.init_done = 0;

  if (Settings.ScriptFilename)
    {
      Message (_("Executing startup script file %s\n"),
	       Settings.ScriptFilename);
      hid_actionl ("ExecuteFile", Settings.ScriptFilename, NULL);
    }
  if (Settings.ActionString)
    {
      Message (_("Executing startup action %s\n"), Settings.ActionString);
      hid_parse_actions (Settings.ActionString, 0);
    }

  if (Settings.init_done == 0)
    {
      Settings.init_done = 1;
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
#endif

      gui->do_export (0);
#if HAVE_DBUS
      pcb_dbus_finish();
#endif
    }

  return (0);
}

