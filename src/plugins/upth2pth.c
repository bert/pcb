/*!
 * \file upth2pth.c
 *
 * \author Copyright (C) 2009 .. 2011 by Bert Timmerman <bert.timmerman@xs4all.nl>
 *
 * \brief A plug-in for pcb to change UnPlated Through Holes to Plated
 * Through Holes or vice versa.
 *
 * Function to change all (or a selection) of the unplated holes into plated holes.\n
 * \n
 * Compile like this:\n
 * \n
 * gcc -Ipath/to/pcb/src -Ipath/to/pcb -O2 -shared upth2pth.c -o upth2pth.so
 * \n\n
 * The resulting upth2pth.so file should go in $HOME/.pcb/plugins/\n
 * \n
 * \warning Be very strict with compiling this plug-in against the exact pcb
 * sources you compiled/installed the pcb executable (i.e. src/pcb) with.\n
 *
 * Usage: Upth2pth([Selected|All])\n
 * Usage: Pth2upth([Selected|All])\n
 * \n
 * If no argument is passed, no changes are carried out.\n
 * Locked holes are not to be changed.\n
 * \n
 *
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
 * \brief Changing all or selected unplated holes to plated holes.
 *
 * Usage: Upth2pth([Selected|All])\n
 * If no argument is passed, no changes are carried out.\n
 * Locked holes are not to be changed.\n
 */
static int
upth2pth (int argc, char **argv, Coord x, Coord y)
{
  int selected = 0;
  int all = 0;
  if (argc > 0 && strcasecmp (argv[0], "Selected") == 0)
    selected = 1;
  else if (argc >0 && strcasecmp (argv[0], "All") == 0)
    all = 1;
  else
  {
    Message ("ERROR: in Upth2pth the argument should be either Selected or All.\n");
    return 1;
  }
  SET_FLAG (NAMEONPCBFLAG, PCB);
  VIA_LOOP(PCB->Data);
  {
    if (!TEST_FLAG (LOCKFLAG, via))
    {
      /* via is not locked */
      if (all)
      {
        CLEAR_FLAG(HOLEFLAG, via);
      }
      else if (selected)
      {
        if (TEST_FLAG (SELECTEDFLAG, via))
        {
          CLEAR_FLAG(HOLEFLAG, via);
        }
      }
    }
  }
  END_LOOP; /* VIA_LOOP */
  ELEMENT_LOOP(PCB->Data);
  {
    if (!TEST_FLAG (LOCKFLAG, element))
    {
      /* element is not locked */
      if (all)
      {
        PIN_LOOP(element);
        {
          CLEAR_FLAG(HOLEFLAG, pin);
        }
        END_LOOP; /* PIN_LOOP */
      }
    }
  }
  END_LOOP; /* ELEMENT_LOOP */
  gui->invalidate_all ();
  IncrementUndoSerialNumber ();
  return 0;
}


/*!
 * \brief Changing all or selected plated holes to unplated holes.
 *
 * Usage: Pth2upth([Selected|All])\n
 * If no argument is passed, no changes are carried out.\n
 * Locked holes are not to be changed.\n
 */
static int
pth2upth (int argc, char **argv, Coord x, Coord y)
{
  int selected = 0;
  int all = 0;
  if (argc > 0 && strcasecmp (argv[0], "Selected") == 0)
    selected = 1;
  else if (argc >0 && strcasecmp (argv[0], "All") == 0)
    all = 1;
  else
  {
    Message ("ERROR: in Upth2pth the argument should be either Selected or All.\n");
    return 1;
  }
  SET_FLAG (NAMEONPCBFLAG, PCB);
  VIA_LOOP(PCB->Data);
  {
    if (!TEST_FLAG (LOCKFLAG, via))
    {
      /* via is not locked */
      if (all)
      {
        SET_FLAG(HOLEFLAG, via);
      }
      else if (selected)
      {
        if (TEST_FLAG (SELECTEDFLAG, via))
        {
          SET_FLAG(HOLEFLAG, via);
        }
      }
    }
  }
  END_LOOP; /* VIA_LOOP */
  ELEMENT_LOOP(PCB->Data);
  {
    if (!TEST_FLAG (LOCKFLAG, element))
    {
      /* element is not locked */
      if (all)
      {
        PIN_LOOP(element);
        {
          SET_FLAG(HOLEFLAG, pin);
        }
        END_LOOP; /* PIN_LOOP */
      }
    }
  }
  END_LOOP; /* ELEMENT_LOOP */
  gui->invalidate_all ();
  IncrementUndoSerialNumber ();
  return 0;
}

static HID_Action upth2pth_action_list[] =
{
  {"Upth2pth", NULL, upth2pth, "Change selected or all unplated holes to plated holes", NULL},
  {"Pth2upth", NULL, pth2upth, "Change selected or all plated holes to unplated holes", NULL}
};

REGISTER_ACTIONS (upth2pth_action_list)

void
pcb_plugin_init ()
{
  register_upth2pth_action_list ();
}

/* EOF */
