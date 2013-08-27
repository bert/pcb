/*!
 * \file distaligntext.c
 *
 * \brief distaligntext plug-in for PCB.
 *
 * \author Copyright (C) 2012 Dan White <dan@whiteaudio.com>
 * Functions to distribute (evenly spread out) and align PCB text.
 *
 * \copyright Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Substantially from distalign.c
 * Copyright (C) 2007 Ben Jackson <ben@ben.com>
 *
 * Modifications and internal differences are significant enough warrant
 * a new related plugin.
 */

#include <stdio.h>
#include <math.h>

#include "config.h"
#include "global.h"
#include "data.h"
#include "hid.h"
#include "misc.h"
#include "create.h"
#include "rtree.h"
#include "undo.h"
#include "rats.h"
#include "error.h"
#include "move.h"
#include "draw.h"
#include "set.h"

#define ARG(n) (argc > (n) ? argv[n] : 0)

static const char aligntext_syntax[] = "AlignText(X/Y, [Lefts/Rights/Tops/Bottoms/Centers, [First/Last/Crosshair/Average[, Gridless]]])";

static const char distributetext_syntax[] = "DistributeText(Y, [Lefts/Rights/Tops/Bottoms/Centers/Gaps, [First/Last/Crosshair, First/Last/Crosshair[, Gridless]]])";

enum
{
  K_X,
  K_Y,
  K_Lefts,
  K_Rights,
  K_Tops,
  K_Bottoms,
  K_Centers,
  K_Gaps,
  K_First,
  K_Last,
  K_Average,
  K_Crosshair,
  K_Gridless,
  K_none,
  K_aligntext,
  K_distributetext
};

static const char *keywords[] =
{
  [K_X] "X",
  [K_Y] "Y",
  [K_Lefts] "Lefts",
  [K_Rights] "Rights",
  [K_Tops] "Tops",
  [K_Bottoms] "Bottoms",
  [K_Centers] "Centers",
  [K_Gaps] "Gaps",
  [K_First] "First",
  [K_Last] "Last",
  [K_Average] "Average",
  [K_Crosshair] "Crosshair",
  [K_Gridless] "Gridless",
};

static int
keyword(const char *s)
{
  int i;

  if (! s)
  {
    return K_none;
  }
  for (i = 0; i < ENTRIES(keywords); ++i)
  {
    if (keywords[i] && strcasecmp(s, keywords[i]) == 0)
      return i;
  }
  return -1;
}


/* this macro produces a function in X or Y that switches on 'point' */
#define COORD(DIR)						\
static inline Coord				        	\
coord ## DIR(TextType *text, int point)		        	\
{								\
	switch (point) {					\
	case K_Lefts:						\
	case K_Tops:						\
		return text->BoundingBox.DIR ## 1;		\
	case K_Rights:						\
	case K_Bottoms:						\
		return text->BoundingBox.DIR ## 2;		\
	case K_Centers:						\
	case K_Gaps:						\
		return (text->BoundingBox.DIR ## 1 +		\
		       text->BoundingBox.DIR ## 2) / 2;	        \
	}							\
	return 0;						\
}

COORD(X)
COORD(Y)

/*!
 * Return the text coordinate associated with the given internal point.
 */
static Coord
coord (TextType *text, int dir, int point)
{
  if (dir == K_X)
    return coordX (text, point);
  else
    return coordY (text, point);
}

static struct text_by_pos
{
  TextType *text;
  Coord pos;
  Coord width;
  int type;
} *texts_by_pos;

static int ntexts_by_pos;

static int
cmp_tbp (const void *a, const void *b)
{
  const struct text_by_pos *ta = a;
  const struct text_by_pos *tb = b;

  return ta->pos - tb->pos;
}

/*!
 * Find all selected text objects, then order them in order by coordinate in
 * the 'dir' axis.  This is used to find the "First" and "Last" elements
 * and also to choose the distribution order.
 *
 * For alignment, first and last are in the orthogonal axis (imagine if
 * you were lining up letters in a sentence, aligning *vertically* to the
 * first letter means selecting the first letter *horizontally*).
 *
 * For distribution, first and last are in the distribution axis.
 */
static int
sort_texts_by_pos (int op, int dir, int point)
{
  int nsel = 0;

  if (ntexts_by_pos)
    return ntexts_by_pos;
  if (op == K_aligntext)
    dir = dir == K_X ? K_Y : K_X; /* see above */
  ELEMENT_LOOP (PCB->Data);
  {
    TextType *text;
    text = &(element)->Name[NAME_INDEX(PCB)];
    if (!TEST_FLAG (SELECTEDFLAG, text))
      continue;
    nsel++;
  }
  END_LOOP;
  ALLTEXT_LOOP (PCB->Data);
  {
    if (!TEST_FLAG (SELECTEDFLAG, text))
      continue;
    nsel++;
  }
  ENDALL_LOOP;
  if (! nsel)
    return 0;
  texts_by_pos = malloc (nsel * sizeof (*texts_by_pos));
  ntexts_by_pos = nsel;
  nsel = 0;
  ELEMENT_LOOP(PCB->Data);
  {
    TextType *text;
    text = &(element)->Name[NAME_INDEX(PCB)];
    if (!TEST_FLAG (SELECTEDFLAG, text))
      continue;
    texts_by_pos[nsel].text = text;
    texts_by_pos[nsel].type = ELEMENTNAME_TYPE;
    texts_by_pos[nsel++].pos = coord(text, dir, point);
  }
  END_LOOP;
  ALLTEXT_LOOP (PCB->Data);
  {
    if (!TEST_FLAG (SELECTEDFLAG, text))
      continue;
    texts_by_pos[nsel].text = text;
    texts_by_pos[nsel].type = TEXT_TYPE;
    texts_by_pos[nsel++].pos = coord(text, dir, point);
  }
  ENDALL_LOOP;
  qsort (texts_by_pos, ntexts_by_pos, sizeof (*texts_by_pos), cmp_tbp);
  return ntexts_by_pos;
}

static void
free_texts_by_pos (void)
{
  if (ntexts_by_pos)
  {
    free (texts_by_pos);
    texts_by_pos = NULL;
    ntexts_by_pos = 0;
  }
}


/*!
 * Find the reference coordinate from the specified points of all
 * selected text.
 */
static Coord
reference_coord (int op, int x, int y, int dir, int point, int reference)
{
  Coord q;
  int i, nsel;

  q = 0;
  switch (reference)
  {
    case K_Crosshair:
      if (dir == K_X)
        q = x;
      else
        q = y;
      break;
    case K_Average: /* the average among selected text */
      nsel = ntexts_by_pos;
      for (i = 0; i < ntexts_by_pos; ++i)
      {
        q += coord (texts_by_pos[i].text, dir, point);
      }
      if (nsel)
        q /= nsel;
      break;
    case K_First: /* first or last in the orthogonal direction */
    case K_Last:
      if (! sort_texts_by_pos (op, dir, point))
      {
        q = 0;
        break;
      }
      if (reference == K_First)
      {
        q = coord (texts_by_pos[0].text, dir, point);
      }
      else
      {
        q = coord (texts_by_pos[ntexts_by_pos-1].text, dir, point);
      }
      break;
  }
  return q;
}


/*!
 * AlignText(X, [Lefts/Rights/Centers, [First/Last/Crosshair/Average[, Gridless]]])\n
 * AlignText(Y, [Tops/Bottoms/Centers, [First/Last/Crosshair/Average[, Gridless]]])
 *
 * X or Y - Select which axis will move, other is untouched. \n
 * Lefts, Rights, \n
 * Tops, Bottoms, \n
 * Centers - Pick alignment point within each element. \n
 * NB: text objects have no Mark. \n
 * First, Last, \n
 * Crosshair, \n
 * Average - Alignment reference, First=Topmost/Leftmost, \n
 * Last=Bottommost/Rightmost, Average or Crosshair point \n
 * Gridless - Do not force results to align to prevailing grid. \n
 *
 * Defaults are Lefts/Tops, First
 */
static int
aligntext (int argc, char **argv, Coord x, Coord y)
{
  int dir;
  int point;
  int reference;
  int gridless;
  Coord q;
  Coord p, dp, dx, dy;
  int changed = 0;

  if (argc < 1 || argc > 4)
  {
    AFAIL (aligntext);
  }
  /* parse direction arg */
  switch ((dir = keyword(ARG(0))))
  {
    case K_X:
    case K_Y:
      break;
    default:
      AFAIL (aligntext);
  }
  /* parse point (within each element) which will be aligned */
  switch ((point = keyword(ARG(1))))
  {
    case K_Centers:
      break;
    case K_Lefts:
    case K_Rights:
      if (dir == K_Y)
      {
        AFAIL (aligntext);
      }
      break;
    case K_Tops:
    case K_Bottoms:
      if (dir == K_X)
      {
        AFAIL (aligntext);
      }
      break;
    case K_none: /* default value */
      if (dir == K_X)
      {
        point = K_Lefts;
      }
      else
      {
        point = K_Tops;
      }
      break;
    default:
      AFAIL (aligntext);
  }
  /* parse reference which will determine alignment coordinates */
  switch ((reference = keyword(ARG(2))))
  {
    case K_First:
    case K_Last:
    case K_Average:
    case K_Crosshair:
      break;
    case K_none:
      reference = K_First; /* default value */
      break;
    default:
      AFAIL (aligntext);
  }
  /* optionally work off the grid (solar cells!) */
  switch (keyword(ARG(3)))
  {
    case K_Gridless:
      gridless = 1;
      break;
    case K_none:
      gridless = 0;
      break;
    default:
      AFAIL (aligntext);
  }
  SaveUndoSerialNumber();
  /* find the final alignment coordinate using the above options */
  q = reference_coord (K_aligntext, Crosshair.X, Crosshair.Y, dir, point, reference);
  /* move all selected elements to the new coordinate */
  /* selected text part of an element */
  ELEMENT_LOOP (PCB->Data);
  {
    TextType *text;
    text = &(element)->Name[NAME_INDEX (PCB)];
    if (!TEST_FLAG (SELECTEDFLAG, text))
      continue;
    /* find delta from reference point to reference point */
    p = coord (text, dir, point);
    dp = q - p;
    /* ...but if we're gridful, keep the mark on the grid */
    /* TODO re-enable for text, need textcoord()
    if (!gridless)
    {
      dp -= (coord (text, dir, K_Marks) + dp) % (long) (PCB->Grid);
    }
     */
    if (dp)
    {
      /* move from generic to X or Y */
      dx = dy = dp;
      if (dir == K_X)
        dy = 0;
      else
        dx = 0;
      MoveObject (ELEMENTNAME_TYPE, element, text, text, dx, dy);
      changed = 1;
    }
  }
  END_LOOP;
  /* Selected bare text objects */
  ALLTEXT_LOOP (PCB->Data);
  {
    if (TEST_FLAG (SELECTEDFLAG, text))
    {
      /* find delta from reference point to reference point */
      p = coord (text, dir, point);
      dp = q - p;
      /* ...but if we're gridful, keep the mark on the grid */
      /* TODO re-enable for text, need textcoord()
      if (!gridless)
      {
        dp -= (coord (text, dir, K_Marks) + dp) % (long) (PCB->Grid);
      }
       */
      if (dp)
      {
        /* move from generic to X or Y */
        dx = dy = dp;
        if (dir == K_X)
          dy = 0;
        else
          dx = 0;
        MoveObject (TEXT_TYPE, layer, text, text, dx, dy);
        changed = 1;
      }
    }
  }
  ENDALL_LOOP;
  if (changed)
  {
    RestoreUndoSerialNumber ();
    IncrementUndoSerialNumber ();
    Redraw ();
    SetChangedFlag (true);
  }
  free_texts_by_pos ();
  return 0;
}

/*!
 * DistributeText(X, [Lefts/Rights/Centers/Gaps, [First/Last/Crosshair, First/Last/Crosshair[, Gridless]]]) \n
 * DistributeText(Y, [Tops/Bottoms/Centers/Gaps, [First/Last/Crosshair, First/Last/Crosshair[, Gridless]]]) \n
 * \n
 * As with align, plus: \n
 * \n
 * Gaps - Make gaps even rather than spreading points evenly. \n
 * First, Last, \n
 * Crosshair - Two arguments specifying both ends of the distribution,
 * they can't both be the same. \n
 * \n
 * Defaults are Lefts/Tops, First, Last \n
 * \n
 * Distributed texts always retain the same relative order they had
 * before they were distributed. \n
 */
static int
distributetext(int argc, char **argv, Coord x, Coord y)
{
  int dir;
  int point;
  int refa, refb;
  int gridless;
  Coord s, e, slack;
  int divisor;
  int changed = 0;
  int i;

  if (argc < 1 || argc == 3 || argc > 4)
  {
    AFAIL (distributetext);
  }
  /* parse direction arg */
  switch ((dir = keyword(ARG(0))))
  {
    case K_X:
    case K_Y:
      break;
    default:
      AFAIL (distributetext);
  }
  /* parse point (within each element) which will be distributed */
  switch ((point = keyword(ARG(1))))
  {
    case K_Centers:
    case K_Gaps:
      break;
    case K_Lefts:
    case K_Rights:
      if (dir == K_Y)
      {
        AFAIL (distributetext);
      }
      break;
    case K_Tops:
    case K_Bottoms:
      if (dir == K_X)
      {
        AFAIL (distributetext);
      }
      break;
    case K_none: /* default value */
      if (dir == K_X)
      {
        point = K_Lefts;
      }
      else
      {
        point = K_Tops;
      }
      break;
    default:
      AFAIL (distributetext);
  }
  /* parse reference which will determine first distribution coordinate */
  switch ((refa = keyword(ARG(2))))
  {
    case K_First:
    case K_Last:
    case K_Average:
    case K_Crosshair:
      break;
    case K_none:
      refa = K_First; /* default value */
      break;
    default:
      AFAIL (distributetext);
  }
  /* parse reference which will determine final distribution coordinate */
  switch ((refb = keyword(ARG(3))))
  {
    case K_First:
    case K_Last:
    case K_Average:
    case K_Crosshair:
      break;
    case K_none:
      refb = K_Last; /* default value */
      break;
    default:
      AFAIL (distributetext);
  }
  if (refa == refb)
  {
    AFAIL (distributetext);
  }
  /* optionally work off the grid (solar cells!) */
  switch (keyword(ARG(4)))
  {
    case K_Gridless:
      gridless = 1;
      break;
    case K_none:
      gridless = 0;
      break;
    default:
      AFAIL (distributetext);
  }
  SaveUndoSerialNumber ();
  /* build list of texts in orthogonal axis order */
  sort_texts_by_pos (K_distributetext, dir, point);
  /* find the endpoints given the above options */
  s = reference_coord (K_distributetext, x, y, dir, point, refa);
  e = reference_coord (K_distributetext, x, y, dir, point, refb);
  slack = e - s;
  /* use this divisor to calculate spacing (for 1 elt, avoid 1/0) */
  divisor = (ntexts_by_pos > 1) ? (ntexts_by_pos - 1) : 1;
  /* even the gaps instead of the edges or whatnot */
  /* find the "slack" in the row */
  if (point == K_Gaps)
  {
    Coord w;

    /* subtract all the "widths" from the slack */
    for (i = 0; i < ntexts_by_pos; ++i)
    {
      TextType *text = texts_by_pos[i].text;

      /* coord doesn't care if I mix Lefts/Tops */
      w = texts_by_pos[i].width =
      coord (text, dir, K_Rights) -
      coord (text, dir, K_Lefts);
      /* Gaps distribution is on centers, so half of
       * first and last text don't count */
      if (i == 0 || i == ntexts_by_pos - 1)
      {
        w /= 2;
      }
      slack -= w;
    }
    /* slack could be negative */
  }
  /* move all selected texts to the new coordinate */
  for (i = 0; i < ntexts_by_pos; ++i)
  {
    TextType *text = texts_by_pos[i].text;
    int type = texts_by_pos[i].type;
    Coord p, q, dp, dx, dy;

    /* find reference point for this text */
    q = s + slack * i / divisor;
    /* find delta from reference point to reference point */
    p = coord (text, dir, point);
    dp = q - p;
    /* ...but if we're gridful, keep the mark on the grid */
    /* TODO re-enable grid
    if (! gridless)
    {
      dp -= (coord (text, dir, K_Marks) + dp) % (long) (PCB->Grid);
    }
     */
    if (dp)
    {
      /* move from generic to X or Y */
      dx = dy = dp;
      if (dir == K_X)
        dy = 0;
      else
        dx = 0;
      /* need to know if the text is part of an element,
       * all are TEXT_TYPE, but text associated with an
       * element is also ELEMENTNAME_TYPE.  For undo, this is
       * significant in search.c: SearchObjectByID.
       *
       * MoveObject() is better as in aligntext(), but we
       * didn't keep the element reference when sorting.
       */
      MOVE_TEXT_LOWLEVEL (text, dx, dy);
      AddObjectToMoveUndoList (type, NULL, NULL, text, dx, dy);
      changed = 1;
    }
    /* in gaps mode, accumulate part widths */
    if (point == K_Gaps)
    {
      /* move remaining half of our text */
      s += texts_by_pos[i].width / 2;
      /* move half of next text */
      if (i < ntexts_by_pos - 1)
        s += texts_by_pos[i + 1].width / 2;
    }
  }
  if (changed)
  {
    RestoreUndoSerialNumber ();
    IncrementUndoSerialNumber ();
    Redraw ();
    SetChangedFlag (true);
  }
  free_texts_by_pos ();
  return 0;
}

static HID_Action distaligntext_action_list[] =
{
  {"distributetext", NULL, distributetext, "Distribute Text Elements", distributetext_syntax},
  {"aligntext", NULL, aligntext, "Align Text Elements", aligntext_syntax}
};

REGISTER_ACTIONS (distaligntext_action_list)

void
hid_distaligntext_init ()
{
  register_distaligntext_action_list ();
}
