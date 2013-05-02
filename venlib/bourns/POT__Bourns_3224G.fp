	# pin 1,3 width, length (1/100 mm)
	# pin 2 width, length (1/100 mm)
	# spacing from pad center to pad center between 2 and 1,3 (1/100 mm)
	# spacing from pad center to pad center between 1 and 3 (1/100 mm)
	# package width (1/100 mm)
	# silkscreen width (mils)
	# how much space to leave around the part before the
	# silk screen (mils)
	# lower right corner for silk screen (mil)
	# refdes text size (mil)
	# x,y coordinates for refdes label (mil)
Element(0x00 "Bourns 3224 Series SMT Trim Pot" "" "BOURNS_3224G" -140 -167 0 100 0x00)
(
# pin 1
# Pads which have the perpendicular pad dimension less
	# than or equal to the parallel pad dimension 	
	Pad(102 45 
            102 45 50 "1" 0x100)
# pin 2
# Pads which have the Y (width) pad dimension greater
	# than or equal to the X (length) pad dimension 
	Pad(-102 -14
            -102 14 50 "2" 0x100)
# pin 3
# Pads which have the perpendicular pad dimension less
	# than or equal to the parallel pad dimension 	
	Pad(102 -45 
            102 -45 50 "3" 0x100)
# Silk screen around package
ElementLine( 140  107  140 -107 10)
ElementLine( 140 -107 -140 -107 10)
ElementLine(-140 -107 -140  107 10)
ElementLine(-140  107  140  107 10)
# Mark at the center of the part
Mark(0 0)
)
