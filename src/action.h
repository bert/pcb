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

void	ActionMovePointer(Widget, XEvent *, String *, Cardinal *);
void	ActionAtomic(Widget, XEvent *, String *, Cardinal *);
void	ActionAdjustStyle(Widget, XEvent *, String *, Cardinal *);
void	ActionRouteStyle(Widget, XEvent *, String *, Cardinal *);
void	EventMoveCrosshair(XMotionEvent *);
void	ActionMarkCrosshair(Widget, XEvent *, String *, Cardinal *);
void	ActionToggleHideName(Widget, XEvent *, String *, Cardinal *);
void	ActionSetValue(Widget, XEvent *, String *, Cardinal *);
void	ActionFinishInputDialog(Widget, XEvent *, String *, Cardinal *);
void	ActionQuit(Widget, XEvent *, String *, Cardinal *);
void	ActionConnection(Widget, XEvent *, String *, Cardinal *);
void	ActionCommand(Widget, XEvent *, String *, Cardinal *);
void	ActionDisplay(Widget, XEvent *, String *, Cardinal *);
void	ActionMode(Widget, XEvent *, String *, Cardinal *);
void	ActionMoveToSilk(Widget, XEvent *, String *, Cardinal *);
void	ActionRemoveSelected(Widget, XEvent *, String *, Cardinal *);
void	ActionDeleteRats(Widget, XEvent *, String *, Cardinal *);
void	ActionAddRats(Widget, XEvent *, String *, Cardinal *);
void	ActionAutoPlaceSelected(Widget, XEvent *, String *, Cardinal *);
void	ActionAutoRoute(Widget, XEvent *, String *, Cardinal *);
void	ActionReport(Widget, XEvent *, String *, Cardinal *);
void	ActionChangeSize(Widget, XEvent *, String *, Cardinal *);
void	ActionChange2ndSize(Widget, XEvent *, String *, Cardinal *);
void	ActionChangeClearSize(Widget, XEvent *, String *, Cardinal *);
void	ActionChangeName(Widget, XEvent *, String *, Cardinal *);
void	ActionChangeJoin(Widget, XEvent *, String *, Cardinal *);
void	ActionChangeSquare(Widget, XEvent *, String *, Cardinal *);
void	ActionChangeOctagon(Widget, XEvent *, String *, Cardinal *);
void	ActionChangeHole(Widget, XEvent *, String *, Cardinal *);
void	ActionSelect(Widget, XEvent *, String *, Cardinal *);
void	ActionUnselect(Widget, XEvent *, String *, Cardinal *);
void	ActionSave(Widget, XEvent *, String *, Cardinal *);
void	ActionLoad(Widget, XEvent *, String *, Cardinal *);
void	ActionPrint(Widget, XEvent *, String *, Cardinal *);
void	ActionNew(Widget, XEvent *, String *, Cardinal *);
void	ActionSwapSides(Widget, XEvent *, String *, Cardinal *);
void	ActionBell(Widget, XEvent *, String *, Cardinal *);
void	ActionPasteBuffer(Widget, XEvent *, String *, Cardinal *);
void	ActionUndo(Widget, XEvent *, String *, Cardinal *);
void	ActionRedo(Widget, XEvent *, String *, Cardinal *);
void	ActionPolygon(Widget, XEvent *, String *, Cardinal *);
void	ActionSwitchDrawingLayer(Widget, XEvent *, String *, Cardinal *);
void	ActionEditLayerGroups(Widget, XEvent *, String *, Cardinal *);
void	ActionMoveToCurrentLayer(Widget, XEvent *, String *, Cardinal *);
void	ActionDRCheck(Widget, XEvent *, String *, Cardinal *);
void	ActionFlip(Widget, XEvent *, String *, Cardinal *);
void	ActionListAct(Widget, XEvent *, String *, Cardinal *);
void	ActionToggleThermal(Widget, XEvent *, String *, Cardinal *);
void	ActionButton3(Widget, XEvent *, String *, Cardinal *);
void	ActionSetSame(Widget, XEvent *, String *, Cardinal *);
void	ActionRipUp(Widget, XEvent *, String *, Cardinal *);
void	AdjustAttachedObjects(void);
void	CallActionProc(Widget, String, XEvent *, String *, Cardinal);
void	warpNoWhere(void);

#endif
