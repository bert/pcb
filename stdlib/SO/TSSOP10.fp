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
Element[0x00000000 "Thin shrink small outline package, narrow (3mm)" "" "TSSOP10" 0 0 -2000 -6000 0 100 0x00000000]
(
# 
# Pad[x1, y1, x2, y2, thickness, clearance, mask, name , pad number, flags]
        Pad[   -11220 -3937 
			 -6496 -3937 
			1181 1000 2181 "1" "1" 0x00000100]
        Pad[   -11220 -1968 
			 -6496 -1968 
			1181 1000 2181 "2" "2" 0x00000100]
        Pad[   -11220 0 
			 -6496 0 
			1181 1000 2181 "3" "3" 0x00000100]
        Pad[   -11220 1968 
			 -6496 1968 
			1181 1000 2181 "4" "4" 0x00000100]
        Pad[   -11220 3937 
			 -6496 3937 
			1181 1000 2181 "5" "5" 0x00000100]
        Pad[   11220 3937 
			 6496 3937 
			1181 1000 2181 "6" "6" 0x00000100]
        Pad[   11220 1968 
			 6496 1968 
			1181 1000 2181 "7" "7" 0x00000100]
        Pad[   11220 0 
			 6496 0 
			1181 1000 2181 "8" "8" 0x00000100]
        Pad[   11220 -1968 
			 6496 -1968 
			1181 1000 2181 "9" "9" 0x00000100]
        Pad[   11220 -3937 
			 6496 -3937 
			1181 1000 2181 "10" "10" 0x00000100]
	ElementLine[-12811 -5527 -12811  5527 1000]
	ElementLine[-12811  5527  12811  5527 1000]
	ElementLine[ 12811  5527  12811 -5527 1000]
	ElementLine[-12811 -5527 -2500 -5527 1000]
	ElementLine[ 12811 -5527  2500 -5527 1000]
	# punt on the arc on small parts as it can cover the pads
	ElementArc[0 -5527 2500 2500 0 180 1000]
)
