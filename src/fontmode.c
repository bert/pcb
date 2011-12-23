/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2006 DJ Delorie
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
 *  DJ Delorie, 334 North Road, Deerfield NH 03037-1110, USA
 *  dj@delorie.com
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"

#include <math.h>
#include <memory.h>
#include <limits.h>


#include "create.h"
#include "data.h"
#include "draw.h"
#include "misc.h"
#include "move.h"
#include "remove.h"
#include "rtree.h"
#include "strflags.h"
#include "undo.h"
#include "pcb-printf.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* FIXME - we currently hardcode the grid and PCB size.  What we
   should do in the future is scan the font for its extents, and size
   the grid appropriately.  Also, when we convert back to a font, we
   should search the grid for the gridlines and use them to figure out
   where the symbols are. */

#define CELL_SIZE	MIL_TO_COORD (100)
#define CELL_OFFSET	MIL_TO_COORD (10)

#define XYtoSym(x,y) ((x1 + CELL_OFFSET) / CELL_SIZE - 1 \
		      + 16 * ((y1 + CELL_OFFSET) / CELL_SIZE - 1))

static const char fontedit_syntax[] = "FontEdit()";

static const char fontedit_help[] =
  "Convert the current font to a PCB for editing.";

/* %start-doc actions FontEdit

%end-doc */

static int
FontEdit (int argc, char **argv, Coord Ux, Coord Uy)
{
  FontType *font;
  SymbolType *symbol;
  LayerTypePtr lfont, lorig, lwidth, lgrid;
  int s, l;

  if (hid_actionl ("New", "Font", 0))
    return 1;

  Settings.grid_unit = get_unit_struct("mil");
  Settings.Bloat = PCB->Bloat = 1;
  Settings.Shrink = PCB->Shrink = 1;
  Settings.minWid = PCB->minWid = 1;
  Settings.minSlk = PCB->minSlk = 1;

  MoveLayerToGroup (max_copper_layer + COMPONENT_LAYER, 0);
  MoveLayerToGroup (max_copper_layer + SOLDER_LAYER, 1);

  while (PCB->Data->LayerN > 4)
    MoveLayer (4, -1);
  for (l = 0; l < 4; l++)
    {
      MoveLayerToGroup (l, l);
    }
  PCB->MaxWidth = CELL_SIZE * 18;
  PCB->MaxHeight = CELL_SIZE * ((MAX_FONTPOSITION + 15) / 16 + 2);
  PCB->Grid = MIL_TO_COORD (5);
  PCB->Data->Layer[0].Name = strdup ("Font");
  PCB->Data->Layer[1].Name = strdup ("OrigFont");
  PCB->Data->Layer[2].Name = strdup ("Width");
  PCB->Data->Layer[3].Name = strdup ("Grid");
  hid_action ("PCBChanged");
  hid_action ("LayersChanged");

  lfont = PCB->Data->Layer + 0;
  lorig = PCB->Data->Layer + 1;
  lwidth = PCB->Data->Layer + 2;
  lgrid = PCB->Data->Layer + 3;

  font = &PCB->Font;
  for (s = 0; s <= MAX_FONTPOSITION; s++)
    {
      Coord ox = (s % 16 + 1) * CELL_SIZE;
      Coord oy = (s / 16 + 1) * CELL_SIZE;
      Coord w, miny, maxy, maxx = 0;

      symbol = &font->Symbol[s];

      miny = MIL_TO_COORD (5);
      maxy = font->MaxHeight;

      for (l = 0; l < symbol->LineN; l++)
	{
	  CreateDrawnLineOnLayer (lfont,
				  symbol->Line[l].Point1.X + ox,
				  symbol->Line[l].Point1.Y + oy,
				  symbol->Line[l].Point2.X + ox,
				  symbol->Line[l].Point2.Y + oy,
				  symbol->Line[l].Thickness,
				  symbol->Line[l].Thickness, NoFlags ());
	  CreateDrawnLineOnLayer (lorig, symbol->Line[l].Point1.X + ox,
				  symbol->Line[l].Point1.Y + oy,
				  symbol->Line[l].Point2.X + ox,
				  symbol->Line[l].Point2.Y + oy,
				  symbol->Line[l].Thickness,
				  symbol->Line[l].Thickness, NoFlags ());
	  if (maxx < symbol->Line[l].Point1.X)
	    maxx = symbol->Line[l].Point1.X;
	  if (maxx < symbol->Line[l].Point2.X)
	    maxx = symbol->Line[l].Point2.X;
	}
      w = maxx + symbol->Delta + ox;
      CreateDrawnLineOnLayer (lwidth,
			      w, miny + oy,
			      w, maxy + oy, MIL_TO_COORD (1), MIL_TO_COORD (1), NoFlags ());
    }

  for (l = 0; l < 16; l++)
    {
      int x = (l + 1) * CELL_SIZE;
      CreateDrawnLineOnLayer (lgrid, x, 0, x, PCB->MaxHeight, MIL_TO_COORD (1),
                              MIL_TO_COORD (1), NoFlags ());
    }
  for (l = 0; l <= MAX_FONTPOSITION / 16 + 1; l++)
    {
      int y = (l + 1) * CELL_SIZE;
      CreateDrawnLineOnLayer (lgrid, 0, y, PCB->MaxWidth, y, MIL_TO_COORD (1),
                              MIL_TO_COORD (1), NoFlags ());
    }
  return 0;
}

static const char fontsave_syntax[] = "FontSave()";

static const char fontsave_help[] = "Convert the current PCB back to a font.";

/* %start-doc actions FontSave

%end-doc */

static int
FontSave (int argc, char **argv, Coord Ux, Coord Uy)
{
  FontTypePtr font;
  SymbolTypePtr symbol;
  int i;
  GList *ii;
  LayerTypePtr lfont, lwidth;

  font = &PCB->Font;
  lfont = PCB->Data->Layer + 0;
  lwidth = PCB->Data->Layer + 2;

  for (i = 0; i <= MAX_FONTPOSITION; i++)
    {
      font->Symbol[i].LineN = 0;
      font->Symbol[i].Valid = 0;
      font->Symbol[i].Width = 0;
    }

  for (ii = lfont->Line; ii != NULL; ii = g_list_next (ii))
    {
      LineType *l = ii->data;
      int x1 = l->Point1.X;
      int y1 = l->Point1.Y;
      int x2 = l->Point2.X;
      int y2 = l->Point2.Y;
      int ox, oy, s;

      s = XYtoSym (x1, y1);
      ox = (s % 16 + 1) * CELL_SIZE;
      oy = (s / 16 + 1) * CELL_SIZE;
      symbol = &PCB->Font.Symbol[s];

      x1 -= ox;
      y1 -= oy;
      x2 -= ox;
      y2 -= oy;

      if (symbol->Width < x1)
	symbol->Width = x1;
      if (symbol->Width < x2)
	symbol->Width = x2;
      symbol->Valid = 1;

      CreateNewLineInSymbol (symbol, x1, y1, x2, y2, l->Thickness);
    }

  for (ii = lwidth->Line; ii != NULL; ii = g_list_next (ii))
    {
      LineType *l = ii->data;
      Coord x1 = l->Point1.X;
      Coord y1 = l->Point1.Y;
      Coord ox, s;

      s = XYtoSym (x1, y1);
      ox = (s % 16 + 1) * CELL_SIZE;
      symbol = &PCB->Font.Symbol[s];

      x1 -= ox;

      symbol->Delta = x1 - symbol->Width;
    }

  SetFontInfo (font);
  
  return 0;
}

HID_Action fontmode_action_list[] = {
  {"FontEdit", 0, FontEdit,
   fontedit_help, fontedit_syntax},
  {"FontSave", 0, FontSave,
   fontsave_help, fontsave_syntax}
};

REGISTER_ACTIONS (fontmode_action_list)
