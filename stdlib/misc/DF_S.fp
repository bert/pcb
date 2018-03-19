# author: Amand Tihon
# email: amand.tihon@alrj.org
# dist-license: GPLv2
# use-license: unlimited

Element["" "DFS Bridge rectifier" "BR000" "" 32500 22500 0 0 0 100 ""]
(
	Pad[-21500 -10000 -15000 -10000 4724 2000 6724 "1" "1" "square"]
	Pad[-21500 10000 -15000 10000 4724 2000 6724 "2" "2" "square"]
	Pad[15000 10000 21500 10000 4724 2000 6724 "3" "3" "square,edge2"]
	Pad[15000 -10000 21500 -10000 4724 2000 6724 "4" "4" "square,edge2"]
	ElementLine [9000 -10000 5000 -10000 1500]
	ElementLine [-12500 -17000 -12500 17000 1500]
	ElementLine [-12500 -17000 12500 -17000 1500]
	ElementLine [12500 17000 -12500 17000 1500]
	ElementLine [12500 -17000 12500 17000 1500]
	ElementLine [7000 8000 7000 12000 1500]
	ElementLine [-7426 -11386 -8575 -8615 1500]
	ElementLine [-7426 9114 -8575 11885 1500]
	ElementLine [9000 10000 5000 10000 1500]
	ElementArc [-9274 -12152 2000 2000 158 90 1500]
	ElementArc [-6727 -7849 2000 2000 338 90 1500]
	ElementArc [-9274 8348 2000 2000 158 90 1500]
	ElementArc [-6727 12651 2000 2000 338 90 1500]

	)

