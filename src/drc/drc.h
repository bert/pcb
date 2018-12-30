/*!
 * \file src/drc.h
 *
 * \brief DRC related structures and functions
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
#ifndef PCB_DRC_H
#define PCB_DRC_H

#include "object_list.h" /* object_list */


/* This list keeps track of DRCViolations */
extern object_list * drc_violation_list;

/* There are two "things", which are just sets of object pointers used to
 * keep track of objects. Thing 1 should be set to the "seed" object, and
 * thing 2 should be set to the found object. 
 *
 * This is an ugly hack that needs to go away eventually. 
 */
bool SetThing(int n, int type, void *p1, void *p2, void *p3);

#endif /* PCB_DRC_H */
