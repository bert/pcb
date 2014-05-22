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

/* prototypes for rubberband routines
 */

#ifndef	PCB_RUBBERBAND_H
#define	PCB_RUBBERBAND_H

#include "global.h"

void LookupRubberbandLines (int, void *, void *, void *);
void LookupRatLines (int, void *, void *, void *);
void MovePointGivenRubberBandMode (PointType *PointOut, 
                                   PointType *Point, 
                                   LineType *Line, 
                                   Coord dx, 
                                   Coord dy,
                                   int Type,
                                   int Diagonal);
void MoveLineGivenRubberBandMode (LineType *LineOut, 
                                  LineType *Line, 
                                  Coord dx, 
                                  Coord dy,
                                  CrosshairType CrossHair);
int IsHorizontal (LineType *Line);
int IsVertical (LineType *Line);
int IsDiagonal (LineType *Line);
void RestrictMovementGivenRubberBandMode (LineType *Line,
                                          Coord *dx, 
                                          Coord *dy);
LineType *FindLineAttachedToPoint (LayerType *Layer,
                                   LineType  *Line,
                                   PointType *LinePoint);
int PointInsidePin (PinType *Pin, Coord x, Coord y);
int BothEndsWithinPad (PadType *Pad, LineType *Line);

#endif
