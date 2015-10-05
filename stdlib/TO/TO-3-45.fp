Element(0x00 "high-power transistor" "Q" "TO3_45" 1300 100 0 250 0x00)
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

	Pin(750 750 90 55 "E" 0x101)
	Pin(960 380 90 55 "B" 0x01)
	Pin(1300 830 250 177 "C" 0x01)
	Pin(280 210 250 177 "C" 0x01)

	ElementLine(660 1010 1300 1010 10)
	ElementLine(1210 230 1470 770 10)
	ElementLine(110 270 375 810 10)
	ElementLine(280 30 920 30 10)

	ElementArc(790 520 420 420 0 360 10)
	ElementArc(790 520 510 510 215 40 10)
	ElementArc(790 520 510 510 35 40 10)

	ElementArc(1300 830 180 180 90 110 10)
	ElementArc(280 210 180 180 270 110 10)

	Mark(750 750)
)
