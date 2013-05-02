	# how much to grow the pads by for soldermask
	# clearance from planes
Element(0x00 "Surface mount electrolytic capacitor, number is dia in mm" "" "SME5" 0 0 206 0 3 100 0x00)
(
	ElementLine(-196 -145 -196 145 20)
	    ElementLine(-196 145 -135 186 10)
	    ElementLine(-135 186 186 186 10)
	    ElementLine(186 186 186 -186 10)
	    ElementLine(186 -186 -135 -186 10)
	    ElementLine(-135 -186 -196 -145 10)
	Pad(-83 -83 
		-83 83
		123 20 129 "1" "1" 0x00000100)
	    Pad(83 -83 
		83 83
		123 20 129 "2" "2" 0x00000100)
)
