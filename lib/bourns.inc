# -*- m4 -*-
#
#                            COPYRIGHT
#
#   PCB, interactive printed circuit board design
#   Copyright (C) 2003, 2005 Dan McMahill
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
#  Bourns Specific Footprints


# Surface mount trim pots, such as the 3224 series.
#
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: pin 1,3 width (1/100 mm)     width is in the direction parallel to the
# $5: pin 1,3 length (1/100 mm)    line which intersects pins 1 and 3
# $6: pin 2 width (1/100 mm)
# $7: pin 2 length (1/100 mm)
# $8: spacing from pad center to pad center between 2 and 1,3 (1/100 mm)
# $9: spacing from pad center to pad center between 1 and 3 (1/100 mm)
# $10: package width (1/100 mm)

define(`PKG_BOURNS_GENERIC_POT',
       `
	# pin 1,3 width, length (1/100 mm)
	define(`W13',  `$4')
	define(`L13',  `$5')

	# pin 2 width, length (1/100 mm)
	define(`W2',  `$6')
	define(`L2',  `$7')

	# spacing from pad center to pad center between 2 and 1,3 (1/100 mm)
	define(`S12', `$8')
	# spacing from pad center to pad center between 1 and 3 (1/100 mm)
	define(`S13', `$9')
	
	# package width (1/100 mm)
	define(`WIDTH', `$10')

	# silkscreen width (mils)
	define(`SILKW', `10')

	# how much space to leave around the part before the
	# silk screen (mils)
	define(`SILKS', `8')
	# lower right corner for silk screen (mil)
	ifelse(eval(L2 > L13), 1
		define(`SILKX', `eval((S12+L2)*50/254 + SILKS + SILKW/2)'),
		define(`SILKX', `eval((S12+L13)*50/254 + SILKS + SILKW/2)')
	)
	define(`SILKY', `eval((WIDTH/2)*100/254 + SILKS + SILKW/2)')

	# refdes text size (mil)
	define(`TEXTSIZE', 100)
	# x,y coordinates for refdes label (mil)
	define(`TEXTX', -SILKX)
	define(`TEXTY', `eval(-SILKY - 10 - TEXTSIZE/2)')

Element(0x00 "$1" "`$2'" "$3" TEXTX TEXTY 0 TEXTSIZE 0x00)
(

# pin 1
ifelse(0, eval(W13>L13),
	# Pads which have the perpendicular pad dimension less
	# than or equal to the parallel pad dimension 	
	Pad(eval( (S12 - L13 + W13)*50/254) eval( S13*50/254) 
            eval( (S12 + L13 - W13)*50/254) eval( S13*50/254) eval(W13*100/254) "1" 0x100)
        ,
	# Pads which have the perpendicular pad dimension greater
	# than or equal to the parallel pad dimension 
	Pad(eval( (S12 + L13)*50/254) eval((-S13 - W13 + L13)*50/254)
            eval( (S12 + L13)*50/254) eval((-S13 + W13 - L13)*50/254) eval(L13*100/254) "1" 0x100)
	)

# pin 2
ifelse(0, eval(W2>L2),
	# Pads which have the perpendicular pad dimension less
	# than or equal to the parallel pad dimension 	
	Pad(eval( (-S12 - L2 + W2)*50/254) 0 
            eval( (-S12 + L2 - W2)*50/254) 0 eval(W2*100/254) "2" 0x100)
        ,
	# Pads which have the Y (width) pad dimension greater
	# than or equal to the X (length) pad dimension 
	Pad(eval( -S12*50/254) eval((- W2 + L2)*50/254)
            eval( -S12*50/254) eval((  W2 - L2)*50/254) eval(L2*100/254) "2" 0x100)
	)

# pin 3
ifelse(0, eval(W13>L13),
	# Pads which have the perpendicular pad dimension less
	# than or equal to the parallel pad dimension 	
	Pad(eval( (S12 - L13 + W13)*50/254) eval(-S13*50/254) 
            eval( (S12 + L13 - W13)*50/254) eval(-S13*50/254) eval(W13*100/254) "3" 0x100)
        ,
	# Pads which have the perpendicular pad dimension greater
	# than or equal to the parallel pad dimension 
	Pad(eval( (S12 + L13)*50/254) eval((-S13 - W13 + L13)*50/254)
            eval( (S12 + L13)*50/254) eval((-S13 + W13 - L13)*50/254) eval(L13*100/254) "3" 0x100)
	)

# Silk screen around package
ElementLine( SILKX  SILKY  SILKX -SILKY SILKW)
ElementLine( SILKX -SILKY -SILKX -SILKY SILKW)
ElementLine(-SILKX -SILKY -SILKX  SILKY SILKW)
ElementLine(-SILKX  SILKY  SILKX  SILKY SILKW)

# Mark at the center of the part
Mark(0 0)
)')

# -------------------------------------------------------------------

# EXTRACT_BEGIN


#
##  Bourns 3224 Series SMT Trim Pot
#

define(`PKG_BOURNS_3224G',  `PKG_BOURNS_GENERIC_POT(`$1',`$2',`$3',
	`127', `127', `200', `127', `520', `230', `480')')
define(`PKG_BOURNS_3224J',  `PKG_BOURNS_GENERIC_POT(`$1',`$2',`$3',
	`130', `200', `200', `200', `400', `230', `480')')
define(`PKG_BOURNS_3224W',  `PKG_BOURNS_GENERIC_POT(`$1',`$2',`$3',
	`130', `160', `200', `160', `290', `254', `480')')
define(`PKG_BOURNS_3224X',  `PKG_BOURNS_GENERIC_POT(`$1',`$2',`$3',
	`132', `190', `200', `190', `510', `254', `480')')

# EXTRACT_END

