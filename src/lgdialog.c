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

static char *rcsid = "$Id$";

/* layer-group dialog routines
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "global.h"

#include "crosshair.h"
#include "data.h"
#include "dialog.h"
#include "error.h"
#include "lgdialog.h"
#include "misc.h"
#include "set.h"

#include <X11/Shell.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Toggle.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void CB_CancelOrOK (Widget, XtPointer, XtPointer);

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static long int ReturnCode;		/* return code of buttons */
static LayerGroupType LayerGroups;	/* working copy of current lg */

/* ---------------------------------------------------------------------------
 * callback function for OK and cancel button
 */
static void
CB_CancelOrOK (Widget W, XtPointer ClientData, XtPointer CallData)
{
  ReturnCode = (long int) ClientData;
}

/* ---------------------------------------------------------------------------
 * layer-group dialog
 */
Boolean LayerGroupDialog (void)
{
  Widget popup, masterform, last, layernameW, radioGroup[MAX_LAYER + 2];
  long int i, group, layer;
  XtTranslations translationtable;
  static DialogButtonType buttons[] = {
    {"defaultButton", "   OK   ", CB_CancelOrOK,
     (XtPointer) OK_BUTTON, NULL},
    {"cancelButton", "No/Cancel", CB_CancelOrOK,
     (XtPointer) CANCEL_BUTTON, NULL}
  };
  static char *translations =
    "<EnterWindow>: highlight(Always) \n"
    "<LeaveWindow>: unhighlight() \n"
    "<Btn1Down>,<Btn1Up>: set() notify() \n";

  /* create a working copy of the current layer-group setting
   * and setup a new translation table
   */
  LayerGroups = PCB->LayerGroups;
  translationtable = XtParseTranslationTable (translations);

  /* create a popup shell */
  popup = XtVaCreatePopupShell ("LayerGroups", transientShellWidgetClass,
				Output.Toplevel,
				XtNallowShellResize, True,
				XtNmappedWhenManaged, False, NULL);

  /* the form that holds everything */
  masterform =
    XtVaCreateManagedWidget ("layerGroupMasterForm", formWidgetClass, popup,
			     XtNresizable, True, NULL);

  /* create a 'head-line' with the various layer-groups */
  last = XtVaCreateManagedWidget ("comment", labelWidgetClass,
				  masterform,
				  XtNlabel, "layer-group #",
				  LAYOUT_TOP, NULL);
  for (i = 0; i < MAX_LAYER; i++)
    {
      char s[6];

      sprintf (s, "%li", i + 1);
      last = XtVaCreateManagedWidget ("groupNumber", labelWidgetClass,
				      masterform,
				      XtNfromHoriz, last,
				      XtNlabel, s, LAYOUT_TOP, NULL);
    }

  /* now all the layers with their according toggle widgets;
   * each row is made up of one radio group of toggle widgets
   * the translation table ensures that at least one of them is set.
   * 'last' identifies a widget on top of the current line
   */
  for (layer = 0; layer < MAX_LAYER + 2; layer++)
    {
      Widget lastInRow;
      char *name;
      Pixel color;

      /* setup name and color */
      switch (layer)
	{
	case MAX_LAYER + COMPONENT_LAYER:
	  name = "component side";
	  color = Settings.ElementColor;
	  break;

	case MAX_LAYER + SOLDER_LAYER:
	  name = "solder side";
	  color = Settings.ElementColor;
	  break;

	default:
	  name = UNKNOWN (PCB->Data->Layer[layer].Name);
	  color = PCB->Data->Layer[layer].Color;
	  break;
	}

      /* start with the name followed by the toggle widgets */
      layernameW = XtVaCreateManagedWidget ("layerName", labelWidgetClass,
					    masterform,
					    XtNfromVert, last,
					    XtNlabel, name,
					    XtNforeground, color,
					    LAYOUT_TOP, NULL);
      lastInRow = layernameW;
      for (i = 0; i < MAX_LAYER; i++)
	{
	  lastInRow = XtVaCreateManagedWidget ("checkBox", toggleWidgetClass,
					       masterform,
					       XtNfromVert, last,
					       XtNfromHoriz, lastInRow,
					       XtNforeground, color,
					       XtNlabel, " ",
					       LAYOUT_TOP, NULL);

	  /* the first toggle widget determines the radio group
	   * the translation table is changed to a
	   * 'one out of many' policy
	   */
	  if (i == 0)
	    radioGroup[layer] = lastInRow;
	  XtVaSetValues (lastInRow,
			 XtNradioGroup, radioGroup[layer],
			 XtNradioData, (XtPointer) (i + 1), NULL);
	  XtOverrideTranslations (lastInRow, translationtable);
	}
      last = layernameW;
    }

  /* add the buttons and install accelerators for them;
   * the first one is always default
   */
  AddButtons (masterform, last, buttons, ENTRIES (buttons));
  XtInstallAccelerators (masterform, buttons[0].W);
  XtInstallAccelerators (masterform, buttons[1].W);

  /* setup current state */
  for (group = 0; group < MAX_LAYER; group++)
    for (i = 0; i < LayerGroups.Number[group]; i++)
      XawToggleSetCurrent (radioGroup[LayerGroups.Entries[group][i]],
			   (XtPointer) (group + 1));

  /* now display dialog window */
  StartDialog (popup);
  DialogEventLoop (&ReturnCode);

  /* get settings */
  if (ReturnCode == OK_BUTTON)
    {
      int componentgroup = 0,	/* assign value to get rid of warnings */
        soldergroup = 0;

      /* clear all entries and read layer by layer */
      for (group = 0; group < MAX_LAYER; group++)
	LayerGroups.Number[group] = 0;
      for (i = 0; i < MAX_LAYER + 2; i++)
	{
	  if ((group = (long int) XawToggleGetCurrent (radioGroup[i])) != 0)
	    {
	      group--;
	      LayerGroups.Entries[group][LayerGroups.Number[group]++] = i;
	    }
	  switch (i)
	    {
	    case MAX_LAYER + COMPONENT_LAYER:
	      componentgroup = group;
	      break;

	    case MAX_LAYER + SOLDER_LAYER:
	      soldergroup = group;
	      break;
	    }
	}

      /* do some cross-checks:
       * solder-side an component-side must be in different groups
       * solder-side an component-side must not be the only one in the
       * group
       */
      if (LayerGroups.Number[soldergroup] <= 1 ||
	  LayerGroups.Number[componentgroup] <= 1)
	{
	  Message ("solder- and component-side must not be the\n"
		   "only layers in the group\n");
	  ReturnCode = CANCEL_BUTTON;
	}
      else if (soldergroup == componentgroup)
	{
	  Message ("solder- and component-side are not allowed\n"
		   "to be in the same layer-group\n");
	  ReturnCode = CANCEL_BUTTON;
	}
      else
	PCB->LayerGroups = LayerGroups;
    }
  EndDialog (popup);
  return (ReturnCode == OK_BUTTON);
}
