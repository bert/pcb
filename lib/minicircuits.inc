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
#  Mini-Circuits Specific Footprints


# EXTRACT_BEGIN


#
##  Mini-Circuits CA Style Package
#
define(`PKG_MINICIRCUITS_CA531',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',6,20000,50000,37000,90000)')

#
##  Mini-Circuits CB Style Package
#
define(`PKG_MINICIRCUITS_CB518',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',10,30000,90000,50000,200000)')
define(`PKG_MINICIRCUITS_CB539',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',10,30000,90000,50000,225000)')

#
##  Mini-Circuits CD Style Package
#
define(`PKG_MINICIRCUITS_CD541',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',6,65000,100000,100000,200000)')
define(`PKG_MINICIRCUITS_CD542',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',6,65000,100000,100000,200000)')
define(`PKG_MINICIRCUITS_CD636',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',6,65000,100000,100000,200000)')
define(`PKG_MINICIRCUITS_CD637',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',6,65000,100000,100000,200000)')

#
##  Mini-Circuits KK Style Package
#
define(`PKG_MINICIRCUITS_KK81',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',6,50000,126000,100000,480000,"reverse")')

# EXTRACT_BEGIN
