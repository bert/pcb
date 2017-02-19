/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
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

#ifndef BORAST_TYPES_PRIVATE_H
#define BORAST_TYPES_PRIVATE_H

#include "borast-minimal.h"
#include "borast-fixed-type-private.h"
#include "borast-compiler-private.h"

typedef struct _borast_array borast_array_t;
typedef struct _borast_backend borast_backend_t;
typedef struct _borast_cache borast_cache_t;
typedef struct _borast_clip borast_clip_t;
typedef struct _borast_clip_path borast_clip_path_t;
typedef struct _borast_gstate borast_gstate_t;
typedef struct _borast_hash_entry borast_hash_entry_t;
typedef struct _borast_hash_table borast_hash_table_t;
typedef struct _borast_path_fixed borast_path_fixed_t;

typedef borast_array_t borast_user_data_array_t;

/**
 * borast_hash_entry_t:
 *
 * A #borast_hash_entry_t contains both a key and a value for
 * #borast_hash_table_t. User-derived types for #borast_hash_entry_t must
 * be type-compatible with this structure (eg. they must have an
 * unsigned long as the first parameter. The easiest way to get this
 * is to use:
 *
 * 	typedef _my_entry {
 *	    borast_hash_entry_t base;
 *	    ... Remainder of key and value fields here ..
 *	} my_entry_t;
 *
 * which then allows a pointer to my_entry_t to be passed to any of
 * the #borast_hash_table_t functions as follows without requiring a cast:
 *
 *	_borast_hash_table_insert (hash_table, &my_entry->base);
 *
 * IMPORTANT: The caller is reponsible for initializing
 * my_entry->base.hash with a hash code derived from the key. The
 * essential property of the hash code is that keys_equal must never
 * return %TRUE for two keys that have different hashes. The best hash
 * code will reduce the frequency of two keys with the same code for
 * which keys_equal returns %FALSE.
 *
 * Which parts of the entry make up the "key" and which part make up
 * the value are entirely up to the caller, (as determined by the
 * computation going into base.hash as well as the keys_equal
 * function). A few of the #borast_hash_table_t functions accept an entry
 * which will be used exclusively as a "key", (indicated by a
 * parameter name of key). In these cases, the value-related fields of
 * the entry need not be initialized if so desired.
 **/
struct _borast_hash_entry {
    unsigned long hash;
};

struct _borast_array {
    unsigned int size;
    unsigned int num_elements;
    unsigned int element_size;
    char **elements;

    borast_bool_t is_snapshot;
};

/* Sure wish C had a real enum type so that this would be distinct
 * from #borast_status_t. Oh well, without that, I'll use this bogus 100
 * offset.  We want to keep it fit in int8_t as the compiler may choose
 * that for #borast_status_t */
typedef enum _borast_int_status {
    BORAST_INT_STATUS_UNSUPPORTED = 100,
    BORAST_INT_STATUS_DEGENERATE,
    BORAST_INT_STATUS_NOTHING_TO_DO,
    BORAST_INT_STATUS_FLATTEN_TRANSPARENCY,
    BORAST_INT_STATUS_IMAGE_FALLBACK,
    BORAST_INT_STATUS_ANALYZE_RECORDING_SURFACE_PATTERN,

    BORAST_INT_STATUS_LAST_STATUS
} borast_int_status_t;

typedef struct _borast_slope {
    borast_fixed_t dx;
    borast_fixed_t dy;
} borast_slope_t, borast_distance_t;

typedef struct _borast_point_double {
    double x;
    double y;
} borast_point_double_t;

typedef struct _borast_distance_double {
    double dx;
    double dy;
} borast_distance_double_t;

typedef struct _borast_line {
    borast_point_t p1;
    borast_point_t p2;
} borast_line_t, borast_box_t;

typedef struct _borast_trapezoid {
    borast_fixed_t top, bottom;
    borast_line_t left, right;
} borast_trapezoid_t;

typedef struct _borast_point_int {
    int x, y;
} borast_point_int_t;

#define BORAST_RECT_INT_MIN (INT_MIN >> BORAST_FIXED_FRAC_BITS)
#define BORAST_RECT_INT_MAX (INT_MAX >> BORAST_FIXED_FRAC_BITS)

/* Rectangles that take part in a composite operation.
 *
 * This defines four translations that define which pixels of the
 * source pattern, mask, clip and destination surface take part in a
 * general composite operation.  The idea is that the pixels at
 *
 *	(i,j)+(src.x, src.y) of the source,
 *      (i,j)+(mask.x, mask.y) of the mask,
 *      (i,j)+(clip.x, clip.y) of the clip and
 *      (i,j)+(dst.x, dst.y) of the destination
 *
 * all combine together to form the result at (i,j)+(dst.x,dst.y),
 * for i,j ranging in [0,width) and [0,height) respectively.
 */
typedef struct _borast_composite_rectangles {
        borast_point_int_t src;
        borast_point_int_t mask;
        borast_point_int_t clip;
        borast_point_int_t dst;
        int width;
        int height;
} borast_composite_rectangles_t;

typedef struct _borast_edge {
    borast_line_t line;
    int top, bottom;
    int dir;
} borast_edge_t;

typedef struct _borast_polygon {
    borast_status_t status;

    borast_point_t first_point;
    borast_point_t last_point;
    borast_point_t current_point;
    borast_slope_t current_edge;
    borast_bool_t has_current_point;
    borast_bool_t has_current_edge;

    borast_box_t extents;
    borast_box_t limit;
    const borast_box_t *limits;
    int num_limits;

    int num_edges;
    int edges_size;
    borast_edge_t *edges;
    borast_edge_t  edges_embedded[32];
} borast_polygon_t;

typedef borast_warn borast_status_t
(*borast_spline_add_point_func_t) (void *closure,
				  const borast_point_t *point);

#endif /* BORAST_TYPES_PRIVATE_H */
