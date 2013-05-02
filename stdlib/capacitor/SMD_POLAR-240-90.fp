	# Silkscreen box coordinates
Element(0x00 "chip_capacitor_polarized" "" "SMD_POLAR 240 90" 62 80 0 100 0x00)
(
	# PAD(X1, Y1, X1, Y2, T, 1)
	# PAD(X2, Y1, X2, Y2, T, 2)
	# Use Pad instead of PAD so both pads come out square
	Pad(0 2 0 -2 105 "1" 0x100)
	Pad(240 2 240 -2 105 "2" 0x100)
	ElementLine(-67 -65 -67 65 8)
	ElementLine(-67 65 307 65 8)
	ElementLine(307 65 307 -65 8)
	ElementLine(307 -65 -67 -65 8)
		# crude plus sign
		# ElementLine(      X1     eval(Y2L+20)       X1    eval(Y2L+70) 8)
		# ElementLine( eval(X1-25) eval(Y2L+45) eval(X1+25) eval(Y2L+45) 8)
		ElementLine( -57 -65 -57 65 8 )
)
