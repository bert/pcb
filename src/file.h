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

/* prototypes for file routines
 */

#ifndef	__FILE_INCLUDED__
#define	__FILE_INCLUDED__

#include <stdio.h>		/* needed to define 'FILE *' */
#include "global.h"

FILE *CheckAndOpenFile (char *, Boolean, Boolean, Boolean *, Boolean *);
FILE *OpenConnectionDataFile (void);
int SavePCB (char *);
int LoadPCB (char *);
void Backup (void);
void SaveInTMP (void);
void EmergencySave (void);
int ReadLibraryContents (void);
int ReadNetlist (char *);
int SaveBufferElements (char *);

/* 
 * Whenever the pcb file format is modified, this version number
 * should be updated to the date when the new code is committed.
 * It will be written out to the file and also used by pcb to give
 * guidance to the user as to what the minimum version of pcb required
 * is.
 */
#define PCB_FILE_VERSION 20061106


#ifndef HAS_ATEXIT
#ifdef HAS_ON_EXIT
void GlueEmergencySave (int, caddr_t);
#else
void SaveTMPData (void);
void RemoveTMPData (void);
#endif
#endif

#endif
