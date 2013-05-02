	    # was 15
	     # was 50
Element(0x00 "generic" "" "PLCC 44 150" 100 325 0 100 0x00)
(
	# top left half
Pad(325 -20 325 40 20 "1" 0x00) 
	Pad(275 -20 275 40 20 "2" 0x100) 
	Pad(225 -20 225 40 20 "3" 0x100) 
	Pad(175 -20 175 40 20 "4" 0x100) 
	Pad(125 -20 125 40 20 "5" 0x100) 
	Pad(75 -20 75 40 20 "6" 0x100) 
	# left row
Pad(-20 75 40 75 20 "7" 0x100) 
	Pad(-20 125 40 125 20 "8" 0x100) 
	Pad(-20 175 40 175 20 "9" 0x100) 
	Pad(-20 225 40 225 20 "10" 0x100) 
	Pad(-20 275 40 275 20 "11" 0x100) 
	Pad(-20 325 40 325 20 "12" 0x100) 
	Pad(-20 375 40 375 20 "13" 0x100) 
	Pad(-20 425 40 425 20 "14" 0x100) 
	Pad(-20 475 40 475 20 "15" 0x100) 
	Pad(-20 525 40 525 20 "16" 0x100) 
	Pad(-20 575 40 575 20 "17" 0x100) 
	# bottom row
Pad(75 670 75 610 20 "18" 0x100) 
Pad(125 670 125 610 20 "19" 0x100) 
Pad(175 670 175 610 20 "20" 0x100) 
Pad(225 670 225 610 20 "21" 0x100) 
Pad(275 670 275 610 20 "22" 0x100) 
Pad(325 670 325 610 20 "23" 0x100) 
Pad(375 670 375 610 20 "24" 0x100) 
Pad(425 670 425 610 20 "25" 0x100) 
Pad(475 670 475 610 20 "26" 0x100) 
Pad(525 670 525 610 20 "27" 0x100) 
Pad(575 670 575 610 20 "28" 0x100) 
	# right row
Pad(670 575 610 575 20 "29" 0x100) 
Pad(670 525 610 525 20 "30" 0x100) 
Pad(670 475 610 475 20 "31" 0x100) 
Pad(670 425 610 425 20 "32" 0x100) 
Pad(670 375 610 375 20 "33" 0x100) 
Pad(670 325 610 325 20 "34" 0x100) 
Pad(670 275 610 275 20 "35" 0x100) 
Pad(670 225 610 225 20 "36" 0x100) 
Pad(670 175 610 175 20 "37" 0x100) 
Pad(670 125 610 125 20 "38" 0x100) 
Pad(670 75 610 75 20 "39" 0x100) 
	# top right row
Pad(575 -20 575 40 20 "40" 0x100) 
Pad(525 -20 525 40 20 "41" 0x100) 
Pad(475 -20 475 40 20 "42" 0x100) 
Pad(425 -20 425 40 20 "43" 0x100) 
Pad(375 -20 375 40 20 "44" 0x100) 
#	ElementLine(50 0 WIDTH 0 20)
#	ElementLine(WIDTH 0 WIDTH WIDTH 20)
#	ElementLine(WIDTH WIDTH 0 WIDTH 20)
#	ElementLine(0 WIDTH 0 50 20)
#	ElementLine(0 50 50 0 20)
# Modified by Thomas Olson to eliminate silkscreen blobbing over pads.
# Approach one: eliminate ElementLine transgression over pads. leave corners
# only.
	ElementLine(600 0 650 0 10)
	ElementLine(650 0 650 50 10)
	ElementLine(650 600 650 650 10)
	ElementLine(650 650 600 650 10)
	ElementLine(50 650 0 650 10)
	ElementLine(0 650 0 600 10)
	ElementLine(0 50 50 0 10)
# Approach two: move outline to edge of pads.
# The outline should be 15 off. But since the pad algorithm
# is not making the square pads correctly I give it a total of 30
# to clear the pads.
# Try 40 mils, and parameterize it.  1/12/00 LRD
	ElementLine(50 -40 690 -40 10)
	ElementLine(690 -40 690 690 10)
	ElementLine(690 690 -40 690 10)
	ElementLine(-40 690 -40 50 10)
	ElementLine(-40 50 50 -40 10)
	ElementArc(325 100 20 20 0 360 10)
	Mark(0 0)
)
