divert(-1)
#
#                             COPYRIGHT
# 
#   PCB, interactive printed circuit board design
#   Copyright (C) 1994,1995,1996 Thomas Nau
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
#   Contact addresses for paper mail and Email:
#   Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
#   Thomas.Nau@rz.uni-ulm.de
# 

# ----------------------------------------------------------------------
# resistor array without common pin
#
define(`Description_r_025_sil_4', ``2xR-array 0.25W'')
define(`Param1_r_025_sil_4', 4)
define(`PinList_r_025_sil_4', ``1a', `1b', `2a', `2b'')

define(`Description_r_025_sil_6', ``3xR-array 0.25W'')
define(`Param1_r_025_sil_6', 6)
define(`PinList_r_025_sil_6', ``1a', `1b', `2a', `2b', `3a', `3b'')

define(`Description_r_025_sil_8', ``4xR-array 0.25W'')
define(`Param1_r_025_sil_8', 8)
define(`PinList_r_025_sil_8', ``1a', `1b', `2a', `2b', `3a', `3b', `4a', `4b'')

# ----------------------------------------------------------------------
# resistor array with common pin 1
#
define(`Description_r_025_csil_4', ``4xR-array 0.25W, common pin'')
define(`Param1_r_025_csil_4', 5)
define(`PinList_r_025_csil_4', ``common', `1', `2', `3', `4'')

define(`Description_r_025_csil_6', ``6xR-array 0.25W, common pin'')
define(`Param1_r_025_csil_6', 7)
define(`PinList_r_025_csil_6', ``common', `1', `2', `3', `4', `5', `6'')

define(`Description_r_025_csil_7', ``7xR-array 0.25W, common pin'')
define(`Param1_r_025_csil_7', 8)
define(`PinList_r_025_csil_7', ``common', `1', `2', `3', `4', `5', `6', `7'')

define(`Description_r_025_csil_8', ``8xR-array 0.25W, common pin'')
define(`Param1_r_025_csil_8', 9)
define(`PinList_r_025_csil_8', ``common', `1', `2', `3', `4', `5', `6', `7', `8'')

divert(0)
