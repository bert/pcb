	# silk screen width (mils)
	# silk screen bounding box
Element(0x00 "3-Pin SMT EMI Filter based on standard SMT sizes" "" "EMI1206" -72 74 0 100 0x00)
(
	# Pads which have the perpendicular pad dimension greater
	# than or equal to the parallel pad dimension 
 	Pad(-63 -2 
            -63  2 28 "1" 0x100)
 	Pad(63 -2 
            63  2 28 "3" 0x100)
	# Pads which have the perpendicular pad dimension greater
	# than or equal to the parallel pad dimension 
 	Pad(0 -20 
            0 20 39 "2" 0x100)
	# silk screen
	# ends
	ElementLine(-92 -54 -92 54 10)
	ElementLine(92 54 92 -54 10)
	# sides
ElementLine(-92 -54 92 -54 10)
	ElementLine(92 54 -92 54 10)
	# Mark the common centroid of the part
	Mark(0 0)
)
