/*!
 * \file autocrop.c
 *
 * \brief Autocrop plug-in for PCB.
 * Reduce the board dimensions to just enclose the elements.
 *
 * \author Copyright (C) 2007 Ben Jackson <ben@ben.com> based on teardrops.c by
 * Copyright (C) 2006 DJ Delorie <dj@delorie.com>
 *
 * \copyright Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * From: Ben Jackson <bjj@saturn.home.ben.com>
 * To: geda-user@moria.seul.org
 * Date: Mon, 19 Feb 2007 13:30:58 -0800
 * Subject: Autocrop() plugin for PCB
 *
 * As a "learn PCB internals" project I've written an Autocrop() plugin.
 * It finds the extents of your existing board and resizes the board to
 * contain only your parts plus a margin.
 *
 * There are some issues that I can't resolve from a plugin:
 *
 * <ol>
 * <li> Board size has no Undo function, so while Undo will put your objects
 * back where they started, the board size has to be replaced manually.
 * <li> There is no 'edge clearance' DRC paramater, so I used 5*line spacing.
 * <li> Moving a layout with lots of objects and polygons is slow due to the
 * repeated clearing/unclearing of the polygons as things move.  Undo is
 * slower than moving because every individual move is drawn (instead of
 * one redraw at the end).
 * </ol>
 *
 * The source is: http://ad7gd.net/geda/autocrop.c
 *
 * And you compile/install with:
 * <pre>
# set PCB to your PCB source directory
PCB=$(HOME)/cvs/pcb
gcc -I$(PCB) -I$(PCB)/src -O2 -shared autocrop.c -o autocrop.so
cp autocrop.so ~/.pcb/plugins
 * </pre>
 * Run it by typing `:Autocrop()' in the gui, or by binding Autocrop() to a key.
 *
 * --
 * Ben Jackson AD7GD
 * <ben@ben.com>
 * http://www.ben.com/
 */

#include <stdio.h>
#include <math.h>

#include "config.h"
#include "global.h"
#include "data.h"
#include "hid.h"
#include "misc.h"
#include "create.h"
#include "rtree.h"
#include "undo.h"
#include "move.h"
#include "draw.h"
#include "set.h"
#include "polygon.h"

static void *
MyMoveViaLowLevel (DataType *Data, PinType *Via, Coord dx, Coord dy)
{
  if (Data)
  {
    RestoreToPolygon (Data, VIA_TYPE, Via, Via);
    r_delete_entry (Data->via_tree, (BoxType *) Via);
  }
  MOVE_VIA_LOWLEVEL (Via, dx, dy);
  if (Data)
  {
    r_insert_entry (Data->via_tree, (BoxType *) Via, 0);
    ClearFromPolygon (Data, VIA_TYPE, Via, Via);
  }
  return Via;
}

static void *
MyMoveLineLowLevel (DataType *Data, LayerType *Layer, LineType *Line, Coord dx, Coord dy)
{
  if (Data)
  {
    RestoreToPolygon (Data, LINE_TYPE, Layer, Line);
    r_delete_entry (Layer->line_tree, (BoxType *) Line);
  }
  MOVE_LINE_LOWLEVEL (Line, dx, dy);
  if (Data)
  {
    r_insert_entry (Layer->line_tree, (BoxType *) Line, 0);
    ClearFromPolygon (Data, LINE_TYPE, Layer, Line);
  }
  return Line;
}

static void *
MyMoveArcLowLevel (DataType *Data, LayerType *Layer, ArcType *Arc, Coord dx, Coord dy)
{
  if (Data)
  {
    RestoreToPolygon (Data, ARC_TYPE, Layer, Arc);
    r_delete_entry(Layer->arc_tree, (BoxType *) Arc);
  }
  MOVE_ARC_LOWLEVEL (Arc, dx, dy);
  if (Data)
  {
    r_insert_entry (Layer->arc_tree, (BoxType *) Arc, 0);
    ClearFromPolygon (Data, ARC_TYPE, Layer, Arc);
  }
  return Arc;
}

static void *
MyMovePolygonLowLevel (DataType *Data, LayerType *Layer, PolygonType *Polygon, Coord dx, Coord dy)
{
  if (Data)
  {
    r_delete_entry (Layer->polygon_tree, (BoxType *) Polygon);
  }
  /* move.c actually only moves points, note no Data/Layer args */
  MovePolygonLowLevel (Polygon, dx, dy);
  if (Data)
  {
    r_insert_entry (Layer->polygon_tree, (BoxType *) Polygon, 0);
    InitClip(Data, Layer, Polygon);
  }
  return Polygon;
}

static void *
MyMoveTextLowLevel (LayerType *Layer, TextType *Text, Coord dx, Coord dy)
{
  if (Layer)
    r_delete_entry (Layer->text_tree, (BoxType *) Text);
  MOVE_TEXT_LOWLEVEL (Text, dx, dy);
  if (Layer)
    r_insert_entry (Layer->text_tree, (BoxType *) Text, 0);
  return Text;
}

/*!
 * \brief Move everything.
 *
 * Call our own 'MyMove*LowLevel' where they don't exist in move.c.
 * This gets very slow if there are large polygons present, since every
 * element move re-clears the poly, followed by the polys moving and
 * re-clearing everything again.
 */
static void
MoveAll(Coord dx, Coord dy)
{
  ELEMENT_LOOP (PCB->Data);
  {
    MoveElementLowLevel (PCB->Data, element, dx, dy);
    AddObjectToMoveUndoList (ELEMENT_TYPE, NULL, NULL, element, dx, dy);
  }
  END_LOOP;
  VIA_LOOP (PCB->Data);
  {
    MyMoveViaLowLevel (PCB->Data, via, dx, dy);
    AddObjectToMoveUndoList (VIA_TYPE, NULL, NULL, via, dx, dy);
  }
  END_LOOP;
  ALLLINE_LOOP (PCB->Data);
  {
    MyMoveLineLowLevel (PCB->Data, layer, line, dx, dy);
    AddObjectToMoveUndoList (LINE_TYPE, NULL, NULL, line, dx, dy);
  }
  ENDALL_LOOP;
  ALLARC_LOOP (PCB->Data);
  {
    MyMoveArcLowLevel (PCB->Data, layer, arc, dx, dy);
    AddObjectToMoveUndoList (ARC_TYPE, NULL, NULL, arc, dx, dy);
  }
  ENDALL_LOOP;
  ALLTEXT_LOOP (PCB->Data);
  {
    MyMoveTextLowLevel (layer, text, dx, dy);
    AddObjectToMoveUndoList (TEXT_TYPE, NULL, NULL, text, dx, dy);
  }
  ENDALL_LOOP;
  ALLPOLYGON_LOOP (PCB->Data);
  {
    /*
     * XXX MovePolygonLowLevel does not mean "no gui" like
     * XXX MoveElementLowLevel, it doesn't even handle layer
     * XXX tree activity.
     */
    MyMovePolygonLowLevel (PCB->Data, layer, polygon, dx, dy);
    AddObjectToMoveUndoList (POLYGON_TYPE, NULL, NULL, polygon, dx, dy);
  }
  ENDALL_LOOP;
}

static int
autocrop (int argc, char **argv, Coord x, Coord y)
{
//  int changed = 0;
  Coord dx, dy, pad;
  BoxType *box;

  box = GetDataBoundingBox (PCB->Data); /* handy! */
  if (!box || (box->X1 == box->X2 || box->Y1 == box->Y2))
  {
    /* board would become degenerate */
    return 0;
  }
  /*
   * Now X1/Y1 are the distance to move the left/top edge
   * (actually moving all components to the left/up) such that
   * the exact edge of the leftmost/topmost component would touch
   * the edge.  Reduce the move by the edge relief requirement XXX
   * and expand the board by the same amount.
   */
  pad = PCB->minWid * 5; /* XXX real edge clearance */
  dx = -box->X1 + pad;
  dy = -box->Y1 + pad;
  box->X2 += pad;
  box->Y2 += pad;
  /*
   * Round move to keep components grid-aligned, then translate the
   * upper coordinates into the new space.
   */
  dx -= dx % (long) PCB->Grid;
  dy -= dy % (long) PCB->Grid;
  box->X2 += dx;
  box->Y2 += dy;
  /*
   * Avoid touching any data if there's nothing to do.
   */
  if (dx == 0 && dy == 0 && PCB->MaxWidth == box->X2
    && PCB->MaxHeight == box->Y2)
  {
    return 0;
  }
  /* Resize -- XXX cannot be undone */
  PCB->MaxWidth = box->X2;
  PCB->MaxHeight = box->Y2;
  MoveAll (dx, dy);
  IncrementUndoSerialNumber ();
  Redraw ();
  SetChangedFlag (1);
  return 0;
}

static HID_Action autocrop_action_list[] =
{
  {"autocrop", NULL, autocrop, NULL, NULL}
};

REGISTER_ACTIONS (autocrop_action_list)

void
hid_autocrop_init ()
{
  register_autocrop_action_list ();
}
