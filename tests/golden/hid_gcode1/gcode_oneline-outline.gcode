(Created by G-code exporter)
(Fri Nov  2 00:02:36 2012)
(Units: mm)
(Board size: 50.80 x 25.40 mm)
(Outline mill file)
(Tool diameter: 1.000000 mm)
#100=2.000000  (safe Z)
#105=-1.000000  (mill depth)
#106=25.000000  (mill plunge feedrate)
#107=50.000000  (mill feedrate)
(---------------------------------)
G17 G21 G90 G64 P0.003 M3 S3000 M7
G0 Z#100
G0 X51.300000 Y-0.500000
G1 Z#105 F#106
G1 X-0.500000 Y-0.500000 F#107
G1 X-0.500000 Y25.900000
G1 X51.300000 Y25.900000
G1 X51.300000 Y-0.500000
G0 Z#100
M5 M9 M2
(end, total distance G0 3.00 mm = 0.12 in)
(     total distance G1 159.40 mm = 6.28 in)
