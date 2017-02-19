/*
 * Copyright © 2004 Carl Worth
 * Copyright © 2006 Red Hat, Inc.
 * Copyright © 2008 Chris Wilson
 * Copyright © 2009 Peter Clifton
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
 * The Initial Developer of the Original Code is Carl Worth
 *
 * Contributor(s):
 *        Carl D. Worth <cworth@cworth.org>
 *        Chris Wilson <chris@chris-wilson.co.uk>
 *        Peter Clifton <pcjc2@cam.ac.uk> (Adaptation to PCB use)
 */

/* Provide definitions for standalone compilation */
#include "borastint-minimal.h"
#include "borast-malloc-private.h"
#include "borast-traps-private.h"
#include "borast-fixed-private.h"
#include "borast-freelist-private.h"
#include "borast-combsort-private.h"

#include <glib.h>

#include "polygon.h"
#include "hid_draw.h"

#ifdef WIN32
#   define WIN32_LEAN_AND_MEAN 1
#endif

#include <GL/gl.h>
#include "hid/common/hidgl.h"

#include "sweep.h"


#define _borast_error(x) (x)

#define DEBUG_PRINT_STATE 0
#define DEBUG_EVENTS 0
#define DEBUG_TRAPS 0

typedef borast_point_t borast_bo_point32_t;

typedef struct _borast_bo_edge borast_bo_edge_t;
typedef struct _borast_bo_trap borast_bo_trap_t;

/* A deferred trapezoid of an edge */
struct _borast_bo_trap {
    borast_bo_edge_t *right;
    int32_t top;
};

struct _borast_bo_edge {
    borast_edge_t edge;
    borast_bo_edge_t *prev;
    borast_bo_edge_t *next;
    borast_bo_trap_t deferred_trap;
};

/* the parent is always given by index/2 */
#define PQ_PARENT_INDEX(i) ((i) >> 1)
#define PQ_FIRST_ENTRY 1

/* left and right children are index * 2 and (index * 2) +1 respectively */
#define PQ_LEFT_CHILD_INDEX(i) ((i) << 1)

typedef enum {
    BORAST_BO_EVENT_TYPE_STOP,
    BORAST_BO_EVENT_TYPE_START
} borast_bo_event_type_t;

typedef struct _borast_bo_event {
    borast_bo_event_type_t type;
    borast_point_t point;
} borast_bo_event_t;

typedef struct _borast_bo_start_event {
    borast_bo_event_type_t type;
    borast_point_t point;
    borast_bo_edge_t edge;
} borast_bo_start_event_t;

typedef struct _borast_bo_queue_event {
    borast_bo_event_type_t type;
    borast_point_t point;
    borast_bo_edge_t *e1;
    borast_bo_edge_t *e2;
} borast_bo_queue_event_t;

typedef struct _pqueue {
    int size, max_size;

    borast_bo_event_t **elements;
    borast_bo_event_t *elements_embedded[1024];
} pqueue_t;

typedef struct _borast_bo_event_queue {
    borast_freepool_t pool;
    pqueue_t pqueue;
    borast_bo_event_t **start_events;
} borast_bo_event_queue_t;

typedef struct _borast_bo_sweep_line {
    borast_bo_edge_t *head;
    borast_bo_edge_t *stopped;
    int32_t current_y;
    borast_bo_edge_t *current_edge;
} borast_bo_sweep_line_t;


static borast_fixed_t
_line_compute_intersection_x_for_y (const borast_line_t *line,
                                    borast_fixed_t y)
{
    borast_fixed_t x, dy;

    if (y == line->p1.y)
        return line->p1.x;
    if (y == line->p2.y)
        return line->p2.x;

    x = line->p1.x;
    dy = line->p2.y - line->p1.y;
    if (dy != 0) {
        x += _borast_fixed_mul_div_floor (y - line->p1.y,
                                         line->p2.x - line->p1.x,
                                         dy);
    }

    return x;
}

static inline int
_borast_bo_point32_compare (borast_bo_point32_t const *a,
                           borast_bo_point32_t const *b)
{
    int cmp;

    cmp = a->y - b->y;
    if (cmp)
        return cmp;

    return a->x - b->x;
}

/* Compare the slope of a to the slope of b, returning 1, 0, -1 if the
 * slope a is respectively greater than, equal to, or less than the
 * slope of b.
 *
 * For each edge, consider the direction vector formed from:
 *
 *        top -> bottom
 *
 * which is:
 *
 *        (dx, dy) = (line.p2.x - line.p1.x, line.p2.y - line.p1.y)
 *
 * We then define the slope of each edge as dx/dy, (which is the
 * inverse of the slope typically used in math instruction). We never
 * compute a slope directly as the value approaches infinity, but we
 * can derive a slope comparison without division as follows, (where
 * the ? represents our compare operator).
 *
 * 1.           slope(a) ? slope(b)
 * 2.            adx/ady ? bdx/bdy
 * 3.        (adx * bdy) ? (bdx * ady)
 *
 * Note that from step 2 to step 3 there is no change needed in the
 * sign of the result since both ady and bdy are guaranteed to be
 * greater than or equal to 0.
 *
 * When using this slope comparison to sort edges, some care is needed
 * when interpreting the results. Since the slope compare operates on
 * distance vectors from top to bottom it gives a correct left to
 * right sort for edges that have a common top point, (such as two
 * edges with start events at the same location). On the other hand,
 * the sense of the result will be exactly reversed for two edges that
 * have a common stop point.
 */
static inline int
_slope_compare (const borast_bo_edge_t *a,
                const borast_bo_edge_t *b)
{
    /* XXX: We're assuming here that dx and dy will still fit in 32
     * bits. That's not true in general as there could be overflow. We
     * should prevent that before the tessellation algorithm
     * begins.
     */
    int32_t adx = a->edge.line.p2.x - a->edge.line.p1.x;
    int32_t bdx = b->edge.line.p2.x - b->edge.line.p1.x;

    /* Since the dy's are all positive by construction we can fast
     * path several common cases.
     */

    /* First check for vertical lines. */
    if (adx == 0)
        return -bdx;
    if (bdx == 0)
        return adx;

    /* Then where the two edges point in different directions wrt x. */
    if ((adx ^ bdx) < 0)
        return adx;

    /* Finally we actually need to do the general comparison. */
    {
        int32_t ady = a->edge.line.p2.y - a->edge.line.p1.y;
        int32_t bdy = b->edge.line.p2.y - b->edge.line.p1.y;
        borast_int64_t adx_bdy = _borast_int32x32_64_mul (adx, bdy);
        borast_int64_t bdx_ady = _borast_int32x32_64_mul (bdx, ady);

        return _borast_int64_cmp (adx_bdy, bdx_ady);
    }
}

/*
 * We need to compare the x-coordinates of a pair of lines for a particular y,
 * without loss of precision.
 *
 * The x-coordinate along an edge for a given y is:
 *   X = A_x + (Y - A_y) * A_dx / A_dy
 *
 * So the inequality we wish to test is:
 *   A_x + (Y - A_y) * A_dx / A_dy ∘ B_x + (Y - B_y) * B_dx / B_dy,
 * where ∘ is our inequality operator.
 *
 * By construction, we know that A_dy and B_dy (and (Y - A_y), (Y - B_y)) are
 * all positive, so we can rearrange it thus without causing a sign change:
 *   A_dy * B_dy * (A_x - B_x) ∘ (Y - B_y) * B_dx * A_dy
 *                                 - (Y - A_y) * A_dx * B_dy
 *
 * Given the assumption that all the deltas fit within 32 bits, we can compute
 * this comparison directly using 128 bit arithmetic. For certain, but common,
 * input we can reduce this down to a single 32 bit compare by inspecting the
 * deltas.
 *
 * (And put the burden of the work on developing fast 128 bit ops, which are
 * required throughout the tessellator.)
 *
 * See the similar discussion for _slope_compare().
 */
static int
edges_compare_x_for_y_general (const borast_bo_edge_t *a,
                               const borast_bo_edge_t *b,
                               int32_t y)
{
    /* XXX: We're assuming here that dx and dy will still fit in 32
     * bits. That's not true in general as there could be overflow. We
     * should prevent that before the tessellation algorithm
     * begins.
     */
    int32_t dx;
    int32_t adx, ady;
    int32_t bdx, bdy;
    enum {
       HAVE_NONE    = 0x0,
       HAVE_DX      = 0x1,
       HAVE_ADX     = 0x2,
       HAVE_DX_ADX  = HAVE_DX | HAVE_ADX,
       HAVE_BDX     = 0x4,
       HAVE_DX_BDX  = HAVE_DX | HAVE_BDX,
       HAVE_ADX_BDX = HAVE_ADX | HAVE_BDX,
       HAVE_ALL     = HAVE_DX | HAVE_ADX | HAVE_BDX
    } have_dx_adx_bdx = HAVE_ALL;

    /* don't bother solving for abscissa if the edges' bounding boxes
     * can be used to order them. */
    {
           int32_t amin, amax;
           int32_t bmin, bmax;
           if (a->edge.line.p1.x < a->edge.line.p2.x) {
                   amin = a->edge.line.p1.x;
                   amax = a->edge.line.p2.x;
           } else {
                   amin = a->edge.line.p2.x;
                   amax = a->edge.line.p1.x;
           }
           if (b->edge.line.p1.x < b->edge.line.p2.x) {
                   bmin = b->edge.line.p1.x;
                   bmax = b->edge.line.p2.x;
           } else {
                   bmin = b->edge.line.p2.x;
                   bmax = b->edge.line.p1.x;
           }
           if (amax < bmin) return -1;
           if (amin > bmax) return +1;
    }

    ady = a->edge.line.p2.y - a->edge.line.p1.y;
    adx = a->edge.line.p2.x - a->edge.line.p1.x;
    if (adx == 0)
        have_dx_adx_bdx &= ~HAVE_ADX;

    bdy = b->edge.line.p2.y - b->edge.line.p1.y;
    bdx = b->edge.line.p2.x - b->edge.line.p1.x;
    if (bdx == 0)
        have_dx_adx_bdx &= ~HAVE_BDX;

    dx = a->edge.line.p1.x - b->edge.line.p1.x;
    if (dx == 0)
        have_dx_adx_bdx &= ~HAVE_DX;

#define L _borast_int64x32_128_mul (_borast_int32x32_64_mul (ady, bdy), dx)
#define A _borast_int64x32_128_mul (_borast_int32x32_64_mul (adx, bdy), y - a->edge.line.p1.y)
#define B _borast_int64x32_128_mul (_borast_int32x32_64_mul (bdx, ady), y - b->edge.line.p1.y)
    switch (have_dx_adx_bdx) {
    default:
    case HAVE_NONE:
        return 0;
    case HAVE_DX:
        /* A_dy * B_dy * (A_x - B_x) ∘ 0 */
        return dx; /* ady * bdy is positive definite */
    case HAVE_ADX:
        /* 0 ∘  - (Y - A_y) * A_dx * B_dy */
        return adx; /* bdy * (y - a->top.y) is positive definite */
    case HAVE_BDX:
        /* 0 ∘ (Y - B_y) * B_dx * A_dy */
        return -bdx; /* ady * (y - b->top.y) is positive definite */
    case HAVE_ADX_BDX:
        /*  0 ∘ (Y - B_y) * B_dx * A_dy - (Y - A_y) * A_dx * B_dy */
        if ((adx ^ bdx) < 0) {
            return adx;
        } else if (a->edge.line.p1.y == b->edge.line.p1.y) { /* common origin */
            borast_int64_t adx_bdy, bdx_ady;

            /* ∴ A_dx * B_dy ∘ B_dx * A_dy */

            adx_bdy = _borast_int32x32_64_mul (adx, bdy);
            bdx_ady = _borast_int32x32_64_mul (bdx, ady);

            return _borast_int64_cmp (adx_bdy, bdx_ady);
        } else
            return _borast_int128_cmp (A, B);
    case HAVE_DX_ADX:
        /* A_dy * (A_x - B_x) ∘ - (Y - A_y) * A_dx */
        if ((-adx ^ dx) < 0) {
            return dx;
        } else {
            borast_int64_t ady_dx, dy_adx;

            ady_dx = _borast_int32x32_64_mul (ady, dx);
            dy_adx = _borast_int32x32_64_mul (a->edge.line.p1.y - y, adx);

            return _borast_int64_cmp (ady_dx, dy_adx);
        }
    case HAVE_DX_BDX:
        /* B_dy * (A_x - B_x) ∘ (Y - B_y) * B_dx */
        if ((bdx ^ dx) < 0) {
            return dx;
        } else {
            borast_int64_t bdy_dx, dy_bdx;

            bdy_dx = _borast_int32x32_64_mul (bdy, dx);
            dy_bdx = _borast_int32x32_64_mul (y - b->edge.line.p1.y, bdx);

            return _borast_int64_cmp (bdy_dx, dy_bdx);
        }
    case HAVE_ALL:
        /* XXX try comparing (a->edge.line.p2.x - b->edge.line.p2.x) et al */
        return _borast_int128_cmp (L, _borast_int128_sub (B, A));
    }
#undef B
#undef A
#undef L
}

/*
 * We need to compare the x-coordinate of a line for a particular y wrt to a
 * given x, without loss of precision.
 *
 * The x-coordinate along an edge for a given y is:
 *   X = A_x + (Y - A_y) * A_dx / A_dy
 *
 * So the inequality we wish to test is:
 *   A_x + (Y - A_y) * A_dx / A_dy ∘ X
 * where ∘ is our inequality operator.
 *
 * By construction, we know that A_dy (and (Y - A_y)) are
 * all positive, so we can rearrange it thus without causing a sign change:
 *   (Y - A_y) * A_dx ∘ (X - A_x) * A_dy
 *
 * Given the assumption that all the deltas fit within 32 bits, we can compute
 * this comparison directly using 64 bit arithmetic.
 *
 * See the similar discussion for _slope_compare() and
 * edges_compare_x_for_y_general().
 */
static int
edge_compare_for_y_against_x (const borast_bo_edge_t *a,
                              int32_t y,
                              int32_t x)
{
    int32_t adx, ady;
    int32_t dx, dy;
    borast_int64_t L, R;

    if (x < a->edge.line.p1.x && x < a->edge.line.p2.x)
        return 1;
    if (x > a->edge.line.p1.x && x > a->edge.line.p2.x)
        return -1;

    adx = a->edge.line.p2.x - a->edge.line.p1.x;
    dx = x - a->edge.line.p1.x;

    if (adx == 0)
        return -dx;
    if (dx == 0 || (adx ^ dx) < 0)
        return adx;

    dy = y - a->edge.line.p1.y;
    ady = a->edge.line.p2.y - a->edge.line.p1.y;

    L = _borast_int32x32_64_mul (dy, adx);
    R = _borast_int32x32_64_mul (dx, ady);

    return _borast_int64_cmp (L, R);
}

static int
edges_compare_x_for_y (const borast_bo_edge_t *a,
                       const borast_bo_edge_t *b,
                       int32_t y)
{
    /* If the sweep-line is currently on an end-point of a line,
     * then we know its precise x value (and considering that we often need to
     * compare events at end-points, this happens frequently enough to warrant
     * special casing).
     */
    enum {
       HAVE_NEITHER = 0x0,
       HAVE_AX      = 0x1,
       HAVE_BX      = 0x2,
       HAVE_BOTH    = HAVE_AX | HAVE_BX
    } have_ax_bx = HAVE_BOTH;
    int32_t ax = 0; /* Initialised to silence compiler, we'd be ok without */
    int32_t bx = 0; /* Initialised to silence compiler, we'd be ok without */

    if (y == a->edge.line.p1.y)
        ax = a->edge.line.p1.x;
    else if (y == a->edge.line.p2.y)
        ax = a->edge.line.p2.x;
    else
        have_ax_bx &= ~HAVE_AX;

    if (y == b->edge.line.p1.y)
        bx = b->edge.line.p1.x;
    else if (y == b->edge.line.p2.y)
        bx = b->edge.line.p2.x;
    else
        have_ax_bx &= ~HAVE_BX;

    switch (have_ax_bx) {
    default:
    case HAVE_NEITHER:
        return edges_compare_x_for_y_general (a, b, y);
    case HAVE_AX:
        return -edge_compare_for_y_against_x (b, y, ax);
    case HAVE_BX:
        return edge_compare_for_y_against_x (a, y, bx);
    case HAVE_BOTH:
        return ax - bx;
    }
}

static inline int
_line_equal (const borast_line_t *a, const borast_line_t *b)
{
    return a->p1.x == b->p1.x && a->p1.y == b->p1.y &&
           a->p2.x == b->p2.x && a->p2.y == b->p2.y;
}

static int
_borast_bo_sweep_line_compare_edges (borast_bo_sweep_line_t        *sweep_line,
                                    const borast_bo_edge_t        *a,
                                    const borast_bo_edge_t        *b)
{
    int cmp;

    /* compare the edges if not identical */
    if (! _line_equal (&a->edge.line, &b->edge.line)) {
        cmp = edges_compare_x_for_y (a, b, sweep_line->current_y);
        if (cmp)
            return cmp;

        /* The two edges intersect exactly at y, so fall back on slope
         * comparison. We know that this compare_edges function will be
         * called only when starting a new edge, (not when stopping an
         * edge), so we don't have to worry about conditionally inverting
         * the sense of _slope_compare. */
        cmp = _slope_compare (a, b);
        if (cmp)
            return cmp;
    }

    /* We've got two collinear edges now. */
    return b->edge.bottom - a->edge.bottom;
}

static inline borast_int64_t
det32_64 (int32_t a, int32_t b,
          int32_t c, int32_t d)
{
    /* det = a * d - b * c */
    return _borast_int64_sub (_borast_int32x32_64_mul (a, d),
                             _borast_int32x32_64_mul (b, c));
}

static inline borast_int128_t
det64x32_128 (borast_int64_t a, int32_t       b,
              borast_int64_t c, int32_t       d)
{
    /* det = a * d - b * c */
    return _borast_int128_sub (_borast_int64x32_128_mul (a, d),
                              _borast_int64x32_128_mul (c, b));
}

static inline int
borast_bo_event_compare (const borast_bo_event_t *a,
                        const borast_bo_event_t *b)
{
    int cmp;

    cmp = _borast_bo_point32_compare (&a->point, &b->point);
    if (cmp)
        return cmp;

    cmp = a->type - b->type;
    if (cmp)
        return cmp;

    return a - b;
}

static inline void
_pqueue_init (pqueue_t *pq)
{
    pq->max_size = ARRAY_LENGTH (pq->elements_embedded);
    pq->size = 0;

    pq->elements = pq->elements_embedded;
}

static inline void
_pqueue_fini (pqueue_t *pq)
{
    if (pq->elements != pq->elements_embedded)
        free (pq->elements);
}

static borast_status_t
_pqueue_grow (pqueue_t *pq)
{
    borast_bo_event_t **new_elements;
    pq->max_size *= 2;

    if (pq->elements == pq->elements_embedded) {
        new_elements = _borast_malloc_ab (pq->max_size,
                                         sizeof (borast_bo_event_t *));
        if (unlikely (new_elements == NULL))
            return _borast_error (BORAST_STATUS_NO_MEMORY);

        memcpy (new_elements, pq->elements_embedded,
                sizeof (pq->elements_embedded));
    } else {
        new_elements = _borast_realloc_ab (pq->elements,
                                          pq->max_size,
                                          sizeof (borast_bo_event_t *));
        if (unlikely (new_elements == NULL))
            return _borast_error (BORAST_STATUS_NO_MEMORY);
    }

    pq->elements = new_elements;
    return BORAST_STATUS_SUCCESS;
}

static inline borast_status_t
_pqueue_push (pqueue_t *pq, borast_bo_event_t *event)
{
    borast_bo_event_t **elements;
    int i, parent;

    if (unlikely (pq->size + 1 == pq->max_size)) {
        borast_status_t status;

        status = _pqueue_grow (pq);
        if (unlikely (status))
            return status;
    }

    elements = pq->elements;

    for (i = ++pq->size;
         i != PQ_FIRST_ENTRY &&
         borast_bo_event_compare (event,
                                 elements[parent = PQ_PARENT_INDEX (i)]) < 0;
         i = parent)
    {
        elements[i] = elements[parent];
    }

    elements[i] = event;

    return BORAST_STATUS_SUCCESS;
}

static inline void
_pqueue_pop (pqueue_t *pq)
{
    borast_bo_event_t **elements = pq->elements;
    borast_bo_event_t *tail;
    int child, i;

    tail = elements[pq->size--];
    if (pq->size == 0) {
        elements[PQ_FIRST_ENTRY] = NULL;
        return;
    }

    for (i = PQ_FIRST_ENTRY;
         (child = PQ_LEFT_CHILD_INDEX (i)) <= pq->size;
         i = child)
    {
        if (child != pq->size &&
            borast_bo_event_compare (elements[child+1],
                                    elements[child]) < 0)
        {
            child++;
        }

        if (borast_bo_event_compare (elements[child], tail) >= 0)
            break;

        elements[i] = elements[child];
    }
    elements[i] = tail;
}

static inline borast_status_t
_borast_bo_event_queue_insert (borast_bo_event_queue_t        *queue,
                              borast_bo_event_type_t         type,
                              borast_bo_edge_t                *e1,
                              borast_bo_edge_t                *e2,
                              const borast_point_t         *point)
{
    borast_bo_queue_event_t *event;

    event = _borast_freepool_alloc (&queue->pool);
    if (unlikely (event == NULL))
        return _borast_error (BORAST_STATUS_NO_MEMORY);

    event->type = type;
    event->e1 = e1;
    event->e2 = e2;
    event->point = *point;

    return _pqueue_push (&queue->pqueue, (borast_bo_event_t *) event);
}

static void
_borast_bo_event_queue_delete (borast_bo_event_queue_t *queue,
                              borast_bo_event_t             *event)
{
    _borast_freepool_free (&queue->pool, event);
}

static borast_bo_event_t *
_borast_bo_event_dequeue (borast_bo_event_queue_t *event_queue)
{
    borast_bo_event_t *event, *cmp;

    event = event_queue->pqueue.elements[PQ_FIRST_ENTRY];
    cmp = *event_queue->start_events;
    if (event == NULL ||
        (cmp != NULL && borast_bo_event_compare (cmp, event) < 0))
    {
        event = cmp;
        event_queue->start_events++;
    }
    else
    {
        _pqueue_pop (&event_queue->pqueue);
    }

    return event;
}

BORAST_COMBSORT_DECLARE (_borast_bo_event_queue_sort,
                        borast_bo_event_t *,
                        borast_bo_event_compare)

static void
_borast_bo_event_queue_init (borast_bo_event_queue_t         *event_queue,
                            borast_bo_event_t                **start_events,
                            int                                  num_events)
{
    _borast_bo_event_queue_sort (start_events, num_events);
    start_events[num_events] = NULL;

    event_queue->start_events = start_events;

    _borast_freepool_init (&event_queue->pool,
                          sizeof (borast_bo_queue_event_t));
    _pqueue_init (&event_queue->pqueue);
    event_queue->pqueue.elements[PQ_FIRST_ENTRY] = NULL;
}

static borast_status_t
_borast_bo_event_queue_insert_stop (borast_bo_event_queue_t        *event_queue,
                                   borast_bo_edge_t                *edge)
{
    borast_bo_point32_t point;

    point.y = edge->edge.bottom;
    point.x = _line_compute_intersection_x_for_y (&edge->edge.line,
                                                  point.y);
    return _borast_bo_event_queue_insert (event_queue,
                                         BORAST_BO_EVENT_TYPE_STOP,
                                         edge, NULL,
                                         &point);
}

static void
_borast_bo_event_queue_fini (borast_bo_event_queue_t *event_queue)
{
    _pqueue_fini (&event_queue->pqueue);
    _borast_freepool_fini (&event_queue->pool);
}

static void
_borast_bo_sweep_line_init (borast_bo_sweep_line_t *sweep_line)
{
    sweep_line->head = NULL;
    sweep_line->stopped = NULL;
    sweep_line->current_y = INT32_MIN;
    sweep_line->current_edge = NULL;
}

static borast_status_t
_borast_bo_sweep_line_insert (borast_bo_sweep_line_t        *sweep_line,
                             borast_bo_edge_t                *edge)
{
    if (sweep_line->current_edge != NULL) {
        borast_bo_edge_t *prev, *next;
        int cmp;

        cmp = _borast_bo_sweep_line_compare_edges (sweep_line,
                                                  sweep_line->current_edge,
                                                  edge);
        if (cmp < 0) {
            prev = sweep_line->current_edge;
            next = prev->next;
            while (next != NULL &&
                   _borast_bo_sweep_line_compare_edges (sweep_line,
                                                       next, edge) < 0)
            {
                prev = next, next = prev->next;
            }

            prev->next = edge;
            edge->prev = prev;
            edge->next = next;
            if (next != NULL)
                next->prev = edge;
        } else if (cmp > 0) {
            next = sweep_line->current_edge;
            prev = next->prev;
            while (prev != NULL &&
                   _borast_bo_sweep_line_compare_edges (sweep_line,
                                                       prev, edge) > 0)
            {
                next = prev, prev = next->prev;
            }

            next->prev = edge;
            edge->next = next;
            edge->prev = prev;
            if (prev != NULL)
                prev->next = edge;
            else
                sweep_line->head = edge;
        } else {
            prev = sweep_line->current_edge;
            edge->prev = prev;
            edge->next = prev->next;
            if (prev->next != NULL)
                prev->next->prev = edge;
            prev->next = edge;
        }
    } else {
        sweep_line->head = edge;
    }

    sweep_line->current_edge = edge;

    return BORAST_STATUS_SUCCESS;
}

static void
_borast_bo_sweep_line_delete (borast_bo_sweep_line_t        *sweep_line,
                             borast_bo_edge_t        *edge)
{
    if (edge->prev != NULL)
        edge->prev->next = edge->next;
    else
        sweep_line->head = edge->next;

    if (edge->next != NULL)
        edge->next->prev = edge->prev;

    if (sweep_line->current_edge == edge)
        sweep_line->current_edge = edge->prev ? edge->prev : edge->next;
}

#if DEBUG_PRINT_STATE
static void
_borast_bo_edge_print (borast_bo_edge_t *edge)
{
    printf ("(%d, %d)-(%d, %d)",
            edge->edge.line.p1.x, edge->edge.line.p1.y,
            edge->edge.line.p2.x, edge->edge.line.p2.y);
}

static void
_borast_bo_event_print (borast_bo_event_t *event)
{
    switch (event->type) {
    case BORAST_BO_EVENT_TYPE_START:
        printf ("Start: ");
        break;
    case BORAST_BO_EVENT_TYPE_STOP:
        printf ("Stop: ");
        break;
    }
    printf ("(%d, %d)\t", event->point.x, event->point.y);
    _borast_bo_edge_print (((borast_bo_queue_event_t *)event)->e1);
    printf ("\n");
}

static void
_borast_bo_event_queue_print (borast_bo_event_queue_t *event_queue)
{
    /* XXX: fixme to print the start/stop array too. */
    printf ("Event queue:\n");
}

static void
_borast_bo_sweep_line_print (borast_bo_sweep_line_t *sweep_line)
{
    borast_bool_t first = TRUE;
    borast_bo_edge_t *edge;

    printf ("Sweep line from edge list: ");
    first = TRUE;
    for (edge = sweep_line->head;
         edge;
         edge = edge->next)
    {
        if (!first)
            printf (", ");
        _borast_bo_edge_print (edge);
        first = FALSE;
    }
    printf ("\n");
}

static void
print_state (const char                        *msg,
             borast_bo_event_t                *event,
             borast_bo_event_queue_t        *event_queue,
             borast_bo_sweep_line_t        *sweep_line)
{
    printf ("%s ", msg);
    _borast_bo_event_print (event);
    _borast_bo_event_queue_print (event_queue);
    _borast_bo_sweep_line_print (sweep_line);
    printf ("\n");
}
#endif

#if DEBUG_EVENTS
static void BORAST_PRINTF_FORMAT (1, 2)
event_log (const char *fmt, ...)
{
    FILE *file;

    if (getenv ("BORAST_DEBUG_EVENTS") == NULL)
        return;

    file = fopen ("bo-events.txt", "a");
    if (file != NULL) {
        va_list ap;

        va_start (ap, fmt);
        vfprintf (file, fmt, ap);
        va_end (ap);

        fclose (file);
    }
}
#endif

static inline borast_bool_t
edges_colinear (const borast_bo_edge_t *a, const borast_bo_edge_t *b)
{
    if (_line_equal (&a->edge.line, &b->edge.line))
        return TRUE;

    if (_slope_compare (a, b))
        return FALSE;

    /* The choice of y is not truly arbitrary since we must guarantee that it
     * is greater than the start of either line.
     */
    if (a->edge.line.p1.y == b->edge.line.p1.y) {
        return a->edge.line.p1.x == b->edge.line.p1.x;
    } else if (a->edge.line.p1.y < b->edge.line.p1.y) {
        return edge_compare_for_y_against_x (b,
                                             a->edge.line.p1.y,
                                             a->edge.line.p1.x) == 0;
    } else {
        return edge_compare_for_y_against_x (a,
                                             b->edge.line.p1.y,
                                             b->edge.line.p1.x) == 0;
    }
}

/* Adds the trapezoid, if any, of the left edge to the #borast_traps_t */
static borast_status_t
_borast_bo_edge_end_trap (borast_bo_edge_t        *left,
                         int32_t                 bot,
                         borast_traps_t          *traps)
{
    borast_bo_trap_t *trap = &left->deferred_trap;

    /* Only emit (trivial) non-degenerate trapezoids with positive height. */
    if (likely (trap->top < bot)) {
        _borast_traps_add_trap (traps,
                               trap->top, bot,
                               &left->edge.line, &trap->right->edge.line);

#if DEBUG_PRINT_STATE
        printf ("Deferred trap: left=(%d, %d)-(%d,%d) "
                "right=(%d,%d)-(%d,%d) top=%d, bot=%d\n",
                left->edge.line.p1.x, left->edge.line.p1.y,
                left->edge.line.p2.x, left->edge.line.p2.y,
                trap->right->edge.line.p1.x, trap->right->edge.line.p1.y,
                trap->right->edge.line.p2.x, trap->right->edge.line.p2.y,
                trap->top, bot);
#endif
#if DEBUG_EVENTS
        event_log ("end trap: %lu %lu %d %d\n",
                   (long) left,
                   (long) trap->right,
                   trap->top,
                   bot);
#endif
    }

    trap->right = NULL;

//    return _borast_traps_status (traps);
    return 0;
}


/* Start a new trapezoid at the given top y coordinate, whose edges
 * are `edge' and `edge->next'. If `edge' already has a trapezoid,
 * then either add it to the traps in `traps', if the trapezoid's
 * right edge differs from `edge->next', or do nothing if the new
 * trapezoid would be a continuation of the existing one. */
static inline borast_status_t
_borast_bo_edge_start_or_continue_trap (borast_bo_edge_t        *left,
                                       borast_bo_edge_t  *right,
                                       int               top,
                                       borast_traps_t        *traps)
{
    borast_status_t status;
    if (left->deferred_trap.right == right) {
        return BORAST_STATUS_SUCCESS;
    }

    if (left->deferred_trap.right != NULL) {
        if (right != NULL && edges_colinear (left->deferred_trap.right, right))
        {
            /* continuation on right, so just swap edges */
            left->deferred_trap.right = right;
            return BORAST_STATUS_SUCCESS;
        }

        status = _borast_bo_edge_end_trap (left, top, traps);
        if (unlikely (status))
            return status;
    }

    if (right != NULL && ! edges_colinear (left, right)) {
        left->deferred_trap.top = top;
        left->deferred_trap.right = right;

#if DEBUG_EVENTS
        event_log ("begin trap: %lu %lu %d\n",
                   (long) left,
                   (long) right,
                   top);
#endif
    }

    return BORAST_STATUS_SUCCESS;
}

static inline borast_status_t
_active_edges_to_traps (borast_bo_edge_t		*left,
			int32_t			 top,
			borast_fill_rule_t	 fill_rule,
			borast_traps_t	        *traps)
{
    borast_bo_edge_t *right;
    borast_status_t status;

#if DEBUG_PRINT_STATE
    printf ("Processing active edges for %d\n", top);
#endif

    if (fill_rule == BORAST_FILL_RULE_WINDING) {
	while (left != NULL) {
	    int in_out;

	    /* Greedily search for the closing edge, so that we generate the
	     * maximal span width with the minimal number of trapezoids.
	     */
	    in_out = left->edge.dir;

	    /* Check if there is a co-linear edge with an existing trap */
	    right = left->next;
	    if (left->deferred_trap.right == NULL) {
		while (right != NULL && right->deferred_trap.right == NULL)
		    right = right->next;

		if (right != NULL && edges_colinear (left, right)) {
		    /* continuation on left */
		    left->deferred_trap = right->deferred_trap;
		    right->deferred_trap.right = NULL;
		}
	    }

	    /* End all subsumed traps */
	    right = left->next;
	    while (right != NULL) {
		if (right->deferred_trap.right != NULL) {
		    status = _borast_bo_edge_end_trap (right, top, traps);
		    if (unlikely (status))
			return status;
		}

		in_out += right->edge.dir;
		if (in_out == 0) {
		    borast_bo_edge_t *next;
		    borast_bool_t skip = FALSE;

		    /* skip co-linear edges */
		    next = right->next;
		    if (next != NULL)
			skip = edges_colinear (right, next);

		    if (! skip)
			break;
		}

		right = right->next;
	    }

	    status = _borast_bo_edge_start_or_continue_trap (left, right,
							    top, traps);
	    if (unlikely (status))
		return status;

	    left = right;
	    if (left != NULL)
		left = left->next;
	}
    } else {
	while (left != NULL) {
	    int in_out = 0;

	    right = left->next;
	    while (right != NULL) {
		if (right->deferred_trap.right != NULL) {
		    status = _borast_bo_edge_end_trap (right, top, traps);
		    if (unlikely (status))
			return status;
		}

		if ((in_out++ & 1) == 0) {
		    borast_bo_edge_t *next;
		    borast_bool_t skip = FALSE;

		    /* skip co-linear edges */
		    next = right->next;
		    if (next != NULL)
			skip = edges_colinear (right, next);

		    if (! skip)
			break;
		}

		right = right->next;
	    }

	    status = _borast_bo_edge_start_or_continue_trap (left, right,
							    top, traps);
	    if (unlikely (status))
		return status;

	    left = right;
	    if (left != NULL)
		left = left->next;
	}
    }

    return BORAST_STATUS_SUCCESS;
}

/* Execute a single pass of the Bentley-Ottmann algorithm on edges,
 * generating trapezoids according to the fill_rule and appending them
 * to traps. */
static borast_status_t
_borast_bentley_ottmann_tessellate_bo_edges (borast_bo_event_t   **start_events,
                                            int                  num_events,
                                            borast_traps_t       *traps,
                                            int                 *num_intersections)
{
    borast_status_t status = BORAST_STATUS_SUCCESS; /* silence compiler */
    int intersection_count = 0;
    borast_bo_event_queue_t event_queue;
    borast_bo_sweep_line_t sweep_line;
    borast_bo_event_t *event;
    borast_bo_edge_t *left; /* , *right; */
    borast_bo_edge_t *e1;

#if DEBUG_EVENTS
    {
        int i;

        for (i = 0; i < num_events; i++) {
            borast_bo_start_event_t *event =
                ((borast_bo_start_event_t **) start_events)[i];
            event_log ("edge: %lu (%d, %d) (%d, %d) (%d, %d) %d\n",
//                       (long) &events[i].edge,
                       (long) 666,
                       event->edge.edge.line.p1.x,
                       event->edge.edge.line.p1.y,
                       event->edge.edge.line.p2.x,
                       event->edge.edge.line.p2.y,
                       event->edge.edge.top,
                       event->edge.edge.bottom,
                       event->edge.edge.dir);
        }
    }
#endif

    _borast_bo_event_queue_init (&event_queue, start_events, num_events);
    _borast_bo_sweep_line_init (&sweep_line);

    while ((event = _borast_bo_event_dequeue (&event_queue))) {
        if (event->point.y != sweep_line.current_y) {
            for (e1 = sweep_line.stopped; e1; e1 = e1->next) {
                if (e1->deferred_trap.right != NULL) {
                    status = _borast_bo_edge_end_trap (e1,
                                                      e1->edge.bottom,
                                                      traps);
                    if (unlikely (status))
                        goto unwind;
                }
            }
            sweep_line.stopped = NULL;

            status = _active_edges_to_traps (sweep_line.head,
                                             sweep_line.current_y,
//                                             BORAST_FILL_RULE_WINDING,
                                             BORAST_FILL_RULE_EVEN_ODD,
                                             traps);
            if (unlikely (status))
                goto unwind;

            sweep_line.current_y = event->point.y;
        }

#if DEBUG_EVENTS
        event_log ("event: %d (%ld, %ld) %lu, %lu\n",
                   event->type,
                   (long) event->point.x,
                   (long) event->point.y,
                   (long) ((borast_bo_queue_event_t *)event)->e1,
                   (long) ((borast_bo_queue_event_t *)event)->e2);
#endif

        switch (event->type) {
        case BORAST_BO_EVENT_TYPE_START:
            e1 = &((borast_bo_start_event_t *) event)->edge;

            status = _borast_bo_sweep_line_insert (&sweep_line, e1);
            if (unlikely (status))
                goto unwind;

            status = _borast_bo_event_queue_insert_stop (&event_queue, e1);
            if (unlikely (status))
                goto unwind;

            /* check to see if this is a continuation of a stopped edge */
            /* XXX change to an infinitesimal lengthening rule */
            for (left = sweep_line.stopped; left; left = left->next) {
                if (e1->edge.top <= left->edge.bottom &&
                    edges_colinear (e1, left))
                {
                    e1->deferred_trap = left->deferred_trap;
                    if (left->prev != NULL)
                        left->prev = left->next;
                    else
                        sweep_line.stopped = left->next;
                    if (left->next != NULL)
                        left->next->prev = left->prev;
                    break;
                }
            }

            left = e1->prev;
            /* right = e1->next */;

            break;

        case BORAST_BO_EVENT_TYPE_STOP:
            e1 = ((borast_bo_queue_event_t *) event)->e1;
            _borast_bo_event_queue_delete (&event_queue, event);

            left = e1->prev;
            /* right = e1->next; */

            _borast_bo_sweep_line_delete (&sweep_line, e1);

            /* first, check to see if we have a continuation via a fresh edge */
            if (e1->deferred_trap.right != NULL) {
                e1->next = sweep_line.stopped;
                if (sweep_line.stopped != NULL)
                    sweep_line.stopped->prev = e1;
                sweep_line.stopped = e1;
                e1->prev = NULL;
            }

            break;

        }
    }

    *num_intersections = intersection_count;
    for (e1 = sweep_line.stopped; e1; e1 = e1->next) {
        if (e1->deferred_trap.right != NULL) {
            status = _borast_bo_edge_end_trap (e1, e1->edge.bottom, traps);
            if (unlikely (status))
                break;
        }
    }
 unwind:
    _borast_bo_event_queue_fini (&event_queue);

#if DEBUG_EVENTS
    event_log ("\n");
#endif

    return status;
}

static void
contour_to_start_events (PLINE                   *contour,
                         borast_bo_start_event_t  *events,
                         borast_bo_event_t       **event_ptrs,
                         int                     *counter,
                         int                      outer_contour)
{
    int i = *counter;
    VNODE *bv;

    /* Loop over nodes, adding edges */
    bv = &contour->head;
    do {
      int x1, y1, x2, y2;
      borast_edge_t borast_edge;
      /* Node is between bv->point[0,1] and bv->next->point[0,1] */

      if (bv->point[1] == bv->next->point[1]) {
          if (bv->point[0] < bv->next->point[0]) {
            x1 = bv->point[0];
            y1 = bv->point[1];
            x2 = bv->next->point[0];
            y2 = bv->next->point[1];
          } else {
            x1 = bv->next->point[0];
            y1 = bv->next->point[1];
            x2 = bv->point[0];
            y2 = bv->point[1];
          }
      } else if (bv->point[1] < bv->next->point[1]) {
        x1 = bv->point[0];
        y1 = bv->point[1];
        x2 = bv->next->point[0];
        y2 = bv->next->point[1];
      } else {
        x1 = bv->next->point[0];
        y1 = bv->next->point[1];
        x2 = bv->point[0];
        y2 = bv->point[1];
      }

      borast_edge.line.p1.x = x1;
      borast_edge.line.p1.y = y1;
      borast_edge.line.p2.x = x2;
      borast_edge.line.p2.y = y2;
      borast_edge.top = y1;
      borast_edge.bottom = y2;
      borast_edge.dir = outer_contour ? 1 : -1;

      event_ptrs[i] = (borast_bo_event_t *) &events[i];

      events[i].type = BORAST_BO_EVENT_TYPE_START;
      events[i].point.y = borast_edge.line.p1.y;
      events[i].point.x = borast_edge.line.p1.x;

      events[i].edge.edge = borast_edge;
      events[i].edge.deferred_trap.right = NULL;
      events[i].edge.prev = NULL;
      events[i].edge.next = NULL;
      i++;

    } while ((bv = bv->next) != &contour->head);

    *counter = i;
}


static void
poly_area_to_start_events (POLYAREA                *poly,
                           borast_bo_start_event_t  *events,
                           borast_bo_event_t       **event_ptrs,
                           int                     *counter)
{
    int i = *counter;
    PLINE *contour;
    POLYAREA *pa;
    int outer_contour;

    pa = poly;
    do {
      /* Loop over contours */
      outer_contour = 1;
      for (contour = pa->contours; contour != NULL; contour = contour->next) {
        contour_to_start_events (contour, events, event_ptrs,
                                 counter, outer_contour);
        outer_contour = 0;
      }
      break;

    } while ((pa = pa->f) != poly);

    *counter = i;
}


borast_status_t
bo_poly_to_traps (hidGC gc, POLYAREA *poly, borast_traps_t *traps)
{
  int intersections;
  borast_bo_start_event_t stack_events[BORAST_STACK_ARRAY_LENGTH (borast_bo_start_event_t)];
  borast_bo_start_event_t *events;
  borast_bo_event_t *stack_event_ptrs[ARRAY_LENGTH (stack_events) + 1];
  borast_bo_event_t **event_ptrs;
  int num_events = 0;
  int i;
  int n;
  POLYAREA *pa;
  PLINE *contour;

  pa = poly;
  do {
    for (contour = pa->contours; contour != NULL; contour = contour->next)
      num_events += contour->Count;
    /* FIXME: Remove horizontal edges? */
    break;
  } while ((pa = pa->f) != poly);

  if (unlikely (0 == num_events))
      return BORAST_STATUS_SUCCESS;

  events = stack_events;
  event_ptrs = stack_event_ptrs;
  if (num_events > ARRAY_LENGTH (stack_events)) {
      events = _borast_malloc_ab_plus_c (num_events,
                                        sizeof (borast_bo_start_event_t) +
                                        sizeof (borast_bo_event_t *),
                                        sizeof (borast_bo_event_t *));
      if (unlikely (events == NULL))
          return BORAST_STATUS_NO_MEMORY;

      event_ptrs = (borast_bo_event_t **) (events + num_events);
  }

  i = 0;

  poly_area_to_start_events (poly, events, event_ptrs, &i);

  /* XXX: This would be the convenient place to throw in multiple
   * passes of the Bentley-Ottmann algorithm. It would merely
   * require storing the results of each pass into a temporary
   * borast_traps_t. */
  _borast_bentley_ottmann_tessellate_bo_edges (event_ptrs,
                                               num_events,
                                               traps,
                                               &intersections);

  for (n = 0; n < traps->num_traps; n++) {
    int x1, y1, x2, y2, x3, y3, x4, y4;

    x1 = _line_compute_intersection_x_for_y (&traps->traps[n].left, traps->traps[n].top);
    y1 = traps->traps[n].top;
    x2 = _line_compute_intersection_x_for_y (&traps->traps[n].right, traps->traps[n].top);
    y2 = traps->traps[n].top;
    x3 = _line_compute_intersection_x_for_y (&traps->traps[n].right, traps->traps[n].bottom);
    y3 = traps->traps[n].bottom;
    x4 = _line_compute_intersection_x_for_y (&traps->traps[n].left, traps->traps[n].bottom);
    y4 = traps->traps[n].bottom;

#if 1
    if (x1 == x2) {
      hidgl_ensure_triangle_space (gc, 1);
      hidgl_add_triangle (gc, x1, y1, x3, y3, x4, y4);
    } else if (x3 == x4) {
      hidgl_ensure_triangle_space (gc, 1);
      hidgl_add_triangle (gc, x1, y1, x2, y2, x3, y3);
    } else {
      hidgl_ensure_triangle_space (gc, 2);
      hidgl_add_triangle (gc, x1, y1, x2, y2, x3, y3);
      hidgl_add_triangle (gc, x3, y3, x4, y4, x1, y1);
    }
#else
    glBegin (GL_LINES);
    glVertex2i (x1, y1); glVertex2i (x2, y2);
    glVertex2i (x2, y2); glVertex2i (x3, y3);
     glVertex2i (x3, y3); glVertex2i (x1, y1);
    glVertex2i (x3, y3); glVertex2i (x4, y4);
    glVertex2i (x4, y4); glVertex2i (x1, y1);
     glVertex2i (x1, y1); glVertex2i (x3, y3);
    glEnd ();
#endif
  }

#if DEBUG_TRAPS
  dump_traps (traps, "bo-polygon-out.txt");
#endif

  if (events != stack_events)
      free (events);

  return BORAST_STATUS_SUCCESS;
}


borast_status_t
bo_contour_to_traps (hidGC gc, PLINE *contour, borast_traps_t *traps)
{
  int intersections;
  borast_bo_start_event_t stack_events[BORAST_STACK_ARRAY_LENGTH (borast_bo_start_event_t)];
  borast_bo_start_event_t *events;
  borast_bo_event_t *stack_event_ptrs[ARRAY_LENGTH (stack_events) + 1];
  borast_bo_event_t **event_ptrs;
  int num_events = 0;
  int i;
  int n;

  num_events = contour->Count;

  if (unlikely (0 == num_events))
      return BORAST_STATUS_SUCCESS;

  events = stack_events;
  event_ptrs = stack_event_ptrs;
  if (num_events > ARRAY_LENGTH (stack_events)) {
      events = _borast_malloc_ab_plus_c (num_events,
                                        sizeof (borast_bo_start_event_t) +
                                        sizeof (borast_bo_event_t *),
                                        sizeof (borast_bo_event_t *));
      if (unlikely (events == NULL))
          return BORAST_STATUS_NO_MEMORY;

      event_ptrs = (borast_bo_event_t **) (events + num_events);
  }

  i = 0;

  contour_to_start_events (contour, events, event_ptrs, &i, 1);

  /* XXX: This would be the convenient place to throw in multiple
   * passes of the Bentley-Ottmann algorithm. It would merely
   * require storing the results of each pass into a temporary
   * borast_traps_t. */
  _borast_bentley_ottmann_tessellate_bo_edges (event_ptrs,
                                              num_events,
                                              traps,
                                              &intersections);

  for (n = 0; n < traps->num_traps; n++) {
    int x1, y1, x2, y2, x3, y3, x4, y4;

    x1 = _line_compute_intersection_x_for_y (&traps->traps[n].left, traps->traps[n].top);
    y1 = traps->traps[n].top;
    x2 = _line_compute_intersection_x_for_y (&traps->traps[n].right, traps->traps[n].top);
    y2 = traps->traps[n].top;
    x3 = _line_compute_intersection_x_for_y (&traps->traps[n].right, traps->traps[n].bottom);
    y3 = traps->traps[n].bottom;
    x4 = _line_compute_intersection_x_for_y (&traps->traps[n].left, traps->traps[n].bottom);
    y4 = traps->traps[n].bottom;

#if 1
    if (x1 == x2) {
      hidgl_ensure_triangle_space (gc, 1);
      hidgl_add_triangle (gc, x1, y1, x3, y3, x4, y4);
    } else if (x3 == x4) {
      hidgl_ensure_triangle_space (gc, 1);
      hidgl_add_triangle (gc, x1, y1, x2, y2, x3, y3);
    } else {
      hidgl_ensure_triangle_space (gc, 2);
      hidgl_add_triangle (gc, x1, y1, x2, y2, x3, y3);
      hidgl_add_triangle (gc, x3, y3, x4, y4, x1, y1);
    }
#else
    glBegin (GL_LINES);
    glVertex2i (x1, y1); glVertex2i (x2, y2);
    glVertex2i (x2, y2); glVertex2i (x3, y3);
     glVertex2i (x3, y3); glVertex2i (x1, y1);
    glVertex2i (x3, y3); glVertex2i (x4, y4);
    glVertex2i (x4, y4); glVertex2i (x1, y1);
     glVertex2i (x1, y1); glVertex2i (x3, y3);
    glEnd ();
#endif
  }

#if DEBUG_TRAPS
  dump_traps (traps, "bo-polygon-out.txt");
#endif

  if (events != stack_events)
      free (events);

  return BORAST_STATUS_SUCCESS;
}
