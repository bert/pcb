/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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

RCSID("$Id$");

/* ----------------------------------------------------------------------
 * print a help message for commands
 */

static int
CommandHelp (int argc, char **argv, int x, int y)
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
 * loads layout data
 * syntax: l [name]
 */

static int
CommandLoadLayout (int argc, char **argv, int x, int y)
{
  char			*filename, *name = NULL;
#ifdef FIXME
  static char	*current_layout_dir = NULL;
#endif

  switch (argc)
    {
#ifdef FIXME
    case 0:			/* open fileselect box */
	  name = gui_dialog_file_select_open(_("Load layout file"),
						&current_layout_dir, Settings.FilePath);
      filename = name;
      if (!name)
	return (0);
      break;
#endif

    case 1:			/* filename is passed in commandline */
      filename = argv[0];
      break;

    default:			/* usage */
      Message ("Usage: l [name]\n  loads layout data\n");
      return (1);
    }

  if (!PCB->Changed || gui->confirm_dialog("OK to override layout data?", 0))
    LoadPCB (filename);
  free(name);
  return (0);
}

/* ---------------------------------------------------------------------------
 * loads an element into the current buffer
 * syntax: le [name]
 */

static int
CommandLoadElementToBuffer (int argc, char **argv, int x, int y)
{
  char *filename;
#ifdef FIXME
  static char	*current_element_dir = NULL;
#endif

  switch (argc)
    {
#ifdef FIXME
    case 0:			/* open fileselect box */
	  filename = gui_dialog_file_select_open(_("Load element to buffer"),
					&current_element_dir, Settings.ElementPath);

      if (filename && LoadElementToBuffer (PASTEBUFFER, filename, True))
	SetMode (PASTEBUFFER_MODE);
      free(filename);
      break;
#endif

    case 1:			/* filename is passed in commandline */
      filename = argv[0];
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
CommandLoadLayoutToBuffer (int argc, char **argv, int x, int y)
{
  char			*filename;
#ifdef FIXME
  static char	*current_layout_dir = NULL;
#endif

  switch (argc)
    {
#ifdef FIXME
    case 0:			/* open fileselect box */
	  filename = gui_dialog_file_select_open(_("Load layout file to buffer"),
						&current_layout_dir, Settings.FilePath);
      if (filename && LoadLayoutToBuffer (PASTEBUFFER, filename))
	SetMode (PASTEBUFFER_MODE);
      free(filename);
      break;
#endif

    case 1:			/* filename is passed in commandline */
      filename = argv[0];
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
 * quits the application after confirming
 * syntax: q
 */

static int
CommandQuit (int argc, char **argv, int x, int y)
{
  if (!PCB->Changed || gui->confirm_dialog("OK to lose data ?", 0))
    QuitApplication ();
  return 0;
}

static int
CommandReallyQuit (int argc, char **argv, int x, int y)
{
  QuitApplication ();
  return 0;
}

/* ----------------------------------------------------------------------
 * reads netlist 
 * syntax: rn [name]
 */

static int
CommandLoadNetlist (int argc, char **argv, int x, int y)
{
  char *filename, *name = NULL;
#ifdef FIXME
  static char	*current_netlist_dir = NULL;
#endif

  switch (argc)
    {
#ifdef FIXME
    case 0:			/* open fileselect box */
	  name = gui_dialog_file_select_open(_("Load netlist file"),
						&current_netlist_dir, Settings.FilePath);
	  filename = name;
      if (!filename)
	return (0);
      break;
#endif

    case 1:			/* filename is passed in commandline */
      filename = argv[0];
      break;

    default:			/* usage */
      Message ("Usage: rn [name]\n  reads in a netlist file\n");
      return (1);
    }
  if (PCB->Netlistname)
    SaveFree (PCB->Netlistname);
  PCB->Netlistname = StripWhiteSpaceAndDup (filename);
  free(name);
  return (0);
}

/* ----------------------------------------------------------------------
 * saves layout data
 * syntax: l name
 */

static int
CommandSaveLayout (int argc, char **argv, int x, int y)
{
#ifdef FIXME
  char *filename;
  static char	*current_save_path = NULL;
#endif

  switch (argc)
    {
#ifdef FIXME
    case 0:			/* query name if necessary */
      if (!PCB->Filename)
	{
	  filename = gui_dialog_file_select_save(_("Save layout as"),
						&current_save_path, NULL,
						Settings.FilePath);
	  if (filename)
	    SavePCB (filename);
	  free(filename);
	}
      else
	SavePCB (PCB->Filename);
      break;
#endif

    case 1:
      SavePCB (argv[0]);
      break;

    default:
      Message ("Usage: s [name] | w [name]\n  saves layout data\n");
      return (1);
    }
  return (0);
}

/* ---------------------------------------------------------------------------
 * saves the layout data and quits
 */

static int
CommandSaveLayoutAndQuit (int argc, char **argv, int x, int y)
{
  if (!CommandSaveLayout (argc, argv, x, y))
    return CommandQuit (0, 0, 0, 0);
  return (1);
}

/* --------------------------------------------------------------------------- */

HID_Action command_action_list[] = {
  { "h",  0, 0, CommandHelp },
  { "l",  0, 0, CommandLoadLayout },
  { "le", 0, 0, CommandLoadElementToBuffer },
  { "m",  0, 0, CommandLoadLayoutToBuffer },
  { "q",  0, 0, CommandQuit },
  { "q!", 0, 0, CommandReallyQuit },
  { "rn", 0, 0, CommandLoadNetlist },
  { "s",  0, 0, CommandSaveLayout },
  { "w",  0, 0, CommandSaveLayout },
  { "wq", 0, 0, CommandSaveLayoutAndQuit },
};
REGISTER_ACTIONS(command_action_list)
