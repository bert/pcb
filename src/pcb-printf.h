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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Andrew Poelstra, 16966 60A Ave, V3S 8X5 Surrey, BC, Canada
 *  asp11@sfu.ca
 *
 */

/* This file defines a wrapper around sprintf, that
 *  defines new specifiers that take pcb Coord objects
 *  as input.
 *
 * There is a fair bit of nasty (repetitious) code in
 *  here, but I feel the gain in clarity for output
 *  code elsewhere in the project will make it worth
 *  it.
 *
 * The new specifiers are:
 *   %mk    output a measure in km
 *   %mf    output a measure in meters
 *   %me    output a measure in cm
 *   %mm    output a measure in mm
 *   %mu    output a measure in um
 *   %mn    output a measure in nm
 *   %mM    output a measure in scaled (mm/um) metric
 *   %ml    output a measure in mil
 *   %mc    output a measure in cmil
 *   %mt    output a measure in 1/10 of mils (for Excellon drill files)
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
 *   %ma    output an angle in degrees (expects degrees)
 *
 * These accept the usual printf modifiers for %f, as well as
 *     $    output a unit suffix after the measure
 *     #    prevents all scaling for %mS/D/1/.../9 (this should
 *          ONLY be used for debug code since its output exposes
 *          pcb's base units).
 *     `    always use '.' as decimal separator (note that %mr uses
 *          this by default).
 *
 * KNOWN ISSUES:
 *   No support for %zu size_t printf spec
 */

#ifndef	PCB_PCB_PRINTF_H
#define	PCB_PCB_PRINTF_H

enum e_allow {
  NO_PRINT = 0,		/* suffixes we can read but not print (i.e., "inch") */
  ALLOW_NM = 1,
  ALLOW_UM = 2,
  ALLOW_MM = 4,
  ALLOW_CM = 8,
  ALLOW_M  = 16,
  ALLOW_KM = 32,

  ALLOW_CMIL = 1024,
  ALLOW_DMIL = 2048,
  ALLOW_MIL  = 4096,
  ALLOW_IN   = 8192,

  ALLOW_METRIC   = ALLOW_NM | ALLOW_UM | ALLOW_MM |
                   ALLOW_CM | ALLOW_M  | ALLOW_KM,
  ALLOW_IMPERIAL = ALLOW_CMIL | ALLOW_DMIL | ALLOW_MIL | ALLOW_IN,
  /* This is all units allowed in parse_l.l */
#if 0
  ALLOW_READABLE = ALLOW_NM | ALLOW_UM | ALLOW_MM |
                   ALLOW_M  | ALLOW_KM | ALLOW_CMIL |
                   ALLOW_MIL | ALLOW_IN,
#else
  ALLOW_READABLE = ALLOW_MIL | ALLOW_MM,
#endif

  ALLOW_ALL = ~ALLOW_DMIL
};

enum e_family { METRIC, IMPERIAL };
enum e_suffix {
  NO_SUFFIX,			/* no suffix  */
  SUFFIX,			/* suffix, prefixed with ' ' */
  FILE_MODE_NO_SUFFIX,		/* no suffix, force '.' as decimal */
  FILE_MODE			/* suffix, force '.' as decimal */
};

struct unit {
  int index;			/* Index into Unit[] list */
  const char *suffix;
  const char *in_suffix;	/* internationalized suffix */
  char printf_code;
  double scale_factor;
  enum e_family family;
  enum e_allow  allow;
  int default_prec;
  /* used for gui spinboxes */
  double step_tiny;
  double step_small;
  double step_medium;
  double step_large;
  double step_huge;
  /* aliases -- right now we only need 1 ("inch"->"in"), add as needed */
  const char *alias[1];
};

struct increments {
  const char *suffix;
  /* key g and <shift>g value  */
  Coord grid;
  Coord grid_min;
  Coord grid_max;
  /* key s and <shift>s value  */
  Coord size;
  Coord size_min;
  Coord size_max;
  /* key l and <shift>l value  */
  Coord line;
  Coord line_min;
  Coord line_max;
  /* key k and <shift>k value  */
  Coord clear;
  Coord clear_min;
  Coord clear_max;
};

void initialize_units();

const Unit *get_unit_struct (const char *suffix);
const Unit *get_unit_list (void);
int get_n_units (void);
double coord_to_unit (const Unit *, Coord);
Coord  unit_to_coord (const Unit *, double);
Increments *get_increments_struct (enum e_family family);
void copy_nonzero_increments (Increments *dst, const Increments *src);
enum e_allow set_allow_readable(enum e_allow new_mask);

int pcb_fprintf(FILE *f, const char *fmt, ...);
int pcb_snprintf(char *string, size_t size, const char *fmt, ...);
int pcb_printf(const char *fmt, ...);
char *pcb_g_strdup_printf(const char *fmt, ...);
gchar *pcb_vprintf(const char *fmt, va_list args);

#ifdef PCB_UNIT_TEST
void pcb_printf_register_tests ();
void pcb_printf_test_unit ();
void pcb_printf_test_printf ();
#endif

#endif

