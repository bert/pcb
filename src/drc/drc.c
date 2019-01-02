/*!
 * \file src/drc.c
 *
 * \brief DRC related structures and functions
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2018 Charles Parker <parker.charles@gmail.com>
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
 */

#include <stdio.h>

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "global.h" /* Coord */
#include "drc.h"
#include "drc_violation.h"
#include "drc_object.h"

#include "data.h" /* Settings and PCB structures */
#include "error.h" /* Message */
#include "find.h" /* Connection lookup functions */
#include "misc.h" /* SaveStackAndVisibility */
#include "object_list.h"
#include "pcb-printf.h" /* Units */
#include "polygon.h" /* PlowsPolygon */
#include "undo.h" /* Lock/Unlock Undo*/

object_list * drc_violation_list = 0;

/* ----------------------------------------------------------------------- *
 * DRC Functions
 * ----------------------------------------------------------------------- */

static DRCObject thing1, thing2;

bool
SetThing (int n, int type, void *ptr1, void *ptr2, void *ptr3)
{
  if (n == 1) 
  {
    thing1.ptr1 = ptr1;
    thing1.ptr2 = ptr2;
    thing1.ptr3 = ptr3;
    thing1.type = type;
    thing1.id = ((AnyObjectType *)ptr2)->ID;
  } 
  else 
  {
    thing2.ptr1 = ptr1;
    thing2.ptr2 = ptr2;
    thing2.ptr3 = ptr3;
    thing2.type = type;
    thing2.id = ((AnyObjectType *)ptr2)->ID;
  }
  return true;
}

static Cardinal drcerr_count;   /*!< Count of drc errors */

static void
append_drc_violation (DrcViolationType *violation)
{
  object_list_append(drc_violation_list, violation);
  if (gui->drc_gui != NULL)
  {
    gui->drc_gui->append_drc_violation (violation);
  }
}

/*!
 * \brief Locate the coordinatates of offending item (thing).
 */
static void
LocateError (Coord *x, Coord *y)
{
  switch (thing1.type)
  {
    case LINE_TYPE:
    {
      LineType *line = (LineType *) thing1.ptr3;
      *x = (line->Point1.X + line->Point2.X) / 2;
      *y = (line->Point1.Y + line->Point2.Y) / 2;
      break;
    }
    case ARC_TYPE:
    {
      ArcType *arc = (ArcType *) thing1.ptr3;
      *x = arc->X;
      *y = arc->Y;
      break;
    }
    case POLYGON_TYPE:
    {
      PolygonType *polygon = (PolygonType *) thing1.ptr3;
      *x =
      (polygon->Clipped->contours->xmin +
       polygon->Clipped->contours->xmax) / 2;
      *y =
      (polygon->Clipped->contours->ymin +
       polygon->Clipped->contours->ymax) / 2;
      break;
    }
    case PIN_TYPE:
    case VIA_TYPE:
    {
      PinType *pin = (PinType *) thing1.ptr3;
      *x = pin->X;
      *y = pin->Y;
      break;
    }
    case PAD_TYPE:
    {
      PadType *pad = (PadType *) thing1.ptr3;
      *x = (pad->Point1.X + pad->Point2.X) / 2;
      *y = (pad->Point1.Y + pad->Point2.Y) / 2;
      break;
    }
    case ELEMENT_TYPE:
    {
      ElementType *element = (ElementType *) thing1.ptr3;
      *x = element->MarkX;
      *y = element->MarkY;
      break;
    }
    default:
      return;
  }
}

struct drc_info
{
  int flag;
};

/*!
 * \brief Check for DRC violations on a single net starting from the pad
 * or pin.
 *
 * Sees if the connectivity changes when everything is bloated, or
 * shrunk. The general algorithm is documented in the top comments, but,
 * briefly...
 *
 * Start at a pin, and build a list of all the objects that touch it, and
 * the objects that touch those, and the objects that touch those, etc. When
 * the list is built, certain flags are also set (it's the flags that are
 * important, not the list of objects). Then make everything slightly larger
 * (increase the value of Bloat) and check again. If any object encountered
 * the second time doesn't have the SELECTEDFLAG set, then it's a new object
 * that is violating the DRC rule.
 *
 * Note: The gtk and lesstif HIDs use this a little differently. The gtk hid
 * builds a list of all the violations and presents it to the user. The
 * lesstif HID goes through it one violation at a time and throws a dialog
 * box for each violation. This allows the user the opportunity to abort the
 * DRC check at any point. So, we need to behave well in either case.
 *
 * This function is entered exclusively from DRCAll. When we enter this
 * function, DRCAll has already set User = false in order to not add lots of
 * unnecessary changes to the undo system. To make anything undoable, we have
 * to unlock it ourselves.
 *
 */
static bool
DRCFind (int What, void *ptr1, void *ptr2, void *ptr3)
{
  Coord x, y;
  DrcViolationType *violation;
  int flag;
  object_list * vobjs = object_list_new(2, sizeof(DRCObject));
  
  if (PCB->Shrink != 0)
  {
    /* Set the DRC and SELECTED flags on all objects that overlap with the
     * passed object after shrinking them.
     *
     * The last parameter sets the global "drc" variable in find.c
     * (through the call to DoIt). It's set to false here because we want
     * to build a list of all the connections.
     *
     * Note that we do the shrunk condition first because it will presumably
     * have fewer objects than the nominal object list.
     */
    start_do_it_and_dump (What, ptr1, ptr2, ptr3, DRCFLAG | SELECTEDFLAG, false, -PCB->Shrink, false);
    /* ok now the shrunk net has the SELECTEDFLAG set */
    
    /* Now build the list without shrinking objects, and set the FOUND
     * flag in the process. If a new object is found, the connectivity has
     * changed, indicating that the minimum overlap rule is violated for
     * something connected to the original object.
     *
     * Note that an object is considered "new" if it doesn't have the
     * SELECTEDFLAG set. This behavior is hard coded into the algorithm in
     * add_object_to_list:find.c. This means that the first search must
     * always set the SELECTEDFLAG.
     *
     * TODO: This means that we will only find one violation of this
     * type for each seed object.
     */
    ListStart (What, ptr1, ptr2, ptr3, FOUNDFLAG);
    /* The last parameter to DoIt sets the global drc flag. This causes the
     * search to abort if we find anything not already found */
    if (DoIt (FOUNDFLAG, 0, true, false, true))
    {
      drcerr_count++;
      LocateError (&x, &y);
      object_list_clear(vobjs);
      object_list_append(vobjs, &thing1);
      object_list_append(vobjs, &thing2);
      violation = pcb_drc_violation_new (
        _("Potential for broken trace"),
        _("Insufficient overlap between objects can lead to broken "
          "tracks\ndue to registration errors with old wheel style "
          "photo-plotters."),
        x, y,
        0,     /* ANGLE OF ERROR UNKNOWN */
        FALSE, /* MEASUREMENT OF ERROR UNKNOWN */
        0,     /* MAGNITUDE OF ERROR UNKNOWN */
        PCB->Shrink,
        vobjs);
      append_drc_violation (violation);
      pcb_drc_violation_free (violation);
    }
    DumpList ();
  }
  
  /* Now check the bloated condition.
   *
   * TODO: Why isn't this wrapped in if (PCB->Bloat > 0) ?
   */
  
  /* Reset all of the flags */
  ClearFlagOnAllObjects (FOUNDFLAG | SELECTEDFLAG, false);
  /* Set the DRC and SELECTED flags on all objects that overlap with the
   * passed object. Here we do the nominal case first, because it will
   * presumably have fewer objects than the bloated case.
   *
   * See above for additional notes.
   */
  
  /* Set the selected flag on anything connected to the pin */
  start_do_it_and_dump (What, ptr1, ptr2, ptr3, SELECTEDFLAG, false, 0, false);
  /* Now bloat everything, and find things are connected now that weren't
   * before */
  flag = FOUNDFLAG;
  ListStart (What, ptr1, ptr2, ptr3, flag);
  /* Why is this one a "while" when it's an "if" above? */
  while (DoIt (flag, PCB->Bloat, true, false, true))
  {
    DumpList ();
    drcerr_count++;
    LocateError (&x, &y);
    object_list_clear(vobjs);
    object_list_append(vobjs, &thing1);
    object_list_append(vobjs, &thing2);
    violation = pcb_drc_violation_new (
      _("Copper areas too close"),
      _("Circuits that are too close may bridge during imaging, etching,"
        "\nplating, or soldering processes resulting in a direct short."),
      x, y,
      0,     /* ANGLE OF ERROR UNKNOWN */
      FALSE, /* MEASUREMENT OF ERROR UNKNOWN */
      0,     /* MAGNITUDE OF ERROR UNKNOWN */
      PCB->Bloat,
      vobjs);
    append_drc_violation (violation);
    pcb_drc_violation_free (violation);
    /* highlight the rest of the encroaching net so it's not reported again */
    flag = FOUNDFLAG | SELECTEDFLAG;
    start_do_it_and_dump (thing2.type, thing2.ptr1, thing2.ptr2, thing2.ptr3, flag, true, 0, false);
    ListStart (What, ptr1, ptr2, ptr3, flag);
  }
  DumpList ();
  ClearFlagOnAllObjects (FOUNDFLAG | SELECTEDFLAG, false);
  object_list_delete(vobjs);
  return (false);
}

/* Create a new object not connected violation */
static void
new_polygon_not_connected_violation ( LayerType *l, PolygonType *poly )
{
  DrcViolationType * violation;
  Coord x, y;
  object_list * vobjs = object_list_new(2, sizeof(DRCObject));
  const char * fmstr = "Joined %s not connected to polygon\n";
  char message[128];

  LocateError (&x, &y);

  drcerr_count++;
  object_list_clear(vobjs);
  object_list_append(vobjs, &thing1);
  object_list_append(vobjs, &thing2);
  
  switch (thing1.type)
  {
  case LINE_TYPE:
    sprintf(message, fmstr, "line");
    break;
  case ARC_TYPE:
    sprintf(message, fmstr, "arc");
    break;
  case PIN_TYPE:
    sprintf(message, fmstr, "pin");
    break;
  case PAD_TYPE:
    sprintf(message, fmstr, "pad");
    break;
  case VIA_TYPE:
    sprintf(message, fmstr, "via");
    break;
  default:
    Message ("Warning: Unknown object type in poly not connected violation!\n");
  }

  violation = pcb_drc_violation_new (message,
      "An object is flagged such that it should connect to the polygon, but\n"
      "does not make electrical contact. If it is not supposed to connect to\n"
      "the polygon, change the clearline flag and rerun the DRC as this can\n"
      "cause violations to be missed.",
      x, y,
      0,     /* ANGLE OF ERROR UNKNOWN */
      FALSE, /* MEASUREMENT OF ERROR UNKNOWN */
      0,     /* MAGNITUDE OF ERROR UNKNOWN */
      0,
      vobjs);
  append_drc_violation (violation);
  pcb_drc_violation_free (violation);
  
  object_list_delete(vobjs);

}

/*
 * Create a new polygon clearance violation.
 *
 * I really don't like that this almost completely duplicates the previous
 * function...
 */
static void
new_polygon_clearance_violation ( LayerType *l, PolygonType *poly )
{
  DrcViolationType * violation;
  Coord x, y, cl;
  object_list * vobjs = object_list_new(2, sizeof(DRCObject));
  const char * fmstr = "%s with insufficient clearance inside polygon\n";
  char message[128];

  LocateError (&x, &y);

  drcerr_count++;
  object_list_clear(vobjs);
  object_list_append(vobjs, &thing1);
  object_list_append(vobjs, &thing2);


  switch (thing1.type)
  {
  case LINE_TYPE:
    sprintf(message, fmstr, "Line");
    cl = ((LineType*) thing1.ptr2)->Clearance;
    break;
  case ARC_TYPE:
    sprintf(message, fmstr, "Arc");
    cl = ((ArcType*) thing1.ptr2)->Clearance;
    break;
  case PIN_TYPE:
    sprintf(message, fmstr, "Pin");
    cl = ((PinType*) thing1.ptr2)->Clearance;
    break;
  case PAD_TYPE:
    sprintf(message, fmstr, "Pad");
    cl = ((PadType*) thing1.ptr2)->Clearance;
    break;
  case VIA_TYPE:
    sprintf(message, fmstr, "Via");
    cl = ((PinType*) thing1.ptr2)->Clearance;
    break;
  default:
    Message ("Warning: Unknown object type in poly clearance violation!\n");
    return;
  }

  violation = pcb_drc_violation_new (message,
    _("Circuits that are too close may bridge during imaging, etching,\n"
      "plating, or soldering processes resulting in a direct short."),
      x, y,
      0,     /* ANGLE OF ERROR UNKNOWN */
      true, /* MEASUREMENT OF ERROR UNKNOWN */
      cl/2.,     /* MAGNITUDE OF ERROR UNKNOWN */
      PCB->Bloat,
      vobjs);
  append_drc_violation (violation);
  pcb_drc_violation_free (violation);
  
  object_list_delete(vobjs);

}

static void
expand_obj_bbox (DRCObject *obj, Coord bloat)
{
  ((AnyObjectType*)obj->ptr2)->BoundingBox.X1 -= bloat/2;
  ((AnyObjectType*)obj->ptr2)->BoundingBox.X2 += bloat/2;
  ((AnyObjectType*)obj->ptr2)->BoundingBox.Y1 -= bloat/2;
  ((AnyObjectType*)obj->ptr2)->BoundingBox.Y2 += bloat/2;
}

/* Bloat the object */
static void
bloat_obj (DRCObject * obj, Coord bloat)
{
  switch (obj->type)
  {
  case LINE_TYPE:
    ((LineType *) obj->ptr2)->Thickness += bloat;
    SetLineBoundingBox(((LineType *) obj->ptr2));
    break;
  case ARC_TYPE:
    ((ArcType *) obj->ptr2)->Thickness += bloat;
    SetArcBoundingBox(((ArcType *) obj->ptr2));
    break;
  case PIN_TYPE:
  case VIA_TYPE:
    ((PinType *) obj->ptr2)->Thickness += bloat;
    SetPinBoundingBox(((PinType *) obj->ptr2));
    break;
  case PAD_TYPE:
    ((PadType *) obj->ptr2)->Thickness += bloat;
    SetPadBoundingBox(((PadType *) obj->ptr2));
    break;
  default:
    /* Type without clearance */
    break;
  }
}

/* Extract the clearance value from the object */
static Coord
obj_clearance (DRCObject * obj)
{
  switch (obj->type)
  {
  case LINE_TYPE:
    return ((LineType *) obj->ptr2)->Clearance;
    break;
  case ARC_TYPE:
    return ((ArcType *) obj->ptr2)->Clearance;
    break;
  case PIN_TYPE:
  case VIA_TYPE:
    return ((PinType *) obj->ptr2)->Clearance;
    break;
  case PAD_TYPE:
    return ((PadType *) obj->ptr2)->Clearance;
    break;
  default:
    /* Type without clearance */
    break;
  }
  return 0;
}

/* Call the appropriate routine to determine if an object is in a polygon */
static Coord
is_obj_in_poly (DRCObject * obj, PolygonType * poly, Cardinal layer)
{
  LineType *line = (LineType *) obj->ptr2;
  ArcType *arc = (ArcType *) obj->ptr2;
  PinType *pin = (PinType *) obj->ptr2;
  PadType *pad = (PadType *) obj->ptr2;

  switch (obj->type)
  {
  case LINE_TYPE:
    return IsLineInPolygon(line, poly);
  case ARC_TYPE:
    return IsArcInPolygon(arc, poly);
  case PIN_TYPE:
  case VIA_TYPE:
    return ViaIsOnLayerGroup (pin, GetLayerGroupNumberByNumber (layer)) 
             && !TEST_FLAG (HOLEFLAG, pin) 
             && IsPinInPolygon (pin,poly);

  case PAD_TYPE:
    return IsPadInPolygon(pad, poly);
  default:
    /* Type without clearance */
    break;
  }
  return 0;
}

/*!
 * \brief DRC clearance callback.
 *
 * Note: An object's "clearance" is the amount that should be added to the
 * objects width to get the width of the polygon cutout. This is two sided,
 * so, clearance = 2 * Bloat.
 */
static int
drc_callback (DataType *data, LayerType *layer, PolygonType *polygon,
              int type, void *ptr1, void *ptr2, void *userdata)
{
  int clearflag;
  Coord clearance = obj_clearance(&thing1);

  LineType *line = (LineType *) ptr2;
  
  /* Thing 1 is the object on which PlowsPolygon was called */
  SetThing (2, type, layer, polygon, polygon);
  
  /* If we're here, we know that the polygon and object have overlapping
   * bounding boxes. 
   */
  switch (type)
  {
  case LINE_TYPE:
  case ARC_TYPE:
    /* Only lines and arcs use the clearline flag */
    clearflag = TEST_FLAG (CLEARLINEFLAG, line);
    if (clearflag && (clearance < 2 * PCB->Bloat))
    {         
      /* The line should clear the polygon, but the clearance is
       * insufficient. 
       *
       * This doesn't necessarily mean that there are two pieces of copper 
       * with insufficient space between them. It's possible that one 
       * object is completely within the * clearance of another object, 
       * for example.
       *
       * In order to decide if there really is an error, we need a more
       * detailed test. So, we bloat the line by twice the minimum
       * clearance and see if the bloated object touches any other bit of
       * polygon.
       *
       * We have to turn off the CLEARLINEFLAG for the object to be tested
       * for intersection, however, we don't have to recompute the polygon
       * contours. 
       * */

      CLEAR_FLAG (CLEARLINEFLAG, line);
      bloat_obj (&thing1, 2*PCB->Bloat);

      /* True if the bloated object touches the polygon, after taking clearances
       * into account... note that IsXInPolygon adds another bloat, but
       * that one should be zeroed out.
       */
       if (is_obj_in_poly(&thing1, polygon, GetLayerNumber(PCB->Data, layer)))
        /* The bloated line touched the polygon, so there's a violation. */
        new_polygon_clearance_violation (layer, polygon);
     
      /* Restore the state of the object */
      bloat_obj (&thing1, -2*PCB->Bloat);
      SET_FLAG (CLEARLINEFLAG, line);  
    
    }
    else if (clearflag == 0)
    {
      /* If the object is supposed to join the polygon, make sure it's
       * connected electrically.
       */
      ClearFlagOnAllObjects (DRCFLAG, false);
      start_do_it_and_dump (thing1.type, ptr1, ptr2, ptr2, DRCFLAG, 
                            false, 0, false);

      /* Now everything that touches the line should have the DRCFLAG set. */
      if (!TEST_FLAG (DRCFLAG, polygon))
        new_polygon_not_connected_violation (layer, polygon);

      /* Pretend we were never here. */
      ClearFlagOnAllObjects (DRCFLAG, false);
    }

    break;

  case PAD_TYPE:
  case PIN_TYPE:
  case VIA_TYPE:
    if (clearance < 2 * PCB->Bloat)
    {
      /* The clearance is too small, but it could be cleared by other
       * objects. 
       * */
      bloat_obj(&thing1, 2*PCB->Bloat);
      if (is_obj_in_poly(&thing1, polygon, GetLayerNumber(PCB->Data, layer)))
        /* The bloated line touched the polygon, so there's a violation. */
        new_polygon_clearance_violation (layer, polygon);
      bloat_obj (&thing1, -2*PCB->Bloat);
    }
    break;
  default:
    Message ("hace: Bad Plow object in callback\n");
  }

  return 0;

}

/*!
 * \brief Check for DRC violations.
 *
 * See if the connectivity changes when everything is bloated, or shrunk.
 */
int
DRCAll (void)
{
  Coord x, y;
  /* violating object list */
  object_list * vobjs = object_list_new(2, sizeof(DRCObject));
  DrcViolationType *violation;
  int undo_flags = 0;
  int tmpcnt;
  int nopastecnt = 0;
  struct drc_info info;
  
  if (!drc_violation_list)
  {
    drc_violation_list = object_list_new(10, sizeof(DrcViolationType));
    drc_violation_list->ops = &drc_violation_ops;
  } else {
    object_list_clear(drc_violation_list);
    if (gui->drc_gui != NULL) gui->drc_gui->reset_drc_dialog_message();
  }
  
  /* This phony violation informs user about what DRC does NOT catch.  */
  violation = pcb_drc_violation_new (
      _("WARNING: DRC doesn't catch everything"),
      _("Detection of outright shorts, missing connections, etc.\n"
        "is handled via rat's nest addition.  To catch these problems,\n"
        "display the message log using Window->Message Log, then use\n"
        "Connects->Optimize rats nest (O hotkey) and watch for messages.\n"),
      /* All remaining arguments are not relevant to this application.  */
        0, 0, 0, TRUE, 0, 0, 0);
  append_drc_violation (violation);
  pcb_drc_violation_free (violation);
  
  drcerr_count = 0;
  
  /* Since the searching functions only operate on visible layers, we need
   * to make sure that everything is turned on in order to check the entire
   * design.
   *
   * TODO: This could be a way of giving the user control over what area to
   *       run the DRC on... ?
   *
   */
  
  /* Save the layer order and visibility settings so we can restore it later */
  SaveStackAndVisibility ();
  /* Turn on everything */
  ResetStackAndVisibility ();
  hid_action ("LayersChanged");
  InitConnectionLookup ();
  
  /* We'll do this again when we're done, and use undo to restore state. */
  if (ClearFlagOnAllObjects (FOUNDFLAG | DRCFLAG | SELECTEDFLAG, true)){
    undo_flags = 1;
    IncrementUndoSerialNumber ();
  }
  
  LockUndo(); /* Don't need to add all of these things */
  
  ELEMENT_LOOP (PCB->Data);
  {
    PIN_LOOP (element);
    {
      if (!TEST_FLAG (DRCFLAG, pin))
        DRCFind (PIN_TYPE, (void *) element, (void *) pin, (void *) pin);
    }
    END_LOOP;

    PAD_LOOP (element);
    {
      
      /* count up how many pads have no solderpaste openings */
      if (TEST_FLAG (NOPASTEFLAG, pad))
        nopastecnt++;
      
      if (!TEST_FLAG (DRCFLAG, pad))
        DRCFind (PAD_TYPE, (void *) element, (void *) pad, (void *) pad);
    }
    END_LOOP;
  }
  END_LOOP;
  
  VIA_LOOP (PCB->Data);
  {
    if (!TEST_FLAG (DRCFLAG, via))
      DRCFind (VIA_TYPE, (void *) via, (void *) via, (void *) via);
  }
  END_LOOP;
  
  /*
   * In the following, PlowsPolygon checks for the overlapping of bounding
   * boxes of objects and polygons, however, if the clearance is less than
   * the design rule, the boxes wont overlap. So we have to bloat the boxes
   * before calling PlowsPolygon.
   *
   * Bloating by PCB->Bloat ensures that the box is at least large enough to
   * deal with the design rule. If it's larger, it's okay, we'll get rid of
   * the false positives in the callback. 
   * */

  info.flag = SELECTEDFLAG;
  /* check minimum widths and polygon clearances */
  COPPERLINE_LOOP (PCB->Data);
  {
    SetThing (1, LINE_TYPE, layer, line, line);
    /* check line clearances in polygons */
    expand_obj_bbox(&thing1, 2*PCB->Bloat);
    PlowsPolygon (PCB->Data, LINE_TYPE, layer, line, drc_callback, &info);
    SetLineBoundingBox(line);
      
    if (line->Thickness < PCB->minWid)
    {
      drcerr_count++;
      LocateError (&x, &y);
      object_list_clear(vobjs);
      object_list_append(vobjs, &thing1);
      violation = pcb_drc_violation_new (
        _("Line width is too thin"),
        _("Process specifications dictate a minimum feature-width\n"
          "that can reliably be reproduced"),
        x, y,
        0,    /* ANGLE OF ERROR UNKNOWN */
        TRUE, /* MEASUREMENT OF ERROR KNOWN */
        line->Thickness,
        PCB->minWid,
        vobjs);
      append_drc_violation (violation);
      pcb_drc_violation_free (violation);
    }
  }
  ENDALL_LOOP;
  
  COPPERARC_LOOP (PCB->Data);
  {
    SetThing (1, ARC_TYPE, layer, arc, arc);
    expand_obj_bbox(&thing1, 2*PCB->Bloat);
    PlowsPolygon (PCB->Data, ARC_TYPE, layer, arc, drc_callback, &info);
    SetArcBoundingBox(arc);
    if (arc->Thickness < PCB->minWid)
    {
      drcerr_count++;
      LocateError (&x, &y);
      object_list_clear(vobjs);
      object_list_append(vobjs, &thing1);
      violation = pcb_drc_violation_new (
        _("Arc width is too thin"),
        _("Process specifications dictate a minimum feature-width\n"
          "that can reliably be reproduced"),
        x, y,
        0,    /* ANGLE OF ERROR UNKNOWN */
        TRUE, /* MEASUREMENT OF ERROR KNOWN */
        arc->Thickness,
        PCB->minWid,
        vobjs);
      append_drc_violation (violation);
      pcb_drc_violation_free (violation);
    }
  }
  ENDALL_LOOP;

  ALLPIN_LOOP (PCB->Data);
  {
    SetThing (1, PIN_TYPE, element, pin, pin);
    expand_obj_bbox(&thing1, 2*PCB->Bloat);
    PlowsPolygon (PCB->Data, PIN_TYPE, element, pin, drc_callback, &info);
    SetPinBoundingBox(pin);
    if (!TEST_FLAG (HOLEFLAG, pin) &&
        pin->Thickness - pin->DrillingHole < 2 * PCB->minRing)
    {
      drcerr_count++;
      LocateError (&x, &y);
      object_list_clear(vobjs);
      object_list_append(vobjs, &thing1);
      violation = pcb_drc_violation_new (
        _("Pin annular ring too small"),
        _("Annular rings that are too small may erode during etching,\n"
          "resulting in a broken connection"),
        x, y,
        0,    /* ANGLE OF ERROR UNKNOWN */
        TRUE, /* MEASUREMENT OF ERROR KNOWN */
        (pin->Thickness - pin->DrillingHole) / 2,
        PCB->minRing,
        vobjs);
      append_drc_violation (violation);
      pcb_drc_violation_free (violation);
    }
    if (pin->DrillingHole < PCB->minDrill)
    {
      drcerr_count++;
      LocateError (&x, &y);
      object_list_clear(vobjs);
      object_list_append(vobjs, &thing1);
      violation = pcb_drc_violation_new (
        _("Pin drill size is too small"),
        _("Process rules dictate the minimum drill size which can be "
          "used"),
        x, y,
       0,    /* ANGLE OF ERROR UNKNOWN */
       TRUE, /* MEASUREMENT OF ERROR KNOWN */
       pin->DrillingHole,
       PCB->minDrill,
       vobjs);
      append_drc_violation (violation);
      pcb_drc_violation_free (violation);
    }
  }
  ENDALL_LOOP;

  ALLPAD_LOOP (PCB->Data);
  {
    SetThing (1, PAD_TYPE, element, pad, pad);
    expand_obj_bbox(&thing1, 2*PCB->Bloat);
    PlowsPolygon (PCB->Data, PAD_TYPE, element, pad, drc_callback, &info);
    SetPadBoundingBox(pad);
    if (pad->Thickness < PCB->minWid)
    {
      drcerr_count++;
      LocateError (&x, &y);
      object_list_clear(vobjs);
      object_list_append(vobjs, &thing1);
      violation = pcb_drc_violation_new (
        _("Pad is too thin"),
        _("Pads which are too thin may erode during etching,\n"
          "resulting in a broken or unreliable connection"),
        x, y,
        0,    /* ANGLE OF ERROR UNKNOWN */
        TRUE, /* MEASUREMENT OF ERROR KNOWN */
        pad->Thickness,
        PCB->minWid,
        vobjs);
      append_drc_violation (violation);
      pcb_drc_violation_free (violation);
    }
  }
  ENDALL_LOOP;

  VIA_LOOP (PCB->Data);
  {
    SetThing (1, VIA_TYPE, via, via, via);
    expand_obj_bbox(&thing1, 2*PCB->Bloat);
    PlowsPolygon (PCB->Data, VIA_TYPE, via, via, drc_callback, &info);
    SetPinBoundingBox(via);
    if (!TEST_FLAG (HOLEFLAG, via) &&
        via->Thickness - via->DrillingHole < 2 * PCB->minRing)
    {
      drcerr_count++;
      LocateError (&x, &y);
      object_list_clear(vobjs);
      object_list_append(vobjs, &thing1);
      violation = pcb_drc_violation_new (
        _("Via annular ring too small"),
        _("Annular rings that are too small may erode during etching,\n"
          "resulting in a broken connection"),
        x, y,
        0,    /* ANGLE OF ERROR UNKNOWN */
        TRUE, /* MEASUREMENT OF ERROR KNOWN */
        (via->Thickness - via->DrillingHole) / 2,
        PCB->minRing,
        vobjs);
      append_drc_violation (violation);
      pcb_drc_violation_free (violation);
    }
    if (via->DrillingHole < PCB->minDrill)
    {
      drcerr_count++;
      LocateError (&x, &y);
      object_list_clear(vobjs);
      object_list_append(vobjs, &thing1);
      violation = pcb_drc_violation_new (
        _("Via drill size is too small"),
        _("Process rules dictate the minimum drill size which can "
          "be used"),
          x, y,
          0,    /* ANGLE OF ERROR UNKNOWN */
          TRUE, /* MEASUREMENT OF ERROR KNOWN */
          via->DrillingHole,
          PCB->minDrill,
          vobjs);
      append_drc_violation (violation);
      pcb_drc_violation_free (violation);
    }
  }
  END_LOOP;
  
  FreeConnectionLookupMemory ();
  
  /* check silkscreen minimum widths outside of elements */
  /* XXX - need to check text and polygons too! */
  SILKLINE_LOOP (PCB->Data);
  {
    SetThing (1, LINE_TYPE, layer, line, line);
    if (line->Thickness < PCB->minSlk)
    {
      drcerr_count++;
      LocateError (&x, &y);
      object_list_clear(vobjs);
      object_list_append(vobjs, &thing1);
      violation = pcb_drc_violation_new (
        _("Silk line is too thin"),
        _("Process specifications dictate a minimum silkscreen\n"
          "feature-width that can reliably be reproduced"),
        x, y,
        0,    /* ANGLE OF ERROR UNKNOWN */
        TRUE, /* MEASUREMENT OF ERROR KNOWN */
        line->Thickness,
        PCB->minSlk,
        vobjs);
      append_drc_violation (violation);
      pcb_drc_violation_free (violation);
    }
  }
  ENDALL_LOOP;
  
  /* check silkscreen minimum widths inside of elements */
  /* XXX - need to check text and polygons too! */
  ELEMENT_LOOP (PCB->Data);
  {
    SetThing (1, ELEMENT_TYPE, element, element, element);
    tmpcnt = 0;
    ELEMENTLINE_LOOP (element);
    {
      if (line->Thickness < PCB->minSlk)  tmpcnt++;
    }
    END_LOOP;
    if (tmpcnt > 0)
    {
      char *title;
      char *name;
      char *buffer;
      int buflen;
      drcerr_count++;
      LocateError (&x, &y);
      object_list_clear(vobjs);
      object_list_append(vobjs, &thing1);
      title = _("Element %s has %i silk lines which are too thin");
      name = (char *)UNKNOWN (NAMEONPCB_NAME (element));
      
      /* -4 is for the %s and %i place-holders */
      /* +11 is the max printed length for a 32 bit integer */
      /* +1 is for the \0 termination */
      buflen = strlen (title) - 4 + strlen (name) + 11 + 1;
      buffer = (char *)malloc (buflen);
      snprintf (buffer, buflen, title, name, tmpcnt);
      
      violation = pcb_drc_violation_new (buffer,
        _("Process specifications dictate a minimum silkscreen\n"
          "feature-width that can reliably be reproduced"),
        x, y,
        0,    /* ANGLE OF ERROR UNKNOWN */
        TRUE, /* MEASUREMENT OF ERROR KNOWN */
        0,    /* MINIMUM OFFENDING WIDTH UNKNOWN */
        PCB->minSlk,
        vobjs);
      free (buffer);
      append_drc_violation (violation);
      pcb_drc_violation_free (violation);
    }
  }
  END_LOOP;
    
  ClearFlagOnAllObjects ((FOUNDFLAG | DRCFLAG | SELECTEDFLAG), false);
  UnlockUndo ();
  
  if (undo_flags)  Undo(false);
  
  RestoreStackAndVisibility ();
  hid_action ("LayersChanged");
  gui->invalidate_all ();
  
  if (nopastecnt > 0)
  {
    Message (ngettext ("Warning: %d pad has the nopaste flag set.\n",
                       "Warning: %d pads have the nopaste flag set.\n",
                       nopastecnt), nopastecnt);
  }
  object_list_delete(vobjs);
  return drcerr_count;
}


/* ----------------------------------------------------------------------- *
 * Actions
 * ----------------------------------------------------------------------- */

static const char drc_syntax[] = N_("DRC()");

static const char drc_help[] = N_("Invoke the DRC check.");

/* %start-doc actions DRC
 
 Note that the design rule check uses the current board rule settings,
 not the current style settings.
 
 %end-doc */

static int
ActionDRCheck (int argc, char **argv, Coord x, Coord y)
{
  int count;
  
  if (gui->drc_gui == NULL || gui->drc_gui->log_drc_overview)
  {
    Message (_("%m+Rules are minspace %$mS, minoverlap %$mS "
               "minwidth %$mS, minsilk %$mS\n"
               "min drill %$mS, min annular ring %$mS\n"),
             Settings.grid_unit->allow,
             PCB->Bloat, PCB->Shrink,
             PCB->minWid, PCB->minSlk,
             PCB->minDrill, PCB->minRing);
  }
  count = DRCAll ();
  if (gui->drc_gui == NULL || gui->drc_gui->log_drc_overview)
  {
    if (count == 0)
      Message (_("No DRC problems found.\n"));
    else if (count > 0)
      Message (_("Found %d design rule errors.\n"), count);
    else
      Message (_("Aborted DRC after %d design rule errors.\n"), -count);
  }
  return 0;
}

static const char drc_report_syntax[] = N_("DRCReport([Output file])");
static const char drc_report_help[] =
N_("Write the DRC violation data from the last DRC to a file.");

static int
ActionDRCReport (int argc, char **argv, Coord x, Coord y)
{
  int i=0, len=0;
  FILE * fp;
  char starliner[81];
  char buffer[80];
  
  if (!drc_violation_list)
  {
  Message("DRCReport: Must run DRC check first.\n");
  return 0;
  }

  memset(starliner, '*', 80);
  starliner[80] = '\0';
  if (argc == 1) fp = fopen(argv[0], "w");
  else fp = stdout;
  
  for (i=0; i < drc_violation_list->count; i++){
    len = sprintf(buffer, "Violation %d", i);
    fprintf(fp, "%s\n%*s\n%s\n", starliner, 40+len/2, buffer, starliner);
    pcb_drc_violation_print(fp,
           (DrcViolationType*) object_list_get_item(drc_violation_list,i));
    fprintf(fp, "\n");
  }
  if (argc == 1) fclose(fp);
  return 0;
}

static const char drc_review_syntax[] = N_("DRCReview()");
static const char drc_review_help[] =
N_("Iterate through the list of DRC violations and present them.");

static int
ActionDRCReview (int argc, char **argv, Coord x, Coord y)
{
  int i=0, cont=0;
  
  if (!drc_violation_list)
  {
  Message("DRCReview: Must run DRC check first.\n");
  return 0;
  }

  for (i=0; i < drc_violation_list->count; i++)
  {
    cont = pcb_drc_violation_prompt (
            (DrcViolationType*) object_list_get_item (drc_violation_list,i));
    if (!cont) break;
  }
  return 0;
}

HID_Action drc_action_list[] = {
  {"DRC", 0, ActionDRCheck, drc_help, drc_syntax},
  {"DRCReport", 0, ActionDRCReport, drc_report_help, drc_report_syntax},
  {"DRCReview", 0, ActionDRCReview, drc_review_help, drc_review_syntax},
};

REGISTER_ACTIONS (drc_action_list)

