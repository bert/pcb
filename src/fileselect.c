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


/* file select box
 * some of the actions are local to this module
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <dirent.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "global.h"

#include "data.h"
#include "dialog.h"
#include "error.h"
#include "fileselect.h"
#include "mymem.h"
#include "misc.h"
#include "selector.h"

#include <X11/Shell.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID("$Id$");


/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static int ChangeDirectory (char *);
static void SetInputWidget (String);
static void CB_CancelOrOK (Widget, XtPointer, XtPointer);
static void CB_Menu (Widget, XtPointer, XtPointer);
static void CB_Directory (Widget, XtPointer, XtPointer);
static void CB_File (Widget, XtPointer, XtPointer);
static void CB_Text (Widget, XtPointer, XtPointer);
static int FillLists (char *);
static int FillListsFromCommand (char *);
static int FillListsFromDirectory (char *);
static Widget CreateMenuFromPath (Widget, Widget, Widget, char *);

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static Widget InputW,		/* input field */
  CurrentW;			/* label (current directory) */
static long int ReturnCode;	/* returncode of buttons */
static char CurrentDir[MAXPATHLEN + 1];	/* current directory */
static Boolean LockCallback;	/* used by CB_Text() */
static SelectorType DirectorySelector =
  { "directoryList", NULL, NULL, CB_Directory,
  (XtPointer) & DirectorySelector,
  0, 0, NULL, NULL
};
static SelectorType FileSelector =
  { "fileList", NULL, NULL, CB_File, (XtPointer) & FileSelector,
  0, 0, NULL, NULL
};
static DialogButtonType Buttons[] = {
  {"defaultButton", "   OK   ", CB_CancelOrOK, (XtPointer) OK_BUTTON, NULL},
  {"cancelButton", "No/Cancel", CB_CancelOrOK, (XtPointer) CANCEL_BUTTON, NULL}
};

/* ----------------------------------------------------------------------
 * changes current directory and prints an error message if failed
 */
static int
ChangeDirectory (char *DirName)
{
  if (chdir (DirName))
    {
      ChdirErrorMessage (DirName);
      return (1);
    }
  return (0);
}

/* ---------------------------------------------------------------------------
 * set contents of input widget to string
 * replace old string
 * this way resizes the widget if necessary
 */
static void
SetInputWidget (String S)
{
  Widget source = XawTextGetSource (InputW);
  XawTextPosition last;
  XawTextBlock block;

  /* get last character position */
  last = XawTextSourceScan (source, 0, XawstAll, XawsdRight, 1, True);
  block.ptr = S;
  block.firstPos = 0;
  block.length = strlen (S);
  block.format = FMT8BIT;

  /* set 'lock' flag for next call of CB_Text() */
  LockCallback = True;

  /* replace contents by new string */
  XawTextReplace (InputW, 0, last, &block);
  XawTextSetInsertionPoint (InputW, block.length);
}

/* ---------------------------------------------------------------------------
 * callback function for OK and cancel button
 * also called via accelerators from a double click
 * ClientData identifies the button
 */
static void
CB_CancelOrOK (Widget W, XtPointer ClientData, XtPointer CallData)
{
  ReturnCode = (long int) ClientData;
}

/* ---------------------------------------------------------------------------
 * callback function for rolldown menus
 * directory name is passed as ClientData
 */
static void
CB_Menu (Widget W, XtPointer ClientData, XtPointer CallData)
{
  FillLists ((char *) ClientData);
}

/* ---------------------------------------------------------------------------
 * callback function for directory list
 * gets current selection and updates the list widgets
 */
static void
CB_Directory (Widget W, XtPointer ClientData, XtPointer CallData)
{
  char newdir[MAXPATHLEN + 1];
  XawListReturnStruct *selected = XawListShowCurrent (W);

  /* abort if nothing has been selected or if the selected thing is
   * an external command
   */
  if (selected->list_index != XAW_LIST_NONE && *selected->string != '|')
    {
      /* create complete new path; update list */
      sprintf (newdir, "%s/%s", CurrentDir, selected->string);
      FillLists (newdir);
    }
}

/* ---------------------------------------------------------------------------
 * callback function for file list
 * gets current selection and updates 'input widget'
 */
static void
CB_File (Widget W, XtPointer ClientData, XtPointer CallData)
{
  XawListReturnStruct *selected = XawListShowCurrent (W);

  if (selected->list_index != XAW_LIST_NONE)
    SetInputWidget (selected->string);
}

/* ---------------------------------------------------------------------------
 * callback function for input text widget
 * invalidates file selections whenever the buffer changes
 * we have to prevent from unhighlighting the selection that has
 * just updated the buffer by evaluating the 'LockCallback' flag
 */
static void
CB_Text (Widget W, XtPointer ClientData, XtPointer CallData)
{
  if (!LockCallback)
    XawListUnhighlight (FileSelector.ListW);
  else
    LockCallback = False;
}

/* ---------------------------------------------------------------------------
 * reads contents of the passed directory into the list
 * or calls an external command to create the list
 */
static int
FillLists (char *DirName)
{
  if ((*DirName == '|' && !FillListsFromCommand (DirName)) ||
      !FillListsFromDirectory (DirName))
    {
      /* sort lists and update them */
      UpdateSelector (&DirectorySelector);
      UpdateSelector (&FileSelector);

      /* clear input field; update label */
#if 0
      XtVaSetValues (InputW, XtNstring, "", NULL);
#endif
      XtVaSetValues (CurrentW, XtNlabel, CurrentDir, NULL);
      return (0);
    }
  return (1);
}

/* ----------------------------------------------------------------------
 * calls an external command and read its stdout into the list widget;
 * external commands starts with '|'
 */
static int
FillListsFromCommand (char *Command)
{
  FILE *fp;
  char line[256];
  int i;

  /* ignore leading '|', execute command */
  if ((fp = popen (Command + 1, "r")) == NULL)
    {
      PopenErrorMessage (Command + 1);
      return (1);
    }

  FreeSelectorEntries (&FileSelector);
  FreeSelectorEntries (&DirectorySelector);
  while (fgets (line, 256, fp))
    {
      /* ignore newline and carriage return */
      for (i = strlen (line) - 1; i >= 0; i--)
	if (line[i] == '\n' || line[i] == '\r')
	  line[i] = '\0';
	else
	  break;
      AddEntryToSelector (line, NULL, &FileSelector);
    }

  if (pclose (fp))
    {
      FreeSelectorEntries (&FileSelector);
      return (1);
    }

  /* update label */
  strncpy (CurrentDir, Command, sizeof (CurrentDir) - 1);
  XtVaSetValues (CurrentW, XtNlabel, Command, NULL);
  return (0);
}

/* ---------------------------------------------------------------------------
 * reads contents of the passed directory into the list
 */
static int
FillListsFromDirectory (char *DirName)
{
  DIR *dir;
  struct dirent *direntry;
  struct stat buffer;

  /* open directory */
  if (ChangeDirectory (DirName))
    return (1);
  GetWorkingDirectory (CurrentDir);
  if ((dir = opendir (CurrentDir)) == NULL)
    {
      OpendirErrorMessage (CurrentDir);
      return (1);
    }

  /* release memory */
  FreeSelectorEntries (&DirectorySelector);
  FreeSelectorEntries (&FileSelector);

  /* read all entries */
  while ((direntry = readdir (dir)) != NULL)
    {
      /* check if entry is directory ...
       * ignore entry on error
       */
      if (!stat (direntry->d_name, &buffer))
	{
	  /* add entry to corresponding list */
	  if (S_ISDIR (buffer.st_mode))
	    {
	      /* ignore '.' entry */
	      if (NSTRCMP (direntry->d_name, "."))
		{
		  char name[MAXPATHLEN + 2];

		  sprintf (name, "%s/", direntry->d_name);
		  AddEntryToSelector (name, NULL, &DirectorySelector);
		}
	    }
	  else
	    AddEntryToSelector (direntry->d_name, NULL, &FileSelector);
	}
    }
  closedir (dir);

  /* add at least '..' just in case the directory isn't readable */
  if (!DirectorySelector.Number)
    AddEntryToSelector ("..", NULL, &DirectorySelector);
  if (!FileSelector.Number)
    AddEntryToSelector ("<no files>", NULL, &FileSelector);

  return (0);
}

/* ----------------------------------------------------------------------
 * creates menu-button with entries from PATH information
 ' 'copy' needs to be static because a pointer to that memory location
 * is returned
 */
static Widget
CreateMenuFromPath (Widget Parent, Widget Top, Widget Left, char *Path)
{
  Widget popup, menubutton, entry;
  char *path;
  Boolean first;
  static char *copy;

  /* create button */
  menubutton = XtVaCreateManagedWidget ("preset", menuButtonWidgetClass,
					Parent,
					LAYOUT_TOP,
					XtNfromVert, Top,
					XtNfromHoriz, Left,
					XtNlabel,
					"click here for preset directories",
					XtNmenuName, "preset", NULL);

  /* create a copy of the path; used for passing to callback */
  MyFree (&copy);
  copy = MyStrdup (Path, "CreateMenuFromPath()");

  /* append menu with paths */
  popup = XtVaCreatePopupShell ("preset", simpleMenuWidgetClass,
				menubutton,
				XtNlabel, NULL, XtNsensitive, True, NULL);
  for (first = True, path = strtok (copy, ":"); path;
       path = strtok (NULL, ":"))
    {
      /* copy first path element to CurrentDir */
      if (first)
	{
	  strncpy (CurrentDir, path, sizeof (CurrentDir) - 1);
	  first = False;
	}
      entry = XtVaCreateManagedWidget ("directory", smeBSBObjectClass,
				       popup,
				       XtNlabel, path,
				       XtNsensitive, True, NULL);
      XtAddCallback (entry, XtNcallback, CB_Menu, (XtPointer) path);
    }
  return (menubutton);
}

/* ---------------------------------------------------------------------------
 * file select box
 * a static pointer to the selected file is returned -->
 * it's only valid till it's called again
 * the directories in the passed path are available from a menu
 */
char *
FileSelectBox (char *MessageText, char *DefaultFile, char *Path)
{
  Widget popup, masterform, last;
  char currentdir[MAXPATHLEN + 1], tempPath[MAXPATHLEN + 1], *file, *file2;
  static char result[MAXPATHLEN + 1];

  /* save current directory */
  GetWorkingDirectory (currentdir);

  /* create the popup shell */
  popup = XtVaCreatePopupShell ("popup", transientShellWidgetClass,
				Output.Toplevel,
				XtNtransientFor, Output.Toplevel,
				XtNallowShellResize, True,
				XtNmappedWhenManaged, False,
				XtNinput, True, NULL);

  /* the form that holds everything */
  masterform =
    XtVaCreateManagedWidget ("fileselectMasterForm", formWidgetClass, popup,
			     XtNresizable, True, NULL);

  /* the line which displays the some text */
  last = XtVaCreateManagedWidget ("comment", labelWidgetClass,
				  masterform,
				  LAYOUT_TOP, XtNlabel, MessageText, NULL);

  /* extract any path off default file */
  for (file = NULL, file2 = DefaultFile; file2 && *file2; file2++)
    if (*file2 == '/')
      file = file2;
  file2 = NULL;
  if (file)
    {
      for (file2 = tempPath; DefaultFile != file; DefaultFile++, file2++)
	*file2 = *DefaultFile;
      *file2 = '\0';
      if (NSTRCMP (tempPath, currentdir) != 0)
	{
	  *file2++ = ':';
	  for (file = currentdir; file && *file; file++)
	    *file2++ = *file;
	}
      if (NSTRCMP (Path, "."))
	for (file = Path; file && *file; file++)
	  {
	    if (file == Path)
	      *file2++ = ':';
	    *file2++ = *file;
	  }
      *file2 = '\0';
    }
  /* create a menu button for preset directories, also
   * initializes 'CurrentDir'
   */
  last = CreateMenuFromPath (masterform, last, NULL,
			     file2 ? tempPath : (Path && *Path) ? Path : ".");
  /* the line which displays the current directory */
  CurrentW = XtVaCreateManagedWidget ("current", labelWidgetClass,
				      masterform,
				      XtNresizable, True,
				      XtNfromVert, last, LAYOUT_TOP, NULL);

  InputW = XtVaCreateManagedWidget ("input", asciiTextWidgetClass,
				    masterform,
				    XtNresizable, True,
				    XtNresize, XawtextResizeWidth,
				    XtNwrap, XawtextWrapNever,
				    XtNeditType, XawtextEdit,
				    XtNfromVert, CurrentW, LAYOUT_TOP, NULL);

  XtAddCallback (XawTextGetSource (InputW), XtNcallback, CB_Text, NULL);

  /* the two selectors and the buttons */
  last = CreateSelector (masterform, InputW, NULL, &DirectorySelector, 1);
  last = CreateSelector (masterform, InputW, last, &FileSelector, 1);
  AddButtons (masterform, last, Buttons, ENTRIES (Buttons));

  /* override the translations for the input widget and
   * install accelerators for the buttons
   * !!! first translations, second accelerators !!!
   * additionaly ignore 'spaces' and 'slashes' during input
   */
  XtOverrideTranslations (InputW,
			  XtParseTranslationTable (InputTranslations));
  XtOverrideTranslations (InputW,
			  XtParseTranslationTable
			  ("<Key>space: no-op()\n <Key>slash: no-op()\n"));
  XtInstallAccelerators (InputW, Buttons[0].W);
  XtInstallAccelerators (InputW, Buttons[1].W);

  /* set keyboard focus to input widget */
  XtSetKeyboardFocus (masterform, InputW);

  /* install additional accelerators for the default button;
   * double click to select
   * install translation so that selection still works
   */
  XtOverrideTranslations (FileSelector.ListW,
			  XtParseTranslationTable
			  ("<Btn1Down>: Set() Notify()\n"));
  XtInstallAccelerators (FileSelector.ListW, Buttons[0].W);

  /* read directory structure of current directory
   * which was set to the first menu entry from CreateMenuFromPath()
   * set text widget to default and reset lock-flag
   */
  FillLists (CurrentDir);
  SetInputWidget (DefaultFile ? DefaultFile : "");
  LockCallback = False;

  /* now display dialog and wait for completion */
  StartDialog (popup);
  DialogEventLoop (&ReturnCode);

  /* evaluate selection */
  *result = '\0';
  if (ReturnCode == OK_BUTTON)
    {
      XtVaGetValues (InputW, XtNstring, &file, NULL);

      /* append result to current directory */
      if (file && *file)
	{
	  if (*CurrentDir != '|')
	    {
	      sprintf (result, "%s/%s", CurrentDir, file);
	    }
	  else
	    strcpy (result, file);
	}
    }

  /* remove event handler, dialog, release resources and
   * change back to initial directory
   */
  EndDialog (popup);
  FreeSelectorEntries (&DirectorySelector);
  FreeSelectorEntries (&FileSelector);
  ChangeDirectory (currentdir);

  return (*result ? result : NULL);
}
