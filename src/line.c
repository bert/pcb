/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
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


#include <stdlib.h>

#include "global.h"
#include "data.h"
#include "crosshair.h"
#include "gui.h"
#include "line.h"

/* ---------------------------------------------------------------------------
 * Adjust the attached line to 45 degrees if necessary
 */
void
AdjustAttachedLine (void)
{
  AttachedLineTypePtr line = &Crosshair.AttachedLine;

  /* I need at least one point */
  if (line->State == STATE_FIRST)
    return;
  /* don't draw outline when ctrl key is pressed */
  if (Settings.Mode == LINE_MODE && CtrlPressed ())
    {
      line->draw = False;
      return;
    }
  else
    line->draw = True;
  /* no 45 degree lines required */
  if (PCB->RatDraw || TEST_FLAG (ALLDIRECTIONFLAG, PCB))
    {
      line->Point2.X = Crosshair.X;
      line->Point2.Y = Crosshair.Y;
      return;
    }
  FortyFiveLine (line);
}

/* ---------------------------------------------------------------------------
 * makes the attached line fit into a 45 degree direction
 *
 * directions:
 *
 *           0
 *          7 1
 *         6   2
 *          5 3
 *           4
 */
void
FortyFiveLine (AttachedLineTypePtr Line)
{
  Location dx, dy, min;
  BYTE direction = 0;
  float m;

  /* first calculate direction of line */
  dx = Crosshair.X - Line->Point1.X;
  dy = Crosshair.Y - Line->Point1.Y;

  if (!dx)
    {
      if (!dy)
	/* zero length line, don't draw anything */
	return;
      else
	direction = dy > 0 ? 0 : 4;
    }
  else
    {
      m = (float) dy / (float) dx;
      direction = 2;
      if (m > TAN_30_DEGREE)
	direction = m > TAN_60_DEGREE ? 0 : 1;
      else if (m < -TAN_30_DEGREE)
	direction = m < -TAN_60_DEGREE ? 0 : 3;
    }
  if (dx < 0)
    direction += 4;

  dx = abs (dx);
  dy = abs (dy);
  min = MIN (dx, dy);

  /* now set up the second pair of coordinates */
  switch (direction)
    {
    case 0:
    case 4:
      Line->Point2.X = Line->Point1.X;
      Line->Point2.Y = Crosshair.Y;
      break;

    case 2:
    case 6:
      Line->Point2.X = Crosshair.X;
      Line->Point2.Y = Line->Point1.Y;
      break;

    case 1:
      Line->Point2.X = Line->Point1.X + min;
      Line->Point2.Y = Line->Point1.Y + min;
      break;

    case 3:
      Line->Point2.X = Line->Point1.X + min;
      Line->Point2.Y = Line->Point1.Y - min;
      break;

    case 5:
      Line->Point2.X = Line->Point1.X - min;
      Line->Point2.Y = Line->Point1.Y - min;
      break;

    case 7:
      Line->Point2.X = Line->Point1.X - min;
      Line->Point2.Y = Line->Point1.Y + min;
      break;
    }
}

/* ---------------------------------------------------------------------------
 *  adjusts the insert lines to make them 45 degrees as necessary
 */
void
AdjustTwoLine (int way)
{
  Location dx, dy;
  AttachedLineTypePtr line = &Crosshair.AttachedLine;

  if (Crosshair.AttachedLine.State == STATE_FIRST)
    return;
  /* don't draw outline when ctrl key is pressed */
  if (CtrlPressed ())
    {
      line->draw = False;
      return;
    }
  else
    line->draw = True;
  if (TEST_FLAG (ALLDIRECTIONFLAG, PCB))
    {
      line->Point2.X = Crosshair.X;
      line->Point2.Y = Crosshair.Y;
      return;
    }
  /* swap the modes if shift is held down */
  if (ShiftPressed ())
    way = !way;
  dx = Crosshair.X - line->Point1.X;
  dy = Crosshair.Y - line->Point1.Y;
  if (!way)
    {
      if (abs (dx) > abs (dy))
	{
	  line->Point2.X = Crosshair.X - SGN (dx) * abs (dy);
	  line->Point2.Y = line->Point1.Y;
	}
      else
	{
	  line->Point2.X = line->Point1.X;
	  line->Point2.Y = Crosshair.Y - SGN (dy) * abs (dx);
	}
    }
  else
    {
      if (abs (dx) > abs (dy))
	{
	  line->Point2.X = line->Point1.X + SGN (dx) * abs (dy);
	  line->Point2.Y = Crosshair.Y;
	}
      else
	{
	  line->Point2.X = Crosshair.X;
	  line->Point2.Y = line->Point1.Y + SGN (dy) * abs (dx);;
	}
    }
}
