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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* executes commands from user
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <ctype.h>

#include "global.h"
#include "action.h"
#include "buffer.h"
#include "command.h"
#include "data.h"
#include "djopt.h"
#include "error.h"
#include "file.h"
#include "mymem.h"
#include "misc.h"
#include "rats.h"
#include "set.h"
#include "vendor.h"

#include "gui.h"

RCSID("$Id$");

typedef int (*FunctionTypePtr) (void);

/* ---------------------------------------------------------------------------
 * local prototypes
 */
static int CommandCallAction (void);
static int CommandLoadElementToBuffer (void);
static int CommandLoadLayoutToBuffer (void);
static int CommandLoadLayout (void);
static int CommandLoadNetlist (void);
static int CommandSaveLayout (void);
static int CommandSaveLayoutAndQuit (void);
static int CommandQuit (void);
static int CommandHelp (void);
static int SetArgcArgv (char *);


typedef struct
	{
	char *Text;					/* commandstring */
	FunctionTypePtr Function;	/* function pointer */
	}
	CommandType, *CommandTypePtr;


typedef struct
	{
	gchar	*text;
	void	(*function)();
	gboolean        needs_params;
	}
	ActionType;


/* ---------------------------------------------------------------------------
 * actions
 */
static ActionType	action[] =
	{
	{"AutoPlaceSelected",	ActionAutoPlaceSelected, FALSE},
	{"AutoRoute",			ActionAutoRoute, FALSE},
	{"SetSame",				ActionSetSame, FALSE},
	{"MovePointer",			ActionMovePointer, FALSE},
	{"ToggleHideName",		ActionToggleHideName, FALSE},
	{"ChangeHole",			ActionChangeHole, FALSE},
	{"ToggleThermal",		ActionToggleThermal, FALSE},
	{"Atomic",				ActionAtomic, FALSE},
	{"RouteStyle",			ActionRouteStyle, FALSE},
	{"DRC",					ActionDRCheck, FALSE},
	{"Flip",				ActionFlip, FALSE},
	{"SetValue",			ActionSetValue, FALSE},
	{"Quit",				ActionQuit, FALSE},
	{"Connection",			ActionConnection, FALSE},
	{"Command",				ActionCommand, FALSE},
	{"DisperseElements",	ActionDisperseElements, FALSE},
	{"Display",				ActionDisplay, FALSE},
	{"Report",				ActionReport, FALSE},
	{"Mode",				ActionMode, FALSE},
	{"RemoveSelected",		ActionRemoveSelected, FALSE},
	{"DeleteRats",			ActionDeleteRats, FALSE},
	{"AddRats",				ActionAddRats, FALSE},
	{"MarkCrosshair",		ActionMarkCrosshair, FALSE},
	{"ChangeSize",			ActionChangeSize, FALSE},
	{"ChangeClearSize",		ActionChangeClearSize, FALSE},
	{"ChangeDrillSize",		ActionChange2ndSize, FALSE},
	{"ChangeName",			ActionChangeName, FALSE},
	{"ChangeSquare",		ActionChangeSquare, FALSE},
	{"ChangeOctagon",		ActionChangeOctagon, FALSE},
	{"ChangeJoin",			ActionChangeJoin, FALSE},
	{"Select",				ActionSelect, FALSE},
	{"Unselect",			ActionUnselect, FALSE},
	{"Save",				ActionSave, FALSE},
	{"Load",				ActionLoad, FALSE},
	{"PrintDialog",				ActionPrintDialog, FALSE},
	{"Print",				ActionPrint, TRUE},
	{"New",					ActionNew, FALSE},
	{"SwapSides",			ActionSwapSides, FALSE},
	{"Bell",				ActionBell, FALSE},
	{"PasteBuffer",			ActionPasteBuffer, FALSE},
	{"Undo",				ActionUndo, FALSE},
	{"Redo",				ActionRedo, FALSE},
	{"RipUp",				ActionRipUp, FALSE},
	{"Polygon",				ActionPolygon, FALSE},
	{"MoveToCurrentLayer",	ActionMoveToCurrentLayer, FALSE},
	{"SwitchDrawingLayer",	ActionSwitchDrawingLayer, FALSE},
	{"ToggleVisibility",	ActionToggleVisibility, FALSE},
	{"MoveObject",			ActionMoveObject, FALSE},
	{"djopt",				ActionDJopt, FALSE},
	{"GetLoc",				ActionGetXY, FALSE},
	{"SetFlag",				ActionSetFlag, FALSE},
	{"ClrFlag",				ActionClrFlag, FALSE},
	{"ChangeFlag",			ActionChangeFlag, FALSE},

	{"ExecuteFile",			ActionExecuteFile, FALSE},

	{"LoadVendor",			ActionLoadVendor, FALSE},
	{"UnloadVendor",		ActionUnloadVendor, FALSE},
	{"ApplyVendor",			ActionApplyVendor, FALSE},
	{"EnableVendor",		ActionEnableVendor, FALSE},
	{"DisableVendor",		ActionDisableVendor, FALSE},
	{"ToggleVendor",		ActionToggleVendor, FALSE},

	};

  /* All action procs invoked from the Gtk menus take 0 - 3 gchar * args,
  |  so when "num" is <= 3, can just pass three args to the action proc.
  |  Some command line action procs may want more than three args, and for
  |  these have an argc/argv style calling convention.
  */
void
CallActionProc(gchar *cmd, gchar **params, gint num)
{
  gchar	*arg1 = "", *arg2 = "", *arg3 = "";
  gint	i;

  /* scan command list */
  for (i = 0; i < G_N_ELEMENTS(action); i++)
    {
      if (!g_strcasecmp(action[i].text, cmd))
	{
	  if (action[i].needs_params)
	    action[i].function(params, num);
	  else
	    {
	      if (num > 3)
		{
		  fprintf(stderr,
			  "%s has too many args to CallActionProc()?\n", cmd);
		  return;
		}
	      if (num > 0)
		arg1 = params[0];
	      if (num > 1)
		arg2 = params[1];
	      if (num > 2)
		arg3 = params[2];
	      action[i].function(arg1, arg2, arg3);
	    }

	  break;
	}
    }
  if (i == G_N_ELEMENTS(action))
    Message(_("Warning: action proc '%s' not found.\n"), cmd);
}


/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static CommandType Command[] = {
  {"h", CommandHelp},
  {"le", CommandLoadElementToBuffer},
  {"m", CommandLoadLayoutToBuffer},
  {"rn", CommandLoadNetlist},
  {"s", CommandSaveLayout},
  {"w", CommandSaveLayout},
  {"wq", CommandSaveLayoutAndQuit},
  {"q", CommandQuit},
  {"q!", CommandQuit},
  {"l", CommandLoadLayout}
};

static char **argv;		/* command-line of arguments */
static int argc;

/* ---------------------------------------------------------------------------
 * quits the application after confirming
 * syntax: q
 */
static int
CommandQuit (void)
{
  if (argc != 1)
    {
      Message (_("Usage: q\n  quits the application\n"
	       "       q!\n"
	       "quits without saving or warning for its need\n"));
      return (1);
    }
  if (strcasecmp (argv[0], "q!") == 0)
    QuitApplication ();
  if (!PCB->Changed || gui_dialog_confirm(_("OK to lose data ?")))
    QuitApplication ();

  return (0);
}

/* ---------------------------------------------------------------------------
 * loads an element into the current buffer
 * syntax: le [name]
 */
static int
CommandLoadElementToBuffer (void)
{
  char *filename;
  static gchar	*current_element_dir = NULL;

  switch (argc)
    {
    case 1:			/* open fileselect box */
	  filename = gui_dialog_file_select_open(_("Load element to buffer"),
					&current_element_dir, Settings.ElementPath);

      if (filename && LoadElementToBuffer (PASTEBUFFER, filename, True))
	SetMode (PASTEBUFFER_MODE);
      g_free(filename);
      break;

    case 2:			/* filename is passed in commandline */
      filename = argv[1];
      if (filename && LoadElementToBuffer (PASTEBUFFER, filename, True))
	SetMode (PASTEBUFFER_MODE);
      break;

    default:			/* usage */
      Message (False, "Usage: le [name]\n  loads element data to buffer\n");
      return (1);
    }
  return (0);
}

/* ---------------------------------------------------------------------------
 * loads a layout into the current buffer
 * syntax: m [name]
 */
static int
CommandLoadLayoutToBuffer (void)
{
  char			*filename;
  static gchar	*current_layout_dir = NULL;

  switch (argc)
    {
    case 1:			/* open fileselect box */
	  filename = gui_dialog_file_select_open(_("Load layout file to buffer"),
						&current_layout_dir, Settings.FilePath);
      if (filename && LoadLayoutToBuffer (PASTEBUFFER, filename))
	SetMode (PASTEBUFFER_MODE);
      g_free(filename);
      break;

    case 2:			/* filename is passed in commandline */
      filename = argv[1];
      if (filename && LoadLayoutToBuffer (PASTEBUFFER, filename))
	SetMode (PASTEBUFFER_MODE);
      break;

    default:			/* usage */
      Message ("Usage: m [name]\n  loads layout data to buffer\n");
      return (1);
    }
  return (0);
}

/* ---------------------------------------------------------------------------
 * saves the layout data and quits
 */
static int
CommandSaveLayoutAndQuit (void)
{
  if (argc != 1 && argc != 2)
    {
      Message ("Usage: wq [name]\n  saves layout data and quits\n");
      return (1);
    }
  if (!CommandSaveLayout ())
    {
      if (!PCB->Changed || gui_dialog_confirm(_("OK to lose data ?")))
	QuitApplication ();
      return (0);
    }
  return (1);
}

/* ----------------------------------------------------------------------
 * saves layout data
 * syntax: l name
 */
static int
CommandSaveLayout (void)
{
  char *filename;
  static gchar	*current_save_path = NULL;

  switch (argc)
    {
    case 1:			/* query name if necessary */
      if (!PCB->Filename)
	{
	  filename = gui_dialog_file_select_save(_("Save layout as"),
						&current_save_path, NULL,
						Settings.FilePath);
	  if (filename)
	    SavePCB (filename);
	  g_free(filename);
	}
      else
	SavePCB (PCB->Filename);
      break;

    case 2:
      SavePCB (argv[1]);
      break;

    default:
      Message ("Usage: s [name] | w [name]\n  saves layout data\n");
      return (1);
    }
  return (0);
}

/* ----------------------------------------------------------------------
 * loads layout data
 * syntax: l [name]
 */
static int
CommandLoadLayout (void)
{
  char			*filename, *name = NULL;
  static gchar	*current_layout_dir = NULL;

  switch (argc)
    {
    case 1:			/* open fileselect box */
	  name = gui_dialog_file_select_open(_("Load layout file"),
						&current_layout_dir, Settings.FilePath);
      filename = name;
      if (!name)
	return (0);
      break;

    case 2:			/* filename is passed in commandline */
      filename = argv[1];
      break;

    default:			/* usage */
      Message ("Usage: l [name]\n  loads layout data\n");
      return (1);
    }

  if (!PCB->Changed || gui_dialog_confirm("OK to override layout data?"))
    LoadPCB (filename);
  g_free(name);
  return (0);
}

/* ----------------------------------------------------------------------
 * reads netlist 
 * syntax: rn [name]
 */
static int
CommandLoadNetlist (void)
{
  char *filename, *name = NULL;
  static gchar	*current_netlist_dir = NULL;

  switch (argc)
    {
    case 1:			/* open fileselect box */
	  name = gui_dialog_file_select_open(_("Load netlist file"),
						&current_netlist_dir, Settings.FilePath);
	  filename = name;
      if (!filename)
	return (0);
      break;

    case 2:			/* filename is passed in commandline */
      filename = argv[1];
      break;

    default:			/* usage */
      Message ("Usage: rn [name]\n  reads in a netlist file\n");
      return (1);
    }
  if (PCB->Netlistname)
    SaveFree (PCB->Netlistname);
  PCB->Netlistname = StripWhiteSpaceAndDup (filename);
  g_free(name);
  return (0);
}

/* ----------------------------------------------------------------------
 * Send an action command
 */
static int
CommandCallAction (void)
{
  CallActionProc (argv[0], &argv[1], argc - 1);
  return (0);
}

/* ----------------------------------------------------------------------
 * print a help message for commands
 */
static int
CommandHelp ()
{
  Message ("following commands are supported:\n"
	   "  Command()   execute an action command (too numerous to list)\n"
	   "              see the manual for the list of action commands\n"
	   "  h           display this help message\n"
	   "  l  [file]   load layout\n"
	   "  le [file]   load element to buffer\n"
	   "  m  [file]   load layout to buffer (merge)\n"
	   "  q           quits the application\n"
	   "  q!          quits without save warning\n"
	   "  rn [file]   read in a net-list file\n"
	   "  s  [file]   save layout\n"
	   "  w  [file]   save layout\n"
	   "  wq [file]   save layout and quit\n");
  return (0);
}

/* ----------------------------------------------------------------------
 * split commandline and fill argv/argc
 */
static int
SetArgcArgv (char *Line)
{
  static int maxcount = 0;
  char *p;

  for (argc = 0, p = strtok (Line, " \t;,()"); p;
       p = strtok (NULL, " \t;,()"))
    {
      /* allocate more memory */
      if (argc >= maxcount)
	{
	  maxcount += 20;
	  argv = (char **) MyRealloc (argv, maxcount * sizeof (char *),
				      "SetArgcArgv()");
	}
      argv[argc++] = p;
    }
  return (argc);
}

/* ---------------------------------------------------------------------------
 * command is passed in, memory is already allocated
 */
void
ExecuteUserCommand (char *CommandLine)
{
  int i;

  if (SetArgcArgv (CommandLine))
    {
      /* scan command list */
      for (i = 0; i < ENTRIES (Command); i++)
	if (!strcasecmp (Command[i].Text, argv[0]))
	  break;
      if (i == ENTRIES (Command))
	/* it wasn't listed, it must have been an action command */
	CommandCallAction ();
      else if (Command[i].Function ())
	gui_beep(Settings.Volume);
    }
  else
    gui_beep(Settings.Volume);
}

