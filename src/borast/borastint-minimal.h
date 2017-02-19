/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is University of Southern
 * California.
 *
 * Contributor(s):
 *	Carl D. Worth <cworth@cworth.org>
 */

/*
 * These definitions are solely for use by the implementation of cairo
 * and constitute no kind of standard.  If you need any of these
 * functions, please drop me a note.  Either the library needs new
 * functionality, or there's a way to do what you need using the
 * existing published interfaces. cworth@cworth.org
 */

#ifndef _BORASTINT_H_
#define _BORASTINT_H_

#if HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _MSC_VER
#define borast_public __declspec(dllexport)
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#include <limits.h>
#include <stdio.h>

#include "borast-minimal.h"

#include "borast-compiler-private.h"

BORAST_BEGIN_DECLS

#if _WIN32 && !_WIN32_WCE /* Permissions on WinCE? No worries! */
borast_private FILE *
_borast_win32_tmpfile (void);
#define tmpfile() _borast_win32_tmpfile()
#endif

#undef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#undef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif

#ifndef M_SQRT1_2
#define M_SQRT1_2 0.707106781186547524400844362104849039
#endif

#undef  ARRAY_LENGTH
#define ARRAY_LENGTH(__array) ((int) (sizeof (__array) / sizeof (__array[0])))

#undef STRINGIFY
#undef STRINGIFY_ARG
#define STRINGIFY(macro_or_string)    STRINGIFY_ARG (macro_or_string)
#define STRINGIFY_ARG(contents)       #contents

#if defined (__GNUC__)
#define borast_container_of(ptr, type, member) ({ \
    const __typeof__ (((type *) 0)->member) *mptr__ = (ptr); \
    (type *) ((char *) mptr__ - offsetof (type, member)); \
})
#else
#define borast_container_of(ptr, type, member) \
    (type *)((char *) (ptr) - (char *) &((type *)0)->member)
#endif


/* Size in bytes of buffer to use off the stack per functions.
 * Mostly used by text functions.  For larger allocations, they'll
 * malloc(). */
#ifndef BORAST_STACK_BUFFER_SIZE
#define BORAST_STACK_BUFFER_SIZE (512 * sizeof (int))
#endif

#define BORAST_STACK_ARRAY_LENGTH(T) (BORAST_STACK_BUFFER_SIZE / sizeof(T))


#include "borast-types-private.h"

#if HAVE_VALGRIND
# include <memcheck.h>
# define VG(x) x
#else
# define VG(x)
#endif

#endif
