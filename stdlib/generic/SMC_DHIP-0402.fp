# line radius (LR) depicts offset to pads lines and pad "band width"
Element(0x00 "smd chip 402" "" "SMD_CHIP 402" 0 0 0 25 0x00)
(
        Pad(5 5 5 15 10      "" 0x100)
        Pad(35 5 35 15 10      "" 0x100)
        ElementLine( 0  0 40  0 5)
        ElementLine(40  0 40 20 5)
        ElementLine(40 20  0 20 5)
        ElementLine( 0 20  0  0 5)
        Mark(20 10)
)
