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

/* global source constants
 */

#ifndef	__CONST_INCLUDED__
#define	__CONST_INCLUDED__

#include <limits.h>

#include "globalconst.h"

/* ---------------------------------------------------------------------------
 * the layer-numbers of the two additional special layers
 * 'component' and 'solder'. The offset of MAX_LAYER is not added
 */
#define	SOLDER_LAYER		0
#define	COMPONENT_LAYER		1

/* ---------------------------------------------------------------------------
 * misc constants
 */
#define	MARK_SIZE		50		/* mark size of elements */
#define	UNDO_WARNING_SIZE	(1024*1024)	/* warning limit of undo */
#define	USERMEDIANAME		"user defined"	/* label of default media */

/* ---------------------------------------------------------------------------
 * some math constants
 */
#ifndef	M_PI
#define	M_PI			3.14159265358979323846
#endif
#define	M180			(M_PI/180.0)
#define RAD_TO_DEG		(180.0/M_PI)
#define	TAN_22_5_DEGREE_2	0.207106781	/* 0.5*tan(22.5) */
#define COS_22_5_DEGREE		0.923879533	/* cos(22.5) */
#define	TAN_30_DEGREE		0.577350269	/* tan(30) */
#define	TAN_60_DEGREE		1.732050808	/* tan(60) */
#define SQRT2OVER2		0.707106781	/* 1/sqrt(2) */
#define MIL_TO_MM               0.025400000
#define MM_TO_MIL               39.37007874

/* ---------------------------------------------------------------------------
 * modes
 */
#define	NO_MODE			0	/* no mode selected */
#define	VIA_MODE		1	/* draw vias */
#define	LINE_MODE		2	/* draw lines */
#define	RECTANGLE_MODE		3	/* create rectangles */
#define	POLYGON_MODE		4	/* draw filled polygons */
#define	PASTEBUFFER_MODE	5	/* paste objects from buffer */
#define	TEXT_MODE		6	/* create text objects */
#define	ROTATE_MODE		102	/* rotate objects */
#define	REMOVE_MODE		103	/* remove objects */
#define	MOVE_MODE		104	/* move objects */
#define	COPY_MODE		105	/* copy objects */
#define	INSERTPOINT_MODE	106	/* insert point into line/polygon */
#define	RUBBERBANDMOVE_MODE	107	/* move objects and attached lines */
#define THERMAL_MODE            108     /* toggle thermal layer flag */
#define ARC_MODE                109     /* draw arcs */
#define ARROW_MODE		110	/* selection with arrow mode */
#define PAN_MODE                0       /* same as no mode */
#define LOCK_MODE               111     /* lock/unlock objects */

/* ---------------------------------------------------------------------------
 * object flags
 */
#define	OBJ_FLAGS		0xffffffff	/* all used flags */
#define	NOFLAG			0x0000
#define	PINFLAG			0x0001	/* is a pin */
#define	VIAFLAG			0x0002	/* is a via */
#define	FOUNDFLAG		0x0004	/* used by 'FindConnection()' */
#define HOLEFLAG		0x0008  /* pin or via is only a hole */
#define RATFLAG                 0x0010          /* indicates line is a rat line */
#define PININPOLYFLAG           0x0010          /* pin found inside poly - same as */
                                                /* rat line since not used on lines */
#define CLEARPOLYFLAG           0x0010          /* pins/vias clear these polygons */
#define HIDENAMEFLAG		0x0010	/* hide the element name */
#define	DISPLAYNAMEFLAG		0x0020	/* display the names of pins/pads */
					/* of an element */
#define CLEARLINEFLAG		0x0020	/* line doesn't touch polygons */
#define	SELECTEDFLAG		0x0040	/* object has been selected */
#define	ONSOLDERFLAG		0x0080	/* element is on bottom side */
#define AUTOFLAG		0x0080	/* line/via created by auto-router */
#define	SQUAREFLAG		0x0100	/* pin is square, not round */
#define RUBBERENDFLAG		0x0200	/* indicates one end already rubber */
					/* banding same as warn flag*/
					/* since pins/pads won't use it */
#define WARNFLAG		0x0200          /* Warning for pin/via/pad */
#define USETHERMALFLAG		0x0400          /* draw pin, via with thermal fingers */
#define ONSILKFLAG              0x0400  /* old files use this to indicate silk */
#define OCTAGONFLAG		0x0800		/* draw pin/via as octagon instead of round */
#define EDGE2FLAG               0x0800  /* Padr.Point2 is closer to outside edge */
#define DRCFLAG			0x1000		/* flag like FOUND flag for DRC checking */
#define LOCKFLAG                0x2000  /* object locked in place */
#define ALLTHERMFLAGS		0xff0000        /* 8 flags indicating pin/via connects to polys */
#define L0THERMFLAG           0x010000        /* Pin/Via connects to Polygons on Layer 0 */
#define L1THERMFLAG           0x020000        /* Pin/Via connects to Polygons on Layer 1 */
#define L2THERMFLAG           0x040000        /* Pin/Via connects to Polygons on Layer 2 */
#define L3THERMFLAG           0x080000        /* Pin/Via connects to Polygons on Layer 3 */
#define L4THERMFLAG           0x100000        /* Pin/Via connects to Polygons on Layer 4 */
#define L5THERMFLAG           0x200000        /* Pin/Via connects to Polygons on Layer 5 */
#define L6THERMFLAG           0x400000        /* Pin/Via connects to Polygons on Layer 6 */
#define L7THERMFLAG           0x800000        /* Pin/Via connects to Polygons on Layer 7 */

#define ALLPIPFLAGS           0xff000000      /* 8 flags indicating if pin/via is inside poly */
#define L0PIPFLAG             0x01000000      /* Pin/Via is inside polygon on Layer 0 */ 
#define L1PIPFLAG             0x02000000      /* Pin/Via is inside polygon on Layer 1 */
#define L2PIPFLAG             0x04000000      /* Pin/Via is inside polygon on Layer 2 */  
#define L3PIPFLAG             0x08000000      /* Pin/Via is inside polygon on Layer 3 */  
#define L4PIPFLAG             0x10000000      /* Pin/Via is inside polygon on Layer 4 */
#define L5PIPFLAG             0x20000000      /* Pin/Via is inside polygon on Layer 5 */
#define L6PIPFLAG             0x40000000      /* Pin/Via is inside polygon on Layer 6 */
#define L7PIPFLAG             0x80000000      /* Pin/Via is inside polygon on Layer 7 */

/* ---------------------------------------------------------------------------
 * PCB flags
 */
#define	PCB_FLAGS		0x3f71	/* all used flags */
#define SHOWNUMBERFLAG          0x0001  /* pinout displays pin numbers instead of names */
#define RUBBERBANDFLAG		0x0010	/* do all move, mirror, rotate with */
					/* rubberband connections */
#define	DESCRIPTIONFLAG		0x0020	/* display description of elements */
#define	NAMEONPCBFLAG		0x0040	/* display name of an element */
#define	ALLDIRECTIONFLAG	0x0100	/* enable 'all-direction' lines */
#define SWAPSTARTDIRFLAG        0x0200  /* switch starting angle after each click */
#define UNIQUENAMEFLAG		0x0400	/* force unique names on board */
#define CLEARNEWFLAG		0x0800  /* new lines/arc clear polygons */
#define SNAPPINFLAG             0x1000  /* crosshair snaps to pins and pads */
#define SHOWMASKFLAG            0x2000  /* show the solder mask layer */
#define THINDRAWFLAG            0x4000  /* draw with thin lines */

/* ---------------------------------------------------------------------------
 * object types
 */
#define	NO_TYPE			0x0000	/* no object */
#define	VIA_TYPE		0x0001
#define	ELEMENT_TYPE		0x0002
#define	LINE_TYPE		0x0004
#define	POLYGON_TYPE		0x0008
#define	TEXT_TYPE		0x0010
#define RATLINE_TYPE		0x0020

#define	PIN_TYPE		0x0100	/* objects that are part */
#define	PAD_TYPE		0x0200	/* 'pin' of SMD element */
#define	ELEMENTNAME_TYPE	0x0400	/* of others */
#define	POLYGONPOINT_TYPE	0x0800
#define	LINEPOINT_TYPE		0x1000
#define ELEMENTLINE_TYPE        0x2000
#define ARC_TYPE                0x4000
#define ELEMENTARC_TYPE		0x8000

#define PIN_TYPES     VIA_TYPE | PIN_TYPE
#define LOCK_TYPES    VIA_TYPE | LINE_TYPE | ARC_TYPE | POLYGON_TYPE | ELEMENT_TYPE \
                      | TEXT_TYPE | ELEMENTNAME_TYPE

#define	ALL_TYPES		-1	/* all bits set */

/* ---------------------------------------------------------------------------
 * define some standard layouts for childs of a form widget
 */
#define	LAYOUT_TOP		XtNleft, XtChainLeft,	\
				XtNright, XtChainLeft,	\
				XtNtop, XtChainTop,	\
				XtNbottom, XtChainTop
#define	LAYOUT_BOTTOM		XtNleft, XtChainLeft,	\
				XtNright, XtChainLeft,	\
				XtNtop, XtChainBottom,	\
				XtNbottom, XtChainBottom
#define	LAYOUT_BOTTOM_RIGHT	XtNleft, XtChainLeft,	\
				XtNright, XtChainRight,	\
				XtNtop, XtChainBottom,	\
				XtNbottom, XtChainBottom
#define	LAYOUT_LEFT		XtNleft, XtChainLeft,	\
				XtNright, XtChainLeft,	\
				XtNtop, XtChainTop,	\
				XtNbottom, XtChainBottom
#define	LAYOUT_NORMAL		XtNleft, XtChainLeft,	\
				XtNright, XtChainRight,	\
				XtNtop, XtChainTop,	\
				XtNbottom, XtChainBottom

#endif
