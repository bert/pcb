## -*- makefile -*-
##
##                            COPYRIGHT
##
##  PCB, interactive printed circuit board design
##  Copyright (C) 2007 DJ Delorie
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

# The makefile including this must set BASE to the base name of the
# primary texinfo file.

info_TEXINFOS=	${BASE}.texi

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


dist_pkgdata_DATA=	${html_docs} ${pdf_docs} ${pcb_output}

html_docs=	${info_TEXINFOS:.texi=.html}
pdf_docs=	${info_TEXINFOS:.texi=.pdf}
dvi_docs=	${info_TEXINFOS:.texi=.dvi}

pcb_files = $(wildcard *.pcb)
fp_files  = $(wildcard *.fp)
sch_files = $(wildcard *.sch)
sym_files = $(wildcard *.sym)

gen_base = ${basename ${pcb_files} ${fp_files} ${sch_files} ${sym_files}}

png_files = ${addsuffix .png,${gen_base}}
png_found = $(filter-out ${png_files},$(wildcard *.png))

gen_base2 = ${gen_base} ${basename ${png_found}}
eps_files = ${addsuffix .eps,${gen_base2}}
pdf_files = ${addsuffix .pdf,${gen_base2}}

tab_files = $(wildcard *.tab)
tab_texi  = ${tab_files:.tab=.texi}

all_texi = $(wildcard *.texi)
inc_texi = $(filter-out ${BASE}.texi,$(all_texi))

${BASE}.html : ${png_files}
${BASE}.dvi : ${eps_files} ${pdf_files}
${BASE}.pdf : ${pdf_files}

# put the html manual into 1 file instead of multiple files?
AM_MAKEINFOHTMLFLAGS=	--css-include=$(srcdir)/${BASE}.css --no-split

# use this to avoid having the clean target delete pcb.{dvi,html,pdf,ps}
# which unfortunately deletes files which are part of the distfile
mostlyclean-aminfo:
	-rm -rf ${BASE}.aux ${BASE}.cp ${BASE}.cps ${BASE}.fn \
	  ${BASE}.fns ${BASE}.ky ${BASE}.kys ${BASE}.log ${BASE}.pg \
	  ${BASE}.pgs ${BASE}.tmp ${BASE}.toc ${BASE}.tp ${BASE}.tps \
	  ${BASE}.vr ${BASE}.vrs

EXTRA_DIST= \
	${BASE}.dvi \
	${BASE}.css \
	${inc_texi} \
	${pcb_files} \
	${fp_files} \
	${sch_files} \
	${sym_files} \
	${tab_files} \
	${tab_texi}

CLEANFILES = \
	${html_docs} \
	${pdf_docs} \
	${eps_files} \
	${png_files} \
	${pdf_files} \
	${tab_texi}

if GIT_VERSION
BUILT_SOURCES=	${tab_texi} ${pcb_output} ${inline_texi}
CLEANFILES+= \
	refcard.aux refcard.dvi refcard.log refcard.pdf \
	${pcb_output} ${tab_texi} ${inline_texi} pcb.dvi pcb.html pcb.pdf

else
BUILT_SOURCES=
endif

ASCII2TEXI=	${AWK} -f $(srcdir)/../ascii2texi.awk ncol=3

pcb_output=	\
	${pcb_files:.pcb=.eps} \
	${pcb_files:.pcb=.pdf} \
	${pcb_files:.pcb=.png}

SUFFIXES = .dvi .eps .pcb .pdf .png .tab .tex

%.png %.eps : %.pcb
	$(PCB) -x eps --font-path $(top_srcdir)/fonts --eps-file $*.eps $<
	$(PERL) $(srcdir)/../eps2png -resolution 100 -output $*.png $*.eps

%.png %.eps : %.fp
	$(PCB) -x eps --font-path $(top_srcdir)/fonts --eps-file $*.eps $<
	$(PERL) $(srcdir)/../eps2png -resolution 100 -output $*.png $*.eps

%.png %.eps : %.sch
if MISSING_GSCHEM
	@echo "****************************************************"
	@echo "WARNING:  gschem is missing on your system but"
	@echo "$@ is out of date and needs to rebuilt."
	@echo "Changes to $< will be ignored"
else
	$(GSCHEM) -p -o $*.eps -s ./print-eps.scm $<
	$(PERL) $(srcdir)/../eps2png -resolution 100 -output $*.png $*.eps
endif

%.png %.eps : %.sym
if MISSING_GSCHEM
	@echo "****************************************************"
	@echo "WARNING:  gschem is missing on your system but"
	@echo "$@ is out of date and needs to rebuilt."
	@echo "Changes to $< will be ignored"
else
	$(GSCHEM) -p -o $*.eps -s $(srcdir)/print-eps.scm $<
	$(PERL) $(srcdir)/../eps2png -resolution 100 -output $*.png $*.eps
endif

.eps.pdf :
if MISSING_PS2PDF
	@echo "****************************************************"
	@echo "WARNING:  ps2pdf is missing on your system but"
	@echo "$@ is out of date and needs to rebuilt."
	@echo "Changes to $< will be ignored"
else
	${PS2PDF} `${AWK} 'BEGIN{s=8}; /BoundingBox/ {printf("-r%d -g%dx%d", s*72, s*$$4, s*$$5);}' $<` $< $@
endif

.tab.texi:
	${ASCII2TEXI} $< > $@

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

%.eps : %.png
	pngtopnm $< | pnmtops -nocenter -equalpixels -noturn -dpi 200 > $@

djtest :
	@echo gen_base = ${gen_base}
	@echo
	@echo png_files = ${png_files}
	@echo
	@echo png_found = ${png_found}

