/*!
 * \file src/box.h
 *
 * \brief Random box-related utilities.
 *
 * \author This file, box.h, was written and is
 * Copyright (c) 2001 C. Scott Ananian.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
 *
 * Copyright (C) 1998,1999,2000,2001 harry eaton
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Contact addresses for paper mail and Email:
 * harry eaton, 6697 Buttonhole Ct, Columbia, MD 21044 USA
 * haceaton@aplcomm.jhuapl.edu
 *
 */

#ifndef PCB_BOX_H
#define PCB_BOX_H

#include <assert.h>
#include "global.h"

#include "misc.h"

typedef enum
{ NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3, NE = 4, SE = 5, SW = 6, NW =
    7, ALL = 8 } direction_t;

/*!
 * \brief Rotates box 90-degrees cw.
 *
 * That's a strange rotation!
 */
#define ROTATEBOX_CW(box) { Coord t;\
    t = (box).X1; (box).X1 = -(box).Y2; (box).Y2 = (box).X2;\
    (box).X2 = -(box).Y1; (box).Y1 = t;\
}
#define ROTATEBOX_TO_NORTH(box, dir) do { Coord t;\
  switch(dir) {\
  case EAST: \
   t = (box).X1; (box).X1 = (box).Y1; (box).Y1 = -(box).X2;\
   (box).X2 = (box).Y2; (box).Y2 = -t; break;\
  case SOUTH: \
   t = (box).X1; (box).X1 = -(box).X2; (box).X2 = -t;\
   t = (box).Y1; (box).Y1 = -(box).Y2; (box).Y2 = -t; break;\
  case WEST: \
   t = (box).X1; (box).X1 = -(box).Y2; (box).Y2 = (box).X2;\
   (box).X2 = -(box).Y1; (box).Y1 = t; break;\
  case NORTH: break;\
  default: assert(0);\
  }\
  } while (0)
#define ROTATEBOX_FROM_NORTH(box, dir) do { Coord t;\
  switch(dir) {\
  case WEST: \
   t = (box).X1; (box).X1 = (box).Y1; (box).Y1 = -(box).X2;\
   (box).X2 = (box).Y2; (box).Y2 = -t; break;\
  case SOUTH: \
   t = (box).X1; (box).X1 = -(box).X2; (box).X2 = -t;\
   t = (box).Y1; (box).Y1 = -(box).Y2; (box).Y2 = -t; break;\
  case EAST: \
   t = (box).X1; (box).X1 = -(box).Y2; (box).Y2 = (box).X2;\
   (box).X2 = -(box).Y1; (box).Y1 = t; break;\
  case NORTH: break;\
  default: assert(0);\
  }\
  } while (0)

/* to avoid overflow, we calculate centers this way */
#define CENTER_X(b) ((b).X1 + ((b).X2 - (b).X1)/2)
#define CENTER_Y(b) ((b).Y1 + ((b).Y2 - (b).Y1)/2)
/* some useful box utilities. */

typedef struct cheap_point
{
  Coord X, Y;
} CheapPointType;


/* note that boxes are closed on top and left and open on bottom and right. */
/* this means that top-left corner is in box, *but bottom-right corner is
 * not*.  */
static inline bool
point_in_box (const BoxType * box, Coord X, Coord Y)
{
  return (X >= box->X1) && (Y >= box->Y1) && (X < box->X2) && (Y < box->Y2);
}

static inline bool
point_in_closed_box (const BoxType * box, Coord X, Coord Y)
{
  return (X >= box->X1) && (Y >= box->Y1) && (X <= box->X2) && (Y <= box->Y2);
}

static inline bool
box_is_good (const BoxType * b)
{
  return (b->X1 < b->X2) && (b->Y1 < b->Y2);
}

static inline bool
box_intersect (const BoxType * a, const BoxType * b)
{
  return
    (a->X1 < b->X2) && (b->X1 < a->X2) && (a->Y1 < b->Y2) && (b->Y1 < a->Y2);
}

static inline CheapPointType
closest_point_in_box (const CheapPointType * from, const BoxType * box)
{
  CheapPointType r;
  assert (box->X1 < box->X2 && box->Y1 < box->Y2);
  r.X =
    (from->X < box->X1) ? box->X1 : (from->X >
				     box->X2 - 1) ? box->X2 - 1 : from->X;
  r.Y =
    (from->Y < box->Y1) ? box->Y1 : (from->Y >
				     box->Y2 - 1) ? box->Y2 - 1 : from->Y;
  assert (point_in_box (box, r.X, r.Y));
  return r;
}

static inline bool
box_in_box (const BoxType * outer, const BoxType * inner)
{
  return
    (outer->X1 <= inner->X1) && (inner->X2 <= outer->X2) &&
    (outer->Y1 <= inner->Y1) && (inner->Y2 <= outer->Y2);
}

static inline BoxType
clip_box (const BoxType * box, const BoxType * clipbox)
{
  BoxType r;
  assert (box_intersect (box, clipbox));
  r.X1 = MAX (box->X1, clipbox->X1);
  r.X2 = MIN (box->X2, clipbox->X2);
  r.Y1 = MAX (box->Y1, clipbox->Y1);
  r.Y2 = MIN (box->Y2, clipbox->Y2);
  assert (box_in_box (clipbox, &r));
  return r;
}

static inline BoxType
shrink_box (const BoxType * box, Coord amount)
{
  BoxType r = *box;
  r.X1 += amount;
  r.Y1 += amount;
  r.X2 -= amount;
  r.Y2 -= amount;
  return r;
}

static inline BoxType
bloat_box (const BoxType * box, Coord amount)
{
  return shrink_box (box, -amount);
}

/*!
 * \brief Construct a minimum box that touches the input box at the
 * center.
 */
static inline BoxType
box_center (const BoxType * box)
{
  BoxType r;
  r.X1 = box->X1 + (box->X2 - box->X1)/2;
  r.X2 = r.X1 + 1;
  r.Y1 = box->Y1 + (box->Y2 - box->Y1)/2;
  r.Y2 = r.Y1 + 1;
  return r;
}

/*!
 * \brief Construct a minimum box that touches the input box at the
 * corner.
 */
static inline BoxType
box_corner (const BoxType * box)
{
  BoxType r;
  r.X1 = box->X1;
  r.X2 = r.X1 + 1;
  r.Y1 = box->Y1;
  r.Y2 = r.Y1 + 1;
  return r;
}

/*!
 * \brief Construct a box that holds a single point.
 */
static inline BoxType
point_box (Coord X, Coord Y)
{
  BoxType r;
  r.X1 = X;
  r.X2 = X + 1;
  r.Y1 = Y;
  r.Y2 = Y + 1;
  return r;
}

/*!
 * \brief Close a bounding box by pushing its upper right corner.
 */
static inline void
close_box (BoxType * r)
{
  r->X2++;
  r->Y2++;
}

/*!
 * \brief Return the square of the minimum distance from a point to some
 * point inside a box.
 *
 * The box is half-closed!\n
 * That is, the top-left corner is considered in the box, but the
 * bottom-right corner is not.
 */
static inline double
dist2_to_box (const CheapPointType * p, const BoxType * b)
{
  CheapPointType r = closest_point_in_box (p, b);
  return Distance (r.X, r.Y, p->X, p->Y);
}

#endif /* __BOX_H_INCLUDED__ */
