#! /bin/sh
#
# $Id$
#
# Run the various GNU autotools to bootstrap the build
# system.  Should only need to be done once.

# for now avoid using bash as not everyone has that installed
CONFIG_SHELL=/bin/sh
export CONFIG_SHELL

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
			exit
		fi
		;;
esac
echo "Running autopoint..."
autopoint --force || exit 1

echo "Running intltoolize..."
echo "no" | intltoolize --force --copy --automake || exit 1

echo "Running aclocal..."
aclocal $ACLOCAL_FLAGS || exit 1
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

echo "You must now run configure"

echo "All done with autogen.sh"

