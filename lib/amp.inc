# -*- m4 -*-
#
#                            COPYRIGHT
#
#   PCB, interactive printed circuit board design
#   Copyright (C) 2003 Dan McMahill
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
# 
#
#  Amp (www.amp.com) Specific Footprints

# -------------------------------------------------------------------
# the definition of a MICTOR connector
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: number of pins
# $5: pad width  (1/1000 mil)
# $6: pad length (1/1000 mil)
# $7: pad pitch (1/1000 mil)
# $8: pad seperation center to center for pads on opposite sides of
#     the package (1/1000 mil)
# $9: define to make the pins get numbered starting with the highest pin
#     instead of pin 1.  Needed for certain brain damaged packages like
#     the Mini-Circuits KK81
# pin 1 will be upper left, pin N/2 will be lower left,
# pin N will be upper right as defined here
define(`COMMON_AMP_MICTOR_MIL',
        `
	# number of pads
	define(`NPADS', `$4')

	# number of segments of 38 pins each
	define(`NSEG', eval(NPADS/38))

	# pad width in 1/1000 mil
        define(`PADWIDTH', `$5')
	# pad length in 1/1000 mil
        define(`PADLENGTH',`$6')
	# pad pitch 1/1000 mil
	define(`PITCH',`$7')
	# seperation between pads on opposite sides 1/1000 mil
	define(`PADSEP',`$8')

	# X coordinates for the right hand column of pads (mils)
        define(`X1', `eval( (PADSEP/2 + PADLENGTH - PADWIDTH/2)/1000)')
        define(`X2', `eval( (PADSEP/2 + PADWIDTH/2)/1000)')

	# silk screen width (mils)
	define(`SILKW', `10')
	define(`SILKSEP', `5')


	# figure out if we have an even or odd number of pins per side
	define(`TMP1', eval(NPADS/4))
	define(`TMP2', eval((4*TMP1 - NPADS) == 0))
	ifelse(TMP2, 1, `define(`EVEN',"yes")', `define(`EVEN',"no")')

	# silk bounding box is -XMAX,-YMAX, XMAX,YMAX (mils)
	define(`XMAX', `200')
	define(`YMAX', `eval(250 + NSEG*250 + 125)')

	define(`REV', `$9')

	ifelse(REV,"reverse",
		`define(`CURPIN', NPADS)'
	,
		`define(`CURPIN', `1')'
	)	
Element(0x00 "$1" "`$2'" "$3" -20 -60 0 100 0x00)
(
	define(`SEG', `1')
	define(`YOFS', eval((1-NSEG)*25000/2))
        forloop(`i', 1, eval(NPADS / 2),
		`
		ifelse(eval(i > SEG*19), 1, 
			`define(`SEG', incr(SEG))
			define(`YOFS', eval((1-NSEG)*25000/2 + (SEG-1)*25000))',)
                ifelse(EVEN,"yes",
		 `Pad(   -X1 eval( (-(NPADS/4)*PITCH - PITCH/2 + i*PITCH + YOFS)/1000) 
			 -X2 eval( (-(NPADS/4)*PITCH - PITCH/2 + i*PITCH + YOFS)/1000) 
			eval(PADWIDTH/1000) "CURPIN" "CURPIN" 0x0)',
		 `Pad(   -X1 eval( (-(NPADS/4)*PITCH - PITCH   + i*PITCH + YOFS)/1000) 
			 -X2 eval( (-(NPADS/4)*PITCH - PITCH   + i*PITCH + YOFS)/1000) 
			eval(PADWIDTH/1000) "CURPIN" "CURPIN" 0x0)')

		ifelse(REV,"reverse", 
			`define(`CURPIN', decr(CURPIN))
			define(`CURPIN', decr(CURPIN))',
			`define(`CURPIN', incr(CURPIN))
			define(`CURPIN', incr(CURPIN))'
			)
        ')
	ifelse(REV,"reverse",
		`define(`CURPIN', `2')'
	,
		`define(`CURPIN', `NPADS')'
	)	

	define(`SEG', `1')
	define(`YOFS', eval((NSEG-1)*25000/2))
        forloop(`i', eval((NPADS / 2) + 1), NPADS,
		`
		ifelse(eval(i - NPADS/2 > SEG*19), 1, 
			`define(`SEG', incr(SEG)) 
			define(`YOFS', eval((NSEG-1)*25000/2 - (SEG-1)*25000))',)
                ifelse(EVEN,"yes",
		 `Pad(   X1 eval( ((NPADS/4)*PITCH + PITCH/2 - (i-NPADS/2)*PITCH + YOFS)/1000) 
			 X2 eval( ((NPADS/4)*PITCH + PITCH/2 - (i-NPADS/2)*PITCH + YOFS)/1000) 
			eval(PADWIDTH/1000) "CURPIN" "CURPIN" 0x0)',
		 `Pad(   X1 eval( ((NPADS/4)*PITCH + PITCH   - (i-NPADS/2)*PITCH + YOFS)/1000) 
			 X2 eval( ((NPADS/4)*PITCH + PITCH   - (i-NPADS/2)*PITCH + YOFS)/1000) 
			eval(PADWIDTH/1000) "CURPIN" "CURPIN" 0x0)')
		ifelse(REV,"reverse", 
			`define(`CURPIN', incr(CURPIN))
			define(`CURPIN', incr(CURPIN))',
			`define(`CURPIN', decr(CURPIN))
			define(`CURPIN', decr(CURPIN))'
			)
        ')

	define(`GNDpad', `60')
	define(`GNDdrl', `32')
# now add the center row of grounding pins
	define(`CURPIN', incr(NPADS))
	define(`YOFS', `eval(250-NSEG*250)')
	forloop(`j', 1, NSEG,
	`forloop(`i', -2, 2,
		`Pin(0 eval(i*100 + YOFS) GNDpad GNDdrl "GND" "CURPIN" 0x01)
		define(`CURPIN', incr(CURPIN))'
	)
	define(`YOFS', eval(YOFS + 500))'
	)
# the latch pins
	Pin(0 eval(-YOFS - 55) 80 53 "LATCH" "CURPIN" 0x01)
	define(`CURPIN', incr(CURPIN))
	Pin(0  eval(YOFS + 55) 80 53 "LATCH" "CURPIN" 0x01)
	define(`CURPIN', incr(CURPIN))
# and the orientation pin
	Pin(0 eval(-YOFS+50) 84 84 "ORIENT" "CURPIN" 0x09)

# and finally the silk screen
	ElementLine(-XMAX -YMAX -XMAX  YMAX SILKW)
	ElementLine(-XMAX  YMAX  XMAX  YMAX SILKW)
	ElementLine( XMAX  YMAX  XMAX -YMAX SILKW)
	ElementLine(-XMAX -YMAX   -25 -YMAX SILKW)
	ElementLine( XMAX -YMAX    25 -YMAX SILKW)

	# punt on the arc on small parts as it can cover the pads
	ifelse(eval((PADSEP-PADLENGTH)/1000 > 50), 1, `ElementArc(0 -YMAX 25 25 0 180 SILKW)', )
	
	# Mark at the common centroid
        Mark(0 0)
)')

# dimensions are given in 1/100 mm.
# $5-$8 are pad width, length, pitch, and sep
define(`COMMON_AMP_MICTOR_MM',  `COMMON_AMP_MICTOR_MIL(`$1',`$2',`$3',`$4',
	eval($5*100000/254),eval($6*100000/254),eval($7*100000/254),eval($8*100000/254), `$9')')


define(`COMMON_AMP_MICTOR_767054',  `COMMON_AMP_MICTOR_MIL(`$1',`$2',`$3',`$4',17000,50000,25000,278000)')

# EXTRACT_BEGIN

#
##  Amp Mictor Connectors
#
define(`PKG_AMP_MICTOR_767054_1',  `COMMON_AMP_MICTOR_767054(`$1',`$2',`$3',38)')
define(`PKG_AMP_MICTOR_767054_2',  `COMMON_AMP_MICTOR_767054(`$1',`$2',`$3',76)')
define(`PKG_AMP_MICTOR_767054_3',  `COMMON_AMP_MICTOR_767054(`$1',`$2',`$3',114)')
define(`PKG_AMP_MICTOR_767054_4',  `COMMON_AMP_MICTOR_767054(`$1',`$2',`$3',152)')
define(`PKG_AMP_MICTOR_767054_5',  `COMMON_AMP_MICTOR_767054(`$1',`$2',`$3',190)')
define(`PKG_AMP_MICTOR_767054_6',  `COMMON_AMP_MICTOR_767054(`$1',`$2',`$3',228)')
define(`PKG_AMP_MICTOR_767054_7',  `COMMON_AMP_MICTOR_767054(`$1',`$2',`$3',266)')

# EXTRACT_END

