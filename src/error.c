/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

static char *rcsid = "$Id$";

/* error and debug funtions
 * getpid() needs a cast to (int) to get rid of compiler warnings
 * on several architectures
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>

#include "global.h"

#include "data.h"
#include "error.h"
#include "dialog.h"
#include "file.h"
#include "gui.h"
#include "log.h"
#include "misc.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ----------------------------------------------------------------------
 * some external identifiers
 */
extern int errno,		/* system error code */
  sys_nerr;			/* number of messages available from array */

#if !defined(HAVE_STRERROR)
#define USE_SYS_ERRLIST
#endif

/* the list is already defined for some OS */
#if !defined(__NetBSD__) && !defined(__FreeBSD__) && !defined(__linux__)
  #ifdef USE_SYS_ERRLIST
	extern char *sys_errlist[];	/* array of error messages */
  #endif
#endif

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static int OriginalStderr = -1;	/* copy of the original stderr */
static Window LogWindow;	/* the logging window */

/* ---------------------------------------------------------------------------
 * output of message in a dialog window or log window
 */
void
Message (char *Format, ...)
{
  va_list args;
  char s[1024];
  XEvent event;
  va_start (args, Format);
  vsprintf (s, Format, args);
  va_end (args);

  if (Settings.UseLogWindow)
    {
      /* fputs may return an error if the pipe isn't read for
       * a longer time
       */
      while (fputs (s, stderr) == EOF && errno == EAGAIN && LogWindow)
	while (XtAppPending (Context))
	  {
	    XtAppNextEvent (Context, &event);
	    XtDispatchEvent (&event);
	  }
      fflush (stderr);
    }
  else
    {
      Beep (Settings.Volume);
      MessageDialog (s);
    }
}

/* ---------------------------------------------------------------------------
 * print standard 'open error'
 */
void
OpenErrorMessage (char *Filename)
{
   #ifdef USE_SYS_ERRLIST
 	Message("can't open file\n"
 		"   '%s'\nfopen() returned: '%s'\n",
 		Filename, errno <= sys_nerr ? sys_errlist[errno] : "???");
   #else
	Message("can't open file\n"
		"   '%s'\nfopen() returned: '%s'\n",
		Filename, strerror(errno));
   #endif
}

/* ---------------------------------------------------------------------------
 * print standard 'popen error'
 */
void
PopenErrorMessage (char *Filename)
{
   #ifdef USE_SYS_ERRLIST
 	Message("can't execute command\n"
 		"   '%s'\npopen() returned: '%s'\n",
 		Filename, errno <= sys_nerr ? sys_errlist[errno] : "???");
   #else
	Message("can't execute command\n"
		"   '%s'\npopen() returned: '%s'\n",
		Filename, strerror(errno));
   #endif
}

/* ---------------------------------------------------------------------------
 * print standard 'opendir'
 */
void
OpendirErrorMessage (char *DirName)
{
   #ifdef USE_SYS_ERRLIST
 	Message("can't scan directory\n"
 		"   '%s'\nopendir() returned: '%s'\n",
 		DirName, errno <= sys_nerr ? sys_errlist[errno] : "???");
   #else
	Message("can't scan directory\n"
		"   '%s'\nopendir() returned: '%s'\n",
		DirName, strerror(errno));
   #endif
}

/* ---------------------------------------------------------------------------
 * print standard 'chdir error'
 */
void
ChdirErrorMessage (char *DirName)
{
   #ifdef USE_SYS_ERRLIST
 	Message("can't change working directory to\n"
 		"   '%s'\nchdir() returned: '%s'\n",
 		DirName, errno <= sys_nerr ? sys_errlist[errno] : "???");
   #else
	Message("can't change working directory to\n"
		"   '%s'\nchdir() returned: '%s'\n",
		DirName, strerror(errno));
   #endif
}

/* ---------------------------------------------------------------------------
 * restores the original stderr
 */
void
RestoreStderr (void)
{
  if (OriginalStderr != -1)
    {
      close (2);
      dup (OriginalStderr);
      OriginalStderr = -1;
    }
}

/* ---------------------------------------------------------------------------
 * output of fatal error message
 */
void
MyFatal (char *Format, ...)
{
  va_list args;

  va_start (args, Format);
  /* try to save the layout and do some cleanup */
  RestoreStderr ();
  EmergencySave ();
  fprintf (stderr, "%s (%i): fatal, ", Progname, (int) getpid ());
  vfprintf (stderr, Format, args);
  fflush (stderr);
  va_end (args);
  exit (1);
}

/* ---------------------------------------------------------------------------
 * catches signals which abort the program
 */
void
CatchSignal (int Signal)
{
  char *s;

  switch (Signal)
    {
    case SIGHUP:
      s = "SIGHUP";
      break;
    case SIGINT:
      s = "SIGINT";
      break;
    case SIGQUIT:
      s = "SIGQUIT";
      break;
    case SIGABRT:
      s = "SIGABRT";
      break;
    case SIGTERM:
      s = "SIGTERM";
      break;
    case SIGSEGV:
      s = "SIGSEGV";
      break;
    default:
      s = "unknown";
      break;
    }
  MyFatal ("aborted by %s signal\n", s);
}

/* ---------------------------------------------------------------------------
 * default error handler for X11 errors
 */
void
X11ErrorHandler (String Msg)
{
  MyFatal ("X11 fatal error: %s\n", Msg);
}

/* ----------------------------------------------------------------------
 * initializes error log if required by resources
 */
void
InitErrorLog (void)
{
  int fildes[2];

  if (Settings.UseLogWindow)
    {
      /* create a pipe with read end at log window and write end
       * to stderr (desc. 2)
       * make a copy of the original stderr descriptor for fatal errors 
       */
      if (pipe (fildes) == -1)
	Message ("can't create pipe to log window\n");
      else
	{
	  OriginalStderr = dup (2);

	  /* close original stderr and create new one */
	  close (2);
	  if (dup (fildes[1]) == -1)
	    MyFatal ("can't dup pipe to stderr\n");

	  /* select non-blocking IO for stderr */
	  fcntl (2, F_SETFL, O_NONBLOCK);
	  LogWindow = XtWindow (InitLogWindow (Output.Toplevel, fildes[0]));
	}
    }
}
