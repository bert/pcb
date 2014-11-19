## -*- makefile -*-
##
## Top level automake file for PCB

DIRS=		w32 intl src data lib newlib example tools tutorial README_FILES po tests models-scad
SUBDIRS=	${DIRS} @DOC@
DIST_SUBDIRS=	${DIRS} doc

INTLTOOL_FILES = intltool-extract.in intltool-merge.in  intltool-update.in

EXTRA_DIST=	config.rpath globalconst.h README.git README.w32 icon-theme-installer $(INTLTOOL_FILES)

DISTCLEANFILES= configure.lineno intltool-extract intltool-merge intltool-update po/.intltool-merge-cache

MAINTAINERCLEANFILES= $(INTLTOOL_FILES)

ACLOCAL_AMFLAGS = -I m4
DISTCHECK_CONFIGURE_FLAGS := ${DISTCHECK_CONFIGURE_FLAGS} --disable-update-mime-database --disable-update-desktop-database GTK_UPDATE_ICON_THEME_BIN=true

