/* $Id$ */
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

#define GRIDFIT(x,g) (int)(0.5 + (int)(((x)+(g)/2.)/(g))*(g))
#include "global.h"
#include "create.h"
#include "data.h"
#include "error.h"
#include "file.h"
#include "mymem.h"
#include "misc.h"
#include "parse_l.h"
#include "polygon.h"
#include "remove.h"
#include "rtree.h"
#include "strflags.h"
#include "thermal.h"

#ifdef HAVE_LIBDMALLOC
# include <dmalloc.h> /* see http://dmalloc.com */
#endif

RCSID("$Id$");

static	LayerTypePtr	Layer;
static	PolygonTypePtr	Polygon;
static	SymbolTypePtr	Symbol;
static	int		pin_num;
static	LibraryMenuTypePtr	Menu;
static	bool			LayerFlag[MAX_LAYER + 2];

extern	char			*yytext;		/* defined by LEX */
extern	PCBTypePtr		yyPCB;
extern	DataTypePtr		yyData;
extern	ElementTypePtr	yyElement;
extern	FontTypePtr		yyFont;
extern	int				yylineno;		/* linenumber */
extern	char			*yyfilename;	/* in this file */

static char *layer_group_string; 

static AttributeListTypePtr attr_list; 

int yyerror(const char *s);
int yylex();
static int check_file_version (int);

%}

%union									/* define YYSTACK type */
{
	int		number;
	float		floating;
	char		*string;
	FlagType	flagtype;
}

%token	<number>	NUMBER CHAR_CONST	/* line thickness, coordinates ... */
%token  <floating>      FLOAT
%token	<string>	STRING				/* element names ... */

%token	T_FILEVERSION T_PCB T_LAYER T_VIA T_RAT T_LINE T_ARC T_RECTANGLE T_TEXT T_ELEMENTLINE
%token	T_ELEMENT T_PIN T_PAD T_GRID T_FLAGS T_SYMBOL T_SYMBOLLINE T_CURSOR
%token	T_ELEMENTARC T_MARK T_GROUPS T_STYLES T_POLYGON T_POLYGON_HOLE T_NETLIST T_NET T_CONN
%token	T_AREA T_THERMAL T_DRC T_ATTRIBUTE
%type	<number>	symbolid
%type	<string>	opt_string
%type	<flagtype>	flags

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
					Message("illegal fileformat\n");
					YYABORT;
				}
				for (i = 0; i < MAX_LAYER + 2; i++)
					LayerFlag[i] = false;
				yyFont = &yyPCB->Font;
				yyData = yyPCB->Data;
				yyData->pcb = (void *)yyPCB;
				yyData->LayerN = 0;
				layer_group_string = NULL;
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
			  int i, j;
			  PCBTypePtr pcb_save = PCB;

			  if (layer_group_string == NULL)
			    layer_group_string = Settings.Groups;
			  CreateNewPCBPost (yyPCB, 0);
			  if (ParseGroupString(layer_group_string, &yyPCB->LayerGroups, yyData->LayerN))
			    {
			      Message("illegal layer-group string\n");
			      YYABORT;
			    }
			/* initialize the polygon clipping now since
			 * we didn't know the layer grouping before.
			 */
			PCB = yyPCB;
			for (i = 0; i < yyData->LayerN+2; i++)
			  for (j = 0; j < yyData->Layer[i].PolygonN; j++)
			      InitClip (yyData, &yyData->Layer[i], &yyData->Layer[i].Polygon[j]);
			PCB = pcb_save;
			}
			   
		| { PreLoadElementPCB ();
		    layer_group_string = NULL; }
		  element
		  { LayerFlag[0] = true;
		    LayerFlag[1] = true;
		    yyData->LayerN = 2;
		    PostLoadElementPCB ();
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
					Message("illegal fileformat\n");
					YYABORT;
				}
				for (i = 0; i < MAX_LAYER + 2; i++)
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
					Message("illegal fileformat\n");
					YYABORT;
				}
				yyFont->Valid = false;
				for (i = 0; i <= MAX_FONTPOSITION; i++)
					yyFont->Symbol[i].Valid = false;
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
T_FILEVERSION '[' NUMBER ']'
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
		| T_PCB '(' STRING NUMBER NUMBER ')'
			{
				yyPCB->Name = $3;
				yyPCB->MaxWidth = $4*100;
				yyPCB->MaxHeight = $5*100;
			}
		| T_PCB '[' STRING NUMBER NUMBER ']'
			{
				yyPCB->Name = $3;
				yyPCB->MaxWidth = $4;
				yyPCB->MaxHeight = $5;
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
		| pcb2grid
		| pcbhigrid
		;
pcbgridold
		: T_GRID '(' NUMBER NUMBER NUMBER ')'
			{
				yyPCB->Grid = $3*100;
				yyPCB->GridOffsetX = $4*100;
				yyPCB->GridOffsetY = $5*100;
			}
		;
pcbgridnew
		: T_GRID '(' NUMBER NUMBER NUMBER NUMBER ')'
			{
				yyPCB->Grid = $3*100;
				yyPCB->GridOffsetX = $4*100;
				yyPCB->GridOffsetY = $5*100;
				if ($6)
					Settings.DrawGrid = true;
				else
					Settings.DrawGrid = false;
			}
		;

pcb2grid
		: T_GRID '(' FLOAT NUMBER NUMBER NUMBER ')'
			{
				yyPCB->Grid = $3*100;
				yyPCB->GridOffsetX = $4*100;
				yyPCB->GridOffsetY = $5*100;
				if ($6)
					Settings.DrawGrid = true;
				else
					Settings.DrawGrid = false;
			}
		;
pcbhigrid
		: T_GRID '[' FLOAT NUMBER NUMBER NUMBER ']'
			{
				yyPCB->Grid = $3;
				yyPCB->GridOffsetX = $4;
				yyPCB->GridOffsetY = $5;
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
@item Zoom
The current zoom factor.  Note that a zoom factor of "0" means 1 mil
per screen pixel, N means @math{2^N} mils per screen pixel, etc.  The
first variant accepts floating point numbers.  The special value
"1000" means "zoom to fit"
@end table

%end-doc */

pcbcursor
		: T_CURSOR '(' NUMBER NUMBER NUMBER ')'
			{
				yyPCB->CursorX = $3*100;
				yyPCB->CursorY = $4*100;
				yyPCB->Zoom = $5*2;
			}
		| T_CURSOR '[' NUMBER NUMBER NUMBER ']'
			{
				yyPCB->CursorX = $3;
				yyPCB->CursorY = $4;
				yyPCB->Zoom = $5;
			}
		| T_CURSOR '[' NUMBER NUMBER FLOAT ']'
			{
				yyPCB->CursorX = $3;
				yyPCB->CursorY = $4;
				yyPCB->Zoom = $5;
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
		| T_AREA '[' FLOAT ']'
			{
				yyPCB->IsleArea = $3;
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
		| T_THERMAL '[' FLOAT ']'
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
                : T_DRC '[' NUMBER NUMBER NUMBER ']'
		        {
				yyPCB->Bloat = $3;
				yyPCB->Shrink = $4;
				yyPCB->minWid = $5;
				yyPCB->minRing = $5;
			}
		;

pcbdrc2
                : T_DRC '[' NUMBER NUMBER NUMBER NUMBER ']'
		        {
				yyPCB->Bloat = $3;
				yyPCB->Shrink = $4;
				yyPCB->minWid = $5;
				yyPCB->minSlk = $6;
				yyPCB->minRing = $5;
			}
		;

pcbdrc3
                : T_DRC '[' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER ']'
		        {
				yyPCB->Bloat = $3;
				yyPCB->Shrink = $4;
				yyPCB->minWid = $5;
				yyPCB->minSlk = $6;
				yyPCB->minDrill = $7;
				yyPCB->minRing = $8;
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
		: T_FLAGS '(' NUMBER ')'
			{
				yyPCB->Flags = MakeFlags ($3 & PCB_FLAGS);
			}
		| T_FLAGS '(' STRING ')'
			{
			  yyPCB->Flags = string_to_pcbflags ($3, yyerror);
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
			  layer_group_string = $3;
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
				if (ParseRouteString($3, &yyPCB->RouteStyle[0], 100))
				{
					Message("illegal route-style string\n");
					YYABORT;
				}
			}
		| T_STYLES '[' STRING ']'
			{
				if (ParseRouteString($3, &yyPCB->RouteStyle[0], 1))
				{
					Message("illegal route-style string\n");
					YYABORT;
				}
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
		| { attr_list = & yyPCB->Attributes; } attributes
		| rats
		| layer
		|
			{
					/* clear pointer to force memory allocation by 
					 * the appropriate subroutine
					 */
				yyElement = NULL;
			}
		  element
		| error { YYABORT; }
		;

via
		: via_hi_format
		| via_2.0_format
		| via_1.7_format
		| via_newformat
		| via_oldformat
		;

/* %start-doc pcbfile Via

@syntax
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

via_hi_format
			/* x, y, thickness, clearance, mask, drilling-hole, name, flags */
		: T_VIA '[' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER STRING flags ']'
			{
				CreateNewVia(yyData, $3, $4, $5, $6, $7, $8, $9, $10);
				free ($9);
			}
		;

via_2.0_format
			/* x, y, thickness, clearance, mask, drilling-hole, name, flags */
		: T_VIA '(' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER STRING NUMBER ')'
			{
				CreateNewVia(yyData, $3*100, $4*100, $5*100, $6*100, $7*100, $8*100, $9,
					OldFlags($10));
				free ($9);
			}
		;


via_1.7_format
			/* x, y, thickness, clearance, drilling-hole, name, flags */
		: T_VIA '(' NUMBER NUMBER NUMBER NUMBER NUMBER STRING NUMBER ')'
			{
				CreateNewVia(yyData, $3*100, $4*100, $5*100, $6*100,
					     ($5 + $6)*100, $7*100, $8, OldFlags($9));
				free ($8);
			}
		;

via_newformat
			/* x, y, thickness, drilling-hole, name, flags */
		: T_VIA '(' NUMBER NUMBER NUMBER NUMBER STRING NUMBER ')'
			{
				CreateNewVia(yyData, $3*100, $4*100, $5*100, 200*GROUNDPLANEFRAME,
					($5 + 2*MASKFRAME)*100,  $6*100, $7, OldFlags($8));
				free ($7);
			}
		;

via_oldformat
			/* old format: x, y, thickness, name, flags */
		: T_VIA '(' NUMBER NUMBER NUMBER STRING NUMBER ')'
			{
				BDimension	hole = ($5 *DEFAULT_DRILLINGHOLE);

					/* make sure that there's enough copper left */
				if ($5 -hole < MIN_PINORVIACOPPER && 
					$5 > MIN_PINORVIACOPPER)
					hole = $5 -MIN_PINORVIACOPPER;

				CreateNewVia(yyData, $3*100, $4*100, $5*100, 200*GROUNDPLANEFRAME,
					($5 + 2*MASKFRAME)*100, hole, $6, OldFlags($7));
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
		: T_RAT '[' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER flags ']'
			{
				CreateNewRat(yyData, $3, $4, $6, $7, $5, $8,
					Settings.RatThickness, $9);
			}
		| T_RAT '(' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER ')'
			{
				CreateNewRat(yyData, $3*100, $4*100, $6*100, $7*100, $5, $8,
					Settings.RatThickness, OldFlags($9));
			}
		;

/* %start-doc pcbfile Layer

@syntax
Layer (LayerNum "Name") (
@ @ @ @dots{} contents @dots{}
)
@end syntax

@table @var
@item LayerNum
The layer number.  Layers are numbered sequentially, starting with 1.
The last two layers (9 and 10 by default) are solder-side silk and
component-side silk, in that order.
@item Name
The layer name.
@item contents
The contents of the layer, which may include attributes, lines, arcs, rectangles,
text, and polygons.
@end table

%end-doc */

layer
			/* name */
		: T_LAYER '(' NUMBER STRING opt_string ')' '('
			{
				if ($3 <= 0 || $3 > MAX_LAYER + 2)
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
				LayerFlag[$3-1] = true;
				if (yyData->LayerN + 2 < $3)
				  yyData->LayerN = $3 - 2;
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
		| T_RECTANGLE '(' NUMBER NUMBER NUMBER NUMBER NUMBER ')'
			{
				CreateNewPolygonFromRectangle(Layer,
					$3*100, $4*100, ($3+$5)*100, ($4+$6)*100, OldFlags($7));
			}
		| text_hi_format
		| text_newformat
		| text_oldformat
		| { attr_list = & Layer->Attributes; } attributes
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
		: T_LINE '[' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER flags ']'
			{
				CreateNewLineOnLayer(Layer, $3, $4, $5, $6, $7, $8, $9);
			}
		;

line_1.7_format
			/* x1, y1, x2, y2, thickness, clearance, flags */
		: T_LINE '(' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER ')'
			{
				CreateNewLineOnLayer(Layer, $3*100, $4*100, $5*100, $6*100,
						     $7*100, $8*100, OldFlags($9));
			}
		;

line_oldformat
			/* x1, y1, x2, y2, thickness, flags */
		: T_LINE '(' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER ')'
			{
				/* eliminate old-style rat-lines */
			if (($8 & RATFLAG) == 0)
				CreateNewLineOnLayer(Layer, $3*100, $4*100, $5*100, $6*100, $7*100,
					200*GROUNDPLANEFRAME, OldFlags($8));
			}
		;

/* %start-doc pcbfile Arc

@syntax
Arc [X Y Width Height Thickness Clearance StartAngle DeltaAngle SFlags]
Arc (X Y Width Height Thickness Clearance StartAngle DeltaAngle NFlags)
Arc (X Y Width Height Thickness StartAngle DeltaAngle NFlags)
@end syntax

@table @var
@item X Y
Coordinates of the center of the arc.
@item Width Height
The width and height, from the center to the edge.  The bounds of the
circle of which this arc is a segment, is thus @math{2*Width} by
@math{2*Height}.
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
		: T_ARC '[' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER flags ']'
			{
			  CreateNewArcOnLayer(Layer, $3, $4, $5, $6, $9, $10, $7, $8, $11);
			}
		;

arc_1.7_format
			/* x, y, width, height, thickness, clearance, startangle, delta, flags */
		: T_ARC '(' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER ')'
			{
				CreateNewArcOnLayer(Layer, $3*100, $4*100, $5*100, $6*100, $9, $10,
						    $7*100, $8*100, OldFlags($11));
			}
		;

arc_oldformat
			/* x, y, width, height, thickness, startangle, delta, flags */
		: T_ARC '(' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER ')'
			{
				CreateNewArcOnLayer(Layer, $3*100, $4*100, $5*100, $5*100, $8, $9,
					$7*100, 200*GROUNDPLANEFRAME, OldFlags($10));
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
		: T_TEXT '(' NUMBER NUMBER NUMBER STRING NUMBER ')'
			{
					/* use a default scale of 100% */
				CreateNewText(Layer,yyFont,$3*100, $4*100, $5, 100, $6, OldFlags($7));
				free ($6);
			}
		;

text_newformat
			/* x, y, direction, scale, text, flags */
		: T_TEXT '(' NUMBER NUMBER NUMBER NUMBER STRING NUMBER ')'
			{
				if ($8 & ONSILKFLAG)
				{
					LayerTypePtr lay = &yyData->Layer[yyData->LayerN +
						(($8 & ONSOLDERFLAG) ? SOLDER_LAYER : COMPONENT_LAYER)];

					CreateNewText(lay ,yyFont, $3*100, $4*100, $5, $6, $7,
						      OldFlags($8));
				}
				else
					CreateNewText(Layer, yyFont, $3*100, $4*100, $5, $6, $7,
						      OldFlags($8));
				free ($7);
			}
		;
text_hi_format
			/* x, y, direction, scale, text, flags */
		: T_TEXT '[' NUMBER NUMBER NUMBER NUMBER STRING flags ']'
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
					LayerTypePtr lay = &yyData->Layer[yyData->LayerN +
						(($8.f & ONSOLDERFLAG) ? SOLDER_LAYER : COMPONENT_LAYER)];

					CreateNewText(lay, yyFont, $3, $4, $5, $6, $7, $8);
				}
				else
					CreateNewText(Layer, yyFont, $3, $4, $5, $6, $7, $8);
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
				    Message("WARNING parsing file '%s'\n"
					    "    line:        %i\n"
					    "    description: 'ignored polygon (< 3 points in a contour)'\n",
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
		| polygonhole
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
		: polygonpoint
		| polygonpoints polygonpoint
		;

polygonpoint
			/* xcoord ycoord */
		: '(' NUMBER NUMBER ')'
			{
				CreateNewPointInPolygon(Polygon, $2*100, $3*100);
			}
		| '[' NUMBER NUMBER ']'
			{
				CreateNewPointInPolygon(Polygon, $2, $3);
			}
		|
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
		: T_ELEMENT '(' STRING STRING NUMBER NUMBER NUMBER ')' '('
			{
				yyElement = CreateNewElement(yyData, yyElement, yyFont, NoFlags(),
					$3, $4, NULL, $5*100, $6*100, $7, 100, NoFlags(), false);
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
		: T_ELEMENT '(' NUMBER STRING STRING NUMBER NUMBER NUMBER NUMBER NUMBER ')' '('
			{
				yyElement = CreateNewElement(yyData, yyElement, yyFont, OldFlags($3),
					$4, $5, NULL, $6*100, $7*100, $8, $9, OldFlags($10), false);
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
		: T_ELEMENT '(' NUMBER STRING STRING STRING NUMBER NUMBER NUMBER NUMBER NUMBER ')' '('
			{
				yyElement = CreateNewElement(yyData, yyElement, yyFont, OldFlags($3),
					$4, $5, $6, $7*100, $8*100, $9, $10, OldFlags($11), false);
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
		: T_ELEMENT '(' NUMBER STRING STRING STRING NUMBER NUMBER
			NUMBER NUMBER NUMBER NUMBER NUMBER ')' '('
			{
				yyElement = CreateNewElement(yyData, yyElement, yyFont, OldFlags($3),
					$4, $5, $6, ($7+$9)*100, ($8+$10)*100, $11, $12, OldFlags($13), false);
				yyElement->MarkX = $7*100;
				yyElement->MarkY = $8*100;
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
		: T_ELEMENT '[' flags STRING STRING STRING NUMBER NUMBER
			NUMBER NUMBER NUMBER NUMBER flags ']' '('
			{
				yyElement = CreateNewElement(yyData, yyElement, yyFont, $3,
					$4, $5, $6, ($7+$9), ($8+$10), $11, $12, $13, false);
				yyElement->MarkX = $7;
				yyElement->MarkY = $8;
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
		| T_ELEMENTLINE '[' NUMBER NUMBER NUMBER NUMBER NUMBER ']'
			{
				CreateNewLineInElement(yyElement, $3, $4, $5, $6, $7);
			}
			/* x1, y1, x2, y2, thickness */
		| T_ELEMENTLINE '(' NUMBER NUMBER NUMBER NUMBER NUMBER ')'
			{
				CreateNewLineInElement(yyElement, $3*100, $4*100, $5*100, $6*100, $7*100);
			}
			/* x, y, width, height, startangle, anglediff, thickness */
		| T_ELEMENTARC '[' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER ']'
			{
				CreateNewArcInElement(yyElement, $3, $4, $5, $6, $7, $8, $9);
			}
			/* x, y, width, height, startangle, anglediff, thickness */
		| T_ELEMENTARC '(' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER ')'
			{
				CreateNewArcInElement(yyElement, $3*100, $4*100, $5*100, $6*100, $7, $8, $9*100);
			}
			/* x, y position */
		| T_MARK '[' NUMBER NUMBER ']'
			{
				yyElement->MarkX = $3;
				yyElement->MarkY = $4;
			}
		| T_MARK '(' NUMBER NUMBER ')'
			{
				yyElement->MarkX = $3*100;
				yyElement->MarkY = $4*100;
			}
		| { attr_list = & yyElement->Attributes; } attributes
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
		| T_ELEMENTLINE '[' NUMBER NUMBER NUMBER NUMBER NUMBER ']'
			{
				CreateNewLineInElement(yyElement, $3 + yyElement->MarkX,
					$4 + yyElement->MarkY, $5 + yyElement->MarkX,
					$6 + yyElement->MarkY, $7);
			}
		| T_ELEMENTLINE '(' NUMBER NUMBER NUMBER NUMBER NUMBER ')'
			{
				CreateNewLineInElement(yyElement, $3*100 + yyElement->MarkX,
					$4*100 + yyElement->MarkY, $5*100 + yyElement->MarkX,
					$6*100 + yyElement->MarkY, $7*100);
			}
			/* x, y, width, height, startangle, anglediff, thickness */
		| T_ELEMENTARC '[' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER ']'
			{
				CreateNewArcInElement(yyElement, $3 + yyElement->MarkX,
					$4 + yyElement->MarkY, $5, $6, $7, $8, $9);
			}
		| T_ELEMENTARC '(' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER ')'
			{
				CreateNewArcInElement(yyElement, $3*100 + yyElement->MarkX,
					$4*100 + yyElement->MarkY, $5*100, $6*100, $7, $8, $9*100);
			}
		| { attr_list = & yyElement->Attributes; } attributes
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
		: T_PIN '[' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER STRING STRING flags ']'
			{
				CreateNewPin(yyElement, $3 + yyElement->MarkX,
					$4 + yyElement->MarkY, $5, $6, $7, $8, $9,
					$10, $11);
				free ($9);
				free ($10);
			}
		;
pin_1.7_format
			/* x, y, thickness, clearance, mask, drilling hole, name,
			   number, flags */
		: T_PIN '(' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER STRING STRING NUMBER ')'
			{
				CreateNewPin(yyElement, $3*100 + yyElement->MarkX,
					$4*100 + yyElement->MarkY, $5*100, $6*100, $7*100, $8*100, $9,
					$10, OldFlags($11));
				free ($9);
				free ($10);
			}
		;

pin_1.6.3_format
			/* x, y, thickness, drilling hole, name, number, flags */
		: T_PIN '(' NUMBER NUMBER NUMBER NUMBER STRING STRING NUMBER ')'
			{
				CreateNewPin(yyElement, $3*100, $4*100, $5*100, 200*GROUNDPLANEFRAME,
					($5 + 2*MASKFRAME)*100, $6*100, $7, $8, OldFlags($9));
				free ($7);
				free ($8);
			}
		;

pin_newformat
			/* x, y, thickness, drilling hole, name, flags */
		: T_PIN '(' NUMBER NUMBER NUMBER NUMBER STRING NUMBER ')'
			{
				char	p_number[8];

				sprintf(p_number, "%d", pin_num++);
				CreateNewPin(yyElement, $3*100, $4*100, $5*100, 200*GROUNDPLANEFRAME,
					($5 + 2*MASKFRAME)*100, $6*100, $7, p_number, OldFlags($8));

				free ($7);
			}
		;

pin_oldformat
			/* old format: x, y, thickness, name, flags
			 * drilling hole is 40% of the diameter
			 */
		: T_PIN '(' NUMBER NUMBER NUMBER STRING NUMBER ')'
			{
				BDimension	hole = ($5 *DEFAULT_DRILLINGHOLE);
				char		p_number[8];

					/* make sure that there's enough copper left */
				if ($5 -hole < MIN_PINORVIACOPPER && 
					$5 > MIN_PINORVIACOPPER)
					hole = $5 -MIN_PINORVIACOPPER;

				sprintf(p_number, "%d", pin_num++);
				CreateNewPin(yyElement, $3*100, $4*100, $5*100, 200*GROUNDPLANEFRAME,
					($5 + 2*MASKFRAME)*100, hole, $6, p_number, OldFlags($7));
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
		: T_PAD '[' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER STRING STRING flags ']'
			{
				CreateNewPad(yyElement, $3 + yyElement->MarkX,
					$4 + yyElement->MarkY,
					$5 + yyElement->MarkX,
					$6 + yyElement->MarkY, $7, $8, $9,
					$10, $11, $12);
				free ($10);
				free ($11);
			}
		;

pad_1.7_format
			/* x1, y1, x2, y2, thickness, clearance, mask, name , pad number, flags */
		: T_PAD '(' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER STRING STRING NUMBER ')'
			{
				CreateNewPad(yyElement,$3*100 + yyElement->MarkX,
					$4*100 + yyElement->MarkY, $5*100 + yyElement->MarkX,
					$6*100 + yyElement->MarkY, $7*100, $8*100, $9*100,
					$10, $11, OldFlags($12));
				free ($10);
				free ($11);
			}
		;

pad_newformat
			/* x1, y1, x2, y2, thickness, name , pad number, flags */
		: T_PAD '(' NUMBER NUMBER NUMBER NUMBER NUMBER STRING STRING NUMBER ')'
			{
				CreateNewPad(yyElement,$3*100,$4*100,$5*100,$6*100,$7*100, 200*GROUNDPLANEFRAME,
					($7 + 2*MASKFRAME)*100, $8,$9, OldFlags($10));
				free ($8);
				free ($9);
			}
		;

pad
			/* x1, y1, x2, y2, thickness, name and flags */
		: T_PAD '(' NUMBER NUMBER NUMBER NUMBER NUMBER STRING NUMBER ')'
			{
				char		p_number[8];

				sprintf(p_number, "%d", pin_num++);
				CreateNewPad(yyElement,$3*100,$4*100,$5*100,$6*100,$7*100, 200*GROUNDPLANEFRAME,
					($7 + 2*MASKFRAME)*100, $8,p_number, OldFlags($9));
				free ($8);
			}
		;

flags		: NUMBER	{ $$ = OldFlags($1); }
		| STRING	{ $$ = string_to_flags ($1, yyerror); }
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

symbol
		: T_SYMBOL '[' symbolid NUMBER ']' '('
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
				Symbol->Delta = $4;
			}
		  symboldata ')'
		| T_SYMBOL '(' symbolid NUMBER ')' '('
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
				Symbol->Delta = $4*100;
			}
		  symboldata ')'
		;

symbolid
		: NUMBER
		| CHAR_CONST
		;

symboldata
		: symboldefinitions
		| symboldata symboldefinitions
		;

symboldefinitions
		: symboldefinition
		| hiressymbol
		|
		;

/* %start-doc pcbfile SymbolLine

@syntax
SymbolLine [X1 Y1 X2 Y1 Thickness]
SymbolLine (X1 Y1 X2 Y1 Thickness)
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
		: T_SYMBOLLINE '(' NUMBER NUMBER NUMBER NUMBER NUMBER ')'
			{
				CreateNewLineInSymbol(Symbol, $3*100, $4*100, $5*100, $6*100, $7*100);
			}
		;
hiressymbol
			/* x1, y1, x2, y2, thickness */
		: T_SYMBOLLINE '[' NUMBER NUMBER NUMBER NUMBER NUMBER ']'
			{
				CreateNewLineInSymbol(Symbol, $3, $4, $5, $6, $7);
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

attributes	: attribute
		| attributes attribute
		;
attribute
		: T_ATTRIBUTE '(' STRING STRING ')'
			{
				CreateNewAttribute (attr_list, $3, $4 ? $4 : "");
				free ($3);
				free ($4);
			}
		;

opt_string	: STRING { $$ = $1; }
		| /* empty */ { $$ = 0; }
		;

%%

/* ---------------------------------------------------------------------------
 * error routine called by parser library
 */
int yyerror(s)
const char *s;
{
	Message("ERROR parsing file '%s'\n"
		"    line:        %i\n"
		"    description: '%s'\n",
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
    Message ("ERROR:  The file you are attempting to load is in a format\n"
	     "which is too new for this version of pcb.  To load this file\n"
	     "you need a version of pcb which is >= %d.  If you are\n"
	     "using a version built from git source, the source date\n"
	     "must be >= %d.  This copy of pcb can only read files\n"
	     "up to file version %d.\n", ver, ver, PCB_FILE_VERSION);
    return 1;
  }
  
  return 0;
}

