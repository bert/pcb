divert(-1)
# $Id$
#
# Definitions for PCI boards
# by D.J. Barrow dj_barrow@ariasoft.ie
#
define(`PL_PCISideB1',``-12V',`TCK',`Ground',`TD0',`+5V',`+5V',`INTB*',`INTD*',`PRSNT1*',`Reserved',`PRSNT2*'')

define(`PL_PCISideB2',``Ground',`Ground'')

define(`PL_PCISideB3',``Reserved',`Ground',`CLK',`Ground',`REQ*',`+5V',`AD[31]',`AD[29]',`Ground',`AD[27]',`AD[25]',`+3.3V',`C/BE[3]*',`AD[23]',`Ground',`AD[21]',`AD[19]',`+3.3V',`AD[17]',`C/BE[2]*',`Ground',`IRDY*',`+3.3V',`DEVSEL*',`Ground',`LOCK*',`PERR*',`+3.3V',`SERR*',`+3.3V',`C/BE[1]*',`AD[14]',`Ground',`AD[12]',`AD[10]',`Ground'')

define(`PL_PCISideB4',``Ground',`Ground'')
define(`PL_PCISideB5',``AD[08]',`AD[07]',`+3.3V',`AD[05]',`AD[03]',`Ground',`AD[01]',`+5V',`ACK64*',`+5V',`+5V'')


define(`PL_PCISideB6',``Reserved',`Ground',`C/BE[6]*',`C/BE[4]*',`Ground',`AD[63]',`AD[61]',`+5V',`AD[59]',`AD[57]',`Ground',`AD[55]',`AD[53]',`Ground',`AD[51]',`AD[49]',`+5V',`AD[47]',`AD[45]',`Ground',`AD[43]',`AD[41]',`Ground',`AD[39]',`AD[37]',`+5V',`AD[35]',`AD[33]',`Ground',`Reserved',`Reserved',`Ground'')

define(`PL_PCISideA1',``TRST*',`+12V',`TMS',`TDI',`+5V',`INTA*',`INTC*',`+5V',`Reserved',`+5V',`Reserved'')

define(`PL_PCISideA2',``Ground',`Ground'')
define(`PL_PCISideA3',``3.3Vaux',`RST*',`+5V',`GNT*',`Ground',`PME*',`AD[30]',`+3.3V',`AD[28]',`AD[26]',`Ground',`AD[24]',`IDSEL',`+3.3V',`AD[22]',`AD[20]',`Ground',`AD[18]',`AD[16]',`+3.3V',`FRAME*',`Ground',`TRDY*',`Ground',`STOP*',`+3.3V',`Reserved',`Reserved',`Ground',`PAR',`AD[15]',`+3.3V',`AD[13]',`AD[11]',`Ground',`AD[09]'')


define(`PL_PCISideA4',``Ground',`Ground'')
define(`PL_PCISideA5',``C/BE[0]*',`+3.3V',`AD[06]',`AD[04]',`Ground',`AD[02]',`AD[00]',`+5V',`REQ64*',`+5V',`+5V'')

define(`PL_PCISideA6',``Ground',`C/BE[7]*',`C/BE[5]*',`+5V',`PAR64',`AD[62]',`Ground',`AD[60]',`AD[58]',`Ground',`AD[56]',`AD[54]',`+5V',`AD[52]',`AD[50]',`Ground',`AD[48]',`AD[46]',`Ground',`AD[44]',`AD[42]',`+5V',`AD[40]',`AD[38]',`Ground',`AD[36]',`AD[34]',`Ground',`AD[32]',`Reserved',`Ground',`Reserved'')
define(`Description_PCI5V_AVE_HEIGHT',`PCI 5V Array Average Height')
define(`Description_PCI5V_MIN_HEIGHT',`PCI 5V Array Min Height')
define(`Description_PCI5V_MAX_HEIGHT',`PCI 5V Array Max Height')
define(`Description_PCI5V_SMALL_HEIGHT',`PCI 5V Array Small Height')
define(`PinList_PCI5V',`PL_PCISideB1,PL_PCISideB2,PL_PCISideB3,PL_PCISideB5,PL_PCISideA1,PL_PCISideA2,PL_PCISideA3,PL_PCISideA5')
define(`PinList_PCI5V_AVE_HEIGHT', `PinList_PCI5V')
define(`PinList_PCI5V_MIN_HEIGHT', `PinList_PCI5V')
define(`PinList_PCI5V_MAX_HEIGHT', `PinList_PCI5V')
define(`PinList_PCI5V_SMALL_HEIGHT', `PinList_PCI5V')

divert(0)dnl








