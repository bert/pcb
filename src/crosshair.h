/*!
 * \file src/crosshair.h
 *
 * \brief Prototypes for crosshair routines.
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

#ifndef	PCB_CROSSHAIR_H
#define	PCB_CROSSHAIR_H

#include "global.h"

/* ---------------------------------------------------------------------------
 * all possible states of an attached object
 */
#define	STATE_FIRST		0	/*!< initial state. */
#define	STATE_SECOND	1
#define	STATE_THIRD		2

/* ---------------------------------------------------------------------------
 * some types for cursor drawing, setting of block and lines
 * as well as for merging of elements
 */

/*!
 * \brief Rubberband lines for element moves.
 */
typedef struct
{
  LayerType *Layer; /*!< Layer that holds the line. */
  LineType *Line; /*!< The line itself. */
  PointType *MovedPoint; /*!< And finally the point. */
} RubberbandType;

/*!
 * \brief Current marked line.
 */
typedef struct
{
  PointType Point1; /*!< Start position. */
  PointType Point2; /*!< End position. */
  long int State;
  bool draw;
} AttachedLineType;

/*!
 * \brief Currently marked block.
 */
typedef struct
{
  PointType Point1; /*!< Start position. */
  PointType Point2; /*!< End position. */
  long int State;
  bool otherway;
} AttachedBoxType;

/*!
 * \brief Currently attached object.
 */
typedef struct
{
  Coord X; /*!< Saved position when MOVE_MODE (X value). */
  Coord Y; /*!< Saved position when MOVE_MODE (Y value). */
  BoxType BoundingBox;
  long int Type, /*!< Object type. */
  State;
  void *Ptr1; /*!< Pointer to data, see search.c. */
  void *Ptr2; /*!< Pointer to data, see search.c. */
  void *Ptr3; /*!< Pointer to data, see search.c. */
  Cardinal RubberbandN, /*!< Number of lines in array. */
  RubberbandMax;
  RubberbandType *Rubberband;
} AttachedObjectType;

enum crosshair_shape
{
  Basic_Crosshair_Shape = 0,  /*!< 4-ray. */
  Union_Jack_Crosshair_Shape, /*!< 8-ray. */
  Dozen_Crosshair_Shape,      /*!< 12-ray. */
  Crosshair_Shapes_Number
};

/*!
 * \brief Holds cursor information.
 */
typedef struct
{
  hidGC GC; /*!< GC for cursor drawing. */
  hidGC AttachGC; /*!< GC for displaying buffer contents. */
  Coord X; /*!< Position in PCB coordinates (X value). */
  Coord Y; /*!< Position in PCB coordinates (Y value). */
  Coord MinX; /*!< Lowest coordinates (X value). */
  Coord MinY; /*!< Lowest coordinates (Y value). */
  Coord MaxX; /*!< Highest coordinates (X value). */
  Coord MaxY; /*!< Highest coordinates (Y value). */
  AttachedLineType AttachedLine; /*!< Data of new lines. */
  AttachedBoxType AttachedBox;
  PolygonType AttachedPolygon;
  AttachedObjectType AttachedObject; /*!< Data of attached objects. */
  enum crosshair_shape shape; /*!< Shape of Crosshair. */
} CrosshairType;

typedef struct
{
  bool status;
  Coord X, Y;
} MarkType;

Coord GridFit (Coord x, Coord grid_spacing, Coord grid_offset);
void notify_crosshair_change (bool changes_complete);
void notify_mark_change (bool changes_complete);
void HideCrosshair (void);
void RestoreCrosshair (void);
void DrawAttached (hidGC gc);
void DrawMark (hidGC gc);
bool MoveCrosshairAbsolute (Coord, Coord);
void SetCrosshairRange (Coord, Coord, Coord, Coord);
void InitCrosshair (void);
void DestroyCrosshair (void);
void FitCrosshairIntoGrid (Coord, Coord);
void crosshair_update_range(void);

#endif
