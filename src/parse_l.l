/* $Id$ */

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

/* lexical definitions to parse ASCII input of PCB and Element description
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#if defined(_POSIX_SOURCE) || defined(_HPUX_SOURCE)
#include <unistd.h>
#endif

#include "global.h"

#ifdef HAVE_LIBDMALLOC
# include <dmalloc.h> /* see http://dmalloc.com */
#endif

RCSID("$Id$");


#include "global.h"
#include "crosshair.h"
#include "data.h"
#include "error.h"
#include "file.h"
#include "gui.h"
#include "mymem.h"
#include "misc.h"
#include "parse_l.h"
#include "parse_y.h"

/* ---------------------------------------------------------------------------
 * some shared parser identifiers
 */
#ifdef FLEX_SCANNER
int				yylineno;		/* linenumber */
#endif

char			*yyfilename;	/* in this file */
PCBTypePtr		yyPCB;			/* used by parser */
DataTypePtr		yyData;
ElementTypePtr		yyElement;
FontTypePtr		yyFont;

/* ---------------------------------------------------------------------------
 * an external prototypes
 */
int	yyparse(void);

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static	int		Parse(char *, char *, char *, char *);

%}

HEX				0x[0-9a-fA-F]+
DECIMAL			-?[1-9][0-9]*|0
FLOATING                -?[0-9]*"."[0-9]*
CARDINAL		[1-9][0-9]*|0
STRINGCHAR		([^"\n\r\\]|\\\\|\\\")

%option yylineno

%%

PCB			{ return(T_PCB); }
Grid		{ return(T_GRID); }
Cursor		{ return(T_CURSOR); }
Thermal		{ return(T_THERMAL); }
DRC		{ return(T_DRC); }
Flags		{ return(T_FLAGS); }
Layer		{ return(T_LAYER); }
Pin			{ return(T_PIN); }
Pad			{ return(T_PAD); }
Via			{ return(T_VIA); }
Line		{ return(T_LINE); }
Rat		{ return(T_RAT); }
Rectangle	{ return(T_RECTANGLE); }
Text		{ return(T_TEXT); }
ElementLine	{ return(T_ELEMENTLINE); }
ElementArc	{ return(T_ELEMENTARC); }
Element		{ return(T_ELEMENT); }
SymbolLine	{ return(T_SYMBOLLINE); }
Symbol		{ return(T_SYMBOL); }
Mark		{ return(T_MARK); }
Groups		{ return(T_GROUPS); }
Styles		{ return(T_STYLES); }
Polygon		{ return(T_POLYGON); }
Arc		{ return(T_ARC); }
NetList		{ return(T_NETLIST); }
Net		{ return(T_NET); }
Connect		{ return(T_CONN); }

\'.\'				{
						yylval.number = (unsigned) *(yytext+1);
						return(CHAR_CONST);
					}
{DECIMAL}		{
						sscanf((char *) yytext, "%i", &yylval.number);
						return(NUMBER);
					}
{HEX}			{
						sscanf((char *) yytext, "%x", &yylval.number);
						return(NUMBER);
					}
{FLOATING}			{
						sscanf((char *) yytext, "%f", &yylval.floating);
						return(FLOAT);
					}
\"{STRINGCHAR}*\"	{
						char	*p1, *p2;

							/* return NULL on empty string */
						if (yyleng == 2)
						{
							yylval.string = NULL;
							return(STRING);
						}

							/* allocate memory and copy string;
							 * stringlength is counted and copied without
							 * leading and trailing '"'
							 */
						yyleng -= 2;
						yylval.string = MyCalloc(yyleng+1, sizeof(char), "LEX");
						p1 = (char *) (yytext +1);
						p2 = yylval.string;
						while(yyleng--)
						{
								/* check for special character */
							if (*p1 == '\\')
							{
								yyleng--;
								p1++;

							}
							*p2++ = *p1++;
						}
						*p2 = '\0';
						return(STRING);
					}
#.*					{}
[ \t]+				{}
[\n]				{
#ifndef FLEX_SCANNER
						yylineno++;
#endif
					}
[\r]				{}
.					{ return(*yytext); }

%%

/* ---------------------------------------------------------------------------
 * sets up the preprocessor command
 */
static int Parse(char *Executable, char *Path, char *Filename, char *Parameter)
{
	static	char	*command = NULL;
			int		returncode;

#ifdef FLEX_SCANNER
	static	Boolean	firsttime = True;
#endif

		/* release old command and create new from template */
	MyFree(&command);
	command = EvaluateFilename(Executable, Path, Filename, Parameter);

		/* open pipe to stdout of command */
	if (*command == '\0' || (yyin = popen(command, "r")) == NULL)
	{
		PopenErrorMessage(command);
		return(1);
	}

#ifdef FLEX_SCANNER
		/* reset parser if not called the first time */
	if (!firsttime)
		yyrestart(yyin);
	firsttime = False;
#endif

		/* init linenumber and filename for yyerror() */
	yylineno = 1;
	yyfilename = Filename;

		/* We need to save the data temorarily because lex-yacc are able
		 * to break the application if the input file has an illegal format.
		 * It's not necessary if the system supports the call of functions
		 * on termination.
		 */
	watchCursor();

#if !defined(HAS_ATEXIT) && !defined(HAS_ON_EXIT)
	SaveTMPData();
	returncode = yyparse();
	RemoveTMPData();
#else
	returncode = yyparse();
#endif
	/* clean up parse buffer */
	yy_delete_buffer(YY_CURRENT_BUFFER);

	restoreCursor();
	return(pclose(yyin) ? 1 : returncode);
}

/* ---------------------------------------------------------------------------
 * initializes LEX and calls parser for a single element file
 */
int ParseElementFile(DataTypePtr Ptr, char *Filename)
{
	yyPCB = NULL;
	yyData = Ptr;
	yyFont = &PCB->Font;
	yyElement = NULL;
	return(Parse(Settings.ElementCommand,Settings.ElementPath,Filename,NULL));
}

/* ---------------------------------------------------------------------------
 * initializes LEX and calls parser for a single library entry
 */
int ParseLibraryEntry(DataTypePtr Ptr, char *Template)
{
	yyPCB = NULL;
	yyData = Ptr;
	yyFont = &PCB->Font;
	yyElement = NULL;
	return(Parse(Settings.LibraryCommand, Settings.LibraryPath,
		Settings.LibraryFilename, Template));
}

/* ---------------------------------------------------------------------------
 * initializes LEX and calls parser for a complete board
 */
int ParsePCB(PCBTypePtr Ptr, char *Filename)
{
	yyPCB = Ptr;
	yyData = NULL;
	yyFont = NULL;
	yyElement = NULL;
	return(Parse(Settings.FileCommand, Settings.FilePath, Filename, NULL));
}

/* ---------------------------------------------------------------------------
 * initializes LEX and calls parser for a font
 */
int ParseFont(FontTypePtr Ptr, char *Filename)
{
	yyPCB = NULL;
	yyFont = Ptr;
	yyElement = NULL;
	return(Parse(Settings.FontCommand, Settings.FontPath, Filename, NULL));
}

