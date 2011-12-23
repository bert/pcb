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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <setjmp.h>
#include <stdlib.h>


#include "global.h"
#include "data.h"
#include "crosshair.h"
#include "find.h"
#include "line.h"
#include "misc.h"
#include "rtree.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

static double drc_lines (PointTypePtr end, bool way);

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
  if (Settings.Mode == LINE_MODE && gui->control_is_pressed ())
    {
      line->draw = false;
      return;
    }
  else
    line->draw = true;
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
  Coord dx, dy, min;
  unsigned direction = 0;
  double m;

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
      m = (double) dy / dx;
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
AdjustTwoLine (bool way)
{
  Coord dx, dy;
  AttachedLineTypePtr line = &Crosshair.AttachedLine;

  if (Crosshair.AttachedLine.State == STATE_FIRST)
    return;
  /* don't draw outline when ctrl key is pressed */
  if (gui->control_is_pressed ())
    {
      line->draw = false;
      return;
    }
  else
    line->draw = true;
  if (TEST_FLAG (ALLDIRECTIONFLAG, PCB))
    {
      line->Point2.X = Crosshair.X;
      line->Point2.Y = Crosshair.Y;
      return;
    }
  /* swap the modes if shift is held down */
  if (gui->shift_is_pressed ())
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

struct drc_info
{
  LineTypePtr line;
  bool solder;
  jmp_buf env;
};

static int
drcVia_callback (const BoxType * b, void *cl)
{
  PinTypePtr via = (PinTypePtr) b;
  struct drc_info *i = (struct drc_info *) cl;

  if (!TEST_FLAG (FOUNDFLAG, via) && PinLineIntersect (via, i->line))
    longjmp (i->env, 1);
  return 1;
}

static int
drcPad_callback (const BoxType * b, void *cl)
{
  PadTypePtr pad = (PadTypePtr) b;
  struct drc_info *i = (struct drc_info *) cl;

  if (TEST_FLAG (ONSOLDERFLAG, pad) == i->solder &&
      !TEST_FLAG (FOUNDFLAG, pad) && LinePadIntersect (i->line, pad))
    longjmp (i->env, 1);
  return 1;
}

static int
drcLine_callback (const BoxType * b, void *cl)
{
  LineTypePtr line = (LineTypePtr) b;
  struct drc_info *i = (struct drc_info *) cl;

  if (!TEST_FLAG (FOUNDFLAG, line) && LineLineIntersect (line, i->line))
    longjmp (i->env, 1);
  return 1;
}

static int
drcArc_callback (const BoxType * b, void *cl)
{
  ArcTypePtr arc = (ArcTypePtr) b;
  struct drc_info *i = (struct drc_info *) cl;

  if (!TEST_FLAG (FOUNDFLAG, arc) && LineArcIntersect (i->line, arc))
    longjmp (i->env, 1);
  return 1;
}

/* drc_lines() checks for intersectors against two lines and
 * adjusts the end point until there is no intersection or
 * it winds up back at the start. If way is false it checks
 * straight start, 45 end lines, otherwise it checks 45 start,
 * straight end. 
 *
 * It returns the straight-line length of the best answer, and
 * changes the position of the input point to the best answer.
 */

static double
drc_lines (PointTypePtr end, bool way)
{
  double f, s, f2, s2, len, best;
  Coord dx, dy, temp, last, length;
  Coord temp2, last2, length2;
  LineType line1, line2;
  Cardinal group, comp;
  struct drc_info info;
  bool two_lines, x_is_long, blocker;
  PointType ans;

  f = 1.0;
  s = 0.5;
  last = -1;
  line1.Flags = line2.Flags = NoFlags ();
  line1.Thickness = Settings.LineThickness + 2 * (PCB->Bloat + 1);
  line2.Thickness = line1.Thickness;
  line1.Clearance = line2.Clearance = 0;
  line1.Point1.X = Crosshair.AttachedLine.Point1.X;
  line1.Point1.Y = Crosshair.AttachedLine.Point1.Y;
  dy = end->Y - line1.Point1.Y;
  dx = end->X - line1.Point1.X;
  if (abs (dx) > abs (dy))
    {
      x_is_long = true;
      length = abs (dx);
    }
  else
    {
      x_is_long = false;
      length = abs (dy);
    }
  group = GetGroupOfLayer (INDEXOFCURRENT);
  comp = max_group + 10;	/* this out-of-range group might save a call */
  if (GetLayerGroupNumberByNumber (solder_silk_layer) == group)
    info.solder = true;
  else
    {
      info.solder = false;
      comp = GetLayerGroupNumberByNumber (component_silk_layer);
    }
  temp = length;
  /* assume the worst */
  best = 0.0;
  ans.X = line1.Point1.X;
  ans.Y = line1.Point1.Y;
  while (length != last)
    {
      last = length;
      if (x_is_long)
	{
	  dx = SGN (dx) * length;
	  dy = end->Y - line1.Point1.Y;
	  length2 = abs (dy);
	}
      else
	{
	  dy = SGN (dy) * length;
	  dx = end->X - line1.Point1.X;
	  length2 = abs (dx);
	}
      temp2 = length2;
      f2 = 1.0;
      s2 = 0.5;
      last2 = -1;
      blocker = true;
      while (length2 != last2)
	{
	  if (x_is_long)
	    dy = SGN (dy) * length2;
	  else
	    dx = SGN (dx) * length2;
	  two_lines = true;
	  if (abs (dx) > abs (dy) && x_is_long)
	    {
	      line1.Point2.X = line1.Point1.X +
		(way ? SGN (dx) * abs (dy) : dx - SGN (dx) * abs (dy));
	      line1.Point2.Y = line1.Point1.Y + (way ? dy : 0);
	    }
	  else if (abs (dy) >= abs (dx) && !x_is_long)
	    {
	      line1.Point2.X = line1.Point1.X + (way ? dx : 0);
	      line1.Point2.Y = line1.Point1.Y +
		(way ? SGN (dy) * abs (dx) : dy - SGN (dy) * abs (dx));
	    }
	  else if (x_is_long)
	    {
	      /* we've changed which axis is long, so only do one line */
	      line1.Point2.X = line1.Point1.X + dx;
	      line1.Point2.Y =
		line1.Point1.Y + (way ? SGN (dy) * abs (dx) : 0);
	      two_lines = false;
	    }
	  else
	    {
	      /* we've changed which axis is long, so only do one line */
	      line1.Point2.Y = line1.Point1.Y + dy;
	      line1.Point2.X =
		line1.Point1.X + (way ? SGN (dx) * abs (dy) : 0);
	      two_lines = false;
	    }
	  line2.Point1.X = line1.Point2.X;
	  line2.Point1.Y = line1.Point2.Y;
	  if (!two_lines)
	    {
	      line2.Point2.Y = line1.Point2.Y;
	      line2.Point2.X = line1.Point2.X;
	    }
	  else
	    {
	      line2.Point2.X = line1.Point1.X + dx;
	      line2.Point2.Y = line1.Point1.Y + dy;
	    }
	  SetLineBoundingBox (&line1);
	  SetLineBoundingBox (&line2);
	  last2 = length2;
	  if (setjmp (info.env) == 0)
	    {
	      info.line = &line1;
	      r_search (PCB->Data->via_tree, &line1.BoundingBox, NULL,
			drcVia_callback, &info);
	      r_search (PCB->Data->pin_tree, &line1.BoundingBox, NULL,
			drcVia_callback, &info);
	      if (info.solder || comp == group)
		r_search (PCB->Data->pad_tree, &line1.BoundingBox, NULL,
			  drcPad_callback, &info);
	      if (two_lines)
		{
		  info.line = &line2;
		  r_search (PCB->Data->via_tree, &line2.BoundingBox, NULL,
			    drcVia_callback, &info);
		  r_search (PCB->Data->pin_tree, &line2.BoundingBox, NULL,
			    drcVia_callback, &info);
		  if (info.solder || comp == group)
		    r_search (PCB->Data->pad_tree, &line2.BoundingBox, NULL,
			      drcPad_callback, &info);
		}
	      GROUP_LOOP (PCB->Data, group);
	      {
		info.line = &line1;
		r_search (layer->line_tree, &line1.BoundingBox, NULL,
			  drcLine_callback, &info);
		r_search (layer->arc_tree, &line1.BoundingBox, NULL,
			  drcArc_callback, &info);
		if (two_lines)
		  {
		    info.line = &line2;
		    r_search (layer->line_tree, &line2.BoundingBox,
			      NULL, drcLine_callback, &info);
		    r_search (layer->arc_tree, &line2.BoundingBox,
			      NULL, drcArc_callback, &info);
		  }
	      }
	      END_LOOP;
	      /* no intersector! */
	      blocker = false;
	      f2 += s2;
	      len = (line2.Point2.X - line1.Point1.X);
	      len *= len;
	      len += (double) (line2.Point2.Y - line1.Point1.Y) *
		(line2.Point2.Y - line1.Point1.Y);
	      if (len > best)
		{
		  best = len;
		  ans.X = line2.Point2.X;
		  ans.Y = line2.Point2.Y;
		}
#if 0
	      if (f2 > 1.0)
		f2 = 0.5;
#endif
	    }
	  else
	    {
	      /* bumped into something, back off */
	      f2 -= s2;
	    }
	  s2 *= 0.5;
	  length2 = MIN (f2 * temp2, temp2);
	}
      if (!blocker && ((x_is_long && line2.Point2.X - line1.Point1.X == dx)
		       || (!x_is_long
			   && line2.Point2.Y - line1.Point1.Y == dy)))
	f += s;
      else
	f -= s;
      s *= 0.5;
      length = MIN (f * temp, temp);
    }

  end->X = ans.X;
  end->Y = ans.Y;
  return best;
}

void
EnforceLineDRC (void)
{
  PointType r45, rs;
  bool shift;
  double r1, r2;

  /* Silence a bogus compiler warning by storing this in a variable */
  int layer_idx = INDEXOFCURRENT;

  if ( gui->mod1_is_pressed() || gui->control_is_pressed () || PCB->RatDraw
      || layer_idx >= max_copper_layer)
    return;

  rs.X = r45.X = Crosshair.X;
  rs.Y = r45.Y = Crosshair.Y;
  /* first try starting straight */
  r1 = drc_lines (&rs, false);
  /* then try starting at 45 */
  r2 = drc_lines (&r45, true);
  shift = gui->shift_is_pressed ();
  if (XOR (r1 > r2, shift))
    {
      if (PCB->Clipping)
	{
	  if (shift)
	    PCB->Clipping = 2;
	  else
	    PCB->Clipping = 1;
	}
      Crosshair.X = rs.X;
      Crosshair.Y = rs.Y;
    }
  else
    {
      if (PCB->Clipping)
	{
	  if (shift)
	    PCB->Clipping = 1;
	  else
	    PCB->Clipping = 2;
	}
      Crosshair.X = r45.X;
      Crosshair.Y = r45.Y;
    }
}
