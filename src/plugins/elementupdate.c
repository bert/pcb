/*!
 * \file elementupdate.c
 *
 * \brief elementupdate plug-in for PCB
 *
 * \author Copyright (C) 2012 Dan White <dan@whiteaudio.com>
 *
 * \copyright Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Updates selected/all elements from disk.
 *
 * ElementUpdate(Selected|All)
 *
 * Action will attempt to update the selected packages to use the
 * footprint in the currently-loaded Library instead of from the .pcb
 * file.
 *
 * Assumes the footprint name (newlib filename) to load is contained in
 * the current element's "Desc" field (internally as:
 * DESCRIPTION_NAME(element)).
 *
 * The new footprint's mark is placed at the same location and in the
 * same orientation as the old footprint.
 *
 * The element's name, refdes, and value are preserved.
 *
 * NO OTHER CHECKING is done (e.g. number of pins).
 *
 * Reports and does not touch missing footprints.
 *
 */

#include <stdio.h>

#include "config.h"
#include "global.h"

#include "buffer.h"
#include "change.h"
#include "copy.h"
#include "create.h"
#include "data.h"
#include "error.h"
#include "mirror.h"
#include "misc.h"
#include "move.h"
#include "remove.h"
#include "rotate.h"
#include "select.h"
#include "set.h"
#include "undo.h"

#define ARG(n) (argc > (n) ? argv[n] : 0)

enum
{
  F_none,
  F_All,
  F_Selected,
};

static const char *functions[] =
{
  [F_All] "All",
  [F_Selected] "Selected",
};

static int
GetFunctionID(const char *s)
{
  int i;

  if (! s)
    return F_none;
  for (i = 0; i < ENTRIES(functions); ++i)
    if (functions[i] && strcasecmp(s, functions[i]) == 0)
      return i;
  return F_none;
}

/* ---------------------------------------------------------------- */
static const char elementupdate_syntax[] = "ElementUpdate(Selected|All)";

static const char elementupdate_help[] = "Reloads the indicated element packages.";

/* %start-doc actions ElementUpdate

@table @code

@item Selected
Only updates the footprint of the selected elements from the currently-loaded
library.

@item All
Reloads from the currently-loaded library the packages of all elements in the
layout.

@end table

Extracts the footprint name to load from the element's @code{Desc} field.
Preserves the old element's Desc, Name, and Value fields.

%end-doc */

static int update_footprints_not_found;

static int
ActionElementUpdate (int argc, char **argv, Coord x, Coord y)
{
  char *function = NULL;
  int fnid;
  int er, pr;
  ElementType *pe = NULL;
  Coord mx, my;
  int i;
  char *old;

  function = ARG (0);
  update_footprints_not_found = 0;
#ifdef DEBUG
  printf ("Entered ActionElementUpdate, executing function %s\n", function);
#endif
  if (function)
  {
    fnid = GetFunctionID (function);
    if (!(fnid == F_All || fnid == F_Selected))
      AFAIL (elementupdate);
  }
  else
  {
    AFAIL (elementupdate);
  }
  /* make sure to re-read footprints from disk */
  /* to enable this, add the following to buffer.h:
   * void clear_footprint_hash (void);
   * and un-comment below */
  /* clear_footprint_hash ();*/
  /* Select all elements for All mode */
  if (fnid == F_All)
    SelectObjectByName (ELEMENT_TYPE, ".*", true);
  /* Flag all original elements, we are adding more but don't want to process
   * the new ones*/
  ELEMENT_LOOP (PCB->Data);
  {
    SET_FLAG (VISITFLAG, element);
  }
  END_LOOP;
  ELEMENT_LOOP (PCB->Data);
  {
    /* All elements (from here on ??) were added in previous iterations of this
     * loop; we are done. */
    if (!TEST_FLAG(VISITFLAG, element))
      break;
    /* Only touch selected elements. */
    if (!TEST_FLAG (SELECTEDFLAG, element))
      continue;
    /* no element name, it's likely not something we wish to change skip */
    if (EMPTY_STRING_P (NAMEONPCB_NAME (element)))
      continue;
    if (EMPTY_STRING_P (DESCRIPTION_NAME (element)))
    {
      Message (_("Skipping refdes %s, no footprint name\n"), NAMEONPCB_NAME (element));
      continue;
      }
    if (EMPTY_STRING_P (VALUE_NAME (element)))
    {
      Message (_("For refdes %s, no value\n"), NAMEONPCB_NAME (element));
    }
    /* to get here, this is a footprint element (??) */
    /* do not touch element if not in current library */
    if (LoadFootprint(1, &DESCRIPTION_NAME(element), x, y))
    {
      update_footprints_not_found ++;
      Message (_("Footprint not found: %s\n"), DESCRIPTION_NAME(element));
      continue;
    }
    /* all good, now update the package */
    Message (_("Updating footprint for (%s, %s, %s).\n"),
      NAMEONPCB_NAME(element),
      VALUE_NAME(element),
      DESCRIPTION_NAME(element)
      );
    /* in the method of ElementList(Need,...) */
    pe = PASTEBUFFER->Data->Element->data;
    if (!FRONT (element))
      MirrorElementCoordinates (PASTEBUFFER->Data, pe, pe->MarkY*2 - PCB->MaxHeight);
    er = ElementOrientation (element);
    pr = ElementOrientation (pe);
    if (er != pr)
      RotateElementLowLevel (PASTEBUFFER->Data, pe, pe->MarkX, pe->MarkY, (er-pr+4)%4);
    /* move element parts to their previous locations */
    mx = element->MarkX;
    my = element->MarkY;
    for (i=0; i<MAX_ELEMENTNAMES; i++)
    {
      pe->Name[i].X = element->Name[i].X - mx + pe->MarkX ;
      pe->Name[i].Y = element->Name[i].Y - my + pe->MarkY ;
      pe->Name[i].Direction = element->Name[i].Direction;
      pe->Name[i].Scale = element->Name[i].Scale;
    }
    /* preserve attributes */
    for (i=0; i<element->Attributes.Number; i++)
    {
      CreateNewAttribute (& pe->Attributes,
        element->Attributes.List[i].name,
        element->Attributes.List[i].value);
    }
    /* preserve name, refdes, and value */
    old = ChangeElementText (PCB, PASTEBUFFER->Data, pe,
      DESCRIPTION_INDEX, strdup (DESCRIPTION_NAME(element)));
    if (old)
      free (old);
    old = ChangeElementText (PCB, PASTEBUFFER->Data, pe, 
      NAMEONPCB_INDEX, strdup (NAMEONPCB_NAME(element)));
    if (old)
      free (old);
    old = ChangeElementText (PCB, PASTEBUFFER->Data, pe, VALUE_INDEX,
      strdup (VALUE_NAME(element)));
    if (old)
      free (old);
    /* who needs old data? */
    RemoveElement (element);
    /* place in layout */
    if (CopyPastebufferToLayout (mx, my))
      SetChangedFlag (true);
  }
  END_LOOP;
  if (update_footprints_not_found > 0)
  {
    gui->confirm_dialog (_("Not all requested footprints were found.\n"
      "See the message log for details"), 0);
  }
#ifdef DEBUG
  printf (" ... Leaving ActionElementUpdate.\n");
#endif
  return 0;
}


static HID_Action elementupdate_action_list[] =
{
  {"ElementUpdate", 0, ActionElementUpdate, elementupdate_help, elementupdate_syntax}
};

REGISTER_ACTIONS (elementupdate_action_list)

void
hid_elementupdate_init ()
{
    register_elementupdate_action_list ();
}
