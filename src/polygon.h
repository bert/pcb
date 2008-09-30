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

/* prototypes for polygon editing routines
 */

#ifndef	__POLYGON_INCLUDED__
#define	__POLYGON_INCLUDED__

#include "global.h"

Cardinal GetLowestDistancePolygonPoint (PolygonTypePtr,
					LocationType, LocationType);
Boolean RemoveExcessPolygonPoints (LayerTypePtr, PolygonTypePtr);
void GoToPreviousPoint (void);
void ClosePolygon (void);
void CopyAttachedPolygonToLayer (void);
int PolygonHoles (const BoxType * range, LayerTypePtr, PolygonTypePtr,
		  int (*callback) (PLINE *, LayerTypePtr, PolygonTypePtr));
int PlowsPolygon (DataType *, int, void *, void *,
		  int (*callback) (DataTypePtr, LayerTypePtr, PolygonTypePtr, int, void *, void *));
POLYAREA * ContourToPoly (PLINE *);
POLYAREA * RectPoly (LocationType x1, LocationType x2, LocationType y1, LocationType y2);
POLYAREA * CirclePoly(LocationType x, LocationType y, BDimension radius);
POLYAREA * OctagonPoly(LocationType x, LocationType y, BDimension radius);
POLYAREA * LinePoly(LineType *l, BDimension thick);
POLYAREA * ArcPoly(ArcType *l, BDimension thick);
POLYAREA * PinPoly(PinType *l, BDimension thick, BDimension clear);
POLYAREA * BoxPolyBloated (BoxType *box, BDimension radius);
void frac_circle (PLINE *, LocationType, LocationType, Vector, int);
int InitClip(DataType *d, LayerType *l, PolygonType *p);
void RestoreToPolygon(DataType *, int, void *, void *);
void ClearFromPolygon(DataType *, int, void *, void *);

Boolean IsPointInPolygon (LocationType, LocationType, BDimension, PolygonTypePtr);
Boolean IsRectangleInPolygon (LocationType, LocationType, LocationType,
			      LocationType, PolygonTypePtr);
Boolean isects (POLYAREA *, PolygonTypePtr, Boolean);
Boolean MorphPolygon (LayerTypePtr, PolygonTypePtr);
void NoHolesPolygonDicer (PolygonTypePtr p, void (*emit) (PolygonTypePtr), const BoxType *clip);
#endif
