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

static char *rcsid = "$Id$";

/* just defines common identifiers
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"
#include "dev_ps.h"
#include "dev_rs274x.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some shared identifiers
 */
XtAppContext Context;		/* application context */
Display *Dpy;			/* my display */
CrosshairType Crosshair;	/* information about cursor settings */
MarkType Marked;		/* a cross-hair mark */
OutputType Output;		/* some widgets ... used for drawing */
PCBTypePtr PCB;			/* pointer to layout struct */
Pixmap Offscreen = BadAlloc;	/* off screen drawing pixmap */
Pixmap Offmask = BadAlloc;	/* off screen mask pixmap */
char *Progname;			/* the program's name */
SettingType Settings;		/* some resources */
int LayerStack[MAX_LAYER];	/* determines the order in */
					/* the layers are drawn */
Atom WMDeleteWindowAtom;	/* all toplevel windows get */
					/* 'Delete' messages */
BufferType Buffers[MAX_BUFFER];	/* my buffers */
LibraryType Library;		/* the library */
Region UpRegion;		/* the region of updated drawing */
Region FullRegion;		/* the full screen clip region */
Boolean Bumped = False;		/* whether the undo serial number has changed */
Window LogWindID;
Location Xorig, Yorig;		/* origin offset for drawing directly in pixmap */
Boolean render;			/* wether or not to re-render the pixmap */
Pixmap *Stipples, XC_clock_source, XC_clock_mask;
Pixmap XC_hand_source, XC_hand_mask;
Pixmap XC_lock_source, XC_lock_mask;
int addedLines;
Location vxl, vxh, vyl, vyh;	/* visible pcb coordinates */
BoxType theScreen;		/* box of screen in pcb coordinates */
BoxType clipBox;		/* box for clipping of drawing */

float Zoom_Multiplier = 0.01;
/*  { 1.5625, 2.2097, 3.125, 4.4194, 6.25, 8.8388,
      12.5, 17.6777, 25, 35.3553, 50, 70.7106, 100,
      141.421, 200, 282.848, 400, 565.685, 800, 1131.37,
      1600, 2262.74, 3200, 4525.48, 6400 };
*/

/* ---------------------------------------------------------------------------
 * set all keys which generate a new line not 'no operation'.
 * 'Return' and 'Escape' are used as accelerators anyway
 */
String InputTranslations =
  "<Key>Linefeed: no-op()\n "
  "<Key>Return:   FinishInputDialog(OK)\n "
  "<Key>Escape:   FinishInputDialog(Cancel)\n "
  "Ctrl<Key>j:    no-op()\n "
  "Ctrl<Key>m:    no-op()\n " "Ctrl<Key>o:    no-op()\n ";

/* ---------------------------------------------------------------------------
 * all known printing devices
 */
DeviceInfoType PrintingDevice[] = {
  {PS_Query, NULL},
  {EPS_Query, NULL},
/*	{ GB_Query, NULL }, */
  {GBX_Queryh, NULL},
/*	{ GBX_Query, NULL }, */
  {NULL, NULL}
};
