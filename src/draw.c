/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2003 Thomas Nau
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

/* drawing routines
 */

/* ---------------------------------------------------------------------------
 * define TO_SCREEN before macro.h is included from global.h
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include "global.h"
#include "clip.h"
#include "compat.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "mymem.h"
#include "misc.h"
#include "polygon.h"
#include "rotate.h"
#include "rtree.h"
#include "search.h"
#include "select.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some local types
 */
typedef struct
{
  float X, Y;
}
FloatPolyType, *FloatPolyTypePtr;

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static int ZoomValue;		/* zoom for pin fonts */
static Window DrawingWindow;	/* flag common to all */
static BoxType Block;
static Boolean Gathering = True;
static int Erasing = False;


/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void Redraw (Boolean);
static void DrawEverything (void);
static void DrawTop (BoxType *);
static void DrawLayer (LayerTypePtr, BoxType *);
static void DrawSpecialPolygon (Drawable, GC, Location, Location, BDimension);
static void DrawPinOrViaLowLevel (PinTypePtr, Boolean);
static void ClearOnlyPin (PinTypePtr, Boolean);
static void ThermPin (LayerTypePtr, PinTypePtr);
static void DrawPlainPin (PinTypePtr, Boolean);
static void DrawPlainVia (PinTypePtr, Boolean);
static void DrawPlainElementPinsAndPads (ElementTypePtr, Boolean);
static void DrawPinOrViaNameLowLevel (PinTypePtr);
static void DrawPadLowLevel (PadTypePtr);
static void DrawPadNameLowLevel (PadTypePtr);
static void DrawLineLowLevel (LineTypePtr, Boolean);
static void DrawTextLowLevel (TextTypePtr);
static void DrawRegularText (LayerTypePtr, TextTypePtr, int);
static void DrawPolygonLowLevel (PolygonTypePtr);
static void DrawArcLowLevel (ArcTypePtr);
static void DrawElementPackageLowLevel (ElementTypePtr Element, int);
static void DrawPlainPolygon (LayerTypePtr Layer, PolygonTypePtr Polygon);
static void AddPart (void *, Boolean);
static void SetPVColor (PinTypePtr, int);
static void DrawGrid (void);
static void DrawEMark (Location, Location, Boolean);
static void ClearLine (LineTypePtr);
static void ClearArc (ArcTypePtr);
static void ClearPad (PadTypePtr, Boolean);
static void DrawHole (PinTypePtr);
static void DrawMask (BoxType *);

/*--------------------------------------------------------------------------------------
 * setup color for pin or via
 */
static void
SetPVColor (PinTypePtr Pin, int Type)
{
  if (Type == VIA_TYPE)
    {
      if (TEST_FLAG (WARNFLAG | SELECTEDFLAG | FOUNDFLAG, Pin))
	{
	  if (TEST_FLAG (WARNFLAG, Pin))
	    XSetForeground (Dpy, Output.fgGC, PCB->WarnColor);
	  else if (TEST_FLAG (SELECTEDFLAG, Pin))
	    XSetForeground (Dpy, Output.fgGC, PCB->ViaSelectedColor);
	  else
	    XSetForeground (Dpy, Output.fgGC, PCB->ConnectedColor);
	}
      else
	XSetForeground (Dpy, Output.fgGC, PCB->ViaColor);
    }
  else
    {
      if (TEST_FLAG (WARNFLAG | SELECTEDFLAG | FOUNDFLAG, Pin))
	{
	  if (TEST_FLAG (WARNFLAG, Pin))
	    XSetForeground (Dpy, Output.fgGC, PCB->WarnColor);
	  else if (TEST_FLAG (SELECTEDFLAG, Pin))
	    XSetForeground (Dpy, Output.fgGC, PCB->PinSelectedColor);
	  else
	    XSetForeground (Dpy, Output.fgGC, PCB->ConnectedColor);
	}
      else
	XSetForeground (Dpy, Output.fgGC, PCB->PinColor);
    }
}

/*---------------------------------------------------------------------------
 *  Adds the update rect to the update region
 */
static void
AddPart (void *b, Boolean screen_coord)
{
  BoxType *box = (BoxType *) b;

  if (screen_coord)
    {
      Block.X1 = MIN (Block.X1, box->X1);
      Block.X2 = MAX (Block.X2, box->X2);
      Block.Y1 = MIN (Block.Y1, box->Y1);
      Block.Y2 = MAX (Block.Y2, box->Y2);
    }
  else
    {
      Block.X1 = MIN (Block.X1, TO_LIMIT_X(box->X1));
      Block.X2 = MAX (Block.X2, TO_LIMIT_X(box->X2));
      Block.Y1 = MIN (Block.Y1, TO_LIMIT_Y(box->Y1));
      Block.Y2 = MAX (Block.Y2, TO_LIMIT_Y(box->Y2));
    }
}

/*
 * force the whole output to be updated
 */
void
UpdateAll (void)
{
  Block.X1 = 1;
  Block.Y1 = 1;
  Block.Y2 = MAX_COORD / 100 - 1;
  Block.X2 = MAX_COORD / 100 - 1;
  Draw ();
}

/*
 * initiate the actual drawing to the pixmap/screen
 * make the update block slightly larger to handle round-off
 * caused by the TO_SCREEN operation
 */
void
Draw (void)
{
  static XExposeEvent erased;

  render = True;
  if (VALID_PIXMAP (Offscreen))
    {
      XFillRectangle (Dpy, Offscreen, Output.bgGC, Block.X1 - 1,
		      Block.Y1 - 1, Block.X2 - Block.X1 + 2,
		      Block.Y2 - Block.Y1 + 2);
      /* create an update event */
      erased.type = Expose;
      erased.display = Dpy;
      erased.window = Output.OutputWindow;
      erased.x = Block.X1 - 1;
      erased.y = Block.Y1 - 1;
      erased.width = Block.X2 - Block.X1 + 2;
      erased.height = Block.Y2 - Block.Y1 + 2;
      erased.count = 0;
      XSendEvent (Dpy, Output.OutputWindow, False, ExposureMask,
		  (XEvent *) & erased);
    }
  else
    {
      HideCrosshair (True);
      /* clear and create event if not drawing to a pixmap */
      XClearArea (Dpy, Output.OutputWindow, Block.X1 - 1, Block.Y1 - 1,
		  Block.X2 - Block.X1 + 2, Block.Y2 - Block.Y1 + 2, True);
      RestoreCrosshair (True);
    }
}

/* ---------------------------------------------------------------------------
 * redraws the output area without clearing it
 */
void
RedrawOutput (void)
{
  Redraw (True);
}

/* ---------------------------------------------------------------------------
 * redraws the output area after clearing it
 */
void
ClearAndRedrawOutput (void)
{
  render = True;
  Gathering = False;
/*
	Redraw(True);
*/
  UpdateAll ();
}

/* ---------------------------------------------------------------------- 
 * redraws all the data
 * all necessary sizes are already set by the porthole widget and
 * by the event handlers
 */
static void
Redraw (Boolean ClearWindow)
{
  /* make sure window exists */
  if (Output.OutputWindow
      && (render || ClearWindow || !VALID_PIXMAP (Offscreen)))
    {
      /* shrink the update block */
      Block.X1 = TO_SCREEN (PCB->MaxWidth);
      Block.Y1 = TO_SCREEN (PCB->MaxHeight);
      Block.X2 = Block.Y2 = 0;

      /* switch off crosshair if needed,
       * set up drawing window and redraw
       * everything with Gather = False
       */
      if (!VALID_PIXMAP (Offscreen))
	HideCrosshair (True);
      SwitchDrawingWindow (PCB->Zoom,
			   VALID_PIXMAP (Offscreen) ? Offscreen :
			   Output.OutputWindow, Settings.ShowSolderSide,
			   False);

      /* clear the background
       * of the drawing area
       */
      XFillRectangle (Dpy, DrawingWindow, Output.bgGC, 0, 0,
		      TO_DRAWABS_X (PCB->MaxWidth),
		      TO_DRAWABS_Y (PCB->MaxHeight));
      XSetForeground (Dpy, Output.bgGC, Settings.bgColor);
      XSetForeground (Dpy, Output.fgGC, Settings.OffLimitColor);
      XFillRectangle (Dpy, DrawingWindow, Output.fgGC,
		      TO_DRAWABS_X (PCB->MaxWidth), 0, MAX_COORD / 100,
		      MAX_COORD / 100);
      XFillRectangle (Dpy, DrawingWindow, Output.fgGC, 0,
		      TO_DRAWABS_Y (PCB->MaxHeight), MAX_COORD / 100,
		      MAX_COORD / 100);
      if (ClearWindow && !VALID_PIXMAP (Offscreen))
	Crosshair.On = False;

      DrawEverything ();

      if (!VALID_PIXMAP (Offscreen))
	RestoreCrosshair (True);
    }
  Gathering = True;
  render = False;
}

/* ----------------------------------------------------------------------
 * setup of zoom and output window for the next drawing operations
 */
Boolean
SwitchDrawingWindow (float Zoom, Window OutputWindow, Boolean Swap,
		     Boolean Gather)
{
  Boolean oldGather = Gathering;

  Gathering = Gather;
  Local_Zoom = 0.01/expf(Zoom * LN_2_OVER_2);
  if (Zoom < 0)
    Zoom = 0;
  if (Zoom > 4)
    Zoom = 4;
  ZoomValue = (int)Zoom;
  DrawingWindow = OutputWindow;
  if (OutputWindow == Offscreen || OutputWindow == Output.OutputWindow)
    {
      dxo = Xorig;
      dyo = Yorig;
    }
  else
    {
      dxo = 0;
      dyo = 0;
    }
  SwapOutput = Swap;
  XSetFont (Dpy, Output.fgGC, Settings.PinoutFont[ZoomValue]->fid);
  XSetFont (Dpy, Output.bgGC, Settings.PinoutFont[ZoomValue]->fid);
  return (oldGather);
}

static int
viaHole_callback (const BoxType *b, void *cl)
{
  DrawHole((PinTypePtr)b);
  return 1;
}

static int
backE_callback (const BoxType *b, void *cl)
{
  ElementTypePtr element = (ElementTypePtr)b;
  
  if (!FRONT (element))
    {
      if (PCB->ElementOn)
	{
	  DrawElementPackage (element, 0);
	  if (VELTEXT (element))
	    DrawElementName (element, 0);
	}
    }
  if (PCB->PinOn && VELEMENT (element))
    PAD_LOOP (element, 
      {
        if (!FRONT (pad))
	  DrawPad (pad, 0);
      }
    );
  return 1;
}

static int
frontE_callback (const BoxType *b, void *cl)
{
  ElementTypePtr element = (ElementTypePtr)b;
  
  if (FRONT (element))
    {
      DrawElementPackage (element, 0);
      if (VELTEXT (element))
	DrawElementName (element, 0);
    }
  /* Draw pin holes */
  if (PCB->PinOn)
    {
      PIN_LOOP (element, 
	{
	  DrawHole (pin);
	}
      );
      DrawEMark (element->MarkX, element->MarkY, !FRONT (element));
    }
  return 1;
}

static int
holeE_callback (const BoxType *b, void *cl)
{
  ElementTypePtr element = (ElementTypePtr)b;

  PIN_LOOP (element, 
    {
      DrawHole (pin);
    }
  );
  return 1;
}

/* ---------------------------------------------------------------------------
 * initializes some identifiers for a new zoom factor and redraws whole screen
 */
static void
DrawEverything (void)
{
  int i;

  /*
   * first draw all 'invisible' stuff
   */
  if (PCB->InvisibleObjectsOn)
    {
      r_search(PCB->Data->element_tree, &theScreen, NULL, backE_callback, NULL);
      if (PCB->ElementOn)
      DrawLayer (LAYER_PTR (MAX_LAYER +
			      (SWAP_IDENT ? COMPONENT_LAYER :
			       SOLDER_LAYER)), &theScreen);
    }
  /* draw all layers in layerstack order */
  for (i = MAX_LAYER - 1; i >= 0; i--)
    if ((LAYER_ON_STACK (i))->On)
      DrawLayer (LAYER_ON_STACK (i), &theScreen);
  /* Draw pins, pads, vias */
  DrawTop (&theScreen);
  /* Draw the solder mask if turned on */
  if (TEST_FLAG (SHOWMASKFLAG, PCB))
    DrawMask (&theScreen);
  /* Draw top silkscreen */
  if (PCB->ElementOn)
    {
      DrawLayer (LAYER_PTR (MAX_LAYER + (SWAP_IDENT ? SOLDER_LAYER :
					 COMPONENT_LAYER)), &theScreen);

      r_search (PCB->Data->element_tree, &theScreen, NULL, frontE_callback, NULL);
    }
  else if (PCB->PinOn)
  /* Draw pin holes */
    r_search (PCB->Data->element_tree, &theScreen, NULL, holeE_callback, NULL);
  /* Draw via holes */
  if (PCB->ViaOn)
    r_search(PCB->Data->via_tree, &theScreen, NULL, viaHole_callback, NULL);
  /* Draw rat lines on top */
  if (PCB->RatOn)
    RAT_LOOP (PCB->Data, 
    {
      if (VLINE (line))
	DrawRat (line, 0);
    }
  );
  if (Settings.DrawGrid)
    DrawGrid ();
}

static void
DrawEMark (Location X, Location Y, Boolean invisible)
{
  if (!PCB->InvisibleObjectsOn && invisible)
    return;
  XSetForeground (Dpy, Output.fgGC,
		  invisible ? PCB->InvisibleMarkColor : PCB->ElementColor);
  XSetLineAttributes (Dpy, Output.fgGC, 1, LineSolid, CapRound, JoinRound);
  XDrawCLine (Dpy, DrawingWindow, Output.fgGC, X - EMARK_SIZE,
	     Y, X, Y - EMARK_SIZE);
  XDrawCLine (Dpy, DrawingWindow, Output.fgGC, X + EMARK_SIZE,
	     Y, X, Y - EMARK_SIZE);
  XDrawCLine (Dpy, DrawingWindow, Output.fgGC, X - EMARK_SIZE,
	     Y, X, Y + EMARK_SIZE);
  XDrawCLine (Dpy, DrawingWindow, Output.fgGC, X + EMARK_SIZE,
	     Y, X, Y + EMARK_SIZE);
}

static int
via_callback (const BoxType *b, void *cl)
{
  DrawPlainVia ((PinTypePtr)b, False);
  return 1;
}

static int
element_callback (const BoxType *b, void *cl)
{
  DrawPlainElementPinsAndPads ((ElementTypePtr)b, False);
  return 1;
}

/* ---------------------------------------------------------------------------
 * draws pins pads and vias
 */
static void
DrawTop (BoxType *screen)
{
  /* draw element pins */
  if (PCB->PinOn)
    r_search(PCB->Data->element_tree, screen, NULL, element_callback, NULL);
  /* draw vias */
  if (PCB->ViaOn)
    r_search(PCB->Data->via_tree, screen, NULL, via_callback, NULL);
}

struct pin_info
{
  Boolean arg;
  int PIPFlag;
  LayerTypePtr Layer;
};

static int
clearPin_callback (const BoxType *b, void *cl)
{
  PinTypePtr pin = (PinTypePtr)b;
  struct pin_info *i = (struct pin_info *)cl;
  if (i->arg)
    ClearOnlyPin (pin, True);
  else if (TEST_FLAG(i->PIPFlag, pin))
    ClearOnlyPin (pin, False);
  return 1;
}

/* ---------------------------------------------------------------------------
 * draws solder mask layer - this will cover nearly everything
 */
static void
DrawMask (BoxType *screen)
{
  struct pin_info info;

  info.arg = True;
  XSetFunction (Dpy, Output.pmGC, GXcopy);
  /* fill whole map first */
  XSetForeground (Dpy, Output.pmGC, AllPlanes);
  XFillRectangle (Dpy, Offmask, Output.pmGC, 0, 0, Output.Width,
		  Output.Height);
  XSetFillStyle (Dpy, Output.pmGC, FillSolid);
  /* make clearances in mask */
  XSetFunction (Dpy, Output.pmGC, GXclear);
  ALLPIN_LOOP (PCB->Data, 
    {
      ClearOnlyPin (pin, True);
    }
  );
  r_search(PCB->Data->via_tree, screen, NULL, clearPin_callback, &info);
  ALLPAD_LOOP (PCB->Data, 
    {
      if (!XOR (TEST_FLAG (ONSOLDERFLAG, pad), SWAP_IDENT))
	ClearPad (pad, True);
    }
  );
  XSetClipMask (Dpy, Output.fgGC, Offmask);
  /* now draw the mask */
  XSetForeground (Dpy, Output.fgGC, PCB->MaskColor);
  XFillRectangle (Dpy, DrawingWindow, Output.fgGC, 0, 0,
		  TO_DRAWABS_X (PCB->MaxWidth),
		  TO_DRAWABS_Y (PCB->MaxHeight));
  /* restore the clip region */
  XCopyGC (Dpy, Output.bgGC, GCClipMask, Output.fgGC);
}

static int
clear_callback (int type, void *ptr1, void *ptr2, void *ptr3,
                LayerTypePtr lay, PolygonTypePtr poly)
{
  LineTypePtr l = (LineTypePtr) ptr2;
  ArcTypePtr a = (ArcTypePtr) ptr2;

  switch (type)
    {
      case LINE_TYPE:
        ClearLine(l);
	break;
      case ARC_TYPE:
        ClearArc(a);
	break;
      case PIN_TYPE:
      case VIA_TYPE:
        ClearOnlyPin ((PinTypePtr) ptr2, False);
	break;
      case PAD_TYPE:
        ClearPad ((PadTypePtr) ptr2, False);
	break;
      default:
        Message("Bad clear callback\n");
    }
  return 0;
}

static int
line_callback (const BoxType *b, void *cl)
{
  DrawLine ((LayerTypePtr)cl, (LineTypePtr)b, 0);
  return 1;
}
    
static int
arc_callback (const BoxType *b, void *cl)
{
  DrawArc ((LayerTypePtr)cl, (ArcTypePtr)b, 0);
  return 1;
}

static int
therm_callback (const BoxType *b, void *cl)
{
  PinTypePtr pin = (PinTypePtr)b;
  struct pin_info *i = (struct pin_info *)cl;

  if (TEST_FLAGS (i->PIPFlag, pin))
    {
      ThermPin (i->Layer, pin);
      return 1;
    }
  return 0;
}
    

/* ---------------------------------------------------------------------------
 * draws one layer
 */
static void
DrawLayer (LayerTypePtr Layer, BoxType *screen)
{
  int layernum = GetLayerNumber (PCB->Data, Layer);
  Cardinal group = GetLayerGroupNumberByNumber (layernum);
  struct pin_info info;

  int PIPFlag = L0PIPFLAG << layernum;
  info.PIPFlag = PIPFlag;
  info.arg = False;
  info.Layer = Layer;
/* in order to render polygons with line cut-outs:
 * draw a solid (or stippled) 1-bit pixmap, then erase
 * the clearance areas.  Use that as the mask when
 * drawing the actual polygons
 */
  if (layernum < MAX_LAYER && Layer->PolygonN)
    {
      XSetFunction (Dpy, Output.pmGC, GXcopy);
      if (Settings.StipplePolygons)
	{
	  XSetBackground (Dpy, Output.pmGC, 0);
	  XSetStipple (Dpy, Output.pmGC, Stipples[layernum]);
	  XSetFillStyle (Dpy, Output.pmGC, FillOpaqueStippled);
	}
      /* fill whole map first */
      XSetForeground (Dpy, Output.pmGC, AllPlanes);
      XFillRectangle (Dpy, Offmask, Output.pmGC, 0, 0, Output.Width,
		      Output.Height);
      if (Settings.StipplePolygons)
	XSetFillStyle (Dpy, Output.pmGC, FillSolid);
      /* make clearances around lines, arcs, pins and vias */
      XSetFunction (Dpy, Output.pmGC, GXclear);
      PolygonPlows(group, screen, clear_callback);
      XSetClipMask (Dpy, Output.fgGC, Offmask);
    }
  if (Layer->PolygonN)
    {
       /* print the clearing polys */
      POLYGON_LOOP (Layer, 
	{
	  if (VPOLY (polygon) && TEST_FLAG (CLEARPOLYFLAG, polygon))
	    DrawPlainPolygon (Layer, polygon);
	}
      );
      /* restore the clip region */
      XCopyGC (Dpy, Output.bgGC, GCClipMask, Output.fgGC);
       /* print the non-clearing polys */
      POLYGON_LOOP (Layer, 
	{
	  if (VPOLY (polygon) && !TEST_FLAG (CLEARPOLYFLAG, polygon))
	    DrawPlainPolygon (Layer, polygon);
	}
      );
      if (layernum < MAX_LAYER)
	{
	  PIPFlag = (L0THERMFLAG | L0PIPFLAG) << layernum;
	  info.PIPFlag = PIPFlag;
	  ALLPIN_LOOP (PCB->Data, 
	    {
	      if (TEST_FLAGS (PIPFlag, pin) && VTHERM(pin))
		ThermPin (Layer, pin);
	    }
	  );
	  r_search(PCB->Data->via_tree, screen, NULL, therm_callback, &info);
	}
    }
  if (TEST_FLAG(CHECKPLANESFLAG, PCB))
    return;
   /* draw all lines on screen */
  r_search(Layer->line_tree, screen, NULL, line_callback, Layer);
   /* draw all arcs on screen */
  r_search(Layer->arc_tree, screen, NULL, arc_callback, Layer);
  TEXT_LOOP (Layer, 
    {
      if (VTEXT (text))
	DrawRegularText (Layer, text, 0);
    }
  );
}

/* ---------------------------------------------------------------------------
 * draws one polygon
 * x and y are already in display coordinates
 * the points are numbered:
 *
 *          5 --- 6
 *         /       \
 *        4         7
 *        |         |
 *        3         0
 *         \       /
 *          2 --- 1
 *
 */
static void
DrawSpecialPolygon (Drawable d, GC DrawGC,
		    Location X, Location Y, BDimension Thickness)
{
  static FloatPolyType p[8] = { {0.5, -TAN_22_5_DEGREE_2},
  {TAN_22_5_DEGREE_2, -0.5},
  {-TAN_22_5_DEGREE_2, -0.5},
  {-0.5, -TAN_22_5_DEGREE_2},
  {-0.5, TAN_22_5_DEGREE_2},
  {-TAN_22_5_DEGREE_2, 0.5},
  {TAN_22_5_DEGREE_2, 0.5},
  {0.5, TAN_22_5_DEGREE_2}
  };
  static Dimension special_size = 0;
  static XPoint scaled[8];
  XPoint polygon[9];
  int i;

  
  if (TO_SCREEN (Thickness) != special_size)
    {
      special_size = TO_SCREEN (Thickness);
      for (i = 0; i < 8; i++)
        {
          scaled[i].x = p[i].X * special_size;
          scaled[i].y = p[i].Y * special_size;
	}
    }
  /* add line offset */
  for (i = 0; i < 8; i++)
    {
      polygon[i].x = X + scaled[i].x;
      polygon[i].y = Y + scaled[i].y;
    }
  if (TEST_FLAG (THINDRAWFLAG, PCB))
    {
      XSetLineAttributes (Dpy, Output.fgGC, 1,
			  LineSolid, CapRound, JoinRound);
      polygon[8].x = X + scaled[0].x;
      polygon[8].y = Y + scaled[0].y;
      XDrawLines (Dpy, d, DrawGC, polygon, 9, CoordModeOrigin);
    }
  else
    XFillPolygon (Dpy, d, DrawGC, polygon, 8, Convex, CoordModeOrigin);
}

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for pins and vias
 */
static void
DrawPinOrViaLowLevel (PinTypePtr Ptr, Boolean drawHole)
{
  if (Gathering)
    {
      AddPart (Ptr, False);
      return;
    }

  if (TEST_FLAG (HOLEFLAG, Ptr))
    {
      if (drawHole)
	{
	  XSetLineAttributes (Dpy, Output.bgGC, TO_SCREEN (Ptr->Thickness),
			      LineSolid, CapRound, JoinRound);
	  XDrawCLine (Dpy, DrawingWindow, Output.bgGC, Ptr->X, Ptr->Y,
	              Ptr->X, Ptr->Y);
	  XSetLineAttributes (Dpy, Output.fgGC, 1, LineSolid, CapRound,
			      JoinRound);
	  XDrawCArc (Dpy, DrawingWindow, Output.fgGC, Ptr->X, Ptr->Y,
	             Ptr->Thickness, Ptr->Thickness, 0, 23040);
	}
      return;
    }
  if (TEST_FLAG (SQUAREFLAG, Ptr))
    {
      if (TEST_FLAG (THINDRAWFLAG, PCB))
	{
	  XSetLineAttributes (Dpy, Output.fgGC, 1,
			      LineSolid, CapRound, JoinRound);
	  XDrawRectangle (Dpy, DrawingWindow, Output.fgGC,
			  TO_DRAW_X (Ptr->X - Ptr->Thickness / 2),
			  TO_DRAW_Y (Ptr->Y -
				     TO_SCREEN_SIGN_Y (Ptr->Thickness / 2)),
			  TO_SCREEN (Ptr->Thickness),
			  TO_SCREEN (Ptr->Thickness));
	}
      else
	{
	  XFillRectangle (Dpy, DrawingWindow, Output.fgGC,
			  TO_DRAW_X (Ptr->X - Ptr->Thickness / 2),
			  TO_DRAW_Y (Ptr->Y -
				     TO_SCREEN_SIGN_Y (Ptr->Thickness / 2)),
			  TO_SCREEN (Ptr->Thickness),
			  TO_SCREEN (Ptr->Thickness));
	}
    }
  else if (TEST_FLAG (OCTAGONFLAG, Ptr))
    {
      XSetLineAttributes (Dpy, Output.fgGC,
			  TO_SCREEN ((Ptr->Thickness - Ptr->DrillingHole) /
				     2), LineSolid, CapRound, JoinRound);

      /* transform X11 specific coord system */
      DrawSpecialPolygon (DrawingWindow, Output.fgGC,
			  TO_DRAW_X (Ptr->X), TO_DRAW_Y (Ptr->Y),
			  Ptr->Thickness);
    }
  else
    {				/* draw a round pin or via */
      if (TEST_FLAG (THINDRAWFLAG, PCB))
	{
	  XSetLineAttributes (Dpy, Output.fgGC, 1,
			      LineSolid, CapRound, JoinRound);
	  XDrawCArc (Dpy, DrawingWindow, Output.fgGC, Ptr->X, Ptr->Y,
		    Ptr->Thickness, Ptr->Thickness, 0, 360 * 64);
	}
      else
	{
	  XSetLineAttributes (Dpy, Output.fgGC, TO_SCREEN (Ptr->Thickness),
			      LineSolid, CapRound, JoinRound);
	  XDrawCLine (Dpy, DrawingWindow, Output.fgGC, Ptr->X, Ptr->Y,
	              Ptr->X, Ptr->Y);
	}
    }

  /* and the drilling hole  (which is always round */
  if (drawHole)
    {
      if (TEST_FLAG (THINDRAWFLAG, PCB))
	{
	  XSetLineAttributes (Dpy, Output.fgGC, 1,
			      LineSolid, CapRound, JoinRound);
	  XDrawCArc (Dpy, DrawingWindow, Output.fgGC,
		    Ptr->X, Ptr->Y, Ptr->DrillingHole,
		    Ptr->DrillingHole, 0, 360 * 64);
	}
      else
	{
	  XSetLineAttributes (Dpy, Output.bgGC, TO_SCREEN (Ptr->DrillingHole),
			      LineSolid, CapRound, JoinRound);
	  XDrawCLine (Dpy, DrawingWindow, Output.bgGC, Ptr->X, Ptr->Y,
	              Ptr->X, Ptr->Y);
	}
    }
}

/**************************************************************
 * draw pin/via hole
 */
static void
DrawHole (PinTypePtr Ptr)
{
  if (TEST_FLAG (THINDRAWFLAG, PCB))
    {
      if (!TEST_FLAG (HOLEFLAG, Ptr))
	{
	  XSetLineAttributes (Dpy, Output.fgGC, 1,
			      LineSolid, CapRound, JoinRound);
	  XDrawCArc (Dpy, DrawingWindow, Output.fgGC, Ptr->X, Ptr->Y,
		    Ptr->DrillingHole, Ptr->DrillingHole, 0, 360 * 64);
	}
    }
  else
    {
      XSetLineAttributes (Dpy, Output.bgGC, TO_SCREEN (Ptr->DrillingHole),
			  LineSolid, CapRound, JoinRound);
      XDrawCLine (Dpy, DrawingWindow, Output.bgGC, Ptr->X,
		 Ptr->Y, Ptr->X, Ptr->Y);
    }
  if (TEST_FLAG (HOLEFLAG, Ptr))
    {
      if (TEST_FLAG(WARNFLAG, Ptr))
        XSetForeground (Dpy, Output.fgGC, PCB->WarnColor);
      else if (TEST_FLAG (SELECTEDFLAG, Ptr))
	XSetForeground (Dpy, Output.fgGC, PCB->ViaSelectedColor);
      else
	XSetForeground (Dpy, Output.fgGC,
			BlackPixelOfScreen (XtScreen (Output.Output)));
      XSetLineAttributes (Dpy, Output.fgGC, 1, LineSolid, CapRound,
			  JoinRound);
      XDrawCArc (Dpy, DrawingWindow, Output.fgGC, Ptr->X, Ptr->Y,
                 Ptr->DrillingHole, Ptr->DrillingHole, 0, 23040);
    }
}

/*******************************************************************
 * draw clearance in pixmask arround pins and vias that pierce polygons
 */
static void
ClearOnlyPin (PinTypePtr Pin, Boolean mask)
{
  BDimension half =
    (mask ? Pin->Mask / 2 : (Pin->Thickness + Pin->Clearance) / 2);

  if (!mask && TEST_FLAG (HOLEFLAG, Pin))
    return;

  /* Clear the area around the pin */
  if (TEST_FLAG (SQUAREFLAG, Pin))
    {
      XFillRectangle (Dpy, Offmask, Output.pmGC,
		      TO_DRAW_X (Pin->X - TO_SCREEN_SIGN_X (half)),
		      TO_DRAW_Y (Pin->Y - TO_SCREEN_SIGN_Y (half)),
		      TO_SCREEN (mask ? Pin->Mask : Pin->Thickness +
				 Pin->Clearance),
		      TO_SCREEN (mask ? Pin->Mask : Pin->Thickness +
				 Pin->Clearance));
    }
  else if (TEST_FLAG (OCTAGONFLAG, Pin))
    {
      XSetLineAttributes (Dpy, Output.pmGC,
			  TO_SCREEN ((Pin->Thickness + Pin->Clearance -
				      Pin->DrillingHole) / 2), LineSolid,
			  CapRound, JoinRound);

      /* transform X11 specific coord system */
      DrawSpecialPolygon (Offmask, Output.pmGC,
			  TO_DRAW_X (Pin->X), TO_DRAW_Y (Pin->Y),
			  mask ? Pin->Mask : Pin->Thickness +
				   Pin->Clearance);
    }
  else
    {
      XSetLineAttributes (Dpy, Output.pmGC,
			  TO_SCREEN (mask ? Pin->Mask : Pin->Thickness +
				     Pin->Clearance), LineSolid, CapRound,
			  JoinRound);
      XDrawCLine (Dpy, Offmask, Output.pmGC, Pin->X, Pin->Y, Pin->X, Pin->Y);
    }
}

/**************************************************************************
 * draw thermals on pin
 */
static void
ThermPin (LayerTypePtr layer, PinTypePtr Pin)
{
  BDimension half = (Pin->Thickness + Pin->Clearance) / 2;
  BDimension finger;

  if (TEST_FLAG (SELECTEDFLAG | FOUNDFLAG, Pin))
    {
      if (TEST_FLAG (SELECTEDFLAG, Pin))
	XSetForeground (Dpy, Output.fgGC, layer->SelectedColor);
      else
	XSetForeground (Dpy, Output.fgGC, PCB->ConnectedColor);
    }
  else
    XSetForeground (Dpy, Output.fgGC, layer->Color);

  finger = (Pin->Thickness - Pin->DrillingHole) * PCB->ThermScale;
  XSetLineAttributes (Dpy, Output.fgGC,
		      TEST_FLAG (THINDRAWFLAG, PCB) ? 1 : TO_SCREEN (finger),
		      LineSolid, CapRound, JoinRound);
  if (TEST_FLAG (SQUAREFLAG, Pin))
    {

      XDrawCLine (Dpy, DrawingWindow, Output.fgGC,
		 Pin->X - half, Pin->Y - half, Pin->X + half, Pin->Y + half);
      XDrawCLine (Dpy, DrawingWindow, Output.fgGC,
		 Pin->X - half, Pin->Y + half, Pin->X + half, Pin->Y - half);
    }
  else
    {
      BDimension halfs = (half * SQRT2OVER2 + 1);
      
      XDrawCLine (Dpy, DrawingWindow, Output.fgGC,
		 Pin->X - halfs, Pin->Y - halfs, Pin->X + halfs, Pin->Y + halfs);
      XDrawCLine (Dpy, DrawingWindow, Output.fgGC,
		 Pin->X - halfs, Pin->Y + halfs, Pin->X + halfs, Pin->Y - halfs);
    }
}

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for pins and vias that pierce polygons
 */
void
ClearPin (PinTypePtr Pin, int Type, int unused)
{
  int ThermLayerFlag;
  LayerTypePtr layer;
  Cardinal i;
  BDimension half = (Pin->Thickness + Pin->Clearance) / 2;

  if (Gathering)
    {
      AddPart (Pin, False);
      return;
    }
  /* Clear the area around the pin */
  if (TEST_FLAG (SQUAREFLAG, Pin))
    {
      XFillRectangle (Dpy, DrawingWindow, Output.bgGC,
		      TO_DRAW_X (Pin->X - TO_SCREEN_SIGN_X (half)),
		      TO_DRAW_Y (Pin->Y - TO_SCREEN_SIGN_Y (half)),
		      TO_SCREEN (Pin->Thickness + Pin->Clearance),
		      TO_SCREEN (Pin->Thickness + Pin->Clearance));
    }
  else if (TEST_FLAG (OCTAGONFLAG, Pin))
    {
      XSetLineAttributes (Dpy, Output.bgGC,
			  TO_SCREEN ((Pin->Thickness + Pin->Clearance -
				      Pin->DrillingHole) / 2), LineSolid,
			  CapRound, JoinRound);

      /* transform X11 specific coord system */
      DrawSpecialPolygon (DrawingWindow, Output.bgGC,
			  TO_DRAW_X (Pin->X), TO_DRAW_Y (Pin->Y),
			  Pin->Thickness + Pin->Clearance);
    }
  else
    {
      XSetLineAttributes (Dpy, Output.bgGC,
			  TO_SCREEN (Pin->Thickness + Pin->Clearance),
			  LineSolid, CapRound, JoinRound);
      XDrawCLine (Dpy, DrawingWindow, Output.bgGC, Pin->X,
		 Pin->Y, Pin->X, Pin->Y);
    }
  /* draw all the thermal(s) */
  for (i = MAX_LAYER; i; i--)
    {
      layer = LAYER_ON_STACK (i - 1);
      if (!layer->On)
	continue;
      ThermLayerFlag = (L0THERMFLAG | L0PIPFLAG) << GetLayerNumber(PCB->Data, layer);
      if (TEST_FLAGS (ThermLayerFlag, Pin))
	{
	  if (!Erasing)
	    {
	      if (TEST_FLAG (SELECTEDFLAG | FOUNDFLAG, Pin))
		{
		  if (TEST_FLAG (SELECTEDFLAG, Pin))
		    XSetForeground (Dpy, Output.fgGC, layer->SelectedColor);
		  else
		    XSetForeground (Dpy, Output.fgGC, PCB->ConnectedColor);
		}
	      else
		XSetForeground (Dpy, Output.fgGC, layer->Color);
	    }
	  if (TEST_FLAG (SQUAREFLAG, Pin))
	    {
	      XSetLineAttributes (Dpy, Output.fgGC,
				  TEST_FLAG (THINDRAWFLAG,
					     PCB) ? 1 : TO_SCREEN (Pin->
								   Clearance /
								   2),
				  LineSolid, CapRound, JoinRound);

	      XDrawCLine (Dpy, DrawingWindow, Output.fgGC, Pin->X - half,
	                  Pin->Y - half, Pin->X + half, Pin->Y + half);
	      XDrawCLine (Dpy, DrawingWindow, Output.fgGC, Pin->X - half,
			 Pin->Y + half, Pin->X + half, Pin->Y - half);
	    }
	  else
	    {
              BDimension halfs = (half * SQRT2OVER2 + 1);

	      XSetLineAttributes (Dpy, Output.fgGC,
				  TO_SCREEN (Pin->Clearance / 2), LineSolid,
				  CapRound, JoinRound);
	      XDrawCLine (Dpy, DrawingWindow, Output.fgGC, Pin->X - halfs,
			 Pin->Y - halfs, Pin->X + halfs, Pin->Y + halfs);
	      XDrawCLine (Dpy, DrawingWindow, Output.fgGC, Pin->X - halfs,
			 Pin->Y + halfs, Pin->X + halfs, Pin->Y - halfs);
	    }
	}
    }
  if ((!TEST_FLAG (PINFLAG, Pin) && !PCB->ViaOn)
      || (TEST_FLAG (PINFLAG, Pin) && !PCB->PinOn))
    return;
  /* now draw the pin or via as appropriate */
  switch (Type)
    {
    case VIA_TYPE:
    case PIN_TYPE:
      SetPVColor (Pin, Type);
      DrawPinOrViaLowLevel (Pin, True);
      break;
    case NO_TYPE:
    default:
      break;
    }
}

/* virtical text handling provided by Martin Devera with fixes by harry eaton */

/* draw vertical text; xywh is bounding, de is text's descend used for
   positioning */
static void
DrawVText (int x, int y, int w, int h, int de, char *str)
{
  Pixmap pm;
  GC gc;
  XImage *im;
  int i, j;

  if (strlen (str) == 0)
    return;

  pm = XCreatePixmap (Dpy, DrawingWindow, w, h, 1);
  gc = XCreateGC (Dpy, pm, 0, 0);

  /* draw into pixmap */
  XFillRectangle (Dpy, pm, gc, 0, 0, w, h);
  XSetForeground (Dpy, gc, 1);
  XSetFont (Dpy, gc, Settings.PinoutFont[ZoomValue]->fid);
  XDrawString (Dpy, pm, gc, 0, h - de, str,
	       MIN (Settings.PinoutNameLength, strlen (str)));

  im = XGetImage (Dpy, pm, 0, 0, w, h, 1, XYPixmap);

  /* draw Transpose(im) but only dark pixels; TODO: find a faster way */
  for (i = 0; i < w; i++)
    for (j = 0; j < h; j++)
      if (XGetPixel (im, i, j))
	XDrawPoint (Dpy, DrawingWindow, Output.fgGC, x + j, y + w - i - 1);
  XFreeGC (Dpy, gc);
  XFreePixmap (Dpy, pm);
}


/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for pin and via names
 */
static void
DrawPinOrViaNameLowLevel (PinTypePtr Ptr)
{
  int direction, ascent, descent;
  XCharStruct overall;
  char *name;
  BoxType box;

  name = EMPTY (TEST_FLAG (SHOWNUMBERFLAG, PCB) ? Ptr->Number : Ptr->Name);
  XTextExtents (Settings.PinoutFont[ZoomValue],
		name, MIN (Settings.PinoutNameLength, strlen (name)),
		&direction, &ascent, &descent, &overall);
  if (TEST_FLAG (EDGE2FLAG, Ptr))
    {
      box.X1 = TO_DRAW_X (Ptr->X) - ascent + descent;
      box.Y1 =
	TO_DRAW_Y (Ptr->Y + Ptr->Thickness / 2 + Settings.PinoutTextOffsetY)
	+ overall.lbearing;
    }
  else
    {
      box.X1 =
	TO_DRAW_X (Ptr->X + Ptr->Thickness / 2 + Settings.PinoutTextOffsetX);
      box.Y1 = TO_DRAW_Y (Ptr->Y) + overall.ascent / 2;
    }
  if (Gathering)
    {
      if (!TEST_FLAG (EDGE2FLAG, Ptr))
	{
	  box.X1 += overall.lbearing;
	  box.Y1 -= overall.ascent;
	  box.X2 = box.X1 + overall.width;
	  box.Y2 = box.Y1 + ascent + descent;
	}
      else
	{
	  box.X2 = box.X1 + ascent + descent;;
	  box.Y2 = box.Y1 + overall.width;
	}
      AddPart (&box, True);
      return;
    }
  if (TEST_FLAG (EDGE2FLAG, Ptr))
    DrawVText (box.X1, box.Y1, overall.width,
	       ascent + descent, descent, name);
  else
    XDrawString (Dpy, DrawingWindow, Output.fgGC,
		 box.X1, box.Y1,
		 name, MIN (Settings.PinoutNameLength, strlen (name)));

}

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for pads
 */

static void
DrawPadLowLevel (PadTypePtr Pad)
{
  if (Gathering)
    {
      AddPart (Pad, False);
      return;
    }

  if (TEST_FLAG (THINDRAWFLAG, PCB))
    {
      int x1, y1, x2, y2, t, t2;
      t = Pad->Thickness / 2;
      t2 = Pad->Thickness - t;
      x1 = Pad->Point1.X;
      y1 = Pad->Point1.Y;
      x2 = Pad->Point2.X;
      y2 = Pad->Point2.Y;
      if (x1 > x2 || y1 > y2)
	{
	  x1 ^= x2;
	  x2 ^= x1;
	  x1 ^= x2;
	  y1 ^= y2;
	  y2 ^= y1;
	  y1 ^= y2;
	}
      XSetLineAttributes (Dpy, Output.fgGC,
			  1, LineSolid, CapRound, JoinRound);
      if (TEST_FLAG (SQUAREFLAG, Pad))
	{
	  x1 -= t;
	  y1 -= t;
	  x2 += t2;
	  y2 += t2;
	  XDrawCLine (Dpy, DrawingWindow, Output.fgGC, x1, y1, x1, y2);
	  XDrawCLine (Dpy, DrawingWindow, Output.fgGC, x1, y2, x2, y2);
	  XDrawCLine (Dpy, DrawingWindow, Output.fgGC, x2, y2, x2, y1);
	  XDrawCLine (Dpy, DrawingWindow, Output.fgGC, x2, y1, x1, y1);
	}
      else if (x1 == x2)
	{
	  XDrawCLine (Dpy, DrawingWindow, Output.fgGC, x1 - t, y1, x2 - t, y2);
	  XDrawCLine (Dpy, DrawingWindow, Output.fgGC, x1 + t2, y1, x2 + t2,
		     y2);
	  XDrawCArc (Dpy, DrawingWindow, Output.fgGC, x1, y1, Pad->Thickness,
		    Pad->Thickness, 0, 180 * 64);
	  XDrawCArc (Dpy, DrawingWindow, Output.fgGC, x2, y2, Pad->Thickness,
		    Pad->Thickness, 180 * 64, 180 * 64);
	}
      else
	{
	  XDrawCLine (Dpy, DrawingWindow, Output.fgGC, x1, y1 - t, x2, y2 - t);
	  XDrawCLine (Dpy, DrawingWindow, Output.fgGC, x1, y1 + t2, x2,
		     y2 + t2);
	  XDrawCArc (Dpy, DrawingWindow, Output.fgGC, x1, y1, Pad->Thickness, 
		    Pad->Thickness, 90 * 64, 180 * 64);
	  XDrawCArc (Dpy, DrawingWindow, Output.fgGC, x2, y2, Pad->Thickness,
		    Pad->Thickness, 270 * 64, 180 * 64);
	}
    }
  else
    {

      XSetLineAttributes (Dpy, Output.fgGC,
			  TO_SCREEN (Pad->Thickness), LineSolid,
			  TEST_FLAG (SQUAREFLAG,
				     Pad) ? CapProjecting : CapRound,
			  JoinRound);
      XDrawCLine (Dpy, DrawingWindow, Output.fgGC, Pad->Point1.X,
		 Pad->Point1.Y, Pad->Point2.X, Pad->Point2.Y);
    }
}

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for pad names
 */

static void
DrawPadNameLowLevel (PadTypePtr Pad)
{
  int direction, ascent, descent, vert;
  XCharStruct overall;
  char *name;
  BoxType box;

  name = EMPTY (TEST_FLAG (SHOWNUMBERFLAG, PCB) ? Pad->Number : Pad->Name);

  XTextExtents (Settings.PinoutFont[ZoomValue], name,
		MIN (Settings.PinoutNameLength, strlen (name)),
		&direction, &ascent, &descent, &overall);
  /* should text be vertical ? */
  vert = (Pad->Point1.X == Pad->Point2.X);
  if (vert)
    {
      box.X1 = TO_DRAW_X (Pad->Point1.X) - ascent + descent;
      box.Y1 =
	TO_DRAW_Y (MAX (Pad->Point1.Y, Pad->Point2.Y) + Pad->Thickness / 2);
    }
  else
    {
      box.X1 =
	TO_DRAW_X (MAX (Pad->Point1.X, Pad->Point2.X) + Pad->Thickness / 2);
      box.Y1 = TO_DRAW_Y (Pad->Point1.Y) + overall.ascent / 2;
    }

  if (vert)
    box.Y1 += overall.lbearing + TO_SCREEN (Settings.PinoutTextOffsetY);
  else
    box.X1 += TO_SCREEN (Settings.PinoutTextOffsetX);

  if (Gathering)
    {
      if (vert)
	{
	  box.X2 = box.X1 + ascent + descent;
	  box.Y2 = box.Y1 + overall.width;
	}
      else
	{
	  box.X1 += overall.lbearing;
	  box.Y1 -= ascent;
	  box.X2 = box.X1 + overall.width;
	  box.Y2 = box.Y1 + ascent + descent;
	}
      AddPart (&box, True);
      return;
    }

  if (vert)
    DrawVText (box.X1, box.Y1, overall.width,
	       ascent + descent, descent, name);
  else
    XDrawString (Dpy, DrawingWindow, Output.fgGC,
		 box.X1, box.Y1,
		 name, MIN (Settings.PinoutNameLength, strlen (name)));
}

/* ---------------------------------------------------------------------------
 * clearance for pads
 */
static void
ClearPad (PadTypePtr Pad, Boolean mask)
{
  XSetLineAttributes (Dpy, Output.pmGC,
		      TO_SCREEN (mask ? Pad->Mask : Pad->Thickness +
				 Pad->Clearance), LineSolid,
		      TEST_FLAG (SQUAREFLAG, Pad) ? CapProjecting : CapRound,
		      JoinRound);
  XDrawCLine (Dpy, Offmask, Output.pmGC, Pad->Point1.X, Pad->Point1.Y,
              Pad->Point2.X, Pad->Point2.Y);
}

/* ---------------------------------------------------------------------------
 * clearance for lines
 */
static void
ClearLine (LineTypePtr Line)
{
  XSetLineAttributes (Dpy, Output.pmGC,
		      TO_SCREEN (Line->Clearance + Line->Thickness),
		      LineSolid, CapRound, JoinRound);
  XDrawCLine (Dpy, Offmask, Output.pmGC, Line->Point1.X, Line->Point1.Y,
              Line->Point2.X, Line->Point2.Y);
}

/* ---------------------------------------------------------------------------
 * clearance for arcs 
 */
static void
ClearArc (ArcTypePtr Arc)
{
  XSetLineAttributes (Dpy, Output.pmGC,
		      TO_SCREEN (Arc->Thickness + Arc->Clearance),
		      LineSolid, CapRound, JoinRound);
  XDrawCArc (Dpy, Offmask, Output.pmGC, Arc->X, Arc->Y,
            2 * Arc->Width, 2 * Arc->Height,
	    (Arc->StartAngle - 180) * 64, Arc->Delta * 64);
}

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for lines
 */
static void
DrawLineLowLevel (LineTypePtr Line, Boolean HaveGathered)
{
  if (Gathering && !HaveGathered)
    {
      AddPart (Line, False);
      return;
    }

  if (TEST_FLAG (THINDRAWFLAG, PCB))
    XSetLineAttributes (Dpy, Output.fgGC, 1, LineSolid, CapRound, JoinRound);
  else
    XSetLineAttributes (Dpy, Output.fgGC,
			TO_SCREEN (Line->Thickness), LineSolid, CapRound,
			JoinRound);

  if (TEST_FLAG (RATFLAG, Line))
    {
      XSetStipple (Dpy, Output.fgGC, Stipples[0]);
      XSetFillStyle (Dpy, Output.fgGC, FillStippled);
      XDrawCLine (Dpy, DrawingWindow, Output.fgGC,
		 Line->Point1.X, Line->Point1.Y,
		 Line->Point2.X, Line->Point2.Y);
      XSetFillStyle (Dpy, Output.fgGC, FillSolid);
    }
  else
    XDrawCLine (Dpy, DrawingWindow, Output.fgGC,
	       Line->Point1.X, Line->Point1.Y,
	       Line->Point2.X, Line->Point2.Y);
}

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for text objects
 */
static void
DrawTextLowLevel (TextTypePtr Text)
{
  Location x = 0;
  unsigned char *string = (unsigned char *) Text->TextString;
  Cardinal n;
  FontTypePtr font = &PCB->Font;

  if (Gathering)
    {
      AddPart (Text, False);
      return;
    }

  while (string && *string)
    {
      /* draw lines if symbol is valid and data is present */
      if (*string <= MAX_FONTPOSITION && font->Symbol[*string].Valid)
	{
	  LineTypePtr line = font->Symbol[*string].Line;
	  LineType newline;

	  for (n = font->Symbol[*string].LineN; n; n--, line++)
	    {
	      /* create one line, scale, move, rotate and swap it */
	      newline = *line;
	      newline.Point1.X = (newline.Point1.X + x) * Text->Scale / 100;
	      newline.Point1.Y = newline.Point1.Y * Text->Scale / 100;
	      newline.Point2.X = (newline.Point2.X + x) * Text->Scale / 100;
	      newline.Point2.Y = newline.Point2.Y * Text->Scale / 100;
	      newline.Thickness = newline.Thickness * Text->Scale / 200;
	      if (newline.Thickness < 800)
		newline.Thickness = 800;

	      RotateLineLowLevel (&newline, 0, 0, Text->Direction);

	      /* the labels of SMD objects on the bottom
	       * side haven't been swapped yet, only their offset
	       */
	      if (TEST_FLAG (ONSOLDERFLAG, Text))
		{
		  newline.Point1.X = SWAP_SIGN_X (newline.Point1.X);
		  newline.Point1.Y = SWAP_SIGN_Y (newline.Point1.Y);
		  newline.Point2.X = SWAP_SIGN_X (newline.Point2.X);
		  newline.Point2.Y = SWAP_SIGN_Y (newline.Point2.Y);
		}
	      /* add offset and draw line */
	      newline.Point1.X += Text->X;
	      newline.Point1.Y += Text->Y;
	      newline.Point2.X += Text->X;
	      newline.Point2.Y += Text->Y;
	      DrawLineLowLevel (&newline, True);
	    }

	  /* move on to next cursor position */
	  x += (font->Symbol[*string].Width + font->Symbol[*string].Delta);
	}
      else
	{
	  /* the default symbol is a filled box */
	  BoxType defaultsymbol = PCB->Font.DefaultSymbol;
	  Location size = (defaultsymbol.X2 - defaultsymbol.X1) * 6 / 5;

	  defaultsymbol.X1 = (defaultsymbol.X1 + x) * Text->Scale / 100;
	  defaultsymbol.Y1 = defaultsymbol.Y1 * Text->Scale / 100;
	  defaultsymbol.X2 = (defaultsymbol.X2 + x) * Text->Scale / 100;
	  defaultsymbol.Y2 = defaultsymbol.Y2 * Text->Scale / 100;

	  if (TEST_FLAG (ONSOLDERFLAG, Text))
	    {
	      defaultsymbol.X1 = TO_SCREEN_SIGN_X (defaultsymbol.X1);
	      defaultsymbol.Y1 = TO_SCREEN_SIGN_Y (defaultsymbol.Y1);
	      defaultsymbol.X2 = TO_SCREEN_SIGN_X (defaultsymbol.X2);
	      defaultsymbol.Y2 = TO_SCREEN_SIGN_Y (defaultsymbol.Y2);
	    }
	  RotateBoxLowLevel (&defaultsymbol, 0, 0, Text->Direction);

	  /* add offset and draw box */
	  defaultsymbol.X1 += Text->X;
	  defaultsymbol.Y1 += Text->Y;
	  defaultsymbol.X2 += Text->X;
	  defaultsymbol.Y2 += Text->Y;
	  XFillRectangle (Dpy, DrawingWindow, Output.fgGC,
			  TO_DRAW_X (defaultsymbol.X1),
			  TO_DRAW_Y (SWAP_IDENT ? defaultsymbol.Y2 :
				     defaultsymbol.Y1),
			  TO_SCREEN (abs
				     (defaultsymbol.X2 - defaultsymbol.X1)),
			  TO_SCREEN (abs
				     (defaultsymbol.Y2 - defaultsymbol.Y1)));

	  /* move on to next cursor position */
	  x += size;
	}
      string++;
    }
}

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for polygons
 */
static void
DrawPolygonLowLevel (PolygonTypePtr Polygon)
{
  if (Gathering)
    {
      AddPart (Polygon, False);
      return;
    }
  DrawCPolygon (DrawingWindow, Polygon);
}

/* ---------------------------------------------------------------------------
 * lowlevel routine to element arcs
 */
static void
DrawArcLowLevel (ArcTypePtr Arc)
{
  if (Gathering)
    {
      AddPart (Arc, False);
      return;
    }
  /* angles have to be converted to X11 notation */
  if (TEST_FLAG (THINDRAWFLAG, PCB))
    XSetLineAttributes (Dpy, Output.fgGC, 1, LineSolid, CapRound, JoinRound);
  else
    XSetLineAttributes (Dpy, Output.fgGC,
			TO_SCREEN (Arc->Thickness), LineSolid, CapRound,
			JoinRound);

  XDrawCArc (Dpy, DrawingWindow, Output.fgGC, Arc->X, Arc->Y, 2 * Arc->Width,
            2 * Arc->Height, (Arc->StartAngle - 180) * 64, Arc->Delta * 64);
}

/* ---------------------------------------------------------------------------
 * draws the package of an element
 */
static void
DrawElementPackageLowLevel (ElementTypePtr Element, int unused)
{
  /* draw lines, arcs, text and pins */
  ELEMENTLINE_LOOP (Element, 
    {
      DrawLineLowLevel (line, False);
    }
  );
  ARC_LOOP (Element, 
    {
      DrawArcLowLevel (arc);
    }
  );
}

/* ---------------------------------------------------------------------------
 * draw a via object
 */
void
DrawVia (PinTypePtr Via, int unused)
{
  if (!Gathering)
    SetPVColor (Via, VIA_TYPE);
  if (!TEST_FLAG (HOLEFLAG, Via) && TEST_FLAG (ALLPIPFLAGS, Via))
    ClearPin (Via, VIA_TYPE, 0);
  else
    DrawPinOrViaLowLevel (Via, True);
  if (!TEST_FLAG (HOLEFLAG, Via) && TEST_FLAG (DISPLAYNAMEFLAG, Via))
    DrawPinOrViaNameLowLevel (Via);
}

/* ---------------------------------------------------------------------------
 * draw a via without dealing with polygon clearance 
 */
static void
DrawPlainVia (PinTypePtr Via, Boolean holeToo)
{
  if (!Gathering)
    SetPVColor (Via, VIA_TYPE);
  DrawPinOrViaLowLevel (Via, holeToo);
  if (!TEST_FLAG (HOLEFLAG, Via) && TEST_FLAG (DISPLAYNAMEFLAG, Via))
    DrawPinOrViaNameLowLevel (Via);
}

/* ---------------------------------------------------------------------------
 * draws the name of a via
 */
void
DrawViaName (PinTypePtr Via, int unused)
{
  if (!Gathering)
    {
      if (TEST_FLAG (SELECTEDFLAG, Via))
	XSetForeground (Dpy, Output.fgGC, PCB->ViaSelectedColor);
      else
	XSetForeground (Dpy, Output.fgGC, PCB->ViaColor);
    }
  DrawPinOrViaNameLowLevel (Via);
}

/* ---------------------------------------------------------------------------
 * draw a pin object
 */
void
DrawPin (PinTypePtr Pin, int unused)
{
  if (!TEST_FLAG (HOLEFLAG, Pin) && TEST_FLAG (ALLPIPFLAGS, Pin))
    ClearPin (Pin, PIN_TYPE, 0);
  else
    {
      if (!Gathering)
	SetPVColor (Pin, PIN_TYPE);
      DrawPinOrViaLowLevel (Pin, True);
    }
  if (!TEST_FLAG (HOLEFLAG, Pin) && TEST_FLAG (DISPLAYNAMEFLAG, Pin))
    DrawPinOrViaNameLowLevel (Pin);
}

/* ---------------------------------------------------------------------------
 * draw a pin without clearing around polygons 
 */
static void
DrawPlainPin (PinTypePtr Pin, Boolean holeToo)
{
  if (!Gathering)
    SetPVColor (Pin, PIN_TYPE);
  DrawPinOrViaLowLevel (Pin, holeToo);
  if (!TEST_FLAG (HOLEFLAG, Pin) && TEST_FLAG (DISPLAYNAMEFLAG, Pin))
    DrawPinOrViaNameLowLevel (Pin);
}

/* ---------------------------------------------------------------------------
 * draws the name of a pin
 */
void
DrawPinName (PinTypePtr Pin, int unused)
{
  if (!Gathering)
    {
      if (TEST_FLAG (SELECTEDFLAG, Pin))
	XSetForeground (Dpy, Output.fgGC, PCB->PinSelectedColor);
      else
	XSetForeground (Dpy, Output.fgGC, PCB->PinColor);
    }
  DrawPinOrViaNameLowLevel (Pin);
}

/* ---------------------------------------------------------------------------
 * draw a pad object
 */
void
DrawPad (PadTypePtr Pad, int unused)
{
  if (!Gathering)
    {
      if (TEST_FLAG (WARNFLAG | SELECTEDFLAG | FOUNDFLAG, Pad))
	{
	  if (TEST_FLAG (WARNFLAG, Pad))
	    XSetForeground (Dpy, Output.fgGC, PCB->WarnColor);
	  else if (TEST_FLAG (SELECTEDFLAG, Pad))
	    XSetForeground (Dpy, Output.fgGC, PCB->PinSelectedColor);
	  else
	    XSetForeground (Dpy, Output.fgGC, PCB->ConnectedColor);
	}
      else if (FRONT (Pad))
	XSetForeground (Dpy, Output.fgGC, PCB->PinColor);
      else
	XSetForeground (Dpy, Output.fgGC, PCB->InvisibleObjectsColor);
    }
  DrawPadLowLevel (Pad);
  if (TEST_FLAG (DISPLAYNAMEFLAG, Pad))
    DrawPadNameLowLevel (Pad);
}

/* ---------------------------------------------------------------------------
 * draws the name of a pad
 */
void
DrawPadName (PadTypePtr Pad, int unused)
{
  if (!Gathering)
    {
      if (TEST_FLAG (SELECTEDFLAG, Pad))
	XSetForeground (Dpy, Output.fgGC, PCB->PinSelectedColor);
      else if (FRONT (Pad))
	XSetForeground (Dpy, Output.fgGC, PCB->PinColor);
      else
	XSetForeground (Dpy, Output.fgGC, PCB->InvisibleObjectsColor);
    }
  DrawPadNameLowLevel (Pad);
}

/* ---------------------------------------------------------------------------
 * draws a line on a layer
 */
void
DrawLine (LayerTypePtr Layer, LineTypePtr Line, int unused)
{
  if (!Gathering)
    {
      if (TEST_FLAG (SELECTEDFLAG | FOUNDFLAG, Line))
	{
	  if (TEST_FLAG (SELECTEDFLAG, Line))
	    XSetForeground (Dpy, Output.fgGC, Layer->SelectedColor);
	  else
	    XSetForeground (Dpy, Output.fgGC, PCB->ConnectedColor);
	}
      else
	XSetForeground (Dpy, Output.fgGC, Layer->Color);
    }
  DrawLineLowLevel (Line, False);
}

/* ---------------------------------------------------------------------------
 * draws a ratline
 */
void
DrawRat (RatTypePtr Line, int unused)
{
  if (!Gathering)
    {
      if (TEST_FLAG (SELECTEDFLAG | FOUNDFLAG, Line))
	{
	  if (TEST_FLAG (SELECTEDFLAG, Line))
	    XSetForeground (Dpy, Output.fgGC, PCB->RatSelectedColor);
	  else
	    XSetForeground (Dpy, Output.fgGC, PCB->ConnectedColor);
	}
      else
	XSetForeground (Dpy, Output.fgGC, PCB->RatColor);
    }
  DrawLineLowLevel ((LineTypePtr) Line, False);
}

/* ---------------------------------------------------------------------------
 * draws an arc on a layer
 */
void
DrawArc (LayerTypePtr Layer, ArcTypePtr Arc, int unused)
{
  if (!Gathering)
    {
      if (TEST_FLAG (SELECTEDFLAG | FOUNDFLAG, Arc))
	{
	  if (TEST_FLAG (SELECTEDFLAG, Arc))
	    XSetForeground (Dpy, Output.fgGC, Layer->SelectedColor);
	  else
	    XSetForeground (Dpy, Output.fgGC, PCB->ConnectedColor);
	}
      else
	XSetForeground (Dpy, Output.fgGC, Layer->Color);
    }
  DrawArcLowLevel (Arc);
}

/* ---------------------------------------------------------------------------
 * draws a text on a layer
 */
void
DrawText (LayerTypePtr Layer, TextTypePtr Text, int unused)
{
  if (!Layer->On)
    return;
  if (TEST_FLAG (SELECTEDFLAG, Text))
    XSetForeground (Dpy, Output.fgGC, Layer->SelectedColor);
  else
    XSetForeground (Dpy, Output.fgGC, Layer->Color);
  DrawTextLowLevel (Text);
}

/* ---------------------------------------------------------------------------
 * draws text on a layer - assumes it's not on silkscreen
 */
static void
DrawRegularText (LayerTypePtr Layer, TextTypePtr Text, int unused)
{
  if (TEST_FLAG (SELECTEDFLAG, Text))
    XSetForeground (Dpy, Output.fgGC, Layer->SelectedColor);
  else
    XSetForeground (Dpy, Output.fgGC, Layer->Color);
  DrawTextLowLevel (Text);
}

/* ---------------------------------------------------------------------------
 * draws a polygon on a layer
 */
void
DrawPolygon (LayerTypePtr Layer, PolygonTypePtr Polygon, int unused)
{
  int layernum;

  if (TEST_FLAG (SELECTEDFLAG | FOUNDFLAG, Polygon))
    {
      if (TEST_FLAG (SELECTEDFLAG, Polygon))
	XSetForeground (Dpy, Output.fgGC, Layer->SelectedColor);
      else
	XSetForeground (Dpy, Output.fgGC, PCB->ConnectedColor);
    }
  else
    XSetForeground (Dpy, Output.fgGC, Layer->Color);
  layernum = GetLayerNumber (PCB->Data, Layer);
  if (Settings.StipplePolygons)
    {
      XSetStipple (Dpy, Output.fgGC, Stipples[layernum]);
      XSetFillStyle (Dpy, Output.fgGC, FillStippled);
    }
  DrawPolygonLowLevel (Polygon);
  if (Settings.StipplePolygons)
    XSetFillStyle (Dpy, Output.fgGC, FillSolid);
  if (TEST_FLAG (CLEARPOLYFLAG, Polygon))
    {
      ALLPIN_LOOP (PCB->Data, 
	{
	  if (IsPointInPolygon (pin->X, pin->Y, (pin->Thickness+pin->Clearance)/2, Polygon))
	    {
	      ClearPin (pin, PIN_TYPE, 0);
	    }
	}
      );
      VIA_LOOP (PCB->Data, 
	{
	  if (IsPointInPolygon (via->X, via->Y, (via->Thickness+via->Clearance)/2, Polygon))
	    {
	      ClearPin (via, VIA_TYPE, 0);
	    }
	}
      );
    }
}

/* ---------------------------------------------------------------------------
 * draws a polygon without cutting away the pin/via clearances
 */
static void
DrawPlainPolygon (LayerTypePtr Layer, PolygonTypePtr Polygon)
{
  if (TEST_FLAG (SELECTEDFLAG | FOUNDFLAG, Polygon))
    {
      if (TEST_FLAG (SELECTEDFLAG, Polygon))
	XSetForeground (Dpy, Output.fgGC, Layer->SelectedColor);
      else
	XSetForeground (Dpy, Output.fgGC, PCB->ConnectedColor);
    }
  else
    XSetForeground (Dpy, Output.fgGC, Layer->Color);
  DrawPolygonLowLevel (Polygon);
}

/* ---------------------------------------------------------------------------
 * draws an element
 */
void
DrawElement (ElementTypePtr Element, int unused)
{
  DrawElementPackage (Element, unused);
  DrawElementName (Element, unused);
  DrawElementPinsAndPads (Element, unused);
}

/* ---------------------------------------------------------------------------
 * draws the name of an element
 */
void
DrawElementName (ElementTypePtr Element, int unused)
{
  if (TEST_FLAG (HIDENAMEFLAG, Element))
    return;
  if (TEST_FLAG (SELECTEDFLAG, &ELEMENT_TEXT (PCB, Element)))
    XSetForeground (Dpy, Output.fgGC, PCB->ElementSelectedColor);
  else if (FRONT (Element))
    XSetForeground (Dpy, Output.fgGC, PCB->ElementColor);
  else
    XSetForeground (Dpy, Output.fgGC, PCB->InvisibleObjectsColor);
  DrawTextLowLevel (&ELEMENT_TEXT (PCB, Element));
}

/* ---------------------------------------------------------------------------
 * draws the package of an element
 */
void
DrawElementPackage (ElementTypePtr Element, int unused)
{
  /* set color and draw lines, arcs, text and pins */
  if (TEST_FLAG (SELECTEDFLAG, Element))
    XSetForeground (Dpy, Output.fgGC, PCB->ElementSelectedColor);
  else if (FRONT (Element))
    XSetForeground (Dpy, Output.fgGC, PCB->ElementColor);
  else
    XSetForeground (Dpy, Output.fgGC, PCB->InvisibleObjectsColor);
  DrawElementPackageLowLevel (Element, unused);
}

/* ---------------------------------------------------------------------------
 * draw pins of an element
 */
void
DrawElementPinsAndPads (ElementTypePtr Element, int unused)
{
  PAD_LOOP (Element, 
    {
      if (FRONT (pad) || PCB->InvisibleObjectsOn)
	DrawPad (pad, unused);
    }
  );
  PIN_LOOP (Element, 
    {
      DrawPin (pin, unused);
    }
  );
}

/* ---------------------------------------------------------------------------
 * draw pins of an element without clearing around polygons
 */
static void
DrawPlainElementPinsAndPads (ElementTypePtr Element, Boolean holeToo)
{
  /* don't draw invisible pads, they're already handled */
  PAD_LOOP (Element, 
    {
      if (FRONT (pad))
	DrawPad (pad, 0);
    }
  );
  PIN_LOOP (Element, 
    {
      DrawPlainPin (pin, holeToo);
    }
  );
}

/* ---------------------------------------------------------------------------
 * erase a via
 */
void
EraseVia (PinTypePtr Via)
{
  Erasing++;
  if (TEST_FLAG (ALLPIPFLAGS, Via))
    ClearPin (Via, NO_TYPE, 0);
  XSetForeground (Dpy, Output.fgGC, Settings.bgColor);
  DrawPinOrViaLowLevel (Via, False);
  if (TEST_FLAG (DISPLAYNAMEFLAG, Via))
    DrawPinOrViaNameLowLevel (Via);
  Erasing--;
}

/* ---------------------------------------------------------------------------
 * erase a ratline
 */
void
EraseRat (RatTypePtr Rat)
{
  Erasing++;
  XSetForeground (Dpy, Output.fgGC, Settings.bgColor);
  DrawLineLowLevel ((LineTypePtr) Rat, False);
  Erasing--;
}


/* ---------------------------------------------------------------------------
 * erase a via name
 */
void
EraseViaName (PinTypePtr Via)
{
  Erasing++;
  XSetForeground (Dpy, Output.fgGC, Settings.bgColor);
  DrawPinOrViaNameLowLevel (Via);
  Erasing--;
}

/* ---------------------------------------------------------------------------
 * erase a pad object
 */
void
ErasePad (PadTypePtr Pad)
{
  Erasing++;
  XSetForeground (Dpy, Output.fgGC, Settings.bgColor);
  DrawPadLowLevel (Pad);
  if (TEST_FLAG (DISPLAYNAMEFLAG, Pad))
    DrawPadNameLowLevel (Pad);
  Erasing--;
}

/* ---------------------------------------------------------------------------
 * erase a pad name
 */
void
ErasePadName (PadTypePtr Pad)
{
  Erasing++;
  XSetForeground (Dpy, Output.fgGC, Settings.bgColor);
  DrawPadNameLowLevel (Pad);
  Erasing--;
}

/* ---------------------------------------------------------------------------
 * erase a pin object
 */
void
ErasePin (PinTypePtr Pin)
{
  Erasing++;
  if (TEST_FLAG (ALLPIPFLAGS, Pin))
    ClearPin (Pin, NO_TYPE, 0);
  XSetForeground (Dpy, Output.fgGC, Settings.bgColor);
  DrawPinOrViaLowLevel (Pin, False);
  if (TEST_FLAG (DISPLAYNAMEFLAG, Pin))
    DrawPinOrViaNameLowLevel (Pin);
  Erasing--;
}

/* ---------------------------------------------------------------------------
 * erase a pin name
 */
void
ErasePinName (PinTypePtr Pin)
{
  Erasing++;
  XSetForeground (Dpy, Output.fgGC, Settings.bgColor);
  DrawPinOrViaNameLowLevel (Pin);
  Erasing--;
}

/* ---------------------------------------------------------------------------
 * erases a line on a layer
 */
void
EraseLine (LineTypePtr Line)
{
  Erasing++;
  XSetForeground (Dpy, Output.fgGC, Settings.bgColor);
  DrawLineLowLevel (Line, False);
  Erasing--;
}

/* ---------------------------------------------------------------------------
 * erases an arc on a layer
 */
void
EraseArc (ArcTypePtr Arc)
{
  Erasing++;
  XSetForeground (Dpy, Output.fgGC, Settings.bgColor);
  DrawArcLowLevel (Arc);
  Erasing--;
}

/* ---------------------------------------------------------------------------
 * erases a text on a layer
 */
void
EraseText (TextTypePtr Text)
{
  Erasing++;
  XSetForeground (Dpy, Output.fgGC, Settings.bgColor);
  DrawTextLowLevel (Text);
  Erasing--;
}

/* ---------------------------------------------------------------------------
 * erases a polygon on a layer
 */
void
ErasePolygon (PolygonTypePtr Polygon)
{
  Erasing++;
  XSetForeground (Dpy, Output.fgGC, Settings.bgColor);
  DrawPolygonLowLevel (Polygon);
  Erasing--;
}

/* ---------------------------------------------------------------------------
 * erases an element
 */
void
EraseElement (ElementTypePtr Element)
{
  Erasing++;
  /* set color and draw lines, arcs, text and pins */
  XSetForeground (Dpy, Output.fgGC, Settings.bgColor);
  ELEMENTLINE_LOOP (Element, 
    {
      DrawLineLowLevel (line, False);
    }
  );
  ARC_LOOP (Element, 
    {
      DrawArcLowLevel (arc);
    }
  );
  if (!TEST_FLAG (HIDENAMEFLAG, Element))
    DrawTextLowLevel (&ELEMENT_TEXT (PCB, Element));
  EraseElementPinsAndPads (Element);
  Erasing--;
}

/* ---------------------------------------------------------------------------
 * erases all pins and pads of an element
 */
void
EraseElementPinsAndPads (ElementTypePtr Element)
{
  Erasing++;
  XSetForeground (Dpy, Output.fgGC, Settings.bgColor);
  PIN_LOOP (Element, 
    {
      if (TEST_FLAG (ALLPIPFLAGS, pin))
	{
	  ClearPin (pin, NO_TYPE, 0);
	  XSetForeground (Dpy, Output.fgGC, Settings.bgColor);
	}
      DrawPinOrViaLowLevel (pin, False);
      if (TEST_FLAG (DISPLAYNAMEFLAG, pin))
	DrawPinOrViaNameLowLevel (pin);
    }
  );
  PAD_LOOP (Element, 
    {
      DrawPadLowLevel (pad);
      if (TEST_FLAG (DISPLAYNAMEFLAG, pad))
	DrawPadNameLowLevel (pad);
    }
  );
  Erasing--;
}

/* ---------------------------------------------------------------------------
 * erases the name of an element
 */
void
EraseElementName (ElementTypePtr Element)
{
  if (TEST_FLAG (HIDENAMEFLAG, Element))
    return;
  Erasing++;
  XSetForeground (Dpy, Output.fgGC, Settings.bgColor);
  DrawTextLowLevel (&ELEMENT_TEXT (PCB, Element));
  Erasing--;
}

/* ---------------------------------------------------------------------------
 * draws grid points if the distance is >= MIN_GRID_DISTANCE
 */
static void
DrawGrid ()
{
  Location minx, miny, maxx, maxy, temp;
  float x, y, delta;

  delta = GetGridFactor () * PCB->Grid;
  if (TO_SCREEN ((int) delta) >= MIN_GRID_DISTANCE)
    {
      minx = TO_PCB_X (0);
      miny = TO_PCB_Y (0);
      maxx = TO_PCB_X (Output.Width);
      maxy = TO_PCB_Y (Output.Height);
      if (miny > maxy)
	{
	  temp = maxy;
	  maxy = miny;
	  miny = temp;
	}
      minx -= delta;
      miny -= delta;
#if 0
      maxx += delta;
      maxy += delta;
#else
      minx -= delta;
      miny -= delta;
#endif
      maxx = MIN ((BDimension) maxx, PCB->MaxWidth);
      maxy = MIN ((BDimension) maxy, PCB->MaxHeight);
      miny = MAX (0, miny);
      minx = MAX (0, minx);
      for (y = miny; y <= maxy; y += delta)
	for (x = minx; x <= maxx; x += delta)
	  XDrawPoint (Dpy, DrawingWindow,
		      Output.GridGC, TO_DRAW_X (GRIDFIT_X (x, delta)),
		      TO_DRAW_Y (GRIDFIT_Y (y, delta)));
    }
}

void
EraseObject (int type, void *ptr)
{
  switch (type)
    {
    case VIA_TYPE:
    case PIN_TYPE:
      ErasePin ((PinTypePtr) ptr);
      break;
    case TEXT_TYPE:
    case ELEMENTNAME_TYPE:
      EraseText ((TextTypePtr) ptr);
      break;
    case POLYGON_TYPE:
      ErasePolygon ((PolygonTypePtr) ptr);
      break;
    case ELEMENT_TYPE:
      EraseElement ((ElementTypePtr) ptr);
      break;
    case LINE_TYPE:
    case ELEMENTLINE_TYPE:
    case RATLINE_TYPE:
      EraseLine ((LineTypePtr) ptr);
      break;
    case PAD_TYPE:
      ErasePad ((PadTypePtr) ptr);
      break;
    case ARC_TYPE:
    case ELEMENTARC_TYPE:
      EraseArc ((ArcTypePtr) ptr);
      break;
    default:
      Message ("hace: Internal ERROR, trying to erase an unknown type\n");
    }
}



void
DrawObject (int type, void *ptr1, void *ptr2, int unused)
{
  switch (type)
    {
    case VIA_TYPE:
      if (PCB->ViaOn)
	DrawVia ((PinTypePtr) ptr2, 0);
      break;
    case LINE_TYPE:
      if (((LayerTypePtr) ptr1)->On)
	DrawLine ((LayerTypePtr) ptr1, (LineTypePtr) ptr2, 0);
      break;
    case ARC_TYPE:
      if (((LayerTypePtr) ptr1)->On)
	DrawArc ((LayerTypePtr) ptr1, (ArcTypePtr) ptr2, 0);
      break;
    case TEXT_TYPE:
      if (((LayerTypePtr) ptr1)->On)
	DrawText ((LayerTypePtr) ptr1, (TextTypePtr) ptr2, 0);
      break;
    case POLYGON_TYPE:
      if (((LayerTypePtr) ptr1)->On)
	DrawPolygon ((LayerTypePtr) ptr1, (PolygonTypePtr) ptr2, 0);
      break;
    case ELEMENT_TYPE:
      if (PCB->ElementOn &&
	  (FRONT ((ElementTypePtr) ptr2) || PCB->InvisibleObjectsOn))
	DrawElement ((ElementTypePtr) ptr2, 0);
      break;
    case RATLINE_TYPE:
      if (PCB->RatOn)
	DrawRat ((RatTypePtr) ptr2, 0);
      break;
    case PIN_TYPE:
      if (PCB->PinOn)
	DrawPin ((PinTypePtr) ptr2, 0);
      break;
    case PAD_TYPE:
      if (PCB->PinOn)
	DrawPad ((PadTypePtr) ptr2, 0);
      break;
    case ELEMENTNAME_TYPE:
      if (PCB->ElementOn &&
	  (FRONT ((ElementTypePtr) ptr2) || PCB->InvisibleObjectsOn))
	DrawElementName ((ElementTypePtr) ptr1, 0);
      break;
    }
}
