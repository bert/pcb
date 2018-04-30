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
  inputs/gsvit_board.pcb \
  inputs/ipcd356_board.pcb \
  inputs/ipcd356_cust0.pcb \
  inputs/ipcd356_cust1.pcb \
  inputs/ipcd356_via_1.pcb \
  inputs/ipcd356_via_2.pcb \
  inputs/ipcd356_via_3.pcb \
  inputs/ipcd356_via_4.pcb \
  inputs/ipcd356_via_5.pcb \
  inputs/ipcd356_via_6.pcb \
  inputs/ipcd356_via_7.pcb \
  inputs/ipcd356_via_8.pcb \
  inputs/ipcd356_hole_1.pcb \
  inputs/ipcd356_hole_2.pcb \
  inputs/ipcd356_smt_1.pcb \
  inputs/ipcd356_smt_2.pcb \
  inputs/ipcd356_smt_3.pcb \
  inputs/minmaskgap.pcb \
  inputs/minmaskgap.script \
  inputs/nelma_board.pcb \
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
  golden/hid_gsvit1/gsvit_board.top.png \
  golden/hid_gsvit2/gsvit_board.bottom.png \
  golden/hid_gsvit3/gsvit_board.outline.png \
  golden/hid_gsvit4/gsvit_board.xem \
  golden/hid_ipcd3561/ipcd356_board.net \
  golden/hid_ipcd3562/ipcd356_cust0.net \
  golden/hid_ipcd3563/ipcd356_cust1.net \
  golden/hid_ipcd3564/ipcd356_via_1.net \
  golden/hid_ipcd3565/ipcd356_via_2.net \
  golden/hid_ipcd3566/ipcd356_via_3.net \
  golden/hid_ipcd3567/ipcd356_via_4.net \
  golden/hid_ipcd3568/ipcd356_via_5.net \
  golden/hid_ipcd3569/ipcd356_via_6.net \
  golden/hid_ipcd35610/ipcd356_via_7.net \
  golden/hid_ipcd35611/ipcd356_via_8.net \
  golden/hid_ipcd35612/ipcd356_hole_1.net \
  golden/hid_ipcd35613/ipcd356_hole_2.net \
  golden/hid_ipcd35614/ipcd356_smt_1.net \
  golden/hid_ipcd35615/ipcd356_smt_2.net \
  golden/hid_ipcd35616/ipcd356_smt_3.net \
  golden/hid_nelma1/nelma_board.top.png \
  golden/hid_nelma2/nelma_board.bottom.png \
  golden/hid_nelma3/nelma_board.outline.png \
  golden/hid_nelma4/nelma_board.em \
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
