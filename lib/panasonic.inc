# -*- m4 -*-
#
# $Id$
#                            COPYRIGHT
#
#   PCB, interactive printed circuit board design
#   Copyright (C) 2003 Dan McMahill
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
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
# 
# 
#
#  Panasonic Specific Footprints


# EXTRACT_BEGIN

#
##  Panasonic EXB Series Chip Resistor Array
#
define(`PKG_PANASONIC_EXB14V',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',4,30,30,50,60)')
define(`PKG_PANASONIC_EXB24V',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',4,38,48,65,100)')
define(`PKG_PANASONIC_EXB28V',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',8,53,54,50,94)')
define(`PKG_PANASONIC_EXBV4V',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',4,43,70,80,150)')
define(`PKG_PANASONIC_EXBV8V',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',8,43,70,80,150)')
define(`PKG_PANASONIC_EXB34V',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',4,45,80,80,160)')
define(`PKG_PANASONIC_EXB38V',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',8,45,80,80,160)')
define(`PKG_PANASONIC_EXBS8V',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',8,63,120,127,230)')
define(`PKG_PANASONIC_EXB2HV',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',16,43,50,50,150)')

# EXTRACT_BEGIN
