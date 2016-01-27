	Element(0x00 "Hermetic Metal Transistor Package," "Q" "TO254" 10 -120 -100 200 0x00)
(
# The pin pitch is 90 to 110 mils.
#
# The mounting hole is 139 to 160 mils diameter
	Pin(120 825 90 60 "1" 0x101)
	Pin(270 825 90 60 "2" 0x01)
	Pin(420 825 90 60 "3" 0x01)

	# Befestigungsbohrung
	Pin(275 130 150 130 "4" 0x01)

	# Anschlussdraehte
	ElementLine(120 750 120 630 25)
	ElementLine(270 750 270 630 25)
	ElementLine(420 750 420 630 25)
	# Gehaeuse
	ElementLine(  0 620 550 620 20)
	ElementLine(550 620 550 245 20)
	ElementLine(550 245   0 245 20)
	ElementLine(  0 245   0 620 20)
	# Kuehlfahne mit Kerben
	ElementLine(  0 245 550 245 20)
	ElementLine(550 245 550  10 20)
	ElementLine(550  10   0  10 20)
	ElementLine(  0  10   0 245 20)
	Mark(120 825)
)
