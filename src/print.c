/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2003 Thomas Nau
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

static char *rcsid = "$Id$";

/* printing routines
 */
#include <stdarg.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <pwd.h>

#include "global.h"

#include "data.h"
#include "dev_ps.h"
#include "dev_rs274x.h"
#include "drill.h"
#include "file.h"
#include "find.h"
#include "gui.h"
#include "error.h"
#include "misc.h"
#include "print.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static PrintDeviceTypePtr Device;
static PrintInitType DeviceFlags;
static Boolean GlobalOutlineFlag,	/* copy of local ident. */
  GlobalAlignmentFlag, GlobalDrillHelperFlag, GlobalColorFlag, GlobalDOSFlag, ReplaceOK;	/* ok two overriode all */
/* print output files */
static char *GlobalCommand;

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void SetPrintColor (Pixel);
static FILE *OpenPrintFile (char *);
static int SetupPrintFile (char *, char *);
static int ClosePrintFile (void);
static int FPrintOutline (void);
static int FPrintAlignment (void);
static int PrintLayergroups (void);
static int PrintSilkscreen (void);
static int PrintDrill (void);
static int PrintMask (void);
static int PrintPaste (void);

/* ----------------------------------------------------------------------
 * queries color from X11 database and calls device  command
 * black is default on errors
 */
static void
SetPrintColor (Pixel X11Color)
{
  XColor rgb;
  int result;

  /* do nothing if no colors are requested */
  if (!GlobalColorFlag)
    return;

  /* query best matching color from X server */
  rgb.pixel = X11Color;
  result = XQueryColor (Dpy,
			DefaultColormapOfScreen (XtScreen (Output.Toplevel)),
			&rgb);

  /* default color is black */
  if (result == BadValue || result == BadColor)
    rgb.red = rgb.green = rgb.blue = 0;
  Device->SetColor (rgb);
}

/* ---------------------------------------------------------------------------
 * opens a file for printing
 */
static FILE *
OpenPrintFile (char *FileExtention)
{
  char *filename, *completeFilename;
  size_t length;
  FILE *fp;

  /* evaluate add extention and suffix to filename */
  if ((filename = ExpandFilename (NULL, GlobalCommand)) == NULL)
    filename = GlobalCommand;
  length = strlen (EMPTY (GlobalCommand)) + 1 +
    strlen (FileExtention) + 1 + strlen (Device->Suffix) + 1;
  completeFilename = MyCalloc (length, sizeof (char), "OpenPrintFile()");
  sprintf (completeFilename, "%s%s%s.%s",
	   GlobalDOSFlag ? "" : EMPTY (GlobalCommand),
	   GlobalDOSFlag ? "" : "_", FileExtention, Device->Suffix);

  /* try to open all the file; if the value of
   * 'ReplaceOK' is set to 'True' no more confirmation is
   * requested for the sequence
   */
  fp = CheckAndOpenFile (completeFilename, !ReplaceOK, True, &ReplaceOK);
  if (fp == NULL)
    {
      OpenErrorMessage (completeFilename);
      SaveFree (completeFilename);
      return (NULL);
    }
  SaveFree (completeFilename);
  return (fp);
}

/* ---------------------------------------------------------------------------
 * setup new print file
 */
static int
SetupPrintFile (char *FileExtention, char *Description)
{
  char *message;

  if ((DeviceFlags.FP = OpenPrintFile (FileExtention)) == NULL)
    return (1);
  if ((message = Device->Preamble (&DeviceFlags, Description)) != NULL)
    {
      Message (message);
      fclose (DeviceFlags.FP);
      return (1);
    }
  return (0);
}

/* ---------------------------------------------------------------------------
 * closes the printfile and calls the exit routine of the driver
 */
static int
ClosePrintFile (void)
{
  Device->Postamble ();
  return (fclose (DeviceFlags.FP));
}

/* ---------------------------------------------------------------------------
 * prints or draws outline to a given file
 */
static int
FPrintOutline (void)
{
  Device->Outline (0, 0, PCB->MaxWidth, PCB->MaxHeight);
  return (0);
}

/* ---------------------------------------------------------------------------
 * prints or draws alignment to a given file
 */
static int
FPrintAlignment (void)
{
  Device->Alignment (DeviceFlags.BoundingBox.X1,
		     DeviceFlags.BoundingBox.Y1,
		     DeviceFlags.BoundingBox.X2, DeviceFlags.BoundingBox.Y2);
  return (0);
}

/* ---------------------------------------------------------------------------
 * prints all layergroups
 * returns != zero on error
 */
static int
PrintLayergroups (void)
{
  char extention[12], description[18];
  Cardinal group, entry, component, solder;
  Boolean noData, negative_plane;
  int Somepolys, use_mode;
  int PIPflag, Tflag;

  /* check for a special case where we can make a negative gnd/power plane */
  /* this can be helpful for lame-brained manufacturers that can't handle */
  /* composite layers. If there is no complex stuff on a layer, a simple */
  /* negative is possible. We require:
   * (1) No Lines, Text or Arcs
   * (2) Exactly one polygon
   * (3) No Pads
   * (4) All Pins and Vias pierce the polygon
   */
  if (PCB->Data->RatN)
    {
      Message ("WARNING!!  Rat lines in layout during printing!\n"
	       "You're not DONE with the layout!!!!\n");
    };

  for (group = 0; group < MAX_LAYER; group++)
    if (PCB->LayerGroups.Number[group])
      {
	Somepolys = 0;
	/* see if we can make the special negative plane */
	if (strcmp (Device->Name, "Gerber/RS-274X") == 0)
	  negative_plane = True;
	else
	  negative_plane = False;

	/* always include solder/component layers */
	component = GetLayerGroupNumberByNumber (MAX_LAYER + COMPONENT_LAYER);
	solder = GetLayerGroupNumberByNumber (MAX_LAYER + SOLDER_LAYER);
	if (component == group || solder == group)
	  noData = False;
	else
	  noData = True;

	if (group == component)
	  {
	    noData = False;
	    ALLPAD_LOOP (PCB->Data, 
	      {
		if (!TEST_FLAG (ONSOLDERFLAG, pad))
		  negative_plane = False;
	      }
	    );
	  }
	else if (group == solder)
	  {
	    noData = False;
	    ALLPAD_LOOP (PCB->Data, 
	      {
		if (TEST_FLAG (ONSOLDERFLAG, pad))
		  negative_plane = False;
	      }
	    );
	  }
	for (entry = 0; entry < PCB->LayerGroups.Number[group]; entry++)
	  {
	    LayerTypePtr layer;
	    Cardinal number;

	    if ((number = PCB->LayerGroups.Entries[group][entry]) >=
		MAX_LAYER)
	      continue;
	    layer = LAYER_PTR (number);
	    if (layer->LineN || layer->TextN || layer->ArcN)
	      {
		noData = False;
		negative_plane = False;
	      }
	    if (layer->PolygonN)
	      {
		noData = False;
		Somepolys += layer->PolygonN;
		PIPflag = L0PIPFLAG << number;
		Tflag = L0THERMFLAG << number;
	      }
	  }
	/* skip empty layers */
	if (noData)
	  continue;
	/* complete the special plane check: All pins/vias must pierce one polygon */
	if (Somepolys != 1)
	  negative_plane = False;
	if (negative_plane)
	  {
	    ALLPIN_LOOP (PCB->Data, 
	      {
		if (!TEST_FLAG (PIPflag, pin))
		  negative_plane = False;
	      }
	    );
	    VIA_LOOP (PCB->Data, 
	      {
		if (!TEST_FLAG (PIPflag, via))
		  negative_plane = False;
	      }
	    );
	  }

	/* setup extention and open new file */
	sprintf (extention, "%s%i", GlobalDOSFlag ? "" : "group", group + 1);
	sprintf (description, "layergroup #%i", group + 1);

	if (SetupPrintFile (extention, description))
	  return (1);
	/* comment on what layers in group */
	if (Device->GroupID)
	  Device->GroupID (group);

	/* now if negative plane is possible, do it here */
	if (!negative_plane)
	  {
	    /* indicate positive polarity */
	    Device->Polarity (0);
	    /* loop and check each layer
	     * for being a member of the group
	     */
	    if (GlobalAlignmentFlag)
	      FPrintAlignment ();

	    /* print all polygons */
	    for (entry = 0; entry < PCB->LayerGroups.Number[group]; entry++)
	      {
		LayerTypePtr layer;
		Cardinal number;

		number = PCB->LayerGroups.Entries[group][entry];
		if (number >= MAX_LAYER)
		  continue;

		layer = LAYER_PTR (number);
		if (layer->PolygonN)
		  {
		    SetPrintColor (layer->Color);
		    POLYGON_LOOP (layer, 
		      {
			Device->Poly (polygon);
		      }
		    );
		  }
	      }
	    /* clear all lines, arcs, pins */
	    PIPflag = Tflag = 0;
/* TO DO: It would be good to make sure something is actually clearing
 * drawn regions before calling Device->Polarity
 */
	    if (Somepolys)
	      {
		Device->Polarity (2);
		for (entry = 0; entry < PCB->LayerGroups.Number[group];
		     entry++)
		  {
		    LayerTypePtr layer;
		    Cardinal number;

		    InitConnectionLookup ();
		    SaveFindFlag (DRCFLAG);
		    number = PCB->LayerGroups.Entries[group][entry];
		    if (number >= MAX_LAYER)
		      continue;

		    layer = LAYER_PTR (number);
		    PIPflag |= L0PIPFLAG << number;
		    Tflag |= L0THERMFLAG << number;
		    LINE_LOOP (layer, 
		      {
			if (TEST_FLAG (CLEARLINEFLAG, line))
			  {
			    CLEAR_FLAG (CLEARLINEFLAG, line);
			    line->Thickness += Settings.Bloat;
			    ResetFoundLinesAndPolygons (False);
			    ResetFoundPinsViasAndPads (False);
			    RatFindHook (LINE_TYPE, layer, line, line,
					 False, False);
			    line->Thickness -= Settings.Bloat;
			    SET_FLAG (CLEARLINEFLAG, line);
			    /* now if any polygons are found, do the clear */
			    COPPERPOLYGON_LOOP (PCB->Data, 
			      {
				if (TEST_FLAG (DRCFLAG, polygon))
				  {
				    Device->Line (line, True);
				    break;
				  }
			      }
			    );
			  }
		      }
		    );
		    ARC_LOOP (layer, 
		      {
			if (TEST_FLAG (CLEARLINEFLAG, arc))
			  CLEAR_FLAG (CLEARLINEFLAG, arc);
			arc->Thickness += Settings.Bloat;
			ResetFoundLinesAndPolygons (False);
			ResetFoundPinsViasAndPads (False);
			RatFindHook (ARC_TYPE, layer, arc, arc, False, False);
			arc->Thickness -= Settings.Bloat;
			SET_FLAG (CLEARLINEFLAG, arc);
			/* now if any polygons are found, do the clear */
			COPPERPOLYGON_LOOP (PCB->Data, 
			  {
			    if (TEST_FLAG (DRCFLAG, polygon))
			      {
				Device->Arc (arc, True);
				break;
			      }
			  }
			);
		      }
		    );
		  }
		ALLPIN_LOOP (PCB->Data, 
		  {
		    if (TEST_FLAG (PIPflag, pin))
		      Device->PinOrVia (pin, 1);
		  }
		);
		VIA_LOOP (PCB->Data, 
		  {
		    if (TEST_FLAG (PIPflag, via))
		      Device->PinOrVia (via, 1);
		  }
		);
		/* fixme: would be nice to only clear pads inside polys
		 * probably the thing to do would be create PIPflags
		 * for the pads too.
		 */
		if (group == component)
		  {
		    ALLPAD_LOOP (PCB->Data, 
		      {
			if (!TEST_FLAG (ONSOLDERFLAG, pad))
			  Device->Pad (pad, 1);
		      }
		    );
		  }
		else if (group == solder)
		  ALLPAD_LOOP (PCB->Data, 
		  {
		    if (TEST_FLAG (ONSOLDERFLAG, pad))
		      Device->Pad (pad, 1);
		  }
		);
		FreeConnectionLookupMemory ();
		RestoreFindFlag ();
		Device->Polarity (3);
	      }
	    /* print the lines/arcs/pins/pads/vias */
	    for (entry = 0; entry < PCB->LayerGroups.Number[group]; entry++)
	      {
		LayerTypePtr layer;
		Cardinal number;

		number = PCB->LayerGroups.Entries[group][entry];
		if (number >= MAX_LAYER)
		  continue;

		layer = LAYER_PTR (number);
		SetPrintColor (layer->Color);
		LINE_LOOP (layer, 
		  {
		    Device->Line (line, False);
		  }
		);
		ARC_LOOP (layer, 
		  {
		    Device->Arc (arc, False);
		  }
		);
		TEXT_LOOP (layer, 
		  {
		    Device->Text (text);
		  }
		);
	      }
	    use_mode = 0;
	  }
	else
	  {
	    /* special negative plane */
	    Device->Polarity (1);
	    use_mode = 3;
	  }
	SetPrintColor (PCB->PinColor);
	ALLPIN_LOOP (PCB->Data, 
	  {
	    if (!TEST_FLAG (HOLEFLAG, pin))
	      {
		if (TEST_FLAG (Tflag, pin))
		  SET_FLAG (USETHERMALFLAG, pin);
		else
		  CLEAR_FLAG (USETHERMALFLAG, pin);
		Device->PinOrVia (pin, use_mode);
	      }
	  }
	);
	SetPrintColor (PCB->ViaColor);
	VIA_LOOP (PCB->Data, 
	  {
	    if (!TEST_FLAG (HOLEFLAG, via))
	      {
		if (TEST_FLAG (Tflag, via))
		  SET_FLAG (USETHERMALFLAG, via);
		else
		  CLEAR_FLAG (USETHERMALFLAG, via);
		Device->PinOrVia (via, use_mode);
	      }
	  }
	);
	if (group == component)
	  {
	    if (GlobalOutlineFlag)
	      FPrintOutline ();
	    ALLPAD_LOOP (PCB->Data, 
	      {
		if (!TEST_FLAG (ONSOLDERFLAG, pad))
		  Device->Pad (pad, 0);
	      }
	    );
	  }
	else if (group == solder)
	  {
	    if (GlobalOutlineFlag)
	      FPrintOutline ();
	    ALLPAD_LOOP (PCB->Data, 
	      {
		if (TEST_FLAG (ONSOLDERFLAG, pad))
		  Device->Pad (pad, 0);
	      }
	    );
	  }

	/* print drill-helper if requested */
	if (GlobalDrillHelperFlag && Device->DrillHelper)
	  {
	    SetPrintColor (PCB->PinColor);
	    ALLPIN_LOOP (PCB->Data, 
	      {
		Device->DrillHelper (pin, 1);
	      }
	    );
	    SetPrintColor (PCB->ViaColor);
	    VIA_LOOP (PCB->Data, 
	      {
		Device->DrillHelper (via, 1);
	      }
	    );
	  }
	/* close the device */
	ClosePrintFile ();
      }
  return (0);
}

/* ---------------------------------------------------------------------------
 * prints solder and component side silk screens
 * first print element outlines and names, then silk layer
 * returns != zero on error
 */
static int
PrintSilkscreen (void)
{
  static char *extention[2] = { "componentsilk", "soldersilk" },
    *DOSextention[2] =
  {
  "csilk", "ssilk"}
  , *description[2] =
  {
  "silkscreen, component side", "silkscreen, solder side"};
  LayerTypePtr layer;
  int i;
  Boolean noData;

  /* loop over both sides, start with component */
  for (i = 0; i < 2; i++)
    {
      layer =
	LAYER_PTR (MAX_LAYER + (i == 0 ? COMPONENT_LAYER : SOLDER_LAYER));
      noData = True;
      ELEMENT_LOOP (PCB->Data, 
	{
	  if ((TEST_FLAG (ONSOLDERFLAG, element) == 0) == (i == 0))
	    {
	      noData = False;
	      break;
	    }
	}
      );
      if (layer->PolygonN || layer->LineN || layer->ArcN || layer->TextN)
	noData = False;
      if (noData)
	continue;
      /* start with the component side */
      if (SetupPrintFile (GlobalDOSFlag ? DOSextention[i] : extention[i],
			  description[i]))
	return (1);
      /* positive polarity */
      Device->Polarity (0);
      /* print element outlines and canonical, instance or
       * value descriptions
       */
      if (GlobalAlignmentFlag)
	FPrintAlignment ();

      SetPrintColor (PCB->ElementColor);
      ELEMENT_LOOP (PCB->Data, 
	{
	  if ((TEST_FLAG (ONSOLDERFLAG, element) == 0) == (i == 0))
	    Device->ElementPackage (element);
	}
      );

      POLYGON_LOOP (layer, 
	{
	  Device->Poly (polygon);
	}
      );
      LINE_LOOP (layer, 
	{
	  Device->Line (line, False);
	}
      );
      ARC_LOOP (layer, 
	{
	  Device->Arc (arc, False);
	}
      );
      TEXT_LOOP (layer, 
	{
	  Device->Text (text);
	}
      );
      ClosePrintFile ();
    }
  return (0);
}

/* ---------------------------------------------------------------------------
 * prints solder and component side solder paste
 * returns != zero on error
 */
static int
PrintPaste (void)
{
  static char *extention[2] = { "componentpaste", "solderpaste" },
    *DOSextention[2] =
  {
  "cpaste", "spaste"}
  , *description[2] =
  {
  "solder paste, component side", "solder paste, solder side"};
  int i;
  Boolean NoData;

  /* loop over both sides, start with component */
  for (i = 0; i < 2; i++)
    {
      NoData = True;
      ALLPAD_LOOP (PCB->Data, 
	{
	  if ((TEST_FLAG (ONSOLDERFLAG, pad) == 0) == (i == 0))
	    {
	      NoData = False;
	      break;
	    }
	}
      );
      /* skip empty files */
      if (NoData)
	continue;

      /* start with the component side */
      if (SetupPrintFile (GlobalDOSFlag ? DOSextention[i] : extention[i],
			  description[i]))
	return (1);

      Device->Polarity (0);
      if (GlobalAlignmentFlag)
	FPrintAlignment ();

      SetPrintColor (PCB->ElementColor);
      ALLPAD_LOOP (PCB->Data, 
	{
	  if ((TEST_FLAG (ONSOLDERFLAG, pad) == 0) == (i == 0))
	    Device->Pad (pad, 3);
	}
      );
      ClosePrintFile ();
    }
  return (0);
}

/* ---------------------------------------------------------------------------
 * creates drill-information file(s) if the device is able to
 * returns != zero on error
 *
 * if the drill helper is selected, merges plated and unplated in a single file
 */
static int
PrintDrill (void)
{
  DrillInfoTypePtr AllDrills;
  Cardinal index;
  int i;

  /* pass drilling information */
  if (Device->HandlesDrill)
    {
      if (SetupPrintFile (GlobalDOSFlag ? "pdrill" : "plated-drill",
			  "drill information"))
	return (1);

      SetPrintColor (PCB->PinColor);
      AllDrills = GetDrillInfo (PCB->Data);
      index = 0;
      DRILL_LOOP (AllDrills, 
	{
	  index++;
	  for (i = 0; i < drill->PinN; i++)
	    if (!TEST_FLAG (HOLEFLAG, drill->Pin[i]))
	      Device->Drill (drill->Pin[i], index);
	    else if (GlobalDrillHelperFlag)
	      Device->Drill (drill->Pin[i], index);
	}
      );
      /* close the file */
      ClosePrintFile ();
      if (GlobalDrillHelperFlag)
	{
	  FreeDrillInfo (AllDrills);
	  return (0);
	}
      if (SetupPrintFile (GlobalDOSFlag ? "udrill" : "unplated-drill",
			  "drill information"))
	{
	  FreeDrillInfo (AllDrills);
	  return (1);
	}
      SetPrintColor (PCB->PinColor);
      index = 0;
      DRILL_LOOP (AllDrills, 
	{
	  index++;
	  for (i = 0; i < drill->PinN; i++)
	    if (TEST_FLAG (HOLEFLAG, drill->Pin[i]))
	      Device->Drill (drill->Pin[i], index);
	}
      );
      /* close the file */
      ClosePrintFile ();
      FreeDrillInfo (AllDrills);
    }
  return (0);
}

/* ---------------------------------------------------------------------------
 * prints solder and component side mask
 * returns != zero on error
 */
static int
PrintMask (void)
{
  static char *extention[2] = { "componentmask", "soldermask" },
    *DOSextention[2] =
  {
  "cmask", "smask"}
  , *description[2] =
  {
  "solder mask component side", "solder mask solder side"};
  int i;

  /* loop over both sides, start with component */
  for (i = 0; i < 2; i++)
    {
      /* solder reliefs are positive, use invert if you need to */
      DeviceFlags.InvertFlag = !DeviceFlags.InvertFlag;

      /* start with the component side */
      if (SetupPrintFile (GlobalDOSFlag ? DOSextention[i] : extention[i],
			  description[i]))
	return (1);

      Device->Polarity (1);
      SetPrintColor (PCB->PinColor);
      if (GlobalAlignmentFlag)
	FPrintAlignment ();
      ALLPAD_LOOP (PCB->Data, 
	{
	  if ((TEST_FLAG (ONSOLDERFLAG, pad) == 0) == (i == 0))
	    Device->Pad (pad, 2);
	}
      );
      ALLPIN_LOOP (PCB->Data, 
	{
	  Device->PinOrVia (pin, 2);
	}
      );
      VIA_LOOP (PCB->Data, 
	{
	  Device->PinOrVia (via, 2);
	}
      );

      ClosePrintFile ();
      /* Restore the invert flag */
      DeviceFlags.InvertFlag = !DeviceFlags.InvertFlag;
    }
  return (0);
}

/* ---------------------------------------------------------------------------
 * prints a FAB drawing.
 */

#define TEXT_SIZE	150
#define TEXT_LINE	15000
#define FAB_LINE_W      800

static void
fab_line (int x1, int y1, int x2, int y2, int t)
{
  LineType l;
  l.Flags = 0;
  l.Point1.X = x1;
  l.Point1.Y = y1;
  l.Point2.X = x2;
  l.Point2.Y = y2;
  l.Thickness = t;
  Device->Line (&l, 0);
}

static void
fab_circle (int x, int y, int r, int t)
{
  ArcType a;
  a.Flags = 0;
  a.X = x;
  a.Y = y;
  a.Width = r;
  a.Height = r;
  a.StartAngle = 0;
  a.Delta = 180;
  a.Thickness = t;
  Device->Arc (&a, 0);
  a.StartAngle = 180;
  Device->Arc (&a, 0);
}

/* align is 0=left, 1=center, 2=right, add 8 for underline */
static void
text_at (int x, int y, int align, char *fmt, ...)
{
  char tmp[512];
  int w = 0, i;
  TextType t;
  va_list a;
  FontTypePtr font = &PCB->Font;

  va_start (a, fmt);
  vsprintf (tmp, fmt, a);
  va_end (a);

  t.Direction = 0;
  t.TextString = tmp;
  t.Scale = TEXT_SIZE;
  t.Flags = 0;
  t.X = x;
  t.Y = y;
  for (i = 0; tmp[i]; i++)
    w += (font->Symbol[tmp[i]].Width + font->Symbol[tmp[i]].Delta);
  w = w * TEXT_SIZE / 100;
  t.X -= w * (align & 3) / 2;
  if (t.X < 0)
    t.X = 0;

  Device->Text (&t);
  if (align & 8)
    fab_line (t.X, t.Y + font->MaxHeight * TEXT_SIZE / 100 + 1000,
	      t.X + w, t.Y + font->MaxHeight * TEXT_SIZE / 100 + 1000, FAB_LINE_W);
}

/* Y, X, circle, square */
static void
drill_sym (int idx, int x, int y)
{
  int type = idx % 5;
  int size = idx / 5;
  int s2 = (size + 1) * 1600;
  int i;

  switch (type)
    {
    case 0:			/* Y */ ;
      fab_line (x, y, x, y + s2, FAB_LINE_W);
      fab_line (x, y, x + s2 * 13 / 15, y - s2 / 2, FAB_LINE_W);
      fab_line (x, y, x - s2 * 13 / 15, y - s2 / 2, FAB_LINE_W);
      for (i = 1; i <= size; i++)
	fab_circle (x, y, i * 1600, FAB_LINE_W);
      break;
    case 1:			/* + */ ;
      fab_line (x, y - s2, x, y + s2, FAB_LINE_W);
      fab_line (x - s2, y, x + s2, y, FAB_LINE_W);
      for (i = 1; i <= size; i++)
	{
	  fab_line (x - i * 1600, y - i * 1600, x + i * 1600, y - i * 1600, FAB_LINE_W);
	  fab_line (x - i * 1600, y - i * 1600, x - i * 1600, y + i * 1600, FAB_LINE_W);
	  fab_line (x - i * 1600, y + i * 1600, x + i * 1600, y + i * 1600, FAB_LINE_W);
	  fab_line (x + i * 1600, y - i * 1600, x + i * 1600, y + i * 1600, FAB_LINE_W);
	}
      break;
    case 2:			/* X */ ;
      fab_line (x - s2 * 3 / 4, y - s2 * 3 / 4, x + s2 * 3 / 4,
		y + s2 * 3 / 4, FAB_LINE_W);
      fab_line (x - s2 * 3 / 4, y + s2 * 3 / 4, x + s2 * 3 / 4,
		y - s2 * 3 / 4, FAB_LINE_W);
      for (i = 1; i <= size; i++)
	{
	  fab_line (x - i * 1600, y - i * 1600, x + i * 1600, y - i * 1600, FAB_LINE_W);
	  fab_line (x - i * 1600, y - i * 1600, x - i * 1600, y + i * 1600, FAB_LINE_W);
	  fab_line (x - i * 1600, y + i * 1600, x + i * 1600, y + i * 1600, FAB_LINE_W);
	  fab_line (x + i * 1600, y - i * 1600, x + i * 1600, y + i * 1600, FAB_LINE_W);
	}
      break;
    case 3:			/* circle */ ;
      for (i = 0; i <= size; i++)
	fab_circle (x, y, (i + 1) * 1600 - 800, FAB_LINE_W);
      break;
    case 4:			/* square */
      for (i = 1; i <= size + 1; i++)
	{
	  fab_line (x - i * 1600, y - i * 1600, x + i * 1600, y - i * 1600, FAB_LINE_W);
	  fab_line (x - i * 1600, y - i * 1600, x - i * 1600, y + i * 1600, FAB_LINE_W);
	  fab_line (x - i * 1600, y + i * 1600, x + i * 1600, y + i * 1600, FAB_LINE_W);
	  fab_line (x + i * 1600, y - i * 1600, x + i * 1600, y + i * 1600, FAB_LINE_W);
	}
      break;
    }
}

static int
PrintFab (void)
{
  PinType tmp_pin;
  DrillInfoTypePtr AllDrills;
  DrillTypePtr dp;
  Cardinal index;
  int i, n, yoff, total_drills = 0, ds = 0;
  char buf[100];
  time_t currenttime;
  char utcTime[64];
  struct passwd *pwentry;

  if (SetupPrintFile ("fab", "Fabrication Drawing"))
    return (1);

  Device->Polarity (0);
  FPrintOutline ();

  tmp_pin.Flags = 0;
  AllDrills = GetDrillInfo (PCB->Data);
  yoff = -TEXT_LINE;

  for (n = AllDrills->DrillN - 1; n >= 0; n--)
    {
      DrillTypePtr drill = &(AllDrills->Drill[n]);

      if (drill->PinCount + drill->ViaCount > drill->UnplatedCount)
	ds++;
      if (drill->UnplatedCount)
	ds++;
    }

  for (n = AllDrills->DrillN - 1; n >= 0; n--)
    {
      int plated_sym = -1, unplated_sym = -1;
      DrillTypePtr drill = &(AllDrills->Drill[n]);

      if (drill->PinCount + drill->ViaCount > drill->UnplatedCount)
	plated_sym = --ds;
      if (drill->UnplatedCount)
	unplated_sym = --ds;

      SetPrintColor (PCB->PinColor);
      for (i = 0; i < drill->PinN; i++)
	drill_sym (TEST_FLAG (HOLEFLAG, drill->Pin[i]) ? unplated_sym :
		   plated_sym, drill->Pin[i]->X, drill->Pin[i]->Y);

      if (plated_sym != -1)
	drill_sym (plated_sym, TEXT_SIZE / 4, yoff + TEXT_SIZE / 4);
      if (unplated_sym != -1)
	drill_sym (unplated_sym, 170000 + TEXT_SIZE / 4, yoff + TEXT_SIZE / 4);
      SetPrintColor (PCB->ElementColor);
      text_at (30000, yoff, 200, "%d", drill->DrillSize/100);
      text_at (60000, yoff, 200, "%d", drill->PinCount);
      text_at (90000, yoff, 200, "%d", drill->ViaCount);
      text_at (125000, yoff, 200, "%d", drill->ElementN);
      text_at (170000, yoff, 200, "%d", drill->UnplatedCount);
      yoff -= TEXT_LINE;

      total_drills += drill->PinCount;
      total_drills += drill->ViaCount;
    }

  SetPrintColor (PCB->ElementColor);
  text_at (25000, yoff, 900, "Diam.");
  text_at (55000, yoff, 900, "Pins");
  text_at (85000, yoff, 900, "Vias");
  text_at (120000, yoff, 900, "Elements");
  text_at (165000, yoff, 900, "Unplated");
  yoff -= TEXT_LINE;
  text_at (0, yoff, 0,
	   "There are %d different drill sizes used in this layout, %d holes total",
	   AllDrills->DrillN, total_drills);

  /* ID the user. */
  pwentry = getpwuid (getuid ());
  /* Create a portable timestamp. */
  currenttime = time (NULL);
  strftime (utcTime, sizeof utcTime, "%c UTC", gmtime (&currenttime));

  yoff = -TEXT_LINE;
  text_at (200000, yoff, 0, "Dimensions: %d mils wide, %d mils high",
	   PCB->MaxWidth/100, PCB->MaxHeight/100);
  yoff -= TEXT_LINE;
  text_at (200000, yoff, 0, "Date: %s", utcTime);
  yoff -= TEXT_LINE;
  text_at (200000, yoff, 0, "Author: %s", pwentry->pw_gecos);
  yoff -= TEXT_LINE;
  text_at (200000, yoff, 0, "Title: %s - Fabrication Drawing",
	   UNKNOWN (PCB->Name));

  text_at (PCB->MaxWidth / 2, PCB->MaxHeight + 2000, 1,
	   "Board outline is the centerline of this 10 mil rectangle - 0,0 to %d,%d mils",
	   PCB->MaxWidth/100, PCB->MaxHeight/100);

#if 0
  ELEMENT_LOOP (PCB->Data, 
    {
      Device->ElementPackage (element);
    }
  );
#endif

  ClosePrintFile ();
  return (0);
}

/* ----------------------------------------------------------------------
 * generates printout
 * mirroring, rotating, scaling and the request for outlines and/or
 * alignment targets were already determined by a dialog
 *
 * output is written either to several files
 * whitespaces have already been removed from 'Command'
 *
 * 'MirrorFlag' has to be negated by the device dependend code if
 * (0,0) is in the lower/left corner for the output device. Some
 * adjustments to the offsets will also be necessary in this case
 *
 * the offsets have to be adjusted if either 'OutlineFlag' or
 * 'AlignmentFlag' is set
 *
 * all offsets are in mil
 */
int
Print (char *Command, float Scale,
       Boolean MirrorFlag, Boolean RotateFlag,
       Boolean ColorFlag, Boolean InvertFlag,
       Boolean OutlineFlag, Boolean AlignmentFlag,
       Boolean DrillHelperFlag, Boolean DOSFlag,
       PrintDeviceTypePtr PrintDevice, MediaTypePtr Media,
       Location OffsetX, Location OffsetY, Boolean SilkscreenTextFlag)
{
  /* it's not OK to override all files -> user interaction
   * is required if a file exists
   */
  ReplaceOK = False;

  /* save pointer... in global identifier */
  Device = PrintDevice;
  GlobalColorFlag = ColorFlag;
  GlobalAlignmentFlag = AlignmentFlag;
  GlobalOutlineFlag = OutlineFlag;
  GlobalDrillHelperFlag = DrillHelperFlag;
  GlobalDOSFlag = DOSFlag;
  GlobalCommand = Command;

  /* set the info struct for the device driver */
  DeviceFlags.SelectedMedia = Media;
  DeviceFlags.MirrorFlag = MirrorFlag;
  DeviceFlags.RotateFlag = RotateFlag;
  DeviceFlags.InvertFlag = InvertFlag;
  DeviceFlags.OffsetX = OffsetX;
  DeviceFlags.OffsetY = OffsetY;
  DeviceFlags.Scale = Scale;

  /* set bounding box coordinates;
   * the layout has already been checked to be non-empty
   * selecting outlines or alignment targets causes adjustments
   */
  if (OutlineFlag)
    {
      DeviceFlags.BoundingBox.X1 = 0;
      DeviceFlags.BoundingBox.Y1 = 0;
      DeviceFlags.BoundingBox.X2 = PCB->MaxWidth;
      DeviceFlags.BoundingBox.Y2 = PCB->MaxHeight;
    }
  else
    DeviceFlags.BoundingBox = *GetDataBoundingBox (PCB->Data);

  if (AlignmentFlag)
    {
      DeviceFlags.BoundingBox.X1 = -2 * Settings.AlignmentDistance;
      DeviceFlags.BoundingBox.Y1 = -2 * Settings.AlignmentDistance;
      DeviceFlags.BoundingBox.X2 =
	PCB->MaxWidth + 2 * Settings.AlignmentDistance;
      DeviceFlags.BoundingBox.Y2 =
	PCB->MaxHeight + 2 * Settings.AlignmentDistance;
    }
  watchCursor ();
  Device->init (&DeviceFlags);
  /* OK, call all necessary subroutines */
  if (PrintLayergroups () || PrintSilkscreen () ||
      PrintDrill () || PrintMask () || PrintPaste () || PrintFab ())
    return (1);
  Device->Exit ();
  restoreCursor ();
  return (0);
}
