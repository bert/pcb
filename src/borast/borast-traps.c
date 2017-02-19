/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/*
 * Copyright © 2002 Keith Packard
 * Copyright © 2007 Red Hat, Inc.
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
 * The Initial Developer of the Original Code is Keith Packard
 *
 * Contributor(s):
 *	Keith R. Packard <keithp@keithp.com>
 *	Carl D. Worth <cworth@cworth.org>
 *
 * 2002-07-15: Converted from XRenderCompositeDoublePoly to #cairo_trap_t. Carl D. Worth
 */

#include "borastint-minimal.h"
#include "borast-malloc-private.h"
#include "borast-traps-private.h"
#include "borast-fixed-private.h"

#define _borast_error(x) (x)

/* private functions */

void
_borast_traps_init (borast_traps_t *traps)
{
    VG (VALGRIND_MAKE_MEM_UNDEFINED (traps, sizeof (borast_traps_t)));

    traps->status = BORAST_STATUS_SUCCESS;

    traps->maybe_region = 1;
    traps->is_rectilinear = 0;
    traps->is_rectangular = 0;

    traps->num_traps = 0;

    traps->traps_size = ARRAY_LENGTH (traps->traps_embedded);
    traps->traps = traps->traps_embedded;

    traps->num_limits = 0;
    traps->has_intersections = FALSE;
}

void
_borast_traps_limit (borast_traps_t	*traps,
		    const borast_box_t	*limits,
		    int			 num_limits)
{
    traps->limits = limits;
    traps->num_limits = num_limits;
}

void
_borast_traps_clear (borast_traps_t *traps)
{
    traps->status = BORAST_STATUS_SUCCESS;

    traps->maybe_region = 1;
    traps->is_rectilinear = 0;
    traps->is_rectangular = 0;

    traps->num_traps = 0;
    traps->has_intersections = FALSE;
}

void
_borast_traps_fini (borast_traps_t *traps)
{
    if (traps->traps != traps->traps_embedded)
	free (traps->traps);

    VG (VALGRIND_MAKE_MEM_NOACCESS (traps, sizeof (borast_traps_t)));
}

/* make room for at least one more trap */
static borast_bool_t
_borast_traps_grow (borast_traps_t *traps)
{
    borast_trapezoid_t *new_traps;
    int new_size = 4 * traps->traps_size;

    if (traps->traps == traps->traps_embedded) {
	new_traps = _borast_malloc_ab (new_size, sizeof (borast_trapezoid_t));
	if (new_traps != NULL)
	    memcpy (new_traps, traps->traps, sizeof (traps->traps_embedded));
    } else {
	new_traps = _borast_realloc_ab (traps->traps,
	                               new_size, sizeof (borast_trapezoid_t));
    }

    if (unlikely (new_traps == NULL)) {
	traps->status = _borast_error (BORAST_STATUS_NO_MEMORY);
	return FALSE;
    }

    traps->traps = new_traps;
    traps->traps_size = new_size;
    return TRUE;
}

void
_borast_traps_add_trap (borast_traps_t *traps,
		       borast_fixed_t top, borast_fixed_t bottom,
		       borast_line_t *left, borast_line_t *right)
{
    borast_trapezoid_t *trap;

    if (unlikely (traps->num_traps == traps->traps_size)) {
	if (unlikely (! _borast_traps_grow (traps)))
	    return;
    }

    trap = &traps->traps[traps->num_traps++];
    trap->top = top;
    trap->bottom = bottom;
    trap->left = *left;
    trap->right = *right;
}

/**
 * _borast_traps_init_box:
 * @traps: a #borast_traps_t
 * @box: an array box that will each be converted to a single trapezoid
 *       to store in @traps.
 *
 * Initializes a #borast_traps_t to contain an array of rectangular
 * trapezoids.
 **/
borast_status_t
_borast_traps_init_boxes (borast_traps_t	    *traps,
		         const borast_box_t  *boxes,
			 int		     num_boxes)
{
    borast_trapezoid_t *trap;

    _borast_traps_init (traps);

    while (traps->traps_size < num_boxes) {
	if (unlikely (! _borast_traps_grow (traps))) {
	    _borast_traps_fini (traps);
	    return _borast_error (BORAST_STATUS_NO_MEMORY);
	}
    }

    traps->num_traps = num_boxes;
    traps->is_rectilinear = TRUE;
    traps->is_rectangular = TRUE;

    trap = &traps->traps[0];
    while (num_boxes--) {
	trap->top    = boxes->p1.y;
	trap->bottom = boxes->p2.y;

	trap->left.p1   = boxes->p1;
	trap->left.p2.x = boxes->p1.x;
	trap->left.p2.y = boxes->p2.y;

	trap->right.p1.x = boxes->p2.x;
	trap->right.p1.y = boxes->p1.y;
	trap->right.p2   = boxes->p2;

	if (traps->maybe_region) {
	    traps->maybe_region  = _borast_fixed_is_integer (boxes->p1.x) &&
		                   _borast_fixed_is_integer (boxes->p1.y) &&
		                   _borast_fixed_is_integer (boxes->p2.x) &&
		                   _borast_fixed_is_integer (boxes->p2.y);
	}

	trap++, boxes++;
    }

    return BORAST_STATUS_SUCCESS;
}

borast_status_t
_borast_traps_tessellate_rectangle (borast_traps_t *traps,
				   const borast_point_t *top_left,
				   const borast_point_t *bottom_right)
{
    borast_line_t left;
    borast_line_t right;
    borast_fixed_t top, bottom;

    if (top_left->y == bottom_right->y)
	return BORAST_STATUS_SUCCESS;

    if (top_left->x == bottom_right->x)
	return BORAST_STATUS_SUCCESS;

     left.p1.x =  left.p2.x = top_left->x;
     left.p1.y = right.p1.y = top_left->y;
    right.p1.x = right.p2.x = bottom_right->x;
     left.p2.y = right.p2.y = bottom_right->y;

     top = top_left->y;
     bottom = bottom_right->y;

    if (traps->num_limits) {
	borast_bool_t reversed;
	int n;

	/* support counter-clockwise winding for rectangular tessellation */
	reversed = top_left->x > bottom_right->x;
	if (reversed) {
	    right.p1.x = right.p2.x = top_left->x;
	    left.p1.x = left.p2.x = bottom_right->x;
	}

	for (n = 0; n < traps->num_limits; n++) {
	    const borast_box_t *limits = &traps->limits[n];
	    borast_line_t _left, _right;
	    borast_fixed_t _top, _bottom;

	    if (top >= limits->p2.y)
		continue;
	    if (bottom <= limits->p1.y)
		continue;

	    /* Trivially reject if trapezoid is entirely to the right or
	     * to the left of the limits. */
	    if (left.p1.x >= limits->p2.x)
		continue;
	    if (right.p1.x <= limits->p1.x)
		continue;

	    /* Otherwise, clip the trapezoid to the limits. */
	    _top = top;
	    if (_top < limits->p1.y)
		_top = limits->p1.y;

	    _bottom = bottom;
	    if (_bottom > limits->p2.y)
		_bottom = limits->p2.y;

	    if (_bottom <= _top)
		continue;

	    _left = left;
	    if (_left.p1.x < limits->p1.x) {
		_left.p1.x = limits->p1.x;
		_left.p1.y = limits->p1.y;
		_left.p2.x = limits->p1.x;
		_left.p2.y = limits->p2.y;
	    }

	    _right = right;
	    if (_right.p1.x > limits->p2.x) {
		_right.p1.x = limits->p2.x;
		_right.p1.y = limits->p1.y;
		_right.p2.x = limits->p2.x;
		_right.p2.y = limits->p2.y;
	    }

	    if (left.p1.x >= right.p1.x)
		continue;

	    if (reversed)
		_borast_traps_add_trap (traps, _top, _bottom, &_right, &_left);
	    else
		_borast_traps_add_trap (traps, _top, _bottom, &_left, &_right);
	}
    } else {
	_borast_traps_add_trap (traps, top, bottom, &left, &right);
    }

    return traps->status;
}

void
_borast_traps_translate (borast_traps_t *traps, int x, int y)
{
    borast_fixed_t xoff, yoff;
    borast_trapezoid_t *t;
    int i;

    /* Ugh. The borast_composite/(Render) interface doesn't allow
       an offset for the trapezoids. Need to manually shift all
       the coordinates to align with the offset origin of the
       intermediate surface. */

    xoff = _borast_fixed_from_int (x);
    yoff = _borast_fixed_from_int (y);

    for (i = 0, t = traps->traps; i < traps->num_traps; i++, t++) {
	t->top += yoff;
	t->bottom += yoff;
	t->left.p1.x += xoff;
	t->left.p1.y += yoff;
	t->left.p2.x += xoff;
	t->left.p2.y += yoff;
	t->right.p1.x += xoff;
	t->right.p1.y += yoff;
	t->right.p2.x += xoff;
	t->right.p2.y += yoff;
    }
}

void
_borast_trapezoid_array_translate_and_scale (borast_trapezoid_t *offset_traps,
                                            borast_trapezoid_t *src_traps,
                                            int num_traps,
                                            double tx, double ty,
                                            double sx, double sy)
{
    int i;
    borast_fixed_t xoff = _borast_fixed_from_double (tx);
    borast_fixed_t yoff = _borast_fixed_from_double (ty);

    if (sx == 1.0 && sy == 1.0) {
        for (i = 0; i < num_traps; i++) {
            offset_traps[i].top = src_traps[i].top + yoff;
            offset_traps[i].bottom = src_traps[i].bottom + yoff;
            offset_traps[i].left.p1.x = src_traps[i].left.p1.x + xoff;
            offset_traps[i].left.p1.y = src_traps[i].left.p1.y + yoff;
            offset_traps[i].left.p2.x = src_traps[i].left.p2.x + xoff;
            offset_traps[i].left.p2.y = src_traps[i].left.p2.y + yoff;
            offset_traps[i].right.p1.x = src_traps[i].right.p1.x + xoff;
            offset_traps[i].right.p1.y = src_traps[i].right.p1.y + yoff;
            offset_traps[i].right.p2.x = src_traps[i].right.p2.x + xoff;
            offset_traps[i].right.p2.y = src_traps[i].right.p2.y + yoff;
        }
    } else {
        borast_fixed_t xsc = _borast_fixed_from_double (sx);
        borast_fixed_t ysc = _borast_fixed_from_double (sy);

        for (i = 0; i < num_traps; i++) {
            offset_traps[i].top = _borast_fixed_mul (src_traps[i].top + yoff, ysc);
            offset_traps[i].bottom = _borast_fixed_mul (src_traps[i].bottom + yoff, ysc);
            offset_traps[i].left.p1.x = _borast_fixed_mul (src_traps[i].left.p1.x + xoff, xsc);
            offset_traps[i].left.p1.y = _borast_fixed_mul (src_traps[i].left.p1.y + yoff, ysc);
            offset_traps[i].left.p2.x = _borast_fixed_mul (src_traps[i].left.p2.x + xoff, xsc);
            offset_traps[i].left.p2.y = _borast_fixed_mul (src_traps[i].left.p2.y + yoff, ysc);
            offset_traps[i].right.p1.x = _borast_fixed_mul (src_traps[i].right.p1.x + xoff, xsc);
            offset_traps[i].right.p1.y = _borast_fixed_mul (src_traps[i].right.p1.y + yoff, ysc);
            offset_traps[i].right.p2.x = _borast_fixed_mul (src_traps[i].right.p2.x + xoff, xsc);
            offset_traps[i].right.p2.y = _borast_fixed_mul (src_traps[i].right.p2.y + yoff, ysc);
        }
    }
}

static borast_fixed_t
_line_compute_intersection_x_for_y (const borast_line_t *line,
				    borast_fixed_t y)
{
    return _borast_edge_compute_intersection_x_for_y (&line->p1, &line->p2, y);
}

void
_borast_traps_extents (const borast_traps_t *traps,
		      borast_box_t *extents)
{
    int i;

    if (traps->num_traps == 0) {
	extents->p1.x = extents->p1.y = 0;
	extents->p2.x = extents->p2.y = 0;
	return;
    }

    extents->p1.x = extents->p1.y = INT32_MAX;
    extents->p2.x = extents->p2.y = INT32_MIN;

    for (i = 0; i < traps->num_traps; i++) {
	const borast_trapezoid_t *trap =  &traps->traps[i];

	if (trap->top < extents->p1.y)
	    extents->p1.y = trap->top;
	if (trap->bottom > extents->p2.y)
	    extents->p2.y = trap->bottom;

	if (trap->left.p1.x < extents->p1.x) {
	    borast_fixed_t x = trap->left.p1.x;
	    if (trap->top != trap->left.p1.y) {
		x = _line_compute_intersection_x_for_y (&trap->left,
							trap->top);
		if (x < extents->p1.x)
		    extents->p1.x = x;
	    } else
		extents->p1.x = x;
	}
	if (trap->left.p2.x < extents->p1.x) {
	    borast_fixed_t x = trap->left.p2.x;
	    if (trap->bottom != trap->left.p2.y) {
		x = _line_compute_intersection_x_for_y (&trap->left,
							trap->bottom);
		if (x < extents->p1.x)
		    extents->p1.x = x;
	    } else
		extents->p1.x = x;
	}

	if (trap->right.p1.x > extents->p2.x) {
	    borast_fixed_t x = trap->right.p1.x;
	    if (trap->top != trap->right.p1.y) {
		x = _line_compute_intersection_x_for_y (&trap->right,
							trap->top);
		if (x > extents->p2.x)
		    extents->p2.x = x;
	    } else
		extents->p2.x = x;
	}
	if (trap->right.p2.x > extents->p2.x) {
	    borast_fixed_t x = trap->right.p2.x;
	    if (trap->bottom != trap->right.p2.y) {
		x = _line_compute_intersection_x_for_y (&trap->right,
							trap->bottom);
		if (x > extents->p2.x)
		    extents->p2.x = x;
	    } else
		extents->p2.x = x;
	}
    }
}
