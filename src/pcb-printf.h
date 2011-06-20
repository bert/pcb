/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2011 Andrew Poelstra
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
 *  Andrew Poelstra, 16966 60A Ave, V3S 8X5 Surrey, BC, Canada
 *  asp11@sfu.ca
 *
 */

/* This file defines a wrapper around sprintf, that
 *  defines new specifiers that take pcb BDimension
 *  objects as input.
 *
 * There is a fair bit of nasty (repetitious) code in
 *  here, but I feel the gain in clarity for output
 *  code elsewhere in the project will make it worth
 *  it.
 *
 * The new specifiers are:
 *   %mm    output a measure in mm
 *   %mM    output a measure in scaled (mm/um) metric
 *   %ml    output a measure in mil
 *   %mL    output a measure in scaled (mil/in) imperial
 *   %ms    output a measure in most natural mm/mil units
 *   %mS    output a measure in most natural scaled units
 *   %md    output a pair of measures in most natural mm/mil units
 *   %mD    output a pair of measures in most natural scaled units
 *   %m3    output 3 measures in most natural scaled units
 *     ...
 *   %m9    output 9 measures in most natural scaled units
 *   %m*    output a measure with unit given as an additional
 *          const char* parameter
 *   %m+    accepts an e_allow parameter that masks all subsequent
 *          "natural" (S/D/3/.../9) specifiers to only use certain
 *          units
 *   %mr    output a measure in a unit readable by parse_l.l
 *          (this will always append a unit suffix)
 *
 * These accept the usual printf modifiers for %f,
 *  as well as the additional modifier $ which is
 *  used to output a unit suffix after the measure.
 *
 * KNOWN ISSUES:
 *   No support for %zu size_t printf spec
 */

#ifndef	__PCB_PRINTF_INCLUDED__
#define	__PCB_PRINTF_INCLUDED__

enum e_allow {
  ALLOW_NM = 1,
  ALLOW_UM = 2,
  ALLOW_MM = 4,
  ALLOW_CM = 8,
  ALLOW_M  = 16,
  ALLOW_KM = 32,

  ALLOW_CMIL = 1024,
  ALLOW_MIL  = 2048,
  ALLOW_IN   = 4096,

  ALLOW_METRIC   = ALLOW_NM | ALLOW_UM | ALLOW_MM |
                   ALLOW_CM | ALLOW_M  | ALLOW_KM,
  ALLOW_IMPERIAL = ALLOW_CMIL | ALLOW_MIL | ALLOW_IN,
  /* This is all units allowed in parse_l.l */
  ALLOW_READABLE = ALLOW_NM | ALLOW_UM | ALLOW_MM |
                   ALLOW_M  | ALLOW_KM | ALLOW_CMIL |
                   ALLOW_MIL | ALLOW_IN,

  ALLOW_ALL = ~0
};

int pcb_fprintf(FILE *f, const char *fmt, ...);
int pcb_sprintf(char *string, const char *fmt, ...);
char *pcb_g_strdup_printf(const char *fmt, ...);

#endif

