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

static char *rcsid = "$Id$";

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "report.h"
#include "crosshair.h"
#include "data.h"
#include "dialog.h"
#include "drill.h"
#include "error.h"
#include "search.h"
#include "misc.h"
#include "mymem.h"


#include <X11/Shell.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Toggle.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

static long int ReturnCode;	/* filled in by dialog */

/*
 * some local prototypes
 */
static void CB_OK (Widget, XtPointer, XtPointer);

static DialogButtonType button =
  { "defaultButton", "  OK  ", CB_OK, (XtPointer) OK_BUTTON, NULL };

/* ---------------------------------------------------------------------------
 * callback for standard dialog
 * just copies the button-code which is passed as ClientData to a
 * public identifier
 */
static void
CB_OK (Widget W, XtPointer ClientData, XtPointer CallData)
{
  ReturnCode = (long int) ClientData;
}

void
ReportDrills (void)
{
  DrillInfoTypePtr AllDrills;
  Cardinal n;
  char *stringlist, *thestring;
  Widget popup;
  int total_drills = 0;

  AllDrills = GetDrillInfo (PCB->Data);

  for (n = 0; n < AllDrills->DrillN; n++)
    {
      total_drills += AllDrills->Drill[n].PinCount;
      total_drills += AllDrills->Drill[n].ViaCount;
      total_drills += AllDrills->Drill[n].UnplatedCount;
    }

  stringlist = malloc (512L + AllDrills->DrillN * 64L);
  sprintf (stringlist,
	   "There are %d different drill sizes used in this layout, %d holes total\n\n"
	   "Drill Diam. (mils)      # of Pins     # of Vias    # of Elements    # Unplated\n",
	   AllDrills->DrillN, total_drills);
  thestring = stringlist;
  while (*thestring != '\0')
    thestring++;
  for (n = 0; n < AllDrills->DrillN; n++)
    {
      sprintf (thestring,
	       "  %5d                 %5d          %5d            %5d           %5d\n",
	       AllDrills->Drill[n].DrillSize/100, AllDrills->Drill[n].PinCount,
	       AllDrills->Drill[n].ViaCount, AllDrills->Drill[n].ElementN,
	       AllDrills->Drill[n].UnplatedCount);
      while (*thestring != '\0')
	thestring++;
    }
  FreeDrillInfo (AllDrills);
  /* create dialog box */
  popup = CreateDialogBox (stringlist, &button, 1, "Drill Report");
  StartDialog (popup);

  /* wait for dialog to complete */
  DialogEventLoop (&ReturnCode);
  EndDialog (popup);
  SaveFree (stringlist);
}




void
ReportDialog (void)
{
  void *ptr1, *ptr2, *ptr3;
  int type;
  char report[2048];
  Widget popup;

  switch (type = SearchScreen (Crosshair.X, Crosshair.Y,
			       REPORT_TYPES, &ptr1, &ptr2, &ptr3))
    {
    case VIA_TYPE:
      {
	PinTypePtr via = (PinTypePtr) ptr2;
	if (TEST_FLAG (HOLEFLAG, via))
	  sprintf (&report[0], "VIA ID# %ld  Flags:0x%08lx\n"
		   "(X,Y) = (%d, %d)\n"
		   "It is a pure hole of diameter %0.2f mils\n"
		   "Name = \"%s\""
		   "%s", via->ID, via->Flags, via->X,
		   via->Y, via->DrillingHole / 100.0, EMPTY (via->Name),
		   TEST_FLAG (LOCKFLAG, via) ? "It is LOCKED\n" : "");
	else
	  sprintf (&report[0], "VIA ID# %ld   Flags:0x%08lx\n"
		   "(X,Y) = (%d, %d)\n"
		   "Copper width = %0.2f mils  Drill width = %0.2f mils\n"
		   "Clearance width in polygons = %0.2f mils\n"
		   "Solder mask hole = %0.2f mils\n"
		   "Name = \"%s\""
		   "%s", via->ID, via->Flags, via->X,
		   via->Y, via->Thickness / 100., via->DrillingHole / 100.,
		   via->Clearance / 200., via->Mask / 100.,
		   EMPTY (via->Name), TEST_FLAG (LOCKFLAG, via) ?
		   "It is LOCKED\n" : "");
	break;
      }
    case PIN_TYPE:
      {
	PinTypePtr Pin = (PinTypePtr) ptr2;
	ElementTypePtr element = (ElementTypePtr) ptr1;

	PIN_LOOP (element, 
	  {
	    if (pin == Pin)
	      break;
	  }
	);
	if (TEST_FLAG (HOLEFLAG, Pin))
	  sprintf (&report[0], "PIN ID# %ld  Flags:0x%08lx\n"
		   "(X,Y) = (%d, %d)\n"
		   "It is a mounting hole, Drill width = %0.2f mils\n"
		   "It is owned by element %s\n"
		   "%s", Pin->ID, Pin->Flags,
		   Pin->X, Pin->Y, Pin->DrillingHole / 100.,
		   EMPTY (element->Name[1].TextString),
		   TEST_FLAG (LOCKFLAG, Pin) ? "It is LOCKED\n" : "");
	else
	  sprintf (&report[0],
		   "PIN ID# %ld   Flags:0x%08lx\n" "(X,Y) = (%d, %d)\n"
		   "Copper width = %0.2f mils  Drill width = %0.2f mils\n"
		   "Clearance width to Polygon = %0.2f mils\n"
		   "Solder mask hole = %0.2f mils\n" "Name = \"%s\"\n"
		   "It is owned by element %s\n" "As pin number %s\n"
		   "%s",
		   Pin->ID, Pin->Flags, Pin->X, Pin->Y, Pin->Thickness / 100.,
		   Pin->DrillingHole / 100., Pin->Clearance / 200.,
		   Pin->Mask / 100., EMPTY (Pin->Name),
		   EMPTY (element->Name[1].TextString), EMPTY (Pin->Number),
		   TEST_FLAG (LOCKFLAG, Pin) ? "It is LOCKED\n" : "");
	break;
      }
    case LINE_TYPE:
      {
	LineTypePtr line = (LineTypePtr) ptr2;
	sprintf (&report[0], "LINE ID# %ld   Flags:0x%08lx\n"
		 "FirstPoint(X,Y) = (%d, %d)  ID = %ld\n"
		 "SecondPoint(X,Y) = (%d, %d)  ID = %ld\n"
		 "Width = %0.2f mils.\nClearance width in polygons = %0.2f mils.\n"
		 "It is on layer %d\n"
		 "and has name %s\n"
		 "%s",
		 line->ID, line->Flags,
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
	RatTypePtr line = (RatTypePtr) ptr2;
	sprintf (&report[0], "RAT-LINE ID# %ld   Flags:0x%08lx\n"
		 "FirstPoint(X,Y) = (%d, %d) ID = %ld "
		 "connects to layer group %d\n"
		 "SecondPoint(X,Y) = (%d, %d) ID = %ld "
		 "connects to layer group %d\n",
		 line->ID, line->Flags,
		 line->Point1.X, line->Point1.Y,
		 line->Point1.ID, line->group1,
		 line->Point2.X, line->Point2.Y,
		 line->Point2.ID, line->group2);
	break;
      }
    case ARC_TYPE:
      {
	ArcTypePtr Arc = (ArcTypePtr) ptr2;
	BoxTypePtr box = GetArcEnds (Arc);

	sprintf (&report[0], "ARC ID# %ld   Flags:0x%08lx\n"
		 "CenterPoint(X,Y) = (%d, %d)\n"
		 "Radius = %0.2f mils, Thickness = %0.2f mils\n"
		 "Clearance width in polygons = %0.2f mils\n"
		 "StartAngle = %ld degrees, DeltaAngle = %ld degrees\n"
		 "That makes the end points at (%d,%d) and (%d,%d)\n"
		 "It is on layer %d\n"
		 "%s", Arc->ID, Arc->Flags,
		 Arc->X, Arc->Y, Arc->Width / 100., Arc->Thickness / 100.,
		 Arc->Clearance / 200., Arc->StartAngle, Arc->Delta, box->X1,
		 box->Y1, box->X2, box->Y2, GetLayerNumber (PCB->Data,
							    (LayerTypePtr)
							    ptr1),
		 TEST_FLAG (LOCKFLAG, Arc) ? "It is LOCKED\n" : "");
	break;
      }
    case POLYGON_TYPE:
      {
	PolygonTypePtr Polygon = (PolygonTypePtr) ptr2;

	sprintf (&report[0], "POLYGON ID# %ld   Flags:0x%08lx\n"
		 "It has %d points and could store %d more\n"
		 "without using more memory.\n"
		 "It resides on layer %d\n"
		 "%s", Polygon->ID,
		 Polygon->Flags, Polygon->PointN, Polygon->PointMax
		 - Polygon->PointN,
		 GetLayerNumber (PCB->Data, (LayerTypePtr) ptr1),
		 TEST_FLAG (LOCKFLAG, Polygon) ? "It is LOCKED\n" : "");
	break;
      }
    case PAD_TYPE:
      {
	PadTypePtr Pad = (PadTypePtr) ptr2;
	ElementTypePtr element = (ElementTypePtr) ptr1;

	PAD_LOOP (element, 
	  {
	    {
	      if (pad == Pad)
		break;
	    }
	  }
	);
	sprintf (&report[0], "PAD ID# %ld   Flags:0x%08lx\n"
		 "FirstPoint(X,Y) = (%d, %d)  ID = %ld\n"
		 "SecondPoint(X,Y) = (%d, %d)  ID = %ld\n"
		 "Width = %0.2f mils.\nClearance width in polygons = %0.2f mils.\n"
		 "Solder mask width = %0.2f mils\n"
		 "Name = \"%s\"\n"
		 "It is owned by SMD element %s\n"
		 "As pin number %s and is on the %s\n"
		 "side of the board.\n"
		 "%s", Pad->ID,
		 Pad->Flags, Pad->Point1.X,
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
	ElementTypePtr element = (ElementTypePtr) ptr2;
	sprintf (&report[0], "ELEMENT ID# %ld   Flags:0x%08lx\n"
		 "Descriptive Name \"%s\"\n"
		 "Name on board \"%s\"\n"
		 "Part number name \"%s\"\n"
		 "Mark located at point (X,Y) = (%d,%d)\n"
		 "It is on the %s side of the board.\n"
		 "%s",
		 element->ID, element->Flags,
		 EMPTY (element->Name[0].TextString),
		 EMPTY (element->Name[1].TextString),
		 EMPTY (element->Name[2].TextString), element->MarkX,
		 element->MarkY, TEST_FLAG (ONSOLDERFLAG,
					    element) ? "solder (bottom)" :
		 "component", TEST_FLAG (LOCKFLAG, element) ?
		 "It is LOCKED\n" : "");
	break;
      }
    case TEXT_TYPE:
    case ELEMENTNAME_TYPE:
      {
	char laynum[32];
	TextTypePtr text = (TextTypePtr) ptr2;

	if (type == TEXT_TYPE)
	  sprintf (laynum, "is on layer %d",
		   GetLayerNumber (PCB->Data, (LayerTypePtr) ptr1));
	sprintf (&report[0], "TEXT ID# %ld   Flags:0x%08lx\n"
		 "Located at (X,Y) = (%d,%d)\n"
		 "Characters are %0.2f mils tall\n"
		 "Value is \"%s\"\n"
		 "Direction is %d\n"
		 "The bounding box is (%d,%d) (%d, %d)\n"
		 "It %s\n"
		 "%s", text->ID, text->Flags,
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
	sprintf (&report[0], "POINT ID# %ld Points don't have flags."
		 "Located at (X,Y) = (%d,%d)\n"
		 "It belongs to a %s on layer %d\n", point->ID,
		 point->X, point->Y,
		 (type == LINEPOINT_TYPE) ? "line" : "polygon",
		 GetLayerNumber (PCB->Data, (LayerTypePtr) ptr1));

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
      Message ("Nothing found to report on\n");
      return;
    }
  HideCrosshair (False);
  /* create dialog box */
  popup = CreateDialogBox (&report[0], &button, 1, "Report");
  StartDialog (popup);

  /* wait for dialog to complete */
  DialogEventLoop (&ReturnCode);
  RestoreCrosshair (False);
  EndDialog (popup);
}

void
ReportFoundPins (void)
{
  static DynamicStringType list;
  char temp[64];
  int col = 0;
  Widget popup;

  DSClearString (&list);
  DSAddString (&list, "The following pins/pads are FOUND:\n");
  ELEMENT_LOOP (PCB->Data, 
    {
      PIN_LOOP (element, 
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
      );
      PAD_LOOP (element, 
	{
	  if (TEST_FLAG (FOUNDFLAG, pad))
	    {
	      sprintf (temp, "%s-%s,%c",
		       NAMEONPCB_NAME (element), pad->Number,
		       ((col++ % (COLUMNS + 1)) == COLUMNS) ? '\n' : ' ');
	      DSAddString (&list, temp);
	    }
	}
      );
    }
  );
  HideCrosshair (False);
  /* create dialog box */
  popup = CreateDialogBox (list.Data, &button, 1, "Report");
  StartDialog (popup);

  /* wait for dialog to complete */
  DialogEventLoop (&ReturnCode);
  RestoreCrosshair (False);
  EndDialog (popup);
}
