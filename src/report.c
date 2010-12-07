/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
 *
 *  This module, report.c, was written and is Copyright (C) 1997 harry eaton
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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>

#include "report.h"
#include "crosshair.h"
#include "data.h"
#include "drill.h"
#include "error.h"
#include "search.h"
#include "misc.h"
#include "mymem.h"
#include "rtree.h"
#include "strflags.h"
#include "macro.h"
#include "undo.h"
#include "find.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");


#define UNIT1(value) (Settings.grid_units_mm ? ((value) / 100000.0 * 25.4) : ((value) / 100.0))
#define UNIT(value) UNIT1(value) , (Settings.grid_units_mm ? "mm" : "mils")

static int
ReportDrills (int argc, char **argv, int x, int y)
{
  DrillInfoTypePtr AllDrills;
  Cardinal n;
  char *stringlist, *thestring;
  int total_drills = 0;

  AllDrills = GetDrillInfo (PCB->Data);
  RoundDrillInfo (AllDrills, 100);

  for (n = 0; n < AllDrills->DrillN; n++)
    {
      total_drills += AllDrills->Drill[n].PinCount;
      total_drills += AllDrills->Drill[n].ViaCount;
      total_drills += AllDrills->Drill[n].UnplatedCount;
    }

  stringlist = malloc (512L + AllDrills->DrillN * 64L);

  /* Use tabs for formatting since can't count on a fixed font anymore.
     |  And even that probably isn't going to work in all cases.
   */
  sprintf (stringlist,
	   "There are %d different drill sizes used in this layout, %d holes total\n\n"
	   "Drill Diam. (mils)\t# of Pins\t# of Vias\t# of Elements\t# Unplated\n",
	   AllDrills->DrillN, total_drills);
  thestring = stringlist;
  while (*thestring != '\0')
    thestring++;
  for (n = 0; n < AllDrills->DrillN; n++)
    {
      sprintf (thestring,
	       "\t%d\t\t\t%d\t\t%d\t\t%d\t\t%d\n",
	       (AllDrills->Drill[n].DrillSize+50) / 100,
	       AllDrills->Drill[n].PinCount, AllDrills->Drill[n].ViaCount,
	       AllDrills->Drill[n].ElementN,
	       AllDrills->Drill[n].UnplatedCount);
      while (*thestring != '\0')
	thestring++;
    }
  FreeDrillInfo (AllDrills);
  /* create dialog box */
  gui->report_dialog ("Drill Report", stringlist);

  free (stringlist);
  return 0;
}


static const char reportdialog_syntax[] = "ReportDialog()";

static const char reportdialog_help[] =
  "Report on the object under the crosshair";

/* %start-doc actions ReportDialog

This is a shortcut for @code{Report(Object)}.

%end-doc */

static int
ReportDialog (int argc, char **argv, int x, int y)
{
  void *ptr1, *ptr2, *ptr3;
  int type;
  char report[2048];

  type = SearchScreen (x, y, REPORT_TYPES, &ptr1, &ptr2, &ptr3);
  if (type == NO_TYPE)
    type =
      SearchScreen (x, y, REPORT_TYPES | LOCKED_TYPE, &ptr1, &ptr2, &ptr3);

  switch (type)
    {
    case VIA_TYPE:
      {
	PinTypePtr via;
#ifndef NDEBUG
	if (gui->shift_is_pressed ())
	  {
	    __r_dump_tree (PCB->Data->via_tree->root, 0);
	    return 0;
	  }
#endif
	via = (PinTypePtr) ptr2;
	if (TEST_FLAG (HOLEFLAG, via))
	  sprintf (&report[0], "VIA ID# %ld  Flags:%s\n"
		   "(X,Y) = (%d, %d)\n"
		   "It is a pure hole of diameter %0.2f %s\n"
		   "Name = \"%s\""
		   "%s", via->ID, flags_to_string (via->Flags, VIA_TYPE),
		   via->X, via->Y, UNIT (via->DrillingHole),
		   EMPTY (via->Name), TEST_FLAG (LOCKFLAG,
						 via) ? "It is LOCKED\n" :
		   "");
	else
	  sprintf (&report[0], "VIA ID# %ld   Flags:%s\n"
		   "(X,Y) = (%d, %d)\n"
		   "Copper width = %0.2f %s  Drill width = %0.2f %s\n"
		   "Clearance width in polygons = %0.2f %s\n"
		   "Annulus = %0.2f %s\n"
		   "Solder mask hole = %0.2f %s (gap = %0.2f %s)\n"
		   "Name = \"%s\""
		   "%s", via->ID, flags_to_string (via->Flags, VIA_TYPE),
		   via->X, via->Y, UNIT (via->Thickness),
		   UNIT (via->DrillingHole), UNIT (via->Clearance / 2.),
		   UNIT ((via->Thickness - via->DrillingHole)/2),
		   UNIT (via->Mask), UNIT ((via->Mask - via->Thickness)/2),
		   EMPTY (via->Name), TEST_FLAG (LOCKFLAG, via) ?
		   "It is LOCKED\n" : "");
	break;
      }
    case PIN_TYPE:
      {
	PinTypePtr Pin;
	ElementTypePtr element;
#ifndef NDEBUG
	if (gui->shift_is_pressed ())
	  {
	    __r_dump_tree (PCB->Data->pin_tree->root, 0);
	    return 0;
	  }
#endif
	Pin = (PinTypePtr) ptr2;
	element = (ElementTypePtr) ptr1;

	PIN_LOOP (element);
	{
	  if (pin == Pin)
	    break;
	}
	END_LOOP;
	if (TEST_FLAG (HOLEFLAG, Pin))
	  sprintf (&report[0], "PIN ID# %ld  Flags:%s\n"
		   "(X,Y) = (%d, %d)\n"
		   "It is a mounting hole, Drill width = %0.2f %s\n"
		   "It is owned by element %s\n"
		   "%s", Pin->ID, flags_to_string (Pin->Flags, PIN_TYPE),
		   Pin->X, Pin->Y, UNIT (Pin->DrillingHole),
		   EMPTY (element->Name[1].TextString),
		   TEST_FLAG (LOCKFLAG, Pin) ? "It is LOCKED\n" : "");
	else
	  sprintf (&report[0],
		   "PIN ID# %ld   Flags:%s\n" "(X,Y) = (%d, %d)\n"
		   "Copper width = %0.2f %s  Drill width = %0.2f %s\n"
		   "Clearance width to Polygon = %0.2f %s\n"
		   "Annulus = %0.2f %s\n"
		   "Solder mask hole = %0.2f %s (gap = %0.2f %s)\n"
		   "Name = \"%s\"\n"
		   "It is owned by element %s\n" "As pin number %s\n"
		   "%s",
		   Pin->ID, flags_to_string (Pin->Flags, PIN_TYPE),
		   Pin->X, Pin->Y, UNIT (Pin->Thickness),
		   UNIT (Pin->DrillingHole), UNIT (Pin->Clearance / 2.),
		   UNIT ((Pin->Thickness - Pin->DrillingHole)/2),
		   UNIT (Pin->Mask), UNIT ((Pin->Mask - Pin->Thickness)/2),
		   EMPTY (Pin->Name),
		   EMPTY (element->Name[1].TextString), EMPTY (Pin->Number),
		   TEST_FLAG (LOCKFLAG, Pin) ? "It is LOCKED\n" : "");
	break;
      }
    case LINE_TYPE:
      {
	LineTypePtr line;
#ifndef NDEBUG
	if (gui->shift_is_pressed ())
	  {
	    LayerTypePtr layer = (LayerTypePtr) ptr1;
	    __r_dump_tree (layer->line_tree->root, 0);
	    return 0;
	  }
#endif
	line = (LineTypePtr) ptr2;
	sprintf (&report[0], "LINE ID# %ld   Flags:%s\n"
		 "FirstPoint(X,Y) = (%d, %d)  ID = %ld\n"
		 "SecondPoint(X,Y) = (%d, %d)  ID = %ld\n"
		 "Width = %0.2f %s.\nClearance width in polygons = %0.2f %s.\n"
		 "It is on layer %d\n"
		 "and has name %s\n"
		 "%s",
		 line->ID, flags_to_string (line->Flags, LINE_TYPE),
		 line->Point1.X, line->Point1.Y,
		 line->Point1.ID, line->Point2.X, line->Point2.Y,
		 line->Point2.ID, UNIT (line->Thickness),
		 UNIT (line->Clearance / 2.), GetLayerNumber (PCB->Data,
							 (LayerTypePtr) ptr1),
		 UNKNOWN (line->Number), TEST_FLAG (LOCKFLAG,
						    line) ? "It is LOCKED\n" :
		 "");
	break;
      }
    case RATLINE_TYPE:
      {
	RatTypePtr line;
#ifndef NDEBUG
	if (gui->shift_is_pressed ())
	  {
	    __r_dump_tree (PCB->Data->rat_tree->root, 0);
	    return 0;
	  }
#endif
	line = (RatTypePtr) ptr2;
	sprintf (&report[0], "RAT-LINE ID# %ld   Flags:%s\n"
		 "FirstPoint(X,Y) = (%d, %d) ID = %ld "
		 "connects to layer group %d\n"
		 "SecondPoint(X,Y) = (%d, %d) ID = %ld "
		 "connects to layer group %d\n",
		 line->ID, flags_to_string (line->Flags, LINE_TYPE),
		 line->Point1.X, line->Point1.Y,
		 line->Point1.ID, line->group1,
		 line->Point2.X, line->Point2.Y,
		 line->Point2.ID, line->group2);
	break;
      }
    case ARC_TYPE:
      {
	ArcTypePtr Arc;
	BoxTypePtr box;
#ifndef NDEBUG
	if (gui->shift_is_pressed ())
	  {
	    LayerTypePtr layer = (LayerTypePtr) ptr1;
	    __r_dump_tree (layer->arc_tree->root, 0);
	    return 0;
	  }
#endif
	Arc = (ArcTypePtr) ptr2;
	box = GetArcEnds (Arc);

	sprintf (&report[0], "ARC ID# %ld   Flags:%s\n"
		 "CenterPoint(X,Y) = (%d, %d)\n"
		 "Radius = %0.2f %s, Thickness = %0.2f %s\n"
		 "Clearance width in polygons = %0.2f %s\n"
		 "StartAngle = %ld degrees, DeltaAngle = %ld degrees\n"
		 "Bounding Box is (%d,%d), (%d,%d)\n"
		 "That makes the end points at (%d,%d) and (%d,%d)\n"
		 "It is on layer %d\n"
		 "%s", Arc->ID, flags_to_string (Arc->Flags, ARC_TYPE),
		 Arc->X, Arc->Y, UNIT (Arc->Width), UNIT (Arc->Thickness),
		 UNIT (Arc->Clearance / 2.), Arc->StartAngle, Arc->Delta,
		 Arc->BoundingBox.X1, Arc->BoundingBox.Y1,
		 Arc->BoundingBox.X2, Arc->BoundingBox.Y2, box->X1,
		 box->Y1, box->X2, box->Y2, GetLayerNumber (PCB->Data,
							    (LayerTypePtr)
							    ptr1),
		 TEST_FLAG (LOCKFLAG, Arc) ? "It is LOCKED\n" : "");
	break;
      }
    case POLYGON_TYPE:
      {
	PolygonTypePtr Polygon;
#ifndef NDEBUG
	if (gui->shift_is_pressed ())
	  {
	    LayerTypePtr layer = (LayerTypePtr) ptr1;
	    __r_dump_tree (layer->polygon_tree->root, 0);
	    return;
	  }
#endif
	Polygon = (PolygonTypePtr) ptr2;

	sprintf (&report[0], "POLYGON ID# %ld   Flags:%s\n"
		 "Its bounding box is (%d,%d) (%d,%d)\n"
		 "It has %d points and could store %d more\n"
		 "without using more memory.\n"
		 "It has %d holes and resides on layer %d\n"
		 "%s", Polygon->ID,
		 flags_to_string (Polygon->Flags, POLYGON_TYPE),
		 Polygon->BoundingBox.X1, Polygon->BoundingBox.Y1,
		 Polygon->BoundingBox.X2, Polygon->BoundingBox.Y2,
		 Polygon->PointN, Polygon->PointMax - Polygon->PointN,
		 Polygon->HoleIndexN,
		 GetLayerNumber (PCB->Data, (LayerTypePtr) ptr1),
		 TEST_FLAG (LOCKFLAG, Polygon) ? "It is LOCKED\n" : "");
	break;
      }
    case PAD_TYPE:
      {
	int len, dx, dy, mgap;
	PadTypePtr Pad;
	ElementTypePtr element;
#ifndef NDEBUG
	if (gui->shift_is_pressed ())
	  {
	    __r_dump_tree (PCB->Data->pad_tree->root, 0);
	    return 0;
	  }
#endif
	Pad = (PadTypePtr) ptr2;
	element = (ElementTypePtr) ptr1;

	PAD_LOOP (element);
	{
	  {
	    if (pad == Pad)
	      break;
	  }
	}
	END_LOOP;
	dx = Pad->Point1.X - Pad->Point2.X;
	dy = Pad->Point1.Y - Pad->Point2.Y;
	len = sqrt (dx*dx+dy*dy);
	mgap = (Pad->Mask - Pad->Thickness)/2;
	sprintf (&report[0], "PAD ID# %ld   Flags:%s\n"
		 "FirstPoint(X,Y)  = (%d, %d)  ID = %ld\n"
		 "SecondPoint(X,Y) = (%d, %d)  ID = %ld\n"
		 "Width = %0.2f %s.  Length = %0.2f %s.\n"
		 "Clearance width in polygons = %0.2f %s.\n"
		 "Solder mask = %0.2f x %0.2f %s (gap = %0.2f %s).\n"
		 "Name = \"%s\"\n"
		 "It is owned by SMD element %s\n"
		 "As pin number %s and is on the %s\n"
		 "side of the board.\n"
		 "%s", Pad->ID,
		 flags_to_string (Pad->Flags, PAD_TYPE),
		 Pad->Point1.X, Pad->Point1.Y, Pad->Point1.ID,
		 Pad->Point2.X, Pad->Point2.Y, Pad->Point2.ID,
		 UNIT (Pad->Thickness), UNIT (len + Pad->Thickness),
		 UNIT (Pad->Clearance / 2.),
		 UNIT1 (Pad->Mask), UNIT (Pad->Mask + len), UNIT (mgap),
		 EMPTY (Pad->Name),
		 EMPTY (element->Name[1].TextString),
		 EMPTY (Pad->Number),
		 TEST_FLAG (ONSOLDERFLAG,
			    Pad) ? "solder (bottom)" : "component",
		 TEST_FLAG (LOCKFLAG, Pad) ? "It is LOCKED\n" : "");
	break;
      }
    case ELEMENT_TYPE:
      {
	ElementTypePtr element;
#ifndef NDEBUG
	if (gui->shift_is_pressed ())
	  {
	    __r_dump_tree (PCB->Data->element_tree->root, 0);
	    return 0;
	  }
#endif
	element = (ElementTypePtr) ptr2;
	sprintf (&report[0], "ELEMENT ID# %ld   Flags:%s\n"
		 "BoundingBox (%d,%d) (%d,%d)\n"
		 "Descriptive Name \"%s\"\n"
		 "Name on board \"%s\"\n"
		 "Part number name \"%s\"\n"
		 "It is %0.2f %s tall and is located at (X,Y) = (%d,%d)\n"
		 "%s"
		 "Mark located at point (X,Y) = (%d,%d)\n"
		 "It is on the %s side of the board.\n"
		 "%s",
		 element->ID, flags_to_string (element->Flags, ELEMENT_TYPE),
		 element->BoundingBox.X1, element->BoundingBox.Y1,
		 element->BoundingBox.X2, element->BoundingBox.Y2,
		 EMPTY (element->Name[0].TextString),
		 EMPTY (element->Name[1].TextString),
		 EMPTY (element->Name[2].TextString),
		 UNIT (0.45 * element->Name[1].Scale * 100.), element->Name[1].X,
		 element->Name[1].Y, TEST_FLAG (HIDENAMEFLAG, element) ?
		 "But it's hidden\n" : "", element->MarkX,
		 element->MarkY, TEST_FLAG (ONSOLDERFLAG,
					    element) ? "solder (bottom)" :
		 "component", TEST_FLAG (LOCKFLAG, element) ?
		 "It is LOCKED\n" : "");
	break;
      }
    case TEXT_TYPE:
#ifndef NDEBUG
      if (gui->shift_is_pressed ())
	{
	  LayerTypePtr layer = (LayerTypePtr) ptr1;
	  __r_dump_tree (layer->text_tree->root, 0);
	  return 0;
	}
#endif
    case ELEMENTNAME_TYPE:
      {
	char laynum[32];
	TextTypePtr text;
#ifndef NDEBUG
	if (gui->shift_is_pressed ())
	  {
	    __r_dump_tree (PCB->Data->name_tree[NAME_INDEX (PCB)]->root, 0);
	    return 0;
	  }
#endif
	text = (TextTypePtr) ptr2;

	if (type == TEXT_TYPE)
	  sprintf (laynum, "is on layer %d",
		   GetLayerNumber (PCB->Data, (LayerTypePtr) ptr1));
	sprintf (&report[0], "TEXT ID# %ld   Flags:%s\n"
		 "Located at (X,Y) = (%d,%d)\n"
		 "Characters are %0.2f %s tall\n"
		 "Value is \"%s\"\n"
		 "Direction is %d\n"
		 "The bounding box is (%d,%d) (%d, %d)\n"
		 "It %s\n"
		 "%s", text->ID, flags_to_string (text->Flags, TEXT_TYPE),
		 text->X, text->Y, UNIT (0.45 * text->Scale * 100.),
		 text->TextString, text->Direction,
		 text->BoundingBox.X1, text->BoundingBox.Y1,
		 text->BoundingBox.X2, text->BoundingBox.Y2,
		 (type == TEXT_TYPE) ? laynum : "is an element name.",
		 TEST_FLAG (LOCKFLAG, text) ? "It is LOCKED\n" : "");
	break;
      }
    case LINEPOINT_TYPE:
    case POLYGONPOINT_TYPE:
      {
	PointTypePtr point = (PointTypePtr) ptr2;
	sprintf (&report[0], "POINT ID# %ld. Points don't have flags.\n"
		 "Located at (X,Y) = (%d,%d)\n"
		 "It belongs to a %s on layer %d\n", point->ID,
		 point->X, point->Y,
		 (type == LINEPOINT_TYPE) ? "line" : "polygon",
		 GetLayerNumber (PCB->Data, (LayerTypePtr) ptr1));
	break;
      }
    case NO_TYPE:
      report[0] = '\0';
      break;

    default:
      sprintf (&report[0], "Unknown\n");
      break;
    }

  if (report[0] == '\0')
    {
      Message (_("Nothing found to report on\n"));
      return 1;
    }
  HideCrosshair (false);
  /* create dialog box */
  gui->report_dialog ("Report", &report[0]);

  RestoreCrosshair (false);
  return 0;
}

static int
ReportFoundPins (int argc, char **argv, int x, int y)
{
  static DynamicStringType list;
  char temp[64];
  int col = 0;

  DSClearString (&list);
  DSAddString (&list, "The following pins/pads are FOUND:\n");
  ELEMENT_LOOP (PCB->Data);
  {
    PIN_LOOP (element);
    {
      if (TEST_FLAG (FOUNDFLAG, pin))
	{
	  sprintf (temp, "%s-%s,%c",
		   NAMEONPCB_NAME (element),
		   pin->Number,
		   ((col++ % (COLUMNS + 1)) == COLUMNS) ? '\n' : ' ');
	  DSAddString (&list, temp);
	}
    }
    END_LOOP;
    PAD_LOOP (element);
    {
      if (TEST_FLAG (FOUNDFLAG, pad))
	{
	  sprintf (temp, "%s-%s,%c",
		   NAMEONPCB_NAME (element), pad->Number,
		   ((col++ % (COLUMNS + 1)) == COLUMNS) ? '\n' : ' ');
	  DSAddString (&list, temp);
	}
    }
    END_LOOP;
  }
  END_LOOP;

  HideCrosshair (false);
  gui->report_dialog ("Report", list.Data);
  RestoreCrosshair (false);
  return 0;
}

static double
XYtoNetLength (int x, int y, int *found)
{
  double length;

  length = 0;
  *found = 0;
  LookupConnection (x, y, true, PCB->Grid, FOUNDFLAG);

  ALLLINE_LOOP (PCB->Data);
  {
    if (TEST_FLAG (FOUNDFLAG, line))
      {
	double l;
	int dx, dy;
	dx = line->Point1.X - line->Point2.X;
	dy = line->Point1.Y - line->Point2.Y;
	l = sqrt ((double)dx*dx + (double)dy*dy);
	length += l;
	*found = 1;
      }
  }
  ENDALL_LOOP;

  ALLARC_LOOP (PCB->Data);
  {
    if (TEST_FLAG (FOUNDFLAG, arc))
      {
	double l;
	/* FIXME: we assume width==height here */
	l = M_PI * 2*arc->Width * abs(arc->Delta)/360.0;
	length += l;
	*found = 1;
      }
  }
  ENDALL_LOOP;

  return length;
}

static int
ReportAllNetLengths (int argc, char **argv, int x, int y)
{
  enum { Upcb, Umm, Umil, Uin } units;
  int ni;
  int found;
  double length;

  units = Settings.grid_units_mm ? Umm : Umil;

  if (argc >= 1)
    {
      printf("Units: %s\n", argv[0]);
      if (strcasecmp (argv[0], "mm") == 0)
	units = Umm;
      else if (strcasecmp (argv[0], "mil") == 0)
	units = Umil;
      else if (strcasecmp (argv[0], "in") == 0)
	units = Uin;
      else
	units = Upcb;
    }

  for (ni = 0; ni < PCB->NetlistLib.MenuN; ni++)
    {
      char *netname = PCB->NetlistLib.Menu[ni].Name + 2;
      char *ename = PCB->NetlistLib.Menu[ni].Entry[0].ListEntry;
      char *pname;

      ename = strdup (ename);
      pname = strchr (ename, '-');
      if (! pname)
	{
	  free (ename);
	  continue;
	}
      *pname++ = 0;

      ELEMENT_LOOP (PCB->Data);
      {
	char *es = element->Name[NAMEONPCB_INDEX].TextString;
	if (es && strcmp (es, ename) == 0)
	  {
	    PIN_LOOP (element);
	    {
	      if (strcmp (pin->Number, pname) == 0)
		{
		  x = pin->X;
		  y = pin->Y;
		  goto got_one;
		}
	    }
	    END_LOOP;
	    PAD_LOOP (element);
	    {
	      if (strcmp (pad->Number, pname) == 0)
		{
		  x = (pad->Point1.X + pad->Point2.X) / 2;
		  y = (pad->Point1.Y + pad->Point2.Y) / 2;
		  goto got_one;
		}
	    }
	    END_LOOP;
	  }
      }
      END_LOOP;

      continue;

    got_one:
      SaveUndoSerialNumber ();
      ResetFoundPinsViasAndPads (true);
      RestoreUndoSerialNumber ();
      ResetFoundLinesAndPolygons (true);
      RestoreUndoSerialNumber ();
      length = XYtoNetLength (x, y, &found);

      switch (units)
	{
	case Upcb:
	  gui->log("Net %s length %d\n", netname, (int)length);
	  break;
	case Umm:
	  length *= COOR_TO_MM;
	  gui->log("Net %s length %.2f mm\n", netname, length);
	  break;
	case Umil:
	  length /= 100;
	  gui->log("Net %s length %d mil\n", netname, (int)length);
	  break;
	case Uin:
	  length /= 100000.0;
	  gui->log("Net %s length %.3f in\n", netname, length);
	  break;
	}
    }
  return 0;
}

static int
ReportNetLength (int argc, char **argv, int x, int y)
{
  double length = 0;
  char *netname = 0;
  int found = 0;

  SaveUndoSerialNumber ();
  ResetFoundPinsViasAndPads (true);
  RestoreUndoSerialNumber ();
  ResetFoundLinesAndPolygons (true);
  RestoreUndoSerialNumber ();
  gui->get_coords ("Click on a connection", &x, &y);

  length = XYtoNetLength (x, y, &found);

  if (!found)
    {
      gui->log ("No net under cursor.\n");
      return 1;
    }

  ELEMENT_LOOP (PCB->Data);
  {
    PIN_LOOP (element);
    {
      if (TEST_FLAG (FOUNDFLAG, pin))
	{
	  int ni, nei;
	  char *ename = element->Name[NAMEONPCB_INDEX].TextString;
	  char *pname = pin->Number;
	  char *n;

	  if (ename && pname)
	    {
	      n = Concat (ename, "-", pname, NULL);
	      for (ni = 0; ni < PCB->NetlistLib.MenuN; ni++)
		for (nei = 0; nei < PCB->NetlistLib.Menu[ni].EntryN; nei++)
		  {
		    if (strcmp (PCB->NetlistLib.Menu[ni].Entry[nei].ListEntry, n) == 0)
		      {
			netname = PCB->NetlistLib.Menu[ni].Name + 2;
			goto got_net_name; /* four for loops deep */
		      }
		  }
	    }
	}
    }
    END_LOOP;
    PAD_LOOP (element);
    {
      if (TEST_FLAG (FOUNDFLAG, pad))
	{
	  int ni, nei;
	  char *ename = element->Name[NAMEONPCB_INDEX].TextString;
	  char *pname = pad->Number;
	  char *n;

	  if (ename && pname)
	    {
	      n = Concat (ename, "-", pname, NULL);
	      for (ni = 0; ni < PCB->NetlistLib.MenuN; ni++)
		for (nei = 0; nei < PCB->NetlistLib.Menu[ni].EntryN; nei++)
		  {
		    if (strcmp (PCB->NetlistLib.Menu[ni].Entry[nei].ListEntry, n) == 0)
		      {
			netname = PCB->NetlistLib.Menu[ni].Name + 2;
			goto got_net_name; /* four for loops deep */
		      }
		  }
	    }
	}
    }
    END_LOOP;
  }
  END_LOOP;
 got_net_name:

  HideCrosshair (false);
  if (netname)
    gui->log ("Net %s length: %0.2f %s\n", netname, UNIT (length));
  else
    gui->log ("Net length: %0.2f %s\n", UNIT (length));
  RestoreCrosshair (false);
  return 0;
}
/* ---------------------------------------------------------------------------
 * reports on an object 
 * syntax: 
 */

static const char report_syntax[] = "Report(Object|DrillReport|FoundPins|NetLength|AllNetLengths)";

static const char report_help[] = "Produce various report.";

/* %start-doc actions Report

@table @code

@item Object
The object under the crosshair will be reported, describing various
aspects of the object.

@item DrillReport
A report summarizing the number of drill sizes used, and how many of
each, will be produced.

@item FoundPins
A report listing all pins and pads which are marked as ``found'' will
be produced.

@item NetLength
The name and length of the net under the crosshair will be reported to
the message log.

@item AllNetLengths
The name and length of the net under the crosshair will be reported to
the message log.  An optional parameter specifies mm, mil, pcb, or in
units

@end table

%end-doc */

static int
Report (int argc, char **argv, int x, int y)
{
  if (argc < 1)
    AUSAGE (report);
  else if (strcasecmp (argv[0], "Object") == 0)
    {
      gui->get_coords ("Click on an object", &x, &y);
      return ReportDialog (argc - 1, argv + 1, x, y);
    }
  else if (strcasecmp (argv[0], "DrillReport") == 0)
    return ReportDrills (argc - 1, argv + 1, x, y);
  else if (strcasecmp (argv[0], "FoundPins") == 0)
    return ReportFoundPins (argc - 1, argv + 1, x, y);
  else if (strcasecmp (argv[0], "NetLength") == 0)
    return ReportNetLength (argc - 1, argv + 1, x, y);
  else if (strcasecmp (argv[0], "AllNetLengths") == 0)
    return ReportAllNetLengths (argc - 1, argv + 1, x, y);
  else
    AFAIL (report);
  return 1;
}

HID_Action report_action_list[] = {
  {"ReportObject", "Click on an object", ReportDialog,
   reportdialog_help, reportdialog_syntax}
  ,
  {"Report", 0, Report,
   report_help, report_syntax}
};

REGISTER_ACTIONS (report_action_list)
