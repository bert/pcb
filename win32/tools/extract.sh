#!/bin/sh
#

dist="$1"

BZIP2=${BZIP2:-bzip2}
TAR=${TAR:-tar}
UNZIP=${UNZIP:-unzip}
XZ=${XZ:-xz}

echo "Extracting $dist in $PWD"

case "$dist" in
	*.zip)		${UNZIP} -f "$dist" ;;
	*.tar.xz)	${XZ} --decompress --stdout "$dist" | ${TAR} -xpf - ;;
	*.tar.gz)	${TAR} -zxpf "$dist" ;;
	*.tgz)		${TAR} -zxpf "$dist" ;;
	*.tar.bz2)	${BZIP2} --decompress --stdout "$dist" | ${TAR} -xpf - ;;
	*)		echo "$0:  I do not know how to extract $dist" ; exit 1 ;;
esac
