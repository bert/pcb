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
Element[0x00000000 "Panasonic EXB Series Chip Resistor Array" "" "PANASONIC_EXB14V" 0 0 -2000 -6000 0 100 0x00000000]
(
# 
# Pad[x1, y1, x2, y2, thickness, clearance, mask, name , pad number, flags]
        Pad[   -1181 -984 
			 -1181 -984 
			1181 1000 2181 "1" "1" 0x00000100]
        Pad[   -1181 984 
			 -1181 984 
			1181 1000 2181 "2" "2" 0x00000100]
        Pad[   1181 984 
			 1181 984 
			1181 1000 2181 "3" "3" 0x00000100]
        Pad[   1181 -984 
			 1181 -984 
			1181 1000 2181 "4" "4" 0x00000100]
	ElementLine[-2771 -2574 -2771  2574 1000]
	ElementLine[-2771  2574  2771  2574 1000]
	ElementLine[ 2771  2574  2771 -2574 1000]
	ElementLine[-2771 -2574 -2500 -2574 1000]
	ElementLine[ 2771 -2574  2500 -2574 1000]
	# punt on the arc on small parts as it can cover the pads
)
