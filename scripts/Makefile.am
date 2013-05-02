## $Id$
##
## Process this file with automake to produce Makefile.in
SUFFIXES = .pl .py .sh .html

TARGET_DIR  = @PCBLIBDIR@/

SHdir	     	= $(TARGET_DIR)
SH_DATA     	= ${SH_SCRIPTS}
dist_SH_DATA 	= $(SH_SCRIPTS)

htmldir	     	= $(TARGET_DIR)
#html_DATA    	= ${HTML}

HTML            = index.html

SH_SCRIPTS	= CreateLibrary \
		  CreateLibraryContents \
                  ListLibraryContents \
                  m4_create_static_lib \
                  png_diff \
                  QueryLibrary \
		  dir2html

EXTRA_DIST	= ${wildcard *.sh} ${HTML}

install-data-hook:
	$(MKDIR_P) $(DESTDIR)/$(TARGET_DIR)
	for file in ${SH_SCRIPTS}; do \
	  cp -p $(srcdir)/$$file $(distdir)/$(TARGET_DIR)/; \
	  cp -p $(srcdir)/$$file.sh $(distdir)/$(TARGET_DIR)/; \
	done;

#Uninstall seems to work perfect find for local builds but not during distribution
#check, apparently more autoconfusion
uninstall-hook:
	@for htmlfile in $(HTML); do \
	  f=$(htmldir)$$htmlfile; \
	  if test -n "$$f"; then \
	    (rm -f $$f) || exit $$?; \
	  fi; \
	done
	@for scripts in $(SH_SCRIPTS); do \
	  f=$(SHdir)$$scripts; \
	  if test -n "$$f"; then \
	    (rm -f $$f) || exit $$?; \
	  fi; \
	  if test -n "$$f.sh"; then \
	    (rm -f $$f.sh) || exit $$?; \
	  fi; \
	done

MOSTLYCLEANFILES     = *.log *.ps core FILE tmp *~ $(SH_SCRIPTS)
CLEANFILES 	     = *.log *.ps core FILE tmp  *~ $(SH_SCRIPTS)
DISTCLEANFILES	     = *.log core FILE tmp *~ $(SH_SCRIPTS)
MAINTAINERCLEANFILES = *.log *.ps core FILE tmp *~ Makefile.in $(SH_SCRIPTS)
