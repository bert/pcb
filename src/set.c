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

/* routines to update widgets and global settings
 * (except output window and dialogs)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "global.h"

#include "action.h"
#include "buffer.h"
#include "crosshair.h"
#include "control.h"
#include "data.h"
#include "error.h"
#include "draw.h"
#include "gui.h"
#include "menu.h"
#include "misc.h"
#include "output.h"
#include "set.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * output of cursor position
 */
void
SetCursorStatusLine (void)
{
  char text[64];

  if (Marked.status)
    sprintf (text, "%-i,%-i <%-i,%-i> (%-.2fmm,%-.2fmm)", Crosshair.X,
	     Crosshair.Y, Crosshair.X - Marked.X, Crosshair.Y - Marked.Y,
             MIL_TO_MM * (Crosshair.X - Marked.X),
             MIL_TO_MM * (Crosshair.Y - Marked.Y));
  else
    sprintf (text, "%-i,%-i", Crosshair.X, Crosshair.Y);
  SetOutputLabel (Output.CursorPosition, text);
}

/* ---------------------------------------------------------------------------
 * output of status line
 */
void
SetStatusLine (void)
{
  char text[140];
  int length;

  if (PCB->Grid == (int) PCB->Grid)
    sprintf (text,
	     "%c %s, grid=%i:%i,%s%szoom=%-i:%-i, line=%-i, via=%-i(%-i),"
	     "keepaway=%-i, text=%i%%, buffer=#%-i, name: ",
	     PCB->Changed ? '*' : ' ',
	     Settings.ShowSolderSide ? "solder" : "component",
	     (int) PCB->Grid,
	     (int) Settings.GridFactor,
	     TEST_FLAG (ALLDIRECTIONFLAG,
			PCB) ? "all" : (PCB->Clipping ==
					0 ? "45" : (PCB->Clipping ==
						    1 ? "45_/" : "45\\_")),
	     TEST_FLAG (RUBBERBANDFLAG, PCB) ? ",R, " : ", ",
	     (int) (TO_SCREEN (1) ? TO_SCREEN (1) : 1),
	     (int) (TO_PCB (1) ? TO_PCB (1) : 1),
	     (int) Settings.LineThickness, (int) Settings.ViaThickness,
	     (int) Settings.ViaDrillingHole, (int) Settings.Keepaway,
	     (int) Settings.TextScale, Settings.BufferNumber + 1);
  else
    sprintf (text,
	     "%c %s, grid=%4.2fmm:%i,%s%szoom=%-i:%-i, line=%-i, via=%-i(%-i),"
	     "keepaway=%-i, text=%i%%, buffer=#%-i, name: ",
	     PCB->Changed ? '*' : ' ',
	     Settings.ShowSolderSide ? "solder" : "component",
	     PCB->Grid * MIL_TO_MM, (int) Settings.GridFactor,
	     TEST_FLAG (ALLDIRECTIONFLAG,
			PCB) ? "all" : (PCB->Clipping ==
					0 ? "45" : (PCB->Clipping ==
						    1 ? "45_/" : "45\\_")),
	     TEST_FLAG (RUBBERBANDFLAG, PCB) ? ",R, " : ", ",
	     (int) (TO_SCREEN (1) ? TO_SCREEN (1) : 1),
	     (int) (TO_PCB (1) ? TO_PCB (1) : 1),
	     (int) Settings.LineThickness, (int) Settings.ViaThickness,
	     (int) Settings.ViaDrillingHole, (int) Settings.Keepaway,
	     (int) Settings.TextScale, Settings.BufferNumber + 1);

  /* append the name of the layout */
  length = sizeof (text) - 1 - strlen (text);
  strncat (text, UNKNOWN (PCB->Name), length);
  text[sizeof (text) - 1] = '\0';
  SetOutputLabel (Output.StatusLine, text);
}

/* ---------------------------------------------------------------------------
 * sets cursor grid with respect to grid offset values
 */
void
SetGrid (float Grid, Boolean align)
{
  if (Grid >= 1 && Grid <= MAX_GRID)
    {
      if (align)
	{
	  PCB->GridOffsetX =
	    Crosshair.X - (int) (Crosshair.X / Grid) * Grid + 0.5;
	  PCB->GridOffsetY =
	    Crosshair.Y - (int) (Crosshair.Y / Grid) * Grid + 0.5;
	}
      PCB->Grid = Grid;
      if (Settings.DrawGrid)
	UpdateAll ();
      SetStatusLine ();
    }
}

/* ---------------------------------------------------------------------------
* sets new zoom factor, adapts size of viewport and centers the cursor
*/
void
SetZoom (int Zoom)
{
  int localMin, old_x, old_y;

  /* special argument of 1000 zooms to extents */
  if (Zoom == 1000)
    {
      BoxTypePtr box = GetDataBoundingBox (PCB->Data);
      if (!box)
	return;
      Zoom = 1 + log ((float) (box->X2 - box->X1) / Output.Width) / log (2);
      Zoom =
	MAX (Zoom,
	     1 + log ((float) (box->Y2 - box->Y1) / Output.Height) / log (2));
      Crosshair.X = (box->X1 + box->X2) / 2;
      Crosshair.Y = (box->Y1 + box->Y2) / 2;
      old_x = Output.Width / 2;
      old_y = Output.Height / 2;
    }
  else
    {
      if (MAX (PCB->MaxWidth, PCB->MaxHeight) > (1 << 14))
	localMin = MIN_ZOOM;
      else if (MAX (PCB->MaxWidth, PCB->MaxHeight) > (1 << 13))
	localMin = MIN_ZOOM - 1;
      else
	localMin = MIN_ZOOM - 2;
      Zoom = MAX (localMin, Zoom);
      Zoom = MIN (MAX_ZOOM, Zoom);
      old_x = TO_SCREEN_X (Crosshair.X);
      old_y = TO_SCREEN_Y (Crosshair.Y);
    }

  /* redraw only if something changed */
  if (PCB->Zoom != Zoom || old_x != TO_SCREEN_X (Crosshair.X)
      || old_y != TO_SCREEN_Y (Crosshair.Y))
    {
      PCB->Zoom = Zoom;
      ScaleOutput (Output.Width, Output.Height);
      if (CoalignScreen (old_x, old_y, Crosshair.X, Crosshair.Y))
	warpNoWhere ();

      UpdateAll ();
    }
  /* always redraw status line (used for init sequence) */
  SetStatusLine ();
}

/* ---------------------------------------------------------------------------
 * sets a new line thickness
 */
void
SetLineSize (Dimension Size)
{
  if (Size >= MIN_LINESIZE && Size <= MAX_LINESIZE)
    {
      Settings.LineThickness = Size;
      SetStatusLine ();
    }
}

/* ---------------------------------------------------------------------------
 * sets a new via thickness
 */
void
SetViaSize (Dimension Size, Boolean Force)
{
  if (Force || (Size <= MAX_PINORVIASIZE &&
		Size >= MIN_PINORVIASIZE &&
		Size >= Settings.ViaDrillingHole + MIN_PINORVIACOPPER))
    {
      Settings.ViaThickness = Size;
      SetStatusLine ();
    }
}

/* ---------------------------------------------------------------------------
 * sets a new via drilling hole
 */
void
SetViaDrillingHole (Dimension Size, Boolean Force)
{
  if (Force || (Size <= MAX_PINORVIASIZE &&
		Size >= MIN_PINORVIAHOLE &&
		Size <= Settings.ViaThickness - MIN_PINORVIACOPPER))
    {
      Settings.ViaDrillingHole = Size;
      SetStatusLine ();
    }
}

/* ---------------------------------------------------------------------------
 * sets a keepaway width
 */
void
SetKeepawayWidth (Dimension Width)
{
  if (Width <= MAX_LINESIZE && Width >= MIN_LINESIZE)
    {
      Settings.Keepaway = Width;
      SetStatusLine ();
    }
}

/* ---------------------------------------------------------------------------
 * sets a text scaling
 */
void
SetTextScale (Dimension Scale)
{
  if (Scale <= MAX_TEXTSCALE && Scale >= MIN_TEXTSCALE)
    {
      Settings.TextScale = Scale;
      SetStatusLine ();
    }
}

/* ---------------------------------------------------------------------------
 * sets or resets changed flag and redraws status
 */
void
SetChangedFlag (Boolean New)
{
  if (PCB->Changed != New)
    {
      PCB->Changed = New;
      SetStatusLine ();
    }
}

/* ---------------------------------------------------------------------------
 * sets the crosshair range to the current buffer extents 
 */
void
SetCrosshairRangeToBuffer (void)
{
  if (Settings.Mode == PASTEBUFFER_MODE)
    {
      SetBufferBoundingBox (PASTEBUFFER);
      SetCrosshairRange (PASTEBUFFER->X - PASTEBUFFER->BoundingBox.X1,
			 PASTEBUFFER->Y - PASTEBUFFER->BoundingBox.Y1,
			 PCB->MaxWidth -
			 (PASTEBUFFER->BoundingBox.X2 - PASTEBUFFER->X),
			 PCB->MaxHeight -
			 (PASTEBUFFER->BoundingBox.Y2 - PASTEBUFFER->Y));
    }
}

/* ---------------------------------------------------------------------------
 * sets a new buffer number
 */
void
SetBufferNumber (int Number)
{
  if (Number >= 0 && Number < MAX_BUFFER)
    {
      Settings.BufferNumber = Number;

      /* do an update on the crosshair range */
      SetCrosshairRangeToBuffer ();
      SetStatusLine ();
    }
}

/* ---------------------------------------------------------------------------
 * updates all widgets like status, cursor position ... on screen
 */
void
UpdateSettingsOnScreen (void)
{
  SetStatusLine ();
  SetCursorStatusLine ();
  UpdateControlPanel ();
  UpdateSizesMenu ();
}

/* ---------------------------------------------------------------------------
 * set a new mode and update X cursor
 */
void
SetMode (int Mode)
{
  static Boolean recursing = False;
  /* protect the cursor while changing the mode
   * perform some additional stuff depending on the new mode
   * reset 'state' of attached objects
   */
  if (recursing)
    return;
  recursing = True;
  HideCrosshair (True);
  addedLines = 0;
  Crosshair.AttachedObject.Type = NO_TYPE;
  Crosshair.AttachedObject.State = STATE_FIRST;
  Crosshair.AttachedPolygon.PointN = 0;
  if (PCB->RatDraw)
    {
      if (Mode == ARC_MODE || Mode == RECTANGLE_MODE ||
	  Mode == VIA_MODE || Mode == POLYGON_MODE ||
	  Mode == TEXT_MODE || Mode == INSERTPOINT_MODE ||
	  Mode == THERMAL_MODE)
	{
	  Message ("That mode is NOT allowed when drawing " "ratlines!\n");
	  Mode = NO_MODE;
	}
    }
  if (Settings.Mode == LINE_MODE && Mode == ARC_MODE &&
      Crosshair.AttachedLine.State != STATE_FIRST)
    {
      Crosshair.AttachedLine.State = STATE_FIRST;
      Crosshair.AttachedBox.State = STATE_SECOND;
      Crosshair.AttachedBox.Point1.X = Crosshair.AttachedBox.Point2.X =
	Crosshair.AttachedLine.Point1.X;
      Crosshair.AttachedBox.Point1.Y = Crosshair.AttachedBox.Point2.Y =
	Crosshair.AttachedLine.Point1.Y;
      AdjustAttachedObjects ();
    }
  else if (Settings.Mode == ARC_MODE && Mode == LINE_MODE &&
	   Crosshair.AttachedBox.State != STATE_FIRST)
    {
      Crosshair.AttachedBox.State = STATE_FIRST;
      Crosshair.AttachedLine.State = STATE_SECOND;
      Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X =
	Crosshair.AttachedBox.Point1.X;
      Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y =
	Crosshair.AttachedBox.Point1.Y;
      Settings.Mode = Mode;
      AdjustAttachedObjects ();
    }
  else
    {
      Crosshair.AttachedBox.State = STATE_FIRST;
      Crosshair.AttachedLine.State = STATE_FIRST;
    }

  Settings.Mode = Mode;
  modeCursor (Mode);
  if (Mode == PASTEBUFFER_MODE)
    /* do an update on the crosshair range */
    SetCrosshairRangeToBuffer ();
  else
    SetCrosshairRange (0, 0, PCB->MaxWidth, PCB->MaxHeight);

  UpdateModeSelection ();
  recursing = False;

  /* force a crosshair grid update because the valid range
   * may have changed
   */
  MoveCrosshairRelative (0, 0);
  RestoreCrosshair (True);
}

void
SetRouteStyle (char *name)
{
  char *arg, num[10];

  STYLE_LOOP (PCB, if (name && strcmp (name, style->Name) == 0)
	      {
	      arg = &num[0];
	      sprintf (num, "%d", n + 1);
	      CallActionProc (Output.Output, "RouteStyle", NULL, &arg, 1);
	      break;}
  );
}

void
SetLocalRef (Position X, Position Y, Boolean Showing)
{
  static MarkType old;

  if (Showing)
    {
      old = Marked;
      Marked.X = X;
      Marked.Y = Y;
      Marked.status = True;
    }
  else
    {
      Marked = old;
    }
}
