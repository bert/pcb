/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2005 DJ Delorie
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
 *  DJ Delorie, 334 North Road, Deerfield NH 03037-1110, USA
 *  dj@delorie.com
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <ctype.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef FLAG_TEST
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "globalconst.h"
#include "global.h"
#include "compat.h"
#include "const.h"
#include "strflags.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* Because all the macros expect it, that's why.  */
typedef struct
{
  FlagType Flags;
} FlagHolder;

/* Be careful to list more specific flags first, followed by general
 * flags, when two flags use the same bit.  For example, "onsolder" is
 * for elements only, while "auto" is for everything else.  They use
 * the same bit, but onsolder is listed first so that elements will
 * use it and not auto.
 *
 * Thermals are handled separately, as they're layer-selective.
 */

typedef struct
{

  /* This is the bit that we're setting.  */
  int mask;

  /* The name used in the output file.  */
  char *name;
  int nlen;
#define N(x) x, sizeof(x)-1

  /* If set, this entry won't be output unless the object type is one
     of these.  */
  int object_types;

} FlagBitsType;

static FlagBitsType object_flagbits[] = {
  { PINFLAG, N ("pin"), ALL_TYPES },
  { VIAFLAG, N ("via"), ALL_TYPES },
  { FOUNDFLAG, N ("found"), ALL_TYPES },
  { HOLEFLAG, N ("hole"), PIN_TYPES },
  { RATFLAG, N ("rat"), RATLINE_TYPE },
  { PININPOLYFLAG, N ("pininpoly"), PIN_TYPES | PAD_TYPE },
  { CLEARPOLYFLAG, N ("clearpoly"), POLYGON_TYPE },
  { HIDENAMEFLAG, N ("hidename"), ELEMENT_TYPE },
  { DISPLAYNAMEFLAG, N ("showname"), ELEMENT_TYPE },
  { CLEARLINEFLAG, N ("clearline"), LINE_TYPE | ARC_TYPE | TEXT_TYPE },
  { SELECTEDFLAG, N ("selected"), ALL_TYPES },
  { ONSOLDERFLAG, N ("onsolder"), ELEMENT_TYPE | PAD_TYPE | TEXT_TYPE },
  { AUTOFLAG, N ("auto"), ALL_TYPES },
  { SQUAREFLAG, N ("square"), PIN_TYPES | PAD_TYPE },
  { RUBBERENDFLAG, N ("rubberend"), LINE_TYPE | ARC_TYPE },
  { WARNFLAG, N ("warn"), PIN_TYPES | PAD_TYPE },
  { USETHERMALFLAG, N ("usetherm"), PIN_TYPES | LINE_TYPE | ARC_TYPE },
  { OCTAGONFLAG, N ("octagon"), PIN_TYPES | PAD_TYPE },
  { DRCFLAG, N ("drc"), ALL_TYPES },
  { LOCKFLAG, N ("lock"), ALL_TYPES },
  { EDGE2FLAG, N ("edge2"), ALL_TYPES },
  { FULLPOLYFLAG, N ("fullpoly"), POLYGON_TYPE},
  { NOPASTEFLAG, N ("nopaste"), PAD_TYPE }
};

static FlagBitsType pcb_flagbits[] = {
  { SHOWNUMBERFLAG, N ("shownumber"), 1 },
  { LOCALREFFLAG, N ("localref"), 1 },
  { CHECKPLANESFLAG, N ("checkplanes"), 1 },
  { SHOWDRCFLAG, N ("showdrc"), 1 },
  { RUBBERBANDFLAG, N ("rubberband"), 1 },
  { DESCRIPTIONFLAG, N ("description"), 1 },
  { NAMEONPCBFLAG, N ("nameonpcb"), 1 },
  { AUTODRCFLAG, N ("autodrc"), 1 },
  { ALLDIRECTIONFLAG, N ("alldirection"), 1 },
  { SWAPSTARTDIRFLAG, N ("swapstartdir"), 1 },
  { UNIQUENAMEFLAG, N ("uniquename"), 1 },
  { CLEARNEWFLAG, N ("clearnew"), 1 },
  { NEWFULLPOLYFLAG, N ("newfullpoly"), 1 },
  { SNAPPINFLAG, N ("snappin"), 1 },
  { SHOWMASKFLAG, N ("showmask"), 1 },
  { THINDRAWFLAG, N ("thindraw"), 1 },
  { ORTHOMOVEFLAG, N ("orthomove"), 1 },
  { LIVEROUTEFLAG, N ("liveroute"), 1 },
  { THINDRAWPOLYFLAG, N ("thindrawpoly"), 1 },
  { LOCKNAMESFLAG, N ("locknames"), 1 },
  { ONLYNAMESFLAG, N ("onlynames"), 1 },
  { HIDENAMESFLAG, N ("hidenames"), 1 },
};

#undef N

/*
 * This helper function maintains a small list of buffers which are
 * used by flags_to_string().  Each buffer is allocated from the heap,
 * but the caller must not free them (they get realloced when they're
 * reused, but never completely freed).
 */

static struct
{
  char *ptr;
  int len;
} buffers[10];
static int bufptr = 0;
static char *
alloc_buf (int len)
{
#define B buffers[bufptr]
  len++;
  bufptr = (bufptr + 1) % 10;
  if (B.len < len)
    {
      if (B.ptr)
	B.ptr = (char *) realloc (B.ptr, len);
      else
	B.ptr = (char *) malloc (len);
      B.len = len;
    }
  return B.ptr;
#undef B
}

/*
 * This set of routines manages a list of layer-specific flags.
 * Callers should call grow_layer_list(0) to reset the list, and
 * set_layer_list(layer,1) to set bits in the layer list.  The results
 * are stored in layers[], which has num_layers valid entries.
 */

static char *layers = 0;
static int max_layers = 0, num_layers = 0;

static void
grow_layer_list (int num)
{
  if (layers == 0)
    {
      layers = (char *) calloc (num > 0 ? num : 1, 1);
      max_layers = num;
    }
  else if (num > max_layers)
    {
      max_layers = num;
      layers = (char *) realloc (layers, max_layers);
    }
  if (num > num_layers)
    memset (layers + num_layers, 0, num - num_layers - 1);
  num_layers = num;
  return;
}

static inline void
set_layer_list (int layer, int v)
{
  if (layer >= num_layers)
    grow_layer_list (layer + 1);
  layers[layer] = v;
}

/*
 * These next two convert between layer lists and strings.
 * parse_layer_list() is passed a pointer to a string, and parses a
 * list of integer which reflect layers to be flagged.  It returns a
 * pointer to the first character following the list.  The syntax of
 * the list is a paren-surrounded, comma-separated list of integers
 * and/or pairs of integers separated by a dash (like "(1,2,3-7)").
 * Spaces and other punctuation are not allowed.  The results are
 * stored in layers[] defined above.
 *
 * print_layer_list() does the opposite - it uses the flags set in
 * layers[] to build a string that represents them, using the syntax
 * above.
 * 
 */

/* Returns a pointer to the first character past the list. */
static const char *
parse_layer_list (const char *bp, int (*error) (const char *))
{
  const char *orig_bp = bp;
  int l = 0, range = -1;
  int value = 1;

  grow_layer_list (0);
  while (*bp)
    {
      if (*bp == '+')
        value = 2;
      else if (*bp == 'S')
        value = 3;
      else if (*bp == 'X')
        value = 4;
      else if (*bp == 't')
        value = 5;
      else if (*bp == ')' || *bp == ',' || *bp == '-')
	{
	  if (range == -1)
	    range = l;
	  while (range <= l)
	    set_layer_list (range++, value);
	  if (*bp == '-')
	    range = l;
	  else
	    range = -1;
	  value = 1;
	  l = 0;
	}

      else if (isdigit ((int) *bp))
	l = l * 10 + (*bp - '0');

      else if (error)
	{
	  char *fmt = "Syntax error parsing layer list \"%.*s\" at %c";
	  char *msg = alloc_buf (strlen (fmt) + strlen (orig_bp));
	  sprintf (msg, fmt, bp - orig_bp + 5, orig_bp, *bp);
	  error (msg);
	  error = NULL;
	}

      if (*bp == ')')
	return bp + 1;

      bp++;
    }
  return bp;
}

/* Number of character the value "i" requires when printed. */
static int
printed_int_length (int i, int j)
{
  int rv;

  if (i < 10)
    return 1 + (j ? 1 : 0);
  if (i < 100)
    return 2 + (j ? 1 : 0);

  for (rv = 1; i >= 10; rv++)
    i /= 10;
  return rv + (j ? 1 : 0);
}

/* Returns a pointer to an internal buffer which is overwritten with
   each new call.  */
static char *
print_layer_list ()
{
  static char *buf = 0;
  static int buflen = 0;
  int len, i, j;
  char *bp;

  len = 2;
  for (i = 0; i < num_layers; i++)
    if (layers[i])
      len += 1 + printed_int_length (i, layers[i]);
  if (buflen < len)
    {
      if (buf)
	buf = (char *) realloc (buf, len);
      else
	buf = (char *) malloc (len);
      buflen = len;
    }

  bp = buf;
  *bp++ = '(';

  for (i = 0; i < num_layers; i++)
    if (layers[i])
      {
	/* 0 0 1 1 1 0 0 */
	/*     i     j   */
	for (j = i + 1; j < num_layers && layers[j] == 1; j++)
	  ;
	if (j > i + 2)
	  {
	    sprintf (bp, "%d-%d,", i, j - 1);
	    i = j - 1;
	  }
	else
	  switch (layers[i])
	  {
	    case 1:
	     sprintf (bp, "%d,", i);
	     break;
	    case 2:
	     sprintf (bp, "%d+,", i);
	     break;
	    case 3:
	     sprintf (bp, "%dS,", i);
	     break;
	    case 4:
	     sprintf (bp, "%dX,", i);
	     break;
	    case 5:
	    default:
	     sprintf (bp, "%dt,", i);
	     break;
	   }
	bp += strlen (bp);
      }
  bp[-1] = ')';
  *bp = 0;
  return buf;
}

/*
 * Ok, now the two entry points to this file.  The first, string_to_flags,
 * is passed a string (usually from parse_y.y) and returns a "set of flags".
 * In theory, this can be anything, but for now it's just an integer.  Later
 * it might be a structure, for example.
 *
 * Currently, there is no error handling :-P
 */

static int
error_ignore (const char *msg)
{				/* do nothing */
  return 0;
}
static FlagType empty_flags;

static FlagType
common_string_to_flags (const char *flagstring,
			int (*error) (const char *msg),
			FlagBitsType *flagbits,
			int n_flagbits)
{
  const char *fp, *ep;
  int flen;
  FlagHolder rv;
  int i;

  rv.Flags = empty_flags;

  if (error == 0)
    error = error_ignore;

  if (flagstring == NULL)
    return empty_flags;

  fp = ep = flagstring;

  if (*fp == '"')
    ep = ++fp;

  while (*ep && *ep != '"')
    {
      int found = 0;

      for (ep = fp; *ep && *ep != ',' && *ep != '"' && *ep != '('; ep++)
	;
      flen = ep - fp;
      if (*ep == '(')
	ep = parse_layer_list (ep + 1, error);

      if (flen == 7 && memcmp (fp, "thermal", 7) == 0)
	{
	  for (i = 0; i < MAX_LAYER && i < num_layers; i++)
	    if (layers[i])
	      ASSIGN_THERM (i, layers[i], &rv);
	}
      else
	{
	  for (i = 0; i < n_flagbits; i++)
	    if (flagbits[i].nlen == flen
		&& memcmp (flagbits[i].name, fp, flen) == 0)
	      {
		found = 1;
		SET_FLAG (flagbits[i].mask, &rv);
		break;
	      }
	  if (!found)
	    {
	      const char *fmt = "Unknown flag: \"%.*s\" ignored";
	      char *msg = alloc_buf (strlen (fmt) + flen);
	      sprintf (msg, fmt, flen, fp);
	      error (msg);
	    }
	}
      fp = ep + 1;
    }
  return rv.Flags;
}

FlagType
string_to_flags (const char *flagstring,
		 int (*error) (const char *msg))
{
  return common_string_to_flags (flagstring,
				 error,
				 object_flagbits,
				 ENTRIES (object_flagbits));
}

FlagType
string_to_pcbflags (const char *flagstring,
		    int (*error) (const char *msg))
{
  return common_string_to_flags (flagstring,
				 error,
				 pcb_flagbits,
				 ENTRIES (pcb_flagbits));
}


/*
 * Given a set of flags for a given type of object, return a string
 * which reflects those flags.  The only requirement is that this
 * string be parseable by string_to_flags.
 *
 * Note that this function knows a little about what kinds of flags
 * will be automatically set by parsing, so it won't (for example)
 * include the "via" flag for VIA_TYPEs because it knows those get
 * forcibly set when vias are parsed.
 */

static char *
common_flags_to_string (FlagType flags,
			int object_type,
			FlagBitsType *flagbits,
			int n_flagbits)
{
  int len;
  int i;
  FlagHolder fh, savef;
  char *buf, *bp;

  fh.Flags = flags;

#ifndef FLAG_TEST
  switch (object_type)
    {
    case VIA_TYPE:
      CLEAR_FLAG (VIAFLAG, &fh);
      break;
    case RATLINE_TYPE:
      CLEAR_FLAG (RATFLAG, &fh);
      break;
    case PIN_TYPE:
      CLEAR_FLAG (PINFLAG, &fh);
      break;
    }
#endif

  savef = fh;

  len = 3;			/* for "()\0" */
  for (i = 0; i < n_flagbits; i++)
    if ((flagbits[i].object_types & object_type)
	&& (TEST_FLAG (flagbits[i].mask, &fh)))
      {
	len += flagbits[i].nlen + 1;
	CLEAR_FLAG (flagbits[i].mask, &fh);
      }

  if (TEST_ANY_THERMS (&fh))
    {
      len += sizeof ("thermal()");
      for (i = 0; i < MAX_LAYER; i++)
	if (TEST_THERM (i, &fh))
	  len += printed_int_length (i, GET_THERM (i, &fh)) + 1;
    }

  bp = buf = alloc_buf (len + 2);

  *bp++ = '"';

  fh = savef;
  for (i = 0; i < n_flagbits; i++)
    if (flagbits[i].object_types & object_type
	&& (TEST_FLAG (flagbits[i].mask, &fh)))
      {
	if (bp != buf + 1)
	  *bp++ = ',';
	strcpy (bp, flagbits[i].name);
	bp += flagbits[i].nlen;
	CLEAR_FLAG (flagbits[i].mask, &fh);
      }

  if (TEST_ANY_THERMS (&fh))
    {
      if (bp != buf + 1)
	*bp++ = ',';
      strcpy (bp, "thermal");
      bp += strlen ("thermal");
      grow_layer_list (0);
      for (i = 0; i < MAX_LAYER; i++)
	if (TEST_THERM (i, &fh))
	  set_layer_list (i, GET_THERM (i, &fh));
      strcpy (bp, print_layer_list ());
      bp += strlen (bp);
    }

  *bp++ = '"';
  *bp = 0;
  return buf;
}

char *
flags_to_string (FlagType flags, int object_type)
{
  return common_flags_to_string (flags,
				 object_type,
				 object_flagbits,
				 ENTRIES (object_flagbits));
}

char *
pcbflags_to_string (FlagType flags)
{
  return common_flags_to_string (flags,
				 ALL_TYPES,
				 pcb_flagbits,
				 ENTRIES (pcb_flagbits));
}

#if FLAG_TEST

static void
dump_flag (FlagType * f)
{
  int l;
  printf ("F:%08x T:[", f->f);
  for (l = 0; l < (MAX_LAYER + 7) / 8; l++)
    printf (" %02x", f->t[l]);
  printf ("]");
}


int
mem_any_set (unsigned char *ptr, int bytes)
{
  while (bytes--)
    if (*ptr++)
      return 1;
  return 0;
}


/*
 * This exists for standalone testing of this file.
 *
 * Compile as: gcc -DHAVE_CONFIG_H -DFLAG_TEST strflags.c -o strflags.x -I..
 * and then run it.
 */

int
main ()
{
  time_t now;
  int i;
  int errors = 0, count = 0;

  time (&now);
  srandom ((unsigned int) now + getpid ());

  grow_layer_list (0);
  for (i = 0; i < 16; i++)
    {
      int j;
      char *p;
      if (i != 1 && i != 4 && i != 5 && i != 9)
	set_layer_list (i, 1);
      else
	set_layer_list (i, 0);
      p = print_layer_list ();
      printf ("%2d : %20s =", i, p);
      parse_layer_list (p + 1, 0);
      for (j = 0; j < num_layers; j++)
	printf (" %d", layers[j]);
      printf ("\n");
    }

  while (count < 1000000)
    {
      FlagHolder fh;
      char *str;
      FlagType new_flags;
      int i;
      int otype;

      otype = ALL_TYPES;
      fh.Flags = empty_flags;
      for (i = 0; i < ENTRIES (object_flagbits); i++)
	{
	  if (TEST_FLAG (object_flagbits[i].mask, &fh))
	    continue;
	  if ((otype & object_flagbits[i].object_types) == 0)
	    continue;
	  if ((random () & 4) == 0)
	    continue;

	  otype &= object_flagbits[i].object_types;
	  SET_FLAG (object_flagbits[i].mask, &fh);
	}

      if (otype & PIN_TYPES)
	for (i = 0; i < MAX_LAYER; i++)
	  if (random () & 4)
	    ASSIGN_THERM (i, 3, &fh);

      str = flags_to_string (fh.Flags, otype);
      new_flags = string_to_flags (str, 0);

      count++;
      if (FLAGS_EQUAL (fh.Flags, new_flags))
	continue;

      dump_flag (&fh.Flags);
      printf (" ");
      dump_flag (&new_flags);
      printf ("\n");
      if (++errors == 5)
	exit (1);
    }

  while (count < 1000000)
    {
      FlagHolder fh;
      char *str;
      FlagType new_flags;
      int i;
      int otype;

      otype = ALL_TYPES;
      fh.Flags = empty_flags;
      for (i = 0; i < ENTRIES (pcb_flagbits); i++)
	{
	  if (TEST_FLAG (pcb_flagbits[i].mask, &fh))
	    continue;
	  if ((random () & 4) == 0)
	    continue;

	  otype &= pcb_flagbits[i].object_types;
	  SET_FLAG (pcb_flagbits[i].mask, &fh);
	}

      str = pcbflags_to_string (fh.Flags);
      new_flags = string_to_pcbflags (str, 0);

      count++;
      if (FLAGS_EQUAL (fh.Flags, new_flags))
	continue;

      dump_flag (&fh.Flags);
      printf (" ");
      dump_flag (&new_flags);
      printf ("\n");
      if (++errors == 5)
	exit (1);
    }
  printf ("%d out of %d failed\n", errors, count);
  return errors;
}

#endif
