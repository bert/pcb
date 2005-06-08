dnl $Id$
dnl Process this file with autoconf to produce a configure script.

AC_INIT(src/draw.c)
AM_INIT_AUTOMAKE(pcb, 1.99q)
AM_CONFIG_HEADER(config.h)

AM_MAINTAINER_MODE

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_CPP
AM_PROG_LEX
AC_PROG_YACC
AC_PROG_INSTALL
AC_CHECK_PROGS(M4, gm4 m4)
if test "X$M4" = "Xnone" ; then
	AC_ERROR([Did not find a m4 executible.  You need to make sure
	that m4 is installed on your system and that m4 is in your path])
fi

AC_PATH_PROGS(WISH, wish wish83 wish8.3 wish80 wish8.0 cygwish83 cygwish80,[none])
if test "X$WISH" = "Xnone" ; then
	AC_ERROR([Did not find the wish executible.  You need to make sure
	that tcl is installed on your system and that wish is in your path])
fi

AC_DEFINE_UNQUOTED(M4,$M4,[m4 executible])
GNUM4=$M4
AC_SUBST(GNUM4)
AC_DEFINE_UNQUOTED(GNUM4,"$M4",[m4 program used by pcb])
AC_CHECK_PROGS(LATEX, latex)
AC_CHECK_PROGS(DVIPS, dvips)
AC_CHECK_PROGS(TGIF, tgif, true)
AC_CHECK_PROGS(PS2PDF, ps2pdf)

dnl Checks for libraries.
AC_CHECK_LIB(m, sqrt)
AC_CHECK_LIB(xnet, gethostbyname)
AC_CHECK_LIB(fl, yywrap)
AC_CHECK_FUNCS(strerror)
AC_CHECK_FUNCS(regcomp re_comp)
AC_CHECK_FUNCS(logf expf rint)

AC_PATH_XTRA

AC_MSG_CHECKING([if X/Xaw should be configured])
gui_xaw=no
AC_ARG_ENABLE([xaw],
[  --enable-xaw            Enable configure detection of Xaw, not complete yet. [[default: disabled]]],
[
if test "X$enable_xaw" = "Xno" ; then
	AC_MSG_RESULT([yes])
	gui_xaw="yes *** THIS IS NOT YET SUPPORTED AND PROBABLY WILL NOT COMPILE ***"
else
	AC_MSG_RESULT([no])
fi
],
[
	AC_MSG_RESULT([no])
])

if test "X$gui_xaw" != "Xno" ; then

AC_ARG_WITH([libstroke],
[  --without-libstroke     disable libstroke support (default: enabled)],
AC_CHECK_LIB(stroke, main, , AC_ERROR([libstroke not found.  Please specify its location or configure --without-libstroke]))
)
AC_CHECK_LIB(X11, XOpenDisplay, , , $X_LIBS)
AC_CHECK_LIB(ICE, main, , , $X_LIBS)
AC_CHECK_LIB(SM, main, , , $X_LIBS)
AC_CHECK_LIB(Xext, main, , , $X_LIBS)
AC_CHECK_LIB(Xt, XtOpenDisplay, , , $X_LIBS)
AC_CHECK_LIB(Xmu, main, , , $X_LIBS)
AC_CHECK_LIB(Xpm, main, , , $X_LIBS)

AC_ARG_WITH([xaw],
[  --with-xaw=Xaw         Use the specified Athena Widgets type.
                          For example, Xaw, or Xaw3d.
                          [default = Xaw]],
[
if test "X$with_xaw" = "Xno" ; then
	AC_ERROR([--without-xaw is not allowed])
fi
],
[
with_xaw=Xaw
])

AC_CHECK_LIB($with_xaw, XawInitializeWidgetSet, , 
	AC_ERROR([You must have some version of Xaw installed on your system]), $X_LIBS)
LIBXAW=$with_xaw
XAWINC=$with_xaw

dnl Determine the Xfuncproto control definitions:
FC_CHECK_X_PROTO_DEFINE(FUNCPROTO)
if test -n "$fc_x_proto_value"; then
	AC_DEFINE_UNQUOTED(FUNCPROTO, $fc_x_proto_value, [FUNCPROTO used by Xaw])
fi
FC_CHECK_X_PROTO_DEFINE(NARROWPROTO)
if test -n "$fc_x_proto_value"; then
	AC_DEFINE_UNQUOTED(NARROWPROTO, $fc_x_proto_value, [NARROWPROTO used by Xaw])
fi

fi 

_use_rpath=yes
AC_MSG_CHECKING([if hardcoding of the X11 runtime library path is desired])
AC_ARG_ENABLE([rpath],
[  --disable-rpath         Disable hardcoding the X11 runtime library path [default=enabled]],
[
if test "X$enable_rpath" = "Xno" ; then
	AC_MSG_RESULT([no])
	_use_rpath=no
else
	AC_MSG_RESULT([yes])
	_use_rpath=yes
fi
],
[
	AC_MSG_RESULT([yes])
	_use_rpath=yes
])

if test "X$_use_rpath" = "Xyes" ; then
	dnl Try to figure out if we need -Rpath for finding X11 libs
	dnl at runtime.  Why autoconf doesn't already do this, I don't
	dnl know...
	xlib_path=""
	for p in $X_LIBS ; do
		case $p in
			-L*)
			xlib_path="$xlib_path $p"
			;;

		esac
	done
	xlib_rpath=`echo $xlib_path | sed 's/-L/-R/g'`

	pcb_save_LIBS=$LIBS
	LIBS="$LIBS $X_LIBS"
	rpath=""
	for fl in "-Wl,-rpath " "-Wl,--rpath " "-rpath " "--rpath " "-Wl,-R" "-R" "-R " ; do
		xlib_rpath=`echo $xlib_path | sed "s/-L/$fl/g"`
		LIBS="$pcb_save_LIBS $X_LIBS $xlib_rpath"
		AC_MSG_CHECKING([if the compiler accepts ${fl}path for runtime libraries])
		AC_LINK_IFELSE([AC_LANG_PROGRAM()],
			AC_MSG_RESULT([yes])
			rpath=$fl, 
			AC_MSG_RESULT([no]))
		test -n "$rpath" && break
	done

	if test -n "$rpath"; then
		X_LIBS="$X_LIBS $xlib_rpath"
	fi
	LIBS=$pcb_save_LIBS
fi

############################################################################
# Check for gtk+
# 

gui_gtk=yes
AC_MSG_CHECKING([if gtk should be configured])
AC_ARG_ENABLE([gtk],
[  --disable-gtk           Disable configure detection of gtk.  Not complete yet. [[default: enabled]]],
[
if test "X$enable_gtk" = "Xno" ; then
	AC_MSG_RESULT([no])
	gui_gtk=no
else
	AC_MSG_RESULT([yes])
fi
],
[
	AC_MSG_RESULT([yes])
])

if test "X$gui_gtk" = "Xyes" ; then
	if test "X$gui_xaw" != "Xno" ; then
		AC_MSG_ERROR([To use --enable-gtk, you must also use --disable-xaw])
	fi

	# Check for pkg-config
	AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
	if test "$PKG_CONFIG" = "no"; then
		AC_MSG_ERROR([Cannot find pkg-config, make sure it is installed and in your PATH])
	fi

	PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.2.0, , 
		[AC_MSG_ERROR([Cannot find gtk+ >= 2.2.0, install it and rerun ./configure])]
	)
	GTK_VERSION=`$PKG_CONFIG gtk+-2.0 --modversion`

	# Search for glib
	PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.2.0, , 
		[AC_MSG_ERROR([Cannot find glib >= 2.2.0, install it and rerun ./configure.])]
	)
	GLIB_VERSION=`$PKG_CONFIG glib-2.0 --modversion`
fi


# 
# end of check for gtk+
############################################################################

#
# make sure we didn't try to configure multiple gui's
#
if test "X$gui_gtk" = "Xno" -a "X$gui_xaw" != "Xyes" ; then
	AC_MSG_ERROR([You must enable some gui toolkit (gtk+ or Xaw)])
fi

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(string.h)
AC_CHECK_HEADERS(regex.h)

dnl Checks for typedefs, structures, and compiler characteristics.

dnl Checks for data types

dnl Checks for library functions.

# ------------- dmalloc -------------------
dnl dmalloc checks
with_dmalloc=no
AC_MSG_CHECKING([if dmalloc debugging should be enabled])
AC_ARG_ENABLE([dmalloc],
[  --enable-dmalloc        Compile and link with dmalloc for malloc debugging [default=no]],
[
if test "X$enable_dmalloc" != "Xno" ; then
	AC_MSG_RESULT([yes])
	AC_CHECK_HEADER(dmalloc.h,,
		AC_ERROR([You have requested dmalloc debugging but dmalloc.h could not be found]))
	AC_CHECK_LIB(dmalloc,main,,
		AC_ERROR([You have requested dmalloc debugging but -ldmalloc could not be found]), $X_LIBS)
	DMALLOC_LIBS="-ldmalloc"
	with_dmalloc=yes
else
	AC_MSG_RESULT([no])
	DMALLOC_LIBS=""
fi
],
[
	AC_MSG_RESULT([no])
	DMALLOC_LIBS=""
])

# ------------- ElectricFence -------------------
dnl ElectricFence checks
with_efence=no
AC_MSG_CHECKING([if ElectricFence debugging should be enabled])
AC_ARG_ENABLE([efence],
[  --enable-efence         Link with ElectricFence for malloc debugging [default=no]],
[
if test "X$enable_efence" != "Xno" ; then
	AC_MSG_RESULT([yes])
	AC_CHECK_LIB(efence,main,,
		AC_ERROR([You have requested ElectricFence debugging but -lefence could not be found]), $X_LIBS)
	with_efence=yes
else
	AC_MSG_RESULT([no])
fi
],
[
AC_MSG_RESULT([no])
])

# XXX fix this!!!
#if [ test "x$prefix" = xNONE ]; then
#	datadir=$ac_default_prefix/share
#else
#	datadir=$prefix/share
#fi

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

CFLAGS="$CFLAGS $X_CFLAGS $GTK_CFLAGS"
LIBS="$LIBS $X_LIBS $GTK_LIBS $DMALLOC_LIBS"

BTNMOD=${BTNMOD:-"Mod1"}
AC_SUBST(BTNMOD)

AC_OUTPUT(
	Makefile
	README_FILES/Makefile
	doc/Makefile
	example/Makefile
	example/libraries/Makefile
	lib/CreateLibraryContents.sh
	lib/CreateLibrary.sh
	lib/Makefile
	lib/QueryLibrary.sh
	lib/qfp-ui
	newlib/2_pin_thru-hole_packages/Makefile
	newlib/Makefile
	newlib/analog-devices/Makefile
	newlib/burr-brown/Makefile
	newlib/connectors/Makefile
	newlib/crystal/Makefile
	newlib/cypress/Makefile
	newlib/electro-optics/Makefile
	newlib/generic_SMD_packages/Makefile
	newlib/headers/Makefile
	newlib/msp430/Makefile
	newlib/not_vetted_ingo/Makefile
	newlib/sockets/Makefile
	newlib/tests/Makefile
	newlib/toko/Makefile
	src/Makefile
	src/script/Makefile
	src/icons/Makefile
	tools/Makefile
	tutorial/Makefile
	)


AC_MSG_RESULT([
** Configuration summary for $PACKAGE $VERSION:

   Athena Widgets Based GUI: $gui_xaw
   GTK+ Based GUI:           $gui_gtk
   GTK+ library version:     $GTK_VERSION
   GLIB library version:     $GLIB_VERSION
   dmalloc debugging:        $with_dmalloc
   ElectricFence debugging:  $with_efence
   CPPFLAGS:                 $CPPFLAGS
   CFLAGS:                   $CFLAGS
   LIBS:                     $LIBS

])
