/* $Id$ */

#ifndef __VRML_H
#define __VRML_H

extern int LoadFootprintByName (BufferType * Buffer, char *Footprint);

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented VRML function %s.\n", __FUNCTION__); abort()
#define CRASH2 fprintf(stderr, "HID error: Internal error in %s.\n", __FUNCTION__); abort()

#define VRML_MAP_EXT ".3dm"
#define VRML_FPMAP_EXT ".fp.3dm"

/*
#define FINAL_SCALE 0.0001

#define VRML_COMPONENT_SCALE	3937

#define BOARD_THICKNESS 		3810
#define OUTER_COPPER_THICKNESS		88
#define INNER_COPPER_THICKNESS		(OUTER_COPPER_THICKNESS / 2)
#define MASK_THICKNESS			(OUTER_COPPER_THICKNESS + 32 )
#define PLATING_COPPER_THICKNESS	INNER_COPPER_THICKNESS
#define SILK_LAYER_OFFSET		( ( BOARD_THICKNESS + SILK_LAYER_THICKNESS ) / 2 )
#define SILK_LAYER_THICKNESS		( OUTER_COPPER_THICKNESS + 10 )
#define SILK_LAYER_OFFSET		( ( BOARD_THICKNESS + SILK_LAYER_THICKNESS ) / 2 )
#define SILK_LAYER_OFFSET2		( ( BOARD_THICKNESS + SILK_LAYER_THICKNESS ) / 2 + MASK_THICKNESS)
#define HOLE_THICKNESS			( BOARD_THICKNESS + 2 * OUTER_COPPER_THICKNESS + 16)
*/


/* nanometer version */
#define FINAL_SCALE 0.000005

#define VRML_COMPONENT_SCALE    1000000

#define BOARD_THICKNESS                 1000000
#define OUTER_COPPER_THICKNESS          35000
#define INNER_COPPER_THICKNESS          (OUTER_COPPER_THICKNESS / 2)
#define MASK_THICKNESS                  (OUTER_COPPER_THICKNESS + 8100 )
#define PLATING_COPPER_THICKNESS        INNER_COPPER_THICKNESS
#define SILK_LAYER_OFFSET               ( ( BOARD_THICKNESS + SILK_LAYER_THICKNESS ) / 2 )
#define SILK_LAYER_THICKNESS            ( OUTER_COPPER_THICKNESS + 2540 )
#define SILK_LAYER_OFFSET               ( ( BOARD_THICKNESS + SILK_LAYER_THICKNESS ) / 2 )
#define SILK_LAYER_OFFSET2              ( ( BOARD_THICKNESS + SILK_LAYER_THICKNESS ) / 2 + MASK_THICKNESS)
#define HOLE_THICKNESS                  ( BOARD_THICKNESS + 2 * OUTER_COPPER_THICKNESS + 4000)

#define VRML_HOLE_COLOR			"0 0 0"
#define VRML_BOARD_COLOR		"0.44 0.44 0"
#define VRML_BOARD_COLOR_MASKED		"0.176 0.416 0.12 shininess 0.7"
#define VRML_BOARD_HOLE_COLOR           "0.1 0.2 0"
#define VRML_SILK_COLOR			"1 1 1"
#define VRML_COPPER_COLOR		"1 0.4 0.2 shininess 0.4"
#define VRML_COPPER_COLOR_MASKED	"0.4 0.4 .02 shininess 0.7"
#define VRML_COPPER_COLOR_TIN		"0.76 0.76 0.76 shininess 0.4"
#define VRML_COPPER_COLOR_TIN_MASKED	"0.304 0.544 0.424 shininess 0.7"
#define VRML_COPPER_COLOR_GOLD		"1 0.85 0.24 shininess 1"
#define VRML_COPPER_COLOR_GOLD_MASKED	"0.44 0.58 0.216 shininess 7"
#define VRML_MASK_COLOR			"0 0.4 0.2 shininess 0.7 transparency 0.4 "

#define VRML_FALLBACK_COLOR_R		255
#define VRML_FALLBACK_COLOR_G   	51
#define VRML_FALLBACK_COLOR_B   	204

#define VRML_LAYER_COLOR_INDEX		0
#define VRML_HOLE_COLOR_INDEX		-1
#define VRML_BOARD_COLOR_INDEX   	-2
#define VRML_SILK_COLOR_INDEX   	-3
#define VRML_COPPER_COLOR_INDEX         -4
#define VRML_MASK_COLOR_INDEX           -5

#define VRML_OUTLINE_NONE	0
#define VRML_OUTLINE_OUTLINE	1
#define VRML_OUTLINE_SHAPE	2
#define VRML_OUTLINE_SIZE	3

#define VRML_COPPER_COPPER	0
#define VRML_COPPER_GOLD	1
#define VRML_COPPER_TIN		2


#define MAX_LAYER_COLORS (MAX_LAYER *2)

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

extern FILE *vrml_output;



typedef struct
{
  int processed;
  int x1, y1, x2, y2;
} t_outline_segment;


#define POLY_CW		1
#define POLY_CCW	2

#define POLY_CONVEX	1
#define POLY_CONCAVE	2

/*******************************************************************************/

typedef struct fplist_struct
{
  char *name;
  char *path;
} fplist_struct;

typedef struct mapvar_struct
{
  char *name;
  char *value;
} mapvar_struct;

typedef struct map3d_struct
{
  mapvar_struct *variables;
  int nvars, mvars;
} map3d_struct;

typedef struct globalvar_struct
{
  char *refdes;
  char *name;
  char *value;
} globalvar_struct;

extern void vrml_process_components ();

#define VRMLBASE "vrml"
#define VRMLMODELS "models"
#define VRMLSCRIPTS "scripts"

#endif
