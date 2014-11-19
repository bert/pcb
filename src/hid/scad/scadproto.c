/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *
 *  OpenSCAD export HID
 *  This code is based on the GERBER and VRML export HID
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>

#include <time.h>

#include "config.h"
#include "global.h"
#include "data.h"
#include "misc.h"
#include "error.h"
#include "buffer.h"
#include "create.h"

#include "hid.h"
#include "hid_draw.h"
#include "../hidint.h"
#include "hid/common/hidnogui.h"
#include "hid/common/draw_helpers.h"
#include "hid/common/hidinit.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#include "scad.h"


void
scad_write_prologue ()
{
  fputs ("//SCAD\n\n", scad_output);

  fputs
    ("module line_segment_r(length, width, thickness, x, y, a, bd, c1, c2) {\n"
     "\ttranslate([x,y,0]) rotate ([0,0,a]) union() {\n"
     "\t\tif (bd) {cube ([length, width,thickness],true);}\n"
     "\t\tif (c2) {translate([length/2.,0,0]) cylinder(h=thickness, r=width/2,center=true,$fn=30);}\n"
     "\t\tif (c1) { translate([-length/2.,0,0]) cylinder(h=thickness, r=width/2,center=true,$fn=30);}\n"
     "\t}\n" "}\n\n", scad_output);

  fputs ("module line_segment(length, width, thickness, x, y, a) {\n"
	 "\ttranslate([x,y,0]) rotate ([0,0,a]) {\n"
	 "\t\tcube ([length, width,thickness],true);\n" "\t}\n" "}\n\n",
	 scad_output);
}

void
scad_generate_holes ()
{
  fprintf (scad_output, "module all_holes() {\n\tplating=%f;\n",
	   HOLE_PLATING);
  fprintf (scad_output, "\tunion () {\n");
  fprintf (scad_output, "\t\tfor (i = layer_pdrill_list) {\n");
  fprintf (scad_output,
	   "\t\t\ttranslate([i[1][0],i[1][1],0]) cylinder(r=i[0]+2*plating, h=%f, center=true, $fn=30);\n",
	   HOLE_THICKNESS);
  fprintf (scad_output, "\t\t}\n");
  fprintf (scad_output, "\t\tfor (i = layer_udrill_list) {\n");
  fprintf (scad_output,
	   "\t\t\ttranslate([i[1][0],i[1][1],0]) cylinder(r=i[0], h=%f, center=true, $fn=30);\n",
	   HOLE_THICKNESS);
  fprintf (scad_output, "\t\t}\n");
  fprintf (scad_output, "\t}\n");
  fprintf (scad_output, "}\n\n");
}

void
scad_generate_plating ()
{
  fprintf (scad_output, "module all_plating() {\n");
  fprintf (scad_output, "\tplating=%f;\n", HOLE_PLATING + 0.02);
  fprintf (scad_output, "\tunion () {\n");
  fprintf (scad_output, "\t\tfor (i = layer_pdrill_list) {\n");
  fprintf (scad_output,
	   "\t\t\ttranslate([i[1][0],i[1][1],0]) difference () {\n");
  fprintf (scad_output,
	   "\t\t\t\tcylinder(r=i[0]+2*plating, h=%f, center=true, $fn=30);\n",
	   PLATING_THICKNESS + 0.01);
  fprintf (scad_output,
	   "\t\t\t\tcylinder(r=i[0]-0.01, h=%f, center=true, $fn=30);\n",
	   PLATING_THICKNESS + 0.2);
  fprintf (scad_output, "\t\t\t}\n");
  fprintf (scad_output, "\t\t}\n");
  fprintf (scad_output, "\t}\n");
  fprintf (scad_output, "}\n\n");
}

void
scad_generate_board ()
{
  fprintf (scad_output, "module board_body() {\n");
  fprintf (scad_output,
	   "\ttranslate ([0, 0, %f]) linear_extrude(height=%f) board_outline();",
	   -BOARD_THICKNESS / 2., BOARD_THICKNESS);
  fprintf (scad_output, "}\n\n");
}

void
scad_generate_mask ()
{
  fprintf (scad_output, "module mask_surface() {\n");
  fprintf (scad_output,
	   "\ttranslate ([0, 0, %f]) linear_extrude(height=%f) board_outline();",
	   -MASK_THICKNESS / 2., MASK_THICKNESS);
  fprintf (scad_output, "}\n\n");
}
