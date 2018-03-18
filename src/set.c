/*!
 * \file src/set.c
 *
 * \brief Routines to update widgets and global settings (except output
 * window and dialogs).
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
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
 *
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *
 * Thomas.Nau@rz.uni-ulm.de
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
#include "compat.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "find.h"
#include "misc.h"
#include "move.h"
#include "set.h"
#include "undo.h"
#include "pcb-printf.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

static int mode_position = 0;
static int mode_stack[MAX_MODESTACK_DEPTH];

/*!
 * \brief Sets cursor grid with respect to grid offset values.
 */
void
SetGrid (Coord Grid, bool align)
{
  char *grid_string;
  if (Grid >= 1 && Grid <= MAX_GRID)
    {
      if (align)
	{
	  PCB->GridOffsetX = Crosshair.X % Grid;
	  PCB->GridOffsetY = Crosshair.Y % Grid;
	}
      PCB->Grid = Grid;
      grid_string = pcb_g_strdup_printf ("%mr", Grid);
      if (grid_string)
        AttributePut (PCB, "PCB::grid::size", grid_string);
      g_free (grid_string);
      if (Settings.DrawGrid)
	Redraw ();
    }
}

/*!
 * \brief Sets a new line thickness.
 */
void
SetLineSize (Coord Size)
{
  if (Size >= MIN_LINESIZE && Size <= MAX_LINESIZE)
    {
      Settings.LineThickness = Size;
      if (TEST_FLAG (AUTODRCFLAG, PCB))
	FitCrosshairIntoGrid (Crosshair.X, Crosshair.Y);
    }
}

/*!
 * \brief Sets a new via thickness.
 */
void
SetViaSize (Coord Size, bool Force)
{
  if (Force || (Size <= MAX_PINORVIASIZE &&
		Size >= MIN_PINORVIASIZE &&
		Size >= Settings.ViaDrillingHole + MIN_PINORVIACOPPER))
    {
      Settings.ViaThickness = Size;
    }
}

/*!
 * \brief Sets a new via drilling hole.
 */
void
SetViaDrillingHole (Coord Size, bool Force)
{
  if (Force || (Size <= MAX_PINORVIASIZE &&
		Size >= MIN_PINORVIAHOLE &&
		Size <= Settings.ViaThickness - MIN_PINORVIACOPPER))
    {
      Settings.ViaDrillingHole = Size;
    }
}

void
SetViaSolderMaskClearance(Coord Size)
{
  Settings.ViaSolderMaskClearance = Size;
}

void
pcb_use_route_style (RouteStyleType * rst)
{
  Settings.LineThickness = rst->Thick;
  Settings.ViaThickness = rst->Diameter;
  Settings.ViaDrillingHole = rst->Hole;
  Settings.Keepaway = rst->Keepaway;
  Settings.ViaSolderMaskClearance = rst->ViaMask;
}

/*!
 * \brief Sets a keepaway width.
 */
void
SetKeepawayWidth (Coord Width)
{
  if (Width <= MAX_LINESIZE)
    {
      Settings.Keepaway = Width;
    }
}

/*!
 * \brief Sets a text scaling.
 */
void
SetTextScale (int Scale)
{
  if (Scale <= MAX_TEXTSCALE && Scale >= MIN_TEXTSCALE)
    {
      Settings.TextScale = Scale;
    }
}

/*!
 * \brief Sets or resets changed flag and redraws status.
 */
void
SetChangedFlag (bool New)
{
  if (PCB->Changed != New)
    {
      PCB->Changed = New;

    }
}

/*!
 * \brief Sets a new buffer number.
 */
void
SetBufferNumber (int Number)
{
  if (Number >= 0 && Number < MAX_BUFFER)
    {
      Settings.BufferNumber = Number;

      /* do an update on the crosshair range */
      crosshair_update_range();
    }
}

/*!
 * \brief Save mode.
 */
void
SaveMode (void)
{
  mode_stack[mode_position] = Settings.Mode;
  if (mode_position < MAX_MODESTACK_DEPTH - 1)
    mode_position++;
}

/*!
 * \brief Restore mode.
 */
void
RestoreMode (void)
{
  if (mode_position == 0)
    {
      Message ("hace: underflow of restore mode\n");
      return;
    }
  SetMode (mode_stack[--mode_position]);
}


/*!
 * \brief Set a new mode and update X cursor.
 *
 * Protect the cursor while changing the mode.
 *
 * Perform some additional stuff depending on the new mode.
 *
 * Reset 'state' of attached objects.
 */
void
SetMode (int Mode)
{
  static bool recursing = false;
  if (recursing)
    return;
  recursing = true;
  notify_crosshair_change (false);
  addedLines = 0;
  Crosshair.AttachedObject.Type = NO_TYPE;
  Crosshair.AttachedObject.State = STATE_FIRST;
  Crosshair.AttachedPolygon.PointN = 0;
  if (PCB->RatDraw)
    {
      if (Mode == ARC_MODE || Mode == RECTANGLE_MODE ||
	  Mode == VIA_MODE || Mode == POLYGON_MODE ||
	  Mode == POLYGONHOLE_MODE ||
	  Mode == TEXT_MODE || Mode == INSERTPOINT_MODE ||
	  Mode == THERMAL_MODE)
	{
	  Message (_("That mode is NOT allowed when drawing ratlines!\n"));
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
  /* Cancel rubberband move */
  else if (Settings.Mode == MOVE_MODE)
    MoveObjectAndRubberband (Crosshair.AttachedObject.Type,
                             Crosshair.AttachedObject.Ptr1,
                             Crosshair.AttachedObject.Ptr2,
                             Crosshair.AttachedObject.Ptr3,
                             0, 0);
  else
    {
      if (Settings.Mode == ARC_MODE || Settings.Mode == LINE_MODE)
	SetLocalRef (0, 0, false);
      Crosshair.AttachedBox.State = STATE_FIRST;
      Crosshair.AttachedLine.State = STATE_FIRST;
      if (Mode == LINE_MODE && TEST_FLAG (AUTODRCFLAG, PCB))
	{
	  if (ClearFlagOnAllObjects (true, CONNECTEDFLAG | FOUNDFLAG))
	    {
	      IncrementUndoSerialNumber ();
	      Draw ();
	    }
	}
    }

  Settings.Mode = Mode;
  crosshair_update_range();

  recursing = false;

  notify_crosshair_change (true);
}

/*!
 * \brief Set route style.
 */
void
SetRouteStyle (char *name)
{
  char num[10];

  STYLE_LOOP (PCB);
  {
    if (name && NSTRCMP (name, style->Name) == 0)
      {
	sprintf (num, "%d", n + 1);
	hid_actionl ("RouteStyle", num, NULL);
	break;
      }
  }
  END_LOOP;
}

void
SetLocalRef (Coord X, Coord Y, bool Showing)
{
  static MarkType old;
  static int count = 0;

  if (Showing)
    {
      notify_mark_change (false);
      if (count == 0)
	old = Marked;
      Marked.X = X;
      Marked.Y = Y;
      Marked.status = true;
      count++;
      notify_mark_change (true);
    }
  else if (count > 0)
    {
      notify_mark_change (false);
      count = 0;
      Marked = old;
      notify_mark_change (true);
    }
}
