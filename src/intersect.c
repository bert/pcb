/*!
 * \file src/intersect.c
 *
 * \brief Rectangle intersection/union routines.
 *
 * \author this file, intersect.c, was written and is
 * Copyright (c) 2001 C. Scott Ananian.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 * Copyright (C) 1994,1995,1996 Thomas Nau
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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"

#include <assert.h>

#include "data.h"
#include "intersect.h"
#include "mymem.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static int compareleft (const void *ptr1, const void *ptr2);
static int compareright (const void *ptr1, const void *ptr2);
static int comparepos (const void *ptr1, const void *ptr2);
static int nextpwrof2 (int i);

/* ---------------------------------------------------------------------------
 * some local types
 */
typedef struct
{
  Coord left, right;
  int covered;
  Coord area;
}
SegmentTreeNode;

typedef struct
{
  SegmentTreeNode *nodes;
  int size;
}
SegmentTree;

typedef struct
{
  Coord *p;
  int size;
}
LocationList;

/*!
 * \brief Create a sorted list of unique y coords from a BoxList.
 */
static LocationList
createSortedYList (BoxListType *boxlist)
{
  LocationList yCoords;
  Coord last;
  int i, n;
  /* create sorted list of Y coordinates */
  yCoords.size = 2 * boxlist->BoxN;
  yCoords.p = (Coord *)calloc (yCoords.size, sizeof (*yCoords.p));
  for (i = 0; i < boxlist->BoxN; i++)
    {
      yCoords.p[2 * i] = boxlist->Box[i].Y1;
      yCoords.p[2 * i + 1] = boxlist->Box[i].Y2;
    }
  qsort (yCoords.p, yCoords.size, sizeof (*yCoords.p), comparepos);
  /* count uniq y coords */
  last = 0;
  for (n = 0, i = 0; i < yCoords.size; i++)
    if (i == 0 || yCoords.p[i] != last)
      yCoords.p[n++] = last = yCoords.p[i];
  yCoords.size = n;
  return yCoords;
}

/*!
 * \brief Create an empty segment tree from the given sorted list of
 * unique y coords.
 */
static SegmentTree
createSegmentTree (Coord * yCoords, int N)
{
  SegmentTree st;
  int i;
  /* size is twice the nearest larger power of 2 */
  st.size = 2 * nextpwrof2 (N);
  st.nodes = (SegmentTreeNode *)calloc (st.size, sizeof (*st.nodes));
  /* initialize the rightmost leaf node */
  st.nodes[st.size - 1].left = (N > 0) ? yCoords[--N] : 10;
  st.nodes[st.size - 1].right = st.nodes[st.size - 1].left + 1;
  /* initialize the rest of the leaf nodes */
  for (i = st.size - 2; i >= st.size / 2; i--)
    {
      st.nodes[i].right = st.nodes[i + 1].left;
      st.nodes[i].left = (N > 0) ? yCoords[--N] : st.nodes[i].right - 1;
    }
  /* initialize the internal nodes */
  for (; i > 0; i--)
    {                           /* node 0 is not used */
      st.nodes[i].right = st.nodes[2 * i + 1].right;
      st.nodes[i].left = st.nodes[2 * i].left;
    }
  /* done! */
  return st;
}

void
insertSegment (SegmentTree * st, int n, Coord Y1, Coord Y2)
{
  Coord discriminant;
  if (st->nodes[n].left >= Y1 && st->nodes[n].right <= Y2)
    {
      st->nodes[n].covered++;
    }
  else
    {
      assert (n < st->size / 2);
      discriminant = st->nodes[n * 2 + 1 /*right */ ].left;
      if (Y1 < discriminant)
        insertSegment (st, n * 2, Y1, Y2);
      if (discriminant < Y2)
        insertSegment (st, n * 2 + 1, Y1, Y2);
    }
  /* fixup area */
  st->nodes[n].area = (st->nodes[n].covered > 0) ?
    (st->nodes[n].right - st->nodes[n].left) :
    (n >= st->size / 2) ? 0 :
    st->nodes[n * 2].area + st->nodes[n * 2 + 1].area;
}

void
deleteSegment (SegmentTree * st, int n, Coord Y1, Coord Y2)
{
  Coord discriminant;
  if (st->nodes[n].left >= Y1 && st->nodes[n].right <= Y2)
    {
      assert (st->nodes[n].covered);
      --st->nodes[n].covered;
    }
  else
    {
      assert (n < st->size / 2);
      discriminant = st->nodes[n * 2 + 1 /*right */ ].left;
      if (Y1 < discriminant)
        deleteSegment (st, n * 2, Y1, Y2);
      if (discriminant < Y2)
        deleteSegment (st, n * 2 + 1, Y1, Y2);
    }
  /* fixup area */
  st->nodes[n].area = (st->nodes[n].covered > 0) ?
    (st->nodes[n].right - st->nodes[n].left) :
    (n >= st->size / 2) ? 0 :
    st->nodes[n * 2].area + st->nodes[n * 2 + 1].area;
}

/*!
 * \brief Compute the area of the intersection of the given rectangles.
 *
 * That is the area covered by more than one rectangle (counted twice if
 * the area is covered by three rectangles, three times if covered by
 * four rectangles, etc.).
 *
 * \note Runs in O(N ln N) time.
 */
double
ComputeIntersectionArea (BoxListType *boxlist)
{
  Cardinal i;
  double area = 0.0;
  /* first get the aggregate area. */
  for (i = 0; i < boxlist->BoxN; i++)
    area += (double) (boxlist->Box[i].X2 - boxlist->Box[i].X1) *
      (double) (boxlist->Box[i].Y2 - boxlist->Box[i].Y1);
  /* intersection area is aggregate - union. */
  return area * 0.0001 - ComputeUnionArea (boxlist);
}

/*!
 * \brief Compute the area of the union of the given rectangles.
 *
 * \note Runs in O(N ln N) time.
 */
double
ComputeUnionArea (BoxListType *boxlist)
{
  BoxType **rectLeft, **rectRight;
  Cardinal i, j;
  LocationList yCoords;
  SegmentTree segtree;
  Coord lastX;
  double area = 0.0;

  if (boxlist->BoxN == 0)
    return 0.0;
  /* create sorted list of Y coordinates */
  yCoords = createSortedYList (boxlist);
  /* now create empty segment tree */
  segtree = createSegmentTree (yCoords.p, yCoords.size);
  free (yCoords.p);
  /* create sorted list of left and right X coordinates of rectangles */
  rectLeft = (BoxType **)calloc (boxlist->BoxN, sizeof (*rectLeft));
  rectRight = (BoxType **)calloc (boxlist->BoxN, sizeof (*rectRight));
  for (i = 0; i < boxlist->BoxN; i++)
    {
      assert (boxlist->Box[i].X1 <= boxlist->Box[i].X2);
      assert (boxlist->Box[i].Y1 <= boxlist->Box[i].Y2);
      rectLeft[i] = rectRight[i] = &boxlist->Box[i];
    }
  qsort (rectLeft, boxlist->BoxN, sizeof (*rectLeft), compareleft);
  qsort (rectRight, boxlist->BoxN, sizeof (*rectRight), compareright);
  /* sweep through x segments from left to right */
  i = j = 0;
  lastX = rectLeft[0]->X1;
  while (j < boxlist->BoxN)
    {
      assert (i <= boxlist->BoxN);
      /* i will step through rectLeft, j will through rectRight */
      if (i == boxlist->BoxN || rectRight[j]->X2 < rectLeft[i]->X1)
        {
          /* right edge of rectangle */
          BoxType *b = rectRight[j++];
          /* check lastX */
          if (b->X2 != lastX)
            {
              assert (lastX < b->X2);
              area += (double) (b->X2 - lastX) * segtree.nodes[1].area;
              lastX = b->X2;
            }
          /* remove a segment from the segment tree. */
          deleteSegment (&segtree, 1, b->Y1, b->Y2);
        }
      else
        {
          /* left edge of rectangle */
          BoxType *b = rectLeft[i++];
          /* check lastX */
          if (b->X1 != lastX)
            {
              assert (lastX < b->X1);
              area += (double) (b->X1 - lastX) * segtree.nodes[1].area;
              lastX = b->X1;
            }
          /* add a segment from the segment tree. */
          insertSegment (&segtree, 1, b->Y1, b->Y2);
        }
    }
  free (rectLeft);
  free (rectRight);
  free (segtree.nodes);
  return area * 0.0001;
}

static int
compareleft (const void *ptr1, const void *ptr2)
{
  BoxType **b1 = (BoxType **) ptr1;
  BoxType **b2 = (BoxType **) ptr2;
  return (*b1)->X1 - (*b2)->X1;
}

static int
compareright (const void *ptr1, const void *ptr2)
{
  BoxType **b1 = (BoxType **) ptr1;
  BoxType **b2 = (BoxType **) ptr2;
  return (*b1)->X2 - (*b2)->X2;
}

static int
comparepos (const void *ptr1, const void *ptr2)
{
  return *((Coord *) ptr1) - *((Coord *) ptr2);
}

static int
nextpwrof2 (int n)
{
  int r = 1;
  while (n != 0)
    {
      n /= 2;
      r *= 2;
    }
  return r;
}
