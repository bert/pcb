/*!
 * \file src/drc_violation.h
 *
 * \brief Class for storing data about DRC violations
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2018 Charles Parker <parker.charles@gmail.com>
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

#ifndef PCB_DRC_VIOLATION_H
#define PCB_DRC_VIOLATION_H

#include "object_list.h" /* object_operations */

typedef struct drc_violation_st
{
  char *title;
  char *explanation;
  Coord x, y;
  Angle angle;
  int have_measured;
  Coord measured_value;
  Coord required_value;
  int object_count;
  long int *object_id_list;
  int *object_type_list;
} DrcViolationType;

DrcViolationType * pcb_drc_violation_new (
		const char *title, const char *explanation,
        Coord x, Coord y, Angle angle,                
		bool have_measured,
		Coord measured_value, Coord required_value,
		int object_count, long int *object_id_list, int *object_type_list);

void pcb_drc_violation_free (DrcViolationType *violation);
void pcb_drc_violation_print(FILE*, DrcViolationType*);

extern object_operations drc_violation_ops;


#endif
