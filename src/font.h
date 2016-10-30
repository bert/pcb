/*!
 * \file src/font.h
 *
 * \brief Prototypes for font functions
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 *  PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996, 2004 Thomas Nau
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 */

#ifndef	PCB_FONT_H
#define	PCB_FONT_H

#include "global.h"
#include <glib.h>

FontType * CreateNewFontInLibrary(GSList ** library, char * name);
FontType * LoadFont(char * filename);
FontType * ChangeSystemFont(char * fontname);
FontType * FindFont(char * fontname);
GSList * FontsUsed();
int UnloadFont(GSList ** library, char * fontname);
void SetFontInfo (FontType *);


#endif
