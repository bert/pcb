/*!
 * \file src/font.c
 *
 * \brief Functions for loading and manipulating fonts
 *
 * \TODO Some of the user actions are very similar and may be able to be
 *       consolidated.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2006 DJ Delorie
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Contact addresses for paper mail and Email:
 * DJ Delorie, 334 North Road, Deerfield NH 03037-1110, USA
 * dj@delorie.com
 */

#include <glib.h>       // GSList type and functions
#include <string.h>     // memset
#include <stdio.h>      // asprintf

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"     // PCB type definitions

#include "font.h"

#include "create.h"     // CreateNewLineInSymbol
#include "const.h"      // MIL_TO_COORD
#include "data.h"       // Settings, PCB
#include "error.h"      // Message
#include "misc.h"       // MoveLayerToGroup, NoFlags, SetFontInfo
#include "move.h"       // MoveLayer
#include "hid.h"        // hid_action, hid_actionl
#include "parse_l.h"    // ParseFont
#include "pcb-printf.h" // get_unit_struct

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

int
check_font_name(FontType * font, char * name)
{
    return g_strcmp0(font->Name, name);
}
/*!
 * \brief Finds a font by name in the system font library
 */
FontType *
FindFont(GSList * library, char * fontname)
{
    GSList * font = g_slist_find_custom(library,
                                        fontname,
                                        (GCompareFunc)check_font_name);
    if (font) return font->data;
    return NULL;
}

/*!
 * \brief Finds a font by name in the system font library
 */
FontType *
ChangeFont(char * fontname)
{
    FontType * font;
    bool embedded = false;
    /* check the embedded library first, these should always have priority */
    font = FindFont(PCB->FontLibrary, fontname);
    if (font) embedded = true;
    /* if the font isn't there, check the system library. */
    if (!font) font = FindFont(Settings.FontLibrary, fontname);
    if (!font)
    {
        Message(_("Could not change font to %s because it isn't in the library\n"
               "Leaving the font set to %s.\n"), fontname, Settings.Font->Name);
        return NULL;
    }
    Message(_("Switching to font %s from the %s library\n"),
            fontname, embedded ? "embedded" : "system");
    Settings.Font = font;
    return font;
}

/*
 * \brief Loads a new font from a file
 */
FontType *
LoadFont(char * filename)
{
    // This will successfully load and install the new font, but it will change
    // the font of all text on the board. Text that was already there also changes
    // to the new font the next time the screen is redrawn.
    FontType * newfont;
    if (FindFont(Settings.FontLibrary, filename)) {
        Message(_("Font %s already loaded. Switching to it.\n"), filename);
        return ChangeFont(filename);
    }
    newfont = g_new(FontType, 1);
    memset(newfont->Symbol, 0, sizeof(newfont->Symbol));
    if (ParseFont(newfont, filename)){
        Message(_("Failed to load font file '%s'\n"), filename);
        g_free(newfont);
        return NULL;
    }
    Settings.Font = newfont;
    Settings.Font->Name = g_strdup(filename);
    Settings.FontLibrary = g_slist_append(Settings.FontLibrary, newfont);
    Message(_("New font loaded: %s\n"), Settings.Font->Name);
    return Settings.Font;
}

/*
 * /brief Creates a new empty font structure in the indicated library
 *
 * /todo add a "name" argument
 */
FontType *
CreateNewFontInLibrary(GSList ** library, char * name){
    FontType * newfont = g_new(FontType, 1);
    if (name) newfont->Name = g_strdup(name);
    else asprintf(&newfont->Name, "Font %d", g_slist_length(*library));
    memset(newfont->Symbol, 0, sizeof(newfont->Symbol));
    newfont->Valid = false;
    *library = g_slist_append(*library, newfont);
    return newfont;
}

int
FreeFontMemory(FontType * font)
{
    int i;
    Message(_("Freeing font memory: %s\n"), font->Name);
    /* Free any lines allocated to font symbols */
    for (i = 0; i <= MAX_FONTPOSITION; i++) free (font->Symbol[i].Line);
    /* Release the memory holding the font name */
    free(font->Name);
    /* Free the memory holding the font structure itself */
    free(font);
    return 0;
}

/*
 * \brief Removes a font from a font libary
 * 
 * UnloadFont removes the named font out of the libary indicated. The library
 * has to be a double pointer because if you may need to modify the actual
 * list pointer, for example, if you remove the first element in the list.
 */
int
UnloadFont(GSList ** pLibrary, char * fontname)
{
    FontType * font;
    GSList *itt;
    if (g_strcmp0(fontname, "all") == 0)
    {  /* Unload all fonts */
        for(itt = *pLibrary; itt; itt = itt->next)
            UnloadFont(pLibrary, ((FontType*)itt->data)->Name);
        if (*pLibrary == Settings.FontLibrary) Settings.Font = NULL;
    }
    else
    { /* Unload a particular font */
        font = FindFont(*pLibrary, fontname);
        if (!font)
        {
            Message(_("Could not unload font %s, "
                      "font not found in system library\n"),
                   fontname);
            return -1;
        }
        FreeFontMemory(font);
        *pLibrary = g_slist_remove(*pLibrary, font);
        /* 
         * if we unloaded the current font, either switch to the first one in
         * the list, or if there are no more, set the font pointer to NULL.
         */
        if (font == Settings.Font)
        {
            if (g_slist_length(Settings.FontLibrary) >= 1)
            {
                Message(_("Current font unloaded. Switching to %s.\n"),
                        ((FontType*)Settings.FontLibrary->data)->Name);
                ChangeFont(((FontType*)Settings.FontLibrary->data)->Name);
            }
            else
                Settings.Font = NULL;
        }
    }
    if (g_slist_length(*pLibrary) < 1)
    {
        g_slist_free(*pLibrary);
        *pLibrary = NULL;
    }
    return 0;
}

int SetPCBDefaultFont(char * fontname)
{
    FontType * font;
    /* Check the embedded library */
    font = FindFont(PCB->FontLibrary, fontname);
    /* Check the system library */
    if (!font) font = FindFont(Settings.FontLibrary, fontname);
    /* Don't allow setting to an unknown font */
    if (!font)
    {
        Message(_("Could not change PCB default font, font '%s' not found\n"),
                fontname);
        return -1;
    }
    /* the font was found in one or the other libraries */
    if (PCB->DefaultFontName) free(PCB->DefaultFontName);
    PCB->DefaultFontName = g_strdup(fontname);
    return 0;
}

static const char setpcbfont_syntax[] = "SetPCBDefaultFont(fontname)";

static const char
setpcbfont_help[] = "Set the default font for the current layout";
/*!
 * \brief User Action to change the default font for a PCB file
 *
 * Set the default font for text elements in a layout.
 * When a text element is read from a file, if the font is not specified, the 
 * font set here will be used. This does not set the "current" font, so, new 
 * text objects created by the user may not use this font. Use ChangeFont to set
 * that font. Note that changing this setting could cause the layout to be
 * different if it is saved and reloaded.
 *
 */
static int
SetPCBDefaultFontAction(int argc, char **argv, Coord x, Coord y)
{
    if (argc < 1)
    {
        Message (_("Tell me what font use\n"));
        return -1;
    }
    SetPCBDefaultFont(argv[0]);
    return 0;
}

static const char changefont_syntax[] = "ChangeFonts(fontname)";

static const char changefont_help[] = "Change the current font";

static int
ChangeFontAction(int argc, char **argv, Coord x, Coord y)
{
    if (argc < 1)
    {
        Message (_("Tell me what font use\n"));
        return -1;
    }
    ChangeFont(argv[0]);
    return 0;
}

static const char listfonts_syntax[] = "ListFonts()";

static const char listfonts_help[] = "List fonts in library.";

static int
ListFontsAction(int argc, char **argv, Coord x, Coord y)
{
    GSList * itt;
    Message(_("Fonts in system library: \n"));
    for (itt = Settings.FontLibrary; itt; itt = itt->next)
        Message(_("\t%s\n"), ((FontType*)itt->data)->Name);
    Message(_("Fonts in embedded library: \n"));
    for (itt = PCB->FontLibrary; itt; itt = itt->next)
        Message(_("\t%s\n"), ((FontType*)itt->data)->Name);
    Message(_("Currently selected font is: %s\n"), Settings.Font->Name);
    Message(_("PCB file default font is: %s\n"), PCB->DefaultFontName);
    return 0;
}

static const char loadfont_syntax[] = "LoadFont(filename)";

static const char loadfont_help[] = "Load font data from a file.";
/*!
 * \brief User Action to load a new font
 */
static int
LoadFontAction (int argc, char **argv, Coord x, Coord y)
{
    if (argc < 1)
    {
        Message (_("Tell me what font file to load\n"));
        return -1;
    }
    LoadFont(argv[0]);
    return 0;
}

static const char unloadfont_syntax[] = "UnloadFont(filename)";

static const char unloadfont_help[] = "Unload font data from the system library.";
/*!
 * \brief User Action to load a new font
 *
 * UnloadFontAction is the users way of removing a font from the system library.
 * The user can only modify the SystemFontLibrary. The EmbeddedFontLibrary is
 * generated only by loading font data from PCB files.
 *
 */
static int
UnloadFontAction (int argc, char **argv, Coord x, Coord y)
{
    if (argc < 1)
    {
        Message (_("Tell me what font to unload\n"));
        return -1;
    }
    UnloadFont(&Settings.FontLibrary, argv[0]);
    /* 
     * Don't let the user get rid of all the fonts, there has to be at least one
     * in order to create any kind of text object. Do this here, because we do 
     * want to let the system unload all fonts when exiting.
     */
    if (g_slist_length(Settings.FontLibrary) < 1)
    {
        Message(_("You have to leave at least one font in the libary.\n"
               "Loading the default font.\n"));
        LoadFont(Settings.FontFile);
    }
    return 0;
}

/*!
 * \brief Transforms symbol coordinates so that the left edge of each
 * symbol is at the zero position.
 *
 * The y coordinates are moved so that min(y) = 0.
 *
 */
void
SetFontInfo (FontType *Ptr)
{
    Cardinal i, j;
    SymbolType *symbol;
    LineType *line;
    Coord totalminy = MAX_COORD;
    
    /* calculate cell with and height (is at least DEFAULT_CELLSIZE)
     * maximum cell width and height
     * minimum x and y position of all lines
     */
    Ptr->MaxWidth = DEFAULT_CELLSIZE;
    Ptr->MaxHeight = DEFAULT_CELLSIZE;
    for (i = 0, symbol = Ptr->Symbol; i <= MAX_FONTPOSITION; i++, symbol++)
    {
        Coord minx, miny, maxx, maxy;
        
        /* next one if the index isn't used or symbol is empty (SPACE) */
        if (!symbol->Valid || !symbol->LineN)
            continue;
        
        minx = miny = MAX_COORD;
        maxx = maxy = 0;
        for (line = symbol->Line, j = symbol->LineN; j; j--, line++)
        {
            minx = MIN (minx, line->Point1.X);
            miny = MIN (miny, line->Point1.Y);
            minx = MIN (minx, line->Point2.X);
            miny = MIN (miny, line->Point2.Y);
            maxx = MAX (maxx, line->Point1.X);
            maxy = MAX (maxy, line->Point1.Y);
            maxx = MAX (maxx, line->Point2.X);
            maxy = MAX (maxy, line->Point2.Y);
        }
        
        /* move symbol to left edge */
        for (line = symbol->Line, j = symbol->LineN; j; j--, line++)
            MOVE_LINE_LOWLEVEL (line, -minx, 0);
        
        /* set symbol bounding box with a minimum cell size of (1,1) */
        symbol->Width = maxx - minx + 1;
        symbol->Height = maxy + 1;
        
        /* check total min/max  */
        Ptr->MaxWidth = MAX (Ptr->MaxWidth, symbol->Width);
        Ptr->MaxHeight = MAX (Ptr->MaxHeight, symbol->Height);
        totalminy = MIN (totalminy, miny);
    }
    
    /* move coordinate system to the upper edge (lowest y on screen) */
    for (i = 0, symbol = Ptr->Symbol; i <= MAX_FONTPOSITION; i++, symbol++)
        if (symbol->Valid)
        {
            symbol->Height -= totalminy;
            for (line = symbol->Line, j = symbol->LineN; j; j--, line++)
                MOVE_LINE_LOWLEVEL (line, 0, -totalminy);
        }
    
    /* setup the box for the default symbol */
    Ptr->DefaultSymbol.X1 = Ptr->DefaultSymbol.Y1 = 0;
    Ptr->DefaultSymbol.X2 = Ptr->DefaultSymbol.X1 + Ptr->MaxWidth;
    Ptr->DefaultSymbol.Y2 = Ptr->DefaultSymbol.Y1 + Ptr->MaxHeight;
}


#define CELL_SIZE	MIL_TO_COORD (100)
#define CELL_OFFSET	MIL_TO_COORD (10)

#define XYtoSym(x,y) ((x1 + CELL_OFFSET) / CELL_SIZE - 1 \
		      + 16 * ((y1 + CELL_OFFSET) / CELL_SIZE - 1))

static const char fontedit_syntax[] = "FontEdit()";

static const char fontedit_help[] =
  "Convert the current font to a PCB for editing.";

/* %start-doc actions FontEdit

This command only allows a font to be edited if the layout being edited contains
font symbols. The existing font symbols are displayed on a layer with an overlaid
grid, with the new version of the font on another layer which can be modified.
Font symbols consist only of lines. The blue lines next to the font symbols
indicate the space required after the symbol when it is used.

%end-doc */

static int
FontEdit (int argc, char **argv, Coord Ux, Coord Uy)
{
  FontType *font;
  SymbolType *symbol;
  LayerType *lfont, *lorig, *lwidth, *lgrid;
  int s, l;

  if (hid_actionl ("New", "Font", 0))
    return 1;

  Settings.grid_unit = get_unit_struct("mil");
  Settings.Bloat = PCB->Bloat = 1;
  Settings.Shrink = PCB->Shrink = 1;
  Settings.minWid = PCB->minWid = 1;
  Settings.minSlk = PCB->minSlk = 1;

  MoveLayerToGroup (max_copper_layer + TOP_SILK_LAYER, 0);
  MoveLayerToGroup (max_copper_layer + BOTTOM_SILK_LAYER, 1);

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

  font = Settings.Font;
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

Once a font has been modified with the FontEdit command, the layout can be saved
as a new PCB layout. The new PCB layout can then be opened with a text editor so
that the font section can be removed and saved as a new "default_font" file for
use in other PCB layouts.

%end-doc */

static int
FontSave (int argc, char **argv, Coord Ux, Coord Uy)
{
  FontType *font;
  SymbolType *symbol;
  int i;
  GList *ii;
  LayerType *lfont, *lwidth;

  font = Settings.Font;
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
      symbol = &Settings.Font->Symbol[s];

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
      symbol = &Settings.Font->Symbol[s];

      x1 -= ox;

      symbol->Delta = x1 - symbol->Width;
    }

  SetFontInfo (font);
  
  return 0;
}

HID_Action fontmode_action_list[] = {
  {"SetPCBDefaultFont", 0, SetPCBDefaultFontAction,
   setpcbfont_help, setpcbfont_syntax},
  {"ChangeFont", 0, ChangeFontAction,
   changefont_help, changefont_syntax},
  {"LoadFont", 0, LoadFontAction,
   loadfont_help, loadfont_syntax},
  {"UnloadFont", 0, UnloadFontAction,
   unloadfont_help, unloadfont_syntax},
  {"ListFonts", 0, ListFontsAction,
   listfonts_help, listfonts_syntax},
  {"FontEdit", 0, FontEdit,
   fontedit_help, fontedit_syntax},
  {"FontSave", 0, FontSave,
   fontsave_help, fontsave_syntax}
};

REGISTER_ACTIONS (fontmode_action_list)
