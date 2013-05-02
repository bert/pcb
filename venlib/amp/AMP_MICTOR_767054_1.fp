	# number of pads
	# number of segments of 38 pins each
	# pad width in 1/1000 mil
	# pad length in 1/1000 mil
	# pad pitch 1/1000 mil
	# seperation between pads on opposite sides 1/1000 mil
	# X coordinates for the right hand column of pads (mils)
	# silk screen width (mils)
	# figure out if we have an even or odd number of pins per side
	# silk bounding box is -XMAX,-YMAX, XMAX,YMAX (mils)
Element(0x00 "Amp Mictor Connectors" "" "AMP_MICTOR_767054_1" -20 -60 0 100 0x00)
(
                Pad(   -180 -225 
			 -147 -225 
			17 "1" "1" 0x0)
                Pad(   -180 -200 
			 -147 -200 
			17 "3" "3" 0x0)
                Pad(   -180 -175 
			 -147 -175 
			17 "5" "5" 0x0)
                Pad(   -180 -150 
			 -147 -150 
			17 "7" "7" 0x0)
                Pad(   -180 -125 
			 -147 -125 
			17 "9" "9" 0x0)
                Pad(   -180 -100 
			 -147 -100 
			17 "11" "11" 0x0)
                Pad(   -180 -75 
			 -147 -75 
			17 "13" "13" 0x0)
                Pad(   -180 -50 
			 -147 -50 
			17 "15" "15" 0x0)
                Pad(   -180 -25 
			 -147 -25 
			17 "17" "17" 0x0)
                Pad(   -180 0 
			 -147 0 
			17 "19" "19" 0x0)
                Pad(   -180 25 
			 -147 25 
			17 "21" "21" 0x0)
                Pad(   -180 50 
			 -147 50 
			17 "23" "23" 0x0)
                Pad(   -180 75 
			 -147 75 
			17 "25" "25" 0x0)
                Pad(   -180 100 
			 -147 100 
			17 "27" "27" 0x0)
                Pad(   -180 125 
			 -147 125 
			17 "29" "29" 0x0)
                Pad(   -180 150 
			 -147 150 
			17 "31" "31" 0x0)
                Pad(   -180 175 
			 -147 175 
			17 "33" "33" 0x0)
                Pad(   -180 200 
			 -147 200 
			17 "35" "35" 0x0)
                Pad(   -180 225 
			 -147 225 
			17 "37" "37" 0x0)
                Pad(   180 225 
			 147 225 
			17 "38" "38" 0x0)
                Pad(   180 200 
			 147 200 
			17 "36" "36" 0x0)
                Pad(   180 175 
			 147 175 
			17 "34" "34" 0x0)
                Pad(   180 150 
			 147 150 
			17 "32" "32" 0x0)
                Pad(   180 125 
			 147 125 
			17 "30" "30" 0x0)
                Pad(   180 100 
			 147 100 
			17 "28" "28" 0x0)
                Pad(   180 75 
			 147 75 
			17 "26" "26" 0x0)
                Pad(   180 50 
			 147 50 
			17 "24" "24" 0x0)
                Pad(   180 25 
			 147 25 
			17 "22" "22" 0x0)
                Pad(   180 0 
			 147 0 
			17 "20" "20" 0x0)
                Pad(   180 -25 
			 147 -25 
			17 "18" "18" 0x0)
                Pad(   180 -50 
			 147 -50 
			17 "16" "16" 0x0)
                Pad(   180 -75 
			 147 -75 
			17 "14" "14" 0x0)
                Pad(   180 -100 
			 147 -100 
			17 "12" "12" 0x0)
                Pad(   180 -125 
			 147 -125 
			17 "10" "10" 0x0)
                Pad(   180 -150 
			 147 -150 
			17 "8" "8" 0x0)
                Pad(   180 -175 
			 147 -175 
			17 "6" "6" 0x0)
                Pad(   180 -200 
			 147 -200 
			17 "4" "4" 0x0)
                Pad(   180 -225 
			 147 -225 
			17 "2" "2" 0x0)
# now add the center row of grounding pins
	Pin(0 -200 60 32 "GND" "39" 0x01)
	Pin(0 -100 60 32 "GND" "40" 0x01)
	Pin(0 0 60 32 "GND" "41" 0x01)
	Pin(0 100 60 32 "GND" "42" 0x01)
	Pin(0 200 60 32 "GND" "43" 0x01)
# the latch pins
	Pin(0 -555 80 53 "LATCH" "44" 0x01)
	Pin(0  555 80 53 "LATCH" "45" 0x01)
# and the orientation pin
	Pin(0 -450 84 84 "ORIENT" "46" 0x09)
# and finally the silk screen
	ElementLine(-200 -625 -200  625 10)
	ElementLine(-200  625  200  625 10)
	ElementLine( 200  625  200 -625 10)
	ElementLine(-200 -625   -25 -625 10)
	ElementLine( 200 -625    25 -625 10)
	# punt on the arc on small parts as it can cover the pads
	ElementArc(0 -625 25 25 0 180 10)
	# Mark at the common centroid
        Mark(0 0)
)
