#! /bin/sh
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
#   RCS: $Id$
#
#
# create a sed script used to create the application default resource file
# and the man pages
# Usage: CreateSedScript.sh [imake defines]
#

# the file with the global constants
#
CONST=../globalconst.h

# a TAB plus one blank for some stupid old sed implementations
#
SPACE="[	 ]"

# some system need nawk
#
case `uname` in
	SunOS)		AWK=nawk;;
	*)			AWK=awk;;
esac

# extract defines passed from Makefile and create sed script
#
while [ $# -gt 0 ]
do
	echo $1 | \
	sed -e '/\//s//\\\//g' \
		-e '/^-D\([A-Za-z0-9_]*\)=\(.*\)/s//\/\1\/s\/\/\2\/g/g' \
		-e '/^[^\/]/d' \
		-e '/"/s///g'
	shift
done

# get defines from const.h and add them to script
# ignore comments and double '/' because they are used as seperator for
# the sed command itself
#
sed -e '/\/\*.*\*\//s///g' \
	-e '/\//s//\\\//g' \
	-e '/'"$SPACE"'+/s// /g' \
	-e '/"/s///g' $CONST |
	$AWK '
		BEGIN	{
			count = 0;
		}
		/^#define/	{
			while(substr($0, length($0), 1) == "\\")
			{
				line[count] = line[count] substr($0, 1, length($0)-1) " ";
				getline;
			}
			line[count] = line[count] $0;
			count++;
			next;
		}
		END	{
			for (i = 0; i < count; i++)
			{
				number = split(line[i], arg);
				printf("/%s/s//", arg[2]);
				for (a = 3; a < number; a++)
					printf("%s ", arg[a]);
				printf("%s/g\n", arg[a]);
			}
		}' 
