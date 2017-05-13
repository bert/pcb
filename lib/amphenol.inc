# -*- m4 -*-
#
#                            COPYRIGHT
#
#   PCB, interactive printed circuit board design
#   Copyright (C) 2003, 2004 Dan McMahill
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
# 
# Amphenol Connectors


# ARFX1229 SMA Connector
# 5.08 mm (200 mil) outer pin spacing
# 1.70 mm (67 mil, #51 drill) hole sizes

define(`PKG_AMPHENOL_ARFX1229',
	`define(`PAD_SIZE', `100')
	define(`HOLE_SIZE', `67')
	define(`CPAD_SIZE', `100')
	define(`CHOLE_SIZE', `67')
	define(`PAD_SPACE', `200')
	define(`CENTER', `eval(PAD_SPACE/2)')
	define(`SILK', `70')
	define(`XMIN', `eval(-1*SILK)')
	define(`XMAX', `eval(PAD_SPACE+SILK)')
	define(`YMIN', `eval(-1*SILK)')
	define(`YMAX', `eval(PAD_SPACE+SILK)')
Element(0x00 "$1" "`$2'" "$3" eval(XMIN+20) eval(YMAX+20) 0 100 0x00)
(

	Pin(CENTER     CENTER     CPAD_SIZE  CHOLE_SIZE  "1" 0x01)
 	Pin(0          0          PAD_SIZE   HOLE_SIZE   "2" 0x01)
 	Pin(0          PAD_SPACE  PAD_SIZE   HOLE_SIZE   "3" 0x01)
 	Pin(PAD_SPACE  0          PAD_SIZE   HOLE_SIZE   "4" 0x01)
 	Pin(PAD_SPACE  PAD_SPACE  PAD_SIZE   HOLE_SIZE   "5" 0x01)

	# silk screen
	# ends
	ElementLine(XMIN YMIN XMIN YMAX 10)
	ElementLine(XMAX YMAX XMAX YMIN 10)
	ElementLine(XMIN YMIN XMAX YMIN 10)
	ElementLine(XMAX YMAX XMIN YMAX 10)
	Mark(CENTER CENTER)
)')

# XXX once we get some sort of annotation layer in PCB, the right
# angle connectors should have an annotation showing where the connector
# really goes to.  Same for a keep out layer.
define(`PKG_AMPHENOL_ARFX1230',`PKG_AMPHENOL_ARFX1229(`$1', `$2', `$3')')
define(`PKG_AMPHENOL_ARFX1231',`PKG_AMPHENOL_ARFX1229(`$1', `$2', `$3')')
define(`PKG_AMPHENOL_ARFX1232',`PKG_AMPHENOL_ARFX1229(`$1', `$2', `$3')')

