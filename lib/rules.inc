#
#   $Id$
#
#   design rule definitions --
#   tune design rules here
#
#   revisions:
#   Oct 12 2002 - Egil Kvaleberg <egil@kvaleberg.no>
#   Initial version
#
#   NOTE: The HOLE specification is the diameter of the finished hole.
#         For through-hole plated PCBs, the actual drill dimension will be
#         be larger. How much depends on process parameters
#

#
#  pads for comopnents with thin pins (e.g TO92)
#
define(`rPAD1',    50)
define(`rHOLE1',   24)

#
#  pads for components on 100mil pitch
#  hole 2 for ICs etc
#  hole 2c for headers etc. with square pins
#
#  NOTE: rPAD2 should really be 62mil
#
define(`rPAD2',    60)
define(`rHOLE2',   28)
define(`rHOLE2c',  40)

#
#  pads for larger transistors
#
define(`rPAD3',    80 )
define(`rHOLE3',   50)

#
#  3mm pads for mounting
#  BUG: drill is bad
#
define(`rPAD4',    120)
define(`rHOLE4',   80)

#
#  standard silkscreen line
#  thin, fat, fatter
#
define(`rSILK',    10)
define(`rSILK2',   20)
define(`rSILK3',   30)

