(Created by G-code exporter)
(Fri Nov  2 00:03:23 2012)
(Units: inch)
(Board size: 2.00 x 1.00 inches)
(Outline mill file)
(Tool diameter: 1.000000 inch)
#100=2.000000  (safe Z)
#105=-1.000000  (mill depth)
#106=25.000000  (mill plunge feedrate)
#107=50.000000  (mill feedrate)
(---------------------------------)
G17 G20 G90 G64 P0.003 M3 S3000 M7
G0 Z#100
G0 X2.500000 Y-0.500000
G1 Z#105 F#106
G1 X-0.500000 Y-0.500000 F#107
G1 X-0.500000 Y1.500000
G1 X2.500000 Y1.500000
G1 X2.500000 Y-0.500000
G0 Z#100
M5 M9 M2
(end, total distance G0 76.20 mm = 3.00 in)
(     total distance G1 330.20 mm = 13.00 in)
