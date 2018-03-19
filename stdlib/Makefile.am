## Top level 'newlib' automake file

htmldir=	@PCBLIBDIR@/stdlib
html_DATA=	${HTML}

SUBDIRS=\
        battery \
        capacitor \
        connector \
        crystal \
        DFN \
        DIP \
        DO \
        filter \
        fuse \
        generic \
        header \
        inductor \
        jtag \
        jumper \
        led \
        misc \
        optical \
        PGA \
        PLCC \
        QFN \
        QFP \
        resistor \
        SIP \
        SC \
        SO \
        socket \
        TO

HTML=  stdlib.html stdlibf.html

EXTRA_DIST	= ${HTML}

install-data-hook:
	cd $(htmldir) && exec @PCBLIBDIR@/dir2html -v;

# Remove what hoook added + extra
uninstall-hook:
	@$(NORMAL_UNINSTALL)
	for htmlfile in $(HTML); do \
	  f="$(htmldir)$$htmlfile"; \
	  if test -n "$$f"; then \
	    (rm -f $$f) || exit $$?; \
	  fi; \
	done
	@for libdir in $(SUBDIRS); do \
	  f="$(htmldir)/$$libdir/index.html"; \
	  if test -n "$$f"; then \
	    (rm -f $$f) || exit $$?; \
	  fi; \
	done

MOSTLYCLEANFILES     = *.log core FILE *~ 
CLEANFILES           = *.log core FILE *~ 
DISTCLEANFILES       = *.log core FILE *~
MAINTAINERCLEANFILES = *.log core FILE *~ Makefile.in ChangeLog




