/*!
 * \file src/buffer.c
 *
 * \brief Functions used by paste- and move/copy buffer.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include "global.h"

#include "buffer.h"
#include "copy.h"
#include "create.h"
#include "crosshair.h"
#include "data.h"
#include "error.h"
#include "mymem.h"
#include "mirror.h"
#include "misc.h"
#include "parse_l.h"
#include "polygon.h"
#include "rats.h"
#include "rotate.h"
#include "remove.h"
#include "rtree.h"
#include "search.h"
#include "select.h"
#include "set.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void *AddViaToBuffer (PinType *);
static void *AddLineToBuffer (LayerType *, LineType *);
static void *AddArcToBuffer (LayerType *, ArcType *);
static void *AddRatToBuffer (RatType *);
static void *AddTextToBuffer (LayerType *, TextType *);
static void *AddPolygonToBuffer (LayerType *, PolygonType *);
static void *AddElementToBuffer (ElementType *);
static void *MoveViaToBuffer (PinType *);
static void *MoveLineToBuffer (LayerType *, LineType *);
static void *MoveArcToBuffer (LayerType *, ArcType *);
static void *MoveRatToBuffer (RatType *);
static void *MoveTextToBuffer (LayerType *, TextType *);
static void *MovePolygonToBuffer (LayerType *, PolygonType *);
static void *MoveElementToBuffer (ElementType *);
static void SwapBuffer (BufferType *);

#define ARG(n) (argc > (n) ? argv[n] : 0)

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static DataType *Dest, *Source;

static ObjectFunctionType AddBufferFunctions = {
  AddLineToBuffer,
  AddTextToBuffer,
  AddPolygonToBuffer,
  AddViaToBuffer,
  AddElementToBuffer,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  AddArcToBuffer,
  AddRatToBuffer
}, MoveBufferFunctions =

{
MoveLineToBuffer,
    MoveTextToBuffer,
    MovePolygonToBuffer,
    MoveViaToBuffer,
    MoveElementToBuffer,
    NULL, NULL, NULL, NULL, NULL, MoveArcToBuffer, MoveRatToBuffer};

static int ExtraFlag = 0;

/*!
 * \brief Copies a via to paste buffer.
 */
static void *
AddViaToBuffer (PinType *Via)
{
  return (CreateNewVia (Dest, Via->X, Via->Y, Via->Thickness, Via->Clearance,
			Via->Mask, Via->DrillingHole, Via->Name,
			MaskFlags (Via->Flags, NOCOPY_FLAGS | ExtraFlag)));
}

/*!
 * \brief Copies a rat-line to paste buffer.
 */
static void *
AddRatToBuffer (RatType *Rat)
{
  return (CreateNewRat (Dest, Rat->Point1.X, Rat->Point1.Y,
			Rat->Point2.X, Rat->Point2.Y, Rat->group1,
			Rat->group2, Rat->Thickness,
			MaskFlags (Rat->Flags, NOCOPY_FLAGS | ExtraFlag)));
}

/*!
 * \brief Copies a line to buffer.
 */
static void *
AddLineToBuffer (LayerType *Layer, LineType *Line)
{
  LineType *line;
  LayerType *layer = &Dest->Layer[GetLayerNumber (Source, Layer)];

  line = CreateNewLineOnLayer (layer, Line->Point1.X, Line->Point1.Y,
			       Line->Point2.X, Line->Point2.Y,
			       Line->Thickness, Line->Clearance,
			       MaskFlags (Line->Flags,
					  NOCOPY_FLAGS | ExtraFlag));
  if (line && Line->Number)
    line->Number = strdup (Line->Number);
  return (line);
}

/*!
 * \brief Copies an arc to buffer.
 */
static void *
AddArcToBuffer (LayerType *Layer, ArcType *Arc)
{
  LayerType *layer = &Dest->Layer[GetLayerNumber (Source, Layer)];

  return (CreateNewArcOnLayer (layer, Arc->X, Arc->Y,
			       Arc->Width, Arc->Height, Arc->StartAngle, Arc->Delta,
			       Arc->Thickness, Arc->Clearance,
			       MaskFlags (Arc->Flags,
					  NOCOPY_FLAGS | ExtraFlag)));
}

/*!
 * \brief Copies a text to buffer.
 */
static void *
AddTextToBuffer (LayerType *Layer, TextType *Text)
{
  LayerType *layer = &Dest->Layer[GetLayerNumber (Source, Layer)];

  return (CreateNewText (layer, Text->Font, Text->X, Text->Y,
			 Text->Direction, Text->Scale, Text->TextString,
			 MaskFlags (Text->Flags, ExtraFlag)));
}

/*!
 * \brief Copies a polygon to buffer.
 */
static void *
AddPolygonToBuffer (LayerType *Layer, PolygonType *Polygon)
{
  LayerType *layer = &Dest->Layer[GetLayerNumber (Source, Layer)];
  PolygonType *polygon;

  polygon = CreateNewPolygon (layer, Polygon->Flags);
  CopyPolygonLowLevel (polygon, Polygon);

  /* Update the polygon r-tree. Unlike similarly named functions for
   * other objects, CreateNewPolygon does not do this as it creates a
   * skeleton polygon object, which won't have correct bounds.
   */
  if (!layer->polygon_tree)
    layer->polygon_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (layer->polygon_tree, (BoxType *)polygon, 0);

  CLEAR_FLAG (NOCOPY_FLAGS | ExtraFlag, polygon);
  return (polygon);
}

/*!
 * \brief Copies a element to buffer.
 */
static void *
AddElementToBuffer (ElementType *Element)
{
  return CopyElementLowLevel (Dest, Element, false, 0, 0, NOCOPY_FLAGS | ExtraFlag);
}

/*!
 * \brief Moves a via to paste buffer without allocating memory for the
 * name.
 */
static void *
MoveViaToBuffer (PinType *via)
{
  RestoreToPolygon (Source, VIA_TYPE, via, via);

  r_delete_entry (Source->via_tree, (BoxType *) via);
  Source->Via = g_list_remove (Source->Via, via);
  Source->ViaN --;
  Dest->Via = g_list_append (Dest->Via, via);
  Dest->ViaN ++;

  CLEAR_FLAG (WARNFLAG | NOCOPY_FLAGS, via);

  if (!Dest->via_tree)
    Dest->via_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (Dest->via_tree, (BoxType *)via, 0);
  ClearFromPolygon (Dest, VIA_TYPE, via, via);
  return via;
}

/*!
 * \brief Moves a rat-line to paste buffer.
 */
static void *
MoveRatToBuffer (RatType *rat)
{
  r_delete_entry (Source->rat_tree, (BoxType *)rat);

  Source->Rat = g_list_remove (Source->Rat, rat);
  Source->RatN --;
  Dest->Rat = g_list_append (Dest->Rat, rat);
  Dest->RatN ++;

  CLEAR_FLAG (NOCOPY_FLAGS, rat);

  if (!Dest->rat_tree)
    Dest->rat_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (Dest->rat_tree, (BoxType *)rat, 0);
  return rat;
}

/*!
 * \brief Moves a line to buffer.
 */
static void *
MoveLineToBuffer (LayerType *layer, LineType *line)
{
  LayerType *lay = &Dest->Layer[GetLayerNumber (Source, layer)];

  RestoreToPolygon (Source, LINE_TYPE, layer, line);
  r_delete_entry (layer->line_tree, (BoxType *)line);

  layer->Line = g_list_remove (layer->Line, line);
  layer->LineN --;
  lay->Line = g_list_append (lay->Line, line);
  lay->LineN ++;

  CLEAR_FLAG (NOCOPY_FLAGS, line);

  if (!lay->line_tree)
    lay->line_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (lay->line_tree, (BoxType *)line, 0);
  ClearFromPolygon (Dest, LINE_TYPE, lay, line);
  return (line);
}

/*!
 * \brief Moves an arc to buffer.
 */
static void *
MoveArcToBuffer (LayerType *layer, ArcType *arc)
{
  LayerType *lay = &Dest->Layer[GetLayerNumber (Source, layer)];

  RestoreToPolygon (Source, ARC_TYPE, layer, arc);
  r_delete_entry (layer->arc_tree, (BoxType *)arc);

  layer->Arc = g_list_remove (layer->Arc, arc);
  layer->ArcN --;
  lay->Arc = g_list_append (lay->Arc, arc);
  lay->ArcN ++;

  CLEAR_FLAG (NOCOPY_FLAGS, arc);

  if (!lay->arc_tree)
    lay->arc_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (lay->arc_tree, (BoxType *)arc, 0);
  ClearFromPolygon (Dest, ARC_TYPE, lay, arc);
  return (arc);
}

/*!
 * \brief Moves a text to buffer without allocating memory for the name.
 */
static void *
MoveTextToBuffer (LayerType *layer, TextType *text)
{
  LayerType *lay = &Dest->Layer[GetLayerNumber (Source, layer)];

  r_delete_entry (layer->text_tree, (BoxType *)text);
  RestoreToPolygon (Source, TEXT_TYPE, layer, text);

  layer->Text = g_list_remove (layer->Text, text);
  layer->TextN --;
  lay->Text = g_list_append (lay->Text, text);
  lay->TextN ++;

  if (!lay->text_tree)
    lay->text_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (lay->text_tree, (BoxType *)text, 0);
  ClearFromPolygon (Dest, TEXT_TYPE, lay, text);
  return (text);
}

/*!
 * \brief Moves a polygon to buffer.
 *
 * Doesn't allocate memory for the points.
 */
static void *
MovePolygonToBuffer (LayerType *layer, PolygonType *polygon)
{
  LayerType *lay = &Dest->Layer[GetLayerNumber (Source, layer)];

  r_delete_entry (layer->polygon_tree, (BoxType *)polygon);

  layer->Polygon = g_list_remove (layer->Polygon, polygon);
  layer->PolygonN --;
  lay->Polygon = g_list_append (lay->Polygon, polygon);
  lay->PolygonN ++;

  CLEAR_FLAG (NOCOPY_FLAGS, polygon);

  if (!lay->polygon_tree)
    lay->polygon_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (lay->polygon_tree, (BoxType *)polygon, 0);
  return (polygon);
}

/*!
 * \brief Moves a element to buffer without allocating memory for
 * pins/names.
 */
static void *
MoveElementToBuffer (ElementType *element)
{
  /*
   * Delete the element from the source (remove it from trees,
   * restore to polygons)
   */
  r_delete_element (Source, element);

  Source->Element = g_list_remove (Source->Element, element);
  Source->ElementN --;
  Dest->Element = g_list_append (Dest->Element, element);
  Dest->ElementN ++;

  PIN_LOOP (element);
  {
    RestoreToPolygon(Source, PIN_TYPE, element, pin);
    CLEAR_FLAG (WARNFLAG | NOCOPY_FLAGS, pin);
  }
  END_LOOP;
  PAD_LOOP (element);
  {
    RestoreToPolygon(Source, PAD_TYPE, element, pad);
    CLEAR_FLAG (WARNFLAG | NOCOPY_FLAGS, pad);
  }
  END_LOOP;
  SetElementBoundingBox (Dest, element);
  /*
   * Now clear the from the polygons in the destination
   */
  PIN_LOOP (element);
  {
    ClearFromPolygon (Dest, PIN_TYPE, element, pin);
  }
  END_LOOP;
  PAD_LOOP (element);
  {
    ClearFromPolygon (Dest, PAD_TYPE, element, pad);
  }
  END_LOOP;

  return element;
}

/*!
 * \brief Calculates the bounding box of the buffer.
 */
void
SetBufferBoundingBox (BufferType *Buffer)
{
  BoxType *box = GetDataBoundingBox (Buffer->Data);

  if (box)
    Buffer->BoundingBox = *box;
}

/*!
 * \brief Clears the contents of the paste buffer.
 */
void
ClearBuffer (BufferType *Buffer)
{
  if (Buffer && Buffer->Data)
    {
      FreeDataMemory (Buffer->Data);
      Buffer->Data->pcb = PCB;
    }
}

/*!
 * \brief Copies all selected and visible objects to the paste buffer.
 *
 * \return true if any objects have been removed.
 */
void
AddSelectedToBuffer (BufferType *Buffer, Coord X, Coord Y, bool LeaveSelected)
{
  /* switch crosshair off because adding objects to the pastebuffer
   * may change the 'valid' area for the cursor
   */
  if (!LeaveSelected)
    ExtraFlag = SELECTEDFLAG;
  notify_crosshair_change (false);
  Source = PCB->Data;
  Dest = Buffer->Data;
  SelectedOperation (&AddBufferFunctions, false, ALL_TYPES);

  /* set origin to passed or current position */
  if (X || Y)
    {
      Buffer->X = X;
      Buffer->Y = Y;
    }
  else
    {
      Buffer->X = Crosshair.X;
      Buffer->Y = Crosshair.Y;
    }
  notify_crosshair_change (true);
  ExtraFlag = 0;
}

/*!
 * \brief Loads element data from file/library into buffer.
 *
 * Parse the file with disabled 'PCB mode' (see parser).
 *
 * \return false on error, if successful, update some other stuff and
 * reposition the pastebuffer.
 */
bool
LoadElementToBuffer (BufferType *Buffer, char *Name, bool FromFile)
{
  ElementType *element;

  ClearBuffer (Buffer);
  if (FromFile)
    {
      if (!ParseElementFile (Buffer->Data, Name))
	{
	  if (Settings.ShowBottomSide)
	    SwapBuffer (Buffer);
	  SetBufferBoundingBox (Buffer);
	  if (Buffer->Data->ElementN)
	    {
	      element = Buffer->Data->Element->data;
	      Buffer->X = element->MarkX;
	      Buffer->Y = element->MarkY;
	    }
	  else
	    {
	      Buffer->X = 0;
	      Buffer->Y = 0;
	    }
	  return (true);
	}
    }
  else
    {
      if (!ParseLibraryEntry (Buffer->Data, Name)
	  && Buffer->Data->ElementN != 0)
	{
	  element = Buffer->Data->Element->data;

	  /* always add elements using top-side coordinates */
	  if (Settings.ShowBottomSide)
	    MirrorElementCoordinates (Buffer->Data, element, 0);
	  SetElementBoundingBox (Buffer->Data, element);

	  /* set buffer offset to 'mark' position */
	  Buffer->X = element->MarkX;
	  Buffer->Y = element->MarkY;
	  SetBufferBoundingBox (Buffer);
	  return (true);
	}
    }
  /* release memory which might have been acquired */
  ClearBuffer (Buffer);
  return (false);
}


typedef struct {
  char *footprint;
  int footprint_allocated;
  int menu_idx;
  int entry_idx;
} FootprintHashEntry;

static FootprintHashEntry *footprint_hash = 0;
int footprint_hash_size = 0;

void
clear_footprint_hash ()
{
  int i;
  if (!footprint_hash)
    return;
  for (i=0; i<footprint_hash_size; i++)
    if (footprint_hash[i].footprint_allocated)
      free (footprint_hash[i].footprint);
  free (footprint_hash);
  footprint_hash = NULL;
  footprint_hash_size = 0;
}

/*!
 * \brief Used to sort footprint pointer entries.
 *
 * \note We include the index numbers so that same-named footprints are
 * sorted by the library search order.
 */
static int
footprint_hash_cmp (const void *va, const void *vb)
{
  int i;
  FootprintHashEntry *a = (FootprintHashEntry *)va;
  FootprintHashEntry *b = (FootprintHashEntry *)vb;

  i = strcmp (a->footprint, b->footprint);
  if (i == 0)
    i = a->menu_idx - b->menu_idx;
  if (i == 0)
    i = a->entry_idx - b->entry_idx;
  return i;
}

void
make_footprint_hash ()
{
  int i, j;
  char *fp;
  int num_entries = 0;

  clear_footprint_hash ();

  for (i=0; i<Library.MenuN; i++)
    for (j=0; j<Library.Menu[i].EntryN; j++)
      num_entries ++;
  footprint_hash = (FootprintHashEntry *)malloc (num_entries * sizeof(FootprintHashEntry));
  num_entries = 0;

  /* There are two types of library entries.  The file-based types
     have a Template of (char *)-1 and the AllocatedMemory is the full
     path to the footprint file.  The m4 ones have the footprint name
     in brackets in the description.  */
  for (i=0; i<Library.MenuN; i++)
    {
#ifdef DEBUG
  printf("In make_footprint_hash, looking for footprints in %s\n", 
	 Library.Menu[i].directory);
#endif

    for (j=0; j<Library.Menu[i].EntryN; j++)
	{
	  footprint_hash[num_entries].menu_idx = i;
	  footprint_hash[num_entries].entry_idx = j;
	  if (Library.Menu[i].Entry[j].Template == (char *) -1) 
          /* file */
	    {
#ifdef DEBUG
/*	      printf(" ... Examining file %s\n", Library.Menu[i].Entry[j].AllocatedMemory); */
#endif
	      fp = strrchr (Library.Menu[i].Entry[j].AllocatedMemory, '/');

	      if (!fp)
		fp = strrchr (Library.Menu[i].Entry[j].AllocatedMemory, '\\');

	      if (fp)
		fp ++;
	      else 
		fp = Library.Menu[i].Entry[j].AllocatedMemory;

#ifdef DEBUG
/* 	      printf(" ... found file footprint %s\n",  fp); */
#endif
	        
	      footprint_hash[num_entries].footprint = fp;
	      footprint_hash[num_entries].footprint_allocated = 0;
	    }
	  else 
          /* m4 */
	    {
	      fp = strrchr (Library.Menu[i].Entry[j].Description, '[');
	      if (fp)
		{
		  footprint_hash[num_entries].footprint = strdup (fp+1);
		  footprint_hash[num_entries].footprint_allocated = 1;
		  fp = strchr (footprint_hash[num_entries].footprint, ']');
		  if (fp)
		    *fp = 0;
		}
	      else
		{
		  fp = Library.Menu[i].Entry[j].Description;
		  footprint_hash[num_entries].footprint = fp;
		  footprint_hash[num_entries].footprint_allocated = 0;
		}
	    }
	  num_entries ++;
	}
    }

  footprint_hash_size = num_entries;
  qsort (footprint_hash, num_entries, sizeof(footprint_hash[0]), footprint_hash_cmp);
/*
#ifdef DEBUG
  printf("       found footprints:  \n");
  for (i=0; i<num_entries; i++)
    printf("[%s]\n", footprint_hash[i].footprint);
#endif
*/
}

/*!
 * \brief Searches for the given element by "footprint" name, and loads
 * it into the buffer.
 *
 * Figuring out which library entry is the one we want is a little
 * tricky.  For file-based footprints, it's just a matter of finding
 * the first match in the search list.  For m4-based footprints you
 * need to know what magic to pass to the m4 functions.  Fortunately,
 * the footprint needed is determined when we build the m4 libraries
 * and stored as a comment in the description, so we can search for
 * that to find the magic we need.  We use a hash to store the
 * corresponding footprints and pointers to the library tree so we can
 * quickly find the various bits we need to load a given
 * footprint.
 */
FootprintHashEntry *
search_footprint_hash (const char *footprint)
{
  int i, min, max, c;

  /* Standard binary search */

  min = -1;
  max = footprint_hash_size;

  while (max - min > 1)
    {
      i = (min+max)/2;
      c = strcmp (footprint, footprint_hash[i].footprint);
      if (c < 0)
	max = i;
      else if (c > 0)
	min = i;
      else
	{
	  /* We want to return the first match, not just any match.  */
	  while (i > 0
		 && strcmp (footprint, footprint_hash[i-1].footprint) == 0)
	    i--;
	  return & footprint_hash[i];
	}
    }
  return NULL;
}

/*!
 * \brief .
 *
 * \return zero on success, non-zero on error.
 */
int
LoadFootprintByName (BufferType *Buffer, char *Footprint)
{
  int i;
  FootprintHashEntry *fpe;
  LibraryMenuType *menu;
  LibraryEntryType *entry;
  char *with_fp = NULL;

  if (!footprint_hash)
    make_footprint_hash ();

  fpe = search_footprint_hash (Footprint);
  if (!fpe)
    {
      with_fp = Concat (Footprint, ".fp", NULL);
      fpe = search_footprint_hash (with_fp);
      if (fpe)
	Footprint = with_fp;
    }
  if (!fpe)
    {
      Message(_("Unable to load footprint %s\n"), Footprint);
      return 1;
    }

  menu = & Library.Menu[fpe->menu_idx];
  entry = & menu->Entry[fpe->entry_idx];

  if (entry->Template == (char *) -1)
    {
      i = LoadElementToBuffer (Buffer, entry->AllocatedMemory, true);
      if (with_fp)
	free (with_fp);
      return i ? 0 : 1;
    }
  else
    {
      char *args;

      args = Concat("'", EMPTY (entry->Template), "' '",
		    EMPTY (entry->Value), "' '", EMPTY (entry->Package), "'", NULL);
      i = LoadElementToBuffer (Buffer, args, false);

      free (args);
      if (with_fp)
	free (with_fp);
      return i ? 0 : 1;
    }

#ifdef DEBUG
  {
    int j;
    printf("Library path: %s\n", Settings.LibraryPath);
    printf("Library tree: %s\n", Settings.LibraryTree);

    printf("Library:\n");
    for (i=0; i<Library.MenuN; i++)
      {
	printf("  [%02d] Name: %s\n", i, Library.Menu[i].Name);
	printf("       Dir:  %s\n", Library.Menu[i].directory);
	printf("       Sty:  %s\n", Library.Menu[i].Style);
	for (j=0; j<Library.Menu[i].EntryN; j++)
	  {
	    printf("       [%02d] E: %s\n", j, Library.Menu[i].Entry[j].ListEntry);
	    if (Library.Menu[i].Entry[j].Template == (char *) -1)
	      printf("            A: %s\n", Library.Menu[i].Entry[j].AllocatedMemory);
	    else
	      {
		printf("            T: %s\n", Library.Menu[i].Entry[j].Template);
		printf("            P: %s\n", Library.Menu[i].Entry[j].Package);
		printf("            V: %s\n", Library.Menu[i].Entry[j].Value);
		printf("            D: %s\n", Library.Menu[i].Entry[j].Description);
	      }
	    if (j == 10)
	      break;
	  }
      }
  }
#endif
}


static const char loadfootprint_syntax[] =
  N_("LoadFootprint(filename[,refdes,value])");

static const char loadfootprint_help[] =
  N_("Loads a single footprint by name.");

/* %start-doc actions LoadFootprint

Loads a single footprint by name, rather than by reference or through
the library.  If a refdes and value are specified, those are inserted
into the footprint as well.  The footprint remains in the paste buffer.

%end-doc */

/*!
 * \brief This action is called from ActionElementAddIf().
 */
int
LoadFootprint (int argc, char **argv, Coord x, Coord y)
{
  char *name = ARG(0);
  char *refdes = ARG(1);
  char *value = ARG(2);
  ElementType *e;

  if (!name)
    AFAIL (loadfootprint);

  if (LoadFootprintByName (PASTEBUFFER, name))
    return 1;

  if (PASTEBUFFER->Data->ElementN == 0)
    {
      Message(_("Footprint %s contains no elements"), name);
      return 1;
    }
  if (PASTEBUFFER->Data->ElementN > 1)
    {
      Message(_("Footprint %s contains multiple elements"), name);
      return 1;
    }

  e = PASTEBUFFER->Data->Element->data;

  if (e->Name[0].TextString)
    free (e->Name[0].TextString);
  e->Name[0].TextString = strdup (name);

  if (e->Name[1].TextString)
    free (e->Name[1].TextString);
  e->Name[1].TextString = refdes ? strdup (refdes) : 0;

  if (e->Name[2].TextString)
    free (e->Name[2].TextString);
  e->Name[2].TextString = value ? strdup (value) : 0;

  return 0;
}

/*!
 * \brief Break buffer element into pieces.
 */
bool
SmashBufferElement (BufferType *Buffer)
{
  ElementType *element;
  Cardinal group;
  LayerType *top_layer, *bottom_layer;

  if (Buffer->Data->ElementN != 1)
    {
      Message (_("Error!  Buffer doesn't contain a single element\n"));
      return (false);
    }
  /*
   * At this point the buffer should contain just a single element.
   * Now we detach the single element from the buffer and then clear the
   * buffer, ready to receive the smashed elements.  As a result of detaching
   * it the single element is orphaned from the buffer and thus will not be
   * free()'d by FreeDataMemory (called via ClearBuffer).  This leaves it
   * around for us to smash bits off it.  It then becomes our responsibility,
   * however, to free the single element when we're finished with it.
   */
  element = Buffer->Data->Element->data;
  Buffer->Data->Element = NULL;
  Buffer->Data->ElementN = 0;
  ClearBuffer (Buffer);
  ELEMENTLINE_LOOP (element);
  {
    CreateNewLineOnLayer (&Buffer->Data->SILKLAYER,
			  line->Point1.X, line->Point1.Y,
			  line->Point2.X, line->Point2.Y,
			  line->Thickness, 0, NoFlags ());
    if (line)
      line->Number = STRDUP (NAMEONPCB_NAME (element));
  }
  END_LOOP;
  ARC_LOOP (element);
  {
    CreateNewArcOnLayer (&Buffer->Data->SILKLAYER,
			 arc->X, arc->Y, arc->Width, arc->Height, arc->StartAngle,
			 arc->Delta, arc->Thickness, 0, NoFlags ());
  }
  END_LOOP;
  PIN_LOOP (element);
  {
    FlagType f = NoFlags ();
    AddFlags (f, VIAFLAG);
    if (TEST_FLAG (HOLEFLAG, pin))
      AddFlags (f, HOLEFLAG);

    CreateNewVia (Buffer->Data, pin->X, pin->Y,
		  pin->Thickness, pin->Clearance, pin->Mask,
		  pin->DrillingHole, pin->Number, f);
  }
  END_LOOP;
  group = GetLayerGroupNumberBySide (SWAP_IDENT ? BOTTOM_SIDE : TOP_SIDE);
  top_layer = &Buffer->Data->Layer[PCB->LayerGroups.Entries[group][0]];
  group = GetLayerGroupNumberBySide (SWAP_IDENT ? TOP_SIDE : BOTTOM_SIDE);
  bottom_layer = &Buffer->Data->Layer[PCB->LayerGroups.Entries[group][0]];
  PAD_LOOP (element);
  {
    LineType *line;
    line = CreateNewLineOnLayer (TEST_FLAG (ONSOLDERFLAG, pad) ? bottom_layer : top_layer,
				 pad->Point1.X, pad->Point1.Y,
				 pad->Point2.X, pad->Point2.Y,
				 pad->Thickness, pad->Clearance, NoFlags ());
    if (line)
      line->Number = STRDUP (pad->Number);
  }
  END_LOOP;
  FreeElementMemory (element);
  g_slice_free (ElementType, element);
  return (true);
}

/*!
 * \brief See if a polygon is a rectangle.
 *
 * If so, canonicalize it.
 */

static int
polygon_is_rectangle (PolygonType *poly)
{
  int i, best;
  PointType temp[4];
  if (poly->PointN != 4 || poly->HoleIndexN != 0)
    return 0;
  best = 0;
  for (i=1; i<4; i++)
    if (poly->Points[i].X < poly->Points[best].X
	|| poly->Points[i].Y < poly->Points[best].Y)
      best = i;
  for (i=0; i<4; i++)
    temp[i] = poly->Points[(i+best)%4];
  if (temp[0].X == temp[1].X)
    memcpy (poly->Points, temp, sizeof(temp));
  else
    {
      /* reverse them */
      poly->Points[0] = temp[0];
      poly->Points[1] = temp[3];
      poly->Points[2] = temp[2];
      poly->Points[3] = temp[1];
    }
  if (poly->Points[0].X == poly->Points[1].X
      && poly->Points[1].Y == poly->Points[2].Y
      && poly->Points[2].X == poly->Points[3].X
      && poly->Points[3].Y == poly->Points[0].Y)
    return 1;
  return 0;
}

/*!
 * \brief Convert buffer contents into an element.
 */
bool
ConvertBufferToElement (BufferType *Buffer)
{
  ElementType *Element;
  Cardinal group;
  Cardinal pin_n = 1;
  bool hasParts = false, crooked = false;
  int onsolder;
  bool warned = false;

  if (Buffer->Data->pcb == 0)
    Buffer->Data->pcb = PCB;

  Element = CreateNewElement (PCB->Data, NoFlags (),
			      NULL, NULL, NULL, PASTEBUFFER->X,
			      PASTEBUFFER->Y, 0, 100,
			      MakeFlags (SWAP_IDENT ? ONSOLDERFLAG : NOFLAG),
			      false);
  if (!Element)
    return (false);
  VIA_LOOP (Buffer->Data);
  {
    char num[8];
    if (via->Mask < via->Thickness)
      via->Mask = via->Thickness + 2 * MASKFRAME;
    if (via->Name)
      CreateNewPin (Element, via->X, via->Y, via->Thickness,
		    via->Clearance, via->Mask, via->DrillingHole,
		    NULL, via->Name, MaskFlags (via->Flags,
						VIAFLAG | NOCOPY_FLAGS |
						SELECTEDFLAG | WARNFLAG));
    else
      {
	sprintf (num, "%d", pin_n++);
	CreateNewPin (Element, via->X, via->Y, via->Thickness,
		      via->Clearance, via->Mask, via->DrillingHole,
		      NULL, num, MaskFlags (via->Flags,
					    VIAFLAG | NOCOPY_FLAGS | SELECTEDFLAG
					    | WARNFLAG));
      }
    hasParts = true;
  }
  END_LOOP;

  for (onsolder = 0; onsolder < 2; onsolder ++)
    {
      int side;
      int onsolderflag;

      if ((!onsolder) == (!SWAP_IDENT))
	{
	  side = TOP_SIDE;
	  onsolderflag = NOFLAG;
	}
      else
	{
	  side = BOTTOM_SIDE;
	  onsolderflag = ONSOLDERFLAG;
	}

#define MAYBE_WARN() \
	  if (onsolder && !hasParts && !warned) \
	    { \
	      warned = true; \
	      Message \
		(_("Warning: All of the pads are on the opposite\n" \
		   "side from the component - that's probably not what\n" \
		   "you wanted\n")); \
	    } \

      /* get the component-side SM pads */
      group = GetLayerGroupNumberBySide (side);
      GROUP_LOOP (Buffer->Data, group);
      {
	char num[8];
	LINE_LOOP (layer);
	{
	  sprintf (num, "%d", pin_n++);
	  CreateNewPad (Element, line->Point1.X,
			line->Point1.Y, line->Point2.X,
			line->Point2.Y, line->Thickness,
			line->Clearance,
			line->Thickness + line->Clearance, NULL,
			line->Number ? line->Number : num,
			MakeFlags (onsolderflag));
	  MAYBE_WARN();
	  hasParts = true;
	}
	END_LOOP;
	POLYGON_LOOP (layer);
	{
	  Coord x1, y1, x2, y2, w, h, t;

	  if (! polygon_is_rectangle (polygon))
	    {
	      crooked = true;
	      continue;
	    }

	  w = polygon->Points[2].X - polygon->Points[0].X;
	  h = polygon->Points[1].Y - polygon->Points[0].Y;
	  t = (w < h) ? w : h;
	  x1 = polygon->Points[0].X + t/2;
	  y1 = polygon->Points[0].Y + t/2;
	  x2 = x1 + (w-t);
	  y2 = y1 + (h-t);

	  sprintf (num, "%d", pin_n++);
	  CreateNewPad (Element,
			x1, y1, x2, y2, t,
			2 * Settings.Keepaway,
			t + Settings.Keepaway,
			NULL, num,
			MakeFlags (SQUAREFLAG | onsolderflag));
	  MAYBE_WARN();
	  hasParts = true;
	}
	END_LOOP;
      }
      END_LOOP;
    }

  /* now add the silkscreen. NOTE: elements must have pads or pins too */
  LINE_LOOP (&Buffer->Data->SILKLAYER);
  {
    if (line->Number && !NAMEONPCB_NAME (Element))
      NAMEONPCB_NAME (Element) = strdup (line->Number);
    CreateNewLineInElement (Element, line->Point1.X,
			    line->Point1.Y, line->Point2.X,
			    line->Point2.Y, line->Thickness);
    hasParts = true;
  }
  END_LOOP;
  ARC_LOOP (&Buffer->Data->SILKLAYER);
  {
    CreateNewArcInElement (Element, arc->X, arc->Y, arc->Width,
			   arc->Height, arc->StartAngle, arc->Delta,
			   arc->Thickness);
    hasParts = true;
  }
  END_LOOP;
  if (!hasParts)
    {
      DestroyObject (PCB->Data, ELEMENT_TYPE, Element, Element, Element);
      Message (_("There was nothing to convert!\n"
		 "Elements must have some silk, pads or pins.\n"));
      return (false);
    }
  if (crooked)
     Message (_("There were polygons that can't be made into pins!\n"
                "So they were not included in the element\n"));
  Element->MarkX = Buffer->X;
  Element->MarkY = Buffer->Y;
  if (SWAP_IDENT)
    SET_FLAG (ONSOLDERFLAG, Element);
  SetElementBoundingBox (PCB->Data, Element);
  ClearBuffer (Buffer);
  MoveObjectToBuffer (Buffer->Data, PCB->Data, ELEMENT_TYPE, Element, Element,
		      Element);
  SetBufferBoundingBox (Buffer);
  return (true);
}

/*!
 * \brief Load PCB into buffer.
 *
 * Parse the file with enabled 'PCB mode' (see parser).
 * If successful, update some other stuff.
 */
bool
LoadLayoutToBuffer (BufferType *Buffer, char *Filename)
{
  PCBType *newPCB = CreateNewPCB ();

  /* new data isn't added to the undo list */
  if (!ParsePCB (newPCB, Filename))
    {
      /* clear data area and replace pointer */
      ClearBuffer (Buffer);
      free (Buffer->Data);
      Buffer->Data = newPCB->Data;
      newPCB->Data = NULL;
      Buffer->X = newPCB->CursorX;
      Buffer->Y = newPCB->CursorY;
      RemovePCB (newPCB);
      Buffer->Data->pcb = PCB;
      return (true);
    }

  /* release unused memory */
  RemovePCB (newPCB);
      Buffer->Data->pcb = PCB;
  return (false);
}

/*!
 * \brief Rotates the contents of the pastebuffer.
 */
void
RotateBuffer (BufferType *Buffer, BYTE Number)
{
  /* rotate vias */
  VIA_LOOP (Buffer->Data);
  {
    r_delete_entry (Buffer->Data->via_tree, (BoxType *)via);
    ROTATE_VIA_LOWLEVEL (via, Buffer->X, Buffer->Y, Number);
    SetPinBoundingBox (via);
    r_insert_entry (Buffer->Data->via_tree, (BoxType *)via, 0);
  }
  END_LOOP;

  /* elements */
  ELEMENT_LOOP (Buffer->Data);
  {
    RotateElementLowLevel (Buffer->Data, element, Buffer->X, Buffer->Y,
			   Number);
  }
  END_LOOP;

  /* all layer related objects */
  ALLLINE_LOOP (Buffer->Data);
  {
    r_delete_entry (layer->line_tree, (BoxType *)line);
    RotateLineLowLevel (line, Buffer->X, Buffer->Y, Number);
    r_insert_entry (layer->line_tree, (BoxType *)line, 0);
  }
  ENDALL_LOOP;
  ALLARC_LOOP (Buffer->Data);
  {
    r_delete_entry (layer->arc_tree, (BoxType *)arc);
    RotateArcLowLevel (arc, Buffer->X, Buffer->Y, Number);
    r_insert_entry (layer->arc_tree, (BoxType *)arc, 0);
  }
  ENDALL_LOOP;
  ALLTEXT_LOOP (Buffer->Data);
  {
    r_delete_entry (layer->text_tree, (BoxType *)text);
    RotateTextLowLevel (text, Buffer->X, Buffer->Y, Number);
    r_insert_entry (layer->text_tree, (BoxType *)text, 0);
  }
  ENDALL_LOOP;
  ALLPOLYGON_LOOP (Buffer->Data);
  {
    r_delete_entry (layer->polygon_tree, (BoxType *)polygon);
    RotatePolygonLowLevel (polygon, Buffer->X, Buffer->Y, Number);
    r_insert_entry (layer->polygon_tree, (BoxType *)polygon, 0);
  }
  ENDALL_LOOP;

  /* finally the origin and the bounding box */
  ROTATE (Buffer->X, Buffer->Y, Buffer->X, Buffer->Y, Number);
  RotateBoxLowLevel (&Buffer->BoundingBox, Buffer->X, Buffer->Y, Number);
  SetCrosshairRangeToBuffer ();
}

static void
free_rotate (Coord *x, Coord *y, Coord cx, Coord cy, double cosa, double sina)
{
  double nx, ny;
  Coord px = *x - cx;
  Coord py = *y - cy;

  nx = px * cosa + py * sina;
  ny = py * cosa - px * sina;

  *x = nx + cx;
  *y = ny + cy;
}

void
FreeRotateElementLowLevel (DataType *Data, ElementType *Element,
			   Coord X, Coord Y,
			   double cosa, double sina, Angle angle)
{
  /* solder side objects need a different orientation */

  /* the text subroutine decides by itself if the direction
   * is to be corrected
   */
#if 0
  ELEMENTTEXT_LOOP (Element);
  {
    if (Data && Data->name_tree[n])
      r_delete_entry (Data->name_tree[n], (BoxType *)text);
    RotateTextLowLevel (text, X, Y, Number);
  }
  END_LOOP;
#endif
  ELEMENTLINE_LOOP (Element);
  {
    free_rotate (&line->Point1.X, &line->Point1.Y, X, Y, cosa, sina);
    free_rotate (&line->Point2.X, &line->Point2.Y, X, Y, cosa, sina);
    SetLineBoundingBox (line);
  }
  END_LOOP;
  PIN_LOOP (Element);
  {
    /* pre-delete the pins from the pin-tree before their coordinates change */
    if (Data)
      r_delete_entry (Data->pin_tree, (BoxType *)pin);
    RestoreToPolygon (Data, PIN_TYPE, Element, pin);
    free_rotate (&pin->X, &pin->Y, X, Y, cosa, sina);
    SetPinBoundingBox (pin);
  }
  END_LOOP;
  PAD_LOOP (Element);
  {
    /* pre-delete the pads before their coordinates change */
    if (Data)
      r_delete_entry (Data->pad_tree, (BoxType *)pad);
    RestoreToPolygon (Data, PAD_TYPE, Element, pad);
    free_rotate (&pad->Point1.X, &pad->Point1.Y, X, Y, cosa, sina);
    free_rotate (&pad->Point2.X, &pad->Point2.Y, X, Y, cosa, sina);
    SetLineBoundingBox ((LineType *) pad);
  }
  END_LOOP;
  ARC_LOOP (Element);
  {
    free_rotate (&arc->X, &arc->Y, X, Y, cosa, sina);
    arc->StartAngle = NormalizeAngle (arc->StartAngle + angle);
  }
  END_LOOP;

  free_rotate (&Element->MarkX, &Element->MarkY, X, Y, cosa, sina);
  SetElementBoundingBox (Data, Element);
  ClearFromPolygon (Data, ELEMENT_TYPE, Element, Element);
}

void
FreeRotateBuffer (BufferType *Buffer, Angle angle)
{
  double cosa, sina;

  cosa = cos(angle * M_PI/180.0);
  sina = sin(angle * M_PI/180.0);

  /* rotate vias */
  VIA_LOOP (Buffer->Data);
  {
    r_delete_entry (Buffer->Data->via_tree, (BoxType *)via);
    free_rotate (&via->X, &via->Y, Buffer->X, Buffer->Y, cosa, sina);
    SetPinBoundingBox (via);
    r_insert_entry (Buffer->Data->via_tree, (BoxType *)via, 0);
  }
  END_LOOP;

  /* elements */
  ELEMENT_LOOP (Buffer->Data);
  {
    FreeRotateElementLowLevel (Buffer->Data, element, Buffer->X, Buffer->Y,
			       cosa, sina, angle);
  }
  END_LOOP;

  /* all layer related objects */
  ALLLINE_LOOP (Buffer->Data);
  {
    r_delete_entry (layer->line_tree, (BoxType *)line);
    free_rotate (&line->Point1.X, &line->Point1.Y, Buffer->X, Buffer->Y, cosa, sina);
    free_rotate (&line->Point2.X, &line->Point2.Y, Buffer->X, Buffer->Y, cosa, sina);
    SetLineBoundingBox (line);
    r_insert_entry (layer->line_tree, (BoxType *)line, 0);
  }
  ENDALL_LOOP;
  ALLARC_LOOP (Buffer->Data);
  {
    r_delete_entry (layer->arc_tree, (BoxType *)arc);
    free_rotate (&arc->X, &arc->Y, Buffer->X, Buffer->Y, cosa, sina);
    arc->StartAngle = NormalizeAngle (arc->StartAngle + angle);
    r_insert_entry (layer->arc_tree, (BoxType *)arc, 0);
  }
  ENDALL_LOOP;
  /* FIXME: rotate text */
  ALLPOLYGON_LOOP (Buffer->Data);
  {
    r_delete_entry (layer->polygon_tree, (BoxType *)polygon);
    POLYGONPOINT_LOOP (polygon);
    {
      free_rotate (&point->X, &point->Y, Buffer->X, Buffer->Y, cosa, sina);
    }
    END_LOOP;
    SetPolygonBoundingBox (polygon);
    r_insert_entry (layer->polygon_tree, (BoxType *)polygon, 0);
  }
  ENDALL_LOOP;

  SetBufferBoundingBox (Buffer);
  SetCrosshairRangeToBuffer ();
}


/* -------------------------------------------------------------------------- */

static const char freerotatebuffer_syntax[] =
  N_("FreeRotateBuffer([Angle])");

static const char freerotatebuffer_help[] =
  N_("Rotates the current paste buffer contents by the specified angle.  The\n"
  "angle is given in degrees.  If no angle is given, the user is prompted\n"
  "for one.\n");

/* %start-doc actions FreeRotateBuffer
   
Rotates the contents of the pastebuffer by an arbitrary angle.  If no
angle is given, the user is prompted for one.

%end-doc */

int
ActionFreeRotateBuffer(int argc, char **argv, Coord x, Coord y)
{
  char *angle_s;

  if (argc < 1)
    angle_s = gui->prompt_for (_("Enter Rotation (degrees, CCW):"), "0");
  else
    angle_s = argv[0];

  notify_crosshair_change (false);
  FreeRotateBuffer(PASTEBUFFER, strtod(angle_s, 0));
  notify_crosshair_change (true);
  return 0;
}

/*!
 * \brief Initializes the buffers by allocating memory.
 */
void
InitBuffers (void)
{
  int i;

  for (i = 0; i < MAX_BUFFER; i++)
    Buffers[i].Data = CreateNewBuffer ();
}

void
UninitBuffers (void)
{
  int i;

  for (i = 0; i < MAX_BUFFER; i++)
    {
      ClearBuffer (Buffers+i);
      free (Buffers[i].Data);
    }
}

void
SwapBuffers (void)
{
  int i;

  for (i = 0; i < MAX_BUFFER; i++)
    SwapBuffer (&Buffers[i]);
  SetCrosshairRangeToBuffer ();
}

void
MirrorBuffer (BufferType *Buffer)
{
  int i;

  if (Buffer->Data->ElementN)
    {
      Message (_("You can't mirror a buffer that has elements!\n"));
      return;
    }
  for (i = 0; i < max_copper_layer + SILK_LAYER; i++)
    {
      LayerType *layer = Buffer->Data->Layer + i;
      if (layer->TextN)
	{
	  Message (_("You can't mirror a buffer that has text!\n"));
	  return;
	}
    }
  /* set buffer offset to 'mark' position */
  Buffer->X = SWAP_X (Buffer->X);
  Buffer->Y = SWAP_Y (Buffer->Y);
  VIA_LOOP (Buffer->Data);
  {
    via->X = SWAP_X (via->X);
    via->Y = SWAP_Y (via->Y);
  }
  END_LOOP;
  ALLLINE_LOOP (Buffer->Data);
  {
    line->Point1.X = SWAP_X (line->Point1.X);
    line->Point1.Y = SWAP_Y (line->Point1.Y);
    line->Point2.X = SWAP_X (line->Point2.X);
    line->Point2.Y = SWAP_Y (line->Point2.Y);
  }
  ENDALL_LOOP;
  ALLARC_LOOP (Buffer->Data);
  {
    arc->X = SWAP_X (arc->X);
    arc->Y = SWAP_Y (arc->Y);
    arc->StartAngle = SWAP_ANGLE (arc->StartAngle);
    arc->Delta = SWAP_DELTA (arc->Delta);
    SetArcBoundingBox (arc);
  }
  ENDALL_LOOP;
  ALLPOLYGON_LOOP (Buffer->Data);
  {
    POLYGONPOINT_LOOP (polygon);
    {
      point->X = SWAP_X (point->X);
      point->Y = SWAP_Y (point->Y);
    }
    END_LOOP;
    SetPolygonBoundingBox (polygon);
  }
  ENDALL_LOOP;
  SetBufferBoundingBox (Buffer);
  SetCrosshairRangeToBuffer ();
}


/*!
 * \brief Flip components/tracks from one side to the other.
 */
static void
SwapBuffer (BufferType *Buffer)
{
  int j, k;
  Cardinal top_group, bottom_group;
  LayerType swap;

  ELEMENT_LOOP (Buffer->Data);
  {
    r_delete_element (Buffer->Data, element);
    MirrorElementCoordinates (Buffer->Data, element, 0);
  }
  END_LOOP;
  /* set buffer offset to 'mark' position */
  Buffer->X = SWAP_X (Buffer->X);
  Buffer->Y = SWAP_Y (Buffer->Y);
  VIA_LOOP (Buffer->Data);
  {
    r_delete_entry (Buffer->Data->via_tree, (BoxType *)via);
    via->X = SWAP_X (via->X);
    via->Y = SWAP_Y (via->Y);
    SetPinBoundingBox (via);
    r_insert_entry (Buffer->Data->via_tree, (BoxType *)via, 0);
  }
  END_LOOP;
  ALLLINE_LOOP (Buffer->Data);
  {
    r_delete_entry (layer->line_tree, (BoxType *)line);
    line->Point1.X = SWAP_X (line->Point1.X);
    line->Point1.Y = SWAP_Y (line->Point1.Y);
    line->Point2.X = SWAP_X (line->Point2.X);
    line->Point2.Y = SWAP_Y (line->Point2.Y);
    SetLineBoundingBox (line);
    r_insert_entry (layer->line_tree, (BoxType *)line, 0);
  }
  ENDALL_LOOP;
  ALLARC_LOOP (Buffer->Data);
  {
    r_delete_entry (layer->arc_tree, (BoxType *)arc);
    arc->X = SWAP_X (arc->X);
    arc->Y = SWAP_Y (arc->Y);
    arc->StartAngle = SWAP_ANGLE (arc->StartAngle);
    arc->Delta = SWAP_DELTA (arc->Delta);
    SetArcBoundingBox (arc);
    r_insert_entry (layer->arc_tree, (BoxType *)arc, 0);
  }
  ENDALL_LOOP;
  ALLPOLYGON_LOOP (Buffer->Data);
  {
    r_delete_entry (layer->polygon_tree, (BoxType *)polygon);
    POLYGONPOINT_LOOP (polygon);
    {
      point->X = SWAP_X (point->X);
      point->Y = SWAP_Y (point->Y);
    }
    END_LOOP;
    SetPolygonBoundingBox (polygon);
    r_insert_entry (layer->polygon_tree, (BoxType *)polygon, 0);
    /* hmmm, how to handle clip */
  }
  ENDALL_LOOP;
  ALLTEXT_LOOP (Buffer->Data);
  {
    r_delete_entry (layer->text_tree, (BoxType *)text);
    text->X = SWAP_X (text->X);
    text->Y = SWAP_Y (text->Y);
    TOGGLE_FLAG (ONSOLDERFLAG, text);
    SetTextBoundingBox(text);
    r_insert_entry (layer->text_tree, (BoxType *)text, 0);
  }
  ENDALL_LOOP;
  /* swap silkscreen layers */
  swap = Buffer->Data->Layer[bottom_silk_layer];
  Buffer->Data->Layer[bottom_silk_layer] =
    Buffer->Data->Layer[top_silk_layer];
  Buffer->Data->Layer[top_silk_layer] = swap;

  /* swap layer groups when balanced */
  top_group = GetLayerGroupNumberBySide (TOP_SIDE);
  bottom_group = GetLayerGroupNumberBySide (BOTTOM_SIDE);
  if (PCB->LayerGroups.Number[top_group] == PCB->LayerGroups.Number[bottom_group])
    {
      for (j = k = 0; j < PCB->LayerGroups.Number[bottom_group]; j++)
	{
	  int t1, t2;
	  Cardinal top_number = PCB->LayerGroups.Entries[top_group][k];
	  Cardinal bottom_number = PCB->LayerGroups.Entries[bottom_group][j];

	  if (bottom_number >= max_copper_layer)
	    continue;
	  swap = Buffer->Data->Layer[bottom_number];

	  while (top_number >= max_copper_layer)
	    {
	      k++;
	      top_number = PCB->LayerGroups.Entries[top_group][k];
	    }
	  Buffer->Data->Layer[bottom_number] = Buffer->Data->Layer[top_number];
	  Buffer->Data->Layer[top_number] = swap;
	  k++;
	  /* move the thermal flags with the layers */
	  ALLPIN_LOOP (Buffer->Data);
	  {
	    t1 = TEST_THERM (bottom_number, pin);
	    t2 = TEST_THERM (top_number, pin);
	    ASSIGN_THERM (bottom_number, t2, pin);
	    ASSIGN_THERM (top_number, t1, pin);
	  }
	  ENDALL_LOOP;
	  VIA_LOOP (Buffer->Data);
	  {
	    t1 = TEST_THERM (bottom_number, via);
	    t2 = TEST_THERM (top_number, via);
	    ASSIGN_THERM (bottom_number, t2, via);
	    ASSIGN_THERM (top_number, t1, via);
	  }
	  END_LOOP;
	}
    }
  SetBufferBoundingBox (Buffer);
  SetCrosshairRangeToBuffer ();
}

/*!
 * \brief Moves the passed object to the passed buffer and removes it
 * from its original place.
 */
void *
MoveObjectToBuffer (DataType *Destination, DataType *Src,
		    int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  /* setup local identifiers used by move operations */
  Dest = Destination;
  Source = Src;
  return (ObjectOperation (&MoveBufferFunctions, Type, Ptr1, Ptr2, Ptr3));
}

/*!
 * \brief Adds the passed object to the passed buffer.
 */
void *
CopyObjectToBuffer (DataType *Destination, DataType *Src,
		    int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  /* setup local identifiers used by Add operations */
  Dest = Destination;
  Source = Src;
  return (ObjectOperation (&AddBufferFunctions, Type, Ptr1, Ptr2, Ptr3));
}

/* ---------------------------------------------------------------------- */

HID_Action rotate_action_list[] = {
  {"FreeRotateBuffer", 0, ActionFreeRotateBuffer,
   freerotatebuffer_syntax, freerotatebuffer_help},
  {"LoadFootprint", 0, LoadFootprint,
   0,0}
};

REGISTER_ACTIONS (rotate_action_list)
