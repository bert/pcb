	# how much to grow the pads by for soldermask
	# clearance from planes
Element(0x00 "SMT diode (pin 1 is cathode)" "" "SOD87" 0 0 124 0 3 100 0x00)
(
	ElementLine(-114 -57 -114 57 20)
	    ElementLine(-114 57 -84 73 10)
	    ElementLine(-84 73 104 73 10)
	    ElementLine(104 73 104 -73 10)
	    ElementLine(104 -73 -84 -73 10)
	    ElementLine(-84 -73 -114 -57 10)
	Pad(-58 -26 
		-58 26
		61 20 67 "1" "1" 0x00000100)
	    Pad(58 -26 
		58 26
		61 20 67 "2" "2" 0x00000100)
)
