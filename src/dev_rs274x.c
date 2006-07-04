/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
 *  Copyright (C) 1998, 1999, 2000, 2001 Harry Eaton
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

/*
 *  This file, dev_rs274x.c was written by and is Copyright (C) 1998
 *  by harry eaton.  It is loosely based on the old RS-274D driver by
 *  Albert John FitzPatrick.
 *
 *  Contact address for Email:
 *  harry eaton <haceaton@aplcomm.jhuapl.edu>
 *
 */


/*
 * Gerber/RS-274X device driver
 */

/* 
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <time.h>
#include <pwd.h>
#include <sys/types.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "global.h"

#include "data.h"
#include "dev_rs274x.h"
#include "drill.h"
#include "error.h"
#include "misc.h"
#include "mymem.h"
#include "rotate.h"

#include "gui.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

/*----------------------------------------------------------------------------*/
/* Private data structures                                                    */
/*----------------------------------------------------------------------------*/

enum ApertureShape
{
  ROUND,			/* Shaped like a circle */
  OCTAGON,			/* octagonal shape */
  SQUARE,			/* Shaped like a square */
  ROUNDCLEAR,			/* clearance in negatives */
  SQUARECLEAR,
  THERMAL			/* negative thermal relief */
};
typedef enum ApertureShape ApertureShape;

typedef struct Aperture
{
  int dCode;			/* The RS-274X D code */
  int apertureSize;		/* Size in mils */
  int gap;			/* thermal clearance */
  int finger;			/* thermal fingers */
  ApertureShape apertureShape;	/* ROUND/SQUARE etc */
}
Aperture;

typedef struct Apertures
{
  int nextAvailable;		/* Number of apertures */
  Aperture aperture[GBX_MAXAPERTURECOUNT];
}
Apertures;


/*----------------------------------------------------------------------------*/
/* Utility routines                                                           */
/*----------------------------------------------------------------------------*/

#define gerberX(pcb, x) ((long) ((x)/10))
#define gerberY(pcb, y) ((long) (((pcb)->MaxHeight - (y)))/10)
#define gerberXOffset(pcb, x) ((long) ((x) /10))
#define gerberYOffset(pcb, y) ((long) (-(y) /10))


/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void GBX_Init (PrintInitTypePtr);
static void GBX_Exit (void);
static char *GBX_Preamble (PrintInitTypePtr, char *);
static void GBX_Postamble (void);
static void GBX_Invert (int);
static void GBX_PrintLine (LineTypePtr, Boolean);
static void GBX_PrintArc (ArcTypePtr, Boolean);
static void GBX_PrintPolygon (PolygonTypePtr);
static void GBX_PrintText (TextTypePtr);
static void GBX_PrintPad (PadTypePtr, int);
static void GBX_PrintPinOrVia (PinTypePtr, int);
static void GBX_PrintElementPackage (ElementTypePtr);
static void GBX_Drill (PinTypePtr, Cardinal);
static void GBX_PrintOutline (LocationType, LocationType, LocationType,
			      LocationType);
static void GBX_PrintAlignment (LocationType, LocationType, LocationType,
				LocationType);
static void GBX_GroupID (int);
static void findTextApertures (TextTypePtr);
static void GBX_PrintFilledRectangle (LocationType, LocationType,
				      LocationType, LocationType);
static void setAperture (Apertures *, int, int, int, ApertureShape);
static int findApertureCode (Apertures *, int, int, int, ApertureShape);

/* ----------------------------------------------------------------------
 * some local identifiers
 *
 */
static PrintDeviceType GBX_QueryConstants = {
  "Gerber/RS-274X",		/* name of driver */
  "gbr",			/* filename suffix */
  GBX_Init,			/* initializes driver */
  GBX_Exit,			/* exit code */
  GBX_Preamble,			/* creates file header */
  GBX_Postamble,		/* cleans up per file */
  NULL,				/* set color */
  GBX_Invert,			/* invert polarity */
  GBX_PrintLine,		/* draws a line */
  GBX_PrintArc,			/* draws an arc */
  GBX_PrintPolygon,		/* fills a polygon */
  GBX_PrintText,		/* print some (silkscreen, etc.) text */
  GBX_PrintPad,			/* print pad */
  GBX_PrintPinOrVia,		/* print pin or via */
  GBX_PrintElementPackage,	/* print element package */
  GBX_Drill,			/* drilling information */
  GBX_PrintOutline,		/* print board outline */
  GBX_PrintAlignment,		/* print alignment marks */
  NULL,				/* print drilling helper marks */
  GBX_GroupID,			/* group comments */

  False,			/* handles colored output */
  True,				/* handles drill info */
  False,			/* handles media */
  False,			/* allows mirroring */
  False,			/* no rotate */
  False,			/* no scaling */
};

static PrintInitType GBX_Flags;

static Apertures GBX_Apertures = { 0 };	/* Holds aperture definitions */

static Boolean GBX_ErrorOccurred;

static DynamicStringType appList;
static int lastX, lastY;	/* the last X and Y coordinate */
static int lastdCode = 11;	/* dCodes start at 11 */
static int lastTherm = 1;
static int lastAperture = 0;
static Cardinal lastDrill = 0;
static Boolean AmDrilling = False;
static int theGroup = 0;

/*----------------------------------------------------------------------------*/
/* Aperture Routines                                                          */
/*----------------------------------------------------------------------------*/

static void
setAperture (Apertures * apertures, int width, int gap, int finger,
	     ApertureShape shape)
{
  int apCode;

  apCode = findApertureCode (apertures, width, gap, finger, shape);
  if (lastAperture != apCode)
    {
      lastAperture = apCode;
      fprintf (GBX_Flags.FP, "G54D%d*", lastAperture);
    }
}

static int
findApertureCode (Apertures * apertures, int width, int gap, int finger,
		  ApertureShape shape)
{
  int i;
  Aperture *ap;
  char appMacro[256];

  /* we never draw zero-width lines */
  if (width == 0)
    return (0);

  /* Search for an appropriate aperture. */

  for (i = 0; i < apertures->nextAvailable; i++)
    {
      ap = &apertures->aperture[i];

      if (ap->apertureSize == width && ap->apertureShape == shape)
	if (shape == SQUARE || shape == ROUND ||
	    (ap->finger == finger && ap->gap == gap))
	  return (ap->dCode);
    }
  appMacro[0] = '\0';

  /* Not found, create a new aperture and add it to the list */
  if (apertures->nextAvailable < GBX_MAXAPERTURECOUNT)
    {
      i = apertures->nextAvailable++;
      ap = &apertures->aperture[i];
      ap->dCode = lastdCode++;
      ap->apertureSize = width;
      ap->apertureShape = shape;
      ap->finger = finger;
      ap->gap = gap;
      switch (shape)
	{
	case ROUND:
	  sprintf (appMacro, "%%ADD%dC,%.4f*%%\015\012", ap->dCode,
		   width / 100000.0);
	  break;
	case SQUARE:
	  sprintf (appMacro, "%%ADD%dR,%.4fX%.4f*%%\015\012",
		   ap->dCode, width / 100000.0, width / 100000.0);
	  break;
	case OCTAGON:
	  sprintf (appMacro, "%%AMOCT%d*5,0,8,0,0,%.4f,22.5*%%\015\012"
		   "%%ADD%dOCT%d*%%\015\012", lastTherm,
		   width / (100000.0 * COS_22_5_DEGREE), ap->dCode,
		   lastTherm);
	  lastTherm++;
	  break;
	case THERMAL:
	  sprintf (appMacro, "%%AMTHERM%d*7,0,0,%.4f,%.4f,%.4f,45*%%\015\012"
		   "%%ADD%dTHERM%d*%%\015\012", lastTherm, gap / 100000.0,
		   width / 100000.0, finger / 100000.0, ap->dCode, lastTherm);
	  lastTherm++;
	  break;
	case ROUNDCLEAR:
	  sprintf (appMacro, "%%ADD%dC,%.4fX%.4f*%%\015\012",
		   ap->dCode, gap / 100000.0, width / 100000.0);
	  break;
	case SQUARECLEAR:
	  sprintf (appMacro, "%%ADD%dR,%.4fX%.4fX%.4fX%.4f*%%\015\012",
		   ap->dCode, gap / 100000.0, gap / 100000.0,
		   width / 100000.0, width / 100000.0);
	  break;
	}

      DSAddString (&appList, appMacro);
      return (ap->dCode);
    }
  else
    {
      GBX_ErrorOccurred = True;
      Message (_("Error, too many apertures needed for Gerber file.\n"));
      return (10);
    }
}


static void
initApertures (Apertures * apertures)
{
  apertures->nextAvailable = 0;
  lastdCode = 11;
  lastTherm = 1;
  lastAperture = 0;
  appList.Data = NULL;
  appList.MaxLength = 0;
}

/* ---------------------------------------------------------------------------
 * returns information about this driver
 */
PrintDeviceTypePtr
GBX_Queryh (void)
{
  return (&GBX_QueryConstants);
}

/* ---------------------------------------------------------------------------
 * global initialization
 */
static void
GBX_Init (PrintInitTypePtr Flags)
{
  initApertures (&GBX_Apertures);
  /* gather all aperture macros */
  ALLLINE_LOOP (PCB->Data);
  {
    findApertureCode (&GBX_Apertures, line->Thickness, 0, 0, ROUND);
    if (TEST_FLAG (CLEARLINEFLAG, line))
      findApertureCode (&GBX_Apertures,
			line->Thickness + line->Clearance, 0, 0, ROUND);
  }
  ENDALL_LOOP;
  ALLARC_LOOP (PCB->Data);
  {
    findApertureCode (&GBX_Apertures, arc->Thickness, 0, 0, ROUND);
    if (TEST_FLAG (CLEARLINEFLAG, arc))
      findApertureCode (&GBX_Apertures,
			arc->Thickness + arc->Clearance, 0, 0, ROUND);
  }
  ENDALL_LOOP;
  ELEMENT_LOOP (PCB->Data);
  {
    PIN_LOOP (element);
    {
      findApertureCode (&GBX_Apertures, pin->Thickness, 0, 0,
			TEST_FLAG (SQUAREFLAG,
				   pin) ? SQUARE
			: (TEST_FLAG (OCTAGONFLAG, pin) ? OCTAGON : ROUND));
      /* mask layers need different sizes */
      findApertureCode (&GBX_Apertures, pin->Mask, 0, 0,
			TEST_FLAG (SQUAREFLAG, pin) ?
			SQUARE
			: (TEST_FLAG (OCTAGONFLAG, pin) ? OCTAGON : ROUND));
      /* check for thermal cross being used */
      if (TEST_ANY_THERMS (pin))
	{
	  int finger = (pin->Thickness - pin->DrillingHole) * PCB->ThermScale;
	  findApertureCode (&GBX_Apertures, finger, 0, 0, ROUND);
	  findApertureCode (&GBX_Apertures, pin->Thickness,
			    pin->Thickness + pin->Clearance, finger, THERMAL);
	}
      /* check for polygon clearance being used */
      if (TEST_ANY_PIPS (pin))
	{
	  /* negative planes need clears */
	  findApertureCode (&GBX_Apertures, pin->Thickness,
			    pin->Thickness + pin->Clearance, 0,
			    TEST_FLAG (SQUAREFLAG, pin) ?
			    SQUARECLEAR : ROUNDCLEAR);
	  findApertureCode (&GBX_Apertures,
			    pin->Thickness + pin->Clearance, 0, 0,
			    TEST_FLAG (SQUAREFLAG,
				       pin) ? SQUARE
			    : TEST_FLAG (OCTAGONFLAG, pin) ? OCTAGON : ROUND);
	}
    }
    END_LOOP;
    PAD_LOOP (element);
    {
      findApertureCode (&GBX_Apertures, pad->Thickness, 0, 0,
			TEST_FLAG (SQUAREFLAG, pad) ? SQUARE : ROUND);
      /* pads need clearance in polygons */
      findApertureCode (&GBX_Apertures,
			pad->Thickness + pad->Clearance, 0, 0,
			TEST_FLAG (SQUAREFLAG, pad) ? SQUARE : ROUND);
      /* pads always need masks */
      findApertureCode (&GBX_Apertures, pad->Mask, 0, 0,
			TEST_FLAG (SQUAREFLAG, pad) ? SQUARE : ROUND);
    }
    END_LOOP;
    ELEMENTLINE_LOOP (element);
    {
      findApertureCode (&GBX_Apertures, line->Thickness, 0, 0, ROUND);
    }
    END_LOOP;
    ARC_LOOP (element);
    {
      findApertureCode (&GBX_Apertures, arc->Thickness, 0, 0, ROUND);
    }
    END_LOOP;
    findTextApertures (&ELEMENT_TEXT (PCB, element));
  }
  END_LOOP;
  ALLTEXT_LOOP (PCB->Data);
  {
    findTextApertures (text);
  }
  ENDALL_LOOP;
  VIA_LOOP (PCB->Data);
  {
    findApertureCode (&GBX_Apertures, via->Thickness, 0, 0,
		      TEST_FLAG (SQUAREFLAG,
				 via) ? SQUARE
		      : (TEST_FLAG (OCTAGONFLAG, via) ? OCTAGON : ROUND));
    /* mask layers need different sizes */
    findApertureCode (&GBX_Apertures, via->Mask, 0, 0,
		      TEST_FLAG (SQUAREFLAG, via) ?
		      SQUARE : (TEST_FLAG (OCTAGONFLAG, via) ?
				OCTAGON : ROUND));
    /* check for thermal cross being used */
    if (TEST_ANY_THERMS (via))
      {
	int finger = (via->Thickness - via->DrillingHole) * PCB->ThermScale;
	findApertureCode (&GBX_Apertures, finger, 0, 0, ROUND);
	findApertureCode (&GBX_Apertures, via->Thickness,
			  via->Thickness + via->Clearance, finger, THERMAL);
      }
    /* check for polygon clearance being used */
    if (TEST_ANY_PIPS (via))
      {
	findApertureCode (&GBX_Apertures, via->Thickness + via->Clearance,
			  0, 0, TEST_FLAG (SQUAREFLAG,
					   via) ? SQUARE
			  : (TEST_FLAG (OCTAGONFLAG, via) ? OCTAGON : ROUND));
	findApertureCode (&GBX_Apertures, via->Thickness,
			  via->Thickness + via->Clearance, 0,
			  TEST_FLAG (SQUAREFLAG,
				     via) ? SQUARECLEAR : ROUNDCLEAR);
      }
  }
  END_LOOP;

  /* make sure the outline/alignment aperture exists should it be used */
  /* polygons use this aperture also  */
  findApertureCode (&GBX_Apertures, 1000, 0, 0, ROUND);
  /* fab drawing uses this one */
  findApertureCode (&GBX_Apertures, 800, 0, 0, ROUND);
}

static void
GBX_Exit ()
{
  /* Why can't I free the appList.Data????
     Sure, none of the other Dynamic strings ever do,
     even so .....
   */
  MYFREE (appList.Data);
  appList.MaxLength = 0;
}

/* ----------------------------------------------------------------------
 * prints custom Gerber/RS-274X compatible header with function definition
 * info struct is are passed in
 */
static char *
GBX_Preamble (PrintInitTypePtr Flags, char *Description)
{
  time_t currenttime;
  char utcTime[64];
  struct passwd *pwentry;

  /* save passed-in data */
  GBX_Flags = *Flags;

  if (NSTRCMP (Description, "drill information") == 0)
    {
      DrillInfoTypePtr usedDrills;
      int index = 1;

      fprintf (GBX_Flags.FP, "M48\015\012" "INCH,TZ\015\012");
      usedDrills = GetDrillInfo (PCB->Data);
      DRILL_LOOP (usedDrills);
      {
	fprintf (GBX_Flags.FP, "T%02dC%.3f\015\012",
		 index++, drill->DrillSize / 100000.0);
      }
      END_LOOP;
      FreeDrillInfo (usedDrills);
      fprintf (GBX_Flags.FP, "%%\015\012");
      lastDrill = 0;
      AmDrilling = True;
      return (NULL);
    }
  /* Create a portable timestamp. */
  currenttime = time (NULL);
  strftime (utcTime, sizeof utcTime, "%c UTC", gmtime (&currenttime));

  /* No errors have occurred so far. */
  GBX_ErrorOccurred = False;

  /* Current Aperture is undefined as this is a new file. */
  lastAperture = 0;

  /* ID the user. */
  pwentry = getpwuid (getuid ());

  /* Print a cute file header at the beginning of each file. */
  fprintf (GBX_Flags.FP, "G04 Title: %s, %s *\015\012", UNKNOWN (PCB->Name),
	   UNKNOWN (Description));
  fprintf (GBX_Flags.FP, "G04 Creator: %s " VERSION " *\015\012", Progname);
  fprintf (GBX_Flags.FP, "G04 CreationDate: %s *\015\012", utcTime);
  fprintf (GBX_Flags.FP, "G04 For: %s *\015\012", pwentry->pw_name);
  fprintf (GBX_Flags.FP, "G04 Format: Gerber/RS-274X *\015\012");
  fprintf (GBX_Flags.FP, "G04 PCB-Dimensions: %d %d *\015\012",
	   PCB->MaxWidth, PCB->MaxHeight);
  fprintf (GBX_Flags.FP, "G04 PCB-Coordinate-Origin: lower left *\015\012");

  /* Signal data in inches. */
  fprintf (GBX_Flags.FP, "%%MOIN*%%\015\012");

  /* Signal Leading zero suppression, Absolute Data, 2.3 format */
  fprintf (GBX_Flags.FP, "%%FSLAX24Y24*%%\015\012");
  return (NULL);
}

static void
GBX_Invert (int mode)
{
  /* inverting only involves a positive/negative change */
  /* you don't want to also switch dark/clear */
  /* for example %IPNEG% with all drawing as %LPC% */
  /* results in a blank film */
  switch (mode)
    {
    case 0:
      /* indicate a positive image */
      if (GBX_Flags.InvertFlag)
	fprintf (GBX_Flags.FP, "%%IPNEG*%%\015\012");
      else
	fprintf (GBX_Flags.FP, "%%IPPOS*%%\015\012");
      /* print the aperture list at the top */
      fprintf (GBX_Flags.FP, "%s", appList.Data);
      fprintf (GBX_Flags.FP, "%%LNGROUP_%d*%%\015\012%%LPD*%%\015\012",
	       theGroup);
      /* indicate straight lines and set our initial position (0,0) */
      fprintf (GBX_Flags.FP, "G01X0Y0D02*\015\012");

      /* initalize our current x,y location.  Note that in PCB,
       * the upper left corner is 0,0 and positive Y extents
       * downwards, however in photoplotterland 0,0 is lower left with 
       * positive Y extending updards.  lastX,lastY are in PCB
       * coordinates.
       */
      lastX = 0;
      lastY = PCB->MaxHeight;
      break;
    case 1:
      /* indicate negative image */
      if (GBX_Flags.InvertFlag)
	fprintf (GBX_Flags.FP, "%%IPPOS*%%\015\012");
      else
	fprintf (GBX_Flags.FP, "%%IPNEG*%%\015\012");
      /* print the aperture list at the top */
      fprintf (GBX_Flags.FP, "%s", appList.Data);
      fprintf (GBX_Flags.FP, "%%LNGROUP_%d*%%\015\012%%LPD*%%\015\012",
	       theGroup);
      /* indicate straight lines */
      fprintf (GBX_Flags.FP, "G01X0Y0D02*\015\012");

      /* initalize our current x,y location.  Note that in PCB,
       * the upper left corner is 0,0 and positive Y extents
       * downwards, however in photoplotterland 0,0 is lower left with 
       * positive Y extending updards.  lastX,lastY are in PCB
       * coordinates.
       */
      lastX = 0;
      lastY = PCB->MaxHeight;
      break;
    case 2:
      fprintf (GBX_Flags.FP, "%%LNCUTS*%%\015\012%%LPC*%%\015\012");
      break;
    case 3:
      fprintf (GBX_Flags.FP, "%%LNTRACKS*%%\015\012%%LPD*%%\015\012");
      break;
    }
}


/* ---------------------------------------------------------------------------
 * exit code for this driver is empty
 */
static void
GBX_Postamble (void)
{
  /* print trailing commands */
  if (AmDrilling)
    {
      AmDrilling = False;
      fprintf (GBX_Flags.FP, "M30\015\012");
    }
  else
    {
      /* Signal End-of-Plot. */
      fprintf (GBX_Flags.FP, "M02*\015\012");
    }
  if (GBX_ErrorOccurred != False)
    Message (_("An error occurred.\n"
	       "The Gerber output file(s) aren't correct.\n"));
}

/* ----------------------------------------------------------------------
 * handle drilling information
 */
static void
GBX_Drill (PinTypePtr PinOrVia, Cardinal index)
{
  if (lastDrill != index)
    {
      lastDrill = index;
      fprintf (GBX_Flags.FP, "T%02d\015\012", index);
    }
  fprintf (GBX_Flags.FP,
	   "X%06ldY%06ld\015\012",
	   gerberX (PCB, PinOrVia->X), gerberY (PCB, PinOrVia->Y));
}

/* ----------------------------------------------------------------------
 * prints preamble to layer's data
 */
static void
GBX_GroupID (int group)
{
  theGroup = group;
  fprintf (GBX_Flags.FP, "G04 contains layers ");
  GROUP_LOOP (group);
  {
    fprintf (GBX_Flags.FP, "%s (%i) ", UNKNOWN (layer->Name), number);
  }
  END_LOOP;
  fprintf (GBX_Flags.FP, "*\015\012");
}

/* ----------------------------------------------------------------------
 * prints a line
 */
static void
GBX_PrintLine (LineTypePtr Line, Boolean Clear)
{
  Boolean m = False;
  int size;

  size = Line->Thickness;
  if (Clear)
    size += Line->Clearance;

  if (size == 0)
    return;
  setAperture (&GBX_Apertures, size, 0, 0, ROUND);
  if (Line->Point1.X != lastX)
    {
      m = True;
      lastX = Line->Point1.X;
      fprintf (GBX_Flags.FP, "X%ld", gerberX (PCB, lastX));
    }
  if (Line->Point1.Y != lastY)
    {
      m = True;
      lastY = Line->Point1.Y;
      fprintf (GBX_Flags.FP, "Y%ld", gerberY (PCB, lastY));
    }
  if ((Line->Point1.X == Line->Point2.X)
      && (Line->Point1.Y == Line->Point2.Y))
    fprintf (GBX_Flags.FP, "D03*\015\012");
  else
    {
      if (m)
	fprintf (GBX_Flags.FP, "D02*");
      if (Line->Point2.X != lastX)
	{
	  lastX = Line->Point2.X;
	  fprintf (GBX_Flags.FP, "X%ld", gerberX (PCB, lastX));
	}
      if (Line->Point2.Y != lastY)
	{
	  lastY = Line->Point2.Y;
	  fprintf (GBX_Flags.FP, "Y%ld", gerberY (PCB, lastY));

	}
      fprintf (GBX_Flags.FP, "D01*\015\012");
    }
}

/* ----------------------------------------------------------------------
 * prints an arc
 */
static void
GBX_PrintArc (ArcTypePtr arc, Boolean Clear)
{
  Boolean m = False;
  int size;
  float arcStartX, arcStopX, arcStartY, arcStopY;

  size = arc->Thickness;
  if (Clear)
    size += arc->Clearance;
  if (size == 0)
    return;
  arcStartX = arc->X - arc->Width * cos (TO_RADIANS (arc->StartAngle));
  arcStartY = arc->Y + arc->Height * sin (TO_RADIANS (arc->StartAngle));
  arcStopX = arc->X
    - arc->Width * cos (TO_RADIANS (arc->StartAngle + arc->Delta));
  arcStopY = arc->Y
    + arc->Height * sin (TO_RADIANS (arc->StartAngle + arc->Delta));
  setAperture (&GBX_Apertures, size, 0, 0, ROUND);
  if (arcStartX != lastX)
    {
      m = True;
      lastX = arcStartX;
      fprintf (GBX_Flags.FP, "X%ld", gerberX (PCB, lastX));
    }
  if (arcStartY != lastY)
    {
      m = True;
      lastY = arcStartY;
      fprintf (GBX_Flags.FP, "Y%ld", gerberY (PCB, lastY));
    }
  if (m)
    fprintf (GBX_Flags.FP, "D02*");
  fprintf (GBX_Flags.FP,
	   "G75*G0%1dX%ldY%ldI%ldJ%ldD01*G01*\015\012",
	   (arc->Delta < 0) ? 2 : 3,
	   gerberX (PCB, arcStopX), gerberY (PCB, arcStopY),
	   gerberXOffset (PCB, arc->X - arcStartX),
	   gerberYOffset (PCB, arc->Y - arcStartY));
  lastX = arcStopX;
  lastY = arcStopY;
}

/* ---------------------------------------------------------------------------
 * prints a filled polygon
 */
static void
GBX_PrintPolygon (PolygonTypePtr Ptr)
{
  Boolean m = False;
  int firstTime = 1;
  LocationType startX = 0, startY = 0;

  /* All polygon fills need to have a defined aperture.  */
  setAperture (&GBX_Apertures, 1000, 0, 0, ROUND);
  fprintf (GBX_Flags.FP, "G36*\015\012");
  POLYGONPOINT_LOOP (Ptr);
  {
    if (point->X != lastX)
      {
	m = True;
	lastX = point->X;
	fprintf (GBX_Flags.FP, "X%ld", gerberX (PCB, lastX));
      }
    if (point->Y != lastY)
      {
	m = True;
	lastY = point->Y;
	fprintf (GBX_Flags.FP, "Y%ld", gerberY (PCB, lastY));
      }
    if (firstTime)
      {
	firstTime = 0;
	startX = point->X;
	startY = point->Y;
	if (m)
	  fprintf (GBX_Flags.FP, "D02*");
      }
    else if (m)
      fprintf (GBX_Flags.FP, "D01*\015\012");
    m = False;
  }
  END_LOOP;
  if (startX != lastX)
    {
      m = True;
      lastX = startX;
      fprintf (GBX_Flags.FP, "X%ld", gerberX (PCB, startX));
    }
  if (startY != lastY)
    {
      m = True;
      lastY = startY;
      fprintf (GBX_Flags.FP, "Y%ld", gerberY (PCB, lastY));
    }
  if (m)
    fprintf (GBX_Flags.FP, "D01*\015\012");
  fprintf (GBX_Flags.FP, "G37*\015\012");
}

/*-----------------------------------------------------------------
 * find any aperture that might be used by the text
 */
static void
findTextApertures (TextTypePtr Text)
{
  unsigned char *string = (unsigned char *) Text->TextString;
  FontTypePtr font = &PCB->Font;
  Cardinal n;

  while (string && *string)
    {
      /* draw lines if symbol is valid and data is present */
      if (*string <= MAX_FONTPOSITION && font->Symbol[*string].Valid)
	{
	  LineTypePtr line = font->Symbol[*string].Line;

	  for (n = font->Symbol[*string].LineN; n; n--, line++)
	    {
	      int tw = line->Thickness * Text->Scale / 200;
	      if (tw < 800)
		tw = 800;
	      findApertureCode (&GBX_Apertures, tw, 0, 0, ROUND);
	    }
	}
      else
	{
	  findApertureCode (&GBX_Apertures, 800, 0, 0, ROUND);
	}
      string++;
    }
}

/* ----------------------------------------------------------------------
 * prints a text
 * the routine is identical to DrawText() in module draw.c except
 * that DrawLine() and DrawRectangle() are replaced by their corresponding
 * printing routines
 */
static void
GBX_PrintText (TextTypePtr Text)
{
  LocationType x = 0, width;
  unsigned char *string = (unsigned char *) Text->TextString;
  Cardinal n;
  FontTypePtr font = &PCB->Font;

  /* Add the text to the gerber file as a comment. */
  if (string && *string)
    fprintf (GBX_Flags.FP, "G04 Text: %s *\015\012", string);

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
	      GBX_PrintLine (&newline, False);
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

	  if (TEST_FLAG (ONSOLDERFLAG, Text))
	    {
	      defaultsymbol.X1 = SWAP_SIGN_X (defaultsymbol.X1);
	      defaultsymbol.X2 = SWAP_SIGN_X (defaultsymbol.X2);
	      defaultsymbol.Y1 = SWAP_SIGN_Y (defaultsymbol.Y1);
	      defaultsymbol.Y2 = SWAP_SIGN_Y (defaultsymbol.Y2);
	    }
	  /* add offset and draw filled box */
	  defaultsymbol.X1 += Text->X;
	  defaultsymbol.Y1 += Text->Y;
	  defaultsymbol.X2 += Text->X;
	  defaultsymbol.Y2 += Text->Y;
	  GBX_PrintFilledRectangle (defaultsymbol.X1,
				    defaultsymbol.Y1,
				    defaultsymbol.X2, defaultsymbol.Y2);

	  /* move on to next cursor position */
	  x += size;
	}
      string++;
    }
}

/* ----------------------------------------------------------------------
 * prints package outline and selected text (Canonical, Instance or Value)
 */
static void
GBX_PrintElementPackage (ElementTypePtr Element)
{
  ELEMENTLINE_LOOP (Element);
  {
    GBX_PrintLine (line, False);
  }
  END_LOOP;
  ARC_LOOP (Element);
  {
    GBX_PrintArc (arc, False);
  }
  END_LOOP;
  if (!TEST_FLAG (HIDENAMEFLAG, Element))
    GBX_PrintText (&ELEMENT_TEXT (PCB, Element));
}

/* ----------------------------------------------------------------------
 * prints a pad
 */
static void
GBX_PrintPad (PadTypePtr Pad, int mode)
{
  Boolean m = False;
  int size;
  FILE *FP;

  FP = GBX_Flags.FP;
  switch (mode)
    {
    case 0:
    case 3:
      size = Pad->Thickness;
      break;
    case 1:
      size = Pad->Thickness + Pad->Clearance;
      break;
    case 2:
      size = Pad->Mask;
      break;
    default:
      size = 0;
      Message (_("Bad mode to GBX_PrintPad\n"));
    }
  if (size == 0)
    return;
  setAperture (&GBX_Apertures, size, 0, 0,
	       TEST_FLAG (SQUAREFLAG, Pad) ? SQUARE : ROUND);
  if (Pad->Point1.X != lastX)
    {
      m = True;
      lastX = Pad->Point1.X;
      fprintf (FP, "X%ld", gerberX (PCB, lastX));
    }
  if (Pad->Point1.Y != lastY)
    {
      m = True;
      lastY = Pad->Point1.Y;
      fprintf (FP, "Y%ld", gerberY (PCB, lastY));
    }
  if ((Pad->Point1.X == Pad->Point2.X) && (Pad->Point1.Y == Pad->Point2.Y))
    fprintf (FP, "D03*\015\012");
  else
    {
      if (m)
	fprintf (FP, "D02*");
      if (Pad->Point2.X != lastX)
	{
	  lastX = Pad->Point2.X;
	  fprintf (FP, "X%ld", gerberX (PCB, lastX));
	}
      if (Pad->Point2.Y != lastY)
	{
	  lastY = Pad->Point2.Y;
	  fprintf (FP, "Y%ld", gerberY (PCB, lastY));
	}
      fprintf (FP, "D01*\015\012");
    }
}

/* ----------------------------------------------------------------------
 * prints a via or pin to a specified file
 */
static void
GBX_PrintPinOrVia (PinTypePtr Ptr, int mode)
{
  int size;

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
      break;
    case 3:
      size = Ptr->Thickness + Ptr->Clearance;
      if (size == 0)
	return;
      /* negative plane mode */
      if (TEST_FLAG (USETHERMALFLAG, Ptr))
	setAperture (&GBX_Apertures, Ptr->Thickness,
		     size,
		     (Ptr->Thickness - Ptr->DrillingHole) * PCB->ThermScale,
		     THERMAL);
      else
	setAperture (&GBX_Apertures, Ptr->Thickness, size,
		     0, TEST_FLAG (SQUAREFLAG,
				   Ptr) ? SQUARECLEAR : ROUNDCLEAR);
      break;
    default:
      size = 0;
      Message (_("Bad mode to GBX_PrintPinOrVia\n"));
    }

  if (size == 0)
    return;
  if (mode != 3)
    setAperture (&GBX_Apertures, size, 0, 0,
		 TEST_FLAG (SQUAREFLAG, Ptr) ? SQUARE :
		 (TEST_FLAG (OCTAGONFLAG, Ptr) ? OCTAGON : ROUND));
  if (Ptr->X != lastX)
    {
      lastX = Ptr->X;
      fprintf (GBX_Flags.FP, "X%ld", gerberX (PCB, lastX));
    }
  if (Ptr->Y != lastY)
    {
      lastY = Ptr->Y;
      fprintf (GBX_Flags.FP, "Y%ld", gerberY (PCB, lastY));
    }
  fprintf (GBX_Flags.FP, "D03*\015\012");
  /* add thermal cross */
  if (mode == 0 && TEST_FLAG (USETHERMALFLAG, Ptr))
    {
      LineType line;
      int size2 = (size + Ptr->Clearance) / 2;
      int finger = (Ptr->Thickness - Ptr->DrillingHole) * PCB->ThermScale;

      if (!TEST_FLAG (SQUAREFLAG, Ptr))
	size2 = (7 * size2) / 10;
      line.Point1.X = Ptr->X - size2;
      line.Point1.Y = Ptr->Y - size2;
      line.Point2.X = Ptr->X + size2;
      line.Point2.Y = Ptr->Y + size2;
      line.Thickness = finger;
      GBX_PrintLine (&line, False);
      line.Point1.Y += 2 * size2;
      line.Point2.Y -= 2 * size2;
      GBX_PrintLine (&line, False);
    }
}

/* ---------------------------------------------------------------------------
 * draws a filled rectangle to the specified file
 * this is used by the text routine
 */
static void
GBX_PrintFilledRectangle (LocationType X1, LocationType Y1, LocationType X2,
			  LocationType Y2)
{
  int j;
  long int X, Y;


  setAperture (&GBX_Apertures, 1000, 0, 0, ROUND);

  for (j = Y1; j < Y2; j += 4)
    {
      X = gerberX (PCB, X1 + 1);
      Y = gerberY (PCB, j);
      fprintf (GBX_Flags.FP, "X%ldY%ldD02*\015\012", X, Y);
      X = gerberX (PCB, X2 - 1);
      fprintf (GBX_Flags.FP, "X%ldY%ldD01*\015\012", X, Y);
    }
  /* these are the last coordinates used */
  lastX = X2 - 1;
  lastY = j - 4;
}

/* ---------------------------------------------------------------------------
 * draw the outlines of a layout;
 * the upper/left and lower/right corner are passed
 *  This routine contributed by Andre M Hedrick
 */
static void
GBX_PrintOutline (LocationType X1, LocationType Y1, LocationType X2,
		  LocationType Y2)
{
  int gx1, gy1, gx2, gy2;

  fprintf (GBX_Flags.FP, "*G04 Outline ***\015\012");

  gx1 = gerberX (PCB, (int) X1);
  gy1 = gerberX (PCB, (int) Y1);
  gx2 = gerberX (PCB, (int) (int) X2);
  gy2 = gerberX (PCB, Y2);

  setAperture (&GBX_Apertures, 1000, 0, 0, ROUND),
    fprintf (GBX_Flags.FP,
	     "X%dY%dD02*X%dY%dD01*\015\012", gx1, gy1, gx2, gy1);

  fprintf (GBX_Flags.FP, "X%dY%dD02*X%dY%dD01*\015\012", gx2, gy1, gx2, gy2);

  fprintf (GBX_Flags.FP, "X%dY%dD02*X%dY%dD01*\015\012", gx2, gy2, gx1, gy2);

  fprintf (GBX_Flags.FP, "X%dY%dD02*X%dY%dD01*\015\012", gx1, gy2, gx1, gy1);
  lastX = X1;
  lastY = Y1;
}

/* ---------------------------------------------------------------------------
 * draw the alignment targets;
 * the upper/left and lower/right corner are passed
 *  This routine contributed by Andre M Hedrick
 */
static void
GBX_PrintAlignment (LocationType X1, LocationType Y1, LocationType X2,
		    LocationType Y2)
{
  int gx1, gy1, gx2, gy2;

  int XZ1 = gerberX (PCB, (int) X1 + Settings.AlignmentDistance);
  int XZ2 = gerberX (PCB, (int) X2 - Settings.AlignmentDistance);
  int XZ3 = gerberX (PCB, (int) X2 - Settings.AlignmentDistance / 2);
  int YZ1 = gerberY (PCB, (int) Y1 + Settings.AlignmentDistance);
  int YZ2 = gerberY (PCB, (int) Y2 - Settings.AlignmentDistance);

  gx1 = gerberX (PCB, X1);
  gy1 = gerberY (PCB, Y1);
  gx2 = gerberX (PCB, X2);
  gy2 = gerberY (PCB, Y2);

  fprintf (GBX_Flags.FP, "*G04 Alignment Targets ***\015\012");

  setAperture (&GBX_Apertures, 1000, 0, 0, ROUND),
    fprintf (GBX_Flags.FP,
	     "X%dY%dD02*X%dY%dD01*\015\012", gx1, gy1, XZ1, gy1);

  fprintf (GBX_Flags.FP, "X%dY%dD02*X%dY%dD01*\015\012", XZ2, gy1, gx2, gy1);

  fprintf (GBX_Flags.FP, "X%dY%dD02*X%dY%dD01*\015\012", gx2, gy1, gx2, YZ1);

  fprintf (GBX_Flags.FP, "X%dY%dD02*X%dY%dD01*\015\012", XZ3, gy1, XZ3, YZ1);

  fprintf (GBX_Flags.FP, "X%dY%dD02*X%dY%dD01*\015\012", gx2, gy1, gx2, YZ1);

  fprintf (GBX_Flags.FP, "X%dY%dD02*X%dY%dD01*\015\012", gx2, YZ2, gx2, gy2);

  fprintf (GBX_Flags.FP, "X%dY%dD02*X%dY%dD01*\015\012", gx2, gy2, XZ2, gy2);

  fprintf (GBX_Flags.FP, "X%dY%dD02*X%dY%dD01*\015\012", XZ1, gy2, gx1, gy2);

  fprintf (GBX_Flags.FP, "X%dY%dD02*X%dY%dD01*\015\012", gx1, gy2, gx1, YZ2);

  fprintf (GBX_Flags.FP, "X%dY%dD02*X%dY%dD01*\015\012", gx1, YZ1, gx1, gy1);

  lastX = X1;
  lastY = Y1;
}
