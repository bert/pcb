# -*- m4 -*-
#
#   gEDA compatible footprint names
#
#   the real component definitions are in the *.inc files
#
#   Initial version from Egil Kvaleberg <egil@kvaleberg.no>,
#   Oct 10 2002
#

#
##  Dual in-line package, narrow (300 mil)
#   DIP6 .. DIP64, DIP24N, DIP28N
#
define(`PKG_DIP4',   `PKG_N(`$1', `$2', `$3',   4,  300)');
define(`PKG_DIP6',   `PKG_N(`$1', `$2', `$3',   6,  300)');
define(`PKG_DIP8',   `PKG_N(`$1', `$2', `$3',   8,  300)');
define(`PKG_DIP14',  `PKG_N(`$1', `$2', `$3',  14,  300)');
define(`PKG_DIP16',  `PKG_N(`$1', `$2', `$3',  16,  300)');
define(`PKG_DIP18',  `PKG_N(`$1', `$2', `$3',  18,  300)');
define(`PKG_DIP20',  `PKG_N(`$1', `$2', `$3',  20,  300)');
define(`PKG_DIP22',  `PKG_N(`$1', `$2', `$3',  22,  300)');
define(`PKG_DIP24N', `PKG_N(`$1', `$2', `$3',  24,  300)');
define(`PKG_DIP28N', `PKG_N(`$1', `$2', `$3',  28,  300)');
##  Dual in-line package, medium wide (400 mil)
define(`PKG_DIP8M',  `PKG_N(`$1', `$2', `$3',   8,  400)');
define(`PKG_DIP14M', `PKG_N(`$1', `$2', `$3',  14,  400)');
define(`PKG_DIP16M', `PKG_N(`$1', `$2', `$3',  16,  400)');
define(`PKG_DIP18M', `PKG_N(`$1', `$2', `$3',  18,  400)');
define(`PKG_DIP20M', `PKG_N(`$1', `$2', `$3',  20,  400)');
define(`PKG_DIP22M', `PKG_N(`$1', `$2', `$3',  22,  400)');
define(`PKG_DIP24M', `PKG_N(`$1', `$2', `$3',  24,  400)');
define(`PKG_DIP28M', `PKG_N(`$1', `$2', `$3',  28,  400)');
##  Dual in-line package, wide (600 mil)
define(`PKG_DIP24',  `PKG_N(`$1', `$2', `$3',  24,  600)');
define(`PKG_DIP28',  `PKG_N(`$1', `$2', `$3',  28,  600)');
define(`PKG_DIP32',  `PKG_N(`$1', `$2', `$3',  32,  600)');
define(`PKG_DIP36',  `PKG_N(`$1', `$2', `$3',  36,  600)');
define(`PKG_DIP40',  `PKG_N(`$1', `$2', `$3',  40,  600)');
define(`PKG_DIP42',  `PKG_N(`$1', `$2', `$3',  42,  600)');
define(`PKG_DIP44',  `PKG_N(`$1', `$2', `$3',  44,  600)');
define(`PKG_DIP48',  `PKG_N(`$1', `$2', `$3',  48,  600)');
##  Dual in-line package, wide (900 mil)
define(`PKG_DIP64',  `PKG_N(`$1', `$2', `$3',  64,  900)');

#
##  Shrink dual in-line package, wide (300 mil)
#
define(`PKG_SDIP18',  `PKG_NS(`$1', `$2', `$3', 18,  300)');
define(`PKG_SDIP20',  `PKG_NS(`$1', `$2', `$3', 20,  300)');
define(`PKG_SDIP22',  `PKG_NS(`$1', `$2', `$3', 22,  300)');
define(`PKG_SDIP24',  `PKG_NS(`$1', `$2', `$3', 24,  300)');
#
##  Shrink dual in-line package, wide (400 mil)
define(`PKG_SDIP30',  `PKG_NS(`$1', `$2', `$3', 30,  400)');
define(`PKG_SDIP32',  `PKG_NS(`$1', `$2', `$3', 32,  400)');
#
##  Shrink dual in-line package, wide (600 mil)
define(`PKG_SDIP42',  `PKG_NS(`$1', `$2', `$3', 42,  600)');

#
##  Single in line package
#   SIP1 .. SIP13
#
define(`PKG_SIP1',  `PKG_CSIL(`$1', `$2', `$3', 1)');
define(`PKG_SIP2',  `PKG_CSIL(`$1', `$2', `$3', 2)');
define(`PKG_SIP3',  `PKG_CSIL(`$1', `$2', `$3', 3)');
define(`PKG_SIP4',  `PKG_CSIL(`$1', `$2', `$3', 4)');
define(`PKG_SIP5',  `PKG_CSIL(`$1', `$2', `$3', 5)');
define(`PKG_SIP6',  `PKG_CSIL(`$1', `$2', `$3', 6)');
define(`PKG_SIP7',  `PKG_CSIL(`$1', `$2', `$3', 7)');
define(`PKG_SIP8',  `PKG_CSIL(`$1', `$2', `$3', 8)');
define(`PKG_SIP9',  `PKG_CSIL(`$1', `$2', `$3', 9)');
define(`PKG_SIP10', `PKG_CSIL(`$1', `$2', `$3', 10)');
define(`PKG_SIP11', `PKG_CSIL(`$1', `$2', `$3', 11)');
define(`PKG_SIP12', `PKG_CSIL(`$1', `$2', `$3', 12)');
define(`PKG_SIP13', `PKG_CSIL(`$1', `$2', `$3', 13)');

#
##  Ultra (Micro?) Small outline package
#
# pins, width (1/100mil), pitch (1/100 mil)
define(`PKG_US8',     `PKG_US( `$1', `$2', `$3',  8)');
define(`PKG_US14',    `PKG_US( `$1', `$2', `$3', 14)');
define(`PKG_US16',    `PKG_US( `$1', `$2', `$3', 16)');

#
##  Small outline package, narrow (150mil)
#   aka SOP8, aka S8
#
# COMMON_SMT_DIL_{MIL,MM}
# $4: number of pins
# $5: pad width  (1/1000 mil or 1/100 mm)
# $6: pad length (1/1000 mil or 1/100 mm)
# $7: pad pitch (1/1000 mil 1/100 mm)
# $8: pad seperation for pads on opposite sides of
#     the package (1/1000 mil or 1/100 mm)
# $9: define to make the pins get numbered starting with the highest pin
#     instead of pin 1.  Needed for certain brain damaged packages like
#     the Mini-Circuits KK81

# dimensions from http://landpatterns.ipc.org/
define(`PKG_SO8',   `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',  8, 20000, 85000, 50000, 120000)');
define(`PKG_SO14',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3', 14, 20000, 85000, 50000, 120000)');
define(`PKG_SO16',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3', 16, 20000, 85000, 50000, 120000)');
define(`PKG_SO18N', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3', 18, 20000, 85000, 50000, 120000)');
define(`PKG_SO20N', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3', 20, 20000, 85000, 50000, 120000)');

##  Small outline package, medium (200mil)
#   aka 8S2
define(`PKG_SO8M',   `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',  8, 20000, 85000, 50000, 170000)');
define(`PKG_SO18M',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3', 18, 20000, 85000, 50000, 170000)');
define(`PKG_SO20M',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3', 20, 20000, 85000, 50000, 170000)');
define(`PKG_SO22M',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3', 22, 20000, 85000, 50000, 170000)');
define(`PKG_SO24M',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3', 24, 20000, 85000, 50000, 170000)');

##  Small outline package, wide (300mil)
# dimensions from http://landpatterns.ipc.org/
define(`PKG_SO8W',   `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',  8, 20000, 80000, 50000, 280000)');
define(`PKG_SO14W',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3', 14, 20000, 80000, 50000, 280000)');
define(`PKG_SO16W',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3', 16, 20000, 80000, 50000, 280000)');
define(`PKG_SO18W',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3', 18, 20000, 80000, 50000, 280000)');
define(`PKG_SO20W',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3', 20, 20000, 80000, 50000, 280000)');
define(`PKG_SO24W',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3', 24, 20000, 80000, 50000, 280000)');
define(`PKG_SO28',   `COMMON_SMT_DIL_MIL(`$1',`$2',`$3', 28, 20000, 80000, 50000, 280000)');
define(`PKG_SO32',   `COMMON_SMT_DIL_MIL(`$1',`$2',`$3', 32, 20000, 80000, 50000, 280000)');

##  Small outline package, wider (325mil)
define(`PKG_SO28W',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3', 28, 20000, 80000, 50000, 305000)');

##  Small outline package, wider (450mil)
define(`PKG_SO32W',  `COMMON_SMT_DIL_MIL(`$1',`$2',`$3', 32, 20000, 80000, 50000, 430000)');

##  Small outline package, wide (525mil)
define(`PKG_SO44',   `COMMON_SMT_DIL_MIL(`$1',`$2',`$3', 44, 20000, 80000, 50000, 505000)');



# NOTE: There is also SOJ20,28,32,40,42
# 0.300" size, 28,32 pin (MO-088, MO-077 respectively)
# 0.400" size, 28,32,36 pin (MO-061)
#
#
# from http://landpatterns.ipc.org/
# 14-28 pin 0.300"  - pitch = 1.27 mm, pad is 0.6 x 2.2, gap is 5
# 14-28 pin 0.350"  - pitch = 1.27 mm, pad is 0.6 x 2.2, gap is 6.2
# 14-28 pin 0.400"  - pitch = 1.27 mm, pad is 0.6 x 2.2, gap is 7.4
# 14-28 pin 0.450"  - pitch = 1.27 mm, pad is 0.6 x 2.2, gap is 8.8
#
# Cypress (www.cypress.com) has 20, 24, 28, 32 in the 0.3" and
# 28, 32, 36, 44 in the 0.4"

## Small outline J-leaded package (300 mil)
define(`PKG_SOJ14_300', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',14,23622,86614,50000,196850)');
define(`PKG_SOJ16_300', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',16,23622,86614,50000,196850)');
define(`PKG_SOJ18_300', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',18,23622,86614,50000,196850)');
define(`PKG_SOJ20_300', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',20,23622,86614,50000,196850)');
define(`PKG_SOJ22_300', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',22,23622,86614,50000,196850)');
define(`PKG_SOJ24_300', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',24,23622,86614,50000,196850)');
define(`PKG_SOJ26_300', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',26,23622,86614,50000,196850)');
define(`PKG_SOJ28_300', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',28,23622,86614,50000,196850)');
define(`PKG_SOJ30_300', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',30,23622,86614,50000,196850)');
define(`PKG_SOJ32_300', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',32,23622,86614,50000,196850)');
define(`PKG_SOJ34_300', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',34,23622,86614,50000,196850)');
define(`PKG_SOJ36_300', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',36,23622,86614,50000,196850)');
define(`PKG_SOJ38_300', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',38,23622,86614,50000,196850)');
define(`PKG_SOJ40_300', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',40,23622,86614,50000,196850)');
define(`PKG_SOJ42_300', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',42,23622,86614,50000,196850)');
define(`PKG_SOJ44_300', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',44,23622,86614,50000,196850)');

## Small outline J-leaded package (350 mil)
define(`PKG_SOJ14_350', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',14,23622,86614,50000,244094)');
define(`PKG_SOJ16_350', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',16,23622,86614,50000,244094)');
define(`PKG_SOJ18_350', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',18,23622,86614,50000,244094)');
define(`PKG_SOJ20_350', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',20,23622,86614,50000,244094)');
define(`PKG_SOJ22_350', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',22,23622,86614,50000,244094)');
define(`PKG_SOJ24_350', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',24,23622,86614,50000,244094)');
define(`PKG_SOJ26_350', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',26,23622,86614,50000,244094)');
define(`PKG_SOJ28_350', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',28,23622,86614,50000,244094)');
define(`PKG_SOJ30_350', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',30,23622,86614,50000,244094)');
define(`PKG_SOJ32_350', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',32,23622,86614,50000,244094)');
define(`PKG_SOJ34_350', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',34,23622,86614,50000,244094)');
define(`PKG_SOJ36_350', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',36,23622,86614,50000,244094)');
define(`PKG_SOJ38_350', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',38,23622,86614,50000,244094)');
define(`PKG_SOJ40_350', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',40,23622,86614,50000,244094)');
define(`PKG_SOJ42_350', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',42,23622,86614,50000,244094)');
define(`PKG_SOJ44_350', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',44,23622,86614,50000,244094)');

## Small outline J-leaded package (400 mil)
define(`PKG_SOJ14_400', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',14,23622,86614,50000,291339)');
define(`PKG_SOJ16_400', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',16,23622,86614,50000,291339)');
define(`PKG_SOJ18_400', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',18,23622,86614,50000,291339)');
define(`PKG_SOJ20_400', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',20,23622,86614,50000,291339)');
define(`PKG_SOJ22_400', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',22,23622,86614,50000,291339)');
define(`PKG_SOJ24_400', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',24,23622,86614,50000,291339)');
define(`PKG_SOJ26_400', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',26,23622,86614,50000,291339)');
define(`PKG_SOJ28_400', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',28,23622,86614,50000,291339)');
define(`PKG_SOJ30_400', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',30,23622,86614,50000,291339)');
define(`PKG_SOJ32_400', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',32,23622,86614,50000,291339)');
define(`PKG_SOJ34_400', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',34,23622,86614,50000,291339)');
define(`PKG_SOJ36_400', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',36,23622,86614,50000,291339)');
define(`PKG_SOJ38_400', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',38,23622,86614,50000,291339)');
define(`PKG_SOJ40_400', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',40,23622,86614,50000,291339)');
define(`PKG_SOJ42_400', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',42,23622,86614,50000,291339)');
define(`PKG_SOJ44_400', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',44,23622,86614,50000,291339)');

## Small outline J-leaded package (450 mil)
define(`PKG_SOJ14_450', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',14,23622,86614,50000,346457)');
define(`PKG_SOJ16_450', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',16,23622,86614,50000,346457)');
define(`PKG_SOJ18_450', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',18,23622,86614,50000,346457)');
define(`PKG_SOJ20_450', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',20,23622,86614,50000,346457)');
define(`PKG_SOJ22_450', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',22,23622,86614,50000,346457)');
define(`PKG_SOJ24_450', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',24,23622,86614,50000,346457)');
define(`PKG_SOJ26_450', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',26,23622,86614,50000,346457)');
define(`PKG_SOJ28_450', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',28,23622,86614,50000,346457)');
define(`PKG_SOJ30_450', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',30,23622,86614,50000,346457)');
define(`PKG_SOJ32_450', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',32,23622,86614,50000,346457)');
define(`PKG_SOJ34_450', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',34,23622,86614,50000,346457)');
define(`PKG_SOJ36_450', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',36,23622,86614,50000,346457)');
define(`PKG_SOJ38_450', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',38,23622,86614,50000,346457)');
define(`PKG_SOJ40_450', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',40,23622,86614,50000,346457)');
define(`PKG_SOJ42_450', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',42,23622,86614,50000,346457)');
define(`PKG_SOJ44_450', `COMMON_SMT_DIL_MIL(`$1',`$2',`$3',44,23622,86614,50000,346457)');


#
# NOTE:
# Shrink small outline SSOP is a full confusion of
#       pitch 25.00 mil, 0.65mm, 0.8mm
#       widths 244, 260, 311, 323 etc
# there are really too many variants, the ones listed here are
# fairly industry standard
#
##  Shrink small outline package, .65mm, narrow
define(`PKG_SSOP8',    `PKG_SSO(  `$1', `$2', `$3',  8, 323, 2559)');
define(`PKG_SSOP14',   `PKG_SSO(  `$1', `$2', `$3', 14, 323, 2559)');
define(`PKG_SSOP16',   `PKG_SSO(  `$1', `$2', `$3', 16, 244, 2559)');
define(`PKG_SSOP20',   `PKG_SSO(  `$1', `$2', `$3', 20, 260, 2559)');
define(`PKG_SSOP24',   `PKG_SSO(  `$1', `$2', `$3', 24, 323, 2559)');
define(`PKG_SSOP28',   `PKG_SSO(  `$1', `$2', `$3', 28, 323, 2559)');

#
##  Shrink small outline package, .65mm, wide
#define(`PKG_SSOP16W', `PKG_SSO(  `$1', `$2', `$3', 16, 420, 2559)');
#define(`PKG_SSOP20W', `PKG_SSO(  `$1', `$2', `$3', 20, 420, 2559)');
#define(`PKG_SSOP28W', `PKG_SSO(  `$1', `$2', `$3', 28, 420, 2559)');
#define(`PKG_SSOP30W', `PKG_SSO(  `$1', `$2', `$3', 30, 420, 2559)');
#define(`PKG_SSOP32W', `PKG_SSO(  `$1', `$2', `$3', 32, 420, 2559)');
#define(`PKG_SSOP36W', `PKG_SSO(  `$1', `$2', `$3', 36, 420, 2559)');

#
##  Shrink small outline package, 25mil, wide
define(`PKG_SSOP48W',  `PKG_SSO(  `$1', `$2', `$3', 48, 420, 2500)');
define(`PKG_SSOP56W',  `PKG_SSO(  `$1', `$2', `$3', 56, 420, 2500)');

#
##  Shrink small outline package, .80mm, wider
define(`PKG_SSOP64W',  `PKG_SSO(  `$1', `$2', `$3', 64, 545, 3150)');

##  Shrink small outline package, .65mm, narrow
# BUG:(`PKG_MSOP8',    `PKG_SSO(   `$1', `$2', `$3', 8, 323, 2559)');
#
##  Mini small outline package, .65mm
define(`PKG_MSOP8',    `PKG_SSO(   `$1', `$2', `$3', 8, 199, 2559)');
##  Mini small outline package, .5mm
define(`PKG_MSOP10',   `PKG_TSOP(  `$1', `$2', `$3', 10, 199, 1969)');

##  Quarter size small outline package
define(`PKG_QSOP16',   `PKG_SSO(   `$1', `$2', `$3', 16, 244, 2500)');
define(`PKG_QSOP20',   `PKG_SSO(   `$1', `$2', `$3', 20, 244, 2500)');
define(`PKG_QSOP24',   `PKG_SSO(   `$1', `$2', `$3', 24, 244, 2500)');
define(`PKG_QSOP28',   `PKG_SSO(   `$1', `$2', `$3', 28, 244, 2500)');

#
##  Thin small outline package
# BUG: TSOP in 26(x2),28(x2),32,44(x2),48,50(x2),54,66,70(x2),86
#
define(`PKG_TSOP28',   `PKG_TSOP(  `$1', `$2', `$3', 28, 535, 2165)');
define(`PKG_TSOP32A',  `PKG_TSOP(  `$1', `$2', `$3', 32, 795, 2000)');
define(`PKG_TSOP32B',  `PKG_TSOP(  `$1', `$2', `$3', 32, 559, 2000)');

##  Thin shrink small outline package, narrow (3mm)
#
define(`PKG_TSSOP8',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',  8, 33, 150, 65, 300)');
define(`PKG_TSSOP10', `COMMON_SMT_DIL_MM(`$1',`$2',`$3', 10, 30, 150, 50, 300)');

##  Thin shrink small outline package, standard (4.4mm)
#
define(`PKG_TSSOP8W',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3',  8, 33, 150, 65, 440)');
define(`PKG_TSSOP14',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3', 14, 33, 150, 65, 440)');
define(`PKG_TSSOP16',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3', 16, 33, 150, 65, 440)');
define(`PKG_TSSOP20',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3', 20, 33, 150, 65, 440)');
define(`PKG_TSSOP24',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3', 24, 33, 150, 65, 440)');
define(`PKG_TSSOP28',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3', 28, 33, 150, 65, 440)');
define(`PKG_TSSOP38N', `COMMON_SMT_DIL_MM(`$1',`$2',`$3', 38, 30, 150, 50, 440)');
define(`PKG_TSSOP48N', `COMMON_SMT_DIL_MM(`$1',`$2',`$3', 48, 25, 150, 40, 440)');
define(`PKG_TSSOP56N', `COMMON_SMT_DIL_MM(`$1',`$2',`$3', 56, 25, 150, 40, 440)');

##  Thin shrink small outline package, wide (6.1mm)
#
define(`PKG_TSSOP32',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3', 32, 33, 150, 65, 610)');
define(`PKG_TSSOP48',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3', 48, 30, 150, 50, 610)');
define(`PKG_TSSOP56',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3', 56, 30, 150, 50, 610)');
define(`PKG_TSSOP64',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3', 64, 30, 150, 50, 610)');
define(`PKG_TSSOP80',  `COMMON_SMT_DIL_MM(`$1',`$2',`$3', 80, 25, 150, 40, 610)');

#
##  Plastic leadless chip carrier
#   PLCC44 .. PLCC84
#
#   BUG: similar:
#   QFJ 18,20,22,28,32,44,68,84
#
define(`PKG_PLCC20',  `PKG_PLCC(`$1', `$2', `$3', 20, 150)');
define(`PKG_PLCC28',  `PKG_PLCC(`$1', `$2', `$3', 28, 150)');
define(`PKG_PLCC32',  `PKG_PLCC(`$1', `$2', `$3', 32, 150)');
define(`PKG_PLCC44',  `PKG_PLCC(`$1', `$2', `$3', 44, 150)');
define(`PKG_PLCC52',  `PKG_PLCC(`$1', `$2', `$3', 52, 150)');
define(`PKG_PLCC68',  `PKG_PLCC(`$1', `$2', `$3', 68, 150)');
define(`PKG_PLCC84',  `PKG_PLCC(`$1', `$2', `$3', 84, 150)');

#
##  Plastic leadless chip carrier with pin socket
#   PLCC44X .. PLCC84X
#
define(`PKG_PLCC20X',  `PKG_PLCC_SOCKET(`$1', `$2', `$3', 20, 150)');
define(`PKG_PLCC28X',  `PKG_PLCC_SOCKET(`$1', `$2', `$3', 28, 150)');
define(`PKG_PLCC32X',  `PKG_PLCC_SOCKET(`$1', `$2', `$3', 32, 150)');
define(`PKG_PLCC44X',  `PKG_PLCC_SOCKET(`$1', `$2', `$3', 44, 150)');
define(`PKG_PLCC52X',  `PKG_PLCC_SOCKET(`$1', `$2', `$3', 52, 150)');
define(`PKG_PLCC68X',  `PKG_PLCC_SOCKET(`$1', `$2', `$3', 68, 150)');
define(`PKG_PLCC84X',  `PKG_PLCC_SOCKET(`$1', `$2', `$3', 84, 150)');

##  Square Quad Flat Nolead (QFN) package
#

# 3x3 mm
define(`PKG_QFN16_3',     `PKG_QFN_50S(`$1',`$2',`$3', `16', `3', `0')')
define(`PKG_TQFN16_3',    `PKG_QFN_50S(`$1',`$2',`$3', `16', `3', `0')')
define(`PKG_QFN16_3_EP',  `PKG_QFN_50S(`$1',`$2',`$3', `16', `3', `150')')
define(`PKG_TQFN16_3_EP', `PKG_QFN_50S(`$1',`$2',`$3', `16', `3', `150')')

# 4x4 mm
define(`PKG_QFN12_4',     `PKG_QFN_80(`$1',`$2',`$3', `12', `4', `0')')
define(`PKG_TQFN12_4',    `PKG_QFN_80(`$1',`$2',`$3', `12', `4', `0')')
define(`PKG_QFN12_4_EP',  `PKG_QFN_80(`$1',`$2',`$3', `12', `4', `210')')
define(`PKG_TQFN12_4_EP', `PKG_QFN_80(`$1',`$2',`$3', `12', `4', `210')')

define(`PKG_QFN16_4',     `PKG_QFN_65(`$1',`$2',`$3', `16', `4', `0')')
define(`PKG_TQFN16_4',    `PKG_QFN_65(`$1',`$2',`$3', `16', `4', `0')')
define(`PKG_QFN16_4_EP',  `PKG_QFN_65(`$1',`$2',`$3', `16', `4', `210')')
define(`PKG_TQFN16_4_EP', `PKG_QFN_65(`$1',`$2',`$3', `16', `4', `210')')

define(`PKG_QFN20_4',     `PKG_QFN_50(`$1',`$2',`$3', `20', `4', `0')')
define(`PKG_TQFN20_4',    `PKG_QFN_50(`$1',`$2',`$3', `20', `4', `0')')
define(`PKG_QFN20_4_EP',  `PKG_QFN_50(`$1',`$2',`$3', `20', `4', `210')')
define(`PKG_TQFN20_4_EP', `PKG_QFN_50(`$1',`$2',`$3', `20', `4', `210')')

define(`PKG_QFN24_4',     `PKG_QFN_50S(`$1',`$2',`$3', `24', `4', `0')')
define(`PKG_TQFN24_4',    `PKG_QFN_50S(`$1',`$2',`$3', `24', `4', `0')')
define(`PKG_QFN24_4_EP',  `PKG_QFN_50S(`$1',`$2',`$3', `24', `4', `210')')
define(`PKG_TQFN24_4_EP', `PKG_QFN_50S(`$1',`$2',`$3', `24', `4', `260')')

# 5x5 mm
define(`PKG_QFN16_5',     `PKG_QFN_80(`$1',`$2',`$3', `16', `5', `0')')
define(`PKG_TQFN16_5',    `PKG_QFN_80(`$1',`$2',`$3', `16', `5', `0')')
define(`PKG_QFN16_5_EP',  `PKG_QFN_80(`$1',`$2',`$3', `16', `5', `310')')
define(`PKG_TQFN16_5_EP', `PKG_QFN_80(`$1',`$2',`$3', `16', `5', `310')')

define(`PKG_QFN20_5',     `PKG_QFN_65(`$1',`$2',`$3', `20', `5', `0')')
define(`PKG_TQFN20_5',    `PKG_QFN_65(`$1',`$2',`$3', `20', `5', `0')')
define(`PKG_QFN20_5_EP',  `PKG_QFN_65(`$1',`$2',`$3', `20', `5', `310')')
define(`PKG_TQFN20_5_EP', `PKG_QFN_65(`$1',`$2',`$3', `20', `5', `310')')

define(`PKG_QFN24_5',     `PKG_QFN_65(`$1',`$2',`$3', `24', `5', `0')')
define(`PKG_TQFN24_5',    `PKG_QFN_65(`$1',`$2',`$3', `24', `5', `0')')

define(`PKG_QFN28_5',     `PKG_QFN_50(`$1',`$2',`$3', `28', `5', `0')')
define(`PKG_TQFN28_5',    `PKG_QFN_50(`$1',`$2',`$3', `28', `5', `0')')
define(`PKG_QFN28_5_EP',  `PKG_QFN_50(`$1',`$2',`$3', `28', `5', `310')')
define(`PKG_TQFN28_5_EP', `PKG_QFN_50(`$1',`$2',`$3', `28', `5', `325')')

define(`PKG_QFN32_5',     `PKG_QFN_50S(`$1',`$2',`$3', `32', `5', `0')')
define(`PKG_TQFN32_5',    `PKG_QFN_50S(`$1',`$2',`$3', `32', `5', `0')')
define(`PKG_QFN32_5_EP',  `PKG_QFN_50S(`$1',`$2',`$3', `32', `5', `310')')
define(`PKG_TQFN32_5_EP', `PKG_QFN_50S(`$1',`$2',`$3', `32', `5', `310')')

# 6x6 mm
define(`PKG_QFN36_6',     `PKG_QFN_50(`$1',`$2',`$3', `36', `6', `0')')
define(`PKG_TQFN36_6',    `PKG_QFN_50(`$1',`$2',`$3', `36', `6', `0')')
define(`PKG_QFN36_6_EP',  `PKG_QFN_50(`$1',`$2',`$3', `36', `6', `370')')
define(`PKG_TQFN36_6_EP', `PKG_QFN_50(`$1',`$2',`$3', `36', `6', `370')')

define(`PKG_QFN40_6',     `PKG_QFN_50S(`$1',`$2',`$3', `40', `6', `0')')
define(`PKG_TQFN40_6',    `PKG_QFN_50S(`$1',`$2',`$3', `40', `6', `0')')
define(`PKG_QFN40_6_EP',  `PKG_QFN_50S(`$1',`$2',`$3', `40', `6', `410')')
define(`PKG_TQFN40_6_EP', `PKG_QFN_50S(`$1',`$2',`$3', `40', `6', `410')')

# 7x7 mm
define(`PKG_QFN32_7',     `PKG_QFN_65(`$1',`$2',`$3', `32', `7', `0')')
define(`PKG_TQFN32_7',    `PKG_QFN_65(`$1',`$2',`$3', `32', `7', `0')')
define(`PKG_QFN32_7_EP',  `PKG_QFN_65(`$1',`$2',`$3', `32', `7', `470')')
define(`PKG_TQFN32_7_EP', `PKG_QFN_65(`$1',`$2',`$3', `32', `7', `470')')

define(`PKG_QFN44_7',     `PKG_QFN_50(`$1',`$2',`$3', `44', `7', `0')')
define(`PKG_TQFN44_7',    `PKG_QFN_50(`$1',`$2',`$3', `44', `7', `0')')
define(`PKG_QFN44_7_EP',  `PKG_QFN_50(`$1',`$2',`$3', `44', `7', `470')')
define(`PKG_TQFN44_7_EP', `PKG_QFN_50(`$1',`$2',`$3', `44', `7', `470')')

define(`PKG_QFN48_7',     `PKG_QFN_50S(`$1',`$2',`$3', `48', `7', `0')')
define(`PKG_TQFN48_7',    `PKG_QFN_50S(`$1',`$2',`$3', `48', `7', `0')')
define(`PKG_QFN48_7_EP',  `PKG_QFN_50S(`$1',`$2',`$3', `48', `7', `560')')
define(`PKG_TQFN48_7_EP', `PKG_QFN_50S(`$1',`$2',`$3', `48', `7', `560')')

# 8x8 mm
define(`PKG_QFN56_8',     `PKG_QFN_50S(`$1',`$2',`$3', `56', `8', `0')')
define(`PKG_TQFN56_8',    `PKG_QFN_50S(`$1',`$2',`$3', `56', `8', `0')')
define(`PKG_QFN56_8_EP',  `PKG_QFN_50S(`$1',`$2',`$3', `56', `8', `665')')
define(`PKG_TQFN56_8_EP', `PKG_QFN_50S(`$1',`$2',`$3', `56', `8', `665')')

# 10x10 mm
define(`PKG_QFN68_10',     `PKG_QFN_50(`$1',`$2',`$3', `68', `10', `0')')
define(`PKG_TQFN68_10',    `PKG_QFN_50(`$1',`$2',`$3', `68', `10', `0')')
define(`PKG_QFN68_10_EP',  `PKG_QFN_50(`$1',`$2',`$3', `68', `10', `770')')
define(`PKG_TQFN68_10_EP', `PKG_QFN_50(`$1',`$2',`$3', `68', `10', `770')')

#
##  Square Quad-side flat pack
#   QFP 32,44,56,64,80,100,128,160,208,240,272,304
#   LQFP 144,176,208
#   TQFP 44,48,64,80,120

define(`PKG_LQFP24_4',   `PKG_LQFP_50(`$1',`$2',`$3', `24', `4',`0')');
define(`PKG_LQFP32_5',   `PKG_LQFP_50(`$1',`$2',`$3', `32', `5',`0')');
define(`PKG_LQFP32_7',   `PKG_LQFP_80(`$1',`$2',`$3', `32', `7',`0')');
define(`PKG_LQFP44_10',  `PKG_LQFP_80(`$1',`$2',`$3', `44',`10',`0')');
define(`PKG_LQFP48_7',   `PKG_LQFP_50(`$1',`$2',`$3', `48', `7',`0')');
define(`PKG_LQFP48_12',  `PKG_LQFP_80(`$1',`$2',`$3', `48',`12',`0')');
define(`PKG_LQFP52_10',  `PKG_LQFP_65(`$1',`$2',`$3', `52',`10',`0')');
define(`PKG_LQFP64_7',   `PKG_LQFP_40(`$1',`$2',`$3', `64', `7',`0')');
define(`PKG_LQFP64_10',  `PKG_LQFP_50(`$1',`$2',`$3', `64',`10',`0')');
define(`PKG_LQFP64_14',  `PKG_LQFP_80(`$1',`$2',`$3', `64',`14',`0')');
define(`PKG_LQFP72_10',  `PKG_LQFP_50(`$1',`$2',`$3', `72',`10',`0')');
define(`PKG_LQFP80_10',  `PKG_LQFP_40(`$1',`$2',`$3', `80',`10',`0')');
define(`PKG_LQFP80_12',  `PKG_LQFP_50(`$1',`$2',`$3', `80',`12',`0')');
define(`PKG_LQFP80_14',  `PKG_LQFP_65(`$1',`$2',`$3', `80',`14',`0')');
define(`PKG_LQFP100_10', `PKG_LQFP_40(`$1',`$2',`$3',`100',`10',`0')');
define(`PKG_LQFP100_14', `PKG_LQFP_50(`$1',`$2',`$3',`100',`14',`0')');
define(`PKG_LQFP112_20', `PKG_LQFP_65(`$1',`$2',`$3',`112',`20',`0')');
define(`PKG_LQFP120_16', `PKG_LQFP_50(`$1',`$2',`$3',`120',`16',`0')');
define(`PKG_LQFP128_14', `PKG_LQFP_40(`$1',`$2',`$3',`128',`14',`0')');
define(`PKG_LQFP128_14_EP', `PKG_LQFP_40(`$1',`$2',`$3',`128',`14',`970')');
define(`PKG_LQFP144_20', `PKG_LQFP_50(`$1',`$2',`$3',`144',`20',`0')');
define(`PKG_LQFP160_24', `PKG_LQFP_50(`$1',`$2',`$3',`160',`24',`0')');
define(`PKG_LQFP176_24', `PKG_LQFP_50(`$1',`$2',`$3',`176',`24',`0')');


define(`PKG_QFP32_7',   `PKG_QFP_80(`$1',`$2',`$3', `32', `7',`0')');
define(`PKG_QFP44_10',  `PKG_QFP_80(`$1',`$2',`$3', `44',`10',`0')');
define(`PKG_QFP52_10',  `PKG_QFP_65(`$1',`$2',`$3', `52',`10',`0')');
define(`PKG_QFP64_14',  `PKG_QFP_80(`$1',`$2',`$3', `64',`14',`0')');
define(`PKG_QFP80_14',  `PKG_QFP_65(`$1',`$2',`$3', `80',`14',`0')');
define(`PKG_QFP100_14', `PKG_QFP_50(`$1',`$2',`$3',`100',`14',`0')');
define(`PKG_QFP120_28', `PKG_QFP_80(`$1',`$2',`$3',`120',`28',`0')');
define(`PKG_QFP128_28', `PKG_QFP_80(`$1',`$2',`$3',`128',`28',`0')');
define(`PKG_QFP144_28', `PKG_QFP_65(`$1',`$2',`$3',`144',`28',`0')');
define(`PKG_QFP160_28', `PKG_QFP_65(`$1',`$2',`$3',`160',`28',`0')');
define(`PKG_QFP208_28', `PKG_QFP_50(`$1',`$2',`$3',`208',`28',`0')');
define(`PKG_QFP160_28', `PKG_QFP_65(`$1',`$2',`$3',`160',`28',`0')');
define(`PKG_QFP240_32', `PKG_QFP_50(`$1',`$2',`$3',`240',`32',`0')');
define(`PKG_QFP304_40', `PKG_QFP_50(`$1',`$2',`$3',`304',`40',`0')');

define(`PKG_TQFP32_7',   `PKG_QFP_80(`$1',`$2',`$3', `32', `7',`0')');
define(`PKG_TQFP44_10',  `PKG_QFP_80(`$1',`$2',`$3', `44',`10',`0')');
define(`PKG_TQFP52_10',  `PKG_QFP_65(`$1',`$2',`$3', `52',`10',`0')');
define(`PKG_TQFP64_10',  `PKG_QFP_50(`$1',`$2',`$3', `64',`10',`0')');
define(`PKG_TQFP64_14',  `PKG_QFP_80(`$1',`$2',`$3', `64',`14',`0')');
define(`PKG_TQFP80_12',  `PKG_QFP_50(`$1',`$2',`$3', `80',`12',`0')');
define(`PKG_TQFP80_14',  `PKG_QFP_65(`$1',`$2',`$3', `80',`14',`0')');
define(`PKG_TQFP100_12', `PKG_QFP_40(`$1',`$2',`$3',`100',`12',`0')');
define(`PKG_TQFP100_14', `PKG_QFP_50(`$1',`$2',`$3',`100',`14',`0')');
define(`PKG_TQFP120_28', `PKG_QFP_80(`$1',`$2',`$3',`120',`28',`0')');
define(`PKG_TQFP128_14', `PKG_LQFP_40(`$1',`$2',`$3',`128',`14',`0')');
define(`PKG_TQFP128_14_EP', `PKG_LQFP_40(`$1',`$2',`$3',`128',`14',`970')');
define(`PKG_TQFP128_28', `PKG_QFP_80(`$1',`$2',`$3',`128',`28',`0')');
define(`PKG_TQFP144_28', `PKG_QFP_65(`$1',`$2',`$3',`144',`28',`0')');
define(`PKG_TQFP160_28', `PKG_QFP_65(`$1',`$2',`$3',`160',`28',`0')');
define(`PKG_TQFP208_28', `PKG_QFP_50(`$1',`$2',`$3',`208',`28',`0')');
define(`PKG_TQFP160_28', `PKG_QFP_65(`$1',`$2',`$3',`160',`28',`0')');
define(`PKG_TQFP240_32', `PKG_QFP_50(`$1',`$2',`$3',`240',`32',`0')');
define(`PKG_TQFP304_40', `PKG_QFP_50(`$1',`$2',`$3',`304',`40',`0')');

#
##  Rectangular Quad-side flat pack

define(`PKG_QFP64_R',   `PKG_QFP_100(`$1',`$2',`$3', `64',`0',`0')');
define(`PKG_QFP80_R',   `PKG_QFP_80L(`$1',`$2',`$3', `80',`0',`0')');
define(`PKG_QFP100_R',  `PKG_QFP_65L(`$1',`$2',`$3',`100',`0',`0')');
define(`PKG_QFP128_R',  `PKG_QFP_50L(`$1',`$2',`$3',`128',`0',`0')');
define(`PKG_LQFP128_R', `PKG_LQFP_50(`$1',`$2',`$3',`128',`0',`0')');

##  Zig-zag in-line package
#   ZIP20 .. ZIP40
#
define(`PKG_ZIP9',   `PKG_SD(`$1', `$2', `$3',  9)');
define(`PKG_ZIP12',  `PKG_SD(`$1', `$2', `$3', 12)');
define(`PKG_ZIP16',  `PKG_SD(`$1', `$2', `$3', 16)');
define(`PKG_ZIP18',  `PKG_SD(`$1', `$2', `$3', 18)');
define(`PKG_ZIP20',  `PKG_SD(`$1', `$2', `$3', 20)');
define(`PKG_ZIP24',  `PKG_SD(`$1', `$2', `$3', 24)');
define(`PKG_ZIP28',  `PKG_SD(`$1', `$2', `$3', 28)');
define(`PKG_ZIP40',  `PKG_SD(`$1', `$2', `$3', 40)');

#
##  Axial non-polar component (typically resistor or capacitor),
#   100 through 1600 mil between pins
#   AXN100 .. AXN1600
#
define(`PKG_ACY100',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 100)');
define(`PKG_ACY150',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 150)');
define(`PKG_ACY200',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 200)');
define(`PKG_ACY300',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 300)');
define(`PKG_ACY400',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 400)');
define(`PKG_ACY500',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 500)');
define(`PKG_ACY600',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 600)');
define(`PKG_ACY700',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 700)');
define(`PKG_ACY800',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 800)');
define(`PKG_ACY900',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 900)');
define(`PKG_ACY1000', `PKG_AXIAL_LAY(`$1', `$2', `$3', 1000)');
define(`PKG_ACY1100', `PKG_AXIAL_LAY(`$1', `$2', `$3', 1100)');
define(`PKG_ACY1200', `PKG_AXIAL_LAY(`$1', `$2', `$3', 1200)');
define(`PKG_ACY1300', `PKG_AXIAL_LAY(`$1', `$2', `$3', 1300)');
define(`PKG_ACY1400', `PKG_AXIAL_LAY(`$1', `$2', `$3', 1400)');
define(`PKG_ACY1500', `PKG_AXIAL_LAY(`$1', `$2', `$3', 1500)');
define(`PKG_ACY1600', `PKG_AXIAL_LAY(`$1', `$2', `$3', 1600)');


##  Axial polar component (typically capacitor),
#   100 through 1600 mil between pins
#
define(`PKG_ACY100P',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 100, 1)')
define(`PKG_ACY150P',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 150, 1)')
define(`PKG_ACY200P',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 200, 1)')
define(`PKG_ACY300P',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 300, 1)')
define(`PKG_ACY400P',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 400, 1)')
define(`PKG_ACY500P',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 500, 1)')
define(`PKG_ACY600P',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 600, 1)')
define(`PKG_ACY700P',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 700, 1)')
define(`PKG_ACY800P',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 800, 1)')
define(`PKG_ACY900P',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 900, 1)')
define(`PKG_ACY1000P', `PKG_AXIAL_LAY(`$1', `$2', `$3', 1000, 1)')
define(`PKG_ACY1100P', `PKG_AXIAL_LAY(`$1', `$2', `$3', 1100, 1)')
define(`PKG_ACY1200P', `PKG_AXIAL_LAY(`$1', `$2', `$3', 1200, 1)')
define(`PKG_ACY1300P', `PKG_AXIAL_LAY(`$1', `$2', `$3', 1300, 1)')
define(`PKG_ACY1400P', `PKG_AXIAL_LAY(`$1', `$2', `$3', 1400, 1)')
define(`PKG_ACY1500P', `PKG_AXIAL_LAY(`$1', `$2', `$3', 1500, 1)')
define(`PKG_ACY1600P', `PKG_AXIAL_LAY(`$1', `$2', `$3', 1600, 1)')

#
##  Axial diode (pin 1 is cathode)
#   ALF300 .. ALF1000
#
#
define(`PKG_ALF300', `PKG_DIODE_LAY(`$1', `$2', `$3',  300)');
define(`PKG_ALF400', `PKG_DIODE_LAY(`$1', `$2', `$3',  400)');
define(`PKG_ALF500', `PKG_DIODE_LAY(`$1', `$2', `$3',  500)');
define(`PKG_ALF600', `PKG_DIODE_LAY(`$1', `$2', `$3',  600)');
define(`PKG_ALF700', `PKG_DIODE_LAY(`$1', `$2', `$3',  700)');
define(`PKG_ALF800', `PKG_DIODE_LAY(`$1', `$2', `$3',  800)');
define(`PKG_ALF900', `PKG_DIODE_LAY(`$1', `$2', `$3',  900)');
define(`PKG_ALF1000',`PKG_DIODE_LAY(`$1', `$2', `$3', 1000)');

#
##  Bottom lead non-polar circular component (typically capacitor)
#   pin spacing 100 mil and up
#   RCY100 .. RCY1000
#
define(`PKG_RCY100',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 200)');
define(`PKG_RCY150',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 300)');
define(`PKG_RCY200',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 400)');
define(`PKG_RCY250',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 500)');
define(`PKG_RCY300',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 600)');
define(`PKG_RCY350',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 700)');
define(`PKG_RCY400',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 800)');
define(`PKG_RCY500',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 1000)');
define(`PKG_RCY600',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 1200)');
define(`PKG_RCY700',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 1400)');
define(`PKG_RCY800',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 1600)');
define(`PKG_RCY900',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 1800)');
define(`PKG_RCY1000', `PKG_RADIAL_CAN(`$1', `$2', `$3', 2000)');
define(`PKG_RCY1100', `PKG_RADIAL_CAN(`$1', `$2', `$3', 2200)');
define(`PKG_RCY1200', `PKG_RADIAL_CAN(`$1', `$2', `$3', 2400)');


##  Bottom lead polar circular component (typically capacitor)
#   pin spacing 100 mil and up
#
define(`PKG_RCY100P',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 200, 1)');
define(`PKG_RCY150P',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 300, 1)');
define(`PKG_RCY200P',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 400, 1)');
define(`PKG_RCY250P',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 500, 1)');
define(`PKG_RCY300P',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 600, 1)');
define(`PKG_RCY350P',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 700, 1)');
define(`PKG_RCY400P',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 800, 1)');
define(`PKG_RCY500P',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 1000, 1)');
define(`PKG_RCY600P',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 1200, 1)');
define(`PKG_RCY700P',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 1400, 1)');
define(`PKG_RCY800P',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 1600, 1)');
define(`PKG_RCY900P',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 1800, 1)');
define(`PKG_RCY1000P', `PKG_RADIAL_CAN(`$1', `$2', `$3', 2000, 1)');
define(`PKG_RCY1100P', `PKG_RADIAL_CAN(`$1', `$2', `$3', 2200, 1)');
define(`PKG_RCY1200P', `PKG_RADIAL_CAN(`$1', `$2', `$3', 2400, 1)');

#
##  Crystals
#
#
define(`PKG_HC49U',   `PKG_CRYSTAL_V(`$1', `$2', `$3', 192, 435, 183,  60, 32, 2)');
define(`PKG_HC49UH',  `PKG_CRYSTAL_H(`$1', `$2', `$3', 192, 435, 515,  60, 32, 2)');
define(`PKG_HC49U_3', `PKG_CRYSTAL_V(`$1', `$2', `$3',  96, 435, 183,  60, 32, 3)');
define(`PKG_HC49U_3H',`PKG_CRYSTAL_H(`$1', `$2', `$3',  96, 435, 515,  60, 32, 3)');
define(`PKG_HC51U',   `PKG_CRYSTAL_V(`$1', `$2', `$3', 485, 757, 352,  80, 40, 2)');
define(`PKG_HC51UH',  `PKG_CRYSTAL_H(`$1', `$2', `$3', 485, 757, 775,  80, 40, 2)');
define(`PKG_UM1',     `PKG_CRYSTAL_V(`$1', `$2', `$3', 148, 311, 126,  60, 32, 2)');
define(`PKG_UM1H',    `PKG_CRYSTAL_H(`$1', `$2', `$3', 148, 311, 322,  60, 32, 2)');
define(`PKG_UM1_3',   `PKG_CRYSTAL_V(`$1', `$2', `$3',  74, 311, 126,  60, 32, 3)');
define(`PKG_UM1_3H',  `PKG_CRYSTAL_H(`$1', `$2', `$3',  74, 311, 322,  60, 32, 3)');
#
#   HC49
#
define(`PKG_HC49',  `PKG_CRYSTAL(`$1', `$2', `$3', 300)');

#
##  Crystal oscillator
#   pins are NC, GND, CLK, VCC
#   OSC14
#
define(`PKG_OSC14',  `PKG_OSC(`$1', `$2', `$3')');

#
##  LED, size in mm (pin 1 is +, 2 is -)
#   LED3, LED5
#
define(`PKG_LED3',  `PKG_LED(`$1', `$2', `$3', 118)');
define(`PKG_LED5',  `PKG_LED(`$1', `$2', `$3', 236)');

#
## Transistor
#  TO3_90
#  TO3_45
#  TO126LAY
#  TO126W for wide
#  TO126S for standing
#  TO126SW for both
#  TO220
#  TO220W for wide
#  TO220S for standing
#  TO220SW for both
#
# as is TO3_90
# as is TO3_45
# as is TO5
# as is TO92
define(`PKG_TO126',  `PKG_TO126LAY(       `$1', `$2', `$3')');
define(`PKG_TO126S', `PKG_TO126LAY_WIDE(  `$1', `$2', `$3')');
define(`PKG_TO126W', `PKG_TO126STAND(     `$1', `$2', `$3')');
define(`PKG_TO126SW',`PKG_TO126STAND_WIDE(`$1', `$2', `$3')');
define(`PKG_TO220',  `PKG_TO220LAY(       `$1', `$2', `$3')');
define(`PKG_TO220S', `PKG_TO220LAY_WIDE(  `$1', `$2', `$3')');
define(`PKG_TO220W', `PKG_TO220STAND(     `$1', `$2', `$3')');
define(`PKG_TO220SW',`PKG_TO220STAND_WIDE(`$1', `$2', `$3')');

# these are just wrappers around the footprints from to.inc.
# they are done here to be in a standard format that will
# make them show up in the footprint window and also get
# generated via the m4lib_to_newlib.sh script
define(`PKG_TO18',   `PKG_TO18BASE(`$1', `$2', `$3')');
define(`PKG_TO39',   `PKG_TO39BASE(`$1', `$2', `$3')');
define(`PKG_TO92',   `PKG_TO92BASE(`$1', `$2', `$3')');

## diode in TO220
#
define(`PKG_TO220ACS', `PKG_TO220ACSTAND(`$1', `$2', `$3')');
define(`PKG_TO218',    `GENERIC_PL_POWER(`$1', `$2', `$3',  3, 219, 615, 200, 120,100, 60,   0)');
# aka TOP3
define(`PKG_TO247',    `GENERIC_PL_POWER(`$1', `$2', `$3',  3, 219, 630, 210, 130,100, 60,   0)');
# diode in TO247/TOP3
define(`PKG_TO247_2',  `GENERIC_PL_POWER(`$1', `$2', `$3',  2, 438, 630, 210, 130,100, 60,   0)');
# aka TOP3BIG
define(`PKG_TO264',    `GENERIC_PL_POWER(`$1', `$2', `$3',  3, 219, 800, 210, 130,100, 60,   0)');
# aka IPAK
define(`PKG_TO251',    `GENERIC_PL_POWER(`$1', `$2', `$3',  3,  90, 265, 100,  50, 70, 40,   0)');

define(`PKG_TO220ACSTAND',    `GENERIC_PL_POWER(`$1', `$2', `$3',  2, 200, 400, 180, 100, 80, 40,   0)')

# the definitions of PKG_TO220STAND PKG_TO220STAND-WIDE can be replaced by following lines:
#define(`PKG_TO220STAND',      `GENERIC_PL_POWER(`$1', `$2', `$3',  3, 100, 400, 180, 100, 80, 40,   0)')
#define(`PKG_TO220STAND-WIDE', `GENERIC_PL_POWER(`$1', `$2', `$3',  3, 100, 400, 180, 200, 80, 40,-100)')

#
##  Power IC, as in MULTIWATT15
#
# See the following:
# PENTAWATT: http://www.st.com/stonline/books/pdf/docs/9262.pdf
# HEPTAWATT: http://www.st.com/stonline/books/pdf/docs/5430.pdf
# MULTIWATT8: http://www.st.com/stonline/books/pdf/docs/5437.pdf
# MULTIWATT11: http://www.st.com/stonline/books/pdf/docs/5433.pdf
# MULTIWATT15: http://www.st.com/stonline/books/pdf/docs/5439.pdf

# GENERIC_PL_POWER
# $1: canonical name
# $2: name on PCB
# $3: value
# $4: pin count
# $5: pin spacing
# $6: body length (dimension parallel to rows of pins)
# $7: body width  (dimension perpendicular to rows of pins)
# $8: pin/tab spacing
# $9: pad size
# $10: drill size
# $11: pin row spacing

define(`PKG_PENTAWATT',       `GENERIC_PL_POWER(`$1', `$2', `$3',  5,  67, 409, 189, 177, 90, 60, 157)' );
define(`PKG_HEPTAWATT',       `GENERIC_PL_POWER(`$1', `$2', `$3',  7,  50, 409, 189, 110, 90, 60, 200)' );
define(`PKG_MULTIWATT8',      `GENERIC_PL_POWER(`$1', `$2', `$3',  8, 100, 800, 200, 115, 90, 60,   0)' );
define(`PKG_MULTIWATT11',     `GENERIC_PL_POWER(`$1', `$2', `$3', 11,  67, 800, 200, 180, 90, 60, 200)' );
define(`PKG_MULTIWATT15',     `GENERIC_PL_POWER(`$1', `$2', `$3', 15,  50, 800, 200, 180, 90, 60, 200)' );

#
## Jumper, i.e. single row headers
#
define(`PKG_JUMPER1',    `PKG_CONNECTOR(`$1', `$2', `$3',  1, 1)');
define(`PKG_JUMPER2',    `PKG_CONNECTOR(`$1', `$2', `$3',  2, 1)');
define(`PKG_JUMPER3',    `PKG_CONNECTOR(`$1', `$2', `$3',  3, 1)');
define(`PKG_JUMPER4',    `PKG_CONNECTOR(`$1', `$2', `$3',  4, 1)');
define(`PKG_JUMPER5',    `PKG_CONNECTOR(`$1', `$2', `$3',  5, 1)');
define(`PKG_JUMPER6',    `PKG_CONNECTOR(`$1', `$2', `$3',  6, 1)');
define(`PKG_JUMPER7',    `PKG_CONNECTOR(`$1', `$2', `$3',  7, 1)');
define(`PKG_JUMPER8',    `PKG_CONNECTOR(`$1', `$2', `$3',  8, 1)');
define(`PKG_JUMPER9',    `PKG_CONNECTOR(`$1', `$2', `$3',  9, 1)');
define(`PKG_JUMPER10',   `PKG_CONNECTOR(`$1', `$2', `$3', 10, 1)');
define(`PKG_JUMPER11',   `PKG_CONNECTOR(`$1', `$2', `$3', 11, 1)');
define(`PKG_JUMPER12',   `PKG_CONNECTOR(`$1', `$2', `$3', 12, 1)');
define(`PKG_JUMPER13',   `PKG_CONNECTOR(`$1', `$2', `$3', 13, 1)');
define(`PKG_JUMPER14',   `PKG_CONNECTOR(`$1', `$2', `$3', 14, 1)');
define(`PKG_JUMPER15',   `PKG_CONNECTOR(`$1', `$2', `$3', 15, 1)');
define(`PKG_JUMPER16',   `PKG_CONNECTOR(`$1', `$2', `$3', 16, 1)');
define(`PKG_JUMPER17',   `PKG_CONNECTOR(`$1', `$2', `$3', 17, 1)');
define(`PKG_JUMPER18',   `PKG_CONNECTOR(`$1', `$2', `$3', 18, 1)');
define(`PKG_JUMPER19',   `PKG_CONNECTOR(`$1', `$2', `$3', 19, 1)');
define(`PKG_JUMPER20',   `PKG_CONNECTOR(`$1', `$2', `$3', 20, 1)');
define(`PKG_JUMPER21',   `PKG_CONNECTOR(`$1', `$2', `$3', 21, 1)');
define(`PKG_JUMPER22',   `PKG_CONNECTOR(`$1', `$2', `$3', 22, 1)');
define(`PKG_JUMPER23',   `PKG_CONNECTOR(`$1', `$2', `$3', 23, 1)');
define(`PKG_JUMPER24',   `PKG_CONNECTOR(`$1', `$2', `$3', 24, 1)');
define(`PKG_JUMPER25',   `PKG_CONNECTOR(`$1', `$2', `$3', 25, 1)');
define(`PKG_JUMPER26',   `PKG_CONNECTOR(`$1', `$2', `$3', 26, 1)');

#
## MTA Connector, (0.100 inch pitch)
#
define(`PKG_MTA100_2',    `PKG_MTA_100(`$1', `$2', `$3',  2)');
define(`PKG_MTA100_3',    `PKG_MTA_100(`$1', `$2', `$3',  3)');
define(`PKG_MTA100_4',    `PKG_MTA_100(`$1', `$2', `$3',  4)');
define(`PKG_MTA100_5',    `PKG_MTA_100(`$1', `$2', `$3',  5)');
define(`PKG_MTA100_6',    `PKG_MTA_100(`$1', `$2', `$3',  6)');
define(`PKG_MTA100_7',    `PKG_MTA_100(`$1', `$2', `$3',  7)');
define(`PKG_MTA100_8',    `PKG_MTA_100(`$1', `$2', `$3',  8)');
define(`PKG_MTA100_9',    `PKG_MTA_100(`$1', `$2', `$3',  9)');
define(`PKG_MTA100_10',   `PKG_MTA_100(`$1', `$2', `$3', 10)');
define(`PKG_MTA100_11',   `PKG_MTA_100(`$1', `$2', `$3', 11)');
define(`PKG_MTA100_12',   `PKG_MTA_100(`$1', `$2', `$3', 12)');
define(`PKG_MTA100_13',   `PKG_MTA_100(`$1', `$2', `$3', 13)');
define(`PKG_MTA100_14',   `PKG_MTA_100(`$1', `$2', `$3', 14)');
define(`PKG_MTA100_15',   `PKG_MTA_100(`$1', `$2', `$3', 15)');

#
## Header connector, DIP pin numbering
#  Corresponding to gEDA symbols header2-1 .. header64-1
#
define(`PKG_HEADER2_1',  `PKG_CONNECTOR_DIL(`$1', `$2', `$3',  1)');
define(`PKG_HEADER4_1',  `PKG_CONNECTOR_DIL(`$1', `$2', `$3',  2)');
define(`PKG_HEADER6_1',  `PKG_CONNECTOR_DIL(`$1', `$2', `$3',  3)');
define(`PKG_HEADER8_1',  `PKG_CONNECTOR_DIL(`$1', `$2', `$3',  4)');
define(`PKG_HEADER10_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3',  5)');
define(`PKG_HEADER12_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3',  6)');
define(`PKG_HEADER14_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3',  7)');
define(`PKG_HEADER16_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3',  8)');
define(`PKG_HEADER18_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3',  9)');
define(`PKG_HEADER20_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 10)');
define(`PKG_HEADER22_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 11)');
define(`PKG_HEADER24_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 12)');
define(`PKG_HEADER26_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 13)');
define(`PKG_HEADER28_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 14)');
define(`PKG_HEADER30_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 15)');
define(`PKG_HEADER32_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 16)');
define(`PKG_HEADER34_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 17)');
define(`PKG_HEADER36_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 18)');
define(`PKG_HEADER38_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 19)');
define(`PKG_HEADER40_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 20)');
define(`PKG_HEADER42_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 21)');
define(`PKG_HEADER44_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 22)');
define(`PKG_HEADER46_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 23)');
define(`PKG_HEADER48_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 24)');
define(`PKG_HEADER50_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 25)');
define(`PKG_HEADER52_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 26)');
define(`PKG_HEADER54_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 27)');
define(`PKG_HEADER56_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 28)');
define(`PKG_HEADER58_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 29)');
define(`PKG_HEADER60_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 30)');
define(`PKG_HEADER62_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 31)');
define(`PKG_HEADER64_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 32)');
define(`PKG_HEADER80_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 40)');

#
## Header connector, ribbon cable numbering
#  corresponding to gEDA symbols header2-2 .. header64-2
#
define(`PKG_HEADER2_2',  `PKG_CONNECTOR(`$1', `$2', `$3',  1, 2)');
define(`PKG_HEADER4_2',  `PKG_CONNECTOR(`$1', `$2', `$3',  2, 2)');
define(`PKG_HEADER6_2',  `PKG_CONNECTOR(`$1', `$2', `$3',  3, 2)');
define(`PKG_HEADER8_2',  `PKG_CONNECTOR(`$1', `$2', `$3',  4, 2)');
define(`PKG_HEADER10_2', `PKG_CONNECTOR(`$1', `$2', `$3',  5, 2)');
define(`PKG_HEADER12_2', `PKG_CONNECTOR(`$1', `$2', `$3',  6, 2)');
define(`PKG_HEADER14_2', `PKG_CONNECTOR(`$1', `$2', `$3',  7, 2)');
define(`PKG_HEADER16_2', `PKG_CONNECTOR(`$1', `$2', `$3',  8, 2)');
define(`PKG_HEADER18_2', `PKG_CONNECTOR(`$1', `$2', `$3',  9, 2)');
define(`PKG_HEADER20_2', `PKG_CONNECTOR(`$1', `$2', `$3', 10, 2)');
define(`PKG_HEADER22_2', `PKG_CONNECTOR(`$1', `$2', `$3', 11, 2)');
define(`PKG_HEADER24_2', `PKG_CONNECTOR(`$1', `$2', `$3', 12, 2)');
define(`PKG_HEADER26_2', `PKG_CONNECTOR(`$1', `$2', `$3', 13, 2)');
define(`PKG_HEADER28_2', `PKG_CONNECTOR(`$1', `$2', `$3', 14, 2)');
define(`PKG_HEADER30_2', `PKG_CONNECTOR(`$1', `$2', `$3', 15, 2)');
define(`PKG_HEADER32_2', `PKG_CONNECTOR(`$1', `$2', `$3', 16, 2)');
define(`PKG_HEADER34_2', `PKG_CONNECTOR(`$1', `$2', `$3', 17, 2)');
define(`PKG_HEADER36_2', `PKG_CONNECTOR(`$1', `$2', `$3', 18, 2)');
define(`PKG_HEADER38_2', `PKG_CONNECTOR(`$1', `$2', `$3', 19, 2)');
define(`PKG_HEADER40_2', `PKG_CONNECTOR(`$1', `$2', `$3', 20, 2)');
define(`PKG_HEADER42_2', `PKG_CONNECTOR(`$1', `$2', `$3', 21, 2)');
define(`PKG_HEADER44_2', `PKG_CONNECTOR(`$1', `$2', `$3', 22, 2)');
define(`PKG_HEADER46_2', `PKG_CONNECTOR(`$1', `$2', `$3', 23, 2)');
define(`PKG_HEADER48_2', `PKG_CONNECTOR(`$1', `$2', `$3', 24, 2)');
define(`PKG_HEADER50_2', `PKG_CONNECTOR(`$1', `$2', `$3', 25, 2)');
define(`PKG_HEADER52_2', `PKG_CONNECTOR(`$1', `$2', `$3', 26, 2)');
define(`PKG_HEADER54_2', `PKG_CONNECTOR(`$1', `$2', `$3', 27, 2)');
define(`PKG_HEADER56_2', `PKG_CONNECTOR(`$1', `$2', `$3', 28, 2)');
define(`PKG_HEADER58_2', `PKG_CONNECTOR(`$1', `$2', `$3', 29, 2)');
define(`PKG_HEADER60_2', `PKG_CONNECTOR(`$1', `$2', `$3', 30, 2)');
define(`PKG_HEADER62_2', `PKG_CONNECTOR(`$1', `$2', `$3', 31, 2)');
define(`PKG_HEADER64_2', `PKG_CONNECTOR(`$1', `$2', `$3', 32, 2)');
define(`PKG_HEADER80_2', `PKG_CONNECTOR(`$1', `$2', `$3', 40, 2)');

#
## Header connector with latches
#
define(`PKG_DIN41651_10',  `PKG_DIN41_651LAY(  `$1', `$2', `$3', 10)');
define(`PKG_DIN41651_14',  `PKG_DIN41_651LAY(  `$1', `$2', `$3', 14)');
define(`PKG_DIN41651_16',  `PKG_DIN41_651LAY(  `$1', `$2', `$3', 16)');
define(`PKG_DIN41651_20',  `PKG_DIN41_651LAY(  `$1', `$2', `$3', 20)');
define(`PKG_DIN41651_26',  `PKG_DIN41_651LAY(  `$1', `$2', `$3', 26)');
define(`PKG_DIN41651_34',  `PKG_DIN41_651LAY(  `$1', `$2', `$3', 34)');
define(`PKG_DIN41651_40',  `PKG_DIN41_651LAY(  `$1', `$2', `$3', 40)');
define(`PKG_DIN41651_50',  `PKG_DIN41_651LAY(  `$1', `$2', `$3', 50)');
define(`PKG_DIN41651_60',  `PKG_DIN41_651LAY(  `$1', `$2', `$3', 60)');
define(`PKG_DIN41651_64',  `PKG_DIN41_651LAY(  `$1', `$2', `$3', 64)');

define(`PKG_DIN41651_10S', `PKG_DIN41_651STAND(`$1', `$2', `$3', 10)');
define(`PKG_DIN41651_14S', `PKG_DIN41_651STAND(`$1', `$2', `$3', 14)');
define(`PKG_DIN41651_16S', `PKG_DIN41_651STAND(`$1', `$2', `$3', 16)');
define(`PKG_DIN41651_20S', `PKG_DIN41_651STAND(`$1', `$2', `$3', 20)');
define(`PKG_DIN41651_26S', `PKG_DIN41_651STAND(`$1', `$2', `$3', 26)');
define(`PKG_DIN41651_34S', `PKG_DIN41_651STAND(`$1', `$2', `$3', 34)');
define(`PKG_DIN41651_40S', `PKG_DIN41_651STAND(`$1', `$2', `$3', 40)');
define(`PKG_DIN41651_50S', `PKG_DIN41_651STAND(`$1', `$2', `$3', 50)');
define(`PKG_DIN41651_60S', `PKG_DIN41_651STAND(`$1', `$2', `$3', 60)');
define(`PKG_DIN41651_64S', `PKG_DIN41_651STAND(`$1', `$2', `$3', 64)');

#
## DSUB connector, female/male
#  DB9F .. DB37F
#  DB9M .. DB37M
#
define(`PKG_DB9F',       `PKG_SUBD_FEMALE_LAY(`$1', `$2', `$3',  9)');
define(`PKG_DB15F',      `PKG_SUBD_FEMALE_LAY(`$1', `$2', `$3', 15)');
define(`PKG_DB25F',      `PKG_SUBD_FEMALE_LAY(`$1', `$2', `$3', 25)');
define(`PKG_DB37F',      `PKG_SUBD_FEMALE_LAY(`$1', `$2', `$3', 37)');
define(`PKG_DB9M',       `PKG_SUBD_MALE_LAY(  `$1', `$2', `$3',  9)');
define(`PKG_DB15M',      `PKG_SUBD_MALE_LAY(  `$1', `$2', `$3', 15)');
define(`PKG_DB25M',      `PKG_SUBD_MALE_LAY(  `$1', `$2', `$3', 25)');
define(`PKG_DB37M',      `PKG_SUBD_MALE_LAY(  `$1', `$2', `$3', 37)');

#
## DIN connector, 96 pin housing
#  DIN41612C96F ...
#
define(`PKG_DIN41612C96F', `PKG_DIN41_612FEMALE(`$1', `$2', `$3', `abc')');
define(`PKG_DIN41612C96M', `PKG_DIN41_612MALE(  `$1', `$2', `$3', `abc')');
define(`PKG_DIN41612C64F', `PKG_DIN41_612FEMALE(`$1', `$2', `$3', `ac')');
define(`PKG_DIN41612C64M', `PKG_DIN41_612MALE(  `$1', `$2', `$3', `ac')');

define(`PKG_DIN41612C96FS', `PKG_DIN41_612FEMALE_SMALL(`$1', `$2', `$3', `abc')');
define(`PKG_DIN41612C96MS', `PKG_DIN41_612MALE_SMALL(  `$1', `$2', `$3', `abc')');
define(`PKG_DIN41612C64FS', `PKG_DIN41_612FEMALE_SMALL(`$1', `$2', `$3', `ac')');
define(`PKG_DIN41612C64MS', `PKG_DIN41_612MALE_SMALL(  `$1', `$2', `$3', `ac')');

#
## Standard SMT resistor, capacitor etc
#  0201 .. 2706
#
#define(`PKG_0201',      `PKG_SMT_2PAD_MIL(  `$1', `$2', `$3',  20,   10)');
#define(`PKG_0402',      `PKG_SMT_2PAD_MIL(  `$1', `$2', `$3',  40,   20)');
#define(`PKG_0603',      `PKG_SMT_2PAD_MIL(  `$1', `$2', `$3',  60,   30)');
#define(`PKG_0805',      `PKG_SMT_2PAD_MIL(  `$1', `$2', `$3',  80,   50)');
#define(`PKG_1008',      `PKG_SMT_2PAD_MIL(  `$1', `$2', `$3', 100,   80)');
#define(`PKG_1206',      `PKG_SMT_2PAD_MIL(  `$1', `$2', `$3', 120,   60)');
#define(`PKG_1210',      `PKG_SMT_2PAD_MIL(  `$1', `$2', `$3', 120, 100)');
#define(`PKG_1806',      `PKG_SMT_2PAD_MIL(  `$1', `$2', `$3', 180,   60)');
#define(`PKG_1812',      `PKG_SMT_2PAD_MIL(  `$1', `$2', `$3', 180, 120)');
#define(`PKG_1825',      `PKG_SMT_2PAD_MIL(  `$1', `$2', `$3', 180, 250)');
define(`PKG_2706',      `PKG_SMT_2PAD_MIL(  `$1', `$2', `$3', 270,   60)');

# The following are recommendations from IPC-7351
# The naming convention, like 'CAPC0603L' follows the IPC standard.  At the end
# of each size is a package named like '0603', '1206' which is more or less a compromise part based 
# on the nominal IPC footprints for that package size.  Any additions to this section
# should follow the IPC naming convention and size.  
# 
# The base macro accepts the numbers directly (after multiplying by 100) from the IPC
# standard so they are fairly simple to add.

# $1:  canonical name
# $2:  name on PCB
# $3:  value
# $4:  pad X (size of pad in direction perpendicular to axis of part) [1/100 mm]
# $5:  pad Y (size of pad in direction parallel to axis of part) [1/100 mm]
# $6:  pad center to center spacing [1/100 mm]
# $7:  courtyard size in direction parallel to axis of part [1/100 mm] (V1)
# $8:  courtyard size in direction perpendicular to axis of part [1/100 mm] (V2)
# $9:  length of silk screen line [1/100 mm] (R1)
# $10: spacing of silk screen line [1/100 mm] (R2)

# 01005
# See for example Murata GRM02 series.
# package X is 0.2 mm +/- 0.02 mm
# package Y is 0.07 to 0.14 mm
# package inner pad edge to pad edge is 0.13 min
# package outer pad edge to outer pad edge is 0.4 mm +/- 0.02 mm
# package height is 0.2mm +/- 0.02 mm

# From page 10 of IPC-7351, Feb 2005,
# Zmax = length of pattern = Lmin (overall length of the component) + 2*Jt + sqrt(Cl^2 + F^2 + P^2)
# Gmin = Distance between lands of the pattern = Smax (distance between terminals) - 2*Jh - sqrt(Cs^2 + F^2 + P^2)
# Xmax = Width of pattern = Wmin (width of lead) + 2*Js + sqrt(Cw^2 + F^2 + P^2)
#
# C = component tolerance = *max - *min
# F = PCB tolerance - used 0.05
# P = placement tolerance - used 0.05
#
# X = Xmax = 0.18/0.18/0.23 round to get 0.20/0.20/0.25
# Y = 0.5 * (Zmax - Gmin)  
# center to center = 0.5 * (Zmax + Gmin)
# 
#
# Jt = 0.00/0.10/0.20
# Jh = -0.05/-0.05/-0.05
# Js = 0.00/0.00/0.05
# Round to nearest 0.05
# Courtyard excess 0.10/0.15/0.20
#
#
# Note that rounding to 0.05 mm produces a roundoff error of up to +/- 1 mil.  This can be significant
# when we're talking about gaps on the order of 6 mils.  So for this really tiny part, round to 0.01 mm
define(`PKG_CAPC0402L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  26,   15,  31,  56,  28,   0,   0)');
define(`PKG_CAPC0402N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  26,   25,  41,  81,  33,   0,   0)');
define(`PKG_CAPC0402M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  36,   35,  51, 106,  38,   0,   0)');

define(`PKG_INDC0402L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  26,   15,  31,  56,  28,   0,   0)');
define(`PKG_INDC0402N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  26,   25,  41,  81,  33,   0,   0)');
define(`PKG_INDC0402M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  36,   35,  51, 106,  38,   0,   0)');

define(`PKG_RESC0402L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  26,   15,  31,  56,  28,   0,   0)');
define(`PKG_RESC0402N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  26,   25,  41,  81,  33,   0,   0)');
define(`PKG_RESC0402M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  36,   35,  51, 106,  38,   0,   0)');

define(`PKG_01005',     `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  26,   25,  41,  81,  33,   0,   0)');

# 0201
define(`PKG_CAPC0603L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  30,   30,  50, 100,  60,   0,   0)');
define(`PKG_CAPC0603N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  40,   40,  60, 130,  80,   0,   0)');
define(`PKG_CAPC0603M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  50,   50,  80, 160,  90,   0,   0)');

define(`PKG_RESC0603L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  30,   30,  50, 100,  60,   0,   0)');
define(`PKG_RESC0603N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  40,   40,  60, 130,  80,   0,   0)');
define(`PKG_RESC0603M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  50,   50,  80, 160,  90,   0,   0)');

define(`PKG_0201',      `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  40,   40,  60, 130,  80,   0,   0)');

# 0402 (C,L = 0.6 mm high, R = 0.4 mm high)
define(`PKG_CAPC1005L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  55,   40,  90, 150,  80,   0,   0)');
define(`PKG_CAPC1005N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  65,   50,  90, 170, 100,   0,   0)');
define(`PKG_CAPC1005M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  75,   60, 100, 200, 120,   0,   0)');

define(`PKG_INDC1005L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  60,   50,  80, 150,  90,   0,   0)');
define(`PKG_INDC1005N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  70,   50,  80, 170, 100,   0,   0)');
define(`PKG_INDC1005M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  80,   70, 100, 200, 120,   0,   0)');

define(`PKG_RESC1005L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  55,   35,  90, 150,  80,   0,   0)');
define(`PKG_RESC1005N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  65,   45, 100, 170, 100,   0,   0)');
define(`PKG_RESC1005M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  75,   55, 110, 200, 120,   0,   0)');

define(`PKG_0402',      `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  70,   50,  80, 170, 100,   0,   0)');

# 0603 (C = 0.85 mm high, L = 0.95 mm high, R = 0.60 mm high)
define(`PKG_CAPC1608L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  90,   65, 120, 200, 120,   0,   0)');
define(`PKG_CAPC1608N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 100,   75, 130, 230, 130,   0,   0)');
define(`PKG_CAPC1608M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 110,   85, 140, 270, 150,   0,   0)');

define(`PKG_INDC1608L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  90,   65, 120, 200, 120,   0,   0)');
define(`PKG_INDC1608N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 100,   75, 130, 230, 130,   0,   0)');
define(`PKG_INDC1608M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 110,   85, 140, 270, 150,   0,   0)');

define(`PKG_RESC1608L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3',  90,   50, 120, 200, 120,   0,  90)');
define(`PKG_RESC1608N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 100,   60, 140, 230, 130,   0,   0)');
define(`PKG_RESC1608M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 110,   70, 150, 270, 150,  10, 110)');

define(`PKG_0603',      `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 100,   75, 130, 230, 130,   0,   0)');

# 0805 (C = 1.1 mm high, L = 1.2 mm high, R = 0.65 mm high)
define(`PKG_CAPC2012L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 140,  110, 160, 280, 170,   0,   0)');
define(`PKG_CAPC2012N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 150,  130, 180, 350, 200,   0,   0)');
define(`PKG_CAPC2012M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 160,  150, 200, 440, 260,   0,   0)');

define(`PKG_INDC2012L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 140,  110, 160, 280, 170,   0,   0)');
define(`PKG_INDC2012N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 150,  130, 180, 350, 200,   0,   0)');
define(`PKG_INDC2012M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 160,  150, 200, 440, 260,   0,   0)');

define(`PKG_RESC2012L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 130,  100, 160, 280, 160,  20, 130)');
define(`PKG_RESC2012N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 140,  120, 180, 350, 190,  20, 140)');
define(`PKG_RESC2012M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 150,  140, 200, 440, 250,  20, 150)');

define(`PKG_0805',      `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 150,  130, 180, 350, 200,  20, 140)');

# 1008 (L = 2.2 mm high)
define(`PKG_INDC2520L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 220,   90, 240, 340, 250,  80, 220)');
define(`PKG_INDC2520N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 230,  110, 260, 410, 280,  70, 230)');
define(`PKG_INDC2520M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 240,  130, 280, 500, 340,  80, 240)');

define(`PKG_1008',      `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 230,  110, 260, 410, 280,  70, 230)');

# 1206 (C = 1.35 mm high, L = 1.9 mm high, R = 0.71 mm high)
define(`PKG_CAPC3216L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 170,  110, 280, 400, 200,  90, 170)');
define(`PKG_CAPC3216N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 180,  130, 300, 470, 230, 100, 180)');
define(`PKG_CAPC3216M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 190,  150, 320, 560, 290, 100, 190)');

define(`PKG_INDC3216L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 180,  100, 300, 420, 210, 120, 180)');
define(`PKG_INDC3216N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 190,  120, 320, 490, 240, 120, 190)');
define(`PKG_INDC3216M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 200,  140, 340, 580, 300, 130, 200)');

define(`PKG_RESC3216L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 170,  110, 280, 400, 200,  90, 170)');
define(`PKG_RESC3216N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 180,  130, 300, 470, 230, 100, 180)');
define(`PKG_RESC3216M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 190,  150, 320, 560, 290, 100, 190)');

define(`PKG_1206',      `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 190,  130, 300, 490, 240, 120, 190)');

# 1210 (C = 1.35 mm high, L = 1.35 mm high, R = 0.71 mm high)
define(`PKG_CAPC3225L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 260,  110, 280, 400, 290,  90, 260)');
define(`PKG_CAPC3225N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 270,  130, 300, 470, 320, 100, 270)');
define(`PKG_CAPC3225M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 280,  150, 320, 560, 380, 100, 280)');

define(`PKG_INDC3225L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 260,  110, 280, 400, 290,  90, 260)');
define(`PKG_INDC3225N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 270,  130, 300, 470, 320, 100, 270)');
define(`PKG_INDC3225M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 280,  150, 320, 560, 380, 100, 280)');

define(`PKG_RESC3225L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 260,  110, 280, 400, 290,  90, 260)');
define(`PKG_RESC3225N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 270,  130, 300, 470, 320, 100, 270)');
define(`PKG_RESC3225M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 280,  150, 320, 560, 380, 100, 280)');

define(`PKG_1210',      `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 270,  130, 300, 470, 320, 100, 270)');

# 1806 (L = 1.9 mm high)

define(`PKG_INDC4509L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 110,  130, 400, 540, 150, 200, 110)');
define(`PKG_INDC4509N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 120,  150, 420, 610, 170, 190, 120)');
define(`PKG_INDC4509M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 130,  170, 440, 700, 230, 200, 130)');

define(`PKG_1806',      `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 120,  150, 420, 610, 170, 190, 120)');

# 1812 (C = 1.35 mm high, L = 1.75 mm high, R = 1.1 mm high)
define(`PKG_CAPC4532L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 330,  140, 380, 540, 370, 160, 330)');
define(`PKG_CAPC4532N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 340,  160, 400, 610, 390, 160, 340)');
define(`PKG_CAPC4532M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 350,  180, 420, 700, 450, 160, 350)');

define(`PKG_INDC4532L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 330,  140, 380, 540, 370, 160, 330)');
define(`PKG_INDC4532N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 340,  160, 400, 610, 390, 160, 340)');
define(`PKG_INDC4532M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 350,  180, 420, 700, 450, 160, 350)');

define(`PKG_RESC4532L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 330,  140, 380, 540, 370, 160, 330)');
define(`PKG_RESC4532N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 340,  160, 400, 610, 390, 160, 340)');
define(`PKG_RESC4532M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 350,  180, 420, 700, 450, 160, 350)');

define(`PKG_1806',      `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 340,  160, 400, 610, 390, 160, 340)');

# 1825 (C = 1.1 mm high, R = 1.35 mm high)
define(`PKG_CAPC4564L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 670,  140, 380, 540, 700, 160, 670)');
define(`PKG_CAPC4564N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 680,  160, 400, 610, 730, 160, 680)');
define(`PKG_CAPC4564M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 690,  180, 420, 700, 790, 160, 690)');

define(`PKG_RESC4564L', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 670,  140, 380, 540, 700, 160, 670)');
define(`PKG_RESC4564N', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 680,  160, 400, 610, 730, 160, 680)');
define(`PKG_RESC4564M', `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 690,  180, 420, 700, 790, 160, 690)');

define(`PKG_1825',      `PKG_SMT_2PAD_MM100(  `$1', `$2', `$3', 680,  160, 400, 610, 730, 160, 680)');



#
## 3-Pin SMT EMI Filter based on standard SMT sizes
#  See www.murata.com for example
#
define(`PKG_EMI0603',      `COMMON_SMT_3PAD_MIL(  `$1', `$2', `$3', 24, 24,  63,  47, 16, 10, 10)');
define(`PKG_EMI0805',      `COMMON_SMT_3PAD_MIL(  `$1', `$2', `$3', 32, 24,  79,  75, 24, 10, 10)');
define(`PKG_EMI1206',      `COMMON_SMT_3PAD_MIL(  `$1', `$2', `$3', 32, 28, 126,  79, 39, 10, 10)');
define(`PKG_EMI1806',      `COMMON_SMT_3PAD_MIL(  `$1', `$2', `$3', 39, 39, 177, 102, 59, 10, 10)');

#
## Tantalum SMT capacitor (pin 1 is +)
#  pin 1 is marked (and is presumably +), pin 2 is minus
#  EIA3216 .. EIA7343
#
define(`PKG_EIA3216',   `PKG_SMT_2PAD_EIA(  `$1', `$2', `$3', 32, 16)');
define(`PKG_EIA3528',   `PKG_SMT_2PAD_EIA(  `$1', `$2', `$3', 35, 28)');
define(`PKG_EIA6032',   `PKG_SMT_2PAD_EIA(  `$1', `$2', `$3', 60, 32)');
define(`PKG_EIA7343',   `PKG_SMT_2PAD_EIA(  `$1', `$2', `$3', 73, 43)');

define(`PKG_TANT_A',    `PKG_SMT_2PAD_EIA(  `$1', `$2', `$3', 32, 16)');
define(`PKG_TANT_B',    `PKG_SMT_2PAD_EIA(  `$1', `$2', `$3', 35, 28)');
define(`PKG_TANT_C',    `PKG_SMT_2PAD_EIA(  `$1', `$2', `$3', 60, 32)');
define(`PKG_TANT_D',    `PKG_SMT_2PAD_EIA(  `$1', `$2', `$3', 73, 43)');

#
## Surface mount electrolytic capacitor, number is dia in mm
define(`PKG_SME3',      `PKG_SMT_2PAD_EIA(  `$1', `$2', `$3', 33, 33)');
define(`PKG_SME4',      `PKG_SMT_2PAD_EIA(  `$1', `$2', `$3', 43, 43)');
define(`PKG_SME5',      `PKG_SMT_2PAD_EIA(  `$1', `$2', `$3', 53, 53)');
define(`PKG_SME6',      `PKG_SMT_2PAD_EIA(  `$1', `$2', `$3', 66, 66)');
define(`PKG_SME8',      `PKG_SMT_2PAD_EIA(  `$1', `$2', `$3', 84, 84)');
define(`PKG_SME10',     `PKG_SMT_2PAD_EIA(  `$1', `$2', `$3',104,104)');

#
## SMT diode (pin 1 is cathode)
#  (pin 2 is anode)
#  SOD110 ..
#
#  SOD23D is a SOT23 housing with pin numbers that match diodes
#  it can also be used for transistors that used this numbering
# dimensions of `PKG_SMT_DIODE( `$1', `$2', `$3', len=mm/10, wid=mm/10)'); #JG
# dimensions of `PKG_SM/EIT_2PAD_EIA( `$1', `$2', `$3', len=mm/10, wid=mm/10)'); #JG
#
define(`PKG_SOD80',    `PKG_SMT_2PAD_EIA(    `$1', `$2', `$3', 37, 16)');
define(`PKG_SOD87',    `PKG_SMT_2PAD_EIA(    `$1', `$2', `$3', 37, 21)');
define(`PKG_SOD106A',  `PKG_SMT_DIODE(       `$1', `$2', `$3', 55, 25)');
define(`PKG_SOD110',   `PKG_SMT_DIODE(       `$1', `$2', `$3', 21, 14)');
# BUG: pads are only 0.65 mm wide:
define(`PKG_SOD123',   `PKG_SMT_DIODE(       `$1', `$2', `$3', 40, 15)');
# BUG: pads are only 0.60 mm wide:
define(`PKG_SOD323',   `PKG_SMT_DIODE(       `$1', `$2', `$3', 27, 13)');
define(`PKG_SOD523',   `PKG_SMT_DIODE(       `$1', `$2', `$3', 12,  8)');
define(`PKG_DO214',   `PKG_SMT_DIODE(       `$1', `$2', `$3', 77, 33)'); #JG
define(`PKG_DO214AB',   `PKG_SMT_DIODE(       `$1', `$2', `$3', 79, 34)'); #JG
define(`PKG_SOT23D',   `PKG_SMT_TRANSISTOR2( `$1', `$2', `$3', 20, 21)');
define(`PKG_SOT323D',  `PKG_SMT_TRANSISTOR2( `$1', `$2', `$3', 13, 18)');

#
## SMT transistor, 3 pins
define(`PKG_SOT23',    `PKG_SMT_TRANSISTOR3( `$1', `$2', `$3', 20, 21)');
define(`PKG_SOT323',   `PKG_SMT_TRANSISTOR3( `$1', `$2', `$3', 13, 18)');
define(`PKG_SC90',     `PKG_SMT_TRANSISTOR3( `$1', `$2', `$3', 10, 15)');
define(`PKG_SC70_3',   `PKG_SMT_TRANSISTOR3( `$1', `$2', `$3', 13, 18)');

## SMT transistor, 4 pins
define(`PKG_SOT89',    `PKG_SMT_TRANSISTOR4X(`$1', `$2', `$3', 31, 31)');
define(`PKG_SOT143',   `PKG_SMT_TRANSISTOR4( `$1', `$2', `$3', 19, 21)');
define(`PKG_SOT223',   `PKG_SMT_TRANSISTOR4X(`$1', `$2', `$3', 46, 62)');
define(`PKG_SC70_4',   `PKG_SMT_TRANSISTOR4( `$1', `$2', `$3', 13, 18)');

## SMT transistor, 5 pins
# aka SOT23-5
define(`PKG_SOT25',    `PKG_SMT_TRANSISTOR5A( `$1', `$2', `$3', 20, 21)');
define(`PKG_SOT353',   `PKG_SMT_TRANSISTOR5A( `$1', `$2', `$3', 13, 18)');
define(`PKG_SC70_5',   `PKG_SMT_TRANSISTOR5A(`$1', `$2', `$3', 13, 18)');
define(`PKG_SC88A',   `PKG_SMT_TRANSISTOR5A(`$1', `$2', `$3', 13, 18)');

## SMT transistor, 6 pins
# aka SOT23-6
define(`PKG_SOT26',    `PKG_SMT_TRANSISTOR6( `$1', `$2', `$3', 20, 21)');
define(`PKG_SOT326',   `PKG_SMT_TRANSISTOR6( `$1', `$2', `$3', 13, 18)');
define(`PKG_SC70_6',   `PKG_SMT_TRANSISTOR6( `$1', `$2', `$3', 13, 18)');

## Pressure transducer
define(`PKG_MPAK',     `COMMON_SMT_TRANSISTORX_MIL(`$1', `$2', `$3', 150, 437, 20, `12345')');

