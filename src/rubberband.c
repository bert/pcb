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
#include "rtree.h"
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
static int  rubber_callback (const BoxType *b, void *cl);

struct rubber_info
{
  int radius;
  Location X, Y;
  LineTypePtr line;
  BoxType box;
  LayerTypePtr layer;
};

static int
rubber_callback (const BoxType *b, void *cl)
{
  LineTypePtr line = (LineTypePtr) b;
  struct rubber_info *i = (struct rubber_info *)cl;
  float x, y;

  if (TEST_FLAG(LOCKFLAG, line))
    return 0;
  if (line == i->line)
    return 0;
  if (i->radius == 0) /* rectangular search region */
    {
      BDimension t = line->Thickness/2;
      if (line->Point1.X + t >= i->box.X1 && line->Point1.X - t <= i->box.X2 &&
          line->Point1.Y + t >= i->box.Y1 && line->Point1.Y - t <= i->box.Y2)
        {
          CreateNewRubberbandEntry (i->layer, line, &line->Point1);
          return 1;
        }
      if (line->Point2.X + t >= i->box.X1 && line->Point2.X - t <= i->box.X2 &&
          line->Point2.Y + t >= i->box.Y1 && line->Point2.Y - t <= i->box.Y2)
        {
          CreateNewRubberbandEntry (i->layer, line, &line->Point2);
          return 1;
        }
      return 0;
    }
   /* circular search region */
  x = (i->X - line->Point1.X);
  x *= x;
  y = (i->Y - line->Point1.Y);
  y *= y;
  x = x + y - (line->Thickness * line->Thickness);
  if (x < (i->radius * ( i->radius + 2 * line->Thickness)))
    {
      CreateNewRubberbandEntry (i->layer, line, &line->Point1);
      return 1;
    }
  x = (i->X - line->Point2.X);
  x *= x;
  y = (i->Y - line->Point2.Y);
  y *= y;
  x = x + y - (line->Thickness * line->Thickness);
  if (x < (i->radius * ( i->radius + 2 * line->Thickness)))
    {
      CreateNewRubberbandEntry (i->layer, line, &line->Point2);
      return 1;
    }
  return 0;
}
  
/* ---------------------------------------------------------------------------
 * checks all visible lines which belong to the same layergroup as the
 * passed pad. If one of the endpoints of the line lays inside the pad,
 * the line is added to the 'rubberband' list
 */
static void
CheckPadForRubberbandConnection (PadTypePtr Pad)
{
  BDimension half = Pad->Thickness / 2;
  Cardinal i, group;
  struct rubber_info info;

  info.box.X1 = MIN (Pad->Point1.X, Pad->Point2.X) - half;
  info.box.Y1 = MIN (Pad->Point1.Y, Pad->Point2.Y) - half;
  info.box.X2 = MAX (Pad->Point1.X, Pad->Point2.X) + half;
  info.box.Y2 = MAX (Pad->Point1.Y, Pad->Point2.Y) + half;
  info.radius = 0;
  info.line = NULL;
  i = MAX_LAYER +
    (TEST_FLAG (ONSOLDERFLAG, Pad) ? SOLDER_LAYER : COMPONENT_LAYER);
  group = GetLayerGroupNumberByNumber (i);

  /* check all visible layers in the same group */
  GROUP_LOOP(group,
    {
      /* check all visible lines of the group member */
      info.layer = layer;
      if (info.layer->On)
	{
	  r_search(info.layer->line_tree, &info.box, NULL, rubber_callback, &info);
	}
    }
  );
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
  struct rubber_info info;
  Cardinal n;
  BDimension t = Pin->Thickness/2;

  info.box.X1 = Pin->X - t;
  info.box.X2 = Pin->X + t;
  info.box.Y1 = Pin->Y - t;
  info.box.Y2 = Pin->Y + t;
  info.line = NULL;
  if (TEST_FLAG(SQUAREFLAG, Pin))
    info.radius = 0;
  else
    {
      info.radius = t;
      info.X = Pin->X;
      info.Y = Pin->Y;
    }

  for (n = 0; n < MAX_LAYER; n++)
    {
      info.layer = LAYER_PTR(n);
      r_search (info.layer->line_tree, &info.box, NULL, rubber_callback, &info);
    }
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
  struct rubber_info info;
  BDimension t = Line->Thickness/2;

  /* lookup layergroup and check all visible lines in this group */
  info.radius = Line->Thickness;
  info.box.X1 = LinePoint->X - t;
  info.box.X2 = LinePoint->X + t;;
  info.box.Y1 = LinePoint->Y - t;
  info.box.Y2 = LinePoint->Y + t;
  info.line = Line;
  info.X = LinePoint->X;
  info.Y = LinePoint->Y;
  group = GetLayerGroupNumberByPointer (Layer);
  GROUP_LOOP (group,
    {
      /* check all visible lines of the group member */
      if (layer->On)
	{
	  info.layer = layer;
	  r_search(layer->line_tree, &info.box, NULL, rubber_callback, &info);
	}
    }
  );
}

/* ---------------------------------------------------------------------------
 * checks all visible lines which belong to the same group as the passed polygon.
 * If one of the endpoints of the line lays inside the passed polygon,
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
