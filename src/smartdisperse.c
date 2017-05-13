/*!
 * \file src/smartdisperse.c
 *
 * \brief Smart dispersion of elements.
 *
 * Improve the initial dispersion of elements by choosing an order based
 * on the netlist, rather than the arbitrary element order.  This isn't
 * the same as a global autoplace, it's more of a linear autoplace.  It
 * might make some useful local groupings.  For example, you should not
 * have to chase all over the board to find the resistor that goes with
 * a given LED.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * \author Copyright (C) 2007 Ben Jackson <ben@ben.com> based on
 * teardrops.c by Copyright (C) 2006 DJ Delorie <dj@delorie.com> as well
 * as the original action.c, and autoplace.c.
 *
 * Licensed under the terms of the GNU General Public
 * License, version 2 or later.
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
 *
 * Contact addresses for paper mail and Email:
 * Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 * haceaton@aplcomm.jhuapl.edu
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
#include "rats.h"
#include "error.h"
#include "move.h"
#include "draw.h"
#include "set.h"

#define GAP 10000
static Coord minx;
static Coord miny;
static Coord maxx;
static Coord maxy;

/*!
 * \brief Place one element.
 *
 * Must initialize statics above before calling for the first time.
 *
 * This is taken almost entirely from ActionDisperseElements, with cleanup
 */
static void
place (ElementType *element)
{
  Coord dx, dy;

  /* figure out how much to move the element */
  dx = minx - element->BoundingBox.X1;
  dy = miny - element->BoundingBox.Y1;

  /* snap to the grid */
  dx -= (element->MarkX + dx) % (long) (PCB->Grid);
  dx += (long) (PCB->Grid);
  dy -= (element->MarkY + dy) % (long) (PCB->Grid);
  dy += (long) (PCB->Grid);

  /*
   * and add one grid size so we make sure we always space by GAP or
   * more
   */
  dx += (long) (PCB->Grid);

  /* Figure out if this row has room.  If not, start a new row */
  if (minx != GAP && GAP + element->BoundingBox.X2 + dx > PCB->MaxWidth)
  {
    miny = maxy + GAP;
    minx = GAP;
    place(element); /* recurse can't loop, now minx==GAP */
    return;
  }

  /* move the element */
  MoveElementLowLevel (PCB->Data, element, dx, dy);

  /* and add to the undo list so we can undo this operation */
  AddObjectToMoveUndoList (ELEMENT_TYPE, NULL, NULL, element, dx, dy);

  /* keep track of how tall this row is */
  minx += element->BoundingBox.X2 - element->BoundingBox.X1 + GAP;
  if (maxy < element->BoundingBox.Y2)
  {
    maxy = element->BoundingBox.Y2;
  }
}

/*!
 * \brief Return the X location of a connection's pad or pin within its
 * element.
 */
static Coord
padDX (ConnectionType *conn)
{
  ElementType *element = (ElementType *) conn->ptr1;
  AnyLineObjectType *line = (AnyLineObjectType *) conn->ptr2;

  return line->BoundingBox.X1 -
    (element->BoundingBox.X1 + element->BoundingBox.X2) / 2;
}

/*!
 * \brief Return true if ea,eb would be the best order, else eb,ea,
 * based on pad loc.
 */
static int
padorder (ConnectionType *conna, ConnectionType *connb)
{
  Coord dxa, dxb;

  dxa = padDX (conna);
  dxb = padDX (connb);
  /* there are other cases that merit rotation, ignore them for now */
  if (dxa > 0 && dxb < 0)
    return 1;
  return 0;
}

/* ewww, these are actually arrays */
#define ELEMENT_N(DATA,ELT)	((ELT) - (DATA)->Element)
#define VISITED(ELT)		(visited[ELEMENT_N(PCB->Data, (ELT))])
#define IS_ELEMENT(CONN)	((CONN)->type == PAD_TYPE || (CONN)->type == PIN_TYPE)

#define ARG(n) (argc > (n) ? argv[n] : 0)

static const char smartdisperse_syntax[] = "SmartDisperse([All|Selected])";

/* %start-doc actions SmartDisperse

The @code{SmartDisperse([All|Selected])} action is a special-purpose
optimization for dispersing elements.

Run with @code{:SmartDisperse()} or @code{:SmartDisperse(Selected)}
(you can also say @code{:SmartDisperse(All)}, but that's the default).

%end-doc */
static int
smartdisperse (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG(0);
  NetListType *Nets;
  char *visited;
//  PointerListType stack = { 0, 0, NULL };
  int all;
//  int changed = 0;
//  int i;

  if (! function)
  {
    all = 1;
  }
  else if (strcmp(function, "All") == 0)
  {
    all = 1;
  }
  else if (strcmp(function, "Selected") == 0)
  {
    all = 0;
  }
  else
  {
    AFAIL (smartdisperse);
  }

  Nets = ProcNetlist (&PCB->NetlistLib);
  if (! Nets)
  {
    Message (_("Can't use SmartDisperse because no netlist is loaded.\n"));
    return 0;
  }

  /* remember which elements we finish with */
  visited = calloc (PCB->Data->ElementN, sizeof(*visited));

  /* if we're not doing all, mark the unselected elements as "visited" */
  ELEMENT_LOOP (PCB->Data);
  {
    if (! (all || TEST_FLAG (SELECTEDFLAG, element)))
    {
      visited[n] = 1;
    }
  }
  END_LOOP;

  /* initialize variables for place() */
  minx = GAP;
  miny = GAP;
  maxx = GAP;
  maxy = GAP;

  /*
   * Pick nets with two connections.  This is the start of a more
   * elaborate algorithm to walk serial nets, but the datastructures
   * are too gross so I'm going with the 80% solution.
   */
  NET_LOOP (Nets);
  {
    ConnectionType *conna, *connb;
    ElementType *ea, *eb;
//    ElementType *epp;

    if (net->ConnectionN != 2)
      continue;

    conna = &net->Connection[0];
    connb = &net->Connection[1];
    if (!IS_ELEMENT(conna) || !IS_ELEMENT(conna))
      continue;

    ea = (ElementType *) conna->ptr1;
    eb = (ElementType *) connb->ptr1;

    /* place this pair if possible */
    if (VISITED((GList *)ea) || VISITED((GList *)eb))
      continue;
    VISITED ((GList *)ea) = 1;
    VISITED ((GList *)eb) = 1;

    /* a weak attempt to get the linked pads side-by-side */
    if (padorder(conna, connb))
    {
      place ((ElementType *) ea);
      place ((ElementType *) eb);
    }
    else
    {
      place (eb);
      place (ea);
    }
  }
  END_LOOP;

  /* Place larger nets, still grouping by net */
  NET_LOOP (Nets);
  {
    CONNECTION_LOOP (net);
    {
      ElementType *element;

      if (! IS_ELEMENT(connection))
        continue;

      element = (ElementType *) connection->ptr1;

      /* place this one if needed */
      if (VISITED ((GList *) element))
        continue;
      VISITED ((GList *) element) = 1;
      place (element);
    }
    END_LOOP;
  }
  END_LOOP;

  /* Place up anything else */
  ELEMENT_LOOP (PCB->Data);
  {
    if (! visited[n])
    {
      place (element);
    }
  }
  END_LOOP;

  free (visited);

  IncrementUndoSerialNumber ();
  Redraw ();
  SetChangedFlag (1);

  return 0;
}

static HID_Action
smartdisperse_action_list[] =
{
  {"smartdisperse", NULL, smartdisperse, NULL, NULL}
};

REGISTER_ACTIONS (smartdisperse_action_list)

void
hid_smartdisperse_init ()
{
  register_smartdisperse_action_list ();
}
