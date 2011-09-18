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
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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



# these should be in the memory file.

# RAM

define(`Description_4016_dil', ``Static RAM 2Kx8'')
define(`Param1_4016_dil', 24)
define(`Param2_4016_dil', 600)

define(`Description_6116_dil', ``Static RAM 2Kx8'')
define(`Param1_6116_dil', 24)
define(`Param2_6116_dil', 600)


define(`Description_2114_dil', ``Static RAM 1Kx4'')
define(`Param1_2114_dil', 18)
define(`Param2_2114_dil', 300)


# some Dallas Semiconductor parts:  
#   http://www.dalsemi.com/products/memory/index.html
# Battery Backed NVSRAM 

define(`Description_DS1220_dil', ``NVSRAM 2Kx8'')
define(`Param1_DS1220_dil', 24)
define(`Param2_DS1220_dil', 600)

define(`Description_DS1225_dil', ``NVSRAM 8Kx8'')
define(`Param1_DS1225_dil', 28)
define(`Param2_DS1225_dil', 600)

define(`Description_DS1230_dil', ``NVSRAM 32Kx8'')
define(`Param1_DS1230_dil', 28)
define(`Param2_DS1230_dil', 600)


# EPROM

define(`Description_2532_dil', ``EPROM 4Kx8'')
define(`Param1_2532_dil', 24)
define(`Param2_2532_dil', 600)

define(`Description_2716_dil', ``EPROM 2Kx8'')
define(`Param1_2716_dil', 24)
define(`Param2_2716_dil', 600)

define(`Description_2732_dil', ``EPROM 4Kx8'')
define(`Param1_2732_dil', 24)
define(`Param2_2732_dil', 600)

define(`Description_2764_dil', ``EPROM 8Kx8'')
define(`Param1_2764_dil', 28)
define(`Param2_2764_dil', 600)

define(`Description_27128_dil', ``EPROM 16Kx8'')
define(`Param1_27128_dil', 28)
define(`Param2_27128_dil', 600)

define(`Description_27256_dil', ``EPROM 32Kx8'')
define(`Param1_27256_dil', 28)
define(`Param2_27256_dil', 600)

divert(0)dnl
