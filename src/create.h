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
PinTypePtr		CreateNewVia(DataTypePtr, Location, Location, BDimension,
				BDimension, BDimension, BDimension, char *, int);
LineTypePtr		CreateDrawnLineOnLayer(LayerTypePtr, Location,
				Location, Location, Location, BDimension,
				BDimension, int);
LineTypePtr		CreateNewLineOnLayer(LayerTypePtr, Location,
				Location, Location, Location, BDimension,
				BDimension, int);
RatTypePtr		CreateNewRat(DataTypePtr, Location,
				Location, Location, Location, Cardinal,
				Cardinal, BDimension, int);
ArcTypePtr		CreateNewArcOnLayer(LayerTypePtr, Location,
				Location, BDimension, int,
				int, BDimension, BDimension, int);
PolygonTypePtr		CreateNewPolygonFromRectangle(LayerTypePtr,
				Location, Location, Location, Location, int);
TextTypePtr		CreateNewText(LayerTypePtr, FontTypePtr, Location,
				Location, BYTE, int, char *, int);
PolygonTypePtr		CreateNewPolygon(LayerTypePtr, int);
PointTypePtr		CreateNewPointInPolygon(PolygonTypePtr,
				Location, Location);
ElementTypePtr		CreateNewElement(DataTypePtr, ElementTypePtr,
				FontTypePtr, int, char *, char *, char *,
				Location, Location, BYTE, int, int, Boolean);
LineTypePtr		CreateNewLineInElement(ElementTypePtr, Location,
				Location, Location, Location, BDimension);
ArcTypePtr		CreateNewArcInElement(ElementTypePtr, Location,
				Location, BDimension, BDimension,
				int, int, BDimension);
PinTypePtr		CreateNewPin(ElementTypePtr, Location, Location,
				BDimension, BDimension, BDimension, BDimension,
				char *, char *, int);
PadTypePtr		CreateNewPad(ElementTypePtr, Location, Location,
				Location, Location, BDimension, BDimension,
				BDimension, char *, char *, int);
LineTypePtr		CreateNewLineInSymbol(SymbolTypePtr, Location,
				Location, Location, Location, BDimension);
void			CreateDefaultFont(void);
RubberbandTypePtr	CreateNewRubberbandEntry(LayerTypePtr,
				LineTypePtr, PointTypePtr);
LibraryMenuTypePtr	CreateNewNet(LibraryTypePtr, char *, char *);
LibraryEntryTypePtr	CreateNewConnection(LibraryMenuTypePtr, char *);

#endif
