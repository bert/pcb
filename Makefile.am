## -*- makefile -*-
## $Id$
##
## Top level automake file for PCB

DIRS=		win32 src data lib newlib example tools tutorial README_FILES intl po
SUBDIRS=	${DIRS} @DOC@
DIST_SUBDIRS=	${DIRS} doc

INTLTOOL_FILES = intltool-extract.in intltool-merge.in  intltool-update.in

EXTRA_DIST=	config.rpath m4/ChangeLog autogen.sh globalconst.h README.cvs README.win32 icon-theme-installer $(INTLTOOL_FILES)

DISTCLEANFILES= configure.lineno intltool-extract intltool-merge intltool-update po/.intltool-merge-cache

MAINTAINERCLEANFILES= $(INTLTOOL_FILES)

ACLOCAL_AMFLAGS = -I m4
DISTCHECK_CONFIGURE_FLAGS = --disable-update-mime-database --disable-update-desktop-database

