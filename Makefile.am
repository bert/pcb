## -*- makefile -*-
##
## Top level automake file for PCB

DIRS=		w32 win32 intl gts src data lib newlib example tools tutorial README_FILES po tests
SUBDIRS=	${DIRS} @DOC@
DIST_SUBDIRS=	${DIRS} doc

INTLTOOL_FILES = intltool-extract.in intltool-merge.in  intltool-update.in

EXTRA_DIST=	config.rpath globalconst.h README.git README.w32 README.win32 icon-theme-installer $(INTLTOOL_FILES)

DISTCLEANFILES= configure.lineno intltool-extract intltool-merge intltool-update po/.intltool-merge-cache src/gpcb-menu.res.tmp src/pcb-menu.res.tmp

MAINTAINERCLEANFILES= $(INTLTOOL_FILES)

ACLOCAL_AMFLAGS = -I m4
DISTCHECK_CONFIGURE_FLAGS := ${DISTCHECK_CONFIGURE_FLAGS} --disable-update-mime-database --disable-update-desktop-database GTK_UPDATE_ICON_THEME_BIN=true --with-gui=batch

