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

/* prototypes for create routines
 */

#ifndef	__CREATE_INCLUDED__
#define	__CREATE_INCLUDED__

#include "global.h"

DataTypePtr		CreateNewBuffer(void);
PCBTypePtr		CreateNewPCB(Boolean);
PinTypePtr		CreateNewVia(DataTypePtr, LocationType, LocationType, BDimension,
				BDimension, BDimension, BDimension, char *, int);
LineTypePtr		CreateDrawnLineOnLayer(LayerTypePtr, LocationType,
				LocationType, LocationType, LocationType, BDimension,
				BDimension, int);
LineTypePtr		CreateNewLineOnLayer(LayerTypePtr, LocationType,
				LocationType, LocationType, LocationType, BDimension,
				BDimension, int);
RatTypePtr		CreateNewRat(DataTypePtr, LocationType,
				LocationType, LocationType, LocationType, Cardinal,
				Cardinal, BDimension, int);
ArcTypePtr		CreateNewArcOnLayer(LayerTypePtr, LocationType,
				LocationType, BDimension, int,
				int, BDimension, BDimension, int);
PolygonTypePtr		CreateNewPolygonFromRectangle(LayerTypePtr,
				LocationType, LocationType, LocationType, LocationType, int);
TextTypePtr		CreateNewText(LayerTypePtr, FontTypePtr, LocationType,
				LocationType, BYTE, int, char *, int);
PolygonTypePtr		CreateNewPolygon(LayerTypePtr, int);
PointTypePtr		CreateNewPointInPolygon(PolygonTypePtr,
				LocationType, LocationType);
ElementTypePtr		CreateNewElement(DataTypePtr, ElementTypePtr,
				FontTypePtr, int, char *, char *, char *,
				LocationType, LocationType, BYTE, int, int, Boolean);
LineTypePtr		CreateNewLineInElement(ElementTypePtr, LocationType,
				LocationType, LocationType, LocationType, BDimension);
ArcTypePtr		CreateNewArcInElement(ElementTypePtr, LocationType,
				LocationType, BDimension, BDimension,
				int, int, BDimension);
PinTypePtr		CreateNewPin(ElementTypePtr, LocationType, LocationType,
				BDimension, BDimension, BDimension, BDimension,
				char *, char *, int);
PadTypePtr		CreateNewPad(ElementTypePtr, LocationType, LocationType,
				LocationType, LocationType, BDimension, BDimension,
				BDimension, char *, char *, int);
LineTypePtr		CreateNewLineInSymbol(SymbolTypePtr, LocationType,
				LocationType, LocationType, LocationType, BDimension);
void			CreateDefaultFont(void);
RubberbandTypePtr	CreateNewRubberbandEntry(LayerTypePtr,
				LineTypePtr, PointTypePtr);
LibraryMenuTypePtr	CreateNewNet(LibraryTypePtr, char *, char *);
LibraryEntryTypePtr	CreateNewConnection(LibraryMenuTypePtr, char *);

#endif
