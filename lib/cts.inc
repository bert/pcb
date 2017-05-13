# -*- m4 -*-
#
#                            COPYRIGHT
#
#   PCB, interactive printed circuit board design
#   Copyright (C) 2004 Dan McMahill
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
# 
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
# 
#   You should have received a copy of the GNU General Public License along
#   with this program; if not, write to the Free Software Foundation, Inc.,
#   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
# 
# 
#
#  CTS (http://www.ctscorp.com) Specific Footprints
#
# for the 742/3/4/5/6 resistors see
# http://www.ctscorp.com/components/Datasheets/CTSChipArrayDs.pdf

# EXTRACT_BEGIN

# COMMON_SMT_DIL_MM
# $4: number of pins
# $5: pad width  (1/100 mm)
# $6: pad length (1/100 mm)
# $7: pad pitch (1/100 mm)
# $8: pad seperation for pads on opposite sides of
#     the package (1/100 mm)
# $9: define to make the pins get numbered starting with the highest pin
#     instead of pin 1.  Needed for certain brain damaged packages like
#     the Mini-Circuits KK81

#
##  CTS 742C Series Chip Resistor Array
#

define(`PKG_CTS_742C_04',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3' ,4,50,90,80,90)')
define(`PKG_CTS_742C_08',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3', 8,50,90,80,90)')
define(`PKG_CTS_742C_16',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',16,50,90,80,90)')

#
##  CTS 742C Series Chip Resistor Array
#

define(`PKG_CTS_742X_08',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3', 8,50,90,80,90)')

#
##  CTS 743C Series Chip Resistor Array
#

define(`PKG_CTS_743C_04',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',4,32000,47000,50000,28000)')
define(`PKG_CTS_743C_08',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',8,32000,47000,50000,28000)')

#
##  CTS 744C Series Chip Resistor Array
#

define(`PKG_CTS_744C_04',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',4,32000,51000,50000,67000)')
define(`PKG_CTS_744C_08',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',8,32000,51000,50000,67000)')

#
##  CTS 745C Series Chip Resistor Array
#

define(`PKG_CTS_745C_10',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',10,32000,51000,50000,67000)')

#
#
##  CTS 745X Series Chip Resistor Array
#

define(`PKG_CTS_745X_10',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',10,32000,51000,50000,67000)')

#
##  CTS 746X Series Chip Resistor Array
#

define(`PKG_CTS_746X_10',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',10,14000,32000,25000,28000)')

# EXTRACT_END

