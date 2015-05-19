	# how much to grow the pads by for soldermask
	# clearance from planes
Element(0x00 "SMT diode (pin 1 is cathode)" "D?" "SOD323" 0 0 -85 -110 0 100 0x00)
(
	ElementLine(-78 -36 -78  36 10)
    ElementLine(-78  36 -65  45 10)
    ElementLine(-65  45  73  45 10)
    ElementLine( 73  45  73 -45 10)
    ElementLine( 73 -45 -65 -45 10)
    ElementLine(-65 -45 -78 -36 10)

	Pad(-37 -10 -37 10 51 20 57 "1" "1" 0x00000100)
    Pad( 37 -10  37 10 51 20 57 "2" "2" 0x00000100)
)
