Element(0x00 "SMT transistor SC88A, 5 pins" "" "SOT325" 100 0 3 100 0x00)
(
	ElementLine( 0   0  0 119 5)
	ElementLine( 0 119 80 119 5)
	ElementLine(80 119 80   0 5)
	ElementLine(80   0  0   0 5)

	# 1st side, 1st pin
    Pad(65 14 65 34 15 "1" "1" 0x100)

    # 1st side, 2nd pin
	Pad(40 14 40 34	15 "2" "2" 0x100)

	# 1st side, 3rd pin
	Pad(14 14 14 34 15 "3" "3" 0x100)

	# 2nd side, 3rd pin
	Pad(14 84 14 104 15 "4" "4" 0x100)
	
    # 2nd side, 2nd pin

	# 2nd side, 1st pin
	Pad(65 84 65 104 15 "5" "5" 0x100)

	Mark(65 24)
)
