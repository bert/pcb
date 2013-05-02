	# how much to grow the pads by for soldermask
	# clearance from planes
Element(0x00 "Surface mount electrolytic capacitor, number is dia in mm" "" "SME6" 0 0 251 0 3 100 0x00)
(
	ElementLine(-241 -180 -241 180 20)
	    ElementLine(-241 180 -165 231 10)
	    ElementLine(-165 231 231 231 10)
	    ElementLine(231 231 231 -231 10)
	    ElementLine(231 -231 -165 -231 10)
	    ElementLine(-165 -231 -241 -180 10)
	Pad(-104 -104 
		-104 104
		153 20 159 "1" "1" 0x00000100)
	    Pad(104 -104 
		104 104
		153 20 159 "2" "2" 0x00000100)
)
