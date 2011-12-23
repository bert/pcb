dnl the FC_* macros were copied from the freeciv program.  The use here
dnl is to figure out if we need -DNARROWPROTO and the correct setting
dnl for FUNCPROTO.  Without these set right, it has been observed that
dnl the sliders don't work right on some systems.


dnl
dnl FC_CHECK_X_PROTO_DEFINE(DEFINED-VARIABLE)
dnl
dnl This macro determines the value of the given defined
dnl variable needed by Xfuncproto.h in order to compile correctly.
dnl
dnl Typical DEFINED-VARIABLEs are:
dnl   FUNCPROTO
dnl   NARROWPROTO
dnl
dnl The following variables are output:
dnl   fc_x_proto_value		-- contains the value to which
dnl				the DEFINED-VARIABLE is set,
dnl				or "" if it has no known value.
dnl
dnl Example use:
dnl   FC_CHECK_X_PROTO_DEFINE(FUNCPROTO)
dnl   if test -n "$fc_x_proto_value"; then
dnl     AC_DEFINE_UNQUOTED(FUNCPROTO, $fc_x_proto_value)
dnl   fi
dnl
AC_DEFUN([FC_CHECK_X_PROTO_DEFINE],
[AC_REQUIRE([FC_CHECK_X_PROTO_FETCH])dnl
AC_MSG_CHECKING(for Xfuncproto control definition $1)
# Search for the requested defined variable; return it's value:
fc_x_proto_value=
for fc_x_define in $fc_x_proto_defines; do
  fc_x_val=1
  eval `echo $fc_x_define | sed -e 's/=/ ; fc_x_val=/' | sed -e 's/^/fc_x_var=/'`
  if test "x$fc_x_var" = "x$1"; then
    fc_x_proto_value=$fc_x_val
    break
  fi
done
if test -n "$fc_x_proto_value"; then
  AC_MSG_RESULT([yes: $fc_x_proto_value])
else
  AC_MSG_RESULT([no])
fi
])

dnl FC_CHECK_X_PROTO_FETCH
dnl
dnl This macro fetches the Xfuncproto control definitions.
dnl (Intended to be called once from FC_CHECK_X_PROTO_DEFINE.)
dnl
dnl The following variables are output:
dnl   fc_x_proto_defines	-- contains the list of defines of
dnl				Xfuncproto control definitions
dnl				(defines may or may not include
dnl				the -D prefix, or an =VAL part).
dnl
dnl Example use:
dnl   AC_REQUIRE([FC_CHECK_X_PROTO_FETCH])
dnl
AC_DEFUN([FC_CHECK_X_PROTO_FETCH],
[AC_REQUIRE([AC_PATH_X])dnl
AC_MSG_CHECKING(whether Xfuncproto was supplied)
dnl May override determined defines with explicit argument:
AC_ARG_WITH(x-funcproto,
    [  --with-x-funcproto=DEFS Xfuncproto control definitions are DEFS
                          (e.g.: --with-x-funcproto='FUNCPROTO=15 NARROWPROTO']dnl
)
if test "x$with_x_funcproto" = "x"; then
  fc_x_proto_defines=
  rm -fr conftestdir
  if mkdir conftestdir; then
    cd conftestdir
    # Make sure to not put "make" in the Imakefile rules, since we grep it out.
    cat > Imakefile <<'EOF'
fcfindpd:
	@echo 'fc_x_proto_defines=" ${PROTO_DEFINES}"'
EOF
    if (xmkmf) >/dev/null 2>/dev/null && test -f Makefile; then
      # GNU make sometimes prints "make[1]: Entering...", which would confuse us.
      eval `${MAKE-make} fcfindpd 2>/dev/null | grep -v make | sed -e 's/ -D/ /g'`
      AC_MSG_RESULT([no, found: $fc_x_proto_defines])
      cd ..
      rm -fr conftestdir
    else
      dnl Oops -- no/bad xmkmf... Time to go a-guessing...
      AC_MSG_RESULT([no])
      cd ..
      rm -fr conftestdir
      dnl First, guess something for FUNCPROTO:
      AC_MSG_CHECKING([for compilable FUNCPROTO definition])
      dnl Try in order of preference...
      for fc_x_value in 15 11 3 1 ""; do
	FC_CHECK_X_PROTO_FUNCPROTO_COMPILE($fc_x_value)
	if test "x$fc_x_proto_FUNCPROTO" != "xno"; then
	  break
	fi
      done
      if test "x$fc_x_proto_FUNCPROTO" != "xno"; then
	fc_x_proto_defines="$fc_x_proto_defines FUNCPROTO=$fc_x_proto_FUNCPROTO"
	AC_MSG_RESULT([yes, determined: $fc_x_proto_FUNCPROTO])
      else
	AC_MSG_RESULT([no, cannot determine])
      fi
      dnl Second, guess something for NARROWPROTO:
      AC_MSG_CHECKING([for workable NARROWPROTO definition])
      dnl Try in order of preference...
      for fc_x_value in 1 ""; do
	FC_CHECK_X_PROTO_NARROWPROTO_WORKS($fc_x_value)
	if test "x$fc_x_proto_NARROWPROTO" != "xno"; then
	  break
	fi
      done
      if test "x$fc_x_proto_NARROWPROTO" != "xno"; then
	fc_x_proto_defines="$fc_x_proto_defines NARROWPROTO=$fc_x_proto_NARROWPROTO"
	AC_MSG_RESULT([yes, determined: $fc_x_proto_NARROWPROTO])
      else
	AC_MSG_RESULT([no, cannot determine])
      fi
      AC_MSG_CHECKING(whether Xfuncproto was determined)
      if test -n "$fc_x_proto_defines"; then
	AC_MSG_RESULT([yes: $fc_x_proto_defines])
      else
	AC_MSG_RESULT([no])
      fi
    fi
  else
    AC_MSG_RESULT([no, examination failed])
  fi
else
  fc_x_proto_defines=$with_x_funcproto
  AC_MSG_RESULT([yes, given: $fc_x_proto_defines])
fi
])

dnl FC_CHECK_X_PROTO_FUNCPROTO_COMPILE(FUNCPROTO-VALUE)
dnl
dnl This macro determines whether or not Xfuncproto.h will
dnl compile given a value to use for the FUNCPROTO definition.
dnl
dnl Typical FUNCPROTO-VALUEs are:
dnl   15, 11, 3, 1, ""
dnl
dnl The following variables are output:
dnl   fc_x_proto_FUNCPROTO	-- contains the passed-in
dnl				FUNCPROTO-VALUE if Xfuncproto.h
dnl				compiled, or "no" if it did not.
dnl
dnl Example use:
dnl   FC_CHECK_X_PROTO_FUNCPROTO_COMPILE($fc_x_value)
dnl   if test "x$fc_x_proto_FUNCPROTO" != "xno"; then
dnl     echo Compile using FUNCPROTO=$fc_x_proto_FUNCPROTO
dnl   fi
dnl
AC_DEFUN([FC_CHECK_X_PROTO_FUNCPROTO_COMPILE],
[AC_REQUIRE([AC_PATH_XTRA])dnl
AC_LANG_SAVE
AC_LANG_C
fc_x_proto_FUNCPROTO=no
if test "x$1" = "x"; then
  fc_x_compile="#undef FUNCPROTO"
else
  fc_x_compile="#define FUNCPROTO $1"
fi
fc_x_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $X_CFLAGS"
AC_TRY_COMPILE([
$fc_x_compile
#include <X11/Xfuncproto.h>
  ],[
exit (0)
  ],
  [fc_x_proto_FUNCPROTO=$1])
CFLAGS="$fc_x_save_CFLAGS"
AC_LANG_RESTORE
])

dnl FC_CHECK_X_PROTO_NARROWPROTO_WORKS(NARROWPROTO-VALUE)
dnl
dnl This macro determines whether or not NARROWPROTO is required
dnl to get a typical X function (XawScrollbarSetThumb) to work.
dnl
dnl Typical NARROWPROTO-VALUEs are:
dnl   1, ""
dnl
dnl The following variables are required for input:
dnl   fc_x_proto_FUNCPROTO	-- the value to use for FUNCPROTO.
dnl
dnl The following variables are output:
dnl   fc_x_proto_NARROWPROTO	-- contains the passed-in
dnl				NARROWPROTO-VALUE if the test
dnl				worked, or "no" if it did not.
dnl
dnl Example use:
dnl   FC_CHECK_X_PROTO_NARROWPROTO_WORKS($fc_x_value)
dnl   if test "x$fc_x_proto_NARROWPROTO" != "xno"; then
dnl     echo Compile using NARROWPROTO=$fc_x_proto_NARROWPROTO
dnl   fi
dnl
AC_DEFUN([FC_CHECK_X_PROTO_NARROWPROTO_WORKS],
[AC_REQUIRE([AC_PATH_XTRA])dnl
AC_LANG_SAVE
AC_LANG_C
fc_x_proto_NARROWPROTO=no
if test "x$1" = "x"; then
  fc_x_works="#undef NARROWPROTO"
else
  fc_x_works="#define NARROWPROTO $1"
fi
if test "x$fc_x_proto_FUNCPROTO" = "x"; then
  fc_x_compile="#define FUNCPROTO 1"
else
  fc_x_compile="#define FUNCPROTO $fc_x_proto_FUNCPROTO"
fi
fc_x_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $X_CFLAGS $X_LIBS $X_PRE_LIBS -l$LIBXAW -lXt -lX11 $X_EXTRA_LIBS"
AC_TRY_RUN([
$fc_x_works
$fc_x_compile
#include <X11/Xfuncproto.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/$XAWINC/Scrollbar.h>
#define TOP_VAL 0.125
#define SHOWN_VAL 0.25
int main (int argc, char ** argv)
{
  Widget toplevel;
  XtAppContext appcon;
  Widget scrollbar;
  double topbuf;
  double shownbuf;
  float * top = (float *)(&topbuf);
  float * shown = (float *)(&shownbuf);
  toplevel =
    XtAppInitialize
    (
     &appcon,
     "FcXTest",
     NULL, 0,
     &argc, argv,
     NULL,
     NULL, 0
    );
  scrollbar =
    XtVaCreateManagedWidget
    (
     "my_scrollbar",
     scrollbarWidgetClass,
     toplevel,
     NULL
    );
  XawScrollbarSetThumb (scrollbar, TOP_VAL, SHOWN_VAL);
  XtVaGetValues
  (
   scrollbar,
   XtNtopOfThumb, top,
   XtNshown, shown,
   NULL
  );
  if ((*top == TOP_VAL) && (*shown == SHOWN_VAL))
    {
      exit (0);
    }
  else
    {
      exit (1);
    }
  return (0);
}
  ],
  [fc_x_proto_NARROWPROTO=$1])
CFLAGS="$fc_x_save_CFLAGS"
AC_LANG_RESTORE
])


dnl ----------------------------------------------------------------
dnl From gcc's libiberty aclocal.m4

dnl See whether we need a declaration for a function.
AC_DEFUN([libiberty_NEED_DECLARATION],
[AC_MSG_CHECKING([whether $1 must be declared])
AC_CACHE_VAL(libiberty_cv_decl_needed_$1,
[AC_TRY_COMPILE([
#include "confdefs.h"
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif],
[char *(*pfn) = (char *(*)) $1],
libiberty_cv_decl_needed_$1=no, libiberty_cv_decl_needed_$1=yes)])
AC_MSG_RESULT($libiberty_cv_decl_needed_$1)
if test $libiberty_cv_decl_needed_$1 = yes; then
	AC_DEFINE([NEED_DECLARATION_]translit($1, [a-z], [A-Z]), 1,
	[Define if $1 is not declared in system header files.])
fi
])dnl


dnl ----------------------------------------------------------------
dnl From http://autoconf-archive.cryp.to/adl_compute_relative_paths.html
dnl and http://autoconf-archive.cryp.to/adl_normalize_path.html
dnl
dnl the adl_* functions which follow have the following copyright:
dnl
dnl Copyright © 2006 Alexandre Duret-Lutz <adl@gnu.org>

dnl This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

dnl This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

dnl You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

dnl As a special exception, the respective Autoconf Macro's copyright owner gives unlimited permission to copy, distribute and modify the configure scripts that are the output of Autoconf when processing the Macro. You need not follow the terms of the GNU General Public License when using or distributing such scripts, even though portions of the text of the Macro appear in them. The GNU General Public License (GPL) does govern all other use of the material that constitutes the Autoconf Macro.

dnl This special exception to the GPL applies to versions of the Autoconf Macro released by the Autoconf Macro Archive. When you make and distribute a modified version of the Autoconf Macro, you may extend this special exception to the GPL to apply to your modified version as well.

AC_DEFUN([adl_NORMALIZE_PATH],
[case ":[$]$1:" in
# change empty paths to '.'
  ::) $1='.' ;;
# strip trailing slashes
  :*[[\\/]]:) $1=`echo "[$]$1" | sed 's,[[\\/]]*[$],,'` ;;
  :*:) ;;
esac
# squeze repeated slashes
case ifelse($2,,"[$]$1",$2) in
# if the path contains any backslashes, turn slashes into backslashes
 *\\*) $1=`echo "[$]$1" | sed 's,\(.\)[[\\/]][[\\/]]*,\1\\\\\\\\,g'` ;;
# if the path contains slashes, also turn backslashes into slashes
 *) $1=`echo "[$]$1" | sed 's,\(.\)[[\\/]][[\\/]]*,\1/,g'` ;;
esac])


AC_DEFUN([adl_COMPUTE_RELATIVE_PATHS],
[for _lcl_i in $1; do
  _lcl_from=\[$]`echo "[$]_lcl_i" | sed 's,:.*$,,'`
  _lcl_to=\[$]`echo "[$]_lcl_i" | sed 's,^[[^:]]*:,,' | sed 's,:[[^:]]*$,,'`
  _lcl_result_var=`echo "[$]_lcl_i" | sed 's,^.*:,,'`
  adl_RECURSIVE_EVAL([[$]_lcl_from], [_lcl_from])
  adl_RECURSIVE_EVAL([[$]_lcl_to], [_lcl_to])
  _lcl_notation="$_lcl_from$_lcl_to"
  adl_NORMALIZE_PATH([_lcl_from],['/'])
  adl_NORMALIZE_PATH([_lcl_to],['/'])
  adl_COMPUTE_RELATIVE_PATH([_lcl_from], [_lcl_to], [_lcl_result_tmp])
  adl_NORMALIZE_PATH([_lcl_result_tmp],["[$]_lcl_notation"])
  eval $_lcl_result_var='[$]_lcl_result_tmp'
done])

## Note:
## *****
## The following helper macros are too fragile to be used out
## of adl_COMPUTE_RELATIVE_PATHS (mainly because they assume that
## paths are normalized), that's why I'm keeping them in the same file.
## Still, some of them maybe worth to reuse.

dnl adl_COMPUTE_RELATIVE_PATH(FROM, TO, RESULT)
dnl ===========================================
dnl Compute the relative path to go from $FROM to $TO and set the value
dnl of $RESULT to that value.  This function work on raw filenames
dnl (for instead it will considerate /usr//local and /usr/local as
dnl two distinct paths), you should really use adl_COMPUTE_REALTIVE_PATHS
dnl instead to have the paths sanitized automatically.
dnl
dnl For instance:
dnl    first_dir=/somewhere/on/my/disk/bin
dnl    second_dir=/somewhere/on/another/disk/share
dnl    adl_COMPUTE_RELATIVE_PATH(first_dir, second_dir, first_to_second)
dnl will set $first_to_second to '../../../another/disk/share'.
AC_DEFUN([adl_COMPUTE_RELATIVE_PATH],
[adl_COMPUTE_COMMON_PATH([$1], [$2], [_lcl_common_prefix])
adl_COMPUTE_BACK_PATH([$1], [_lcl_common_prefix], [_lcl_first_rel])
adl_COMPUTE_SUFFIX_PATH([$2], [_lcl_common_prefix], [_lcl_second_suffix])
$3="[$]_lcl_first_rel[$]_lcl_second_suffix"])


dnl adl_COMPUTE_COMMON_PATH(LEFT, RIGHT, RESULT)
dnl ============================================
dnl Compute the common path to $LEFT and $RIGHT and set the result to $RESULT.
dnl
dnl For instance:
dnl    first_path=/somewhere/on/my/disk/bin
dnl    second_path=/somewhere/on/another/disk/share
dnl    adl_COMPUTE_COMMON_PATH(first_path, second_path, common_path)
dnl will set $common_path to '/somewhere/on'.
AC_DEFUN([adl_COMPUTE_COMMON_PATH],
[$3=''
_lcl_second_prefix_match=''
while test "[$]_lcl_second_prefix_match" != 0; do
  _lcl_first_prefix=`expr "x[$]$1" : "x\([$]$3/*[[^/]]*\)"`
  _lcl_second_prefix_match=`expr "x[$]$2" : "x[$]_lcl_first_prefix"`
  if test "[$]_lcl_second_prefix_match" != 0; then
    if test "[$]_lcl_first_prefix" != "[$]$3"; then
      $3="[$]_lcl_first_prefix"
    else
      _lcl_second_prefix_match=0
    fi
  fi
done])

dnl adl_COMPUTE_SUFFIX_PATH(PATH, SUBPATH, RESULT)
dnl ==============================================
dnl Substrack $SUBPATH from $PATH, and set the resulting suffix
dnl (or the empty string if $SUBPATH is not a subpath of $PATH)
dnl to $RESULT.
dnl
dnl For instace:
dnl    first_path=/somewhere/on/my/disk/bin
dnl    second_path=/somewhere/on
dnl    adl_COMPUTE_SUFFIX_PATH(first_path, second_path, common_path)
dnl will set $common_path to '/my/disk/bin'.
AC_DEFUN([adl_COMPUTE_SUFFIX_PATH],
[$3=`expr "x[$]$1" : "x[$]$2/*\(.*\)"`])

dnl adl_COMPUTE_BACK_PATH(PATH, SUBPATH, RESULT)
dnl ============================================
dnl Compute the relative path to go from $PATH to $SUBPATH, knowing that
dnl $SUBPATH is a subpath of $PATH (any other words, only repeated '../'
dnl should be needed to move from $PATH to $SUBPATH) and set the value
dnl of $RESULT to that value.  If $SUBPATH is not a subpath of PATH,
dnl set $RESULT to the empty string.
dnl
dnl For instance:
dnl    first_path=/somewhere/on/my/disk/bin
dnl    second_path=/somewhere/on
dnl    adl_COMPUTE_BACK_PATH(first_path, second_path, back_path)
dnl will set $back_path to '../../../'.
AC_DEFUN([adl_COMPUTE_BACK_PATH],
[adl_COMPUTE_SUFFIX_PATH([$1], [$2], [_lcl_first_suffix])
$3=''
_lcl_tmp='xxx'
while test "[$]_lcl_tmp" != ''; do
  _lcl_tmp=`expr "x[$]_lcl_first_suffix" : "x[[^/]]*/*\(.*\)"`
  if test "[$]_lcl_first_suffix" != ''; then
     _lcl_first_suffix="[$]_lcl_tmp"
     $3="../[$]$3"
  fi
done])


dnl adl_RECURSIVE_EVAL(VALUE, RESULT)
dnl =================================
dnl Interpolate the VALUE in loop until it doesn't change,
dnl and set the result to $RESULT.
dnl WARNING: It's easy to get an infinite loop with some unsane input.
AC_DEFUN([adl_RECURSIVE_EVAL],
[_lcl_receval="$1"
$2=`(test "x$prefix" = xNONE && prefix="$ac_default_prefix"
     test "x$exec_prefix" = xNONE && exec_prefix="${prefix}"
     _lcl_receval_old=''
     while test "[$]_lcl_receval_old" != "[$]_lcl_receval"; do
       _lcl_receval_old="[$]_lcl_receval"
       eval _lcl_receval="\"[$]_lcl_receval\""
     done
     echo "[$]_lcl_receval")`])


