# Convert micrometers to mils ( thousandths of an inch ) 
define(`UM_TO_MIL',`eval(($1*10)/254)')
# the definition of a general axial package
# $1: canonical name
# $2: name on PCB
# $3: length, width,padlength
define(`PKG_GENERIC_SMT',`
 	define(`cnt',`1')
	define(`ARGS',$3)	
	args(ARGS)
	define(`LENGTH',`UM_TO_MIL(arg1)')
 	define(`WIDTH',`UM_TO_MIL(arg2)')
	define(`PADLENGTH',`UM_TO_MIL(arg3)')
   	define(`CENTRE',`eval(LENGTH/2)')
	define(`PENWIDTH',`eval(PADLENGTH)')
	define(`PADLENGTH2',`eval(LENGTH-PADLENGTH)')
	define(`PADCENTRE',`eval(PADLENGTH/2)')
	define(`PADCENTRE2',`eval(LENGTH-(PADLENGTH/2))')
Element(0x00 "$1" "$2" "$3" CENTRE eval(WIDTH+10) 0 100 0x00)
(
 	PAD(0,0,0,WIDTH,PENWIDTH, 1)
	PAD(0,0,PADLENGTH,0,PENWIDTH,1)
	PAD(PADLENGTH,0,PADLENGTH,WIDTH,PENWIDTH, 1)
	PAD(0,WIDTH,PADLENGTH,WIDTH,PENWIDTH, 1)
	PAD(PADCENTRE,0,PADCENTRE,WIDTH,PENWIDTH,1)


	PAD(LENGTH,0,LENGTH,WIDTH,PENWIDTH, 1)
	PAD(LENGTH,0,PADLENGTH2,0,PENWIDTH,1)
	PAD(PADLENGTH2,0,PADLENGTH2,WIDTH,PENWIDTH, 1)
	PAD(LENGTH,WIDTH,PADLENGTH2,WIDTH,PENWIDTH, 1)
	PAD(PADCENTRE2,0,PADCENTRE2,WIDTH,PENWIDTH,1)

        ElementLine(0 0 LENGTH 0 1)
	ElementLine(LENGTH 0 LENGTH WIDTH 1)
	ElementLine(0 WIDTH LENGTH WIDTH 1)
	ElementLine(0 0 0 WIDTH 1)
        	
)
')


