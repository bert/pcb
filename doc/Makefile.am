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
pkgdata_DATA=	${html_docs} ${ps_docs} ${pdf_docs}

html_docs=	${info_TEXINFOS:.texi=.html}
ps_docs=	${info_TEXINFOS:.texi=.ps} refcard.ps
pdf_docs=	${info_TEXINFOS:.texi=.pdf} refcard.pdf

EXTRA_DIST= \
	${html_docs} \
	${ps_docs} \
	${pdf_docs} \
	refcard.tex \
	pad.obj \
	pad.png \
	pad.eps \
	pcb.dvi \
	refcard.dvi

SUFFIXES=	.html .pdf

DISTCLEANFILES= \
	*.fns \
	*.trs \
	*.vrs \
	refcard.log \
	refcard.aux 

BUILT_SOURCES=	\
	pad.eps \
	pad.png

TGIF_X=		XENVIRONMENT=$(srcdir)/Xdefaults.tgif ${TGIF}
TGIF_FLAGS=	-print -color

.obj.eps:
	${TGIF_X} ${TGIF_FLAGS} -eps $<

.obj.png:
	${TGIF_X} ${TGIF_FLAGS} -png $<

.ps.pdf:
	ps2pdf $<

.texi.html:
	$(MAKEINFO) --html $<

.PHONY: html
html: version.texi $(html_docs)


.PHONY: ps
ps: version.texi $(ps_docs)

.PHONY: pdf
pdf: version.texi $(pdf_docs)

refcard.dvi: refcard.tex
	${LATEX} ${srcdir}/refcard.tex

refcard.ps: refcard.dvi
	${DVIPS} -Ppdf -j0 -D 300 -o $@ refcard.dvi

