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
## Nichicon LGX Series of Radial Snap-in Aluminum Electrolytic Capacitors
#

# See publication CAT.8100F.  For the LGX series:
# pin spacing = 10mm +/- 0.1 mm
# lead diameter = 0.8 mm +0.2/-0.1mm for the thin part,
#                 1.5 mm +0.2/-0.1mm for the thick part
# Case size specified in case diameter +1.5mm max x case height +/- 2mm

# _M = most
# _N = nominal
# _L = leaset

# 22x25mm
define(`PKG_NICHICON_LGX_22_25_M', `PKG_CAPPRD1000W170D2200H2500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_22_25_N', `PKG_CAPPRD1000W170D2200H2500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_22_25_L', `PKG_CAPPRD1000W170D2200H2500C(  `$1', `$2', `$3')');

# 22x30mm
define(`PKG_NICHICON_LGX_22_30_M', `PKG_CAPPRD1000W170D2200H3000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_22_30_N', `PKG_CAPPRD1000W170D2200H3000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_22_30_L', `PKG_CAPPRD1000W170D2200H3000C(  `$1', `$2', `$3')');

# 22x35mm
define(`PKG_NICHICON_LGX_22_35_M', `PKG_CAPPRD1000W170D2200H3500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_22_35_N', `PKG_CAPPRD1000W170D2200H3500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_22_35_L', `PKG_CAPPRD1000W170D2200H3500C(  `$1', `$2', `$3')');

# 22x40mm
define(`PKG_NICHICON_LGX_22_40_M', `PKG_CAPPRD1000W170D2200H4000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_22_40_N', `PKG_CAPPRD1000W170D2200H4000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_22_40_L', `PKG_CAPPRD1000W170D2200H4000C(  `$1', `$2', `$3')');

# 22x45mm
define(`PKG_NICHICON_LGX_22_45_M', `PKG_CAPPRD1000W170D2200H4500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_22_45_N', `PKG_CAPPRD1000W170D2200H4500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_22_45_L', `PKG_CAPPRD1000W170D2200H4500C(  `$1', `$2', `$3')');

# 22x50mm
define(`PKG_NICHICON_LGX_22_50_M', `PKG_CAPPRD1000W170D2200H5000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_22_50_N', `PKG_CAPPRD1000W170D2200H5000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_22_50_L', `PKG_CAPPRD1000W170D2200H5000C(  `$1', `$2', `$3')');

# 25x25mm
define(`PKG_NICHICON_LGX_25_25_M', `PKG_CAPPRD1000W170D2500H2500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_25_25_N', `PKG_CAPPRD1000W170D2500H2500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_25_25_L', `PKG_CAPPRD1000W170D2500H2500C(  `$1', `$2', `$3')');

# 25x30mm
define(`PKG_NICHICON_LGX_25_30_M', `PKG_CAPPRD1000W170D2500H3000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_25_30_N', `PKG_CAPPRD1000W170D2500H3000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_25_30_L', `PKG_CAPPRD1000W170D2500H3000C(  `$1', `$2', `$3')');

# 25x35mm
define(`PKG_NICHICON_LGX_25_35_M', `PKG_CAPPRD1000W170D2500H3500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_25_35_N', `PKG_CAPPRD1000W170D2500H3500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_25_35_L', `PKG_CAPPRD1000W170D2500H3500C(  `$1', `$2', `$3')');

# 25x40mm
define(`PKG_NICHICON_LGX_25_40_M', `PKG_CAPPRD1000W170D2500H4000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_25_40_N', `PKG_CAPPRD1000W170D2500H4000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_25_40_L', `PKG_CAPPRD1000W170D2500H4000C(  `$1', `$2', `$3')');

# 25x45mm
define(`PKG_NICHICON_LGX_25_45_M', `PKG_CAPPRD1000W170D2500H4500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_25_45_N', `PKG_CAPPRD1000W170D2500H4500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_25_45_L', `PKG_CAPPRD1000W170D2500H4500C(  `$1', `$2', `$3')');

# 25x50mm
define(`PKG_NICHICON_LGX_25_50_M', `PKG_CAPPRD1000W170D2500H5000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_25_50_N', `PKG_CAPPRD1000W170D2500H5000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_25_50_L', `PKG_CAPPRD1000W170D2500H5000C(  `$1', `$2', `$3')');

# 30x25mm
define(`PKG_NICHICON_LGX_30_25_M', `PKG_CAPPRD1000W170D3000H2500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_30_25_N', `PKG_CAPPRD1000W170D3000H2500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_30_25_L', `PKG_CAPPRD1000W170D3000H2500C(  `$1', `$2', `$3')');

# 30x30mm
define(`PKG_NICHICON_LGX_30_30_M', `PKG_CAPPRD1000W170D3000H3000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_30_30_N', `PKG_CAPPRD1000W170D3000H3000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_30_30_L', `PKG_CAPPRD1000W170D3000H3000C(  `$1', `$2', `$3')');

# 30x35mm
define(`PKG_NICHICON_LGX_30_35_M', `PKG_CAPPRD1000W170D3000H3500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_30_35_N', `PKG_CAPPRD1000W170D3000H3500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_30_35_L', `PKG_CAPPRD1000W170D3000H3500C(  `$1', `$2', `$3')');

# 30x40mm
define(`PKG_NICHICON_LGX_30_40_M', `PKG_CAPPRD1000W170D3000H4000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_30_40_N', `PKG_CAPPRD1000W170D3000H4000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_30_40_L', `PKG_CAPPRD1000W170D3000H4000C(  `$1', `$2', `$3')');

# 30x45mm
define(`PKG_NICHICON_LGX_30_45_M', `PKG_CAPPRD1000W170D3000H4500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_30_45_N', `PKG_CAPPRD1000W170D3000H4500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_30_45_L', `PKG_CAPPRD1000W170D3000H4500C(  `$1', `$2', `$3')');

# 30x50mm
define(`PKG_NICHICON_LGX_30_50_M', `PKG_CAPPRD1000W170D3000H5000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_30_50_N', `PKG_CAPPRD1000W170D3000H5000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_30_50_L', `PKG_CAPPRD1000W170D3000H5000C(  `$1', `$2', `$3')');

# 35x25mm
define(`PKG_NICHICON_LGX_35_25_M', `PKG_CAPPRD1000W170D3500H2500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_35_25_N', `PKG_CAPPRD1000W170D3500H2500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_35_25_L', `PKG_CAPPRD1000W170D3500H2500C(  `$1', `$2', `$3')');

# 35x30mm
define(`PKG_NICHICON_LGX_35_30_M', `PKG_CAPPRD1000W170D3500H3000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_35_30_N', `PKG_CAPPRD1000W170D3500H3000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_35_30_L', `PKG_CAPPRD1000W170D3500H3000C(  `$1', `$2', `$3')');

# 35x35mm
define(`PKG_NICHICON_LGX_35_35_M', `PKG_CAPPRD1000W170D3500H3500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_35_35_N', `PKG_CAPPRD1000W170D3500H3500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_35_35_L', `PKG_CAPPRD1000W170D3500H3500C(  `$1', `$2', `$3')');

# 35x40mm
define(`PKG_NICHICON_LGX_35_40_M', `PKG_CAPPRD1000W170D3500H4000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_35_40_N', `PKG_CAPPRD1000W170D3500H4000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_35_40_L', `PKG_CAPPRD1000W170D3500H4000C(  `$1', `$2', `$3')');

# 35x45mm
define(`PKG_NICHICON_LGX_35_45_M', `PKG_CAPPRD1000W170D3500H4500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_35_45_N', `PKG_CAPPRD1000W170D3500H4500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_35_45_L', `PKG_CAPPRD1000W170D3500H4500C(  `$1', `$2', `$3')');

# 35x50mm
define(`PKG_NICHICON_LGX_35_50_M', `PKG_CAPPRD1000W170D3500H5000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_35_50_N', `PKG_CAPPRD1000W170D3500H5000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGX_35_50_L', `PKG_CAPPRD1000W170D3500H5000C(  `$1', `$2', `$3')');

#
## Nichicon LGY Series of Radial Snap-in Aluminum Electrolytic Capacitors
#

# pins are identical to LGX series
#
# _M = most
# _N = nominal
# _L = leaset

# 22x25mm
define(`PKG_NICHICON_LGY_22_25_M', `PKG_CAPPRD1000W170D2200H2500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_22_25_N', `PKG_CAPPRD1000W170D2200H2500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_22_25_L', `PKG_CAPPRD1000W170D2200H2500C(  `$1', `$2', `$3')');

# 22x30mm
define(`PKG_NICHICON_LGY_22_30_M', `PKG_CAPPRD1000W170D2200H3000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_22_30_N', `PKG_CAPPRD1000W170D2200H3000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_22_30_L', `PKG_CAPPRD1000W170D2200H3000C(  `$1', `$2', `$3')');

# 22x35mm
define(`PKG_NICHICON_LGY_22_35_M', `PKG_CAPPRD1000W170D2200H3500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_22_35_N', `PKG_CAPPRD1000W170D2200H3500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_22_35_L', `PKG_CAPPRD1000W170D2200H3500C(  `$1', `$2', `$3')');

# 22x40mm
define(`PKG_NICHICON_LGY_22_40_M', `PKG_CAPPRD1000W170D2200H4000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_22_40_N', `PKG_CAPPRD1000W170D2200H4000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_22_40_L', `PKG_CAPPRD1000W170D2200H4000C(  `$1', `$2', `$3')');

# 22x45mm
define(`PKG_NICHICON_LGY_22_45_M', `PKG_CAPPRD1000W170D2200H4500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_22_45_N', `PKG_CAPPRD1000W170D2200H4500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_22_45_L', `PKG_CAPPRD1000W170D2200H4500C(  `$1', `$2', `$3')');

# 22x50mm
define(`PKG_NICHICON_LGY_22_50_M', `PKG_CAPPRD1000W170D2200H5000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_22_50_N', `PKG_CAPPRD1000W170D2200H5000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_22_50_L', `PKG_CAPPRD1000W170D2200H5000C(  `$1', `$2', `$3')');

# 25x25mm
define(`PKG_NICHICON_LGY_25_25_M', `PKG_CAPPRD1000W170D2500H2500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_25_25_N', `PKG_CAPPRD1000W170D2500H2500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_25_25_L', `PKG_CAPPRD1000W170D2500H2500C(  `$1', `$2', `$3')');

# 25x30mm
define(`PKG_NICHICON_LGY_25_30_M', `PKG_CAPPRD1000W170D2500H3000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_25_30_N', `PKG_CAPPRD1000W170D2500H3000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_25_30_L', `PKG_CAPPRD1000W170D2500H3000C(  `$1', `$2', `$3')');

# 25x35mm
define(`PKG_NICHICON_LGY_25_35_M', `PKG_CAPPRD1000W170D2500H3500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_25_35_N', `PKG_CAPPRD1000W170D2500H3500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_25_35_L', `PKG_CAPPRD1000W170D2500H3500C(  `$1', `$2', `$3')');

# 25x40mm
define(`PKG_NICHICON_LGY_25_40_M', `PKG_CAPPRD1000W170D2500H4000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_25_40_N', `PKG_CAPPRD1000W170D2500H4000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_25_40_L', `PKG_CAPPRD1000W170D2500H4000C(  `$1', `$2', `$3')');

# 25x45mm
define(`PKG_NICHICON_LGY_25_45_M', `PKG_CAPPRD1000W170D2500H4500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_25_45_N', `PKG_CAPPRD1000W170D2500H4500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_25_45_L', `PKG_CAPPRD1000W170D2500H4500C(  `$1', `$2', `$3')');

# 25x50mm
define(`PKG_NICHICON_LGY_25_50_M', `PKG_CAPPRD1000W170D2500H5000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_25_50_N', `PKG_CAPPRD1000W170D2500H5000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_25_50_L', `PKG_CAPPRD1000W170D2500H5000C(  `$1', `$2', `$3')');

# 30x25mm
define(`PKG_NICHICON_LGY_30_25_M', `PKG_CAPPRD1000W170D3000H2500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_30_25_N', `PKG_CAPPRD1000W170D3000H2500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_30_25_L', `PKG_CAPPRD1000W170D3000H2500C(  `$1', `$2', `$3')');

# 30x30mm
define(`PKG_NICHICON_LGY_30_30_M', `PKG_CAPPRD1000W170D3000H3000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_30_30_N', `PKG_CAPPRD1000W170D3000H3000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_30_30_L', `PKG_CAPPRD1000W170D3000H3000C(  `$1', `$2', `$3')');

# 30x35mm
define(`PKG_NICHICON_LGY_30_35_M', `PKG_CAPPRD1000W170D3000H3500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_30_35_N', `PKG_CAPPRD1000W170D3000H3500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_30_35_L', `PKG_CAPPRD1000W170D3000H3500C(  `$1', `$2', `$3')');

# 30x40mm
define(`PKG_NICHICON_LGY_30_40_M', `PKG_CAPPRD1000W170D3000H4000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_30_40_N', `PKG_CAPPRD1000W170D3000H4000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_30_40_L', `PKG_CAPPRD1000W170D3000H4000C(  `$1', `$2', `$3')');

# 30x45mm
define(`PKG_NICHICON_LGY_30_45_M', `PKG_CAPPRD1000W170D3000H4500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_30_45_N', `PKG_CAPPRD1000W170D3000H4500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_30_45_L', `PKG_CAPPRD1000W170D3000H4500C(  `$1', `$2', `$3')');

# 30x50mm
define(`PKG_NICHICON_LGY_30_50_M', `PKG_CAPPRD1000W170D3000H5000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_30_50_N', `PKG_CAPPRD1000W170D3000H5000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_30_50_L', `PKG_CAPPRD1000W170D3000H5000C(  `$1', `$2', `$3')');

# 35x25mm
define(`PKG_NICHICON_LGY_35_25_M', `PKG_CAPPRD1000W170D3500H2500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_35_25_N', `PKG_CAPPRD1000W170D3500H2500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_35_25_L', `PKG_CAPPRD1000W170D3500H2500C(  `$1', `$2', `$3')');

# 35x30mm
define(`PKG_NICHICON_LGY_35_30_M', `PKG_CAPPRD1000W170D3500H3000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_35_30_N', `PKG_CAPPRD1000W170D3500H3000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_35_30_L', `PKG_CAPPRD1000W170D3500H3000C(  `$1', `$2', `$3')');

# 35x35mm
define(`PKG_NICHICON_LGY_35_35_M', `PKG_CAPPRD1000W170D3500H3500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_35_35_N', `PKG_CAPPRD1000W170D3500H3500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_35_35_L', `PKG_CAPPRD1000W170D3500H3500C(  `$1', `$2', `$3')');

# 35x40mm
define(`PKG_NICHICON_LGY_35_40_M', `PKG_CAPPRD1000W170D3500H4000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_35_40_N', `PKG_CAPPRD1000W170D3500H4000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_35_40_L', `PKG_CAPPRD1000W170D3500H4000C(  `$1', `$2', `$3')');

# 35x45mm
define(`PKG_NICHICON_LGY_35_45_M', `PKG_CAPPRD1000W170D3500H4500A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_35_45_N', `PKG_CAPPRD1000W170D3500H4500B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_35_45_L', `PKG_CAPPRD1000W170D3500H4500C(  `$1', `$2', `$3')');

# 35x50mm
define(`PKG_NICHICON_LGY_35_50_M', `PKG_CAPPRD1000W170D3500H5000A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_35_50_N', `PKG_CAPPRD1000W170D3500H5000B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_LGY_35_50_L', `PKG_CAPPRD1000W170D3500H5000C(  `$1', `$2', `$3')');

#
## Nichicon UPT Series of Radial Aluminum Electrolytic Capacitors
#

#
# _M = most
# _N = nominal
# _L = leaset
#
# For the UPT series, refer to the following table in mm:
#
# D  10   | 12.5  | 16   | 18    | 20   | 22   | 25   | case diameter
# P   5.0 |  5.0  |  7.5 |  7.5  | 10   | 10   | 12.5 | pin spacing
# d   06  |  0.6  |  0.8 |  0.8  |  1.0 |  1.0 |  1.0 | pin diameter
# d*      |  0.8  |                                   | alternate pin diameter for L > 25

# 18x31.5mm
define(`PKG_NICHICON_UPT_18_31p5_M', `PKG_CAPPRD750W80D1800H3150A(  `$1', `$2', `$3')');
define(`PKG_NICHICON_UPT_18_31p5_N', `PKG_CAPPRD750W80D1800H3150B(  `$1', `$2', `$3')');
define(`PKG_NICHICON_UPT_18_31p5_L', `PKG_CAPPRD750W80D1800H3150C(  `$1', `$2', `$3')');

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

