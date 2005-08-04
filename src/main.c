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

#include "gui.h"

#include "buffer.h"
#include "create.h"
#include "crosshair.h"
#include "draw.h"
#include "error.h"
#include "file.h"
#include "set.h"
#include "action.h"

RCSID("$Id$");




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
  signal (SIGPIPE, SIG_IGN);
}

/* ---------------------------------------------------------------------------
 * init device drivers for printing
 */
static void
InitDeviceDriver (void)
{
  DeviceInfoTypePtr ptr = PrintingDevice;

  while (ptr->Query)
    {
      ptr->Info = ptr->Query ();
      ptr++;
    }
}


  /* ----------------------------------------------------------------------
  |  command line and rc file processing.
  */
static gchar	*command_line_pcb;

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

void
usage (void)
	{
	fprintf (stderr,

	"\nUSAGE: %s [standard Gtk options] [standard options] [layout]\n"
	"or   : %s [standard Gtk options] <exactly one special option>\n\n"
	"standard options are:\n"
	"  -action-script <file>    the name of a PCB actions script file to\n"
	"                                execute on startup\n"
	"  -action-string <string>  A PCB actions string to execute on startup\n"
	"  -background <file>       PPM file to display as board background\n"

	"  -lib-command <cmd>       command to copy elements from library to stdout,\n"
	"                                %%a is set to 'template value package'\n"
	"                                %%f is set to the filename\n"
	"                                %%p is set to the searchpath\n"
	"  -lib-contents-command <cmd> command to list library contents,\n"
	"                                %%f is set to the filename\n"
	"                                %%p is set to the searchpath\n"
	"  -lib-path <path>         the library search-path\n"
	"  -lib-name <file>         the name of the library (pcblib default)\n"
	"  -lib-newlib <path>       pathname of a newlib library\n"

	"  -font-command <cmd>      command to copy font files to stdout,\n"
	"                                %%f is set to the filename\n"
	"                                %%p is set to the seachpath\n"
	"  -font-path <dir>         search for font-file in this directory\n"
	"  -font-file <file>        read default font from this file\n"
	"  -element-command <cmd>   command to copy element files to stdout,\n"
	"                                %%f is set to the filename\n"
	"                                %%p is set to the seachpath\n"
	"  -element-path <dir>      search for elements in this directory\n"

	"  -file-command <cmd>      command to copy layout files to stdout,\n"
	"                                %%f is set to the filename\n"
	"                                %%p is set to the seachpath\n"
	"  -save-command <cmd>      command to copy stdin to layout file,\n"
	"                                %%f is set to the filename\n"
	"  -print-file <file>       write printout files to this file sequence\n"

	"  -groups <layergroups>    set layergroups of new layouts to this\n"
	"  -routes <routeslist>     set route styles of new layouts to this\n"
	"  -board-size <w>x<h>      Set size of new layouts in current grid\n"
	"                                units (mils or millimeters)."
	"special options are:\n"
	"  --copyright:             prints copyright information\n"
	"  -h  --help               prints this message\n"
	"  -V  --version            prints the current version number\n"
	"  -v  --verbose\n",
		Progname, Progname);
	exit (1);
	}

static GList	*lib_path_list,
				*element_path_list,
				*font_path_list,
				*lib_newlib_list;

static gchar	*library_command_dir,
				*library_command,
				*library_contents_command,
				*groups_override,
				*routes_override,
				*board_size_override;

typedef struct
	{
	gchar		*option;
	gchar		**string_arg;	/* Only one of these next 4 must be non NULL */
	gint		*value;
	gboolean	*flag;
	GList		**list;
	}
	PcbOption;

static PcbOption	pcb_options[] = 
	{
	/* option  string_arg  value  boolean  list*/

	/* These have no default and are used only if rc file or command line set.
	*/
	{"action-string",	&Settings.ActionString,			NULL, NULL, NULL},
	{"action-script",	&Settings.ScriptFilename,		NULL, NULL, NULL},
	{"auto-place",	        NULL,                     NULL, &Settings.AutoPlace, NULL},
	{"background",		&Settings.BackgroundImage,		NULL, NULL, NULL},

	{"lib-command-dir",	&library_command_dir,			NULL, NULL, NULL},
	{"lib-command",		&library_command,				NULL, NULL, NULL},
	{"lib-contents-command", &library_contents_command,	NULL, NULL, NULL},
	{"lib-path",		NULL, NULL, NULL,			&lib_path_list},
	{"lib-name",		&Settings.LibraryFilename,		NULL, NULL, NULL},

	{"lib-newlib",		NULL, NULL, NULL,			&lib_newlib_list},

	/* These are m4 based options.  The commands are the m4 commands and
	|  the paths are a ':' separated list of search directories that will
	|  be passed to the m4 command via the '%p' parameter.
	*/
	{"font-command",	&Settings.FontCommand,			NULL, NULL, NULL},
	{"font-path",		NULL, NULL, NULL,			&font_path_list},
	{"font-file",		&Settings.FontFile,				NULL, NULL, NULL},
	{"element-command",	&Settings.ElementCommand,		NULL, NULL, NULL},
	{"element-path",	NULL, NULL, NULL,			&element_path_list},

	{"file-command",	&Settings.FileCommand,			NULL, NULL, NULL},

	{"save-command",	&Settings.SaveCommand,			NULL, NULL, NULL},
	{"print-file",		&Settings.PrintFile,			NULL, NULL, NULL},

	/* rc file or command line setting these overrides users gui config.
	*/
	{"groups",			&groups_override,				NULL, NULL, NULL},
	{"routes",			&routes_override,				NULL, NULL, NULL},
	{"board-size",		&board_size_override,			NULL, NULL, NULL},
	};

static void
settings_init(void)
	{
	gchar	*s;

	Settings.FontFile = g_strdup("default_font");
	Settings.LibraryFilename = g_strdup(LIBRARYFILENAME);

	lib_path_list = g_list_append(lib_path_list, g_strdup(PCBLIBDIR));
	element_path_list = g_list_append(element_path_list, g_strdup(PCBLIBDIR));
	lib_newlib_list = g_list_append(lib_newlib_list, g_strdup(PCBTREEDIR));

	s = g_strconcat(".:", PCBLIBDIR, NULL);
	font_path_list = g_list_append(font_path_list, s);

	library_command_dir = g_strdup(PCBLIBDIR);
		/* library command '%p' uses LibraryPath derived from lib_path_list */
	library_command = g_strdup("QueryLibrary.sh '%p' '%f' %a");
		/* library_contents_command also uses LibraryPath	*/
	library_contents_command = g_strdup("ListLibraryContents.sh '%p' '%f'");

		/* ElementsCommand uses ElementPath derived from element_path_list */
	Settings.ElementCommand = g_strdup_printf("%s | %s",
				"M4PATH='%p';export M4PATH;echo 'include(%f)'", GNUM4);
		/* FontCommand uses FontPath derived from font_path_list */
	Settings.FontCommand = g_strdup_printf("%s | %s",
				"M4PATH='%p';export M4PATH;echo 'include(%f)'", GNUM4);

	Settings.PrintFile = g_strdup("%f.output");
	Settings.RatCommand = g_strdup("cat %f");
	Settings.SaveCommand = g_strdup("cat - > '%f'");

	Settings.FileCommand = g_strdup("cat '%f'");
	Settings.FilePath = g_strdup("");	/* Needed if FileCommand used %p */
	}

static gchar *
expand_dir(gchar *dir)
	{
	gchar	*s;

	if (*dir == '~')
		s = g_build_filename((gchar *) g_get_home_dir(), dir + 1, NULL);
	else
		s = g_strdup(dir);
	return s;
	}

static void
settings_post_process(void)
	{
	GList	*list;
	gchar	*str, *colon, *dir;
	gint	width, height;

	if (groups_override)
		dup_string(&Settings.Groups, groups_override);
	if (routes_override)
		dup_string(&Settings.Routes, routes_override);
	if (   board_size_override
	    && sscanf(board_size_override, "%dx%d", &width, &height) == 2
	   )
		{
		Settings.MaxWidth = TO_PCB_UNITS(width);
		Settings.MaxHeight = TO_PCB_UNITS(height);
		}

	/* Build the LibTree, ElementPath, LibraryPath and FontPath from
	|  accumulated command line and and rc file sources.
	*/
	for (list = lib_newlib_list; list; list = list->next)
		{
		str = Settings.LibraryTree;
		colon = list->next ? ":" : NULL;
		dir = expand_dir((gchar *) list->data);
		Settings.LibraryTree = g_strconcat(str ? str : "", dir, colon, NULL);
		g_free(dir);
		g_free(str);
		}

	for (list = element_path_list; list; list = list->next)
		{
		str = Settings.ElementPath;
		colon = list->next ? ":" : NULL;
		dir = expand_dir((gchar *) list->data);
		Settings.ElementPath = g_strconcat(str ? str : "", dir, colon, NULL);
		g_free(dir);
		g_free(str);
		}

	for (list = lib_path_list; list; list = list->next)
		{
		str = Settings.LibraryPath;
		colon = list->next ? ":" : NULL;
		dir = expand_dir((gchar *) list->data);
		Settings.LibraryPath = g_strconcat(str ? str : "", dir, colon, NULL);
		g_free(dir);
		g_free(str);
		}

	for (list = font_path_list; list; list = list->next)
		{
		str = Settings.FontPath;
		colon = list->next ? ":" : NULL;
		dir = expand_dir((gchar *) list->data);
		Settings.FontPath = g_strconcat(str ? str : "", dir, colon, NULL);
		g_free(dir);
		g_free(str);
		}

	if (*library_command == '/' || *library_command == '.')
		Settings.LibraryCommand = g_strdup(library_command);
	else
		Settings.LibraryCommand = g_build_filename(library_command_dir,
					library_command, NULL);

	if (*library_contents_command == '/' || *library_contents_command == '.')
		Settings.LibraryContentsCommand = g_strdup(library_contents_command);
	else
		Settings.LibraryContentsCommand = g_build_filename(
					library_command_dir, library_contents_command, NULL);


	if (Settings.verbose)
		{
		printf(	"LibraryCommand:  %s\n"
				"LibraryContentsCommand:  %s\n"
				"LibraryPath:  %s\n"
				"LibraryFilename:  %s\n"
				"\n"
				"FontCommand:  %s\n"
				"FontPath:  %s\n"
				"\n"
				"ElementCommand:  %s\n"
				"ElementPath:  %s\n"
				"LibraryTree:  %s\n"
				"\n",

				Settings.LibraryCommand, Settings.LibraryContentsCommand,
				Settings.LibraryPath, Settings.LibraryFilename,
				Settings.FontCommand, Settings.FontPath,
				Settings.ElementCommand, Settings.ElementPath,
				Settings.LibraryTree);
		}
	}


void
gui_settings_add_to_newlib_list(gchar *config_library_newlib)
	{
	gchar	*p, *paths;

	paths = g_strdup(config_library_newlib);
	for (p = strtok(paths, ":"); p && *p; p = strtok(NULL, ":"))
		lib_newlib_list = g_list_prepend(lib_newlib_list, expand_dir(p));
	g_free(paths);
	}

static gint
parse_option(gchar *option, gchar *arg)
	{
	PcbOption	*opt;
	gint		i;
	gboolean	append_flag = FALSE, prepend_flag = FALSE;

	if (arg)
		{
		if (*arg == '+')
			{
			append_flag = TRUE;
			++arg;
			}
		if (*arg == '=')
			++arg;
		if (*arg == '+')
			{
			prepend_flag = TRUE;
			++arg;
			}
		while (*arg == ' ' || *arg == '\t')
			++arg;
		}

	for (i = 0; i < G_N_ELEMENTS(pcb_options); ++i)
		{
		opt = &pcb_options[i];

		if (!strcmp(opt->option, option))
			{
			if (opt->flag)
				{
				*opt->flag = TRUE;
				return 0;			/* no extra args consumed */
				}
			else if (opt->string_arg && arg)
				{
				dup_string(opt->string_arg, arg);
				return 1;			/* one extra arg consumed */
				}
			else if (opt->value && arg)
				{
				*opt->value = atoi(arg);
				return 1;
				}
			else if (opt->list && arg)
				{
				if (prepend_flag)
					*opt->list = g_list_prepend(*opt->list, g_strdup(arg));
				else if (append_flag)
					*opt->list = g_list_append(*opt->list, g_strdup(arg));
				else
					{
					free_glist_and_data(opt->list);
					*opt->list = g_list_append(*opt->list, g_strdup(arg));
					}
				return 1;
				}
			break;
			}
		}
	return -1;
	}


static void
load_rc_file(gchar *path)
	{
	FILE	*f;
	gchar	*s, buf[1024], option[32], arg[768];

	f = fopen(path, "r");
	if (!f)
		return;
	if (Settings.verbose)
		printf("Reading pcbrc file: %s\n", path);
	while (fgets(buf, sizeof(buf), f))
		{
		for (s = buf; *s == ' ' || *s == '\t' || *s == '\n'; ++s)
			;
		if (!*s || *s == '#' || *s == '/' || *s == ';')
			continue;
		arg[0] = '\0';
		sscanf(s, "%31s %767[^\n]", option, arg);
		if (parse_option(option, arg[0] != '\0' ? arg : NULL) < 0)
			printf("%s: option '%s' is bad or incomplete.\n", path, option);
		}
    fclose(f);
    }

static void
load_all_rc_files(void)
	{
	gchar			*path;

	load_rc_file("/etc/pcbrc");
	load_rc_file("/usr/local/etc/pcbrc");

	path = g_build_filename(PCBLIBDIR, "pcbrc", NULL);
	load_rc_file(path);
	g_free(path);

	path = g_build_filename((gchar *) g_get_home_dir(), ".pcb/pcbrc", NULL);
	load_rc_file(path);
	g_free(path);

	load_rc_file("pcbrc");
	}

static void
get_args(gint argc, gchar **argv)
	{
	gchar	*opt, *arg;
	gint	i, r;

	for (i = 1; i < argc; ++i)
		{
		opt = argv[i];
		arg = argv[i + 1];
		if (*opt == '-')
			{
			++opt;
			if (*opt == '-')
				++opt;
			if (!strcmp(opt, "version") || !strcmp(opt, "V"))
				{
				printf("%s: %s\n", Progname, VERSION);
				exit(0);
				}
			else if (!strcmp(opt, "verbose") || !strcmp(opt, "v"))
				{
				Settings.verbose += 1;
				continue;
				}
			else if (!strcmp(opt, "help") || !strcmp(opt, "h"))
				usage();
			else if (!strcmp(opt, "copyright"))
				copyright();
			else if (   i < argc
					 && ((r = parse_option(opt, (i < argc - 1) ? arg : NULL))
									>= 0)
					)
				{
				i += r;
				continue;
				}
			printf("%s: bad or incomplete arg: %s\n", Progname, argv[i]);
			usage();
			}
		else
			command_line_pcb = g_strdup(argv[i]);
		}
	}

/* ---------------------------------------------------------------------- 
 * main program
 */
int
main (int argc, char *argv[])
	{
	GdkRectangle	Big;

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
	if ((Progname = strrchr (*argv, '/')) == NULL)
		Progname = *argv;
	else
		Progname++;

	gui_init(&argc, &argv);

	settings_init();
	load_all_rc_files();
	get_args(argc, argv);
	config_settings_load();
	settings_post_process();

	InitDeviceDriver ();
	PCB = CreateNewPCB (True);
	ResetStackAndVisibility ();

	gui_create_pcb_widgets();

	CreateDefaultFont ();
	InitCrosshair ();
	InitHandler ();
	InitBuffers ();
	SetMode (NO_MODE);

	/* update zoom and grid... */
	UpdateSettingsOnScreen ();

	gdk_window_get_geometry(NULL, NULL, NULL, &Settings.w_display,
                &Settings.h_display, NULL);

	FullRegion = gdk_region_new();

	Big.x = Big.y = 0;
	Big.width = Big.height = TO_SCREEN(MAX_COORD);
	gdk_region_union_with_rect(FullRegion, &Big);

	if (!command_line_pcb)
		SetZoom(PCB->Zoom);
	else
		{
		/* keep filename even if initial load command failed;
		* file might not exist
		*/
		if (LoadPCB (command_line_pcb))
			PCB->Filename = MyStrdup (command_line_pcb, "main()");
		}

/*	FIX_ME
	LoadBackgroundImage (Settings.BackgroundImage); */

	/* Register a function to be called when the program terminates.
	 * This makes sure that data is saved even if LEX/YACC routines
	 * abort the program.
	 * If the OS doesn't have at least one of them,
	 * the critical sections will be handled by parse_l.l
	*/
	g_atexit(EmergencySave);

	/* read the library file and display it if it's not empty
	*/
	if (!ReadLibraryContents () && Library.MenuN)
		gui_library_window_show(&Output);

	gui_config_start_backup_timer();

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
		Message(_("Executing startup script file %s\n"),
					Settings.ScriptFilename);
		ActionExecuteFile(Settings.ScriptFilename);
		}
	if (Settings.ActionString)
		{
		gchar	*param[1];

		Message(_("Executing startup action %s\n"), Settings.ActionString);
		param[0] = Settings.ActionString;
		ActionExecuteAction(param, 1);
		}

	if( Settings.init_done == 0 )
	{
		Settings.init_done = 1;
		gtk_main();
	}

	if (Settings.config_modified)
		config_file_write();
	return (0);
	}
