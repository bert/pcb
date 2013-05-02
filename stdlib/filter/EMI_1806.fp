	# silk screen width (mils)
	# silk screen bounding box
Element(0x00 "3-Pin SMT EMI Filter based on standard SMT sizes" "" "EMI1806" -102 86 0 100 0x00)
(
	# Pads which have the perpendicular pad dimension less
	# than or equal to the parallel pad dimension 	
	Pad(-88 0 
            -88 0 39 "1" 0x100)
	Pad(88 0 
            88 0 39 "3" 0x100)
	# Pads which have the perpendicular pad dimension greater
	# than or equal to the parallel pad dimension 
 	Pad(0 -21 
            0 21 59 "2" 0x100)
	# silk screen
	# ends
	ElementLine(-122 -66 -122 66 10)
	ElementLine(122 66 122 -66 10)
	# sides
ElementLine(-122 -66 122 -66 10)
	ElementLine(122 66 -122 66 10)
	# Mark the common centroid of the part
	Mark(0 0)
)
