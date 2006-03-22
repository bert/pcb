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

/* prototypes for misc routines
 */

#ifndef	__MISC_INCLUDED__
#define	__MISC_INCLUDED__

#include <stdlib.h>
#include "global.h"
#include "mymem.h"

void SetLineBoundingBox (LineTypePtr);
void SetArcBoundingBox (ArcTypePtr);
void SetPointBoundingBox (PointTypePtr);
void SetPinBoundingBox (PinTypePtr);
void SetPadBoundingBox (PadTypePtr);
void SetPolygonBoundingBox (PolygonTypePtr);
void SetElementBoundingBox (DataTypePtr, ElementTypePtr, FontTypePtr);
Boolean IsDataEmpty (DataTypePtr);
BoxTypePtr GetDataBoundingBox (DataTypePtr);
void CenterDisplay (LocationType, LocationType, Boolean);
void SetFontInfo (FontTypePtr);
int ParseGroupString (char *, LayerGroupTypePtr);
int ParseRouteString (char *, RouteStyleTypePtr, int);
char *build_route_string (RouteStyleType *);
void QuitApplication (void);
char *EvaluateFilename (char *, char *, char *, char *);
char *ExpandFilename (char *, char *);
void SetTextBoundingBox (FontTypePtr, TextTypePtr);

void SaveOutputWindow (void);
int GetLayerNumber (DataTypePtr, LayerTypePtr);
int GetLayerGroupNumberByPointer (LayerTypePtr);
int GetLayerGroupNumberByNumber (Cardinal);
int GetGroupOfLayer (int);
int ChangeGroupVisibility (int, Boolean, Boolean);


BoxTypePtr GetObjectBoundingBox (int, void *, void *, void *);
void ResetStackAndVisibility (void);
void SaveStackAndVisibility (void);
void RestoreStackAndVisibility (void);
char *GetWorkingDirectory (char *);
void CreateQuotedString (DynamicStringTypePtr, char *);
int GetGridFactor (void);
BoxTypePtr GetArcEnds (ArcTypePtr);
void ChangeArcAngles (LayerTypePtr, ArcTypePtr, long int, long int);
char *UniqueElementName (DataTypePtr, char *);
void AttachForCopy (LocationType, LocationType);
float GetValue (char *, char *, Boolean *);
int FileExists (const char *);
char *Concat (const char *, ...);	/* end with NULL */

char *pcb_author ();

/* For passing modified flags to other functions. */
FlagType MakeFlags (unsigned int);
FlagType OldFlags (unsigned int);
FlagType AddFlags (FlagType, unsigned int);
FlagType MaskFlags (FlagType, unsigned int);
#define		NoFlags() MakeFlags(0)

/* Layer Group Functions */

/* Returns group actually moved to (i.e. either group or previous) */
int MoveLayerToGroup (int layer, int group);
/* returns pointer to private buffer */
char *LayerGroupsToString (LayerGroupTypePtr);
/* Make the current layer groups the default.  */
void MakeLayerGroupsDefault ();

#endif
