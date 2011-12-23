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

/* prototypes for create routines
 */

#ifndef	PCB_CREATE_H
#define	PCB_CREATE_H

#include "global.h"

/* TRUE during file loads, for example to allow overlapping vias.
   FALSE otherwise, to stop the user from doing normally dangerous
   things.  */
void CreateBeLenient (bool);

DataTypePtr CreateNewBuffer (void);
void pcb_colors_from_settings (PCBTypePtr);
PCBTypePtr CreateNewPCB (bool);
/* Called after PCB->Data->LayerN is set.  Returns zero if no errors,
   else nonzero.  */
int CreateNewPCBPost (PCBTypePtr, int /* set defaults */);
PinTypePtr CreateNewVia (DataTypePtr, Coord, Coord, Coord, Coord,
			 Coord, Coord, char *, FlagType);
LineTypePtr CreateDrawnLineOnLayer (LayerTypePtr, Coord, Coord, Coord,
				    Coord, Coord, Coord, FlagType);
LineTypePtr CreateNewLineOnLayer (LayerTypePtr, Coord, Coord, Coord,
				  Coord, Coord, Coord, FlagType);
RatTypePtr CreateNewRat (DataTypePtr, Coord, Coord, Coord,
			 Coord, Cardinal, Cardinal, Coord, FlagType);
ArcTypePtr CreateNewArcOnLayer (LayerTypePtr, Coord, Coord, Coord, Coord,
				Angle, Angle, Coord, Coord, FlagType);
PolygonTypePtr CreateNewPolygonFromRectangle (LayerTypePtr, Coord,
					      Coord, Coord,
					      Coord, FlagType);
TextTypePtr CreateNewText (LayerTypePtr, FontTypePtr, Coord,
			   Coord, unsigned, int, char *, FlagType);
PolygonTypePtr CreateNewPolygon (LayerTypePtr, FlagType);
PointTypePtr CreateNewPointInPolygon (PolygonTypePtr,
				      Coord, Coord);
PolygonType *CreateNewHoleInPolygon (PolygonType *polygon);
ElementTypePtr CreateNewElement (DataTypePtr, ElementTypePtr,
				 FontTypePtr, FlagType, char *, char *,
				 char *, Coord, Coord, BYTE,
				 int, FlagType, bool);
LineTypePtr CreateNewLineInElement (ElementTypePtr, Coord, Coord,
				    Coord, Coord, Coord);
ArcTypePtr CreateNewArcInElement (ElementTypePtr, Coord, Coord,
				  Coord, Coord, Angle, Angle, Coord);
PinTypePtr CreateNewPin (ElementTypePtr, Coord, Coord,
			 Coord, Coord, Coord, Coord,
			 char *, char *, FlagType);
PadTypePtr CreateNewPad (ElementTypePtr, Coord, Coord,
			 Coord, Coord, Coord, Coord,
			 Coord, char *, char *, FlagType);
LineTypePtr CreateNewLineInSymbol (SymbolTypePtr, Coord, Coord,
				   Coord, Coord, Coord);
void CreateDefaultFont (PCBTypePtr);
RubberbandTypePtr CreateNewRubberbandEntry (LayerTypePtr,
					    LineTypePtr, PointTypePtr);
LibraryMenuTypePtr CreateNewNet (LibraryTypePtr, char *, char *);
LibraryEntryTypePtr CreateNewConnection (LibraryMenuTypePtr, char *);

AttributeTypePtr CreateNewAttribute (AttributeListTypePtr list, char *name, char *value);

#endif
