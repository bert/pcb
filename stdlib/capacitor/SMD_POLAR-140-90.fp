	# Silkscreen box coordinates
Element(0x00 "chip_capacitor_polarized" "" "SMD_POLAR 140 90" 50 80 0 100 0x00)
(
	# PAD(X1, Y1, X1, Y2, T, 1)
	# PAD(X2, Y1, X2, Y2, T, 2)
	# Use Pad instead of PAD so both pads come out square
	Pad(0 -10 0 10 80 "1" 0x100)
	Pad(140 -10 140 10 80 "2" 0x100)
	ElementLine(-55 -65 -55 65 8)
	ElementLine(-55 65 195 65 8)
	ElementLine(195 65 195 -65 8)
	ElementLine(195 -65 -55 -65 8)
		# crude plus sign
		# ElementLine(      X1     eval(Y2L+20)       X1    eval(Y2L+70) 8)
		# ElementLine( eval(X1-25) eval(Y2L+45) eval(X1+25) eval(Y2L+45) 8)
		ElementLine( -45 -65 -45 65 8 )
)
