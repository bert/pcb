/*!
 * \file src/netlist.c
 *
 * \brief Generic netlist operations.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996, 2005 Thomas Nau
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Contact addresses for paper mail and Email:
 *
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *
 * Thomas.Nau@rz.uni-ulm.de
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <ctype.h>
#include <sys/types.h>
#ifdef HAVE_REGEX_H
#include <regex.h>
#endif

#include "global.h"
#include "action.h"
#include "buffer.h"
#include "data.h"
#include "djopt.h"
#include "error.h"
#include "file.h"
#include "find.h"
#include "mymem.h"
#include "misc.h"
#include "rats.h"
#include "set.h"
#include "vendor.h"
#include "create.h"

#ifdef HAVE_REGCOMP
#undef HAVE_RE_COMP
#endif

#if defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP)
#define USE_RE
#endif

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/*
  int    PCB->NetlistLib.MenuN
  char * PCB->NetlistLib.Menu[i].Name
     [0] == '*' (ok for rats) or ' ' (skip for rats)
     [1] == unused
     [2..] actual name
  char * PCB->NetlistLib.Menu[i].Style
  int    PCB->NetlistLib.Menu[i].EntryN
  char * PCB->NetlistLib.Menu[i].Entry[j].ListEntry
*/

typedef void (*NFunc) (LibraryMenuType *, LibraryEntryType *);

int netlist_frozen = 0;
static int netlist_needs_update = 0;

void
NetlistChanged (int force_unfreeze)
{
  if (force_unfreeze)
    netlist_frozen = 0;
  if (netlist_frozen)
    netlist_needs_update = 1;
  else
    {
      netlist_needs_update = 0;
      hid_action ("NetlistChanged");
    }
}

LibraryMenuType *
netnode_to_netname (char *nodename)
{
  int i, j;
  /*printf("nodename [%s]\n", nodename);*/
  for (i=0; i<PCB->NetlistLib.MenuN; i++)
    {
      for (j=0; j<PCB->NetlistLib.Menu[i].EntryN; j++)
	{
	  if (strcmp (PCB->NetlistLib.Menu[i].Entry[j].ListEntry, nodename) == 0)
	    {
	      /*printf(" in [%s]\n", PCB->NetlistLib.Menu[i].Name);*/
	      return & (PCB->NetlistLib.Menu[i]);
	    }
	}
    }
  return 0;
}

LibraryMenuType *
netname_to_netname (char *netname)
{
  int i;

  if ((netname[0] == '*' || netname[0] == ' ') && netname[1] == ' ')
    {
      /* Looks like we were passed an internal netname, skip the prefix */
      netname += 2;
    }
  for (i=0; i<PCB->NetlistLib.MenuN; i++)
    {
      if (strcmp (PCB->NetlistLib.Menu[i].Name + 2, netname) == 0)
	{
	  return & (PCB->NetlistLib.Menu[i]);
	}
    }
  return 0;
}

static int
pin_name_to_xy (LibraryEntryType * pin, int *x, int *y)
{
  ConnectionType conn;
  if (!SeekPad (pin, &conn, false))
    return 1;
  switch (conn.type)
    {
    case PIN_TYPE:
      *x = ((PinType *) (conn.ptr2))->X;
      *y = ((PinType *) (conn.ptr2))->Y;
      return 0;
    case PAD_TYPE:
      *x = ((PadType *) (conn.ptr2))->Point1.X;
      *y = ((PadType *) (conn.ptr2))->Point1.Y;
      return 0;
    }
  return 1;
}

static void
netlist_find (LibraryMenuType * net, LibraryEntryType * pin)
{
  int x, y;
  if (pin_name_to_xy (net->Entry, &x, &y))
    return;
  LookupConnection (x, y, 1, 1, FOUNDFLAG, true);
}

static void
netlist_select (LibraryMenuType * net, LibraryEntryType * pin)
{
  int x, y;
  if (pin_name_to_xy (net->Entry, &x, &y))
    return;
  LookupConnection (x, y, 1, 1, SELECTEDFLAG, true);
}

static void
netlist_rats (LibraryMenuType * net, LibraryEntryType * pin)
{
  net->Name[0] = ' ';
  net->flag = 1;
  NetlistChanged (0);
}

static void
netlist_norats (LibraryMenuType * net, LibraryEntryType * pin)
{
  net->Name[0] = '*';
  net->flag = 0;
  NetlistChanged (0);
}

/*!
 * \brief The primary purpose of this action is to remove the netlist
 * completely so that a new one can be loaded, usually via a gsch2pcb
 * style script.
 */
static void
netlist_clear (LibraryMenuType * net, LibraryEntryType * pin)
{
  LibraryType *netlist = &PCB->NetlistLib;
  int ni, pi;

  if (net == 0)
    {
      /* Clear the entire netlist. */
      FreeLibraryMemory (&PCB->NetlistLib);
    }
  else if (pin == 0)
    {
      /* Remove a net from the netlist. */
      ni = net - netlist->Menu;
      if (ni >= 0 && ni < netlist->MenuN)
	{
	  /* if there is exactly one item, MenuN is 1 and ni is 0 */
	  if (netlist->MenuN - ni > 1)
	    memmove (net, net+1, (netlist->MenuN - ni - 1) * sizeof (*net));
	  netlist->MenuN --;
	}
    }
  else
    {
      /* Remove a pin from the given net.  Note that this may leave an
	 empty net, which is different than removing the net
	 (above).  */
      pi = pin - net->Entry;
      if (pi >= 0 && pi < net->EntryN)
	{
	  /* if there is exactly one item, MenuN is 1 and ni is 0 */
	  if (net->EntryN - pi > 1)
	    memmove (pin, pin+1, (net->EntryN - pi - 1) * sizeof (*pin));
	  net->EntryN --;
	}
    }
  NetlistChanged (0);
}

static void
netlist_style (LibraryMenuType *net, const char *style)
{
  free (net->Style);
  net->Style = STRDUP ((char *)style);
}

/*!
 * \brief The primary purpose of this action is to rebuild a netlist
 * from a script, in conjunction with the clear action above.
 */
static int
netlist_add (const char *netname, const char *pinname)
{
  int ni, pi;
  LibraryType *netlist = &PCB->NetlistLib;
  LibraryMenuType *net = NULL;
  LibraryEntryType *pin = NULL;

  for (ni=0; ni<netlist->MenuN; ni++)
    if (strcmp (netlist->Menu[ni].Name+2, netname) == 0)
      {
	net = & (netlist->Menu[ni]);
	break;
      }
  if (net == NULL)
    {
      net = CreateNewNet (netlist, (char *)netname, NULL);
    }

  for (pi=0; pi<net->EntryN; pi++)
    if (strcmp (net->Entry[pi].ListEntry, pinname) == 0)
      {
	pin = & (net->Entry[pi]);
	break;
      }
  if (pin == NULL)
    {
      pin = CreateNewConnection (net, (char *)pinname);
    }

  NetlistChanged (0);
  return 0;
}

static const char netlist_syntax[] =
  "Net(find|select|rats|norats|clear[,net[,pin]])\n"
  "Net(freeze|thaw|forcethaw)\n"
  "Net(add,net,pin)";

static const char netlist_help[] = "Perform various actions on netlists.";

/* %start-doc actions Netlist

Each of these actions apply to a specified set of nets.  @var{net} and
@var{pin} are patterns which match one or more nets or pins; these
patterns may be full names or regular expressions.  If an exact match
is found, it is the only match; if no exact match is found,
@emph{then} the pattern is tried as a regular expression.

If neither @var{net} nor @var{pin} are specified, all nets apply.  If
@var{net} is specified but not @var{pin}, all nets matching @var{net}
apply.  If both are specified, nets which match @var{net} and contain
a pin matching @var{pin} apply.

@table @code

@item find
Nets which apply are marked @emph{found} and are drawn in the
@code{connected-color} color.

@item select
Nets which apply are selected.

@item rats
Nets which apply are marked as available for the rats nest.

@item norats
Nets which apply are marked as not available for the rats nest.

@item clear
Clears the netlist.

@item add
Add the given pin to the given netlist, creating either if needed.

@item sort
Called after a list of add's, this sorts the netlist.

@item freeze
@itemx thaw
@itemx forcethaw
Temporarily prevents changes to the netlist from being reflected in
the GUI.  For example, if you need to make multiple changes, you
freeze the netlist, make the changes, then thaw it.  Note that
freeze/thaw requests may nest, with the netlist being fully thawed
only when all pending freezes are thawed.  You can bypass the nesting
by using forcethaw, which resets the freeze count and immediately
updates the GUI.

@end table

%end-doc */

#define ARG(n) (argc > (n) ? argv[n] : 0)

static int
Netlist (int argc, char **argv, Coord x, Coord y)
{
  NFunc func;
  int i, j;
  LibraryMenuType *net;
  LibraryEntryType *pin;
  int net_found = 0;
  int pin_found = 0;
#if defined(USE_RE)
  int use_re = 0;
#endif
#if defined(HAVE_REGCOMP)
  regex_t elt_pattern;
  regmatch_t match;
#endif
#if defined(HAVE_RE_COMP)
  char *elt_pattern;
#endif

  if (!PCB)
    return 1;
  if (argc == 0)
    {
      Message (netlist_syntax);
      return 1;
    }
  if (strcasecmp (argv[0], "find") == 0)
    func = netlist_find;
  else if (strcasecmp (argv[0], "select") == 0)
    func = netlist_select;
  else if (strcasecmp (argv[0], "rats") == 0)
    func = netlist_rats;
  else if (strcasecmp (argv[0], "norats") == 0)
    func = netlist_norats;
  else if (strcasecmp (argv[0], "clear") == 0)
    {
      func = netlist_clear;
      if (argc == 1)
	{
	  netlist_clear (NULL, NULL);
	  return 0;
	}
    }
  else if (strcasecmp (argv[0], "style") == 0)
    func = (NFunc)netlist_style;
  else if (strcasecmp (argv[0], "add") == 0)
    {
      /* Add is different, because the net/pin won't already exist.  */
      return netlist_add (ARG(1), ARG(2));
    }
  else if (strcasecmp (argv[0], "sort") == 0)
    {
      sort_netlist ();
      return 0;
    }
  else if (strcasecmp (argv[0], "freeze") == 0)
    {
      netlist_frozen ++;
      return 0;
    }
  else if (strcasecmp (argv[0], "thaw") == 0)
    {
      if (netlist_frozen > 0)
	{
	  netlist_frozen --;
	  if (netlist_needs_update)
	    NetlistChanged (0);
	}
      return 0;
    }
  else if (strcasecmp (argv[0], "forcethaw") == 0)
    {
      netlist_frozen = 0;
      if (netlist_needs_update)
	NetlistChanged (0);
      return 0;
    }
  else
    {
      Message (netlist_syntax);
      return 1;
    }

#if defined(USE_RE)
  if (argc > 1)
    {
      int result;
      use_re = 1;
      for (i = 0; i < PCB->NetlistLib.MenuN; i++)
	{
	  net = PCB->NetlistLib.Menu + i;
	  if (strcasecmp (argv[1], net->Name + 2) == 0)
	    use_re = 0;
	}
      if (use_re)
	{
#if defined(HAVE_REGCOMP)
	  result =
	    regcomp (&elt_pattern, argv[1],
		     REG_EXTENDED | REG_ICASE | REG_NOSUB);
	  if (result)
	    {
	      char errorstring[128];

	      regerror (result, &elt_pattern, errorstring, 128);
	      Message (_("regexp error: %s\n"), errorstring);
	      regfree (&elt_pattern);
	      return (1);
	    }
#endif
#if defined(HAVE_RE_COMP)
	  if ((elt_pattern = re_comp (argv[1])) != NULL)
	    {
	      Message (_("re_comp error: %s\n"), elt_pattern);
	      return (false);
	    }
#endif
	}
    }
#endif

  for (i = PCB->NetlistLib.MenuN-1; i >= 0; i--)
    {
      net = PCB->NetlistLib.Menu + i;

      if (argc > 1)
	{
#if defined(USE_RE)
	  if (use_re)
	    {
#if defined(HAVE_REGCOMP)
	      if (regexec (&elt_pattern, net->Name + 2, 1, &match, 0) != 0)
		continue;
#endif
#if defined(HAVE_RE_COMP)
	      if (re_exec (net->Name + 2) != 1)
		continue;
#endif
	    }
	  else
#endif
	  if (strcasecmp (net->Name + 2, argv[1]))
	    continue;
	}
      net_found = 1;

      pin = 0;
      if (func == (void *)netlist_style)
	{
	  netlist_style (net, ARG(2));
	}
      else if (argc > 2)
	{
	  int l = strlen (argv[2]);
	  for (j = net->EntryN-1; j >= 0 ; j--)
	    if (strcasecmp (net->Entry[j].ListEntry, argv[2]) == 0
		|| (strncasecmp (net->Entry[j].ListEntry, argv[2], l) == 0
		    && net->Entry[j].ListEntry[l] == '-'))
	      {
		pin = net->Entry + j;
		pin_found = 1;
		func (net, pin);
	      }
	}
      else
	func (net, 0);
    }

  if (argc > 2 && !pin_found)
    {
      gui->log ("Net %s has no pin %s\n", argv[1], argv[2]);
      return 1;
    }
  else if (!net_found)
    {
      gui->log ("No net named %s\n", argv[1]);
    }
#ifdef HAVE_REGCOMP
  if (use_re)
    regfree (&elt_pattern);
#endif

  return 0;
}

HID_Action netlist_action_list[] = {
  {"net", 0, Netlist,
   netlist_help, netlist_syntax}
  ,
  {"netlist", 0, Netlist,
   netlist_help, netlist_syntax}
};

REGISTER_ACTIONS (netlist_action_list)
