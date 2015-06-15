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
Element["" "VSSOP-8" "U?" "val" 0 0 -5500 -12000 0 100 ""]
(
	Pad[-9881 -2952 -6220 -2952 708 393 1102 "1" "1" "square"]
	Pad[-9881 -984 -6220 -984 708 393 1102 "2" "2" "square"]
	Pad[-9881 984 -6220 984 708 393 1102 "3" "3" "square"]
	Pad[-9881 2952 -6220 2952 708 393 1102 "4" "4" "square"]
	Pad[ 9881 2952 6220 2952 708 393 1102 "5" "5" "square"]
	Pad[ 9881 984 6220 984 708 393 1102 "6" "6" "square"]
	Pad[ 9881 -984 6220 -984 708 393 1102 "7" "7" "square"]
	Pad[ 9881 -2952 6220 -2952 708 393 1102 "8" "8" "square"]

# Pin 1 outline
	ElementLine[-10500 -3484 -10500 -2421 100]
	ElementLine[-10500 -3484 -5906 -3484 100]
	ElementLine[-5906 -2421 -10500 -2421 100]
	ElementLine[-5906 -2421 -5906 -3484 100]
# Pin 8 outline
	ElementLine[10500 -3484 10500 -2421 100]
	ElementLine[10500 -3484 5906 -3484 100]
	ElementLine[5906 -2421 10500 -2421 100]
	ElementLine[5906 -2421 5906 -3484 100]
# Pin 2 outline
	ElementLine[-10500 -1515 -10500 -452 100]
	ElementLine[-10500 -1515 -5906 -1515 100]
	ElementLine[-5906 -452 -10500 -452 100]
	ElementLine[-5906 -452 -5906 -1515 100]
# Pin 7 outline
	ElementLine[10500 -1515 10500 -452 100]
	ElementLine[10500 -1515 5906 -1515 100]
	ElementLine[5906 -452 10500 -452 100]
	ElementLine[5906 -452 5906 -1515 100]
# Pin 3 outline
	ElementLine[-10500 452 -10500 1515 100]
	ElementLine[-10500 452 -5906 452 100]
	ElementLine[-5906 1515 -10500 1515 100]
	ElementLine[-5906 1515 -5906 452 100]
# Pin 6 outline
	ElementLine[10500 452 10500 1515 100]
	ElementLine[10500 452 5906 452 100]
	ElementLine[5906 1515 10500 1515 100]
	ElementLine[5906 1515 5906 452 100]
# Pin 4 outline
	ElementLine[-10500 2421 -10500 3484 100]
	ElementLine[-10500 2421 -5906 2421 100]
	ElementLine[-5906 3484 -10500 3484 100]
	ElementLine[-5906 3484 -5906 2421 100]
# Pin 5 outline
	ElementLine[10500 2421 10500 3484 100]
	ElementLine[10500 2421 5906 2421 100]
	ElementLine[5906 3484 10500 3484 100]
	ElementLine[5906 3484 5906 2421 100]

	ElementArc[0 -5906 2690 2690 0 180 500]
	ElementLine[-5906 -5906 -2690 -5906 500]
	ElementLine[2690 -5906 5906 -5906 500]
	ElementLine[-5906 5906 5906 5906 500]
	ElementLine[5906 -5906 5906 5906 500]
	ElementLine[-5906 5906 -5906 -5906 500]
)

