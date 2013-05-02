	# how much to grow the pads by for soldermask
	# clearance from planes
Element(0x00 "Standard SMT resistor, capacitor etc" "" "2706" 0 0 179 0 3 100 0x00)
(
	ElementLine(-159 -54 -159 54 10)
	    ElementLine(-159 54 159 54 10)
	    ElementLine(159 54 159 -54 10)
	    ElementLine(159 -54 -159 -54 10)
	Pad(-108 -3 
		-108 3
		78 20 84 "1" "1" 0x00000100)
	    Pad(108 -3 
		108 3
		78 20 84 "2" "2" 0x00000100)
)
