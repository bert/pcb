/*!
 * \file findrubble.c
 *
 * \author Copyright (C) 2011 by Bert Timmerman <bert.timmerman@xs4all.nl>
 *
 * \brief Plug-in for PCB to purge traces with the specified minimum length.
 *
 * \copyright Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Function to look up traces with the specified length.\n
 * \n
 * Compile like this:\n
 * \n
 * gcc -Ipath/to/pcb/src -Ipath/to/pcb -O2 -shared find_rubble.c -o find_rubble.so
 * \n\n
 * The resulting find_rubble.so file should go in $HOME/.pcb/plugins/\n
 * \n
 *
 * \warning Be very strict in compiling this plug-in against the exact pcb
 * sources you compiled/installed the pcb executable (i.e. src/pcb) with.\n
 *
 * Usage: FindRubble(Length)\n
 * Usage: FR(Length)\n
 * If no argument is passed, no action is carried out.\n
 * \n
 * FR is a shortcut for lazy users ;-).
 * <hr>
 * This program is free software; you can redistribute it and/or modify\n
 * it under the terms of the GNU General Public License as published by\n
 * the Free Software Foundation; either version 2 of the License, or\n
 * (at your option) any later version.\n
 * \n
 * This program is distributed in the hope that it will be useful,\n
 * but WITHOUT ANY WARRANTY; without even the implied warranty of\n
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.\n
 * \n
 * You should have received a copy of the GNU General Public License\n
 * along with this program; if not, write to:\n
 * the Free Software Foundation, Inc.,\n
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.\n
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "config.h"
#include "global.h"
#include "macro.h"
#include "data.h"
#include "hid.h"
#include "misc.h"
#include "create.h"
#include "rtree.h"
#include "undo.h"
#include "set.h"
#include "error.h"

/*!
 * \brief Find the traces with the specified (minimum) length.
 *
 * Usage: FindRubble(Length)\n
 * If no argument is passed, no action is carried out.
 */
static int
find_rubble (int argc, char **argv, Coord x, Coord y)
{
  double act_length;
  double min_length;

  if (argc == 0 || strcasecmp (argv[0], "") == 0)
  {
    Message ("WARNING: the argument should be a non-empty string value.\n");
    return 0;
  }
  else
  {
    min_length = strtod (argv[0], NULL);
    SET_FLAG (NAMEONPCBFLAG, PCB);
    ALLLINE_LOOP (PCB->Data);
    {
      act_length = sqrt (
        ((line->Point2.X - line->Point1.X)
        * (line->Point2.X - line->Point1.X))
        + ((line->Point2.Y - line->Point1.Y)
        * (line->Point2.Y - line->Point1.Y))
        ); /* Pythagoras */
      if (act_length < min_length)
      {
        gui->set_crosshair
        (
          line->Point1.X,
          line->Point1.Y,
          HID_SC_PAN_VIEWPORT
        );
      }
    }
    ENDALL_LOOP;
    gui->invalidate_all ();
    IncrementUndoSerialNumber ();
  };
  return 0;
}


static HID_Action find_rubble_action_list[] =
{
  {"FindRubble", NULL, find_rubble, NULL, NULL},
  {"FR", NULL, find_rubble, NULL, NULL}
};


REGISTER_ACTIONS (find_rubble_action_list)


void
pcb_plugin_init ()
{
  register_find_rubble_action_list ();
}

/* EOF */
