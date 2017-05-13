/*!
 * \file src/buffer.h
 *
 * \brief Prototypes for buffer handling routines.
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

#ifndef	PCB_BUFFER_H
#define	PCB_BUFFER_H

#include "global.h"

/* ---------------------------------------------------------------------------
 * prototypes
 */
void SetBufferBoundingBox (BufferType *);
void ClearBuffer (BufferType *);
void AddSelectedToBuffer (BufferType *, Coord, Coord, bool);
bool LoadElementToBuffer (BufferType *, char *, bool);
bool ConvertBufferToElement (BufferType *);
bool SmashBufferElement (BufferType *);
bool LoadLayoutToBuffer (BufferType *, char *);
void RotateBuffer (BufferType *, BYTE);
void SelectPasteBuffer (int);
void SwapBuffers (void);
void MirrorBuffer (BufferType *);
void InitBuffers (void);
void UninitBuffers (void);
void *MoveObjectToBuffer (DataType *, DataType *, int, void *, void *, void *); 
void *CopyObjectToBuffer (DataType *, DataType *, int,
			  void *, void *, void *);

int LoadFootprint (int argc, char **argv, Coord x, Coord y);

#endif
