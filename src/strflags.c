/*!
 * \file src/strflags.c
 *
 * \brief strflags.
 *
 * The purpose of this interface is to make the file format able to
 * handle more than 32 flags, and to hide the internal details of
 * flags from the file format.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2005 DJ Delorie
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
 * DJ Delorie, 334 North Road, Deerfield NH 03037-1110, USA
 *
 * dj@delorie.com
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

#include "globalconst.h"
#include "global.h"
#include "compat.h"
#include "hid.h"
#include "strflags.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* Because all the macros expect it, that's why.  */
typedef struct
{
  FlagType Flags;
} FlagHolder;

/*!
 * Be careful to list more specific flags first, followed by general
 * flags, when two flags use the same bit.  For example, "onsolder" is
 * for elements only, while "auto" is for everything else.  They use
 * the same bit, but onsolder is listed first so that elements will
 * use it and not auto.
 *
 * Thermals are handled separately, as they're layer-selective.
 */
typedef struct
{
  int mask; /*!< This is the bit that we're setting. */
  char *name; /*!< The name used in the output file. */
  int nlen;
#define N(x) x, sizeof(x)-1

  int object_types; /*!< If set, this entry won't be output unless the
    * object type is one of these.  */
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
  { ONSOLDERFLAG, N ("onsolder"), ELEMENT_TYPE | PAD_TYPE | TEXT_TYPE | ELEMENTNAME_TYPE },
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
  { NOPASTEFLAG, N ("nopaste"), PAD_TYPE },
  { CONNECTEDFLAG, N ("connected"), ALL_TYPES }
};

static FlagBitsType pcb_flagbits[] = {
  { SHOWNUMBERFLAG, N ("shownumber"), ALL_TYPES },
  { LOCALREFFLAG, N ("localref"), ALL_TYPES },
  { CHECKPLANESFLAG, N ("checkplanes"), ALL_TYPES },
  { SHOWDRCFLAG, N ("showdrc"), ALL_TYPES },
  { RUBBERBANDFLAG, N ("rubberband"), ALL_TYPES },
  { DESCRIPTIONFLAG, N ("description"), ALL_TYPES },
  { NAMEONPCBFLAG, N ("nameonpcb"), ALL_TYPES },
  { AUTODRCFLAG, N ("autodrc"), ALL_TYPES },
  { ALLDIRECTIONFLAG, N ("alldirection"), ALL_TYPES },
  { SWAPSTARTDIRFLAG, N ("swapstartdir"), ALL_TYPES },
  { UNIQUENAMEFLAG, N ("uniquename"), ALL_TYPES },
  { CLEARNEWFLAG, N ("clearnew"), ALL_TYPES },
  { NEWFULLPOLYFLAG, N ("newfullpoly"), ALL_TYPES },
  { SNAPPINFLAG, N ("snappin"), ALL_TYPES },
  { SHOWMASKFLAG, N ("showmask"), ALL_TYPES },
  { THINDRAWFLAG, N ("thindraw"), ALL_TYPES },
  { ORTHOMOVEFLAG, N ("orthomove"), ALL_TYPES },
  { LIVEROUTEFLAG, N ("liveroute"), ALL_TYPES },
  { THINDRAWPOLYFLAG, N ("thindrawpoly"), ALL_TYPES },
  { LOCKNAMESFLAG, N ("locknames"), ALL_TYPES },
  { ONLYNAMESFLAG, N ("onlynames"), ALL_TYPES },
  { HIDENAMESFLAG, N ("hidenames"), ALL_TYPES },
  { AUTOBURIEDVIASFLAG, N ("autoburiedvias"), ALL_TYPES },
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

void
uninit_strflags_buf (void)
{
  int n;
  for (n = 0; n < 10; n++)
    {
      if (buffers[n].ptr != NULL)
        {
          free (buffers[n].ptr);
          buffers[n].ptr = NULL;
        }
    }
}

static char *layers = 0;
static int max_layers = 0, num_layers = 0;

/*!
 * \brief Grow layer list.
 *
 * This routine manages a list of layer-specific flags.
 *
 * Callers should call grow_layer_list(0) to reset the list, and
 * set_layer_list(layer,1) to set bits in the layer list.
 *
 * The results are stored in layers[], which has \c num_layers valid
 * entries.
 */
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

void
uninit_strflags_layerlist (void)
{
  if (layers != NULL)
    {
      free (layers);
      layers = NULL;
      num_layers = max_layers = 0;
    }
}

/*!
 * \brief Set layer list.
 *
 * This routine manages a list of layer-specific flags.
 *
 * Callers should call grow_layer_list(0) to reset the list, and
 * set_layer_list(layer,1) to set bits in the layer list.
 *
 * The results are stored in layers[], which has \c num_layers valid
 * entries.
 */
static inline void
set_layer_list (int layer, int v)
{
  if (layer >= num_layers)
    grow_layer_list (layer + 1);
  layers[layer] = v;
}

/*!
 * \brief Parse layer list.
 *
 * parse_layer_list() is passed a pointer to a string, and parses a
 * list of integer which reflect layers to be flagged.
 *
 * It returns a pointer to the first character following the list.
 *
 * The syntax of the list is a paren-surrounded, comma-separated list of
 * integers and/or pairs of integers separated by a dash
 * (like "(1,2,3-7)").
 *
 * Spaces and other punctuation are not allowed.
 *
 * The results are stored in \c layers[].
 *
 * \return a pointer to the first character past the list.
 */
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

/*!
 * \brief Number of character the value "i" requires when printed.
 */
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

/*!
 * \brief Print layer list.
 *
 * print_layer_list() uses the flags set in layers[] to build a string
 * that represents them, using the syntax described below.
 *
 * The syntax of the list is a paren-surrounded, comma-separated list of
 * integers and/or pairs of integers separated by a dash
 * (like "(1,2,3-7)").
 *
 * Spaces and other punctuation are not allowed.
 *
 * \return a pointer to an internal buffer which is overwritten with
 * each new call.
 */
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

/*!
 * \brief Convert strings to flags.
 *
 * When passed a string, parse it and return an appropriate set of
 * flags.
 *
 * Errors cause error() to be called with a suitable message;
 * if error is NULL, errors are ignored.
 */
FlagType
string_to_flags (const char *flagstring,
		 int (*error) (const char *msg))
{
  return common_string_to_flags (flagstring,
				 error,
				 object_flagbits,
				 ENTRIES (object_flagbits));
}

/*!
 * \brief Convert strings to PCB flags.
 *
 * When passed a string, parse it and return an appropriate set of
 * flags.
 *
 * Errors cause error() to be called with a suitable message;
 * if error is NULL, errors are ignored.
 */
FlagType
string_to_pcbflags (const char *flagstring,
		    int (*error) (const char *msg))
{
  return common_string_to_flags (flagstring,
				 error,
				 pcb_flagbits,
				 ENTRIES (pcb_flagbits));
}

/*!
 * \brief Common flags converted to strings.
 *
 * Given a set of flags for a given type of object, return a string
 * which reflects those flags.
 *
 * The only requirement is that this string be parseable by string_to_flags.
 *
 * \note This function knows a little about what kinds of flags
 * will be automatically set by parsing, so it won't (for example)
 * include the "via" flag for VIA_TYPEs because it knows those get
 * forcibly set when vias are parsed.
 *
 * \warning Currently, there is no error handling :-P
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

/*!
 * \brief Object flags converted to strings.
 *
 * Given a set of flags for a given object type, return a string which
 * can be output to a file.
 *
 * The returned pointer must not be freed.
 */
char *
flags_to_string (FlagType flags, int object_type)
{
  return common_flags_to_string (flags,
				 object_type,
				 object_flagbits,
				 ENTRIES (object_flagbits));
}

/*!
 * \brief PCB flags converted to strings.
 */
char *
pcbflags_to_string (FlagType flags)
{
  return common_flags_to_string (flags,
				 ALL_TYPES,
				 pcb_flagbits,
				 ENTRIES (pcb_flagbits));
}
