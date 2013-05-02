	# number of pins on left/right sides (pin1 is upper pin on left side)
	# number of pins on top/bottom sides
	# pin pitch (1/1000 mil)
	# y-coordinate for upper pin on left/right sides  (1/1000 mil)
	# x-coordinate for right pin on top/bottom sides  (1/1000 mil)
	# total horizontal package width (1/1000 mil)
	# total vertical package width (1/1000 mil)
	# how much pads extend beyond the package edge (1/1000 mil) (the 75 is 0.75 mm)
	# how much pads extend inward from the package pad edge (1/1000 mil)
	# pad length/width (1/1000 mil)
	# pad width (mil/100)
	# min/max x coordinates for the pads on the left/right sides of the package (mil/100)
	# min/max y coordinates for the pads on the top/bottom sides of the package (mil/100)
	# pad size and drill size (mil/100) for the mounting holes
	# soldermask relief size for mounting holes (mil/100) 
	# silkscreen width (mils/100)
	# how much the silk screen is moved away from the package (1/1000 mil)
	# upper right corner for silk screen (mil/100)
	# refdes text size (mil/100)
	# x,y coordinates for refdes label (mil/100)
	# square exposed paddle size (mil/100)
	# location of mounting holes (mil/100)
	# latch silkscreen width (mils/100)
	# points for latch silk on the left/right sides of the part (mil/100)
	# points for latch silk on the top/bottom sides of the part (mil/100)
	# points for silk showing where the exposed paddle contacts are (mil/100)
	# spacing between rows of EP contacts in 1/100 mm.
	# soldermask opening (mil/100)
# element_flags, description, pcb-name, value, mark_x, mark_y,
# text_x, text_y, text_direction, text_scale, text_flags
Element[0x00000000 "Johnstech QFN Socket, Series 1MM (724812-724839)" "" "JOHNSTECH_QFN32_5" 0 0 -18716 -19766 0 100 0x00000000]
(
# left row
	Pad[-12086  -6889  -8385  -6889  1417  0 0 "1" "1"  0x00000000]
	Pad[-12086  -4921  -8385  -4921  1417  0 0 "2" "2"  0x00000000]
	Pad[-12086  -2952  -8385  -2952  1417  0 0 "3" "3"  0x00000000]
	Pad[-12086  -984  -8385  -984  1417  0 0 "4" "4"  0x00000000]
	Pad[-12086  984  -8385  984  1417  0 0 "5" "5"  0x00000000]
	Pad[-12086  2952  -8385  2952  1417  0 0 "6" "6"  0x00000000]
	Pad[-12086  4921  -8385  4921  1417  0 0 "7" "7"  0x00000000]
	Pad[-12086  6889  -8385  6889  1417  0 0 "8" "8"  0x00000000]
# bottom row
	Pad[-6889  12086  -6889  8385  1417 0 0 "9" "9"  0x00000800]
	Pad[-4921  12086  -4921  8385  1417 0 0 "10" "10"  0x00000800]
	Pad[-2952  12086  -2952  8385  1417 0 0 "11" "11"  0x00000800]
	Pad[-984  12086  -984  8385  1417 0 0 "12" "12"  0x00000800]
	Pad[984  12086  984  8385  1417 0 0 "13" "13"  0x00000800]
	Pad[2952  12086  2952  8385  1417 0 0 "14" "14"  0x00000800]
	Pad[4921  12086  4921  8385  1417 0 0 "15" "15"  0x00000800]
	Pad[6889  12086  6889  8385  1417 0 0 "16" "16"  0x00000800]
# right row
	Pad[12086  6889  8385  6889  1417  0 0 "17" "17"  0x00000000]
	Pad[12086  4921  8385  4921  1417  0 0 "18" "18"  0x00000000]
	Pad[12086  2952  8385  2952  1417  0 0 "19" "19"  0x00000000]
	Pad[12086  984  8385  984  1417  0 0 "20" "20"  0x00000000]
	Pad[12086  -984  8385  -984  1417  0 0 "21" "21"  0x00000000]
	Pad[12086  -2952  8385  -2952  1417  0 0 "22" "22"  0x00000000]
	Pad[12086  -4921  8385  -4921  1417  0 0 "23" "23"  0x00000000]
	Pad[12086  -6889  8385  -6889  1417  0 0 "24" "24"  0x00000000]
# top row
	Pad[6889  -12086  6889  -8385  1417 0 0 "25" "25" 0x00000800]
	Pad[4921  -12086  4921  -8385  1417 0 0 "26" "26" 0x00000800]
	Pad[2952  -12086  2952  -8385  1417 0 0 "27" "27" 0x00000800]
	Pad[984  -12086  984  -8385  1417 0 0 "28" "28" 0x00000800]
	Pad[-984  -12086  -984  -8385  1417 0 0 "29" "29" 0x00000800]
	Pad[-2952  -12086  -2952  -8385  1417 0 0 "30" "30" 0x00000800]
	Pad[-4921  -12086  -4921  -8385  1417 0 0 "31" "31" 0x00000800]
	Pad[-6889  -12086  -6889  -8385  1417 0 0 "32" "32" 0x00000800]
# Exposed paddle.  Note that this pad also sets the soldermask
# relief for the entire part.
# Pad(X1, Y1, X2, Y3, width, clearance,
#     soldermask, "pin name", "pin number", flags)
Pad[0 0 0 0 12204 0 35433 "33" "33" 0x00000100]
# Mounting pins
# Pin(x, y, thickness, clearance, mask, drilling hole, name,
#     number, flags 
Pin[ 13779 13779 7700 1000 8700 2000 "Mount1" "34" 0x0]
Pin[ -13779 13779 7700 1000 8700 2000 "Mount2" "35" 0x0]
Pin[ -13779 -13779 7700 1000 8700 2000 "Mount3" "36" 0x0]
Pin[ 13779 -13779 7700 1000 8700 2000 "Mount4" "37" 0x0]
# Silk screen around package
ElementLine[ 18716  18716  18716 -18716 1000]
ElementLine[ 18716 -18716 -18716 -18716 1000]
ElementLine[-18716 -18716 -18716  18716 1000]
ElementLine[-18716  18716  18716  18716 1000]
# Pin 1 indicator
ElementLine[-18716 -18716 -20216 -20216 1000]
# Silk showing latch area
# top
ElementLine[ -10826 -18716 -10826 -25590 100 ]
ElementLine[ -10826 -25590 10826 -25590 100 ]
ElementLine[ 10826 -18716 10826 -25590 100 ]
# bottom
ElementLine[ -10826 18716 -10826 25590 100 ]
ElementLine[ -10826 25590 10826 25590 100 ]
ElementLine[ 10826 18716 10826 25590 100 ]
# left
ElementLine[ -18716 10826 -25590 10826 100 ]
ElementLine[ -25590 10826 -25590 -10826 100 ]
ElementLine[ -18716 -10826 -25590 -10826 100 ]
# right
ElementLine[ 18716 10826 25590 10826 100 ]
ElementLine[ 25590 10826 25590 -10826 100 ]
ElementLine[ 18716 -10826 25590 -10826 100 ]
# Silk showing area for exposed paddle socket contacts
ElementLine[ 984 -11811 984  11811 100 ]
ElementLine[ 1771 -11811 1771  11811 100 ]
ElementLine[ 984  11811 1771  11811 100 ]
ElementLine[ 984 -11811 1771 -11811 100 ]
# packages with width >= 6.0 mm have 2 rows of contacts
)
