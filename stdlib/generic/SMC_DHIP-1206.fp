# line radius (LR) depicts offset to pads lines and pad "band width"
Element(0x00 "smd chip 1206" "" "SMD_CHIP 1206" 0 0 0 25 0x00)
(
        Pad(10 10 10 50 20      "" 0x100)
        Pad(110 10 110 50 20      "" 0x100)
        ElementLine( 0  0 120  0 5)
        ElementLine(120  0 120 60 5)
        ElementLine(120 60  0 60 5)
        ElementLine( 0 60  0  0 5)
        Mark(60 30)
)
