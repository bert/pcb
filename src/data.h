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

/* common identifiers
 */

#ifndef	__DATA_INCLUDED__
#define	__DATA_INCLUDED__

#include "global.h"
#include <X11/Intrinsic.h>

/* ---------------------------------------------------------------------------
 * some shared identifiers
 */
extern	XtAppContext	Context;
extern	Display		*Dpy;
extern	CrosshairType	Crosshair;
extern  MarkType	Marked;
extern	OutputType	Output;
extern	PCBTypePtr	PCB;
extern	Pixmap		Offscreen, Offmask;
extern	char		*Progname;
extern	SettingType	Settings;
extern	Boolean		RedrawOnEnter;
extern	int		LayerStack[MAX_LAYER];
extern	String		InputTranslations;
extern	Atom		WMDeleteWindowAtom;
extern	BufferType	Buffers[MAX_BUFFER];
extern	LibraryType	Library;
extern	DeviceInfoType	PrintingDevice[];
extern	Pixmap		*Stipples, XC_clock_source, XC_clock_mask;
extern  Pixmap          XC_hand_source, XC_hand_mask;
extern  Pixmap          XC_lock_source, XC_lock_mask;
extern	int		addedLines;
extern  Region		UpRegion;
extern	Region		FullRegion;
extern	Boolean		Bumped;
extern	Window		LogWindID;
extern	Location	Xorig, Yorig;
extern	Boolean		render;
extern	Location	vxl, vxh, vyl, vyh;
extern  BoxType         theScreen, clipBox;
extern	float		Zoom_Multiplier;
#endif
