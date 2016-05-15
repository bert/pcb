/*!
 * \file src/differential.c
 *
 * \brief Create differential pairs.
 *
 * \author Stephen Ecob <silicon.on.inspiration@gmail.com>
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996, 2004 Thomas Nau
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"

#include <memory.h>
#include <limits.h>


#include "data.h"
#include "create.h"
#include "remove.h"
#include "move.h"
#include "draw.h"
#include "undo.h"
#include "rtree.h"
#include "set.h"
#include "polygon.h"
#include "misc.h"
#include "error.h"
#include "strflags.h"
#include "find.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

static const char makedifferential_syntax[] =
  "MakeDifferential(width,gap)";

static const char makedifferential_help[] = "Splits lines and arcs into differential pairs.";

/* %start-doc actions MakeDifferential

All line or arc segments of traces with @emph{width} will be split into
paralel lines and arcs, separated by a distance @emph{gap}.

Warnings:

Back up your work before running.

Doesn't support undo properly.

Doesn't work well with straight line intersections, it creates gaps
and overhangs at the intersections.

The workaround is to use arcs for changing direction, as it handles line
to arc intersections beautifully.

%end-doc */
/*!
 * \brief Split line or arc trace segments into differential pairs.
 */
static int
ActionMakeDifferential (int argc, char **argv, Coord x, Coord y)
{
  char *width;
  char *gap;
  Coord w = 1000000;
  Coord g = 200000;
  Coord t;
  Coord x1,y1,x2,y2;
  double wx, wy;
  double i, offset;
  double nx, ny;
  LineType *line2;
  ArcType *arc2;

  if (argc < 2)
  {
    Message("Usage: MakeDifferential(width,gap)");
    return 1;
  }

  width = argv[0];
  gap = argv[1];

  if (width && gap)
    {
      w = GetValue (width, NULL, NULL);
      g = GetValue (gap, NULL, NULL);
    }
  t = (w - g) / 2;
  offset = (double)((g+w)>>2);
  /* printf ("w=%d, g=%d, t=%d\n", w, g, t); */

  ALLLINE_LOOP (PCB->Data);
  {
    if (TEST_FLAG (LOCKFLAG, line) || line->Thickness != w)
      continue;

    AddObjectToSizeUndoList (LINE_TYPE, layer, line, line);
    EraseLine (line);
    r_delete_entry (layer->line_tree, (BoxType *) line);
    RestoreToPolygon (PCB->Data, LINE_TYPE, layer, line);
    line->Thickness = t;
    wx = (double) (line->Point2.X - line->Point1.X);
    wy = (double) (line->Point2.Y - line->Point1.Y);
    i = 1.0 / sqrt (wx * wx + wy * wy);
    nx = -wy * i * offset;
    ny = wx * i * offset;
    x1 = line->Point1.X - (Coord) nx;
    y1 = line->Point1.Y - (Coord) ny;
    x2 = line->Point2.X - (Coord) nx;
    y2 = line->Point2.Y - (Coord) ny;
    line->Point1.X += (Coord) nx;
    line->Point1.Y += (Coord) ny;
    line->Point2.X += (Coord) nx;
    line->Point2.Y += (Coord) ny;

    SetLineBoundingBox (line);
    r_insert_entry (layer->line_tree, (BoxType *) line, 0);
    ClearFromPolygon (PCB->Data, LINE_TYPE, layer, line);
    DrawLine (layer, line);

    if ((line2 = CreateDrawnLineOnLayer (layer,
                                         x1, y1,
                                         x2, y2,
                                         t,
                                         line->Clearance,
                                         line->Flags)))
    {
      AddObjectToCreateUndoList (LINE_TYPE, layer, line2, line2);
      DrawLine (layer, line2);
      ClearFromPolygon (PCB->Data, LINE_TYPE, layer, line2);
    }
  }
  ENDALL_LOOP;

  ALLARC_LOOP (PCB->Data);
  {
    if (arc->Thickness != w)
      continue;

    AddObjectToSizeUndoList (ARC_TYPE, layer, arc, arc);
    EraseArc (arc);
    r_delete_entry (layer->arc_tree, (BoxType *) arc);
    RestoreToPolygon (PCB->Data, ARC_TYPE, layer, arc);

    arc->Thickness = t;
    arc->Width += (Coord) offset;
    arc->Height += (Coord) offset;

    SetArcBoundingBox (arc);
    r_insert_entry (layer->arc_tree, (BoxType *) arc, 0);
    ClearFromPolygon (PCB->Data, ARC_TYPE, layer, arc);
    DrawArc (layer, arc);

    if ((arc2 = CreateNewArcOnLayer (layer,
                                     arc->X, arc->Y,
                                     arc->Width - 2 * (Coord)offset,
                                     arc->Height - 2 * (Coord)offset,
                                     arc->StartAngle,
                                     arc->Delta,
                                     t,
                                     arc->Clearance,
                                     arc->Flags)))
    {
      AddObjectToCreateUndoList (ARC_TYPE, layer, arc2, arc2);
      DrawArc (layer, arc2);
      ClearFromPolygon (PCB->Data, ARC_TYPE, layer, arc2);
    }
  }
  ENDALL_LOOP;

  return 0;
}

HID_Action differential_action_list[] =
{
  {"MakeDifferential", 0, ActionMakeDifferential,
   makedifferential_help, makedifferential_syntax}
};

REGISTER_ACTIONS (differential_action_list)

/* EOF */
