#!/bin/sh
#
# Copyright (c) 2003, 2004, 2005, 2006, 2007 Dan McMahill

#  This program is free software; you can redistribute it and/or modify
#  it under the terms of version 2 of the GNU General Public License as
#  published by the Free Software Foundation
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301 USA.
#
# All rights reserved.
#
# This code was derived from code written by Dan McMahill as part of the
# latex-mk testsuite.  The original code was covered by a BSD license
# but the copyright holder is releasing the version for gerbv under the GPL.

usage() {
cat <<EOF

$0 -- Recursively compare all .png files between two directories.

$0 -h|--help
$0 [-o | --out <outdir>] dir1 dir2

OVERVIEW

The $0 script is used to compare all png files which exist in both 
<dir1> and <dir2>.  The comparison indicates if the files differ
graphically as well as providing a visual difference output.
This script is used to help verify changes made to the m4 libraries
since a simple change in a macro may have far reaching and unintended
results.

The results are placed in <outdir> which defaults to "mismatch".

EXAMPLES

$0 pcblib-newlib.orig pcblib-newlib.new


EOF
}

show_sep() {
	echo "----------------------------------------------------------------------"
}

all_tests=""
while test -n "$1"
	do
	case "$1" in
		
		-h|--help)
			usage
			exit 0
			;;
		
		-o|--out)
			ERRDIR="$2"
			shift 2
			;;
		
		-*)
			echo "unknown option: $1"
			exit 1
			;;
		
		*)
			break
			;;
		
  esac
done

if test $# -ne 2 ; then
	usage
	exit 1
fi

dir1="$1"
dir2="$2"

if test ! -d $dir1 ; then
	echo "$dir1 does not exist or is not a directory"
	usage
	exit 1
fi

if test ! -d $dir2 ; then
	echo "$dir2 does not exist or is not a directory"
	usage
	exit 1
fi
	
# Source directory
srcdir=${srcdir:-.}

# various ImageMagick tools
ANIMATE=${ANIMATE:-animate}
COMPARE=${COMPARE:-compare}
COMPOSITE=${COMPOSITE:-composite}
CONVERT=${CONVERT:-convert}
DISPLAY=${DISPLAY:-display}
MONTAGE=${MONTAGE:-montage}

# golden directories
ERRDIR=${ERRDIR:-mismatch}

# some system tools
AWK=${AWK:-awk}

# create output directory
if test ! -d $ERRDIR ; then
	mkdir -p $ERRDIR
	if test $? -ne 0 ; then
	echo "Failed to create output directory ${ERRDIR}"
	exit 1
	fi
fi


# fail/pass/total counts
fail=0
pass=0
skip=0
tot=0

cat << EOF

srcdir				${srcdir}
top_srcdir			${top_srcdir}

AWK					${AWK}
ERRDIR				${ERRDIR}

ImageMagick Tools:

ANIMATE				${ANIMATE}
COMPARE				${COMPARE}
COMPOSITE			 ${COMPOSITE}
CONVERT				${CONVERT}
DISPLAY				${DISPLAY}
MONTAGE				${MONTAGE}

EOF

find $dir1 -name \*.png -print | while read -r t ; do
	show_sep

	f1="$t"
	f2=`echo "$t" | sed "s;^${dir1}/;${dir2}/;g"`
	
	tnm=`echo "$t" | sed -e "s;^${dir1}/;;g" -e 's;/;_;g' -e 's;.png$;;g' -e 's; ;_;g'`
	echo "Test:  $tnm"
	echo "t:     $t"
	echo "File1: $f1"	
	echo "File2: $f2"	

	errdir=${ERRDIR}/${tnm}

	tot=`expr $tot + 1`


	######################################################################
	#
	# compare the png files
	#

	if test -f "${f2}" ; then
		same=`${COMPARE} -metric MAE "$f1" "$f2"  null: 2>&1 | \
				${AWK} '{if($1 == 0){print "yes"} else {print "no"}}'`
		if test "$same" = yes ; then
		echo "PASS"
		pass=`expr $pass + 1`
		else
		echo "FAILED:  See ${errdir}"
		mkdir -p ${errdir}
		${COMPARE} "${f1}" "${f2}" ${errdir}/compare.png
		${COMPOSITE} "${f1}" "${f2}" -compose difference ${errdir}/composite.png
		${CONVERT} "${f1}" "${f2}" -compose difference -composite  -colorspace gray	${errdir}/gray.png
cat > ${errdir}/animate.sh << EOF
#!/bin/sh
${CONVERT} -label "%f" "${f1}" "${f2}" miff:- | \
${MONTAGE} - -geometry +0+0 -tile 1x1 miff:- | \
${ANIMATE} -delay 0.5 -loop 0 -
EOF
		chmod a+x ${errdir}/animate.sh
		fail=`expr $fail + 1`
		fi
	else
		echo "Missing file ${f2}.  Skipping test"
		skip=`expr $skip + 1`
	fi

done

show_sep
echo "Passed $pass, failed $fail, skipped $skip out of $tot tests."

rc=0
if test $pass -ne $tot ; then
	rc=1
fi

exit $rc

