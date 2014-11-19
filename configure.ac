dnl Process this file with autoconf to produce a configure script.

AC_INIT([pcb], [1.99z])
AC_CONFIG_SRCDIR([src/draw.c])
AC_PREREQ([2.60])
AM_INIT_AUTOMAKE([1.9])
AC_GNU_SOURCE
AC_CONFIG_HEADERS([config.h])

##########################################################################
#
# Try to figure out if we are building from git sources.
# If we are then unless building of the documentation has
# been disabled then just require the user have all the right
# tools.  Users building from a tarball won't need latex, makeinfo,
# etc. but if you're already downloading development sources, then
# it is not unreasonable to require development tools.  What motivated
# this is that using maintainer mode proved to cause regular confusion.

pcb_sources="tarball"
AC_MSG_CHECKING([if you are building from a git checkout])
pcb_git_version=no
if test -f $srcdir/.gitignore ; then
	pcb_git_version=yes
	AC_MSG_RESULT([yes])
	pcb_sources="GIT"
else
	AC_MSG_RESULT([no])
fi
AM_CONDITIONAL(GIT_VERSION, test x$pcb_git_version = xyes)

AC_MSG_CHECKING([if you are building from a anoncvs checkout])
pcb_cvs_version=no
if test -f $srcdir/CVS/Root ; then
	pcb_cvs_version=yes
	AC_MSG_RESULT([yes])
	pcb_sources="CVS"
else
	AC_MSG_RESULT([no])
fi
AM_CONDITIONAL(CVS_VERSION, test x$pcb_cvs_version = xyes)
AM_CONDITIONAL(GIT_OR_CVS_VERSION, test x$pcb_git_version = xyes -o x$pcb_cvs_version = xyes)


##########################################################################
#
# See if we are supposed to build the docs
#

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


dnl determine host type
AC_CANONICAL_HOST
AC_MSG_CHECKING(for windows)
PCB_PATH_DELIMETER=":"
PCB_DIR_SEPARATOR_S="/"
PCB_DIR_SEPARATOR_C='/'
case $host in
	*-*-mingw* )
		WIN32=yes
		CFLAGS="$CFLAGS ${MINGW_CFLAGS:--mms-bitfields -mwindows}"
		CPPFLAGS="$CPPFLAGS ${MINGW_CPPFLAGS}"
		;;

	* )
		WIN32=no
		;;
esac

AC_MSG_RESULT($WIN32)
AC_SUBST(WIN32)
AM_CONDITIONAL(WIN32, test x$WIN32 = xyes)
if test "x$WIN32" = "xyes" ; then
	PCB_PATH_DELIMETER=";"
	PCB_DIR_SEPARATOR_S="\\\\"
	PCB_DIR_SEPARATOR_C='\\'
fi

AC_DEFINE_UNQUOTED(PCB_DIR_SEPARATOR_C,'$PCB_DIR_SEPARATOR_C',[Directory separator char])
AC_DEFINE_UNQUOTED(PCB_DIR_SEPARATOR_S,"$PCB_DIR_SEPARATOR_S",[Directory separator string])
AC_DEFINE_UNQUOTED(PCB_PATH_DELIMETER,"$PCB_PATH_DELIMETER",[Search path separator string])


dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX

if test "x$WIN32" = "xyes" ; then
	AC_CHECK_TOOL(WINDRES, windres, [no])
	if test "$WINDRES" = "no" ; then
		AC_MSG_ERROR([*** Could not find an implementation of windres in your PATH.])
	fi
fi

# i18n
GETTEXT_PACKAGE=$PACKAGE
AH_TEMPLATE([GETTEXT_PACKAGE], [Name of this program's gettext domain])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"])
AC_SUBST(GETTEXT_PACKAGE)

AM_GNU_GETTEXT_VERSION([0.14])
AM_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])

AC_C_INLINE
AC_PROG_CC_STDC
AC_PROG_CC_C99
AM_PROG_CC_C_O
AC_PROG_CPP

AC_MSG_CHECKING([that C compiler supports C99])
if test "X$ac_cv_prog_cc_c99" = "Xno"; then
  AC_MSG_RESULT([no])
  AC_MSG_ERROR([A C99-compliant compiler is requred])
else
  AC_MSG_RESULT([yes])
fi

AC_PROG_AWK
AC_PROG_MKDIR_P

AM_PROG_LEX
AC_PATH_PROG(LEX_PATH, $LEX, [notfound])
if test "$LEX_PATH" = "notfound" ; then
	AC_MSG_ERROR([Couldn't find a usable lex program.
Please install flex which is available from
ftp://ftp.gnu.org/pub/non-gnu/flex/
])
fi


AC_PROG_YACC
AC_PATH_PROG(YACC_PATH, $YACC, [notfound])
if test "$YACC_PATH" = "notfound" ; then
	AC_MSG_ERROR([Couldn't find a usable yacc program.
Please install bison which is available from
ftp://ftp.gnu.org/pub/gnu/bison/
])
fi

AC_PROG_INSTALL
AC_PROG_RANLIB

#
# Used for building the icons
#
AC_PATH_PROG(XPMTOPPM, xpmtoppm, notfound)
AC_PATH_PROG(PPMTOWINICON, ppmtowinicon, notfound)
AC_PATH_PROG(CONVERT, convert, notfound)

##########################################################################
#
#
if test "X$docs_yesno" = "Xyes" -a "X$pcb_git_version" = "Xyes" ; then
   AC_PATH_PROG(MKINFO, makeinfo, no)
   if test "X$MKINFO" != "Xno"; then
      AC_MSG_CHECKING([for GNU makeinfo version >= 4.6])
      v=`$MKINFO --version | grep "GNU texinfo"`
      if test $? -ne 0; then
         AC_MSG_RESULT([non-GNU])
         MKINFO="no"
      fi
   fi
   if test "X$MKINFO" != "Xno"; then
      vmajor=`echo "$v" | sed 's/.* \([[0-9]]*\)\.\([[0-9]]*\)$/\1/'`
      vminor=`echo "$v" | sed 's/.* \([[0-9]]*\)\.\([[0-9]]*\)$/\2/'`
      AC_MSG_RESULT([$vmajor.$vminor])
      if test "$vmajor" -lt 4 \
              || (test "$vmajor" -eq 4 && test "$vminor" -lt 6); then
         MKINFO=no
      fi
   fi
   if test "X$MKINFO" = "Xno"; then
      AC_MSG_ERROR([You have requested a build
of the documentation.  For this to work, you must have version 4.6 or newer of
the GNU texinfo package.  You seem to have

$v

Please update to a newer version of texinfo or disable building of
the documentation with --disable-doc
])
   fi

   AC_PATH_PROG(TEXI2DVI, texi2dvi, no)
   if test "X$TEXI2DVI" = "Xno"; then
      AC_MSG_ERROR([You have requested a build
of the documentation.  For this to work, you must have the texi2dvi program
installed.  Alternatively, you can disable building the documentation with
--disable-doc.])
   fi


	AC_PATH_PROG(PERL, perl, notfound)
	if test "X$PERL" = "Xnotfound"; then
			AC_MSG_ERROR([You have requested a build
of the documentation.  For this to work, you must have perl installed.
Alternatively, you can disable building the documentation with
--disable-doc.
])
	fi


   AC_PATH_PROG([KPSEWHICH], [kpsewhich], [no])
   if test "X$KPSEWHICH" = "Xno"; then
      AC_MSG_ERROR([You have requested a build
of the documentation.  For this to work, you must have a functional install of
TeX and LaTeX.  kpsewhich is part of TeX.
Alternatively, you can disable building the documentation with
--disable-doc.])
   fi

   AC_MSG_CHECKING([If your TeX installation contains epsf.tex])
   f=`$KPSEWHICH epsf.tex`
   if test $? -eq 0 ; then
	AC_MSG_RESULT([yes ($f)])
   else
	AC_MSG_RESULT([no])
        AC_MSG_ERROR([You have requested a build
of the documentation.  For this to work, you must have a functional install of
TeX and LaTeX that includes epsf.tex.  On some linux distributions this is
part of the texlive-generic-recommended package.
Alternatively, you can disable building the documentation with
--disable-doc.])
   fi

fi

##########################################################################
#
#

# FIXME:  for now, only try to add -rdynamic if we're using gcc.  We really
# need to figure out what the correct test is here.  In the mean time, this
# should let things build with SunPRO again.
if test "x$GCC" = "xyes"; then
AC_MSG_CHECKING([If the compiler accepts -rdynamic])
old_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS -rdynamic"
AC_LINK_IFELSE([AC_LANG_PROGRAM()],
	[AC_MSG_RESULT([yes])],
	[LDFLAGS="$old_LDFLAGS"
	AC_MSG_RESULT([no])
	])
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
[  --with-gui=	          Specify the GUI to use: batch gtk lesstif [[default=gtk]]],
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

AC_MSG_CHECKING([whether to enable toporouter])
AC_ARG_ENABLE([toporouter],
 [AS_HELP_STRING([--enable-toporouter], [build toporouter [default=yes]]) ]
)
AS_CASE(["x$enable_toporouter"],[xyes | xno],,
 [enable_toporouter=yes
 ]
)
AC_MSG_RESULT([$enable_toporouter])
AM_CONDITIONAL([WITH_TOPOROUTER], test $enable_toporouter != no)

if test "x$enable_toporouter" = "xyes"; then
  this_error_text="
    You have requested to build with the toporouter.  For this to work you
    must have the GTS library installed, typically found in packages named
    'libgts-dev' or similar.

    Alternatively you can disable the toporouter with --disable-toporouter."
  # Testing for gts.h requires glib.h, which is tested for later. To not take
  # apart toporouter related stuff and because testing for the library makes
  # reasonably sure that GTS is available, we omit a test for gts.h.
  #AC_CHECK_HEADER(gts.h, , AC_MSG_ERROR($this_error_text), glib.h)
  AC_SEARCH_LIBS(gts_object_class_new, gts, , AC_MSG_ERROR($this_error_text))
  unset this_error_text
fi

AC_MSG_CHECKING([whether to enable toporouter output])
AC_ARG_ENABLE([toporouter-output],
 [AS_HELP_STRING([--enable-toporouter-output], [enable toporouter graphical output [default=no]]) ]
)
AS_CASE(["z$enable_toporouter_output"],[zyes | zno],,
 [enable_toporouter_output=no]
)
AC_MSG_RESULT([$enable_toporouter_output])
AS_CASE([$enable_toporouter_output],[yes],
 [
  topo_output_enabled=1
 ],
 [
  topo_output_enabled=0
 ]
)
AC_DEFINE_UNQUOTED([TOPO_OUTPUT_ENABLED], [$topo_output_enabled],
 [Define to 1 to enable toporouter graphical output]
)

PKG_PROG_PKG_CONFIG()

if test "x$enable_toporouter_output" = "xyes"; then
   PKG_CHECK_MODULES(CAIRO, cairo,,
                     [AC_MSG_ERROR([Cannot find cairo, needed by the toporouter
Please review the following errors:
$CAIRO_PKG_ERRORS])]
   )
fi

AC_MSG_CHECKING([for whether to use DBUS])
AC_ARG_ENABLE([dbus],
[  --enable-dbus           Enable DBUS IPC],
[],[
	case " $with_gui " in
		*\ gtk\ *)     enable_dbus=yes ;;
		*\ lesstif\ *) enable_dbus=yes ;;
		* )            enable_dbus=no ;;
	esac
])

AC_MSG_RESULT([$enable_dbus])
AM_CONDITIONAL(WITH_DBUS, test x$enable_dbus = xyes)

if test "x$enable_dbus" = "xyes"; then
	case " $with_gui " in
		*\ gtk\ *) ;;
		*\ lesstif\ *) ;;
		* ) AC_MSG_ERROR([DBUS enabled but only works with a mainloop capable GUI HID.
Either do not use --enable-dbus or enable the gtk or lesstif GUI HID.])
	esac
	
	PKG_CHECK_MODULES(DBUS, dbus-1 >= 0.61,
		[saved_LIBS="$LIBS"
		 LIBS="$LIBS $DBUS_LIBS"
		 AC_CHECK_FUNCS(dbus_watch_get_unix_fd)
		 LIBS="$saved_LIBS" ],
		[AC_MSG_ERROR([Cannot find dbus-1 >= 0.61, install it and rerun ./configure
Please review the following errors:
$DBUS_PKG_ERRORS])]
	)
	DBUS_VERSION=`$PKG_CONFIG dbus-1 --modversion`
	
	AC_DEFINE([HAVE_DBUS], 1,
		[Define to 1 if DBUS IPC is to be compiled in])

fi


AC_MSG_CHECKING([for whether to use GL drawing])
AC_ARG_ENABLE([gl],
[  --enable-gl           Enable GL drawing (with GTK HID)],
[],[
	case " $with_gui " in
		*\ gtk\ *) enable_gl=yes;;
		* ) enable_gl=no;;
	esac
])
AC_MSG_RESULT([$enable_gl])
AM_CONDITIONAL(USE_GL, test x$enable_gl = xyes)

if test "x$enable_gl" = "xyes"; then
	case " $with_gui " in
		*\ gtk\ *) ;;
		* ) AC_MSG_ERROR([GL drawing enabled but only works with the GTK HID.
Either do not use --enable-gl or enable the gtk HID.])
	;;
	esac

	AX_CHECK_GL
	AS_IF([test X$no_gl = Xyes],
	      [AC_MSG_FAILURE([OpenGL is required.])])

	AX_CHECK_GLU
	AS_IF([test X$no_glu = Xyes],
	      [AC_MSG_FAILURE([The OpenGL GLU library is required.])])

	AC_DEFINE([ENABLE_GL], 1,
		[Define to 1 if GL support is to be compiled in])
fi

AC_MSG_CHECKING([for which printer to use])
AC_ARG_WITH([printer],
[  --with-printer= 	  Specify the printer: lpr [[default=lpr]]],
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
[  --with-exporters=       Enable export devices: bom gerber gcode nelma png ps [[default=bom gerber gcode nelma png ps]]],
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

if test "X$enable_jpeg" = "Xno" -a "X$enable_gif" = "Xno" -a "X$enable_png" = "Xno" ; then
	case " ${HIDLIST} " in
		*\ png\ *)
			AC_MSG_ERROR([you have requested the png HID but turned off all output
formats!  If you do not want gif/jpeg/png output, use --with-exporters to list
which exporters you want and do not list png there.])
			;;

		*)
			;;
	esac
fi

for hid in $HIDLIST; do
    F=$srcdir/src/hid/$hid/hid.conf
    if test -f $F ; then
        echo checking $hid dependencies
        deps=
        . $F
        for dep in $deps; do
            if test "X`echo $HIDLIST | grep $dep`" = "X"; then
                AC_MSG_ERROR([you have requested the $hid HID but not the $dep HID, which it depends on])
            fi
        done
   fi
done

for e in $HIDLIST; do
    HIDLIBS="$HIDLIBS lib$e.a"
done

AC_SUBST(HIDLIST)
AC_SUBST(HIDLIBS)

# ------------- end HID config -------------------

######################################################################
#
# desktop integration
#

AC_PATH_PROG(SETENV, env, [])
AC_PATH_PROG(GTK_UPDATE_ICON_CACHE_BIN, gtk-update-icon-path, [true])

# Change default location for XDG files (MIME and Icons)
AC_ARG_WITH(xdgdatadir, [  --with-xdgdatadir=path  Change where the theme icons 
and mime registrations are installed [[DATADIR]]], [opt_xdgdatadir=$withval])

# Change default location for KDE data files (KDE MIME registrations)
AC_ARG_WITH(kdedatadir, [  --with-kdedatadir=path  Change where the KDE mime reg
istrations are installed [[DATADIR]]], [opt_kdedatadir=$withval])

if test x$opt_xdgdatadir = x; then
	# path was not specified with --with-xdgdatadir
	XDGDATADIR='${datadir}'
else
	# path WAS specified with --with-xdgdatadir
	XDGDATADIR="$opt_xdgdatadir"
fi
AC_SUBST(XDGDATADIR)

if test x$opt_kdedatadir = x; then
	# path was not specified with --with-kdedatadir
	KDEDATADIR='${datadir}'
else
	# path WAS specified with --with-kdedatadir
	KDEDATADIR="$opt_kdedatadir"
fi
AC_SUBST(KDEDATADIR)

AC_ARG_ENABLE(update-desktop-database,
	AC_HELP_STRING([--disable-update-desktop-database],
	[do not update desktop database after installation]),,
	enable_update_desktop_database=yes)

AM_CONDITIONAL(ENABLE_UPDATE_DESKTOP_DATABASE, test x$enable_update_desktop_database = xyes)

if test x$enable_update_desktop_database = xyes ; then
	AC_PATH_PROG(UPDATE_DESKTOP_DATABASE, [update-desktop-database], no)
	if test $UPDATE_DESKTOP_DATABASE = no; then
	AC_MSG_ERROR([Cannot find update-desktop-database, make sure it is installed and in your PATH, or configure with --disable-update-desktop-database])
	fi
fi

AC_ARG_ENABLE(update-mime-database,
	AC_HELP_STRING([--disable-update-mime-database],
	[do not update mime database after installation]),,
	enable_update_mime_database=yes)

AM_CONDITIONAL(ENABLE_UPDATE_MIME_DATABASE, test x$enable_update_mime_database = xyes)

if test x$enable_update_mime_database = xyes ; then
	AC_PATH_PROG(UPDATE_MIME_DATABASE, [update-mime-database], no)
	if test $UPDATE_MIME_DATABASE = no; then
	AC_MSG_ERROR([Cannot find update-mime-database, make sure it is installed and in your PATH, or configure with --disable-update-mime-database])
	fi
fi

#
######################################################################

AC_PATH_PROGS(M4, gm4 m4, [none])
if test "X$M4" = "Xnone" ; then
	AC_MSG_ERROR([Did not find a m4 executible.  You need to make sure
	that m4 is installed on your system and that m4 is in your path])
fi
AC_MSG_CHECKING([if $M4 has the division involving negative numbers bug])
pcb_m4_r=`echo "eval(-2/2)" | $M4`
if test "$pcb_m4_r" != "-1" ; then
	AC_MSG_RESULT([yes])
	AC_MSG_ERROR([It appears that $M4 has a bug involving division
with negative numbers.  In particular it just returned the result that
-2/2 = $pcb_m4_r instead of -1.  This is a known bug in GNU m4-1.4.9.  Please
install a non-broken m4.])
else
	AC_MSG_RESULT([no])
fi


AC_PATH_PROGS(WISH, wish wish85 wish8.5 wish83 wish8.3 wish80 wish8.0 cygwish83 cygwish80,[none])
if test "X$WISH" = "Xnone" ; then
	AC_MSG_ERROR([Did not find the wish executible.  You need to make sure
	that tcl is installed on your system and that wish is in your path])
fi

AC_DEFINE_UNQUOTED(M4,$M4,[m4 executible])
GNUM4=$M4
AC_SUBST(GNUM4)
AC_DEFINE_UNQUOTED(GNUM4,"$M4",[m4 program used by pcb])

AC_PATH_PROG(PDFLATEX, pdflatex, notfound)
AM_CONDITIONAL(MISSING_PDFLATEX, test x$PDFLATEX = xnotfound)

AC_PATH_PROG(TEXI2DVI, texi2dvi, notfound)
AM_CONDITIONAL(MISSING_TEXI2DVI, test x$TEXI2DVI = xnotfound)

AC_PATH_PROG(PS2PDF, ps2pdf, notfound)
AM_CONDITIONAL(MISSING_PS2PDF, test x$PS2PDF = xnotfound)

# used to build some of the getting started guide
AC_PATH_PROG(GSCHEM, gschem, notfound)
AM_CONDITIONAL(MISSING_GSCHEM, test x$GSCHEM = xnotfound)

if test "X$docs_yesno" = "Xyes" -a "X$pcb_git_version" = "Xyes" ; then
	if test "$PDFLATEX" = "notfound" -o "$TEXI2DVI" = "notfound" -o "$PS2PDF" = "notfound" ; then
		AC_MSG_ERROR([It appears that you are building from a source tree obtained
via git but you do not have the required tools installed to build the documentation.  Here
is a list of tools and the detected values:
PDFLATEX:  $PDFLATEX
TEXI2DVI:  $TEXI2DVI
PS2PDF:    $PS2PDF
GSCHEM:    $GSCHEM

Either make sure these tools are installed or disable building and installing the documentation
by using the --disable-doc configure option.
])
	fi
fi

############################################################################
#
# These checks are for tools used by the testsuite.  It will not be fatal
# if these are missing because this is primarily for developer use.  It is
# possible that we might add some --enable flag in the future that forces
# full tools for development work.

# Check for ImageMagick tools used by the testsuite
AC_PATH_PROG(IM_ANIMATE, animate, notfound)
AC_PATH_PROG(IM_COMPARE, compare, notfound)
AC_PATH_PROG(IM_COMPOSITE, composite, notfound)
AC_PATH_PROG(IM_CONVERT, convert, notfound)
AC_PATH_PROG(IM_DISPLAY, display, notfound)
AC_PATH_PROG(IM_MONTAGE, montage, notfound)
missing_magick=""
test "${IM_ANIMATE}" != "notfound" || missing_magick="${missing_magick} animate"
test "${IM_COMPARE}" != "notfound" || missing_magick="${missing_magick} compare"
test "${IM_COMPOSITE}" != "notfound" || missing_magick="${missing_magick} composite"
test "${IM_CONVERT}" != "notfound" || missing_magick="${missing_magick} convert"
test "${IM_DISPLAY}" != "notfound" || missing_magick="${missing_magick} display"
test "${IM_MONTAGE}" != "notfound" || missing_magick="${missing_magick} montage"

AC_MSG_CHECKING([if all ImageMagick tools needed for the testsuite were found])
if test "X${missing_magick}" != "X" ; then
    AC_MSG_RESULT([no.  The testsuite will be disabled because the following
tools from the ImageMagick suite were not found:
${missing_magick}
No loss in pcb functionality should be experienced, you just will not
be able to run the full regression testsuite.
])
    have_magick=no
else
    AC_MSG_RESULT([yes])
    have_magick=yes
fi

have_test_tools=yes

test $have_magick = yes || have_test_tools=no

# the RS274-X export HID is partially checked by looking at the result with
# gerbv
AC_PATH_PROG(GERBV, gerbv, notfound)
test $GERBV != notfound || have_test_tools=no


AM_CONDITIONAL(HAVE_TEST_TOOLS, test x$have_test_tools = xyes)
AC_MSG_CHECKING([if all the requried testsuite tools were found])
if test x$have_test_tools = xyes ; then
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no -- the testsuite will be disabled])
fi


#
############################################################################


dnl Checks for libraries.
AC_CHECK_LIB(m, sqrt)
AC_CHECK_LIB(dl, dlopen)
AC_CHECK_LIB(xnet, gethostbyname)
AC_CHECK_LIB(fl, yywrap)
AC_CHECK_FUNCS(strerror)
AC_CHECK_FUNCS(regcomp re_comp)
AC_CHECK_FUNCS(logf expf rint)
AC_CHECK_FUNCS(vsnprintf)
AC_CHECK_FUNCS(getpwuid getcwd)
AC_CHECK_FUNCS(rand random)
AC_CHECK_FUNCS(stat)

AC_CHECK_FUNCS(mkdtemp)

# normally used for all file i/o
AC_CHECK_FUNCS(popen)

# for lrealpath.c
AC_CHECK_FUNCS(realpath canonicalize_file_name)
libiberty_NEED_DECLARATION(canonicalize_file_name)

# for pcb_spawnvp in action.c on Windows
AC_CHECK_FUNCS(_spawnvp)

AC_HEADER_STDC
AC_CHECK_HEADERS(limits.h locale.h string.h sys/types.h regex.h pwd.h)
AC_CHECK_HEADERS(sys/socket.h netinet/in.h netdb.h sys/param.h sys/times.h sys/wait.h)
AC_CHECK_HEADERS(dlfcn.h)

if test "x${WIN32}" = "xyes" ; then
	AC_CHECK_HEADERS(windows.h)
fi
# Search for glib
PKG_CHECK_MODULES(GLIB, glib-2.0, ,
		[AC_MSG_RESULT([Note: cannot find glib-2.0.
You may want to review the following errors:
$GLIB_PKG_ERRORS])]
)

need_gdlib=no
with_gif=no
with_png=no
with_jpeg=no

for e in $HIDLIST; do
    case $e in
      lesstif )
	AC_PATH_XTRA
	save_CPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$X_CFLAGS"
	AC_CHECK_LIB(X11, XOpenDisplay, , , $X_LIBS)
	AC_CHECK_LIB(ICE, main, , , $X_LIBS)
	AC_CHECK_LIB(SM, main, , , $X_LIBS)
	AC_CHECK_LIB(Xext, main, , , $X_LIBS)
	AC_CHECK_LIB(Xt, XtOpenDisplay, , , $X_LIBS)
	AC_CHECK_LIB(Xmu, main, , , $X_LIBS)
	AC_CHECK_LIB(Xpm, main, , , $X_LIBS)
	AC_CHECK_LIB(Xm, XmCreateMainWindow, , , $X_LIBS)
	CPPFLAGS="$save_CPPFLAGS"
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

	PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.18.0, ,
		[AC_MSG_ERROR([Cannot find gtk+ >= 2.18.0, install it and rerun ./configure
Please review the following errors:
$GTK_PKG_ERRORS])]
	)
	GTK_VERSION=`$PKG_CONFIG gtk+-2.0 --modversion`
	GLIB_VERSION=`$PKG_CONFIG glib-2.0 --modversion`

	if test "x$enable_gl" = "xyes"; then
		# Check for GtkGLExt
		PKG_CHECK_MODULES(GTKGLEXT, gtkglext-1.0 >= 1.0.0, , [AC_MSG_ERROR([
*** Required version of gtkglext is not installed - please install first ***
Please review the following errors:
$GTKGLEXT_PKG_ERRORS])]
		)
	GTKGLEXT_VER=`$PKG_CONFIG gtkglext-1.0 --modversion`
	fi

	;;

      png)
	need_gdlib=yes
	AC_MSG_CHECKING([if GIF output from the png HID is desired])
	AC_ARG_ENABLE([gif],
		[  --disable-gif           Disable support for gif output when the png HID is used [[default=include gif support]]],
		[
		if test "X$enable_gif" != "Xno" ; then
			AC_MSG_RESULT([yes])
			with_gif=yes
		else
			AC_MSG_RESULT([no])
		fi
		],
		[
		AC_MSG_RESULT([yes])
		with_gif=yes
		])

	AC_MSG_CHECKING([if JPEG output from the png HID is desired])
	AC_ARG_ENABLE([jpeg],
		[  --disable-jpeg          Disable support for JPEG output when the png HID is used [[default=include JPEG support]]],
		[
		if test "X$enable_jpeg" != "Xno" ; then
			AC_MSG_RESULT([yes])
			with_jpeg=yes
		else
			AC_MSG_RESULT([no])
		fi
		],
		[
		AC_MSG_RESULT([yes])
		with_jpeg=yes
		])

	AC_MSG_CHECKING([if PNG output from the png HID is desired])
	AC_ARG_ENABLE([png],
		[  --disable-png           Disable support for PNG output when the png HID is used [[default=include PNG support]]],
		[
		if test "X$enable_png" != "Xno" ; then
			AC_MSG_RESULT([yes])
			with_png=yes
		else
			AC_MSG_RESULT([no])
		fi
		],
		[
		AC_MSG_RESULT([yes])
		with_png=yes
		])
	;;

      gcode|nelma)
	need_gdlib=yes
	with_png=yes
	;;

    esac
done

if test "$need_gdlib" = "yes"; then
	# Check for gdlib-config for gd (www.boutell.com/gd)
	AC_PATH_PROG(GDLIB_CONFIG, gdlib-config, no)
	if test "$GDLIB_CONFIG" = "no"; then
		AC_MSG_RESULT([Cannot find gdlib-config.
Make sure it is installed and in your PATH.
gdlib-config is part of the GD library available from www.boutell.com/gd.
This is needed for the png HID.  I will look for libgd anyway and maybe
you will get lucky.
])
		if test "$WIN32" != "yes" ; then
			AC_CHECK_LIB(gd,main,,
			AC_MSG_ERROR([[You have requested gcode, nelma, or png HIDs  but -lgd could not be found]]))
		else 
			AC_CHECK_LIB(bgd,main,,
			AC_MSG_ERROR([[You have requested gcode, nelma, or png HIDs  but -lbgd could not be found]]))
		fi
	else
		if test "$WIN32" = "yes" ; then
			GD=bgd
		else
			GD=gd
		fi
		AC_MSG_CHECKING([for libgd cflags])
		GD_CFLAGS="`$GDLIB_CONFIG --cflags`"
		AC_MSG_RESULT([$GD_CFLAGS])
		AC_MSG_CHECKING([for libgd libs])
		GD_LIBS="`$GDLIB_CONFIG --ldflags` `$GDLIB_CONFIG --libs` -l${GD}"
		AC_MSG_RESULT([$GD_LIBS])
	fi
   
	# since some linux systems evidently install gdlib-config but fail to
	# install the headers (nice), check for the header too and fail if it
	# is not there.
	save_CPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$CPPFLAGS $GD_CFLAGS"
	AC_CHECK_HEADERS(gd.h)
	CPPFLASS="$save_CPPFLAGS"
	case $ac_cv_header_gd_h in
	     no )
		AC_MSG_ERROR([
You evidentally do not have a complete installation of the GD library available from www.boutell.com/gd.
This is needed for the nelma and/or png HID.
])
		;;
	     * ) ;;
	esac
	
	# Some versions of gd (prior to the expiration of the
	# patent related to gif compression) do not support
	# gif output.  Check for that here.
	save_LIBS="$LIBS"
	LIBS="$save_LIBS $GD_LIBS $X_LIBS"

	if test "X$with_gif" = "Xyes" ; then
		AC_CHECK_FUNCS(gdImageGif)
		if test "$ac_cv_func_gdImageGif" != "yes"; then
			AC_MSG_ERROR([Your gd installation does not appear to include gif support.
You may need to update your installation of gd or disable
gif export with --disable-gif])
		fi
	fi

	if test "X$with_jpeg" = "Xyes" ; then
		AC_CHECK_FUNCS(gdImageJpeg)
		if test "$ac_cv_func_gdImageJpeg" != "yes"; then
			AC_MSG_ERROR([Your gd installation does not appear to include JPEG support.
You may need to update your installation of gd or disable
JPEG export with --disable-jpeg])
		fi
	fi

	if test "X$with_png" = "Xyes" ; then
		AC_CHECK_FUNCS(gdImagePng)
		if test "$ac_cv_func_gdImagePng" != "yes"; then
			AC_MSG_ERROR([Your gd installation does not appear to include PNG support.
You may need to update your installation of gd or disable
PNG export with --disable-png])
		fi
	fi
	LIBS="$save_LIBS"
fi

AM_CONDITIONAL(PNG, test x$with_png = xyes)
AM_CONDITIONAL(GIF, test x$with_gif = xyes)

# ------------- check if png previews should be built for pcblib-newlib
AC_MSG_CHECKING([if the m4lib to newlib export should create png previews])
AC_ARG_ENABLE(
	[m4lib-png],
	[  --enable-m4lib-png      Enable creating png previews for the m4 library],
	[],[enable_m4lib_png=no])
AC_MSG_RESULT([$enable_m4lib_png])
AM_CONDITIONAL(PNG_PREVIEW, test x$enable_m4lib_png = xyes)


# Run away.... more ugly stuff here.  By default we don't actually build
# pcblib-newlib from pcblib unless we are building from cvs or git sources.
# The reason is it takes a while and requires the png HID.  The problem is,
# what if someone wants to use --enable-m4lib-png but the tarball was built
# without the previews.  Or, what if someone does not want the PNG previews
# but the person building the tarball included them.  Ugh!  So what the following
# code attempts to do is detect that mismatch situation.  Note that we only
# want to kick this code in when *not* building from git or cvs sources.
build_pcblib_newlib=no
if test "$pcb_sources" = "tarball" ; then
	AC_MSG_CHECKING([If pcblib-newlib was built with png previews])
	stamp=$srcdir/lib/pcblib-newlib.stamp
	if test ! -f ${stamp} ; then
		AC_MSG_RESULT([unknown, missing ${stamp}])
		build_pcblib_newlib=yes
	else
		if test "`cat ${stamp}`" = "png-preview=yes" ; then
			AC_MSG_RESULT([yes])
			# lib exists and built with preview.
			# if we don't want the preview, than rebuild
			if test x$enable_m4lib_png != xyes ; then
				build_pcblib_newlib=yes
			fi
		else
			AC_MSG_RESULT([no])
			# lib exists and built without preview.
			# if we want the preview, than rebuild
			if test x$enable_m4lib_png = xyes ; then
				build_pcblib_newlib=yes
			fi
		fi
	fi
else
	build_pcblib_newlib=yes
fi
AC_MSG_CHECKING([If pcblib-newlib needs to be rebuilt])
AC_MSG_RESULT([$build_pcblib_newlib])
AM_CONDITIONAL(BUILD_PCBLIB_NEWLIB, test x$build_pcblib_newlib = xyes)

if test "X$cross_compiling" = "Xyes" ; then
	# we are cross compiling so we will need a build host binary for pcb
	AC_PATH_PROG(PCB, [pcb], [notfound])
else
	PCB="\${top_builddir}/src/pcb"
fi
AC_SUBST(PCB)

# now make see how essential it was that we have a pcb executable for the build
# host
if test "X$pcb_git_version" = "Xyes" ; then
	if test "X$docs_yesno" = "Xyes" -o "X$enable_m4lib_png" = "Xyes" ; then
		if test "$PCB" = "notfound" ; then
			AC_MSG_ERROR([You have selected a build with m4lib png
previews enabled and/or with building the documentation enabled but you also
appear to be cross-compiling.  For this to work, you must have a pcb installed that
can run on this machine (the build machine) because it is needed for generating
library footprint png previews as well as some of the figures in the documentation.
If you wish to skip building the documentation and the footprint previews then add
--disable-doc --disable-m4lib-png
This will allow your cross build to work.])
		fi
	fi
fi

# ------------- Xrender -------------------
have_xrender=no
AC_CHECK_LIB(Xrender,XRenderQueryExtension,have_xrender=yes,have_xrender=no,$X_LIBS)

AC_ARG_ENABLE([xrender],
[  --disable-xrender       Compile and link with Xrender [default=yes]])
case "$have_xrender:$enable_xrender" in
   no:* ) ;;
   *:no ) ;;
   * )
     X_LIBS="-lXrender $X_LIBS"
     AC_DEFINE([HAVE_XRENDER], 1,
		[Define to 1 if Xrender is available])
     ;;
esac

# ------------- Xinerama -------------------
have_xinerama=no
AC_CHECK_LIB(Xinerama,XineramaQueryExtension,have_xinerama=yes,have_xinerama=no,$X_LIBS)

AC_ARG_ENABLE([xinerama],
[  --disable-xinerama       Compile and link with Xinerama [default=yes]])
case "$have_xinerama:$enable_xinerama" in
   no:* ) ;;
   *:no ) ;;
   * )
     X_LIBS="-lXinerama $X_LIBS"
     AC_DEFINE([HAVE_XINERAMA], 1,
		[Define to 1 if Xinerama is available])
     ;;
esac

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
		AC_MSG_ERROR([You have requested dmalloc debugging but dmalloc.h could not be found]))
	AC_CHECK_LIB(dmalloc,main,,
		AC_MSG_ERROR([You have requested dmalloc debugging but -ldmalloc could not be found]))
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
		AC_MSG_ERROR([You have requested ElectricFence debugging but -lefence could not be found]))
	with_efence=yes
else
	AC_MSG_RESULT([no])
fi
],
[
AC_MSG_RESULT([no])
])

# ------------- Enable Debug code -------------------
AC_MSG_CHECKING([for whether to enable debugging code])
AC_ARG_ENABLE([debug],
[  --enable-debug          Enable debugging code],
[],[enable_debug=no])

AC_MSG_RESULT([$enable_debug])
AM_CONDITIONAL(DEBUG_BUILD, test x$enable_debug = xyes)

# ------------- mkdir required for win32 compatibility ------------
# WIN32 mkdir takes only one argument, POSIX takes two.
# #include "misc.h" where mkdir is required.
m4_include([m4/m4_ax_func_mkdir.m4])
AX_FUNC_MKDIR

# ------------- Type used for "Coord" type -------------------

AC_CHECK_HEADERS(stdint.h)
AC_ARG_ENABLE([coord64],
[  --enable-coord64        Force 64-bit coordinate types],
[],[enable_coord64=no])
AC_ARG_ENABLE([coord32],
[  --enable-coord32        Force 32-bit coordinate types],
[],[enable_coord32=no])

COORDTYPE="long"

echo "$enable_coord32:$enable_coord64:$ac_cv_header_stdint_h"
case "$enable_coord32:$enable_coord64:$ac_cv_header_stdint_h" in
  yes:no:yes )
    COORD_TYPE="int32_t"
    COORD_MAX="INT32_MAX"
    ;;
  no:yes:yes )
    COORD_TYPE="int64_t"
    COORD_MAX="INT64_MAX"
    ;;
  yes:no:no  )
    COORD_TYPE="int"
    COORD_MAX="INT_MAX"
    ;;
  no:yes:no  )
    COORD_TYPE="long long"
    COORD_MAX="LLONG_MAX"
    ;;
  yes:yes:*  )
    AC_MSG_ERROR("*** cannot require both 32 and 64 bit coordinates")
    ;;
  *:*:*    )
    COORD_TYPE="long"
    COORD_MAX="LONG_MAX"
    ;;
esac
AC_DEFINE_UNQUOTED([COORD_TYPE],[$COORD_TYPE],
  [C type for Coordinates])
AC_DEFINE_UNQUOTED([COORD_MAX],[$COORD_MAX],
  [Maximum value of coordinate type])

# ------------- Complete set of CPPFLAGS and LIBS -------------------

CPPFLAGS="$CPPFLAGS $X_CFLAGS $DBUS_CFLAGS $GLIB_CFLAGS $GTK_CFLAGS $GD_CFLAGS $CAIRO_CFLAGS $GTKGLEXT_CFLAGS $GLU_CFLAGS $GL_CFLAGS"
LIBS="$LIBS $XM_LIBS $DBUS_LIBS $X_LIBS $GLIB_LIBS $GTK_LIBS $DMALLOC_LIBS $GD_LIBS $INTLLIBS $CAIRO_LIBS $GTKGLEXT_LIBS $GLU_LIBS $GL_LIBS"


# if we have gcc then add -Wall
if test "x$GCC" = "xyes"; then
	# see about adding some extra checks if the compiler takes them
	for flag in -Wall ; do
		case " ${CFLAGS} " in
			*\ ${flag}\ *)
				# flag is already present
				;;
			*)
				AC_MSG_CHECKING([if the compiler accepts ${flag}])
				ac_save_CFLAGS="$CFLAGS"
				CFLAGS="$CFLAGS ${flag}"
				AC_COMPILE_IFELSE([AC_LANG_PROGRAM()],
					[AC_MSG_RESULT([yes])],
					[AC_MSG_RESULT([no])
					 CFLAGS="$ac_save_CFLAGS"
					]
				)
				;;
		esac
	done
fi

CXXFLAGS="$CFLAGS"

# Now add C-specific flags
if test "x$GCC" = "xyes"; then
	# see about adding some extra checks if the compiler takes them
	for flag in -Wdeclaration-after-statement ; do
		case " ${CFLAGS} " in
			*\ ${flag}\ *)
				# flag is already present
				;;
			*)
				AC_MSG_CHECKING([if the compiler accepts ${flag}])
				ac_save_CFLAGS="$CFLAGS"
				CFLAGS="$CFLAGS ${flag}"
				AC_COMPILE_IFELSE([AC_LANG_PROGRAM()],
					[AC_MSG_RESULT([yes])],
					[AC_MSG_RESULT([no])
					 CFLAGS="$ac_save_CFLAGS"
					]
				)
				;;
		esac
	done
fi

# See if we are building gcc with C++.
AC_ARG_ENABLE(build-with-cxx,
[  --enable-build-with-cxx build with C++ compiler instead of C compiler],
ENABLE_BUILD_WITH_CXX=$enableval,
ENABLE_BUILD_WITH_CXX=no)

case "$ENABLE_BUILD_WITH_CXX" in
  yes)
    CC_OR_CXX="$CXX"
    ;;
  no)
    CC_OR_CXX="$CC"
    ;;
esac
AC_SUBST(CC_OR_CXX)

# font filename
FONTFILENAME=${FONTFILENAME:-"default_font"}
AC_SUBST(FONTFILENAME)
AC_DEFINE_UNQUOTED(FONTFILENAME,"$FONTFILENAME",[File for default font])

# standard autoconf variables
CPPFLAGS="$CPPFLAGS -DPREFIXDIR=\\\"\${prefix}\\\""
CPPFLAGS="$CPPFLAGS -DBINDIR=\\\"\${bindir}\\\""
CPPFLAGS="$CPPFLAGS -DHOST=\\\"\${host}\\\""

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
PCBTREEPATH=${PCBTREEDIR}:${PCBLIBDIR}/pcblib-newlib
PCBTREEDIR=${PCBTREEDIR:-"$PCBTREEDIR"}
AC_SUBST(PCBTREEDIR)
AC_SUBST(PCBTREEPATH)
#AC_DEFINE_UNQUOTED(PCBTREEDIR,"$PCBLIB",[top directory for new style pcb library])
CPPFLAGS="$CPPFLAGS -DPCBTREEDIR=\\\"$PCBTREEDIR\\\""
CPPFLAGS="$CPPFLAGS -DPCBTREEPATH=\\\"$PCBTREEPATH\\\""

# directory for scad model library
SCADTREEDIR=${datadir}/pcb/models/scad
SCADTREEDIR=${SCADTREEDIR:-"$SCADTREEDIR"}
AC_SUBST(SCADTREEDIR)
#CPPFLAGS="$CPPFLAGS -DSCADTREEDIR=\\\"$SCADTREEDIR\\\""

# Figure out relative paths
AC_MSG_CHECKING([for the bindir to pcblibdir relative path])
adl_COMPUTE_RELATIVE_PATHS([bindir:PCBLIBDIR:bindir_to_pcblibdir])
adl_NORMALIZE_PATH([bindir_to_pcblibdir], [$PCB_DIR_SEPARATOR_S])
AC_MSG_RESULT([$bindir_to_pcblibdir])
AC_DEFINE_UNQUOTED(BINDIR_TO_PCBLIBDIR, "$bindir_to_pcblibdir", [Relative path from bindir to pcblibdir])

AC_MSG_CHECKING([for the bindir to pcbtreedir relative path])
adl_COMPUTE_RELATIVE_PATHS([bindir:PCBTREEDIR:bindir_to_pcbtreedir])
adl_NORMALIZE_PATH([bindir_to_pcbtreedir], [$PCB_DIR_SEPARATOR_S])
AC_MSG_RESULT([$bindir_to_pcbtreedir])
AC_DEFINE_UNQUOTED(BINDIR_TO_PCBTREEDIR, "$bindir_to_pcbtreedir", [Relative path from bindir to pcbtreedir])

AC_MSG_CHECKING([for the bindir to scadtreedir relative path])
adl_COMPUTE_RELATIVE_PATHS([bindir:SCADTREEDIR:bindir_to_scadtreedir])
adl_NORMALIZE_PATH([bindir_to_scadtreedir], [$PCB_DIR_SEPARATOR_S])
AC_MSG_RESULT([$bindir_to_scadtreedir])
AC_DEFINE_UNQUOTED(BINDIR_TO_SCADTREEDIR, "$bindir_to_scadtreedir", [Relative path from bindir to scadtreedir])

AC_MSG_CHECKING([for the bindir to exec_prefix relative path])
adl_COMPUTE_RELATIVE_PATHS([bindir:exec_prefix:bindir_to_execprefix])
adl_NORMALIZE_PATH([bindir_to_execprefix], [$PCB_DIR_SEPARATOR_S])
AC_MSG_RESULT([$bindir_to_execprefix])
AC_DEFINE_UNQUOTED(BINDIR_TO_EXECPREFIX, "$bindir_to_execprefix", [Relative path from bindir to exec_prefix])


BTNMOD=${BTNMOD:-"Mod1"}
AC_SUBST(BTNMOD)

TOPDIRS=
for dir in src lib newlib doc example tools tutorial README_FILES models-scad
do
   test -d $dir/. && TOPDIRS="$TOPDIRS $dir"
done
AC_SUBST(TOPDIRS)

AC_CONFIG_FILES(Makefile data/Makefile intl/Makefile po/Makefile.in)

if test -d $srcdir/README_FILES; then
   AC_CONFIG_FILES(README_FILES/Makefile)
fi
if test -d $srcdir/doc; then
   AC_CONFIG_FILES(doc/Makefile)
fi
if test -d $srcdir/doc/gs; then
   AC_CONFIG_FILES(doc/gs/Makefile)
   AC_CONFIG_FILES(doc/gs/gafrc)
   AC_CONFIG_FILES(doc/gs/gschemrc)
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
   AC_CONFIG_FILES(newlib/connectors/Makefile)
   AC_CONFIG_FILES(newlib/crystal/Makefile)
   AC_CONFIG_FILES(newlib/electro-optics/Makefile)
   AC_CONFIG_FILES(newlib/headers/Makefile)
   AC_CONFIG_FILES(newlib/keystone/Makefile)
   AC_CONFIG_FILES(newlib/msp430/Makefile)
   AC_CONFIG_FILES(newlib/not_vetted_ingo/Makefile)
   AC_CONFIG_FILES(newlib/sockets/Makefile)
   AC_CONFIG_FILES(newlib/tests/Makefile)
fi
if test -d $srcdir/models-scad; then
  AC_CONFIG_FILES(models-scad/Makefile)
  AC_CONFIG_FILES(models-scad/proto/Makefile)
fi
AC_CONFIG_FILES(src/Makefile)
AC_CONFIG_FILES(src/icons/Makefile)
if test -d $srcdir/tools; then
   AC_CONFIG_FILES(tools/Makefile)
fi
if test -d $srcdir/tutorial; then
   AC_CONFIG_FILES(tutorial/Makefile)
fi

dnl testsuite
AC_CONFIG_FILES(tests/Makefile)

dnl win32 build scripts
AC_CONFIG_FILES(w32/Makefile)

AC_OUTPUT

with_gui=`echo $with_gui`
with_printer=`echo $with_printer`
with_exporters=`echo $with_exporters | sed 's/,/ /g'`

expandedXDGDATADIR=`eval "echo $XDGDATADIR"`
expandedKDEDATADIR=`eval "echo $KDEDATADIR"`

AC_MSG_RESULT([
** Configuration summary for $PACKAGE $VERSION:

   Cross Compiling:          $cross_compiling
   CC:                       $CC
   CXX:                      $CXX
   CPPFLAGS:                 $CPPFLAGS
   CFLAGS:                   $CFLAGS
   CXXFLAGS:                 $CXXFLAGS
   LIBS:                     $LIBS
   PCB:                      $PCB

   GUI:                      $with_gui
   Printer:                  $with_printer
   Exporters:                $with_exporters
   Coordinate type:          $COORD_TYPE
   Source tree distribution: $pcb_sources
   Build documentation:      $docs_yesno
   Build toporouter:         $enable_toporouter
   Enable toporouter output: $enable_toporouter_output
   xdg data directory:       $expandedXDGDATADIR
   KDE data directory:       $expandedKDEDATADIR
   dmalloc debugging:        $with_dmalloc
   ElectricFence debugging:  $with_efence
   Regression tests enabled: $have_test_tools
])
