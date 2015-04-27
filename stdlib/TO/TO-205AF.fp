	Element["" "Transistor" "" "TO39" 18800 18800 6000 7000 0 100 ""]
(
# The JEDEC drawing shows a pin diameter of 16-21 mils
#
#
#         ___x_
#        /     \
# TO39:  |3   1|     <-- bottom view (supposed to be a circle)
#        \  2  /
#          ---
#       
# NOTE:  some vendors, ST for example, number the pins
# differently.  Here we follow the JEDEC drawing.
#
# the pins are arranged along a 200 mil diameter
# circle.  The can outline is 315 to 335 mils (320 nom)
# for the top of the can and 350 to 370 mils (360 nom)
# for the bottom edge of thecan
#
        Pin[0 -10000 5500 3000 6100 3500 "1" "1" "square"]
        Pin[-10000 0 5500 3000 6100 3500 "2" "2" ""]
        Pin[0 10000 5500 3000 6100 3500 "3" "3" ""]
# tab is 29 to 40 mils long, 28 to 34 wide
# and comes off at an angle of 45 deg clockwise from
# pin 1 when looking at the top of the board
        ElementLine [12700 -13900 14800 -16000 1000]
        ElementLine [13300 -13300 15400 -15400 1000]
        ElementLine [13900 -12700 16000 -14800 1000]
        ElementLine [16000 -14800 14800 -16000 1000]
# x, y, width, height, start angle, delta angle, thickness
        ElementArc [0 0 18300 18300 0 360 1000]
        )
