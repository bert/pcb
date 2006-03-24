dnl $Id$
dnl Process this file with autoconf to produce a configure script.

AC_INIT(src/draw.c)
AM_INIT_AUTOMAKE(pcb, 1.99q)
AM_CONFIG_HEADER(config.h)

AM_MAINTAINER_MODE

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CC_STDC
AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_AWK
AM_PROG_LEX
AC_PROG_YACC
AC_PROG_INSTALL
AC_PROG_RANLIB

docs_yesno=yes

AC_MSG_CHECKING([if the documentation should be built])
AC_ARG_ENABLE([doc],
[  --enable-doc            Build and install the documentation [[default=yes]]],
[
if test "X$enable_doc" = "Xno" ; then
        DOC=""
        AC_MSG_RESULT([no])
        docs_yesno=no
else
        DOC=doc
        AC_MSG_RESULT([yes])
        docs_yesno=yes
fi
],
[
DOC=doc
AC_MSG_RESULT([yes])
docs_yesno=yes
])
AC_SUBST(DOC)

if test "X$docs_yesno" = "Xyes" -a "X$USE_MAINTAINER_MODE" = "Xyes" ; then
	AC_CHECK_PROGS(MKINFO, makeinfo, notfound)
	if test "X$MKINFO" = "Xnotfound"; then
			AC_MSG_ERROR([You have requested a maintainer-mode build and a build
of the documentation.  For this to work, you must have version 4.6 or newer of
the GNU texinfo package.
])
	fi
   AC_MSG_CHECKING([for makeinfo version 4.6 or newer])
   v=`$MKINFO --version | grep "GNU texinfo"`
   if test $? -ne 0 ; then
		v=notfound
   fi

	case " $v " in
		*\ 4.[[6-9]]\ *)
			AC_MSG_RESULT([yes])
			;;

		*)
			AC_MSG_RESULT([no])
			AC_MSG_ERROR([You have requested a maintainer-mode build and a build
of the documentation.  For this to work, you must have version 4.6 or newer of
the GNU texinfo package.  You seem to have

$v

])
			;;
	esac
fi

# ------------- HID config -------------------

hid_guis=""
hid_printers=""
hid_exporters=""
hid_always=""

for hid in `cd $srcdir/src/hid; echo *`; do
    F=$srcdir/src/hid/$hid/hid.conf
    if test -f $F 
    then
    echo checking $F
        . $F
	case $type in
	  gui ) hid_guis="$hid_guis $hid" ;;
	  printer ) hid_printers="$hid_printers $hid" ;;
	  export ) hid_exporters="$hid_exporters $hid" ;;
	  always ) hid_always="$hid_always $hid" ;;
	esac
    fi
done

AC_MSG_CHECKING([for which gui to use])
AC_ARG_WITH([gui],
[  --with-gui=	          Specify the GUI to use: $hid_guis],
[],
[with_gui=gtk]
)
AC_MSG_RESULT([$with_gui])
case " $hid_guis no none " in
     *\ $with_gui\ * ) HIDLIST="$with_gui" ;;
     * ) AC_MSG_ERROR([$with_gui is not a valid gui]) ;;
esac

if test x"$with_gui" = x"none" -o x"$with_gui" = x"no"
then
    HIDLIST=
fi

AC_MSG_CHECKING([for which printer to use])
AC_ARG_WITH([printer],
[  --with-printer= 	  Specify the printer: $hid_printers],
[],[with_printer=lpr])
AC_MSG_RESULT([$with_printer])
case " $hid_printers " in
     *\ $with_printer\ * )
         HIDLIST="$HIDLIST $with_printer"
	 ;;
     * ) AC_MSG_ERROR([$with_printer is not a valid printer]) ;;
esac

AC_MSG_CHECKING([for which exporters to use])
AC_ARG_WITH([exporters],
[  --with-exporters=       Enable export devices:] $hid_exporters,
[],[with_exporters=$hid_exporters])
AC_MSG_RESULT([$with_exporters])
for e in `echo $with_exporters | sed 's/,/ /g'`; do
    case " $hid_exporters " in
      *\ $e\ * )
         HIDLIST="$HIDLIST $e"
	 ;;
      * ) AC_MSG_ERROR([$e is not a valid exporter]) ;;
    esac
done

for e in $HIDLIST; do
    HIDLIBS="$HIDLIBS lib$e.a"
done

AC_SUBST(HIDLIST)
AC_SUBST(HIDLIBS)

# ------------- end HID config -------------------

AC_PATH_PROGS(M4, gm4 m4, [none])
if test "X$M4" = "Xnone" ; then
	AC_MSG_ERROR([Did not find a m4 executible.  You need to make sure
	that m4 is installed on your system and that m4 is in your path])
fi

AC_PATH_PROGS(WISH, wish wish83 wish8.3 wish80 wish8.0 cygwish83 cygwish80,[none])
if test "X$WISH" = "Xnone" ; then
	AC_MSG_ERROR([Did not find the wish executible.  You need to make sure
	that tcl is installed on your system and that wish is in your path])
fi

AC_DEFINE_UNQUOTED(M4,$M4,[m4 executible])
GNUM4=$M4
AC_SUBST(GNUM4)
AC_DEFINE_UNQUOTED(GNUM4,"$M4",[m4 program used by pcb])

AC_CHECK_PROGS(LATEX, latex, notfound)
AM_CONDITIONAL(MISSING_LATEX, test x$LATEX = xnotfound)

AC_CHECK_PROGS(PDFLATEX, pdflatex, notfound)
AM_CONDITIONAL(MISSING_PDFLATEX, test x$PDFLATEX = xnotfound)

AC_CHECK_PROGS(DVIPS, dvips, notfound)
AM_CONDITIONAL(MISSING_DVIPS, test x$DVIPS = xnotfound)

AC_CHECK_PROGS(PS2PDF, ps2pdf, notfound)
AM_CONDITIONAL(MISSING_PS2PDF, test x$PS2PDF = xnotfound)

dnl Checks for libraries.
AC_CHECK_LIB(m, sqrt)
AC_CHECK_LIB(xnet, gethostbyname)
AC_CHECK_LIB(fl, yywrap)
AC_CHECK_FUNCS(strerror)
AC_CHECK_FUNCS(regcomp re_comp)
AC_CHECK_FUNCS(logf expf rint)
AC_CHECK_FUNCS(vsnprintf)
AC_HEADER_STDC
AC_CHECK_HEADERS(string.h sys/types.h regex.h)

AC_PATH_XTRA

for e in $HIDLIST; do
    case $e in
      lesstif )
	AC_CHECK_LIB(X11, XOpenDisplay, , , $X_LIBS)
	AC_CHECK_LIB(ICE, main, , , $X_LIBS)
	AC_CHECK_LIB(SM, main, , , $X_LIBS)
	AC_CHECK_LIB(Xext, main, , , $X_LIBS)
	AC_CHECK_LIB(Xt, XtOpenDisplay, , , $X_LIBS)
	AC_CHECK_LIB(Xmu, main, , , $X_LIBS)
	AC_CHECK_LIB(Xpm, main, , , $X_LIBS)
        AC_CHECK_LIB(Xm, XmCreateMainWindow, , , $X_LIBS)
	case $ac_cv_lib_Xm_XmCreateMainWindow in
	  no )
	    AC_MSG_ERROR([You don't seem to have the Lesstif development environment installed.])
	    ;;
	  * ) ;;
	esac
	AC_CHECK_HEADERS(Xm/Xm.h)
	case $ac_cv_header_Xm_Xm_h in
	  no )
	    AC_MSG_ERROR([You don't seem to have the Lesstif development environment installed.])
	    ;;
	  * ) ;;
	esac
        ;;

      gtk )
	# Check for pkg-config
	AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
	if test "$PKG_CONFIG" = "no"; then
		AC_MSG_ERROR([Cannot find pkg-config, make sure it is installed and in your PATH])
	fi

	PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.4.0, , 
		[AC_MSG_ERROR([Cannot find gtk+ >= 2.4.0, install it and rerun ./configure])]
	)
	GTK_VERSION=`$PKG_CONFIG gtk+-2.0 --modversion`

	# Search for glib
	PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.4.0, , 
		[AC_MSG_ERROR([Cannot find glib >= 2.4.0, install it and rerun ./configure.])]
	)
	GLIB_VERSION=`$PKG_CONFIG glib-2.0 --modversion`
	;;

      png )
	# Check for gdlib-config for gd (www.boutell.com/gd)
	AC_PATH_PROG(GDLIB_CONFIG, gdlib-config, no)
	if test "$GDLIB_CONFIG" = "no"; then
		AC_MSG_ERROR([Cannot find gdlib-config.
Make sure it is installed and in your PATH.
gdlib-config is part of the GD library available from www.boutell.com/gd.
This is needed for the png HID.
])
	fi
	AC_MSG_CHECKING([for libgd cflags])
	GD_CFLAGS="`$GDLIB_CONFIG --cflags`"
	AC_MSG_RESULT([$GD_CFLAGS])
	AC_MSG_CHECKING([for libgd libs])
	GD_LIBS="`$GDLIB_CONFIG --ldflags` `$GDLIB_CONFIG --libs` -lgd"
	AC_MSG_RESULT([$GD_LIBS])
   
	# since some linux systems evidently install gdlib-config but fail to
	# install the headers (nice), check for the header too and fail if it
	# is not there.
	AC_CHECK_HEADERS(gd.h)
	case $ac_cv_header_gd_h in
	     no )
		AC_MSG_ERROR([
You evidentally do not have a complete installation of the GD library available from www.boutell.com/gd.
This is needed for the png HID.
])
		;;
	     * ) ;;
	esac

    esac
done



# font filename
FONTFILENAME=${FONTFILENAME:-"default_font"}
AC_SUBST(FONTFILENAME)
AC_DEFINE_UNQUOTED(FONTFILENAME,"$FONTFILENAME",[File for default font])

# directory for old-style library and for fonts
PCBLIBDIR=${datadir}/pcb
AC_SUBST(PCBLIBDIR)
#AC_DEFINE_UNQUOTED(PCBLIBDIR,"$PCBLIBDIR",[Library directory])
CPPFLAGS="$CPPFLAGS -DPCBLIBDIR=\\\"$PCBLIBDIR\\\""

# name for old-style library
LIBRARYFILENAME=pcblib
AC_SUBST(LIBRARYFILENAME)
AC_DEFINE_UNQUOTED(LIBRARYFILENAME,"$LIBRARYFILENAME",[library file name])


# directory for new library
PCBTREEDIR=${datadir}/pcb/newlib
PCBTREEDIR=${PCBTREEDIR:-"$PCBTREEDIR"}
AC_SUBST(PCBTREEDIR)
#AC_DEFINE_UNQUOTED(PCBTREEDIR,"$PCBLIB",[top directory for new style pcb library])
CPPFLAGS="$CPPFLAGS -DPCBTREEDIR=\\\"$PCBTREEDIR\\\""

# if we have gcc then add -Wall
if test "x$GCC" = "xyes"; then
	if test -z "`echo "$CFLAGS" | grep "\-Wall" 2> /dev/null`" ; then
		CFLAGS="$CFLAGS -Wall"
	fi
fi

CFLAGS="$CFLAGS $X_CFLAGS $GTK_CFLAGS $GD_CFLAGS"
LIBS="$LIBS $XM_LIBS $X_LIBS $GTK_LIBS $DMALLOC_LIBS $GD_LIBS"

BTNMOD=${BTNMOD:-"Mod1"}
AC_SUBST(BTNMOD)

TOPDIRS=
for dir in src lib newlib doc example tools tutorial README_FILES
do
   test -d $dir/. && TOPDIRS="$TOPDIRS $dir"
done
AC_SUBST(TOPDIRS)

AC_CONFIG_FILES(Makefile)
if test -d $srcdir/README_FILES; then
   AC_CONFIG_FILES(README_FILES/Makefile)
fi
if test -d $srcdir/doc; then
   AC_CONFIG_FILES(doc/Makefile)
fi
if test -d $srcdir/example; then
   AC_CONFIG_FILES(example/Makefile)
   AC_CONFIG_FILES(example/libraries/Makefile)
fi
if test -d $srcdir/lib; then
   AC_CONFIG_FILES(lib/CreateLibraryContents.sh)
   AC_CONFIG_FILES(lib/CreateLibrary.sh)
   AC_CONFIG_FILES(lib/ListLibraryContents.sh)
   AC_CONFIG_FILES(lib/Makefile)
   AC_CONFIG_FILES(lib/QueryLibrary.sh)
   AC_CONFIG_FILES(lib/qfp-ui)
fi
if test -d $srcdir/newlib; then
   AC_CONFIG_FILES(newlib/2_pin_thru-hole_packages/Makefile)
   AC_CONFIG_FILES(newlib/Makefile)
   AC_CONFIG_FILES(newlib/analog-devices/Makefile)
   AC_CONFIG_FILES(newlib/burr-brown/Makefile)
   AC_CONFIG_FILES(newlib/connectors/Makefile)
   AC_CONFIG_FILES(newlib/crystal/Makefile)
   AC_CONFIG_FILES(newlib/cypress/Makefile)
   AC_CONFIG_FILES(newlib/electro-optics/Makefile)
   AC_CONFIG_FILES(newlib/generic_SMD_packages/Makefile)
   AC_CONFIG_FILES(newlib/headers/Makefile)
   AC_CONFIG_FILES(newlib/msp430/Makefile)
   AC_CONFIG_FILES(newlib/not_vetted_ingo/Makefile)
   AC_CONFIG_FILES(newlib/sockets/Makefile)
   AC_CONFIG_FILES(newlib/tests/Makefile)
   AC_CONFIG_FILES(newlib/toko/Makefile)
fi
AC_CONFIG_FILES(src/Makefile)
AC_CONFIG_FILES(src/script/Makefile)
AC_CONFIG_FILES(src/icons/Makefile)
if test -d $srcdir/tools; then
   AC_CONFIG_FILES(tools/Makefile)
fi
if test -d $srcdir/tutorial; then
   AC_CONFIG_FILES(tutorial/Makefile)
fi

AC_OUTPUT

with_gui=`echo $with_gui`
with_printer=`echo $with_printer`
with_exporters=`echo $with_exporters | sed 's/,/ /g'`

AC_MSG_RESULT([
** Configuration summary for $PACKAGE $VERSION:

   GUI:                      $with_gui
   Printer:                  $with_printer
   Exporters:                $with_exporters
   Build documentation:      $docs_yesno
   CPPFLAGS:                 $CPPFLAGS
   CFLAGS:                   $CFLAGS
   LIBS:                     $LIBS

])
