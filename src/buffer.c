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

static char *rcsid = "$Id$";

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
#include "gui.h"
#include "mymem.h"
#include "mirror.h"
#include "misc.h"
#include "parse_l.h"
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
},

MoveBufferFunctions = {
    MoveLineToBuffer,
    MoveTextToBuffer,
    MovePolygonToBuffer,
    MoveViaToBuffer,
    MoveElementToBuffer,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    MoveArcToBuffer,
    MoveRatToBuffer
};

static int ExtraFlag = 0;

/* ---------------------------------------------------------------------------
 * copies a via to paste buffer
 */
static void *
AddViaToBuffer (PinTypePtr Via)
{
  return (CreateNewVia (Dest, Via->X, Via->Y, Via->Thickness, Via->Clearance,
			Via->Mask, Via->DrillingHole, Via->Name,
			Via->Flags & ~(FOUNDFLAG | ExtraFlag)));
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
			Rat->Flags & ~(FOUNDFLAG | ExtraFlag)));
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
			       Line->Flags & ~(FOUNDFLAG | ExtraFlag));
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
			       Arc->Flags & ~(FOUNDFLAG | ExtraFlag)));
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
			 Text->Flags & ~ExtraFlag));
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
  polygon->Flags = polygon->Flags & ~(FOUNDFLAG | ExtraFlag);
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
  CopyElementLowLevel (Dest, element, Element, False);
  element->Flags &= ~ExtraFlag;
  if (ExtraFlag)
    {
      ELEMENTTEXT_LOOP (element, 
	{
	  text->Flags &= ~ExtraFlag;
	}
      );
      PIN_LOOP (element, 
	{
	  pin->Flags &= ~ExtraFlag;
	}
      );
      PAD_LOOP (element, 
	{
	  pad->Flags &= ~ExtraFlag;
	}
      );
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

  r_delete_entry(Source->via_tree, (BoxType *)Via);
  via = GetViaMemory (Dest);
  *via = *Via;
  *Via = Source->Via[--Source->ViaN];
  r_substitute(Source->via_tree, (BoxType *)&Source->Via[Source->ViaN], (BoxType *)Via);
  via->Flags &= ~(WARNFLAG | FOUNDFLAG);
  memset (&Source->Via[Source->ViaN], 0, sizeof (PinType));
  if (!Dest->via_tree)
    Dest->via_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry(Dest->via_tree, (BoxType *)via, 0);
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
  *Rat = Source->Rat[--Source->RatN];
  Rat->Flags &= ~FOUNDFLAG;
  memset (&Source->Rat[Source->RatN], 0, sizeof (RatType));
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

  r_delete_entry(Layer->line_tree, (BoxTypePtr)Line);
  lay = &Dest->Layer[GetLayerNumber (Source, Layer)];
  line = GetLineMemory (lay);
  *line = *Line;
  line->Flags &= ~FOUNDFLAG;
    /* line pointers being shuffled */
  *Line = Layer->Line[--Layer->LineN];
  r_substitute(Layer->line_tree, (BoxTypePtr)&Layer->Line[Layer->LineN], (BoxTypePtr)Line);
  memset (&Layer->Line[Layer->LineN], 0, sizeof (LineType));
  if (!lay->line_tree)
    lay->line_tree = r_create_tree(NULL, 0, 0);
  r_insert_entry(lay->line_tree, (BoxTypePtr)line, 0);
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

  r_delete_entry(Layer->arc_tree, (BoxTypePtr)Arc);
  lay = &Dest->Layer[GetLayerNumber (Source, Layer)];
  arc = GetArcMemory (lay);
  *arc = *Arc;
  arc->Flags &= ~FOUNDFLAG;
    /* arc pointers being shuffled */
  *Arc = Layer->Arc[--Layer->ArcN];
  r_substitute(Layer->arc_tree, (BoxTypePtr)&Layer->Arc[Layer->ArcN], (BoxTypePtr)Arc);
  memset (&Layer->Arc[Layer->ArcN], 0, sizeof (ArcType));
  if (!lay->arc_tree)
    lay->arc_tree = r_create_tree(NULL, 0, 0);
  r_insert_entry(lay->arc_tree, (BoxTypePtr)arc, 0);
  return (arc);
}

/* ---------------------------------------------------------------------------
 * moves a text to buffer without allocating memory for the name
 */
static void *
MoveTextToBuffer (LayerTypePtr Layer, TextTypePtr Text)
{
  TextTypePtr text;

  text = GetTextMemory (&Dest->Layer[GetLayerNumber (Source, Layer)]);
  *text = *Text;
  *Text = Layer->Text[--Layer->TextN];
  memset (&Layer->Text[Layer->TextN], 0, sizeof (TextType));
  return (text);
}

/* ---------------------------------------------------------------------------
 * moves a polygon to buffer. Doesn't allocate memory for the points
 */
static void *
MovePolygonToBuffer (LayerTypePtr Layer, PolygonTypePtr Polygon)
{
  PolygonTypePtr polygon;

  polygon = GetPolygonMemory (&Dest->Layer[GetLayerNumber (Source, Layer)]);
  *polygon = *Polygon;
  polygon->Flags &= ~FOUNDFLAG;
  *Polygon = Layer->Polygon[--Layer->PolygonN];
  memset (&Layer->Polygon[Layer->PolygonN], 0, sizeof (PolygonType));
  return (polygon);
}

/* ---------------------------------------------------------------------------
 * moves a element to buffer without allocating memory for pins/names
 */
static void *
MoveElementToBuffer (ElementTypePtr Element)
{
  ElementTypePtr element;

  r_delete_entry(Source->element_tree, (BoxType *)Element);
  PIN_LOOP (Element,
    {
      r_delete_entry (Source->pin_tree, (BoxType *)pin);
    }
  );
  PAD_LOOP (Element,
    {
      r_delete_entry (Source->pad_tree, (BoxType *)pad);
    }
  );
  element = GetElementMemory (Dest);
  *element = *Element;
  PIN_LOOP (element, 
    {
      pin->Flags &= ~(WARNFLAG | FOUNDFLAG);
      pin->Element = element;
    }
  );
  PAD_LOOP (element, 
    {
      pad->Flags &= ~(WARNFLAG | FOUNDFLAG);
      pad->Element = element;
    }
  );
  *Element = Source->Element[--Source->ElementN];
   /* deal with element pointer changing */
  r_substitute (Source->element_tree, (BoxType *)&Source->Element[Source->ElementN],
                (BoxType *)Element);
  PIN_LOOP (Element,
    {
       pin->Element = Element;
    }
  );
  PAD_LOOP (Element,
    {
       pad->Element = Element;
    }
  );
  memset (&Source->Element[Source->ElementN], 0, sizeof (ElementType));
   /* do we need rtrees for the buffer ? */
  if (!Dest->element_tree)
    Dest->element_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry(Dest->element_tree, (BoxType *)element, 0);
  if (!Dest->pin_tree)
    Dest->pin_tree = r_create_tree (NULL, 0, 0);
  if (!Dest->pad_tree)
    Dest->pad_tree = r_create_tree (NULL, 0, 0);
  PIN_LOOP (element,
    {
      r_insert_entry (Dest->pin_tree, (BoxType *)pin, 0);
    }
  ); 
  PAD_LOOP (element,
    {
      r_insert_entry (Dest->pad_tree, (BoxType *)pad, 0);
    }
  ); 
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
  if (Buffer)
    FreeDataMemory (Buffer->Data);
}

/* ----------------------------------------------------------------------
 * copies all selected and visible objects to the paste buffer
 * returns True if any objects have been removed
 */
void
AddSelectedToBuffer (BufferTypePtr Buffer, Location X, Location Y,
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

  watchCursor ();
  ClearBuffer (Buffer);
  if (FromFile)
    {
      if (!ParseElementFile (Buffer->Data, Name))
	{
	  if (Settings.ShowSolderSide)
	    SwapBuffer (Buffer);
	  else
	    {
	      ELEMENT_LOOP (Buffer->Data, 
		{
		  SetElementBoundingBox (Buffer->Data, element, &PCB->Font);
		}
	      );
	      ALLPOLYGON_LOOP (Buffer->Data, 
		{
		  SetPolygonBoundingBox (polygon);
		}
	      );
	      ALLTEXT_LOOP (Buffer->Data, 
		{
		  SetTextBoundingBox (&PCB->Font, text);
		}
	      );
	    }
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
	  restoreCursor ();
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
	  restoreCursor ();
	  return (True);
	}
    }
  /* release memory which might have been aquired */
  ClearBuffer (Buffer);
  restoreCursor ();
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
      Message ("ERROR!  Buffer doesn't contain a single element\n");
      return (False);
    }
  element = &Buffer->Data->Element[0];
  Buffer->Data->ElementN = 0;
  ClearBuffer (Buffer);
  ELEMENTLINE_LOOP (element, 
    {
      CreateNewLineOnLayer (&Buffer->Data->SILKLAYER,
			    line->Point1.X, line->Point1.Y,
			    line->Point2.X, line->Point2.Y,
			    line->Thickness, 0, 0);
      if (line)
	line->Number = MyStrdup (NAMEONPCB_NAME (element), "SmashBuffer");
    }
  );
  ARC_LOOP (element, 
    {
      CreateNewArcOnLayer (&Buffer->Data->SILKLAYER,
			   arc->X, arc->Y, arc->Width, arc->StartAngle,
			   arc->Delta, arc->Thickness, 0, 0);
    }
  );
  PIN_LOOP (element, 
    {
      CreateNewVia (Buffer->Data, pin->X, pin->Y,
		    pin->Thickness, pin->Clearance, pin->Mask,
		    pin->DrillingHole, pin->Number,
		    VIAFLAG | (pin->Flags & HOLEFLAG));
    }
  );
  group =
    GetLayerGroupNumberByNumber (MAX_LAYER +
				 (SWAP_IDENT ? SOLDER_LAYER :
				  COMPONENT_LAYER));
  layer = &Buffer->Data->Layer[PCB->LayerGroups.Entries[group][0]];
  PAD_LOOP (element, 
    {
      LineTypePtr line;
      line = CreateNewLineOnLayer (layer, pad->Point1.X, pad->Point1.Y,
				   pad->Point2.X, pad->Point2.Y,
				   pad->Thickness, pad->Clearance, 0);
      if (line)
	line->Number = MyStrdup (pad->Number, "SmashBuffer");
    }
  );
  FreeElementMemory (element);
  SaveFree (element);
  return (True);
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
  Boolean hasParts = False;

  Element = CreateNewElement (PCB->Data, NULL, &PCB->Font, NOFLAG,
			      NULL, NULL, NULL, PASTEBUFFER->X,
			      PASTEBUFFER->Y, 0, 100,
			      SWAP_IDENT ? ONSOLDERFLAG : NOFLAG,
			      False);
  if (!Element)
    return (False);
  VIA_LOOP (Buffer->Data, 
    {
      char num[8];
      if (via->Mask < via->Thickness)
	via->Mask = via->Thickness + 2 * MASKFRAME;
      if (via->Name)
	CreateNewPin (Element, via->X, via->Y, via->Thickness,
		      via->Clearance, via->Mask, via->DrillingHole,
		      NULL, via->Name, (via->Flags &
					~(VIAFLAG | FOUNDFLAG |
					  SELECTEDFLAG | WARNFLAG)) |
		      PINFLAG);
      else
	{
	  sprintf (num, "%d", pin_n++);
	  CreateNewPin (Element, via->X, via->Y, via->Thickness,
			via->Clearance, via->Mask, via->DrillingHole,
			NULL, num, (via->Flags &
				    ~(VIAFLAG | FOUNDFLAG | SELECTEDFLAG
				      | WARNFLAG)) | PINFLAG);
	}
      hasParts = True;
    }
  );
  /* get the component-side SM pads */
  group = GetLayerGroupNumberByNumber (MAX_LAYER +
				       (SWAP_IDENT ? SOLDER_LAYER :
					COMPONENT_LAYER));
  GROUP_LOOP (group,
    {
      char num[8];
      LINE_LOOP (layer, 
	{
	  if (line->Point1.X == line->Point2.X
	      || line->Point1.Y == line->Point2.Y)
	    {
	      sprintf (num, "%d", pin_n++);
	      CreateNewPad (Element, line->Point1.X,
			    line->Point1.Y, line->Point2.X, line->Point2.Y,
			    line->Thickness, line->Clearance,
			    line->Thickness + line->Clearance, NULL,
			    line->Number ? line->Number : num,
			    SWAP_IDENT ? ONSOLDERFLAG : NOFLAG);
	      hasParts = True;
	    }
	}
      );
    }
  );
  /* now get the opposite side pads */
  group = GetLayerGroupNumberByNumber (MAX_LAYER +
				       (SWAP_IDENT ? COMPONENT_LAYER :
					SOLDER_LAYER));
  GROUP_LOOP (group,
    {
      Boolean warned = False;
      char num[8];
      LINE_LOOP (layer, 
	{
	  if (line->Point1.X == line->Point2.X
	      || line->Point1.Y == line->Point2.Y)
	    {
	      sprintf (num, "%d", pin_n++);
	      CreateNewPad (Element, line->Point1.X,
			    line->Point1.Y, line->Point2.X, line->Point2.Y,
			    line->Thickness, line->Clearance,
			    line->Thickness + line->Clearance, NULL,
			    line->Number ? line->Number : num,
			    SWAP_IDENT ? NOFLAG : ONSOLDERFLAG);
	      if (!hasParts && !warned)
		{
		  warned = True;
		  Message ("Warning: All of the pads are on the opposite\n"
			   "side from the component - that's probably not what\n"
			   "you wanted\n");
		}
	      hasParts = True;
	    }
	 }
      );
    }
  );
  /* now add the silkscreen. NOTE: elements must have pads or pins too */
  LINE_LOOP (&Buffer->Data->SILKLAYER, 
    {
      if (line->Number && !NAMEONPCB_NAME (Element))
	NAMEONPCB_NAME (Element) = MyStrdup (line->Number,
					     "ConvertBufferToElement");
      CreateNewLineInElement (Element, line->Point1.X,
			      line->Point1.Y, line->Point2.X,
			      line->Point2.Y, line->Thickness);
    }
  );
  ARC_LOOP (&Buffer->Data->SILKLAYER, 
    {
      CreateNewArcInElement (Element, arc->X, arc->Y, arc->Width,
			     arc->Height, arc->StartAngle, arc->Delta,
			     arc->Thickness);
    }
  );
  if (!hasParts)
    {
      DestroyObject (PCB->Data, ELEMENT_TYPE, Element, Element, Element);
      Message ("There was nothing to convert!\n"
	       "Elements must have some pads or pins.\n");
      return (False);
    }
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
  VIA_LOOP (Buffer->Data, 
    {
      r_delete_entry (Buffer->Data->via_tree, (BoxTypePtr)via);
      ROTATE_VIA_LOWLEVEL (via, Buffer->X, Buffer->Y, Number);
      SetPinBoundingBox(via);
      r_insert_entry (Buffer->Data->via_tree, (BoxTypePtr)via, 0);
    }
  );

  /* elements */
  ELEMENT_LOOP (Buffer->Data, 
    {
      RotateElementLowLevel (Buffer->Data, element, Buffer->X, Buffer->Y, Number);
    }
  );

  /* all layer related objects */
  ALLLINE_LOOP (Buffer->Data, 
    {
      r_delete_entry (layer->line_tree, (BoxTypePtr)line);
      RotateLineLowLevel (line, Buffer->X, Buffer->Y, Number);
      SetLineBoundingBox (line);
      r_insert_entry (layer->line_tree, (BoxTypePtr)line, 0);
    }
  );
  ALLARC_LOOP (Buffer->Data, 
    {
      r_delete_entry (layer->arc_tree, (BoxTypePtr)arc);
      RotateArcLowLevel (arc, Buffer->X, Buffer->Y, Number);
      r_insert_entry (layer->arc_tree, (BoxTypePtr)arc, 0);
    }
  );
  ALLTEXT_LOOP (Buffer->Data, 
    {
      RotateTextLowLevel (text, Buffer->X, Buffer->Y, Number);
    }
  );
  ALLPOLYGON_LOOP (Buffer->Data, 
    {
      RotatePolygonLowLevel (polygon, Buffer->X, Buffer->Y, Number);
    }
  );

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
      Message("You can't mirror a buffer that has element(s)!\n");
      return;
    }
  for (i = 0; i < MAX_LAYER + 2; i++)
    {
      LayerTypePtr layer = LAYER_PTR(i);
      if (layer->TextN)
        {
          Message("You can't mirror a buffer that has text(s)!\n");
          return;
        }
    }
  /* set buffer offset to 'mark' position */
  Buffer->X = SWAP_X (Buffer->X);
  Buffer->Y = SWAP_Y (Buffer->Y);
  VIA_LOOP (Buffer->Data, 
    {
      via->X = SWAP_X (via->X);
      via->Y = SWAP_Y (via->Y);
    }
  );
  ALLLINE_LOOP (Buffer->Data, 
    {
      line->Point1.X = SWAP_X (line->Point1.X);
      line->Point1.Y = SWAP_Y (line->Point1.Y);
      line->Point2.X = SWAP_X (line->Point2.X);
      line->Point2.Y = SWAP_Y (line->Point2.Y);
    }
  );
  ALLARC_LOOP (Buffer->Data, 
    {
      arc->X = SWAP_X (arc->X);
      arc->Y = SWAP_Y (arc->Y);
      arc->StartAngle = SWAP_ANGLE (arc->StartAngle);
      arc->Delta = SWAP_DELTA (arc->Delta);
      SetArcBoundingBox(arc);
    }
  );
  ALLPOLYGON_LOOP (Buffer->Data, 
    {
      POLYGONPOINT_LOOP (polygon, 
	{
	  point->X = SWAP_X (point->X);
	  point->Y = SWAP_Y (point->Y);
	}
      );
      SetPolygonBoundingBox (polygon);
    }
  );
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

  ELEMENT_LOOP (Buffer->Data, 
    {
      MirrorElementCoordinates (Buffer->Data, element, 0);
    }
  );
  /* set buffer offset to 'mark' position */
  Buffer->X = SWAP_X (Buffer->X);
  Buffer->Y = SWAP_Y (Buffer->Y);
  VIA_LOOP (Buffer->Data, 
    {
      r_delete_entry (Buffer->Data->via_tree, (BoxTypePtr)via);
      via->X = SWAP_X (via->X);
      via->Y = SWAP_Y (via->Y);
      SetPinBoundingBox(via);
      r_insert_entry (Buffer->Data->via_tree, (BoxTypePtr)via, 0);
    }
  );
  ALLLINE_LOOP (Buffer->Data, 
    {
      r_delete_entry(layer->line_tree, (BoxTypePtr)line);
      line->Point1.X = SWAP_X (line->Point1.X);
      line->Point1.Y = SWAP_Y (line->Point1.Y);
      line->Point2.X = SWAP_X (line->Point2.X);
      line->Point2.Y = SWAP_Y (line->Point2.Y);
      SetLineBoundingBox(line);
      r_insert_entry(layer->line_tree, (BoxTypePtr)line, 0);
    }
  );
  ALLARC_LOOP (Buffer->Data, 
    {
      r_delete_entry(layer->arc_tree, (BoxTypePtr)arc);
      arc->X = SWAP_X (arc->X);
      arc->Y = SWAP_Y (arc->Y);
      arc->StartAngle = SWAP_ANGLE (arc->StartAngle);
      arc->Delta = SWAP_DELTA (arc->Delta);
      SetArcBoundingBox(arc);
      r_insert_entry(layer->arc_tree, (BoxTypePtr)arc, 0);
    }
  );
  ALLPOLYGON_LOOP (Buffer->Data, 
    {
      POLYGONPOINT_LOOP (polygon, 
	{
	  point->X = SWAP_X (point->X);
	  point->Y = SWAP_Y (point->Y);
	}
      );
      SetPolygonBoundingBox (polygon);
    }
  );
  ALLTEXT_LOOP (Buffer->Data, 
    {
      text->X = SWAP_X (text->X);
      text->Y = SWAP_Y (text->Y);
      TOGGLE_FLAG (ONSOLDERFLAG, text);
      SetTextBoundingBox (&PCB->Font, text);
    }
  );
  /* swap silkscreen layers */
  swap = Buffer->Data->Layer[MAX_LAYER + SOLDER_LAYER];
  Buffer->Data->Layer[MAX_LAYER + SOLDER_LAYER] =
    Buffer->Data->Layer[MAX_LAYER + COMPONENT_LAYER];
  Buffer->Data->Layer[MAX_LAYER + COMPONENT_LAYER] = swap;

  /* swap layer groups when balanced */
  sgroup = GetLayerGroupNumberByNumber (MAX_LAYER + SOLDER_LAYER);
  cgroup = GetLayerGroupNumberByNumber (MAX_LAYER + COMPONENT_LAYER);
  if (PCB->LayerGroups.Number[cgroup] == PCB->LayerGroups.Number[sgroup])
    {
      for (j = k = 0; j < PCB->LayerGroups.Number[sgroup]; j++)
	{
	  int t1, t2, f1, f2;
	  Cardinal cnumber = PCB->LayerGroups.Entries[cgroup][k];
	  Cardinal snumber = PCB->LayerGroups.Entries[sgroup][j];

	  if (snumber >= MAX_LAYER)
	    continue;
	  swap = Buffer->Data->Layer[snumber];

	  while (cnumber >= MAX_LAYER)
	    {
	      k++;
	      cnumber = PCB->LayerGroups.Entries[cgroup][k];
	    }
	  Buffer->Data->Layer[snumber] = Buffer->Data->Layer[cnumber];
	  Buffer->Data->Layer[cnumber] = swap;
	  k++;
	  /* move the thermal flags with the layers */
	  t1 = L0THERMFLAG << snumber;
	  t2 = L0THERMFLAG << cnumber;
	  ALLPIN_LOOP (Buffer->Data, 
	    {
	      f1 = (TEST_FLAG (t1, pin)) ? t2 : 0;
	      f2 = (TEST_FLAG (t2, pin)) ? t1 : 0;
	      pin->Flags = (pin->Flags & ~t1 & ~t2) | f1 | f2;
	    }
	  );
	  VIA_LOOP (Buffer->Data, 
	    {
	      f1 = (TEST_FLAG (t1, via)) ? t2 : 0;
	      f2 = (TEST_FLAG (t2, via)) ? t1 : 0;
	      via->Flags = (via->Flags & ~t1 & ~t2) | f1 | f2;
	    }
	  );
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
