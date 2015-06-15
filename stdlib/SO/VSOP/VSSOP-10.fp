# units: MM
# BL = 3.1
# BW = 3.1
# C = .05
# CW = 5
# E = .5
# G = 4.1
# LL = 1
# LW = .27
# M = .05
# PG = .23
# PL = 1.2
# PLC = 4
# PLE = .2
# PW = .18
# SOC = .127
# SW = .127
Element["" "VSSOP-10" "U?" "val" 0 0 -5500 -12000 0 100 ""]
(
	Pad[-9881 -3937 -6220 -3937 708 393 1102  "1"  "1" "square"]
	Pad[-9881 -1968 -6220 -1968 708 393 1102  "2"  "2" "square"]
	Pad[-9881     0 -6220     0 708 393 1102  "3"  "3" "square"]
	Pad[-9881  1968 -6220  1968 708 393 1102  "4"  "4" "square"]
	Pad[-9881  3937 -6220  3937 708 393 1102  "5"  "5" "square"]
	Pad[ 9881  3937  6220  3937 708 393 1102  "6"  "6" "square"]
	Pad[ 9881  1968  6220  1968 708 393 1102  "7"  "7" "square"]
	Pad[ 9881     0  6220     0 708 393 1102  "8"  "8" "square"]
	Pad[ 9881 -1968  6220 -1968 708 393 1102  "9"  "9" "square"]
	Pad[ 9881 -3937  6220 -3937 708 393 1102 "10" "10" "square"]

# Pin 1 outline
	ElementLine[-10500 -4468 -10500 -3405 100]
	ElementLine[-10500 -4468  -5906 -4468 100]
	ElementLine[ -5906 -3405 -10500 -3405 100]
	ElementLine[ -5906 -3405  -5906 -4468 100]
# Pin 10 outline
	ElementLine[ 10500 -4468  10500 -3405 100]
	ElementLine[ 10500 -4468   5906 -4468 100]
	ElementLine[  5906 -3405  10500 -3405 100]
	ElementLine[  5906 -3405   5906 -4468 100]
# Pin 2 outline
	ElementLine[-10500 -2500 -10500 -1437 100]
	ElementLine[-10500 -2500  -5906 -2500 100]
	ElementLine[ -5906 -1437 -10500 -1437 100]
	ElementLine[ -5906 -1437  -5906 -2500 100]
# Pin 9 outline
	ElementLine[ 10500 -2500  10500 -1437 100]
	ElementLine[ 10500 -2500   5906 -2500 100]
	ElementLine[  5906 -1437  10500 -1437 100]
	ElementLine[  5906 -1437   5906 -2500 100]
# Pin 3 outline
	ElementLine[-10500  -531 -10500   531 100]
	ElementLine[-10500  -531  -5906  -531 100]
	ElementLine[ -5906   531 -10500   531 100]
	ElementLine[ -5906   531  -5906  -531 100]
# Pin 8 outline
	ElementLine[ 10500  -531  10500   531 100]
	ElementLine[ 10500  -531   5906  -531 100]
	ElementLine[  5906   531  10500   531 100]
	ElementLine[  5906   531   5906  -531 100]
# Pin 4 outline
	ElementLine[-10500  1437 -10500  2500 100]
	ElementLine[-10500  1437  -5906  1437 100]
	ElementLine[ -5906  2500 -10500  2500 100]
	ElementLine[ -5906  2500  -5906  1437 100]
# Pin 7 outline
	ElementLine[ 10500  1437  10500  2500 100]
	ElementLine[ 10500  1437   5906  1437 100]
	ElementLine[  5906  2500  10500  2500 100]
	ElementLine[  5906  2500   5906  1437 100]
# Pin 5 outline
	ElementLine[-10500  3405 -10500  4468 100]
	ElementLine[-10500  3405  -5906  3405 100]
	ElementLine[ -5906  4468 -10500  4468 100]
	ElementLine[ -5906  4468  -5906  3405 100]
# Pin 6 outline
	ElementLine[ 10500  3405  10500  4468 100]
	ElementLine[ 10500  3405   5906  3405 100]
	ElementLine[  5906  4468  10500  4468 100]
	ElementLine[  5906  4468   5906  3405 100]

# Body Outline
	ElementArc [0 -5904 2690 2690 0 180 300]
	ElementLine[-5906 -5906 -2690 -5906 300]
	ElementLine[ 2690 -5906  5906 -5906 300]
	ElementLine[-5906  5906  5906  5906 300]
 	ElementLine[ 5906 -5906  5906  5906 300]
 	ElementLine[-5906  5906 -5906 -5906 300]
)

