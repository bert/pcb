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
 * All dimensions in mils. Angles in degrees.
 */


include <COLORS.scad>
include <CONST.scad>
include <BOARD.scad>
include <PIN_HOLE.scad>
include <VIA_HOLE.scad>
include <PACKAGES.scad>
use <INSERT_PART_MODEL.scad>


/* Uncomment the following line for an example. */
//Basic_OpenSCAD_Test ();


module Basic_OpenSCAD_Test ()
{
    /* Modelling a printed circuit board based on maximum dimensions. */
    difference ()
    {
        BOARD (6000.00, 5000.00, 62.00);
        /* Now subtract some via holes. */
        /* Now subtract some pin holes. */
        PIN_HOLE (400.00, 4500.00, 28.00, 62.00);
        PIN_HOLE (400.00, 4400.00, 28.00, 62.00);
        PIN_HOLE (400.00, 4300.00, 28.00, 62.00);
        PIN_HOLE (400.00, 4200.00, 28.00, 62.00);
        PIN_HOLE (700.00, 4200.00, 28.00, 62.00);
        PIN_HOLE (700.00, 4300.00, 28.00, 62.00);
        PIN_HOLE (700.00, 4400.00, 28.00, 62.00);
        PIN_HOLE (700.00, 4500.00, 28.00, 62.00);
        PIN_HOLE (1100.00, 4200.00, 28.00, 62.00);
        PIN_HOLE (1200.00, 4200.00, 28.00, 62.00);
        PIN_HOLE (1300.00, 4200.00, 28.00, 62.00);
        PIN_HOLE (1400.00, 4200.00, 28.00, 62.00);
        PIN_HOLE (1400.00, 4500.00, 28.00, 62.00);
        PIN_HOLE (1300.00, 4500.00, 28.00, 62.00);
        PIN_HOLE (1200.00, 4500.00, 28.00, 62.00);
        PIN_HOLE (1100.00, 4500.00, 28.00, 62.00);
        PIN_HOLE (2100.00, 4200.00, 28.00, 62.00);
        PIN_HOLE (2100.00, 4300.00, 28.00, 62.00);
        PIN_HOLE (2100.00, 4400.00, 28.00, 62.00);
        PIN_HOLE (2100.00, 4500.00, 28.00, 62.00);
        PIN_HOLE (1800.00, 4500.00, 28.00, 62.00);
        PIN_HOLE (1800.00, 4400.00, 28.00, 62.00);
        PIN_HOLE (1800.00, 4300.00, 28.00, 62.00);
        PIN_HOLE (1800.00, 4200.00, 28.00, 62.00);
        PIN_HOLE (2800.00, 4500.00, 28.00, 62.00);
        PIN_HOLE (2700.00, 4500.00, 28.00, 62.00);
        PIN_HOLE (2600.00, 4500.00, 28.00, 62.00);
        PIN_HOLE (2500.00, 4500.00, 28.00, 62.00);
        PIN_HOLE (2500.00, 4200.00, 28.00, 62.00);
        PIN_HOLE (2600.00, 4200.00, 28.00, 62.00);
        PIN_HOLE (2700.00, 4200.00, 28.00, 62.00);
        PIN_HOLE (2800.00, 4200.00, 28.00, 62.00);
        PIN_HOLE (400.00, 3400.00, 28.00, 62.00);
        PIN_HOLE (400.00, 3500.00, 28.00, 62.00);
        PIN_HOLE (400.00, 3600.00, 28.00, 62.00);
        PIN_HOLE (400.00, 3700.00, 28.00, 62.00);
        PIN_HOLE (700.00, 3700.00, 28.00, 62.00);
        PIN_HOLE (700.00, 3600.00, 28.00, 62.00);
        PIN_HOLE (700.00, 3500.00, 28.00, 62.00);
        PIN_HOLE (700.00, 3400.00, 28.00, 62.00);
        PIN_HOLE (1100.00, 3700.00, 28.00, 62.00);
        PIN_HOLE (1200.00, 3700.00, 28.00, 62.00);
        PIN_HOLE (1300.00, 3700.00, 28.00, 62.00);
        PIN_HOLE (1400.00, 3700.00, 28.00, 62.00);
        PIN_HOLE (1400.00, 3400.00, 28.00, 62.00);
        PIN_HOLE (1300.00, 3400.00, 28.00, 62.00);
        PIN_HOLE (1200.00, 3400.00, 28.00, 62.00);
        PIN_HOLE (1100.00, 3400.00, 28.00, 62.00);
        PIN_HOLE (2100.00, 3700.00, 28.00, 62.00);
        PIN_HOLE (2100.00, 3600.00, 28.00, 62.00);
        PIN_HOLE (2100.00, 3500.00, 28.00, 62.00);
        PIN_HOLE (2100.00, 3400.00, 28.00, 62.00);
        PIN_HOLE (1800.00, 3400.00, 28.00, 62.00);
        PIN_HOLE (1800.00, 3500.00, 28.00, 62.00);
        PIN_HOLE (1800.00, 3600.00, 28.00, 62.00);
        PIN_HOLE (1800.00, 3700.00, 28.00, 62.00);
        PIN_HOLE (2800.00, 3400.00, 28.00, 62.00);
        PIN_HOLE (2700.00, 3400.00, 28.00, 62.00);
        PIN_HOLE (2600.00, 3400.00, 28.00, 62.00);
        PIN_HOLE (2500.00, 3400.00, 28.00, 62.00);
        PIN_HOLE (2500.00, 3700.00, 28.00, 62.00);
        PIN_HOLE (2600.00, 3700.00, 28.00, 62.00);
        PIN_HOLE (2700.00, 3700.00, 28.00, 62.00);
        PIN_HOLE (2800.00, 3700.00, 28.00, 62.00);
    }

    /* Now insert some element models.
    INSERT_PART_MODEL ("Package type", "Model name", Tx, Ty, Rz, "Side", "Value"); // RefDes */
    INSERT_PART_MODEL ("Standard SMT resistor, capacitor etc", "Standard SMT resistor, capacitor etc", 600.00, 3900.00, 0.00, "bottom", "RESC3216N"); // refdes: R0_BOT
    INSERT_PART_MODEL ("Standard SMT resistor, capacitor etc", "Standard SMT resistor, capacitor etc", 1000.00, 3900.00, 90.00, "bottom", "RESC3216N"); // refdes: R90_BOT
    INSERT_PART_MODEL ("Standard SMT resistor, capacitor etc", "Standard SMT resistor, capacitor etc", 1300.00, 3900.00, 180.00, "bottom", "RESC3216N"); // refdes: R180_BOT
    INSERT_PART_MODEL ("Standard SMT resistor, capacitor etc", "Standard SMT resistor, capacitor etc", 1700.00, 3900.00, 270.00, "bottom", "RESC3216N"); // refdes: R270_BOT
    INSERT_PART_MODEL ("Small outline package, narrow (", "Small outline package, narrow (150mil)", 600.00, 2600.00, 0.00, "bottom", "SO8"); // refdes: USO0_BOT
    INSERT_PART_MODEL ("Small outline package, narrow (", "Small outline package, narrow (150mil)", 1000.00, 2600.00, 90.00, "bottom", "SO8"); // refdes: USO90_BOT
    INSERT_PART_MODEL ("Small outline package, narrow (", "Small outline package, narrow (150mil)", 1300.00, 2600.00, 180.00, "bottom", "SO8"); // refdes: USO180_BOT
    INSERT_PART_MODEL ("Small outline package, narrow (", "Small outline package, narrow (150mil)", 1700.00, 2600.00, 270.00, "bottom", "SO8"); // refdes: USO270_BOT
    INSERT_PART_MODEL ("Dual in-line package, narrow (", "Dual in-line package, narrow (300 mil)", 550.00, 650.00, 0.00, "bottom", "DIP8"); // refdes: UDIP0_BOT
    INSERT_PART_MODEL ("Dual in-line package, narrow (", "Dual in-line package, narrow (300 mil)", 1250.00, 650.00, 90.00, "bottom", "DIP8"); // refdes: UDIP90_BOT
    INSERT_PART_MODEL ("Dual in-line package, narrow (", "Dual in-line package, narrow (300 mil)", 1950.00, 650.00, 180.00, "bottom", "DIP8"); // refdes: UDIP180_BOT
    INSERT_PART_MODEL ("Dual in-line package, narrow (", "Dual in-line package, narrow (300 mil)", 2650.00, 650.00, 270.00, "bottom", "DIP8"); // refdes: UDIP270_BOT
    INSERT_PART_MODEL ("Small outline package, narrow (", "Small outline package, narrow (150mil)", 600.00, 3200.00, 90.00, "top", "SO8"); // refdes: USO0_TOP
    INSERT_PART_MODEL ("Small outline package, narrow (", "Small outline package, narrow (150mil)", 1000.00, 3200.00, 0.00, "top", "SO8"); // refdes: USO270_TOP
    INSERT_PART_MODEL ("Small outline package, narrow (", "Small outline package, narrow (150mil)", 1300.00, 3200.00, 270.00, "top", "SO8"); // refdes: USO180_TOP
    INSERT_PART_MODEL ("Small outline package, narrow (", "Small outline package, narrow (150mil)", 1700.00, 3200.00, 180.00, "top", "SO8"); // refdes: USO90_TOP
    INSERT_PART_MODEL ("Dual in-line package, narrow (", "Dual in-line package, narrow (300 mil)", 550.00, 1450.00, 90.00, "top", "DIP8"); // refdes: UDIP0_TOP
    INSERT_PART_MODEL ("Dual in-line package, narrow (", "Dual in-line package, narrow (300 mil)", 1250.00, 1450.00, 0.00, "top", "DIP8"); // refdes: UDIP270_TOP
    INSERT_PART_MODEL ("Dual in-line package, narrow (", "Dual in-line package, narrow (300 mil)", 1950.00, 1450.00, 270.00, "top", "DIP8"); // refdes: UDIP180_TOP
    INSERT_PART_MODEL ("Dual in-line package, narrow (", "Dual in-line package, narrow (300 mil)", 2650.00, 1450.00, 180.00, "top", "DIP8"); // refdes: UDIP90_TOP
    INSERT_PART_MODEL ("Standard SMT resistor, capacitor etc", "Standard SMT resistor, capacitor etc", 600.00, 4300.00, 0.00, "top", "RESC3216N"); // refdes: R0_TOP
    INSERT_PART_MODEL ("Standard SMT resistor, capacitor etc", "Standard SMT resistor, capacitor etc", 1000.00, 4300.00, 270.00, "top", "RESC3216N"); // refdes: R270_TOP
    INSERT_PART_MODEL ("Standard SMT resistor, capacitor etc", "Standard SMT resistor, capacitor etc", 1300.00, 4300.00, 180.00, "top", "RESC3216N"); // refdes: R180_TOP
    INSERT_PART_MODEL ("Standard SMT resistor, capacitor etc", "Standard SMT resistor, capacitor etc", 1700.00, 4300.00, 90.00, "top", "RESC3216N"); // refdes: R90_TOP
}

/* EOF */ 

