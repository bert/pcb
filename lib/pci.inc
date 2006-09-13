# $Id$
#
# Definitions for PCI boards
# by D.J. Barrow dj_barrow@ariasoft.ie
#
# please note that you will need to edit the saved pcb file
# by hand to move the board outline onto a seperate layer
# I suggest one of the unused layers.

define(`GETNUMARRAYS',`eval(($#)/2)')
# $1: canonical name
# $2: name on PCB
# $3: boardheight,number of pins,arcradius,..., number of pins
define(`PKG_PCIPINARRAY',
	`
        define(`cnt',`1')
        define(`ARGS',`$4, $5, $6, $7')
	args(ARGS)
	define(`LEFTOFFSET',`100')
	define(`EDGEGAP',`12')
	define(`PINDIST',`50')
    	define(`PINWIDTH',`25')
	define(`NUMARRAYS',`GETNUMARRAYS(ARGS)')
        define(`TOTALPINS1',forloop(`IDX',0,eval(NUMARRAYS-1),
		``arg'eval((IDX*2)+2) + ')0)
	define(`TOTALPINS',eval(TOTALPINS1))
	ifelse(1,eval(NUMARRAYS>1),`
		define(`TOTALDIAMETER1',forloop(`IDX',0,eval(NUMARRAYS-2),
			``arg'eval((IDX*2)+3) + ')0)
		define(`TOTALDIAMETER',eval(TOTALDIAMETER1))
	')
    	define(`PINCENTER',`eval(LEFTOFFSET+(PINDIST/2))')
    	define(`PINRADIUS',`eval(PINWIDTH/2)')
    	define(`PINLEFT',`eval(PINCENTER-PINRADIUS)')
    	define(`PINRIGHT',`eval(PINCENTER+PINRADIUS)')
    	define(`PENWIDTH',`eval(PINRADIUS+4)')
    	define(`PENRADIUS',`eval(PENWIDTH/2)')
    	define(`MAXIDX',`eval(TOTALPINS - 1)')
    	define(`ARRAYWIDTH', `eval((TOTALPINS*PINDIST)+TOTALDIAMETER)')
    	define(`CENTERX', `eval(((ARRAYWIDTH+LEFTOFFSET)/2))')
    	define(`MINY',`20')
    	define(`MIDY',`220')
    	define(`MAXY',`270')
    	define(`ARCRADIUS',`12')
	define(`ARCDIAMETER',`eval(ARCRADIUS*2)')
    	define(`LMAXY',`eval(MAXY+ARCRADIUS)')
	define(`BOARDTOP',`eval(LMAXY-arg1)')
	define(`SCREWBORDER',275)
Element(0x00  "$1" "`$2'" "$3" CENTERX 10 1 30 0x00)
(
	Mark(CENTERX 135)
	define(`ADDSIDE',0)
	define(`FLAG',0x0)
	define(`LOOPIDX1',eval(NUMARRAYS-1))
	define(`LEFT1',`eval(PINLEFT-ARCRADIUS)')
	define(`ARCRADIUS2',`100')
	define(`SLOTWIDTH',`1000')
	define(`BRACKETLEFT',`-1525')
	define(`LEFT2',`eval(LEFT1-ARCRADIUS2)')
	define(`LEFT3',`eval(LEFT1+ARCRADIUS2-SLOTWIDTH)')
	define(`LEFT4',`eval(LEFT1-SLOTWIDTH)')
	ElementArc(LEFT2 0 ARCRADIUS2 ARCRADIUS2 180 90 1)
  	ElementLine(LEFT2 -ARCRADIUS2 LEFT3 -ARCRADIUS2 1)
	ElementArc(LEFT3 0 ARCRADIUS2 ARCRADIUS2 270 90 1)
	ElementLine(LEFT4 0 LEFT4 MAXY 1)
        define(`LEFT5',`eval(LEFT4-ARCRADIUS)')
	ElementArc(LEFT5 MAXY ARCRADIUS ARCRADIUS 90 90 1)
        ElementLine(LEFT5 LMAXY BRACKETLEFT LMAXY 1)
   	forloop(`SIDE',0,1,
   	`
		define(`IDX',`0')
		define(`ADDDIAMETER',`0')
		forloop(`IDX1',0,LOOPIDX1,
		`
			define(`CURRNUMPINS',`arg'eval((IDX1*2)+2))
			ifelse(SIDE,`0',`
				define(`CURRLEFT',`eval((PINDIST*IDX)+(ADDDIAMETER)+PINLEFT)')
				define(`CURRRIGHT',`eval(CURRLEFT+((CURRNUMPINS-1)*PINDIST)+PINWIDTH)')
				ElementLine(CURRLEFT LMAXY CURRRIGHT LMAXY 2)
				ElementArc(CURRLEFT MAXY ARCRADIUS ARCRADIUS 0 90 2)
				ElementArc(CURRRIGHT MAXY ARCRADIUS ARCRADIUS 90 90 2)
				define(`CURRLEFT1',`eval(CURRLEFT-ARCRADIUS)')
				define(`CURRRIGHT1',`eval(CURRRIGHT+ARCRADIUS)')
		                ElementLine(CURRLEFT1 0 CURRLEFT1 MAXY 1)
                                ElementLine(CURRRIGHT1 0 CURRRIGHT1 MAXY 1)
		                ifelse(1,eval((NUMARRAYS>1)&&(IDX1<LOOPIDX1)),`
					define(`CURRDIAMETER',`arg'eval((IDX1*2)+3))
					define(`CURRRADIUS',`eval(CURRDIAMETER/2)')
					ElementArc(eval(CURRRIGHT+EDGEGAP+(CURRDIAMETER/2)) 0 CURRRADIUS CURRRADIUS 180 180 1)
				')
	   			ifelse(1,eval(IDX1==LOOPIDX1),`
					ElementLine(BRACKETLEFT LMAXY BRACKETLEFT BOARDTOP 1)
					ElementLine(BRACKETLEFT BOARDTOP CURRRIGHT1 BOARDTOP 1)
					ElementLine(CURRRIGHT1 BOARDTOP CURRRIGHT1 0 1)
				        
				        ElementArc(eval(BRACKETLEFT+SCREWBORDER) eval(LMAXY-SCREWBORDER) 125 125 0 360 1)
				        ElementArc(eval(BRACKETLEFT+SCREWBORDER) eval(BOARDTOP+SCREWBORDER) 125 125 0 360 1)
				')
			')
   			forloop(`IDX2',0,eval(CURRNUMPINS-1),
   			`
				define(`CURRX',`eval((PINDIST*IDX)+(ADDDIAMETER))')
				define(`MINX',`eval(PINLEFT+CURRX)')
				define(`MIDX',`eval(PINCENTER+CURRX)')
				define(`MAXX',`eval(PINRIGHT+CURRX)')
				define(`PINIDX',`eval(IDX+1+ADDSIDE)')
	                        EDGECONN(MINX,MINY,MAXX,MINY,PENWIDTH,PINIDX,FLAG)
				EDGECONN(MINX,MINY,MINX,MIDY,PENWIDTH,PINIDX,FLAG)
				EDGECONN(MINX,MIDY,MIDX,MAXY,PENWIDTH,PINIDX,FLAG)
				EDGECONN(MIDX,MINY,MIDX,MAXY,PENWIDTH,PINIDX,FLAG)
				EDGECONN(MAXX,MINY,MAXX,MIDY,PENWIDTH,PINIDX,FLAG)
				EDGECONN(MAXX,MIDY,MIDX,MAXY,PENWIDTH,PINIDX,FLAG)				
				define(`IDX',incr(IDX))
			')
			
			ifelse(1,eval((NUMARRAYS>1)&&(IDX1<LOOPIDX1)),`
				define(`ADDDIAMETER1',forloop(`IDX2',0,eval(IDX1),
				``arg'eval((IDX2*2)+3)+')0)
				define(`ADDDIAMETER',eval(ADDDIAMETER1+(EDGEGAP*2)-PINDIST+PINWIDTH))
			')
		')
		define(`ADDSIDE',`TOTALPINS')
		define(`FLAG',0x80)
  	')
	 
)')



define(`PKG_PCI5V_AVE_HEIGHT',   `PKG_PCIPINARRAY(`$1', `$2', `$3', 2810,49,100,11)');
define(`PKG_PCI5V_MIN_HEIGHT',   `PKG_PCIPINARRAY(`$1', `$2', `$3', 1420,49,100,11)');
define(`PKG_PCI5V_MAX_HEIGHT',   `PKG_PCIPINARRAY(`$1', `$2', `$3', 4200,49,100,11)');
define(`PKG_PCI5V_SMALL_HEIGHT', `PKG_PCIPINARRAY(`$1', `$2', `$3', 2063,49,100,11)');






