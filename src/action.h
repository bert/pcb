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

/* prototypes for action routines
 */

#ifndef	__ACTION_INCLUDED__
#define	__ACTION_INCLUDED__

#include "global.h"

#define CLONE_TYPES LINE_TYPE | ARC_TYPE | VIA_TYPE | POLYGON_TYPE

void	ActionMoveObject(char *, char *, char *);
void	ActionMovePointer(char *, char *);
void	ActionAtomic(String);
void	ActionAdjustStyle(char *);
void	ActionRouteStyle(char *);
void	EventMoveCrosshair(int, int);
void	ActionMarkCrosshair(char *);
void	ActionToggleHideName(char *);
void	ActionSetValue(char *, char *, char *);
void	ActionSetThermal(char *);
void	ActionClearThermal(char *);
void	ActionSetSquare(char *);
void	ActionClearSquare(char *);
void	ActionSetOctagon(char *);
void	ActionClearOctagon(char *);
void	ActionQuit(void);
void	ActionConnection(char *);
void	ActionCommand(char *);
void	ActionDisperseElements(char *);
void	ActionDisplay(char *, char *);
void	ActionMode(char *);
void	ActionRemoveSelected(void);
void	ActionDeleteRats(char *);
void	ActionAddRats(char *);
void	ActionAutoPlaceSelected(void);
void	ActionAutoRoute(char *);
void	ActionReport(char *);
void	ActionChangeSize(char *, char *, char *);
void	ActionChange2ndSize(char *, char *, char *);
void	ActionChangeClearSize(char *, char *, char *);
void	ActionChangeName(char *);
void	ActionChangeJoin(char *);
void	ActionChangeSquare(char *);
void	ActionChangeOctagon(char *);
void	ActionChangeHole(char *);
void	ActionSelect(char *);
void	ActionUnselect(char *);
void	ActionSave(char *);
void	ActionLoad(char *);
void	ActionPrint(void);
void	ActionNew(void);
void	ActionSwapSides(void);
void	ActionBell(char *);
void	ActionPasteBuffer(char *, char *);
void	ActionUndo(char *);
void	ActionRedo(void);
void	ActionPolygon(char *);
void	ActionSwitchDrawingLayer(char *);
void	ActionToggleVisibility(char *);
void	ActionMoveToCurrentLayer(char *);
void	ActionDRCheck(void);
void	ActionFlip(char *);
void	ActionToggleThermal(char *);

#if 0
void	ActionListAct(Widget, XEvent *, String *, Cardinal *);
void	ActionButton3(Widget, XEvent *, String *, Cardinal *);
#endif

void	ActionSetSame(void);
void	ActionRipUp(char *);
void	AdjustAttachedObjects(void);

void	warpNoWhere(void);
void	ActionSetFlag(char *, char *);
void	ActionClrFlag(char *, char *);
void	ActionChangeFlag(char *, char *, int);

void	ActionExecuteFile(gchar *);
void	ActionExecuteAction(gchar **, gint);

/* In gui-misc.c */
gboolean	ActionGetLocation(gchar *);
void	ActionGetXY(gchar *);
#endif
