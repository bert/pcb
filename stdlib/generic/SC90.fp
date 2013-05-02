	                             # 78 for SOT23
	                             # 82 for SOT23
	      # 41 for SOT23
	               # 34 for SOT23, 24 for SOT25
Element(0x00 "SMT transistor, 3 pins" "" "SC90" 93 0 3 100 0x00)
(
	ElementLine(0 0 0 98 10)
	ElementLine(0 98 73 98 10)
	ElementLine(73 98 73 0 10)
	ElementLine(73 0 0 0 10)
	# 1st side, 1st pin
	Pad(17 76
	       17 80
			   24
			      "1" "1" 0x100)
	# 1st side, 2nd pin
	# 1st side, 3rd pin
	Pad(56 76
	    56 80
			   24
			      "2" "2" 0x100)
	# 2nd side, 3rd pin
	# 2nd side, 2nd pin
	Pad(36 17
	       36 21
			   24
			      "3" "3" 0x100)
	# 2nd side, 1st pin
	Mark(17 78)
)
