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

/* logging routines
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "global.h"

#include "data.h"
#include "gui.h"
#include "log.h"
#include "mymem.h"

#include <X11/Shell.h>
#include <X11/Xaw/AsciiText.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static Widget Popup,		/* the popup shell */
  LogWidget;			/* the text widget */
static XtInputId LogInputID;	/* input ID of log window */
static Boolean Maped = False;	/* maped state of log window */

/* ---------------------------------------------------------------------------
 * some external identifiers
 */
extern int errno;

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void LogEvent (Widget, XtPointer, XEvent *, Boolean *);
static void ReadData (XtPointer, int *, XtInputId *);
static void AppendToLog (char *, int);

/* ---------------------------------------------------------------------------
 * creates a new window to display logging messages
 */
Widget InitLogWindow (Widget Parent, int Filedes)
{
  static char *windowName = NULL;

  if (!windowName)
    {
      windowName = MyCalloc (strlen (Progname) + 5, sizeof (char),
			     "InitLogWindow()");
      sprintf (windowName, "%s-log", Progname);
    }

  /* create shell window with text-widget and exit button */
  Popup = XtVaCreatePopupShell ("log", topLevelShellWidgetClass,
				Parent,
				XtNtitle, windowName,
				XtNiconName, windowName, NULL);

  LogWidget = XtVaCreateManagedWidget ("text", asciiTextWidgetClass,
				       Popup,
				       XtNeditType, XawtextRead,
				       XtNscrollHorizontal,
				       XawtextScrollWhenNeeded,
				       XtNscrollVertical,
				       XawtextScrollAlways, NULL);

  /* bring all stuff to the screen */
  XtPopup (Popup, XtGrabNone);

  /* register 'delete window' message */
  XSetWMProtocols (Dpy, XtWindow (Popup), &WMDeleteWindowAtom, 1);

  /* register event handler for map and unmap (icon) */
  XtAddEventHandler (Popup,
		     StructureNotifyMask, False, (XtEventHandler) LogEvent,
		     NULL);
  LogWindID = XtWindow (Popup);
  /* register new input source */
  LogInputID = XtAppAddInput (Context, Filedes,
			      (XtPointer) XtInputReadMask,
			      (XtInputCallbackProc) ReadData, NULL);

  return (Popup);
}

/* ----------------------------------------------------------------------
 * event handler for map and unmap events
 * used to determine the icon state of the log window
 */
static void
LogEvent (Widget W, XtPointer ClientData, XEvent * Event, Boolean * Flag)
{
  switch (Event->type)
    {
    case MapNotify:		/* window state changes to visible */
      Maped = True;
      break;

    case UnmapNotify:		/* window state changes to invisible or iconified */
      Maped = False;
      break;
    }
}

/* ---------------------------------------------------------------------------
 * this function is called from the X dispatcher when a filedescriptor
 * is ready for reading.
 * Our stderr was 'duped' to a pipe of which the read end is registered
 * as an additional X input source
 */
static void
ReadData (XtPointer ClientData, int *Source, XtInputId * ID)
{
  char buffer[512];		/* data buffer */
  int n;			/* number of bytes read to buffer */

  /* read data and do some error processing */
  switch (n = read (*Source, buffer, sizeof (buffer) - 1))
    {
    case -1:			/* do nothing on signal, remove source on error */
      if (errno != EINTR)
	XtRemoveInput (*ID);
      break;

    case 0:			/* EOF condition */
      XtRemoveInput (*ID);
      break;

    default:			/* copy data to text widget */
      Beep (Settings.Volume);
      AppendToLog (buffer, n);
      break;
    }
}

/* ---------------------------------------------------------------------------
 * append text to log 
 * hints have been taken from 'xconsole.c' by Keith Packard, MIT X Consortium
 */
static void
AppendToLog (char *Buffer, int Length)
{
  XawTextPosition current,	/* cursor position in text widget */
    last;			/* last position in widget */
  XawTextBlock block;		/* description of text */
  Widget source;		/* source widget within text widget */

  /* start searching from the start till the end to get
   * the current text size
   */
  source = XawTextGetSource (LogWidget);
  current = XawTextGetInsertionPoint (LogWidget);
  last = XawTextSourceScan (source, 0, XawstAll, XawsdRight, 1, True);

  /* insert all new data by:
   *   getting the source widget within the text widget
   *   changing the edit mode to XawtextEdit
   *   inserting data by calling XawTextReplace()
   *   changing the edit mode back to XawtextRead
   */
  block.ptr = Buffer;
  block.firstPos = 0;
  block.length = Length;
  block.format = FMT8BIT;
  XtVaSetValues (source, XtNeditType, XawtextEdit, NULL);
  XawTextReplace (LogWidget, last, last, &block);
  XtVaSetValues (source, XtNeditType, XawtextRead, NULL);
  if (current == last)
    XawTextSetInsertionPoint (LogWidget, last + block.length);

  /* change from icon to visible window if necessary */
  if (!Maped)
    XMapRaised (Dpy, XtWindow (Popup));
}
