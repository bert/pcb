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

/* pinout routines */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "global.h"

#include "copy.h"
#include "data.h"
#include "draw.h"
#include "mymem.h"
#include "move.h"
#include "pinout.h"
#include "rotate.h"

#include <X11/Shell.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Simple.h>
#include <X11/Xaw/Viewport.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some local types
 */
typedef struct			/* information of one window */
{
  ElementType Element;		/* element data to display */
  String Title;			/* window title */
  Widget Shell,			/* shell widget */
    Output,			/* output widget managed by a viewport widget */
    Enlarge,			/* enlarge button */
    Shrink;			/* shrink button */
  int Zoom;			/* zoom factor of window */
  Location MaxX,		/* size of used drawing area independend */
    MaxY;			/* from zoom setting */
}
PinoutType, *PinoutTypePtr;

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void CB_Dismiss (Widget, XtPointer, XtPointer);
static void CB_ShrinkOrEnlarge (Widget, XtPointer, XtPointer);
static void RedrawPinoutWindow (PinoutTypePtr);
static void PinoutEvent (Widget, XtPointer, XEvent *, Boolean *);

/* ---------------------------------------------------------------------------
 * creates a new window to display an elements pinout
 */
void
PinoutWindow (Widget Parent, ElementTypePtr Element)
{
  Widget masterform, dismiss, scrollbar, viewport;
  PinoutTypePtr pinout;
  Dimension tx, ty, minx, miny;

  if (!Element)
    return;

  /* allocate memory for title and pinout data, init zoom factor */
  pinout =
    (PinoutTypePtr) MyCalloc (1, sizeof (PinoutType), "PinoutWindow()");
  pinout->Title =
    (String) MyCalloc (strlen (UNKNOWN (NAMEONPCB_NAME (Element))) +
		       strlen (UNKNOWN (DESCRIPTION_NAME (Element))) +
		       strlen (UNKNOWN (VALUE_NAME (Element))) + 5,
		       sizeof (char), "PinoutWindow()");
  sprintf (pinout->Title, "%s [%s,%s]", UNKNOWN (DESCRIPTION_NAME (Element)),
	   UNKNOWN (NAMEONPCB_NAME (Element)),
	   UNKNOWN (VALUE_NAME (Element)));

  /* copy element data 
   * enable output of pin- and padnames
   * move element to a 5% offset from zero position
   * set all package lines/arcs to zero with
   */
  CopyElementLowLevel (PCB->Data, &pinout->Element, Element, False);
  minx = miny = 32767;
  PIN_LOOP (&pinout->Element, 
    {
      tx = abs (pinout->Element.Pin[0].X - pin->X);
      ty = abs (pinout->Element.Pin[0].Y - pin->Y);
      if (tx != 0 && tx < minx)
	minx = tx;
      if (ty != 0 && ty < miny)
	miny = ty;
      SET_FLAG (DISPLAYNAMEFLAG, pin);
    }
  );

  PAD_LOOP (&pinout->Element, 
    {
      tx = abs (pinout->Element.Pad[0].Point1.X - pad->Point1.X);
      ty = abs (pinout->Element.Pad[0].Point1.Y - pad->Point1.Y);
      if (tx != 0 && tx < minx)
	minx = tx;
      if (ty != 0 && ty < miny)
	miny = ty;
      SET_FLAG (DISPLAYNAMEFLAG, pad);
    }
  );
  if (minx < miny)
    RotateElementLowLevel (&pinout->Element, pinout->Element.BoundingBox.X1,
			   pinout->Element.BoundingBox.Y1, 1);

  MoveElementLowLevel (&pinout->Element,
		       -pinout->Element.BoundingBox.X1 +
		       Settings.PinoutOffsetX,
		       -pinout->Element.BoundingBox.Y1 +
		       Settings.PinoutOffsetY);
  pinout->Zoom = Settings.PinoutZoom;
  pinout->MaxX = pinout->Element.BoundingBox.X2 + Settings.PinoutOffsetX;
  pinout->MaxY = pinout->Element.BoundingBox.Y2 + Settings.PinoutOffsetY;
  ELEMENTLINE_LOOP (&pinout->Element, 
    {
      line->Thickness = 0;
    }
  );
  ARC_LOOP (&pinout->Element, 
    {
      arc->Thickness = 0;
    }
  );

  /* create shell window with viewport,
   * shrink, enlarge and exit button
   */
  pinout->Shell = XtVaCreatePopupShell ("pinout",
					topLevelShellWidgetClass,
					Parent,
					XtNtitle, pinout->Title,
					XtNallowShellResize, False,
					XtNmappedWhenManaged, False, NULL);
  masterform = XtVaCreateManagedWidget ("pinoutMasterForm",
					formWidgetClass,
					pinout->Shell,
					XtNresizable, False,
					XtNfromHoriz, NULL,
					XtNfromVert, NULL, NULL);
  viewport = XtVaCreateManagedWidget ("viewport",
				      viewportWidgetClass,
				      masterform,
				      XtNresizable, False,
				      XtNforceBars, True,
				      LAYOUT_NORMAL,
				      XtNallowHoriz, True,
				      XtNallowVert, True,
				      XtNuseBottom, True, NULL);
  pinout->Output = XtVaCreateManagedWidget ("output",
					    simpleWidgetClass,
					    viewport,
					    XtNresizable, True,
					    XtNwidth,
					    pinout->MaxX >> pinout->Zoom,
					    XtNheight,
					    pinout->MaxY >> pinout->Zoom,
					    NULL);
  dismiss =
    XtVaCreateManagedWidget ("dismiss", commandWidgetClass, masterform,
			     XtNfromVert, viewport, LAYOUT_BOTTOM, NULL);
  pinout->Shrink =
    XtVaCreateManagedWidget ("shrink", commandWidgetClass, masterform,
			     XtNfromVert, viewport, XtNfromHoriz, dismiss,
			     LAYOUT_BOTTOM, NULL);
  pinout->Enlarge =
    XtVaCreateManagedWidget ("enlarge", commandWidgetClass, masterform,
			     XtNfromVert, viewport, XtNfromHoriz,
			     pinout->Shrink, LAYOUT_BOTTOM, NULL);

  /* install accelerators for WM messages and to
   * move scrollbars with keys
   */
  XtInstallAccelerators (pinout->Shell, dismiss);
  if ((scrollbar = XtNameToWidget (viewport, "horizontal")) != NULL)
    XtInstallAccelerators (masterform, scrollbar);
  if ((scrollbar = XtNameToWidget (viewport, "vertical")) != NULL)
    XtInstallAccelerators (masterform, scrollbar);

  /* add event handler for viewport and callbacks for buttons
   * the pointer to the pinout structure is passed to these functions
   * as 'ClientData'
   */
  XtAddCallback (dismiss, XtNcallback, CB_Dismiss, (XtPointer) pinout);
  XtAddCallback (pinout->Shrink, XtNcallback,
		 CB_ShrinkOrEnlarge, (XtPointer) pinout);
  XtAddCallback (pinout->Enlarge, XtNcallback,
		 CB_ShrinkOrEnlarge, (XtPointer) pinout);
  XtAddEventHandler (pinout->Output,
		     ExposureMask,
		     False, (XtEventHandler) PinoutEvent, (XtPointer) pinout);

  /* realize (without mapping) and handle 'delete' messages */
  XtRealizeWidget (pinout->Shell);
  XSetWMProtocols (Dpy, XtWindow (pinout->Shell), &WMDeleteWindowAtom, 1);

  /* bring all stuff to the screen */
  XtPopup (pinout->Shell, XtGrabNone);
}

/* ---------------------------------------------------------------------------
 * redraws pinout window
 */
static void
RedrawPinoutWindow (PinoutTypePtr Pinout)
{
  Window window = XtWindow (Pinout->Output);

  if (window)			/* check for valid ID */
    {
      /* setup drawable and zoom factor for drawing routines */
      SwitchDrawingWindow (Pinout->Zoom, window, False, False);

      /* clear background call the drawing routine */
      XFillRectangle (Dpy, window, Output.bgGC, 0, 0, MAX_COORD, MAX_COORD);
      DrawElement (&Pinout->Element, 0);

      /* reset drawing routines to normal operation */
      SwitchDrawingWindow (PCB->Zoom, Output.OutputWindow,
			   Settings.ShowSolderSide, True);
    }
}

/* ---------------------------------------------------------------------------
 * event handler for all pinout windows
 * a pointer to the pinout struct is passed as ClientData
 */
static void
PinoutEvent (Widget W, XtPointer ClientData, XEvent * Event, Boolean * Flag)
{
  switch (Event->type)
    {
    case Expose:		/* just redraw the complete window */
      RedrawPinoutWindow ((PinoutTypePtr) ClientData);
      break;
    }
}

/* ---------------------------------------------------------------------------
 * callback routine of the dismiss buttons of all pinout windows
 * a pointer to the pinout struct is passed as ClientData
 */
static void
CB_Dismiss (Widget W, XtPointer ClientData, XtPointer CallData)
{
  PinoutTypePtr pinout = (PinoutTypePtr) ClientData;

  /* release memory */
  XtDestroyWidget (pinout->Shell);
  SaveFree (pinout->Title);
  FreeElementMemory (&pinout->Element);
  SaveFree (pinout);
}

/* ---------------------------------------------------------------------------
 * callback routine of the shrink or enlarge buttons of all windows
 * a pointer to the pinout struct is passed as ClientData
 */
static void
CB_ShrinkOrEnlarge (Widget W, XtPointer ClientData, XtPointer CallData)
{
  PinoutTypePtr pinout = (PinoutTypePtr) ClientData;

  if (W == pinout->Shrink && pinout->Zoom < MAX_ZOOM)
    pinout->Zoom++;
  if (W == pinout->Enlarge && pinout->Zoom > MIN_ZOOM)
    pinout->Zoom--;
  XtVaSetValues (pinout->Output,
		 XtNwidth, pinout->MaxX >> pinout->Zoom,
		 XtNheight, pinout->MaxY >> pinout->Zoom, NULL);
  RedrawPinoutWindow (pinout);
}
