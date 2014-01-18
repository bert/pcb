/*!
 * \file renumberblock.c
 *
 * \brief RenumberBlock plug-in for PCB.
 *
 * \author Copyright (C) 2006 DJ Delorie <dj@delorie.com>
 *
 * \copyright Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * http://www.delorie.com/pcb/renumberblock.c
 *
 * Compile like this:
 *
 * gcc -I$HOME/geda/pcb-cvs/src -I$HOME/geda/pcb-cvs -O2 -shared renumberblock.c -o renumberblock.so
 *
 * The resulting renumberblock.so goes in
 * $HOME/.pcb/plugins/renumberblock.so.
 *
 * Usage: RenumberBlock(oldnum,newnum)
 *
 * All selected elements are renumbered by adding (newnum-oldnum) to
 * the existing number.  I.e. RenumberBlock(100,200) will change R213
 * to R313.
 *
 * Usage: RenumberBuffer(oldnum,newnum)
 *
 * Same, but the paste buffer is renumbered.
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
#include "error.h"
#include "change.h"

static int
renumber_block (int argc, char **argv, Coord x, Coord y)
{
  char num_buf[15];
  int old_base, new_base;

  if (argc < 2) {
    Message("Usage: RenumberBlock oldnum newnum");
    return 1;
  }

  old_base = atoi (argv[0]);
  new_base = atoi (argv[1]);

  SET_FLAG (NAMEONPCBFLAG, PCB);

  ELEMENT_LOOP (PCB->Data);
  {
    char *refdes_split, *cp;
    char *old_ref, *new_ref;
    int num;

    if (!TEST_FLAG (SELECTEDFLAG, element))
      continue;

    old_ref = element->Name[1].TextString;
    for (refdes_split=cp=old_ref; *cp; cp++)
      if (!isdigit(*cp))
        refdes_split = cp+1;

    num = atoi (refdes_split);
    num += (new_base - old_base);
    sprintf(num_buf, "%d" ,num);
    new_ref = (char *) malloc (refdes_split - old_ref + strlen(num_buf) + 1);
    memcpy (new_ref, old_ref, refdes_split - old_ref);
    strcpy (new_ref + (refdes_split - old_ref), num_buf);

    AddObjectToChangeNameUndoList (ELEMENT_TYPE, NULL, NULL,
                                   element,
                                   NAMEONPCB_NAME (element));

    ChangeObjectName (ELEMENT_TYPE, element, NULL, NULL, new_ref);
  }
  END_LOOP;
  IncrementUndoSerialNumber ();
  return 0;
}

static int
renumber_buffer (int argc, char **argv, Coord x, Coord y)
{
  char num_buf[15];
  int old_base, new_base;

  if (argc < 2) {
    Message("Usage: RenumberBuffer oldnum newnum");
    return 1;
  }

  old_base = atoi (argv[0]);
  new_base = atoi (argv[1]);

  SET_FLAG (NAMEONPCBFLAG, PCB);

  ELEMENT_LOOP (PASTEBUFFER->Data);
  {
    char *refdes_split, *cp;
    char *old_ref, *new_ref;
    int num;

    old_ref = element->Name[1].TextString;
    for (refdes_split=cp=old_ref; *cp; cp++)
      if (!isdigit(*cp))
        refdes_split = cp+1;

    num = atoi (refdes_split);
    num += (new_base - old_base);
    sprintf(num_buf, "%d" ,num);
    new_ref = (char *) malloc (refdes_split - old_ref + strlen(num_buf) + 1);
    memcpy (new_ref, old_ref, refdes_split - old_ref);
    strcpy (new_ref + (refdes_split - old_ref), num_buf);

    ChangeObjectName (ELEMENT_TYPE, element, NULL, NULL, new_ref);
  }
  END_LOOP;
  return 0;
}

static HID_Action renumber_block_action_list[] = {
  {"RenumberBlock", NULL, renumber_block,
   NULL, NULL},
  {"RenumberBuffer", NULL, renumber_buffer,
   NULL, NULL}
};

REGISTER_ACTIONS (renumber_block_action_list)

void
pcb_plugin_init()
{
  register_renumber_block_action_list();
}
