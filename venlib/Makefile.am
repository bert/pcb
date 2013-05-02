
vendors  = @VENDORDIRS@
libdir 	 = @PCB_VENLIB_DIR@

htmldir  = @PCBLIBDIR@/vendor
html_DATA= ${HTML}

HTML= venlib.html venlibf.html

EXTRA_DIST	= ${HTML}

# Copies venlib into distribution
dist-hook:
	for htmlfile in $(HTML); do \
	  cp -u $(srcdir)/$$htmlfile $(distdir)/; \
	done;
	@for vendor in $(vendors); do \
	  $(MKDIR_P) $(distdir)/$$vendor && \
	  list=`(ls $(srcdir)/$$vendor) | grep '.[Ff][Pp]$$'`; \
	  for file in $$list; do \
	    cp -p $(srcdir)/$$vendor/$$file $(distdir)/$$vendor || exit $$?; \
	  done; \
	  SubDirs=`find $(srcdir)/$$vendor/* -maxdepth 0 -type d ! -name $$vendor ! -name doc -printf "%f "`; \
	  for dirs in $$SubDirs; do \
	    $(MKDIR_P) $(distdir)/$$vendor/$$dirs && \
	    list=`(ls $(srcdir)/$$vendor/$$dirs) | grep '.[Ff][Pp]$$'`; \
	    for file in $$list; do \
	      cp -p $(srcdir)/$$vendor/$$dirs/$$file $(distdir)/$$vendor/$$dirs || exit $$?; \
	    done; \
	  done; \
	done;

# Copy the vendor footprints
install-data-local:
	@$(NORMAL_INSTALL)
	$(MKDIR_P) "$(libdir)"; \
	for htmlfile in $(HTML); do \
	  cp -u $(srcdir)/$$htmlfile $(distdir)/; \
	done;
	@for vendor in $(vendors); do \
	  d="$(libdir)/$$vendor"; \
	  $(MKDIR_P) "$$d"; \
	  list=`(ls $(srcdir)/$$vendor) | grep '.[Ff][Pp]$$'`; \
	  if test -n "$$list"; then \
	    (cd $(srcdir)/$$vendor && $(INSTALL_DATA) $$list "$$d") || exit $$?; \
	  fi; \
	  SubDirs=`find $(srcdir)/$$vendor/* -maxdepth 0 -type d ! -name $$vendor ! -name doc -printf "%f "`; \
	  for dirs in $$SubDirs; do \
	    $(MKDIR_P) "$$d/$$dirs"; \
	    list=`(ls $(srcdir)/$$vendor/$$dirs) | grep '.[Ff][Pp]$$'`; \
	    if test -n "$$list"; then \
	      (cd $(srcdir)/$$vendor/$$dirs && $(INSTALL_DATA) $$list "$$d/$$dirs") || exit $$?; \
	    fi; \
	  done; \
	done;
	cd $(htmldir) && exec @PCBLIBDIR@/dir2html -v venlib;

# Uninstalls symbols
uninstall-local:
	@$(NORMAL_UNINSTALL)
	@for vendor in $(vendor); do \
	  d="$(libdir)/$$vendor"; \
	  list=`(ls $(srcdir)/$$vendor) | grep '\.[Ff][Pp]$$'`; \
	  if test -n "$$list"; then \
	    echo " ( cd '$$d' && rm -f" $$list ")"; \
	    (cd "$$d" && rm -f $$list) || exit $$?; \
	  fi; \
	done

# Since there are no makefile in the footprint subfolders, each must be include here
# with ./*/*~
MOSTLYCLEANFILES     = *.log core FILE *~ ./*/*~
CLEANFILES           = *.log core FILE *~ ./*/*~
DISTCLEANFILES       = *.log core FILE *~ ./*/*~
MAINTAINERCLEANFILES = *.log core FILE *~ ./*/*~ Makefile.in ChangeLog
