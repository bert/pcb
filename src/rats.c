/* $Id$ */

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

RCSID ("$Id$");


#define TRIEDFIRST 0x1
#define BESTFOUND 0x2

/* ---------------------------------------------------------------------------
 * some forward declarations
 */
static Boolean FindPad (char *, char *, ConnectionType *, Boolean);
static Boolean ParseConnection (char *, char *, char *);
static Boolean DrawShortestRats (NetListTypePtr, void (*)());
static Boolean GatherSubnets (NetListTypePtr, Boolean, Boolean);
static Boolean CheckShorts (LibraryMenuTypePtr);
static void TransferNet (NetListTypePtr, NetTypePtr, NetTypePtr);

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static Boolean badnet = False;
static Cardinal SLayer, CLayer;	/* layer group holding solder/component side */

/* ---------------------------------------------------------------------------
 * parse a connection description from a string
 * puts the element name in the string and the pin number in
 * the number.  If a valid connection is found, it returns the
 * number of characters processed from the string, otherwise
 * it returns 0
 */
static Boolean
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
      return (False);
    }
  else
    {
      ElementName[j] = '\0';
      Message (_("Bad net-list format encountered near: \"%s\"\n"),
	       ElementName);
      return (True);
    }
}

/* ---------------------------------------------------------------------------
 * Find a particular pad from an element name and pin number
 */
static Boolean
FindPad (char *ElementName, char *PinNum, ConnectionType * conn, Boolean Same)
{
  ElementTypePtr element;
  Cardinal i;

  if ((element = SearchElementByName (PCB->Data, ElementName)) != NULL)
    {
      for (i = 0; i < element->PadN; i++)
	if (NSTRCMP (PinNum, element->Pad[i].Number) == 0 && (!Same
							      ||
							      !TEST_FLAG
							      (DRCFLAG,
							       &element->
							       Pad[i])))
	  {
	    conn->type = PAD_TYPE;
	    conn->ptr2 = &element->Pad[i];
	    conn->group =
	      TEST_FLAG (ONSOLDERFLAG, &element->Pad[i]) ? SLayer : CLayer;
	    if (TEST_FLAG (EDGE2FLAG, &element->Pad[i]))
	      {
		conn->X = element->Pad[i].Point2.X;
		conn->Y = element->Pad[i].Point2.Y;
	      }
	    else
	      {
		conn->X = element->Pad[i].Point1.X;
		conn->Y = element->Pad[i].Point1.Y;
	      }
	    break;
	  }
      if (i == element->PadN)
	{
	  for (i = 0; i < element->PinN; i++)
	    if (!TEST_FLAG (HOLEFLAG, &element->Pin[i]) &&
		element->Pin[i].Number &&
		NSTRCMP (PinNum, element->Pin[i].Number) == 0 &&
		(!Same || !TEST_FLAG (DRCFLAG, &element->Pin[i])))
	      {
		conn->type = PIN_TYPE;
		conn->ptr2 = &element->Pin[i];
		conn->group = SLayer;	/* any layer will do */
		conn->X = element->Pin[i].X;
		conn->Y = element->Pin[i].Y;
		break;
	      }
	  if (i == element->PinN)
	    return (False);
	}
      conn->ptr1 = element;
      return (True);
    }
  return (False);
}

/*--------------------------------------------------------------------------
 * parse a netlist menu entry and locate the corresponding pad
 * returns True if found, and fills in Connection information
 */
Boolean
SeekPad (LibraryEntryType * entry, ConnectionType * conn, Boolean Same)
{
  int j;
  char ElementName[256];
  char PinNum[256];

  if (ParseConnection (entry->ListEntry, ElementName, PinNum))
    return (False);
  for (j = 0; PinNum[j] != '\0'; j++);
  if (j == 0)
    {
      Message (_("Error! Netlist file is missing pin!\n"
		 "white space after \"%s-\"\n"), ElementName);
      badnet = True;
    }
  else
    {
      if (FindPad (ElementName, PinNum, conn, Same))
	return (True);
      if (Same)
	return (False);
      if (PinNum[j - 1] < '0' || PinNum[j - 1] > '9')
	{
	  Message ("WARNING! Pin number ending with '%c'"
		   " encountered in netlist file\n"
		   "Probably a bad netlist file format\n", PinNum[j - 1]);
	}
    }
  Message (_("Can't find %s pin %s called for in netlist.\n"),
	   ElementName, PinNum);
  return (False);
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
  SaveFree (Wantlist);
  /*  MYFREE (Wantlist); *//* awkward */
  badnet = False;

  /* find layer groups of the component side and solder side */
  SLayer = GetLayerGroupNumberByNumber (max_layer + SOLDER_LAYER);
  CLayer = GetLayerGroupNumberByNumber (max_layer + COMPONENT_LAYER);

  Wantlist = MyCalloc (1, sizeof (NetListType), "ProcNetlist()");
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
	    badnet = True;
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
	  if (SeekPad (entry, &LastPoint, False))
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
	    badnet = True;
	  /* check for more pins with the same number */
	  for (; SeekPad (entry, &LastPoint, True);)
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

static Boolean
CheckShorts (LibraryMenuTypePtr theNet)
{
  Boolean new, warn = False;
  PointerListTypePtr generic = MyCalloc (1, sizeof (PointerListType),
					 "CheckShorts");
  /* the first connection was starting point so
   * the menu is always non-null
   */
  void **menu = GetPointerMemory (generic);

  *menu = theNet;
  ALLPIN_LOOP (PCB->Data);
  {
    if (TEST_FLAG (DRCFLAG, pin))
      {
	warn = True;
	if (!pin->Spare)
	  {
	    Message (_("Warning! Net \"%s\" is shorted to %s pin %s\n"),
		     &theNet->Name[2],
		     UNKNOWN (NAMEONPCB_NAME (element)),
		     UNKNOWN (pin->Number));
	    SET_FLAG (WARNFLAG, pin);
	    continue;
	  }
	new = True;
	POINTER_LOOP (generic);
	{
	  if (*ptr == pin->Spare)
	    {
	      new = False;
	      break;
	    }
	}
	END_LOOP;
	if (new)
	  {
	    menu = GetPointerMemory (generic);
	    *menu = pin->Spare;
	    Message (_("Warning! net \"%s\" is shorted to net \"%s\"\n"),
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
	warn = True;
	if (!pad->Spare)
	  {
	    Message (_("Warning! Net \"%s\" is shorted  to %s pad %s\n"),
		     &theNet->Name[2],
		     UNKNOWN (NAMEONPCB_NAME (element)),
		     UNKNOWN (pad->Number));
	    SET_FLAG (WARNFLAG, pad);
	    continue;
	  }
	new = True;
	POINTER_LOOP (generic);
	{
	  if (*ptr == pad->Spare)
	    {
	      new = False;
	      break;
	    }
	}
	END_LOOP;
	if (new)
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
  SaveFree (generic);
  return (warn);
}


/* ---------------------------------------------------------------------------
 * Determine existing interconnections of the net and gather into sub-nets
 *
 * initially the netlist has each connection in its own individual net
 * afterwards there can be many fewer nets with multiple connections each
 */
static Boolean
GatherSubnets (NetListTypePtr Netl, Boolean NoWarn, Boolean AndRats)
{
  NetTypePtr a, b;
  ConnectionTypePtr conn;
  Cardinal m, n;
  Boolean Warned = False;

  for (m = 0; Netl->NetN > 0 && m < Netl->NetN; m++)
    {
      a = &Netl->Net[m];
      ResetFoundPinsViasAndPads (False);
      ResetFoundLinesAndPolygons (False);
      RatFindHook (a->Connection[0].type, a->Connection[0].ptr1,
		   a->Connection[0].ptr2, a->Connection[0].ptr2, False,
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
  ResetFoundPinsViasAndPads (False);
  ResetFoundLinesAndPolygons (False);
  return (Warned);
}

/* ---------------------------------------------------------------------------
 * Draw a rat net (tree) having the shortest lines
 * this also frees the subnet memory as they are consumed
 */

static Boolean
DrawShortestRats (NetListTypePtr Netl, void (*funcp) ())
{
  RatTypePtr line;
  register float distance, temp;
  register ConnectionTypePtr conn1, conn2, firstpoint, secondpoint;
  PolygonTypePtr polygon;
  Boolean changed = False;
  Cardinal n, m, j;
  NetTypePtr next, subnet, theSubnet = NULL;

  /*
   * Everything inside the NetList Netl should be connected together.
   * Each Net in Netl is a group of Connections which are already
   * connected together somehow, either by real wires or by rats we've
   * already drawn.  Each Connection is a vertex within that blob of
   * connected items.  This loop finds the closest vertex pairs between
   * each blob and draws rats that merge the blobs until there's just
   * one big blob.
   */
  distance = 0.0;
  while (Netl->NetN > 1)
    {
      firstpoint = secondpoint = NULL;
      subnet = &Netl->Net[0];
      for (j = 1; j < Netl->NetN; j++)
	{
	  next = &Netl->Net[j];
	  for (n = subnet->ConnectionN - 1; n != -1; n--)
	    {
	      conn1 = &subnet->Connection[n];
	      for (m = next->ConnectionN - 1; m != -1; m--)
		{
		  conn2 = &next->Connection[m];
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
		    }
		  else if ((temp = SQUARE (conn1->X - conn2->X) +
		       SQUARE (conn1->Y - conn2->Y)) < distance || !firstpoint)
		    {
		      distance = temp;
		      firstpoint = conn1;
		      secondpoint = conn2;
		      theSubnet = next;
		    }
		}
	    }
	}
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
	      DrawRat (line, 0);
	      changed = True;
	    }
	}

      /* copy theSubnet into the current subnet */
      TransferNet (Netl, theSubnet, subnet);
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
Boolean
AddAllRats (Boolean SelectedOnly, void (*funcp) ())
{
  NetListTypePtr Nets, Wantlist;
  NetTypePtr lonesome;
  ConnectionTypePtr onepin;
  Boolean changed, Warned = False;

  /* the netlist library has the text form
   * ProcNetlist fills in the Netlist
   * structure the way the final routing
   * is supposed to look
   */
  Wantlist = ProcNetlist (&PCB->NetlistLib);
  if (!Wantlist)
    {
      Message (_("Can't add rat lines because no netlist is loaded.\n"));
      return (False);
    }
  changed = False;
  /* initialize finding engine */
  InitConnectionLookup ();
  SaveFindFlag (DRCFLAG);
  Nets = MyCalloc (1, sizeof (NetListType), "AddAllRats()");
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
    Warned |= GatherSubnets (Nets, SelectedOnly, True);
    if (Nets->NetN > 0)
      changed |= DrawShortestRats (Nets, funcp);
  }
  END_LOOP;
  FreeNetListMemory (Nets);
  FreeConnectionLookupMemory ();
  RestoreFindFlag ();
  if (funcp)
    return (True);

  if (Warned || changed)
    Draw ();

  if (Warned)
    Settings.RatWarn = True;

  if (changed)
    {
      IncrementUndoSerialNumber ();
      if (PCB->Data->RatN > 0) 
	{
	  Message ("%d rat line%s remaining\n", PCB->Data->RatN,
		   PCB->Data->RatN > 1 ? "s" : "");
	}
      return (True);
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
  return (False);
}

/* XXX: This is copied in large part from AddAllRats above; for
 * maintainability, AddAllRats probably wants to be tweaked to use this
 * version of the code so that we don't have duplication. */
NetListListType
CollectSubnets (Boolean SelectedOnly)
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
    GatherSubnets (Nets, SelectedOnly, False);
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
	    GetLayerGroupNumberByNumber (max_layer + SOLDER_LAYER) :
	    GetLayerGroupNumberByNumber (max_layer + COMPONENT_LAYER));
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
	    GetLayerGroupNumberByNumber (max_layer + SOLDER_LAYER) :
	    GetLayerGroupNumberByNumber (max_layer + COMPONENT_LAYER));
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
      entry->ListEntry = MyStrdup (name2, "AddNet");
      netnode_to_netname (name2);
      goto ratIt;
    }
  /* ok, the first name did not belong to a net */
  menu = netnode_to_netname (name2);
  if (menu)
    {
      entry = GetLibraryEntryMemory (menu);
      entry->ListEntry = MyStrdup (name1, "AddNet");
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
  menu->Name = MyStrdup (ratname, "AddNet");
  entry = GetLibraryEntryMemory (menu);
  entry->ListEntry = MyStrdup (name1, "AddNet");
  entry = GetLibraryEntryMemory (menu);
  entry->ListEntry = MyStrdup (name2, "AddNet");
  menu->flag = 1;

ratIt:
  hid_action ("NetlistChanged");
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
