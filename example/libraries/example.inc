# -*- m4 -*-
#
# Example .inc file


# -------------------------------------------------------------------
# the definition for suface mount resistors and capacitors
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: pad width   (in direction perpendicular to part)
# $5: pad length  (in direction parallel with part)
# $6: pad spacing (center to center)
# $7: distance from edge of pad to silk (in direction
#     perpendicular to part)
# $8: distance from edge of pad to silk (in direction
#     parallel with part)
# $9: Set to "no" to skip silk screen on the sides of the part

define(`COMMON_PKG_RCSMT',
	`define(`XMIN', `eval( -1*`$6'/2 - `$5'/2 - `$8')')	
	define(`XMAX', `eval(  `$6'/2 + `$5'/2 + `$8')')
	define(`YMIN', `eval(-1*`$4'/2 - `$7')')	
	define(`YMAX', `eval(   `$4'/2 + `$7')')
Element(0x00 "$1" "$2" "$3" eval(XMIN+20) eval(YMAX+20) 0 100 0x00)
(
	ifelse(0, eval($4>$5),
	# Pads which have the perpendicular pad dimension less
	# than or equal to the parallel pad dimension 	
	Pad(eval(-1*(   $6 + $5 - $4)/2) 0 
            eval((-1*$6 + $5 - $4)/2) 0 eval($4) "1" 0x100)
	Pad(eval(-1*(-1*$6 + $5 - $4)/2) 0 
            eval((   $6 + $5 - $4)/2) 0 eval($4) "2" 0x100)
        ,
	# Pads which have the perpendicular pad dimension greater
	# than or equal to the parallel pad dimension 
 	Pad(eval(-1*$6/2) eval(-1*($4 - $5)/2) 
            eval(-1*$6/2)  eval(($4 - $5)/2) eval($5) "1" 0x100)
 	Pad(eval(   $6/2) eval(-1*($4 - $5)/2) 
            eval(   $6/2)  eval(($4 - $5)/2) eval($5) "2" 0x100)
	)

	# silk screen
	# ends
	ElementLine(XMIN YMIN XMIN YMAX 10)
	ElementLine(XMAX YMAX XMAX YMIN 10)
	# sides
ifelse($9,"no",
	#skip side silk
	,
	ElementLine(XMIN YMIN XMAX YMIN 10)
	ElementLine(XMAX YMAX XMIN YMAX 10)
)
	Mark(0 0)
)')


# 0402 package
#
# 30x30 mil pad, 15 mil metal-metal spacing=> 
# 15 + 15 + 15 = 45 center-to-center
define(`PKG_RC0402', 
  `COMMON_PKG_RCSMT(`$1', `$2', `$3', 30, 30, 45, 0, 10, "no")')

# 0603 package
#
# 40x40 mil pad, 30 mil metal-metal spacing=>
#  30 + 20 + 20 = 70 center-to-center
define(`PKG_RC0603', 
  `COMMON_PKG_RCSMT(`$1', `$2', `$3', 40, 40, 70, 10, 10)')

# 1206 package
#
# 40x60 mil pad, 90 mil metal-metal spacing=> 
#  90 + 20 + 20 = 130 center-to-center
define(`PKG_RC1206', 
  `COMMON_PKG_RCSMT(`$1', `$2', `$3', 60, 40, 130, 10, 10)')

