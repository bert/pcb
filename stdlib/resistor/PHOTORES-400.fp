Element(0x00 "Generic photo-resistor, 400 mil lead pitch" "R" "" 185 -200 0 150 0x00)
(
	Pin(-200 0 55 30 "1" 0x101)
	Pin( 200 0 55 30 "2" 0x01)

	ElementLine(-100 100 -100 -100 10)
	ElementLine( -50 100  -50 -100 10)
	ElementLine(  50 100   50 -100 10)
	ElementLine( 100 100  100 -100 10)

	ElementArc(0 0  200  200 30 120 10)
	ElementArc(0 0 -200 -400 30 120 10)
)
