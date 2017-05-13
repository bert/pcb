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

#-----------------------------------------------------------------
# ma librairie --- C. PETER 14 11 99
#
# l'origine 0 0 est en haut a gauche (x vers la droite, y vers le bas)
# Element(0x00 "$1" "`$2'" "$3" pos_x_text pos_y_text ? taille_text 0x00)
# PIN(pos_x, pos_y, diametre, percage, type?) type : 1 carre, 2 ronde
# ElementLine(debut_x debut_y fin_x fin_y epaisseur-trait)
# ElementArc(centre_x centre_y rayon_x rayon_y debut_arc fin_arc ep-trait)
# Mark fixe la position du curseur lors de la creation des composants

# -------------------------------------------------------------------
# R_AJ_H
# Pin 1 is 1, 2 is c, 3 is 2
# 
# $1: canonical name
# $2: name on PCB
# $3: value
#
define(`PKG_R_AJ_H',
`Element(0x00 "$1" "`$2'" "$3" 100 325 0 100 0x00)
(
        PIN(50, 450, 80, 30, 1)
        PIN(250, 450, 80, 30, 3)
        PIN(150, 50, 80, 30, 2)
	ElementLine(0 150 0 500 20)
	ElementLine(0 500 300 500 20)
	ElementLine(300 500 300 150 20)
	ElementArc(150 150 100 100 0 360 10)
	ElementArc(150 150 150 150 0 -180 20)
        Mark(50 450)
)')


# -------------------------------------------------------------------
# R_AJ_V
# Pin 1 is 1, 2 is c, 3 is 2
# 
# $1: canonical name
# $2: name on PCB
# $3: value
#
define(`PKG_R_AJ_V',
`Element(0x00 "$1" "`$2'" "$3" 100 45 0 100 0x00)
(
        PIN(50, 50, 80, 30, 1)
        PIN(250, 50, 80, 30, 3)
        PIN(150, 0, 80, 30, 2)
	ElementLine(0 0 0 100 20)
	ElementLine(0 0 300 0 20)
	ElementLine(300 0 300 100 20)
	ElementLine(0 100 300 100 20)
        Mark(50 50)
)')


# -------------------------------------------------------------------


