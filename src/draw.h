/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2004 Thomas Nau
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

/* prototypes for drawing routines
 */

#ifndef	PCB_DRAW_H
#define	PCB_DRAW_H

#include "global.h"

/*bool	SwitchDrawingWindow(double, GdkDrawable *, gboolean, gboolean);*/

void Draw (void);
void Redraw (void);
void DrawVia (PinTypePtr);
void DrawRat (RatTypePtr);
void DrawViaName (PinTypePtr);
void DrawPin (PinTypePtr);
void DrawPinName (PinTypePtr);
void DrawPad (PadTypePtr);
void DrawPadName (PadTypePtr);
void DrawLine (LayerTypePtr, LineTypePtr);
void DrawArc (LayerTypePtr, ArcTypePtr);
void DrawText (LayerTypePtr, TextTypePtr);
void DrawTextLowLevel (TextTypePtr, Coord);
void DrawPolygon (LayerTypePtr, PolygonTypePtr);
void DrawElement (ElementTypePtr);
void DrawElementName (ElementTypePtr);
void DrawElementPackage (ElementTypePtr);
void DrawElementPinsAndPads (ElementTypePtr);
void DrawObject (int, void *, void *);
void DrawLayer (LayerTypePtr, const BoxType *);
void EraseVia (PinTypePtr);
void EraseRat (RatTypePtr);
void EraseViaName (PinTypePtr);
void ErasePad (PadTypePtr);
void ErasePadName (PadTypePtr);
void ErasePin (PinTypePtr);
void ErasePinName (PinTypePtr);
void EraseLine (LineTypePtr);
void EraseArc (ArcTypePtr);
void EraseText (LayerTypePtr, TextTypePtr);
void ErasePolygon (PolygonTypePtr);
void EraseElement (ElementTypePtr);
void EraseElementPinsAndPads (ElementTypePtr);
void EraseElementName (ElementTypePtr);
void EraseObject (int, void *, void *);

void DrawLayerGroup (int side, const BoxType *drawn_area);
void DrawPaste (int side, const BoxType *drawn_area);
void DrawSilk (int side, const BoxType *drawn_area);
void DrawMask (int side, const BoxType *drawn_area);
void DrawHoles (bool draw_plated, bool draw_unplated, const BoxType *drawn_area);
void PrintAssembly (int side, const BoxType *drawn_area);

#endif
