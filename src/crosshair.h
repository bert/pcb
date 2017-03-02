/*!
 * \file src/crosshair.h
 *
 * \brief Prototypes for crosshair routines.
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

#ifndef	PCB_CROSSHAIR_H
#define	PCB_CROSSHAIR_H

#include "global.h"

/* ---------------------------------------------------------------------------
 * all possible states of an attached object
 */
#define	STATE_FIRST		0	/*!< initial state. */
#define	STATE_SECOND	1
#define	STATE_THIRD		2

#define ALLOWED_DIR_0_DEGREES   (1 << 0)
#define ALLOWED_DIR_45_DEGREES  (1 << 1)
#define ALLOWED_DIR_90_DEGREES  (1 << 2)
#define ALLOWED_DIR_135_DEGREES (1 << 3)
#define ALLOWED_DIR_180_DEGREES (1 << 4)
#define ALLOWED_DIR_225_DEGREES (1 << 5)
#define ALLOWED_DIR_270_DEGREES (1 << 6)
#define ALLOWED_DIR_315_DEGREES (1 << 7)



Coord GridFit (Coord x, Coord grid_spacing, Coord grid_offset);
void notify_crosshair_change (bool changes_complete);
void notify_mark_change (bool changes_complete);
void HideCrosshair (void);
void RestoreCrosshair (void);
void DrawAttached (hidGC gc);
void DrawMark (hidGC gc);
bool MoveCrosshairAbsolute (Coord, Coord);
void SetCrosshairRange (Coord, Coord, Coord, Coord);
void InitCrosshair (void);
void DestroyCrosshair (void);
void FitCrosshairIntoGrid (Coord, Coord);

#endif
