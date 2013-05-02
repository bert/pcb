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
Element[0x00000000 "CTS 742C Series Chip Resistor Array" "" "CTS_742C_16" 0 0 -2000 -6000 0 100 0x00000000]
(
# 
# Pad[x1, y1, x2, y2, thickness, clearance, mask, name , pad number, flags]
        Pad[   -4330 -11023 
			 -2755 -11023 
			1968 1000 2968 "1" "1" 0x00000100]
        Pad[   -4330 -7874 
			 -2755 -7874 
			1968 1000 2968 "2" "2" 0x00000100]
        Pad[   -4330 -4724 
			 -2755 -4724 
			1968 1000 2968 "3" "3" 0x00000100]
        Pad[   -4330 -1574 
			 -2755 -1574 
			1968 1000 2968 "4" "4" 0x00000100]
        Pad[   -4330 1574 
			 -2755 1574 
			1968 1000 2968 "5" "5" 0x00000100]
        Pad[   -4330 4724 
			 -2755 4724 
			1968 1000 2968 "6" "6" 0x00000100]
        Pad[   -4330 7874 
			 -2755 7874 
			1968 1000 2968 "7" "7" 0x00000100]
        Pad[   -4330 11023 
			 -2755 11023 
			1968 1000 2968 "8" "8" 0x00000100]
        Pad[   4330 11023 
			 2755 11023 
			1968 1000 2968 "9" "9" 0x00000100]
        Pad[   4330 7874 
			 2755 7874 
			1968 1000 2968 "10" "10" 0x00000100]
        Pad[   4330 4724 
			 2755 4724 
			1968 1000 2968 "11" "11" 0x00000100]
        Pad[   4330 1574 
			 2755 1574 
			1968 1000 2968 "12" "12" 0x00000100]
        Pad[   4330 -1574 
			 2755 -1574 
			1968 1000 2968 "13" "13" 0x00000100]
        Pad[   4330 -4724 
			 2755 -4724 
			1968 1000 2968 "14" "14" 0x00000100]
        Pad[   4330 -7874 
			 2755 -7874 
			1968 1000 2968 "15" "15" 0x00000100]
        Pad[   4330 -11023 
			 2755 -11023 
			1968 1000 2968 "16" "16" 0x00000100]
	ElementLine[-6314 -13007 -6314  13007 1000]
	ElementLine[-6314  13007  6314  13007 1000]
	ElementLine[ 6314  13007  6314 -13007 1000]
	ElementLine[-6314 -13007 -2500 -13007 1000]
	ElementLine[ 6314 -13007  2500 -13007 1000]
	# punt on the arc on small parts as it can cover the pads
)
