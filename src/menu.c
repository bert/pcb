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

/* initializes menus and handles callbacks
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <ctype.h>
#include <sys/types.h>
#include <X11/Intrinsic.h>

#include "global.h"

#include "action.h"
#include "buffer.h"
#include "data.h"
#include "dialog.h"
#include "draw.h"
#include "gui.h"
#include "mymem.h"
#include "menu.h"
#include "misc.h"
#include "output.h"
#include "sizedialog.h"
#include "resource.h"
#include "resmenu.h"

#include <X11/cursorfont.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#include "pcb-menu.h"

/* ---------------------------------------------------------------------------
 * include icon data
 */
#include "check_icon.data"

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void CB_Action (Widget, XtPointer, XtPointer);
static void InitPopupTree (Widget, PopupEntryTypePtr);
static void InitPopupMenu (Widget, PopupMenuTypePtr);
static Boolean InitCheckPixmap (void);

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static Pixmap Check = BadAlloc;

/*-----------------------------------------------------------------------------
 * pop up menu for third mouse button
 */
static PopupEntryType pMenuEntries[] = {
  {"header", "Operations on Selections", NULL, NULL, NULL},
  {"unselect", "Unselect All", CB_Action, "Unselect,All", NULL},
  {"remove", "Remove Selected", CB_Action,
   "RemoveSelected", NULL},
  {"copy", "Copy Selection to Buffer", CB_Action,
   "PasteBuffer,Clear\n" "PasteBuffer,AddSelected\n" "Mode,PasteBuffer",
   NULL},
  {"cut", "Cut Selection to Buffer", CB_Action,
   "PasteBuffer,Clear\n"
   "PasteBuffer,AddSelected\n" "RemoveSelected\n" "Mode,PasteBuffer",
   NULL},
  {"convert", "Convert Selection to Element", CB_Action,
   "Select,Convert", NULL},
  {"bash", "break element to pieces", CB_Action,
   "RipUp,Element", NULL},
  {"autoplace", "Auto-place Selected", CB_Action,
   "AutoPlaceSelected", NULL},
  {"autoroute", "Auto-route Selected Rats", CB_Action,
   "AutoRoute,SelectedRats", NULL},
  {"ripup", "Rip-up selected auto-routed tracks", CB_Action,
   "RipUp,Selected", NULL},
  {"header", "Operations on This Location", NULL, NULL, NULL},
  {"hide", "Toggle Name Visibility", CB_Action,
   "ToggleHideName,Selected", NULL},
  {"edit", "Edit Name", CB_Action, "ChangeName,Object", NULL},
  {"report", "Object Report", CB_Action, "Report,Object", NULL},
  {"rotate", "Rotate Object CCW", CB_Action,
   "Mode,Save\n" "Mode,Rotate\n" "Mode,Notify\n" "Mode,Restore", NULL},
  {"rotate", "Rotate Object CW", CB_Action,
   "Mode,Save\n" "Atomic,Save\n" "Mode,Rotate\n" "Mode,Notify\n"
   "Atomic,Restore" "Mode,Notify\n" " Atomic,Restore"
   "Mode,Notify\n" "Atomic,Block" "Mode,Restore", NULL},
  {"flip", "Send To Other Side", CB_Action, "Flip,Object", NULL},
  {"therm", "Toggle Thermal", CB_Action,
   "Mode,Save\n" "Mode,Thermal\n" "Mode,Notify\n" "Mode,Restore", NULL},
  {"lookup", "Lookup Connections", CB_Action,
   "Connection,Find", NULL},
#if 0
  {"mark", "Position reference here", CB_Action,
   "MarkCrosshair,Center", NULL},
#endif
  {"line", NULL, NULL, NULL, NULL},
  {"undo", "Undo Last Operation", CB_Action, "Undo", NULL},
  {"redo", "Redo Last Undone Operation", CB_Action, "Redo", NULL},
  {"line", NULL, NULL, NULL, NULL},
  {"lmode", "Line Tool", CB_Action, "Mode,Line", NULL},
  {"vmode", "Via Tool", CB_Action, "Mode,Via", NULL},
  {"rmode", "Rectangle Tool", CB_Action, "Mode,Rectangle", NULL},
  {"amode", "Selection Tool", CB_Action, "Mode,Arrow", NULL},
  {"tmode", "Text Tool", CB_Action, "Mode,Text", NULL},
  {"nmode", "Panner Tool", CB_Action, "Mode,None", NULL},
  {NULL, NULL, NULL, NULL, NULL}
};
static PopupMenuType pMenu =
  { "pmenu", NULL, pMenuEntries, NULL, NULL, NULL };
static PopupEntryType p2MenuEntries[] = {
  {"nmode", "No Tool", CB_Action, "Mode,None", NULL},
  {NULL, NULL, NULL, NULL, NULL}
};
static PopupMenuType p2Menu =
  { "p2menu", NULL, p2MenuEntries, NULL, NULL, NULL };

/* --------------------------------------------------------------------------- */

/* FLAG(style,FlagCurrentStyle) */
int
FlagCurrentStyle()
{
  STYLE_LOOP (PCB);
  {
    if (style->Thick == Settings.LineThickness &&
	style->Diameter == Settings.ViaThickness &&
	style->Hole == Settings.ViaDrillingHole)
      return n+1;
  }
  END_LOOP;
  return 0;
}

/* MENU(sizes,SizesMenuInclude) */
void
SizesMenuInclude(Resource *menu)
{
  char tmp[64];
  int i;

  for (i = 0; i <  NUM_STYLES; i++)
    {
      Resource *acc;
      Resource *sub = resource_create(menu);
      resource_add_val (sub, 0, PCB->RouteStyle[i].Name, 0);
      sprintf(tmp, "SizesLabel(%d,use)", i);
      resource_add_val (sub, 0, MyStrdup(tmp, "SizesMenuInclude"), 0);
      sprintf(tmp, "RouteStyle(%d)", i+1);
      resource_add_val (sub, 0, MyStrdup(tmp, "SizesMenuInclude"), 0);
      sprintf(tmp, "CheckWhen(style,%d)", i+1);
      resource_add_val (sub, 0, MyStrdup(tmp, "SizesMenuInclude"), 0);
      resource_add_val (menu, 0, 0, sub);

      acc = resource_create(sub);
      sprintf(tmp, "Ctrl-%d", i+1);
      resource_add_val (acc, 0, MyStrdup(tmp, "SizesMenuInclude"), 0);
      sprintf(tmp, "Ctrl<Key>%d", i+1);
      resource_add_val (acc, 0, MyStrdup(tmp, "SizesMenuInclude"), 0);
      resource_add_val (sub, "a", 0, acc);
    }
  for (i = 0; i <  NUM_STYLES; i++)
    {
      Resource *sub = resource_create(menu);
      resource_add_val (sub, 0, PCB->RouteStyle[i].Name, 0);
      sprintf(tmp, "SizesLabel(%d,set)", i);
      resource_add_val (sub, 0, MyStrdup(tmp, "SizesMenuInclude"), 0);
      sprintf(tmp, "AdjustStyle(%d)", i+1);
      resource_add_val (sub, 0, MyStrdup(tmp, "SizesMenuInclude"), 0);
      resource_add_val (menu, 0, 0, sub);
    }
}

/* ACTION(SizesLabel,ActionSizesLabel) POPUP */
void
ActionSizesLabel (Widget w, XEvent * e, String * argv, Cardinal * argc)
{
  char tmp[64];
  int i = atoi(argv[0]);

  if (strcmp(argv[1], "use") == 0)
    sprintf(tmp, "Use '%s' routing style", PCB->RouteStyle[i].Name);
  else
    sprintf(tmp, "Adjust '%s' sizes ...", PCB->RouteStyle[i].Name);
  XtVaSetValues (w, XtNlabel, tmp, NULL);
}

/* ----------------------------------------------------------------------
 * menu callback for window menu
 */

/* ACTION(DoWindows,ActionDoWindows) */
void
ActionDoWindows (Widget w, XEvent * e, String * argv, Cardinal * argc)
{
  int n;

  if (*argc == 0)
    return;
  n = atoi (argv[0]);

  switch (n)
    {
    case 1:
      XRaiseWindow (Dpy, XtWindow (Output.Toplevel));
      break;
    case 2:
      XMapRaised (Dpy, Library.Wind);
      break;
    case 3:
      if (Settings.UseLogWindow)
	XMapRaised (Dpy, LogWindID);
      break;
    case 4:
      if (PCB->NetlistLib.Wind)
	XMapRaised (Dpy, PCB->NetlistLib.Wind);
      break;
    }
}

/* ----------------------------------------------------------------------
 * menu callback interface for actions routines that don't need
 * position information
 *
 * ClientData passes a pointer to a comma seperated list of arguments.
 * The first one determines the action routine to be called, the
 * rest of them are arguments to the action routine
 *
 * if more than one action is to be called a new list is seperated
 * by '\n'
 */
static void
CB_Action (Widget W, XtPointer ClientData, XtPointer CallData)
{
  static char **array = NULL;
  static size_t number = 0;
  int n;
  char *copy, *current, *next, *token;

  /* get a copy of the string and split it */
  copy = MyStrdup ((char *) ClientData, "CB_CallActionWithoutPosition()");

  /* first loop over all action routines;
   * strtok cannot be used in nested loops because it saves
   * a pointer in a private data area which would be corrupted
   * by the inner loop
   */
  for (current = copy; current; current = next)
    {
      /* lookup seperating '\n' character;
       * update pointer if not at the end of the string
       */
      for (next = current; *next && *next != '\n'; next++);
      if (*next)
	{
	  *next = '\0';
	  next++;
	}
      else
	next = NULL;

      token = strtok (current, ",");
      for (n = 0; token; token = strtok (NULL, ","), n++)
	{
	  /* allocate memory if necessary */
	  if (n >= number)
	    {
	      number += 10;
	      array = MyRealloc (array, number * sizeof (char *),
				 "CB_CallActionWithoutPosition()");
	    }
	  array[n] = token;
	}
      /* call action routine */
      CallActionProc (Output.Output, array[0], NULL, &array[1], n - 1);
    }

  /* release memory */
  SaveFree (copy);
}





/* FLAG(grid,FlagGrid) */
int
FlagGrid()
{
  return (int)PCB->Grid;
}

/* FLAG(zoom,FlagZoom) */
int
FlagZoom()
{
  int zoom = (int)(PCB->Zoom + (PCB->Zoom < 0 ? -0.5 : 0.5));
  if (abs(PCB->Zoom - zoom) > 0.1)
    zoom = -8; /* not close enough to integer value */
  return zoom;
}

/* FLAG(elementname,FlagElementName) */
int
FlagElementName()
{
  if (TEST_FLAG (NAMEONPCBFLAG, PCB))
    return 2;
  if (TEST_FLAG (DESCRIPTIONFLAG, PCB))
    return 1;
  return 3;
}

/* FLAG(gridfactor,FlagGridFactor) */
int
FlagGridFactor()
{
  return GetGridFactor() != 0;
}

/* FLAG(shownumber,FlagTESTFLAG,SHOWNUMBERFLAG) */
/* FLAG(localref,FlagTESTFLAG,LOCALREFFLAG) */
/* FLAG(checkplanes,FlagTESTFLAG,CHECKPLANESFLAG) */
/* FLAG(showdrc,FlagTESTFLAG,SHOWDRCFLAG) */
/* FLAG(rubberband,FlagTESTFLAG,RUBBERBANDFLAG) */
/* FLAG(description,FlagTESTFLAG,DESCRIPTIONFLAG) */
/* FLAG(nameonpcb,FlagTESTFLAG,NAMEONPCBFLAG) */
/* FLAG(autodrc,FlagTESTFLAG,AUTODRCFLAG) */
/* FLAG(alldirection,FlagTESTFLAG,ALLDIRECTIONFLAG) */
/* FLAG(swapstartdir,FlagTESTFLAG,SWAPSTARTDIRFLAG) */
/* FLAG(uniquename,FlagTESTFLAG,UNIQUENAMEFLAG) */
/* FLAG(clearnew,FlagTESTFLAG,CLEARNEWFLAG) */
/* FLAG(snappin,FlagTESTFLAG,SNAPPINFLAG) */
/* FLAG(showmask,FlagTESTFLAG,SHOWMASKFLAG) */
/* FLAG(thindraw,FlagTESTFLAG,THINDRAWFLAG) */
/* FLAG(orthomove,FlagTESTFLAG,ORTHOMOVEFLAG) */
/* FLAG(liveroute,FlagTESTFLAG,LIVEROUTEFLAG) */
int
FlagTESTFLAG(int bit)
{
  return TEST_FLAG(bit, PCB) ? 1 : 0;
}

/* FLAG(clearline,FlagSETTINGS,XtOffsetOf(SettingType,ClearLine)) */
/* FLAG(uniquenames,FlagSETTINGS,XtOffsetOf(SettingType,UniqueNames)) */
/* FLAG(uselogwindow,FlagSETTINGS,XtOffsetOf(SettingType,UseLogWindow)) */
/* FLAG(raiselogwindow,FlagSETTINGS,XtOffsetOf(SettingType,RaiseLogWindow)) */
/* FLAG(showsolderside,FlagSETTINGS,XtOffsetOf(SettingType,ShowSolderSide)) */
/* FLAG(savelastcommand,FlagSETTINGS,XtOffsetOf(SettingType,SaveLastCommand)) */
/* FLAG(saveintmp,FlagSETTINGS,XtOffsetOf(SettingType,SaveInTMP)) */
/* FLAG(drawgrid,FlagSETTINGS,XtOffsetOf(SettingType,DrawGrid)) */
/* FLAG(ratwarn,FlagSETTINGS,XtOffsetOf(SettingType,RatWarn)) */
/* FLAG(stipplepolygons,FlagSETTINGS,XtOffsetOf(SettingType,StipplePolygons)) */
/* FLAG(alldirectionlines,FlagSETTINGS,XtOffsetOf(SettingType,AllDirectionLines)) */
/* FLAG(rubberbandmode,FlagSETTINGS,XtOffsetOf(SettingType,RubberBandMode)) */
/* FLAG(swapstartdirection,FlagSETTINGS,XtOffsetOf(SettingType,SwapStartDirection)) */
/* FLAG(showdrc,FlagSETTINGS,XtOffsetOf(SettingType,ShowDRC)) */
/* FLAG(liveroute,FlagSETTINGS,XtOffsetOf(SettingType,liveRouting)) */
/* FLAG(resetafterelement,FlagSETTINGS,XtOffsetOf(SettingType,ResetAfterElement)) */
/* FLAG(ringbellwhenfinished,FlagSETTINGS,XtOffsetOf(SettingType,RingBellWhenFinished)) */
int
FlagSETTINGS(int ofs)
{
  return *(Boolean *)((char *)(&Settings) + ofs);
}

/* FLAG(netlist,FlagNetlist) */
int
FlagNetlist()
{
  return PCB->NetlistLib.Wind != 0;
}

/* ---------------------------------------------------------------------------
 * remove all 'check' symbols from menu entries
 */
void
RemoveCheckFromMenu (PopupMenuTypePtr MenuPtr)
{
  PopupEntryTypePtr entries = MenuPtr->Entries;

  for (; entries->Name; entries++)
    if (entries->Label)
      XtVaSetValues (entries->W, XtNleftBitmap, None, NULL);
}

/* ---------------------------------------------------------------------------
 * add 'check' symbol to menu entry
 */
void
CheckEntry (PopupMenuTypePtr MenuPtr, String WidgetName)
{
  PopupEntryTypePtr entries = MenuPtr->Entries;

  if (InitCheckPixmap ())
    for (; entries->Name; entries++)
      if (entries->Label && !strcmp (entries->Name, (char *) WidgetName))
	{
	  XtVaSetValues (entries->W, XtNleftBitmap, Check, NULL);
	  return;
	}
}

/* ---------------------------------------------------------------------------
 * initializes a menu tree
 * depending on the 'Name' field of the struct either a smeBSB or a
 * smeLine widget is created. If a callback routine is defined for the
 * smeBSB widget it will be registered else the entry will be disabled
 */
static void
InitPopupTree (Widget Parent, PopupEntryTypePtr EntryPtr)
{
  for (; EntryPtr->Name; EntryPtr++)
    {
      /* check if it's only a seperator */
      if (EntryPtr->Label)
	{
	  EntryPtr->W =
	    XtVaCreateManagedWidget (EntryPtr->Name, smeBSBObjectClass,
				     Parent, XtNlabel, EntryPtr->Label,
				     XtNleftMargin, 12, XtNsensitive, True,
				     NULL);
	  if (EntryPtr->Callback)
	    XtAddCallback (EntryPtr->W, XtNcallback,
			   EntryPtr->Callback,
			   (XtPointer) EntryPtr->ClientData);
	  else
	    /* entry is not selectable */
	    XtVaSetValues (EntryPtr->W,
			   XtNsensitive, False, XtNvertSpace, 60, NULL);
	}
      else
	XtVaCreateManagedWidget ("menuLine", smeLineObjectClass, Parent,
				 NULL);
    }
}

/* ---------------------------------------------------------------------------
 * initializes one popup menu
 * create a popup shell, add all entries to it and register the popup and
 * popdown callback functions if required
 */
static void
InitPopupMenu (Widget Parent, PopupMenuTypePtr MenuPtr)
{
  Cursor point = XCreateFontCursor (Dpy, XC_left_ptr);
  MenuPtr->W = XtVaCreatePopupShell (MenuPtr->Name, simpleMenuWidgetClass,
				     Parent,
				     XtNlabel, MenuPtr->Label,
				     XtNsensitive, True,
				     XtNcursor, point, NULL);
  InitPopupTree (MenuPtr->W, MenuPtr->Entries);

  /* install popup and popdown callbacks */
  if (MenuPtr->CB_Popup)
    XtAddCallback (MenuPtr->W, XtNpopupCallback, MenuPtr->CB_Popup, NULL);
  if (MenuPtr->CB_Popdown)
    XtAddCallback (MenuPtr->W, XtNpopupCallback, MenuPtr->CB_Popdown, NULL);
}

/* ---------------------------------------------------------------------------
 * initializes one menubutton plus it's menu
 * create a menu button widget first than add the popup shell to it
 */
Widget
InitMenuButton (Widget Parent,
		MenuButtonTypePtr MenuButtonPtr, Widget Top, Widget Left)
{
  MenuButtonPtr->W =
    XtVaCreateManagedWidget (MenuButtonPtr->Name, menuButtonWidgetClass,
			     Parent, XtNlabel, MenuButtonPtr->Label,
			     XtNmenuName, MenuButtonPtr->PopupMenu->Name,
			     XtNfromHoriz, Left, XtNfromVert, Top, LAYOUT_TOP,
			     NULL);
  XtAddEventHandler (MenuButtonPtr->W, EnterWindowMask, False, CB_StopScroll,
		     NULL);
  InitPopupMenu (MenuButtonPtr->W, MenuButtonPtr->PopupMenu);

  /* return the created button widget to position some others */
  return (MenuButtonPtr->W);
}

/* ---------------------------------------------------------------------------
 * initializes 'check' pixmap if not already done
 */
static Boolean
InitCheckPixmap (void)
{
  if (Check == BadAlloc)
    Check = XCreateBitmapFromData (Dpy,
				   RootWindowOfScreen (XtScreen
						       (Output.Toplevel)),
				   check_icon_bits, check_icon_width,
				   check_icon_height);
  return (Check != BadAlloc ? True : False);
}

/* ---------------------------------------------------------------------------
 * initializes button related menus
 */

void
InitMenu (Widget Parent, Widget Top, Widget Left)
{
  Widget last;
  Resource *res, *mres;
  char *home = getenv("HOME");
  char *menu_file = 0;

  if (FileExists("pcb-menu.res"))
    menu_file = "pcb-menu.res";
  if (home && !menu_file)
    {
      menu_file = Concat (home, "/.pcb-menu.res", 0);
      if (! FileExists(menu_file))
	menu_file = 0;
    }
  if (!menu_file && FileExists(Settings.MenuFile))
    menu_file = Settings.MenuFile;

  if (menu_file)
    res = resource_parse(menu_file, 0);
  else
    {
      menu_file = "<internal default>";
      res = resource_parse (0, pcb_menu_default);
    }

  if (!res)
    {
      fprintf(stderr, "Unable to parse menu file %s\n", menu_file);
      return;
    }
  mres = resource_subres (res, "MainMenu");
  if (!mres)
    {
      fprintf(stderr, "Cannot find MainMenu resource in %s, trying internal defaults\n", menu_file);
      menu_file = "<internal default>";
      res = resource_parse (0, pcb_menu_default);
      mres = resource_subres (res, "MainMenu");
      if (!mres)
	{
	  fprintf (stderr, "Cannot find MainMenu resource internally either\n");
	  exit (1);
	}
    }
  last = MenuCreateFromResource (Parent, mres, Top, Left, 1);

  InitPopupMenu (Parent, &pMenu);
  InitPopupMenu (Parent, &p2Menu);
  Output.Menu = last;
}

void
DumpMenu()
{
  int i;
  for (i=0; pcb_menu_default[i]; i++)
    puts(pcb_menu_default[i]);
  exit (0);
}
