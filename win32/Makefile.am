## -*- Makefile -*-
##
## $Id$
##


EXTRA_DIST= build_pcb pcb.nsi.in Readme.txt \
	${TGIF_OBJS} ${XPMS} ${PPMS_16} ${PPMS_256} ${ICONS}

TGIF_OBJS=	pcb_icon_big.obj pcb_icon_med.obj pcb_icon_sml.obj
XPMS=		${TGIF_OBJS:.obj=.xpm}
PPMS_16=	${TGIF_OBJS:.obj=_16.ppm}
PPMS_256=	${TGIF_OBJS:.obj=_256.ppm}
ICONS=		pcb_icon.ico

TGIF_PROG=	XENVIRONMENT=$(top_srcdir)/pixmaps/Xdefaults.tgif @TGIF@ -print -color -xpm

pkgdatadir=     ${datadir}/${PACKAGE}-${VERSION}

if WIN32
pkgdata_DATA=	Readme.txt
endif


SUFFIXES=	.xpm .obj _16.ppm _256.ppm .ico

%.xpm : %.obj
if MISSING_TGIF
	@echo "Tgif is missing on your system so I will not rebuild $@"
	@echo "even though $< seems newer.  You do not need this unless"
	@echo "you are building for win32 anyway.  And even without it"
	@echo "it just means your icon modifications will not take."
else
	${TGIF_PROG} $<
endif

%_16.ppm : %.xpm
if MISSING_CONVERT
	@echo "convert (from ImageMagick) is missing on your system so I will not rebuild $@"
	@echo "even though $< seems newer.  You do not need this unless"
	@echo "you are building for win32 anyway.  And even without it"
	@echo "it just means your icon modifications will not take."
else
	${CONVERT} -colors 16 $< $@
endif

%_256.ppm : %.xpm
if MISSING_CONVERT
	@echo "convert (from ImageMagick) is missing on your system so I will not rebuild $@"
	@echo "even though $< seems newer.  You do not need this unless"
	@echo "you are building for win32 anyway.  And even without it"
	@echo "it just means your icon modifications will not take."
else
	${CONVERT} -colors 256 $< $@
endif

pcb_icon.ico : ${PPMS_16} ${PPMS_256}
if MISSING_XPMTOPPM
	@echo "xpmtoppm/ppmtowinicon are missing on your system so I will not rebuild $@"
	@echo "even though $< seems newer.  You do not need this unless"
	@echo "you are building for win32 anyway.  And even without it"
	@echo "it just means your icon modifications will not take."
else
	${PPMTOWINICON} -output $@ ${PPMS_16} ${PPMS_256}
endif

