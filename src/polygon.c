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

/* special polygon editing routines
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <memory.h>

#include "global.h"

#include "create.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "find.h"
#include "misc.h"
#include "move.h"
#include "polygon.h"
#include "remove.h"
#include "search.h"
#include "set.h"
#include "undo.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * local prototypes
 */
static Boolean DoPIPFlags (PinTypePtr, ElementTypePtr,
			   LayerTypePtr, PolygonTypePtr, int);

static int new_flags, mask;

/* --------------------------------------------------------------------------
 * remove redundant polygon points. Any point that lies on the straight
 * line between the points on either side of it is redundant.
 * returns true if any points are removed
 */
Boolean
RemoveExcessPolygonPoints (LayerTypePtr Layer, PolygonTypePtr Polygon)
{
  PointTypePtr pt1, pt2, pt3;
  Cardinal n;
  LineType line;
  Boolean changed = False;

  if (Undoing ())
    return (False);
  /* there are always at least three points in a polygon */
  pt1 = &Polygon->Points[Polygon->PointN - 1];
  pt2 = &Polygon->Points[0];
  pt3 = &Polygon->Points[1];
  for (n = 0; n < Polygon->PointN; n++, pt1++, pt2++, pt3++)
    {
      /* wrap around polygon */
      if (n == 1)
	pt1 = &Polygon->Points[0];
      if (n == Polygon->PointN - 1)
	pt3 = &Polygon->Points[0];
      line.Point1 = *pt1;
      line.Point2 = *pt3;
      line.Thickness = 0;
      if (IsPointOnLine ((float) pt2->X, (float) pt2->Y, 0.0, &line))
	{
	  RemoveObject (POLYGONPOINT_TYPE, (void *) Layer, (void *) Polygon,
			(void *) pt2);
	  changed = True;
	}
    }
  return (changed);
}

/* ---------------------------------------------------------------------------
 * returns the index of the polygon point which is the end
 * point of the segment with the lowest distance to the passed
 * coordinates
 */
Cardinal
GetLowestDistancePolygonPoint (PolygonTypePtr Polygon, Location X, Location Y)
{
  float mindistance = MAX_COORD, length, distance, temp;
  PointTypePtr ptr1 = &Polygon->Points[Polygon->PointN - 1],
    ptr2 = &Polygon->Points[0];
  Cardinal n, result = 0;

  /* get segment next to crosshair location;
   * it is determined by the last point of it
   */
  for (n = 0; n < Polygon->PointN; n++, ptr2++)
    {

      distance = ptr2->X - ptr1->X;
      temp = ptr2->Y - ptr1->Y;
      length = sqrt(distance * distance + temp * temp);
      if (length != 0.0)
	{
	  distance = fabs ((temp * (Crosshair.X - ptr1->X) -
			    distance * (Crosshair.Y - ptr1->Y)) / length);
	  if (distance < mindistance)
	    {
	      mindistance = distance;
	      result = n;
	    }
	}
      ptr1 = ptr2;
    }
  return (result);
}

/* ---------------------------------------------------------------------------
 * go back to the  previous point of the polygon
 */
void
GoToPreviousPoint (void)
{
  switch (Crosshair.AttachedPolygon.PointN)
    {
      /* do nothing if mode has just been entered */
    case 0:
      break;

      /* reset number of points and 'LINE_MODE' state */
    case 1:
      Crosshair.AttachedPolygon.PointN = 0;
      Crosshair.AttachedLine.State = STATE_FIRST;
      addedLines = 0;
      break;

      /* back-up one point */
    default:
      {
	PointTypePtr points = Crosshair.AttachedPolygon.Points;
	Cardinal n = Crosshair.AttachedPolygon.PointN - 2;

	Crosshair.AttachedPolygon.PointN--;
	Crosshair.AttachedLine.Point1.X = points[n].X;
	Crosshair.AttachedLine.Point1.Y = points[n].Y;
	break;
      }
    }
}

/* ---------------------------------------------------------------------------
 * close polygon if possible
 */
void
ClosePolygon (void)
{
  Cardinal n = Crosshair.AttachedPolygon.PointN;

  /* check number of points */
  if (n >= 3)
    {
      /* if 45 degree lines are what we want do a quick check
       * if closing the polygon makes sense
       */
      if (!TEST_FLAG (ALLDIRECTIONFLAG, PCB))
	{
	  BDimension dx, dy;

	  dx = abs (Crosshair.AttachedPolygon.Points[n - 1].X -
		    Crosshair.AttachedPolygon.Points[0].X);
	  dy = abs (Crosshair.AttachedPolygon.Points[n - 1].Y -
		    Crosshair.AttachedPolygon.Points[0].Y);
	  if (!(dx == 0 || dy == 0 || dx == dy))
	    {
	      Message
		("Cannot close polygon because 45 degree lines are requested.\n");
	      return;
	    }
	}
      CopyAttachedPolygonToLayer ();
      Draw ();
    }
  else
    Message ("a polygon has to have at least 3 points\n");
}

/* ---------------------------------------------------------------------------
 * moves the data of the attached (new) polygon to the current layer
 */
void
CopyAttachedPolygonToLayer (void)
{
  PolygonTypePtr polygon;
  int saveID;

  /* move data to layer and clear attached struct */
  polygon = CreateNewPolygon (CURRENT, NOFLAG);
  saveID = polygon->ID;
  *polygon = Crosshair.AttachedPolygon;
  polygon->ID = saveID;
  SET_FLAG (CLEARPOLYFLAG, polygon);
  memset (&Crosshair.AttachedPolygon, 0, sizeof (PolygonType));
  SetPolygonBoundingBox (polygon);
  UpdatePIPFlags (NULL, NULL, CURRENT, polygon, True);
  DrawPolygon (CURRENT, polygon, 0);
  SetChangedFlag (True);

  /* reset state of attached line */
  Crosshair.AttachedLine.State = STATE_FIRST;
  addedLines = 0;

  /* add to undo list */
  AddObjectToCreateUndoList (POLYGON_TYPE, CURRENT, polygon, polygon);
  IncrementUndoSerialNumber ();
}

/* ---------------------------------------------------------------------------
 *  Updates the pin-in-polygon flags
 *  if called with Element == NULL, seach all pins
 *  if called with Pin == NULL, search all pins on element
 *  if called with Polygon == NULL, search all polygons
 *  if called with Layer == NULL, search all layers
 */
void
UpdatePIPFlags (PinTypePtr Pin, ElementTypePtr Element,
		LayerTypePtr Layer, PolygonTypePtr Polygon, Boolean AddUndo)
{

  if (Element == NULL)
    {
      ALLPIN_LOOP (PCB->Data, 
	{
	  UpdatePIPFlags (pin, element, Layer, Polygon, AddUndo);
	}
      );
      VIA_LOOP (PCB->Data, 
	{
	  UpdatePIPFlags (via, (ElementTypePtr) via, Layer, Polygon, AddUndo);
	}
      );
    }
  else if (Pin == NULL)
    {
      PIN_LOOP (Element, 
	{
	  UpdatePIPFlags (pin, Element, Layer, Polygon, AddUndo);
	}
      );
    }
  else if (Layer == NULL)
    {
      Cardinal l;
      for (l = 0; l < MAX_LAYER; l++)
	UpdatePIPFlags (Pin, Element, LAYER_PTR (l), Polygon, AddUndo);
    }
  else
    {
      int old_flags = Pin->Flags;
      mask = L0PIPFLAG << GetLayerNumber (PCB->Data, Layer);
      /* assume no pierce on this layer */
      if (Polygon == NULL)
	{
	  Pin->Flags &= ~(mask | WARNFLAG);
	  POLYGON_LOOP (Layer, 
	    {
	      if (TEST_FLAG (CLEARPOLYFLAG, polygon))
		if (DoPIPFlags (Pin, Element, Layer, polygon, mask))
		  break;
	    }
	  );
	}
      /* if already piercing, no need to check the new poly */
      else if (Pin->Flags & mask)
	return;
      else
	{
	  if (TEST_FLAG (CLEARPOLYFLAG, Polygon))
	    DoPIPFlags (Pin, Element, Layer, Polygon, mask);
	}
      new_flags = Pin->Flags;
      if (new_flags != old_flags)
	{
	  Pin->Flags = old_flags;
	  if (AddUndo)
	    {
	      if (Pin == (PinTypePtr) Element)
		AddObjectToFlagUndoList (VIA_TYPE, Pin, Pin, Pin);
	      else
		AddObjectToFlagUndoList (PIN_TYPE, Element, Pin, Pin);
	    }
	  Pin->Flags = new_flags;
	}
    }
}

static Boolean
DoPIPFlags (PinTypePtr Pin, ElementTypePtr Element,
	    LayerTypePtr Layer, PolygonTypePtr Polygon, int LayerPIPFlag)
{
  float wide;
  
  if (TEST_FLAG(SQUAREFLAG, Pin))
    wide = (Pin->Thickness + Pin->Clearance ) * SQRT2OVER2;
  else
    wide = (Pin->Thickness + Pin->Clearance ) * 0.5;
  if (IsPointInPolygon (Pin->X, Pin->Y, wide, Polygon))
    {
      if (TEST_FLAG(HOLEFLAG, Pin) && !TEST_FLAG(WARNFLAG, Pin))
        {
	  Message("WARNING Unplated hole piercing or too close to polygon\n");
	  SET_FLAG(WARNFLAG, Pin);
	}
      SET_FLAG (LayerPIPFlag, Pin);
      return True;
    }
  return False;
}

/* find everything clearing an actual polygon and call the callback function for it */

int PolygonPlows (int group, int (*any_call)(int type, void *ptr1, void *ptr2, void *ptr3))
{
  int entry, PIPflag = 0;
  int r;

  for (entry = 0; entry < PCB->LayerGroups.Number[group]; entry++)
    {
      LayerTypePtr layer;
      Cardinal number;

      number = PCB->LayerGroups.Entries[group][entry];
      if (number >= MAX_LAYER)
        continue;

      layer = LAYER_PTR (number);
      if (!layer->PolygonN)
        continue;
      PIPflag |= L0PIPFLAG << number;

      LINE_LOOP (layer, 
      {
	if (TEST_FLAG (CLEARLINEFLAG, line))
	  {
	    CLEAR_FLAG (CLEARLINEFLAG, line);
	    line->Thickness += line->Clearance;
	    /* now see if it would touch any polygon */
	    POLYGON_LOOP (layer, 
	      {
	        if (IsLineInPolygon(line, polygon))
		  {
	            line->Thickness -= line->Clearance;
                    r = any_call (LINE_TYPE, layer, line, line);
	            line->Thickness += line->Clearance;
		    break;
		  }
	      }
	    );
	    line->Thickness -= line->Clearance;
	    SET_FLAG (CLEARLINEFLAG, line);
            if (r)
             return r;
	  }
      }
    ); /* end of LINE_LOOP */
    ARC_LOOP (layer, 
      {
	if (TEST_FLAG (CLEARLINEFLAG, arc))
	  {
	    CLEAR_FLAG (CLEARLINEFLAG, arc);
	    arc->Thickness += arc->Clearance;
	    POLYGON_LOOP(layer,
	      {
	        if (IsArcInPolygon(arc, polygon))
		  {
	            arc->Thickness -= arc->Clearance;
		    r = any_call (ARC_TYPE, layer, arc, arc);
	            arc->Thickness += arc->Clearance;
		    break;
		  }
	      }
            );
            arc->Thickness -= arc->Clearance; 
            SET_FLAG (CLEARLINEFLAG, arc);
            if (r)
              return r;
          }
       }
     ); /* end of ARC_LOOP */
    }  /* end of entry loop */
  ALLPIN_LOOP (PCB->Data, 
    {
      if (!TEST_FLAG(HOLEFLAG, pin) && TEST_FLAG (PIPflag, pin))
        {
          r = any_call (PIN_TYPE, element, pin, pin);
        } 
      if (r)
       return r;
    }
  );
  VIA_LOOP (PCB->Data, 
    {
      if (!TEST_FLAG(HOLEFLAG, via) && TEST_FLAG (PIPflag, via))
        {
          r = any_call (VIA_TYPE, via, via, via);
        }
      if (r)
        return r;
    }
  );
  /* pads are a bitch */
      Cardinal component = GetLayerGroupNumberByNumber (MAX_LAYER + COMPONENT_LAYER);
      Cardinal solder = GetLayerGroupNumberByNumber (MAX_LAYER + SOLDER_LAYER);
      if (group == component || group == solder)
        {
          ALLPAD_LOOP (PCB->Data,
            {
	      if ((TEST_FLAG (ONSOLDERFLAG, pad)) == (group == solder ? True : False))
                {
                   CLEAR_FLAG(CLEARLINEFLAG, pad);
                   pad->Thickness += pad->Clearance;
                   for (entry = 0; entry < PCB->LayerGroups.Number[group]; entry++)
                     {
                        LayerTypePtr layer;
                        Cardinal number;
		        /* commas aren't good inside the LOOP macro */
                        Location x1;
		        Location x2;
		        Location y1;
		        Location y2;
		        BDimension wid = pad->Thickness/2;
                        number = PCB->LayerGroups.Entries[group][entry];
                        if (number >= MAX_LAYER)
                          continue;
		        layer = LAYER_PTR(number);
		        POLYGON_LOOP(layer,
		          {
	                    if (TEST_FLAG(SQUAREFLAG, pad))
	                      {
		                x1 = MIN(pad->Point1.X, pad->Point2.X) - wid;
		                y1 = MIN(pad->Point1.Y, pad->Point2.Y) - wid;
		                x2 = MAX(pad->Point1.X, pad->Point2.X) + wid;
		                y2 = MAX(pad->Point1.Y, pad->Point2.Y) + wid;
			      }
	                    if ((TEST_FLAG(SQUAREFLAG, pad) && 
		                 IsRectangleInPolygon (x1, y1, x2, y2, polygon)) ||
			        (!TEST_FLAG(SQUAREFLAG, pad) &&
	                         IsLineInPolygon ((LineTypePtr)pad, polygon)))
		              {
			        pad->Thickness -= pad->Clearance;
	                        r = any_call (PAD_TYPE, element, pad, pad);
			        pad->Thickness += pad->Clearance;
			        goto twice_break;
                              }
		          }
		        ); // end of POLYGON_LOOP 
	             } // end of entry loop
twice_break:
                   pad->Thickness -= pad->Clearance;
                   if (r)
                     return r;
                 } // end of solderside test
             }
          ); // end of ALLPAD_LOOP
        } /* end of group test */
  return 0;
}
