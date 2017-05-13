/*!
 * \file src/rtree.h
 *
 * \brief Prototypes for r-tree routines.
 *
 * \author This file, rtree.h, was written and is Copyright (c) 2004
 * harry eaton, it's based on C. Scott's kdtree.h template.
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
 *
 * harry eaton, 6697 Buttonhole Ct, Columbia, MD 21044 USA
 *
 * haceaton@aplcomm.jhuapl.edu
 */


#ifndef PCB_RTREE_H
#define PCB_RTREE_H

#include "global.h"


rtree_t *r_create_tree (const BoxType * boxlist[], int N, int manage);
void r_destroy_tree (rtree_t ** rtree);

bool r_delete_entry (rtree_t * rtree, const BoxType * which);
void r_insert_entry (rtree_t * rtree, const BoxType * which, int manage);
int r_search (rtree_t * rtree, const BoxType * starting_region,
	      int (*region_in_search) (const BoxType * region, void *cl),
	      int (*rectangle_in_region) (const BoxType * box, void *cl),
	      void *closure);
static inline int r_search_pt (rtree_t * rtree, const PointType * pt,
	      int radius,
	      int (*region_in_search) (const BoxType * region, void *cl),
	      int (*rectangle_in_region) (const BoxType * box, void *cl),
	      void *closure)
{
  BoxType box;

  box.X1 = pt->X - radius;
  box.X2 = pt->X + radius;
  box.Y1 = pt->Y - radius;
  box.Y2 = pt->Y + radius;

  return r_search(rtree, &box, region_in_search, rectangle_in_region, closure);
}
int r_region_is_empty (rtree_t * rtree, const BoxType * region);
void __r_dump_tree (struct rtree_node *, int);

#endif
