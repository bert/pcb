Element(0x00 "Plastic leadless chip carrier with pin socket" "" "PLCC20X" 350 350 0 100 0x00)
# PLCC - 44 is a  special case, pad 1 in inner row
(
# the default case, Pad 1 is on outer top row, in the middle
#top left row
Pin(350 150 62 35 "1" 0x101) 
	Pin(350 250 62 35 "2" 0x01) 
Pin(250 150 62 35 "3" 0x01)
# left row
Pin(150 250 62 35 "4" 0x01) 
	Pin(250 250 62 35 "5" 0x01) 
Pin(150 350 62 35 "6" 0x01) 
	Pin(250 350 62 35 "7" 0x01) 
Pin(150 450 62 35 "8" 0x01)
# bottom row
Pin(250 550 62 35 "9" 0x01) 
	Pin(250 450 62 35 "10" 0x01) 
Pin(350 550 62 35 "11" 0x01) 
	Pin(350 450 62 35 "12" 0x01) 
Pin(450 550 62 35 "13" 0x01)
# right row
Pin(550 450 62 35 "14" 0x01) 
	Pin(450 450 62 35 "15" 0x01) 
Pin(550 350 62 35 "16" 0x01) 
	Pin(450 350 62 35 "17" 0x01) 
Pin(550 250 62 35 "18" 0x01)
#top right row
Pin(450 150 62 35 "19" 0x01) 
	Pin(450 250 62 35 "20" 0x01) 
	ElementLine(0 0 700 0 20)
	ElementLine(700 0 700 700 20)
	ElementLine(700 700 0 700 20)
	ElementLine(0 700 0 0 20)
	ElementLine(0 100 100 0 10)
	ElementLine(300 0 350 50 10)
	ElementLine(350 50 400 0 10)
	Mark(350 150)
)
