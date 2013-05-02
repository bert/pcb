	# how much to grow the pads by for soldermask
	# clearance from planes
Element(0x00 "Tantalum SMT capacitor (pin 1 is +)" "" "TANT_A" 0 0 106 0 3 100 0x00)
(
	ElementLine(-96 -43 -96 43 20)
	    ElementLine(-96 43 -72 55 10)
	    ElementLine(-72 55 86 55 10)
	    ElementLine(86 55 86 -55 10)
	    ElementLine(86 -55 -72 -55 10)
	    ElementLine(-72 -55 -96 -43 10)
	Pad(-50 -18 
		-50 18
		49 20 55 "1" "1" 0x00000100)
	    Pad(50 -18 
		50 18
		49 20 55 "2" "2" 0x00000100)
)
