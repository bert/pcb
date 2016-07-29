/*!
 * \file src/const.h
 *
 * \brief Global source constants.
 *
 * <hr>
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 */

#ifndef	PCB_CONST_H
#define	PCB_CONST_H

#include <limits.h>
#include <math.h>

#include "globalconst.h"

/* ---------------------------------------------------------------------------
 * integer codings for the board sides.
 */
#define BOTTOM_SIDE             0
#define TOP_SIDE                1


/*!
 * Layer Stack.
 *
 * The situation is not ideal, because on top of the ordinary layer stack
 * there are two (both) silk layers. It was a hack to separate these and
 * probably done to easier iterate through non-silk layers, only. As we have
 * layer types now, iterating through any kind of distinct layer type has
 * become simple, see LAYER_TYPE_LOOP() in macro.h.
 *
 * Accordingly, the separation of these two silk layers should go away, they
 * should return to be "normal" layers. One might want to have more than two
 * silk layers, after all.
 *
 * Anyways, here's the current setup:
 *
 *   Copper layer 0             \
 *   Copper layer 1             |
 *   Outline layer              >- in unspecified order
 *   Routing layer              |
 *   (...additional layers...)  /     <== max_copper_layer
 *   Bottom silk layer
 *   Top silk layer                   <== max_copper_layer + SILK_LAYER
 *   (...unused layers...)
 *   (last layer - 2)                 <== MAX_LAYER
 *   (last layer - 1)
 *   (last layer)                     <== MAX_ALL_LAYER
 *                                        ( == MAX_LAYER + SILK_LAYER)
 *
 * With all layers in use (rarely the case), max_copper_layer == MAX_LAYER.
 *
 * \note Position on the layer stack does not decide wether a layer is on the
 *       top side, on the bottom side or in between. Each layer is part of a
 *       layer group, and this group represents the physical layer, like top,
 *       inner or bottom.
 */


/* ---------------------------------------------------------------------------
 * the layer-numbers of the two additional special (silkscreen) layers
 * 'bottom' and 'top'. The offset of MAX_LAYER is not added
 */
#define SILK_LAYER              2
#define BOTTOM_SILK_LAYER       0
#define TOP_SILK_LAYER          1

/* ---------------------------------------------------------------------------
 * the resulting maximum number of layers, including additional silk layers
 */
#define MAX_ALL_LAYER           (MAX_LAYER + SILK_LAYER)

/* ---------------------------------------------------------------------------
 * misc constants
 */
#define	MARK_SIZE		MIL_TO_COORD(50)	/*!< relative marker size */
#define	UNDO_WARNING_SIZE	(1024*1024)	/*!< warning limit of undo */
#define	USERMEDIANAME		"user defined"	/*!< label of default media */

/* ---------------------------------------------------------------------------
 * some math constants
 */
#ifndef	M_PI
#define	M_PI			3.14159265358979323846
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2 		0.707106781	/*!< 1/sqrt(2) */
#endif
#define	M180			(M_PI/180.0)
#define RAD_TO_DEG		(180.0/M_PI)
#define	TAN_22_5_DEGREE_2	0.207106781	/*!< 0.5*tan(22.5) */
#define COS_22_5_DEGREE		0.923879533	/*!< cos(22.5) */
#define	TAN_30_DEGREE		0.577350269	/*!< tan(30) */
#define	TAN_60_DEGREE		1.732050808	/*!< tan(60) */
#define LN_2_OVER_2		0.346573590

/* PCB/physical unit conversions */
#define COORD_TO_MIL(n)	((n) / 25400.0)
#define MIL_TO_COORD(n)	((n) * 25400.0)
#define COORD_TO_MM(n)	((n) / 1000000.0)
#define MM_TO_COORD(n)	((n) * 1000000.0)
#define COORD_TO_INCH(n)	(COORD_TO_MIL(n) / 1000.0)
#define INCH_TO_COORD(n)	(MIL_TO_COORD(n) * 1000.0)

/* These need to be carefully written to avoid overflows, and return
   a Coord type.  */
#define SCALE_TEXT(COORD,TEXTSCALE) ((Coord)((COORD) * ((double)(TEXTSCALE) / 100.0)))
#define UNSCALE_TEXT(COORD,TEXTSCALE) ((Coord)((COORD) * (100.0 / (double)(TEXTSCALE))))

/* ---------------------------------------------------------------------------
 * modes
 */
#define	NO_MODE			0	/*!< no mode selected */
#define	VIA_MODE		1	/*!< draw vias */
#define	LINE_MODE		2	/*!< draw lines */
#define	RECTANGLE_MODE		3	/*!< create rectangles */
#define	POLYGON_MODE		4	/*!< draw filled polygons */
#define	PASTEBUFFER_MODE	5	/*!< paste objects from buffer */
#define	TEXT_MODE		6	/*!< create text objects */
#define	ROTATE_MODE		102	/*!< rotate objects */
#define	REMOVE_MODE		103	/*!< remove objects */
#define	MOVE_MODE		104	/*!< move objects */
#define	COPY_MODE		105	/*!< copy objects */
#define	INSERTPOINT_MODE	106	/*!< insert point into line/polygon */
#define	RUBBERBANDMOVE_MODE	107	/*!< move objects and attached lines */
#define THERMAL_MODE            108	/*!< toggle thermal layer flag */
#define ARC_MODE                109	/*!< draw arcs */
#define ARROW_MODE		110	/*!< selection with arrow mode */
#define PAN_MODE                0	/*!< same as no mode */
#define LOCK_MODE               111	/*!< lock/unlock objects */
#define	POLYGONHOLE_MODE	112	/*!< cut holes in filled polygons */

/* ---------------------------------------------------------------------------
 * object flags
 */

/* %start-doc pcbfile ~objectflags
@node Object Flags
@section Object Flags

Note that object flags can be given numerically (like @code{0x0147})
or symbolically (like @code{"found,showname,square"}.  Some numeric
values are reused for different object types.  The table below lists
the numeric value followed by the symbolic name.

@table @code
@item 0x0001 pin
If set, this object is a pin.  This flag is for internal use only.
@item 0x0002 via
Likewise, for vias.
@item 0x0004 found
If set, this object has been found by @code{FindConnection()}.
@item 0x0008 hole
For pins and vias, this flag means that the pin or via is a hole
without a copper annulus.
@item 0x0008 nopaste
For pads, set to prevent a solderpaste stencil opening for the
pad.  Primarily used for pads used as fiducials.
@item 0x0010 rat
If set for a line, indicates that this line is a rat line instead of a
copper trace.
@item 0x0010 pininpoly
For pins and pads, this flag is used internally to indicate that the
pin or pad overlaps a polygon on some layer.
@item 0x0010 clearpoly
For polygons, this flag means that pins and vias will normally clear
these polygons (thus, thermals are required for electrical
connection).  When clear, polygons will solidly connect to pins and
vias.
@item 0x0010 hidename
For elements, when set the name of the element is hidden.
@item 0x0020 showname
For elements, when set the names of pins are shown.
@item 0x0020 clearline
For lines and arcs, the line/arc will clear polygons instead of
connecting to them.
@item 0x0020 fullpoly
For polygons, the full polygon is drawn (i.e. all parts instead of only the biggest one).
@item 0x0040 selected
Set when the object is selected.
@item 0x0080 onsolder
For elements and pads, indicates that they are on the solder side.
@item 0x0080 auto
For lines and vias, indicates that these were created by the
autorouter.
@item 0x0100 square
For pins and pads, indicates a square (vs round) pin/pad.
@item 0x0200 rubberend
For lines, used internally for rubber band moves.
@item 0x0200 warn
For pins, vias, and pads, set to indicate a warning.
@item 0x0400 usetherm
Obsolete, indicates that pins/vias should be drawn with thermal
fingers.
@item 0x0400
Obsolete, old files used this to indicate lines drawn on silk.
@item 0x0800 octagon
Draw pins and vias as octagons.
@item 0x1000 drc
Set for objects that fail DRC.
@item 0x2000 lock
Set for locked objects.
@item 0x4000 edge2
For pads, indicates that the second point is closer to the edge.  For
pins, indicates that the pin is closer to a horizontal edge and thus
pinout text should be vertical.
@item 0x8000 marker
Marker used internally to avoid revisiting an object.
@item 0x10000 connected
If set, this object has been as physically connected by @code{FindConnection()}.
@end table
%end-doc */

#define NOFLAG                  0x0000
#define PINFLAG                 0x0001  /*!< is a pin */
#define VIAFLAG                 0x0002  /*!< is a via */
#define FOUNDFLAG               0x0004  /*!< used by 'FindConnection()' */
#define HOLEFLAG                0x0008  /*!< pin or via is only a hole */
#define NOPASTEFLAG             0x0008  /*!< pad should not receive
                                             solderpaste.  This is to
                                             support fiducials */
#define RATFLAG                 0x0010  /*!< indicates line is a rat line */
#define PININPOLYFLAG           0x0010  /*!< pin found inside poly - same as */
                                        /*!< rat line since not used on lines */
#define CLEARPOLYFLAG           0x0010  /*!< pins/vias clear these polygons */
#define HIDENAMEFLAG            0x0010  /*!< hide the element name */
#define DISPLAYNAMEFLAG         0x0020  /*!< display the names of pins/pads
                                             of an element */
#define CLEARLINEFLAG           0x0020  /*!< line doesn't touch polygons */
#define FULLPOLYFLAG            0x0020  /*!< full polygon is drawn (i.e. all parts instead of only the biggest one) */
#define SELECTEDFLAG            0x0040  /*!< object has been selected */
#define ONSOLDERFLAG            0x0080  /*!< element is on bottom side */
#define AUTOFLAG                0x0080  /*!< line/via created by auto-router */
#define SQUAREFLAG              0x0100  /*!< pin is square, not round */
#define RUBBERENDFLAG           0x0200  /*!< indicates one end already rubber
                                             banding same as warn flag
                                             since pins/pads won't use it */
#define WARNFLAG                0x0200  /*!< Warning for pin/via/pad */
#define USETHERMALFLAG          0x0400  /*!< draw pin, via with thermal fingers */
#define ONSILKFLAG              0x0400  /*!< old files use this to indicate silk */
#define OCTAGONFLAG             0x0800  /*!< draw pin/via as octagon instead of round */
#define DRCFLAG                 0x1000  /*!< flag like FOUND flag for DRC checking */
#define LOCKFLAG                0x2000  /*!< object locked in place */
#define EDGE2FLAG               0x4000  /*!< Padr.Point2 is closer to outside edge
                                             also pinout text for pins is vertical */
#define VISITFLAG               0x8000  /*!< marker to avoid re-visiting an object */
#define CONNECTEDFLAG          0x10000  /*!< flag like FOUND flag, but used to identify physically connected objects (not rats) */


#define NOCOPY_FLAGS (FOUNDFLAG | CONNECTEDFLAG)

/* ---------------------------------------------------------------------------
 * PCB flags
 */

/* %start-doc pcbfile ~pcbflags
@node PCBFlags
@section PCBFlags
@table @code
@item 0x00001
Pinout displays pin numbers instead of pin names.
@item 0x00002
Use local reference for moves, by setting the mark at the beginning of
each move.
@item 0x00004
When set, only polygons and their clearances are drawn, to see if
polygons have isolated regions.
@item 0x00008
Display DRC region on crosshair.
@item 0x00010
Do all move, mirror, rotate with rubberband connections.
@item 0x00020
Display descriptions of elements, instead of refdes.
@item 0x00040
Display names of elements, instead of refdes.
@item 0x00080
Auto-DRC flag.  When set, PCB doesn't let you place copper that
violates DRC.
@item 0x00100
Enable 'all-direction' lines.
@item 0x00200
Switch starting angle after each click.
@item 0x00400
Force unique names on board.
@item 0x00800
New lines/arc clear polygons.
@item 0x01000
Crosshair snaps to pins and pads.
@item 0x02000
Show the solder mask layer.
@item 0x04000
Draw with thin lines.
@item 0x08000
Move items orthogonally.
@item 0x10000
Draw autoroute paths real-time.
@item 0x20000
New polygons are full ones.
@item 0x40000
Names are locked, the mouse cannot select them.
@item 0x80000
Everything but names are locked, the mouse cannot select anything else.
@item 0x100000
New polygons are full polygons.
@item 0x200000
When set, element names are not drawn.
@end table
%end-doc */

#define	PCB_FLAGS		0x000fffff	/* all used flags */

#define SHOWNUMBERFLAG          0x00000001
#define LOCALREFFLAG            0x00000002
#define CHECKPLANESFLAG         0x00000004
#define SHOWDRCFLAG             0x00000008
#define RUBBERBANDFLAG		0x00000010
#define	DESCRIPTIONFLAG		0x00000020
#define	NAMEONPCBFLAG		0x00000040
#define AUTODRCFLAG             0x00000080
#define	ALLDIRECTIONFLAG	0x00000100
#define SWAPSTARTDIRFLAG        0x00000200
#define UNIQUENAMEFLAG		0x00000400
#define CLEARNEWFLAG		0x00000800
#define SNAPPINFLAG             0x00001000
#define SHOWMASKFLAG            0x00002000
#define THINDRAWFLAG            0x00004000
#define ORTHOMOVEFLAG           0x00008000
#define LIVEROUTEFLAG           0x00010000
#define THINDRAWPOLYFLAG        0x00020000
#define LOCKNAMESFLAG           0x00040000
#define ONLYNAMESFLAG           0x00080000
#define NEWFULLPOLYFLAG         0x00100000
#define HIDENAMESFLAG           0x00200000
#define AUTOBURIEDVIASFLAG      0x00400000

/* ---------------------------------------------------------------------------
 * object types
 */
#define	NO_TYPE			0x00000	/*!< no object */
#define	VIA_TYPE		0x00001
#define	ELEMENT_TYPE		0x00002
#define	LINE_TYPE		0x00004
#define	POLYGON_TYPE		0x00008
#define	TEXT_TYPE		0x00010
#define RATLINE_TYPE		0x00020

#define	PIN_TYPE		0x00100	/*!< objects that are part */
#define	PAD_TYPE		0x00200	/*!< 'pin' of SMD element */
#define	ELEMENTNAME_TYPE	0x00400	/*!< of others */
#define	POLYGONPOINT_TYPE	0x00800
#define	LINEPOINT_TYPE		0x01000
#define ELEMENTLINE_TYPE        0x02000
#define ARC_TYPE                0x04000
#define ELEMENTARC_TYPE		0x08000

#define LOCKED_TYPE 		0x10000	/*!< used to tell search to include locked items. */
#define NET_TYPE		0x20000 /*!< used to select whole net. */
#define ARCPOINT_TYPE		0x40000

#define PIN_TYPES     (VIA_TYPE | PIN_TYPE)
#define LOCK_TYPES    (VIA_TYPE | LINE_TYPE | ARC_TYPE | POLYGON_TYPE | ELEMENT_TYPE \
                      | TEXT_TYPE | ELEMENTNAME_TYPE | LOCKED_TYPE)

#define	ALL_TYPES		(~0)	/*!< all bits set */

#endif
