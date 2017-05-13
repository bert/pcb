/*!
 * \file src/rtree.c
 *
 * \brief Implements r-tree structures.
 *
 * These should be much faster for the auto-router because the recursive
 * search is much more efficient and that's where the auto-router spends
 * all its time.
 *
 * \author this file, rtree.c, was written and is Copyright (c) 2004,
 * harry eaton
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
 *
 * Copyright (C) 1998,1999,2000,2001,2002,2003,2004 harry eaton
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
 *
 * harry eaton, 6697 Buttonhole Ct, Columbia, MD 21044 USA
 *
 * haceaton@aplcomm.jhuapl.edu
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"

#include <assert.h>
#include <inttypes.h>
#include <setjmp.h>

#include "mymem.h"

#include "rtree.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define SLOW_ASSERTS
/* All rectangles are closed on the bottom left and open on the
 * top right. i.e. they contain one corner point, but not the other.
 * This requires that the corner points not be equal!
 */

/* the number of entries in each rtree node
 * 4 - 7 seem to be pretty good settings
 */
#define M_SIZE 6

/* it seems that sorting the leaf order slows us down
 * but sometimes gives better routes
 */
#undef SORT
#define SORT_NONLEAF

#define DELETE_BY_POINTER

typedef struct
{
  const BoxType *bptr;          /* pointer to the box */
  BoxType bounds;               /* copy of the box for locality of reference */
} Rentry;

struct rtree_node
{
  BoxType box;                  /* bounds rectangle of this node */
  struct rtree_node *parent;    /* parent of this node, NULL = root */
  struct
  {
    unsigned is_leaf:1;         /* this is a leaf node */
    unsigned manage:31;         /* true==should free 'rect.bptr' if node is destroyed */
  }
  flags;
  union
  {
    struct rtree_node *kids[M_SIZE + 1];        /* when not leaf */
    Rentry rects[M_SIZE + 1];   /* when leaf */
  } u;
};

#ifndef NDEBUG
#ifdef SLOW_ASSERTS
static int
__r_node_is_good (struct rtree_node *node)
{
  int i, flag = 1;
  int kind = -1;
  bool last = false;

  if (node == NULL)
    return 1;
  for (i = 0; i < M_SIZE; i++)
    {
      if (node->flags.is_leaf)
        {
          if (!node->u.rects[i].bptr)
            {
              last = true;
              continue;
            }
          /* check that once one entry is empty, all the rest are too */
          if (node->u.rects[i].bptr && last)
            assert (0);
          /* check that the box makes sense */
          if (node->box.X1 > node->box.X2)
            assert (0);
          if (node->box.Y1 > node->box.Y2)
            assert (0);
          /* check that bounds is the same as the pointer */
          if (node->u.rects[i].bounds.X1 != node->u.rects[i].bptr->X1)
            assert (0);
          if (node->u.rects[i].bounds.Y1 != node->u.rects[i].bptr->Y1)
            assert (0);
          if (node->u.rects[i].bounds.X2 != node->u.rects[i].bptr->X2)
            assert (0);
          if (node->u.rects[i].bounds.Y2 != node->u.rects[i].bptr->Y2)
            assert (0);
          /* check that entries are within node bounds */
          if (node->u.rects[i].bounds.X1 < node->box.X1)
            assert (0);
          if (node->u.rects[i].bounds.X2 > node->box.X2)
            assert (0);
          if (node->u.rects[i].bounds.Y1 < node->box.Y1)
            assert (0);
          if (node->u.rects[i].bounds.Y2 > node->box.Y2)
            assert (0);
        }
      else
        {
          if (!node->u.kids[i])
            {
              last = true;
              continue;
            }
          /* make sure all children are the same type */
          if (kind == -1)
            kind = node->u.kids[i]->flags.is_leaf;
          else if (kind != node->u.kids[i]->flags.is_leaf)
            assert (0);
          /* check that once one entry is empty, all the rest are too */
          if (node->u.kids[i] && last)
            assert (0);
          /* check that entries are within node bounds */
          if (node->u.kids[i]->box.X1 < node->box.X1)
            assert (0);
          if (node->u.kids[i]->box.X2 > node->box.X2)
            assert (0);
          if (node->u.kids[i]->box.Y1 < node->box.Y1)
            assert (0);
          if (node->u.kids[i]->box.Y2 > node->box.Y2)
            assert (0);
        }
      flag <<= 1;
    }
  /* check that we're completely in the parent's bounds */
  if (node->parent)
    {
      if (node->parent->box.X1 > node->box.X1)
        assert (0);
      if (node->parent->box.X2 < node->box.X2)
        assert (0);
      if (node->parent->box.Y1 > node->box.Y1)
        assert (0);
      if (node->parent->box.Y2 < node->box.Y2)
        assert (0);
    }
  /* make sure overflow is empty */
  if (!node->flags.is_leaf && node->u.kids[i])
    assert (0);
  if (node->flags.is_leaf && node->u.rects[i].bptr)
    assert (0);
  return 1;
}

/*!
 * \brief Check the whole tree from this node down for consistency.
 */
static bool
__r_tree_is_good (struct rtree_node *node)
{
  int i;

  if (!node)
    return 1;
  if (!__r_node_is_good (node))
    assert (0);
  if (node->flags.is_leaf)
    return 1;
  for (i = 0; i < M_SIZE; i++)
    {
      if (!__r_tree_is_good (node->u.kids[i]))
        return 0;
    }
  return 1;
}
#endif
#endif

#ifndef NDEBUG
/*!
 * \brief Print out the tree.
 */
void
__r_dump_tree (struct rtree_node *node, int depth)
{
  int i, j;
  static int count;
  static double area;

  if (depth == 0)
    {
      area = 0;
      count = 0;
    }
  area +=
    (node->box.X2 - node->box.X1) * (double) (node->box.Y2 - node->box.Y1);
  count++;
  for (i = 0; i < depth; i++)
    printf ("  ");
  if (!node->flags.is_leaf)
    {
      printf (
          "p=0x%p node X(%" PRIi64 ", %" PRIi64 ") Y(%" PRIi64 ", %" PRIi64 
          ")\n",
          (void *) node,
          (int64_t) (node->box.X1),
          (int64_t) (node->box.X2),
          (int64_t) (node->box.Y1),
          (int64_t) (node->box.Y2) );
    }
  else
    {
      printf (
          "p=0x%p leaf manage(%02x) X(%" PRIi64 ", %" PRIi64 ") Y(%" PRIi64
          ", %" PRIi64 ")\n",
          (void *) node,
          node->flags.manage,
          (int64_t) (node->box.X1),
          (int64_t) (node->box.X2),
          (int64_t) (node->box.Y1),
          (int64_t) (node->box.Y2) );
      for (j = 0; j < M_SIZE; j++)
        {
          if (!node->u.rects[j].bptr)
            break;
          area +=
            (node->u.rects[j].bounds.X2 -
             node->u.rects[j].bounds.X1) *
            (double) (node->u.rects[j].bounds.Y2 -
                      node->u.rects[j].bounds.Y1);
          count++;
          for (i = 0; i < depth + 1; i++)
            printf ("  ");
          printf (
              "entry 0x%p X(%" PRIi64 ", %" PRIi64 ") Y(%" PRIi64 ", "
              "%" PRIi64 ")\n",
              (void *) (node->u.rects[j].bptr),
              (int64_t) (node->u.rects[j].bounds.X1),
              (int64_t) (node->u.rects[j].bounds.X2),
              (int64_t) (node->u.rects[j].bounds.Y1),
              (int64_t) (node->u.rects[j].bounds.Y2) );
        }
      return;
    }
  for (i = 0; i < M_SIZE; i++)
    if (node->u.kids[i])
      __r_dump_tree (node->u.kids[i], depth + 1);
  if (depth == 0)
    printf ("average box area is %g\n", area / count);
}
#endif

#ifdef SORT
/*!
 * \brief Sort the children or entries of a node according to the
 * largest side.
 *
 * Compare two box coordinates so that the __r_search will fail at the
 * earliest comparison possible.
 *
 * It needs to see the biggest X1 first, then the smallest X2, the
 * biggest Y1 and smallest Y2.
 */
static int
cmp_box (const BoxType * a, const BoxType * b)
{
  if (a->X1 < b->X1)
    return 0;
  if (a->X1 > b->X1)
    return 1;
  if (a->X2 > b->X2)
    return 0;
  if (a->X2 < b->X2)
    return 1;
  if (a->Y1 < b->Y1)
    return 0;
  if (a->Y1 > b->Y1)
    return 1;
  if (a->Y2 > b->Y2)
    return 0;
  return 1;
}

static void
sort_node (struct rtree_node *node)
{
  if (node->flags.is_leaf)
    {
      register Rentry *r, *i, temp;

      for (r = &node->u.rects[1]; r->bptr; r++)
        {
          temp = *r;
          i = r - 1;
          while (i >= &node->u.rects[0])
            {
              if (cmp_box (&i->bounds, &r->bounds))
                break;
              *(i + 1) = *i;
              i--;
            }
          *(i + 1) = temp;
        }
    }
#ifdef SORT_NONLEAF
  else
    {
      register struct rtree_node **r, **i, *temp;

      for (r = &node->u.kids[1]; *r; r++)
        {
          temp = *r;
          i = r - 1;
          while (i >= &node->u.kids[0])
            {
              if (cmp_box (&(*i)->box, &(*r)->box))
                break;
              *(i + 1) = *i;
              i--;
            }
          *(i + 1) = temp;
        }
    }
#endif
}
#else
#define sort_node(x)
#endif

/*!
 * \brief Set the node bounds large enough to encompass all of the
 * children's rectangles.
 */
static void
adjust_bounds (struct rtree_node *node)
{
  int i;

  assert (node);
  assert (node->u.kids[0]);
  if (node->flags.is_leaf)
    {
      node->box = node->u.rects[0].bounds;
      for (i = 1; i < M_SIZE + 1; i++)
        {
          if (!node->u.rects[i].bptr)
            return;
          MAKEMIN (node->box.X1, node->u.rects[i].bounds.X1);
          MAKEMAX (node->box.X2, node->u.rects[i].bounds.X2);
          MAKEMIN (node->box.Y1, node->u.rects[i].bounds.Y1);
          MAKEMAX (node->box.Y2, node->u.rects[i].bounds.Y2);
        }
    }
  else
    {
      node->box = node->u.kids[0]->box;
      for (i = 1; i < M_SIZE + 1; i++)
        {
          if (!node->u.kids[i])
            return;
          MAKEMIN (node->box.X1, node->u.kids[i]->box.X1);
          MAKEMAX (node->box.X2, node->u.kids[i]->box.X2);
          MAKEMIN (node->box.Y1, node->u.kids[i]->box.Y1);
          MAKEMAX (node->box.Y2, node->u.kids[i]->box.Y2);
        }
    }
}

/*!
 * \brief Create an r-tree from an unsorted list of boxes.
 *
 * Create an rtree from the list of boxes.  If 'manage' is true, then
 * the tree will take ownership of 'boxlist' and free it when the tree
 * is destroyed.
 *
 * The r-tree will keep pointers into it, so don't free the box list
 * until you've called r_destroy_tree.
 *
 * If you set 'manage' to true, r_destroy_tree will free your boxlist.
 */
rtree_t *
r_create_tree (const BoxType * boxlist[], int N, int manage)
{
  rtree_t *rtree;
  struct rtree_node *node;
  int i;

  assert (N >= 0);
  rtree = (rtree_t *)calloc (1, sizeof (*rtree));
  /* start with a single empty leaf node */
  node = (struct rtree_node *)calloc (1, sizeof (*node));
  node->flags.is_leaf = 1;
  node->parent = NULL;
  rtree->root = node;
  /* simple, just insert all of the boxes! */
  for (i = 0; i < N; i++)
    {
      assert (boxlist[i]);
      r_insert_entry (rtree, boxlist[i], manage);
    }
#ifdef SLOW_ASSERTS
  assert (__r_tree_is_good (rtree->root));
#endif
  return rtree;
}

/*!
 * \brief Destroy an rtree.
 */
static void
__r_destroy_tree (struct rtree_node *node)
{
  int i, flag = 1;

  if (node->flags.is_leaf)
    for (i = 0; i < M_SIZE; i++)
      {
        if (!node->u.rects[i].bptr)
          break;
        if (node->flags.manage & flag)
          free ((void *) node->u.rects[i].bptr);
        flag = flag << 1;
      }
  else
    for (i = 0; i < M_SIZE; i++)
      {
        if (!node->u.kids[i])
          break;
        __r_destroy_tree (node->u.kids[i]);
      }
  free (node);
}

/*!
 * \brief Free the memory associated with an rtree.
 */
void
r_destroy_tree (rtree_t ** rtree)
{

  __r_destroy_tree ((*rtree)->root);
  free (*rtree);
  *rtree = NULL;
}

typedef struct
{
  int (*check_it) (const BoxType * region, void *cl);
  int (*found_it) (const BoxType * box, void *cl);
  void *closure;
} r_arg;

/*!
 * \brief .
 *
 * Most of the auto-routing time is spent in this routine so some
 * careful thought has been given to maximizing the speed.
 *
 */
int
__r_search (struct rtree_node *node, const BoxType * query, r_arg * arg)
{
  assert (node);
  /* assert that starting_region is well formed */
  assert (query->X1 < query->X2 && query->Y1 < query->Y2);
  assert (node->box.X1 < query->X2 && node->box.X2 > query->X1 &&
          node->box.Y1 < query->Y2 && node->box.Y2 > query->Y1);
#ifdef SLOW_ASSERTS
  /* assert that node is well formed */
  assert (__r_node_is_good (node));
#endif
  /* the check for bounds is done before entry. This saves the overhead
   * of building/destroying the stack frame for each bounds that fails
   * to intersect, which is the most common condition.
   */
  if (node->flags.is_leaf)
    {
      register int i;
      if (arg->found_it)        /* test this once outside of loop */
        {
          register int seen = 0;
          for (i = 0; node->u.rects[i].bptr; i++)
            {
              if ((node->u.rects[i].bounds.X1 < query->X2) &&
                  (node->u.rects[i].bounds.X2 > query->X1) &&
                  (node->u.rects[i].bounds.Y1 < query->Y2) &&
                  (node->u.rects[i].bounds.Y2 > query->Y1) &&
                  arg->found_it (node->u.rects[i].bptr, arg->closure))
                seen++;
            }
          return seen;
        }
      else
        {
          register int seen = 0;
          for (i = 0; node->u.rects[i].bptr; i++)
            {
              if ((node->u.rects[i].bounds.X1 < query->X2) &&
                  (node->u.rects[i].bounds.X2 > query->X1) &&
                  (node->u.rects[i].bounds.Y1 < query->Y2) &&
                  (node->u.rects[i].bounds.Y2 > query->Y1))
                seen++;
            }
          return seen;
        }
    }

  /* not a leaf, recurse on lower nodes */
  if (arg->check_it)
    {
      int seen = 0;
      struct rtree_node **n;
      for (n = &node->u.kids[0]; *n; n++)
        {
          if ((*n)->box.X1 >= query->X2 ||
              (*n)->box.X2 <= query->X1 ||
              (*n)->box.Y1 >= query->Y2 || (*n)->box.Y2 <= query->Y1 ||
              !arg->check_it (&(*n)->box, arg->closure))
            continue;
          seen += __r_search (*n, query, arg);
        }
      return seen;
    }
  else
    {
      int seen = 0;
      struct rtree_node **n;
      for (n = &node->u.kids[0]; *n; n++)
        {
          if ((*n)->box.X1 >= query->X2 ||
              (*n)->box.X2 <= query->X1 ||
              (*n)->box.Y1 >= query->Y2 || (*n)->box.Y2 <= query->Y1)
            continue;
          seen += __r_search (*n, query, arg);
        }
      return seen;
    }
}

/*!
 * \brief Parameterized search in the rtree.
 *
 * Calls found_rectangle for each intersection seen and calls
 * \c check_region with the current sub-region to see whether deeper
 * searching is desired.
 *
 * Generic search routine.
 *
 * \c region_in_search should return true if "what you're looking for"
 * is within the specified region; regions, like rectangles, are closed
 * on top and left and open on bottom and right.
 *
 * rectangle_in_region should return true if the given rectangle is
 * "what you're looking for".
 *
 * The search will find all rectangles matching the criteria given
 * by region_in_search and rectangle_in_region and return a count of
 * how many things rectangle_in_region returned true for.
 *
 * Closure is used to abort the search if desired from within
 * rectangel_in_region.
 *
 * Look at the implementation of r_region_is_empty for how to abort the
 * search if that is the desired behavior.
 *
 * \return the number of rectangles found.
 */
int
r_search (rtree_t * rtree, const BoxType * query,
          int (*check_region) (const BoxType * region, void *cl),
          int (*found_rectangle) (const BoxType * box, void *cl), void *cl)
{
  r_arg arg;

  if (!rtree || rtree->size < 1)
    return 0;
  if (query)
    {
#ifdef SLOW_ASSERTS
  assert (__r_tree_is_good (rtree->root));
#endif
#ifdef DEBUG
      if (query->X2 <= query->X1 || query->Y2 <= query->Y1)
        return 0;
#endif
      /* check this box. If it's not touched we're done here */
      if (rtree->root->box.X1 >= query->X2 ||
          rtree->root->box.X2 <= query->X1 ||
          rtree->root->box.Y1 >= query->Y2
          || rtree->root->box.Y2 <= query->Y1)
        return 0;
      arg.check_it = check_region;
      arg.found_it = found_rectangle;
      arg.closure = cl;
      return __r_search (rtree->root, query, &arg);
    }
  else
    {
      arg.check_it = check_region;
      arg.found_it = found_rectangle;
      arg.closure = cl;
      return __r_search (rtree->root, &rtree->root->box, &arg);
    }
}

/*!
 * \brief r_region_is_empty.
 */
static int
__r_region_is_empty_rect_in_reg (const BoxType * box, void *cl)
{
  jmp_buf *envp = (jmp_buf *) cl;
  longjmp (*envp, 1);           /* found one! */
}

/*!
 * \brief Special-purpose searches build upon r_search.
 *
 * \return 0 if there are any rectangles in the given region.
 */
int
r_region_is_empty (rtree_t * rtree, const BoxType * region)
{
  jmp_buf env;
#ifndef NDEBUG
  int r;
#endif

  if (setjmp (env))
    return 0;
#ifndef NDEBUG
  r =
#endif
    r_search (rtree, region, NULL, __r_region_is_empty_rect_in_reg, &env);
#ifndef NDEBUG
  assert (r == 0);              /* otherwise longjmp would have been called */
#endif
  return 1;                     /* no rectangles found */
}

struct centroid
{
  float x, y, area;
};

/*!
 * \brief Split the node into two nodes putting clusters in each use the
 * k-means clustering algorithm.
 */
struct rtree_node *
find_clusters (struct rtree_node *node)
{
  float total_a, total_b;
  float a_X, a_Y, b_X, b_Y;
  bool belong[M_SIZE + 1];
  struct centroid center[M_SIZE + 1];
  int clust_a, clust_b, tries;
  int a_manage = 0, b_manage = 0;
  int i, old_ax, old_ay, old_bx, old_by;
  struct rtree_node *new_node;
  BoxType *b;

  for (i = 0; i < M_SIZE + 1; i++)
    {
      if (node->flags.is_leaf)
        b = &(node->u.rects[i].bounds);
      else
        b = &(node->u.kids[i]->box);
      center[i].x = 0.5 * (b->X1 + b->X2);
      center[i].y = 0.5 * (b->Y1 + b->Y2);
      /* adding 1 prevents zero area */
      center[i].area = 1. + (float) (b->X2 - b->X1) * (float) (b->Y2 - b->Y1);
    }
  /* starting 'A' cluster center */
  a_X = center[0].x;
  a_Y = center[0].y;
  /* starting 'B' cluster center */
  b_X = center[M_SIZE].x;
  b_Y = center[M_SIZE].y;
  /* don't allow the same cluster centers */
  if (b_X == a_X && b_Y == a_Y)
    {
      b_X += 10000;
      a_Y -= 10000;
    }
  for (tries = 0; tries < M_SIZE; tries++)
    {
      old_ax = (int) a_X;
      old_ay = (int) a_Y;
      old_bx = (int) b_X;
      old_by = (int) b_Y;
      clust_a = clust_b = 0;
      for (i = 0; i < M_SIZE + 1; i++)
        {
          float dist1, dist2;

          dist1 = SQUARE (a_X - center[i].x) + SQUARE (a_Y - center[i].y);
          dist2 = SQUARE (b_X - center[i].x) + SQUARE (b_Y - center[i].y);
          if (dist1 * (clust_a + M_SIZE / 2) < dist2 * (clust_b + M_SIZE / 2))
            {
              belong[i] = true;
              clust_a++;
            }
          else
            {
              belong[i] = false;
              clust_b++;
            }
        }
      /* kludge to fix degenerate cases */
      if (clust_a == M_SIZE + 1)
        belong[M_SIZE / 2] = false;
      else if (clust_b == M_SIZE + 1)
        belong[M_SIZE / 2] = true;
      /* compute new center of gravity of clusters */
      total_a = total_b = 0;
      a_X = a_Y = b_X = b_Y = 0;
      for (i = 0; i < M_SIZE + 1; i++)
        {
          if (belong[i])
            {
              a_X += center[i].x * center[i].area;
              a_Y += center[i].y * center[i].area;
              total_a += center[i].area;
            }
          else
            {
              b_X += center[i].x * center[i].area;
              b_Y += center[i].y * center[i].area;
              total_b += center[i].area;
            }
        }
      a_X /= total_a;
      a_Y /= total_a;
      b_X /= total_b;
      b_Y /= total_b;
      if (old_ax == (int) a_X && old_ay == (int) a_Y &&
          old_bx == (int) b_X && old_by == (int) b_Y)
        break;
    }
  /* Now 'belong' has the partition map */
  new_node = (struct rtree_node *)calloc (1, sizeof (*new_node));
  new_node->parent = node->parent;
  new_node->flags.is_leaf = node->flags.is_leaf;
  clust_a = clust_b = 0;
  if (node->flags.is_leaf)
    {
      int flag, a_flag, b_flag;
      flag = a_flag = b_flag = 1;
      for (i = 0; i < M_SIZE + 1; i++)
        {
          if (belong[i])
            {
              node->u.rects[clust_a++] = node->u.rects[i];
              if (node->flags.manage & flag)
                a_manage |= a_flag;
              a_flag <<= 1;
            }
          else
            {
              new_node->u.rects[clust_b++] = node->u.rects[i];
              if (node->flags.manage & flag)
                b_manage |= b_flag;
              b_flag <<= 1;
            }
          flag <<= 1;
        }
    }
  else
    {
      for (i = 0; i < M_SIZE + 1; i++)
        {
          if (belong[i])
            node->u.kids[clust_a++] = node->u.kids[i];
          else
            {
              node->u.kids[i]->parent = new_node;
              new_node->u.kids[clust_b++] = node->u.kids[i];
            }
        }
    }
  node->flags.manage = a_manage;
  new_node->flags.manage = b_manage;
  assert (clust_a != 0);
  assert (clust_b != 0);
  if (node->flags.is_leaf)
    for (; clust_a < M_SIZE + 1; clust_a++)
      node->u.rects[clust_a].bptr = NULL;
  else
    for (; clust_a < M_SIZE + 1; clust_a++)
      node->u.kids[clust_a] = NULL;
  adjust_bounds (node);
  sort_node (node);
  adjust_bounds (new_node);
  sort_node (new_node);
  return (new_node);
}

/*!
 * \brief Split a node according to clusters.
 */
static void
split_node (struct rtree_node *node)
{
  int i;
  struct rtree_node *new_node;

  assert (node);
  assert (node->flags.is_leaf ? (void *) node->u.rects[M_SIZE].
          bptr : (void *) node->u.kids[M_SIZE]);
  new_node = find_clusters (node);
  if (node->parent == NULL)     /* split root node */
    {
      struct rtree_node *second;

      second = (struct rtree_node *)calloc (1, sizeof (*second));
      *second = *node;
      if (!second->flags.is_leaf)
        for (i = 0; i < M_SIZE; i++)
          if (second->u.kids[i])
            second->u.kids[i]->parent = second;
      node->flags.is_leaf = 0;
      node->flags.manage = 0;
      second->parent = new_node->parent = node;
      node->u.kids[0] = new_node;
      node->u.kids[1] = second;
      for (i = 2; i < M_SIZE + 1; i++)
        node->u.kids[i] = NULL;
      adjust_bounds (node);
      sort_node (node);
#ifdef SLOW_ASSERTS
      assert (__r_tree_is_good (node));
#endif
      return;
    }
  for (i = 0; i < M_SIZE; i++)
    if (!node->parent->u.kids[i])
      break;
  node->parent->u.kids[i] = new_node;
#ifdef SLOW_ASSERTS
  assert (__r_node_is_good (node));
  assert (__r_node_is_good (new_node));
#endif
  if (i < M_SIZE)
    {
#ifdef SLOW_ASSERTS
      assert (__r_node_is_good (node->parent));
#endif
      sort_node (node->parent);
      return;
    }
  split_node (node->parent);
}

static inline int
contained (struct rtree_node *node, const BoxType * query)
{
  if (node->box.X1 > query->X1 || node->box.X2 < query->X2 ||
      node->box.Y1 > query->Y1 || node->box.Y2 < query->Y2)
    return 0;
  return 1;
}


/*!
 * \brief Compute the area penalty for inserting here and return.
 *
 * The penalty is the increase in area necessary to include the query.
 */
static inline double
penalty (struct rtree_node *node, const BoxType * query)
{
  double score;

  score  = (MAX (node->box.X2, query->X2) - MIN (node->box.X1, query->X1));
  score *= (MAX (node->box.Y2, query->Y2) - MIN (node->box.Y1, query->Y1));
  score -=
    (double)(node->box.X2 - node->box.X1) *
    (double)(node->box.Y2 - node->box.Y1);
  return score;
}

static void
__r_insert_node (struct rtree_node *node, const BoxType * query,
                 int manage, bool force)
{

#ifdef SLOW_ASSERTS
  assert (__r_node_is_good (node));
#endif
  /* Ok, at this point we must already enclose the query or we're forcing
   * this node to expand to enclose it, so if we're a leaf, simply store
   * the query here
   */

  if (node->flags.is_leaf)
    {
      register int i;

      if (UNLIKELY (manage))
        {
          register int flag = 1;

          for (i = 0; i < M_SIZE; i++)
            {
              if (!node->u.rects[i].bptr)
                break;
              flag <<= 1;
            }
          node->flags.manage |= flag;
        }
      else
        {
          for (i = 0; i < M_SIZE; i++)
            if (!node->u.rects[i].bptr)
              break;
        }
      /* the node always has an extra space available */
      node->u.rects[i].bptr = query;
      node->u.rects[i].bounds = *query;
      /* first entry in node determines initial bounding box */
      if (i == 0)
        node->box = *query;
      else if (force)
        {
          MAKEMIN (node->box.X1, query->X1);
          MAKEMAX (node->box.X2, query->X2);
          MAKEMIN (node->box.Y1, query->Y1);
          MAKEMAX (node->box.Y2, query->Y2);
        }
      if (i < M_SIZE)
        {
          sort_node (node);
          return;
        }
      /* we must split the node */
      split_node (node);
      return;
    }
  else
    {
      int i;
      struct rtree_node *best_node;
      double score, best_score;

      if (force)
        {
          MAKEMIN (node->box.X1, query->X1);
          MAKEMAX (node->box.X2, query->X2);
          MAKEMIN (node->box.Y1, query->Y1);
          MAKEMAX (node->box.Y2, query->Y2);
        }

      /* this node encloses it, but it's not a leaf, so descend the tree */

      /* First check if any children actually encloses it */
      assert (node->u.kids[0]);
      for (i = 0; i < M_SIZE; i++)
        {
          if (!node->u.kids[i])
            break;
          if (contained (node->u.kids[i], query))
            {
              __r_insert_node (node->u.kids[i], query, manage, false);
              sort_node (node);
              return;
            }
        }

      /* see if there is room for a new leaf node */
      if (node->u.kids[0]->flags.is_leaf && i < M_SIZE)
        {
          struct rtree_node *new_node;
          new_node = (struct rtree_node *)calloc (1, sizeof (*new_node));
          new_node->parent = node;
          new_node->flags.is_leaf = true;
          node->u.kids[i] = new_node;
          new_node->u.rects[0].bptr = query;
          new_node->u.rects[0].bounds = *query;
          new_node->box = *query;
          if (UNLIKELY (manage))
            new_node->flags.manage = 1;
          sort_node (node);
          return;
        }

      /* Ok, so we're still here - look for the best child to push it into */
      best_score = penalty (node->u.kids[0], query);
      best_node = node->u.kids[0];
      for (i = 1; i < M_SIZE; i++)
        {
          if (!node->u.kids[i])
            break;
          score = penalty (node->u.kids[i], query);
          if (score < best_score)
            {
              best_score = score;
              best_node = node->u.kids[i];
            }
        }
      __r_insert_node (best_node, query, manage, true);
      sort_node (node);
      return;
    }
}

void
r_insert_entry (rtree_t * rtree, const BoxType * which, int man)
{
  assert (which);
  assert (which->X1 <= which->X2);
  assert (which->Y1 <= which->Y2);
  /* recursively search the tree for the best leaf node */
  assert (rtree->root);
  __r_insert_node (rtree->root, which, man,
                   rtree->root->box.X1 > which->X1
                   || rtree->root->box.X2 < which->X2
                   || rtree->root->box.Y1 > which->Y1
                   || rtree->root->box.Y2 < which->Y2);
  rtree->size++;
}

bool
__r_delete (struct rtree_node *node, const BoxType * query)
{
  int i, flag, mask, a;

  /* the tree might be inconsistent during delete */
  if (query->X1 < node->box.X1 || query->Y1 < node->box.Y1
      || query->X2 > node->box.X2 || query->Y2 > node->box.Y2)
    return false;
  if (!node->flags.is_leaf)
    {
      for (i = 0; i < M_SIZE; i++)
        {
          /* if this is us being removed, free and copy over */
          if (node->u.kids[i] == (struct rtree_node *) query)
            {
              free ((void *) query);
              for (; i < M_SIZE; i++)
                {
                  node->u.kids[i] = node->u.kids[i + 1];
                  if (!node->u.kids[i])
                    break;
                }
              /* nobody home here now ? */
              if (!node->u.kids[0])
                {
                  if (!node->parent)
                    {
                      /* wow, the root is empty! */
                      node->flags.is_leaf = 1;
                      /* changing type of node, be sure it's all zero */
                      for (i = 1; i < M_SIZE + 1; i++)
                        node->u.rects[i].bptr = NULL;
                      return true;
                    }
                  return (__r_delete (node->parent, &node->box));
                }
              else
                /* propegate boundary adjust upward */
                while (node)
                  {
                    adjust_bounds (node);
                    node = node->parent;
                  }
              return true;
            }
          if (node->u.kids[i])
            {
              if (__r_delete (node->u.kids[i], query))
                return true;
            }
          else
            break;
        }
      return false;
    }
  /* leaf node here */
  mask = 0;
  a = 1;
  for (i = 0; i < M_SIZE; i++)
    {
#ifdef DELETE_BY_POINTER
      if (!node->u.rects[i].bptr || node->u.rects[i].bptr == query)
#else
      if (node->u.rects[i].bounds.X1 == query->X1 &&
          node->u.rects[i].bounds.X2 == query->X2 &&
          node->u.rects[i].bounds.Y1 == query->Y1 &&
          node->u.rects[i].bounds.Y2 == query->Y2)
#endif
        break;
      mask |= a;
      a <<= 1;
    }
  if (!node->u.rects[i].bptr)
    return false;               /* not at this leaf */
  if (node->flags.manage & a)
    {
      free ((void *) node->u.rects[i].bptr);
      node->u.rects[i].bptr = NULL;
    }
  /* squeeze the manage flags together */
  flag = node->flags.manage & mask;
  mask = (~mask) << 1;
  node->flags.manage = flag | ((node->flags.manage & mask) >> 1);
  /* remove the entry */
  for (; i < M_SIZE; i++)
    {
      node->u.rects[i] = node->u.rects[i + 1];
      if (!node->u.rects[i].bptr)
        break;
    }
  if (!node->u.rects[0].bptr)
    {
      if (node->parent)
        __r_delete (node->parent, &node->box);
      return true;
    }
  else
    /* propagate boundary adjustment upward */
    while (node)
      {
        adjust_bounds (node);
        node = node->parent;
      }
  return true;
}

bool
r_delete_entry (rtree_t * rtree, const BoxType * box)
{
  bool r;

  assert (box);
  assert (rtree);
  r = __r_delete (rtree->root, box);
  if (r)
    rtree->size--;
#ifdef SLOW_ASSERTS
  assert (__r_tree_is_good (rtree->root));
#endif
  return r;
}
