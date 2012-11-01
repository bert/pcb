(Created by G-code exporter)
(Fri Nov  2 00:03:23 2012)
(Units: inch)
(Board size: 2.00 x 1.00 inches)
(Drillmill file)
(Tool diameter: 1.000000 inch)
#100=2.000000  (safe Z)
#105=-1.000000  (mill depth)
#106=25.000000  (mill plunge feedrate)
#107=50.000000  (mill feedrate)
(---------------------------------)
G17 G20 G90 G64 P0.003 M3 S3000 M7
G0 X1.100000 Y0.500000
G1 Z#105 F#106
G0 Z#100
M5 M9 M2
