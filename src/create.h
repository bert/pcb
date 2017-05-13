/*!
 * \file src/create.h
 *
 * \brief Prototypes for create routines.
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

#ifndef	PCB_CREATE_H
#define	PCB_CREATE_H

#include "global.h"

void CreateBeLenient (bool);

DataType * CreateNewBuffer (void);
void pcb_colors_from_settings (PCBType *);
PCBType * CreateNewPCB ();
int CreateNewPCBPost (PCBType *, int /* set defaults */);
PinType * CreateNewVia (DataType *, Coord, Coord, Coord, Coord, Coord, Coord, char *, FlagType);
PinType * CreateNewViaEx (DataType *, Coord, Coord, Coord, Coord, Coord, Coord, char *, FlagType, Cardinal buried_from, Cardinal buried_to);
LineType * CreateDrawnLineOnLayer (LayerType *, Coord, Coord, Coord, Coord, Coord, Coord, FlagType);
LineType * CreateNewLineOnLayer (LayerType *, Coord, Coord, Coord, Coord, Coord, Coord, FlagType);
RatType * CreateNewRat (DataType *, Coord, Coord, Coord, Coord, Cardinal, Cardinal, Coord, FlagType);
ArcType * CreateNewArcOnLayer (LayerType *, Coord, Coord, Coord, Coord, Angle, Angle, Coord, Coord, FlagType);
PolygonType * CreateNewPolygonFromRectangle (LayerType *, Coord, Coord, Coord, Coord, FlagType);
TextType * CreateNewText (LayerType *, FontType *, Coord, Coord, unsigned, int, char *, FlagType);
PolygonType * CreateNewPolygon (LayerType *, FlagType);
PointType * CreateNewPointInPolygon (PolygonType *, Coord, Coord);
PolygonType * CreateNewHoleInPolygon (PolygonType *polygon);
ElementType * CreateNewElement (DataType *, FontType *, FlagType, char *, char *, char *, Coord, Coord, BYTE, int, FlagType, bool);
LineType * CreateNewLineInElement (ElementType *, Coord, Coord, Coord, Coord, Coord);
ArcType * CreateNewArcInElement (ElementType *, Coord, Coord, Coord, Coord, Angle, Angle, Coord);
PinType * CreateNewPin (ElementType *, Coord, Coord, Coord, Coord, Coord, Coord, char *, char *, FlagType);
PadType * CreateNewPad (ElementType *, Coord, Coord, Coord, Coord, Coord, Coord, Coord, char *, char *, FlagType);
LineType * CreateNewLineInSymbol (SymbolType *, Coord, Coord, Coord, Coord, Coord);
void CreateDefaultFont (PCBType *);
RubberbandType * CreateNewRubberbandEntry (LayerType *, LineType *, PointType *);
LibraryMenuType * CreateNewNet (LibraryType *, char *, char *);
LibraryEntryType * CreateNewConnection (LibraryMenuType *, char *);
AttributeType * CreateNewAttribute (AttributeListType *list, char *name, char *value);

#endif
