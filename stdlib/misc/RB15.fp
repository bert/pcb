# author: Amand Tihon
# email: amand.tihon@alrj.org
# dist-license: GPL3, http://www.gnu.org/licenses/gpl-3.0.txt
# use-license: unlimited


Element["" "Diode bridge, RB-15" "BR000" "" 80000 50000 -10000 20000 0
100 ""]
(
	Pin[-10000 -10000 8000 2000 8600 1500 "1" "1" ""]
	Pin[-10000 10000 8000 2000 8600 1500 "3" "3" ""]
	Pin[10000 -10000 8000 2000 8600 1500 "4" "4" ""]
	Pin[10000 10000 8000 2000 8600 1500 "2" "2" ""]
	ElementLine [-7000 5000 -3000 5000 1500]
	ElementLine [3000 -5000 7000 -5000 1500]
	ElementLine [5000 -7000 5000 -3000 1500]
	ElementArc [-6000 -5000 1000 1000 270 90 1500]
	ElementArc [-6000 -5000 1000 1000 180 90 1500]
	ElementArc [-4000 -5000 1000 1000 0 90 1500]
	ElementArc [-4000 -5000 1000 1000 90 90 1500]
	ElementArc [4000 5000 1000 1000 270 90 1500]
	ElementArc [4000 5000 1000 1000 180 90 1500]
	ElementArc [6000 5000 1000 1000 0 90 1500]
	ElementArc [6000 5000 1000 1000 90 90 1500]
	ElementArc [0 0 18000 18000 0 360 1500]

	)
