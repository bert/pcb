# -*- m4 -*-
#
#                            COPYRIGHT
#
#   PCB, interactive printed circuit board design
#   Copyright (C) 2007 Dan McMahill
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; version 2 of the License.
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
#   The symbols produced by this file may be used and redistributed 
#   without restriction as part of a PCB layout file.  Distribution 
#   of footprints produced by this file as part of a footprint
#   library is governed by the same license as this source file.
#

#  C&K Specific Footprints


# ES series surface mount switches

# $1: canonical name
# $2: name on PCB
# $3: value

define(`PKG_CANDK_ES_SMT_SWITCH',
       `
	# pad 1,2,3 width (1/100 mil)
	define(`PADW1',  `3500')

	# pad 1,2,3 length (1/100 mil)
	define(`PADL1',  `8200')

	# x value for pads 1,3 (1/100 mil)
	define(`PADX1', `10000')
	
	# y value for pads 1,2,3 (1/100 mil)
	define(`PADY1', `17500')
	define(`PADY1A', `eval(PADY1 - (PADL1 - PADW1)/2)')
	define(`PADY1B', `eval(PADY1 + (PADL1 - PADW1)/2)')


	# mounting pad width (1/100 mil)
	define(`PADW2',  `9150')

	# mounting pad length (1/100 mil)
	define(`PADL2',  `6000')

	# x value for the mounting pads (1/100 mil)
	define(`PADX2', `21175')
	define(`PADX2A', `eval(PADX2 - (PADW2 - PADL2)/2)')
	define(`PADX2B', `eval(PADX2 + (PADW2 - PADL2)/2)')
	
	# y value for the mounting pads (1/100 mil)
	define(`PADY2', `-17500')

	# package width (1/100 mil)
	define(`PACKW', `39500')

	# package height (1/100 mil)
	define(`PACKH', `36000')

	# component veritcal height off board (1/100 mil)
	define(`COMPH', `eval($9*10000/254)')

	# pad width and length
	define(`PADW', `eval(PINW + 2000)')
	define(`PADL', `eval(PINL + 2000)')

	# y values for drawing the pad.  
	# The Y center of the pad is 0.5*(PINL + PINS)
	# we need a line segment of length PADL - PADW so we have end points:
	# 0.5*(PINL + PINS) +/- 0.5*(PADL - PADW)
	define(`PY1', `eval((PINL + PINS + PADL - PADW)/2)')
	define(`PY2', `eval((PINL + PINS - PADL + PADW)/2)')

	# width of soldermask relief (5 mil on each side)
	define(`MASKW1', `eval(PADW1 + 1000)')
	define(`MASKW2', `eval(PADL2 + 1000)')

	# top edge of switch body (1/100 mil)
	define(`SWTOP', `eval(PADY2 - 5000)')

	# bottom edge of switch body (1/100 mil)
	define(`SWBOT', `eval(SWTOP + PACKH)')

	# how much the switch extends beyond the body
	define(`SWEXT', `12100')

	# y value for the far end of the switch
	define(`SWYMAX', `eval(SWTOP - SWEXT)')

	# silkscreen width (1/100 mils)
	define(`SILKW', `1000')

	# how much space to leave around the part before the
	# silk screen (1/100 mils)
	define(`SILKS', `800')

	# X values for silk on sides and bottom of switch
	define(`SX1', `eval( (PACKW + 2*SILKS + SILKW)/2)')
	define(`SX2', `eval( 10000 + PADW1/2 + SILKS + SILKW/2)')

	# bottom edge of the switch body
	define(`SY1', `eval( SWBOT + SILKS + SILKW/2 )')

	# bottom edge of upper pads
	define(`SY2', `eval( PADY2 + PADL2/2 + SILKS + SILKW/2)')

	# bottom edge of the lower pads
	define(`SY3', `eval( PADY1 + PADL1/2 + SILKS + SILKW/2)')

	# top edge of the switch body
	define(`SY4', `eval( PADY2 - PADL2/2 - SILKS - SILKW/2)')

	# top edge of the switch 
	define(`SY5', `eval( SWTOP - SILKS - SILKW/2)')


# Element [SFlags "Desc" "Name" "Value" MX MY TX TY TDir TScale TSFlags]
Element[ "" "`$1'" "`$2'" "`$3'" 0 0 0 0 0 100 ""]
(

# Pad [rX1 rY1 rX2 rY2 Thickness Clearance Mask "Name" "Number" SFlags]

# the signal pads
Pad[ -PADX1 PADY1A -PADX1 PADY1B PADW1 1000 MASKW1 "1"  "1" "square"]
Pad[      0 PADY1A      0 PADY1B PADW1 1000 MASKW1 "2"  "2" "square"]
Pad[  PADX1 PADY1A  PADX1 PADY1B PADW1 1000 MASKW1 "3"  "3" "square"]

# the mounting pads
Pad[ -PADX2A PADY2 -PADX2B PADY2 PADL2 1000 MASKW2 "4"  "4" "square"]
Pad[  PADX2A PADY2  PADX2B PADY2 PADL2 1000 MASKW2 "4"  "4" "square"]

# Silk screen around package
# ElementLine[ x1 y1 x2 y2 width]

# bottom edge
ElementLine[ SX1 SY1  SX2 SY1 SILKW ]
ElementLine[-SX1 SY1 -SX2 SY1 SILKW ]
ElementLine[-SX2 SY1 -SX2 SY3 SILKW ]
ElementLine[ SX2 SY1  SX2 SY3 SILKW ]
ElementLine[-SX2 SY3  SX2 SY3 SILKW ]

# left/right
ElementLine[ SX1 SY1  SX1 SY2 SILKW ]
ElementLine[-SX1 SY1 -SX1 SY2 SILKW ]

# top edge
ElementLine[-SX1 SWYMAX  SX1 SWYMAX SILKW ]
ElementLine[-SX1 SY5     SX1 SY5    SILKW ]
ElementLine[-SX1 SY4    -SX1 SWYMAX SILKW ]
ElementLine[ SX1 SY4     SX1 SWYMAX SILKW ]

# cross at top where switch moves
ElementLine[-SX1 SWYMAX  SX1 SY5    SILKW ]
ElementLine[-SX1 SY5     SX1 SWYMAX SILKW ]

)')

# -------------------------------------------------------------------

# EXTRACT_BEGIN


#
##  C&K ES Series Switches
#

# See www.ittcannon.com.

define(`PKG_CANDK_ES01MSABE', `PKG_CANDK_ES_SMT_SWITCH(`$1', `$2', `$3')')
define(`PKG_CANDK_ES02MSABE', `PKG_CANDK_ES_SMT_SWITCH(`$1', `$2', `$3')')
define(`PKG_CANDK_ES03MSABE', `PKG_CANDK_ES_SMT_SWITCH(`$1', `$2', `$3')')



# EXTRACT_END

