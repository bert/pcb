/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/* This file written by Bill Wilson for the PCB Gtk port.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "gui.h"
#include "hid.h"
#include "../hidint.h"
#include "gtkhid.h"

#include "global.h"
#include "action.h"
#include "change.h"
#include "file.h"
#include "font.h"
#include "error.h"
#include "draw.h"
#include "misc.h" /* MKDIR() */
#include "pcb-printf.h"
#include "set.h"

#if 0
#include <locale.h>
#endif
#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

extern int	MoveLayerAction(int argc, char **argv, int x, int y);
/* This is defined in main.c */
void save_increments (const Increments *mm, const Increments *mil);

enum ConfigType
{
  CONFIG_Boolean,
  CONFIG_Integer,
  CONFIG_Coord,
  CONFIG_Real,
  CONFIG_String,
  CONFIG_Unused
};

typedef struct
{
  gchar *name;
  enum ConfigType type;
  void *value;
}
ConfigAttribute;


enum ColorTypes
{
  MISC_COLOR,
  MISC_SELECTED_COLOR,
  LAYER_COLOR,
  LAYER_SELECTED_COLOR
};

typedef struct
{
  HID_Attribute *attributes;
  enum ColorTypes type;
  GdkColor color;
  gboolean color_is_mapped;
}
ConfigColor;

static GList *config_color_list, *lib_newlib_list;

static gchar *lib_newlib_config, *board_size_override;


static gchar *color_file;

extern void ghid_set_special_colors (HID_Attribute * ha);


#define PCB_CONFIG_DIR	".pcb"
#define PCB_CONFIG_FILE	"preferences"
#define PCB_COLORS_DIR	"colors"

static gchar *config_dir, *color_dir;

  /* CONFIG_Unused types are expected to be found in main_attribute_list and
     |  will be assigned the type found there.  NULL value pointers here are
     |  also expected to be assigned values from the main_attribute_list.
   */
  /* PinoutFont also not used anymore */

static ConfigAttribute config_attributes[] = {
  {"gui-compact-horizontal", CONFIG_Boolean, &_ghidgui.compact_horizontal},
  {"gui-compact-vertical", CONFIG_Boolean, &_ghidgui.compact_vertical},
  {"use-command-window", CONFIG_Boolean, &_ghidgui.use_command_window},
  {"save-in-tmp", CONFIG_Unused, NULL},
  {"save-metric-only", CONFIG_Unused, NULL},
  {"grid-units", CONFIG_Unused, NULL},
  {"grid", CONFIG_Unused, NULL},

  {"grid-increment-mm", CONFIG_Unused, NULL},
  {"line-increment-mm", CONFIG_Unused, NULL},
  {"size-increment-mm", CONFIG_Unused, NULL},
  {"clear-increment-mm", CONFIG_Unused, NULL},
  {"grid-increment-mil", CONFIG_Unused, NULL},
  {"line-increment-mil", CONFIG_Unused, NULL},
  {"size-increment-mil", CONFIG_Unused, NULL},
  {"clear-increment-mil", CONFIG_Unused, NULL},

  {"history-size", CONFIG_Integer, &_ghidgui.history_size},
  {"top-window-width", CONFIG_Integer, &_ghidgui.top_window_width},
  {"top-window-height", CONFIG_Integer, &_ghidgui.top_window_height},
  {"log-window-width", CONFIG_Integer, &_ghidgui.log_window_width},
  {"log-window-height", CONFIG_Integer, &_ghidgui.log_window_height},
  {"drc-window-width", CONFIG_Integer, &_ghidgui.drc_window_width},
  {"drc-window-height", CONFIG_Integer, &_ghidgui.drc_window_height},
  {"library-window-width", CONFIG_Integer, &_ghidgui.library_window_width},
  {"library-window-height", CONFIG_Integer, &_ghidgui.library_window_height},
  {"netlist-window-width", CONFIG_Integer, &_ghidgui.netlist_window_width},
  {"netlist-window-height", CONFIG_Integer, &_ghidgui.netlist_window_height},
  {"keyref-window-width", CONFIG_Integer, &_ghidgui.keyref_window_width},
  {"keyref-window-height", CONFIG_Integer, &_ghidgui.keyref_window_height},
  {"text-scale", CONFIG_Unused, NULL},
  {"via-thickness", CONFIG_Unused, NULL},
  {"via-drilling-hole", CONFIG_Unused, NULL},
  {"backup-interval", CONFIG_Unused, NULL},
  {"line-thickness", CONFIG_Unused, NULL},
  {"rat-thickness", CONFIG_Unused, NULL},
  {"bloat", CONFIG_Unused, NULL},
  {"shrink", CONFIG_Unused, NULL},
  {"min-width", CONFIG_Unused, NULL},
  {"min-silk", CONFIG_Unused, NULL},
  {"min-drill", CONFIG_Unused, NULL},
  {"min-ring", CONFIG_Unused, NULL},
  {"default-PCB-width", CONFIG_Unused, NULL},
  {"default-PCB-height", CONFIG_Unused, NULL},

  {"groups", CONFIG_Unused, NULL},
  {"route-styles", CONFIG_Unused, NULL},
  {"library-newlib", CONFIG_String, &lib_newlib_config},
  {"color-file", CONFIG_String, &color_file},
  {"save-font-data", CONFIG_Unused, NULL},
  {"save-symbols", CONFIG_Unused, NULL},
  {"embed-fonts", CONFIG_Unused, NULL},
/* FIXME: construct layer-names- in a list */
  {"layer-name-1", CONFIG_Unused, NULL},
  {"layer-name-2", CONFIG_Unused, NULL},
  {"layer-name-3", CONFIG_Unused, NULL},
  {"layer-name-4", CONFIG_Unused, NULL},
  {"layer-name-5", CONFIG_Unused, NULL},
  {"layer-name-6", CONFIG_Unused, NULL},
  {"layer-name-7", CONFIG_Unused, NULL},
  {"layer-name-8", CONFIG_Unused, NULL},
};

static gboolean
dup_core_string (gchar ** dst, const gchar * src)
{
  if (dst == NULL || (*dst == NULL && src == NULL))
    return FALSE;

  if (*dst != NULL && src != NULL && strcmp (*dst, src) == 0)
    return FALSE;

  free (*dst);
  *dst = (src == NULL) ? NULL : strdup (src);

  return TRUE;
}

static FILE *
config_file_open (gchar * mode)
{
  FILE *f;
  gchar *homedir, *fname;


  homedir = (gchar *) g_get_home_dir ();
  if (!homedir)
    {
      g_message ("config_file_open: Can't get home directory!");
      return NULL;
    }

  if (!config_dir)
    {
      config_dir =
	g_build_path (G_DIR_SEPARATOR_S, homedir, PCB_CONFIG_DIR, NULL);
      if (!g_file_test (config_dir, G_FILE_TEST_IS_DIR)
	  && MKDIR (config_dir, 0755) < 0)
	{
	  g_message ("config_file_open: Can't make \"%s\" directory!",
		     config_dir);
	  g_free (config_dir);
	  config_dir = NULL;
	  return NULL;
	}
    }

  if (!color_dir)		/* Convenient to make the color dir here */
    {
      color_dir =
	g_build_path (G_DIR_SEPARATOR_S, config_dir, PCB_COLORS_DIR, NULL);
      if (!g_file_test (color_dir, G_FILE_TEST_IS_DIR))
	{
	  if (MKDIR (color_dir, 0755) < 0)
	    {
	      g_message ("config_file_open: Can't make \"%s\" directory!",
			 color_dir);
	      g_free (color_dir);
	      color_dir = NULL;
	    }
	  fname = g_build_path (G_DIR_SEPARATOR_S,
				color_dir, "Default", NULL);
	  dup_string (&color_file, fname);
	  g_free (fname);
	}
    }

  fname = g_build_path (G_DIR_SEPARATOR_S, config_dir, PCB_CONFIG_FILE, NULL);
  f = fopen (fname, mode);

  g_free (fname);
  return f;
}

static ConfigAttribute *
lookup_config_attribute (gchar * name, gboolean if_null_value)
{
  ConfigAttribute *ca;

  for (ca = &config_attributes[0];
       ca < &config_attributes[0] + G_N_ELEMENTS (config_attributes); ++ca)
    {
      if (name && (!strcmp (name, ca->name)))
	{
	  if (ca->value && if_null_value)
	    break;
	  return ca;
	}
    }
  return NULL;
}

void
ghid_config_init (void)
{
  HID_AttrNode *ha;
  HID_Attribute *a;
  ConfigAttribute *ca, dummy_attribute;
  ConfigColor *cc;
  gint len;

  ghidgui->n_mode_button_columns = 3;
  ghidgui->small_label_markup = TRUE;
  ghidgui->history_size = 5;
  dup_string (&color_file, "");

  for (ha = hid_attr_nodes; ha; ha = ha->next)
    {
      for (a = ha->attributes; a < ha->attributes + ha->n; ++a)
	{
	  if (!a->value)
	    continue;
	  if ((ca = lookup_config_attribute (a->name, TRUE)) == NULL)
	    ca = &dummy_attribute;
	  ca->value = a->value;	/* Typically &Setting.xxx */
	  ca->type = CONFIG_Unused;
	  switch (a->type)
	    {
	    case HID_Boolean:
	      *(char *) a->value = a->default_val.int_value;
	      ca->type = CONFIG_Boolean;
	      break;
	    case HID_Integer:
	      *(int *) a->value = a->default_val.int_value;
	      ca->type = CONFIG_Integer;
	      break;
	    case HID_Coord:
	      *(Coord *) a->value = a->default_val.coord_value;
	      ca->type = CONFIG_Coord;
	      break;
	    case HID_Real:
	      *(double *) a->value = a->default_val.real_value;
	      ca->type = CONFIG_Real;
	      break;

	    case HID_String:
	      if (!a->name)
		break;
	      *(char **) a->value = g_strdup (a->default_val.str_value);
	      ca->type = CONFIG_String;

	      len = strlen (a->name);
	      if (len < 7 || strstr (a->name, "color") == NULL)
		break;

	      cc = g_new0 (ConfigColor, 1);
	      cc->attributes = a;

	      if (!strncmp (a->name, "layer-color", 11))
		cc->type = LAYER_COLOR;
	      else if (!strncmp (a->name, "layer-selected-color", 20))
		cc->type = LAYER_SELECTED_COLOR;
	      else if (!strncmp (a->name + len - 14, "selected-color", 14))
		cc->type = MISC_SELECTED_COLOR;
	      else
		cc->type = MISC_COLOR;

	      config_color_list = g_list_append (config_color_list, cc);
	      break;

	    case HID_Enum:
	    case HID_Unit:
	      *(int *) a->value = a->default_val.int_value;
	      break;

	    case HID_Label:
	    case HID_Mixed:
	    case HID_Path:
	      break;
	    default:
	      abort ();
	    }
	  }
  }
}

static gint
parse_option_line (gchar * line, gchar ** option_result, gchar ** arg_result)
{
  gchar *s, *ss, *option = NULL, *arg = NULL;
  gint argc = 1;

  if (option_result)
    *option_result = NULL;
  if (arg_result)
    *arg_result = NULL;

  s = line;
  while (*s == ' ' || *s == '\t')
    ++s;
  if (!*s || *s == '\n' || *s == '#' || *s == '[')
    return 0;
  if ((ss = strchr (s, '\n')) != NULL)
    *ss = '\0';

  if ((ss = strchr (s, ' ')) != NULL) {
    option = malloc(ss - s + 1);
    if (option == NULL)
      return 0;
    memcpy(option, s, ss - s);
    option[ss - s] = '\0';

    arg = strdup(ss + 1);
    if (arg == NULL) {
      free(option);
      return 0;
    }
  } else {
    option = strdup(s);
    if (option == NULL)
      return 0;

    arg = strdup("");
    if (arg == NULL) {
      free(option);
      return 0;
    }
  }

  s = option;			/* Strip trailing ':' or '=' */
  while (*s && *s != ':' && *s != '=')
    ++s;
  *s = '\0';

  s = arg;			/* Strip leading ':', '=', and whitespace */
  while (*s == ' ' || *s == '\t' || *s == ':' || *s == '=' || *s == '"')
    ++s;
  if ((ss = strchr (s, '"')) != NULL)
    *ss = '\0';

  if (option_result)
    *option_result = g_strdup (option);
  if (arg_result && *s)
    {
      *arg_result = g_strdup (s);
      ++argc;
    }

  free(arg);
  free(option);
  return argc;
}

static gboolean
set_config_attribute (gchar * option, gchar * arg)
{
  ConfigAttribute *ca;
#if 0
  struct lconv *lc;
  gchar locale_point, *comma_point, *period_point;

  /* Until LC_NUMERIC is totally resolved, check if we need to decimal
     |  point convert.  Ultimately, data files will be POSIX and gui
     |  presentation (hence the config file reals) will be in the users locale.
   */
  lc = localeconv ();
  locale_point = *lc->decimal_point;
#endif

  if ((ca = lookup_config_attribute (option, FALSE)) == NULL)
    return FALSE;
  switch (ca->type)
    {
    case CONFIG_Boolean:
      *(gchar *) ca->value = (gchar) atoi (arg);
      break;

    case CONFIG_Integer:
      *(gint *) ca->value = atoi (arg);
      break;

    case CONFIG_Real:
      /* Hopefully temporary locale decimal point check:
       */
#if 0
      comma_point = strrchr (arg, ',');
      period_point = strrchr (arg, '.');
      if (comma_point && *comma_point != locale_point)
	*comma_point = locale_point;
      else if (period_point && *period_point != locale_point)
	*period_point = locale_point;
#endif
      *(double *) ca->value = atof (arg);
      break;

    case CONFIG_String:
      dup_string ((gchar **) ca->value, arg ? arg : (gchar *)"");
      break;
    case CONFIG_Coord:
      *(Coord *) ca->value = GetValue (arg, NULL, NULL);
      break;
    default:
      break;
    }
  return TRUE;
}

static void
config_file_read (void)
{
  FILE *f;
  char *buf = NULL;
  size_t n = 0;
  gchar *option, *arg;

  if ((f = config_file_open ("r")) == NULL)
    return;

  while (getline (&buf, &n, f) != -1)
    {
      if (parse_option_line (buf, &option, &arg) > 0)
	set_config_attribute (option, arg);
      g_free (option);
      g_free (arg);
    }

  free (buf);
  fclose (f);
}

static void
config_colors_write (gchar * path)
{
  FILE *f;
  GList *list;
  HID_Attribute *ha;
  ConfigColor *cc;

  if ((f = fopen (path, "w")) == NULL)
    return;
  for (list = config_color_list; list; list = list->next)
    {
      cc = (ConfigColor *) list->data;
      ha = cc->attributes;
      fprintf (f, "%s =\t%s\n", ha->name, *(char **) ha->value);
    }
  fclose (f);
}

static gboolean
config_colors_read (gchar * path)
{
  FILE *f;
  GList *list;
  ConfigColor *cc;
  HID_Attribute *ha;
  gchar *s, buf[512], option[64], arg[512];

  if (!path || !*path || (f = fopen (path, "r")) == NULL)
    return FALSE;

  while (fgets (buf, sizeof (buf), f))
    {
      sscanf (buf, "%63s %511[^\n]", option, arg);
      s = option;		/* Strip trailing ':' or '=' */
      while (*s && *s != ':' && *s != '=')
	++s;
      *s = '\0';
      s = arg;			/* Strip leading ':', '=', and whitespace */
      while (*s == ' ' || *s == '\t' || *s == ':' || *s == '=')
	++s;

      for (list = config_color_list; list; list = list->next)
	{
	  cc = (ConfigColor *) list->data;
	  ha = cc->attributes;
	  if (!strcmp (option, ha->name))
	    {
	      *(char **) ha->value = g_strdup (s);
	      cc->color_is_mapped = FALSE;
	      ghid_set_special_colors (ha);
	      break;
	    }
	}
    }
  fclose (f);

  return TRUE;
}

static gchar *
expand_dir (gchar * dir)
{
  gchar *s;

  if (*dir == '~')
    s = g_build_filename ((gchar *) g_get_home_dir (), dir + 1, NULL);
  else
    s = g_strdup (dir);
  return s;
}

static void
add_to_paths_list (GList ** list, gchar * path_string)
{
  gchar *p, *paths;

  paths = g_strdup (path_string);
  for (p = strtok (paths, PCB_PATH_DELIMETER); p && *p; p = strtok (NULL, PCB_PATH_DELIMETER))
    *list = g_list_prepend (*list, expand_dir (p));
  g_free (paths);
}

  /* Parse command line code borrowed from hid/common/hidinit.c
   */
static void
parse_optionv (gint * argc, gchar *** argv, gboolean from_cmd_line)
{
  HID_AttrNode *ha;
  HID_Attribute *a;
  const Unit *unit;
  gchar *ep;
  gint e, ok, offset;
  gboolean matched = FALSE;

  offset = from_cmd_line ? 2 : 0;

  while (*argc
	 && (((*argv)[0][0] == '-' && (*argv)[0][1] == '-')
	     || !from_cmd_line))
    {
      for (ha = hid_attr_nodes; ha; ha = ha->next)
	{
	  for (a = ha->attributes; a < ha->attributes + ha->n; ++a)
	    {
	      if (!a->name || strcmp ((*argv)[0] + offset, a->name))
		continue;
	      switch (a->type)
		{
		case HID_Label:
		  break;
		case HID_Integer:
		  if (a->value)
		    *(int *) a->value = strtol ((*argv)[1], 0, 0);
		  else
		    a->default_val.int_value = strtol ((*argv)[1], 0, 0);
		  (*argc)--;
		  (*argv)++;
		  break;
		case HID_Coord:
		  if (a->value)
		    *(Coord *) a->value = GetValue ((*argv)[1], 0, 0);
		  else
		    a->default_val.coord_value = GetValue ((*argv)[1], 0, 0);
		  (*argc)--;
		  (*argv)++;
                  break;
		case HID_Real:
		  if (a->value)
		    *(double *) a->value = strtod ((*argv)[1], 0);
		  else
		    a->default_val.real_value = strtod ((*argv)[1], 0);
		  (*argc)--;
		  (*argv)++;
		  break;
		case HID_String:
		  if (a->value)
		    *(char **) a->value = g_strdup((*argv)[1]);
		  else
		    a->default_val.str_value = g_strdup((*argv)[1]);
		  (*argc)--;
		  (*argv)++;
		  break;
		case HID_Boolean:
		  if (a->value)
		    *(char *) a->value = 1;
		  else
		    a->default_val.int_value = 1;
		  break;
		case HID_Mixed:
		  a->default_val.real_value = strtod ((*argv)[1], &ep);
		  goto do_enum;
		case HID_Enum:
		  ep = (*argv)[1];
		do_enum:
		  ok = 0;
		  for (e = 0; a->enumerations[e]; e++)
		    if (strcmp (a->enumerations[e], ep) == 0)
		      {
			ok = 1;
			a->default_val.int_value = e;
			a->default_val.str_value = ep;
			break;
		      }
		  if (!ok)
		    {
		      fprintf (stderr,
			       "ERROR:  \"%s\" is an unknown value for the --%s option\n",
			       (*argv)[1], a->name);
		      exit (1);
		    }
		  (*argc)--;
		  (*argv)++;
		  break;
		case HID_Path:
		  abort ();
		  a->default_val.str_value = (*argv)[1];
		  (*argc)--;
		  (*argv)++;
		  break;
	        case HID_Unit:
                  unit = get_unit_struct ((*argv)[1]);
                  if (unit == NULL)
		    {
		      fprintf (stderr,
		               "ERROR:  unit \"%s\" is unknown to pcb (option --%s)\n",
			       (*argv)[1], a->name);
		      exit (1);
		    }
	          a->default_val.int_value = unit->index;
	          a->default_val.str_value = unit->suffix;
		  (*argc)--;
		  (*argv)++;
	          break;
		}
	      (*argc)--;
	      (*argv)++;
	      ha = 0;
	      goto got_match;
	    }
	  if (a < ha->attributes + ha->n)
	    matched = TRUE;
	}
      if (!matched)
	{
	  if (from_cmd_line)
	    {
	      fprintf (stderr, "unrecognized option: %s\n", (*argv)[0]);
	      exit (1);
	    }
	  else
//                              ghid_log("unrecognized option: %s\n", (*argv)[0]);
	    fprintf (stderr, "unrecognized option: %s\n", (*argv)[0]);
	}
    got_match:;
    }
  (*argc)++;
  (*argv)--;
}

static void
load_rc_file (gchar * path)
{
  FILE *f;
  gchar buf[1024], *av[2], **argv;
  gint argc;

  f = fopen (path, "r");
  if (!f)
    return;

  if (Settings.verbose)
    printf ("Loading pcbrc file: %s\n", path);
  while (fgets (buf, sizeof (buf), f))
    {
      argv = &(av[0]);
      if ((argc = parse_option_line (buf, &av[0], &av[1])) > 0)
	parse_optionv (&argc, &argv, FALSE);
      g_free (av[0]);
      g_free (av[1]);
    }
  fclose (f);
}

static void
load_rc_files (void)
{
  gchar *path;

  load_rc_file ("/etc/pcbrc");
  load_rc_file ("/usr/local/etc/pcbrc");

  path = g_build_filename (pcblibdir, "pcbrc", NULL);
  load_rc_file (path);
  g_free (path);

  path = g_build_filename ((gchar *) g_get_home_dir (), ".pcb/pcbrc", NULL);
  load_rc_file (path);
  g_free (path);

  load_rc_file ("pcbrc");
}


void
ghid_config_files_read (gint * argc, gchar *** argv)
{
  GList *list;
  gchar *str, *dir;
  gint width, height;

  ghidgui = &_ghidgui;

  ghid_config_init ();
  load_rc_files ();
  config_file_read ();
  config_colors_read (color_file);
  (*argc)--;
  (*argv)++;
  parse_optionv (argc, argv, TRUE);

  if (board_size_override
      && sscanf (board_size_override, "%dx%d", &width, &height) == 2)
    {
      Settings.MaxWidth = TO_PCB_UNITS (width);
      Settings.MaxHeight = TO_PCB_UNITS (height);
    }

  if (lib_newlib_config && *lib_newlib_config)
    add_to_paths_list (&lib_newlib_list, lib_newlib_config);

  for (list = lib_newlib_list; list; list = list->next)
    {
      str = Settings.LibraryTree;
      dir = expand_dir ((gchar *) list->data);
      Settings.LibraryTree = g_strconcat (str, PCB_PATH_DELIMETER, dir, NULL);
      g_free (dir);
      g_free (str);
    }
}

void
ghid_config_files_write (void)
{
  FILE *f;
  ConfigAttribute *ca;

  if (!ghidgui->config_modified || (f = config_file_open ("w")) == NULL)
    return;

  fprintf (f, "### PCB configuration file. ###\n");

  for (ca = &config_attributes[0];
       ca < &config_attributes[0] + G_N_ELEMENTS (config_attributes); ++ca)
    {
      switch (ca->type)
	{
	case CONFIG_Boolean:
	  fprintf (f, "%s = %d\n", ca->name, (gint) * (gchar *) ca->value);
	  break;

	case CONFIG_Integer:
	  fprintf (f, "%s = %d\n", ca->name, *(gint *) ca->value);
	  break;

	case CONFIG_Real:
	  fprintf (f, "%s = %f\n", ca->name, *(double *) ca->value);
	  break;

	case CONFIG_Coord:
	  pcb_fprintf (f, "%s = %$mS\n", ca->name, *(Coord *) ca->value);
	  break;

	case CONFIG_String:
	  if (*(char **) ca->value == NULL)
	    fprintf (f, "# %s = NULL\n", ca->name);
	  else
	    fprintf (f, "%s = %s\n", ca->name, *(char **) ca->value);
	  break;
	default:
	  break;
	}
    }
  fclose (f);

  ghidgui->config_modified = FALSE;
}

/* =================== OK, now the gui stuff ======================
*/
static GtkWidget *config_window;

  /* -------------- The General config page ----------------
   */

static void
config_command_window_toggle_cb (GtkToggleButton * button, gpointer data)
{
  gboolean active = gtk_toggle_button_get_active (button);
  static gboolean holdoff;

  if (holdoff)
    return;

  /* Can't toggle into command window mode if the status line command
     |  entry is active.
   */
  if (ghidgui->command_entry_status_line_active)
    {
      holdoff = TRUE;
      gtk_toggle_button_set_active (button, FALSE);
      holdoff = FALSE;
      return;
    }
  ghidgui->use_command_window = active;
  ghid_command_use_command_window_sync ();
}


static void
config_compact_horizontal_toggle_cb (GtkToggleButton * button, gpointer data)
{
  gboolean active = gtk_toggle_button_get_active (button);

  ghidgui->compact_horizontal = active;
  ghid_set_status_line_label ();
  ghidgui->config_modified = TRUE;
}

static void
config_compact_vertical_toggle_cb (GtkToggleButton * button, gpointer data)
{
  gboolean active = gtk_toggle_button_get_active (button);

  ghidgui->compact_vertical = active;
  ghid_pack_mode_buttons();
  ghidgui->config_modified = TRUE;
}

static void
config_general_toggle_cb (GtkToggleButton * button, void * setting)
{
  *(gint *)setting = gtk_toggle_button_get_active (button);
  ghidgui->config_modified = TRUE;
}

static void
config_backup_spin_button_cb (GtkSpinButton * spin_button, gpointer data)
{
  Settings.BackupInterval = gtk_spin_button_get_value_as_int (spin_button);
  EnableAutosave ();
  ghidgui->config_modified = TRUE;
}

static void
config_history_spin_button_cb (GtkSpinButton * spin_button, gpointer data)
{
  ghidgui->history_size = gtk_spin_button_get_value_as_int (spin_button);
  ghidgui->config_modified = TRUE;
}

static void
config_general_tab_create (GtkWidget * tab_vbox)
{
  GtkWidget *vbox;

  gtk_container_set_border_width (GTK_CONTAINER (tab_vbox), 6);

  vbox = ghid_category_vbox (tab_vbox, _("Enables"), 4, 2, TRUE, TRUE);

  ghid_check_button_connected (vbox, NULL, ghidgui->use_command_window,
			       TRUE, FALSE, FALSE, 2,
			       config_command_window_toggle_cb, NULL,
			       _("Use separate window for command entry"));

  ghid_check_button_connected (vbox, NULL, ghidgui->compact_horizontal,
			       TRUE, FALSE, FALSE, 2,
			       config_compact_horizontal_toggle_cb, NULL,
			       _("Alternate window layout to allow smaller horizontal size"));

  ghid_check_button_connected (vbox, NULL, ghidgui->compact_vertical,
			       TRUE, FALSE, FALSE, 2,
			       config_compact_vertical_toggle_cb, NULL,
			       _("Alternate window layout to allow smaller vertical size"));

  vbox = ghid_category_vbox (tab_vbox, _("Backups"), 4, 2, TRUE, TRUE);
  ghid_check_button_connected (vbox, NULL, Settings.SaveInTMP,
			       TRUE, FALSE, FALSE, 2,
			       config_general_toggle_cb, &Settings.SaveInTMP,
			       _("If layout is modified at exit, save into PCB.%i.save"));
  ghid_spin_button (vbox, NULL, Settings.BackupInterval, 0.0, 60 * 60, 60.0,
		    600.0, 0, 0, config_backup_spin_button_cb, NULL, FALSE,
		    _("Seconds between auto backups\n"
		      "(set to zero to disable auto backups)"));

  vbox = ghid_category_vbox (tab_vbox, _("Misc"), 4, 2, TRUE, TRUE);
  ghid_spin_button (vbox, NULL, ghidgui->history_size,
		    5.0, 25.0, 1.0, 1.0, 0, 0,
		    config_history_spin_button_cb, NULL, FALSE,
		    _("Number of commands to remember in the history list"));

  ghid_check_button_connected (vbox, NULL, Settings.SaveMetricOnly,
			       TRUE, FALSE, FALSE, 2,
			       config_general_toggle_cb, &Settings.SaveMetricOnly,
			       _("Use only metric units when saving pcb files"));
}


static void
config_general_apply (void)
{
  /* save the settings */
  ghid_config_files_write ();
}


  /* -------------- The Sizes config page ----------------
   */

static GtkWidget *config_sizes_vbox,
  *config_sizes_tab_vbox, *config_text_spin_button;

static GtkWidget *use_board_size_default_button,
  *use_drc_sizes_default_button,
  *use_increments_default_button;

static Coord new_board_width, new_board_height;

static void
config_sizes_apply (void)
{
  gboolean active;

  active =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
				  (use_board_size_default_button));
  if (active)
    {
      Settings.MaxWidth = new_board_width;
      Settings.MaxHeight = new_board_height;
      ghidgui->config_modified = TRUE;
    }

  active =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
				  (use_drc_sizes_default_button));
  if (active)
    {
      Settings.Bloat = PCB->Bloat;
      Settings.Shrink = PCB->Shrink;
      Settings.minWid = PCB->minWid;
      Settings.minSlk = PCB->minSlk;
      Settings.IsleArea = PCB->IsleArea;
      Settings.minDrill = PCB->minDrill;
      Settings.minRing = PCB->minRing;
      ghidgui->config_modified = TRUE;
    }

  active =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
				  (use_increments_default_button));
  if (active)
    {
      save_increments (get_increments_struct (METRIC),
                       get_increments_struct (IMPERIAL));
      ghidgui->config_modified = TRUE;
    }

  if (PCB->MaxWidth != new_board_width || PCB->MaxHeight != new_board_height)
    ChangePCBSize (new_board_width, new_board_height);
}

static void
text_spin_button_cb (GtkSpinButton * spin, void * dst)
{
  *(gint *)dst = gtk_spin_button_get_value_as_int (spin);
  ghidgui->config_modified = TRUE;
  ghid_set_status_line_label ();
}

static void
coord_entry_cb (GHidCoordEntry * ce, void * dst)
{
  *(Coord *) dst = ghid_coord_entry_get_value (ce);
  ghidgui->config_modified = TRUE;
}

static void
config_sizes_tab_create (GtkWidget * tab_vbox)
{
  GtkWidget *table, *vbox, *hbox;

  /* Need a vbox we can destroy if user changes grid units.
   */
  if (!config_sizes_vbox)
    {
      vbox = gtk_vbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (tab_vbox), vbox, FALSE, FALSE, 0);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
      config_sizes_vbox = vbox;
      config_sizes_tab_vbox = tab_vbox;
    }

  /* ---- Board Size ---- */
  vbox = ghid_category_vbox (config_sizes_vbox, _("Board Size"),
			     4, 2, TRUE, TRUE);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  table = gtk_table_new (2, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);

  new_board_width = PCB->MaxWidth;
  new_board_height = PCB->MaxHeight;
  ghid_table_coord_entry (table, 0, 0, NULL,
			  PCB->MaxWidth, MIN_SIZE, MAX_COORD,
			  CE_LARGE, 0, coord_entry_cb,
			  &new_board_width, FALSE, _("Width"));

  ghid_table_coord_entry (table, 1, 0, NULL,
			  PCB->MaxHeight, MIN_SIZE, MAX_COORD,
			  CE_LARGE, 0, coord_entry_cb,
			  &new_board_height, FALSE, _("Height"));
  ghid_check_button_connected (vbox, &use_board_size_default_button, FALSE,
			       TRUE, FALSE, FALSE, 0, NULL, NULL,
			       _("Use this board size as the default for new layouts"));

  /* ---- Text Scale ---- */
  vbox = ghid_category_vbox (config_sizes_vbox, _("Text Scale"),
			     4, 2, TRUE, TRUE);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  table = gtk_table_new (4, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);

  ghid_table_spin_button (table, 0, 0, &config_text_spin_button,
			  Settings.TextScale,
			  MIN_TEXTSCALE, MAX_TEXTSCALE,
			  10.0, 10.0,
			  0, 0, text_spin_button_cb,
			  &Settings.TextScale, FALSE, "%");


  /* ---- DRC Sizes ---- */
  vbox = ghid_category_vbox (config_sizes_vbox, _("Design Rule Checking"),
			     4, 2, TRUE, TRUE);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  table = gtk_table_new (4, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);

  ghid_table_coord_entry (table, 0, 0, NULL,
			  PCB->Bloat, MIN_DRC_VALUE, MAX_DRC_VALUE,
			  CE_SMALL, 0, coord_entry_cb,
			  &PCB->Bloat, FALSE,
			  _("Minimum copper spacing"));

  ghid_table_coord_entry (table, 1, 0, NULL,
			  PCB->minWid, MIN_DRC_VALUE, MAX_DRC_VALUE,
			  CE_SMALL, 0, coord_entry_cb,
			  &PCB->minWid, FALSE,
			  _("Minimum copper width"));

  ghid_table_coord_entry (table, 2, 0, NULL,
			  PCB->Shrink, MIN_DRC_VALUE, MAX_DRC_VALUE,
			  CE_SMALL, 0, coord_entry_cb,
			  &PCB->Shrink, FALSE,
			  _("Minimum touching copper overlap"));

  ghid_table_coord_entry (table, 3, 0, NULL,
			  PCB->minSlk, MIN_DRC_VALUE, MAX_DRC_VALUE,
			  CE_SMALL, 0, coord_entry_cb,
			  &PCB->minSlk, FALSE,
			  _("Minimum silk width"));

  ghid_table_coord_entry (table, 4, 0, NULL,
			  PCB->minDrill, MIN_DRC_VALUE, MAX_DRC_VALUE,
			  CE_SMALL, 0, coord_entry_cb,
			  &PCB->minDrill, FALSE,
			  _("Minimum drill diameter"));

  ghid_table_coord_entry (table, 5, 0, NULL,
			  PCB->minRing, MIN_DRC_VALUE, MAX_DRC_VALUE,
			  CE_SMALL, 0, coord_entry_cb,
			  &PCB->minRing, FALSE,
			  _("Minimum annular ring"));

  ghid_check_button_connected (vbox, &use_drc_sizes_default_button, FALSE,
			       TRUE, FALSE, FALSE, 0, NULL, NULL,
			       _
			       ("Use DRC values as the default for new layouts"));

  gtk_widget_show_all (config_sizes_vbox);
}


  /* -------------- The Increments config page ----------------
   */
  /* Increment/decrement values are kept in mil and mm units and not in
     |  PCB units.
   */
static GtkWidget *config_increments_vbox, *config_increments_tab_vbox;

static void
increment_spin_button_cb (GHidCoordEntry * ce, void * dst)
{
  *(Coord *)dst = ghid_coord_entry_get_value (ce);
  ghidgui->config_modified = TRUE;
}

static void
config_increments_tab_create (GtkWidget * tab_vbox)
{
  Increments *incr_mm  = get_increments_struct (METRIC);
  Increments *incr_mil = get_increments_struct (IMPERIAL);
  GtkWidget *vbox, *hbox, *table;

  /* Need a vbox we can destroy if user changes grid units.
   */
  if (!config_increments_vbox)
    {
      vbox = gtk_vbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (tab_vbox), vbox, FALSE, FALSE, 0);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
      config_increments_vbox = vbox;
      config_increments_tab_vbox = tab_vbox;
    }

#define INCR_ENTRY(row, name, family, type, msg)	\
  gtk_table_attach_defaults (GTK_TABLE (table),	\
                             gtk_label_new (name),	\
                             0, 1, row, row + 1);	\
  ghid_table_coord_entry (table, row, 1, NULL,	\
                          incr_##family->type,	\
                          incr_##family->type##_min,	\
                          incr_##family->type##_max,	\
			  CE_SMALL, 0, increment_spin_button_cb,	\
			  &incr_##family->type, FALSE,	\
                          msg)
  
  /* ---- Metric Settings ---- */
  vbox = ghid_category_vbox (config_increments_vbox,
			     _("Metric Increment Settings"), 4, 2, TRUE, TRUE);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  table = gtk_table_new (4, 3, FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 18);

  INCR_ENTRY (0, _("Grid:"), mm, grid,
              _("For 'g' and '<shift>g' grid change actions"));
  INCR_ENTRY (1, _("Size:"), mm, size,
              _("For 's' and '<shift>s' size change actions on lines,\n"
                "pads, pins and text.\n"
                "Use '<ctrl>s' and '<shift><ctrl>s' for drill holes."));
  INCR_ENTRY (2, _("Line:"), mm, line,
              _("For 'l' and '<shift>l' routing line width change actions"));
  INCR_ENTRY (3, _("Clear:"), mm, clear,
              _("For 'k' and '<shift>k' line clearance inside polygon size\n"
                "change actions"));

  vbox = ghid_category_vbox (config_increments_vbox,
			     _("Imperial Increment Settings"), 4, 2, TRUE, TRUE);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  table = gtk_table_new (4, 3, FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 18);

  INCR_ENTRY (0, _("Grid:"), mil, grid,
              _("For 'g' and '<shift>g' grid change actions"));
  INCR_ENTRY (1, _("Size:"), mil, size,
              _("For 's' and '<shift>s' size change actions on lines,\n"
                "pads, pins and text.\n"
                "Use '<ctrl>s' and '<shift><ctrl>s' for drill holes."));
  INCR_ENTRY (2, _("Line:"), mil, line,
              _("For 'l' and '<shift>l' routing line width change actions"));
  INCR_ENTRY (3, _("Clear:"), mil, clear,
              _("For 'k' and '<shift>k' line clearance inside polygon size\n"
                "change actions"));
#undef INCR_ENTRY

  vbox = ghid_category_vbox (config_increments_vbox,
			     _("Save as Default"), 4, 2, TRUE, TRUE);
  ghid_check_button_connected (vbox, &use_increments_default_button, FALSE,
			       TRUE, FALSE, FALSE, 0, NULL, NULL,
			       _("Use values as the default for new layouts"));

  gtk_widget_show_all (config_increments_vbox);
}

  /* -------------- The Library config page ----------------
   */
static GtkWidget *library_newlib_entry;

static void
config_library_apply (void)
{
  if (dup_string
      (&lib_newlib_config, ghid_entry_get_text (library_newlib_entry)))
    ghidgui->config_modified = TRUE;
}

static void
config_library_tab_create (GtkWidget * tab_vbox)
{
  GtkWidget *vbox, *label, *entry;
  GString *string;

  gtk_container_set_border_width (GTK_CONTAINER (tab_vbox), 6);
  vbox = ghid_category_vbox (tab_vbox, _("Element Directories"),
			     4, 2, TRUE, TRUE);
  string = g_string_new ("");
  g_string_printf (string, _("<small>Enter a \"%s\" "
			 "separated list of custom top level\n"
			 "element directories.  For example:\n%s\n"
			 "Elements should be organized into subdirectories below each\n"
			 "top level directory.  Restart program for changes to take effect."
			 "</small>"), PCB_PATH_DELIMETER,
			 "\t<b>~/gaf/pcb-elements" PCB_PATH_DELIMETER
			 "packages" PCB_PATH_DELIMETER "/usr/local/pcb-elements</b>\n");
  label = gtk_label_new ("");
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_markup (GTK_LABEL (label), string->str);
  g_string_free (string, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  entry = gtk_entry_new ();
  library_newlib_entry = entry;
  gtk_entry_set_text (GTK_ENTRY (entry), lib_newlib_config);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 4);
}


  /* -------------- The Layers Group config page ----------------
   */
static GtkWidget	*config_groups_table, *config_groups_vbox, *config_groups_window;

static GtkWidget *layer_entry[MAX_LAYER];
static GtkWidget *group_button[MAX_ALL_LAYER][MAX_GROUP];

#if FIXME
static GtkWidget *use_layer_default_button;
#endif

static gint config_layer_group[MAX_ALL_LAYER];

static LayerGroupType layer_groups,	/* Working copy */
 *lg_monitor;			/* Keep track if our working copy */
										/* needs to be changed (new layout) */

static gboolean groups_modified, groups_holdoff, layers_applying;

static gchar *layer_info_text[] = {
  N_("<h>Layer Names\n"),
  N_("You may enter layer names for the layers drawn on the screen.\n"
     "The special 'top side' and 'bottom side' are layers which\n"
     "will be printed out, so they must have in their group at least one\n"
     "of the other layers that are drawn on the screen.\n"),
  "\n",
  N_("<h>Layer Groups\n"),
  N_("Each layer on the screen may be in its own group which allows the\n"
     "maximum number of board layers.  However, for boards with fewer\n"
     "layers, you may group layers together which will then print as a\n"
     "single layer on a printout.  This allows a visual color distinction\n"
     "to be displayed on the screen for signal groups which will print as\n"
     "a single layer\n"),
  "\n",
  N_("For example, for a 4 layer board a useful layer group arrangement\n"
     "can be to have 3 screen displayed layers grouped into the same group\n"
     "as the 'top side' and 'bottom side' printout layers.  Then\n"
     "groups such as signals, ground, and supply traces can be color\n"
     "coded on the screen while printing as a single layer.  For this\n"
     "you would select buttons and enter names on the Setup page to\n"
     "structure four layer groups similar to this:\n"),
  "\n",
  N_("<b>Group 1:"),
  "\n\t",
  N_("top"),
  "\n\t",
  N_("GND-top"),
  "\n\t",
  N_("Vcc-top"),
  "\n\t",
  N_("top side"),
  "\n",
  N_("<b>Group 2:"),
  "\n\t",
  N_("bottom"),
  "\n\t",
  N_("GND-bottom"),
  "\n\t",
  N_("Vcc-bottom"),
  "\n\t",
  N_("bottom side"),
  "\n",
  N_("<b>Group 3:"),
  "\n\t",
  N_("signal1"),
  "\n",
  N_("<b>Group 4:"),
  "\n\t",
  N_("signal2"),
  "\n"
};

static void
config_layer_groups_radio_button_cb (GtkToggleButton * button, gpointer data)
{
  gint layer = GPOINTER_TO_INT (data) >> 8;
  gint group = GPOINTER_TO_INT (data) & 0xff;

  if (!gtk_toggle_button_get_active (button) || groups_holdoff)
    return;
  config_layer_group[layer] = group;
  groups_modified = TRUE;
  ghidgui->config_modified = TRUE;
}

  /* Construct a layer group string.  Follow logic in WritePCBDataHeader(),
     |  but use g_string functions.
   */
static gchar *
make_layer_group_string (LayerGroupType * lg)
{
  GString *string;
  gint group, entry, layer;

  string = g_string_new ("");

  for (group = 0; group < max_group; group++)
    {
      if (lg->Number[group] == 0)
	continue;
      for (entry = 0; entry < lg->Number[group]; entry++)
	{
	  layer = lg->Entries[group][entry];
	  if (layer == top_silk_layer)
	    string = g_string_append (string, "c");
	  else if (layer == bottom_silk_layer)
	    string = g_string_append (string, "s");
	  else
	    g_string_append_printf (string, "%d", layer + 1);

	  if (entry != lg->Number[group] - 1)
	    string = g_string_append (string, ",");
	}
      if (group != max_group - 1)
	string = g_string_append (string, ":");
    }
  return g_string_free (string, FALSE);	/* Don't free string->str */
}

static void
config_layers_apply (void)
{
  LayerType *layer;
  gchar *s;
  gint group, i;
  gint top_group = 0, bottom_group = 0;
  gboolean use_as_default = FALSE, layers_modified = FALSE;

#if FIXME
  use_as_default =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
				  (use_layer_default_button));
#endif

  /* Get each layer name entry and dup if modified into the PCB layer names
     |  and, if to use as default, the Settings layer names.
   */
  for (i = 0; i < max_copper_layer; ++i)
    {
      layer = &PCB->Data->Layer[i];
      s = ghid_entry_get_text (layer_entry[i]);
      if (dup_core_string (&layer->Name, s))
	layers_modified = TRUE;
/* FIXME */
      if (use_as_default && dup_core_string (&Settings.DefaultLayerName[i], s))
	ghidgui->config_modified = TRUE;

    }
  /* Layer names can be changed from the menus and that can update the
     |  config.  So holdoff the loop.
   */
  layers_applying = TRUE;
  if (layers_modified)
    ghid_layer_buttons_update ();
  layers_applying = FALSE;

  if (groups_modified)		/* If any group radio buttons were toggled. */
    {
      /* clear all entries and read layer by layer
       */
      for (group = 0; group < max_group; group++)
	layer_groups.Number[group] = 0;

      for (i = 0; i < max_copper_layer + SILK_LAYER; i++)
	{
	  group = config_layer_group[i] - 1;
	  layer_groups.Entries[group][layer_groups.Number[group]++] = i;

	  if (i == top_silk_layer)
	    top_group = group;
	  else if (i == bottom_silk_layer)
	    bottom_group = group;
	}

      /* do some cross-checking
         |  top-side and bottom-side must be in different groups
         |  top-side and bottom-side must not be the only one in the group
       */
      if (layer_groups.Number[bottom_group] <= 1
	  || layer_groups.Number[top_group] <= 1)
	{
	  Message (_
		   ("Both, 'top side' and 'bottom side' layer must have at least\n"
		    "\tone other layer in their group.\n"));
	  return;
	}
      else if (bottom_group == top_group)
	{
	  Message (_
		   ("The 'top side' and 'bottom side' layers are not allowed\n"
		    "\tto be in the same layer group #\n"));
	  return;
	}
      PCB->LayerGroups = layer_groups;
      ghid_invalidate_all();
      groups_modified = FALSE;
    }
  if (use_as_default)
    {
      s = make_layer_group_string (&PCB->LayerGroups);
      if (dup_core_string (&Settings.Groups, s))
	{
	  ParseGroupString (Settings.Groups, &Settings.LayerGroups, &max_copper_layer);
	  ghidgui->config_modified = TRUE;
	}
      g_free (s);
    }
}

static void
config_layer_group_button_state_update (void)
{
  gint g, i;

  /* Set button active corresponding to layer group state.
   */
  groups_holdoff = TRUE;
  for (g = 0; g < max_group; g++)
    for (i = 0; i < layer_groups.Number[g]; i++)
      {
/*			printf("layer %d in group %d\n", layer_groups.Entries[g][i], g +1); */
	config_layer_group[layer_groups.Entries[g][i]] = g + 1;
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (group_button
				       [layer_groups.Entries[g][i]][g]),
				      TRUE);
      }
  groups_holdoff = FALSE;
}

static void
layer_name_entry_cb(GtkWidget *entry, gpointer data)
{
	gint		i = GPOINTER_TO_INT(data);
	LayerType	*layer;
	gchar		*name;

	layer = &PCB->Data->Layer[i];
	name = ghid_entry_get_text(entry);
	if (dup_core_string (&layer->Name, name))
		ghid_layer_buttons_update();
}

void
ghid_config_groups_changed(void)
{
  GtkWidget *vbox, *table, *button, *label, *scrolled_window;
  GSList *group;
  gchar buf[32], *name;
  gint layer, row, i;

  if (!config_groups_vbox)
	return;
  vbox = config_groups_vbox;

  if (config_groups_table)
	gtk_widget_destroy(config_groups_table);
  if (config_groups_window)
    gtk_widget_destroy(config_groups_window);

  config_groups_window = scrolled_window = 
    gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scrolled_window, 34, 408);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 3);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);


  table = gtk_table_new (max_copper_layer + SILK_LAYER + 1,
                         max_group + 1, FALSE);
  config_groups_table = table;
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_scrolled_window_add_with_viewport (
        GTK_SCROLLED_WINDOW (scrolled_window), table);
  gtk_widget_show (table);

  layer_groups = PCB->LayerGroups;	/* working copy */
  lg_monitor = &PCB->LayerGroups;	/* So can know if PCB changes on us */

  label = gtk_label_new (_("Layer group number: "));
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

  for (i = 1; i < max_group + 1; ++i)
    {
	if (i < 10)
		snprintf (buf, sizeof (buf), "%d   ", i);
	else
		snprintf (buf, sizeof (buf), "%d", i);
      label = gtk_label_new (buf);
      gtk_table_attach_defaults (GTK_TABLE (table), label, i, i + 1, 0, 1);
    }

  /* Create a row of radio toggle buttons for layer.  So each layer
     |  can have an active radio button set for the group it needs to be in.
   */
  for (layer = 0; layer < max_copper_layer; ++layer)
    {
	  name = (gchar *) UNKNOWN (PCB->Data->Layer[layer].Name);
	  row = layer + 1;
 
	  layer_entry[layer] = gtk_entry_new ();
	  gtk_entry_set_text (GTK_ENTRY (layer_entry[layer]), name);
	  gtk_table_attach_defaults (GTK_TABLE (table), layer_entry[layer],
				     0, 1, row, row + 1);
	  g_signal_connect(G_OBJECT(layer_entry[layer]), "activate",
				G_CALLBACK(layer_name_entry_cb), GINT_TO_POINTER(layer));

      group = NULL;
      for (i = 0; i < max_group; ++i)
	{
	  button = gtk_radio_button_new (group);
	  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), TRUE);
	  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
	  gtk_table_attach_defaults (GTK_TABLE (table), button,
				     i + 1, i + 2, row, row +1);
	  g_signal_connect (G_OBJECT (button), "toggled",
			    G_CALLBACK (config_layer_groups_radio_button_cb),
			    GINT_TO_POINTER ((layer << 8) | (i + 1)));
	  group_button[layer][i] = button;
	}
    }

  /* silk layers double as special layer groups 'top side' and 'bottom side'. 
    So some special treatment is needed:
    1) top and bottom silk cannot be renamed by the user
    2) The dialog shows the top side buttons above bottom side button row
   */
  group = NULL;
  layer = top_silk_layer;
  label = gtk_label_new ("Top side: ");
  row = max_copper_layer + 1;
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
				     0, 1, row, row + 1);
  for (i = 0; i < max_group; ++i)
	{
	  button = gtk_radio_button_new (group);
	  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), TRUE);
	  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
	  gtk_table_attach_defaults (GTK_TABLE (table), button,
				     i + 1, i + 2, row, row + 1);
	  g_signal_connect (G_OBJECT (button), "toggled",
			    G_CALLBACK (config_layer_groups_radio_button_cb),
			    GINT_TO_POINTER ((layer << 8) | (i + 1)));
	  group_button[layer][i] = button;
	}
  
  group = NULL;
  layer = bottom_silk_layer;
  label = gtk_label_new ("Bottom side: ");
  row = max_copper_layer + 2;
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
				     0, 1, row, row + 1);
  for (i = 0; i < max_group; ++i)
	{
	  button = gtk_radio_button_new (group);
	  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), TRUE);
	  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
	  gtk_table_attach_defaults (GTK_TABLE (table), button,
				     i + 1, i + 2, row, row + 1);
	  g_signal_connect (G_OBJECT (button), "toggled",
			    G_CALLBACK (config_layer_groups_radio_button_cb),
			    GINT_TO_POINTER ((layer << 8) | (i + 1)));
	  group_button[layer][i] = button;
	}

  gtk_widget_show_all(config_groups_vbox);
  config_layer_group_button_state_update ();
}


static void
edit_layer_button_cb(GtkWidget *widget, gchar *data)
{
	gchar	**argv;

	if (PCB->RatDraw || PCB->SilkActive)
		return;

	argv = g_strsplit(data, ",", -1);
	MoveLayerAction(2, argv, 0, 0);
	g_strfreev(argv);
}

static void
config_layers_tab_create (GtkWidget * tab_vbox)
{
  GtkWidget *tabs, *vbox, *vbox1, *button, *text, *sep;
  GtkWidget *hbox, *arrow;
  gint i;

  tabs = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (tab_vbox), tabs, TRUE, TRUE, 0);

/* -- Change tab */
  vbox = ghid_notebook_page(tabs, _("Change"), 0, 6);
  vbox1 = ghid_category_vbox(vbox,
			_("Operations on currently selected layer:"),
			4, 2, TRUE, TRUE);

  button = gtk_button_new();
  arrow = gtk_arrow_new(GTK_ARROW_UP, GTK_SHADOW_ETCHED_IN);
  gtk_container_add(GTK_CONTAINER(button), arrow);
  g_signal_connect(G_OBJECT(button), (gchar *)"clicked",
		   G_CALLBACK(edit_layer_button_cb), (gchar *)"c,up");
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox1), hbox, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

  button = gtk_button_new();
  arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_ETCHED_IN);
  gtk_container_add(GTK_CONTAINER(button), arrow);
  g_signal_connect(G_OBJECT(button), (gchar *)"clicked",
		   G_CALLBACK(edit_layer_button_cb), (gchar *)"c,down");
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox1), hbox, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

  button = gtk_button_new_from_stock(GTK_STOCK_DELETE);
  g_signal_connect(G_OBJECT(button), (gchar *)"clicked",
		   G_CALLBACK(edit_layer_button_cb), (gchar *)"c,-1");
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox1), hbox, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

  vbox1 = ghid_category_vbox(vbox,
			_("Add new layer above currently selected layer:"),
			4, 2, TRUE, TRUE);
  button = gtk_button_new_from_stock(GTK_STOCK_ADD);
  g_signal_connect(G_OBJECT(button), (gchar *)"clicked",
		   G_CALLBACK(edit_layer_button_cb), (gchar *)"-1,c");
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox1), hbox, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

/* -- Groups tab */
  vbox = ghid_notebook_page (tabs, _("Groups"), 0, 6);
  config_groups_vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), config_groups_vbox, FALSE, FALSE, 0);
  ghid_config_groups_changed();
  
  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 4);

#if FIXME
  ghid_check_button_connected (vbox, &use_layer_default_button, FALSE,
			       TRUE, FALSE, FALSE, 8, NULL, NULL,
			       ("Use these layer settings as the default for new layouts"));
#endif


/* -- Info tab */
  vbox = ghid_notebook_page (tabs, C_("tab", "Info"), 0, 6);

  text = ghid_scrolled_text_view (vbox, NULL,
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  for (i = 0; i < sizeof (layer_info_text) / sizeof (gchar *); ++i)
    ghid_text_view_append (text, _(layer_info_text[i]));
}


void
ghid_config_layer_name_update (gchar * name, gint layer)
{
  if (!config_window || layers_applying || !name)
    return;
  gtk_entry_set_text (GTK_ENTRY (layer_entry[layer]), name);

  /* If we get a config layer name change because a new PCB is loaded
     |  or new layout started, need to change our working layer group copy.
   */
  if (lg_monitor != &PCB->LayerGroups)
    {
      layer_groups = PCB->LayerGroups;
      lg_monitor = &PCB->LayerGroups;
      config_layer_group_button_state_update ();
      groups_modified = FALSE;
    }
}

  /* -------------- The Colors config page ----------------
   */
static GtkWidget *config_colors_vbox,
  *config_colors_tab_vbox,
  *config_colors_save_button,
  *config_color_file_label, *config_color_warn_label;

static void config_colors_tab_create (GtkWidget * tab_vbox);

static gboolean config_colors_modified;

static void
config_color_file_set_label (void)
{
  gchar *str, *name;

  if (!*color_file)
    name = g_strdup ("defaults");
  else
    name = g_path_get_basename (color_file);

  str = g_strdup_printf (_("Current colors loaded: <b>%s</b>"), name);
  gtk_label_set_markup (GTK_LABEL (config_color_file_label), str);
  g_free (name);
  g_free (str);
}

static void
config_color_defaults_cb (gpointer data)
{
  GList *list;
  ConfigColor *cc;
  HID_Attribute *ha;

  for (list = config_color_list; list; list = list->next)
    {
      cc = (ConfigColor *) list->data;
      ha = cc->attributes;
      dup_core_string ((char **) ha->value, ha->default_val.str_value);
      cc->color_is_mapped = FALSE;
      ghid_set_special_colors (ha);
    }

  dup_string (&color_file, "");
  ghidgui->config_modified = TRUE;

  gtk_widget_set_sensitive (config_colors_save_button, FALSE);
  gtk_widget_set_sensitive (config_color_warn_label, FALSE);
  config_color_file_set_label ();
  config_colors_modified = FALSE;

  ghid_layer_buttons_color_update ();

  /* Receate the colors config page to pick up new colors.
   */
  gtk_widget_destroy (config_colors_vbox);
  config_colors_tab_create (config_colors_tab_vbox);

  ghid_invalidate_all();
}

static void
config_color_load_cb (gpointer data)
{
  gchar *path, *dir = g_strdup (color_dir);

  path = ghid_dialog_file_select_open (_("Load Color File"), &dir, NULL);
  if (path)
    {
      config_colors_read (path);
      dup_string (&color_file, path);
      ghidgui->config_modified = TRUE;

      gtk_widget_set_sensitive (config_colors_save_button, FALSE);
      gtk_widget_set_sensitive (config_color_warn_label, FALSE);
      config_color_file_set_label ();
      config_colors_modified = FALSE;
    }
  g_free (path);
  g_free (dir);

  /* Receate the colors config page to pick up new colors.
   */
  gtk_widget_destroy (config_colors_vbox);
  config_colors_tab_create (config_colors_tab_vbox);

  ghid_layer_buttons_color_update ();
  ghid_invalidate_all();
}

static void
config_color_save_cb (gpointer data)
{
  gchar *name, *path, *dir = g_strdup (color_dir);

  path =
    ghid_dialog_file_select_save (_("Save Color File"), &dir, NULL, NULL);
  if (path)
    {
      name = g_path_get_basename (path);
      if (!strcmp (name, "default"))
	ghid_dialog_message (_
			     ("Sorry, not overwriting the default color file!"));
      else
	{
	  config_colors_write (path);
	  dup_string (&color_file, path);
	  ghidgui->config_modified = TRUE;

	  gtk_widget_set_sensitive (config_colors_save_button, FALSE);
	  gtk_widget_set_sensitive (config_color_warn_label, FALSE);
	  config_color_file_set_label ();
	  config_colors_modified = FALSE;
	}
      g_free (name);
    }
  g_free (path);
  g_free (dir);
}

static void
config_color_set_cb (GtkWidget * button, ConfigColor * cc)
{
  GdkColor new_color;
  HID_Attribute *ha = cc->attributes;
  gchar *str;

  gtk_color_button_get_color (GTK_COLOR_BUTTON (button), &new_color);
  str = ghid_get_color_name (&new_color);
  ghid_map_color_string (str, &cc->color);
  *(char **) ha->value = str;
/*	g_free(str);		Memory leak */

  config_colors_modified = TRUE;
  gtk_widget_set_sensitive (config_colors_save_button, TRUE);
  gtk_widget_set_sensitive (config_color_warn_label, TRUE);

  ghid_set_special_colors (ha);
  ghid_layer_buttons_color_update ();
  ghid_invalidate_all();
}

static void
config_color_button_create (GtkWidget * box, ConfigColor * cc)
{
  GtkWidget *button, *hbox, *label;
  HID_Attribute *ha = cc->attributes;
  gchar *title;

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);

  if (!cc->color_is_mapped)
    ghid_map_color_string (*(char **) ha->value, &cc->color);
  cc->color_is_mapped = TRUE;

  title = g_strdup_printf (_("PCB %s Color"), ha->name);
  button = gtk_color_button_new_with_color (&cc->color);
  gtk_color_button_set_title (GTK_COLOR_BUTTON (button), title);
  g_free (title);

  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  label = gtk_label_new (ha->name);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "color-set",
		    G_CALLBACK (config_color_set_cb), cc);
}

static void
config_colors_tab_create (GtkWidget * tab_vbox)
{
  GtkWidget *scrolled_vbox, *vbox, *hbox, *expander, *sep;
  GList *list;
  ConfigColor *cc;

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (tab_vbox), vbox, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

  config_colors_vbox = vbox;	/* can be destroyed if color file loaded */
  config_colors_tab_vbox = tab_vbox;

  scrolled_vbox = ghid_scrolled_vbox (config_colors_vbox, NULL,
				      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  /* ---- Main colors ---- */
  expander = gtk_expander_new (_("Main colors"));
  gtk_box_pack_start (GTK_BOX (scrolled_vbox), expander, FALSE, FALSE, 2);
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (expander), vbox);
  vbox = ghid_category_vbox (vbox, NULL, 0, 2, TRUE, FALSE);

  for (list = config_color_list; list; list = list->next)
    {
      cc = (ConfigColor *) list->data;
      if (cc->type != MISC_COLOR)
	continue;
      config_color_button_create (vbox, cc);
    }

  /* ---- Layer colors ---- */
  expander = gtk_expander_new (_("Layer colors"));
  gtk_box_pack_start (GTK_BOX (scrolled_vbox), expander, FALSE, FALSE, 2);
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (expander), vbox);
  vbox = ghid_category_vbox (vbox, NULL, 0, 2, TRUE, FALSE);

  for (list = config_color_list; list; list = list->next)
    {
      cc = (ConfigColor *) list->data;
      if (cc->type != LAYER_COLOR)
	continue;
      config_color_button_create (vbox, cc);
    }

  /* ---- Selected colors ---- */
  expander = gtk_expander_new (_("Selected colors"));
  gtk_box_pack_start (GTK_BOX (scrolled_vbox), expander, FALSE, FALSE, 2);
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (expander), vbox);
  vbox = ghid_category_vbox (vbox, NULL, 0, 2, TRUE, FALSE);

  for (list = config_color_list; list; list = list->next)
    {
      cc = (ConfigColor *) list->data;
      if (cc->type != MISC_SELECTED_COLOR)
	continue;
      config_color_button_create (vbox, cc);
    }
  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 2);
  for (list = config_color_list; list; list = list->next)
    {
      cc = (ConfigColor *) list->data;
      if (cc->type != LAYER_SELECTED_COLOR)
	continue;
      config_color_button_create (vbox, cc);
    }

  config_color_warn_label = gtk_label_new ("");
  gtk_label_set_use_markup (GTK_LABEL (config_color_warn_label), TRUE);
  gtk_label_set_markup (GTK_LABEL (config_color_warn_label),
			_("<b>Warning:</b> unsaved color changes will be lost"
			  " at program exit."));
  gtk_box_pack_start (GTK_BOX (config_colors_vbox), config_color_warn_label,
		      FALSE, FALSE, 4);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (config_colors_vbox), hbox, FALSE, FALSE, 6);

  config_color_file_label = gtk_label_new ("");
  gtk_label_set_use_markup (GTK_LABEL (config_color_file_label), TRUE);
  config_color_file_set_label ();
  gtk_box_pack_start (GTK_BOX (hbox), config_color_file_label,
		      FALSE, FALSE, 0);

  ghid_button_connected (hbox, NULL, FALSE, FALSE, FALSE, 4,
			 config_color_load_cb, NULL, _("Load"));
  ghid_button_connected (hbox, &config_colors_save_button,
			 FALSE, FALSE, FALSE, 4,
			 config_color_save_cb, NULL, _("Save"));
  ghid_button_connected (hbox, NULL, FALSE, FALSE, FALSE, 4,
			 config_color_defaults_cb, NULL, _("Defaults"));

  gtk_widget_set_sensitive (config_colors_save_button,
			    config_colors_modified);
  gtk_widget_set_sensitive (config_color_warn_label, config_colors_modified);
  gtk_widget_show_all (config_colors_vbox);
}

/* -------------- The Fonts config page ----------------
 */
static GtkWidget *config_fonts_vbox;
static GtkWidget *list_embfont, *list_sysfont;
static GtkWidget *lab_sysfont, *lab_pcbfont;
static GtkWidget *xbox_savefonts, *xbox_savesyms, *xbox_embedfonts;
static GtkWidget *lab_warnfonts;
static void config_fonts_tab_create (GtkWidget * tab_vbox);

void init_font_list(GtkWidget *list, char * name) {
    
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkListStore *store;
    
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes(name,
                                                      renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
    
    store = gtk_list_store_new(1, G_TYPE_STRING);
    
    gtk_tree_view_set_model(GTK_TREE_VIEW(list),
                            GTK_TREE_MODEL(store));
    
    g_object_unref(store);
}

void add_to_font_list(GtkWidget *list, const gchar *str) {
    
    GtkListStore *store;
    GtkTreeIter iter;
    
    store = GTK_LIST_STORE(gtk_tree_view_get_model
                           (GTK_TREE_VIEW(list)));
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, str, -1);
}

void on_sel_font_changed(GtkWidget *widget, gpointer label) {

    /*
    GtkTreeIter iter;
    GtkTreeModel *model;
    gchar *value;
     */
    GtkTreeView * treeview;
    GtkTreeSelection * sel;
    
    /* Leaving this code as a placeholder/reminder for updating a font preview
     * in the config window */
    /*
    if (gtk_tree_selection_get_selected(
            GTK_TREE_SELECTION(widget), &model, &iter))
    {
        
        gtk_tree_model_get(model, &iter, 0, &value,  -1);
        // do somethign
        g_free(value);
    }*/
    
    /* TODO: This logic doesn't work quite right. If something else had the
             focus, like one of the set buttons or a checkbox, and something
             in the other list is selected, both lists become unselected.
     */
    /* Only one font selected at a time, so if unselect everything in the list
       that wasn't clicked on. */
    treeview = gtk_tree_selection_get_tree_view(GTK_TREE_SELECTION(widget));
    if(treeview == GTK_TREE_VIEW(list_sysfont))
        sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list_embfont));
    else
        sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list_sysfont));
    if(!gtk_widget_has_focus(GTK_WIDGET(treeview)))
        gtk_tree_selection_unselect_all(sel);
}

static void
config_fonts_xb_cb(GtkToggleButton * btn, gpointer p)
{
    gboolean active = gtk_toggle_button_get_active(btn);
    *((bool*)p) = active;
    
    if (Settings.SaveFontData) Settings.SaveSymbols = FALSE;
    else Settings.EmbedFonts = FALSE;

    gtk_widget_set_sensitive(xbox_embedfonts, Settings.SaveFontData);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(xbox_embedfonts), Settings.EmbedFonts);
    gtk_widget_set_sensitive(xbox_savesyms, !Settings.SaveFontData);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(xbox_savesyms), Settings.SaveSymbols);

    ghidgui->config_modified = TRUE;
}

static void
config_fonts_setbtn_cb(GtkWidget * btn, gpointer p)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    gchar *value, *str;
    GtkTreeSelection * sel;
    
    /* Figure out which font is selected */
    /* Check the system font list */
    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list_sysfont));
    /* If not, check the embedded font list */
    if (!gtk_tree_selection_get_selected(sel, &model, &iter))
      sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list_embfont));
    /* If nothing's selected, don't do anything. */
    if (!gtk_tree_selection_get_selected(sel, &model, &iter))
      return;
    
    gtk_tree_model_get(model, &iter, 0, &value,  -1);
    
    if (p == &Settings.Font) ChangeSystemFont(value);
    else if (p == &PCB->DefaultFontName) SetPCBDefaultFont(value);
    else if ((p == (void*)1) || (p == (void*)2)) /* Change fonts of all objects */
    {
        asprintf(&str, "ChangeFont(%s, %s)", (p==(void*)1) ? "All":"Selected", value);
        hid_parse_command(str);
        free(str);
    }

    g_free(value);
    
    asprintf(&str, "Current System Font: %s", Settings.Font->Name);
    gtk_label_set_text(GTK_LABEL(lab_sysfont), str);
    free(str);
    
    asprintf(&str, "Current PCB Font: %s", PCB->DefaultFontName);
    gtk_label_set_text(GTK_LABEL(lab_pcbfont), str);
    free(str);
    
    Redraw();
}

static void
config_fonts_tab_create(GtkWidget * tab_vbox)
{
    GtkWidget *hbox, *vbox, *align, *btn;
    GtkTreeSelection *selection;
    GSList * iter;
    char * str;
   
    config_fonts_vbox = tab_vbox;
    vbox = ghid_category_vbox (config_fonts_vbox, _("File Settings"), 4, 2, TRUE, TRUE);
    ghid_check_button_connected (vbox, &xbox_savefonts, Settings.SaveFontData,
                                 TRUE, FALSE, FALSE, 2,
                                 config_fonts_xb_cb, &Settings.SaveFontData,
                                 _("Save font data in files."));
    gtk_widget_set_tooltip_text(xbox_savefonts,
        "Enable or disable the storage of font information in saved files. "
        "Disable this setting to make saved files readable by older version of "
        "pcb.");
    ghid_check_button_connected (vbox, &xbox_savesyms, Settings.SaveSymbols,
                                 TRUE, FALSE, FALSE, 2,
                                 config_fonts_xb_cb, &Settings.SaveSymbols,
                                 _("Save current system font as symbols in pcb files (old format)."));
    gtk_widget_set_tooltip_text(xbox_savesyms,
        "This options allows a single font to be exported with the design in "
        "a way that can be read by older versions of pcb. This font will be "
        "used for all text elements, reference designators, and system text "
        "(e.g. pin numbers) in the design.");
    gtk_widget_set_sensitive(xbox_savesyms, !Settings.SaveFontData);
    ghid_check_button_connected (vbox, &xbox_embedfonts, Settings.EmbedFonts,
                                 TRUE, FALSE, FALSE, 2,
                                 config_fonts_xb_cb, &Settings.EmbedFonts,
                                 _("Embed used fonts in pcb files"));
    gtk_widget_set_tooltip_text(xbox_embedfonts,
        "Enabling this option includes font data for all used fonts in saved "
        "files. This ensures portability of files in case the destination "
        "doesn't have, or has a different version of, one of the used fonts.");
    gtk_widget_set_sensitive(xbox_embedfonts, Settings.SaveFontData);
    lab_warnfonts = gtk_label_new(
        "Warning: if you disable the saving of font data all font information\n"
        "will be lost when saved to disk. Please use File > Revert after saving\n"
        "to keep the displayed data consistent with that saved in the file.");
    gtk_box_pack_start(GTK_BOX(vbox), lab_warnfonts, FALSE, FALSE, 5);
    
    vbox = ghid_category_vbox (config_fonts_vbox, _("Fonts"), 4, 2, TRUE, TRUE);

    asprintf(&str, "Current System Font: %s", Settings.Font->Name);
    lab_sysfont = gtk_label_new(str);
    align = gtk_alignment_new(0,0,0,0);
    gtk_container_add(GTK_CONTAINER(align), lab_sysfont);
    gtk_box_pack_start(GTK_BOX(vbox), align, FALSE, FALSE, 5);
    free(str);
    gtk_widget_set_tooltip_text(lab_sysfont,
        "The system font is used for all new text elements, and generated text "
        "such as pin numbers and functions.");
  
    asprintf(&str, "Current PCB Font: %s", PCB->DefaultFontName ? PCB->DefaultFontName : "(null)");
    lab_pcbfont = gtk_label_new(str);
    align = gtk_alignment_new(0,0,0,0);
    gtk_container_add(GTK_CONTAINER(align), lab_pcbfont);
    gtk_box_pack_start(GTK_BOX(vbox), align, FALSE, FALSE, 5);
    free(str);
    gtk_widget_set_tooltip_text(lab_pcbfont,
        "The PCB font is used for all board object text that cannot be "
        "specifically assigned a font, such as element text (e.g. ref des).");

    
    hbox = gtk_hbox_new (TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    
    list_sysfont = gtk_tree_view_new();
    gtk_box_pack_start(GTK_BOX(hbox), list_sysfont, TRUE, TRUE, 5);
    gtk_widget_set_tooltip_text(list_sysfont,
        "System fonts are fonts pcb found in your font path.");

    list_embfont = gtk_tree_view_new();
    gtk_box_pack_start(GTK_BOX(hbox), list_embfont, TRUE, TRUE, 5);
    gtk_widget_set_tooltip_text(list_embfont,
        "Embedded fonts stored in the current pcb file.");

    hbox = gtk_hbox_new (TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    
    
    /* The gpointer passed to these callbacks is only used as an indicator of
     what the callback should do. So, we can really pass any value we want.
     We have to cast to a pointer though in order to avoid compile warnings.
     */
    btn = gtk_button_new_with_mnemonic("Set System Font");
    gtk_box_pack_start (GTK_BOX (hbox), btn, FALSE, FALSE, 0);
    g_signal_connect(btn, "clicked", G_CALLBACK(config_fonts_setbtn_cb), &Settings.Font);
    
    btn = gtk_button_new_with_mnemonic("Set PCB Font");
    gtk_box_pack_start (GTK_BOX (hbox), btn, FALSE, FALSE, 0);
    g_signal_connect(btn, "clicked", G_CALLBACK(config_fonts_setbtn_cb), &PCB->DefaultFontName);

    btn = gtk_button_new_with_mnemonic("Change All");
    gtk_box_pack_start (GTK_BOX (hbox), btn, FALSE, FALSE, 0);
    g_signal_connect(btn, "clicked", G_CALLBACK(config_fonts_setbtn_cb), (void*)1);

    btn = gtk_button_new_with_mnemonic("Change Selected");
    gtk_box_pack_start (GTK_BOX (hbox), btn, FALSE, FALSE, 0);
    g_signal_connect(btn, "clicked", G_CALLBACK(config_fonts_setbtn_cb), (void*)2);
    
    /* Populate the font lists */
    init_font_list(list_sysfont, "System Fonts");
    for (iter=Settings.FontLibrary; iter; iter=iter->next)
        add_to_font_list(list_sysfont, ((FontType*)iter->data)->Name);
    init_font_list(list_embfont, "Embedded Fonts");
    if (PCB->FontLibrary)
      for (iter = PCB->FontLibrary; iter; iter=iter->next)
        add_to_font_list(list_embfont, ((FontType*)iter->data)->Name);
    
    
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list_sysfont));
    
    g_signal_connect(selection, "changed",
                     G_CALLBACK(on_sel_font_changed), lab_sysfont);
    
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list_embfont));
    
    g_signal_connect(selection, "changed",
                     G_CALLBACK(on_sel_font_changed), lab_sysfont);
    
    
    gtk_widget_show_all (config_fonts_vbox);

}

  /* --------------- The main config page -----------------
   */
enum
{
  CONFIG_NAME_COLUMN,
  CONFIG_PAGE_COLUMN,
  N_CONFIG_COLUMNS
};

static GtkNotebook *config_notebook;

static GtkWidget *
config_page_create (GtkTreeStore * tree, GtkTreeIter * iter,
		    GtkNotebook * notebook)
{
  GtkWidget *vbox;
  gint page;

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_notebook_append_page (notebook, vbox, NULL);
  page = gtk_notebook_get_n_pages (notebook) - 1;
  gtk_tree_store_set (tree, iter, CONFIG_PAGE_COLUMN, page, -1);
  return vbox;
}

void
ghid_config_handle_units_changed (void)
{
  gchar *text = pcb_g_strdup_printf ("<b>%s</b>",
                                     Settings.grid_unit->in_suffix);
  ghid_set_cursor_position_labels ();
  gtk_label_set_markup (GTK_LABEL (ghidgui->grid_units_label), text);
  g_free (text);

  if (config_sizes_vbox)
    {
      gtk_widget_destroy (config_sizes_vbox);
      config_sizes_vbox = NULL;
      config_sizes_tab_create (config_sizes_tab_vbox);
    }
  if (config_increments_vbox)
    {
      gtk_widget_destroy (config_increments_vbox);
      config_increments_vbox = NULL;
      config_increments_tab_create (config_increments_tab_vbox);
    }
  ghidgui->config_modified = TRUE;
}

void
ghid_config_text_scale_update (void)
{
  if (config_window)
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (config_text_spin_button),
			       (gdouble) Settings.TextScale);
}

static void
config_close_cb (gpointer data)
{
  /* Config pages may need to check for modified entries, use as default
     |  options, etc when the config window is closed.
   */
  config_sizes_apply ();
  config_layers_apply ();
  config_library_apply ();
  config_general_apply ();

  config_sizes_vbox = NULL;
  config_increments_vbox = NULL;

  config_groups_vbox = config_groups_table = NULL;
  config_groups_window = NULL;

  gtk_widget_destroy (config_window);
  config_window = NULL;
}

static void
config_destroy_cb (gpointer data)
{
  config_sizes_vbox = NULL;
  config_increments_vbox = NULL;
  config_groups_vbox = config_groups_table = NULL;
  config_groups_window = NULL;
  gtk_widget_destroy (config_window);
  config_window = NULL;
}

static void
config_selection_changed_cb (GtkTreeSelection * selection, gpointer data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  gint page;

  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return;
  gtk_tree_model_get (model, &iter, CONFIG_PAGE_COLUMN, &page, -1);
  gtk_notebook_set_current_page (config_notebook, page);
}

void
ghid_config_window_show (void)
{
  GtkWidget *widget, *main_vbox, *vbox, *config_hbox, *hbox;
  GtkWidget *scrolled;
  GtkWidget *button;
  GtkTreeStore *model;
  GtkTreeView *treeview;
  GtkTreeIter iter;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *select;

  if (config_window)
    {
      gtk_window_present (GTK_WINDOW (config_window));
      return;
    }

  config_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (config_window), "delete_event",
		    G_CALLBACK (config_destroy_cb), NULL);

  gtk_window_set_title (GTK_WINDOW (config_window), _("PCB Preferences"));
  gtk_window_set_wmclass (GTK_WINDOW (config_window), "Pcb_Conf", "PCB");
  gtk_container_set_border_width (GTK_CONTAINER (config_window), 2);

  config_hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (config_window), config_hbox);

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (config_hbox), scrolled, FALSE, FALSE, 0);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (config_hbox), main_vbox, TRUE, TRUE, 0);

  widget = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), widget, TRUE, TRUE, 0);
  config_notebook = GTK_NOTEBOOK (widget);
  gtk_notebook_set_show_tabs (config_notebook, FALSE);

  model = gtk_tree_store_new (N_CONFIG_COLUMNS, G_TYPE_STRING, G_TYPE_INT);


  /* -- General -- */
  gtk_tree_store_append (model, &iter, NULL);
  gtk_tree_store_set (model, &iter, CONFIG_NAME_COLUMN, _("General"), -1);
  vbox = config_page_create (model, &iter, config_notebook);
  config_general_tab_create (vbox);


  /* -- Sizes -- */
  gtk_tree_store_append (model, &iter, NULL);
  gtk_tree_store_set (model, &iter, CONFIG_NAME_COLUMN, _("Sizes"), -1);
  vbox = config_page_create (model, &iter, config_notebook);
  config_sizes_tab_create (vbox);

  /* -- Increments -- */
  gtk_tree_store_append (model, &iter, NULL);
  gtk_tree_store_set (model, &iter, CONFIG_NAME_COLUMN, _("Increments"), -1);
  vbox = config_page_create (model, &iter, config_notebook);
  config_increments_tab_create (vbox);

  /* -- Library -- */
  gtk_tree_store_append (model, &iter, NULL);
  gtk_tree_store_set (model, &iter, CONFIG_NAME_COLUMN, _("Library"), -1);
  vbox = config_page_create (model, &iter, config_notebook);
  config_library_tab_create (vbox);

  /* -- Layer names and groups -- */
  gtk_tree_store_append (model, &iter, NULL);
  gtk_tree_store_set (model, &iter, CONFIG_NAME_COLUMN, _("Layers"), -1);
  vbox = config_page_create (model, &iter, config_notebook);
  config_layers_tab_create (vbox);


  /* -- Colors -- */
  gtk_tree_store_append (model, &iter, NULL);
  gtk_tree_store_set (model, &iter, CONFIG_NAME_COLUMN, _("Colors"), -1);
  vbox = config_page_create (model, &iter, config_notebook);
  config_colors_tab_create (vbox);


  /* -- Fonts -- */
  gtk_tree_store_append (model, &iter, NULL);
  gtk_tree_store_set (model, &iter, CONFIG_NAME_COLUMN, _("Fonts"), -1);
  vbox = config_page_create (model, &iter, config_notebook);
  config_fonts_tab_create (vbox);

    
  /* Create the tree view
   */
  treeview =
    GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL (model)));
  g_object_unref (G_OBJECT (model));	/* Don't need the model anymore */

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (NULL, renderer,
						     "text",
						     CONFIG_NAME_COLUMN,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_container_add (GTK_CONTAINER (scrolled), GTK_WIDGET (treeview));


  select = gtk_tree_view_get_selection (treeview);
  gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (select), "changed",
		    G_CALLBACK (config_selection_changed_cb), NULL);


  hbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing (GTK_BOX (hbox), 5);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

  button = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_widget_set_can_default (button, TRUE);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (config_close_cb), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);

  gtk_widget_show_all (config_window);
}
