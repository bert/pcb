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
#define DEBUG_ID 1
static char *rcsid = "$Id$";

/* functions used to undo operations
 *
 * Description:
 * There are two lists which hold
 *   - information about a command
 *   - data of removed objects
 * Both lists are organized as first-in-last-out which means that the undo
 * list can always use the last entry of the remove list.
 * A serial number is incremented whenever an operation is completed.
 * An operation itself may consist of several basic instructions.
 * E.g.: removing all selected objects is one operation with exactly one
 * serial number even if the remove function is called several times.
 *
 * a lock flag ensures that no infinite loops occure
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
#include "dialog.h"
#include "draw.h"
#include "error.h"
#include "insert.h"
#include "misc.h"
#include "mirror.h"
#include "move.h"
#include "mymem.h"
#include "remove.h"
#include "rotate.h"
#include "search.h"
#include "set.h"
#include "undo.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some local data types
 */
typedef struct			/* information about a change command */
{
  char *Name;
}
ChangeNameType, *ChangeNameTypePtr;

typedef struct			/* information about a move command */
{
  Position DX,			/* movement vector */
    DY;
}
MoveType, *MoveTypePtr;

typedef struct			/* information about removed polygon points */
{
  Position X, Y;		/* data */
  int ID;
  Cardinal Index;		/* index in a polygons array of points */
}
RemovedPointType, *RemovedPointTypePtr;

typedef struct			/* informstion about rotation */
{
  Position CenterX,		/* center of rotation */
    CenterY;
  BYTE Steps;			/* number of steps */
}
RotateType, *RotateTypePtr;

typedef struct			/* information about moves between layers */
{
  Cardinal OriginalLayer;	/* the index of the original layer */
}
MoveToLayerType, *MoveToLayerTypePtr;

typedef struct			/* holds information about an operation */
{
  int Serial,			/* serial number of operation */
    Type,			/* type of operation */
    Kind,			/* type of object with given ID */
    ID;				/* object ID */
  union				/* some additional information */
  {
    ChangeNameType ChangeName;
    MoveType Move;
    RemovedPointType RemovedPoint;
    RotateType Rotate;
    MoveToLayerType MoveToLayer;
    int Flags;
    Dimension Size;
  }
  Data;
}
UndoListType, *UndoListTypePtr;

/* ---------------------------------------------------------------------------
 * some local variables
 */
static DataTypePtr RemoveList;	/* list of removed objects */
static UndoListTypePtr UndoList;	/* list of operations */
static int Serial,		/* serial number */
  SavedSerial;
static size_t UndoN, RedoN,	/* number of entries */
  UndoMax;
static Boolean Locked = False;	/* do not add entries if */
static Boolean andDraw = True;
										/* flag is set; prevents from */
										/* infinite loops */

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static UndoListTypePtr GetUndoSlot (int, int, int);
static void DrawRecoveredObject (int, void *, void *, void *);
static Boolean UndoRotate (UndoListTypePtr);
static Boolean UndoChangeName (UndoListTypePtr);
static Boolean UndoCopyOrCreate (UndoListTypePtr);
static Boolean UndoMove (UndoListTypePtr);
static Boolean UndoRemove (UndoListTypePtr);
static Boolean UndoRemovePoint (UndoListTypePtr);
static Boolean UndoInsertPoint (UndoListTypePtr);
static Boolean UndoMoveToLayer (UndoListTypePtr);
static Boolean UndoFlag (UndoListTypePtr);
static Boolean UndoMirror (UndoListTypePtr);
static Boolean UndoChangeSize (UndoListTypePtr);
static Boolean UndoChange2ndSize (UndoListTypePtr);
static Boolean UndoChangeClearSize (UndoListTypePtr);
static Boolean UndoChangeMaskSize (UndoListTypePtr);
static int PerformUndo (UndoListTypePtr);

/* ---------------------------------------------------------------------------
 * adds a command plus some data to the undo list
 */
static UndoListTypePtr
GetUndoSlot (int CommandType, int ID, int Kind)
{
  UndoListTypePtr ptr;
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
      UndoList = (UndoListTypePtr) MyRealloc (UndoList, size,
					      "AddCommandToUndoList()");
      memset (&UndoList[UndoN], 0, STEP_REMOVELIST * sizeof (UndoListType));

      /* ask user to flush the table because of it's size */
      if (size > limit)
	{
	  limit = (size / UNDO_WARNING_SIZE + 1) * UNDO_WARNING_SIZE;
	  Message ("size of 'undo-list' exceeds %li kb\n",
		   (long) (size >> 10));
	}
    }

  /* free structures from the pruned redo list */

  for (ptr = &UndoList[UndoN]; RedoN; ptr++, RedoN--)
    switch (ptr->Type)
      {
      case UNDO_CHANGENAME:
	SaveFree (ptr->Data.ChangeName.Name);
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

  /* copy typefield and serial number to the list */
  ptr = &UndoList[UndoN++];
  ptr->Type = CommandType;
  ptr->Kind = Kind;
  ptr->ID = ID;
  ptr->Serial = Serial;
  return (ptr);
}

/* ---------------------------------------------------------------------------
 * redraws the recovered object
 */
static void
DrawRecoveredObject (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  if (Type & (LINE_TYPE | TEXT_TYPE | POLYGON_TYPE | ARC_TYPE))
    {
      LayerTypePtr layer;

      layer = LAYER_PTR (GetLayerNumber (RemoveList, (LayerTypePtr) Ptr1));
      DrawObject (Type, (void *) layer, Ptr2, 0);
    }
  else
    DrawObject (Type, Ptr1, Ptr2, 0);
}

/* ---------------------------------------------------------------------------
 * recovers an object from a 'rotate' operation
 * returns True if anything has been recovered
 */
static Boolean
UndoRotate (UndoListTypePtr Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;

  /* lookup entry by it's ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type != NO_TYPE)
    {
      if (TEST_FLAG (LOCKFLAG, (ArcTypePtr) ptr2))
	return (False);
      RotateObject (type, ptr1, ptr2, ptr3,
		    Entry->Data.Rotate.CenterX, Entry->Data.Rotate.CenterY,
		    (4 - Entry->Data.Rotate.Steps) & 0x03);
      Entry->Data.Rotate.Steps = (4 - Entry->Data.Rotate.Steps) & 0x03;
      return (True);
    }
  return (False);
}

/* ---------------------------------------------------------------------------
 * recovers an object from a 'change name' operation
 * returns True if anything has been recovered
 */
static Boolean
UndoChangeName (UndoListTypePtr Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;

  /* lookup entry by it's ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type != NO_TYPE)
    {
      if (TEST_FLAG (LOCKFLAG, (TextTypePtr) ptr3))
	return (False);
      Entry->Data.ChangeName.Name =
	(ChangeObjectName (type, ptr1, ptr2, ptr3,
			   Entry->Data.ChangeName.Name));
      return (True);
    }
  return (False);
}

/* ---------------------------------------------------------------------------
 * recovers an object from a 2ndSize change operation
 */
static Boolean
UndoChange2ndSize (UndoListTypePtr Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;
  Dimension swap;

  /* lookup entry by ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type != NO_TYPE)
    {
      if (TEST_FLAG (LOCKFLAG, (PinTypePtr) ptr2))
	return (False);
      swap = ((PinTypePtr) ptr2)->DrillingHole;
      if (andDraw)
	EraseObject (type, ptr2);
      ((PinTypePtr) ptr2)->DrillingHole = Entry->Data.Size;
      Entry->Data.Size = swap;
      DrawObject (type, ptr1, ptr2, 0);
      return (True);
    }
  return (False);
}

/* ---------------------------------------------------------------------------
 * recovers an object from a clearance size change operation
 */
static Boolean
UndoChangeClearSize (UndoListTypePtr Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;
  Dimension swap;

  /* lookup entry by ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type != NO_TYPE)
    {
      if (TEST_FLAG (LOCKFLAG, (LineTypePtr) ptr2))
	return (False);
      swap = ((PinTypePtr) ptr2)->Clearance;
      if (andDraw)
	EraseObject (type, ptr2);
      ((PinTypePtr) ptr2)->Clearance = Entry->Data.Size;
      Entry->Data.Size = swap;
      if (andDraw)
	DrawObject (type, ptr1, ptr2, 0);
      return (True);
    }
  return (False);
}

/* ---------------------------------------------------------------------------
 * recovers an object from a mask size change operation
 */
static Boolean
UndoChangeMaskSize (UndoListTypePtr Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;
  Dimension swap;

  /* lookup entry by ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type & (VIA_TYPE | PIN_TYPE | PAD_TYPE))
    {
      if (TEST_FLAG (LOCKFLAG, (PinTypePtr) ptr2))
	return (False);
      swap = ((PinTypePtr) ptr2)->Mask;
      if (andDraw)
	EraseObject (type, ptr2);
      ((PinTypePtr) ptr2)->Mask = Entry->Data.Size;
      Entry->Data.Size = swap;
      if (andDraw)
	DrawObject (type, ptr1, ptr2, 0);
      return (True);
    }
  return (False);
}


/* ---------------------------------------------------------------------------
 * recovers an object from a Size change operation
 */
static Boolean
UndoChangeSize (UndoListTypePtr Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;
  Dimension swap;

  /* lookup entry by ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type != NO_TYPE)
    {
      if (TEST_FLAG (LOCKFLAG, (PinTypePtr) ptr2))
	return (False);
      swap = ((PinTypePtr) ptr2)->Thickness;
      if (andDraw)
	EraseObject (type, ptr2);
      ((PinTypePtr) ptr2)->Thickness = Entry->Data.Size;
      Entry->Data.Size = swap;
      if (andDraw)
	DrawObject (type, ptr1, ptr2, 0);
      return (True);
    }
  return (False);
}

/* ---------------------------------------------------------------------------
 * recovers an object from a FLAG change operation
 */
static Boolean
UndoFlag (UndoListTypePtr Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type, swap, tests;

  /* lookup entry by ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type != NO_TYPE)
    {
      if (TEST_FLAG (LOCKFLAG, (PinTypePtr) ptr2))
	return (False);
      swap = tests = ((PinTypePtr) ptr2)->Flags;
      tests ^= Entry->Data.Flags;
      if (andDraw && (tests & DRAW_FLAGS & ~(SELECTEDFLAG | FOUNDFLAG)))
	EraseObject (type, ptr2);
      ((PinTypePtr) ptr2)->Flags = Entry->Data.Flags;
      Entry->Data.Flags = swap;
      if (andDraw && (tests & DRAW_FLAGS))
	DrawObject (type, ptr1, ptr2, 0);
      return (True);
    }
  Message ("hace Internal error: Can't find ID %d type %08x\n", Entry->ID,
	   Entry->Kind);
  Message ("for UndoFlag Operation. Previous flags: %08x\n",
	   Entry->Data.Flags);
  return (False);
}

/* ---------------------------------------------------------------------------
 * recovers an object from a mirror operation
 * returns True if anything has been recovered
 */
static Boolean
UndoMirror (UndoListTypePtr Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;

  /* lookup entry by ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type == ELEMENT_TYPE)
    {
      ElementTypePtr element = (ElementTypePtr) ptr3;
      if (TEST_FLAG (LOCKFLAG, element))
	return (False);
      if (andDraw)
	EraseElement (element);
      MirrorElementCoordinates (element, Entry->Data.Move.DY);
      if (andDraw)
	DrawElement (element, 0);
      return (True);
    }
  Message ("hace Internal error: UndoMirror on object type %d\n", type);
  return (False);
}

/* ---------------------------------------------------------------------------
 * recovers an object from a 'copy' or 'create' operation
 * returns True if anything has been recovered
 */
static Boolean
UndoCopyOrCreate (UndoListTypePtr Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;

  /* lookup entry by it's ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type != NO_TYPE)
    {
      if (TEST_FLAG (LOCKFLAG, (PinTypePtr) ptr2))
	return (False);
      if (!RemoveList)
	RemoveList = CreateNewBuffer ();
      if (andDraw)
	EraseObject (type, ptr2);
      /* in order to make this re-doable we move it to the RemoveList */
      MoveObjectToBuffer (RemoveList, PCB->Data, type, ptr1, ptr2, ptr3);
      Entry->Type = UNDO_REMOVE;
      return (True);
    }
  return (False);
}

/* ---------------------------------------------------------------------------
 * recovers an object from a 'move' operation
 * returns True if anything has been recovered
 */
static Boolean
UndoMove (UndoListTypePtr Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type;

  /* lookup entry by it's ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type != NO_TYPE)
    {
      if (TEST_FLAG (LOCKFLAG, (LineTypePtr) ptr2))
	return (False);
      MoveObject (type, ptr1, ptr2, ptr3,
		  -Entry->Data.Move.DX, -Entry->Data.Move.DY);
      Entry->Data.Move.DX *= -1;
      Entry->Data.Move.DY *= -1;
      return (True);
    }
  return (False);
}

/* ----------------------------------------------------------------------
 * recovers an object from a 'remove' operation
 * returns True if anything has been recovered
 */
static Boolean
UndoRemove (UndoListTypePtr Entry)
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
      return (True);
    }
  return (False);
}

/* ----------------------------------------------------------------------
 * recovers an object from a 'move to another layer' operation
 * returns True if anything has been recovered
 */
static Boolean
UndoMoveToLayer (UndoListTypePtr Entry)
{
  void *ptr1, *ptr2, *ptr3;
  int type, swap;

  /* lookup entry by it's ID */
  type =
    SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
  if (type != NO_TYPE)
    {
      if (TEST_FLAG (LOCKFLAG, (LineTypePtr) ptr2))
	return (False);
      swap = GetLayerNumber (PCB->Data, (LayerTypePtr) ptr1);
      MoveObjectToLayer (type, ptr1, ptr2, ptr3,
			 LAYER_PTR (Entry->Data.
				    MoveToLayer.OriginalLayer), True);
      Entry->Data.MoveToLayer.OriginalLayer = swap;
      return (True);
    }
  return (False);
}

/* ---------------------------------------------------------------------------
 * recovers a removed polygon point
 * returns true on success
 */
static Boolean
UndoRemovePoint (UndoListTypePtr Entry)
{
  LayerTypePtr layer;
  PolygonTypePtr polygon;
  void *ptr3;
  int type;

  /* lookup entry (polygon not point was saved) by it's ID */
  assert (Entry->Kind == POLYGON_TYPE);
  type =
    SearchObjectByID (PCB->Data, (void *) &layer, (void *) &polygon, &ptr3,
		      Entry->ID, Entry->Kind);
  switch (type)
    {
    case POLYGON_TYPE:		/* restore the removed point */
      {
	if (TEST_FLAG (LOCKFLAG, polygon))
	  return (False);
	/* recover the point */
	if (andDraw && layer->On)
	  ErasePolygon (polygon);
	InsertPointIntoObject (POLYGON_TYPE, layer, polygon,
			       &Entry->Data.RemovedPoint.Index,
			       Entry->Data.RemovedPoint.X,
			       Entry->Data.RemovedPoint.Y, True);
	polygon->Points[Entry->Data.RemovedPoint.Index].ID =
	  Entry->Data.RemovedPoint.ID;
	if (andDraw && layer->On)
	  DrawPolygon (layer, polygon, 0);
	Entry->Type = UNDO_INSERT_POINT;
	Entry->ID = Entry->Data.RemovedPoint.ID;
	Entry->Kind = POLYGONPOINT_TYPE;
	return (True);
      }

    default:
      return (False);
    }
}

/* ---------------------------------------------------------------------------
 * recovers an inserted polygon point
 * returns true on success
 */
static Boolean
UndoInsertPoint (UndoListTypePtr Entry)
{
  LayerTypePtr layer;
  PolygonTypePtr polygon;
  PointTypePtr pnt;
  int type;

  assert (Entry->Kind == POLYGONPOINT_TYPE);
  /* lookup entry by it's ID */
  type =
    SearchObjectByID (PCB->Data, (void *) &layer, (void *) &polygon,
		      (void *) &pnt, Entry->ID, Entry->Kind);
  switch (type)
    {
    case POLYGONPOINT_TYPE:	/* removes an inserted polygon point */
      {
	if (TEST_FLAG (LOCKFLAG, polygon))
	  return (False);
	if (andDraw && layer->On)
	  ErasePolygon (polygon);
	Entry->Data.RemovedPoint.X = pnt->X;
	Entry->Data.RemovedPoint.Y = pnt->Y;
	Entry->Data.RemovedPoint.ID = pnt->ID;
	Entry->ID = polygon->ID;
	Entry->Kind = POLYGON_TYPE;
	Entry->Type = UNDO_REMOVE_POINT;
	POLYGONPOINT_LOOP (polygon, 
	  {
	    if (pnt == point)
	      {
		Entry->Data.RemovedPoint.Index = n;
		break;
	      }
	  }
	);
	DestroyObject (PCB->Data, POLYGONPOINT_TYPE, layer, polygon, pnt);
	if (andDraw && layer->On)
	  DrawPolygon (layer, polygon, 0);
	return (True);
      }

    default:
      return (False);
    }
}

/* ---------------------------------------------------------------------------
 * undo of any 'hard to recover' operation
 *
 * returns the bitfield for the types of operations that were undone
 */
int
Undo (Boolean draw)
{
  UndoListTypePtr ptr;
  int Types = 0;

  andDraw = draw;

  do
    {
      if (!UndoN)
	{
	  if (!Serial)
	    Message ("Nothing to undo - buffer is empty\n");
	  else
	    Serial--;
	  return (False);
	}

      /* lock undo module to prevent from loops
       * and loop over all entries with the same serial number
       */
      ptr = &UndoList[UndoN - 1];
      if (ptr->Serial != Serial - 1)
	{
	  Message ("hace undo bad serial number %d expecting %d\n",
		   ptr->Serial, Serial - 1);
	  Serial = ptr->Serial + 1;
	  return (False);
	}
      LockUndo ();
      Serial = ptr->Serial;
      for (; UndoN && ptr->Serial == Serial; ptr--, UndoN--, RedoN++)
	Types |= PerformUndo (ptr);
      /* release lock */
      UnlockUndo ();
    }
  while (Types == 0);
  if (Types && andDraw)
    Draw ();
  return (Types);
}

static int
PerformUndo (UndoListTypePtr ptr)
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

    case UNDO_ROTATE:
      if (UndoRotate (ptr))
	return (UNDO_ROTATE);
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

    case UNDO_MIRROR:
      if (UndoMirror (ptr))
	return (UNDO_MIRROR);
      break;
    }
  return 0;
}

/* ---------------------------------------------------------------------------
 * redo of any 'hard to recover' operation
 *
 * returns the number of operations redone
 */
int
Redo (Boolean draw)
{
  UndoListTypePtr ptr;
  int Types = 0;

  andDraw = draw;
  do
    {
      if (!RedoN)
	{
	  Message
	    ("Nothing to redo. Perhaps changes have been made since last undo\n");
	  return (False);
	}

      /* lock undo module to prevent from loops
       * and loop over all entries with the same serial number
       */
      LockUndo ();
      ptr = &UndoList[UndoN];
      Serial = ptr->Serial;
      for (; RedoN && ptr->Serial == Serial; ptr++, UndoN++, RedoN--)
	Types |= PerformUndo (ptr);
      /* Make next serial number current in case we take a new branch */
      Serial++;
      UnlockUndo ();
    }
  while (Types == 0);
  if (Types && andDraw)
    Draw ();
  return (Types);
}

/* ---------------------------------------------------------------------------
 * restores the serial number of the undo list
 */
void
RestoreUndoSerialNumber (void)
{
  Serial = SavedSerial;
}

/* ---------------------------------------------------------------------------
 * saves the serial number of the undo list
 */
void
SaveUndoSerialNumber (void)
{
  Bumped = False;
  SavedSerial = Serial;
}

/* ---------------------------------------------------------------------------
 * increments the serial number of the undo list
 * it's not done automatically because some operations perform more
 * than one request with the same serial #
 */
void
IncrementUndoSerialNumber (void)
{
  if (!Locked)
    {
      Serial++;
      Bumped = True;
      SetChangedFlag (True);
    }
}

/* ---------------------------------------------------------------------------
 * releases memory of the undo- and remove list
 */
void
ClearUndoList (Boolean Force)
{
  UndoListTypePtr undo;

  if (UndoN && (Force || ConfirmDialog ("OK to clear 'undo' buffer ?")))
    {
      /* release memory allocated by objects in undo list */
      for (undo = UndoList; UndoN; undo++, UndoN--)
	{
	  if (undo->Type == UNDO_CHANGENAME)
	    SaveFree (undo->Data.ChangeName.Name);
	}
      MyFree ((char **) &UndoList);
      FreeDataMemory (RemoveList);

      /* reset some counters */
      UndoN = UndoMax = RedoN = 0;
    }

  /* reset counter in any case */
  Serial = 0;
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of mirrored objects
 */
void
AddObjectToMirrorUndoList (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
			   Position yoff)
{
  UndoListTypePtr undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_MIRROR, OBJECT_ID (Ptr3), Type);
      undo->Data.Move.DY = yoff;
    }
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of rotated objects
 */
void
AddObjectToRotateUndoList (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
			   Position CenterX, Position CenterY, BYTE Steps)
{
  UndoListTypePtr undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_ROTATE, OBJECT_ID (Ptr3), Type);
      undo->Data.Rotate.CenterX = CenterX;
      undo->Data.Rotate.CenterY = CenterY;
      undo->Data.Rotate.Steps = Steps;
    }
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of removed objects and removes it from
 * the current PCB
 */
void
MoveObjectToRemoveUndoList (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  UndoListTypePtr undo;

  if (!Locked)
    {
      if (!RemoveList)
	RemoveList = CreateNewBuffer ();

      undo = GetUndoSlot (UNDO_REMOVE, OBJECT_ID (Ptr3), Type);
      MoveObjectToBuffer (RemoveList, PCB->Data, Type, Ptr1, Ptr2, Ptr3);
    }
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of removed polygon/... points
 */
void
AddObjectToRemovePointUndoList (int Type,
				void *Ptr1, void *Ptr2, Cardinal index)
{
  UndoListTypePtr undo;
  PolygonTypePtr polygon = (PolygonTypePtr) Ptr2;

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
	  }
	  break;
	}
    }
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of inserted polygon/... points
 */
void
AddObjectToInsertPointUndoList (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  UndoListTypePtr undo;

  if (!Locked)
    undo = GetUndoSlot (UNDO_INSERT_POINT, OBJECT_ID (Ptr3), Type);
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of moved objects
 */
void
AddObjectToMoveUndoList (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
			 Position DX, Position DY)
{
  UndoListTypePtr undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_MOVE, OBJECT_ID (Ptr3), Type);
      undo->Data.Move.DX = DX;
      undo->Data.Move.DY = DY;
    }
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of objects with changed names
 */
void
AddObjectToChangeNameUndoList (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
			       char *OldName)
{
  UndoListTypePtr undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_CHANGENAME, OBJECT_ID (Ptr3), Type);
      undo->Data.ChangeName.Name = OldName;
    }
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of objects moved to another layer
 */
void
AddObjectToMoveToLayerUndoList (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  UndoListTypePtr undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_MOVETOLAYER, OBJECT_ID (Ptr3), Type);
      undo->Data.MoveToLayer.OriginalLayer =
	GetLayerNumber (PCB->Data, (LayerTypePtr) Ptr1);
    }
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of created objects
 */
void
AddObjectToCreateUndoList (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  UndoListTypePtr undo;

  if (!Locked)
    undo = GetUndoSlot (UNDO_CREATE, OBJECT_ID (Ptr3), Type);
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of objects with flags changed
 */
void
AddObjectToFlagUndoList (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  UndoListTypePtr undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_FLAG, OBJECT_ID (Ptr2), Type);
      undo->Data.Flags = ((PinTypePtr) Ptr2)->Flags;
    }
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of objects with Size changes
 */
void
AddObjectToSizeUndoList (int Type, void *ptr1, void *ptr2, void *ptr3)
{
  UndoListTypePtr undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_CHANGESIZE, OBJECT_ID (ptr2), Type);
      switch (Type)
	{
	case PIN_TYPE:
	case VIA_TYPE:
	  undo->Data.Size = ((PinTypePtr) ptr2)->Thickness;
	  break;
	case LINE_TYPE:
	case ELEMENTLINE_TYPE:
	  undo->Data.Size = ((LineTypePtr) ptr2)->Thickness;
	  break;
	case TEXT_TYPE:
	case ELEMENTNAME_TYPE:
	  undo->Data.Size = ((TextTypePtr) ptr2)->Scale;
	  break;
	case PAD_TYPE:
	  undo->Data.Size = ((PadTypePtr) ptr2)->Thickness;
	  break;
	case ARC_TYPE:
	case ELEMENTARC_TYPE:
	  undo->Data.Size = ((ArcTypePtr) ptr2)->Thickness;
	  break;
	}
    }
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of objects with Size changes
 */
void
AddObjectToClearSizeUndoList (int Type, void *ptr1, void *ptr2, void *ptr3)
{
  UndoListTypePtr undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_CHANGECLEARSIZE, OBJECT_ID (ptr2), Type);
      switch (Type)
	{
	case PIN_TYPE:
	case VIA_TYPE:
	  undo->Data.Size = ((PinTypePtr) ptr2)->Clearance;
	  break;
	case LINE_TYPE:
	  undo->Data.Size = ((LineTypePtr) ptr2)->Clearance;
	  break;
	case PAD_TYPE:
	  undo->Data.Size = ((PadTypePtr) ptr2)->Clearance;
	  break;
	case ARC_TYPE:
	  undo->Data.Size = ((ArcTypePtr) ptr2)->Clearance;
	  break;
	}
    }
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of objects with Size changes
 */
void
AddObjectToMaskSizeUndoList (int Type, void *ptr1, void *ptr2, void *ptr3)
{
  UndoListTypePtr undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_CHANGEMASKSIZE, OBJECT_ID (ptr2), Type);
      switch (Type)
	{
	case PIN_TYPE:
	case VIA_TYPE:
	  undo->Data.Size = ((PinTypePtr) ptr2)->Mask;
	  break;
	case PAD_TYPE:
	  undo->Data.Size = ((PadTypePtr) ptr2)->Mask;
	  break;
	}
    }
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of objects with 2ndSize changes
 */
void
AddObjectTo2ndSizeUndoList (int Type, void *ptr1, void *ptr2, void *ptr3)
{
  UndoListTypePtr undo;

  if (!Locked)
    {
      undo = GetUndoSlot (UNDO_CHANGE2NDSIZE, OBJECT_ID (ptr2), Type);
      if (Type == PIN_TYPE || Type == VIA_TYPE)
	undo->Data.Size = ((PinTypePtr) ptr2)->DrillingHole;
    }
}

/* ---------------------------------------------------------------------------
 * set lock flag
 */
void
LockUndo (void)
{
  Locked = True;
}

/* ---------------------------------------------------------------------------
 * reset lock flag
 */
void
UnlockUndo (void)
{
  Locked = False;
}

/* ---------------------------------------------------------------------------
 * return undo lock state
 */
Boolean
Undoing (void)
{
  return (Locked);
}
