/*!
 * \file src/backtrace.c
 *
 * \brief Implementation of interface described in backtrace.h.
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

#include <assert.h>
#include <execinfo.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "backtrace.h"

#ifndef NDEBUG

/*!
 * \brief Return a new multi-line string containing a backtrace of the
 * call point, with line numbers.
 *
 * Caveats:
 *
 *   * This uses the GNU libc backtrace() function, all caveats related to
 *     that function apply here as well.  In particular, the GCC -g compile
 *     time and -rdynamic link-time options are required, and line numbers
 *     might be wrong if the -O0 (disable all optimization) isn't used.
 *     There might be other issues as well.  To avoid these problems build
 *     pcb like this:
 *
 *       make CFLAGS='-Wall -g -O0'
 *
 *   * FIXME: as of this writing, --disable-nls is also required at
 *     ./configure-time to avoid bugs in NLS that trigger with -O0.
 *
 *   * Flex and Bison still seem to cause slight line number error.
 *     
 *   * It uses malloc(), so cannot reliably be used when malloc() might fail
 *     (e.g. after memory corruption).
 *
 *   * It probably isn't reentrant.
 */
char *
backtrace_with_line_numbers (void)
{
  char executable_name[PATH_MAX + 1];
  ssize_t bytes_read;
  size_t btrace_size;                 /* Number of addresses */
  int return_code;
  /* Backtrace Addresses (temporary file, initialized to template) */
  char ba[] = "/tmp/baXXXXXX";  
  /* Most Recent Backtrace Text (temporary file, initialized to template) */
  char bt[] = "/tmp/btXXXXXX";   /* Most Recent Backtrace Text  */
  int tfd;     /* Temporary File Descriptor (reused for different files) */
  FILE *tfp;   /* Trmporary FILE Pointer (reused for different things) */
#define BT_MAX_STACK 142
  void *btrace_array[BT_MAX_STACK];   /* Actual addressess */
  int ii;      /* Index variable */
#define ADDR2LINE_COMMAND_MAX_LENGTH 242
  char addr2line_command[ADDR2LINE_COMMAND_MAX_LENGTH + 1];
  int bytes_printed;
  struct stat stat_buf;
  char *result;

  /* Use /proc magic to find which binary we are  */
  bytes_read = readlink ("/proc/self/exe", executable_name, PATH_MAX + 1);
  assert (bytes_read != -1);
  assert (bytes_read <= PATH_MAX);   /* Systems don't always honor PATH_MAX */
  executable_name[bytes_read] = '\0';   /* Readlink doesn't do this for us */
  
  /* Get the actual backtrace  */
  btrace_size = backtrace (btrace_array, BT_MAX_STACK);
  assert (btrace_size < BT_MAX_STACK);

  /* Create temp files (filling in actual names), close file descriptors  */
  tfd = mkstemp (ba);
  assert (tfd != -1);
  return_code = close (tfd);
  assert (return_code == 0);
  tfd = mkstemp (bt);
  assert (tfd != -1);
  return_code = close (tfd);
  assert (return_code == 0);

  /* Print the addresses to the address file  */
  tfp = fopen (ba, "w");
  assert (tfp != NULL);
  for ( ii = 1 ; ii < btrace_size ; ii++ )    /* Skip 0 because that's us */
    {
      fprintf (tfp, "%p\n", btrace_array[ii]);
    } 
  return_code = fclose (tfp);
  assert (return_code == 0);

  /* Run addr2line to convert addresses to show func, file, line  */
  bytes_printed
    = snprintf (
        addr2line_command,
        ADDR2LINE_COMMAND_MAX_LENGTH + 1,
        "addr2line --exe %s -f -i <%s >%s",
        executable_name,
        ba,
        bt );
  assert (bytes_printed <= ADDR2LINE_COMMAND_MAX_LENGTH);
  return_code = system (addr2line_command);
  assert (return_code == 0);

  /* Get the size of the result */
  return_code = stat (bt, &stat_buf);
  assert (return_code == 0);
  
  /* Allocate storage for result */
  result = malloc (stat_buf.st_size + 1);   /* +1 for trailing null byte */
  assert (result != NULL);

  /* Read the func, file, line form back in */
  tfp = fopen (bt, "r");
  assert (tfp != NULL);
  bytes_read = fread (result, 1, stat_buf.st_size, tfp);
  assert (bytes_read == stat_buf.st_size);
  result[stat_buf.st_size] = '\0';
  return_code = fclose (tfp);
  assert (return_code == 0);

  /* Remove the temporary files */
  return_code = unlink (bt);
  assert (return_code == 0);
  return_code = unlink (ba);
  assert (return_code == 0);

  return result;
}

#endif
