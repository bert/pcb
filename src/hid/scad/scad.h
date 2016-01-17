/*!
 * \file src/hid/scad.h
 *
 * \brief Header file for OpenSCAD export HID.
 *
 * \author Milan Prochac <milan@prochac.sk>
 *
 * <hr>
 *
 * PCB, interactive printed circuit board design
 *
 * This code is based on the GERBER and VRML export HID
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
 */

#ifndef __SCAD_H
#define __SCAD_H

extern int LoadFootprintByName (BufferType * Buffer, char *Footprint);

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented FreeCAD function %s.\n", __FUNCTION__); abort()
#define CRASH2 fprintf(stderr, "HID error: Internal error in %s.\n", __FUNCTION__); abort()

#define SCAD_EXT ".scad"
#define SCAD_MAP_EXT ".3dm"
#define SCAD_FPMAP_EXT ".fp.3dm"

#define SCADBASE "scad"
#define SCADMODELS "models"
#define SCADSCRIPTS "scripts"



/* dimensions (nanometer version) and colors*/
#define METRIC_SCALE	0.000001

#define SCAD_MIN_OUTLINE_DIST		100000

#define BOARD_THICKNESS			1.6
#define OUTER_COPPER_THICKNESS		0.035
#define INNER_COPPER_THICKNESS		(OUTER_COPPER_THICKNESS / 2.)
#define MASK_THICKNESS			(OUTER_COPPER_THICKNESS + 0.02 )
#define SILK_LAYER_THICKNESS		( OUTER_COPPER_THICKNESS + 0.0025 )
#define SILK_LAYER_OFFSET		( ( BOARD_THICKNESS + SILK_LAYER_THICKNESS ) / 2. )
#define SILK_LAYER_OFFSET2		( ( BOARD_THICKNESS + SILK_LAYER_THICKNESS ) / 2. + MASK_THICKNESS)
#define HOLE_THICKNESS			( BOARD_THICKNESS + 2. * OUTER_COPPER_THICKNESS + 0.1)
#define PLATING_THICKNESS		( BOARD_THICKNESS + 2. * OUTER_COPPER_THICKNESS)
#define HOLE_PLATING			0.0175

#define SCAD_BOARD_COLOR		"[0.44, 0.44, 0]"
#define SCAD_SILK_COLOR			"[1, 1, 1]"
#define SCAD_COPPER_COLOR		"[1, 0.4, 0.2]"
#define SCAD_COPPER_COLOR_TIN		"[0.76, 0.76, 0.76]"
#define SCAD_COPPER_COLOR_GOLD		"[1, 0.85, 0.24]"
#define SCAD_MASK_COLOR_G		"[0, 0.4, 0.2, 0.65]"
#define SCAD_MASK_COLOR_R		"[0.8, 0.1, 0.1, 0.65]"
#define SCAD_MASK_COLOR_B		"[0.1, 0.1, 0.8, 0.65]"


#define SCAD_OUTLINE_NONE	0
#define SCAD_OUTLINE_OUTLINE	1
#define SCAD_OUTLINE_SIZE	2

#define SCAD_COPPER_COPPER	0
#define SCAD_COPPER_GOLD	1
#define SCAD_COPPER_TIN		2

#define SCAD_MASK_NONE		0
#define SCAD_MASK_GREEN		1
#define SCAD_MASK_BLUE		2
#define SCAD_MASK_RED		3

#define MAX_LAYER_COLORS (MAX_LAYER *2)

/* polygon attributes */

#define POLY_CW		1
#define POLY_CCW	2

#define POLY_CONVEX	1
#define POLY_CONCAVE	2

#define SCAD_EL_STANDARD	0
#define SCAD_EL_POLY		1
#define SCAD_EL_LASTPOLY	2

typedef struct color_table_struct
{
  int r, g, b;
} color_table_struct;

typedef struct hid_gc_struct
{
  EndCapStyle cap;
  int width;
  int erase;
  int drill;
  int r, g, b;
  int seq;
} hid_gc_struct;

typedef struct
{
  int processed;
  Coord x1, y1, x2, y2;
} t_outline_segment;

typedef struct fplist_struct
{
  char *name;
  char *path;
} fplist_struct;

/*!
 * \brief Variable and its value, read from map file.
 */
typedef struct mapvar_struct
{
  char *name;
  char *value;
} mapvar_struct;

/*!
 * \brief Variables from mapfiles.
 */
typedef struct map3d_struct
{
  mapvar_struct *variables; /*!< array of variables with values. */
  int nvars; /*!< number of variables: actual. */
  int mvars; /*!< number of variables: allocated. */
} map3d_struct;

/*!
 * \brieg Global variables from project file.
 */
typedef struct projectvar_struct
{
  char *refdes;
  char *name;
  char *value;
} projectvar_struct;

extern FILE *scad_output;

extern void scad_write_prologue ();
extern void scad_generate_holes ();
extern void scad_generate_plating ();
extern void scad_generate_board ();
extern void scad_generate_mask ();
extern float scad_scale_coord (float x);

void scad_process_components (void);


#endif
