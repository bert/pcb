	                             # 78 for SOT23
	                             # 82 for SOT23
	      # 41 for SOT23
	               # 34 for SOT23, 24 for SOT25
Element(0x00 "SMT transistor, 6 pins" "" "SC70_6" 100 0 3 100 0x00)
(
	ElementLine(-5  -5 -5 125 8)
	ElementLine(-5 125 85 125 8)
	ElementLine(85 125 85  -5 8)
	ElementLine(85  -5 -5  -5 8)
	# 1st side, 1st pin
	Pad(14 84
	       14 104
			   15 "1" "1" 0x100)
	# 1st side, 2nd pin
	Pad(40 84
	       40 104
			   15 "2" "2" 0x100)
	# 1st side, 3rd pin
	Pad(65 84
	    65 104
			   15 "3" "3" 0x100)
	# 2nd side, 3rd pin
	Pad(65 14
	       65 34
			   15 "4" "4" 0x100)
	# 2nd side, 2nd pin
	Pad(40 14
	       40 34
			   15 "5" "5" 0x100)
	# 2nd side, 1st pin
	Pad(14 14
	       14 34
			   15 "6" "6" 0x100)
	Mark(14 94)
)
