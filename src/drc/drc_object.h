/*!
 * \file src/drc_object.h
 *
 * \brief A data structure that contains info about an object found by the DRC.
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
#ifndef PCB_DRC_OBJECT_H
#define PCB_DRC_OBJECT_H

 
/*
 * DRCObject
 *
 * An object identified by the DRC. 
 *
 */
typedef struct drc_object_st {
  long int id;
  int type;
  void *ptr1, *ptr2, *ptr3;
} DRCObject;

#endif /* PCB_DRC_OBJECT_H */
