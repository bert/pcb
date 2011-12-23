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

/* prototypes for buffer handling routines
 */

#ifndef	PCB_BUFFER_H
#define	PCB_BUFFER_H

#include "global.h"

/* ---------------------------------------------------------------------------
 * prototypes
 */
void SetBufferBoundingBox (BufferTypePtr);
void ClearBuffer (BufferTypePtr);
void AddSelectedToBuffer (BufferTypePtr, Coord, Coord, bool);
bool LoadElementToBuffer (BufferTypePtr, char *, bool);
bool ConvertBufferToElement (BufferTypePtr);
bool SmashBufferElement (BufferTypePtr);
bool LoadLayoutToBuffer (BufferTypePtr, char *);
void RotateBuffer (BufferTypePtr, BYTE);
void SelectPasteBuffer (int);
void SwapBuffers (void);
void MirrorBuffer (BufferTypePtr);
void InitBuffers (void);
void *MoveObjectToBuffer (DataTypePtr, DataTypePtr, int, void *, void *, void *); 
void *CopyObjectToBuffer (DataTypePtr, DataTypePtr, int,
			  void *, void *, void *);

/* This action is called from ActionElementAddIf() */
int LoadFootprint (int argc, char **argv, Coord x, Coord y);

#endif
