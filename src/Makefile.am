## $Id$
##

SUBDIRS=	icons script

pcbtreedir=	@PCBTREEDIR@
pcblibdir=	@PCBLIBDIR@

AUTOMAKE_OPTIONS = subdir-objects
HIDLIST = @HIDLIST@
noinst_LIBRARIES = @HIDLIBS@
EXTRA_LIBRARIES = libgtk.a liblesstif.a liblpr.a libgerber.a libbom.a libpng.a libps.a

pcblib_DATA=	\
	default_font \
	pcb-menu.res

pkgdata_DATA=	Pcb

bin_PROGRAMS=	pcb-bin

AM_CFLAGS = -DNDEBUG

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
	compat.c \
	compat.h \
	const.h \
	copy.c \
	copy.h \
	create.c \
	create.h \
	crosshair.c \
	crosshair.h \
	data.c \
	data.h \
	djopt.c \
	djopt.h \
	dolists.h \
	draw.c \
	draw.h \
	drill.c \
	drill.h \
	error.c \
	error.h \
	file.c \
	file.h \
	find.c \
	find.h \
	flags.c \
	global.h \
	heap.c \
	heap.h \
	hid.h \
	insert.c \
	insert.h \
	intersect.c \
	intersect.h \
	line.c \
	line.h \
	macro.h \
	main.c \
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
	output.c \
	output.h \
	parse_l.h \
	parse_l.l \
	parse_y.h \
	parse_y.y \
	polygon.c \
	polygon.h \
	puller.c \
	print.c \
	print.h \
	rats.c \
	rats.h \
	remove.c \
	remove.h \
	report.c \
	report.h \
	res_parse.y \
	res_lex.l \
	resource.h \
	rotate.c \
	rotate.h \
	rtree.c \
	rtree.h \
	rubberband.c \
	rubberband.h \
	search.c \
	search.h \
	select.c \
	select.h \
	set.c \
	set.h \
	strflags.c \
	strflags.h \
	undo.c \
	undo.h \
	vector.c \
	vector.h \
	vendor.c \
	vendor.h \
	hid/common/actions.c \
	hid/common/flags.c \
	hid/common/hidinit.c \
	hid/common/hidnogui.c \
	hid/common/extents.c \
	hid/hidint.h \
	core_lists.h

BUILT_SOURCES = \
	core_lists.h \
	hid/gtk/gtk_lists.h \
	hid/lesstif/lesstif_lists.h \
	hid/png/png_lists.h \
	hid/ps/ps_lists.h \
	parse_y.h \
	pcb-menu.h \
	res_parse.h \
	hid/common/hidlist.h

pcb_bin_LDADD = @HIDLIBS@
pcb_bin_DEPENDENCIES = @HIDLIBS@

# Action, Attribute, and Flag lists.
core_lists.h : ${pcb_bin_SOURCES} Makefile
	true > $@
	(for f in ${pcb_bin_SOURCES} ; do cat $(srcdir)/$$f ; done) | grep "^REGISTER" > $@.tmp
	mv $@.tmp $@

# for globalconst.h
INCLUDES=	-I$(top_srcdir) -I$(srcdir)/icons

EXTRA_DIST= \
	check_icon.data \
	default_font \
	$(srcdir)/hid/bom/hid.conf \
	$(srcdir)/hid/gerber/hid.conf \
	$(srcdir)/hid/gtk/gui-icons-misc.data \
	$(srcdir)/hid/gtk/gui-icons-mode-buttons.data \
	$(srcdir)/hid/gtk/hid.conf \
	$(srcdir)/hid/lesstif/hid.conf \
	$(srcdir)/hid/lpr/hid.conf \
	$(srcdir)/hid/png/hid.conf \
	$(srcdir)/hid/ps/hid.conf \
	Pcb.ad.in  \
	pcb-menu.res \
	pcbtest.sh.in

AM_YFLAGS=	-d

all-local: pcbtest.sh .test/Pcb

pcb-menu.h : pcb-menu.res
	echo '/* AUTOMATICALLY GENERATED FROM pcb-menu.res DO NOT EDIT */' > pcb-menu.h
	echo 'const char *pcb_menu_default[] = {' >> pcb-menu.h
	sed 's/\\/\\\\/g; s/"/\\"/g; s/^/"/; s/$$/",/' < ${srcdir}/pcb-menu.res >> pcb-menu.h
	echo '0};' >> pcb-menu.h

hid/common/hidlist.h : Makefile
	true > $@
	for e in ${HIDLIST}; do \
	  echo "HID_DEF($${e})" >> $@; \
	done

libgtk_a_CPPFLAGS = -I./hid/gtk
libgtk_a_SOURCES = \
	dolists.h \
	hid/hidint.h \
	hid/gtk/gtkhid-main.c \
	hid/gtk/gtkhid.h \
	hid/gtk/gui.h \
	hid/gtk/gui-command-window.c \
	hid/gtk/gui-config.c \
	hid/gtk/gui-dialog-print.c \
	hid/gtk/gui-dialog-size.c \
	hid/gtk/gui-dialog.c \
	hid/gtk/gui-keyref-window.c \
	hid/gtk/gui-library-window.c \
	hid/gtk/gui-log-window.c \
	hid/gtk/gui-misc.c \
	hid/gtk/gui-netlist-window.c \
	hid/gtk/gui-output-events.c \
	hid/gtk/gui-pinout-window.c \
	hid/gtk/gui-top-window.c \
	hid/gtk/gui-utils.c \
	hid/gtk/gtk_lists.h

hid/gtk/gtk_lists.h : ${libgtk_a_SOURCES} Makefile
	true > $@
	(for f in ${libgtk_a_SOURCES} ; do cat $(srcdir)/$$f ; done) | grep "^REGISTER" > $@.tmp
	mv $@.tmp $@

liblesstif_a_CPPFLAGS = -I./hid/lesstif
liblesstif_a_SOURCES = \
	dolists.h \
	hid/hidint.h \
	hid/lesstif/dialogs.c \
	hid/lesstif/library.c \
	hid/lesstif/main.c \
	hid/lesstif/menu.c \
	hid/lesstif/netlist.c \
	hid/lesstif/styles.c \
	hid/lesstif/lesstif_lists.h

hid/lesstif/lesstif_lists.h : ${liblesstif_a_SOURCES} Makefile
	true > $@
	(for f in ${liblesstif_a_SOURCES} ; do cat $(srcdir)/$$f ; done) | grep "^REGISTER" > $@.tmp
	mv $@.tmp $@

libgerber_a_SOURCES = \
	hid/hidint.h \
	hid/gerber/gerber.c

libbom_a_SOURCES = \
	hid/hidint.h \
	hid/bom/bom.c

libps_a_CPPFLAGS = -I./hid/ps
libps_a_SOURCES = \
	dolists.h \
	hid/hidint.h \
	hid/ps/ps.c \
	hid/ps/ps.h \
	hid/ps/eps.c \
	hid/ps/ps_lists.h

hid/ps/ps_lists.h : ${libps_a_SOURCES} Makefile
	true > $@
	(for f in ${libps_a_SOURCES} ; do cat $(srcdir)/$$f ; done) | grep "^REGISTER" > $@.tmp
	mv $@.tmp $@

libpng_a_CPPFLAGS = -I./hid/png
libpng_a_SOURCES = \
	dolists.h \
	hid/hidint.h \
	hid/png/png.c \
	hid/png/png.h \
	hid/png/png_lists.h

hid/png/png_lists.h : ${libpng_a_SOURCES} Makefile
	true > $@
	(for f in ${libpng_a_SOURCES} ; do cat $(srcdir)/$$f ; done) | grep "^REGISTER" > $@.tmp
	mv $@.tmp $@

liblpr_a_SOURCES = \
	hid/hidint.h \
	hid/lpr/lpr.c

DISTCLEANFILES= pcbtest.sh Pcb .test/Pcb pcb-menu.h \
	hid/common/hidlist.h \
	hid/gtk/gtk_lists.h \
	hid/lesstif/lesstif_lists.h \
	hid/png/png_lists.h \
	hid/ps/ps_lists.h \
	core_lists.h


# create wrapper script that lets you test pcb prior to installation
pcbtest.sh:	$(srcdir)/pcbtest.sh.in Makefile
	sed -e "s;@BUILDDIR@;`pwd`;g" -e "s;@TOPSRCDIR@;${top_srcdir};g" \
		$(srcdir)/pcbtest.sh.in > $@
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


