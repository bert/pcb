## -*- makefile -*-

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
  TESTS = ${RUN_TESTS}
else
  TESTS = missing_test
endif

EXTRA_DIST= ${RUN_TESTS} tests.list README.txt

include inputs/Makefile_insert

include golden/hid_bom1/Makefile_insert
include golden/hid_bom2/Makefile_insert
include golden/hid_bom3/Makefile_insert
include golden/hid_bom4/Makefile_insert

include golden/hid_gerber1/Makefile_insert
include golden/hid_gerber2/Makefile_insert
include golden/hid_gerber3/Makefile_insert
include golden/hid_gcode1/Makefile_insert
include golden/hid_gcode2/Makefile_insert
include golden/hid_gcode3/Makefile_insert
include golden/hid_gcode4/Makefile_insert
include golden/hid_gcode5/Makefile_insert
include golden/hid_gcode6/Makefile_insert
include golden/hid_gcode7/Makefile_insert
include golden/hid_gcode8/Makefile_insert
include golden/hid_gcode9/Makefile_insert
include golden/hid_gcode10/Makefile_insert
include golden/hid_gcode11/Makefile_insert

include golden/hid_ipcd356/Makefile_insert

include golden/hid_openscad1/Makefile_insert
include golden/hid_openscad2/Makefile_insert
include golden/hid_openscad3/Makefile_insert
include golden/hid_openscad4/Makefile_insert

include golden/hid_png1/Makefile_insert
include golden/hid_png2/Makefile_insert
include golden/hid_png3/Makefile_insert

include golden/MinMaskGap/Makefile_insert

missing_test:
	@echo "Test environment was found to be incomplete at configuration time,"
	@echo "can't run regression tests. Check configure output to find out which"
	@echo "tools are missing."
	@false

# these are created by 'make check'
clean-local:
	rm -rf outputs
