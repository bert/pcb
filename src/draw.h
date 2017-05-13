/*!
 * \file src/draw.h
 *
 * \brief Prototypes for drawing routines.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 *  PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996, 2004 Thomas Nau
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

#ifndef	PCB_DRAW_H
#define	PCB_DRAW_H

#include "global.h"

void Draw (void);
void Redraw (void);
void DrawVia (PinType *);
void DrawRat (RatType *);
void DrawViaName (PinType *);
void DrawPin (PinType *);
void DrawPinName (PinType *);
void DrawPad (PadType *);
void DrawPadName (PadType *);
void DrawLine (LayerType *, LineType *);
void DrawArc (LayerType *, ArcType *);
void DrawText (LayerType *, TextType *);
void DrawPolygon (LayerType *, PolygonType *);
void DrawElement (ElementType *);
void DrawElementName (ElementType *);
void DrawElementPackage (ElementType *);
void DrawElementPinsAndPads (ElementType *);
void DrawObject (int, void *, void *);
void DrawLayer (LayerType *, const BoxType *);
void EraseVia (PinType *);
void EraseRat (RatType *);
void EraseViaName (PinType *);
void ErasePad (PadType *);
void ErasePadName (PadType *);
void ErasePin (PinType *);
void ErasePinName (PinType *);
void EraseLine (LineType *);
void EraseArc (ArcType *);
void EraseText (LayerType *, TextType *);
void ErasePolygon (PolygonType *);
void EraseElement (ElementType *);
void EraseElementPinsAndPads (ElementType *);
void EraseElementName (ElementType *);
void EraseObject (int, void *, void *);

void DrawLayerGroup (int side, const BoxType *drawn_area);
void DrawPaste (int side, const BoxType *drawn_area);
void DrawSilk (int side, const BoxType *drawn_area);
void DrawMask (int side, const BoxType *drawn_area);
void DrawHoles (bool draw_plated, bool draw_unplated, const BoxType *drawn_area, Cardinal g_from, Cardinal g_to);
void PrintAssembly (int side, const BoxType *drawn_area);

#endif
