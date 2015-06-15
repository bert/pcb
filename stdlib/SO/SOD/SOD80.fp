	# how much to grow the pads by for soldermask
	# clearance from planes
Element(0x00 "SMT diode (pin 1 is cathode)" "" "SOD80" 0 0 116 0 3 100 0x00)
(
	ElementLine(-106 -43 -106 43 20)
	    ElementLine(-106 43 -80 55 10)
	    ElementLine(-80 55 96 55 10)
	    ElementLine(96 55 96 -55 10)
	    ElementLine(96 -55 -80 -55 10)
	    ElementLine(-80 -55 -106 -43 10)
	Pad(-58 -16 
		-58 16
		53 20 59 "1" "1" 0x00000100)
	    Pad(58 -16 
		58 16
		53 20 59 "2" "2" 0x00000100)
)
