## -*- makefile -*-

SUBDIRS=	golden inputs

TESTS_ENVIRONMENT=	\
	GERBV=${GERBV} \
	IM_ANIMATE=${IM_ANIMATE} \
	IM_COMPARE=${IM_COMPARE} \
	IM_COMPOSITE=${IM_COMPOSITE} \
	IM_CONVERT=${IM_CONVERT} \
	IM_DISPLAY=${IM_DISPLAY} \
	IM_MONTAGE=${IM_MONTAGE}

RUN_TESTS=	run_tests.sh

check_SCRIPTS=		${RUN_TESTS}

# if we have the required tools, then run the regression test
if HAVE_TEST_TOOLS
TESTS=	${RUN_TESTS}
endif

EXTRA_DIST=	${RUN_TESTS} tests.list README.txt

# these are created by 'make check'
clean-local:
	rm -rf outputs

