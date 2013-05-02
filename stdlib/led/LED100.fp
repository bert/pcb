Element(0x00 "LED 5mm" "" "LED 100" 100 70 0 100 0x00)
(
# typical LED is 0.5 mm or 0.020" square pin.  See for example
# http://www.lumex.com and part number SSL-LX3054LGD.
# 0.020" square is 0.0288" diagonal.  A number 57 drill is 
# 0.043" which should be enough.  a 65 mil pad gives 11 mils
# of annular ring.
	Pin(-50 0 65 43 "-" 0x101)
	Pin(50 0 65 43 "+" 0x01)
   ElementArc(0 0 50 50    45  90 10)
	ElementArc(0 0 50 50   225  90 10)
   ElementArc(0 0 70 70    45  90 10)
	ElementArc(0 0 70 70   225  90 10)
	Mark(0 0)
)
