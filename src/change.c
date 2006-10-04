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

/* functions used to change object properties
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>

#include "global.h"

#include "change.h"
#include "create.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "mymem.h"
#include "misc.h"
#include "mirror.h"
#include "output.h"
#include "polygon.h"
#include "rats.h"
#include "remove.h"
#include "rtree.h"
#include "search.h"
#include "select.h"
#include "set.h"
#include "thermal.h"
#include "undo.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void *ChangePinSize (ElementTypePtr, PinTypePtr);
static void *ChangePinClearSize (ElementTypePtr, PinTypePtr);
static void *ChangePinMaskSize (ElementTypePtr, PinTypePtr);
static void *ChangePadSize (ElementTypePtr, PadTypePtr);
static void *ChangePadClearSize (ElementTypePtr, PadTypePtr);
static void *ChangePadMaskSize (ElementTypePtr, PadTypePtr);
static void *ChangePin2ndSize (ElementTypePtr, PinTypePtr);
static void *ChangeElement2ndSize (ElementTypePtr);
static void *ChangeViaSize (PinTypePtr);
static void *ChangeVia2ndSize (PinTypePtr);
static void *ChangeViaClearSize (PinTypePtr);
static void *ChangeViaMaskSize (PinTypePtr);
static void *ChangeLineSize (LayerTypePtr, LineTypePtr);
static void *ChangeLineClearSize (LayerTypePtr, LineTypePtr);
static void *ChangeArcSize (LayerTypePtr, ArcTypePtr);
static void *ChangeArcClearSize (LayerTypePtr, ArcTypePtr);
static void *ChangeTextSize (LayerTypePtr, TextTypePtr);
static void *ChangeElementSize (ElementTypePtr);
static void *ChangeElementNameSize (ElementTypePtr);
static void *ChangePinName (ElementTypePtr, PinTypePtr);
static void *ChangePadName (ElementTypePtr, PadTypePtr);
static void *ChangeViaName (PinTypePtr);
static void *ChangeLineName (LayerTypePtr, LineTypePtr);
static void *ChangeElementName (ElementTypePtr);
static void *ChangeTextName (LayerTypePtr, TextTypePtr);
static void *ChangeElementSquare (ElementTypePtr);
static void *SetElementSquare (ElementTypePtr);
static void *ClrElementSquare (ElementTypePtr);
static void *ChangeElementOctagon (ElementTypePtr);
static void *SetElementOctagon (ElementTypePtr);
static void *ClrElementOctagon (ElementTypePtr);
static void *ChangePinSquare (ElementTypePtr, PinTypePtr);
static void *SetPinSquare (ElementTypePtr, PinTypePtr);
static void *ClrPinSquare (ElementTypePtr, PinTypePtr);
static void *ChangePinOctagon (ElementTypePtr, PinTypePtr);
static void *SetPinOctagon (ElementTypePtr, PinTypePtr);
static void *ClrPinOctagon (ElementTypePtr, PinTypePtr);
static void *ChangeViaOctagon (PinTypePtr);
static void *SetViaOctagon (PinTypePtr);
static void *ClrViaOctagon (PinTypePtr);
static void *ChangePadSquare (ElementTypePtr, PadTypePtr);
static void *SetPadSquare (ElementTypePtr, PadTypePtr);
static void *ClrPadSquare (ElementTypePtr, PadTypePtr);
static void *ChangeViaThermal (PinTypePtr);
static void *ChangePinThermal (ElementTypePtr, PinTypePtr);
static void *ChangeLineJoin (LayerTypePtr, LineTypePtr);
static void *SetLineJoin (LayerTypePtr, LineTypePtr);
static void *ClrLineJoin (LayerTypePtr, LineTypePtr);
static void *ChangeArcJoin (LayerTypePtr, ArcTypePtr);
static void *SetArcJoin (LayerTypePtr, ArcTypePtr);
static void *ClrArcJoin (LayerTypePtr, ArcTypePtr);
static void *ChangePolyClear (LayerTypePtr, PolygonTypePtr);

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static int Delta;		/* change of size */
static int Absolute;		/* Absolute size */
static char *NewName;		/* new name */
static ObjectFunctionType ChangeSizeFunctions = {
  ChangeLineSize,
  ChangeTextSize,
  ChangePolyClear,
  ChangeViaSize,
  ChangeElementSize,		/* changes silk screen line width */
  ChangeElementNameSize,
  ChangePinSize,
  ChangePadSize,
  NULL,
  NULL,
  ChangeArcSize,
  NULL
};
static ObjectFunctionType Change2ndSizeFunctions = {
  NULL,
  NULL,
  NULL,
  ChangeVia2ndSize,
  ChangeElement2ndSize,
  NULL,
  ChangePin2ndSize,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
static ObjectFunctionType ChangeThermalFunctions = {
  NULL,
  NULL,
  NULL,
  ChangeViaThermal,
  NULL,
  NULL,
  ChangePinThermal,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
static ObjectFunctionType ChangeClearSizeFunctions = {
  ChangeLineClearSize,
  NULL,
  NULL,
  ChangeViaClearSize,
  NULL,
  NULL,
  ChangePinClearSize,
  ChangePadClearSize,
  NULL,
  NULL,
  ChangeArcClearSize,
  NULL
};
static ObjectFunctionType ChangeNameFunctions = {
  ChangeLineName,
  ChangeTextName,
  NULL,
  ChangeViaName,
  ChangeElementName,
  NULL,
  ChangePinName,
  ChangePadName,
  NULL,
  NULL,
  NULL,
  NULL
};
static ObjectFunctionType ChangeSquareFunctions = {
  NULL,
  NULL,
  NULL,
  NULL,
  ChangeElementSquare,
  NULL,
  ChangePinSquare,
  ChangePadSquare,
  NULL,
  NULL,
  NULL,
  NULL
};
static ObjectFunctionType ChangeJoinFunctions = {
  ChangeLineJoin,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  ChangeArcJoin,
  NULL
};
static ObjectFunctionType ChangeOctagonFunctions = {
  NULL,
  NULL,
  NULL,
  ChangeViaOctagon,
  ChangeElementOctagon,
  NULL,
  ChangePinOctagon,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
static ObjectFunctionType ChangeMaskSizeFunctions = {
  NULL,
  NULL,
  NULL,
  ChangeViaMaskSize,
#if 0
  ChangeElementMaskSize,
#else
  NULL,
#endif
  NULL,
  ChangePinMaskSize,
  ChangePadMaskSize,
  NULL,
  NULL,
  NULL,
  NULL
};
static ObjectFunctionType SetSquareFunctions = {
  NULL,
  NULL,
  NULL,
  NULL,
  SetElementSquare,
  NULL,
  SetPinSquare,
  SetPadSquare,
  NULL,
  NULL,
  NULL,
  NULL
};
static ObjectFunctionType SetJoinFunctions = {
  SetLineJoin,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  SetArcJoin,
  NULL
};
static ObjectFunctionType SetOctagonFunctions = {
  NULL,
  NULL,
  NULL,
  SetViaOctagon,
  SetElementOctagon,
  NULL,
  SetPinOctagon,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
static ObjectFunctionType ClrSquareFunctions = {
  NULL,
  NULL,
  NULL,
  NULL,
  ClrElementSquare,
  NULL,
  ClrPinSquare,
  ClrPadSquare,
  NULL,
  NULL,
  NULL,
  NULL
};
static ObjectFunctionType ClrJoinFunctions = {
  ClrLineJoin,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  ClrArcJoin,
  NULL
};
static ObjectFunctionType ClrOctagonFunctions = {
  NULL,
  NULL,
  NULL,
  ClrViaOctagon,
  ClrElementOctagon,
  NULL,
  ClrPinOctagon,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

/* ---------------------------------------------------------------------------
 * changes the thermal on a via
 * returns TRUE if changed
 */
static void *
ChangeViaThermal (PinTypePtr Via)
{
  RestoreToPolygon (PCB->Data, VIA_TYPE, CURRENT, Via);
  AddObjectToClearPolyUndoList (VIA_TYPE, Via, Via, Via, False);
  AddObjectToFlagUndoList (VIA_TYPE, Via, Via, Via);
  if (!Delta)			/* remove the thermals */
    CLEAR_THERM (INDEXOFCURRENT, Via);
  else
    ASSIGN_THERM (INDEXOFCURRENT, Delta, Via);
  AddObjectToClearPolyUndoList (VIA_TYPE, Via, Via, Via, True);
  ClearFromPolygon (PCB->Data, VIA_TYPE, CURRENT, Via);
  DrawVia (Via, 0);
  return Via;
}

/* ---------------------------------------------------------------------------
 * changes the thermal on a pin 
 * returns TRUE if changed
 */
static void *
ChangePinThermal (ElementTypePtr element, PinTypePtr Pin)
{
  RestoreToPolygon (PCB->Data, VIA_TYPE, CURRENT, Pin);
  AddObjectToClearPolyUndoList (PIN_TYPE, element, Pin, Pin, False);
  AddObjectToFlagUndoList (PIN_TYPE, element, Pin, Pin);
  if (!Delta)			/* remove the thermals */
    CLEAR_THERM (INDEXOFCURRENT, Pin);
  else
    ASSIGN_THERM (INDEXOFCURRENT, Delta, Pin);
  ClearFromPolygon (PCB->Data, VIA_TYPE, CURRENT, Pin);
  AddObjectToClearPolyUndoList (PIN_TYPE, element, Pin, Pin, True);
  DrawPin (Pin, 0);
  return Pin;
}

/* ---------------------------------------------------------------------------
 * changes the size of a via
 * returns TRUE if changed
 */
static void *
ChangeViaSize (PinTypePtr Via)
{
  BDimension value = Absolute ? Absolute : Via->Thickness + Delta;

  if (TEST_FLAG (LOCKFLAG, Via))
    return (NULL);
  if (!TEST_FLAG (HOLEFLAG, Via) && value <= MAX_PINORVIASIZE &&
      value >= MIN_PINORVIASIZE &&
      value >= Via->DrillingHole + MIN_PINORVIACOPPER &&
      value != Via->Thickness)
    {
      AddObjectToSizeUndoList (VIA_TYPE, Via, Via, Via);
      EraseVia (Via);
      r_delete_entry (PCB->Data->via_tree, (BoxType *) Via);
      RestoreToPolygon (PCB->Data, PIN_TYPE, Via, Via);
      if (Via->Mask)
	{
	  AddObjectToMaskSizeUndoList (VIA_TYPE, Via, Via, Via);
	  Via->Mask += value - Via->Thickness;
	}
      Via->Thickness = value;
      SetPinBoundingBox (Via);
      r_insert_entry (PCB->Data->via_tree, (BoxType *) Via, 0);
      ClearFromPolygon (PCB->Data, VIA_TYPE, Via, Via);
      DrawVia (Via, 0);
      return (Via);
    }
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the drilling hole of a via
 * returns TRUE if changed
 */
static void *
ChangeVia2ndSize (PinTypePtr Via)
{
  BDimension value = (Absolute) ? Absolute : Via->DrillingHole + Delta;

  if (TEST_FLAG (LOCKFLAG, Via))
    return (NULL);
  if (value <= MAX_PINORVIASIZE &&
      value >= MIN_PINORVIAHOLE && (TEST_FLAG (HOLEFLAG, Via) ||
				    value <=
				    Via->Thickness - MIN_PINORVIACOPPER)
      && value != Via->DrillingHole)
    {
      AddObjectTo2ndSizeUndoList (VIA_TYPE, Via, Via, Via);
      EraseVia (Via);
      Via->DrillingHole = value;
      if (TEST_FLAG (HOLEFLAG, Via))
	{
	  AddObjectToSizeUndoList (VIA_TYPE, Via, Via, Via);
	  Via->Thickness = value;
	}
      DrawVia (Via, 0);
      return (Via);
    }
  return (NULL);
}


/* ---------------------------------------------------------------------------
 * changes the clearance size of a via 
 * returns TRUE if changed
 */
static void *
ChangeViaClearSize (PinTypePtr Via)
{
  BDimension value = (Absolute) ? Absolute : Via->Clearance + Delta;

  if (TEST_FLAG (LOCKFLAG, Via))
    return (NULL);
  value = MIN (MAX_LINESIZE, value);
  if (value < 0)
    value = 0;
  if (Delta < 0 && value < PCB->Bloat * 2)
    value = 0;
  if ((Delta > 0 || Absolute) && value < PCB->Bloat * 2)
    value = PCB->Bloat * 2 + 2;
  if (Via->Clearance == value)
    return NULL;
  RestoreToPolygon (PCB->Data, VIA_TYPE, Via, Via);
  AddObjectToClearSizeUndoList (VIA_TYPE, Via, Via, Via);
  EraseVia (Via);
  r_delete_entry (PCB->Data->via_tree, (BoxType *) Via);
  Via->Clearance = value;
  SetPinBoundingBox (Via);
  r_insert_entry (PCB->Data->via_tree, (BoxType *) Via, 0);
  ClearFromPolygon (PCB->Data, VIA_TYPE, Via, Via);
  DrawVia (Via, 0);
  Via->Element = NULL;
  return (Via);
}


/* ---------------------------------------------------------------------------
 * changes the size of a pin
 * returns TRUE if changed
 */
static void *
ChangePinSize (ElementTypePtr Element, PinTypePtr Pin)
{
  BDimension value = (Absolute) ? Absolute : Pin->Thickness + Delta;

  if (TEST_FLAG (LOCKFLAG, Pin))
    return (NULL);
  if (!TEST_FLAG (HOLEFLAG, Pin) && value <= MAX_PINORVIASIZE &&
      value >= MIN_PINORVIASIZE &&
      value >= Pin->DrillingHole + MIN_PINORVIACOPPER &&
      value != Pin->Thickness)
    {
      AddObjectToSizeUndoList (PIN_TYPE, Element, Pin, Pin);
      AddObjectToMaskSizeUndoList (PIN_TYPE, Element, Pin, Pin);
      ErasePin (Pin);
      r_delete_entry (PCB->Data->pin_tree, &Pin->BoundingBox);
      RestoreToPolygon (PCB->Data, PIN_TYPE, Element, Pin);
      Pin->Mask += value - Pin->Thickness;
      Pin->Thickness = value;
      /* SetElementBB updates all associated rtrees */
      SetElementBoundingBox (PCB->Data, Element, &PCB->Font);
      ClearFromPolygon (PCB->Data, PIN_TYPE, Element, Pin);
      DrawPin (Pin, 0);
      return (Pin);
    }
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the clearance size of a pin
 * returns TRUE if changed
 */
static void *
ChangePinClearSize (ElementTypePtr Element, PinTypePtr Pin)
{
  BDimension value = (Absolute) ? Absolute : Pin->Clearance + Delta;

  if (TEST_FLAG (LOCKFLAG, Pin))
    return (NULL);
  value = MIN (MAX_LINESIZE, MAX (value, PCB->Bloat * 2 + 2));
  if (Pin->Clearance == value)
    return NULL;
  RestoreToPolygon (PCB->Data, PIN_TYPE, Element, Pin);
  AddObjectToClearSizeUndoList (PIN_TYPE, Element, Pin, Pin);
  ErasePin (Pin);
  r_delete_entry (PCB->Data->pin_tree, &Pin->BoundingBox);
  Pin->Clearance = value;
  /* SetElementBB updates all associated rtrees */
  SetElementBoundingBox (PCB->Data, Element, &PCB->Font);
  ClearFromPolygon (PCB->Data, PIN_TYPE, Element, Pin);
  DrawPin (Pin, 0);
  return (Pin);
}

/* ---------------------------------------------------------------------------
 * changes the size of a pad
 * returns TRUE if changed
 */
static void *
ChangePadSize (ElementTypePtr Element, PadTypePtr Pad)
{
  BDimension value = (Absolute) ? Absolute : Pad->Thickness + Delta;

  if (TEST_FLAG (LOCKFLAG, Pad))
    return (NULL);
  if (value <= MAX_PADSIZE && value >= MIN_PADSIZE && value != Pad->Thickness)
    {
      AddObjectToSizeUndoList (PAD_TYPE, Element, Pad, Pad);
      AddObjectToMaskSizeUndoList (PAD_TYPE, Element, Pad, Pad);
      RestoreToPolygon (PCB->Data, PAD_TYPE, Element, Pad);
      ErasePad (Pad);
      r_delete_entry (PCB->Data->pad_tree, &Pad->BoundingBox);
      Pad->Mask += value - Pad->Thickness;
      Pad->Thickness = value;
      /* SetElementBB updates all associated rtrees */
      SetElementBoundingBox (PCB->Data, Element, &PCB->Font);
      ClearFromPolygon (PCB->Data, PAD_TYPE, Element, Pad);
      DrawPad (Pad, 0);
      return (Pad);
    }
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the clearance size of a pad
 * returns TRUE if changed
 */
static void *
ChangePadClearSize (ElementTypePtr Element, PadTypePtr Pad)
{
  BDimension value = (Absolute) ? Absolute : Pad->Clearance + Delta;

  if (TEST_FLAG (LOCKFLAG, Pad))
    return (NULL);
  value = MIN (MAX_LINESIZE, MAX (value, PCB->Bloat * 2 + 2));
  if (value <= MAX_PADSIZE && value >= MIN_PADSIZE && value != Pad->Clearance)
    {
      AddObjectToClearSizeUndoList (PAD_TYPE, Element, Pad, Pad);
      RestoreToPolygon (PCB->Data, PAD_TYPE, Element, Pad);
      ErasePad (Pad);
      r_delete_entry (PCB->Data->pad_tree, &Pad->BoundingBox);
      Pad->Clearance = value;
      /* SetElementBB updates all associated rtrees */
      SetElementBoundingBox (PCB->Data, Element, &PCB->Font);
      ClearFromPolygon (PCB->Data, PAD_TYPE, Element, Pad);
      DrawPad (Pad, 0);
      return (Pad);
    }
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the drilling hole of all pins of an element
 * returns TRUE if changed
 */
static void *
ChangeElement2ndSize (ElementTypePtr Element)
{
  Boolean changed = False;
  BDimension value;

  if (TEST_FLAG (LOCKFLAG, Element))
    return (NULL);
  PIN_LOOP (Element);
  {
    value = (Absolute) ? Absolute : pin->DrillingHole + Delta;
    if (value <= MAX_PINORVIASIZE &&
	value >= MIN_PINORVIAHOLE && (TEST_FLAG (HOLEFLAG, pin) ||
				      value <=
				      pin->Thickness -
				      MIN_PINORVIACOPPER)
	&& value != pin->DrillingHole)
      {
	changed = True;
	AddObjectTo2ndSizeUndoList (PIN_TYPE, Element, pin, pin);
	ErasePin (pin);
	pin->DrillingHole = value;
	DrawPin (pin, 0);
	if (TEST_FLAG (HOLEFLAG, pin))
	  {
	    AddObjectToSizeUndoList (PIN_TYPE, Element, pin, pin);
	    pin->Thickness = value;
	  }
      }
  }
  END_LOOP;
  if (changed)
    return (Element);
  else
    return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the drilling hole of a pin
 * returns TRUE if changed
 */
static void *
ChangePin2ndSize (ElementTypePtr Element, PinTypePtr Pin)
{
  BDimension value = (Absolute) ? Absolute : Pin->DrillingHole + Delta;

  if (TEST_FLAG (LOCKFLAG, Pin))
    return (NULL);
  if (value <= MAX_PINORVIASIZE &&
      value >= MIN_PINORVIAHOLE && (TEST_FLAG (HOLEFLAG, Pin) ||
				    value <=
				    Pin->Thickness - MIN_PINORVIACOPPER)
      && value != Pin->DrillingHole)
    {
      AddObjectTo2ndSizeUndoList (PIN_TYPE, Element, Pin, Pin);
      ErasePin (Pin);
      Pin->DrillingHole = value;
      DrawPin (Pin, 0);
      if (TEST_FLAG (HOLEFLAG, Pin))
	{
	  AddObjectToSizeUndoList (PIN_TYPE, Element, Pin, Pin);
	  Pin->Thickness = value;
	}
      return (Pin);
    }
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the size of a line
 * returns TRUE if changed
 */
static void *
ChangeLineSize (LayerTypePtr Layer, LineTypePtr Line)
{
  BDimension value = (Absolute) ? Absolute : Line->Thickness + Delta;

  if (TEST_FLAG (LOCKFLAG, Line))
    return (NULL);
  if (value <= MAX_LINESIZE && value >= MIN_LINESIZE &&
      value != Line->Thickness)
    {
      AddObjectToSizeUndoList (LINE_TYPE, Layer, Line, Line);
      EraseLine (Line);
      r_delete_entry (Layer->line_tree, (BoxTypePtr) Line);
      RestoreToPolygon (PCB->Data, LINE_TYPE, Layer, Line);
      Line->Thickness = value;
      SetLineBoundingBox (Line);
      r_insert_entry (Layer->line_tree, (BoxTypePtr) Line, 0);
      ClearFromPolygon (PCB->Data, LINE_TYPE, Layer, Line);
      DrawLine (Layer, Line, 0);
      return (Line);
    }
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the clearance size of a line
 * returns TRUE if changed
 */
static void *
ChangeLineClearSize (LayerTypePtr Layer, LineTypePtr Line)
{
  BDimension value = (Absolute) ? Absolute : Line->Clearance + Delta;

  if (TEST_FLAG (LOCKFLAG, Line) || !TEST_FLAG (CLEARLINEFLAG, Line))
    return (NULL);
  value = MIN (MAX_LINESIZE, MAX (value, PCB->Bloat * 2 + 2));
  if (value != Line->Clearance)
    {
      AddObjectToClearSizeUndoList (LINE_TYPE, Layer, Line, Line);
      RestoreToPolygon (PCB->Data, LINE_TYPE, Layer, Line);
      EraseLine (Line);
      r_delete_entry (Layer->line_tree, (BoxTypePtr) Line);
      Line->Clearance = value;
      if (Line->Clearance == 0)
	{
	  CLEAR_FLAG (CLEARLINEFLAG, Line);
	  Line->Clearance = 1000;
	}
      SetLineBoundingBox (Line);
      r_insert_entry (Layer->line_tree, (BoxTypePtr) Line, 0);
      ClearFromPolygon (PCB->Data, LINE_TYPE, Layer, Line);
      DrawLine (Layer, Line, 0);
      return (Line);
    }
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the size of an arc
 * returns TRUE if changed
 */
static void *
ChangeArcSize (LayerTypePtr Layer, ArcTypePtr Arc)
{
  BDimension value = (Absolute) ? Absolute : Arc->Thickness + Delta;

  if (TEST_FLAG (LOCKFLAG, Arc))
    return (NULL);
  if (value <= MAX_LINESIZE && value >= MIN_LINESIZE &&
      value != Arc->Thickness)
    {
      AddObjectToSizeUndoList (ARC_TYPE, Layer, Arc, Arc);
      EraseArc (Arc);
      r_delete_entry (Layer->arc_tree, (BoxTypePtr) Arc);
      RestoreToPolygon (PCB->Data, ARC_TYPE, Layer, Arc);
      Arc->Thickness = value;
      SetArcBoundingBox (Arc);
      r_insert_entry (Layer->arc_tree, (BoxTypePtr) Arc, 0);
      ClearFromPolygon (PCB->Data, ARC_TYPE, Layer, Arc);
      DrawArc (Layer, Arc, 0);
      return (Arc);
    }
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the clearance size of an arc
 * returns TRUE if changed
 */
static void *
ChangeArcClearSize (LayerTypePtr Layer, ArcTypePtr Arc)
{
  BDimension value = (Absolute) ? Absolute : Arc->Clearance + Delta;

  if (TEST_FLAG (LOCKFLAG, Arc) || !TEST_FLAG (CLEARLINEFLAG, Arc))
    return (NULL);
  value = MIN (MAX_LINESIZE, MAX (value, PCB->Bloat * 2 + 2));
  if (value != Arc->Clearance)
    {
      AddObjectToClearSizeUndoList (ARC_TYPE, Layer, Arc, Arc);
      EraseArc (Arc);
      r_delete_entry (Layer->arc_tree, (BoxTypePtr) Arc);
      RestoreToPolygon (PCB->Data, ARC_TYPE, Layer, Arc);
      Arc->Clearance = value;
      if (Arc->Clearance == 0)
	{
	  CLEAR_FLAG (CLEARLINEFLAG, Arc);
	  Arc->Clearance = 1000;
	}
      SetArcBoundingBox (Arc);
      r_insert_entry (Layer->arc_tree, (BoxTypePtr) Arc, 0);
      ClearFromPolygon (PCB->Data, ARC_TYPE, Layer, Arc);
      DrawArc (Layer, Arc, 0);
      return (Arc);
    }
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the scaling factor of a text object
 * returns TRUE if changed
 */
static void *
ChangeTextSize (LayerTypePtr Layer, TextTypePtr Text)
{
  BDimension value = (Absolute) ? Absolute / 45 : Text->Scale + Delta / 45;

  if (TEST_FLAG (LOCKFLAG, Text))
    return (NULL);
  if (value <= MAX_TEXTSCALE && value >= MIN_TEXTSCALE &&
      value != Text->Scale)
    {
      AddObjectToSizeUndoList (TEXT_TYPE, Layer, Text, Text);
      EraseText (Text);
      r_delete_entry (Layer->text_tree, (BoxTypePtr) Text);
      Text->Scale = value;
      SetTextBoundingBox (&PCB->Font, Text);
      r_insert_entry (Layer->text_tree, (BoxTypePtr) Text, 0);
      DrawText (Layer, Text, 0);
      return (Text);
    }
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the scaling factor of an element's outline
 * returns TRUE if changed
 */
static void *
ChangeElementSize (ElementTypePtr Element)
{
  BDimension value;
  Boolean changed = False;

  if (TEST_FLAG (LOCKFLAG, Element))
    return (NULL);
  if (PCB->ElementOn)
    EraseElement (Element);
  ELEMENTLINE_LOOP (Element);
  {
    value = (Absolute) ? Absolute : line->Thickness + Delta;
    if (value <= MAX_LINESIZE && value >= MIN_LINESIZE &&
	value != line->Thickness)
      {
	AddObjectToSizeUndoList (ELEMENTLINE_TYPE, Element, line, line);
	line->Thickness = value;
	changed = True;
      }
  }
  END_LOOP;
  ARC_LOOP (Element);
  {
    value = (Absolute) ? Absolute : arc->Thickness + Delta;
    if (value <= MAX_LINESIZE && value >= MIN_LINESIZE &&
	value != arc->Thickness)
      {
	AddObjectToSizeUndoList (ELEMENTARC_TYPE, Element, arc, arc);
	arc->Thickness = value;
	changed = True;
      }
  }
  END_LOOP;
  if (PCB->ElementOn)
    {
      DrawElement (Element, 0);
    }
  if (changed)
    return (Element);
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the scaling factor of a elementname object
 * returns TRUE if changed
 */
static void *
ChangeElementNameSize (ElementTypePtr Element)
{
  BDimension value =
    (Absolute) ? Absolute / 45 : DESCRIPTION_TEXT (Element).Scale +
    Delta / 45;

  if (TEST_FLAG (LOCKFLAG, &Element->Name[0]))
    return (NULL);
  if (value <= MAX_TEXTSCALE && value >= MIN_TEXTSCALE)
    {
      EraseElementName (Element);
      ELEMENTTEXT_LOOP (Element);
      {
	AddObjectToSizeUndoList (ELEMENTNAME_TYPE, Element, text, text);
	r_delete_entry (PCB->Data->name_tree[n], (BoxType *) text);
	text->Scale = value;
	SetTextBoundingBox (&PCB->Font, text);
	r_insert_entry (PCB->Data->name_tree[n], (BoxType *) text, 0);
      }
      END_LOOP;
      DrawElementName (Element, 0);
      return (Element);
    }
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the name of a via
 */
static void *
ChangeViaName (PinTypePtr Via)
{
  char *old = Via->Name;

  if (TEST_FLAG (DISPLAYNAMEFLAG, Via))
    {
      ErasePinName (Via);
      Via->Name = NewName;
      DrawPinName (Via, 0);
    }
  else
    Via->Name = NewName;
  return (old);
}

/* ---------------------------------------------------------------------------
 * changes the name of a pin
 */
static void *
ChangePinName (ElementTypePtr Element, PinTypePtr Pin)
{
  char *old = Pin->Name;

  Element = Element;		/* get rid of 'unused...' warnings */
  if (TEST_FLAG (DISPLAYNAMEFLAG, Pin))
    {
      ErasePinName (Pin);
      Pin->Name = NewName;
      DrawPinName (Pin, 0);
    }
  else
    Pin->Name = NewName;
  return (old);
}

/* ---------------------------------------------------------------------------
 * changes the name of a pad
 */
static void *
ChangePadName (ElementTypePtr Element, PadTypePtr Pad)
{
  char *old = Pad->Name;

  Element = Element;		/* get rid of 'unused...' warnings */
  if (TEST_FLAG (DISPLAYNAMEFLAG, Pad))
    {
      ErasePadName (Pad);
      Pad->Name = NewName;
      DrawPadName (Pad, 0);
    }
  else
    Pad->Name = NewName;
  return (old);
}

/* ---------------------------------------------------------------------------
 * changes the name of a line
 */
static void *
ChangeLineName (LayerTypePtr Layer, LineTypePtr Line)
{
  char *old = Line->Number;

  Layer = Layer;
  Line->Number = NewName;
  return (old);
}

/* ---------------------------------------------------------------------------
 * changes the layout-name of an element
 */
static void *
ChangeElementName (ElementTypePtr Element)
{
  char *old = ELEMENT_NAME (PCB, Element);

  if (TEST_FLAG (LOCKFLAG, &Element->Name[0]))
    return (NULL);
  if (NAME_INDEX (PCB) == NAMEONPCB_INDEX)
    {
      if (TEST_FLAG (UNIQUENAMEFLAG, PCB) &&
	  UniqueElementName (PCB->Data, NewName) != NewName)
	{
	  Message (_("Error: The name \"%s\" is not unique!\n"), NewName);
	  return ((char *) -1);
	}
    }
  EraseElementName (Element);
  r_delete_entry (PCB->Data->name_tree[NAME_INDEX (PCB)],
		  (BoxType *) & ELEMENT_TEXT (PCB, Element));
  ELEMENT_NAME (PCB, Element) = NewName;
  SetTextBoundingBox (&PCB->Font, &ELEMENT_TEXT (PCB, Element));
  r_insert_entry (PCB->Data->name_tree[NAME_INDEX (PCB)],
		  (BoxType *) & ELEMENT_TEXT (PCB, Element), 0);
  DrawElementName (Element, 0);
  return (old);
}

/* ---------------------------------------------------------------------------
 * sets data of a text object and calculates bounding box
 * memory must have already been allocated
 * the one for the new string is allocated
 * returns True if the string has been changed
 */
static void *
ChangeTextName (LayerTypePtr Layer, TextTypePtr Text)
{
  char *old = Text->TextString;

  if (TEST_FLAG (LOCKFLAG, Text))
    return (NULL);
  EraseText (Text);
  Text->TextString = NewName;

  /* calculate size of the bounding box */
  SetTextBoundingBox (&PCB->Font, Text);
  DrawText (Layer, Text, 0);
  return (old);
}

/* ---------------------------------------------------------------------------
 * changes the name of a layout; memory has to be already allocated
 */
Boolean
ChangeLayoutName (char *Name)
{
  PCB->Name = Name;
  hid_action ("PCBChanged");
  return (True);
}

/* ---------------------------------------------------------------------------
 * changes the side of the board an element is on
 * returns TRUE if done
 */
Boolean
ChangeElementSide (ElementTypePtr Element, LocationType yoff)
{
  if (TEST_FLAG (LOCKFLAG, Element))
    return (False);
  EraseElement (Element);
  AddObjectToMirrorUndoList (ELEMENT_TYPE, Element, Element, Element, yoff);
  MirrorElementCoordinates (PCB->Data, Element, yoff);
  DrawElement (Element, 0);
  return (True);
}

/* ---------------------------------------------------------------------------
 * changes the name of a layer; memory has to be already allocated
 */
Boolean
ChangeLayerName (LayerTypePtr Layer, char *Name)
{
  CURRENT->Name = Name;
  hid_action ("LayersChanged");
  return (True);
}

/* ---------------------------------------------------------------------------
 * changes the clearance flag of a line
 */
static void *
ChangeLineJoin (LayerTypePtr Layer, LineTypePtr Line)
{
  if (TEST_FLAG (LOCKFLAG, Line))
    return (NULL);
  EraseLine (Line);
  AddObjectToFlagUndoList (LINE_TYPE, Layer, Line, Line);
  RestoreToPolygon (PCB->Data, LINE_TYPE, Layer, Line);
  TOGGLE_FLAG (CLEARLINEFLAG, Line);
  ClearFromPolygon (PCB->Data, LINE_TYPE, Layer, Line);
  DrawLine (Layer, Line, 0);
  return (Line);
}

/* ---------------------------------------------------------------------------
 * changes the clearance flag of a line
 */
static void *
SetLineJoin (LayerTypePtr Layer, LineTypePtr Line)
{
  if (TEST_FLAG (LOCKFLAG, Line))
    return (NULL);
  EraseLine (Line);
  RestoreToPolygon (PCB->Data, LINE_TYPE, Layer, Line);
  AddObjectToFlagUndoList (LINE_TYPE, Layer, Line, Line);
  SET_FLAG (CLEARLINEFLAG, Line);
  DrawLine (Layer, Line, 0);
  return (Line);
}

/* ---------------------------------------------------------------------------
 * changes the clearance flag of a line
 */
static void *
ClrLineJoin (LayerTypePtr Layer, LineTypePtr Line)
{
  if (TEST_FLAG (LOCKFLAG, Line))
    return (NULL);
  EraseLine (Line);
  AddObjectToFlagUndoList (LINE_TYPE, Layer, Line, Line);
  CLEAR_FLAG (CLEARLINEFLAG, Line);
  ClearFromPolygon (PCB->Data, LINE_TYPE, Layer, Line);
  DrawLine (Layer, Line, 0);
  return (Line);
}

/* ---------------------------------------------------------------------------
 * changes the clearance flag of an arc
 */
static void *
ChangeArcJoin (LayerTypePtr Layer, ArcTypePtr Arc)
{
  if (TEST_FLAG (LOCKFLAG, Arc))
    return (NULL);
  EraseArc (Arc);
  if (TEST_FLAG (CLEARLINEFLAG, Arc))
    RestoreToPolygon (PCB->Data, ARC_TYPE, Layer, Arc);
  AddObjectToFlagUndoList (ARC_TYPE, Layer, Arc, Arc);
  TOGGLE_FLAG (CLEARLINEFLAG, Arc);
  if (TEST_FLAG (CLEARLINEFLAG, Arc))
    ClearFromPolygon (PCB->Data, ARC_TYPE, Layer, Arc);
  DrawArc (Layer, Arc, 0);
  return (Arc);
}

/* ---------------------------------------------------------------------------
 * changes the clearance flag of an arc
 */
static void *
SetArcJoin (LayerTypePtr Layer, ArcTypePtr Arc)
{
  if (TEST_FLAG (LOCKFLAG, Arc))
    return (NULL);
  EraseArc (Arc);
  AddObjectToFlagUndoList (ARC_TYPE, Layer, Arc, Arc);
  SET_FLAG (CLEARLINEFLAG, Arc);
  ClearFromPolygon (PCB->Data, ARC_TYPE, Layer, Arc);
  DrawArc (Layer, Arc, 0);
  return (Arc);
}

/* ---------------------------------------------------------------------------
 * changes the clearance flag of an arc
 */
static void *
ClrArcJoin (LayerTypePtr Layer, ArcTypePtr Arc)
{
  if (TEST_FLAG (LOCKFLAG, Arc))
    return (NULL);
  EraseArc (Arc);
  RestoreToPolygon (PCB->Data, ARC_TYPE, Layer, Arc);
  AddObjectToFlagUndoList (ARC_TYPE, Layer, Arc, Arc);
  CLEAR_FLAG (CLEARLINEFLAG, Arc);
  DrawArc (Layer, Arc, 0);
  return (Arc);
}

/* ---------------------------------------------------------------------------
 * changes the square flag of all pins on an element
 */
static void *
ChangeElementSquare (ElementTypePtr Element)
{
  void *ans = NULL;

  if (TEST_FLAG (LOCKFLAG, Element))
    return (NULL);
  PIN_LOOP (Element);
  {
    ans = ChangePinSquare (Element, pin);
  }
  END_LOOP;
  PAD_LOOP (Element);
  {
    ans = ChangePadSquare (Element, pad);
  }
  END_LOOP;
  return (ans);
}

/* ---------------------------------------------------------------------------
 * sets the square flag of all pins on an element
 */
static void *
SetElementSquare (ElementTypePtr Element)
{
  void *ans = NULL;

  if (TEST_FLAG (LOCKFLAG, Element))
    return (NULL);
  PIN_LOOP (Element);
  {
    ans = SetPinSquare (Element, pin);
  }
  END_LOOP;
  PAD_LOOP (Element);
  {
    ans = SetPadSquare (Element, pad);
  }
  END_LOOP;
  return (ans);
}

/* ---------------------------------------------------------------------------
 * clears the square flag of all pins on an element
 */
static void *
ClrElementSquare (ElementTypePtr Element)
{
  void *ans = NULL;

  if (TEST_FLAG (LOCKFLAG, Element))
    return (NULL);
  PIN_LOOP (Element);
  {
    ans = ClrPinSquare (Element, pin);
  }
  END_LOOP;
  PAD_LOOP (Element);
  {
    ans = ClrPadSquare (Element, pad);
  }
  END_LOOP;
  return (ans);
}

/* ---------------------------------------------------------------------------
 * changes the octagon flags of all pins of an element
 */
static void *
ChangeElementOctagon (ElementTypePtr Element)
{
  void *result = NULL;

  if (TEST_FLAG (LOCKFLAG, Element))
    return (NULL);
  PIN_LOOP (Element);
  {
    ChangePinOctagon (Element, pin);
    result = Element;
  }
  END_LOOP;
  return (result);
}

/* ---------------------------------------------------------------------------
 * sets the octagon flags of all pins of an element
 */
static void *
SetElementOctagon (ElementTypePtr Element)
{
  void *result = NULL;

  if (TEST_FLAG (LOCKFLAG, Element))
    return (NULL);
  PIN_LOOP (Element);
  {
    SetPinOctagon (Element, pin);
    result = Element;
  }
  END_LOOP;
  return (result);
}

/* ---------------------------------------------------------------------------
 * clears the octagon flags of all pins of an element
 */
static void *
ClrElementOctagon (ElementTypePtr Element)
{
  void *result = NULL;

  if (TEST_FLAG (LOCKFLAG, Element))
    return (NULL);
  PIN_LOOP (Element);
  {
    ClrPinOctagon (Element, pin);
    result = Element;
  }
  END_LOOP;
  return (result);
}

/* ---------------------------------------------------------------------------
 * changes the square flag of a pad
 */
static void *
ChangePadSquare (ElementTypePtr Element, PadTypePtr Pad)
{
  if (TEST_FLAG (LOCKFLAG, Pad))
    return (NULL);
  ErasePad (Pad);
  RestoreToPolygon (PCB->Data, PAD_TYPE, Element, Pad);
  AddObjectToFlagUndoList (PAD_TYPE, Element, Pad, Pad);
  TOGGLE_FLAG (SQUAREFLAG, Pad);
  ClearFromPolygon (PCB->Data, PAD_TYPE, Element, Pad);
  DrawPad (Pad, 0);
  return (Pad);
}

/* ---------------------------------------------------------------------------
 * sets the square flag of a pad
 */
static void *
SetPadSquare (ElementTypePtr Element, PadTypePtr Pad)
{

  if (TEST_FLAG (LOCKFLAG, Pad) || TEST_FLAG (SQUAREFLAG, Pad))
    return (NULL);

  return (ChangePadSquare (Element, Pad));
}


/* ---------------------------------------------------------------------------
 * clears the square flag of a pad
 */
static void *
ClrPadSquare (ElementTypePtr Element, PadTypePtr Pad)
{

  if (TEST_FLAG (LOCKFLAG, Pad) || !TEST_FLAG (SQUAREFLAG, Pad))
    return (NULL);

  return (ChangePadSquare (Element, Pad));
}


/* ---------------------------------------------------------------------------
 * changes the square flag of a pin
 */
static void *
ChangePinSquare (ElementTypePtr Element, PinTypePtr Pin)
{
  if (TEST_FLAG (LOCKFLAG, Pin))
    return (NULL);
  ErasePin (Pin);
  AddObjectToFlagUndoList (PIN_TYPE, Element, Pin, Pin);
  RestoreToPolygon (PCB->Data, PIN_TYPE, Element, Pin);
  TOGGLE_FLAG (SQUAREFLAG, Pin);
  ClearFromPolygon (PCB->Data, PIN_TYPE, Element, Pin);
  DrawPin (Pin, 0);
  return (Pin);
}

/* ---------------------------------------------------------------------------
 * sets the square flag of a pin
 */
static void *
SetPinSquare (ElementTypePtr Element, PinTypePtr Pin)
{
  if (TEST_FLAG (LOCKFLAG, Pin) || TEST_FLAG (SQUAREFLAG, Pin))
    return (NULL);

  return (ChangePinSquare (Element, Pin));
}

/* ---------------------------------------------------------------------------
 * clears the square flag of a pin
 */
static void *
ClrPinSquare (ElementTypePtr Element, PinTypePtr Pin)
{
  if (TEST_FLAG (LOCKFLAG, Pin) || !TEST_FLAG (SQUAREFLAG, Pin))
    return (NULL);

  return (ChangePinSquare (Element, Pin));
}

/* ---------------------------------------------------------------------------
 * changes the octagon flag of a via 
 */
static void *
ChangeViaOctagon (PinTypePtr Via)
{
  if (TEST_FLAG (LOCKFLAG, Via))
    return (NULL);
  EraseVia (Via);
  RestoreToPolygon (PCB->Data, VIA_TYPE, Via, Via);
  AddObjectToFlagUndoList (VIA_TYPE, Via, Via, Via);
  TOGGLE_FLAG (OCTAGONFLAG, Via);
  ClearFromPolygon (PCB->Data, VIA_TYPE, Via, Via);
  DrawVia (Via, 0);
  return (Via);
}

/* ---------------------------------------------------------------------------
 * sets the octagon flag of a via 
 */
static void *
SetViaOctagon (PinTypePtr Via)
{
  if (TEST_FLAG (LOCKFLAG, Via) || TEST_FLAG (OCTAGONFLAG, Via))
    return (NULL);

  return (ChangeViaOctagon (Via));
}

/* ---------------------------------------------------------------------------
 * clears the octagon flag of a via 
 */
static void *
ClrViaOctagon (PinTypePtr Via)
{
  if (TEST_FLAG (LOCKFLAG, Via) || !TEST_FLAG (OCTAGONFLAG, Via))
    return (NULL);

  return (ChangeViaOctagon (Via));
}

/* ---------------------------------------------------------------------------
 * changes the octagon flag of a pin
 */
static void *
ChangePinOctagon (ElementTypePtr Element, PinTypePtr Pin)
{
  if (TEST_FLAG (LOCKFLAG, Pin))
    return (NULL);
  ErasePin (Pin);
  AddObjectToFlagUndoList (PIN_TYPE, Element, Pin, Pin);
  RestoreToPolygon (PCB->Data, PIN_TYPE, Element, Pin);
  TOGGLE_FLAG (OCTAGONFLAG, Pin);
  ClearFromPolygon (PCB->Data, PIN_TYPE, Element, Pin);
  DrawPin (Pin, 0);
  return (Pin);
}

/* ---------------------------------------------------------------------------
 * sets the octagon flag of a pin
 */
static void *
SetPinOctagon (ElementTypePtr Element, PinTypePtr Pin)
{
  if (TEST_FLAG (LOCKFLAG, Pin) || TEST_FLAG (OCTAGONFLAG, Pin))
    return (NULL);

  return (ChangePinOctagon (Element, Pin));
}

/* ---------------------------------------------------------------------------
 * clears the octagon flag of a pin
 */
static void *
ClrPinOctagon (ElementTypePtr Element, PinTypePtr Pin)
{
  if (TEST_FLAG (LOCKFLAG, Pin) || !TEST_FLAG (OCTAGONFLAG, Pin))
    return (NULL);

  return (ChangePinOctagon (Element, Pin));
}

/* ---------------------------------------------------------------------------
 * changes the hole flag of a via
 */
Boolean
ChangeHole (PinTypePtr Via)
{
  if (TEST_FLAG (LOCKFLAG, Via))
    return (False);
  EraseVia (Via);
  AddObjectToFlagUndoList (VIA_TYPE, Via, Via, Via);
  TOGGLE_FLAG (HOLEFLAG, Via);
  if (TEST_FLAG (HOLEFLAG, Via))
    {
      AddObjectToSizeUndoList (VIA_TYPE, Via, Via, Via);
      Via->Thickness = Via->Mask = Via->DrillingHole;
    }
  else
    {
      AddObjectTo2ndSizeUndoList (VIA_TYPE, Via, Via, Via);
      Via->DrillingHole = Via->Thickness - MIN_PINORVIACOPPER;
    }
  DrawVia (Via, 0);
  Draw ();
  return (True);
}

/* ---------------------------------------------------------------------------
 * changes the CLEARPOLY flag of a polygon
 */
static void *
ChangePolyClear (LayerTypePtr Layer, PolygonTypePtr Polygon)
{
  if (TEST_FLAG (LOCKFLAG, Polygon))
    return (NULL);
  AddObjectToFlagUndoList (POLYGON_TYPE, Layer, Polygon, Polygon);
  TOGGLE_FLAG (CLEARPOLYFLAG, Polygon);
  InitClip (PCB->Data, Layer, Polygon);
  DrawPolygon (Layer, Polygon, 0);
  return (Polygon);
}

/* ----------------------------------------------------------------------
 * changes the side of all selected and visible elements 
 * returns True if anything has changed
 */
Boolean
ChangeSelectedElementSide (void)
{
  Boolean change = False;

  /* setup identifiers */
  if (PCB->PinOn && PCB->ElementOn)
    ELEMENT_LOOP (PCB->Data);
  {
    if (TEST_FLAG (SELECTEDFLAG, element))
      {
	change |= ChangeElementSide (element, 0);
      }
  }
  END_LOOP;
  if (change)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (change);
}

/* ----------------------------------------------------------------------
 * changes the thermals on all selected and visible pins
 * and/or vias. Returns True if anything has changed
 */
Boolean
ChangeSelectedThermals (int types, int therm_style)
{
  Boolean change = False;

  Delta = therm_style;
  change = SelectedOperation (&ChangeThermalFunctions, False, types);
  if (change)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (change);
}

/* ----------------------------------------------------------------------
 * changes the size of all selected and visible object types 
 * returns True if anything has changed
 */
Boolean
ChangeSelectedSize (int types, LocationType Difference, Boolean fixIt)
{
  Boolean change = False;

  /* setup identifiers */
  Absolute = (fixIt) ? Difference : 0;
  Delta = Difference;

  change = SelectedOperation (&ChangeSizeFunctions, False, types);
  if (change)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (change);
}

/* ----------------------------------------------------------------------
 * changes the clearance size of all selected and visible objects
 * returns True if anything has changed
 */
Boolean
ChangeSelectedClearSize (int types, LocationType Difference, Boolean fixIt)
{
  Boolean change = False;

  /* setup identifiers */
  Absolute = (fixIt) ? Difference : 0;
  Delta = Difference;
  if (TEST_FLAG (SHOWMASKFLAG, PCB))
    change = SelectedOperation (&ChangeMaskSizeFunctions, False, types);
  else
    change = SelectedOperation (&ChangeClearSizeFunctions, False, types);
  if (change)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (change);
}

/* --------------------------------------------------------------------------
 * changes the 2nd size (drilling hole) of all selected and visible objects
 * returns True if anything has changed
 */
Boolean
ChangeSelected2ndSize (int types, LocationType Difference, Boolean fixIt)
{
  Boolean change = False;

  /* setup identifiers */
  Absolute = (fixIt) ? Difference : 0;
  Delta = Difference;
  change = SelectedOperation (&Change2ndSizeFunctions, False, types);
  if (change)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (change);
}

/* ----------------------------------------------------------------------
 * changes the clearance flag (join) of all selected and visible lines
 * and/or arcs. Returns True if anything has changed
 */
Boolean
ChangeSelectedJoin (int types)
{
  Boolean change = False;

  change = SelectedOperation (&ChangeJoinFunctions, False, types);
  if (change)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (change);
}

/* ----------------------------------------------------------------------
 * changes the clearance flag (join) of all selected and visible lines
 * and/or arcs. Returns True if anything has changed
 */
Boolean
SetSelectedJoin (int types)
{
  Boolean change = False;

  change = SelectedOperation (&SetJoinFunctions, False, types);
  if (change)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (change);
}

/* ----------------------------------------------------------------------
 * changes the clearance flag (join) of all selected and visible lines
 * and/or arcs. Returns True if anything has changed
 */
Boolean
ClrSelectedJoin (int types)
{
  Boolean change = False;

  change = SelectedOperation (&ClrJoinFunctions, False, types);
  if (change)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (change);
}

/* ----------------------------------------------------------------------
 * changes the square-flag of all selected and visible pins or pads
 * returns True if anything has changed
 */
Boolean
ChangeSelectedSquare (int types)
{
  Boolean change = False;

  change = SelectedOperation (&ChangeSquareFunctions, False, types);
  if (change)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (change);
}

/* ----------------------------------------------------------------------
 * sets the square-flag of all selected and visible pins or pads
 * returns True if anything has changed
 */
Boolean
SetSelectedSquare (int types)
{
  Boolean change = False;

  change = SelectedOperation (&SetSquareFunctions, False, types);
  if (change)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (change);
}

/* ----------------------------------------------------------------------
 * clears the square-flag of all selected and visible pins or pads
 * returns True if anything has changed
 */
Boolean
ClrSelectedSquare (int types)
{
  Boolean change = False;

  change = SelectedOperation (&ClrSquareFunctions, False, types);
  if (change)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (change);
}

/* ----------------------------------------------------------------------
 * changes the octagon-flag of all selected and visible pins and vias
 * returns True if anything has changed
 */
Boolean
ChangeSelectedOctagon (int types)
{
  Boolean change = False;

  change = SelectedOperation (&ChangeOctagonFunctions, False, types);
  if (change)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (change);
}

/* ----------------------------------------------------------------------
 * sets the octagon-flag of all selected and visible pins and vias
 * returns True if anything has changed
 */
Boolean
SetSelectedOctagon (int types)
{
  Boolean change = False;

  change = SelectedOperation (&SetOctagonFunctions, False, types);
  if (change)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (change);
}

/* ----------------------------------------------------------------------
 * clears the octagon-flag of all selected and visible pins and vias
 * returns True if anything has changed
 */
Boolean
ClrSelectedOctagon (int types)
{
  Boolean change = False;

  change = SelectedOperation (&ClrOctagonFunctions, False, types);
  if (change)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (change);
}

/* ----------------------------------------------------------------------
 * changes the hole-flag of all selected and visible vias 
 * returns True if anything has changed
 */
Boolean
ChangeSelectedHole (void)
{
  Boolean change = False;

  if (PCB->ViaOn)
    VIA_LOOP (PCB->Data);
  {
    if (TEST_FLAG (SELECTEDFLAG, via))
      change |= ChangeHole (via);
  }
  END_LOOP;
  if (change)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (change);
}


/* ---------------------------------------------------------------------------
 * changes the size of the passed object
 * Returns True if anything is changed
 */
Boolean
ChangeObjectSize (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
		  LocationType Difference, Boolean fixIt)
{
  Boolean change;

  /* setup identifier */
  Absolute = (fixIt) ? Difference : 0;
  Delta = Difference;
  change =
    (ObjectOperation (&ChangeSizeFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL);
  if (change)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (change);
}

/* ---------------------------------------------------------------------------
 * changes the clearance size of the passed object
 * Returns True if anything is changed
 */
Boolean
ChangeObjectClearSize (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
		       LocationType Difference, Boolean fixIt)
{
  Boolean change;

  /* setup identifier */
  Absolute = (fixIt) ? Difference : 0;
  Delta = Difference;
  if (TEST_FLAG (SHOWMASKFLAG, PCB))
    change =
      (ObjectOperation (&ChangeMaskSizeFunctions, Type, Ptr1, Ptr2, Ptr3) !=
       NULL);
  else
    change =
      (ObjectOperation (&ChangeClearSizeFunctions, Type, Ptr1, Ptr2, Ptr3) !=
       NULL);
  if (change)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (change);
}

/* ---------------------------------------------------------------------------
 * changes the thermal of the passed object
 * Returns True if anything is changed
 *
 * therm_type = 0 means no thermsl
 *            = 1 means 45 degree lines
 *            = 2 means horizontal and vertical lines
 *            = 3 means 4 lines
 *            = 4 means solid connection to plane
 */
Boolean
ChangeObjectThermal (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
		     int therm_type)
{
  Boolean change;

  Delta = Absolute = therm_type;
  change =
    (ObjectOperation (&ChangeThermalFunctions, Type, Ptr1, Ptr2, Ptr3) !=
     NULL);
  if (change)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (change);
}

/* ---------------------------------------------------------------------------
 * changes the 2nd size of the passed object
 * Returns True if anything is changed
 */
Boolean
ChangeObject2ndSize (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
		     LocationType Difference, Boolean fixIt, Boolean incundo)
{
  Boolean change;

  /* setup identifier */
  Absolute = (fixIt) ? Difference : 0;
  Delta = Difference;
  change =
    (ObjectOperation (&Change2ndSizeFunctions, Type, Ptr1, Ptr2, Ptr3) !=
     NULL);
  if (change)
    {
      Draw ();
      if (incundo)
	IncrementUndoSerialNumber ();
    }
  return (change);
}

/* ---------------------------------------------------------------------------
 * changes the mask size of the passed object
 * Returns True if anything is changed
 */
Boolean
ChangeObjectMaskSize (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
		      LocationType Difference, Boolean fixIt)
{
  Boolean change;

  /* setup identifier */
  Absolute = (fixIt) ? Difference : 0;
  Delta = Difference;
  change =
    (ObjectOperation (&ChangeMaskSizeFunctions, Type, Ptr1, Ptr2, Ptr3) !=
     NULL);
  if (change)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (change);
}

/* ---------------------------------------------------------------------------
 * changes the name of the passed object
 * returns the old name
 *
 * The allocated memory isn't freed because the old string is used
 * by the undo module.
 */
void *
ChangeObjectName (int Type, void *Ptr1, void *Ptr2, void *Ptr3, char *Name)
{
  void *result;
  /* setup identifier */
  NewName = Name;
  if ((result =
       ObjectOperation (&ChangeNameFunctions, Type, Ptr1, Ptr2, Ptr3)));
  Draw ();
  return (result);
}

/* ---------------------------------------------------------------------------
 * changes the clearance-flag of the passed object
 * Returns True if anything is changed
 */
Boolean
ChangeObjectJoin (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  if (ObjectOperation (&ChangeJoinFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL)
    {
      Draw ();
      IncrementUndoSerialNumber ();
      return (True);
    }
  return (False);
}

/* ---------------------------------------------------------------------------
 * sets the clearance-flag of the passed object
 * Returns True if anything is changed
 */
Boolean
SetObjectJoin (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  if (ObjectOperation (&SetJoinFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL)
    {
      Draw ();
      IncrementUndoSerialNumber ();
      return (True);
    }
  return (False);
}

/* ---------------------------------------------------------------------------
 * clears the clearance-flag of the passed object
 * Returns True if anything is changed
 */
Boolean
ClrObjectJoin (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  if (ObjectOperation (&ClrJoinFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL)
    {
      Draw ();
      IncrementUndoSerialNumber ();
      return (True);
    }
  return (False);
}

/* ---------------------------------------------------------------------------
 * changes the square-flag of the passed object
 * Returns True if anything is changed
 */
Boolean
ChangeObjectSquare (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  if (ObjectOperation (&ChangeSquareFunctions, Type, Ptr1, Ptr2, Ptr3) !=
      NULL)
    {
      Draw ();
      IncrementUndoSerialNumber ();
      return (True);
    }
  return (False);
}

/* ---------------------------------------------------------------------------
 * sets the square-flag of the passed object
 * Returns True if anything is changed
 */
Boolean
SetObjectSquare (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  if (ObjectOperation (&SetSquareFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL)
    {
      Draw ();
      IncrementUndoSerialNumber ();
      return (True);
    }
  return (False);
}

/* ---------------------------------------------------------------------------
 * clears the square-flag of the passed object
 * Returns True if anything is changed
 */
Boolean
ClrObjectSquare (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  if (ObjectOperation (&ClrSquareFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL)
    {
      Draw ();
      IncrementUndoSerialNumber ();
      return (True);
    }
  return (False);
}

/* ---------------------------------------------------------------------------
 * changes the octagon-flag of the passed object
 * Returns True if anything is changed
 */
Boolean
ChangeObjectOctagon (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  if (ObjectOperation (&ChangeOctagonFunctions, Type, Ptr1, Ptr2, Ptr3) !=
      NULL)
    {
      Draw ();
      IncrementUndoSerialNumber ();
      return (True);
    }
  return (False);
}

/* ---------------------------------------------------------------------------
 * sets the octagon-flag of the passed object
 * Returns True if anything is changed
 */
Boolean
SetObjectOctagon (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  if (ObjectOperation (&SetOctagonFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL)
    {
      Draw ();
      IncrementUndoSerialNumber ();
      return (True);
    }
  return (False);
}

/* ---------------------------------------------------------------------------
 * clears the octagon-flag of the passed object
 * Returns True if anything is changed
 */
Boolean
ClrObjectOctagon (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  if (ObjectOperation (&ClrOctagonFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL)
    {
      Draw ();
      IncrementUndoSerialNumber ();
      return (True);
    }
  return (False);
}

/* ---------------------------------------------------------------------------
 * queries the user for a new object name and changes it
 *
 * The allocated memory isn't freed because the old string is used
 * by the undo module.
 */
void *
QueryInputAndChangeObjectName (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  char *name = NULL;
  char msg[513];

  /* if passed an element name, make it an element reference instead */
  if (Type == ELEMENTNAME_TYPE)
    {
      Type = ELEMENT_TYPE;
      Ptr2 = Ptr1;
      Ptr3 = Ptr1;
    }
  switch (Type)
    {
    case LINE_TYPE:
      name = gui->prompt_for (_("Linename:"),
			      EMPTY (((LineTypePtr) Ptr2)->Number));
      break;

    case VIA_TYPE:
      name = gui->prompt_for (_("Vianame:"),
			      EMPTY (((PinTypePtr) Ptr2)->Name));
      break;

    case PIN_TYPE:
      sprintf (msg, _("%s Pin Name:"), EMPTY (((PinTypePtr) Ptr2)->Number));
      name = gui->prompt_for (msg, EMPTY (((PinTypePtr) Ptr2)->Name));
      break;

    case PAD_TYPE:
      sprintf (msg, _("%s Pad Name:"), EMPTY (((PadTypePtr) Ptr2)->Number));
      name = gui->prompt_for (msg, EMPTY (((PadTypePtr) Ptr2)->Name));
      break;

    case TEXT_TYPE:
      name = gui->prompt_for (_("Enter text:"),
			      EMPTY (((TextTypePtr) Ptr2)->TextString));
      break;

    case ELEMENT_TYPE:
      name = gui->prompt_for (_("Elementname:"),
			      EMPTY (ELEMENT_NAME
				     (PCB, (ElementTypePtr) Ptr2)));
      break;
    }
  if (name)
    {
      /* XXX Memory leak!! */
      char *old = ChangeObjectName (Type, Ptr1, Ptr2, Ptr3, name);
      if (old != (char *) -1)
	{
	  AddObjectToChangeNameUndoList (Type, Ptr1, Ptr2, Ptr3, old);
	  IncrementUndoSerialNumber ();
	}
      Draw ();
      return (Ptr3);
    }
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the maximum size of a layout
 * adjusts the scrollbars
 * releases the saved pixmap if necessary
 * and adjusts the cursor confinement box
 */
void
ChangePCBSize (BDimension Width, BDimension Height)
{
  PCB->MaxWidth = Width;
  PCB->MaxHeight = Height;

  /* crosshair range is different if pastebuffer-mode
   * is enabled
   */
  if (Settings.Mode == PASTEBUFFER_MODE)
    SetCrosshairRange (PASTEBUFFER->X - PASTEBUFFER->BoundingBox.X1,
		       PASTEBUFFER->Y - PASTEBUFFER->BoundingBox.Y1,
		       MAX (0,
			    Width - (PASTEBUFFER->BoundingBox.X2 -
				     PASTEBUFFER->X)), MAX (0,
							    Height -
							    (PASTEBUFFER->
							     BoundingBox.Y2 -
							     PASTEBUFFER->
							     Y)));
  else
    SetCrosshairRange (0, 0, (LocationType) Width, (LocationType) Height);
  hid_action ("PCBChanged");
#if 0
  UpdateAll ();
#endif
}

/* ---------------------------------------------------------------------------
 * changes the mask size of a pad
 * returns TRUE if changed
 */
static void *
ChangePadMaskSize (ElementTypePtr Element, PadTypePtr Pad)
{
  BDimension value = (Absolute) ? Absolute : Pad->Mask + Delta;

  value = MAX (value, 0);
  if (value == Pad->Mask && Absolute == 0)
    value = Pad->Thickness;
  if (value != Pad->Mask)
    {
      AddObjectToMaskSizeUndoList (PAD_TYPE, Element, Pad, Pad);
      ErasePad (Pad);
      r_delete_entry (PCB->Data->pad_tree, &Pad->BoundingBox);
      Pad->Mask = value;
      SetElementBoundingBox (PCB->Data, Element, &PCB->Font);
      DrawPad (Pad, 0);
      return (Pad);
    }
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the mask size of a pin
 * returns TRUE if changed
 */
static void *
ChangePinMaskSize (ElementTypePtr Element, PinTypePtr Pin)
{
  BDimension value = (Absolute) ? Absolute : Pin->Mask + Delta;

  value = MAX (value, 0);
  if (value == Pin->Mask && Absolute == 0)
    value = Pin->Thickness;
  if (value != Pin->Mask)
    {
      AddObjectToMaskSizeUndoList (PIN_TYPE, Element, Pin, Pin);
      ErasePin (Pin);
      r_delete_entry (PCB->Data->pin_tree, &Pin->BoundingBox);
      Pin->Mask = value;
      SetElementBoundingBox (PCB->Data, Element, &PCB->Font);
      DrawPin (Pin, 0);
      return (Pin);
    }
  return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the mask size of a via
 * returns TRUE if changed
 */
static void *
ChangeViaMaskSize (PinTypePtr Via)
{
  BDimension value;

  value = (Absolute) ? Absolute : Via->Mask + Delta;
  value = MAX (value, 0);
  if (value != Via->Mask)
    {
      AddObjectToMaskSizeUndoList (VIA_TYPE, Via, Via, Via);
      EraseVia (Via);
      r_delete_entry (PCB->Data->via_tree, &Via->BoundingBox);
      Via->Mask = value;
      SetPinBoundingBox (Via);
      r_insert_entry (PCB->Data->via_tree, &Via->BoundingBox, 0);
      DrawVia (Via, 0);
      return (Via);
    }
  return (NULL);
}
