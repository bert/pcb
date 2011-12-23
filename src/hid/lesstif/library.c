#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "xincludes.h"

#include "compat.h"
#include "global.h"
#include "data.h"
#include "misc.h"
#include "set.h"
#include "buffer.h"

#include "hid.h"
#include "../hidint.h"
#include "lesstif.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

static Arg args[30];
static int n;
#define stdarg(t,v) XtSetArg(args[n], t, v); n++

static Widget library_dialog = 0;
static Widget library_list, libnode_list;

static XmString *library_strings = 0;
static XmString *libnode_strings = 0;
static int last_pick = -1;

static void
pick_net (int pick)
{
  LibraryMenuType *menu = Library.Menu + pick;
  int i;

  if (pick == last_pick)
    return;
  last_pick = pick;

  if (libnode_strings)
    free (libnode_strings);
  libnode_strings = (XmString *) malloc (menu->EntryN * sizeof (XmString));
  for (i = 0; i < menu->EntryN; i++)
    libnode_strings[i] = XmStringCreatePCB (menu->Entry[i].ListEntry);
  n = 0;
  stdarg (XmNitems, libnode_strings);
  stdarg (XmNitemCount, menu->EntryN);
  XtSetValues (libnode_list, args, n);
}

static void
library_browse (Widget w, void *v, XmListCallbackStruct * cbs)
{
  pick_net (cbs->item_position - 1);
}

static void
libnode_select (Widget w, void *v, XmListCallbackStruct * cbs)
{
  char *args;
  LibraryEntryType *e =
    Library.Menu[last_pick].Entry + cbs->item_position - 1;

  if (e->Template == (char *) -1)
    {
      if (LoadElementToBuffer (PASTEBUFFER, e->AllocatedMemory, true))
	SetMode (PASTEBUFFER_MODE);
      return;
    }
  args = Concat("'", EMPTY (e->Template), "' '",
		EMPTY (e->Value), "' '", EMPTY (e->Package), "'", NULL);
  if (LoadElementToBuffer (PASTEBUFFER, args, false))
    SetMode (PASTEBUFFER_MODE);
}

static int
build_library_dialog ()
{
  if (!mainwind)
    return 1;
  if (library_dialog)
    return 0;

  n = 0;
  stdarg (XmNresizePolicy, XmRESIZE_GROW);
  stdarg (XmNtitle, "Element Library");
  library_dialog = XmCreateFormDialog (mainwind, "library", args, n);

  n = 0;
  stdarg (XmNtopAttachment, XmATTACH_FORM);
  stdarg (XmNbottomAttachment, XmATTACH_FORM);
  stdarg (XmNleftAttachment, XmATTACH_FORM);
  stdarg (XmNvisibleItemCount, 10);
  library_list = XmCreateScrolledList (library_dialog, "nets", args, n);
  XtManageChild (library_list);
  XtAddCallback (library_list, XmNbrowseSelectionCallback,
		 (XtCallbackProc) library_browse, 0);

  n = 0;
  stdarg (XmNtopAttachment, XmATTACH_FORM);
  stdarg (XmNbottomAttachment, XmATTACH_FORM);
  stdarg (XmNrightAttachment, XmATTACH_FORM);
  stdarg (XmNleftAttachment, XmATTACH_WIDGET);
  stdarg (XmNleftWidget, library_list);
  libnode_list = XmCreateScrolledList (library_dialog, "nodes", args, n);
  XtManageChild (libnode_list);
  XtAddCallback (libnode_list, XmNbrowseSelectionCallback,
		 (XtCallbackProc) libnode_select, 0);

  return 0;
}

static int
LibraryChanged (int argc, char **argv, Coord x, Coord y)
{
  int i;
  if (!Library.MenuN)
    return 0;
  if (build_library_dialog ())
    return 0;
  last_pick = -1;
  if (library_strings)
    free (library_strings);
  library_strings = (XmString *) malloc (Library.MenuN * sizeof (XmString));
  for (i = 0; i < Library.MenuN; i++)
    library_strings[i] = XmStringCreatePCB (Library.Menu[i].Name);
  n = 0;
  stdarg (XmNitems, library_strings);
  stdarg (XmNitemCount, Library.MenuN);
  XtSetValues (library_list, args, n);
  pick_net (0);
  return 0;
}

static const char libraryshow_syntax[] =
"LibraryShow()";

static const char libraryshow_help[] =
"Displays the library window.";

/* %start-doc actions LibraryShow

%end-doc */

static int
LibraryShow (int argc, char **argv, Coord x, Coord y)
{
  if (build_library_dialog ())
    return 0;
  return 0;
}

void
lesstif_show_library ()
{
  if (mainwind)
    {
      if (!library_dialog)
	LibraryChanged (0, 0, 0, 0);
      XtManageChild (library_dialog);
    }
}

HID_Action lesstif_library_action_list[] = {
  {"LibraryChanged", 0, LibraryChanged,
   librarychanged_help, librarychanged_syntax},
  {"LibraryShow", 0, LibraryShow,
   libraryshow_help, libraryshow_syntax},
};

REGISTER_ACTIONS (lesstif_library_action_list)
