/*!
 * \file src/file.h
 *
 * \brief Prototypes for file routines.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 */

#ifndef	PCB_FILE_H
#define	PCB_FILE_H

#include <stdio.h>		/* needed to define 'FILE *' */
#include "global.h"

FILE *CheckAndOpenFile (char *, bool, bool, bool *, bool *);
FILE *OpenConnectionDataFile (void);
int SavePCB (char *);
int LoadPCB (char *);
int RevertPCB (void);
void EnableAutosave (void);
void Backup (void);
void SaveInTMP (void);
void EmergencySave (void);
void DisableEmergencySave (void);
int ReadLibraryContents (void);
int ImportNetlist (char *);
int SaveBufferElements (char *);
void sort_netlist (void);

int PCBFileVersionNeeded (void);
        /*!< This is the version needed by the file we're saving. */

#define PCB_FILE_VERSION 20161008
        /*!< \brief This is the version we support.
         *
         * Whenever the pcb file format is modified, this version number
         * should be updated to the date when the new code is committed.
         * It will be written out to the file and also used by pcb to
         * give guidance to the user as to what the minimum version of
         * pcb required is.
         */

#ifndef HAS_ATEXIT
#ifdef HAS_ON_EXIT
void GlueEmergencySave (int, caddr_t);
#else
void SaveTMPData (void);
void RemoveTMPData (void);
#endif
#endif

#endif
