/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 * 
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
 *  this module is also subject to the GNU GPL as described below
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
 */

/* Change History:
 * Started 6/10/97
 * Added support for minimum length rat lines 6/13/97
 * rat lines to nearest line/via 8/29/98
 * support for netlist window 10/24/98
 */

/* rats nest routines
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdio.h>

#include "global.h"

#include "create.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "file.h"
#include "find.h"
#include "misc.h"
#include "mymem.h"
#include "polygon.h"
#include "rats.h"
#include "search.h"
#include "set.h"
#include "undo.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define TRIEDFIRST 0x1
#define BESTFOUND 0x2

/* ---------------------------------------------------------------------------
 * some forward declarations
 */
static bool FindPad (char *, char *, ConnectionType *, bool);
static bool ParseConnection (char *, char *, char *);
static bool DrawShortestRats (NetListTypePtr, void (*)(register ConnectionTypePtr, register ConnectionTypePtr, register RouteStyleTypePtr));
static bool GatherSubnets (NetListTypePtr, bool, bool);
static bool CheckShorts (LibraryMenuTypePtr);
static void TransferNet (NetListTypePtr, NetTypePtr, NetTypePtr);

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static bool badnet = false;
static Cardinal SLayer, CLayer;	/* layer group holding solder/component side */

/* ---------------------------------------------------------------------------
 * parse a connection description from a string
 * puts the element name in the string and the pin number in
 * the number.  If a valid connection is found, it returns the
 * number of characters processed from the string, otherwise
 * it returns 0
 */
static bool
ParseConnection (char *InString, char *ElementName, char *PinNum)
{
  int i, j;

  /* copy element name portion */
  for (j = 0; InString[j] != '\0' && InString[j] != '-'; j++)
    ElementName[j] = InString[j];
  if (InString[j] == '-')
    {
      for (i = j; i > 0 && ElementName[i - 1] >= 'a'; i--);
      ElementName[i] = '\0';
      for (i = 0, j++; InString[j] != '\0'; i++, j++)
	PinNum[i] = InString[j];
      PinNum[i] = '\0';
      return (false);
    }
  else
    {
      ElementName[j] = '\0';
      Message (_("Bad net-list format encountered near: \"%s\"\n"),
	       ElementName);
      return (true);
    }
}

/* ---------------------------------------------------------------------------
 * Find a particular pad from an element name and pin number
 */
static bool
FindPad (char *ElementName, char *PinNum, ConnectionType * conn, bool Same)
{
  ElementTypePtr element;
  GList *i;

  if ((element = SearchElementByName (PCB->Data, ElementName)) == NULL)
    return false;

  for (i = element->Pad; i != NULL; i = g_list_next (i))
    {
      PadType *pad = i->data;

      if (NSTRCMP (PinNum, pad->Number) == 0 &&
          (!Same || !TEST_FLAG (DRCFLAG, pad)))
        {
          conn->type = PAD_TYPE;
          conn->ptr1 = element;
          conn->ptr2 = pad;
          conn->group = TEST_FLAG (ONSOLDERFLAG, pad) ? SLayer : CLayer;

          if (TEST_FLAG (EDGE2FLAG, pad))
            {
              conn->X = pad->Point2.X;
              conn->Y = pad->Point2.Y;
            }
          else
            {
              conn->X = pad->Point1.X;
              conn->Y = pad->Point1.Y;
            }
          return true;
        }
    }

  for (i = element->Pin; i != NULL; i = g_list_next (i))
    {
      PinType *pin = i->data;

      if (!TEST_FLAG (HOLEFLAG, pin) &&
          pin->Number && NSTRCMP (PinNum, pin->Number) == 0 &&
          (!Same || !TEST_FLAG (DRCFLAG, pin)))
        {
          conn->type = PIN_TYPE;
          conn->ptr1 = element;
          conn->ptr2 = pin;
          conn->group = SLayer;        /* any layer will do */
          conn->X = pin->X;
          conn->Y = pin->Y;
          return true;
        }
    }

  return false;
}

/*--------------------------------------------------------------------------
 * parse a netlist menu entry and locate the corresponding pad
 * returns true if found, and fills in Connection information
 */
bool
SeekPad (LibraryEntryType * entry, ConnectionType * conn, bool Same)
{
  int j;
  char ElementName[256];
  char PinNum[256];

  if (ParseConnection (entry->ListEntry, ElementName, PinNum))
    return (false);
  for (j = 0; PinNum[j] != '\0'; j++);
  if (j == 0)
    {
      Message (_("Error! Netlist file is missing pin!\n"
		 "white space after \"%s-\"\n"), ElementName);
      badnet = true;
    }
  else
    {
      if (FindPad (ElementName, PinNum, conn, Same))
	return (true);
      if (Same)
	return (false);
      if (PinNum[j - 1] < '0' || PinNum[j - 1] > '9')
	{
	  Message ("WARNING! Pin number ending with '%c'"
		   " encountered in netlist file\n"
		   "Probably a bad netlist file format\n", PinNum[j - 1]);
	}
    }
  Message (_("Can't find %s pin %s called for in netlist.\n"),
	   ElementName, PinNum);
  return (false);
}

/* ---------------------------------------------------------------------------
 * Read the library-netlist build a true Netlist structure
 */

NetListTypePtr
ProcNetlist (LibraryTypePtr net_menu)
{
  ConnectionTypePtr connection;
  ConnectionType LastPoint;
  NetTypePtr net;
  static NetListTypePtr Wantlist = NULL;

  if (!net_menu->MenuN)
    return (NULL);
  FreeNetListMemory (Wantlist);
  free (Wantlist);
  badnet = false;

  /* find layer groups of the component side and solder side */
  SLayer = GetLayerGroupNumberByNumber (solder_silk_layer);
  CLayer = GetLayerGroupNumberByNumber (component_silk_layer);

  Wantlist = (NetListTypePtr)calloc (1, sizeof (NetListType));
  if (Wantlist)
    {
      ALLPIN_LOOP (PCB->Data);
      {
	pin->Spare = NULL;
	CLEAR_FLAG (DRCFLAG, pin);
      }
      ENDALL_LOOP;
      ALLPAD_LOOP (PCB->Data);
      {
	pad->Spare = NULL;
	CLEAR_FLAG (DRCFLAG, pad);
      }
      ENDALL_LOOP;
      MENU_LOOP (net_menu);
      {
	if (menu->Name[0] == '*' || menu->flag == 0)
	  {
	    badnet = true;
	    continue;
	  }
	net = GetNetMemory (Wantlist);
	if (menu->Style)
	  {
	    STYLE_LOOP (PCB);
	    {
	      if (style->Name && !NSTRCMP (style->Name, menu->Style))
		{
		  net->Style = style;
		  break;
		}
	    }
	    END_LOOP;
	  }
	else			/* default to NULL if none found */
	  net->Style = NULL;
	ENTRY_LOOP (menu);
	{
	  if (SeekPad (entry, &LastPoint, false))
	    {
	      if (TEST_FLAG (DRCFLAG, (PinTypePtr) LastPoint.ptr2))
		Message (_
			 ("Error! Element %s pin %s appears multiple times in the netlist file.\n"),
			 NAMEONPCB_NAME ((ElementTypePtr) LastPoint.ptr1),
			 (LastPoint.type ==
			  PIN_TYPE) ? ((PinTypePtr) LastPoint.ptr2)->
			 Number : ((PadTypePtr) LastPoint.ptr2)->Number);
	      else
		{
		  connection = GetConnectionMemory (net);
		  *connection = LastPoint;
		  /* indicate expect net */
		  connection->menu = menu;
		  /* mark as visited */
		  SET_FLAG (DRCFLAG, (PinTypePtr) LastPoint.ptr2);
		  if (LastPoint.type == PIN_TYPE)
		    ((PinTypePtr) LastPoint.ptr2)->Spare = (void *) menu;
		  else
		    ((PadTypePtr) LastPoint.ptr2)->Spare = (void *) menu;
		}
	    }
	  else
	    badnet = true;
	  /* check for more pins with the same number */
	  for (; SeekPad (entry, &LastPoint, true);)
	    {
	      connection = GetConnectionMemory (net);
	      *connection = LastPoint;
	      /* indicate expect net */
	      connection->menu = menu;
	      /* mark as visited */
	      SET_FLAG (DRCFLAG, (PinTypePtr) LastPoint.ptr2);
	      if (LastPoint.type == PIN_TYPE)
		((PinTypePtr) LastPoint.ptr2)->Spare = (void *) menu;
	      else
		((PadTypePtr) LastPoint.ptr2)->Spare = (void *) menu;
	    }
	}
	END_LOOP;
      }
      END_LOOP;
    }
  /* clear all visit marks */
  ALLPIN_LOOP (PCB->Data);
  {
    CLEAR_FLAG (DRCFLAG, pin);
  }
  ENDALL_LOOP;
  ALLPAD_LOOP (PCB->Data);
  {
    CLEAR_FLAG (DRCFLAG, pad);
  }
  ENDALL_LOOP;
  return (Wantlist);
}

/*
 * copy all connections from one net into another
 * and then remove the first net from its netlist
 */
static void
TransferNet (NetListTypePtr Netl, NetTypePtr SourceNet, NetTypePtr DestNet)
{
  ConnectionTypePtr conn;

  /* It would be worth checking if SourceNet is NULL here to avoid a segfault. Seb James. */
  CONNECTION_LOOP (SourceNet);
  {
    conn = GetConnectionMemory (DestNet);
    *conn = *connection;
  }
  END_LOOP;
  DestNet->Style = SourceNet->Style;
  /* free the connection memory */
  FreeNetMemory (SourceNet);
  /* remove SourceNet from its netlist */
  *SourceNet = Netl->Net[--(Netl->NetN)];
  /* zero out old garbage */
  memset (&Netl->Net[Netl->NetN], 0, sizeof (NetType));
}

static bool
CheckShorts (LibraryMenuTypePtr theNet)
{
  bool newone, warn = false;
  PointerListTypePtr generic = (PointerListTypePtr)calloc (1, sizeof (PointerListType));
  /* the first connection was starting point so
   * the menu is always non-null
   */
  void **menu = GetPointerMemory (generic);

  *menu = theNet;
  ALLPIN_LOOP (PCB->Data);
  {
    if (TEST_FLAG (DRCFLAG, pin))
      {
	warn = true;
	if (!pin->Spare)
	  {
	    Message (_("Warning! Net \"%s\" is shorted to %s pin %s\n"),
		     &theNet->Name[2],
		     UNKNOWN (NAMEONPCB_NAME (element)),
		     UNKNOWN (pin->Number));
	    SET_FLAG (WARNFLAG, pin);
	    continue;
	  }
	newone = true;
	POINTER_LOOP (generic);
	{
	  if (*ptr == pin->Spare)
	    {
	      newone = false;
	      break;
	    }
	}
	END_LOOP;
	if (newone)
	  {
	    menu = GetPointerMemory (generic);
	    *menu = pin->Spare;
	    Message (_("Warning! Net \"%s\" is shorted to net \"%s\"\n"),
		     &theNet->Name[2],
		     &((LibraryMenuTypePtr) (pin->Spare))->Name[2]);
	    SET_FLAG (WARNFLAG, pin);
	  }
      }
  }
  ENDALL_LOOP;
  ALLPAD_LOOP (PCB->Data);
  {
    if (TEST_FLAG (DRCFLAG, pad))
      {
	warn = true;
	if (!pad->Spare)
	  {
	    Message (_("Warning! Net \"%s\" is shorted  to %s pad %s\n"),
		     &theNet->Name[2],
		     UNKNOWN (NAMEONPCB_NAME (element)),
		     UNKNOWN (pad->Number));
	    SET_FLAG (WARNFLAG, pad);
	    continue;
	  }
	newone = true;
	POINTER_LOOP (generic);
	{
	  if (*ptr == pad->Spare)
	    {
	      newone = false;
	      break;
	    }
	}
	END_LOOP;
	if (newone)
	  {
	    menu = GetPointerMemory (generic);
	    *menu = pad->Spare;
	    Message (_("Warning! Net \"%s\" is shorted to net \"%s\"\n"),
		     &theNet->Name[2],
		     &((LibraryMenuTypePtr) (pad->Spare))->Name[2]);
	    SET_FLAG (WARNFLAG, pad);
	  }
      }
  }
  ENDALL_LOOP;
  FreePointerListMemory (generic);
  free (generic);
  return (warn);
}


/* ---------------------------------------------------------------------------
 * Determine existing interconnections of the net and gather into sub-nets
 *
 * initially the netlist has each connection in its own individual net
 * afterwards there can be many fewer nets with multiple connections each
 */
static bool
GatherSubnets (NetListTypePtr Netl, bool NoWarn, bool AndRats)
{
  NetTypePtr a, b;
  ConnectionTypePtr conn;
  Cardinal m, n;
  bool Warned = false;

  for (m = 0; Netl->NetN > 0 && m < Netl->NetN; m++)
    {
      a = &Netl->Net[m];
      ResetConnections (false);
      RatFindHook (a->Connection[0].type, a->Connection[0].ptr1,
		   a->Connection[0].ptr2, a->Connection[0].ptr2, false,
		   AndRats);
      /* now anybody connected to the first point has DRCFLAG set */
      /* so move those to this subnet */
      CLEAR_FLAG (DRCFLAG, (PinTypePtr) a->Connection[0].ptr2);
      for (n = m + 1; n < Netl->NetN; n++)
	{
	  b = &Netl->Net[n];
	  /* There can be only one connection in net b */
	  if (TEST_FLAG (DRCFLAG, (PinTypePtr) b->Connection[0].ptr2))
	    {
	      CLEAR_FLAG (DRCFLAG, (PinTypePtr) b->Connection[0].ptr2);
	      TransferNet (Netl, b, a);
	      /* back up since new subnet is now at old index */
	      n--;
	    }
	}
      /* now add other possible attachment points to the subnet */
      /* e.g. line end-points and vias */
      /* don't add non-manhattan lines, the auto-router can't route to them */
      ALLLINE_LOOP (PCB->Data);
      {
	if (TEST_FLAG (DRCFLAG, line)
	    && ((line->Point1.X == line->Point2.X)
		|| (line->Point1.Y == line->Point2.Y)))
	  {
	    conn = GetConnectionMemory (a);
	    conn->X = line->Point1.X;
	    conn->Y = line->Point1.Y;
	    conn->type = LINE_TYPE;
	    conn->ptr1 = layer;
	    conn->ptr2 = line;
	    conn->group = GetLayerGroupNumberByPointer (layer);
	    conn->menu = NULL;	/* agnostic view of where it belongs */
	    conn = GetConnectionMemory (a);
	    conn->X = line->Point2.X;
	    conn->Y = line->Point2.Y;
	    conn->type = LINE_TYPE;
	    conn->ptr1 = layer;
	    conn->ptr2 = line;
	    conn->group = GetLayerGroupNumberByPointer (layer);
	    conn->menu = NULL;
	  }
      }
      ENDALL_LOOP;
      /* add polygons so the auto-router can see them as targets */
      ALLPOLYGON_LOOP (PCB->Data);
      {
	if (TEST_FLAG (DRCFLAG, polygon))
	  {
	    conn = GetConnectionMemory (a);
	    /* make point on a vertex */
	    conn->X = polygon->Clipped->contours->head.point[0];
	    conn->Y = polygon->Clipped->contours->head.point[1];
	    conn->type = POLYGON_TYPE;
	    conn->ptr1 = layer;
	    conn->ptr2 = polygon;
	    conn->group = GetLayerGroupNumberByPointer (layer);
	    conn->menu = NULL;	/* agnostic view of where it belongs */
	  }
      }
      ENDALL_LOOP;
      VIA_LOOP (PCB->Data);
      {
	if (TEST_FLAG (DRCFLAG, via))
	  {
	    conn = GetConnectionMemory (a);
	    conn->X = via->X;
	    conn->Y = via->Y;
	    conn->type = VIA_TYPE;
	    conn->ptr1 = via;
	    conn->ptr2 = via;
	    conn->group = SLayer;
	  }
      }
      END_LOOP;
      if (!NoWarn)
	Warned |= CheckShorts (a->Connection[0].menu);
    }
  ResetConnections (false);
  return (Warned);
}

/* ---------------------------------------------------------------------------
 * Draw a rat net (tree) having the shortest lines
 * this also frees the subnet memory as they are consumed
 *
 * Note that the Netl we are passed is NOT the main netlist - it's the
 * connectivity for ONE net.  It represents the CURRENT connectivity
 * state for the net, with each Netl->Net[N] representing one
 * copper-connected subset of the net.
 */

static bool
DrawShortestRats (NetListTypePtr Netl, void (*funcp) (register ConnectionTypePtr, register ConnectionTypePtr, register RouteStyleTypePtr))
{
  RatTypePtr line;
  register float distance, temp;
  register ConnectionTypePtr conn1, conn2, firstpoint, secondpoint;
  PolygonTypePtr polygon;
  bool changed = false;
  bool havepoints;
  Cardinal n, m, j;
  NetTypePtr next, subnet, theSubnet = NULL;

  /* This is just a sanity check, to make sure we're passed
   * *something*.
   */
  if (!Netl || Netl->NetN < 1)
    return false;

  /*
   * Everything inside the NetList Netl should be connected together.
   * Each Net in Netl is a group of Connections which are already
   * connected together somehow, either by real wires or by rats we've
   * already drawn.  Each Connection is a vertex within that blob of
   * connected items.  This loop finds the closest vertex pairs between
   * each blob and draws rats that merge the blobs until there's just
   * one big blob.
   *
   * Just to clarify, with some examples:
   *
   * Each Netl is one full net from a netlist, like from gnetlist.
   * Each Netl->Net[N] is a subset of that net that's already
   * physically connected on the pcb.
   *
   * So a new design with no traces yet, would have a huge list of Net[N],
   * each with one pin in it.
   *
   * A fully routed design would have one Net[N] with all the pins
   * (for that net) in it.
   */

  /*
   * We keep doing this do/while loop until everything's connected.
   * I.e. once per rat we add.
   */
  distance = 0.0;
  havepoints = true; /* so we run the loop at least once */
  while (Netl->NetN > 1 && havepoints)
    {
      /* This is the top of the "find one rat" logic.  */
      havepoints = false;
      firstpoint = secondpoint = NULL;

      /* Test Net[0] vs Net[N] for N=1..max.  Find the shortest
	 distance between any two points in different blobs.  */
      subnet = &Netl->Net[0];
      for (j = 1; j < Netl->NetN; j++)
	{
	  /*
	   * Scan between Net[0] blob (subnet) and Net[N] blob (next).
	   * Note the shortest distance we find.
	   */
	  next = &Netl->Net[j];
	  for (n = subnet->ConnectionN - 1; n != -1; n--)
	    {
	      conn1 = &subnet->Connection[n];
	      for (m = next->ConnectionN - 1; m != -1; m--)
		{
		  conn2 = &next->Connection[m];
		  /*
		   * At this point, conn1 and conn2 are two pins in
		   * different blobs of the same net.  See how far
		   * apart they are, and if they're "closer" than what
		   * we already have.
		   */

		  /*
		   * Prefer to connect Connections over polygons to the
		   * polygons (ie assume the user wants a via to a plane,
		   * not a daisy chain).  Further prefer to pick an existing
		   * via in the Net to make that connection.
		   */
		  if (conn1->type == POLYGON_TYPE &&
		      (polygon = (PolygonTypePtr)conn1->ptr2) &&
		      !(distance == 0 &&
		        firstpoint && firstpoint->type == VIA_TYPE) &&
		      IsPointInPolygonIgnoreHoles (conn2->X, conn2->Y, polygon))
		    {
		      distance = 0;
		      firstpoint = conn2;
		      secondpoint = conn1;
		      theSubnet = next;
		      havepoints = true;
		    }
		  else if (conn2->type == POLYGON_TYPE &&
		      (polygon = (PolygonTypePtr)conn2->ptr2) &&
		      !(distance == 0 &&
		        firstpoint && firstpoint->type == VIA_TYPE) &&
		      IsPointInPolygonIgnoreHoles (conn1->X, conn1->Y, polygon))
		    {
		      distance = 0;
		      firstpoint = conn1;
		      secondpoint = conn2;
		      theSubnet = next;
		      havepoints = true;
		    }
		  else if ((temp = SQUARE (conn1->X - conn2->X) +
		       SQUARE (conn1->Y - conn2->Y)) < distance || !firstpoint)
		    {
		      distance = temp;
		      firstpoint = conn1;
		      secondpoint = conn2;
		      theSubnet = next;
		      havepoints = true;
		    }
		}
	    }
	}

      /*
       * If HAVEPOINTS is true, we've found a pair of points in two
       * separate blobs of the net, and need to connect them together.
       */
      if (havepoints)
	{
	  if (funcp)
	    {
	      (*funcp) (firstpoint, secondpoint, subnet->Style);
	    }
	  else
	    {
	      /* found the shortest distance subnet, draw the rat */
	      if ((line = CreateNewRat (PCB->Data,
					firstpoint->X, firstpoint->Y,
					secondpoint->X, secondpoint->Y,
					firstpoint->group, secondpoint->group,
					Settings.RatThickness,
					NoFlags ())) != NULL)
		{
		  if (distance == 0)
		    SET_FLAG (VIAFLAG, line);
		  AddObjectToCreateUndoList (RATLINE_TYPE, line, line, line);
		  DrawRat (line);
		  changed = true;
		}
	    }

	  /* copy theSubnet into the current subnet */
	  TransferNet (Netl, theSubnet, subnet);
	}
    }

  /* presently nothing to do with the new subnet */
  /* so we throw it away and free the space */
  FreeNetMemory (&Netl->Net[--(Netl->NetN)]);
  /* Sadly adding a rat line messes up the sorted arrays in connection finder */
  /* hace: perhaps not necessarily now that they aren't stored in normal layers */
  if (changed)
    {
      FreeConnectionLookupMemory ();
      InitConnectionLookup ();
    }
  return (changed);
}


/* ---------------------------------------------------------------------------
 *  AddAllRats puts the rats nest into the layout from the loaded netlist
 *  if SelectedOnly is true, it will only draw rats to selected pins and pads
 */
bool
AddAllRats (bool SelectedOnly, void (*funcp) (register ConnectionTypePtr, register ConnectionTypePtr, register RouteStyleTypePtr))
{
  NetListTypePtr Nets, Wantlist;
  NetTypePtr lonesome;
  ConnectionTypePtr onepin;
  bool changed, Warned = false;

  /* the netlist library has the text form
   * ProcNetlist fills in the Netlist
   * structure the way the final routing
   * is supposed to look
   */
  Wantlist = ProcNetlist (&PCB->NetlistLib);
  if (!Wantlist)
    {
      Message (_("Can't add rat lines because no netlist is loaded.\n"));
      return (false);
    }
  changed = false;
  /* initialize finding engine */
  InitConnectionLookup ();
  SaveFindFlag (DRCFLAG);
  Nets = (NetListTypePtr)calloc (1, sizeof (NetListType));
  /* now we build another netlist (Nets) for each
   * net in Wantlist that shows how it actually looks now,
   * then fill in any missing connections with rat lines.
   *
   * we first assume each connection is separate
   * (no routing), then gather them into groups
   * if the net is all routed, the new netlist (Nets)
   * will have only one net entry.
   * Note that DrawShortestRats consumes all nets
   * from Nets, so *Nets is empty after the
   * DrawShortestRats call
   */
  NET_LOOP (Wantlist);
  {
    CONNECTION_LOOP (net);
    {
      if (!SelectedOnly
	  || TEST_FLAG (SELECTEDFLAG, (PinTypePtr) connection->ptr2))
	{
	  lonesome = GetNetMemory (Nets);
	  onepin = GetConnectionMemory (lonesome);
	  *onepin = *connection;
	  lonesome->Style = net->Style;
	}
    }
    END_LOOP;
    Warned |= GatherSubnets (Nets, SelectedOnly, true);
    if (Nets->NetN > 0)
      changed |= DrawShortestRats (Nets, funcp);
  }
  END_LOOP;
  FreeNetListMemory (Nets);
  free (Nets);
  FreeConnectionLookupMemory ();
  RestoreFindFlag ();
  if (funcp)
    return (true);

  if (Warned || changed)
    Draw ();

  if (Warned)
    Settings.RatWarn = true;

  if (changed)
    {
      IncrementUndoSerialNumber ();
      if (PCB->Data->RatN > 0) 
	{
	  Message ("%d rat line%s remaining\n", PCB->Data->RatN,
		   PCB->Data->RatN > 1 ? "s" : "");
	}
      return (true);
    }
  if (!SelectedOnly && !Warned)
    {
      if (!PCB->Data->RatN && !badnet)
	Message (_("Congratulations!!\n"
		   "The layout is complete and has no shorted nets.\n"));
      else
	Message (_("Nothing more to add, but there are\n"
		   "either rat-lines in the layout, disabled nets\n"
		   "in the net-list, or missing components\n"));
    }
  return (false);
}

/* XXX: This is copied in large part from AddAllRats above; for
 * maintainability, AddAllRats probably wants to be tweaked to use this
 * version of the code so that we don't have duplication. */
NetListListType
CollectSubnets (bool SelectedOnly)
{
  NetListListType result = { 0, 0, NULL };
  NetListTypePtr Nets, Wantlist;
  NetTypePtr lonesome;
  ConnectionTypePtr onepin;

  /* the netlist library has the text form
   * ProcNetlist fills in the Netlist
   * structure the way the final routing
   * is supposed to look
   */
  Wantlist = ProcNetlist (&PCB->NetlistLib);
  if (!Wantlist)
    {
      Message (_("Can't add rat lines because no netlist is loaded.\n"));
      return result;
    }
  /* initialize finding engine */
  InitConnectionLookup ();
  SaveFindFlag (DRCFLAG);
  /* now we build another netlist (Nets) for each
   * net in Wantlist that shows how it actually looks now,
   * then fill in any missing connections with rat lines.
   *
   * we first assume each connection is separate
   * (no routing), then gather them into groups
   * if the net is all routed, the new netlist (Nets)
   * will have only one net entry.
   * Note that DrawShortestRats consumes all nets
   * from Nets, so *Nets is empty after the
   * DrawShortestRats call
   */
  NET_LOOP (Wantlist);
  {
    Nets = GetNetListMemory (&result);
    CONNECTION_LOOP (net);
    {
      if (!SelectedOnly
	  || TEST_FLAG (SELECTEDFLAG, (PinTypePtr) connection->ptr2))
	{
	  lonesome = GetNetMemory (Nets);
	  onepin = GetConnectionMemory (lonesome);
	  *onepin = *connection;
	  lonesome->Style = net->Style;
	}
    }
    END_LOOP;
    /* Note that AndRats is *FALSE* here! */
    GatherSubnets (Nets, SelectedOnly, false);
  }
  END_LOOP;
  FreeConnectionLookupMemory ();
  RestoreFindFlag ();
  return result;
}

/*
 * Check to see if a particular name is the name of an already existing rats
 * line
 */
static int
rat_used (char *name)
{
  if (name == NULL)
    return -1;

  MENU_LOOP (&PCB->NetlistLib);
  {
    if (menu->Name && (strcmp (menu->Name, name) == 0))
      return 1;
  }
  END_LOOP;

  return 0;
}

  /* These next two functions moved from the original netlist.c as part of the
     |  gui code separation for the Gtk port.
   */
RatTypePtr
AddNet (void)
{
  static int ratDrawn = 0;
  char name1[256], *name2;
  Cardinal group1, group2;
  char ratname[20];
  int found;
  void *ptr1, *ptr2, *ptr3;
  LibraryMenuTypePtr menu;
  LibraryEntryTypePtr entry;

  if (Crosshair.AttachedLine.Point1.X == Crosshair.AttachedLine.Point2.X
      && Crosshair.AttachedLine.Point1.Y == Crosshair.AttachedLine.Point2.Y)
    return (NULL);

  found = SearchObjectByLocation (PAD_TYPE | PIN_TYPE, &ptr1, &ptr2, &ptr3,
				  Crosshair.AttachedLine.Point1.X,
				  Crosshair.AttachedLine.Point1.Y, 5);
  if (found == NO_TYPE)
    {
      Message (_("No pad/pin under rat line\n"));
      return (NULL);
    }
  if (NAMEONPCB_NAME ((ElementTypePtr) ptr1) == NULL
      || *NAMEONPCB_NAME ((ElementTypePtr) ptr1) == 0)
    {
      Message (_("You must name the starting element first\n"));
      return (NULL);
    }

  /* will work for pins to since the FLAG is common */
  group1 = (TEST_FLAG (ONSOLDERFLAG, (PadTypePtr) ptr2) ?
	    GetLayerGroupNumberByNumber (solder_silk_layer) :
	    GetLayerGroupNumberByNumber (component_silk_layer));
  strcpy (name1, ConnectionName (found, ptr1, ptr2));
  found = SearchObjectByLocation (PAD_TYPE | PIN_TYPE, &ptr1, &ptr2, &ptr3,
				  Crosshair.AttachedLine.Point2.X,
				  Crosshair.AttachedLine.Point2.Y, 5);
  if (found == NO_TYPE)
    {
      Message (_("No pad/pin under rat line\n"));
      return (NULL);
    }
  if (NAMEONPCB_NAME ((ElementTypePtr) ptr1) == NULL
      || *NAMEONPCB_NAME ((ElementTypePtr) ptr1) == 0)
    {
      Message (_("You must name the ending element first\n"));
      return (NULL);
    }
  group2 = (TEST_FLAG (ONSOLDERFLAG, (PadTypePtr) ptr2) ?
	    GetLayerGroupNumberByNumber (solder_silk_layer) :
	    GetLayerGroupNumberByNumber (component_silk_layer));
  name2 = ConnectionName (found, ptr1, ptr2);

  menu = netnode_to_netname (name1);
  if (menu)
    {
      if (netnode_to_netname (name2))
	{
	  Message (_
		   ("Both connections already in netlist - cannot merge nets\n"));
	  return (NULL);
	}
      entry = GetLibraryEntryMemory (menu);
      entry->ListEntry = strdup (name2);
      netnode_to_netname (name2);
      goto ratIt;
    }
  /* ok, the first name did not belong to a net */
  menu = netnode_to_netname (name2);
  if (menu)
    {
      entry = GetLibraryEntryMemory (menu);
      entry->ListEntry = strdup (name1);
      netnode_to_netname (name1);
      goto ratIt;
    }

  /*
   * neither belong to a net, so create a new one.
   *
   * before creating a new rats here, we need to search
   * for a unique name.
   */
  sprintf (ratname, "  ratDrawn%i", ++ratDrawn);
  while (rat_used (ratname))
    {
      sprintf (ratname, "  ratDrawn%i", ++ratDrawn);
    }

  menu = GetLibraryMenuMemory (&PCB->NetlistLib);
  menu->Name = strdup (ratname);
  entry = GetLibraryEntryMemory (menu);
  entry->ListEntry = strdup (name1);
  entry = GetLibraryEntryMemory (menu);
  entry->ListEntry = strdup (name2);
  menu->flag = 1;

ratIt:
  NetlistChanged (0);
  return (CreateNewRat (PCB->Data, Crosshair.AttachedLine.Point1.X,
			Crosshair.AttachedLine.Point1.Y,
			Crosshair.AttachedLine.Point2.X,
			Crosshair.AttachedLine.Point2.Y,
			group1, group2, Settings.RatThickness, NoFlags ()));
}


char *
ConnectionName (int type, void *ptr1, void *ptr2)
{
  static char name[256];
  char *num;

  switch (type)
    {
    case PIN_TYPE:
      num = ((PinTypePtr) ptr2)->Number;
      break;
    case PAD_TYPE:
      num = ((PadTypePtr) ptr2)->Number;
      break;
    default:
      return (NULL);
    }
  strcpy (name, UNKNOWN (NAMEONPCB_NAME ((ElementTypePtr) ptr1)));
  strcat (name, "-");
  strcat (name, UNKNOWN (num));
  return (name);
}
