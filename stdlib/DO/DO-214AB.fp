	# how much to grow the pads by for soldermask
	# clearance from planes
Element(0x00 "SMT diode (pin 1 is cathode)" "D?" "DO214AB" 0 0 -220 -35 3 150 0x00)
(

	Pad(-109 -20 
		-109 20
		145 20 151 "1" "1" 0x00000100)
	    Pad(109 -20 
		109 20
		145 20 151 "2" "2" 0x00000100)

	ElementLine(-217  -92 -217   92 20)

	ElementLine(-217   92 -185  118 10)
	ElementLine(-185  118  207  118 10)
	ElementLine( 207  118  207 -118 10)
	ElementLine( 207 -118 -185 -118 10)
	ElementLine(-185 -118 -217 -92 10)

)
