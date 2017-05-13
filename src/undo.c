/*!
 * \file src/undo.c
 *
 * \brief Functions used to undo operations.
 *
 * Description:
 *
 * There are two lists which hold:
 * - information about a command
 * - data of removed objects
 *
 * Both lists are organized as first-in-last-out which means that the undo
 * list can always use the last entry of the remove list.
 *
 * A serial number is incremented whenever an operation is completed.
 *
 * An operation itself may consist of several basic instructions.
 *
 * E.g.: removing all selected objects is one operation with exactly one
 * serial number even if the remove function is called several times.
 *
 * \note A lock flag ensures that no infinite loops occur.
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
 *
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *
 * Thomas.Nau@rz.uni-ulm.de
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <memory.h>
#include "global.h"

#include "buffer.h"
#include "change.h"
#include "create.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "insert.h"
#include "misc.h"
#include "mirror.h"
#include "move.h"
#include "mymem.h"
#include "polygon.h"
#include "remove.h"
#include "rotate.h"
#include "rtree.h"
#include "search.h"
#include "set.h"
#include "undo.h"
#include "strflags.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

static bool between_increment_and_restore = false;
static bool added_undo_between_increment_and_restore = false;

/* ---------------------------------------------------------------------------
 * some local data types
 */

/*!
 * \brief Information about a change command.
 */
typedef struct
{
  char *Name;
} ChangeNameType;

/*!
 * \brief Information about a move command.
 */
typedef struct
{
  Coord DX; /*!< movement vector. */
  Coord DY; /*!< movement vector. */
} MoveType;

/*!
 * \brief Information about removed polygon points.
 */
typedef struct
{
  Coord X; /*!< Data. */
  Coord Y; /*!< Data. */
  int ID;
  Cardinal Index; /*!< Index in a polygons array of points. */
  bool last_in_contour; /*!< Whether the point was the last in its contour. */
} RemovedPointType;

/*!
 * \brief Information about rotation.
 */
typedef struct
{
  Coord CenterX; /*!< Center of rotation. */
  Coord CenterY; /*!< Center of rotation. */
  Cardinal Steps; /*!< Number of steps. */
} RotateType;

/*!
 * \brief Information about moves between layers.
 */
typedef struct
{
  Cardinal OriginalLayer; /*!< The index of the original layer. */
} MoveToLayerType;

/*!
 * \brief Information about layer changes.
 */
typedef struct
{
  int old_index;
  int new_index;
} LayerChangeType;

/*!
 * \brief Information about layer changes.
 */
typedef struct
{
  int from;
  int to;
} SetViaLayersChangeType;

/*!
 * \brief Information about poly clear/restore.
 */
typedef struct
{
  bool Clear; /*!< true was clear, false was restore. */
  LayerType *Layer;
} ClearPolyType;

/*!
 * \brief Information about netlist lib changes.
 */
typedef struct
{
  LibraryType *old;
  LibraryType *lib;
} NetlistChangeType;

/*!
 * \brief Holds information about an operation.
 */
typedef struct
{
  int Serial, /*!< Serial number of operation. */
    Type, /*!< Type of operation. */
    Kind, /*!< Type of object with given ID. */
    ID; /*!< Object ID. */
  union /* Some additional information. */
  {
    ChangeNameType ChangeName;
    MoveType Move;
    RemovedPointType RemovedPoint;
    RotateType Rotate;
    MoveToLayerType MoveToLayer;
    FlagType Flags;
    Coord Size;
    int Scale;
    LayerChangeType LayerChange;
    ClearPolyType ClearPoly;
    NetlistChangeType NetlistChange;
    SetViaLayersChangeType SetViaLayersChange;
    long int CopyID;
  }
  Data;
} UndoListType;

/* ---------------------------------------------------------------------------
 * some local variables
 */
static DataType *RemoveList = NULL; /*!< List of removed objects. */
static UndoListType *UndoList = NULL; /*!< List of operations. */
static int Serial = 1; /*!< Serial number. */
static int SavedSerial;
static size_t UndoN; /*!< Number of entries. */
static size_t RedoN; /*!< Number of entries. */
static size_t UndoMax; /*!< Number of entries. */
static bool Locked = false; /*!< Do not add entries if. */
static bool andDraw = true;
										/* flag is set; prevents from */
										/* infinite loops */

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static UndoListType *GetUndoSlot (int, int, int);
static void DrawRecoveredObject (int, void *, void *, void *);
static bool UndoRotate (UndoListType *);
static bool UndoChangeName (UndoListType *);
static bool UndoCopyOrCreate (UndoListType *);
static bool UndoMove (UndoListType *);
static bool UndoRemove (UndoListType *);
static bool UndoRemovePoint (UndoListType *);
static bool UndoInsertPoint (UndoListType *);
static bool UndoRemoveContour (UndoListType *);
static bool UndoInsertContour (UndoListType *);
static bool UndoMoveToLayer (UndoListType *);
static bool UndoFlag (UndoListType *);
static bool UndoMirror (UndoListType *);
static bool UndoChangeSize (UndoListType *);
static bool UndoChange2ndSize (UndoListType *);
static bool UndoChangeAngles (UndoListType *);
static bool UndoChangeClearSize (UndoListType *);
static bool UndoChangeMaskSize (UndoListType *);
static bool UndoClearPoly (UndoListType *);
static bool UndoSetViaLayers (UndoListType *);
static int PerformUndo (UndoListType *);

/*!
 * \brief Adds a command plus some data to the undo list.
 */
static UndoListType *
GetUndoSlot (int CommandType, int ID, int Kind)
{
  UndoListType *ptr;
  void *ptr1, *ptr2, *ptr3;
  int type;
  static size_t limit = UNDO_WARNING_SIZE;

#ifdef DEBUG_ID
  if (SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, ID, Kind) == NO_TYPE)
    Message ("hace: ID (%d) and Type (%x) mismatch in AddObject...\n", ID,
	     Kind);
#endif

  /* allocate memory */
  if (UndoN >= UndoMax)
    {
      size_t size;

      UndoMax += STEP_UNDOLIST;
      size = UndoMax * sizeof (UndoListType);
      UndoList = (UndoListType *) realloc (UndoList, size);
      memset (&UndoList[UndoN], 0, STEP_REMOVELIST * sizeof (UndoListType));

      /* ask user to flush the table because of it's size */
      if (size > limit)
	{
	  limit = (size / UNDO_WARNING_SIZE + 1) * UNDO_WARNING_SIZE;
	  Message (_("Size of 'undo-list' exceeds %li kb\n"),
		   (long) (size >> 10));
	}
    }

  /* free structures from the pruned redo list */

  for (ptr = &UndoList[UndoN]; RedoN; ptr++, RedoN--)
    switch (ptr->Type)
      {
      case UNDO_CHANGENAME:
	free (ptr->Data.ChangeName.Name);
	break;
      case UNDO_REMOVE:
	type =
	  SearchObjectByID (RemoveList, &ptr1, &ptr2, &ptr3, ptr->ID,
			    ptr->Kind);
	if (type != NO_TYPE)
	  {
	    DestroyObject (RemoveList, type, ptr1, ptr2, ptr3);
	  }
	break;
      default:
	break;
      }

  if (between_increment_and_restore)
    added_undo_between_increment_and_restore = true;

  /* copy typefield and serial number to the list */
  ptr = &UndoList[UndoN++];
  ptr->Type = CommandType;
  ptr->Kind = Kind;
  ptr->ID = ID;
  ptr->Serial = Serial;
  return (ptr);
}

/*!
 * \brief Redraws the recovered object.
 */
static void
DrawRecoveredObject (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  if (Type & (LINE_TYPE | TEXT_TYPE | POLYGON_TYPE | ARC_TYPE))
    {
      LayerType *layer;

      layer = LAYER_PTR (GetLayerNumber (RemoveList, (LayerType *) Ptr1));
      DrawObject (Type, (void *) layer, Ptr2);
    }
  else
    DrawObject (Type, Ptr1, Ptr2);
}

/*!
 * \brief Recovers an object from a 'rotate' operation.
 *
 * \return true if anything has been recovered.
 */
static bool
UndoRotate (UndoListType *Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;

  /* lookup entry by it's ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type != NO_TYPE)
    {
      RotateObject (type, ptr1, ptr2, ptr3,
		    Entry->Data.Rotate.CenterX, Entry->Data.Rotate.CenterY,
		    (4 - Entry->Data.Rotate.Steps) & 0x03);
      Entry->Data.Rotate.Steps = (4 - Entry->Data.Rotate.Steps) & 0x03;
      return (true);
    }
  return (false);
}

/*!
 * \brief Recovers an object from a clear/restore poly operation.
 *
 * \return true if anything has been recovered.
 */
static bool
UndoClearPoly (UndoListType *Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;

  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type != NO_TYPE)
    {
      if (Entry->Data.ClearPoly.Clear)
	RestoreToPolygon (PCB->Data, type, Entry->Data.ClearPoly.Layer, ptr3);
      else
	ClearFromPolygon (PCB->Data, type, Entry->Data.ClearPoly.Layer, ptr3);
      Entry->Data.ClearPoly.Clear = !Entry->Data.ClearPoly.Clear;
      return true;
    }
  return false;
}

/*!
 * \brief Recovers an object from a 'change name' operation.
 *
 * \return true if anything has been recovered.
 */
static bool
UndoChangeName (UndoListType *Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;

  /* lookup entry by it's ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type != NO_TYPE)
    {
      Entry->Data.ChangeName.Name =
	(char *)(ChangeObjectName (type, ptr1, ptr2, ptr3,
			   Entry->Data.ChangeName.Name));
      return (true);
    }
  return (false);
}

/*!
 * \brief Recovers an object from a 2ndSize change operation.
 */
static bool
UndoChange2ndSize (UndoListType *Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;
  Coord swap;

  /* lookup entry by ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type != NO_TYPE)
    {
      swap = ((PinType *) ptr2)->DrillingHole;
      if (andDraw)
	EraseObject (type, ptr1, ptr2);
      ((PinType *) ptr2)->DrillingHole = Entry->Data.Size;
      Entry->Data.Size = swap;
      DrawObject (type, ptr1, ptr2);
      return (true);
    }
  return (false);
}

/*!
 * \brief Recovers an object from a ChangeAngles change operation.
 */
static bool
UndoChangeAngles (UndoListType *Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;
  long int old_sa, old_da;

  /* lookup entry by ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type == ARC_TYPE)
    {
      LayerType *Layer = (LayerType *) ptr1;
      ArcType *a = (ArcType *) ptr2;
      r_delete_entry (Layer->arc_tree, (BoxType *) a);
      old_sa = a->StartAngle;
      old_da = a->Delta;
      if (andDraw)
	EraseObject (type, Layer, a);
      a->StartAngle = Entry->Data.Move.DX;
      a->Delta = Entry->Data.Move.DY;
      SetArcBoundingBox (a);
      r_insert_entry (Layer->arc_tree, (BoxType *) a, 0);
      Entry->Data.Move.DX = old_sa;
      Entry->Data.Move.DY = old_da;;
      DrawObject (type, ptr1, a);
      return (true);
    }
  return (false);
}

/*!
 * \brief Recovers an object from a clearance size change operation.
 */
static bool
UndoChangeClearSize (UndoListType *Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;
  Coord swap;

  /* lookup entry by ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type != NO_TYPE)
    {
      swap = ((PinType *) ptr2)->Clearance;
      RestoreToPolygon (PCB->Data, type, ptr1, ptr2);
      if (andDraw)
	EraseObject (type, ptr1, ptr2);
      ((PinType *) ptr2)->Clearance = Entry->Data.Size;
      ClearFromPolygon (PCB->Data, type, ptr1, ptr2);
      Entry->Data.Size = swap;
      if (andDraw)
	DrawObject (type, ptr1, ptr2);
      return (true);
    }
  return (false);
}

/*!
 * \brief Recovers an object from a mask size change operation.
 */
static bool
UndoChangeMaskSize (UndoListType *Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;
  Coord swap;

  /* lookup entry by ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type & (VIA_TYPE | PIN_TYPE | PAD_TYPE))
    {
      swap =
	(type ==
	 PAD_TYPE ? ((PadType *) ptr2)->Mask : ((PinType *) ptr2)->Mask);
      if (andDraw)
	EraseObject (type, ptr1, ptr2);
      if (type == PAD_TYPE)
	((PadType *) ptr2)->Mask = Entry->Data.Size;
      else
	((PinType *) ptr2)->Mask = Entry->Data.Size;
      Entry->Data.Size = swap;
      if (andDraw)
	DrawObject (type, ptr1, ptr2);
      return (true);
    }
  return (false);
}


/*!
 * \brief Recovers an object from a Size change operation.
 */
static bool
UndoChangeSize (UndoListType *Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type, iswap = 0;
  Coord swap = 0;

  /* lookup entry by ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type != NO_TYPE)
    {
      /* Wow! can any object be treated as a pin type for size change?? */
      /* pins, vias, lines, and arcs can. Text can't but it has it's own mechanism */
      switch (type)
	{
	case PIN_TYPE:
	case VIA_TYPE:
	  swap = ((PinType *) ptr2)->Thickness;
	  break;
	case LINE_TYPE:
	case ELEMENTLINE_TYPE:
	  swap = ((LineType *) ptr2)->Thickness;
	  break;
	case TEXT_TYPE:
	case ELEMENTNAME_TYPE:
	  iswap = ((TextType *) ptr2)->Scale;
	  break;
	case PAD_TYPE:
	  swap = ((PadType *) ptr2)->Thickness;
	  break;
	case ARC_TYPE:
	case ELEMENTARC_TYPE:
	  swap = ((ArcType *) ptr2)->Thickness;
	  break;
	}

      RestoreToPolygon (PCB->Data, type, ptr1, ptr2);
      if (andDraw)
	EraseObject (type, ptr1, ptr2);

      switch (type)
	{
	case PIN_TYPE:
	case VIA_TYPE:
	  ((PinType *) ptr2)->Thickness = Entry->Data.Size;
	  Entry->Data.Size = swap;
	  break;
	case LINE_TYPE:
	case ELEMENTLINE_TYPE:
	  ((LineType *) ptr2)->Thickness = Entry->Data.Size;
	  Entry->Data.Size = swap;
	  break;
	case TEXT_TYPE:
	case ELEMENTNAME_TYPE:
	  ((TextType *) ptr2)->Scale = Entry->Data.Scale;
	  Entry->Data.Scale = iswap;
	  break;
	case PAD_TYPE:
	  ((PadType *) ptr2)->Thickness = Entry->Data.Size;
	  Entry->Data.Size = swap;
	  break;
	case ARC_TYPE:
	case ELEMENTARC_TYPE:
	  ((ArcType *) ptr2)->Thickness = Entry->Data.Size;
	  Entry->Data.Size = swap;
	  break;
	}

      ClearFromPolygon (PCB->Data, type, ptr1, ptr2);
      if (andDraw)
	DrawObject (type, ptr1, ptr2);
      return (true);
    }
  return (false);
}

/*!
 * \brief Recovers an object from a FLAG change operation.
 */
static bool
UndoFlag (UndoListType *Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;
  FlagType swap;
  int must_redraw;

  /* lookup entry by ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type != NO_TYPE)
    {
      FlagType f1, f2;
      PinType *pin = (PinType *) ptr2;

      swap = pin->Flags;

      must_redraw = 0;
      f1 = MaskFlags (pin->Flags, ~DRAW_FLAGS);
      f2 = MaskFlags (Entry->Data.Flags, ~DRAW_FLAGS);

      if (!FLAGS_EQUAL (f1, f2))
	must_redraw = 1;

      if (andDraw && must_redraw)
	EraseObject (type, ptr1, ptr2);

      pin->Flags = Entry->Data.Flags;

      Entry->Data.Flags = swap;

      if (andDraw && must_redraw)
	DrawObject (type, ptr1, ptr2);
      return (true);
    }
  Message ("hace Internal error: Can't find ID %d type %08x\n", Entry->ID,
	   Entry->Kind);
  Message ("for UndoFlag Operation. Previous flags: %s\n",
	   flags_to_string (Entry->Data.Flags, 0));
  return (false);
}

/*!
 * \brief Recovers an object from a mirror operation.
 *
 * \return true if anything has been recovered.
 */
static bool
UndoMirror (UndoListType *Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;

  /* lookup entry by ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type == ELEMENT_TYPE)
    {
      ElementType *element = (ElementType *) ptr3;
      if (andDraw)
	EraseElement (element);
      MirrorElementCoordinates (PCB->Data, element, Entry->Data.Move.DY);
      if (andDraw)
	DrawElement (element);
      return (true);
    }
  Message ("hace Internal error: UndoMirror on object type %d\n", type);
  return (false);
}

/*!
 * \brief Recovers an object from a 'copy' or 'create' operation.
 *
 * \return true if anything has been recovered.
 */
static bool
UndoCopyOrCreate (UndoListType *Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;

  /* lookup entry by it's ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type != NO_TYPE)
    {
      if (!RemoveList)
	RemoveList = CreateNewBuffer ();
      if (andDraw)
	EraseObject (type, ptr1, ptr2);
      /* in order to make this re-doable we move it to the RemoveList */
      MoveObjectToBuffer (RemoveList, PCB->Data, type, ptr1, ptr2, ptr3);
      Entry->Type = UNDO_REMOVE;
      return (true);
    }
  return (false);
}

/*!
 * \brief Recovers an object from a 'move' operation.
 *
 * \return true if anything has been recovered.
 */
static bool
UndoMove (UndoListType *Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;

  /* lookup entry by it's ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type != NO_TYPE)
    {
      MoveObject (type, ptr1, ptr2, ptr3,
		  -Entry->Data.Move.DX, -Entry->Data.Move.DY);
      Entry->Data.Move.DX *= -1;
      Entry->Data.Move.DY *= -1;
      return (true);
    }
  return (false);
}

/*!
 * \brief Recovers an object from a 'remove' operation.
 *
 * \return true if anything has been recovered.
 */
static bool
UndoRemove (UndoListType *Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;

  /* lookup entry by it's ID */
  type =
    SearchObjectByID (RemoveList, &ptr1, &ptr2, &ptr3, Entry->ID,
		      Entry->Kind);
  if (type != NO_TYPE)
    {
      if (andDraw)
	DrawRecoveredObject (type, ptr1, ptr2, ptr3);
      MoveObjectToBuffer (PCB->Data, RemoveList, type, ptr1, ptr2, ptr3);
      Entry->Type = UNDO_CREATE;
      return (true);
    }
  return (false);
}

/*!
 * \brief Recovers an object from a 'move to another layer' operation.
 *
 * \return true if anything has been recovered.
 */
static bool
UndoMoveToLayer (UndoListType *Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type, swap;

  /* lookup entry by it's ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type != NO_TYPE)
    {
      swap = GetLayerNumber (PCB->Data, (LayerType *) ptr1);
      MoveObjectToLayer (type, ptr1, ptr2, ptr3,
			 LAYER_PTR (Entry->Data.
				    MoveToLayer.OriginalLayer), true);
      Entry->Data.MoveToLayer.OriginalLayer = swap;
      return (true);
    }
  return (false);
}

/*!
 * \brief Recovers a removed polygon point.
 *
 * \return true on success.
 */
static bool
UndoRemovePoint (UndoListType *Entry)
{
  LayerType *layer;
  PolygonType *polygon;
  void *ptr3;
  int type;

  /* lookup entry (polygon not point was saved) by it's ID */
  assert (Entry->Kind == POLYGON_TYPE);
  type =
    SearchObjectByID (PCB->Data, (void **) &layer, (void **) &polygon, &ptr3,
		      Entry->ID, Entry->Kind);
  switch (type)
    {
    case POLYGON_TYPE:		/* restore the removed point */
      {
	/* recover the point */
	if (andDraw && layer->On)
	  ErasePolygon (polygon);
	InsertPointIntoObject (POLYGON_TYPE, layer, polygon,
			       &Entry->Data.RemovedPoint.Index,
			       Entry->Data.RemovedPoint.X,
			       Entry->Data.RemovedPoint.Y, true,
			       Entry->Data.RemovedPoint.last_in_contour);

	polygon->Points[Entry->Data.RemovedPoint.Index].ID =
	  Entry->Data.RemovedPoint.ID;
	if (andDraw && layer->On)
	  DrawPolygon (layer, polygon);
	Entry->Type = UNDO_INSERT_POINT;
	Entry->ID = Entry->Data.RemovedPoint.ID;
	Entry->Kind = POLYGONPOINT_TYPE;
	return (true);
      }

    default:
      return (false);
    }
}

/*!
 * \brief Recovers an inserted polygon point.
 *
 * \return true on success.
 */
static bool
UndoInsertPoint (UndoListType *Entry)
{
  LayerType *layer;
  PolygonType *polygon;
  PointType *pnt;
  int type;
  Cardinal point_idx;
  Cardinal hole;
  bool last_in_contour = false;

  assert (Entry->Kind == POLYGONPOINT_TYPE);
  /* lookup entry by it's ID */
  type =
    SearchObjectByID (PCB->Data, (void **) &layer, (void **) &polygon,
		      (void **) &pnt, Entry->ID, Entry->Kind);
  switch (type)
    {
    case POLYGONPOINT_TYPE:	/* removes an inserted polygon point */
      {
	if (andDraw && layer->On)
	  ErasePolygon (polygon);

	/* Check whether this point was at the end of its contour.
	 * If so, we need to flag as such when re-adding the point
	 * so it goes back in the correct place
	 */
	point_idx = polygon_point_idx (polygon, pnt);
	for (hole = 0; hole < polygon->HoleIndexN; hole++)
	  if (point_idx == polygon->HoleIndex[hole] - 1)
	    last_in_contour = true;
	if (point_idx == polygon->PointN - 1)
	  last_in_contour = true;
	Entry->Data.RemovedPoint.last_in_contour = last_in_contour;

	Entry->Data.RemovedPoint.X = pnt->X;
	Entry->Data.RemovedPoint.Y = pnt->Y;
	Entry->Data.RemovedPoint.ID = pnt->ID;
	Entry->ID = polygon->ID;
	Entry->Kind = POLYGON_TYPE;
	Entry->Type = UNDO_REMOVE_POINT;
	Entry->Data.RemovedPoint.Index = point_idx;
	DestroyObject (PCB->Data, POLYGONPOINT_TYPE, layer, polygon, pnt);
	if (andDraw && layer->On)
	  DrawPolygon (layer, polygon);
	return (true);
      }

    default:
      return (false);
    }
}

static bool
UndoSwapCopiedObject (UndoListType *Entry)
{
  void *ptr1, *ptr2, *ptr3;
  void *ptr1b, *ptr2b, *ptr3b;
  AnyObjectType *obj, *obj2;
  int type;
  long int swap_id;

  /* lookup entry by it's ID */
  type =
    SearchObjectByID (RemoveList, &ptr1, &ptr2, &ptr3, Entry->Data.CopyID,
		      Entry->Kind);
  if (type == NO_TYPE)
    return false;

  type =
    SearchObjectByID (PCB->Data, &ptr1b, &ptr2b, &ptr3b, Entry->ID,
		      Entry->Kind);
  if (type == NO_TYPE)
    return FALSE;

  obj = (AnyObjectType *)ptr2;
  obj2 = (AnyObjectType *)ptr2b;

  swap_id = obj->ID;
  obj->ID = obj2->ID;
  obj2->ID = swap_id;

  MoveObjectToBuffer (RemoveList, PCB->Data, type, ptr1b, ptr2b, ptr3b);

  if (andDraw)
    DrawRecoveredObject (Entry->Kind, ptr1, ptr2, ptr3);

  obj = (AnyObjectType *)MoveObjectToBuffer (PCB->Data, RemoveList, type, ptr1, ptr2, ptr3);
  if (Entry->Kind == POLYGON_TYPE)
    InitClip (PCB->Data, (LayerType *)ptr1b, (PolygonType *)obj);
  return (true);
}

/*!
 * \brief Recovers an removed polygon point.
 *
 * \return true on success.
 */
static bool
UndoRemoveContour (UndoListType *Entry)
{
  assert (Entry->Kind == POLYGON_TYPE);
  return UndoSwapCopiedObject (Entry);
}

/*!
 * \brief Recovers an inserted polygon point.
 *
 * \return true on success.
 */
static bool
UndoInsertContour (UndoListType *Entry)
{
  assert (Entry->Kind == POLYGON_TYPE);
  return UndoSwapCopiedObject (Entry);
}

/*!
 * \brief Undo a layer change.
 *
 * \return true on success.
 */
static bool
UndoLayerChange (UndoListType *Entry)
{
  LayerChangeType *l = &Entry->Data.LayerChange;
  int tmp;

  tmp = l->new_index;
  l->new_index = l->old_index;
  l->old_index = tmp;

  if (MoveLayer (l->old_index, l->new_index))
    return false;
  else
    return true;
}

/*!
 * \brief Undo a netlist change.
 *
 * \return true on success.
 */
static bool
UndoNetlistChange (UndoListType *Entry)
{
  NetlistChangeType *l = & Entry->Data.NetlistChange;
  unsigned int i, j;
  LibraryType *lib, *saved;

  lib = l->lib;
  saved = l->old;

  /* iterate over each net */
  for (i = 0 ; i < lib->MenuN; i++)
    {
      if (lib->Menu[i].Name)
	free (lib->Menu[i].Name);

      if (lib->Menu[i].directory)
	free (lib->Menu[i].directory);

      if (lib->Menu[i].Style)
	free (lib->Menu[i].Style);

      /* iterate over each pin on the net */
      for (j = 0; j < lib->Menu[i].EntryN; j++) {
	
	if (lib->Menu[i].Entry[j].ListEntry)
	  free (lib->Menu[i].Entry[j].ListEntry);
	
	if (lib->Menu[i].Entry[j].AllocatedMemory)
	  free (lib->Menu[i].Entry[j].AllocatedMemory);
	
	if (lib->Menu[i].Entry[j].Template)
	  free (lib->Menu[i].Entry[j].Template);
	
	if (lib->Menu[i].Entry[j].Package)
	  free (lib->Menu[i].Entry[j].Package);
	
	if (lib->Menu[i].Entry[j].Value)
	  free (lib->Menu[i].Entry[j].Value);
	
	if (lib->Menu[i].Entry[j].Description)
	  free (lib->Menu[i].Entry[j].Description);
	
      }
    }

  if (lib->Menu)
    free (lib->Menu);

  *lib = *saved;

  NetlistChanged (0);
  return true;
}

/* ---------------------------------------------------------------------------
 * recovers an object from a Size change operation
 */
static bool
UndoSetViaLayers (UndoListType *Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;
  int from, to;

  /* lookup entry by ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type == VIA_TYPE)
    {
      from = ((PinType *) ptr2)->BuriedFrom;
      to = ((PinType *) ptr2)->BuriedTo;
      RestoreToPolygon (PCB->Data, type, ptr1, ptr2);
      if (andDraw)
	EraseObject (type, ptr1, ptr2);
      ((PinType *) ptr2)->BuriedFrom = Entry->Data.SetViaLayersChange.from;
      ((PinType *) ptr2)->BuriedTo = Entry->Data.SetViaLayersChange.to;
      Entry->Data.SetViaLayersChange.from = from;
      Entry->Data.SetViaLayersChange.to = to;
      ClearFromPolygon (PCB->Data, type, ptr1, ptr2);
      if (andDraw)
	DrawObject (type, ptr1, ptr2);
      return (true);
    }
  return (false);
}

/* ---------------------------------------------------------------------------
 * \brief Undo of any 'hard to recover' operation
 *
 * \return The bitfield for the types of operations that were undone.
 */
int
Undo (bool draw)
{
  UndoListType *ptr;
  int Types = 0;
  int unique;
  bool error_undoing = false;

  unique = TEST_FLAG (UNIQUENAMEFLAG, PCB);
  CLEAR_FLAG (UNIQUENAMEFLAG, PCB);

  andDraw = draw;

  if (Serial == 0)
    {
      Message (_("ERROR: Attempt to Undo() with Serial == 0\n"
                 "       Please save your work and report this bug.\n"));
      return 0;
    }

  if (UndoN == 0)
    {
      Message (_("Nothing to undo - buffer is empty\n"));
      return 0;
    }

  Serial --;

  ptr = &UndoList[UndoN - 1];

  if (ptr->Serial > Serial)
    {
      Message (_("ERROR: Bad undo serial number %d in undo stack - expecting %d or lower\n"
                 "       Please save your work and report this bug.\n"),
               ptr->Serial, Serial);

      /* It is likely that the serial number got corrupted through some bad
       * use of the SaveUndoSerialNumber() / RestoreUndoSerialNumber() APIs.
       *
       * Reset the serial number to be consistent with that of the last
       * operation on the undo stack in the hope that this might clear
       * the problem and  allow the user to hit Undo again.
       */
      Serial = ptr->Serial + 1;
      return 0;
    }

  LockUndo (); /* lock undo module to prevent from loops */

  /* Loop over all entries with the correct serial number */
  for (; UndoN && ptr->Serial == Serial; ptr--, UndoN--, RedoN++)
    {
      int undid = PerformUndo (ptr);
      if (undid == 0)
        error_undoing = true;
      Types |= undid;
    }

  UnlockUndo ();

  if (error_undoing)
    Message (_("ERROR: Failed to undo some operations\n"));

  if (Types && andDraw)
    Draw ();

  /* restore the unique flag setting */
  if (unique)
    SET_FLAG (UNIQUENAMEFLAG, PCB);

  return Types;
}

static int
PerformUndo (UndoListType *ptr)
{
  switch (ptr->Type)
    {
    case UNDO_CHANGENAME:
      if (UndoChangeName (ptr))
	return (UNDO_CHANGENAME);
      break;

    case UNDO_CREATE:
      if (UndoCopyOrCreate (ptr))
	return (UNDO_CREATE);
      break;

    case UNDO_MOVE:
      if (UndoMove (ptr))
	return (UNDO_MOVE);
      break;

    case UNDO_REMOVE:
      if (UndoRemove (ptr))
	return (UNDO_REMOVE);
      break;

    case UNDO_REMOVE_POINT:
      if (UndoRemovePoint (ptr))
	return (UNDO_REMOVE_POINT);
      break;

    case UNDO_INSERT_POINT:
      if (UndoInsertPoint (ptr))
	return (UNDO_INSERT_POINT);
      break;

    case UNDO_REMOVE_CONTOUR:
      if (UndoRemoveContour (ptr))
	return (UNDO_REMOVE_CONTOUR);
      break;

    case UNDO_INSERT_CONTOUR:
      if (UndoInsertContour (ptr))
	return (UNDO_INSERT_CONTOUR);
      break;

    case UNDO_ROTATE:
      if (UndoRotate (ptr))
	return (UNDO_ROTATE);
      break;

    case UNDO_CLEAR:
      if (UndoClearPoly (ptr))
	return (UNDO_CLEAR);
      break;

    case UNDO_MOVETOLAYER:
      if (UndoMoveToLayer (ptr))
	return (UNDO_MOVETOLAYER);
      break;

    case UNDO_FLAG:
      if (UndoFlag (ptr))
	return (UNDO_FLAG);
      break;

    case UNDO_CHANGESIZE:
      if (UndoChangeSize (ptr))
	return (UNDO_CHANGESIZE);
      break;

    case UNDO_CHANGECLEARSIZE:
      if (UndoChangeClearSize (ptr))
	return (UNDO_CHANGECLEARSIZE);
      break;

    case UNDO_CHANGEMASKSIZE:
      if (UndoChangeMaskSize (ptr))
	return (UNDO_CHANGEMASKSIZE);
      break;

    case UNDO_CHANGE2NDSIZE:
      if (UndoChange2ndSize (ptr))
	return (UNDO_CHANGE2NDSIZE);
      break;

    case UNDO_CHANGEANGLES:
      if (UndoChangeAngles (ptr))
	return (UNDO_CHANGEANGLES);
      break;

    case UNDO_LAYERCHANGE:
      if (UndoLayerChange (ptr))
	return (UNDO_LAYERCHANGE);
      break;

    case UNDO_NETLISTCHANGE:
      if (UndoNetlistChange (ptr))
	return (UNDO_NETLISTCHANGE);
      break;

    case UNDO_MIRROR:
      if (UndoMirror (ptr))
	return (UNDO_MIRROR);
      break;

    case UNDO_CHANGESETVIALAYERS:
      if (UndoSetViaLayers (ptr))
        return (UNDO_CHANGESETVIALAYERS);
      break;
    }
  return 0;
}

/*!
 * \brief Redo of any 'hard to recover' operation.
 *
 * \return The number of operations redone.
 */
int
Redo (bool draw)
{
  UndoListType *ptr;
  int Types = 0;
  bool error_undoing = false;

  andDraw = draw;

  if (RedoN == 0)
    {
      Message (_("Nothing to redo. Perhaps changes have been made since last undo\n"));
      return 0;
    }

  ptr = &UndoList[UndoN];

  if (ptr->Serial < Serial)
    {
      Message (_("ERROR: Bad undo serial number %d in redo stack - expecting %d or higher\n"
                 "       Please save your work and report this bug.\n"),
               ptr->Serial, Serial);

      /* It is likely that the serial number got corrupted through some bad
       * use of the SaveUndoSerialNumber() / RestoreUndoSerialNumber() APIs.
       *
       * Reset the serial number to be consistent with that of the first
       * operation on the redo stack in the hope that this might clear
       * the problem and  allow the user to hit Redo again.
       */
      Serial = ptr->Serial;
      return 0;
    }

  LockUndo (); /* lock undo module to prevent from loops */

  /* and loop over all entries with the correct serial number */
  for (; RedoN && ptr->Serial == Serial; ptr++, UndoN++, RedoN--)
    {
      int undid = PerformUndo (ptr);
      if (undid == 0)
        error_undoing = true;
      Types |= undid;
    }

  /* Make next serial number current */
  Serial++;

  UnlockUndo ();

  if (error_undoing)
    Message (_("ERROR: Failed to redo some operations\n"));

  if (Types && andDraw)
    Draw ();

  return Types;
}

/*!
 * \brief Restores the serial number of the undo list.
 */
void
RestoreUndoSerialNumber (void)
{
  if (added_undo_between_increment_and_restore)
    Message (_("ERROR: Operations were added to the Undo stack with an incorrect serial number\n"));
  between_increment_and_restore = false;
  added_undo_between_increment_and_restore = false;
  Serial = SavedSerial;
}

/*!
 * \brief Saves the serial number of the undo list.
 */
void
SaveUndoSerialNumber (void)
{
  Bumped = false;
  between_increment_and_restore = false;
  added_undo_between_increment_and_restore = false;
  SavedSerial = Serial;
}

/*!
 * \brief Increments the serial number of the undo list.
 *
 * It's not done automatically because some operations perform more
 * than one request with the same serial #.
 */
void
IncrementUndoSerialNumber (void)
{
  if (!Locked)
    {
      /* Set the changed flag if anything was added prior to this bump */
      if (UndoN > 0 && UndoList[UndoN - 1].Serial == Serial)
        SetChangedFlag (true);
      Serial++;
      Bumped = true;
      between_increment_and_restore = true;
    }
}

/*!
 * \brief Releases memory of the undo- and remove list.
 */
void
ClearUndoList (bool Force)
{
  UndoListType *undo;

  if (UndoN
      && (Force || gui->confirm_dialog ("OK to clear 'undo' buffer?", 0)))
    {
      /* release memory allocated by objects in undo list */
      for (undo = UndoList; UndoN; undo++, UndoN--)
	{
	  if (undo->Type == UNDO_CHANGENAME)
	    free (undo->Data.ChangeName.Name);
	}
      free (UndoList);
      UndoList = NULL;
      if (RemoveList)
	{
          FreeDataMemory (RemoveList);
	  free (RemoveList);
	  RemoveList = NULL;
        }

      /* reset some counters */
      UndoN = UndoMax = RedoN = 0;
    }

  /* reset counter in any case */
  Serial = 1;
}

/*!
 * \brief Adds an object to the list of clearpoly objects.
 */
void
AddObjectToClearPolyUndoList (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
			      bool clear)
{
  UndoListType *undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_CLEAR, OBJECT_ID (Ptr3), Type);
      undo->Data.ClearPoly.Clear = clear;
      undo->Data.ClearPoly.Layer = (LayerType *) Ptr1;
    }
}

/*!
 * \brief Adds an object to the list of mirrored objects.
 */
void
AddObjectToMirrorUndoList (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
			   Coord yoff)
{
  UndoListType *undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_MIRROR, OBJECT_ID (Ptr3), Type);
      undo->Data.Move.DY = yoff;
    }
}

/*!
 * \brief Adds an object to the list of rotated objects.
 */
void
AddObjectToRotateUndoList (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
			   Coord CenterX, Coord CenterY,
			   BYTE Steps)
{
  UndoListType *undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_ROTATE, OBJECT_ID (Ptr3), Type);
      undo->Data.Rotate.CenterX = CenterX;
      undo->Data.Rotate.CenterY = CenterY;
      undo->Data.Rotate.Steps = Steps;
    }
}

/*!
 * \brief Adds an object to the list of removed objects and removes it
 * from the current PCB.
 */
void
MoveObjectToRemoveUndoList (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  if (Locked)
    return;

  if (!RemoveList)
    RemoveList = CreateNewBuffer ();

  GetUndoSlot (UNDO_REMOVE, OBJECT_ID (Ptr3), Type);
  MoveObjectToBuffer (RemoveList, PCB->Data, Type, Ptr1, Ptr2, Ptr3);
}

/*!
 * \brief Adds an object to the list of removed polygon/... points.
 */
void
AddObjectToRemovePointUndoList (int Type,
				void *Ptr1, void *Ptr2, Cardinal index)
{
  UndoListType *undo;
  PolygonType *polygon = (PolygonType *) Ptr2;
  Cardinal hole;
  bool last_in_contour = false;

  if (!Locked)
    {
      switch (Type)
	{
	case POLYGONPOINT_TYPE:
	  {
	    /* save the ID of the parent object; else it will be
	     * impossible to recover the point
	     */
	    undo =
	      GetUndoSlot (UNDO_REMOVE_POINT, OBJECT_ID (polygon),
			   POLYGON_TYPE);
	    undo->Data.RemovedPoint.X = polygon->Points[index].X;
	    undo->Data.RemovedPoint.Y = polygon->Points[index].Y;
	    undo->Data.RemovedPoint.ID = polygon->Points[index].ID;
	    undo->Data.RemovedPoint.Index = index;

	    /* Check whether this point was at the end of its contour.
	     * If so, we need to flag as such when re-adding the point
	     * so it goes back in the correct place
	     */
	    for (hole = 0; hole < polygon->HoleIndexN; hole++)
	      if (index == polygon->HoleIndex[hole] - 1)
		last_in_contour = true;
	    if (index == polygon->PointN - 1)
	      last_in_contour = true;
	    undo->Data.RemovedPoint.last_in_contour = last_in_contour;
	  }
	  break;
	}
    }
}

/*!
 * \brief Adds an object to the list of inserted polygon/... points.
 */
void
AddObjectToInsertPointUndoList (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  if (!Locked)
    GetUndoSlot (UNDO_INSERT_POINT, OBJECT_ID (Ptr3), Type);
}

static void
CopyObjectToUndoList (int undo_type, int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  UndoListType *undo;
  AnyObjectType *copy;

  if (Locked)
    return;

  if (!RemoveList)
    RemoveList = CreateNewBuffer ();

  undo = GetUndoSlot (undo_type, OBJECT_ID (Ptr2), Type);
  copy = (AnyObjectType *)CopyObjectToBuffer (RemoveList, PCB->Data, Type, Ptr1, Ptr2, Ptr3);
  undo->Data.CopyID = copy->ID;
}

/*!
 * \brief Adds an object to the list of removed contours.
 *
 * Actually just takes a copy of the whole polygon to restore.
 */
void
AddObjectToRemoveContourUndoList (int Type,
				  LayerType *Layer, PolygonType *Polygon)
{
  CopyObjectToUndoList (UNDO_REMOVE_CONTOUR, Type, Layer, Polygon, NULL);
}

/*!
 * \brief Adds an object to the list of insert contours.
 *
 * Actually just takes a copy of the whole polygon to restore.
 */
void
AddObjectToInsertContourUndoList (int Type,
				  LayerType *Layer, PolygonType *Polygon)
{
  CopyObjectToUndoList (UNDO_INSERT_CONTOUR, Type, Layer, Polygon, NULL);
}

/*!
 * \brief Adds an object to the list of moved objects.
 */
void
AddObjectToMoveUndoList (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
			 Coord DX, Coord DY)
{
  UndoListType *undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_MOVE, OBJECT_ID (Ptr3), Type);
      undo->Data.Move.DX = DX;
      undo->Data.Move.DY = DY;
    }
}

/*!
 * \brief Adds an object to the list of objects with changed names.
 */
void
AddObjectToChangeNameUndoList (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
			       char *OldName)
{
  UndoListType *undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_CHANGENAME, OBJECT_ID (Ptr3), Type);
      undo->Data.ChangeName.Name = OldName;
    }
}

/*!
 * \brief Adds an object to the list of objects moved to another layer.
 */
void
AddObjectToMoveToLayerUndoList (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  UndoListType *undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_MOVETOLAYER, OBJECT_ID (Ptr3), Type);
      undo->Data.MoveToLayer.OriginalLayer =
	GetLayerNumber (PCB->Data, (LayerType *) Ptr1);
    }
}

/*!
 * \brief Adds an object to the list of created objects.
 */
void
AddObjectToCreateUndoList (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  if (!Locked)
    GetUndoSlot (UNDO_CREATE, OBJECT_ID (Ptr3), Type);
  ClearFromPolygon (PCB->Data, Type, Ptr1, Ptr2);
}

/*!
 * \brief Adds an object to the list of objects with flags changed.
 */
void
AddObjectToFlagUndoList (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  UndoListType *undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_FLAG, OBJECT_ID (Ptr2), Type);
      undo->Data.Flags = ((PinType *) Ptr2)->Flags;
    }
}

/*!
 * \brief Adds an object to the list of objects with Size changes.
 */
void
AddObjectToSizeUndoList (int Type, void *ptr1, void *ptr2, void *ptr3)
{
  UndoListType *undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_CHANGESIZE, OBJECT_ID (ptr2), Type);
      switch (Type)
	{
	case PIN_TYPE:
	case VIA_TYPE:
	  undo->Data.Size = ((PinType *) ptr2)->Thickness;
	  break;
	case LINE_TYPE:
	case ELEMENTLINE_TYPE:
	  undo->Data.Size = ((LineType *) ptr2)->Thickness;
	  break;
	case TEXT_TYPE:
	case ELEMENTNAME_TYPE:
	  undo->Data.Scale = ((TextType *) ptr2)->Scale;
	  break;
	case PAD_TYPE:
	  undo->Data.Size = ((PadType *) ptr2)->Thickness;
	  break;
	case ARC_TYPE:
	case ELEMENTARC_TYPE:
	  undo->Data.Size = ((ArcType *) ptr2)->Thickness;
	  break;
	}
    }
}

/*!
 * \brief Adds an object to the list of objects with Size changes.
 */
void
AddObjectToClearSizeUndoList (int Type, void *ptr1, void *ptr2, void *ptr3)
{
  UndoListType *undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_CHANGECLEARSIZE, OBJECT_ID (ptr2), Type);
      switch (Type)
	{
	case PIN_TYPE:
	case VIA_TYPE:
	  undo->Data.Size = ((PinType *) ptr2)->Clearance;
	  break;
	case LINE_TYPE:
	  undo->Data.Size = ((LineType *) ptr2)->Clearance;
	  break;
	case PAD_TYPE:
	  undo->Data.Size = ((PadType *) ptr2)->Clearance;
	  break;
	case ARC_TYPE:
	  undo->Data.Size = ((ArcType *) ptr2)->Clearance;
	  break;
	}
    }
}

/*!
 * \brief Adds an object to the list of objects with Size changes.
 */
void
AddObjectToMaskSizeUndoList (int Type, void *ptr1, void *ptr2, void *ptr3)
{
  UndoListType *undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_CHANGEMASKSIZE, OBJECT_ID (ptr2), Type);
      switch (Type)
	{
	case PIN_TYPE:
	case VIA_TYPE:
	  undo->Data.Size = ((PinType *) ptr2)->Mask;
	  break;
	case PAD_TYPE:
	  undo->Data.Size = ((PadType *) ptr2)->Mask;
	  break;
	}
    }
}

/*!
 * \brief Adds an object to the list of objects with 2ndSize changes.
 */
void
AddObjectTo2ndSizeUndoList (int Type, void *ptr1, void *ptr2, void *ptr3)
{
  UndoListType *undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_CHANGE2NDSIZE, OBJECT_ID (ptr2), Type);
      if (Type == PIN_TYPE || Type == VIA_TYPE)
	undo->Data.Size = ((PinType *) ptr2)->DrillingHole;
    }
}

/*!
 * \brief Adds an object to the list of changed angles.
 *
 * \note You must call this before changing the angles, passing the new
 * start/delta.
 */
void
AddObjectToChangeAnglesUndoList (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  UndoListType *undo;
  ArcType *a = (ArcType *) Ptr3;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_CHANGEANGLES, OBJECT_ID (Ptr3), Type);
      undo->Data.Move.DX = a->StartAngle;
      undo->Data.Move.DY = a->Delta;
    }
}

/*!
 * \brief Adds a layer change (new, delete, move) to the undo list.
 */
void
AddLayerChangeToUndoList (int old_index, int new_index)
{
  UndoListType *undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_LAYERCHANGE, 0, 0);
      undo->Data.LayerChange.old_index = old_index;
      undo->Data.LayerChange.new_index = new_index;
    }
}

/*!
 * \brief Adds a netlist change to the undo list.
 */
void
AddNetlistLibToUndoList (LibraryType *lib)
{
  UndoListType *undo;
  unsigned int i, j;
  LibraryType *old;
  
  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_NETLISTCHANGE, 0, 0);
      /* keep track of where the data needs to go */
      undo->Data.NetlistChange.lib = lib;

      /* and what the old data is that we'll need to restore */
      undo->Data.NetlistChange.old = (LibraryType *)malloc (sizeof (LibraryType));
      old = undo->Data.NetlistChange.old;
      old->MenuN = lib->MenuN;
      old->MenuMax = lib->MenuMax;
      old->Menu = (LibraryMenuType *)malloc (old->MenuMax * sizeof (LibraryMenuType));
      if (old->Menu == NULL)
	{
	  fprintf (stderr, "malloc() failed in %s\n", __FUNCTION__);
	  exit (1);
	}

      /* iterate over each net */
      for (i = 0 ; i < lib->MenuN; i++)
	{
	  old->Menu[i].EntryN = lib->Menu[i].EntryN;
	  old->Menu[i].EntryMax = lib->Menu[i].EntryMax;

	  old->Menu[i].Name = 
	    lib->Menu[i].Name ? strdup (lib->Menu[i].Name) : NULL;
	  
	  old->Menu[i].directory = 
	    lib->Menu[i].directory ? strdup (lib->Menu[i].directory) : NULL;
	  
	  old->Menu[i].Style = 
	    lib->Menu[i].Style ? strdup (lib->Menu[i].Style) : NULL;

      
	  old->Menu[i].Entry = 
	    (LibraryEntryType *)malloc (old->Menu[i].EntryMax * sizeof (LibraryEntryType));
	  if (old->Menu[i].Entry == NULL)
	    {
	      fprintf (stderr, "malloc() failed in %s\n", __FUNCTION__);
	      exit (1);
	    }
	  
	  /* iterate over each pin on the net */
	  for (j = 0; j < lib->Menu[i].EntryN; j++) {

	    old->Menu[i].Entry[j].ListEntry = 
	      lib->Menu[i].Entry[j].ListEntry ? 
	      strdup (lib->Menu[i].Entry[j].ListEntry) :
	      NULL;

	    old->Menu[i].Entry[j].AllocatedMemory = 
	      lib->Menu[i].Entry[j].AllocatedMemory ? 
	      strdup (lib->Menu[i].Entry[j].AllocatedMemory) :
	      NULL;

	    old->Menu[i].Entry[j].Template = 
	      lib->Menu[i].Entry[j].Template ? 
	      strdup (lib->Menu[i].Entry[j].Template) :
	      NULL;

	    old->Menu[i].Entry[j].Package = 
	      lib->Menu[i].Entry[j].Package ? 
	      strdup (lib->Menu[i].Entry[j].Package) :
	      NULL;

	    old->Menu[i].Entry[j].Value = 
	      lib->Menu[i].Entry[j].Value ? 
	      strdup (lib->Menu[i].Entry[j].Value) :
	      NULL;

	    old->Menu[i].Entry[j].Description = 
	      lib->Menu[i].Entry[j].Description ? 
	      strdup (lib->Menu[i].Entry[j].Description) :
	      NULL;
	    

	  }
	}

    }
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of objects with buried info data
 */
void
AddObjectToSetViaLayersUndoList (void *ptr1, void *ptr2, void *ptr3)
{
  UndoListType *undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_CHANGESETVIALAYERS, OBJECT_ID (ptr2), VIA_TYPE);
      undo->Data.SetViaLayersChange.from = ((PinType *) ptr2)->BuriedFrom;
      undo->Data.SetViaLayersChange.to = ((PinType *) ptr2)->BuriedTo;
    }
}


/* ---------------------------------------------------------------------------
 * \brief Set lock flag
 */
void
LockUndo (void)
{
  Locked = true;
}

/*!
 * \brief Reset lock flag.
 */
void
UnlockUndo (void)
{
  Locked = false;
}

/*!
 * \brief Return undo lock state.
 */
bool
Undoing (void)
{
  return (Locked);
}
