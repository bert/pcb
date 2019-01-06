#!/bin/bash

PCB=../../../pcbtest.sh ./run_tests.sh --regen DRCTests

rm golden/DRCTests/*-flags-after.txt
rm golden/DRCTests/*-flags-before.txt
rm golden/DRCTests/*.pcb
rm golden/DRCTests/drctest.script

