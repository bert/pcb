/* $Id$ */

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

/* this file, mtspace.c, was written and is
 * Copyright (c) 2001 C. Scott Ananian.
 */

/* implementation for "empty space" routines (needed for via-space tracking
 * in the auto-router.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"

#include <assert.h>
#include <setjmp.h>

#include "box.h"
#include "rtree.h"
#include "mtspace.h"
#include "vector.h"


RCSID("$Id$");


/* define this for more thorough self-checking of data structures */
#define SLOW_ASSERTIONS

/* mtspace data structures are built on r-trees. */

/* ---------------------------------------------------------------------------
 * some local types
 */
typedef struct mtspacebox
{
  const BoxType box;
  int fixed_count;
  int even_count;
  int odd_count;
  BDimension keepaway; /* the smallest keepaway around this box */
}
mtspacebox_t;

/* this is an mtspace_t */
struct mtspace
{
  /* r-tree keeping track of "empty" regions. */
  rtree_t *rtree;
  /* bounding box */
  BoxType bounds;
};

#ifndef NDEBUG
static int
__mtspace_box_is_good (mtspacebox_t * mtsb)
{
  int r = mtsb &&
    __box_is_good (&mtsb->box) &&
    mtsb->fixed_count >= 0 && mtsb->even_count >= 0 && mtsb->odd_count >= 0 &&
    mtsb->keepaway > 0 && 1;
  assert (r);
  return r;
}
static int
__mtspace_is_good (mtspace_t * mtspace)
{
  int r = mtspace && mtspace->rtree && __box_is_good (&mtspace->bounds) &&
    /* XXX: check that no boxed in mtspace tree overlap */
    1;
  assert (r);
  return r;
}
#endif /* !NDEBUG */

mtspacebox_t *
mtspace_create_box (const BoxType * box, int fixed, int even, int odd)
{
  mtspacebox_t *mtsb;
  assert (__box_is_good (box));
  mtsb = malloc(sizeof (*mtsb));
  *((BoxTypePtr) & mtsb->box) = *box;
  mtsb->fixed_count = fixed;
  mtsb->even_count = even;
  mtsb->odd_count = odd;
  mtsb->keepaway = MAX_COORD; /* impossibly large start */
  assert (__mtspace_box_is_good (mtsb));
  return mtsb;
}

/* create an "empty space" representation */
mtspace_t *
mtspace_create (const BoxType * bounds, BDimension keepaway)
{
  BoxType smaller_bounds;
  mtspacebox_t *mtsb;
  mtspace_t *mtspace;

  assert (bounds);
  assert (__box_is_good (bounds));
  /* create initial "empty region" */
  smaller_bounds = shrink_box (bounds, keepaway);
  mtsb = mtspace_create_box (&smaller_bounds, 0, 0, 0);
  mtsb->keepaway = keepaway;
  /* create mtspace data structure */
  mtspace = malloc(sizeof (*mtspace));
  mtspace->rtree = r_create_tree ((const BoxType **) &mtsb, 1, 1);
  mtspace->bounds = smaller_bounds;
  /* done! */
  assert (__mtspace_is_good (mtspace));
  return mtspace;
}

/* destroy an "empty space" representation. */
void
mtspace_destroy (mtspace_t ** mtspacep)
{
  assert (mtspacep && __mtspace_is_good (*mtspacep));
  r_destroy_tree (&(*mtspacep)->rtree);
  free (*mtspacep);
  *mtspacep = NULL;
}

struct coalesce_closure
{
  vector_t *add_vec;
  vector_t *remove_vec;
  /* these are for convenience */
  mtspacebox_t *mtsb;
  Boolean is_add;
  jmp_buf env;
};

#if 0
/* not used */
static int
boxtype (mtspacebox_t * mtsb)
{
  assert (__mtspace_box_is_good (mtsb));
  return
    ((mtsb->fixed_count > 0) ? 1 : 0) |
    ((mtsb->even_count > 0) ? 2 : 0) | ((mtsb->odd_count > 0) ? 4 : 0);
}
#endif

/* look at last element in add_vec to see if it can be coalesced with
 * adjacent rectangles.  If it can, add the adjacent rectangle to the
 * remove_vec and replace the item in add_vec with the coalesced rect.
 * then remove all in remove_vec and invoke again.  otherwise, we
 * add the rectangle in add_vec and invoke again, until add_vec is empty. */
static int
check_one (const BoxType * box, void *cl)
{
  struct coalesce_closure *cc = (struct coalesce_closure *) cl;
  mtspacebox_t *adj = (mtspacebox_t *) box, *nb;
  direction_t d;
  BoxType a, b, c;
  LocationType x1, x2;
  int i;
  float new_area, area_a, area_b;
  assert (__mtspace_box_is_good (cc->mtsb));
  assert (__mtspace_box_is_good (adj));
  /* nothing should intersect mtsb */
  assert (!box_intersect (&adj->box, &cc->mtsb->box));
  /* if the box types are different, we can't coalesce */
  if (cc->mtsb->fixed_count != adj->fixed_count)
    return 0;
  if (cc->mtsb->even_count != adj->even_count)
    return 0;
  if (cc->mtsb->odd_count != adj->odd_count)
    return 0;
  /* determine on which side (if any) adj is adjacent to cc->mtsb */
  if (0) /* this is just to justify the lines below */ ;
  else if (adj->box.X1 == cc->mtsb->box.X2)
    d = EAST;
  else if (adj->box.X2 == cc->mtsb->box.X1)
    d = WEST;
  else if (adj->box.Y1 == cc->mtsb->box.Y2)
    d = SOUTH;
  else if (adj->box.Y2 == cc->mtsb->box.Y1)
    d = NORTH;
  else
    return 0;			/* not adjacent */
  a = adj->box;
  b = cc->mtsb->box;
  ROTATEBOX_TO_NORTH (a, d);
  ROTATEBOX_TO_NORTH (b, d);
  assert (a.Y2 == b.Y1);
  x1 = MAX (a.X1, b.X1);
  x2 = MIN (a.X2, b.X2);
  assert (x1 <= x2);		/* overlap */
  /* area of new coalesced box is (b.Y2-a.Y1)*(x2-x1).  this must be
   * larger than area of either box a or box b for this to be a win */
  new_area = (float) (b.Y2 - a.Y1) * (float) (x2 - x1);
  area_a = (float) (a.Y2 - a.Y1) * (float) (a.X2 - a.X1);
  area_b = (float) (b.Y2 - b.Y1) * (float) (b.X2 - b.X1);
  if ((new_area <= area_a) || (new_area <= area_b))
    return 0;
  /* okay! coalesce these boxes! */
  /* add the adjacent rectangle to remove_vec... */
  vector_append (cc->remove_vec, adj);
  /* ...and add the split/coalesced rectangles to add_vec */
  for (i = 0; i < 5; i++)
    {				/* five possible rectangles (only 3 ever created) */
      switch (i)
	{
	default:
	  assert (0);
	case 0:
	  c = a;
	  c.X2 = x1;
	  break;		/* left top */
	case 1:
	  c = a;
	  c.X1 = x2;
	  break;		/* right top */
	case 2:
	  c = b;
	  c.X2 = x1;
	  break;		/* left bottom */
	case 3:
	  c = b;
	  c.X1 = x2;
	  break;		/* right bottom */
	case 4:
	  c.X1 = x1;
	  c.X2 = x2;
	  c.Y1 = a.Y1;
	  c.Y2 = b.Y2;
	  break;		/* coalesced box */
	}
      if (c.X1 < c.X2)
	{
	  ROTATEBOX_FROM_NORTH (c, d);
	  nb = mtspace_create_box
	    (&c, adj->fixed_count, adj->even_count, adj->odd_count);
	  nb->keepaway = MIN (adj->keepaway, cc->mtsb->keepaway);
	  vector_append (cc->add_vec, nb);
	}
    }
  /* done coalescing! */
  longjmp (cc->env, 1);		/* don't continue searching */
  return 1;			/* never reached! */
}
static void
mtspace_coalesce (mtspace_t * mtspace, struct coalesce_closure *cc)
{
  do
    {
      /* first, remove anything in remove_vec */
      while (!vector_is_empty (cc->remove_vec))
	{
	  mtspacebox_t *mtsb = (mtspacebox_t *)
	    vector_remove_last (cc->remove_vec);
	  r_delete_entry (mtspace->rtree, &mtsb->box);
	}
      assert (vector_is_empty (cc->remove_vec));
      assert (!vector_is_empty (cc->add_vec));
      cc->mtsb = (mtspacebox_t *) vector_remove_last (cc->add_vec);
      if (setjmp (cc->env) == 0)
	{
	  /* search region is one larger than mtsb on all sides */
	  BoxType region = bloat_box (&cc->mtsb->box, 1);
	  int r;
	  r = r_search (mtspace->rtree, &region, NULL, check_one, cc);
	  assert (r == 0);	/* otherwise we would have called 'longjmp' */
	  /* ----- didn't find anything to coalesce ----- */
#ifndef NDEBUG
	  r = r_region_is_empty (mtspace->rtree, &cc->mtsb->box);
          assert(r);
#endif
	  r_insert_entry (mtspace->rtree, &cc->mtsb->box, 1);
	}
      else
	{
	  /* ----- found something to coalesce and added it to add_vec ---- */
	  /* free old mtsb */
	  free (cc->mtsb);
	}
    }
  while (!vector_is_empty (cc->add_vec));
  assert (vector_is_empty (cc->add_vec));
  assert (vector_is_empty (cc->remove_vec));
  assert (__mtspace_is_good (mtspace));
}

/* for every box which intersects mtsb->box, split off a chunk with
 * altered counts.  Add original to remove_vec and add replacements to
 * add_vec */
static int
remove_one (const BoxType * box, void *cl)
{
  struct coalesce_closure *cc = (struct coalesce_closure *) cl;
  mtspacebox_t *ibox = (mtspacebox_t *) box, *nb;
  BoxType a, b;
  int i, j;
  assert (__mtspace_box_is_good (cc->mtsb));
  assert (__mtspace_box_is_good (ibox));
  /* some of the candidates won't intersect */
  if (!box_intersect (&ibox->box, &cc->mtsb->box))
    return 0;
  /* remove the intersecting box by adding it to remove_vec */
  vector_append (cc->remove_vec, ibox);
  /* there are nine possible split boxes to add */
  a = cc->mtsb->box;
  for (i = 0; i < 3; i++)
    {
      for (j = 0; j < 3; j++)
	{
	  b = ibox->box;
	  switch (i)
	    {
	    default:
	      assert (0);
	    case 0:
	      b.X2 = MIN (b.X2, a.X1);
	      break;		/* left */
	    case 1:
	      b.X1 = MAX (b.X1, a.X1);
	      b.X2 = MIN (b.X2, a.X2);
	      break;		/* center */
	    case 2:
	      b.X1 = MAX (b.X1, a.X2);
	      break;		/* right */
	    }
	  switch (j)
	    {
	    default:
	      assert (0);
	    case 0:
	      b.Y2 = MIN (b.Y2, a.Y1);
	      break;		/* top */
	    case 1:
	      b.Y1 = MAX (b.Y1, a.Y1);
	      b.Y2 = MIN (b.Y2, a.Y2);
	      break;		/* center */
	    case 2:
	      b.Y1 = MAX (b.Y1, a.Y2);
	      break;		/* bottom */
	    }
	  if (b.X1 < b.X2 && b.Y1 < b.Y2)
	    {
	      nb = mtspace_create_box
		(&b, ibox->fixed_count, ibox->even_count, ibox->odd_count);
	      nb->keepaway = MIN (nb->keepaway, cc->mtsb->keepaway);
	      if (i == 1 && j == 1)
		{		/* this is the intersecting piece */
		  assert (box_intersect (&nb->box, &cc->mtsb->box));
		  if (cc->is_add)
		    {
		      nb->fixed_count += cc->mtsb->fixed_count;
		      nb->even_count += cc->mtsb->even_count;
		      nb->odd_count += cc->mtsb->odd_count;
		    }
		  else
		    {
		      nb->fixed_count -= cc->mtsb->fixed_count;
		      nb->even_count -= cc->mtsb->even_count;
		      nb->odd_count -= cc->mtsb->odd_count;
		    }
		}
	      else
		assert (!box_intersect (&nb->box, &cc->mtsb->box));
	      vector_append (cc->add_vec, nb);
	    }
	}
    }
  return 1;
}
static int
mtspace_remove_chunk (mtspace_t * mtspace, struct coalesce_closure *cc)
{
  return r_search (mtspace->rtree, &cc->mtsb->box, NULL, remove_one, cc);
}

/* add/remove a chunk from the empty-space representation */
static void
mtspace_mutate (mtspace_t * mtspace,
		const BoxType * box, mtspace_type_t which,
		BDimension keepaway, Boolean is_add)
{
  struct coalesce_closure cc;
  BoxType bloated;
  assert (__mtspace_is_good (mtspace));
  assert (__box_is_good (box));
  assert (which == FIXED || which == ODD || which == EVEN);
  /* create coalesce_closure */
  cc.add_vec = vector_create ();
  cc.remove_vec = vector_create ();
  cc.is_add = is_add;
  /* clip to bounds */
  bloated = bloat_box (box, keepaway);
  bloated = clip_box (&bloated, &mtspace->bounds);
  assert (__box_is_good (&bloated));
  cc.mtsb = mtspace_create_box
    (&bloated, which == FIXED ? 1 : 0, which == EVEN ? 1 : 0,
     which == ODD ? 1 : 0);
  cc.mtsb->keepaway = keepaway;
  assert (boxtype (cc.mtsb) != 0);
  /* take a chunk out of anything which intersects our clipped bloated box */
  mtspace_remove_chunk (mtspace, &cc);
  free (cc.mtsb);
  /* coalesce adjacent chunks */
  mtspace_coalesce (mtspace, &cc);
  /* clean up coalesce_closure */
  vector_destroy (&cc.add_vec);
  vector_destroy (&cc.remove_vec);
  /* done! */
  return;
}

/* add a space-filler to the empty space representation.  */
void
mtspace_add (mtspace_t * mtspace, const BoxType * box, mtspace_type_t which,
             BDimension keepaway)
{
  mtspace_mutate (mtspace, box, which, keepaway, True);
}

/* remove a space-filler from the empty space representation. */
void
mtspace_remove (mtspace_t * mtspace,
		const BoxType * box, mtspace_type_t which, BDimension keepaway)
{
  mtspace_mutate (mtspace, box, which, keepaway, False);
}

struct query_closure
{
  const BoxType *region;
  vector_t *free_space_vec;
  vector_t *lo_conflict_space_vec;
  vector_t *hi_conflict_space_vec;
  BDimension radius, keepaway;
  Boolean is_odd;
};

static int
query_one (const BoxType * box, void *cl)
{
  struct query_closure *qc = (struct query_closure *) cl;
  mtspacebox_t *mtsb = (mtspacebox_t *) box;
  BDimension shrink = 0;
  BoxTypePtr shrunk;
  assert (box_intersect (qc->region, &mtsb->box));
  if (mtsb->fixed_count > 0)
    /* ignore fixed boxes */
    return 0;
  if (qc->keepaway > mtsb->keepaway)
    shrink = qc->keepaway - mtsb->keepaway;
  shrink += qc->radius;
  shrunk = (BoxType *) malloc(sizeof(*shrunk));
  *shrunk = shrink_box (box, shrink);
  if (shrunk->X2 <= shrunk->X1
      || shrunk->Y2 <= shrunk->Y1 ||
  !box_intersect (qc->region, shrunk))
    {
      free (shrunk);
      return 0;
    }
  else if ((qc->is_odd ? mtsb->odd_count : mtsb->even_count) > 0)
    {
      /* this is a hi_conflict */
      vector_append (qc->hi_conflict_space_vec, shrunk);
    }
  else if (mtsb->odd_count > 0 || mtsb->even_count > 0)
    {
      /* this is a lo_conflict */
      vector_append (qc->lo_conflict_space_vec, shrunk);
    }
  else
    {
      /* no conflict! */
      assert (boxtype (mtsb) == 0);
      vector_append (qc->free_space_vec, shrunk);
    }
  return 1;
}

/* returns all empty spaces in 'region' which may hold a feature with the
 * mtspace feature_radius with the specified minimum keepaway.  Completely
 * empty regions are appended to the free_space_vec vector; regions which
 * are filled by objects generated by the previous pass are appended to the
 * lo_conflict_space_vec vector, and regions which are filled by objects
 * generated during *this* pass are appended to the hi_conflict_space_vec
 * vector.  The current pass identity is given by 'is_odd'.  Regions which
 * are filled by fixed objects are not returned at all. */
void
mtspace_query_rect (mtspace_t * mtspace, const BoxType * region,
		    BDimension radius, BDimension keepaway,
		    vector_t * free_space_vec,
		    vector_t * lo_conflict_space_vec,
		    vector_t * hi_conflict_space_vec, Boolean is_odd)
{
  struct query_closure qc;
  /* pre-assertions */
  assert (__mtspace_is_good (mtspace));
  assert (__box_is_good (region));
  assert (free_space_vec && vector_is_empty (free_space_vec));
  assert (lo_conflict_space_vec && vector_is_empty (lo_conflict_space_vec));
  assert (hi_conflict_space_vec && vector_is_empty (hi_conflict_space_vec));
  /* initialize closure */
  qc.region = region;
  qc.free_space_vec = free_space_vec;
  qc.lo_conflict_space_vec = lo_conflict_space_vec;
  qc.hi_conflict_space_vec = hi_conflict_space_vec;
  qc.is_odd = is_odd;
  qc.keepaway = keepaway;
  qc.radius = radius;
  /* do the query */
  r_search (mtspace->rtree, region, NULL, query_one, &qc);
  /* post-assertions */
#ifndef NDEBUG
#ifdef SLOW_ASSERTIONS
  {				/* assert that all returned boxes are inside the given search region */
    int i, j;
    for (i = 0; i < 3; i++)
      {
	vector_t *v =
	  (i == 0) ? free_space_vec :
	  (i == 1) ? lo_conflict_space_vec : hi_conflict_space_vec;
	for (j = 0; j < vector_size (v); j++)
	  {
	    const BoxType *b = (const BoxType *) vector_element (v, j);
	    assert (__box_is_good (b));
	    assert (box_intersect (region, b));
	  }
      }
  }
#endif /* SLOW_ASSERTIONS */
#endif /* !NDEBUG */
}
