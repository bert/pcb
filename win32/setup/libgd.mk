NAME= libgd
VER=  2.2.3
WVER= 1
LICENSE=	libgd
SRC_DISTURLS+=	https://github.com/libgd/libgd/releases/download/gd-2.2.3/libgd-2.2.3.tar.gz
CONFIGURE_ARGS+= --build=i686-pc-cygwin --host=mingw32 --prefix=c:\\cygwin\\home\\dan\\gd_win32
BUILD=		yes
