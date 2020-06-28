## -*- makefile -*-
##
## Top level automake file for PCB

DIRS=		w32 win32 gts src data lib newlib example tools tutorial README_FILES tests po
SUBDIRS=	${DIRS} @DOC@
DIST_SUBDIRS=	${DIRS} doc

EXTRA_DIST=	config.rpath globalconst.h README.git README.w32 README.win32 icon-theme-installer

DISTCLEANFILES= configure.lineno src/gpcb-menu.res.tmp src/pcb-menu.res.tmp

MAINTAINERCLEANFILES= $(DISTCLEANFILES)

ACLOCAL_AMFLAGS = -I m4
DISTCHECK_CONFIGURE_FLAGS := ${DISTCHECK_CONFIGURE_FLAGS} --disable-update-mime-database --disable-update-desktop-database GTK_UPDATE_ICON_THEME_BIN=true --with-gui=batch

