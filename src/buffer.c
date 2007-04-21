/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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

/* functions used by paste- and move/copy buffer
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <memory.h>

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

RCSID ("$Id$");

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void *AddViaToBuffer (PinTypePtr);
static void *AddLineToBuffer (LayerTypePtr, LineTypePtr);
static void *AddArcToBuffer (LayerTypePtr, ArcTypePtr);
static void *AddRatToBuffer (RatTypePtr);
static void *AddTextToBuffer (LayerTypePtr, TextTypePtr);
static void *AddPolygonToBuffer (LayerTypePtr, PolygonTypePtr);
static void *AddElementToBuffer (ElementTypePtr);
static void *MoveViaToBuffer (PinTypePtr);
static void *MoveLineToBuffer (LayerTypePtr, LineTypePtr);
static void *MoveArcToBuffer (LayerTypePtr, ArcTypePtr);
static void *MoveRatToBuffer (RatTypePtr);
static void *MoveTextToBuffer (LayerTypePtr, TextTypePtr);
static void *MovePolygonToBuffer (LayerTypePtr, PolygonTypePtr);
static void *MoveElementToBuffer (ElementTypePtr);
static void SwapBuffer (BufferTypePtr);

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static DataTypePtr Dest, Source;

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

/* ---------------------------------------------------------------------------
 * copies a via to paste buffer
 */
static void *
AddViaToBuffer (PinTypePtr Via)
{
  return (CreateNewVia (Dest, Via->X, Via->Y, Via->Thickness, Via->Clearance,
			Via->Mask, Via->DrillingHole, Via->Name,
			MaskFlags (Via->Flags, FOUNDFLAG | ExtraFlag)));
}

/* ---------------------------------------------------------------------------
 * copies a rat-line to paste buffer
 */
static void *
AddRatToBuffer (RatTypePtr Rat)
{
  return (CreateNewRat (Dest, Rat->Point1.X, Rat->Point1.Y,
			Rat->Point2.X, Rat->Point2.Y, Rat->group1,
			Rat->group2, Rat->Thickness,
			MaskFlags (Rat->Flags, FOUNDFLAG | ExtraFlag)));
}

/* ---------------------------------------------------------------------------
 * copies a line to buffer  
 */
static void *
AddLineToBuffer (LayerTypePtr Layer, LineTypePtr Line)
{
  LineTypePtr line;
  LayerTypePtr layer = &Dest->Layer[GetLayerNumber (Source, Layer)];

  line = CreateNewLineOnLayer (layer, Line->Point1.X, Line->Point1.Y,
			       Line->Point2.X, Line->Point2.Y,
			       Line->Thickness, Line->Clearance,
			       MaskFlags (Line->Flags,
					  FOUNDFLAG | ExtraFlag));
  if (line && Line->Number)
    line->Number = MyStrdup (Line->Number, "AddLineToBuffer");
  return (line);
}

/* ---------------------------------------------------------------------------
 * copies an arc to buffer  
 */
static void *
AddArcToBuffer (LayerTypePtr Layer, ArcTypePtr Arc)
{
  LayerTypePtr layer = &Dest->Layer[GetLayerNumber (Source, Layer)];

  return (CreateNewArcOnLayer (layer, Arc->X, Arc->Y,
			       Arc->Width, Arc->StartAngle, Arc->Delta,
			       Arc->Thickness, Arc->Clearance,
			       MaskFlags (Arc->Flags,
					  FOUNDFLAG | ExtraFlag)));
}

/* ---------------------------------------------------------------------------
 * copies a text to buffer
 */
static void *
AddTextToBuffer (LayerTypePtr Layer, TextTypePtr Text)
{
  LayerTypePtr layer = &Dest->Layer[GetLayerNumber (Source, Layer)];

  return (CreateNewText (layer, &PCB->Font, Text->X, Text->Y,
			 Text->Direction, Text->Scale, Text->TextString,
			 MaskFlags (Text->Flags, ExtraFlag)));
}

/* ---------------------------------------------------------------------------
 * copies a polygon to buffer
 */
static void *
AddPolygonToBuffer (LayerTypePtr Layer, PolygonTypePtr Polygon)
{
  LayerTypePtr layer = &Dest->Layer[GetLayerNumber (Source, Layer)];
  PolygonTypePtr polygon;

  polygon = GetPolygonMemory (layer);
  CopyPolygonLowLevel (polygon, Polygon);
  CLEAR_FLAG (FOUNDFLAG | ExtraFlag, polygon);
  return (polygon);
}

/* ---------------------------------------------------------------------------
 * copies a element to buffer
 */
static void *
AddElementToBuffer (ElementTypePtr Element)
{
  ElementTypePtr element;

  element = GetElementMemory (Dest);
  CopyElementLowLevel (Dest, element, Element, False, 0, 0);
  CLEAR_FLAG (ExtraFlag, element);
  if (ExtraFlag)
    {
      ELEMENTTEXT_LOOP (element);
      {
	CLEAR_FLAG (ExtraFlag, text);
      }
      END_LOOP;
      PIN_LOOP (element);
      {
	CLEAR_FLAG (ExtraFlag, pin);
      }
      END_LOOP;
      PAD_LOOP (element);
      {
	CLEAR_FLAG (ExtraFlag, pad);
      }
      END_LOOP;
    }
  return (element);
}

/* ---------------------------------------------------------------------------
 * moves a via to paste buffer without allocating memory for the name
 */
static void *
MoveViaToBuffer (PinTypePtr Via)
{
  PinTypePtr via;

  RestoreToPolygon (Source, VIA_TYPE, Via, Via);
  r_delete_entry (Source->via_tree, (BoxType *) Via);
  via = GetViaMemory (Dest);
  *via = *Via;
  *Via = Source->Via[--Source->ViaN];
  r_substitute (Source->via_tree, (BoxType *) & Source->Via[Source->ViaN],
		(BoxType *) Via);
  CLEAR_FLAG (WARNFLAG | FOUNDFLAG, via);
  memset (&Source->Via[Source->ViaN], 0, sizeof (PinType));
  if (!Dest->via_tree)
    Dest->via_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (Dest->via_tree, (BoxType *) via, 0);
  ClearFromPolygon (Dest, VIA_TYPE, via, via);
  return (via);
}

/* ---------------------------------------------------------------------------
 * moves a rat-line to paste buffer
 */
static void *
MoveRatToBuffer (RatTypePtr Rat)
{
  RatTypePtr rat;

  rat = GetRatMemory (Dest);
  *rat = *Rat;
  r_delete_entry (Source->rat_tree, &Rat->BoundingBox);
  *Rat = Source->Rat[--Source->RatN];
  r_substitute (Source->rat_tree, &Source->Rat[Source->RatN].BoundingBox,
		&Rat->BoundingBox);
  CLEAR_FLAG (FOUNDFLAG, Rat);
  memset (&Source->Rat[Source->RatN], 0, sizeof (RatType));
  if (!Dest->rat_tree)
    Dest->rat_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (Dest->rat_tree, &rat->BoundingBox, 0);
  return (rat);
}

/* ---------------------------------------------------------------------------
 * moves a line to buffer  
 */
static void *
MoveLineToBuffer (LayerTypePtr Layer, LineTypePtr Line)
{
  LayerTypePtr lay;
  LineTypePtr line;

  RestoreToPolygon (Source, LINE_TYPE, Layer, Line);
  r_delete_entry (Layer->line_tree, (BoxTypePtr) Line);
  lay = &Dest->Layer[GetLayerNumber (Source, Layer)];
  line = GetLineMemory (lay);
  *line = *Line;
  CLEAR_FLAG (FOUNDFLAG, line);
  /* line pointers being shuffled */
  *Line = Layer->Line[--Layer->LineN];
  r_substitute (Layer->line_tree, (BoxTypePtr) & Layer->Line[Layer->LineN],
		(BoxTypePtr) Line);
  memset (&Layer->Line[Layer->LineN], 0, sizeof (LineType));
  if (!lay->line_tree)
    lay->line_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (lay->line_tree, (BoxTypePtr) line, 0);
  ClearFromPolygon (Dest, LINE_TYPE, lay, line);
  return (line);
}

/* ---------------------------------------------------------------------------
 * moves an arc to buffer  
 */
static void *
MoveArcToBuffer (LayerTypePtr Layer, ArcTypePtr Arc)
{
  LayerTypePtr lay;
  ArcTypePtr arc;

  RestoreToPolygon (Source, ARC_TYPE, Layer, Arc);
  r_delete_entry (Layer->arc_tree, (BoxTypePtr) Arc);
  lay = &Dest->Layer[GetLayerNumber (Source, Layer)];
  arc = GetArcMemory (lay);
  *arc = *Arc;
  CLEAR_FLAG (FOUNDFLAG, arc);
  /* arc pointers being shuffled */
  *Arc = Layer->Arc[--Layer->ArcN];
  r_substitute (Layer->arc_tree, (BoxTypePtr) & Layer->Arc[Layer->ArcN],
		(BoxTypePtr) Arc);
  memset (&Layer->Arc[Layer->ArcN], 0, sizeof (ArcType));
  if (!lay->arc_tree)
    lay->arc_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (lay->arc_tree, (BoxTypePtr) arc, 0);
  RestoreToPolygon (Dest, ARC_TYPE, lay, arc);
  return (arc);
}

/* ---------------------------------------------------------------------------
 * moves a text to buffer without allocating memory for the name
 */
static void *
MoveTextToBuffer (LayerTypePtr Layer, TextTypePtr Text)
{
  TextTypePtr text;
  LayerTypePtr lay;

  r_delete_entry (Layer->text_tree, (BoxTypePtr) Text);
  lay = &Dest->Layer[GetLayerNumber (Source, Layer)];
  text = GetTextMemory (lay);
  *text = *Text;
  *Text = Layer->Text[--Layer->TextN];
  r_substitute (Layer->text_tree, (BoxTypePtr) & Layer->Text[Layer->TextN],
		(BoxTypePtr) Text);
  memset (&Layer->Text[Layer->TextN], 0, sizeof (TextType));
  if (!lay->text_tree)
    lay->text_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (lay->text_tree, (BoxTypePtr) text, 0);
  return (text);
}

/* ---------------------------------------------------------------------------
 * moves a polygon to buffer. Doesn't allocate memory for the points
 */
static void *
MovePolygonToBuffer (LayerTypePtr Layer, PolygonTypePtr Polygon)
{
  LayerTypePtr lay;
  PolygonTypePtr polygon;

  r_delete_entry (Layer->polygon_tree, (BoxTypePtr) Polygon);
  lay = &Dest->Layer[GetLayerNumber (Source, Layer)];
  polygon = GetPolygonMemory (lay);
  *polygon = *Polygon;
  CLEAR_FLAG (FOUNDFLAG, polygon);
  *Polygon = Layer->Polygon[--Layer->PolygonN];
  r_substitute (Layer->polygon_tree,
		(BoxTypePtr) & Layer->Polygon[Layer->PolygonN],
		(BoxTypePtr) Polygon);
  memset (&Layer->Polygon[Layer->PolygonN], 0, sizeof (PolygonType));
  if (!lay->polygon_tree)
    lay->polygon_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (lay->polygon_tree, (BoxTypePtr) polygon, 0);
  return (polygon);
}

/* ---------------------------------------------------------------------------
 * moves a element to buffer without allocating memory for pins/names
 */
static void *
MoveElementToBuffer (ElementTypePtr Element)
{
  ElementTypePtr element;
  int i;

  r_delete_element (Source, Element);
  element = GetElementMemory (Dest);
  *element = *Element;
  PIN_LOOP (element);
  {
    RestoreToPolygon(Source, PIN_TYPE, Element, pin);
    CLEAR_FLAG (WARNFLAG | FOUNDFLAG, pin);
    pin->Element = element;
  }
  END_LOOP;
  PAD_LOOP (element);
  {
    RestoreToPolygon(Source, PAD_TYPE, Element, pad);
    CLEAR_FLAG (WARNFLAG | FOUNDFLAG, pad);
    pad->Element = element;
  }
  END_LOOP;
  ELEMENTTEXT_LOOP (element);
  {
    text->Element = element;
  }
  END_LOOP;
  SetElementBoundingBox (Dest, element, &PCB->Font);
  *Element = Source->Element[--Source->ElementN];
  /* deal with element pointer changing */
  r_substitute (Source->element_tree,
		(BoxType *) & Source->Element[Source->ElementN],
		(BoxType *) Element);
  for (i = 0; i < MAX_ELEMENTNAMES; i++)
    r_substitute (Source->name_tree[i],
		  (BoxType *) & Source->Element[Source->ElementN].Name[i],
		  (BoxType *) & Element->Name[i]);

  PIN_LOOP (Element);
  {
    pin->Element = Element;
    ClearFromPolygon (Dest, PIN_TYPE, Element, pin);
  }
  END_LOOP;
  PAD_LOOP (Element);
  {
    pad->Element = Element;
    ClearFromPolygon (Dest, PAD_TYPE, Element, pad);
  }
  END_LOOP;
  ELEMENTTEXT_LOOP (Element);
  {
    text->Element = Element;
  }
  END_LOOP;
  memset (&Source->Element[Source->ElementN], 0, sizeof (ElementType));
  return (element);
}

/* ---------------------------------------------------------------------------
 * calculates the bounding box of the buffer
 */
void
SetBufferBoundingBox (BufferTypePtr Buffer)
{
  BoxTypePtr box = GetDataBoundingBox (Buffer->Data);

  if (box)
    Buffer->BoundingBox = *box;
}

/* ---------------------------------------------------------------------------
 * clears the contents of the paste buffer
 */
void
ClearBuffer (BufferTypePtr Buffer)
{
  if (Buffer && Buffer->Data)
    {
      FreeDataMemory (Buffer->Data);
      Buffer->Data->pcb = PCB;
    }
}

/* ----------------------------------------------------------------------
 * copies all selected and visible objects to the paste buffer
 * returns True if any objects have been removed
 */
void
AddSelectedToBuffer (BufferTypePtr Buffer, LocationType X, LocationType Y,
		     Boolean LeaveSelected)
{
  /* switch crosshair off because adding objects to the pastebuffer
   * may change the 'valid' area for the cursor
   */
  if (!LeaveSelected)
    ExtraFlag = SELECTEDFLAG;
  HideCrosshair (True);
  Source = PCB->Data;
  Dest = Buffer->Data;
  SelectedOperation (&AddBufferFunctions, False, ALL_TYPES);

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
  RestoreCrosshair (True);
  ExtraFlag = 0;
}

/* ---------------------------------------------------------------------------
 * loads element data from file/library into buffer
 * parse the file with disabled 'PCB mode' (see parser)
 * returns False on error
 * if successful, update some other stuff and reposition the pastebuffer
 */
Boolean
LoadElementToBuffer (BufferTypePtr Buffer, char *Name, Boolean FromFile)
{
  ElementTypePtr element;

  ClearBuffer (Buffer);
  if (FromFile)
    {
      if (!ParseElementFile (Buffer->Data, Name))
	{
	  if (Settings.ShowSolderSide)
	    SwapBuffer (Buffer);
	  SetBufferBoundingBox (Buffer);
	  if (Buffer->Data->ElementN)
	    {
	      element = &(Buffer->Data->Element[0]);
	      Buffer->X = element->MarkX;
	      Buffer->Y = element->MarkY;
	    }
	  else
	    {
	      Buffer->X = 0;
	      Buffer->Y = 0;
	    }
	  return (True);
	}
    }
  else
    {
      if (!ParseLibraryEntry (Buffer->Data, Name)
	  && Buffer->Data->ElementN != 0)
	{
	  element = &(Buffer->Data->Element[0]);

	  /* always add elements using top-side coordinates */
	  if (Settings.ShowSolderSide)
	    MirrorElementCoordinates (Buffer->Data, element, 0);
	  SetElementBoundingBox (Buffer->Data, element, &PCB->Font);

	  /* set buffer offset to 'mark' position */
	  Buffer->X = element->MarkX;
	  Buffer->Y = element->MarkY;
	  SetBufferBoundingBox (Buffer);
	  return (True);
	}
    }
  /* release memory which might have been acquired */
  ClearBuffer (Buffer);
  return (False);
}


/*---------------------------------------------------------------------------
 *
 * break buffer element into pieces
 */
Boolean
SmashBufferElement (BufferTypePtr Buffer)
{
  ElementTypePtr element;
  Cardinal group;
  LayerTypePtr layer;

  if (Buffer->Data->ElementN != 1)
    {
      Message (_("Error!  Buffer doesn't contain a single element\n"));
      return (False);
    }
  element = &Buffer->Data->Element[0];
  Buffer->Data->ElementN = 0;
  ClearBuffer (Buffer);
  ELEMENTLINE_LOOP (element);
  {
    CreateNewLineOnLayer (&Buffer->Data->SILKLAYER,
			  line->Point1.X, line->Point1.Y,
			  line->Point2.X, line->Point2.Y,
			  line->Thickness, 0, NoFlags ());
    if (line)
      line->Number = MyStrdup (NAMEONPCB_NAME (element), "SmashBuffer");
  }
  END_LOOP;
  ARC_LOOP (element);
  {
    CreateNewArcOnLayer (&Buffer->Data->SILKLAYER,
			 arc->X, arc->Y, arc->Width, arc->StartAngle,
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
  group =
    GetLayerGroupNumberByNumber (max_layer +
				 (SWAP_IDENT ? SOLDER_LAYER :
				  COMPONENT_LAYER));
  layer = &Buffer->Data->Layer[PCB->LayerGroups.Entries[group][0]];
  PAD_LOOP (element);
  {
    LineTypePtr line;
    line = CreateNewLineOnLayer (layer, pad->Point1.X, pad->Point1.Y,
				 pad->Point2.X, pad->Point2.Y,
				 pad->Thickness, pad->Clearance, NoFlags ());
    if (line)
      line->Number = MyStrdup (pad->Number, "SmashBuffer");
  }
  END_LOOP;
  FreeElementMemory (element);
  SaveFree (element);
  return (True);
}

/*---------------------------------------------------------------------------
 *
 * see if a polygon is a rectangle.  If so, canonicalize it.
 */

static int
polygon_is_rectangle (PolygonTypePtr poly)
{
  int i, best;
  PointType temp[4];
  if (poly->PointN != 4)
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

/*---------------------------------------------------------------------------
 *
 * convert buffer contents into an element
 */
Boolean
ConvertBufferToElement (BufferTypePtr Buffer)
{
  ElementTypePtr Element;
  Cardinal group;
  Cardinal pin_n = 1;
  Boolean hasParts = False, crooked = False;

  if (Buffer->Data->pcb == 0)
    Buffer->Data->pcb = PCB;

  Element = CreateNewElement (PCB->Data, NULL, &PCB->Font, NoFlags (),
			      NULL, NULL, NULL, PASTEBUFFER->X,
			      PASTEBUFFER->Y, 0, 100,
			      MakeFlags (SWAP_IDENT ? ONSOLDERFLAG : NOFLAG),
			      False);
  if (!Element)
    return (False);
  VIA_LOOP (Buffer->Data);
  {
    char num[8];
    if (via->Mask < via->Thickness)
      via->Mask = via->Thickness + 2 * MASKFRAME * 100;	/* MASKFRAME is in mils */
    if (via->Name)
      CreateNewPin (Element, via->X, via->Y, via->Thickness,
		    via->Clearance, via->Mask, via->DrillingHole,
		    NULL, via->Name, MaskFlags (via->Flags,
						VIAFLAG | FOUNDFLAG |
						SELECTEDFLAG | WARNFLAG));
    else
      {
	sprintf (num, "%d", pin_n++);
	CreateNewPin (Element, via->X, via->Y, via->Thickness,
		      via->Clearance, via->Mask, via->DrillingHole,
		      NULL, num, MaskFlags (via->Flags,
					    VIAFLAG | FOUNDFLAG | SELECTEDFLAG
					    | WARNFLAG));
      }
    hasParts = True;
  }
  END_LOOP;
  /* get the component-side SM pads */
  group = GetLayerGroupNumberByNumber (max_layer +
				       (SWAP_IDENT ? SOLDER_LAYER :
					COMPONENT_LAYER));
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
		    MakeFlags (SWAP_IDENT ? ONSOLDERFLAG : NOFLAG));
      hasParts = True;
    }
    END_LOOP;
    POLYGON_LOOP (layer);
    {
      int x1, y1, x2, y2, w, h, t;

      if (! polygon_is_rectangle (polygon))
        {
          crooked = True;
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
		    MakeFlags (SQUAREFLAG | (SWAP_IDENT ? ONSOLDERFLAG : NOFLAG)));
      hasParts = True;
    }
    END_LOOP;
  }
  END_LOOP;
  /* now get the opposite side pads */
  group = GetLayerGroupNumberByNumber (max_layer +
				       (SWAP_IDENT ? COMPONENT_LAYER :
					SOLDER_LAYER));
  GROUP_LOOP (Buffer->Data, group);
  {
    Boolean warned = False;
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
		    MakeFlags (SWAP_IDENT ? NOFLAG : ONSOLDERFLAG));
      if (!hasParts && !warned)
	{
	  warned = True;
	  Message
	    (_("Warning: All of the pads are on the opposite\n"
	       "side from the component - that's probably not what\n"
	       "you wanted\n"));
	}
      hasParts = True;
    }
    END_LOOP;
  }
  END_LOOP;
  /* now add the silkscreen. NOTE: elements must have pads or pins too */
  LINE_LOOP (&Buffer->Data->SILKLAYER);
  {
    if (line->Number && !NAMEONPCB_NAME (Element))
      NAMEONPCB_NAME (Element) = MyStrdup (line->Number,
					   "ConvertBufferToElement");
    CreateNewLineInElement (Element, line->Point1.X,
			    line->Point1.Y, line->Point2.X,
			    line->Point2.Y, line->Thickness);
  }
  END_LOOP;
  ARC_LOOP (&Buffer->Data->SILKLAYER);
  {
    CreateNewArcInElement (Element, arc->X, arc->Y, arc->Width,
			   arc->Height, arc->StartAngle, arc->Delta,
			   arc->Thickness);
  }
  END_LOOP;
  if (!hasParts)
    {
      DestroyObject (PCB->Data, ELEMENT_TYPE, Element, Element, Element);
      Message (_("There was nothing to convert!\n"
		 "Elements must have some pads or pins.\n"));
      return (False);
    }
  if (crooked)
     Message (_("There were polygons that can't be made into pins!\n"
                "So they were not included in the element\n"));
  Element->MarkX = Buffer->X;
  Element->MarkY = Buffer->Y;
  if (SWAP_IDENT)
    SET_FLAG (ONSOLDERFLAG, Element);
  SetElementBoundingBox (PCB->Data, Element, &PCB->Font);
  ClearBuffer (Buffer);
  MoveObjectToBuffer (Buffer->Data, PCB->Data, ELEMENT_TYPE, Element, Element,
		      Element);
  SetBufferBoundingBox (Buffer);
  return (True);
}

/* ---------------------------------------------------------------------------
 * load PCB into buffer
 * parse the file with enabled 'PCB mode' (see parser)
 * if successful, update some other stuff
 */
Boolean
LoadLayoutToBuffer (BufferTypePtr Buffer, char *Filename)
{
  PCBTypePtr newPCB = CreateNewPCB (False);

  /* new data isn't added to the undo list */
  if (!ParsePCB (newPCB, Filename))
    {
      /* clear data area and replace pointer */
      ClearBuffer (Buffer);
      SaveFree (Buffer->Data);
      Buffer->Data = newPCB->Data;
      newPCB->Data = NULL;
      Buffer->X = newPCB->CursorX;
      Buffer->Y = newPCB->CursorY;
      RemovePCB (newPCB);
      return (True);
    }

  /* release unused memory */
  RemovePCB (newPCB);
  return (False);
}

/* ---------------------------------------------------------------------------
 * rotates the contents of the pastebuffer
 */
void
RotateBuffer (BufferTypePtr Buffer, BYTE Number)
{
  /* rotate vias */
  VIA_LOOP (Buffer->Data);
  {
    r_delete_entry (Buffer->Data->via_tree, (BoxTypePtr) via);
    ROTATE_VIA_LOWLEVEL (via, Buffer->X, Buffer->Y, Number);
    SetPinBoundingBox (via);
    r_insert_entry (Buffer->Data->via_tree, (BoxTypePtr) via, 0);
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
    r_delete_entry (layer->line_tree, (BoxTypePtr) line);
    RotateLineLowLevel (line, Buffer->X, Buffer->Y, Number);
    r_insert_entry (layer->line_tree, (BoxTypePtr) line, 0);
  }
  ENDALL_LOOP;
  ALLARC_LOOP (Buffer->Data);
  {
    r_delete_entry (layer->arc_tree, (BoxTypePtr) arc);
    RotateArcLowLevel (arc, Buffer->X, Buffer->Y, Number);
    r_insert_entry (layer->arc_tree, (BoxTypePtr) arc, 0);
  }
  ENDALL_LOOP;
  ALLTEXT_LOOP (Buffer->Data);
  {
    r_delete_entry (layer->text_tree, (BoxTypePtr) text);
    RotateTextLowLevel (text, Buffer->X, Buffer->Y, Number);
    r_insert_entry (layer->text_tree, (BoxTypePtr) text, 0);
  }
  ENDALL_LOOP;
  ALLPOLYGON_LOOP (Buffer->Data);
  {
    r_delete_entry (layer->polygon_tree, (BoxTypePtr) polygon);
    RotatePolygonLowLevel (polygon, Buffer->X, Buffer->Y, Number);
    r_insert_entry (layer->polygon_tree, (BoxTypePtr) polygon, 0);
  }
  ENDALL_LOOP;

  /* finally the origin and the bounding box */
  ROTATE (Buffer->X, Buffer->Y, Buffer->X, Buffer->Y, Number);
  RotateBoxLowLevel (&Buffer->BoundingBox, Buffer->X, Buffer->Y, Number);
}

/* ---------------------------------------------------------------------------
 * initializes the buffers by allocating memory
 */
void
InitBuffers (void)
{
  int i;

  for (i = 0; i < MAX_BUFFER; i++)
    Buffers[i].Data = CreateNewBuffer ();
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
MirrorBuffer (BufferTypePtr Buffer)
{
  int i;

  if (Buffer->Data->ElementN)
    {
      Message (_("You can't mirror a buffer that has elements!\n"));
      return;
    }
  for (i = 0; i < max_layer + 2; i++)
    {
      LayerTypePtr layer = Buffer->Data->Layer + i;
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
}


/* ---------------------------------------------------------------------------
 * flip components/tracks from one side to the other
 */
static void
SwapBuffer (BufferTypePtr Buffer)
{
  int j, k;
  Cardinal sgroup, cgroup;
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
    r_delete_entry (Buffer->Data->via_tree, (BoxTypePtr) via);
    via->X = SWAP_X (via->X);
    via->Y = SWAP_Y (via->Y);
    SetPinBoundingBox (via);
    r_insert_entry (Buffer->Data->via_tree, (BoxTypePtr) via, 0);
  }
  END_LOOP;
  ALLLINE_LOOP (Buffer->Data);
  {
    r_delete_entry (layer->line_tree, (BoxTypePtr) line);
    line->Point1.X = SWAP_X (line->Point1.X);
    line->Point1.Y = SWAP_Y (line->Point1.Y);
    line->Point2.X = SWAP_X (line->Point2.X);
    line->Point2.Y = SWAP_Y (line->Point2.Y);
    SetLineBoundingBox (line);
    r_insert_entry (layer->line_tree, (BoxTypePtr) line, 0);
  }
  ENDALL_LOOP;
  ALLARC_LOOP (Buffer->Data);
  {
    r_delete_entry (layer->arc_tree, (BoxTypePtr) arc);
    arc->X = SWAP_X (arc->X);
    arc->Y = SWAP_Y (arc->Y);
    arc->StartAngle = SWAP_ANGLE (arc->StartAngle);
    arc->Delta = SWAP_DELTA (arc->Delta);
    SetArcBoundingBox (arc);
    r_insert_entry (layer->arc_tree, (BoxTypePtr) arc, 0);
  }
  ENDALL_LOOP;
  ALLPOLYGON_LOOP (Buffer->Data);
  {
    r_delete_entry (layer->polygon_tree, (BoxTypePtr) polygon);
    POLYGONPOINT_LOOP (polygon);
    {
      point->X = SWAP_X (point->X);
      point->Y = SWAP_Y (point->Y);
    }
    END_LOOP;
    SetPolygonBoundingBox (polygon);
    r_insert_entry (layer->polygon_tree, (BoxTypePtr) polygon, 0);
    /* hmmm, how to handle clip */
  }
  ENDALL_LOOP;
  ALLTEXT_LOOP (Buffer->Data);
  {
    r_delete_entry (layer->text_tree, (BoxTypePtr) text);
    text->X = SWAP_X (text->X);
    text->Y = SWAP_Y (text->Y);
    TOGGLE_FLAG (ONSOLDERFLAG, text);
    SetTextBoundingBox (&PCB->Font, text);
    r_insert_entry (layer->text_tree, (BoxTypePtr) text, 0);
  }
  ENDALL_LOOP;
  /* swap silkscreen layers */
  swap = Buffer->Data->Layer[max_layer + SOLDER_LAYER];
  Buffer->Data->Layer[max_layer + SOLDER_LAYER] =
    Buffer->Data->Layer[max_layer + COMPONENT_LAYER];
  Buffer->Data->Layer[max_layer + COMPONENT_LAYER] = swap;

  /* swap layer groups when balanced */
  sgroup = GetLayerGroupNumberByNumber (max_layer + SOLDER_LAYER);
  cgroup = GetLayerGroupNumberByNumber (max_layer + COMPONENT_LAYER);
  if (PCB->LayerGroups.Number[cgroup] == PCB->LayerGroups.Number[sgroup])
    {
      for (j = k = 0; j < PCB->LayerGroups.Number[sgroup]; j++)
	{
	  int t1, t2;
	  Cardinal cnumber = PCB->LayerGroups.Entries[cgroup][k];
	  Cardinal snumber = PCB->LayerGroups.Entries[sgroup][j];

	  if (snumber >= max_layer)
	    continue;
	  swap = Buffer->Data->Layer[snumber];

	  while (cnumber >= max_layer)
	    {
	      k++;
	      cnumber = PCB->LayerGroups.Entries[cgroup][k];
	    }
	  Buffer->Data->Layer[snumber] = Buffer->Data->Layer[cnumber];
	  Buffer->Data->Layer[cnumber] = swap;
	  k++;
	  /* move the thermal flags with the layers */
	  ALLPIN_LOOP (Buffer->Data);
	  {
	    t1 = TEST_THERM (snumber, pin);
	    t2 = TEST_THERM (cnumber, pin);
	    ASSIGN_THERM (snumber, t2, pin);
	    ASSIGN_THERM (cnumber, t1, pin);
	  }
	  ENDALL_LOOP;
	  VIA_LOOP (Buffer->Data);
	  {
	    t1 = TEST_THERM (snumber, via);
	    t2 = TEST_THERM (cnumber, via);
	    ASSIGN_THERM (snumber, t2, via);
	    ASSIGN_THERM (cnumber, t1, via);
	  }
	  END_LOOP;
	}
    }
  SetBufferBoundingBox (Buffer);
}

/* ----------------------------------------------------------------------
 * moves the passed object to the passed buffer and removes it
 * from its original place
 */
void *
MoveObjectToBuffer (DataTypePtr Destination, DataTypePtr Src,
		    int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  /* setup local identifiers used by move operations */
  Dest = Destination;
  Source = Src;
  return (ObjectOperation (&MoveBufferFunctions, Type, Ptr1, Ptr2, Ptr3));
}

/* ----------------------------------------------------------------------
 * Adds the passed object to the passed buffer
 */
void *
CopyObjectToBuffer (DataTypePtr Destination, DataTypePtr Src,
		    int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  /* setup local identifiers used by Add operations */
  Dest = Destination;
  Source = Src;
  return (ObjectOperation (&AddBufferFunctions, Type, Ptr1, Ptr2, Ptr3));
}
