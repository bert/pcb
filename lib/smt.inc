# -*- m4 -*-
#   $Id$
#
#   surface mounted components
#
#   revisions:
#   Oct 10 2002 - Egil Kvaleberg <egil@kvaleberg.no>
#   Initial stub
#
#


# -------------------------------------------------------------------
#
# internal: general purpose two pole surface mount
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: length of component in mil
# $5: width of component in mil, also width of pad
# $6: length of pad, expressed in percent of component length
# $7: 1 if pin 1 should be marked
#
define(`COMMON_SMT_2PAD_MIL',
       `define(`sizX',     `$4')
	define(`sizY',     `$5')
	define(`metalX',   `eval((sizX * $6) / 100)')
	define(`addFRAME', `eval(sizY / 5)')
	define(`addTIN',   `eval(sizY / 5)')
	define(`padX',     `eval(metalX + 2*addTIN)')
	define(`padY',     `eval(sizY + 2*addTIN)')
	define(`maxX',     `eval(sizX + 2*addFRAME + 2*addTIN)')
	define(`maxY',     `eval(sizY + 2*addFRAME + 2*addTIN)')
	define(`centerY',  `eval(maxY / 2)')
	define(`centerX1', `eval(addFRAME + padX/2)')
	define(`centerX2', `eval(maxX - centerX1)')
	define(`silkW',    `ifelse(eval(sizY >= 50), 1, 10, eval(sizY/5))')
Element(0x00 "$1" "$2" "$3" eval(maxX + 20) 0 3 100 0x00)
(
	ifelse(eval($7 > 0), 1,
	   `ElementLine(0 addFRAME 0 eval(maxY-addFRAME) eval(2*silkW))
	    ElementLine(0 eval(maxY-addFRAME) eval(padX/2) maxY silkW)
	    ElementLine(eval(padX/2) maxY maxX maxY silkW)
	    ElementLine(maxX maxY maxX 0 silkW)
	    ElementLine(maxX 0 eval(padX/2) 0 silkW)
	    ElementLine(eval(padX/2) 0 0 addFRAME silkW)
	   ', 
	   `ElementLine(0 0 0 maxY silkW)
	    ElementLine(0 maxY maxX maxY silkW)
	    ElementLine(maxX maxY maxX 0 silkW)
	    ElementLine(maxX 0 0 0 silkW)
	   ')

	ifelse(eval(padX > padY), 1,       
	   `Pad(eval(addFRAME+padY/2) centerY eval(addFRAME+padX-padY/2) centerY
							padY "1" "1" 0x100)
	    Pad(eval(maxX-(addFRAME+padY/2)) centerY eval(maxX-(addFRAME+padX-padY/2)) centerY
							padY "2" "2" 0x100)
	 ',
	   `Pad(centerX1 eval(addFRAME+padX/2) centerX1 eval(addFRAME+padY-padX/2)
							padX "1" "1" 0x100)
	    Pad(centerX2 eval(addFRAME+padX/2) centerX2 eval(addFRAME+padY-padX/2)
							padX "2" "2" 0x100)
	 ')

	Mark(eval(addFRAME+padX/2) centerY)
)')
# -------------------------------------------------------------------
# 
#
# general purpose 3 pole surface mount
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: outer pad width   (mil) (in direction perpendicular to part)
# $5: outer pad length  (mil) (in direction parallel with part)
# $6: outer pad spacing (mil) (center to center)
# $7: inner pad width  (mil) (in direction perpendicular to part)
# $8: inner pad length (mil) (in direction parallel with part)
# $9: distance from edge of pad to silk (mil) (in direction
#     perpendicular to part)
# $10: distance from edge of pad to silk (mil) (in direction
#     parallel with part)
# $11: Set to "no" to skip silk screen on the sides of the part
define(`COMMON_SMT_3PAD_MIL',
	`
	define(`PADW', `$4')
	define(`PADL', `$5')
	define(`PADS', `$6')
	define(`PADW2',`$7')
	define(`PADL2',`$8')
	define(`SLKW', `$9')
	define(`SLKL', `$10')

	# silk screen width (mils)
	define(`SILKW', `10')
	
	# silk screen bounding box
	define(`XMIN', `eval( -PADS/2 - PADL/2 - SLKL - SILKW/2)')	
	define(`XMAX', `eval(  PADS/2 + PADL/2 + SLKL + SILKW/2)')
	define(`YMIN', `eval(-PADW2/2 - SLKW - SILKW/2)')	
	define(`YMAX', `eval( PADW2/2 + SLKW + SILKW/2)')
	define(`SKIP_SILK', `$11')

		
Element(0x00 "$1" "$2" "$3" eval(XMIN+20) eval(YMAX+20) 0 100 0x00)
(
	ifelse(0, eval(PADW>PADL),
	# Pads which have the perpendicular pad dimension less
	# than or equal to the parallel pad dimension 	
	Pad(eval(-1*(   PADS + PADL - PADW)/2) 0 
            eval((-1*PADS + PADL - PADW)/2) 0 eval(PADW) "1" 0x100)
	Pad(eval(-1*(-1*PADS + PADL - PADW)/2) 0 
            eval((   PADS + PADL - PADW)/2) 0 eval(PADW) "3" 0x100)
        ,
	# Pads which have the perpendicular pad dimension greater
	# than or equal to the parallel pad dimension 
 	Pad(eval(-1*PADS/2) eval(-1*(PADW - PADL)/2) 
            eval(-1*PADS/2)  eval((PADW - PADL)/2) eval(PADL) "1" 0x100)
 	Pad(eval(   PADS/2) eval(-1*(PADW - PADL)/2) 
            eval(   PADS/2)  eval((PADW - PADL)/2) eval(PADL) "3" 0x100)
	)

	ifelse(0, eval(PADW2>PADL2),
	# Pads which have the perpendicular pad dimension less
	# than or equal to the parallel pad dimension 	
	Pad(eval((-PADL2 + PADW2)/2) 0 
            eval(( PADL2 - PADW2)/2) 0 PADW2 "2" 0x100)
        ,
	# Pads which have the perpendicular pad dimension greater
	# than or equal to the parallel pad dimension 
 	Pad(0 eval((-PADW2 + PADL2)/2) 
            0 eval(( PADW2 - PADL2)/2) PADL2 "2" 0x100)
	)

	# silk screen
	# ends
	ElementLine(XMIN YMIN XMIN YMAX SILKW)
	ElementLine(XMAX YMAX XMAX YMIN SILKW)
	# sides
ifelse(SKIP_SILK,"no",
	#skip side silk
	,
	ElementLine(XMIN YMIN XMAX YMIN SILKW)
	ElementLine(XMAX YMAX XMIN YMAX SILKW)
)
	# Mark the common centroid of the part
	Mark(0 0)
)')


#
# -------------------------------------------------------------------
#
# SOT23 style transistor: 3, 4, 5 or 6 pins
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: pin spacing, lengthwise, mil
# $5: pin spacing across, in mil
# $6: size of pad, in percent of spacing across
# $7: pin numbers, "abc", where a is lower left, b is lower right, c is upper
#     if "abcd", c is upper right, d is upper left, and a is a wider pad
#
define(`COMMON_SMT_TRANSISTOR_MIL',
       `define(`spaceX',   `$4')                             # 78 for SOT23
	define(`spaceY',   `$5')                             # 82 for SOT23
	define(`padY',     `eval((spaceY * $6) / 100)')      # 41 for SOT23
	define(`padX',     `ifelse(eval(len(`$7') >= 5), 1,
			       `eval((spaceX * 31) / 100)',
			       `eval((padY * 85) / 100)'
			     )')               # 34 for SOT23, 24 for SOT25
	define(`addFRAME', `eval(padY / 5)')
	define(`maxX',     `eval(2*addFRAME+spaceX+padX)')
	define(`maxY',     `eval(2*addFRAME+spaceY+padY)')
	define(`centerX',  `eval(maxX / 2)')
	define(`centerY1', `eval(addFRAME + padY/2)')
	define(`centerX1', `eval(addFRAME + padX/2)')
	define(`deltaY',   `eval((padY-padX)/2)')
	define(`silkW',    `ifelse(eval(spaceY >= 50), 1, 10, eval(spaceY/5))')
	define(`indeX',    `0')

Element(0x00 "$1" "$2" "$3" eval(maxX+20) 0 3 100 0x00)
(
	ElementLine(0 0 0 maxY silkW)
	ElementLine(0 maxY maxX maxY silkW)
	ElementLine(maxX maxY maxX 0 silkW)
	ElementLine(maxX 0 0 0 silkW)

	# 1st side, 1st pin
	ifelse(len(`$7'), 4,
	  `# extra width
	   Pad(eval(centerX1+deltaY) eval(centerY1+spaceY)
	       eval(centerX1+2*deltaY) eval(centerY1+spaceY)
			   padX "substr(`$7',indeX,1)" "substr(`$7',indeX,1)" 0x100)
	 ',
	  `Pad(centerX1 eval(centerY1+spaceY-deltaY)
	       centerX1 eval(centerY1+spaceY+deltaY)
			   padX "substr(`$7',indeX,1)" "substr(`$7',indeX,1)" 0x100)
	 ')
	define(`indeX',incr(indeX))

	# 1st side, 2nd pin
	ifelse(eval(len(`$7') == 6), 1,
	  `Pad(centerX eval(centerY1+spaceY-deltaY)
	       centerX eval(centerY1+spaceY+deltaY)
			   padX "substr(`$7',indeX,1)" "substr(`$7',indeX,1)" 0x100)
	   define(`indeX',incr(indeX))
	 ')

	# 1st side, 3rd pin
	Pad(eval(centerX1+spaceX) eval(centerY1+spaceY-deltaY)
	    eval(centerX1+spaceX) eval(centerY1+spaceY+deltaY)
			   padX "substr(`$7',indeX,1)" "substr(`$7',indeX,1)" 0x100)
	define(`indeX',incr(indeX))

	# 2nd side, 3rd pin
	ifelse(eval(len(`$7') >= 4), 1,
	  `Pad(eval(centerX1+spaceX) eval(centerY1-deltaY)
	       eval(centerX1+spaceX) eval(centerY1+deltaY)
			   padX "substr(`$7',indeX,1)" "substr(`$7',indeX,1)" 0x100)
	   define(`indeX',incr(indeX))
	 ')

	# 2nd side, 2nd pin
	ifelse(eval(len(`$7') != 4), 1,
	  `Pad(centerX eval(centerY1-deltaY)
	       centerX eval(centerY1+deltaY)
			   padX "substr(`$7',indeX,1)" "substr(`$7',indeX,1)" 0x100)
	   define(`indeX',incr(indeX))
	 ')

	# 2nd side, 1st pin
	ifelse(eval(len(`$7') >= 4), 1,
	  `Pad(centerX1 eval(centerY1-deltaY)
	       centerX1 eval(centerY1+deltaY)
			   padX "substr(`$7',indeX,1)" "substr(`$7',indeX,1)" 0x100)
	 ')

	Mark(centerX1 eval(centerY1+spaceY))
)')
#
# -------------------------------------------------------------------
#
# SOT223 style transistor, multiple pins on one side, large pin on other side
# supports 2+1, 3+1 and 4+1 pins
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: pin spacing, lengthwise, mil
# $5: pin spacing across, in mil
# $6: size of pad, in percent of spacing across
# $7: pin numbers, "abcd", where abc is three pins on one side, d the extra pin
#
define(`COMMON_SMT_TRANSISTORX_MIL',
       `define(`spaceX',   `$4')
	define(`spaceY',   `$5')
	define(`padY',     `eval((spaceY * $6) / 100)')
	define(`padX',     `ifelse(eval(len(`$7') >= 5), 1,
			       `eval((spaceX * 21) / 100)',
			       `eval((spaceX * 31) / 100)'
			     )')
	define(`addFRAME', `eval(padY / 5)')
	define(`maxX',     `eval(2*addFRAME+spaceX+padX)')
	define(`maxY',     `eval(2*addFRAME+spaceY+padY)')
	define(`centerX',  `eval(maxX / 2)')
	define(`centerY1', `eval(addFRAME + padY/2)')
	define(`centerX1', `eval(addFRAME + padX/2)')
	define(`deltaY',   `eval((padY-padX)/2)')
	define(`silkW',    `ifelse(eval(spaceY >= 50), 1, 10, eval(spaceY/5))')
	define(`indeX',    `0')

Element(0x00 "$1" "$2" "$3" eval(maxX+20) 0 3 100 0x00)
(
	ElementLine(0 0 0 maxY silkW)
	ElementLine(0 maxY maxX maxY silkW)
	ElementLine(maxX maxY maxX 0 silkW)
	ElementLine(maxX 0 0 0 silkW)

	# 1st pin on pin side
	Pad(centerX1 eval(centerY1+spaceY-deltaY)
	    centerX1 eval(centerY1+spaceY+deltaY)
			   padX "substr(`$7',indeX,1)" "substr(`$7',indeX,1)" 0x100)
	define(`indeX',incr(indeX))

	ifelse(eval(len(`$7') == 4), 1,
	  `Pad(centerX eval(centerY1+spaceY-deltaY)
	       centerX eval(centerY1+spaceY+deltaY)
			   padX "substr(`$7',indeX,1)" "substr(`$7',indeX,1)" 0x100)
	   define(`indeX',incr(indeX))
	 ')
	ifelse(eval(len(`$7') == 5), 1,
	  `Pad(eval(centerX1+spaceX/3) eval(centerY1+spaceY-deltaY)
	       eval(centerX1+spaceX/3) eval(centerY1+spaceY+deltaY)
			   padX "substr(`$7',indeX,1)" "substr(`$7',indeX,1)" 0x100)
	   define(`indeX',incr(indeX))
	   Pad(eval(centerX1+2*spaceX/3) eval(centerY1+spaceY-deltaY)
	       eval(centerX1+2*spaceX/3) eval(centerY1+spaceY+deltaY)
			   padX "substr(`$7',indeX,1)" "substr(`$7',indeX,1)" 0x100)
	   define(`indeX',incr(indeX))
	 ')

	# last pin on pin side
	Pad(eval(centerX1+spaceX) eval(centerY1+spaceY-deltaY)
	    eval(centerX1+spaceX) eval(centerY1+spaceY+deltaY)
			   padX "substr(`$7',indeX,1)" "substr(`$7',indeX,1)" 0x100)
	define(`indeX',incr(indeX))

	# extra wide pin on opposite side
	Pad(eval(centerX+spaceX/4) centerY1
	    eval(centerX-spaceX/4) centerY1
			   padY "substr(`$7',indeX,1)" "substr(`$7',indeX,1)" 0x100)

	Mark(centerX1 eval(centerY1+spaceY))
)')
#
# -------------------------------------------------------------------
#
#
# definition of a SMT dual inline package of any size
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: number of pins
# $5: width of component itself in mils (228 for SO)    -> 261
# $6: pin spacing in 1/100 mils (500 for SO)
# $7: pad fatness in mils (20 for SO)
# $8: pad basic length in mils, will add extra tin here (50 for SO)
#
define(`COMMON_SMT_DUALINLINE',
       `define(`pinCOUNT', `$4')
	define(`pinS100',  `$6')
	define(`padY',    `$7')
	define(`addTIN',  `eval(padY / 5)')
	define(`maxX',    `eval($5 + 2*addTIN)')
	define(`padX',    `eval($8 + 2*addTIN)')
	define(`maxY',    `eval((pinCOUNT/2 * pinS100) / 100)')
	define(`lowX1',   `eval(padY/2)')
	define(`highX1',  `eval(padX-padY)')
	define(`lowX2',   `eval(maxX - (padX-padY))')
	define(`highX2',  `eval(maxX - padY/2)')
	define(`centerX', `eval(maxX / 2)')
Element(0x00 "$1" "$2" "$3" eval(centerX + 20) 50 3 100 0x00)
(
	forloop(`i', 1, eval(pinCOUNT/2),
		`PAD(lowX1, eval((i*pinS100 - pinS100/2)/100),
		    highX1, eval((i*pinS100 - pinS100/2)/100), padY, i)
	')
	forloop(`i', 1, eval(pinCOUNT/2),
		`PAD(lowX2, eval(maxY - (i*pinS100 - pinS100/2)/100),
		    highX2, eval(maxY - (i*pinS100 - pinS100/2)/100), padY, eval(i + pinCOUNT/2))
	')
	ElementLine(0 0 eval(centerX + pinS100/200) 0 rSILK)
	ElementArc(centerX 0 eval(pinS100/200) eval(pinS100/200) 0 180 rSILK)
	ElementLine(eval(centerX - pinS100/200) 0 maxX 0 rSILK)
	ElementLine(maxX 0 maxX maxY rSILK)
	ElementLine(maxX maxY 0 maxY rSILK)
	ElementLine(0 maxY 0 0 rSILK)
	Mark(eval(padX/2) eval(pinS100/200))
)')
#
#
# -------------------------------------------------------------------
#
# general purpose two pole surface mount
# size args for a 0805 is 8,5 for 0402 is 4,2 etc
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: length of component in mil
# $5: width of component in mil
#
define(`PKG_SMT_2PAD_MIL',
  `COMMON_SMT_2PAD_MIL(`$1', `$2', `$3', `$4', `$5', 20, 0)')
#
# general purpose two pole surface mount, pin 1 is marked
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: length of component in 1/10 mm
# $5: width of component in 1/10 mm
#
define(`PKG_SMT_2PAD_EIA',
  `COMMON_SMT_2PAD_MIL(`$1', `$2', `$3',
		  `eval(($4*1000)/254)', `eval(($5*1000)/254)', 20, 1)')
#
#
# general purpose diode surface mount, pin 1 is marked
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: length of component in 1/10 mm
# $5: width of component in 1/10 mm
#
define(`PKG_SMT_DIODE',
  `COMMON_SMT_2PAD_MIL(`$1', `$2', `$3',
		  `eval(($4*1000)/254)', `eval(($5*1000)/254)', 30, 1)')
#
#
# diode in transistor housing, 2 active pins, 1 is cathode, 2 is anode
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: pin spacing lengthwise in 1/10 mm
# $5: pin spacing across in 1/10 mm
#
define(`PKG_SMT_TRANSISTOR2',
  `COMMON_SMT_TRANSISTOR_MIL(`$1', `$2', `$3',
		  `eval(($4*1000)/254)', `eval(($5*1000)/254)', 50, `231')')
#
#
#
# general purpose transistor, 3 pins
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: pin spacing lengthwise in 1/10 mm
# $5: pin spacing across in 1/10 mm
#
define(`PKG_SMT_TRANSISTOR3',
  `COMMON_SMT_TRANSISTOR_MIL(`$1', `$2', `$3',
		  `eval(($4*1000)/254)', `eval(($5*1000)/254)', 50, `123')')
#
#
# general purpose transistor, 4 pins
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: pin spacing lengthwise in 1/10 mm
# $5: pin spacing across in 1/10 mm
#
define(`PKG_SMT_TRANSISTOR4',
  `COMMON_SMT_TRANSISTOR_MIL(`$1', `$2', `$3',
		  `eval(($4*1000)/254)', `eval(($5*1000)/254)', 50, `1234')')
#
#
# general purpose transistor, 5 pins
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: pin spacing lengthwise in 1/10 mm
# $5: pin spacing across in 1/10 mm
#
define(`PKG_SMT_TRANSISTOR5',
  `COMMON_SMT_TRANSISTOR_MIL(`$1', `$2', `$3',
		  `eval(($4*1000)/254)', `eval(($5*1000)/254)', 50, `12345')')
define(`PKG_SMT_TRANSISTOR5A',
  `COMMON_SMT_TRANSISTOR_MIL(`$1', `$2', `$3',
		  `eval(($4*1000)/254)', `eval(($5*1000)/254)', 50, `45123')')
#
#
# general purpose transistor, 6 pins
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: pin spacing lengthwise in 1/10 mm
# $5: pin spacing across in 1/10 mm
#
define(`PKG_SMT_TRANSISTOR6',
  `COMMON_SMT_TRANSISTOR_MIL(`$1', `$2', `$3',
		  `eval(($4*1000)/254)', `eval(($5*1000)/254)', 50, `123456')')
#
#
# general purpose transistor, 3 pins on one side, cooling tab/pin on other
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: pin spacing lengthwise in 1/10 mm
# $5: pin spacing across in 1/10 mm
#
define(`PKG_SMT_TRANSISTOR4X',
  `COMMON_SMT_TRANSISTORX_MIL(`$1', `$2', `$3',
		  `eval(($4*1000)/254)', `eval(($5*1000)/254)', 50, `1234')')
#
# SO package, narrow and wide
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: pin count
# $5: total width
# $6: pin pitch 1/100mil
#
# BUG: total package size in Y is larger
#
#
# 14 plasticX,Y=157,344
# 24W plasticX,Y=299,614
define(`PKG_SO',   `COMMON_SMT_DUALINLINE(`$1',`$2',`$3',`$4',`$5',`$6',20,50)')
#
# 8..28
# NOTE: 24 plasticX,Y=245,368
define(`PKG_SSO',  `COMMON_SMT_DUALINLINE(`$1',`$2',`$3',`$4',`$5',`$6',16,40)')
#
# 28, 32A, 32B
# NOTE: 28 plasticX,Y=469,319
# NOTE: 32A plasticX,Y=728,319
# BUG: check total size wrt rounding, it shoudl
# BUG: numbering is all different -- check
define(`PKG_TSOP', `COMMON_SMT_DUALINLINE(`$1',`$2',`$3',`$4',`$5',`$6',12,40)')
#
# 8, TSSOP56 TSSOP64
# NOTE: 8 plasticX,Y=177,122
# BUG: check total size wrt rounding
# BUG: numbering is all different -- check
define(`PKG_TSSOP', `COMMON_SMT_DUALINLINE(`$1',`$2',`$3',`$4',260,2600,14,40)')
#
# BUG: add SO8ePAD = SO8 plus dX=95+ dy=122+ solder pad
#efine(`xKG_SO',   `COMMON_SMT_DUALINLINE(`$1',`$2',`$3',`$4',244,5000,20,50)')
# BUG: add PowerSO.PDF = MO166 - include dx=244+ dy=630 wide strip / X,Y=437,630
#efine(`PKG_MO166',  `COMMON_SMT_DUALINLINE(`$1',`$2',`$3',`$4',570,5000,25,50)') # 24:


# -------------------------------------------------------------------
# the definition of a SMT dual inline package
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: number of pins
# $5: pad width  (1/1000 mil)
# $6: pad length (1/1000 mil)
# $7: pad pitch (1/1000 mil)
# $8: pad seperation center to center for pads on opposite sides of
#     the package (1/1000 mil)
# $9: define to make the pins get numbered starting with the highest pin
#     instead of pin 1.  Needed for certain brain damaged packages like
#     the Mini-Circuits KK81
# pin 1 will be upper left, pin N/2 will be lower left,
# pin N will be upper right as defined here
define(`COMMON_SMT_DIL_MIL',
        `
	# number of pads
	define(`NPADS', `$4')
	# pad width in 1/1000 mil
        define(`PADWIDTH', `$5')
	# pad length in 1/1000 mil
        define(`PADLENGTH',`$6')
	# pad pitch 1/1000 mil
	define(`PITCH',`$7')
	# seperation between pads on opposite sides 1/1000 mil
	define(`PADSEP',`$8')

	# X coordinates for the right hand column of pads (mils)
        define(`X1', `eval( (PADSEP/2 + PADLENGTH - PADWIDTH/2)/1000)')
        define(`X2', `eval( (PADSEP/2 + PADWIDTH/2)/1000)')

	# silk screen width (mils)
	define(`SILKW', `10')
	define(`SILKSEP', `5')


	# figure out if we have an even or odd number of pins per side
	define(`TMP1', eval(NPADS/4))
	define(`TMP2', eval((4*TMP1 - NPADS) == 0))
	ifelse(TMP2, 1, `define(`EVEN',"yes")', `define(`EVEN',"no")')

	# silk bounding box is -XMAX,-YMAX, XMAX,YMAX (mils)
	define(`XMAX', `eval( (PADSEP/2 + PADLENGTH + 500*SILKW)/1000 + SILKSEP )')
	ifelse(EVEN,"yes",
		`define(`YMAX', `eval( ((NPADS/4)*PITCH - PITCH/2 + PADWIDTH/2 + 500*SILKW)/1000 + SILKSEP )')',
		`define(`YMAX', `eval( ((NPADS/4)*PITCH           + PADWIDTH/2 + 500*SILKW)/1000 + SILKSEP )')'
	)

	define(`REV', `$9')

	ifelse(REV,"reverse",
		`define(`CURPIN', NPADS)'
	,
		`define(`CURPIN', `1')'
	)	
Element(0x00 "$1" "$2" "$3" -20 -60 0 100 0x00)
(
        forloop(`i', 1, eval(NPADS / 2),
                `ifelse(EVEN,"yes",
		 `Pad(   -X1 eval( (-(NPADS/4)*PITCH - PITCH/2 + i*PITCH)/1000) 
			 -X2 eval( (-(NPADS/4)*PITCH - PITCH/2 + i*PITCH)/1000) 
			eval(PADWIDTH/1000) "CURPIN" 0x100)',
		 `Pad(   -X1 eval( (-(NPADS/4)*PITCH - PITCH   + i*PITCH)/1000) 
			 -X2 eval( (-(NPADS/4)*PITCH - PITCH   + i*PITCH)/1000) 
			eval(PADWIDTH/1000) "CURPIN" 0x100)')

		ifelse(REV,"reverse", 
			`define(`CURPIN', decr(CURPIN))',
			`define(`CURPIN', incr(CURPIN))'
			)
        ')
        forloop(`i', eval((NPADS / 2) + 1), NPADS,
                `ifelse(EVEN,"yes",
		 `Pad(   X1 eval( ((NPADS/4)*PITCH + PITCH/2 - (i-NPADS/2)*PITCH)/1000) 
			 X2 eval( ((NPADS/4)*PITCH + PITCH/2 - (i-NPADS/2)*PITCH)/1000) 
			eval(PADWIDTH/1000) "CURPIN" 0x100)',
		 `Pad(   X1 eval( ((NPADS/4)*PITCH + PITCH   - (i-NPADS/2)*PITCH)/1000) 
			 X2 eval( ((NPADS/4)*PITCH + PITCH   - (i-NPADS/2)*PITCH)/1000) 
			eval(PADWIDTH/1000) "CURPIN" 0x100)')
		ifelse(REV,"reverse", 
			`define(`CURPIN', decr(CURPIN))',
			`define(`CURPIN', incr(CURPIN))'
			)
        ')

	ElementLine(-XMAX -YMAX -XMAX  YMAX SILKW)
	ElementLine(-XMAX  YMAX  XMAX  YMAX SILKW)
	ElementLine( XMAX  YMAX  XMAX -YMAX SILKW)
	ElementLine(-XMAX -YMAX   -25 -YMAX SILKW)
	ElementLine( XMAX -YMAX    25 -YMAX SILKW)

	# punt on the arc on small parts as it can cover the pads
	ifelse(eval((PADSEP-PADLENGTH)/1000 > 50), 1, `ElementArc(0 -YMAX 25 25 0 180 SILKW)', )
	
	# Mark at the common centroid
        Mark(0 0)
)')

# dimensions are given in 1/100 mm.
# $5-$8 are pad width, length, pitch, and sep
define(`COMMON_SMT_DIL_MM',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',`$4',
	eval($5*100000/254),eval($6*100000/254),eval($7*100000/254),eval($8*100000/254), `$9')')

define(`PKG_US',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',`$4',30,70,50,200)')

