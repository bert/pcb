/*!
 * \file src/strcasestr.c
 *
 * \brief Replacement for strcasestr on systems which need it
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2017 Dan McMahill
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation;  version 2 of the License, or
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

#if !HAVE_STRCASESTR
/*
 * strcasestr() is non-standard so provide a replacement
 * when missing
 */
char * strcasestr(const char *big, const char *little)
{
  char *b, *l, *p;

  b = strdup(big);
  if(b == NULL ) {
    return NULL;
  }

  l = strdup(little);
  if(l == NULL ) {
    free(b);
    return NULL;
  }


  p = b;
  while(*p != '\0') {
    if (islower(*p)) {
      *p = (char) toupper( (int) *p);
    }
    p++;
  }

  p = l;
  while(*p != '\0') {
    if (islower(*p)) {
      *p = (char) toupper( (int) *p);
    }
    p++;
  }


  /* cheat and do this the easy.  Convert to upper case,
   * use strstr, and then figure out an offset in *big
   */
  p = strstr(b, l);
  if (p == NULL) {
    free(b);
    free(l);
    return NULL;
  }

  p = (char *) big + (p - b);
  free(b);
  free(l);
  return p;
}
#endif
