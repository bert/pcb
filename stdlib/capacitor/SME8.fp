	# how much to grow the pads by for soldermask
	# clearance from planes
Element(0x00 "Surface mount electrolytic capacitor, number is dia in mm" "" "SME8" 0 0 317 0 3 100 0x00)
(
	ElementLine(-307 -231 -307 231 20)
	    ElementLine(-307 231 -208 297 10)
	    ElementLine(-208 297 297 297 10)
	    ElementLine(297 297 297 -297 10)
	    ElementLine(297 -297 -208 -297 10)
	    ElementLine(-208 -297 -307 -231 10)
	Pad(-132 -132 
		-132 132
		198 20 204 "1" "1" 0x00000100)
	    Pad(132 -132 
		132 132
		198 20 204 "2" "2" 0x00000100)
)
