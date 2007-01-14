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

/* this file, mtspace.h, was written and is
 * Copyright (c) 2001 C. Scott Ananian.
 */

/* prototypes for "empty space" routines (needed for via-space tracking
 * in the auto-router.
 */

#ifndef __MTSPACE_INCLUDED__
#define __MTSPACE_INCLUDED__
/* mtspace data structures are built on r-trees. */

#include "global.h"
#include "vector.h"             /* for vector_t in mtspace_query_rect prototype */

typedef struct mtspace mtspace_t;
typedef enum
{ FIXED, ODD, EVEN } mtspace_type_t;
typedef struct vetting vetting_t;

/* create an "empty space" representation with a shrunken boundary */
mtspace_t *mtspace_create (void);
/* destroy an "empty space" representation. */
void mtspace_destroy (mtspace_t ** mtspacep);

/* -- mutation -- */

/* add a space-filler to the empty space representation.  The given box
 * should *not* be bloated; it should be "true".  The feature will fill
 * *at least* a radius of keepaway around it;
 */
void mtspace_add (mtspace_t * mtspace,
                  const BoxType * box, mtspace_type_t which, BDimension
                  keepaway);
/* remove a space-filler from the empty space representation.  The given box
 * should *not* be bloated; it should be "true".  The feature will fill
 * *at least* a radius of keepaway around it;
 */
void mtspace_remove (mtspace_t * mtspace,
                     const BoxType * box, mtspace_type_t which,
                     BDimension keepaway);


vetting_t *mtspace_query_rect (mtspace_t * mtspace, const BoxType * region,
                               BDimension radius, BDimension keepaway,
                               vetting_t * work,
                               vector_t * free_space_vec,
                               vector_t * lo_conflict_space_vec,
                               vector_t * hi_conflict_space_vec,
                               Boolean is_odd, Boolean with_conflicts);

void mtsFreeWork (vetting_t **);
int mtsBoxCount (vetting_t *);
#endif /* ! __MTSPACE_INCLUDED__ */
