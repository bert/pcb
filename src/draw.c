/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2003, 2004 Thomas Nau
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


/* drawing routines
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"

/*#include "clip.h"*/
#include "compat.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "mymem.h"
#include "misc.h"
#include "rotate.h"
#include "rtree.h"
#include "search.h"
#include "select.h"
#include "print.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

#define	SMALL_SMALL_TEXT_SIZE	0
#define	SMALL_TEXT_SIZE			1
#define	NORMAL_TEXT_SIZE		2
#define	LARGE_TEXT_SIZE			3
#define	N_TEXT_SIZES			4


/* ---------------------------------------------------------------------------
 * some local types
 */
typedef struct
{
  double X, Y;
}
FloatPolyType, *FloatPolyTypePtr;

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static BoxType Block;
static Boolean Gathering = True;
static int Erasing = False;

static int doing_pinout = False;
static int doing_assy = False;
static const BoxType *clip_box = NULL;

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void Redraw (Boolean, BoxTypePtr);
static void DrawEverything (BoxTypePtr);
static void DrawTop (const BoxType *);
static int DrawLayerGroup (int, const BoxType *);
static void DrawPinOrViaLowLevel (PinTypePtr, Boolean);
static void ClearOnlyPin (PinTypePtr, Boolean);
static void DrawPlainPin (PinTypePtr, Boolean);
static void DrawPlainVia (PinTypePtr, Boolean);
static void DrawPinOrViaNameLowLevel (PinTypePtr);
static void DrawPadLowLevel (hidGC, PadTypePtr, Boolean, Boolean);
static void DrawPadNameLowLevel (PadTypePtr);
static void DrawLineLowLevel (LineTypePtr, Boolean);
static void DrawRegularText (LayerTypePtr, TextTypePtr, int);
static void DrawPolygonLowLevel (PolygonTypePtr);
static void DrawArcLowLevel (ArcTypePtr);
static void DrawElementPackageLowLevel (ElementTypePtr Element, int);
static void DrawPlainPolygon (LayerTypePtr Layer, PolygonTypePtr Polygon);
static void AddPart (void *);
static void SetPVColor (PinTypePtr, int);
static void DrawEMark (ElementTypePtr, LocationType, LocationType, Boolean);
static void ClearPad (PadTypePtr, Boolean);
static void DrawHole (PinTypePtr);
static void DrawMask (BoxType *);
static void DrawRats (BoxType *);
static void DrawSilk (int, int, BoxType *);
static int pin_callback (const BoxType * b, void *cl);
static int pad_callback (const BoxType * b, void *cl);

/*--------------------------------------------------------------------------------------
 * setup color for pin or via
 */
static void
SetPVColor (PinTypePtr Pin, int Type)
{
  char *color;

  if (Type == VIA_TYPE)
    {
      if (!doing_pinout
	  && TEST_FLAG (WARNFLAG | SELECTEDFLAG | FOUNDFLAG, Pin))
	{
	  if (TEST_FLAG (WARNFLAG, Pin))
	    color = PCB->WarnColor;
	  else if (TEST_FLAG (SELECTEDFLAG, Pin))
	    color = PCB->ViaSelectedColor;
	  else
	    color = PCB->ConnectedColor;
	}
      else
	color = PCB->ViaColor;
    }
  else
    {
      if (!doing_pinout
	  && TEST_FLAG (WARNFLAG | SELECTEDFLAG | FOUNDFLAG, Pin))
	{
	  if (TEST_FLAG (WARNFLAG, Pin))
	    color = PCB->WarnColor;
	  else if (TEST_FLAG (SELECTEDFLAG, Pin))
	    color = PCB->PinSelectedColor;
	  else
	    color = PCB->ConnectedColor;
	}
      else
	color = PCB->PinColor;
    }

  gui->set_color (Output.fgGC, color);
}

/*---------------------------------------------------------------------------
 *  Adds the update rect to the update region
 */
static void
AddPart (void *b)
{
  BoxType *box = (BoxType *) b;

  Block.X1 = MIN (Block.X1, box->X1);
  Block.X2 = MAX (Block.X2, box->X2);
  Block.Y1 = MIN (Block.Y1, box->Y1);
  Block.Y2 = MAX (Block.Y2, box->Y2);
}

/*
 * force the whole output to be updated
 */
void
UpdateAll (void)
{
  gui->invalidate_all ();
}

/*
 * initiate the actual drawing to the pixmap/screen
 * make the update block slightly larger to handle round-off
 * caused by the TO_SCREEN operation
 */
void
Draw (void)
{

  render = True;

  HideCrosshair (True);

  /* clear and create event if not drawing to a pixmap
   */
  gui->invalidate_lr (Block.X1, Block.X2, Block.Y1, Block.Y2, 1);

  RestoreCrosshair (True);

  /* shrink the update block */
  Block.X1 = Block.Y1 = Block.X2 = Block.Y2 = 0;
}

/* ---------------------------------------------------------------------------
 * redraws the output area without clearing it
 */
void
RedrawOutput (BoxTypePtr area)
{
  Redraw (True, area);
}

/* ---------------------------------------------------------------------------
 * redraws the output area after clearing it
 */
void
ClearAndRedrawOutput (void)
{
  render = True;
  Gathering = False;
  UpdateAll ();
}




/* ---------------------------------------------------------------------- 
 * redraws all the data
 * all necessary sizes are already set by the porthole widget and
 * by the event handlers
 */
static void
Redraw (Boolean ClearWindow, BoxTypePtr screen_area)
{
  gui->invalidate_all ();
  Gathering = True;
  render = False;
}

static int
backE_callback (const BoxType * b, void *cl)
{
  ElementTypePtr element = (ElementTypePtr) b;

  if (!FRONT (element))
    {
      DrawElementPackage (element, 0);
    }
  return 1;
}

static int
backN_callback (const BoxType * b, void *cl)
{
  TextTypePtr text = (TextTypePtr) b;
  ElementTypePtr element = (ElementTypePtr) text->Element;

  if (!FRONT (element) && !TEST_FLAG (HIDENAMEFLAG, element))
    DrawElementName (element, 0);
  return 0;
}

static int
backPad_callback (const BoxType * b, void *cl)
{
  PadTypePtr pad = (PadTypePtr) b;

  if (!FRONT (pad))
    DrawPad (pad, 0);
  return 1;
}

static int
frontE_callback (const BoxType * b, void *cl)
{
  ElementTypePtr element = (ElementTypePtr) b;

  if (FRONT (element))
    {
      DrawElementPackage (element, 0);
    }
  return 1;
}

static int
EMark_callback (const BoxType * b, void *cl)
{
  ElementTypePtr element = (ElementTypePtr) b;

  DrawEMark (element, element->MarkX, element->MarkY, !FRONT (element));
  return 1;
}

static int
frontN_callback (const BoxType * b, void *cl)
{
  TextTypePtr text = (TextTypePtr) b;
  ElementTypePtr element = (ElementTypePtr) text->Element;

  if (FRONT (element) && !TEST_FLAG (HIDENAMEFLAG, element))
    DrawElementName (element, 0);
  return 0;
}

static int
hole_callback (const BoxType * b, void *cl)
{
  PinTypePtr pin = (PinTypePtr) b;
  int plated = cl ? *(int *) cl : -1;
  switch (plated)
    {
    case -1:
      break;
    case 0:
      if (!TEST_FLAG (HOLEFLAG, pin))
	return 1;
      break;
    case 1:
      if (TEST_FLAG (HOLEFLAG, pin))
	return 1;
      break;
    }
  DrawHole ((PinTypePtr) b);
  return 1;
}

typedef struct
{
  int nplated;
  int nunplated;
} HoleCountStruct;

static int
hole_counting_callback (const BoxType * b, void *cl)
{
  PinTypePtr pin = (PinTypePtr) b;
  HoleCountStruct *hcs = (HoleCountStruct *) cl;
  if (TEST_FLAG (HOLEFLAG, pin))
    hcs->nunplated++;
  else
    hcs->nplated++;
  return 1;
}

static int
rat_callback (const BoxType * b, void *cl)
{
  DrawRat ((RatTypePtr) b, 0);
  return 1;
}

static int
lowvia_callback (const BoxType * b, void *cl)
{
  PinTypePtr via = (PinTypePtr) b;
  if (!via->Mask)
    DrawPlainVia (via, False);
  return 1;
}

/* ---------------------------------------------------------------------------
 * prints assembly drawing.
 */

static void
PrintAssembly (const BoxType * drawn_area, int side_group, int swap_ident)
{
  int save_swap = SWAP_IDENT;

  gui->set_draw_faded (Output.fgGC, 1);
  SWAP_IDENT = swap_ident;
  DrawLayerGroup (side_group, drawn_area);
  r_search (PCB->Data->via_tree, drawn_area, NULL, lowvia_callback, NULL);
  DrawTop (drawn_area);
  gui->set_draw_faded (Output.fgGC, 0);

  /* draw package */
  DrawSilk (swap_ident,
	    swap_ident ? SOLDER_LAYER : COMPONENT_LAYER,
	    drawn_area);
  SWAP_IDENT = save_swap;
}

/* ---------------------------------------------------------------------------
 * initializes some identifiers for a new zoom factor and redraws whole screen
 */
static void
DrawEverything (BoxTypePtr drawn_area)
{
  int i, ngroups, side;
  int plated;
  int component, solder;
  /* This is the list of layer groups we will draw.  */
  int do_group[MAX_LAYER];
  /* This is the reverse of the order in which we draw them.  */
  int drawn_groups[MAX_LAYER];

  PCB->Data->SILKLAYER.Color = PCB->ElementColor;
  PCB->Data->BACKSILKLAYER.Color = PCB->InvisibleObjectsColor;

  memset (do_group, 0, sizeof (do_group));
  for (ngroups = 0, i = 0; i < max_layer; i++)
    {
      LayerType *l = LAYER_ON_STACK (i);
      int group = GetLayerGroupNumberByNumber (LayerStack[i]);
      if (l->On && !do_group[group])
	{
	  do_group[group] = 1;
	  drawn_groups[ngroups++] = group;
	}
    }

  component = GetLayerGroupNumberByNumber (max_layer + COMPONENT_LAYER);
  solder = GetLayerGroupNumberByNumber (max_layer + SOLDER_LAYER);

  /*
   * first draw all 'invisible' stuff
   */
  if (!TEST_FLAG (CHECKPLANESFLAG, PCB)
      && gui->set_layer ("invisible", SL (INVISIBLE, 0), 0))
    {
      r_search (PCB->Data->pad_tree, drawn_area, NULL, backPad_callback,
		NULL);
      if (PCB->ElementOn)
	{
	  r_search (PCB->Data->element_tree, drawn_area, NULL, backE_callback,
		    NULL);
	  r_search (PCB->Data->name_tree[NAME_INDEX (PCB)], drawn_area, NULL,
		    backN_callback, NULL);
	  DrawLayer (&(PCB->Data->BACKSILKLAYER), drawn_area);
	}
    }

  /* draw all layers in layerstack order */
  for (i = ngroups - 1; i >= 0; i--)
    {
      int group = drawn_groups[i];

      if (gui->set_layer (0, group, 0))
	{
	  if (DrawLayerGroup (group, drawn_area) && !gui->gui)
	    {
	      int save_swap = SWAP_IDENT;

	      if (TEST_FLAG (CHECKPLANESFLAG, PCB) && gui->gui)
		continue;
	      r_search (PCB->Data->pin_tree, drawn_area, NULL, pin_callback,
			NULL);
	      r_search (PCB->Data->via_tree, drawn_area, NULL, pin_callback,
			NULL);
	      /* draw element pads */
	      if (group == component || group == solder)
		{
		  SWAP_IDENT = (group == solder);
		  r_search (PCB->Data->pad_tree, drawn_area, NULL,
			    pad_callback, NULL);
		}
	      SWAP_IDENT = save_swap;

	      if (!gui->gui)
		{
		  /* draw holes */
		  plated = -1;
		  r_search (PCB->Data->pin_tree, drawn_area, NULL, hole_callback,
			    &plated);
		  r_search (PCB->Data->via_tree, drawn_area, NULL, hole_callback,
			    &plated);
		}
	    }
	}
    }
  if (TEST_FLAG (CHECKPLANESFLAG, PCB) && gui->gui)
    return;

  /* draw vias below silk */
  if (PCB->ViaOn && gui->gui)
    r_search (PCB->Data->via_tree, drawn_area, NULL, lowvia_callback, NULL);
  /* Draw the solder mask if turned on */
  if (gui->set_layer ("componentmask", SL (MASK, TOP), 0))
    {
      int save_swap = SWAP_IDENT;
      SWAP_IDENT = 0;
      DrawMask (drawn_area);
      SWAP_IDENT = save_swap;
    }
  if (gui->set_layer ("soldermask", SL (MASK, BOTTOM), 0))
    {
      int save_swap = SWAP_IDENT;
      SWAP_IDENT = 1;
      DrawMask (drawn_area);
      SWAP_IDENT = save_swap;
    }
  /* Draw pins, pads, vias below silk */
  if (gui->gui)
    DrawTop (drawn_area);
  else
    {
      HoleCountStruct hcs;
      hcs.nplated = hcs.nunplated = 0;
      r_search (PCB->Data->pin_tree, drawn_area, NULL, hole_counting_callback,
		&hcs);
      r_search (PCB->Data->via_tree, drawn_area, NULL, hole_counting_callback,
		&hcs);
      if (hcs.nplated && gui->set_layer ("plated-drill", SL (PDRILL, 0), 0))
	{
	  plated = 1;
	  r_search (PCB->Data->pin_tree, drawn_area, NULL, hole_callback,
		    &plated);
	  r_search (PCB->Data->via_tree, drawn_area, NULL, hole_callback,
		    &plated);
	}
      if (hcs.nunplated && gui->set_layer ("unplated-drill", SL (UDRILL, 0), 0))
	{
	  plated = 0;
	  r_search (PCB->Data->pin_tree, drawn_area, NULL, hole_callback,
		    &plated);
	  r_search (PCB->Data->via_tree, drawn_area, NULL, hole_callback,
		    &plated);
	}
    }
  /* Draw top silkscreen */
  if (gui->set_layer ("topsilk", SL (SILK, TOP), 0))
    DrawSilk (0, COMPONENT_LAYER, drawn_area);
  if (gui->set_layer ("bottomsilk", SL (SILK, BOTTOM), 0))
    DrawSilk (1, SOLDER_LAYER, drawn_area);
  if (gui->gui)
    {
      /* Draw element Marks */
      if (PCB->PinOn)
	r_search (PCB->Data->element_tree, drawn_area, NULL, EMark_callback,
		  NULL);
      /* Draw rat lines on top */
      if (PCB->RatOn)
	DrawRats(drawn_area);
    }

  for (side = 0; side <= 1; side++)
    {
      int doit;
      Boolean NoData = True;
      ALLPAD_LOOP (PCB->Data);
      {
	if ((TEST_FLAG (ONSOLDERFLAG, pad) && side == SOLDER_LAYER)
	    || (!TEST_FLAG (ONSOLDERFLAG, pad) && side == COMPONENT_LAYER))
	  {
	    NoData = False;
	    break;
	  }
      }
      ENDALL_LOOP;

      if (side == SOLDER_LAYER)
	doit = gui->set_layer ("bottompaste", SL (PASTE, BOTTOM), NoData);
      else
	doit = gui->set_layer ("toppaste", SL (PASTE, TOP), NoData);
      if (doit)
	{
	  gui->set_color (Output.fgGC, PCB->ElementColor);
	  ALLPAD_LOOP (PCB->Data);
	  {
	    if ((TEST_FLAG (ONSOLDERFLAG, pad) && side == SOLDER_LAYER)
		|| (!TEST_FLAG (ONSOLDERFLAG, pad)
		    && side == COMPONENT_LAYER))
	      if (!TEST_FLAG (NOPASTEFLAG, pad))
		DrawPadLowLevel (Output.fgGC, pad, False, False);
	  }
	  ENDALL_LOOP;
	}
    }

  doing_assy = True;
  if (gui->set_layer ("topassembly", SL (ASSY, TOP), 0))
    PrintAssembly (drawn_area, component, 0);

  if (gui->set_layer ("bottomassembly", SL (ASSY, BOTTOM), 0))
    PrintAssembly (drawn_area, solder, 1);
  doing_assy = False;

  if (gui->set_layer ("fab", SL (FAB, 0), 0))
    PrintFab ();
}

static void
DrawEMark (ElementTypePtr e, LocationType X, LocationType Y,
	   Boolean invisible)
{
  int mark_size = EMARK_SIZE;
  if (!PCB->InvisibleObjectsOn && invisible)
    return;

  if (e->PinN && mark_size > e->Pin[0].Thickness / 2)
    mark_size = e->Pin[0].Thickness / 2;
  if (e->PadN && mark_size > e->Pad[0].Thickness / 2)
    mark_size = e->Pad[0].Thickness / 2;

  gui->set_color (Output.fgGC,
		  invisible ? PCB->InvisibleMarkColor : PCB->ElementColor);
  gui->set_line_cap (Output.fgGC, Trace_Cap);
  gui->set_line_width (Output.fgGC, 0);
  gui->draw_line (Output.fgGC, X - mark_size, Y, X, Y - mark_size);
  gui->draw_line (Output.fgGC, X + mark_size, Y, X, Y - mark_size);
  gui->draw_line (Output.fgGC, X - mark_size, Y, X, Y + mark_size);
  gui->draw_line (Output.fgGC, X + mark_size, Y, X, Y + mark_size);

  /*
   * If an element is locked, place a "L" on top of the "diamond".
   * This provides a nice visual indication that it is locked that
   * works even for color blind users.
   */
  if (TEST_FLAG (LOCKFLAG, e) )
    {
      gui->draw_line (Output.fgGC, X, Y, X + 2 * mark_size, Y);
      gui->draw_line (Output.fgGC, X, Y, X, Y - 4* mark_size);
    }
  
}

static int
via_callback (const BoxType * b, void *cl)
{
  PinTypePtr via = (PinTypePtr) b;
  if (via->Mask)
    DrawPlainVia (via, False);
  return 1;
}

static int
pin_callback (const BoxType * b, void *cl)
{
  DrawPlainPin ((PinTypePtr) b, False);
  return 1;
}

static int
pad_callback (const BoxType * b, void *cl)
{
  PadTypePtr pad = (PadTypePtr) b;
  if (FRONT (pad))
    DrawPad (pad, 0);
  return 1;
}

/* ---------------------------------------------------------------------------
 * draws pins pads and vias
 */
static void
DrawTop (const BoxType * screen)
{
  if (PCB->PinOn || doing_assy)
    {
      /* draw element pins */
      r_search (PCB->Data->pin_tree, screen, NULL, pin_callback, NULL);
      /* draw element pads */
      r_search (PCB->Data->pad_tree, screen, NULL, pad_callback, NULL);
    }
  /* draw vias */
  if (PCB->ViaOn || doing_assy)
    {
      r_search (PCB->Data->via_tree, screen, NULL, via_callback, NULL);
      r_search (PCB->Data->via_tree, screen, NULL, hole_callback, NULL);
    }
  if (PCB->PinOn || doing_assy)
    r_search (PCB->Data->pin_tree, screen, NULL, hole_callback, NULL);
}

struct pin_info
{
  Boolean arg;
  LayerTypePtr Layer;
};

static int
clearPin_callback (const BoxType * b, void *cl)
{
  PinTypePtr pin = (PinTypePtr) b;
  struct pin_info *i = (struct pin_info *) cl;
  if (i->arg)
    ClearOnlyPin (pin, True);
  return 1;
}
static int
poly_callback (const BoxType * b, void *cl)
{
  struct pin_info *i = (struct pin_info *) cl;

  DrawPlainPolygon (i->Layer, (PolygonTypePtr) b);
  return 1;
}

static int
clearPad_callback (const BoxType * b, void *cl)
{
  PadTypePtr pad = (PadTypePtr) b;
  if (!XOR (TEST_FLAG (ONSOLDERFLAG, pad), SWAP_IDENT))
    ClearPad (pad, True);
  return 1;
}

/* ---------------------------------------------------------------------------
 * Draws silk layer.
 */

static void
DrawSilk (int new_swap, int layer, BoxTypePtr drawn_area)
{
#if 0
  /* This code is used when you want to mask silk to avoid exposed
     pins and pads.  We decided it was a bad idea to do this
     unconditionally, but the code remains.  */
  struct pin_info info;
#endif
  int save_swap = SWAP_IDENT;
  SWAP_IDENT = new_swap;

#if 0
  if (gui->poly_before)
    {
      gui->use_mask (HID_MASK_BEFORE);
#endif
      DrawLayer (LAYER_PTR (max_layer + layer), drawn_area);
      /* draw package */
      r_search (PCB->Data->element_tree, drawn_area, NULL, frontE_callback,
		NULL);
      r_search (PCB->Data->name_tree[NAME_INDEX (PCB)], drawn_area, NULL,
		frontN_callback, NULL);
#if 0
    }

  gui->use_mask (HID_MASK_CLEAR);
  info.arg = True;
  r_search (PCB->Data->pin_tree, drawn_area, NULL, clearPin_callback, &info);
  r_search (PCB->Data->via_tree, drawn_area, NULL, clearPin_callback, &info);
  r_search (PCB->Data->pad_tree, drawn_area, NULL, clearPad_callback, &info);

  if (gui->poly_after)
    {
      gui->use_mask (HID_MASK_AFTER);
      DrawLayer (LAYER_PTR (max_layer + layer), drawn_area);
      /* draw package */
      r_search (PCB->Data->element_tree, drawn_area, NULL, frontE_callback,
		NULL);
      r_search (PCB->Data->name_tree[NAME_INDEX (PCB)], drawn_area, NULL,
		frontN_callback, NULL);
    }
  gui->use_mask (HID_MASK_OFF);
#endif
  SWAP_IDENT = save_swap;
}

/* ---------------------------------------------------------------------------
 * draws solder mask layer - this will cover nearly everything
 */
static void
DrawMask (BoxType * screen)
{
  struct pin_info info;
  int thin = TEST_FLAG(THINDRAWFLAG, PCB) || TEST_FLAG(THINDRAWPOLYFLAG, PCB);

  OutputType *out = &Output;

  info.arg = True;

  if (thin)
    gui->set_color (Output.pmGC, PCB->MaskColor);
  else
    {
      if (gui->poly_before)
	{
	  gui->use_mask (HID_MASK_BEFORE);
	  gui->set_color (out->fgGC, PCB->MaskColor);
	  gui->fill_rect (out->fgGC, 0, 0, PCB->MaxWidth, PCB->MaxHeight);
	}
      gui->use_mask (HID_MASK_CLEAR);
    }

  r_search (PCB->Data->pin_tree, screen, NULL, clearPin_callback, &info);
  r_search (PCB->Data->via_tree, screen, NULL, clearPin_callback, &info);
  r_search (PCB->Data->pad_tree, screen, NULL, clearPad_callback, &info);

  if (thin)
    gui->set_color (Output.pmGC, "erase");
  else
    {
      if (gui->poly_after)
	{
	  gui->use_mask (HID_MASK_AFTER);
	  gui->set_color (out->fgGC, PCB->MaskColor);
	  gui->fill_rect (out->fgGC, 0, 0, PCB->MaxWidth, PCB->MaxHeight);
	}
      gui->use_mask (HID_MASK_OFF);
    }

#if 0
  /* Some fabs want the board outline on the solder mask layer.  If
     you need this, change the '0' above to '1', and the code below
     will copy the outline layer to the mask layers.  */
  if (!gui->gui)
    {
      int i;
      for (i=PCB->Data->LayerN; i>=0; i--)
	{
	  LayerTypePtr Layer = PCB->Data->Layer + i;
	  if (strcasecmp (Layer->Name, "outline") == 0)
	    DrawLayer (Layer, screen);
	}
    }
#endif
}

static void
DrawRats (BoxTypePtr drawn_area)
{
  /*
   * XXX lesstif allows positive AND negative drawing in HID_MASK_CLEAR.
   * XXX gtk only allows negative drawing.
   * XXX using the mask here is to get rat transparency
   */
  int can_mask = strcmp(gui->name, "lesstif") == 0;

  if (can_mask)
    gui->use_mask (HID_MASK_CLEAR);
  r_search (PCB->Data->rat_tree, drawn_area, NULL, rat_callback, NULL);
  if (can_mask)
    gui->use_mask (HID_MASK_OFF);
}

static int
line_callback (const BoxType * b, void *cl)
{
  DrawLine ((LayerTypePtr) cl, (LineTypePtr) b, 0);
  return 1;
}

static int
arc_callback (const BoxType * b, void *cl)
{
  DrawArc ((LayerTypePtr) cl, (ArcTypePtr) b, 0);
  return 1;
}

static int
text_callback (const BoxType * b, void *cl)
{
  DrawRegularText ((LayerTypePtr) cl, (TextTypePtr) b, 0);
  return 1;
}


/* ---------------------------------------------------------------------------
 * draws one non-copper layer
 */
void
DrawLayer (LayerTypePtr Layer, BoxType * screen)
{
  struct pin_info info;

  /* print the non-clearing polys */
  info.Layer = Layer;
  info.arg = False;
  clip_box = screen;
  r_search (Layer->polygon_tree, screen, NULL, poly_callback, &info);

  /* draw all visible lines this layer */
  r_search (Layer->line_tree, screen, NULL, line_callback, Layer);

  /* draw the layer arcs on screen */
  r_search (Layer->arc_tree, screen, NULL, arc_callback, Layer);

  /* draw the layer text on screen */
  r_search (Layer->text_tree, screen, NULL, text_callback, Layer);
  clip_box = NULL;
}

/* ---------------------------------------------------------------------------
 * draws one layer group.  Returns non-zero if pins and pads should be
 * drawn with this group.
 */
static int
DrawLayerGroup (int group, const BoxType * screen)
{
  int i, rv = 1;
  int layernum;
  struct pin_info info;
  LayerTypePtr Layer;
  int n_entries = PCB->LayerGroups.Number[group];
  Cardinal *layers = PCB->LayerGroups.Entries[group];

  clip_box = screen;
  for (i = n_entries - 1; i >= 0; i--)
    {
      layernum = layers[i];
      Layer = PCB->Data->Layer + layers[i];
      if (strcasecmp (Layer->Name, "outline") == 0
	  || strcasecmp (Layer->Name, "route") == 0)
	rv = 0;
      if (layernum < max_layer && Layer->On)
	{
	  /* draw all polygons on this layer */
	  if (Layer->PolygonN)
	    {
	      info.Layer = Layer;
	      info.arg = True;
	      r_search (Layer->polygon_tree, screen, NULL, poly_callback,
			&info);
	      info.arg = False;
	    }

	  if (TEST_FLAG (CHECKPLANESFLAG, PCB))
	    continue;

	  /* draw all visible lines this layer */
	  r_search (Layer->line_tree, screen, NULL, line_callback, Layer);

	  /* draw the layer arcs on screen */
	  r_search (Layer->arc_tree, screen, NULL, arc_callback, Layer);

	  /* draw the layer text on screen */
	  r_search (Layer->text_tree, screen, NULL, text_callback, Layer);

	}
    }
  if (n_entries > 1)
    rv = 1;
  return rv;
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
  */
static void
DrawSpecialPolygon (HID * hid, hidGC DrawGC,
		    LocationType X, LocationType Y, int Thickness)
{
  static FloatPolyType p[8] = {
    {
     0.5, -TAN_22_5_DEGREE_2},
    {
     TAN_22_5_DEGREE_2, -0.5},
    {
     -TAN_22_5_DEGREE_2, -0.5},
    {
     -0.5, -TAN_22_5_DEGREE_2},
    {
     -0.5, TAN_22_5_DEGREE_2},
    {
     -TAN_22_5_DEGREE_2, 0.5},
    {
     TAN_22_5_DEGREE_2, 0.5},
    {
     0.5, TAN_22_5_DEGREE_2}
  };
  static int special_size = 0;
  static int scaled_x[8];
  static int scaled_y[8];
  int polygon_x[9];
  int polygon_y[9];
  int i;


  if (Thickness != special_size)
    {
      special_size = Thickness;
      for (i = 0; i < 8; i++)
	{
	  scaled_x[i] = p[i].X * special_size;
	  scaled_y[i] = p[i].Y * special_size;
	}
    }
  /* add line offset */
  for (i = 0; i < 8; i++)
    {
      polygon_x[i] = X + scaled_x[i];
      polygon_y[i] = Y + scaled_y[i];
    }
  if (TEST_FLAG (THINDRAWFLAG, PCB))
    {
      int i;
      hid->set_line_cap (Output.fgGC, Round_Cap);
      hid->set_line_width (Output.fgGC, 0);
      polygon_x[8] = X + scaled_x[0];
      polygon_y[8] = Y + scaled_y[0];
      for (i = 0; i < 8; i++)
	hid->draw_line (DrawGC, polygon_x[i], polygon_y[i],
			polygon_x[i + 1], polygon_y[i + 1]);
    }
  else
    hid->fill_polygon (DrawGC, 8, polygon_x, polygon_y);
}

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for pins and vias
 */
static void
DrawPinOrViaLowLevel (PinTypePtr Ptr, Boolean drawHole)
{
  if (Gathering)
    {
      AddPart (Ptr);
      return;
    }

  if (TEST_FLAG (HOLEFLAG, Ptr))
    {
      if (drawHole)
	{
	  gui->fill_circle (Output.bgGC, Ptr->X, Ptr->Y, Ptr->Thickness / 2);
	  gui->set_line_cap (Output.fgGC, Round_Cap);
	  gui->set_line_width (Output.fgGC, 0);
	  gui->draw_arc (Output.fgGC, Ptr->X, Ptr->Y,
			 Ptr->Thickness / 2, Ptr->Thickness / 2, 0, 360);
	}
      return;
    }
  if (TEST_FLAG (SQUAREFLAG, Ptr))
    {
      int l, r, t, b;
      l = Ptr->X - Ptr->Thickness / 2;
      b = Ptr->Y - Ptr->Thickness / 2;
      r = l + Ptr->Thickness;
      t = b + Ptr->Thickness;
      if (TEST_FLAG (THINDRAWFLAG, PCB))
        {
          gui->set_line_cap (Output.fgGC, Round_Cap);
          gui->set_line_width (Output.fgGC, 0);
          gui->draw_line (Output.fgGC, r, t, r, b);
          gui->draw_line (Output.fgGC, l, t, l, b);
          gui->draw_line (Output.fgGC, r, t, l, t);
          gui->draw_line (Output.fgGC, r, b, l, b);
        }
      else
        {
          gui->fill_rect (Output.fgGC, l, b, r, t);
        }
    }
  else if (TEST_FLAG (OCTAGONFLAG, Ptr))
    {
      gui->set_line_cap (Output.fgGC, Round_Cap);
      gui->set_line_width (Output.fgGC,
			   (Ptr->Thickness - Ptr->DrillingHole) / 2);

      /* transform X11 specific coord system */
      DrawSpecialPolygon (gui, Output.fgGC, Ptr->X, Ptr->Y, Ptr->Thickness);
    }
  else
    {				/* draw a round pin or via */
      if (TEST_FLAG (THINDRAWFLAG, PCB))
	{
	  gui->set_line_cap (Output.fgGC, Round_Cap);
	  gui->set_line_width (Output.fgGC, 0);
	  gui->draw_arc (Output.fgGC, Ptr->X, Ptr->Y,
			 Ptr->Thickness / 2, Ptr->Thickness / 2, 0, 360);
	}
      else
	{
	  gui->fill_circle (Output.fgGC, Ptr->X, Ptr->Y, Ptr->Thickness / 2);
	}
    }

  /* and the drilling hole  (which is always round */
  if (drawHole)
    {
      if (TEST_FLAG (THINDRAWFLAG, PCB))
	{
	  gui->set_line_cap (Output.fgGC, Round_Cap);
	  gui->set_line_width (Output.fgGC, 0);
	  gui->draw_arc (Output.fgGC,
			 Ptr->X, Ptr->Y, Ptr->DrillingHole / 2,
			 Ptr->DrillingHole / 2, 0, 360);
	}
      else
	{
	  gui->fill_circle (Output.bgGC, Ptr->X, Ptr->Y,
			    Ptr->DrillingHole / 2);
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
	  gui->set_line_cap (Output.fgGC, Round_Cap);
	  gui->set_line_width (Output.fgGC, 0);
	  gui->draw_arc (Output.fgGC,
			 Ptr->X, Ptr->Y, Ptr->DrillingHole / 2,
			 Ptr->DrillingHole / 2, 0, 360);
	}
    }
  else
    {
      gui->fill_circle (Output.bgGC, Ptr->X, Ptr->Y, Ptr->DrillingHole / 2);
    }
  if (TEST_FLAG (HOLEFLAG, Ptr))
    {
      if (TEST_FLAG (WARNFLAG, Ptr))
	gui->set_color (Output.fgGC, PCB->WarnColor);
      else if (TEST_FLAG (SELECTEDFLAG, Ptr))
	gui->set_color (Output.fgGC, PCB->ViaSelectedColor);
      else
	gui->set_color (Output.fgGC, Settings.BlackColor);

      gui->set_line_cap (Output.fgGC, Round_Cap);
      gui->set_line_width (Output.fgGC, 0);
      gui->draw_arc (Output.fgGC,
		     Ptr->X, Ptr->Y, Ptr->DrillingHole / 2,
		     Ptr->DrillingHole / 2, 0, 360);
    }
}

/*******************************************************************
 * draw clearance in pixmask around pins and vias that pierce polygons
 */
static void
ClearOnlyPin (PinTypePtr Pin, Boolean mask)
{
  BDimension half =
    (mask ? Pin->Mask / 2 : (Pin->Thickness + Pin->Clearance) / 2);

  if (!mask && TEST_FLAG (HOLEFLAG, Pin))
    return;
  if (half == 0)
    return;
  if (!mask && Pin->Clearance <= 0)
    return;

  /* Clear the area around the pin */
  if (TEST_FLAG (SQUAREFLAG, Pin))
    {
      int l, r, t, b;
      l = Pin->X - half;
      b = Pin->Y - half;
      r = l + half * 2;
      t = b + half * 2;
      if (TEST_FLAG (THINDRAWFLAG, PCB) || TEST_FLAG (THINDRAWPOLYFLAG, PCB))
        {
          gui->set_line_cap (Output.pmGC, Round_Cap);
          gui->set_line_width (Output.pmGC, 0);
          gui->draw_line (Output.pmGC, r, t, r, b);
          gui->draw_line (Output.pmGC, l, t, l, b);
          gui->draw_line (Output.pmGC, r, t, l, t);
          gui->draw_line (Output.pmGC, r, b, l, b);
        }
      else
	gui->fill_rect (Output.pmGC, l, b, r, t);
    }
  else if (TEST_FLAG (OCTAGONFLAG, Pin))
    {
      gui->set_line_cap (Output.pmGC, Round_Cap);
      gui->set_line_width (Output.pmGC, (Pin->Clearance + Pin->Thickness
					 - Pin->DrillingHole));

      DrawSpecialPolygon (gui, Output.pmGC, Pin->X, Pin->Y, half * 2);
    }
  else
    {
      if (TEST_FLAG (THINDRAWFLAG, PCB) || TEST_FLAG (THINDRAWPOLYFLAG, PCB))
	gui->draw_arc (Output.pmGC, Pin->X, Pin->Y, half, half, 0, 360);
      else
	gui->fill_circle (Output.pmGC, Pin->X, Pin->Y, half);
    }
}

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for pins and vias that pierce polygons
 */
void
ClearPin (PinTypePtr Pin, int Type, int unused)
{
  BDimension half = (Pin->Thickness + Pin->Clearance) / 2;

  if (Gathering)
    {
      AddPart (Pin);
      return;
    }
  /* Clear the area around the pin */
  if (TEST_FLAG (SQUAREFLAG, Pin))
    {
      int l, r, t, b;
      l = Pin->X - half;
      b = Pin->Y - half;
      r = l + half * 2;
      t = b + half * 2;
      gui->fill_rect (Output.pmGC, l, b, r, t);
    }
  else if (TEST_FLAG (OCTAGONFLAG, Pin))
    {
      gui->set_line_cap (Output.pmGC, Round_Cap);
      gui->set_line_width (Output.pmGC, (Pin->Clearance + Pin->Thickness
					 - Pin->DrillingHole) / 2);

      DrawSpecialPolygon (gui, Output.pmGC, Pin->X, Pin->Y, half * 2);
    }
  else
    {
      gui->fill_circle (Output.pmGC, Pin->X, Pin->Y, half);
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


#if VERTICAL_TEXT
/* vertical text handling provided by Martin Devera with fixes by harry eaton */

/* draw vertical text; xywh is bounding, de is text's descend used for
   positioning */
static void
DrawVText (int x, int y, int w, int h, char *str)
{
  GdkPixmap *pm;
  GdkImage *im;
  GdkGCValues values;
  guint32 pixel;
  int i, j;

  if (!str || !*str)
    return;

  pm = gdk_pixmap_new (DrawingWindow, w, h, -1);

  /* draw into pixmap */
  gdk_draw_rectangle (pm, Output.bgGC, TRUE, 0, 0, w, h);

  gui_draw_string_markup (DrawingWindow, Output.font_desc, Output.fgGC,
			  0, 0, str);

  im = gdk_drawable_get_image (pm, 0, 0, w, h);
  gdk_gc_get_values (Output.fgGC, &values);

  /* draw Transpose(im).  TODO: Pango should be doing vertical text soon */
  for (i = 0; i < w; i++)
    for (j = 0; j < h; j++)
      {
	pixel = gdk_image_get_pixel (im, i, j);
	if (pixel == values.foreground.pixel)
	  gdk_draw_point (DrawingWindow, Output.fgGC, x + j, y + w - i - 1);
      }
  g_object_unref (G_OBJECT (pm));
}
#endif

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for pin and via names
 */
static void
DrawPinOrViaNameLowLevel (PinTypePtr Ptr)
{
  char *name;
  BoxType box;
  Boolean vert;
  TextType text;

  if (!Ptr->Name || !Ptr->Name[0])
    name = EMPTY (Ptr->Number);
  else
    name = EMPTY (TEST_FLAG (SHOWNUMBERFLAG, PCB) ? Ptr->Number : Ptr->Name);

  vert = TEST_FLAG (EDGE2FLAG, Ptr);

  if (vert)
    {
      box.X1 = Ptr->X - Ptr->Thickness / 2 + Settings.PinoutTextOffsetY;
      box.Y1 = Ptr->Y - Ptr->DrillingHole / 2 - Settings.PinoutTextOffsetX;
    }
  else
    {
      box.X1 = Ptr->X + Ptr->DrillingHole / 2 + Settings.PinoutTextOffsetX;
      box.Y1 = Ptr->Y - Ptr->Thickness / 2 + Settings.PinoutTextOffsetY;
    }
  if (Gathering)
    {
      if (vert)
	{
	  box.X2 = box.X1;
	  box.Y2 = box.Y1;
	}
      else
	{
	  box.X2 = box.X1;
	  box.Y2 = box.Y1;
	}
/*printf("AddPart: x1=%d y1=%d x2=%d y2=%d\n", box.X1, box.Y1, box.X2, box.Y2);*/
      AddPart (&box);
      return;
    }
/*printf("DrawPin(%d,%d): x=%d y=%d w=%d h=%d\n",
  TO_DRAW_X(Ptr->X), TO_DRAW_Y(Ptr->Y), box.X1, box.Y1, width, height);*/

  gui->set_color (Output.fgGC, PCB->PinNameColor);

  text.Flags = NoFlags ();
  text.Scale = Ptr->Thickness / 80;
  text.X = box.X1;
  text.Y = box.Y1;
  text.Direction = vert ? 1 : 0;
  text.TextString = name;

  if (gui->gui)
    doing_pinout++;
  DrawTextLowLevel (&text, 0);
  if (gui->gui)
    doing_pinout--;
}

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for pads
 */

static void
DrawPadLowLevel (hidGC gc, PadTypePtr Pad, Boolean clear, Boolean mask)
{
  int w = clear ? (mask ? Pad->Mask : Pad->Thickness + Pad->Clearance)
		: Pad->Thickness;

  if (Gathering)
    {
      AddPart (Pad);
      return;
    }

  if (TEST_FLAG (THINDRAWFLAG, PCB) ||
      (clear && TEST_FLAG (THINDRAWPOLYFLAG, PCB)))
    {
      int x1, y1, x2, y2, t, t2;
      t = w / 2;
      t2 = w - t;
      x1 = Pad->Point1.X;
      y1 = Pad->Point1.Y;
      x2 = Pad->Point2.X;
      y2 = Pad->Point2.Y;
      if (x1 > x2 || y1 > y2)
	{
	  /* this is a silly way to swap the variables */
	  x1 ^= x2;
	  x2 ^= x1;
	  x1 ^= x2;
	  y1 ^= y2;
	  y2 ^= y1;
	  y1 ^= y2;
	}
      gui->set_line_cap (gc, Round_Cap);
      gui->set_line_width (gc, 0);
      if (TEST_FLAG (SQUAREFLAG, Pad)
	  && (x1 == x2 || y1 == y2))
	{
	  x1 -= t;
	  y1 -= t;
	  x2 += t2;
	  y2 += t2;
	  gui->draw_line (gc, x1, y1, x1, y2);
	  gui->draw_line (gc, x1, y2, x2, y2);
	  gui->draw_line (gc, x2, y2, x2, y1);
	  gui->draw_line (gc, x2, y1, x1, y1);
	}
      else if (TEST_FLAG (SQUAREFLAG, Pad))
	{
	  /* slanted square pad */
	  float tx, ty, theta;

	  theta = atan2 (y2-y1, x2-x1);

	  /* T is a vector half a thickness long, in the direction of
	     one of the corners.  */
	  tx = t * cos (theta + M_PI/4) * sqrt(2.0);
	  ty = t * sin (theta + M_PI/4) * sqrt(2.0);

	  gui->draw_line (gc, x1-tx, y1-ty, x2+ty, y2-tx);
	  gui->draw_line (gc, x2+ty, y2-tx, x2+tx, y2+ty);
	  gui->draw_line (gc, x2+tx, y2+ty, x1-ty, y1+tx);
	  gui->draw_line (gc, x1-ty, y1+tx, x1-tx, y1-ty);
	}
      else if (x1 == x2 && y1 == y2)
	{
	  gui->draw_arc (gc, x1, y1, w / 2, w / 2, 0, 360);
	}
      else if (x1 == x2)
	{
	  gui->draw_line (gc, x1 - t, y1, x2 - t, y2);
	  gui->draw_line (gc, x1 + t2, y1, x2 + t2, y2);
	  gui->draw_arc (gc, x1, y1, w / 2, w / 2, 0, -180);
	  gui->draw_arc (gc, x2, y2, w / 2, w / 2, 180, -180);
	}
      else if (y1 == y2)
	{
	  gui->draw_line (gc, x1, y1 - t, x2, y2 - t);
	  gui->draw_line (gc, x1, y1 + t2, x2, y2 + t2);
	  gui->draw_arc (gc, x1, y1, w / 2, w / 2, 90, -180);
	  gui->draw_arc (gc, x2, y2, w / 2, w / 2, 270, -180);
	}
      else
	{
	  /* Slanted round-end pads.  */
	  LocationType dx, dy, ox, oy;
	  float h;

	  dx = x2 - x1;
	  dy = y2 - y1;
	  h = t / sqrt (SQUARE (dx) + SQUARE (dy));
	  ox = dy * h + 0.5 * SGN (dy);
	  oy = -(dx * h + 0.5 * SGN (dx));

	  gui->draw_line (gc, x1 + ox, y1 + oy, x2 + ox, y2 + oy);

	  if (abs (ox) >= pixel_slop || abs (oy) >= pixel_slop)
	    {
	      LocationType angle = atan2 ((float) dx, (float) dy) * 57.295779;
	      gui->draw_line (gc, x1 - ox, y1 - oy, x2 - ox, y2 - oy);
	      gui->draw_arc (gc,
			     x1, y1, t, t, angle - 180, 180);
	      gui->draw_arc (gc, x2, y2, t, t, angle, 180);
	    }
	}
    }
  else if (Pad->Point1.X == Pad->Point2.X
           && Pad->Point1.Y == Pad->Point2.Y)
    {
      if (TEST_FLAG (SQUAREFLAG, Pad))
        {
          int l, r, t, b;
          l = Pad->Point1.X - w / 2;
          b = Pad->Point1.Y - w / 2;
          r = l + w;
          t = b + w;
          gui->fill_rect (gc, l, b, r, t);
        }
      else
        {
          gui->fill_circle (gc, Pad->Point1.X, Pad->Point1.Y, w / 2);
        }
    }
  else
    {
      gui->set_line_cap (gc,
                         TEST_FLAG (SQUAREFLAG,
                                    Pad) ? Square_Cap : Round_Cap);
      gui->set_line_width (gc, w);

      gui->draw_line (gc,
                      Pad->Point1.X, Pad->Point1.Y,
                      Pad->Point2.X, Pad->Point2.Y);
    }
#if 0
  { /* Draw bounding box for test */
    BoxType *box = &Pad->BoundingBox;
    gui->set_line_width (gc, 0);
    gui->draw_line (gc, box->X1, box->Y1, box->X1, box->Y2);
    gui->draw_line (gc, box->X1, box->Y2, box->X2, box->Y2);
    gui->draw_line (gc, box->X2, box->Y2, box->X2, box->Y1);
    gui->draw_line (gc, box->X2, box->Y1, box->X1, box->Y1);
  }
#endif
}

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for pad names
 */

static void
DrawPadNameLowLevel (PadTypePtr Pad)
{
  BoxType box;
  char *name;
  Boolean vert;
  TextType text;

  if (!Pad->Name || !Pad->Name[0])
    name = EMPTY (Pad->Number);
  else
    name = EMPTY (TEST_FLAG (SHOWNUMBERFLAG, PCB) ? Pad->Number : Pad->Name);

  /* should text be vertical ? */
  vert = (Pad->Point1.X == Pad->Point2.X);

  if (vert)
    {
      box.X1 = Pad->Point1.X - Pad->Thickness / 2;
      box.Y1 = MAX (Pad->Point1.Y, Pad->Point2.Y) + Pad->Thickness / 2;
    }
  else
    {
      box.X1 = MIN (Pad->Point1.X, Pad->Point2.X) - Pad->Thickness / 2;
      box.Y1 = Pad->Point1.Y - Pad->Thickness / 2;
    }

  if (vert)
    {
      box.X1 += Settings.PinoutTextOffsetY;
      box.Y1 -= Settings.PinoutTextOffsetX;
    }
  else
    {
      box.X1 += Settings.PinoutTextOffsetX;
      box.Y1 += Settings.PinoutTextOffsetY;
    }

  if (Gathering)
    {
      if (vert)
	{
	  box.X2 = box.X1;
	  box.Y2 = box.Y1;
	}
      else
	{
	  box.X2 = box.X1;
	  box.Y2 = box.Y1;
	}
      AddPart (&box);
      return;
    }

  gui->set_color (Output.fgGC, PCB->PinNameColor);

  text.Flags = NoFlags ();
  text.Scale = Pad->Thickness / 50;
  text.X = box.X1;
  text.Y = box.Y1;
  text.Direction = vert ? 1 : 0;
  text.TextString = name;

  DrawTextLowLevel (&text, 0);

}

/* ---------------------------------------------------------------------------
 * clearance for pads
 */
static void
ClearPad (PadTypePtr Pad, Boolean mask)
{
  DrawPadLowLevel(Output.pmGC, Pad, True, mask);
}

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for lines
 */
static void
DrawLineLowLevel (LineTypePtr Line, Boolean HaveGathered)
{
  if (Gathering && !HaveGathered)
    {
      AddPart (Line);
      return;
    }

  gui->set_line_cap (Output.fgGC, Trace_Cap);
  if (TEST_FLAG (THINDRAWFLAG, PCB))
    gui->set_line_width (Output.fgGC, 0);
  else
    gui->set_line_width (Output.fgGC, Line->Thickness);

  gui->draw_line (Output.fgGC,
		  Line->Point1.X, Line->Point1.Y,
		  Line->Point2.X, Line->Point2.Y);
}

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for text objects
 */
void
DrawTextLowLevel (TextTypePtr Text, int min_line_width)
{
  LocationType x = 0;
  unsigned char *string = (unsigned char *) Text->TextString;
  Cardinal n;
  FontTypePtr font = &PCB->Font;

  if (Gathering)
    {
      AddPart (Text);
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
	      if (newline.Thickness < min_line_width)
		newline.Thickness = min_line_width;

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
	  LocationType size = (defaultsymbol.X2 - defaultsymbol.X1) * 6 / 5;

	  defaultsymbol.X1 = (defaultsymbol.X1 + x) * Text->Scale / 100;
	  defaultsymbol.Y1 = defaultsymbol.Y1 * Text->Scale / 100;
	  defaultsymbol.X2 = (defaultsymbol.X2 + x) * Text->Scale / 100;
	  defaultsymbol.Y2 = defaultsymbol.Y2 * Text->Scale / 100;

	  RotateBoxLowLevel (&defaultsymbol, 0, 0, Text->Direction);

	  /* add offset and draw box */
	  defaultsymbol.X1 += Text->X;
	  defaultsymbol.Y1 += Text->Y;
	  defaultsymbol.X2 += Text->X;
	  defaultsymbol.Y2 += Text->Y;
	  gui->fill_rect (Output.fgGC,
			  defaultsymbol.X1, defaultsymbol.Y1,
			  defaultsymbol.X2, defaultsymbol.Y2);

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
  if (!Polygon->Clipped)
    return;

  if (Gathering)
    {
      AddPart (Polygon);
      return;
    }

  printf ("DrawPolygonLowLevel: Called without Gathering set!\n");
}

/* ---------------------------------------------------------------------------
 * lowlevel routine to element arcs
 */
static void
DrawArcLowLevel (ArcTypePtr Arc)
{
  if (!Arc->Thickness)
    return;
  if (Gathering)
    {
      AddPart (Arc);
      return;
    }

  if (TEST_FLAG (THINDRAWFLAG, PCB))
    gui->set_line_width (Output.fgGC, 0);
  else
    gui->set_line_width (Output.fgGC, Arc->Thickness);
  gui->set_line_cap (Output.fgGC, Trace_Cap);

  gui->draw_arc (Output.fgGC, Arc->X, Arc->Y, Arc->Width,
		 Arc->Height, Arc->StartAngle, Arc->Delta);
}

/* ---------------------------------------------------------------------------
 * draws the package of an element
 */
static void
DrawElementPackageLowLevel (ElementTypePtr Element, int unused)
{
  /* draw lines, arcs, text and pins */
  ELEMENTLINE_LOOP (Element);
  {
    DrawLineLowLevel (line, False);
  }
  END_LOOP;
  ARC_LOOP (Element);
  {
    DrawArcLowLevel (arc);
  }
  END_LOOP;
}

/* ---------------------------------------------------------------------------
 * draw a via object
 */
void
DrawVia (PinTypePtr Via, int unused)
{
  if (!Gathering)
    SetPVColor (Via, VIA_TYPE);
  //if (!doing_pinout && !TEST_FLAG (HOLEFLAG, Via) && TEST_ANY_PIPS (Via))
  // ClearPin (Via, VIA_TYPE, 0);
  //else
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
	gui->set_color (Output.fgGC, PCB->ViaSelectedColor);
      else
	gui->set_color (Output.fgGC, PCB->ViaColor);
    }
  DrawPinOrViaNameLowLevel (Via);
}

/* ---------------------------------------------------------------------------
 * draw a pin object
 */
void
DrawPin (PinTypePtr Pin, int unused)
{
  //if (!doing_pinout && !TEST_FLAG (HOLEFLAG, Pin) && TEST_ANY_PIPS (Pin))
  //  ClearPin (Pin, PIN_TYPE, 0);
  //else
  {
    if (!Gathering)
      SetPVColor (Pin, PIN_TYPE);
    DrawPinOrViaLowLevel (Pin, True);
  }
  if ((!TEST_FLAG (HOLEFLAG, Pin) && TEST_FLAG (DISPLAYNAMEFLAG, Pin))
      || doing_pinout)
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
	gui->set_color (Output.fgGC, PCB->PinSelectedColor);
      else
	gui->set_color (Output.fgGC, PCB->PinColor);
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
      if (doing_pinout)
	gui->set_color (Output.fgGC, PCB->PinColor);
      else if (TEST_FLAG (WARNFLAG | SELECTEDFLAG | FOUNDFLAG, Pad))
	{
	  if (TEST_FLAG (WARNFLAG, Pad))
	    gui->set_color (Output.fgGC, PCB->WarnColor);
	  else if (TEST_FLAG (SELECTEDFLAG, Pad))
	    gui->set_color (Output.fgGC, PCB->PinSelectedColor);
	  else
	    gui->set_color (Output.fgGC, PCB->ConnectedColor);
	}
      else if (FRONT (Pad))
	gui->set_color (Output.fgGC, PCB->PinColor);
      else
	gui->set_color (Output.fgGC, PCB->InvisibleObjectsColor);
    }
  DrawPadLowLevel (Output.fgGC, Pad, False, False);
  if (doing_pinout || TEST_FLAG (DISPLAYNAMEFLAG, Pad))
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
	gui->set_color (Output.fgGC, PCB->PinSelectedColor);
      else if (FRONT (Pad))
	gui->set_color (Output.fgGC, PCB->PinColor);
      else
	gui->set_color (Output.fgGC, PCB->InvisibleObjectsColor);
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
	    gui->set_color (Output.fgGC, Layer->SelectedColor);
	  else
	    gui->set_color (Output.fgGC, PCB->ConnectedColor);
	}
      else
	gui->set_color (Output.fgGC, Layer->Color);
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
	    gui->set_color (Output.fgGC, PCB->RatSelectedColor);
	  else
	    gui->set_color (Output.fgGC, PCB->ConnectedColor);
	}
      else
	gui->set_color (Output.fgGC, PCB->RatColor);
    }
  if (Settings.RatThickness < 20)
    Line->Thickness = pixel_slop * Settings.RatThickness;
  /* rats.c set VIAFLAG if this rat goes to a containing poly: draw a donut */
  if (TEST_FLAG(VIAFLAG, Line))
    {
      int w = Line->Thickness;

      if (Gathering)
	{
	  BoxType b;

	  b.X1 = Line->Point1.X - w * 2 - w / 2;
	  b.X2 = Line->Point1.X + w * 2 + w / 2;
	  b.Y1 = Line->Point1.Y - w * 2 - w / 2;
	  b.Y2 = Line->Point1.Y + w * 2 + w / 2;
	  AddPart(&b);
	}
      else
	{
	  if (TEST_FLAG (THINDRAWFLAG, PCB))
	    gui->set_line_width (Output.fgGC, 0);
	  else
	    gui->set_line_width (Output.fgGC, w);
	  gui->draw_arc (Output.fgGC, Line->Point1.X, Line->Point1.Y,
			 w * 2, w * 2, 0, 360);
	}
    }
  else
    DrawLineLowLevel ((LineTypePtr) Line, False);
}

/* ---------------------------------------------------------------------------
 * draws an arc on a layer
 */
void
DrawArc (LayerTypePtr Layer, ArcTypePtr Arc, int unused)
{
  if (!Arc->Thickness)
    return;
  if (!Gathering)
    {
      if (TEST_FLAG (SELECTEDFLAG | FOUNDFLAG, Arc))
	{
	  if (TEST_FLAG (SELECTEDFLAG, Arc))
	    gui->set_color (Output.fgGC, Layer->SelectedColor);
	  else
	    gui->set_color (Output.fgGC, PCB->ConnectedColor);
	}
      else
	gui->set_color (Output.fgGC, Layer->Color);
    }
  DrawArcLowLevel (Arc);
}

/* ---------------------------------------------------------------------------
 * draws a text on a layer
 */
void
DrawText (LayerTypePtr Layer, TextTypePtr Text, int unused)
{
  int min_silk_line;
  if (!Layer->On)
    return;
  if (TEST_FLAG (SELECTEDFLAG, Text))
    gui->set_color (Output.fgGC, Layer->SelectedColor);
  else
    gui->set_color (Output.fgGC, Layer->Color);
  if (Layer == & PCB->Data->SILKLAYER
      || Layer == & PCB->Data->BACKSILKLAYER)
    min_silk_line = PCB->minSlk;
  else
    min_silk_line = PCB->minWid;
  DrawTextLowLevel (Text, min_silk_line);
}

/* ---------------------------------------------------------------------------
 * draws text on a layer
 */
static void
DrawRegularText (LayerTypePtr Layer, TextTypePtr Text, int unused)
{
  int min_silk_line;
  if (TEST_FLAG (SELECTEDFLAG, Text))
    gui->set_color (Output.fgGC, Layer->SelectedColor);
  else
    gui->set_color (Output.fgGC, Layer->Color);
  if (Layer == & PCB->Data->SILKLAYER
      || Layer == & PCB->Data->BACKSILKLAYER)
    min_silk_line = PCB->minSlk;
  else
    min_silk_line = PCB->minWid;
  DrawTextLowLevel (Text, min_silk_line);
}

static int
cp_callback (const BoxType * b, void *cl)
{
  ClearPin ((PinTypePtr) b, (int) (size_t) cl, 0);
  return 1;
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
	gui->set_color (Output.fgGC, Layer->SelectedColor);
      else
	gui->set_color (Output.fgGC, PCB->ConnectedColor);
    }
  else
    gui->set_color (Output.fgGC, Layer->Color);
  layernum = GetLayerNumber (PCB->Data, Layer);
  DrawPolygonLowLevel (Polygon);
  if (TEST_FLAG (CLEARPOLYFLAG, Polygon))
    {
      r_search (PCB->Data->pin_tree, &Polygon->BoundingBox, NULL,
		cp_callback, (void *) PIN_TYPE);
      r_search (PCB->Data->via_tree, &Polygon->BoundingBox, NULL,
		cp_callback, (void *) VIA_TYPE);
    }
}

int
thin_callback (PLINE * pl, LayerTypePtr lay, PolygonTypePtr poly)
{
  int i, *x, *y;
  VNODE *v;

  i = 0;
  x = (int *) malloc (pl->Count * sizeof (int));
  y = (int *) malloc (pl->Count * sizeof (int));
  for (v = &pl->head; i < pl->Count; v = v->next)
    {
      x[i] = v->point[0];
      y[i++] = v->point[1];
    }
  gui->set_line_cap (Output.fgGC, Round_Cap);
  gui->set_line_width (Output.fgGC, 0);
  for (i = 0; i < pl->Count - 1; i++)
    {
      gui->draw_line (Output.fgGC, x[i], y[i], x[i + 1], y[i + 1]);
      //  gui->fill_circle (Output.fgGC, x[i], y[i], 30);
    }
  gui->draw_line (Output.fgGC, x[pl->Count - 1], y[pl->Count - 1], x[0],
		  y[0]);
  free (x);
  free (y);
  return 0;
}


/* ---------------------------------------------------------------------------
 * draws a polygon
 */
static void
DrawPlainPolygon (LayerTypePtr Layer, PolygonTypePtr Polygon)
{
  static char *color;

  if (!Polygon->Clipped)
    return;

  if (Gathering)
    {
      AddPart (Polygon);
      return;
    }

  if (TEST_FLAG (SELECTEDFLAG, Polygon))
    color = Layer->SelectedColor;
  else if (TEST_FLAG (FOUNDFLAG, Polygon))
    color = PCB->ConnectedColor;
  else
    color = Layer->Color;
  gui->set_color (Output.fgGC, color);

  if (gui->thindraw_pcb_polygon != NULL &&
      (TEST_FLAG (THINDRAWFLAG, PCB) ||
       TEST_FLAG (THINDRAWPOLYFLAG, PCB)))
    gui->thindraw_pcb_polygon (Output.fgGC, Polygon, clip_box);
  else
    gui->fill_pcb_polygon (Output.fgGC, Polygon, clip_box);

  /* If checking planes, thin-draw any pieces which have been clipped away */
  if (gui->thindraw_pcb_polygon != NULL &&
      TEST_FLAG (CHECKPLANESFLAG, PCB) &&
      !TEST_FLAG (FULLPOLYFLAG, Polygon))
    {
      PolygonType poly = *Polygon;

      for (poly.Clipped = Polygon->Clipped->f;
           poly.Clipped != Polygon->Clipped;
           poly.Clipped = poly.Clipped->f)
        gui->thindraw_pcb_polygon (Output.fgGC, &poly, clip_box);
    }
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
  if (gui->gui && TEST_FLAG (HIDENAMESFLAG, PCB))
    return;
  if (TEST_FLAG (HIDENAMEFLAG, Element))
    return;
  if (doing_pinout || doing_assy)
    gui->set_color (Output.fgGC, PCB->ElementColor);
  else if (TEST_FLAG (SELECTEDFLAG, &ELEMENT_TEXT (PCB, Element)))
    gui->set_color (Output.fgGC, PCB->ElementSelectedColor);
  else if (FRONT (Element))
    gui->set_color (Output.fgGC, PCB->ElementColor);
  else
    gui->set_color (Output.fgGC, PCB->InvisibleObjectsColor);
  DrawTextLowLevel (&ELEMENT_TEXT (PCB, Element), PCB->minSlk);
}

/* ---------------------------------------------------------------------------
 * draws the package of an element
 */
void
DrawElementPackage (ElementTypePtr Element, int unused)
{
  /* set color and draw lines, arcs, text and pins */
  if (doing_pinout || doing_assy)
    gui->set_color (Output.fgGC, PCB->ElementColor);
  else if (TEST_FLAG (SELECTEDFLAG, Element))
    gui->set_color (Output.fgGC, PCB->ElementSelectedColor);
  else if (FRONT (Element))
    gui->set_color (Output.fgGC, PCB->ElementColor);
  else
    gui->set_color (Output.fgGC, PCB->InvisibleObjectsColor);
  DrawElementPackageLowLevel (Element, unused);
}

/* ---------------------------------------------------------------------------
 * draw pins of an element
 */
void
DrawElementPinsAndPads (ElementTypePtr Element, int unused)
{
  PAD_LOOP (Element);
  {
    if (doing_pinout || doing_assy || FRONT (pad) || PCB->InvisibleObjectsOn)
      DrawPad (pad, unused);
  }
  END_LOOP;
  PIN_LOOP (Element);
  {
    DrawPin (pin, unused);
  }
  END_LOOP;
}

/* ---------------------------------------------------------------------------
 * erase a via
 */
void
EraseVia (PinTypePtr Via)
{
  Erasing++;
  gui->set_color (Output.fgGC, Settings.BackgroundColor);
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
  gui->set_color (Output.fgGC, Settings.BackgroundColor);
  if (TEST_FLAG(VIAFLAG, Rat))
    {
      int w = Rat->Thickness;

      if (TEST_FLAG (THINDRAWFLAG, PCB))
	gui->set_line_width (Output.fgGC, 0);
      else
	gui->set_line_width (Output.fgGC, w);
      gui->draw_arc (Output.fgGC, Rat->Point1.X, Rat->Point1.Y,
		     w * 2, w * 2, 0, 360);
    }
  else
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
  gui->set_color (Output.fgGC, Settings.BackgroundColor);
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
  gui->set_color (Output.fgGC, Settings.BackgroundColor);
  DrawPadLowLevel (Output.fgGC, Pad, False, False);
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
  gui->set_color (Output.fgGC, Settings.BackgroundColor);
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
  gui->set_color (Output.fgGC, Settings.BackgroundColor);
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
  gui->set_color (Output.fgGC, Settings.BackgroundColor);
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
  gui->set_color (Output.fgGC, Settings.BackgroundColor);
  DrawLineLowLevel (Line, False);
  Erasing--;
}

/* ---------------------------------------------------------------------------
 * erases an arc on a layer
 */
void
EraseArc (ArcTypePtr Arc)
{
  if (!Arc->Thickness)
    return;
  Erasing++;
  gui->set_color (Output.fgGC, Settings.BackgroundColor);
  DrawArcLowLevel (Arc);
  Erasing--;
}

/* ---------------------------------------------------------------------------
 * erases a text on a layer
 */
void
EraseText (LayerTypePtr Layer, TextTypePtr Text)
{
  int min_silk_line;
  Erasing++;
  gui->set_color (Output.fgGC, Settings.BackgroundColor);
  if (Layer == & PCB->Data->SILKLAYER
      || Layer == & PCB->Data->BACKSILKLAYER)
    min_silk_line = PCB->minSlk;
  else
    min_silk_line = PCB->minWid;
  DrawTextLowLevel (Text, min_silk_line);
  Erasing--;
}

/* ---------------------------------------------------------------------------
 * erases a polygon on a layer
 */
void
ErasePolygon (PolygonTypePtr Polygon)
{
  Erasing++;
  gui->set_color (Output.fgGC, Settings.BackgroundColor);
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
  gui->set_color (Output.fgGC, Settings.BackgroundColor);
  ELEMENTLINE_LOOP (Element);
  {
    DrawLineLowLevel (line, False);
  }
  END_LOOP;
  ARC_LOOP (Element);
  {
    DrawArcLowLevel (arc);
  }
  END_LOOP;
  if (!TEST_FLAG (HIDENAMEFLAG, Element))
    DrawTextLowLevel (&ELEMENT_TEXT (PCB, Element), PCB->minSlk);
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
  gui->set_color (Output.fgGC, Settings.BackgroundColor);
  PIN_LOOP (Element);
  {
    /* if (TEST_ANY_PIPS (pin))
       {
       ClearPin (pin, NO_TYPE, 0);
       gui->set_color (Output.fgGC, Settings.BackgroundColor);
       }
     */
    DrawPinOrViaLowLevel (pin, False);
    if (TEST_FLAG (DISPLAYNAMEFLAG, pin))
      DrawPinOrViaNameLowLevel (pin);
  }
  END_LOOP;
  PAD_LOOP (Element);
  {
    DrawPadLowLevel (Output.fgGC, pad, False, False);
    if (TEST_FLAG (DISPLAYNAMEFLAG, pad))
      DrawPadNameLowLevel (pad);
  }
  END_LOOP;
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
  gui->set_color (Output.fgGC, Settings.BackgroundColor);
  DrawTextLowLevel (&ELEMENT_TEXT (PCB, Element), PCB->minSlk);
  Erasing--;
}


void
EraseObject (int type, void *lptr, void *ptr)
{
  switch (type)
    {
    case VIA_TYPE:
    case PIN_TYPE:
      ErasePin ((PinTypePtr) ptr);
      break;
    case TEXT_TYPE:
    case ELEMENTNAME_TYPE:
      EraseText (lptr, (TextTypePtr) ptr);
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

/* ---------------------------------------------------------------------------
 * HID drawing callback.
 */

void
hid_expose_callback (HID * hid, BoxType * region, void *item)
{
  HID *old_gui = gui;
  hidGC savebg = Output.bgGC;
  hidGC savefg = Output.fgGC;
  hidGC savepm = Output.pmGC;

  gui = hid;
  Output.fgGC = gui->make_gc ();
  Output.bgGC = gui->make_gc ();
  Output.pmGC = gui->make_gc ();

  render = True;
  Gathering = False;

  /*printf("\033[32mhid_expose_callback, s=%p %d\033[0m\n", &(SWAP_IDENT), SWAP_IDENT); */

  hid->set_color (Output.pmGC, "erase");
  hid->set_color (Output.bgGC, "drill");

  if (item)
    {
      doing_pinout = True;
      DrawElement (item, 0);
      doing_pinout = False;
    }
  else
    DrawEverything (region);

  gui->destroy_gc (Output.fgGC);
  gui->destroy_gc (Output.bgGC);
  gui->destroy_gc (Output.pmGC);
  gui = old_gui;
  Output.fgGC = savefg;
  Output.bgGC = savebg;
  Output.pmGC = savepm;

  Gathering = True;
}
