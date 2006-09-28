/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,2004,2006 Thomas Nau
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


/* thermal manipulation functions
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdarg.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <assert.h>
#include <setjmp.h>
#include <memory.h>
#include <ctype.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#include "global.h"

#include "create.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "misc.h"
#include "move.h"
#include "output.h"
#include "polygon.h"
#include "rtree.h"
#include "thermal.h"
#include "undo.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

/* thermal_backward_compat is used to handle the creation
 * of thermal lines needed by old format files where thermals
 * exisited only in the form of flags
 */

void
thermal_backward_compat (PCBTypePtr pcb, PinTypePtr Pin)
{
  if (!pcb || !pcb->Data)
    return;
  LAYER_LOOP (pcb->Data, max_layer);
  {
    if (TEST_THERM (n, Pin))
      {
	LineTypePtr Line;
	BDimension half = (Pin->Thickness + Pin->Clearance + 1) / 2;

	if (!TEST_FLAG (SQUAREFLAG, Pin))
	  half = (half * M_SQRT1_2 + 1);
	Line = CreateNewLineOnLayer (layer, Pin->X - half, Pin->Y - half,
				     Pin->X + half, Pin->Y + half,
				     (Pin->Thickness -
				      Pin->DrillingHole) * pcb->ThermScale, 0,
				     MakeFlags (USETHERMALFLAG));
	Line =
	  CreateNewLineOnLayer (layer, Pin->X - half, Pin->Y + half,
				Pin->X + half, Pin->Y - half,
				(Pin->Thickness -
				 Pin->DrillingHole) * pcb->ThermScale, 0,
				MakeFlags (USETHERMALFLAG));
      }
  }
  END_LOOP;
}

struct therm_info
{
  LineTypePtr line;
  jmp_buf env;
};

static int
visit_therm (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  CLEAR_FLAG (VISITFLAG, line);
  return 1;
}

static int
pin_therm (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  struct therm_info *info = (struct therm_info *) cl;

  if (!TEST_FLAG (USETHERMALFLAG, line))
    return 0;
  if (TEST_FLAG (VISITFLAG, line))
    return 0;
  SET_FLAG (VISITFLAG, line);
  info->line = line;
  longjmp (info->env, 1);
  return 1;
}


/* ---------------------------------------------------------------------------
 * Loops through all thermal fingers on the given layer found on a pin
 * and calls the callback function for it. Returns a count of how many
 * fingers were found.
 *
 * the longjmp loop is a bit ugly, but we need to call a function from outside
 * the search because that function could modify the tree and/or the
 * line pointers.
 */
int
ModifyThermals (LayerTypePtr layer, PinTypePtr Pin,
		void *(*callback) (LayerTypePtr lay, LineTypePtr line),
		void *(*cb2) (LayerTypePtr lay, ArcTypePtr arc))
{
  if (!TEST_FLAG (HOLEFLAG, Pin))
    {
      struct therm_info info;
      BoxType sb;
      /* this variable needs to be static so it's not on the stack
       * otherwise the longjmp loop could reset it.
       */
      static int count;

      count = 0;
      info.line = NULL;
      /* the search box for thermals is the center point of the pin only */
      sb.X1 = sb.X2 = Pin->X;
      sb.Y1 = sb.Y2 = Pin->Y;
      sb.X2 += 1;
      sb.Y2 += 1;
      /* mark all as unvisited */
      if (!r_search (layer->line_tree, &sb, NULL, visit_therm, NULL))
	goto chk_arcs;		/* nothing to see here */
      if (setjmp (info.env) != 1)
	{
	  r_search (layer->line_tree, &sb, NULL, pin_therm, &info);
	  return count;
	}
      else
	{
	  /* Can't call the callback within the r_search because
	   * it is allowed to modify the tree by deleting the line
	   * for example. So we call it after the longjmp.
	   */
	  count++;
	  if (callback)
	    callback (layer, info.line);
	  /* loop finding all thermals */
	  longjmp (info.env, 2);
	}
    chk_arcs:
      info.line = NULL;		/* really used for arcs here */
      /* the search box for arc thermals is the whole cleared area */
      sb.X1 = Pin->X - (Pin->Thickness + Pin->Clearance + 1) / 2;
      sb.X2 = Pin->X + (Pin->Thickness + Pin->Clearance + 1) / 2;
      sb.Y1 = Pin->Y - (Pin->Thickness + Pin->Clearance + 1) / 2;
      sb.Y2 = Pin->Y + (Pin->Thickness + Pin->Clearance + 1) / 2;
      /* mark all as unvisited */
      if (!r_search (layer->arc_tree, &sb, NULL, visit_therm, NULL))
	return 0;		/* nothing to see here */
      if (setjmp (info.env) != 1)
	{
	  r_search (layer->arc_tree, &sb, NULL, pin_therm, &info);
	  return count;
	}
      else
	{
	  /* Can't call the callback within the r_search because
	   * it is allowed to modify the tree by deleting the arc 
	   * for example. So we call it after the longjmp.
	   */
	  count = 1;
	  if (cb2)
	    cb2 (layer, (ArcTypePtr) info.line);
	  /* loop finding all thermals */
	  longjmp (info.env, 2);
	}
    }
  /* no thermals on hole */
  return 0;
}

/* PlaceThermal will put thermal fingers on the active layout.
 * It takes a style argument as to what type of fingers you want.
 * style = 0 means no thermal (i.e. don't connect to the plane)
 * but this routine does not remove thermals, so that is not an
 * appropriate argument to this function.
 * style = 1 means original style dual 45 degree lines.
 * style = 2 means horizontal and vertical fingers.
 * style = 3 means clear 4 arc segments leaving fingers at 45.
 * style = 4 means clear 4 arc segments leaving fingers at hz vt.
 * style = 5 means solid join to plane.
 * More styles may be added in the future.
 */

void
PlaceThermal (LayerTypePtr layer, PinTypePtr Via, int style)
{
  LineTypePtr line;
  BDimension half = (Via->Thickness + Via->Clearance + 1) / 2;

  assert (style != 0);
  switch (style - 1)
    {
    case 0:
      if (!TEST_FLAG (SQUAREFLAG, Via))
	half = (half * M_SQRT1_2 + 1);
      line =
	CreateNewLineOnLayer (layer, Via->X - half, Via->Y - half,
			      Via->X + half, Via->Y + half,
			      (Via->Thickness -
			       Via->DrillingHole) * PCB->ThermScale, 0,
			      MakeFlags (USETHERMALFLAG));
      AddObjectToCreateUndoList (LINE_TYPE, CURRENT, line, line);
      DrawLine (layer, line, 0);
      line =
	CreateNewLineOnLayer (layer, Via->X - half, Via->Y + half,
			      Via->X + half, Via->Y - half,
			      (Via->Thickness -
			       Via->DrillingHole) * PCB->ThermScale, 0,
			      MakeFlags (USETHERMALFLAG));
      AddObjectToCreateUndoList (LINE_TYPE, CURRENT, line, line);
      DrawLine (layer, line, 0);
      break;
    case 1:
      line =
	CreateNewLineOnLayer (layer, Via->X - half, Via->Y,
			      Via->X + half, Via->Y,
			      (Via->Thickness -
			       Via->DrillingHole) * PCB->ThermScale, 0,
			      MakeFlags (USETHERMALFLAG));
      AddObjectToCreateUndoList (LINE_TYPE, layer, line, line);
      DrawLine (layer, line, 0);
      line =
	CreateNewLineOnLayer (layer, Via->X, Via->Y - half,
			      Via->X, Via->Y + half,
			      (Via->Thickness -
			       Via->DrillingHole) * PCB->ThermScale, 0,
			      MakeFlags (USETHERMALFLAG));
      AddObjectToCreateUndoList (LINE_TYPE, layer, line, line);
      DrawLine (layer, line, 0);
      break;
    case 2:
      {
	ArcTypePtr arc;
	RestoreToPolygon (PCB->Data, VIA_TYPE, layer, Via);
	AddObjectToClearPolyUndoList (VIA_TYPE, layer, Via, Via, False);
	arc = CreateNewArcOnLayer (layer, Via->X, Via->Y,
				   Via->Thickness / 2 + Via->Clearance / 4,
				   65, 50, 0, Via->Clearance / 2,
				   MakeFlags (CLEARLINEFLAG |
					      USETHERMALFLAG));
	AddObjectToCreateUndoList (ARC_TYPE, layer, arc, arc);
	arc = CreateNewArcOnLayer (layer, Via->X, Via->Y,
				   Via->Thickness / 2 + Via->Clearance / 4,
				   155, 50, 0, Via->Clearance / 2,
				   MakeFlags (CLEARLINEFLAG |
					      USETHERMALFLAG));
	AddObjectToCreateUndoList (ARC_TYPE, layer, arc, arc);
	arc = CreateNewArcOnLayer (layer, Via->X, Via->Y,
				   Via->Thickness / 2 + Via->Clearance / 4,
				   245, 50, 0, Via->Clearance / 2,
				   MakeFlags (CLEARLINEFLAG |
					      USETHERMALFLAG));
	AddObjectToCreateUndoList (ARC_TYPE, layer, arc, arc);
	arc = CreateNewArcOnLayer (layer, Via->X, Via->Y,
				   Via->Thickness / 2 + Via->Clearance / 4,
				   335, 50, 0, Via->Clearance / 2,
				   MakeFlags (CLEARLINEFLAG |
					      USETHERMALFLAG));
	AddObjectToCreateUndoList (ARC_TYPE, layer, arc, arc);
      }
      break;
    case 3:
      {
	ArcTypePtr arc;
	RestoreToPolygon (PCB->Data, VIA_TYPE, layer, Via);
	AddObjectToClearPolyUndoList (VIA_TYPE, layer, Via, Via, False);
	arc = CreateNewArcOnLayer (layer, Via->X, Via->Y,
				   Via->Thickness / 2 + Via->Clearance / 4,
				   20, 50, 0, Via->Clearance / 2,
				   MakeFlags (CLEARLINEFLAG |
					      USETHERMALFLAG));
	AddObjectToCreateUndoList (ARC_TYPE, layer, arc, arc);
	arc = CreateNewArcOnLayer (layer, Via->X, Via->Y,
				   Via->Thickness / 2 + Via->Clearance / 4,
				   110, 50, 0, Via->Clearance / 2,
				   MakeFlags (CLEARLINEFLAG |
					      USETHERMALFLAG));
	AddObjectToCreateUndoList (ARC_TYPE, layer, arc, arc);
	arc = CreateNewArcOnLayer (layer, Via->X, Via->Y,
				   Via->Thickness / 2 + Via->Clearance / 4,
				   200, 50, 0, Via->Clearance / 2,
				   MakeFlags (CLEARLINEFLAG |
					      USETHERMALFLAG));
	AddObjectToCreateUndoList (ARC_TYPE, layer, arc, arc);
	arc = CreateNewArcOnLayer (layer, Via->X, Via->Y,
				   Via->Thickness / 2 + Via->Clearance / 4,
				   290, 50, 0, Via->Clearance / 2,
				   MakeFlags (CLEARLINEFLAG |
					      USETHERMALFLAG));
	AddObjectToCreateUndoList (ARC_TYPE, layer, arc, arc);
      }
      break;
    default:
      line =
	CreateNewLineOnLayer (layer, Via->X, Via->Y,
			      Via->X, Via->Y, Via->Thickness + Via->Clearance,
			      0, MakeFlags (USETHERMALFLAG));
      AddObjectToCreateUndoList (LINE_TYPE, layer, line, line);
      /* fill in the polygon hole while we're at it */
      /* not necessary, but the screen has glitches otherwise */
      /* it is problematic for undo however */
      RestoreToPolygon (PCB->Data, VIA_TYPE, layer, Via);
      AddObjectToClearPolyUndoList (VIA_TYPE, layer, Via, Via, False);
      DrawLine (layer, line, 0);
    }
}
