	Element(0x00 "Transistor" "" "TO126" 80 480 1 100 0x00)
(
# From the JEDEC drawing, the pins are rectangular with dimensions
# 25-35 mil X 15-25 mil
#
# This gives a diagonal dimension of 29.2 to 43.0 mils.
# Pin pitch is 80 to 100 mils.
# 
# For a minimum clearance of 10 mils (probably not unreasonable if
# you are doing a design with leaded parts, this gives a max pad size
# of 80 mils.  A 52 mil drill will give 14 mil annular ring which should
# be plenty.
#
# The mounting hole is 100 to 130 mils diameter
	Pin(110 600 80 52 "1" 0x101)
	Pin(200 600 80 52 "2" 0x01)
	Pin(290 600 80 52 "3" 0x01) 
	# Befestigungsbohrung
	Pin(200 170 130 110 "4" 0x01)
	# Anschlussdraehte
	ElementLine(100 600 100 500 30)
	ElementLine(200 600 200 500 30)
	ElementLine(300 600 300 500 30)
	# Gehaeuse
	ElementLine( 50 500 350 500 20)
	ElementLine(350 500 350  70 20)
	ElementLine(350  70  50  70 20)
	ElementLine( 50  70  50 500 20)
	Mark(100 600)
)
