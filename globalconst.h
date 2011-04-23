/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 *  RCS: $Id$
 */

/* global constants
 * most of these values are also required by files outside the source tree
 * (manuals...)
 */

#ifndef	__GLOBALCONST_INCLUDED__
#define	__GLOBALCONST_INCLUDED__

#include <limits.h>

/* ---------------------------------------------------------------------------
 * some file-, directory- and environment names
 */
#define	EMERGENCY_NAME		"PCB.%.8i.save"	        /* %i --> pid */
#define	BACKUP_NAME		"PCB.%.8i.backup"	/* %i --> pid */

/* ---------------------------------------------------------------------------
 * some default values
 */
#define	DEFAULT_SIZE		"7000x5000"	/* default layout size */
#define	DEFAULT_MEDIASIZE	"a4"		/* default output media */
#define	DEFAULT_CELLSIZE	50		/* default cell size for symbols */
#define CLICK_TIME		200		/* default time for click expiration */
#define SCROLL_TIME 		25		/* time between scrolls when drawing beyond border */
#define COLUMNS			8		/* number of columns for found pin report */

/* ---------------------------------------------------------------------------
 * frame between the groundplane and the copper
 */
#define	GROUNDPLANEFRAME	15	/* unit == mil */
#define MASKFRAME               3       /* unit == mil */

/* ---------------------------------------------------------------------------
 * some limit specifications
 */
#define LARGE_VALUE		10000000 /* maximum extent of board and elements in 1/100 mil */
 
#define	MAX_LAYER		16	/* max number of layer, check source */
					/* code for more changes, a *lot* more changes */
#define DEF_LAYER		8	/* default number of layers for new boards */
#define NUM_STYLES		4
#define	MIN_LINESIZE		1	/* thickness of lines in 1/100000'' */
#define	MAX_LINESIZE		LARGE_VALUE
#define	MIN_TEXTSCALE		10	/* scaling of text objects in percent */
#define	MAX_TEXTSCALE		10000
#define	MIN_PINORVIASIZE	2000	/* size of a pin or via in 1/100 mils */
#define	MIN_PINORVIAHOLE	400	/* size of a pins or vias drilling hole */
#define	MAX_PINORVIASIZE	LARGE_VALUE
#define	MIN_PINORVIACOPPER	400	/* min difference outer-inner diameter */
#define	MIN_PADSIZE		100	/* min size of a pad */
#define	MAX_PADSIZE		LARGE_VALUE   /* max size of a pad */
#define	MIN_DRC_VALUE		10
#define	MAX_DRC_VALUE		50000
#define	MIN_DRC_SILK		100
#define	MAX_DRC_SILK		3000
#define	MIN_DRC_DRILL		100
#define	MAX_DRC_DRILL		5000
#define	MIN_DRC_RING		0
#define	MAX_DRC_RING		10000
#define	MIN_GRID		1	/* min grid in 1/100 mils */
#define	MAX_GRID		100000 /* max grid in 1/100 mils */ 
#define	MAX_FONTPOSITION	255	/* upper limit of characters in my font */

#define	MAX_COORD		LARGE_VALUE	/* coordinate limits */
#define	MIN_SIZE		1000	/* lowest width and height of the board */
#define	MAX_BUFFER		5	/* number of pastebuffers */
					/* additional changes in menu.c are */
					/* also required to select more buffers */

#define	DEFAULT_DRILLINGHOLE	40	/* default inner/outer ratio for */
					/* pins/vias in percent */

#if MAX_LINESIZE > MAX_PINORVIASIZE	/* maximum size value */
#define	MAX_SIZE	MAX_LINESIZE
#else
#define	MAX_SIZE	MAX_PINORVIASIZE
#endif

#ifndef	MAXPATHLEN			/* maximum path length */
#ifdef	PATH_MAX
#define	MAXPATHLEN	PATH_MAX
#else
#define	MAXPATHLEN	2048
#endif
#endif

#define	MAX_LINE_POINT_DISTANCE		0	/* maximum distance when searching */
						/* line points */
#define	MAX_POLYGON_POINT_DISTANCE	0	/* maximum distance when searching */
						/* polygon points */
#define	MAX_ELEMENTNAMES		3	/* number of supported names of */
						/* an element */
#define	MAX_LIBRARY_LINE_LENGTH		255	/* maximum line length in the */
						/* library-description file */
#define MAX_NETLIST_LINE_LENGTH		255	/* maximum line length for netlist files */
#define	MAX_MODESTACK_DEPTH		16	/* maximum depth of mode stack */
#define	MAX_CROSSHAIRSTACK_DEPTH	16	/* maximum depth of state stack */
#define	MIN_GRID_DISTANCE		4	/* minimum distance between point */
						/* to enable grid drawing */
	/* size of diamond element mark */
#define EMARK_SIZE	1000
#define GBX_MAXAPERTURECOUNT	2560
#endif
