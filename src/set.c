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

static int mode_position = 0;
static int mode_stack[MAX_MODESTACK_DEPTH];

/* ---------------------------------------------------------------------------
 * output of cursor position
 */
void
SetCursorStatusLine (void)
{
  char text[64];

  if (Marked.status)
    sprintf (text,
	     "%-i.%02d, %-i.%02d <%-li.%02d, %-li.%02d> (%-.2fmm, %-.2fmm)",
	     Crosshair.X / 100, abs(Crosshair.X % 100), Crosshair.Y / 100,
	     abs(Crosshair.Y % 100), (Crosshair.X - Marked.X) / 100,
	     abs(Crosshair.X - Marked.X) % 100, (Crosshair.Y - Marked.Y) / 100,
	     abs(Crosshair.Y - Marked.Y) % 100,
	     COOR_TO_MM * (Crosshair.X - Marked.X),
	     COOR_TO_MM * (Crosshair.Y - Marked.Y));
  else
    sprintf (text, "%-i.%02d,%-i.%02d", Crosshair.X / 100, abs(Crosshair.X % 100),
	     Crosshair.Y / 100, abs(Crosshair.Y % 100));
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
	     "%c %s, grid=%i.%02i:%i,%s%sline=%i.%02i, via=%i.%02i(%i.%02i),"
	     " clearance=%i.%02i, text=%i%%, buffer=#%li, name: ",
	     PCB->Changed ? '*' : ' ',
	     Settings.ShowSolderSide ? "solder" : "component",
	     (int) PCB->Grid / 100, (int) (PCB->Grid) % 100,
	     (int) Settings.GridFactor,
	     TEST_FLAG (ALLDIRECTIONFLAG,
			PCB) ? "all" : (PCB->Clipping ==
					0 ? "45" : (PCB->Clipping ==
						    1 ? "45_/" : "45\\_")),
	     TEST_FLAG (RUBBERBANDFLAG, PCB) ? ",R, " : ", ",
	     Settings.LineThickness / 100, Settings.LineThickness % 100,
	     Settings.ViaThickness / 100, Settings.ViaThickness % 100,
	     Settings.ViaDrillingHole / 100, Settings.ViaDrillingHole % 100,
	     Settings.Keepaway / 100, Settings.Keepaway % 100,
	     Settings.TextScale, Settings.BufferNumber + 1);
  else
    sprintf (text,
	     "%c %s, grid=%4.2fmm:%i,%s%sline=%i.%02i, via=%i.%02i(%i.%02i),"
	     " clearance=%i.%02i, text=%i%%, buffer=#%li, name: ",
	     PCB->Changed ? '*' : ' ',
	     Settings.ShowSolderSide ? "solder" : "component",
	     PCB->Grid * COOR_TO_MM, (int) Settings.GridFactor,
	     TEST_FLAG (ALLDIRECTIONFLAG,
			PCB) ? "all" : (PCB->Clipping ==
					0 ? "45" : (PCB->Clipping ==
						    1 ? "45_/" : "45\\_")),
	     TEST_FLAG (RUBBERBANDFLAG, PCB) ? ",R, " : ", ",
	     Settings.LineThickness / 100, Settings.LineThickness % 100,
	     Settings.ViaThickness / 100, Settings.ViaThickness % 100,
	     Settings.ViaDrillingHole / 100, Settings.ViaDrillingHole % 100,
	     Settings.Keepaway / 100, Settings.Keepaway % 100,
	     Settings.TextScale, Settings.BufferNumber + 1);

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
SetZoom (float Zoom)
{
  int old_x, old_y;

  /* special argument of 1000 zooms to extents plus 10 percent */
  if (Zoom == 1000.0)
    {
      BoxTypePtr box = GetDataBoundingBox (PCB->Data);
      if (!box)
	return;
      Zoom =
	logf (0.011 * (float) (box->X2 - box->X1) / Output.Width) / LN_2_OVER_2;
      Zoom =
	MAX (Zoom, logf (0.011 * (float) (box->Y2 - box->Y1) / Output.Height) /
	     LN_2_OVER_2);
      Crosshair.X = (box->X1 + box->X2) / 2;
      Crosshair.Y = (box->Y1 + box->Y2) / 2;
      old_x = Output.Width / 2;
      old_y = Output.Height / 2;
    }
  else
    {
      old_x = TO_SCREEN_X (Crosshair.X);
      old_y = TO_SCREEN_Y (Crosshair.Y);
    }
  Zoom = MAX (MIN_ZOOM, Zoom);
  Zoom = MIN (MAX_ZOOM, Zoom);
  Zoom_Multiplier = 0.01/expf(Zoom * LN_2_OVER_2);

  /* redraw only if something changed */
  if (PCB->Zoom != Zoom)
    {
      PCB->Zoom = Zoom;
      RedrawZoom (old_x, old_y);
    }
}

void
RedrawZoom (Position old_x, Position old_y)
{
  ScaleOutput (Output.Width, Output.Height);
  if (CoalignScreen (old_x, old_y, Crosshair.X, Crosshair.Y))
    warpNoWhere ();

  UpdateAll ();
  /* always redraw status line (used for init sequence) */
  SetStatusLine ();
}

/* ---------------------------------------------------------------------------
 * sets a new line thickness
 */
void
SetLineSize (BDimension Size)
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
SetViaSize (BDimension Size, Boolean Force)
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
SetViaDrillingHole (BDimension Size, Boolean Force)
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
SetKeepawayWidth (BDimension Width)
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

void
SaveMode (void)
{
  mode_stack[mode_position] = Settings.Mode;
  if (mode_position < MAX_MODESTACK_DEPTH -1)
    mode_position++;
}

void
RestoreMode (void)
{
  if (mode_position == 0)
    {
      Message("hace: underflow of restore mode\n");
      return;
    }
  SetMode(mode_stack[--mode_position]);
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
      if (Settings.Mode == ARC_MODE || Settings.Mode == LINE_MODE)
        SetLocalRef(0, 0, False);
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

  STYLE_LOOP (PCB, 
    {
      if (name && strcmp (name, style->Name) == 0)
	{
	  arg = &num[0];
	  sprintf (num, "%d", n + 1);
	  CallActionProc (Output.Output, "RouteStyle", NULL, &arg, 1);
	  break;
	}
    }
  );
}

void
SetLocalRef (Location X, Location Y, Boolean Showing)
{
  static MarkType old;
  static int count = 0;

  if (Showing)
    {
      HideCrosshair (True);
      if (count == 0)
        old = Marked;
      Marked.X = X;
      Marked.Y = Y;
      Marked.status = True;
      count++;
      RestoreCrosshair (False);
    }
  else if (count > 0)
    {
      HideCrosshair (False);
      count = 0;
      Marked = old;
      RestoreCrosshair (False);
    }
}
