/*!
 * \file src/copy.c
 *
 * \brief Functions used to copy pins, elements ...
 *
 * It's necessary to copy data by calling create... since the base pointer
 * may change cause of dynamic memory allocation.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
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
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "global.h"

#include "copy.h"
#include "create.h"
#include "data.h"
#include "draw.h"
#include "mymem.h"
#include "mirror.h"
#include "misc.h"
#include "move.h"
#include "polygon.h"
#include "rats.h"
#include "rtree.h"
#include "select.h"
#include "undo.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void *CopyVia (PinType *);
static void *CopyLine (LayerType *, LineType *);
static void *CopyArc (LayerType *, ArcType *);
static void *CopyText (LayerType *, TextType *);
static void *CopyPolygon (LayerType *, PolygonType *);
static void *CopyElement (ElementType *);

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static Coord DeltaX, DeltaY;	/* movement vector */
static ObjectFunctionType CopyFunctions = {
  CopyLine,
  CopyText,
  CopyPolygon,
  CopyVia,
  CopyElement,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  CopyArc,
  NULL
};

/*!
 * \brief Copies data from one polygon to another.
 *
 * 'Dest' has to exist.
 */
PolygonType *
CopyPolygonLowLevel (PolygonType *Dest, PolygonType *Src)
{
  Cardinal hole = 0;
  Cardinal n;

  for (n = 0; n < Src->PointN; n++)
    {
      if (hole < Src->HoleIndexN && n == Src->HoleIndex[hole])
        {
          CreateNewHoleInPolygon (Dest);
          hole++;
        }
      CreateNewPointInPolygon (Dest, Src->Points[n].X, Src->Points[n].Y);
    }
  SetPolygonBoundingBox (Dest);
  Dest->Flags = Src->Flags;
  CLEAR_FLAG (NOCOPY_FLAGS, Dest);
  return (Dest);
}

/*!
 * \brief Copies data from one element to another and creates the
 * destination if necessary.
 */
ElementType *
CopyElementLowLevel (DataType *Data, ElementType *Src,
                     bool uniqueName, Coord dx, Coord dy, int mask_flags)
{
  int i;
  ElementType *Dest;

  /* both coordinates and flags are the same */
  Dest = CreateNewElement (Data, &PCB->Font,
			   MaskFlags (Src->Flags, mask_flags),
			   DESCRIPTION_NAME (Src), NAMEONPCB_NAME (Src),
			   VALUE_NAME (Src), DESCRIPTION_TEXT (Src).X + dx,
			   DESCRIPTION_TEXT (Src).Y + dy,
			   DESCRIPTION_TEXT (Src).Direction,
			   DESCRIPTION_TEXT (Src).Scale,
			   MaskFlags (DESCRIPTION_TEXT (Src).Flags,
				      mask_flags), uniqueName);

  /* abort on error */
  if (!Dest)
    return (Dest);

  ELEMENTLINE_LOOP (Src);
  {
    CreateNewLineInElement (Dest, line->Point1.X + dx,
			    line->Point1.Y + dy, line->Point2.X + dx,
			    line->Point2.Y + dy, line->Thickness);
  }
  END_LOOP;
  PIN_LOOP (Src);
  {
    CreateNewPin (Dest, pin->X + dx, pin->Y + dy, pin->Thickness,
		  pin->Clearance, pin->Mask, pin->DrillingHole,
		  pin->Name, pin->Number, MaskFlags (pin->Flags, mask_flags));
  }
  END_LOOP;
  PAD_LOOP (Src);
  {
    CreateNewPad (Dest, pad->Point1.X + dx, pad->Point1.Y + dy,
		  pad->Point2.X + dx, pad->Point2.Y + dy, pad->Thickness,
		  pad->Clearance, pad->Mask, pad->Name, pad->Number,
		  MaskFlags (pad->Flags, mask_flags));
  }
  END_LOOP;
  ARC_LOOP (Src);
  {
    CreateNewArcInElement (Dest, arc->X + dx, arc->Y + dy, arc->Width,
			   arc->Height, arc->StartAngle, arc->Delta,
			   arc->Thickness);
  }
  END_LOOP;

  for (i=0; i<Src->Attributes.Number; i++)
    CreateNewAttribute (& Dest->Attributes,
			Src->Attributes.List[i].name,
			Src->Attributes.List[i].value);

  Dest->MarkX = Src->MarkX + dx;
  Dest->MarkY = Src->MarkY + dy;

  SetElementBoundingBox (Data, Dest, &PCB->Font);
  return (Dest);
}

/*!
 * \brief Copies a via.
 */
static void *
CopyVia (PinType *Via)
{
  PinType *via;

  via = CreateNewViaEx (PCB->Data, Via->X + DeltaX, Via->Y + DeltaY,
		      Via->Thickness, Via->Clearance, Via->Mask,
		      Via->DrillingHole, Via->Name,
		      MaskFlags (Via->Flags, NOCOPY_FLAGS), Via->BuriedFrom, Via->BuriedTo);
  if (!via)
    return (via);
  DrawVia (via);
  AddObjectToCreateUndoList (VIA_TYPE, via, via, via);
  return (via);
}

/*!
 * \brief Copies a line.
 */
static void *
CopyLine (LayerType *Layer, LineType *Line)
{
  LineType *line;

  line = CreateDrawnLineOnLayer (Layer, Line->Point1.X + DeltaX,
				 Line->Point1.Y + DeltaY,
				 Line->Point2.X + DeltaX,
				 Line->Point2.Y + DeltaY, Line->Thickness,
				 Line->Clearance,
				 MaskFlags (Line->Flags, NOCOPY_FLAGS));
  if (!line)
    return (line);
  if (Line->Number)
    line->Number = strdup (Line->Number);
  DrawLine (Layer, line);
  AddObjectToCreateUndoList (LINE_TYPE, Layer, line, line);
  return (line);
}

/*!
 * \brief Copies an arc.
 */
static void *
CopyArc (LayerType *Layer, ArcType *Arc)
{
  ArcType *arc;

  arc = CreateNewArcOnLayer (Layer, Arc->X + DeltaX,
			     Arc->Y + DeltaY, Arc->Width, Arc->Height, Arc->StartAngle,
			     Arc->Delta, Arc->Thickness, Arc->Clearance,
			     MaskFlags (Arc->Flags, NOCOPY_FLAGS));
  if (!arc)
    return (arc);
  DrawArc (Layer, arc);
  AddObjectToCreateUndoList (ARC_TYPE, Layer, arc, arc);
  return (arc);
}

/*!
 * \brief Copies a text.
 */
static void *
CopyText (LayerType *Layer, TextType *Text)
{
  TextType *text;

  text = CreateNewText (Layer, &PCB->Font, Text->X + DeltaX,
			Text->Y + DeltaY, Text->Direction,
			Text->Scale, Text->TextString,
			MaskFlags (Text->Flags, NOCOPY_FLAGS));
  DrawText (Layer, text);
  AddObjectToCreateUndoList (TEXT_TYPE, Layer, text, text);
  return (text);
}

/*!
 * \brief Copies a polygon.
 */
static void *
CopyPolygon (LayerType *Layer, PolygonType *Polygon)
{
  PolygonType *polygon;

  polygon = CreateNewPolygon (Layer, NoFlags ());
  CopyPolygonLowLevel (polygon, Polygon);
  MovePolygonLowLevel (polygon, DeltaX, DeltaY);
  if (!Layer->polygon_tree)
    Layer->polygon_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (Layer->polygon_tree, (BoxType *) polygon, 0);
  InitClip (PCB->Data, Layer, polygon);
  DrawPolygon (Layer, polygon);
  AddObjectToCreateUndoList (POLYGON_TYPE, Layer, polygon, polygon);
  return (polygon);
}

/*!
 * \brief Copies an element onto the PCB, then does a draw.
 */
static void *
CopyElement (ElementType *Element)
{
  ElementType *element;

#ifdef DEBUG
  printf("Entered CopyElement, trying to copy element %s\n",
         Element->Name[1].TextString);
#endif

  element = CopyElementLowLevel (PCB->Data, Element,
                                 TEST_FLAG (UNIQUENAMEFLAG, PCB),
                                 DeltaX, DeltaY, NOCOPY_FLAGS);
  /* this call clears the polygons */
  AddObjectToCreateUndoList (ELEMENT_TYPE, element, element, element);
  if (PCB->ElementOn && (FRONT (element) || PCB->InvisibleObjectsOn))
    {
      DrawElementName (element);
      DrawElementPackage (element);
    }
  if (PCB->PinOn)
    {
      DrawElementPinsAndPads (element);
    }
#ifdef DEBUG
  printf(" ... Leaving CopyElement.\n");
#endif
  return (element);
}

/*!
 * \brief Pastes the contents of the buffer to the layout.
 *
 * \note Only visible objects are handled by the routine.
 */
bool
CopyPastebufferToLayout (Coord X, Coord Y)
{
  Cardinal i;
  bool changed = false;

#ifdef DEBUG
  printf("Entering CopyPastebufferToLayout.....\n");
#endif

  /* set movement vector */
  DeltaX = X - PASTEBUFFER->X, DeltaY = Y - PASTEBUFFER->Y;

  /* paste all layers */
  for (i = 0; i < max_copper_layer + SILK_LAYER; i++)
    {
      LayerType *sourcelayer = &PASTEBUFFER->Data->Layer[i];
      LayerType *destlayer = LAYER_PTR (i);

      if (destlayer->On)
	{
	  changed = changed ||
	    (sourcelayer->LineN != 0) ||
	    (sourcelayer->ArcN != 0) ||
	    (sourcelayer->PolygonN != 0) || (sourcelayer->TextN != 0);
	  LINE_LOOP (sourcelayer);
	  {
	    CopyLine (destlayer, line);
	  }
	  END_LOOP;
	  ARC_LOOP (sourcelayer);
	  {
	    CopyArc (destlayer, arc);
	  }
	  END_LOOP;
	  TEXT_LOOP (sourcelayer);
	  {
	    CopyText (destlayer, text);
	  }
	  END_LOOP;
	  POLYGON_LOOP (sourcelayer);
	  {
	    CopyPolygon (destlayer, polygon);
	  }
	  END_LOOP;
	}
    }

  /* paste elements */
  if (PCB->PinOn && PCB->ElementOn)
    {
      ELEMENT_LOOP (PASTEBUFFER->Data);
      {
#ifdef DEBUG
	printf("In CopyPastebufferToLayout, pasting element %s\n",
	      element->Name[1].TextString);
#endif
	if (FRONT (element) || PCB->InvisibleObjectsOn)
	  {
	    CopyElement (element);
	    changed = true;
	  }
      }
      END_LOOP;
    }

  /* finally the vias */
  if (PCB->ViaOn)
    {
      changed |= (PASTEBUFFER->Data->ViaN != 0);
      VIA_LOOP (PASTEBUFFER->Data);
      {
	CopyVia (via);
      }
      END_LOOP;
    }

  if (changed)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }

#ifdef DEBUG
  printf("  .... Leaving CopyPastebufferToLayout.\n");
#endif

  return (changed);
}

/*!
 * \brief Copies the object identified by its data pointers and the type
 * the new objects is moved by DX,DY.
 *
 * \note I assume that the appropriate layer ... is switched on.
 */
void *
CopyObject (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
	    Coord DX, Coord DY)
{
  void *ptr;

  /* setup movement vector */
  DeltaX = DX;
  DeltaY = DY;

  /* the subroutines add the objects to the undo-list */
  ptr = ObjectOperation (&CopyFunctions, Type, Ptr1, Ptr2, Ptr3);
  IncrementUndoSerialNumber ();
  return (ptr);
}
