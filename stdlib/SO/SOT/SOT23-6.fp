Element(0x00 "SMT transistor, 6 pins" "" "SOT26" 138 0 3 100 0x00)
(
	# 1st side, 1st pin
	Pad(20 102 20 118 24 "1" "1" 0x100)

	# 1st side, 2nd pin
	Pad(59 102 59 118 24 "2" "2" 0x100)

	# 1st side, 3rd pin
	Pad(98 102 98 118 24 "3" "3" 0x100)

	# 2nd side, 3rd pin
	Pad(98 20  98 36  24 "4" "4" 0x100)

	# 2nd side, 2nd pin
	Pad(59 20  59 36  24 "5" "5" 0x100)

	# 2nd side, 1st pin
	Pad(20 20  20 36  24 "6" "6" 0x100)

	ElementLine(0 0 0 139 5)
	ElementLine(0 139 118 139 5)

	ElementLine(118 139 118 0 5)
	ElementLine(118 0 0 0 5)

	ElementLine(0 139 -10 149 5)

	Mark(20 110)
)
