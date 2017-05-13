# -*- m4 -*-
#
#                            COPYRIGHT
#
#   PCB, interactive printed circuit board design
#   Copyright (C) 2017 Dan McMahill
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; version 2 of the License
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
#
#
#  United Chemi-Con Specific Footprints



# EXTRACT_BEGIN

#
## GPA Series or Radial Electrolytic Capacitors
#

#
# _M = most
# _N = nominal
# _L = leaset

define(`PKG_CHEMICON_GPA16X35_M', `PKG_CAPPRD750W80D1600H3150A(  `$1', `$2', `$3')');
define(`PKG_CHEMICON_GPA16X35_N', `PKG_CAPPRD750W80D1600H3150B(  `$1', `$2', `$3')');
define(`PKG_CHEMICON_GPA16X35_L', `PKG_CAPPRD750W80D1600H3150C(  `$1', `$2', `$3')');


# EXTRACT_END
