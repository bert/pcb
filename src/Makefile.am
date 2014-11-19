## -*- makefile -*-

SUBDIRS=	icons

CC = @CC_OR_CXX@

pcbtreedir=	@PCBTREEDIR@
pcblibdir=	@PCBLIBDIR@

AUTOMAKE_OPTIONS = subdir-objects
HIDLIST = @HIDLIST@
noinst_LIBRARIES = @HIDLIBS@
EXTRA_LIBRARIES = \
	libgtk.a liblesstif.a libbatch.a \
	liblpr.a libgerber.a libbom.a libpng.a libps.a libnelma.a \
	libgcode.a libscad.a

pcblib_DATA= \
	default_font \
	gpcb-menu.res \
	pcb-menu.res

bin_PROGRAMS=	pcb

if DEBUG_BUILD
# don't disable assert()
else
AM_CFLAGS= -DNDEBUG
endif

PCB_SRCS = \
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
	clip.c \
	clip.h \
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
	edif.y \
	edif_parse.h \
	error.c \
	error.h \
	file.c \
	file.h \
	find.c \
	find.h \
	flags.c \
	fontmode.c \
	free_atexit.c \
	free_atexit.h \
	gettext.h \
	global.h \
	heap.c \
	heap.h \
	hid.h \
	hid_draw.h \
	insert.c \
	insert.h \
	intersect.c \
	intersect.h \
	layerflags.c \
	layerflags.h \
	line.c \
	line.h \
	lrealpath.c \
	lrealpath.h \
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
	parse_l.h \
	parse_l.l \
	parse_y.y \
	pcb-printf.c \
	pcb-printf.h \
	polygon.c \
	polygon.h \
	polygon1.c \
	polyarea.h \
	puller.c \
	print.c \
	print.h \
	rats.c \
	rats.h \
	relocate.c \
	remove.c \
	remove.h \
	renumber.c \
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
	smartdisperse.c \
	strflags.c \
	strflags.h \
	teardrops.c \
	thermal.c \
	thermal.h \
	undo.c \
	undo.h \
	vector.c \
	vector.h \
	vendor.c \
	vendor.h \
	hid/common/actions.c \
	hid/common/actions.h \
	hid/common/flags.c \
	hid/common/hidinit.c \
	hid/common/hidinit.h \
	hid/common/hidnogui.c \
	hid/common/hidnogui.h \
	hid/common/extents.c \
	hid/common/draw_helpers.c \
	hid/common/draw_helpers.h \
	hid/common/hid_resource.c \
	hid/common/hid_resource.h \
	hid/hidint.h 

EXTRA_pcb_SOURCES = ${DBUS_SRCS} ${GL_SRCS} toporouter.c toporouter.h
DBUS_SRCS= \
	dbus-pcbmain.c \
	dbus-pcbmain.h \
	dbus.h \
	dbus.c

LIBGTK_GDK_SRCS= \
	hid/gtk/gtkhid-gdk.c

LIBGTK_GL_SRCS= \
	hid/gtk/gtkhid-gl.c \
	hid/gtk/gui-trackball.c \
	hid/gtk/gui-trackball.h

GL_SRCS= \
	hid/common/hidgl.c \
	hid/common/hidgl.h \
	hid/common/trackball.c \
	hid/common/trackball.h

BUILT_SOURCES = \
	core_lists.h \
	gpcb-menu.h \
	hid/gtk/gtk_lists.h \
	hid/lesstif/lesstif_lists.h \
	hid/batch/batch_lists.h \
	hid/png/png_lists.h \
	hid/gcode/gcode_lists.h \
	hid/nelma/nelma_lists.h \
	hid/ps/ps_lists.h \
	parse_y.h \
	pcb-menu.h \
	res_parse.h \
	hid/common/hidlist.h

pcb_LDADD = @HIDLIBS@
pcb_DEPENDENCIES = @HIDLIBS@

if WITH_TOPOROUTER
PCB_SRCS += toporouter.c toporouter.h
endif

# All these -I$(top_srcdir) in this file are for globalconst.h.
pcb_CPPFLAGS = -I$(top_srcdir)
pcb_SOURCES = ${PCB_SRCS} core_lists.h

TEST_SRCS = \
	pcb-printf.c	\
	main-test.c

unittest_CPPFLAGS = -I$(top_srcdir) -DPCB_UNIT_TEST
unittest_SOURCES = ${TEST_SRCS}
check_PROGRAMS = unittest
check_SCRIPTS = unittest
TESTS = unittest

# Action, Attribute, and Flag lists.
core_lists.h : ${PCB_SRCS} Makefile
	true > $@
	(for f in ${PCB_SRCS} ; do cat $(srcdir)/$$f ; done) | grep "^REGISTER" > $@.tmp
	mv $@.tmp $@

DEFS= 	-DLOCALEDIR=\"$(localedir)\" @DEFS@

EXTRA_DIST= \
	check_icon.data \
	default_font \
	$(srcdir)/hid/batch/hid.conf \
	$(srcdir)/hid/bom/hid.conf \
	$(srcdir)/hid/gcode/hid.conf \
	$(srcdir)/hid/gerber/hid.conf \
	$(srcdir)/hid/gtk/gui-icons-misc.data \
	$(srcdir)/hid/gtk/gui-icons-mode-buttons.data \
	$(srcdir)/hid/gtk/hid.conf \
	$(srcdir)/hid/gtk/pcb.rc \
	$(srcdir)/hid/lesstif/hid.conf \
	$(srcdir)/hid/lpr/hid.conf \
	$(srcdir)/hid/png/hid.conf \
	$(srcdir)/hid/nelma/hid.conf \
	$(srcdir)/hid/ps/hid.conf \
	$(srcdir)/hid/scad/hid.conf \
	gpcb-menu.res.in \
	pcb-menu.res.in \
	gpcb-menu.res.h \
	pcb-menu.res.h \
	pcbtest.sh.in \
	dbus.xml

AM_YFLAGS=	-d

all-local: pcbtest.sh

pcb-menu.res : pcb-menu.res.in
	echo '/* AUTOMATICALLY GENERATED FROM pcb-menu.res.in DO NOT EDIT */' > $@
	cat ${srcdir}/pcb-menu.res.in >> $@

pcb-menu.h : pcb-menu.res
	echo '/* AUTOMATICALLY GENERATED FROM pcb-menu.res DO NOT EDIT */' > $@
	echo 'const char *pcb_menu_default[] = {' >> $@
	sed 's/\\/\\\\/g; s/"/\\"/g; s/^/"/; s/$$/",/' < pcb-menu.res >> $@
	echo '0};' >> $@

gpcb-menu.res : gpcb-menu.res.in
	echo '/* AUTOMATICALLY GENERATED FROM gpcb-menu.res.in DO NOT EDIT */' > $@
	cat ${srcdir}/gpcb-menu.res.in >> $@

gpcb-menu.h : gpcb-menu.res
	echo '/* AUTOMATICALLY GENERATED FROM gpcb-menu.res DO NOT EDIT */' > $@
	echo 'const char *gpcb_menu_default[] = {' >> $@
	sed 's/\\/\\\\/g; s/"/\\"/g; s/^/"/; s/$$/",/' < gpcb-menu.res >> $@
	echo '0};' >> $@

# Menus i18n
%.res.h : %.res
	$(INTLTOOL_EXTRACT) --type=gettext/quoted $^
	mv -f $@ ${srcdir}/$@ || true

check-local: gpcb-menu.res.h pcb-menu.res.h

hid/common/hidlist.h : Makefile
	$(MKDIR_P) hid/common
	true > $@
	for e in ${HIDLIST}; do \
	  echo "HID_DEF($${e})" >> $@; \
	done

libgtk_a_CPPFLAGS = -I$(top_srcdir) -I./hid/gtk
LIBGTK_SRCS = \
	dolists.h \
	hid/hidint.h \
	hid/gtk/ghid-cell-renderer-visibility.c \
	hid/gtk/ghid-cell-renderer-visibility.h \
	hid/gtk/ghid-coord-entry.c \
	hid/gtk/ghid-coord-entry.h \
	hid/gtk/ghid-layer-selector.c \
	hid/gtk/ghid-layer-selector.h \
	hid/gtk/ghid-main-menu.c \
	hid/gtk/ghid-main-menu.h \
	hid/gtk/ghid-route-style-selector.c \
	hid/gtk/ghid-route-style-selector.h \
	hid/gtk/gtkhid-main.c \
	hid/gtk/gtkhid.h \
	hid/gtk/gui.h \
	hid/gtk/gui-command-window.c \
	hid/gtk/gui-config.c \
	hid/gtk/gui-dialog-print.c \
	hid/gtk/gui-dialog.c \
	hid/gtk/gui-drc-window.c \
	hid/gtk/gui-drc-window.h \
	hid/gtk/gui-keyref-window.c \
	hid/gtk/gui-library-window.c \
	hid/gtk/gui-library-window.h \
	hid/gtk/gui-log-window.c \
	hid/gtk/gui-misc.c \
	hid/gtk/gui-netlist-window.c \
	hid/gtk/gui-output-events.c \
	hid/gtk/gui-pinout-preview.c \
	hid/gtk/gui-pinout-preview.h \
	hid/gtk/gui-pinout-window.c \
	hid/gtk/gui-top-window.c \
	hid/gtk/gui-utils.c
libgtk_a_SOURCES = ${LIBGTK_SRCS} hid/gtk/gtk_lists.h

hid/gtk/gtk_lists.h : ${LIBGTK_SRCS} Makefile
	$(MKDIR_P) hid/gtk
	true > $@
	(for f in ${LIBGTK_SRCS} ; do cat $(srcdir)/$$f ; done) | grep "^REGISTER" > $@.tmp
	mv $@.tmp $@

# If we are building with dbus support, we need some extra files
if WITH_DBUS
dbus-introspect.h : dbus.xml Makefile
	echo '/* AUTOMATICALLY GENERATED FROM dbus.xml DO NOT EDIT */' > $@.tmp
	echo "static char *pcb_dbus_introspect_xml ="  > $@.tmp
	sed 's/\\/\\\\/g; s/"/\\"/g; s/^/"/; s/$$/"/' < $(srcdir)/dbus.xml >> $@.tmp
	echo ";" >> $@.tmp
	mv $@.tmp $@

PCB_SRCS+=	${DBUS_SRCS}
BUILT_SOURCES+=	dbus-introspect.h
endif

# If we are building with GL support, we need some extra files
if USE_GL
PCB_SRCS+=	${GL_SRCS}
LIBGTK_SRCS+=	${LIBGTK_GL_SRCS}
else
LIBGTK_SRCS+=	${LIBGTK_GDK_SRCS}
endif

# If we are building on win32, then compile in some icons for the
# desktop and application toolbar
if WIN32
pcb_icon.o : pcb_icon.ico $(srcdir)/hid/gtk/pcb.rc
	$(WINDRES) $(srcdir)/hid/gtk/pcb.rc $@

pcb_icon.ico: $(top_srcdir)/data/pcb_icon.ico
	cp $(top_srcdir)/data/pcb_icon.ico $@

pcb_LDADD+=			pcb_icon.o
pcb_DEPENDENCIES+=	pcb_icon.o
endif

liblesstif_a_CPPFLAGS = -I$(top_srcdir) -I./hid/lesstif
LIBLESSTIF_SRCS = \
	dolists.h \
	hid/hidint.h \
	hid/lesstif/dialogs.c \
	hid/lesstif/lesstif.h \
	hid/lesstif/library.c \
	hid/lesstif/main.c \
	hid/lesstif/menu.c \
	hid/lesstif/netlist.c \
	hid/lesstif/styles.c \
	hid/lesstif/xincludes.h 
liblesstif_a_SOURCES = ${LIBLESSTIF_SRCS} hid/lesstif/lesstif_lists.h

hid/lesstif/lesstif_lists.h : ${LIBLESSTIF_SRCS} Makefile
	$(MKDIR_P) hid/lesstif
	true > $@
	(for f in ${LIBLESSTIF_SRCS} ; do cat $(srcdir)/$$f ; done) | grep "^REGISTER" > $@.tmp
	mv $@.tmp $@

libbatch_a_CPPFLAGS = -I$(top_srcdir) -I./hid/batch
LIBBATCH_SRCS = \
	hid/hidint.h \
	hid/batch/batch.c 
libbatch_a_SOURCES = ${LIBBATCH_SRCS} hid/batch/batch_lists.h

hid/batch/batch_lists.h : ${LIBBATCH_SRCS} Makefile
	$(MKDIR_P) hid/batch
	true > $@
	(for f in ${LIBBATCH_SRCS} ; do cat $(srcdir)/$$f ; done) | grep "^REGISTER" > $@.tmp
	mv $@.tmp $@

libgerber_a_CPPFLAGS = -I$(top_srcdir)
libgerber_a_SOURCES = \
	hid/hidint.h \
	hid/gerber/gerber.c

libscad_a_CPPFLAGS = -I$(top_srcdir)
LIBSCAD_SRCS = \
	hid/hidint.h \
	hid/scad/scad.c \
	hid/scad/scadproto.c \
	hid/scad/scadcomp.c
libscad_a_SOURCES =  ${LIBSCAD_SRCS} hid/scad/scad.h

libbom_a_CPPFLAGS = -I$(top_srcdir)
libbom_a_SOURCES = \
	hid/hidint.h \
	hid/bom/bom.c

libps_a_CPPFLAGS = -I$(top_srcdir) -I./hid/ps
LIBPS_SRCS = \
	dolists.h \
	hid/hidint.h \
	hid/ps/ps.c \
	hid/ps/ps.h \
	hid/ps/eps.c 
libps_a_SOURCES = ${LIBPS_SRCS} hid/ps/ps_lists.h

hid/ps/ps_lists.h : ${LIBPS_SRCS} Makefile
	$(MKDIR_P) hid/ps
	true > $@
	(for f in ${LIBPS_SRCS} ; do cat $(srcdir)/$$f ; done) | grep "^REGISTER" > $@.tmp
	mv $@.tmp $@

libpng_a_CPPFLAGS = -I$(top_srcdir) -I./hid/png
LIBPNG_SRCS = \
	dolists.h \
	hid/hidint.h \
	hid/png/png.c \
	hid/png/png.h 
libpng_a_SOURCES = ${LIBPNG_SRCS} hid/png/png_lists.h

hid/png/png_lists.h : ${LIBPNG_SRCS} Makefile
	$(MKDIR_P) hid/png
	true > $@
	(for f in ${LIBPNG_SRCS} ; do cat $(srcdir)/$$f ; done) | grep "^REGISTER" > $@.tmp
	mv $@.tmp $@

libgcode_a_CPPFLAGS = -I$(top_srcdir) -I./hid/gcode
LIBGCODE_SRCS = \
	dolists.h \
	hid/hidint.h \
	hid/gcode/gcode.c \
	hid/gcode/decompose.c \
	hid/gcode/decompose.h \
	hid/gcode/trace.c \
	hid/gcode/trace.h \
	hid/gcode/curve.c \
	hid/gcode/curve.h \
	hid/gcode/auxiliary.h \
	hid/gcode/bitmap.h \
	hid/gcode/lists.h \
	hid/gcode/potracelib.h 
libgcode_a_SOURCES = ${LIBGCODE_SRCS} hid/gcode/gcode_lists.h

hid/gcode/gcode_lists.h : ${LIBGCODE_SRCS} Makefile
	$(MKDIR_P) hid/gcode
	true > $@
	(for f in ${LIBGCODE_SRCS} ; do cat $(srcdir)/$$f ; done) | grep "^REGISTER" > $@.tmp
	mv $@.tmp $@

libnelma_a_CPPFLAGS = -I$(top_srcdir) -I./hid/nelma
LIBNELMA_SRCS = \
	dolists.h \
	hid/hidint.h \
	hid/nelma/nelma.c
libnelma_a_SOURCES = ${LIBNELMA_SRCS} hid/nelma/nelma_lists.h

hid/nelma/nelma_lists.h : ${LIBNELMA_SRCS} Makefile
	$(MKDIR_P) hid/nelma
	true > $@
	(for f in ${LIBNELMA_SRCS} ; do cat $(srcdir)/$$f ; done) | grep "^REGISTER" > $@.tmp
	mv $@.tmp $@

liblpr_a_CPPFLAGS = -I$(top_srcdir)
liblpr_a_SOURCES = \
	hid/hidint.h \
	hid/lpr/lpr.c

DISTCLEANFILES= pcbtest.sh gpcb-menu.h pcb-menu.h \
	hid/batch/batch_lists.h \
	hid/common/hidlist.h \
	hid/gtk/gtk_lists.h \
	hid/lesstif/lesstif_lists.h \
	hid/png/png_lists.h \
	hid/gcode/gcode_lists.h \
	hid/nelma/nelma_lists.h \
	hid/ps/ps_lists.h \
	core_lists.h \
	dbus-introspect.h \
	gpcb-menu.res \
	pcb-menu.res \
	gpcb-menu.res.h \
	pcb-menu.res.h

# create wrapper script that lets you test pcb prior to installation
pcbtest.sh:	$(srcdir)/pcbtest.sh.in Makefile
	sed -e 's;@BUILDDIR@;${abs_builddir};g' -e 's;@TOP_BUILDDIR@;${abs_top_builddir};g' \
		-e 's;@TOPSRCDIR@;${abs_top_srcdir};g' \
		$(srcdir)/pcbtest.sh.in > $@
	chmod 755 $@

