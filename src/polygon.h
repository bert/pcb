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

/* prototypes for polygon editing routines
 */

#ifndef	PCB_POLYGON_H
#define	PCH_POLYGON_H

#include "global.h"

/* Implementation constants */

#define POLY_CIRC_SEGS 40
#define POLY_CIRC_SEGS_F ((float)POLY_CIRC_SEGS)

/* adjustment to make the segments outline the circle rather than connect
 * points on the circle: 1 - cos (\alpha / 2) < (\alpha / 2) ^ 2 / 2
 */
#define POLY_CIRC_RADIUS_ADJ (1.0 + M_PI / POLY_CIRC_SEGS_F * \
                                    M_PI / POLY_CIRC_SEGS_F / 2.0)

/* polygon diverges from modelled arc no more than MAX_ARC_DEVIATION * thick */
#define POLY_ARC_MAX_DEVIATION 0.02

/* Prototypes */

void polygon_init (void);
Cardinal polygon_point_idx (PolygonTypePtr polygon, PointTypePtr point);
Cardinal polygon_point_contour (PolygonTypePtr polygon, Cardinal point);
Cardinal prev_contour_point (PolygonTypePtr polygon, Cardinal point);
Cardinal next_contour_point (PolygonTypePtr polygon, Cardinal point);
Cardinal GetLowestDistancePolygonPoint (PolygonTypePtr,
					Coord, Coord);
bool RemoveExcessPolygonPoints (LayerTypePtr, PolygonTypePtr);
void GoToPreviousPoint (void);
void ClosePolygon (void);
void CopyAttachedPolygonToLayer (void);
int PolygonHoles (PolygonType *ptr, const BoxType *range,
		  int (*callback) (PLINE *, void *user_data),
                  void *user_data);
int PlowsPolygon (DataType *, int, void *, void *,
		  int (*callback) (DataTypePtr, LayerTypePtr, PolygonTypePtr, int, void *, void *));
void ComputeNoHoles (PolygonType *poly);
POLYAREA * ContourToPoly (PLINE *);
POLYAREA * PolygonToPoly (PolygonType *);
POLYAREA * RectPoly (Coord x1, Coord x2, Coord y1, Coord y2);
POLYAREA * CirclePoly (Coord x, Coord y, Coord radius);
POLYAREA * OctagonPoly(Coord x, Coord y, Coord radius);
POLYAREA * LinePoly(LineType *l, Coord thick);
POLYAREA * ArcPoly(ArcType *l, Coord thick);
POLYAREA * PinPoly(PinType *l, Coord thick, Coord clear);
POLYAREA * BoxPolyBloated (BoxType *box, Coord radius);
void frac_circle (PLINE *, Coord, Coord, Vector, int);
int InitClip(DataType *d, LayerType *l, PolygonType *p);
void RestoreToPolygon(DataType *, int, void *, void *);
void ClearFromPolygon(DataType *, int, void *, void *);

bool IsPointInPolygon (Coord, Coord, Coord, PolygonTypePtr);
bool IsPointInPolygonIgnoreHoles (Coord, Coord, PolygonTypePtr);
bool IsRectangleInPolygon (Coord, Coord, Coord, Coord, PolygonTypePtr);
bool isects (POLYAREA *, PolygonTypePtr, bool);
bool MorphPolygon (LayerTypePtr, PolygonTypePtr);
void NoHolesPolygonDicer (PolygonType *p, const BoxType *clip,
                          void (*emit) (PLINE *, void *), void *user_data);
void PolyToPolygonsOnLayer (DataType *, LayerType *, POLYAREA *, FlagType);

#endif
