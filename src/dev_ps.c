/* $Id$ */

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

/* Change History:
 * 10/11/96 11:37 AJF Added support for a Text() driver function.
 * This was done out of a pressing need to force text to be printed on the
 * silkscreen layer. Perhaps the design is not the best.
 */

/* PostScript device driver
 * code is shared for EPS and PS output
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <time.h>
#include <pwd.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "global.h"

#include "data.h"
#include "dev_ps.h"
#include "error.h"
#include "misc.h"
#include "rotate.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID("$Id$");

/* ---------------------------------------------------------------------------
 * some defines
 */
#define	PS_UNIT		0.00072	/* 0.01 mil in PostScript units */
#define POST_SCALE(x) ((int)((x) * PS_UNIT))

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void PrintStringArray (char **, int, FILE *);
static char *PS_Preamble (PrintInitTypePtr, char *);
static char *EPS_Preamble (PrintInitTypePtr, char *);
static char *PS_EPS_Init (PrintInitTypePtr, char *, Boolean);
static void PS_Exit (void);
static void EPS_Exit (void);
static void EPS_Postamble (void);
static void PS_Postamble (void);
static void PS_EPS_Exit (Boolean);
static void PS_Init (PrintInitTypePtr);
static void EPS_Init (PrintInitTypePtr);
static void PS_SetColor (XColor);
static void PS_Invert (int);
static void PS_PrintLine (LineTypePtr, Boolean);
static void PS_PrintArc (ArcTypePtr, Boolean);
static void PS_PrintPolygon (PolygonTypePtr);
static void PS_PrintText (TextTypePtr);
static void PS_PrintTextLowLevel (TextTypePtr);
static void PS_PrintElementPackage (ElementTypePtr);
static void PS_PrintPad (PadTypePtr, int);
static void PS_PrintPinOrVia (PinTypePtr, int);
static void PS_Outline (Location, Location, Location, Location);
static void PS_Alignment (Location, Location, Location, Location);
static void PS_DrillHelper (PinTypePtr, int);

/* ----------------------------------------------------------------------
 * some local identifiers
 *
 * PSFunctions is initializes by the standard PostScript code used by PCB.
 * All PS functions are declared in this section:
 *   A:         draws an element arc
 *   Alignment: draws the boards outline
 *   B:         draws a filled box
 *   CA:	clears an arc
 *   CL:	clears a line
 *   CLRB       clears a box
 *   CLRPA:     clears a pad
 *   CLRPV:     clears an octagon 
 *   CLRPVSQ:   clears a square
 *   CLRPVR:    clears a circle
 *   DH:        drill helper, a small circle
 *   FILL:      draws a filled rectangle for the ground plane
 *   L:         draws a line
 *   Outline:   draws the boards outline
 *   P:         draws a filled pin-polygon 
 *   PA:        draws a pad
 *   PO:        draws a filled polygon 
 *   PV:        draws an octagonal pin or via
 *   PVR:       draws a round pin or via
 *   PVSQ:      draws a square pin or via
 * additional information is available from the PS comments.
 *
 * some PS code for arcs comes from the output of 'Fig2Dev'. Thanks to
 * the author
 */
static PrintDeviceType PS_QueryConstants = {
  "PostScript",
  "ps",
  PS_Init,
  PS_Exit,
  PS_Preamble,
  PS_Postamble,
  PS_SetColor,
  PS_Invert,
  PS_PrintLine,
  PS_PrintArc,
  PS_PrintPolygon,
  PS_PrintText,
  PS_PrintPad,
  PS_PrintPinOrVia,
  PS_PrintElementPackage,
  NULL,				/* no drill information */
  PS_Outline,
  PS_Alignment,
  PS_DrillHelper,
  NULL,				/* no group IDs */
  True,				/* driver can handle color */
  False,			/* handles no drill information */
  True,				/* handles different media */
  True,				/* allows mirroring */
  True,				/* allows rotate */
  True
},				/* allows scaling */

  EPS_QueryConstants =
{
  "encapsulated PostScript", "eps", EPS_Init, EPS_Exit, EPS_Preamble, EPS_Postamble, PS_SetColor, PS_Invert, PS_PrintLine, PS_PrintArc, PS_PrintPolygon, PS_PrintText, PS_PrintPad, PS_PrintPinOrVia, PS_PrintElementPackage, NULL,	/* no drill information */
    PS_Outline, PS_Alignment, PS_DrillHelper, NULL,	/* no group ID */
    True, False, False,		/* encapsulated doesn't allow media changes */
    True, True,			/* allows rotate */
True};				/* allows scaling */

static PrintInitType PS_Flags;
static Boolean preamble = False;

static char *PS_Functions[] = {
  "",
  "/PcbDict 200 dict def",
  "PcbDict begin",
  "PcbDict /DictMatrix matrix put",
  "",
  "% some constants",
  "/Black {0.0 mysetgray} def",
  "/White {1.0 mysetgray} def",
  "/TAN {0.207106781} def",
  "/MTAN {-0.207106781} def",
  "",
  "% draw a filled polygon",
  "% get (x,y)... and number of points from stack",
  "/PO {",
  "	/number exch def",
  "	newpath",
  "	moveto",
  "	number 1 sub { lineto } repeat",
  "	closepath fill stroke",
  "} def",
  "",
  "/P {",
  "% draw a pin-polygon,",
  "% get x, y and thickness from stack",
  "	/thickness exch def /y exch def /x exch def",
  "	gsave x y translate thickness thickness scale",
  "	0.5  MTAN",
  "	TAN  -0.5",
  "	MTAN -0.5",
  "	-0.5 MTAN",
  "	-0.5 TAN",
  "	MTAN 0.5",
  "	TAN  0.5",
  "	0.5  TAN",
  "	8 PO grestore",
  "} def",
  "",
  "/PV {",
  "% pin or via, x, y and thickness are on the stack",
  "	/drillinghole exch def /thickness exch def /y exch def /x exch def",
  "	x y thickness P",
  "% draw drilling hole",
  "	gsave White 0 setlinewidth",
  "   newpath x y drillinghole 2 div 0 360 arc closepath fill stroke",
  "   grestore",
  "} def",
  "",
  "/PVR {",
  "% pin or via, x, y and thickness are on the stack",
  "	/drillinghole exch def /thickness exch def /y exch def /x exch def",
  "	gsave 0 setlinewidth",
  "	newpath x y thickness 2 div 0 360 arc closepath fill stroke",
  "% draw drilling whole",
  "	White",
  "	newpath x y drillinghole 2 div 0 360 arc closepath fill stroke",
  "	grestore",
  "} def",
  "",
  "/PVSQ {",
  "% square pin or via, x, y and thickness are on the stack",
  "	/drillinghole exch def /thickness exch def /y exch def /x exch def",
  "	newpath x thickness 2 div sub y thickness 2 div sub moveto",
  "	thickness 0 rlineto 0 thickness rlineto",
  "	thickness neg 0 rlineto closepath fill stroke",
  "% draw drilling hole",
  "	gsave White 0 setlinewidth",
  "   newpath x y drillinghole 2 div 0 360 arc closepath fill stroke",
  "   grestore",
  "} def",
  "",
  "/DH {",
  "% drill helpher; x, y, hole, copper-thickness are on stack",
  "	/copper exch def /hole exch def /y exch def /x exch def",
  "   gsave copper setlinewidth",
  "   newpath x y hole copper add 2 div 0 360 arc closepath stroke",
  "   grestore",
  "} def",
  "",
  "/L {",
  "% line, get x1, y1, x2, y2 and thickness from stack",
  "	/thick exch def /y2 exch def /x2 exch def /y1 exch def /x1 exch def",
  "	gsave thick setlinewidth",
  "	x1 y1 moveto x2 y2 lineto stroke",
  "	grestore",
  "} def",
  "",
  "/CL {",
  "% line, get x1, y1, x2, y2 and thickness from stack",
  "	/thick exch def /y2 exch def /x2 exch def /y1 exch def /x1 exch def",
  "	gsave White thick setlinewidth",
  "	x1 y1 moveto x2 y2 lineto stroke",
  "	grestore",
  "} def",
  "",
  "/B {",
  "% filled box, get x1, y1, x2 and y2 from stack",
  "	/y2 exch def /x2 exch def /y1 exch def /x1 exch def",
  "	newpath",
  "	x1 y1 moveto x2 y1 lineto x2 y2 lineto x1 y2 lineto",
  "	closepath fill stroke",
  "} def",
  "",
  "/PA {",
  "% pad, same as line",
  "	L",
  "} def",
  "",
  "/A {",
  "% arc for elements, get x, y, width, height, thickness",
  "% startangle and delta-angle from stack",
  "  /delta exch def /start exch def /thickness exch def",
  "  /height exch def /width exch def /y exch def /x exch def",
  "% draw it",
  "	gsave thickness setlinewidth /save DictMatrix currentmatrix def",
  "% scaling is less then zero because the coord system has to be swapped",
  "	x y translate width neg height scale",
  "	0 0 1 start start delta add arc save setmatrix stroke",
  "	grestore",
  "} def",
  "",
  "/CA {",
  "% arc for elements, get x, y, width, height, thickness",
  "% startangle and delta-angle from stack",
  "  /delta exch def /start exch def /thickness exch def",
  "  /height exch def /width exch def /y exch def /x exch def",
  "% draw it",
  "	gsave White thickness setlinewidth /save DictMatrix currentmatrix def",
  "% scaling is less then zero because the coord system has to be swapped",
  "	x y translate width neg height scale",
  "	0 0 1 start start delta add arc save setmatrix stroke",
  "	grestore",
  "} def",
  "",
  "/CLRPV {",
  "% clears a pin/via for groundplane; x,y and thickness are on stack",
  "   /thickness exch def /y exch def /x exch def",
  "	gsave White x y thickness P grestore",
  "} def",
  "",
  "/CLRPVSQ {",
  "% clears a square pin, x,y and thickness are on stack",
  "	/thickness exch def /y exch def /x exch def",
  "   gsave White",
  "	newpath x thickness 2 div sub y thickness 2 div sub moveto",
  "	thickness 0 rlineto 0 thickness rlineto",
  "	thickness neg 0 rlineto closepath fill stroke",
  "	grestore",
  "} def",
  "",
  "/CLRPVR {",
  "% clears a round pin/via for groundplane; x,y and thickness are on the stack",
  "	/thickness exch def /y exch def /x exch def",
  "	gsave White 0 setlinewidth",
  "	newpath x y thickness 2 div 0 360 arc closepath fill stroke",
  "	grestore",
  "} def",
  "",
  "/CLRPA {",
  "% clear line, get x1, y1, x2, y2 and thickness from stack",
  "	/thick exch def /y2 exch def /x2 exch def /y1 exch def /x1 exch def",
  "	gsave White thick setlinewidth",
  "	x1 y1 moveto x2 y2 lineto stroke",
  "	grestore",
  "} def",
  "",
  "/CLRB {",
  "% cleared box, get x1, y1, x2 and y2 from stack",
  "	/y2 exch def /x2 exch def /y1 exch def /x1 exch def",
  "	gsave White newpath",
  "	x1 y1 moveto x2 y1 lineto x2 y2 lineto x1 y2 lineto",
  "	closepath fill stroke",
  "	grestore",
  "} def",
  "",
  "/FILL {",
  "% draw a filled rectangle for the ground plane",
  "% get x1, y1, x2 and y2 from stack",
  "	/y2 exch def /x2 exch def /y1 exch def /x1 exch def",
  "   gsave 0 setlinewidth",
  "	newpath",
  "	x1 y1 moveto x2 y1 lineto x2 y2 lineto x1 y2 lineto",
  "	closepath fill stroke",
  "   grestore",
  "} def",
  "",
  "/Outline {",
  "% outline, get x1, y1, x2 and y2 from stack",
  "	/y2 exch def /x2 exch def /y1 exch def /x1 exch def",
  "   gsave 0.175 setlinewidth",
  "	newpath",
  "	x1 y1 moveto x2 y1 lineto x2 y2 lineto x1 y2 lineto",
  "	closepath stroke",
  "   grestore",
  "} def",
  "",
  "/Alignment {",
  "% alignment targets, get x1, y1, x2, y2 and distance from stack",
  "	/dis exch def /y2 exch def /x2 exch def /y1 exch def /x1 exch def",
  "   gsave 0.175 setlinewidth",
  "   newpath x1 y1 dis add moveto",
  "      0 dis 2 mul neg rlineto",
  "      dis neg dis rmoveto",
  "      dis 2 mul 0 rlineto",
  "   stroke",
  "   newpath x1 y1 dis 0 90 arcn stroke",
  "   newpath x1 y2 dis sub moveto 0 dis rlineto dis 0 rlineto stroke",
  "   newpath x2 y2 dis sub moveto",
  "      0 2 dis mul rlineto",
  "      dis dup neg rmoveto",
  "      2 dis mul neg 0 rlineto" "   stroke",
  "   newpath x2 y1 dis add moveto 0 dis neg rlineto dis neg 0 rlineto stroke",
  "   grestore",
  "} def",
  ""
};

static char *PS_ColorFunctions[] = {
  "/mysetgray { setgray } def",
  "/mysetrgbcolor { setrgbcolor } def",
  ""
};

static char *PS_InvertColorFunctions[] = {
  "/mysetgray {neg 1.0 add setgray} def",
  "/mysetrgbcolor {",
  "  /blue exch def /green exch def /red exch def",
  "  1.0 red sub 1.0 green sub 1.0 blue sub setrgbcolor",
  "} def" ""
};

static char *PS_Setup[] = {
  "0.0 setlinewidth",
  "1 setlinecap",
  "Black"
};

/* ---------------------------------------------------------------------------
 * prints a string array
 */
static void
PrintStringArray (char **Array, int Number, FILE * FP)
{
  for (; Number; Number--, Array++)
    fprintf (FP, "%s\n", *Array);
}

/* ---------------------------------------------------------------------------
 * returns information about the PostScript driver
 */
PrintDeviceTypePtr
PS_Query (void)
{
  return (&PS_QueryConstants);
}

/* ---------------------------------------------------------------------------
 * returns information about the encapsulated PostScript driver
 */
PrintDeviceTypePtr
EPS_Query (void)
{
  return (&EPS_QueryConstants);
}

/* ----------------------------------------------------------------------
 * call init routine for PostScript output
 */
static char *
PS_Preamble (PrintInitTypePtr Flags, char *Description)
{
  return (PS_EPS_Init (Flags, Description, False));
}
static void
PS_Init (PrintInitTypePtr wasted)
{
}

/* ----------------------------------------------------------------------
 * call init routine for encapsulated PostScript output
 */
static char *
EPS_Preamble (PrintInitTypePtr Flags, char *Description)
{
  return (PS_EPS_Init (Flags, Description, True));
}
static void
EPS_Init (PrintInitTypePtr wasted)
{
}
static void
PS_Invert (int mode)
{
  if (mode == 1)
    {
      if (PS_Flags.InvertFlag)
	PrintStringArray (PS_ColorFunctions,
			  ENTRIES (PS_ColorFunctions), PS_Flags.FP);
      else
	PrintStringArray (PS_InvertColorFunctions,
			  ENTRIES (PS_InvertColorFunctions), PS_Flags.FP);
    }
}

/* ----------------------------------------------------------------------
 * prints PostScript or enc. PostScript header with function definition
 * info struct is are passed in
 */
static char *
PS_EPS_Init (PrintInitTypePtr Flags, char *Description, Boolean CreateEPS)
{
  BoxType box;
  time_t currenttime;
  float dx, dy;
  struct passwd *pwentry;

  /* save passed-in data */
  PS_Flags = *Flags;
  /* adjust the 'mirror' flag because (0,0) is in the lower/left
   * corner which is different from X coordinates
   */
  if (!Settings.ShowSolderSide)
    PS_Flags.MirrorFlag = !PS_Flags.MirrorFlag;

  /* adjusting the offsets is also necessary because the
   * passed coordinates are X (upper/left corner is 0,0)
   */
  dx = dy = PS_Flags.Scale;
  dx *= (float) (PS_Flags.BoundingBox.X2 - PS_Flags.BoundingBox.X1);
  dy *= (float) (PS_Flags.BoundingBox.Y2 - PS_Flags.BoundingBox.Y1);

  /* create standard PS header */
  if (CreateEPS)
    {
      PS_Flags.OffsetX = 0;
      PS_Flags.OffsetY = 0;
    }
  else
    {
      PS_Flags.OffsetY = PS_Flags.SelectedMedia->Height - PS_Flags.OffsetY;
      PS_Flags.OffsetY -= (BDimension) (PS_Flags.RotateFlag ? dx : dy);
    }
  if (!preamble)
    {
      preamble = True;
      if (CreateEPS)
	fputs ("%!PS-Adobe-3.0 EPSF-3.0\n", PS_Flags.FP);
      else
	fputs ("%!PS-Adobe-3.0\n", PS_Flags.FP);
      currenttime = time (NULL);

      fprintf (PS_Flags.FP, "%%%%Title: %s, %s\n", UNKNOWN (PCB->Name),
	       UNKNOWN (Description));
      fprintf (PS_Flags.FP, "%%%%Creator: %s " VERSION "\n", Progname);
      fprintf (PS_Flags.FP, "%%%%CreationDate: %s",
	       asctime (localtime (&currenttime)));
      pwentry = getpwuid (getuid ());
      fprintf (PS_Flags.FP, "%%%%For: %s (%s)\n", pwentry->pw_name,
	       pwentry->pw_gecos);
      fputs ("%%LanguageLevel: 1\n", PS_Flags.FP);
      fputs ("%%Orientation: Portrait\n", PS_Flags.FP);

      /* - calculate the width and height of the bounding box;
       * - write bounding box data to file
       * - rotate it
       * - transform to PS coordinates#
       */
      box.X1 = (Location) ((float) PS_Flags.OffsetX * PS_UNIT) - 1;
      box.Y1 = (Location) ((float) PS_Flags.OffsetY * PS_UNIT) - 1;
      if (!PS_Flags.RotateFlag)
	{
	  box.X2 = (Location) ((dx + PS_Flags.OffsetX) * PS_UNIT) + 1;
	  box.Y2 = (Location) ((dy + PS_Flags.OffsetY) * PS_UNIT) + 1;
	}
      else
	{
	  box.X2 = (Location) ((dy + PS_Flags.OffsetX) * PS_UNIT) + 1;
	  box.Y2 = (Location) ((dx + PS_Flags.OffsetY) * PS_UNIT) + 1;
	}

      /* print it if encapsulated PostScript has been requested
       * and add the appropriate structured comments
       */
      if (CreateEPS)
	{
	  fprintf (PS_Flags.FP, "%%%%BoundingBox: %i %i %i %i\n",
		   (int) box.X1, (int) box.Y1, (int) box.X2, (int) box.Y2);
	  fputs ("%%Pages: 0\n", PS_Flags.FP);
	}
      else
	{
	  fputs ("%%Pages: 1\n", PS_Flags.FP);
	  fputs ("%%PageOrder: Ascend\n", PS_Flags.FP);
	  fprintf (PS_Flags.FP, "%%%%DocumentMedia: %s %d %d\n",
		   PS_Flags.SelectedMedia->Name,
		   POST_SCALE(PS_Flags.SelectedMedia->Width),
		   POST_SCALE(PS_Flags.SelectedMedia->Height));
	}

      /* OK, continue with structured comments */
      fputs ("%%EndComments\n", PS_Flags.FP);
      fputs ("%%BeginProlog\n", PS_Flags.FP);
      PrintStringArray (PS_Functions, ENTRIES (PS_Functions), PS_Flags.FP);
      if (PS_Flags.InvertFlag)
	PrintStringArray (PS_InvertColorFunctions,
			  ENTRIES (PS_InvertColorFunctions), PS_Flags.FP);
      else
	PrintStringArray (PS_ColorFunctions,
			  ENTRIES (PS_ColorFunctions), PS_Flags.FP);
      fputs ("%%EndProlog\n", PS_Flags.FP);
      fputs ("%%BeginDefaults\n", PS_Flags.FP);
      fputs ("%%EndDefaults\n", PS_Flags.FP);
      fputs ("%%BeginSetup\n", PS_Flags.FP);
      PrintStringArray (PS_Setup, ENTRIES (PS_Setup), PS_Flags.FP);
      fputs ("%%EndSetup\n", PS_Flags.FP);

      if (!CreateEPS)
	{
	  fputs ("%%Page: 1 1\n", PS_Flags.FP);
	  fputs ("%%BeginPageSetup\n", PS_Flags.FP);
	  fputs ("%%EndPageSetup\n", PS_Flags.FP);
	}

      /* clear the area */
      fputs ("gsave White newpath\n", PS_Flags.FP);
      fprintf (PS_Flags.FP,
	       "%i %i moveto %i %i lineto %i %i lineto %i %i lineto\n",
	       (int) box.X1, (int) box.Y1,
	       (int) box.X2, (int) box.Y1,
	       (int) box.X2, (int) box.Y2, (int) box.X1, (int) box.Y2);
      fputs ("closepath fill stroke grestore\n", PS_Flags.FP);

      /* add information about layout size, offset ... */
      fprintf (PS_Flags.FP, "%% PCBMIN(%d,%d), PCBMAX(%d,%d)\n",
	       (int) PS_Flags.BoundingBox.X1,
	       (int) PS_Flags.BoundingBox.Y1,
	       (int) PS_Flags.BoundingBox.X2, (int) PS_Flags.BoundingBox.Y2);
      fprintf (PS_Flags.FP, "%% PCBOFFSET(%d,%d), PCBSCALE(%.5f)\n",
	       PS_Flags.OffsetX, PS_Flags.OffsetY, PS_Flags.Scale);
      fputs ("% PCBSTARTDATA --- do not remove ---\n", PS_Flags.FP);
      fputs ("gsave\n", PS_Flags.FP);
    }
  else
    fputs ("grestore\ngsave\n", PS_Flags.FP);


  /* now insert transformation commands (reverse order):
   * - move upper/left edge of layout to (0,0)
   * - mirror it to transform X to PostScript coordinates
   * - move to (0,0) again
   * - if rotation is required, rotate and move to (0,0)
   * - apply user scaling
   * - move to new offset
   * - scale to PostScript (72 dots per inch)
   */
  fprintf (PS_Flags.FP, "%.5f %.5f scale\n", PS_UNIT, PS_UNIT);
  fprintf (PS_Flags.FP, "%i %i translate\n",
	   PS_Flags.OffsetX, PS_Flags.OffsetY);
  fprintf (PS_Flags.FP, "%.3f %.3f scale\n", PS_Flags.Scale, PS_Flags.Scale);
  if (PS_Flags.RotateFlag)
    {
      fprintf (PS_Flags.FP, "%i 0 translate\n",
	       (int) PS_Flags.BoundingBox.Y2 - PS_Flags.BoundingBox.Y1);
      fputs ("90 rotate\n", PS_Flags.FP);
    }
  if (PS_Flags.MirrorFlag)
    {
      fprintf (PS_Flags.FP, "0 %i translate\n",
	       (int) (PS_Flags.BoundingBox.Y2 - PS_Flags.BoundingBox.Y1));
      fputs ("1 -1 scale\n", PS_Flags.FP);
    }
  fprintf (PS_Flags.FP, "%i %i translate\n",
	   -PS_Flags.BoundingBox.X1, -PS_Flags.BoundingBox.Y1);
  return (NULL);
}

/* ----------------------------------------------------------------------
 * call exit routine for PostScript output
 */
static void
PS_Postamble (void)
{
  preamble = False;
  PS_EPS_Exit (False);
}
static void
PS_Exit (void)
{
}

/* ----------------------------------------------------------------------
 * call exit routine for encapsulated PostScript output
 */
static void
EPS_Postamble (void)
{
  preamble = False;
  PS_EPS_Exit (True);
}
static void
EPS_Exit (void)
{
}

/* ---------------------------------------------------------------------------
 * exit code for this driver is empty
 */
static void
PS_EPS_Exit (Boolean CreateEPS)
{
  /* print trailing commands */
  fputs ("grestore\n", PS_Flags.FP);
  fputs ("% PCBENDDATA --- do not remove ---\n", PS_Flags.FP);
  if (!CreateEPS)
    fputs ("showpage\n", PS_Flags.FP);
  fputs ("%%EOF\n", PS_Flags.FP);
}

/* ----------------------------------------------------------------------
 * prints a line
 */
static void
PS_PrintLine (LineTypePtr Line, Boolean Clear)
{
  if (Clear)
    fprintf (PS_Flags.FP, "%d %d %d %d %d CL\n",
	     (int) Line->Point1.X,
	     (int) Line->Point1.Y,
	     (int) Line->Point2.X,
	     (int) Line->Point2.Y,
	     (int) Line->Thickness + (int) Line->Clearance);
  else
    fprintf (PS_Flags.FP, "%d %d %d %d %d L\n",
	     (int) Line->Point1.X,
	     (int) Line->Point1.Y,
	     (int) Line->Point2.X,
	     (int) Line->Point2.Y, (int) Line->Thickness);
}

/* ---------------------------------------------------------------------------
 * prints an arc 
 */
static void
PS_PrintArc (ArcTypePtr arc, Boolean Clear)
{
  if (Clear)
    fprintf (PS_Flags.FP, "%d %d %d %d %d %ld %ld CA\n",
	     (int) arc->X,
	     (int) arc->Y,
	     (int) arc->Width,
	     (int) arc->Height,
	     (int) arc->Thickness + (int) arc->Clearance,
	     (arc->Delta <
	      0) ? arc->StartAngle + arc->Delta : arc->StartAngle,
	     (arc->Delta < 0) ? -arc->Delta : arc->Delta);
  else
    fprintf (PS_Flags.FP, "%d %d %d %d %d %ld %ld A\n", (int) arc->X,
	     (int) arc->Y, (int) arc->Width, (int) arc->Height,
	     (int) arc->Thickness,
	     (arc->Delta <
	      0) ? arc->StartAngle + arc->Delta : arc->StartAngle,
	     (arc->Delta < 0) ? -arc->Delta : arc->Delta);
}

/* ---------------------------------------------------------------------------
 * prints a filled polygon
 */
static void
PS_PrintPolygon (PolygonTypePtr Ptr)
{
  int i = 0;

  POLYGONPOINT_LOOP (Ptr);
  {
    if (i++ % 9 == 8)
      fputc ('\n', PS_Flags.FP);
    fprintf (PS_Flags.FP, "%i %i ", (int) point->X, (int) point->Y);
  }
  END_LOOP;
  fprintf (PS_Flags.FP, "%d PO\n", Ptr->PointN);
}

/* ----------------------------------------------------------------------
 * lowlevel routine to print text
 * the routine is identical to DrawText() in module draw.c except
 * that DrawLine() and DrawRectangle() are replaced by their corresponding
 * printing routines
 */
static void
PS_PrintTextLowLevel (TextTypePtr Text)
{
  Location x = 0, width;
  unsigned char *string = (unsigned char *) Text->TextString;
  Cardinal n;
  FontTypePtr font = &PCB->Font;

  /* get the center of the text for mirroring */
  width = Text->Direction & 0x01 ?
    Text->BoundingBox.Y2 - Text->BoundingBox.Y1 :
    Text->BoundingBox.X2 - Text->BoundingBox.X1;
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
	      if (newline.Thickness < 8)
		newline.Thickness = 8;

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
	      PS_PrintLine (&newline, False);
	    }

	  /* move on to next cursor position */
	  x += (font->Symbol[*string].Width + font->Symbol[*string].Delta);
	}
      else
	{
	  /* the default symbol is a filled box */
	  BoxType defaultsymbol = PCB->Font.DefaultSymbol;
	  Location size = (defaultsymbol.X2 - defaultsymbol.X1) * 6 / 5;

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
	  fprintf (PS_Flags.FP, "%d %d %d %d B\n",
		   (int) defaultsymbol.X1,
		   (int) defaultsymbol.Y1,
		   (int) defaultsymbol.X2, (int) defaultsymbol.Y2);

	  /* move on to next cursor position */
	  x += size;
	}
      string++;
    }
}

/* ----------------------------------------------------------------------
 * print text; the code has been added by
 * Albert John FitzPatrick III <ajf_nylorac@acm.org>
 * see ../CHANGES for details
 */
static void
PS_PrintText (TextTypePtr Text)
{
  PS_PrintTextLowLevel (Text);
}

/* ----------------------------------------------------------------------
 * prints package outline
 */
static void
PS_PrintElementPackage (ElementTypePtr Element)
{
  ELEMENTLINE_LOOP (Element);
  {
    PS_PrintLine (line, False);
  }
  END_LOOP;
  ARC_LOOP (Element);
  {
    fprintf (PS_Flags.FP, "%d %d %d %d %d %ld %ld A\n",
	     (int) arc->X,
	     (int) arc->Y,
	     (int) arc->Width,
	     (int) arc->Height,
	     (int) arc->Thickness, arc->StartAngle, arc->Delta);
  }
  END_LOOP;
  if (!TEST_FLAG (HIDENAMEFLAG, Element))
    PS_PrintTextLowLevel (&ELEMENT_TEXT (PCB, Element));
}

/* ----------------------------------------------------------------------
 * prints a pad
 */
static void
PS_PrintPad (PadTypePtr Pad, int mode)
{
  Dimension scrunch = 0;

  switch (mode)
    {
    case 0:
    case 3:
      scrunch = Pad->Thickness;
      break;
    case 1:
      scrunch = (Pad->Thickness + Pad->Clearance);
      break;
    case 2:
      scrunch = Pad->Mask;
      break;
    }

  if (TEST_FLAG (SQUAREFLAG, Pad))
    {
      scrunch /= 2;
      fprintf (PS_Flags.FP, "%d %d %d %d %s\n",
	       ((Pad->Point1.X > Pad->Point2.X) ? Pad->Point2.X : Pad->
		Point1.X) - scrunch,
	       ((Pad->Point1.Y > Pad->Point2.Y) ? Pad->Point2.Y : Pad->
		Point1.Y) - scrunch,
	       ((Pad->Point1.X > Pad->Point2.X) ? Pad->Point1.X : Pad->
		Point2.X) + scrunch,
	       ((Pad->Point1.Y > Pad->Point2.Y) ? Pad->Point1.Y : Pad->
		Point2.Y) + scrunch, (mode == 1 || mode == 2) ? "CLRB" : "B");
    }
  else
    fprintf (PS_Flags.FP, "%d %d %d %d %d %s\n",
	     (int) Pad->Point1.X,
	     (int) Pad->Point1.Y,
	     (int) Pad->Point2.X,
	     (int) Pad->Point2.Y,
	     (int) scrunch, (mode == 1 || mode == 2) ? "CLRPA" : "PA");
}

/* ----------------------------------------------------------------------
 * prints a via or pin
 */
static void
PS_PrintPinOrVia (PinTypePtr Ptr, int mode)
{
  int size = 0;

  switch (mode)
    {
    case 0:
      size = Ptr->Thickness;
      break;
    case 1:
      size = Ptr->Thickness + Ptr->Clearance;
      break;
    case 2:
      size = Ptr->Mask;
      if (size == 0)
	return;
      break;
    }

  if (mode)
    fprintf (PS_Flags.FP, "%d %d %d %s\n",
	     (int) Ptr->X,
	     (int) Ptr->Y,
	     size,
	     TEST_FLAG (SQUAREFLAG, Ptr) ? "CLRPVSQ"
	     : TEST_FLAG (OCTAGONFLAG, Ptr) ? "CLRPV" : "CLRPVR");
  else
    {
      if (TEST_FLAG (USETHERMALFLAG, Ptr))
	{
	  int size2 = (size + Ptr->Clearance) / 2;
	  int finger = (Ptr->Thickness - Ptr->DrillingHole) * PCB->ThermScale;

	  if (!TEST_FLAG (SQUAREFLAG, Ptr))
	    size2 = (7 * size2) / 10;
	  fprintf (PS_Flags.FP, "%d %d %d %d %d L\n",
		   Ptr->X - size2, Ptr->Y - size2,
		   Ptr->X + size2, Ptr->Y + size2, finger);
	  fprintf (PS_Flags.FP, "%d %d %d %d %d L\n",
		   Ptr->X - size2, Ptr->Y + size2,
		   Ptr->X + size2, Ptr->Y - size2, finger);
	  CLEAR_FLAG (USETHERMALFLAG, Ptr);
	}
      fprintf (PS_Flags.FP, "%d %d %d %d %s\n",
	       (int) Ptr->X,
	       (int) Ptr->Y,
	       size,
	       (int) Ptr->DrillingHole,
	       TEST_FLAG (SQUAREFLAG, Ptr) ? "PVSQ"
	       : TEST_FLAG (OCTAGONFLAG, Ptr) ? "PV" : "PVR");
    }
}

/* ---------------------------------------------------------------------------
 * draw the outlines of a layout;
 * the upper/left and lower/right corner are passed
 */
static void
PS_Outline (Location X1, Location Y1, Location X2, Location Y2)
{
  fprintf (PS_Flags.FP, "%d %d %d %d Outline\n",
	   (int) X1, (int) Y1, (int) X2, (int) Y2);
}

/* ---------------------------------------------------------------------------
 * draw the alignment targets;
 * the upper/left and lower/right corner are passed
 */
static void
PS_Alignment (Location X1, Location Y1, Location X2, Location Y2)
{
  fprintf (PS_Flags.FP, "%d %d %d %d %d Alignment\n",
	   (int) X1,
	   (int) Y1, (int) X2, (int) Y2, (int) Settings.AlignmentDistance);
}

/* ----------------------------------------------------------------------
 * prints a via or pin
 */
static void
PS_DrillHelper (PinTypePtr Ptr, int unused)
{
  if (Ptr->DrillingHole >= 4 * MIN_PINORVIAHOLE)
    fprintf (PS_Flags.FP, "%d %d %d %d DH\n",
	     (int) Ptr->X,
	     (int) Ptr->Y,
	     (int) 2 * MIN_PINORVIAHOLE, (int) MIN_PINORVIAHOLE);
}

/* ----------------------------------------------------------------------
 * Convert X color to postscript
 */
static void
PS_SetColor (XColor RGB)
{
  fprintf (PS_Flags.FP,
	   "/Color {%.3f %.3f %.3f mysetrgbcolor} def Color\n",
	   (float) RGB.red / 65535.0,
	   (float) RGB.green / 65535.0, (float) RGB.blue / 65535.0);
}
