/* $Id$ */

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

#include "hid.h"
#include "../hidint.h"
#include "lesstif.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

static Arg args[30];
static int n;
#define stdarg(t,v) XtSetArg(args[n], t, v); n++

static Widget netlist_dialog = 0;
static Widget netlist_list, netnode_list;

static XmString *netlist_strings = 0;
static XmString *netnode_strings = 0;
static int last_pick = -1;

static void
pick_net (int pick)
{
  LibraryMenuType *menu = PCB->NetlistLib.Menu + pick;
  int i;

  if (pick == last_pick)
    return;
  last_pick = pick;

  if (netnode_strings)
    free (netnode_strings);
  netnode_strings = (XmString *) malloc (menu->EntryN * sizeof (XmString));
  for (i = 0; i < menu->EntryN; i++)
    netnode_strings[i] = XmStringCreateLocalized (menu->Entry[i].ListEntry);
  n = 0;
  stdarg (XmNitems, netnode_strings);
  stdarg (XmNitemCount, menu->EntryN);
  XtSetValues (netnode_list, args, n);
}

static void
netlist_browse (Widget w, void *v, XmListCallbackStruct * cbs)
{
  char *name = PCB->NetlistLib.Menu[cbs->item_position - 1].Name + 2;
  hid_actionl ("unselect", "all", NULL);
  hid_actionl ("netlist", "select", name, NULL);
  pick_net (cbs->item_position - 1);
}

static void
netlist_select (Widget w, void *v, XmListCallbackStruct * cbs)
{
  char *name = PCB->NetlistLib.Menu[cbs->item_position - 1].Name + 2;
  hid_actionl ("connection", "reset", NULL);
  hid_actionl ("netlist", "find", name, NULL);
}

static void
netlist_extend (Widget w, void *v, XmListCallbackStruct * cbs)
{
  char *name;
  int i;
  hid_actionl ("connection", "reset", NULL);
  for (i=0; i<cbs->selected_item_count; i++)
    {
      name = PCB->NetlistLib.Menu[cbs->selected_item_positions[i]-1].Name + 2;
      hid_actionl ("netlist", "select", name, NULL);
    }
}

static int
build_netlist_dialog ()
{
  if (!mainwind)
    return 1;
  if (netlist_dialog)
    return 0;

  n = 0;
  stdarg (XmNresizePolicy, XmRESIZE_GROW);
  stdarg (XmNtitle, "Netlists");
  netlist_dialog = XmCreateFormDialog (mainwind, "netlist", args, n);

  n = 0;
  stdarg (XmNtopAttachment, XmATTACH_FORM);
  stdarg (XmNbottomAttachment, XmATTACH_FORM);
  stdarg (XmNleftAttachment, XmATTACH_FORM);
  stdarg (XmNrightAttachment, XmATTACH_POSITION);
  stdarg (XmNrightPosition, 50);
  stdarg (XmNvisibleItemCount, 10);
  stdarg (XmNselectionPolicy, XmEXTENDED_SELECT);
  netlist_list = XmCreateScrolledList (netlist_dialog, "nets", args, n);
  XtManageChild (netlist_list);
  XtAddCallback (netlist_list, XmNbrowseSelectionCallback, (XtCallbackProc)netlist_browse, 0);
  XtAddCallback (netlist_list, XmNdefaultActionCallback, (XtCallbackProc)netlist_select, 0);
  XtAddCallback (netlist_list, XmNextendedSelectionCallback, (XtCallbackProc)netlist_extend, 0);

  n = 0;
  stdarg (XmNtopAttachment, XmATTACH_FORM);
  stdarg (XmNbottomAttachment, XmATTACH_FORM);
  stdarg (XmNrightAttachment, XmATTACH_FORM);
  stdarg (XmNleftAttachment, XmATTACH_POSITION);
  stdarg (XmNleftPosition, 50);
  netnode_list = XmCreateScrolledList (netlist_dialog, "nodes", args, n);
  XtManageChild (netnode_list);

  return 0;
}

static int
NetlistChanged (int argc, char **argv, int x, int y)
{
  int i;
  if (!PCB->NetlistLib.MenuN)
    return 0;
  if (build_netlist_dialog ())
    return 0;
  last_pick = -1;
  if (netlist_strings)
    free (netlist_strings);
  netlist_strings =
    (XmString *) malloc (PCB->NetlistLib.MenuN * sizeof (XmString));
  for (i = 0; i < PCB->NetlistLib.MenuN; i++)
    netlist_strings[i] =
      XmStringCreateLocalized (PCB->NetlistLib.Menu[i].Name);
  n = 0;
  stdarg (XmNitems, netlist_strings);
  stdarg (XmNitemCount, PCB->NetlistLib.MenuN);
  XtSetValues (netlist_list, args, n);
  pick_net (0);
  return 0;
}

static const char netlistshow_syntax[] =
"NetlistShow()";

static const char netlistshow_help[] =
"Displays the netlist window.";

/* %start-doc actions NetlistShow

%end-doc */

static int
NetlistShow (int argc, char **argv, int x, int y)
{
  if (build_netlist_dialog ())
    return 0;
  return 0;
}

void
lesstif_show_netlist ()
{
  build_netlist_dialog ();
  XtManageChild (netlist_dialog);
}

HID_Action lesstif_netlist_action_list[] = {
  {"NetlistChanged", 0, NetlistChanged,
   netlistchanged_help, netlistchanged_syntax},
  {"NetlistShow", 0, NetlistShow,
   netlistshow_help, netlistshow_syntax}
};

REGISTER_ACTIONS (lesstif_netlist_action_list)
