/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1998,1999,2000,2001 harry eaton
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
 *  harry eaton, 6697 Buttonhole Ct, Columbia, MD 21044 USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

/* this file, kdtree.h, was written and is
 * Copyright (c) 2001 C. Scott Ananian.
 */

/* prototypes for k-d tree routines.
 */

#ifndef __KDTREE_INCLUDED__
#define __KDTREE_INCLUDED__

#include "global.h"

typedef struct kdtree kdtree_t;

/* create a kd tree from the list of boxes.  if 'manage' is true, then
 * the tree will take ownership of 'boxlist' and free it when the tree
 * is destroyed. */
kdtree_t * kd_create_tree(const BoxType * boxlist[], int N, int manage);
/* destroy a kd_tree */
void kd_destroy_tree(kdtree_t **kdtree);

/* rebuild the kd-tree (in O(N ln N) time) in order to perfectly balance it */
void kd_optimize_tree(kdtree_t *kdtree);

/* -- mutation -- */
void kd_delete_node(kdtree_t *kdtree, const BoxType *which);
void kd_insert_node(kdtree_t *kdtree, const BoxType *which, int manage);

/* generic search routine */
/* region_in_search should return true if "what you're looking for" is
 * within the specified region; regions, like rectangles, are closed on
 * top and left and open on bottom and right.
 * rectangle_in_region should return true if the given rectangle is
 * "what you're looking for".
 * The search will find all rectangles matching the criteria given
 * by region_in_search and rectangle_in_region and return a count of
 * how many things rectangle_in_region returned true for.  Look at
 * the implementation of kd_region_is_empty for ideas on how to
 * short-circuit the search if that is the desired behavior. */
int kd_search(kdtree_t *kdtree, const BoxType * starting_region,
	      int (*region_in_search)(const BoxType * region, void *cl),
	      int (*rectangle_in_region)(const BoxType * box, void *cl),
	      void *closure);
/* -- special-purpose searches build upon kd_search -- */
/* return 0 if there are any rectangles in the given region. */
int kd_region_is_empty(kdtree_t *kdtree, const BoxType * region);
/* dump a listing of all rectangles in the kd-tree to the stream */
void kd_print(kdtree_t *kdtree, FILE *stream);

#endif
