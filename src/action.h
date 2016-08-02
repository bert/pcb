/*!
 * \file src/action.h
 *
 * \brief Prototypes for action routines.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 */

#ifndef	PCB_ACTION_H
#define	PCB_ACTION_H

#include "global.h"

#define CLONE_TYPES LINE_TYPE | ARC_TYPE | VIA_TYPE | POLYGON_TYPE

void ActionAdjustStyle (char *);
void EventMoveCrosshair (int, int);

void AdjustAttachedObjects (void);

void warpNoWhere (void);

/* In gui-misc.c */
// FIXME: No function named ActionGetXY exists now, so maybe this prototype
// should go away.  Check the other prototypes in this interface also.
bool ActionGetLocation (char *);
void ActionGetXY (char *);

#endif
