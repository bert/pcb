/*!
 * \file src/flags.h
 *
 * \brief Some commonly used macros not related to a special C-file.
 *
 * The file is included by global.h after const.h
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
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
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 */

#ifndef	PCB_FLAGS_H
#define	PCB_FLAGS_H

#include <stdbool.h>
#include "globalconst.h"
/*!
 * \brief Nobody should know about the internals of this except the
 * macros below that access it.
 *
 * This structure must be simple-assignable for now.
 */
typedef struct
{
  unsigned long f;		/* generic flags */
  unsigned char t[(MAX_LAYER + 1) / 2];  /* thermals */
} FlagType;

int pcb_flag_eq (FlagType *f1, FlagType *f2);


/* ---------------------------------------------------------------------------
 * some routines for flag setting, clearing, changing and testing
 */
#define	SET_FLAG(F,P)		((P)->Flags.f |= (F))
#define	CLEAR_FLAG(F,P)		((P)->Flags.f &= (~(F)))
#define	TEST_FLAG(F,P)		((P)->Flags.f & (F) ? 1 : 0)
#define	TOGGLE_FLAG(F,P)	((P)->Flags.f ^= (F))
#define	ASSIGN_FLAG(F,V,P)	((P)->Flags.f = ((P)->Flags.f & (~(F))) | ((V) ? (F) : 0))
#define TEST_FLAGS(F,P)         (((P)->Flags.f & (F)) == (F) ? 1 : 0)

#define FLAGS_EQUAL(F1,F2)	pcb_flag_eq(&(F1), &(F2))

#define THERMFLAG(L)		(0xf << (4 *((L) % 2)))

#define TEST_THERM(L,P)		((P)->Flags.t[(L)/2] & THERMFLAG(L) ? 1 : 0)
#define GET_THERM(L,P)		(((P)->Flags.t[(L)/2] >> (4 * ((L) % 2))) & 0xf) 
#define CLEAR_THERM(L,P)	(P)->Flags.t[(L)/2] &= ~THERMFLAG(L)
#define ASSIGN_THERM(L,V,P)	(P)->Flags.t[(L)/2] = ((P)->Flags.t[(L)/2] & ~THERMFLAG(L)) | ((V)  << (4 * ((L) % 2)))

//defined in misc.c
extern int mem_any_set (unsigned char *, int);
#define TEST_ANY_THERMS(P)	mem_any_set((P)->Flags.t, sizeof((P)->Flags.t))

/* For passing modified flags to other functions. */
FlagType MakeFlags (unsigned int);
FlagType OldFlags (unsigned int);
FlagType AddFlags (FlagType, unsigned int);
FlagType MaskFlags (FlagType, unsigned int);
#define    NoFlags() MakeFlags(0)

bool ClearFlagOnLinesAndPolygons (bool, int flag);
bool ClearFlagOnPinsViasAndPads (bool, int flag);
bool ClearFlagOnAllObjects (bool, int flag);

#endif  // ifndef PCB_FLAGS_H
