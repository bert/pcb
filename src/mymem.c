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

/* ---------------------------------------------------------------------------
 * local prototypes
 */
static void DSRealloc (DynamicStringTypePtr, size_t);


/* This API is quite new, provide a version here */
#if !GLIB_CHECK_VERSION (2, 28, 0)
static void
g_list_free_full (GList *list, GDestroyNotify free_func)
{
  g_list_foreach (list, (GFunc) free_func, NULL);
  g_list_free (list);
}
#endif

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
      ptr = (RubberbandTypePtr)realloc (ptr, Crosshair.AttachedObject.RubberbandMax *
                          sizeof (RubberbandType));
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
      ptr = (void **)realloc (ptr, list->PtrMax * sizeof (void *));
      list->Ptr = ptr;
      memset (ptr + list->PtrN, 0,
	      (list->PtrMax - list->PtrN) * sizeof (void *));
    }
  return (ptr + list->PtrN++);
}

void
FreePointerListMemory (PointerListTypePtr list)
{
  free (list->Ptr);
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
      box = (BoxTypePtr)realloc (box, Boxes->BoxMax * sizeof (BoxType));
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
      con = (ConnectionTypePtr)realloc (con, Net->ConnectionMax * sizeof (ConnectionType));
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
      net = (NetTypePtr)realloc (net, Netlist->NetMax * sizeof (NetType));
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
      netlist = (NetListTypePtr)realloc (netlist,
                         Netlistlist->NetListMax * sizeof (NetListType));
      Netlistlist->NetList = netlist;
      memset (netlist + Netlistlist->NetListN, 0,
	      STEP_POINT * sizeof (NetListType));
    }
  return (netlist + Netlistlist->NetListN++);
}
/* ---------------------------------------------------------------------------
 * get next slot for a pin, allocates memory if necessary
 */
PinType *
GetPinMemory (ElementType *element)
{
  PinType *new_obj;

  new_obj = g_slice_new0 (PinType);
  element->Pin = g_list_append (element->Pin, new_obj);
  element->PinN ++;

  return new_obj;
}

static void
FreePin (PinType *data)
{
  g_slice_free (PinType, data);
}

/* ---------------------------------------------------------------------------
 * get next slot for a pad, allocates memory if necessary
 */
PadType *
GetPadMemory (ElementType *element)
{
  PadType *new_obj;

  new_obj = g_slice_new0 (PadType);
  element->Pad = g_list_append (element->Pad, new_obj);
  element->PadN ++;

  return new_obj;
}

static void
FreePad (PadType *data)
{
  g_slice_free (PadType, data);
}

/* ---------------------------------------------------------------------------
 * get next slot for a via, allocates memory if necessary
 */
PinType *
GetViaMemory (DataType *data)
{
  PinType *new_obj;

  new_obj = g_slice_new0 (PinType);
  data->Via = g_list_append (data->Via, new_obj);
  data->ViaN ++;

  return new_obj;
}

static void
FreeVia (PinType *data)
{
  g_slice_free (PinType, data);
}

/* ---------------------------------------------------------------------------
 * get next slot for a Rat, allocates memory if necessary
 */
RatType *
GetRatMemory (DataType *data)
{
  RatType *new_obj;

  new_obj = g_slice_new0 (RatType);
  data->Rat = g_list_append (data->Rat, new_obj);
  data->RatN ++;

  return new_obj;
}

static void
FreeRat (RatType *data)
{
  g_slice_free (RatType, data);
}

/* ---------------------------------------------------------------------------
 * get next slot for a line, allocates memory if necessary
 */
LineType *
GetLineMemory (LayerType *layer)
{
  LineType *new_obj;

  new_obj = g_slice_new0 (LineType);
  layer->Line = g_list_append (layer->Line, new_obj);
  layer->LineN ++;

  return new_obj;
}

static void
FreeLine (LineType *data)
{
  g_slice_free (LineType, data);
}

/* ---------------------------------------------------------------------------
 * get next slot for an arc, allocates memory if necessary
 */
ArcTypePtr
GetArcMemory (LayerType *layer)
{
  ArcType *new_obj;

  new_obj = g_slice_new0 (ArcType);
  layer->Arc = g_list_append (layer->Arc, new_obj);
  layer->ArcN ++;

  return new_obj;
}

static void
FreeArc (ArcType *data)
{
  g_slice_free (ArcType, data);
}

/* ---------------------------------------------------------------------------
 * get next slot for a text object, allocates memory if necessary
 */
TextTypePtr
GetTextMemory (LayerType *layer)
{
  TextType *new_obj;

  new_obj = g_slice_new0 (TextType);
  layer->Text = g_list_append (layer->Text, new_obj);
  layer->TextN ++;

  return new_obj;
}

static void
FreeText (TextType *data)
{
  g_slice_free (TextType, data);
}

/* ---------------------------------------------------------------------------
 * get next slot for a polygon object, allocates memory if necessary
 */
PolygonType *
GetPolygonMemory (LayerType *layer)
{
  PolygonType *new_obj;

  new_obj = g_slice_new0 (PolygonType);
  layer->Polygon = g_list_append (layer->Polygon, new_obj);
  layer->PolygonN ++;

  return new_obj;
}

static void
FreePolygon (PolygonType *data)
{
  g_slice_free (PolygonType, data);
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
      points = (PointTypePtr)realloc (points, Polygon->PointMax * sizeof (PointType));
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
      holeindex = (Cardinal *)realloc (holeindex, Polygon->HoleIndexMax * sizeof (int));
      Polygon->HoleIndex = holeindex;
      memset (holeindex + Polygon->HoleIndexN, 0,
	      STEP_POLYGONHOLEINDEX * sizeof (int));
    }
  return (holeindex + Polygon->HoleIndexN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for an element, allocates memory if necessary
 */
ElementType *
GetElementMemory (DataType *data)
{
  ElementType *new_obj;

  new_obj = g_slice_new0 (ElementType);
  data->Element = g_list_append (data->Element, new_obj);
  data->ElementN ++;

  return new_obj;
}

static void
FreeElement (ElementType *data)
{
  g_slice_free (ElementType, data);
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
      menu = (LibraryMenuTypePtr)realloc (menu, lib->MenuMax * sizeof (LibraryMenuType));
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
      entry = (LibraryEntryTypePtr)realloc (entry, Menu->EntryMax * sizeof (LibraryEntryType));
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
      element = (ElementTypePtr *)realloc (element,
                         Drill->ElementMax * sizeof (ElementTypeHandle));
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
      pin = (PinTypePtr *)realloc (pin, Drill->PinMax * sizeof (PinTypeHandle));
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
      drill = (DrillTypePtr)realloc (drill, DrillInfo->DrillMax * sizeof (DrillType));
      DrillInfo->Drill = drill;
      memset (drill + DrillInfo->DrillN, 0, STEP_DRILL * sizeof (DrillType));
    }
  return (drill + DrillInfo->DrillN++);
}

/* ---------------------------------------------------------------------------
 * frees memory used by a polygon
 */
void
FreePolygonMemory (PolygonType *polygon)
{
  if (polygon == NULL)
    return;

  free (polygon->Points);
  free (polygon->HoleIndex);

  if (polygon->Clipped)
    poly_Free (&polygon->Clipped);
  poly_FreeContours (&polygon->NoHoles);

  memset (polygon, 0, sizeof (PolygonType));
}

/* ---------------------------------------------------------------------------
 * frees memory used by a box list
 */
void
FreeBoxListMemory (BoxListTypePtr Boxlist)
{
  if (Boxlist)
    {
      free (Boxlist->Box);
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
      free (Netlist->Net);
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
      free (Netlistlist->NetList);
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
      free (Net->Connection);
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
      free (list->List[i].name);
      free (list->List[i].value);
    }
  free (list->List);
  list->List = NULL;
  list->Max = 0;
}

/* ---------------------------------------------------------------------------
 * frees memory used by an element
 */
void
FreeElementMemory (ElementType *element)
{
  if (element == NULL)
    return;

  ELEMENTNAME_LOOP (element);
  {
    free (textstring);
  }
  END_LOOP;
  PIN_LOOP (element);
  {
    free (pin->Name);
    free (pin->Number);
  }
  END_LOOP;
  PAD_LOOP (element);
  {
    free (pad->Name);
    free (pad->Number);
  }
  END_LOOP;

  g_list_free_full (element->Pin,  (GDestroyNotify)FreePin);
  g_list_free_full (element->Pad,  (GDestroyNotify)FreePad);
  g_list_free_full (element->Line, (GDestroyNotify)FreeLine);
  g_list_free_full (element->Arc,  (GDestroyNotify)FreeArc);

  FreeAttributeListMemory (&element->Attributes);
  memset (element, 0, sizeof (ElementType));
}

/* ---------------------------------------------------------------------------
 * free memory used by PCB
 */
void
FreePCBMemory (PCBType *pcb)
{
  int i;

  if (pcb == NULL)
    return;

  free (pcb->Name);
  free (pcb->Filename);
  free (pcb->PrintFilename);
  FreeDataMemory (pcb->Data);
  free (pcb->Data);
  /* release font symbols */
  for (i = 0; i <= MAX_FONTPOSITION; i++)
    free (pcb->Font.Symbol[i].Line);
  FreeLibraryMemory (&pcb->NetlistLib);
  NetlistChanged (0);
  FreeAttributeListMemory (&pcb->Attributes);
  /* clear struct */
  memset (pcb, 0, sizeof (PCBType));
}

/* ---------------------------------------------------------------------------
 * free memory used by data struct
 */
void
FreeDataMemory (DataType *data)
{
  LayerTypePtr layer;
  int i;

  if (data == NULL)
    return;

  VIA_LOOP (data);
  {
    free (via->Name);
  }
  END_LOOP;
  g_list_free_full (data->Via, (GDestroyNotify)FreeVia);
  ELEMENT_LOOP (data);
  {
    FreeElementMemory (element);
  }
  END_LOOP;
  g_list_free_full (data->Element, (GDestroyNotify)FreeElement);
  g_list_free_full (data->Rat, (GDestroyNotify)FreeRat);

  for (layer = data->Layer, i = 0; i < MAX_LAYER + 2; layer++, i++)
    {
      FreeAttributeListMemory (&layer->Attributes);
      TEXT_LOOP (layer);
      {
        free (text->TextString);
      }
      END_LOOP;
      if (layer->Name)
        free (layer->Name);
      LINE_LOOP (layer);
      {
        if (line->Number)
          free (line->Number);
      }
      END_LOOP;
      g_list_free_full (layer->Line, (GDestroyNotify)FreeLine);
      g_list_free_full (layer->Arc, (GDestroyNotify)FreeArc);
      g_list_free_full (layer->Text, (GDestroyNotify)FreeText);
      POLYGON_LOOP (layer);
      {
        FreePolygonMemory (polygon);
      }
      END_LOOP;
      g_list_free_full (layer->Polygon, (GDestroyNotify)FreePolygon);
      if (layer->line_tree)
        r_destroy_tree (&layer->line_tree);
      if (layer->arc_tree)
        r_destroy_tree (&layer->arc_tree);
      if (layer->text_tree)
        r_destroy_tree (&layer->text_tree);
      if (layer->polygon_tree)
        r_destroy_tree (&layer->polygon_tree);
    }

  if (data->element_tree)
    r_destroy_tree (&data->element_tree);
  for (i = 0; i < MAX_ELEMENTNAMES; i++)
    if (data->name_tree[i])
      r_destroy_tree (&data->name_tree[i]);
  if (data->via_tree)
    r_destroy_tree (&data->via_tree);
  if (data->pin_tree)
    r_destroy_tree (&data->pin_tree);
  if (data->pad_tree)
    r_destroy_tree (&data->pad_tree);
  if (data->rat_tree)
    r_destroy_tree (&data->rat_tree);
  /* clear struct */
  memset (data, 0, sizeof (DataType));
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
      free (entry->AllocatedMemory);
      free (entry->ListEntry);
    }
    END_LOOP;
    free (menu->Entry);
    free (menu->Name);
  }
  END_LOOP;
  free (lib->Menu);

  /* clear struct */
  memset (lib, 0, sizeof (LibraryType));
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
      Ptr->Data = (char *)realloc (Ptr->Data, Ptr->MaxLength);
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
StripWhiteSpaceAndDup (const char *S)
{
  const char *p1, *p2;
  char *copy;
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
      copy = (char *)realloc (NULL, length + 1);
      strncpy (copy, p1, length);
      copy[length] = '\0';
      return (copy);
    }
  else
    return (NULL);
}
