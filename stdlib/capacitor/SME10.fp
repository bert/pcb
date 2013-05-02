	# how much to grow the pads by for soldermask
	# clearance from planes
Element(0x00 "Surface mount electrolytic capacitor, number is dia in mm" "" "SME10" 0 0 386 0 3 100 0x00)
(
	ElementLine(-376 -285 -376 285 20)
	    ElementLine(-376 285 -255 366 10)
	    ElementLine(-255 366 366 366 10)
	    ElementLine(366 366 366 -366 10)
	    ElementLine(366 -366 -255 -366 10)
	    ElementLine(-255 -366 -376 -285 10)
	Pad(-164 -164 
		-164 164
		243 20 249 "1" "1" 0x00000100)
	    Pad(164 -164 
		164 164
		243 20 249 "2" "2" 0x00000100)
)
