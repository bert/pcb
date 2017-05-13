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
#  Johnstech Evaluation Socket for QFN packages
#  Johnstech Socket Part Numbers 724810 through 724839
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

define(`PKG_GENERIC_JOHNSTECH7248_QFN',
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
	define(`TBWIDTHMM', `ifelse(0,`$5',`551',`eval($5)')')
	define(`TBWIDTH',`ifelse(0,`$5',`551',`eval(`$5'*10000000/254)')')
	# how much pads extend beyond the package edge (1/1000 mil) (the 75 is 0.75 mm)
	define(`EXTOUT', eval(75*100000/254))
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

	# pad size and drill size (mil/100) for the mounting holes
	define(`MOUNTPAD', 7700)
	define(`MOUNTDRILL', 2000)
	# soldermask relief size for mounting holes (mil/100) 
	define(`MOUNTMASK', `eval(MOUNTPAD + 1000)')

	# silkscreen width (mils/100)
	define(`SILKW', 1000)
	# how much the silk screen is moved away from the package (1/1000 mil)
	define(`SILKEXT', `eval(200*100000/254 + SILKW*10/2 + 5*1000)')
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

	# location of mounting holes (mil/100)
	define(`MOUNTX', `eval((LRWIDTH/2 + 39370)/10)')
	define(`MOUNTY', `eval((TBWIDTH/2 + 39370)/10)')

	# latch silkscreen width (mils/100)
	define(`LSILKW', 100)
	# points for latch silk on the left/right sides of the part (mil/100)
	define(`LATCHLRYMIN', `eval((-TBWIDTH/2 - 25*100000/254 )/10)')
	define(`LATCHLRYMAX', `eval(( TBWIDTH/2 + 25*100000/254 )/10)')
	define(`LATCHLRXMIN', SILKX)
	define(`LATCHLRXMAX', `eval(( LRWIDTH/2 + 400*100000/254 )/10)')
	# points for latch silk on the top/bottom sides of the part (mil/100)
	define(`LATCHTBXMIN', `eval((-LRWIDTH/2 - 25*100000/254 )/10)')
	define(`LATCHTBXMAX', `eval(( LRWIDTH/2 + 25*100000/254 )/10)')
	define(`LATCHTBYMIN', SILKY)
	define(`LATCHTBYMAX', `eval(( TBWIDTH/2 + 400*100000/254 )/10)')

	# points for silk showing where the exposed paddle contacts are (mil/100)
	define(`CONTL', 100)
	ifelse(1, eval(TBWIDTHMM>4), define(`CONTL', 200))
	ifelse(1, eval(TBWIDTHMM>5), define(`CONTL', 300))

	# spacing between rows of EP contacts in 1/100 mm.
	define(`CONTS', 0)
	ifelse(1, eval(TBWIDTHMM>5), define(`CONTS', `eval(TBWIDTHMM*100 - 430)'))

	define(`EPY', `eval((CONTL*100000/254)/10)')
	define(`EPXMIN', `eval(((CONTS/2 - 20/2)*100000/254)/10)')
	define(`EPXMAX', `eval(((CONTS/2 + 20/2)*100000/254)/10)')

	# soldermask opening (mil/100)
	define(`MASKSIZE', `eval((400*100000/254 + TBWIDTH)/10)')


# element_flags, description, pcb-name, value, mark_x, mark_y,
# text_x, text_y, text_direction, text_scale, text_flags
Element[0x00000000 "$1" "`$2'" "$3" 0 0 TEXTX TEXTY 0 TEXTSIZE 0x00000000]
(

# left row
define(`CURPIN', 1)
define(`idx',0)
forloop(`i', 1, LRPINS,
	`define(`Y', `eval((-LRYMAX + PITCH*idx)/10)')'
	`Pad[-LRXMAX  Y  -LRXMIN  Y  PADWIDTHMIL100  0 0 "CURPIN" "CURPIN"  0x00000000]'
	`define(`CURPIN', incr(CURPIN))'
	`define(`idx',incr(idx))'
)

# bottom row
define(`idx',0)
forloop(`i', 1, TBPINS,
	`define(`X', `eval((-TBXMAX + PITCH*idx)/10)')'
	`Pad[X  TBYMAX  X  TBYMIN  PADWIDTHMIL100 0 0 "CURPIN" "CURPIN"  0x00000800]'
	`define(`CURPIN', incr(CURPIN))'
	`define(`idx',incr(idx))'
)

# right row
define(`idx',0)
forloop(`i', 1, LRPINS,
	`define(`Y', `eval(( LRYMAX - PITCH*idx)/10)')'
	`Pad[LRXMAX  Y  LRXMIN  Y  PADWIDTHMIL100  0 0 "CURPIN" "CURPIN"  0x00000000]'
	`define(`CURPIN', incr(CURPIN))'
	`define(`idx',incr(idx))'
)

# top row
define(`idx',0)
forloop(`i', 1, TBPINS,
	`define(`X', `eval((TBXMAX - PITCH*idx)/10)')'
	`Pad[X  -TBYMAX  X  -TBYMIN  PADWIDTHMIL100 0 0 "CURPIN" "CURPIN" 0x00000800]'
	`define(`CURPIN', incr(CURPIN))'
	`define(`idx',incr(idx))'
)

# Exposed paddle.  Note that this pad also sets the soldermask
# relief for the entire part.
# Pad(X1, Y1, X2, Y3, width, clearance,
#     soldermask, "pin name", "pin number", flags)
Pad[0 0 0 0 EPSIZE 0 MASKSIZE "CURPIN" "CURPIN" 0x00000100]
define(`CURPIN', incr(CURPIN))
# Mounting pins

# Pin(x, y, thickness, clearance, mask, drilling hole, name,
#     number, flags 
Pin[ MOUNTX MOUNTY MOUNTPAD 1000 MOUNTMASK MOUNTDRILL "Mount1" "CURPIN" 0x0]
define(`CURPIN', incr(CURPIN))
Pin[ -MOUNTX MOUNTY MOUNTPAD 1000 MOUNTMASK MOUNTDRILL "Mount2" "CURPIN" 0x0]
define(`CURPIN', incr(CURPIN))
Pin[ -MOUNTX -MOUNTY MOUNTPAD 1000 MOUNTMASK MOUNTDRILL "Mount3" "CURPIN" 0x0]
define(`CURPIN', incr(CURPIN))
Pin[ MOUNTX -MOUNTY MOUNTPAD 1000 MOUNTMASK MOUNTDRILL "Mount4" "CURPIN" 0x0]

# Silk screen around package
ElementLine[ SILKX  SILKY  SILKX -SILKY SILKW]
ElementLine[ SILKX -SILKY -SILKX -SILKY SILKW]
ElementLine[-SILKX -SILKY -SILKX  SILKY SILKW]
ElementLine[-SILKX  SILKY  SILKX  SILKY SILKW]

# Pin 1 indicator
ElementLine[-SILKX -SILKY eval(-SILKX - 1500) eval(-SILKY - 1500) SILKW]

# Silk showing latch area

# top
ElementLine[ LATCHTBXMIN -LATCHTBYMIN LATCHTBXMIN -LATCHTBYMAX LSILKW ]
ElementLine[ LATCHTBXMIN -LATCHTBYMAX LATCHTBXMAX -LATCHTBYMAX LSILKW ]
ElementLine[ LATCHTBXMAX -LATCHTBYMIN LATCHTBXMAX -LATCHTBYMAX LSILKW ]

# bottom
ElementLine[ LATCHTBXMIN LATCHTBYMIN LATCHTBXMIN LATCHTBYMAX LSILKW ]
ElementLine[ LATCHTBXMIN LATCHTBYMAX LATCHTBXMAX LATCHTBYMAX LSILKW ]
ElementLine[ LATCHTBXMAX LATCHTBYMIN LATCHTBXMAX LATCHTBYMAX LSILKW ]

# left
ElementLine[ -LATCHLRXMIN LATCHLRYMAX -LATCHLRXMAX LATCHLRYMAX LSILKW ]
ElementLine[ -LATCHLRXMAX LATCHLRYMAX -LATCHLRXMAX LATCHLRYMIN LSILKW ]
ElementLine[ -LATCHLRXMIN LATCHLRYMIN -LATCHLRXMAX LATCHLRYMIN LSILKW ]

# right
ElementLine[ LATCHLRXMIN LATCHLRYMAX LATCHLRXMAX LATCHLRYMAX LSILKW ]
ElementLine[ LATCHLRXMAX LATCHLRYMAX LATCHLRXMAX LATCHLRYMIN LSILKW ]
ElementLine[ LATCHLRXMIN LATCHLRYMIN LATCHLRXMAX LATCHLRYMIN LSILKW ]

# Silk showing area for exposed paddle socket contacts
ElementLine[ EPXMIN -EPY EPXMIN  EPY LSILKW ]
ElementLine[ EPXMAX -EPY EPXMAX  EPY LSILKW ]
ElementLine[ EPXMIN  EPY EPXMAX  EPY LSILKW ]
ElementLine[ EPXMIN -EPY EPXMAX -EPY LSILKW ]

# packages with width >= 6.0 mm have 2 rows of contacts
ifelse(1, eval(TBWIDTHMM>5), 
ElementLine[ -EPXMIN -EPY -EPXMIN  EPY LSILKW ]
ElementLine[ -EPXMAX -EPY -EPXMAX  EPY LSILKW ]
ElementLine[ -EPXMIN  EPY -EPXMAX  EPY LSILKW ]
ElementLine[ -EPXMIN -EPY -EPXMAX -EPY LSILKW ]
)

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
define(`PKG_JOHNSTECH_QFN_80', `PKG_GENERIC_JOHNSTECH7248_QFN(`$1',`$2',`$3',`$4',`$5', `80',`36',`50', `$6')')
# 0.65 mm pitch
define(`PKG_JOHNSTECH_QFN_65', `PKG_GENERIC_JOHNSTECH7248_QFN(`$1',`$2',`$3',`$4',`$5', `65',`36',`50', `$6')')
# 0.50 mm pitch, 0.6 mm pad length
define(`PKG_JOHNSTECH_QFN_50', `PKG_GENERIC_JOHNSTECH7248_QFN(`$1',`$2',`$3',`$4',`$5', `50',`36',`50', `$6')')
# 0.50 mm pitch, 0.4 mm pad length
define(`PKG_JOHNSTECH_QFN_50S',`PKG_GENERIC_JOHNSTECH7248_QFN(`$1',`$2',`$3',`$4',`$5', `50',`36',`50', `$6')')
# 0.40 mm pitch
define(`PKG_JOHNSTECH_QFN_40', `PKG_GENERIC_JOHNSTECH7248_QFN(`$1',`$2',`$3',`$4',`$5', `40',`36',`50', `$6')')


# PKG_GENERIC_JOHNSTECH7248_QFN
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

# EXTRACT_BEGIN

##  Johnstech QFN Socket, Series 1MM (724812-724839)

# 4x4 mm
define(`PKG_JOHNSTECH_QFN12_4',  `PKG_JOHNSTECH_QFN_80(`$1',`$2',`$3', `12', `4', `210')')
#define(`PKG_JOHNSTECH_TQFN12_4', `PKG_JOHNSTECH_QFN_80(`$1',`$2',`$3', `12', `4', `210')')

define(`PKG_JOHNSTECH_QFN16_4',  `PKG_JOHNSTECH_QFN_65(`$1',`$2',`$3', `16', `4', `210')')
#define(`PKG_JOHNSTECH_TQFN16_4', `PKG_JOHNSTECH_QFN_65(`$1',`$2',`$3', `16', `4', `210')')

define(`PKG_JOHNSTECH_QFN20_4',  `PKG_JOHNSTECH_QFN_50(`$1',`$2',`$3', `20', `4', `210')')
#define(`PKG_JOHNSTECH_TQFN20_4', `PKG_JOHNSTECH_QFN_50(`$1',`$2',`$3', `20', `4', `210')')

define(`PKG_JOHNSTECH_QFN24_4',  `PKG_JOHNSTECH_QFN_50S(`$1',`$2',`$3', `24', `4', `210')')
#define(`PKG_JOHNSTECH_TQFN24_4', `PKG_JOHNSTECH_QFN_50S(`$1',`$2',`$3', `24', `4', `260')')

# 5x5 mm
define(`PKG_JOHNSTECH_QFN16_5',  `PKG_JOHNSTECH_QFN_80(`$1',`$2',`$3', `16', `5', `310')')
#define(`PKG_JOHNSTECH_TQFN16_5', `PKG_JOHNSTECH_QFN_80(`$1',`$2',`$3', `16', `5', `310')')

define(`PKG_JOHNSTECH_QFN20_5',  `PKG_JOHNSTECH_QFN_65(`$1',`$2',`$3', `20', `5', `310')')
#define(`PKG_JOHNSTECH_TQFN20_5', `PKG_JOHNSTECH_QFN_65(`$1',`$2',`$3', `20', `5', `310')')

define(`PKG_JOHNSTECH_QFN28_5',  `PKG_JOHNSTECH_QFN_50(`$1',`$2',`$3', `28', `5', `310')')
#define(`PKG_JOHNSTECH_TQFN28_5', `PKG_JOHNSTECH_QFN_50(`$1',`$2',`$3', `28', `5', `325')')

define(`PKG_JOHNSTECH_QFN32_5',  `PKG_JOHNSTECH_QFN_50S(`$1',`$2',`$3', `32', `5', `310')')
#define(`PKG_JOHNSTECH_TQFN32_5', `PKG_JOHNSTECH_QFN_50S(`$1',`$2',`$3', `32', `5', `310')')

# 6x6 mm
define(`PKG_JOHNSTECH_QFN36_6',  `PKG_JOHNSTECH_QFN_50(`$1',`$2',`$3', `36', `6', `370')')
#define(`PKG_JOHNSTECH_TQFN36_6', `PKG_JOHNSTECH_QFN_50(`$1',`$2',`$3', `36', `6', `370')')

define(`PKG_JOHNSTECH_QFN40_6',  `PKG_JOHNSTECH_QFN_50S(`$1',`$2',`$3', `40', `6', `410')')
#define(`PKG_JOHNSTECH_TQFN40_6', `PKG_JOHNSTECH_QFN_50S(`$1',`$2',`$3', `40', `6', `410')')

# 7x7 mm
define(`PKG_JOHNSTECH_QFN32_7',  `PKG_JOHNSTECH_QFN_65(`$1',`$2',`$3', `32', `7', `470')')
#define(`PKG_JOHNSTECH_TQFN32_7', `PKG_JOHNSTECH_QFN_65(`$1',`$2',`$3', `32', `7', `470')')

define(`PKG_JOHNSTECH_QFN44_7',  `PKG_JOHNSTECH_QFN_50(`$1',`$2',`$3', `44', `7', `470')')
#define(`PKG_JOHNSTECH_TQFN44_7', `PKG_JOHNSTECH_QFN_50(`$1',`$2',`$3', `44', `7', `470')')

define(`PKG_JOHNSTECH_QFN48_7',  `PKG_JOHNSTECH_QFN_50S(`$1',`$2',`$3', `48', `7', `560')')
#define(`PKG_JOHNSTECH_TQFN48_7', `PKG_JOHNSTECH_QFN_50S(`$1',`$2',`$3', `48', `7', `560')')

# 8x8 mm
define(`PKG_JOHNSTECH_QFN56_8',  `PKG_JOHNSTECH_QFN_50S(`$1',`$2',`$3', `56', `8', `665')')
#define(`PKG_JOHNSTECH_TQFN56_8', `PKG_JOHNSTECH_QFN_50S(`$1',`$2',`$3', `56', `8', `665')')

# 10x10 mm
define(`PKG_JOHNSTECH_QFN68_10',  `PKG_JOHNSTECH_QFN_50(`$1',`$2',`$3', `68', `10', `770')')
#define(`PKG_JOHNSTECH_TQFN68_10', `PKG_JOHNSTECH_QFN_50(`$1',`$2',`$3', `68', `10', `770')')

# EXTRACT_END

