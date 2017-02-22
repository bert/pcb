# -*- m4 -*-
#
#                            COPYRIGHT
#
#   PCB, interactive printed circuit board design
#   Copyright (C) 2017 Dan McMahill
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; version 2 of the License
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#
#
#  TDK Specific Footprints



# EXTRACT_BEGIN

#
## HLBC Series (B82145A 6.5 x 12 mm) Inductors
#

# B82145A series = 6.5 x 12 mm
# ordering code: B82145A + 1 +  xx + m + J000
# xx * 10^m = inductance in nH.  For example 224 = 220,000 nH = 220 uH
#
# _M = most
# _N = nominal
# _L = leaset

define(`PKG_TDK_B82145A_M', `PKG_INDAD1600W80L1200D650A(  `$1', `$2', `$3')');
define(`PKG_TDK_B82145A_N', `PKG_INDAD1560W80L1200D650B(  `$1', `$2', `$3')');
define(`PKG_TDK_B82145A_L', `PKG_INDAD1520W80L1200D650C(  `$1', `$2', `$3')');


# EXTRACT_END
