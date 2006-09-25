/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *  RCS: $Id$
 */

/* prototypes for memory routines
 */

#ifndef	__MYMEM_INCLUDED__
#define	__MYMEM_INCLUDED__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include "global.h"

/* ---------------------------------------------------------------------------
 * number of additional objects that are allocated with one system call
 */
#define	STEP_VIA			2000	/* block size for realloc() calls */
#define	STEP_PIN			20
#define	STEP_PAD			20
#define	STEP_LINE			10000
#define STEP_ARC			1000
#define	STEP_ELEMENTLINE	10
#define	STEP_ELEMENTARC		5
#define	STEP_TEXT			50
#define	STEP_ELEMENT		50
#define STEP_DRILL		30
#define STEP_POINT		100
#define	STEP_SYMBOLLINE		10
#define	STEP_SELECTORENTRY	128
#define	STEP_REMOVELIST		500
#define	STEP_UNDOLIST		500
#define	STEP_POLYGON		10
#define	STEP_POLYGONPOINT	10
#define	STEP_LIBRARYMENU	10
#define	STEP_LIBRARYENTRY	20
#define	STEP_RUBBERBAND		100
#define STEP_RAT		2000

/* ---------------------------------------------------------------------------
 * some memory types
 */
typedef struct
{
  size_t MaxLength;
  char *Data;
} DynamicStringType, *DynamicStringTypePtr;

RubberbandTypePtr GetRubberbandMemory (void);
PinTypePtr GetPinMemory (ElementTypePtr);
PadTypePtr GetPadMemory (ElementTypePtr);
PinTypePtr GetViaMemory (DataTypePtr);
LineTypePtr GetLineMemory (LayerTypePtr);
ArcTypePtr GetArcMemory (LayerTypePtr);
RatTypePtr GetRatMemory (DataTypePtr);
TextTypePtr GetTextMemory (LayerTypePtr);
PolygonTypePtr GetPolygonMemory (LayerTypePtr);
PointTypePtr GetPointMemoryInPolygon (PolygonTypePtr);
ElementTypePtr GetElementMemory (DataTypePtr);
BoxTypePtr GetBoxMemory (BoxListTypePtr);
ConnectionTypePtr GetConnectionMemory (NetTypePtr);
NetTypePtr GetNetMemory (NetListTypePtr);
NetListTypePtr GetNetListMemory (NetListListTypePtr);
LibraryMenuTypePtr GetLibraryMenuMemory (LibraryTypePtr);
LibraryEntryTypePtr GetLibraryEntryMemory (LibraryMenuTypePtr);
ElementTypeHandle GetDrillElementMemory (DrillTypePtr);
PinTypeHandle GetDrillPinMemory (DrillTypePtr);
DrillTypePtr GetDrillInfoDrillMemory (DrillInfoTypePtr);
void **GetPointerMemory (PointerListTypePtr);
void *MyCalloc (size_t, size_t, const char *);
void *MyMalloc (size_t, const char *);
void *MyRealloc (void *, size_t, const char *);
char *MyStrdup (char *s, const char *);
/* void MyFree (void **); */
#define MYFREE(x) do { SaveFree(x); (x)=NULL; } while (0)
void FreePolygonMemory (PolygonTypePtr);
void FreeElementMemory (ElementTypePtr);
void FreePCBMemory (PCBTypePtr);
void FreeBoxListMemory (BoxListTypePtr);
void FreeNetListListMemory (NetListListTypePtr);
void FreeNetListMemory (NetListTypePtr);
void FreeNetMemory (NetTypePtr);
void FreeDataMemory (DataTypePtr);
void FreeLibraryMemory (LibraryTypePtr);
void FreePointerListMemory (PointerListTypePtr);
void SaveFree (void *);
void DSAddCharacter (DynamicStringTypePtr, char);
void DSAddString (DynamicStringTypePtr, const char *);
void DSClearString (DynamicStringTypePtr);
char *StripWhiteSpaceAndDup (char *);

#ifdef NEED_STRDUP
char *strdup (const char *);
#endif

#ifndef HAVE_LIBDMALLOC
#define malloc(x) calloc(1,(x))
#endif

#endif
