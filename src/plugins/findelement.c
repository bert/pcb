/*!
 * \file findelement.c
 *
 * \author Copyright (C) 2009 .. 2011 by Bert Timmerman <bert.timmerman@xs4all.nl>
 * with some usefull hints from DJ Delorie to finish this plug-in.
 *
 * \brief Plug-in for PCB to find the specified element.
 *
 * \copyright Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Function to look up the specified PCB element on the screen.\n
 * \n
 * Compile like this:\n
 * \n
 * gcc -Ipath/to/pcb/src -Ipath/to/pcb -O2 -shared findelement.c -o findelement.so
 * \n\n
 * The resulting findelement.so file should go in $HOME/.pcb/plugins/\n
 * \n
 * \warning Be very strict in compiling this plug-in against the exact pcb
 * sources you compiled/installed the pcb executable (i.e. src/pcb) with.\n
 *
 * Usage: FindElement(Refdes)\n
 * Usage: FE(Refdes)\n
 * If no argument is passed, no action is carried out.\n
 * \n
 * FE is a shortcut for lazy users ;-).
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
#include <math.h>

#include "config.h"
#include "global.h"
#include "data.h"
#include "hid.h"
#include "misc.h"
#include "create.h"
#include "rtree.h"
#include "undo.h"
#include "set.h"
#include "error.h"

/*!
 * \brief Find the specified element.
 *
 * Usage: FindElement(Refdes)\n
 * If no argument is passed, no action is carried out.
 */
static int
find_element (int argc, char **argv, Coord x, Coord y)
{
  if (argc == 0 || strcasecmp (argv[0], "") == 0)
  {
    Message ("WARNING: in FindElement the argument should be a non-empty string value.\n");
      return 0;
  }
  else
  {
    SET_FLAG (NAMEONPCBFLAG, PCB);
    ELEMENT_LOOP(PCB->Data);
    {
      if (NAMEONPCB_NAME(element)
        && strcmp (argv[0], NAMEONPCB_NAME(element)) == 0)
      {
        gui->set_crosshair
        (
          element->MarkX,
          element->MarkY,
          HID_SC_PAN_VIEWPORT
        );
      }
    }
    END_LOOP;
    gui->invalidate_all ();
    IncrementUndoSerialNumber ();
    return 0;
  };
}


static HID_Action findelement_action_list[] =
{
  {"FindElement", NULL, find_element, "Find the specified element", NULL},
  {"FE", NULL, find_element, "Find the specified element", NULL}
};


REGISTER_ACTIONS (findelement_action_list)


void
pcb_plugin_init ()
{
        register_findelement_action_list ();
}

/* EOF */
