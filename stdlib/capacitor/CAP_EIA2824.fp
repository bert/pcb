	# how much to grow the pads by for soldermask
	# clearance from planes
Element(0x00 "Tantalum SMT capacitor (pin 1 is +)" "" "EIA2824" 0 0 132 0 3 100 0x00)
(
	ElementLine( -85 -180  85 -180 20)

	ElementLine( -135  175 -135 -155 10)
	ElementLine(  135  175  135 -155 10)

	ElementLine( -135  175  135  175 10)

	Pad(-79 -115 79 -115 90 20 100 "1" "1" 0x00000100)
    Pad(-79  115 79  115 90 20 100 "2" "2" 0x00000100)
)
