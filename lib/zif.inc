define(`PKG_ZIF',
	`define(`NUMPINS', `$4')
	define(`CENTER', eval(NUMPINS*19))
Element(0x00 "$1" "`$2'" "$3" 100 CENTER 0 100 0x00)
(

define(`X', 0)
define(`count', 1)
forloop(`i', 1, NUMPINS,
	`PAD(X, 12, X, 97, 24, count)' `define(`count', incr(count))'
	`define(`X', eval(X+39))'
)
PAD(-124, -41, -124, -151, 82, eval(NUMPINS+1))
PAD(eval(X+124-39), -41, eval(X+124-39), -151, 82, eval(NUMPINS+2))
)')

# -------------------------------------------------------------------
