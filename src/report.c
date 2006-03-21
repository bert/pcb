/* $Id$ */

#include "rtree.h"
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

#include "report.h"
#include "crosshair.h"
#include "data.h"
#include "drill.h"
#include "error.h"
#include "search.h"
#include "misc.h"
#include "mymem.h"
#include "strflags.h"

RCSID("$Id$");



static int
ReportDrills (int argc, char **argv, int x, int y)
{
  DrillInfoTypePtr AllDrills;
  Cardinal n;
  char *stringlist, *thestring;
  int total_drills = 0;

  AllDrills = GetDrillInfo (PCB->Data);

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
	       "\t%d\t\t\t%d\t\t%d\t\t%td\t\t\t%d\n",
	       AllDrills->Drill[n].DrillSize / 100,
	       AllDrills->Drill[n].PinCount, AllDrills->Drill[n].ViaCount,
	       AllDrills->Drill[n].ElementN,
	       AllDrills->Drill[n].UnplatedCount);
      while (*thestring != '\0')
	thestring++;
    }
  FreeDrillInfo (AllDrills);
  /* create dialog box */
  gui->report_dialog("Drill Report", stringlist);

  SaveFree (stringlist);
  return 0;
}

static int
ReportDialog (int argc, char **argv, int x, int y)
{
  void *ptr1, *ptr2, *ptr3;
  int type;
  char report[2048];

  switch (type = SearchScreen (x, y,
			       REPORT_TYPES, &ptr1, &ptr2, &ptr3))
    {
    case VIA_TYPE:
      {
#ifdef FIXME
#ifndef NDEBUG
	if (gui_shift_is_pressed())
	  {
	    __r_dump_tree (PCB->Data->via_tree->root, 0);
	    return 0;
	  }
#endif
#endif
	PinTypePtr via = (PinTypePtr) ptr2;
	if (TEST_FLAG (HOLEFLAG, via))
	  sprintf (&report[0], "VIA ID# %ld  Flags:%s\n"
		   "(X,Y) = (%d, %d)\n"
		   "It is a pure hole of diameter %0.2f mils\n"
		   "Name = \"%s\""
		   "%s", via->ID, flags_to_string (via->Flags, VIA_TYPE), via->X,
		   via->Y, via->DrillingHole / 100.0, EMPTY (via->Name),
		   TEST_FLAG (LOCKFLAG, via) ? "It is LOCKED\n" : "");
	else
	  sprintf (&report[0], "VIA ID# %ld   Flags:%s\n"
		   "(X,Y) = (%d, %d)\n"
		   "Copper width = %0.2f mils  Drill width = %0.2f mils\n"
		   "Clearance width in polygons = %0.2f mils\n"
		   "Solder mask hole = %0.2f mils\n"
		   "Name = \"%s\""
		   "%s", via->ID, flags_to_string (via->Flags, VIA_TYPE), via->X,
		   via->Y, via->Thickness / 100., via->DrillingHole / 100.,
		   via->Clearance / 200., via->Mask / 100.,
		   EMPTY (via->Name), TEST_FLAG (LOCKFLAG, via) ?
		   "It is LOCKED\n" : "");
	break;
      }
    case PIN_TYPE:
      {
#ifdef FIXME
#ifndef NDEBUG
	if (gui_shift_is_pressed())
	  {
	    __r_dump_tree (PCB->Data->pin_tree->root, 0);
	    return 0;
	  }
#endif
#endif
	PinTypePtr Pin = (PinTypePtr) ptr2;
	ElementTypePtr element = (ElementTypePtr) ptr1;

	PIN_LOOP (element);
	{
	  if (pin == Pin)
	    break;
	}
	END_LOOP;
	if (TEST_FLAG (HOLEFLAG, Pin))
	  sprintf (&report[0], "PIN ID# %ld  Flags:%s\n"
		   "(X,Y) = (%d, %d)\n"
		   "It is a mounting hole, Drill width = %0.2f mils\n"
		   "It is owned by element %s\n"
		   "%s", Pin->ID, flags_to_string (Pin->Flags, PIN_TYPE),
		   Pin->X, Pin->Y, Pin->DrillingHole / 100.,
		   EMPTY (element->Name[1].TextString),
		   TEST_FLAG (LOCKFLAG, Pin) ? "It is LOCKED\n" : "");
	else
	  sprintf (&report[0],
		   "PIN ID# %ld   Flags:%s\n" "(X,Y) = (%d, %d)\n"
		   "Copper width = %0.2f mils  Drill width = %0.2f mils\n"
		   "Clearance width to Polygon = %0.2f mils\n"
		   "Solder mask hole = %0.2f mils\n" "Name = \"%s\"\n"
		   "It is owned by element %s\n" "As pin number %s\n"
		   "%s",
		   Pin->ID, flags_to_string (Pin->Flags, PIN_TYPE),
		   Pin->X, Pin->Y, Pin->Thickness / 100.,
		   Pin->DrillingHole / 100., Pin->Clearance / 200.,
		   Pin->Mask / 100., EMPTY (Pin->Name),
		   EMPTY (element->Name[1].TextString), EMPTY (Pin->Number),
		   TEST_FLAG (LOCKFLAG, Pin) ? "It is LOCKED\n" : "");
	break;
      }
    case LINE_TYPE:
      {
#ifdef FIXME
#ifndef NDEBUG
	if (gui_shift_is_pressed())
	  {
	    LayerTypePtr layer = (LayerTypePtr) ptr1;
	    __r_dump_tree (layer->line_tree->root, 0);
	    return 0;
	  }
#endif
#endif
	LineTypePtr line = (LineTypePtr) ptr2;
	sprintf (&report[0], "LINE ID# %ld   Flags:%s\n"
		 "FirstPoint(X,Y) = (%d, %d)  ID = %ld\n"
		 "SecondPoint(X,Y) = (%d, %d)  ID = %ld\n"
		 "Width = %0.2f mils.\nClearance width in polygons = %0.2f mils.\n"
		 "It is on layer %d\n"
		 "and has name %s\n"
		 "%s",
		 line->ID, flags_to_string (line->Flags, LINE_TYPE),
		 line->Point1.X, line->Point1.Y,
		 line->Point1.ID, line->Point2.X, line->Point2.Y,
		 line->Point2.ID, line->Thickness / 100.,
		 line->Clearance / 200., GetLayerNumber (PCB->Data,
							 (LayerTypePtr) ptr1),
		 UNKNOWN (line->Number), TEST_FLAG (LOCKFLAG,
						    line) ? "It is LOCKED\n" :
		 "");
	break;
      }
    case RATLINE_TYPE:
      {
#ifdef FIXME
#ifndef NDEBUG
	if (gui_shift_is_pressed())
	  {
	    __r_dump_tree (PCB->Data->rat_tree->root, 0);
	    return 0;
	  }
#endif
#endif
	RatTypePtr line = (RatTypePtr) ptr2;
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
#ifdef FIXME
#ifndef NDEBUG
	if (gui_shift_is_pressed())
	  {
	    LayerTypePtr layer = (LayerTypePtr) ptr1;
	    __r_dump_tree (layer->arc_tree->root, 0);
	    return 0;
	  }
#endif
#endif
	ArcTypePtr Arc = (ArcTypePtr) ptr2;
	BoxTypePtr box = GetArcEnds (Arc);

	sprintf (&report[0], "ARC ID# %ld   Flags:%s\n"
		 "CenterPoint(X,Y) = (%d, %d)\n"
		 "Radius = %0.2f mils, Thickness = %0.2f mils\n"
		 "Clearance width in polygons = %0.2f mils\n"
		 "StartAngle = %ld degrees, DeltaAngle = %ld degrees\n"
		 "Bounding Box is (%d,%d), (%d,%d)\n"
		 "That makes the end points at (%d,%d) and (%d,%d)\n"
		 "It is on layer %d\n"
		 "%s", Arc->ID, flags_to_string (Arc->Flags, ARC_TYPE),
		 Arc->X, Arc->Y, Arc->Width / 100., Arc->Thickness / 100.,
		 Arc->Clearance / 200., Arc->StartAngle, Arc->Delta,
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
#ifndef NDEBUG
	if (gui_shift_is_pressed ())
	  {
	    LayerTypePtr layer = (LayerTypePtr) ptr1;
	    __r_dump_tree (layer->polygon_tree->root, 0);
	    return;
	  }
#endif
	PolygonTypePtr Polygon = (PolygonTypePtr) ptr2;

	sprintf (&report[0], "POLYGON ID# %ld   Flags:%s\n"
		 "Its bounding box is (%d,%d) (%d,%d)\n"
		 "It has %d points and could store %d more\n"
		 "without using more memory.\n"
		 "It resides on layer %d\n"
		 "%s", Polygon->ID,
		 flags_to_string (Polygon->Flags, POLYGON_TYPE), Polygon->BoundingBox.X1,
		 Polygon->BoundingBox.Y1, Polygon->BoundingBox.X2,
		 Polygon->BoundingBox.Y2, Polygon->PointN,
		 Polygon->PointMax - Polygon->PointN,
		 GetLayerNumber (PCB->Data, (LayerTypePtr) ptr1),
		 TEST_FLAG (LOCKFLAG, Polygon) ? "It is LOCKED\n" : "");
	break;
      }
    case PAD_TYPE:
      {
#ifdef FIXME
#ifndef NDEBUG
	if (gui_shift_is_pressed())
	  {
	    __r_dump_tree (PCB->Data->pad_tree->root, 0);
	    return 0;
	  }
#endif
#endif
	PadTypePtr Pad = (PadTypePtr) ptr2;
	ElementTypePtr element = (ElementTypePtr) ptr1;

	PAD_LOOP (element);
	{
	  {
	    if (pad == Pad)
	      break;
	  }
	}
	END_LOOP;
	sprintf (&report[0], "PAD ID# %ld   Flags:%s\n"
		 "FirstPoint(X,Y) = (%d, %d)  ID = %ld\n"
		 "SecondPoint(X,Y) = (%d, %d)  ID = %ld\n"
		 "Width = %0.2f mils.\nClearance width in polygons = %0.2f mils.\n"
		 "Solder mask width = %0.2f mils\n"
		 "Name = \"%s\"\n"
		 "It is owned by SMD element %s\n"
		 "As pin number %s and is on the %s\n"
		 "side of the board.\n"
		 "%s", Pad->ID,
		 flags_to_string (Pad->Flags, PAD_TYPE), Pad->Point1.X,
		 Pad->Point1.Y, Pad->Point1.ID, Pad->Point2.X, Pad->Point2.Y,
		 Pad->Point2.ID, Pad->Thickness / 100., Pad->Clearance / 200.,
		 Pad->Mask / 100., EMPTY (Pad->Name),
		 EMPTY (element->Name[1].TextString), EMPTY (Pad->Number),
		 TEST_FLAG (ONSOLDERFLAG,
			    Pad) ? "solder (bottom)" : "component",
		 TEST_FLAG (LOCKFLAG, Pad) ? "It is LOCKED\n" : "");
	break;
      }
    case ELEMENT_TYPE:
      {
#ifdef FIXME
#ifndef NDEBUG
	if (gui_shift_is_pressed())
	  {
	    __r_dump_tree (PCB->Data->element_tree->root, 0);
	    return 0;
	  }
#endif
#endif
	ElementTypePtr element = (ElementTypePtr) ptr2;
	sprintf (&report[0], "ELEMENT ID# %ld   Flags:%s\n"
		 "BoundingBox (%d,%d) (%d,%d)\n"
		 "Descriptive Name \"%s\"\n"
		 "Name on board \"%s\"\n"
		 "Part number name \"%s\"\n"
		 "It is %0.2f mils tall and is located at (X,Y) = (%d,%d)\n"
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
		 0.45 * element->Name[1].Scale, element->Name[1].X,
		 element->Name[1].Y, TEST_FLAG (HIDENAMEFLAG, element) ?
		 "But it's hidden\n" : "", element->MarkX,
		 element->MarkY, TEST_FLAG (ONSOLDERFLAG,
					    element) ? "solder (bottom)" :
		 "component", TEST_FLAG (LOCKFLAG, element) ?
		 "It is LOCKED\n" : "");
	break;
      }
    case TEXT_TYPE:
#ifdef FIXME
#ifndef NDEBUG
      if (gui_shift_is_pressed())
	{
	  LayerTypePtr layer = (LayerTypePtr) ptr1;
	  __r_dump_tree (layer->text_tree->root, 0);
	  return 0;
	}
#endif
#endif
    case ELEMENTNAME_TYPE:
      {
#ifdef FIXME
#ifndef NDEBUG
	if (gui_shift_is_pressed())
	  {
	    __r_dump_tree (PCB->Data->name_tree[NAME_INDEX (PCB)]->root, 0);
	    return 0;
	  }
#endif
#endif
	char laynum[32];
	TextTypePtr text = (TextTypePtr) ptr2;

	if (type == TEXT_TYPE)
	  sprintf (laynum, "is on layer %d",
		   GetLayerNumber (PCB->Data, (LayerTypePtr) ptr1));
	sprintf (&report[0], "TEXT ID# %ld   Flags:%s\n"
		 "Located at (X,Y) = (%d,%d)\n"
		 "Characters are %0.2f mils tall\n"
		 "Value is \"%s\"\n"
		 "Direction is %d\n"
		 "The bounding box is (%d,%d) (%d, %d)\n"
		 "It %s\n"
		 "%s", text->ID, flags_to_string (text->Flags, TEXT_TYPE),
		 text->X, text->Y, 0.45 * text->Scale,
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
  HideCrosshair (False);
  /* create dialog box */
  gui->report_dialog("Report", &report[0]);

  RestoreCrosshair (False);
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

  HideCrosshair (False);
  gui->report_dialog("Report", list.Data);
  RestoreCrosshair (False);
  return 0;
}

/* ---------------------------------------------------------------------------
 * reports on an object 
 * syntax: Report(Object|DrillReport|FoundPins)
 */
static int
Report (int argc, char **argv, int x, int y)
{
  if (argc != 1)
    Message ("Usage: Report(Object|DrillReport|FoundPins)\n");
  else if (strcasecmp(argv[0], "Object") == 0)
    {
      gui->get_coords("Click on an object", &x, &y);
      return ReportDialog (argc-1, argv+1, x, y);
    }
  else if (strcasecmp(argv[0], "DrillReport") == 0)
    return ReportDrills (argc-1, argv+1, x, y);
  else if (strcasecmp(argv[0], "FoundPins") == 0)
    return ReportFoundPins (argc-1, argv+1, x, y);
  else
    Message ("Usage: Report(Object|DrillReport|FoundPins)\n");
  return 1;
}

HID_Action report_action_list[] = {
  { "ReportObject", 1, "Click on an object", ReportDialog },
  { "Report", 0, 0, Report }
};
REGISTER_ACTIONS(report_action_list)

