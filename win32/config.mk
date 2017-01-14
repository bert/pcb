
# directory for download files
DISTDIR?= ${CURDIR}/distfiles

# directory for library runtime files
RUNTIMEDIR?=	${CURDIR}/gtk_win32_runtime

# directory for library dev files
DEVTIMEDIR?=	${CURDIR}/gtk_win32_devel

# unpacked sources
LIBSRCDIR?=	${CURDIR}/gtk_win32_srcs

STAMPDIR?=	${CURDIR}/gtk_win32_stamps

# some helper scripts
TOOLSDIR?=	${CURDIR}/tools

# some common download sites
MASTER_SITE_GNOME_SRC= https://download.gnome.org/sources
MASTER_SITE_GNOME_W32= https://download.gnome.org/binaries/win32
MASTER_SITE_SOURCEFORGE= http://downloads.sourceforge.net/project

# tools
AWK?=		awk
ECHO?= 		echo
HASHTYPE?=	sha1
OPENSSL?=	openssl
SED?=		sed
WGET?= 		wget --no-check-certificate
#WGET?= 		curl
