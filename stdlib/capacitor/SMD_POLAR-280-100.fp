	# Silkscreen box coordinates
Element(0x00 "chip_capacitor_polarized" "" "SMD_POLAR 280 100" 70 85 0 100 0x00)
(
	# PAD(X1, Y1, X1, Y2, T, 1)
	# PAD(X2, Y1, X2, Y2, T, 2)
	# Use Pad instead of PAD so both pads come out square
	Pad(0 5 0 -5 120 "1" 0x100)
	Pad(280 5 280 -5 120 "2" 0x100)
	ElementLine(-75 -70 -75 70 8)
	ElementLine(-75 70 355 70 8)
	ElementLine(355 70 355 -70 8)
	ElementLine(355 -70 -75 -70 8)
		# crude plus sign
		# ElementLine(      X1     eval(Y2L+20)       X1    eval(Y2L+70) 8)
		# ElementLine( eval(X1-25) eval(Y2L+45) eval(X1+25) eval(Y2L+45) 8)
		ElementLine( -65 -70 -65 70 8 )
)
