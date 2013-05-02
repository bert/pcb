	# how much to grow the pads by for soldermask
	# clearance from planes
Element(0x00 "SMT diode (pin 1 is cathode)" "" "SOD106A" 0 0 166 0 3 100 0x00)
(
	ElementLine(-156 -68 -156 68 20)
	    ElementLine(-156 68 -105 87 10)
	    ElementLine(-105 87 146 87 10)
	    ElementLine(146 87 146 -87 10)
	    ElementLine(146 -87 -105 -87 10)
	    ElementLine(-105 -87 -156 -68 10)
	Pad(-76 -17 
		-76 17
		102 20 108 "1" "1" 0x00000100)
	    Pad(76 -17 
		76 17
		102 20 108 "2" "2" 0x00000100)
)
