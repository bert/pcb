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
define(`Description_connector2', `connector 2x1 pins')
define(`Param1_connector2', 1)
define(`Param2_connector2', 2)

define(`Description_connector4', `connector 2x2 pins')
define(`Param1_connector4', 2)
define(`Param2_connector4', 2)

define(`Description_connector6', `connector 2x3 pins')
define(`Param1_connector6', 3)
define(`Param2_connector6', 2)

define(`Description_connector8', `connector 2x4 pins')
define(`Param1_connector8', 4)
define(`Param2_connector8', 2)

define(`Description_connector10', `connector 2x5 pins')
define(`Param1_connector10', 5)
define(`Param2_connector10', 2)

define(`Description_connector12', `connector 2x6 pins')
define(`Param1_connector12', 6)
define(`Param2_connector12', 2)

define(`Description_connector12', `connector 3x4 pins')
define(`Param1_connector12', 4)
define(`Param2_connector12', 3)

define(`Description_connector14', `connector 2x7 pins')
define(`Param1_connector14', 7)
define(`Param2_connector14', 2)

define(`Description_connector15', `connector 3x5 pins')
define(`Param1_connector15', 5)
define(`Param2_connector15', 3)

define(`Description_connector16', `connector 2x8 pins')
define(`Param1_connector16', 8)
define(`Param2_connector16', 2)

define(`Description_connector18', `connector 2x9 pins')
define(`Param1_connector18', 9)
define(`Param2_connector18', 2)

define(`Description_connector18', `connector 3x6 pins')
define(`Param1_connector18', 6)
define(`Param2_connector18', 3)

define(`Description_connector20', `connector 2x10 pins')
define(`Param1_connector20', 10)
define(`Param2_connector20', 2)

define(`Description_connector50', `connector 2x25 pins')
define(`Param1_connector50', 25)
define(`Param2_connector50', 2)


define(`Description_DIN41_612_abc48male_s', `DIN41.612 row a+b+c male')
define(`Param1_DIN41_612_abc48male', `abc')
define(`PinList_DIN41_612_abc48male',
	`forloop(`i', 1, 16, `define(`P_'i, `a'i)')'
	`forloop(`i', 1, 16, `define(`P_'eval(i +16), `b'i)')'
	`forloop(`i', 1, 16, `define(`P_'eval(i +32), `c'i)')')

define(`Description_DIN41_612_abc48female_s', `DIN41.612 row a+b+c female')
define(`Param1_DIN41_612_abc48female', `abc')
define(`PinList_DIN41_612_abc48female',
	`forloop(`i', 1, 16, `define(`P_'i, `a'i)')'
	`forloop(`i', 1, 16, `define(`P_'eval(i +16), `b'i)')'
	`forloop(`i', 1, 16, `define(`P_'eval(i +32), `c'i)')')

define(`Description_DIN41_612_ab64male', `DIN41.612 row a+b male')
define(`Param1_DIN41_612_ab64male', `ab')
define(`PinList_DIN41_612_ab64male',
	`forloop(`i', 1, 32, `define(`P_'i, `a'i)')'
	`forloop(`i', 1, 32, `define(`P_'eval(i +32), `b'i)')')

define(`Description_DIN41_612_ab64female', `DIN41.612 row a+b female')
define(`Param1_DIN41_612_ab64female', `ab')
define(`PinList_DIN41_612_ab64female',
	`forloop(`i', 1, 32, `define(`P_'i, `a'i)')'
	`forloop(`i', 1, 32, `define(`P_'eval(i +32), `b'i)')')

define(`Description_DIN41_612_ac64male', `DIN41.612 row a+c male')
define(`Param1_DIN41_612_ac64male', `ac')
define(`PinList_DIN41_612_ac64male',
	`forloop(`i', 1, 32, `define(`P_'i, `a'i)')'
	`forloop(`i', 1, 32, `define(`P_'eval(i +64), `c'i)')')

define(`Description_DIN41_612_ac64female', `DIN41.612 row a+c female')
define(`Param1_DIN41_612_ac64female', `ac')
define(`PinList_DIN41_612_ac64female',
	`forloop(`i', 1, 32, `define(`P_'i, `a'i)')'
	`forloop(`i', 1, 32, `define(`P_'eval(i +64), `c'i)')')

define(`Description_DIN41_612_abc96male', `DIN41.612 row a+b+c male')
define(`Param1_DIN41_612_abc96male', `abc')
define(`PinList_DIN41_612_abc96male',
	`forloop(`i', 1, 32, `define(`P_'i, `a'i)')'
	`forloop(`i', 1, 32, `define(`P_'eval(i +32), `b'i)')'
	`forloop(`i', 1, 32, `define(`P_'eval(i +64), `c'i)')')

define(`Description_DIN41_612_abc96female', `DIN41.612 row a+b+c female')
define(`Param1_DIN41_612_abc96female', `abc')
define(`PinList_DIN41_612_abc96female',
	`forloop(`i', 1, 32, `define(`P_'i, `a'i)')'
	`forloop(`i', 1, 32, `define(`P_'eval(i +32), `b'i)')'
	`forloop(`i', 1, 32, `define(`P_'eval(i +64), `c'i)')')

define(`Description_DIN41_651_10lay', `DIN41.651 laying 10 pins')
define(`Param1_DIN41_651_10lay', 10)

define(`Description_DIN41_651_10stand', `DIN41.651 standing 10 pins')
define(`Param1_DIN41_651_10stand', 10)

define(`Description_DIN41_651_14lay', `DIN41.651 laying 14 pins')
define(`Param1_DIN41_651_14lay', 14)

define(`Description_DIN41_651_14stand', `DIN41.651 standing 14 pins')
define(`Param1_DIN41_651_14stand', 14)

define(`Description_DIN41_651_16lay', `DIN41.651 laying 16 pins')
define(`Param1_DIN41_651_16lay', 16)

define(`Description_DIN41_651_16stand', `DIN41.651 standing 16 pins')
define(`Param1_DIN41_651_16stand', 16)

define(`Description_DIN41_651_18lay', `DIN41.651 laying 18 pins')
define(`Param1_DIN41_651_18lay', 18)

define(`Description_DIN41_651_18stand', `DIN41.651 standing 18 pins')
define(`Param1_DIN41_651_18stand', 18)

define(`Description_DIN41_651_20lay', `DIN41.651 laying 20 pins')
define(`Param1_DIN41_651_20lay', 20)

define(`Description_DIN41_651_20stand', `DIN41.651 standing 20 pins')
define(`Param1_DIN41_651_20stand', 20)

define(`Description_DIN41_651_22lay', `DIN41.651 laying 22 pins')
define(`Param1_DIN41_651_22lay', 22)

define(`Description_DIN41_651_22stand', `DIN41.651 standing 22 pins')
define(`Param1_DIN41_651_22stand', 22)

define(`Description_DIN41_651_24lay', `DIN41.651 laying 24 pins')
define(`Param1_DIN41_651_24lay', 24)

define(`Description_DIN41_651_24stand', `DIN41.651 standing 24 pins')
define(`Param1_DIN41_651_24stand', 24)

define(`Description_DIN41_651_26lay', `DIN41.651 laying 26 pins')
define(`Param1_DIN41_651_26lay', 26)

define(`Description_DIN41_651_26stand', `DIN41.651 standing 26 pins')
define(`Param1_DIN41_651_26stand', 26)

define(`Description_DIN41_651_34lay', `DIN41.651 laying 34 pins')
define(`Param1_DIN41_651_34lay', 34)

define(`Description_DIN41_651_34stand', `DIN41.651 standing 34 pins')
define(`Param1_DIN41_651_34stand', 34)

define(`Description_DIN41_651_40lay', `DIN41.651 laying 40 pins')
define(`Param1_DIN41_651_40lay', 40)

define(`Description_DIN41_651_40stand', `DIN41.651 standing 40 pins')
define(`Param1_DIN41_651_40stand', 40)

define(`Description_DIN41_651_50lay', `DIN41.651 laying 50 pins')
define(`Param1_DIN41_651_50lay', 50)

define(`Description_DIN41_651_50stand', `DIN41.651 standing 50 pins')
define(`Param1_DIN41_651_50stand', 50)

define(`Description_DIN41_651_64lay', `DIN41.651 laying 64 pins')
define(`Param1_DIN41_651_64lay', 64)

define(`Description_DIN41_651_64stand', `DIN41.651 standing 64 pins')
define(`Param1_DIN41_651_64stand', 64)

define(`Description_PC_Centronics', `PC Centronics Connector')
define(`Param1_PC_Centronics', 25)
define(`PinList_PC_Centronics', ``/Strb',`D0',`D1',`D2',`D3',`D4',`D5',`D6',`D7',`/Ack',`Busy',`PaperE',`Sel',`AutoF',`/Fault',`/Res',`SelIn',`Gnd',`Gnd',`Gnd',`Gnd',`Gnd',`Gnd',`Gnd',`Gnd'')

define(`Description_PC_V24_9P', `PC V24 9-pins')
define(`Param1_PC_V24_9P', 9)
define(`PinList_PC_V24_9P', ``DCD',`RxD',`TxD',`DTR',`Gnd',`DSR',`RTS',`CTS',`RI'')

define(`Description_PC_V24_25P', `PC V24 25-pins')
define(`Param1_PC_V24_25P', 25)
define(`PinList_PC_V24_25P', ``PE',`TxD',`RxD',`RTS',`CTS',`DSR',`Gnd',`DCD',`9',`10',`11',`12',`13',`14',`15',`16',`17',`18',`19',`DTR',`21',`RI',`23',`24',`25'')

define(`Description_SCSI_SE', `single-ended SCSI 2x25 pins')
define(`Param1_SCSI_SE', 25)
define(`Param2_SCSI_SE', 2)
define(`PinList_SCSI_SE', ``Gnd',`/DB0',`Gnd',`/DB1',`Gnd',`/DB2',`Gnd',`/DB3',`Gnd',`/DB4',`Gnd',`/DB5',`Gnd',`/DB6',`Gnd',`/DB7',`Gnd',`/DBP',`Gnd',`NC',`Gnd',`NC',`Gnd',`NC',`NC',`TermPwr',`Gnd',`NC',`Gnd',`NC',`Gnd',`/Atn',`Gnd',`NC',`Gnd',`/Bsy',`Gnd',`/Ack',`Gnd',`/Rst',`Gnd',`/Msg',`Gnd',`/Sel',`Gnd',`/CD',`Gnd',`/Req',`Gnd',`/IO'')

define(`Description_SUBD_9F', `SUB-D female 9 pins')
define(`Param1_SUBD_9F', 9)

define(`Description_SUBD_9M', `SUB-D male 9 pins')
define(`Param1_SUBD_9M', 9)

define(`Description_SUBD_15F', `SUB-D female 15 pins')
define(`Param1_SUBD_15F', 15)

define(`Description_SUBD_15M', `SUB-D male 15 pins')
define(`Param1_SUBD_15M', 15)

define(`Description_SUBD_25F', `SUB-D female 25 pins')
define(`Param1_SUBD_25F', 25)

define(`Description_SUBD_25M', `SUB-D male 25 pins')
define(`Param1_SUBD_25M', 25)

define(`Description_BNC_rightangle', `right angle BNC')

define(`Description_MTA_156_2', `MTA-156 header')
define(`Param1_MTA_156_2', 2)

define(`Description_SMA_vertical', `vertical SMA')

define(`Molex_025_style',
	`define(`Description_MOLEX_025_$1', `Molex .025 pitch $1 pin plug')'
	`define(`Param1_MOLEX_025_$1', $1)'
)
Molex_025_style(20)
Molex_025_style(30)
Molex_025_style(40)
Molex_025_style(50)
Molex_025_style(60)
Molex_025_style(68)
Molex_025_style(70)
Molex_025_style(80)
Molex_025_style(90)
Molex_025_style(100)
Molex_025_style(120)
Molex_025_style(140)
Molex_025_style(160)
Molex_025_style(180)
Molex_025_style(200)
Molex_025_style(240)

define(`Description_NANOENGINE', `Bright Star nanoEngine SBC')

divert(0)dnl
