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

/* prototypes connection search routines
 */

#ifndef	__FIND_INCLUDED__
#define	__FIND_INCLUDED__

#include <stdio.h>		/* needed to define 'FILE *' */
#include "global.h"

/* ---------------------------------------------------------------------------
 * some local defines
 */
#define LOOKUP_FIRST	\
	(PIN_TYPE | PAD_TYPE)
#define LOOKUP_MORE	\
	(VIA_TYPE | LINE_TYPE | RATLINE_TYPE | POLYGON_TYPE | ARC_TYPE)
#define SILK_TYPE	\
	(LINE_TYPE | ARC_TYPE | POLYGON_TYPE)

Boolean	LineLineIntersect(LineTypePtr, LineTypePtr);
Boolean LineArcIntersect(LineTypePtr, ArcTypePtr);
Boolean	IsPolygonInPolygon(PolygonTypePtr, PolygonTypePtr);
void	LookupElementConnections(ElementTypePtr, FILE *);
void	LookupConnectionsToAllElements(FILE *);
void	LookupConnection(Location, Location, Boolean, BDimension);
void	LookupUnusedPins(FILE *);
void	ResetFoundLinesAndPolygons(Boolean);
void	ResetFoundPinsViasAndPads(Boolean);
void	InitConnectionLookup(void);
void	InitComponentLookup(void);
void	InitLayoutLookup(void);
void	FreeConnectionLookupMemory(void);
void	FreeComponentLookupMemory(void);
void	FreeLayoutLookupMemory(void);
void	RatFindHook(int, void *, void *, void *, Boolean, Boolean);
void	SaveFindFlag(int);
void	RestoreFindFlag(void);
Cardinal DRCAll(void);
Boolean lineClear(LineTypePtr, Cardinal);
Boolean viaClear(PinTypePtr);
Boolean IsLineInPolygon(LineTypePtr, PolygonTypePtr);
Boolean IsArcInPolygon(ArcTypePtr, PolygonTypePtr);

#endif
