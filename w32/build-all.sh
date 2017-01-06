NEVERBUILD="
"

NOBUILD="
"

BUILD="
jpeg
zlib
libpng
tiff
freetype
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

MPK_VERBOSE=no
export MPK_VERBOSE

while test $# -gt 0 ; do
  case $1 in
    --verbose) MPK_VERBOSE=yes ; shift ;;
    -*) echo "Error:  $1 unknown"; exit 1 ;;
    *) break;;
  esac
done

for D in $BUILD; do
  ./mpk source $D || fail
done

for D in $BUILD; do
  ./mpk build $D || fail
done

succeed

