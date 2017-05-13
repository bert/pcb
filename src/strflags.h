/*!
 * \file src/strflags.h
 *
 * \brief Prototypes for strflags.
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

#ifndef PCB_STRFLAGS_H
#define PCB_STRFLAGS_H

FlagType string_to_flags (const char *flagstring,
			  int (*error) (const char *msg));

char *flags_to_string (FlagType flags, int object_type);

FlagType string_to_pcbflags (const char *flagstring,
			  int (*error) (const char *msg));
char *pcbflags_to_string (FlagType flags);
void uninit_strflags_buf (void);
void uninit_strflags_layerlist (void);

#endif /* PCB_STRFLAGS_H */
