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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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
#include "rats.h"
#include "rtree.h"
#include "strflags.h"
#include "macro.h"
#include "undo.h"
#include "find.h"
#include "draw.h"
#include "pcb-printf.h"
#ifdef HAVE_REGEX_H
#include <regex.h>
#endif

#ifdef HAVE_REGCOMP
#undef HAVE_RE_COMP
#endif

#if defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP)
#define USE_RE
#endif

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define USER_UNITMASK (Settings.grid_unit->allow)

static int
ReportDrills (int argc, char **argv, Coord x, Coord y)
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

  stringlist = (char *)malloc (512L + AllDrills->DrillN * 64L);

  /* Use tabs for formatting since can't count on a fixed font anymore.
     |  And even that probably isn't going to work in all cases.
   */
  sprintf (stringlist,
	   "There are %d different drill sizes used in this layout, %d holes total\n\n"
	   "Drill Diam. (%s)\t# of Pins\t# of Vias\t# of Elements\t# Unplated\n",
	   AllDrills->DrillN, total_drills, Settings.grid_unit->suffix);
  thestring = stringlist;
  while (*thestring != '\0')
    thestring++;
  for (n = 0; n < AllDrills->DrillN; n++)
    {
      pcb_sprintf (thestring,
	       "%10m*\t\t%d\t\t%d\t\t%d\t\t%d\n",
	       Settings.grid_unit->suffix,
	       AllDrills->Drill[n].DrillSize,
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
ReportDialog (int argc, char **argv, Coord x, Coord y)
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
	  pcb_sprintf (&report[0], "%m+VIA ID# %ld; Flags:%s\n"
		   "(X,Y) = %$mD.\n"
		   "It is a pure hole of diameter %$mS.\n"
		   "Name = \"%s\"."
		   "%s", USER_UNITMASK, via->ID, flags_to_string (via->Flags, VIA_TYPE),
		   via->X, via->Y, via->DrillingHole, EMPTY (via->Name),
		   TEST_FLAG (LOCKFLAG, via) ? "It is LOCKED.\n" : "");
	else
	  pcb_sprintf (&report[0], "%m+VIA ID# %ld;  Flags:%s\n"
		   "(X,Y) = %$mD.\n"
		   "Copper width = %$mS. Drill width = %$mS.\n"
		   "Clearance width in polygons = %$mS.\n"
		   "Annulus = %$mS.\n"
		   "Solder mask hole = %$mS (gap = %$mS).\n"
		   "Name = \"%s\"."
		   "%s", USER_UNITMASK, via->ID, flags_to_string (via->Flags, VIA_TYPE),
		   via->X, via->Y,
		   via->Thickness,
		   via->DrillingHole,
		   via->Clearance / 2,
		   (via->Thickness - via->DrillingHole) / 2,
		   via->Mask,
		   (via->Mask - via->Thickness) / 2,
		   EMPTY (via->Name), TEST_FLAG (LOCKFLAG, via) ?
		   "It is LOCKED.\n" : "");
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
	  pcb_sprintf (&report[0], "%m+PIN ID# %ld; Flags:%s\n"
		   "(X,Y) = %$mD.\n"
		   "It is a mounting hole. Drill width = %$mS.\n"
		   "It is owned by element %$mS.\n"
		   "%s", USER_UNITMASK, Pin->ID, flags_to_string (Pin->Flags, PIN_TYPE),
		   Pin->X, Pin->Y, Pin->DrillingHole,
		   EMPTY (element->Name[1].TextString),
		   TEST_FLAG (LOCKFLAG, Pin) ? "It is LOCKED.\n" : "");
	else
	  pcb_sprintf (&report[0],
		   "%m+PIN ID# %ld;  Flags:%s\n" "(X,Y) = %$mD.\n"
		   "Copper width = %$mS. Drill width = %$mS.\n"
		   "Clearance width to Polygon = %$mS.\n"
		   "Annulus = %$mS.\n"
		   "Solder mask hole = %$mS (gap = %$mS).\n"
		   "Name = \"%s\".\n"
		   "It is owned by element %s\n as pin number %s.\n"
		   "%s", USER_UNITMASK,
		   Pin->ID, flags_to_string (Pin->Flags, PIN_TYPE),
		   Pin->X, Pin->Y, Pin->Thickness,
		   Pin->DrillingHole,
		   Pin->Clearance / 2,
		   (Pin->Thickness - Pin->DrillingHole) / 2,
		   Pin->Mask,
		   (Pin->Mask - Pin->Thickness) / 2,
		   EMPTY (Pin->Name),
		   EMPTY (element->Name[1].TextString), EMPTY (Pin->Number),
		   TEST_FLAG (LOCKFLAG, Pin) ? "It is LOCKED.\n" : "");
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
	pcb_sprintf (&report[0], "%m+LINE ID# %ld;  Flags:%s\n"
		 "FirstPoint(X,Y)  = %$mD, ID = %ld.\n"
		 "SecondPoint(X,Y) = %$mD, ID = %ld.\n"
		 "Width = %$mS.\nClearance width in polygons = %$mS.\n"
		 "It is on layer %d\n"
		 "and has name \"%s\".\n"
		 "%s", USER_UNITMASK,
		 line->ID, flags_to_string (line->Flags, LINE_TYPE),
		 line->Point1.X, line->Point1.Y, line->Point1.ID,
		 line->Point2.X, line->Point2.Y, line->Point2.ID,
		 line->Thickness, line->Clearance / 2,
		 GetLayerNumber (PCB->Data, (LayerTypePtr) ptr1),
		 UNKNOWN (line->Number),
		 TEST_FLAG (LOCKFLAG, line) ? "It is LOCKED.\n" : "");
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
	pcb_sprintf (&report[0], "%m+RAT-LINE ID# %ld;  Flags:%s\n"
		 "FirstPoint(X,Y)  = %$mD; ID = %ld; "
		 "connects to layer group %d.\n"
		 "SecondPoint(X,Y) = %$mD; ID = %ld; "
		 "connects to layer group %d.\n",
		 USER_UNITMASK, line->ID, flags_to_string (line->Flags, LINE_TYPE),
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

	pcb_sprintf (&report[0], "%m+ARC ID# %ld;  Flags:%s\n"
		 "CenterPoint(X,Y) = %$mD.\n"
		 "Radius = %$mS, Thickness = %$mS.\n"
		 "Clearance width in polygons = %$mS.\n"
		 "StartAngle = %ma degrees, DeltaAngle = %ma degrees.\n"
		 "Bounding Box is %$mD, %$mD.\n"
		 "That makes the end points at %$mD and %$mD.\n"
		 "It is on layer %d.\n"
		 "%s", USER_UNITMASK, Arc->ID, flags_to_string (Arc->Flags, ARC_TYPE),
		 Arc->X, Arc->Y,
		 Arc->Width, Arc->Thickness,
		 Arc->Clearance / 2, Arc->StartAngle, Arc->Delta,
		 Arc->BoundingBox.X1, Arc->BoundingBox.Y1,
		 Arc->BoundingBox.X2, Arc->BoundingBox.Y2,
		 box->X1, box->Y1,
		 box->X2, box->Y2,
		 GetLayerNumber (PCB->Data, (LayerTypePtr) ptr1),
		 TEST_FLAG (LOCKFLAG, Arc) ? "It is LOCKED.\n" : "");
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
	    return 0;
	  }
#endif
	Polygon = (PolygonTypePtr) ptr2;

	pcb_sprintf (&report[0], "%m+POLYGON ID# %ld;  Flags:%s\n"
		 "Its bounding box is %$mD %$mD.\n"
		 "It has %d points and could store %d more\n"
		 "  without using more memory.\n"
		 "It has %d holes and resides on layer %d.\n"
		 "%s", USER_UNITMASK, Polygon->ID,
		 flags_to_string (Polygon->Flags, POLYGON_TYPE),
		 Polygon->BoundingBox.X1, Polygon->BoundingBox.Y1,
		 Polygon->BoundingBox.X2, Polygon->BoundingBox.Y2,
		 Polygon->PointN, Polygon->PointMax - Polygon->PointN,
		 Polygon->HoleIndexN,
		 GetLayerNumber (PCB->Data, (LayerTypePtr) ptr1),
		 TEST_FLAG (LOCKFLAG, Polygon) ? "It is LOCKED.\n" : "");
	break;
      }
    case PAD_TYPE:
      {
	Coord len;
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
	len = Distance (Pad->Point1.X, Pad->Point1.Y, Pad->Point2.X, Pad->Point2.Y);
	pcb_sprintf (&report[0], "%m+PAD ID# %ld;  Flags:%s\n"
		 "FirstPoint(X,Y)  = %$mD; ID = %ld.\n"
		 "SecondPoint(X,Y) = %$mD; ID = %ld.\n"
		 "Width = %$mS.  Length = %$mS.\n"
		 "Clearance width in polygons = %$mS.\n"
		 "Solder mask = %$mS x %$mS (gap = %$mS).\n"
		 "Name = \"%s\".\n"
		 "It is owned by SMD element %s\n"
		 "  as pin number %s and is on the %s\n"
		 "side of the board.\n"
		 "%s", USER_UNITMASK, Pad->ID,
		 flags_to_string (Pad->Flags, PAD_TYPE),
		 Pad->Point1.X, Pad->Point1.Y, Pad->Point1.ID,
		 Pad->Point2.X, Pad->Point2.Y, Pad->Point2.ID,
		 Pad->Thickness, len + Pad->Thickness,
		 Pad->Clearance / 2,
		 Pad->Mask, len + Pad->Mask,
		 (Pad->Mask - Pad->Thickness) / 2,
		 EMPTY (Pad->Name),
		 EMPTY (element->Name[1].TextString),
		 EMPTY (Pad->Number),
		 TEST_FLAG (ONSOLDERFLAG,
			    Pad) ? "solder (bottom)" : "component",
		 TEST_FLAG (LOCKFLAG, Pad) ? "It is LOCKED.\n" : "");
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
	pcb_sprintf (&report[0], "%m+ELEMENT ID# %ld;  Flags:%s\n"
		 "BoundingBox %$mD %$mD.\n"
		 "Descriptive Name \"%s\".\n"
		 "Name on board \"%s\".\n"
		 "Part number name \"%s\".\n"
		 "It is %$mS tall and is located at (X,Y) = %$mD %s.\n"
		 "Mark located at point (X,Y) = %$mD.\n"
		 "It is on the %s side of the board.\n"
		 "%s", USER_UNITMASK,
		 element->ID, flags_to_string (element->Flags, ELEMENT_TYPE),
		 element->BoundingBox.X1, element->BoundingBox.Y1,
		 element->BoundingBox.X2, element->BoundingBox.Y2,
		 EMPTY (element->Name[0].TextString),
		 EMPTY (element->Name[1].TextString),
		 EMPTY (element->Name[2].TextString),
		 (Coord) (MIL_TO_COORD (45) * element->Name[1].Scale / 100.),
		 element->Name[1].X, element->Name[1].Y,
		 TEST_FLAG (HIDENAMEFLAG, element) ? ",\n  but it's hidden" : "",
		 element->MarkX, element->MarkY,
		 TEST_FLAG (ONSOLDERFLAG, element) ? "solder (bottom)" : "component",
		 TEST_FLAG (LOCKFLAG, element) ? "It is LOCKED.\n" : "");
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
	  sprintf (laynum, "It is on layer %d.",
		   GetLayerNumber (PCB->Data, (LayerTypePtr) ptr1));
	pcb_sprintf (&report[0], "%m+TEXT ID# %ld;  Flags:%s\n"
		 "Located at (X,Y) = %$mD.\n"
		 "Characters are %$mS tall.\n"
		 "Value is \"%s\".\n"
		 "Direction is %d.\n"
		 "The bounding box is %$mD %$mD.\n"
		 "%s\n"
		 "%s", USER_UNITMASK, text->ID, flags_to_string (text->Flags, TEXT_TYPE),
		 text->X, text->Y, (Coord) (MIL_TO_COORD (45) * (float)text->Scale / 100.),
		 text->TextString, text->Direction,
		 text->BoundingBox.X1, text->BoundingBox.Y1,
		 text->BoundingBox.X2, text->BoundingBox.Y2,
		 (type == TEXT_TYPE) ? laynum : "It is an element name.",
		 TEST_FLAG (LOCKFLAG, text) ? "It is LOCKED.\n" : "");
	break;
      }
    case LINEPOINT_TYPE:
    case POLYGONPOINT_TYPE:
      {
	PointTypePtr point = (PointTypePtr) ptr2;
	pcb_sprintf (&report[0], "%m+POINT ID# %ld.\n"
		 "Located at (X,Y) = %$mD.\n"
		 "It belongs to a %s on layer %d.\n", USER_UNITMASK, point->ID,
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
  /* create dialog box */
  gui->report_dialog ("Report", &report[0]);

  return 0;
}

static int
ReportFoundPins (int argc, char **argv, Coord x, Coord y)
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

  gui->report_dialog ("Report", list.Data);
  return 0;
}

static double
XYtoNetLength (Coord x, Coord y, int *found)
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
ReportAllNetLengths (int argc, char **argv, Coord x, Coord y)
{
  int ni;
  int found;

  for (ni = 0; ni < PCB->NetlistLib.MenuN; ni++)
    {
      char *netname = PCB->NetlistLib.Menu[ni].Name + 2;
      char *ename = PCB->NetlistLib.Menu[ni].Entry[0].ListEntry;
      char *pname;
      bool got_one = 0;

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
		  got_one = 1;
                  break;
		}
	    }
	    END_LOOP;
	    PAD_LOOP (element);
	    {
	      if (strcmp (pad->Number, pname) == 0)
		{
		  x = (pad->Point1.X + pad->Point2.X) / 2;
		  y = (pad->Point1.Y + pad->Point2.Y) / 2;
		  got_one = 1;
                  break;
		}
	    }
	    END_LOOP;
	  }
      }
      END_LOOP;

      if (got_one)
        {
          char buf[50];
          const char *units_name = argv[0];
          Coord length;

          if (argc < 1)
            units_name = Settings.grid_unit->suffix;

          if (ResetConnections (true))
            Draw ();
          /* NB: XYtoNetLength calls LookupConnection, which performs an undo
           *     serial number update, so we don't need to add one here.
           */
          length = XYtoNetLength (x, y, &found);

          pcb_sprintf(buf, "%$m*", units_name, length);
          gui->log("Net %s length %s\n", netname, buf);
        }
    }
  return 0;
}

static int
ReportNetLength (int argc, char **argv, Coord x, Coord y)
{
  Coord length = 0;
  char *netname = 0;
  int found = 0;

  if (ResetConnections (true))
    Draw ();
  /* NB: XYtoNetLength calls LookupConnection, which performs an undo
   *     serial number update, so we don't need to add one here.
   */
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

  {
    char buf[50];
    pcb_sprintf(buf, "%$m*", Settings.grid_unit->suffix, length);
    if (netname)
      gui->log ("Net \"%s\" length: %s\n", netname, buf);
    else
      gui->log ("Net length: %s\n", buf);
  }
  return 0;
}

static int
ReportNetLengthByName (char *tofind, int x, int y)
{
  int result;
  char *netname = 0;
  Coord length = 0;
  int found = 0;
  int i;
  LibraryMenuType *net;
  ConnectionType conn;
  int net_found = 0;
#if defined(USE_RE)
  int use_re = 0;
#endif
#if defined(HAVE_REGCOMP)
  regex_t elt_pattern;
  regmatch_t match;
#endif
#if defined(HAVE_RE_COMP)
  char *elt_pattern;
#endif

  if (!PCB)
    return 1;

  if (!tofind)
    return 1;

  SaveUndoSerialNumber ();
  ResetFoundPinsViasAndPads (true);
  RestoreUndoSerialNumber ();
  ResetFoundLinesAndPolygons (true);
  RestoreUndoSerialNumber ();

#if defined(USE_RE)
      use_re = 1;
      for (i = 0; i < PCB->NetlistLib.MenuN; i++)
	{
	  net = PCB->NetlistLib.Menu + i;
	  if (strcasecmp (tofind, net->Name + 2) == 0)
	    use_re = 0;
	}
      if (use_re)
	{
#if defined(HAVE_REGCOMP)
	  result =
	    regcomp (&elt_pattern, tofind,
		     REG_EXTENDED | REG_ICASE | REG_NOSUB);
	  if (result)
	    {
	      char errorstring[128];

	      regerror (result, &elt_pattern, errorstring, 128);
	      Message (_("regexp error: %s\n"), errorstring);
	      regfree (&elt_pattern);
	      return (1);
	    }
#endif
#if defined(HAVE_RE_COMP)
	  if ((elt_pattern = re_comp (tofind)) != NULL)
	    {
	      Message (_("re_comp error: %s\n"), elt_pattern);
	      return (false);
	    }
#endif
	}
#endif

  for (i = 0; i < PCB->NetlistLib.MenuN; i++)
    {
      net = PCB->NetlistLib.Menu + i;

#if defined(USE_RE)
	  if (use_re)
	    {
#if defined(HAVE_REGCOMP)
	      if (regexec (&elt_pattern, net->Name + 2, 1, &match, 0) != 0)
		continue;
#endif
#if defined(HAVE_RE_COMP)
	      if (re_exec (net->Name + 2) != 1)
		continue;
#endif
	    }
	  else
#endif
	  if (strcasecmp (net->Name + 2, tofind))
	    continue;

        if (SeekPad (net->Entry, &conn, false))
        {
          switch (conn.type)
          {
            case PIN_TYPE:
              x = ((PinType *) (conn.ptr2))->X;
              y = ((PinType *) (conn.ptr2))->Y;
              net_found=1;
	      break;
            case PAD_TYPE:
              x = ((PadType *) (conn.ptr2))->Point1.X;
              y = ((PadType *) (conn.ptr2))->Point1.Y;
              net_found=1;
	      break;
          }
	  if (net_found)
	    break;
        }
    }

  if (!net_found)
    {
      gui->log ("No net named %s\n", tofind);
      return 1;
    }
#ifdef HAVE_REGCOMP
  if (use_re)
    regfree (&elt_pattern);
#endif

  length = XYtoNetLength (x, y, &found);
  netname = net->Name + 2;

  if (!found && net_found)
  {
      gui->log ("Net found, but no lines or arcs were flagged.\n");
      return 1;
  }
  else if (!found)
  {
      gui->log ("Net not found.\n");
      return 1;
  }

  {
    char buf[50];
    pcb_sprintf(buf, "%$m*", Settings.grid_unit->suffix, length);
    if (netname)
      gui->log ("Net \"%s\" length: %s\n", netname, buf);
    else
      gui->log ("Net length: %s\n", buf);
  }
  return 0;
}

/* ---------------------------------------------------------------------------
 * reports on an object 
 * syntax: 
 */

static const char report_syntax[] = "Report(Object|DrillReport|FoundPins|NetLength|AllNetLengths|[,name])";

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
Report (int argc, char **argv, Coord x, Coord y)
{
  if ((argc < 1) || (argc > 2))
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
  else if ((strcasecmp (argv[0], "NetLength") == 0) && (argc == 1))
    return ReportNetLength (argc - 1, argv + 1, x, y);
  else if (strcasecmp (argv[0], "AllNetLengths") == 0)
    return ReportAllNetLengths (argc - 1, argv + 1, x, y);
  else if ((strcasecmp (argv[0], "NetLength") == 0) && (argc == 2))
    return ReportNetLengthByName (argv[1], x, y);
  else if (argc == 2)
    AUSAGE (report);
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
