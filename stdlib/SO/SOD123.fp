	# how much to grow the pads by for soldermask
	# clearance from planes
Element(0x00 "SMT diode (pin 1 is cathode)" "" "SOD123" 0 0 120 0 3 100 0x00)
(
	ElementLine(-110 -40 -110 40 20)
	    ElementLine(-110 40 -76 51 10)
	    ElementLine(-76 51 100 51 10)
	    ElementLine(100 51 100 -51 10)
	    ElementLine(100 -51 -76 -51 10)
	    ElementLine(-76 -51 -110 -40 10)
	Pad(-55 -6 
		-55 6
		69 20 75 "1" "1" 0x00000100)
	    Pad(55 -6 
		55 6
		69 20 75 "2" "2" 0x00000100)
)
