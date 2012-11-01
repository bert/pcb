(Created by G-code exporter)
(Fri Nov  2 00:03:17 2012)
(Units: inch)
(Board size: 2.00 x 1.00 inches)
(Outline mill file)
(Tool diameter: 0.001000 inch)
#100=0.002000  (safe Z)
#105=-0.001000  (mill depth)
#106=0.025000  (mill plunge feedrate)
#107=0.050000  (mill feedrate)
(---------------------------------)
G17 G20 G90 G64 P0.003 M3 S3000 M7
G0 Z#100
G0 X2.000500 Y-0.000500
G1 Z#105 F#106
G1 X-0.000500 Y-0.000500 F#107
G1 X-0.000500 Y1.000500
G1 X2.000500 Y1.000500
G1 X2.000500 Y-0.000500
G0 Z#100
M5 M9 M2
(end, total distance G0 0.08 mm = 0.00 in)
(     total distance G1 152.58 mm = 6.01 in)
