## -*- makefile -*-
##
##                            COPYRIGHT
##
##  PCB, interactive printed circuit board design
##  Copyright (C) 1994,1995,1996 Thomas Nau
##
##  This program is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation; either version 2 of the License, or
##  (at your option) any later version.
##
##  This program is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with this program; if not, write to the Free Software
##  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
##
##  Contact addresses for paper mail and Email:
##  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
##  Thomas.Nau@rz.uni-ulm.de
##
##  RCS: $Id$
##

info_TEXINFOS=	pcb.texi

# We avoid adding all the dependencies except in maintainer mode.  This is
# because the automake team feels that you should not require the user
# to have makeinfo and friends installed just to build the package.
# This seems reasonable.  Because of that, the documentation must be
# built in the source directory so it can be found when creating a distfile
# and this means you can't build outside the source tree with a read-only
# source.  So, ignore the dependencies unless we're in maintainer mode
# but be sure and put the generated files in the distfile via EXTRA_DIST
# and dist_pkgdata_DATA.  Also this helps us avoid problem with users
# having various out of date versions of the texinfo package installed
# which can have all sorts of issues.
pcb_TEXINFOS=	@MAINTAINER_MODE_TRUE@ ${inline_texi} ${pcb_output} ${tab_texi}

dist_pkgdata_DATA=	${html_docs} ${ps_docs} ${pdf_docs} ${pcb_output}

dist_man_MANS=	pcb.1

html_docs=	${info_TEXINFOS:.texi=.html}
ps_docs=	${info_TEXINFOS:.texi=.ps} refcard.ps
pdf_docs=	${info_TEXINFOS:.texi=.pdf} refcard.pdf

# put the html manual into 1 file instead of multiple files?
AM_MAKEINFOHTMLFLAGS=	--css-include=$(srcdir)/pcb.css

# use this to avoid having the clean target delete pcb.{dvi,html,pdf,ps}
# which unfortunately deletes files which are part of the distfile
mostlyclean-aminfo:
	-rm -rf pcb.aux pcb.cp pcb.cps pcb.fn pcb.fns pcb.ky pcb.kys pcb.log pcb.pg \
	pcb.pgs pcb.tmp pcb.toc pcb.tp pcb.tps pcb.vr pcb.vrs 

EXTRA_DIST= \
	ascii2texi.awk \
	extract-docs \
	refcard.tex \
	pcb.dvi \
	pcb.css \
	refcard.dvi \
	${inline_texi} \
	${pcb_files} \
	${tab_files} \
	${tab_texi}

MAINTAINERCLEANFILES= \
	refcard.aux refcard.dvi refcard.log refcard.pdf refcard.ps \
	${pcb_output} ${tab_texi} ${inline_texi} pcb.dvi pcb.html pcb.pdf pcb.ps

BUILT_SOURCES=	@MAINTAINER_MODE_TRUE@ ${tab_texi} ${pcb_output} ${inline_texi}

inline_texi=	\
	actions.texi \
	pcbfile.texi

ASCII2TEXI=	${AWK} -f $(srcdir)/ascii2texi.awk ncol=3

# Tables
tab_files=	\
	fractional_size.tab \
	letter_size.tab \
	metric_size.tab \
	wire_size.tab
tab_texi=	${tab_files:.tab=.texi}

# PCB Drawings
pcb_files=	\
	pad.pcb

pcb_output=	\
	${pcb_files:.pcb=.eps} \
	${pcb_files:.pcb=.gif} \
	${pcb_files:.pcb=.pdf} \
	${pcb_files:.pcb=.png}

${inline_texi} : extracted-docs

# no need to build these when not in maintainer mode.  They're not used then
# anyway.
.PHONY : extracted-docs
extracted-docs :
	@MAINTAINER_MODE_TRUE@perl $(srcdir)/extract-docs $(srcdir)
	@@MAINTAINER_MODE_FALSE@echo "Skipping documentation extraction since you are not in maintainer-mode"

SUFFIXES = .dvi .eps .gif .pcb .pdf .png .ps .tab .tex

.pcb.eps :
	../src/pcb-bin -x eps --only-visible --font-path $(top_srcdir)/src --eps-file $@ $<

.pcb.gif :
	../src/pcb-bin -x png --only-visible --format GIF --font-path $(top_srcdir)/src --outfile $@ $<

.pcb.png :
	../src/pcb-bin -x png --only-visible --font-path $(top_srcdir)/src --outfile $@ $<

.eps.pdf :
if MISSING_PS2PDF
	@echo "****************************************************"
	@echo "WARNING:  ps2pdf is missing on your system but"
	@echo "$@ is out of date and needs to rebuilt."
	@echo "Changes to $< will be ignored"
else
	${PS2PDF} -r72 `sed -n '/BoundingBox:/ s/.* \([0-9][0-9]*\) \([0-9][0-9]*\)/-g\1x\2/ p' $<` $< $@
endif

.tab.texi:
	${ASCII2TEXI} $< > $@

.tex.dvi:
if MISSING_LATEX
	@echo "****************************************************"
	@echo "WARNING:  LaTeX is missing on your system but"
	@echo "$@ is out of date and needs to rebuilt."
	@echo "Changes to $< will be ignored"
else
	${LATEX} $<
	${LATEX} $<
	${LATEX} $<
endif

.dvi.ps:
if MISSING_DVIPS
	@echo "****************************************************"
	@echo "WARNING:  dvips is missing on your system but"
	@echo "$@ is out of date and needs to rebuilt."
	@echo "Changes to $< will be ignored"
else
	${DVIPS} -Ppdf -j0 -D 300 -o $@ $<
endif

.tex.pdf:
if MISSING_PDFLATEX
	@echo "****************************************************"
	@echo "WARNING:  pdflatex is missing on your system but"
	@echo "$@ is out of date and needs to rebuilt."
	@echo "Changes to $< will be ignored"
else
	${PDFLATEX} $<
	${PDFLATEX} $<
	${PDFLATEX} $<
endif
