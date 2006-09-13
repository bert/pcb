#  BGA packages
#  A. G. Major, 2000

# $1: canonical name
# $2: name on PCB
# $3: value
# $4: number of pins along outer edge
# $5: number of rows
#
define(`PKG_BGA',
	`define(`XBLOCK', `$4')
	define(`YBLOCK', `$5')
	define(`PITCH', `50')
	define(`DIAMETER', `25')
	define(`NUMPINS', `eval(4*(XBLOCK-4)*YBLOCK)')
Element(0x00 "$1" "`$2'" "$3" 100 0 0 100 0x00)
(
	forloop(`i', 0, eval(XBLOCK-1),
		`forloop(`j', 0, eval(YBLOCK-1),
			`PAD(eval(i*PITCH), eval(j*PITCH), eval(i*PITCH), eval(j*PITCH), DIAMETER, 1)'
			`PAD(eval((XBLOCK+YBLOCK-j-1)*PITCH), eval(i*PITCH), eval((XBLOCK+YBLOCK-j-1)*PITCH), eval(i*PITCH), DIAMETER, 1)'
			`PAD(eval((XBLOCK+YBLOCK-i-1)*PITCH), eval((XBLOCK+YBLOCK-j-1)*PITCH), eval((XBLOCK+YBLOCK-i-1)*PITCH), eval((XBLOCK+YBLOCK-j-1)*PITCH), DIAMETER, 1)'
			`PAD(eval(j*PITCH), eval((XBLOCK+YBLOCK-i-1)*PITCH), eval(j*PITCH), eval((XBLOCK+YBLOCK-i-1)*PITCH), DIAMETER, 1)'
		)'
	)
	ElementLine(eval(-PITCH/2) eval(-1*PITCH) eval((XBLOCK+YBLOCK)*PITCH) eval(-1*PITCH) 10)
	ElementLine(eval((XBLOCK+YBLOCK)*PITCH) eval(-1*PITCH) eval((XBLOCK+YBLOCK)*PITCH) eval((XBLOCK+YBLOCK)*PITCH) 10)
	ElementLine(eval(-1*PITCH) eval((XBLOCK+YBLOCK)*PITCH) eval((XBLOCK+YBLOCK)*PITCH) eval((XBLOCK+YBLOCK)*PITCH) 10)
	ElementLine(eval(-1*PITCH) eval(-PITCH/2) eval(-1*PITCH) eval((XBLOCK+YBLOCK)*PITCH) 10)
	ElementLine(eval(-PITCH/2) eval(-PITCH) eval(-PITCH/2) eval(-PITCH/2) 10)
	ElementLine(eval(-PITCH) eval(-PITCH/2) eval(-PITCH/2) eval(-PITCH/2) 10)
	Mark(eval((YBLOCK+1)*PITCH) eval((YBLOCK+1)*PITCH))
)')

# -------------------------------------------------------------------
