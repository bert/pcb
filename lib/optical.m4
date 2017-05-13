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
#
# most (all at the beginning) of the data was provided by
# Volker Bosch (bosch@iema.e-technik.uni-stuttgart.de)
#
define(`Description_LED_3MM', `LED 3mm')
define(`Param1_LED_3MM', 60)
define(`PinList_LED_3MM', ``-', `+'')

define(`Description_LED_5MM', `LED 5mm')
define(`Param1_LED_5MM', 100)
define(`PinList_LED_5MM', ``-', `+'')

define(`Description_OPTO_6N136', `optical coupling device')
define(`Param1_OPTO_6N136', 8)
define(`Param2_OPTO_6N136', 300)
define(`PinList_OPTO_6N136', ``NC', `A+', `K-', `NC', `Gnd', `Out-Col', `Basis', `Vcc'')

define(`Description_OPTO_6N137', `optical coupling device')
define(`Param1_OPTO_6N137', 8)
define(`Param2_OPTO_6N137', 300)
define(`PinList_OPTO_6N137', ``NC', `A+', `K-', `NC', `Gnd', `Out', `En', `Vcc'')

divert(0)dnl
