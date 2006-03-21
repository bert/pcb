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
pcb_TEXINFOS=	\
	fractional_size.texi \
	letter_size.texi \
	metric_size.texi \
	wire_size.texi \
	pcbfile.texi

dist_pkgdata_DATA=	${html_docs} ${ps_docs} ${pdf_docs}

dist_man_MANS=	pcb.1

html_docs=	${info_TEXINFOS:.texi=.html}
ps_docs=	${info_TEXINFOS:.texi=.ps} refcard.ps
pdf_docs=	${info_TEXINFOS:.texi=.pdf} refcard.pdf

# put the html manual into 1 file instead of multiple files?
AM_MAKEINFOHTMLFLAGS=	--css-include=pcb.css

# use this to avoid having the clean target delete pcb.{dvi,html,pdf,ps}
# which unfortunately deletes files which are part of the distfile
mostlyclean-aminfo:
	-rm -rf pcb.aux pcb.cp pcb.cps pcb.fn pcb.fns pcb.ky pcb.kys pcb.log pcb.pg \
	pcb.pgs pcb.tmp pcb.toc pcb.tp pcb.tps pcb.vr pcb.vrs 

EXTRA_DIST= \
	ascii2texi.awk \
	extract-docs \
	refcard.tex \
	pad.eps \
	pad.obj \
	pad.pdf \
	pad.png \
	pcb.dvi \
	pcb.css \
	refcard.dvi \
	wire_size.tab \
	wire_size.texi \
	letter_size.tab \
	letter_size.texi \
	metric_size.tab \
	metric_size.texi \
	fractional_size.tab \
	fractional_size.texi \
	pcbfile.texi

SUFFIXES=	.obj .tab


MAINTAINERCLEANFILES= \
	refcard.aux refcard.dvi refcard.log refcard.pdf refcard.ps \
	pcb.dvi pcb.html pcb.pdf pcb.ps

BUILT_SOURCES=	\
	pad.eps \
	pad.pdf \
	pad.png \
	wire_size.texi \
	letter_size.texi \
	metric_size.texi \
	fractional_size.texi \
	pcbfile.texi

ASCII2TEXI=	${AWK} -f $(srcdir)/ascii2texi.awk ncol=3

$(srcdir)/wire_size.texi: $(srcdir)/wire_size.tab $(srcdir)/ascii2texi.awk

$(srcdir)/letter_size.texi: $(srcdir)/letter_size.tab $(srcdir)/ascii2texi.awk

$(srcdir)/metric_size.texi: $(srcdir)/metric_size.tab $(srcdir)/ascii2texi.awk

$(srcdir)/fractional_size.texi: $(srcdir)/fractional_size.tab $(srcdir)/ascii2texi.awk

.tab.texi:
	${ASCII2TEXI} $< > $@


TGIF_X=		XENVIRONMENT=$(srcdir)/Xdefaults.tgif ${TGIF}
TGIF_FLAGS=	-print -color

pcbfile.texi : extracted-docs

.PHONY : extracted-docs
extracted-docs :
	perl $(srcdir)/extract-docs $(srcdir)

$(srcdir)/pad.eps: $(srcdir)/pad.obj
	${TGIF_X} ${TGIF_FLAGS} -eps $(srcdir)/pad.obj

$(srcdir)/pad.pdf: $(srcdir)/pad.eps
	ps2pdf $(srcdir)/pad.eps $(srcdir)/pad.pdf

$(srcdir)/pad.png: $(srcdir)/pad.obj
	${TGIF_X} ${TGIF_FLAGS} -png $(srcdir)/pad.obj

refcard.dvi: $(srcdir)/refcard.tex
	${LATEX} $(srcdir)/refcard.tex

refcard.ps: $(srcdir)/refcard.dvi
	${DVIPS} -Ppdf -j0 -D 300 -o $@ refcard.dvi

refcard.pdf: refcard.ps
	${PS2PDF} refcard.ps

