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
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
# 
#   Contact addresses for paper mail and Email:
#   Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
#   Thomas.Nau@rz.uni-ulm.de
#   Improvments by D.J. Barrow dj_barrow@ariasoft.ie
# 
#   RCS: $Id$
#
#  QFP packages
#
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: number of pins
# $5: additional border
# $6: pad pitch in millionths of inches
# $7: pad thickness
# $8: pad min offset ( edge nearest package )
# $9: pad max offset ( edge furthest away from package )
# $10: pin0 in middle
#
define(`PKG_GENERIC_QFP',
       `define(`QUARTER', `eval($4 /4)')
	define(`OFFSET', `eval((QUARTER +1) /2)')
	define(`BORDER',`$5')
	define(`PITCH',`$6')
	define(`WIDTH', `eval((((QUARTER-1)*PITCH)/1000)+2*BORDER)')
	define(`CENTER', `eval(WIDTH / 2)')
	define(`NUMPINS', `$4')
	define(`PADTHICKNESS',`$7')
	define(`PADMINOFFSET',`$8')
	define(`PADMAXOFFSET',`$9')
	define(`FARPIN',`eval(WIDTH-BORDER)')
	define(`PIN1INMIDDLE',`$10')
	define(`EDGECUT',`ifelse(1,eval(PIN1INMIDDLE),`0',`28')')
	define(`STARTPIN',`ifelse(1,eval(PIN1INMIDDLE),`eval(((QUARTER+1)/2)+1)',`1')')
	define(`CIRCLECENTRE',`ifelse(1,eval(PIN1INMIDDLE),`eval(WIDTH/2)',35)')
Element(0x00 "$1" "$2" "$3" 100 CENTER 0 100 0x00)
(

	# left row
define(`X', 0)
define(`QFPPIN',`STARTPIN')
define(`idx',0)
forloop(`i', 1, QUARTER,
	`define(`Y', `eval(BORDER+((PITCH*idx)/1000))')'
	`PAD(eval(X-PADMAXOFFSET), Y, eval(X+PADMINOFFSET), Y, PADTHICKNESS, QFPPIN)'
	`define(`QFPPIN', incr(QFPPIN))'
	`define(`idx',incr(idx))'
)

	# bottom row
define(`Y', WIDTH)
define(`idx',0)
forloop(`i', 1, QUARTER,
	`define(`X', `eval(BORDER+((PITCH*idx)/1000))')'
	`PAD(X, eval(Y+PADMAXOFFSET), X, eval(Y-PADMINOFFSET), PADTHICKNESS, QFPPIN)'
	`define(`QFPPIN', incr(QFPPIN))'
	`define(`idx',incr(idx))'
)

	# right row
define(`X', WIDTH)
define(`idx',0)
forloop(`i', 1, QUARTER,
	`define(`Y', `eval(FARPIN-((PITCH*idx)/1000))')'
	`PAD(eval(X+PADMAXOFFSET), Y, eval(X-PADMINOFFSET), Y, PADTHICKNESS, QFPPIN)'
	`define(`QFPPIN', incr(QFPPIN))'
	`define(`idx',incr(idx))'
)

	# top row
define(`idx',0)
define(`Y', 0)
forloop(`i', 1, QUARTER,
	`define(`X',`eval(FARPIN-((PITCH*idx)/1000))')'
	`PAD(X, eval(Y-PADMAXOFFSET), X, eval(Y+PADMINOFFSET), PADTHICKNESS, QFPPIN)'
	`define(`QFPPIN', incr(QFPPIN))'
	`ifelse(eval(QFPPIN > NUMPINS), 1, `define(`QFPPIN', 1)')'
	`define(`idx',incr(idx))'
)

	ElementLine(EDGECUT 0 WIDTH 0 10)
	ElementLine(WIDTH 0 WIDTH WIDTH rSILK)
	ElementLine(WIDTH WIDTH 0 WIDTH rSILK)
	ElementLine(0 WIDTH 0 EDGECUT 10)
	ElementLine(0 EDGECUT EDGECUT 0 10)
	ElementArc(CIRCLECENTRE 35 10 10 0 360 10)

	Mark(0 0)
)')

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
define(`PKG_QFP',`PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`42',`31000',`20',`5',`65',`0')')
# Same as QFP except more compact typical example is a XCS40 PQ 208C
define(`PKG_COMPACTQFP',`PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`39',`19685',`12',`-20',`80',`0')')
define(`PKG_JEDECQFP2',`PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`42',`50000',`35',`-5',`65',`1')')

# -------------------------------------------------------------------
