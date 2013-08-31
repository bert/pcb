/*!
 * \file join-found.c
 *
 * \brief Join found plug-in for PCB.
 *
 * \author Copyright (C) 2009 Peter Clifton <pcjc2@cam.ac.uk>
 *
 * \copyright Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Compile like this:
 * <pre>
gcc -Wall -I$HOME/pcbsrc/pcb.clean/src -I$HOME/pcbsrc/pcb.clean -O2 -shared join-found.c -o join-found.so
 * </pre>
 *  The resulting join-found.so goes in $HOME/.pcb/plugins/join-found.so
 */

#include <stdio.h>
#include <math.h>

#include "config.h"
#include "global.h"
#include "data.h"
#include "change.h"
#include "undo.h"

#define THERMAL_STYLE 4

static int changed = 0;


/* FIXME: PCB API STOPS ME DOING THIS */
#if 0
static void *
init_line (LayerTypePtr Layer, LineTypePtr Line)
{
  ListStart (LINE_TYPE, NULL, Line, NULL);
  return Line;
}

static void *
init_via (PinTypePtr Via)
{
  ListStart (VIA_TYPE, NULL, Via, NULL);
  return Via;
}

static void *
init_pin (ElementTypePtr Element, PinTypePtr Pin)
{
  ListStart (PIN_TYPE, Element, Pin, NULL);
  return Pin;
}

static void *
init_pad (ElementTypePtr Element, PadTypePtr Pad)
{
  ListStart (PAD_TYPE, Element, Pad, NULL);
  return Pad;
}

static void *
init_arc (LayerTypePtr Layer, ArcTypePtr Arc)
{
  ListStart (ARC_TYPE, Layer, Arc, NULL);
  return Arc;
}

static ObjectFunctionType init_funcs = {
  init_line,
  NULL,
  NULL,
  init_via,
  NULL,
  NULL,
  init_pin,
  init_pad,
  NULL,
  NULL,
  init_arc,
  NULL
};
#endif

static int
joinfound (int argc, char **argv, Coord x, Coord y)
{
  int TheFlag = FOUNDFLAG;

  changed = 0;
  /* FIXME: PCB API STOPS ME DOING THIS */
#if 0
  TheFlag = FOUNDFLAG; // | DRCFLAG;
  SaveFindFlag (TheFlag);
  InitConnectionLookup ();
  ResetConnections (False);
  SelectedOperation (init_funcs, False, LINE_TYPE | ARC_TYPE | PIN_TYPE | VIA_TYPE | PAD_TYPE);
  DoIt (true, False);
#endif
  VISIBLELINE_LOOP (PCB->Data);
  {
    if (TEST_FLAG (TheFlag, line))
    {
      ChangeObjectJoin (LINE_TYPE, layer, line, line);
      changed = true;
    }
  }
  ENDALL_LOOP;
  VISIBLEARC_LOOP (PCB->Data);
  {
    if (TEST_FLAG (TheFlag, arc))
    {
      ChangeObjectJoin (ARC_TYPE, layer, arc, arc);
      changed = true;
    }
  }
  ENDALL_LOOP;
  if (PCB->PinOn)
    ELEMENT_LOOP (PCB->Data);
  {
    PIN_LOOP (element);
    {
      if (TEST_FLAG (TheFlag, pin))
      {
        ChangeObjectThermal (PIN_TYPE, element, pin, pin, THERMAL_STYLE);
        changed = true;
      }
    }
    END_LOOP; /* PIN_LOOP */
  }
  END_LOOP; /* ELEMENT_LOOP */
  if (PCB->ViaOn)
    VIA_LOOP (PCB->Data);
  {
    if (TEST_FLAG (TheFlag, via))
    {
      ChangeObjectThermal (VIA_TYPE, via, via, via, THERMAL_STYLE);
      changed = true;
    }
  }
  END_LOOP;
  /* FIXME: PCB API STOPS ME DOING THIS */
#if 0
  ResetConnections (False);
  FreeConnectionLookupMemory ();
  RestoreFindFlag ();
#endif
  gui->invalidate_all ();
  if (changed)
    IncrementUndoSerialNumber ();
  return 0;
}

static HID_Action joinfound_action_list[] =
{
  {"JoinFound", NULL, joinfound, NULL, NULL}
};

REGISTER_ACTIONS (joinfound_action_list)

void
pcb_plugin_init ()
{
  register_joinfound_action_list ();
}
