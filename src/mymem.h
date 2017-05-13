/*!
 * \file src/mymem.h
 *
 * \brief Prototypes for memory routines.
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
 *
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *
 * Thomas.Nau@rz.uni-ulm.de
 */

#ifndef	PCB_MYMEM_H
#define	PCB_MYMEM_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include "global.h"

/* ---------------------------------------------------------------------------
 * number of additional objects that are allocated with one system call
 */
#define	STEP_ELEMENT		50
#define STEP_DRILL		30
#define STEP_POINT		100
#define	STEP_SYMBOLLINE		10
#define	STEP_SELECTORENTRY	128
#define	STEP_REMOVELIST		500
#define	STEP_UNDOLIST		500
#define	STEP_POLYGONPOINT	10
#define	STEP_POLYGONHOLEINDEX	10
#define	STEP_LIBRARYMENU	10
#define	STEP_LIBRARYENTRY	20
#define	STEP_RUBBERBAND		100

#define STRDUP(x) (((x) != NULL) ? strdup (x) : NULL)

/*!
 * \brief Dynamic string type.
 */
typedef struct
{
  size_t MaxLength;
  char *Data;
} DynamicStringType;

RubberbandType * GetRubberbandMemory (void);
PinType * GetPinMemory (ElementType *);
PadType * GetPadMemory (ElementType *);
PinType * GetViaMemory (DataType *);
LineType * GetLineMemory (LayerType *);
ArcType * GetArcMemory (LayerType *);
RatType * GetRatMemory (DataType *);
TextType * GetTextMemory (LayerType *);
PolygonType * GetPolygonMemory (LayerType *);
PointType * GetPointMemoryInPolygon (PolygonType *);
Cardinal *GetHoleIndexMemoryInPolygon (PolygonType *);
ElementType * GetElementMemory (DataType *);
BoxType * GetBoxMemory (BoxListType *);
ConnectionType * GetConnectionMemory (NetType *);
NetType * GetNetMemory (NetListType *);
NetListType * GetNetListMemory (NetListListType *);
LibraryMenuType * GetLibraryMenuMemory (LibraryType *);
LibraryEntryType * GetLibraryEntryMemory (LibraryMenuType *);
ElementType **GetDrillElementMemory (DrillType *);
PinType ** GetDrillPinMemory (DrillType *);
DrillType * GetDrillInfoDrillMemory (DrillInfoType *);
void **GetPointerMemory (PointerListType *);
void FreePolygonMemory (PolygonType *);
void FreeElementMemory (ElementType *);
void FreePCBMemory (PCBType *);
void FreeBoxListMemory (BoxListType *);
void FreeNetListListMemory (NetListListType *);
void FreeNetListMemory (NetListType *);
void FreeNetMemory (NetType *);
void FreeDataMemory (DataType *);
void FreeLibraryMemory (LibraryType *);
void FreePointerListMemory (PointerListType *);
void DSAddCharacter (DynamicStringType *, char);
void DSAddString (DynamicStringType *, const char *);
void DSClearString (DynamicStringType *);
char *StripWhiteSpaceAndDup (const char *);

#ifdef NEED_STRDUP
char *strdup (const char *);
#endif

#ifndef HAVE_LIBDMALLOC
#define malloc(x) calloc(1,(x))
#endif

#endif
