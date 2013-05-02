# line radius (LR) depicts offset to pads lines and pad "band width"
Element(0x00 "smd chip 1210" "" "SMD_CHIP 1210" 0 0 0 25 0x00)
(
        Pad(10 10 10 90 20      "" 0x100)
        Pad(110 10 110 90 20      "" 0x100)
        ElementLine( 0  0 120  0 5)
        ElementLine(120  0 120 100 5)
        ElementLine(120 100  0 100 5)
        ElementLine( 0 100  0  0 5)
        Mark(60 50)
)
