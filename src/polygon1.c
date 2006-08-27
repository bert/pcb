#include <stdio.h>
/*
       polygon clipping functions. harry eaton implemented the algorithm
       described in "A Closed Set of Algorithms for Performing Set
       Operations on Polygonal Regions in the Plane" which the original
       code did not do. I also modified it for integer coordinates
       and faster computation. The license for this modified copy was
       switched to the GPL per term (3) of the original LGPL license.
       Copyright (C) 2006 harry eaton
 
   based on:
       poly_Boolean: a polygon clip library
       Copyright (C) 1997  Alexey Nikitin, Michael Leonov
       (also the authors of the paper describing the actual algorithm)
       leonov@propro.iis.nsk.su

   in turn based on:
       nclip: a polygon clip library
       Copyright (C) 1993  Klamer Schutte
 
       This program is free software; you can redistribute it and/or
       modify it under the terms of the GNU General Public
       License as published by the Free Software Foundation; either
       version 2 of the License, or (at your option) any later version.
 
       This program is distributed in the hope that it will be useful,
       but WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
       General Public License for more details.
 
       You should have received a copy of the GNU General Public
       License along with this program; if not, write to the Free
       Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
      polygon1.c
      (C) 1997 Alexey Nikitin, Michael Leonov
      (C) 1993 Klamer Schutte

      all cases where original (Klamer Schutte) code is present
      are marked
*/

#include	<assert.h>
#include	<stdlib.h>
#include	<setjmp.h>
#include	<math.h>
#include	<string.h>

#define ROUND(a) (long)((a) > 0 ? ((a) + 0.5) : ((a) - 0.5)) 

#include "polyarea.h"
#define EPSILON (1E-9)
#define IsZero(a, b) (fabs((a) - (b)) < EPSILON)
#ifndef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif

/*********************************************************************/
/*              L o n g   V e c t o r   S t u f f                    */
/*********************************************************************/

int    vect_equal ( Vector v1, Vector v2 );
void   vect_copy ( Vector des, Vector sou );
void   vect_init ( Vector v, double x, double y, double z);
void   vect_sub( Vector res, Vector v2, Vector v3 );

void   vect_min( Vector res, Vector v2, Vector v3 );
void   vect_max( Vector res, Vector v2, Vector v3 );

double vect_dist2(Vector v1, Vector v2);
double vect_det2(Vector v1, Vector v2);
double vect_len2(Vector v1);

int vect_inters2(Vector A, Vector B, Vector C, Vector D, Vector S1, Vector S2);

/* note that a vertex v's Flags.status represents the edge defined by
 * v to v->next (i.e. the edge is forward of v)
 */
#define ISECTED 0
#define SHARED  ISECTED
#define INSIDE  1
#define OUTSIDE 2
#define UNKNOWN 3
#define SHARED2 4

#define PLF_ESTATE(n)  ((n)->Flags.status)

#define error(code) longjmp(*(e), code)
#define LABEL_NODE(n,l) ((n)->Flags.status = (l))


#define MemGet(ptr, type) \
if (((ptr) = malloc(sizeof(type))) == NULL) \
    error(err_no_memory);


/***************************************************************/
/* routines for processing intersections */

/*
node_add
 (C) 1993 Klamer Schutte
 (C) 1997 Alexey Nikitin, Michael Leonov
*/
static
VNODE *node_add(VNODE * dest, Vector po, int *new_point)
{
    VNODE *p;

    if (vect_equal(po, dest->point))
            return dest;
    if (vect_equal(po, dest->next->point))
    {
        (*new_point) += 4;
        return dest->next;
    }
    p = poly_CreateNode(po);
    if ( p == NULL ) return NULL;
    (*new_point) += 5;
    p->prev = dest;
    p->next = dest->next;
    p->cvc_prev = p->cvc_next = NULL;
    p->Flags.status = UNKNOWN;
    return ( dest->next = dest->next->prev = p );
} /* node_add */

#define ISECT_BAD_PARAM (-1)
#define ISECT_NO_MEMORY (-2)


/*
new_descriptor
  (C) 2006 harry eaton
*/
static
CVCList * new_descriptor (VNODE *a, char poly, char side)
{
   CVCList *l = (CVCList *)malloc(sizeof(CVCList));
   Vector v;
   register double ang;

   if (!l)
     return NULL;
   l->parent = a;
   l->poly = poly;
   if (side == 'P')
       vect_sub(v, a->prev->point, a->point); 
   else
       vect_sub(v, a->next->point, a->point); 
   /* we use sin^2(ang) over the 0->pi/4 range as a proxy for
    * the angle. It still has the same monotonic sort result
    * and is far less expensive to compute.
    */
   assert(!vect_equal(v, vect_zero));
   if (ABS(v[0]) >= ABS(v[1]))
   {
     ang = (double)(v[1]) * v[1];
     ang = ang/(ang + (double)(v[0])*v[0]);
   }
   else
   {
     ang = (double)(v[0]) * v[0];
     ang = ang/(ang + (double)(v[1])*v[1]);
     ang = 1.0 - ang;
   }
   if (v[0] < 0 && v[1] >= 0)
     ang = 2.0 - ang;
   else if (v[0] < 0 && v[1] < 0)
     ang += 2.0;
   else if (v[0] >=0 && v[1] < 0)
     ang = 4.0 - ang;
   l->angle = ang;
   l->side = side;
   // printf("node on %c at (%ld,%ld) assigned angle %g on side %c\n",poly, a->point[0],a->point[1],ang,side);
   l->next = l->prev = l;
   return l;
}

/*
insert_descriptor
  (C) 2006 harry eaton
*/
static
CVCList * insert_descriptor (VNODE *a, char poly, char side, CVCList *start)
{
   CVCList *l, *new, *big, *small;
   
   if (!(new = new_descriptor(a, poly, side)))
     return NULL;
   assert(start);
   big = small = start;
   for (l = start->next ; l != start; l = l->next)
     if (l->angle >= big->angle)
        big = l;
   if (big->angle <= new->angle)
     {
       new->prev = big;
       new->next = big->next;
       big->next = big->next->prev = new;
       return new;
     }
   for (l = start->prev ; l != start; l = l->prev)
     if (l->angle <= small->angle)
        small = l;
   if (small->angle >= new->angle)
     {
       new->next = small;
       new->prev = small->prev;
       small->prev = small->prev->next = new;
       return new;
     }
   for (l = small->next; new->angle > l->angle; l = l->next)
      assert(l != small);
   new->next = l;
   new->prev = l->prev;
   l->prev = l->prev->next = new;
   return new;
}

/*
node_add_point
 (C) 1993 Klamer Schutte
 (C) 1997 Alexey Nikitin, Michael Leonov

 return 1 if new node in b, 2 if new node in a and 3 if new node in both
*/

static
int node_add_point(VNODE * a, VNODE * b, Vector p)
{
    int res = 0;

    VNODE *node_a, *node_b;
    
    node_a = node_add(a, p, &res);
    res += res;
    node_b = node_add(b, p, &res);

    if (node_a == NULL || node_b == NULL )
        return ISECT_NO_MEMORY;
    if (node_a->cvc_prev && !node_b->cvc_prev)
      {
        assert (node_a->cvc_next && !node_b->cvc_next);
        node_b->cvc_prev = insert_descriptor (node_b, 'B', 'P', node_a->cvc_prev);
        node_b->cvc_next = insert_descriptor (node_b, 'B', 'N', node_a->cvc_prev);
        if (!node_b->cvc_next || !node_b->cvc_prev)
          return ISECT_NO_MEMORY;
      }
    else if (node_b->cvc_prev && !node_a->cvc_prev)
      {
        assert (node_b->cvc_next && !node_a->cvc_next);
        node_a->cvc_prev = insert_descriptor (node_a, 'A', 'P', node_b->cvc_prev);
        node_a->cvc_next = insert_descriptor (node_a, 'A', 'N', node_b->cvc_prev);
        if (!node_a->cvc_next || !node_a->cvc_prev)
          return ISECT_NO_MEMORY;
      }
    else if (!node_b->cvc_prev && !node_a->cvc_prev)
      {
        assert (!node_b->cvc_next && !node_a->cvc_next);
        node_a->cvc_next = new_descriptor (node_a, 'A', 'N');
        node_a->cvc_prev = insert_descriptor (node_a, 'A', 'P', node_a->cvc_next);
        node_b->cvc_next = insert_descriptor (node_b, 'B', 'N', node_a->cvc_prev);
        node_b->cvc_prev = insert_descriptor (node_b, 'B', 'P', node_a->cvc_next);
        if (!node_a->cvc_next || !node_a->cvc_prev || !node_b->cvc_next || !node_b->cvc_prev)
          return ISECT_NO_MEMORY;
      }
    return res;
} /* node_add_point */
static
CVCList *clock(CVCList *l)
{
  CVCList *t;
  assert(l);
  for (t = l->prev; t != l && fabs(t->angle - l->angle) < EPSILON; t=t->prev) ;
  return t;
}
static
CVCList *counter(CVCList *l)
{
  CVCList *t;
  assert(l);
  for (t = l->next; t != l && fabs(t->angle - l->angle) < EPSILON; t=t->next) ;
  return t;
}

/*
node_label
 (C) 2006 harry eaton
*/

static
unsigned int node_label(VNODE * pn)
{
    CVCList *l;
    char this_poly;
    int region = UNKNOWN;

    assert(pn);
    assert(pn->cvc_prev);
    this_poly = pn->cvc_prev->poly;
    /* search clockwise in the cross vertex connectivity list
     *
     * check for shared edges, and check if our edges
     * are ever found between the other poly's entry and exit
     */
    //printf("CVCLIST for point (%ld,%ld)\n",pn->point[0],pn->point[1]);
    //printf("  poly %c side %c angle = %g\n",pn->cvc_prev->poly, pn->cvc_prev->side, pn->cvc_prev->angle);
    /* first find whether we're starting inside or outside */
    for (l = pn->cvc_prev->prev; l != pn->cvc_prev; l = l->prev)
      if (l->poly != this_poly)
        {
	  /* these are opposite because they represent the state *before*
	   * we got to this edge
	   */
	  if (l->side == 'P')
	     region = OUTSIDE;
	  else
	     region = INSIDE;
	  break;
	}
    l = pn->cvc_prev;
    do
    {
      //printf("  poly %c side %c angle = %g\n",l->poly, l->side, l->angle);
      if (l->poly != this_poly)
        {
	  if (l->side == 'P')
	     region = INSIDE;
	  else
	     region = OUTSIDE;
      if (fabs(l->angle - pn->cvc_prev->angle) < EPSILON) /* previous on same vector */
      {
        if (l->side == 'P')
	{
	  LABEL_NODE(pn->prev, SHARED); /* incoming is shared */
	  l->parent->prev->shared = pn->prev;
	  pn->prev->shared = l->parent->prev;
	}
	else
	{
	  LABEL_NODE(pn->prev, SHARED2); /* incoming is shared2 */
	  l->parent->shared = pn->prev;
	  pn->prev->shared = l->parent;
	}
      }
      if (fabs(l->angle - pn->cvc_next->angle) < EPSILON) /* next on same vector */
      {
        if (l->side == 'N')
	{
	  LABEL_NODE(pn, SHARED); /* outgoing is shared */
	  l->parent->shared = pn;
	  pn->shared = l->parent;
	}
        else
	{
	  LABEL_NODE(pn, SHARED2); /* outgoing is shared2 */
	  l->parent->prev->shared = pn;
	  pn->shared = l->parent->prev;
	}
      }
     }
     else
     {
       if (l->side == 'P' && l->parent->prev->Flags.status == UNKNOWN)
         LABEL_NODE(l->parent->prev, region);
       else if (l->side == 'N' && l->parent->Flags.status == UNKNOWN)
         LABEL_NODE(l->parent, region);
      }
      } while ((l = l->prev) != pn->cvc_prev);
    //printf("\n");
    assert(pn->Flags.status != UNKNOWN && pn->prev->Flags.status != UNKNOWN);
    if (pn->Flags.status == INSIDE || pn->Flags.status == OUTSIDE)
      return pn->Flags.status;
    return UNKNOWN;
} /* node_label */

/*
intersect
 (C) 1993 Klamer Schutte
 (C) 1997 Alexey Nikitin, Michael Leonov

 calculate all the intersections between a and b. Add the intersection points
 to a and b.
 returns total number of intersections found.*/
static
int intersect(PLINE * a, PLINE * b)
{
    VNODE *a_iter, *b_iter; /* pline node iterators */
    VNODE *a_ph, *b_ph; /* nodes of physical insertion */
    int res = 0, res2 = 0, cnt, i, j, errc;
    Vector intersect1, intersect2;

    assert(a != NULL);
    assert(b != NULL);

    /* if their boxes do not isect return 0 intersections */
    if ( ( a->xmin > b->xmax ) || ( a->xmax < b->xmin ) ||
         ( a->ymin > b->ymax ) || ( a->ymax < b->ymin ) )
         return 0;

    a_iter = &a->head;
    do {
        b_iter = &b->head;
        do {
            cnt = vect_inters2(a_iter->point, a_iter->next->point,
                               b_iter->point, b_iter->next->point,
                               intersect1, intersect2);
            res += cnt;
            if (cnt > 0)
            {
                i = ( errc = node_add_point(a_iter, b_iter, intersect1) );
                if ( errc < 0 )
		   return errc;
		a_ph = a_iter;
		b_ph = b_iter;
		if (i & 2)
		    a_ph = a_ph->next;
		if (i & 1)
		    b_ph = b_ph->next;
		j = (i & 1);
                if (cnt >= 2)
                {
		    errc = 0;
		    if (vect_dist2(a_iter->point, intersect2) < vect_dist2(a_iter->point, intersect1))
		        errc += 1;
		    if (vect_dist2(b_iter->point, intersect2) < vect_dist2(b_iter->point, intersect1))
		      errc += 2;
		    switch(errc)
		    {
		      case 0: /* i1 closer to both a_iter and b_iter */
			 /* it seems we know this is a SHARED edge */
                         i = ( errc = node_add_point(a_ph, b_ph, intersect2) );
		         j += (i & 1);
			 break;
		      case 1: /* i1 closer to b_iter, i2 closer to a_iter */
			 /* must be a SHARED2 edge */
                         i = ( errc = node_add_point(a_iter, b_ph, intersect2) );
		         j += (i & 1);
			 break;
		      case 2: /* i1 closer to a_iter, i2 closer to b_iter */
			 /* another SHARED 2 condition */
                         i = ( errc = node_add_point(a_ph, b_iter, intersect2) );
		         j += (i & 1);
			 break;
		      case 3: /* i2 closer to both a_iter and b_iter */
                         i = ( errc = node_add_point(a_iter, b_iter, intersect2) );
		         j += (i & 1);
			 break;
		    }
                    if ( errc < 0 ) return errc;
                }
		for (res2 += j; j>0; j--)
		  b_iter = b_iter->next;
            }

        } while ((b_iter = b_iter->next) != &b->head);
    } while ((a_iter = a_iter->next) != &a->head);

    return res;
} /* intersect */

static
void M_POLYAREA_intersect(jmp_buf *e, POLYAREA * afst, POLYAREA * bfst)
{
    POLYAREA *a = afst, *b = bfst;
    PLINE *curcA, *curcB;
    int locisect;

    if ( a == NULL || b == NULL )
        error(err_bad_parm);
    do {
        do {
            for (curcA = a->contours; curcA != NULL; curcA = curcA->next)
                for (curcB = b->contours; curcB != NULL; curcB = curcB->next)
                    if ((locisect = intersect(curcA, curcB)) > 0)
                    {
                        curcA->Flags.status = ISECTED;
                        curcB->Flags.status = ISECTED;
                    }
                    else if ( locisect == ISECT_BAD_PARAM )
                        error(err_bad_parm);
                    else if ( locisect == ISECT_NO_MEMORY )
                        error(err_no_memory);
        } while ((a = a->f) != afst);
    } while ((b = b->f) != bfst);
} /* M_POLYAREA_intersect */

/*****************************************************************/
/* Routines for making labels */

/* cntr_in_M_POLYAREA
returns poly is inside outfst ? TRUE : FALSE */
static
int cntr_in_M_POLYAREA(PLINE * poly, POLYAREA * outfst)
{
    PLINE  *curc;
    POLYAREA *outer = outfst;

    assert(poly != NULL);
    assert(outer != NULL);

    do {
        if (poly_ContourInContour(outer->contours, poly))
        {
            for (curc = outer->contours->next; curc != NULL; curc = curc->next)
                if (poly_ContourInContour(curc, poly))
                    goto proceed;
            return TRUE;
        }
proceed:;
    } while ((outer = outer->f) != outfst);
    return FALSE;
} /* cntr_in_M_POLYAREA */


char *
theState(VNODE *v)
{
static char u[] = "UNKNOWN";
static char i[] = "INSIDE";
static char o[] = "OUTSIDE";
static char s[] = "SHARED";
static char s2[] = "SHARED2";

  switch (PLF_ESTATE(v))
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

void
print_labels(PLINE *a)
{
  VNODE *c = &a->head;

  do
    {
      printf("(%ld,%ld)->(%ld,%ld) marked %s\n", c->point[0],c->point[1],
           c->next->point[0], c->next->point[1], theState(c));
    } while ((c=c->next) != &a->head);
}

/*
label_contour
 (C) 2006 harry eaton
 (C) 1993 Klamer Schutte
 (C) 1997 Alexey Nikitin, Michael Leonov
*/

static
void label_contour(PLINE * a)
{
    VNODE *cur = &a->head;
    int did_label, label = UNKNOWN;

    do {
        if (cur == &a->head)
	   did_label = FALSE;
        if ( PLF_ESTATE(cur) != UNKNOWN )
	{
	    label = PLF_ESTATE(cur);
            continue;
	}
	if (cur->cvc_next) /* examine cross vertex */
	{
	  label = node_label(cur);
	  did_label = TRUE;
	}
	else if (label == INSIDE || label == OUTSIDE)
	{
	    cur->Flags.status = label;
	    did_label = TRUE;
	}
    } while ((cur = cur->next) != &a->head || did_label);
  /*print_labels(a);
  printf("\n\n");
  */
} /* label_contour */

static
void cntr_label_POLYAREA(PLINE * poly, POLYAREA * ppl)
{
    assert(ppl != NULL && ppl->contours != NULL);
    if (poly->Flags.status == ISECTED)
        label_contour(poly);
    else if ( cntr_in_M_POLYAREA( poly, ppl ) )
        poly->Flags.status = INSIDE;
    else
        poly->Flags.status = OUTSIDE;
} /* cntr_label_POLYAREA */

static
void M_POLYAREA_label(POLYAREA * afst, POLYAREA * b)
{
    POLYAREA *a = afst;
    PLINE *curc;

    assert(a != NULL);
    do {
    for (curc = a->contours; curc != NULL; curc = curc->next)
        cntr_label_POLYAREA(curc, b);
    } while ((a = a->f) != afst);
}

/****************************************************************/

/* routines for temporary storing resulting contours */
static
void InsCntr(jmp_buf *e, PLINE * c, POLYAREA ** dst)
{
    POLYAREA *newp;

    if (*dst == NULL)
    {
        MemGet(*dst, POLYAREA);
        (*dst)->f = (*dst)->b = *dst;
        newp = *dst;
    }
    else
    {
        MemGet(newp, POLYAREA);
        newp->f = *dst;
        newp->b = (*dst)->b;
        newp->f->b = newp->b->f = newp;
    }
    newp->contours = c;
    c->next = NULL;
} /* InsCntr */

static
void PutContour(jmp_buf *e, PLINE * cntr, POLYAREA ** contours, PLINE ** holes)
{
    PLINE *cur;

    assert(cntr != NULL);
    cntr->next = NULL;
    if (cntr->Flags.orient == PLF_DIR)
        InsCntr(e, cntr, contours);

    /* put hole into temporary list */
    else if (*holes == NULL)
        *holes = cntr; /* let cntr be 1st hole in list */
    else
    {
        for (cur = *holes; cur->next != NULL; cur = cur->next);
        cur->next = cntr;
    }
} /* PutContour */

static
void InsertHoles(jmp_buf *e, POLYAREA * dest, PLINE **src)
{
    POLYAREA *curc, *container;
    PLINE *curh;

    if (*src == NULL) return; /* empty hole list */
    if (dest == NULL) error(err_bad_parm); /* empty contour list */
    
    while ((curh = *src) != NULL)
    {
        *src = curh->next;

        container = NULL;
        curc = dest;
        do
        {
            /* find the smallest container for the hole */
            if (poly_ContourInContour(curc->contours, curh) &&
               (container == NULL ||
                poly_ContourInContour(container->contours, curc->contours)))
                container = curc;
        } while ((curc = curc->f) != dest);
        curh->next = NULL;
        if (container == NULL)
        {
            /* bad input polygons were given */
            poly_DelContour(&curh);
            error(err_bad_parm);
        }
        else
            poly_InclContour(container, curh);
    }
} /* InsertHoles */


/****************************************************************/
/* routines for collecting result */

typedef enum
{
    FORW, BACKW
} DIRECTION;

/* Start Rule */
typedef int (*S_Rule) (VNODE *, DIRECTION *);

/* Jump Rule  */
typedef int (*J_Rule) (CVCList *, VNODE *, DIRECTION *);

static
int UniteS_Rule(VNODE * cur, DIRECTION * initdir)
{
    *initdir = FORW;
    return (PLF_ESTATE(cur) == OUTSIDE) ||
           (PLF_ESTATE(cur) == SHARED);
}

static
int IsectS_Rule(VNODE * cur, DIRECTION * initdir)
{
    *initdir = FORW;
    return (PLF_ESTATE(cur) == INSIDE) ||
           (PLF_ESTATE(cur) == SHARED);
}


static
int SubS_Rule(VNODE * cur, DIRECTION * initdir)
{
    *initdir = FORW;
    return (PLF_ESTATE(cur) == OUTSIDE) ||
           (PLF_ESTATE(cur) == SHARED2);
}

static
int XorS_Rule(VNODE * cur, DIRECTION * initdir)
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

static
int IsectJ_Rule(CVCList * l, VNODE * v , DIRECTION * cdir)
{
    assert(*cdir == FORW);
    return (v->Flags.status == INSIDE || v->Flags.status == SHARED);
}

static
int UniteJ_Rule(CVCList * l, VNODE * v, DIRECTION * cdir)
{
    assert(*cdir == FORW);
    return (v->Flags.status == OUTSIDE || v->Flags.status == SHARED);
}

static
int XorJ_Rule(CVCList * l, VNODE * v, DIRECTION * cdir)
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

static
int SubJ_Rule(CVCList * l, VNODE *v, DIRECTION * cdir)
{
   if (l->poly == 'B' && v->Flags.status == INSIDE)
     {
         *cdir = BACKW;
          return TRUE;
     }
   if (l->poly == 'A' && v->Flags.status == OUTSIDE)
    {
          *cdir = FORW;
           return TRUE;
    }
   if (v->Flags.status == SHARED2)
    {
         *cdir = FORW;
         return TRUE;
    }
    return FALSE;
}

static
int jump (VNODE **cur, DIRECTION *cdir, J_Rule rule)
{
  CVCList *d, *start;
  VNODE *e;
  DIRECTION new;

  //printf("jump entering node at (%ld, %ld)\n",(*cur)->point[0], (*cur)->point[1]);
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
    new = *cdir;
    if (!e->Flags.mark && rule (d, e, &new))
    {
       if ((d->side == 'N' && new == FORW) ||
          (d->side == 'P' && new == BACKW))
	 {
	 /*
  if (new == FORW)
    printf("jump leaving node at (%ld, %ld)\n",e->next->point[0], e->next->point[1]);
  else
    printf("jump leaving node at (%ld, %ld)\n",e->prev->point[0], e->prev->point[1]);
    */
           *cur = d->parent;
	   *cdir = new;
           return TRUE;
	 }
    }
  } while ((d = d->prev) != start);
  return FALSE;
}

static
int Gather(VNODE * start, PLINE ** result, J_Rule v_rule, DIRECTION initdir)
{
    VNODE *cur = start, *newn, *cn;
    DIRECTION dir = initdir;
    //printf("gather direction = %d\n", dir);
    do
    {
        if (*result == NULL)
	{
	   *result = poly_NewContour(start->point);
           if ( *result == NULL ) return err_no_memory;
	}
	else
	{
           if ( (newn = poly_CreateNode(cur->point)) == NULL)
               return err_no_memory;
           poly_InclVertex((*result)->head.prev, newn);
	}
        //printf("gather node at (%ld, %ld)\n", cur->point[0], cur->point[1]);

        cn = (dir == FORW ? cur : cur->prev);
        cn->Flags.mark = 1;

        /* for SHARED edge mark both */
	if (cn->Flags.status == SHARED || cn->Flags.status == SHARED2)
	  cn->shared->Flags.mark = 1;

        cur = (dir == FORW ? cur->next : cur->prev);

        if (cur->cvc_prev != NULL)
	  if (!jump(&cur, &dir, v_rule ))
	     break;
    } while (! (dir == FORW ? cur->Flags.mark : cur->prev->Flags.mark));
    return err_ok;
} /* Gather */

static
void Collect(jmp_buf *e, PLINE * a, POLYAREA ** contours, PLINE ** holes,
              S_Rule s_rule, J_Rule j_rule)
{
    VNODE *cur;
    PLINE *p = NULL;
    int errc = err_ok;
    DIRECTION dir;

    cur = &a->head;
    do
        if ((cur->Flags.mark == 0) && s_rule(cur, &dir))
        {
            p = NULL; /* start making contour */
            if ( (errc = Gather(dir == FORW ? cur : cur->next, &p, j_rule, dir)) != err_ok)
            {
                if (p != NULL) poly_DelContour(&p);
                error(errc);
            }
            poly_PreContour(p, TRUE);
            // printf("adding contour with %d verticies and direction %c\n", p->Count, p->Flags.orient ? 'F' : 'B');
            PutContour(e, p, contours, holes);
        }
    while ((cur = cur->next) != &a->head);
} /* Collect */

static
void cntr_Collect(jmp_buf *e, PLINE * A, POLYAREA ** contours, PLINE ** holes,
                   int action)
{
    PLINE *tmprev;

    if (A->Flags.status == ISECTED)
    {
        switch (action)
        {
        case PBO_UNITE:
            Collect(e, A, contours, holes, UniteS_Rule, UniteJ_Rule);
            break;
        case PBO_ISECT:
            Collect(e, A, contours, holes, IsectS_Rule, IsectJ_Rule);
            break;
        case PBO_XOR:
            Collect(e, A, contours, holes, XorS_Rule, XorJ_Rule);
	    break;
        case PBO_SUB:
            Collect(e, A, contours, holes, SubS_Rule, SubJ_Rule);
            break;
        };
    }
    else
    {
        switch (action)
        {
        case PBO_ISECT:
            if (A->Flags.status == INSIDE)
            {
                if (!poly_CopyContour(&tmprev, A) )
                    error(err_no_memory);
                poly_PreContour(tmprev, TRUE);
                PutContour(e, tmprev, contours, holes);
            }
            break;
        case PBO_XOR:
            if (A->Flags.status == INSIDE)
            {
                if ( !poly_CopyContour(&tmprev, A) )
                    error(err_no_memory);
                poly_PreContour(tmprev, TRUE);
                poly_InvContour(tmprev);
                PutContour(e, tmprev, contours, holes);
            }
        case PBO_UNITE:
        case PBO_SUB:
            if (A->Flags.status  == OUTSIDE)
            {
                if ( !poly_CopyContour(&tmprev, A) )
                    error(err_no_memory);
                poly_PreContour(tmprev, TRUE);
                PutContour(e, tmprev, contours, holes);
            }
            break;
        }
    }
} /* cntr_Collect */

static
void M_B_AREA_Collect(jmp_buf *e, POLYAREA *bfst, POLYAREA ** contours, PLINE **holes, int action)
{
    POLYAREA *b = bfst;
    PLINE *cur, *tmprev;

    assert(b != NULL);
    do {
        for (cur = b->contours; cur != NULL; cur = cur->next)
	{
    	  if (cur->Flags.status == ISECTED) continue;
	  if (cur->Flags.status == INSIDE)
	      switch(action)
	      {
	        case PBO_XOR:
		case PBO_SUB:
		 /* invert and include */
                if ( !poly_CopyContour(&tmprev, cur) )
                    error(err_no_memory);
                poly_PreContour(tmprev, TRUE);
                poly_InvContour(tmprev);
                PutContour(e, tmprev, contours, holes);
		 break;
		case PBO_ISECT:
                if ( !poly_CopyContour(&tmprev, cur) )
                    error(err_no_memory);
                poly_PreContour(tmprev, TRUE);
                PutContour(e, tmprev, contours, holes);
		  /* include */
		  break;
		case PBO_UNITE:
		  break; /* nothing to do - already included */
	      }
	   else if (cur->Flags.status == OUTSIDE)
	     switch(action)
	     {
	        case PBO_XOR:
		case PBO_UNITE:
		  /* include */
                if ( !poly_CopyContour(&tmprev, cur) )
                    error(err_no_memory);
                poly_PreContour(tmprev, TRUE);
                PutContour(e, tmprev, contours, holes);
		  break;
		case PBO_ISECT:
		case PBO_SUB:
		  break;  /* do nothing, not included */
            }
	  }

    } while ((b = b->f) != bfst);
}


static
void M_POLYAREA_Collect(jmp_buf *e, POLYAREA * afst, POLYAREA ** contours, PLINE ** holes,
                      int action)
{
    POLYAREA *a = afst;
    PLINE *cur;

    assert(a != NULL);
    do {
        for (cur = a->contours; cur != NULL; cur = cur->next)
            cntr_Collect(e, cur, contours, holes, action);
    } while ((a = a->f) != afst);
}

/************************************************************************/
/* prepares polygon for algorithm, sets UNKNOWN labels and clears MARK bits */
static
void M_InitPolygon(POLYAREA * afst)
{
    PLINE *curc;
    VNODE *curn;
    POLYAREA *a = afst;

    assert(a != NULL);
    do {
        for (curc = a->contours; curc != NULL; curc = curc->next)
        {
            poly_PreContour(curc, TRUE);
            curc->Flags.status = UNKNOWN;
            curn = &curc->head;
            do {
                curn->Flags.mark = 0;
		curn->Flags.status = UNKNOWN;
                curn->cvc_prev = curn->cvc_next = NULL;
            } while ((curn = curn->next) != &curc->head);
        }
    } while ((a = a->f) != afst);
} /* M_InitPolygon */

/* the main clipping routine */
int poly_Boolean(const POLYAREA * a_org, const POLYAREA * b_org, POLYAREA ** res, int action)
{
    POLYAREA    *a = NULL, *b = NULL;
    PLINE       *p, *holes = NULL;
    jmp_buf     e;
    int         code;

    *res = NULL;

    if ((code = setjmp(e)) == 0)
    {
        if (!poly_M_Copy0(&a, a_org) || !poly_M_Copy0(&b, b_org))
            longjmp(e, err_no_memory);

        /* prepare polygons */
        M_InitPolygon(a);
        M_InitPolygon(b);

        M_POLYAREA_intersect(&e, a, b);

        M_POLYAREA_label(a, b);
        M_POLYAREA_label(b, a);

        M_POLYAREA_Collect(&e, a, res, &holes, action);
	M_B_AREA_Collect(&e, b, res, &holes, action);

        InsertHoles(&e, *res, &holes);
    }
    /* delete holes */
    while ((p = holes) != NULL)
    {
        holes = p->next;
        poly_DelContour(&p);
    }
    poly_Free(&a);
    poly_Free(&b);

    if (code)
        poly_Free(res);
    return code;
} /* poly_Boolean */


static
void cntrbox_adjust(PLINE * c, Vector p)
{
    c->xmin = min(c->xmin, p[0]);
    c->xmax = max(c->xmax, p[0]);
    c->ymin = min(c->ymin, p[1]);
    c->ymax = max(c->ymax, p[1]);
}

static
int cntrbox_pointin(PLINE * c, Vector p)
{
    return (p[0] >= c->xmin && p[1] >= c->ymin &&
            p[0] <= c->xmax && p[1] <= c->ymax);

}

static
int cntrbox_inside(PLINE * c1, PLINE * c2)
{
    assert(c1 != NULL && c2 != NULL);
    return ((c1->xmin >= c2->xmin) &&
            (c1->ymin >= c2->ymin) &&
            (c1->xmax <= c2->xmax) &&
            (c1->ymax <= c2->ymax));
}

static
int node_neighbours(VNODE * a, VNODE * b)
{
    return (a == b) || (a->next == b) || (b->next == a) ||
    (a->next == b->next);
}

VNODE *poly_CreateNode(Vector v)
{
    VNODE *res;

    if (v == NULL)
        return NULL;
    res = (VNODE *) malloc(sizeof(VNODE));
    if (res == NULL)
        return NULL;
    res->Flags.status = UNKNOWN;
    res->Flags.mark = 0;
    res->cvc_prev = res->cvc_next = NULL;
    vect_copy(res->point, v);
    res->prev = res->next = res;
    return res;
}

void poly_IniContour(PLINE * c)
{
    if (c == NULL)
        return;
    c->next = NULL;
    c->Flags.status = UNKNOWN;
    c->Flags.orient = 0;
    c->Flags.cyclink = 0;
    c->Count = 0;
    vect_copy(c->head.point, vect_zero);
    c->head.next = c->head.prev = &c->head;
    c->xmin = c->ymin = 0x7fffffff;
    c->xmax = c->ymax = 0x80000000;
}

PLINE *poly_NewContour(Vector v)
{
    PLINE *res;

    res = (PLINE *)calloc(1, sizeof(PLINE));
    if (res == NULL)
        return NULL;

    poly_IniContour(res);

    if (v != NULL)
    {
        vect_copy(res->head.point, v);
        cntrbox_adjust(res, v);
    }
    
    return res;
}

void poly_ClrContour(PLINE * c)
{
    VNODE *cur;

    assert(c != NULL);
    while ( ( cur = c->head.next ) != &c->head )
    {
        poly_ExclVertex(cur);
        free(cur);
    }
    poly_IniContour(c);
}

void poly_DelContour(PLINE ** c)
{
    if (*c == NULL)
        return;
    poly_ClrContour(*c);
    free(*c), *c = NULL;
}

void poly_PreContour(PLINE *C, BOOLp optimize)
{
    double  area = 0;
    VNODE	*p, *c;

    assert(C != NULL);

	if (optimize)
	{
		for (c = (p = &C->head)->next; c != &C->head; c = (p = c)->next)
		{
			if (IsZero(p->point[0], c->point[0]) &&
				IsZero(p->point[1], c->point[1]))
			{
				// printf("removing redundant point at (%ld, %ld)\n", c->point[0], c->point[1]);
				poly_ExclVertex(c);
				free(c);
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
		do {
			/* calculate area for orientation */
			area += (double)(p->point[0] - c->point[0]) * (p->point[1] + c->point[1]);
			cntrbox_adjust(C, c->point);
			C->Count++;
		} while ((c = (p = c)->next) != &C->head);
	}
    C->area = ABS(area);
    if (C->Count > 2)
        C->Flags.orient = ((area < 0) ? PLF_INV : PLF_DIR);
} /* poly_PreContour */

void poly_InvContour(PLINE * c)
{
    VNODE *cur, *next;

    assert(c != NULL);
    cur = &c->head;
    do
    {
        next = cur->next;
        cur->next = cur->prev;
        cur->prev = next;
    } while ((cur = next) != &c->head);
    c->Flags.orient ^= 1;
}

void poly_InclVertex(VNODE * after, VNODE * node)
{
    assert(after != NULL);
    assert(node != NULL);

    node->prev = after;
    node->next = after->next;
    after->next = after->next->prev = node;
}

void poly_ExclVertex(VNODE * node)
{
    assert (node != NULL);
    if (node->cvc_next)
      {
        free(node->cvc_next);
        free(node->cvc_prev);
      }
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

BOOLp poly_CopyContour(PLINE ** dst, PLINE * src)
{
    VNODE *cur, *newnode;

    assert(src != NULL);
    *dst = NULL;
    *dst = poly_NewContour(src->head.point);
    if (*dst == NULL)
        return FALSE;

    (*dst)->Count = src->Count;
    (*dst)->Flags = src->Flags;
    (*dst)->xmin = src->xmin, (*dst)->xmax = src->xmax;
    (*dst)->ymin = src->ymin, (*dst)->ymax = src->ymax;

    for (cur = src->head.next; cur != &src->head; cur = cur->next)
    {
        if ( (newnode = poly_CreateNode(cur->point) ) == NULL)
            return FALSE;
        newnode->Flags = cur->Flags;
        poly_InclVertex((*dst)->head.prev, newnode);
    }
    return TRUE;
}

/**********************************************************************/
/* polygon routines */

BOOLp poly_Copy0(POLYAREA ** dst, const POLYAREA * src)
{
    *dst = NULL;
    if (src != NULL)
        *dst = calloc(1, sizeof(POLYAREA));
    if (*dst == NULL)
        return FALSE;

    return poly_Copy1(*dst, src);
}

BOOLp poly_Copy1(POLYAREA * dst, const POLYAREA * src)
{
    PLINE *cur, **last = &dst->contours;

    *last = NULL;
    dst->f = dst->b = dst;

    for (cur = src->contours; cur != NULL; cur = cur->next)
    {
        if (!poly_CopyContour(last, cur)) 
            return FALSE;
        last = &(*last)->next;
    }
    return TRUE;
}

void poly_M_Incl(POLYAREA **list, POLYAREA *a)
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

BOOLp poly_M_Copy0(POLYAREA ** dst, const POLYAREA * srcfst)
{
    const POLYAREA *src = srcfst;
    POLYAREA *di;

    *dst = NULL;
    if (src == NULL)
        return FALSE;
    do {
        if ((di = poly_Create()) == NULL ||
            !poly_Copy1(di, src))
            return FALSE;
        poly_M_Incl(dst, di);
    } while ((src = src->f) != srcfst);
    return TRUE;
}

BOOLp poly_InclContour(POLYAREA * p, PLINE * c)
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
        for (tmp = p->contours; tmp->next != NULL; tmp = tmp->next);
        tmp->next = c;
        c->next = NULL;
    }
    return TRUE;
}

BOOLp poly_M_InclContour(POLYAREA ** p, PLINE * c)
{
    POLYAREA *a, *t;

    if (c == NULL)
        return FALSE;
    if (c->Flags.orient == PLF_DIR)
    {
        t = poly_Create();
        if (*p != NULL)
        {
            a = (*p)->b;
            (((t->f = a->f)->b = t)->b = a)->f = t;
        }
        else
            *p = t;
        return poly_InclContour(t, c);
    }
    else
    {
        if (*p == NULL)
            return FALSE;
        a = *p;
        t = NULL;
        do {
            /* find the smallest container for the hole */
            if (poly_ContourInContour(a->contours, c) &&
               (t == NULL ||
                poly_ContourInContour(t->contours, a->contours)))
                t = a;
        } while ((a = a->f) != *p);
        if (t == NULL)
            return FALSE;
        else
            return poly_InclContour(t, c);
    }
}

BOOLp poly_ExclContour(POLYAREA * p, PLINE * c)
{
    PLINE *tmp;

    if ((c == NULL) || (p == NULL) || (p->contours == NULL))
        return FALSE;
    if (c->Flags.orient == PLF_DIR)
    {
        if (c != p->contours)
            return FALSE;
        assert(c->next == NULL);
        p->contours = NULL;
    }
    else
    {
        for (tmp = p->contours; (tmp->next != c) && (tmp->next != NULL);
             tmp = tmp->next) {}
        if (tmp->next == NULL)
            return FALSE;
        tmp->next = c->next;
    }
    return TRUE;
}


BOOLp poly_M_ExclContour(POLYAREA * p, PLINE * c)
{
    POLYAREA *a = p;

    if (p == NULL || c == NULL)
        return FALSE;
    do {
        if (poly_ExclContour(a, c))
            return TRUE;
    } while ((a = a->f) != p);
    return FALSE;
}

int poly_InsideContour(PLINE * c, Vector p)
{
    int f = 0;
    VNODE *cur;

    if (!cntrbox_pointin(c, p))
        return FALSE;

    cur = &c->head;
    do
    {
        if ((
             ((cur->point[1] <= p[1]) && (p[1] < cur->prev->point[1])) ||
             ((cur->prev->point[1] <= p[1]) && (p[1] < cur->point[1]))
             ) && (p[0] <
           (cur->prev->point[0] - cur->point[0]) * (p[1] - cur->point[1])
                 / (cur->prev->point[1] - cur->point[1]) + cur->point[0])
            )
            f = !f;
    } while ((cur = cur->next) != &c->head);
    return f;
}


BOOLp poly_CheckInside(POLYAREA * p, Vector v0)
{
    PLINE *cur;

    if ((p == NULL) || (v0 == NULL) || (p->contours == NULL))
        return FALSE;
    cur = p->contours;
    if (poly_InsideContour(cur, v0))
    {
        for (cur = cur->next; cur != NULL; cur = cur->next)
            if (poly_InsideContour(cur, v0))
                return FALSE;
        return TRUE;
    }
    return FALSE;
}

BOOLp poly_M_CheckInside(POLYAREA * p, Vector v0)
{
    POLYAREA *cur;

    if ((p == NULL) || (v0 == NULL))
        return FALSE;
    cur = p;
    do {
        if (poly_CheckInside(cur, v0))
            return TRUE;
    } while ((cur = cur->f) != p);
    return FALSE;
}

int poly_ContourInContour(PLINE * poly, PLINE * inner)
{
    assert(poly != NULL);
    assert(inner != NULL);
    if (!cntrbox_inside(inner, poly))
        return FALSE;
    return poly_InsideContour(poly, inner->head.point);
}

void poly_Init(POLYAREA  *p)
{
    p->f = p->b = p;
    p->contours = NULL;
}

POLYAREA *poly_Create(void)
{
    POLYAREA *res;

    if ((res = malloc(sizeof(POLYAREA))) != NULL)
		poly_Init(res);
    return res;
}

void poly_Clear(POLYAREA *P)
{
    PLINE *p;

    assert(P != NULL);
    while ((p = P->contours) != NULL)
    {
        P->contours = p->next;
        poly_DelContour(&p);
    }
}

void poly_Free(POLYAREA ** p)
{
    POLYAREA *cur;

    if (*p == NULL)
        return;
    for (cur = (*p)->f; cur != *p; cur = (*p)->f)
    {
        poly_Clear(cur);
        cur->f->b = cur->b;
        cur->b->f = cur->f;
        free(cur);
    }
    poly_Clear(cur);
    free(*p), *p = NULL;
}

static
BOOLp inside_sector(VNODE * pn, Vector p2)
{
    Vector cdir, ndir, pdir;
    int p_c, n_c, p_n;

    assert(pn != NULL);
    vect_sub(cdir, p2, pn->point);
    vect_sub(pdir, pn->point, pn->prev->point);
    vect_sub(ndir, pn->next->point, pn->point);

    p_c = vect_det2(pdir, cdir) >= 0;
    n_c = vect_det2(ndir, cdir) >= 0;
    p_n = vect_det2(pdir, ndir) >= 0;

    if ((p_n && p_c && n_c) || ((!p_n) && (p_c || n_c)))
        return TRUE;
    else
        return FALSE;
} /* inside_sector */

/* returns TRUE if bad contour */
BOOLp poly_ChkContour(PLINE * a)
{
    VNODE *a1, *a2, *hit1, *hit2;
    Vector i1, i2;
    int    icnt;

    assert(a != NULL);
    a1 = &a->head;
    do
    {
        a2 = a1;
        do
        {
            if (!node_neighbours(a1, a2) &&
                (icnt = vect_inters2(a1->point, a1->next->point,
                              a2->point, a2->next->point, i1, i2)) > 0)
            {
                if (icnt > 1)
                    return TRUE;

                if (vect_dist2(i1, a1->point) < EPSILON)
                    hit1 = a1;
                else if (vect_dist2(i1, a1->next->point) < EPSILON)
                    hit1 = a1->next;
                else
                    return TRUE;

                if (vect_dist2(i1, a2->point) < EPSILON)
                    hit2 = a2;
                else if (vect_dist2(i1, a2->next->point) < EPSILON)
                    hit2 = a2->next;
                else
                    return TRUE;

#if 0
                /* now check if they are inside each other */
                if (inside_sector(hit1, hit2->prev->point) ||
                    inside_sector(hit1, hit2->next->point) ||
                    inside_sector(hit2, hit1->prev->point) ||
                    inside_sector(hit2, hit1->next->point))
                    return TRUE;
#endif
            }
        }
        while ((a2 = a2->next) != &a->head);
    }
    while ((a1 = a1->next) != &a->head);
    return FALSE;
}


BOOLp poly_Valid(POLYAREA * p)
{
    PLINE *c;

    if ((p == NULL) || (p->contours == NULL))
        return FALSE;

    if (p->contours->Flags.orient == PLF_INV ||
        poly_ChkContour(p->contours))
        return FALSE;
    for (c = p->contours->next; c != NULL; c = c->next)
    {
        if (c->Flags.orient == PLF_DIR ||
            poly_ChkContour(c) ||
            !poly_ContourInContour(p->contours, c) )
            return FALSE;
    }
    return TRUE;
}


Vector vect_zero = { (long)0, (long)0, (long)0 };

/*********************************************************************/
/*             L o n g   V e c t o r   S t u f f                     */
/*********************************************************************/

void vect_init ( Vector v, double x, double y, double z )
{
    v[0] = (long)x;
    v[1] = (long)y;
    v[2] = (long)z;
} /* vect_init */

#define Vcopy(a,b) {(a)[0]=(b)[0];(a)[1]=(b)[1];(a)[2]=(b)[2];}
#define Vzero(a)   ((a)[0] == 0. && (a)[1] == 0. && (a)[2] == 0.)

#define Vsub(a,b,c) {(a)[0]=(b)[0]-(c)[0];(a)[1]=(b)[1]-(c)[1];(a)[2]=(b)[2]-(c)[2];}

void vect_copy ( Vector des, Vector sou ) { Vcopy( des, sou ) } /* vect_copy */
int vect_equal ( Vector v1, Vector v2 )
{
    return ( v1[0] == v2[0] && v1[1] == v2[1] && v1[2] == v2[2] );
} /* vect_equal */


void vect_sub ( Vector res, Vector v1, Vector v2 ) { Vsub( res, v1, v2 ) } /* vect_sub */

void vect_min ( Vector v1, Vector v2, Vector v3 )
{
    v1[0] = (v2[0] < v3[0]) ? v2[0] : v3[0];
    v1[1] = (v2[1] < v3[1]) ? v2[1] : v3[1];
    v1[2] = (v2[2] < v3[2]) ? v2[2] : v3[2];
} /* vect_min */

void vect_max ( Vector v1, Vector v2, Vector v3 )
{
    v1[0] = (v2[0] > v3[0]) ? v2[0] : v3[0];
    v1[1] = (v2[1] > v3[1]) ? v2[1] : v3[1];
    v1[2] = (v2[2] > v3[2]) ? v2[2] : v3[2];
} /* vect_max */

/* ///////////////////////////////////////////////////////////////////////////// * /
/ *  2-Dimentional stuff
/ * ///////////////////////////////////////////////////////////////////////////// */


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

double vect_len2(Vector v)
{
    return ((double)v[0] * v[0] + (double)v[1] * v[1]); /* why sqrt? only used for compares */
}

double vect_dist2(Vector v1, Vector v2)
{
	double dx = v1[0] - v2[0];
	double dy = v1[1] - v2[1];

    return (dx * dx + dy * dy); /* why sqrt */
}

/* value has sign of angle between vectors */
double vect_det2(Vector v1, Vector v2)
{
    return (((double)v1[0] * v2[1]) - ((double)v2[0] * v1[1]));
}

static
double vect_m_dist(Vector v1, Vector v2)
{
	double dx = v1[0] - v2[0];
	double dy = v1[1] - v2[1];
    double dd = (dx * dx + dy * dy); /* sqrt */
	
    if (dx > 0)  
        return +dd;
    if (dx < 0)
        return -dd;
    if (dy > 0)
        return +dd;
    return -dd;
} /* vect_m_dist */

/*
vect_inters2
 (C) 1993 Klamer Schutte
 (C) 1997 Michael Leonov, Alexey Nikitin
*/

int vect_inters2(Vector p1, Vector p2, Vector q1, Vector q2, 
				 Vector S1, Vector S2)
{
    double s, t, deel;
    double rpx, rpy, rqx, rqy;

    if (max(p1[0], p2[0]) < min(q1[0], q2[0]) ||
		max(q1[0], q2[0]) < min(p1[0], p2[0]) ||
		max(p1[1], p2[1]) < min(q1[1], q2[1]) ||
		max(q1[1], q2[1]) < min(p1[1], p2[1]))
        return 0;

    S1[2] = S2[2] = 0;
    rpx = p2[0] - p1[0];
    rpy = p2[1] - p1[1];
    rqx = q2[0] - q1[0];
    rqy = q2[1] - q1[1];

    deel = rpy * rqx - rpx * rqy; /* -vect_det(rp,rq); */

	/* coordinates are 30-bit integers so deel will be exactly zero
	 * if the lines are parallel
	 */

    if (deel == 0) /* parallel */
    {
		double dc1, dc2, d1, d2, h;	/* Check too see whether p1-p2 and q1-q2 are on the same line */
	    Vector hp1, hq1, hp2, hq2, q1p1, q1q2;

        Vsub2(q1p1, q1, p1);
        Vsub2(q1q2, q1, q2);


        /* If this product is not zero then p1-p2 and q1-q2 are not on same line! */
        if (vect_det2(q1p1, q1q2) != 0)
            return 0;
        dc1 = 0;  /* m_len(p1 - p1) */

        dc2 = vect_m_dist(p1, p2);
        d1 = vect_m_dist(p1, q1);
        d2 = vect_m_dist(p1, q2);

/* Sorting the independent points from small to large: */
        Vcpy2(hp1, p1);
        Vcpy2(hp2, p2);
        Vcpy2(hq1, q1);
        Vcpy2(hq2, q2);
        if (dc1 > dc2)
        { /* hv and h are used as help-variable. */
            Vswp2(hp1, hp2);
            h = dc1, dc1 = dc2, dc2 = h;
        }
        if (d1 > d2)
        {
            Vswp2(hq1, hq2);
            h = d1, d1 = d2, d2 = h;
        }

/* Now the line-pieces are compared: */

        if (dc1 < d1)
        {
            if (dc2 < d1)
                return 0;
            if (dc2 < d2)	{ Vcpy2(S1, hp2); Vcpy2(S2, hq1); }
            else			{ Vcpy2(S1, hq1); Vcpy2(S2, hq2); };
        }
        else
        {
            if (dc1 > d2)
                return 0;
            if (dc2 < d2)	{ Vcpy2(S1, hp1); Vcpy2(S2, hp2); }
            else			{ Vcpy2(S1, hp1); Vcpy2(S2, hq2); };
        }
        return (Vequ2(S1, S2) ? 1 : 2);
    }
    else
    {  /* not parallel */
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

        if (Vequ2(q1, p1) || Vequ2(q1, p2)) Vcpy2(S1, q1) else 
		if (Vequ2(q2, p1) || Vequ2(q2, p2)) Vcpy2(S1, q2) else
        {
	    s = (rqy * (p1[0] - q1[0]) + rqx * (q1[1] - p1[1])) / deel; 
	    if (s < 0 || s > 1)
	       return 0;
            t = (rpy * (p1[0] - q1[0]) + rpx * (q1[1] - p1[1])) / deel;
	    if (t < 0 || t > 1)
	       return 0;
            S1[0] = q1[0] + ROUND(t * rqx);
            S1[1] = q1[1] + ROUND(t * rqy);
        }
	return 1;
    }
} /* vect_inters2 */

/* how about expanding polygons so that edges can be arcs rather than
 * lines. Consider using the third coordinate to store the radius of the
 * arc. The arc would pass through the vertex points. Positive radius
 * would indicate the arc bows left (center on right of P1-P2 path)
 * negative radius would put the center on the other side. 0 radius
 * would mean the edge is a line instead of arc
 * The intersections of the two circles centered at the vertex points
 * would determine the two possible arc centers. If P2.x > P1.x then
 * the center with smaller Y is selected for positive r. If P2.y > P1.y
 * then the center with greate X is selected for positive r.
 *
 * the vec_inters2() routine would then need to handle line-line
 * line-arc and arc-arc intersections.
 *
 * perhaps reverse tracing the arc would require look-ahead to check
 * for arcs
 */
