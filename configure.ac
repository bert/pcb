dnl $Id$
dnl Process this file with autoconf to produce a configure script.

AC_INIT(src/pinout.c)
AM_INIT_AUTOMAKE(pcb, 1.99o)
AM_CONFIG_HEADER(config.h)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CC_STDC
AM_PROG_LEX
AC_PROG_YACC
AC_PROG_INSTALL
AC_CHECK_PROGS(M4, gm4 m4)

AC_PATH_PROG(WISH, wish,[none])
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

dnl Checks for libraries.
AC_CHECK_LIB(m, sqrt)
AC_CHECK_LIB(xnet, gethostbyname)
AC_CHECK_LIB(fl, yywrap)
AC_PATH_XTRA
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
AC_CHECK_LIB(Xaw, XawInitializeWidgetSet, , , $X_LIBS)

dnl Determine the Xfuncproto control definitions:
FC_CHECK_X_PROTO_DEFINE(FUNCPROTO)
if test -n "$fc_x_proto_value"; then
	AC_DEFINE_UNQUOTED(FUNCPROTO, $fc_x_proto_value, [FUNCPROTO used by Xaw])
fi
FC_CHECK_X_PROTO_DEFINE(NARROWPROTO)
if test -n "$fc_x_proto_value"; then
	AC_DEFINE_UNQUOTED(NARROWPROTO, $fc_x_proto_value, [NARROWPROTO used by Xaw])
fi

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
for fl in "-R" "-R " "-rpath "; do
    xlib_rpath=`echo $xlib_path | sed "s/-L/$fl/g"`
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

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADER(string.h)

dnl Checks for typedefs, structures, and compiler characteristics.

dnl Checks for data types

dnl Checks for library functions.
AC_CHECK_FUNCS(strerror)

# ------------- dmalloc -------------------
dnl dmalloc checks
AC_MSG_CHECKING([if dmalloc debugging should be enabled])
AC_ARG_ENABLE([dmalloc],
[  --enable-dmalloc        Compile and link with dmalloc for malloc debugging [default=no]],
[
if test "X$enable_dmalloc" != "Xno" ; then
	AC_MSG_RESULT([yes])
	AC_CHECK_HEADER(dmalloc.h)
	AC_CHECK_LIB(dmalloc)
	DMALLOC_LIBS="-ldmalloc"
else
	AC_MSG_RESULT([no])
	DMALLOC_LIBS=""
fi
],
[
	AC_MSG_RESULT([no])
	DMALLOC_LIBS=""
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


CFLAGS="$CFLAGS $X_CFLAGS"
LIBS="$LIBS $X_LIBS $DMALLOC_LIBS"

BTNMOD=${BTNMOD:-"Mod1"}
AC_SUBST(BTNMOD)

AC_OUTPUT(
	Makefile
	README_FILES/Makefile
	doc/Makefile
	example/Makefile
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
	src/icons/Makefile
	tools/Makefile
	tutorial/Makefile
	)


