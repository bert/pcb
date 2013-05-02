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
Element[0x00000000 "Ultra (Micro?) Small outline package" "" "US16" 0 0 -2000 -6000 0 100 0x00000000]
(
# 
# Pad[x1, y1, x2, y2, thickness, clearance, mask, name , pad number, flags]
        Pad[   -6102 -6889 
			 -4527 -6889 
			1181 1000 2181 "1" "1" 0x00000100]
        Pad[   -6102 -4921 
			 -4527 -4921 
			1181 1000 2181 "2" "2" 0x00000100]
        Pad[   -6102 -2952 
			 -4527 -2952 
			1181 1000 2181 "3" "3" 0x00000100]
        Pad[   -6102 -984 
			 -4527 -984 
			1181 1000 2181 "4" "4" 0x00000100]
        Pad[   -6102 984 
			 -4527 984 
			1181 1000 2181 "5" "5" 0x00000100]
        Pad[   -6102 2952 
			 -4527 2952 
			1181 1000 2181 "6" "6" 0x00000100]
        Pad[   -6102 4921 
			 -4527 4921 
			1181 1000 2181 "7" "7" 0x00000100]
        Pad[   -6102 6889 
			 -4527 6889 
			1181 1000 2181 "8" "8" 0x00000100]
        Pad[   6102 6889 
			 4527 6889 
			1181 1000 2181 "9" "9" 0x00000100]
        Pad[   6102 4921 
			 4527 4921 
			1181 1000 2181 "10" "10" 0x00000100]
        Pad[   6102 2952 
			 4527 2952 
			1181 1000 2181 "11" "11" 0x00000100]
        Pad[   6102 984 
			 4527 984 
			1181 1000 2181 "12" "12" 0x00000100]
        Pad[   6102 -984 
			 4527 -984 
			1181 1000 2181 "13" "13" 0x00000100]
        Pad[   6102 -2952 
			 4527 -2952 
			1181 1000 2181 "14" "14" 0x00000100]
        Pad[   6102 -4921 
			 4527 -4921 
			1181 1000 2181 "15" "15" 0x00000100]
        Pad[   6102 -6889 
			 4527 -6889 
			1181 1000 2181 "16" "16" 0x00000100]
	ElementLine[-7692 -8480 -7692  8480 1000]
	ElementLine[-7692  8480  7692  8480 1000]
	ElementLine[ 7692  8480  7692 -8480 1000]
	ElementLine[-7692 -8480 -2500 -8480 1000]
	ElementLine[ 7692 -8480  2500 -8480 1000]
	# punt on the arc on small parts as it can cover the pads
	ElementArc[0 -8480 2500 2500 0 180 1000]
)
