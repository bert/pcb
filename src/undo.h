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

/* prototypes for undo routines
 */

#ifndef	__UNDO_INCLUDED__
#define	__UNDO_INCLUDED__

#include "global.h"

#define DRAW_FLAGS	(RATFLAG | SELECTEDFLAG | SQUAREFLAG | ALLTHERMFLAGS \
			| HIDENAMEFLAG | HOLEFLAG | OCTAGONFLAG | FOUNDFLAG | CLEARLINEFLAG)

											/* different layers */

int	Undo(Boolean);
int	Redo(Boolean);
void	IncrementUndoSerialNumber(void);
void	SaveUndoSerialNumber(void);
void	RestoreUndoSerialNumber(void);
void	ClearUndoList(Boolean);
void	MoveObjectToRemoveUndoList(int, void *, void *, void *);
void	AddObjectToRemovePointUndoList(int, void *, void *, Cardinal);
void	AddObjectToInsertPointUndoList(int, void *, void *, void *);
void	AddObjectToMoveUndoList(int, void *, void *, void *,
			LocationType, LocationType);
void	AddObjectToChangeNameUndoList(int, void *,void *, void *, char *);
void	AddObjectToRotateUndoList(int, void *, void *,void *,
			LocationType, LocationType, BYTE);
void	AddObjectToCreateUndoList(int, void *, void *,void *);
void	AddObjectToMirrorUndoList(int, void *, void *,void *, LocationType);
void	AddObjectToMoveToLayerUndoList(int, void *, void *, void *);
void	AddObjectToFlagUndoList(int, void *, void *, void *);
void	AddObjectToSizeUndoList(int, void *, void *, void *);
void	AddObjectTo2ndSizeUndoList(int, void *, void *, void *);
void	AddObjectToClearSizeUndoList(int, void *, void *, void *);
void	AddObjectToMaskSizeUndoList(int, void *, void *, void *);
void	LockUndo(void);
void	UnlockUndo(void);
Boolean	Undoing(void);
 
#endif
