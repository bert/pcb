/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */


/* selector routines */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <memory.h>

#include "global.h"

#include "data.h"
#include "error.h"
#include "mymem.h"
#include "misc.h"
#include "selector.h"

#include <X11/Xaw/Form.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Viewport.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID("$Id$");




/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static int CompareFunction (const void *, const void *);

/* ----------------------------------------------------------------------
 * compare function for qsort() routine
 * used to sort list by name
 */
static int
CompareFunction (const void *P1, const void *P2)
{
  SelectorEntryTypePtr p1 = (SelectorEntryTypePtr) P1,
    p2 = (SelectorEntryTypePtr) P2;

  return (NSTRCMP (p1->Text, p2->Text));
}

/* ---------------------------------------------------------------------------
 * updates an selector
 */
void
UpdateSelector (SelectorTypePtr Selector)
{
  Cardinal i;

  if (Selector->Number)
    {
      /* sort entries */
      qsort (Selector->Entries, Selector->Number,
	     sizeof (SelectorEntryType), CompareFunction);

      /* copy data to tmp list */
      Selector->StringList = MyRealloc (Selector->StringList,
					Selector->Number * sizeof (String),
					"UpdateSelector()");
      for (i = 0; i < Selector->Number; i++)
	Selector->StringList[i] = Selector->Entries[i].Text;

      /* update screen */
      XawListChange (Selector->ListW, Selector->StringList,
		     Selector->Number, 0, True);
    }
  else
    {
      static String dummy[] = { NULL, NULL };

      /* create an empty list */
      XawListChange (Selector->ListW, dummy, 0, 0, True);
    }
}

/* ---------------------------------------------------------------------------
 * adds a new entry to the list;
 * memory is allocated as needed;
 */
void
AddEntryToSelector (char *Text, XtPointer ClientData,
		    SelectorTypePtr Selector)
{
  /* allocate more memory */
  if (Selector->Number >= Selector->MaxNumber)
    {
      Selector->MaxNumber += STEP_SELECTORENTRY;
      Selector->Entries = MyRealloc (Selector->Entries,
				     Selector->MaxNumber *
				     sizeof (SelectorEntryType),
				     "AddEntryToSelector()");
      memset (Selector->Entries + Selector->Number, 0,
	      STEP_SELECTORENTRY * sizeof (SelectorEntryType));
    }
  Selector->Entries[Selector->Number].Text =
    MyStrdup (Text, "AddEntryToSelector()");
  Selector->Entries[Selector->Number++].ClientData = ClientData;
}

/* ---------------------------------------------------------------------------
 * releases all memory that is allocated by list entries, not the list
 * itself
 */
void
FreeSelectorEntries (SelectorTypePtr Selector)
{
  while (Selector->Number)
    MyFree (&Selector->Entries[--Selector->Number].Text);
  MyFree ((char **) &Selector->StringList);
}

/* ---------------------------------------------------------------------------
 * creates a selector (list widget managed by a viewport)
 */
Widget
CreateSelector (Widget Parent,
		Widget Top, Widget Left,
		SelectorTypePtr Selector, Cardinal columns)
{
  Selector->ViewportW =
    XtVaCreateManagedWidget ("selector", viewportWidgetClass, Parent,
			     XtNfromVert, Top, XtNfromHoriz, Left,
			     LAYOUT_NORMAL, XtNallowHoriz, True, XtNallowVert,
			     True, XtNuseBottom, True, NULL);

  Selector->ListW = XtVaCreateManagedWidget (Selector->Name, listWidgetClass,
					     Selector->ViewportW,
					     XtNdefaultColumns, columns,
					     XtNforceColumns, True, NULL);
  if (Selector->Callback)
    XtAddCallback (Selector->ListW, XtNcallback,
		   Selector->Callback, Selector->ClientData);

  return (Selector->ViewportW);
}
