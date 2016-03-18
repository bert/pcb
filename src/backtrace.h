/*!
 * \file src/backtrace.h
 *
 * \brief Provide a backtrace with line numbers, and an assert()-like
 * routine that prints a backtrace on failure.
 *
 * \author Britton Leo Kerin <britton.kerin@gmail.com>
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Contact addresses for paper mail and Email:
 *
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *
 * Thomas.Nau@rz.uni-ulm.de
 */

#include <errno.h>
#include <stdlib.h>

#ifndef NDEBUG

char *
backtrace_with_line_numbers (void);

/* Like assert(), but prints a full backtrace (if it does anything).
 * All caveats of backtrace_with_line_numbers() apply.  */
#  define ASSERT_BT(COND)                                                    \
    do {                                                                     \
      if ( __builtin_expect (!(COND), 0) ) {                                 \
        fprintf (                                                            \
            stderr,                                                          \
            "%s: %s:%u: %s: Assertion ` " #COND "' failed.  Backtrace:\n%s", \
            program_invocation_short_name,                                   \
            __FILE__,                                                        \
            __LINE__,                                                        \
            __func__,                                                        \
            backtrace_with_line_numbers() );                                 \
        abort ();                                                            \
      }                                                                      \
    } while ( 0 )

#else

#  define ASSERT_BT(COND)

#endif
