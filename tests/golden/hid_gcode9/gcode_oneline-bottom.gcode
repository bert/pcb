(Created by G-code exporter)
(Fri Nov  2 00:03:17 2012)
(Units: inch)
(Board size: 2.00 x 1.00 inches)
(Accuracy 600 dpi)
(Tool diameter: 0.000200 inch)
#100=0.002000  (safe Z)
#101=-0.000050  (cutting depth)
#102=0.025000  (plunge feedrate)
#103=0.050000  (feedrate)
(with predrilling)
(---------------------------------)
G17 G20 G90 G64 P0.003 M3 S3000 M7
G0 Z#100
(polygon 1)
G0 X1.093333 Y0.530000    (start point)
G1 Z#101 F#102
F#103
G1 X1.083333 Y0.525000
G1 X1.076667 Y0.520000
G1 X0.291667 Y0.518333
G1 X0.280000 Y0.506667
G1 X0.280000 Y0.491667
G1 X0.291667 Y0.480000
G1 X1.076667 Y0.478333
G1 X1.088333 Y0.470000
G1 X1.100000 Y0.468333
G1 X1.113333 Y0.471667
G1 X1.123333 Y0.480000
G1 X1.130000 Y0.493333
G1 X1.130000 Y0.505000
G1 X1.123333 Y0.518333
G1 X1.115000 Y0.525000
G1 X1.103333 Y0.530000
G1 X1.093333 Y0.530000
G0 Z#100
(polygon end, distance 1.77)
(predrilling)
F#102
(0 predrills)
(milling distance 44.84mm = 1.77in)
M5 M9 M2
