	# number of pads
	# pad width in 1/1000 mil
	# pad length in 1/1000 mil
	# pad pitch 1/1000 mil
	# seperation between pads on opposite sides 1/1000 mil
	# X coordinates for the right hand column of pads (1/100 mils)
	# pad clearance to plane layer in 1/100 mil
	# pad soldermask width in 1/100 mil
	# silk screen width (1/100 mils)
	# figure out if we have an even or odd number of pins per side
	# silk bounding box is -XMAX,-YMAX, XMAX,YMAX (1/100 mils)
# element_flags, description, pcb-name, value, mark_x, mark_y,
# text_x, text_y, text_direction, text_scale, text_flags
Element[0x00000000 "Panasonic EXB Series Chip Resistor Array" "" "PANASONIC_EXB2HV" 0 0 -2000 -6000 0 100 0x00000000]
(
# 
# Pad[x1, y1, x2, y2, thickness, clearance, mask, name , pad number, flags]
        Pad[   -3444 -6889 
			 -2460 -6889 
			984 1000 1984 "1" "1" 0x00000100]
        Pad[   -3444 -4921 
			 -2460 -4921 
			984 1000 1984 "2" "2" 0x00000100]
        Pad[   -3444 -2952 
			 -2460 -2952 
			984 1000 1984 "3" "3" 0x00000100]
        Pad[   -3444 -984 
			 -2460 -984 
			984 1000 1984 "4" "4" 0x00000100]
        Pad[   -3444 984 
			 -2460 984 
			984 1000 1984 "5" "5" 0x00000100]
        Pad[   -3444 2952 
			 -2460 2952 
			984 1000 1984 "6" "6" 0x00000100]
        Pad[   -3444 4921 
			 -2460 4921 
			984 1000 1984 "7" "7" 0x00000100]
        Pad[   -3444 6889 
			 -2460 6889 
			984 1000 1984 "8" "8" 0x00000100]
        Pad[   3444 6889 
			 2460 6889 
			984 1000 1984 "9" "9" 0x00000100]
        Pad[   3444 4921 
			 2460 4921 
			984 1000 1984 "10" "10" 0x00000100]
        Pad[   3444 2952 
			 2460 2952 
			984 1000 1984 "11" "11" 0x00000100]
        Pad[   3444 984 
			 2460 984 
			984 1000 1984 "12" "12" 0x00000100]
        Pad[   3444 -984 
			 2460 -984 
			984 1000 1984 "13" "13" 0x00000100]
        Pad[   3444 -2952 
			 2460 -2952 
			984 1000 1984 "14" "14" 0x00000100]
        Pad[   3444 -4921 
			 2460 -4921 
			984 1000 1984 "15" "15" 0x00000100]
        Pad[   3444 -6889 
			 2460 -6889 
			984 1000 1984 "16" "16" 0x00000100]
	ElementLine[-4937 -8381 -4937  8381 1000]
	ElementLine[-4937  8381  4937  8381 1000]
	ElementLine[ 4937  8381  4937 -8381 1000]
	ElementLine[-4937 -8381 -2500 -8381 1000]
	ElementLine[ 4937 -8381  2500 -8381 1000]
	# punt on the arc on small parts as it can cover the pads
)
