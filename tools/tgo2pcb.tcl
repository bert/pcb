#! /usr/bin/tclsh
#
# tango to PCB netlist converter
# usage: tgo2pcb input > output
#
# Copyright 1998, Ingo Cyliax, Derivation Systems, Inc.
# Email: cyliax@derivation.com
#

set nets() ""

#
# read a tango netlist and extract the essentials we need, this
# has been tested with OrCad's tango netlister
#
proc rdtgo { file } {
	set fd [open $file r]
	set line ""
	set args ""
	set ln 0
	set lch ""
	set net ""
	set pin ""
	set ref ""
	global nets cnvt
	while { 1 } {
		set n [gets $fd line]
		if { $n == -1 } { break }
		incr ln 1

		set ch [string index $line 0]

		if { $ch == "\["} {
			set lch $ch
			set ln 0
		}
		if { $ch == "]"} {
			set lch $ch
			set ln 0
		}
		if { $ch == "("} {
			set lch $ch
			set ln 0
		}
		if { $ch == ")"} {
			set lch $ch
			set ln 0
		}
		if { $ch != "(" && $lch == "(" && $ln == "1"} {
			set net $line
		}
		if { $ch != "(" && $lch == "(" && $ln != "1"} {
			set xx [split $line ,]
			set pin [lindex $xx 1]
			set ref [lindex $xx 0]
			if { $cnvt($pin) != "" } {
				set pin $cnvt($pin)
			}
			lappend nets($net) "$ref-$pin"
		}
	}
	close $fd
}

#
# write out a PCB netlist
#
proc wrpcb { } {
	global nets
	foreach i [array names nets] {
		if { $nets($i) != "" } {
			puts "$i $nets($i)"
		}
	}
}

#
# pins 1-99 convert to 1-99, all input pins are converted in this array
#
set pin 1
for { set i 1 } { $i <= 99 } { incr i } {
	set cnvt($i) $pin
	incr pin
}

#
# read the input file and convert to internal netlist
#
rdtgo [lindex $argv 0]

#
# convert and write to output
#
wrpcb

exit
