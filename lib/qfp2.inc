#
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
#  QFP packages
#

# -------------------------------------------------------------------
# ThanX to Johan Andersson (johan@homemail.com), modified by Thomas Nau
# the definition of a plcc package for base code to make qfp package.
# modified for correct pad numbering by Holm Tiffe
#
# Code from plcc.inc modified by Thomas Olson to make this qfp.inc definition.
# Although in retrospec quad flat packs are more diverse than this algorithm will do.
# Many qfp are the same physical size but have more thus narrower pads. 
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: number of pins
# $5: additional border (will be ignored)
#
define(`PKG_QFP2',
	`define(`QUARTER', `eval($4 /4)')
	define(`OFFSET', `eval((QUARTER +1) /2)')
	define(`WIDTH', `eval((QUARTER-1) *25 +2*50)')
	define(`CENTER', `eval(WIDTH / 2)')
	define(`NUMPINS', `$4')
Element(0x00 "$1" "`$2'" "$3" 100 CENTER 0 100 0x00)
(

	# left row
define(`X', 0)
define(`Y', 50)
#define(`count', `eval(OFFSET+1)')
define(`count', 1)
forloop(`i', 1, QUARTER,
	`PAD(eval(X-65), Y, eval(X+5), Y, 15, count)' `define(`count', incr(count))'
	`define(`Y', eval(Y+25))'
	)

	# bottom row
define(`X', 50)
define(`Y', WIDTH)
forloop(`i', 1, QUARTER,
	`PAD(X, eval(Y+65), X, eval(Y-5), 15, count)' `define(`count', incr(count))'
	`define(`X', eval(X+25))'
)

	# right row
define(`X', WIDTH)
define(`Y', eval(WIDTH-50))
forloop(`i', 1, QUARTER,
	`PAD(eval(X+65), Y, eval(X-5), Y, 15, count)' `define(`count', incr(count))'
	`define(`Y', eval(Y-25))'
)

	# top row
define(`X', eval(WIDTH-50))
define(`Y', 0)
forloop(`i', 1, QUARTER,
	`PAD(X, eval(Y-65), X, eval(Y+5), 15, count)' `define(`count', incr(count))'
	`ifelse(eval(count > NUMPINS), 1, `define(`count', 1)')'
	`define(`X', eval(X-25))'
)

	ElementLine(28 0 WIDTH 0 10)
	ElementLine(WIDTH 0 WIDTH WIDTH 10)
	ElementLine(WIDTH WIDTH 0 WIDTH 10)
	ElementLine(0 WIDTH 0 28 10)
	ElementLine(0 28 28 0 10)

	ElementArc(80 80 20 20 0 360 10)

	Mark(0 0)
)')

# -------------------------------------------------------------------
