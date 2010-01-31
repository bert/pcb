## $Id$
##

## XXX the tools should probably be cleaned up a bit and installed
## in ${exec_prefix}/bin  (by bin_SCRIPTS and bin_PROGRAMS)

toolsdir=	$(pkgdatadir)/tools
tools_DATA=	${TOOLS}

scmdatadir = $(datadir)/gEDA/scheme
scmdata_DATA = $(DIST_SCM)

EXTRA_DIST= ${TOOLS}

bin_SCRIPTS= \
	MergePCBPS \
	Merge_dimPCBPS 

TOOLS= \
	apctools.zip \
	gerbertotk.c \
	pcb2ncap.tgz \
	tgo2pcb.tcl \
	MergePCBPS \
	Merge_dimPCBPS \
	PCB2HPGL 

DIST_SCM= \
	gnet-pcbfwd.scm
