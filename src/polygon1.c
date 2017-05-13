/*!
 * \file src/polygon1.c
 *
 * \brief Polygon clipping functions.
 *
 * harry eaton implemented the algorithm described in "A Closed Set of
 * Algorithms for Performing Set Operations on Polygonal Regions in the
 * Plane" which the original code did not do.
 *
 * I also modified it for integer coordinates and faster computation.
 *
 * The license for this modified copy was switched to the GPL per term
 * (3) of the original LGPL license.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * Copyright (C) 2006 harry eaton
 *
 * based on:
 *
 * poly_Boolean: a polygon clip library
 *
 * Copyright (C) 1997  Alexey Nikitin, Michael Leonov
 *
 * (also the authors of the paper describing the actual algorithm)
 *
 * leonov@propro.iis.nsk.su
 *
 * in turn based on:
 *
 * nclip: a polygon clip library
 *
 * Copyright (C) 1993  Klamer Schutte
 *
 * All cases where original (Klamer Schutte) code is present are marked.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * <hr>
 *
 * \note How about expanding polygons so that edges can be arcs rather
 * than lines.\n
 * Consider using the third coordinate to store the radius of the arc.\n
 * The arc would pass through the vertex points.\n
 * Positive radius would indicate the arc bows left (center on right of
 * P1-P2 path).\n
 * Negative radius would put the center on the other side.\n
 * 0 radius would mean the edge is a line instead of arc.\n
 * The intersections of the two circles centered at the vertex points
 * would determine the two possible arc centers.\n
 * If P2.x > P1.x then the center with smaller Y is selected for
 * positive r.\n
 * If P2.y > P1.y then the center with greate X is selected for
 * positive r.\n
 * The vec_inters2() routine would then need to handle line-line
 * line-arc and arc-arc intersections.\n
 * Perhaps reverse tracing the arc would require look-ahead to check
 * for arcs
 */

#include	<assert.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<setjmp.h>
#include	<math.h>
#include	<string.h>

#include "global.h"
#include "pcb-printf.h"
#include "rtree.h"
#include "heap.h"

#define ROUND(a) (long)((a) > 0 ? ((a) + 0.5) : ((a) - 0.5))

#define EPSILON (1E-8)
#define IsZero(a, b) (fabs((a) - (b)) < EPSILON)

/*              L o n g   V e c t o r   S t u f f                    */

#define Vcopy(a,b) {(a)[0]=(b)[0];(a)[1]=(b)[1];}
int vect_equal (Vector v1, Vector v2);
void vect_init (Vector v, double x, double y);
void vect_sub (Vector res, Vector v2, Vector v3);

void vect_min (Vector res, Vector v2, Vector v3);
void vect_max (Vector res, Vector v2, Vector v3);

double vect_dist2 (Vector v1, Vector v2);
double vect_det2 (Vector v1, Vector v2);
double vect_len2 (Vector v1);

int vect_inters2 (Vector A, Vector B, Vector C, Vector D, Vector S1,
		  Vector S2);

/* note that a vertex v's Flags.status represents the edge defined by
 * v to v->next (i.e. the edge is forward of v)
 */
#define ISECTED 3
#define UNKNWN  0
#define INSIDE  1
#define OUTSIDE 2
#define SHARED 3
#define SHARED2 4

#define TOUCHES 99

#define NODE_LABEL(n)  ((n)->Flags.status)
#define LABEL_NODE(n,l) ((n)->Flags.status = (l))

#define error(code)  longjmp(*(e), code)

#define MemGet(ptr, type) \
  if (UNLIKELY (((ptr) = (type *)malloc(sizeof(type))) == NULL))	\
    error(err_no_memory);

#undef DEBUG_LABEL
#undef DEBUG_ALL_LABELS
#undef DEBUG_JUMP
#undef DEBUG_GATHER
#undef DEBUG_ANGLE
#undef DEBUG
#ifdef DEBUG
#define DEBUGP(...) pcb_fprintf(stderr, ## __VA_ARGS__)
#else
#define DEBUGP(...)
#endif

/* 2-Dimentional stuff */

#define Vsub2(r,a,b)	{(r)[0] = (a)[0] - (b)[0]; (r)[1] = (a)[1] - (b)[1];}
#define Vadd2(r,a,b)	{(r)[0] = (a)[0] + (b)[0]; (r)[1] = (a)[1] + (b)[1];}
#define Vsca2(r,a,s)	{(r)[0] = (a)[0] * (s); (r)[1] = (a)[1] * (s);}
#define Vcpy2(r,a)		{(r)[0] = (a)[0]; (r)[1] = (a)[1];}
#define Vequ2(a,b)		((a)[0] == (b)[0] && (a)[1] == (b)[1])
#define Vadds(r,a,b,s)	{(r)[0] = ((a)[0] + (b)[0]) * (s); (r)[1] = ((a)[1] + (b)[1]) * (s);}
#define Vswp2(a,b) { long t; \
	t = (a)[0], (a)[0] = (b)[0], (b)[0] = t; \
	t = (a)[1], (a)[1] = (b)[1], (b)[1] = t; \
}

#ifdef DEBUG
static char *theState (VNODE * v);

static void
pline_dump (VNODE * v)
{
  VNODE *s, *n;

  s = v;
  do
    {
      n = v->next;
      pcb_fprintf (stderr, "Line [%#mS %#mS %#mS %#mS 10 10 \"%s\"]\n",
	       v->point[0], v->point[1],
	       n->point[0], n->point[1], theState (v));
    }
  while ((v = v->next) != s);
}

static void
poly_dump (POLYAREA * p)
{
  POLYAREA *f = p;
  PLINE *pl;

  do
    {
      pl = p->contours;
      do
        {
          pline_dump (&pl->head);
          fprintf (stderr, "NEXT PLINE\n");
        }
      while ((pl = pl->next) != NULL);
      fprintf (stderr, "NEXT POLY\n");
    }
  while ((p = p->f) != f);
}
#endif

/* routines for processing intersections */

/*!
 * \brief node_add.
 *
 * (C) 1993 Klamer Schutte.
 *
 * (C) 1997 Alexey Nikitin, Michael Leonov.
 *
 * (C) 2006 harry eaton.
 *
 * \return a bit field in new_point that indicates where the point was.
 * 1 means a new node was created and inserted.
 * 4 means the intersection was not on the dest point.
 */
static VNODE *
node_add_single (VNODE * dest, Vector po)
{
  VNODE *p;

  if (vect_equal (po, dest->point))
    return dest;
  if (vect_equal (po, dest->next->point))
    return dest->next;
  p = poly_CreateNode (po);
  if (p == NULL)
    return NULL;
  p->cvc_prev = p->cvc_next = NULL;
  p->Flags.status = UNKNWN;
  return p;
}				/* node_add */

#define ISECT_BAD_PARAM (-1)
#define ISECT_NO_MEMORY (-2)


/*!
 * \brief new_descriptor.
 *
 * (C) 2006 harry eaton.
 */
static CVCList *
new_descriptor (VNODE * a, char poly, char side)
{
  CVCList *l = (CVCList *) malloc (sizeof (CVCList));
  Vector v;
  register double ang, dx, dy;

  if (!l)
    return NULL;
  l->head = NULL;
  l->parent = a;
  l->poly = poly;
  l->side = side;
  l->next = l->prev = l;
  if (side == 'P')		/* previous */
    vect_sub (v, a->prev->point, a->point);
  else				/* next */
    vect_sub (v, a->next->point, a->point);
  /* Uses slope/(slope+1) in quadrant 1 as a proxy for the angle.
   * It still has the same monotonic sort result
   * and is far less expensive to compute than the real angle.
   */
  if (vect_equal (v, vect_zero))
    {
      if (side == 'P')
	{
	  if (a->prev->cvc_prev == (CVCList *) - 1)
	    a->prev->cvc_prev = a->prev->cvc_next = NULL;
	  poly_ExclVertex (a->prev);
	  vect_sub (v, a->prev->point, a->point);
	}
      else
	{
	  if (a->next->cvc_prev == (CVCList *) - 1)
	    a->next->cvc_prev = a->next->cvc_next = NULL;
	  poly_ExclVertex (a->next);
	  vect_sub (v, a->next->point, a->point);
	}
    }
  assert (!vect_equal (v, vect_zero));
  dx = fabs ((double) v[0]);
  dy = fabs ((double) v[1]);
  ang = dy / (dy + dx);
  /* now move to the actual quadrant */
  if (v[0] < 0 && v[1] >= 0)
    ang = 2.0 - ang;		/* 2nd quadrant */
  else if (v[0] < 0 && v[1] < 0)
    ang += 2.0;			/* 3rd quadrant */
  else if (v[0] >= 0 && v[1] < 0)
    ang = 4.0 - ang;		/* 4th quadrant */
  l->angle = ang;
  assert (ang >= 0.0 && ang <= 4.0);
#ifdef DEBUG_ANGLE
  DEBUGP ("node on %c at %#mD assigned angle %g on side %c\n", poly,
	  a->point[0], a->point[1], ang, side);
#endif
  return l;
}

/*!
 * \brief insert_descriptor.
 *
 * (C) 2006 harry eaton.
 *
 * \param a is a cross-vertex node.
 * \param poly is the polygon it comes from ('A' or 'B').
 * \param side is the side this descriptor goes on ('P' for
 * previous 'N' for next).
 * \param start is the head of the list of cvclists.
 */
static CVCList *
insert_descriptor (VNODE * a, char poly, char side, CVCList * start)
{
  CVCList *l, *newone, *big, *small;

  if (!(newone = new_descriptor (a, poly, side)))
    return NULL;
  /* search for the CVCList for this point */
  if (!start)
    {
      start = newone;		/* return is also new, so we know where start is */
      start->head = newone;	/* circular list */
      return newone;
    }
  else
    {
      l = start;
      do
	{
	  assert (l->head);
	  if (l->parent->point[0] == a->point[0]
	      && l->parent->point[1] == a->point[1])
	    {			/* this CVCList is at our point */
	      start = l;
	      newone->head = l->head;
	      break;
	    }
	  if (l->head->parent->point[0] == start->parent->point[0]
	      && l->head->parent->point[1] == start->parent->point[1])
	    {
	      /* this seems to be a new point */
	      /* link this cvclist to the list of all cvclists */
	      for (; l->head != newone; l = l->next)
		l->head = newone;
	      newone->head = start;
	      return newone;
	    }
	  l = l->head;
	}
      while (1);
    }
  assert (start);
  l = big = small = start;
  do
    {
      if (l->next->angle < l->angle)	/* find start/end of list */
	{
	  small = l->next;
	  big = l;
	}
      else if (newone->angle >= l->angle && newone->angle <= l->next->angle)
	{
	  /* insert new cvc if it lies between existing points */
	  newone->prev = l;
	  newone->next = l->next;
	  l->next = l->next->prev = newone;
	  return newone;
	}
    }
  while ((l = l->next) != start);
  /* didn't find it between points, it must go on an end */
  if (big->angle <= newone->angle)
    {
      newone->prev = big;
      newone->next = big->next;
      big->next = big->next->prev = newone;
      return newone;
    }
  assert (small->angle >= newone->angle);
  newone->next = small;
  newone->prev = small->prev;
  small->prev = small->prev->next = newone;
  return newone;
}

/*!
 * \brief node_add_point.
 *
 * (C) 1993 Klamer Schutte.
 *
 * (C) 1997 Alexey Nikitin, Michael Leonov.
 *
 * \return 1 if new node in b, 2 if new node in a and 3 if new node in
 * both.
 */
static VNODE *
node_add_single_point (VNODE * a, Vector p)
{
  VNODE *next_a, *new_node;

  next_a = a->next;

  new_node = node_add_single (a, p);
  assert (new_node != NULL);

  new_node->cvc_prev = new_node->cvc_next = (CVCList *) - 1;

  if (new_node == a || new_node == next_a)
    return NULL;

  return new_node;
}				/* node_add_point */

/*!
 * \brief node_label.
 *
 * (C) 2006 harry eaton.
 */
static unsigned int
node_label (VNODE * pn)
{
  CVCList *first_l, *l;
  char this_poly;
  int region = UNKNWN;

  assert (pn);
  assert (pn->cvc_next);
  this_poly = pn->cvc_next->poly;
  /* search counter-clockwise in the cross vertex connectivity (CVC) list
   *
   * check for shared edges (that could be prev or next in the list since the angles are equal)
   * and check if this edge (pn -> pn->next) is found between the other poly's entry and exit
   */
  if (pn->cvc_next->angle == pn->cvc_next->prev->angle)
    l = pn->cvc_next->prev;
  else
    l = pn->cvc_next->next;

  first_l = l;
  while ((l->poly == this_poly) && (l != first_l->prev))
    l = l->next;
  assert (l->poly != this_poly);

  assert (l && l->angle >= 0 && l->angle <= 4.0);
  if (l->poly != this_poly)
    {
      if (l->side == 'P')
	{
	  if (l->parent->prev->point[0] == pn->next->point[0] &&
	      l->parent->prev->point[1] == pn->next->point[1])
	    {
	      region = SHARED2;
	      pn->shared = l->parent->prev;
	    }
	  else
	    region = INSIDE;
	}
      else
	{
	  if (l->angle == pn->cvc_next->angle)
	    {
	      assert (l->parent->next->point[0] == pn->next->point[0] &&
		      l->parent->next->point[1] == pn->next->point[1]);
	      region = SHARED;
	      pn->shared = l->parent;
	    }
	  else
	    region = OUTSIDE;
	}
    }
  if (region == UNKNWN)
    {
      for (l = l->next; l != pn->cvc_next; l = l->next)
	{
	  if (l->poly != this_poly)
	    {
	      if (l->side == 'P')
		region = INSIDE;
	      else
		region = OUTSIDE;
	      break;
	    }
	}
    }
  assert (region != UNKNWN);
  assert (NODE_LABEL (pn) == UNKNWN || NODE_LABEL (pn) == region);
  LABEL_NODE (pn, region);
  if (region == SHARED || region == SHARED2)
    return UNKNWN;
  return region;
}				/* node_label */

/*!
 * \brief add_descriptors.
 *
 * (C) 2006 harry eaton.
 */
static CVCList *
add_descriptors (PLINE * pl, char poly, CVCList * list)
{
  VNODE *node = &pl->head;

  do
    {
      if (node->cvc_prev)
	{
	  assert (node->cvc_prev == (CVCList *) - 1
		  && node->cvc_next == (CVCList *) - 1);
	  list = node->cvc_prev = insert_descriptor (node, poly, 'P', list);
	  if (!node->cvc_prev)
	    return NULL;
	  list = node->cvc_next = insert_descriptor (node, poly, 'N', list);
	  if (!node->cvc_next)
	    return NULL;
	}
    }
  while ((node = node->next) != &pl->head);
  return list;
}

static inline void
cntrbox_adjust (PLINE * c, Vector p)
{
  c->xmin = min (c->xmin, p[0]);
  c->xmax = max (c->xmax, p[0] + 1);
  c->ymin = min (c->ymin, p[1]);
  c->ymax = max (c->ymax, p[1] + 1);
}

/* some structures for handling segment intersections using the rtrees */

typedef struct seg
{
  BoxType box;
  VNODE *v;
  PLINE *p;
  int intersected;
} seg;

typedef struct _insert_node_task insert_node_task;

struct _insert_node_task
{
  insert_node_task *next;
  seg * node_seg;
  VNODE *new_node;
};

typedef struct info
{
  double m, b;
  rtree_t *tree;
  VNODE *v;
  struct seg *s;
  jmp_buf *env, sego, *touch;
  int need_restart;
  insert_node_task *node_insert_list;
} info;

typedef struct contour_info
{
  PLINE *pa;
  jmp_buf restart;
  jmp_buf *getout;
  int need_restart;
  insert_node_task *node_insert_list;
} contour_info;


/*!
 * \brief adjust_tree().
 *
 * (C) 2006 harry eaton.
 *
 * This replaces the segment in the tree with the two new segments after
 * a vertex has been added.
 */
static int
adjust_tree (rtree_t * tree, struct seg *s)
{
  struct seg *q;

  q = (seg *)malloc (sizeof (struct seg));
  if (!q)
    return 1;
  q->intersected = 0;
  q->v = s->v;
  q->p = s->p;
  q->box.X1 = min (q->v->point[0], q->v->next->point[0]);
  q->box.X2 = max (q->v->point[0], q->v->next->point[0]) + 1;
  q->box.Y1 = min (q->v->point[1], q->v->next->point[1]);
  q->box.Y2 = max (q->v->point[1], q->v->next->point[1]) + 1;
  r_insert_entry (tree, (const BoxType *) q, 1);
  q = (seg *)malloc (sizeof (struct seg));
  if (!q)
    return 1;
  q->intersected = 0;
  q->v = s->v->next;
  q->p = s->p;
  q->box.X1 = min (q->v->point[0], q->v->next->point[0]);
  q->box.X2 = max (q->v->point[0], q->v->next->point[0]) + 1;
  q->box.Y1 = min (q->v->point[1], q->v->next->point[1]);
  q->box.Y2 = max (q->v->point[1], q->v->next->point[1]) + 1;
  r_insert_entry (tree, (const BoxType *) q, 1);
  r_delete_entry (tree, (const BoxType *) s);
  return 0;
}

/*!
 * \brief seg_in_region().
 *
 * (C) 2006, harry eaton.
 *
 * This prunes the search for boxes that don't intersect the segment.
 */
static int
seg_in_region (const BoxType * b, void *cl)
{
  struct info *i = (struct info *) cl;
  double y1, y2;
  /* for zero slope the search is aligned on the axis so it is already pruned */
  if (i->m == 0.)
    return 1;
  y1 = i->m * b->X1 + i->b;
  y2 = i->m * b->X2 + i->b;
  if (min (y1, y2) >= b->Y2)
    return 0;
  if (max (y1, y2) < b->Y1)
    return 0;
  return 1;			/* might intersect */
}

/*!
 * \brief Prepend a deferred node-insersion task to a list.
 */
static insert_node_task *
prepend_insert_node_task (insert_node_task *list, seg *seg, VNODE *new_node)
{
  insert_node_task *task = (insert_node_task *)malloc (sizeof (*task));
  task->node_seg = seg;
  task->new_node = new_node;
  task->next = list;
  return task;
}

/*!
 * \brief seg_in_seg().
 *
 * (C) 2006 harry eaton.
 *
 * This routine checks if the segment in the tree intersect the search
 * segment.
 * If it does, the plines are marked as intersected and the point is
 * marked for the cvclist.
 * If the point is not already a vertex, a new vertex is inserted and
 * the search for intersections starts over at the beginning.
 * That is potentially a significant time penalty, but it does solve the
 * snap rounding problem.
 *
 * \todo There are efficient algorithms for finding intersections with
 * snap rounding, but I don't have time to implement them right now.
 */
static int
seg_in_seg (const BoxType * b, void *cl)
{
  struct info *i = (struct info *) cl;
  struct seg *s = (struct seg *) b;
  Vector s1, s2;
  int cnt;
  VNODE *new_node;

  /* When new nodes are added at the end of a pass due to an intersection
   * the segments may be altered. If either segment we're looking at has
   * already been intersected this pass, skip it until the next pass.
   */
  if (s->intersected || i->s->intersected)
    return 0;

  cnt = vect_inters2 (s->v->point, s->v->next->point,
		      i->v->point, i->v->next->point, s1, s2);
  if (!cnt)
    return 0;
  if (i->touch)			/* if checking touches one find and we're done */
    longjmp (*i->touch, TOUCHES);
  i->s->p->Flags.status = ISECTED;
  s->p->Flags.status = ISECTED;
  for (; cnt; cnt--)
    {
      bool done_insert_on_i = false;
      new_node = node_add_single_point (i->v, cnt > 1 ? s2 : s1);
      if (new_node != NULL)
	{
#ifdef DEBUG_INTERSECT
	  DEBUGP ("new intersection on segment \"i\" at %#mD\n",
	          cnt > 1 ? s2[0] : s1[0], cnt > 1 ? s2[1] : s1[1]);
#endif
	  i->node_insert_list =
	    prepend_insert_node_task (i->node_insert_list, i->s, new_node);
	  i->s->intersected = 1;
	  done_insert_on_i = true;
	}
      new_node = node_add_single_point (s->v, cnt > 1 ? s2 : s1);
      if (new_node != NULL)
	{
#ifdef DEBUG_INTERSECT
	  DEBUGP ("new intersection on segment \"s\" at %#mD\n",
	          cnt > 1 ? s2[0] : s1[0], cnt > 1 ? s2[1] : s1[1]);
#endif
	  i->node_insert_list =
	    prepend_insert_node_task (i->node_insert_list, s, new_node);
	  s->intersected = 1;
	  return 0; /* Keep looking for intersections with segment "i" */
	}
      /* Skip any remaining r_search hits against segment i, as any futher
       * intersections will be rejected until the next pass anyway.
       */
      if (done_insert_on_i)
	longjmp (*i->env, 1);
    }
  return 0;
}

static void *
make_edge_tree (PLINE * pb)
{
  struct seg *s;
  VNODE *bv;
  rtree_t *ans = r_create_tree (NULL, 0, 0);
  bv = &pb->head;
  do
    {
      s = (seg *)malloc (sizeof (struct seg));
      s->intersected = 0;
      if (bv->point[0] < bv->next->point[0])
	{
	  s->box.X1 = bv->point[0];
	  s->box.X2 = bv->next->point[0] + 1;
	}
      else
	{
	  s->box.X2 = bv->point[0] + 1;
	  s->box.X1 = bv->next->point[0];
	}
      if (bv->point[1] < bv->next->point[1])
	{
	  s->box.Y1 = bv->point[1];
	  s->box.Y2 = bv->next->point[1] + 1;
	}
      else
	{
	  s->box.Y2 = bv->point[1] + 1;
	  s->box.Y1 = bv->next->point[1];
	}
      s->v = bv;
      s->p = pb;
      r_insert_entry (ans, (const BoxType *) s, 1);
    }
  while ((bv = bv->next) != &pb->head);
  return (void *) ans;
}

static int
get_seg (const BoxType * b, void *cl)
{
  struct info *i = (struct info *) cl;
  struct seg *s = (struct seg *) b;
  if (i->v == s->v)
    {
      i->s = s;
      longjmp (i->sego, 1);
    }
  return 0;
}

/*!
 * \brief intersect() (and helpers).
 *
 * (C) 2006, harry eaton.
 *
 * This uses an rtree to find A-B intersections.
 * Whenever a new vertex is added, the search for intersections is
 * re-started because the rounding could alter the topology otherwise.
 * This should use a faster algorithm for snap rounding intersection
 * finding.
 * The best algorthim is probably found in:
 *
 * "Improved output-sensitive snap rounding," John Hershberger,
 * Proceedings of the 22nd annual symposium on Computational geomerty,
 * 2006, pp 357-366.
 *
 * http://doi.acm.org/10.1145/1137856.1137909
 *
 * Algorithms described by de Berg, or Goodrich or Halperin, or Hobby
 * would probably work as well.
 */

static int
contour_bounds_touch (const BoxType * b, void *cl)
{
  contour_info *c_info = (contour_info *) cl;
  PLINE *pa = c_info->pa;
  PLINE *pb = (PLINE *) b;
  PLINE *rtree_over;
  PLINE *looping_over;
  VNODE *av;			/* node iterators */
  struct info info;
  BoxType box;
  jmp_buf restart;

  /* Have seg_in_seg return to our desired location if it touches */
  info.env = &restart;
  info.touch = c_info->getout;
  info.need_restart = 0;
  info.node_insert_list = c_info->node_insert_list;

  /* Pick which contour has the fewer points, and do the loop
   * over that. The r_tree makes hit-testing against a contour
   * faster, so we want to do that on the bigger contour.
   */
  if (pa->Count < pb->Count)
    {
      rtree_over = pb;
      looping_over = pa;
    }
  else
    {
      rtree_over = pa;
      looping_over = pb;
    }

  av = &looping_over->head;
  do				/* Loop over the nodes in the smaller contour */
    {
      /* check this edge for any insertions */
      double dx;
      info.v = av;
      /* compute the slant for region trimming */
      dx = av->next->point[0] - av->point[0];
      if (dx == 0)
	info.m = 0;
      else
	{
	  info.m = (av->next->point[1] - av->point[1]) / dx;
	  info.b = av->point[1] - info.m * av->point[0];
	}
      box.X2 = (box.X1 = av->point[0]) + 1;
      box.Y2 = (box.Y1 = av->point[1]) + 1;

      /* fill in the segment in info corresponding to this node */
      if (setjmp (info.sego) == 0)
	{
	  r_search (looping_over->tree, &box, NULL, get_seg, &info);
	  assert (0);
	}

      /* If we're going to have another pass anyway, skip this */
      if (info.s->intersected && info.node_insert_list != NULL)
	continue;

      if (setjmp (restart))
	continue;

      /* NB: If this actually hits anything, we are teleported back to the beginning */
      info.tree = rtree_over->tree;
      if (info.tree)
	if (UNLIKELY (r_search (info.tree, &info.s->box,
				seg_in_region, seg_in_seg, &info)))
	  assert (0); /* XXX: Memory allocation failure */
    }
  while ((av = av->next) != &looping_over->head);

  c_info->node_insert_list = info.node_insert_list;
  if (info.need_restart)
    c_info->need_restart = 1;
  return 0;
}

static int
intersect_impl (jmp_buf * jb, POLYAREA * b, POLYAREA * a, int add)
{
  POLYAREA *t;
  PLINE *pa;
  contour_info c_info;
  int need_restart = 0;
  insert_node_task *task;
  c_info.need_restart = 0;
  c_info.node_insert_list = NULL;

  /* Search the r-tree of the object with most contours
   * We loop over the contours of "a". Swap if necessary.
   */
  if (a->contour_tree->size > b->contour_tree->size)
    {
      t = b;
      b = a;
      a = t;
    }

  for (pa = a->contours; pa; pa = pa->next)	/* Loop over the contours of POLYAREA "a" */
    {
      BoxType sb;
      jmp_buf out;
      int retval;

      c_info.getout = NULL;
      c_info.pa = pa;

      if (!add)
	{
	  retval = setjmp (out);
	  if (retval)
	    {
	      /* The intersection test short-circuited back here,
	       * we need to clean up, then longjmp to jb */
	      longjmp (*jb, retval);
	    }
	  c_info.getout = &out;
	}

      sb.X1 = pa->xmin;
      sb.Y1 = pa->ymin;
      sb.X2 = pa->xmax + 1;
      sb.Y2 = pa->ymax + 1;

      r_search (b->contour_tree, &sb, NULL, contour_bounds_touch, &c_info);
      if (c_info.need_restart)
	need_restart = 1;
    }

  /* Process any deferred node insersions */
  task = c_info.node_insert_list;
  while (task != NULL)
    {
      insert_node_task *next = task->next;

      /* Do insersion */
      task->new_node->prev = task->node_seg->v;
      task->new_node->next = task->node_seg->v->next;
      task->node_seg->v->next->prev = task->new_node;
      task->node_seg->v->next = task->new_node;
      task->node_seg->p->Count++;

      cntrbox_adjust (task->node_seg->p, task->new_node->point);
      if (adjust_tree (task->node_seg->p->tree, task->node_seg))
	assert (0); /* XXX: Memory allocation failure */

      need_restart = 1; /* Any new nodes could intersect */

      free (task);
      task = next;
    }

  return need_restart;
}

static int
intersect (jmp_buf * jb, POLYAREA * b, POLYAREA * a, int add)
{
  int call_count = 1;
  while (intersect_impl (jb, b, a, add))
    call_count++;
  return 0;
}

static void
M_POLYAREA_intersect (jmp_buf * e, POLYAREA * afst, POLYAREA * bfst, int add)
{
  POLYAREA *a = afst, *b = bfst;
  PLINE *curcA, *curcB;
  CVCList *the_list = NULL;

  if (a == NULL || b == NULL)
    error (err_bad_parm);
  do
    {
      do
	{
	  if (a->contours->xmax >= b->contours->xmin &&
	      a->contours->ymax >= b->contours->ymin &&
	      a->contours->xmin <= b->contours->xmax &&
	      a->contours->ymin <= b->contours->ymax)
	    {
	      if (UNLIKELY (intersect (e, a, b, add)))
		error (err_no_memory);
	    }
	}
      while (add && (a = a->f) != afst);
      for (curcB = b->contours; curcB != NULL; curcB = curcB->next)
	if (curcB->Flags.status == ISECTED)
	  {
	    the_list = add_descriptors (curcB, 'B', the_list);
	    if (UNLIKELY (the_list == NULL))
	      error (err_no_memory);
	  }
    }
  while (add && (b = b->f) != bfst);
  do
    {
      for (curcA = a->contours; curcA != NULL; curcA = curcA->next)
	if (curcA->Flags.status == ISECTED)
	  {
	    the_list = add_descriptors (curcA, 'A', the_list);
	    if (UNLIKELY (the_list == NULL))
	      error (err_no_memory);
	  }
    }
  while (add && (a = a->f) != afst);
}				/* M_POLYAREA_intersect */

static inline int
cntrbox_inside (PLINE * c1, PLINE * c2)
{
  assert (c1 != NULL && c2 != NULL);
  return ((c1->xmin >= c2->xmin) &&
	  (c1->ymin >= c2->ymin) &&
	  (c1->xmax <= c2->xmax) && (c1->ymax <= c2->ymax));
}

/* Routines for making labels */

static int
count_contours_i_am_inside (const BoxType * b, void *cl)
{
  PLINE *me = (PLINE *) cl;
  PLINE *check = (PLINE *) b;

  if (poly_ContourInContour (check, me))
    return 1;
  return 0;
}

/*!
 * \brief cntr_in_M_POLYAREA.
 *
 * \return poly is inside outfst ? TRUE : FALSE.
 */
static int
cntr_in_M_POLYAREA (PLINE * poly, POLYAREA * outfst, BOOLp test)
{
  POLYAREA *outer = outfst;
  heap_t *heap;

  assert (poly != NULL);
  assert (outer != NULL);

  heap = heap_create ();
  do
    {
      if (cntrbox_inside (poly, outer->contours))
	heap_insert (heap, outer->contours->area, (void *) outer);
    }
  /* if checking touching, use only the first polygon */
  while (!test && (outer = outer->f) != outfst);
  /* we need only check the smallest poly container
   * but we must loop in case the box containter is not
   * the poly container */
  do
    {
      if (heap_is_empty (heap))
	break;
      outer = (POLYAREA *) heap_remove_smallest (heap);

      switch (r_search
	      (outer->contour_tree, (BoxType *) poly, NULL,
	       count_contours_i_am_inside, poly))
	{
	case 0:		/* Didn't find anything in this piece, Keep looking */
	  break;
	case 1:		/* Found we are inside this piece, and not any of its holes */
	  heap_destroy (&heap);
	  return TRUE;
	case 2:		/* Found inside a hole in the smallest polygon so far. No need to check the other polygons */
	  heap_destroy (&heap);
	  return FALSE;
	default:
	  printf ("Something strange here\n");
	  break;
	}
    }
  while (1);
  heap_destroy (&heap);
  return FALSE;
}				/* cntr_in_M_POLYAREA */

#ifdef DEBUG

static char *
theState (VNODE * v)
{
  static char u[] = "UNKNOWN";
  static char i[] = "INSIDE";
  static char o[] = "OUTSIDE";
  static char s[] = "SHARED";
  static char s2[] = "SHARED2";

  switch (NODE_LABEL (v))
    {
    case INSIDE:
      return i;
    case OUTSIDE:
      return o;
    case SHARED:
      return s;
    case SHARED2:
      return s2;
    default:
      return u;
    }
}

#ifdef DEBUG_ALL_LABELS
static void
print_labels (PLINE * a)
{
  VNODE *c = &a->head;

  do
    {
      DEBUGP ("%#mD->%#mD labeled %s\n", c->point[0], c->point[1],
	      c->next->point[0], c->next->point[1], theState (c));
    }
  while ((c = c->next) != &a->head);
}
#endif
#endif

/*!
 * \brief label_contour.
 *
 * (C) 2006 harry eaton.
 *
 * (C) 1993 Klamer Schutte.
 *
 * (C) 1997 Alexey Nikitin, Michael Leonov.
 */

static BOOLp
label_contour (PLINE * a)
{
  VNODE *cur = &a->head;
  VNODE *first_labelled = NULL;
  int label = UNKNWN;

  do
    {
      if (cur->cvc_next)	/* examine cross vertex */
	{
	  label = node_label (cur);
	  if (first_labelled == NULL)
	    first_labelled = cur;
	  continue;
	}

      if (first_labelled == NULL)
	continue;

      /* This labels nodes which aren't cross-connected */
      assert (label == INSIDE || label == OUTSIDE);
      LABEL_NODE (cur, label);
    }
  while ((cur = cur->next) != first_labelled);
#ifdef DEBUG_ALL_LABELS
  print_labels (a);
  DEBUGP ("\n\n");
#endif
  return FALSE;
}				/* label_contour */

static BOOLp
cntr_label_POLYAREA (PLINE * poly, POLYAREA * ppl, BOOLp test)
{
  assert (ppl != NULL && ppl->contours != NULL);
  if (poly->Flags.status == ISECTED)
    {
      label_contour (poly);	/* should never get here when BOOLp is true */
    }
  else if (cntr_in_M_POLYAREA (poly, ppl, test))
    {
      if (test)
	return TRUE;
      poly->Flags.status = INSIDE;
    }
  else
    {
      if (test)
	return false;
      poly->Flags.status = OUTSIDE;
    }
  return FALSE;
}				/* cntr_label_POLYAREA */

static BOOLp
M_POLYAREA_label_separated (PLINE * afst, POLYAREA * b, BOOLp touch)
{
  PLINE *curc = afst;

  for (curc = afst; curc != NULL; curc = curc->next)
    {
      if (cntr_label_POLYAREA (curc, b, touch) && touch)
	return TRUE;
    }
  return FALSE;
}

static BOOLp
M_POLYAREA_label (POLYAREA * afst, POLYAREA * b, BOOLp touch)
{
  POLYAREA *a = afst;
  PLINE *curc;

  assert (a != NULL);
  do
    {
      for (curc = a->contours; curc != NULL; curc = curc->next)
	if (cntr_label_POLYAREA (curc, b, touch))
	  {
	    if (touch)
	      return TRUE;
	  }
    }
  while (!touch && (a = a->f) != afst);
  return FALSE;
}


/*!
 * \brief Routines for temporary storing resulting contours.
 */
static void
InsCntr (jmp_buf * e, PLINE * c, POLYAREA ** dst)
{
  POLYAREA *newp;

  if (*dst == NULL)
    {
      MemGet (*dst, POLYAREA);
      (*dst)->f = (*dst)->b = *dst;
      newp = *dst;
    }
  else
    {
      MemGet (newp, POLYAREA);
      newp->f = *dst;
      newp->b = (*dst)->b;
      newp->f->b = newp->b->f = newp;
    }
  newp->contours = c;
  newp->contour_tree = r_create_tree (NULL, 0, 0);
  r_insert_entry (newp->contour_tree, (BoxType *) c, 0);
  c->next = NULL;
}				/* InsCntr */

static void
PutContour (jmp_buf * e, PLINE * cntr, POLYAREA ** contours, PLINE ** holes,
	    POLYAREA * owner, POLYAREA * parent, PLINE * parent_contour)
{
  assert (cntr != NULL);
  assert (cntr->Count > 2);
  cntr->next = NULL;

  if (cntr->Flags.orient == PLF_DIR)
    {
      if (owner != NULL)
	r_delete_entry (owner->contour_tree, (BoxType *) cntr);
      InsCntr (e, cntr, contours);
    }
  /* put hole into temporary list */
  else
    {
      /* if we know this belongs inside the parent, put it there now */
      if (parent_contour)
	{
	  cntr->next = parent_contour->next;
	  parent_contour->next = cntr;
	  if (owner != parent)
	    {
	      if (owner != NULL)
		r_delete_entry (owner->contour_tree, (BoxType *) cntr);
	      r_insert_entry (parent->contour_tree, (BoxType *) cntr, 0);
	    }
	}
      else
	{
	  cntr->next = *holes;
	  *holes = cntr;	/* let cntr be 1st hole in list */
	  /* We don't insert the holes into an r-tree,
	   * they just form a linked list */
	  if (owner != NULL)
	    r_delete_entry (owner->contour_tree, (BoxType *) cntr);
	}
    }
}				/* PutContour */

static inline void
remove_contour (POLYAREA * piece, PLINE * prev_contour, PLINE * contour,
		int remove_rtree_entry)
{
  if (piece->contours == contour)
    piece->contours = contour->next;
  else if (prev_contour != NULL)
    {
      assert (prev_contour->next == contour);
      prev_contour->next = contour->next;
    }

  contour->next = NULL;

  if (remove_rtree_entry)
    r_delete_entry (piece->contour_tree, (BoxType *) contour);
}

struct polyarea_info
{
  BoxType BoundingBox;
  POLYAREA *pa;
};

static int
heap_it (const BoxType * b, void *cl)
{
  heap_t *heap = (heap_t *) cl;
  struct polyarea_info *pa_info = (struct polyarea_info *) b;
  PLINE *p = pa_info->pa->contours;
  if (p->Count == 0)
    return 0;			/* how did this happen? */
  heap_insert (heap, p->area, pa_info);
  return 1;
}

struct find_inside_info
{
  jmp_buf jb;
  PLINE *want_inside;
  PLINE *result;
};

static int
find_inside (const BoxType * b, void *cl)
{
  struct find_inside_info *info = (struct find_inside_info *) cl;
  PLINE *check = (PLINE *) b;
  /* Do test on check to see if it inside info->want_inside */
  /* If it is: */
  if (check->Flags.orient == PLF_DIR)
    {
      return 0;
    }
  if (poly_ContourInContour (info->want_inside, check))
    {
      info->result = check;
      longjmp (info->jb, 1);
    }
  return 0;
}

static void
InsertHoles (jmp_buf * e, POLYAREA * dest, PLINE ** src)
{
  POLYAREA *curc;
  PLINE *curh, *container;
  heap_t *heap;
  rtree_t *tree;
  int i;
  int num_polyareas = 0;
  struct polyarea_info *all_pa_info, *pa_info;

  if (*src == NULL)
    return;			/* empty hole list */
  if (dest == NULL)
    error (err_bad_parm);	/* empty contour list */

  /* Count dest polyareas */
  curc = dest;
  do
    {
      num_polyareas++;
    }
  while ((curc = curc->f) != dest);

  /* make a polyarea info table */
  /* make an rtree of polyarea info table */
  all_pa_info = (struct polyarea_info *) malloc (sizeof (struct polyarea_info) * num_polyareas);
  tree = r_create_tree (NULL, 0, 0);
  i = 0;
  curc = dest;
  do
    {
      all_pa_info[i].BoundingBox.X1 = curc->contours->xmin;
      all_pa_info[i].BoundingBox.Y1 = curc->contours->ymin;
      all_pa_info[i].BoundingBox.X2 = curc->contours->xmax;
      all_pa_info[i].BoundingBox.Y2 = curc->contours->ymax;
      all_pa_info[i].pa = curc;
      r_insert_entry (tree, (const BoxType *) &all_pa_info[i], 0);
      i++;
    }
  while ((curc = curc->f) != dest);

  /* loop through the holes and put them where they belong */
  while ((curh = *src) != NULL)
    {
      *src = curh->next;

      container = NULL;
      /* build a heap of all of the polys that the hole is inside its bounding box */
      heap = heap_create ();
      r_search (tree, (BoxType *) curh, NULL, heap_it, heap);
      if (heap_is_empty (heap))
	{
#ifndef NDEBUG
#ifdef DEBUG
	  poly_dump (dest);
#endif
#endif
	  poly_DelContour (&curh);
	  error (err_bad_parm);
	}
      /* Now search the heap for the container. If there was only one item
       * in the heap, assume it is the container without the expense of
       * proving it.
       */
      pa_info = (struct polyarea_info *) heap_remove_smallest (heap);
      if (heap_is_empty (heap))
	{			/* only one possibility it must be the right one */
	  assert (poly_ContourInContour (pa_info->pa->contours, curh));
	  container = pa_info->pa->contours;
	}
      else
	{
	  do
	    {
	      if (poly_ContourInContour (pa_info->pa->contours, curh))
		{
		  container = pa_info->pa->contours;
		  break;
		}
	      if (heap_is_empty (heap))
		break;
	      pa_info = (struct polyarea_info *) heap_remove_smallest (heap);
	    }
	  while (1);
	}
      heap_destroy (&heap);
      if (container == NULL)
	{
	  /* bad input polygons were given */
#ifndef NDEBUG
#ifdef DEBUG
	  poly_dump (dest);
#endif
#endif
	  curh->next = NULL;
	  poly_DelContour (&curh);
	  error (err_bad_parm);
	}
      else
	{
	  /* Need to check if this new hole means we need to kick out any old ones for reprocessing */
	  while (1)
	    {
	      struct find_inside_info info;
	      PLINE *prev;

	      info.want_inside = curh;

	      /* Set jump return */
	      if (setjmp (info.jb))
		{
		  /* Returned here! */
		}
	      else
		{
		  info.result = NULL;
		  /* Rtree search, calling back a routine to longjmp back with data about any hole inside the added one */
		  /*   Be sure not to bother jumping back to report the main contour! */
		  r_search (pa_info->pa->contour_tree, (BoxType *) curh, NULL,
			    find_inside, &info);

		  /* Nothing found? */
		  break;
		}

	      /* We need to find the contour before it, so we can update its next pointer */
	      prev = container;
	      while (prev->next != info.result)
		{
		  prev = prev->next;
		}

	      /* Remove hole from the contour */
	      remove_contour (pa_info->pa, prev, info.result, TRUE);

	      /* Add hole as the next on the list to be processed in this very function */
	      info.result->next = *src;
	      *src = info.result;
	    }
	  /* End check for kicked out holes */

	  /* link at front of hole list */
	  curh->next = container->next;
	  container->next = curh;
	  r_insert_entry (pa_info->pa->contour_tree, (BoxType *) curh, 0);

	}
    }
  r_destroy_tree (&tree);
  free (all_pa_info);
}				/* InsertHoles */


/* routines for collecting result */

typedef enum
{
  FORW, BACKW
} DIRECTION;

/*!
 * \brief Start Rule.
 */
typedef int (*S_Rule) (VNODE *, DIRECTION *);

/*!
 * \brief Jump Rule.
 */
typedef int (*J_Rule) (char, VNODE *, DIRECTION *);

static int
UniteS_Rule (VNODE * cur, DIRECTION * initdir)
{
  *initdir = FORW;
  return (NODE_LABEL (cur) == OUTSIDE) || (NODE_LABEL (cur) == SHARED);
}

static int
IsectS_Rule (VNODE * cur, DIRECTION * initdir)
{
  *initdir = FORW;
  return (NODE_LABEL (cur) == INSIDE) || (NODE_LABEL (cur) == SHARED);
}

static int
SubS_Rule (VNODE * cur, DIRECTION * initdir)
{
  *initdir = FORW;
  return (NODE_LABEL (cur) == OUTSIDE) || (NODE_LABEL (cur) == SHARED2);
}

static int
XorS_Rule (VNODE * cur, DIRECTION * initdir)
{
  if (cur->Flags.status == INSIDE)
    {
      *initdir = BACKW;
      return TRUE;
    }
  if (cur->Flags.status == OUTSIDE)
    {
      *initdir = FORW;
      return TRUE;
    }
  return FALSE;
}

static int
IsectJ_Rule (char p, VNODE * v, DIRECTION * cdir)
{
  assert (*cdir == FORW);
  return (v->Flags.status == INSIDE || v->Flags.status == SHARED);
}

static int
UniteJ_Rule (char p, VNODE * v, DIRECTION * cdir)
{
  assert (*cdir == FORW);
  return (v->Flags.status == OUTSIDE || v->Flags.status == SHARED);
}

static int
XorJ_Rule (char p, VNODE * v, DIRECTION * cdir)
{
  if (v->Flags.status == INSIDE)
    {
      *cdir = BACKW;
      return TRUE;
    }
  if (v->Flags.status == OUTSIDE)
    {
      *cdir = FORW;
      return TRUE;
    }
  return FALSE;
}

static int
SubJ_Rule (char p, VNODE * v, DIRECTION * cdir)
{
  if (p == 'B' && v->Flags.status == INSIDE)
    {
      *cdir = BACKW;
      return TRUE;
    }
  if (p == 'A' && v->Flags.status == OUTSIDE)
    {
      *cdir = FORW;
      return TRUE;
    }
  if (v->Flags.status == SHARED2)
    {
      if (p == 'A')
	*cdir = FORW;
      else
	*cdir = BACKW;
      return TRUE;
    }
  return FALSE;
}

/*!
 * \brief Return the edge that comes next.
 *
 * If the direction is BACKW, then we return the next vertex so that
 * prev vertex has the flags for the edge.
 *
 * \return true if an edge is found, false otherwise.
 */
static int
jump (VNODE ** cur, DIRECTION * cdir, J_Rule rule)
{
  CVCList *d, *start;
  VNODE *e;
  DIRECTION newone;

  if (!(*cur)->cvc_prev)	/* not a cross-vertex */
    {
      if (*cdir == FORW ? (*cur)->Flags.mark : (*cur)->prev->Flags.mark)
	return FALSE;
      return TRUE;
    }
#ifdef DEBUG_JUMP
  DEBUGP ("jump entering node at %$mD\n", (*cur)->point[0], (*cur)->point[1]);
#endif
  if (*cdir == FORW)
    d = (*cur)->cvc_prev->prev;
  else
    d = (*cur)->cvc_next->prev;
  start = d;
  do
    {
      e = d->parent;
      if (d->side == 'P')
	e = e->prev;
      newone = *cdir;
      if (!e->Flags.mark && rule (d->poly, e, &newone))
	{
	  if ((d->side == 'N' && newone == FORW) ||
	      (d->side == 'P' && newone == BACKW))
	    {
#ifdef DEBUG_JUMP
	      if (newone == FORW)
		DEBUGP ("jump leaving node at %#mD\n",
			e->next->point[0], e->next->point[1]);
	      else
		DEBUGP ("jump leaving node at %#mD\n",
			e->point[0], e->point[1]);
#endif
	      *cur = d->parent;
	      *cdir = newone;
	      return TRUE;
	    }
	}
    }
  while ((d = d->prev) != start);
  return FALSE;
}

static int
Gather (VNODE * start, PLINE ** result, J_Rule v_rule, DIRECTION initdir)
{
  VNODE *cur = start, *newn;
  DIRECTION dir = initdir;
#ifdef DEBUG_GATHER
  DEBUGP ("gather direction = %d\n", dir);
#endif
  assert (*result == NULL);
  do
    {
      /* see where to go next */
      if (!jump (&cur, &dir, v_rule))
	break;
      /* add edge to polygon */
      if (!*result)
	{
	  *result = poly_NewContour (cur->point);
	  if (*result == NULL)
	    return err_no_memory;
	}
      else
	{
	  if ((newn = poly_CreateNode (cur->point)) == NULL)
	    return err_no_memory;
	  poly_InclVertex ((*result)->head.prev, newn);
	}
#ifdef DEBUG_GATHER
      DEBUGP ("gather vertex at %#mD\n", cur->point[0], cur->point[1]);
#endif
      /* Now mark the edge as included.  */
      newn = (dir == FORW ? cur : cur->prev);
      newn->Flags.mark = 1;
      /* for SHARED edge mark both */
      if (newn->shared)
	newn->shared->Flags.mark = 1;

      /* Advance to the next edge.  */
      cur = (dir == FORW ? cur->next : cur->prev);
    }
  while (1);
  return err_ok;
}				/* Gather */

static void
Collect1 (jmp_buf * e, VNODE * cur, DIRECTION dir, POLYAREA ** contours,
	  PLINE ** holes, J_Rule j_rule)
{
  PLINE *p = NULL;		/* start making contour */
  int errc = err_ok;
  if ((errc =
       Gather (cur, &p, j_rule, dir)) != err_ok)
    {
      if (p != NULL)
	poly_DelContour (&p);
      error (errc);
    }
  if (!p)
    return;
  poly_PreContour (p, TRUE);
  if (p->Count > 2)
    {
#ifdef DEBUG_GATHER
      DEBUGP ("adding contour with %d verticies and direction %c\n",
	      p->Count, p->Flags.orient ? 'F' : 'B');
#endif
      PutContour (e, p, contours, holes, NULL, NULL, NULL);
    }
  else
    {
      /* some sort of computation error ? */
#ifdef DEBUG_GATHER
      DEBUGP ("Bad contour! Not enough points!!\n");
#endif
      poly_DelContour (&p);
    }
}

static void
Collect (jmp_buf * e, PLINE * a, POLYAREA ** contours, PLINE ** holes,
	 S_Rule s_rule, J_Rule j_rule)
{
  VNODE *cur, *other;
  DIRECTION dir;

  cur = &a->head;
  do
    {
      dir = FORW;	/* avoid uninitialized variable with XOR rule (side note: XOR not used in PCB anyway) */
      if (s_rule (cur, &dir) && cur->Flags.mark == 0)
	Collect1 (e, dir == FORW ? cur : cur->next, dir, contours, holes, j_rule);
		/* Note: when the direction is not FORW, move to the vertex, Gather() should actually start from. */
      other = cur;
      if ((other->cvc_prev && jump (&other, &dir, j_rule)))
	Collect1 (e, other, dir, contours, holes, j_rule);
    }
  while ((cur = cur->next) != &a->head);
}				/* Collect */


static int
cntr_Collect (jmp_buf * e, PLINE ** A, POLYAREA ** contours, PLINE ** holes,
	      int action, POLYAREA * owner, POLYAREA * parent,
	      PLINE * parent_contour)
{
  PLINE *tmprev;

  if ((*A)->Flags.status == ISECTED)
    {
      switch (action)
	{
	case PBO_UNITE:
	  Collect (e, *A, contours, holes, UniteS_Rule, UniteJ_Rule);
	  break;
	case PBO_ISECT:
	  Collect (e, *A, contours, holes, IsectS_Rule, IsectJ_Rule);
	  break;
	case PBO_XOR:
	  Collect (e, *A, contours, holes, XorS_Rule, XorJ_Rule);
	  break;
	case PBO_SUB:
	  Collect (e, *A, contours, holes, SubS_Rule, SubJ_Rule);
	  break;
	};
    }
  else
    {
      switch (action)
	{
	case PBO_ISECT:
	  if ((*A)->Flags.status == INSIDE)
	    {
	      tmprev = *A;
	      /* disappear this contour (rtree entry removed in PutContour) */
	      *A = tmprev->next;
	      tmprev->next = NULL;
	      PutContour (e, tmprev, contours, holes, owner, NULL, NULL);
	      return TRUE;
	    }
	  break;
	case PBO_XOR:
	  if ((*A)->Flags.status == INSIDE)
	    {
	      tmprev = *A;
	      /* disappear this contour (rtree entry removed in PutContour) */
	      *A = tmprev->next;
	      tmprev->next = NULL;
	      poly_InvContour (tmprev);
	      PutContour (e, tmprev, contours, holes, owner, NULL, NULL);
	      return TRUE;
	    }
	  /* break; *//* Fall through (I think this is correct! pcjc2) */
	case PBO_UNITE:
	case PBO_SUB:
	  if ((*A)->Flags.status == OUTSIDE)
	    {
	      tmprev = *A;
	      /* disappear this contour (rtree entry removed in PutContour) */
	      *A = tmprev->next;
	      tmprev->next = NULL;
	      PutContour (e, tmprev, contours, holes, owner, parent,
			  parent_contour);
	      return TRUE;
	    }
	  break;
	}
    }
  return FALSE;
}				/* cntr_Collect */

static void
M_B_AREA_Collect (jmp_buf * e, POLYAREA * bfst, POLYAREA ** contours,
		  PLINE ** holes, int action)
{
  POLYAREA *b = bfst;
  PLINE **cur, **next, *tmp;

  assert (b != NULL);
  do
    {
      for (cur = &b->contours; *cur != NULL; cur = next)
	{
	  next = &((*cur)->next);
	  if ((*cur)->Flags.status == ISECTED)
	    continue;

	  if ((*cur)->Flags.status == INSIDE)
	    switch (action)
	      {
	      case PBO_XOR:
	      case PBO_SUB:
		poly_InvContour (*cur);
	      case PBO_ISECT:
		tmp = *cur;
		*cur = tmp->next;
		next = cur;
		tmp->next = NULL;
		tmp->Flags.status = UNKNWN;
		PutContour (e, tmp, contours, holes, b, NULL, NULL);
		break;
	      case PBO_UNITE:
		break;		/* nothing to do - already included */
	      }
	  else if ((*cur)->Flags.status == OUTSIDE)
	    switch (action)
	      {
	      case PBO_XOR:
	      case PBO_UNITE:
		/* include */
		tmp = *cur;
		*cur = tmp->next;
		next = cur;
		tmp->next = NULL;
		tmp->Flags.status = UNKNWN;
		PutContour (e, tmp, contours, holes, b, NULL, NULL);
		break;
	      case PBO_ISECT:
	      case PBO_SUB:
		break;		/* do nothing, not included */
	      }
	}
    }
  while ((b = b->f) != bfst);
}


static inline int
contour_is_first (POLYAREA * a, PLINE * cur)
{
  return (a->contours == cur);
}


static inline int
contour_is_last (PLINE * cur)
{
  return (cur->next == NULL);
}


static inline void
remove_polyarea (POLYAREA ** list, POLYAREA * piece)
{
  /* If this item was the start of the list, advance that pointer */
  if (*list == piece)
    *list = (*list)->f;

  /* But reset it to NULL if it wraps around and hits us again */
  if (*list == piece)
    *list = NULL;

  piece->b->f = piece->f;
  piece->f->b = piece->b;
  piece->f = piece->b = piece;
}

static void
M_POLYAREA_separate_isected (jmp_buf * e, POLYAREA ** pieces,
			     PLINE ** holes, PLINE ** isected)
{
  POLYAREA *a = *pieces;
  POLYAREA *anext;
  PLINE *curc, *next, *prev;
  int finished;

  if (a == NULL)
    return;

  /* TODO: STASH ENOUGH INFORMATION EARLIER ON, SO WE CAN REMOVE THE INTERSECTED
     CONTOURS WITHOUT HAVING TO WALK THE FULL DATA-STRUCTURE LOOKING FOR THEM. */

  do
    {
      int hole_contour = 0;
      int is_outline = 1;

      anext = a->f;
      finished = (anext == *pieces);

      prev = NULL;
      for (curc = a->contours; curc != NULL; curc = next, is_outline = 0)
	{
	  int is_first = contour_is_first (a, curc);
	  int is_last = contour_is_last (curc);
	  int isect_contour = (curc->Flags.status == ISECTED);

	  next = curc->next;

	  if (isect_contour || hole_contour)
	    {

	      /* Reset the intersection flags, since we keep these pieces */
	      if (curc->Flags.status != ISECTED)
		curc->Flags.status = UNKNWN;

	      remove_contour (a, prev, curc, !(is_first && is_last));

	      if (isect_contour)
		{
		  /* Link into the list of intersected contours */
		  curc->next = *isected;
		  *isected = curc;
		}
	      else if (hole_contour)
		{
		  /* Link into the list of holes */
		  curc->next = *holes;
		  *holes = curc;
		}
	      else
		{
		  assert (0);
		}

	      if (is_first && is_last)
		{
		  remove_polyarea (pieces, a);
		  poly_Free (&a);	/* NB: Sets a to NULL */
		}

	    }
	  else
	    {
	      /* Note the item we just didn't delete as the next
	         candidate for having its "next" pointer adjusted.
	         Saves walking the contour list when we delete one. */
	      prev = curc;
	    }

	  /* If we move or delete an outer contour, we need to move any holes
	     we wish to keep within that contour to the holes list. */
	  if (is_outline && isect_contour)
	    hole_contour = 1;

	}

      /* If we deleted all the pieces of the polyarea, *pieces is NULL */
    }
  while ((a = anext), *pieces != NULL && !finished);
}


struct find_inside_m_pa_info
{
  jmp_buf jb;
  POLYAREA *want_inside;
  PLINE *result;
};

static int
find_inside_m_pa (const BoxType * b, void *cl)
{
  struct find_inside_m_pa_info *info = (struct find_inside_m_pa_info *) cl;
  PLINE *check = (PLINE *) b;
  /* Don't report for the main contour */
  if (check->Flags.orient == PLF_DIR)
    return 0;
  /* Don't look at contours marked as being intersected */
  if (check->Flags.status == ISECTED)
    return 0;
  if (cntr_in_M_POLYAREA (check, info->want_inside, FALSE))
    {
      info->result = check;
      longjmp (info->jb, 1);
    }
  return 0;
}


static void
M_POLYAREA_update_primary (jmp_buf * e, POLYAREA ** pieces,
			   PLINE ** holes, int action, POLYAREA * bpa)
{
  POLYAREA *a = *pieces;
  POLYAREA *b;
  POLYAREA *anext;
  PLINE *curc, *next, *prev;
  BoxType box;
  /* int inv_inside = 0; */
  int del_inside = 0;
  int del_outside = 0;
  int finished;

  if (a == NULL)
    return;

  switch (action)
    {
    case PBO_ISECT:
      del_outside = 1;
      break;
    case PBO_UNITE:
    case PBO_SUB:
      del_inside = 1;
      break;
    case PBO_XOR:		/* NOT IMPLEMENTED OR USED */
      /* inv_inside = 1; */
      assert (0);
      break;
    }

  box = *((BoxType *) bpa->contours);
  b = bpa;
  while ((b = b->f) != bpa)
    {
      BoxType *b_box = (BoxType *) b->contours;
      MAKEMIN (box.X1, b_box->X1);
      MAKEMIN (box.Y1, b_box->Y1);
      MAKEMAX (box.X2, b_box->X2);
      MAKEMAX (box.Y2, b_box->Y2);
    }

  if (del_inside)
    {

      do
	{
	  anext = a->f;
	  finished = (anext == *pieces);

	  /* Test the outer contour first, as we may need to remove all children */

	  /* We've not yet split intersected contours out, just ignore them */
	  if (a->contours->Flags.status != ISECTED &&
	      /* Pre-filter on bounding box */
	      ((a->contours->xmin >= box.X1) && (a->contours->ymin >= box.Y1)
	       && (a->contours->xmax <= box.X2)
	       && (a->contours->ymax <= box.Y2)) &&
	      /* Then test properly */
	      cntr_in_M_POLYAREA (a->contours, bpa, FALSE))
	    {

	      /* Delete this contour, all children -> holes queue */

	      /* Delete the outer contour */
	      curc = a->contours;
	      remove_contour (a, NULL, curc, FALSE);	/* Rtree deleted in poly_Free below */
	      /* a->contours now points to the remaining holes */
	      poly_DelContour (&curc);

	      if (a->contours != NULL)
		{
		  /* Find the end of the list of holes */
		  curc = a->contours;
		  while (curc->next != NULL)
		    curc = curc->next;

		  /* Take the holes and prepend to the holes queue */
		  curc->next = *holes;
		  *holes = a->contours;
		  a->contours = NULL;
		}

	      remove_polyarea (pieces, a);
	      poly_Free (&a);	/* NB: Sets a to NULL */

	      continue;
	    }

	  /* Loop whilst we find INSIDE contours to delete */
	  while (1)
	    {
	      struct find_inside_m_pa_info info;
	      PLINE *prev;

	      info.want_inside = bpa;

	      /* Set jump return */
	      if (setjmp (info.jb))
		{
		  /* Returned here! */
		}
	      else
		{
		  info.result = NULL;
		  /* r-tree search, calling back a routine to longjmp back with
		   * data about any hole inside the B polygon.
		   * NB: Does not jump back to report the main contour!
		   */
		  r_search (a->contour_tree, &box, NULL, find_inside_m_pa,
			    &info);

		  /* Nothing found? */
		  break;
		}

	      /* We need to find the contour before it, so we can update its next pointer */
	      prev = a->contours;
	      while (prev->next != info.result)
		{
		  prev = prev->next;
		}

	      /* Remove hole from the contour */
	      remove_contour (a, prev, info.result, TRUE);
	      poly_DelContour (&info.result);
	    }
	  /* End check for deleted holes */

	  /* If we deleted all the pieces of the polyarea, *pieces is NULL */
	}
      while ((a = anext), *pieces != NULL && !finished);

      return;
    }
  else
    {
      /* This path isn't optimised for speed */
    }

  do
    {
      int hole_contour = 0;
      int is_outline = 1;

      anext = a->f;
      finished = (anext == *pieces);

      prev = NULL;
      for (curc = a->contours; curc != NULL; curc = next, is_outline = 0)
	{
	  int is_first = contour_is_first (a, curc);
	  int is_last = contour_is_last (curc);
	  int del_contour = 0;

	  next = curc->next;

	  if (del_outside)
	    del_contour = curc->Flags.status != ISECTED &&
	      !cntr_in_M_POLYAREA (curc, bpa, FALSE);

	  /* Skip intersected contours */
	  if (curc->Flags.status == ISECTED)
	    {
	      prev = curc;
	      continue;
	    }

	  /* Reset the intersection flags, since we keep these pieces */
	  curc->Flags.status = UNKNWN;

	  if (del_contour || hole_contour)
	    {

	      remove_contour (a, prev, curc, !(is_first && is_last));

	      if (del_contour)
		{
		  /* Delete the contour */
		  poly_DelContour (&curc);	/* NB: Sets curc to NULL */
		}
	      else if (hole_contour)
		{
		  /* Link into the list of holes */
		  curc->next = *holes;
		  *holes = curc;
		}
	      else
		{
		  assert (0);
		}

	      if (is_first && is_last)
		{
		  remove_polyarea (pieces, a);
		  poly_Free (&a);	/* NB: Sets a to NULL */
		}

	    }
	  else
	    {
	      /* Note the item we just didn't delete as the next
	         candidate for having its "next" pointer adjusted.
	         Saves walking the contour list when we delete one. */
	      prev = curc;
	    }

	  /* If we move or delete an outer contour, we need to move any holes
	     we wish to keep within that contour to the holes list. */
	  if (is_outline && del_contour)
	    hole_contour = 1;

	}

      /* If we deleted all the pieces of the polyarea, *pieces is NULL */
    }
  while ((a = anext), *pieces != NULL && !finished);
}

static void
M_POLYAREA_Collect_separated (jmp_buf * e, PLINE * afst, POLYAREA ** contours,
			      PLINE ** holes, int action, BOOLp maybe)
{
  PLINE **cur, **next;

  for (cur = &afst; *cur != NULL; cur = next)
    {
      next = &((*cur)->next);
      /* if we disappear a contour, don't advance twice */
      if (cntr_Collect (e, cur, contours, holes, action, NULL, NULL, NULL))
	next = cur;
    }
}

static void
M_POLYAREA_Collect (jmp_buf * e, POLYAREA * afst, POLYAREA ** contours,
		    PLINE ** holes, int action, BOOLp maybe)
{
  POLYAREA *a = afst;
  POLYAREA *parent = NULL;	/* Quiet compiler warning */
  PLINE **cur, **next, *parent_contour;

  assert (a != NULL);
  while ((a = a->f) != afst);
  /* now the non-intersect parts are collected in temp/holes */
  do
    {
      if (maybe && a->contours->Flags.status != ISECTED)
	parent_contour = a->contours;
      else
	parent_contour = NULL;

      /* Take care of the first contour - so we know if we
       * can shortcut reparenting some of its children
       */
      cur = &a->contours;
      if (*cur != NULL)
	{
	  next = &((*cur)->next);
	  /* if we disappear a contour, don't advance twice */
	  if (cntr_Collect (e, cur, contours, holes, action, a, NULL, NULL))
	    {
	      parent = (*contours)->b;	/* InsCntr inserts behind the head */
	      next = cur;
	    }
	  else
	    parent = a;
	  cur = next;
	}
      for (; *cur != NULL; cur = next)
	{
	  next = &((*cur)->next);
	  /* if we disappear a contour, don't advance twice */
	  if (cntr_Collect (e, cur, contours, holes, action, a, parent,
			    parent_contour))
	    next = cur;
	}
    }
  while ((a = a->f) != afst);
}

/*!
 * \brief Determine if two polygons touch or overlap.
 */
BOOLp
Touching (POLYAREA * a, POLYAREA * b)
{
  jmp_buf e;
  int code;

  if ((code = setjmp (e)) == 0)
    {
#ifdef DEBUG
      if (!poly_Valid (a))
	return -1;
      if (!poly_Valid (b))
	return -1;
#endif
      M_POLYAREA_intersect (&e, a, b, false);

      if (M_POLYAREA_label (a, b, TRUE))
	return TRUE;
      if (M_POLYAREA_label (b, a, TRUE))
	return TRUE;
    }
  else if (code == TOUCHES)
    return TRUE;
  return FALSE;
}

/*!
 * \brief The main clipping routines.
 */
int
poly_Boolean (const POLYAREA * a_org, const POLYAREA * b_org,
	      POLYAREA ** res, int action)
{
  POLYAREA *a = NULL, *b = NULL;

  if (!poly_M_Copy0 (&a, a_org) || !poly_M_Copy0 (&b, b_org))
    return err_no_memory;

  return poly_Boolean_free (a, b, res, action);
}				/* poly_Boolean */

/*!
 * \brief Just like poly_Boolean but frees the input polys.
 */
int
poly_Boolean_free (POLYAREA * ai, POLYAREA * bi, POLYAREA ** res, int action)
{
  POLYAREA *a = ai, *b = bi;
  PLINE *a_isected = NULL;
  PLINE *p, *holes = NULL;
  jmp_buf e;
  int code;

  *res = NULL;

  if (!a)
    {
      switch (action)
	{
	case PBO_XOR:
	case PBO_UNITE:
	  *res = bi;
	  return err_ok;
	case PBO_SUB:
	case PBO_ISECT:
	  if (b != NULL)
	    poly_Free (&b);
	  return err_ok;
	}
    }
  if (!b)
    {
      switch (action)
	{
	case PBO_SUB:
	case PBO_XOR:
	case PBO_UNITE:
	  *res = ai;
	  return err_ok;
	case PBO_ISECT:
	  if (a != NULL)
	    poly_Free (&a);
	  return err_ok;
	}
    }

  if ((code = setjmp (e)) == 0)
    {
#ifdef DEBUG
      assert (poly_Valid (a));
      assert (poly_Valid (b));
#endif

      /* intersect needs to make a list of the contours in a and b which are intersected */
      M_POLYAREA_intersect (&e, a, b, TRUE);

      /* We could speed things up a lot here if we only processed the relevant contours */
      /* NB: Relevant parts of a are labeled below */
      M_POLYAREA_label (b, a, FALSE);

      *res = a;
      M_POLYAREA_update_primary (&e, res, &holes, action, b);
      M_POLYAREA_separate_isected (&e, res, &holes, &a_isected);
      M_POLYAREA_label_separated (a_isected, b, FALSE);
      M_POLYAREA_Collect_separated (&e, a_isected, res, &holes, action,
				    FALSE);
      M_B_AREA_Collect (&e, b, res, &holes, action);
      poly_Free (&b);

      /* free a_isected */
      while ((p = a_isected) != NULL)
	{
	  a_isected = p->next;
	  poly_DelContour (&p);
	}

      InsertHoles (&e, *res, &holes);
    }
  /* delete holes if any left */
  while ((p = holes) != NULL)
    {
      holes = p->next;
      poly_DelContour (&p);
    }

  if (code)
    {
      poly_Free (res);
      return code;
    }
  assert (!*res || poly_Valid (*res));
  return code;
}				/* poly_Boolean_free */

static void
clear_marks (POLYAREA * p)
{
  POLYAREA *n = p;
  PLINE *c;
  VNODE *v;

  do
    {
      for (c = n->contours; c; c = c->next)
	{
	  v = &c->head;
	  do
	    {
	      v->Flags.mark = 0;
	    }
	  while ((v = v->next) != &c->head);
	}
    }
  while ((n = n->f) != p);
}

/*!
 * \brief Compute the intersection and subtraction (divides "a" into two
 * pieces) and frees the input polys.
 *
 * This assumes that bi is a single simple polygon.
 */
int
poly_AndSubtract_free (POLYAREA * ai, POLYAREA * bi,
		       POLYAREA ** aandb, POLYAREA ** aminusb)
{
  POLYAREA *a = ai, *b = bi;
  PLINE *p, *holes = NULL;
  jmp_buf e;
  int code;

  *aandb = NULL;
  *aminusb = NULL;

  if ((code = setjmp (e)) == 0)
    {

#ifdef DEBUG
      if (!poly_Valid (a))
	return -1;
      if (!poly_Valid (b))
	return -1;
#endif
      M_POLYAREA_intersect (&e, a, b, TRUE);

      M_POLYAREA_label (a, b, FALSE);
      M_POLYAREA_label (b, a, FALSE);

      M_POLYAREA_Collect (&e, a, aandb, &holes, PBO_ISECT, FALSE);
      InsertHoles (&e, *aandb, &holes);
      assert (poly_Valid (*aandb));
      /* delete holes if any left */
      while ((p = holes) != NULL)
	{
	  holes = p->next;
	  poly_DelContour (&p);
	}
      holes = NULL;
      clear_marks (a);
      clear_marks (b);
      M_POLYAREA_Collect (&e, a, aminusb, &holes, PBO_SUB, FALSE);
      InsertHoles (&e, *aminusb, &holes);
      poly_Free (&a);
      poly_Free (&b);
      assert (poly_Valid (*aminusb));
    }
  /* delete holes if any left */
  while ((p = holes) != NULL)
    {
      holes = p->next;
      poly_DelContour (&p);
    }


  if (code)
    {
      poly_Free (aandb);
      poly_Free (aminusb);
      return code;
    }
  assert (!*aandb || poly_Valid (*aandb));
  assert (!*aminusb || poly_Valid (*aminusb));
  return code;
}				/* poly_AndSubtract_free */

static inline int
cntrbox_pointin (PLINE * c, Vector p)
{
  return (p[0] >= c->xmin && p[1] >= c->ymin &&
	  p[0] <= c->xmax && p[1] <= c->ymax);

}

static inline int
node_neighbours (VNODE * a, VNODE * b)
{
  return (a == b) || (a->next == b) || (b->next == a) || (a->next == b->next);
}

VNODE *
poly_CreateNode (Vector v)
{
  VNODE *res;
  Coord *c;

  assert (v);
  res = (VNODE *) calloc (1, sizeof (VNODE));
  if (res == NULL)
    return NULL;
  // bzero (res, sizeof (VNODE) - sizeof(Vector));
  c = res->point;
  *c++ = *v++;
  *c = *v;
  return res;
}

void
poly_IniContour (PLINE * c)
{
  if (c == NULL)
    return;
  /* bzero (c, sizeof(PLINE)); */
  c->head.next = c->head.prev = &c->head;
  c->xmin = c->ymin = COORD_MAX;
  c->xmax = c->ymax = -COORD_MAX - 1;
  c->is_round = FALSE;
  c->cx = 0;
  c->cy = 0;
  c->radius = 0;
}

PLINE *
poly_NewContour (Vector v)
{
  PLINE *res;

  res = (PLINE *) calloc (1, sizeof (PLINE));
  if (res == NULL)
    return NULL;

  poly_IniContour (res);

  if (v != NULL)
    {
      Vcopy (res->head.point, v);
      cntrbox_adjust (res, v);
    }

  return res;
}

void
poly_ClrContour (PLINE * c)
{
  VNODE *cur;

  assert (c != NULL);
  while ((cur = c->head.next) != &c->head)
    {
      poly_ExclVertex (cur);
      free (cur);
    }
  poly_IniContour (c);
}

void
poly_DelContour (PLINE ** c)
{
  VNODE *cur, *prev;

  if (*c == NULL)
    return;
  for (cur = (*c)->head.prev; cur != &(*c)->head; cur = prev)
    {
      prev = cur->prev;
      if (cur->cvc_next != NULL)
	{
	  free (cur->cvc_next);
	  free (cur->cvc_prev);
	}
      free (cur);
    }
  if ((*c)->head.cvc_next != NULL)
    {
      free ((*c)->head.cvc_next);
      free ((*c)->head.cvc_prev);
    }
  /*! \todo FIXME -- strict aliasing violation. */
  if ((*c)->tree)
    {
      rtree_t *r = (*c)->tree;
      r_destroy_tree (&r);
    }
  free (*c), *c = NULL;
}

void
poly_PreContour (PLINE * C, BOOLp optimize)
{
  double area = 0;
  VNODE *p, *c;
  Vector p1, p2;

  assert (C != NULL);

  if (optimize)
    {
      for (c = (p = &C->head)->next; c != &C->head; c = (p = c)->next)
	{
	  /* if the previous node is on the same line with this one, we should remove it */
	  Vsub2 (p1, c->point, p->point);
	  Vsub2 (p2, c->next->point, c->point);
	  /* If the product below is zero then
	   * the points on either side of c 
	   * are on the same line!
	   * So, remove the point c
	   */

	  if (vect_det2 (p1, p2) == 0)
	    {
	      poly_ExclVertex (c);
	      free (c);
	      c = p;
	    }
	}
    }
  C->Count = 0;
  C->xmin = C->xmax = C->head.point[0];
  C->ymin = C->ymax = C->head.point[1];

  p = (c = &C->head)->prev;
  if (c != p)
    {
      do
	{
	  /* calculate area for orientation */
	  area +=
	    (double) (p->point[0] - c->point[0]) * (p->point[1] +
						    c->point[1]);
	  cntrbox_adjust (C, c->point);
	  C->Count++;
	}
      while ((c = (p = c)->next) != &C->head);
    }
  C->area = ABS (area);
  if (C->Count > 2)
    C->Flags.orient = ((area < 0) ? PLF_INV : PLF_DIR);
  C->tree = (rtree_t *)make_edge_tree (C);
}				/* poly_PreContour */

static int
flip_cb (const BoxType * b, void *cl)
{
  struct seg *s = (struct seg *) b;
  s->v = s->v->prev;
  return 1;
}

void
poly_InvContour (PLINE * c)
{
  VNODE *cur, *next;
#ifndef NDEBUG
  int r;
#endif

  assert (c != NULL);
  cur = &c->head;
  do
    {
      next = cur->next;
      cur->next = cur->prev;
      cur->prev = next;
      /* fix the segment tree */
    }
  while ((cur = next) != &c->head);
  c->Flags.orient ^= 1;
  if (c->tree)
    {
#ifndef NDEBUG
      r =
#endif
        r_search (c->tree, NULL, NULL, flip_cb, NULL);
#ifndef NDEBUG
      assert (r == c->Count);
#endif
    }
}

void
poly_ExclVertex (VNODE * node)
{
  assert (node != NULL);
  if (node->cvc_next)
    {
      free (node->cvc_next);
      free (node->cvc_prev);
    }
  node->prev->next = node->next;
  node->next->prev = node->prev;
}

void
poly_InclVertex (VNODE * after, VNODE * node)
{
  double a, b;
  assert (after != NULL);
  assert (node != NULL);

  node->prev = after;
  node->next = after->next;
  after->next = after->next->prev = node;
  /* remove points on same line */
  if (node->prev->prev == node)
    return;			/* we don't have 3 points in the poly yet */
  a = (node->point[1] - node->prev->prev->point[1]);
  a *= (node->prev->point[0] - node->prev->prev->point[0]);
  b = (node->point[0] - node->prev->prev->point[0]);
  b *= (node->prev->point[1] - node->prev->prev->point[1]);
  if (fabs (a - b) < EPSILON)
    {
      VNODE *t = node->prev;
      t->prev->next = node;
      node->prev = t->prev;
      free (t);
    }
}

BOOLp
poly_CopyContour (PLINE ** dst, PLINE * src)
{
  VNODE *cur, *newnode;

  assert (src != NULL);
  *dst = NULL;
  *dst = poly_NewContour (src->head.point);
  if (*dst == NULL)
    return FALSE;

  (*dst)->Count = src->Count;
  (*dst)->Flags.orient = src->Flags.orient;
  (*dst)->xmin = src->xmin, (*dst)->xmax = src->xmax;
  (*dst)->ymin = src->ymin, (*dst)->ymax = src->ymax;
  (*dst)->area = src->area;

  for (cur = src->head.next; cur != &src->head; cur = cur->next)
    {
      if ((newnode = poly_CreateNode (cur->point)) == NULL)
	return FALSE;
      // newnode->Flags = cur->Flags;
      poly_InclVertex ((*dst)->head.prev, newnode);
    }
  (*dst)->tree = (rtree_t *)make_edge_tree (*dst);
  return TRUE;
}

/* polygon routines */

BOOLp
poly_Copy0 (POLYAREA ** dst, const POLYAREA * src)
{
  *dst = NULL;
  if (src != NULL)
    *dst = (POLYAREA *)calloc (1, sizeof (POLYAREA));
  if (*dst == NULL)
    return FALSE;
  (*dst)->contour_tree = r_create_tree (NULL, 0, 0);

  return poly_Copy1 (*dst, src);
}

BOOLp
poly_Copy1 (POLYAREA * dst, const POLYAREA * src)
{
  PLINE *cur, **last = &dst->contours;

  *last = NULL;
  dst->f = dst->b = dst;

  for (cur = src->contours; cur != NULL; cur = cur->next)
    {
      if (!poly_CopyContour (last, cur))
	return FALSE;
      r_insert_entry (dst->contour_tree, (BoxType *) * last, 0);
      last = &(*last)->next;
    }
  return TRUE;
}

void
poly_M_Incl (POLYAREA ** list, POLYAREA * a)
{
  if (*list == NULL)
    a->f = a->b = a, *list = a;
  else
    {
      a->f = *list;
      a->b = (*list)->b;
      (*list)->b = (*list)->b->f = a;
    }
}

BOOLp
poly_M_Copy0 (POLYAREA ** dst, const POLYAREA * srcfst)
{
  const POLYAREA *src = srcfst;
  POLYAREA *di;

  *dst = NULL;
  if (src == NULL)
    return FALSE;
  do
    {
      if ((di = poly_Create ()) == NULL || !poly_Copy1 (di, src))
	return FALSE;
      poly_M_Incl (dst, di);
    }
  while ((src = src->f) != srcfst);
  return TRUE;
}

BOOLp
poly_InclContour (POLYAREA * p, PLINE * c)
{
  PLINE *tmp;

  if ((c == NULL) || (p == NULL))
    return FALSE;
  if (c->Flags.orient == PLF_DIR)
    {
      if (p->contours != NULL)
	return FALSE;
      p->contours = c;
    }
  else
    {
      if (p->contours == NULL)
	return FALSE;
      /* link at front of hole list */
      tmp = p->contours->next;
      p->contours->next = c;
      c->next = tmp;
    }
  r_insert_entry (p->contour_tree, (BoxType *) c, 0);
  return TRUE;
}

typedef struct pip
{
  int f;
  Vector p;
  jmp_buf env;
} pip;


static int
crossing (const BoxType * b, void *cl)
{
  struct seg *s = (struct seg *) b;
  struct pip *p = (struct pip *) cl;

  if (s->v->point[1] <= p->p[1])
    {
      if (s->v->next->point[1] > p->p[1])
	{
	  Vector v1, v2;
	  long long cross;
	  Vsub2 (v1, s->v->next->point, s->v->point);
	  Vsub2 (v2, p->p, s->v->point);
	  cross = (long long) v1[0] * v2[1] - (long long) v2[0] * v1[1];
	  if (cross == 0)
	    {
	      p->f = 1;
	      longjmp (p->env, 1);
	    }
	  if (cross > 0)
	    p->f += 1;
	}
    }
  else
    {
      if (s->v->next->point[1] <= p->p[1])
	{
	  Vector v1, v2;
	  long long cross;
	  Vsub2 (v1, s->v->next->point, s->v->point);
	  Vsub2 (v2, p->p, s->v->point);
	  cross = (long long) v1[0] * v2[1] - (long long) v2[0] * v1[1];
	  if (cross == 0)
	    {
	      p->f = 1;
	      longjmp (p->env, 1);
	    }
	  if (cross < 0)
	    p->f -= 1;
	}
    }
  return 1;
}

int
poly_InsideContour (PLINE * c, Vector p)
{
  struct pip info;
  BoxType ray;

  if (!cntrbox_pointin (c, p))
    return FALSE;
  info.f = 0;
  info.p[0] = ray.X1 = p[0];
  info.p[1] = ray.Y1 = p[1];
  ray.X2 = COORD_MAX;
  ray.Y2 = p[1] + 1;
  if (setjmp (info.env) == 0)
    r_search (c->tree, &ray, NULL, crossing, &info);
  return info.f;
}

BOOLp
poly_CheckInside (POLYAREA * p, Vector v0)
{
  PLINE *cur;

  if ((p == NULL) || (v0 == NULL) || (p->contours == NULL))
    return FALSE;
  cur = p->contours;
  if (poly_InsideContour (cur, v0))
    {
      for (cur = cur->next; cur != NULL; cur = cur->next)
	if (poly_InsideContour (cur, v0))
	  return FALSE;
      return TRUE;
    }
  return FALSE;
}

BOOLp
poly_M_CheckInside (POLYAREA * p, Vector v0)
{
  POLYAREA *cur;

  if ((p == NULL) || (v0 == NULL))
    return FALSE;
  cur = p;
  do
    {
      if (poly_CheckInside (cur, v0))
	return TRUE;
    }
  while ((cur = cur->f) != p);
  return FALSE;
}

static double
dot (Vector A, Vector B)
{
  return (double)A[0] * (double)B[0] + (double)A[1] * (double)B[1];
}

/*!
 * \brief Compute whether point is inside a triangle formed by 3 other
 * points.
 *
 * Algorithm from http://www.blackpawn.com/texts/pointinpoly/default.html.
 */
static int
point_in_triangle (Vector A, Vector B, Vector C, Vector P)
{
  Vector v0, v1, v2;
  double dot00, dot01, dot02, dot11, dot12;
  double invDenom;
  double u, v;

  /* Compute vectors */
  v0[0] = C[0] - A[0];  v0[1] = C[1] - A[1];
  v1[0] = B[0] - A[0];  v1[1] = B[1] - A[1];
  v2[0] = P[0] - A[0];  v2[1] = P[1] - A[1];

  /* Compute dot products */
  dot00 = dot (v0, v0);
  dot01 = dot (v0, v1);
  dot02 = dot (v0, v2);
  dot11 = dot (v1, v1);
  dot12 = dot (v1, v2);

  /* Compute barycentric coordinates */
  invDenom = 1. / (dot00 * dot11 - dot01 * dot01);
  u = (dot11 * dot02 - dot01 * dot12) * invDenom;
  v = (dot00 * dot12 - dot01 * dot02) * invDenom;

  /* Check if point is in triangle */
  return (u > 0.0) && (v > 0.0) && (u + v < 1.0);
}


/*!
 * \brief Returns the dot product of Vector A->B, and a vector
 * orthogonal to Vector C->D.
 *
 * The result is not normalisd, so will be weighted by the magnitude of
 * the C->D vector.
 */
static double
dot_orthogonal_to_direction (Vector A, Vector B, Vector C, Vector D)
{
  Vector l1, l2, l3;
  l1[0] = B[0] - A[0];  l1[1] = B[1] - A[1];
  l2[0] = D[0] - C[0];  l2[1] = D[1] - C[1];

  l3[0] = -l2[1];
  l3[1] = l2[0];

  return dot (l1, l3);
}

/*!
 * \brief Algorithm from http://www.exaflop.org/docs/cgafaq/cga2.html.
 *
 * "Given a simple polygon, find some point inside it.
 *
 * Here is a method based on the proof that there exists an internal
 * diagonal, in [O'Rourke, 13-14].
 *
 * The idea is that the midpoint of a diagonal is interior to the
 * polygon.
 *
 * <ol>
 *   <li>Identify a convex vertex v; let its adjacent vertices be a and b.</li>
 *   <li>For each other vertex q do:</li>
 *   <ol type="a">
 *     <li>If q is inside avb, compute distance to v (orthogonal to ab).</li>
 *     <li>Save point q if distance is a new min.</li>
 *   </ol>
 *   <li>If no point is inside, return midpoint of ab, or centroid of avb.</li>
 *   <li>Else if some point inside, qv is internal: return its midpoint."</li>
 * </ol>
 *
 * [O'Rourke]: Computational Geometry in C (2nd Ed.)
 *
 * Joseph O'Rourke, Cambridge University Press 1998,
 * ISBN 0-521-64010-5 Pbk, ISBN 0-521-64976-5 Hbk.
 */
static void
poly_ComputeInteriorPoint (PLINE *poly, Vector v)
{
  VNODE *pt1, *pt2, *pt3, *q;
  VNODE *min_q = NULL;
  double dist;
  double min_dist = 0.0;
  double dir = (poly->Flags.orient == PLF_DIR) ? 1. : -1;

  /* Find a convex node on the polygon */
  pt1 = &poly->head;
  do
    {
      double dot_product;

      pt2 = pt1->next;
      pt3 = pt2->next;

      dot_product = dot_orthogonal_to_direction (pt1->point, pt2->point,
                                                 pt3->point, pt2->point);

      if (dot_product * dir > 0.)
        break;
    }
  while ((pt1 = pt1->next) != &poly->head);

  /* Loop over remaining vertices */
  q = pt3;
  do
    {
      /* Is current vertex "q" outside pt1 pt2 pt3 triangle? */
      if (!point_in_triangle (pt1->point, pt2->point, pt3->point, q->point))
        continue;

      /* NO: compute distance to pt2 (v) orthogonal to pt1 - pt3) */
      /*     Record minimum */
      dist = dot_orthogonal_to_direction (q->point, pt2->point, pt1->point, pt3->point);
      if (min_q == NULL || dist < min_dist) {
        min_dist = dist;
        min_q = q;
      }
    }
  while ((q = q->next) != pt2);

  /* Were any "q" found inside pt1 pt2 pt3? */
  if (min_q == NULL) {
    /* NOT FOUND: Return midpoint of pt1 pt3 */
    v[0] = (pt1->point[0] + pt3->point[0]) / 2;
    v[1] = (pt1->point[1] + pt3->point[1]) / 2;
  } else {
    /* FOUND: Return mid point of min_q, pt2 */
    v[0] = (pt2->point[0] + min_q->point[0]) / 2;
    v[1] = (pt2->point[1] + min_q->point[1]) / 2;
  }
}


/*!
 * \brief .
 *
 * \note This function assumes the caller _knows_ the contours do not
 * intersect. If the contours intersect, the result is undefined.
 * It will return the correct result if the two contours share
 * common points beteween their contours. (Identical contours
 * are treated as being inside each other).
 */
int
poly_ContourInContour (PLINE * poly, PLINE * inner)
{
  Vector point;
  assert (poly != NULL);
  assert (inner != NULL);
  if (cntrbox_inside (inner, poly))
    {
      /* We need to prove the "inner" contour is not outside
       * "poly" contour. If it is outside, we can return.
       */
      if (!poly_InsideContour (poly, inner->head.point))
        return 0;

      poly_ComputeInteriorPoint (inner, point);
      return poly_InsideContour (poly, point);
    }
  return 0;
}

void
poly_Init (POLYAREA * p)
{
  p->f = p->b = p;
  p->contours = NULL;
  p->contour_tree = r_create_tree (NULL, 0, 0);
}

POLYAREA *
poly_Create (void)
{
  POLYAREA *res;

  if ((res = (POLYAREA *)malloc (sizeof (POLYAREA))) != NULL)
    poly_Init (res);
  return res;
}

void
poly_FreeContours (PLINE ** pline)
{
  PLINE *pl;

  while ((pl = *pline) != NULL)
    {
      *pline = pl->next;
      poly_DelContour (&pl);
    }
}

void
poly_Free (POLYAREA ** p)
{
  POLYAREA *cur;

  if (*p == NULL)
    return;
  for (cur = (*p)->f; cur != *p; cur = (*p)->f)
    {
      poly_FreeContours (&cur->contours);
      r_destroy_tree (&cur->contour_tree);
      cur->f->b = cur->b;
      cur->b->f = cur->f;
      free (cur);
    }
  poly_FreeContours (&cur->contours);
  r_destroy_tree (&cur->contour_tree);
  free (*p), *p = NULL;
}

static BOOLp
inside_sector (VNODE * pn, Vector p2)
{
  Vector cdir, ndir, pdir;
  int p_c, n_c, p_n;

  assert (pn != NULL);
  vect_sub (cdir, p2, pn->point);
  vect_sub (pdir, pn->point, pn->prev->point);
  vect_sub (ndir, pn->next->point, pn->point);

  p_c = vect_det2 (pdir, cdir) >= 0;
  n_c = vect_det2 (ndir, cdir) >= 0;
  p_n = vect_det2 (pdir, ndir) >= 0;

  if ((p_n && p_c && n_c) || ((!p_n) && (p_c || n_c)))
    return TRUE;
  else
    return FALSE;
}				/* inside_sector */

/*!
 * \brief .
 *
 * \return TRUE if bad contour.
 */
BOOLp
poly_ChkContour (PLINE * a)
{
  VNODE *a1, *a2, *hit1, *hit2;
  Vector i1, i2;
  int icnt;

  assert (a != NULL);
  a1 = &a->head;
  do
    {
      a2 = a1;
      do
	{
	  if (!node_neighbours (a1, a2) &&
	      (icnt = vect_inters2 (a1->point, a1->next->point,
				    a2->point, a2->next->point, i1, i2)) > 0)
	    {
	      if (icnt > 1)
		return TRUE;

	      if (vect_dist2 (i1, a1->point) < EPSILON)
		hit1 = a1;
	      else if (vect_dist2 (i1, a1->next->point) < EPSILON)
		hit1 = a1->next;
	      else
		hit1 = NULL;

	      if (vect_dist2 (i1, a2->point) < EPSILON)
		hit2 = a2;
	      else if (vect_dist2 (i1, a2->next->point) < EPSILON)
		hit2 = a2->next;
	      else
		hit2 = NULL;

	      if ((hit1 == NULL) && (hit2 == NULL))
		{
		  /* If the intersection didn't land on an end-point of either
		   * line, we know the lines cross and we return TRUE.
		   */
		  return TRUE;
		}
	      else if (hit1 == NULL)
		{
		/* An end-point of the second line touched somewhere along the
		   length of the first line. Check where the second line leads. */
		  if (inside_sector (hit2, a1->point) !=
		      inside_sector (hit2, a1->next->point))
		    return TRUE;
		}
	      else if (hit2 == NULL)
		{
		/* An end-point of the first line touched somewhere along the
		   length of the second line. Check where the first line leads. */
		  if (inside_sector (hit1, a2->point) !=
		      inside_sector (hit1, a2->next->point))
		    return TRUE;
		}
	      else
		{
		/* Both lines intersect at an end-point. Check where they lead. */
		  if (inside_sector (hit1, hit2->prev->point) !=
		      inside_sector (hit1, hit2->next->point))
		    return TRUE;
		}
	    }
	}
      while ((a2 = a2->next) != &a->head);
    }
  while ((a1 = a1->next) != &a->head);
  return FALSE;
}


BOOLp
poly_Valid (POLYAREA * p)
{
  PLINE *c;

  if ((p == NULL) || (p->contours == NULL))
    return FALSE;

  if (p->contours->Flags.orient == PLF_INV || poly_ChkContour (p->contours))
    {
#ifndef NDEBUG
      VNODE *v, *n;
      DEBUGP ("Invalid Outer PLINE\n");
      if (p->contours->Flags.orient == PLF_INV)
	DEBUGP ("failed orient\n");
      if (poly_ChkContour (p->contours))
	DEBUGP ("failed self-intersection\n");
      v = &p->contours->head;
      do
	{
	  n = v->next;
	  pcb_fprintf (stderr, "Line [%#mS %#mS %#mS %#mS 100 100 \"\"]\n",
		   v->point[0], v->point[1], n->point[0], n->point[1]);
	}
      while ((v = v->next) != &p->contours->head);
#endif
      return FALSE;
    }
  for (c = p->contours->next; c != NULL; c = c->next)
    {
      if (c->Flags.orient == PLF_DIR ||
	  poly_ChkContour (c) || !poly_ContourInContour (p->contours, c))
	{
#ifndef NDEBUG
	  VNODE *v, *n;
	  DEBUGP ("Invalid Inner PLINE orient = %d\n", c->Flags.orient);
	  if (c->Flags.orient == PLF_DIR)
	    DEBUGP ("failed orient\n");
	  if (poly_ChkContour (c))
	    DEBUGP ("failed self-intersection\n");
	  if (!poly_ContourInContour (p->contours, c))
	    DEBUGP ("failed containment\n");
	  v = &c->head;
	  do
	    {
	      n = v->next;
	      pcb_fprintf (stderr, "Line [%#mS %#mS %#mS %#mS 100 100 \"\"]\n",
		       v->point[0], v->point[1], n->point[0], n->point[1]);
	    }
	  while ((v = v->next) != &c->head);
#endif
	  return FALSE;
	}
    }
  return TRUE;
}


Vector vect_zero = { (long) 0, (long) 0 };

/*             L o n g   V e c t o r   S t u f f                     */

void
vect_init (Vector v, double x, double y)
{
  v[0] = (long) x;
  v[1] = (long) y;
}				/* vect_init */

#define Vzero(a)   ((a)[0] == 0. && (a)[1] == 0.)

#define Vsub(a,b,c) {(a)[0]=(b)[0]-(c)[0];(a)[1]=(b)[1]-(c)[1];}

int
vect_equal (Vector v1, Vector v2)
{
  return (v1[0] == v2[0] && v1[1] == v2[1]);
}				/* vect_equal */


void
vect_sub (Vector res, Vector v1, Vector v2)
{
Vsub (res, v1, v2)}		/* vect_sub */

void
vect_min (Vector v1, Vector v2, Vector v3)
{
  v1[0] = (v2[0] < v3[0]) ? v2[0] : v3[0];
  v1[1] = (v2[1] < v3[1]) ? v2[1] : v3[1];
}				/* vect_min */

void
vect_max (Vector v1, Vector v2, Vector v3)
{
  v1[0] = (v2[0] > v3[0]) ? v2[0] : v3[0];
  v1[1] = (v2[1] > v3[1]) ? v2[1] : v3[1];
}				/* vect_max */

double
vect_len2 (Vector v)
{
  return ((double) v[0] * v[0] + (double) v[1] * v[1]);	/* why sqrt? only used for compares */
}

double
vect_dist2 (Vector v1, Vector v2)
{
  double dx = v1[0] - v2[0];
  double dy = v1[1] - v2[1];

  return (dx * dx + dy * dy);	/* why sqrt */
}

/*!
 * \brief Value has sign of angle between vectors.
 */
double
vect_det2 (Vector v1, Vector v2)
{
  return (((double) v1[0] * v2[1]) - ((double) v2[0] * v1[1]));
}

static double
vect_m_dist (Vector v1, Vector v2)
{
  double dx = v1[0] - v2[0];
  double dy = v1[1] - v2[1];
  double dd = (dx * dx + dy * dy);	/* sqrt */

  if (dx > 0)
    return +dd;
  if (dx < 0)
    return -dd;
  if (dy > 0)
    return +dd;
  return -dd;
}				/* vect_m_dist */

/*!
 * \brief vect_inters2.
 *
 * (C) 1993 Klamer Schutte.
 *
 * (C) 1997 Michael Leonov, Alexey Nikitin.
 */
int
vect_inters2 (Vector p1, Vector p2, Vector q1, Vector q2,
	      Vector S1, Vector S2)
{
  double s, t, deel;
  double rpx, rpy, rqx, rqy;

  if (max (p1[0], p2[0]) < min (q1[0], q2[0]) ||
      max (q1[0], q2[0]) < min (p1[0], p2[0]) ||
      max (p1[1], p2[1]) < min (q1[1], q2[1]) ||
      max (q1[1], q2[1]) < min (p1[1], p2[1]))
    return 0;

  rpx = p2[0] - p1[0];
  rpy = p2[1] - p1[1];
  rqx = q2[0] - q1[0];
  rqy = q2[1] - q1[1];

  deel = rpy * rqx - rpx * rqy;	/* -vect_det(rp,rq); */

  /* coordinates are 30-bit integers so deel will be exactly zero
   * if the lines are parallel
   */

  if (deel == 0)		/* parallel */
    {
      double dc1, dc2, d1, d2, h;	/* Check to see whether p1-p2 and q1-q2 are on the same line */
      Vector hp1, hq1, hp2, hq2, q1p1, q1q2;

      Vsub2 (q1p1, q1, p1);
      Vsub2 (q1q2, q1, q2);


      /* If this product is not zero then p1-p2 and q1-q2 are not on same line! */
      if (vect_det2 (q1p1, q1q2) != 0)
	return 0;
      dc1 = 0;			/* m_len(p1 - p1) */

      dc2 = vect_m_dist (p1, p2);
      d1 = vect_m_dist (p1, q1);
      d2 = vect_m_dist (p1, q2);

/* Sorting the independent points from small to large */
      Vcpy2 (hp1, p1);
      Vcpy2 (hp2, p2);
      Vcpy2 (hq1, q1);
      Vcpy2 (hq2, q2);
      if (dc1 > dc2)
	{			/* hv and h are used as help-variable. */
	  Vswp2 (hp1, hp2);
	  h = dc1, dc1 = dc2, dc2 = h;
	}
      if (d1 > d2)
	{
	  Vswp2 (hq1, hq2);
	  h = d1, d1 = d2, d2 = h;
	}

/* Now the line-pieces are compared */

      if (dc1 < d1)
	{
	  if (dc2 < d1)
	    return 0;
	  if (dc2 < d2)
	    {
	      Vcpy2 (S1, hp2);
	      Vcpy2 (S2, hq1);
	    }
	  else
	    {
	      Vcpy2 (S1, hq1);
	      Vcpy2 (S2, hq2);
	    };
	}
      else
	{
	  if (dc1 > d2)
	    return 0;
	  if (dc2 < d2)
	    {
	      Vcpy2 (S1, hp1);
	      Vcpy2 (S2, hp2);
	    }
	  else
	    {
	      Vcpy2 (S1, hp1);
	      Vcpy2 (S2, hq2);
	    };
	}
      return (Vequ2 (S1, S2) ? 1 : 2);
    }
  else
    {				/* not parallel */
      /*
       * We have the lines:
       * l1: p1 + s(p2 - p1)
       * l2: q1 + t(q2 - q1)
       * And we want to know the intersection point.
       * Calculate t:
       * p1 + s(p2-p1) = q1 + t(q2-q1)
       * which is similar to the two equations:
       * p1x + s * rpx = q1x + t * rqx
       * p1y + s * rpy = q1y + t * rqy
       * Multiplying these by rpy resp. rpx gives:
       * rpy * p1x + s * rpx * rpy = rpy * q1x + t * rpy * rqx
       * rpx * p1y + s * rpx * rpy = rpx * q1y + t * rpx * rqy
       * Subtracting these gives:
       * rpy * p1x - rpx * p1y = rpy * q1x - rpx * q1y + t * ( rpy * rqx - rpx * rqy )
       * So t can be isolated:
       * t = (rpy * ( p1x - q1x ) + rpx * ( - p1y + q1y )) / ( rpy * rqx - rpx * rqy )
       * and s can be found similarly
       * s = (rqy * (q1x - p1x) + rqx * (p1y - q1y))/( rqy * rpx - rqx * rpy)
       */

      if (Vequ2 (q1, p1) || Vequ2 (q1, p2))
	{
	  S1[0] = q1[0];
	  S1[1] = q1[1];
	}
      else if (Vequ2 (q2, p1) || Vequ2 (q2, p2))
	{
	  S1[0] = q2[0];
	  S1[1] = q2[1];
	}
      else
	{
	  s = (rqy * (p1[0] - q1[0]) + rqx * (q1[1] - p1[1])) / deel;
	  if (s < 0 || s > 1.)
	    return 0;
	  t = (rpy * (p1[0] - q1[0]) + rpx * (q1[1] - p1[1])) / deel;
	  if (t < 0 || t > 1.)
	    return 0;

	  S1[0] = q1[0] + ROUND (t * rqx);
	  S1[1] = q1[1] + ROUND (t * rqy);
	}
      return 1;
    }
}				/* vect_inters2 */
