%{
/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,2006 Thomas Nau
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

#include "global.h"
#include "crosshair.h"
#include "data.h"
#include "error.h"
#include "file.h"
#include "mymem.h"
#include "misc.h"
#include "strflags.h"
#include "parse_l.h"
#include "parse_y.h"
#include "create.h"

#define YY_NO_INPUT

/* ---------------------------------------------------------------------------
 * some shared parser identifiers
 */
#ifdef FLEX_SCANNER

#define yyunput ATTRIBUTE_UNUSED yyunput
#endif

char			*yyfilename;	/* in this file */
PCBType			*yyPCB;		/* used by parser */
DataType		*yyData;
ElementType		*yyElement;
FontType		*yyFont;

static int parse_number (void);

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
INTEGER                 [+-]?([1-9][0-9]*|0)
FLOATING                {INTEGER}?"."[0-9]*
STRINGCHAR		([^"\n\r\\]|\\.)

%option yylineno

%%

FileVersion	{ return(T_FILEVERSION); }
PCB			{ return(T_PCB); }
Grid		{ return(T_GRID); }
Cursor		{ return(T_CURSOR); }
Thermal		{ return(T_THERMAL); }
PolyArea	{ return(T_AREA); }
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
Font        { return(T_FONT); }
Mark		{ return(T_MARK); }
Groups		{ return(T_GROUPS); }
Styles		{ return(T_STYLES); }
Polygon		{ return(T_POLYGON); }
Hole		{ return(T_POLYGON_HOLE); }
Arc		{ return(T_ARC); }
NetList		{ return(T_NETLIST); }
Net		{ return(T_NET); }
Connect		{ return(T_CONN); }
Attribute	{ return(T_ATTRIBUTE); }

nm	{ return T_NM; }
um	{ return T_UM; }
mm	{ return T_MM; }
m	{ return T_M; }
km	{ return T_KM; }
umil	{ return T_UMIL; }
cmil	{ return T_CMIL; }
mil	{ return T_MIL; }
in	{ return T_IN; }
px  { return T_PX; }

\'.\'				{
						yylval.integer = (unsigned) *(yytext+1);
						return(CHAR_CONST);
					}
{FLOATING}		{	return parse_number(); }
{INTEGER}		{	yylval.integer = round (g_ascii_strtod (yytext, NULL)); return INTEGER; }

{HEX}			{	unsigned n;
				sscanf((char *) yytext, "%x", &n);
				yylval.integer = n;
				return INTEGER;
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
						yylval.string = (char *)calloc (yyleng+1, sizeof (char));
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
	int		used_popen = 0;
	char *tmps;
	size_t l;
#ifdef FLEX_SCANNER
	static	bool	firsttime = true;
#endif

	if (EMPTY_STRING_P (Executable))
	  {
	    l = 2;
	    if ( Path != NULL )
              l += strlen (Path);

            l += strlen (Filename);

	    if ( (tmps = (char *) malloc ( l * sizeof (char))) == NULL)
              {
                fprintf (stderr, "Parse():  malloc failed\n");
                exit (1);
              }

	    if ( Path != NULL && *Path != '\0')
              sprintf (tmps, "%s%s%s", Path, PCB_DIR_SEPARATOR_S, Filename);
            else
              sprintf (tmps, "%s", Filename);

	    yyin = fopen (tmps, "r");
	    if (!yyin)
	      {
	        /* Special case this one, we get it all the time... */
	        if (strcmp (tmps, "./default_font"))
		  Message("Can't open %s for reading\n", tmps);
		return(1);
	      }
            free (tmps);
	  }
	else
	  {
	    used_popen = 1;
	    /* release old command and create new from template */
	    free (command);
	    command = EvaluateFilename(Executable, Path, Filename, Parameter);

	    /* open pipe to stdout of command */
	    if (*command == '\0' || (yyin = popen(command, "r")) == NULL)
	      {
		PopenErrorMessage(command);
		return(1);
	      }
	  }

#ifdef FLEX_SCANNER
		/* reset parser if not called the first time */
	if (!firsttime)
		yyrestart(yyin);
	firsttime = false;
#endif

		/* init linenumber and filename for yyerror() */
	yylineno = 1;
	yyfilename = Filename;

		/* We need to save the data temporarily because lex-yacc are able
		 * to break the application if the input file has an illegal format.
		 * It's not necessary if the system supports the call of functions
		 * on termination.
		 */

	CreateBeLenient (true);

#if !defined(HAS_ATEXIT) && !defined(HAS_ON_EXIT)
	if (PCB && PCB->Data)
	  SaveTMPData();
	returncode = yyparse();
	RemoveTMPData();
#else
	returncode = yyparse();
#endif
	/* clean up parse buffer */
	yy_delete_buffer(YY_CURRENT_BUFFER);

	CreateBeLenient (false);

	if (used_popen)
	  return(pclose(yyin) ? 1 : returncode);
	return(fclose(yyin) ? 1 : returncode);
}

/* ---------------------------------------------------------------------------
 * initializes LEX and calls parser for a single element file
 */
int
ParseElementFile (DataType *Ptr, char *Filename)
{
	yyPCB = NULL;
	yyData = Ptr;
	yyFont = Settings.Font;
	yyElement = NULL;
	return(Parse(NULL,NULL,Filename,NULL));
}

/* ---------------------------------------------------------------------------
 * initializes LEX and calls parser for a single library entry
 */
int
ParseLibraryEntry (DataType *Ptr, char *Template)
{
	yyPCB = NULL;
	yyData = Ptr;
	yyFont = Settings.Font;
	yyElement = NULL;
	return(Parse(Settings.LibraryCommand, Settings.LibraryPath,
		Settings.LibraryFilename, Template));
}

/* ---------------------------------------------------------------------------
 * initializes LEX and calls parser for a complete board
 */
int
ParsePCB (PCBType *Ptr, char *Filename)
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
int
ParseFont (FontType *Ptr, char *Filename)
{
	int r = 0;
	char *path, *p;
	yyPCB = NULL;
	yyFont = Ptr;
	yyElement = NULL;

	path = strdup (Settings.FontPath);

        /* search through the font path for a font file */
	for (p = strtok (path, PCB_PATH_DELIMETER); p && *p;
		p = strtok (NULL, PCB_PATH_DELIMETER))
	  {
#ifdef DEBUG
            Message ("Looking for %s in %s\n", Filename, p);
#endif
	    r = Parse(Settings.FontCommand, p, Filename, NULL);
            if (r == 0)
              {
#ifdef DEBUG
                Message ("Found %s in %s\n", Filename, p);
#endif
                break;
              }
          }
        free (path);

	return r;
}

static int
parse_number ()
{
  yylval.number = g_ascii_strtod ((gchar *) yytext, NULL);
  return FLOATING;
}

