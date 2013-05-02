	# silk screen width (mils)
	# silk screen bounding box
Element(0x00 "3-Pin SMT EMI Filter based on standard SMT sizes" "" "EMI0603" -38 58 0 100 0x00)
(
	# Pads which have the perpendicular pad dimension less
	# than or equal to the parallel pad dimension 	
	Pad(-31 0 
            -31 0 24 "1" 0x100)
	Pad(31 0 
            31 0 24 "3" 0x100)
	# Pads which have the perpendicular pad dimension greater
	# than or equal to the parallel pad dimension 
 	Pad(0 -15 
            0 15 16 "2" 0x100)
	# silk screen
	# ends
	ElementLine(-58 -38 -58 38 10)
	ElementLine(58 38 58 -38 10)
	# sides
ElementLine(-58 -38 58 -38 10)
	ElementLine(58 38 -58 38 10)
	# Mark the common centroid of the part
	Mark(0 0)
)
