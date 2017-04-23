/*!
 * \file src/data.h
 *
 * \brief Common identifiers.
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 */

#ifndef	PCB_DATA_H
#define	PCB_DATA_H

#include "global.h"
#include "crosshair.h"

/* ---------------------------------------------------------------------------
 * some shared identifiers
 */


extern CrosshairType Crosshair;

extern MarkType Marked;

extern OutputType Output;

extern PCBType *PCB;

#define max_group (PCB->Data->LayerN)
#define max_copper_layer (PCB->Data->LayerN)
#define bottom_silk_layer (max_copper_layer + BOTTOM_SILK_LAYER)
#define top_silk_layer (max_copper_layer + TOP_SILK_LAYER)

extern SettingType Settings;

extern BufferType Buffers[MAX_BUFFER];

extern LibraryType Library;

/*extern	DeviceInfoType	PrintingDevice[];*/

extern char *Progname;
extern char *InputTranslations;

extern int addedLines;
extern int LayerStack[MAX_LAYER];

extern bool Bumped;

extern FlagType no_flags;

extern int netlist_frozen;

#endif
