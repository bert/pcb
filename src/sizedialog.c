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

/* size dialog routines
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "global.h"

#include "change.h"
#include "crosshair.h"
#include "data.h"
#include "dialog.h"
#include "error.h"
#include "menu.h"
#include "misc.h"
#include "sizedialog.h"
#include "set.h"

#include <X11/Shell.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Toggle.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * define resource for X11R4 (names have changed from R4 to R5)
 * and scrollbar sizes
 */
#ifndef	XtNtopOfThumb
#define	XtNtopOfThumb	XtNtop
#endif

#define	MIN_SCALE		0.5
#define	MAX_SCALE		3.0
#define	THUMB_LENGTH		10
#define	SCROLLBAR_LENGTH	250

#define	LINE_SLIDER		0
#define	VIADRILL_SLIDER		1
#define	VIASIZE_SLIDER		2
#define KEEPAWAY_SLIDER		3
#define	TEXTSCALE_SLIDER	4
#define	WIDTH_SLIDER		5
#define	HEIGHT_SLIDER		6

/* ---------------------------------------------------------------------------
 * some local types
 */
typedef struct
{
  String Label;			/* label to display */
  Dimension Min,		/* limits of scale */
    Max, Step,			/* one step */
    Value;
  Widget Scrollbar,		/* some widgets */
    Size;			/* size field (label widget) */
}
SliderType, *SliderTypePtr;

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static long int ReturnCode;		/* returncode of buttons */
static SliderType Sliders[] = {
  {"linewidth", MIN_LINESIZE, MAX_LINESIZE, CHUNK, 0, NULL, NULL},
  {"via hole", MIN_PINORVIAHOLE, 0, CHUNK, 0, NULL, NULL},
  {"via size", 0, MAX_PINORVIASIZE, CHUNK, 0, NULL, NULL},
  {"keepaway", MIN_LINESIZE, MAX_LINESIZE, CHUNK, 0, NULL, NULL},
  {"text size", MIN_TEXTSCALE, MAX_TEXTSCALE, 10, 0, NULL, NULL},
  {"PCB width", MIN_SIZE, MAX_COORD, 100, 0, NULL, NULL},
  {"PCB height", MIN_SIZE, MAX_COORD, 100, 0, NULL, NULL}
};

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void UpdateSlider (SliderTypePtr);
static Dimension UpdateThumb (SliderTypePtr, Dimension);
static void UpdateScrollbar (SliderTypePtr, Dimension);
static void CB_CancelOrOK (Widget, XtPointer, XtPointer);
static void CB_ScrollProc (Widget, XtPointer, XtPointer);
static void CB_JumpProc (Widget, XtPointer, XtPointer);

/* ---------------------------------------------------------------------------
 * updates a slider
 */
static void
UpdateSlider (SliderTypePtr Slider)
{
  char s[10];

  sprintf (s, "%i", Slider->Value);
  XtVaSetValues (Slider->Size, XtNlabel, s, NULL);
}

/* ---------------------------------------------------------------------------
 * clips the value to fit into min/max range and updates the widget
 * returns the clipped value
 */
static Dimension
UpdateThumb (SliderTypePtr Slider, Dimension NewValue)
{
  float top;

  /* set new value, reduce to valid 'step' values */
  NewValue = (NewValue / Slider->Step) * Slider->Step;
  NewValue = MIN (NewValue, Slider->Max);
  NewValue = MAX (NewValue, Slider->Min);
  Slider->Value = NewValue;

  /* prevent from floating point errors */
  if (Slider->Max == Slider->Min)
    top = 1.0;
  else
    top =
      (float) (Slider->Value - Slider->Min) / (float) (Slider->Max -
						       Slider->Min);
  top = MIN (top, (1.0 - ((float) THUMB_LENGTH / (float) SCROLLBAR_LENGTH)));

  /* change position only */
  XawScrollbarSetThumb (Slider->Scrollbar, top, -1.0);
  return (NewValue);
}

/* ---------------------------------------------------------------------------
 * updates the position of the scrollbar thumb
 * the thumb position is passed; related scrollbars are updated too
 */
static void
UpdateScrollbar (SliderTypePtr Slider, Dimension NewValue)
{
  NewValue = UpdateThumb (Slider, NewValue);
  UpdateSlider (Slider);

  /* changing of via thickness might cause the drilling hole slider
   * to change it's limits and vice versa
   */
  if (Slider == &Sliders[VIADRILL_SLIDER])
    {
      /* set lower limit of via size slider and recalculate
       * its current value
       */
      Sliders[VIASIZE_SLIDER].Min = NewValue + MIN_PINORVIACOPPER;
      UpdateThumb (&Sliders[VIASIZE_SLIDER], Sliders[VIASIZE_SLIDER].Value);
      UpdateSlider (&Sliders[VIASIZE_SLIDER]);
    }
  if (Slider == &Sliders[VIASIZE_SLIDER])
    {
      /* set lower limit of via drilling hole slider and recalculate
       * its current value
       */
      Sliders[VIADRILL_SLIDER].Max = NewValue - MIN_PINORVIACOPPER;
      UpdateThumb (&Sliders[VIADRILL_SLIDER], Sliders[VIADRILL_SLIDER].Value);
      UpdateSlider (&Sliders[VIADRILL_SLIDER]);
    }
}

/* ---------------------------------------------------------------------------
 * callback function for OK and cancel button
 */
static void
CB_CancelOrOK (Widget W, XtPointer ClientData, XtPointer CallData)
{
  ReturnCode = (long int) ClientData;
}

/* ---------------------------------------------------------------------------
 * callback function for scrolling
 * see Athena Widget manual for details
 * the scrollwidth is replaced by a constant
 */
static void
CB_ScrollProc (Widget W, XtPointer ClientData, XtPointer CallData)
{
  long int delta = (long int) CallData;
  SliderTypePtr slider = (SliderTypePtr) ClientData;

  UpdateScrollbar (slider,
		   slider->Value + (delta >=
				    0 ? slider->Step : -slider->Step));
}

/* ---------------------------------------------------------------------------
 * callback function for scrolling
 * see Athena Widget manual for details
 */
static void
CB_JumpProc (Widget W, XtPointer ClientData, XtPointer CallData)
{
  float top = *(float *) CallData;
  SliderTypePtr slider = (SliderTypePtr) ClientData;

  UpdateScrollbar (slider,
		   (Dimension) (top * (slider->Max - slider->Min) +
				(float) slider->Min + 0.5));
}

/* ---------------------------------------------------------------------------
 * active size dialog
 */
void
SizeDialog (void)
{
  Widget popup, masterform, label, last;
  int i;
  BoxTypePtr box;
  static DialogButtonType buttons[] = {
    {"defaultButton", "   OK   ", CB_CancelOrOK,
     (XtPointer) OK_BUTTON, NULL},
    {"cancelButton", "No/Cancel", CB_CancelOrOK,
     (XtPointer) CANCEL_BUTTON, NULL}
  };

  /* copy current values to struct */
  Sliders[LINE_SLIDER].Value = Settings.LineThickness;
  Sliders[VIADRILL_SLIDER].Value = Settings.ViaDrillingHole;
  Sliders[VIADRILL_SLIDER].Max = Settings.ViaThickness - MIN_PINORVIACOPPER;
  Sliders[VIASIZE_SLIDER].Value = Settings.ViaThickness;
  Sliders[VIASIZE_SLIDER].Min = Settings.ViaDrillingHole + MIN_PINORVIACOPPER;
  Sliders[KEEPAWAY_SLIDER].Value = Settings.Keepaway;
  Sliders[TEXTSCALE_SLIDER].Value = Settings.TextScale;
  Sliders[WIDTH_SLIDER].Value = PCB->MaxWidth;
  Sliders[HEIGHT_SLIDER].Value = PCB->MaxHeight;

  /* adjust the minimum size that's allowed */
  if ((box = GetDataBoundingBox (PCB->Data)) == NULL)
    {
      Sliders[WIDTH_SLIDER].Min = MIN_SIZE;
      Sliders[HEIGHT_SLIDER].Min = MIN_SIZE;
    }
  else
    {
      Sliders[WIDTH_SLIDER].Min = box->X2;
      Sliders[HEIGHT_SLIDER].Min = box->Y2;
    }

  /* create a popup shell */
  popup = XtVaCreatePopupShell ("Sizes", transientShellWidgetClass,
				Output.Toplevel,
				XtNallowShellResize, True,
				XtNmappedWhenManaged, False, NULL);

  /* the form that holds everything */
  masterform = XtVaCreateManagedWidget ("sizeMasterForm", formWidgetClass,
					popup, XtNresizable, True, NULL);

  /* create the sliders */
  last = NULL;
  for (i = 0; i < ENTRIES (Sliders); i++)
    {
      label = XtVaCreateManagedWidget ("comment", labelWidgetClass,
				       masterform,
				       XtNfromVert, last,
				       XtNlabel, Sliders[i].Label,
				       LAYOUT_TOP, NULL);
      Sliders[i].Size = XtVaCreateManagedWidget ("size", labelWidgetClass,
						 masterform,
						 XtNfromVert, last,
						 XtNfromHoriz, label,
						 XtNresizable, True,
						 LAYOUT_TOP, NULL);
      last = XtVaCreateManagedWidget ("scrollbar", scrollbarWidgetClass,
				      masterform,
				      LAYOUT_TOP,
				      XtNfromVert, label,
				      XtNminimumThumb, THUMB_LENGTH,
				      XtNorientation, XtorientHorizontal,
				      XtNwidth, SCROLLBAR_LENGTH, NULL);
      Sliders[i].Scrollbar = last;

      /* add callbacks for scrolling */
      XtAddCallback (Sliders[i].Scrollbar,
		     XtNjumpProc, CB_JumpProc, &Sliders[i]);
      XtAddCallback (Sliders[i].Scrollbar,
		     XtNscrollProc, CB_ScrollProc, &Sliders[i]);
    }

  /* now that all sliders are initialized --> do an update */
  for (i = 0; i < ENTRIES (Sliders); i++)
    UpdateScrollbar (&Sliders[i], Sliders[i].Value);

  /* add the buttons and install accelerators for them;
   * the first one is always default
   */
  AddButtons (masterform, last, buttons, ENTRIES (buttons));
  XtInstallAccelerators (masterform, buttons[0].W);
  XtInstallAccelerators (masterform, buttons[1].W);

  /* now display dialog window */
  StartDialog (popup);
  DialogEventLoop (&ReturnCode);

  /* get settings */
  if (ReturnCode == OK_BUTTON)
    {
      SetLineSize (Sliders[LINE_SLIDER].Value);
      SetViaSize (Sliders[VIASIZE_SLIDER].Value, True);
      SetViaDrillingHole (Sliders[VIADRILL_SLIDER].Value, True);
      SetKeepawayWidth (Sliders[KEEPAWAY_SLIDER].Value);
      SetTextScale (Sliders[TEXTSCALE_SLIDER].Value);

      /* set new maximum size and update scrollbars */
      if (PCB->MaxWidth != Sliders[WIDTH_SLIDER].Value ||
	  PCB->MaxHeight != Sliders[HEIGHT_SLIDER].Value)
	{
	  ChangePCBSize (Sliders[WIDTH_SLIDER].Value,
			 Sliders[HEIGHT_SLIDER].Value);
	}
    }
  EndDialog (popup);
}

/* ---------------------------------------------------------------------------
 * style sizes dialog
 */
void
StyleSizeDialog (int index)
{
  Widget popup, masterform, label, last, inputfield;
  char *string;
  int i;
  static DialogButtonType buttons[] = {
    {"defaultButton", "   OK   ", CB_CancelOrOK,
     (XtPointer) OK_BUTTON, NULL},
    {"cancelButton", "No/Cancel", CB_CancelOrOK,
     (XtPointer) CANCEL_BUTTON, NULL}
  };
  static char styleName[64];

  /* copy current values to struct */
  Sliders[LINE_SLIDER].Value = PCB->RouteStyle[index].Thick;
  Sliders[VIADRILL_SLIDER].Value = PCB->RouteStyle[index].Hole;
  Sliders[VIADRILL_SLIDER].Max =
    PCB->RouteStyle[index].Diameter - MIN_PINORVIACOPPER;
  Sliders[VIASIZE_SLIDER].Value = PCB->RouteStyle[index].Diameter;
  Sliders[VIASIZE_SLIDER].Min =
    PCB->RouteStyle[index].Hole + MIN_PINORVIACOPPER;
  Sliders[KEEPAWAY_SLIDER].Value = PCB->RouteStyle[index].Keepaway;

  sprintf (styleName, "'%s' Sizes", PCB->RouteStyle[index].Name);
  /* create a popup shell */
  popup = XtVaCreatePopupShell (styleName, transientShellWidgetClass,
				Output.Toplevel,
				XtNallowShellResize, True,
				XtNmappedWhenManaged, False, NULL);

  /* the form that holds everything */
  masterform = XtVaCreateManagedWidget ("sizeMasterForm", formWidgetClass,
					popup, XtNresizable, True, NULL);
  last = XtVaCreateManagedWidget ("comment", labelWidgetClass,
				  masterform,
				  XtNfromVert, NULL,
				  XtNlabel, "Routing Style Name",
				  LAYOUT_TOP, NULL);
  inputfield = XtVaCreateManagedWidget ("Name", asciiTextWidgetClass,
					masterform,
					XtNfromVert, last,
					XtNresizable, True,
					XtNeditType, XawtextEdit,
					XtNstring,
					PCB->RouteStyle[index].Name,
					LAYOUT_TOP, NULL);
  XtInstallAccelerators (masterform, inputfield);

  /* create the sliders */
  last = inputfield;
  for (i = 0; i <= KEEPAWAY_SLIDER; i++)
    {
      label = XtVaCreateManagedWidget ("comment", labelWidgetClass,
				       masterform,
				       XtNfromVert, last,
				       XtNlabel, Sliders[i].Label,
				       LAYOUT_TOP, NULL);
      Sliders[i].Size = XtVaCreateManagedWidget ("size", labelWidgetClass,
						 masterform,
						 XtNfromVert, last,
						 XtNfromHoriz, label,
						 XtNresizable, True,
						 LAYOUT_TOP, NULL);
      last = XtVaCreateManagedWidget ("scrollbar", scrollbarWidgetClass,
				      masterform,
				      LAYOUT_TOP,
				      XtNfromVert, label,
				      XtNminimumThumb, THUMB_LENGTH,
				      XtNorientation, XtorientHorizontal,
				      XtNwidth, SCROLLBAR_LENGTH, NULL);
      Sliders[i].Scrollbar = last;

      /* add callbacks for scrolling */
      XtAddCallback (Sliders[i].Scrollbar,
		     XtNjumpProc, CB_JumpProc, &Sliders[i]);
      XtAddCallback (Sliders[i].Scrollbar,
		     XtNscrollProc, CB_ScrollProc, &Sliders[i]);
    }

  /* now that all sliders are initialized --> do an update */
  for (i = 0; i <= KEEPAWAY_SLIDER; i++)
    UpdateScrollbar (&Sliders[i], Sliders[i].Value);

  /* add the buttons and install accelerators for them;
   * the first one is always default
   */
  AddButtons (masterform, last, buttons, ENTRIES (buttons));
  XtInstallAccelerators (masterform, buttons[0].W);
  XtInstallAccelerators (masterform, buttons[1].W);

  /* now display dialog window */
  StartDialog (popup);
  DialogEventLoop (&ReturnCode);

  /* get settings */
  if (ReturnCode == OK_BUTTON)
    {
      PCB->RouteStyle[index].Thick = Sliders[LINE_SLIDER].Value;
      PCB->RouteStyle[index].Diameter = Sliders[VIASIZE_SLIDER].Value;
      PCB->RouteStyle[index].Hole = Sliders[VIADRILL_SLIDER].Value;
      PCB->RouteStyle[index].Keepaway = Sliders[KEEPAWAY_SLIDER].Value;
      XtVaGetValues (inputfield, XtNstring, &string, NULL);
      SaveFree (PCB->RouteStyle[index].Name);
      PCB->RouteStyle[index].Name = StripWhiteSpaceAndDup (string);
      UpdateSizesMenu ();
      SetChangedFlag (True);
    }
  EndDialog (popup);
}
