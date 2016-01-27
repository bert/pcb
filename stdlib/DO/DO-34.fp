# description: DO-34 diode
# dist-license: GPL 3
# use-license: unlimited
Element["" "" "" "" 18124 4593 0 0 2 100 ""]
(
	Pin[ 11000 -207 7200 2000 8400 2900 "" "1" "square,edge2"]
	Pin[-11000 -207 7200 2000 8400 2900 "" "2" "edge2"]

#bottom
	ElementLine [-5100  2893  5100  2893 1000]

#left
	ElementLine [-5100 -3307 -5100  2893 1000]

#top
	ElementLine [-5100 -3307  5100 -3307 1000]

#right
	ElementLine [ 5100 -3307  5100  2893 1000]

#band
	ElementLine [ 3200 -3307 3200 2893 1000]

#right lead
	ElementLine [  5100 -207 7200 -207 1000]

#left lead
	ElementLine [-7200 -207 -5100 -207 1000]
)
