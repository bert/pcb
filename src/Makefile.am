## $Id$
##

SUBDIRS=	icons

pcbtreedir=	@PCBTREEDIR@
pcblibdir=	@PCBLIBDIR@

pcblib_DATA=	\
	default_font

pkgdata_DATA=	Pcb

bin_PROGRAMS=	pcb-bin
bin_SCRIPTS=	pcb

pcb_bin_SOURCES = \
	action.c \
	action.h \
	autoplace.c \
	autoplace.h \
	autoroute.c \
	autoroute.h \
	box.h \
	buffer.c \
	buffer.h \
	change.c \
	change.h \
	command.c \
	command.h \
	const.h \
	control.c \
	control.h \
	copy.c \
	copy.h \
	create.c \
	create.h \
	crosshair.c \
	crosshair.h \
	data.c \
	data.h \
	dev_ps.c \
	dev_ps.h \
	dev_rs274x.c \
	dev_rs274x.h \
	dialog.c \
	dialog.h \
	djopt.c \
	djopt.h \
	draw.c \
	draw.h \
	drill.c \
	drill.h \
	error.c \
	error.h \
	file.c \
	file.h \
	fileselect.c \
	fileselect.h \
	find.c \
	find.h \
	global.h \
	gui.c \
	gui.h \
	heap.c \
	heap.h \
	insert.c \
	insert.h \
	intersect.c \
	intersect.h \
	kdtree.c \
	kdtree.h \
	lgdialog.c \
	lgdialog.h \
	library.c \
	library.h \
	log.c \
	log.h \
	macro.h \
	main.c \
	menu.c \
	menu.h \
	mirror.c \
	mirror.h \
	misc.c \
	misc.h \
	move.c \
	move.h \
	mtspace.c \
	mtspace.h \
	mymem.c \
	mymem.h \
	netlist.c \
	netlist.h \
	output.c \
	output.h \
	parse_l.h \
	parse_l.l \
	parse_y.h \
	parse_y.y \
	pinout.c \
	pinout.h \
	polygon.c \
	polygon.h \
	print.c \
	print.h \
	printdialog.c \
	printdialog.h \
	printpanner.c \
	printpanner.h \
	rats.c \
	rats.h \
	remove.c \
	remove.h \
	report.c \
	report.h \
	rotate.c \
	rotate.h \
	rubberband.c \
	rubberband.h \
	search.c \
	search.h \
	select.c \
	select.h \
	selector.c \
	selector.h \
	set.c \
	set.h \
	sizedialog.c \
	sizedialog.h \
	undo.c \
	undo.h \
	vector.c \
	vector.h 

# for globalconst.h
INCLUDES=	-I$(top_srcdir) -I$(srcdir)/icons

BUILT_SOURCES=	parse_y.h

EXTRA_DIST= \
	check_icon.data \
	default_font \
	icon.data \
	mode_icon.data \
	Pcb.ad.in  \
	pcb.in \
	pcbtest.sh.in

AM_YFLAGS=	-d

all-local: pcbtest.sh .test/Pcb

DISTCLEANFILES= pcbtest.sh pcb Pcb .test/Pcb

# create wrapper script that lets you test pcb prior to installation
pcbtest.sh:	$(srcdir)/pcbtest.sh.in Makefile
	sed -e "s;@BUILDDIR@;`pwd`;g" -e "s;@TOPSRCDIR@;${top_srcdir};g" \
		$(srcdir)/pcbtest.sh.in > $@
	chmod 755 $@

# create wrapper script for running pcb after installation
pcb:	$(srcdir)/pcb.in Makefile
	sed -e 's;@BINDIR@;${bindir};g' -e 's;@PKGDATADIR@;${pkgdatadir};g' \
		$(srcdir)/pcb.in > $@
	chmod 755 $@

# we create Pcb.ad here instead of directly with autoconf because we need
# recursive evaluation of variables.  Ie, if we directly use autoconf, we'll
# end up with ${prefix}/share/.... instead of the fully evaluated version in
# the app-default file
FONTFILENAME=		@FONTFILENAME@
LIBRARYFILENAME=	@LIBRARYFILENAME@
BTNMOD=			@BTNMOD@
Pcb:	$(srcdir)/Pcb.ad.in Makefile
	sed \
		-e 's;@gnum4@;${M4};g' \
		-e 's;@pcblibdir@;${pcblibdir};g' \
		-e 's;@pcbtreedir@;${pcbtreedir};g' \
		-e 's;@fontfilename@;${FONTFILENAME};g' \
		-e 's;@libraryfilename@;${LIBRARYFILENAME};g' \
		-e 's;@btnmod@;${BTNMOD};g' \
		$(srcdir)/Pcb.ad.in > $@

# Note:  We reference QueryLibrary.sh and the libraries via $(top_srcdir) 
# since they are not built.  For QueryLibrary.sh, we use a relative path
# since it is a built file
.test/Pcb: Pcb
	if [ ! -d .test ]; then mkdir -p .test ; fi
	sed	-e 's;^\(Pcb.elementPath\).*$$;\1: .:$(top_srcdir)/newlib:packages:circuits:$(top_srcdir)/lib:$(top_srcdir)/lib/circuits:$(top_srcdir)/lib/packages;' \
		-e 's;^\(Pcb\.filePath\).*$$;\1: .;' \
		-e 's;^\(Pcb.fontPath\).*$$;\1: .;' \
		-e 's;^\(Pcb.fontFile\).*$$;\1: $(srcdir)/default_font;' \
		-e "s;^\(Pcb.libraryCommand\).*$$;\1: `pwd`/../lib/QueryLibrary.sh \"%p\" \"%f\" %a;" \
		-e 's;^\(Pcb.libraryContentsCommand\).*$$;\1: $(top_srcdir)/lib/ListLibraryContents.sh "%p" "%f";' \
		-e "s;^\(Pcb.libraryPath\).*$$;\1: .:`pwd`/../lib;" \
		-e 's;^\(Pcb.libraryTree\).*$$;\1: $(top_srcdir)/newlib;' \
		-e 's;^\(Pcb.libraryFilename\).*$$;\1: ${LIBRARYFILENAME};' \
			Pcb > $@


