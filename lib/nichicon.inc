# -*- m4 -*-
#
# $Id$
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
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
# 
#   The symbols produced by this file may be used and redistributed 
#   without restriction as part of a PCB layout file.  Distribution 
#   of footprints produced by this file as part of a footprint
#   library is governed by the same license as this source file.
#

#  Nichicon Specific Footprints


# Surface mount electrolytic capacitors.  WT series.

# $1: canonical name
# $2: name on PCB
# $3: value
# $4: pin width (1/100 mm)        (max dimension "H" on datasheet * 100)
# $5: pin length (1/100 mm)       (max dimension "A" on datasheet * 100)
# $6: gap between pins (1/100 mm) (min dimension "E" on datasheet * 100)
# $7: package width (1/100 mm)    (max dimension "C" on datasheet * 100)
# $8: package height (1/100 mm)   (max dimension "B" on datasheet * 100)
# $9: package vertical height (1/100mm) -- vertical height off board
#                                 (max dimension "L" on datasheet * 100)

define(`PKG_NICHICON_WT_CAP',
       `
	# max pin width (perpendicular to axis of package) (1/100 mil)
	define(`PINW',  `eval($4*10000/254)')

	# max pin length (parallel to axis of package) (1/100 mil)
	define(`PINL',  `eval($5*10000/254)')

	# gap between the pads (1/100 mil)
	define(`PINS', `eval($6*10000/254)')

	# package width (1/100 mil)
	define(`PACKW', `eval($7*10000/254)')

	# package height (1/100 mil)
	define(`PACKH', `eval($8*10000/254)')

	# component veritcal height off board (1/100 mil)
	define(`COMPH', `eval($9*10000/254)')

	# pad width and length
	define(`PADW', `eval(PINW + 2000)')
	define(`PADL', `eval(PINL + 2000)')

	# y values for drawing the pad.  The Y center of the pad is 0.5*(PINL + PINS)
	# we need a line segment of length PADL - PADW so we have end points:
	# 0.5*(PINL + PINS) +/- 0.5*(PADL - PADW)
	define(`PY1', `eval((PINL + PINS + PADL - PADW)/2)')
	define(`PY2', `eval((PINL + PINS - PADL + PADW)/2)')

	# width of soldermask relief (5 mil on each side)
	define(`MASKW', `eval(PADW + 1000)')

	# silkscreen width (1/100 mils)
	define(`SILKW', `1000')

	# how much space to leave around the part before the
	# silk screen (1/100 mils)
	define(`SILKS', `800')

	# lower left corner for silk screen (1/100 mil)
	define(`LLX', `eval( (PACKW + 2*SILKS + SILKW)/2)')
	define(`LLY', `eval( (PACKH + 2*SILKS + SILKW)/2)')

	# upper right corner for silk screen (1/100 mil)
	define(`URX', `eval( (PADW + 2*SILKS + SILKW)/2)')
	define(`URY', `eval( -LLY)')

	# how much to notch the corners by in silk to indicate polarity
	define(`NOTCH', `3000')
	define(`NOTCHX', `eval(LLX - NOTCH)')
	define(`NOTCHY', `eval(URY + NOTCH)')

# Element [SFlags "Desc" "Name" "Value" MX MY TX TY TDir TScale TSFlags]
Element[ "" "`$1'" "`$2'" "`$3'" 0 0 0 0 0 100 ""]
(

# Pad [rX1 rY1 rX2 rY2 Thickness Clearance Mask "Name" "Number" SFlags]                                                                                              
Pad[ 0 -PY1 0 -PY2 PADW 1000 MASKW "Plus"  "1" "square"]
Pad[ 0  PY1 0  PY2 PADW 1000 MASKW "Minus" "2" "square"]

# Silk screen around package
# ElementLine[ x1 y1 x2 y2 width]

ElementLine[ LLX    LLY     URX    LLY    SILKW]
ElementLine[ LLX    LLY     LLX    NOTCHY SILKW]
ElementLine[ LLX    NOTCHY  NOTCHX URY    SILKW]
ElementLine[ NOTCHX URY     URX    URY    SILKW]

ElementLine[ -LLX    LLY    -URX    LLY    SILKW]
ElementLine[ -LLX    LLY    -LLX    NOTCHY SILKW]
ElementLine[ -LLX    NOTCHY -NOTCHX URY    SILKW]
ElementLine[ -NOTCHX URY    -URX    URY    SILKW]



)')

# -------------------------------------------------------------------

# EXTRACT_BEGIN


#
##  Nichicon WT series SMT Aluminum Electrolytic Capacitor
#

# See Nichicon publication CAT.8100S

# datasheet dimensions Hmax, Amax, E, Cmax, Bmax, Lmax
define(`PKG_NICHICON_WT_CAP_3_5p4',  `PKG_NICHICON_WT_CAP(`$1', `$2', `$3',
	`80', `170', `80', `350', `350', `590')')
define(`PKG_NICHICON_WT_CAP_4_5p4',  `PKG_NICHICON_WT_CAP(`$1', `$2', `$3',
	`80', `200', `100', `450', `450', `590')')
define(`PKG_NICHICON_WT_CAP_5_5p4',  `PKG_NICHICON_WT_CAP(`$1', `$2', `$3',
	`80', `230', `130', `550', `550', `590')')
define(`PKG_NICHICON_WT_CAP_6p3_5p4',  `PKG_NICHICON_WT_CAP(`$1', `$2', `$3',
	`80', `260', `220', `680', `680', `590')')
define(`PKG_NICHICON_WT_CAP_6p3_5p8',  `PKG_NICHICON_WT_CAP(`$1', `$2', `$3',
	`80', `260', `220', `680', `680', `630')')
define(`PKG_NICHICON_WT_CAP_6p3_7p7',  `PKG_NICHICON_WT_CAP(`$1', `$2', `$3',
	`80', `260', `220', `680', `680', `820')')
define(`PKG_NICHICON_WT_CAP_8_5p4',  `PKG_NICHICON_WT_CAP(`$1', `$2', `$3',
	`80', `350', `230', `850', `850', `590')')
define(`PKG_NICHICON_WT_CAP_8_10',  `PKG_NICHICON_WT_CAP(`$1', `$2', `$3',
	`110', `310', `310', `850', `850', `1050')')
define(`PKG_NICHICON_WT_CAP_10_10',  `PKG_NICHICON_WT_CAP(`$1', `$2', `$3',
	`110', `340', `450', `1050', `1050', `1050')')


# EXTRACT_END

