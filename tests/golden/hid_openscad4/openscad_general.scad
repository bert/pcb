/*!
 * \file Basic_OpenSCAD_Test.scad
 *
 * \author Copyright Bert Timmerman.
 *
 * \brief PCB - OpenSCAD 3D-model exporter Version 1.0
 *
 * This OpenSCAD 3D-model is free software; you may redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * As a special exception, if you create a design which uses this
 * OpenSCAD 3D-model, and embed this 3D-model file or unaltered portions
 * of this OpenSCAD 3D-model into the design, this OpenSCAD 3D-model does
 * not by itself cause the resulting design to be covered by the GNU
 * General Public License.
 * This exception does not however invalidate any other reasons why
 * the design itself might be covered by the GNU General Public
 * License.
 * If you modify this OpenSCAD 3D-model, you may extend this exception
 * to your version of the OpenSCAD 3D-model, but you are not obligated
 * to do so.
 * If you do not wish to do so, delete this exception statement from
 * your version.
 *
 * All dimensions in mm. Angles in degrees.
 */


COPPER = [0.88, 0.78, 0.5];
FR4 = [0.7, 0.67, 0.6, 0.95];
DRILL_HOLE = [1.0, 1.0, 1.0];

$fa = 1;
$fs = 0.1;
$fn = 36;
$t = 0.01;

module BOARD
(
  length,
  width,
  thickness,
)
{
  color (FR4)
  {
    cube([length, width, thickness], center = false);
  }
}

module PIN_HOLE (x, y, diameter, depth)
{
    translate ([x, y, -depth*.05])
    {
        color (DRILL_HOLE)
        cylinder (r = (diameter / 2), h = depth*1.1, center = false);
    }
}

module VIA_HOLE (x, y, diameter, depth)
{
    translate ([x, y, -depth*.05])
    {
        color (COPPER)
        cylinder (r = (diameter / 2), h = depth*1.1, center = false);
    }
}

include <PACKAGES.scad>
use <INSERT_PART_MODEL.scad>


/* Uncomment the following line for an example. */
//Basic_OpenSCAD_Test ();


module Basic_OpenSCAD_Test ()
{
    /* Modelling a printed circuit board based on maximum dimensions. */
    difference ()
    {
        BOARD (152.40, 127.00, 62.00);
        /* Now subtract some via holes. */
        /* Now subtract some pin holes. */
        PIN_HOLE (10.16, 114.30, 0.71, 62.00); // pin# 1
        PIN_HOLE (10.16, 111.76, 0.71, 62.00); // pin# 2
        PIN_HOLE (10.16, 109.22, 0.71, 62.00); // pin# 3
        PIN_HOLE (10.16, 106.68, 0.71, 62.00); // pin# 4
        PIN_HOLE (17.78, 106.68, 0.71, 62.00); // pin# 5
        PIN_HOLE (17.78, 109.22, 0.71, 62.00); // pin# 6
        PIN_HOLE (17.78, 111.76, 0.71, 62.00); // pin# 7
        PIN_HOLE (17.78, 114.30, 0.71, 62.00); // pin# 8
        PIN_HOLE (27.94, 106.68, 0.71, 62.00); // pin# 1
        PIN_HOLE (30.48, 106.68, 0.71, 62.00); // pin# 2
        PIN_HOLE (33.02, 106.68, 0.71, 62.00); // pin# 3
        PIN_HOLE (35.56, 106.68, 0.71, 62.00); // pin# 4
        PIN_HOLE (35.56, 114.30, 0.71, 62.00); // pin# 5
        PIN_HOLE (33.02, 114.30, 0.71, 62.00); // pin# 6
        PIN_HOLE (30.48, 114.30, 0.71, 62.00); // pin# 7
        PIN_HOLE (27.94, 114.30, 0.71, 62.00); // pin# 8
        PIN_HOLE (53.34, 106.68, 0.71, 62.00); // pin# 1
        PIN_HOLE (53.34, 109.22, 0.71, 62.00); // pin# 2
        PIN_HOLE (53.34, 111.76, 0.71, 62.00); // pin# 3
        PIN_HOLE (53.34, 114.30, 0.71, 62.00); // pin# 4
        PIN_HOLE (45.72, 114.30, 0.71, 62.00); // pin# 5
        PIN_HOLE (45.72, 111.76, 0.71, 62.00); // pin# 6
        PIN_HOLE (45.72, 109.22, 0.71, 62.00); // pin# 7
        PIN_HOLE (45.72, 106.68, 0.71, 62.00); // pin# 8
        PIN_HOLE (71.12, 114.30, 0.71, 62.00); // pin# 1
        PIN_HOLE (68.58, 114.30, 0.71, 62.00); // pin# 2
        PIN_HOLE (66.04, 114.30, 0.71, 62.00); // pin# 3
        PIN_HOLE (63.50, 114.30, 0.71, 62.00); // pin# 4
        PIN_HOLE (63.50, 106.68, 0.71, 62.00); // pin# 5
        PIN_HOLE (66.04, 106.68, 0.71, 62.00); // pin# 6
        PIN_HOLE (68.58, 106.68, 0.71, 62.00); // pin# 7
        PIN_HOLE (71.12, 106.68, 0.71, 62.00); // pin# 8
        PIN_HOLE (10.16, 86.36, 0.71, 62.00); // pin# 1
        PIN_HOLE (10.16, 88.90, 0.71, 62.00); // pin# 2
        PIN_HOLE (10.16, 91.44, 0.71, 62.00); // pin# 3
        PIN_HOLE (10.16, 93.98, 0.71, 62.00); // pin# 4
        PIN_HOLE (17.78, 93.98, 0.71, 62.00); // pin# 5
        PIN_HOLE (17.78, 91.44, 0.71, 62.00); // pin# 6
        PIN_HOLE (17.78, 88.90, 0.71, 62.00); // pin# 7
        PIN_HOLE (17.78, 86.36, 0.71, 62.00); // pin# 8
        PIN_HOLE (27.94, 93.98, 0.71, 62.00); // pin# 1
        PIN_HOLE (30.48, 93.98, 0.71, 62.00); // pin# 2
        PIN_HOLE (33.02, 93.98, 0.71, 62.00); // pin# 3
        PIN_HOLE (35.56, 93.98, 0.71, 62.00); // pin# 4
        PIN_HOLE (35.56, 86.36, 0.71, 62.00); // pin# 5
        PIN_HOLE (33.02, 86.36, 0.71, 62.00); // pin# 6
        PIN_HOLE (30.48, 86.36, 0.71, 62.00); // pin# 7
        PIN_HOLE (27.94, 86.36, 0.71, 62.00); // pin# 8
        PIN_HOLE (53.34, 93.98, 0.71, 62.00); // pin# 1
        PIN_HOLE (53.34, 91.44, 0.71, 62.00); // pin# 2
        PIN_HOLE (53.34, 88.90, 0.71, 62.00); // pin# 3
        PIN_HOLE (53.34, 86.36, 0.71, 62.00); // pin# 4
        PIN_HOLE (45.72, 86.36, 0.71, 62.00); // pin# 5
        PIN_HOLE (45.72, 88.90, 0.71, 62.00); // pin# 6
        PIN_HOLE (45.72, 91.44, 0.71, 62.00); // pin# 7
        PIN_HOLE (45.72, 93.98, 0.71, 62.00); // pin# 8
        PIN_HOLE (71.12, 86.36, 0.71, 62.00); // pin# 1
        PIN_HOLE (68.58, 86.36, 0.71, 62.00); // pin# 2
        PIN_HOLE (66.04, 86.36, 0.71, 62.00); // pin# 3
        PIN_HOLE (63.50, 86.36, 0.71, 62.00); // pin# 4
        PIN_HOLE (63.50, 93.98, 0.71, 62.00); // pin# 5
        PIN_HOLE (66.04, 93.98, 0.71, 62.00); // pin# 6
        PIN_HOLE (68.58, 93.98, 0.71, 62.00); // pin# 7
        PIN_HOLE (71.12, 93.98, 0.71, 62.00); // pin# 8
    }

    /* Now insert some element models.
    INSERT_PART_MODEL ("Package type", "Model name", Tx, Ty, Rz, "Side", "Value"); // RefDes */
    INSERT_PART_MODEL ("Standard SMT resistor, capacitor etc", "Standard SMT resistor, capacitor etc", 15.24, 99.06, 0.00, "bottom", "RESC3216N"); // refdes: R0_BOT
    INSERT_PART_MODEL ("Standard SMT resistor, capacitor etc", "Standard SMT resistor, capacitor etc", 25.40, 99.06, 90.00, "bottom", "RESC3216N"); // refdes: R90_BOT
    INSERT_PART_MODEL ("Standard SMT resistor, capacitor etc", "Standard SMT resistor, capacitor etc", 33.02, 99.06, 180.00, "bottom", "RESC3216N"); // refdes: R180_BOT
    INSERT_PART_MODEL ("Standard SMT resistor, capacitor etc", "Standard SMT resistor, capacitor etc", 43.18, 99.06, 270.00, "bottom", "RESC3216N"); // refdes: R270_BOT
    INSERT_PART_MODEL ("Small outline package, narrow (", "Small outline package, narrow (150mil)", 15.24, 66.04, 0.00, "bottom", "SO8"); // refdes: USO0_BOT
    INSERT_PART_MODEL ("Small outline package, narrow (", "Small outline package, narrow (150mil)", 25.40, 66.04, 90.00, "bottom", "SO8"); // refdes: USO90_BOT
    INSERT_PART_MODEL ("Small outline package, narrow (", "Small outline package, narrow (150mil)", 33.02, 66.04, 180.00, "bottom", "SO8"); // refdes: USO180_BOT
    INSERT_PART_MODEL ("Small outline package, narrow (", "Small outline package, narrow (150mil)", 43.18, 66.04, 270.00, "bottom", "SO8"); // refdes: USO270_BOT
    INSERT_PART_MODEL ("Dual in-line package, narrow (", "Dual in-line package, narrow (300 mil)", 13.97, 16.51, 0.00, "bottom", "DIP8"); // refdes: UDIP0_BOT
    INSERT_PART_MODEL ("Dual in-line package, narrow (", "Dual in-line package, narrow (300 mil)", 31.75, 16.51, 90.00, "bottom", "DIP8"); // refdes: UDIP90_BOT
    INSERT_PART_MODEL ("Dual in-line package, narrow (", "Dual in-line package, narrow (300 mil)", 49.53, 16.51, 180.00, "bottom", "DIP8"); // refdes: UDIP180_BOT
    INSERT_PART_MODEL ("Dual in-line package, narrow (", "Dual in-line package, narrow (300 mil)", 67.31, 16.51, 270.00, "bottom", "DIP8"); // refdes: UDIP270_BOT
    INSERT_PART_MODEL ("Small outline package, narrow (", "Small outline package, narrow (150mil)", 15.24, 81.28, 90.00, "top", "SO8"); // refdes: USO0_TOP
    INSERT_PART_MODEL ("Small outline package, narrow (", "Small outline package, narrow (150mil)", 25.40, 81.28, 0.00, "top", "SO8"); // refdes: USO270_TOP
    INSERT_PART_MODEL ("Small outline package, narrow (", "Small outline package, narrow (150mil)", 33.02, 81.28, 270.00, "top", "SO8"); // refdes: USO180_TOP
    INSERT_PART_MODEL ("Small outline package, narrow (", "Small outline package, narrow (150mil)", 43.18, 81.28, 180.00, "top", "SO8"); // refdes: USO90_TOP
    INSERT_PART_MODEL ("Dual in-line package, narrow (", "Dual in-line package, narrow (300 mil)", 13.97, 36.83, 90.00, "top", "DIP8"); // refdes: UDIP0_TOP
    INSERT_PART_MODEL ("Dual in-line package, narrow (", "Dual in-line package, narrow (300 mil)", 31.75, 36.83, 0.00, "top", "DIP8"); // refdes: UDIP270_TOP
    INSERT_PART_MODEL ("Dual in-line package, narrow (", "Dual in-line package, narrow (300 mil)", 49.53, 36.83, 270.00, "top", "DIP8"); // refdes: UDIP180_TOP
    INSERT_PART_MODEL ("Dual in-line package, narrow (", "Dual in-line package, narrow (300 mil)", 67.31, 36.83, 180.00, "top", "DIP8"); // refdes: UDIP90_TOP
    INSERT_PART_MODEL ("Standard SMT resistor, capacitor etc", "Standard SMT resistor, capacitor etc", 15.24, 109.22, 0.00, "top", "RESC3216N"); // refdes: R0_TOP
    INSERT_PART_MODEL ("Standard SMT resistor, capacitor etc", "Standard SMT resistor, capacitor etc", 25.40, 109.22, 270.00, "top", "RESC3216N"); // refdes: R270_TOP
    INSERT_PART_MODEL ("Standard SMT resistor, capacitor etc", "Standard SMT resistor, capacitor etc", 33.02, 109.22, 180.00, "top", "RESC3216N"); // refdes: R180_TOP
    INSERT_PART_MODEL ("Standard SMT resistor, capacitor etc", "Standard SMT resistor, capacitor etc", 43.18, 109.22, 90.00, "top", "RESC3216N"); // refdes: R90_TOP
}

/* EOF */
