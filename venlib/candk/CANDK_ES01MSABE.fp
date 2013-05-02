	# pad 1,2,3 width (1/100 mil)
	# pad 1,2,3 length (1/100 mil)
	# x value for pads 1,3 (1/100 mil)
	# y value for pads 1,2,3 (1/100 mil)
	# mounting pad width (1/100 mil)
	# mounting pad length (1/100 mil)
	# x value for the mounting pads (1/100 mil)
	# y value for the mounting pads (1/100 mil)
	# package width (1/100 mil)
	# package height (1/100 mil)
	# component veritcal height off board (1/100 mil)
	# pad width and length
	# y values for drawing the pad.  
	# The Y center of the pad is 0.5*(PINL + PINS)
	# we need a line segment of length PADL - PADW so we have end points:
	# 0.5*(PINL + PINS) +/- 0.5*(PADL - PADW)
	# width of soldermask relief (5 mil on each side)
	# top edge of switch body (1/100 mil)
	# bottom edge of switch body (1/100 mil)
	# how much the switch extends beyond the body
	# y value for the far end of the switch
	# silkscreen width (1/100 mils)
	# how much space to leave around the part before the
	# silk screen (1/100 mils)
	# X values for silk on sides and bottom of switch
	# bottom edge of the switch body
	# bottom edge of upper pads
	# bottom edge of the lower pads
	# top edge of the switch body
	# top edge of the switch 
# Element [SFlags "Desc" "Name" "Value" MX MY TX TY TDir TScale TSFlags]
Element[ "" "Description_candk_CANDK_ES01MSABE" "" "`CANDK_ES01MSABE'" 0 0 0 0 0 100 ""]
(
# Pad [rX1 rY1 rX2 rY2 Thickness Clearance Mask "Name" "Number" SFlags]
# the signal pads
Pad[ -10000 15150 -10000 19850 3500 1000 4500 "1"  "1" "square"]
Pad[      0 15150      0 19850 3500 1000 4500 "2"  "2" "square"]
Pad[  10000 15150  10000 19850 3500 1000 4500 "3"  "3" "square"]
# the mounting pads
Pad[ -19600 -17500 -22750 -17500 6000 1000 7000 "4"  "4" "square"]
Pad[  19600 -17500  22750 -17500 6000 1000 7000 "4"  "4" "square"]
# Silk screen around package
# ElementLine[ x1 y1 x2 y2 width]
# bottom edge
ElementLine[ 21050 14800  13050 14800 1000 ]
ElementLine[-21050 14800 -13050 14800 1000 ]
ElementLine[-13050 14800 -13050 22900 1000 ]
ElementLine[ 13050 14800  13050 22900 1000 ]
ElementLine[-13050 22900  13050 22900 1000 ]
# left/right
ElementLine[ 21050 14800  21050 -13200 1000 ]
ElementLine[-21050 14800 -21050 -13200 1000 ]
# top edge
ElementLine[-21050 -34600  21050 -34600 1000 ]
ElementLine[-21050 -23800     21050 -23800    1000 ]
ElementLine[-21050 -21800    -21050 -34600 1000 ]
ElementLine[ 21050 -21800     21050 -34600 1000 ]
# cross at top where switch moves
ElementLine[-21050 -34600  21050 -23800    1000 ]
ElementLine[-21050 -23800     21050 -34600 1000 ]
)
