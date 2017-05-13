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

# Optek Specific Footprints -- see www.optekinc.com


# Surface mount LED lamps, Package "RA 2.8x1.2x.8"  for example part number OVSRWACR6

# $1: canonical name
# $2: name on PCB
# $3: value

define(`PKG_OPTEK_RA_2p8_1p2_0p8',
	`
	# pad width and length (1/100 mil)
	define(`PADW',  `eval(140 * 10000/254)')
	define(`PADL',  `eval(90 * 10000/254)')

	# pad center (X)  (1/100 mil)
	define(`PADC',  `eval(120 * 10000/254)')

	# x values for the pads
	define(`PX1', `eval(PADC - (PADL - PADW)/2)')
	define(`PX2', `eval(PADC + (PADL - PADW)/2)')

	# package width (1/100 mil)
	define(`PACKW', `39500')

	# package height (1/100 mil)
	define(`PACKH', `36000')

	# width of soldermask relief (5 mil on each side)
	define(`MASKW', `eval(PADL + 1000)')

	# silkscreen width (1/100 mils)
	define(`SILKW', `1000')

	# how much space to leave around the part before the
	# silk screen (1/100 mils)
	define(`SILKS', `800')

	# top edge silk
	define(`SXTOP', `eval( 50*10000/254 - SILKS - SILKW/2)')
	define(`SYTOP', `eval( -45 * 10000/254)')

	# bottom edge silk
	define(`SXBOT', `eval( 5500 + SILKS + SILKW/2 )')
	define(`SYBOT1', `eval( PADL/2 + SILKS + SILKW/2 )')
	define(`SYBOT2', `eval( -1400 + 4700 + SILKS + SILKW/2 )')

# Element [SFlags "Desc" "Name" "Value" MX MY TX TY TDir TScale TSFlags]
Element[ "" "`$1'" "`$2'" "`$3'" 0 0 0 0 0 100 ""]
(

# Pad [rX1 rY1 rX2 rY2 Thickness Clearance Mask "Name" "Number" SFlags]

# the pads
Pad[ -PX1 0 -PX2 0 PADL 1000 MASKW "K"  "1" "square"]
Pad[  PX1 0  PX2 0 PADL 1000 MASKW "A"  "2" "square"]


# Silk screen around package
# ElementLine[ x1 y1 x2 y2 width]

# top edge
ElementLine[ -SXTOP SYTOP SXTOP SYTOP SILKW ]

# left/right and bottom
ElementLine[ -SXBOT SYBOT1 -SXBOT SYBOT2 SILKW ]
ElementLine[  SXBOT SYBOT1  SXBOT SYBOT2 SILKW ]
ElementLine[ -SXBOT SYBOT2  SXBOT SYBOT2 SILKW ]

)')

# -------------------------------------------------------------------

# EXTRACT_BEGIN


#
##  OPTEK Surface Mount LED's
#

# See www.optekinc.com

define(`PKG_OPTEK_OVSRWACR6', `PKG_OPTEK_RA_2p8_1p2_0p8(`$1', `$2', `$3')')

# EXTRACT_END

