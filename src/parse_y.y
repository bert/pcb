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

static	char	*rcsid = "$Id$";

/* grammar to parse ASCII input of PCB description
 */

#define GRIDFIT(x,g) (int)(0.5 + (int)(((x)+(g)/2.)/(g))*(g))
#include "global.h"
#include "create.h"
#include "data.h"
#include "error.h"
#include "mymem.h"
#include "misc.h"
#include "parse_l.h"
#include "remove.h"

#ifdef DMALLOC
# include <dmalloc.h> /* see http://dmalloc.com */
#endif

static	LayerTypePtr	Layer;
static	PolygonTypePtr	Polygon;
static	SymbolTypePtr	Symbol;
static	int		pin_num;
static	LibraryMenuTypePtr	Menu;
static	Boolean			LayerFlag[MAX_LAYER + 2];

extern	char			*yytext;		/* defined by LEX */
extern	PCBTypePtr		yyPCB;
extern	DataTypePtr		yyData;
extern	ElementTypePtr	yyElement;
extern	FontTypePtr		yyFont;
extern	int				yylineno;		/* linenumber */
extern	char			*yyfilename;	/* in this file */

%}

%union									/* define YYSTACK type */
{
	int		number;
	float		floating;
	char		*string;
}

%token	<number>	NUMBER CHAR_CONST	/* line thickness, coordinates ... */
%token  <floating>      FLOAT
%token	<string>	STRING				/* element names ... */

%token	T_PCB T_LAYER T_VIA T_RAT T_LINE T_ARC T_RECTANGLE T_TEXT T_ELEMENTLINE
%token	T_ELEMENT T_PIN T_PAD T_GRID T_FLAGS T_SYMBOL T_SYMBOLLINE T_CURSOR
%token	T_ELEMENTARC T_MARK T_GROUPS T_STYLES T_POLYGON T_NETLIST T_NET T_CONN
%token	T_THERMAL T_DRC

%type	<number>	symbolid

%%

parse
		: parsepcb
		| parsedata
		| parsefont
		| error { YYABORT; }
		;
		
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
					LayerFlag[i] = False;
				yyFont = &yyPCB->Font;
				yyData = yyPCB->Data;
			}
		  pcbname 
		  pcbgrid
		  pcbcursor
		  pcbthermal 
		  pcbdrc
		  pcbflags
		  pcbgroups
		  pcbstyles
		  pcbfont
		  pcbdata
		  pcbnetlist
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
					LayerFlag[i] = False;
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
				yyFont->Valid = False;
				for (i = 0; i <= MAX_FONTPOSITION; i++)
					yyFont->Symbol[i].Valid = False;
			}
		  symbols
			{
				yyFont->Valid = True;
		  		SetFontInfo(yyFont);
			}
		;

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
					Settings.DrawGrid = True;
				else
					Settings.DrawGrid = False;
			}
		;

pcb2grid
		: T_GRID '(' FLOAT NUMBER NUMBER NUMBER ')'
			{
				yyPCB->Grid = $3*100;
				yyPCB->GridOffsetX = $4*100;
				yyPCB->GridOffsetY = $5*100;
				if ($6)
					Settings.DrawGrid = True;
				else
					Settings.DrawGrid = False;
			}
		;
pcbhigrid
		: T_GRID '[' FLOAT NUMBER NUMBER NUMBER ']'
			{
				yyPCB->Grid = $3;
				yyPCB->GridOffsetX = $4;
				yyPCB->GridOffsetY = $5;
				if ($6)
					Settings.DrawGrid = True;
				else
					Settings.DrawGrid = False;
			}
		;

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

pcbthermal
		:
		| T_THERMAL '[' FLOAT ']'
			{
				yyPCB->ThermScale = $3;
			}
		;

pcbdrc
                :
		| T_DRC '[' NUMBER NUMBER NUMBER ']'
		        {
				Settings.Bloat = $3;
				Settings.Shrink = $4;
				Settings.minWid = $5;
			}
		;

pcbflags
		: T_FLAGS '(' NUMBER ')'
			{
				yyPCB->Flags = $3 & PCB_FLAGS;
			}
		|
		;

pcbgroups
		: T_GROUPS '(' STRING ')'
			{
				if (ParseGroupString($3, &yyPCB->LayerGroups))
				{
					Message("illegal layer-group string\n");
					YYABORT;
				}
			}
		|
		;
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

via_hi_format
			/* x, y, thickness, clearance, mask, drilling-hole, name, flags */
		: T_VIA '[' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER STRING NUMBER ']'
			{
				CreateNewVia(yyData, $3, $4, $5, $6, $7, $8, $9,
					($10 & OBJ_FLAGS) | VIAFLAG);
				SaveFree($9);
			}
		;

via_2.0_format
			/* x, y, thickness, clearance, mask, drilling-hole, name, flags */
		: T_VIA '(' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER STRING NUMBER ')'
			{
				CreateNewVia(yyData, $3*100, $4*100, $5*100, $6*100, $7*100, $8*100, $9,
					($10 & OBJ_FLAGS) | VIAFLAG);
				SaveFree($9);
			}
		;


via_1.7_format
			/* x, y, thickness, clearance, drilling-hole, name, flags */
		: T_VIA '(' NUMBER NUMBER NUMBER NUMBER NUMBER STRING NUMBER ')'
			{
				CreateNewVia(yyData, $3*100, $4*100, $5*100, $6*100, ($5 + $6)*100, $7*100, $8,
					($9 & OBJ_FLAGS) | VIAFLAG);
				SaveFree($8);
			}
		;

via_newformat
			/* x, y, thickness, drilling-hole, name, flags */
		: T_VIA '(' NUMBER NUMBER NUMBER NUMBER STRING NUMBER ')'
			{
				CreateNewVia(yyData, $3*100, $4*100, $5*100, 200*GROUNDPLANEFRAME,
					($5 + 2*MASKFRAME)*100,  $6*100, $7, ($8 & OBJ_FLAGS) | VIAFLAG);
				SaveFree($7);
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
					($5 + 2*MASKFRAME)*100, hole, $6, ($7 & OBJ_FLAGS) | VIAFLAG);
				SaveFree($6);
			}
		;

rats
		: T_RAT '[' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER ']'
			{
				CreateNewRat(yyData, $3, $4, $6, $7, $5, $8,
					Settings.RatThickness, ($9 & OBJ_FLAGS));
			}
		| T_RAT '(' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER ')'
			{
				CreateNewRat(yyData, $3*100, $4*100, $6*100, $7*100, $5, $8,
					Settings.RatThickness, ($9 & OBJ_FLAGS));
			}
		;

layer
			/* name */
		: T_LAYER '(' NUMBER STRING ')' '('
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

					/* memory for name is alread allocated */
				Layer->Name = $4;
				LayerFlag[$3-1] = True;
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
					$3*100, $4*100, ($3+$5)*100, ($4+$6)*100, $7 & OBJ_FLAGS);
			}
		| text_hi_format
		| text_newformat
		| text_oldformat
			/* flags are passed in */
		| T_POLYGON '(' NUMBER ')' '('
			{
				Polygon = CreateNewPolygon(Layer, $3 & OBJ_FLAGS);
			}
		  polygonpoints ')'
		  	{
					/* ignore junk */
				if (Polygon->PointN >= 3)
					SetPolygonBoundingBox(Polygon);
				else
				{
					Message("WARNING parsing file '%s'\n"
						"    line:        %i\n"
						"    description: 'ignored polygon (< 3 points)'\n",
						yyfilename, yylineno);
					DestroyObject(PCB->Data, POLYGON_TYPE, Layer, Polygon, Polygon);
				}
			}
		;

line_hi_format
			/* x1, y1, x2, y2, thickness, clearance, flags */
		: T_LINE '[' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER ']'
			{
				CreateNewLineOnLayer(Layer, $3, $4, $5, $6, $7, $8, $9 & OBJ_FLAGS);
			}
		;

line_1.7_format
			/* x1, y1, x2, y2, thickness, clearance, flags */
		: T_LINE '(' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER ')'
			{
				CreateNewLineOnLayer(Layer, $3*100, $4*100, $5*100, $6*100, $7*100, $8*100, $9 & OBJ_FLAGS);
			}
		;

line_oldformat
			/* x1, y1, x2, y2, thickness, flags */
		: T_LINE '(' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER ')'
			{
				/* elliminate old-style rat-lines */
			if (($8 & RATFLAG) == 0)
				CreateNewLineOnLayer(Layer, $3*100, $4*100, $5*100, $6*100, $7*100,
					200*GROUNDPLANEFRAME, $8 & OBJ_FLAGS);
			}
		;

arc_hi_format
			/* x, y, width, height, thickness, clearance, startangle, delta, flags */
		: T_ARC '[' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER ']'
			{
				CreateNewArcOnLayer(Layer, $3, $4, $5, $9, $10, $7, $8, $11 & OBJ_FLAGS);
			}
		;

arc_1.7_format
			/* x, y, width, height, thickness, clearance, startangle, delta, flags */
		: T_ARC '(' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER ')'
			{
				CreateNewArcOnLayer(Layer, $3*100, $4*100, $5*100, $9, $10, $7*100, $8*100, $11 & OBJ_FLAGS);
			}
		;

arc_oldformat
			/* x, y, width, height, thickness, startangle, delta, flags */
		: T_ARC '(' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER ')'
			{
				CreateNewArcOnLayer(Layer, $3*100, $4*100, $5*100, $8, $9,
					$7*100, 200*GROUNDPLANEFRAME, $10 & OBJ_FLAGS);
			}
		;

text_oldformat
			/* x, y, direction, text, flags */
		: T_TEXT '(' NUMBER NUMBER NUMBER STRING NUMBER ')'
			{
					/* use a default scale of 100% */
				CreateNewText(Layer,yyFont,$3*100, $4*100, $5, 100, $6, $7 & OBJ_FLAGS);
				SaveFree($6);
			}
		;

text_newformat
			/* x, y, direction, scale, text, flags */
		: T_TEXT '(' NUMBER NUMBER NUMBER NUMBER STRING NUMBER ')'
			{
				if ($8 & ONSILKFLAG)
				{
					LayerTypePtr lay = &yyData->Layer[MAX_LAYER +
						(($8 & ONSOLDERFLAG) ? SOLDER_LAYER : COMPONENT_LAYER)];

					CreateNewText(lay ,yyFont, $3*100, $4*100, $5, $6, $7, $8 & OBJ_FLAGS);
				}
				else
					CreateNewText(Layer, yyFont, $3*100, $4*100, $5, $6, $7, $8 & OBJ_FLAGS);
				SaveFree($7);
			}
		;
text_hi_format
			/* x, y, direction, scale, text, flags */
		: T_TEXT '[' NUMBER NUMBER NUMBER NUMBER STRING NUMBER ']'
			{
				if ($8 & ONSILKFLAG)
				{
					LayerTypePtr lay = &yyData->Layer[MAX_LAYER +
						(($8 & ONSOLDERFLAG) ? SOLDER_LAYER : COMPONENT_LAYER)];

					CreateNewText(lay ,yyFont, $3, $4, $5, $6, $7, $8 & OBJ_FLAGS);
				}
				else
					CreateNewText(Layer, yyFont, $3, $4, $5, $6, $7, $8 & OBJ_FLAGS);
				SaveFree($7);
			}
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
				yyElement = CreateNewElement(yyData, yyElement, yyFont, 0x0000,
					$3, $4, NULL, $5*100, $6*100, $7, 100, 0x0000, False);
				SaveFree($3);
				SaveFree($4);
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
				yyElement = CreateNewElement(yyData, yyElement, yyFont, $3,
					$4, $5, NULL, $6*100, $7*100, $8, $9, $10, False);
				SaveFree($4);
				SaveFree($5);
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
				yyElement = CreateNewElement(yyData, yyElement, yyFont, $3,
					$4, $5, $6, $7*100, $8*100, $9, $10, $11, False);
				SaveFree($4);
				SaveFree($5);
				SaveFree($6);
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
				yyElement = CreateNewElement(yyData, yyElement, yyFont, $3,
					$4, $5, $6, ($7+$9)*100, ($8+$10)*100, $11, $12, $13, False);
				yyElement->MarkX = $7*100;
				yyElement->MarkY = $8*100;
				SaveFree($4);
				SaveFree($5);
				SaveFree($6);
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
		: T_ELEMENT '[' NUMBER STRING STRING STRING NUMBER NUMBER
			NUMBER NUMBER NUMBER NUMBER NUMBER ']' '('
			{
				yyElement = CreateNewElement(yyData, yyElement, yyFont, $3,
					$4, $5, $6, ($7+$9), ($8+$10), $11, $12, $13, False);
				yyElement->MarkX = $7;
				yyElement->MarkY = $8;
				SaveFree($4);
				SaveFree($5);
				SaveFree($6);
			}
		  relementdefs ')'
			{
				SetElementBoundingBox(yyData, yyElement, yyFont);
			}
		;

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
			/* x, y, width, height, startangle, anglediff, thickness */
		| T_ELEMENTLINE '(' NUMBER NUMBER NUMBER NUMBER NUMBER ')'
			{
				CreateNewLineInElement(yyElement, $3*100, $4*100, $5*100, $6*100, $7*100);
			}
			/* x, y, width, height, startangle, anglediff, thickness */
		| T_ELEMENTARC '[' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER ']'
			{
				CreateNewArcInElement(yyElement, $3, $4, $5, $6, $7, $8, $9);
			}
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
		;

pin_hi_format
			/* x, y, thickness, clearance, mask, drilling hole, name,
			   number, flags */
		: T_PIN '[' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER STRING STRING NUMBER ']'
			{
				CreateNewPin(yyElement, $3 + yyElement->MarkX,
					$4 + yyElement->MarkY, $5, $6, $7, $8, $9,
					$10, ($11 & OBJ_FLAGS) | PINFLAG);
				SaveFree($9);
				SaveFree($10);
			}
		;
pin_1.7_format
			/* x, y, thickness, clearance, mask, drilling hole, name,
			   number, flags */
		: T_PIN '(' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER STRING STRING NUMBER ')'
			{
				CreateNewPin(yyElement, $3*100 + yyElement->MarkX,
					$4*100 + yyElement->MarkY, $5*100, $6*100, $7*100, $8*100, $9,
					$10, ($11 & OBJ_FLAGS) | PINFLAG);
				SaveFree($9);
				SaveFree($10);
			}
		;

pin_1.6.3_format
			/* x, y, thickness, drilling hole, name, number, flags */
		: T_PIN '(' NUMBER NUMBER NUMBER NUMBER STRING STRING NUMBER ')'
			{
				CreateNewPin(yyElement, $3*100, $4*100, $5*100, 200*GROUNDPLANEFRAME,
					($5 + 2*MASKFRAME)*100, $6*100, $7, $8,
					($9 & OBJ_FLAGS) | PINFLAG);
				SaveFree($7);
				SaveFree($8);
			}
		;

pin_newformat
			/* x, y, thickness, drilling hole, name, flags */
		: T_PIN '(' NUMBER NUMBER NUMBER NUMBER STRING NUMBER ')'
			{
				char	p_number[8];

				sprintf(p_number, "%d", pin_num++);
				CreateNewPin(yyElement, $3*100, $4*100, $5*100, 200*GROUNDPLANEFRAME,
					($5 + 2*MASKFRAME)*100, $6*100, $7, p_number,
					($8 & OBJ_FLAGS) | PINFLAG);
				SaveFree($7);
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
					($5 + 2*MASKFRAME)*100, hole, $6, p_number,
					($7 & OBJ_FLAGS) | PINFLAG);
				SaveFree($6);
			}
		;

pad_hi_format
			/* x1, y1, x2, y2, thickness, clearance, mask, name , pad number, flags */
		: T_PAD '[' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER STRING STRING NUMBER ']'
			{
				CreateNewPad(yyElement, $3 + yyElement->MarkX,
					$4 + yyElement->MarkY,
					$5 + yyElement->MarkX,
					$6 + yyElement->MarkY, $7, $8, $9,
					$10, $11, ($12 & OBJ_FLAGS));
				SaveFree($10);
				SaveFree($11);
			}
		;

pad_1.7_format
			/* x1, y1, x2, y2, thickness, clearance, mask, name , pad number, flags */
		: T_PAD '(' NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER STRING STRING NUMBER ')'
			{
				CreateNewPad(yyElement,$3*100 + yyElement->MarkX,
					$4*100 + yyElement->MarkY, $5*100 + yyElement->MarkX,
					$6*100 + yyElement->MarkY, $7*100, $8*100, $9*100,
					$10, $11, ($12 & OBJ_FLAGS));
				SaveFree($10);
				SaveFree($11);
			}
		;

pad_newformat
			/* x1, y1, x2, y2, thickness, name , pad number, flags */
		: T_PAD '(' NUMBER NUMBER NUMBER NUMBER NUMBER STRING STRING NUMBER ')'
			{
				CreateNewPad(yyElement,$3*100,$4*100,$5*100,$6*100,$7*100, 200*GROUNDPLANEFRAME,
					($7 + 2*MASKFRAME)*100, $8,$9, ($10 & OBJ_FLAGS));
				SaveFree($8);
				SaveFree($9);
			}
		;

pad
			/* x1, y1, x2, y2, thickness, name and flags */
		: T_PAD '(' NUMBER NUMBER NUMBER NUMBER NUMBER STRING NUMBER ')'
			{
				char		p_number[8];

				sprintf(p_number, "%d", pin_num++);
				CreateNewPad(yyElement,$3*100,$4*100,$5*100,$6*100,$7*100, 200*GROUNDPLANEFRAME,
					($7 + 2*MASKFRAME)*100, $8,p_number,
					($9 & OBJ_FLAGS));
				SaveFree($8);
			}
		;

symbols
		: symbol
		| symbols symbol
		;

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
				Symbol->Valid = True;
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
				Symbol->Valid = True;
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

net
			/* name style pin pin ... */
		: T_NET '(' STRING STRING ')' '('
			{
				Menu = CreateNewNet(&yyPCB->NetlistLib, $3, $4);
				SaveFree($3);
				SaveFree($4);
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

conn
		: T_CONN '(' STRING ')'
			{
				CreateNewConnection(Menu, $3);
				SaveFree($3);
			}
		;
%%

/* ---------------------------------------------------------------------------
 * error routine called by parser library
 */
int yyerror(s)
char *s;
{
	Message("ERROR parsing file '%s'\n"
		"    line:        %i\n"
		"    description: '%s'\n",
		yyfilename, yylineno, s);
	return(0);
}

