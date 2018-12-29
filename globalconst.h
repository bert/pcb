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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* global constants
 * most of these values are also required by files outside the source tree
 * (manuals...)
 */

#ifndef	__GLOBALCONST_INCLUDED__
#define	__GLOBALCONST_INCLUDED__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

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
#define	GROUNDPLANEFRAME	MIL_TO_COORD(15)
#define MASKFRAME               MIL_TO_COORD(3)

/* ---------------------------------------------------------------------------
 * some limit specifications
 */
#define LARGE_VALUE		(COORD_MAX / 2 - 1) /* maximum extent of board and elements */
 
#define MAX_LAYER           16         /* max number of copper layers */
#define MAX_GROUP           MAX_LAYER  /* max number of layer groups */
#define NUM_STYLES		4
#define	MIN_LINESIZE		MIL_TO_COORD(0.01)	/* thickness of lines */
#define	MAX_LINESIZE		LARGE_VALUE
#define	MIN_TEXTSCALE		10	/* scaling of text objects in percent */
#define	MAX_TEXTSCALE		10000
#define	MIN_PINORVIASIZE	MIL_TO_COORD(20)	/* size of a pin or via */
#define	MIN_PINORVIAHOLE	MIL_TO_COORD(4)	/* size of a pins or vias drilling hole */
#define	MAX_PINORVIASIZE	LARGE_VALUE
#define	MIN_PADSIZE		MIL_TO_COORD(1)	/* min size of a pad */
#define	MAX_PADSIZE		LARGE_VALUE   /* max size of a pad */
#define	MIN_DRC_VALUE		MIL_TO_COORD(0.1)
#define	MAX_DRC_VALUE		MIL_TO_COORD(500)
#define	MIN_DRC_SILK		MIL_TO_COORD(1)
#define	MAX_DRC_SILK		MIL_TO_COORD(30)
#define	MIN_DRC_DRILL		MIL_TO_COORD(1)
#define	MAX_DRC_DRILL		MIL_TO_COORD(50)
#define	MIN_DRC_RING		0
#define	MAX_DRC_RING		MIL_TO_COORD(100)
#define	MIN_GRID		1
#define	MAX_GRID		MIL_TO_COORD(1000)
#define	MAX_FONTPOSITION	255	/* upper limit of characters in my font */

#define	MAX_COORD		LARGE_VALUE	/* coordinate limits */
#define	MIN_SIZE		MIL_TO_COORD(10)	/* lowest width and height of the board */
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
#define	MAX_ARC_POINT_DISTANCE		0	/* maximum distance when searching */
						/* arc points */
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
#define EMARK_SIZE	MIL_TO_COORD (10)

/* (Approximate) capheight size of the default PCB font */
#define FONT_CAPHEIGHT  MIL_TO_COORD (45)
#endif
