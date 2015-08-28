/*!
 * \file src/ascii-printf.h
 *
 * \brief Prototypes for ASCII printf() replacements.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef PCB_ASCII_PRINTF_H
#define PCB_ASCII_PRINTF_H

#include <stdio.h>
#include <stdarg.h>


int ascii_fprintf(FILE *stream, const char *format, ...);
int ascii_snprintf(char *str, size_t size, const char *format, ...);


#ifdef PCB_UNIT_TEST
void ascii_printf_register_tests();
void ascii_printf_test_printf();
#endif

#endif /* PCB_ASCII_PRINTF_H */

