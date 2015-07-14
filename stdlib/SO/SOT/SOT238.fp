Element(0x00 "SMT transistor, 8 pins" "" "SOT238" 100 0 3 100 0x00)
(
	ElementLine(0 0 0 119 5)
	ElementLine(0 119 106 119 5)
	ElementLine(106 119 106 0 5)
	ElementLine(106 0 0 0 5)
    ElementLine(0 119 -10 129 5)

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
	# 1st side, 4rd pin
	Pad(91 84
	    91 104
			   15 "4" "4" 0x100)
	# 2nd side, 4rd pin
	Pad(91 14
	       91 34
			   15 "5" "5" 0x100)
	# 2nd side, 3rd pin
	Pad(65 14
	       65 34
			   15 "6" "6" 0x100)
	# 2nd side, 2nd pin
	Pad(40 14
	       40 34
			   15 "7" "7" 0x100)
	# 2nd side, 1st pin
	Pad(14 14
	       14 34
			   15 "8" "8" 0x100)
	Mark(14 94)
)

