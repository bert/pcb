/* $Id$ */

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


/* printing routines
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"

#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <pwd.h>
#include <setjmp.h>


#include "data.h"
#include "dev_ps.h"
#include "dev_rs274x.h"
#include "drill.h"
#include "file.h"
#include "find.h"
#include "error.h"
#include "misc.h"
#include "print.h"
#include "polygon.h"
#include "rtree.h"
#include "search.h"

#include "gui.h"


RCSID("$Id$");




/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static PrintDeviceTypePtr Device;
static PrintInitType DeviceFlags;
static Boolean GlobalOutlineFlag,	/* copy of local ident. */
  GlobalAlignmentFlag, GlobalDrillHelperFlag, GlobalColorFlag, GlobalDOSFlag, ReplaceOK;	/* ok two overriode all */
/* print output files */
static char *GlobalCommand;
static Boolean polarity_called = False;
static Boolean negative_plane;


enum {
  FILE_DRILL,
  FILE_LAYER,
  FILE_BOM
};

typedef struct _StringList
{
  char *str;
  struct _StringList *next;
} StringList;

typedef struct _BomList
{
  char *descr;
  char *value;
  int num;
  StringList *refdes;
  struct _BomList *next;
} BomList;


/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void SetPrintColor (GdkColor *);
static FILE *OpenPrintFile (char *, int);
static int SetupPrintFile (char *, char *, int);
static int ClosePrintFile (void);
static int FPrintOutline (void);
static int FPrintAlignment (void);
static int PrintLayergroups (void);
static int PrintSilkscreen (void);
static int PrintDrill (void);
static int PrintMask (void);
static int PrintPaste (void);
static int PrintBOM (void);

static char *CleanBOMString (char *);
static double xyToAngle(double, double);
static BomList *bom_insert (char *, char *, char *, BomList *);
static StringList *string_insert(char *, StringList *);

static void
DoPolarity (void)
{
  if (!polarity_called)
    {
      polarity_called = True;
      Device->Polarity (2);
    }
}

/* ----------------------------------------------------------------------
 * queries color from X11 database and calls device  command
 * black is default on errors
 */
static void
SetPrintColor (GdkColor *color)
{
  /* do nothing if no colors are requested */
  if (!GlobalColorFlag)
    return;

  Device->SetColor (color);
}

/* ---------------------------------------------------------------------------
 * opens a file for printing
 */
static FILE *
OpenPrintFile (char *FileExtention, int file_type)
{
  char *filename, *completeFilename;
  size_t length;
  FILE *fp;
  Boolean was_cancel;

  /* evaluate add extention and suffix to filename */
  if ((filename = ExpandFilename (NULL, GlobalCommand)) == NULL)
    filename = GlobalCommand;

  if ( (file_type == FILE_DRILL) && NSTRCMP (Device->Suffix, "gbr") == 0)
    {
      length = strlen (EMPTY (GlobalCommand)) + 1 +
	strlen (FileExtention) + 5;
      completeFilename = MyCalloc (length, sizeof (char), "OpenPrintFile()");
      sprintf (completeFilename, "%s%s%s.cnc",
	       GlobalDOSFlag ? "" : EMPTY (GlobalCommand),
	       GlobalDOSFlag ? "" : "_", FileExtention);
    }
  else if (file_type == FILE_BOM) 
    {
      length = strlen (EMPTY (GlobalCommand)) + 1 +
	strlen (FileExtention) + 5;
      completeFilename = MyCalloc (length, sizeof (char), "OpenPrintFile()");
      sprintf (completeFilename, "%s%s%s.txt",
	       GlobalDOSFlag ? "" : EMPTY (GlobalCommand),
	       GlobalDOSFlag ? "" : "_", FileExtention);
    }
  else
    {
      length = strlen (EMPTY (GlobalCommand)) + 1 +
	strlen (FileExtention) + 1 + strlen (Device->Suffix) + 1;
      completeFilename = MyCalloc (length, sizeof (char), "OpenPrintFile()");
      sprintf (completeFilename, "%s%s%s.%s",
	       GlobalDOSFlag ? "" : EMPTY (GlobalCommand),
	       GlobalDOSFlag ? "" : "_", FileExtention, Device->Suffix);
    }

  /* try to open all the file; if the value of
   * 'ReplaceOK' is set to 'True' no more confirmation is
   * requested for the sequence
   */
  was_cancel = False;

  fp = CheckAndOpenFile (completeFilename, !ReplaceOK, True,
       &ReplaceOK, &was_cancel);
  if (fp == NULL)
    {   /* get rid of annoying error message when hitting cancel */
      if (!was_cancel)
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
SetupPrintFile (char *FileExtention, char *Description, int file_type)
{
  char *message;

  if ((DeviceFlags.FP = OpenPrintFile (FileExtention, file_type)) == NULL)
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
 * callback functions for polygon clearances
 */
static int
any_callback (int type, void *ptr1, void *ptr2, void *ptr3,
	      LayerTypePtr lay, PolygonTypePtr poly)
{
  DoPolarity ();
  switch (type)
    {
    case LINE_TYPE:
      Device->Line ((LineTypePtr) ptr2, True);
      break;
    case ARC_TYPE:
      Device->Arc ((ArcTypePtr) ptr2, True);
      break;
    case PIN_TYPE:
    case VIA_TYPE:
      Device->PinOrVia ((PinTypePtr) ptr2, 1);
      break;
    case PAD_TYPE:
      Device->Pad ((PadTypePtr) ptr2, 1);
      break;
    default:
      Message ("hace bad plow callback\n");
    }
  return 0;
}

/* Returns TRUE if pin/via needs to be plotted.  Sets USETHERMALFLAG
   appropriately for the group being plotted. */
static int
pin_thermal_layer_group (PinTypePtr pin, Cardinal group)
{
  if (TEST_FLAG (HOLEFLAG, pin))
    return 0;
  CLEAR_FLAG (USETHERMALFLAG, pin);
  GROUP_LOOP (group);
  {
    if (TEST_PIP (number, pin)
	&& TEST_THERM (number, pin))
      {
	SET_FLAG (USETHERMALFLAG, pin);
	return 1;
      }
  }
  END_LOOP;
  return 1;
}

int
PrintOneGroup (Cardinal group, Boolean manageFile)
{
  char extention[12], description[18];
  Cardinal component, solder;
  Boolean noData;
  int Somepolys, use_mode;
  int PIPflag, Tflag;

  /* check for a special case where we can make a negative gnd/power plane
   * this can be helpful for lame-brained manufacturers that can't handle
   * composite layers. If there is no complex stuff on a layer, a simple
   * negative is possible. We require:
   * (1) No Lines, Text or Arcs
   * (2) Exactly one polygon
   * (3) No Pads
   * (4) All Pins and Vias pierce the polygon
   * should probably require (5) polygon is a rectangle but that's TODO
   */

  Somepolys = 0;
  PIPflag = Tflag = 0;

  /* see if we can make the special negative plane */
  if (NSTRCMP (Device->Name, "Gerber/RS-274X") == 0)
    negative_plane = True;
  else
    negative_plane = False;

  /* always include solder/component layers */
  component = GetLayerGroupNumberByNumber (MAX_LAYER + COMPONENT_LAYER);
  solder = GetLayerGroupNumberByNumber (MAX_LAYER + SOLDER_LAYER);
  noData = True;

  /* negative plane possibility checks */
  if (group == component)
    {
      noData = False;
      ALLPAD_LOOP (PCB->Data);
      {
	if (!TEST_FLAG (ONSOLDERFLAG, pad))
	  {
	    negative_plane = False;
	    break;
	  }
      }
      ENDALL_LOOP;
    }
  else if (group == solder)
    {
      noData = False;
      ALLPAD_LOOP (PCB->Data);
      {
	if (TEST_FLAG (ONSOLDERFLAG, pad))
	  {
	    negative_plane = False;
	    break;
	  }
      }
      ENDALL_LOOP;
    }
  GROUP_LOOP (group);
  {
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
	if (!TEST_FLAG (CLEARPOLYFLAG, layer->Polygon))
	  negative_plane = False;
      }
  }
  END_LOOP;
  /* skip empty layers */
  if (noData)
    return -1;
  /* complete the special plane check: All pins/vias must pierce one polygon */
  if (Somepolys != 1)
    negative_plane = False;
  if (negative_plane)
    ALLPIN_LOOP (PCB->Data);
  {
    GROUP_LOOP (group);
    {
      if (!TEST_PIP (number, pin))
	{
	  negative_plane = False;
	  break;
	}
    }
    END_LOOP;
  }
  ENDALL_LOOP;
  if (negative_plane)
    VIA_LOOP (PCB->Data);
  {
    GROUP_LOOP (group);
    {
      if (!TEST_PIP (number, via))
	{
	  negative_plane = False;
	  break;
	}
    }
    END_LOOP;
  }
  END_LOOP;

  if (manageFile)
    {
      /* setup extention and open new file */
      if (component == group)
	sprintf (extention, "front");
      else if (solder == group)
	sprintf (extention, "back");
      else
	sprintf (extention, "%s%i", GlobalDOSFlag ? "" : "group", group + 1);
      sprintf (description, "layergroup #%i", group + 1);

      if (SetupPrintFile (extention, description, FILE_LAYER))
	return (1);
      /* comment on what layers in group */
      if (Device->GroupID)
	Device->GroupID (group);
    }

  /* if negative plane is not possible draw and erase */
  if (!negative_plane)
    {
      /* indicate positive polarity */
      Device->Polarity (0);
      if (GlobalAlignmentFlag)
	FPrintAlignment ();

      /* print all polygons in the group that get clearances */
      GROUP_LOOP (group);
      {
	if (layer->PolygonN)
	  {
	    if (manageFile)
	      SetPrintColor (layer->Color);
	    POLYGON_LOOP (layer);
	    {
	      if (TEST_FLAG (CLEARPOLYFLAG, polygon))
		Device->Poly (polygon);
	    }
	    END_LOOP;
	  }
      }
      END_LOOP;
      /* clear the intersecting lines, arcs, pins and vias */
      if (Somepolys)
	{
	  BoxType all;

	  all.X1 = -MAX_COORD;
	  all.X2 = MAX_COORD;
	  all.Y1 = -MAX_COORD;
	  all.Y2 = MAX_COORD;
	  polarity_called = False;
	  PolygonPlows (group, &all, any_callback);
	  if (polarity_called)
	    Device->Polarity (3);
	}
      /* ok clearances are done, now print
       * the lines/arcs/text and non-clearing polygons
       */
      GROUP_LOOP (group);
      {
	if (manageFile)
	  SetPrintColor (layer->Color);
	POLYGON_LOOP (layer);
	{
	  if (!TEST_FLAG (CLEARPOLYFLAG, polygon))
	    Device->Poly (polygon);
	}
	END_LOOP;
	LINE_LOOP (layer);
	{
	  Device->Line (line, False);
	}
	END_LOOP;
	ARC_LOOP (layer);
	{
	  Device->Arc (arc, False);
	}
	END_LOOP;
	TEXT_LOOP (layer);
	{
	  Device->Text (text);
	}
	END_LOOP;
      }
      END_LOOP;
      use_mode = 0;
    }
  else
    {
      /* special negative plane */
      Device->Polarity (1);
      use_mode = 3;
    }
  /* now print the pins/pads and vias */
  if (manageFile)
    SetPrintColor (PCB->PinColor);
  ALLPIN_LOOP (PCB->Data);
  {
    if (pin_thermal_layer_group (pin, group))
      Device->PinOrVia (pin, use_mode);
  }
  ENDALL_LOOP;
  if (manageFile)
    SetPrintColor (PCB->ViaColor);
  VIA_LOOP (PCB->Data);
  {
    if (pin_thermal_layer_group (via, group))
      Device->PinOrVia (via, use_mode);
  }
  END_LOOP;
  if (group == component)
    {
      if (GlobalOutlineFlag)
	FPrintOutline ();
      ALLPAD_LOOP (PCB->Data);
      {
	if (!TEST_FLAG (ONSOLDERFLAG, pad))
	  Device->Pad (pad, 0);
      }
      ENDALL_LOOP;
    }
  else if (group == solder)
    {
      if (GlobalOutlineFlag)
	FPrintOutline ();
      ALLPAD_LOOP (PCB->Data);
      {
	if (TEST_FLAG (ONSOLDERFLAG, pad))
	  Device->Pad (pad, 0);
      }
      ENDALL_LOOP;
    }

  /* print drill-helper if requested */
  if (GlobalDrillHelperFlag && Device->DrillHelper)
    {
      if (manageFile)
	SetPrintColor (PCB->PinColor);
      ALLPIN_LOOP (PCB->Data);
      {
	Device->DrillHelper (pin, 1);
      }
      ENDALL_LOOP;
      if (manageFile)
	SetPrintColor (PCB->ViaColor);
      VIA_LOOP (PCB->Data);
      {
	Device->DrillHelper (via, 1);
      }
      END_LOOP;
    }
  return 0;
}

/* ---------------------------------------------------------------------------
 * prints all layergroups
 * returns != zero on error
 */
static int
PrintLayergroups (void)
{
  Cardinal group;
  int result;

  if (PCB->Data->RatN)
    {
      Message (_("Warning!  Rat lines in layout during printing!\n"
	       "You are not DONE with the layout!\n"));
    };

  for (group = 0; group < MAX_LAYER; group++)
    if (PCB->LayerGroups.Number[group])
      {
	result = PrintOneGroup (group, True);
	if (!result)
	  ClosePrintFile ();
	else if (result > 0)
	  return 1;
      }
  return (0);
}

struct silkInfo
{
  PinTypePtr pin;
  PadTypePtr pad;
  jmp_buf env;
};

static void
clearSilkPad (PadTypePtr pad)
{
  DoPolarity ();
  pad->Thickness -= pad->Clearance;
  Device->Pad (pad, 1);
  pad->Thickness += pad->Clearance;
}

static void
clearSilkPin (PinTypePtr pin)
{
  DoPolarity ();
  pin->Thickness -= pin->Clearance;
  Device->PinOrVia (pin, 1);
  pin->Thickness += pin->Clearance;
}

static int
silkPinElement_callback (const BoxType * box, void *cl)
{
  ElementTypePtr element = (ElementTypePtr) box;
  struct silkInfo *i = (struct silkInfo *) cl;

  LINE_LOOP (element);
  {
    if (IsPointOnLine (i->pin->X, i->pin->Y, 0.5 * i->pin->Thickness, line))
      {
	clearSilkPin (i->pin);
	longjmp (i->env, 1);
      }
  }
  END_LOOP;
  ARC_LOOP (element);
  {
    if (IsPointOnArc (i->pin->X, i->pin->Y, 0.5 * i->pin->Thickness, arc))
      {
	clearSilkPin (i->pin);
	longjmp (i->env, 1);
      }
  }
  END_LOOP;
  if (IsPointInBox
      (i->pin->X, i->pin->Y, &NAMEONPCB_TEXT (element).BoundingBox,
       i->pin->Thickness / 2))
    {
      clearSilkPin (i->pin);
      longjmp (i->env, 1);
    }
  return 0;
}

static int
silkPadElement_callback (const BoxType * box, void *cl)
{
  ElementTypePtr element = (ElementTypePtr) box;
  struct silkInfo *i = (struct silkInfo *) cl;

  LINE_LOOP (element);
  {
    if (LinePadIntersect (line, i->pad))
      {
	clearSilkPad (i->pad);
	longjmp (i->env, 1);
      }
  }
  END_LOOP;
  ARC_LOOP (element);
  {
    if (ArcPadIntersect (arc, i->pad))
      {
	clearSilkPad (i->pad);
	longjmp (i->env, 1);
      }
  }
  END_LOOP;
  return 0;
}

static int
silkPinLine_callback (const BoxType * box, void *cl)
{
  LineTypePtr line = (LineTypePtr) box;
  struct silkInfo *i = (struct silkInfo *) cl;

  if (IsPointOnLine (i->pin->X, i->pin->Y, 0.5 * i->pin->Thickness, line))
    {
      clearSilkPin (i->pin);
      longjmp (i->env, 1);
    }
  return 0;
}

static int
silkPinArc_callback (const BoxType * box, void *cl)
{
  ArcTypePtr arc = (ArcTypePtr) box;
  struct silkInfo *i = (struct silkInfo *) cl;

  if (IsPointOnArc (i->pin->X, i->pin->Y, 0.5 * i->pin->Thickness, arc))
    {
      clearSilkPin (i->pin);
      longjmp (i->env, 1);
    }
  return 0;
}

static int
silkPadLine_callback (const BoxType * box, void *cl)
{
  LineTypePtr line = (LineTypePtr) box;
  struct silkInfo *i = (struct silkInfo *) cl;

  if (LinePadIntersect (line, i->pad))
    {
      clearSilkPad (i->pad);
      longjmp (i->env, 1);
    }
  return 0;
}

static int
silkPadArc_callback (const BoxType * box, void *cl)
{
  ArcTypePtr arc = (ArcTypePtr) box;
  struct silkInfo *i = (struct silkInfo *) cl;

  if (ArcPadIntersect (arc, i->pad))
    {
      clearSilkPad (i->pad);
      longjmp (i->env, 1);
    }
  return 0;
}

static int
silkPinText_callback (const BoxType * box, void *cl)
{
  TextTypePtr text = (TextTypePtr) box;
  struct silkInfo *i = (struct silkInfo *) cl;

  if (IsPointInBox (i->pin->X, i->pin->Y, &text->BoundingBox,
		    i->pin->Thickness / 2))
    {
      clearSilkPin (i->pin);
      longjmp (i->env, 1);
    }
  return 0;
}

static int
silkPadText_callback (const BoxType * box, void *cl)
{
  TextTypePtr text = (TextTypePtr) box;
  struct silkInfo *i = (struct silkInfo *) cl;
  BoxTypePtr b1;
  BoxType b2;
  BDimension w;

  w = i->pad->Thickness / 2;
  b2.X1 = MIN (i->pad->Point1.X, i->pad->Point2.X) - w;
  b2.X2 = MAX (i->pad->Point1.X, i->pad->Point2.X) + w;
  b2.Y1 = MIN (i->pad->Point1.Y, i->pad->Point2.Y) - w;
  b2.Y2 = MAX (i->pad->Point1.Y, i->pad->Point2.Y) + w;
  b1 = &text->BoundingBox;
  if (b1->X1 <= b2.X2 && b1->X2 >= b2.X1 && b1->Y1 <= b2.Y2
      && b1->Y2 >= b2.Y1)
    {
      clearSilkPad (i->pad);
      longjmp (i->env, 1);
    }
  return 0;
}

void
DoSilkPrint (Cardinal i, LayerTypePtr layer, Boolean clip)
{
  struct silkInfo info;

  ELEMENT_LOOP (PCB->Data);
  {
    if ((TEST_FLAG (ONSOLDERFLAG, element) == 0) == (i == 0))
      Device->ElementPackage (element);
  }
  END_LOOP;

  POLYGON_LOOP (layer);
  {
    Device->Poly (polygon);
  }
  END_LOOP;
  LINE_LOOP (layer);
  {
    Device->Line (line, False);
  }
  END_LOOP;
  ARC_LOOP (layer);
  {
    Device->Arc (arc, False);
  }
  END_LOOP;
  TEXT_LOOP (layer);
  {
    Device->Text (text);
  }
  END_LOOP;

  if (!clip)
    return;

  /* erase any silk that might have crossed solder areas */
  polarity_called = False;
  ALLPIN_LOOP (PCB->Data);
  {
    info.pin = pin;
    if (setjmp (info.env) == 0)
      {
	r_search (PCB->Data->element_tree, &pin->BoundingBox, NULL,
		  silkPinElement_callback, &info);
	r_search (PCB->Data->name_tree[NAMEONPCB_INDEX], &pin->BoundingBox, NULL,
		  silkPinText_callback, &info);
	r_search (layer->line_tree, &pin->BoundingBox, NULL,
		  silkPinLine_callback, &info);
	r_search (layer->arc_tree, &pin->BoundingBox, NULL,
		  silkPinArc_callback, &info);
	r_search (layer->text_tree, &pin->BoundingBox, NULL,
		  silkPinText_callback, &info);
	POLYGON_LOOP (layer);
	{
	  if (IsPointInPolygon (pin->X, pin->Y, 0.5 * pin->Thickness,
				polygon))
	    {
	      DoPolarity ();
	      clearSilkPin (pin);
	      break;
	    }
	}
	END_LOOP;
      }
  }
  ENDALL_LOOP;
  VIA_LOOP (PCB->Data);
  {
    info.pin = via;
    if (via->Mask && setjmp (info.env) == 0)
      {
	r_search (PCB->Data->element_tree, &via->BoundingBox, NULL,
		  silkPinElement_callback, &info);
	r_search (PCB->Data->name_tree[NAMEONPCB_INDEX], &via->BoundingBox, NULL,
		  silkPinText_callback, &info);
	r_search (layer->line_tree, &via->BoundingBox, NULL,
		  silkPinLine_callback, &info);
	r_search (layer->arc_tree, &via->BoundingBox, NULL,
		  silkPinArc_callback, &info);
	r_search (layer->text_tree, &via->BoundingBox, NULL,
		  silkPinText_callback, &info);
	POLYGON_LOOP (layer);
	{
	  if (IsPointInPolygon
	      (via->X, via->Y, 0.5 * via->Thickness, polygon))
	    {
	      DoPolarity ();
	      clearSilkPin (via);
	      break;
	    }
	}
	END_LOOP;
      }
  }
  END_LOOP;
  ALLPAD_LOOP (PCB->Data);
  {
    if ((TEST_FLAG (ONSOLDERFLAG, element) == 0) != (i == 0))
	continue;
    info.pad = pad;
    if (setjmp (info.env) == 0)
      {
	r_search (PCB->Data->element_tree, &pad->BoundingBox, NULL,
		  silkPadElement_callback, &info);
	r_search (PCB->Data->name_tree[NAMEONPCB_INDEX], &pad->BoundingBox, NULL,
		  silkPadText_callback, &info);
	r_search (layer->line_tree, &pad->BoundingBox, NULL,
		  silkPadLine_callback, &info);
	r_search (layer->arc_tree, &pad->BoundingBox, NULL,
		  silkPadArc_callback, &info);
	r_search (layer->text_tree, &pad->BoundingBox, NULL,
		  silkPadText_callback, &info);
	POLYGON_LOOP (layer);
	{
	  CLEAR_FLAG (CLEARPOLYFLAG, polygon);
	  if (IsPadInPolygon (pad, polygon))
	    {
	      DoPolarity ();
	      clearSilkPad (pad);
	      break;
	    }
	}
	END_LOOP;
      }
  }
  ENDALL_LOOP;
}

/* ---------------------------------------------------------------------------
 * prints solder and component side silk screens
 * first print element outlines and names, then silk layer
 * returns != zero on error
 */
static int
PrintSilkscreen (void)
{
  static char *extention[2] = { "frontsilk", "backsilk" }, *DOSextention[2] =
  {
  "fsilk", "bsilk"}
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
      ELEMENT_LOOP (PCB->Data);
      {
	if ((TEST_FLAG (ONSOLDERFLAG, element) == 0) == (i == 0))
	  {
	    noData = False;
	    break;
	  }
      }
      END_LOOP;
      if (layer->PolygonN || layer->LineN || layer->ArcN || layer->TextN)
	noData = False;
      if (noData)
	continue;
      /* start with the component side */
      if (SetupPrintFile (GlobalDOSFlag ? DOSextention[i] : extention[i],
			  description[i], FILE_LAYER))
	return (1);
      /* positive polarity */
      Device->Polarity (0);
      /* print element outlines and canonical, instance or
       * value descriptions
       */
      if (GlobalAlignmentFlag)
	FPrintAlignment ();

      SetPrintColor (PCB->ElementColor);
      DoSilkPrint (i, layer, True);
      ClosePrintFile ();
    }
  return (0);
}

#define DRIFT 1.05
/* ---------------------------------------------------------------------------
 * prints assembly drawing, it prints the copper in light grey
 * then un-clipped silkscreen; it draws the front, then the
 * mirrored back
 */
static int
PrintAssembly (void)
{
  static char *extention = "assembly", *DOSextention = "assem", *description =
    "assembly drawing";
  GdkColor rgb;
  LocationType y2;
  Cardinal layer;
  LayerTypePtr layptr;

  /* no assembly concept in gerber */
  if (NSTRCMP (Device->Name, "Gerber/RS-274X") == 0)
    return 0;

  y2 = DeviceFlags.BoundingBox.Y2 - DeviceFlags.BoundingBox.Y1;
  DeviceFlags.BoundingBox.Y2 += y2 * DRIFT;
  y2 *= (DRIFT - 1);
  DeviceFlags.Scale /= 2;
  if (SetupPrintFile
      (GlobalDOSFlag ? DOSextention : extention, description, FILE_LAYER))
    return (1);
  /* start with 10% intensity */
  rgb.red = rgb.blue = rgb.green = 52428;
  Device->SetColor (&rgb);
  layer = GetLayerGroupNumberByNumber (MAX_LAYER + COMPONENT_LAYER);
  if (PrintOneGroup (layer, False))
    return 1;
  DeviceFlags.BoundingBox.Y1 += y2;
  DeviceFlags.BoundingBox.Y2 += y2;
  DeviceFlags.MirrorFlag = !DeviceFlags.MirrorFlag;
  Device->Preamble (&DeviceFlags, "");
  Device->SetColor (&rgb);
  layer = GetLayerGroupNumberByNumber (MAX_LAYER + SOLDER_LAYER);
  if (PrintOneGroup (layer, False))
    return 1;
  /* switch to black */
  rgb.red = rgb.blue = rgb.green = 0;
  Device->SetColor (&rgb);
  layptr = LAYER_PTR (MAX_LAYER + SOLDER_LAYER);
  DoSilkPrint (1, layptr, False);
  DeviceFlags.MirrorFlag = !DeviceFlags.MirrorFlag;
  DeviceFlags.BoundingBox.Y1 -= y2;
  DeviceFlags.BoundingBox.Y2 -= y2;
  Device->Preamble (&DeviceFlags, "");
  Device->SetColor (&rgb);
  layptr = LAYER_PTR (MAX_LAYER + COMPONENT_LAYER);
  DoSilkPrint (0, layptr, False);
  ClosePrintFile ();
  return 0;
}


/* ---------------------------------------------------------------------------
 * prints solder and component side solder paste
 * returns != zero on error
 */
static int
PrintPaste (void)
{
  static char *extention[2] = {
    "frontpaste", "backpaste"
  }, *DOSextention[2] =
  {
  "fpaste", "bpaste"}
  , *description[2] =
  {
  "solder paste, component side", "solder paste, solder side"};
  int i;
  Boolean NoData;
  /* loop over both sides, start with component */
  for (i = 0; i < 2; i++)
    {
      NoData = True;
      ALLPAD_LOOP (PCB->Data);
      {
	if ((TEST_FLAG (ONSOLDERFLAG, pad) == 0) == (i == 0))
	  {
	    NoData = False;
	    break;
	  }
      }
      ENDALL_LOOP;
      /* skip empty files */
      if (NoData)
	continue;
      /* start with the component side */
      if (SetupPrintFile
	  (GlobalDOSFlag ? DOSextention[i] : extention[i],
	   description[i], FILE_LAYER))
	return (1);
      Device->Polarity (0);
      if (GlobalAlignmentFlag)
	FPrintAlignment ();
      SetPrintColor (PCB->ElementColor);
      ALLPAD_LOOP (PCB->Data);
      {
	if ((TEST_FLAG (ONSOLDERFLAG, pad) == 0) == (i == 0))
	  Device->Pad (pad, 3);
      }
      ENDALL_LOOP;
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
      if (SetupPrintFile
	  (GlobalDOSFlag ? "pdrill" : "plated-drill",
	   "drill information", FILE_DRILL))
	return (1);
      SetPrintColor (PCB->PinColor);
      AllDrills = GetDrillInfo (PCB->Data);
      index = 0;
      DRILL_LOOP (AllDrills);
      {
	index++;
	for (i = 0; i < drill->PinN; i++)
	  if (!TEST_FLAG (HOLEFLAG, drill->Pin[i]))
	    Device->Drill (drill->Pin[i], index);
	  else if (GlobalDrillHelperFlag)
	    Device->Drill (drill->Pin[i], index);
      }
      END_LOOP;
      /* close the file */
      ClosePrintFile ();
      if (GlobalDrillHelperFlag)
	{
	  FreeDrillInfo (AllDrills);
	  return (0);
	}
      if (SetupPrintFile
	  (GlobalDOSFlag ? "udrill" : "unplated-drill",
	   "drill information", FILE_DRILL))
	{
	  FreeDrillInfo (AllDrills);
	  return (1);
	}
      SetPrintColor (PCB->PinColor);
      index = 0;
      DRILL_LOOP (AllDrills);
      {
	index++;
	for (i = 0; i < drill->PinN; i++)
	  if (TEST_FLAG (HOLEFLAG, drill->Pin[i]))
	    Device->Drill (drill->Pin[i], index);
      }
      END_LOOP;
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
  static char *extention[2] = {
    "frontmask", "backmask"
  }, *DOSextention[2] =
  {
  "fmask", "bmask"}
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
      if (SetupPrintFile
	  (GlobalDOSFlag ? DOSextention[i] : extention[i],
	   description[i], FILE_LAYER))
	return (1);
      Device->Polarity (1);
      SetPrintColor (PCB->PinColor);
      if (GlobalAlignmentFlag)
	FPrintAlignment ();
      ALLPAD_LOOP (PCB->Data);
      {
	if ((TEST_FLAG (ONSOLDERFLAG, pad) == 0) == (i == 0))
	  Device->Pad (pad, 2);
      }
      ENDALL_LOOP;
      ALLPIN_LOOP (PCB->Data);
      {
	Device->PinOrVia (pin, 2);
      }
      ENDALL_LOOP;
      VIA_LOOP (PCB->Data);
      {
	Device->PinOrVia (via, 2);
      }
      END_LOOP;
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
  l.Flags = NoFlags();
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
  a.Flags = NoFlags();
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
  t.Flags = NoFlags();
  t.X = x;
  t.Y = y;
  for (i = 0; tmp[i]; i++)
    w +=
      (font->Symbol[(int) tmp[i]].Width + font->Symbol[(int) tmp[i]].Delta);
  w = w * TEXT_SIZE / 100;
  t.X -= w * (align & 3) / 2;
  if (t.X < 0)
    t.X = 0;
  Device->Text (&t);
  if (align & 8)
    fab_line (t.X,
	      t.Y +
	      font->MaxHeight * TEXT_SIZE /
	      100 + 1000, t.X + w,
	      t.Y + font->MaxHeight * TEXT_SIZE / 100 + 1000, FAB_LINE_W);
}

/* Y, +, X, circle, square */
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
    case 1:			/* + */
      ;
      fab_line (x, y - s2, x, y + s2, FAB_LINE_W);
      fab_line (x - s2, y, x + s2, y, FAB_LINE_W);
      for (i = 1; i <= size; i++)
	{
	  fab_line (x - i * 1600, y - i * 1600, x + i * 1600,
		    y - i * 1600, FAB_LINE_W);
	  fab_line (x - i * 1600, y - i * 1600, x - i * 1600,
		    y + i * 1600, FAB_LINE_W);
	  fab_line (x - i * 1600, y + i * 1600, x + i * 1600,
		    y + i * 1600, FAB_LINE_W);
	  fab_line (x + i * 1600, y - i * 1600, x + i * 1600,
		    y + i * 1600, FAB_LINE_W);
	}
      break;
    case 2:			/* X */ ;
      fab_line (x - s2 * 3 / 4, y - s2 * 3 / 4, x + s2 * 3 / 4,
		y + s2 * 3 / 4, FAB_LINE_W);
      fab_line (x - s2 * 3 / 4, y + s2 * 3 / 4, x + s2 * 3 / 4,
		y - s2 * 3 / 4, FAB_LINE_W);
      for (i = 1; i <= size; i++)
	{
	  fab_line (x - i * 1600, y - i * 1600, x + i * 1600,
		    y - i * 1600, FAB_LINE_W);
	  fab_line (x - i * 1600, y - i * 1600, x - i * 1600,
		    y + i * 1600, FAB_LINE_W);
	  fab_line (x - i * 1600, y + i * 1600, x + i * 1600,
		    y + i * 1600, FAB_LINE_W);
	  fab_line (x + i * 1600, y - i * 1600, x + i * 1600,
		    y + i * 1600, FAB_LINE_W);
	}
      break;
    case 3:			/* circle */ ;
      for (i = 0; i <= size; i++)
	fab_circle (x, y, (i + 1) * 1600 - 800, FAB_LINE_W);
      break;
    case 4:			/* square */
      for (i = 1; i <= size + 1; i++)
	{
	  fab_line (x - i * 1600, y - i * 1600, x + i * 1600,
		    y - i * 1600, FAB_LINE_W);
	  fab_line (x - i * 1600, y - i * 1600, x - i * 1600,
		    y + i * 1600, FAB_LINE_W);
	  fab_line (x - i * 1600, y + i * 1600, x + i * 1600,
		    y + i * 1600, FAB_LINE_W);
	  fab_line (x + i * 1600, y - i * 1600, x + i * 1600,
		    y + i * 1600, FAB_LINE_W);
	}
      break;
    }
}

static char *
fab_author (void)
{
  static struct passwd *pwentry;
  static char *fab_author = 0;

  if (!fab_author)
    {
      if (Settings.FabAuthor && Settings.FabAuthor[0])
	fab_author = Settings.FabAuthor;
      else
	{
	  /* ID the user. */
	  pwentry = getpwuid (getuid ());
	  fab_author = pwentry->pw_gecos;
	}
    }
  return fab_author;
}

static int
PrintFab (void)
{
  PinType tmp_pin;
  DrillInfoTypePtr AllDrills;
  int i, n, yoff, total_drills = 0, ds = 0;
  time_t currenttime;
  char utcTime[64];
  if (SetupPrintFile ("fab", "Fabrication Drawing", FILE_LAYER))
    return (1);
  Device->Polarity (0);
  tmp_pin.Flags = NoFlags();
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

  /*
   * When we only have a few drill sizes we need to make sure the
   * drill table header doesn't fall on top of the board info
   * section.
   */
  if (AllDrills->DrillN < 4 ) 
    {
      yoff -= (4 - AllDrills->DrillN) * TEXT_LINE;
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
	drill_sym (TEST_FLAG (HOLEFLAG, drill->Pin[i]) ?
		   unplated_sym : plated_sym, drill->Pin[i]->X,
		   drill->Pin[i]->Y);
      if (plated_sym != -1)
	{
	  drill_sym (plated_sym, 100 * TEXT_SIZE, yoff + 100 * TEXT_SIZE / 4);
	  text_at (135000, yoff, 200, "YES");
	  text_at (98000, yoff, 200, "%d", drill->PinCount + drill->ViaCount);

	  if (unplated_sym != -1)
	    yoff -= TEXT_LINE;
	}
      if (unplated_sym != -1)
	{
	  drill_sym (unplated_sym, 100 * TEXT_SIZE,
		     yoff + 100 * TEXT_SIZE / 4);
	  text_at (140000, yoff, 200, "NO");
	  text_at (98000, yoff, 200, "%d", drill->UnplatedCount);
	}
      SetPrintColor (PCB->ElementColor);
      text_at (45000, yoff, 200, "%0.3f",
	       drill->DrillSize / 100000. + 0.0004);
      if (plated_sym != -1 && unplated_sym != -1)
	text_at (45000, yoff + TEXT_LINE, 200, "%0.3f",
		 drill->DrillSize / 100000. + 0.0004);
      yoff -= TEXT_LINE;
      total_drills += drill->PinCount;
      total_drills += drill->ViaCount;
    }

  SetPrintColor (PCB->ElementColor);
  text_at (0, yoff, 900, "Symbol");
  text_at (41000, yoff, 900, "Diam. (Inch)");
  text_at (95000, yoff, 900, "Count");
  text_at (130000, yoff, 900, "Plated?");
  yoff -= TEXT_LINE;
  text_at (0, yoff, 0,
	   "There are %d different drill sizes used in this layout, %d holes total",
	   AllDrills->DrillN, total_drills);
  /* Create a portable timestamp. */
  currenttime = time (NULL);
  strftime (utcTime, sizeof utcTime, "%c UTC", gmtime (&currenttime));
  yoff = -TEXT_LINE;
  for (i = 0; i < MAX_LAYER; i++)
    if (LAYER_PTR(i)->Name)
      {
	if (strcasecmp ("route", LAYER_PTR (i)->Name) == 0)
	  break;
	if (strcasecmp ("outline", LAYER_PTR (i)->Name) == 0)
	  break;
      }
  if (i == MAX_LAYER)
    {
      text_at (200000, yoff, 0,
	       "Maximum Dimensions: %d mils wide, %d mils high",
	       PCB->MaxWidth / 100, PCB->MaxHeight / 100);
      FPrintOutline ();
      text_at (PCB->MaxWidth / 2, PCB->MaxHeight + 2000, 1,
	       "Board outline is the centerline of this 10 mil"
	       " rectangle - 0,0 to %d,%d mils",
	       PCB->MaxWidth / 100, PCB->MaxHeight / 100);
    }
  else
    {
      LayerTypePtr layer = LAYER_PTR (i);
      LINE_LOOP (layer);
      {
	Device->Line (line, False);
      }
      END_LOOP;
      ARC_LOOP (layer);
      {
	Device->Arc (arc, False);
      }
      END_LOOP;
      TEXT_LOOP (layer);
      {
	Device->Text (text);
      }
      END_LOOP;
      text_at (PCB->MaxWidth / 2, PCB->MaxHeight + 2000, 1,
	       "Board outline is the centerline of this path");
    }
  yoff -= TEXT_LINE;
  text_at (200000, yoff, 0, "Date: %s", utcTime);
  yoff -= TEXT_LINE;
  text_at (200000, yoff, 0, "Author: %s", fab_author());
  yoff -= TEXT_LINE;
  text_at (200000, yoff, 0,
	   "Title: %s - Fabrication Drawing", UNKNOWN (PCB->Name));
  ClosePrintFile ();
  return (0);
}

/* ---------------------------------------------------------------------------
 * prints a centroid file in a format which includes data needed by a
 * pick and place machine.  Further formatting for a particular factory setup
 * can easily be generated with awk or perl.  In addition, a bill of materials
 * file is generated which can be used for checking stock and purchasing needed
 * materials.
 * returns != zero on error
 */
static int
PrintBOM (void)
{
  char utcTime[64];
  double x, y, theta = 0.0;
  double sumx, sumy;
  double pin1x = 0.0, pin1y = 0.0, pin1angle = 0.0;
  double pin2x = 0.0, pin2y = 0.0, pin2angle;
  int found_pin1;
  int found_pin2;
  int pin_cnt;
  time_t currenttime;
  FILE *fp;
  BomList *bom = NULL;
  BomList *lastb;
  StringList *lasts;

  /* Only print the BOM when we do gerber output */
  if ( NSTRCMP (Device->Suffix, "gbr") != 0 )
    return 0;

  if ( (fp = OpenPrintFile ("xy", FILE_BOM)) == NULL)
    return (1);

  /* Create a portable timestamp. */
  currenttime = time (NULL);
  strftime (utcTime, sizeof (utcTime), "%c UTC", gmtime (&currenttime));

  fprintf (fp, "# $Id");
  fprintf (fp, "$\n");
  fprintf (fp, "# PcbXY Version 1.0\n");
  fprintf (fp, "# Date: %s\n", utcTime);
  fprintf (fp, "# Author: %s\n", fab_author());
  fprintf (fp, "# Title: %s - PCB X-Y\n", UNKNOWN (PCB->Name));
  fprintf (fp, "# RefDes, Description, Value, X, Y, rotation, top/bottom\n");
  fprintf (fp, "# X,Y in mils.  rotation in degrees.\n");
  fprintf (fp, "# --------------------------------------------\n");

  /*
   * For each element we calculate the centroid of the footprint.
   * In addition, we need to extract some notion of rotation.  
   * While here generate the BOM list
   */

  ELEMENT_LOOP (PCB->Data);
  {

    /* initialize our pin count and our totals for finding the
       centriod */
    pin_cnt = 0;
    sumx = 0.0;
    sumy = 0.0;
    found_pin1 = 0;
    found_pin2 = 0;
    
    /* insert this component into the bill of materials list */
    bom = bom_insert ( UNKNOWN (NAMEONPCB_NAME (element)),
		       UNKNOWN (DESCRIPTION_NAME (element)),
		       UNKNOWN (VALUE_NAME (element)), bom);
      
    
    /*
     * iterate over the pins and pads keeping a running count of how
     * many pins/pads total and the sum of x and y coordinates
     * 
     * While we're at it, store the location of pin/pad #1 and #2 if
     * we can find them
     */

    PIN_LOOP (element);
    {
      sumx += (double) pin->X;
      sumy += (double) pin->Y;
      pin_cnt++;

      if (NSTRCMP(pin->Number, "1") == 0)
	{
	  pin1x = (double) pin->X;
	  pin1y = (double) pin->Y;
	  pin1angle = 0.0; /* pins have no notion of angle */
	  found_pin1 = 1;
	}
      else if (NSTRCMP(pin->Number, "2") == 0)
	{
	  pin2x = (double) pin->X;
	  pin2y = (double) pin->Y;
	  pin2angle = 0.0; /* pins have no notion of angle */
	  found_pin2 = 1;
	}
    }
    END_LOOP;
    
    PAD_LOOP (element);
    {
      sumx +=  (pad->Point1.X + pad->Point2.X)/2.0;
      sumy +=  (pad->Point1.Y + pad->Point2.Y)/2.0;
      pin_cnt++;

      if (NSTRCMP(pad->Number, "1") == 0)
	{
	  pin1x = (double) (pad->Point1.X + pad->Point2.X)/2.0;
	  pin1y = (double) (pad->Point1.Y + pad->Point2.Y)/2.0;
	  /*
	   * NOTE:  We swap the Y points because in PCB, the Y-axis
	   * is inverted.  Increasing Y moves down.  We want to deal
	   * in the usual increasing Y moves up coordinates though.
	   */
	  pin1angle = (180.0 / M_PI) * atan2(pad->Point1.Y - pad->Point2.Y ,
					     pad->Point2.X - pad->Point1.X );
	  found_pin1 = 1;
	}
      else if (NSTRCMP(pad->Number, "2") == 0)
	{
	  pin2x = (double) (pad->Point1.X + pad->Point2.X)/2.0;
	  pin2y = (double) (pad->Point1.Y + pad->Point2.Y)/2.0;
	  pin2angle = (180.0 / M_PI) * atan2(pad->Point1.Y - pad->Point2.Y ,
					     pad->Point2.X - pad->Point1.X );
	  found_pin2 = 1;
	}

    }
    END_LOOP;

    if ( pin_cnt > 0) 
      {
	x = sumx / (double) pin_cnt;
	y = sumy / (double) pin_cnt;

	if (found_pin1) 
	  {
	    /* recenter pin #1 onto the axis which cross at the part
	       centroid */
	    pin1x -= x;
	    pin1y -= y;
	    pin1y = -1.0 * pin1y;

	    /* if only 1 pin, use pin 1's angle */
	    if (pin_cnt == 1)
	      theta = pin1angle;
	    else
	      {
		/* if pin #1 is at (0,0) use pin #2 for rotation */
		if ( (pin1x == 0.0) && (pin1y == 0.0) )
		  {
		    if (found_pin2)
		      theta = xyToAngle( pin2x, pin2y);
		    else
		      {
			Message ("PrintBOM(): unable to figure out angle of element\n"
				 "     %s because pin #1 is at the centroid of the part.\n"
				 "     and I could not find pin #2's location\n"
				 "     Setting to %g degrees\n",
				 UNKNOWN (NAMEONPCB_NAME(element) ), theta );
		      }
		  }
		else
		  theta = xyToAngle( pin1x, pin1y);
	      }
	  }
	/* we did not find pin #1 */
	else
	  {
	    theta = 0.0;
	    Message ("PrintBOM(): unable to figure out angle because I could\n"
		     "     not find pin #1 of element %s\n"
		     "     Setting to %g degrees\n",
		     UNKNOWN (NAMEONPCB_NAME(element) ),
		     theta );
	  }
	
	fprintf (fp, "%s,\"%s\",\"%s\",%.2f,%.2f,%g,%s\n", 
		 CleanBOMString(UNKNOWN (NAMEONPCB_NAME(element) )),
		 CleanBOMString(UNKNOWN (DESCRIPTION_NAME(element) )),
		 CleanBOMString(UNKNOWN (VALUE_NAME(element) )),
#if 0
		 (double) element->MarkX, (double) element->MarkY,
#else
		 0.01*x, 0.01*y,
#endif
		 theta,
		 FRONT(element) == 1 ? "top" : "bottom");
      }
  }
  END_LOOP;
  
  fclose (fp);

  /* Now print out a Bill of Materials file */
  
  if ( (fp = OpenPrintFile ("bom", FILE_BOM)) == NULL)
    return (1);
    
  fprintf (fp, "# $Id");
  fprintf (fp, "$\n");
  fprintf (fp, "# PcbBOM Version 1.0\n");
  fprintf (fp, "# Date: %s\n", utcTime);
  fprintf (fp, "# Author: %s\n", fab_author());
  fprintf (fp, "# Title: %s - PCB BOM\n", UNKNOWN (PCB->Name));
  fprintf (fp, "# Quantity, Description, Value, RefDes\n");
  fprintf (fp, "# --------------------------------------------\n");
  
  while (bom != NULL)
    {
      fprintf (fp,"%d,\"%s\",\"%s\",", 
	       bom->num, 
	       CleanBOMString(bom->descr), 
	       CleanBOMString(bom->value));
      while (bom->refdes != NULL)
	{
	  fprintf (fp,"%s ", bom->refdes->str);
	  g_free (bom->refdes->str);
	  lasts = bom->refdes;
	  bom->refdes = bom->refdes->next;
	  g_free (lasts);
	}
      fprintf (fp, "\n");
      lastb = bom;
      bom = bom->next;
      g_free (lastb);
    }

  fclose (fp);
  
  return (0);
}


static char *CleanBOMString (char *in)
{
  char *out;
  int i;

  if ( (out = g_malloc( (strlen(in) + 1)*sizeof(char) ) ) == NULL ) {
    fprintf(stderr, "Error:  CleanBOMString() g_malloc() failed\n");
    exit (1);
  }

  /* 
   * copy over in to out with some character conversions.
   * Go all the way to then end to get the terminating \0
   */
  for (i = 0; i<=strlen(in); i++) {
    switch (in[i])
      {
      case '"':
	out[i] = '\'';
	break;
      default:
	out[i] = in[i];
      }
  }

  return out;
}


static double xyToAngle(double x, double y)
{
  double theta;

  if( ( x > 0.0) && ( y >= 0.0) )
    theta = 180.0;
  else if( ( x <= 0.0) && ( y > 0.0) )
    theta = 90.0;
  else if( ( x < 0.0) && ( y <= 0.0) )
    theta = 0.0;
  else if( ( x >= 0.0) && ( y < 0.0) )
    theta = 270.0;
  else
    {
      theta = 0.0;
      Message ("xyToAngle(): unable to figure out angle of element\n"
	       "     because the pin is at the centroid of the part.\n"
	       "     This is a BUG!!!\n"
	       "     Setting to %g degrees\n",
	       theta );
    }

  return (theta);
}

BomList *bom_insert (char *refdes, char *descr, char *value, BomList *bom)
{
  BomList *new, *cur, *prev = NULL;

  if (bom == NULL)
    {
      /* this is the first element so automatically create an entry */
      if ( (new = (BomList *) g_malloc (sizeof (BomList) ) ) == NULL )
	{
	  fprintf(stderr, "g_malloc() failed in bom_insert()\n");
	  exit (1);
	}

      new->next = NULL;
      new->descr = g_strdup(descr);
      new->value = g_strdup(value);
      new->num = 1;
      new->refdes = string_insert(refdes, NULL);
      return (new);
    }

  /* search and see if we already have used one of these
     components */
  cur = bom;
  while (cur != NULL)
    {
      if ( (NSTRCMP(descr, cur->descr) == 0) &&
	   (NSTRCMP(value, cur->value) == 0) )
	{
	  cur->num++;
	  cur->refdes = string_insert(refdes, cur->refdes);
	  break;
	}
      prev = cur;
      cur = cur->next;
    }
  
  if (cur == NULL)
    {
      if ( (new = (BomList *) g_malloc (sizeof (BomList) ) ) == NULL )
	{
	  fprintf(stderr, "g_malloc() failed in bom_insert()\n");
	  exit (1);
	}
      
      prev->next = new;
      
      new->next = NULL;
      new->descr = g_strdup(descr);
      new->value = g_strdup(value);
      new->num = 1;
      new->refdes = string_insert(refdes, NULL);
    }

  return (bom);

}

static StringList *string_insert(char *str, StringList *list)
{
  StringList *new, *cur;

  if ( (new = (StringList *) g_malloc (sizeof (StringList) ) ) == NULL )
    {
	  fprintf(stderr, "g_malloc() failed in string_insert()\n");
	  exit (1);
    }
  
  new->next = NULL;
  new->str = g_strdup(str);
  
  if (list == NULL)
    return (new);

  cur = list;
  while ( cur->next != NULL )
    cur = cur->next;
  
  cur->next = new;

  return (list);
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
       LocationType OffsetX, LocationType OffsetY, Boolean SilkscreenTextFlag)
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
  gui_watch_cursor ();
  Device->init (&DeviceFlags);
  /* OK, call all necessary subroutines */
  if (PrintLayergroups () || PrintSilkscreen () ||
      PrintDrill () || PrintMask () || PrintPaste () || 
      PrintFab () || PrintBOM () || PrintAssembly () )
    {
    gui_restore_cursor ();
    return (1);
	}
  Device->Exit ();
  gui_restore_cursor ();
  return (0);
}
