# -*- m4 -*-
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
# DIL packages

# -------------------------------------------------------------------
# the definition of a dual-inline package N and similar types
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: number of pins
# $5: package size (300, 600, 900 + 100 for socket space)
# $6: pin spacing
# $7: pad size
# $8: drill size
#
define(`PKG_DIL',
	`
# retain backwards compatibility to older versions of PKG_DIL 
# which did not have $6,$7,$8 args

        ifelse("`$6'","",
                `define(`PINSPACE', `100')'
        ,
                `define(`PINSPACE', eval(`$6'))'
        )      
        ifelse("`$7'","",
                `define(`PADSIZE', `60')'
        ,
                `define(`PADSIZE', `$7')'
        )      
        ifelse("`$8'","",
                `define(`DRILLSIZE', `28')'
        ,
                `define(`DRILLSIZE', `$8')'
        )      
	define(`MAXY', `eval(`$4' / 2 * PINSPACE)')
	define(`MAXX', `eval(`$5' + 100)')
	define(`CENTERX', `eval(MAXX / 2)')
	define(`MARKY', `eval(PINSPACE / 2)')
Element(0x00 "$1" "`$2'" "$3" eval(CENTERX + 20) 100 3 100 0x00)
(
	forloop(`i', 1, eval($4 / 2),
		`PIN(50, eval((2*i-1) * PINSPACE/2), 
			eval(PADSIZE), eval(DRILLSIZE), i)
	')
	forloop(`i', 1, eval($4 / 2),
		`PIN(eval(MAXX -50), eval(MAXY - (2*i-1) * PINSPACE/2), 
			eval(PADSIZE), eval(DRILLSIZE), eval(i + $4/2))
	')
	ElementLine(0 0 0 MAXY 10)
	ElementLine(0 MAXY MAXX MAXY 10)
	ElementLine(MAXX MAXY MAXX 0 10)
	ElementLine(0 0 eval(CENTERX - 50) 0 10)
	ElementLine(eval(CENTERX + 50) 0 MAXX 0 10)
	ElementArc(CENTERX 0 50 50 0 180 10)
	Mark(50 MARKY)
)')
define(`PKG_J',  `PKG_DIL(`$1', `$2', `$3', `$4', `$5', 100, 60, 28)')
define(`PKG_JD', `PKG_DIL(`$1', `$2', `$3', `$4', `$5', 100, 60, 28)')
define(`PKG_JG', `PKG_DIL(`$1', `$2', `$3', `$4', `$5', 100, 60, 28)')
define(`PKG_N',  `PKG_DIL(`$1', `$2', `$3', `$4', `$5', 100, 60, 28)')
define(`PKG_NT', `PKG_DIL(`$1', `$2', `$3', `$4', `$5', 100, 60, 28)')
define(`PKG_P',  `PKG_DIL(`$1', `$2', `$3', `$4', `$5', 100, 60, 28)')
#shrink DIP
define(`PKG_NS', `PKG_DIL(`$1', `$2', `$3', `$4', `$5',  70, 55, 28)')

# -------------------------------------------------------------------
# the definition of a dual-inline package D and DW
# never used by circuits, just a short-cut for others
# width D==244, DW==419
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: number of pins
# $5: width
#
define(`COMMON_D_DW',
	`define(`MAXY', `eval(`$4' / 2 * 50)')
	define(`MAXX', `$5')
	define(`XLOW', `50')
	define(`XHIGH', `eval(MAXX - 50)')
	define(`CENTERX', `eval(MAXX / 2)')
Element(0x00 "$1" "`$2'" "$3" eval(CENTERX + 20) 50 3 100 0x00)
(
	forloop(`i', 1, eval($4 / 2),
		`PAD(0, eval(i * 50 -25), XLOW, eval(i * 50 -25), 20, i)
	')
	forloop(`i', 1, eval($4 / 2),
		`PAD(XHIGH, eval(MAXY - i * 50 +25), MAXX, eval(MAXY - i * 50 +25), 20, eval(i + $4/2))
	')
	ElementLine(XLOW 0 XLOW MAXY 10)
	ElementLine(XLOW MAXY XHIGH MAXY 10)
	ElementLine(XHIGH MAXY XHIGH 0 10)
	ElementLine(XLOW 0 eval(CENTERX - 25) 0 10)
	ElementLine(eval(CENTERX + 25) 0 XHIGH 0 10)
	ElementArc(CENTERX 0 25 25 0 180 10)
	Mark(25 25)
)')

define(`PKG_D', `COMMON_D_DW(`$1', `$2', `$3', `$4', 244)')
define(`PKG_DW', `COMMON_D_DW(`$1', `$2', `$3', `$4', 419)')

