Element(0x00 "Pressure transducer" "" "MPAK" 235 0 3 100 0x00)
(
	ElementLine(0 0 0 558 10)
	ElementLine(0 558 215 558 10)
	ElementLine(215 558 215 0 10)
	ElementLine(215 0 0 0 10)
	# 1st pin on pin side
	Pad(32 469
	    32 525
			   31 "1" "1" 0x100)
	Pad(82 469
	       82 525
			   31 "2" "2" 0x100)
	   Pad(132 469
	       132 525
			   31 "3" "3" 0x100)
	# last pin on pin side
	Pad(182 469
	    182 525
			   31 "4" "4" 0x100)
	# extra wide pin on opposite side
	Pad(144 60
	    70 60
			   87 "5" "5" 0x100)
	Mark(32 497)
)
