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

#  Arcade machine parts needed by Scott "Jerry" Lawrence
#   jsl@absynth.com


# this should be in the lsi file perhaps.

define(`Description_z80_dil', ``Zilog Z80'')
define(`Param1_z80_dil', 40)
define(`Param2_z80_dil', 600)
define(`PinList_z80_dil', ``A11', `A12', `A13', `A14', `A15', `theta', `D3',  `D4',  `D5',  `D6', `+5v', `D2',  `D7',  `D0',  `D1', `/INT', `/NMI', `/HALT', `/MREQ', `/IORQ', `/RD',  `/WR',  `/BUSAK', `/WAIT', `/BUSRQ', `/RESET', `/M1', `/RFSH', `GND', `A0', `A1',  `A2', `A3', `A4', `A5', `A6',  `A7', `A8', `A9', `A10'')



# these should be in the memory file.

# RAM

define(`Description_4016_dil', ``Static RAM 2Kx8'')
define(`Param1_4016_dil', 24)
define(`Param2_4016_dil', 600)
define(`PinList_4016_dil', ``A7',`A6',`A5',`A4',`A3',`A2',`A1',`A0',`D0',`D1',`D2',`Gnd',`D3',`D4',`D5',`D6',`D7',`/Cs',`A10',`/Oe',`/W',`A9',`A8',`Vcc'')

define(`Description_6116_dil', ``Static RAM 2Kx8'')
define(`Param1_6116_dil', 24)
define(`Param2_6116_dil', 600)
define(`PinList_6116_dil', ``A7',`A6',`A5',`A4',`A3',`A2',`A1',`A0',`D0',`D1',`D2',`Gnd',`D3',`D4',`D5',`D6',`D7',`/Cs',`A10',`/Oe',`/We',`A9',`A8',`Vcc'')


define(`Description_2114_dil', ``Static RAM 1Kx4'')
define(`Param1_2114_dil', 18)
define(`Param2_2114_dil', 300)
define(`PinList_2114_dil', ``A6', `A5', `A4', `A3', `A0', `A1', `A2', `/Ce', `Gnd', `/We', `D3', `D2', `D1', `D0', `A9', `A8', `A7', `Vcc'')


# some Dallas Semiconductor parts:  
#   http://www.dalsemi.com/products/memory/index.html
# Battery Backed NVSRAM 

define(`Description_DS1220_dil', ``NVSRAM 2Kx8'')
define(`Param1_DS1220_dil', 24)
define(`Param2_DS1220_dil', 600)
define(`PinList_DS1220_dil', ``A7',`A6',`A5',`A4',`A3',`A2',`A1',`A0',`D0',`D1',`D2',`Gnd',`D3',`D4',`D5',`D6',`D7',`/Ce',`A10',`/Oe',`/We',`A9',`A8',`Vcc'')

define(`Description_DS1225_dil', ``NVSRAM 8Kx8'')
define(`Param1_DS1225_dil', 28)
define(`Param2_DS1225_dil', 600)
define(`PinList_DS1225_dil', ``n/c', `A12', `A7',`A6',`A5',`A4',`A3',`A2',`A1',`A0',`D0',`D1',`D2',`Gnd',`D3',`D4',`D5',`D6',`D7',`/Ce',`A10',`/Oe',`A11',`A9',`A8',`n/c', `/We', `Vcc'')

define(`Description_DS1230_dil', ``NVSRAM 32Kx8'')
define(`Param1_DS1230_dil', 28)
define(`Param2_DS1230_dil', 600)
define(`PinList_DS1230_dil', ``A14', `A12', `A7',`A6',`A5',`A4',`A3',`A2',`A1',`A0',`D0',`D1',`D2',`Gnd',`D3',`D4',`D5',`D6',`D7',`/Ce',`A10',`/Oe',`A11',`A9',`A8',`A13', `/We', `Vcc'')


# EPROM

define(`Description_2532_dil', ``EPROM 4Kx8'')
define(`Param1_2532_dil', 24)
define(`Param2_2532_dil', 600)
define(`PinList_2532_dil', ``A7',`A6',`A5',`A4',`A3',`A2',`A1',`A0',`D0',`D1',`D2',`Gnd',`D3',`D4',`D5',`D6',`D7',`A11',`A10',`/Oe',`Vpp',`A9',`A8',`Vcc'')

define(`Description_2716_dil', ``EPROM 2Kx8'')
define(`Param1_2716_dil', 24)
define(`Param2_2716_dil', 600)
define(`PinList_2716_dil', ``n/c',`A6',`A5',`A4',`A3',`A2',`A1',`A0',`D0',`D1',`D2',`Gnd',`D3',`D4',`D5',`D6',`D7',`/CeP',`A10',`/Oe',`Vpp',`A9',`A8',`Vcc'')

define(`Description_2732_dil', ``EPROM 4Kx8'')
define(`Param1_2732_dil', 24)
define(`Param2_2732_dil', 600)
define(`PinList_2732_dil', ``A7',`A6',`A5',`A4',`A3',`A2',`A1',`A0',`D0',`D1',`D2',`Gnd',`D3',`D4',`D5',`D6',`D7',`/Cs',`A10',`/Oe',`A11',`A9',`A8',`Vcc'')

define(`Description_2764_dil', ``EPROM 8Kx8'')
define(`Param1_2764_dil', 28)
define(`Param2_2764_dil', 600)
define(`PinList_2764_dil', ``Vpp', `A12', `A7',`A6',`A5',`A4',`A3',`A2',`A1',`A0',`D0',`D1',`D2',`Gnd',`D3',`D4',`D5',`D6',`D7',`/Cs',`A10',`/Oe',`A11',`A9',`A8', `n/c', `/PGM', `Vcc'')

define(`Description_27128_dil', ``EPROM 16Kx8'')
define(`Param1_27128_dil', 28)
define(`Param2_27128_dil', 600)
define(`PinList_27128_dil', ``Vpp', `A12', `A7',`A6',`A5',`A4',`A3',`A2',`A1',`A0',`D0',`D1',`D2',`Gnd',`D3',`D4',`D5',`D6',`D7',`/Cs',`A10',`/Oe',`A11',`A9',`A8', `A13', `A14', `Vcc'')

define(`Description_27256_dil', ``EPROM 32Kx8'')
define(`Param1_27256_dil', 28)
define(`Param2_27256_dil', 600)
define(`PinList_27256_dil', ``Vpp', `A12', `A7',`A6',`A5',`A4',`A3',`A2',`A1',`A0',`D0',`D1',`D2',`Gnd',`D3',`D4',`D5',`D6',`D7',`/Cs',`A10',`/Oe',`A11',`A9',`A8', `A13', `A14', `Vcc'')

divert(0)dnl
