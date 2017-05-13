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
#   Improvments by D.J. Barrow dj_barrow@ariasoft.ie
#   Further improved by W. Kazubski <wk@ire.pw.edu.pl>
# 
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
# $11: Exposed paddle size, 0 for no exposed paddle (1/100 mm)
#
define(`PKG_GENERIC_QFP',
       `define(`QUARTER',  `ifelse(0,`$5',`eval($4 * 3 / 10)',`eval($4 / 4)')')
        define(`QUARTER2', `eval(`$4' / 2 - QUARTER)')
	define(`OFFSET', `eval((QUARTER +1) /2)')
	define(`WIDTH', `ifelse(0,`$5',`78740',`eval(`$5'*1000000/254)')')
	define(`WIDTH2',`ifelse(0,`$5',`55118',`eval(`$5'*1000000/254)')')
	define(`PITCH',`eval(`$6'*100000/254)')
	define(`PINREACH',  `eval(((QUARTER-1)*PITCH/10)/2)')
	define(`PINREACH2', `eval(((QUARTER2-1)*PITCH/10)/2)')
	define(`HALFWIDTH', `eval(WIDTH / 2)')
	define(`HALFWIDTH2', `eval(WIDTH2 / 2)')
	define(`NUMPINS', `$4')
	define(`PADTHICKNESS',`eval(`$7'*10000/254)')
	define(`PADMINOFFSET',`eval(`$8'*100+PADTHICKNESS/2)')
	define(`PADMAXOFFSET',`eval(`$9'*100-PADTHICKNESS/2)')
	define(`PIN1INMIDDLE',`$10')
	define(`CUTSIZE',`3300')
	define(`EDGECUT',`ifelse(1,eval(PIN1INMIDDLE),`HALFWIDTH',`eval(HALFWIDTH-CUTSIZE)')')
	define(`EDGECUT2',`ifelse(1,eval(PIN1INMIDDLE),`HALFWIDTH2',`eval(HALFWIDTH2-CUTSIZE)')')
	define(`STARTPIN',`ifelse(1,eval(PIN1INMIDDLE),`eval(((QUARTER+1)/2)+1)',`1')')
	define(`CIRCLECENTRE',`eval(-EDGECUT)')
	define(`CIRCLECENTRE2',`ifelse(1,eval(PIN1INMIDDLE),`0',`eval(-EDGECUT2)')')
	define(`PADMASK', `eval(2*150 + PADTHICKNESS)')
	define(`PADCLEAR', `3000')
	define(`EPCLEAR', `500')
	define(`EPSIZE', `eval(`$11'*10000/254)')
	define(`EPMASK', `eval(2*200 + EPSIZE)')
	define(`SILKW', `800')
	define(`SLX', `eval(-HALFWIDTH2 + SILKW/2)')
	define(`SRX', `eval( HALFWIDTH2 - SILKW/2)')
	define(`SUY', `eval(-HALFWIDTH  + SILKW/2)')
	define(`SLY', `eval( HALFWIDTH  - SILKW/2)')


Element[0x00000000 "$1" "`$2'" "$3" 0 0 -2000 -6000 0 100 0x00000000]
(

	# left row
define(`QFPPIN',`STARTPIN')
define(`idx',0)
define(`PADIN',`eval(-HALFWIDTH2-PADMINOFFSET)')
define(`PADOUT',`eval(-HALFWIDTH2-PADMAXOFFSET)')
forloop(`i', 1, QUARTER,
	`define(`Y', `eval(-PINREACH+((PITCH*idx)/10))')'
	`Pad[PADOUT  Y  PADIN  Y  PADTHICKNESS  PADCLEAR PADMASK "QFPPIN" "QFPPIN"  0x00000100]'
	`define(`QFPPIN', incr(QFPPIN))'
	`define(`idx',incr(idx))'
)

	# bottom row
define(`idx',0)
define(`PADIN',`eval(HALFWIDTH+PADMINOFFSET)')
define(`PADOUT',`eval(HALFWIDTH+PADMAXOFFSET)')
forloop(`i', 1, QUARTER2,
	`define(`X', `eval(-PINREACH2+((PITCH*idx)/10))')'
	`Pad[X PADOUT  X  PADIN  PADTHICKNESS  PADCLEAR PADMASK "QFPPIN" "QFPPIN"  0x00000900]'
	`define(`QFPPIN', incr(QFPPIN))'
	`define(`idx',incr(idx))'
)

	# right row
define(`idx',0)
define(`PADIN',`eval(HALFWIDTH2+PADMINOFFSET)')
define(`PADOUT',`eval(HALFWIDTH2+PADMAXOFFSET)')
forloop(`i', 1, QUARTER,
	`define(`Y', `eval(PINREACH-((PITCH*idx)/10))')'
	`Pad[PADOUT  Y  PADIN  Y  PADTHICKNESS  PADCLEAR PADMASK "QFPPIN" "QFPPIN"  0x00000100]'
	`define(`QFPPIN', incr(QFPPIN))'
	`define(`idx',incr(idx))'
)

	# top row
define(`idx',0)
define(`PADIN',`eval(-HALFWIDTH-PADMINOFFSET)')
define(`PADOUT',`eval(-HALFWIDTH-PADMAXOFFSET)')
forloop(`i', 1, QUARTER2,
	`define(`X',`eval(PINREACH2-((PITCH*idx)/10))')'
	`Pad[X PADOUT  X  PADIN  PADTHICKNESS  PADCLEAR PADMASK "QFPPIN" "QFPPIN"  0x00000900]'
	`define(`QFPPIN', incr(QFPPIN))'
	`ifelse(eval(QFPPIN > NUMPINS), 1, `define(`QFPPIN', 1)')'
	`define(`idx',incr(idx))'
)

	# exposed paddle (if this is an exposed paddle part) 
ifelse(1, eval(EPSIZE>0), 
	`Pad[0 0 0 0 EPSIZE EPCLEAR EPMASK "QFPPIN" "QFPPIN" 0x00000100]'
	`define(`QFPPIN', incr(QFPPIN))'
)

	# top
	ElementLine[-EDGECUT2 SUY SRX SUY SILKW]
	# right
	ElementLine[SRX SUY SRX SLY SILKW]
	# bottom
	ElementLine[SRX SLY SLX SLY SILKW]
	# left
	ElementLine[SLX SLY SLX -EDGECUT SILKW]
	# angled corner
	ElementLine[SLX -EDGECUT -EDGECUT2 SUY SILKW]

	ElementArc[CIRCLECENTRE2 CIRCLECENTRE 1000 1000 0 360 SILKW]

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
# $8: pad min offset ( edge nearest package )
# $9: pad max offset ( edge furthest away from package )
# $10: pin 1 in middle
#

# pins 1.9-2.1mm long
define(`PKG_QFP_100',`PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5',`100',`50',`20',`100',`0',`$6')')
define(`PKG_QFP_80L',`PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5', `80',`50',`20',`100',`0',`$6')')
define(`PKG_QFP_65L',`PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5', `65',`40',`20',`100',`0',`$6')')
define(`PKG_QFP_50L',`PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5', `50',`28',`20',`100',`0',`$6')')

# pins 1.6mm long
define(`PKG_QFP_80', `PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5', `80',`40',`20', `80',`0',`$6')')
define(`PKG_QFP_65', `PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5', `65',`40',`20', `80',`0',`$6')')
define(`PKG_QFP_50', `PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5', `50',`28',`20', `80',`0',`$6')')
define(`PKG_QFP_40', `PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5', `40',`25',`20', `80',`0',`$6')')

# pins 1mm long
define(`PKG_LQFP_80',`PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5', `80',`40', `5', `50',`0',`$6')')
define(`PKG_LQFP_65',`PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5', `65',`40', `5', `50',`0',`$6')')
define(`PKG_LQFP_50',`PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5', `50',`28', `5', `50',`0',`$6')')
define(`PKG_LQFP_40',`PKG_GENERIC_QFP(`$1',`$2',`$3',`$4',`$5', `40',`25', `5', `50',`0',`$6')')

# -------------------------------------------------------------------

