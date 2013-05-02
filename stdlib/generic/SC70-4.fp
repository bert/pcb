	                             # 78 for SOT23
	                             # 82 for SOT23
	      # 41 for SOT23
	               # 34 for SOT23, 24 for SOT25
Element(0x00 "SMT transistor, 4 pins" "" "SC70_4" 114 0 3 100 0x00)
(
	ElementLine(0 0 0 119 10)
	ElementLine(0 119 94 119 10)
	ElementLine(94 119 94 0 10)
	ElementLine(94 0 0 0 10)
	# 1st side, 1st pin
	# extra width
	   Pad(24 94
	       27 94
			   29
			      "1" "1" 0x100)
	# 1st side, 2nd pin
	# 1st side, 3rd pin
	Pad(72 91
	    72 97
			   29
			      "2" "2" 0x100)
	# 2nd side, 3rd pin
	Pad(72 21
	       72 27
			   29
			      "3" "3" 0x100)
	# 2nd side, 2nd pin
	# 2nd side, 1st pin
	Pad(21 21
	       21 27
			   29
			      "4" "4" 0x100)
	Mark(21 94)
)
