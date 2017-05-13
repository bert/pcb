/*!
 * \file src/layerflags.h
 *
 * \brief Prototypes for changing layer flags.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2007 DJ Delorie <dj@delorie.com>
 *
 * Copyright (C) 2015 Markus "Traumflug" Hitter <mah@jump-ing.de>
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
 */

#ifndef PCB_LAYERFLAGS_H
#define PCB_LAYERFLAGS_H

unsigned int string_to_layertype (const char *typestring,
                                  int (*error) (const char *msg));
const char *layertype_to_string (unsigned int type);
unsigned int guess_layertype (const char *name,
                              int layer_number,
                              DataType *data);

#endif /* PCB_LAYERFLAGS_H */
