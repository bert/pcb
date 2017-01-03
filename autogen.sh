#! /bin/sh
#
# Run the various GNU autotools to bootstrap the build
# system.  Should only need to be done once.

# for now avoid using bash as not everyone has that installed
CONFIG_SHELL=/bin/sh
export CONFIG_SHELL

############################################################################
#
# autopoint (gettext)
#

echo "Checking autopoint version..."
ver=`autopoint --version | awk '{print $NF; exit}'`
ap_maj=`echo $ver | sed 's;\..*;;g'`
ap_min=`echo $ver | sed -e 's;^[0-9]*\.;;g'  -e 's;\..*$;;g'`
ap_teeny=`echo $ver | sed -e 's;^[0-9]*\.[0-9]*\.;;g'`
echo "    $ver"

case $ap_maj in
	0)
		if test $ap_min -lt 14 ; then
			echo "You must have gettext >= 0.14.0 but you seem to have $ver"
			exit 1
		fi
		;;
esac
echo "Running autopoint..."
autopoint --force || exit 1

############################################################################
#
# intltoolize (intltool)
#

echo "Checking intltoolize version..."
it_ver=`intltoolize --version | awk '{print $NF; exit}'`
it_maj=`echo $it_ver | sed 's;\..*;;g'`
it_min=`echo $it_ver | sed -e 's;^[0-9]*\.;;g'  -e 's;\..*$;;g'`
it_teeny=`echo $it_ver | sed -e 's;^[0-9]*\.[0-9]*\.;;g'`
echo "    $it_ver"

case $it_maj in
	0)
		if test $it_min -lt 35 ; then
			echo "You must have intltool >= 0.35.0 but you seem to have $it_ver"
			exit 1
		fi
		;;
esac
echo "Running intltoolize..."
echo "no" | intltoolize --force --copy --automake || exit 1

echo "Patching some intltoolize output"

# both intltoolize and autopoint create a po/Makefile.in.in, this can't be good...
# but intltoolize seems to have some bugs in it.  In particular, XGETTEXT and MSGFMT
# are set in the Makefile but not passed down when calling MSGMERGE or GENPOT.
# This defeats specifying the path to xgettext and msgfmt.  Also
# we don't have a ChangeLog in the po/ directory right now so don't let
# intltool try to include it in the distfile.

mv po/Makefile.in.in po/Makefile.in.in.orig
sed \
	-e 's/^MSGMERGE *=/MSGMERGE = XGETTEXT="\${XGETTEXT}" MSGFMT="\${MSGFMT}" /g' \
	-e 's/^GENPOT *=/GENPOT = XGETTEXT="\${XGETTEXT}" MSGFMT="\${MSGFMT}" /g' \
	-e 's/ChangeLog//g' \
	po/Makefile.in.in.orig > po/Makefile.in.in

# Menu i18n
echo "
%.res.h: %.res
	\$(MAKE) -C ../src \$@" >> po/Makefile.in.in

rm -f po/Makefile.in.in.orig

############################################################################
#
# automake/autoconf stuff
#

echo "Running aclocal..."
aclocal -I m4 $ACLOCAL_FLAGS || exit 1
echo "Done with aclocal"

echo "Running autoheader..."
autoheader || exit 1
echo "Done with autoheader"

echo "Running automake..."
automake -a -c --foreign || exit 1
echo "Done with automake"

echo "Running autoconf..."
autoconf || exit 1
echo "Done with autoconf"

############################################################################
#
# finish up
#

echo "You must now run configure"

echo "All done with autogen.sh"

