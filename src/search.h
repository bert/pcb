/*!
 * \file src/search.h
 *
 * \brief Prototypes for search routines.
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

#ifndef	PCB_SEARCH_H
#define	PCB_SEARCH_H

#include "global.h"

#define SLOP 5
/* ---------------------------------------------------------------------------
 * some useful macros
 */
/* ---------------------------------------------------------------------------
 * some define to check for 'type' in box
 */
#define	POINT_IN_BOX(x,y,b)	\
	((x) >= (b)->X1 && (x) <= (b)->X2 && (y) >= (b)->Y1 && (y) <= (b)->Y2)

#define	VIA_OR_PIN_IN_BOX(v,b) \
	POINT_IN_BOX((v)->X,(v)->Y,(b))

#define	LINE_IN_BOX(l,b)	\
	(POINT_IN_BOX((l)->Point1.X,(l)->Point1.Y,(b)) &&	\
	POINT_IN_BOX((l)->Point2.X,(l)->Point2.Y,(b)))

#define	PAD_IN_BOX(p,b)	LINE_IN_BOX((LineType *)(p),(b))

#define	BOX_IN_BOX(b1,b)	\
	((b1)->X1 >= (b)->X1 && (b1)->X2 <= (b)->X2 &&	\
	((b1)->Y1 >= (b)->Y1 && (b1)->Y2 <= (b)->Y2))

#define	TEXT_IN_BOX(t,b)	\
	(BOX_IN_BOX(&((t)->BoundingBox), (b)))

#define	POLYGON_IN_BOX(p,b)	\
	(BOX_IN_BOX(&((p)->BoundingBox), (b)))

#define	ELEMENT_IN_BOX(e,b)	\
	(BOX_IN_BOX(&((e)->BoundingBox), (b)))

#define ARC_IN_BOX(a,b)		\
	(BOX_IN_BOX(&((a)->BoundingBox), (b)))

/* ---------------------------------------------------------------------------
 * prototypes
 */
bool IsPointOnLine (Coord, Coord, Coord, LineType *);
bool IsPointOnPin (Coord, Coord, Coord, PinType *);
bool IsPointOnArc (Coord, Coord, Coord, ArcType *);
bool IsPointOnLineEnd (Coord, Coord, RatType *);
bool IsLineInRectangle (Coord, Coord, Coord, Coord, LineType *);
bool IsLineInQuadrangle (PointType p[4], LineType * Line);
bool IsArcInRectangle (Coord, Coord, Coord, Coord, ArcType *);
bool IsPointInPad (Coord, Coord, Coord, PadType *);
bool IsPointInBox (Coord, Coord, BoxType *, Coord);
int SearchObjectByLocation (unsigned, void **, void **, void **, Coord, Coord, Coord);
int SearchScreen (Coord, Coord, int, void **, void **, void **);
int SearchObjectByID (DataType *, void **, void **, void **, int, int);
ElementType * SearchElementByName (DataType *, char *);
int SearchLayerByName (DataType *Base, char *Name);
#endif
