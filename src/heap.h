/*!
 * \file src/heap.h
 *
 * \brief Prototypes for heap routines.
 *
 * This file, heap.h, was written and is
 *
 * Copyright (c) 2001 C. Scott Ananian
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
 */

#ifndef PCB_HEAP_H
#define PCB_HEAP_H

#include "global.h"

/*!
 * \brief Type of heap costs.
 */
typedef double cost_t;
/*!
 * \brief What a heap looks like.
 */
typedef struct heap_struct heap_t;

heap_t *heap_create ();
void heap_destroy (heap_t ** heap);
void heap_free (heap_t * heap, void (*funcfree) (void *));

/* -- mutation -- */
void heap_insert (heap_t * heap, cost_t cost, void *data);
void *heap_remove_smallest (heap_t * heap);
void *heap_replace (heap_t * heap, cost_t cost, void *data);

/* -- interrogation -- */
int heap_is_empty (heap_t * heap);
int heap_size (heap_t * heap);

#endif /* PCB_HEAP_H */
