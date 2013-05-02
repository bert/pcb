	# silk screen width (mils)
	# silk screen bounding box
Element(0x00 "3-Pin SMT EMI Filter based on standard SMT sizes" "" "EMI0805" -46 72 0 100 0x00)
(
	# Pads which have the perpendicular pad dimension greater
	# than or equal to the parallel pad dimension 
 	Pad(-39 -4 
            -39  4 24 "1" 0x100)
 	Pad(39 -4 
            39  4 24 "3" 0x100)
	# Pads which have the perpendicular pad dimension greater
	# than or equal to the parallel pad dimension 
 	Pad(0 -25 
            0 25 24 "2" 0x100)
	# silk screen
	# ends
	ElementLine(-66 -52 -66 52 10)
	ElementLine(66 52 66 -52 10)
	# sides
ElementLine(-66 -52 66 -52 10)
	ElementLine(66 52 -66 52 10)
	# Mark the common centroid of the part
	Mark(0 0)
)
