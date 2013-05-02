	    # was 15
	     # was 50
Element(0x00 "Plastic leadless chip carrier" "" "PLCC32" 100 225 0 100 0x00)
(
	# top left half
Pad(225 -20 225 40 20 "1" 0x00) 
	Pad(175 -20 175 40 20 "2" 0x100) 
	Pad(125 -20 125 40 20 "3" 0x100) 
	Pad(75 -20 75 40 20 "4" 0x100) 
	# left row
Pad(-20 75 40 75 20 "5" 0x100) 
	Pad(-20 125 40 125 20 "6" 0x100) 
	Pad(-20 175 40 175 20 "7" 0x100) 
	Pad(-20 225 40 225 20 "8" 0x100) 
	Pad(-20 275 40 275 20 "9" 0x100) 
	Pad(-20 325 40 325 20 "10" 0x100) 
	Pad(-20 375 40 375 20 "11" 0x100) 
	Pad(-20 425 40 425 20 "12" 0x100) 
	Pad(-20 475 40 475 20 "13" 0x100) 
	# bottom row
Pad(75 570 75 510 20 "14" 0x100) 
Pad(125 570 125 510 20 "15" 0x100) 
Pad(175 570 175 510 20 "16" 0x100) 
Pad(225 570 225 510 20 "17" 0x100) 
Pad(275 570 275 510 20 "18" 0x100) 
Pad(325 570 325 510 20 "19" 0x100) 
Pad(375 570 375 510 20 "20" 0x100) 
	# right row
Pad(470 475 410 475 20 "21" 0x100) 
Pad(470 425 410 425 20 "22" 0x100) 
Pad(470 375 410 375 20 "23" 0x100) 
Pad(470 325 410 325 20 "24" 0x100) 
Pad(470 275 410 275 20 "25" 0x100) 
Pad(470 225 410 225 20 "26" 0x100) 
Pad(470 175 410 175 20 "27" 0x100) 
Pad(470 125 410 125 20 "28" 0x100) 
Pad(470 75 410 75 20 "29" 0x100) 
	# top right row
Pad(375 -20 375 40 20 "30" 0x100) 
Pad(325 -20 325 40 20 "31" 0x100) 
Pad(275 -20 275 40 20 "32" 0x100) 
#	ElementLine(50 0 WIDTH 0 20)
#	ElementLine(WIDTH 0 WIDTH WIDTH 20)
#	ElementLine(WIDTH WIDTH 0 WIDTH 20)
#	ElementLine(0 WIDTH 0 50 20)
#	ElementLine(0 50 50 0 20)
# Modified by Thomas Olson to eliminate silkscreen blobbing over pads.
# Approach one: eliminate ElementLine transgression over pads. leave corners
# only.
	ElementLine(400 0 450 0 10)
	ElementLine(450 0 450 50 10)
	ElementLine(450 500 450 550 10)
	ElementLine(450 550 400 550 10)
	ElementLine(50 550 0 550 10)
	ElementLine(0 550 0 500 10)
	ElementLine(0 50 50 0 10)
# Approach two: move outline to edge of pads.
# The outline should be 15 off. But since the pad algorithm
# is not making the square pads correctly I give it a total of 30
# to clear the pads.
# Try 40 mils, and parameterize it.  1/12/00 LRD
	ElementLine(50 -40 490 -40 10)
	ElementLine(490 -40 490 590 10)
	ElementLine(490 590 -40 590 10)
	ElementLine(-40 590 -40 50 10)
	ElementLine(-40 50 50 -40 10)
	ElementArc(225 100 20 20 0 360 10)
	Mark(0 0)
)
