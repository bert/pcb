/*
 *                            COPYRIGHT
 *
 *  gerbertotk, simple Gerber file to tcl/tk program converter
 *  Copyright (C) 1996 Albert John FitzPatrick III
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Albert John FitzPatrick III/ 8803 Humming Bird Lane/ Quinton, VA 23141/
 *    USA.
 *  ajf_nylorac@acm.org
 *
 */

/* WARNING: This is an alpha version.  Use it with many grains of salt. */

static char Copyright[] = "Copyright (C) 1996 Albert John FitzPatrick III";

/*
 * Example Usage:
 *    cat *.gbr | gerbertotk | wish
 */

#include <string.h>
#include <stdio.h>

typedef long GerberValue;

typedef struct {
	int		gc_inComment;
	GerberValue	gc_d;
	GerberValue	gc_x1;
	GerberValue	gc_y1;
	GerberValue	gc_x2;
	GerberValue	gc_y2;
	GerberValue	gc_i;
	GerberValue	gc_j;
	} GerberContext;

GerberContext gc;

#define gbMaxX 10000L
#define gbMaxY 10000L
#define tkScale 4L
#define tkX(x) (800L * (x) * tkScale / gbMaxX)
#define tkY(y) ((600L * (gbMaxY - (y)) * tkScale / gbMaxY))


do_cmd(GerberContext *gc, int cmd, char *buffer)
{
	GerberValue value = atol(buffer);

	switch (cmd) {
	case 'G':
		switch (value) {
		case 4L:
			gc->gc_inComment = 1;
			break;
		}
		break;
	case 'X':
		gc->gc_x2 = value;
		break;
	case 'Y':
		gc->gc_y2 = value;
		break;
	case 'I':
		gc->gc_i = value;
		break;
	case 'J':
		gc->gc_j = value;
		break;
	case 'D':
		switch (value) {
		case 1L:
			/* Move with light on. */
			printf(".f.c create line %ld %ld %ld %ld\n",
				(long) tkX(gc->gc_x1),
				(long) tkY(gc->gc_y1),
				(long) tkX(gc->gc_x2),
				(long) tkY(gc->gc_y2)
				);
			gc->gc_x1 = gc->gc_x2;
			gc->gc_y1 = gc->gc_y2;
			break;
		case 2L:
			/* Move with light off. */
			gc->gc_x1 = gc->gc_x2;
			gc->gc_y1 = gc->gc_y2;
			break;
		case 3L:
			/* Move with light off.  Flash light on.  Turn light off. */
			printf(".f.c create line %ld %ld %ld %ld\n",
				(long) tkX(gc->gc_x2),
				(long) tkY(gc->gc_y2),
				(long) tkX(gc->gc_x2),
				(long) tkY(gc->gc_y2)
				);
			break;
		default:
			gc->gc_d = value;
			break;
		}
		break;
	case 'M':
		break;
	case '*':
		gc->gc_inComment = 0;
		break;
	case '\0':
		/* FIXME: what if missing on first time?
		fprintf(stderr, "Missing Gerber command character.\n");
		*/
		break;
	}
}


main(int argc, char *argv[])
{
	int ch;
	int cmd;
	char buffer[1024];
	char *bp;

	if (argc != 1) {
		fprintf(stderr, "%s: %s\n",
			argv[0],
			Copyright
			);
		fprintf(stderr, "%s: Usage: %s\n",
			argv[0],
			argv[0]
			);
		exit(1);
	}

	bp = buffer;
	cmd = '\0';
	gc.gc_inComment = 0;
	gc.gc_d = 0L;
	gc.gc_x1 = 0L;
	gc.gc_y1 = 0L;
	gc.gc_x2 = 0L;
	gc.gc_y2 = 0L;
	gc.gc_i = 0L;
	gc.gc_j = 0L;

	printf("button .exit \\\n");
	printf("\t-command exit \\\n");
	printf("\t-text Exit\n");
	printf("frame .f\n");
	printf("canvas .f.c \\\n");
	printf("\t-width 800 \\\n");
	printf("\t-height 600 \\\n");
	printf("\t-scrollregion { 0 0 10000 10000 } \\\n");
	printf("\t-xscroll { .f.h set } \\\n");
	printf("\t-yscroll { .f.v set }\n");
	printf("scrollbar .f.v \\\n");
	printf("\t-orient vertical \\\n");
	printf("\t-command { .f.c yview }\n");
	printf("scrollbar .f.h \\\n");
	printf("\t-orient horizontal \\\n");
	printf("\t-command { .f.c xview }\n");

	printf("\n");
	printf("pack .exit .f \\\n");
	printf("\t-side top \\\n");
	printf("\t-expand yes \\\n");
	printf("\t-fill both\n");
	printf("pack .f.v \\\n");
	printf("\t-side right \\\n");
	printf("\t-fill y\n");
	printf("pack .f.h \\\n");
	printf("\t-side bottom \\\n");
	printf("\t-fill x\n");
	printf("pack .f.c \\\n");
	printf("\t-side left\n");

	printf("\n");

	while ((ch = getchar()) != EOF) {
		if (gc.gc_inComment && ch != '*')
			continue;

		switch (ch) {
		case 'D':
		case 'G':
		case 'X':
		case 'Y':
		case 'I':
		case 'J':
		case 'M':
			do_cmd(&gc, cmd, buffer);
			cmd = ch;
			bp = buffer;
			break;
		case '*':
			/* FIXME: Handle other EOB characters. */
			if (gc.gc_inComment)
				do_cmd(&gc, ch, buffer);
			else
				do_cmd(&gc, cmd, buffer);
			cmd = '\0';
			bp = buffer;
			break;
		case '\r':
			/* FIXME: only after EOB. */
			cmd = '\0';
			bp = buffer;
			break;
		case '\012':
			/* FIXME: only after EOB. */
			cmd = '\0';
			bp = buffer;
			break;
		case '+':
		case '-':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			*bp++ = (char) ch;
			*bp = '\0';
			break;
		default:
			do_cmd(&gc, cmd, buffer);
			cmd = '\0';
			bp = buffer;
			if (! gc.gc_inComment)
				fprintf(stderr, "Unknown Gerber command '%c' (%d).\n",
					(char) ch,
					ch
					);
			break;
		}
	}
	
	exit(0);
}
