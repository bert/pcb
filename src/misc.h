/*!
 * \file src/misc.h
 *
 * \brief Prototypes for misc routines.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996,2006 Thomas Nau
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
 *
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *
 * Thomas.Nau@rz.uni-ulm.de
 */

#ifndef	PCB_MISC_H
#define	PCB_MISC_H

#include <stdlib.h>
#include "global.h"
#include "mymem.h"

enum unitflags { UNIT_PERCENT = 1 };

typedef struct {
  const char *suffix;
  double scale;
  enum unitflags flags;
} UnitList[];

double Distance (double x1, double y1, double x2, double y2);
Angle  NormalizeAngle (Angle a);

void r_delete_element (DataType *, ElementType *);
void SetLineBoundingBox (LineType *);
void SetArcBoundingBox (ArcType *);
void SetPointBoundingBox (PointType *);
void SetPinBoundingBox (PinType *);
void SetPadBoundingBox (PadType *);
void SetPolygonBoundingBox (PolygonType *);
void SetElementBoundingBox (DataType *, ElementType *, FontType *);
bool IsDataEmpty (DataType *);
bool IsLayerEmpty (LayerType *);
bool IsLayerNumEmpty (int);
bool IsLayerGroupEmpty (int);
bool IsPasteEmpty (int);
void CountHoles (int *, int *, const BoxType *);
BoxType * GetDataBoundingBox (DataType *);
void CenterDisplay (Coord, Coord, bool warp_pointer);
char *make_route_string (RouteStyleType rs[], int n_styles);
int ParseGroupString (char *, LayerGroupType *, int * /* LayerN */);
int ParseRouteString (char *, RouteStyleType *, const char *);
void QuitApplication (void);
char *EvaluateFilename (char *, char *, char *, char *);
char *ExpandFilename (char *, char *);
void SetTextBoundingBox (TextType *);

void SaveOutputWindow (void);
int GetLayerNumber (DataType *, LayerType *);
int GetLayerGroupNumberByPointer (LayerType *);
int GetLayerGroupNumberByNumber (Cardinal);
int GetLayerGroupNumberBySide (int);
int ChangeGroupVisibility (int, bool, bool);
void LayerStringToLayerStack (char *);


BoxType * GetObjectBoundingBox (int, void *, void *, void *);
void ResetStackAndVisibility (void);
void SaveStackAndVisibility (void);
void RestoreStackAndVisibility (void);
char *GetWorkingDirectory (char *);
void CreateQuotedString (DynamicStringType *, char *);
BoxType * GetArcEnds (ArcType *);
void ChangeArcAngles (LayerType *, ArcType *, Angle, Angle);
char *UniqueElementName (DataType *, char *);
void AttachForCopy (Coord, Coord);
double GetValue (const char *, const char *, bool *);
double GetValueEx (const char *, const char *, bool *, UnitList, const char *);
double GetUnitlessValue (const char *, bool *);
int FileExists (const char *);
char *Concat (const char *, ...);	/* end with NULL */

char *pcb_author ();
char *AttributeGetFromList (AttributeListType *list, char *name);
int AttributePutToList (AttributeListType *list, const char *name, const char *value, int replace);
/* Simplistic version: Takes a pointer to an object, looks up attributes in it.  */
#define AttributeGet(OBJ,name) AttributeGetFromList (&(OBJ->Attributes), name)
/* Simplistic version: Takes a pointer to an object, sets attributes in it.  */
#define AttributePut(OBJ,name,value) AttributePutToList (&(OBJ->Attributes), name, value, 1)
void AttributeRemoveFromList(AttributeListType *list, char *name);
/* Simplistic version of Remove.  */
#define AttributeRemove(OBJ, name) AttributeRemoveFromList (&(OBJ->Attributes), name)

/* For passing modified flags to other functions. */
FlagType MakeFlags (unsigned int);
FlagType OldFlags (unsigned int);
FlagType AddFlags (FlagType, unsigned int);
FlagType MaskFlags (FlagType, unsigned int);
#define		NoFlags() MakeFlags(0)

/* Layer Group Functions */

int MoveLayerToGroup (int layer, int group);
char *LayerGroupsToString (LayerGroupType *);
/*! \todo is this an orphan prototype ?
 *
 * Make the current layer groups the default.
 */
void MakeLayerGroupsDefault ();

extern const char *c_dtostr(double d);
char * GetInfoString (void);
void AssignDefaultLayerTypes ();
int ElementOrientation (ElementType *e);

/*!
 * \todo These are in netlist.c
 */
void NetlistChanged (int force_unfreeze);

/*
 * Check whether mkdir() is mkdir or _mkdir, and whether it takes one
 * or two arguments.  WIN32 mkdir takes one argument and POSIX takes
 * two.
 */
#if HAVE_MKDIR
        #if MKDIR_TAKES_ONE_ARG
         /* MinGW32 */
#include <io.h> /* mkdir under MinGW only takes one argument */
         #define MKDIR(a, b) mkdir(a)
        #else
         #define MKDIR(a, b) mkdir(a, b)
        #endif
#else
        #if HAVE__MKDIR
         /* plain Windows 32 */
         #define MKDIR(a, b) _mkdir(a)
        #else
         #define MKDIR(a, b) pcb_mkdir(a, b)
         #define MKDIR_IS_PCBMKDIR 1
         int pcb_mkdir (const char *path, int mode);
        #endif
#endif


#endif /* PCB_MISC_H */

