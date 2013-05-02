	# how much to grow the pads by for soldermask
	# clearance from planes
Element(0x00 "Tantalum SMT capacitor (pin 1 is +)" "" "EIA6032" 0 0 188 0 3 100 0x00)
(
	ElementLine(-178 -87 -178 87 20)
	    ElementLine(-178 87 -130 112 10)
	    ElementLine(-130 112 168 112 10)
	    ElementLine(168 112 168 -112 10)
	    ElementLine(168 -112 -130 -112 10)
	    ElementLine(-130 -112 -178 -87 10)
	Pad(-94 -39 
		-94 39
		97 20 103 "1" "1" 0x00000100)
	    Pad(94 -39 
		94 39
		97 20 103 "2" "2" 0x00000100)
)
