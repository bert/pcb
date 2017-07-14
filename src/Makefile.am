## -*- makefile -*-

SUBDIRS=	icons

CC = @CC_OR_CXX@

pcbtreedir=	@PCBTREEDIR@
pcblibdir=	@PCBLIBDIR@

AUTOMAKE_OPTIONS = subdir-objects
HIDLIST = @HIDLIST@
noinst_LIBRARIES = @HIDLIBS@
EXTRA_LIBRARIES = \
	libbatch.a \
	libbom.a \
	libgcode.a \
	libgerber.a \
	libgtk.a \
	libipcd356.a \
	liblesstif.a \
	liblpr.a \
	libnelma.a \
	libpng.a \
	libps.a \
	libstep.a

pcblib_DATA= \
	default_font \
	gpcb-menu.res \
	pcb-menu.res

bin_PROGRAMS=	pcb

if DEBUG_BUILD
# don't disable assert()
AM_CFLAGS= -Wno-unused-parameter
else
AM_CFLAGS= -DNDEBUG -Wno-unused-parameter
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
	draw_funcs.c \
	draw_funcs.h \
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
	getline.c \
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
	netclass.c \
	netclass.h \
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
	strcasestr.c \
	strflags.c \
	strflags.h \
	sweep.h \
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
	hid/common/draw_drc.c \
	hid/common/draw_drc.h \
	hid/common/draw_helpers.c \
	hid/common/draw_helpers.h \
	hid/common/hid_resource.c \
	hid/common/hid_resource.h \
	hid/common/appearance.c \
	hid/common/appearance.h \
	hid/common/contour3d.c \
	hid/common/contour3d.h \
	hid/common/edge3d.c \
	hid/common/edge3d.h \
	hid/common/face3d.c \
	hid/common/face3d.h \
	hid/common/face3d_gl.c \
	hid/common/face3d_gl.h \
	hid/common/object3d.c \
	hid/common/object3d.h \
	hid/common/object3d_gl.c \
	hid/common/object3d_gl.h \
	hid/common/quad.c \
	hid/common/quad.h \
	hid/common/step_id.h \
	hid/common/vertex3d.c \
	hid/common/vertex3d.h \
	hid/hidint.h 

EXTRA_pcb_SOURCES = ${DBUS_SRCS} ${GL_SRCS} ${LIBGTK_GL_SRCS} ${LIBGTK_SNAVI_SRCS} toporouter.c toporouter.h
nodist_EXTRA_pcb_SOURCES = dummy.cpp
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
	hid/common/glext.h \
	hid/common/hidgl_package_acy_resistor.c \
	hid/common/hidgl_shaders.c \
	hid/common/hidgl_shaders.h \
	hid/common/trackball.c \
	hid/common/trackball.h \
	borast/borast-minimal.h \
	borast/borastint-minimal.h \
	borast/borast-bentley-ottmann.c \
	borast/borast-combsort-private.h \
	borast/borast-compiler-private.h \
	borast/borast-fixed-private.h \
	borast/borast-fixed-type-private.h \
	borast/borast-freelist.c \
	borast/borast-freelist-private.h \
	borast/borast-malloc-private.h \
	borast/borast-traps.c \
	borast/borast-traps-private.h \
	borast/borast-types-private.h \
	borast/borast-wideint.c \
	borast/borast-wideint-private.h \
	borast/borast-wideint-type-private.h

LIBGTK_SNAVI_SRCS= \
	hid/gtk/snavi.c \
	hid/gtk/snavi.h


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
	hid/step/step_lists.h \
	parse_y.h \
	pcb-menu.h \
	res_parse.h \
	hid/common/hidlist.h

pcb_LDADD = @HIDLIBS@
pcb_DEPENDENCIES = @HIDLIBS@

if WITH_TOPOROUTER
PCB_SRCS += toporouter.c toporouter.h
pcb_LDADD += ../gts/libgts.a
pcb_DEPENDENCIES += ../gts/libgts.a
endif

# All these -I$(top_srcdir) in this file are for globalconst.h.
pcb_CPPFLAGS = -I$(top_srcdir) -I$(srcdir)/../gts
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
	$(srcdir)/hid/ipcd356/hid.conf \
	$(srcdir)/hid/lesstif/hid.conf \
	$(srcdir)/hid/lpr/hid.conf \
	$(srcdir)/hid/png/hid.conf \
	$(srcdir)/hid/nelma/hid.conf \
	$(srcdir)/hid/ps/hid.conf \
	$(srcdir)/hid/step/hid.conf \
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

# Menus i18n.  The .res.h files are used simply to give strings which
# feed into ../po/pcb.pot for translation.  This is the only place
# these files are used.  When menu resources are parsed, the strings
# are translated.  We likely should find a way to not try to copy
# back to srcdir all the time but it is possible that the gettext/intltool
# flow can't really deal with getting strings from built sources.
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

# If we are building with SpaceNavigator support, we need some extra files
if WITH_SNAVI
LIBGTK_SRCS+=	${LIBGTK_SNAVI_SRCS}
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

libbom_a_CPPFLAGS = -I$(top_srcdir)
libbom_a_SOURCES = \
	hid/hidint.h \
	hid/bom/bom.c

libipcd356_a_CPPFLAGS = -I$(top_srcdir)
libipcd356_a_SOURCES = \
	hid/hidint.h \
	hid/ipcd356/ipcd356.c

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

libstep_a_CPPFLAGS = \
	-I$(top_srcdir) \
	-I./hid/step \
	-I${STEPCODEDIR}/include/stepcode/ \
	-I${STEPCODEDIR}/include/stepcode/base \
	-I${STEPCODEDIR}/include/stepcode/clutils \
	-I${STEPCODEDIR}/include/stepcode/cldai \
	-I${STEPCODEDIR}/include/stepcode/clstepcore \
	-I${STEPCODEDIR}/include/stepcode/cleditor \
	-I${STEPCODEDIR}/include/schemas/sdai_ap214e3

LIBSTEP_SRCS = \
	dolists.h \
	hid/hidint.h \
	hid/step/step.c \
	hid/step/step.h \
	hid/step/assembly.cpp \
	hid/step/assembly.h \
	hid/step/model.cpp \
	hid/step/model.h \
	hid/step/utils.cpp \
	hid/step/utils.h \
	hid/step/step_writer.c \
	hid/step/step_writer.h \
	hid/step/object3d_step.c \
	hid/step/object3d_step.h
libstep_a_SOURCES = ${LIBSTEP_SRCS} hid/step/step_lists.h

pcb_LDFLAGS = \
	-Wl,-rpath,${STEPCODEDIR}/lib # HACK FOR EASIER DEBUG

pcb_LDADD += \
	-L${STEPCODEDIR}/lib \
	-lstepcore \
	-lstepeditor \
	-lstepdai \
	-lsteputils \
	-lsdai_ap214e3

hid/step/step_lists.h : ${LIBPS_SRCS} Makefile
	$(MKDIR_P) hid/step
	true > $@
	(for f in ${LIBSTEP_SRCS} ; do cat $(srcdir)/$$f ; done) | grep "^REGISTER" > $@.tmp
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

