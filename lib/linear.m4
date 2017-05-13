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
define(`Description_MAX222_dil', `high-speed dual RS232 driver w. shutdown')
define(`Param1_MAX222_dil', 18)
define(`Param2_MAX222_dil', 300)
define(`PinList_MAX222_dil', ``NC',`C1+',`V+',`C1-',`C2+',`C2-',`V-',`T2out',`R2in',`R2out',`T2in',`T1in',`R1out',`R1in',`T1out',`GND',`Vcc',`/Shdn'')

define(`Description_MAX232_dil', `dual RS232 driver')
define(`Param1_MAX232_dil', 16)
define(`Param2_MAX232_dil', 300)
define(`PinList_MAX232_dil', ``C1+',`V+',`C1-',`C2+',`C2-',`V-',`T2out',`R2in',`R2out',`T2in',`T1in',`R1out',`R1in',`T1out',`GND',`Vcc'')

define(`Description_MAX232A_dil', `high-speed dual RS232 driver')
define(`Param1_MAX232A_dil', 16)
define(`Param2_MAX232A_dil', 300)
define(`PinList_MAX232A_dil', ``C1+',`V+',`C1-',`C2+',`C2-',`V-',`T2out',`R2in',`R2out',`T2in',`T1in',`R1out',`R1in',`T1out',`GND',`Vcc'')

define(`Description_MAX233_dil', `dual RS232 driver without external components')
define(`Param1_MAX233_dil', 20)
define(`Param2_MAX233_dil', 300)
define(`PinList_MAX233_dil', ``T2in',`T1in',`R1out',`R1in',`T1out',`Gnd',`Vcc',`C1+',`Gnd',`C2-',`C2+',`V-',`C1-',`V+',`C2+',`C2-',`V-',`T2out',`R2in',`R2out'')

define(`Description_MAX233A_dil', `high-speed dual RS232 driver without external components')
define(`Param1_MAX233A_dil', 20)
define(`Param2_MAX233A_dil', 300)
define(`PinList_MAX233A_dil', ``T2in',`T1in',`R1out',`R1in',`T1out',`Gnd',`Vcc',`C1+',`Gnd',`C2-',`C2+',`V-',`C1-',`V+',`C2+',`C2-',`V-',`T2out',`R2in',`R2out'')

define(`Description_MAX667_dil', `5V/adjustable low-dropout linear regulator')
define(`Param1_MAX667_dil', 8)
define(`Param2_MAX667_dil', 300)
define(`PinList_MAX667_dil', ``DD',`OUT',`LBI',`GND',`SHDN',`SET',`LBO',`IN'')

define(`Description_MAX680_dil', `+-10V voltage converter')
define(`Param1_MAX680_dil', 8)
define(`Param2_MAX680_dil', 300)
define(`PinList_MAX680_dil', ``C1-',`C2+',`C2-',`V-',`GND',`Vcc',`C1+',`V+'')

define(`Description_MAX690_dil', `uP supervisor w. watchdog and power-fail signal')
define(`Param1_MAX690_dil', 8)
define(`Param2_MAX690_dil', 300)
define(`PinList_MAX690_dil', ``Vout',`Vcc',`Gnd',`PFI',`/PFO',`WDI',`/RESET',`VBatt'')

define(`Description_MAX691_dil', `uP supervisor w. watchdog, chip-enable and power-fail signal')
define(`Param1_MAX691_dil', 16)
define(`Param2_MAX691_dil', 300)
define(`PinList_MAX691_dil', ``Vbatt',`Vout',`Vcc',`Gnd',`BattOn',`/LowLine',`OSC_In',`OSC_Out',`PFI',`/PFO',`WDI',`/CE_Out',`/CE_In',`/WDO',`/Reset',`Reset'')

# --------------------------------------------------------------------
# based on data mailed by Olaf Kaluza (olaf@criseis.ruhr.de)
#
define(`Description_L297_dil', `stepper-motor controller')
define(`Param1_L297_dil', 20)
define(`Param2_L297_dil', 300)
define(`PinList_L297_dil', ``Sync',`Gnd',`Home',`A',`/Inh1',`B',`C',`/Inh2',`D',`Enable',`Control',`Vs',`Sens2',`Sens1',`Vref',`Osc',`CW/CCW',`/Clock',`Half/Full',`/Reset'')

define(`Description_L297A_dil', `stepper-motor controller')
define(`Param1_L297A_dil', 20)
define(`Param2_L297A_dil', 300)
define(`PinList_L297A_dil', ``Sync',`Gnd',`Home',`A',`/Inh1',`B',`C',`/Inh2',`D',`Enable',`Dir-Mem',`Vs',`Sens2',`Sens1',`Vref',`Osc',`CW/CCW',`/Clock',`Half/Full',`/Reset'')

define(`Description_NE4558_dil', `dual operating-amplifier')
define(`Param1_NE4558_dil', 8)
define(`Param2_NE4558_dil', 300)
define(`PinList_NE4558_dil', ``Out1',`Inv1',`NoInv1',`-Us',`NoInv2',`Inv2',`Out2',`+Us'')

define(`Description_L298_multiwatt', `dual full-bridge driver')
define(`PinList_L298_multiwatt', ``I-Sens1', `Out1', `Out2', `Vcc', `In1', `Enable A', `In2', `Gnd', `Logic-Vcc', `In3', `Enable B', `In4', `Out3', `Out4', `I-Sens2'')

divert(0)dnl
