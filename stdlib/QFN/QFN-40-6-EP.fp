	# number of pins on left/right sides (pin1 is upper pin on left side)
	# number of pins on top/bottom sides
	# pin pitch (1/1000 mil)
	# y-coordinate for upper pin on left/right sides  (1/1000 mil)
	# x-coordinate for right pin on top/bottom sides  (1/1000 mil)
	# total horizontal package width (1/1000 mil)
	# total vertical package width (1/1000 mil)
	# how much pads extend beyond the package edge (1/1000 mil) (the 25 is 0.25 mm)
	# how much pads extend inward from the package pad edge (1/1000 mil)
	# pad length/width (1/1000 mil)
	# pad width (mil/100)
	# min/max x coordinates for the pads on the left/right sides of the package (mil/100)
	# min/max y coordinates for the pads on the top/bottom sides of the package (mil/100)
	# silkscreen width (mils/100)
	# how much the silk screen is moved away from the package (1/1000 mil)
	# upper right corner for silk screen (mil/100)
	# refdes text size (mil/100)
	# x,y coordinates for refdes label (mil/100)
	# square exposed paddle size (mil/100)
   # pad clearance to polygons (1/100 mil)
   # width of the pad solder mask relief (1/100 mil). 
   # grow by 1.5 mils on each side
   # width of the paddle soldermask relief (1/100 mil)
   # grow by 200 mils on each side
# element_flags, description, pcb-name, value, mark_x, mark_y,
# text_x, text_y, text_direction, text_scale, text_flags
Element[0x00000000 "Square Quad Flat Nolead (QFN) package" "" "QFN40_6_EP" 0 0 -13795 -14845 0 100 0x00000000]
(
# Pad[X1, Y1, X2, Y3, width, clearance,
#     soldermask, "pin name", "pin number", flags]
# left row
	Pad[-12244  -8858  -10590  -8858  1102  2000 1402 "1" "1"  0x00000100]
	Pad[-12244  -6889  -10590  -6889  1102  2000 1402 "2" "2"  0x00000100]
	Pad[-12244  -4921  -10590  -4921  1102  2000 1402 "3" "3"  0x00000100]
	Pad[-12244  -2952  -10590  -2952  1102  2000 1402 "4" "4"  0x00000100]
	Pad[-12244  -984  -10590  -984  1102  2000 1402 "5" "5"  0x00000100]
	Pad[-12244  984  -10590  984  1102  2000 1402 "6" "6"  0x00000100]
	Pad[-12244  2952  -10590  2952  1102  2000 1402 "7" "7"  0x00000100]
	Pad[-12244  4921  -10590  4921  1102  2000 1402 "8" "8"  0x00000100]
	Pad[-12244  6889  -10590  6889  1102  2000 1402 "9" "9"  0x00000100]
	Pad[-12244  8858  -10590  8858  1102  2000 1402 "10" "10"  0x00000100]
# bottom row
	Pad[-8858  12244  -8858  10590  1102 2000 1402 "11" "11"  0x00000900]
	Pad[-6889  12244  -6889  10590  1102 2000 1402 "12" "12"  0x00000900]
	Pad[-4921  12244  -4921  10590  1102 2000 1402 "13" "13"  0x00000900]
	Pad[-2952  12244  -2952  10590  1102 2000 1402 "14" "14"  0x00000900]
	Pad[-984  12244  -984  10590  1102 2000 1402 "15" "15"  0x00000900]
	Pad[984  12244  984  10590  1102 2000 1402 "16" "16"  0x00000900]
	Pad[2952  12244  2952  10590  1102 2000 1402 "17" "17"  0x00000900]
	Pad[4921  12244  4921  10590  1102 2000 1402 "18" "18"  0x00000900]
	Pad[6889  12244  6889  10590  1102 2000 1402 "19" "19"  0x00000900]
	Pad[8858  12244  8858  10590  1102 2000 1402 "20" "20"  0x00000900]
# right row
	Pad[12244  8858  10590  8858  1102  2000 1402 "21" "21"  0x00000100]
	Pad[12244  6889  10590  6889  1102  2000 1402 "22" "22"  0x00000100]
	Pad[12244  4921  10590  4921  1102  2000 1402 "23" "23"  0x00000100]
	Pad[12244  2952  10590  2952  1102  2000 1402 "24" "24"  0x00000100]
	Pad[12244  984  10590  984  1102  2000 1402 "25" "25"  0x00000100]
	Pad[12244  -984  10590  -984  1102  2000 1402 "26" "26"  0x00000100]
	Pad[12244  -2952  10590  -2952  1102  2000 1402 "27" "27"  0x00000100]
	Pad[12244  -4921  10590  -4921  1102  2000 1402 "28" "28"  0x00000100]
	Pad[12244  -6889  10590  -6889  1102  2000 1402 "29" "29"  0x00000100]
	Pad[12244  -8858  10590  -8858  1102  2000 1402 "30" "30"  0x00000100]
# top row
	Pad[8858  -12244  8858  -10590  1102 2000 1402 "31" "31" 0x00000900]
	Pad[6889  -12244  6889  -10590  1102 2000 1402 "32" "32" 0x00000900]
	Pad[4921  -12244  4921  -10590  1102 2000 1402 "33" "33" 0x00000900]
	Pad[2952  -12244  2952  -10590  1102 2000 1402 "34" "34" 0x00000900]
	Pad[984  -12244  984  -10590  1102 2000 1402 "35" "35" 0x00000900]
	Pad[-984  -12244  -984  -10590  1102 2000 1402 "36" "36" 0x00000900]
	Pad[-2952  -12244  -2952  -10590  1102 2000 1402 "37" "37" 0x00000900]
	Pad[-4921  -12244  -4921  -10590  1102 2000 1402 "38" "38" 0x00000900]
	Pad[-6889  -12244  -6889  -10590  1102 2000 1402 "39" "39" 0x00000900]
	Pad[-8858  -12244  -8858  -10590  1102 2000 1402 "40" "40" 0x00000900]
# Exposed paddle (if this is an exposed paddle part)
# Pad(X1, Y1, X2, Y3, width, clearance,
#     soldermask, "pin name", "pin number", flags)
	Pad[0 0 0 0 16141 0 16541 "41" "41" 0x00000100]
# Silk screen around package
ElementLine[ 13795  13795  13795 -13795 1000]
ElementLine[ 13795 -13795 -13795 -13795 1000]
ElementLine[-13795 -13795 -13795  13795 1000]
ElementLine[-13795  13795  13795  13795 1000]
# Pin 1 indicator
ElementLine[-13795 -13795 -15295 -15295 1000]
)
