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
define(`PKG_OLD_QFP',
	`define(`QUARTER', `eval($4 /4)')
	define(`OFFSET', `eval((QUARTER +1) /2)')
	define(`WIDTH', `eval((QUARTER-1) *31 +2*42)')
	define(`CENTER', `eval(WIDTH / 2)')
	define(`NUMPINS', `$4')
Element(0x00 "$1" "`$2'" "$3" 100 CENTER 0 100 0x00)
(

	# left row
define(`X', 0)
define(`Y', 42)
#define(`count', `eval(OFFSET+1)')
define(`count', 1)
forloop(`i', 1, QUARTER,
	`PAD(eval(X-65), Y, eval(X+5), Y, 20, count)' `define(`count', incr(count))'
	`define(`Y', eval(Y+31))'
	)

	# bottom row
define(`X', 42)
define(`Y', WIDTH)
forloop(`i', 1, QUARTER,
	`PAD(X, eval(Y+65), X, eval(Y-5), 20, count)' `define(`count', incr(count))'
	`define(`X', eval(X+31))'
)

	# right row
define(`X', WIDTH)
define(`Y', eval(WIDTH-42))
forloop(`i', 1, QUARTER,
	`PAD(eval(X+65), Y, eval(X-5), Y, 20, count)' `define(`count', incr(count))'
	`define(`Y', eval(Y-31))'
)

	# top row
define(`X', eval(WIDTH-42))
define(`Y', 0)
forloop(`i', 1, QUARTER,
	`PAD(X, eval(Y-65), X, eval(Y+5), 20, count)' `define(`count', incr(count))'
	`ifelse(eval(count > NUMPINS), 1, `define(`count', 1)')'
	`define(`X', eval(X-31))'
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


# Yet another QFP Package family -- untested (but looks pretty)
# Concepts stolen from PLCC and older QFP macros above.
# December 1999, Larry Doolittle <LRDoolittle@lbl.gov>
# This is intended to be fully general, and replace all other QFP code.
# It can handle square or rectangular packages of variable pitch.
# It sings, it dances, it might even be made to handle PLCCs and
# the Motorola package outlines with those cool-looking tabs on the
# plastic.
#
# PKG_GEN_QFP is the general case, PKG_LQFP and PKG_QFP call it
# with different setup.
#   PITCH      pad center-to-center distance ***IN UNITS OF 0.1 MICROMETER***
#             (weird, I know, but this sets 1 mil=254 units exactly, so
#              pitches defined in either Imperial or Metric may be used)
#   PAD_WIDTH  width of pad (mils)
#   XPADS      Number of pads in X direction
#   YPADS      Number of pads in Y direction
#             (total number of pads is 2*(XPADS+YPADS) )
#   X_LENGTH   X length in mils from end-to-end of the pads
#   Y_LENGTH   Y length in mils from end-to-end of the pads
#   ISTART    Where along the left row (YPADS long) to find pin 1.
#             (generally ISTART=1 for pin 1 in the corner, but ISTART
#              is 11 for some 84-pin square packages and 17 for some
#              132-pin square packages)
#
# Feb 3, 2000 LRD: allow XPADS == 0 to represent SOIC packages

define(`PKG_GEN_QFP',
	`
	define(`PX', `eval((PITCH*(XPADS-1)+127)/254)')
	define(`PY', `eval((PITCH*(YPADS-1)+127)/254)')
	define(`PHW', `eval(PAD_WIDTH/2)')
Element(0x00 "$1" "`$2'" "$3" 100 0 0 100 0x00)
(
	define(`count', 1)

	# left row, going down
	define(`X_OUTER', PHW)
	define(`X_INNER', eval(PAD_LENGTH-PHW))
	define(`Y0', `eval((Y_LENGTH-PY)/2)')
	forloop(`i', ISTART, YPADS,
		`define(`Y', eval(Y0+(PITCH*(i-1)+127)/254) )'
		`PAD(X_OUTER, Y, X_INNER, Y, PAD_WIDTH, count)'
		`define(`count',incr(count))'
	)

	# bottom row, going right
	ifelse(XPADS,0,,`
	define(`Y_OUTER', eval(Y_LENGTH-PHW))
	define(`Y_INNER', eval(Y_LENGTH+PHW-PAD_LENGTH))
	define(`X0', `eval((X_LENGTH-PX)/2)')
	forloop(`i', 1, XPADS,
		`define(`X', eval(X0+(PITCH*(i-1)+127)/254) )'
		`PAD(X, Y_OUTER, X, Y_INNER, PAD_WIDTH, count)'
		`define(`count',incr(count))'
	)')

	# right row, going up
	define(`X_OUTER', eval(X_LENGTH-PHW))
	define(`X_INNER', eval(X_LENGTH+PHW-PAD_LENGTH))
	define(`Y0', `eval((Y_LENGTH+PY)/2)')
	forloop(`i', 1, YPADS,
		`define(`Y', eval(Y0-(PITCH*(i-1)+127)/254) )'
		`PAD(X_OUTER, Y, X_INNER, Y, PAD_WIDTH, count)'
		`define(`count',incr(count))'
	)

	# top row, going left
	ifelse(XPADS,0,,`
	define(`Y_OUTER', PHW)
	define(`Y_INNER', eval(PAD_LENGTH+PHW-PAD_WIDTH))
	define(`X0', `eval((X_LENGTH+PX)/2)')
	forloop(`i', 1, XPADS,
		`define(`X', eval(X0-(PITCH*(i-1)+127)/254) )'
		`PAD(X, Y_OUTER, X, Y_INNER, PAD_WIDTH, count)'
		`define(`count',incr(count))'
	)')

	# left row, going down again, maybe
	define(`X_OUTER', PHW)
	define(`X_INNER', eval(PAD_LENGTH-PHW))
	define(`Y0', `eval((Y_LENGTH-PY)/2)')
	ifelse(ISTART,1,,`forloop(`i', 1, eval(ISTART-1),
		`define(`Y', eval(Y0+(PITCH*(i-1)+127)/254) )'
		`PAD(X_OUTER, Y, X_INNER, Y, PAD_WIDTH, count)'
		`define(`count',incr(count))'
	)')

	define(`NOSMUDGE', 10)
	define(`SSOX', eval(NOSMUDGE+PAD_LENGTH))
	define(`SSOY', ifelse(XPADS,0,0,eval(NOSMUDGE+PAD_LENGTH)))
	define(`PPX', eval(X_LENGTH-SSOX))
	define(`PPY', eval(Y_LENGTH-SSOY))
	ElementLine(SSOX SSOY PPX  SSOY 8)
	ElementLine(PPX  SSOY PPX  PPY  8)
	ElementLine(PPX  PPY  SSOX PPY  8)
	ElementLine(SSOX PPY  SSOX SSOY 8)

	# Pin 1 Indicator
	define(`Y1', ifelse(ISTART,1,`eval(SSOY+40)',
	                             `eval(Y0+(PITCH*(ISTART-1)+127)/254)'))
	ElementArc(eval(SSOX+40) Y1 20 20 0 360 10)

	# Moderately useful place for the Mark.  This way,
	# if the pins can line up with the grid, they do.
	Mark(eval((X_LENGTH-PX)/2) eval((Y_LENGTH-PY)/2))
)')

# PKG_QFP: square quad flat pack, 0.8 mm pitch, based on PKG_GEN_QFP above
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: total number of pins (package is assumed square)
# This configuration should correspond to the old PKG_QFP, renamed
# PKG_OLD_QFP above for comparison.  The two implementations have,
# among other things, different coordinate zeros.

define(`PKG_QFP',
	`define(`PITCH', 8000)
	define(`PAD_WIDTH', 20)
	define(`PAD_LENGTH', 90)
	define(`XPADS', `eval($4 /4)')
	define(`YPADS', `eval($4 /4)')
	define(`X_LENGTH', `eval((PITCH*(XPADS-1)+127)/254+232)')
	define(`Y_LENGTH', X_LENGTH)
	define(`ISTART', 1)
	PKG_GEN_QFP($1, $2, $3)'
)

# PKG_208_LQFP: leaded quad flat pack, 0.5 mm pitch, based on PKG_GEN_QFP above
# $1: canonical name
# $2: name on PCB
# $3: value
# This configuration based on a mechanical drawing of a
# Cirrus Logic EP7211 in a 208-Pin LQFP

define(`PKG_208_LQFP',
	`define(`PITCH', 5000)
	define(`PAD_LENGTH', 60)
	define(`PAD_WIDTH', 10)
	define(`XPADS', 52)
	define(`YPADS', 52)
	define(`X_LENGTH', 1220)
	define(`Y_LENGTH', 1220)
	define(`ISTART', 1)
	PKG_GEN_QFP($1, $2, $3)'
)

# Interesting hack, this.  pcb -> sh -> m4 -> tcl
# Refer to qfp-ui, a tcl program that should accompany this file.
# Search path for this puppy comes from QueryLibrary.sh, which
# I (LRD) patched to merge M4PATH into PATH
define(`PKG_MENU_QFP',
	`esyscmd(qfp-ui "$1" "`$2'" "$3")'
)

