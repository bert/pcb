/*!
 * \file src/undo.h
 *
 * \brief Prototypes for undo routines.
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
 *
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *
 * Thomas.Nau@rz.uni-ulm.de
 */

#ifndef	PCB_UNDO_H
#define	PCB_UNDO_H

#include "global.h"

#define DRAW_FLAGS  (RATFLAG | SELECTEDFLAG | SQUAREFLAG |    \
                     HIDENAMEFLAG | HOLEFLAG | OCTAGONFLAG |  \
                     CONNECTEDFLAG | FOUNDFLAG | CLEARLINEFLAG)

											/* different layers */

int Undo (bool);
int Redo (bool);
void IncrementUndoSerialNumber (void);
void SaveUndoSerialNumber (void);
void RestoreUndoSerialNumber (void);
void ClearUndoList (bool);
void MoveObjectToRemoveUndoList (int, void *, void *, void *);
void AddObjectToRemovePointUndoList (int, void *, void *, Cardinal);
void AddObjectToInsertPointUndoList (int, void *, void *, void *);
void AddObjectToRemoveContourUndoList (int, LayerType *, PolygonType *);
void AddObjectToInsertContourUndoList (int, LayerType *, PolygonType *);
void AddObjectToMoveUndoList (int, void *, void *, void *,
			      Coord, Coord);
void AddObjectToChangeNameUndoList (int, void *, void *, void *, char *, int);
void AddObjectToRotateUndoList (int, void *, void *, void *,
				Coord, Coord, BYTE);
void AddObjectToCreateUndoList (int, void *, void *, void *);
void AddObjectToMirrorUndoList (int, void *, void *, void *, Coord);
void AddObjectToMoveToLayerUndoList (int, void *, void *, void *);
void AddObjectToFlagUndoList (int, void *, void *, void *);
void AddObjectToSizeUndoList (int, void *, void *, void *);
void AddObjectTo2ndSizeUndoList (int, void *, void *, void *);
void AddObjectToClearSizeUndoList (int, void *, void *, void *);
void AddObjectToMaskSizeUndoList (int, void *, void *, void *);
void AddObjectToChangeAnglesUndoList (int, void *, void *, void *);
void AddObjectToClearPolyUndoList (int, void *, void *, void *, bool);
void AddObjectToSetViaLayersUndoList (void *ptr1, void *ptr2, void *ptr3);
void AddLayerChangeToUndoList (int, int);
void AddNetlistLibToUndoList (LibraryType *);
void LockUndo (void);
void UnlockUndo (void);
bool Undoing (void);

#endif
