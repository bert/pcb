# -*- m4 -*-
#
#                            COPYRIGHT
#
#   PCB, interactive printed circuit board design
#   Copyright (C) 2017 Dan McMahill
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; version 2 of the License
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
#
#
#  Toko Specific Footprints

# -------------------------------------------------------------------
# the definition of a more general axial package
# $1:  canonical name
# $2:  name on PCB
# $3:  value
# $4:  signal pin diameter max [1/100 mm]
# $5:  case pin diameter max [1/100 mm]
# $6:  signal pin grid size [1/100 mm]
# $7:  case size (assumed square) [1/100 mm]
define(`PKG_TOKO_INDV_SQ',
	# grab the input values and convert to 1/100 mil
	`
	define(`PIND_MM100', `eval( `$4')')
	define(`GNDD_MM100', `eval( `$5')')
	define(`PINXY', `eval( (`$6' * 10000) / 254)')
	define(`SILKW', `800')
	define(`SILKXY', `eval( ($7 * 10000) / 254 / 2 )')
	# add to the pin size to get the drill size and round
	# to the nearest .05mm to get a standard drill.  The
	# +3 causes us to round up
	define(`PINDRILL_MM100', `eval(5*( (PIND_MM100 + 20 + 3) / 5 ) )')
	define(`PINDRILL', `eval((PINDRILL_MM100 * 10000) / 254 )')

	define(`PINPAD_MM100', `eval(PINDRILL_MM100 + 70)')
	define(`PINPAD', `eval((PINPAD_MM100 * 10000) / 254 )')
	define(`PINCLEAR', `2000')
	define(`PINMASK', `eval(PINPAD + 500)')

	define(`GNDDRILL_MM100', `eval(5*( (GNDD_MM100 + 20 + 3)/5 ) )')
	define(`GNDDRILL', `eval((GNDDRILL_MM100 * 10000) / 254 )')

	define(`GNDPAD_MM100', `eval(GNDDRILL_MM100 + 70)')
	define(`GNDPAD', `eval((GNDPAD_MM100 * 10000) / 254 )')
	define(`GNDCLEAR', `2000')
	define(`GNDMASK', `eval(GNDPAD + 500)')
	
# element_flags, description, pcb-name, value, mark_x, mark_y,
# text_x, text_y, text_direction, text_scale, text_flags
Element[0x00000000 "$1" "`$2'" "$3" 0 0 -SILKXY SILKXY 0 100 ""]
(
# Pin[x, y, thickness, clearance, mask, drilling hole, name,
#     number, flags
Pin[  PINXY  PINXY PINPAD PINCLEAR PINMASK PINDRILL "1" "1" 0x0]
Pin[      0  PINXY PINPAD PINCLEAR PINMASK PINDRILL "2" "2" 0x0]
Pin[ -PINXY  PINXY PINPAD PINCLEAR PINMASK PINDRILL "3" "3" 0x0]
Pin[ -PINXY -PINXY PINPAD PINCLEAR PINMASK PINDRILL "4" "4" 0x0]
Pin[  PINXY -PINXY PINPAD PINCLEAR PINMASK PINDRILL "6" "6" 0x0]

Pin[  SILKXY  0 GNDPAD GNDCLEAR GNDMASK GNDDRILL "CASE1" "7" 0x0]
Pin[ -SILKXY  0 GNDPAD GNDCLEAR GNDMASK GNDDRILL "CASE2" "8" 0x0]

# Silk screen around package
ElementLine[ SILKXY  SILKXY  SILKXY -SILKXY SILKW]
ElementLine[ SILKXY -SILKXY -SILKXY -SILKXY SILKW]
ElementLine[-SILKXY -SILKXY -SILKXY  SILKXY SILKW]
ElementLine[-SILKXY  SILKXY  SILKXY  SILKXY SILKW] 


)')

# EXTRACT_BEGIN

#
## Toko 7P Variable Coils
#

define(`PKG_TOKO_7P', `PKG_TOKO_INDV_SQ(  `$1', `$2', `$3', 50, 100, 225, 700)');


# EXTRACT_END
