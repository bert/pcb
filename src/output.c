/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
 *
 *  This module, output.c, was written and is Copyright (C) 1999 harry eaton
 *  portions of code taken from main.c by Thomas Nau
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define VERY_SMALL 8
#include "global.h"

#include "action.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "gui.h"
#include "output.h"
#include "misc.h"

#include <X11/Xaw/Form.h>
#include <X11/Xaw/Panner.h>
#include <X11/Xaw/Porthole.h>
#include <X11/Xaw/Simple.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ----------------------------------------------------------------------
 * Function prototype declarations
 */
static void CB_ScrollX (XtPointer, XtIntervalId *);
static void CB_ScrollY (XtPointer, XtIntervalId *);
static void CB_Panner (Widget, XtPointer, XtPointer);
static Boolean ActiveDrag (void);
static void DrawClipped (Region);
static void StopAutoScroll (void);

static XtIntervalId SxID = 0, SyID = 0;
static Location LastX, LastY;

/* ----------------------------------------------------------------------
 * sets the size of the drawing area
 * is called by MapNotify and ConfigureNotify events
 */
void
GetSizeOfDrawingArea (void)
{
  Dimension width, height;

  XtVaGetValues (Output.Porthole, XtNwidth, &width, XtNheight, &height, NULL);

  /* adjust the size of the statusline */
  XtVaSetValues (Output.StatusLine, XtNwidth, width, NULL);

#ifdef DEBUGDISP
  Message ("GetSizeOfDrawingArea()\n");
#endif
  /* get a new pixmap if necessary */
  if (width != Output.Width || height != Output.Height)
    {
#ifdef DEBUGDISP
      Message ("NewSize!\n");
#endif
      /* can't call NewPixmap() because the sizes aren't right yet ? */
      ReleaseOffscreenPixmap ();
      Offmask = XCreatePixmap (Dpy, Output.OutputWindow, width, height, 1);
      Offscreen = XCreatePixmap (Dpy, Output.OutputWindow, width, height,
				 DefaultDepthOfScreen (XtScreen
						       (Output.Output)));
      if (VALID_PIXMAP (Offmask) && Output.pmGC == (GC) 0)
	Output.pmGC = XCreateGC (Dpy, Offmask, 0, NULL);
      if (!VALID_PIXMAP (Offmask))
	Message ("Damn! Can't get the bitmap!!\n");
      if (!VALID_PIXMAP (Offscreen))
	Message ("Can't get pixmap for offscreen drawing\n"
		 "Drawing will flicker as a result\n");
      ScaleOutput (width, height);
      UpdateAll ();
    }
}
void
ScaleOutput (Dimension width, Dimension height)
{
  Location xmore, ymore;
  Location maxw = TO_SCREEN (PCB->MaxWidth),
    maxh = TO_SCREEN (PCB->MaxHeight);

#ifdef DEBUGDISP
  Message ("ScaleOutput(%d, %d)\n", width, height);
#endif
  /* Compute the output size so the panner
   * has constant aspect ratio and minimum
   * size. xmore and ymore are the amount
   * of grey space beyond the physical pcb
   * that could be shown on the screen
   * The total size is width + xmore
   * by height + ymore and should have
   * a 4/3 aspect ratio.
   */
  ymore = MAX (0, MAX (height, 3 * width / 4) - maxh);
  xmore = 4 * (maxh + ymore) / 3 - maxw;
  if (xmore < 0)
    {
      ymore -= 3 * xmore / 4;
      xmore = 0;
    }
  /* round-off can cause undesired scrollability */
  if (ymore > 0 && xmore > 0)
    {
      xmore = width - maxw;
      ymore = height - maxh;
    }
#ifdef DEBUGDISP
  Message ("ScaleOutput: maxw = %d, maxh= %d xmore = %d ymore = %d\n", maxw,
	   maxh, xmore, ymore);
#endif

  Output.Width = width;
  Output.Height = height;
  Output.cw = maxw + xmore;
  Output.ch = maxh + ymore;
  Pan (Xorig, Yorig, False, False);
  if (Output.ch / height > VERY_SMALL)
    height = Output.ch / VERY_SMALL;
  if (Output.cw / width > VERY_SMALL)
    width = Output.cw / VERY_SMALL;
  XtVaSetValues (Output.panner, XtNcanvasWidth, Output.cw,
		 XtNcanvasHeight, Output.ch,
		 XtNsliderHeight, height, XtNsliderWidth, width, NULL);
}

static void
CB_ScrollX (XtPointer unused, XtIntervalId * time)
{
  SxID = 0;
  HideCrosshair (False);
  CenterDisplay (TO_SCREEN(LastX), 0, True);
  MoveCrosshairRelative (LastX, 0);
  AdjustAttachedObjects ();
  RestoreCrosshair (False);
  LastX += SGN (LastX) * Settings.Grid;
  SxID = XtAppAddTimeOut (Context, SCROLL_TIME, CB_ScrollX, NULL);
}

static void
CB_ScrollY (XtPointer unused, XtIntervalId * time)
{
  SyID = 0;
  HideCrosshair (False);
  CenterDisplay (0, TO_SCREEN(LastY), True);
  MoveCrosshairRelative (0, LastY);
  AdjustAttachedObjects ();
  RestoreCrosshair (False);
  LastY += SGN (LastY) * Settings.Grid;
  SyID = XtAppAddTimeOut (Context, SCROLL_TIME, CB_ScrollY, NULL);
}

/* Pan the screen so that pcb coordinate xp, yp is displayed at screen
 * coordinates xs,ys.
 * Returns True if that is not possible, False otherwise.
 */
Boolean
CoalignScreen (Position xs, Position ys, Location xp, Location yp)
{
  Location x, y;

#ifdef DEBUGDISP
  Message ("CoalignScreen(%d %d %d %d)\n", xs, ys, xp, yp);
#endif
  x = xp - TO_PCB (xs);
  if (SWAP_IDENT)
    y  = PCB->MaxHeight - yp - TO_PCB(ys);
  else
    y = yp - TO_PCB (ys);
  return Pan (x, y, False, True);
}


/* Pan shifts the origin of the output window to PCB coordinates X,Y if
 * possible. if X,Y is out of range, it goes as far as possible and
 * returns True. Otherwise it goest to X,Y and returns false.
 * If Scroll is True and there is no offscreen pixmap, the display
 * is scrolled and only the newly visible part gets updated.
 * If Update is True, an update event is generated so the display
 * is redrawn.
 */
Boolean
Pan (Location X, Location Y, Boolean Scroll, Boolean Update)
{
  Boolean clip = False;
  static Position x, y;
  Region myRegion = XCreateRegion ();
  XRectangle rect;


#ifdef DEBUGDISP
  Message ("Pan(%d, %d, %s, %s)\n", X, Y, Scroll ? "True" : "False",
	   Update ? "True" : "False");
#endif
  if (X < 0)
    {
      clip = True;
      X = 0;
    }
  else if (X > PCB->MaxWidth - TO_PCB (Output.Width))
    {
      clip = True;
      X = MAX (0, PCB->MaxWidth - TO_PCB (Output.Width));
    }
  if (Y < 0)
    {
      clip = True;
      Y = 0;
    }
  else if (Y > PCB->MaxHeight - TO_PCB (Output.Height))
    {
      clip = True;
      Y = MAX (0, PCB->MaxHeight - TO_PCB (Output.Height));
    }

  Xorig = X;
  Yorig = Y;
  render = True;
  /* calculate the visbile bounds in pcb coordinates */
  theScreen.X1 = vxl = TO_PCB_X (0);
  theScreen.X2 = vxh = TO_PCB_X (Output.Width);
  theScreen.Y1 = vyl = MAX (0, MIN (TO_PCB_Y (0), TO_PCB_Y (Output.Height)));
  theScreen.Y2 = vyh = MIN (PCB->MaxHeight, MAX (TO_PCB_Y (0), TO_PCB_Y (Output.Height)));
  /* set up the clipBox */
  clipBox = theScreen;
#ifdef CLIPDEBUG
  clipBox.X1 += TO_PCB(50);
  clipBox.X2 -= TO_PCB(50);
  clipBox.Y1 += TO_PCB(50);
  clipBox.Y2 -= TO_PCB(50);
#else
  clipBox.X1 -= MAX_SIZE;
  clipBox.X2 += MAX_SIZE;
  clipBox.Y1 -= MAX_SIZE;
  clipBox.Y2 += MAX_SIZE;
#endif
#ifdef DEBUGDISP
  Message ("Pan setting Xorig, Yorig = (%d,%d)\n", X, Y);
  Message ("Visible is (%d < X < %d), (%d < Y < %d)\n",vxl,vxh,vyl,vyh);
#endif

  /* ignore the no - change case 
   * BUG: if zoom or viewed side
   * changed, need to redraw anyway.
   * clip can fix all?? of these?
   */
  if (Xorig == x && Yorig == y && !clip)
    {
      return clip;
    }
  /* scroll the pixmap */
  if (Scroll && !VALID_PIXMAP (Offscreen))
    {
      XCopyArea (Dpy, Output.OutputWindow, Output.OutputWindow,
		 Output.bgGC, 0, 0, Output.Width,
		 Output.Height, TO_SCREEN (Xorig - x), TO_SCREEN (Yorig - y));
      /* do same for Offmask ? */
      if (VALID_PIXMAP (Offmask))
	XCopyArea (Dpy, Offmask, Offmask, Output.pmGC, 0, 0,
		   Output.Width, Output.Height, TO_SCREEN (Xorig - x),
		   TO_SCREEN (Yorig - y));
      if (y != Yorig)
	{
	  rect.x = 0;
	  rect.y =
	    (Yorig - y > 0) ? 0 : Output.Height + TO_SCREEN (Yorig - y);
	  rect.width = Output.Width;
	  rect.height = abs (TO_SCREEN (y - Yorig));
	  XUnionRectWithRegion (&rect, myRegion, myRegion);
	  XFillRectangle (Dpy, Output.OutputWindow, Output.bgGC,
			  rect.x, rect.y, rect.width, rect.height);
	}
      if (x != Xorig)
	{
	  rect.x = (Xorig - x > 0) ? 0 : Output.Width + TO_SCREEN (Xorig - x);
	  rect.y = 0;
	  rect.width = abs (TO_SCREEN (x - Xorig));
	  rect.height = Output.Height;
	  XUnionRectWithRegion (&rect, myRegion, myRegion);
	  XFillRectangle (Dpy, Output.OutputWindow, Output.bgGC,
			  rect.x, rect.y, rect.width, rect.height);
	}
    }
  else
    {
      /* the offscreen pixmap is entirely re-drawn */
      rect.x = 0;
      rect.y = 0;
      rect.width = Output.Width;
      rect.height = Output.Height;
      XUnionRectWithRegion (&rect, myRegion, myRegion);
    }

  if (Update)
    {
      XtVaSetValues (Output.panner, XtNsliderX, TO_SCREEN (Xorig),
		     XtNsliderY, TO_SCREEN (Yorig), NULL);
      DrawClipped (myRegion);
    }
  x = Xorig;
  y = Yorig;
  return clip;
}

/* ---------------------------------------------------------------------- 
 * handles all events from porthole window
 */
void
PortholeEvent (Widget W, XtPointer ClientData, XEvent * Event, Boolean * Flag)
{
  if (Event->type == ConfigureNotify)
    GetSizeOfDrawingArea ();
}

void
UpdateExposed (XEvent * Event)
{
  XEvent others;
  Region myRegion = XCreateRegion ();

#ifdef DEBUGDISP
  Message ("UpdateExposed()\n");
#endif
  /* make sure all geometry changes have taken effect */
  XSync (Dpy, False);
  if (Event)
    XtAddExposureToRegion (Event, myRegion);
  while (XtAppPending (Context))
    {
      XtAppNextEvent (Context, &others);
      /* any future re-configuration obviates prior updates */
      if (others.type != Expose ||
	  ((XExposeEvent *) & others)->window != Output.OutputWindow)
	XtDispatchEvent (&others);
      else
	XtAddExposureToRegion (&others, myRegion);
    }
  DrawClipped (myRegion);
}

static void
DrawClipped (Region myRegion)
{
  XRectangle clipBox;

  /* pixmap and screen have different clip regions */
  if (VALID_PIXMAP (Offscreen))
    {
      XClipBox (myRegion, &clipBox);
#ifdef DEBUGDISP
      Message ("ClipBox= (%d, %d), size = (%d, %d)\n", clipBox.x, clipBox.y,
	       clipBox.width, clipBox.height);
#endif
    }
  else
    /* clip drawing to only exposed region */
    {
#ifdef DEBUGDISP
      Message ("Updating with no pixmap!\n");
#endif
      XSetRegion (Dpy, Output.fgGC, myRegion);
      XSetRegion (Dpy, Output.bgGC, myRegion);
      XSetRegion (Dpy, Output.GridGC, myRegion);
      XSetFunction (Dpy, Output.pmGC, GXclear);
      XFillRectangle (Dpy, Offmask, Output.pmGC,
		      0, 0, Output.Width, Output.Height);
      XSetRegion (Dpy, Output.pmGC, myRegion);
    }
  RedrawOutput ();
  XDestroyRegion (myRegion);
  if (!VALID_PIXMAP (Offscreen))
    {
      XSetRegion (Dpy, Output.fgGC, FullRegion);
      XSetRegion (Dpy, Output.bgGC, FullRegion);
      XSetRegion (Dpy, Output.pmGC, FullRegion);
      XSetRegion (Dpy, Output.GridGC, FullRegion);
    }
  else
    /* if output is drawn in pixmap, copy it */
    {
      HideCrosshair (True);
      if (!XCopyArea (Dpy, Offscreen, Output.OutputWindow,
		      Output.fgGC,
		      clipBox.x, clipBox.y,
		      clipBox.width, clipBox.height, clipBox.x, clipBox.y))
	{
	  /* shouldn't be possible to fail, but... */
	  ReleaseOffscreenPixmap ();
	  RedrawOutput ();
	  Message ("hace: problem with XCopyArea!\n");
	}
      RestoreCrosshair (True);
    }
}

static Boolean
ActiveDrag (void)
{
  Boolean active = False;

  switch (Settings.Mode)
    {
    case POLYGON_MODE:
      if (Crosshair.AttachedLine.State != STATE_FIRST)
	active = True;
      break;

    case ARC_MODE:
      if (Crosshair.AttachedBox.State != STATE_FIRST)
	active = True;
      break;

    case LINE_MODE:
      if (Crosshair.AttachedLine.State != STATE_FIRST)
	active = True;
      break;

    case COPY_MODE:
    case MOVE_MODE:
    case INSERTPOINT_MODE:
      if (Crosshair.AttachedObject.Type != NO_TYPE)
	active = True;
      break;
    default:
      break;
    }

  if (Crosshair.AttachedBox.State == STATE_SECOND)
    active = True;

  return (active);
}

static void
StopAutoScroll (void)
{
  if (SxID)
    XtRemoveTimeOut (SxID);
  if (SyID)
    XtRemoveTimeOut (SyID);
  if (SxID == 0 && SyID == 0)
    RestoreCrosshair (True);
  SxID = 0;
  SyID = 0;
}

/* ---------------------------------------------------------------------- 
 * handles all events from output window
 * the handling of MapNotify and ConfigureNotify could be done much
 * easier with X11R5 and later but...
 *
 * I also had to set a flag to indicate a valid 'enter window' because
 * modal windows may generate 'LeaveNotify' events without 'EnterNotify'.
 * This behavior disturbs the crosshair-visibility management
 */
void
OutputEvent (Widget W, XtPointer ClientData, XEvent * Event, Boolean * Flag)
{
  int ksym;
  static Boolean has_entered = False;

  switch (Event->type)
    {
    case Expose:		/* expose event means redraw */
      {
	UpdateExposed (Event);
	break;
      }

    case EnterNotify:		/* enter output area */
      has_entered = True;
      /* stop scrolling if necessary */
      StopAutoScroll ();
      break;

    case LeaveNotify:		/* leave output area */
      /* see also the description at start of this function */
      if (has_entered)
	{
	  Position x, y;

	  /* if actively drawing, start scrolling */
	  if (ActiveDrag ())
	    {
	      x = ((XLeaveWindowEvent *) Event)->x;
	      y = ((XLeaveWindowEvent *) Event)->y;
	      if (x < 0 || x >= Output.Width)
		{
		  LastX = SGN (x) * Settings.Grid;
		  SxID =
		    XtAppAddTimeOut (Context, SCROLL_TIME, CB_ScrollX, NULL);
		}
	      if (y < 0 || y >= Output.Height)
		{
		  LastY = SGN (y) * Settings.Grid;
		  SyID =
		    XtAppAddTimeOut (Context, SCROLL_TIME, CB_ScrollY, NULL);
		}
	    }
	  else
	    HideCrosshair (True);
	}
      has_entered = False;
      break;

    case MapNotify:		/* get initial size of drawing area */
    case ConfigureNotify:	/* get position of origin */
      GetSizeOfDrawingArea ();
      break;

    case MotionNotify:
      /* the damn pop-up menu gives a leave notify */
      /* but no enter notify if the mouse is released */
      /* outside the menu window */
      /* in fact *NO* event is generated to report that */
      /* the menu is gone and we're back in the output */
      /* window ... sigh */
      /* of course movement here suggest we're back */
      if (!has_entered)
	{
	  StopAutoScroll ();
	  has_entered = True;
	}
      EventMoveCrosshair ((XMotionEvent *) Event);
      break;

    case KeyPress:
    case KeyRelease:
      ksym = XKeycodeToKeysym (Dpy, ((XKeyEvent *) Event)->keycode, 0);
      if (ksym == XK_Shift_R || ksym == XK_Shift_L ||
	  ksym == XK_Control_R || ksym == XK_Control_L)
	{
	  HideCrosshair (False);
	  AdjustAttachedObjects ();
	  RestoreCrosshair (True);
	}
      break;
    default:
      break;
    }
}

static void
CB_Panner (Widget W, XtPointer ClientData, XtPointer CallData)
{
  XawPannerReport *report = (XawPannerReport *) CallData;

#ifdef DEBUGDISP
  Message ("CB_Panner()\n");
#endif
  Pan (TO_PCB (report->slider_x), TO_PCB (report->slider_y), True, True);
}

void
CB_StopScroll (Widget W, XtPointer ClientData, XEvent * Event, Boolean * Flag)
{
  if (SxID || SyID)
    StopAutoScroll ();
}

Widget
InitOutputPanner (Widget Parent, Widget Top, Widget Left)
{
  Widget panner;

  panner = XtVaCreateManagedWidget ("panner", pannerWidgetClass,
				    Parent,
				    XtNfromVert, Top,
				    XtNfromHoriz, Left, NULL);
  Output.panner = panner;
  XtAddCallback (panner, XtNreportCallback, CB_Panner, NULL);
  XtAddEventHandler (panner, EnterWindowMask, False, CB_StopScroll, NULL);
  return (panner);
}

void
SetOutputLabel (Widget W, char *Text)
{
  XtVaSetValues (W, XtNlabel, Text, NULL);
}
