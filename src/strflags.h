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

#ifndef PCB_STRFLAGS_H
#define PCB_STRFLAGS_H

/* The purpose of this interface is to make the file format able to
   handle more than 32 flags, and to hide the internal details of
   flags from the file format.  */

/* When passed a string, parse it and return an appropriate set of
   flags.  Errors cause error() to be called with a suitable message;
   if error is NULL, errors are ignored.  */
FlagType string_to_flags (const char *flagstring,
			  int (*error) (const char *msg));

/* Given a set of flags for a given object type, return a string which
   can be output to a file.  The returned pointer must not be
   freed.  */
char *flags_to_string (FlagType flags, int object_type);

/* Same as above, but for pcb flags.  */
FlagType string_to_pcbflags (const char *flagstring,
			  int (*error) (const char *msg));
char *pcbflags_to_string (FlagType flags);

#endif
