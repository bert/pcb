/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998 Thomas Nau
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

#ifndef __OUTPUT_INCLUDED__
#define __OUTPUT_INCLUDED__

#include "global.h"

void PortholeEvent(Widget, XtPointer, XEvent *, Boolean *);
Boolean Pan(LocationType, LocationType, Boolean, Boolean);
Boolean CoalignScreen(Position, Position, LocationType, LocationType);
void OutputEvent(Widget, XtPointer, XEvent *, Boolean *);
void GetSizeOfDrawingArea(void);
void ScaleOutput(Dimension, Dimension);
Widget InitOutputPanner(Widget, Widget, Widget);
void SetOutputLabel(Widget, char *);
void UpdateExposed(XEvent *);
void CB_StopScroll (Widget, XtPointer, XEvent *, Boolean *);
#endif
