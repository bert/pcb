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
 *  RCS: $Id$
 */

/* prototypes for menu handling
 */

#ifndef	__MENU_INCLUDED__
#define	__MENU_INCLUDED__

#include "global.h"

/* ---------------------------------------------------------------------------
 * some menu types
 */
typedef struct						/* a single popup menu entry */
{
	String			Name,			/* the widgets name */
					Label;			/* menu item label, '---' for a seperator */
	XtCallbackProc	Callback;
	XtPointer		ClientData;
	Widget			W;
} PopupEntryType, *PopupEntryTypePtr;

typedef struct						/* a popup menu */
{
	String				Name,		/* the widgets name */
						Label;		/* the menu label */
	PopupEntryTypePtr	Entries;	/* pointer to popup menu */
	XtCallbackProc		CB_Popup,	/* called on popup */
						CB_Popdown;	/* and popdown */
	Widget			W;
} PopupMenuType, *PopupMenuTypePtr;

typedef struct
{
	String				Name,		/* the widgets name */
						Label;		/* the buttontext */
	PopupMenuTypePtr	PopupMenu;	/* pointer to popup menu */
	Widget			W;
} MenuButtonType, *MenuButtonTypePtr;

typedef struct
{
	String			Name,			/* the widgets name */
					Label;			/* the buttontext */
	XtCallbackProc	Callback;		/* called on selection */
	XtPointer		ClientData;		/* data passed to callback */
	Widget			W;
} CommandButtonType, *CommandButtonTypePtr;

/* ---------------------------------------------------------------------------
 * some prototypes
 */
Widget	InitMenuButton(Widget, MenuButtonTypePtr, Widget, Widget);
void	InitMenu(Widget, Widget, Widget);
void	DumpMenu();
void	RemoveCheckFromMenu(PopupMenuTypePtr);
void	CheckEntry(PopupMenuTypePtr, String);


#endif
