*# -*- m4 -*-
#                             COPYRIGHT
# 
#   PCB, interactive printed circuit board design
#   Copyright (C) 1994,1995,1996 Thomas Nau
# 
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
# 
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
# 
#   You should have received a copy of the GNU General Public License along
#   with this program; if not, write to the Free Software Foundation, Inc.,
#   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
# 
#   Contact addresses for paper mail and Email:
#   Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
#   Thomas.Nau@rz.uni-ulm.de
# 
#
# connector packages

# -------------------------------------------------------------------
# the definition of a single connector
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: number of rows
# $5: number of columns
#
define(`PKG_CONNECTOR',
	`define(`MAXY', `eval(`$4' * 100)')
	define(`MAXX', `eval(`$5' * 100)')
Element(0x00 "$1" "`$2'" "$3" eval(MAXX + 60) 0 3 100 0x00)
(
	forloop(`row', 1, $4, `forloop(`col', 1, $5,
		`PIN(eval(col * 100 -50), eval(row * 100 -50), 60, 38, eval((row-1)*$5+col))
	') ')
	ElementLine(0 0 0 MAXY 10)
	ElementLine(0 MAXY MAXX MAXY 10)
	ElementLine(MAXX MAXY MAXX 0 10)
	ElementLine(MAXX 0 0 0 10)
	ElementLine(0 100 100 100 10)
	ElementLine(100 100 100 0 10)
	Mark(50 50)
)')

# -------------------------------------------------------------------
# the definition of a 2 column header connector with DIP
# pin numbering.  Use PKG_CONNECTOR for ribbon cable numbering
#
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: number of rows
#
define(`PKG_CONNECTOR_DIL',
	`define(`MAXY', `eval(`$4' * 100)')
	define(`MAXX', `200')
Element(0x00 "$1" "`$2'" "$3" eval(MAXX + 60) 0 3 100 0x00)
(
	forloop(`row', 1, eval(`$4'),
		`PIN(50, eval(row * 100 -50), 60, 38, eval(row))
	')
	forloop(`row', 1, eval(`$4'),
		`PIN(150, eval(($4 + 1 - row) * 100 -50), 60, 38, eval($4 + row))
	')
	ElementLine(0 0 0 MAXY 10)
	ElementLine(0 MAXY MAXX MAXY 10)
	ElementLine(MAXX MAXY MAXX 0 10)
	ElementLine(MAXX 0 0 0 10)
	ElementLine(0 100 100 100 10)
	ElementLine(100 100 100 0 10)
	Mark(50 50)
)')



# ---------------------------------------------------------------
# base definition (housing) for connector DIN 41.612
# 11/95 Volker Bosch (bosch@iema.e-technik.uni-stuttgart.de)
# female package added 05/96 also by Volker Bosch
#
# derived from above for 48 - Pin Connectors from the same series
# Holm Tiffe
#
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: requested rows a,b,c
#
define(`PKG_DIN41_612MALE_SMALL',
	`define(`XPOS', `eval(300 + 100 * i)')
	define(`MINY', 300)
Element(0x00 "$1" "`$2'" "$3" 520 550 0 200 0x00)
(
	# Reihe a
	ifelse(index(`$4', `a'), `-1', ,
		forloop(`i', 1, 16, `PIN(eval(2000 - 100 *i), 300, 60, 30, i)
	'))

	# Reihe b
	ifelse(index(`$4', `b'), `-1', , `define(`MINY', 200)'
		forloop(`i', 1, 16, `PIN(eval(2000 - 100 *i), 200, 60, 30, eval(16 + i))
	'))

	# Reihe c
	ifelse(index(`$4', `c'), `-1', , `define(`MINY', 100)'
		forloop(`i', 1, 16, `PIN(eval(2000 - 100 *i), 100, 60, 30, eval(32 + i))
	'))
    # Rueckseite Kontaktstifte
	forloop(`i', 1, 16, `ElementLine(XPOS MINY XPOS 375 40)
	')

	# Befestigungsbohrung
	Pin( 200 400 120 80 "M1" 0x01)
	Pin(2100 400 120 80 "M2" 0x01)

	# Begrenzung M1
	ElementLine( 100  300  320  300 20)
	ElementLine( 320  300  320  395 20)
	ElementLine( 320  395  320  620 10)
	ElementLine( 320  620  200  620 10)
	ElementLine( 200  620  100  620 20)
	ElementLine( 100  620  100  300 20)

	# Begrenzung M2
	ElementLine(1980  300 2200  300 20)
	ElementLine(2200  300 2200  620 20)
	ElementLine(2200  620 2100  620 20)
	ElementLine(2100  620 1980  620 10)
	ElementLine(1980  620 1980  395 10)
	ElementLine(1980  395 1980  300 20)

	# Kante Pins
	ElementLine( 320  395 1980  395 20)

	# Kanten Stifte
	ElementLine( 200 620  200 800 20)
	ElementLine( 200 800 2100 800 20)
	ElementLine(2100 800 2100 620 20)

	Mark(1900 300)
)')

define(`PKG_DIN41_612FEMALE_SMALL',
	`define(`MAXX', 420)
	define(`MAXX1', `eval(MAXX -170)')
	define(`MAXX2', `eval(MAXX -40)')
Element(0x00 "$1" "`$2'" "$3" 50 100 3 200 0x00)
(
	# Reihe a
	ifelse(index(`$4', `a'), `-1', ,
		forloop(`i', 1, 16, `PIN(200, eval(300 + 100 *i), 60, 30, i)
	'))

	# Reihe b
	ifelse(index(`$4', `b'), `-1', ,
		forloop(`i', 1, 16, `PIN(300, eval(300 + 100 *i), 60, 30, eval(16 + i))
	'))

	# Reihe c
	ifelse(index(`$4', `c'), `-1', , `define(`MAXX', 520)'
		forloop(`i', 1, 16, `PIN(400, eval(300 + 100 *i), 60, 30, eval(32 + i))
	'))

	# Befestigungsbohrung
	Pin(290  180 120 80 "M1" 0x01)
	Pin(290 2120 120 80 "M2" 0x01)

	# Aeussere Begrenzung
	ElementLine( 80  80 MAXX   80 20)
	ElementLine(MAXX  80 MAXX 2220 20)
	ElementLine(MAXX 2220 80 2220 20)
	ElementLine( 80 2220 80   80 20)

	# Innere Begrenzung
	ElementLine(120  320 MAXX1  320 10)
	ElementLine(MAXX1  320 MAXX1  360 10)
	ElementLine(MAXX1  360 MAXX2  360 10)
	ElementLine(MAXX2  360 MAXX2 1940 10)
	ElementLine(MAXX2 1940 MAXX1 1940 10)
	ElementLine(MAXX1 1940 MAXX1 1980 10)
	ElementLine(MAXX1 1980 120 1980 10)
	ElementLine(120 1980 120  320 10)

	# Markierung: Pin 1a
	Mark(200 400)
)')


# ---------------------------------------------------------------
# base definition (housing) for connector DIN 41.612
# 11/95 Volker Bosch (bosch@iema.e-technik.uni-stuttgart.de)
# female package added 05/96 also by Volker Bosch
#
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: requested rows a,b,c
#
define(`PKG_DIN41_612MALE',
	`define(`XPOS', `eval(300 + 100 * i)')
	define(`MINY', 300)
Element(0x00 "$1" "`$2'" "$3" 520 550 0 200 0x00)
(
	# Reihe a
	ifelse(index(`$4', `a'), `-1', ,
		forloop(`i', 1, 32, `PIN(eval(3600 - 100 *i), 300, 60, 30, i)
	'))

	# Reihe b
	ifelse(index(`$4', `b'), `-1', , `define(`MINY', 200)'
		forloop(`i', 1, 32, `PIN(eval(3600 - 100 *i), 200, 60, 30, eval(32 + i))
	'))

	# Reihe c
	ifelse(index(`$4', `c'), `-1', , `define(`MINY', 100)'
		forloop(`i', 1, 32, `PIN(eval(3600 - 100 *i), 100, 60, 30, eval(64 + i))
	'))
    # Rueckseite Kontaktstifte
	forloop(`i', 1, 32, `ElementLine(XPOS MINY XPOS 375 40)
	')

	# Befestigungsbohrung
	Pin( 200 400 120 80 "M1" 0x01)
	Pin(3700 400 120 80 "M2" 0x01)

	# Begrenzung M1
	ElementLine( 100  300  320  300 20)
	ElementLine( 320  300  320  395 20)
	ElementLine( 320  395  320  620 10)
	ElementLine( 320  620  200  620 10)
	ElementLine( 200  620  100  620 20)
	ElementLine( 100  620  100  300 20)

	# Begrenzung M2
	ElementLine(3580  300 3800  300 20)
	ElementLine(3800  300 3800  620 20)
	ElementLine(3800  620 3700  620 20)
	ElementLine(3700  620 3580  620 10)
	ElementLine(3580  620 3580  395 10)
	ElementLine(3580  395 3580  300 20)

	# Kante Pins
	ElementLine( 320  395 3580  395 20)

	# Kanten Stifte
	ElementLine( 200 620  200 800 20)
	ElementLine( 200 800 3700 800 20)
	ElementLine(3700 800 3700 620 20)

	Mark(3500 300)
)')

define(`PKG_DIN41_612FEMALE',
	`define(`MAXX', 420)
	define(`MAXX1', `eval(MAXX -170)')
	define(`MAXX2', `eval(MAXX -40)')
Element(0x00 "$1" "`$2'" "$3" 50 100 3 200 0x00)
(
	# Reihe a
	ifelse(index(`$4', `a'), `-1', ,
		forloop(`i', 1, 32, `PIN(200, eval(300 + 100 *i), 60, 30, i)
	'))

	# Reihe b
	ifelse(index(`$4', `b'), `-1', ,
		forloop(`i', 1, 32, `PIN(300, eval(300 + 100 *i), 60, 30, eval(32 + i))
	'))

	# Reihe c
	ifelse(index(`$4', `c'), `-1', , `define(`MAXX', 520)'
		forloop(`i', 1, 32, `PIN(400, eval(300 + 100 *i), 60, 30, eval(64 + i))
	'))

	# Befestigungsbohrung
	Pin(290  180 120 80 "M1" 0x01)
	Pin(290 3720 120 80 "M2" 0x01)

	# Aeussere Begrenzung
	ElementLine( 80  80 MAXX   80 20)
	ElementLine(MAXX  80 MAXX 3820 20)
	ElementLine(MAXX 3820 80 3820 20)
	ElementLine( 80 3820 80   80 20)

	# Innere Begrenzung
	ElementLine(120  320 MAXX1  320 10)
	ElementLine(MAXX1  320 MAXX1  360 10)
	ElementLine(MAXX1  360 MAXX2  360 10)
	ElementLine(MAXX2  360 MAXX2 3540 10)
	ElementLine(MAXX2 3540 MAXX1 3540 10)
	ElementLine(MAXX1 3540 MAXX1 3580 10)
	ElementLine(MAXX1 3580 120 3580 10)
	ElementLine(120 3580 120  320 10)

	# Markierung: Pin 1a
	Mark(200 400)
)')

# ---------------------------------------------------------------
# base definition (housing) for connector DIN 41.651 laying and standing
# 05/96 Volker Bosch (bosch@iema.e-technik.uni-stuttgart.de)
#
# Changed hole size from 20 mils to 28 mils; 07/00 LRDoolittle@lbl.gov
#
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: number of pins
#
define(`PKG_DIN41_651LAY',
	`define(`MAXY', `eval($4/2 *100 +400)')
	define(`CENTERY', `eval($4/2 *50 +425)')
Element(0x00 "$1" "`$2'" "$3" 400 250 3 200 0x00)
(
	forloop(`i', 1, eval($4 / 2),
		`PIN(100,  eval(100*i +400), 60, 40, eval(2*i-1))
		PIN(200, eval(100*i +400), 60, 40, eval(2*i))
	')

	# Befestigungsbohrung
	Pin(180  270 100 80 "M1" 0x01)
	Pin(180 eval(MAXY+230) 100 80 "M2" 0x01)

	# aeusserer Rahmen
	ElementLine(80 70 335 70 20)
	ElementLine(335 70 770 200 20)
	ElementLine(770 200 770 300 20)
	ElementLine(770 300 610 390 20)
	ElementLine(610 390 610 eval(MAXY+150) 20)
	ElementLine(610 eval(MAXY+150) 770 eval(MAXY+200) 20)
	ElementLine(770 eval(MAXY+200) 770 eval(MAXY+300) 20)
	ElementLine(770 eval(MAXY+300) 335 eval(MAXY+430) 20)
	ElementLine(335 eval(MAXY+430) 80 eval(MAXY+430) 20)
	ElementLine( 80 eval(MAXY+430) 80 70 20)

	# Codieraussparung
	ElementLine(610 eval(CENTERY -50) 435 eval(CENTERY -50) 5)
	ElementLine(435 eval(CENTERY -50) 435 eval(CENTERY +100) 5)
	ElementLine(435 eval(CENTERY +100) 610 eval(CENTERY +100) 5)

	# Markierung Pin 1
	ElementLine(610 450 500 500 5)
	ElementLine(500 500 610 550 5)

	# Plazierungsmarkierung == Pin 1
	Mark(100 500)
)')

define(`PKG_DIN41_651STAND',
	`define(`MAXY', `eval($4/2 *100 +400)')
	define(`CENTERY', `eval($4/2 *50 +425)')
Element(0x00 "$1" "`$2'" "$3" 50 100 3 200 0x00)
(
	forloop(`i', 1, eval($4 / 2),
		`PIN(200,  eval(100*i +400), 60, 40, eval(2*i-1))
		PIN(300, eval(100*i +400), 60, 40, eval(2*i))
	')
	# aeusserer Rahmen
	ElementLine(90 70 410 70 20)
	ElementLine(410 70 410 eval(MAXY +430) 20)
	ElementLine(410 eval(MAXY +430) 90 eval(MAXY +430) 20)
	ElementLine(90 eval(MAXY +430) 90 70 20)

	# innerer Rahmen mit Codieraussparung
	ElementLine(110  350 390  350 5)
	ElementLine(390  350 390 eval(MAXY +150) 5)
	ElementLine(390 eval(MAXY +150) 110 eval(MAXY +150) 5)
	ElementLine(110 eval(MAXY +150) 110 eval(CENTERY +100) 5)
	ElementLine(110 eval(CENTERY +100)  90 eval(CENTERY +100) 5)
	ElementLine(90  eval(CENTERY -50) 110 eval(CENTERY -50) 5)
	ElementLine(110 eval(CENTERY -50) 110  350 5)

	# Markierung Pin 1
	ElementLine(110 390 150 350 5)

	# Auswurfhebel oben
	ElementLine(200 70 200 350 5)
	ElementLine(300 70 300 350 5)

	# Auswurfhebel unten
	ElementLine(200 eval(MAXY+150) 200 eval(MAXY+430) 5)
	ElementLine(300 eval(MAXY+150) 300 eval(MAXY+430) 5)

	# Plazierungsmarkierung == Pin 1
	Mark(200 500)
)')

# ---------------------------------------------------------------
# base definition (housing) for SUB-D connectors, laying
# 11/95 Volker Bosch (bosch@iema.e-technik.uni-stuttgart.de)
# female package added 05/96 also by Volker Bosch
# Pins reordered, 12/99 Larry Doolittle  <LRDoolittle@lbl.gov>

# Connector dimensions changed!  No, these stupid things don't
# line up neatly on a 50 mil grid.  Now corresponds to AMP's
# Application Specification 114-40013 03-APR-97 Rev B, for
# AMPLIMITE[TM] HD-20 Series 454 and 545.
# Also pulled silkscreen lines away from pins by NOSMUDGE.
# 12/99 Larry Doolittle  <LRDoolittle@lbl.gov>
#
# Parameterized silkscreen in terms of PANEL_DISTANCE, the distance
# from the drilling holes to the intended edge of the board.
# According to the drawing listed above, that should be 398 or 489
# plus some unknown flange thickness.  For the part I have in my hand
# (AMP 747846-4), it's about 365 mils.  The original version of this
# macro had it hard coded at 460.  I also reduced size of pads and holes
# to 60/35.  The drawing listed above recommends 47 mil holes, the part
# in my hand has 28 mil pins.
#
# Status: Female tested, works great, 07-Jan-2000.
#
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: pins
#
# base is upper drilling hole
#
define(`PKG_SUBD_LAY_BASE',
	`define(`BASEX', 1000)
	define(`BASEY', 1000)
	define(`PY1', `eval(BASEY +270)')
	define(`PY2', `eval(PY1 +($4-1)/2*108)')
	define(`OFFSET', `eval(($4+1)/2)')
	define(`PANEL_DISTANCE', 365)
Element(0x00 "$1" "`$2'" "$3" eval(BASEX-0) eval(PY2-0) 1 150 0x00)
(
	# Gehaeuse (schmaler Kasten incl. Bohrungen)
	define(`X1', `eval(BASEX-PANEL_DISTANCE)')
	define(`Y1', `eval(PY1-390)')
	define(`X2', `eval(BASEX-PANEL_DISTANCE+30)')
	define(`Y2', `eval(PY2+390)')
	ElementLine(X1 Y1 X2 Y1 10)
	ElementLine(X2 Y1 X2 Y2 10)
	ElementLine(X2 Y2 X1 Y2 10)
	ElementLine(X1 Y2 X1 Y1 10)
	ElementLine(X1 eval(Y1+60) X2 eval(Y1+60) 10)
	ElementLine(X1 eval(Y1+180) X2 eval(Y1+180) 10)
	ElementLine(X1 eval(Y2-60) X2 eval(Y2-60) 10)
	ElementLine(X1 eval(Y2-180) X2 eval(Y2-180) 10)

	# Gehaeuse (aeusserer Kasten)
	# This part of the connector normally hangs off the circuit board,
	# so it is confusing to actually mark it on the silkscreen
	# define(`X1', `eval(BASEX-PANEL_DISTANCE-260)')
	# define(`Y1', `eval(PY1-100)')
	# define(`X2', `eval(BASEX-PANEL_DISTANCE)')
	# define(`Y2', `eval(PY2+100)')
	# ElementLine(X1 Y1 X2 Y1 20)
	# ElementLine(X2 Y1 X2 Y2 10)
	# ElementLine(X2 Y2 X1 Y2 20)
	# ElementLine(X1 Y2 X1 Y1 20)

	# Gehaeuse (innerer Kasten)
	define(`X1', `eval(BASEX-PANEL_DISTANCE+30)')
	define(`Y1', `eval(PY1-160)')
	define(`X2', `eval(BASEX-230)')
	define(`Y2', `eval(PY2+160)')
	ElementLine(X1 Y1 X2 Y1 20)
	ElementLine(X2 Y1 X2 Y2 20)
	ElementLine(X2 Y2 X1 Y2 20)
	ElementLine(X1 Y2 X1 Y1 10)

	# Pins
	SUBD_SUBFUNCTION

	# Befestigungsbohrung
	Pin(BASEX  BASEY 250 125 "C1" 0x01)
	Pin(BASEX eval(PY2+270) 250 125 "C2" 0x01)

)')

define(`PKG_SUBD_MALE_LAY',
	`define(`SUBD_SUBFUNCTION',
		`
		define(`NOSMUDGE', 40)
		# First row
		forloop(`i', 1, eval($4/2),
			`define(`Y', eval(PY1 +(i-1)*108))
			PIN(eval(BASEX+56), Y, 60, 35, i)
			ElementLine(eval(BASEX+56-NOSMUDGE) Y X2 Y 20)
		')

		# Last pin in first row
		PIN(eval(BASEX+56), PY2, 60, 35, OFFSET)
		ElementLine(eval(BASEX+56-NOSMUDGE) PY2 X2 PY2 20)

		# Second row
		forloop(`i', 1, eval($4/2),
			`define(`Y', eval(PY1 +(i-1)*108+54))
			PIN(eval(BASEX-56), Y, 60, 35, eval(i+OFFSET))
			ElementLine(eval(BASEX-56-NOSMUDGE) Y X2 Y 20)
		')
		# Plazierungsmarkierung == PIN 1
		Mark(eval(BASEX +50) PY1)
	')'
	`PKG_SUBD_LAY_BASE(`$1', `$2', `$3', `$4')'
)

define(`PKG_SUBD_FEMALE_LAY',
	`define(`SUBD_SUBFUNCTION',
		`
		define(`NOSMUDGE', 40)
		# Pin 1
		PIN(eval(BASEX+56), PY2, 60, 35, 1)
		ElementLine(eval(BASEX+56-NOSMUDGE) PY2 X2 PY2 20)

		# Plazierungsmarkierung == PIN 1
		# Changed PY1 to PY2 13-Dec-1999 LRD
		Mark(eval(BASEX +56) PY2)

		# Remainder of the first row
		forloop(`i', 1, eval($4/2),
			`define(`Y', eval(PY1 +($4/2-i)*108))
			PIN(eval(BASEX+56), Y, 60, 35, eval($4/2+i+2-OFFSET))
			ElementLine(eval(BASEX+56-NOSMUDGE) Y X2 Y 20)
		')

		# Second row
		forloop(`i', 1, eval($4/2),
			`define(`Y', eval(PY1 +($4/2-i)*108+54))
			PIN(eval(BASEX-56), Y, 60, 35, eval($4/2+i+1))
			ElementLine(eval(BASEX-56-NOSMUDGE) Y X2 Y 20)
		')
	')'
	`PKG_SUBD_LAY_BASE(`$1', `$2', `$3', `$4')'
)

# ---------------------------------------------------------------
# definition for a right angle BNC connector,
# such as Amphenol 31-5640-1010 or OUPIIN 8928
# 12/99 Larry Doolittle <LRDoolittle@lbl.gov>
#
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: pins
#
# base is one of the pegs
#
define(`PKG_BNC_LAY',
	`define(`BASEX', 200)
	define(`BASEY', -200)
Element(0x00 "$1" "`$2'" "$3" BASEX eval(BASEY+200) 3 100 0x00)
(
	define(`X1', `eval(BASEX-260)')
	define(`Y1', `eval(BASEY-90)')
	define(`X2', `eval(BASEX+290)')
	define(`Y2', `eval(BASEY+490)')
	ElementLine(X1 Y1 X2 Y1 10)
	ElementLine(X2 Y1 X2 Y2 10)
	ElementLine(X2 Y2 X1 Y2 10)
	ElementLine(X1 Y2 X1 Y1 10)
	PIN(eval(BASEX-200), eval(BASEY+200), 60, 35, 1)
	PIN(eval(BASEX-200), eval(BASEY+100), 60, 35, 2)
	PIN(eval(BASEX),     eval(BASEY),     100, 81, m1)
	PIN(eval(BASEX),     eval(BASEY+400), 100, 81, m2)
)')


# ---------------------------------------------------------------
# definition for MTA-100 header,
# such as AMP 640456-x
#
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: pins
#
#      0.100" pin pitch
#      0.038" drill diameter
#      0.060" pad diameter
#      Available with 2 through 15 pins
#
define(`PKG_MTA_100',
`
        define(`PITCH', `100')
        define(`NPINS', `eval($4)')
        define(`SILKW', `10')
        define(`SILKXOFS', `75')
        define(`SILKYOFS', `115')
        define(`SILKX1', `-SILKXOFS')
        define(`SILKX2', `eval((NPINS-1)*PITCH + SILKXOFS)')
        define(`SILKY1', `-SILKYOFS')
        define(`SILKY2', `SILKYOFS')
        define(`SILKY3', `eval(SILKYOFS+2*SILKW)')

Element(0x00 "$1" "`$2'" "$3" 0  140 0 150 0x00)
(
        forloop(`i', 1, `$4',
                `PIN(eval((i-1)*100), 0, 60, 38, `i')
        ')
        define(`X1',-78)
        define(`X2', `eval(`$4'*100-78)')
        ElementLine(SILKX1  SILKY1 SILKX2 SILKY1 SILKW) 
        ElementLine(SILKX2  SILKY1 SILKX2 SILKY3 SILKW) 
        ElementLine(SILKX2  SILKY2 SILKX1 SILKY2 SILKW) 
        ElementLine(SILKX1  SILKY3 SILKX1 SILKY1 SILKW) 

        ElementLine(SILKX2  SILKY3 SILKX1 SILKY3 SILKW) 

        Mark(0 0)
)')

# ---------------------------------------------------------------
# definition for MTA-156 header,
# such as AMP 640445-x
# 12/99 Larry Doolittle <LRDoolittle@lbl.gov>
#
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: pins
#
# base is pin 1 (although I don't claim to know which end is
#                conventionally pin 1)
#      0.156 pin pitch
#      0.070 holes
#      0.078 end clearance
#      0.180 and 0.120 side clearance
#      Available with 2 through 24 pins
#
define(`PKG_MTA_156',
	`
Element(0x00 "$1" "`$2'" "$3" 0  140 0 150 0x00)
(
	forloop(`i', 1, `$4',
		`PIN(eval((i-1)*156), 0, 100, 70, `i')
	')
	define(`X1',-78)
	define(`X2', `eval(`$4'*156-78)')
	ElementLine(X1  120 X2  120 10) 
	ElementLine(X2  120 X2 -180 10) 
	ElementLine(X2 -180 X1 -180 10) 
	ElementLine(X2  -80 X1  -80 10) 
	ElementLine(X1 -180 X1  120 10)
)')

# ---------------------------------------------------------------
# definition for a vertical SMA connector,
# 6/00 Larry Doolittle <LRDoolittle@lbl.gov>
#
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: pins
#
# base is the center pin
#
define(`PKG_SMA_VERT',
	`define(`BASEX', 0)
	define(`BASEY', 0)
Element(0x00 "$1" "`$2'" "$3" BASEX eval(BASEY+150) 3 100 0x00)
(
	define(`X1', `eval(BASEX-160)')
	define(`Y1', `eval(BASEY-160)')
	define(`X2', `eval(BASEX+160)')
	define(`Y2', `eval(BASEY+160)')
	ElementLine(X1 Y1 X2 Y1 10)
	ElementLine(X2 Y1 X2 Y2 10)
	ElementLine(X2 Y2 X1 Y2 10)
	ElementLine(X1 Y2 X1 Y1 10)
	PIN(     BASEX     ,      BASEY     , 80, 30, 1)
	PIN(eval(BASEX-100), eval(BASEY+100), 80, 30, 2)
	PIN(eval(BASEX-100), eval(BASEY-100), 80, 30, 3)
	PIN(eval(BASEX+100), eval(BASEY+100), 80, 30, 4)
	PIN(eval(BASEX+100), eval(BASEY-100), 80, 30, 5)
)')

# ---------------------------------------------------------------
# definition for a Molex high density connector, 53467/53645
# Board-to-Board 0.635mm (0.25") Ptich Plug - SMT Dual Row, Vertical Stacking
# Reference: http://www.molex.com/product/micro/53467.html
# 4/01 Larry Doolittle <LRDoolittle@lbl.gov>
#
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: pins
#
# base is on the symmetry line, between pins 1 and 2
#
define(`PKG_MOLEX_025',
	`define(`PINPAIRS',`eval($4/2)')
Element(0x00 "$1" "`$2'" "$3" 0 0 3 100 0x00)
(
	forloop(`i', 1, PINPAIRS,
		`define(`Y', eval(25*(i-1)) )'
		`PAD(  54, Y, 111, Y, 14, eval(i*2-1))'
		`PAD(-111, Y, -54, Y, 14, eval(i*2))'
	)
	define(`ENDY',eval(25*(PINPAIRS-1)))

	# Keying is done with two sizes of alignment pins: 35 and 28 mils
	Pin(0 -50 50 35 "M1" 0x01)
	Pin(0 eval(ENDY+50) 43 28 "M2" 0x01)

	# ends of mounting pads are 71 and 169 mils from end pad centers
	PAD(0, -110, 0, -130, 79, M3)
	PAD(0, eval(ENDY+110), 0, eval(ENDY+130), 79, M4)

	define(`BOXY',eval(ENDY+150))
	ElementLine(-100 -150   50 -150 10)
	ElementLine(  50 -150  100 -100 10)
	ElementLine( 100 -100  100 BOXY 10)
	ElementLine( 100 BOXY -100 BOXY 10)
	ElementLine(-100 BOXY -100 -150 10)

	# Support for aggregate parts built from this base, like
	# the nanoEngine below.
	ifdef(`MOLEX_EXTRA', `MOLEX_EXTRA', )
)')

# ---------------------------------------------------------------
# definition for a Bright Star nanoEngine, StrongARM single board computer
# Reference: http://www.brightstar.com/arm/nanoman.pdf
# 4/01 Larry Doolittle <LRDoolittle@lbl.gov>
#
# $1: canonical name
# $2: name on PCB
# $3: value
#
# base borrowed from the Molex connector
#
define(`PKG_NANOENGINE',
	`define(`MOLEX_EXTRA', `
		ElementLine(-175 -215 -175 2185 15)
		ElementLine(-175 2185 1225 2185 15)
		ElementLine(1225 2185 1225 -215 15)
		ElementLine(1225 -215 -175 -215 15)
		Pin(525 -115 125 70 "M5" 0x01)
		Pin(525 2085 125 70 "M6" 0x01)
	')'
	`PKG_MOLEX_025($1, $2, $3, 160)'
)

# -------------------------------------------------------------------
# the definition of a through hole test point
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: drill diameter (mils)
# $5: silk diameter  (mils)
#
define(`PKG_TP',
        `
	define(`DRILL', `$4')
	define(`SILK',  `eval($5/2)')
	define(`PADDIA', `eval(DRILL + 20)')
	define(`SILKW', `10')
Element(0x00 "$1" "`$2'" "$3" -20 eval(-SILK - 60) 0 100 0x00)
(
	Pin( 0 0 PADDIA DRILL "1" "1" 0x0)
	ElementArc(0 0 SILK SILK 0 360 SILKW)
	
	# Mark at the common centroid
        Mark(0 0)
)')
