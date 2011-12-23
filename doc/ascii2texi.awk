#!/usr/bin/awk -f

BEGIN {
	first = 1;
	col = 1;
}

/^[ \t]*#/ {
	# skip comment lines
	next;
}

/^[ \t]*$/ {
	# skip blank
	next;
}

# we do this 'first' song and dance because variables set
# on the command line are not available in the BEGIN section
first == 1 {
	first = 0;
	printf("@c Generated file.  Do not edit directly\n");
	printf("@multitable @columnfractions ");
	for(i = 1 ; i <= 2*ncol ; i = i + 1) {
		printf("%.3g ", 0.5 / ncol);
	}
	printf("\n");

	printf("@item ");
	for(i = 1 ; i <= ncol ; i = i + 1) {
		if( i > 1 ) { printf("@tab "); }
		printf("Drill @tab Diameter ");
	}
	printf("\n");

	printf("@item ");
	for(i = 1 ; i <= ncol ; i = i + 1) {
		if( i > 1 ) { printf("@tab "); }
		printf("Size @tab (inches) ");
	}
	printf("\n");

	printf("\n");
}

{	
	if( col == 1 ) {
		printf("@item ");
	} else {
		printf("@tab ");
	}
	drl = $1;
	dia = $2;
	gsub(/_/, " ", drl);
	printf("%s @tab %s ", drl, dia);
	col = col + 1;
	if( col > ncol ) {
		col = 1;
		printf("\n");
	}
}

END {
	while( (col > 1) && (col <= ncol ) ) {
		printf("@tab @tab ");
		col = col + 1;
	}

	printf("@end multitable\n\n");
}



