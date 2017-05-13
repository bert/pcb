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
# PLCC packages
#

# -------------------------------------------------------------------
# ThanX to Johan Andersson (johan@homemail.com), modified by Thomas Nau
# the definition of a plcc package
# modified for correct pad numbering by Holm Tiffe
#
# modified 11-Dec-1999 Larry Doolittle <LRDoolittle@lbl.gov>
# to shorten the pads by 10 mils; PUSHOUT=5 PUSHIN=40
#
# modified 22-Dec-1999 Larry Doolittle <LRDoolittle@lbl.gov>
# put back the 10 mils on the outside; PUSHOUT=5, PUSHIN=50
#
# One last tweak 12-Jan-2000 LRD, now that the shop complained I
# didn't leave them any room to solder (looks like the 22-Dec
# change was backwards);  PUSHOUT=20, PUSHIN=40
#
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: number of pins
# $5: additional border (will be ignored)
#
define(`PKG_PLCC',
	`define(`NUMPINS', `$4')
	ifelse(eval(NUMPINS == 32),1,`define(`QUARTER',7)',`define(`QUARTER', `eval($4 /4)')')
	define(`OFFSET', `eval((QUARTER +1) /2)')
	define(`WIDTH', `eval((QUARTER-1) *50 +2*75)')
	ifelse(eval(NUMPINS == 32),1,`define(`SIDE',`eval((NUMPINS-2*QUARTER)/2)')', `define(`SIDE',QUARTER)')
	define(`HEIGHT', `eval((SIDE-1)* 50 + 2*75)')
	define(`CENTER', `eval(WIDTH / 2)')
	define(`PUSHOUT', 20)    # was 15
	define(`PUSHIN', 40)     # was 50
Element(0x00 "$1" "`$2'" "$3" 100 CENTER 0 100 0x00)
(
	# top left half
define(`X', CENTER)
define(`Y', 0)
define(`count', 1)
forloop(`i', 1, OFFSET,
	`PAD(X, eval(Y-PUSHOUT), X, eval(Y+PUSHIN), 20, count)' `define(`count', incr(count))'
	`define(`X', eval(X-50))'
	)


	# left row
define(`X', 0)
define(`Y', 75)
define(`count', `eval(OFFSET+1)')
forloop(`i', 1, SIDE,
	`PAD(eval(X-PUSHOUT), Y, eval(X+PUSHIN), Y, 20, count)' `define(`count', incr(count))'
	`define(`Y', eval(Y+50))'
	)

	# bottom row
define(`X', 75)
define(`Y', HEIGHT)
forloop(`i', 1, QUARTER,
	`PAD(X, eval(Y+PUSHOUT), X, eval(Y-PUSHIN), 20, count)' `define(`count', incr(count))'
	`define(`X', eval(X+50))'
)

	# right row
define(`X', WIDTH)
define(`Y', eval(HEIGHT-75))
forloop(`i', 1, SIDE,
	`PAD(eval(X+PUSHOUT), Y, eval(X-PUSHIN), Y, 20, count)' `define(`count', incr(count))'
	`define(`Y', eval(Y-50))'
)

	# top right row
define(`X', eval(WIDTH-75))
define(`Y', 0)
forloop(`i', 1, eval(OFFSET-1),
	`PAD(X, eval(Y-PUSHOUT), X, eval(Y+PUSHIN), 20, count)' `define(`count', incr(count))'
	`ifelse(eval(count > NUMPINS), 1, `define(`count', 1)')'
	`define(`X', eval(X-50))'
)

#	ElementLine(50 0 WIDTH 0 20)
#	ElementLine(WIDTH 0 WIDTH WIDTH 20)
#	ElementLine(WIDTH WIDTH 0 WIDTH 20)
#	ElementLine(0 WIDTH 0 50 20)
#	ElementLine(0 50 50 0 20)

# Modified by Thomas Olson to eliminate silkscreen blobbing over pads.
# Approach one: eliminate ElementLine transgression over pads. leave corners
# only.

define(`OLWIDTH', eval(WIDTH-50))
define(`OLHEIGHT', eval(HEIGHT-50))
	
	ElementLine(OLWIDTH 0 WIDTH 0 10)
	ElementLine(WIDTH 0 WIDTH 50 10)
	ElementLine(WIDTH OLHEIGHT WIDTH HEIGHT 10)
	ElementLine(WIDTH HEIGHT OLWIDTH HEIGHT 10)
	ElementLine(50 HEIGHT 0 HEIGHT 10)
	ElementLine(0 HEIGHT 0 OLHEIGHT 10)
	ElementLine(0 50 50 0 10)

# Approach two: move outline to edge of pads.
# The outline should be 15 off. But since the pad algorithm
# is not making the square pads correctly I give it a total of 30
# to clear the pads.

# Try 40 mils, and parameterize it.  1/12/00 LRD
define(`NOSMUDGE', 40)
define(`OLWIDTH', eval(WIDTH+NOSMUDGE))
define(`OLHEIGHT', eval(HEIGHT+NOSMUDGE))

	ElementLine(50 -NOSMUDGE OLWIDTH -NOSMUDGE 10)
	ElementLine(OLWIDTH -NOSMUDGE OLWIDTH OLHEIGHT 10)
	ElementLine(OLWIDTH OLHEIGHT -NOSMUDGE OLHEIGHT 10)
	ElementLine(-NOSMUDGE OLHEIGHT -NOSMUDGE 50 10)
	ElementLine(-NOSMUDGE 50 50 -NOSMUDGE 10)

	ElementArc(CENTER 100 20 20 0 360 10)

	Mark(0 0)
)')

# -------------------------------------------------------------------
# the definition of a plcc package with through-hole socket
#
# for example, Assmann A-CCS##-{Z,G} series
# or mil-max 940-XX-XXX-24-000000 series.
#
# mil-max specifies 0.035" drill hole and 0.062" pad.
# Assman says 0.0315" hole.  Also the mil-max outside
# dimensions are ever so slightly larger than Assmann so
# use those.  See for example
# http://www.milmax.com/images/products/pdf/092.PDF
#
# modified for correct pin numbering by Holm Tiffe
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: number of pins
# $5: additional border
#
define(`PKG_PLCC_SOCKET',
	`define(`QUARTER', `eval($4 /4)')
	define(`OFFSET', eval((QUARTER +1) / 2))
	define(`BORDER', `$5')
	define(`WIDTH', `eval((OFFSET+1) *100 +2*BORDER)')
	define(`CENTER', `eval(WIDTH / 2)')
	define(`NUMPINS', `$4')
	define(`PADSIZE', `62')
	define(`DRILL', `35')
Element(0x00 "$1" "`$2'" "$3" eval(BORDER+200) CENTER 0 100 0x00)

# PLCC - 44 is a  special case, pad 1 in inner row

ifelse(eval(NUMPINS == 44 || NUMPINS == 28 || NUMPINS == 32), 1, `(

define(`X',eval(CENTER+50))
define(`Y',eval(BORDER+100))
define(`count',1)

ifelse(eval(NUMPINS == 44),1,`define(`ltr',3)',`define(`ltr',2)')
ifelse(eval(NUMPINS == 32),1,`define(`QUARTER',9)')

# top left row
forloop(`i', 1, ltr,
	`PIN(X, Y, PADSIZE, DRILL, count)' `define(`count', incr(count))'
	`PIN(eval(X-100), eval(Y-100), PADSIZE, DRILL, count)' `define(`count', incr(count))'
	`define(`X', eval(X-100))'
)

#left row
define(`X',eval(X-100))
forloop(`i', 1, eval((QUARTER-1)/2),
	`PIN(X, Y, PADSIZE, DRILL, count)' `define(`count', incr(count))'
	`PIN(eval(X+100), Y, PADSIZE, DRILL, count)' `define(`count', incr(count))'
	`define(`Y', eval(Y+100))'
)
PIN(X, Y, PADSIZE, DRILL, count)
define(`count', incr(count))

# bottom row
define(`X',eval(X+100))
define(`Y',eval(Y+100))
forloop(`i', 1, eval(ltr*2-1),
	`PIN(X, Y, PADSIZE, DRILL, count)' `define(`count', incr(count))'
	`PIN(X, eval(Y-100), PADSIZE, DRILL, count)' `define(`count', incr(count))'
	`define(`X', eval(X+100))'
)
PIN(X, Y, PADSIZE, DRILL, count)
define(`count', incr(count))

# right row
define(`X',eval(X+100))
define(`Y',eval(Y-100))
forloop(`i', 1, eval((QUARTER-1)/2),
	`PIN(X, Y, PADSIZE, DRILL, count)' `define(`count', incr(count))'
	`PIN(eval(X-100), Y, PADSIZE, DRILL, count)' `define(`count', incr(count))'
	`define(`Y', eval(Y-100))'
)
PIN(X, Y, PADSIZE, DRILL, count)
define(`count', incr(count))

# top rigth row
define(`X',eval(X-100))
define(`Y',eval(Y-100))
forloop(`i', 1, eval(ltr-1),
	`PIN(X, Y, PADSIZE, DRILL, count)' `define(`count', incr(count))'
	`PIN(X, eval(Y+100), PADSIZE, DRILL, count)' `define(`count', incr(count))'
	`define(`X', eval(X-100))'
)
PIN(X, Y, PADSIZE, DRILL, count)

ifelse(eval(NUMPINS == 32),1,`define(`HEIGHT',eval(WIDTH+100))',`define(`HEIGHT',WIDTH)')
	ElementLine(0 0 WIDTH 0 20)
	ElementLine(WIDTH 0 WIDTH HEIGHT 20)
	ElementLine(WIDTH HEIGHT 0 HEIGHT 20)
	ElementLine(0 HEIGHT 0 0 20)
	ElementLine(0 100 100 0 10)
	ElementLine(eval(CENTER-50) 0 CENTER 50 10)
	ElementLine(CENTER 50 eval(CENTER+50) 0 10)

	Mark(CENTER BORDER)


#------------------------------------------------------------------------
# all other sockets, currently 20,52,68 and 84 pins

)', `(

# the default case, Pad 1 is on outer top row, in the middle


#top left row

define(`X',CENTER)
define(`Y',BORDER)
define(`count',1)
forloop(`i', 1, eval((QUARTER-1)/4),
	`PIN(X, Y, PADSIZE, DRILL, count)' `define(`count', incr(count))'
	`PIN(X, eval(Y+100), PADSIZE, DRILL, count)' `define(`count', incr(count))'
	`define(`X',eval(X-100))'
)
PIN(X, Y, PADSIZE, DRILL, count)
define(`count', incr(count))

# left row
define(`Y',eval(BORDER+100))
define(`X',BORDER)
forloop(`i', 1, eval((QUARTER-1)/2),
	`PIN(X, Y, PADSIZE, DRILL, count)' `define(`count', incr(count))'
	`PIN(eval(X+100), Y, PADSIZE, DRILL, count)' `define(`count', incr(count))'
	`define(`Y',eval(Y+100))'
)
PIN(X, Y, PADSIZE, DRILL, count)
define(`count', incr(count))

# bottom row
define(`X',eval(BORDER+100))
define(`Y',eval(WIDTH-BORDER))
forloop(`i', 1, eval((QUARTER-1)/2),
	`PIN(X, Y, PADSIZE, DRILL, count)' `define(`count', incr(count))'
	`PIN(X, eval(Y-100), PADSIZE, DRILL, count)' `define(`count', incr(count))'
	`define(`X',eval(X+100))'
)
PIN(X, Y, PADSIZE, DRILL, count)
define(`count', incr(count))

# right row
define(`X',eval(WIDTH-BORDER))
define(`Y',eval(WIDTH-BORDER-100))
forloop(`i', 1, eval((QUARTER-1)/2),
	`PIN(X, Y, PADSIZE, DRILL, count)' `define(`count', incr(count))'
	`PIN(eval(X-100), Y, PADSIZE, DRILL, count)' `define(`count', incr(count))'
	`define(`Y',eval(Y-100))'
)
PIN(X, Y, PADSIZE, DRILL, count)
define(`count', incr(count))

#top right row

define(`X',eval(WIDTH-BORDER-100))
define(`Y',BORDER)
forloop(`i', 1, eval((QUARTER-1)/4),
	`PIN(X, Y, PADSIZE, DRILL, count)' `define(`count', incr(count))'
	`PIN(X, eval(Y+100), PADSIZE, DRILL, count)' `define(`count', incr(count))'
	`define(`X',eval(X-100))'
)

	ElementLine(0 0 WIDTH 0 20)
	ElementLine(WIDTH 0 WIDTH WIDTH 20)
	ElementLine(WIDTH WIDTH 0 WIDTH 20)
	ElementLine(0 WIDTH 0 0 20)
	ElementLine(0 100 100 0 10)
	ElementLine(eval(CENTER-50) 0 CENTER 50 10)
	ElementLine(CENTER 50 eval(CENTER+50) 0 10)

	Mark(CENTER BORDER)

)')')

