	Element["" "Transistor" "" "TO18" 10300 11100 6000 7000 0 100 ""]
(
# The JEDEC drawing shows a pin diameter of 16-21 mils
#
#
#         ___x_
#        /     \
# TO18:  |3   1|     <-- bottom view (supposed to be a circle)
#        \  2  /
#          ---
#       
# NOTE:  some vendors, ST for example, number the pins
# differently.  Here we follow the JEDEC drawing.
#
# the pins are arranged along a 100 mil diameter
# circle.  The can outline is 178 to 195 mils
# for the top of the can and 209 to 230 mils
# for the bottom edge of the can
#
        Pin[0 -5000 5500 3000 6100 3500 "1" "1" ""]
        Pin[-5000 0 5500 3000 6100 3500 "2" "2" ""]
        Pin[0 5000 5500 3000 6100 3500 "3" "3" ""]
# x, y, width, height, start angle, delta angle, thickness
        ElementArc [0 0 9800 9800 0 360 1000]
# tab is 28 to 48 mils long, 36 to 46 wide
# and comes off at an angle of 45 deg clockwise from
# pin 1 when looking at the top of the board
        ElementLine [6700 -7900 9400 -10600 1000]
        ElementLine [7300 -7300 10000 -10000 1000]
        ElementLine [7900 -6700 10600 -9400 1000]
        ElementLine [9400 -10600 10600 -9400 1000]
)
