/*
 * ************************** README *******************
 *
 * If the file format is modified in any way, update
 * PCB_FILE_VERSION in file.h
 * 
 * ************************** README *******************
 */

%{
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

/* grammar to parse ASCII input of PCB description
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"
#include "create.h"
#include "data.h"
#include "error.h"
#include "file.h"
#include "layerflags.h"
#include "mymem.h"
#include "misc.h"
#include "parse_l.h"
#include "polygon.h"
#include "remove.h"
#include "rtree.h"
#include "strflags.h"
#include "thermal.h"
#include "move.h"

#ifdef HAVE_LIBDMALLOC
# include <dmalloc.h> /* see http://dmalloc.com */
#endif

static	LayerType	*Layer;
static	PolygonType	*Polygon;
static	SymbolType	*Symbol;
static	int		pin_num;
static	LibraryMenuType	*Menu;
static	bool			LayerFlag[MAX_ALL_LAYER];

extern	char		*yytext;		/* defined by LEX */
extern	PCBType		*yyPCB;
extern	DataType	*yyData;
extern	ElementType	*yyElement;
extern	FontType	*yyFont;
extern	int		yylineno;		/* linenumber */
extern	char		*yyfilename;	/* in this file */

static AttributeListType *attr_list; 

int yyerror(const char *s);
int yylex();
static int check_file_version (int);

static void do_measure (PLMeasure *m, Coord i, double d, int u);
#define M(r,f,d) do_measure (&(r), f, d, 1)

/* Macros for interpreting what "measure" means - integer value only,
   old units (mil), or new units (cmil).  */
#define IV(m) integer_value (m)
#define OU(m) old_units (m)
#define NU(m) new_units (m)

static int integer_value (PLMeasure m);
static Coord old_units (PLMeasure m);
static Coord new_units (PLMeasure m);

#define YYDEBUG 1
#define YYERROR_VERBOSE 1

#include "parse_y.h"

%}

%verbose

%union									/* define YYSTACK type */
{
	int		integer;
	double		number;
	char		*string;
	FlagType	flagtype;
	PLMeasure	measure;
}

%token	<number>	FLOATING		/* line thickness, coordinates ... */
%token	<integer>	INTEGER	CHAR_CONST	/* flags ... */
%token	<string>	STRING			/* element names ... */

%token	T_FILEVERSION T_PCB T_LAYER T_VIA T_RAT T_LINE T_ARC T_RECTANGLE T_TEXT T_ELEMENTLINE
%token	T_ELEMENT T_PIN T_PAD T_GRID T_FLAGS T_SYMBOL T_SYMBOLLINE T_CURSOR
%token	T_ELEMENTARC T_MARK T_GROUPS T_STYLES T_POLYGON T_POLYGON_HOLE T_NETLIST T_NET T_CONN
%token	T_AREA T_THERMAL T_DRC T_ATTRIBUTE
%token	T_UMIL T_CMIL T_MIL T_IN T_NM T_UM T_MM T_M T_KM T_PX
%type	<integer>	symbolid
%type	<string>	opt_string
%type	<flagtype>	flags
%type	<number>	number
%type	<measure>	measure

%%

parse
		: parsepcb
		| parsedata
		| parsefont
		| error { YYABORT; }
		;

/* %start-doc pcbfile 00pcb
@nodetype subsection
@nodename %s syntax

A special note about units: Older versions of @code{pcb} used mils
(1/1000 inch) as the base unit; a value of 500 in the file meant
half an inch.  Newer versions uses a "high resolution" syntax,
where the base unit is 1/100 of a mil (0.000010 inch); a value of 500 in
the file means 5 mils.  As a general rule, the variants of each entry
listed below which use square brackets are the high resolution formats
and use the 1/100 mil units, and the ones with parentheses are the older
variants and use 1 mil units.  Note that when multiple variants
are listed, the most recent (and most preferred) format is the first
listed.

Symbolic and numeric flags (SFlags and NFlags) are described in
@ref{Object Flags}.

%end-doc */
		
parsepcb
		:	{
					/* reset flags for 'used layers';
					 * init font and data pointers
					 */
				int	i;

				if (!yyPCB)
				{
					Message(_("illegal fileformat\n"));
					YYABORT;
				}
				for (i = 0; i < MAX_ALL_LAYER; i++)
					LayerFlag[i] = false;
				yyFont = &yyPCB->Font;
				yyData = yyPCB->Data;
				yyData->pcb = yyPCB;
				yyData->LayerN = 0;
				/* Parse the default layer group string, just in case the file doesn't have one */
				if (ParseGroupString (Settings.Groups, &yyPCB->LayerGroups, &yyData->LayerN))
				    {
				      Message(_("illegal default layer-group string\n"));
				      YYABORT;
				    }
			}
		  pcbfileversion
		  pcbname 
		  pcbgrid
		  pcbcursor
		  polyarea
		  pcbthermal 
		  pcbdrc
		  pcbflags
		  pcbgroups
		  pcbstyles
		  pcbfont
		  pcbdata
		  pcbnetlist
			{
			  PCBType *pcb_save = PCB;

			  CreateNewPCBPost (yyPCB, 0);
			/* initialize the polygon clipping now since
			 * we didn't know the layer grouping before.
			 */
			PCB = yyPCB;
			ALLPOLYGON_LOOP (yyData);
			{
			  InitClip (yyData, layer, polygon);
			}
			ENDALL_LOOP;
			PCB = pcb_save;
			}
			   
		| {
		    if (yyPCB != NULL)
		      {
			/* This case is when we load a footprint with file->open, or from the command line */
			yyFont = &yyPCB->Font;
			yyData = yyPCB->Data;
			yyData->pcb = yyPCB;
			yyData->LayerN = 0;
		      }
		  }
		  element
		  {
		    PCBType *pcb_save = PCB;
		    ElementType *e;
		    if (yyPCB != NULL)
		      {
			/* This case is when we load a footprint with file->open, or from the command line */
			CreateNewPCBPost (yyPCB, 0);
			ParseGroupString("1,c:2,s", &yyPCB->LayerGroups, &yyData->LayerN);
			e = yyPCB->Data->Element->data; /* we know there's only one */
			PCB = yyPCB;
			MoveElementLowLevel (yyPCB->Data, e, -e->BoundingBox.X1, -e->BoundingBox.Y1);
			PCB = pcb_save;
			yyPCB->MaxWidth = e->BoundingBox.X2;
			yyPCB->MaxHeight = e->BoundingBox.Y2;
			yyPCB->is_footprint = 1;
		      }
		  }
		;

parsedata
		:	{
					/* reset flags for 'used layers';
					 * init font and data pointers
					 */
				int	i;

				if (!yyData || !yyFont)
				{
					Message(_("illegal fileformat\n"));
					YYABORT;
				}
				for (i = 0; i < MAX_ALL_LAYER; i++)
					LayerFlag[i] = false;
				yyData->LayerN = 0;
			}
		 pcbdata
		;

pcbfont
		: parsefont
		|
		;

parsefont
		:
			{
					/* mark all symbols invalid */
				int	i;

				if (!yyFont)
				{
					Message(_("illegal fileformat\n"));
					YYABORT;
				}
				yyFont->Valid = false;
				for (i = 0; i <= MAX_FONTPOSITION; i++)
					free (yyFont->Symbol[i].Line);
				bzero(yyFont->Symbol, sizeof(yyFont->Symbol));
			}
		  symbols
			{
				yyFont->Valid = true;
		  		SetFontInfo(yyFont);
			}
		;

/* %start-doc pcbfile FileVersion

@syntax
FileVersion[Version]
@end syntax

@table @var
@item Version
File format version.  This version number represents the date when the pcb file
format was last changed.
@end table

Any version of pcb build from sources equal to or newer
than this number should be able to read the file.  If this line is not present
in the input file then file format compatibility is not checked.


%end-doc */

pcbfileversion
: |
T_FILEVERSION '[' INTEGER ']'
{
  if (check_file_version ($3) != 0)
    {
      YYABORT;
    }
}
;	

/* %start-doc pcbfile PCB

@syntax
PCB ["Name" Width Height]
PCB ("Name" Width Height]
PCB ("Name")
@end syntax

@table @var
@item Name
Name of the PCB project
@item Width Height
Size of the board
@end table

If you don't specify the size of the board, a very large default is
chosen.

%end-doc */

pcbname
		: T_PCB '(' STRING ')'
			{
				yyPCB->Name = $3;
				yyPCB->MaxWidth = MAX_COORD;
				yyPCB->MaxHeight = MAX_COORD;
			}
		| T_PCB '(' STRING measure measure ')'
			{
				yyPCB->Name = $3;
				yyPCB->MaxWidth = OU ($4);
				yyPCB->MaxHeight = OU ($5);
			}
		| T_PCB '[' STRING measure measure ']'
			{
				yyPCB->Name = $3;
				yyPCB->MaxWidth = NU ($4);
				yyPCB->MaxHeight = NU ($5);
			}
		;	

/* %start-doc pcbfile Grid

@syntax
Grid [Step OffsetX OffsetY Visible]
Grid (Step OffsetX OffsetY Visible)
Grid (Step OffsetX OffsetY)
@end syntax

@table @var
@item Step
Distance from one grid point to adjacent points.  This value may be a
floating point number for the first two variants.
@item OffsetX OffsetY
The "origin" of the grid.  Normally zero.
@item Visible
If non-zero, the grid will be visible on the screen.
@end table

%end-doc */

pcbgrid
		: pcbgridold
		| pcbgridnew
		| pcbhigrid
		;
pcbgridold
		: T_GRID '(' measure measure measure ')'
			{
				yyPCB->Grid = OU ($3);
				yyPCB->GridOffsetX = OU ($4);
				yyPCB->GridOffsetY = OU ($5);
			}
		;
pcbgridnew
		: T_GRID '(' measure measure measure INTEGER ')'
			{
				yyPCB->Grid = OU ($3);
				yyPCB->GridOffsetX = OU ($4);
				yyPCB->GridOffsetY = OU ($5);
				if ($6)
					Settings.DrawGrid = true;
				else
					Settings.DrawGrid = false;
			}
		;

pcbhigrid
		: T_GRID '[' measure measure measure INTEGER ']'
			{
				yyPCB->Grid = NU ($3);
				yyPCB->GridOffsetX = NU ($4);
				yyPCB->GridOffsetY = NU ($5);
				if ($6)
					Settings.DrawGrid = true;
				else
					Settings.DrawGrid = false;
			}
		;

/* %start-doc pcbfile Cursor

@syntax
Cursor [X Y Zoom]
Cursor (X Y Zoom)
@end syntax

@table @var
@item X Y
Location of the cursor when the board was saved.
As of November 2012 the cursor position is not written to file anymore.
Older versions of pcb ignore the absence of this line in the pcb file.
@item Zoom
The current zoom factor.  Note that a zoom factor of "0" means 1 mil
per screen pixel, N means @math{2^N} mils per screen pixel, etc.  The
first variant accepts floating point numbers.  The special value
"1000" means "zoom to fit"

This field is ignored by PCB.
@end table

%end-doc */

pcbcursor
		: T_CURSOR '(' measure measure number ')'
			{
				yyPCB->CursorX = OU ($3);
				yyPCB->CursorY = OU ($4);
			}
		| T_CURSOR '[' measure measure number ']'
			{
				yyPCB->CursorX = NU ($3);
				yyPCB->CursorY = NU ($4);
			}
		|
		;

/* %start-doc pcbfile PolyArea

@syntax
PolyArea [Area]
@end syntax

@table @var
@item Area 
Minimum area of polygon island to retain. If a polygon has clearances that cause an isolated island to be created, then will only be retained if the area exceeds this minimum area.
@end table

%end-doc */

polyarea
		:
		| T_AREA '[' number ']'
			{
				/* Read in cmil^2 for now; in future this should be a noop. */
				yyPCB->IsleArea = MIL_TO_COORD (MIL_TO_COORD ($3) / 100.0) / 100.0;
			}
		;


/* %start-doc pcbfile Thermal

@syntax
Thermal [Scale]
@end syntax

@table @var
@item Scale
Relative size of thermal fingers.  A value of 1.0 makes the finger
width twice the clearance gap width (measured across the gap, not
diameter).  The normal value is 0.5, which results in a finger width
the same as the clearance gap width.
@end table

%end-doc */


pcbthermal
		:
		| T_THERMAL '[' number ']'
			{
				yyPCB->ThermScale = $3;
			}
		;

/* %start-doc pcbfile DRC

@syntax
DRC [Bloat Shrink Line Silk Drill Ring]
DRC [Bloat Shrink Line Silk]
DRC [Bloat Shrink Line]
@end syntax

@table @var
@item Bloat
Minimum spacing between copper.
@item Shrink
Minimum copper overlap to guarantee connectivity.
@item Line
Minimum line thickness.
@item Silk
Minimum silk thickness.
@item Drill
Minimum drill size.
@item Ring
Minimum width of the annular ring around pins and vias.
@end table

%end-doc */

pcbdrc
		:
		| pcbdrc1
		| pcbdrc2
		| pcbdrc3
		;

pcbdrc1
                : T_DRC '[' measure measure measure ']'
		        {
				yyPCB->Bloat = NU ($3);
				yyPCB->Shrink = NU ($4);
				yyPCB->minWid = NU ($5);
				yyPCB->minRing = NU ($5);
			}
		;

pcbdrc2
                : T_DRC '[' measure measure measure measure ']'
		        {
				yyPCB->Bloat = NU ($3);
				yyPCB->Shrink = NU ($4);
				yyPCB->minWid = NU ($5);
				yyPCB->minSlk = NU ($6);
				yyPCB->minRing = NU ($5);
			}
		;

pcbdrc3
                : T_DRC '[' measure measure measure measure measure measure ']'
		        {
				yyPCB->Bloat = NU ($3);
				yyPCB->Shrink = NU ($4);
				yyPCB->minWid = NU ($5);
				yyPCB->minSlk = NU ($6);
				yyPCB->minDrill = NU ($7);
				yyPCB->minRing = NU ($8);
			}
		;

/* %start-doc pcbfile Flags

@syntax
Flags(Number)
@end syntax

@table @var
@item Number
A number, whose value is normally given in hex, individual bits of which
represent pcb-wide flags as defined in @ref{PCBFlags}.

@end table

%end-doc */

pcbflags
		: T_FLAGS '(' INTEGER ')'
			{
				yyPCB->Flags = MakeFlags ($3 & PCB_FLAGS);
			}
		| T_FLAGS '(' STRING ')'
			{
			  yyPCB->Flags = string_to_pcbflags ($3, yyerror);
                          free ($3);
			}
		|
		;

/* %start-doc pcbfile Groups

@syntax
Groups("String")
@end syntax

@table @var
@item String

Encodes the layer grouping information.  Each group is separated by a
colon, each member of each group is separated by a comma.  Group
members are either numbers from @code{1}..@var{N} for each layer, and
the letters @code{c} or @code{s} representing the component side and
solder side of the board.  Including @code{c} or @code{s} marks that
group as being the top or bottom side of the board.

@example
Groups("1,2,c:3:4:5,6,s:7,8")
@end example

@end table

%end-doc */

pcbgroups
		: T_GROUPS '(' STRING ')'
			{
			  if (ParseGroupString ($3, &yyPCB->LayerGroups, &yyData->LayerN))
			    {
			      Message(_("illegal layer-group string\n"));
			      YYABORT;
			    }
                            free ($3);
			}
		|
		;

/* %start-doc pcbfile Styles

@syntax
Styles("String")
@end syntax

@table @var
@item String

Encodes the four routing styles @code{pcb} knows about.  The four styles
are separated by colons.  Each style consists of five parameters as follows:

@table @var
@item Name
The name of the style.
@item Thickness
Width of lines and arcs.
@item Diameter
Copper diameter of pins and vias.
@item Drill
Drill diameter of pins and vias.
@item Keepaway
Minimum spacing to other nets.  If omitted, 10 mils is the default.

@end table

@end table

@example
Styles("Signal,10,40,20:Power,25,60,35:Fat,40,60,35:Skinny,8,36,20")
Styles["Logic,1000,3600,2000,1000:Power,2500,6000,3500,1000:
@ @ @ Line,4000,6000,3500,1000:Breakout,600,2402,1181,600"]
@end example

@noindent
Note that strings in actual files cannot span lines; the above example
is split across lines only to make it readable.

%end-doc */

pcbstyles
		: T_STYLES '(' STRING ')'
			{
				if (ParseRouteString($3, &yyPCB->RouteStyle[0], "mil"))
				{
					Message(_("illegal route-style string\n"));
					YYABORT;
				}
                                free ($3);
			}
		| T_STYLES '[' STRING ']'
			{
				if (ParseRouteString($3, &yyPCB->RouteStyle[0], "cmil"))
				{
					Message(_("illegal route-style string\n"));
					YYABORT;
				}
                                free ($3);
			}
		|
		;

pcbdata
		: pcbdefinitions
		|
		;

pcbdefinitions
		: pcbdefinition
		| pcbdefinitions pcbdefinition
		;

pcbdefinition
		: via
		| { attr_list = & yyPCB->Attributes; } attribute
		| rats
		| layer
		| element
		| error { YYABORT; }
		;

via
		: via_ehi_format
		| via_hi_format
		| via_2.0_format
		| via_1.7_format
		| via_newformat
		| via_oldformat
		;

/* %start-doc pcbfile Via

@syntax
Via [X Y Thickness Clearance Mask Drill BuriedFrom BuriedTo "Name" SFlags]
Via [X Y Thickness Clearance Mask Drill "Name" SFlags]
Via (X Y Thickness Clearance Mask Drill "Name" NFlags)
Via (X Y Thickness Clearance Drill "Name" NFlags)
Via (X Y Thickness Drill "Name" NFlags)
Via (X Y Thickness "Name" NFlags)
@end syntax

@table @var
@item X Y
coordinates of center
@item Thickness
outer diameter of copper annulus
@item Clearance
add to thickness to get clearance diameter
@item Mask
diameter of solder mask opening
@item Drill
diameter of drill
@item Name
string, name of via (vias have names?)
@item SFlags
symbolic or numerical flags
@item NFlags
numerical flags only
@end table

%end-doc */

via_ehi_format
		/* x, y, thickness, clearance, mask, drilling-hole, buried_from, buried_to, name, flags */
		: T_VIA '[' measure measure measure measure measure measure INTEGER INTEGER STRING flags ']'
			{
				CreateNewViaEx (yyData, NU ($3), NU ($4), NU ($5), NU ($6), NU ($7),
						     NU ($8), $11, $12, $9, $10);
				free ($11);
			}
		;

via_hi_format
			/* x, y, thickness, clearance, mask, drilling-hole, name, flags */
		: T_VIA '[' measure measure measure measure measure measure STRING flags ']'
			{
				CreateNewVia(yyData, NU ($3), NU ($4), NU ($5), NU ($6), NU ($7),
				                     NU ($8), $9, $10);
				free ($9);
			}
		;

via_2.0_format
			/* x, y, thickness, clearance, mask, drilling-hole, name, flags */
		: T_VIA '(' measure measure measure measure measure measure STRING INTEGER ')'
			{
				CreateNewVia(yyData, OU ($3), OU ($4), OU ($5), OU ($6), OU ($7), OU ($8), $9,
					OldFlags($10));
				free ($9);
			}
		;


via_1.7_format
			/* x, y, thickness, clearance, drilling-hole, name, flags */
		: T_VIA '(' measure measure measure measure measure STRING INTEGER ')'
			{
				CreateNewVia(yyData, OU ($3), OU ($4), OU ($5), OU ($6),
					     OU ($5) + OU($6), OU ($7), $8, OldFlags($9));
				free ($8);
			}
		;

via_newformat
			/* x, y, thickness, drilling-hole, name, flags */
		: T_VIA '(' measure measure measure measure STRING INTEGER ')'
			{
				CreateNewVia(yyData, OU ($3), OU ($4), OU ($5), 2*GROUNDPLANEFRAME,
					OU($5) + 2*MASKFRAME,  OU ($6), $7, OldFlags($8));
				free ($7);
			}
		;

via_oldformat
			/* old format: x, y, thickness, name, flags */
		: T_VIA '(' measure measure measure STRING INTEGER ')'
			{
				Coord	hole = (OU($5) * DEFAULT_DRILLINGHOLE);

					/* make sure that there's enough copper left */
				if (OU($5) - hole < MIN_PINORVIACOPPER && 
					OU($5) > MIN_PINORVIACOPPER)
					hole = OU($5) - MIN_PINORVIACOPPER;

				CreateNewVia(yyData, OU ($3), OU ($4), OU ($5), 2*GROUNDPLANEFRAME,
					OU($5) + 2*MASKFRAME, hole, $6, OldFlags($7));
				free ($6);
			}
		;

/* %start-doc pcbfile Rat

@syntax
Rat [X1 Y1 Group1 X2 Y2 Group2 SFlags]
Rat (X1 Y1 Group1 X2 Y2 Group2 NFlags)
@end syntax

@table @var
@item X1 Y1 X2 Y2
The endpoints of the rat line.
@item Group1 Group2
The layer group each end is connected on.
@item SFlags
Symbolic or numeric flags.
@item NFlags
Numeric flags.
@end table

%end-doc */

rats
		: T_RAT '[' measure measure INTEGER measure measure INTEGER flags ']'
			{
				CreateNewRat(yyData, NU ($3), NU ($4), NU ($6), NU ($7), $5, $8,
					Settings.RatThickness, $9);
			}
		| T_RAT '(' measure measure INTEGER measure measure INTEGER INTEGER ')'
			{
				CreateNewRat(yyData, OU ($3), OU ($4), OU ($6), OU ($7), $5, $8,
					Settings.RatThickness, OldFlags($9));
			}
		;

/* %start-doc pcbfile Layer

@syntax
Layer (LayerNum "Name" "Flags") (
@ @ @ @dots{} contents @dots{}
)
@end syntax

@table @var
@item LayerNum
The layer number.  Layers are numbered sequentially, starting with 1.
The last two layers (9 and 10 by default) are solder-side silk and
component-side silk, in that order. The two silk layers also mark top and
bottom side; the layer group where the solder-side silk layer is member in
is the solder side group. Analogous for the other side.
@item Name
The layer name.

For layout files predating layer flags the name also defines
the layer type in some situations. For example, a layer named @emph{outline}
was considered to be the layer defining the extents of the board.
@item Flags
Layer flags. Currently this is the layer type, like @emph{copper}, @emph{silk}
or @emph{outline}. For a complete list see layertype_name[] in layerflags.c.

With layer flags missing, the type of layer is guessed at load time, mostly by
the layer name. This mechanism ensures compatibility with older layouts.
@item contents
The contents of the layer, which may include attributes, lines, arcs, rectangles,
text, and polygons.
@end table

%end-doc */

layer
			/* name */
		: T_LAYER '(' INTEGER STRING opt_string ')' '('
			{
				if ($3 <= 0 || $3 > MAX_ALL_LAYER)
				{
					yyerror("Layernumber out of range");
					YYABORT;
				}
				if (LayerFlag[$3-1])
				{
					yyerror("Layernumber used twice");
					YYABORT;
				}
				Layer = &yyData->Layer[$3-1];

                                /* memory for name is already allocated */
				Layer->Name = $4;
                         	if (Layer->Name == NULL)
                                   Layer->Name = strdup("");
				LayerFlag[$3-1] = true;
                                if ($5)
                                  Layer->Type = string_to_layertype ($5, yyerror);
                                else
                                  Layer->Type = guess_layertype ($4, $3, yyData);
                                if ($5 != NULL)
                                  free ($5);
			}
		  layerdata ')'
		;

layerdata
		: layerdefinitions
		|
		;

layerdefinitions
		: layerdefinition
		| layerdefinitions layerdefinition
		;

layerdefinition
		: line_hi_format
		| line_1.7_format
		| line_oldformat
		| arc_hi_format
		| arc_1.7_format
		| arc_oldformat
			/* x1, y1, x2, y2, flags */
		| T_RECTANGLE '(' measure measure measure measure INTEGER ')'
			{
				CreateNewPolygonFromRectangle(Layer,
					OU ($3), OU ($4), OU ($3) + OU ($5), OU ($4) + OU ($6), OldFlags($7));
			}
		| text_hi_format
		| text_newformat
		| text_oldformat
		| { attr_list = & Layer->Attributes; } attribute
		| polygon_format

/* %start-doc pcbfile Line

@syntax
Line [X1 Y1 X2 Y2 Thickness Clearance SFlags]
Line (X1 Y1 X2 Y2 Thickness Clearance NFlags)
Line (X1 Y1 X2 Y2 Thickness NFlags)
@end syntax

@table @var
@item X1 Y1 X2 Y2
The end points of the line
@item Thickness
The width of the line
@item Clearance
The amount of space cleared around the line when the line passes
through a polygon.  The clearance is added to the thickness to get the
thickness of the clear; thus the space between the line and the
polygon is @math{Clearance/2} wide.
@item SFlags
Symbolic or numeric flags
@item NFlags
Numeric flags.
@end table

%end-doc */

line_hi_format
			/* x1, y1, x2, y2, thickness, clearance, flags */
		: T_LINE '[' measure measure measure measure measure measure flags ']'
			{
				CreateNewLineOnLayer(Layer, NU ($3), NU ($4), NU ($5), NU ($6),
				                            NU ($7), NU ($8), $9);
			}
		;

line_1.7_format
			/* x1, y1, x2, y2, thickness, clearance, flags */
		: T_LINE '(' measure measure measure measure measure measure INTEGER ')'
			{
				CreateNewLineOnLayer(Layer, OU ($3), OU ($4), OU ($5), OU ($6),
						     OU ($7), OU ($8), OldFlags($9));
			}
		;

line_oldformat
			/* x1, y1, x2, y2, thickness, flags */
		: T_LINE '(' measure measure measure measure measure measure ')'
			{
				/* eliminate old-style rat-lines */
			if ((IV ($8) & RATFLAG) == 0)
				CreateNewLineOnLayer(Layer, OU ($3), OU ($4), OU ($5), OU ($6), OU ($7),
					200*GROUNDPLANEFRAME, OldFlags(IV ($8)));
			}
		;

/* %start-doc pcbfile Arc

@syntax
Arc [X Y RadiusX RadiusY Thickness Clearance StartAngle DeltaAngle SFlags]
Arc (X Y RadiusX RadiusY Thickness Clearance StartAngle DeltaAngle NFlags)
Arc (X Y RadiusX RadiusY Thickness StartAngle DeltaAngle NFlags)
@end syntax

@table @var
@item X Y
Coordinates of the center of the arc.
@item RadiusX RadiusY
The RadiusX and RadiusY, from the center to the edge (centerline of the
trace).  The bounds of the circle of which this arc is a segment, is
thus @math{2*RadiusX} by @math{2*RadiusY}.
@item Thickness
The width of the copper trace which forms the arc.
@item Clearance
The amount of space cleared around the arc when the line passes
through a polygon.  The clearance is added to the thickness to get the
thickness of the clear; thus the space between the arc and the polygon
is @math{Clearance/2} wide.
@item StartAngle
The angle of one end of the arc, in degrees.  In PCB, an angle of zero
points left (negative X direction), and 90 degrees points down
(positive Y direction).
@item DeltaAngle
The sweep of the arc.  This may be negative.  Positive angles sweep
counterclockwise.
@item SFlags
Symbolic or numeric flags.
@item NFlags
Numeric flags.
@end table

%end-doc */

arc_hi_format
			/* x, y, width, height, thickness, clearance, startangle, delta, flags */
		: T_ARC '[' measure measure measure measure measure measure number number flags ']'
			{
			  CreateNewArcOnLayer(Layer, NU ($3), NU ($4), NU ($5), NU ($6), $9, $10,
			                             NU ($7), NU ($8), $11);
			}
		;

arc_1.7_format
			/* x, y, width, height, thickness, clearance, startangle, delta, flags */
		: T_ARC '(' measure measure measure measure measure measure number number INTEGER ')'
			{
				CreateNewArcOnLayer(Layer, OU ($3), OU ($4), OU ($5), OU ($6), $9, $10,
						    OU ($7), OU ($8), OldFlags($11));
			}
		;

arc_oldformat
			/* x, y, width, height, thickness, startangle, delta, flags */
		: T_ARC '(' measure measure measure measure measure measure number INTEGER ')'
			{
				CreateNewArcOnLayer(Layer, OU ($3), OU ($4), OU ($5), OU ($5), IV ($8), $9,
					OU ($7), 200*GROUNDPLANEFRAME, OldFlags($10));
			}
		;

/* %start-doc pcbfile Text

@syntax
Text [X Y Direction Scale "String" SFlags]
Text (X Y Direction Scale "String" NFlags)
Text (X Y Direction "String" NFlags)
@end syntax

@table @var
@item X Y
The location of the upper left corner of the text.
@item Direction
0 means text is drawn left to right, 1 means up, 2 means right to left
(i.e. upside down), and 3 means down.
@item Scale
Size of the text, as a percentage of the ``default'' size of of the
font (the default font is about 40 mils high).  Default is 100 (40
mils).
@item String
The string to draw.
@item SFlags
Symbolic or numeric flags.
@item NFlags
Numeric flags.
@end table

%end-doc */

text_oldformat
			/* x, y, direction, text, flags */
		: T_TEXT '(' measure measure number STRING INTEGER ')'
			{
					/* use a default scale of 100% */
				CreateNewText(Layer,yyFont,OU ($3), OU ($4), $5, 100, $6, OldFlags($7));
				free ($6);
			}
		;

text_newformat
			/* x, y, direction, scale, text, flags */
		: T_TEXT '(' measure measure number number STRING INTEGER ')'
			{
				if ($8 & ONSILKFLAG)
				{
					LayerType *lay = &yyData->Layer[yyData->LayerN +
						(($8 & ONSOLDERFLAG) ? BOTTOM_SILK_LAYER : TOP_SILK_LAYER)];

					CreateNewText(lay ,yyFont, OU ($3), OU ($4), $5, $6, $7,
						      OldFlags($8));
				}
				else
					CreateNewText(Layer, yyFont, OU ($3), OU ($4), $5, $6, $7,
						      OldFlags($8));
				free ($7);
			}
		;
text_hi_format
			/* x, y, direction, scale, text, flags */
		: T_TEXT '[' measure measure number number STRING flags ']'
			{
				/* FIXME: shouldn't know about .f */
				/* I don't think this matters because anything with hi_format
				 * will have the silk on its own layer in the file rather
				 * than using the ONSILKFLAG and having it in a copper layer.
				 * Thus there is no need for anything besides the 'else'
				 * part of this code.
				 */
				if ($8.f & ONSILKFLAG)
				{
					LayerType *lay = &yyData->Layer[yyData->LayerN +
						(($8.f & ONSOLDERFLAG) ? BOTTOM_SILK_LAYER : TOP_SILK_LAYER)];

					CreateNewText(lay, yyFont, NU ($3), NU ($4), $5, $6, $7, $8);
				}
				else
					CreateNewText(Layer, yyFont, NU ($3), NU ($4), $5, $6, $7, $8);
				free ($7);
			}
		;

/* %start-doc pcbfile Polygon

@syntax
Polygon (SFlags) (
@ @ @ @dots{} (X Y) @dots{}
@ @ @ @dots{} [X Y] @dots{}
@ @ @ Hole (
@ @ @ @ @ @ @dots{} (X Y) @dots{}
@ @ @ @ @ @ @dots{} [X Y] @dots{}
@ @ @ )
@ @ @ @dots{}
)
@end syntax

@table @var
@item SFlags
Symbolic or numeric flags.
@item X Y
Coordinates of each vertex.  You must list at least three coordinates.
@item Hole (...)
Defines a hole within the polygon's outer contour. There may be zero or more such sections.
@end table

%end-doc */

polygon_format
		: /* flags are passed in */
		T_POLYGON '(' flags ')' '('
			{
				Polygon = CreateNewPolygon(Layer, $3);
			}
		  polygonpoints
		  polygonholes ')'
			{
				Cardinal contour, contour_start, contour_end;
				bool bad_contour_found = false;
				/* ignore junk */
				for (contour = 0; contour <= Polygon->HoleIndexN; contour++)
				  {
				    contour_start = (contour == 0) ?
						      0 : Polygon->HoleIndex[contour - 1];
				    contour_end = (contour == Polygon->HoleIndexN) ?
						 Polygon->PointN :
						 Polygon->HoleIndex[contour];
				    if (contour_end - contour_start < 3)
				      bad_contour_found = true;
				  }

				if (bad_contour_found)
				  {
				    Message(_("WARNING parsing file '%s'\n"
					    "    line:        %i\n"
					    "    description: 'ignored polygon "
					    "(< 3 points in a contour)'\n"),
					    yyfilename, yylineno);
				    DestroyObject(yyData, POLYGON_TYPE, Layer, Polygon, Polygon);
				  }
				else
				  {
				    SetPolygonBoundingBox (Polygon);
				    if (!Layer->polygon_tree)
				      Layer->polygon_tree = r_create_tree (NULL, 0, 0);
				    r_insert_entry (Layer->polygon_tree, (BoxType *) Polygon, 0);
				  }
			}
		;

polygonholes
		: /* empty */
		| polygonholes polygonhole
		;

polygonhole
		: T_POLYGON_HOLE '('
			{
				CreateNewHoleInPolygon (Polygon);
			}
		  polygonpoints ')'
		;

polygonpoints
		: /* empty */
		| polygonpoint polygonpoints
		;

polygonpoint
			/* xcoord ycoord */
		: '(' measure measure ')'
			{
				CreateNewPointInPolygon(Polygon, OU ($2), OU ($3));
			}
		| '[' measure measure ']'
			{
				CreateNewPointInPolygon(Polygon, NU ($2), NU ($3));
			}
		;

/* %start-doc pcbfile Element

@syntax
Element [SFlags "Desc" "Name" "Value" MX MY TX TY TDir TScale TSFlags] (
Element (NFlags "Desc" "Name" "Value" MX MY TX TY TDir TScale TNFlags) (
Element (NFlags "Desc" "Name" "Value" TX TY TDir TScale TNFlags) (
Element (NFlags "Desc" "Name" TX TY TDir TScale TNFlags) (
Element ("Desc" "Name" TX TY TDir TScale TNFlags) (
@ @ @ @dots{} contents @dots{}
)
@end syntax

@table @var
@item SFlags
Symbolic or numeric flags, for the element as a whole.
@item NFlags
Numeric flags, for the element as a whole.
@item Desc
The description of the element.  This is one of the three strings
which can be displayed on the screen.
@item Name
The name of the element, usually the reference designator.
@item Value
The value of the element.
@item MX MY
The location of the element's mark.  This is the reference point
for placing the element and its pins and pads.
@item TX TY
The upper left corner of the text (one of the three strings).
@item TDir
The relative direction of the text.  0 means left to right for
an unrotated element, 1 means up, 2 left, 3 down.
@item TScale
Size of the text, as a percentage of the ``default'' size of of the
font (the default font is about 40 mils high).  Default is 100 (40
mils).
@item TSFlags
Symbolic or numeric flags, for the text.
@item TNFlags
Numeric flags, for the text.
@end table

Elements may contain pins, pads, element lines, element arcs,
attributes, and (for older elements) an optional mark.  Note that
element definitions that have the mark coordinates in the element
line, only support pins and pads which use relative coordinates.  The
pin and pad coordinates are relative to the mark.  Element definitions
which do not include the mark coordinates in the element line, may
have a Mark definition in their contents, and only use pin and pad
definitions which use absolute coordinates.

%end-doc */

element
		: element_oldformat
		| element_1.3.4_format
		| element_newformat
		| element_1.7_format
		| element_hi_format
		;

element_oldformat
			/* element_flags, description, pcb-name,
			 * text_x, text_y, text_direction, text_scale, text_flags
			 */
		: T_ELEMENT '(' STRING STRING measure measure INTEGER ')' '('
			{
				yyElement = CreateNewElement(yyData, yyFont, NoFlags(),
					$3, $4, NULL, OU ($5), OU ($6), $7, 100, NoFlags(), false);
				free ($3);
				free ($4);
				pin_num = 1;
			}
		  elementdefinitions ')'
			{
				SetElementBoundingBox(yyData, yyElement, yyFont);
			}
		;

element_1.3.4_format
			/* element_flags, description, pcb-name,
			 * text_x, text_y, text_direction, text_scale, text_flags
			 */
		: T_ELEMENT '(' INTEGER STRING STRING measure measure measure measure INTEGER ')' '('
			{
				yyElement = CreateNewElement(yyData, yyFont, OldFlags($3),
					$4, $5, NULL, OU ($6), OU ($7), IV ($8), IV ($9), OldFlags($10), false);
				free ($4);
				free ($5);
				pin_num = 1;
			}
		  elementdefinitions ')'
			{
				SetElementBoundingBox(yyData, yyElement, yyFont);
			}
		;

element_newformat
			/* element_flags, description, pcb-name, value, 
			 * text_x, text_y, text_direction, text_scale, text_flags
			 */
		: T_ELEMENT '(' INTEGER STRING STRING STRING measure measure measure measure INTEGER ')' '('
			{
				yyElement = CreateNewElement(yyData, yyFont, OldFlags($3),
					$4, $5, $6, OU ($7), OU ($8), IV ($9), IV ($10), OldFlags($11), false);
				free ($4);
				free ($5);
				free ($6);
				pin_num = 1;
			}
		  elementdefinitions ')'
			{
				SetElementBoundingBox(yyData, yyElement, yyFont);
			}
		;

element_1.7_format
			/* element_flags, description, pcb-name, value, mark_x, mark_y,
			 * text_x, text_y, text_direction, text_scale, text_flags
			 */
		: T_ELEMENT '(' INTEGER STRING STRING STRING measure measure
			measure measure number number INTEGER ')' '('
			{
				yyElement = CreateNewElement(yyData, yyFont, OldFlags($3),
					$4, $5, $6, OU ($7) + OU ($9), OU ($8) + OU ($10),
					$11, $12, OldFlags($13), false);
				yyElement->MarkX = OU ($7);
				yyElement->MarkY = OU ($8);
				free ($4);
				free ($5);
				free ($6);
			}
		  relementdefs ')'
			{
				SetElementBoundingBox(yyData, yyElement, yyFont);
			}
		;

element_hi_format
			/* element_flags, description, pcb-name, value, mark_x, mark_y,
			 * text_x, text_y, text_direction, text_scale, text_flags
			 */
		: T_ELEMENT '[' flags STRING STRING STRING measure measure
			measure measure number number flags ']' '('
			{
				yyElement = CreateNewElement(yyData, yyFont, $3,
					$4, $5, $6, NU ($7) + NU ($9), NU ($8) + NU ($10),
					$11, $12, $13, false);
				yyElement->MarkX = NU ($7);
				yyElement->MarkY = NU ($8);
				free ($4);
				free ($5);
				free ($6);
			}
		  relementdefs ')'
			{
				SetElementBoundingBox(yyData, yyElement, yyFont);
			}
		;

/* %start-doc pcbfile ElementLine

@syntax
ElementLine [X1 Y1 X2 Y2 Thickness]
ElementLine (X1 Y1 X2 Y2 Thickness)
@end syntax

@table @var
@item X1 Y1 X2 Y2
Coordinates of the endpoints of the line.  These are relative to the
Element's mark point for new element formats, or absolute for older
formats.
@item Thickness
The width of the silk for this line.
@end table

%end-doc */

/* %start-doc pcbfile ElementArc

@syntax
ElementArc [X Y Width Height StartAngle DeltaAngle Thickness]
ElementArc (X Y Width Height StartAngle DeltaAngle Thickness)
@end syntax

@table @var
@item X Y
Coordinates of the center of the arc.  These are relative to the
Element's mark point for new element formats, or absolute for older
formats.
@item Width Height
The width and height, from the center to the edge.  The bounds of the
circle of which this arc is a segment, is thus @math{2*Width} by
@math{2*Height}.
@item StartAngle
The angle of one end of the arc, in degrees.  In PCB, an angle of zero
points left (negative X direction), and 90 degrees points down
(positive Y direction).
@item DeltaAngle
The sweep of the arc.  This may be negative.  Positive angles sweep
counterclockwise.
@item Thickness
The width of the silk line which forms the arc.
@end table

%end-doc */

/* %start-doc pcbfile Mark

@syntax
Mark [X Y]
Mark (X Y)
@end syntax

@table @var
@item X Y
Coordinates of the Mark, for older element formats that don't have
the mark as part of the Element line.
@end table

%end-doc */

elementdefinitions
		: elementdefinition
		| elementdefinitions elementdefinition
		;

elementdefinition
		: pin_1.6.3_format
		| pin_newformat
		| pin_oldformat
		| pad_newformat
		| pad
			/* x1, y1, x2, y2, thickness */
		| T_ELEMENTLINE '[' measure measure measure measure measure ']'
			{
				CreateNewLineInElement(yyElement, NU ($3), NU ($4), NU ($5), NU ($6), NU ($7));
			}
			/* x1, y1, x2, y2, thickness */
		| T_ELEMENTLINE '(' measure measure measure measure measure ')'
			{
				CreateNewLineInElement(yyElement, OU ($3), OU ($4), OU ($5), OU ($6), OU ($7));
			}
			/* x, y, width, height, startangle, anglediff, thickness */
		| T_ELEMENTARC '[' measure measure measure measure number number measure ']'
			{
				CreateNewArcInElement(yyElement, NU ($3), NU ($4), NU ($5), NU ($6), $7, $8, NU ($9));
			}
			/* x, y, width, height, startangle, anglediff, thickness */
		| T_ELEMENTARC '(' measure measure measure measure number number measure ')'
			{
				CreateNewArcInElement(yyElement, OU ($3), OU ($4), OU ($5), OU ($6), $7, $8, OU ($9));
			}
			/* x, y position */
		| T_MARK '[' measure measure ']'
			{
				yyElement->MarkX = NU ($3);
				yyElement->MarkY = NU ($4);
			}
		| T_MARK '(' measure measure ')'
			{
				yyElement->MarkX = OU ($3);
				yyElement->MarkY = OU ($4);
			}
		| { attr_list = & yyElement->Attributes; } attribute
		;

relementdefs
		: relementdef
		| relementdefs relementdef
		;

relementdef
		: pin_1.7_format
		| pin_hi_format
		| pad_1.7_format
		| pad_hi_format
			/* x1, y1, x2, y2, thickness */
		| T_ELEMENTLINE '[' measure measure measure measure measure ']'
			{
				CreateNewLineInElement(yyElement, NU ($3) + yyElement->MarkX,
					NU ($4) + yyElement->MarkY, NU ($5) + yyElement->MarkX,
					NU ($6) + yyElement->MarkY, NU ($7));
			}
		| T_ELEMENTLINE '(' measure measure measure measure measure ')'
			{
				CreateNewLineInElement(yyElement, OU ($3) + yyElement->MarkX,
					OU ($4) + yyElement->MarkY, OU ($5) + yyElement->MarkX,
					OU ($6) + yyElement->MarkY, OU ($7));
			}
			/* x, y, width, height, startangle, anglediff, thickness */
		| T_ELEMENTARC '[' measure measure measure measure number number measure ']'
			{
				CreateNewArcInElement(yyElement, NU ($3) + yyElement->MarkX,
					NU ($4) + yyElement->MarkY, NU ($5), NU ($6), $7, $8, NU ($9));
			}
		| T_ELEMENTARC '(' measure measure measure measure number number measure ')'
			{
				CreateNewArcInElement(yyElement, OU ($3) + yyElement->MarkX,
					OU ($4) + yyElement->MarkY, OU ($5), OU ($6), $7, $8, OU ($9));
			}
		| { attr_list = & yyElement->Attributes; } attribute
		;

/* %start-doc pcbfile Pin

@syntax
Pin [rX rY Thickness Clearance Mask Drill "Name" "Number" SFlags]
Pin (rX rY Thickness Clearance Mask Drill "Name" "Number" NFlags)
Pin (aX aY Thickness Drill "Name" "Number" NFlags)
Pin (aX aY Thickness Drill "Name" NFlags)
Pin (aX aY Thickness "Name" NFlags)
@end syntax

@table @var
@item rX rY
coordinates of center, relative to the element's mark
@item aX aY
absolute coordinates of center.
@item Thickness
outer diameter of copper annulus
@item Clearance
add to thickness to get clearance diameter
@item Mask
diameter of solder mask opening
@item Drill
diameter of drill
@item Name
name of pin
@item Number
number of pin
@item SFlags
symbolic or numerical flags
@item NFlags
numerical flags only
@end table

%end-doc */

pin_hi_format
			/* x, y, thickness, clearance, mask, drilling hole, name,
			   number, flags */
		: T_PIN '[' measure measure measure measure measure measure STRING STRING flags ']'
			{
				CreateNewPin(yyElement, NU ($3) + yyElement->MarkX,
					NU ($4) + yyElement->MarkY, NU ($5), NU ($6), NU ($7), NU ($8), $9,
					$10, $11);
				free ($9);
				free ($10);
			}
		;
pin_1.7_format
			/* x, y, thickness, clearance, mask, drilling hole, name,
			   number, flags */
		: T_PIN '(' measure measure measure measure measure measure STRING STRING INTEGER ')'
			{
				CreateNewPin(yyElement, OU ($3) + yyElement->MarkX,
					OU ($4) + yyElement->MarkY, OU ($5), OU ($6), OU ($7), OU ($8), $9,
					$10, OldFlags($11));
				free ($9);
				free ($10);
			}
		;

pin_1.6.3_format
			/* x, y, thickness, drilling hole, name, number, flags */
		: T_PIN '(' measure measure measure measure STRING STRING INTEGER ')'
			{
				CreateNewPin(yyElement, OU ($3), OU ($4), OU ($5), 2*GROUNDPLANEFRAME,
					OU ($5) + 2*MASKFRAME, OU ($6), $7, $8, OldFlags($9));
				free ($7);
				free ($8);
			}
		;

pin_newformat
			/* x, y, thickness, drilling hole, name, flags */
		: T_PIN '(' measure measure measure measure STRING INTEGER ')'
			{
				char	p_number[8];

				sprintf(p_number, "%d", pin_num++);
				CreateNewPin(yyElement, OU ($3), OU ($4), OU ($5), 2*GROUNDPLANEFRAME,
					OU ($5) + 2*MASKFRAME, OU ($6), $7, p_number, OldFlags($8));

				free ($7);
			}
		;

pin_oldformat
			/* old format: x, y, thickness, name, flags
			 * drilling hole is 40% of the diameter
			 */
		: T_PIN '(' measure measure measure STRING INTEGER ')'
			{
				Coord	hole = OU ($5) * DEFAULT_DRILLINGHOLE;
				char	p_number[8];

					/* make sure that there's enough copper left */
				if (OU ($5) - hole < MIN_PINORVIACOPPER && 
					OU ($5) > MIN_PINORVIACOPPER)
					hole = OU ($5) - MIN_PINORVIACOPPER;

				sprintf(p_number, "%d", pin_num++);
				CreateNewPin(yyElement, OU ($3), OU ($4), OU ($5), 2*GROUNDPLANEFRAME,
					OU ($5) + 2*MASKFRAME, hole, $6, p_number, OldFlags($7));
				free ($6);
			}
		;

/* %start-doc pcbfile Pad

@syntax
Pad [rX1 rY1 rX2 rY2 Thickness Clearance Mask "Name" "Number" SFlags]
Pad (rX1 rY1 rX2 rY2 Thickness Clearance Mask "Name" "Number" NFlags)
Pad (aX1 aY1 aX2 aY2 Thickness "Name" "Number" NFlags)
Pad (aX1 aY1 aX2 aY2 Thickness "Name" NFlags)
@end syntax

@table @var
@item rX1 rY1 rX2 rY2
Coordinates of the endpoints of the pad, relative to the element's
mark.  Note that the copper extends beyond these coordinates by half
the thickness.  To make a square or round pad, specify the same
coordinate twice.
@item aX1 aY1 aX2 aY2
Same, but absolute coordinates of the endpoints of the pad.
@item Thickness
width of the pad.
@item Clearance
add to thickness to get clearance width.
@item Mask
width of solder mask opening.
@item Name
name of pin
@item Number
number of pin
@item SFlags
symbolic or numerical flags
@item NFlags
numerical flags only
@end table

%end-doc */

pad_hi_format
			/* x1, y1, x2, y2, thickness, clearance, mask, name , pad number, flags */
		: T_PAD '[' measure measure measure measure measure measure measure STRING STRING flags ']'
			{
				CreateNewPad(yyElement, NU ($3) + yyElement->MarkX,
					NU ($4) + yyElement->MarkY,
					NU ($5) + yyElement->MarkX,
					NU ($6) + yyElement->MarkY, NU ($7), NU ($8), NU ($9),
					$10, $11, $12);
				free ($10);
				free ($11);
			}
		;

pad_1.7_format
			/* x1, y1, x2, y2, thickness, clearance, mask, name , pad number, flags */
		: T_PAD '(' measure measure measure measure measure measure measure STRING STRING INTEGER ')'
			{
				CreateNewPad(yyElement,OU ($3) + yyElement->MarkX,
					OU ($4) + yyElement->MarkY, OU ($5) + yyElement->MarkX,
					OU ($6) + yyElement->MarkY, OU ($7), OU ($8), OU ($9),
					$10, $11, OldFlags($12));
				free ($10);
				free ($11);
			}
		;

pad_newformat
			/* x1, y1, x2, y2, thickness, name , pad number, flags */
		: T_PAD '(' measure measure measure measure measure STRING STRING INTEGER ')'
			{
				CreateNewPad(yyElement,OU ($3),OU ($4),OU ($5),OU ($6),OU ($7), 2*GROUNDPLANEFRAME,
					OU ($7) + 2*MASKFRAME, $8, $9, OldFlags($10));
				free ($8);
				free ($9);
			}
		;

pad
			/* x1, y1, x2, y2, thickness, name and flags */
		: T_PAD '(' measure measure measure measure measure STRING INTEGER ')'
			{
				char		p_number[8];

				sprintf(p_number, "%d", pin_num++);
				CreateNewPad(yyElement,OU ($3),OU ($4),OU ($5),OU ($6),OU ($7), 2*GROUNDPLANEFRAME,
					OU ($7) + 2*MASKFRAME, $8,p_number, OldFlags($9));
				free ($8);
			}
		;

flags		: INTEGER	{ $$ = OldFlags($1); }
		| STRING	{ $$ = string_to_flags ($1, yyerror); free($1); }
		;

symbols
		: symbol
		| symbols symbol
		;

/* %start-doc pcbfile Symbol

@syntax
Symbol [Char Delta] (
Symbol (Char Delta) (
@ @ @ @dots{} symbol lines @dots{}
)
@end syntax

@table @var
@item Char
The character or numerical character value this symbol represents.
Characters must be in single quotes.
@item Delta
Additional space to allow after this character.
@end table

%end-doc */

symbol : symbolhead symboldata ')'

symbolhead	: T_SYMBOL '[' symbolid measure ']' '('
			{
				if ($3 <= 0 || $3 > MAX_FONTPOSITION)
				{
					yyerror("fontposition out of range");
					YYABORT;
				}
				Symbol = &yyFont->Symbol[$3];
				if (Symbol->Valid)
				{
					yyerror("symbol ID used twice");
					YYABORT;
				}
				Symbol->Valid = true;
				Symbol->Delta = NU ($4);
			}
		| T_SYMBOL '(' symbolid measure ')' '('
			{
				if ($3 <= 0 || $3 > MAX_FONTPOSITION)
				{
					yyerror("fontposition out of range");
					YYABORT;
				}
				Symbol = &yyFont->Symbol[$3];
				if (Symbol->Valid)
				{
					yyerror("symbol ID used twice");
					YYABORT;
				}
				Symbol->Valid = true;
				Symbol->Delta = OU ($4);
			}
		;

symbolid
		: INTEGER
		| CHAR_CONST
		;

symboldata
		: /* empty */
		| symboldata symboldefinition
		| symboldata hiressymbol
		;

/* %start-doc pcbfile SymbolLine

@syntax
SymbolLine [X1 Y1 X2 Y2 Thickness]
SymbolLine (X1 Y1 X2 Y2 Thickness)
@end syntax

@table @var
@item X1 Y1 X2 Y2
The endpoints of this line.
@item Thickness
The width of this line.
@end table

%end-doc */

symboldefinition
			/* x1, y1, x2, y2, thickness */
		: T_SYMBOLLINE '(' measure measure measure measure measure ')'
			{
				CreateNewLineInSymbol(Symbol, OU ($3), OU ($4), OU ($5), OU ($6), OU ($7));
			}
		;
hiressymbol
			/* x1, y1, x2, y2, thickness */
		: T_SYMBOLLINE '[' measure measure measure measure measure ']'
			{
				CreateNewLineInSymbol(Symbol, NU ($3), NU ($4), NU ($5), NU ($6), NU ($7));
			}
		;

/* %start-doc pcbfile Netlist

@syntax
Netlist ( ) (
@ @ @ @dots{} nets @dots{}
)
@end syntax

%end-doc */

pcbnetlist	: pcbnetdef
		|
		;
pcbnetdef
			/* net(...) net(...) ... */
		: T_NETLIST '(' ')' '('
		  nets ')'
		;

nets
		: netdefs
		|
		;

netdefs
		: net 
		| netdefs net
		;

/* %start-doc pcbfile Net

@syntax
Net ("Name" "Style") (
@ @ @ @dots{} connects @dots{}
)
@end syntax

@table @var
@item Name
The name of this net.
@item Style
The routing style that should be used when autorouting this net.
@end table

%end-doc */

net
			/* name style pin pin ... */
		: T_NET '(' STRING STRING ')' '('
			{
				Menu = CreateNewNet(&yyPCB->NetlistLib, $3, $4);
				free ($3);
				free ($4);
			}
		 connections ')'
		;

connections
		: conndefs
		|
		;

conndefs
		: conn
		| conndefs conn
		;

/* %start-doc pcbfile Connect

@syntax
Connect ("PinPad")
@end syntax

@table @var
@item PinPad
The name of a pin or pad which is included in this net.  Pin and Pad
names are named by the refdes and pin name, like @code{"U14-7"} for
pin 7 of U14, or @code{"T4-E"} for pin E of T4.
@end table

%end-doc */

conn
		: T_CONN '(' STRING ')'
			{
				CreateNewConnection(Menu, $3);
				free ($3);
			}
		;

/* %start-doc pcbfile Attribute

@syntax
Attribute ("Name" "Value")
@end syntax

Attributes allow boards and elements to have arbitrary data attached
to them, which is not directly used by PCB itself but may be of use by
other programs or users.

@table @var
@item Name
The name of the attribute

@item Value
The value of the attribute.  Values are always stored as strings, even
if the value is interpreted as, for example, a number.

@end table

%end-doc */

attribute
		: T_ATTRIBUTE '(' STRING STRING ')'
			{
			  CreateNewAttribute (attr_list, $3, $4 ? $4 : (char *)"");
				free ($3);
				free ($4);
			}
		;

opt_string	: STRING { $$ = $1; }
		| /* empty */ { $$ = 0; }
		;

number
		: FLOATING	{ $$ = $1; }
		| INTEGER	{ $$ = $1; }
		;

measure
		/* Default unit (no suffix) is cmil */
		: number	{ do_measure(&$$, $1, MIL_TO_COORD ($1) / 100.0, 0); }
		| number T_UMIL	{ M ($$, $1, MIL_TO_COORD ($1) / 1000000.0); }
		| number T_CMIL	{ M ($$, $1, MIL_TO_COORD ($1) / 100.0); }
		| number T_MIL	{ M ($$, $1, MIL_TO_COORD ($1)); }
		| number T_IN	{ M ($$, $1, INCH_TO_COORD ($1)); }
		| number T_NM	{ M ($$, $1, MM_TO_COORD ($1) / 1000000.0); }
		| number T_PX	{ M ($$, $1, MM_TO_COORD ($1) / 1000000.0); }
		| number T_UM	{ M ($$, $1, MM_TO_COORD ($1) / 1000.0); }
		| number T_MM	{ M ($$, $1, MM_TO_COORD ($1)); }
		| number T_M	{ M ($$, $1, MM_TO_COORD ($1) * 1000.0); }
		| number T_KM	{ M ($$, $1, MM_TO_COORD ($1) * 1000000.0); }
		;

%%

/* ---------------------------------------------------------------------------
 * error routine called by parser library
 */
int yyerror(const char * s)
{
	Message(_("ERROR parsing file '%s'\n"
		"    line:        %i\n"
		"    description: '%s'\n"),
		yyfilename, yylineno, s);
	return(0);
}

int yywrap()
{
  return 1;
}

static int
check_file_version (int ver)
{
  if ( ver > PCB_FILE_VERSION ) {
    Message (_("ERROR:  The file you are attempting to load is in a format\n"
	     "which is too new for this version of pcb.  To load this file\n"
	     "you need a version of pcb which is >= %d.  If you are\n"
	     "using a version built from git source, the source date\n"
	     "must be >= %d.  This copy of pcb can only read files\n"
	     "up to file version %d.\n"), ver, ver, PCB_FILE_VERSION);
    return 1;
  }
  
  return 0;
}

static void
do_measure (PLMeasure *m, Coord i, double d, int u)
{
  m->ival = i;
  m->bval = round (d);
  m->dval = d;
  m->has_units = u;
}

static int
integer_value (PLMeasure m)
{
  if (m.has_units)
    yyerror("units ignored here");
  return m.ival;
}

static Coord
old_units (PLMeasure m)
{
  if (m.has_units)
    return m.bval;
  return round (MIL_TO_COORD (m.ival));
}

static Coord
new_units (PLMeasure m)
{
  if (m.has_units)
    return m.bval;
  return round (MIL_TO_COORD (m.ival) / 100.0);
}
