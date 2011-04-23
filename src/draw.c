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

#ifndef MAXINT
#define MAXINT (((unsigned int)(~0))>>1)
#endif

RCSID ("$Id$");

#define	SMALL_SMALL_TEXT_SIZE	0
#define	SMALL_TEXT_SIZE			1
#define	NORMAL_TEXT_SIZE		2
#define	LARGE_TEXT_SIZE			3
#define	N_TEXT_SIZES			4

#define ON_SIDE(element, side) (TEST_FLAG (ONSOLDERFLAG, element) == (*side == SOLDER_LAYER))

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static BoxType Block = {MAXINT, MAXINT, -MAXINT, -MAXINT};
static bool Gathering = true;

static int doing_pinout = 0;
static bool doing_assy = false;
static const BoxType *clip_box = NULL;

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void Redraw (bool, BoxTypePtr);
static void DrawEverything (BoxTypePtr);
static void DrawTop (const BoxType *);
static int DrawLayerGroup (int, const BoxType *);
static void DrawPinOrViaLowLevel (PinTypePtr, bool);
static void DrawPlainPin (PinTypePtr, bool);
static void DrawPlainVia (PinTypePtr, bool);
static void DrawPinOrViaNameLowLevel (PinTypePtr);
static void DrawPadLowLevel (hidGC, PadTypePtr, bool, bool);
static void DrawPadNameLowLevel (PadTypePtr);
static void DrawLineLowLevel (LineTypePtr);
static void DrawRegularText (LayerTypePtr, TextTypePtr);
static void DrawPolygonLowLevel (PolygonTypePtr);
static void DrawArcLowLevel (ArcTypePtr);
static void DrawElementPackageLowLevel (ElementTypePtr Element);
static void DrawPlainPolygon (LayerTypePtr Layer, PolygonTypePtr Polygon);
static void AddPart (void *);
static void SetPVColor (PinTypePtr, int);
static void DrawEMark (ElementTypePtr, LocationType, LocationType, bool);
static void ClearPad (PadTypePtr, bool);
static void DrawHole (PinTypePtr);
static void DrawMask (int side, BoxType *);
static void DrawRats (BoxType *);
static void DrawSilk (int, int, const BoxType *);
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
  HideCrosshair ();

  /* clear and create event if not drawing to a pixmap
   */
  if (Block.X1 <= Block.X2 && Block.Y1 <= Block.Y2)
    gui->invalidate_lr (Block.X1, Block.X2, Block.Y1, Block.Y2);

  RestoreCrosshair ();

  /* shrink the update block */
  Block.X1 = Block.Y1 =  MAXINT;
  Block.X2 = Block.Y2 = -MAXINT;
}

/* ---------------------------------------------------------------------------
 * redraws the output area without clearing it
 */
void
RedrawOutput (BoxTypePtr area)
{
  Redraw (true, area);
}

/* ---------------------------------------------------------------------------
 * redraws the output area after clearing it
 */
void
ClearAndRedrawOutput (void)
{
  Gathering = false;
  UpdateAll ();
}




/* ---------------------------------------------------------------------- 
 * redraws all the data
 * all necessary sizes are already set by the porthole widget and
 * by the event handlers
 */
static void
Redraw (bool ClearWindow, BoxTypePtr screen_area)
{
  gui->invalidate_all ();
  Gathering = true;
}

static int
element_callback (const BoxType * b, void *cl)
{
  ElementTypePtr element = (ElementTypePtr) b;
  int *side = cl;

  if (ON_SIDE (element, side))
    DrawElementPackage (element);
  return 1;
}

static int
name_callback (const BoxType * b, void *cl)
{
  TextTypePtr text = (TextTypePtr) b;
  ElementTypePtr element = (ElementTypePtr) text->Element;
  int *side = cl;

  if (TEST_FLAG (HIDENAMEFLAG, element))
    return 0;

  if (ON_SIDE (element, side))
    DrawElementName (element);
  return 0;
}

static int
EMark_callback (const BoxType * b, void *cl)
{
  ElementTypePtr element = (ElementTypePtr) b;

  DrawEMark (element, element->MarkX, element->MarkY, !FRONT (element));
  return 1;
}

static int
hole_callback (const BoxType * b, void *cl)
{
  PinTypePtr pin = (PinTypePtr) b;
  int plated = cl ? *(int *) cl : -1;

  if ((plated == 0 && !TEST_FLAG (HOLEFLAG, pin)) ||
      (plated == 1 &&  TEST_FLAG (HOLEFLAG, pin)))
    return 1;

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
  DrawRat ((RatTypePtr) b);
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
  DrawTop (drawn_area);
  gui->set_draw_faded (Output.fgGC, 0);

  /* draw package */
  DrawSilk (swap_ident,
	    swap_ident ? solder_silk_layer : component_silk_layer,
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
  for (ngroups = 0, i = 0; i < max_copper_layer; i++)
    {
      LayerType *l = LAYER_ON_STACK (i);
      int group = GetLayerGroupNumberByNumber (LayerStack[i]);
      if (l->On && !do_group[group])
	{
	  do_group[group] = 1;
	  drawn_groups[ngroups++] = group;
	}
    }

  component = GetLayerGroupNumberByNumber (component_silk_layer);
  solder = GetLayerGroupNumberByNumber (solder_silk_layer);

  /*
   * first draw all 'invisible' stuff
   */
  if (!TEST_FLAG (CHECKPLANESFLAG, PCB)
      && gui->set_layer ("invisible", SL (INVISIBLE, 0), 0))
    {
      side = SWAP_IDENT ? COMPONENT_LAYER : SOLDER_LAYER;
      r_search (PCB->Data->pad_tree, drawn_area, NULL, pad_callback, &side);
      if (PCB->ElementOn)
	{
	  r_search (PCB->Data->element_tree, drawn_area, NULL, element_callback, &side);
	  r_search (PCB->Data->name_tree[NAME_INDEX (PCB)], drawn_area, NULL, name_callback, &side);
	  DrawLayer (&(PCB->Data->Layer[max_copper_layer + side]), drawn_area);
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
	      r_search (PCB->Data->pin_tree, drawn_area, NULL, pin_callback, NULL);
	      r_search (PCB->Data->via_tree, drawn_area, NULL, via_callback, NULL);
	      /* draw element pads */
	      if (group == component || group == solder)
		{
                  side = (group == solder) ? SOLDER_LAYER : COMPONENT_LAYER;
		  r_search (PCB->Data->pad_tree, drawn_area, NULL, pad_callback, &side);
		}

	      /* draw holes */
	      plated = -1;
	      r_search (PCB->Data->pin_tree, drawn_area, NULL, hole_callback, &plated);
	      r_search (PCB->Data->via_tree, drawn_area, NULL, hole_callback, &plated);
	    }
	}
    }
  if (TEST_FLAG (CHECKPLANESFLAG, PCB) && gui->gui)
    return;

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
  /* Draw the solder mask if turned on */
  if (gui->set_layer ("componentmask", SL (MASK, TOP), 0))
    DrawMask (COMPONENT_LAYER, drawn_area);

  if (gui->set_layer ("soldermask", SL (MASK, BOTTOM), 0))
    DrawMask (SOLDER_LAYER, drawn_area);

  /* Draw top silkscreen */
  if (gui->set_layer ("topsilk", SL (SILK, TOP), 0))
    DrawSilk (0, component_silk_layer, drawn_area);
  if (gui->set_layer ("bottomsilk", SL (SILK, BOTTOM), 0))
    DrawSilk (1, solder_silk_layer, drawn_area);
  if (gui->gui)
    {
      /* Draw element Marks */
      if (PCB->PinOn)
	r_search (PCB->Data->element_tree, drawn_area, NULL, EMark_callback,
		  NULL);
      /* Draw rat lines on top */
      if (gui->set_layer ("rats", SL (RATS, 0), 0))
	DrawRats(drawn_area);
    }

  for (side = 0; side <= 1; side++)
    {
      int doit;
      bool NoData = true;
      ALLPAD_LOOP (PCB->Data);
      {
	if ((TEST_FLAG (ONSOLDERFLAG, pad) && side == SOLDER_LAYER)
	    || (!TEST_FLAG (ONSOLDERFLAG, pad) && side == COMPONENT_LAYER))
	  {
	    NoData = false;
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
	      if (!TEST_FLAG (NOPASTEFLAG, pad) && pad->Mask > 0)
		{
		  if (pad->Mask < pad->Thickness)
		    DrawPadLowLevel (Output.fgGC, pad, true, true);
		  else
		    DrawPadLowLevel (Output.fgGC, pad, false, false);
		}
	  }
	  ENDALL_LOOP;
	}
    }

  doing_assy = true;
  if (gui->set_layer ("topassembly", SL (ASSY, TOP), 0))
    PrintAssembly (drawn_area, component, 0);

  if (gui->set_layer ("bottomassembly", SL (ASSY, BOTTOM), 0))
    PrintAssembly (drawn_area, solder, 1);
  doing_assy = false;

  if (gui->set_layer ("fab", SL (FAB, 0), 0))
    PrintFab ();
}

static void
DrawEMark (ElementTypePtr e, LocationType X, LocationType Y,
	   bool invisible)
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
  DrawPlainVia (via, false);
  return 1;
}

static int
pin_callback (const BoxType * b, void *cl)
{
  DrawPlainPin ((PinTypePtr) b, false);
  return 1;
}

static int
pad_callback (const BoxType * b, void *cl)
{
  PadTypePtr pad = (PadTypePtr) b;
  int *side = cl;

  if (TEST_FLAG (ONSOLDERFLAG, pad) == (*side == SOLDER_LAYER))
    DrawPad (pad);
  return 1;
}

/* ---------------------------------------------------------------------------
 * draws pins pads and vias
 */
static void
DrawTop (const BoxType * screen)
{
  int side = SWAP_IDENT ? SOLDER_LAYER : COMPONENT_LAYER;

  if (PCB->PinOn || doing_assy)
    {
      /* draw element pins */
      r_search (PCB->Data->pin_tree, screen, NULL, pin_callback, NULL);
      /* draw element pads */
      r_search (PCB->Data->pad_tree, screen, NULL, pad_callback, &side);
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

static int
clearPin_callback (const BoxType * b, void *cl)
{
  PinType *pin = (PinTypePtr) b;
  if (TEST_FLAG (THINDRAWFLAG, PCB) || TEST_FLAG (THINDRAWPOLYFLAG, PCB))
    gui->thindraw_pcb_pv (Output.pmGC, Output.pmGC, pin, false, true);
  else
    gui->fill_pcb_pv (Output.pmGC, Output.pmGC, pin, false, true);
  return 1;
}
static int
poly_callback (const BoxType * b, void *cl)
{
  LayerType *layer = cl;

  DrawPlainPolygon (layer, (PolygonTypePtr) b);
  return 1;
}

static int
clearPad_callback (const BoxType * b, void *cl)
{
  PadTypePtr pad = (PadTypePtr) b;
  int *side = cl;
  if (ON_SIDE (pad, side) && pad->Mask)
    ClearPad (pad, true);
  return 1;
}

/* ---------------------------------------------------------------------------
 * Draws silk layer.
 */

static void
DrawSilk (int new_swap, int layer, const BoxType * drawn_area)
{
  int side = new_swap ? SOLDER_LAYER : COMPONENT_LAYER;
#if 0
  /* This code is used when you want to mask silk to avoid exposed
     pins and pads.  We decided it was a bad idea to do this
     unconditionally, but the code remains.  */
#endif

#if 0
  if (gui->poly_before)
    {
      gui->use_mask (HID_MASK_BEFORE);
#endif
      DrawLayer (LAYER_PTR (layer), drawn_area);
      /* draw package */
      r_search (PCB->Data->element_tree, drawn_area, NULL, element_callback, &side);
      r_search (PCB->Data->name_tree[NAME_INDEX (PCB)], drawn_area, NULL, name_callback, &side);
#if 0
    }

  gui->use_mask (HID_MASK_CLEAR);
  r_search (PCB->Data->pin_tree, drawn_area, NULL, clearPin_callback, NULL);
  r_search (PCB->Data->via_tree, drawn_area, NULL, clearPin_callback, NULL);
  r_search (PCB->Data->pad_tree, drawn_area, NULL, clearPad_callback, &side);

  if (gui->poly_after)
    {
      gui->use_mask (HID_MASK_AFTER);
      DrawLayer (LAYER_PTR (max_copper_layer + layer), drawn_area);
      /* draw package */
      r_search (PCB->Data->element_tree, drawn_area, NULL, element_callback, &side);
      r_search (PCB->Data->name_tree[NAME_INDEX (PCB)], drawn_area, NULL, name_callback, &side);
    }
  gui->use_mask (HID_MASK_OFF);
#endif
}


static void
DrawMaskBoardArea (int mask_type, BoxType *screen)
{
  /* Skip the mask drawing if the GUI doesn't want this type */
  if ((mask_type == HID_MASK_BEFORE && !gui->poly_before) ||
      (mask_type == HID_MASK_AFTER  && !gui->poly_after))
    return;

  gui->use_mask (mask_type);
  gui->set_color (Output.fgGC, PCB->MaskColor);
  if (screen == NULL)
    gui->fill_rect (Output.fgGC, 0, 0, PCB->MaxWidth, PCB->MaxHeight);
  else
    gui->fill_rect (Output.fgGC, screen->X1, screen->Y1,
                                 screen->X2, screen->Y2);
}

/* ---------------------------------------------------------------------------
 * draws solder mask layer - this will cover nearly everything
 */
static void
DrawMask (int side, BoxType * screen)
{
  int thin = TEST_FLAG(THINDRAWFLAG, PCB) || TEST_FLAG(THINDRAWPOLYFLAG, PCB);

  if (thin)
    gui->set_color (Output.pmGC, PCB->MaskColor);
  else
    {
      DrawMaskBoardArea (HID_MASK_BEFORE, screen);
      gui->use_mask (HID_MASK_CLEAR);
    }

  r_search (PCB->Data->pin_tree, screen, NULL, clearPin_callback, NULL);
  r_search (PCB->Data->via_tree, screen, NULL, clearPin_callback, NULL);
  r_search (PCB->Data->pad_tree, screen, NULL, clearPad_callback, &side);

  if (thin)
    gui->set_color (Output.pmGC, "erase");
  else
    {
      DrawMaskBoardArea (HID_MASK_AFTER, screen);
      gui->use_mask (HID_MASK_OFF);
    }
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
  DrawLine ((LayerTypePtr) cl, (LineTypePtr) b);
  return 1;
}

static int
arc_callback (const BoxType * b, void *cl)
{
  DrawArc ((LayerTypePtr) cl, (ArcTypePtr) b);
  return 1;
}

static int
text_callback (const BoxType * b, void *cl)
{
  DrawRegularText ((LayerTypePtr) cl, (TextTypePtr) b);
  return 1;
}


/* ---------------------------------------------------------------------------
 * draws one non-copper layer
 */
void
DrawLayerCommon (LayerTypePtr Layer, const BoxType * screen, bool clear_pins)
{
  /* print the non-clearing polys */
  clip_box = screen;
  r_search (Layer->polygon_tree, screen, NULL, poly_callback, Layer);

  if (clear_pins && TEST_FLAG (CHECKPLANESFLAG, PCB))
    return;

  /* draw all visible lines this layer */
  r_search (Layer->line_tree, screen, NULL, line_callback, Layer);

  /* draw the layer arcs on screen */
  r_search (Layer->arc_tree, screen, NULL, arc_callback, Layer);

  /* draw the layer text on screen */
  r_search (Layer->text_tree, screen, NULL, text_callback, Layer);

  /* We should check for gui->gui here, but it's kinda cool seeing the
     auto-outline magically disappear when you first add something to
     the "outline" layer.  */
  if (IsLayerEmpty (Layer)
      && (strcmp (Layer->Name, "outline") == 0
	  || strcmp (Layer->Name, "route") == 0))
    {
      gui->set_color (Output.fgGC, Layer->Color);
      gui->set_line_width (Output.fgGC, PCB->minWid);
      gui->draw_rect (Output.fgGC,
		      0, 0,
		      PCB->MaxWidth, PCB->MaxHeight);
    }

  clip_box = NULL;
}

void
DrawLayer (LayerTypePtr Layer, const BoxType * screen)
{
  DrawLayerCommon (Layer, screen, false);
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
  LayerTypePtr Layer;
  int n_entries = PCB->LayerGroups.Number[group];
  Cardinal *layers = PCB->LayerGroups.Entries[group];

  clip_box = screen;
  for (i = n_entries - 1; i >= 0; i--)
    {
      layernum = layers[i];
      Layer = PCB->Data->Layer + layers[i];
      if (strcmp (Layer->Name, "outline") == 0 ||
	  strcmp (Layer->Name, "route") == 0)
	rv = 0;
      if (layernum < max_copper_layer && Layer->On)
	DrawLayerCommon (Layer, screen, true);
    }
  if (n_entries > 1)
    rv = 1;
  return rv;
}

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for pins and vias
 */
static void
DrawPinOrViaLowLevel (PinTypePtr pv, bool drawHole)
{
  if (Gathering)
    {
      AddPart (pv);
      return;
    }

  if (TEST_FLAG (THINDRAWFLAG, PCB))
    gui->thindraw_pcb_pv (Output.fgGC, Output.fgGC, pv, drawHole, false);
  else
    gui->fill_pcb_pv (Output.fgGC, Output.bgGC, pv, drawHole, false);
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

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for pin and via names
 */
static void
DrawPinOrViaNameLowLevel (PinTypePtr Ptr)
{
  char *name;
  BoxType box;
  bool vert;
  TextType text;

  if (!Ptr->Name || !Ptr->Name[0])
    name = (char *)EMPTY (Ptr->Number);
  else
    name = (char *)EMPTY (TEST_FLAG (SHOWNUMBERFLAG, PCB) ? Ptr->Number : Ptr->Name);

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
DrawPadLowLevel (hidGC gc, PadTypePtr Pad, bool clear, bool mask)
{
  if (Gathering)
    {
      AddPart (Pad);
      return;
    }

  if (clear && !mask && Pad->Clearance <= 0)
    return;

  if (TEST_FLAG (THINDRAWFLAG, PCB) ||
      (clear && TEST_FLAG (THINDRAWPOLYFLAG, PCB)))
    gui->thindraw_pcb_pad (gc, Pad, clear, mask);
  else
    gui->fill_pcb_pad (gc, Pad, clear, mask);
}

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for pad names
 */

static void
DrawPadNameLowLevel (PadTypePtr Pad)
{
  BoxType box;
  char *name;
  bool vert;
  TextType text;

  if (!Pad->Name || !Pad->Name[0])
    name = (char *)EMPTY (Pad->Number);
  else
    name = (char *)EMPTY (TEST_FLAG (SHOWNUMBERFLAG, PCB) ? Pad->Number : Pad->Name);

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
ClearPad (PadTypePtr Pad, bool mask)
{
  DrawPadLowLevel(Output.pmGC, Pad, true, mask);
}

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for lines
 */
static void
DrawLineLowLevel (LineTypePtr Line)
{
  if (Gathering)
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
	      DrawLineLowLevel (&newline);
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
DrawElementPackageLowLevel (ElementTypePtr Element)
{
  /* draw lines, arcs, text and pins */
  ELEMENTLINE_LOOP (Element);
  {
    DrawLineLowLevel (line);
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
DrawVia (PinTypePtr Via)
{
  if (!Gathering)
    SetPVColor (Via, VIA_TYPE);
  DrawPinOrViaLowLevel (Via, true);
  if (!TEST_FLAG (HOLEFLAG, Via) && TEST_FLAG (DISPLAYNAMEFLAG, Via))
    DrawPinOrViaNameLowLevel (Via);
}

/* ---------------------------------------------------------------------------
 * draw a via without dealing with polygon clearance 
 */
static void
DrawPlainVia (PinTypePtr Via, bool holeToo)
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
DrawViaName (PinTypePtr Via)
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
DrawPin (PinTypePtr Pin)
{
  {
    if (!Gathering)
      SetPVColor (Pin, PIN_TYPE);
    DrawPinOrViaLowLevel (Pin, true);
  }
  if ((!TEST_FLAG (HOLEFLAG, Pin) && TEST_FLAG (DISPLAYNAMEFLAG, Pin))
      || doing_pinout)
    DrawPinOrViaNameLowLevel (Pin);
}

/* ---------------------------------------------------------------------------
 * draw a pin without clearing around polygons 
 */
static void
DrawPlainPin (PinTypePtr Pin, bool holeToo)
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
DrawPinName (PinTypePtr Pin)
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
DrawPad (PadTypePtr Pad)
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
  DrawPadLowLevel (Output.fgGC, Pad, false, false);
  if (doing_pinout || TEST_FLAG (DISPLAYNAMEFLAG, Pad))
    DrawPadNameLowLevel (Pad);
}

/* ---------------------------------------------------------------------------
 * draws the name of a pad
 */
void
DrawPadName (PadTypePtr Pad)
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
DrawLine (LayerTypePtr Layer, LineTypePtr Line)
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
  DrawLineLowLevel (Line);
}

/* ---------------------------------------------------------------------------
 * draws a ratline
 */
void
DrawRat (RatTypePtr Line)
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
    DrawLineLowLevel ((LineTypePtr) Line);
}

/* ---------------------------------------------------------------------------
 * draws an arc on a layer
 */
void
DrawArc (LayerTypePtr Layer, ArcTypePtr Arc)
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
DrawText (LayerTypePtr Layer, TextTypePtr Text)
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
DrawRegularText (LayerTypePtr Layer, TextTypePtr Text)
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

/* ---------------------------------------------------------------------------
 * draws a polygon on a layer
 */
void
DrawPolygon (LayerTypePtr Layer, PolygonTypePtr Polygon)
{
  DrawPolygonLowLevel (Polygon);
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
DrawElement (ElementTypePtr Element)
{
  DrawElementPackage (Element);
  DrawElementName (Element);
  DrawElementPinsAndPads (Element);
}

/* ---------------------------------------------------------------------------
 * draws the name of an element
 */
void
DrawElementName (ElementTypePtr Element)
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
DrawElementPackage (ElementTypePtr Element)
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
  DrawElementPackageLowLevel (Element);
}

/* ---------------------------------------------------------------------------
 * draw pins of an element
 */
void
DrawElementPinsAndPads (ElementTypePtr Element)
{
  PAD_LOOP (Element);
  {
    if (doing_pinout || doing_assy || FRONT (pad) || PCB->InvisibleObjectsOn)
      DrawPad (pad);
  }
  END_LOOP;
  PIN_LOOP (Element);
  {
    DrawPin (pin);
  }
  END_LOOP;
}

/* ---------------------------------------------------------------------------
 * erase a via
 */
void
EraseVia (PinTypePtr Via)
{
  DrawPinOrViaLowLevel (Via, false);
  if (TEST_FLAG (DISPLAYNAMEFLAG, Via))
    DrawPinOrViaNameLowLevel (Via);
}

/* ---------------------------------------------------------------------------
 * erase a ratline
 */
void
EraseRat (RatTypePtr Rat)
{
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
    DrawLineLowLevel ((LineTypePtr) Rat);
}


/* ---------------------------------------------------------------------------
 * erase a via name
 */
void
EraseViaName (PinTypePtr Via)
{
  DrawPinOrViaNameLowLevel (Via);
}

/* ---------------------------------------------------------------------------
 * erase a pad object
 */
void
ErasePad (PadTypePtr Pad)
{
  DrawPadLowLevel (Output.fgGC, Pad, false, false);
  if (TEST_FLAG (DISPLAYNAMEFLAG, Pad))
    DrawPadNameLowLevel (Pad);
}

/* ---------------------------------------------------------------------------
 * erase a pad name
 */
void
ErasePadName (PadTypePtr Pad)
{
  DrawPadNameLowLevel (Pad);
}

/* ---------------------------------------------------------------------------
 * erase a pin object
 */
void
ErasePin (PinTypePtr Pin)
{
  DrawPinOrViaLowLevel (Pin, false);
  if (TEST_FLAG (DISPLAYNAMEFLAG, Pin))
    DrawPinOrViaNameLowLevel (Pin);
}

/* ---------------------------------------------------------------------------
 * erase a pin name
 */
void
ErasePinName (PinTypePtr Pin)
{
  DrawPinOrViaNameLowLevel (Pin);
}

/* ---------------------------------------------------------------------------
 * erases a line on a layer
 */
void
EraseLine (LineTypePtr Line)
{
  DrawLineLowLevel (Line);
}

/* ---------------------------------------------------------------------------
 * erases an arc on a layer
 */
void
EraseArc (ArcTypePtr Arc)
{
  if (!Arc->Thickness)
    return;
  DrawArcLowLevel (Arc);
}

/* ---------------------------------------------------------------------------
 * erases a text on a layer
 */
void
EraseText (LayerTypePtr Layer, TextTypePtr Text)
{
  int min_silk_line;
  if (Layer == & PCB->Data->SILKLAYER
      || Layer == & PCB->Data->BACKSILKLAYER)
    min_silk_line = PCB->minSlk;
  else
    min_silk_line = PCB->minWid;
  DrawTextLowLevel (Text, min_silk_line);
}

/* ---------------------------------------------------------------------------
 * erases a polygon on a layer
 */
void
ErasePolygon (PolygonTypePtr Polygon)
{
  DrawPolygonLowLevel (Polygon);
}

/* ---------------------------------------------------------------------------
 * erases an element
 */
void
EraseElement (ElementTypePtr Element)
{
  ELEMENTLINE_LOOP (Element);
  {
    DrawLineLowLevel (line);
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
}

/* ---------------------------------------------------------------------------
 * erases all pins and pads of an element
 */
void
EraseElementPinsAndPads (ElementTypePtr Element)
{
  PIN_LOOP (Element);
  {
    DrawPinOrViaLowLevel (pin, false);
    if (TEST_FLAG (DISPLAYNAMEFLAG, pin))
      DrawPinOrViaNameLowLevel (pin);
  }
  END_LOOP;
  PAD_LOOP (Element);
  {
    DrawPadLowLevel (Output.fgGC, pad, false, false);
    if (TEST_FLAG (DISPLAYNAMEFLAG, pad))
      DrawPadNameLowLevel (pad);
  }
  END_LOOP;
}

/* ---------------------------------------------------------------------------
 * erases the name of an element
 */
void
EraseElementName (ElementTypePtr Element)
{
  if (TEST_FLAG (HIDENAMEFLAG, Element))
    return;
  DrawTextLowLevel (&ELEMENT_TEXT (PCB, Element), PCB->minSlk);
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
      EraseText ((LayerTypePtr)lptr, (TextTypePtr) ptr);
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
DrawObject (int type, void *ptr1, void *ptr2)
{
  switch (type)
    {
    case VIA_TYPE:
      if (PCB->ViaOn)
	DrawVia ((PinTypePtr) ptr2);
      break;
    case LINE_TYPE:
      if (((LayerTypePtr) ptr1)->On)
	DrawLine ((LayerTypePtr) ptr1, (LineTypePtr) ptr2);
      break;
    case ARC_TYPE:
      if (((LayerTypePtr) ptr1)->On)
	DrawArc ((LayerTypePtr) ptr1, (ArcTypePtr) ptr2);
      break;
    case TEXT_TYPE:
      if (((LayerTypePtr) ptr1)->On)
	DrawText ((LayerTypePtr) ptr1, (TextTypePtr) ptr2);
      break;
    case POLYGON_TYPE:
      if (((LayerTypePtr) ptr1)->On)
	DrawPolygon ((LayerTypePtr) ptr1, (PolygonTypePtr) ptr2);
      break;
    case ELEMENT_TYPE:
      if (PCB->ElementOn &&
	  (FRONT ((ElementTypePtr) ptr2) || PCB->InvisibleObjectsOn))
	DrawElement ((ElementTypePtr) ptr2);
      break;
    case RATLINE_TYPE:
      if (PCB->RatOn)
	DrawRat ((RatTypePtr) ptr2);
      break;
    case PIN_TYPE:
      if (PCB->PinOn)
	DrawPin ((PinTypePtr) ptr2);
      break;
    case PAD_TYPE:
      if (PCB->PinOn)
	DrawPad ((PadTypePtr) ptr2);
      break;
    case ELEMENTNAME_TYPE:
      if (PCB->ElementOn &&
	  (FRONT ((ElementTypePtr) ptr2) || PCB->InvisibleObjectsOn))
	DrawElementName ((ElementTypePtr) ptr1);
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

  Gathering = false;

  /*printf("\033[32mhid_expose_callback, s=%p %d\033[0m\n", &(SWAP_IDENT), SWAP_IDENT); */

  hid->set_color (Output.pmGC, "erase");
  hid->set_color (Output.bgGC, "drill");

  if (item)
    {
      doing_pinout = true;
      DrawElement ((ElementTypePtr)item);
      doing_pinout = false;
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

  Gathering = true;
}
