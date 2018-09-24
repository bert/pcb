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

#include "find.h"
#include "flags.h"
#include "rats.h"
#include "select.h"
#include "undo.h"
#include "remove.h"
#include "crosshair.h"
#include "draw.h"

#include "hid.h"
#include "../hidint.h"
#include "lesstif.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

static Arg args[30];
static int n;
#define stdarg(t,v) XtSetArg(args[n], t, v); n++

static Widget netlist_dialog = 0;
static Widget netlist_list, netnode_list;

static XmString *netlist_strings = 0;
static XmString *netnode_strings = 0;
static int n_netnode_strings;
static int last_pick = -1;

static int LesstifNetlistChanged (int argc, char **argv, Coord x, Coord y);

static void
pick_net (int pick)
{
  LibraryMenuType *menu = PCB->NetlistLib.Menu + pick;
  int i;

  if (pick == last_pick)
    return;
  last_pick = pick;

  if (netnode_strings)
    free (netnode_strings);	/* XXX leaked all XmStrings??? */
  n_netnode_strings = menu->EntryN;
  netnode_strings = (XmString *) malloc (menu->EntryN * sizeof (XmString));
  for (i = 0; i < menu->EntryN; i++)
    netnode_strings[i] = XmStringCreatePCB (menu->Entry[i].ListEntry);
  n = 0;
  stdarg (XmNitems, netnode_strings);
  stdarg (XmNitemCount, menu->EntryN);
  XtSetValues (netnode_list, args, n);
}

static void
netlist_select (Widget w, void *v, XmListCallbackStruct * cbs)
{
  XmString str;
  int pos = cbs->item_position;
  LibraryMenuType *net = & (PCB->NetlistLib.Menu[pos - 1]);
  char *name = net->Name;
  if (name[0] == ' ')
    {
      name[0] = '*';
      net->flag = 0;
    }
  else
    {
      name[0] = ' ';
      net->flag = 1;
    }

  str = XmStringCreatePCB (name);
  XmListReplaceItemsPos (netlist_list, &str, 1, pos);
  XmStringFree (str);
  XmListSelectPos (netlist_list, pos, False);
}

static void
netlist_extend (Widget w, void *v, XmListCallbackStruct * cbs)
{
  if (cbs->selected_item_count == 1)
    pick_net (cbs->item_position - 1);
}

typedef void (*Std_Nbcb_Func)(LibraryMenuType *, int);

static void
nbcb_rat_on (LibraryMenuType *net, int pos)
{
  XmString str;
  char *name = net->Name;
  name[0] = ' ';
  net->flag = 1;
  str = XmStringCreatePCB (name);
  XmListReplaceItemsPos (netlist_list, &str, 1, pos);
  XmStringFree (str);
}

static void
nbcb_rat_off (LibraryMenuType *net, int pos)
{
  XmString str;
  char *name = net->Name;
  name[0] = '*';
  net->flag = 0;
  str = XmStringCreatePCB (name);
  XmListReplaceItemsPos (netlist_list, &str, 1, pos);
  XmStringFree (str);
}


/* Select on the layout the current net treeview selection
 */
static void
nbcb_select_common (LibraryMenuType *net, int pos, int select_flag)
{
  LibraryEntryType *entry;
  ConnectionType conn;
  int i;

  InitConnectionLookup ();
  ClearFlagOnAllObjects (true, FOUNDFLAG);

  for (i = net->EntryN, entry = net->Entry; i; i--, entry++)
    if (SeekPad (entry, &conn, false))
      RatFindHook (conn.type, conn.ptr1, conn.ptr2, conn.ptr2, true, FOUNDFLAG, true);

  SelectByFlag (FOUNDFLAG, select_flag);
  ClearFlagOnAllObjects (false, FOUNDFLAG);
  FreeConnectionLookupMemory ();
  IncrementUndoSerialNumber ();
  Draw ();
}

static void
nbcb_select (LibraryMenuType *net, int pos)
{
  nbcb_select_common (net, pos, 1);
}

static void
nbcb_deselect (LibraryMenuType *net, int pos)
{
  nbcb_select_common (net, pos, 0);
}

static void
nbcb_find (LibraryMenuType *net, int pos)
{
  char *name = net->Name + 2;
  hid_actionl ("netlist", "find", name, NULL);
}

static void
nbcb_std_callback (Widget w, Std_Nbcb_Func v, XmPushButtonCallbackStruct * cbs)
{
  int *posl, posc, i;
  XmString **items, **selected;
  if (XmListGetSelectedPos (netlist_list, &posl, &posc) == False)
    return;
  if (v == nbcb_find)
    hid_actionl ("connection", "reset", NULL);
  for (i=0; i<posc; i++)
    {
      LibraryMenuType *net = & (PCB->NetlistLib.Menu[posl[i] - 1]);
      v(net, posl[i]);
    }
  n = 0;
  stdarg (XmNitems, &items);
  XtGetValues (netlist_list, args, n);
  selected = (XmString **) malloc (posc * sizeof (XmString *));
  for (i=0; i<posc; i++)
    selected[i] = items[posl[i]-1];

  n = 0;
  stdarg (XmNselectedItems, selected);
  XtSetValues (netlist_list, args, n);
}

static void
nbcb_ripup (Widget w, Std_Nbcb_Func v, XmPushButtonCallbackStruct * cbs)
{
  nbcb_std_callback (w, nbcb_find, cbs);

  VISIBLELINE_LOOP (PCB->Data);
  {
    if (TEST_FLAG (FOUNDFLAG, line) && !TEST_FLAG (LOCKFLAG, line))
      RemoveObject (LINE_TYPE, layer, line, line);
  }
  ENDALL_LOOP;

  VISIBLEARC_LOOP (PCB->Data);
  {
    if (TEST_FLAG (FOUNDFLAG, arc) && !TEST_FLAG (LOCKFLAG, arc))
      RemoveObject (ARC_TYPE, layer, arc, arc);
  }
  ENDALL_LOOP;

  if (PCB->ViaOn)
    VIA_LOOP (PCB->Data);
  {
    if (TEST_FLAG (FOUNDFLAG, via) && !TEST_FLAG (LOCKFLAG, via))
      RemoveObject (VIA_TYPE, via, via, via);
  }
  END_LOOP;
}

static void
netnode_browse (Widget w, XtPointer v, XmListCallbackStruct * cbs)
{
  LibraryMenuType *menu = PCB->NetlistLib.Menu + last_pick;
  char *name = menu->Entry[cbs->item_position - 1].ListEntry;
  char *ename, *pname;

  ename = strdup (name);
  pname = strchr (ename, '-');
  if (! pname)
    {
      free (ename);
      return;
    }
  *pname++ = 0;

  ELEMENT_LOOP (PCB->Data);
  {
    char *es = element->Name[NAMEONPCB_INDEX].TextString;
    if (es && strcmp (es, ename) == 0)
      {
	PIN_LOOP (element);
	{
	  if (strcmp (pin->Number, pname) == 0)
	    {
	      MoveCrosshairAbsolute (pin->X, pin->Y);
	      free (ename);
	      return;
	    }
	}
	END_LOOP;
	PAD_LOOP (element);
	{
	  if (strcmp (pad->Number, pname) == 0)
	    {
	      int x = (pad->Point1.X + pad->Point2.X) / 2;
	      int y = (pad->Point1.Y + pad->Point2.Y) / 2;
	      gui->set_crosshair (x, y, HID_SC_PAN_VIEWPORT);
	      free (ename);
	      return;
	    }
	}
	END_LOOP;
      }
  }
  END_LOOP;
  free (ename);
}

#define NLB_FORM ((Widget)(~0))
static Widget
netlist_button (Widget parent, char *name, char *string,
		Widget top, Widget bottom, Widget left, Widget right,
		XtCallbackProc callback, void *user_data)
{
  Widget rv;
  XmString str;

#define NLB_W(w) if (w == NLB_FORM) { stdarg(XmN ## w ## Attachment, XmATTACH_FORM); } \
  else if (w) { stdarg(XmN ## w ## Attachment, XmATTACH_WIDGET); \
    stdarg (XmN ## w ## Widget, w); }

  NLB_W (top);
  NLB_W (bottom);
  NLB_W (left);
  NLB_W (right);
  str = XmStringCreatePCB (string);
  stdarg(XmNlabelString, str);
  rv = XmCreatePushButton (parent, name, args, n);
  XtManageChild (rv);
  if (callback)
    XtAddCallback (rv, XmNactivateCallback, callback, (XtPointer)user_data);
  XmStringFree(str);
  return rv;
}

static int
build_netlist_dialog ()
{
  Widget b_sel, b_unsel, b_find, b_rat_on, l_ops;
  XmString ops_str;

  if (!mainwind)
    return 1;
  if (netlist_dialog)
    return 0;

  n = 0;
  stdarg (XmNresizePolicy, XmRESIZE_GROW);
  stdarg (XmNtitle, "Netlists");
  stdarg (XmNautoUnmanage, False);
  netlist_dialog = XmCreateFormDialog (mainwind, "netlist", args, n);

  n = 0;
  b_rat_on = netlist_button (netlist_dialog, "rat_on", "Enable for rats",
			     0, NLB_FORM, NLB_FORM, 0,
			     (XtCallbackProc)nbcb_std_callback, (void *)nbcb_rat_on);

  n = 0;
  netlist_button (netlist_dialog, "rat_off", "Disable for rats",
                  0, NLB_FORM, b_rat_on, 0,
                  (XtCallbackProc)nbcb_std_callback, (void *)nbcb_rat_off);

  n = 0;
  b_sel = netlist_button (netlist_dialog, "select", "Select",
			  0, b_rat_on, NLB_FORM, 0, 
			      (XtCallbackProc)nbcb_std_callback, (void *)nbcb_select);

  n = 0;
  b_unsel = netlist_button (netlist_dialog, "deselect", "Deselect",
			    0, b_rat_on, b_sel, 0, 
			      (XtCallbackProc)nbcb_std_callback, (void *)nbcb_deselect);

  n = 0;
  b_find = netlist_button (netlist_dialog, "find", "Find",
			   0, b_rat_on, b_unsel, 0,
			   (XtCallbackProc)nbcb_std_callback, (void *)nbcb_find);


  n = 0;
  netlist_button (netlist_dialog, "ripup", "Rip Up",
                  0, b_rat_on, b_find, 0,
                  (XtCallbackProc)nbcb_ripup, 0);

  n = 0;
  stdarg (XmNbottomAttachment, XmATTACH_WIDGET);
  stdarg (XmNbottomWidget, b_sel);
  stdarg (XmNleftAttachment, XmATTACH_FORM);
  ops_str = XmStringCreatePCB ("Operations on selected net names:");
  stdarg (XmNlabelString, ops_str);
  l_ops = XmCreateLabel (netlist_dialog, "ops", args, n);
  XtManageChild (l_ops);

  n = 0;
  stdarg (XmNtopAttachment, XmATTACH_FORM);
  stdarg (XmNbottomAttachment, XmATTACH_WIDGET);
  stdarg (XmNbottomWidget, l_ops);
  stdarg (XmNleftAttachment, XmATTACH_FORM);
  stdarg (XmNrightAttachment, XmATTACH_POSITION);
  stdarg (XmNrightPosition, 50);
  stdarg (XmNvisibleItemCount, 10);
  stdarg (XmNselectionPolicy, XmEXTENDED_SELECT);
  netlist_list = XmCreateScrolledList (netlist_dialog, "nets", args, n);
  XtManageChild (netlist_list);
  XtAddCallback (netlist_list, XmNdefaultActionCallback, (XtCallbackProc)netlist_select, 0);
  XtAddCallback (netlist_list, XmNextendedSelectionCallback, (XtCallbackProc)netlist_extend, 0);

  n = 0;
  stdarg (XmNtopAttachment, XmATTACH_FORM);
  stdarg (XmNbottomAttachment, XmATTACH_WIDGET);
  stdarg (XmNbottomWidget, l_ops);
  stdarg (XmNrightAttachment, XmATTACH_FORM);
  stdarg (XmNleftAttachment, XmATTACH_POSITION);
  stdarg (XmNleftPosition, 50);
  netnode_list = XmCreateScrolledList (netlist_dialog, "nodes", args, n);
  XtManageChild (netnode_list);
  XtAddCallback (netnode_list, XmNbrowseSelectionCallback, (XtCallbackProc)netnode_browse, 0);

  return 0;
}

static int
LesstifNetlistChanged (int argc, char **argv, Coord x, Coord y)
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
      XmStringCreatePCB (PCB->NetlistLib.Menu[i].Name);
  n = 0;
  stdarg (XmNitems, netlist_strings);
  stdarg (XmNitemCount, PCB->NetlistLib.MenuN);
  XtSetValues (netlist_list, args, n);
  pick_net (0);
  return 0;
}

static const char netlistshow_syntax[] =
"NetlistShow(pinname|netname)";

static const char netlistshow_help[] =
"Selects the given pinname or netname in the netlist window.";

/* %start-doc actions NetlistShow

%end-doc */

static int
LesstifNetlistShow (int argc, char **argv, Coord x, Coord y)
{
  if (build_netlist_dialog ())
    return 0;

  if (argc == 1)
    {
      LibraryMenuType *net;

      net = netnode_to_netname(argv[0]);
      if (net)
	{
	  XmString item;
	  int vis = 0;

	  /* Select net first, 'True' causes pick_net() to be invoked */
	  item = XmStringCreatePCB (net->Name);
	  XmListSelectItem (netlist_list, item, True);
	  XmListSetItem (netlist_list, item);
	  XmStringFree (item);

	  /* Now the netnode_list has the right contents */
	  item = XmStringCreatePCB (argv[0]);
	  XmListSelectItem (netnode_list, item, False);

	  /*
	   * Only force the item to the top if there are enough to scroll.
	   * A bug (?) in lesstif will cause the window to get ever wider
	   * if an XmList that doesn't require a scrollbar is forced to
	   * have one (when the top item is not the first item).
	   */
	  n = 0;
	  stdarg (XmNvisibleItemCount, &vis);
	  XtGetValues (netnode_list, args, n);
	  if (n_netnode_strings > vis)
	    {
	      XmListSetItem (netnode_list, item);
	    }
	  XmStringFree (item);
	}
      else
	{
	  /* Try the argument as a netname */
	  net = netname_to_netname(argv[0]);
	  if (net)
	    {
	      XmString item;

	      item = XmStringCreatePCB (net->Name);
	      XmListSetItem (netlist_list, item);
	      XmListSelectItem (netlist_list, item, True);
	      XmStringFree (item);
	    }
	}
    }
  return 0;
}

void
lesstif_show_netlist ()
{
  build_netlist_dialog ();
  XtManageChild (netlist_dialog);
}

HID_Action lesstif_netlist_action_list[] = {
  {"NetlistChanged", 0, LesstifNetlistChanged,
   netlistchanged_help, netlistchanged_syntax},
  {"NetlistShow", 0, LesstifNetlistShow,
   netlistshow_help, netlistshow_syntax}
};

REGISTER_ACTIONS (lesstif_netlist_action_list)
