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

/* prototypes to change object properties
 */

#ifndef	PCB_CHANGE_H
#define	PCB_CHANGE_H

#include "global.h"

/* ---------------------------------------------------------------------------
 * some defines
 */
#define	CHANGENAME_TYPES        \
	(VIA_TYPE | PIN_TYPE | PAD_TYPE | TEXT_TYPE | ELEMENT_TYPE | ELEMENTNAME_TYPE | LINE_TYPE)

#define	CHANGESIZE_TYPES        \
	(POLYGON_TYPE | VIA_TYPE | PIN_TYPE | PAD_TYPE | LINE_TYPE | \
	 ARC_TYPE | TEXT_TYPE | ELEMENTNAME_TYPE | ELEMENT_TYPE)

#define	CHANGE2NDSIZE_TYPES     \
	(VIA_TYPE | PIN_TYPE | ELEMENT_TYPE)

/* We include polygons here only to inform the user not to do it that way.  */
#define CHANGECLEARSIZE_TYPES	\
	(PIN_TYPE | PAD_TYPE | VIA_TYPE | LINE_TYPE | ARC_TYPE | POLYGON_TYPE)

#define	CHANGESQUARE_TYPES     \
	(ELEMENT_TYPE | PIN_TYPE | PAD_TYPE)

#define	CHANGEOCTAGON_TYPES     \
	(ELEMENT_TYPE | PIN_TYPE | VIA_TYPE)

#define CHANGEJOIN_TYPES	\
	(ARC_TYPE | LINE_TYPE | TEXT_TYPE)

#define CHANGETHERMAL_TYPES	\
	(PIN_TYPE | VIA_TYPE)

#define CHANGEMASKSIZE_TYPES    \
        (PIN_TYPE | VIA_TYPE | PAD_TYPE)

bool ChangeLayoutName (char *);
bool ChangeLayerName (LayerTypePtr, char *);
bool ChangeSelectedSize (int, Coord, bool);
bool ChangeSelectedClearSize (int, Coord, bool);
bool ChangeSelected2ndSize (int, Coord, bool);
bool ChangeSelectedMaskSize (int, Coord, bool);
bool ChangeSelectedJoin (int);
bool SetSelectedJoin (int);
bool ClrSelectedJoin (int);
bool ChangeSelectedSquare (int);
bool SetSelectedSquare (int);
bool ClrSelectedSquare (int);
bool ChangeSelectedThermals (int, int);
bool ChangeSelectedHole (void);
bool ChangeSelectedPaste (void);
bool ChangeSelectedOctagon (int);
bool SetSelectedOctagon (int);
bool ClrSelectedOctagon (int);
bool ChangeSelectedElementSide (void);
bool ChangeElementSide (ElementTypePtr, Coord);
bool ChangeHole (PinTypePtr);
bool ChangePaste (PadTypePtr);
bool ChangeObjectSize (int, void *, void *, void *, Coord, bool);
bool ChangeObjectThermal (int, void *, void *, void *, int);
bool ChangeObjectClearSize (int, void *, void *, void *, Coord,
			       bool);
bool ChangeObject2ndSize (int, void *, void *, void *, Coord,
			     bool, bool);
bool ChangeObjectMaskSize (int, void *, void *, void *, Coord,
			      bool);
bool ChangeObjectJoin (int, void *, void *, void *);
bool SetObjectJoin (int, void *, void *, void *);
bool ClrObjectJoin (int, void *, void *, void *);
bool ChangeObjectSquare (int, void *, void *, void *);
bool SetObjectSquare (int, void *, void *, void *);
bool ClrObjectSquare (int, void *, void *, void *);
bool ChangeObjectOctagon (int, void *, void *, void *);
bool SetObjectOctagon (int, void *, void *, void *);
bool ClrObjectOctagon (int, void *, void *, void *);
void *ChangeObjectName (int, void *, void *, void *, char *);
void *QueryInputAndChangeObjectName (int, void *, void *, void *);
void ChangePCBSize (Coord, Coord);

/* Change the specified text on an element, either on the board (give
   PCB, PCB->Data) or in a buffer (give NULL, Buffer->Data).  The old
   string is returned, and must be properly freed by the caller.  */
char *ChangeElementText (PCBType *pcb, DataType *data, ElementTypePtr Element,
			 int which, char *new_name);

#endif
