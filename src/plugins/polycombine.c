/*!
 * \file polycombine.c
 *
 * \brief PolyCombine plug-in for PCB.
 *
 * \author Copyright (C) 2010 Peter Clifton <pcjc2@cam.ac.uk>
 *
 * \copyright Licensed under the terms of the GNU General Public
 * License, version 2.
 *
 * Compile like this:
 *
 * gcc -I$HOME/pcbsrc/git/src -I$HOME/pcbsrc/git -O2 -shared polycombine.c -o polycombine.so
 *
 * The resulting polycombine.so goes in $HOME/.pcb/plugins/polycombine.so.
 *
 * Usage: PolyCombine()
 *
 * The selected polygons are combined together according to the ordering
 * of their points.
 */

#include <stdio.h>
#include <math.h>

#include "config.h"
#include "global.h"
#include "data.h"
#include "macro.h"
#include "create.h"
#include "remove.h"
#include "hid.h"
#include "error.h"
#include "rtree.h"
#include "polygon.h"
#include "polyarea.h"
#include "assert.h"
#include "strflags.h"
#include "find.h"
#include "misc.h"
#include "draw.h"
#include "undo.h"

static POLYAREA *
original_poly (PolygonType * p, bool *forward)
{
  PLINE *contour = NULL;
  POLYAREA *np = NULL;
  Cardinal n;
  Vector v;
  int hole = 0;

  *forward = true;

  if ((np = poly_Create ()) == NULL)
    return NULL;

  /* first make initial polygon contour */
  for (n = 0; n < p->PointN; n++)
    {
      /* No current contour? Make a new one starting at point */
      /*   (or) Add point to existing contour */

      v[0] = p->Points[n].X;
      v[1] = p->Points[n].Y;
      if (contour == NULL)
        {
          if ((contour = poly_NewContour (v)) == NULL)
            return NULL;
        }
      else
        {
          poly_InclVertex (contour->head.prev, poly_CreateNode (v));
        }

      /* Is current point last in contour? If so process it. */
      if (n == p->PointN - 1 ||
          (hole < p->HoleIndexN && n == p->HoleIndex[hole] - 1))
        {
          poly_PreContour (contour, TRUE);

          /* Log the direction in which the outer contour was specified */
          if (hole == 0)
            *forward = (contour->Flags.orient == PLF_DIR);

          /* make sure it is a positive contour (outer) or negative (hole) */
          if (contour->Flags.orient != (hole ? PLF_INV : PLF_DIR))
            poly_InvContour (contour);
          assert (contour->Flags.orient == (hole ? PLF_INV : PLF_DIR));

          poly_InclContour (np, contour);
          contour = NULL;
          assert (poly_Valid (np));

          hole++;
        }
  }
  return np;
}

typedef struct poly_tree poly_tree;

struct poly_tree
{
  PolygonType *polygon;
  bool forward;
  POLYAREA *polyarea;
  poly_tree *parent;
  poly_tree *child;
  poly_tree *prev;
  poly_tree *next;
};

/*!
 * <pre>
 *                      ______
 *  ___________________|_  P6 |             +P1 ____ +P6
 * | P1                | |    |              |
 * |   _____      ____ |_|____|             -P2 ____ -P4 ____ -P5
 * |  |P2   |    |P5  |  |                   |
 * |  | []  |    |____|  |                  +P3
 * |  |  P3 |            |
 * |  |_____|            |
 * |                     |
 * |          ___        |
 * |         |P4 |       |
 * |         |___|       |
 * |                     |
 * |_____________________|
 *
 * </pre>
 * As we encounter each polygon, it gets a record. We need to check
 * whether it contains any of the polygons existing in our tree. If
 * it does, it will become the parent of them. (Check breadth first).
 *
 * When processing, work top down (breadth first), although if the
 * contours can be assumed not to overlap, we can drill down in this
 * order: P1, P2, P3, P4, P5, P6.
 */
static bool
PolygonContainsPolygon (POLYAREA *outer, POLYAREA *inner)
{
//  int contours_isect;
  /* Should check outer contours don't intersect? */
//  contours_isect = Touching (outer, inner);
  /* Cheat and assume simple single contour polygons for now */
//  return contours_isect ?
//           0 : poly_ContourInContour (outer->contours, inner->contours);
  return poly_ContourInContour (outer->contours, inner->contours);
}


static poly_tree *
insert_node_recursive (poly_tree *start_point, poly_tree *to_insert)
{
  poly_tree *cur_node, *next = NULL;
//  bool to_insert_isects_cur_node;   /* Intersection */
  bool to_insert_contains_cur_node; /* Containment */
  bool cur_node_contains_to_insert; /* Containment */
  bool placed_to_insert = false;

  poly_tree *return_root = start_point;

  if (start_point == NULL)
    {
//      printf ("start_point is NULL, so returning to_insert\n");
      //to_insert->parent = !!; UNDEFINED
      return to_insert;
    }

  /* Investigate the start point and its peers first */
  for (cur_node = start_point; cur_node != NULL; cur_node = next)
    {
      next = cur_node->next;

//      to_insert_isects_cur_node = IsPolygonInPolygon (to_insert->polygon, cur_node->polygon);
      to_insert_contains_cur_node = PolygonContainsPolygon (to_insert->polyarea, cur_node->polyarea);

#if 0
      printf ("Inspecting polygon %ld %s, curnode is %ld %s: to_insert_isects_cur_node = %d, to_insert_contains_cur_node = %i\n",
              to_insert->polygon->ID,
              to_insert->forward ? "FWD" : "BWD",
              cur_node->polygon->ID,
              cur_node->forward ? "FWD" : "BWD",
              to_insert_isects_cur_node,
              to_insert_contains_cur_node);

      if (to_insert_isects_cur_node) /* Place as peer of this node? */
        {
        }
#endif

      if (to_insert_contains_cur_node) /* Should be a parent of this node */
        {
          /* Remove cur_node from its peers */
          if (cur_node->prev)
            cur_node->prev->next = cur_node->next;
          if (cur_node->next)
            cur_node->next->prev = cur_node->prev;

          /* If we've not yet got a home, insert the to_insert node where cur_node was previously */
          if (!placed_to_insert)
            {
              to_insert->parent = cur_node->parent;
              to_insert->next = cur_node->next;
              to_insert->prev = cur_node->prev;
              if (to_insert->prev)
                to_insert->prev->next = to_insert;
              if (to_insert->next)
                to_insert->next->prev = to_insert;
              placed_to_insert = true;

              if (cur_node == start_point)
                return_root = to_insert;
            }

          /* Prepend cur_node to our list of children */
          cur_node->parent = to_insert;

          cur_node->prev = NULL;
          cur_node->next = to_insert->child;
          if (to_insert->child)
            to_insert->child->prev = cur_node;
          to_insert->child = cur_node;

        }
    }

  if (placed_to_insert)
    {
//      printf ("Returning new root %ld\n", return_root->polygon->ID);
      return return_root;
    }
//    return (to_insert->parent == NULL) ? to_insert : to_insert->parent;

  /* Ok, so we still didn't find anywhere which the to_insert contour contained,
   * we need to start looking at the children of the start_point and its peers.
   */
//  printf ("Looking at child nodes of the start_point\n");

  /* Investigate the start point and its peers first */
  for (cur_node = start_point; cur_node != NULL; cur_node = next)
    {
      next = cur_node->next;

      cur_node_contains_to_insert = PolygonContainsPolygon (cur_node->polyarea, to_insert->polyarea);

#if 0
      printf ("Inspecting polygon %ld, curnode is %ld: cur_node_contains_to_insert = %d\n",
              to_insert->polygon->ID, cur_node->polygon->ID,
              cur_node_contains_to_insert);
#endif

      /* Need to look at the child, ONLY if we know it engulfs our to_insert polygon */
      if (cur_node_contains_to_insert)
        {
          /* Can't set the parent within the call, so: */
          to_insert->parent = cur_node;
          cur_node->child = insert_node_recursive (cur_node->child, to_insert);
          return start_point;
          //return cur_node->parent;
        }
    }

//  if (!placed_to_insert)
    /* prepend to_insert polygon to peer of start_point ? */
//  printf ("Prepending as peer to start_poly\n");
  to_insert->parent = start_point->parent;
  to_insert->prev = NULL;
  to_insert->next = start_point;
  to_insert->next->prev = to_insert;

  return to_insert;
}

static POLYAREA *
compute_polygon_recursive (poly_tree *root, POLYAREA *accumulate)
{
  POLYAREA *res;
  poly_tree *cur_node;
  for (cur_node = root; cur_node != NULL; cur_node = cur_node->next)
    {
      /* Process this element */
//      printf ("Processing node %ld %s\n", cur_node->polygon->ID, cur_node->forward ? "FWD" : "BWD");
      poly_Boolean_free (accumulate, cur_node->polyarea, &res, cur_node->forward ? PBO_UNITE : PBO_SUB);
      accumulate = res;

      /* And its children if it has them */
      if (cur_node->child)
        {
//          printf ("Processing children\n");
          accumulate = compute_polygon_recursive (cur_node->child, accumulate);
        }
    }
  return accumulate;
}

static int
polycombine (int argc, char **argv, Coord x, Coord y)
{
  POLYAREA *res;
  bool forward;
//  bool outer;
  POLYAREA *np;
//  POLYAREA *pa;
//  PLINE *pline;
//  VNODE *node;
//  PolygonType *Polygon;
  LayerType *Layer = NULL;
  poly_tree *root = NULL;
  poly_tree *this_node;

  /* First pass to combine the forward and backward contours */
  VISIBLEPOLYGON_LOOP (PCB->Data);
  {
    if (!TEST_FLAG (SELECTEDFLAG, polygon))
      continue;

    /* Pick the layer of the first polygon we find selected */
    if (Layer == NULL)
      Layer = layer;

    /* Only combine polygons on the same layer */
    if (Layer != layer)
      continue;

    np = original_poly (polygon, &forward);

    /* Build a poly_tree record */
    this_node = calloc (1, sizeof (poly_tree));
    this_node->polygon = polygon;
    this_node->forward = forward;
    this_node->polyarea = np;

    /* Check where we should place the node in the tree */
    root = insert_node_recursive (root, this_node);

    //RemovePolygon (layer, polygon);
  }
  ENDALL_LOOP;

  /* Now perform a traversal of the tree, computing a polygon */
  res = compute_polygon_recursive (root, NULL);

  SaveUndoSerialNumber ();

  /* Second pass to remove the input polygons */
  VISIBLEPOLYGON_LOOP (PCB->Data);
  {
    if (!TEST_FLAG (SELECTEDFLAG, polygon))
      continue;

    /* Only combine polygons on the same layer */
    if (Layer != layer)
      continue;

    RemovePolygon (layer, polygon);
  }
  ENDALL_LOOP;

  /* Now de-construct the resulting polygon into raw PCB polygons */
  PolyToPolygonsOnLayer (PCB->Data, Layer, res,
                         string_to_pcbflags ("clearpoly", NULL));
  RestoreUndoSerialNumber ();
  IncrementUndoSerialNumber ();
  Draw ();

  return 0;
}

static HID_Action polycombine_action_list[] = {
  {"PolyCombine", "???", polycombine,
   NULL, NULL}
};

REGISTER_ACTIONS (polycombine_action_list)

void
pcb_plugin_init()
{
  register_polycombine_action_list();
}
