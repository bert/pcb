/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *	rats.c, rats.h Copyright (C) 1997, harry eaton
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
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* prototypes for rats routines
 */

#ifndef	PCB_RATS_H
#define	PCB_RATS_H

#include "global.h"

/* This one is actually in netlist.h, but it's used by rats.c */
LibraryMenuTypePtr netnode_to_netname (char *nodename);
LibraryMenuTypePtr netname_to_netname (char *netname);

RatTypePtr AddNet (void);
char *ConnectionName (int, void *, void *);

bool AddAllRats (bool, void (*)(register ConnectionTypePtr, register ConnectionTypePtr, register RouteStyleTypePtr));
bool SeekPad (LibraryEntryTypePtr, ConnectionTypePtr, bool);

NetListTypePtr ProcNetlist (LibraryTypePtr);
NetListListType CollectSubnets (bool);

#endif
