#!/usr/bin/awk -f
#
# $Id$
#
# Script to regenerate geda.m4 from geda.inc
#
# Usage:
#
#  awk -f gen_geda_m4.awk geda.inc > geda.m4
#

BEGIN {
}

/^[ \t]*\#[ \t]*\$Id$]*\$/ {
	id = substr($0, index($0, "Id:"));
	ind = index(id, "Exp $");
	id = substr(id, 1, ind+3);
	printf("divert(-1)\n");
	printf("#\n");
	printf("# NOTE: Auto-generated. Do not change.\n");
	printf("# Generated from:\n");
	printf("#  %s\n", id);
	next;
}

/^\#\#/ {
	descr = $0;
	ind = index(descr, $2);
	descr = substr(descr, ind);
	printf("#\n");
	next;
}


/^[ \t]*define/ {
	pkg = $1;
	ind = index(pkg, "PKG");
	pkg = substr(pkg, ind+4);
	ind = index(pkg, "'");
	pkg = substr(pkg, 1, ind-1);
	printf("define(`Description_geda_%s',\t``%s'')\n", pkg, descr);
}

END {
	printf("divert(0)dnl\n");
}

