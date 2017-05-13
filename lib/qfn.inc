# -*- m4 -*-
#                            COPYRIGHT
#
#   PCB, interactive printed circuit board design
#   Copyright (C) 2003 Dan McMahill
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
#  QFN packages
#
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: number of pins
# $5: body size (mm)
# $6: pad pitch (1/100 mm)
# $7: PCB pad width (1/100 mm)
# $8: Package pad length (1/100 mm)
# $9: Exposed paddle size, 0 for no exposed paddle (1/100 mm)

define(`PKG_GENERIC_QFN',
       `
	# number of pins on left/right sides (pin1 is upper pin on left side)
	define(`LRPINS',  `eval($4 / 4)')
	# number of pins on top/bottom sides
        define(`TBPINS', `eval(`$4' / 2 - LRPINS)')
	# pin pitch (1/1000 mil)
	define(`PITCH',`eval(`$6'*100000/254)')
	# y-coordinate for upper pin on left/right sides  (1/1000 mil)
	define(`LRYMAX', `eval((LRPINS-1)*PITCH/2)')
	# x-coordinate for right pin on top/bottom sides  (1/1000 mil)
	define(`TBXMAX', `eval((TBPINS-1)*PITCH/2)')
	# total horizontal package width (1/1000 mil)
	define(`LRWIDTHMM', `ifelse(0,`$5',`787',`eval($5)')')
	define(`LRWIDTH', `ifelse(0,`$5',`787',`eval(`$5'*10000000/254)')')
	# total vertical package width (1/1000 mil)
	define(`TBWIDTH',`ifelse(0,`$5',`551',`eval(`$5'*10000000/254)')')
	# how much pads extend beyond the package edge (1/1000 mil) (the 25 is 0.25 mm)
	define(`EXTOUT', eval(25*100000/254))
	# how much pads extend inward from the package pad edge (1/1000 mil)
	define(`EXTIN', eval(5*100000/254))
	# pad length/width (1/1000 mil)
	define(`PADLENGTH', `eval(`$8'*100000/254)')
	define(`PADWIDTH', `eval(`$7'*100000/254)')
	# pad width (mil/100)
	define(`PADWIDTHMIL100', `eval(PADWIDTH/10)')
	# min/max x coordinates for the pads on the left/right sides of the package (mil/100)
	define(`LRXMAX',`eval((LRWIDTH/2 + EXTOUT - PADWIDTH/2)/10)')
	define(`LRXMIN',`eval((LRWIDTH/2 -PADLENGTH - EXTIN + PADWIDTH/2)/10)')
	# min/max y coordinates for the pads on the top/bottom sides of the package (mil/100)
	define(`TBYMAX',`eval((TBWIDTH/2 + EXTOUT - PADWIDTH/2)/10)')
	define(`TBYMIN',`eval((TBWIDTH/2 -PADLENGTH - EXTIN + PADWIDTH/2)/10)')

	# silkscreen width (mils/100)
	define(`SILKW', 1000)
	# how much the silk screen is moved away from the package (1/1000 mil)
	define(`SILKEXT', `eval(EXTOUT + SILKW*10/2 + 5*1000)')
	# upper right corner for silk screen (mil/100)
	define(`SILKX', `eval((LRWIDTH/2 + SILKEXT )/10)')
	define(`SILKY', `eval((TBWIDTH/2 + SILKEXT )/10)')
	# refdes text size (mil/100)
	define(`TEXTSIZE', 100)
	# x,y coordinates for refdes label (mil/100)
	define(`TEXTX', -SILKX)
	define(`TEXTY', `eval(-SILKY - 1000 - TEXTSIZE/2)')
	# square exposed paddle size (mil/100)
	define(`EPSIZE', `eval(`$9'*10000/254)')

   # pad clearance to polygons (1/100 mil)
   define(`PADCLEAR', `2000')

   # width of the pad solder mask relief (1/100 mil). 
   # grow by 1.5 mils on each side
   define(`PADMASK', `eval(2*150 + PADWIDTHMIL100)')

   # width of the paddle soldermask relief (1/100 mil)
   # grow by 200 mils on each side
   define(`EPMASK', `eval(2*200 + EPSIZE)')

# element_flags, description, pcb-name, value, mark_x, mark_y,
# text_x, text_y, text_direction, text_scale, text_flags
Element[0x00000000 "$1" "`$2'" "$3" 0 0 TEXTX TEXTY 0 TEXTSIZE 0x00000000]
(


# Pad[X1, Y1, X2, Y3, width, clearance,
#     soldermask, "pin name", "pin number", flags]

# left row
define(`CURPIN', 1)
define(`idx',0)
forloop(`i', 1, LRPINS,
	`define(`Y', `eval((-LRYMAX + PITCH*idx)/10)')'
	`Pad[-LRXMAX  Y  -LRXMIN  Y  PADWIDTHMIL100  PADCLEAR PADMASK "CURPIN" "CURPIN"  0x00000100]'
	`define(`CURPIN', incr(CURPIN))'
	`define(`idx',incr(idx))'
)

# bottom row
define(`idx',0)
forloop(`i', 1, TBPINS,
	`define(`X', `eval((-TBXMAX + PITCH*idx)/10)')'
	`Pad[X  TBYMAX  X  TBYMIN  PADWIDTHMIL100 PADCLEAR PADMASK "CURPIN" "CURPIN"  0x00000900]'
	`define(`CURPIN', incr(CURPIN))'
	`define(`idx',incr(idx))'
)

# right row
define(`idx',0)
forloop(`i', 1, LRPINS,
	`define(`Y', `eval(( LRYMAX - PITCH*idx)/10)')'
	`Pad[LRXMAX  Y  LRXMIN  Y  PADWIDTHMIL100  PADCLEAR PADMASK "CURPIN" "CURPIN"  0x00000100]'
	`define(`CURPIN', incr(CURPIN))'
	`define(`idx',incr(idx))'
)

# top row
define(`idx',0)
forloop(`i', 1, TBPINS,
	`define(`X', `eval((TBXMAX - PITCH*idx)/10)')'
	`Pad[X  -TBYMAX  X  -TBYMIN  PADWIDTHMIL100 PADCLEAR PADMASK "CURPIN" "CURPIN" 0x00000900]'
	`define(`CURPIN', incr(CURPIN))'
	`define(`idx',incr(idx))'
)

# Exposed paddle (if this is an exposed paddle part)
ifelse(1, eval(EPSIZE>0), 
# Pad(X1, Y1, X2, Y3, width, clearance,
#     soldermask, "pin name", "pin number", flags)
	Pad[0 0 0 0 EPSIZE 0 EPMASK "CURPIN" "CURPIN" 0x00000100]
	define(`CURPIN', incr(CURPIN))
)

# Silk screen around package
ElementLine[ SILKX  SILKY  SILKX -SILKY SILKW]
ElementLine[ SILKX -SILKY -SILKX -SILKY SILKW]
ElementLine[-SILKX -SILKY -SILKX  SILKY SILKW]
ElementLine[-SILKX  SILKY  SILKX  SILKY SILKW]

# Pin 1 indicator
ElementLine[-SILKX -SILKY eval(-SILKX - 1500) eval(-SILKY - 1500) SILKW]

)')

# -------------------------------------------------------------------

# The following macros take:
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: number of pins
# $5: body size (mm)
# $6: exposed paddle size (1/100 mm).  0 for no exposed paddle


# For an exposed paddle package, the paddle for an 'n' pin package is pin 'n+1'

# 0.8 mm pitch
define(`PKG_QFN_80', `PKG_GENERIC_QFN(`$1',`$2',`$3',`$4',`$5', `80',`42',`60', `$6')')
# 0.65 mm pitch
define(`PKG_QFN_65', `PKG_GENERIC_QFN(`$1',`$2',`$3',`$4',`$5', `65',`37',`60', `$6')')
# 0.50 mm pitch, 0.6 mm pad length
define(`PKG_QFN_50', `PKG_GENERIC_QFN(`$1',`$2',`$3',`$4',`$5', `50',`28',`60', `$6')')
# 0.50 mm pitch, 0.4 mm pad length
define(`PKG_QFN_50S',`PKG_GENERIC_QFN(`$1',`$2',`$3',`$4',`$5', `50',`28',`40', `$6')')
# 0.40 mm pitch
define(`PKG_QFN_40', `PKG_GENERIC_QFN(`$1',`$2',`$3',`$4',`$5', `40',`25',`60', `$6')')


# PKG_GENERIC_QFN
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: number of pins
# $5: body size (mm)
# $6: pad pitch (1/100 mm)
# $7: PCB pad width (1/100 mm)
# $8: Package pad length (1/100 mm)
# $9: Exposed paddle size, 0 for no exposed paddle (1/100 mm)
# -------------------------------------------------------------------

