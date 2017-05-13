# -*- m4 -*-
#
#                            COPYRIGHT
#
#   PCB, interactive printed circuit board design
#   Copyright (C) 2003, 2004 Dan McMahill
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
#  Panasonic Specific Footprints


# EXTRACT_BEGIN

#
##  Panasonic EXB Series Chip Resistor Array
#

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

define(`PKG_PANASONIC_EXB14V',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',4,30,30,50,30)')
define(`PKG_PANASONIC_EXB24V',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',4,38,48,65,50)')
define(`PKG_PANASONIC_EXB28V',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',8,25,50,50,40)')
define(`PKG_PANASONIC_EXBV4V',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',4,43,70,80,80)')
define(`PKG_PANASONIC_EXBV8V',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',8,43,70,80,80)')
define(`PKG_PANASONIC_EXB34V',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',4,45,80,80,80)')
define(`PKG_PANASONIC_EXB38V',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',8,45,80,80,80)')
define(`PKG_PANASONIC_EXBS8V',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',8,63,120,127,110)')
define(`PKG_PANASONIC_EXB2HV',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',16,25,50,50,100)')

# EXTRACT_END

