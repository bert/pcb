	# how much to grow the pads by for soldermask
	# clearance from planes
Element(0x00 "SMT diode (pin 1 is cathode)" "" "SOD110" 0 0 83 0 3 100 0x00)
(
	ElementLine(-73 -38 -73 38 20)
	    ElementLine(-73 38 -50 49 10)
	    ElementLine(-50 49 63 49 10)
	    ElementLine(63 49 63 -49 10)
	    ElementLine(63 -49 -50 -49 10)
	    ElementLine(-50 -49 -73 -38 10)
	Pad(-29 -15 
		-29 15
		46 20 52 "1" "1" 0x00000100)
	    Pad(29 -15 
		29 15
		46 20 52 "2" "2" 0x00000100)
)
