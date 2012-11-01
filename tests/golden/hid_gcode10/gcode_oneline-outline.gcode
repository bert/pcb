(Created by G-code exporter)
(Fri Nov  2 00:03:20 2012)
(Units: mm)
(Board size: 50.80 x 25.40 mm)
(Outline mill file)
(Tool diameter: 0.001000 mm)
#100=0.002000  (safe Z)
#105=-0.001000  (mill depth)
#106=0.025000  (mill plunge feedrate)
#107=0.050000  (mill feedrate)
(---------------------------------)
G17 G21 G90 G64 P0.003 M3 S3000 M7
G0 Z#100
G0 X50.800500 Y-0.000500
G1 Z#105 F#106
G1 X-0.000500 Y-0.000500 F#107
G1 X-0.000500 Y25.400500
G1 X50.800500 Y25.400500
G1 X50.800500 Y-0.000500
G0 Z#100
M5 M9 M2
(end, total distance G0 0.00 mm = 0.00 in)
(     total distance G1 152.41 mm = 6.00 in)
