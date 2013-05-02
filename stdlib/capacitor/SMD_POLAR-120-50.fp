	# Silkscreen box coordinates
Element(0x00 "chip_capacitor_polarized" "" "SMD_POLAR 120 50" 37 60 0 100 0x00)
(
	# PAD(X1, Y1, X1, Y2, T, 1)
	# PAD(X2, Y1, X2, Y2, T, 2)
	# Use Pad instead of PAD so both pads come out square
	Pad(0 -3 0 3 55 "1" 0x100)
	Pad(120 -3 120 3 55 "2" 0x100)
	ElementLine(-42 -45 -42 45 8)
	ElementLine(-42 45 162 45 8)
	ElementLine(162 45 162 -45 8)
	ElementLine(162 -45 -42 -45 8)
		# crude plus sign
		# ElementLine(      X1     eval(Y2L+20)       X1    eval(Y2L+70) 8)
		# ElementLine( eval(X1-25) eval(Y2L+45) eval(X1+25) eval(Y2L+45) 8)
		ElementLine( -32 -45 -32 45 8 )
)
