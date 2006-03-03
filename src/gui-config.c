/* $Id$ */
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* This file written by Bill Wilson for the PCB Gtk port.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "gui.h"
#include "global.h"
#include "action.h"
#include "change.h"
#include "file.h"
#include "error.h"
#include "draw.h"
#include "set.h"

#include <locale.h>

typedef struct
	{
	gchar		*name;
	gint		*value;
	gchar		*initial;
	}
	ConfigSettingsValues;

typedef struct
	{
	gchar		*name;
	gdouble		*value;
	gchar		*initial;
	}
	ConfigSettingsReals;

typedef struct
	{
	gchar		*name;
	gchar		**string;
	gchar		*initial;
	}
	ConfigSettingsStrings;

typedef struct
	{
	gchar		*name;
	GdkColor	*color;
	gchar		*initial;
	}
	ConfigSettingsColors;

static gchar	*library_newlib;

static ConfigSettingsReals	config_settings_reals[] =
	{
	{"grid-increment-mil",	&Settings.grid_increment_mil,	"5.0"},
	{"grid-increment-mm",	&Settings.grid_increment_mm,	"0.1"},
	{"size-increment-mil",	&Settings.size_increment_mil,	"10.0"},
	{"size-increment-mm",	&Settings.size_increment_mm,	"0.2"},
	{"line-increment-mil",	&Settings.line_increment_mil,	"5.0"},
	{"line-increment-mm",	&Settings.line_increment_mm,	"0.1"},
	{"clear-increment-mil",	&Settings.clear_increment_mil,	"2.0"},
	{"clear-increment-mm",	&Settings.clear_increment_mm,	"0.05"},
	{"zoom",				&Settings.Zoom,					"3.0"},
	};

static ConfigSettingsValues	config_settings_values[] =
	{
	/* booleans */
	{"gui-compact-horizontal", &Settings.gui_compact_horizontal, "0"},
	{"gui-title-window",	&Settings.gui_title_window,		"0"},
	{"use-command-window",	&Settings.use_command_window,	"1"},
	{"grid-units-mm",		&Settings.grid_units_mm,		"0"},
	{"all-direction-lines",	&Settings.AllDirectionLines,	"0"},
	{"use-log-window",		&Settings.UseLogWindow,			"1"},
	{"unique-names",		&Settings.UniqueNames,			"1"},
	{"snap-pin",			&Settings.SnapPin,				"1"},
	{"show-number",			&Settings.ShowNumber,			"0"},
	{"clear-line",			&Settings.ClearLine,			"1"},
	{"save-in-tmp",			&Settings.SaveInTMP,			"0"},
	{"reset-after-element",	&Settings.ResetAfterElement,	"1"},
	{"ring-bell-finished",	&Settings.RingBellWhenFinished,	"0"},
	{"save-last-command",	&Settings.SaveLastCommand,		"0"},
	{"stipple-polygons",	&Settings.StipplePolygons,		"0"},


	{"history-size",		&Settings.HistorySize,			"15"},
	{"text-scale",			&Settings.TextScale,			"100"},
	{"via-thickness",		&Settings.ViaThickness,			"6000"},
	{"via-drilling-hole",	&Settings.ViaDrillingHole,		"2800"},
	{"volume",				&Settings.Volume,				"100"},
	{"alignment-distance",	&Settings.AlignmentDistance,	"200"},
	{"backup-interval",		&Settings.BackupInterval,		"60"},
/*	{"grid",				&Settings.Grid,					"10"}, */
	{"line-thickness",		&Settings.LineThickness,		"1000"},
	{"rat-thickness",		&Settings.RatThickness,			"1000"},
	{"pinout-offset-x",		&Settings.PinoutOffsetX,		"100"},
	{"pinout-offset-y",		&Settings.PinoutOffsetY,		"100"},
	{"pinout-text-offset-x",&Settings.PinoutTextOffsetX,	"8"},
	{"pinout-text-offset-y",&Settings.PinoutTextOffsetY,	"8"},

	{"PCB-width",			&Settings.pcb_width,			"800"},
	{"PCB-height",			&Settings.pcb_height,			"600"},
	{"log-window-width",	&Settings.log_window_width,		"500"},
	{"log-window-height",	&Settings.log_window_height,	"300"},
	{"library-window-height",&Settings.library_window_height, "300"},
	{"netlist-window-height",&Settings.netlist_window_height, "300"},
	{"keyref-window-width",	&Settings.keyref_window_width,	"300"},
	{"keyref-window-height",&Settings.keyref_window_height,	"300"},
	{"selected-print-device",&Settings.selected_print_device, "0"},
	{"selected-media",		&Settings.selected_media,		"4"},

	{"bloat",				&Settings.Bloat,				"699"},
	{"shrink",				&Settings.Shrink,				"400"},
	{"min-width",			&Settings.minWid,				"800"},
	{"min-silk",			&Settings.minSlk,				"800"},
	{"min-drill",			&Settings.minDrill,				"1000"},
	{"min-ring",			&Settings.minRing,				"750"},

	{"default-PCB-width",	&Settings.MaxWidth,				"600000"},
	{"default-PCB-height",	&Settings.MaxHeight,			"500000"},
	};

static ConfigSettingsStrings	config_settings_strings[] =
	{
	/* These two can be overriden via rc file or command line args.
	*/
	{"groups",				&Settings.Groups,
				"1,c:2,s:3:4:5:6:7:8"},
	{"route-styles",		&Settings.Routes,
				"Signal,1000,3600,2000,1000:Power,2500,6000,3500,1000"
				":Fat,4000,6000,3500,1000:Skinny,600,2402,1181,600" },

	{"fab-author",			&Settings.FabAuthor,			"" },

	{"pinout-font",			&Settings.PinoutFont,			"Sans 10"},

	{"layer-groups",		&Settings.Groups,		"1,c:2,s:3:4:5:6:7:8"},

	{"layer-name-1",		&Settings.DefaultLayerName[0],	N_("component")},
	{"layer-name-2",		&Settings.DefaultLayerName[1],	N_("solder")},
	{"layer-name-3",		&Settings.DefaultLayerName[2],	N_("GND")},
	{"layer-name-4",		&Settings.DefaultLayerName[3],	N_("power")},
	{"layer-name-5",		&Settings.DefaultLayerName[4],	N_("signal1")},
	{"layer-name-6",		&Settings.DefaultLayerName[5],	N_("signal2")},
	{"layer-name-7",		&Settings.DefaultLayerName[6],	N_("unused")},
	{"layer-name-8",		&Settings.DefaultLayerName[7],	N_("unused")},

	{"library-newlib",		&library_newlib,				""},

	{"size",				&Settings.Size,					"6000x5000"},
	{"color-file",			&Settings.color_file,			"" },
	};

static ConfigSettingsColors	config_settings_colors[] =
	{
	{"element-color",		&Settings.ElementColor,			"black"},
	{"via-color",			&Settings.ViaColor,				"gray50"},
	{"pin-color",			&Settings.PinColor,				"gray30"},
	{"rat-color",			&Settings.RatColor,				"DarkGoldenrod"},
	{"warn-color",			&Settings.WarnColor,			"hot pink"},
	{"off-limit-color",		&Settings.OffLimitColor,		"gray80"},
	{"invisible-objects-color", &Settings.InvisibleObjectsColor, "gray80"},
	{"invisible-mark-color",	&Settings.InvisibleMarkColor,	"gray70"},
	{"connected-color",		&Settings.ConnectedColor,		"green"},
	{"crosshair-color",		&Settings.CrosshairColor,		"red"},
	{"cross-color",			&Settings.CrossColor,			"yellow"},
	{"grid-color",			&Settings.GridColor,			"red"},
	{"mask-color",			&Settings.MaskColor,			"red"},
	{"background-color",	&Settings.BackgroundColor,		"gray90"},

	{"element-selected-color", &Settings.ElementSelectedColor,"cyan"},
	{"via-selected-color",	&Settings.ViaSelectedColor,		"cyan"},
	{"pin-selected-color",	&Settings.PinSelectedColor,		"cyan"},
	{"rat-selected-color",	&Settings.RatSelectedColor,		"cyan"},

	{"layer-color-1",			&Settings.LayerColor[0],		"brown4"},
	{"layer-color-2",			&Settings.LayerColor[1],		"RoyalBlue3"},
	{"layer-color-3",			&Settings.LayerColor[2],		"DodgerBlue4"},
	{"layer-color-4",			&Settings.LayerColor[3],		"OrangeRed3"},
	{"layer-color-5",			&Settings.LayerColor[4],		"PaleGreen4"},
	{"layer-color-6",			&Settings.LayerColor[5],		"burlywood4"},
	{"layer-color-7",			&Settings.LayerColor[6],		"turquoise4"},
	{"layer-color-8",			&Settings.LayerColor[7],	"forest green"},
	{"layer-selected-color-1",	&Settings.LayerSelectedColor[0], "cyan"},
	{"layer-selected-color-2",	&Settings.LayerSelectedColor[1], "cyan"},
	{"layer-selected-color-3",	&Settings.LayerSelectedColor[2], "cyan"},
	{"layer-selected-color-4",	&Settings.LayerSelectedColor[3], "cyan"},
	{"layer-selected-color-5",	&Settings.LayerSelectedColor[4], "cyan"},
	{"layer-selected-color-6",	&Settings.LayerSelectedColor[5], "cyan"},
	{"layer-selected-color-7",	&Settings.LayerSelectedColor[6], "cyan"},
	{"layer-selected-color-8",	&Settings.LayerSelectedColor[7], "cyan"},
	};


  /* Not used anywhere.  Just put some strings into translators .po files
  */
gchar	*gui_config_translation_helpers[] =
	{
	N_("GND-solder"),
	N_("GND-component"),
	N_("Vcc-solder"),
	N_("Vcc-component"),
	N_("Signal"),
	N_("Power"),
	N_("Fat"),
	N_("Skinny"),
	N_("(unknown)"),
	};

#define PCB_CONFIG_DIR	".pcb"
#define PCB_CONFIG_FILE	"preferences"
#define PCB_COLORS_DIR	"colors"

static gchar	*config_dir,
				*color_dir;

static FILE *
config_file_open(gchar *mode)
	{
	FILE		*f;
	gchar		*homedir, *fname;


	homedir = (gchar *) g_get_home_dir();
	if (!homedir)
		{
		g_message("config_file_open: Can't get home directory!");
		return NULL;
		}

	if (!config_dir)
		{
		config_dir = 
			g_build_path(G_DIR_SEPARATOR_S, homedir, PCB_CONFIG_DIR, NULL);
		if (   !g_file_test(config_dir, G_FILE_TEST_IS_DIR)
			&& mkdir(config_dir, 0755) < 0
		   )
			{
			g_message("config_file_open: Can't make \"%s\" directory!",
						config_dir);
			g_free(config_dir);
			config_dir = NULL;
			return NULL;
			}
		}

	if (!color_dir)	/* Convenient to make the color dir here */
		{
		color_dir =
			g_build_path(G_DIR_SEPARATOR_S, config_dir, PCB_COLORS_DIR, NULL);
		if (!g_file_test(color_dir, G_FILE_TEST_IS_DIR))
			{
			if (mkdir(color_dir, 0755) < 0)
				{
				g_message("config_file_open: Can't make \"%s\" directory!",
							color_dir);
				g_free(color_dir);
				color_dir = NULL;
				}
			fname = g_build_path(G_DIR_SEPARATOR_S,
					color_dir, "Default", NULL);
			dup_string(&Settings.color_file, fname);
			g_free(fname);
			}
		}

	fname = g_build_path(G_DIR_SEPARATOR_S, config_dir, PCB_CONFIG_FILE, NULL);
	f = fopen(fname, mode);

	g_free(fname);
	return f;
	}

enum ConfigBlocks
	{
	VALUES_BLOCK,
	REALS_BLOCK,
	STRINGS_BLOCK,
	COLORS_BLOCK,
	LISTS_BLOCK,
	UNKNOWN_BLOCK
	};

static void
config_file_read(void)
	{
	FILE					*f;
	ConfigSettingsValues 	*cv;
	ConfigSettingsReals 	*cr;
	ConfigSettingsStrings	*cs;
	gchar					*s, *ss, buf[512], option[64], arg[512];
	enum ConfigBlocks		cb = UNKNOWN_BLOCK;
	gint					i;
	struct lconv			*lc;
	gchar					locale_point,
							*comma_point, *period_point;

	if ((f = config_file_open("r")) == NULL)
		return;

	/* Until LC_NUMERIC is totally resolved, check if we need to decimal
	|  point convert.  Ultimately, data files will be POSIX and gui
	|  presentation (hence the config file reals) will be in the users locale.
	*/
	lc = localeconv();
	locale_point = *lc->decimal_point;

	buf[0] = '\0';
	while (fgets(buf, sizeof(buf), f))
		{
		s = buf;
		while (*s == ' ' || *s == '\t')
			++s;
		if (!*s || *s == '\n' || *s == '#')
			continue;
		if ((ss = strchr(s, '\n')) != NULL)
			*ss = '\0';
		if (*s == '[')
			{
			if (!strcmp(s, "[values]"))
				cb = VALUES_BLOCK;
			else if (!strcmp(s, "[reals]"))
				cb = REALS_BLOCK;
			else if (!strcmp(s, "[strings]"))
				cb = STRINGS_BLOCK;
			else if (!strcmp(s, "[colors]"))	/* Now in separate file! */
				cb = COLORS_BLOCK;
			else if (!strcmp(s, "[lists]"))
				cb = LISTS_BLOCK;
			else
				cb = UNKNOWN_BLOCK;
			continue;
			}
		if (cb == UNKNOWN_BLOCK)
			continue;
		sscanf(s, "%63s %511[^\n]", option, arg);

		s = option;		/* Strip trailing ':' or '=' */
		while (*s && *s != ':' && *s != '=')
			++s;
		*s = '\0';

		s = arg;		/* Strip leading ':', '=', and whitespace */
		while (*s == ' ' || *s == '\t' || *s == ':' || *s == '=')
			++s;

		switch (cb)
			{
			case VALUES_BLOCK:
				for (i = 0; i < G_N_ELEMENTS(config_settings_values); ++i)
					{
					cv = &config_settings_values[i];
					if (strcmp(option, cv->name))
						continue;
					*cv->value = atoi(s);
					break;
					}
				break;
			case REALS_BLOCK:
				for (i = 0; i < G_N_ELEMENTS(config_settings_reals); ++i)
					{
					cr = &config_settings_reals[i];
					if (strcmp(option, cr->name))
						continue;

					/* Hopefully temporary locale decimal point check:
					*/
					comma_point = strrchr(s, ',');
					period_point = strrchr(s, '.');
					if (comma_point && *comma_point != locale_point)
						*comma_point = locale_point;
					else if (period_point && *period_point != locale_point)
						*period_point = locale_point;
					*cr->value = atof(s);
					break;
					}
				break;
			case STRINGS_BLOCK:
				for (i = 0; i < G_N_ELEMENTS(config_settings_strings); ++i)
					{
					cs = &config_settings_strings[i];
					if (strcmp(option, cs->name))
						continue;
					dup_string(cs->string, s);
					break;
					}
				break;
			default:
				break;
			}
		}
	}

void
config_file_write(void)
	{
	FILE					*f;
	ConfigSettingsValues 	*cv;
	ConfigSettingsReals 	*cr;
	ConfigSettingsStrings	*cs;
	gint					i;

	if ((f = config_file_open("w")) == NULL)
		{
		return;
		}

	fprintf(f, "### PCB configuration file. ###\n");

	fprintf(f, "\n[values]\n");
	for (i = 0; i < G_N_ELEMENTS(config_settings_values); ++i)
		{
		cv = &config_settings_values[i];
		fprintf(f, "%s =\t%d\n", cv->name, *cv->value);
		}

	fprintf(f, "\n[reals]\n");
	for (i = 0; i < G_N_ELEMENTS(config_settings_reals); ++i)
		{
		cr = &config_settings_reals[i];
		fprintf(f, "%s =\t%f\n", cr->name, *cr->value);
		}

	fprintf(f, "\n[strings]\n");
	for (i = 0; i < G_N_ELEMENTS(config_settings_strings); ++i)
		{
		cs = &config_settings_strings[i];
		fprintf(f, "%s =\t%s\n", cs->name, *cs->string);
		}

	fclose(f);

	Settings.config_modified = FALSE;
	}

static void
config_colors_write(gchar *path)
	{
	FILE					*f;
	ConfigSettingsColors	*cc;
	gchar					*color_name;
	gint					i;

	if ((f = fopen(path, "w")) == NULL)
		return;
	for (i = 0; i < G_N_ELEMENTS(config_settings_colors); ++i)
		{
		cc = &config_settings_colors[i];
		color_name = gui_get_color_name(cc->color);
		fprintf(f, "%s =\t%s\n", cc->name, color_name);
		g_free(color_name);
		}
	fclose(f);
	}

static gboolean
config_colors_read(gchar *path)
	{
	FILE					*f;
	ConfigSettingsColors	*cc;
	gchar					*s, buf[512], option[64], arg[512];
	gint					i;

	if ((f = fopen(path, "r")) == NULL)
		return FALSE;
	while (fgets(buf, sizeof(buf), f))
		{
		sscanf(buf, "%63s %511[^\n]", option, arg);
		s = option;     /* Strip trailing ':' or '=' */
		while (*s && *s != ':' && *s != '=')
			++s;
		*s = '\0';
		s = arg;        /* Strip leading ':', '=', and whitespace */
		while (*s == ' ' || *s == '\t' || *s == ':' || *s == '=')
			++s;

		for (i = 0; i < G_N_ELEMENTS(config_settings_colors); ++i)
			{
			cc = &config_settings_colors[i];
			if (!strcmp(option, cc->name))
				{
				gui_map_color_string(s, cc->color);
				break;
				}
			}
		}
	fclose(f);

	gui_layer_buttons_color_update();

	return TRUE;
	}


static void
config_color_file_read(void)
	{
	gchar	*path;

	if (!color_dir)
		return;

	/* Initialize the default color file if needed.
	*/
	path = g_build_path(G_DIR_SEPARATOR_S, color_dir, "Default", NULL);
	if (!g_file_test(path, G_FILE_TEST_EXISTS))
		config_colors_write(path);

	/* Use configured color file instead of default
	*/
	if (Settings.color_file && *Settings.color_file)
		dup_string(&path, Settings.color_file);
	else
		dup_string(&Settings.color_file, path);

	config_colors_read(path);
	g_free(path);
	}

void
config_settings_load(void)
	{
	ConfigSettingsValues 	*cv;
	ConfigSettingsReals 	*cr;
	ConfigSettingsStrings	*cs;
	ConfigSettingsColors	*cc;
	gint					i;

	/* Initialize all config settings to default values
	*/
	for (i = 0; i < G_N_ELEMENTS(config_settings_values); ++i)
		{
		cv = &config_settings_values[i];
		*cv->value = atoi(cv->initial);
		}
	for (i = 0; i < G_N_ELEMENTS(config_settings_reals); ++i)
		{
		cr = &config_settings_reals[i];
		*cr->value = atof(cr->initial);
		}
	for (i = 0; i < G_N_ELEMENTS(config_settings_strings); ++i)
		{
		cs = &config_settings_strings[i];
		*cs->string = g_strdup(cs->initial);
		}
	for (i = 0; i < G_N_ELEMENTS(config_settings_colors); ++i)
		{
		cc = &config_settings_colors[i];
		gui_map_color_string(cc->initial, cc->color);
		}

	/* XXX These look good for me, but could be made configurable.
	*/
	Settings.n_mode_button_columns = 3;
	Settings.small_layer_enable_label_markup = TRUE;

	config_file_read();
	config_color_file_read();

	gui_map_color_string("white", &Settings.WhiteColor);
	gui_map_color_string("black", &Settings.BlackColor);

	if (   Settings.LineThickness > MAX_LINESIZE
		|| Settings.LineThickness < MIN_LINESIZE
	   )
		Settings.LineThickness = 1000;

	if (   Settings.ViaThickness > MAX_PINORVIASIZE
		|| Settings.ViaThickness < MIN_PINORVIASIZE
	   )
		Settings.ViaThickness = 4000;

	if (Settings.ViaDrillingHole <= 0)
		Settings.ViaDrillingHole =
				DEFAULT_DRILLINGHOLE * Settings.ViaThickness / 100;



	Settings.MaxWidth = MIN(MAX_COORD, MAX(Settings.MaxWidth, MIN_SIZE));
	Settings.MaxHeight = MIN(MAX_COORD, MAX(Settings.MaxHeight, MIN_SIZE));

/*	Settings.Volume = MIN (100, MAX (-100, Settings.Volume)); */

	Settings.Grid = 10;		/* XXX ConfigSettingsFloat ??? */

	ParseGroupString (Settings.Groups, &Settings.LayerGroups);
	ParseRouteString (Settings.Routes, &Settings.RouteStyle[0], 1);

	Output.font_desc = pango_font_description_from_string(Settings.PinoutFont);

	/* Make sure this is off.  When on it nearly kills the X server.
	*/
	Settings.StipplePolygons = FALSE;

	if (library_newlib && *library_newlib)
		gui_settings_add_to_newlib_list(library_newlib);	/* In main.c */
	}


/* =================== OK, now the gui stuff ======================
*/
static GtkWidget	*config_window;

  /* -------------- The General config page ----------------
  */
static gint		config_backup_interval_tmp;

static void
config_command_window_toggle_cb(GtkToggleButton *button, gpointer data)
	{
	gboolean		active = gtk_toggle_button_get_active(button);
	static gboolean	holdoff;

	if (holdoff)
		return;

	/* Can't toggle into command window mode if the status line command
	|  entry is active.
	*/
	if (gui->command_entry_status_line_active)
		{
		holdoff = TRUE;
		gtk_toggle_button_set_active(button, FALSE);
		holdoff = FALSE;
		return;
		}
	Settings.use_command_window = active;
	gui_command_use_command_window_sync();
	}


static void
config_compact_toggle_cb(GtkToggleButton *button, gpointer data)
	{
	gboolean	active = gtk_toggle_button_get_active(button);

	Settings.gui_compact_horizontal = active;
	if (active)
		{
		gtk_container_remove(GTK_CONTAINER(gui->top_hbox),
					gui->position_hbox);
		gtk_box_pack_end(GTK_BOX(gui->compact_vbox), gui->position_hbox,
					FALSE, FALSE, 0);
		}
	else
		{
		gtk_container_remove(GTK_CONTAINER(gui->compact_vbox),
					gui->position_hbox);
		gtk_box_pack_end(GTK_BOX(gui->top_hbox), gui->position_hbox,
					FALSE, FALSE, 0);
		}
	set_status_line_label();
	Settings.config_modified = TRUE;
	}

static void
config_title_window_cb(GtkToggleButton *button, gpointer data)
	{
	gboolean	active = gtk_toggle_button_get_active(button);

	Settings.gui_title_window = active;
	gui_output_set_name_label(gui->name_label_string);
	Settings.config_modified = TRUE;
	}

static void
config_general_toggle_cb(GtkToggleButton *button, gint	*setting)
	{
	*setting = gtk_toggle_button_get_active(button);
	Settings.config_modified = TRUE;
	}

static void
config_backup_spin_button_cb(GtkSpinButton *spin_button, gpointer data)
	{
	config_backup_interval_tmp = gtk_spin_button_get_value_as_int(spin_button);
	Settings.config_modified = TRUE;
	}

static void
config_history_spin_button_cb(GtkSpinButton *spin_button, gpointer data)
	{
	Settings.HistorySize = gtk_spin_button_get_value_as_int(spin_button);
	Settings.config_modified = TRUE;
	}

static void
config_general_tab_create(GtkWidget *tab_vbox)
	{
	GtkWidget	*vbox;

	config_backup_interval_tmp = Settings.BackupInterval;

	gtk_container_set_border_width(GTK_CONTAINER(tab_vbox), 6);

	vbox = gui_category_vbox(tab_vbox, _("Enables"),
				4, 2, TRUE, TRUE);
	gui_check_button_connected(vbox, NULL, Settings.UseLogWindow,
				TRUE, FALSE, FALSE, 2,
				config_general_toggle_cb, &Settings.UseLogWindow,
				_("Send messages to the log window instead of a popup"));

	gui_check_button_connected(vbox, NULL, Settings.use_command_window,
				TRUE, FALSE, FALSE, 2,
				config_command_window_toggle_cb, NULL,
				_("Use separate window for command entry"));

	gui_check_button_connected(vbox, NULL, Settings.gui_compact_horizontal,
				TRUE, FALSE, FALSE, 2,
				config_compact_toggle_cb, NULL,
				_("Compact horizontal top window for narrow screens"));

	gui_check_button_connected(vbox, NULL, Settings.gui_title_window,
				TRUE, FALSE, FALSE, 2,
				config_title_window_cb, NULL,
				_("Put layout name on the window title bar"));

#if 0
/* Works poorly */
	gui_check_button_connected(vbox, NULL, Settings.StipplePolygons,
				TRUE, FALSE, FALSE, 2,
				config_general_toggle_cb, &Settings.StipplePolygons,
				"Display polygons with a stipple pattern");
#endif

	vbox = gui_category_vbox(tab_vbox, _("Backups"),
				4, 2, TRUE, TRUE);
	gui_check_button_connected(vbox, NULL, Settings.SaveInTMP,
				TRUE, FALSE, FALSE, 2,
				config_general_toggle_cb, &Settings.SaveInTMP,
			_("If layout is modified at exit, save into /tmp/PCB.%i.save"));
	gui_spin_button(vbox, NULL, Settings.BackupInterval,
				0.0, 60 * 60, 60.0, 600.0, 0, 0,
				config_backup_spin_button_cb, NULL, FALSE,
				_("Seconds between auto backups to /tmp/PCB.%i.save\n"
				"(set to zero to disable auto backups)"));

	vbox = gui_category_vbox(tab_vbox, _("Misc"),
				4, 2, TRUE, TRUE);
	gui_spin_button(vbox, NULL, Settings.HistorySize,
				5.0, 25.0, 1.0, 1.0, 0, 0,
				config_history_spin_button_cb, NULL, FALSE,
				_("Number of commands to remember in the history list"));
	}


static gint
backup_timeout_cb(gpointer data)
	{
	Backup ();

	if (Settings.config_modified)
		config_file_write();

	return TRUE;	/* restarts timer */
	}

void
gui_config_start_backup_timer(void)
	{
	static gint	timeout_id = 0;

	if (timeout_id)
		gtk_timeout_remove(timeout_id);

	timeout_id = 0;
	if (Settings.BackupInterval > 0)
		timeout_id = gtk_timeout_add(1000 * Settings.BackupInterval,
					(GtkFunction) backup_timeout_cb, NULL);
	}

static void
config_general_apply(void)
	{
	if (config_backup_interval_tmp != Settings.BackupInterval)
		gui_config_start_backup_timer();
	}


  /* -------------- The Sizes config page ----------------
  */
#define	STEP0_SMALL_SIZE	(Settings.grid_units_mm ? 0.005 : 0.1)
#define	STEP1_SMALL_SIZE	(Settings.grid_units_mm ? 0.05 : 1.0)
#define	STEP0_SIZE			(Settings.grid_units_mm ? 5.0 : 100)
#define	STEP1_SIZE			(Settings.grid_units_mm ? 25.0 : 1000)
#define	SPIN_DIGITS			(Settings.grid_units_mm ? 3 : 1)

static GtkWidget	*config_sizes_vbox,
					*config_sizes_tab_vbox,
					*config_text_spin_button;

static GtkWidget	*use_board_size_default_button,
					*use_drc_sizes_default_button;

static gint			new_board_width,
					new_board_height;

static void
config_sizes_apply(void)
	{
	gboolean	active;

	active = gtk_toggle_button_get_active(
				GTK_TOGGLE_BUTTON(use_board_size_default_button));
	if (active)
		{
		Settings.MaxWidth = new_board_width;
		Settings.MaxHeight = new_board_height;
		Settings.config_modified = TRUE;
		}

	active = gtk_toggle_button_get_active(
				GTK_TOGGLE_BUTTON(use_drc_sizes_default_button));
	if (active)
		{
		Settings.Bloat = PCB->Bloat;
		Settings.Shrink = PCB->Shrink;
		Settings.minWid = PCB->minWid;
		Settings.minSlk = PCB->minSlk;
		Settings.minDrill = PCB->minDrill;
		Settings.minRing = PCB->minRing;
		Settings.config_modified = TRUE;
		}

	if (PCB->MaxWidth != new_board_width || PCB->MaxHeight != new_board_height)
		ChangePCBSize(new_board_width, new_board_height);
	}

static void
text_spin_button_cb(GtkSpinButton *spin, gint *dst)
	{
	*dst = gtk_spin_button_get_value_as_int(spin);
	Settings.config_modified = TRUE;
	set_status_line_label();
	}


static void
size_spin_button_cb(GtkSpinButton *spin, gint *dst)
	{
	gdouble	value;

	value = gtk_spin_button_get_value(spin);
	*dst = TO_PCB_UNITS(value);
	Settings.config_modified = TRUE;
	}

static void
config_sizes_tab_create(GtkWidget *tab_vbox)
	{
	GtkWidget	*table, *vbox, *hbox, *label;
	gchar		*str;

	/* Need a vbox we can destroy if user changes grid units.
	*/
	if (!config_sizes_vbox)
		{
		vbox = gtk_vbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(tab_vbox), vbox, FALSE, FALSE, 0);
		gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
		config_sizes_vbox = vbox;
		config_sizes_tab_vbox = tab_vbox;
		}

	str = g_strdup_printf(_("<b>%s</b> grid units are selected"),
			Settings.grid_units_mm ? "mm" : "mil");
	label = gtk_label_new("");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), str);
	g_free(str);

	gtk_box_pack_start(GTK_BOX(config_sizes_vbox), label, FALSE, FALSE, 4);


	/* ---- Board Size ---- */
	vbox = gui_category_vbox(config_sizes_vbox, _("Board Size"),
				4, 2, TRUE, TRUE);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	table = gtk_table_new(2, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);

	new_board_width = PCB->MaxWidth;
	new_board_height = PCB->MaxHeight;
	gui_table_spin_button(table, 0, 0, NULL,
			FROM_PCB_UNITS(PCB->MaxWidth),
			FROM_PCB_UNITS(MIN_SIZE), FROM_PCB_UNITS(MAX_COORD),
			STEP0_SIZE, STEP1_SIZE,
			SPIN_DIGITS, 0, size_spin_button_cb,
			&new_board_width, FALSE, _("Width"));

	gui_table_spin_button(table, 1, 0, NULL,
			FROM_PCB_UNITS(PCB->MaxHeight),
			FROM_PCB_UNITS(MIN_SIZE), FROM_PCB_UNITS(MAX_COORD),
			STEP0_SIZE, STEP1_SIZE,
			SPIN_DIGITS, 0, size_spin_button_cb,
			&new_board_height, FALSE, _("Height"));
	gui_check_button_connected(vbox, &use_board_size_default_button, FALSE,
			TRUE, FALSE, FALSE, 0, NULL, NULL,
			_("Use this board size as the default for new layouts"));

	/* ---- Text Scale ---- */
	vbox = gui_category_vbox(config_sizes_vbox, _("Text Scale"),
				4, 2, TRUE, TRUE);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	table = gtk_table_new(4, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);

	gui_table_spin_button(table, 0, 0, &config_text_spin_button,
			Settings.TextScale,
			MIN_TEXTSCALE, MAX_TEXTSCALE,
			10.0, 10.0,
			0, 0, text_spin_button_cb,
			&Settings.TextScale, FALSE, "%");


	/* ---- DRC Sizes ---- */
	vbox = gui_category_vbox(config_sizes_vbox, _("Design Rule Checking"),
				4, 2, TRUE, TRUE);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	table = gtk_table_new(4, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);

	gui_table_spin_button(table, 0, 0, NULL,
			FROM_PCB_UNITS(PCB->Bloat),
			FROM_PCB_UNITS(MIN_DRC_VALUE), FROM_PCB_UNITS(MAX_DRC_VALUE),
			STEP0_SMALL_SIZE, STEP1_SMALL_SIZE,
			SPIN_DIGITS, 0, size_spin_button_cb,
			&PCB->Bloat, FALSE, _("Minimum copper spacing"));

	gui_table_spin_button(table, 1, 0, NULL,
			FROM_PCB_UNITS(PCB->minWid),
			FROM_PCB_UNITS(MIN_DRC_VALUE), FROM_PCB_UNITS(MAX_DRC_VALUE),
			STEP0_SMALL_SIZE, STEP1_SMALL_SIZE,
			SPIN_DIGITS, 0, size_spin_button_cb,
			&PCB->minWid, FALSE, _("Minimum copper width"));

	gui_table_spin_button(table, 2, 0, NULL,
			FROM_PCB_UNITS(PCB->Shrink),
			FROM_PCB_UNITS(MIN_DRC_VALUE), FROM_PCB_UNITS(MAX_DRC_VALUE),
			STEP0_SMALL_SIZE, STEP1_SMALL_SIZE,
			SPIN_DIGITS, 0, size_spin_button_cb,
			&PCB->Shrink, FALSE, _("Minimum touching copper overlap"));

	gui_table_spin_button(table, 3, 0, NULL,
			FROM_PCB_UNITS(PCB->minSlk),
			FROM_PCB_UNITS(MIN_DRC_SILK), FROM_PCB_UNITS(MAX_DRC_SILK),
			STEP0_SMALL_SIZE, STEP1_SMALL_SIZE,
			SPIN_DIGITS, 0, size_spin_button_cb,
			&PCB->minSlk, FALSE, _("Minimum silk width"));

	gui_table_spin_button(table, 4, 0, NULL,
			FROM_PCB_UNITS(PCB->minDrill),
			FROM_PCB_UNITS(MIN_DRC_DRILL), FROM_PCB_UNITS(MAX_DRC_DRILL),
			STEP0_SMALL_SIZE, STEP1_SMALL_SIZE,
			SPIN_DIGITS, 0, size_spin_button_cb,
			&PCB->minDrill, FALSE, _("Minimum drill size"));

	gui_table_spin_button(table, 5, 0, NULL,
			FROM_PCB_UNITS(PCB->minRing),
			FROM_PCB_UNITS(MIN_DRC_RING), FROM_PCB_UNITS(MAX_DRC_RING),
			STEP0_SMALL_SIZE, STEP1_SMALL_SIZE,
			SPIN_DIGITS, 0, size_spin_button_cb,
			&PCB->minRing, FALSE, _("Minimum annular ring on drilled pads/vias"));

	gui_check_button_connected(vbox, &use_drc_sizes_default_button, FALSE,
			TRUE, FALSE, FALSE, 0, NULL, NULL,
			_("Use DRC values as the default for new layouts"));

	gtk_widget_show_all(config_sizes_vbox);
	}


  /* -------------- The Increments config page ----------------
  */
  /* Increment/decrement values are kept in mil and mm units and not in
  |  PCB units.
  */
static GtkWidget	*config_increments_vbox,
					*config_increments_tab_vbox;

static void
increment_spin_button_cb(GtkSpinButton *spin, gdouble *dst)
	{
	gdouble	value;

	value = gtk_spin_button_get_value(spin);
	*dst = value;		/* Not using PCB units */

	if (   dst == &Settings.size_increment_mm
	    || dst == &Settings.size_increment_mil
	   )
		gui_change_selected_update_menu_actions();
	Settings.config_modified = TRUE;
	}

static void
config_increments_tab_create(GtkWidget *tab_vbox)
	{
	GtkWidget	*vbox, *label;
	gdouble		*target;
	gchar		*str;

	/* Need a vbox we can destroy if user changes grid units.
	*/
	if (!config_increments_vbox)
		{
		vbox = gtk_vbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(tab_vbox), vbox, FALSE, FALSE, 0);
		gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
		config_increments_vbox = vbox;
		config_increments_tab_vbox = tab_vbox;
		}

	str = g_strdup_printf(
		_("Increment/Decrement values to use in <b>%s</b> units mode.\n"),
			Settings.grid_units_mm ? "mm" : "mil");
	label = gtk_label_new("");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), str);
	gtk_box_pack_start(GTK_BOX(config_increments_vbox), label,
			FALSE, FALSE, 4);
	g_free(str);


	/* ---- Grid Increment/Decrement ---- */
	vbox = gui_category_vbox(config_increments_vbox,
				_("Grid Increment/Decrement"), 4, 2, TRUE, TRUE);

	/* Grid increment spin button ('g' and '<shift>g').  For mil
	|  units, range from 1.0 to 25.0.  For mm, range from 0.01 to 1.0
	|  Step sizes of 5 mil or .05 mm, and .01 mm precision or 1 mil precision.
	*/
	target = Settings.grid_units_mm ?
				&Settings.grid_increment_mm : &Settings.grid_increment_mil;
	gui_spin_button(vbox, NULL,
			GRID_UNITS_VALUE(Settings.grid_increment_mm,
						Settings.grid_increment_mil),
			GRID_UNITS_VALUE(0.01, 1.0), GRID_UNITS_VALUE(1.0, 25.0),
			GRID_UNITS_VALUE(0.05, 5.0), GRID_UNITS_VALUE(0.05, 5.0),
			GRID_UNITS_VALUE(2, 0), 0, increment_spin_button_cb,
			target, FALSE, _("For 'g' and '<shift>g' grid change actions"));


	/* ---- Size Increment/Decrement ---- */
	vbox = gui_category_vbox(config_increments_vbox,
				_("Size Increment/Decrement"), 4, 2, TRUE, TRUE);

	/* Size increment spin button ('s' and '<shift>s').  For mil
	|  units, range from 1.0 to 10.0.  For mm, range from 0.01 to 0.5
	|  Step sizes of 1 mil or .01 mm, and .01 mm precision or 1 mil precision.
	*/
	target = Settings.grid_units_mm ?
				&Settings.size_increment_mm : &Settings.size_increment_mil;
	gui_spin_button(vbox, NULL,
			GRID_UNITS_VALUE(Settings.size_increment_mm,
						Settings.size_increment_mil),
			GRID_UNITS_VALUE(0.01, 1.0), GRID_UNITS_VALUE(0.5, 10.0),
			GRID_UNITS_VALUE(0.01, 1.0), GRID_UNITS_VALUE(0.05, 5.0),
			GRID_UNITS_VALUE(2, 0), 0, increment_spin_button_cb,
			target, FALSE, _("For 's' and '<shift>s' size change actions\n"
							"on lines, pads, pins, text and drill hole\n"
							"(with '<control>s' and '<shift><control>s'"));

	/* ---- Line Increment/Decrement ---- */
	vbox = gui_category_vbox(config_increments_vbox,
				_("Line Increment/Decrement"), 4, 2, TRUE, TRUE);

	/* Line increment spin button ('l' and '<shift>l').  For mil
	|  units, range from 0.5 to 10.0.  For mm, range from 0.005 to 0.5
	|  Step sizes of 0.5 mil or .005 mm, and .001 mm precision or 0.1 mil
	|  precision.
	*/
	target = Settings.grid_units_mm ?
				&Settings.line_increment_mm : &Settings.line_increment_mil;
	gui_spin_button(vbox, NULL,
			GRID_UNITS_VALUE(Settings.line_increment_mm,
						Settings.line_increment_mil),
			GRID_UNITS_VALUE(0.005, 0.5), GRID_UNITS_VALUE(0.5, 10.0),
			GRID_UNITS_VALUE(0.005, 0.5), GRID_UNITS_VALUE(0.05, 5.0),
			GRID_UNITS_VALUE(3, 1), 0, increment_spin_button_cb,
			target, FALSE,
				_("For 'l' and '<shift>l' routing line width change actions"));

	/* ---- Clear Increment/Decrement ---- */
	vbox = gui_category_vbox(config_increments_vbox,
				_("Clear Increment/Decrement"), 4, 2, TRUE, TRUE);

	/* Clear increment spin button ('l' and '<shift>l').  For mil
	|  units, range from 0.5 to 10.0.  For mm, range from 0.005 to 0.5
	|  Step sizes of 0.5 mil or .005 mm, and .001 mm precision or 0.1 mil
	|  precision.
	*/
	target = Settings.grid_units_mm ?
				&Settings.clear_increment_mm : &Settings.clear_increment_mil;
	gui_spin_button(vbox, NULL,
			GRID_UNITS_VALUE(Settings.clear_increment_mm,
						Settings.clear_increment_mil),
			GRID_UNITS_VALUE(0.005, 0.5), GRID_UNITS_VALUE(0.5, 10.0),
			GRID_UNITS_VALUE(0.005, 0.5), GRID_UNITS_VALUE(0.05, 5.0),
			GRID_UNITS_VALUE(3, 1), 0, increment_spin_button_cb,
			target, FALSE,
			_("For 'k' and '<shift>k' line clearance inside polygon size\n"
			  "change actions"));


	gtk_widget_show_all(config_increments_vbox);
	}

  /* -------------- The Library config page ----------------
  */
static GtkWidget	*library_newlib_entry;

static void
config_library_apply(void)
	{
	if (dup_string(&library_newlib, gui_entry_get_text(library_newlib_entry)))
		Settings.config_modified = TRUE;
	}

static void
config_library_tab_create(GtkWidget *tab_vbox)
	{
	GtkWidget	*vbox, *label, *entry;

	gtk_container_set_border_width(GTK_CONTAINER(tab_vbox), 6);
	vbox = gui_category_vbox(tab_vbox, _("Element Directories"),
				4, 2, TRUE, TRUE);
	label = gtk_label_new("");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label),

		_("<small>Enter a colon separated list of custom top level\n"
		"element directories.  For example:\n"
		"\t<b>~/gaf/pcb-elements:packages:/usr/local/pcb-elements</b>\n"
		"Elements should be organized into subdirectories below each\n"
		"top level directory.  Restart program for changes to take effect."
		"</small>"));

	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	entry = gtk_entry_new();
	library_newlib_entry = entry;
	gtk_entry_set_text(GTK_ENTRY(entry), library_newlib);
	gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 4);
	}


  /* -------------- The Layers Group config page ----------------
  */
static GtkWidget		*layer_entry[MAX_LAYER];
static GtkWidget		*group_button[MAX_LAYER + 2][MAX_LAYER];
static GtkWidget		*use_layer_default_button;

static gint				config_layer_group[MAX_LAYER + 2];

static LayerGroupType	layer_groups,	/* Working copy */
						*lg_monitor;	/* Keep track if our working copy */
										/* needs to be changed (new layout)*/

static gboolean			groups_modified, groups_holdoff, layers_applying;

static gchar	*layer_info_text[] =
{
N_("<h>Layer Names\n"),
N_("You may enter layer names for the layers drawn on the screen.\n"
   "The special 'component side' and 'solder side' are layers which\n"
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
   "as the 'component side' and 'solder side' printout layers.  Then\n"
   "groups such as signals, ground, and supply traces can be color\n"
   "coded on the screen while printing as a single layer.  For this\n"
   "you would select buttons and enter names on the Setup page to\n"
   "structure four layer groups similar to this:\n"),
"\n",
N_("<b>Group 1:"),
"\n\t",
N_("solder"),
"\n\t",
N_("GND-solder"),
"\n\t",
N_("Vcc-solder"),
"\n\t",
N_("solder side"),
"\n",
N_("<b>Group 2:"),
"\n\t",
N_("component"),
"\n\t",
N_("GND-component"),
"\n\t",
N_("Vcc-component"),
"\n\t",
N_("component side"),
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
config_layer_groups_radio_button_cb(GtkToggleButton *button, gpointer data)
	{
	gint	layer = GPOINTER_TO_INT(data) >> 8;
	gint	group = GPOINTER_TO_INT(data) & 0xff;

	if (!gtk_toggle_button_get_active(button) || groups_holdoff)
		return;
	config_layer_group[layer] = group;
	groups_modified = TRUE;
	Settings.config_modified = TRUE;
	}

  /* Construct a layer group string.  Follow logic in WritePCBDataHeader(),
  |  but use g_string functions.
  */
static gchar *
make_layer_group_string(LayerGroupType *lg)
	{
	GString	*string;
	gint	group, entry, layer;

	string = g_string_new("");

	for (group = 0; group < MAX_LAYER; group++)
		{
		if (lg->Number[group] == 0)
			continue;
		for (entry = 0; entry < lg->Number[group]; entry++)
			{
			layer = lg->Entries[group][entry];
			if (layer == MAX_LAYER + COMPONENT_LAYER)
				string = g_string_append(string, "c");
			else if (layer == MAX_LAYER + SOLDER_LAYER)
				string = g_string_append(string, "s");
			else
				g_string_append_printf(string, "%d", layer +1);

			if (entry != lg->Number[group] - 1)
				string = g_string_append(string, ",");
			}
		if (group != MAX_LAYER - 1)
			string = g_string_append(string, ":");
		}
	return g_string_free(string, FALSE);	/* Don't free string->str */
	}

static void
config_layers_apply(void)
	{
	LayerType	*layer;
	gchar		*s;
	gint		group, i;
	gint		componentgroup = 0, soldergroup = 0;
	gboolean	use_as_default, layers_modified = FALSE;

	use_as_default = gtk_toggle_button_get_active(
				GTK_TOGGLE_BUTTON(use_layer_default_button));

	/* Get each layer name entry and dup if modified into the PCB layer names
	|  and, if to use as default, the Settings layer names.
	*/
	for (i = 0; i < MAX_LAYER; ++i)
		{
		layer = &PCB->Data->Layer[i];
		s = gui_entry_get_text(layer_entry[i]);
		if (dup_string(&layer->Name, s))
			layers_modified = TRUE;
		if (use_as_default && dup_string(&Settings.DefaultLayerName[i], s))
			Settings.config_modified = TRUE;
			
		}
	/* Layer names can be changed from the menus and that can update the
	|  config.  So holdoff the loop.
	*/
	layers_applying = TRUE;
	if (layers_modified)
		gui_layer_buttons_update();
	layers_applying = FALSE;

	if (groups_modified)	/* If any group radio buttons were toggled. */
		{
		/* clear all entries and read layer by layer
		*/
		for (group = 0; group < MAX_LAYER; group++)
	    	layer_groups.Number[group] = 0;

		for (i = 0; i < MAX_LAYER + 2; i++)
			{
			group = config_layer_group[i] - 1;
			layer_groups.Entries[group][layer_groups.Number[group]++] = i;

			if (i == MAX_LAYER + COMPONENT_LAYER)
				componentgroup = group;
			else if (i == MAX_LAYER + SOLDER_LAYER)
				soldergroup = group;
			}

		/* do some cross-checking
		|  solder-side and component-side must be in different groups
		|  solder-side and component-side must not be the only one in the group
		*/
		if (   layer_groups.Number[soldergroup] <= 1
		    || layer_groups.Number[componentgroup] <= 1
		   )
			{
			Message (
		_("Both 'solder side' or 'component side' layers must have at least\n"
						"\tone other layer in their group.\n"));
			return;
			}
		else if (soldergroup == componentgroup)
			{
			Message (
		_("The 'solder side' and 'component side' layers are not allowed\n"
						"\tto be in the same layer group #\n"));
			return;
			}
		PCB->LayerGroups = layer_groups;
		ClearAndRedrawOutput();
		groups_modified = FALSE;
		}
	if (use_as_default)
		{
		s = make_layer_group_string(&PCB->LayerGroups);
		if (dup_string(&Settings.Groups, s))
			{
			ParseGroupString (Settings.Groups, &Settings.LayerGroups);
			Settings.config_modified = TRUE;
			}
		g_free(s);
		}
	}

static void
config_layer_group_button_state_update(void)
	{
	gint	g, i;

	/* Set button active corresponding to layer group state.
	*/
	groups_holdoff = TRUE;
	for (g = 0; g < MAX_LAYER; g++)
		for (i = 0; i < layer_groups.Number[g]; i++)
			{
/*			printf("layer %d in group %d\n", layer_groups.Entries[g][i], g +1); */
			config_layer_group[layer_groups.Entries[g][i]] = g + 1;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
					group_button[layer_groups.Entries[g][i]] [g]), TRUE);
			}
	groups_holdoff = FALSE;
	}

static void
config_layers_tab_create(GtkWidget *tab_vbox)
	{
	GtkWidget	*tabs, *vbox, *table, *button, *label, *text, *sep;
	GSList		*group;
	gchar		buf[32], *name;
	gint		layer, i;

	tabs = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(tab_vbox), tabs, TRUE, TRUE, 0);

/* -- Setup tab */
	vbox = gui_notebook_page(tabs, _("Setup"), 0, 6);

	table = gtk_table_new(MAX_LAYER + 3, MAX_LAYER + 1, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);

	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

	layer_groups = PCB->LayerGroups;	/* working copy */
	lg_monitor = &PCB->LayerGroups;		/* So can know if PCB changes on us */

	label = gtk_label_new(_("Group #"));
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);

	for (i = 1; i < MAX_LAYER + 1; ++i)
		{
		snprintf(buf, sizeof(buf), "%d", i);
		label = gtk_label_new(buf);
		gtk_table_attach_defaults(GTK_TABLE(table), label, i, i + 1, 0, 1);
		}

	/* Create a row of radio toggle buttons for layer.  So each layer
	|  can have an active radio button set for the group it needs to be in.
	*/
	for (layer = 0; layer < MAX_LAYER + 2; ++layer)
		{
		if (layer == MAX_LAYER + COMPONENT_LAYER)
			name = _("component side");
		else if (layer == MAX_LAYER + SOLDER_LAYER)
			name = _("solder side");
		else
			name = UNKNOWN (PCB->Data->Layer[layer].Name);

		if (layer >= MAX_LAYER)
			{
			label = gtk_label_new(name);
			gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
			gtk_table_attach_defaults(GTK_TABLE(table), label,
						0, 1, layer + 1, layer + 2);
			}
		else
			{
			layer_entry[layer] = gtk_entry_new();
			gtk_entry_set_text(GTK_ENTRY(layer_entry[layer]), name);
			gtk_table_attach_defaults(GTK_TABLE(table), layer_entry[layer],
						0, 1, layer + 1, layer + 2);
			}

		group = NULL;
		for (i = 0; i < MAX_LAYER; ++i)
			{
			if (layer < MAX_LAYER || i > 0)
				button = gtk_radio_button_new(group);
			else		/* See layer button fixup below */
				button = gtk_radio_button_new_with_label(group, "8");

			gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(button), FALSE);
			group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
			gtk_table_attach_defaults(GTK_TABLE(table), button,
						i + 1, i + 2, layer + 1, layer + 2);
			g_signal_connect(G_OBJECT(button), "toggled",
						G_CALLBACK(config_layer_groups_radio_button_cb),
						GINT_TO_POINTER((layer << 8) | (i + 1)));
			group_button[layer][i] = button;
			}
		}
	config_layer_group_button_state_update();

	sep = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 4);

	gui_check_button_connected(vbox, &use_layer_default_button, FALSE,
			TRUE, FALSE, FALSE, 8, NULL, NULL,
			_("Use these layer settings as the default for new layouts"));


/* -- Info tab */
	vbox = gui_notebook_page(tabs, _("Info"), 0, 6);

	text = gui_scrolled_text_view(vbox, NULL,
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	for (i = 0; i < sizeof(layer_info_text)/sizeof(gchar *); ++i)
		gui_text_view_append(text, _(layer_info_text[i]));
	}

  /* All the radio buttons should have no label, but the component and solder
  |  side rows need a button with an initial label to force the button to fill
  |  out vertically when realized so it will match in size the other buttons
  |  which naturally fill out because the layer entries force more vertical
  |  space on their rows.  So erase that initial kludge label after showing.
  */
static void
layer_button_post_show_fixup(void)
	{
	gint 	l;

	for (l = MAX_LAYER; l < MAX_LAYER + 2; ++l)
		gtk_button_set_label(GTK_BUTTON(group_button[l][0]), "");
	}

void
gui_config_layer_name_update(gchar *name, gint layer)
	{
	if (!config_window || layers_applying || !name)
		return;
	gtk_entry_set_text(GTK_ENTRY(layer_entry[layer]), name);

	/* If we get a config layer name change because a new PCB is loaded
	|  or new layout started, need to change our working layer group copy.
	*/
	if (lg_monitor != &PCB->LayerGroups)
		{
		layer_groups = PCB->LayerGroups;
		lg_monitor = &PCB->LayerGroups;
		config_layer_group_button_state_update();
		groups_modified = FALSE;
		}
	}

  /* -------------- The Colors config page ----------------
  */
typedef struct
	{
	gchar		*name;
	GdkColor	*settings_color;
	GdkGC		*gc;
	}
	ConfigColor;

static ConfigColor	config_misc_colors[] =
	{
	{"Background",		&Settings.BackgroundColor,			NULL },
	{"Silk",			&Settings.ElementColor,				NULL},
	{"Via",				&Settings.ViaColor,					NULL},
	{"Pin",				&Settings.PinColor,					NULL},
	{"Rat lines",		&Settings.RatColor,					NULL},
	{"Invisible objects", &Settings.InvisibleObjectsColor,	NULL},
	{"Invisible mark",	&Settings.InvisibleMarkColor,		NULL},
	{"Connected",		&Settings.ConnectedColor,			NULL},
	{"Crosshair",		&Settings.CrosshairColor,			NULL},
	{"Cross",			&Settings.CrossColor,				NULL},
	{"Grid",			&Settings.GridColor,				NULL},
	{"Mask",			&Settings.MaskColor,				NULL},
	{"Warn",			&Settings.WarnColor,				NULL},
	{"OffLimit",		&Settings.OffLimitColor,			NULL},
	};

static gint	n_misc_colors = G_N_ELEMENTS(config_misc_colors);


static ConfigColor	config_selected_colors[] =
	{
	{"Silk",			&Settings.ElementSelectedColor,		NULL},
	{"Vias",			&Settings.ViaSelectedColor,			NULL},
	{"Pins",			&Settings.PinSelectedColor,			NULL},
	{"Rat lines",		&Settings.RatSelectedColor,			NULL},
	};

static gint	n_selected_colors = G_N_ELEMENTS(config_selected_colors);


static GtkWidget	*config_colors_vbox,
					*config_colors_tab_vbox,
					*config_colors_save_button,
					*config_color_file_label,
					*config_color_warn_label;

static GList		*config_color_list;

static void			config_colors_tab_create(GtkWidget *tab_vbox);

static gboolean		config_colors_modified;

static void
config_color_file_set_label(void)
	{
	gchar	*str, *name;

	name = g_path_get_basename(Settings.color_file);
	str = g_strdup_printf("Current colors loaded: <b>%s</b>", name);
	gtk_label_set_markup(GTK_LABEL(config_color_file_label), str);
	g_free(name);
	g_free(str);
	}

static void
config_color_load_cb(gpointer data)
	{
	gchar	*path, *dir = g_strdup(color_dir);

	path = gui_dialog_file_select_open(_("Load Color File"), &dir, NULL);
	if (path)
		{
		config_colors_read(path);
		dup_string(&Settings.color_file, path);
		gdk_gc_set_foreground(Output.bgGC, &Settings.BackgroundColor);
		Settings.config_modified = TRUE;

		gtk_widget_set_sensitive(config_colors_save_button, FALSE);
		gtk_widget_set_sensitive(config_color_warn_label, FALSE);
		config_color_file_set_label();
		config_colors_modified = FALSE;
		}
	g_free(path);
	g_free(dir);

	/* Receate the colors config page to pick up new colors.
	*/
	gtk_widget_destroy(config_colors_vbox);
	config_colors_tab_create(config_colors_tab_vbox);

	ActionDisplay("ClearAndRedraw", "");
	}

static void
config_color_save_cb(gpointer data)
	{
	gchar	*name, *path, *dir = g_strdup(color_dir);

	path = gui_dialog_file_select_save(_("Save Color File"), &dir, NULL, NULL);
	if (path)
		{
		name = g_path_get_basename(path);
		if (!strcmp(name, "default"))
			gui_dialog_message(
					_("Sorry, not overwriting the default color file!"));
		else
			{
			config_colors_write(path);
			dup_string(&Settings.color_file, path);
			Settings.config_modified = TRUE;

			gtk_widget_set_sensitive(config_colors_save_button, FALSE);
			gtk_widget_set_sensitive(config_color_warn_label, FALSE);
			config_color_file_set_label();
			config_colors_modified = FALSE;
			}
		g_free(name);
		}
	g_free(path);
	g_free(dir);
	}

static void
config_color_set_cb(GtkWidget *button, ConfigColor *cc)
	{
	GdkColor	new_color;
	gchar		*str;

	gtk_color_button_get_color(GTK_COLOR_BUTTON(button), &new_color);
	str = gui_get_color_name(&new_color);
	gui_map_color_string(str, cc->settings_color);
	g_free(str);

	if (cc->gc)
		gdk_gc_set_foreground(cc->gc, cc->settings_color);

	config_colors_modified = TRUE;
	gtk_widget_set_sensitive(config_colors_save_button, TRUE);
	gtk_widget_set_sensitive(config_color_warn_label, TRUE);

	gui_layer_buttons_color_update();

	ActionDisplay("ClearAndRedraw", "");
	}

static void
config_color_button_create(GtkWidget *box, ConfigColor *cc)
	{
	GtkWidget	*button, *hbox, *label;
	gchar		*title;

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 0);

	title = g_strdup_printf(_("PCB %s Color"), cc->name);
	button = gtk_color_button_new_with_color(cc->settings_color);
	gtk_color_button_set_title(GTK_COLOR_BUTTON(button), title);
	g_free(title);

	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	label = gtk_label_new(cc->name);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "color-set",
                G_CALLBACK(config_color_set_cb), cc);
	}

static void
config_colors_tab_create(GtkWidget *tab_vbox)
	{
	GtkWidget	*scrolled_vbox, *vbox, *hbox, *expander, *sep;
	ConfigColor	*cc;
	gint		i;

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tab_vbox), vbox, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	config_colors_vbox = vbox;		/* can be destroyed if color file loaded */
	config_colors_tab_vbox = tab_vbox;

	scrolled_vbox = gui_scrolled_vbox(config_colors_vbox, NULL,
			GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	/* ---- Main colors ---- */
	expander = gtk_expander_new(_("Main colors"));
	gtk_box_pack_start(GTK_BOX(scrolled_vbox), expander, FALSE, FALSE, 2);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(expander), vbox);
	vbox = gui_category_vbox(vbox, NULL, 0, 2, TRUE, FALSE);

	config_misc_colors[0].gc = Output.bgGC;
	for (cc = &config_misc_colors[0];
						cc < &config_misc_colors[n_misc_colors]; ++cc)
		config_color_button_create(vbox, cc);


	/* ---- Layer colors ---- */
	expander = gtk_expander_new(_("Layer colors"));
	gtk_box_pack_start(GTK_BOX(scrolled_vbox), expander, FALSE, FALSE, 2);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(expander), vbox);
	vbox = gui_category_vbox(vbox, NULL, 0, 2, TRUE, FALSE);

	for (i = 0; i < MAX_LAYER; ++i)
		{
		cc = g_new0(ConfigColor, 1);
		cc->name = PCB->Data->Layer[i].Name;
		cc->settings_color = &Settings.LayerColor[i];
		config_color_button_create(vbox, cc);
		config_color_list = g_list_append(config_color_list, cc);
		}

	/* ---- Selected colors ---- */
	expander = gtk_expander_new(_("Selected colors"));
	gtk_box_pack_start(GTK_BOX(scrolled_vbox), expander, FALSE, FALSE, 2);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(expander), vbox);
	vbox = gui_category_vbox(vbox, NULL, 0, 2, TRUE, FALSE);

	for (cc = &config_selected_colors[0];
						cc < &config_selected_colors[n_selected_colors]; ++cc)
		config_color_button_create(vbox, cc);

	sep = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 2);

	for (i = 0; i < MAX_LAYER; ++i)
		{
		cc = g_new0(ConfigColor, 1);
		cc->name = PCB->Data->Layer[i].Name;
		cc->settings_color = &Settings.LayerSelectedColor[i];
		config_color_button_create(vbox, cc);
		config_color_list = g_list_append(config_color_list, cc);
		}

	config_color_warn_label = gtk_label_new("");
	gtk_label_set_use_markup(GTK_LABEL(config_color_warn_label), TRUE);
	gtk_label_set_markup(GTK_LABEL(config_color_warn_label),
			_("<b>Warning:</b> unsaved color changes will be lost"
			" at program exit."));
	gtk_box_pack_start(GTK_BOX(config_colors_vbox), config_color_warn_label,
			FALSE, FALSE, 4);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(config_colors_vbox), hbox, FALSE, FALSE, 6);

	config_color_file_label = gtk_label_new("");
	gtk_label_set_use_markup(GTK_LABEL(config_color_file_label), TRUE);
	config_color_file_set_label();
	gtk_box_pack_start(GTK_BOX(hbox), config_color_file_label,
			FALSE, FALSE, 0);

	gui_button_connected(hbox, NULL, FALSE, FALSE, FALSE, 4,
			config_color_load_cb, NULL, _("Load"));
	gui_button_connected(hbox, &config_colors_save_button,
			FALSE, FALSE, FALSE, 4,
			config_color_save_cb, NULL, _("Save"));

	gtk_widget_set_sensitive(config_colors_save_button,
				config_colors_modified);
	gtk_widget_set_sensitive(config_color_warn_label, config_colors_modified);
	gtk_widget_show_all(config_colors_vbox);
	}

static void
config_colors_destroy_list(void)
	{
	free_glist_and_data(&config_color_list);
	}


  /* --------------- The main config page -----------------
  */
enum
	{
	CONFIG_NAME_COLUMN,
	CONFIG_PAGE_COLUMN,
	N_CONFIG_COLUMNS
	};

static GtkNotebook	*config_notebook;

static GtkWidget *
config_page_create(GtkTreeStore *tree, GtkTreeIter *iter,
				GtkNotebook *notebook)
	{
	GtkWidget	*vbox;
	gint		page;

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_notebook_append_page(notebook, vbox, NULL);
	page = g_list_length(notebook->children) - 1;
	gtk_tree_store_set(tree, iter,
			CONFIG_PAGE_COLUMN, page,
			-1);
	return vbox;
	}

void
gui_config_handle_units_changed(void)
	{
	set_cursor_position_labels();
	gtk_label_set_markup(GTK_LABEL(gui->cursor_units_label),
				Settings.grid_units_mm ?
				"<b>mm</b> " : "<b>mil</b> ");
	if (config_sizes_vbox)
		{
		gtk_widget_destroy(config_sizes_vbox);
		config_sizes_vbox = NULL;
		config_sizes_tab_create(config_sizes_tab_vbox);
		}
	if (config_increments_vbox)
		{
		gtk_widget_destroy(config_increments_vbox);
		config_increments_vbox = NULL;
		config_increments_tab_create(config_increments_tab_vbox);
		}
	Settings.config_modified = TRUE;
	}

void
gui_config_text_scale_update(void)
	{
	if (config_window)
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(config_text_spin_button),
					(gdouble) Settings.TextScale);
	}

static void
config_close_cb(gpointer data)
	{
	/* Config pages may need to check for modified entries, use as default
	|  options, etc when the config window is closed.
	*/
	config_general_apply();
	config_sizes_apply();
	config_layers_apply();
	config_library_apply();

	config_sizes_vbox = NULL;
	config_increments_vbox = NULL;
	config_colors_destroy_list();
	gtk_widget_destroy(config_window);
	config_window = NULL;
	}

static void
config_destroy_cb(gpointer data)
	{
	config_sizes_vbox = NULL;
	config_increments_vbox = NULL;
	config_window = NULL;
	}

static void
config_selection_changed_cb(GtkTreeSelection *selection, gpointer data)
	{
	GtkTreeIter		iter;
	GtkTreeModel	*model;
	gint			page;

	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;
	gtk_tree_model_get(model, &iter,
				CONFIG_PAGE_COLUMN, &page,
				-1);
	gtk_notebook_set_current_page(config_notebook, page);
	}

void
gui_config_window_show(void)
	{
	GtkWidget			*widget,
						*main_vbox, *vbox,
						*config_hbox, *hbox;
	GtkWidget			*scrolled;
	GtkWidget			*button;
	GtkTreeStore		*model;
	GtkTreeView			*treeview;
	GtkTreeIter			iter;
	GtkCellRenderer		*renderer;
	GtkTreeViewColumn	*column;
	GtkTreeSelection	*select;
	
	if (config_window)
		{
		gtk_window_present(GTK_WINDOW(config_window));
		return;
		}

	config_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
 	g_signal_connect(G_OBJECT(config_window), "delete_event",
			G_CALLBACK(config_destroy_cb), NULL);

	gtk_window_set_title(GTK_WINDOW(config_window), "PCB Preferences");
	gtk_window_set_wmclass(GTK_WINDOW(config_window),
					"Pcb_Conf", "PCB");
	gtk_container_set_border_width(GTK_CONTAINER(config_window), 2);
	
	config_hbox = gtk_hbox_new(FALSE, 4);
	gtk_container_add(GTK_CONTAINER(config_window), config_hbox);

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
			GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(config_hbox), scrolled, FALSE, FALSE, 0);

	main_vbox = gtk_vbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(config_hbox), main_vbox, TRUE, TRUE, 0);

	widget = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(main_vbox), widget, TRUE, TRUE, 0);
	config_notebook = GTK_NOTEBOOK(widget);
	gtk_notebook_set_show_tabs(config_notebook, FALSE);

	model = gtk_tree_store_new(N_CONFIG_COLUMNS, G_TYPE_STRING, G_TYPE_INT);


	/* -- General -- */
	gtk_tree_store_append(model, &iter, NULL);
	gtk_tree_store_set(model, &iter, CONFIG_NAME_COLUMN, _("General"), -1);
	vbox = config_page_create(model, &iter, config_notebook);
	config_general_tab_create(vbox);


	/* -- Sizes -- */
	gtk_tree_store_append(model, &iter, NULL);
	gtk_tree_store_set(model, &iter, CONFIG_NAME_COLUMN, _("Sizes"), -1);
	vbox = config_page_create(model, &iter, config_notebook);
	config_sizes_tab_create(vbox);

	/* -- Increments -- */
	gtk_tree_store_append(model, &iter, NULL);
	gtk_tree_store_set(model, &iter, CONFIG_NAME_COLUMN, _("Increments"), -1);
	vbox = config_page_create(model, &iter, config_notebook);
	config_increments_tab_create(vbox);

	/* -- Library -- */
	gtk_tree_store_append(model, &iter, NULL);
	gtk_tree_store_set(model, &iter, CONFIG_NAME_COLUMN, _("Library"), -1);
	vbox = config_page_create(model, &iter, config_notebook);
	config_library_tab_create(vbox);

	/* -- Layer names and groups -- */
	gtk_tree_store_append(model, &iter, NULL);
	gtk_tree_store_set(model, &iter, CONFIG_NAME_COLUMN, _("Layers"), -1);
	vbox = config_page_create(model, &iter, config_notebook);
	config_layers_tab_create(vbox);


	/* -- Colors -- */
	gtk_tree_store_append(model, &iter, NULL);
	gtk_tree_store_set(model, &iter, CONFIG_NAME_COLUMN, _("Colors"), -1);
	vbox = config_page_create(model, &iter, config_notebook);
	config_colors_tab_create(vbox);


	/* Create the tree view
	*/
	treeview =
			GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(model)));
	g_object_unref(G_OBJECT(model));		/* Don't need the model anymore */

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(NULL, renderer,
					"text", CONFIG_NAME_COLUMN, NULL);
	gtk_tree_view_append_column(treeview, column);
	gtk_container_add(GTK_CONTAINER(scrolled), GTK_WIDGET(treeview));


	select = gtk_tree_view_get_selection(treeview);
	gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
	g_signal_connect(G_OBJECT(select), "changed",
				G_CALLBACK(config_selection_changed_cb), NULL);


	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(hbox), 5);
	gtk_box_pack_start(GTK_BOX(main_vbox), hbox, FALSE, FALSE, 0);

	button = gtk_button_new_from_stock(GTK_STOCK_OK);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	g_signal_connect(G_OBJECT(button), "clicked",
				G_CALLBACK(config_close_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
	gtk_widget_grab_default(button);

	gtk_widget_show_all(config_window);

	layer_button_post_show_fixup();
	}

