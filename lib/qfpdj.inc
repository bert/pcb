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
#   Further improved by W. Kazubski <wk@ire.pw.edu.pl>
# 
#   RCS: $Id$
#
#  QFP packages
#
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: number of pins
# $5: body size in mm (0 if rectangular 20x14 mm)
# $6: pad pitch in 1/100 mmm
# $7: pad thickness in 1/100 mmm
# $8: pad min offset ( edge nearest package )
# $9: pad max offset ( edge furthest away from package )
# $10: pin 1 in middle
#
define(`PKG_GENERIC_QFP',
       `define(`QUARTER',  `ifelse(0,`$5',`eval($4 * 3 / 10)',`eval($4 / 4)')')
        define(`QUARTER2', `eval(`$4' / 2 - QUARTER)')
	define(`OFFSET', `eval((QUARTER +1) /2)')
	define(`WIDTH', `ifelse(0,`$5',`787',`eval(`$5'*10000/254)')')
	define(`WIDTH2',`ifelse(0,`$5',`551',`eval(`$5'*10000/254)')')
	define(`PITCH',`eval(`$6'*100000/254)')
	define(`BORDER',  `eval((WIDTH-(QUARTER-1)*PITCH/1000)/2)')
	define(`BORDER2', `eval((WIDTH2-(QUARTER2-1)*PITCH/1000)/2)')
	define(`CENTER', `eval(WIDTH2 / 2)')
	define(`NUMPINS', `$4')
	define(`PADTHICKNESS',`eval(`$7'*100/254)')
	define(`PADMINOFFSET',`eval(`$8'+PADTHICKNESS/2)')
	define(`PADMAXOFFSET',`eval(`$9'-PADTHICKNESS/2)')
	define(`FARPIN', `eval(WIDTH-BORDER)')
	define(`FARPIN2',`eval(WIDTH2-BORDER2)')
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
	`PAD(eval(X-PADMAXOFFSET), Y, eval(X-PADMINOFFSET), Y, PADTHICKNESS, QFPPIN)'
	`define(`QFPPIN', incr(QFPPIN))'
	`define(`idx',incr(idx))'
)

	# bottom row
define(`Y', WIDTH)
define(`idx',0)
forloop(`i', 1, QUARTER2,
	`define(`X', `eval(BORDER2+((PITCH*idx)/1000))')'
	`PAD(X, eval(Y+PADMAXOFFSET), X, eval(Y+PADMINOFFSET), PADTHICKNESS, QFPPIN)'
	`define(`QFPPIN', incr(QFPPIN))'
	`define(`idx',incr(idx))'
)

	# right row
define(`X', WIDTH2)
define(`idx',0)
forloop(`i', 1, QUARTER,
	`define(`Y', `eval(FARPIN-((PITCH*idx)/1000))')'
	`PAD(eval(X+PADMAXOFFSET), Y, eval(X+PADMINOFFSET), Y, PADTHICKNESS, QFPPIN)'
	`define(`QFPPIN', incr(QFPPIN))'
	`define(`idx',incr(idx))'
)

	# top row
define(`idx',0)
define(`Y', 0)
forloop(`i', 1, QUARTER2,
	`define(`X',`eval(FARPIN2-((PITCH*idx)/1000))')'
	`PAD(X, eval(Y-PADMAXOFFSET), X, eval(Y-PADMINOFFSET), PADTHICKNESS, QFPPIN)'
	`define(`QFPPIN', incr(QFPPIN))'
	`ifelse(eval(QFPPIN > NUMPINS), 1, `define(`QFPPIN', 1)')'
	`define(`idx',incr(idx))'
)

	ElementLine(EDGECUT 0 WIDTH2 0 10)
	ElementLine(WIDTH2 0 WIDTH2 WIDTH 10)
	ElementLine(WIDTH2 WIDTH 0 WIDTH 10)
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
# $5: body size in mm
# $6: pad pitch in 1/100 mmm
# $7: pad thickness
#

# pins 1.9-2.1mm long
define(`PKG_QFP_100',`PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5',`100',`50',`20',`100',`0')')
define(`PKG_QFP_80L',`PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5', `80',`50',`20',`100',`0')')
define(`PKG_QFP_65L',`PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5', `65',`40',`20',`100',`0')')
define(`PKG_QFP_50L',`PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5', `50',`28',`20',`100',`0')')

# pins 1.6mm long
define(`PKG_QFP_80', `PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5', `80',`40',`20', `80',`0')')
define(`PKG_QFP_65', `PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5', `65',`40',`20', `80',`0')')
define(`PKG_QFP_50', `PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5', `50',`28',`20', `80',`0')')

# pins 1mm long
define(`PKG_LQFP_80',`PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5', `80',`40', `5', `50',`0')')
define(`PKG_LQFP_65',`PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5', `65',`40', `5', `50',`0')')
define(`PKG_LQFP_50',`PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5', `50',`28', `5', `50',`0')')
define(`PKG_LQFP_40',`PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5', `40',`25', `5', `50',`0')')

# -------------------------------------------------------------------

