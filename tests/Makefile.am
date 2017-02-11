## -*- makefile -*-

TESTS_ENVIRONMENT=	\
	GERBV=${GERBV} \
	IM_ANIMATE=${IM_ANIMATE} \
	IM_COMPARE=${IM_COMPARE} \
	IM_COMPOSITE=${IM_COMPOSITE} \
	IM_CONVERT=${IM_CONVERT} \
	IM_DISPLAY=${IM_DISPLAY} \
	IM_MONTAGE=${IM_MONTAGE} \
	XHOST=${XHOST}

RUN_TESTS=	run_tests.sh

check_SCRIPTS=		${RUN_TESTS}

# if we have the required tools, then run the regression test
if HAVE_TEST_TOOLS
  TESTS = ${RUN_TESTS}
else
  TESTS = missing_test
endif

# Instead of recursing into each of the many subdirectories, simply define
# the relevant files here in one place. This is considerably faster and makes
# changes to top level configure.ac unneccessary when adding new tests.
EXTRA_DIST = \
  ${RUN_TESTS} \
  tests.list \
  README.txt \
  inputs/bom_general.pcb \
  inputs/buried.pcb \
  inputs/circles.pcb \
  inputs/gcode_oneline.pcb \
  inputs/gerber_arcs.pcb \
  inputs/gerber_oneline.pcb \
  inputs/ipcd356_board.pcb \
  inputs/minmaskgap.pcb \
  inputs/minmaskgap.script \
  golden/hid_bom1/bom_general.bom \
  golden/hid_bom1/bom_general.xy \
  golden/hid_bom2/test.bom \
  golden/hid_bom2/bom_general.xy \
  golden/hid_bom3/bom_general.bom \
  golden/hid_bom3/test.xy \
  golden/hid_bom4/bom_general.bom \
  golden/hid_bom4/bom_general.xy \
  golden/hid_gerber1/gerber_oneline.top.gbr \
  golden/hid_gerber1/gerber_oneline.fab.gbr \
  golden/hid_gerber1/gerber_oneline.bottom.gbr \
  golden/hid_gerber1/gerber_oneline.plated-drill.cnc \
  golden/hid_gerber2/out.fab.gbr \
  golden/hid_gerber2/out.top.gbr \
  golden/hid_gerber2/out.bottom.gbr \
  golden/hid_gerber2/out.plated-drill.cnc \
  golden/hid_gerber3/arcs.group1.gbr \
  golden/hid_gerber3/arcs.top.gbr \
  golden/hid_gerber3/arcs.group4.gbr \
  golden/hid_gerber3/arcs.fab.gbr \
  golden/hid_gerber3/arcs.bottom.gbr \
  golden/hid_gerber3/arcs.plated-drill.cnc \
  golden/hid_gerber4/buried.bottom.gbr \
  golden/hid_gerber4/buried.fab.gbr \
  golden/hid_gerber4/buried.group2.gbr \
  golden/hid_gerber4/buried.group4.gbr \
  golden/hid_gerber4/buried.group7.gbr \
  golden/hid_gerber4/buried.plated-drill.cnc \
  golden/hid_gerber4/buried.plated-drill_03-08.cnc \
  golden/hid_gerber4/buried.top.gbr \
  golden/hid_gcode1/gcode_oneline-bottom.gcode \
  golden/hid_gcode1/gcode_oneline-top.gcode \
  golden/hid_gcode1/gcode_oneline-outline.gcode \
  golden/hid_gcode1/gcode_oneline-0.8890.drill.gcode \
  golden/hid_gcode2/out-bottom.gcode \
  golden/hid_gcode2/out-top.gcode \
  golden/hid_gcode2/out-outline.gcode \
  golden/hid_gcode2/out-0.8890.drill.gcode \
  golden/hid_gcode3/gcode_oneline-bottom.gcode \
  golden/hid_gcode3/gcode_oneline-top.gcode \
  golden/hid_gcode3/gcode_oneline-outline.gcode \
  golden/hid_gcode3/gcode_oneline-0.8890.drill.gcode \
  golden/hid_gcode4/gcode_oneline-bottom.gcode \
  golden/hid_gcode4/gcode_oneline-top.gcode \
  golden/hid_gcode4/gcode_oneline-outline.gcode \
  golden/hid_gcode4/gcode_oneline-0.8890.drill.gcode \
  golden/hid_gcode5/gcode_oneline-bottom.gcode \
  golden/hid_gcode5/gcode_oneline-top.gcode \
  golden/hid_gcode5/gcode_oneline-outline.gcode \
  golden/hid_gcode5/gcode_oneline-0.8890.drill.gcode \
  golden/hid_gcode6/gcode_oneline-bottom.gcode \
  golden/hid_gcode6/gcode_oneline-top.gcode \
  golden/hid_gcode6/gcode_oneline-outline.gcode \
  golden/hid_gcode6/gcode_oneline-0.8890.drill.gcode \
  golden/hid_gcode7/gcode_oneline-bottom.gcode \
  golden/hid_gcode7/gcode_oneline-top.gcode \
  golden/hid_gcode7/gcode_oneline-outline.gcode \
  golden/hid_gcode7/gcode_oneline-0.8890.drill.gcode \
  golden/hid_gcode8/gcode_oneline-bottom.gcode \
  golden/hid_gcode8/gcode_oneline-top.gcode \
  golden/hid_gcode8/gcode_oneline-outline.gcode \
  golden/hid_gcode8/gcode_oneline-0.8890.drill.gcode \
  golden/hid_gcode9/gcode_oneline-bottom.gcode \
  golden/hid_gcode9/gcode_oneline-top.gcode \
  golden/hid_gcode9/gcode_oneline-outline.gcode \
  golden/hid_gcode9/gcode_oneline-drillmill.gcode \
  golden/hid_gcode10/gcode_oneline-bottom.gcode \
  golden/hid_gcode10/gcode_oneline-top.gcode \
  golden/hid_gcode10/gcode_oneline-outline.gcode \
  golden/hid_gcode10/gcode_oneline-drillmill.gcode \
  golden/hid_gcode11/gcode_oneline-bottom.gcode \
  golden/hid_gcode11/gcode_oneline-top.gcode \
  golden/hid_gcode11/gcode_oneline-outline.gcode \
  golden/hid_gcode11/gcode_oneline-0.0350.drill.gcode \
  golden/hid_ipcd3561/ipcd356_board.net \
  golden/hid_png1/gerber_oneline.png \
  golden/hid_png2/myfile.png \
  golden/hid_png3/gerber_oneline.png \
  golden/hid_ps1/circles.ps \
  golden/hid_ps2/buried.ps \
  golden/MinMaskGap/minmaskgap.pcb

.PHONY: missing_test
missing_test:
	@echo "Test environment was found to be incomplete at configuration time,"
	@echo "can't run regression tests. Check configure output to find out which"
	@echo "tools are missing."
	@false

# these are created by 'make check'
clean-local:
	rm -rf outputs
