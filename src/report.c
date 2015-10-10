/*!
 * \file src/report.c
 *
 * \brief .
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
 *
 * This module, report.c, was written and is Copyright (C) 1997 harry eaton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact addresses for paper mail and Email:
 *
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *
 * Thomas.Nau@rz.uni-ulm.de
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
  DrillInfoType *AllDrills;
  Cardinal n;
  char *stringlist, *thestring;
  int total_drills = 0;
  size_t size_left;

  AllDrills = GetDrillInfo (PCB->Data);

  for (n = 0; n < AllDrills->DrillN; n++)
    {
      total_drills += AllDrills->Drill[n].PinCount;
      total_drills += AllDrills->Drill[n].ViaCount;
    }

  size_left = 512L + AllDrills->DrillN * 64L;
  stringlist = (char *)malloc (size_left);

  /* Use tabs for formatting since can't count on a fixed font anymore.
     |  And even that probably isn't going to work in all cases.
   */
  sprintf (stringlist,
	   _("There are %d different drill sizes used in this layout, %d holes total\n\n"
	   "Drill Diam. (%s)\t# of Pins\t# of Vias\t# of Elements\t# Unplated\n"),
	   AllDrills->DrillN, total_drills, Settings.grid_unit->suffix);

  thestring = stringlist;
  while (size_left > 0 && *thestring != '\0')
    {
      thestring++;
      size_left--;
    }

  for (n = 0; n < AllDrills->DrillN; n++)
    {
      pcb_snprintf (thestring, size_left,
	       "%10m*\t\t%d\t\t%d\t\t%d\t\t%d\n",
	       Settings.grid_unit->suffix,
	       AllDrills->Drill[n].DrillSize,
	       AllDrills->Drill[n].PinCount, AllDrills->Drill[n].ViaCount,
	       AllDrills->Drill[n].ElementN,
	       AllDrills->Drill[n].UnplatedCount);
      while (size_left > 0 && *thestring != '\0')
	{
	  thestring++;
	  size_left--;
	}
    }
  FreeDrillInfo (AllDrills);
  /* create dialog box */
  gui->report_dialog (_("Drill Report"), stringlist);

  free (stringlist);
  return 0;
}


static const char reportdialog_syntax[] = N_("ReportDialog()");

static const char reportdialog_help[] =
  N_("Report on the object under the crosshair");

/* %start-doc actions ReportDialog

This is a shortcut for @code{Report(Object)}.

%end-doc */

static int
ReportDialog (int argc, char **argv, Coord x, Coord y)
{
  void *ptr1, *ptr2, *ptr3;
  int type;
  char report[2048];
  char *attr;

  type = SearchScreen (x, y, REPORT_TYPES, &ptr1, &ptr2, &ptr3);
  if (type == NO_TYPE)
    type =
      SearchScreen (x, y, REPORT_TYPES | LOCKED_TYPE, &ptr1, &ptr2, &ptr3);

  switch (type)
    {
    case VIA_TYPE:
      {
	PinType *via;
#ifndef NDEBUG
	if (gui->shift_is_pressed ())
	  {
	    __r_dump_tree (PCB->Data->via_tree->root, 0);
	    return 0;
	  }
#endif
	via = (PinType *) ptr2;
	if (TEST_FLAG (HOLEFLAG, via))
	  pcb_snprintf (report, sizeof (report), _("%m+VIA ID# %ld; Flags:%s\n"
		   "(X,Y) = %$mD.\n"
		   "It is a pure hole of diameter %$mS.\n"
		   "Name = \"%s\"."
		   "%s"), USER_UNITMASK, via->ID, flags_to_string (via->Flags, VIA_TYPE),
		   via->X, via->Y, via->DrillingHole, EMPTY (via->Name),
		   TEST_FLAG (LOCKFLAG, via) ? _("It is LOCKED.\n") : "");
	else
	  pcb_snprintf (report, sizeof (report), _("%m+VIA ID# %ld;  Flags:%s\n"
		   "(X,Y) = %$mD.\n"
		   "Copper width = %$mS. Drill width = %$mS.\n"
		   "Clearance width in polygons = %$mS.\n"
		   "Annulus = %$mS.\n"
		   "Solder mask hole = %$mS (gap = %$mS).\n"
		   "Name = \"%s\"."
		   "%s"), USER_UNITMASK, via->ID, flags_to_string (via->Flags, VIA_TYPE),
		   via->X, via->Y,
		   via->Thickness,
		   via->DrillingHole,
		   via->Clearance / 2,
		   (via->Thickness - via->DrillingHole) / 2,
		   via->Mask,
		   (via->Mask - via->Thickness) / 2,
		   EMPTY (via->Name), TEST_FLAG (LOCKFLAG, via) ?
		   _("It is LOCKED.\n") : "");
	break;
      }
    case PIN_TYPE:
      {
	PinType *Pin;
	ElementType *element;
#ifndef NDEBUG
	if (gui->shift_is_pressed ())
	  {
	    __r_dump_tree (PCB->Data->pin_tree->root, 0);
	    return 0;
	  }
#endif
	Pin = (PinType *) ptr2;
	element = (ElementType *) ptr1;

	PIN_LOOP (element);
	{
	  if (pin == Pin)
	    break;
	}
	END_LOOP;
	if (TEST_FLAG (HOLEFLAG, Pin))
	  pcb_snprintf (report, sizeof (report), _("%m+PIN ID# %ld; Flags:%s\n"
		   "(X,Y) = %$mD.\n"
		   "It is a mounting hole. Drill width = %$mS.\n"
		   "It is owned by element %$mS.\n"
		   "%s"), USER_UNITMASK, Pin->ID, flags_to_string (Pin->Flags, PIN_TYPE),
		   Pin->X, Pin->Y, Pin->DrillingHole,
		   EMPTY (element->Name[1].TextString),
		   TEST_FLAG (LOCKFLAG, Pin) ? _("It is LOCKED.\n") : "");
	else
	  pcb_snprintf (report, sizeof (report),
		   _("%m+PIN ID# %ld;  Flags:%s\n" "(X,Y) = %$mD.\n"
		   "Copper width = %$mS. Drill width = %$mS.\n"
		   "Clearance width to Polygon = %$mS.\n"
		   "Annulus = %$mS.\n"
		   "Solder mask hole = %$mS (gap = %$mS).\n"
		   "Name = \"%s\".\n"
		   "It is owned by element %s\n as pin number %s.\n"
		   "%s"), USER_UNITMASK,
		   Pin->ID, flags_to_string (Pin->Flags, PIN_TYPE),
		   Pin->X, Pin->Y, Pin->Thickness,
		   Pin->DrillingHole,
		   Pin->Clearance / 2,
		   (Pin->Thickness - Pin->DrillingHole) / 2,
		   Pin->Mask,
		   (Pin->Mask - Pin->Thickness) / 2,
		   EMPTY (Pin->Name),
		   EMPTY (element->Name[1].TextString), EMPTY (Pin->Number),
		   TEST_FLAG (LOCKFLAG, Pin) ? _("It is LOCKED.\n") : "");
	break;
      }
    case LINE_TYPE:
      {
	LineType *line;
#ifndef NDEBUG
	if (gui->shift_is_pressed ())
	  {
	    LayerType *layer = (LayerType *) ptr1;
	    __r_dump_tree (layer->line_tree->root, 0);
	    return 0;
	  }
#endif
	line = (LineType *) ptr2;
	pcb_snprintf (report, sizeof (report), _("%m+LINE ID# %ld;  Flags:%s\n"
		 "FirstPoint(X,Y)  = %$mD, ID = %ld.\n"
		 "SecondPoint(X,Y) = %$mD, ID = %ld.\n"
		 "Width = %$mS.\nClearance width in polygons = %$mS.\n"
		 "It is on layer %d\n"
		 "and has name \"%s\".\n"
		 "%s"), USER_UNITMASK,
		 line->ID, flags_to_string (line->Flags, LINE_TYPE),
		 line->Point1.X, line->Point1.Y, line->Point1.ID,
		 line->Point2.X, line->Point2.Y, line->Point2.ID,
		 line->Thickness, line->Clearance / 2,
		 GetLayerNumber (PCB->Data, (LayerType *) ptr1),
		 UNKNOWN (line->Number),
		 TEST_FLAG (LOCKFLAG, line) ? _("It is LOCKED.\n") : "");
	break;
      }
    case RATLINE_TYPE:
      {
	RatType *line;
#ifndef NDEBUG
	if (gui->shift_is_pressed ())
	  {
	    __r_dump_tree (PCB->Data->rat_tree->root, 0);
	    return 0;
	  }
#endif
	line = (RatType *) ptr2;
	pcb_snprintf (report, sizeof (report), _("%m+RAT-LINE ID# %ld;  Flags:%s\n"
		 "FirstPoint(X,Y)  = %$mD; ID = %ld; "
		 "connects to layer group %d.\n"
		 "SecondPoint(X,Y) = %$mD; ID = %ld; "
		 "connects to layer group %d.\n"),
		 USER_UNITMASK, line->ID, flags_to_string (line->Flags, LINE_TYPE),
		 line->Point1.X, line->Point1.Y,
		 line->Point1.ID, line->group1,
		 line->Point2.X, line->Point2.Y,
		 line->Point2.ID, line->group2);
	break;
      }
    case ARC_TYPE:
      {
	ArcType *Arc;
	BoxType *box;
#ifndef NDEBUG
	if (gui->shift_is_pressed ())
	  {
	    LayerType *layer = (LayerType *) ptr1;
	    __r_dump_tree (layer->arc_tree->root, 0);
	    return 0;
	  }
#endif
	Arc = (ArcType *) ptr2;
	box = GetArcEnds (Arc);

	pcb_snprintf (report, sizeof (report), _("%m+ARC ID# %ld;  Flags:%s\n"
		 "CenterPoint(X,Y) = %$mD.\n"
		 "Radius = %$mS, Thickness = %$mS.\n"
		 "Clearance width in polygons = %$mS.\n"
		 "StartAngle = %ma degrees, DeltaAngle = %ma degrees.\n"
		 "Bounding Box is %$mD, %$mD.\n"
		 "That makes the end points at %$mD and %$mD.\n"
		 "It is on layer %d.\n"
		 "%s"), USER_UNITMASK, Arc->ID, flags_to_string (Arc->Flags, ARC_TYPE),
		 Arc->X, Arc->Y,
		 Arc->Width, Arc->Thickness,
		 Arc->Clearance / 2, Arc->StartAngle, Arc->Delta,
		 Arc->BoundingBox.X1, Arc->BoundingBox.Y1,
		 Arc->BoundingBox.X2, Arc->BoundingBox.Y2,
		 box->X1, box->Y1,
		 box->X2, box->Y2,
		 GetLayerNumber (PCB->Data, (LayerType *) ptr1),
		 TEST_FLAG (LOCKFLAG, Arc) ? _("It is LOCKED.\n") : "");
	break;
      }
    case POLYGON_TYPE:
      {
	PolygonType *Polygon;
#ifndef NDEBUG
	if (gui->shift_is_pressed ())
	  {
	    LayerType *layer = (LayerType *) ptr1;
	    __r_dump_tree (layer->polygon_tree->root, 0);
	    return 0;
	  }
#endif
	Polygon = (PolygonType *) ptr2;

	pcb_snprintf (report, sizeof (report), _("%m+POLYGON ID# %ld;  Flags:%s\n"
		 "Its bounding box is %$mD %$mD.\n"
		 "It has %d points and could store %d more\n"
		 "  without using more memory.\n"
		 "It has %d holes and resides on layer %d.\n"
		 "%s"), USER_UNITMASK, Polygon->ID,
		 flags_to_string (Polygon->Flags, POLYGON_TYPE),
		 Polygon->BoundingBox.X1, Polygon->BoundingBox.Y1,
		 Polygon->BoundingBox.X2, Polygon->BoundingBox.Y2,
		 Polygon->PointN, Polygon->PointMax - Polygon->PointN,
		 Polygon->HoleIndexN,
		 GetLayerNumber (PCB->Data, (LayerType *) ptr1),
		 TEST_FLAG (LOCKFLAG, Polygon) ? _("It is LOCKED.\n") : "");
	break;
      }
    case PAD_TYPE:
      {
	Coord len;
	PadType *Pad;
	ElementType *element;
#ifndef NDEBUG
	if (gui->shift_is_pressed ())
	  {
	    __r_dump_tree (PCB->Data->pad_tree->root, 0);
	    return 0;
	  }
#endif
	Pad = (PadType *) ptr2;
	element = (ElementType *) ptr1;

	PAD_LOOP (element);
	{
	  {
	    if (pad == Pad)
	      break;
	  }
	}
	END_LOOP;
	len = Distance (Pad->Point1.X, Pad->Point1.Y, Pad->Point2.X, Pad->Point2.Y);
	pcb_snprintf (report, sizeof (report), _("%m+PAD ID# %ld;  Flags:%s\n"
		 "FirstPoint(X,Y)  = %$mD; ID = %ld.\n"
		 "SecondPoint(X,Y) = %$mD; ID = %ld.\n"
		 "Width = %$mS.  Length = %$mS.\n"
		 "Clearance width in polygons = %$mS.\n"
		 "Solder mask = %$mS x %$mS (gap = %$mS).\n"
		 "Name = \"%s\".\n"
		 "It is owned by SMD element %s\n"
		 "  as pin number %s and is on the %s\n"
		 "side of the board.\n"
		 "%s"), USER_UNITMASK, Pad->ID,
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
			    Pad) ? _("solder (bottom)") : _("component"),
		 TEST_FLAG (LOCKFLAG, Pad) ? _("It is LOCKED.\n") : "");
	break;
      }
    case ELEMENT_TYPE:
      {
	ElementType *element;
#ifndef NDEBUG
	if (gui->shift_is_pressed ())
	  {
	    __r_dump_tree (PCB->Data->element_tree->root, 0);
	    return 0;
	  }
#endif
	element = (ElementType *) ptr2;
	attr = AttributeGetFromList (&(element->Attributes), "Footprint::RotationTracking");
	pcb_snprintf (report, sizeof (report), _("%m+ELEMENT ID# %ld;  Flags:%s\n"
		 "BoundingBox %$mD %$mD.\n"
		 "Descriptive Name \"%s\".\n"
		 "Name on board \"%s\".\n"
		 "Part number name \"%s\".\n"
		 "It is %$mS tall and is located at (X,Y) = %$mD %s.\n"
		 "Mark located at point (X,Y) = %$mD.\n"
                 "Element is rotated by %s degrees.\n"
		 "It is on the %s side of the board.\n"
		 "%s"), USER_UNITMASK,
		 element->ID, flags_to_string (element->Flags, ELEMENT_TYPE),
		 element->BoundingBox.X1, element->BoundingBox.Y1,
		 element->BoundingBox.X2, element->BoundingBox.Y2,
		 EMPTY (element->Name[0].TextString),
		 EMPTY (element->Name[1].TextString),
		 EMPTY (element->Name[2].TextString),
		 SCALE_TEXT (FONT_CAPHEIGHT, element->Name[1].Scale),
		 element->Name[1].X, element->Name[1].Y,
		 TEST_FLAG (HIDENAMEFLAG, element) ? _(",\n  but it's hidden") : "",
		 element->MarkX, element->MarkY,
                 (attr)?attr:"0.00",
		 TEST_FLAG (ONSOLDERFLAG, element) ? _("solder (bottom)") : _("component"),
		 TEST_FLAG (LOCKFLAG, element) ? _("It is LOCKED.\n") : "");
	break;
      }
    case TEXT_TYPE:
#ifndef NDEBUG
      if (gui->shift_is_pressed ())
	{
	  LayerType *layer = (LayerType *) ptr1;
	  __r_dump_tree (layer->text_tree->root, 0);
	  return 0;
	}
#endif
    case ELEMENTNAME_TYPE:
      {
	char laynum[32];
	TextType *text;
#ifndef NDEBUG
	if (gui->shift_is_pressed ())
	  {
	    __r_dump_tree (PCB->Data->name_tree[NAME_INDEX (PCB)]->root, 0);
	    return 0;
	  }
#endif
	text = (TextType *) ptr2;

	if (type == TEXT_TYPE)
	  sprintf (laynum, _("It is on layer %d."),
		   GetLayerNumber (PCB->Data, (LayerType *) ptr1));
	pcb_snprintf (report, sizeof (report), _("%m+TEXT ID# %ld;  Flags:%s\n"
		 "Located at (X,Y) = %$mD.\n"
		 "Characters are %$mS tall.\n"
		 "Value is \"%s\".\n"
		 "Direction is %d.\n"
		 "The bounding box is %$mD %$mD.\n"
		 "%s\n"
		 "%s"), USER_UNITMASK, text->ID, flags_to_string (text->Flags, TEXT_TYPE),
		 text->X, text->Y, SCALE_TEXT (FONT_CAPHEIGHT, text->Scale),
		 text->TextString, text->Direction,
		 text->BoundingBox.X1, text->BoundingBox.Y1,
		 text->BoundingBox.X2, text->BoundingBox.Y2,
		 (type == TEXT_TYPE) ? laynum : _("It is an element name."),
		 TEST_FLAG (LOCKFLAG, text) ? _("It is LOCKED.\n") : "");
	break;
      }
    case LINEPOINT_TYPE:
    case POLYGONPOINT_TYPE:
      {
	PointType *point = (PointType *) ptr2;
	pcb_snprintf (report, sizeof (report), _("%m+POINT ID# %ld.\n"
		 "Located at (X,Y) = %$mD.\n"
		 "It belongs to a %s on layer %d.\n"), USER_UNITMASK, point->ID,
		 point->X, point->Y,
		 (type == LINEPOINT_TYPE) ?
		     C_("report", "line") : C_("report", "polygon"),
		 GetLayerNumber (PCB->Data, (LayerType *) ptr1));
	break;
      }
    case NO_TYPE:
      report[0] = '\0';
      break;

    default:
      sprintf (report, _("Unknown\n"));
      break;
    }

  if (report[0] == '\0')
    {
      Message (_("Nothing found to report on\n"));
      return 1;
    }
  /* create dialog box */
  gui->report_dialog (_("Report"), report);

  return 0;
}

static int
ReportFoundPins (int argc, char **argv, Coord x, Coord y)
{
  static DynamicStringType list;
  char temp[64];
  int col = 0;

  DSClearString (&list);
  DSAddString (&list, _("The following pins/pads are FOUND:\n"));
  ELEMENT_LOOP (PCB->Data);
  {
    PIN_LOOP (element);
    {
      if (TEST_FLAG (FOUNDFLAG, pin))
	{
	  sprintf (temp, _("%s-%s,%c"),
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
	  sprintf (temp, _("%s-%s,%c"),
		   NAMEONPCB_NAME (element), pad->Number,
		   ((col++ % (COLUMNS + 1)) == COLUMNS) ? '\n' : ' ');
	  DSAddString (&list, temp);
	}
    }
    END_LOOP;
  }
  END_LOOP;

  gui->report_dialog (_("Report"), list.Data);
  return 0;
}

/*!
 * \brief Assumes that we start with a blank connection state,
 * e.g. ClearFlagOnAllObjects() has been run.
 *
 * Does not add its own changes to the undo system
 */
static double
XYtoNetLength (Coord x, Coord y, int *found)
{
  double length;

  length = 0;
  *found = 0;

  /* NB: The third argument here, 'false' ensures LookupConnection
   *     does not add its changes to the undo system.
   */
  LookupConnection (x, y, false, PCB->Grid, FOUNDFLAG, true);

  ALLLINE_LOOP (PCB->Data);
  {
    if (TEST_FLAG (FOUNDFLAG, line))
      {
	int dx, dy;
	dx = line->Point1.X - line->Point2.X;
	dy = line->Point1.Y - line->Point2.Y;
	length += hypot (dx, dy);
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

  /* Reset all connection flags and save an undo-state to get back
   * to the state the board was in when we started this function.
   *
   * After this, we don't add any changes to the undo system, but
   * ensure we get back to a point where we can Undo() our changes
   * by resetting the connections with ClearFlagOnAllObjects() before
   * calling Undo() at the end of the procedure.
   */
  ClearFlagOnAllObjects (true, FOUNDFLAG);
  IncrementUndoSerialNumber ();

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

          length = XYtoNetLength (x, y, &found);

          /* Reset connectors for the next lookup */
          ClearFlagOnAllObjects (false, FOUNDFLAG);

          pcb_snprintf(buf, sizeof (buf), _("%$m*"), units_name, length);
          gui->log(_("Net %s length %s\n"), netname, buf);
        }
    }

  ClearFlagOnAllObjects (false, FOUNDFLAG);
  Undo (true);
  return 0;
}

static int
ReportNetLength (int argc, char **argv, Coord x, Coord y)
{
  Coord length = 0;
  char *netname = 0;
  int found = 0;

  gui->get_coords (_("Click on a connection"), &x, &y);

  /* Reset all connection flags and save an undo-state to get back
   * to the state the board was in when we started this function.
   *
   * After this, we don't add any changes to the undo system, but
   * ensure we get back to a point where we can Undo() our changes
   * by resetting the connections with ClearFlagOnAllObjects() before
   * calling Undo() at the end of the procedure.
   */
  ClearFlagOnAllObjects (true, FOUNDFLAG);
  IncrementUndoSerialNumber ();

  length = XYtoNetLength (x, y, &found);

  if (!found)
    {
      ClearFlagOnAllObjects (false, FOUNDFLAG);
      Undo (true);
      gui->log (_("No net under cursor.\n"));
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
	      n = Concat (ename, _("-"), pname, NULL);
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
	      n = Concat (ename, _("-"), pname, NULL);
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
  ClearFlagOnAllObjects (false, FOUNDFLAG);
  Undo (true);

  {
    char buf[50];
    pcb_snprintf(buf, sizeof (buf), _("%$m*"), Settings.grid_unit->suffix, length);
    if (netname)
      gui->log (_("Net \"%s\" length: %s\n"), netname, buf);
    else
      gui->log (_("Net length: %s\n"), buf);
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
	      return (1);
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
      gui->log (_("No net named %s\n"), tofind);
      return 1;
    }

#ifdef HAVE_REGCOMP
  if (use_re)
    regfree (&elt_pattern);
#endif

  /* Reset all connection flags and save an undo-state to get back
   * to the state the board was in when we started.
   *
   * After this, we don't add any changes to the undo system, but
   * ensure we get back to a point where we can Undo() our changes
   * by resetting the connections with ClearFlagOnAllObjects() before
   * calling Undo() when we are finished.
   */
  ClearFlagOnAllObjects (true, FOUNDFLAG);
  IncrementUndoSerialNumber ();

  length = XYtoNetLength (x, y, &found);
  netname = net->Name + 2;

  ClearFlagOnAllObjects (false, FOUNDFLAG);
  Undo (true);

  if (!found)
    {
      if (net_found)
        gui->log (_("Net found, but no lines or arcs were flagged.\n"));
      else
        gui->log (_("Net not found.\n"));

      return 1;
    }

  {
    char buf[50];
    pcb_snprintf(buf, 50, _("%$m*"), Settings.grid_unit->suffix, length);
    if (netname)
      gui->log (_("Net \"%s\" length: %s\n"), netname, buf);
    else
      gui->log (_("Net length: %s\n"), buf);
  }

  return 0;
}

static const char report_syntax[] =
  N_("Report(Object|DrillReport|FoundPins|NetLength|AllNetLengths|[,name])");

static const char report_help[] = N_("Produce various report.");

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
/*!
 * \brief Reports on an object.
 */
static int
Report (int argc, char **argv, Coord x, Coord y)
{
  if ((argc < 1) || (argc > 2))
    AUSAGE (report);
  else if (strcasecmp (argv[0], "Object") == 0)
    {
      gui->get_coords (_("Click on an object"), &x, &y);
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
  {"ReportObject", N_("Click on an object"), ReportDialog,
   reportdialog_help, reportdialog_syntax}
  ,
  {"Report", 0, Report,
   report_help, report_syntax}
};

REGISTER_ACTIONS (report_action_list)
