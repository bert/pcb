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
# 
#   RCS: $Id$
#
# misc packages
#

# -------------------------------------------------------------------
# the definition of a SD (ZIP) package
# based on 'old style format' by Olaf Kaluza (olaf@criseis.ruhr.de)
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: number of pins
#
define(`PKG_SD',
	`define(`MAXY', `eval(`$4' / 2 * 100 + 50)')
Element(0x00 "$1" "$2" "$3" 275 50 3 100 0x00)
(
	forloop(`i', 1, eval($4 / 2),
		`PIN(50,  eval(100*(i-1)+50), 50, 20, eval(2*i-1))
		PIN(150, eval(100*(i-1)+100), 50, 20, eval(2*i))
	')

	ElementLine(0 0 0 MAXY 20)
	ElementLine(0 MAXY 200 MAXY 20)
	ElementLine(200 MAXY 200 0 20)
	ElementLine(200 0 0 0 20)
	ElementLine(100 0 100 100 10)
	ElementLine(100 100 0 100 10)

	Mark(50 50)
)')

# -------------------------------------------------------------------
# the definition of a "MULTIWATT" package (I never actually saw one)
# based on 'old style format' by Olaf Kaluza (olaf@criseis.ruhr.de)
# $1: canonical name
# $2: name on PCB
# $3: value
define(`PKG_MULTIWATT15',
`Element(0x00 "$1" "$2" "$3" 150 100 3 100 0x00)
(
	forloop(`i', 1, 15,
		`PIN(ifelse(eval(i % 2 == 0), 1, 250, 50), eval(i*50), 60, 20, i)
	')
	ElementLine(0 0 400 0 20)
	ElementLine(400 0 400 800 20)
	ElementLine(400 800 0 800 20)
	ElementLine(0 800 0 0 20)
	Mark(50 50)
)')

# -------------------------------------------------------------------
# the definition of a resistor (0.25W) package
# $1: canonical name
# $2: name on PCB
# $3: value
define(`PKG_R025',
`Element(0x00 "$1" "$2" "$3" 120 30 0 100 0x00)
(
	PIN(0, 50, 50, 20, 1)
	PIN(400, 50, 50, 20, 2)
	ElementLine(100 0 300 0 20)
	ElementLine(300 0 300 100 20)
	ElementLine(300 100 100 100 20)
	ElementLine(100 100 100 0 20)
	ElementLine(0 50 100 50 20)
	ElementLine(300 50 400 50 20)
	Mark(0 50)
)')

# -------------------------------------------------------------------
# the definition of a SIL package without a common pin
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: number of pins
define(`PKG_SIL',
	`define(`MAXY', `eval(`$4' * 100 -50)')
Element(0x00 "$1" "$2" "$3" 160 10 3 100 0x00)
(
	forloop(`i', 1, $4,
		`PIN(50, eval(i * 100 -50), 50, 20, i)
	')
	ElementLine(  0 50   0 MAXY 20)
	ElementLine(100 50 100 MAXY 20)
	ElementArc(50  50 50 50 180 180 20)
	ElementArc(50 MAXY 50 50   0 180 20)
	forloop(`i', 1, eval($4 /2 -1),
		`ElementLine(0 eval(i * 200) 100 eval(i * 200) 10)
	')
	Mark(50 50)
)')

# -------------------------------------------------------------------
# the definition of a SIL package with a common pin
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: number of pins
define(`PKG_CSIL',
	`define(`MAXY', `eval(`$4' * 100 -50)')
Element(0x00 "$1" "$2" "$3" 160 10 3 100 0x00)
(
	forloop(`i', 1, $4,
		`PIN(50, eval(i * 100 -50), 50, 20, i)
	')
	ElementLine(  0 50   0 MAXY 20)
	ElementLine(100 50 100 MAXY 20)
	ElementLine(  0 100 100 100 10)
	ElementArc(50  50 50 50 180 180 20)
	ElementArc(50 MAXY 50 50   0 180 20)
	Mark(50 50)
)')

# -------------------------------------------------------------------
# a QFP-132 pin flat pack
# 
# $1: canonical name
# $2: name on PCB
# $3: value
#
# based on mail by Volker Bosch (bosch@iema.e-technik.uni-stuttgart.de)
define(`PKG_QFP132',
`Element(0x00 "$1" "$2" "$3" 250 200 0 150 0x00)
(
	forloop(`i', 1, 17,
		`define(`XPOS', eval(625 -i*25))'
		`PAD(XPOS, 40, XPOS, 90, 15, i)
	')
	forloop(`i', 1, 16,
		`define(`XPOS', eval(1025 -i*25))'
		`PAD(XPOS, 40, XPOS, 90, 15, eval(i+116))
	')
	forloop(`i', 1, 33,
		`define(`YPOS', eval(175 +i*25))'
		`PAD(30, YPOS, 80, YPOS, 15, i)
	')
	forloop(`i', 1, 33,
		`define(`XPOS', eval(175 +i*25))'
		`PAD(XPOS, 1160, XPOS, 1110, 15, eval(i+50))
	')
	forloop(`i', 1, 33,
		`define(`YPOS', eval(1025 -i*25))'
		`PAD(1120, YPOS, 1170, YPOS, 15, i)
	')

	# Markierung pin 1
	ElementArc(600 150 10 10 0 360 5)

	# Nase links oben (PIN 17/18)
	ElementLine( 125 175  75 150 20)
	ElementLine(  75 150  75 125 20)
	ElementLine(  75 125 125  75 20)
	ElementLine( 125  75 150  75 20)
	ElementLine( 150  75 175 125 20)

	# Verbindungsline zur Ecke rechts oben
	ElementLine( 175  125 1025 125 20)

	# Nase rechts oben (PIN 116/117)
	ElementLine(1025 125 1050  75 20)
	ElementLine(1050  75 1075  75 20)
	ElementLine(1075  75 1125 125 20) 
	ElementLine(1125 125 1125 150 20)
	ElementLine(1125 150 1075 175 20)

	# Verbindungsline zur Ecke rechts unten
	ElementLine(1075 175  1075 1025 20)

	# Nase rechts unten (PIN 83/84)
	ElementLine(1075 1025 1125 1050 20)
	ElementLine(1125 1050 1125 1075 20)
	ElementLine(1125 1075 1075 1125 20)
	ElementLine(1075 1125 1050 1125 20)
	ElementLine(1050 1125 1025 1075 20)

	# Verbindungsline zur Ecke links unten 
	ElementLine(1025 1075  175 1075 20)

	# Nase links unten (PIN 50/51)
	ElementLine( 175 1075 150 1125 20)
	ElementLine( 150 1125 125 1125 20)
	ElementLine( 125 1125  75 1075 20)
	ElementLine(  75 1075  75 1050 20)
	ElementLine(  75 1050 125 1025 20)

	# Verbindungsline zur Ecke links oben
	ElementLine( 125 1025 125 175 20)

	# Markierung so anordnen, dass Pinanchse im 25-MIL-Raster zu liegen kommt
	Mark(200 200)
)')

# -------------------------------------------------------------------
# LED
# Pin 1 is -, 2 is +
# 
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: diameter
#
# based on mail by Volker Bosch (bosch@iema.e-technik.uni-stuttgart.de)
define(`PKG_LED',
	`define(`RADIUS1', `eval(`$4' /2)')
	define(`RADIUS2', `eval(`$4' /2 +20)')
Element(0x00 "$1" "$2" "$3" 100 70 0 100 0x00)
(
	PIN(150, 150, 60, 20, 1)
	PIN(150, 250, 60, 20, 2)
	ElementArc(150 200 RADIUS1 RADIUS1 0 360 15)
	ElementArc(150 200 RADIUS2 RADIUS2 0 360 10)
	Mark(150 150)
)')

# -------------------------------------------------------------------
# diodes
# Pin 1 is K, 2 is A
# 
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: distance in mil
#
define(`PKG_DIODE_LAY',
	`define(`X1', `eval(`$4' /3)')
	define(`X2', `eval(`$4' -X1)')
	define(`Y', `eval(`$4' /2)')
	define(`DELTA', `eval(X1 /2)')
	define(`PINSIZE', `ifelse(eval($4 >= 500), 1, 80, 50)')
Element(0x00 "$1" "$2" "$3" eval(X2+20) eval(Y-DELTA) 0 100 0x00)
(
	PIN(0, Y, PINSIZE, 20, 1)
	PIN($4, Y, PINSIZE, 20, 2)

	ElementLine(0 Y X1 Y 10)
	ElementLine(X2 Y $4 Y 10)

	ElementLine(X1 Y X2 eval(Y-DELTA) 10)
	ElementLine(X2 eval(Y-DELTA) X2 eval(Y+DELTA) 10)
	ElementLine(X2 eval(Y+DELTA) X1 Y 10)
	ElementLine(X1 eval(Y-DELTA) X1 eval(Y+DELTA) 10)
	Mark(0 Y)
)')

# -------------------------------------------------------------------
# the definition of a general axial package
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: distance in mil
define(`PKG_AXIAL_LAY',
	`define(`X1', `eval(`$4' /4)')
	define(`X2', `eval(`$4' -X1)')
	define(`Y1', `eval(X1 /3)')
	define(`Y2', `eval(Y1 *2)')
	define(`PINSIZE', `ifelse(eval($4 >= 600), 1, 80, 50)')
Element(0x00 "$1" "$2" "$3" eval(X2+20) eval(Y2+20) 0 100 0x00)
(
	PIN(0, Y1, PINSIZE, 20, 1)
	PIN($4, Y1, PINSIZE, 20, 2)

	ElementLine(0 Y1 X1 Y1 10)
	ElementLine(X2 Y1 $4 Y1 10)

	ElementLine(X1 0 X2 0 10)
	ElementLine(X2 0 X2 Y2 10)
	ElementLine(X2 Y2 X1 Y2 10)
	ElementLine(X1 Y2 X1 0 10)

#	ElementArc(X1 Y 50 50 270 180 10)
#	ElementArc(X2 Y 50 50 90 180 10)

	Mark(X1 Y1)
)')

# -------------------------------------------------------------------
# an crystal package
# Pin 1 is K, 2 is A
# 
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: package width in MIL
#
define(`PKG_CRYSTAL',
	`define(`X1', `100')
	define(`X2', `$4')
	define(`Y', `100')
	define(`Y1', `eval(Y -50)')
	define(`Y2', `eval(Y +50)')
Element(0x00 "$1" "`$2'" "$3" X1 eval(Y2+20) 0 100 0x00)
(
	PIN(X1, Y, 60, 20, 1)
	PIN(X2, Y, 60, 20, 2)
	ElementLine(X1 Y1 X2 Y1 10)
	ElementLine(X1 Y2 X2 Y2 10)
	ElementArc(X1 Y 50 50 270 180 10)
	ElementArc(X2 Y 50 50 90 180 10)
	Mark (X1 Y)
)')

# -------------------------------------------------------------------
# a can oscillator package
#
# $1: canonical name
# $2: name on PCB
# $3: value
#
define(`PKG_OSC',
`Element(0x00 "$1" "$2" "$3" 270 300 3 100 0x00)
(
	Pin(100 100 50 20 "NC" 0x01)
	Pin(100 700 50 20 "GND" 0x01)
	Pin(400 700 50 20 "CLK" 0x01)
	Pin(400 100 50 20 "VCC" 0x01)

	ElementLine(5 5 400 5 10)
	ElementArc(400 100 95 95 180 90 10)
	ElementLine(495 100 495 700 10)
	ElementArc(400 700 95 95 90 90 10)
	ElementLine(400 795 100 795 10)
	ElementArc(100 700 95 95 0 90 10)
	ElementLine(5 700 5 5 10)

	ElementLine(100 60 400 60 10)
	ElementArc(400 100 40 40 180 90 10)
	ElementLine(440 100 440 700 10)
	ElementArc(400 700 40 40 90 90 10)
	ElementLine(400 740 100 740 10)
	ElementArc(100 700 40 40 0 90 10)
	ElementLine(60 700 60 100 10)
	ElementArc(100 100 40 40 270 90 10)

	Mark(100 100)
)')

# -------------------------------------------------------------------
# 8 bit ISA Slot card
#
# Volker Bosch (bosch@iema.e-technik.uni-stuttgart.de), 12/95
#
# $1: canonical name
# $2: name on PCB
# $3: value
#
define(`PKG_ISA8',
`Element(0x00 "$1" "$2" "$3" 4000 0 0 100 0x00)
(
    # Pins, entspr. Anschl. auf Loetseite (b31..b1)
	Pin( 200 3850 60 20 "Gnd (b31)" 0x01)
	Pin( 300 3850 60 20 "Osc" 0x01)
	Pin( 400 3850 60 20 "+5V" 0x01)
	Pin( 500 3850 60 20 "BALE" 0x01)
	Pin( 600 3850 60 20 "TC" 0x01)
	Pin( 700 3850 60 20 "/DAck2" 0x01)
	Pin( 800 3850 60 20 "IRq3" 0x01)
	Pin( 900 3850 60 20 "IRq4" 0x01)
	Pin(1000 3850 60 20 "IRq5" 0x01)
	Pin(1100 3850 60 20 "IRq6" 0x01)
	Pin(1200 3850 60 20 "IRq7" 0x01)
	Pin(1300 3850 60 20 "Clock" 0x01)
	Pin(1400 3850 60 20 "/Ref" 0x01)
	Pin(1500 3850 60 20 "DRq1" 0x01)
	Pin(1600 3850 60 20 "/DAck1" 0x01)
	Pin(1700 3850 60 20 "DRq3" 0x01)
	Pin(1800 3850 60 20 "/DAck3" 0x01)
	Pin(1900 3850 60 20 "/IOR" 0x01)
	Pin(2000 3850 60 20 "/IOW" 0x01)
	Pin(2100 3850 60 20 "/SMEMR" 0x01)
	Pin(2200 3850 60 20 "/SMEMW" 0x01)
	Pin(2300 3850 60 20 "Gnd" 0x01)
	Pin(2400 3850 60 20 "+12V" 0x01)
	Pin(2500 3850 60 20 "/0WS" 0x01)
	Pin(2600 3850 60 20 "-12V" 0x01)
	Pin(2700 3850 60 20 "DRq2" 0x01)
	Pin(2800 3850 60 20 "-5V" 0x01)
	Pin(2900 3850 60 20 "IEQ2" 0x01)
	Pin(3000 3850 60 20 "+5V" 0x01)
	Pin(3100 3850 60 20 "ResDrv" 0x01)
	Pin(3200 3850 60 20 "Gnd (b1)" 0x01)

      # Pins, entspr. Anschl. auf Bestueckseite
	Pin( 200 3950 60 20 "SA0 (a31)" 0x01)
	Pin( 300 3950 60 20 "SA1" 0x01)
	Pin( 400 3950 60 20 "SA2" 0x01)
	Pin( 500 3950 60 20 "SA3" 0x01)
	Pin( 600 3950 60 20 "SA4" 0x01)
	Pin( 700 3950 60 20 "SA5" 0x01)
	Pin( 800 3950 60 20 "SA6" 0x01)
	Pin( 900 3950 60 20 "SA7" 0x01)
	Pin(1000 3950 60 20 "SA8" 0x01)
	Pin(1100 3950 60 20 "SA9" 0x01)
	Pin(1200 3950 60 20 "SA10" 0x01)
	Pin(1300 3950 60 20 "SA11" 0x01)
	Pin(1400 3950 60 20 "SA12" 0x01)
	Pin(1500 3950 60 20 "SA13" 0x01)
	Pin(1600 3950 60 20 "SA14" 0x01)
	Pin(1700 3950 60 20 "SA15" 0x01)
	Pin(1800 3950 60 20 "SA16" 0x01)
	Pin(1900 3950 60 20 "SA17" 0x01)
	Pin(2000 3950 60 20 "SA18" 0x01)
	Pin(2100 3950 60 20 "SA19" 0x01)
	Pin(2200 3950 60 20 "AEN" 0x01)
	Pin(2300 3950 60 20 "IOChRdy" 0x01)
	Pin(2400 3950 60 20 "SD0" 0x01)
	Pin(2500 3950 60 20 "SD1" 0x01)
	Pin(2600 3950 60 20 "SD2" 0x01)
	Pin(2700 3950 60 20 "SD3" 0x01)
	Pin(2800 3950 60 20 "SD4" 0x01)
	Pin(2900 3950 60 20 "SD5" 0x01)
	Pin(3000 3950 60 20 "SD6" 0x01)
	Pin(3100 3950 60 20 "SD7" 0x01)
	Pin(3200 3950 60 20 "/IOChCk (a1)" 0x01)

      # Umrahmung
	ElementLine(   0  100  100  100 2)
	ElementLine( 100    0  100  100 2)
	ElementLine( 100  100 4250  100 5)
	ElementLine(4250  100 4250 3810 5)
	ElementLine(4250 3810 3620 3810 5)
	ElementLine(3620 3810 3620 3510 5)
	ElementLine(3620 3510 3300 3510 5)
	ElementLine(3300 3510 3300 3810 5)
	ElementLine(3300 3810  100 3810 5)
	ElementLine( 100 3810  100 3510 5)
	ElementLine( 100 3510    0 3510 2)

      # Markierung == Pin B1
	Mark(3200 3850)
)')

# -------------------------------------------------------------------
# an ovenized-oscillator package
#
# $1: canonical name
# $2: name on PCB
# $3: value
# Text(620 320 0 100 "GROUND" 0x0001)
# Text(620 520 0 100 "VECTRON LABORATORIES, INC." 0x0001)
# Text(620 720 0 100 "CO711 SERIES OSC" 0x0001)
# Text(620 920 0 100 "711-04-006" 0x0001)
# Text(620 1680 0 100 "+12 -> 15 VOLTS DC" 0x0001) 
define(`PKG_OVEN_OSC',
`Element(0x00 "$1" "$2" "$3" 580 280 0 100 0x00)
(
	Pin(320 320 150 35 "NC" 0x01)
	Pin(320 1000 150 35 "VCC" 0x01)
	Pin(320 1680 150 35 "CLK" 0x01)
	Pin(1680 320 150 35 "+12V" 0x01)
	Pin(1680 1680 150 35 "GND" 0x01)

	ElementLine(2020 1125 2020 875 10)
	ElementLine(2100 875 2020 875 10)
	ElementLine(2100 975 2100 875 10)
	ElementLine(2075 975 2100 975 10)
	ElementLine(2075 1025 2075 975 10)
	ElementLine(2100 1025 2075 1025 10)
	ElementLine(2100 1125 2100 1025 10)
	ElementLine(2100 1125 2020 1125 10)

	ElementLine(2000 320 2000 1680 10)
	ElementLine(320 0 1680 0 10)
	ElementLine(0 1680 0 320 10)
	ElementLine(1680 2000 320 2000 10)

	ElementArc(320 320 320 320 270 90 10)
	ElementArc(320 1680 320 320 0 90 10)
	ElementArc(1680 320 320 320 180 90 10)
	ElementArc(1680 1680 320 320 90 90 10)

	Mark(320 320)
)')

# a radial capacitor package
#
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: package width in MIL
#
define(`PKG_RADIAL_CAN',
	`define(`X1', `eval(`$4' /4)')
	define(`X2', `eval(`$4' -X1)')
	define(`X3', `eval(`$4' /2)')
	define(`Y', `eval(`$4' /2)')
	define(`R', `eval(`$4' /2)')
	define(`PINSIZE', `ifelse(eval($4 >= 600), 1, 80, 50)')
Element(0x00 "$1" "$2" "$3" eval(`$4') 0 0 100 0x00)
(
	PIN(X1, Y, 60, 20, 1)
	PIN(X2, Y, 60, 20, 2)
	ElementArc(X3 Y R R 0 360 10)
	Mark (X1 Y)
)')

# a core surface mount package
# 12/99 Larry Doolittle <LRDoolittle@lbl.gov>
#
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: device length in MIL
# $5: device width in MIL
#
# Note that the pad width, length, and separation are derived from
# the given device dimensions.  I <LRDoolittle@lbl.gov> don't
# claim to have followed any industry standards here, these sizes
# are based on someone else's measurements:
#  0603 is 30x40 mil pads on 60 mil pad ctr
#  0805 is 44x60 mil pads on 80 mil pad ctr
# on a PC motherboard.  If you know the "real" values, please educate
# me and/or adjust this code.
#
define(`PKG_SMD_BASE',
	`define(`T', `eval(`$4'/4+`$5'/2)')
	define(`W', `eval(T/2+15)')
	define(`X1', `0')
	define(`X2',  ``$4'')
	define(`Y',  `0')
	define(`Y1',  `eval(Y-(`$5'-T)/2-5)')
	define(`Y2',  `eval(Y+(`$5'-T)/2+5)')
	# Silkscreen box coordinates
	define(`X1L', `eval(X1-W)')
	define(`X2L', `eval(X2+W)')
	define(`Y1L', `eval(Y1-W)')
	define(`Y2L', `eval(Y2+W)')
Element(0x00 "$1" "$2" "$3" eval(10+T/2) eval(Y2L+15) 0 100 0x00)
(
	# PAD(X1, Y1, X1, Y2, T, 1)
	# PAD(X2, Y1, X2, Y2, T, 2)
	# Use Pad instead of PAD so both pads come out square
	Pad(X1 Y1 X1 Y2 T "1" 0x100)
	Pad(X2 Y1 X2 Y2 T "2" 0x100)
	ElementLine(X1L Y1L X1L Y2L 8)
	ElementLine(X1L Y2L X2L Y2L 8)
	ElementLine(X2L Y2L X2L Y1L 8)
	ElementLine(X2L Y1L X1L Y1L 8)
	SMD_OUTLINE_EXTRA
)')

define(`PKG_SMD_SIMPLE',
	`define(`SMD_OUTLINE_EXTRA', `')
	PKG_SMD_BASE(`$1', `$2', `$3', `$4', `$5')
')

define(`PKG_SMD_DIODE',
	`define(`SMD_OUTLINE_EXTRA',
		`
		define(`XBAR', `eval(X1L+10)')
		ElementLine( XBAR Y1L XBAR Y2L 8 )
	')
	PKG_SMD_BASE(`$1', `$2', `$3', `$4', `$5')
')

define(`PKG_SMD_POLAR',
	`define(`SMD_OUTLINE_EXTRA',
		`
		# crude plus sign
		# ElementLine(      X1     eval(Y2L+20)       X1    eval(Y2L+70) 8)
		# ElementLine( eval(X1-25) eval(Y2L+45) eval(X1+25) eval(Y2L+45) 8)
		define(`XBAR', `eval(X1L+10)')
		ElementLine( XBAR Y1L XBAR Y2L 8 )

	')
	PKG_SMD_BASE(`$1', `$2', `$3', `$4', `$5')
')

# a smd chip package, capacitor or resistor
#
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: package designator length in MIL *100 + width in MIL /10
#     ie 402 == 40 X 20; 603 == 60 X 30 
#
define(`PKG_SMD_CHIP',
	`define(`X1', `eval(eval(`$4' /100)*10)')
	define(`Y1', `eval(eval(`$4'- X1 *10) *10)')
# line radius (LR) depicts offset to pads lines and pad "band width"
	define(`LR', `ifelse(eval(Y1 < 40), 1, eval(Y1/4), 10)')
Element(0x00 "$1" "$2" "$3" 0 0 0 25 0x00)
(
        PAD(LR LR LR eval(Y1-LR) eval(LR*2) )
        PAD(eval(X1-LR) LR eval(X1-LR) eval(Y1-LR) eval(LR*2) )

        ElementLine( 0  0 X1  0 5)
        ElementLine(X1  0 X1 Y1 5)
        ElementLine(X1 Y1  0 Y1 5)
        ElementLine( 0 Y1  0  0 5)

        Mark(eval(X1/2) eval(Y1/2))
)')

