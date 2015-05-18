	# how much to grow the pads by for soldermask
	# clearance from planes
Element(0x00 "SMT diode (pin 1 is cathode)" "D?" "SOD123" 0 0 120 0 0 100 0x00)
(
	Pad(-55 -6 -55 6 69 20 75 "1" "1" 0x00000100)
	Pad( 55 -6  55 6 69 20 75 "2" "2" 0x00000100)

	ElementLine(-110 -40 -110  40 15)
#	ElementLine(-110  40  -76  51 10)
	ElementLine( -26  51   26  51 10)
#	ElementLine( 100  51  100 -51 10)
	ElementLine(  26 -51  -26 -51 10)
#	ElementLine( -76 -51 -110 -40 10)
)
