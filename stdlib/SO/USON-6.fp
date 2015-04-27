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
Element[0x00 "Micro Small outline package" "" "US8" 0 0 -2000 -6000 0 100 0x000]
(
    Pad[ -5200 -2000 -3800 -2000 1200 1000 2180 "1" "1" 0x100]
    Pad[ -5200     0 -3800     0 1200 1000 2180 "2" "2" 0x100]
    Pad[ -5200  2000 -3800  2000 1200 1000 2180 "3" "3" 0x100]

    Pad[  5200  2000  3800  2000 1200 1000 2180 "4" "4" 0x100]
    Pad[  5200     0  3800     0 1200 1000 2180 "5" "5" 0x100]
    Pad[  5200 -2000  3800 -2000 1200 1000 2180 "6" "6" 0x100]

    ElementLine[-6500 -4000 -6500  4000 500]
    ElementLine[-6500  4000  6500  4000 500]
    ElementLine[ 6500  4000  6500 -4000 500]
    ElementLine[-6500 -4000  6500 -4000 500]
#   ElementLine[ 6500 -4000  2500 -4000 500]
)
