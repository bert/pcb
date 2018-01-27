#!/bin/sh
#
# Copyright (c) 2003, 2004, 2005, 2006, 2007, 2009, 2010, 2017 Dan McMahill

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
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA
# All rights reserved.
#
# This code was derived from code written by Dan McMahill as part of the
# latex-mk testsuite.  The original code was covered by a BSD license
# but the copyright holder (Dan McMahill) has re-released the code under
# the GPL.

magic_test_skip=${PCB_MAGIC_TEST_SKIP:-no}

if test "x${magic_test_skip}" = "xyes" ; then
	cat << EOF

The environment variable PCB_MAGIC_TEST_SKIP is set to yes.
This causes the testsuite to skip all tests and report no errors.
This is used for certain debugging *only*.  The primary use is to 
allow testing of the 'distcheck' target without including the effects
of the testsuite. The reason this is useful is that due to minor differences
in library versions and perhaps roundoff in different CPU's, the testsuite
may falsely report failures on some systems.  These reported failures
prevent using 'distcheck' for verifying the rest of the build system.
These comments only apply to the tests which try to compare image files
like PNG files.

EOF

	exit 0
fi

regen=no

usage() {
cat <<EOF

$0 -- Run pcb regression tests

$0 -h|--help
$0 [-d | --debug] [-g | --golden dir] [-r|--regen] [testname1 [testname2[ ...]]]

OVERVIEW

The $0 script is used both for running the pcb regression testsuite
as well as maintaining it.  The way the test suite works is a number
of different layouts are exported using the various export HID's.

The resulting output files are examined in various ways to make sure
they are correct.  The exact details of how they are compared varies.
For example, the PNG outputs are compared using tools from the ImageMagick
suite while the ASCII centroid and bill of materials files are normalized
with awk and then compared with the standard diff utility.  The normalization
removes things like a comment line which contains the creation date.

OPTIONS

-d | --debug              Enables extra debug output

-g | --golden <dir>    :  Specifies that <dir> should be used for the
                          reference files. 

-r | --regen           :  Specifies that the results should be taken as new
                          reference files instead of comparing them to the
                          existing ones.

LIMITATIONS

- The GUI interface is not checked via the regression testsuite.

EOF
}

show_sep() {
    echo "----------------------------------------------------------------------"
}

##########################################################################
#
# debug print out
#

do_debug=no
debug() {
    if test $do_debug = yes ; then
	cat <<EOF
$*
EOF
    fi
}

##########################################################################
#
# command line processing
#


all_tests=""
while test -n "$1"
  do
  case "$1"
      in
      
      -d|--debug)
	  do_debug=yes
	  shift
	  ;;

      -h|--help)
	  usage
	  exit 0
	  ;;
      
      -g|--golden)
	# set the 'golden' directory.
	  REFDIR="$2"
	  shift 2
	  ;;
      
      -r|--regen)
	# regenerate the 'golden' output files.  Use this with caution.
	# In particular, all differences should be noted and understood.
	  regen=yes
	  shift
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

if test "X$regen" = "Xyes" && test $# -ne 1; then
    echo "Please regenerate only one test at a time."
    echo "This limitation is a safety measure."
    exit 1
fi

##########################################################################
#
# set up various tools
#


all_tests="$*"

# Source directory
srcdir=${srcdir:-.}
top_srcdir=${top_srcdir:-.}

# The pcb wrapper script we want to test
#
# by default we will be running it from $(srcdir)/runs/testname
# so we need to look 3 levels up and then down to src
PCB=${PCB:-../../../src/pcbtest.sh}

# The gerbv executible 
GERBV=${GERBV:-gerbv}
GERBV_DEFAULT_FLAGS=${GERBV_DEFAULT_FLAGS:---export=png --window=640x480}

# various ImageMagick tools
IM_ANIMATE=${IM_ANIMATE:-animate}
IM_COMPARE=${IM_COMPARE:-compare}
IM_COMPOSITE=${IM_COMPOSITE:-composite}
IM_CONVERT=${IM_CONVERT:-convert}
IM_DISPLAY=${IM_DISPLAY:-display}
IM_MONTAGE=${IM_MONTAGE:-montage}

# xhost program to check for a working display
XHOST=${XHOST:-xhost}

# golden directories
INDIR=${INDIR:-${srcdir}/inputs}
OUTDIR=outputs
REFDIR=${REFDIR:-${srcdir}/golden}

# some system tools
AWK=${AWK:-awk}

# the list of tests to run
TESTLIST=${srcdir}/tests.list

if test "X$regen" = "Xyes" ; then
    OUTDIR="${REFDIR}"
fi

# create output directory
if test ! -d $OUTDIR ; then
    mkdir -p $OUTDIR
    if test $? -ne 0 ; then
	echo "Failed to create output directory ${OUTDIR}"
	exit 1
    fi
fi

if test -z "$all_tests" ; then
    if test ! -f ${TESTLIST} ; then
	echo "ERROR: ($0)  Test list $TESTLIST does not exist"
	exit 1
    fi
    all_tests=`${AWK} 'BEGIN{FS="|"} /^#/{next} {print $1}' ${TESTLIST} | sed 's; ;;g'`
fi

if test -z "${all_tests}" ; then
    echo "$0:  No tests specified"
    exit 0
fi


# check if we have a working display.  Without it we will
# fail the action script checks unless pcb was built as 
# batch HID.   I'm open to suggestions on improving
# the robustness of this check

echo "Checking for working X display"
${XHOST} >/dev/null 2>&1
rc=$?
if test $rc -eq 0 ; then
	have_display=yes
	echo "yes"
else
	have_display=no
	echo "no.  Certain tests will be skipped."
fi

# fail/pass/total counts
fail=0
pass=0
skip=0
tot=0

##########################################################################
#
# config summary
#


cat << EOF

srcdir                ${srcdir}
top_srcdir            ${top_srcdir}

AWK                   ${AWK}
PCB                   ${PCB}
INDIR                 ${INDIR}
OUTDIR                ${OUTDIR}
REFDIR                ${REFDIR}
TESTLIST              ${TESTLIST}

Gerbv:

GERBV                 ${GERBV}
GERBV_DEFAULT_FLAGS   ${GERBV_DEFAULT_FLAGS}

ImageMagick Tools:

IM_ANIMATE               ${IM_ANIMATE}
IM_COMPARE               ${IM_COMPARE}
IM_COMPOSITE             ${IM_COMPOSITE}
IM_CONVERT               ${IM_CONVERT}
IM_DISPLAY               ${IM_DISPLAY}
IM_MONTAGE               ${IM_MONTAGE}

Working Display (for Action tests):

have_display             ${have_display}

EOF

tmpd=/tmp/pcb_tests.$$
mkdir -p -m 0700 $tmpd
rc=$?
if test $rc -ne 0 ; then
    echo "$0:  ERROR:  could not create $tmpd"
    exit 1
fi

##########################################################################
#
# utility functions for comparison
#

# Usage:
#  compare_check "test_name" "file1" "file2"
#
# Makes sure that file1 and file2 both exist.  If not, mark the current
# test as skipped and give an error message
#
compare_check() {
    local fn="$1"
    local f1="$2"
    local f2="$3"

    if test ! -f "$f1" ; then 
	echo "$0:  ${fn}(): $f1 does not exist"
	test_skipped=yes
	return 1
    fi

    if test ! -f "$f2" ; then 
	echo "$0:  ${fn}(): $f2 does not exist"
	test_skipped=yes
	return 1
    fi
    return 0
}

##########################################################################
#
# ASCII file comparison routines
#

# Usage:
#   run_diff "file1" "file2"
#
run_diff() {
    local f1="$1"
    local f2="$2"
    diff -U 2 $f1 $f2
    if test $? -ne 0 ; then return 1 ; fi
    return 0
}

##########################################################################
#
# BOM comparison routines
#

# used to remove things like creation date from BOM files
normalize_bom() {
    local f1="$1"
    local f2="$2"
    $AWK '
	/^# Date:/ {print "# Date: today"; next}
	/^# Author:/ {print "# Author: PCB"; next}
	{print}' \
	$f1 > $f2
}

# top level function to compare BOM output
compare_bom() {
    local f1="$1"
    local f2="$2"
    compare_check "compare_bom" "$f1" "$f2" || return 1

    # an example BOM file is:

    #  # PcbBOM Version 1.0
    #  # Date: Wed Jun 17 14:41:43 2009 UTC
    #  # Author: Dan McMahill
    #  # Title: Basic BOM/XY Test - PCB BOM
    #  # Quantity, Description, Value, RefDes
    #  # --------------------------------------------
    #  8,"Standard SMT resistor, capacitor etc","RESC3216N",R90_TOP R180_TOP R270_TOP R0_TOP R270_BOT R180_BOT R90_BOT R0_BOT 
    #  8,"Dual in-line package, narrow (300 mil)","DIP8",UDIP90_TOP UDIP180_TOP UDIP270_TOP UDIP0_TOP UDIP270_BOT UDIP180_BOT UDIP90_BOT UDIP0_BOT 
    #  8,"Small outline package, narrow (150mil)","SO8",USO90_TOP USO180_TOP USO270_TOP USO0_TOP USO270_BOT USO180_BOT USO90_BOT USO0_BOT 

    #  For comparison, we need to ignore changes in the Date and Author lines.
    local cf1=${tmpd}/`basename $f1`-ref
    local cf2=${tmpd}/`basename $f2`-out

    normalize_bom $f1 $cf1
    normalize_bom $f2 $cf2
    run_diff $cf1 $cf2 || test_failed=yes
}

##########################################################################
#
# X-Y (centroid) comparison routines
#

# used to remove things like creation date from BOM files
normalize_xy() {
    local f1="$1"
    local f2="$2"
    $AWK '
	/^# Date:/ {print "# Date: today"; next}
	/^# Author:/ {print "# Author: PCB"; next}
	{print}' \
	$f1 > $f2
}

compare_xy() {
    local f1="$1"
    local f2="$2"
    compare_check "compare_xy" "$f1" "$f2" || return 1

    local cf1=${tmpd}/`basename $f1`-ref
    local cf2=${tmpd}/`basename $f2`-out
    normalize_xy "$f1" "$cf1"
    normalize_xy "$f2" "$cf2"
    run_diff "$cf1" "$cf2" || test_failed=yes
}

##########################################################################
#
# GCODE comparison routines
#

# used to remove things like creation date from gcode files
normalize_gcode() {
    local f1="$1"
    local f2="$2"
    # matches string such as '( Tue Mar  9 17:45:43 2010 )'
    $AWK  '/^\( *[A-Z][a-z][a-z] [A-Z][a-z][a-z] [0123 ][0-9] [0-9][0-9]:[0-9][0-9]:[0-9][0-9] [0-9][0-9][0-9][0-9] *\)$/ {print "( Date String )"; next}
	{print}' \
	$f1 > $f2
}

compare_gcode() {
    local f1="$1"
    local f2="$2"
    compare_check "compare_gcode" "$f1" "$f2" || return 1

    #  For comparison, we need to ignore changes in the Date and Author lines.
    local cf1=${tmpd}/`basename $f1`-ref
    local cf2=${tmpd}/`basename $f2`-out

    normalize_gcode $f1 $cf1
    normalize_gcode $f2 $cf2

    run_diff "$cf1" "$cf2" || test_failed=yes
}

##########################################################################
#
# RS274-X and Excellon comparison
#

compare_rs274x() {
    local f1="$1"
    local f2="$2"
    compare_check "compare_rs274x" "$f1" "$f2" || return 1

    # use gerbv to export our reference RS274-X file and our generated
    # RS274-X file to png.  Then we'll use ImageMagick to see
    # if there are any differences in the resulting files
    pngdir=${rundir}/png
    mkdir -p ${pngdir}
    nb=`basename $f1`
    png1=${pngdir}/${nb}-ref.png
    png2=${pngdir}/${nb}-out.png

    debug "${GERBV} ${GERBV_DEFAULT_FLAGS} --output=${png1} ${f1}"
    ${GERBV} ${GERBV_DEFAULT_FLAGS} --output=${png1} ${f1}

    debug "${GERBV} ${GERBV_DEFAULT_FLAGS} --output=${png2} ${f2}"
    ${GERBV} ${GERBV_DEFAULT_FLAGS} --output=${png2} ${f2}

    compare_image ${png1} ${png2}

}

compare_cnc() {
    compare_rs274x $*
}

##########################################################################
#
# gsvit comparison
#

# used to remove things like job name and creation date from gsvit files
normalize_xem() {
    local f1="$1"
    local f2="$2"
    $AWK '
	/<genTime>Thu Jan 11 23:25:46 2018</genTime>/ {print} "<genTime>today</genTime>"; next}
	{print}' \
	$f1 > $f2
}

# top level function to compare gsvit output (*.xem)
compare_gsvit() {
    local f1="$1"
    local f2="$2"
    compare_check "compare_gsvit" "$f1" "$f2" || return 1

    #  For comparison, we need to ignore changes in the Date line.
    local cf1=${tmpd}/`basename $f1`-ref
    local cf2=${tmpd}/`basename $f2`-out

    normalize_xem $f1 $cf1
    normalize_xem $f2 $cf2
    run_diff $cf1 $cf2 || test_failed=yes
}

##########################################################################
#
# IPC-D-356 netlist comparison
#

# used to remove things like job name and creation date from IPC-D-356
# netlist files
normalize_ipcd356() {
    local f1="$1"
    local f2="$2"
    $AWK '
	/^C  IPC-D-356 Netlist generated by gEDA/ {print "C  IPC-D-356 Netlist generated by gEDA PCB"; next}
	/^C  File created on/ {print "C  File created on today"; next}
	/^P  JOB/ {print "P  JOB PCB"; next}
	{print}' \
	$f1 > $f2
}

# top level function to compare IPC-D-356 netlist output
compare_ipcd356() {
    local f1="$1"
    local f2="$2"
    compare_check "compare_ipcd356" "$f1" "$f2" || return 1

#	An example IPC-D-356 netlist file header is:

#	C  IPC-D-356 Netlist generated by gEDA PCB 1.99z
#	C  
#	C  File created on Thu 01 Sep 2016 05:53:02 PM GMT UTC
#	C  
#	P  JOB   /home/bert/workspace/git/pcb/tests/inputs/ipcd356_board.pcb
#	P  CODE  00
#	P  UNITS CUST 0
#	P  DIM   N
#	P  VER   IPC-D-356
#	P  IMAGE PRIMARY
#	C
  
    #  For comparison, we need to ignore changes in the Date and Job lines.
    local cf1=${tmpd}/`basename $f1`-ref
    local cf2=${tmpd}/`basename $f2`-out

    normalize_ipcd356 $f1 $cf1
    normalize_ipcd356 $f2 $cf2
    run_diff $cf1 $cf2 || test_failed=yes
}

##########################################################################
#
# nelma comparison
#

# used to remove things like job name and creation date from nelma files
normalize_em() {
    local f1="$1"
    local f2="$2"
    $AWK '
	/Fri Jan 26 23:44:30 2018/ {print} "today"; next}
	{print}' \
	$f1 > $f2
}

# top level function to compare gsvit output (*.xem)
compare_nelma() {
    local f1="$1"
    local f2="$2"
    compare_check "compare_gsvit" "$f1" "$f2" || return 1

    #  For comparison, we need to ignore changes in the Date line.
    local cf1=${tmpd}/`basename $f1`-ref
    local cf2=${tmpd}/`basename $f2`-out

    normalize_em $f1 $cf1
    normalize_em $f2 $cf2
    run_diff $cf1 $cf2 || test_failed=yes
}

##########################################################################
#
# PostScript comparison
#

compare_ps() {
    local f1="$1"
    local f2="$2"
    compare_check "compare_ps" "$f1" "$f2" || return 1

    # PostScript output is difficult to compare, because the last page
    # ( = fab page) contains a date stamp written not as text, but drawn
    # with lines. For now we only check whether the file contains valid
    # PostScript and whether the page count matches.
    TEMP_FILE=`mktemp`
    echo "%!"                                      > ${TEMP_FILE}
    echo "currentpagedevice /PageCount get"       >> ${TEMP_FILE}
    echo " == flush"                              >> ${TEMP_FILE}

    ORG_COUNT=`gs -q -dNODISPLAY -dBATCH -dNOPAUSE "$f1" ${TEMP_FILE}`
    NEW_COUNT=`gs -q -dNODISPLAY -dBATCH -dNOPAUSE "$f2" ${TEMP_FILE}`
    RESULT=$?

    rm -f ${TEMP_FILE}

    if test ${RESULT} -ne 0; then
        echo "Invalid PostScript generated."
        test_failed=yes
    else if test "${ORG_COUNT}" != "${NEW_COUNT}"; then
        echo -n "Page count of generated PostScript is ${NEW_COUNT} "
        echo    "instead of expected ${OLD_COUNT}."
        test_failed=yes
    fi; fi

    unset ORG_COUNT NEW_COUNT RESULT
}

##########################################################################
#
# GIF/JPEG/PNG comparison routines
#

compare_image() {
    local f1="$1"
    local f2="$2"
    compare_check "compare_image" "$f1" "$f2" || return 1

    # now see if the image files are the same
    debug "${IM_COMPARE} -metric MAE ${f1} ${f2}  null:"
    same=`${IM_COMPARE} -metric MAE ${f1} ${f2}  null: 2>&1 | \
          ${AWK} '{if($1 == 0){print "yes"} else {print "no"}}'`
    debug "compare_image():  same = $same"

    if test "$same" != yes ; then
	test_failed=yes
	echo "FAILED:  See ${errdir}"
	mkdir -p ${errdir}
	${IM_COMPARE} ${f1} ${f2} ${errdir}/compare.png
	${IM_COMPOSITE} ${f1} ${f2} -compose difference ${errdir}/composite.png
	${IM_CONVERT} ${f1} ${f2} -compose difference -composite  -colorspace gray   ${errdir}/gray.png
	cat > ${errdir}/animate.sh << EOF
#!/bin/sh
${IM_CONVERT} -label "%f" ${f1} ${f2} miff:- | \
${IM_MONTAGE} - -geometry +0+0 -tile 1x1 miff:- | \
${IM_ANIMATE} -delay 0.5 -loop 0 -
EOF
	chmod a+x ${errdir}/animate.sh
    fi
}

##########################################################################
#
# PCB (layout) comparison routines
#
# used to remove things like creation date from BOM files
normalize_pcb() {
    local f1="$1"
    local f2="$2"
    sed -e 's/#.*$//' -e '/^$/d' $f1 > $f2
}


compare_pcb() {
    local f1="$1"
    local f2="$2"

    compare_check "compare_pcb" "$f1" "$f2" || return 1

    #  For comparison, we need to ignore changes in the Date and Author lines.
    local cf1=${tmpd}/`basename $f1`-ref
    local cf2=${tmpd}/`basename $f2`-out

    normalize_pcb $f1 $cf1
    normalize_pcb $f2 $cf2


    run_diff "$cf1" "$cf2" || test_failed=yes
}

##########################################################################
#
# The main program loop
#

for t in $all_tests ; do
    show_sep
    echo "Test:  $t"

    tot=`expr $tot + 1`

    ######################################################################
    #
    # extract the details for the test
    #

    pcb_flags="${PCB_DEFAULT_FLAGS}"

    rundir="${OUTDIR}/${t}"
    refdir="${REFDIR}/${t}"
    errdir=${rundir}/mismatch

    # test_name | layout file(s) | [export hid name] | [optional arguments to pcb] |  [mismatch] 
    # | output files
    name=`grep "^[ \t]*${t}[ \t]*|" $TESTLIST | $AWK 'BEGIN{FS="|"} {print $1}'`
    files=`grep "^[ \t]*${t}[ \t]*|" $TESTLIST | $AWK 'BEGIN{FS="|"} {print $2}'`
    hid=`grep "^[ \t]*${t}[ \t]*|" $TESTLIST | $AWK 'BEGIN{FS="|"} {gsub(/[ \t]*/, ""); print $3}'`
    args=`grep "^[ \t]*${t}[ \t]*|" $TESTLIST | $AWK 'BEGIN{FS="|"} {print $4}'`
    mismatch=`grep "^[ \t]*${t}[ \t]*|" $TESTLIST | $AWK 'BEGIN{FS="|"} {if($5 == "mismatch"){print "yes"}else{print "no"}}'`
    out_files=`grep "^[ \t]*${t}[ \t]*|" $TESTLIST | $AWK 'BEGIN{FS="|"} {print $6}'`
   
    # strip whitespace from single file names
    while test "X${files# }" != "X${files#}" ; do
	files="${files# }"
    done
    while test "X${files% }" != "X${files%}" ; do
	files="${files% }"
    done

    if test "X${name}" = "X" ; then
	echo "ERROR:  Specified test ${t} does not appear to exist"
	skip=`expr $skip + 1`
	continue
    fi
    
    if test "X${args}" != "X" ; then
	pcb_flags="${args}"
    fi

    if test "X${hid}" = "Xgerber" ; then
	pcb_flags="--fab-author Fab_Author ${pcb_flags}"
    fi

    if test "X${hid}" = "Xaction" ; then
	script_file=${files%.pcb}.script
	pcb_flags="--action-script ../../${INDIR}/${script_file}"
	if test "X${have_display}" = "Xno" ; then
		echo "Skipping action test because there is currently no X display available"
		skip=`expr $skip + 1`
		continue
	fi
    else
	# hidden here, still in effect for all HID tests
	pcb_flags="-x ${hid} ${pcb_flags}"
    fi

    ######################################################################
    #
    # Set up the run directory
    #

    test -d ${rundir} && rm -fr ${rundir}
    mkdir -p ${rundir}
    if test $? -ne 0 ; then
	echo "$0:  Could not create run directory ${rundir}"
	skip=`expr $skip + 1`
	continue
    fi


    ######################################################################
    #
    # check to see if the files we need exist and copy them to the run
    # directory
    #

    missing_files=no
    path_files=""
    for f in $files ; do
	if test ! -f ${INDIR}/${f} ; then
	    echo "ERROR:  File $f specified as part of the $t test does not exist"
	    missing_files=yes
	else
	    path_files="${path_files} ${INDIR}/${f}"
	    cp "${INDIR}/${f}" "${rundir}"
	    chmod u+w "${rundir}/${f}"
	fi
    done

    if test "$missing_files" = "yes" ; then
	echo "${t} had missing input files.  Skipping test"
	skip=`expr $skip + 1`
	continue
    fi
    
    ######################################################################
    #
    # run PCB
    #

    echo "(cd ${rundir} && ${PCB} ${pcb_flags} ${files})"
    (cd ${rundir} && ${PCB} ${pcb_flags} ${files})
    pcb_rc=$?

    if test $pcb_rc -ne 0 ; then
	echo "${PCB} returned ${pcb_rc}.  This is a failure."
	fail=`expr $fail + 1`
	continue
    fi

    ######################################################################
    #
    # check the result
    #

    if test "X$regen" = "Xyes" ; then
	echo "Regenerated ${t}"
    else
	# compare the result to our reference file
	test_failed=no
	test_skipped=no
	for f in $out_files ; do
	    debug "processing $f"
	    # break apart type:fn into the type and file name
	    type=`echo $f | sed 's;:.*;;g'`
	    fn=`echo $f | sed 's;.*:;;g'`

	    case $type in
		# BOM HID
		bom)
		    compare_bom ${refdir}/${fn} ${rundir}/${fn}
		    ;;

	        xy)
		    compare_xy ${refdir}/${fn} ${rundir}/${fn}
		    ;;

		# GCODE HID
		gcode)
		    compare_gcode ${refdir}/${fn} ${rundir}/${fn}
		    ;;

		# GERBER HID
		cnc)
		    compare_cnc ${refdir}/${fn} ${rundir}/${fn}
		    ;;

		gbx)
		    compare_rs274x ${refdir}/${fn} ${rundir}/${fn}
		    ;;

		# NELMA HID
		em)
		    compare_nelma ${refdir}/${fn} ${rundir}/${fn}
		    ;;

		# GSVIT HID
		xem)
		    compare_gsvit ${refdir}/${fn} ${rundir}/${fn}
		    ;;

		# IPC-D-356 HID
		net)
		    compare_ipcd356 ${refdir}/${fn} ${rundir}/${fn}
		    ;;

		# PS HID
		ps)
		    compare_ps ${refdir}/${fn} ${rundir}/${fn}
		    ;;

		# PNG HID
	        gif)
		    compare_image ${refdir}/${fn} ${rundir}/${fn}
		    ;;

		jpg)
		    compare_image ${refdir}/${fn} ${rundir}/${fn}
		    ;;

		png)
		    compare_image ${refdir}/${fn} ${rundir}/${fn}
		    ;;

		# actions
		pcb)
		    compare_pcb ${refdir}/${fn} ${rundir}/${fn}
		    ;;

		# unknown
		*)
		    echo "internal error:  $type is not a known file type"
		    exit 1
		    ;;
	    esac

	done

	if test "X${hid}" != "Xaction" ; then
	    # clean up the input files we'd copied over
	    for f in $files ; do
		rm -f ${rundir}/${f}
	    done
	fi

	if test $test_failed = yes ; then
	    echo "FAIL"
	    fail=`expr $fail + 1`
	elif test $test_skipped = yes ; then
	    echo "SKIPPED"
	    skip=`expr $skip + 1`
	else
	    echo "PASSED"
	    pass=`expr $pass + 1`
	fi

    fi

done

show_sep
echo "Passed $pass, failed $fail, skipped $skip out of $tot tests."

rc=0
if test $fail -gt 0 ; then
    rc=1
fi

exit $rc

