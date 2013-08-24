/*!
 * \file boardflip.c
 *
 * \brief BoardFlip plug-in for PCB.
 *
 * \author Copyright (C) 2008 DJ Delorie <dj@delorie.com>
 *
 * \copyright Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * http://www.delorie.com/pcb/boardflip.c
 *
 * Compile like this:
 * <pre>
gcc -I$HOME/geda/pcb-cvs/src -I$HOME/geda/pcb-cvs -O2 -shared boardflip.c -o boardflip.so
 * </pre>
 * The resulting boardflip.so goes in $HOME/.pcb/plugins/boardflip.so.
 *
 * Usage: Boardflip()
 *
 * All objects on the board are up-down flipped.
 *
 * Command line board flipping:
 *
 * pcb --action-string "BoardFlip() SaveTo(LayoutAs,$OUTFILE) Quit()" $INFILE
 *
 * To flip the board physically, use BoardFlip(sides)
 *
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

/* Things that need to be flipped:

  lines
  text
  polygons
  arcs
  vias
  elements
    elementlines
    elementarcs
    pins
    pads
  rats
*/

#define FLIP(y) (y) = h - (y)
#define NEG(y) (y) = - (y)

static int
boardflip (int argc, char **argv, Coord x, Coord y)
{
  int h = PCB->MaxHeight;
  int sides = 0;

  if (argc > 0 && strcasecmp (argv[0], "sides") == 0)
    sides = 1;
  printf("argc %d argv %s sides %d\n", argc, argv[0], sides);
  LAYER_LOOP (PCB->Data, max_copper_layer + 2);
  {
    LINE_LOOP (layer);
    {
      FLIP (line->Point1.Y);
      FLIP (line->Point2.Y);
    }
    END_LOOP;
    TEXT_LOOP (layer);
    {
      FLIP (text->Y);
      TOGGLE_FLAG(ONSOLDERFLAG, text);
    }
    END_LOOP;
    POLYGON_LOOP (layer);
    {
      int i, j;
      POLYGONPOINT_LOOP (polygon);
      {
        FLIP (point->Y);
      }
      END_LOOP;
      i = 0;
      j = polygon->PointN - 1;
      while (i < j)
      {
        PointType p = polygon->Points[i];
        polygon->Points[i] = polygon->Points[j];
        polygon->Points[j] = p;
        i++;
        j--;
      }
    }
    END_LOOP;
    ARC_LOOP (layer);
    {
      FLIP (arc->Y);
      NEG (arc->StartAngle);
      NEG (arc->Delta);
    }
    END_LOOP;
  }
  END_LOOP;
  VIA_LOOP(PCB->Data);
  {
    FLIP (via->Y);
  }
  END_LOOP;
  ELEMENT_LOOP (PCB->Data);
  {
    FLIP (element->MarkY);
    if (sides)
      TOGGLE_FLAG (ONSOLDERFLAG, element);
    ELEMENTTEXT_LOOP (element);
    {
      FLIP (text->Y);
      TOGGLE_FLAG (ONSOLDERFLAG, text);
    }
    END_LOOP;
    ELEMENTLINE_LOOP (element);
    {
      FLIP (line->Point1.Y);
      FLIP (line->Point2.Y);
    }
    END_LOOP;
    ELEMENTARC_LOOP (element);
    {
      FLIP (arc->Y);
      NEG (arc->StartAngle);
      NEG (arc->Delta);
    }
    END_LOOP;
    PIN_LOOP (element);
    {
      FLIP (pin->Y);
    }
    END_LOOP;
    PAD_LOOP (element);
    {
      FLIP (pad->Point1.Y);
      FLIP (pad->Point2.Y);
      if (sides)
        TOGGLE_FLAG (ONSOLDERFLAG, pad);
    }
    END_LOOP;
  }
  END_LOOP;
  RAT_LOOP (PCB->Data);
  {
    FLIP (line->Point1.Y);
    FLIP (line->Point2.Y);
  }
  END_LOOP;
  return 0;
}

static HID_Action boardflip_action_list[] =
{
  {"BoardFlip", NULL, boardflip, NULL, NULL}
};

REGISTER_ACTIONS (boardflip_action_list)

void
pcb_plugin_init ()
{
  register_boardflip_action_list ();
}
