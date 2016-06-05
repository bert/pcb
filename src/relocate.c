/*!
 * \file src/relocate.c
 *
 * \brief Relocate plug-in for PCB.
 *
 * \author Jean Richard <jean@geemoo.ca>
 *
 * Licensed under the terms of the GNU General Public License, version
 * 2 or later.
 */


#include <stdio.h>

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


/* %start-doc actions Relocate

The @code{Relocate()} action in pcb that will take all selected elements
and stack them up at the current crosshairs.
This works well when starting a new layout and you have hundreds of
parts on your workspace and you're just starting to do your component
placement.
Use Select Element by Name (refdes) to select the elements you want to
move, and then use :Relocate() to set the position of those elements to
where ever your crosshair is.

TODO: Right now, if multiple elements are selected, they are all are
stacked up on top of each other.
Instead make them spread out the way Disperse Elements does.


%end-doc */
static int
relocate (int argc, char **argv, Coord x, Coord y)
{
  Coord origin_x;
  Coord origin_y;
  Coord dx;
  Coord dy;

  /* get crosshair location and make this our "origin". */
  origin_x = Crosshair.X;
  origin_y = Crosshair.Y;

  /* loop over all elements, and move selected elements to crosshair
   * location. */
  ELEMENT_LOOP (PCB->Data);
  {
    /* only affect selected elements. */
      if (TEST_FLAG (SELECTEDFLAG, element))
      {
        /* how far does the element need to move? */
        dx = origin_x - element->BoundingBox.X1;
        dy = origin_y - element->BoundingBox.Y1;

        /* snap to grid. */
        dx -= (element->MarkX + dx) % PCB->Grid;
        dx += PCB->Grid;
        dy -= (element->MarkY + dy) % PCB->Grid;
        dy += PCB->Grid;

        /* move the element. */
        MoveElementLowLevel (PCB->Data, element, dx, dy);

        /* add to the undo list. */
        AddObjectToMoveUndoList (ELEMENT_TYPE, NULL, NULL, element, dx, dy);
      }
  }
  END_LOOP;

  /* done with our action, so increment the undo # .*/
  IncrementUndoSerialNumber ();

  Redraw ();
  SetChangedFlag (true);

  return 0;
}


static HID_Action relocate_action_list[] =
{
  { "Relocate", NULL, relocate, NULL, NULL}
};

REGISTER_ACTIONS(relocate_action_list)

void hid_relocate_init()
{
  register_relocate_action_list();
}
