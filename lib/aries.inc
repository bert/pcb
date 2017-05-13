# -*- m4 -*-
#
#                            COPYRIGHT
#
#   PCB, interactive printed circuit board design
#   Copyright (C) 2015 Dan McMahill
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
#  Aries Electronics, Inc. Specific Footprints


# -------------------------------------------------------------------
# the definition of a dual-inline package for Aries ZIF sockets
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: number of pins
# $5: package size (300, 600, 900 + 100 for socket space)
# $6: flag for lever location (1 = left, 2 = right)
#
define(`PKG_ARIES_DIP',
	`
	define(`PINSPACE', `100')
	define(`PADSIZE',   `68')
	define(`DRILLSIZE', `32')

	define(`MTPADSIZE',   `105')
	define(`MTDRILLSIZE', `65')

	define(`MAXY', `eval(((`$4' / 2) - 1) * PINSPACE)')
	define(`MAXX', `eval(`$5' + 100)')
	define(`SILKXL', `-475')
	define(`SILKXR', `475')

	define(`SILKYT', `eval(180 + 87 - 590 - 25)')
	define(`SILKYB', `eval(MAXY + 180 + 87 + 25)')

	define(`SILKLYT', `eval(SILKYT - 485)')

	# y-location of REFDES silk
	define(`REFDESY', `eval(SILKYT - 125)')

	# width of box drawn in silk for the lever
	define(`SILKLW', `150')

	ifelse(eval(eval(`$6') ==  2), 1, `
		# lever on right
		define(`SILKLXL', `eval(SILKXL)')
		define(`SILKLXR', `eval(SILKLXL + SILKLW)')'
		, `
		# lever on left
		define(`SILKLXR', `eval(SILKXR)')
		define(`SILKLXL', `eval(SILKLXR - SILKLW)')'
	)
	define(`PADXL', `eval(-1 * `$5' / 2)')
	define(`PADXR', `eval(`$5' / 2)')

	define(`CENTERX', `eval(MAXX / 2)')
Element(0x00 "$1" "`$2'" "$3" 0 REFDESY 0 200 0x00)
(
        # footprint runs in two vertical rows with pin 1 at upper left
	# X = 0 will be the center line of the part
	forloop(`i', 1, eval($4 / 2),
		`PIN(PADXL, eval((i-1) * PINSPACE),
			eval(PADSIZE), eval(DRILLSIZE), i)
	')

	# right column
	forloop(`i', 1, eval($4 / 2),
		`PIN(PADXR, eval(MAXY - (i-1) * PINSPACE),
			eval(PADSIZE), eval(DRILLSIZE), eval(i + $4/2))
	')

        # the mounting holes
        PIN(0, eval(MAXY + 180), eval(MTPADSIZE), eval(MTDRILLSIZE), eval(1 + $4))
        PIN(0, -335,  eval(MTPADSIZE), eval(MTDRILLSIZE), eval(2 + $4))

	ElementLine(SILKXL SILKYT SILKXL SILKYB 10)
	ElementLine(SILKXL SILKYB SILKXR SILKYB 10)
	ElementLine(SILKXR SILKYB SILKXR SILKYT 10)
	ElementLine(SILKXR SILKYT 50 SILKYT 10)
	ElementLine(SILKXL SILKYT -50 SILKYT 10)
	ElementArc(0 SILKYT 50 50 0 180 10)

	# and the lever silk
	ElementLine(SILKLXL SILKYT  SILKLXL SILKLYT 10)
	ElementLine(SILKLXL SILKLYT SILKLXR SILKLYT 10)
	ElementLine(SILKLXR SILKLYT SILKLXR SILKYT 10)

	Mark(PADXL 0)
)')



# EXTRACT_BEGIN

#
## High-Temp (250C) Universal ZIF DIP Burn-in and Test Socket
#

# XX-X55X-X X
#  | |  | | |- plating:      8 = plating
#  | |  | |--- termination:  1 = solder pin tail
#  | |  |----- handle options: 1 = left up, 2 = right up, 3 = left down, 4 = right down (std)
#  | |-------- row to row spacing:  3 = 300mil, 6 = 600mil
#  |---------- number of pins: 24, 28, 32, 36, 40, 42, 44, 48
#

#  24 pin

define(`PKG_ARIES_24_3551_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   24,  300, 1)');
define(`PKG_ARIES_24_3552_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   24,  300, 2)');
define(`PKG_ARIES_24_3553_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   24,  300, 1)');
define(`PKG_ARIES_24_3554_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   24,  300, 2)');

define(`PKG_ARIES_24_6551_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   24,  600, 1)');
define(`PKG_ARIES_24_6552_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   24,  600, 2)');
define(`PKG_ARIES_24_6553_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   24,  600, 1)');
define(`PKG_ARIES_24_6554_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   24,  600, 2)');


#  28 pin

define(`PKG_ARIES_28_3551_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   28,  300, 1)');
define(`PKG_ARIES_28_3552_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   28,  300, 2)');
define(`PKG_ARIES_28_3553_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   28,  300, 1)');
define(`PKG_ARIES_28_3554_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   28,  300, 2)');

define(`PKG_ARIES_28_6551_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   28,  600, 1)');
define(`PKG_ARIES_28_6552_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   28,  600, 2)');
define(`PKG_ARIES_28_6553_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   28,  600, 1)');
define(`PKG_ARIES_28_6554_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   28,  600, 2)');


#  32 pin

define(`PKG_ARIES_32_3551_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   32,  300, 1)');
define(`PKG_ARIES_32_3552_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   32,  300, 2)');
define(`PKG_ARIES_32_3553_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   32,  300, 1)');
define(`PKG_ARIES_32_3554_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   32,  300, 2)');

define(`PKG_ARIES_32_6551_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   32,  600, 1)');
define(`PKG_ARIES_32_6552_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   32,  600, 2)');
define(`PKG_ARIES_32_6553_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   32,  600, 1)');
define(`PKG_ARIES_32_6554_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   32,  600, 2)');


#  36 pin

define(`PKG_ARIES_36_3551_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   36,  300, 1)');
define(`PKG_ARIES_36_3552_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   36,  300, 2)');
define(`PKG_ARIES_36_3553_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   36,  300, 1)');
define(`PKG_ARIES_36_3554_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   36,  300, 2)');

define(`PKG_ARIES_36_6551_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   36,  600, 1)');
define(`PKG_ARIES_36_6552_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   36,  600, 2)');
define(`PKG_ARIES_36_6553_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   36,  600, 1)');
define(`PKG_ARIES_36_6554_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   36,  600, 2)');


#  40 pin

define(`PKG_ARIES_40_3551_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   40,  300, 1)');
define(`PKG_ARIES_40_3552_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   40,  300, 2)');
define(`PKG_ARIES_40_3553_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   40,  300, 1)');
define(`PKG_ARIES_40_3554_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   40,  300, 2)');

define(`PKG_ARIES_40_6551_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   40,  600, 1)');
define(`PKG_ARIES_40_6552_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   40,  600, 2)');
define(`PKG_ARIES_40_6553_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   40,  600, 1)');
define(`PKG_ARIES_40_6554_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   40,  600, 2)');


#  42 pin

define(`PKG_ARIES_42_3551_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   42,  300, 1)');
define(`PKG_ARIES_42_3552_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   42,  300, 2)');
define(`PKG_ARIES_42_3553_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   42,  300, 1)');
define(`PKG_ARIES_42_3554_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   42,  300, 2)');

define(`PKG_ARIES_42_6551_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   42,  600, 1)');
define(`PKG_ARIES_42_6552_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   42,  600, 2)');
define(`PKG_ARIES_42_6553_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   42,  600, 1)');
define(`PKG_ARIES_42_6554_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   42,  600, 2)');


#  44 pin

define(`PKG_ARIES_44_3551_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   44,  300, 1)');
define(`PKG_ARIES_44_3552_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   44,  300, 2)');
define(`PKG_ARIES_44_3553_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   44,  300, 1)');
define(`PKG_ARIES_44_3554_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   44,  300, 2)');

define(`PKG_ARIES_44_6551_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   44,  600, 1)');
define(`PKG_ARIES_44_6552_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   44,  600, 2)');
define(`PKG_ARIES_44_6553_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   44,  600, 1)');
define(`PKG_ARIES_44_6554_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   44,  600, 2)');


#  48 pin

define(`PKG_ARIES_48_3551_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   48,  300, 1)');
define(`PKG_ARIES_48_3552_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   48,  300, 2)');
define(`PKG_ARIES_48_3553_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   48,  300, 1)');
define(`PKG_ARIES_48_3554_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   48,  300, 2)');

define(`PKG_ARIES_48_6551_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   48,  600, 1)');
define(`PKG_ARIES_48_6552_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   48,  600, 2)');
define(`PKG_ARIES_48_6553_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   48,  600, 1)');
define(`PKG_ARIES_48_6554_1',   `PKG_ARIES_DIP(`$1', `$2', `$3',   48,  600, 2)');

# EXTRACT_END
