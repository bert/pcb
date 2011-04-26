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

/* prototypes for crosshair routines
 */

#ifndef	__CROSSHAIR_INCLUDED__
#define	__CROSSHAIR_INCLUDED__

#include "global.h"

/* ---------------------------------------------------------------------------
 * fits screen coordinates into grid
 */
#define	GRIDFIT_X(x,g)	(int)(0.5 + ((int)(((x) -PCB->GridOffsetX + g/2) /(g)) *(g)) +PCB->GridOffsetX)
#define	GRIDFIT_Y(y,g)	(int)(0.5 + ((int)(((y) -PCB->GridOffsetY + g/2) /(g)) *(g)) +PCB->GridOffsetY)

/* ---------------------------------------------------------------------------
 * all possible states of an attached object
 */
#define	STATE_FIRST		0	/* initial state */
#define	STATE_SECOND	1
#define	STATE_THIRD		2

void notify_crosshair_change (bool changes_complete);
void notify_mark_change (bool changes_complete);
void CrosshairOn (void);
void CrosshairOff (void);
void HideCrosshair (void);
void RestoreCrosshair (void);
void DrawAttached (void);
void DrawMark (void);
void MoveCrosshairRelative (LocationType, LocationType);
bool MoveCrosshairAbsolute (LocationType, LocationType);
void SetCrosshairRange (LocationType, LocationType, LocationType,
			LocationType);
void InitCrosshair (void);
void DestroyCrosshair (void);
void FitCrosshairIntoGrid (LocationType, LocationType);

#endif
