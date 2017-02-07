#!/bin/sh

# This script is used to extract all elements from an "oldlib" (M4)
# style library and place them in individual "newlib" style files.

outd=/tmp/newlib
contents=pcblib.contents
AWK=${AWK:-awk}
PCB=${PCB:-pcb}

usage() {
cat << EOF
Usage:
	$0 [-h | --help]
	$0 [-v | --version]
	$0 [-c|--contents contents_file] [-o|--output output_directory] [-p|--png] [-d|--dpi]

Extracts all footprints from an m4 library and creates a "newlib" style
library.

The following options are supported:

    -a | --awk awk       : Specifies the awk implementation to use.  Defaults to "${AWK}".

    -c | --contents file : Specifies the contents file to be use as an input.
                           Default is "${contents}".

    -d | --dpi           : Specifies that the png output should use a fixed pixels
                           per inch scaling instead of a fixed maximum size.  This
                           option is useful when comparing the before and after footprints
                           when making footprint library changes.

    -h | --help          : Outputs this message and exits.

    -o | --output dir    : Specifies the directory that the newlib library will be
                           written to.  This directory must exist and be empty.
                           Default is "${outd}".

    -P | --pcb pcb       : Specifies the pcb binary to use for creating png previews.  Defaults to
			   "${PCB}"

    -p | --png           : Generates png previews for all the footprints.

    -v | --version       : Displays the version of this script and exits.

EOF
}

version() {
	$AWK '/# [\$]Id:.*$/ {sub(/,v/,""); \
		print $3 " Version "$4", "$5}' $0
}

do_png=0
png_flag="--xy-max 200"

while test $# -gt 0 ; do
	case $1 in
		-a|--awk )
			AWK="$2"
			shift 2
			;;

		-c|--contents )
			contents=$2
			shift 2
			;;

		-d|--dpi )
			png_flag="--dpi 1000"
			shift
			;;

		-h|--help )
			usage
			exit 0
			;;

		-o|--output )
			outd=$2
			shift 2
			;;

		-P|--pcb )
			PCB="$2"
			shift 2
			;;

		-p|--png )
			do_png=1
			shift
			;;

		-v|--version )
			version
			exit 0
			;;

		-* )
			echo "ERROR:  $0: Unknown option $1"
			usage
			exit 1
			;;

		* )
			break
			;;
	esac
done


if test -d ${outd} ; then
	echo "Output directory ${outd} already exists"
	exit 1
else
	mkdir -p ${outd}
fi
outd_full="`cd $outd && pwd`"

$AWK '

BEGIN {
	first = 1;
	libind = "";
}

# we have to use this trick because variables like outd are not yet defined
# in BEGIN.
first == 1 {
	brokenurl = "broken.html";
	broken = outd "/" brokenurl;
	print "" > broken;

	ind = outd "/index.html";

	print "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">" > ind;
	print "<HTML><HEAD><TITLE>PCB Footprint Library</TITLE></HEAD>" >> ind;
	print "<BODY bgcolor=\"#FFFFFF\" text=\"#000000\">" >> ind;
	print "<H1>PCB Footprint Library</H1>" >> ind;
	print "<UL>" >> ind;
	print "" >> ind;

	print "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">" > broken;
	print "<HTML><BODY>" >> broken;
	print "<TABLE BORDER=2>" >> broken;
	print "<TR><TD><EM>Library</EM></TD>" >> broken;
	print "    <TD><EM>Comment</EM></TD>" >> broken;
	print "    <TD><EM>Footprint Name</EM></TD>" >> broken;
	print "    <TD><EM>Broken Command</EM></TD>" >> broken;
	print "</TR>" >> broken;
	print "" >> broken;

	first = 0;
}

# we are starting a new library
/^TYPE=/ {
	finish_libind();
	lib=$0;
	gsub(/TYPE=~/, "", lib);
	txtdir = lib;
	urldir = lib;
	gsub(/ /, "%20", urldir); 

	libind =  outd "/" lib "/index.html";
	#gsub(/ /, "\\ ", libind);

	dir = outd "/" lib ;
	gsub(/ /,"\\ ", dir); 
	print "Processing library: " lib " and creating " libind;
	system("mkdir -p " dir);

	print "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">" > libind;
	print "<HTML><HEAD><TITLE>PCB " lib " Footprint Library</TITLE></HEAD>" >> libind;
	print "<BODY bgcolor=\"#FFFFFF\" text=\"#000000\">" >> libind;
	print "<H1>PCB " lib " Footprint Library</H1>" >> libind;

	print "<TABLE BORDER=2>" >> libind;
	print "<TR>" >> libind;
	print "    <TD><EM>Comment</EM></TD>" >> libind;
	print "    <TD><EM>Footprint Name</EM></TD>" >> libind;
	print "</TR>" >> libind;
	print "" >> libind;

	print "<LI><A HREF=\"" txtdir "/index.html\">~" lib "</A></LI>" >> ind;

	next;
}

{
	line=$0;
	split(line, a, "[:]");

	template = a[1];
	package = a[2];
	comp = a[3];
	comment = a[4];

	# pick out the name of the footprint
	loc = match(comment, /.*\[.*\]/);
	if(loc > 0) {
		fp[0] = substr(comment, RSTART, RLENGTH);
		loc2 =  index(fp[0], "[");
		fp[1] = substr(fp[0], 0, loc2-1);
		loc3 = length(fp[0]) - (loc2 + 1);
		fp[2] = substr(fp[0], loc2+1, loc3);
	} else {
		printf("Bad line (could not extract component name): %s\n", $0) > "/dev/stderr";
		exit(1);
	}
	comp = fp[2];
	comment = a[3] ", " fp[1];

	txtcomp = comp;
	urlcomp = comp;

	# escape the spaces in for URLs and also filenames
	gsub(/ /, "%20", urlcomp);
	gsub(/ /, "\\ ", comp);


	# extract the footprint
	# path library template value package
	templ = a[1];
	gsub(/ /, "\\ ", templ);

	pkg = a[2];
	gsub(/ /, "\\ ", pkg);


	# skip the QFP builder menu
	skip = 0;
	if( templ == "menu_qfp" ) {
		cmd1 = "Skipping QFP builder menu";
		rc = 1;
	} else {
		cmd1 = "sh " cmd_path "/QueryLibrary.sh . pcblib " templ " " comp " " pkg;
		cmd = cmd1 " > " dir "/" comp ".fp";
 		rc = system( cmd );
	}

	if( rc != 0) {
		printf("<TR><TD>~%s</TD>\n", lib) >> broken;
		printf("    <TD>%s</TD>\n", comp) >> broken;
		printf("    <TD>%s</TD>\n", comment) >> broken;
		printf("    <TD>%s</TD>\n", cmd1) >> broken;
		printf("</TR>\n") >> broken;

		# no need to go further with this footprint.  It is broken.
		next;
	} else {
		# generate the web index
		printf("  <TR>\n    <TD>%s</TD>\n", comment) >> libind;
		printf("    <TD><A HREF=\"%s.fp\">%s.fp</A>", txtcomp, txtcomp) >> libind;
		if( do_png ) {
			printf("(<A HREF=\"%s.png\">preview</A>)", txtcomp) >> libind;
		}
		printf("</TD>\n  </TR>\n") >> libind;
	}

	# Now create a layout with that element and print it.
	if( do_png ) {
		layout = "temp.pcb" ;
		laytmpl = "footprint.pcb" ;
		compfile = dir "/" comp ".fp";
		pngfile = dir "/" comp ".png";
		compfile2 = compfile;
		gsub(/\\/, "", compfile2);

		printf("   ===> %s\n", compfile);
		printf("") > layout;
		pok = 1;
		while ( (getline < laytmpl) == 1 ) {
			if( $0 ~ /ELEMENT/ ) {
				pok = 0;
			}
			if( pok ) {
				print >> layout ;
			}
		}
		close( laytmpl );

		while( (x = getline < compfile2) == 1 ) {
			print >> layout;
		}
		close( compfile2 );

		pok = 0;
		while( (getline < laytmpl) == 1 ) {
			if( pok ) {
				print >> layout;
			}
			if( $0 ~ /ELEMENT/ ) {
				pok = 1;
			}
		}
		close( laytmpl );
		close( layout );

		cmd = PCB " -x png --outfile temp.png ${png_flag} --only-visible " layout " 2>&1 > /dev/null" ;
		rc = system( cmd );
		if( rc != 0) {
			printf("<TR><TD>~%s</TD>\n", lib) >> broken;
			printf("    <TD>%s</TD>\n", comp) >> broken;
			printf("    <TD>%s</TD>\n", comment) >> broken;
			printf("    <TD>%s</TD>\n", cmd) >> broken;
			printf("</TR>\n") >> broken;
		} else {
			system( "mv temp.png " pngfile " ; rm " layout);
		}
	}
}

END {
	print "" >> ind;
	print "</UL>" >> ind;
	print "" >> ind;
	print "<P>For a list of footprints with either m4 syntax errors" >> ind;
	print "or PCB syntax errors see <A HREF=\"" brokenurl "\">the broken log file.</A></P>" >> ind;
	print "</BODY></HTML>" >> ind;
	close( ind );

	print "" >> broken;
	print "</TABLE>" >> broken;
	print "" >> broken;
	print "</BODY></HTML>" >> broken;
	close( broken );

	finish_libind();
}

function finish_libind() {
	if(libind != "") {
		print "" >> libind;
		print "</TABLE>" >> libind;
		print "" >> libind;
		print "</BODY></HTML>" >> libind;
		close( libind );
	}
}

' cmd_path=./ do_png=$do_png outd="$outd_full" awk=$AWK PCB="${PCB}" $contents



