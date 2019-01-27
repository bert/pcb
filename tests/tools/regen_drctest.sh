#!/bin/bash

if [ $# -gt 0 ]; then
  tests=$*
else
  tests=`awk -F'|' '/^drc-/ {print $1}' tests.list`
fi

for t in $tests;
do
  echo "Regenerating $t..."
  PCB=../../../pcbtest.sh ./run_tests.sh --regen $t
  rm golden/$t/flags-after.txt
  rm golden/$t/flags-before.txt
  rm golden/$t/*.pcb
  rm golden/$t/drctest.script
  rm -f golden/$t/run_tests.err.log
done

