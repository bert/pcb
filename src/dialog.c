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

/* dialog routines
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "global.h"

#include "data.h"
#include "dialog.h"
#include "error.h"
#include "mymem.h"
#include "misc.h"

#include <X11/Shell.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Toggle.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * include the icon data
 */
#include "icon.data"

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static long int ReturnCode;	/* used by standard dialogs */

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void CB_OK (Widget, XtPointer, XtPointer);
static void CenterDialog (Widget, Boolean, Boolean);
static void SendEnterNotify (void);

/* ---------------------------------------------------------------------------
 * callback for standard dialog
 * just copies the button-code which is passed as ClientData to a
 * public identifier
 */
static void
CB_OK (Widget W, XtPointer ClientData, XtPointer CallData)
{
  ReturnCode = (long int) ClientData;
}

/* ---------------------------------------------------------------------------
 * centers the widget around the current cursor position
 */
static void
CenterDialog (Widget Popup, Boolean CenterX, Boolean CenterY)
{
  Window root, child;
  int root_x, root_y, child_x, child_y;
  unsigned int mask;
  Dimension width, height;

  /* get current pointer position relativ to it's parent */
  XQueryPointer (Dpy, XtWindow (Popup),
		 &root, &child, &root_x, &root_y, &child_x, &child_y, &mask);

  /* get the dialogs size */
  XtVaGetValues (Popup, XtNheight, &height, XtNwidth, &width, NULL);

  /* make sure position is inside our screen */
  if (CenterX)
    {
      if ((root_x -= (width / 2)) < 0)
	{
	  root_x = 0;
	}
      else
	{
	  if (root_x > WidthOfScreen (XtScreen (Popup)) - width)
	    root_x = WidthOfScreen (XtScreen (Popup)) - width;
	}
    }

  if (CenterY)
    {
      if ((root_y -= (height / 2)) < 0)
	{
	  root_y = 0;
	}
      else
	{
	  if (root_y > HeightOfScreen (XtScreen (Popup)) - height)
	    root_y = HeightOfScreen (XtScreen (Popup)) - height;
	}
    }

  XtVaSetValues (Popup, XtNx, root_x, XtNy, root_y, NULL);
}


/* ---------------------------------------------------------------------------
 * waits until returncode is differnt from -1
 * see also CB_OK()
 */
long int
DialogEventLoop (long int *Code)
{
  XEvent event;

  for (*Code = -1; *Code == -1;)
    {
      XtAppNextEvent (Context, &event);
      /* exit if window manager says delete window */
      if (event.type == ClientMessage)
	return (*Code);
      XtDispatchEvent (&event);
    }
  return (*Code);
}

/* ---------------------------------------------------------------------------
 * adds the buttons to a dialog
 */
Widget
AddButtons (Widget Parent, Widget Top,
	    DialogButtonTypePtr Buttons, size_t Count)
{
  int i;

  for (i = 0; i < Count; i++)
    {
      /* skip button if there's no label,
       * used for dialogs without default button
       */
      if (!Buttons[i].Label)
	continue;
      Buttons[i].W = XtVaCreateManagedWidget (Buttons[i].Name,
					      commandWidgetClass,
					      Parent,
					      XtNlabel, Buttons[i].Label,
					      XtNfromHoriz,
					      i ? Buttons[i - 1].W : NULL,
					      XtNfromVert, Top, XtNresizable,
					      True, NULL);

      XtAddCallback (Buttons[i].W,
		     XtNcallback, Buttons[i].Callback, Buttons[i].ClientData);
    }
  return (Buttons[Count - 1].W);
}

/* ---------------------------------------------------------------------------
 * thanks to Ellen M. Sentovich and Rick L. Spickelm for their hint in
 * xrn
 *
 * creates a dialog box as a child of ... (see arguments)
 */
Widget
CreateDialogBox (char *LabelText,
		 DialogButtonTypePtr Buttons, size_t Count, char *title)
{
  Widget dialog, popup;

  /* create the popup shell */
  popup = XtVaCreatePopupShell (title, transientShellWidgetClass,
				Output.Toplevel,
				XtNinput, True,
				XtNallowShellResize, True, NULL);
  dialog = XtVaCreateManagedWidget ("dialog", dialogWidgetClass,
				    popup, XtNlabel, LabelText, NULL);

  /* now we add the buttons and install the accelerators */
  AddButtons (dialog, NULL, Buttons, Count);
  while (Count--)
    XtInstallAccelerators (dialog, Buttons[Count].W);
  return (popup);
}

/* ---------------------------------------------------------------------------
 * pops up a dialog
 * saves output window in pixmap
 * first realize the dialog and
 * focus keyboard events from the application to it
 */
void
StartDialog (Widget Popup)
{
  Atom protocol;

  XtRealizeWidget (Popup);
  CenterDialog (Popup, True, True);
  XtPopup (Popup, XtGrabExclusive);
  protocol = XInternAtom (Dpy, "WM_DELETE_WINDOW", True);
  if (protocol != None)
    XSetWMProtocols (Dpy, XtWindow (Popup), &protocol, 1);
}

/* ---------------------------------------------------------------------------
 * removes a dialog from screen and from memory
 * pixmap is released by next dialog
 */
void
EndDialog (Widget Popup)
{
  XtPopdown (Popup);
  XtDestroyWidget (Popup);
  XFlush (Dpy);
  XSync (Dpy, False);
}

/* ---------------------------------------------------------------------------
 * prints out a warning
 */
void
MessageDialog (char *MessageText)
{
  Widget popup;
  static DialogButtonType buttons[] = {
    {"defaultButton", "  OK  ", CB_OK, (XtPointer) OK_BUTTON, NULL}
  };

  /* create dialog box */
  popup = CreateDialogBox (MessageText, buttons, ENTRIES (buttons), "popup");
  StartDialog (popup);

  /* wait for dialog to complete */
  DialogEventLoop (&ReturnCode);
  EndDialog (popup);
}

/* ---------------------------------------------------------------------------
 * displays a single line prompt (message)
 */
void
MessagePrompt (char *MessageText)
{
  /* set some resources for the input fields */
  if (Output.Message && (!MessageText || !*MessageText))
    {
      XUnmapWindow (Dpy, XtWindow (Output.Message));
      XMapWindow (Dpy, XtWindow (Output.StatusLine));
      Output.Message = NULL;
    }
  else
    {
      XUnmapWindow (Dpy, XtWindow (Output.StatusLine));

      /* the message label */
      Output.Message =
	XtVaCreateManagedWidget ("messageText", labelWidgetClass,
				 Output.MasterForm, XtNlabel,
				 EMPTY (MessageText), XtNfromHoriz,
				 Output.Control, XtNfromVert, Output.Porthole,
				 LAYOUT_BOTTOM, NULL);
    }
}

/* ---------------------------------------------------------------------------
 * called by action ActionFinishInputDialog() to indicate that a user input
 * is finished. The passed parameter signals either OK (True) or Cancel
 */
void
FinishInputDialog (Boolean OK)
{
  ReturnCode = OK ? OK_BUTTON : CANCEL_BUTTON;
}

/* ---------------------------------------------------------------------------
 * since all my dialogs are modal the X server wont create 'EnterNotify'
 * events during the dialog is popped up. This breaks the crosshair
 * visibility management. Therefore I check if the pointer is inside
 * the output window and generate a 'EnterNotify' event.
 */
static void
SendEnterNotify (void)
{
  XEnterWindowEvent event;
  Window root, child;
  int root_x, root_y, child_x, child_y;
  unsigned int mask;

  if (XQueryPointer (Dpy, Output.OutputWindow, &root, &child,
		     &root_x, &root_y, &child_x, &child_y, &mask))
    {
      event.type = EnterNotify;
      event.display = Dpy;
      event.window = Output.OutputWindow;
      event.root = root;
      XSendEvent (Dpy, Output.OutputWindow, True,
		  EnterWindowMask, (XEvent *) & event);
    }
}

/* ---------------------------------------------------------------------------
 * gets a string from user, memory is allocated for the string,
 * the user is responsible for releasing the allocated memory
 * string might be empty if flag is set
 */
char *
GetUserInput (char *MessageText, char *OutputString)
{
  char *string;
  Widget inputfield;
  Dimension messageWidth, viewportWidth;

  /* display single line message */
  MessagePrompt (MessageText);

  /* calculate size */
  XtVaGetValues (Output.Message, XtNwidth, &messageWidth, NULL);
  XtVaGetValues (Output.Porthole, XtNwidth, &viewportWidth, NULL);

  /* the input field itself */
  inputfield = XtVaCreateManagedWidget ("inputField", asciiTextWidgetClass,
					Output.MasterForm,
					XtNresizable, True,
					XtNeditType, XawtextEdit,
					XtNfromHoriz, Output.Message,
					XtNfromVert, Output.Porthole,
					XtNstring, EMPTY (OutputString),
					XtNwidth, MAX (100,
						       viewportWidth -
						       messageWidth),
					LAYOUT_BOTTOM_RIGHT, NULL);

  /* set focus to input field, override default translations
   * and install accelerators
   * grap all events --> make widget modal
   */
  XtSetKeyboardFocus (Output.Toplevel, inputfield);
  XtOverrideTranslations (inputfield,
			  XtParseTranslationTable (InputTranslations));
  XtInstallAccelerators (inputfield, Output.MasterForm);
  XtAddGrab (inputfield, True, False);
  XtRealizeWidget (inputfield);

  /* wait for input to complete and allocate memory if necessary */
  if (DialogEventLoop (&ReturnCode) == OK_BUTTON)
    {
      /* strip white space and return string */
      XtVaGetValues (inputfield, XtNstring, &string, NULL);
      string = StripWhiteSpaceAndDup (string);
    }
  else
    string = NULL;

  /* restore normal outfit */
  XtRemoveGrab (inputfield);
  XtDestroyWidget (inputfield);
  MessagePrompt (NULL);

  /* force an 'EnterNotify' event if the pointer has moved
   * into the output area
   */
  SendEnterNotify ();
  XSync (Dpy, False);
  return (string);
}

/* ---------------------------------------------------------------------------
 * pops up and 'About' dialog
 */
void
AboutDialog (void)
{
  Widget popup, dialog;
  Pixmap icon = BadAlloc;
  static DialogButtonType button =
    { "defaultButton", "  OK  ", CB_OK, (XtPointer) OK_BUTTON, NULL };

  /* create dialog box */
  popup = CreateDialogBox ("This is PCB, an interactive\n"
			   "printed circuit board editor\n"
			   "version " VERSION "\n\n"
			   "by harry eaton\n\n"
			   "Copyright (C) Thomas Nau 1994, 1995, 1996, 1997\n"
			   "Copyright (C) harry eaton 1998-2004\n"
			   "Copyright (C) C. Scott Ananian 2001\n\n"
			   "It is licensed under the terms of the GNU\n"
			   "General Public License version 2\n"
			   "See the LICENSE file for more information\n\n"
			   "If you have problems, hints or\n"
			   "suggestions, send mail to:\n"
			   "bumpelo at comcast dot net\n\n",
			   &button, 1, "About PCB");
  if ((dialog = XtNameToWidget (popup, "dialog")) != NULL)
    {
      Screen *screen = XtScreen (Output.Toplevel);

      if ((icon = XCreatePixmapFromBitmapData (Dpy,
					       XtWindow (Output.Toplevel),
					       icon_bits, icon_width,
					       icon_height,
					       BlackPixelOfScreen (screen),
					       WhitePixelOfScreen (screen),
					       DefaultDepthOfScreen (screen)))
	  != BadAlloc)
	{
	  XtVaSetValues (dialog, XtNicon, icon, NULL);
	}
    }

  StartDialog (popup);

  /* wait for dialog to complete */
  DialogEventLoop (&ReturnCode);
  EndDialog (popup);
  if (icon != BadAlloc)
    XFreePixmap (Dpy, icon);
}

/* ----------------------------------------------------------------------
 * confirmation dialog for replacing existing files
 * the 'ALL' button is used for a global OK
 */
int
ConfirmReplaceFileDialog (char *MessageText, Boolean AllButton)
{
  Widget popup;
  static DialogButtonType buttons[] = {
    {"defaultButton", " No/Cancel ", CB_OK, (XtPointer) CANCEL_BUTTON, NULL},
    {"okButton", "    OK    ", CB_OK, (XtPointer) OK_BUTTON, NULL},
    {"okButton", "Sequence OK", CB_OK, (XtPointer) ALL_BUTTON, NULL}
  };

  /* create dialog box */
  popup = CreateDialogBox (MessageText, buttons,
			   ENTRIES (buttons) - (AllButton ? 0 : 1),
			   "Confirm");
  StartDialog (popup);

  /* wait for dialog to complete */
  DialogEventLoop (&ReturnCode);
  EndDialog (popup);
  return (ReturnCode);
}

/* ----------------------------------------------------------------------
 * confirmation dialog
 */
Boolean
ConfirmDialog (char *MessageText)
{
  Widget popup;
  static DialogButtonType buttons[] = {
    {"defaultButton", "No/Cancel", CB_OK, (XtPointer) CANCEL_BUTTON, NULL},
    {"okButton", "   OK   ", CB_OK, (XtPointer) OK_BUTTON, NULL}
  };

  /* create dialog box */
  popup =
    CreateDialogBox (MessageText, buttons, ENTRIES (buttons), "Confirm");
  StartDialog (popup);

  /* wait for dialog to complete */
  DialogEventLoop (&ReturnCode);
  EndDialog (popup);
  return (ReturnCode == OK_BUTTON);
}
