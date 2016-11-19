/*!
 * \file src/action.c
 *
 * \brief Action routines for output window.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
 *
 * Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact addresses for paper mail and Email:
 * Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 * haceaton@aplcomm.jhuapl.edu
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"

#include "action.h"
#include "autoplace.h"
#include "autoroute.h"
#include "buffer.h"
#include "change.h"
#include "copy.h"
#include "create.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "file.h"
#include "find.h"
#include "hid.h"
#include "insert.h"
#include "line.h"
#include "mymem.h"
#include "misc.h"
#include "mirror.h"
#include "move.h"
#include "polygon.h"
/*#include "print.h"*/
#include "rats.h"
#include "remove.h"
#include "report.h"
#include "rotate.h"
#include "rubberband.h"
#include "search.h"
#include "select.h"
#include "set.h"
#include "thermal.h"
#include "undo.h"
#include "rtree.h"
#include "macro.h"
#include "pcb-printf.h"

#include <assert.h>
#include <stdlib.h> /* rand() */

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* for fork() and friends */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

/* ---------------------------------------------------------------------------
 * some local types
 */
typedef enum
{
  F_AddSelected,
  F_All,
  F_AllConnections,
  F_AllRats,
  F_AllUnusedPins,
  F_Arc,
  F_Arrow,
  F_Block,
  F_Description,
  F_Cancel,
  F_Center,
  F_Clear,
  F_ClearAndRedraw,
  F_ClearList,
  F_Close,
  F_Found,
  F_Connection,
  F_Convert,
  F_Copy,
  F_CycleClip,
  F_CycleCrosshair,
  F_DeleteRats,
  F_Drag,
  F_DrillReport,
  F_Element,
  F_ElementByName,
  F_ElementConnections,
  F_ElementToBuffer,
  F_Escape,
  F_Find,
  F_FlipElement,
  F_FoundPins,
  F_Grid,
  F_InsertPoint,
  F_Layer,
  F_Layout,
  F_LayoutAs,
  F_LayoutToBuffer,
  F_Line,
  F_LineSize,
  F_Lock,
  F_Mirror,
  F_Move,
  F_NameOnPCB,
  F_Netlist,
  F_NetByName,
  F_None,
  F_Notify,
  F_Object,
  F_ObjectByName,
  F_PasteBuffer,
  F_PadByName,
  F_PinByName,
  F_PinOrPadName,
  F_Pinout,
  F_Polygon,
  F_PolygonHole,
  F_PreviousPoint,
  F_RatsNest,
  F_Rectangle,
  F_Redraw,
  F_Release,
  F_Revert,
  F_Remove,
  F_RemoveSelected,
  F_Report,
  F_Reset,
  F_ResetLinesAndPolygons,
  F_ResetPinsViasAndPads,
  F_Restore,
  F_Rotate,
  F_Save,
  F_Selected,
  F_SelectedArcs,
  F_SelectedElements,
  F_SelectedLines,
  F_SelectedNames,
  F_SelectedObjects,
  F_SelectedPads,
  F_SelectedPins,
  F_SelectedTexts,
  F_SelectedVias,
  F_SelectedRats,
  F_Stroke,
  F_Text,
  F_TextByName,
  F_TextScale,
  F_Thermal,
  F_ToLayout,
  F_ToggleAllDirections,
  F_ToggleAutoDRC,
  F_ToggleClearLine,
  F_ToggleFullPoly,
  F_ToggleGrid,
  F_ToggleHideNames,
  F_ToggleMask,
  F_ToggleName,
  F_ToggleObject,
  F_ToggleShowDRC,
  F_ToggleLiveRoute,
  F_ToggleRubberBandMode,
  F_ToggleStartDirection,
  F_ToggleSnapPin,
  F_ToggleThindraw,
  F_ToggleLockNames,
  F_ToggleOnlyNames,
  F_ToggleThindrawPoly,
  F_ToggleOrthoMove,
  F_ToggleLocalRef,
  F_ToggleCheckPlanes,
  F_ToggleUniqueNames,
  F_Via,
  F_ViaByName,
  F_Value,
  F_ViaDrillingHole,
  F_ViaSize,
  F_Zoom
}
FunctionID;

typedef struct			/* used to identify subfunctions */
{
  char *Identifier;
  FunctionID ID;
}
FunctionType;

/* --------------------------------------------------------------------------- */

/* %start-doc actions 00delta

Many actions take a @code{delta} parameter as the last parameter,
which is an amount to change something.  That @code{delta} may include
units, as an additional parameter, such as @code{Action(Object,5,mm)}.
If no units are specified, the default is PCB's native units
(currently 1/100 mil).  Also, if the delta is prefixed by @code{+} or
@code{-}, the size is increased or decreased by that amount.
Otherwise, the size size is set to the given amount.

@example
Action(Object,5,mil)
Action(Object,+0.5,mm)
Action(Object,-1)
@end example

Actions which take a @code{delta} parameter which do not accept all
these options will specify what they do take.

%end-doc */

/* %start-doc actions 00objects

Many actions act on indicated objects on the board.  They will have
parameters like @code{ToggleObject} or @code{SelectedVias} to indicate
what group of objects they act on.  Unless otherwise specified, these
parameters are defined as follows:

@table @code

@item Object
@itemx ToggleObject
Affects the object under the mouse pointer.  If this action is invoked
from a menu or script, the user will be prompted to click on an
object, which is then the object affected.

@item Selected
@itemx SelectedObjects

Affects all objects which are currently selected.  At least, all
selected objects for which the given action makes sense.

@item SelectedPins
@itemx SelectedVias
@itemx Selected@var{Type}
@itemx @i{etc}
Affects all objects which are both selected and of the @var{Type} specified.

@end table

%end-doc */

/*  %start-doc actions 00macros

@macro pinshapes

Pins, pads, and vias can have various shapes.  All may be round.  Pins
and pads may be square (obviously "square" pads are usually
rectangular).  Pins and vias may be octagonal.  When you change a
shape flag of an element, you actually change all of its pins and
pads.

Note that the square flag takes precedence over the octagon flag,
thus, if both the square and octagon flags are set, the object is
square.  When the square flag is cleared, the pins and pads will be
either round or, if the octagon flag is set, octagonal.

@end macro

%end-doc */

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static PointType InsertedPoint;
static LayerType *lastLayer;
static struct
{
  PolygonType *poly;
  LineType line;
}
fake;

static struct
{
  Coord X, Y;
  Cardinal Buffer;
  bool Click;
  bool Moving;		/* selected type clicked on */
  int Hit;			/* move type clicked on */
  void *ptr1;
  void *ptr2;
  void *ptr3;
}
Note;

static int defer_updates = 0;
static int defer_needs_update = 0;

static Cardinal polyIndex = 0;
static bool saved_mode = false;
#ifdef HAVE_LIBSTROKE
static bool mid_stroke = false;
static BoxType StrokeBox;
#endif
static FunctionType Functions[] = {
  {"AddSelected", F_AddSelected},
  {"All", F_All},
  {"AllConnections", F_AllConnections},
  {"AllRats", F_AllRats},
  {"AllUnusedPins", F_AllUnusedPins},
  {"Arc", F_Arc},
  {"Arrow", F_Arrow},
  {"Block", F_Block},
  {"Description", F_Description},
  {"Cancel", F_Cancel},
  {"Center", F_Center},
  {"Clear", F_Clear},
  {"ClearAndRedraw", F_ClearAndRedraw},
  {"ClearList", F_ClearList},
  {"Close", F_Close},
  {"Found", F_Found},
  {"Connection", F_Connection},
  {"Convert", F_Convert},
  {"Copy", F_Copy},
  {"CycleClip", F_CycleClip},
  {"CycleCrosshair", F_CycleCrosshair},
  {"DeleteRats", F_DeleteRats},
  {"Drag", F_Drag},
  {"DrillReport", F_DrillReport},
  {"Element", F_Element},
  {"ElementByName", F_ElementByName},
  {"ElementConnections", F_ElementConnections},
  {"ElementToBuffer", F_ElementToBuffer},
  {"Escape", F_Escape},
  {"Find", F_Find},
  {"FlipElement", F_FlipElement},
  {"FoundPins", F_FoundPins},
  {"Grid", F_Grid},
  {"InsertPoint", F_InsertPoint},
  {"Layer", F_Layer},
  {"Layout", F_Layout},
  {"LayoutAs", F_LayoutAs},
  {"LayoutToBuffer", F_LayoutToBuffer},
  {"Line", F_Line},
  {"LineSize", F_LineSize},
  {"Lock", F_Lock},
  {"Mirror", F_Mirror},
  {"Move", F_Move},
  {"NameOnPCB", F_NameOnPCB},
  {"Netlist", F_Netlist},
  {"NetByName", F_NetByName},
  {"None", F_None},
  {"Notify", F_Notify},
  {"Object", F_Object},
  {"ObjectByName", F_ObjectByName},
  {"PasteBuffer", F_PasteBuffer},
  {"PadByName", F_PadByName},
  {"PinByName", F_PinByName},
  {"PinOrPadName", F_PinOrPadName},
  {"Pinout", F_Pinout},
  {"Polygon", F_Polygon},
  {"PolygonHole", F_PolygonHole},
  {"PreviousPoint", F_PreviousPoint},
  {"RatsNest", F_RatsNest},
  {"Rectangle", F_Rectangle},
  {"Redraw", F_Redraw},
  {"Release", F_Release},
  {"Remove", F_Remove},
  {"RemoveSelected", F_RemoveSelected},
  {"Report", F_Report},
  {"Reset", F_Reset},
  {"ResetLinesAndPolygons", F_ResetLinesAndPolygons},
  {"ResetPinsViasAndPads", F_ResetPinsViasAndPads},
  {"Restore", F_Restore},
  {"Revert", F_Revert},
  {"Rotate", F_Rotate},
  {"Save", F_Save},
  {"Selected", F_Selected},
  {"SelectedArcs", F_SelectedArcs},
  {"SelectedElements", F_SelectedElements},
  {"SelectedLines", F_SelectedLines},
  {"SelectedNames", F_SelectedNames},
  {"SelectedObjects", F_SelectedObjects},
  {"SelectedPins", F_SelectedPins},
  {"SelectedPads", F_SelectedPads},
  {"SelectedRats", F_SelectedRats},
  {"SelectedTexts", F_SelectedTexts},
  {"SelectedVias", F_SelectedVias},
  {"Stroke", F_Stroke},
  {"Text", F_Text},
  {"TextByName", F_TextByName},
  {"TextScale", F_TextScale},
  {"Thermal", F_Thermal},
  {"ToLayout", F_ToLayout},
  {"Toggle45Degree", F_ToggleAllDirections},
  {"ToggleClearLine", F_ToggleClearLine},
  {"ToggleFullPoly", F_ToggleFullPoly},
  {"ToggleGrid", F_ToggleGrid},
  {"ToggleMask", F_ToggleMask},
  {"ToggleName", F_ToggleName},
  {"ToggleObject", F_ToggleObject},
  {"ToggleRubberBandMode", F_ToggleRubberBandMode},
  {"ToggleStartDirection", F_ToggleStartDirection},
  {"ToggleSnapPin", F_ToggleSnapPin},
  {"ToggleThindraw", F_ToggleThindraw},
  {"ToggleThindrawPoly", F_ToggleThindrawPoly},
  {"ToggleLockNames", F_ToggleLockNames},
  {"ToggleOnlyNames", F_ToggleOnlyNames},
  {"ToggleHideNames", F_ToggleHideNames},
  {"ToggleCheckPlanes", F_ToggleCheckPlanes},
  {"ToggleLocalRef", F_ToggleLocalRef},
  {"ToggleOrthoMove", F_ToggleOrthoMove},
  {"ToggleShowDRC", F_ToggleShowDRC},
  {"ToggleLiveRoute", F_ToggleLiveRoute},
  {"ToggleAutoDRC", F_ToggleAutoDRC},
  {"ToggleUniqueNames", F_ToggleUniqueNames},
  {"Value", F_Value},
  {"Via", F_Via},
  {"ViaByName", F_ViaByName},
  {"ViaSize", F_ViaSize},
  {"ViaDrillingHole", F_ViaDrillingHole},
  {"Zoom", F_Zoom}
};

/* ---------------------------------------------------------------------------
 * some local routines
 */
static int GetFunctionID (String);
static void AdjustAttachedBox (void);
static void NotifyLine (void);
static void NotifyBlock (void);
static void NotifyMode (void);
static void ClearWarnings (void);
#ifdef HAVE_LIBSTROKE
static void FinishStroke (void);
extern void stroke_init (void);
extern void stroke_record (int x, int y);
extern int stroke_trans (char *s);
#endif
static void ChangeFlag (char *, char *, int, char *);

#define ARG(n) (argc > (n) ? argv[n] : NULL)

#ifdef HAVE_LIBSTROKE

/*!
 * \brief Try to recognize the stroke sent.
 */
void
FinishStroke (void)
{
  char msg[255];
  int type;
  unsigned long num;
  void *ptr1, *ptr2, *ptr3;

  mid_stroke = false;
  if (stroke_trans (msg))
    {
      num = atoi (msg);
      switch (num)
	{
	case 456:
	  if (Settings.Mode == LINE_MODE)
	    {
	      SetMode (LINE_MODE);
	    }
	  break;
	case 9874123:
	case 74123:
	case 987412:
	case 8741236:
	case 874123:
	  RotateScreenObject (StrokeBox.X1, StrokeBox.Y1, SWAP_IDENT ? 1 : 3);
	  break;
	case 7896321:
	case 786321:
	case 789632:
	case 896321:
	  RotateScreenObject (StrokeBox.X1, StrokeBox.Y1, SWAP_IDENT ? 3 : 1);
	  break;
	case 258:
	  SetMode (LINE_MODE);
	  break;
	case 852:
	  SetMode (ARROW_MODE);
	  break;
	case 1478963:
	  ActionUndo ("");
	  break;
	case 147423:
	case 147523:
	case 1474123:
	  Redo (true);
	  break;
	case 148963:
	case 147863:
	case 147853:
	case 145863:
	  SetMode (VIA_MODE);
	  break;
	case 951:
	case 9651:
	case 9521:
	case 9621:
	case 9851:
	case 9541:
	case 96521:
	case 96541:
	case 98541:
	  /* XXX: FIXME: Call a zoom-extents action */
	  break;
	case 159:
	case 1269:
	case 1259:
	case 1459:
	case 1569:
	case 1589:
	case 12569:
	case 12589:
	case 14589:
	  /* XXX: FIXME: Zoom to fit the box StrokeBox.[X1,Y1] - StrokeBox.[X2,Y2] */
	  break;

	default:
	  Message (_("Unknown stroke %s\n"), msg);
	  break;
	}
    }
  else
    gui->beep ();
}
#endif

/*!
 * \brief Clear warning color from pins/pads.
 */
static void
ClearWarnings ()
{
  Settings.RatWarn = false;
  ALLPIN_LOOP (PCB->Data);
  {
    if (TEST_FLAG (WARNFLAG, pin))
      {
	CLEAR_FLAG (WARNFLAG, pin);
	DrawPin (pin);
      }
  }
  ENDALL_LOOP;
  ALLPAD_LOOP (PCB->Data);
  {
    if (TEST_FLAG (WARNFLAG, pad))
      {
	CLEAR_FLAG (WARNFLAG, pad);
	DrawPad (pad);
      }
  }
  ENDALL_LOOP;
  Draw ();
}

/*!
 * \brief Click callback.
 *
 * This is called a clicktime after a mouse down, to we can distinguish
 * between short clicks (typically: select or create something) and long
 * clicks. Long clicks typically drag something.
 */
static void
click_cb (hidval hv)
{
  if (Note.Click)
    {
      notify_crosshair_change (false);
      Note.Click = false;
      if (Note.Moving && !gui->shift_is_pressed ())
	{
	  Note.Buffer = Settings.BufferNumber;
	  SetBufferNumber (MAX_BUFFER - 1);
	  ClearBuffer (PASTEBUFFER);
	  AddSelectedToBuffer (PASTEBUFFER, Note.X, Note.Y, true);
	  SaveUndoSerialNumber ();
	  RemoveSelected ();
	  SaveMode ();
	  saved_mode = true;
	  SetMode (PASTEBUFFER_MODE);
	}
      else if (Note.Hit && !gui->shift_is_pressed ())
	{
	  SaveMode ();
	  saved_mode = true;
	  SetMode (gui->control_is_pressed ()? COPY_MODE : MOVE_MODE);
	  Crosshair.AttachedObject.Ptr1 = Note.ptr1;
	  Crosshair.AttachedObject.Ptr2 = Note.ptr2;
	  Crosshair.AttachedObject.Ptr3 = Note.ptr3;
	  Crosshair.AttachedObject.Type = Note.Hit;
	  AttachForCopy (Note.X, Note.Y);
	}
      else
	{
	  BoxType box;

	  Note.Hit = 0;
	  Note.Moving = false;
	  SaveUndoSerialNumber ();
	  box.X1 = -MAX_COORD;
	  box.Y1 = -MAX_COORD;
	  box.X2 = MAX_COORD;
	  box.Y2 = MAX_COORD;
	  /* unselect first if shift key not down */
	  if (!gui->shift_is_pressed () && SelectBlock (&box, false))
	    SetChangedFlag (true);
	  NotifyBlock ();
	  Crosshair.AttachedBox.Point1.X = Note.X;
	  Crosshair.AttachedBox.Point1.Y = Note.Y;
	}
      notify_crosshair_change (true);
    }
}

/*!
 * \brief This is typically called when the mouse has moved or the mouse
 * button was released.
 */
static void
ReleaseMode (void)
{
  BoxType box;

  if (Note.Click)
    {
      BoxType box;

      box.X1 = -MAX_COORD;
      box.Y1 = -MAX_COORD;
      box.X2 = MAX_COORD;
      box.Y2 = MAX_COORD;

      Note.Click = false;	/* inhibit timer action */
      SaveUndoSerialNumber ();
      /* unselect first if shift key not down */
      if (!gui->shift_is_pressed ())
	{
	  if (SelectBlock (&box, false))
	    SetChangedFlag (true);
	  if (Note.Moving)
	    {
	      Note.Moving = 0;
	      Note.Hit = 0;
	      return;
	    }
	}
        /* Restore the SN so that if we select something the deselect/select combo
         gets the same SN. */
        RestoreUndoSerialNumber();
        if (SelectObject ())
            SetChangedFlag (true);
        else
        /* We didn't select anything new, so, the deselection should get its
         own SN. */
            IncrementUndoSerialNumber();
      Note.Hit = 0;
      Note.Moving = 0;
    }
  else if (Note.Moving)
    {
      RestoreUndoSerialNumber ();
      NotifyMode ();
      ClearBuffer (PASTEBUFFER);
      SetBufferNumber (Note.Buffer);
      Note.Moving = false;
      Note.Hit = 0;
    }
  else if (Note.Hit)
    {
      NotifyMode ();
      Note.Hit = 0;
    }
  else if (Settings.Mode == ARROW_MODE)
    {
      box.X1 = MIN (Crosshair.AttachedBox.Point1.X,
		    Crosshair.AttachedBox.Point2.X);
      box.Y1 = MIN (Crosshair.AttachedBox.Point1.Y,
		    Crosshair.AttachedBox.Point2.Y);
      box.X2 = MAX (Crosshair.AttachedBox.Point1.X,
		    Crosshair.AttachedBox.Point2.X);
      box.Y2 = MAX (Crosshair.AttachedBox.Point1.Y,
		    Crosshair.AttachedBox.Point2.Y);
      RestoreUndoSerialNumber ();
      if (SelectBlock (&box, true))
	SetChangedFlag (true);
      else if (Bumped)
	IncrementUndoSerialNumber ();
      Crosshair.AttachedBox.State = STATE_FIRST;
    }
  if (saved_mode)
    RestoreMode ();
  saved_mode = false;
}

#define HSIZE 257
static char function_hash[HSIZE];
static int hash_initted = 0;

static int
hashfunc(String s)
{
  int i = 0;
  while (*s)
    {
      i ^= i >> 16;
      i = (i * 13) ^ (unsigned char)tolower((int) *s);
      s ++;
    }
  i = (unsigned int)i % HSIZE;
  return i;
}

/*!
 * \brief Get function ID of passed string.
 */
static int
GetFunctionID (String Ident)
{
  int i, h;

  if (Ident == 0)
    return -1;

  if (!hash_initted)
    {
      hash_initted = 1;
      if (HSIZE < ENTRIES (Functions) * 2)
	{
	  fprintf(stderr, _("Error: function hash size too small (%d vs %lu at %s:%d)\n"),
		  HSIZE, (unsigned long) ENTRIES (Functions)*2, __FILE__,  __LINE__);
	  exit(1);
	}
      if (ENTRIES (Functions) > 254)
	{
	  /* Change 'char' to 'int' and remove this when we get to 256
	     strings to hash. */
	  fprintf(stderr, _("Error: function hash type too small (%d vs %lu at %s:%d)\n"),
		  256, (unsigned long) ENTRIES (Functions), __FILE__,  __LINE__);
	  exit(1);
	  
	}
      for (i=ENTRIES (Functions)-1; i>=0; i--)
	{
	  h = hashfunc (Functions[i].Identifier);
	  while (function_hash[h])
	    h = (h + 1) % HSIZE;
	  function_hash[h] = i + 1;
	}
    }

  i = hashfunc (Ident);
  while (1)
    {
      /* We enforce the "hash table bigger than function table" rule,
	 so we know there will be at least one zero entry to find.  */
      if (!function_hash[i])
	return (-1);
      if (!strcasecmp (Ident, Functions[function_hash[i]-1].Identifier))
	return ((int) Functions[function_hash[i]-1].ID);
      i = (i + 1) % HSIZE;
    }
}

/*!
 * \brief Set new coordinates if in 'RECTANGLE' mode.
 *
 * The cursor shape is also adjusted.
 */
static void
AdjustAttachedBox (void)
{
  if (Settings.Mode == ARC_MODE)
    {
      Crosshair.AttachedBox.otherway = gui->shift_is_pressed ();
      return;
    }
  switch (Crosshair.AttachedBox.State)
    {
    case STATE_SECOND:		/* one corner is selected */
      {
	/* update coordinates */
	Crosshair.AttachedBox.Point2.X = Crosshair.X;
	Crosshair.AttachedBox.Point2.Y = Crosshair.Y;
	break;
      }
    }
}

/*!
 * \brief Adjusts the objects which are to be created like attached
 * lines.
 */
void
AdjustAttachedObjects (void)
{
  PointType *pnt;
  switch (Settings.Mode)
    {
      /* update at least an attached block (selection) */
    case NO_MODE:
    case ARROW_MODE:
      if (Crosshair.AttachedBox.State)
	{
	  Crosshair.AttachedBox.Point2.X = Crosshair.X;
	  Crosshair.AttachedBox.Point2.Y = Crosshair.Y;
	}
      break;

      /* rectangle creation mode */
    case RECTANGLE_MODE:
    case ARC_MODE:
      AdjustAttachedBox ();
      break;

      /* polygon creation mode */
    case POLYGON_MODE:
    case POLYGONHOLE_MODE:
      AdjustAttachedLine ();
      break;
      /* line creation mode */
    case LINE_MODE:
      if (PCB->RatDraw || PCB->Clipping == 0)
	AdjustAttachedLine ();
      else
	AdjustTwoLine (PCB->Clipping - 1);
      break;
      /* point insertion mode */
    case INSERTPOINT_MODE:
      pnt = AdjustInsertPoint ();
      if (pnt)
	InsertedPoint = *pnt;
      break;
    case ROTATE_MODE:
      break;
    }
}

/*!
 * \brief Creates points of a line.
 */
static void
NotifyLine (void)
{
  int type = NO_TYPE;
  void *ptr1, *ptr2, *ptr3;

  if (!Marked.status || TEST_FLAG (LOCALREFFLAG, PCB))
    SetLocalRef (Crosshair.X, Crosshair.Y, true);
  switch (Crosshair.AttachedLine.State)
    {
    case STATE_FIRST:		/* first point */
      if (PCB->RatDraw && SearchScreen (Crosshair.X, Crosshair.Y,
					PAD_TYPE | PIN_TYPE, &ptr1, &ptr1,
					&ptr1) == NO_TYPE)
	{
	  gui->beep ();
	  break;
	}
      if (TEST_FLAG (AUTODRCFLAG, PCB) && Settings.Mode == LINE_MODE)
	{
	  type = SearchScreen (Crosshair.X, Crosshair.Y,
			       PIN_TYPE | PAD_TYPE | VIA_TYPE, &ptr1, &ptr2,
			       &ptr3);
	  LookupConnection (Crosshair.X, Crosshair.Y, true, 1, CONNECTEDFLAG, false);
	  LookupConnection (Crosshair.X, Crosshair.Y, true, 1, FOUNDFLAG, true);
	}
      if (type == PIN_TYPE || type == VIA_TYPE)
	{
	  Crosshair.AttachedLine.Point1.X =
	    Crosshair.AttachedLine.Point2.X = ((PinType *) ptr2)->X;
	  Crosshair.AttachedLine.Point1.Y =
	    Crosshair.AttachedLine.Point2.Y = ((PinType *) ptr2)->Y;
	}
      else if (type == PAD_TYPE)
	{
	  PadType *pad = (PadType *) ptr2;
	  double d1 = Distance (Crosshair.X, Crosshair.Y, pad->Point1.X, pad->Point1.Y);
	  double d2 = Distance (Crosshair.X, Crosshair.Y, pad->Point2.X, pad->Point2.Y);
	  if (d2 < d1)
	    {
	      Crosshair.AttachedLine.Point1 =
		Crosshair.AttachedLine.Point2 = pad->Point2;
	    }
	  else
	    {
	      Crosshair.AttachedLine.Point1 =
		Crosshair.AttachedLine.Point2 = pad->Point1;
	    }
	}
      else
	{
	  Crosshair.AttachedLine.Point1.X =
	    Crosshair.AttachedLine.Point2.X = Crosshair.X;
	  Crosshair.AttachedLine.Point1.Y =
	    Crosshair.AttachedLine.Point2.Y = Crosshair.Y;
	}
      Crosshair.AttachedLine.State = STATE_SECOND;
      break;

    case STATE_SECOND:
      /* fall through to third state too */
      lastLayer = CURRENT;
    default:			/* all following points */
      Crosshair.AttachedLine.State = STATE_THIRD;
      break;
    }
}

/*!
 * \brief Create first or second corner of a marked block.
 */
static void
NotifyBlock (void)
{
  notify_crosshair_change (false);
  switch (Crosshair.AttachedBox.State)
    {
    case STATE_FIRST:		/* setup first point */
      Crosshair.AttachedBox.Point1.X =
	Crosshair.AttachedBox.Point2.X = Crosshair.X;
      Crosshair.AttachedBox.Point1.Y =
	Crosshair.AttachedBox.Point2.Y = Crosshair.Y;
      Crosshair.AttachedBox.State = STATE_SECOND;
      break;

    case STATE_SECOND:		/* setup second point */
      Crosshair.AttachedBox.State = STATE_THIRD;
      break;
    }
  notify_crosshair_change (true);
}


/*!
 * \brief This is called after every mode change, like mouse button pressed,
 * mouse button released, dragging something started or a different tool
 * selected.
 *
 * It does what's appropriate for the current mode setting.
 * This can also mean creation of an object at the current crosshair location.
 *
 * New created objects are added to the create undo list of course.
 */
static void
NotifyMode (void)
{
  void *ptr1, *ptr2, *ptr3;
  int type;

  if (Settings.RatWarn)
    ClearWarnings ();
  switch (Settings.Mode)
    {
    case ARROW_MODE:
      {
	int test;
	hidval hv;

	Note.Click = true;
	/* do something after click time */
	gui->add_timer (click_cb, CLICK_TIME, hv);

	/* see if we clicked on something already selected
	 * (Note.Moving) or clicked on a MOVE_TYPE
	 * (Note.Hit)
	 */
	for (test = (SELECT_TYPES | MOVE_TYPES) & ~RATLINE_TYPE;
	     test; test &= ~type)
	  {
	    type = SearchScreen (Note.X, Note.Y, test, &ptr1, &ptr2, &ptr3);
	    if (!Note.Hit && (type & MOVE_TYPES) &&
		!TEST_FLAG (LOCKFLAG, (PinType *) ptr2))
	      {
		Note.Hit = type;
		Note.ptr1 = ptr1;
		Note.ptr2 = ptr2;
		Note.ptr3 = ptr3;
	      }
	    if (!Note.Moving && (type & SELECT_TYPES) &&
		TEST_FLAG (SELECTEDFLAG, (PinType *) ptr2))
	      Note.Moving = true;
	    if ((Note.Hit && Note.Moving) || type == NO_TYPE)
	      break;
	  }
	break;
      }

    case VIA_MODE:
      {
	PinType *via;

	if (!PCB->ViaOn)
	  {
	    Message (_("You must turn via visibility on before\n"
		       "you can place vias\n"));
	    break;
	  }
	if ((via = CreateNewVia (PCB->Data, Note.X, Note.Y,
				 Settings.ViaThickness, 2 * Settings.Keepaway,
				 0, Settings.ViaDrillingHole, NULL,
				 NoFlags ())) != NULL)
	  {
	    AddObjectToCreateUndoList (VIA_TYPE, via, via, via);
	    if (gui->shift_is_pressed ())
	      ChangeObjectThermal (VIA_TYPE, via, via, via, PCB->ThermStyle);
	    IncrementUndoSerialNumber ();
	    DrawVia (via);
	    Draw ();
	  }
	break;
      }

    case ARC_MODE:
      {
	switch (Crosshair.AttachedBox.State)
	  {
	  case STATE_FIRST:
	    Crosshair.AttachedBox.Point1.X =
	      Crosshair.AttachedBox.Point2.X = Note.X;
	    Crosshair.AttachedBox.Point1.Y =
	      Crosshair.AttachedBox.Point2.Y = Note.Y;
	    Crosshair.AttachedBox.State = STATE_SECOND;
	    break;

	  case STATE_SECOND:
	  case STATE_THIRD:
	    {
	      ArcType *arc;
	      Coord wx, wy;
	      Angle sa, dir;

	      wx = Note.X - Crosshair.AttachedBox.Point1.X;
	      wy = Note.Y - Crosshair.AttachedBox.Point1.Y;
	      if (XOR (Crosshair.AttachedBox.otherway, abs (wy) > abs (wx)))
		{
		  Crosshair.AttachedBox.Point2.X =
		    Crosshair.AttachedBox.Point1.X + abs (wy) * SGNZ (wx);
		  sa = (wx >= 0) ? 0 : 180;
#ifdef ARC45
		  if (abs (wy) / 2 >= abs (wx))
		    dir = (SGNZ (wx) == SGNZ (wy)) ? 45 : -45;
		  else
#endif
		    dir = (SGNZ (wx) == SGNZ (wy)) ? 90 : -90;
		}
	      else
		{
		  Crosshair.AttachedBox.Point2.Y =
		    Crosshair.AttachedBox.Point1.Y + abs (wx) * SGNZ (wy);
		  sa = (wy >= 0) ? -90 : 90;
#ifdef ARC45
		  if (abs (wx) / 2 >= abs (wy))
		    dir = (SGNZ (wx) == SGNZ (wy)) ? -45 : 45;
		  else
#endif
		    dir = (SGNZ (wx) == SGNZ (wy)) ? -90 : 90;
		  wy = wx;
		}
	      if (abs (wy) > 0 && (arc = CreateNewArcOnLayer (CURRENT,
							      Crosshair.
							      AttachedBox.
							      Point2.X,
							      Crosshair.
							      AttachedBox.
							      Point2.Y,
							      abs (wy),
							      abs (wy),
							      sa,
							      dir,
							      Settings.
							      LineThickness,
							      2 * Settings.
							      Keepaway,
							      MakeFlags
							      (TEST_FLAG
							       (CLEARNEWFLAG,
								PCB) ?
							       CLEARLINEFLAG :
							       0))))
		{
		  BoxType *bx;

		  bx = GetArcEnds (arc);
		  Crosshair.AttachedBox.Point1.X =
		    Crosshair.AttachedBox.Point2.X = bx->X2;
		  Crosshair.AttachedBox.Point1.Y =
		    Crosshair.AttachedBox.Point2.Y = bx->Y2;
		  AddObjectToCreateUndoList (ARC_TYPE, CURRENT, arc, arc);
		  IncrementUndoSerialNumber ();
		  addedLines++;
		  DrawArc (CURRENT, arc);
		  Draw ();
		  Crosshair.AttachedBox.State = STATE_THIRD;
		}
	      break;
	    }
	  }
	break;
      }
    case LOCK_MODE:
      {
	type = SearchScreen (Note.X, Note.Y, LOCK_TYPES, &ptr1, &ptr2, &ptr3);
	if (type == ELEMENT_TYPE)
	  {
	    ElementType *element = (ElementType *) ptr2;

	    TOGGLE_FLAG (LOCKFLAG, element);
	    PIN_LOOP (element);
	    {
	      TOGGLE_FLAG (LOCKFLAG, pin);
	      CLEAR_FLAG (SELECTEDFLAG, pin);
	    }
	    END_LOOP;
	    PAD_LOOP (element);
	    {
	      TOGGLE_FLAG (LOCKFLAG, pad);
	      CLEAR_FLAG (SELECTEDFLAG, pad);
	    }
	    END_LOOP;
	    CLEAR_FLAG (SELECTEDFLAG, element);
	    /* always re-draw it since I'm too lazy
	     * to tell if a selected flag changed
	     */
	    DrawElement (element);
	    Draw ();
	    SetChangedFlag (true);
	    hid_actionl ("Report", "Object", NULL);
	  }
	else if (type != NO_TYPE)
	  {
	    TextType *thing = (TextType *) ptr3;
	    TOGGLE_FLAG (LOCKFLAG, thing);
	    if (TEST_FLAG (LOCKFLAG, thing)
		&& TEST_FLAG (SELECTEDFLAG, thing))
	      {
		/* this is not un-doable since LOCK isn't */
		CLEAR_FLAG (SELECTEDFLAG, thing);
		DrawObject (type, ptr1, ptr2);
		Draw ();
	      }
	    SetChangedFlag (true);
	    hid_actionl ("Report", "Object", NULL);
	  }
	break;
      }
    case THERMAL_MODE:
      {
	if (((type
	      =
	      SearchScreen (Note.X, Note.Y, PIN_TYPES, &ptr1, &ptr2,
			    &ptr3)) != NO_TYPE)
	    && !TEST_FLAG (HOLEFLAG, (PinType *) ptr3))
	  {
	    if (gui->shift_is_pressed ())
	      {
		int tstyle = GET_THERM (INDEXOFCURRENT, (PinType *) ptr3);
		tstyle++;
		if (tstyle > 5)
		  tstyle = 1;
		ChangeObjectThermal (type, ptr1, ptr2, ptr3, tstyle);
	      }
	    else if (GET_THERM (INDEXOFCURRENT, (PinType *) ptr3))
	      ChangeObjectThermal (type, ptr1, ptr2, ptr3, 0);
	    else
	      ChangeObjectThermal (type, ptr1, ptr2, ptr3, PCB->ThermStyle);
	  }
	break;
      }

    case LINE_MODE:
      /* do update of position */
      NotifyLine ();
      if (Crosshair.AttachedLine.State != STATE_THIRD)
	break;

      /* Remove anchor if clicking on start point;
       * this means we can't paint 0 length lines
       * which could be used for square SMD pads.
       * Instead use a very small delta, or change
       * the file after saving.
       */
      if (Crosshair.X == Crosshair.AttachedLine.Point1.X
	  && Crosshair.Y == Crosshair.AttachedLine.Point1.Y)
	{
	  SetMode (LINE_MODE);
	  break;
	}

      if (PCB->RatDraw)
	{
	  RatType *line;
	  if ((line = AddNet ()))
	    {
	      addedLines++;
	      AddObjectToCreateUndoList (RATLINE_TYPE, line, line, line);
	      IncrementUndoSerialNumber ();
	      DrawRat (line);
	      Crosshair.AttachedLine.Point1.X =
		Crosshair.AttachedLine.Point2.X;
	      Crosshair.AttachedLine.Point1.Y =
		Crosshair.AttachedLine.Point2.Y;
	      Draw ();
	    }
	  break;
	}
      else
	/* create line if both ends are determined && length != 0 */
	{
	  LineType *line;
	  int line_flags = 0;

	  if (TEST_FLAG (AUTODRCFLAG, PCB) && !TEST_SILK_LAYER (CURRENT))
	    line_flags |= CONNECTEDFLAG | FOUNDFLAG;

	  if (TEST_FLAG (CLEARNEWFLAG, PCB))
	    line_flags |= CLEARLINEFLAG;

	  if (PCB->Clipping
	      && Crosshair.AttachedLine.Point1.X ==
	      Crosshair.AttachedLine.Point2.X
	      && Crosshair.AttachedLine.Point1.Y ==
	      Crosshair.AttachedLine.Point2.Y
	      && (Crosshair.AttachedLine.Point2.X != Note.X
		  || Crosshair.AttachedLine.Point2.Y != Note.Y))
	    {
	      /* We will only need to paint the second line segment.
	         Since we only check for vias on the first segment,
	         swap them so the non-empty segment is the first segment. */
	      Crosshair.AttachedLine.Point2.X = Note.X;
	      Crosshair.AttachedLine.Point2.Y = Note.Y;
	    }

	  if ((Crosshair.AttachedLine.Point1.X !=
	       Crosshair.AttachedLine.Point2.X
	       || Crosshair.AttachedLine.Point1.Y !=
	       Crosshair.AttachedLine.Point2.Y))
            {
              PinType *via;

              if ((line =
		  CreateDrawnLineOnLayer (CURRENT,
					  Crosshair.AttachedLine.Point1.X,
					  Crosshair.AttachedLine.Point1.Y,
					  Crosshair.AttachedLine.Point2.X,
					  Crosshair.AttachedLine.Point2.Y,
					  Settings.LineThickness,
					  2 * Settings.Keepaway,
					  MakeFlags (line_flags))) != NULL)
                {

                  addedLines++;
                  AddObjectToCreateUndoList (LINE_TYPE, CURRENT, line, line);
                  DrawLine (CURRENT, line);
                }
	      /* place a via if vias are visible, the layer is
	         in a new group since the last line and there
	         isn't a pin already here */
	      if (PCB->ViaOn && GetLayerGroupNumberByPointer (CURRENT) !=
		  GetLayerGroupNumberByPointer (lastLayer) &&
		  SearchObjectByLocation (PIN_TYPES, &ptr1, &ptr2, &ptr3,
					  Crosshair.AttachedLine.Point1.X,
					  Crosshair.AttachedLine.Point1.Y,
					  Settings.ViaThickness / 2) ==
		  NO_TYPE
		  && (via =
		      CreateNewVia (PCB->Data,
				    Crosshair.AttachedLine.Point1.X,
				    Crosshair.AttachedLine.Point1.Y,
				    Settings.ViaThickness,
				    2 * Settings.Keepaway, 0,
				    Settings.ViaDrillingHole, NULL,
				    NoFlags ())) != NULL)
		{
		  AddObjectToCreateUndoList (VIA_TYPE, via, via, via);
		  DrawVia (via);
		}
	      /* copy the coordinates */
	      Crosshair.AttachedLine.Point1.X =
		Crosshair.AttachedLine.Point2.X;
	      Crosshair.AttachedLine.Point1.Y =
		Crosshair.AttachedLine.Point2.Y;
	      IncrementUndoSerialNumber ();
	      lastLayer = CURRENT;
	    }
	  if (PCB->Clipping && (Note.X != Crosshair.AttachedLine.Point2.X
				|| Note.Y !=
				Crosshair.AttachedLine.Point2.Y))
            {
              if ((line =
		  CreateDrawnLineOnLayer (CURRENT,
					  Crosshair.AttachedLine.Point2.X,
					  Crosshair.AttachedLine.Point2.Y,
					  Note.X, Note.Y,
					  Settings.LineThickness,
					  2 * Settings.Keepaway,
					  MakeFlags (line_flags))) != NULL)
                {
                  addedLines++;
                  AddObjectToCreateUndoList (LINE_TYPE, CURRENT, line, line);
                  IncrementUndoSerialNumber ();
                  DrawLine (CURRENT, line);
                }
	      /* move to new start point */
	      Crosshair.AttachedLine.Point1.X = Note.X;
	      Crosshair.AttachedLine.Point1.Y = Note.Y;
	      Crosshair.AttachedLine.Point2.X = Note.X;
	      Crosshair.AttachedLine.Point2.Y = Note.Y;
	      if (TEST_FLAG (SWAPSTARTDIRFLAG, PCB))
		{
		  PCB->Clipping ^= 3;
		}
	    }
	  if (TEST_FLAG (AUTODRCFLAG, PCB) && !TEST_SILK_LAYER (CURRENT))
	    LookupConnection (Note.X, Note.Y, true, 1, CONNECTEDFLAG, false);
	  Draw ();
	}
      break;

    case RECTANGLE_MODE:
      /* do update of position */
      NotifyBlock ();

      /* create rectangle if both corners are determined 
       * and width, height are != 0
       */
      if (Crosshair.AttachedBox.State == STATE_THIRD &&
	  Crosshair.AttachedBox.Point1.X != Crosshair.AttachedBox.Point2.X &&
	  Crosshair.AttachedBox.Point1.Y != Crosshair.AttachedBox.Point2.Y)
	{
	  PolygonType *polygon;

	  int flags = CLEARPOLYFLAG;
	  if (TEST_FLAG (NEWFULLPOLYFLAG, PCB))
	    flags |= FULLPOLYFLAG;
	  if ((polygon = CreateNewPolygonFromRectangle (CURRENT,
							Crosshair.
							AttachedBox.Point1.X,
							Crosshair.
							AttachedBox.Point1.Y,
							Crosshair.
							AttachedBox.Point2.X,
							Crosshair.
							AttachedBox.Point2.Y,
							MakeFlags
							(flags))) !=
	      NULL)
	    {
	      AddObjectToCreateUndoList (POLYGON_TYPE, CURRENT,
					 polygon, polygon);
	      IncrementUndoSerialNumber ();
	      DrawPolygon (CURRENT, polygon);
	      Draw ();
	    }

	  /* reset state to 'first corner' */
	  Crosshair.AttachedBox.State = STATE_FIRST;
	}
      break;

    case TEXT_MODE:
      {
	char *string;

	if ((string = gui->prompt_for (_("Enter text:"), "")) != NULL)
	  {
	    if (strlen(string) > 0)
	      {
		TextType *text;
		int flag = CLEARLINEFLAG;

		if (GetLayerGroupNumberByNumber (INDEXOFCURRENT) ==
		    GetLayerGroupNumberBySide (BOTTOM_SIDE))
		  flag |= ONSOLDERFLAG;
		if ((text = CreateNewText (CURRENT, Settings.Font, Note.X,
					   Note.Y, 0, Settings.TextScale,
					   string, MakeFlags (flag))) != NULL)
		  {
		    AddObjectToCreateUndoList (TEXT_TYPE, CURRENT, text, text);
		    IncrementUndoSerialNumber ();
		    DrawText (CURRENT, text);
		    Draw ();
		  }
		}
	    free (string);
	  }
	break;
      }

    case POLYGON_MODE:
      {
	PointType *points = Crosshair.AttachedPolygon.Points;
	Cardinal n = Crosshair.AttachedPolygon.PointN;

	/* do update of position; use the 'LINE_MODE' mechanism */
	NotifyLine ();

	/* check if this is the last point of a polygon */
	if (n >= 3 &&
	    points->X == Crosshair.AttachedLine.Point2.X &&
	    points->Y == Crosshair.AttachedLine.Point2.Y)
	  {
	    CopyAttachedPolygonToLayer ();
	    Draw ();
	    break;
	  }

	/* create new point if it's the first one or if it's
	 * different to the last one
	 */
	if (!n ||
	    points[n - 1].X != Crosshair.AttachedLine.Point2.X ||
	    points[n - 1].Y != Crosshair.AttachedLine.Point2.Y)
	  {
	    CreateNewPointInPolygon (&Crosshair.AttachedPolygon,
				     Crosshair.AttachedLine.Point2.X,
				     Crosshair.AttachedLine.Point2.Y);

	    /* copy the coordinates */
	    Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X;
	    Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y;
	  }
	break;
      }

    case POLYGONHOLE_MODE:
      {
	switch (Crosshair.AttachedObject.State)
	  {
	    /* first notify, lookup object */
	  case STATE_FIRST:
	    Crosshair.AttachedObject.Type =
	      SearchScreen (Note.X, Note.Y, POLYGON_TYPE,
			    &Crosshair.AttachedObject.Ptr1,
			    &Crosshair.AttachedObject.Ptr2,
			    &Crosshair.AttachedObject.Ptr3);

          if (Crosshair.AttachedObject.Type == NO_TYPE)
            {
              Message (_("The first point of a polygon hole must be on a polygon.\n"));
              break; /* don't start doing anything if clicked outside of polys */
            }

          if (TEST_FLAG(LOCKFLAG, (PolygonType *) Crosshair.AttachedObject.Ptr2))
            {
              Message (_("Sorry, the object is locked\n"));
              Crosshair.AttachedObject.Type = NO_TYPE;
              break;
            }
          else
            Crosshair.AttachedObject.State = STATE_SECOND;
            /* Fall thru: first click is also the first point of the
             * poly hole. */

            /* second notify, insert new point into object */
          case STATE_SECOND:
            {
	      PointType *points = Crosshair.AttachedPolygon.Points;
	      Cardinal n = Crosshair.AttachedPolygon.PointN;
	      POLYAREA *original, *new_hole, *result;
	      FlagType Flags;

	      /* do update of position; use the 'LINE_MODE' mechanism */
	      NotifyLine ();

	      /* check if this is the last point of a polygon */
	      if (n >= 3 &&
		  points->X == Crosshair.AttachedLine.Point2.X &&
		  points->Y == Crosshair.AttachedLine.Point2.Y)
		{
		  /* Create POLYAREAs from the original polygon
		   * and the new hole polygon */
		  original = PolygonToPoly ((PolygonType *)Crosshair.AttachedObject.Ptr2);
		  new_hole = PolygonToPoly (&Crosshair.AttachedPolygon);

		  /* Subtract the hole from the original polygon shape */
		  poly_Boolean_free (original, new_hole, &result, PBO_SUB);

		  /* Convert the resulting polygon(s) into a new set of nodes
		   * and place them on the page. Delete the original polygon.
		   */
		  SaveUndoSerialNumber ();
		  Flags = ((PolygonType *)Crosshair.AttachedObject.Ptr2)->Flags;
		  PolyToPolygonsOnLayer (PCB->Data, (LayerType *)Crosshair.AttachedObject.Ptr1,
					 result, Flags);
		  RemoveObject (POLYGON_TYPE,
				Crosshair.AttachedObject.Ptr1,
				Crosshair.AttachedObject.Ptr2,
				Crosshair.AttachedObject.Ptr3);
		  RestoreUndoSerialNumber ();
		  IncrementUndoSerialNumber ();
		  Draw ();

		/* reset state of attached line */
		memset (&Crosshair.AttachedPolygon, 0, sizeof (PolygonType));
		Crosshair.AttachedObject.State = STATE_FIRST;
		addedLines = 0;

		  break;
		}

	      /* create new point if it's the first one or if it's
	       * different to the last one
	       */
	      if (!n ||
		  points[n - 1].X != Crosshair.AttachedLine.Point2.X ||
		  points[n - 1].Y != Crosshair.AttachedLine.Point2.Y)
		{
		  CreateNewPointInPolygon (&Crosshair.AttachedPolygon,
					   Crosshair.AttachedLine.Point2.X,
					   Crosshair.AttachedLine.Point2.Y);

		  /* copy the coordinates */
		  Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X;
		  Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y;
		}
	      break;
	    }
	  }

	break;
      }

    case PASTEBUFFER_MODE:
      {
	TextType estr[MAX_ELEMENTNAMES];
	ElementType *e = 0;

	if (gui->shift_is_pressed ())
	  {
	    int type =
	      SearchScreen (Note.X, Note.Y, ELEMENT_TYPE, &ptr1, &ptr2,
			    &ptr3);
	    if (type == ELEMENT_TYPE)
	      {
		e = (ElementType *) ptr1;
		if (e)
		  {
		    int i;

		    memcpy (estr, e->Name,
			    MAX_ELEMENTNAMES * sizeof (TextType));
		    for (i = 0; i < MAX_ELEMENTNAMES; ++i)
		      estr[i].TextString = estr[i].TextString ? strdup(estr[i].TextString) : NULL;
		    RemoveElement (e);
		  }
	      }
	  }
	if (CopyPastebufferToLayout (Note.X, Note.Y))
	  SetChangedFlag (true);
	if (e)
	  {
	    int type =
	      SearchScreen (Note.X, Note.Y, ELEMENT_TYPE, &ptr1, &ptr2,
			    &ptr3);
	    if (type == ELEMENT_TYPE && ptr1)
	      {
		int i, save_n;
		e = (ElementType *) ptr1;

		save_n = NAME_INDEX (PCB);

		for (i = 0; i < MAX_ELEMENTNAMES; i++)
		  {
		    if (i == save_n)
		      EraseElementName (e);
		    r_delete_entry (PCB->Data->name_tree[i],
				    (BoxType *) & (e->Name[i]));
		    memcpy (&(e->Name[i]), &(estr[i]), sizeof (TextType));
		    e->Name[i].Element = e;
		    SetTextBoundingBox (&(e->Name[i]));
		    r_insert_entry (PCB->Data->name_tree[i],
				    (BoxType *) & (e->Name[i]), 0);
		    if (i == save_n)
		      DrawElementName (e);
		  }
	      }
	  }
	break;
      }

    case REMOVE_MODE:
      if ((type =
	   SearchScreen (Note.X, Note.Y, REMOVE_TYPES, &ptr1, &ptr2,
			 &ptr3)) != NO_TYPE)
	{
	  if (TEST_FLAG (LOCKFLAG, (LineType *) ptr2))
	    {
	      Message (_("Sorry, the object is locked\n"));
	      break;
	    }
	  if (type == ELEMENT_TYPE)
	    {
	      RubberbandType *ptr;
	      int i;

	      Crosshair.AttachedObject.RubberbandN = 0;
	      LookupRatLines (type, ptr1, ptr2, ptr3);
	      ptr = Crosshair.AttachedObject.Rubberband;
	      for (i = 0; i < Crosshair.AttachedObject.RubberbandN; i++)
		{
		  if (PCB->RatOn)
		    EraseRat ((RatType *) ptr->Line);
                  if (TEST_FLAG (RUBBERENDFLAG, ptr->Line))
		    MoveObjectToRemoveUndoList (RATLINE_TYPE,
					        ptr->Line, ptr->Line,
					        ptr->Line);
                  else
                    TOGGLE_FLAG (RUBBERENDFLAG, ptr->Line); /* only remove line once */
		  ptr++;
		}
	    }
	  RemoveObject (type, ptr1, ptr2, ptr3);
	  IncrementUndoSerialNumber ();
	  SetChangedFlag (true);
	}
      break;

    case ROTATE_MODE:
      RotateScreenObject (Note.X, Note.Y,
			  gui->shift_is_pressed ()? (SWAP_IDENT ?
						     1 : 3)
			  : (SWAP_IDENT ? 3 : 1));
      break;

      /* both are almost the same */
    case COPY_MODE:
    case MOVE_MODE:
      switch (Crosshair.AttachedObject.State)
	{
	  /* first notify, lookup object */
	case STATE_FIRST:
	  {
	    int types = (Settings.Mode == COPY_MODE) ?
	      COPY_TYPES : MOVE_TYPES;

	    Crosshair.AttachedObject.Type =
	      SearchScreen (Note.X, Note.Y, types,
			    &Crosshair.AttachedObject.Ptr1,
			    &Crosshair.AttachedObject.Ptr2,
			    &Crosshair.AttachedObject.Ptr3);
	    if (Crosshair.AttachedObject.Type != NO_TYPE)
	      {
		if (Settings.Mode == MOVE_MODE &&
		    TEST_FLAG (LOCKFLAG, (PinType *)
			       Crosshair.AttachedObject.Ptr2))
		  {
		    Message (_("Sorry, the object is locked\n"));
		    Crosshair.AttachedObject.Type = NO_TYPE;
		  }
		else
		  AttachForCopy (Note.X, Note.Y);
	      }
	    break;
	  }

	  /* second notify, move or copy object */
	case STATE_SECOND:
	  if (Settings.Mode == COPY_MODE)
	    CopyObject (Crosshair.AttachedObject.Type,
			Crosshair.AttachedObject.Ptr1,
			Crosshair.AttachedObject.Ptr2,
			Crosshair.AttachedObject.Ptr3,
			Note.X - Crosshair.AttachedObject.X,
			Note.Y - Crosshair.AttachedObject.Y);
	  else
	    {
	      MoveObjectAndRubberband (Crosshair.AttachedObject.Type,
				       Crosshair.AttachedObject.Ptr1,
				       Crosshair.AttachedObject.Ptr2,
				       Crosshair.AttachedObject.Ptr3,
				       Note.X - Crosshair.AttachedObject.X,
				       Note.Y - Crosshair.AttachedObject.Y);
	      SetLocalRef (0, 0, false);
	    }
	  SetChangedFlag (true);

	  /* reset identifiers */
	  Crosshair.AttachedObject.Type = NO_TYPE;
	  Crosshair.AttachedObject.State = STATE_FIRST;
	  break;
	}
      break;

      /* insert a point into a polygon/line/... */
    case INSERTPOINT_MODE:
      switch (Crosshair.AttachedObject.State)
	{
	  /* first notify, lookup object */
	case STATE_FIRST:
	  Crosshair.AttachedObject.Type =
	    SearchScreen (Note.X, Note.Y, INSERT_TYPES,
			  &Crosshair.AttachedObject.Ptr1,
			  &Crosshair.AttachedObject.Ptr2,
			  &Crosshair.AttachedObject.Ptr3);

	  if (Crosshair.AttachedObject.Type != NO_TYPE)
	    {
	      if (TEST_FLAG (LOCKFLAG, (PolygonType *)
			     Crosshair.AttachedObject.Ptr2))
		{
		  Message (_("Sorry, the object is locked\n"));
		  Crosshair.AttachedObject.Type = NO_TYPE;
		  break;
		}
	      else
		{
		  /* get starting point of nearest segment */
		  if (Crosshair.AttachedObject.Type == POLYGON_TYPE)
		    {
		      fake.poly =
			(PolygonType *) Crosshair.AttachedObject.Ptr2;
		      polyIndex =
			GetLowestDistancePolygonPoint (fake.poly, Note.X,
						       Note.Y);
		      fake.line.Point1 = fake.poly->Points[polyIndex];
		      fake.line.Point2 = fake.poly->Points[
			  prev_contour_point (fake.poly, polyIndex)];
		      Crosshair.AttachedObject.Ptr2 = &fake.line;

		    }
		  Crosshair.AttachedObject.State = STATE_SECOND;
		  InsertedPoint = *AdjustInsertPoint ();
		}
	    }
	  break;

	  /* second notify, insert new point into object */
	case STATE_SECOND:
	  if (Crosshair.AttachedObject.Type == POLYGON_TYPE)
	    InsertPointIntoObject (POLYGON_TYPE,
				   Crosshair.AttachedObject.Ptr1, fake.poly,
				   &polyIndex,
				   InsertedPoint.X, InsertedPoint.Y, false, false);
	  else
	    InsertPointIntoObject (Crosshair.AttachedObject.Type,
				   Crosshair.AttachedObject.Ptr1,
				   Crosshair.AttachedObject.Ptr2,
				   &polyIndex,
				   InsertedPoint.X, InsertedPoint.Y, false, false);
	  SetChangedFlag (true);

	  /* reset identifiers */
	  Crosshair.AttachedObject.Type = NO_TYPE;
	  Crosshair.AttachedObject.State = STATE_FIRST;
	  break;
	}
      break;
    }
}


/* --------------------------------------------------------------------------- */

static const char atomic_syntax[] = N_("Atomic(Save|Restore|Close|Block)");

static const char atomic_help[] = N_("Save or restore the undo serial number.");

/* %start-doc actions Atomic

This action allows making multiple-action bindings into an atomic
operation that will be undone by a single Undo command.  For example,
to optimize rat lines, you'd delete the rats and re-add them.  To
group these into a single undo, you'd want the deletions and the
additions to have the same undo serial number.  So, you @code{Save},
delete the rats, @code{Restore}, add the rats - using the same serial
number as the deletes, then @code{Block}, which checks to see if the
deletions or additions actually did anything.  If not, the serial
number is set to the saved number, as there's nothing to undo.  If
something did happen, the serial number is incremented so that these
actions are counted as a single undo step.

@table @code

@item Save
Saves the undo serial number.

@item Restore
Returns it to the last saved number.

@item Close
Sets it to 1 greater than the last save.

@item Block
Does a Restore if there was nothing to undo, else does a Close.

@end table

%end-doc */

static int
ActionAtomic (int argc, char **argv, Coord x, Coord y)
{
  if (argc != 1)
    AFAIL (atomic);

  switch (GetFunctionID (argv[0]))
    {
    case F_Save:
      SaveUndoSerialNumber ();
      break;
    case F_Restore:
      RestoreUndoSerialNumber ();
      break;
    case F_Close:
      RestoreUndoSerialNumber ();
      IncrementUndoSerialNumber ();
      break;
    case F_Block:
      RestoreUndoSerialNumber ();
      if (Bumped)
	IncrementUndoSerialNumber ();
      break;
    }
  return 0;
}

/* -------------------------------------------------------------------------- */

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

/* -------------------------------------------------------------------------- */

static const char dumplibrary_syntax[] = N_("DumpLibrary()");

static const char dumplibrary_help[] =
  N_("Display the entire contents of the libraries.");

/* %start-doc actions DumpLibrary


%end-doc */

static int
ActionDumpLibrary (int argc, char **argv, Coord x, Coord y)
{
  int i, j;

  printf ("**** Do not count on this format.  It will change ****\n\n");
  printf ("MenuN   = %d\n", (int) Library.MenuN);
  printf ("MenuMax = %d\n", (int) Library.MenuMax);
  for (i = 0; i < Library.MenuN; i++)
    {
      printf ("Library #%d:\n", i);
      printf ("    EntryN    = %d\n", (int) Library.Menu[i].EntryN);
      printf ("    EntryMax  = %d\n", (int) Library.Menu[i].EntryMax);
      printf ("    Name      = \"%s\"\n", UNKNOWN (Library.Menu[i].Name));
      printf ("    directory = \"%s\"\n",
	      UNKNOWN (Library.Menu[i].directory));
      printf ("    Style     = \"%s\"\n", UNKNOWN (Library.Menu[i].Style));
      printf ("    flag      = %d\n", Library.Menu[i].flag);

      for (j = 0; j < Library.Menu[i].EntryN; j++)
	{
	  printf ("    #%4d: ", j);
	  if (Library.Menu[i].Entry[j].Template == (char *) -1)
	    {
	      printf ("newlib: \"%s\"\n",
		      UNKNOWN (Library.Menu[i].Entry[j].ListEntry));
	    }
	  else
	    {
	      printf ("\"%s\", \"%s\", \"%s\", \"%s\", \"%s\"\n",
		      UNKNOWN (Library.Menu[i].Entry[j].ListEntry),
		      UNKNOWN (Library.Menu[i].Entry[j].Template),
		      UNKNOWN (Library.Menu[i].Entry[j].Package),
		      UNKNOWN (Library.Menu[i].Entry[j].Value),
		      UNKNOWN (Library.Menu[i].Entry[j].Description));
	    }
	}
    }

  return 0;
}

/* -------------------------------------------------------------------------- */

static const char flip_syntax[] = N_("Flip(Object|Selected|SelectedElements)");

static const char flip_help[] =
  N_("Flip an element to the opposite side of the board.");

/* %start-doc actions Flip

Note that the location of the element will be symmetric about the
cursor location; i.e. if the part you are pointing at will still be at
the same spot once the element is on the other side.  When flipping
multiple elements, this retains their positions relative to each
other, not their absolute positions on the board.

%end-doc */

static int
ActionFlip (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  ElementType *element;
  void *ptrtmp;
  int err = 0;

  if (function)
    {
      switch (GetFunctionID (function))
	{
	case F_Object:
	  if ((SearchScreen (x, y, ELEMENT_TYPE,
			     &ptrtmp, &ptrtmp, &ptrtmp)) != NO_TYPE)
	    {
	      element = (ElementType *) ptrtmp;
	      ChangeElementSide (element, 2 * Crosshair.Y - PCB->MaxHeight);
	      IncrementUndoSerialNumber ();
	      Draw ();
	    }
	  break;
	case F_Selected:
	case F_SelectedElements:
	  ChangeSelectedElementSide ();
	  break;
	default:
	  err = 1;
	  break;
	}
      if (!err)
	return 0;
    }

  AFAIL (flip);
}

/* -------------------------------------------------------------------------- */

static const char message_syntax[] = N_("Message(message)");

static const char message_help[] = N_("Writes a message to the log window.");

/* %start-doc actions Message

This action displays a message to the log window.  This action is primarily
provided for use by other programs which may interface with PCB.  If
multiple arguments are given, each one is sent to the log window
followed by a newline.

%end-doc */

static int
ActionMessage (int argc, char **argv, Coord x, Coord y)
{
  int i;

  if (argc < 1)
    AFAIL (message);

  for (i = 0; i < argc; i++)
    {
      Message (argv[i]);
      Message ("\n");
    }

  return 0;
}


/* -------------------------------------------------------------------------- */

static const char setthermal_syntax[] =
  "SetThermal(Object|SelectedPins|SelectedVias|Selected, Style)";

static const char setthermal_help[] =
  N_("Set the thermal (on the current layer) of pins or vias to the given style.\n"
  "Style = 0 means no thermal.\n"
  "Style = 1 has diagonal fingers with sharp edges.\n"
  "Style = 2 has horizontal and vertical fingers with sharp edges.\n"
  "Style = 3 is a solid connection to the plane.\n"
  "Style = 4 has diagonal fingers with rounded edges.\n"
  "Style = 5 has horizontal and vertical fingers with rounded edges.\n");

/* %start-doc actions SetThermal

This changes how/whether pins or vias connect to any rectangle or polygon
on the current layer. The first argument can specify one object, or all
selected pins, or all selected vias, or all selected pins and vias.
The second argument specifies the style of connection.
There are 5 possibilities:
0 - no connection,
1 - 45 degree fingers with sharp edges,
2 - horizontal & vertical fingers with sharp edges,
3 - solid connection,
4 - 45 degree fingers with rounded corners,
5 - horizontal & vertical fingers with rounded corners.

Pins and Vias may have thermals whether or not there is a polygon available 
to connect with. However, they will have no effect without the polygon.
%end-doc */

static int
ActionSetThermal (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  char *style = ARG (1);
  void *ptr1, *ptr2, *ptr3;
  int type, kind;
  int err = 0;

  if (function && *function)
    {
      bool absolute;

      if ( ! style || ! *style)
	{
	  kind = PCB->ThermStyle;
	  absolute = true;
	}
      else
	kind = GetUnitlessValue (style, &absolute);

      /* To allow relative values we could search for the first selected
	 item and make 'kind' relative to that, but that's not too useful
	 and requires quite some code. For example there's no
	 GetFirstSelectedPin() function available. Let's postpone this
	 functionality, there are more urgent things to do. */

      if (absolute)
	switch (GetFunctionID (function))
	  {
	  case F_Object:
	    if ((type =
		 SearchScreen (Crosshair.X, Crosshair.Y, CHANGETHERMAL_TYPES,
			       &ptr1, &ptr2, &ptr3)) != NO_TYPE)
	      {
		ChangeObjectThermal (type, ptr1, ptr2, ptr3, kind);
		IncrementUndoSerialNumber ();
		Draw ();
	      }
	    break;
	  case F_SelectedPins:
	    ChangeSelectedThermals (PIN_TYPE, kind);
	    break;
	  case F_SelectedVias:
	    ChangeSelectedThermals (VIA_TYPE, kind);
	    break;
	  case F_Selected:
	  case F_SelectedElements:
	    ChangeSelectedThermals (CHANGETHERMAL_TYPES, kind);
	    break;
	  default:
	    err = 1;
	    break;
	  }
      else
	err = 1;
    }
  else
    err = 1;

  if (err)
    AFAIL (setthermal);

  return 0;
}

/*!
 * \brief Event handler to set the cursor according to the X pointer
 * position called from inside main.c.
 *
 * \warning !!! no action routine !!!
 */
void
EventMoveCrosshair (int ev_x, int ev_y)
{
#ifdef HAVE_LIBSTROKE
  if (mid_stroke)
    {
      StrokeBox.X2 = ev_x;
      StrokeBox.Y2 = ev_y;
      stroke_record (ev_x, ev_y);
      return;
    }
#endif /* HAVE_LIBSTROKE */
  if (MoveCrosshairAbsolute (ev_x, ev_y))
    {
      /* update object position and cursor location */
      AdjustAttachedObjects ();
      notify_crosshair_change (true);
    }
}

/* --------------------------------------------------------------------------- */

static const char setvalue_syntax[] =
  N_("SetValue(Grid|Line|LineSize|Text|TextScale|ViaDrillingHole|Via|ViaSize, "
      "delta)");

static const char setvalue_help[] =
  N_("Change various board-wide values and sizes.");

/* %start-doc actions SetValue

@table @code

@item ViaDrillingHole
Changes the diameter of the drill for new vias.

@item Grid
Sets the grid spacing.

@item Line
@item LineSize
Changes the thickness of new lines.

@item Via
@item ViaSize
Changes the diameter of new vias.

@item Text
@item TextScale
Changes the size of new text.

@end table

%end-doc */

static int
ActionSetValue (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  char *val = ARG (1);
  char *units = ARG (2);
  bool absolute;			/* flag for 'absolute' value */
  double value;
  int text_scale;
  int err = 0;

  if (function && val)
    {
      value = GetValue (val, units, &absolute);
      switch (GetFunctionID (function))
	{
	case F_ViaDrillingHole:
	  SetViaDrillingHole (absolute ? value :
                                value + Settings.ViaDrillingHole,
			      false);
	  hid_action ("RouteStylesChanged");
	  break;

	case F_Grid:
	  if (absolute)
	    SetGrid (value, false);
	  else
	    {
              if (value == 0)
                value = val[0] == '-' ? -Settings.increments->grid
                                      :  Settings.increments->grid;
              /* On the way down, short against the minimum 
               * PCB drawing unit */
              if ((value + PCB->Grid) < 1)
                SetGrid (1, false);
              else if (PCB->Grid == 1)
                SetGrid (value, false);
              else
                SetGrid (value + PCB->Grid, false);
	    }
	  break;

	case F_LineSize:
	case F_Line:
          if (!absolute && value == 0)
            value = val[0] == '-' ? -Settings.increments->line
                                  :  Settings.increments->line;
	  SetLineSize (absolute ? value : value + Settings.LineThickness);
	  hid_action ("RouteStylesChanged");
	  break;

	case F_Via:
	case F_ViaSize:
	  SetViaSize (absolute ? value : value + Settings.ViaThickness, false);
	  hid_action ("RouteStylesChanged");
	  break;

	case F_Text:
	case F_TextScale:
	  text_scale = value / (double)FONT_CAPHEIGHT * 100.;
	  if (!absolute)
	    text_scale += Settings.TextScale;
	  SetTextScale (text_scale);
	  break;
	default:
	  err = 1;
	  break;
	}
      if (!err)
	return 0;
    }

  AFAIL (setvalue);
}


/* --------------------------------------------------------------------------- */

static const char quit_syntax[] = N_("Quit()");

static const char quit_help[] = N_("Quits the application after confirming.");

/* %start-doc actions Quit

If you have unsaved changes, you will be prompted to confirm (or
save) before quitting.

%end-doc */

static int
ActionQuit (int argc, char **argv, Coord x, Coord y)
{
  char *force = ARG (0);
  if (force && strcasecmp (force, "force") == 0)
    {
      PCB->Changed = 0;
      exit (0);
    }
  if (!PCB->Changed || gui->close_confirm_dialog () == HID_CLOSE_CONFIRM_OK)
    QuitApplication ();
  return 1;
}

/* --------------------------------------------------------------------------- */

static const char connection_syntax[] =
  N_("Connection(Find|ResetLinesAndPolygons|ResetPinsAndVias|Reset)");

static const char connection_help[] =
  N_("Searches connections of the object at the cursor position.");

/* %start-doc actions Connection

Connections found with this action will be highlighted in the
``connected-color'' color and will have the ``found'' flag set.

@table @code

@item Find
The net under the cursor is ``found''.

@item ResetLinesAndPolygons
Any ``found'' lines and polygons are marked ``not found''.

@item ResetPinsAndVias
Any ``found'' pins and vias are marked ``not found''.

@item Reset
All ``found'' objects are marked ``not found''.

@end table

%end-doc */

static int
ActionConnection (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  if (function)
    {
      switch (GetFunctionID (function))
	{
	case F_Find:
	  {
	    gui->get_coords (_("Click on a connection"), &x, &y);
	    LookupConnection (x, y, true, 1, CONNECTEDFLAG, false);
	    LookupConnection (x, y, true, 1, FOUNDFLAG, true);
	    break;
	  }

	case F_ResetLinesAndPolygons:
	  if (ClearFlagOnLinesAndPolygons (true, CONNECTEDFLAG | FOUNDFLAG))
	    {
	      IncrementUndoSerialNumber ();
	      Draw ();
	    }
	  break;

	case F_ResetPinsViasAndPads:
	  if (ClearFlagOnPinsViasAndPads (true, CONNECTEDFLAG | FOUNDFLAG))
	    {
	      IncrementUndoSerialNumber ();
	      Draw ();
	    }
	  break;

	case F_Reset:
	  if (ClearFlagOnAllObjects (true, CONNECTEDFLAG | FOUNDFLAG))
	    {
	      IncrementUndoSerialNumber ();
	      Draw ();
	    }
	  break;
	}
      return 0;
    }

  AFAIL (connection);
}

/* --------------------------------------------------------------------------- */

static const char disperseelements_syntax[] =
  N_("DisperseElements(All|Selected)");

static const char disperseelements_help[] = N_("Disperses elements.");

/* %start-doc actions DisperseElements

Normally this is used when starting a board, by selecting all elements
and then dispersing them.  This scatters the elements around the board
so that you can pick individual ones, rather than have all the
elements at the same 0,0 coordinate and thus impossible to choose
from.

%end-doc */

#define GAP MIL_TO_COORD(100)

static int
ActionDisperseElements (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  Coord minx = GAP,
    miny = GAP,
    maxy = GAP,
    dx, dy;
  int all = 0, bad = 0;

  if (!function || !*function)
    {
      bad = 1;
    }
  else
    {
      switch (GetFunctionID (function))
	{
	case F_All:
	  all = 1;
	  break;

	case F_Selected:
	  all = 0;
	  break;

	default:
	  bad = 1;
	}
    }

  if (bad)
    {
      AFAIL (disperseelements);
    }


  ELEMENT_LOOP (PCB->Data);
  {
    /* 
     * If we want to disperse selected elements, maybe we need smarter
     * code here to avoid putting components on top of others which
     * are not selected.  For now, I'm assuming that this is typically
     * going to be used either with a brand new design or a scratch
     * design holding some new components
     */
    if (!TEST_FLAG (LOCKFLAG, element) && (all || TEST_FLAG (SELECTEDFLAG, element)))
      {

	/* figure out how much to move the element */
	dx = minx - element->BoundingBox.X1;

	/* snap to the grid */
	dx -= (element->MarkX + dx) % PCB->Grid;

	/* 
	 * and add one grid size so we make sure we always space by GAP or
	 * more
	 */
	dx += PCB->Grid;

	/* Figure out if this row has room.  If not, start a new row */
	if (GAP + element->BoundingBox.X2 + dx > PCB->MaxWidth)
	  {
	    miny = maxy + GAP;
	    minx = GAP;
	  }

	/* figure out how much to move the element */
	dx = minx - element->BoundingBox.X1;
	dy = miny - element->BoundingBox.Y1;

	/* snap to the grid */
	dx -= (element->MarkX + dx) % PCB->Grid;
	dx += PCB->Grid;
	dy -= (element->MarkY + dy) % PCB->Grid;
	dy += PCB->Grid;

	/* move the element */
	MoveElementLowLevel (PCB->Data, element, dx, dy);

	/* and add to the undo list so we can undo this operation */
	AddObjectToMoveUndoList (ELEMENT_TYPE, NULL, NULL, element, dx, dy);

	/* keep track of how tall this row is */
	minx += element->BoundingBox.X2 - element->BoundingBox.X1 + GAP;
	if (maxy < element->BoundingBox.Y2)
	  {
	    maxy = element->BoundingBox.Y2;
	  }
      }

  }
  END_LOOP;

  /* done with our action so increment the undo # */
  IncrementUndoSerialNumber ();

  Redraw ();
  SetChangedFlag (true);

  return 0;
}

#undef GAP

/* --------------------------------------------------------------------------- */

static const char display_syntax[] =
  N_("Display(NameOnPCB|Description|Value)\n"
  "Display(Grid|Redraw)\n"
  "Display(CycleClip|CycleCrosshair|Toggle45Degree|ToggleStartDirection)\n"
  "Display(ToggleGrid|ToggleRubberBandMode|ToggleUniqueNames)\n"
  "Display(ToggleMask|ToggleName|ToggleClearLine|ToggleFullPoly|ToggleSnapPin)\n"
  "Display(ToggleThindraw|ToggleThindrawPoly|ToggleOrthoMove|ToggleLocalRef)\n"
  "Display(ToggleCheckPlanes|ToggleShowDRC|ToggleAutoDRC)\n"
  "Display(ToggleLiveRoute|LockNames|OnlyNames)\n"
  "Display(Pinout|PinOrPadName)");

static const char display_help[] = N_("Several display-related actions.");

/* %start-doc actions Display

@table @code

@item NameOnPCB
@item Description
@item Value
Specify whether all elements show their name, description, or value.

@item Redraw
Redraw the whole board.

@item Toggle45Degree
When clear, lines can be drawn at any angle.  When set, lines are
restricted to multiples of 45 degrees and requested lines may be
broken up according to the clip setting.

@item CycleClip
Changes the way lines are restricted to 45 degree increments.  The
various settings are: straight only, orthogonal then angled, and angled
then orthogonal.  If AllDirections is set, this action disables it.

@item CycleCrosshair
Changes crosshair drawing.  Crosshair may accept form of 4-ray,
8-ray and 12-ray cross.

@item ToggleRubberBandMode
If set, moving an object moves all the lines attached to it too.

@item ToggleStartDirection
If set, each time you set a point in a line, the Clip toggles between
orth-angle and angle-ortho.

@item ToggleUniqueNames
If set, you will not be permitted to change the name of an element to
match that of another element.

@item ToggleSnapPin
If set, pin centers and pad end points are treated as additional grid
points that the cursor can snap to.

@item ToggleLocalRef
If set, the mark is automatically set to the beginning of any move, so
you can see the relative distance you've moved.

@item ToggleThindraw
If set, objects on the screen are drawn as outlines (lines are drawn
as center-lines).  This lets you see line endpoints hidden under pins,
for example.

@item ToggleThindrawPoly
If set, polygons on the screen are drawn as outlines.

@item ToggleShowDRC
If set, pending objects (i.e. lines you're in the process of drawing)
will be drawn with an outline showing how far away from other copper
you need to be.

@item ToggleLiveRoute
If set, the progress of the autorouter will be visible on the screen.

@item ToggleAutoDRC
If set, you will not be permitted to make connections which violate
the current DRC and netlist settings.

@item ToggleCheckPlanes
If set, lines and arcs aren't drawn, which usually leaves just the
polygons.  If you also disable all but the layer you're interested in,
this allows you to check for isolated regions.

@item ToggleOrthoMove
If set, the crosshair is only allowed to move orthogonally from its
previous position.  I.e. you can move an element or line up, down,
left, or right, but not up+left or down+right.

@item ToggleName
Selects whether the pinouts show the pin names or the pin numbers.

@item ToggleLockNames
If set, text will ignore left mouse clicks and actions that work on
objects under the mouse. You can still select text with a lasso (left
mouse drag) and perform actions on the selection.

@item ToggleOnlyNames
If set, only text will be sensitive for mouse clicks and actions that
work on objects under the mouse. You can still select other objects
with a lasso (left mouse drag) and perform actions on the selection.

@item ToggleMask
Turns the solder mask on or off.

@item ToggleClearLine
When set, the clear-line flag causes new lines and arcs to have their
``clear polygons'' flag set, so they won't be electrically connected
to any polygons they overlap.

@item ToggleFullPoly
When set, the full-poly flag causes new polygons to have their
``full polygon'' flag set, so all parts of them will be displayed
instead of only the biggest one.

@item ToggleGrid
Resets the origin of the current grid to be wherever the mouse pointer
is (not where the crosshair currently is).  If you provide two numbers
after this, the origin is set to that coordinate.

@item Grid
Toggles whether the grid is displayed or not.

@item Pinout
Causes the pinout of the element indicated by the cursor to be
displayed, usually in a separate window.

@item PinOrPadName
Toggles whether the names of pins, pads, or (yes) vias will be
displayed.  If the cursor is over an element, all of its pins and pads
are affected.

@end table

%end-doc */

static enum crosshair_shape
CrosshairShapeIncrement (enum crosshair_shape shape)
{
  switch(shape)
    {
    case Basic_Crosshair_Shape:
      shape = Union_Jack_Crosshair_Shape;
      break;
    case Union_Jack_Crosshair_Shape:
      shape = Dozen_Crosshair_Shape;
      break;
    case Dozen_Crosshair_Shape:
      shape = Crosshair_Shapes_Number;
      break;
    case Crosshair_Shapes_Number:
      shape = Basic_Crosshair_Shape;
      break;
    }
  return shape;
}

static int
ActionDisplay (int argc, char **argv, Coord childX, Coord childY)
{
  char *function, *str_dir;
  int id;
  int err = 0;

  function = ARG (0);
  str_dir = ARG (1);

  if (function && (!str_dir || !*str_dir))
    {
      switch (id = GetFunctionID (function))
	{

	  /* redraw layout */
	case F_ClearAndRedraw:
	case F_Redraw:
	  Redraw ();
	  break;

	  /* change the displayed name of elements */
	case F_Value:
	case F_NameOnPCB:
	case F_Description:
	  ELEMENT_LOOP (PCB->Data);
	  {
	    EraseElementName (element);
	  }
	  END_LOOP;
	  CLEAR_FLAG (DESCRIPTIONFLAG | NAMEONPCBFLAG, PCB);
	  switch (id)
	    {
	    case F_Value:
	      break;
	    case F_NameOnPCB:
	      SET_FLAG (NAMEONPCBFLAG, PCB);
	      break;
	    case F_Description:
	      SET_FLAG (DESCRIPTIONFLAG, PCB);
	      break;
	    }
	  ELEMENT_LOOP (PCB->Data);
	  {
	    DrawElementName (element);
	  }
	  END_LOOP;
	  Draw ();
	  break;

	  /* toggle line-adjust flag */
	case F_ToggleAllDirections:
	  TOGGLE_FLAG (ALLDIRECTIONFLAG, PCB);
	  AdjustAttachedObjects ();
	  break;

	case F_CycleClip:
	  notify_crosshair_change (false);
	  if (TEST_FLAG (ALLDIRECTIONFLAG, PCB))
	    {
	      TOGGLE_FLAG (ALLDIRECTIONFLAG, PCB);
	      PCB->Clipping = 0;
	    }
	  else
	    PCB->Clipping = (PCB->Clipping + 1) % 3;
	  AdjustAttachedObjects ();
	  notify_crosshair_change (true);
	  break;

	case F_CycleCrosshair:
	  notify_crosshair_change (false);
	  Crosshair.shape = CrosshairShapeIncrement(Crosshair.shape);
	  if (Crosshair_Shapes_Number == Crosshair.shape)
	    Crosshair.shape = Basic_Crosshair_Shape;
	  notify_crosshair_change (true);
	  break;

	case F_ToggleRubberBandMode:
	  notify_crosshair_change (false);
	  TOGGLE_FLAG (RUBBERBANDFLAG, PCB);
	  notify_crosshair_change (true);
	  break;

	case F_ToggleStartDirection:
	  notify_crosshair_change (false);
	  TOGGLE_FLAG (SWAPSTARTDIRFLAG, PCB);
	  notify_crosshair_change (true);
	  break;

	case F_ToggleUniqueNames:
	  TOGGLE_FLAG (UNIQUENAMEFLAG, PCB);
	  break;

	case F_ToggleSnapPin:
	  notify_crosshair_change (false);
	  TOGGLE_FLAG (SNAPPINFLAG, PCB);
	  notify_crosshair_change (true);
	  break;

	case F_ToggleLocalRef:
	  TOGGLE_FLAG (LOCALREFFLAG, PCB);
	  break;

	case F_ToggleThindraw:
	  TOGGLE_FLAG (THINDRAWFLAG, PCB);
	  Redraw ();
	  break;

	case F_ToggleThindrawPoly:
	  TOGGLE_FLAG (THINDRAWPOLYFLAG, PCB);
	  Redraw ();
	  break;

	case F_ToggleLockNames:
	  TOGGLE_FLAG (LOCKNAMESFLAG, PCB);
	  CLEAR_FLAG (ONLYNAMESFLAG, PCB);
	  break;

	case F_ToggleOnlyNames:
	  TOGGLE_FLAG (ONLYNAMESFLAG, PCB);
	  CLEAR_FLAG (LOCKNAMESFLAG, PCB);
	  break;

	case F_ToggleHideNames:
	  TOGGLE_FLAG (HIDENAMESFLAG, PCB);
	  Redraw ();
	  break;

	case F_ToggleShowDRC:
	  TOGGLE_FLAG (SHOWDRCFLAG, PCB);
	  break;

	case F_ToggleLiveRoute:
	  TOGGLE_FLAG (LIVEROUTEFLAG, PCB);
	  break;

	case F_ToggleAutoDRC:
	  notify_crosshair_change (false);
	  TOGGLE_FLAG (AUTODRCFLAG, PCB);
	  if (TEST_FLAG (AUTODRCFLAG, PCB) && Settings.Mode == LINE_MODE)
	    {
	      if (ClearFlagOnAllObjects (true, CONNECTEDFLAG | FOUNDFLAG))
		{
		  IncrementUndoSerialNumber ();
		  Draw ();
		}
	      if (Crosshair.AttachedLine.State != STATE_FIRST)
		{
		  LookupConnection (Crosshair.AttachedLine.Point1.X,
		                    Crosshair.AttachedLine.Point1.Y,
		                    true, 1, CONNECTEDFLAG, false);
		  LookupConnection (Crosshair.AttachedLine.Point1.X,
		                    Crosshair.AttachedLine.Point1.Y,
		                    true, 1, FOUNDFLAG, true);
		}
	    }
	  notify_crosshair_change (true);
	  break;

	case F_ToggleCheckPlanes:
	  TOGGLE_FLAG (CHECKPLANESFLAG, PCB);
	  Redraw ();
	  break;

	case F_ToggleOrthoMove:
	  TOGGLE_FLAG (ORTHOMOVEFLAG, PCB);
	  break;

	case F_ToggleName:
	  TOGGLE_FLAG (SHOWNUMBERFLAG, PCB);
	  Redraw ();
	  break;

	case F_ToggleMask:
	  TOGGLE_FLAG (SHOWMASKFLAG, PCB);
	  Redraw ();
	  break;

	case F_ToggleClearLine:
	  TOGGLE_FLAG (CLEARNEWFLAG, PCB);
	  break;

	case F_ToggleFullPoly:
	  TOGGLE_FLAG (NEWFULLPOLYFLAG, PCB);
	  break;

	  /* shift grid alignment */
	case F_ToggleGrid:
	  {
	    Coord oldGrid = PCB->Grid;

	    PCB->Grid = 1;
	    if (MoveCrosshairAbsolute (Crosshair.X, Crosshair.Y))
	      notify_crosshair_change (true);	/* first notify was in MoveCrosshairAbs */
	    SetGrid (oldGrid, true);
	  }
	  break;

	  /* toggle displaying of the grid */
	case F_Grid:
	  Settings.DrawGrid = !Settings.DrawGrid;
	  Redraw ();
	  break;

	  /* display the pinout of an element */
	case F_Pinout:
	  {
	    ElementType *element;
	    void *ptrtmp;
	    Coord x, y;

	    gui->get_coords (_("Click on an element"), &x, &y);
	    if ((SearchScreen
		 (x, y, ELEMENT_TYPE, &ptrtmp,
		  &ptrtmp, &ptrtmp)) != NO_TYPE)
	      {
		element = (ElementType *) ptrtmp;
		gui->show_item (element);
	      }
	    break;
	  }

	  /* toggle displaying of pin/pad/via names */
	case F_PinOrPadName:
	  {
	    void *ptr1, *ptr2, *ptr3;
	    Coord x, y;

	    gui->get_coords(_("Click on an element"), &x, &y);

	    switch (SearchScreen (x, y,
				  ELEMENT_TYPE | PIN_TYPE | PAD_TYPE |
				  VIA_TYPE, (void **) &ptr1, (void **) &ptr2,
				  (void **) &ptr3))
	      {
	      case ELEMENT_TYPE:
		PIN_LOOP ((ElementType *) ptr1);
		{
		  if (TEST_FLAG (DISPLAYNAMEFLAG, pin))
		    ErasePinName (pin);
		  else
		    DrawPinName (pin);
		  AddObjectToFlagUndoList (PIN_TYPE, ptr1, pin, pin);
		  TOGGLE_FLAG (DISPLAYNAMEFLAG, pin);
		}
		END_LOOP;
		PAD_LOOP ((ElementType *) ptr1);
		{
		  if (TEST_FLAG (DISPLAYNAMEFLAG, pad))
		    ErasePadName (pad);
		  else
		    DrawPadName (pad);
		  AddObjectToFlagUndoList (PAD_TYPE, ptr1, pad, pad);
		  TOGGLE_FLAG (DISPLAYNAMEFLAG, pad);
		}
		END_LOOP;
		SetChangedFlag (true);
		IncrementUndoSerialNumber ();
		Draw ();
		break;

	      case PIN_TYPE:
		if (TEST_FLAG (DISPLAYNAMEFLAG, (PinType *) ptr2))
		  ErasePinName ((PinType *) ptr2);
		else
		  DrawPinName ((PinType *) ptr2);
		AddObjectToFlagUndoList (PIN_TYPE, ptr1, ptr2, ptr3);
		TOGGLE_FLAG (DISPLAYNAMEFLAG, (PinType *) ptr2);
		SetChangedFlag (true);
		IncrementUndoSerialNumber ();
		Draw ();
		break;

	      case PAD_TYPE:
		if (TEST_FLAG (DISPLAYNAMEFLAG, (PadType *) ptr2))
		  ErasePadName ((PadType *) ptr2);
		else
		  DrawPadName ((PadType *) ptr2);
		AddObjectToFlagUndoList (PAD_TYPE, ptr1, ptr2, ptr3);
		TOGGLE_FLAG (DISPLAYNAMEFLAG, (PadType *) ptr2);
		SetChangedFlag (true);
		IncrementUndoSerialNumber ();
		Draw ();
		break;
	      case VIA_TYPE:
		if (TEST_FLAG (DISPLAYNAMEFLAG, (PinType *) ptr2))
		  EraseViaName ((PinType *) ptr2);
		else
		  DrawViaName ((PinType *) ptr2);
		AddObjectToFlagUndoList (VIA_TYPE, ptr1, ptr2, ptr3);
		TOGGLE_FLAG (DISPLAYNAMEFLAG, (PinType *) ptr2);
		SetChangedFlag (true);
		IncrementUndoSerialNumber ();
		Draw ();
		break;
	      }
	    break;
	  }
	default:
	  err = 1;
	}
    }
  else if (function && str_dir)
    {
      switch (GetFunctionID (function))
	{
	case F_ToggleGrid:
	  if (argc > 2)
	    {
	      PCB->GridOffsetX = GetValue (argv[1], NULL, NULL);
	      PCB->GridOffsetY = GetValue (argv[2], NULL, NULL);
	      if (Settings.DrawGrid)
		Redraw ();
	    }
	  break;

	default:
	  err = 1;
	  break;
	}
    }
  else
    err = 1;

  if (err)
    AFAIL (display);

  return 0;
}

/* --------------------------------------------------------------------------- */

static const char mode_syntax[] =
  N_("Mode(Arc|Arrow|Copy|InsertPoint|Line|Lock|Move|None|PasteBuffer)\n"
  "Mode(Polygon|Rectangle|Remove|Rotate|Text|Thermal|Via)\n"
  "Mode(Notify|Release|Cancel|Stroke)\n"
  "Mode(Save|Restore)");

static const char mode_help[] = N_("Change or use the tool mode.");

/* %start-doc actions Mode

@table @code

@item Arc
@itemx Arrow
@itemx Copy
@itemx InsertPoint
@itemx Line
@itemx Lock
@itemx Move
@itemx None
@itemx PasteBuffer
@itemx Polygon
@itemx Rectangle
@itemx Remove
@itemx Rotate
@itemx Text
@itemx Thermal
@itemx Via
Select the indicated tool.

@item Notify
Called when you press the mouse button, or move the mouse.

@item Release
Called when you release the mouse button.

@item Cancel
Cancels any pending tool activity, allowing you to restart elsewhere.
For example, this allows you to start a new line rather than attach a
line to the previous line.

@item Escape
Similar to Cancel but calling this action a second time will return
to the Arrow tool.

@item Stroke
If your @code{pcb} was built with libstroke, this invokes the stroke
input method.  If not, this will restart a drawing mode if you were
drawing, else it will select objects.

@item Save
Remembers the current tool.

@item Restore
Restores the tool to the last saved tool.

@end table

%end-doc */

static int
ActionMode (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);

  if (function)
    {
      Note.X = Crosshair.X;
      Note.Y = Crosshair.Y;
      notify_crosshair_change (false);
      switch (GetFunctionID (function))
	{
	case F_Arc:
	  SetMode (ARC_MODE);
	  break;
	case F_Arrow:
	  SetMode (ARROW_MODE);
	  break;
	case F_Copy:
	  SetMode (COPY_MODE);
	  break;
	case F_InsertPoint:
	  SetMode (INSERTPOINT_MODE);
	  break;
	case F_Line:
	  SetMode (LINE_MODE);
	  break;
	case F_Lock:
	  SetMode (LOCK_MODE);
	  break;
	case F_Move:
	  SetMode (MOVE_MODE);
	  break;
	case F_None:
	  SetMode (NO_MODE);
	  break;
	case F_Cancel:
	  {
	    int saved_mode = Settings.Mode;
	    SetMode (NO_MODE);
	    SetMode (saved_mode);
	  }
	  break;
	case F_Escape:
	  {
	    switch (Settings.Mode)
	      {
	      case VIA_MODE:
	      case PASTEBUFFER_MODE:
	      case TEXT_MODE:
	      case ROTATE_MODE:
	      case REMOVE_MODE:
	      case MOVE_MODE:
	      case COPY_MODE:
	      case INSERTPOINT_MODE:
	      case RUBBERBANDMOVE_MODE:
	      case THERMAL_MODE:
	      case LOCK_MODE:
		SetMode (NO_MODE);
		SetMode (ARROW_MODE);
		break;

	      case LINE_MODE:
		if (Crosshair.AttachedLine.State == STATE_FIRST)
		  SetMode (ARROW_MODE);
		else
		  {
		    SetMode (NO_MODE);
		    SetMode (LINE_MODE);
		  }
		break;

	      case RECTANGLE_MODE:
		if (Crosshair.AttachedBox.State == STATE_FIRST)
		  SetMode (ARROW_MODE);
		else
		  {
		    SetMode (NO_MODE);
		    SetMode (RECTANGLE_MODE);
		  }
		break;
	  
	      case POLYGON_MODE:
		if (Crosshair.AttachedLine.State == STATE_FIRST)
		  SetMode (ARROW_MODE);
		else
		  {
		    SetMode (NO_MODE);
		    SetMode (POLYGON_MODE);
		  }
		break;

	      case POLYGONHOLE_MODE:
		if (Crosshair.AttachedLine.State == STATE_FIRST)
		  SetMode (ARROW_MODE);
		else
		  {
		    SetMode (NO_MODE);
		    SetMode (POLYGONHOLE_MODE);
		  }
		break;

	      case ARC_MODE:
		if (Crosshair.AttachedBox.State == STATE_FIRST)
		  SetMode (ARROW_MODE);
		else
		  {
		    SetMode (NO_MODE);
		    SetMode (ARC_MODE);
		  }
		break;
		
	      case ARROW_MODE:
		break;
		
	      default:
		break;
	      }
	  }
	  break;
	  
	case F_Notify:
	  NotifyMode ();
	  break;
	case F_PasteBuffer:
	  SetMode (PASTEBUFFER_MODE);
	  break;
	case F_Polygon:
	  SetMode (POLYGON_MODE);
	  break;
	case F_PolygonHole:
	  SetMode (POLYGONHOLE_MODE);
	  break;
#ifndef HAVE_LIBSTROKE
	case F_Release:
	  ReleaseMode ();
	  break;
#else
	case F_Release:
	  if (mid_stroke)
	    FinishStroke ();
	  else
	    ReleaseMode ();
	  break;
#endif
	case F_Remove:
	  SetMode (REMOVE_MODE);
	  break;
	case F_Rectangle:
	  SetMode (RECTANGLE_MODE);
	  break;
	case F_Rotate:
	  SetMode (ROTATE_MODE);
	  break;
	case F_Stroke:
#ifdef HAVE_LIBSTROKE
	  mid_stroke = true;
	  StrokeBox.X1 = Crosshair.X;
	  StrokeBox.Y1 = Crosshair.Y;
	  break;
#else
	  /* Handle middle mouse button restarts of drawing mode.  If not in
	     |  a drawing mode, middle mouse button will select objects.
	   */
	  if (Settings.Mode == LINE_MODE
	      && Crosshair.AttachedLine.State != STATE_FIRST)
	    {
	      SetMode (LINE_MODE);
	    }
	  else if (Settings.Mode == ARC_MODE
		   && Crosshair.AttachedBox.State != STATE_FIRST)
	    SetMode (ARC_MODE);
	  else if (Settings.Mode == RECTANGLE_MODE
		   && Crosshair.AttachedBox.State != STATE_FIRST)
	    SetMode (RECTANGLE_MODE);
	  else if (Settings.Mode == POLYGON_MODE
		   && Crosshair.AttachedLine.State != STATE_FIRST)
	    SetMode (POLYGON_MODE);
	  else
	    {
	      SaveMode ();
	      saved_mode = true;
	      SetMode (ARROW_MODE);
	      NotifyMode ();
	    }
	  break;
#endif
	case F_Text:
	  SetMode (TEXT_MODE);
	  break;
	case F_Thermal:
	  SetMode (THERMAL_MODE);
	  break;
	case F_Via:
	  SetMode (VIA_MODE);
	  break;

	case F_Restore:	/* restore the last saved mode */
	  RestoreMode ();
	  break;

	case F_Save:		/* save currently selected mode */
	  SaveMode ();
	  break;
	}
      notify_crosshair_change (true);
      return 0;
    }

  AFAIL (mode);
}

/* --------------------------------------------------------------------------- */

static const char removeselected_syntax[] = N_("RemoveSelected()");

static const char removeselected_help[] = N_("Removes any selected objects.");

/* %start-doc actions RemoveSelected

%end-doc */

static int
ActionRemoveSelected (int argc, char **argv, Coord x, Coord y)
{
  if (RemoveSelected ())
    SetChangedFlag (true);
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char renumber_syntax[] = N_("Renumber()\n"
                                      "Renumber(filename)");

static const char renumber_help[] =
  N_("Renumber all elements.  The changes will be recorded to filename\n"
  "for use in backannotating these changes to the schematic.");

/* %start-doc actions Renumber

%end-doc */

static int
ActionRenumber (int argc, char **argv, Coord x, Coord y)
{
  bool changed = false;
  ElementType **element_list;
  ElementType **locked_element_list;
  unsigned int i, j, k, cnt, lock_cnt;
  unsigned int tmpi;
  size_t sz;
  char *tmps;
  char *name;
  FILE *out;
  static char * default_file = NULL;
  size_t cnt_list_sz = 100;
  struct _cnt_list
  {
    char *name;
    unsigned int cnt;
  } *cnt_list;
  char **was, **is, *pin;
  unsigned int c_cnt = 0;
  int unique, ok;
  int free_name = 0;

  if (argc < 1)
    {
      /*
       * We deal with the case where name already exists in this
       * function so the GUI doesn't need to deal with it 
       */
      name = gui->fileselect (_("Save Renumber Annotation File As ..."),
			      _("Choose a file to record the renumbering to.\n"
				"This file may be used to back annotate the\n"
				"change to the schematics.\n"),
			      default_file, ".eco", "eco",
			      0);

      free_name = 1;
    }
  else
    name = argv[0];

  if (default_file)
    {
      free (default_file);
      default_file = NULL;
    }

  if (name && *name)
    {
      default_file = strdup (name);
    }

  if ((out = fopen (name, "r")))
    {
      fclose (out);
      if (!gui->confirm_dialog (_("File exists!  Ok to overwrite?"), 0))
	{
	  if (free_name && name)
	    free (name);
	  return 0;
	}
    }

  if ((out = fopen (name, "w")) == NULL)
    {
      Message (_("Could not open %s\n"), name);
      if (free_name && name)
	free (name);
      return 1;
    }
  
  if (free_name && name)
    free (name);

  fprintf (out, "*COMMENT* PCB Annotation File\n");
  fprintf (out, "*FILEVERSION* 20061031\n");

  /*
   * Make a first pass through all of the elements and sort them out
   * by location on the board.  While here we also collect a list of
   * locked elements.
   *
   * We'll actually renumber things in the 2nd pass.
   */
  element_list = (ElementType **)calloc (PCB->Data->ElementN, sizeof (ElementType *));
  locked_element_list = (ElementType **)calloc (PCB->Data->ElementN, sizeof (ElementType *));
  was = (char **)calloc (PCB->Data->ElementN, sizeof (char *));
  is = (char **)calloc (PCB->Data->ElementN, sizeof (char *));
  if (element_list == NULL || locked_element_list == NULL || was == NULL
      || is == NULL)
    {
      fprintf (stderr, "calloc() failed in %s\n", __FUNCTION__);
      exit (1);
    }


  cnt = 0;
  lock_cnt = 0;
  ELEMENT_LOOP (PCB->Data);
  {
    if (TEST_FLAG (LOCKFLAG, element->Name) || TEST_FLAG (LOCKFLAG, element))
      {
	/* 
	 * add to the list of locked elements which we won't try to
	 * renumber and whose reference designators are now reserved.
	 */
	pcb_fprintf (out,
		     "*WARN* Element \"%s\" at %$md is locked and will not be renumbered.\n",
		      UNKNOWN (NAMEONPCB_NAME (element)), element->MarkX, element->MarkY);
	locked_element_list[lock_cnt] = element;
	lock_cnt++;
      }

    else
      {
	/* count of devices which will be renumbered */
	cnt++;

	/* search for correct position in the list */
	i = 0;
	while (element_list[i] && element->MarkY > element_list[i]->MarkY)
	  i++;

	/* 
	 * We have found the position where we have the first element that
	 * has the same Y value or a lower Y value.  Now move forward if
	 * needed through the X values
	 */
	while (element_list[i]
	       && element->MarkY == element_list[i]->MarkY
	       && element->MarkX > element_list[i]->MarkX)
	  i++;

	for (j = cnt - 1; j > i; j--)
	  {
	    element_list[j] = element_list[j - 1];
	  }
	element_list[i] = element;
      }
  }
  END_LOOP;


  /* 
   * Now that the elements are sorted by board position, we go through
   * and renumber them.
   */

  /* 
   * turn off the flag which requires unique names so it doesn't get
   * in our way.  When we're done with the renumber we will have unique
   * names.
   */
  unique = TEST_FLAG (UNIQUENAMEFLAG, PCB);
  CLEAR_FLAG (UNIQUENAMEFLAG, PCB);

  cnt_list = (struct _cnt_list *)calloc (cnt_list_sz, sizeof (struct _cnt_list));
  for (i = 0; i < cnt; i++)
    {
      /* If there is no refdes, maybe just spit out a warning */
      if (NAMEONPCB_NAME (element_list[i]))
	{
	  /* figure out the prefix */
	  tmps = strdup (NAMEONPCB_NAME (element_list[i]));
	  j = 0;
	  while (tmps[j] && (tmps[j] < '0' || tmps[j] > '9')
		 && tmps[j] != '?')
	    j++;
	  tmps[j] = '\0';

	  /* check the counter for this prefix */
	  for (j = 0;
	       cnt_list[j].name && (strcmp (cnt_list[j].name, tmps) != 0)
	       && j < cnt_list_sz; j++);

	  /* grow the list if needed */
	  if (j == cnt_list_sz)
	    {
	      cnt_list_sz += 100;
	      cnt_list = (struct _cnt_list *)realloc (cnt_list, cnt_list_sz);
	      if (cnt_list == NULL)
		{
		  fprintf (stderr, _("realloc failed() in %s\n"), __FUNCTION__);
		  exit (1);
		}
	      /* zero out the memory that we added */
	      for (tmpi = j; tmpi < cnt_list_sz; tmpi++)
		{
		  cnt_list[tmpi].name = NULL;
		  cnt_list[tmpi].cnt = 0;
		}
	    }

	  /* 
	   * start a new counter if we don't have a counter for this
	   * prefix 
	   */
	  if (!cnt_list[j].name)
	    {
	      cnt_list[j].name = strdup (tmps);
	      cnt_list[j].cnt = 0;
	    }

	  /*
	   * check to see if the new refdes is already used by a
	   * locked element
	   */
	  do
	    {
	      ok = 1;
	      cnt_list[j].cnt++;
	      free (tmps);

	      /* space for the prefix plus 1 digit plus the '\0' */
	      sz = strlen (cnt_list[j].name) + 2;

	      /* and 1 more per extra digit needed to hold the number */
	      tmpi = cnt_list[j].cnt;
	      while (tmpi > 10)
		{
		  sz++;
		  tmpi = tmpi / 10;
		}
	      tmps = (char *)malloc (sz * sizeof (char));
	      sprintf (tmps, "%s%d", cnt_list[j].name, (int) cnt_list[j].cnt);

	      /* 
	       * now compare to the list of reserved (by locked
	       * elements) names 
	       */
	      for (k = 0; k < lock_cnt; k++)
		{
		  if (strcmp
		      (UNKNOWN (NAMEONPCB_NAME (locked_element_list[k])),
		       tmps) == 0)
		    {
		      ok = 0;
		      break;
		    }
		}

	    }
	  while (!ok);

	  if (strcmp (tmps, NAMEONPCB_NAME (element_list[i])) != 0)
	    {
	      fprintf (out, "*RENAME* \"%s\" \"%s\"\n",
		       NAMEONPCB_NAME (element_list[i]), tmps);

	      /* add this rename to our table of renames so we can update the netlist */
	      was[c_cnt] = strdup (NAMEONPCB_NAME (element_list[i]));
	      is[c_cnt] = strdup (tmps);
	      c_cnt++;

	      AddObjectToChangeNameUndoList (ELEMENT_TYPE, NULL, NULL,
					     element_list[i],
					     NAMEONPCB_NAME (element_list
							     [i]));

	      ChangeObjectName (ELEMENT_TYPE, element_list[i], NULL, NULL,
				tmps);
	      changed = true;

	      /* we don't free tmps in this case because it is used */
	    }
	  else
	    free (tmps);
	}
      else
	{
	  pcb_fprintf (out, "*WARN* Element at %$md has no name.\n",
		   element_list[i]->MarkX, element_list[i]->MarkY);
	}

    }

  fclose (out);

  /* restore the unique flag setting */
  if (unique)
    SET_FLAG (UNIQUENAMEFLAG, PCB);

  if (changed)
    {

      /* update the netlist */
      AddNetlistLibToUndoList (&(PCB->NetlistLib));

      /* iterate over each net */
      for (i = 0; i < PCB->NetlistLib.MenuN; i++)
	{

	  /* iterate over each pin on the net */
	  for (j = 0; j < PCB->NetlistLib.Menu[i].EntryN; j++)
	    {

	      /* figure out the pin number part from strings like U3-21 */
	      tmps = strdup (PCB->NetlistLib.Menu[i].Entry[j].ListEntry);
	      for (k = 0; tmps[k] && tmps[k] != '-'; k++);
	      tmps[k] = '\0';
	      pin = tmps + k + 1;

	      /* iterate over the list of changed reference designators */
	      for (k = 0; k < c_cnt; k++)
		{
		  /*
		   * if the pin needs to change, change it and quit
		   * searching in the list. 
		   */
		  if (strcmp (tmps, was[k]) == 0)
		    {
		      free (PCB->NetlistLib.Menu[i].Entry[j].ListEntry);
		      PCB->NetlistLib.Menu[i].Entry[j].ListEntry =
			(char *)malloc ((strlen (is[k]) + strlen (pin) +
				 2) * sizeof (char));
		      sprintf (PCB->NetlistLib.Menu[i].Entry[j].ListEntry,
			       "%s-%s", is[k], pin);
		      k = c_cnt;
		    }

		}
	      free (tmps);
	    }
	}
      for (k = 0; k < c_cnt; k++)
	{
	  free (was[k]);
	  free (is[k]);
	}

      NetlistChanged (0);
      IncrementUndoSerialNumber ();
      SetChangedFlag (true);
    }

  free (locked_element_list);
  free (element_list);
  free (cnt_list);
  free (is);
  free (was);
  return 0;
}


/* --------------------------------------------------------------------------- */

static const char ripup_syntax[] = N_("RipUp(All|Selected|Element)");

static const char ripup_help[] =
  N_("Ripup auto-routed tracks, or convert an element to parts.");

/* %start-doc actions RipUp

@table @code

@item All
Removes all lines and vias which were created by the autorouter.

@item Selected
Removes all selected lines and vias which were created by the
autorouter.

@item Element
Converts the element under the cursor to parts (vias and lines).  Note
that this uses the highest numbered paste buffer.

@end table

%end-doc */

static int
ActionRipUp (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  bool changed = false;

  if (function)
    {
      switch (GetFunctionID (function))
	{
	case F_All:
	  ALLLINE_LOOP (PCB->Data);
	  {
	    if (TEST_FLAG (AUTOFLAG, line) && !TEST_FLAG (LOCKFLAG, line))
	      {
		RemoveObject (LINE_TYPE, layer, line, line);
		changed = true;
	      }
	  }
	  ENDALL_LOOP;
	  ALLARC_LOOP (PCB->Data);
	  {
	    if (TEST_FLAG (AUTOFLAG, arc) && !TEST_FLAG (LOCKFLAG, arc))
	      {
		RemoveObject (ARC_TYPE, layer, arc, arc);
		changed = true;
	      }
	  }
	  ENDALL_LOOP;
	  VIA_LOOP (PCB->Data);
	  {
	    if (TEST_FLAG (AUTOFLAG, via) && !TEST_FLAG (LOCKFLAG, via))
	      {
		RemoveObject (VIA_TYPE, via, via, via);
		changed = true;
	      }
	  }
	  END_LOOP;

	  if (changed)
	    {
	      IncrementUndoSerialNumber ();
	      SetChangedFlag (true);
	    }
	  break;
	case F_Selected:
	  VISIBLELINE_LOOP (PCB->Data);
	  {
	    if (TEST_FLAGS (AUTOFLAG | SELECTEDFLAG, line)
		&& !TEST_FLAG (LOCKFLAG, line))
	      {
		RemoveObject (LINE_TYPE, layer, line, line);
		changed = true;
	      }
	  }
	  ENDALL_LOOP;
	  if (PCB->ViaOn)
	    VIA_LOOP (PCB->Data);
	  {
	    if (TEST_FLAGS (AUTOFLAG | SELECTEDFLAG, via)
		&& !TEST_FLAG (LOCKFLAG, via))
	      {
		RemoveObject (VIA_TYPE, via, via, via);
		changed = true;
	      }
	  }
	  END_LOOP;
	  if (changed)
	    {
	      IncrementUndoSerialNumber ();
	      SetChangedFlag (true);
	    }
	  break;
	case F_Element:
	  {
	    void *ptr1, *ptr2, *ptr3;

	    if (SearchScreen (Crosshair.X, Crosshair.Y, ELEMENT_TYPE,
			      &ptr1, &ptr2, &ptr3) != NO_TYPE)
	      {
		Note.Buffer = Settings.BufferNumber;
		SetBufferNumber (MAX_BUFFER - 1);
		ClearBuffer (PASTEBUFFER);
		CopyObjectToBuffer (PASTEBUFFER->Data, PCB->Data,
				    ELEMENT_TYPE, ptr1, ptr2, ptr3);
		SmashBufferElement (PASTEBUFFER);
		PASTEBUFFER->X = 0;
		PASTEBUFFER->Y = 0;
		SaveUndoSerialNumber ();
		EraseObject (ELEMENT_TYPE, ptr1, ptr1);
		MoveObjectToRemoveUndoList (ELEMENT_TYPE, ptr1, ptr2, ptr3);
		RestoreUndoSerialNumber ();
		CopyPastebufferToLayout (0, 0);
		SetBufferNumber (Note.Buffer);
		SetChangedFlag (true);
	      }
	  }
	  break;
	}
    }
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char addrats_syntax[] = N_("AddRats(AllRats|SelectedRats|Close)");

static const char addrats_help[] =
  N_("Add one or more rat lines to the board.");

/* %start-doc actions AddRats

@table @code

@item AllRats
Create rat lines for all loaded nets that aren't already connected on
with copper.

@item SelectedRats
Similarly, but only add rat lines for nets connected to selected pins
and pads.

@item Close
Selects the shortest unselected rat on the board.

@end table

%end-doc */

static int
ActionAddRats (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  RatType *shorty;
  float len, small;

  if (function)
    {
      if (Settings.RatWarn)
	ClearWarnings ();
      switch (GetFunctionID (function))
	{
	case F_AllRats:
	  if (AddAllRats (false, NULL))
	    SetChangedFlag (true);
	  break;
	case F_SelectedRats:
	case F_Selected:
	  if (AddAllRats (true, NULL))
	    SetChangedFlag (true);
	  break;
	case F_Close:
	  small = SQUARE (MAX_COORD);
	  shorty = NULL;
	  RAT_LOOP (PCB->Data);
	  {
	    if (TEST_FLAG (SELECTEDFLAG, line))
	      continue;
	    len = SQUARE (line->Point1.X - line->Point2.X) +
	      SQUARE (line->Point1.Y - line->Point2.Y);
	    if (len < small)
	      {
		small = len;
		shorty = line;
	      }
	  }
	  END_LOOP;
	  if (shorty)
	    {
	      AddObjectToFlagUndoList (RATLINE_TYPE, shorty, shorty, shorty);
	      SET_FLAG (SELECTEDFLAG, shorty);
	      DrawRat (shorty);
	      Draw ();
	      CenterDisplay ((shorty->Point2.X + shorty->Point1.X) / 2,
			     (shorty->Point2.Y + shorty->Point1.Y) / 2,
                             false);
	    }
	  break;
	}
    }
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char delete_syntax[] =
  N_("Delete(Object|Selected)\n"
  "Delete(AllRats|SelectedRats)");

static const char delete_help[] = N_("Delete stuff.");

/* %start-doc actions Delete

%end-doc */

static int
ActionDelete (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  int id = GetFunctionID (function);

  Note.X = Crosshair.X;
  Note.Y = Crosshair.Y;

  if (id == -1) /* no arg */
    {
      if (RemoveSelected() == false)
	id = F_Object;
    }

  switch (id)
    {
    case F_Object:
      SaveMode();
      SetMode(REMOVE_MODE);
      NotifyMode();
      RestoreMode();
      break;
    case F_Selected:
      RemoveSelected();
      break;
    case F_AllRats:
      if (DeleteRats (false))
	SetChangedFlag (true);
      break;
    case F_SelectedRats:
      if (DeleteRats (true))
	SetChangedFlag (true);
      break;
    }

  return 0;
}

/* --------------------------------------------------------------------------- */

static const char deleterats_syntax[] =
  N_("DeleteRats(AllRats|Selected|SelectedRats)");

static const char deleterats_help[] = N_("Delete rat lines.");

/* %start-doc actions DeleteRats

%end-doc */

static int
ActionDeleteRats (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  if (function)
    {
      if (Settings.RatWarn)
	ClearWarnings ();
      switch (GetFunctionID (function))
	{
	case F_AllRats:
	  if (DeleteRats (false))
	    SetChangedFlag (true);
	  break;
	case F_SelectedRats:
	case F_Selected:
	  if (DeleteRats (true))
	    SetChangedFlag (true);
	  break;
	}
    }
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char autoplace_syntax[] = N_("AutoPlaceSelected()");

static const char autoplace_help[] = N_("Auto-place selected components.");

/* %start-doc actions AutoPlaceSelected

Attempts to re-arrange the selected components such that the nets
connecting them are minimized.  Note that you cannot undo this.

%end-doc */

static int
ActionAutoPlaceSelected (int argc, char **argv, Coord x, Coord y)
{
  hid_action("Busy");
  if (gui->confirm_dialog (_("Auto-placement can NOT be undone.\n"
			     "Do you want to continue anyway?\n"), 0))
    {
      if (AutoPlaceSelected ())
	SetChangedFlag (true);
    }
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char autoroute_syntax[] = N_("AutoRoute(AllRats|SelectedRats)");

static const char autoroute_help[] = N_("Auto-route some or all rat lines.");

/* %start-doc actions AutoRoute

@table @code

@item AllRats
Attempt to autoroute all rats.

@item SelectedRats
Attempt to autoroute the selected rats.

@end table

Before autorouting, it's important to set up a few things.  First,
make sure any layers you aren't using are disabled, else the
autorouter may use them.  Next, make sure the current line and via
styles are set accordingly.  Last, make sure "new lines clear
polygons" is set, in case you eventually want to add a copper pour.

Autorouting takes a while.  During this time, the program may not be
responsive.

%end-doc */

static int
ActionAutoRoute (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  hid_action("Busy");
  if (function)			/* one parameter */
    {
      switch (GetFunctionID (function))
	{
	case F_AllRats:
	  if (AutoRoute (false))
	    SetChangedFlag (true);
	  break;
	case F_SelectedRats:
	case F_Selected:
	  if (AutoRoute (true))
	    SetChangedFlag (true);
	  break;
	}
    }
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char markcrosshair_syntax[] =
  N_("MarkCrosshair()\n"
  "MarkCrosshair(Center)");

static const char markcrosshair_help[] = N_("Set/Reset the Crosshair mark.");

/* %start-doc actions MarkCrosshair

The ``mark'' is a small X-shaped target on the display which is
treated like a second origin (the normal origin is the upper let
corner of the board).  The GUI will display a second set of
coordinates for this mark, which tells you how far you are from it.

If no argument is given, the mark is toggled - disabled if it was
enabled, or enabled at the current cursor position of disabled.  If
the @code{Center} argument is given, the mark is moved to the current
cursor location.

%end-doc */

static int
ActionMarkCrosshair (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  if (!function || !*function)
    {
      if (Marked.status)
	{
	  notify_mark_change (false);
	  Marked.status = false;
	  notify_mark_change (true);
	}
      else
	{
	  notify_mark_change (false);
	  Marked.status = false;
	  Marked.status = true;
	  Marked.X = Crosshair.X;
	  Marked.Y = Crosshair.Y;
	  notify_mark_change (true);
	}
    }
  else if (GetFunctionID (function) == F_Center)
    {
      notify_mark_change (false);
      Marked.status = true;
      Marked.X = Crosshair.X;
      Marked.Y = Crosshair.Y;
      notify_mark_change (true);
    }
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char changesize_syntax[] =
  N_("ChangeSize(Object, delta)\n"
  "ChangeSize(SelectedObjects|Selected, delta)\n"
  "ChangeSize(SelectedLines|SelectedPins|SelectedVias, delta)\n"
  "ChangeSize(SelectedPads|SelectedTexts|SelectedNames, delta)\n"
  "ChangeSize(SelectedElements, delta)");

static const char changesize_help[] = N_("Changes the size of objects.");

/* %start-doc actions ChangeSize

For lines and arcs, this changes the width.  For pins and vias, this
changes the overall diameter of the copper annulus.  For pads, this
changes the width and, indirectly, the length.  For texts and names,
this changes the scaling factor.  For elements, this changes the width
of the silk layer lines and arcs for this element.

%end-doc */

static int
ActionChangeSize (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  char *delta = ARG (1);
  char *units = ARG (2);
  bool absolute;			/* indicates if absolute size is given */
  Coord value;

  if (function && delta)
    {
      value = GetValue (delta, units, &absolute);
      if (value == 0)
        value = delta[0] == '-' ? -Settings.increments->size
                                :  Settings.increments->size;
      switch (GetFunctionID (function))
	{
	case F_Object:
	  {
	    int type;
	    void *ptr1, *ptr2, *ptr3;

	    if ((type =
		 SearchScreen (Crosshair.X, Crosshair.Y, CHANGESIZE_TYPES,
			       &ptr1, &ptr2, &ptr3)) != NO_TYPE)
	      if (TEST_FLAG (LOCKFLAG, (PinType *) ptr2))
		Message (_("Sorry, the object is locked\n"));
	    if (ChangeObjectSize (type, ptr1, ptr2, ptr3, value, absolute))
	      SetChangedFlag (true);
	    break;
	  }

	case F_SelectedVias:
	  if (ChangeSelectedSize (VIA_TYPE, value, absolute))
	    SetChangedFlag (true);
	  break;

	case F_SelectedPins:
	  if (ChangeSelectedSize (PIN_TYPE, value, absolute))
	    SetChangedFlag (true);
	  break;

	case F_SelectedPads:
	  if (ChangeSelectedSize (PAD_TYPE, value, absolute))
	    SetChangedFlag (true);
	  break;

	case F_SelectedArcs:
	  if (ChangeSelectedSize (ARC_TYPE, value, absolute))
	    SetChangedFlag (true);
	  break;

	case F_SelectedLines:
	  if (ChangeSelectedSize (LINE_TYPE, value, absolute))
	    SetChangedFlag (true);
	  break;

	case F_SelectedTexts:
	  if (ChangeSelectedSize (TEXT_TYPE, value, absolute))
	    SetChangedFlag (true);
	  break;

	case F_SelectedNames:
	  if (ChangeSelectedSize (ELEMENTNAME_TYPE, value, absolute))
	    SetChangedFlag (true);
	  break;

	case F_SelectedElements:
	  if (ChangeSelectedSize (ELEMENT_TYPE, value, absolute))
	    SetChangedFlag (true);
	  break;

	case F_Selected:
	case F_SelectedObjects:
	  if (ChangeSelectedSize (CHANGESIZE_TYPES, value, absolute))
	    SetChangedFlag (true);
	  break;
	}
    }
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char changedrillsize_syntax[] =
  N_("ChangeDrillSize(Object, delta)\n"
  "ChangeDrillSize(SelectedPins|SelectedVias|Selected|SelectedObjects, delta)");

static const char changedrillsize_help[] =
  N_("Changes the drilling hole size of objects.");

/* %start-doc actions ChangeDrillSize

%end-doc */

static int
ActionChange2ndSize (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  char *delta = ARG (1);
  char *units = ARG (2);
  bool absolute;
  Coord value;

  if (function && delta)
    {
      value = GetValue (delta, units, &absolute);
      switch (GetFunctionID (function))
	{
	case F_Object:
	  {
	    int type;
	    void *ptr1, *ptr2, *ptr3;

	    gui->get_coords (_("Select an Object"), &x, &y);
	    if ((type =
		 SearchScreen (x, y, CHANGE2NDSIZE_TYPES,
			       &ptr1, &ptr2, &ptr3)) != NO_TYPE)
	      if (ChangeObject2ndSize
		  (type, ptr1, ptr2, ptr3, value, absolute, true))
		SetChangedFlag (true);
	    break;
	  }

	case F_SelectedVias:
	  if (ChangeSelected2ndSize (VIA_TYPE, value, absolute))
	    SetChangedFlag (true);
	  break;

	case F_SelectedPins:
	  if (ChangeSelected2ndSize (PIN_TYPE, value, absolute))
	    SetChangedFlag (true);
	  break;
	case F_Selected:
	case F_SelectedObjects:
	  if (ChangeSelected2ndSize (PIN_TYPES, value, absolute))
	    SetChangedFlag (true);
	  break;
	}
    }
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char changeclearsize_syntax[] =
  N_("ChangeClearSize(Object, delta)\n"
  "ChangeClearSize(SelectedPins|SelectedPads|SelectedVias, delta)\n"
  "ChangeClearSize(SelectedLines|SelectedArcs, delta\n"
  "ChangeClearSize(Selected|SelectedObjects, delta)");

static const char changeclearsize_help[] =
  N_("Changes the clearance size of objects.");

/* %start-doc actions ChangeClearSize

If the solder mask is currently showing, this action changes the
solder mask clearance.  If the mask is not showing, this action
changes the polygon clearance.

%end-doc */

static int
ActionChangeClearSize (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  char *delta = ARG (1);
  char *units = ARG (2);
  bool absolute;
  Coord value;

  if (function && delta)
    {
      value = 2 * GetValue (delta, units, &absolute);
      if (value == 0)
        value = delta[0] == '-' ? -Settings.increments->clear
                                :  Settings.increments->clear;
      switch (GetFunctionID (function))
	{
	case F_Object:
	  {
	    int type;
	    void *ptr1, *ptr2, *ptr3;

	    gui->get_coords (_("Select an Object"), &x, &y);
	    if ((type =
		 SearchScreen (x, y,
			       CHANGECLEARSIZE_TYPES, &ptr1, &ptr2,
			       &ptr3)) != NO_TYPE)
	      if (ChangeObjectClearSize (type, ptr1, ptr2, ptr3, value, absolute))
		SetChangedFlag (true);
	    break;
	  }
	case F_SelectedVias:
	  if (ChangeSelectedClearSize (VIA_TYPE, value, absolute))
	    SetChangedFlag (true);
	  break;
	case F_SelectedPads:
	  if (ChangeSelectedClearSize (PAD_TYPE, value, absolute))
	    SetChangedFlag (true);
	  break;
	case F_SelectedPins:
	  if (ChangeSelectedClearSize (PIN_TYPE, value, absolute))
	    SetChangedFlag (true);
	  break;
	case F_SelectedLines:
	  if (ChangeSelectedClearSize (LINE_TYPE, value, absolute))
	    SetChangedFlag (true);
	  break;
	case F_SelectedArcs:
	  if (ChangeSelectedClearSize (ARC_TYPE, value, absolute))
	    SetChangedFlag (true);
	  break;
	case F_Selected:
	case F_SelectedObjects:
	  if (ChangeSelectedClearSize (CHANGECLEARSIZE_TYPES, value, absolute))
	    SetChangedFlag (true);
	  break;
	}
    }
  return 0;
}

/* ---------------------------------------------------------------------------  */

static const char minmaskgap_syntax[] =
  N_("MinMaskGap(delta)\n"
  "MinMaskGap(Selected, delta)");

static const char minmaskgap_help[] =
  N_("Ensures the mask is a minimum distance from pins and pads.");

/* %start-doc actions MinMaskGap

Checks all specified pins and/or pads, and increases the mask if
needed to ensure a minimum distance between the pin or pad edge and
the mask edge.

%end-doc */

static int
ActionMinMaskGap (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  char *delta = ARG (1);
  char *units = ARG (2);
  bool absolute;
  Coord value;
  Coord thickness;
  int flags;

  if (!function)
    return 1;
  if (strcasecmp (function, "Selected") == 0)
    flags = SELECTEDFLAG;
  else
    {
      units = delta;
      delta = function;
      flags = 0;
    }
  value = 2 * GetValue (delta, units, &absolute);

  SaveUndoSerialNumber ();
  ELEMENT_LOOP (PCB->Data);
  {
    PIN_LOOP (element);
    {
      if (!TEST_FLAGS (flags, pin) || ! pin->Mask)
	continue;

	thickness = pin->DrillingHole;
      if (pin->Thickness > thickness)
	thickness = pin->Thickness;
      thickness += value;

      if (pin->Mask < thickness)
	{
	  ChangeObjectMaskSize (PIN_TYPE, element, pin, 0, thickness, 1);
	  RestoreUndoSerialNumber ();
	}
    }
    END_LOOP;
    PAD_LOOP (element);
    {
      if (!TEST_FLAGS (flags, pad) || ! pad->Mask)
	continue;
      if (pad->Mask < pad->Thickness + value)
	{
	  ChangeObjectMaskSize (PAD_TYPE, element, pad, 0,
				pad->Thickness + value, 1);
	  RestoreUndoSerialNumber ();
	}
    }
    END_LOOP;
  }
  END_LOOP;
  VIA_LOOP (PCB->Data);
  {
    if (!TEST_FLAGS (flags, via) || ! via->Mask)
      continue;

    thickness = via->DrillingHole;
    if (via->Thickness > thickness)
      thickness = via->Thickness;
    thickness += value;

    if (via->Mask < thickness)
      {
	ChangeObjectMaskSize (VIA_TYPE, via, 0, 0, thickness, 1);
	RestoreUndoSerialNumber ();
      }
  }
  END_LOOP;
  RestoreUndoSerialNumber ();
  IncrementUndoSerialNumber ();
  return 0;
}

/* ---------------------------------------------------------------------------  */

static const char mincleargap_syntax[] =
  N_("MinClearGap(delta)\n"
  "MinClearGap(Selected, delta)");

static const char mincleargap_help[] =
  N_("Ensures that polygons are a minimum distance from objects.");

/* %start-doc actions MinClearGap

Checks all specified objects, and increases the polygon clearance if
needed to ensure a minimum distance between their edges and the
polygon edges.

%end-doc */

static int
ActionMinClearGap (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  char *delta = ARG (1);
  char *units = ARG (2);
  bool absolute;
  Coord value;
  int flags;

  if (!function)
    return 1;
  if (strcasecmp (function, "Selected") == 0)
    flags = SELECTEDFLAG;
  else
    {
      units = delta;
      delta = function;
      flags = 0;
    }
  value = 2 * GetValue (delta, units, &absolute);

  SaveUndoSerialNumber ();
  ELEMENT_LOOP (PCB->Data);
  {
    PIN_LOOP (element);
    {
      if (!TEST_FLAGS (flags, pin))
	continue;
      if (pin->Clearance < value)
	{
	  ChangeObjectClearSize (PIN_TYPE, element, pin, 0,
				value, 1);
	  RestoreUndoSerialNumber ();
	}
    }
    END_LOOP;
    PAD_LOOP (element);
    {
      if (!TEST_FLAGS (flags, pad))
	continue;
      if (pad->Clearance < value)
	{
	  ChangeObjectClearSize (PAD_TYPE, element, pad, 0,
				value, 1);
	  RestoreUndoSerialNumber ();
	}
    }
    END_LOOP;
  }
  END_LOOP;
  VIA_LOOP (PCB->Data);
  {
    if (!TEST_FLAGS (flags, via))
      continue;
    if (via->Clearance < value)
      {
	ChangeObjectClearSize (VIA_TYPE, via, 0, 0, value, 1);
	RestoreUndoSerialNumber ();
      }
  }
  END_LOOP;
  ALLLINE_LOOP (PCB->Data);
  {
    if (!TEST_FLAGS (flags, line))
      continue;
    if (line->Clearance < value)
      {
	ChangeObjectClearSize (LINE_TYPE, layer, line, 0, value, 1);
	RestoreUndoSerialNumber ();
      }
  }
  ENDALL_LOOP;
  ALLARC_LOOP (PCB->Data);
  {
    if (!TEST_FLAGS (flags, arc))
      continue;
    if (arc->Clearance < value)
      {
	ChangeObjectClearSize (ARC_TYPE, layer, arc, 0, value, 1);
	RestoreUndoSerialNumber ();
      }
  }
  ENDALL_LOOP;
  RestoreUndoSerialNumber ();
  IncrementUndoSerialNumber ();
  return 0;
}

/* ---------------------------------------------------------------------------  */

static const char changepinname_syntax[] =
  N_("ChangePinName(ElementName,PinNumber,PinName)");

static const char changepinname_help[] =
  N_("Sets the name of a specific pin on a specific element.");

/* %start-doc actions ChangePinName

This can be especially useful for annotating pin names from a
schematic to the layout without requiring knowledge of the pcb file
format.

@example
ChangePinName(U3, 7, VCC)
@end example

%end-doc */

static int
ActionChangePinName (int argc, char **argv, Coord x, Coord y)
{
  int changed = 0;
  char *refdes, *pinnum, *pinname;

  if (argc != 3)
    {
      AFAIL (changepinname);
    }

  refdes = argv[0];
  pinnum = argv[1];
  pinname = argv[2];

  ELEMENT_LOOP (PCB->Data);
  {
    if (NSTRCMP (refdes, NAMEONPCB_NAME (element)) == 0)
      {
	PIN_LOOP (element);
	{
	  if (NSTRCMP (pinnum, pin->Number) == 0)
	    {
	      AddObjectToChangeNameUndoList (PIN_TYPE, NULL, NULL,
					     pin, pin->Name);
	      /*
	       * Note:  we can't free() pin->Name first because 
	       * it is used in the undo list
	       */
	      pin->Name = strdup (pinname);
	      SetChangedFlag (true);
	      changed = 1;
	    }
	}
	END_LOOP;

	PAD_LOOP (element);
	{
	  if (NSTRCMP (pinnum, pad->Number) == 0)
	    {
	      AddObjectToChangeNameUndoList (PAD_TYPE, NULL, NULL,
					     pad, pad->Name);
	      /* 
	       * Note:  we can't free() pad->Name first because 
	       * it is used in the undo list
	       */
	      pad->Name = strdup (pinname);
	      SetChangedFlag (true);
	      changed = 1;
	    }
	}
	END_LOOP;
      }
  }
  END_LOOP;
  /* 
   * done with our action so increment the undo # if we actually
   * changed anything
   */
  if (changed)
    {
      if (defer_updates)
	defer_needs_update = 1;
      else
	{
	  IncrementUndoSerialNumber ();
	  gui->invalidate_all ();
	}
    }

  return 0;
}

/* --------------------------------------------------------------------------- */

static const char changename_syntax[] =
  N_("ChangeName(Object)\n"
  "ChangeName(Layout|Layer)");

static const char changename_help[] = N_("Sets the name of objects.");

/* %start-doc actions ChangeName

@table @code

@item Object
Changes the name of the element under the cursor.

@item Layout
Changes the name of the layout.  This is printed on the fab drawings.

@item Layer
Changes the name of the currently active layer.

@end table

%end-doc */

int
ActionChangeName (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  char *name;

  if (function)
    {
      switch (GetFunctionID (function))
	{
	  /* change the name of an object */
	case F_Object:
	  {
	    int type;
	    void *ptr1, *ptr2, *ptr3;

	    gui->get_coords (_("Select an Object"), &x, &y);
	    if ((type =
		 SearchScreen (x, y, CHANGENAME_TYPES,
			       &ptr1, &ptr2, &ptr3)) != NO_TYPE)
	      {
		SaveUndoSerialNumber ();
		if (QueryInputAndChangeObjectName (type, ptr1, ptr2, ptr3))
		  {
		    SetChangedFlag (true);
		    if (type == ELEMENT_TYPE)
		      {
			RubberbandType *ptr;
			int i;

			RestoreUndoSerialNumber ();
			Crosshair.AttachedObject.RubberbandN = 0;
			LookupRatLines (type, ptr1, ptr2, ptr3);
			ptr = Crosshair.AttachedObject.Rubberband;
			for (i = 0; i < Crosshair.AttachedObject.RubberbandN;
			     i++, ptr++)
			  {
			    if (PCB->RatOn)
			      EraseRat ((RatType *) ptr->Line);
			    MoveObjectToRemoveUndoList (RATLINE_TYPE,
							ptr->Line, ptr->Line,
							ptr->Line);
			  }
			IncrementUndoSerialNumber ();
			Draw ();
		      }
		  }
	      }
	    break;
	  }

	  /* change the layout's name */
	case F_Layout:
	  name =
	    gui->prompt_for (_("Enter the layout name:"), EMPTY (PCB->Name));
	  /* NB: ChangeLayoutName takes ownership of the passed memory */
	  if (name && ChangeLayoutName (name))
	    SetChangedFlag (true);
	  break;

	  /* change the name of the active layer */
	case F_Layer:
	  name = gui->prompt_for (_("Enter the layer name:"),
				  EMPTY (CURRENT->Name));
	  /* NB: ChangeLayerName takes ownership of the passed memory */
	  if (name && ChangeLayerName (CURRENT, name))
	    SetChangedFlag (true);
	  break;
	}
    }
  return 0;
}


/* --------------------------------------------------------------------------- */

static const char morphpolygon_syntax[] = N_("MorphPolygon(Object|Selected)");

static const char morphpolygon_help[] =
  N_("Converts dead polygon islands into separate polygons.");

/* %start-doc actions MorphPolygon 

If a polygon is divided into unconnected "islands", you can use
this command to convert the otherwise disappeared islands into
separate polygons. Be sure the cursor is over a portion of the
polygon that remains visible. Very small islands that may flake
off are automatically deleted.

%end-doc */

static int
ActionMorphPolygon (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  if (function)
    {
      switch (GetFunctionID (function))
	{
	case F_Object:
	  {
	    int type;
	    void *ptr1, *ptr2, *ptr3;

	    gui->get_coords (_("Select an Object"), &x, &y);
	    if ((type = SearchScreen (x, y, POLYGON_TYPE,
				      &ptr1, &ptr2, &ptr3)) != NO_TYPE)
	      {
		MorphPolygon ((LayerType *) ptr1, (PolygonType *) ptr3);
		Draw ();
		IncrementUndoSerialNumber ();
	      }
	    break;
	  }
	case F_Selected:
	case F_SelectedObjects:
	  ALLPOLYGON_LOOP (PCB->Data);
	  {
	    if (TEST_FLAG (SELECTEDFLAG, polygon))
	      MorphPolygon (layer, polygon);
	  }
	  ENDALL_LOOP;
	  Draw ();
	  IncrementUndoSerialNumber ();
	  break;
	}
    }
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char togglehidename_syntax[] =
  N_("ToggleHideName(Object|SelectedElements)");

static const char togglehidename_help[] =
  N_("Toggles the visibility of element names.");

/* %start-doc actions ToggleHideName

If names are hidden you won't see them on the screen and they will not
appear on the silk layer when you print the layout.

%end-doc */

static int
ActionToggleHideName (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  if (function && PCB->ElementOn)
    {
      switch (GetFunctionID (function))
	{
	case F_Object:
	  {
	    int type;
	    void *ptr1, *ptr2, *ptr3;

	    gui->get_coords (_("Select an Object"), &x, &y);
	    if ((type = SearchScreen (x, y, ELEMENT_TYPE,
				      &ptr1, &ptr2, &ptr3)) != NO_TYPE)
	      {
		AddObjectToFlagUndoList (type, ptr1, ptr2, ptr3);
		EraseElementName ((ElementType *) ptr2);
		TOGGLE_FLAG (HIDENAMEFLAG, (ElementType *) ptr2);
		DrawElementName ((ElementType *) ptr2);
		Draw ();
		IncrementUndoSerialNumber ();
	      }
	    break;
	  }
	case F_SelectedElements:
	case F_Selected:
	  {
	    bool changed = false;
	    ELEMENT_LOOP (PCB->Data);
	    {
	      if ((TEST_FLAG (SELECTEDFLAG, element) ||
		   TEST_FLAG (SELECTEDFLAG,
			      &NAMEONPCB_TEXT (element)))
		  && (FRONT (element) || PCB->InvisibleObjectsOn))
		{
		  AddObjectToFlagUndoList (ELEMENT_TYPE, element,
					   element, element);
		  EraseElementName (element);
		  TOGGLE_FLAG (HIDENAMEFLAG, element);
		  DrawElementName (element);
		  changed = true;
		}
	    }
	    END_LOOP;
	    if (changed)
	      {
		Draw ();
		IncrementUndoSerialNumber ();
	      }
	  }
	}
    }
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char changejoin_syntax[] =
  N_("ChangeJoin(ToggleObject|SelectedLines|SelectedArcs|Selected)");

static const char changejoin_help[] =
  N_("Changes the join (clearance through polygons) of objects.");

/* %start-doc actions ChangeJoin

The join flag determines whether a line or arc, drawn to intersect a
polygon, electrically connects to the polygon or not.  When joined,
the line/arc is simply drawn over the polygon, making an electrical
connection.  When not joined, a gap is drawn between the line and the
polygon, insulating them from each other.

%end-doc */

static int
ActionChangeJoin (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  if (function)
    {
      switch (GetFunctionID (function))
	{
	case F_ToggleObject:
	case F_Object:
	  {
	    int type;
	    void *ptr1, *ptr2, *ptr3;

	    gui->get_coords (_("Select an Object"), &x, &y);
	    if ((type =
		 SearchScreen (x, y, CHANGEJOIN_TYPES,
			       &ptr1, &ptr2, &ptr3)) != NO_TYPE)
	      if (ChangeObjectJoin (type, ptr1, ptr2, ptr3))
		SetChangedFlag (true);
	    break;
	  }

	case F_SelectedLines:
	  if (ChangeSelectedJoin (LINE_TYPE))
	    SetChangedFlag (true);
	  break;

	case F_SelectedArcs:
	  if (ChangeSelectedJoin (ARC_TYPE))
	    SetChangedFlag (true);
	  break;

	case F_Selected:
	case F_SelectedObjects:
	  if (ChangeSelectedJoin (CHANGEJOIN_TYPES))
	    SetChangedFlag (true);
	  break;
	}
    }
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char changesquare_syntax[] =
  N_("ChangeSquare(ToggleObject)\n"
  "ChangeSquare(SelectedElements|SelectedPins)\n"
  "ChangeSquare(Selected|SelectedObjects)");

static const char changesquare_help[] =
  N_("Changes the square flag of pins and pads.");

/* %start-doc actions ChangeSquare

Note that @code{Pins} means both pins and pads.

@pinshapes

%end-doc */

static int
ActionChangeSquare (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  if (function)
    {
      switch (GetFunctionID (function))
	{
	case F_ToggleObject:
	case F_Object:
	  {
	    int type;
	    void *ptr1, *ptr2, *ptr3;

	    gui->get_coords (_("Select an Object"), &x, &y);
	    if ((type =
		 SearchScreen (x, y, CHANGESQUARE_TYPES,
			       &ptr1, &ptr2, &ptr3)) != NO_TYPE)
	      if (ChangeObjectSquare (type, ptr1, ptr2, ptr3))
		SetChangedFlag (true);
	    break;
	  }

	case F_SelectedElements:
	  if (ChangeSelectedSquare (ELEMENT_TYPE))
	    SetChangedFlag (true);
	  break;

	case F_SelectedPins:
	  if (ChangeSelectedSquare (PIN_TYPE | PAD_TYPE))
	    SetChangedFlag (true);
	  break;

	case F_Selected:
	case F_SelectedObjects:
	  if (ChangeSelectedSquare (PIN_TYPE | PAD_TYPE))
	    SetChangedFlag (true);
	  break;
	}
    }
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char setsquare_syntax[] =
  N_("SetSquare(ToggleObject|SelectedElements|SelectedPins)");

static const char setsquare_help[] = N_("sets the square-flag of objects.");

/* %start-doc actions SetSquare

Note that @code{Pins} means pins and pads.

@pinshapes

%end-doc */

static int
ActionSetSquare (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  if (function && *function)
    {
      switch (GetFunctionID (function))
	{
	case F_ToggleObject:
	case F_Object:
	  {
	    int type;
	    void *ptr1, *ptr2, *ptr3;

	    gui->get_coords (_("Select an Object"), &x, &y);
	    if ((type =
		 SearchScreen (x, y, CHANGESQUARE_TYPES,
			       &ptr1, &ptr2, &ptr3)) != NO_TYPE)
	      if (SetObjectSquare (type, ptr1, ptr2, ptr3))
		SetChangedFlag (true);
	    break;
	  }

	case F_SelectedElements:
	  if (SetSelectedSquare (ELEMENT_TYPE))
	    SetChangedFlag (true);
	  break;

	case F_SelectedPins:
	  if (SetSelectedSquare (PIN_TYPE | PAD_TYPE))
	    SetChangedFlag (true);
	  break;

	case F_Selected:
	case F_SelectedObjects:
	  if (SetSelectedSquare (PIN_TYPE | PAD_TYPE))
	    SetChangedFlag (true);
	  break;
	}
    }
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char clearsquare_syntax[] =
  N_("ClearSquare(ToggleObject|SelectedElements|SelectedPins)");

static const char clearsquare_help[] =
  N_("Clears the square-flag of pins and pads.");

/* %start-doc actions ClearSquare

Note that @code{Pins} means pins and pads.

@pinshapes

%end-doc */

static int
ActionClearSquare (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  if (function && *function)
    {
      switch (GetFunctionID (function))
	{
	case F_ToggleObject:
	case F_Object:
	  {
	    int type;
	    void *ptr1, *ptr2, *ptr3;

	    gui->get_coords (_("Select an Object"), &x, &y);
	    if ((type =
		 SearchScreen (x, y, CHANGESQUARE_TYPES,
			       &ptr1, &ptr2, &ptr3)) != NO_TYPE)
	      if (ClrObjectSquare (type, ptr1, ptr2, ptr3))
		SetChangedFlag (true);
	    break;
	  }

	case F_SelectedElements:
	  if (ClrSelectedSquare (ELEMENT_TYPE))
	    SetChangedFlag (true);
	  break;

	case F_SelectedPins:
	  if (ClrSelectedSquare (PIN_TYPE | PAD_TYPE))
	    SetChangedFlag (true);
	  break;

	case F_Selected:
	case F_SelectedObjects:
	  if (ClrSelectedSquare (PIN_TYPE | PAD_TYPE))
	    SetChangedFlag (true);
	  break;
	}
    }
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char changeoctagon_syntax[] =
  N_("ChangeOctagon(Object|ToggleObject|SelectedObjects|Selected)\n"
  "ChangeOctagon(SelectedElements|SelectedPins|SelectedVias)");

static const char changeoctagon_help[] =
  N_("Changes the octagon-flag of pins and vias.");

/* %start-doc actions ChangeOctagon

@pinshapes

%end-doc */

static int
ActionChangeOctagon (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  if (function)
    {
      switch (GetFunctionID (function))
	{
	case F_ToggleObject:
	case F_Object:
	  {
	    int type;
	    void *ptr1, *ptr2, *ptr3;

	    gui->get_coords (_("Select an Object"), &x, &y);
	    if ((type =
		 SearchScreen (x, y, CHANGEOCTAGON_TYPES,
			       &ptr1, &ptr2, &ptr3)) != NO_TYPE)
	      if (ChangeObjectOctagon (type, ptr1, ptr2, ptr3))
		SetChangedFlag (true);
	    break;
	  }

	case F_SelectedElements:
	  if (ChangeSelectedOctagon (ELEMENT_TYPE))
	    SetChangedFlag (true);
	  break;

	case F_SelectedPins:
	  if (ChangeSelectedOctagon (PIN_TYPE))
	    SetChangedFlag (true);
	  break;

	case F_SelectedVias:
	  if (ChangeSelectedOctagon (VIA_TYPE))
	    SetChangedFlag (true);
	  break;

	case F_Selected:
	case F_SelectedObjects:
	  if (ChangeSelectedOctagon (PIN_TYPES))
	    SetChangedFlag (true);
	  break;
	}
    }
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char setoctagon_syntax[] =
  N_("SetOctagon(Object|ToggleObject|SelectedElements|Selected)");

static const char setoctagon_help[] = N_("Sets the octagon-flag of objects.");

/* %start-doc actions SetOctagon

@pinshapes

%end-doc */

static int
ActionSetOctagon (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  if (function)
    {
      switch (GetFunctionID (function))
	{
	case F_ToggleObject:
	case F_Object:
	  {
	    int type;
	    void *ptr1, *ptr2, *ptr3;

	    gui->get_coords (_("Select an Object"), &x, &y);
	    if ((type =
		 SearchScreen (x, y, CHANGEOCTAGON_TYPES,
			       &ptr1, &ptr2, &ptr3)) != NO_TYPE)
	      if (SetObjectOctagon (type, ptr1, ptr2, ptr3))
		SetChangedFlag (true);
	    break;
	  }

	case F_SelectedElements:
	  if (SetSelectedOctagon (ELEMENT_TYPE))
	    SetChangedFlag (true);
	  break;

	case F_SelectedPins:
	  if (SetSelectedOctagon (PIN_TYPE))
	    SetChangedFlag (true);
	  break;

	case F_SelectedVias:
	  if (SetSelectedOctagon (VIA_TYPE))
	    SetChangedFlag (true);
	  break;

	case F_Selected:
	case F_SelectedObjects:
	  if (SetSelectedOctagon (PIN_TYPES))
	    SetChangedFlag (true);
	  break;
	}
    }
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char clearoctagon_syntax[] =
  N_("ClearOctagon(ToggleObject|Object|SelectedObjects|Selected)\n"
  "ClearOctagon(SelectedElements|SelectedPins|SelectedVias)");

static const char clearoctagon_help[] =
  N_("Clears the octagon-flag of pins and vias.");

/* %start-doc actions ClearOctagon

@pinshapes

%end-doc */

static int
ActionClearOctagon (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  if (function)
    {
      switch (GetFunctionID (function))
	{
	case F_ToggleObject:
	case F_Object:
	  {
	    int type;
	    void *ptr1, *ptr2, *ptr3;

	    gui->get_coords (_("Select an Object"), &x, &y);
	    if ((type =
		 SearchScreen (Crosshair.X, Crosshair.Y, CHANGEOCTAGON_TYPES,
			       &ptr1, &ptr2, &ptr3)) != NO_TYPE)
	      if (ClrObjectOctagon (type, ptr1, ptr2, ptr3))
		SetChangedFlag (true);
	    break;
	  }

	case F_SelectedElements:
	  if (ClrSelectedOctagon (ELEMENT_TYPE))
	    SetChangedFlag (true);
	  break;

	case F_SelectedPins:
	  if (ClrSelectedOctagon (PIN_TYPE))
	    SetChangedFlag (true);
	  break;

	case F_SelectedVias:
	  if (ClrSelectedOctagon (VIA_TYPE))
	    SetChangedFlag (true);
	  break;

	case F_Selected:
	case F_SelectedObjects:
	  if (ClrSelectedOctagon (PIN_TYPES))
	    SetChangedFlag (true);
	  break;
	}
    }
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char changehold_syntax[] =
  N_("ChangeHole(ToggleObject|Object|SelectedVias|Selected)");

static const char changehold_help[] = N_("Changes the hole flag of objects.");

/* %start-doc actions ChangeHole

The "hole flag" of a via determines whether the via is a
plated-through hole (not set), or an unplated hole (set).

%end-doc */

static int
ActionChangeHole (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  if (function)
    {
      switch (GetFunctionID (function))
	{
	case F_ToggleObject:
	case F_Object:
	  {
	    int type;
	    void *ptr1, *ptr2, *ptr3;

	    gui->get_coords (_("Select an Object"), &x, &y);
	    if ((type = SearchScreen (x, y, VIA_TYPE,
				      &ptr1, &ptr2, &ptr3)) != NO_TYPE
		&& ChangeHole ((PinType *) ptr3))
	      IncrementUndoSerialNumber ();
	    break;
	  }

	case F_SelectedVias:
	case F_Selected:
	  if (ChangeSelectedHole ())
	    SetChangedFlag (true);
	  break;
	}
    }
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char changepaste_syntax[] =
  N_("ChangePaste(ToggleObject|Object|SelectedPads|Selected)");

static const char changepaste_help[] =
  N_("Changes the no paste flag of objects.");

/* %start-doc actions ChangePaste

The "no paste flag" of a pad determines whether the solderpaste
 stencil will have an opening for the pad (no set) or if there wil be
 no solderpaste on the pad (set).  This is used for things such as
 fiducial pads.

%end-doc */

static int
ActionChangePaste (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  if (function)
    {
      switch (GetFunctionID (function))
	{
	case F_ToggleObject:
	case F_Object:
	  {
	    int type;
	    void *ptr1, *ptr2, *ptr3;

	    gui->get_coords (_("Select an Object"), &x, &y);
	    if ((type = SearchScreen (x, y, PAD_TYPE,
				      &ptr1, &ptr2, &ptr3)) != NO_TYPE
		&& ChangePaste ((PadType *) ptr3))
	      IncrementUndoSerialNumber ();
	    break;
	  }

	case F_SelectedPads:
	case F_Selected:
	  if (ChangeSelectedPaste ())
	    SetChangedFlag (true);
	  break;
	}
    }
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char select_syntax[] =
  N_("Select(Object|ToggleObject)\n"
  "Select(All|Block|Connection)\n"
  "Select(ElementByName|ObjectByName|PadByName|PinByName)\n"
  "Select(ElementByName|ObjectByName|PadByName|PinByName, Name)\n"
  "Select(TextByName|ViaByName|NetByName)\n"
  "Select(TextByName|ViaByName|NetByName, Name)\n"
  "Select(Convert)");

static const char select_help[] = N_("Toggles or sets the selection.");

/* %start-doc actions Select

@table @code

@item ElementByName
@item ObjectByName
@item PadByName
@item PinByName
@item TextByName
@item ViaByName
@item NetByName

These all rely on having a regular expression parser built into
@code{pcb}.  If the name is not specified then the user is prompted
for a pattern, and all objects that match the pattern and are of the
type specified are selected.

@item Object
@item ToggleObject
Selects the object under the cursor.

@item Block
Selects all objects in a rectangle indicated by the cursor.

@item All
Selects all objects on the board.

@item Found
Selects all connections with the ``found'' flag set.

@item Connection
Selects all connections with the ``connected'' flag set.

@item Convert
Converts the selected objects to an element.  This uses the highest
numbered paste buffer.

@end table

%end-doc */

static int
ActionSelect (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  if (function)
    {
      switch (GetFunctionID (function))
	{
#if defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP)
	  int type;
	  /* select objects by their names */
	case F_ElementByName:
	  type = ELEMENT_TYPE;
	  goto commonByName;
	case F_ObjectByName:
	  type = ALL_TYPES;
	  goto commonByName;
	case F_PadByName:
	  type = PAD_TYPE;
	  goto commonByName;
	case F_PinByName:
	  type = PIN_TYPE;
	  goto commonByName;
	case F_TextByName:
	  type = TEXT_TYPE;
	  goto commonByName;
	case F_ViaByName:
	  type = VIA_TYPE;
	  goto commonByName;
	case F_NetByName:
	  type = NET_TYPE;
	  goto commonByName;

	commonByName:
	  {
	    char *pattern = ARG (1);

	    if (pattern
		|| (pattern =
		    gui->prompt_for (_("Enter pattern:"), "")) != NULL)
	      {
		if (SelectObjectByName (type, pattern, true))
		  SetChangedFlag (true);
		if (ARG (1) == NULL)
		  free (pattern);
	      }
	    break;
	  }
#endif /* defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP) */

	  /* select a single object */
	case F_ToggleObject:
	case F_Object:
	  if (SelectObject ())
	    SetChangedFlag (true);
	  break;

	  /* all objects in block */
	case F_Block:
	  {
	    BoxType box;

	    box.X1 = MIN (Crosshair.AttachedBox.Point1.X,
			  Crosshair.AttachedBox.Point2.X);
	    box.Y1 = MIN (Crosshair.AttachedBox.Point1.Y,
			  Crosshair.AttachedBox.Point2.Y);
	    box.X2 = MAX (Crosshair.AttachedBox.Point1.X,
			  Crosshair.AttachedBox.Point2.X);
	    box.Y2 = MAX (Crosshair.AttachedBox.Point1.Y,
			  Crosshair.AttachedBox.Point2.Y);
	    notify_crosshair_change (false);
	    NotifyBlock ();
	    if (Crosshair.AttachedBox.State == STATE_THIRD &&
		SelectBlock (&box, true))
	      {
		SetChangedFlag (true);
		Crosshair.AttachedBox.State = STATE_FIRST;
	      }
	    notify_crosshair_change (true);
	    break;
	  }

	  /* select all visible objects */
	case F_All:
	  {
	    BoxType box;

	    box.X1 = -MAX_COORD;
	    box.Y1 = -MAX_COORD;
	    box.X2 = MAX_COORD;
	    box.Y2 = MAX_COORD;
	    if (SelectBlock (&box, true))
	      SetChangedFlag (true);
	    break;
	  }

	  /* all logical connections */
	case F_Found:
	  if (SelectByFlag (FOUNDFLAG, true))
	    {
              Draw ();
	      IncrementUndoSerialNumber ();
	      SetChangedFlag (true);
	    }
	  break;

	  /* all physical connections */
	case F_Connection:
	  if (SelectByFlag (CONNECTEDFLAG, true))
	    {
              Draw ();
	      IncrementUndoSerialNumber ();
	      SetChangedFlag (true);
	    }
	  break;

	case F_Convert:
	  {
	    Coord x, y;
	    Note.Buffer = Settings.BufferNumber;
	    SetBufferNumber (MAX_BUFFER - 1);
	    ClearBuffer (PASTEBUFFER);
	    gui->get_coords (_("Select the Element's Mark Location"), &x, &y);
	    x = GridFit (x, PCB->Grid, PCB->GridOffsetX);
	    y = GridFit (y, PCB->Grid, PCB->GridOffsetY);
	    AddSelectedToBuffer (PASTEBUFFER, x, y, true);
	    SaveUndoSerialNumber ();
	    RemoveSelected ();
	    ConvertBufferToElement (PASTEBUFFER);
	    RestoreUndoSerialNumber ();
	    CopyPastebufferToLayout (x, y);
	    SetBufferNumber (Note.Buffer);
	  }
	  break;

	default:
	  AFAIL (select);
	  break;
	}
    }
  return 0;
}

/* FLAG(have_regex,FlagHaveRegex,0) */
int
FlagHaveRegex (int parm)
{
#if defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP)
  return 1;
#else
  return 0;
#endif
}

/* --------------------------------------------------------------------------- */

static const char unselect_syntax[] =
  N_("Unselect(All|Block|Connection)\n"
  "Unselect(ElementByName|ObjectByName|PadByName|PinByName)\n"
  "Unselect(ElementByName|ObjectByName|PadByName|PinByName, Name)\n"
  "Unselect(TextByName|ViaByName)\n"
  "Unselect(TextByName|ViaByName, Name)\n");

static const char unselect_help[] =
  N_("Unselects the object at the pointer location or the specified objects.");

/* %start-doc actions Unselect

@table @code

@item All
Unselect all objects.

@item Block
Unselect all objects in a rectangle given by the cursor.

@item Connection
Unselect all connections with the ``found'' flag set.

@item ElementByName
@item ObjectByName
@item PadByName
@item PinByName
@item TextByName
@item ViaByName

These all rely on having a regular expression parser built into
@code{pcb}.  If the name is not specified then the user is prompted
for a pattern, and all objects that match the pattern and are of the
type specified are unselected.


@end table

%end-doc */

static int
ActionUnselect (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  if (function)
    {
      switch (GetFunctionID (function))
	{
#if defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP)
	  int type;
	  /* select objects by their names */
	case F_ElementByName:
	  type = ELEMENT_TYPE;
	  goto commonByName;
	case F_ObjectByName:
	  type = ALL_TYPES;
	  goto commonByName;
	case F_PadByName:
	  type = PAD_TYPE;
	  goto commonByName;
	case F_PinByName:
	  type = PIN_TYPE;
	  goto commonByName;
	case F_TextByName:
	  type = TEXT_TYPE;
	  goto commonByName;
	case F_ViaByName:
	  type = VIA_TYPE;
	  goto commonByName;
	case F_NetByName:
	  type = NET_TYPE;
	  goto commonByName;

	commonByName:
	  {
	    char *pattern = ARG (1);

	    if (pattern
		|| (pattern =
		    gui->prompt_for (_("Enter pattern:"), "")) != NULL)
	      {
		if (SelectObjectByName (type, pattern, false))
		  SetChangedFlag (true);
		if (ARG (1) == NULL)
		  free (pattern);
	      }
	    break;
	  }
#endif /* defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP) */

	  /* all objects in block */
	case F_Block:
	  {
	    BoxType box;

	    box.X1 = MIN (Crosshair.AttachedBox.Point1.X,
			  Crosshair.AttachedBox.Point2.X);
	    box.Y1 = MIN (Crosshair.AttachedBox.Point1.Y,
			  Crosshair.AttachedBox.Point2.Y);
	    box.X2 = MAX (Crosshair.AttachedBox.Point1.X,
			  Crosshair.AttachedBox.Point2.X);
	    box.Y2 = MAX (Crosshair.AttachedBox.Point1.Y,
			  Crosshair.AttachedBox.Point2.Y);
	    notify_crosshair_change (false);
	    NotifyBlock ();
	    if (Crosshair.AttachedBox.State == STATE_THIRD &&
		SelectBlock (&box, false))
	      {
		SetChangedFlag (true);
		Crosshair.AttachedBox.State = STATE_FIRST;
	      }
	    notify_crosshair_change (true);
	    break;
	  }

	  /* unselect all visible objects */
	case F_All:
	  {
	    BoxType box;

	    box.X1 = -MAX_COORD;
	    box.Y1 = -MAX_COORD;
	    box.X2 = MAX_COORD;
	    box.Y2 = MAX_COORD;
	    if (SelectBlock (&box, false))
	      SetChangedFlag (true);
	    break;
	  }

	  /* all logical connections */
	case F_Found:
	  if (SelectByFlag (FOUNDFLAG, false))
	    {
              Draw ();
	      IncrementUndoSerialNumber ();
	      SetChangedFlag (true);
	    }
	  break;

	  /* all physical connections */
	case F_Connection:
	  if (SelectByFlag (CONNECTEDFLAG, false))
	    {
              Draw ();
	      IncrementUndoSerialNumber ();
	      SetChangedFlag (true);
	    }
	  break;

	default:
	  AFAIL (unselect);
	  break;

	}
    }
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char saveto_syntax[] =
  N_("SaveTo(Layout|LayoutAs,filename)\n"
  "SaveTo(AllConnections|AllUnusedPins|ElementConnections,filename)\n"
  "SaveTo(PasteBuffer,filename)");

static const char saveto_help[] = N_("Saves data to a file.");

/* %start-doc actions SaveTo

@table @code

@item Layout
Saves the current layout.

@item LayoutAs
Saves the current layout, and remembers the filename used.

@item AllConnections
Save all connections to a file.

@item AllUnusedPins
List all unused pins to a file.

@item ElementConnections
Save connections to the element at the cursor to a file.

@item PasteBuffer
Save the content of the active Buffer to a file. This is the graphical way to create a footprint.

@end table

%end-doc */

static int
ActionSaveTo (int argc, char **argv, Coord x, Coord y)
{
  char *function;
  char *name;

  function = ARG (0);
  
  if ( ! function || strcasecmp (function, "Layout") == 0)
    {
      if (SavePCB (PCB->Filename) == 0)
        SetChangedFlag (false);
      return 0;
    }

  if (argc != 2)
    AFAIL (saveto);

  name = argv[1];

  if (strcasecmp (function, "LayoutAs") == 0)
    {
      if (SavePCB (name) == 0)
        {
          SetChangedFlag (false);
          free (PCB->Filename);
          PCB->Filename = strdup (name);
          if (gui->notify_filename_changed != NULL)
            gui->notify_filename_changed ();
        }
      return 0;
    }

  if (strcasecmp (function, "AllConnections") == 0)
    {
      FILE *fp;
      bool result;
      if ((fp = CheckAndOpenFile (name, true, false, &result, NULL)) != NULL)
	{
	  LookupConnectionsToAllElements (fp);
	  fclose (fp);
	  SetChangedFlag (true);
	}
      return 0;
    }

  if (strcasecmp (function, "AllUnusedPins") == 0)
    {
      FILE *fp;
      bool result;
      if ((fp = CheckAndOpenFile (name, true, false, &result, NULL)) != NULL)
	{
	  LookupUnusedPins (fp);
	  fclose (fp);
	  SetChangedFlag (true);
	}
      return 0;
    }

  if (strcasecmp (function, "ElementConnections") == 0)
    {
      ElementType *element;
      void *ptrtmp;
      FILE *fp;
      bool result;

      if ((SearchScreen (Crosshair.X, Crosshair.Y, ELEMENT_TYPE,
			 &ptrtmp, &ptrtmp, &ptrtmp)) != NO_TYPE)
	{
	  element = (ElementType *) ptrtmp;
	  if ((fp =
	       CheckAndOpenFile (name, true, false, &result, NULL)) != NULL)
	    {
	      LookupElementConnections (element, fp);
	      fclose (fp);
	      SetChangedFlag (true);
	    }
	}
      return 0;
    }

  if (strcasecmp (function, "PasteBuffer") == 0)
    {
      return SaveBufferElements (name);
    }

  AFAIL (saveto);
}

/* --------------------------------------------------------------------------- */

static const char savesettings_syntax[] =
  N_("SaveSettings()\n"
  "SaveSettings(local)");

static const char savesettings_help[] = N_("Saves settings.");

/* %start-doc actions SaveSettings

If you pass no arguments, the settings are stored in
@code{$HOME/.pcb/settings}.  If you pass the word @code{local} they're
saved in @code{./pcb.settings}.

%end-doc */

static int
ActionSaveSettings (int argc, char **argv, Coord x, Coord y)
{
  int locally = argc > 0 ? (strncasecmp (argv[0], "local", 5) == 0) : 0;
  hid_save_settings (locally);
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char loadfrom_syntax[] =
  N_("LoadFrom(Layout|LayoutToBuffer|ElementToBuffer|Netlist|Revert,filename)");

static const char loadfrom_help[] = N_("Load layout data from a file.");

/* %start-doc actions LoadFrom

This action assumes you know what the filename is.  The various GUIs
should have a similar @code{Load} action where the filename is
optional, and will provide their own file selection mechanism to let
you choose the file name.

@table @code

@item Layout
Loads an entire PCB layout, replacing the current one.

@item LayoutToBuffer
Loads an entire PCB layout to the paste buffer.

@item ElementToBuffer
Loads the given element file into the paste buffer.  Element files
contain only a single @code{Element} definition, such as the
``newlib'' library uses.

@item Netlist
Loads a new netlist, replacing any current netlist.

@item Revert
Re-loads the current layout from its disk file, reverting any changes
you may have made.

@end table

%end-doc */

static int
ActionLoadFrom (int argc, char **argv, Coord x, Coord y)
{
  char *function;
  char *name;

  if (argc < 2)
    AFAIL (loadfrom);

  function = argv[0];
  name = argv[1];

  if (strcasecmp (function, "ElementToBuffer") == 0)
    {
      notify_crosshair_change (false);
      if (LoadElementToBuffer (PASTEBUFFER, name, true))
	SetMode (PASTEBUFFER_MODE);
      notify_crosshair_change (true);
    }

  else if (strcasecmp (function, "LayoutToBuffer") == 0)
    {
      notify_crosshair_change (false);
      if (LoadLayoutToBuffer (PASTEBUFFER, name))
	SetMode (PASTEBUFFER_MODE);
      notify_crosshair_change (true);
    }

  else if (strcasecmp (function, "Layout") == 0)
    {
      if (!PCB->Changed ||
	  gui->confirm_dialog (_("OK to override layout data?"), 0))
	LoadPCB (name);
    }

  else if (strcasecmp (function, "Netlist") == 0)
    {
      if (PCB->Netlistname)
	free (PCB->Netlistname);
      PCB->Netlistname = StripWhiteSpaceAndDup (name);
      FreeLibraryMemory (&PCB->NetlistLib);
      ImportNetlist (PCB->Netlistname);
      NetlistChanged (1);
    }
  else if (strcasecmp (function, "Revert") == 0 && PCB->Filename
	   && (!PCB->Changed
	       || gui->confirm_dialog (_("OK to override changes?"), 0)))
    {
      RevertPCB ();
    }

  return 0;
}

/* --------------------------------------------------------------------------- */

static const char new_syntax[] = N_("New([name])");

static const char new_help[] = N_("Starts a new layout.");

/* %start-doc actions New

If a name is not given, one is prompted for.

%end-doc */

static int
ActionNew (int argc, char **argv, Coord x, Coord y)
{
  char *name = ARG (0);

  if (!PCB->Changed || gui->confirm_dialog (_("OK to clear layout data?"), 0))
    {
      if (name)
	name = strdup (name);
      else
	name = gui->prompt_for (_("Enter the layout name:"), "");

      if (!name)
        return 1;

      notify_crosshair_change (false);
      /* do emergency saving
       * clear the old struct and allocate memory for the new one
       */
      if (PCB->Changed && Settings.SaveInTMP)
	SaveInTMP ();
      RemovePCB (PCB);
      PCB = NULL;
      PCB = CreateNewPCB ();
      CreateNewPCBPost (PCB, 1);

      /* setup the new name and reset some values to default */
      free (PCB->Name);
      PCB->Name = name;

      ResetStackAndVisibility ();
      SetCrosshairRange (0, 0, PCB->MaxWidth, PCB->MaxHeight);
      CenterDisplay (PCB->MaxWidth / 2, PCB->MaxHeight / 2, false);
      Redraw ();

      hid_action ("PCBChanged");
      notify_crosshair_change (true);
      return 0;
    }
  return 1;
}

/*!
 * \brief No operation, just for testing purposes.
 * syntax: Bell(volume)
 */
void
ActionBell (char *volume)
{
  gui->beep ();
}

/* --------------------------------------------------------------------------- */

static const char pastebuffer_syntax[] =
  N_("PasteBuffer(AddSelected|Clear|1..MAX_BUFFER)\n"
  "PasteBuffer(Rotate, 1..3)\n"
  "PasteBuffer(Convert|Save|Restore|Mirror)\n"
  "PasteBuffer(ToLayout, X, Y, units)");

static const char pastebuffer_help[] =
  N_("Various operations on the paste buffer.");

/* %start-doc actions PasteBuffer

There are a number of paste buffers; the actual limit is a
compile-time constant @code{MAX_BUFFER} in @file{globalconst.h}.  It
is currently @code{5}.  One of these is the ``current'' paste buffer,
often referred to as ``the'' paste buffer.

@table @code

@item AddSelected
Copies the selected objects to the current paste buffer.

@item Clear
Remove all objects from the current paste buffer.

@item Convert
Convert the current paste buffer to an element.  Vias are converted to
pins, lines are converted to pads.

@item Restore
Convert any elements in the paste buffer back to vias and lines.

@item Mirror
Flip all objects in the paste buffer vertically (up/down flip).  To mirror
horizontally, combine this with rotations.

@item Rotate
Rotates the current buffer.  The number to pass is 1..3, where 1 means
90 degrees counter clockwise, 2 means 180 degrees, and 3 means 90
degrees clockwise (270 CCW).

@item Save
Saves any elements in the current buffer to the indicated file.

@item ToLayout
Pastes any elements in the current buffer to the indicated X, Y
coordinates in the layout.  The @code{X} and @code{Y} are treated like
@code{delta} is for many other objects.  For each, if it's prefixed by
@code{+} or @code{-}, then that amount is relative to the last
location.  Otherwise, it's absolute.  Units can be
@code{mil} or @code{mm}; if unspecified, units are PCB's internal
units, currently 1/100 mil.


@item 1..MAX_BUFFER
Selects the given buffer to be the current paste buffer.

@end table

%end-doc */

static int
ActionPasteBuffer (int argc, char **argv, Coord x, Coord y)
{
  char *function = argc ? argv[0] : (char *)"";
  char *sbufnum = argc > 1 ? argv[1] : (char *)"";
  char *name;
  static char *default_file = NULL;
  int free_name = 0;

  notify_crosshair_change (false);
  if (function)
    {
      switch (GetFunctionID (function))
	{
	  /* clear contents of paste buffer */
	case F_Clear:
	  ClearBuffer (PASTEBUFFER);
	  break;

	  /* copies objects to paste buffer */
	case F_AddSelected:
	  AddSelectedToBuffer (PASTEBUFFER, 0, 0, false);
	  break;

	  /* converts buffer contents into an element */
	case F_Convert:
	  ConvertBufferToElement (PASTEBUFFER);
	  break;

	  /* break up element for editing */
	case F_Restore:
	  SmashBufferElement (PASTEBUFFER);
	  break;

	  /* Mirror buffer */
	case F_Mirror:
	  MirrorBuffer (PASTEBUFFER);
	  break;

	case F_Rotate:
	  if (sbufnum)
	    {
	      RotateBuffer (PASTEBUFFER, (BYTE) atoi (sbufnum));
	      SetCrosshairRangeToBuffer ();
	    }
	  break;

	case F_Save:
	  if (PASTEBUFFER->Data->ElementN == 0)
	    {
	      Message (_("Buffer has no elements!\n"));
	      break;
	    }
	  free_name = 0;
	  if (argc <= 1)
	    {
	      name = gui->fileselect (_("Save Paste Buffer As ..."),
				      _("Choose a file to save the contents of the\n"
					"paste buffer to.\n"),
				      default_file, ".fp", "footprint",
				      0);

	      if (default_file)
		{
		  free (default_file);
		  default_file = NULL;
		}
	      if ( name && *name)
		{
		  default_file = strdup (name);
		}
	      free_name = 1;
	    }
	      
	  else
	    name = argv[1];

	  {
	    FILE *exist;

	    if ((exist = fopen (name, "r")))
	      {
		fclose (exist);
		if (gui->
		    confirm_dialog (_("File exists!  Ok to overwrite?"), 0))
		  SaveBufferElements (name);
	      }
	    else
	      SaveBufferElements (name);

	    if (free_name && name)
	      free (name);
	  }
	  break;

	case F_ToLayout:
	  {
	    static Coord oldx = 0, oldy = 0;
	    Coord x, y;
	    bool absolute;

	    if (argc == 1)
	      {
		x = y = 0;
	      }
	    else if (argc == 3 || argc == 4)
	      {
		x = GetValue (ARG (1), ARG (3), &absolute);
		if (!absolute)
		  x += oldx;
		y = GetValue (ARG (2), ARG (3), &absolute);
		if (!absolute)
		  y += oldy;
	      }
	    else
	      {
		notify_crosshair_change (true);
		AFAIL (pastebuffer);
	      }

	    oldx = x;
	    oldy = y;
	    if (CopyPastebufferToLayout (x, y))
	      SetChangedFlag (true);
	  }
	  break;

	  /* set number */
	default:
	  {
	    int number = atoi (function);

	    /* correct number */
	    if (number)
	      SetBufferNumber (number - 1);
	  }
	}
    }

  notify_crosshair_change (true);
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char undo_syntax[] = N_("Undo()\n"
                                  "Undo(ClearList)");

static const char undo_help[] = N_("Undo recent changes.");

/* %start-doc actions Undo

The unlimited undo feature of @code{Pcb} allows you to recover from
most operations that materially affect you work.  Calling
@code{Undo()} without any parameter recovers from the last (non-undo)
operation. @code{ClearList} is used to release the allocated
memory. @code{ClearList} is called whenever a new layout is started or
loaded. See also @code{Redo} and @code{Atomic}.

Note that undo groups operations by serial number; changes with the
same serial number will be undone (or redone) as a group.  See
@code{Atomic}.

%end-doc */

static int
ActionUndo (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  if (!function || !*function)
    {
      /* don't allow undo in the middle of an operation */
      if (Settings.Mode != POLYGONHOLE_MODE &&
	  Crosshair.AttachedObject.State != STATE_FIRST)
	return 1;
      if (Crosshair.AttachedBox.State != STATE_FIRST
	  && Settings.Mode != ARC_MODE)
	return 1;
      /* undo the last operation */

      notify_crosshair_change (false);
      if ((Settings.Mode == POLYGON_MODE ||
           Settings.Mode == POLYGONHOLE_MODE) &&
          Crosshair.AttachedPolygon.PointN)
	{
	  GoToPreviousPoint ();
	  notify_crosshair_change (true);
	  return 0;
	}
      /* move anchor point if undoing during line creation */
      if (Settings.Mode == LINE_MODE)
	{
	  if (Crosshair.AttachedLine.State == STATE_SECOND)
	    {
	      if (TEST_FLAG (AUTODRCFLAG, PCB))
		Undo (true);	/* undo the connection find */
	      Crosshair.AttachedLine.State = STATE_FIRST;
	      SetLocalRef (0, 0, false);
	      notify_crosshair_change (true);
	      return 0;
	    }
	  if (Crosshair.AttachedLine.State == STATE_THIRD)
	    {
	      int type;
	      void *ptr1, *ptr3, *ptrtmp;
	      LineType *ptr2;
	      /* this search is guaranteed to succeed */
	      SearchObjectByLocation (LINE_TYPE | RATLINE_TYPE, &ptr1,
				      &ptrtmp, &ptr3,
				      Crosshair.AttachedLine.Point1.X,
				      Crosshair.AttachedLine.Point1.Y, 0);
	      ptr2 = (LineType *) ptrtmp;

	      /* save both ends of line */
	      Crosshair.AttachedLine.Point2.X = ptr2->Point1.X;
	      Crosshair.AttachedLine.Point2.Y = ptr2->Point1.Y;
	      if ((type = Undo (true)))
		SetChangedFlag (true);
	      /* check that the undo was of the right type */
	      if ((type & UNDO_CREATE) == 0)
		{
		  /* wrong undo type, restore anchor points */
		  Crosshair.AttachedLine.Point2.X =
		    Crosshair.AttachedLine.Point1.X;
		  Crosshair.AttachedLine.Point2.Y =
		    Crosshair.AttachedLine.Point1.Y;
		  notify_crosshair_change (true);
		  return 0;
		}
	      /* move to new anchor */
	      Crosshair.AttachedLine.Point1.X =
		Crosshair.AttachedLine.Point2.X;
	      Crosshair.AttachedLine.Point1.Y =
		Crosshair.AttachedLine.Point2.Y;
	      /* check if an intermediate point was removed */
	      if (type & UNDO_REMOVE)
		{
		  /* this search should find the restored line */
		  SearchObjectByLocation (LINE_TYPE | RATLINE_TYPE, &ptr1,
					  &ptrtmp,
					  &ptr3,
					  Crosshair.AttachedLine.Point2.X,
					  Crosshair.AttachedLine.Point2.Y, 0);
		  ptr2 = (LineType *) ptrtmp;
	          if (TEST_FLAG (AUTODRCFLAG, PCB))
		    {
		      /* undo loses CONNECTEDFLAG and FOUNDFLAG */
		      SET_FLAG(CONNECTEDFLAG, ptr2);
		      SET_FLAG(FOUNDFLAG, ptr2);
		      DrawLine (CURRENT, ptr2);
		    }
		  Crosshair.AttachedLine.Point1.X =
		    Crosshair.AttachedLine.Point2.X = ptr2->Point2.X;
		  Crosshair.AttachedLine.Point1.Y =
		    Crosshair.AttachedLine.Point2.Y = ptr2->Point2.Y;
		}
	      FitCrosshairIntoGrid (Crosshair.X, Crosshair.Y);
	      AdjustAttachedObjects ();
	      if (--addedLines == 0)
		{
		  Crosshair.AttachedLine.State = STATE_SECOND;
		  lastLayer = CURRENT;
		}
	      else
		{
		  /* this search is guaranteed to succeed too */
		  SearchObjectByLocation (LINE_TYPE | RATLINE_TYPE, &ptr1,
					  &ptrtmp,
					  &ptr3,
					  Crosshair.AttachedLine.Point1.X,
					  Crosshair.AttachedLine.Point1.Y, 0);
		  ptr2 = (LineType *) ptrtmp;
		  lastLayer = (LayerType *) ptr1;
		}
	      notify_crosshair_change (true);
	      return 0;
	    }
	}
      if (Settings.Mode == ARC_MODE)
	{
	  if (Crosshair.AttachedBox.State == STATE_SECOND)
	    {
	      Crosshair.AttachedBox.State = STATE_FIRST;
	      notify_crosshair_change (true);
	      return 0;
	    }
	  if (Crosshair.AttachedBox.State == STATE_THIRD)
	    {
	      void *ptr1, *ptr2, *ptr3;
	      BoxType *bx;
	      /* guaranteed to succeed */
	      SearchObjectByLocation (ARC_TYPE, &ptr1, &ptr2, &ptr3,
				      Crosshair.AttachedBox.Point1.X,
				      Crosshair.AttachedBox.Point1.Y, 0);
	      bx = GetArcEnds ((ArcType *) ptr2);
	      Crosshair.AttachedBox.Point1.X =
		Crosshair.AttachedBox.Point2.X = bx->X1;
	      Crosshair.AttachedBox.Point1.Y =
		Crosshair.AttachedBox.Point2.Y = bx->Y1;
	      AdjustAttachedObjects ();
	      if (--addedLines == 0)
		Crosshair.AttachedBox.State = STATE_SECOND;
	    }
	}
      /* undo the last destructive operation */
      if (Undo (true))
	SetChangedFlag (true);
    }
  else if (function)
    {
      switch (GetFunctionID (function))
	{
	  /* clear 'undo objects' list */
	case F_ClearList:
	  ClearUndoList (false);
	  break;
	}
    }
  notify_crosshair_change (true);
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char redo_syntax[] = N_("Redo()");

static const char redo_help[] = N_("Redo recent \"undo\" operations.");

/* %start-doc actions Redo

This routine allows you to recover from the last undo command.  You
might want to do this if you thought that undo was going to revert
something other than what it actually did (in case you are confused
about which operations are un-doable), or if you have been backing up
through a long undo list and over-shoot your stopping point.  Any
change that is made since the undo in question will trim the redo
list.  For example if you add ten lines, then undo three of them you
could use redo to put them back, but if you move a line on the board
before performing the redo, you will lose the ability to "redo" the
three "undone" lines.

%end-doc */

static int
ActionRedo (int argc, char **argv, Coord x, Coord y)
{
  if (((Settings.Mode == POLYGON_MODE ||
        Settings.Mode == POLYGONHOLE_MODE) &&
       Crosshair.AttachedPolygon.PointN) ||
      Crosshair.AttachedLine.State == STATE_SECOND)
    return 1;
  notify_crosshair_change (false);
  if (Redo (true))
    {
      SetChangedFlag (true);
      if (Settings.Mode == LINE_MODE &&
	  Crosshair.AttachedLine.State != STATE_FIRST)
	{
	  LineType *line = g_list_last (CURRENT->Line)->data;
	  Crosshair.AttachedLine.Point1.X =
	    Crosshair.AttachedLine.Point2.X = line->Point2.X;
	  Crosshair.AttachedLine.Point1.Y =
	    Crosshair.AttachedLine.Point2.Y = line->Point2.Y;
	  addedLines++;
	}
    }
  notify_crosshair_change (true);
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char polygon_syntax[] = N_("Polygon(Close|PreviousPoint)");

static const char polygon_help[] = N_("Some polygon related stuff.");

/* %start-doc actions Polygon

Polygons need a special action routine to make life easier.

@table @code

@item Close
Creates the final segment of the polygon.  This may fail if clipping
to 45 degree lines is switched on, in which case a warning is issued.

@item PreviousPoint
Resets the newly entered corner to the previous one. The Undo action
will call Polygon(PreviousPoint) when appropriate to do so.

@end table

%end-doc */

static int
ActionPolygon (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  if (function && Settings.Mode == POLYGON_MODE)
    {
      notify_crosshair_change (false);
      switch (GetFunctionID (function))
	{
	  /* close open polygon if possible */
	case F_Close:
	  ClosePolygon ();
	  break;

	  /* go back to the previous point */
	case F_PreviousPoint:
	  GoToPreviousPoint ();
	  break;
	}
      notify_crosshair_change (true);
    }
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char routestyle_syntax[] = N_("RouteStyle(1|2|3|4)");

static const char routestyle_help[] =
  N_("Copies the indicated routing style into the current sizes.");

/* %start-doc actions RouteStyle

%end-doc */

static int
ActionRouteStyle (int argc, char **argv, Coord x, Coord y)
{
  char *str = ARG (0);
  RouteStyleType *rts;
  int number;

  if (str)
    {
      number = atoi (str);
      if (number > 0 && number <= NUM_STYLES)
	{
	  rts = &PCB->RouteStyle[number - 1];
	  SetLineSize (rts->Thick);
	  SetViaSize (rts->Diameter, true);
	  SetViaDrillingHole (rts->Hole, true);
	  SetKeepawayWidth (rts->Keepaway);
	  hid_action("RouteStylesChanged");
	}
    }
  return 0;
}


/* --------------------------------------------------------------------------- */

static const char moveobject_syntax[] = N_("MoveObject(X,Y,dim)");

static const char moveobject_help[] =
  N_("Moves the object under the crosshair.");

/* %start-doc actions MoveObject

The @code{X} and @code{Y} are treated like @code{delta} is for many
other objects.  For each, if it's prefixed by @code{+} or @code{-},
then that amount is relative.  Otherwise, it's absolute.  Units can be
@code{mil} or @code{mm}; if unspecified, units are PCB's internal
units, currently 1/100 mil.

%end-doc */

static int
ActionMoveObject (int argc, char **argv, Coord x, Coord y)
{
  char *x_str = ARG (0);
  char *y_str = ARG (1);
  char *units = ARG (2);
  Coord nx, ny;
  bool absolute1, absolute2;
  void *ptr1, *ptr2, *ptr3;
  int type;

  ny = GetValue (y_str, units, &absolute1);
  nx = GetValue (x_str, units, &absolute2);

  type = SearchScreen (x, y, MOVE_TYPES, &ptr1, &ptr2, &ptr3);
  if (type == NO_TYPE)
    {
      Message (_("Nothing found under crosshair\n"));
      return 1;
    }
  if (absolute1)
    nx -= x;
  if (absolute2)
    ny -= y;
  Crosshair.AttachedObject.RubberbandN = 0;
  if (TEST_FLAG (RUBBERBANDFLAG, PCB))
    LookupRubberbandLines (type, ptr1, ptr2, ptr3);
  if (type == ELEMENT_TYPE)
    LookupRatLines (type, ptr1, ptr2, ptr3);
  MoveObjectAndRubberband (type, ptr1, ptr2, ptr3, nx, ny);
  SetChangedFlag (true);
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char movetocurrentlayer_syntax[] =
  N_("MoveToCurrentLayer(Object|SelectedObjects)");

static const char movetocurrentlayer_help[] =
  N_("Moves objects to the current layer.");

/* %start-doc actions MoveToCurrentLayer

Note that moving an element from a component layer to a solder layer,
or from solder to component, won't automatically flip it.  Use the
@code{Flip()} action to do that.

%end-doc */

static int
ActionMoveToCurrentLayer (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  if (function)
    {
      switch (GetFunctionID (function))
	{
	case F_Object:
	  {
	    int type;
	    void *ptr1, *ptr2, *ptr3;

	    gui->get_coords (_("Select an Object"), &x, &y);
	    if ((type =
		 SearchScreen (x, y, MOVETOLAYER_TYPES,
			       &ptr1, &ptr2, &ptr3)) != NO_TYPE)
	      if (MoveObjectToLayer (type, ptr1, ptr2, ptr3, CURRENT, false))
		SetChangedFlag (true);
	    break;
	  }

	case F_SelectedObjects:
	case F_Selected:
	  if (MoveSelectedObjectsToLayer (CURRENT))
	    SetChangedFlag (true);
	  break;
	}
    }
  return 0;
}


static const char setsame_syntax[] = N_("SetSame()");

static const char setsame_help[] =
  N_("Sets current layer and sizes to match indicated item.");

/* %start-doc actions SetSame

When invoked over any line, arc, polygon, or via, this changes the
current layer to be the layer that item is on, and changes the current
sizes (thickness, keepaway, drill, etc) according to that item.

%end-doc */

static int
ActionSetSame (int argc, char **argv, Coord x, Coord y)
{
  void *ptr1, *ptr2, *ptr3;
  int type;
  LayerType *layer = CURRENT;

  type = SearchScreen (x, y, CLONE_TYPES, &ptr1, &ptr2, &ptr3);
/* set layer current and size from line or arc */
  switch (type)
    {
    case LINE_TYPE:
      notify_crosshair_change (false);
      Settings.LineThickness = ((LineType *) ptr2)->Thickness;
      Settings.Keepaway = ((LineType *) ptr2)->Clearance / 2;
      layer = (LayerType *) ptr1;
      if (Settings.Mode != LINE_MODE)
	SetMode (LINE_MODE);
      notify_crosshair_change (true);
      hid_action ("RouteStylesChanged");
      break;

    case ARC_TYPE:
      notify_crosshair_change (false);
      Settings.LineThickness = ((ArcType *) ptr2)->Thickness;
      Settings.Keepaway = ((ArcType *) ptr2)->Clearance / 2;
      layer = (LayerType *) ptr1;
      if (Settings.Mode != ARC_MODE)
	SetMode (ARC_MODE);
      notify_crosshair_change (true);
      hid_action ("RouteStylesChanged");
      break;

    case POLYGON_TYPE:
      layer = (LayerType *) ptr1;
      break;

    case VIA_TYPE:
      notify_crosshair_change (false);
      Settings.ViaThickness = ((PinType *) ptr2)->Thickness;
      Settings.ViaDrillingHole = ((PinType *) ptr2)->DrillingHole;
      Settings.Keepaway = ((PinType *) ptr2)->Clearance / 2;
      if (Settings.Mode != VIA_MODE)
	SetMode (VIA_MODE);
      notify_crosshair_change (true);
      hid_action ("RouteStylesChanged");
      break;

    default:
      return 1;
    }
  if (layer != CURRENT)
    {
      ChangeGroupVisibility (GetLayerNumber (PCB->Data, layer), true, true);
      Redraw ();
    }
  return 0;
}


/* --------------------------------------------------------------------------- */

static const char setflag_syntax[] =
  N_("SetFlag(Object|Selected|SelectedObjects, flag)\n"
  "SetFlag(SelectedLines|SelectedPins|SelectedVias, flag)\n"
  "SetFlag(SelectedPads|SelectedTexts|SelectedNames, flag)\n"
  "SetFlag(SelectedElements, flag)\n"
  "flag = square | octagon | thermal | join");

static const char setflag_help[] = N_("Sets flags on objects.");

/* %start-doc actions SetFlag

Turns the given flag on, regardless of its previous setting.  See
@code{ChangeFlag}.

@example
SetFlag(SelectedPins,thermal)
@end example

%end-doc */

static int
ActionSetFlag (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  char *flag = ARG (1);
  ChangeFlag (function, flag, 1, "SetFlag");
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char clrflag_syntax[] =
  N_("ClrFlag(Object|Selected|SelectedObjects, flag)\n"
  "ClrFlag(SelectedLines|SelectedPins|SelectedVias, flag)\n"
  "ClrFlag(SelectedPads|SelectedTexts|SelectedNames, flag)\n"
  "ClrFlag(SelectedElements, flag)\n"
  "flag = square | octagon | thermal | join");

static const char clrflag_help[] = N_("Clears flags on objects.");

/* %start-doc actions ClrFlag

Turns the given flag off, regardless of its previous setting.  See
@code{ChangeFlag}.

@example
ClrFlag(SelectedLines,join)
@end example

%end-doc */

static int
ActionClrFlag (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  char *flag = ARG (1);
  ChangeFlag (function, flag, 0, "ClrFlag");
  return 0;
}

/* --------------------------------------------------------------------------- */

static const char changeflag_syntax[] =
  N_("ChangeFlag(Object|Selected|SelectedObjects, flag, value)\n"
  "ChangeFlag(SelectedLines|SelectedPins|SelectedVias, flag, value)\n"
  "ChangeFlag(SelectedPads|SelectedTexts|SelectedNames, flag, value)\n"
  "ChangeFlag(SelectedElements, flag, value)\n"
  "flag = square | octagon | thermal | join\n"
  "value = 0 | 1");

static const char changeflag_help[] = N_("Sets or clears flags on objects.");

/* %start-doc actions ChangeFlag

Toggles the given flag on the indicated object(s).  The flag may be
one of the flags listed above (square, octagon, thermal, join).  The
value may be the number 0 or 1.  If the value is 0, the flag is
cleared.  If the value is 1, the flag is set.

%end-doc */

static int
ActionChangeFlag (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  char *flag = ARG (1);
  int value = argc > 2 ? atoi (argv[2]) : -1;
  if (value != 0 && value != 1)
    AFAIL (changeflag);

  ChangeFlag (function, flag, value, "ChangeFlag");
  return 0;
}


static void
ChangeFlag (char *what, char *flag_name, int value, char *cmd_name)
{
  bool (*set_object) (int, void *, void *, void *);
  bool (*set_selected) (int);

  if (NSTRCMP (flag_name, "square") == 0)
    {
      set_object = value ? SetObjectSquare : ClrObjectSquare;
      set_selected = value ? SetSelectedSquare : ClrSelectedSquare;
    }
  else if (NSTRCMP (flag_name, "octagon") == 0)
    {
      set_object = value ? SetObjectOctagon : ClrObjectOctagon;
      set_selected = value ? SetSelectedOctagon : ClrSelectedOctagon;
    }
  else if (NSTRCMP (flag_name, "join") == 0)
    {
      /* Note: these are backwards, because the flag is "clear" but
         the command is "join".  */
      set_object = value ? ClrObjectJoin : SetObjectJoin;
      set_selected = value ? ClrSelectedJoin : SetSelectedJoin;
    }
  else
    {
      Message (_("%s():  Flag \"%s\" is not valid\n"), cmd_name, flag_name);
      return;
    }

  switch (GetFunctionID (what))
    {
    case F_Object:
      {
	int type;
	void *ptr1, *ptr2, *ptr3;

	if ((type =
	     SearchScreen (Crosshair.X, Crosshair.Y, CHANGESIZE_TYPES,
			   &ptr1, &ptr2, &ptr3)) != NO_TYPE)
	  if (TEST_FLAG (LOCKFLAG, (PinType *) ptr2))
	    Message (_("Sorry, the object is locked\n"));
	if (set_object (type, ptr1, ptr2, ptr3))
	  SetChangedFlag (true);
	break;
      }

    case F_SelectedVias:
      if (set_selected (VIA_TYPE))
	SetChangedFlag (true);
      break;

    case F_SelectedPins:
      if (set_selected (PIN_TYPE))
	SetChangedFlag (true);
      break;

    case F_SelectedPads:
      if (set_selected (PAD_TYPE))
	SetChangedFlag (true);
      break;

    case F_SelectedLines:
      if (set_selected (LINE_TYPE))
	SetChangedFlag (true);
      break;

    case F_SelectedTexts:
      if (set_selected (TEXT_TYPE))
	SetChangedFlag (true);
      break;

    case F_SelectedNames:
      if (set_selected (ELEMENTNAME_TYPE))
	SetChangedFlag (true);
      break;

    case F_SelectedElements:
      if (set_selected (ELEMENT_TYPE))
	SetChangedFlag (true);
      break;

    case F_Selected:
    case F_SelectedObjects:
      if (set_selected (CHANGESIZE_TYPES))
	SetChangedFlag (true);
      break;
    }
}

/* --------------------------------------------------------------------------- */

static const char executefile_syntax[] = N_("ExecuteFile(filename)");

static const char executefile_help[] = N_("Run actions from the given file.");

/* %start-doc actions ExecuteFile

Lines starting with @code{#} are ignored.

%end-doc */

static int
ActionExecuteFile (int argc, char **argv, Coord x, Coord y)
{
  FILE *fp;
  char *fname;
  char line[256];
  int n = 0;
  char *sp;

  if (argc != 1)
    AFAIL (executefile);

  fname = argv[0];

  if ((fp = fopen (fname, "r")) == NULL)
    {
      fprintf (stderr, _("Could not open actions file \"%s\".\n"), fname);
      return 1;
    }

  defer_updates = 1;
  defer_needs_update = 0;
  while (fgets (line, sizeof (line), fp) != NULL)
    {
      n++;
      sp = line;

      /* eat the trailing newline */
      while (*sp && *sp != '\r' && *sp != '\n')
	sp++;
      *sp = '\0';

      /* eat leading spaces and tabs */
      sp = line;
      while (*sp && (*sp == ' ' || *sp == '\t'))
	sp++;

      /* 
       * if we have anything left and its not a comment line
       * then execute it
       */

      if (*sp && *sp != '#')
	{
	  /*Message ("%s : line %-3d : \"%s\"\n", fname, n, sp);*/
	  hid_parse_actions (sp);
	}
    }

  defer_updates = 0;
  if (defer_needs_update)
    {
      IncrementUndoSerialNumber ();
      gui->invalidate_all ();
    }
  fclose (fp);
  return 0;
}

/* --------------------------------------------------------------------------- */

static int
ActionPSCalib (int argc, char **argv, Coord x, Coord y)
{
  HID *ps = hid_find_exporter ("ps");
  ps->calibrate (0.0,0.0);
  return 0;
}

/* --------------------------------------------------------------------------- */

static ElementType *element_cache = NULL;

static ElementType *
find_element_by_refdes (char *refdes)
{
  if (element_cache
      && NAMEONPCB_NAME(element_cache)
      && strcmp (NAMEONPCB_NAME(element_cache), refdes) == 0)
    return element_cache;

  ELEMENT_LOOP (PCB->Data);
  {
    if (NAMEONPCB_NAME(element)
	&& strcmp (NAMEONPCB_NAME(element), refdes) == 0)
      {
	element_cache = element;
	return element_cache;
      }
  }
  END_LOOP;
  return NULL;
}

static AttributeType *
lookup_attr (AttributeListType *list, const char *name)
{
  int i;
  for (i=0; i<list->Number; i++)
    if (strcmp (list->List[i].name, name) == 0)
      return & list->List[i];
  return NULL;
}

static void
delete_attr (AttributeListType *list, AttributeType *attr)
{
  int idx = attr - list->List;
  if (idx < 0 || idx >= list->Number)
    return;
  if (list->Number - idx > 1)
    memmove (attr, attr+1, (list->Number - idx - 1) * sizeof(AttributeType));
  list->Number --;
}

/* ---------------------------------------------------------------- */
static const char elementlist_syntax[] =
  N_("ElementList(Start|Done|Need,<refdes>,<footprint>,<value>)");

static const char elementlist_help[] =
  N_("Adds the given element if it doesn't already exist.");

/* %start-doc actions elementlist

@table @code

@item Start
Indicates the start of an element list; call this before any Need
actions.

@item Need
Searches the board for an element with a matching refdes.

If found, the value and footprint are updated.

If not found, a new element is created with the given footprint and value.

@item Done
Compares the list of elements needed since the most recent
@code{start} with the list of elements actually on the board.  Any
elements that weren't listed are selected, so that the user may delete
them.

@end table

%end-doc */

static int number_of_footprints_not_found;

static int
parse_layout_attribute_units (char *name, int def)
{
  const char *as = AttributeGet (PCB, name);
  if (!as)
    return def;
  return GetValue (as, NULL, NULL);
}

static int
ActionElementList (int argc, char **argv, Coord x, Coord y)
{
  ElementType *e = NULL;
  char *refdes, *value, *footprint, *old;
  char *args[3];
  char *function;

  if (argc < 1)
    AFAIL (elementlist);

  function = argv[0];

#ifdef DEBUG
  printf("Entered ActionElementList, executing function %s\n", function);
#endif

  if (strcasecmp (function, "start") == 0)
    {
      ELEMENT_LOOP (PCB->Data);
      {
	CLEAR_FLAG (FOUNDFLAG, element);
      }
      END_LOOP;
      element_cache = NULL;
      number_of_footprints_not_found = 0;
      return 0;
    }

  if (strcasecmp (function, "done") == 0)
    {
      ELEMENT_LOOP (PCB->Data);
      {
	if (TEST_FLAG (FOUNDFLAG, element))
	  {
	    CLEAR_FLAG (FOUNDFLAG, element);
	  }
	else if (! EMPTY_STRING_P (NAMEONPCB_NAME (element)))
	  {
	    /* Unnamed elements should remain untouched */
	    SET_FLAG (SELECTEDFLAG, element);
	  }
      }
      END_LOOP;
      if (number_of_footprints_not_found > 0)
	gui->confirm_dialog (_("Not all requested footprints were found.\n"
			     "See the message log for details"),
			     "Ok", NULL);
      return 0;
    }

  if (strcasecmp (function, "need") != 0)
    AFAIL (elementlist);

  if (argc != 4)
    AFAIL (elementlist);

  argc --;
  argv ++;

  refdes = ARG(0);
  footprint = ARG(1);
  value = ARG(2);

  args[0] = footprint;
  args[1] = refdes;
  args[2] = value;

#ifdef DEBUG
  printf("  ... footprint = %s\n", footprint);
  printf("  ... refdes = %s\n", refdes);
  printf("  ... value = %s\n", value);
#endif

  e = find_element_by_refdes (refdes);

  if (!e)
    {
      Coord nx, ny, d;

#ifdef DEBUG
      printf("  ... Footprint not on board, need to add it.\n");
#endif
      /* Not on board, need to add it. */
      if (LoadFootprint(argc, args, x, y))
	{
	  number_of_footprints_not_found ++;
	  return 1;
	}

      nx = PCB->MaxWidth / 2;
      ny = PCB->MaxHeight / 2;
      d = MIN (PCB->MaxWidth, PCB->MaxHeight) / 10;

      nx = parse_layout_attribute_units ("import::newX", nx);
      ny = parse_layout_attribute_units ("import::newY", ny);
      d = parse_layout_attribute_units ("import::disperse", d);

      if (d > 0)
	{
	  nx += rand () % (d*2) - d;
	  ny += rand () % (d*2) - d;
	}

      if (nx < 0)
	nx = 0;
      if (nx >= PCB->MaxWidth)
	nx = PCB->MaxWidth - 1;
      if (ny < 0)
	ny = 0;
      if (ny >= PCB->MaxHeight)
	ny = PCB->MaxHeight - 1;

      /* Place components onto center of board. */
      if (CopyPastebufferToLayout (nx, ny))
	SetChangedFlag (true);
    }

  else if (e && DESCRIPTION_NAME(e) && strcmp (DESCRIPTION_NAME(e), footprint) != 0)
    {
#ifdef DEBUG
      printf("  ... Footprint on board, but different from footprint loaded.\n");
#endif
      int er, pr, i;
      Coord mx, my;
      ElementType *pe;

      /* Different footprint, we need to swap them out.  */
      if (LoadFootprint(argc, args, x, y))
	{
	  number_of_footprints_not_found ++;
	  return 1;
	}

      er = ElementOrientation (e);
      pe = PASTEBUFFER->Data->Element->data;
      if (!FRONT (e))
	MirrorElementCoordinates (PASTEBUFFER->Data, pe, pe->MarkY*2 - PCB->MaxHeight);
      pr = ElementOrientation (pe);

      mx = e->MarkX;
      my = e->MarkY;

      if (er != pr)
	RotateElementLowLevel (PASTEBUFFER->Data, pe, pe->MarkX, pe->MarkY, (er-pr+4)%4);

      for (i=0; i<MAX_ELEMENTNAMES; i++)
	{
	  pe->Name[i].X = e->Name[i].X - mx + pe->MarkX ;
	  pe->Name[i].Y = e->Name[i].Y - my + pe->MarkY ;
	  pe->Name[i].Direction = e->Name[i].Direction;
	  pe->Name[i].Scale = e->Name[i].Scale;
	}

      RemoveElement (e);

      if (CopyPastebufferToLayout (mx, my))
	SetChangedFlag (true);
    }

  /* Now reload footprint */
  element_cache = NULL;
  e = find_element_by_refdes (refdes);

  old = ChangeElementText (PCB, PCB->Data, e, NAMEONPCB_INDEX, strdup (refdes));
  if (old)
    free(old);
  old = ChangeElementText (PCB, PCB->Data, e, VALUE_INDEX, strdup (value));
  if (old)
    free(old);

  SET_FLAG (FOUNDFLAG, e);

#ifdef DEBUG
  printf(" ... Leaving ActionElementList.\n");
#endif

  return 0;
}

/* ---------------------------------------------------------------- */
static const char elementsetattr_syntax[] =
  N_("ElementSetAttr(refdes,name[,value])");

static const char elementsetattr_help[] =
  N_("Sets or clears an element-specific attribute.");

/* %start-doc actions elementsetattr

If a value is specified, the named attribute is added (if not already
present) or changed (if it is) to the given value.  If the value is
not specified, the given attribute is removed if present.

%end-doc */

static int
ActionElementSetAttr (int argc, char **argv, Coord x, Coord y)
{
  ElementType *e = NULL;
  char *refdes, *name, *value;
  AttributeType *attr;

  if (argc < 2)
    {
      AFAIL (elementsetattr);
    }

  refdes = argv[0];
  name = argv[1];
  value = ARG(2);

  ELEMENT_LOOP (PCB->Data);
  {
    if (NSTRCMP (refdes, NAMEONPCB_NAME (element)) == 0)
      {
	e = element;
	break;
      }
  }
  END_LOOP;

  if (!e)
    {
      Message(_("Cannot change attribute of %s - element not found\n"), refdes);
      return 1;
    }

  attr = lookup_attr (&e->Attributes, name);

  if (attr && value)
    {
      free (attr->value);
      attr->value = strdup (value);
    }
  if (attr && ! value)
    {
      delete_attr (& e->Attributes, attr);
    }
  if (!attr && value)
    {
      CreateNewAttribute (& e->Attributes, name, value);
    }

  return 0;
}

/* ---------------------------------------------------------------- */
static const char execcommand_syntax[] = N_("ExecCommand(command)");

static const char execcommand_help[] = N_("Runs a command.");

/* %start-doc actions execcommand

Runs the given command, which is a system executable.

%end-doc */

static int
ActionExecCommand (int argc, char **argv, Coord x, Coord y)
{
  char *command;

  if (argc < 1)
    {
      AFAIL (execcommand);
    }

  command = ARG(0);

  if (system (command))
    return 1;
  return 0;
}

/* ---------------------------------------------------------------- */

static int
pcb_spawnvp (char **argv)
{
#ifdef HAVE__SPAWNVP
  int result = _spawnvp (_P_WAIT, argv[0], (const char * const *) argv);
  if (result == -1)
    return 1;
  else
    return 0;
#else
  int pid;
  pid = fork ();
  if (pid < 0)
    {
      /* error */
      Message(_("Cannot fork!"));
      return 1;
    }
  else if (pid == 0)
    {
      /* Child */
      execvp (argv[0], argv);
      exit(1);
    }
  else
    {
      int rv;
      /* Parent */
      wait (&rv);
    }
  return 0;
#endif
}

/* ---------------------------------------------------------------- */

/*! 
 * \brief Creates a new temporary file name.
 * 
 * Hopefully the operating system provides a mkdtemp() function to
 * securily create a temporary directory with mode 0700.\n
 * If so then that directory is created and the returned string is made
 * up of the directory plus the name variable.\n
 * For example:\n
 *
 * tempfile_name_new ("myfile") might return
 * "/var/tmp/pcb.123456/myfile".
 *
 * If mkdtemp() is not available then 'name' is ignored and the
 * insecure tmpnam() function is used.
 *  
 * Files/names created with tempfile_name_new() should be unlinked
 * with tempfile_unlink to make sure the temporary directory is also
 * removed when mkdtemp() is used.
 */
static char *
tempfile_name_new (char * name)
{
  char *tmpfile = NULL;
#ifdef HAVE_MKDTEMP
  char *tmpdir, *mytmpdir;
  size_t len;
#endif

  assert ( name != NULL );

#ifdef HAVE_MKDTEMP
#define TEMPLATE "pcb.XXXXXXXX"
    
  
  tmpdir = getenv ("TMPDIR");

  /* FIXME -- what about win32? */
  if (tmpdir == NULL) {
    tmpdir = "/tmp";
  }
  
  mytmpdir = (char *) malloc (sizeof(char) * 
			      (strlen (tmpdir) + 
			       1 +
			       strlen (TEMPLATE) + 
			       1));
  if (mytmpdir == NULL) {
    fprintf (stderr, "%s(): malloc failed()\n", __FUNCTION__);
    exit (1);
  }
  
  *mytmpdir = '\0';
  (void)strcat (mytmpdir, tmpdir);
  (void)strcat (mytmpdir, PCB_DIR_SEPARATOR_S);
  (void)strcat (mytmpdir, TEMPLATE);
  if (mkdtemp (mytmpdir) == NULL) {
    fprintf (stderr, "%s():  mkdtemp (\"%s\") failed\n", __FUNCTION__, mytmpdir);
    free (mytmpdir);
    return NULL;
  }


  len = strlen (mytmpdir) + /* the temp directory name */
    1 +                     /* the directory sep. */
    strlen (name) +         /* the file name */
    1                       /* the \0 termination */
    ;

  tmpfile = (char *) malloc (sizeof (char) * len);

  *tmpfile = '\0';
  (void)strcat (tmpfile, mytmpdir);
  (void)strcat (tmpfile, PCB_DIR_SEPARATOR_S);
  (void)strcat (tmpfile, name);
  
  free (mytmpdir);
#undef TEMPLATE
#else
  /*
   * tmpnam() uses a static buffer so strdup() the result right away
   * in case someone decides to create multiple temp names.
   */
  tmpfile = strdup (tmpnam (NULL));
#ifdef __WIN32__
    {
      /* Guile doesn't like \ separators */
      char *c;
      for (c = tmpfile; *c; c++)
	if (*c == '\\')
	  *c = '/';
    }
#endif
#endif

  return tmpfile;
}

/* ---------------------------------------------------------------- */

/*!
 * \brief Unlink a temporary file.
 * 
 * If we have mkdtemp() then our temp file lives in a temporary
 * directory and we need to remove that directory too.
 */
static int
tempfile_unlink (char * name)
{
#ifdef DEBUG
    /* SDB says:  Want to keep old temp files for examiniation when debugging */
  return 0;
#endif

#ifdef HAVE_MKDTEMP
  int e, rc2 = 0;
  char *dname;

  unlink (name);
  /* it is possible that the file was never created so it is OK if the
     unlink fails */

  /* now figure out the directory name to remove */
  e = strlen (name) - 1;
  while (e > 0 && name[e] != PCB_DIR_SEPARATOR_C) {e--;}
  
  dname = strdup (name);
  dname[e] = '\0';

  /* 
   * at this point, e *should* point to the end of the directory part 
   * but lets make sure.
   */
  if (e > 0) {
    rc2 = rmdir (dname);
    if (rc2 != 0) {
      perror (dname);
    }

  } else {
    fprintf (stderr, _("%s():  Unable to determine temp directory name from the temp file\n"),
	     __FUNCTION__);
    fprintf (stderr, "%s():  \"%s\"\n", 
	     __FUNCTION__, name);
    rc2 = -1;
  }

  /* name was allocated with malloc */
  free (dname);
  free (name);

  /*
   * FIXME - should also return -1 if the temp file exists and was not
   * removed.  
   */
  if (rc2 != 0) {
    return -1;
  }

#else
  int rc = unlink (name);

  if (rc != 0) {
    fprintf (stderr, _("Failed to unlink \"%s\"\n"), name);
    free (name);
    return rc;
  }
  free (name);

#endif

  return 0;
}

/* ---------------------------------------------------------------- */
static const char import_syntax[] =
  N_("Import()\n"
  "Import([gnetlist|make[,source,source,...]])\n"
  "Import(setnewpoint[,(mark|center|X,Y)])\n"
  "Import(setdisperse,D,units)\n");

static const char import_help[] = N_("Import schematics.");

/* %start-doc actions Import

Imports element and netlist data from the schematics (or some other
source).  The first parameter, which is optional, is the mode.  If not
specified, the @code{import::mode} attribute in the PCB is used.
@code{gnetlist} means gnetlist is used to obtain the information from
the schematics.  @code{make} invokes @code{make}, assuming the user
has a @code{Makefile} in the current directory.  The @code{Makefile}
will be invoked with the following variables set:

@table @code

@item PCB
The name of the .pcb file

@item SRCLIST
A space-separated list of source files

@item OUT
The name of the file in which to put the command script, which may
contain any @pcb{} actions.  By default, this is a temporary file
selected by @pcb{}, but if you specify an @code{import::outfile}
attribute, that file name is used instead (and not automatically
deleted afterwards).

@end table

The target specified to be built is the first of these that apply:

@itemize @bullet

@item
The target specified by an @code{import::target} attribute.

@item
The output file specified by an @code{import::outfile} attribute.

@item
If nothing else is specified, the target is @code{pcb_import}.

@end itemize

If you specify an @code{import::makefile} attribute, then "-f <that
file>" will be added to the command line.

If you specify the mode, you may also specify the source files
(schematics).  If you do not specify any, the list of schematics is
obtained by reading the @code{import::src@var{N}} attributes (like
@code{import::src0}, @code{import::src1}, etc).

For compatibility with future extensions to the import file format,
the generated file @emph{must not} start with the two characters
@code{#%}.

If a temporary file is needed the @code{TMPDIR} environment variable
is used to select its location.

Note that the programs @code{gnetlist} and @code{make} may be
overridden by the user via the @code{make-program} and @code{gnetlist}
@code{pcb} settings (i.e. in @code{~/.pcb/settings} or on the command
line).

If @pcb{} cannot determine which schematic(s) to import from, the GUI
is called to let user choose (see @code{ImportGUI()}).

Note that Import() doesn't delete anything - after an Import, elements
which shouldn't be on the board are selected and may be removed once
it's determined that the deletion is appropriate.

If @code{Import()} is called with @code{setnewpoint}, then the location
of new components can be specified.  This is where parts show up when
they're added to the board.  The default is the center of the board.

@table @code

@item Import(setnewpoint)

Prompts the user to click on the board somewhere, uses that point.  If
called by a hotkey, uses the current location of the crosshair.

@item Import(setnewpoint,mark)

Uses the location of the mark.  If no mark is present, the point is
not changed.

@item Import(setnewpoint,center)

Resets the point to the center of the board.

@item Import(setnewpoint,X,Y,units)

Sets the point to the specific coordinates given.  Example:
@code{Import(setnewpoint,50,25,mm)}

@end table

Note that the X and Y locations are stored in attributes named
@code{import::newX} and @code{import::newY} so you could change them
manually if you wished.

Calling @code{Import(setdisperse,D,units)} sets how much the newly
placed elements are dispersed relative to the set point.  For example,
@code{Import(setdisperse,10,mm)} will offset each part randomly up to
10mm away from the point.  The default dispersion is 1/10th of the
smallest board dimension.  Dispersion is saved in the
@code{import::disperse} attribute.

%end-doc */

static int
ActionImport (int argc, char **argv, Coord x, Coord y)
{
  char *mode;
  char **sources = NULL;
  int nsources = 0;

#ifdef DEBUG
      printf("ActionImport:  ===========  Entering ActionImport  ============\n");
#endif

  mode = ARG (0);

  if (mode && strcasecmp (mode, "setdisperse") == 0)
    {
      char *ds, *units;
      char buf[50];

      ds = ARG (1);
      units = ARG (2);
      if (!ds)
	{
	  const char *as = AttributeGet (PCB, "import::disperse");
	  ds = gui->prompt_for(_("Enter dispersion:"), as ? as : "0");
	}
      if (units)
	{
	  sprintf(buf, "%s%s", ds, units);
	  AttributePut (PCB, "import::disperse", buf);
	}
      else
	AttributePut (PCB, "import::disperse", ds);
      if (ARG (1) == NULL)
        free (ds);
      return 0;
    }

  if (mode && strcasecmp (mode, "setnewpoint") == 0)
    {
      const char *xs, *ys, *units;
      Coord x, y;
      char buf[50];

      xs = ARG (1);
      ys = ARG (2);
      units = ARG (3);

      if (!xs)
	{
	  gui->get_coords (_("Click on a location"), &x, &y);
	}
      else if (strcasecmp (xs, "center") == 0)
	{
	  AttributeRemove (PCB, "import::newX");
	  AttributeRemove (PCB, "import::newY");
	  return 0;
	}
      else if (strcasecmp (xs, "mark") == 0)
	{
	  if (!Marked.status)
	    return 0;

	  x = Marked.X;
	  y = Marked.Y;
	}
      else if (ys)
	{
	  x = GetValue (xs, units, NULL);
	  y = GetValue (ys, units, NULL);
	}
      else
	{
	  Message (_("Bad syntax for Import(setnewpoint)"));
	  return 1;
	}

      pcb_snprintf (buf, sizeof (buf), "%$ms", x);
      AttributePut (PCB, "import::newX", buf);
      pcb_snprintf (buf, sizeof (buf), "%$ms", y);
      AttributePut (PCB, "import::newY", buf);
      return 0;
    }

  if (! mode)
    mode = AttributeGet (PCB, "import::mode");
  if (! mode)
    mode = "gnetlist";

  if (argc > 1)
    {
      sources = argv + 1;
      nsources = argc - 1;
    }

  if (! sources)
    {
      char sname[40];
      char *src;

      nsources = -1;
      do {
	nsources ++;
	sprintf(sname, "import::src%d", nsources);
	src = AttributeGet (PCB, sname);
      } while (src);

      if (nsources > 0)
	{
	  sources = (char **) malloc ((nsources + 1) * sizeof (char *));
	  nsources = -1;
	  do {
	    nsources ++;
	    sprintf(sname, "import::src%d", nsources);
	    src = AttributeGet (PCB, sname);
	    sources[nsources] = src;
	  } while (src);
	}
    }

  if (! sources)
    {
      /* Replace .pcb with .sch and hope for the best.  */
      char *pcbname = PCB->Filename;
      char *schname;
      char *dot, *slash, *bslash;

      if (!pcbname)
	return hid_action("ImportGUI");

      schname = (char *) malloc (strlen(pcbname) + 5);
      strcpy (schname, pcbname);
      dot = strchr (schname, '.');
      slash = strchr (schname, '/');
      bslash = strchr (schname, '\\');
      if (dot && slash && dot < slash)
	dot = NULL;
      if (dot && bslash && dot < bslash)
	dot = NULL;
      if (dot)
	*dot = 0;
      strcat (schname, ".sch");

      if (access (schname, F_OK))
        {
          free (schname);
          return hid_action("ImportGUI");
        }

      sources = (char **) malloc (2 * sizeof (char *));
      sources[0] = schname;
      sources[1] = NULL;
      nsources = 1;
    }

  if (strcasecmp (mode, "gnetlist") == 0)
    {
      char *tmpfile = tempfile_name_new ("gnetlist_output");
      char **cmd;
      int i;

      if (tmpfile == NULL) {
	Message (_("Could not create temp file"));
	return 1;
      }

      cmd = (char **) malloc ((7 + nsources) * sizeof (char *));
      cmd[0] =  Settings.GnetlistProgram;
      cmd[1] = "-g";
      cmd[2] = "pcbfwd";
      cmd[3] = "-o";
      cmd[4] = tmpfile;
      cmd[5] = "--";
      for (i=0; i<nsources; i++)
	cmd[6+i] = sources[i];
      cmd[6+nsources] = NULL;

#ifdef DEBUG
      printf("ActionImport:  ===========  About to run gnetlist  ============\n");
      printf("%s %s %s %s %s %s %s ...\n", 
	     cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6]);
#endif

      if (pcb_spawnvp (cmd))
	{
	  unlink (tmpfile);
	  return 1;
	}

#ifdef DEBUG
      printf("ActionImport:  ===========  About to run ActionExecuteFile, file = %s  ============\n", tmpfile);
#endif

      cmd[0] = tmpfile;
      cmd[1] = NULL;
      ActionExecuteFile (1, cmd, 0, 0);

      free (cmd);
      tempfile_unlink (tmpfile);
    }
  else if (strcasecmp (mode, "make") == 0)
    {
      int must_free_tmpfile = 0;
      char *tmpfile;
      char *cmd[10];
      int i;
      char *srclist;
      int srclen;
      char *user_outfile = NULL;
      char *user_makefile = NULL;
      char *user_target = NULL;


      user_outfile = AttributeGet (PCB, "import::outfile");
      user_makefile = AttributeGet (PCB, "import::makefile");
      user_target = AttributeGet (PCB, "import::target");
      if (user_outfile && !user_target)
	user_target = user_outfile;

      if (user_outfile)
	tmpfile = user_outfile;
      else
	{
	  tmpfile = tempfile_name_new ("gnetlist_output");
	  if (tmpfile == NULL) {
	    Message (_("Could not create temp file"));
            free (sources);
	    return 1;
	  }
	  must_free_tmpfile = 1;
	}

      srclen = sizeof("SRCLIST=") + 2;
      for (i=0; i<nsources; i++)
	srclen += strlen (sources[i]) + 2;
      srclist = (char *) malloc (srclen);
      strcpy (srclist, "SRCLIST=");
      for (i=0; i<nsources; i++)
	{
	  if (i)
	    strcat (srclist, " ");
	  strcat (srclist, sources[i]);
	}
      
      cmd[0] = Settings.MakeProgram;
      cmd[1] = "-s";
      cmd[2] = Concat ("PCB=", PCB->Filename, NULL);
      cmd[3] = srclist;
      cmd[4] = Concat ("OUT=", tmpfile, NULL);
      i = 5;
      if (user_makefile)
	{
	  cmd[i++] = "-f";
	  cmd[i++] = user_makefile;
	}
      cmd[i++] = user_target ? user_target : (char *)"pcb_import";
      cmd[i++] = NULL;

      if (pcb_spawnvp (cmd))
	{
	  if (must_free_tmpfile)
	    unlink (tmpfile);
	  free (cmd[2]);
	  free (cmd[3]);
	  free (cmd[4]);
	  return 1;
	}

      cmd[0] = tmpfile;
      cmd[1] = NULL;
      ActionExecuteFile (1, cmd, 0, 0);

      free (cmd[2]);
      free (cmd[3]);
      free (cmd[4]);
      if (must_free_tmpfile)
	tempfile_unlink (tmpfile);
    }
  else
    {
      Message (_("Unknown import mode: %s\n"), mode);
      return 1;
    }

  DeleteRats (false);
  AddAllRats (false, NULL);

#ifdef DEBUG
      printf("ActionImport:  ===========  Leaving ActionImport  ============\n");
#endif

  return 0;
}

/* ------------------------------------------------------------ */

static const char attributes_syntax[] =
  N_("Attributes(Layout|Layer|Element)\n"
  "Attributes(Layer,layername)");

static const char attributes_help[] =
  N_("Let the user edit the attributes of the layout, current or given\n"
  "layer, or selected element.");

/* %start-doc actions Attributes

This just pops up a dialog letting the user edit the attributes of the
pcb, an element, or a layer.

%end-doc */


static int
ActionAttributes (int argc, char **argv, Coord x, Coord y)
{
  char *function = ARG (0);
  char *layername = ARG (1);
  char *buf;

  if (!function)
    AFAIL (attributes);

  if (!gui->edit_attributes)
    {
      Message (_("This GUI doesn't support Attribute Editing\n"));
      return 1;
    }

  switch (GetFunctionID (function))
    {
    case F_Layout:
      {
	gui->edit_attributes(_("Layout Attributes"), &(PCB->Attributes));
	return 0;
      }

    case F_Layer:
      {
	LayerType *layer = CURRENT;
	if (layername)
	  {
	    int i;
	    layer = NULL;
	    for (i=0; i<max_copper_layer; i++)
	      if (strcmp (PCB->Data->Layer[i].Name, layername) == 0)
		{
		  layer = & (PCB->Data->Layer[i]);
		  break;
		}
	    if (layer == NULL)
	      {
		Message (_("No layer named %s\n"), layername);
		return 1;
	      }
	  }
	buf = (char *) malloc (strlen (layer->Name) +
	    strlen (_("Layer %s Attributes")));
	sprintf (buf, _("Layer %s Attributes"), layer->Name);
	gui->edit_attributes(buf, &(layer->Attributes));
	free (buf);
	return 0;
      }

    case F_Element:
      {
	int n_found = 0;
	ElementType *e = NULL;
	ELEMENT_LOOP (PCB->Data);
	{
	  if (TEST_FLAG (SELECTEDFLAG, element))
	    {
	      e = element;
	      n_found ++;
	    }
	}
	END_LOOP;
	if (n_found > 1)
	  {
	    Message (_("Too many elements selected\n"));
	    return 1;
	  }
	if (n_found == 0)
	  {
	    void *ptrtmp;
	    gui->get_coords (_("Click on an element"), &x, &y);
	    if ((SearchScreen
		 (x, y, ELEMENT_TYPE, &ptrtmp,
		  &ptrtmp, &ptrtmp)) != NO_TYPE)
	      e = (ElementType *) ptrtmp;
	    else
	      {
		Message (_("No element found there\n"));
		return 1;
	      }
	  }

	if (NAMEONPCB_NAME(e))
	  {
	    buf = (char *) malloc (strlen (NAMEONPCB_NAME(e)) +
		strlen (_("Element %s Attributes")));
	    sprintf(buf, _("Element %s Attributes"), NAMEONPCB_NAME(e));
	  }
	else
	  {
	    buf = strdup (_("Unnamed Element Attributes"));
	  }
	gui->edit_attributes(buf, &(e->Attributes));
	free (buf);
	break;
      }

    default:
      AFAIL (attributes);
    }

  return 0;
}

/* --------------------------------------------------------------------------- */

HID_Action action_action_list[] = {
  {"AddRats", 0, ActionAddRats,
   addrats_help, addrats_syntax}
  ,
  {"Attributes", 0, ActionAttributes,
   attributes_help, attributes_syntax}
  ,
  {"Atomic", 0, ActionAtomic,
   atomic_help, atomic_syntax}
  ,
  {"AutoPlaceSelected", 0, ActionAutoPlaceSelected,
   autoplace_help, autoplace_syntax}
  ,
  {"AutoRoute", 0, ActionAutoRoute,
   autoroute_help, autoroute_syntax}
  ,
  {"ChangeClearSize", 0, ActionChangeClearSize,
   changeclearsize_help, changeclearsize_syntax}
  ,
  {"ChangeDrillSize", 0, ActionChange2ndSize,
   changedrillsize_help, changedrillsize_syntax}
  ,
  {"ChangeHole", 0, ActionChangeHole,
   changehold_help, changehold_syntax}
  ,
  {"ChangeJoin", 0, ActionChangeJoin,
   changejoin_help, changejoin_syntax}
  ,
  {"ChangeName", 0, ActionChangeName,
   changename_help, changename_syntax}
  ,
  {"ChangePaste", 0, ActionChangePaste,
   changepaste_help, changepaste_syntax}
  ,
  {"ChangePinName", 0, ActionChangePinName,
   changepinname_help, changepinname_syntax}
  ,
  {"ChangeSize", 0, ActionChangeSize,
   changesize_help, changesize_syntax}
  ,
  {"ChangeSquare", 0, ActionChangeSquare,
   changesquare_help, changesquare_syntax}
  ,
  {"ChangeOctagon", 0, ActionChangeOctagon,
   changeoctagon_help, changeoctagon_syntax}
  ,
  {"ClearSquare", 0, ActionClearSquare,
   clearsquare_help, clearsquare_syntax}
  ,
  {"ClearOctagon", 0, ActionClearOctagon,
   clearoctagon_help, clearoctagon_syntax}
  ,
  {"Connection", 0, ActionConnection,
   connection_help, connection_syntax}
  ,
  {"Delete", 0, ActionDelete,
   delete_help, delete_syntax}
  ,
  {"DeleteRats", 0, ActionDeleteRats,
   deleterats_help, deleterats_syntax}
  ,
  {"DisperseElements", 0, ActionDisperseElements,
   disperseelements_help, disperseelements_syntax}
  ,
  {"Display", 0, ActionDisplay,
   display_help, display_syntax}
  ,
  {"DRC", 0, ActionDRCheck,
   drc_help, drc_syntax}
  ,
  {"DumpLibrary", 0, ActionDumpLibrary,
   dumplibrary_help, dumplibrary_syntax}
  ,
  {"ExecuteFile", 0, ActionExecuteFile,
   executefile_help, executefile_syntax}
  ,
  {"Flip", N_("Click on Object or Flip Point"), ActionFlip,
   flip_help, flip_syntax}
  ,
  {"LoadFrom", 0, ActionLoadFrom,
   loadfrom_help, loadfrom_syntax}
  ,
  {"MarkCrosshair", 0, ActionMarkCrosshair,
   markcrosshair_help, markcrosshair_syntax}
  ,
  {"Message", 0, ActionMessage,
   message_help, message_syntax}
  ,
  {"MinMaskGap", 0, ActionMinMaskGap,
   minmaskgap_help, minmaskgap_syntax}
  ,
  {"MinClearGap", 0, ActionMinClearGap,
   mincleargap_help, mincleargap_syntax}
  ,
  {"Mode", 0, ActionMode,
   mode_help, mode_syntax}
  ,
  {"MorphPolygon", 0, ActionMorphPolygon,
   morphpolygon_help, morphpolygon_syntax}
  ,
  {"PasteBuffer", 0, ActionPasteBuffer,
   pastebuffer_help, pastebuffer_syntax}
  ,
  {"Quit", 0, ActionQuit,
   quit_help, quit_syntax}
  ,
  {"RemoveSelected", 0, ActionRemoveSelected,
   removeselected_help, removeselected_syntax}
  ,
  {"Renumber", 0, ActionRenumber,
   renumber_help, renumber_syntax}
  ,
  {"RipUp", 0, ActionRipUp,
   ripup_help, ripup_syntax}
  ,
  {"Select", 0, ActionSelect,
   select_help, select_syntax}
  ,
  {"Unselect", 0, ActionUnselect,
   unselect_help, unselect_syntax}
  ,
  {"SaveSettings", 0, ActionSaveSettings,
   savesettings_help, savesettings_syntax}
  ,
  {"SaveTo", 0, ActionSaveTo,
   saveto_help, saveto_syntax}
  ,
  {"SetSquare", 0, ActionSetSquare,
   setsquare_help, setsquare_syntax}
  ,
  {"SetOctagon", 0, ActionSetOctagon,
   setoctagon_help, setoctagon_syntax}
  ,
  {"SetThermal", 0, ActionSetThermal,
   setthermal_help, setthermal_syntax}
  ,
  {"SetValue", 0, ActionSetValue,
   setvalue_help, setvalue_syntax}
  ,
  {"ToggleHideName", 0, ActionToggleHideName,
   togglehidename_help, togglehidename_syntax}
  ,
  {"Undo", 0, ActionUndo,
   undo_help, undo_syntax}
  ,
  {"Redo", 0, ActionRedo,
   redo_help, redo_syntax}
  ,
  {"SetSame", N_("Select item to use attributes from"), ActionSetSame,
   setsame_help, setsame_syntax}
  ,
  {"SetFlag", 0, ActionSetFlag,
   setflag_help, setflag_syntax}
  ,
  {"ClrFlag", 0, ActionClrFlag,
   clrflag_help, clrflag_syntax}
  ,
  {"ChangeFlag", 0, ActionChangeFlag,
   changeflag_help, changeflag_syntax}
  ,
  {"Polygon", 0, ActionPolygon,
   polygon_help, polygon_syntax}
  ,
  {"RouteStyle", 0, ActionRouteStyle,
   routestyle_help, routestyle_syntax}
  ,
  {"MoveObject", N_("Select an Object"), ActionMoveObject,
   moveobject_help, moveobject_syntax}
  ,
  {"MoveToCurrentLayer", 0, ActionMoveToCurrentLayer,
   movetocurrentlayer_help, movetocurrentlayer_syntax}
  ,
  {"New", 0, ActionNew,
   new_help, new_syntax}
  ,
  {"pscalib", 0, ActionPSCalib}
  ,
  {"ElementList", 0, ActionElementList,
   elementlist_help, elementlist_syntax}
  ,
  {"ElementSetAttr", 0, ActionElementSetAttr,
   elementsetattr_help, elementsetattr_syntax}
  ,
  {"ExecCommand", 0, ActionExecCommand,
   execcommand_help, execcommand_syntax}
  ,
  {"Import", 0, ActionImport,
   import_help, import_syntax}
  ,
};

REGISTER_ACTIONS (action_action_list)

