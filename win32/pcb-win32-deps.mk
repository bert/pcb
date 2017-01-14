# This makefile fragment is used to manage downloading, verifying, and when required
# building the required files for a cygwin build environment

# Normally called from build_pcb but can be run manually with:
# for glib:
#
# gmake -f pcb-win32-deps.mk DEPMK=setup/glib.mk <target>

include config.mk
include ${DEPMK}

ALL_DISTURLS=	${SRC_DISTURLS} ${DEV_DISTURLS} ${RUN_DISTURLS}
DISTINFO=	${CURDIR}/setup/${NAME}.distinfo
PROMPT=		"PCB Dependencies Setup --\> "

.PHONY: check-dirs
check-dirs:
	@test -d ${DISTDIR} || mkdir -p ${DISTDIR}
	@test -d ${STAMPDIR} || mkdir -p ${STAMPDIR}
	@test -d ${RUNTIMEDIR} || mkdir -p ${RUNTIMEDIR}
	@test -d ${DEVTIMEDIR} || mkdir -p ${DEVTIMEDIR}
	@test -d ${LIBSRCDIR} || mkdir -p ${LIBSRCDIR}

.PHONY: fetch
fetch: check-dirs
	@${ECHO} "${PROMPT}Checking downloads for ${NAME}"
	@for disturl in ${ALL_DISTURLS} ; do \
		distfile=`${ECHO} $$disturl | ${SED} 's;.*/;;g'` ; \
		if test ! -f ${DISTDIR}/$$distfile ; then \
			${ECHO} "${PROMPT}Attempting to download $$disturl" ; \
			(cd ${DISTDIR} && ${WGET} $$disturl) || exit $$?; \
		fi ; \
	done
	@${ECHO} "${PROMPT}Downloads complete for ${NAME}"

.PHONY: checksum
checksum: check-dirs
	@${ECHO} "${PROMPT}Comparing checksums for ${NAME}"
	@for disturl in ${ALL_DISTURLS} ; do \
		distfile=`echo $$disturl | ${SED} 's;.*/;;g'` ; \
		if test ! -f ${DISTDIR}/$$distfile ; then \
			${ECHO} "Missing distfile: ${DISTDIR}/$$distfile" ; \
			exit 1 ; \
		fi ; \
		hash=`grep "\($$distfile\)" ${DISTINFO} | ${AWK} 'NF>1 { print $$NF }'` ; \
		dlhash=`${OPENSSL} ${HASHTYPE} ${DISTDIR}/$$distfile | ${AWK} 'NF>1 {print $$NF}'` ; \
		if test "$$hash" != "$$dlhash" ; then \
			${ECHO} "Checksum failure for $$distfile" ; \
			${ECHO} "     computed:  $$dlhash" ; \
			${ECHO} "     expected:  $$hash" ; \
			exit 1 ; \
		fi ; \
	done
	@${ECHO} "${PROMPT}All checksums match for ${NAME}"

.PHONY: makesum
makesum: check-dirs
	@${ECHO} "${PROMPT}Recreating checksums for ${NAME}"
	@${ECHO} "" > ${DISTINFO}.tmp
	@for disturl in ${ALL_DISTURLS} ; do \
		distfile=`echo $$disturl | ${SED} 's;.*/;;g'` ; \
		if test ! -f ${DISTDIR}/$$distfile ; then \
			${ECHO} "Missing distfile: ${DISTDIR}/$$distfile" ; \
			exit 1 ; \
		fi ; \
		${ECHO} "Calculating hash for $${distfile}..." ; \
		(cd ${DISTDIR} && ${OPENSSL} ${HASHTYPE} $$distfile >> ${DISTINFO}.tmp) ; \
	done
	@mv -f ${DISTINFO}.tmp ${DISTINFO}
	@${ECHO} "${PROMPT}Recreated ${DISTINFO} for ${NAME}"

.PHONY: extract
extract: check-dirs
	@${ECHO} "${PROMPT}Extracting runtime package for ${NAME}"
	@for disturl in ${RUN_DISTURLS} ; do \
		distfile=`echo $$disturl | ${SED} 's;.*/;;g'` ; \
		if test ! -f ${DISTDIR}/$$distfile ; then \
			${ECHO} "Missing distfile: ${DISTDIR}/$$distfile" ; \
			exit 1 ; \
		fi ; \
		(cd ${RUNTIMEDIR} && ${TOOLSDIR}/extract.sh ${DISTDIR}/$$distfile) ; \
	done
	@${ECHO} "${PROMPT}Extracting development package for ${NAME}"
	@for disturl in ${DEV_DISTURLS} ; do \
		distfile=`echo $$disturl | ${SED} 's;.*/;;g'` ; \
		if test ! -f ${DISTDIR}/$$distfile ; then \
			${ECHO} "Missing distfile: ${DISTDIR}/$$distfile" ; \
			exit 1 ; \
		fi ; \
		(cd ${DEVTIMEDIR} && ${TOOLSDIR}/extract.sh ${DISTDIR}/$$distfile) ; \
	done
	@${ECHO} "${PROMPT}Extracting src package for ${NAME}"
	@for disturl in ${SRC_DISTURLS} ; do \
		distfile=`echo $$disturl | ${SED} 's;.*/;;g'` ; \
		if test ! -f ${DISTDIR}/$$distfile ; then \
			${ECHO} "Missing distfile: ${DISTDIR}/$$distfile" ; \
			exit 1 ; \
		fi ; \
		(cd ${LIBSRCDIR} && ${TOOLSDIR}/extract.sh ${DISTDIR}/$$distfile) ; \
	done
	@${ECHO} "${PROMPT}Done extracting for ${NAME}"


.PHONY: show-license
show-license:
	@${ECHO} "${LICENSE} ${NAME}-${VER}"

.PHONY: config-variables
config-variables:
	@${ECHO} "gtk_win32_runtime=${RUNTIMEDIR}"
	@${ECHO} "gtk_win32_devel=${DEVTIMEDIR}"

.PHONY: patch
patch:
	echo "not implemented"

.PHONY: configure
configure:
	echo "not implemented"

.PHONY: build
build:
	echo "not implemented"

.PHONY: install
install:
	echo "not implemented"
