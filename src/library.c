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

static char *rcsid = "$Id$";

/* library-element select box
 * some of the actions are local to this module
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "global.h"

#include "buffer.h"
#include "data.h"
#include "library.h"
#include "mymem.h"
#include "selector.h"
#include "set.h"

#include <X11/Shell.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/List.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static Widget CreateTypeSelector (Widget, Widget, Widget);
static void UpdateCircuitSelector (LibraryMenuTypePtr);
static void CB_Type (Widget, XtPointer, XtPointer);
static void CB_Circuit (Widget, XtPointer, XtPointer);

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static Widget TypeLabel;
static SelectorType TypeSelector =
  { "typeList", NULL, NULL, CB_Type, (XtPointer) & TypeSelector,
  0, 0, NULL, NULL
};
static SelectorType CircuitSelector =
  { "circuitList", NULL, NULL, CB_Circuit, (XtPointer) & CircuitSelector,
  0, 0, NULL, NULL
};

/* ----------------------------------------------------------------------
 * creates menu-button with entries for all predefined types
 */
static Widget
CreateTypeSelector (Widget Parent, Widget Top, Widget Left)
{
  Widget selector;

  /* create the selector, add all entries and sort them */
  selector = CreateSelector (Parent, Top, Left, &TypeSelector, 1);
  MENU_LOOP (&Library,
	     AddEntryToSelector (menu->Name, (XtPointer) menu,
				 &TypeSelector););
  return (selector);
}

/* ---------------------------------------------------------------------------
 * setup displaying of the entries of the passed library menu
 */
static void
UpdateCircuitSelector (LibraryMenuTypePtr Menu)
{
  FreeSelectorEntries (&CircuitSelector);
  ENTRY_LOOP (Menu,
	      AddEntryToSelector (MyStrdup
				  (entry->ListEntry,
				   "UpdateCircuitSelector()"),
				  (XtPointer) entry, &CircuitSelector););
  UpdateSelector (&CircuitSelector);

  /* update label */
  XtVaSetValues (TypeLabel, XtNlabel, Menu->Name, NULL);
}

/* ---------------------------------------------------------------------------
 * callback function for for the circuit type selector
 */
static void
CB_Type (Widget W, XtPointer ClientData, XtPointer CallData)
{
  XawListReturnStruct *selected = XawListShowCurrent (W);

  if (selected->list_index != XAW_LIST_NONE)
    UpdateCircuitSelector (
			   (LibraryMenuTypePtr)
			   TypeSelector.Entries[selected->list_index].
			   ClientData);
}

/* ---------------------------------------------------------------------------
 * callback function for element selections
 */
static void
CB_Circuit (Widget W, XtPointer ClientData, XtPointer CallData)
{
  static char *arguments = NULL;
  static size_t max = 0;
  size_t length;
  XawListReturnStruct *selected = XawListShowCurrent (W);

  if (selected->list_index != XAW_LIST_NONE)
    {
      LibraryEntryTypePtr entry;

      entry =
	(LibraryEntryTypePtr) CircuitSelector.Entries[selected->
						      list_index].ClientData;

      if (entry->Template == (char *) -1)
	{
	  if (LoadElementToBuffer (PASTEBUFFER, entry->AllocatedMemory, True))
	    SetMode (PASTEBUFFER_MODE);
	  return;
	}

      /* create a string that holds all parameters that are
       * passed to the library command as an argument
       */
      length = strlen (EMPTY (entry->Template)) + 3 +
	strlen (EMPTY (entry->Package)) + 3 +
	strlen (EMPTY (entry->Value)) + 3;
      if (length >= max)
	{
	  max = 2 * length;
	  arguments =
	    MyRealloc (arguments, max * sizeof (char), "CB_Circuit()");
	}
      sprintf (arguments, "'%s' '%s' '%s'", EMPTY (entry->Template),
	       EMPTY (entry->Value), EMPTY (entry->Package));
      if (LoadElementToBuffer (PASTEBUFFER, arguments, False))
	SetMode (PASTEBUFFER_MODE);
    }
}

/* ---------------------------------------------------------------------------
 * creates the library-element select box
 */
void
InitLibraryWindow (Widget Parent)
{
  static char *windowName = NULL;
  Widget popup, masterform, label, type, circuit;

  if (!windowName)
    {
      windowName = MyCalloc (strlen (Progname) + 9, sizeof (char),
			     "InitLibraryWindow()");
      sprintf (windowName, "%s-library", Progname);
    }

  /* create shell window with text-widget and exit button */
  popup = XtVaCreatePopupShell ("library", topLevelShellWidgetClass,
				Parent,
				XtNtitle, windowName,
				XtNiconName, windowName, NULL);

  masterform = XtVaCreateManagedWidget ("libraryMasterForm", formWidgetClass,
					popup,
					XtNresizable, False,
					XtNfromHoriz, NULL,
					XtNfromVert, NULL, NULL);
  label = XtVaCreateManagedWidget ("comment", labelWidgetClass,
				   masterform,
				   XtNfromHoriz, NULL,
				   XtNfromVert, NULL,
				   LAYOUT_TOP,
				   XtNlabel, "selected group:", NULL);
  TypeLabel = XtVaCreateManagedWidget ("type", labelWidgetClass,
				       masterform,
				       XtNresizable, True,
				       XtNfromHoriz, label,
				       XtNfromVert, NULL, LAYOUT_TOP, NULL);
  type = CreateTypeSelector (masterform, label, NULL);
  XtVaSetValues (type, LAYOUT_LEFT, NULL);
  circuit = CreateSelector (masterform, label, type, &CircuitSelector, 1);

  /* display the whole bunch */
  XtPopup (popup, XtGrabNone);

  /* start with the first menu */
  UpdateSelector (&TypeSelector);
  UpdateCircuitSelector (Library.Menu);

  Library.Wind = XtWindow (popup);
  /* register 'delete window' message (will be ignored) */
  XSetWMProtocols (Dpy, XtWindow (popup), &WMDeleteWindowAtom, 1);
}
