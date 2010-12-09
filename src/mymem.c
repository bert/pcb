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


/* memory management functions
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"

#include <memory.h>

#include "data.h"
#include "error.h"
#include "mymem.h"
#include "misc.h"
#include "rats.h"
#include "rtree.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

/* ---------------------------------------------------------------------------
 * local prototypes
 */
static void DSRealloc (DynamicStringTypePtr, size_t);

/* ---------------------------------------------------------------------------
 * get next slot for a rubberband connection, allocates memory if necessary
 */
RubberbandTypePtr
GetRubberbandMemory (void)
{
  RubberbandTypePtr ptr = Crosshair.AttachedObject.Rubberband;

  /* realloc new memory if necessary and clear it */
  if (Crosshair.AttachedObject.RubberbandN >=
      Crosshair.AttachedObject.RubberbandMax)
    {
      Crosshair.AttachedObject.RubberbandMax += STEP_RUBBERBAND;
      ptr = MyRealloc (ptr,
		       Crosshair.AttachedObject.RubberbandMax *
		       sizeof (RubberbandType), "GetRubberbandMemory()");
      Crosshair.AttachedObject.Rubberband = ptr;
      memset (ptr + Crosshair.AttachedObject.RubberbandN, 0,
	      STEP_RUBBERBAND * sizeof (RubberbandType));
    }
  return (ptr + Crosshair.AttachedObject.RubberbandN++);
}

void **
GetPointerMemory (PointerListTypePtr list)
{
  void **ptr = list->Ptr;

  /* realloc new memory if necessary and clear it */
  if (list->PtrN >= list->PtrMax)
    {
      list->PtrMax = STEP_POINT + (2 * list->PtrMax);
      ptr = MyRealloc (ptr, list->PtrMax * sizeof (void *),
		       "GetPointerMemory()");
      list->Ptr = ptr;
      memset (ptr + list->PtrN, 0,
	      (list->PtrMax - list->PtrN) * sizeof (void *));
    }
  return (ptr + list->PtrN++);
}

void
FreePointerListMemory (PointerListTypePtr list)
{
  MYFREE (list->Ptr);
  memset (list, 0, sizeof (PointerListType));
}

/* ---------------------------------------------------------------------------
 * get next slot for a box, allocates memory if necessary
 */
BoxTypePtr
GetBoxMemory (BoxListTypePtr Boxes)
{
  BoxTypePtr box = Boxes->Box;

  /* realloc new memory if necessary and clear it */
  if (Boxes->BoxN >= Boxes->BoxMax)
    {
      Boxes->BoxMax = STEP_POINT + (2 * Boxes->BoxMax);
      box = MyRealloc (box, Boxes->BoxMax * sizeof (BoxType),
		       "GetBoxMemory()");
      Boxes->Box = box;
      memset (box + Boxes->BoxN, 0,
	      (Boxes->BoxMax - Boxes->BoxN) * sizeof (BoxType));
    }
  return (box + Boxes->BoxN++);
}


/* ---------------------------------------------------------------------------
 * get next slot for a connection, allocates memory if necessary
 */
ConnectionTypePtr
GetConnectionMemory (NetTypePtr Net)
{
  ConnectionTypePtr con = Net->Connection;

  /* realloc new memory if necessary and clear it */
  if (Net->ConnectionN >= Net->ConnectionMax)
    {
      Net->ConnectionMax += STEP_POINT;
      con = MyRealloc (con, Net->ConnectionMax * sizeof (ConnectionType),
		       "GetConnectionMemory()");
      Net->Connection = con;
      memset (con + Net->ConnectionN, 0,
	      STEP_POINT * sizeof (ConnectionType));
    }
  return (con + Net->ConnectionN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for a subnet, allocates memory if necessary
 */
NetTypePtr
GetNetMemory (NetListTypePtr Netlist)
{
  NetTypePtr net = Netlist->Net;

  /* realloc new memory if necessary and clear it */
  if (Netlist->NetN >= Netlist->NetMax)
    {
      Netlist->NetMax += STEP_POINT;
      net = MyRealloc (net, Netlist->NetMax * sizeof (NetType),
		       "GetNetMemory()");
      Netlist->Net = net;
      memset (net + Netlist->NetN, 0, STEP_POINT * sizeof (NetType));
    }
  return (net + Netlist->NetN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for a net list, allocates memory if necessary
 */
NetListTypePtr
GetNetListMemory (NetListListTypePtr Netlistlist)
{
  NetListTypePtr netlist = Netlistlist->NetList;

  /* realloc new memory if necessary and clear it */
  if (Netlistlist->NetListN >= Netlistlist->NetListMax)
    {
      Netlistlist->NetListMax += STEP_POINT;
      netlist = MyRealloc
	(netlist, Netlistlist->NetListMax * sizeof (NetListType),
	 "GetNetListMemory()");
      Netlistlist->NetList = netlist;
      memset (netlist + Netlistlist->NetListN, 0,
	      STEP_POINT * sizeof (NetListType));
    }
  return (netlist + Netlistlist->NetListN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for a pin, allocates memory if necessary
 */
PinTypePtr
GetPinMemory (ElementTypePtr Element)
{
  PinTypePtr pin = Element->Pin;
  bool onBoard = false;

  /* realloc new memory if necessary and clear it */
  if (Element->PinN >= Element->PinMax)
    {
      if (PCB->Data->pin_tree)
	{
	  PIN_LOOP (Element);
	  {
	    if (r_delete_entry (PCB->Data->pin_tree, (BoxType *) pin))
	      onBoard = true;
	  }
	  END_LOOP;
	}
      Element->PinMax += STEP_PIN;
      pin = MyRealloc (pin, Element->PinMax * sizeof (PinType),
		       "GetPinMemory()");
      Element->Pin = pin;
      memset (pin + Element->PinN, 0, STEP_PIN * sizeof (PinType));
      if (onBoard)
	{
	  PIN_LOOP (Element);
	  {
	    r_insert_entry (PCB->Data->pin_tree, (BoxType *) pin, 0);
	  }
	  END_LOOP;
	}
    }
  return (pin + Element->PinN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for a pad, allocates memory if necessary
 */
PadTypePtr
GetPadMemory (ElementTypePtr Element)
{
  PadTypePtr pad = Element->Pad;
  bool onBoard = false;

  /* realloc new memory if necessary and clear it */
  if (Element->PadN >= Element->PadMax)
    {
      if (PCB->Data->pad_tree)
	{
	  PAD_LOOP (Element);
	  {
	    if (r_delete_entry (PCB->Data->pad_tree, (BoxType *) pad))
	      onBoard = true;
	  }
	  END_LOOP;
	}
      Element->PadMax += STEP_PAD;
      pad = MyRealloc (pad, Element->PadMax * sizeof (PadType),
		       "GetPadMemory()");
      Element->Pad = pad;
      memset (pad + Element->PadN, 0, STEP_PAD * sizeof (PadType));
      if (onBoard)
	{
	  PAD_LOOP (Element);
	  {
	    r_insert_entry (PCB->Data->pad_tree, (BoxType *) pad, 0);
	  }
	  END_LOOP;
	}
    }
  return (pad + Element->PadN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for a via, allocates memory if necessary
 */
PinTypePtr
GetViaMemory (DataTypePtr Data)
{
  PinTypePtr via = Data->Via;

  /* realloc new memory if necessary and clear it */
  if (Data->ViaN >= Data->ViaMax)
    {
      Data->ViaMax += STEP_VIA;
      if (Data->via_tree)
	r_destroy_tree (&Data->via_tree);
      via = MyRealloc (via, Data->ViaMax * sizeof (PinType),
		       "GetViaMemory()");
      Data->Via = via;
      memset (via + Data->ViaN, 0, STEP_VIA * sizeof (PinType));
      Data->via_tree = r_create_tree (NULL, 0, 0);
      VIA_LOOP (Data);
      {
	r_insert_entry (Data->via_tree, (BoxType *) via, 0);
      }
      END_LOOP;
    }
  return (via + Data->ViaN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for a Rat, allocates memory if necessary
 */
RatTypePtr
GetRatMemory (DataTypePtr Data)
{
  RatTypePtr rat = Data->Rat;

  /* realloc new memory if necessary and clear it */
  if (Data->RatN >= Data->RatMax)
    {
      Data->RatMax += STEP_RAT;
      /* all of the pointers move, so rebuild the whole tree */
      if (Data->rat_tree)
        r_destroy_tree (&Data->rat_tree);
      rat = MyRealloc (rat, Data->RatMax * sizeof (RatType),
		       "GetRatMemory()");
      Data->Rat = rat;
      memset (rat + Data->RatN, 0, STEP_RAT * sizeof (RatType));
      Data->rat_tree = r_create_tree (NULL, 0, 0);
      RAT_LOOP (Data);
      {
        r_insert_entry (Data->rat_tree, (BoxTypePtr) line, 0);
      }
      END_LOOP;
    }
  return (rat + Data->RatN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for a line, allocates memory if necessary
 */
LineTypePtr
GetLineMemory (LayerTypePtr Layer)
{
  LineTypePtr line = Layer->Line;

  /* realloc new memory if necessary and clear it */
  if (Layer->LineN >= Layer->LineMax)
    {
      Layer->LineMax += STEP_LINE;
      /* all of the pointers move, so rebuild the whole tree */
      if (Layer->line_tree)
	r_destroy_tree (&Layer->line_tree);
      line = MyRealloc (line, Layer->LineMax * sizeof (LineType),
			"GetLineMemory()");
      Layer->Line = line;
      memset (line + Layer->LineN, 0, STEP_LINE * sizeof (LineType));
      Layer->line_tree = r_create_tree (NULL, 0, 0);
      LINE_LOOP (Layer);
      {
	r_insert_entry (Layer->line_tree, (BoxTypePtr) line, 0);
      }
      END_LOOP;
    }
  return (line + Layer->LineN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for an arc, allocates memory if necessary
 */
ArcTypePtr
GetArcMemory (LayerTypePtr Layer)
{
  ArcTypePtr arc = Layer->Arc;

  /* realloc new memory if necessary and clear it */
  if (Layer->ArcN >= Layer->ArcMax)
    {
      Layer->ArcMax += STEP_ARC;
      if (Layer->arc_tree)
	r_destroy_tree (&Layer->arc_tree);
      arc = MyRealloc (arc, Layer->ArcMax * sizeof (ArcType),
		       "GetArcMemory()");
      Layer->Arc = arc;
      memset (arc + Layer->ArcN, 0, STEP_ARC * sizeof (ArcType));
      Layer->arc_tree = r_create_tree (NULL, 0, 0);
      ARC_LOOP (Layer);
      {
	r_insert_entry (Layer->arc_tree, (BoxTypePtr) arc, 0);
      }
      END_LOOP;
    }
  return (arc + Layer->ArcN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for a text object, allocates memory if necessary
 */
TextTypePtr
GetTextMemory (LayerTypePtr Layer)
{
  TextTypePtr text = Layer->Text;

  /* realloc new memory if necessary and clear it */
  if (Layer->TextN >= Layer->TextMax)
    {
      Layer->TextMax += STEP_TEXT;
      if (Layer->text_tree)
	r_destroy_tree (&Layer->text_tree);
      text = MyRealloc (text, Layer->TextMax * sizeof (TextType),
			"GetTextMemory()");
      Layer->Text = text;
      memset (text + Layer->TextN, 0, STEP_TEXT * sizeof (TextType));
      Layer->text_tree = r_create_tree (NULL, 0, 0);
      TEXT_LOOP (Layer);
      {
	r_insert_entry (Layer->text_tree, (BoxTypePtr) text, 0);
      }
      END_LOOP;
    }
  return (text + Layer->TextN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for a polygon object, allocates memory if necessary
 */
PolygonTypePtr
GetPolygonMemory (LayerTypePtr Layer)
{
  PolygonTypePtr polygon = Layer->Polygon;

  /* realloc new memory if necessary and clear it */
  if (Layer->PolygonN >= Layer->PolygonMax)
    {
      Layer->PolygonMax += STEP_POLYGON;
      if (Layer->polygon_tree)
	r_destroy_tree (&Layer->polygon_tree);
      polygon = MyRealloc (polygon, Layer->PolygonMax * sizeof (PolygonType),
			   "GetPolygonMemory()");
      Layer->Polygon = polygon;
      memset (polygon + Layer->PolygonN, 0,
	      STEP_POLYGON * sizeof (PolygonType));
      Layer->polygon_tree = r_create_tree (NULL, 0, 0);
      POLYGON_LOOP (Layer);
      {
	r_insert_entry (Layer->polygon_tree, (BoxType *) polygon, 0);
      }
      END_LOOP;
    }
  return (polygon + Layer->PolygonN++);
}

/* ---------------------------------------------------------------------------
 * gets the next slot for a point in a polygon struct, allocates memory
 * if necessary
 */
PointTypePtr
GetPointMemoryInPolygon (PolygonTypePtr Polygon)
{
  PointTypePtr points = Polygon->Points;

  /* realloc new memory if necessary and clear it */
  if (Polygon->PointN >= Polygon->PointMax)
    {
      Polygon->PointMax += STEP_POLYGONPOINT;
      points = MyRealloc (points, Polygon->PointMax * sizeof (PointType),
			  "GetPointMemoryInPolygon()");
      Polygon->Points = points;
      memset (points + Polygon->PointN, 0,
	      STEP_POLYGONPOINT * sizeof (PointType));
    }
  return (points + Polygon->PointN++);
}

/* ---------------------------------------------------------------------------
 * gets the next slot for a point in a polygon struct, allocates memory
 * if necessary
 */
Cardinal *
GetHoleIndexMemoryInPolygon (PolygonTypePtr Polygon)
{
  Cardinal *holeindex = Polygon->HoleIndex;

  /* realloc new memory if necessary and clear it */
  if (Polygon->HoleIndexN >= Polygon->HoleIndexMax)
    {
      Polygon->HoleIndexMax += STEP_POLYGONHOLEINDEX;
      holeindex = MyRealloc (holeindex, Polygon->HoleIndexMax * sizeof (int),
			     "GetHoleIndexMemoryInPolygon()");
      Polygon->HoleIndex = holeindex;
      memset (holeindex + Polygon->HoleIndexN, 0,
	      STEP_POLYGONHOLEINDEX * sizeof (int));
    }
  return (holeindex + Polygon->HoleIndexN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for an element, allocates memory if necessary
 */
ElementTypePtr
GetElementMemory (DataTypePtr Data)
{
  ElementTypePtr element = Data->Element;
  int i;

  /* realloc new memory if necessary and clear it */
  if (Data->ElementN >= Data->ElementMax)
    {
      Data->ElementMax += STEP_ELEMENT;
      if (Data->element_tree)
	r_destroy_tree (&Data->element_tree);
      element = MyRealloc (element, Data->ElementMax * sizeof (ElementType),
			   "GetElementMemory()");
      Data->Element = element;
      memset (element + Data->ElementN, 0,
	      STEP_ELEMENT * sizeof (ElementType));
      Data->element_tree = r_create_tree (NULL, 0, 0);
      for (i = 0; i < MAX_ELEMENTNAMES; i++)
	{
	  if (Data->name_tree[i])
	    r_destroy_tree (&Data->name_tree[i]);
	  Data->name_tree[i] = r_create_tree (NULL, 0, 0);
	}

      ELEMENT_LOOP (Data);
      {
	r_insert_entry (Data->element_tree, (BoxType *) element, 0);
	PIN_LOOP (element);
	{
	  pin->Element = element;
	}
	END_LOOP;
	PAD_LOOP (element);
	{
	  pad->Element = element;
	}
	END_LOOP;
	ELEMENTTEXT_LOOP (element);
	{
	  text->Element = element;
	  r_insert_entry (Data->name_tree[n], (BoxType *) text, 0);
	}
	END_LOOP;
      }
      END_LOOP;
    }
  return (element + Data->ElementN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for a library menu, allocates memory if necessary
 */
LibraryMenuTypePtr
GetLibraryMenuMemory (LibraryTypePtr lib)
{
  LibraryMenuTypePtr menu = lib->Menu;

  /* realloc new memory if necessary and clear it */
  if (lib->MenuN >= lib->MenuMax)
    {
      lib->MenuMax += STEP_LIBRARYMENU;
      menu = MyRealloc (menu, lib->MenuMax * sizeof (LibraryMenuType),
			"GetLibraryMenuMemory()");
      lib->Menu = menu;
      memset (menu + lib->MenuN, 0,
	      STEP_LIBRARYMENU * sizeof (LibraryMenuType));
    }
  return (menu + lib->MenuN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for a library entry, allocates memory if necessary
 */
LibraryEntryTypePtr
GetLibraryEntryMemory (LibraryMenuTypePtr Menu)
{
  LibraryEntryTypePtr entry = Menu->Entry;

  /* realloc new memory if necessary and clear it */
  if (Menu->EntryN >= Menu->EntryMax)
    {
      Menu->EntryMax += STEP_LIBRARYENTRY;
      entry = MyRealloc (entry, Menu->EntryMax * sizeof (LibraryEntryType),
			 "GetLibraryEntryMemory()");
      Menu->Entry = entry;
      memset (entry + Menu->EntryN, 0,
	      STEP_LIBRARYENTRY * sizeof (LibraryEntryType));
    }
  return (entry + Menu->EntryN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for a DrillElement, allocates memory if necessary
 */
ElementTypeHandle
GetDrillElementMemory (DrillTypePtr Drill)
{
  ElementTypePtr *element;

  element = Drill->Element;

  /* realloc new memory if necessary and clear it */
  if (Drill->ElementN >= Drill->ElementMax)
    {
      Drill->ElementMax += STEP_ELEMENT;
      element =
	MyRealloc (element, Drill->ElementMax * sizeof (ElementTypeHandle),
		   "GetDrillElementMemory()");
      Drill->Element = element;
      memset (element + Drill->ElementN, 0,
	      STEP_ELEMENT * sizeof (ElementTypeHandle));
    }
  return (element + Drill->ElementN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for a DrillPoint, allocates memory if necessary
 */
PinTypeHandle
GetDrillPinMemory (DrillTypePtr Drill)
{
  PinTypePtr *pin;

  pin = Drill->Pin;

  /* realloc new memory if necessary and clear it */
  if (Drill->PinN >= Drill->PinMax)
    {
      Drill->PinMax += STEP_POINT;
      pin = MyRealloc (pin, Drill->PinMax * sizeof (PinTypeHandle),
		       "GetDrillPinMemory()");
      Drill->Pin = pin;
      memset (pin + Drill->PinN, 0, STEP_POINT * sizeof (PinTypeHandle));
    }
  return (pin + Drill->PinN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for a Drill, allocates memory if necessary
 */
DrillTypePtr
GetDrillInfoDrillMemory (DrillInfoTypePtr DrillInfo)
{
  DrillTypePtr drill = DrillInfo->Drill;

  /* realloc new memory if necessary and clear it */
  if (DrillInfo->DrillN >= DrillInfo->DrillMax)
    {
      DrillInfo->DrillMax += STEP_DRILL;
      drill = MyRealloc (drill, DrillInfo->DrillMax * sizeof (DrillType),
			 "GetDrillInfoDrillMemory()");
      DrillInfo->Drill = drill;
      memset (drill + DrillInfo->DrillN, 0, STEP_DRILL * sizeof (DrillType));
    }
  return (drill + DrillInfo->DrillN++);
}

/* ---------------------------------------------------------------------------
 * allocates memory with error handling
 */
void *
MyCalloc (size_t Number, size_t Size, const char *Text)
{
  void *p;

#ifdef MEM_DEBUG
  fprintf (stderr, "MyCalloc %d by %d from %s ", Number, Size, Text);
#endif
  /* InitComponentLookup() at least can ask for zero here, so return something
     |  that can be freed.
   */
  if (Number == 0)
    Number = 1;
  if (Size == 0)
    Size = 1;

  if ((p = calloc (Number, Size)) == NULL)
    MyFatal ("out of memory during malloc() in '%s'()\n",
	     (Text ? Text : "(unknown)"));
#ifdef MEM_DEBUG
  fprintf (stderr, "returned 0x%x\n", p);
#endif
  return (p);
}

void *
MyMalloc (size_t Size, const char *Text)
{
  void *p;

#ifdef MEM_DEBUG
  fprintf (stderr, "MyMalloc %d by %d from %s ", Number, Size, Text);
#endif
  /* avoid malloc of 0 bytes */
  if (Size == 0)
    Size = 1;
  if ((p = malloc (Size)) == NULL)
    MyFatal ("out of memory during malloc() in '%s'()\n",
	     (Text ? Text : "(unknown)"));
#ifdef MEM_DEBUG
  fprintf (stderr, "returned 0x%x\n", p);
#endif
  return (p);
}

/* ---------------------------------------------------------------------------
 * allocates memory with error handling
 * this is a save version because BSD doesn't support the
 * handling of NULL pointers in realloc()
 */
void *
MyRealloc (void *Ptr, size_t Size, const char *Text)
{
  void *p;

#ifdef MEM_DEBUG
  fprintf (stderr, "0x%x Realloc to %d from %s ", Ptr, Size, Text);
#endif
  if (Size == 0)
    Size = 1;
  p = Ptr ? realloc (Ptr, Size) : malloc (Size);
  if (!p)
    MyFatal ("out of memory during realloc() in '%s'()\n",
	     (Text ? Text : "(unknown)"));
#ifdef MEM_DEBUG
  fprintf (stderr, "returned 0x%x\n", p);
#endif
  return (p);
}

/* ---------------------------------------------------------------------------
 * allocates memory for a new string, does some error processing
 */
char *
MyStrdup (const char *S, const char *Text)
{
  char *p = NULL;

  /* bug-fix by Ulrich Pegelow (ulrpeg@bigcomm.gun.de) */
  if (S && ((p = strdup (S)) == NULL))
    MyFatal ("out of memory during g_strdup() in '%s'\n",
	     (Text ? Text : "(unknown)"));
#ifdef MEM_DEBUG
  fprintf (stderr, "g_strdup returning 0x%x\n", p);
#endif
  return (p);
}

/* ---------------------------------------------------------------------------
 * frees memory and sets pointer to NULL
 * too troublesome for modern C compiler,
 * warning: dereferencing type-punned pointer will break strict-aliasing rules
 * Use MYFREE() macro instead
 */
#if 0
void
MyFree (char **Ptr)
{
  SaveFree (*Ptr);
  *Ptr = NULL;
}
#endif

/* ---------------------------------------------------------------------------
 * frees memory used by a polygon
 */
void
FreePolygonMemory (PolygonTypePtr Polygon)
{
  if (Polygon)
    {
      MYFREE (Polygon->Points);
      MYFREE (Polygon->HoleIndex);
      if (Polygon->Clipped)
	poly_Free (&Polygon->Clipped);
      poly_FreeContours (&Polygon->NoHoles);
      memset (Polygon, 0, sizeof (PolygonType));
    }
}

/* ---------------------------------------------------------------------------
 * frees memory used by a box list
 */
void
FreeBoxListMemory (BoxListTypePtr Boxlist)
{
  if (Boxlist)
    {
      MYFREE (Boxlist->Box);
      memset (Boxlist, 0, sizeof (BoxListType));
    }
}

/* ---------------------------------------------------------------------------
 * frees memory used by a net 
 */
void
FreeNetListMemory (NetListTypePtr Netlist)
{
  if (Netlist)
    {
      NET_LOOP (Netlist);
      {
	FreeNetMemory (net);
      }
      END_LOOP;
      MYFREE (Netlist->Net);
      memset (Netlist, 0, sizeof (NetListType));
    }
}

/* ---------------------------------------------------------------------------
 * frees memory used by a net list
 */
void
FreeNetListListMemory (NetListListTypePtr Netlistlist)
{
  if (Netlistlist)
    {
      NETLIST_LOOP (Netlistlist);
      {
	FreeNetListMemory (netlist);
      }
      END_LOOP;
      MYFREE (Netlistlist->NetList);
      memset (Netlistlist, 0, sizeof (NetListListType));
    }
}

/* ---------------------------------------------------------------------------
 * frees memory used by a subnet 
 */
void
FreeNetMemory (NetTypePtr Net)
{
  if (Net)
    {
      MYFREE (Net->Connection);
      memset (Net, 0, sizeof (NetType));
    }
}
/* ---------------------------------------------------------------------------
 * frees memory used by an attribute list
 */
static void
FreeAttributeListMemory (AttributeListTypePtr list)
{
  int i;

  for (i = 0; i < list->Number; i++)
    {
      SaveFree (list->List[i].name);
      SaveFree (list->List[i].value);
    }
  SaveFree (list->List);
  list->List = NULL;
  list->Max = 0;
}

/* ---------------------------------------------------------------------------
 * frees memory used by an element
 */
void
FreeElementMemory (ElementTypePtr Element)
{
  if (Element)
    {
      ELEMENTNAME_LOOP (Element);
      {
	MYFREE (textstring);
      }
      END_LOOP;
      PIN_LOOP (Element);
      {
	MYFREE (pin->Name);
	MYFREE (pin->Number);
      }
      END_LOOP;
      PAD_LOOP (Element);
      {
	MYFREE (pad->Name);
	MYFREE (pad->Number);
      }
      END_LOOP;
      MYFREE (Element->Pin);
      MYFREE (Element->Pad);
      MYFREE (Element->Line);
      MYFREE (Element->Arc);
      FreeAttributeListMemory (&Element->Attributes);
      memset (Element, 0, sizeof (ElementType));
    }
}

/* ---------------------------------------------------------------------------
 * free memory used by PCB
 */
void
FreePCBMemory (PCBTypePtr PCBPtr)
{
  int i;

  if (PCBPtr)
    {
      MYFREE (PCBPtr->Name);
      MYFREE (PCBPtr->Filename);
      MYFREE (PCBPtr->PrintFilename);
      if (PCBPtr->Data)
	FreeDataMemory (PCBPtr->Data);
      MYFREE (PCBPtr->Data);
      /* release font symbols */
      for (i = 0; i <= MAX_FONTPOSITION; i++)
	MYFREE (PCBPtr->Font.Symbol[i].Line);
      FreeLibraryMemory (&PCBPtr->NetlistLib);
      FreeAttributeListMemory (&PCBPtr->Attributes);
      /* clear struct */
      memset (PCBPtr, 0, sizeof (PCBType));
    }
  else
    {
      fprintf (stderr, "Warning:  Tried to FreePCBMemory(null)\n");
    }
}

/* ---------------------------------------------------------------------------
 * free memory used by data struct
 */
void
FreeDataMemory (DataTypePtr Data)
{
  LayerTypePtr layer;
  int i;

  if (Data)
    {
      VIA_LOOP (Data);
      {
	MYFREE (via->Name);
      }
      END_LOOP;
      MYFREE (Data->Via);
      ELEMENT_LOOP (Data);
      {
	FreeElementMemory (element);
      }
      END_LOOP;
      MYFREE (Data->Element);
      MYFREE (Data->Rat);

      for (layer = Data->Layer, i = 0; i < MAX_LAYER + 2; layer++, i++)
	{
	  FreeAttributeListMemory (&layer->Attributes);
	  TEXT_LOOP (layer);
	  {
	    MYFREE (text->TextString);
	  }
	  END_LOOP;
	  if (layer->Name)
	    MYFREE (layer->Name);
	  LINE_LOOP (layer);
	  {
	    if (line->Number)
	      MYFREE (line->Number);
	  }
	  END_LOOP;
	  MYFREE (layer->Line);
	  MYFREE (layer->Arc);
	  MYFREE (layer->Text);
	  POLYGON_LOOP (layer);
	  {
	    FreePolygonMemory (polygon);
	  }
	  END_LOOP;
	  MYFREE (layer->Polygon);
	  if (layer->line_tree)
	    r_destroy_tree (&layer->line_tree);
	  if (layer->arc_tree)
	    r_destroy_tree (&layer->arc_tree);
	  if (layer->text_tree)
	    r_destroy_tree (&layer->text_tree);
	  if (layer->polygon_tree)
	    r_destroy_tree (&layer->polygon_tree);
	}

      if (Data->element_tree)
	r_destroy_tree (&Data->element_tree);
      for (i = 0; i < MAX_ELEMENTNAMES; i++)
	if (Data->name_tree[i])
	  r_destroy_tree (&Data->name_tree[i]);
      if (Data->via_tree)
	r_destroy_tree (&Data->via_tree);
      if (Data->pin_tree)
	r_destroy_tree (&Data->pin_tree);
      if (Data->pad_tree)
	r_destroy_tree (&Data->pad_tree);
      if (Data->rat_tree)
	r_destroy_tree (&Data->rat_tree);
      /* clear struct */
      memset (Data, 0, sizeof (DataType));
    }
  else
    {
      fprintf (stderr, "Warning:  Tried to FreeDataMemory(null)\n");
    }
}

/* ---------------------------------------------------------------------------
 * releases the memory that's allocated by the library
 */
void
FreeLibraryMemory (LibraryTypePtr lib)
{
  MENU_LOOP (lib);
  {
    ENTRY_LOOP (menu);
    {
      SaveFree ((void *) entry->AllocatedMemory);
      SaveFree ((void *) entry->ListEntry);
    }
    END_LOOP;
    SaveFree ((void *) menu->Entry);
    SaveFree ((void *) menu->Name);
  }
  END_LOOP;
  SaveFree ((void *) lib->Menu);

  /* clear struct */
  memset (lib, 0, sizeof (LibraryType));
}

/* ---------------------------------------------------------------------------
 * a 'save' free routine which first does a quick check if the pointer
 * is zero. The routine isn't implemented as a macro to make additional
 * safety features easier to implement
 */
void
SaveFree (void *Ptr)
{
#ifdef MEM_DEBUG
  fprintf (stderr, "Freeing 0x%x\n", Ptr);
#endif
  if (Ptr)
    free (Ptr);
}

/* ---------------------------------------------------------------------------
 * reallocates memory for a dynamic length string if necessary
 */
static void
DSRealloc (DynamicStringTypePtr Ptr, size_t Length)
{
  int input_null = (Ptr->Data == NULL);
  if (input_null || Length >= Ptr->MaxLength)
    {
      Ptr->MaxLength = Length + 512;
      Ptr->Data = MyRealloc (Ptr->Data, Ptr->MaxLength, "ReallocDS()");
      if (input_null)
	Ptr->Data[0] = '\0';
    }
}

/* ---------------------------------------------------------------------------
 * adds one character to a dynamic string
 */
void
DSAddCharacter (DynamicStringTypePtr Ptr, char Char)
{
  size_t position = Ptr->Data ? strlen (Ptr->Data) : 0;

  DSRealloc (Ptr, position + 1);
  Ptr->Data[position++] = Char;
  Ptr->Data[position] = '\0';
}

/* ---------------------------------------------------------------------------
 * add a string to a dynamic string
 */
void
DSAddString (DynamicStringTypePtr Ptr, const char *S)
{
  size_t position = Ptr->Data ? strlen (Ptr->Data) : 0;

  if (S && *S)
    {
      DSRealloc (Ptr, position + 1 + strlen (S));
      strcat (&Ptr->Data[position], S);
    }
}

/* ----------------------------------------------------------------------
 * clears a dynamic string
 */
void
DSClearString (DynamicStringTypePtr Ptr)
{
  if (Ptr->Data)
    Ptr->Data[0] = '\0';
}

/* ---------------------------------------------------------------------------
 * strips leading and trailing blanks from the passed string and
 * returns a pointer to the new 'duped' one or NULL if the old one
 * holds only white space characters
 */
char *
StripWhiteSpaceAndDup (char *S)
{
  char *p1, *p2;
  size_t length;

  if (!S || !*S)
    return (NULL);

  /* strip leading blanks */
  for (p1 = S; *p1 && isspace ((int) *p1); p1++);

  /* strip trailing blanks and get string length */
  length = strlen (p1);
  for (p2 = p1 + length - 1; length && isspace ((int) *p2); p2--, length--);

  /* string is not empty -> allocate memory */
  if (length)
    {
      p2 = MyRealloc (NULL, length + 1, "StripWhiteSpace()");
      strncpy (p2, p1, length);
      *(p2 + length) = '\0';
      return (p2);
    }
  else
    return (NULL);
}
