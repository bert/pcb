
SUBDIRS= BNC DB DIN MTA RF RIA RJ SUBD XL

pcblibsubdir= @PCB_STDLIB_DIR@/connector

pcblibsub_DATA= ${FOOTPRINTS}

FOOTPRINTS= ${wildcard *.fp} 
	 
EXTRA_DIST= ${FOOTPRINTS}

MOSTLYCLEANFILES = *.log core FILE *~ 
CLEANFILES = *.log core FILE *~ 
DISTCLEANFILES = *.log core FILE *~
MAINTAINERCLEANFILES = *.log core FILE *~ Makefile.in ChangeLog

