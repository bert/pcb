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

/* this file, kdtree.c, was written and is
 * Copyright (c) 2001 C. Scott Ananian
 */

/* functions used for k-d tree.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>

#include "global.h"
#include "mymem.h"

#include "kdtree.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* define this for more thorough self-checking of data structures */
#undef SLOW_ASSERTIONS
/* define this to auto-optimize the kd-tree when the maximum rank
 * exceeds twice the log-base-2 of the size. */
//#define AUTO_OPTIMIZE
/* define this to print a message on standard out whenever the kdtree
 * is optimized. */
#undef SHOW_OPTIMIZATION

#define OTHERSIDE(x) (3&((x)+2))
#define LO_SIDE(x) ((x)&1)
#define HI_SIDE(x) (2+LO_SIDE(x))

/* all rectangles are assumed to be closed on the top and left and
 * open on the bottom and right.   That is, they include their top-left
 * corner but don't include their bottom-right corner. */

struct kdtree
{
  struct kdtree_node *root;	/* root of the tree */
  BoxType bounding_box;		/* (non-tight) bounding box for all items in tree */
};
struct kdtree_node
{
  const BoxType *box;		/* rectangle stored at this node */
  Position lo_min_bound, hi_max_bound, other_bound;
  struct kdtree_node *parent, *lo_kid, *hi_kid;
  struct
  {
    unsigned discrim:2;		/* discriminator */
    unsigned inactive:1;	/* active bit */
    unsigned own_box:1;		/* true==should free 'box' when node is destroyed */
  }
  flags;
#ifdef AUTO_OPTIMIZE
  int rank, size;
#endif				/* AUTO_OPTIMIZE */
};

/* select numbered Position from box */
static Position
key (const BoxType * box, int which)
{
  switch (which)
    {
    default:
      assert (0);
    case 0:
      return box->X1;
    case 1:
      return box->Y1;
    case 2:
      return box->X2;
    case 3:
      return box->Y2;
    }
}

/* helpers for assertions */
#ifndef NDEBUG
static int
__kd_node_is_good (struct kdtree_node *node)
{
  int r = node &&
    /* constraints on included box */
    (node->box &&
     node->box->X1 <= node->box->X2 && node->box->Y1 <= node->box->Y2) &&
    /* constraints on discriminator */
    (node->flags.discrim >= 0 && node->flags.discrim < 4) &&
    /* constraints on bounds */
    (node->lo_kid == NULL || (node->lo_min_bound <= key (node->lo_kid->box,
							 LO_SIDE (node->
								  flags.discrim))))
    && (node->hi_kid == NULL || (node->hi_max_bound >= key (node->hi_kid->box,
							    HI_SIDE
							    (node->flags.discrim))))
    &&
    /** XXX: bounds on other_bound */
    /* constraints on parent/kid pointers */
    (node->parent == NULL ||
     node == node->parent->lo_kid || node == node->parent->hi_kid) &&
    (node->lo_kid == NULL || node == node->lo_kid->parent) &&
    (node->hi_kid == NULL || node == node->hi_kid->parent) &&
#ifdef AUTO_OPTIMIZE
    /* rank and size validity */
    (node->rank == 1 + MAX (node->lo_kid == NULL ? 0 : node->lo_kid->rank,
			    node->hi_kid == NULL ? 0 : node->hi_kid->rank)) &&
    (node->size == ((node->flags.inactive ? 0 : 1) +
		    (node->lo_kid == NULL ? 0 : node->lo_kid->size) +
		    (node->hi_kid == NULL ? 0 : node->hi_kid->size))) &&
#endif				/* AUTO_OPTIMIZE */
    /* done */
    1;
  assert (r);			/* bad node */
  return r;
}

#ifdef SLOW_ASSERTIONS
static int
__kd_tree_is_good_bounded (struct kdtree_node *node, BoxType lt, BoxType ge)
{
  int r = 0;
  if (__kd_node_is_good (node) &&
      node->box->X1 < lt.X1 && node->box->X2 < lt.X2 &&
      node->box->Y1 < lt.Y1 && node->box->Y2 < lt.Y2 &&
      node->box->X1 >= ge.X1 && node->box->X2 >= ge.X2 &&
      node->box->Y1 >= ge.Y1 && node->box->Y2 >= ge.Y2)
    {
      /* new bounds, yippee! */
      /* all nodes in lo_kid have keys < this,
       * all nodes in hi_kid have keys >=this */
      BoxType lo_lt = lt, hi_lt = lt, lo_ge = ge, hi_ge = ge;
      switch (node->flags.discrim)
	{
	case 0:
	  /* key bounds */
	  lo_lt.X1 = MIN (lo_lt.X1, node->box->X1);
	  hi_ge.X1 = MAX (hi_ge.X1, node->box->X1);
	  /* other bounds */
	  lo_ge.X1 = MAX (lo_ge.X1, node->lo_min_bound);
	  lo_lt.X2 = MIN (lo_lt.X2, node->other_bound + 1);	/* less-than-or-equals */
	  hi_lt.X2 = MIN (hi_lt.X2, node->hi_max_bound + 1);	/* less-than-or-equals */
	  break;
	case 1:
	  /* key bounds */
	  lo_lt.Y1 = MIN (lo_lt.Y1, node->box->Y1);
	  hi_ge.Y1 = MAX (hi_ge.Y1, node->box->Y1);
	  /* other bounds */
	  lo_ge.Y1 = MAX (lo_ge.Y1, node->lo_min_bound);
	  lo_lt.Y2 = MIN (lo_lt.Y2, node->other_bound + 1);	/* less-than-or-equals */
	  hi_lt.Y2 = MIN (hi_lt.Y2, node->hi_max_bound + 1);	/* less-than-or-equals */
	  break;
	case 2:
	  /* key bounds */
	  lo_lt.X2 = MIN (lo_lt.X2, node->box->X2);
	  hi_ge.X2 = MAX (hi_ge.X2, node->box->X2);
	  /* other bounds */
	  lo_ge.X1 = MAX (lo_ge.X1, node->lo_min_bound);
	  hi_ge.X1 = MIN (hi_ge.X1, node->other_bound);
	  hi_lt.X2 = MIN (hi_lt.X2, node->hi_max_bound + 1);	/* less-than-or-equals */
	  break;
	case 3:
	  /* key bounds */
	  lo_lt.Y2 = MIN (lo_lt.Y2, node->box->Y2);
	  hi_ge.Y2 = MAX (hi_ge.Y2, node->box->Y2);
	  /* other bounds */
	  lo_ge.Y1 = MAX (lo_ge.Y1, node->lo_min_bound);
	  hi_ge.Y1 = MIN (hi_ge.Y1, node->other_bound);
	  hi_lt.Y2 = MIN (hi_lt.Y2, node->hi_max_bound + 1);	/* less-than-or-equals */
	  break;
	default:
	  assert (0);
	}
      r = (node->lo_kid == NULL ||
	   __kd_tree_is_good_bounded (node->lo_kid, lo_lt, lo_ge)) &&
	(node->hi_kid == NULL ||
	 __kd_tree_is_good_bounded (node->hi_kid, hi_lt, hi_ge));
    }
  assert (r);			/* bad node */
  return r;
}
static int
__kd_tree_is_good (struct kdtree *kdtree)
{
  BoxType lt, ge;
  if (kdtree == NULL)
    return 0;
  if (kdtree->root == NULL)
    return 1;
  lt.X1 = lt.X2 = 1 + kdtree->bounding_box.X2;
  lt.Y1 = lt.Y2 = 1 + kdtree->bounding_box.Y2;
  ge.X1 = ge.X2 = kdtree->bounding_box.X1;
  ge.Y1 = ge.Y2 = kdtree->bounding_box.Y1;
  /* use bounding box to determine initial bounds */
  return __kd_tree_is_good_bounded (kdtree->root, lt, ge);
}
#endif /* SLOW_ASSERTIONS */
#endif /* ! NDEBUG */

/* select the k'th sorted element from the boxlist */
/* O(end-start) time. */
static void
__select_box (const BoxType * boxlist[],
	      Cardinal start, Cardinal end, Cardinal k, int d)
{
  Position v;
  int i, j, l, r;
#define SWAP(x,y) do { \
 const BoxType * t;\
 t=boxlist[x]; boxlist[x]=boxlist[y]; boxlist[y]=t; \
} while(0)

  l = start;
  r = end - 1;
  while (r > l)
    {
      /* median of three partition */
      {
	int lo = l, mid = (l + r) / 2, hi = r;
	if (key (boxlist[lo], d) > key (boxlist[mid], d))
	  SWAP (lo, mid);
	if (key (boxlist[lo], d) > key (boxlist[hi], d))
	  SWAP (lo, hi);
	if (key (boxlist[mid], d) > key (boxlist[hi], d))
	  SWAP (mid, hi);
	SWAP (mid, r - 1);
      }
      v = key (boxlist[r - 1], d);	/* partition element */
      i = l;
      j = r - 1;
      if (i < j)
	for (;;)
	  {
	    while (key (boxlist[++i], d) < v);
	    while (key (boxlist[--j], d) > v);
	    if (i >= j)
	      break;
	    SWAP (i, j);
	  }
      SWAP (i, r - 1);		/* move partitioning element */
      if (i >= k)
	r = i - 1;
      if (i <= k)
	l = i + 1;
    }
}

/* O(end-start) time */
static Cardinal
__partition (const BoxType * boxlist[],
	     const Boolean managelist[],
	     Cardinal start, Cardinal end, struct kdtree_node *node)
{
  BoxType minbox, maxbox;
  int i, median;
  Position k, ob;
  int d;
  /* find bounding box of boxlist */
  minbox = maxbox = *boxlist[start];
  for (i = start + 1; i < end; i++)
    {
      minbox.X1 = MIN (minbox.X1, boxlist[i]->X1);
      minbox.Y1 = MIN (minbox.Y1, boxlist[i]->Y1);
      minbox.X2 = MIN (minbox.X2, boxlist[i]->X2);
      minbox.Y2 = MIN (minbox.Y2, boxlist[i]->Y2);

      maxbox.X1 = MAX (maxbox.X1, boxlist[i]->X1);
      maxbox.Y1 = MAX (maxbox.Y1, boxlist[i]->Y1);
      maxbox.X2 = MAX (maxbox.X2, boxlist[i]->X2);
      maxbox.Y2 = MAX (maxbox.Y2, boxlist[i]->Y2);
    }
  /* find largest dimension */
  {
    int size, tmp;
    d = 0;
    size = maxbox.X1 - minbox.X1;
    if (size < (tmp = maxbox.Y1 - minbox.Y1))
      {
	d = 1;
	size = tmp;
      }
    if (size < (tmp = maxbox.X2 - minbox.X2))
      {
	d = 2;
	size = tmp;
      }
    if (size < (tmp = maxbox.Y2 - minbox.Y2))
      {
	d = 3;
	size = tmp;
      }
  }
  /* d now has largest dimension */
  node->flags.discrim = d;
  node->lo_min_bound = key (&minbox, LO_SIDE (d));
  node->hi_max_bound = key (&maxbox, HI_SIDE (d));

  /* find median element of dimension d */
  median = (start + end) / 2;
  __select_box (boxlist, start, end, median, d);
  k = key (boxlist[median], d);

  /* now partition such that all lo kids are < median and hi >= median.
   * current median is 'true' median and so lo kids <= median and hi >= median
   */
  for (i = start;;)
    {
      while (key (boxlist[i], d) < k)
	i++;
      while (i <= median && key (boxlist[median], d) >= k)
	median--;
      if (i > median)
	break;
      SWAP (i, median);
    }
  node->box = boxlist[median = i];
  node->flags.own_box = managelist[i];

  /* find other_bound */
  if (d < 2)
    {				/* low-end keys */
      ob = node->lo_min_bound;
      for (i = start; i < median; i++)
	ob = MAX (ob, key (boxlist[i], HI_SIDE (d)));
    }
  else
    {				/* high-end keys */
      ob = node->hi_max_bound;
      for (i = end - 1; i > median; i--)
	ob = MIN (ob, key (boxlist[i], LO_SIDE (d)));
    }
  node->other_bound = ob;

  /* done */
  return median;
}

/* O(N ln N) time */
static struct kdtree_node *
__kd_create_tree (const BoxType * boxlist[],
		  const Boolean managelist[], Cardinal start, Cardinal end)
{
  struct kdtree_node *node = calloc (1, sizeof (*node));
  Cardinal i;

  assert (start < end);

  if (end - start == 1)
    {
      /* trivial case */
      node->box = boxlist[start];
      node->flags.own_box = managelist[start];
    }
  else
    {
      /* non-trivial case */
      i = __partition (boxlist, managelist, start, end, node);
      assert (start <= i && i < end);
      assert (0 <= node->flags.discrim && node->flags.discrim < 4);
      assert (node->box);
      if (start < i)
	{
	  node->lo_kid = __kd_create_tree (boxlist, managelist, start, i);
	  node->lo_kid->parent = node;
	}
      if (i + 1 < end)
	{
	  node->hi_kid = __kd_create_tree (boxlist, managelist, i + 1, end);
	  node->hi_kid->parent = node;
	}
    }
#ifdef AUTO_OPTIMIZE
  node->rank = 1 + MAX (node->lo_kid ? node->lo_kid->rank : 0,
			node->hi_kid ? node->hi_kid->rank : 0);
  node->size = 1 + ((node->lo_kid ? node->lo_kid->size : 0) +
		    (node->hi_kid ? node->hi_kid->size : 0));
#endif /* AUTO_OPTIMIZE */
  /* post condition */
  assert (__kd_node_is_good (node));
  return node;
}

/* create a k-d tree from an unsorted list of boxes.
 * the k-d tree will rearrange the box list and then keep pointers into 
 * it, so don't free the box list until you've called kd_destroy_tree.
 * if you set 'manage' to true, kd_destroy_tree will free your boxlist
 * for you.
 */
kdtree_t *
kd_create_tree (const BoxType * boxlist[], int N, int manage)
{
  struct kdtree *kdtree;
  int i;
  Boolean *managelist;
  assert (boxlist || N == 0);
  assert (N >= 0);
  kdtree = calloc (1, sizeof (*kdtree));
  /* scan through and determine bounding box */
  if (N > 0)
    {
      kdtree->bounding_box = *boxlist[0];
      for (i = 1; i < N; i++)
	{
	  const BoxType *b = boxlist[i];
	  kdtree->bounding_box.X1 = MIN (kdtree->bounding_box.X1, b->X1);
	  kdtree->bounding_box.Y1 = MIN (kdtree->bounding_box.Y1, b->Y1);
	  kdtree->bounding_box.X2 = MAX (kdtree->bounding_box.X2, b->X2);
	  kdtree->bounding_box.Y2 = MAX (kdtree->bounding_box.Y2, b->Y2);
	}
      /* create array of booleans for 'manage' flag for each box */
      managelist = malloc (N * sizeof (*managelist));
      for (i = 0; i < N; i++)
	managelist[i] = (manage != False);
      kdtree->root = __kd_create_tree (boxlist, managelist, 0, N);
      free (managelist);
    }
#ifdef SLOW_ASSERTIONS
  assert (__kd_tree_is_good (kdtree));
#endif /* SLOW_ASSERTIONS */
  /* done */
  return kdtree;
}

static void
__kd_destroy_tree (struct kdtree_node **nodepp)
{
  if ((*nodepp)->lo_kid)
    __kd_destroy_tree (&((*nodepp)->lo_kid));
  if ((*nodepp)->hi_kid)
    __kd_destroy_tree (&((*nodepp)->hi_kid));
  if ((*nodepp)->flags.own_box)
    free ((void *) (*nodepp)->box);
  free (*nodepp);
  *nodepp = NULL;
}

/* free the memory associated with a k-d tree. */
void
kd_destroy_tree (kdtree_t ** kdtree)
{
#ifdef SLOW_ASSERTIONS
  assert (__kd_tree_is_good (*kdtree));
#endif /* SLOW_ASSERTIONS */
  if ((*kdtree)->root)
    __kd_destroy_tree (&((*kdtree)->root));
  free (*kdtree);
  *kdtree = NULL;
}

static int
__kd_collect (struct kdtree_node *node, BoxType * bbox,
	      const BoxType * boxlist[], Boolean managelist[], int start)
{
  if (node == NULL)
    return start;
  if (!node->flags.inactive)
    {
      /* add box to lists */
      boxlist[start] = node->box;
      managelist[start] = node->flags.own_box;
      /* reset flags.own_box so that this box is not freed when we destroy the
       * old tree. */
      node->flags.own_box = 0;
      /* collect bounding-box information */
      bbox->X1 = MIN (bbox->X1, node->box->X1);
      bbox->Y1 = MIN (bbox->Y1, node->box->Y1);
      bbox->X2 = MAX (bbox->X2, node->box->X2);
      bbox->Y2 = MAX (bbox->Y2, node->box->Y2);
      /* increment start */
      start++;
    }
  start = __kd_collect (node->lo_kid, bbox, boxlist, managelist, start);
  start = __kd_collect (node->hi_kid, bbox, boxlist, managelist, start);
  return start;
}

/* rebuild the kd-tree (in O(N ln N) time) in order to perfectly balance it */
void
kd_optimize_tree (kdtree_t * kdtree)
{
  const BoxType **boxlist;
  Boolean *managelist;
  BoxType bbox;
  int i, size;
  assert (kdtree);
#ifdef SLOW_ASSERTIONS
  assert (__kd_tree_is_good (kdtree));
#endif /* SLOW_ASSERTIONS */
  if (kdtree->root == NULL)
    return;			/* already perfectly balanced! =) */
#ifdef SHOW_OPTIMIZATION
  printf ("OPTIMIZING TREE.... [size %d rank %d]\n",
	  kdtree->root->size, kdtree->root->rank);
#endif /* SHOW_OPTIMIZATION */
  /* determine number of active boxes in tree */
#ifdef AUTO_OPTIMIZE
  size = kdtree->root->size;	/* short cut if we're keeping stats */
#else /* ! AUTO_OPTIMIZE */
  size = kd_search (kdtree, NULL, NULL, NULL, NULL);
#endif /* ! AUTO_OPTIMIZE */
  /* initialize bbox to "very large" bounds */
  bbox.X1 = kdtree->bounding_box.X2;
  bbox.X2 = kdtree->bounding_box.X1;
  bbox.Y1 = kdtree->bounding_box.Y2;
  bbox.Y2 = kdtree->bounding_box.Y1;
  /* collect lists and bounding box information */
  boxlist = malloc (size * sizeof (*boxlist));
  managelist = malloc (size * sizeof (*managelist));
  i = __kd_collect (kdtree->root, &bbox, boxlist, managelist, 0);
  assert (i == size);
  /* destroy old tree */
  __kd_destroy_tree (&kdtree->root);
  /* (re-)create new tree */
  kdtree->bounding_box = bbox;
  kdtree->root = __kd_create_tree (boxlist, managelist, 0, size);
  /* free lists */
  free (boxlist);
  free (managelist);
  /* done! */
#ifdef SLOW_ASSERTIONS
  assert (__kd_tree_is_good (kdtree));
#endif /* SLOW_ASSERTIONS */
#ifdef SHOW_OPTIMIZATION
  printf ("           ....DONE [size %d rank %d]\n",
	  kdtree->root->size, kdtree->root->rank);
#endif /* SHOW_OPTIMIZATION */
  return;
}

int
__kd_search (struct kdtree_node *node, const BoxType * starting_region,
	     int (*region_in_search) (const BoxType * region, void *cl),
	     int (*rectangle_in_region) (const BoxType * box, void *cl),
	     void *closure)
{
  BoxType lo, hi;
  int seen = 0;
  assert (node && starting_region && region_in_search && rectangle_in_region);
  /** assert that starting_region is well formed */
  assert (starting_region->X1 <= starting_region->X2 &&
	  starting_region->Y1 <= starting_region->Y2);
  /** assert that node is well formed */
  assert (__kd_node_is_good (node));
  /* check this box */
  if ((!node->flags.inactive) &&
      node->box->X1 < starting_region->X2 &&
      node->box->X2 > starting_region->X1 &&
      node->box->Y1 < starting_region->Y2 &&
      node->box->Y2 > starting_region->Y1 &&
      rectangle_in_region (node->box, closure))
    seen++;
  /* narrow regions for low/hi children */
  lo = hi = *starting_region;
  switch (node->flags.discrim)
    {
    case 0:			/* x1 */
      lo.X1 = MAX (lo.X1, node->lo_min_bound);
      lo.X2 = MIN (lo.X2, node->other_bound);
      hi.X1 = MAX (hi.X1, node->box->X1);
      hi.X2 = MIN (hi.X2, node->hi_max_bound);
      break;
    case 1:			/* y1 */
      lo.Y1 = MAX (lo.Y1, node->lo_min_bound);
      lo.Y2 = MIN (lo.Y2, node->other_bound);
      hi.Y1 = MAX (hi.Y1, node->box->Y1);
      hi.Y2 = MIN (hi.Y2, node->hi_max_bound);
      break;
    case 2:			/* x2 */
      lo.X1 = MAX (lo.X1, node->lo_min_bound);
      lo.X2 = MIN (lo.X2, node->box->X2);
      hi.X1 = MAX (hi.X1, node->other_bound);
      hi.X2 = MIN (hi.X2, node->hi_max_bound);
      break;
    case 3:			/* y2 */
      lo.Y1 = MAX (lo.Y1, node->lo_min_bound);
      lo.Y2 = MIN (lo.Y2, node->box->Y2);
      hi.Y1 = MAX (hi.Y1, node->other_bound);
      hi.Y2 = MIN (hi.Y2, node->hi_max_bound);
      break;
    default:
      assert (0);
    }
  /* recurse if any chance lo or hi kid could be in the target region */
  if (node->lo_kid &&
      lo.X1 <= lo.X2 && lo.Y1 <= lo.Y2 && region_in_search (&lo, closure))
    seen += __kd_search (node->lo_kid, &lo,
			 region_in_search, rectangle_in_region, closure);
  if (node->hi_kid &&
      hi.X1 <= hi.X2 && hi.Y1 <= hi.Y2 && region_in_search (&hi, closure))
    seen += __kd_search (node->hi_kid, &hi,
			 region_in_search, rectangle_in_region, closure);
  /* done! */
  return seen;
}
static int
__kd_search_nop (const BoxType * b, void *cl)
{
  return 1;
}

/* parameterized search in a k-d tree.  Returns num of rectangles found. */
int
kd_search (kdtree_t * kdtree, const BoxType * starting_region,
	   int (*region_in_search) (const BoxType * region, void *cl),
	   int (*rectangle_in_region) (const BoxType * box, void *cl),
	   void *closure)
{
  BoxType bbox;
  assert (kdtree);
  if (!region_in_search)
    region_in_search = __kd_search_nop;
  if (!rectangle_in_region)
    rectangle_in_region = __kd_search_nop;
#ifdef SLOW_ASSERTIONS
  assert (__kd_tree_is_good (kdtree));
#endif /* SLOW_ASSERTIONS */
  if (kdtree->root == NULL)
    return 0;
  bbox = kdtree->bounding_box;
  if (starting_region)
    {
      /* narrow bbox according to given starting_region */
      bbox.X1 = MAX (bbox.X1, starting_region->X1);
      bbox.Y1 = MAX (bbox.Y1, starting_region->Y1);
      bbox.X2 = MIN (bbox.X2, starting_region->X2);
      bbox.Y2 = MIN (bbox.Y2, starting_region->Y2);
    }
  if (bbox.X1 <= bbox.X2 && bbox.Y1 <= bbox.Y2)
    return __kd_search (kdtree->root, &bbox,
			region_in_search, rectangle_in_region, closure);
  return 0;
}

/*------ kd_region_is_empty ------*/
static int
__kd_region_is_empty_rect_in_reg (const BoxType * box, void *cl)
{
  jmp_buf *envp = (jmp_buf *) cl;
  longjmp (*envp, 1);		/* found one! */
}

/* return 0 if there are any rectangles in the given region. */
int
kd_region_is_empty (kdtree_t * kdtree, const BoxType * region)
{
  jmp_buf env;
  int r;
  if (setjmp (env))
    return 0;
  r = kd_search (kdtree, region, NULL,
		 __kd_region_is_empty_rect_in_reg, &env);
  assert (r == 0);		/* otherwise longjmp would have been called */
  return 1;			/* no rectangles found */
}

static struct kdtree_node **
__kd_points_to (kdtree_t * kdtree, struct kdtree_node *node)
{
  struct kdtree_node **nodepp =
    (node == NULL) ? NULL :
    (node->parent == NULL) ? &kdtree->root :
    (node == node->parent->lo_kid) ?
    &(node->parent->lo_kid) : &(node->parent->hi_kid);
  assert (nodepp == NULL || *nodepp == node);
  return nodepp;
}

/* non-recursive 'node find' routine */
struct kd_find_info
{
  struct kdtree_node *parent;
  struct kdtree_node **nodepp;
};
typedef enum
{ DELETE = 0, INSERT = 1 }
is_insert_t;
typedef enum
{ INEXACT = 0, EXACT = 1 }
is_exact_t;
static struct kd_find_info
__kd_find_node (kdtree_t * kdtree,
		const BoxType * which,
		is_insert_t is_insert, is_exact_t is_exact)
{
  struct kd_find_info fi = { NULL, &kdtree->root };
#ifdef SLOW_ASSERTIONS
  assert (__kd_tree_is_good (kdtree));
#endif /* SLOW_ASSERTIONS */
  while (*fi.nodepp)
    {
      struct kdtree_node *n = *fi.nodepp;
      int d = n->flags.discrim;
      Position v = key (which, d), k = key (n->box, d);
      assert (__kd_node_is_good (n));
      /* did we find an inactive node or find a node matching box? */
      if (n->flags.inactive ? (is_insert && v == k) :
	  (is_insert == 0 && (is_exact ? n->box == which : 1) &&
	   which->X1 == n->box->X1 && which->X2 == n->box->X2 &&
	   which->Y1 == n->box->Y1 && which->Y2 == n->box->Y2))
	return fi;
      /* widen bounds if inserting */
      if (is_insert)
	{
	  if (v < k)
	    {			/* lo kid */
	      n->lo_min_bound =
		MIN (n->lo_min_bound, key (which, LO_SIDE (d)));
	      if (d < 2)
		n->other_bound =
		  MAX (n->other_bound, key (which, HI_SIDE (d)));
	    }
	  else
	    {			/* hi kid */
	      n->hi_max_bound =
		MAX (n->hi_max_bound, key (which, HI_SIDE (d)));
	      if (d > 1)
		n->other_bound =
		  MIN (n->other_bound, key (which, LO_SIDE (d)));
	    }
	}
      /* okay, where to look next? */
      fi.parent = n;
      fi.nodepp = (v < k) ? &(n->lo_kid) : &(n->hi_kid);
      assert (*fi.nodepp == NULL || (*fi.nodepp)->parent == fi.parent);
    }
  /* *fi.nodepp is NULL: this is where an insertion should occur */
  assert (fi.nodepp);
  return fi;
}

/* must be exact match for which! */
void
kd_delete_node (kdtree_t * kdtree, const BoxType * which)
{
  struct kdtree_node **nodepp =
    __kd_find_node (kdtree, which, DELETE, EXACT).nodepp;
  assert (nodepp && *nodepp);	/* node must be found */
  assert ((*nodepp)->box == which);
  /* set to 'inactive' */
  (*nodepp)->flags.inactive = 1;
  /* now zoom up parents, freeing any totally inactive subtrees. */
  while (nodepp && (*nodepp)->flags.inactive &&
	 (*nodepp)->lo_kid == NULL && (*nodepp)->hi_kid == NULL)
    {
      struct kdtree_node **next = __kd_points_to (kdtree, (*nodepp)->parent);
      __kd_destroy_tree (nodepp);
      nodepp = next;
    }
#ifdef AUTO_OPTIMIZE
  if (nodepp)
    {
      struct kdtree_node *np;
      for (np = *nodepp; np != NULL; np = np->parent)
	{
	  np->rank = 1 + MAX (np->lo_kid ? np->lo_kid->rank : 0,
			      np->hi_kid ? np->hi_kid->rank : 0);
	  np->size--;
	}
    }
#endif /* AUTO_OPTIMIZE */
#ifdef SLOW_ASSERTIONS
  assert (__kd_tree_is_good (kdtree));
  assert (*__kd_find_node (kdtree, which, DELETE, EXACT).nodepp == NULL);
#endif /* SLOW_ASSERTIONS */
}

void
kd_insert_node (kdtree_t * kdtree, const BoxType * which, int manage)
{
  struct kd_find_info fi;
  struct kdtree_node *node;
  assert (kdtree && which);
#ifdef SLOW_ASSERTIONS
  assert (__kd_tree_is_good (kdtree));
#endif /* SLOW_ASSERTIONS */
  /* expand top-level bounds */
  if (kdtree->root == NULL)
    kdtree->bounding_box = *which;
  kdtree->bounding_box.X1 = MIN (kdtree->bounding_box.X1, which->X1);
  kdtree->bounding_box.Y1 = MIN (kdtree->bounding_box.Y1, which->Y1);
  kdtree->bounding_box.X2 = MAX (kdtree->bounding_box.X2, which->X2);
  kdtree->bounding_box.Y2 = MAX (kdtree->bounding_box.Y2, which->Y2);
  /* okay, do the insertion */
  fi = __kd_find_node (kdtree, which, INSERT, INEXACT);
  assert (fi.nodepp);
  if (*fi.nodepp)
    {				/* replace an inactive entry */
      assert (__kd_node_is_good (*fi.nodepp));
      node = *fi.nodepp;
      assert (node->flags.inactive);	/* otherwise, a duplicate! */
      if (node->flags.own_box)
	free ((void *) node->box);
    }
  else
    {				/* insert on a NULL */
      node = *fi.nodepp = calloc (1, sizeof (*node));
      node->parent = fi.parent;
      node->flags.discrim = 0;
      if (node->parent)
	node->flags.discrim = node->parent->flags.discrim + 1;
      node->lo_min_bound = node->hi_max_bound = node->other_bound =
	key (which, node->flags.discrim);
    }
  /* okay, finish setting up node */
  node->box = which;
  node->flags.inactive = 0;
  node->flags.own_box = manage;
#ifdef AUTO_OPTIMIZE
  {
    struct kdtree_node *np;
    int rank = MAX (node->lo_kid ? node->lo_kid->rank : 0,
		    node->hi_kid ? node->hi_kid->rank : 0);
    for (np = node; np != NULL; np = np->parent)
      {
	rank = np->rank = MAX (np->rank, 1 + rank);
	np->size++;
      }
  }
#endif /* AUTO_OPTIMIZE */
  assert (__kd_node_is_good (node));
  /* bounds inside tree have already been expanded in kd_find_node */
#ifdef SLOW_ASSERTIONS
  assert (__kd_tree_is_good (kdtree));
  assert (*__kd_find_node (kdtree, which, DELETE, EXACT).nodepp == node);
#endif /* SLOW_ASSERTIONS */
#ifdef AUTO_OPTIMIZE
  assert (kdtree->root);
  if (kdtree->root->size < (1 << (kdtree->root->rank / 2)))
    kd_optimize_tree (kdtree);
#endif /* AUTO_OPTIMIZE */
}

static int
__kd_print (const BoxType * box, void *cl)
{
  FILE *stream = (FILE *) cl;
  fprintf (stream, "  (%d, %d)-(%d, %d)\n",
	   box->X1, box->Y1, box->X2, box->Y2);
  return 1;
}

/* dump a listing of all rectangles in the kd-tree to the stream */
void
kd_print (kdtree_t * kdtree, FILE * stream)
{
  int n;
  fprintf (stream, "KDTREE bounds (%d, %d)-(%d, %d)\n",
	   kdtree->bounding_box.X1, kdtree->bounding_box.Y1,
	   kdtree->bounding_box.X2, kdtree->bounding_box.Y2);
  n = kd_search (kdtree, NULL, NULL, __kd_print, stream);
  fprintf (stream, "KDTREE end (%d rectangles)\n", n);
}

#ifdef KDTREE_STATISTICS
/* statistics code, if you're interested in peering a bit into the
 * performance of your kd-trees. */
int
kd_size (kdtree_t * kdtree)
{
  return kd_search (kdtree, NULL, NULL, NULL, NULL);
}
static int
__kd_inactive_node (struct kdtree_node *node)
{
  if (node == NULL)
    return 0;
  return (node->flags.inactive ? 1 : 0) +
    __kd_inactive_node (node->lo_kid) + __kd_inactive_node (node->hi_kid);
}

int
kd_inactive (kdtree_t * kdtree)
{
  return __kd_inactive_node (kdtree->root);
}
static int
__kd_max_rank_node (struct kdtree_node *node)
{
  int l, h;
  if (node == NULL)
    return 0;
  l = __kd_max_rank_node (node->lo_kid);
  h = __kd_max_rank_node (node->hi_kid);
  return 1 + MAX (l, h);
}

int
kd_max_rank (kdtree_t * kdtree)
{
  return __kd_max_rank_node (kdtree->root);
}
#endif /* KDTREE_STATISTICS */
