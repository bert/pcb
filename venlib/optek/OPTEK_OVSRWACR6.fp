	# pad width and length (1/100 mil)
	# pad center (X)  (1/100 mil)
	# x values for the pads
	# package width (1/100 mil)
	# package height (1/100 mil)
	# width of soldermask relief (5 mil on each side)
	# silkscreen width (1/100 mils)
	# how much space to leave around the part before the
	# silk screen (1/100 mils)
	# top edge silk
	# bottom edge silk
# Element [SFlags "Desc" "Name" "Value" MX MY TX TY TDir TScale TSFlags]
Element[ "" "Description_optek_OPTEK_OVSRWACR6" "" "`OPTEK_OVSRWACR6'" 0 0 0 0 0 100 ""]
(
# Pad [rX1 rY1 rX2 rY2 Thickness Clearance Mask "Name" "Number" SFlags]
# the pads
Pad[ -5708 0 -3740 0 3543 1000 4543 "K"  "1" "square"]
Pad[  5708 0  3740 0 3543 1000 4543 "A"  "2" "square"]
# Silk screen around package
# ElementLine[ x1 y1 x2 y2 width]
# top edge
ElementLine[ -668 -1771 668 -1771 1000 ]
# left/right and bottom
ElementLine[ -6800 3071 -6800 4600 1000 ]
ElementLine[  6800 3071  6800 4600 1000 ]
ElementLine[ -6800 4600  6800 4600 1000 ]
)
