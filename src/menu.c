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

RCSID("$Id$");




#include "pcb-menu.h"

/* ---------------------------------------------------------------------------
 * include icon data
 */
#include "check_icon.data"

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void InitPopupTree (Widget, PopupEntryTypePtr);
static void InitPopupMenu (Widget, PopupMenuTypePtr);
static Boolean InitCheckPixmap (void);

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static Pixmap Check = BadAlloc;

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
  Resource *res, *mres, *pres;
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

  pres = resource_subres (res, "PopupMenu");
  if (pres)
    MenuCreatePopup(Parent, pres);

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
