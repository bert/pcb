# 78 for SOT23
# 82 for SOT23
# 41 for SOT23
# 34 for SOT23, 24 for SOT25

Element(0x00 "SMT transistor, 3 pins" "" "SC70_3" 114 0 3 100 0x00)
(
	ElementLine(0 0 0 119 5)
	ElementLine(0 119 94 119 5)
	ElementLine(94 119 94 0 5)
	ElementLine(94 0 0 0 5)
	# 1st side, 1st pin
	Pad(21 91
	       21 97
			   29
			      "1" "1" 0x100)
	# 1st side, 2nd pin
	# 1st side, 3rd pin
	Pad(72 91
	    72 97
			   29
			      "2" "2" 0x100)
	# 2nd side, 3rd pin
	# 2nd side, 2nd pin
	Pad(47 21
	       47 27
			   29
			      "3" "3" 0x100)
	# 2nd side, 1st pin
	Mark(21 94)
)
