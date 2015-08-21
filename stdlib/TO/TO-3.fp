Element(0x00 "high-power transistor" "Q" "TO-3" 1200 300 0 250 0x00)
(
# The JEDEC drawing specifies that pins #1
# and #2 have a diameter from 38 to 43 mils.
# The mounting holes (pins 3 and 4 here) are
# 151 to 161 mils.  Increasing by 15 mils would
# give a drill diameter of 58 and 176 mils.  
# 55 and 177 are close in standard drill sizes.
# a #4 machine screw is 110 mils, a #6 is 140 mils and a 
# #8 is 160 mils in diameter.  Looks like you can not count
# on using a #8 for a TO3, but a #6 is fair.
# This would give something like a 90 pad size for a 
# 35 mil annular ring for pins 1 and 2.

	Pin(650 1000 90 55 "E" 0x101)
	Pin(650 550 90 55 "B" 0x01)
	Pin(1320 775 250 177 "C" 0x01)
	Pin(125 775 250 177 "C" 0x01)

	ElementArc(700 775 500 500 70 40 10)
	ElementArc(700 775 500 500 250 40 10)
	ElementArc(1320 775 180 180 125 110 10)
	ElementArc(125 775 180 180 305 110 10)
	ElementLine(25 925 530 1245 10)
	ElementLine(25 625 530 305 10)
	ElementLine(870 305 1430 630 10)
	ElementLine(870 1245 1430 920 10)

	Mark(650 775)
)
