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

/* ---------------------------------------------------------------------------
 * define TO_SCREEN before macro.h is included from global.h
 */
#define TO_SCREEN(c)  ((Position)((c)*Local_Zoom)) 

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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

#include "gui.h"


RCSID("$Id$");

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
  gfloat X, Y;
}
FloatPolyType, *FloatPolyTypePtr;

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static GdkDrawable	*DrawingWindow;	/* flag common to all */
static BoxType		Block;
static gboolean		Gathering = True;
static gint			Erasing = False;


/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void Redraw (gboolean, BoxTypePtr);
static void DrawEverything (BoxTypePtr);
static void DrawTop (BoxType *);
static void DrawLayer (LayerTypePtr, BoxType *);
static void DrawSpecialPolygon (GdkDrawable *, GdkGC *, LocationType, LocationType, BDimension);
static void DrawPinOrViaLowLevel (PinTypePtr, gboolean);
static void ClearOnlyPin (PinTypePtr, gboolean);
static void ThermPin (LayerTypePtr, PinTypePtr);
static void DrawPlainPin (PinTypePtr, gboolean);
static void DrawPlainVia (PinTypePtr, gboolean);
static void DrawPinOrViaNameLowLevel (PinTypePtr);
static void DrawPadLowLevel (PadTypePtr);
static void DrawPadNameLowLevel (PadTypePtr);
static void DrawLineLowLevel (LineTypePtr, gboolean);
static void DrawTextLowLevel (TextTypePtr);
static void DrawRegularText (LayerTypePtr, TextTypePtr, int);
static void DrawPolygonLowLevel (PolygonTypePtr);
static void DrawArcLowLevel (ArcTypePtr);
static void DrawElementPackageLowLevel (ElementTypePtr Element, int);
static void DrawPlainPolygon (LayerTypePtr Layer, PolygonTypePtr Polygon);
static void AddPart (void *, gboolean);
static void SetPVColor (PinTypePtr, int);
static void DrawGrid (void);
static void DrawEMark (LocationType, LocationType, gboolean);
static void ClearLine (LineTypePtr);
static void ClearArc (ArcTypePtr);
static void ClearPad (PadTypePtr, gboolean);
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
	    gdk_gc_set_foreground(Output.fgGC, PCB->WarnColor);
	  else if (TEST_FLAG (SELECTEDFLAG, Pin))
	    gdk_gc_set_foreground(Output.fgGC, PCB->ViaSelectedColor);
	  else
	    gdk_gc_set_foreground(Output.fgGC, PCB->ConnectedColor);
	}
      else
	gdk_gc_set_foreground(Output.fgGC, PCB->ViaColor);
    }
  else
    {
      if (TEST_FLAG (WARNFLAG | SELECTEDFLAG | FOUNDFLAG, Pin))
	{
	  if (TEST_FLAG (WARNFLAG, Pin))
	    gdk_gc_set_foreground(Output.fgGC, PCB->WarnColor);
	  else if (TEST_FLAG (SELECTEDFLAG, Pin))
	    gdk_gc_set_foreground(Output.fgGC, PCB->PinSelectedColor);
	  else
	    gdk_gc_set_foreground(Output.fgGC, PCB->ConnectedColor);
	}
      else
	gdk_gc_set_foreground(Output.fgGC, PCB->PinColor);
    }
}

/*---------------------------------------------------------------------------
 *  Adds the update rect to the update region
 */
static void
AddPart (void *b, gboolean screen_coord)
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
      Block.X1 = MIN (Block.X1, TO_LIMIT_X (box->X1));
      Block.X2 = MAX (Block.X2, TO_LIMIT_X (box->X2));
      Block.Y1 =
	MIN (Block.Y1, MIN (TO_LIMIT_Y (box->Y1), TO_LIMIT_Y (box->Y2)));
      Block.Y2 =
	MAX (Block.Y2, MAX (TO_LIMIT_Y (box->Y2), TO_LIMIT_Y (box->Y1)));
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
	OutputType		*out = &Output;
	GdkRectangle	erased;

	render = True;
	if (out->pixmap)
		{
		erased.x = SATURATE (Block.X1 - 1);
		erased.y = SATURATE (Block.Y1 - 1);
		erased.width = SATURATE (Block.X2 - Block.X1 + 2);
		erased.height = SATURATE (Block.Y2 - Block.Y1 + 2);
		gdk_draw_rectangle(out->pixmap, Output.bgGC, TRUE,
				erased.x, erased.y, erased.width, erased.height);
		gdk_window_invalidate_rect(out->drawing_area->window, &erased, FALSE);
		}
	else
		{
		HideCrosshair (True);

		/* clear and create event if not drawing to a pixmap
		*/
		gdk_window_clear_area_e(Output.drawing_area->window,
				SATURATE (Block.X1 - 1),
				SATURATE (Block.Y1 - 1), SATURATE (Block.X2 - Block.X1 + 2),
				SATURATE (Block.Y2 - Block.Y1 + 2));
		RestoreCrosshair (True);
		}

	/* shrink the update block */
	Block.X1 = MAX_COORD / 100;
	Block.Y1 = MAX_COORD / 100;
	Block.X2 = Block.Y2 = 0;
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
 * redraws the background image
 */
#if XXX
/* Not ported to Gtk yet.*/

static int bg_w, bg_h, bgi_w, bgi_h;
static GdkPixel **bg = 0;
static GdkImage *bgi = 0;
static enum {
  PT_unknown,
  PT_RGB565
} pixel_type = PT_unknown;

static void
LoadBackgroundFile (FILE *f, char *filename)
{
  Display *display;
  Colormap cmap;
  XVisualInfo vinfot, *vinfo;
  Visual *vis;
  int c, r, b;
  int i, nret;
  int p[3], rows, cols, maxval;

  if (fgetc(f) != 'P')
    {
      printf("bgimage: %s signature not P6\n", filename);
      return;
    }
  if (fgetc(f) != '6')
    {
      printf("bgimage: %s signature not P6\n", filename);
      return;
    }
  for (i=0; i<3; i++)
    {
      do {
	b = fgetc(f);
	if (feof(f))
	  return;
	if (b == '#')
	  while (!feof(f) && b != '\n')
	    b = fgetc(f);
      } while (!isdigit(b));
      p[i] = b - '0';
      while (isdigit(b = fgetc(f)))
	p[i] = p[i]*10 + b - '0';
    }
  bg_w = cols = p[0];
  bg_h = rows = p[1];
  maxval = p[2];

  setbuf(stdout, 0);
  bg = (Pixel **) g_malloc (rows * sizeof (Pixel *));
  if (!bg)
    {
      printf("Out of memory loading %s\n", filename);
      return;
    }
  for (i=0; i<rows; i++)
    {
      bg[i] = (Pixel *) g_malloc (cols * sizeof (Pixel));
      if (!bg[i])
	{
	  printf("Out of memory loading %s\n", filename);
	  while (--i >= 0)
	    g_free (bg[i]);
	  g_free (bg);
	  bg = 0;
	  return;
	}
    }

  display = XtDisplay (Output.Toplevel);
  cmap = DefaultColormap (display, DefaultScreen(display));
  vis = DefaultVisual (display, DefaultScreen(display));

  vinfot.visualid = XVisualIDFromVisual(vis);
  vinfo = XGetVisualInfo (display, VisualIDMask, &vinfot, &nret);

#if 0
  /* If you want to support more visuals below, you'll probably need
     this. */
  printf("vinfo: rm %04x gm %04x bm %04x depth %d class %d\n",
	 vinfo->red_mask, vinfo->green_mask, vinfo->blue_mask,
	 vinfo->depth, vinfo->class);
#endif

  if (vinfo->class == TrueColor
      && vinfo->depth == 16
      && vinfo->red_mask == 0xf800
      && vinfo->green_mask == 0x07e0
      && vinfo->blue_mask == 0x001f)
    pixel_type = PT_RGB565;

  for (r=0; r<rows; r++)
    {
      for (c=0; c<cols; c++)
	{
	  XColor pix;
	  unsigned int pr = (unsigned)fgetc(f);
	  unsigned int pg = (unsigned)fgetc(f);
	  unsigned int pb = (unsigned)fgetc(f);

	  switch (pixel_type)
	    {
	    case PT_unknown:
	      pix.red = pr * 65535 / maxval;
	      pix.green = pg * 65535 / maxval;
	      pix.blue = pb * 65535 / maxval;
	      pix.flags = DoRed | DoGreen | DoBlue;
	      XAllocColor (display, cmap, &pix);
	      bg[r][c] = pix.pixel;
	      break;
	    case PT_RGB565:
	      bg[r][c] = (pr>>3)<<11 | (pg>>2)<<5 | (pb>>3);
	      break;
	    }
	}
    }
}

void
LoadBackgroundImage (char *filename)
{
  FILE *f = fopen(filename, "rb");
  if (!f)
    {
      if (NSTRCMP (filename, "pcb-background.ppm"))
	perror(filename);
      return;
    }
  LoadBackgroundFile (f, filename);
  fclose(f);
}

static void
DrawBackgroundImage ()
{
  int x, y, w, h;
  double xscale, yscale;
  int pcbwidth = TO_DRAWABS_X (PCB->MaxWidth);
  int pcbheight = TO_DRAWABS_Y (PCB->MaxHeight);

  if (!DrawingWindow || !bg)
    return;

  if (!bgi || Output.Width != bgi_w || Output.Height != bgi_h)
    {
      if (bgi)
	XDestroyImage (bgi);
      /* Cheat - get the image, which sets up the format too.  */
      bgi = XGetImage (XtDisplay(Output.Toplevel),
		       DrawingWindow,
		       0, 0, Output.Width, Output.Height,
		       -1, ZPixmap);
      bgi_w = Output.Width;
      bgi_h = Output.Height;
    }

  w = MIN (Output.Width, pcbwidth);
  h = MIN (Output.Height, pcbheight);

  xscale = (double)bg_w / PCB->MaxWidth;
  yscale = (double)bg_h / PCB->MaxHeight;

  for (y=0; y<h; y++)
    {
      int pr = TO_PCB_Y(y);
      int ir = pr * yscale;
      for (x=0; x<w; x++)
	{
	  int pc = TO_PCB_X(x);
	  int ic = pc * xscale;
	  XPutPixel (bgi, x, y, bg[ir][ic]);
	}
    }
  XPutImage(XtDisplay(Output.Toplevel), DrawingWindow, Output.fgGC,
	    bgi,
	    0, 0, 0, 0, w, h);
}
#endif

static gchar *
markup_sized_string(gchar *str)
	{
	gchar	*markup, *fmt;
	gint	size;

	size = Output.font_size;
	if (size == SMALL_SMALL_TEXT_SIZE)
		fmt = "<small><small>%s</small></small>";
	else if (size == SMALL_TEXT_SIZE)
		fmt = "<small>%s</small>";
	else if (size == LARGE_TEXT_SIZE)
		fmt = "<big>%s</big>";
	else
		fmt = "%s";
	markup = g_strdup_printf(fmt, str);

	return markup;
	}

GdkDrawable *
draw_get_current_drawable(void)
	{
	return DrawingWindow;
	}

/* ----------------------------------------------------------------------
 * setup of zoom and output window for the next drawing operations
 */
gboolean
SwitchDrawingWindow (gfloat Zoom, GdkDrawable *OutputWindow, gboolean Swap,
		     gboolean Gather)
{
  gboolean oldGather = Gathering;

  Gathering = Gather;
  Local_Zoom = 0.01 / expf (Zoom * LN_2_OVER_2);

  Output.font_size = N_TEXT_SIZES - (gint) Zoom + 1;
  if (Output.font_size < SMALL_SMALL_TEXT_SIZE)
    Output.font_size = SMALL_SMALL_TEXT_SIZE;
  else if (Output.font_size > LARGE_TEXT_SIZE)
    Output.font_size = LARGE_TEXT_SIZE;

  DrawingWindow = OutputWindow;
  if (   OutputWindow == Output.pixmap
	  || OutputWindow == Output.drawing_area->window
	 )
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

  return (oldGather);
}


/* ---------------------------------------------------------------------- 
 * redraws all the data
 * all necessary sizes are already set by the porthole widget and
 * by the event handlers
 */
static void
Redraw (gboolean ClearWindow, BoxTypePtr screen_area)
{
  BoxType draw_area;
  Dimension pcbwidth, pcbheight;

  /* make sure window exists */
  if (Output.drawing_area->window
      && (render || ClearWindow || !Output.pixmap))
    {
      /* shrink the update block */
      Block.X1 = MAX_COORD / 100;
      Block.Y1 = MAX_COORD / 100;
      Block.X2 = Block.Y2 = 0;

      /* switch off crosshair if needed,
       * set up drawing window and redraw
       * everything with Gather = False
       */
      if (!Output.pixmap)
	HideCrosshair (True);
      SwitchDrawingWindow (PCB->Zoom,
			   Output.pixmap ? Output.pixmap :
			   Output.drawing_area->window, Settings.ShowSolderSide,
			   False);
      draw_area.X1 = TO_PCB_X (screen_area->X1);
      draw_area.X2 = TO_PCB_X (screen_area->X2);
      draw_area.Y1 =
	MIN (TO_PCB_Y (screen_area->Y1), TO_PCB_Y (screen_area->Y2));
      draw_area.Y2 =
	MAX (TO_PCB_Y (screen_area->Y1), TO_PCB_Y (screen_area->Y2));

      /* clear the background
       * of the drawing area
       */
      pcbwidth = TO_DRAWABS_X (PCB->MaxWidth);
      pcbheight = TO_DRAWABS_Y (PCB->MaxHeight);
      gdk_draw_rectangle(DrawingWindow, Output.bgGC, TRUE, 0, 0,
		      MIN (pcbwidth, Output.Width),
		      MIN (pcbheight, Output.Height));
      gdk_gc_set_foreground(Output.fgGC, &Settings.OffLimitColor);
      if (pcbwidth < Output.Width)
	gdk_draw_rectangle(DrawingWindow, Output.fgGC, TRUE,
			pcbwidth, 0, Output.Width - pcbwidth, Output.Height);
      if (pcbheight < Output.Height)
	gdk_draw_rectangle(DrawingWindow, Output.fgGC, TRUE,
			0, pcbheight, Output.Width, Output.Height - pcbheight);
      if (ClearWindow && !Output.pixmap)
	Crosshair.On = False;

/*      DrawBackgroundImage (); */
      DrawEverything (&draw_area);

      if (!Output.pixmap)
	RestoreCrosshair (True);
    }
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

  DrawEMark (element->MarkX, element->MarkY, !FRONT (element));
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
  DrawHole ((PinTypePtr) b);
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
 * initializes some identifiers for a new zoom factor and redraws whole screen
 */
static void
DrawEverything (BoxTypePtr drawn_area)
{
  int i;

  /*
   * first draw all 'invisible' stuff
   */
  if (PCB->InvisibleObjectsOn)
    {
      r_search (PCB->Data->pad_tree, drawn_area, NULL, backPad_callback,
		NULL);
      if (PCB->ElementOn)
	{
	  r_search (PCB->Data->element_tree, drawn_area, NULL, backE_callback,
		    NULL);
	  r_search (PCB->Data->name_tree[NAME_INDEX (PCB)], drawn_area, NULL,
		    backN_callback, NULL);
	  DrawLayer (LAYER_PTR
		     (MAX_LAYER +
		      (SWAP_IDENT ? COMPONENT_LAYER : SOLDER_LAYER)),
		     drawn_area);
	}
    }
  /* draw all layers in layerstack order */
  for (i = MAX_LAYER - 1; i >= 0; i--)
    if ((LAYER_ON_STACK (i))->On)
      DrawLayer (LAYER_ON_STACK (i), drawn_area);
  /* draw vias below silk */
  if (PCB->ViaOn)
    r_search (PCB->Data->via_tree, drawn_area, NULL, lowvia_callback, NULL);
  /* Draw the solder mask if turned on */
  if (TEST_FLAG (SHOWMASKFLAG, PCB))
    DrawMask (drawn_area);
  /* Draw top silkscreen */
  if (PCB->ElementOn)
    {
      DrawLayer (LAYER_PTR (MAX_LAYER + (SWAP_IDENT ? SOLDER_LAYER :
					 COMPONENT_LAYER)), drawn_area);

      /* draw package */
      r_search (PCB->Data->element_tree, drawn_area, NULL, frontE_callback,
		NULL);
      r_search (PCB->Data->name_tree[NAME_INDEX (PCB)], drawn_area, NULL,
		frontN_callback, NULL);
    }
  /* Draw pins, pads, vias above silk */
  DrawTop (drawn_area);
  if (PCB->PinOn)
    /* Draw pin holes */
    r_search (PCB->Data->pin_tree, drawn_area, NULL, hole_callback, NULL);
  /* Draw via holes */
  if (PCB->ViaOn)
    r_search (PCB->Data->via_tree, drawn_area, NULL, hole_callback, NULL);
  /* Draw element Marks */
  if (PCB->PinOn)
    r_search (PCB->Data->element_tree, drawn_area, NULL, EMark_callback, NULL);
  /* Draw rat lines on top */
  if (PCB->RatOn)
    r_search (PCB->Data->rat_tree, drawn_area, NULL, rat_callback, NULL);
  if (Settings.DrawGrid)
    DrawGrid ();
}

static void
DrawEMark (LocationType X, LocationType Y, gboolean invisible)
{
  if (!PCB->InvisibleObjectsOn && invisible)
    return;
  gdk_gc_set_foreground(Output.fgGC,
		  invisible ? PCB->InvisibleMarkColor : PCB->ElementColor);
  gdk_gc_set_line_attributes(Output.fgGC, 1,
			GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
  XDrawCLine (DrawingWindow, Output.fgGC, X - EMARK_SIZE,
	      Y, X, Y - EMARK_SIZE);
  XDrawCLine (DrawingWindow, Output.fgGC, X + EMARK_SIZE,
	      Y, X, Y - EMARK_SIZE);
  XDrawCLine (DrawingWindow, Output.fgGC, X - EMARK_SIZE,
	      Y, X, Y + EMARK_SIZE);
  XDrawCLine (DrawingWindow, Output.fgGC, X + EMARK_SIZE,
	      Y, X, Y + EMARK_SIZE);
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
DrawTop (BoxType * screen)
{
  if (PCB->PinOn)
    {
      /* draw element pins */
      r_search (PCB->Data->pin_tree, screen, NULL, pin_callback, NULL);
      /* draw element pads */
      r_search (PCB->Data->pad_tree, screen, NULL, pad_callback, NULL);
    }
  /* draw vias */
  if (PCB->ViaOn)
    r_search (PCB->Data->via_tree, screen, NULL, via_callback, NULL);
}

struct pin_info
{
  gboolean arg;
  int PIPFlag;
  LayerTypePtr Layer;
};

static int
clearPin_callback (const BoxType * b, void *cl)
{
  PinTypePtr pin = (PinTypePtr) b;
  struct pin_info *i = (struct pin_info *) cl;
  if (i->arg)
    ClearOnlyPin (pin, True);
  else if (TEST_FLAG (i->PIPFlag, pin))
    ClearOnlyPin (pin, False);
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
 * draws solder mask layer - this will cover nearly everything
 */
static void
DrawMask (BoxType * screen)
	{
	struct pin_info	info;
	OutputType		*out = &Output;

	info.arg = True;
	gdk_gc_set_function(out->pmGC, GDK_COPY);

	/* fill whole map first */
	/* pmGC is depth 1, so just need a color with color.pixel = 1
	*/
	gdk_gc_set_foreground(Output.pmGC, &Settings.WhiteColor);
	gdk_gc_set_fill(out->pmGC, GDK_SOLID);
	gdk_draw_rectangle(out->mask, out->pmGC, TRUE, 0, 0, out->Width,
				out->Height);

	/* make clearances in mask */
	gdk_gc_set_function(out->pmGC, GDK_CLEAR);
	r_search (PCB->Data->pin_tree, screen, NULL, clearPin_callback, &info);
	r_search (PCB->Data->via_tree, screen, NULL, clearPin_callback, &info);
	r_search (PCB->Data->pad_tree, screen, NULL, clearPad_callback, &info);
 
	gdk_gc_set_clip_mask(out->fgGC, out->mask);

	/* now draw the mask */
	gdk_gc_set_foreground(out->fgGC, PCB->MaskColor);
	gdk_draw_rectangle(DrawingWindow, out->fgGC, TRUE, 0, 0,
			TO_DRAWABS_X (PCB->MaxWidth), TO_DRAWABS_Y (PCB->MaxHeight));

	/* restore the clip region */
	gdk_gc_set_clip_mask(out->fgGC, NULL);
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
      ClearLine (l);
      break;
    case ARC_TYPE:
      ClearArc (a);
      break;
    case PIN_TYPE:
    case VIA_TYPE:
      ClearOnlyPin ((PinTypePtr) ptr2, False);
      break;
    case PAD_TYPE:
      ClearPad ((PadTypePtr) ptr2, False);
      break;
    default:
      Message ("Bad clear callback\n");
    }
  return 0;
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

static int
therm_callback (const BoxType * b, void *cl)
{
  PinTypePtr pin = (PinTypePtr) b;
  struct pin_info *i = (struct pin_info *) cl;

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
DrawLayer (LayerTypePtr Layer, BoxType * screen)
	{
	gint			layernum = GetLayerNumber (PCB->Data, Layer);
	gint			group = GetLayerGroupNumberByNumber (layernum);
	struct pin_info	info;
	OutputType		*out = &Output;
	gint			PIPFlag = L0PIPFLAG << layernum;

	info.PIPFlag = PIPFlag;
	info.arg = False;
	info.Layer = Layer;

	/* in order to render polygons with line cut-outs, draw a solid
	|  (or stippled) 1-bit pixmap, then erase the clearance areas.
	|  Use that as the mask when drawing the actual polygons
	*/
	if (layernum < MAX_LAYER && Layer->PolygonN)
		{
		gdk_gc_set_function(Output.pmGC, GDK_COPY);
		if (Settings.StipplePolygons)
			{
			gdk_gc_set_background(out->pmGC, &Settings.BlackColor);
			gdk_gc_set_stipple(out->pmGC, Stipples[layernum]);
			gdk_gc_set_fill(out->pmGC, GDK_OPAQUE_STIPPLED);
			}
		/* fill whole map first
		*/
		gdk_draw_rectangle(out->mask, out->pmGC, TRUE, 0, 0, out->Width,
					out->Height);
		if (Settings.StipplePolygons)
			gdk_gc_set_fill(out->pmGC, GDK_SOLID);

		/* Make clearances around lines, arcs, pins and vias
		*/
		gdk_gc_set_function(out->pmGC, GDK_CLEAR);
		PolygonPlows (group, screen, clear_callback);
		gdk_gc_set_clip_mask(out->fgGC, out->mask);
		}
	if (Layer->PolygonN)
		{
		/* print the clearing polys */
		POLYGON_LOOP (Layer);
			{
			if (VPOLY (polygon) && TEST_FLAG (CLEARPOLYFLAG, polygon))
				DrawPlainPolygon (Layer, polygon);
			}
		END_LOOP;

		/* restore the clip region */
		gdk_gc_set_clip_mask(out->fgGC, NULL);

		/* print the non-clearing polys */
		POLYGON_LOOP (Layer);
			{
			if (VPOLY (polygon) && !TEST_FLAG (CLEARPOLYFLAG, polygon))
				DrawPlainPolygon (Layer, polygon);
			}
		END_LOOP;

		if (layernum < MAX_LAYER)
			{
			PIPFlag = (L0THERMFLAG | L0PIPFLAG) << layernum;
			info.PIPFlag = PIPFlag;
			r_search(PCB->Data->pin_tree, screen, NULL, therm_callback, &info);
			r_search(PCB->Data->via_tree, screen, NULL, therm_callback, &info);
			}
		}
	if (TEST_FLAG (CHECKPLANESFLAG, PCB))
		return;

	/* draw all visible lines this layer */
	r_search (Layer->line_tree, screen, NULL, line_callback, Layer);

	/* draw the layer arcs on screen */
	r_search (Layer->arc_tree, screen, NULL, arc_callback, Layer);

	/* draw the layer text on screen */
	r_search (Layer->text_tree, screen, NULL, text_callback, Layer);
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
DrawSpecialPolygon (GdkDrawable *d, GdkGC *DrawGC,
		    LocationType X, LocationType Y, gint Thickness)
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
  static GdkPoint scaled[8];
  GdkPoint polygon[9];
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
      gdk_gc_set_line_attributes(Output.fgGC, 1,
			  GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
      polygon[8].x = X + scaled[0].x;
      polygon[8].y = Y + scaled[0].y;
      gdk_draw_lines(d, DrawGC, polygon, 9);
    }
  else
    gdk_draw_polygon(d, DrawGC, TRUE, polygon, 8);
}

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for pins and vias
 */
static void
DrawPinOrViaLowLevel (PinTypePtr Ptr, gboolean drawHole)
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
	  gdk_gc_set_line_attributes(Output.bgGC, TO_SCREEN (Ptr->Thickness),
			      GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	  XDrawCLine (DrawingWindow, Output.bgGC, Ptr->X, Ptr->Y,
		      Ptr->X, Ptr->Y);
	  gdk_gc_set_line_attributes(Output.fgGC, 1, GDK_LINE_SOLID, GDK_CAP_ROUND,
			      GDK_JOIN_ROUND);
	  XDrawCArc (DrawingWindow, Output.fgGC, Ptr->X, Ptr->Y,
		     Ptr->Thickness, Ptr->Thickness, 0, 23040);
	}
      return;
    }
  if (TEST_FLAG (SQUAREFLAG, Ptr))
    {
      if (TEST_FLAG (THINDRAWFLAG, PCB))
	{
	  gdk_gc_set_line_attributes(Output.fgGC, 1,
			      GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	  gdk_draw_rectangle(DrawingWindow, Output.fgGC, FALSE,
			  TO_DRAW_X (Ptr->X - Ptr->Thickness / 2),
			  TO_DRAW_Y (Ptr->Y - TO_SCREEN_SIGN_Y (Ptr->Thickness / 2)),
			  TO_SCREEN (Ptr->Thickness),
			  TO_SCREEN (Ptr->Thickness));
	}
      else
	{
	  gdk_draw_rectangle(DrawingWindow, Output.fgGC, TRUE,
			  TO_DRAW_X (Ptr->X - Ptr->Thickness / 2),
			  TO_DRAW_Y (Ptr->Y - TO_SCREEN_SIGN_Y (Ptr->Thickness / 2)),
			  TO_SCREEN (Ptr->Thickness),
			  TO_SCREEN (Ptr->Thickness));
	}
    }
  else if (TEST_FLAG (OCTAGONFLAG, Ptr))
    {
      gdk_gc_set_line_attributes(Output.fgGC,
			  TO_SCREEN ((Ptr->Thickness - Ptr->DrillingHole) /
				     2), GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);

      /* transform X11 specific coord system */
      DrawSpecialPolygon (DrawingWindow, Output.fgGC,
			  TO_DRAW_X (Ptr->X), TO_DRAW_Y (Ptr->Y),
			  Ptr->Thickness);
    }
  else
    {				/* draw a round pin or via */
      if (TEST_FLAG (THINDRAWFLAG, PCB))
	{
	  gdk_gc_set_line_attributes(Output.fgGC, 1,
			      GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	  XDrawCArc (DrawingWindow, Output.fgGC, Ptr->X, Ptr->Y,
		     Ptr->Thickness, Ptr->Thickness, 0, 360 * 64);
	}
      else
	{
	  gdk_gc_set_line_attributes(Output.fgGC, TO_SCREEN (Ptr->Thickness),
			      GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	  XDrawCLine (DrawingWindow, Output.fgGC, Ptr->X, Ptr->Y,
		      Ptr->X, Ptr->Y);
	}
    }

  /* and the drilling hole  (which is always round */
  if (drawHole)
    {
      if (TEST_FLAG (THINDRAWFLAG, PCB))
	{
	  gdk_gc_set_line_attributes(Output.fgGC, 1,
			      GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	  XDrawCArc (DrawingWindow, Output.fgGC,
		     Ptr->X, Ptr->Y, Ptr->DrillingHole,
		     Ptr->DrillingHole, 0, 360 * 64);
	}
      else
	{
	  gdk_gc_set_line_attributes(Output.bgGC, TO_SCREEN (Ptr->DrillingHole),
			      GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	  XDrawCLine (DrawingWindow, Output.bgGC, Ptr->X, Ptr->Y,
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
	  gdk_gc_set_line_attributes(Output.fgGC, 1,
			      GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	  XDrawCArc (DrawingWindow, Output.fgGC, Ptr->X, Ptr->Y,
		     Ptr->DrillingHole, Ptr->DrillingHole, 0, 360 * 64);
	}
    }
  else
    {
      gdk_gc_set_line_attributes(Output.bgGC, TO_SCREEN (Ptr->DrillingHole),
			  GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
      XDrawCLine (DrawingWindow, Output.bgGC, Ptr->X,
		  Ptr->Y, Ptr->X, Ptr->Y);
    }
  if (TEST_FLAG (HOLEFLAG, Ptr))
    {
      if (TEST_FLAG (WARNFLAG, Ptr))
	gdk_gc_set_foreground(Output.fgGC, PCB->WarnColor);
      else if (TEST_FLAG (SELECTEDFLAG, Ptr))
	gdk_gc_set_foreground(Output.fgGC, PCB->ViaSelectedColor);
      else
	gdk_gc_set_foreground(Output.fgGC, &Settings.BlackColor);

      gdk_gc_set_line_attributes(Output.fgGC, 1, GDK_LINE_SOLID, GDK_CAP_ROUND,
			  GDK_JOIN_ROUND);
      XDrawCArc (DrawingWindow, Output.fgGC, Ptr->X, Ptr->Y,
		 Ptr->DrillingHole, Ptr->DrillingHole, 0, 23040);
    }
}

/*******************************************************************
 * draw clearance in pixmask arround pins and vias that pierce polygons
 */
static void
ClearOnlyPin (PinTypePtr Pin, gboolean mask)
{
  BDimension half =
    (mask ? Pin->Mask / 2 : (Pin->Thickness + Pin->Clearance) / 2);

  if (!mask && TEST_FLAG (HOLEFLAG, Pin))
    return;

  /* Clear the area around the pin */
  if (TEST_FLAG (SQUAREFLAG, Pin))
    {
      gdk_draw_rectangle(Output.mask, Output.pmGC, TRUE,
		      TO_DRAW_X (Pin->X - TO_SCREEN_SIGN_X (half)),
		      TO_DRAW_Y (Pin->Y - TO_SCREEN_SIGN_Y (half)),
		      TO_SCREEN (mask ? Pin->Mask : Pin->Thickness +
				 Pin->Clearance),
		      TO_SCREEN (mask ? Pin->Mask : Pin->Thickness +
				 Pin->Clearance));
    }
  else if (TEST_FLAG (OCTAGONFLAG, Pin))
    {
      gdk_gc_set_line_attributes(Output.pmGC,
			  TO_SCREEN ((Pin->Thickness + Pin->Clearance -
				      Pin->DrillingHole) / 2), GDK_LINE_SOLID,
			  GDK_CAP_ROUND, GDK_JOIN_ROUND);

      /* transform X11 specific coord system */
      DrawSpecialPolygon (Output.mask, Output.pmGC,
			  TO_DRAW_X (Pin->X), TO_DRAW_Y (Pin->Y),
			  mask ? Pin->Mask : Pin->Thickness + Pin->Clearance);
    }
  else
    {
      gdk_gc_set_line_attributes(Output.pmGC,
			  TO_SCREEN (mask ? Pin->Mask : Pin->Thickness +
				     Pin->Clearance), GDK_LINE_SOLID, GDK_CAP_ROUND,
			  GDK_JOIN_ROUND);
      XDrawCLine (Output.mask, Output.pmGC, Pin->X, Pin->Y, Pin->X, Pin->Y);
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
	gdk_gc_set_foreground(Output.fgGC, layer->SelectedColor);
      else
	gdk_gc_set_foreground(Output.fgGC, PCB->ConnectedColor);
    }
  else
    gdk_gc_set_foreground(Output.fgGC, layer->Color);

  finger = (Pin->Thickness - Pin->DrillingHole) * PCB->ThermScale;
  gdk_gc_set_line_attributes(Output.fgGC,
		      TEST_FLAG (THINDRAWFLAG, PCB) ? 1 : TO_SCREEN (finger),
		      GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
  if (TEST_FLAG (SQUAREFLAG, Pin))
    {

      XDrawCLine (DrawingWindow, Output.fgGC,
		  Pin->X - half, Pin->Y - half, Pin->X + half, Pin->Y + half);
      XDrawCLine (DrawingWindow, Output.fgGC,
		  Pin->X - half, Pin->Y + half, Pin->X + half, Pin->Y - half);
    }
  else
    {
      BDimension halfs = (half * M_SQRT1_2 + 1);

      XDrawCLine (DrawingWindow, Output.fgGC,
		  Pin->X - halfs, Pin->Y - halfs, Pin->X + halfs,
		  Pin->Y + halfs);
      XDrawCLine (DrawingWindow, Output.fgGC, Pin->X - halfs,
		  Pin->Y + halfs, Pin->X + halfs, Pin->Y - halfs);
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
      gdk_draw_rectangle(DrawingWindow, Output.bgGC, TRUE,
		      TO_DRAW_X (Pin->X - TO_SCREEN_SIGN_X (half)),
		      TO_DRAW_Y (Pin->Y - TO_SCREEN_SIGN_Y (half)),
		      TO_SCREEN (Pin->Thickness + Pin->Clearance),
		      TO_SCREEN (Pin->Thickness + Pin->Clearance));
    }
  else if (TEST_FLAG (OCTAGONFLAG, Pin))
    {
      gdk_gc_set_line_attributes(Output.bgGC,
			  TO_SCREEN ((Pin->Thickness + Pin->Clearance -
				      Pin->DrillingHole) / 2), GDK_LINE_SOLID,
			  GDK_CAP_ROUND, GDK_JOIN_ROUND);

      /* transform X11 specific coord system */
      DrawSpecialPolygon (DrawingWindow, Output.bgGC,
			  TO_DRAW_X (Pin->X), TO_DRAW_Y (Pin->Y),
			  Pin->Thickness + Pin->Clearance);
    }
  else
    {
      gdk_gc_set_line_attributes(Output.bgGC,
			  TO_SCREEN (Pin->Thickness + Pin->Clearance),
			  GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
      XDrawCLine (DrawingWindow, Output.bgGC, Pin->X,
		  Pin->Y, Pin->X, Pin->Y);
    }
  /* draw all the thermal(s) */
  for (i = MAX_LAYER; i; i--)
    {
      layer = LAYER_ON_STACK (i - 1);
      if (!layer->On)
	continue;
      ThermLayerFlag =
	(L0THERMFLAG | L0PIPFLAG) << GetLayerNumber (PCB->Data, layer);
      if (TEST_FLAGS (ThermLayerFlag, Pin))
	{
	  if (!Erasing)
	    {
	      if (TEST_FLAG (SELECTEDFLAG | FOUNDFLAG, Pin))
		{
		  if (TEST_FLAG (SELECTEDFLAG, Pin))
		    gdk_gc_set_foreground(Output.fgGC, layer->SelectedColor);
		  else
		    gdk_gc_set_foreground(Output.fgGC, PCB->ConnectedColor);
		}
	      else
		gdk_gc_set_foreground(Output.fgGC, layer->Color);
	    }
	  if (TEST_FLAG (SQUAREFLAG, Pin))
	    {
	      gdk_gc_set_line_attributes(Output.fgGC,
				  TEST_FLAG (THINDRAWFLAG,
					     PCB) ? 1 : TO_SCREEN (Pin->
								   Clearance /
								   2),
				  GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);

	      XDrawCLine (DrawingWindow, Output.fgGC, Pin->X - half,
			  Pin->Y - half, Pin->X + half, Pin->Y + half);
	      XDrawCLine (DrawingWindow, Output.fgGC, Pin->X - half,
			  Pin->Y + half, Pin->X + half, Pin->Y - half);
	    }
	  else
	    {
	      BDimension halfs = (half * M_SQRT1_2 + 1);

	      gdk_gc_set_line_attributes(Output.fgGC,
				  TO_SCREEN (Pin->Clearance / 2), GDK_LINE_SOLID,
				  GDK_CAP_ROUND, GDK_JOIN_ROUND);
	      XDrawCLine (DrawingWindow, Output.fgGC, Pin->X - halfs,
			  Pin->Y - halfs, Pin->X + halfs, Pin->Y + halfs);
	      XDrawCLine (DrawingWindow, Output.fgGC, Pin->X - halfs,
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


/* vertical text handling provided by Martin Devera with fixes by harry eaton */

/* draw vertical text; xywh is bounding, de is text's descend used for
   positioning */
static void
DrawVText (int x, int y, int w, int h, char *str)
	{
	GdkPixmap	*pm;
	GdkImage	*im;
	GdkGCValues	values;
	guint32		pixel;
	gint		i, j;

	if (!str || !*str)
		return;

	pm = gdk_pixmap_new(DrawingWindow, w, h, -1);

	/* draw into pixmap */
	gdk_draw_rectangle(pm, Output.bgGC, TRUE, 0, 0, w, h);

	gui_draw_string_markup(DrawingWindow, Output.font_desc, Output.fgGC,
			0, 0, str);

	im = gdk_drawable_get_image(pm, 0, 0, w, h);
	gdk_gc_get_values(Output.fgGC, &values);

	/* draw Transpose(im).  TODO: Pango should be doing vertical text soon */
	for (i = 0; i < w; i++)
		for (j = 0; j < h; j++)
			{
			pixel = gdk_image_get_pixel(im, i, j);
			if (pixel == values.foreground.pixel)
				gdk_draw_point(DrawingWindow, Output.fgGC,
						x + j, y + w - i - 1);
			}
	g_object_unref(G_OBJECT(pm));
	}


/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for pin and via names
 */
static void
DrawPinOrViaNameLowLevel (PinTypePtr Ptr)
{
  gint		width, height;
  gchar		*name;
  BoxType	box;
  gboolean	vert;

  name = EMPTY (TEST_FLAG (SHOWNUMBERFLAG, PCB) ? Ptr->Number : Ptr->Name);
  name = markup_sized_string(name);

  gui_string_markup_extents(Output.font_desc, name, &width, &height);

#if VERTICAL_TEXT
  vert = TEST_FLAG (EDGE2FLAG, Ptr);
#else
  vert = FALSE;
#endif

  if (vert)
    {
      box.X1 = TO_DRAW_X (Ptr->X) - height;
      box.Y1 =
	TO_DRAW_Y (Ptr->Y + Ptr->Thickness / 2 + Settings.PinoutTextOffsetY);
    }
  else
    {
      box.X1 =
	TO_DRAW_X (Ptr->X + Ptr->Thickness / 2 + Settings.PinoutTextOffsetX);
      box.Y1 = TO_DRAW_Y (Ptr->Y) - height / 2;
    }
  if (Gathering)
    {
      if (vert)
	{
	  box.X2 = box.X1 + height;
	  box.Y2 = box.Y1 + width;
	}
      else
	{
	  box.X2 = box.X1 + width;
	  box.Y2 = box.Y1 + height;
	}
/*printf("AddPart: x1=%d y1=%d x2=%d y2=%d\n", box.X1, box.Y1, box.X2, box.Y2);*/
      AddPart (&box, True);
      return;
    }
/*printf("DrawPin(%d,%d): x=%d y=%d w=%d h=%d\n",
  TO_DRAW_X(Ptr->X), TO_DRAW_Y(Ptr->Y), box.X1, box.Y1, width, height);*/

  if (vert)
    DrawVText (box.X1, box.Y1, width, height, name);
  else
    gui_draw_string_markup(DrawingWindow, Output.font_desc, Output.fgGC,
		 box.X1, box.Y1, name);
  g_free(name);
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
      gdk_gc_set_line_attributes(Output.fgGC,
			  1, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
      if (TEST_FLAG (SQUAREFLAG, Pad))
	{
	  x1 -= t;
	  y1 -= t;
	  x2 += t2;
	  y2 += t2;
	  XDrawCLine (DrawingWindow, Output.fgGC, x1, y1, x1, y2);
	  XDrawCLine (DrawingWindow, Output.fgGC, x1, y2, x2, y2);
	  XDrawCLine (DrawingWindow, Output.fgGC, x2, y2, x2, y1);
	  XDrawCLine (DrawingWindow, Output.fgGC, x2, y1, x1, y1);
	}
      else if (x1 == x2)
	{
	  XDrawCLine (DrawingWindow, Output.fgGC, x1 - t, y1, x2 - t,
		      y2);
	  XDrawCLine (DrawingWindow, Output.fgGC, x1 + t2, y1, x2 + t2,
		      y2);
	  XDrawCArc (DrawingWindow, Output.fgGC, x1, y1, Pad->Thickness,
		     Pad->Thickness, 0, 180 * 64);
	  XDrawCArc (DrawingWindow, Output.fgGC, x2, y2, Pad->Thickness,
		     Pad->Thickness, 180 * 64, 180 * 64);
	}
      else
	{
	  XDrawCLine (DrawingWindow, Output.fgGC, x1, y1 - t, x2,
		      y2 - t);
	  XDrawCLine (DrawingWindow, Output.fgGC, x1, y1 + t2, x2,
		      y2 + t2);
	  XDrawCArc (DrawingWindow, Output.fgGC, x1, y1, Pad->Thickness,
		     Pad->Thickness, 90 * 64, 180 * 64);
	  XDrawCArc (DrawingWindow, Output.fgGC, x2, y2, Pad->Thickness,
		     Pad->Thickness, 270 * 64, 180 * 64);
	}
    }
  else
    {

      gdk_gc_set_line_attributes(Output.fgGC,
			  TO_SCREEN (Pad->Thickness), GDK_LINE_SOLID,
			  TEST_FLAG (SQUAREFLAG, Pad) ? GDK_CAP_PROJECTING : GDK_CAP_ROUND,
			  GDK_JOIN_ROUND);
      XDrawCLine (DrawingWindow, Output.fgGC, Pad->Point1.X,
		  Pad->Point1.Y, Pad->Point2.X, Pad->Point2.Y);
    }
}

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for pad names
 */

static void
DrawPadNameLowLevel (PadTypePtr Pad)
{
  BoxType	box;
  gchar		*name;
  gint		width, height;
  gboolean	vert;

  name = EMPTY (TEST_FLAG (SHOWNUMBERFLAG, PCB) ? Pad->Number : Pad->Name);
  name = markup_sized_string(name);

  gui_string_markup_extents(Output.font_desc, name, &width, &height);

  /* should text be vertical ? */
#if VERTICAL_TEXT
  vert = (Pad->Point1.X == Pad->Point2.X);
#else
  vert = FALSE;
#endif

  if (vert)
    {
      box.X1 = TO_DRAW_X (Pad->Point1.X) - height;
      box.Y1 =
	TO_DRAW_Y (MAX (Pad->Point1.Y, Pad->Point2.Y) + Pad->Thickness / 2);
    }
  else
    {
      box.X1 =
	TO_DRAW_X (MAX (Pad->Point1.X, Pad->Point2.X) + Pad->Thickness / 2);
      box.Y1 = TO_DRAW_Y (Pad->Point1.Y) -  height / 2;
    }

  if (vert)
    box.Y1 +=  TO_SCREEN (Settings.PinoutTextOffsetY);
  else
    box.X1 += TO_SCREEN (Settings.PinoutTextOffsetX);

  if (Gathering)
    {
      if (vert)
	{
	  box.X2 = box.X1 + height;
	  box.Y2 = box.Y1 + width;
	}
      else
	{
	  box.X2 = box.X1 + width;
	  box.Y2 = box.Y1 + height;
	}
      AddPart (&box, True);
      return;
    }

#if VERTICAL_TEXT
  if (vert)
   DrawVText (box.X1, box.Y1, width, height, name);
  else
#endif
    gui_draw_string_markup(DrawingWindow, Output.font_desc, Output.fgGC,
		 box.X1, box.Y1, name);
  g_free(name);
}

/* ---------------------------------------------------------------------------
 * clearance for pads
 */
static void
ClearPad (PadTypePtr Pad, gboolean mask)
{
  gdk_gc_set_line_attributes(Output.pmGC,
		      TO_SCREEN (mask ? Pad->Mask : Pad->Thickness +
				 Pad->Clearance), GDK_LINE_SOLID,
		      TEST_FLAG (SQUAREFLAG, Pad) ? GDK_CAP_PROJECTING : GDK_CAP_ROUND,
		      GDK_JOIN_ROUND);
  XDrawCLine (Output.mask, Output.pmGC, Pad->Point1.X, Pad->Point1.Y,
	      Pad->Point2.X, Pad->Point2.Y);
}

/* ---------------------------------------------------------------------------
 * clearance for lines
 */
static void
ClearLine (LineTypePtr Line)
{
  gdk_gc_set_line_attributes(Output.pmGC,
		      TO_SCREEN (Line->Clearance + Line->Thickness),
		      GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
  XDrawCLine (Output.mask, Output.pmGC, Line->Point1.X, Line->Point1.Y,
	      Line->Point2.X, Line->Point2.Y);
}

/* ---------------------------------------------------------------------------
 * clearance for arcs 
 */
static void
ClearArc (ArcTypePtr Arc)
{
  gdk_gc_set_line_attributes(Output.pmGC,
		      TO_SCREEN (Arc->Thickness + Arc->Clearance),
		      GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
  XDrawCArc (Output.mask, Output.pmGC, Arc->X, Arc->Y,
	     2 * Arc->Width, 2 * Arc->Height,
	     (Arc->StartAngle - 180) * 64, Arc->Delta * 64);
}

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for lines
 */
static void
DrawLineLowLevel (LineTypePtr Line, gboolean HaveGathered)
{
  if (Gathering && !HaveGathered)
    {
      AddPart (Line, False);
      return;
    }

  if (TEST_FLAG (THINDRAWFLAG, PCB))
    gdk_gc_set_line_attributes(Output.fgGC, 1, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
  else
    gdk_gc_set_line_attributes(Output.fgGC,
			TO_SCREEN (Line->Thickness), GDK_LINE_SOLID, GDK_CAP_ROUND,
			GDK_JOIN_ROUND);

  if (TEST_FLAG (RATFLAG, Line))
    {
      gdk_gc_set_stipple(Output.fgGC, Stipples[0]);
      gdk_gc_set_fill(Output.fgGC, GDK_STIPPLED);
      XDrawCLine (DrawingWindow, Output.fgGC,
		  Line->Point1.X, Line->Point1.Y,
		  Line->Point2.X, Line->Point2.Y);
      gdk_gc_set_fill(Output.fgGC, GDK_SOLID);
    }
  else
    XDrawCLine (DrawingWindow, Output.fgGC,
		Line->Point1.X, Line->Point1.Y,
		Line->Point2.X, Line->Point2.Y);
}

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for text objects
 */
static void
DrawTextLowLevel (TextTypePtr Text)
{
  LocationType x = 0;
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
	  LocationType size = (defaultsymbol.X2 - defaultsymbol.X1) * 6 / 5;

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
	  gdk_draw_rectangle(DrawingWindow, Output.fgGC, TRUE,
			  TO_DRAW_X (defaultsymbol.X1),
			  TO_DRAW_Y (SWAP_IDENT ? defaultsymbol.Y2 : defaultsymbol.Y1),
			  TO_SCREEN (abs(defaultsymbol.X2 - defaultsymbol.X1)),
			  TO_SCREEN (abs(defaultsymbol.Y2 - defaultsymbol.Y1)));

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
    gdk_gc_set_line_attributes(Output.fgGC, 1, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
  else
    gdk_gc_set_line_attributes(Output.fgGC,
			TO_SCREEN (Arc->Thickness), GDK_LINE_SOLID, GDK_CAP_ROUND,
			GDK_JOIN_ROUND);

  XDrawCArc (DrawingWindow, Output.fgGC, Arc->X, Arc->Y, 2 * Arc->Width,
	     2 * Arc->Height, (Arc->StartAngle - 180) * 64, Arc->Delta * 64);
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
DrawPlainVia (PinTypePtr Via, gboolean holeToo)
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
	gdk_gc_set_foreground(Output.fgGC, PCB->ViaSelectedColor);
      else
	gdk_gc_set_foreground(Output.fgGC, PCB->ViaColor);
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
DrawPlainPin (PinTypePtr Pin, gboolean holeToo)
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
	gdk_gc_set_foreground(Output.fgGC, PCB->PinSelectedColor);
      else
	gdk_gc_set_foreground(Output.fgGC, PCB->PinColor);
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
	    gdk_gc_set_foreground(Output.fgGC, PCB->WarnColor);
	  else if (TEST_FLAG (SELECTEDFLAG, Pad))
	    gdk_gc_set_foreground(Output.fgGC, PCB->PinSelectedColor);
	  else
	    gdk_gc_set_foreground(Output.fgGC, PCB->ConnectedColor);
	}
      else if (FRONT (Pad))
	gdk_gc_set_foreground(Output.fgGC, PCB->PinColor);
      else
	gdk_gc_set_foreground(Output.fgGC, PCB->InvisibleObjectsColor);
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
	gdk_gc_set_foreground(Output.fgGC, PCB->PinSelectedColor);
      else if (FRONT (Pad))
	gdk_gc_set_foreground(Output.fgGC, PCB->PinColor);
      else
	gdk_gc_set_foreground(Output.fgGC, PCB->InvisibleObjectsColor);
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
	    gdk_gc_set_foreground(Output.fgGC, Layer->SelectedColor);
	  else
	    gdk_gc_set_foreground(Output.fgGC, PCB->ConnectedColor);
	}
      else
	gdk_gc_set_foreground(Output.fgGC, Layer->Color);
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
	    gdk_gc_set_foreground(Output.fgGC, PCB->RatSelectedColor);
	  else
	    gdk_gc_set_foreground(Output.fgGC, PCB->ConnectedColor);
	}
      else
	gdk_gc_set_foreground(Output.fgGC, PCB->RatColor);
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
	    gdk_gc_set_foreground(Output.fgGC, Layer->SelectedColor);
	  else
	    gdk_gc_set_foreground(Output.fgGC, PCB->ConnectedColor);
	}
      else
	gdk_gc_set_foreground(Output.fgGC, Layer->Color);
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
    gdk_gc_set_foreground(Output.fgGC, Layer->SelectedColor);
  else
    gdk_gc_set_foreground(Output.fgGC, Layer->Color);
  DrawTextLowLevel (Text);
}

/* ---------------------------------------------------------------------------
 * draws text on a layer - assumes it's not on silkscreen
 */
static void
DrawRegularText (LayerTypePtr Layer, TextTypePtr Text, int unused)
{
  if (TEST_FLAG (SELECTEDFLAG, Text))
    gdk_gc_set_foreground(Output.fgGC, Layer->SelectedColor);
  else
    gdk_gc_set_foreground(Output.fgGC, Layer->Color);
  DrawTextLowLevel (Text);
}

static int
cp_callback (const BoxType * b, void *cl)
{
  ClearPin ((PinTypePtr) b, (int) cl, 0);
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
	gdk_gc_set_foreground(Output.fgGC, Layer->SelectedColor);
      else
	gdk_gc_set_foreground(Output.fgGC, PCB->ConnectedColor);
    }
  else
    gdk_gc_set_foreground(Output.fgGC, Layer->Color);
  layernum = GetLayerNumber (PCB->Data, Layer);
  if (Settings.StipplePolygons)
    {
      gdk_gc_set_stipple(Output.fgGC, Stipples[layernum]);
      gdk_gc_set_fill(Output.fgGC, GDK_STIPPLED);
    }
  DrawPolygonLowLevel (Polygon);
  if (Settings.StipplePolygons)
    gdk_gc_set_fill(Output.fgGC, GDK_SOLID);
  if (TEST_FLAG (CLEARPOLYFLAG, Polygon))
    {
      r_search (PCB->Data->pin_tree, &Polygon->BoundingBox, NULL,
		cp_callback, (void *) PIN_TYPE);
      r_search (PCB->Data->via_tree, &Polygon->BoundingBox, NULL,
		cp_callback, (void *) VIA_TYPE);
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
	gdk_gc_set_foreground(Output.fgGC, Layer->SelectedColor);
      else
	gdk_gc_set_foreground(Output.fgGC, PCB->ConnectedColor);
    }
  else
    gdk_gc_set_foreground(Output.fgGC, Layer->Color);
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
    gdk_gc_set_foreground(Output.fgGC, PCB->ElementSelectedColor);
  else if (FRONT (Element))
    gdk_gc_set_foreground(Output.fgGC, PCB->ElementColor);
  else
    gdk_gc_set_foreground(Output.fgGC, PCB->InvisibleObjectsColor);
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
    gdk_gc_set_foreground(Output.fgGC, PCB->ElementSelectedColor);
  else if (FRONT (Element))
    gdk_gc_set_foreground(Output.fgGC, PCB->ElementColor);
  else
    gdk_gc_set_foreground(Output.fgGC, PCB->InvisibleObjectsColor);
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
    if (FRONT (pad) || PCB->InvisibleObjectsOn)
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
  if (TEST_FLAG (ALLPIPFLAGS, Via))
    ClearPin (Via, NO_TYPE, 0);
  gdk_gc_set_foreground(Output.fgGC, &Settings.BackgroundColor);
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
  gdk_gc_set_foreground(Output.fgGC, &Settings.BackgroundColor);
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
  gdk_gc_set_foreground(Output.fgGC, &Settings.BackgroundColor);
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
  gdk_gc_set_foreground(Output.fgGC, &Settings.BackgroundColor);
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
  gdk_gc_set_foreground(Output.fgGC, &Settings.BackgroundColor);
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
  gdk_gc_set_foreground(Output.fgGC, &Settings.BackgroundColor);
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
  gdk_gc_set_foreground(Output.fgGC, &Settings.BackgroundColor);
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
  gdk_gc_set_foreground(Output.fgGC, &Settings.BackgroundColor);
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
  gdk_gc_set_foreground(Output.fgGC, &Settings.BackgroundColor);
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
  gdk_gc_set_foreground(Output.fgGC, &Settings.BackgroundColor);
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
  gdk_gc_set_foreground(Output.fgGC, &Settings.BackgroundColor);
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
  gdk_gc_set_foreground(Output.fgGC, &Settings.BackgroundColor);
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
  gdk_gc_set_foreground(Output.fgGC, &Settings.BackgroundColor);
  PIN_LOOP (Element);
  {
    if (TEST_FLAG (ALLPIPFLAGS, pin))
      {
	ClearPin (pin, NO_TYPE, 0);
	gdk_gc_set_foreground(Output.fgGC, &Settings.BackgroundColor);
      }
    DrawPinOrViaLowLevel (pin, False);
    if (TEST_FLAG (DISPLAYNAMEFLAG, pin))
      DrawPinOrViaNameLowLevel (pin);
  }
  END_LOOP;
  PAD_LOOP (Element);
  {
    DrawPadLowLevel (pad);
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
  gdk_gc_set_foreground(Output.fgGC, &Settings.BackgroundColor);
  DrawTextLowLevel (&ELEMENT_TEXT (PCB, Element));
  Erasing--;
}

/* ---------------------------------------------------------------------------
 * draws grid points if the distance is >= MIN_GRID_DISTANCE
 */
static void
DrawGrid ()
{
  LocationType minx, miny, maxx, maxy, temp;
  gfloat x, y, delta;

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
	  gdk_draw_point(DrawingWindow,
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
