Element(0x00 "SMT transistor, 4 pins" "" "SOT89" 203 0 3 100 0x00)
(
	ElementLine(0 0 0 207 10)
	ElementLine(0 207 183 207 10)
	ElementLine(183 207 183 0 10)
	ElementLine(183 0 0 0 10)
	# 1st pin on pin side
	Pad(30 152
	    30 176
			   37
			      "1" "1" 0x100)
	Pad(91 152
	       91 176
			   37
			      "2" "2" 0x100)
	# last pin on pin side
	Pad(152 152
	    152 176
			   37
			      "3" "3" 0x100)
	# extra wide pin on opposite side
	Pad(121 42
	    61 42
			   61 "4" "4" 0x100)
	Mark(30 164)
)
