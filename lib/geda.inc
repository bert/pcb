# -*- m4 -*-
#   $Id$
#
#   gEDA compatible footprint names
#
#   the real component definitions are in the *.inc files
#
#   revisions:
#   Oct 10 2002 - Egil Kvaleberg <egil@kvaleberg.no>
#   Initial version
#

#
##  Dual in-line package, narrow (300 mil)
#   DIP6 .. DIP64, DIP24N, DIP28N
#
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
##  Dual in-line package, wide (600 mil)
define(`PKG_DIP24',  `PKG_N(`$1', `$2', `$3',  24,  600)');
define(`PKG_DIP28',  `PKG_N(`$1', `$2', `$3',  28,  600)');
define(`PKG_DIP32',  `PKG_N(`$1', `$2', `$3',  32,  600)');
define(`PKG_DIP36',  `PKG_N(`$1', `$2', `$3',  36,  600)');
define(`PKG_DIP40',  `PKG_N(`$1', `$2', `$3',  40,  600)');
define(`PKG_DIP42',  `PKG_N(`$1', `$2', `$3',  42,  600)');
define(`PKG_DIP48',  `PKG_N(`$1', `$2', `$3',  48,  600)');
##  Dual in-line package, wide (900 mil)
define(`PKG_DIP64',  `PKG_N(`$1', `$2', `$3',  64,  900)');

#
##  Shrink dual in-line package, wide (400 mil)
# BUG: reduce pad size for SDIP
# BUG: SDIP 70 mil pitch 64 pins
#
define(`PKG_SDIP18',  `PKG_N(`$1', `$2', `$3', 18,  300)');
define(`PKG_SDIP22',  `PKG_N(`$1', `$2', `$3', 22,  300)');
define(`PKG_SDIP24',  `PKG_N(`$1', `$2', `$3', 24,  300)');
#
##  Shrink dual in-line package, wide (400 mil)
define(`PKG_SDIP30',  `PKG_N(`$1', `$2', `$3', 30,  400)');
define(`PKG_SDIP32',  `PKG_N(`$1', `$2', `$3', 32,  400)');
#
##  Shrink dual in-line package, wide (600 mil)
define(`PKG_SDIP42',  `PKG_N(`$1', `$2', `$3', 42,  600)');

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
define(`PKG_SO8',     `PKG_SO( `$1', `$2', `$3',  8, 244, 5000)');
define(`PKG_SO14',    `PKG_SO( `$1', `$2', `$3', 14, 244, 5000)');
define(`PKG_SO16',    `PKG_SO( `$1', `$2', `$3', 16, 244, 5000)');
define(`PKG_SO18N',   `PKG_SO( `$1', `$2', `$3', 18, 244, 5000)');
define(`PKG_SO20N',   `PKG_SO( `$1', `$2', `$3', 20, 244, 5000)');

##  Small outline package, medium (200mil)
#   aka 8S2
define(`PKG_SO8M',    `PKG_SO( `$1', `$2', `$3',  8, 330, 5000)');
define(`PKG_SO18M',   `PKG_SO( `$1', `$2', `$3', 18, 330, 5000)');
define(`PKG_SO20M',   `PKG_SO( `$1', `$2', `$3', 20, 330, 5000)');
define(`PKG_SO22M',   `PKG_SO( `$1', `$2', `$3', 22, 330, 5000)');
define(`PKG_SO24M',   `PKG_SO( `$1', `$2', `$3', 24, 330, 5000)');

##  Small outline package, wide (300mil)
define(`PKG_SO8W',    `PKG_SO( `$1', `$2', `$3',  8, 419, 5000)');
define(`PKG_SO14W',   `PKG_SO( `$1', `$2', `$3', 14, 419, 5000)');
define(`PKG_SO16W',   `PKG_SO( `$1', `$2', `$3', 16, 419, 5000)');
define(`PKG_SO18W',   `PKG_SO( `$1', `$2', `$3', 18, 419, 5000)');
define(`PKG_SO20W',   `PKG_SO( `$1', `$2', `$3', 20, 419, 5000)');
define(`PKG_SO24W',   `PKG_SO( `$1', `$2', `$3', 24, 419, 5000)');
define(`PKG_SO28',    `PKG_SO( `$1', `$2', `$3', 28, 419, 5000)');
define(`PKG_SO32',    `PKG_SO( `$1', `$2', `$3', 32, 419, 5000)');

##  Small outline package, wider (325mil)
define(`PKG_SO28W',   `PKG_SO( `$1', `$2', `$3', 28, 474, 5000)');

##  Small outline package, wider (450mil)
define(`PKG_SO32W',   `PKG_SO( `$1', `$2', `$3', 32, 567, 5000)');

##  Small outline package, wide (525mil)
define(`PKG_SO44',    `PKG_SO( `$1', `$2', `$3', 44, 634, 5000)');

# NOTE: There is also SOJ20,28,32,40,42

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
define(`PKG_MSOP10',   `PKG_TSOP(  `$1', `$2', `$3', 8, 199, 1969)');

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

##  Thin shrink small outline package
# BUG: TSSOP 14, 20, 28 are all 4.4mm
define(`PKG_TSSOP8',   `PKG_TSSOP( `$1', `$2', `$3',  8, 260, 2600)');

define(`PKG_TSSOP48',  `PKG_TSOP(  `$1', `$2', `$3', 48, 319, 2000)');
define(`PKG_TSSOP56',  `PKG_TSOP(  `$1', `$2', `$3', 56, 319, 2000)');
define(`PKG_TSSOP64',  `PKG_TSOP(  `$1', `$2', `$3', 64, 319, 2000)');

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

define(`PKG_LQFP24_4',   `PKG_LQFP_50(`$1',`$2',`$3', `24', `4')');
define(`PKG_LQFP32_5',   `PKG_LQFP_50(`$1',`$2',`$3', `32', `5')');
define(`PKG_LQFP32_7',   `PKG_LQFP_80(`$1',`$2',`$3', `32', `7')');
define(`PKG_LQFP44_10',  `PKG_LQFP_80(`$1',`$2',`$3', `44',`10')');
define(`PKG_LQFP48_7',   `PKG_LQFP_50(`$1',`$2',`$3', `48', `7')');
define(`PKG_LQFP48_12',  `PKG_LQFP_80(`$1',`$2',`$3', `48',`12')');
define(`PKG_LQFP52_10',  `PKG_LQFP_65(`$1',`$2',`$3', `52',`10')');
define(`PKG_LQFP64_7',   `PKG_LQFP_40(`$1',`$2',`$3', `64', `7')');
define(`PKG_LQFP64_10',  `PKG_LQFP_50(`$1',`$2',`$3', `64',`10')');
define(`PKG_LQFP64_14',  `PKG_LQFP_80(`$1',`$2',`$3', `64',`14')');
define(`PKG_LQFP72_10',  `PKG_LQFP_50(`$1',`$2',`$3', `72',`10')');
define(`PKG_LQFP80_12',  `PKG_LQFP_50(`$1',`$2',`$3', `80',`12')');
define(`PKG_LQFP80_14',  `PKG_LQFP_65(`$1',`$2',`$3', `80',`14')');
define(`PKG_LQFP100_10', `PKG_LQFP_40(`$1',`$2',`$3', `72',`10')');
define(`PKG_LQFP100_14', `PKG_LQFP_50(`$1',`$2',`$3',`100',`14')');
define(`PKG_LQFP112_20', `PKG_LQFP_65(`$1',`$2',`$3',`112',`20')');
define(`PKG_LQFP120_16', `PKG_LQFP_50(`$1',`$2',`$3',`120',`16')');
define(`PKG_LQFP128_14', `PKG_LQFP_40(`$1',`$2',`$3',`128',`14')');
define(`PKG_LQFP144_20', `PKG_LQFP_50(`$1',`$2',`$3',`144',`20')');
define(`PKG_LQFP160_24', `PKG_LQFP_50(`$1',`$2',`$3',`160',`24')');
define(`PKG_LQFP176_24', `PKG_LQFP_50(`$1',`$2',`$3',`176',`24')');

define(`PKG_QFP32_7',   `PKG_QFP_80(`$1',`$2',`$3', `32', `7')');
define(`PKG_QFP44_10',  `PKG_QFP_80(`$1',`$2',`$3', `44',`10')');
define(`PKG_QFP52_10',  `PKG_QFP_65(`$1',`$2',`$3', `52',`10')');
define(`PKG_QFP64_14',  `PKG_QFP_80(`$1',`$2',`$3', `64',`14')');
define(`PKG_QFP80_14',  `PKG_QFP_65(`$1',`$2',`$3', `80',`14')');
define(`PKG_QFP100_14', `PKG_QFP_50(`$1',`$2',`$3',`100',`14')');
define(`PKG_QFP120_28', `PKG_QFP_80(`$1',`$2',`$3',`120',`28')');
define(`PKG_QFP128_28', `PKG_QFP_80(`$1',`$2',`$3',`128',`28')');
define(`PKG_QFP144_28', `PKG_QFP_65(`$1',`$2',`$3',`144',`28')');
define(`PKG_QFP160_28', `PKG_QFP_65(`$1',`$2',`$3',`160',`28')');
define(`PKG_QFP208_28', `PKG_QFP_50(`$1',`$2',`$3',`208',`28')');
define(`PKG_QFP160_28', `PKG_QFP_65(`$1',`$2',`$3',`160',`28')');
define(`PKG_QFP240_32', `PKG_QFP_50(`$1',`$2',`$3',`240',`32')');
define(`PKG_QFP304_40', `PKG_QFP_50(`$1',`$2',`$3',`304',`40')');
#
##  Rectangular Quad-side flat pack

define(`PKG_QFP64_R',   `PKG_QFP_100(`$1',`$2',`$3', `64',`0')');
define(`PKG_QFP80_R',   `PKG_QFP_80L(`$1',`$2',`$3', `80',`0')');
define(`PKG_QFP100_R',  `PKG_QFP_65L(`$1',`$2',`$3',`100',`0')');
define(`PKG_QFP128_R',  `PKG_QFP_50L(`$1',`$2',`$3',`128',`0')');
define(`PKG_LQFP128_R', `PKG_LQFP_50(`$1',`$2',`$3',`128',`0')');

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
#   300 through 1000 mil between pins
#   AXN300 .. AXN1000
#
define(`PKG_ACY300',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 300)');
define(`PKG_ACY400',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 400)');
define(`PKG_ACY500',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 500)');
define(`PKG_ACY600',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 600)');
define(`PKG_ACY800',  `PKG_AXIAL_LAY(`$1', `$2', `$3', 800)');
define(`PKG_ACY1000', `PKG_AXIAL_LAY(`$1', `$2', `$3', 1000)');

#
##  Axial diode (pin 1 is cathode)
#   ALF300 .. ALF1000
#
#
define(`PKG_ALF300', `PKG_DIODE_LAY(`$1', `$2', `$3',  300)');
define(`PKG_ALF400', `PKG_DIODE_LAY(`$1', `$2', `$3',  400)');
define(`PKG_ALF500', `PKG_DIODE_LAY(`$1', `$2', `$3',  500)');
define(`PKG_ALF600', `PKG_DIODE_LAY(`$1', `$2', `$3',  600)');
define(`PKG_ALF800', `PKG_DIODE_LAY(`$1', `$2', `$3',  800)');
define(`PKG_ALF1000',`PKG_DIODE_LAY(`$1', `$2', `$3', 1000)');

#
##  Bottom lead non-polar circular component (typically capacitor)
#   pin spacing 100 mil and up
#   RCY100 .. RCY1000
#
define(`PKG_RCY100',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 200)');
define(`PKG_RCY200',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 400)');
define(`PKG_RCY300',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 600)');
define(`PKG_RCY400',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 800)');
define(`PKG_RCY500',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 1000)');
define(`PKG_RCY600',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 1200)');
define(`PKG_RCY800',  `PKG_RADIAL_CAN(`$1', `$2', `$3', 1600)');
define(`PKG_RCY1000', `PKG_RADIAL_CAN(`$1', `$2', `$3', 2000)');

#
##  Bottom lead rectangular non-polar component (typically capacitor)
#   pin spacing 100 mil and up
#   BRE100 .. BRE1200
#
define(`PKG_BRE100',  `PKG_RECTANGULAR2N(`$1', `$2', `$3',  200, 100)');
define(`PKG_BRE200',  `PKG_RECTANGULAR2N(`$1', `$2', `$3',  400, 100)');
define(`PKG_BRE300',  `PKG_RECTANGULAR2N(`$1', `$2', `$3',  600, 150)');
define(`PKG_BRE400',  `PKG_RECTANGULAR2( `$1', `$2', `$3',  600, 200)');
define(`PKG_BRE500',  `PKG_RECTANGULAR2( `$1', `$2', `$3',  750, 250)');
define(`PKG_BRE600',  `PKG_RECTANGULAR2( `$1', `$2', `$3', 1200, 300)');
define(`PKG_BRE700',  `PKG_RECTANGULAR2( `$1', `$2', `$3', 1050, 350)');
define(`PKG_BRE800',  `PKG_RECTANGULAR2( `$1', `$2', `$3', 1200, 400)');
define(`PKG_BRE900',  `PKG_RECTANGULAR2( `$1', `$2', `$3', 1350, 450)');
define(`PKG_BRE1000', `PKG_RECTANGULAR2( `$1', `$2', `$3', 1500, 500)');
define(`PKG_BRE1100', `PKG_RECTANGULAR2( `$1', `$2', `$3', 1650, 550)');
define(`PKG_BRE1200', `PKG_RECTANGULAR2( `$1', `$2', `$3', 1800, 600)');

#
##  Crystal
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
define(`PKG_LED3',  `PKG_LEDX(`$1', `$2', `$3', 60)');
define(`PKG_LED5',  `PKG_LEDX(`$1', `$2', `$3', 100)');

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

#
#  Power IC
#
# as is MULTIWATT15

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
define(`PKG_JUMPER14',   `PKG_CONNECTOR(`$1', `$2', `$3', 14, 1)');
define(`PKG_JUMPER16',   `PKG_CONNECTOR(`$1', `$2', `$3', 16, 1)');
define(`PKG_JUMPER18',   `PKG_CONNECTOR(`$1', `$2', `$3', 18, 1)');
define(`PKG_JUMPER20',   `PKG_CONNECTOR(`$1', `$2', `$3', 20, 1)');

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
define(`PKG_HEADER26_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 13)');
define(`PKG_HEADER30_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 15)');
define(`PKG_HEADER34_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 17)');
define(`PKG_HEADER40_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 20)');
define(`PKG_HEADER50_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 25)');
define(`PKG_HEADER64_1', `PKG_CONNECTOR_DIL(`$1', `$2', `$3', 32)');

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
define(`PKG_HEADER26_2', `PKG_CONNECTOR(`$1', `$2', `$3', 13, 2)');
define(`PKG_HEADER30_2', `PKG_CONNECTOR(`$1', `$2', `$3', 15, 2)');
define(`PKG_HEADER34_2', `PKG_CONNECTOR(`$1', `$2', `$3', 17, 2)');
define(`PKG_HEADER40_2', `PKG_CONNECTOR(`$1', `$2', `$3', 20, 2)');
define(`PKG_HEADER50_2', `PKG_CONNECTOR(`$1', `$2', `$3', 25, 2)');
define(`PKG_HEADER60_2', `PKG_CONNECTOR(`$1', `$2', `$3', 30, 2)');
define(`PKG_HEADER64_2', `PKG_CONNECTOR(`$1', `$2', `$3', 32, 2)');

#
## Header connector with latches
#
define(`PKG_DIN41651_10',  `PKG_DIN41_651LAY(  `$1', `$2', `$3', 10)');
define(`PKG_DIN41651_14',  `PKG_DIN41_651LAY(  `$1', `$2', `$3', 14)');
define(`PKG_DIN41651_16',  `PKG_DIN41_651LAY(  `$1', `$2', `$3', 16)');
define(`PKG_DIN41651_20',  `PKG_DIN41_651LAY(  `$1', `$2', `$3', 10)');
define(`PKG_DIN41651_26',  `PKG_DIN41_651LAY(  `$1', `$2', `$3', 26)');
define(`PKG_DIN41651_34',  `PKG_DIN41_651LAY(  `$1', `$2', `$3', 34)');
define(`PKG_DIN41651_40',  `PKG_DIN41_651LAY(  `$1', `$2', `$3', 40)');
define(`PKG_DIN41651_50',  `PKG_DIN41_651LAY(  `$1', `$2', `$3', 50)');
define(`PKG_DIN41651_60',  `PKG_DIN41_651LAY(  `$1', `$2', `$3', 60)');
define(`PKG_DIN41651_64',  `PKG_DIN41_651LAY(  `$1', `$2', `$3', 64)');

define(`PKG_DIN41651_10S', `PKG_DIN41_651STAND(`$1', `$2', `$3', 10)');
define(`PKG_DIN41651_14S', `PKG_DIN41_651STAND(`$1', `$2', `$3', 14)');
define(`PKG_DIN41651_16S', `PKG_DIN41_651STAND(`$1', `$2', `$3', 16)');
define(`PKG_DIN41651_20S', `PKG_DIN41_651STAND(`$1', `$2', `$3', 10)');
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
define(`PKG_0201',      `PKG_SMT_2PAD_MIL(  `$1', `$2', `$3',  20,   10)');
define(`PKG_0402',      `PKG_SMT_2PAD_MIL(  `$1', `$2', `$3',  40,   20)');
define(`PKG_0603',      `PKG_SMT_2PAD_MIL(  `$1', `$2', `$3',  60,   30)');
define(`PKG_0805',      `PKG_SMT_2PAD_MIL(  `$1', `$2', `$3',  80,   50)');
define(`PKG_1206',      `PKG_SMT_2PAD_MIL(  `$1', `$2', `$3', 120,   60)');
define(`PKG_1210',      `PKG_SMT_2PAD_MIL(  `$1', `$2', `$3', 120, 100)');
define(`PKG_1806',      `PKG_SMT_2PAD_MIL(  `$1', `$2', `$3', 180,   60)');
define(`PKG_1812',      `PKG_SMT_2PAD_MIL(  `$1', `$2', `$3', 180, 120)');
define(`PKG_1825',      `PKG_SMT_2PAD_MIL(  `$1', `$2', `$3', 180, 250)');
define(`PKG_2706',      `PKG_SMT_2PAD_MIL(  `$1', `$2', `$3', 270,   60)');

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
define(`PKG_SME4',      `PKG_SMT_2PAD_EIA(  `$1', `$2', `$3', 53, 53)');
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
define(`PKG_SOT25',    `PKG_SMT_TRANSISTOR5( `$1', `$2', `$3', 20, 21)');
define(`PKG_SOT325',   `PKG_SMT_TRANSISTOR5( `$1', `$2', `$3', 13, 18)');
define(`PKG_SC70_5',   `PKG_SMT_TRANSISTOR5A(`$1', `$2', `$3', 13, 18)');

## SMT transistor, 6 pins
# aka SOT23-6
define(`PKG_SOT26',    `PKG_SMT_TRANSISTOR6( `$1', `$2', `$3', 20, 21)');
define(`PKG_SOT326',   `PKG_SMT_TRANSISTOR6( `$1', `$2', `$3', 13, 18)');
define(`PKG_SC70_6',   `PKG_SMT_TRANSISTOR6( `$1', `$2', `$3', 13, 18)');

## Pressure transducer
define(`PKG_MPAK',     `COMMON_SMT_TRANSISTORX_MIL(`$1', `$2', `$3', 150, 437, 20, `12345')');

#
## Virtual component, no footprint
#
define(`PKG_DUMMY');
