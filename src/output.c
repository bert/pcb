/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
 *
 *  This module, output.c, was written and is Copyright (C) 1999 harry eaton
 *  portions of code taken from main.c by Thomas Nau
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gui.h"

#include "action.h"
#include "crosshair.h"
#include "draw.h"
#include "error.h"
#include "output.h"
#include "misc.h"
#include "set.h"



/* Pan the screen so that pcb coordinate xp, yp is displayed at screen
 * coordinates xs,ys.
 * Returns TRUE if that is not possible, FALSE otherwise.
 */
gboolean
CoalignScreen(Position xs, Position ys,
			LocationType xp, LocationType yp)
	{
	LocationType x, y;

#ifdef DEBUGDISP
	g_message ("CoalignScreen(%d %d %d %d)\n", xs, ys, xp, yp);
#endif
	x = xp - TO_PCB (xs);
	if (SWAP_IDENT)
		y  = PCB->MaxHeight - yp - TO_PCB(ys);
	else
		y = yp - TO_PCB (ys);

	return Pan(x, y, FALSE, TRUE);
	}

void
DrawClipped(GdkRegion *myRegion)
	{
	BoxType			drawBox;
	GdkRectangle	clipBox;

	/* pixmap and screen have different clip regions
	*/
	if (Output.pixmap)
		{
		gdk_region_get_clipbox(myRegion, &clipBox);
#ifdef DEBUGDISP
		g_message ("ClipBox= (%d, %d), size = (%d, %d)\n",
				clipBox.x, clipBox.y, clipBox.width, clipBox.height);
#endif
		}
	else
		/* clip drawing to only exposed region */
		{
#ifdef DEBUGDISP
		g_message ("Updating with no pixmap!\n");
#endif
		gdk_gc_set_clip_region(Output.fgGC, myRegion);
		gdk_gc_set_clip_region(Output.bgGC, myRegion);
		gdk_gc_set_clip_region(Output.GridGC, myRegion);
		gdk_gc_set_function(Output.pmGC, GDK_CLEAR);
		gdk_draw_rectangle(Output.mask, Output.pmGC, TRUE,
					0, 0, Output.Width, Output.Height);
		gdk_gc_set_clip_region(Output.pmGC, myRegion);
		}
	drawBox.X1 = clipBox.x;
	drawBox.Y1 = clipBox.y;
	drawBox.X2 = clipBox.x + clipBox.width;
	drawBox.Y2 = clipBox.y + clipBox.height;
	RedrawOutput (&drawBox);

	if (!Output.pixmap)
		{
		gdk_gc_set_clip_region(Output.fgGC, FullRegion);
		gdk_gc_set_clip_region(Output.bgGC, FullRegion);
		gdk_gc_set_clip_region(Output.GridGC, FullRegion);
		gdk_gc_set_clip_region(Output.pmGC, FullRegion);
		}
	else
		/* if output is drawn in pixmap, copy it */
		{
		HideCrosshair (TRUE);
		gdk_draw_drawable(Output.drawing_area->window,
				Output.fgGC, Output.pixmap,
				clipBox.x, clipBox.y, clipBox.x, clipBox.y,
				clipBox.width, clipBox.height);
		RestoreCrosshair (TRUE);
		}
	}

/* Pan shifts the origin of the output window to PCB coordinates X,Y if
 * possible. if X,Y is out of range, it goes as far as possible and
 * returns TRUE. Otherwise it goest to X,Y and returns false.
 * If Scroll is TRUE and there is no offscreen pixmap, the display
 * is scrolled and only the newly visible part gets updated.
 * If Update is TRUE, an update event is generated so the display
 * is redrawn.
 */
gboolean
Pan(LocationType X, LocationType Y,
			gboolean Scroll, gboolean Update)
	{
	OutputType		*out = &Output;
	gboolean		clip = FALSE;
	static Position	x, y;
	GdkRegion		*myRegion;
	GdkRectangle	rect;


#ifdef DEBUGDISP
	g_message ("Pan(%d, %d, %s, %s)\n", X, Y, Scroll ? "TRUE" : "FALSE",
				Update ? "TRUE" : "FALSE");
#endif
	if (X < 0)
		{
		clip = TRUE;
		X = 0;
		}
	else if (X > PCB->MaxWidth - TO_PCB (Output.Width))
		{
		clip = TRUE;
		X = MAX (0, PCB->MaxWidth - TO_PCB (Output.Width));
		}
	if (Y < 0)
		{
		clip = TRUE;
		Y = 0;
		}
	else if (Y > PCB->MaxHeight - TO_PCB (Output.Height))
		{
		clip = TRUE;
		Y = MAX (0, PCB->MaxHeight - TO_PCB (Output.Height));
		}

	Xorig = X;
	Yorig = Y;
	render = TRUE;

	/* calculate the visbile bounds in pcb coordinates
	*/
	theScreen.X1 = vxl = TO_PCB_X (0);
	theScreen.X2 = vxh = TO_PCB_X (Output.Width);
	theScreen.Y1 = vyl = MAX (0, MIN (TO_PCB_Y (0), TO_PCB_Y (Output.Height)));
	theScreen.Y2 = vyh = MIN (PCB->MaxHeight, MAX (TO_PCB_Y (0), TO_PCB_Y (Output.Height)));

	/* set up the clipBox
	*/
	clipBox = theScreen;
#ifdef CLIPDEBUG
	clipBox.X1 += TO_PCB(50);
	clipBox.X2 -= TO_PCB(50);
	clipBox.Y1 += TO_PCB(50);
	clipBox.Y2 -= TO_PCB(50);
#else
	clipBox.X1 -= MAX_SIZE;
	clipBox.X2 += MAX_SIZE;
	clipBox.Y1 -= MAX_SIZE;
	clipBox.Y2 += MAX_SIZE;
#endif
#ifdef DEBUGDISP
	g_message ("Pan setting Xorig, Yorig = (%d,%d)\n", X, Y);
	g_message ("Visible is (%d < X < %d), (%d < Y < %d)\n",vxl,vxh,vyl,vyh);
#endif

	/* ignore the no - change case 
	| BUG: if zoom or viewed changed, need to redraw anyway.
	| clip can fix all?? of these?
	*/
	if (Xorig == x && Yorig == y && !clip)
		{
		return clip;
		}

	/* scroll the pixmap
	*/
	myRegion = gdk_region_new();

	if (Scroll && !out->pixmap)
		{
		gdk_draw_drawable(out->drawing_area->window, out->bgGC,
						out->drawing_area->window,
						0, 0, TO_SCREEN (Xorig - x), TO_SCREEN (Yorig - y),
						out->Width, out->Height);

		/* do same for mask ?
		*/
		if (out->mask)
			gdk_draw_drawable(out->mask, out->bgGC, out->mask,
						0, 0, TO_SCREEN (Xorig - x), TO_SCREEN (Yorig - y),
						out->Width, out->Height);
		if (y != Yorig)
			{
			rect.x = 0;
			rect.y = (Yorig - y > 0) ? 0 : out->Height + TO_SCREEN (Yorig - y);
			rect.width = out->Width;
			rect.height = abs (TO_SCREEN (y - Yorig));
			gdk_region_union_with_rect(myRegion, &rect);
#if 0
			gdk_draw_rectangle(out->drawing_area->window, out->bgGC, TRUE,
					rect.x, rect.y, rect.width, rect.height);
#endif
			}
		if (x != Xorig)
			{
			rect.x = (Xorig - x > 0) ? 0 : out->Width + TO_SCREEN (Xorig - x);
			rect.y = 0;
			rect.width = abs (TO_SCREEN (x - Xorig));
			rect.height = out->Height;
			gdk_region_union_with_rect(myRegion, &rect);
#if 0
			gdk_draw_rectangle(out->drawing_area->window, out->bgGC, TRUE,
					rect.x, rect.y, rect.width, rect.height);
#endif
			}
		}
	else
		{
		/* the offscreen pixmap is entirely re-drawn
		*/
		rect.x = 0;
		rect.y = 0;
		rect.width = out->Width;
		rect.height = out->Height;
		gdk_region_union_with_rect(myRegion, &rect);
		}

	if (Update)
		{
		gui_output_positioners_set_values(Xorig, Yorig);
		DrawClipped(myRegion);
		}
	gdk_region_destroy(myRegion);

	x = Xorig;
	y = Yorig;
	return clip;
	}

gboolean
ActiveDrag (void)
{
  gboolean active = FALSE;

  switch (Settings.Mode)
    {
    case POLYGON_MODE:
      if (Crosshair.AttachedLine.State != STATE_FIRST)
	active = TRUE;
      break;

    case ARC_MODE:
      if (Crosshair.AttachedBox.State != STATE_FIRST)
	active = TRUE;
      break;

    case LINE_MODE:
      if (Crosshair.AttachedLine.State != STATE_FIRST)
	active = TRUE;
      break;

    case COPY_MODE:
    case MOVE_MODE:
    case INSERTPOINT_MODE:
      if (Crosshair.AttachedObject.Type != NO_TYPE)
	active = TRUE;
      break;
    default:
      break;
    }

  if (Crosshair.AttachedBox.State == STATE_SECOND)
    active = TRUE;

  return (active);
}

