/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is University of Southern
 * California.
 *
 * Contributor(s):
 *	Carl D. Worth <cworth@cworth.org>
 */

/*
 * These definitions are solely for use by the implementation of cairo
 * and constitute no kind of standard.  If you need any of these
 * functions, please drop me a note.  Either the library needs new
 * functionality, or there's a way to do what you need using the
 * existing published interfaces. cworth@cworth.org
 */

#ifndef _BORAST_TRAPS_PRIVATE_H_
#define _BORAST_TRAPS_PRIVATE_H_

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "borast-types-private.h"

typedef struct _borast_traps {
    borast_status_t status;

    const borast_box_t *limits;
    int num_limits;

    unsigned int maybe_region : 1; /* hint: 0 implies that it cannot be */
    unsigned int has_intersections : 1;
    unsigned int is_rectilinear : 1;
    unsigned int is_rectangular : 1;

    int num_traps;
    int traps_size;
    borast_trapezoid_t *traps;
    borast_trapezoid_t  traps_embedded[16];
} borast_traps_t;


/* borast-traps.c */
borast_private void
_borast_traps_init (borast_traps_t *traps);

borast_private void
_borast_traps_limit (borast_traps_t	*traps,
		    const borast_box_t	*boxes,
		    int			 num_boxes);

borast_private borast_status_t
_borast_traps_init_boxes (borast_traps_t	    *traps,
			 const borast_box_t    *boxes,
			 int		     num_boxes);

borast_private void
_borast_traps_clear (borast_traps_t *traps);

borast_private void
_borast_traps_fini (borast_traps_t *traps);

#define _borast_traps_status(T) (T)->status

borast_private void
_borast_traps_translate (borast_traps_t *traps, int x, int y);

borast_private borast_status_t
_borast_traps_tessellate_rectangle (borast_traps_t *traps,
				   const borast_point_t *top_left,
				   const borast_point_t *bottom_right);

borast_private void
_borast_traps_add_trap (borast_traps_t *traps,
		       borast_fixed_t top, borast_fixed_t bottom,
		       borast_line_t *left, borast_line_t *right);

borast_private borast_status_t
_borast_bentley_ottmann_tessellate_rectilinear_polygon (borast_traps_t	 *traps,
						       const borast_polygon_t *polygon,
						       borast_fill_rule_t	  fill_rule);

borast_private borast_status_t
_borast_bentley_ottmann_tessellate_polygon (borast_traps_t         *traps,
					   const borast_polygon_t *polygon);

borast_private borast_status_t
_borast_bentley_ottmann_tessellate_traps (borast_traps_t *traps,
					 borast_fill_rule_t fill_rule);

borast_private borast_status_t
_borast_bentley_ottmann_tessellate_rectangular_traps (borast_traps_t *traps,
						     borast_fill_rule_t fill_rule);

borast_private borast_status_t
_borast_bentley_ottmann_tessellate_rectilinear_traps (borast_traps_t *traps,
						     borast_fill_rule_t fill_rule);

borast_private int
_borast_traps_contain (const borast_traps_t *traps,
		      double x, double y);

borast_private void
_borast_traps_extents (const borast_traps_t *traps,
		      borast_box_t         *extents);

borast_private borast_int_status_t
_borast_traps_extract_region (borast_traps_t  *traps,
			     borast_region_t **region);

borast_private borast_status_t
_borast_traps_path (const borast_traps_t *traps,
		   borast_path_fixed_t  *path);

borast_private void
_borast_trapezoid_array_translate_and_scale (borast_trapezoid_t *offset_traps,
					    borast_trapezoid_t *src_traps,
					    int num_traps,
					    double tx, double ty,
					    double sx, double sy);

#endif
