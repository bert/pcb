	# how much to grow the pads by for soldermask
	# clearance from planes
Element(0x00 "Tantalum SMT capacitor (pin 1 is +)" "" "EIA3528" 0 0 132 0 3 100 0x00)
(
	ElementLine(-122 -77 -122 77 20)
	    ElementLine(-122 77 -87 99 10)
	    ElementLine(-87 99 112 99 10)
	    ElementLine(112 99 112 -99 10)
	    ElementLine(112 -99 -87 -99 10)
	    ElementLine(-87 -99 -122 -77 10)
	Pad(-55 -41 
		-55 41
		71 20 77 "1" "1" 0x00000100)
	    Pad(55 -41 
		55 41
		71 20 77 "2" "2" 0x00000100)
)
