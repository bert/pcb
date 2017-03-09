	# y-location of REFDES silk
	# width of box drawn in silk for the lever
		# lever on left
Element(0x00 "High-Temp (250C) Universal ZIF DIP Burn-in and Test Socket" "" "ARIES_42_6553_1" 0 -473 0 200 0x00)
(
        # footprint runs in two vertical rows with pin 1 at upper left
	# X = 0 will be the center line of the part
	Pin(-300 0 68 32 "1" 0x101)
	Pin(-300 100 68 32 "2" 0x01)
	Pin(-300 200 68 32 "3" 0x01)
	Pin(-300 300 68 32 "4" 0x01)
	Pin(-300 400 68 32 "5" 0x01)
	Pin(-300 500 68 32 "6" 0x01)
	Pin(-300 600 68 32 "7" 0x01)
	Pin(-300 700 68 32 "8" 0x01)
	Pin(-300 800 68 32 "9" 0x01)
	Pin(-300 900 68 32 "10" 0x01)
	Pin(-300 1000 68 32 "11" 0x01)
	Pin(-300 1100 68 32 "12" 0x01)
	Pin(-300 1200 68 32 "13" 0x01)
	Pin(-300 1300 68 32 "14" 0x01)
	Pin(-300 1400 68 32 "15" 0x01)
	Pin(-300 1500 68 32 "16" 0x01)
	Pin(-300 1600 68 32 "17" 0x01)
	Pin(-300 1700 68 32 "18" 0x01)
	Pin(-300 1800 68 32 "19" 0x01)
	Pin(-300 1900 68 32 "20" 0x01)
	Pin(-300 2000 68 32 "21" 0x01)
	# right column
	Pin(300 2000 68 32 "22" 0x01)
	Pin(300 1900 68 32 "23" 0x01)
	Pin(300 1800 68 32 "24" 0x01)
	Pin(300 1700 68 32 "25" 0x01)
	Pin(300 1600 68 32 "26" 0x01)
	Pin(300 1500 68 32 "27" 0x01)
	Pin(300 1400 68 32 "28" 0x01)
	Pin(300 1300 68 32 "29" 0x01)
	Pin(300 1200 68 32 "30" 0x01)
	Pin(300 1100 68 32 "31" 0x01)
	Pin(300 1000 68 32 "32" 0x01)
	Pin(300 900 68 32 "33" 0x01)
	Pin(300 800 68 32 "34" 0x01)
	Pin(300 700 68 32 "35" 0x01)
	Pin(300 600 68 32 "36" 0x01)
	Pin(300 500 68 32 "37" 0x01)
	Pin(300 400 68 32 "38" 0x01)
	Pin(300 300 68 32 "39" 0x01)
	Pin(300 200 68 32 "40" 0x01)
	Pin(300 100 68 32 "41" 0x01)
	Pin(300 0 68 32 "42" 0x01)
        # the mounting holes
        Pin(0 2180 105 65 "43" 0x01)
        Pin(0 -335 105 65 "44" 0x01)
	ElementLine(-475 -348 -475 2292 10)
	ElementLine(-475 2292 475 2292 10)
	ElementLine(475 2292 475 -348 10)
	ElementLine(475 -348 50 -348 10)
	ElementLine(-475 -348 -50 -348 10)
	ElementArc(0 -348 50 50 0 180 10)
	# and the lever silk
	ElementLine(325 -348  325 -833 10)
	ElementLine(325 -833 475 -833 10)
	ElementLine(475 -833 475 -348 10)
	Mark(-300 0)
)
