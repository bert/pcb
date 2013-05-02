# line radius (LR) depicts offset to pads lines and pad "band width"
Element(0x00 "smd chip 805" "" "SMD_CHIP 805" 0 0 0 25 0x00)
(
        Pad(10 10 10 40 20      "" 0x100)
        Pad(70 10 70 40 20      "" 0x100)
        ElementLine( 0  0 80  0 5)
        ElementLine(80  0 80 50 5)
        ElementLine(80 50  0 50 5)
        ElementLine( 0 50  0  0 5)
        Mark(40 25)
)
