(Created by G-code exporter)
(Fri Nov 23 00:31:44 2012)
(Units: inch)
(Board size: 2.00 x 1.00 inches)
(Drillmill file)
(Tool diameter: 0.001000 inch)
#100=0.002000  (safe Z)
#105=-0.001000  (mill depth)
#106=0.025000  (mill plunge feedrate)
#107=0.050000  (mill feedrate)
(---------------------------------)
G17 G20 G90 G64 P0.003 M3 S3000 M7
G0 X1.100000 Y0.500000
G1 Z#105 F#106
F#107
G1 X1.117000 Y0.500000
G1 X1.110599 Y0.513291
G1 X1.096217 Y0.516574
G1 X1.084684 Y0.507376
G1 X1.084684 Y0.492624
G1 X1.096217 Y0.483426
G1 X1.110599 Y0.486709
G1 X1.117000 Y0.500000
G0 X1.100000 Y0.500000
G0 Z#100
M5 M9 M2
