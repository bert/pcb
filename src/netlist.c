/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  This particular file, netlist.c, was written by and
 *  is Copyright (C) 1998, 1999, 2000, 2001 harry eaton
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998 Thomas Nau
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

/* netlist selection tool
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

#include "create.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "find.h"
#include "netlist.h"
#include "misc.h"
#include "mymem.h"
#include "rats.h"
#include "search.h"
#include "select.h"
#include "selector.h"
#include "set.h"
#include "undo.h"

#include <X11/Shell.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/MenuButton.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static Widget CreateNetSelector (Widget, Widget, Widget);
static void UpdateConnectionSelector (LibraryMenuTypePtr);
static void UpdateNetSelector (void);
static void CB_Net (Widget, XtPointer, XtPointer);
static void CB_Connection (Widget, XtPointer, XtPointer);
static void CB_Select (Widget, XtPointer, XtPointer);
static void CB_Enable (Widget, XtPointer, XtPointer);
static void CB_Disable (Widget, XtPointer, XtPointer);
static LibraryMenuTypePtr GetMenuFromName (char *, Boolean);
static char *ConnectionName (int, void *, void *);

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static Widget NetLabel;
static Widget StyleLabel;
static LibraryMenuTypePtr SelectedNet;
static SelectorType NetSelector =
  { "netList", NULL, NULL, CB_Net, (XtPointer) & NetSelector,
  0, 0, NULL, NULL
};
static SelectorType ConnectionSelector =
  { "connList", NULL, NULL, CB_Connection, (XtPointer) & ConnectionSelector,
  0, 0, NULL, NULL
};

/* ----------------------------------------------------------------------
 * create entries for each net 
 */
static Widget
CreateNetSelector (Widget Parent, Widget Top, Widget Left)
{
  Widget selector;

  /* create the selector, add all entries and sort them */
  selector = CreateSelector (Parent, Top, Left, &NetSelector, 1);
  MENU_LOOP (&PCB->NetlistLib, 
    {
      AddEntryToSelector (menu->Name, (XtPointer) menu, &NetSelector);
    }
  );
  return (selector);
}

/* ---------------------------------------------------------------------------
 * setup displaying of the entries of the passed net menu
 */
static void
UpdateNetSelector (void)
{
  FreeSelectorEntries (&NetSelector);
  MENU_LOOP (&PCB->NetlistLib, 
    {
      AddEntryToSelector (menu->Name, (XtPointer) menu, &NetSelector);
    }
  );
  UpdateSelector (&NetSelector);
}

/* ---------------------------------------------------------------------------
 * setup displaying of the entries of the passed net menu
 */
static void
UpdateConnectionSelector (LibraryMenuTypePtr Menu)
{
  SelectedNet = Menu;
  FreeSelectorEntries (&ConnectionSelector);
  ENTRY_LOOP (Menu, 
    {
      AddEntryToSelector (MyStrdup
			  (entry->ListEntry,
			   "UpdateCircuitSelector()"),
			  (XtPointer) entry, &ConnectionSelector);
    }
  );
  UpdateSelector (&ConnectionSelector);

  /* update label */
  XtVaSetValues (NetLabel, XtNlabel, Menu->Name + 2, NULL);
  XtVaSetValues (StyleLabel, XtNlabel, UNKNOWN (Menu->Style), NULL);
}

/* ---------------------------------------------------------------------------
 * toggle selection of pin
 */
static void
SelectPin (LibraryEntryTypePtr entry, Boolean toggle)
{
  ConnectionType conn;

  if (SeekPad (entry, &conn, False))
    {
      switch (conn.type)
	{
	case PIN_TYPE:
	  {
	    PinTypePtr pin = (PinTypePtr) conn.ptr2;

	    AddObjectToFlagUndoList (PIN_TYPE, conn.ptr1, conn.ptr2,
				     conn.ptr2);
	    if (toggle)
	      {
		TOGGLE_FLAG (SELECTEDFLAG, pin);
		CenterDisplay (TO_SCREEN_X (pin->X),
			       TO_SCREEN_Y (pin->Y), False);
	      }
	    else
	      SET_FLAG (SELECTEDFLAG, pin);
	    DrawPin (pin, 0);
	    break;
	  }
	case PAD_TYPE:
	  {
	    PadTypePtr pad = (PadTypePtr) conn.ptr2;

	    AddObjectToFlagUndoList (PAD_TYPE, conn.ptr1, conn.ptr2,
				     conn.ptr2);
	    if (toggle)
	      {
		TOGGLE_FLAG (SELECTEDFLAG, pad);
		CenterDisplay (TO_SCREEN_X (pad->Point1.X),
			       TO_SCREEN_Y (pad->Point1.Y), False);
	      }
	    else
	      SET_FLAG (SELECTEDFLAG, pad);
	    DrawPad (pad, 0);
	    break;
	  }
	}
    }
}

/* ---------------------------------------------------------------------------
 * callback function for for the individual net selectors
 */
static void
CB_Net (Widget W, XtPointer ClientData, XtPointer CallData)
{
  XawListReturnStruct *selected = XawListShowCurrent (W);
  LibraryMenuTypePtr menu = (LibraryMenuTypePtr)
    NetSelector.Entries[selected->list_index].ClientData;

  if (selected->list_index != XAW_LIST_NONE)
    {
      if (CallData == NULL)
	{
	  if (selected->string[0] == ' ')
	    {
	      menu->Name[0] = '*';
	      selected->string[0] = '*';
	    }
	  else
	    {
	      menu->Name[0] = ' ';
	      selected->string[0] = ' ';
	    }
	  XawListChange (W, NetSelector.StringList, NetSelector.Number, 0,
			 False);
	}
      else
	UpdateConnectionSelector (menu);
    }
}

/* ---------------------------------------------------------------------------
 * callback function for disabling all nets
 */
static void
CB_Disable (Widget W, XtPointer ClientData, XtPointer CallData)
{
  MENU_LOOP (&PCB->NetlistLib, 
    {
      *NetSelector.StringList[l] = '*';
      menu->Name[0] = '*';
    }
  );
  XawListChange (NetSelector.ListW, NetSelector.StringList,
		 NetSelector.Number, 0, False);
}

/* ---------------------------------------------------------------------------
 * callback function for enabling all nets
 */
static void
CB_Enable (Widget W, XtPointer ClientData, XtPointer CallData)
{
  MENU_LOOP (&PCB->NetlistLib, 
    {
      *NetSelector.StringList[l] = ' ';
      menu->Name[0] = ' ';
    }
  );
  XawListChange (NetSelector.ListW, NetSelector.StringList,
		 NetSelector.Number, 0, False);
}

/* ---------------------------------------------------------------------------
 * callback function for for the whole net selector button at the top
 */
static void
CB_Select (Widget W, XtPointer ClientData, XtPointer CallData)
{
  XawListReturnStruct *net = XawListShowCurrent (NetSelector.ListW);
  LibraryEntryTypePtr entry;
  ConnectionType Conn;
  int i;

  if (net->list_index != XAW_LIST_NONE)
    {
      LibraryMenuTypePtr Menu = (LibraryMenuTypePtr)
	NetSelector.Entries[net->list_index].ClientData;

      InitConnectionLookup ();
      ResetFoundPinsViasAndPads (False);
      ResetFoundLinesAndPolygons (False);
      SaveUndoSerialNumber ();
      for (i = Menu->EntryN, entry = Menu->Entry; i; i--, entry++)
	if (SeekPad (entry, &Conn, False))
	  RatFindHook (Conn.type, Conn.ptr1, Conn.ptr2, Conn.ptr2, True,
		       True);
      RestoreUndoSerialNumber ();
      SelectConnection (True);
      ResetFoundPinsViasAndPads (False);
      ResetFoundLinesAndPolygons (False);
      FreeConnectionLookupMemory ();
      IncrementUndoSerialNumber ();
      Draw ();
    }
}



/* ---------------------------------------------------------------------------
 * callback function for connection selections
 */
static void
CB_Connection (Widget W, XtPointer ClientData, XtPointer CallData)
{
  XawListReturnStruct *selected = XawListShowCurrent (W);

  if (CallData)
    {
      if (PCB->RatDraw && strcmp (*(char **) CallData, "Kill") == 0)
	/* delete the selected connection */
	if (selected->list_index != XAW_LIST_NONE)
	  {
	    LibraryEntryTypePtr entry;

	    entry = (LibraryEntryTypePtr)
	      ConnectionSelector.Entries[selected->list_index].ClientData;
	    SaveFree (entry->ListEntry);
	    *entry = SelectedNet->Entry[--SelectedNet->EntryN];
	    if (SelectedNet->EntryN == 0)
	      {
		/* if last connection, delete net too */
		SaveFree (SelectedNet->Name);
		*SelectedNet = PCB->NetlistLib.Menu[--PCB->NetlistLib.MenuN];
		if (PCB->NetlistLib.MenuN == 0)
		  {		/* all gone, remove the window */
		    InitNetlistWindow (Output.Toplevel);
		    return;
		  }
		UpdateNetSelector ();
		XawListHighlight (NetSelector.ListW, 0);
		SelectedNet = &PCB->NetlistLib.Menu[0];
	      }
	    UpdateConnectionSelector (SelectedNet);
	    /* bug what if that was the last net that was just removed ? */
	  }
    }
  else if (selected->list_index != XAW_LIST_NONE)
    {
      LibraryEntryTypePtr entry;

      entry = (LibraryEntryTypePtr)
	ConnectionSelector.Entries[selected->list_index].ClientData;
      /* toggle selection of the pin */
      SelectPin (entry, True);
      IncrementUndoSerialNumber ();
      Draw ();
    }
}

/* SetNetlist searches the netlist for the
 * found object, then highlights it in the netlist
 * window if it's there. It returns the netmenu pointer if it was
 * found, NULL if not. It also sets the current
 * routing style to the net's style if change_style is true
 */
void
SetNetlist (int type, void *ptr1, void *ptr2, Boolean change_style)
{
  LibraryMenuTypePtr netmenu = NULL;
  int i;
  char *name;

  name = ConnectionName (type, ptr1, ptr2);
  if (!name)
    return;
  netmenu = GetMenuFromName (name, change_style);
  if (!netmenu)
    return;
  if (change_style)
    SetRouteStyle (netmenu->Style);
  UpdateConnectionSelector (netmenu);
  /* find connection to highlight it */
  for (i = 0; i < ConnectionSelector.Number; i++)
    if (strcmp (name, ConnectionSelector.StringList[i]) == 0)
      break;
  XawListHighlight (ConnectionSelector.ListW, i);
}

/* ---------------------------------------------------------------------------
 * creates the net spreadsheet 
 */
void
InitNetlistWindow (Widget Parent)
{
  static char *windowName = NULL;
  static Widget popup = NULL,
    masterform, label, sel, enableAll, disableAll, net, connection;

  if (!windowName)
    {
      windowName = MyCalloc (strlen (Progname) + 9, sizeof (char),
			     "InitNetlistWindow()");
      sprintf (windowName, "%s-Netlist", Progname);
    }
  /* delete previous netlist window */
  if (popup != NULL)
    {
      FreeSelectorEntries (&ConnectionSelector);
      FreeSelectorEntries (&NetSelector);
      XtDestroyWidget (popup);
      PCB->NetlistLib.Wind = 0;
      popup = NULL;
    }
  /* if no menu, don't create widgets */
  if (!PCB->NetlistLib.MenuN)
    return;

  /* create shell window with text-widget and exit button */
  popup = XtVaCreatePopupShell ("netlist", topLevelShellWidgetClass,
				Parent,
				XtNtitle, windowName,
				XtNiconName, windowName, NULL);

  masterform = XtVaCreateManagedWidget ("netlistMasterForm", formWidgetClass,
					popup,
					XtNresizable, False,
					XtNfromHoriz, NULL,
					XtNfromVert, NULL, NULL);
  sel = XtVaCreateManagedWidget ("select", commandWidgetClass,
				 masterform,
				 XtNlabel, "Sel Net On Layout",
				 XtNfromVert, NULL,
				 XtNfromHoriz, NULL, LAYOUT_TOP, NULL);
  disableAll = XtVaCreateManagedWidget ("disableAll", commandWidgetClass,
					masterform,
					XtNlabel, "Disable All Nets",
					XtNfromVert, NULL,
					XtNfromHoriz, sel, LAYOUT_TOP, NULL);
  enableAll = XtVaCreateManagedWidget ("enableAll", commandWidgetClass,
				       masterform,
				       XtNlabel, "Enable All Nets",
				       XtNfromVert, NULL,
				       XtNfromHoriz, disableAll,
				       LAYOUT_TOP, NULL);
  label = XtVaCreateManagedWidget ("nodes", labelWidgetClass,
				   masterform,
				   XtNfromHoriz, NULL,
				   XtNfromVert, enableAll,
				   LAYOUT_TOP,
				   XtNlabel, "Nodes displayed for net:",
				   NULL);
  NetLabel = XtVaCreateManagedWidget ("net", labelWidgetClass,
				      masterform,
				      XtNresizable, True,
				      XtNfromHoriz, label,
				      XtNfromVert, enableAll,
				      LAYOUT_TOP, NULL);
  label = XtVaCreateManagedWidget ("stylem", labelWidgetClass,
				   masterform,
				   XtNfromHoriz, NetLabel,
				   XtNfromVert, enableAll,
				   LAYOUT_TOP, XtNlabel, "Style:", NULL);
  StyleLabel = XtVaCreateManagedWidget ("net", labelWidgetClass,
					masterform,
					XtNresizable, True,
					XtNfromHoriz, label,
					XtNfromVert, enableAll,
					LAYOUT_TOP, NULL);
  net = CreateNetSelector (masterform, label, NULL);
  XtAddCallback (sel, XtNcallback, CB_Select, net);
  XtAddCallback (enableAll, XtNcallback, CB_Enable, net);
  XtAddCallback (disableAll, XtNcallback, CB_Disable, net);
  XtVaSetValues (net, LAYOUT_LEFT, NULL);
  connection =
    CreateSelector (masterform, label, net, &ConnectionSelector, 4);

  /* display the whole bunch */
  XtPopup (popup, XtGrabNone);

  /* start with the first menu */
  UpdateSelector (&NetSelector);
  XawListHighlight (NetSelector.ListW, 0);
  UpdateConnectionSelector ((LibraryMenuTypePtr) NetSelector.
			    Entries[0].ClientData);

  PCB->NetlistLib.Wind = XtWindow (popup);
  /* register 'delete window' message (will be ignored) */
  XSetWMProtocols (Dpy, XtWindow (popup), &WMDeleteWindowAtom, 1);
}

static char *
ConnectionName (int type, void *ptr1, void *ptr2)
{
  static char name[256];
  char *num;

  switch (type)
    {
    case PIN_TYPE:
      num = ((PinTypePtr) ptr2)->Number;
      break;
    case PAD_TYPE:
      num = ((PadTypePtr) ptr2)->Number;
      break;
    default:
      return (NULL);
    }
  strcpy (name, UNKNOWN (NAMEONPCB_NAME ((ElementTypePtr) ptr1)));
  strcat (name, "-");
  strcat (name, num);
  return (name);
}

static LibraryMenuTypePtr
GetMenuFromName (char *name, Boolean EnabledOnly)
{
  LibraryMenuTypePtr netmenu = NULL;
  LibraryEntryTypePtr entry;
  int i, j, net;

  net = -1;
  /* find the name in the netlist */
  /* linear search for now */
  for (i = 0; i < NetSelector.Number; i++)
    {
      netmenu = NetSelector.Entries[i].ClientData;
      if (EnabledOnly && netmenu->Name[0] == '*')
	continue;
      for (j = netmenu->EntryN, entry = netmenu->Entry; j; j--, entry++)
	if (strcmp (name, entry->ListEntry) == 0)
	  {
	    net = i;
	    break;
	  }
      if (net == i)
	break;
    }
  if (net == -1)
    return (NULL);
  if (EnabledOnly)
    XawListHighlight (NetSelector.ListW, net);
  /* should also scroll the list so this is visible,
   * but I don't know how.
   */
  return (netmenu);
}

RatTypePtr
AddNet (void)
{
  static int ratDrawn = 0;
  char name1[256], *name2;
  Cardinal group1, group2;
  char ratname[20];
  int found;
  void *ptr1, *ptr2, *ptr3;
  LibraryMenuTypePtr menu;
  LibraryEntryTypePtr entry;

  if (Crosshair.AttachedLine.Point1.X == Crosshair.AttachedLine.Point2.X
      && Crosshair.AttachedLine.Point1.Y == Crosshair.AttachedLine.Point2.Y)
    return (NULL);

  found = SearchObjectByPosition (PAD_TYPE | PIN_TYPE, &ptr1, &ptr2, &ptr3,
				  Crosshair.AttachedLine.Point1.X,
				  Crosshair.AttachedLine.Point1.Y, 5);
  if (found == NO_TYPE)
    {
      Message ("No pad/pin under rat line\n");
      return (NULL);
    }
  if (NAMEONPCB_NAME ((ElementTypePtr) ptr1) == NULL
      || *NAMEONPCB_NAME ((ElementTypePtr) ptr1) == 0)
    {
      Message ("You must name the starting element first\n");
      return (NULL);
    }

  /* will work for pins to since the FLAG is common */
  group1 = (TEST_FLAG (ONSOLDERFLAG, (PadTypePtr) ptr2) ?
	    GetLayerGroupNumberByNumber (MAX_LAYER + SOLDER_LAYER) :
	    GetLayerGroupNumberByNumber (MAX_LAYER + COMPONENT_LAYER));
  strcpy (name1, ConnectionName (found, ptr1, ptr2));
  found = SearchObjectByPosition (PAD_TYPE | PIN_TYPE, &ptr1, &ptr2, &ptr3,
				  Crosshair.AttachedLine.Point2.X,
				  Crosshair.AttachedLine.Point2.Y, 5);
  if (found == NO_TYPE)
    {
      Message ("No pad/pin under rat line\n");
      return (NULL);
    }
  if (NAMEONPCB_NAME ((ElementTypePtr) ptr1) == NULL
      || *NAMEONPCB_NAME ((ElementTypePtr) ptr1) == 0)
    {
      Message ("You must name the ending element first\n");
      return (NULL);
    }
  group2 = (TEST_FLAG (ONSOLDERFLAG, (PadTypePtr) ptr2) ?
	    GetLayerGroupNumberByNumber (MAX_LAYER + SOLDER_LAYER) :
	    GetLayerGroupNumberByNumber (MAX_LAYER + COMPONENT_LAYER));
  name2 = ConnectionName (found, ptr1, ptr2);
  menu = GetMenuFromName (name1, False);
  if (menu)
    {
      if (GetMenuFromName (name2, False))
	{
	  Message ("Both connection alread in netlist - cannot merge nets\n");
	  return (NULL);
	}
      entry = GetLibraryEntryMemory (menu);
      entry->ListEntry = MyStrdup (name2, "AddNet");
      GetMenuFromName (name2, True);
      goto ratIt;
    }
  /* ok, the first name did not belong to a net */
  menu = GetMenuFromName (name2, False);
  if (menu)
    {
      entry = GetLibraryEntryMemory (menu);
      entry->ListEntry = MyStrdup (name1, "AddNet");
      GetMenuFromName (name1, True);
      goto ratIt;
    }
  /* neither belong to a net, so create a new one */
  menu = GetLibraryMenuMemory (&PCB->NetlistLib);
  sprintf (ratname, "  ratDrawn%i", ++ratDrawn);
  menu->Name = MyStrdup (ratname, "AddNet");
  entry = GetLibraryEntryMemory (menu);
  entry->ListEntry = MyStrdup (name1, "AddNet");
  entry = GetLibraryEntryMemory (menu);
  entry->ListEntry = MyStrdup (name2, "AddNet");
  if (!PCB->NetlistLib.Wind)
    InitNetlistWindow (Output.Toplevel);
  UpdateNetSelector ();
  GetMenuFromName (name2, True);
ratIt:
  UpdateConnectionSelector (menu);
  return (CreateNewRat (PCB->Data, Crosshair.AttachedLine.Point1.X,
			Crosshair.AttachedLine.Point1.Y,
			Crosshair.AttachedLine.Point2.X,
			Crosshair.AttachedLine.Point2.Y,
			group1, group2, Settings.RatThickness, NOFLAG));
}
