	# how much to grow the pads by for soldermask
	# clearance from planes
Element(0x00 "SMT diode (pin 1 is cathode)" "" "DO214" 0 0 221 0 3 100 0x00)
(
	ElementLine(-211 -89 -211 89 20)
	    ElementLine(-211 89 -141 114 10)
	    ElementLine(-141 114 201 114 10)
	    ElementLine(201 114 201 -114 10)
	    ElementLine(201 -114 -141 -114 10)
	    ElementLine(-141 -114 -211 -89 10)
	Pad(-106 -19 
		-106 19
		140 20 146 "1" "1" 0x00000100)
	    Pad(106 -19 
		106 19
		140 20 146 "2" "2" 0x00000100)
)
