	# how much to grow the pads by for soldermask
	# clearance from planes
Element(0x00 "Surface mount electrolytic capacitor, number is dia in mm" "" "SME4" 0 0 170 0 3 100 0x00)
(
	ElementLine(-160 -117 -160 117 20)
	    ElementLine(-160 117 -111 150 10)
	    ElementLine(-111 150 150 150 10)
	    ElementLine(150 150 150 -150 10)
	    ElementLine(150 -150 -111 -150 10)
	    ElementLine(-111 -150 -160 -117 10)
	Pad(-68 -68 
		-68 68
		99 20 105 "1" "1" 0x00000100)
	    Pad(68 -68 
		68 68
		99 20 105 "2" "2" 0x00000100)
)
