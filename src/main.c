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


  /* -----------------------------------------------------------------------
  |  Most settings can be configured within the gui.
  |  The Xt PCB has several additional options but it seemed inconsistent
  |  to me that only a few settings were selected to be command line
  |  settable.  So I'm only including the ones that would be needed at
  |  startup or ones that would be considered an advanced setting which
  |  isn't yet settable in the gui config (FileCommand, FontCommand, and
  |  some others).  Others can be easily added back in later if desired.
  */
typedef struct
	{
	gchar		*string,
				**string_arg;
	gint		*value;
	gboolean	*flag;
	}
	CommandLineOption;

static CommandLineOption	command_line_options[] = 
	{
	/* Only one of string_arg, value, or flag fields should be non-NULL */
	/* option			string arg			value arg		boolean (no arg) */

	{"-action",			&Settings.ActionString,			NULL, NULL },
	{"-background",		&Settings.BackgroundImage,		NULL, NULL },
	{"-fontfile",		&Settings.FontFile,				NULL, NULL  },
	{"-lelement",		&Settings.ElementCommand,		NULL, NULL  },
	{"-lfile",			&Settings.FileCommand,			NULL, NULL  },
	{"-lfont",			&Settings.FontCommand,			NULL, NULL  },
	{"-lg",				&Settings.Groups,				NULL, NULL  },
	{"-libname",		&Settings.LibraryFilename,		NULL, NULL  },
	{"-libpath",		&Settings.LibraryPath,			NULL, NULL  },
	{"-libtree",		&Settings.LibraryTree,			NULL, NULL  },
	{"-llib",			&Settings.LibraryCommand,		NULL, NULL  },
	{"-llibcont",		&Settings.LibraryContentsCommand, NULL, NULL  },
	{"-rs",				&Settings.Routes,				NULL, NULL  },
	{"-script",			&Settings.ScriptFilename,		NULL, NULL  },
	{"-size",			&Settings.Size,					NULL, NULL  },
	{"-sfile",			&Settings.SaveCommand,			NULL, NULL  },
	{"-debug",			NULL,	NULL,			&Settings.debug },
	};

  /* Find our command line args in argv and NULL out the args we consume
  |  here.  Then collapse the argv over the NULL entries and adjust the
  |  argc.  This code based on the Gtk lib gtkmain.c argc/argv handling.
  */
static void
get_args(gint *argc, gchar ***argv)
	{
	CommandLineOption	*option;
	gchar				*str;
	gint				i, j, k;

	if (!argc || !argv)
		return;

	for (i = 1; i < *argc;)
		{
		for (j = 0; j < G_N_ELEMENTS(command_line_options); ++j)
			{
			option = &command_line_options[j];
			str = (*argv)[i];

			/* Allow -arg and --arg */
			if (   !strcmp(option->string, str)
			    || (*str == '-' && !strcmp(option->string, str + 1))
			   )
				{
				if (option->flag)
					*option->flag = TRUE;
				else if (i + 1 < *argc)
					{
					(*argv)[i] = NULL;
					++i;
					if (option->string_arg)
						dup_string(option->string_arg, (*argv)[i]);
					else if (option->value)
						*option->value = atoi((*argv)[i]);
					}
				(*argv)[i] = NULL;
				break;
				}
			}
		++i;
		}
	for (i = 1; i < *argc; i++)
		{
		for (k = i; k < *argc; k++)
			if ((*argv)[k] != NULL)
				break;
		if (k > i)
			{
			k -= i;
			for (j = i + k; j < *argc; j++)
				(*argv)[j - k] = (*argv)[j];
			*argc -= k;
			}
		}
	}


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

	/* take care of special options */
	if (argc == 2)
		{
		if (   !NSTRCMP ("-help", argv[1])
		    || !NSTRCMP ("--help", argv[1])
		    || !NSTRCMP ("-h", argv[1])
		   )
			Usage ();
		if (!NSTRCMP ("-copyright", argv[1]) ||
				!NSTRCMP ("--copyright", argv[1]) )
			Copyright ();
		if (!NSTRCMP ("-version", argv[1]) ||
				!NSTRCMP ("--version", argv[1]) )
			{
			puts (VERSION);
			exit (0);
			}
		}

	gui_init(&argc, &argv);
	get_args(&argc, &argv);
	config_settings_load();


//	InitShell (&argc, argv);

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
//	GetSizeOfDrawingArea ();

	FullRegion = gdk_region_new();

	Big.x = Big.y = 0;
	Big.width = Big.height = TO_SCREEN(MAX_COORD);
	gdk_region_union_with_rect(FullRegion, &Big);

	switch (--argc)
		{
		case 0:			/* only program name */
			SetZoom(PCB->Zoom);
		break;

		case 1:		/* load an initial layout;
					* abort if filename looks like an option
					*/
			++argv;
			if (**argv == '-')
				Usage ();

			/* keep filename even if initial load command failed;
			* file might not exist
			*/
			if (LoadPCB (*argv))
				PCB->Filename = MyStrdup (*argv, "main()");
			break;

		default:			/* error */
			Usage ();
			break;
		}

//	XXX
//	LoadBackgroundImage (Settings.BackgroundImage);

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

	gtk_main();

	if (Settings.config_modified)
		config_file_write();
	return (0);
	}
