	# Silkscreen box coordinates
Element(0x00 "chip_capacitor_polarized" "" "SMD_POLAR 80 50" 32 59 0 100 0x00)
(
	# PAD(X1, Y1, X1, Y2, T, 1)
	# PAD(X2, Y1, X2, Y2, T, 2)
	# Use Pad instead of PAD so both pads come out square
	Pad(0 -7 0 7 45 "1" 0x100)
	Pad(80 -7 80 7 45 "2" 0x100)
	ElementLine(-37 -44 -37 44 8)
	ElementLine(-37 44 117 44 8)
	ElementLine(117 44 117 -44 8)
	ElementLine(117 -44 -37 -44 8)
		# crude plus sign
		# ElementLine(      X1     eval(Y2L+20)       X1    eval(Y2L+70) 8)
		# ElementLine( eval(X1-25) eval(Y2L+45) eval(X1+25) eval(Y2L+45) 8)
		ElementLine( -27 -44 -27 44 8 )
)
