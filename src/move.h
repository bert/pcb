/*!
 * \file src/move.h
 *
 * \brief Prototypes for move routines.
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

#ifndef	PCB_MOVE_H
#define	PCB_MOVE_H

#include "global.h"

/* ---------------------------------------------------------------------------
 * some useful transformation macros and constants
 */
#define	MOVE(xs,ys,deltax,deltay)							\
	{														\
		((xs) += (deltax));									\
		((ys) += (deltay));									\
	}
#define	MOVE_BOX_LOWLEVEL(b,dx,dy)		\
	{									\
		MOVE((b)->X1,(b)->Y1,(dx),(dy))	\
		MOVE((b)->X2,(b)->Y2,(dx),(dy))	\
	}
#define	MOVE_VIA_LOWLEVEL(v,dx,dy) \
        { \
	        MOVE((v)->X,(v)->Y,(dx),(dy)) \
		MOVE_BOX_LOWLEVEL(&((v)->BoundingBox),(dx),(dy));		\
	}
#define	MOVE_PIN_LOWLEVEL(p,dx,dy) \
	{ \
		MOVE((p)->X,(p)->Y,(dx),(dy)) \
		MOVE_BOX_LOWLEVEL(&((p)->BoundingBox),(dx),(dy));		\
	}

#define	MOVE_ARC_LOWLEVEL(a,dx,dy) \
	{ \
		MOVE((a)->X,(a)->Y,(dx),(dy)) \
		MOVE_BOX_LOWLEVEL(&((a)->BoundingBox),(dx),(dy));		\
		MOVE((a)->Point1.X,(a)->Point1.Y,(dx),(dy))			\
		MOVE((a)->Point2.X,(a)->Point2.Y,(dx),(dy))			\
	}
/* Rather than mode the line bounding box, we set it so the point bounding
 * boxes are updated too.
 */
#define	MOVE_LINE_LOWLEVEL(l,dx,dy)							\
	{									\
		MOVE((l)->Point1.X,(l)->Point1.Y,(dx),(dy))			\
		MOVE((l)->Point2.X,(l)->Point2.Y,(dx),(dy))			\
		SetLineBoundingBox ((l)); \
	}
#define	MOVE_PAD_LOWLEVEL(p,dx,dy)	\
	{									\
		MOVE((p)->Point1.X,(p)->Point1.Y,(dx),(dy))			\
		MOVE((p)->Point2.X,(p)->Point2.Y,(dx),(dy))			\
		SetPadBoundingBox ((p)); \
	}
#define	MOVE_TEXT_LOWLEVEL(t,dx,dy)								\
	{															\
		MOVE_BOX_LOWLEVEL(&((t)->BoundingBox),(dx),(dy));		\
		MOVE((t)->X, (t)->Y, (dx), (dy));						\
	}

#define	MOVE_TYPES	\
	(VIA_TYPE | LINE_TYPE | TEXT_TYPE | ELEMENT_TYPE | ELEMENTNAME_TYPE |	\
	POLYGON_TYPE | POLYGONPOINT_TYPE | LINEPOINT_TYPE | ARC_TYPE)
#define	MOVETOLAYER_TYPES	\
	(LINE_TYPE | TEXT_TYPE | POLYGON_TYPE | RATLINE_TYPE | ARC_TYPE)


/* ---------------------------------------------------------------------------
 * prototypes
 */
void MovePolygonLowLevel (PolygonType *, Coord, Coord);
void MoveElementLowLevel (DataType *, ElementType *, Coord, Coord);
void *MoveObject (int, void *, void *, void *, Coord, Coord);
void *MoveObjectToLayer (int, void *, void *, void *, LayerType *, bool);
void *MoveObjectAndRubberband (int, void *, void *, void *,
			       Coord, Coord);
bool MoveSelectedObjectsToLayer (LayerType *);

int MoveLayer(int old_index, int new_index);

#endif
