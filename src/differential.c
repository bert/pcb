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

#define ARG(n) (argc > (n) ? argv[n] : 0)

static const char makedifferential_syntax[] =
  N_("MakeDifferential(All, width, gap)\n"
  "MakeDifferential(Selected, offset, thickness)\n");

static const char makedifferential_help[] =
  "Splits lines and arcs into differential pair traces.";

/* %start-doc actions MakeDifferential

@emph{All} line or arc segments of traces on all layers with
@emph{width} will be split into parallel lines and arcs, separated by a
distance @emph{gap}.

@emph{Selected} line or arc segments of traces will be split into
parallel lines and arcs with an @emph{offset} and @emph{thickness}.

Warnings:

Back up your work before running.

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
  char *function = NULL;
  int all;
  char *width;
  char *gap;
  char *offsat;
  char *thickness;
  Coord w = 1000000;
  Coord g = 200000;
  Coord t;
  Coord o;
  Coord x1,y1,x2,y2;
  double wx, wy;
  double i, offset;
  double nx, ny;
  LineType *line2;
  ArcType *arc2;

  if (argc < 3)
  {
    Message (N_("Usage:\n"
                "MakeDifferential(All, width, gap)\n"
                "MakeDifferential(Selected, offset, thickness)\n"));
    fprintf (stderr, N_("Usage:\n"
                        "MakeDifferential(All, width, gap)\n"
                        "MakeDifferential(Selected, offset, thickness)\n"));
    return 1;
  }

  function = ARG(0);

  if (!function)
  {
    all = 1;
  }
  else if (strcmp (function, "All") == 0)
  {
    all = 1;
  }
  else if (strcmp (function, "Selected") == 0)
  {
    all = 0;
  }
  else
  {
    Message (N_("Usage:\n"
                "MakeDifferential(All, width, gap)\n"
                "MakeDifferential(Selected, offset, thickness)\n"));
    fprintf (stderr, N_("Usage:\n"
                        "MakeDifferential(All, width, gap)\n"
                        "MakeDifferential(Selected, offset, thickness)\n"));
    return 1;
  }

  if (all) /* "All" */
  {
    width = argv[1];
    gap = argv[2];

    if (width && gap)
      {
        w = GetValue (width, NULL, NULL);
        g = GetValue (gap, NULL, NULL);
      }

    t = abs ((w - g) / 2); /* Avoid a negative value. */
    offset = (double) ((g + w) >> 2);

    ALLLINE_LOOP (PCB->Data);
    {
      /* Do not modify locked lines and only process lines with the
       * specified thickness. */
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
      /* Do not modify locked arcs and only process arcs with the
       * specified thickness. */
      if (TEST_FLAG (LOCKFLAG, arc) || arc->Thickness != w)
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

    Message (N_("Created Differential traces: width = %d, gap = %d, thickness = %d\n"),
    w, g, t);

  }
  
  else /* "Selected". */
  {
    offsat = argv[1];
    thickness = argv[2];

    if (offsat && thickness)
      {
        o = GetValue (offsat, NULL, NULL);
        t = GetValue (thickness, NULL, NULL);
      }

    offset = (double)(o);

    ALLLINE_LOOP (PCB->Data);
    {
      /* Only process selected lines. */
      if (!TEST_FLAG (SELECTEDFLAG, line))
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
      /* Only process selected arcs. */
      if (!TEST_FLAG (SELECTEDFLAG, arc))
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

    Message (N_("Created Differential traces: offset = %d, thickness = %d\n"),
    o, t);

  } /* */

  /* Done with our action, so increment the undo # .*/
  IncrementUndoSerialNumber ();

  gui->invalidate_all ();

  return 0;
}

HID_Action differential_action_list[] =
{
  {"MakeDifferential", 0, ActionMakeDifferential,
   makedifferential_help, makedifferential_syntax},

  /* Shortcut for lazy users. */
  {"MD", 0, ActionMakeDifferential,
   makedifferential_help, makedifferential_syntax}
};

REGISTER_ACTIONS (differential_action_list)

/* EOF */
