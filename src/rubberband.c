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

static char *rcsid =
  "$Id$";

/* functions used by 'rubberband moves'
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <memory.h>
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "global.h"

#include "create.h"
#include "data.h"
#include "misc.h"
#include "rubberband.h"
#include "search.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void CheckPadForRubberbandConnection (PadTypePtr);
static void CheckPinForRubberbandConnection (PinTypePtr);
static void CheckLinePointForRubberbandConnection (LayerTypePtr,
						   LineTypePtr, PointTypePtr);
static void CheckPolygonForRubberbandConnection (LayerTypePtr,
						 PolygonTypePtr);
static void CheckLinePointForRat (LayerTypePtr, PointTypePtr);

/* ---------------------------------------------------------------------------
 * checks all visible lines which belong to the same layergroup as the
 * passed pad. If one of the endpoints of the line lays inside the pad,
 * the line is added to the 'rubberband' list
 */
static void
CheckPadForRubberbandConnection (PadTypePtr Pad)
{
  BDimension half = Pad->Thickness / 2;
  Location minx = MIN (Pad->Point1.X, Pad->Point2.X) - half,
    miny = MIN (Pad->Point1.Y, Pad->Point2.Y) - half,
    maxx = MAX (Pad->Point1.X, Pad->Point2.X) + half,
    maxy = MAX (Pad->Point1.Y, Pad->Point2.Y) + half;
  Cardinal i, group;

  i = MAX_LAYER +
    (TEST_FLAG (ONSOLDERFLAG, Pad) ? SOLDER_LAYER : COMPONENT_LAYER);
  group = GetLayerGroupNumberByNumber (i);

  /* check all visible layers in the same group */
  for (i = 0; i < PCB->LayerGroups.Number[group]; i++)
    {
      Cardinal number = PCB->LayerGroups.Entries[group][i];
      LayerTypePtr layer;

      /* skip solder/component layers */
      if (number >= MAX_LAYER)
	continue;

      /* check all visible lines of the group member */
      layer = LAYER_PTR (number);
      if (layer->On)
	{
	  LINE_LOOP (layer, 
	    {
	      if (TEST_FLAG (LOCKFLAG, line))
		continue;
	      if (line->Point1.X >= minx && line->Point1.X <= maxx &&
		  line->Point1.Y >= miny && line->Point1.Y <= maxy)
		CreateNewRubberbandEntry (layer, line, &line->Point1);
	      else
		if (line->Point2.X >= minx && line->Point2.X <= maxx &&
		    line->Point2.Y >= miny && line->Point2.Y <= maxy)
		CreateNewRubberbandEntry (layer, line, &line->Point2);
	    }
	  );
	}
    }
}

static void
CheckPadForRat (PadTypePtr Pad)
{
  Cardinal i, group;

  i = MAX_LAYER +
    (TEST_FLAG (ONSOLDERFLAG, Pad) ? SOLDER_LAYER : COMPONENT_LAYER);
  group = GetLayerGroupNumberByNumber (i);

  RAT_LOOP (PCB->Data, 
    {
      if (line->Point1.X == Pad->Point1.X &&
	  line->Point1.Y == Pad->Point1.Y && line->group1 == group)
	CreateNewRubberbandEntry (NULL, (LineTypePtr) line, &line->Point1);
      else
	if (line->Point2.X == Pad->Point1.X &&
	    line->Point2.Y == Pad->Point1.Y && line->group2 == group)
	CreateNewRubberbandEntry (NULL, (LineTypePtr) line, &line->Point2);
      else
	if (line->Point1.X == Pad->Point2.X &&
	    line->Point1.Y == Pad->Point2.Y && line->group1 == group)
	CreateNewRubberbandEntry (NULL, (LineTypePtr) line, &line->Point1);
      else
	if (line->Point2.X == Pad->Point2.X &&
	    line->Point2.Y == Pad->Point2.Y && line->group2 == group)
	CreateNewRubberbandEntry (NULL, (LineTypePtr) line, &line->Point2);
    }
  );
}

/* ---------------------------------------------------------------------------
 * checks all visible lines. If one of the endpoints of the line lays
 * inside the pin, the line is added to the 'rubberband' list
 *
 * Square pins are handled as if they were round. Speed
 * and readability is more important then the few %
 * of faiures that are immediately recognized
 */
static void
CheckPinForRubberbandConnection (PinTypePtr Pin)
{
  register float radius, dx, dy;

  VISIBLELINE_LOOP (PCB->Data, 
    {
      if (TEST_FLAG (LOCKFLAG, line))
	continue;
      /* save sqrt computation */
      radius = (float) (Pin->Thickness + line->Thickness) / 2.0;
      radius *= radius;
      dx = line->Point1.X - Pin->X;
      dy = line->Point1.Y - Pin->Y;
      if (dx * dx + dy * dy <= radius)
	CreateNewRubberbandEntry (layer, line, &line->Point1);
      else
	{
	  dx = line->Point2.X - Pin->X;
	  dy = line->Point2.Y - Pin->Y;
	  if (dx * dx + dy * dy <= radius)
	    CreateNewRubberbandEntry (layer, line, &line->Point2);
	}
    }
  );
}

static void
CheckPinForRat (PinTypePtr Pin)
{
  RAT_LOOP (PCB->Data, 
    {
      if (line->Point1.X == Pin->X && line->Point1.Y == Pin->Y)
	CreateNewRubberbandEntry (NULL, (LineTypePtr) line, &line->Point1);
      else if (line->Point2.X == Pin->X && line->Point2.Y == Pin->Y)
	CreateNewRubberbandEntry (NULL, (LineTypePtr) line, &line->Point2);
    }
  );
}

static void
CheckLinePointForRat (LayerTypePtr Layer, PointTypePtr Point)
{
  Cardinal group = GetLayerGroupNumberByPointer (Layer);

  RAT_LOOP (PCB->Data, 
    {
      if (line->group1 == group &&
	  line->Point1.X == Point->X && line->Point1.Y == Point->Y)
	CreateNewRubberbandEntry (NULL, (LineTypePtr) line, &line->Point1);
      else
	if (line->group2 == group &&
	    line->Point2.X == Point->X && line->Point2.Y == Point->Y)
	CreateNewRubberbandEntry (NULL, (LineTypePtr) line, &line->Point2);
    }
  );
}



/* ---------------------------------------------------------------------------
 * checks all visible lines which belong to the same group as the passed line.
 * If one of the endpoints of the line lays * inside the passed line,
 * the scanned line is added to the 'rubberband' list
 */
static void
CheckLinePointForRubberbandConnection (LayerTypePtr Layer,
				       LineTypePtr Line,
				       PointTypePtr LinePoint)
{
  Cardinal group;

  /* lookup layergroup and check all visible lines in this group */
  group = GetLayerGroupNumberByPointer (Layer);
  GROUP_LOOP (group,
    {
      if (layer->On)
	{
	  register float radius;
	  register float dx;
	  register float dy;
	  /* the following code just compares the endpoints
	   * of the lines
	   */
	  LINE_LOOP (layer, 
	    {
	      /* skip the original line */
	      if (line == Line)
		continue;
	      if (TEST_FLAG (LOCKFLAG, line))
		continue;
	      /* save sqrt computation */
	      radius = (float) (Line->Thickness + line->Thickness) / 2.0;
	      radius *= radius;
	      dx = line->Point1.X - LinePoint->X;
	      dy = line->Point1.Y - LinePoint->Y;
	      if (dx * dx + dy * dy <= radius)
		{
		  CreateNewRubberbandEntry (layer, line, &line->Point1);
		  continue;
		}
	      dx = line->Point2.X - LinePoint->X;
	      dy = line->Point2.Y - LinePoint->Y;
	      if (dx * dx + dy * dy <= radius)
		{
		  CreateNewRubberbandEntry (layer, line, &line->Point2);
		  continue;
		}
	    }
	  );
	}
    }
  );
}

/* ---------------------------------------------------------------------------
 * checks all visible lines which belong to the same group as the passed line.
 * If one of the endpoints of the line lays * inside the passed line,
 * the scanned line is added to the 'rubberband' list
 */
static void
CheckPolygonForRubberbandConnection (LayerTypePtr Layer,
				     PolygonTypePtr Polygon)
{
  Cardinal group;

  /* lookup layergroup and check all visible lines in this group */
  group = GetLayerGroupNumberByPointer (Layer);
  GROUP_LOOP (group,
    {
      if (layer->On)
	{
	  BDimension thick;

	  /* the following code just stupidly compares the endpoints
	   * of the lines
	   */
	  LINE_LOOP (layer, 
	    {
	      if (TEST_FLAG (LOCKFLAG, line))
		continue;
	      if (TEST_FLAG (CLEARLINEFLAG, line))
		continue;
	      thick = (line->Thickness + 1) / 2;
	      if (IsPointInPolygon (line->Point1.X, line->Point1.Y,
				    thick, Polygon))
		CreateNewRubberbandEntry (layer, line, &line->Point1);
	      if (IsPointInPolygon (line->Point2.X, line->Point2.Y,
				    thick, Polygon))
		CreateNewRubberbandEntry (layer, line, &line->Point1);
	    }
	  );
	}
    }
  );
}

/* ---------------------------------------------------------------------------
 * lookup all lines that are connected to an object and save the
 * data to 'Crosshair.AttachedObject.Rubberband'
 * lookup is only done for visible layers
 */
void
LookupRubberbandLines (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{

  /* the function is only supported for some types
   * check all visible lines;
   * it is only necessary to check if one of the endpoints
   * is connected
   */
  switch (Type)
    {
    case ELEMENT_TYPE:
      {
	ElementTypePtr element = (ElementTypePtr) Ptr1;

	/* square pins are handled as if they are round. Speed
	 * and readability is more important then the few %
	 * of faiures that are immediately recognized
	 */
	PIN_LOOP (element, 
	  {
	    CheckPinForRubberbandConnection (pin);
	  }
	);
	PAD_LOOP (element, 
	  {
	    CheckPadForRubberbandConnection (pad);
	  }
	);
	break;
      }

    case LINE_TYPE:
      {
	LayerTypePtr layer = (LayerTypePtr) Ptr1;
	LineTypePtr line = (LineTypePtr) Ptr2;
	if (GetLayerNumber (PCB->Data, layer) < MAX_LAYER)
	  {
	    CheckLinePointForRubberbandConnection (layer, line,
						   &line->Point1);
	    CheckLinePointForRubberbandConnection (layer, line,
						   &line->Point2);
	  }
	break;
      }

    case LINEPOINT_TYPE:
      if (GetLayerNumber (PCB->Data, (LayerTypePtr) Ptr1) < MAX_LAYER)
	CheckLinePointForRubberbandConnection ((LayerTypePtr) Ptr1,
					       (LineTypePtr) Ptr2,
					       (PointTypePtr) Ptr3);
      break;

    case VIA_TYPE:
      CheckPinForRubberbandConnection ((PinTypePtr) Ptr1);
      break;

    case POLYGON_TYPE:
      if (GetLayerNumber (PCB->Data, (LayerTypePtr) Ptr1) < MAX_LAYER)
	CheckPolygonForRubberbandConnection ((LayerTypePtr) Ptr1,
					     (PolygonTypePtr) Ptr2);
      break;
    }
}

void
LookupRatLines (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  switch (Type)
    {
    case ELEMENT_TYPE:
      {
	ElementTypePtr element = (ElementTypePtr) Ptr1;

	PIN_LOOP (element, 
	  {
	    CheckPinForRat (pin);
	  }
	);
	PAD_LOOP (element, 
	  {
	    CheckPadForRat (pad);
	  }
	);
	break;
      }

    case LINE_TYPE:
      {
	LayerTypePtr layer = (LayerTypePtr) Ptr1;
	LineTypePtr line = (LineTypePtr) Ptr2;

	CheckLinePointForRat (layer, &line->Point1);
	CheckLinePointForRat (layer, &line->Point2);
	break;
      }

    case LINEPOINT_TYPE:
      CheckLinePointForRat ((LayerTypePtr) Ptr1, (PointTypePtr) Ptr3);
      break;

    case VIA_TYPE:
      CheckPinForRat ((PinTypePtr) Ptr1);
      break;
    }
}
