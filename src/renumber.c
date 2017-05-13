/*!
 * \file renumber.c
 *
 * \brief Renumber refdesses on pcb or in the buffer.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2006 DJ Delorie <dj@delorie.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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


/* %start-doc actions RenumberBlock

The @code{RenumberBlocks()} action renumbers all selected refdesses on
the pcb.

Usage: RenumberBlock(oldnum,newnum)

All selected elements are renumbered by adding (newnum-oldnum) to
the existing number.

To invoke it, use the command window, usually by typing ":".

Example:

@code{RenumberBlock(100,200)} will change R213 to R313.

%end-doc */
static int
renumber_block (int argc, char **argv, Coord x, Coord y)
{
  char num_buf[15];
  int old_base;
  int new_base;

  if (argc < 2)
  {
    Message("Usage: RenumberBlock oldnum newnum");
    return 1;
  }

  old_base = atoi (argv[0]);
  new_base = atoi (argv[1]);

  SET_FLAG (NAMEONPCBFLAG, PCB);

  ELEMENT_LOOP (PCB->Data);
  {
    char *refdes_split;
    char *cp;
    char *old_ref;
    char *new_ref;
    int num;

    if (!TEST_FLAG (SELECTEDFLAG, element)
        || EMPTY_STRING_P(element->Name[1].TextString))
      continue;

    old_ref = element->Name[1].TextString;
    for (refdes_split = cp = old_ref; *cp; cp++)
      if (!isdigit (*cp))
        refdes_split = cp+1;

    num = atoi (refdes_split);
    num += (new_base - old_base);
    sprintf(num_buf, "%d" ,num);
    new_ref = (char *) malloc (refdes_split - old_ref + strlen (num_buf) + 1);
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


/* %start-doc actions RenumberBuffer

The @code{RenumberBuffer()} action renumbers all selected refdesses in
the paste buffer.

Usage: RenumberBuffer(oldnum,newnum)

All selected elements are renumbered by adding (newnum-oldnum) to
the existing number.

To invoke it, use the command window, usually by typing ":".

Example:

@code{RenumberBuffer(0,10)} will change R2 to R12.

%end-doc */
static int
renumber_buffer (int argc, char **argv, Coord x, Coord y)
{
  char num_buf[15];
  int old_base;
  int new_base;

  if (argc < 2)
  {
    Message("Usage: RenumberBuffer oldnum newnum");
    return 1;
  }

  old_base = atoi (argv[0]);
  new_base = atoi (argv[1]);

  SET_FLAG (NAMEONPCBFLAG, PCB);

  ELEMENT_LOOP (PASTEBUFFER->Data);
  {
    char *refdes_split;
    char *cp;
    char *old_ref;
    char *new_ref;
    int num;

    if (EMPTY_STRING_P(element->Name[1].TextString))
      continue;

    old_ref = element->Name[1].TextString;
    for (refdes_split=cp=old_ref; *cp; cp++)
      if (!isdigit(*cp))
        refdes_split = cp+1;

    num = atoi (refdes_split);
    num += (new_base - old_base);
    sprintf (num_buf, "%d" ,num);
    new_ref = (char *) malloc (refdes_split - old_ref + strlen (num_buf) + 1);
    memcpy (new_ref, old_ref, refdes_split - old_ref);
    strcpy (new_ref + (refdes_split - old_ref), num_buf);

    ChangeObjectName (ELEMENT_TYPE, element, NULL, NULL, new_ref);
  }
  END_LOOP;
  return 0;
}

static HID_Action renumber_block_action_list[] =
{
  {"RenumberBlock", NULL, renumber_block, NULL, NULL},
  {"RenumberBuffer", NULL, renumber_buffer, NULL, NULL}
};

REGISTER_ACTIONS (renumber_block_action_list)

void
hid_renumber_init()
{
  register_renumber_block_action_list();
}
