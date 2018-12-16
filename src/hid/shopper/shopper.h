/*!
 * \file src/hid/shopper/shopper.h
 *
 * \brief Header file for the pcb shopper Exporter
 * (https://pcbshopper.com/).
 *
 * The idea of the ULP code is adapted for use as an exporter by this
 * inter-active pcb layout editor and this adaptation is re-licensed
 * under the GPL v2+ license.
 *
 * \author Copyright (C) 2018 for adaptation by Bert Timmerman
 * <bert.timmerman@xs4all.nl>
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
 *
 * Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact addresses for paper mail and Email:
 * Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 * haceaton@aplcomm.jhuapl.edu
 */

#ifndef	PCB_SHOPPER_H
#define	PCB_SHOPPER_H

#include "global.h"

extern void shopper_hid_export_to_file (FILE *, HID_Attr_Val *);

#endif
