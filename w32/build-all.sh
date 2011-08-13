NEVERBUILD="
"

NOBUILD="
"

BUILD="
jpeg
zlib
libpng
tiff
pixman
libiconv
gettext
glib
atk
cairo
pango
gtk+
gd
pcb
"

fail()
{
  echo
  echo "=================="
  echo "Build failed."
  echo "=================="
  exit 1
}

succeed()
{
  echo
  echo "====================="
  echo "Build succeeded."
  echo "====================="
}

for D in $BUILD; do
  ./mpk source $D || fail
done

for D in $BUILD; do
  ./mpk build $D || fail
done

succeed

