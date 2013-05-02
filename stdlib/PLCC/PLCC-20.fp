	    # was 15
	     # was 50
Element(0x00 "Plastic leadless chip carrier" "" "PLCC20" 100 175 0 100 0x00)
(
	# top left half
Pad(175 -20 175 40 20 "1" 0x00) 
	Pad(125 -20 125 40 20 "2" 0x100) 
	Pad(75 -20 75 40 20 "3" 0x100) 
	# left row
Pad(-20 75 40 75 20 "4" 0x100) 
	Pad(-20 125 40 125 20 "5" 0x100) 
	Pad(-20 175 40 175 20 "6" 0x100) 
	Pad(-20 225 40 225 20 "7" 0x100) 
	Pad(-20 275 40 275 20 "8" 0x100) 
	# bottom row
Pad(75 370 75 310 20 "9" 0x100) 
Pad(125 370 125 310 20 "10" 0x100) 
Pad(175 370 175 310 20 "11" 0x100) 
Pad(225 370 225 310 20 "12" 0x100) 
Pad(275 370 275 310 20 "13" 0x100) 
	# right row
Pad(370 275 310 275 20 "14" 0x100) 
Pad(370 225 310 225 20 "15" 0x100) 
Pad(370 175 310 175 20 "16" 0x100) 
Pad(370 125 310 125 20 "17" 0x100) 
Pad(370 75 310 75 20 "18" 0x100) 
	# top right row
Pad(275 -20 275 40 20 "19" 0x100) 
Pad(225 -20 225 40 20 "20" 0x100) 
#	ElementLine(50 0 WIDTH 0 20)
#	ElementLine(WIDTH 0 WIDTH WIDTH 20)
#	ElementLine(WIDTH WIDTH 0 WIDTH 20)
#	ElementLine(0 WIDTH 0 50 20)
#	ElementLine(0 50 50 0 20)
# Modified by Thomas Olson to eliminate silkscreen blobbing over pads.
# Approach one: eliminate ElementLine transgression over pads. leave corners
# only.
	ElementLine(300 0 350 0 10)
	ElementLine(350 0 350 50 10)
	ElementLine(350 300 350 350 10)
	ElementLine(350 350 300 350 10)
	ElementLine(50 350 0 350 10)
	ElementLine(0 350 0 300 10)
	ElementLine(0 50 50 0 10)
# Approach two: move outline to edge of pads.
# The outline should be 15 off. But since the pad algorithm
# is not making the square pads correctly I give it a total of 30
# to clear the pads.
# Try 40 mils, and parameterize it.  1/12/00 LRD
	ElementLine(50 -40 390 -40 10)
	ElementLine(390 -40 390 390 10)
	ElementLine(390 390 -40 390 10)
	ElementLine(-40 390 -40 50 10)
	ElementLine(-40 50 50 -40 10)
	ElementArc(175 100 20 20 0 360 10)
	Mark(0 0)
)
