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

int lines_intersect(Coord ax1, Coord ay1, Coord ax2, Coord ay2, Coord bx1, Coord by1, Coord bx2, Coord by2);

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

/* == the same but accept if any part of the object touches the box == */
#define POINT_IN_CIRCLE(x, y, cx, cy, r) \
	(((x)-(cx)) * ((x)-(cx)) + ((y)-(cy)) * ((y)-(cy)) <= (r)*(r))

#define CIRCLE_TOUCH_BOX(cx, cy, r, b) \
	(    POINT_IN_BOX((cx)-(r),(cy),(b)) || POINT_IN_BOX((cx)+(r),(cy),(b)) || POINT_IN_BOX((cx),(cy)-(r),(b)) || POINT_IN_BOX((cx),(cy)+(r),(b)) \
	  || POINT_IN_CIRCLE((b)->X1, (b)->Y1, (cx), (cy), (r)) || POINT_IN_CIRCLE((b)->X2, (b)->Y1, (cx), (cy), (r)) || POINT_IN_CIRCLE((b)->X1, (b)->Y2, (cx), (cy), (r)) || POINT_IN_CIRCLE((b)->X2, (b)->Y2, (cx), (cy), (r)))

#define	VIA_OR_PIN_TOUCHES_BOX(v,b) \
	CIRCLE_TOUCH_BOX((v)->X,(v)->Y,((v)->Thickness + (v)->DrillingHole/2),(b))

#define	LINE_TOUCHES_BOX(l,b)	\
	(    lines_intersect((l)->Point1.X,(l)->Point1.Y,(l)->Point2.X,(l)->Point2.Y, (b)->X1, (b)->Y1, (b)->X2, (b)->Y1) \
	  || lines_intersect((l)->Point1.X,(l)->Point1.Y,(l)->Point2.X,(l)->Point2.Y, (b)->X1, (b)->Y1, (b)->X1, (b)->Y2) \
	  || lines_intersect((l)->Point1.X,(l)->Point1.Y,(l)->Point2.X,(l)->Point2.Y, (b)->X2, (b)->Y2, (b)->X1, (b)->Y2) \
	  || lines_intersect((l)->Point1.X,(l)->Point1.Y,(l)->Point2.X,(l)->Point2.Y, (b)->X2, (b)->Y2, (b)->X2, (b)->Y1))

#define	PAD_TOUCHES_BOX(p,b)	LINE_TOUCHES_BOX((LineType *)(p),(b))

/* a corner of either box is within the other, or edges cross */
#define	BOX_TOUCHES_BOX(b1,b2)	\
	(   POINT_IN_BOX((b1)->X1,(b1)->Y1,b2) || POINT_IN_BOX((b1)->X1,(b1)->Y2,b2) || POINT_IN_BOX((b1)->X2,(b1)->Y1,b2) || POINT_IN_BOX((b1)->X2,(b1)->Y2,b2)  \
	 || POINT_IN_BOX((b2)->X1,(b2)->Y1,b1) || POINT_IN_BOX((b2)->X1,(b2)->Y2,b1) || POINT_IN_BOX((b2)->X2,(b2)->Y1,b1) || POINT_IN_BOX((b2)->X2,(b2)->Y2,b1)  \
	 || lines_intersect((b1)->X1,(b1)->Y1, (b1)->X2,(b1)->Y1, (b2)->X1,(b2)->Y1, (b2)->X1,(b2)->Y2) \
	 || lines_intersect((b2)->X1,(b2)->Y1, (b2)->X2,(b2)->Y1, (b1)->X1,(b1)->Y1, (b1)->X1,(b1)->Y2))

#define	TEXT_TOUCHES_BOX(t,b)	\
	(BOX_TOUCHES_BOX(&((t)->BoundingBox), (b)))

#define	POLYGON_TOUCHES_BOX(p,b)	\
	(BOX_TOUCHES_BOX(&((p)->BoundingBox), (b)))

#define	ELEMENT_TOUCHES_BOX(e,b)	\
	(BOX_TOUCHES_BOX(&((e)->BoundingBox), (b)))

#define ARC_TOUCHES_BOX(a,b)		\
	(BOX_TOUCHES_BOX(&((a)->BoundingBox), (b)))


/* == the combination of *_IN_* and *_TOUCHES_*: use IN for positive boxes == */
#define IS_BOX_NEGATIVE(b) (((b)->X2 < (b)->X1) || ((b)->Y2 < (b)->Y1))

#define	BOX_NEAR_BOX(b1,b) \
	(IS_BOX_NEGATIVE(b) ? BOX_TOUCHES_BOX(b1,b) : BOX_IN_BOX(b1,b))

#define	VIA_OR_PIN_NEAR_BOX(v,b) \
	(IS_BOX_NEGATIVE(b) ? VIA_OR_PIN_TOUCHES_BOX(v,b) : VIA_OR_PIN_IN_BOX(v,b))

#define	LINE_NEAR_BOX(l,b) \
	(IS_BOX_NEGATIVE(b) ? LINE_TOUCHES_BOX(l,b) : LINE_IN_BOX(l,b))

#define	PAD_NEAR_BOX(p,b) \
	(IS_BOX_NEGATIVE(b) ? PAD_TOUCHES_BOX(p,b) : PAD_IN_BOX(p,b))

#define	TEXT_NEAR_BOX(t,b) \
	(IS_BOX_NEGATIVE(b) ? TEXT_TOUCHES_BOX(t,b) : TEXT_IN_BOX(t,b))

#define	POLYGON_NEAR_BOX(p,b) \
	(IS_BOX_NEGATIVE(b) ? POLYGON_TOUCHES_BOX(p,b) : POLYGON_IN_BOX(p,b))

#define	ELEMENT_NEAR_BOX(e,b) \
	(IS_BOX_NEGATIVE(b) ? ELEMENT_TOUCHES_BOX(e,b) : ELEMENT_IN_BOX(e,b))

#define ARC_NEAR_BOX(a,b) \
	(IS_BOX_NEGATIVE(b) ? ARC_TOUCHES_BOX(a,b) : ARC_IN_BOX(a,b))


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
