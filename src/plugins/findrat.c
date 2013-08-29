/*!
 * \file findrat.c
 *
 * \brief FindRat plug-in for PCB.
 *
 * \author Copyright (C) 2008 DJ Delorie <dj@delorie.com>
 *
 * \copyright Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Compile like this:
 * <pre>
gcc -I$HOME/geda/pcb-cvs/src -I$HOME/geda/pcb-cvs -O2 -shared findrat.c -o findrat.so
 * </pre>
 *
 * The resulting findrat.so goes in $HOME/.pcb/plugins/findrat.so.
 *
 * Usage: FindRat()
 *
 * The cursor scrolls to one of the rats on the board.
 *
 * http://www.delorie.com/pcb/findrat.c
 */

#include <stdio.h>
#include <math.h>

#include "config.h"
#include "global.h"
#include "data.h"
#include "hid.h"
#include "error.h"

static int
findrat (int argc, char **argv, Coord x, Coord y)
{
  RatType *r;

  if (PCB->Data->RatN == 0)
  {
    Message("No Rats");
    return 0;
  }
  r = & (PCB->Data->Rat[0]);
  gui->set_crosshair (r->Point1.X, r->Point1.Y, HID_SC_PAN_VIEWPORT);
  return 0;
}

static HID_Action findrat_action_list[] =
{
  {"FindRat", NULL, findrat, NULL, NULL}
};

REGISTER_ACTIONS (findrat_action_list)

void
pcb_plugin_init ()
{
  register_findrat_action_list ();
}
