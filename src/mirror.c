/*!
 * \file src/mirror.c
 *
 * \brief Functions used to change the mirror flag of an object.
 *
 * An undo operation is not implemented because it's easy to
 * recover an object.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

#include <stdlib.h>

#include "global.h"

#include "data.h"
#include "draw.h"
#include "mirror.h"
#include "misc.h"
#include "polygon.h"
#include "search.h"
#include "select.h"
#include "set.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/*!
 * \brief Mirrors the coordinates of an element.
 *
 * An additional offset is passed.
 */
void
MirrorElementCoordinates (DataType *Data, ElementType *Element,
			  Coord yoff)
{
  r_delete_element (Data, Element);
  ELEMENTLINE_LOOP (Element);
  {
    line->Point1.X = SWAP_X (line->Point1.X);
    line->Point1.Y = SWAP_Y (line->Point1.Y) + yoff;
    line->Point2.X = SWAP_X (line->Point2.X);
    line->Point2.Y = SWAP_Y (line->Point2.Y) + yoff;
  }
  END_LOOP;
  PIN_LOOP (Element);
  {
    RestoreToPolygon (Data, PIN_TYPE, Element, pin);
    pin->X = SWAP_X (pin->X);
    pin->Y = SWAP_Y (pin->Y) + yoff;
  }
  END_LOOP;
  PAD_LOOP (Element);
  {
    RestoreToPolygon (Data, PAD_TYPE, Element, pad);
    pad->Point1.X = SWAP_X (pad->Point1.X);
    pad->Point1.Y = SWAP_Y (pad->Point1.Y) + yoff;
    pad->Point2.X = SWAP_X (pad->Point2.X);
    pad->Point2.Y = SWAP_Y (pad->Point2.Y) + yoff;
    TOGGLE_FLAG (ONSOLDERFLAG, pad);
  }
  END_LOOP;
  ARC_LOOP (Element);
  {
    arc->X = SWAP_X (arc->X);
    arc->Y = SWAP_Y (arc->Y) + yoff;
    arc->StartAngle = SWAP_ANGLE (arc->StartAngle);
    arc->Delta = SWAP_DELTA (arc->Delta);
  }
  END_LOOP;
  ELEMENTTEXT_LOOP (Element);
  {
    text->X = SWAP_X (text->X);
    text->Y = SWAP_Y (text->Y) + yoff;
    TOGGLE_FLAG (ONSOLDERFLAG, text);
  }
  END_LOOP;
  Element->MarkX = SWAP_X (Element->MarkX);
  Element->MarkY = SWAP_Y (Element->MarkY) + yoff;

  /* now toggle the solder-side flag */
  TOGGLE_FLAG (ONSOLDERFLAG, Element);
  /* this inserts all of the rtree data too */
  SetElementBoundingBox (Data, Element, Settings.Font);
  ClearFromPolygon (Data, ELEMENT_TYPE, Element, Element);
}
