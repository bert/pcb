Element(0x00 "SMT transistor, 4 pins" "" "SOT143" 144 0 3 100 0x00)
(
	ElementLine(0 0 0 139 5)
	ElementLine(0 139 124 139 5)
	ElementLine(124 139 124 0 5)
	ElementLine(124 0 0 0 5)

	# 1st side, 1st pin
	# extra width
	   Pad(28 110
	       31 110
			   34
			      "1" "1" 0x100)
	# 1st side, 2nd pin
	# 1st side, 3rd pin
	Pad(99 107
	    99 113
			   34
			      "2" "2" 0x100)
	# 2nd side, 3rd pin
	Pad(99 25
	       99 31
			   34
			      "3" "3" 0x100)
	# 2nd side, 2nd pin
	# 2nd side, 1st pin
	Pad(25 25
	       25 31
			   34
			      "4" "4" 0x100)
	Mark(25 110)
)
