/*!
 * \file src/data.c
 *
 * \brief Just defines common identifiers.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "data.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some shared identifiers
 */

CrosshairType Crosshair;	/*!< information about cursor settings. */
MarkType Marked;		/*!< a cross-hair mark. */
OutputType Output;		/*!< some widgets ... used for drawing. */
PCBType *PCB;			/*!< pointer to layout struct. */

char *Progname;
SettingType Settings;
int LayerStack[MAX_LAYER];	/*!< determines the layer draw order. */

BufferType Buffers[MAX_BUFFER];	/*!< my buffers. */
LibraryType Library;		/*!< the library. */
bool Bumped;			/*!< if the undo serial number has changed. */

int addedLines;
