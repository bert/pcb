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

/* some commonly used macros not related to a special C-file
 * the file is included by global.h after const.h
 */

#ifndef	__MACRO_INCLUDED__
#define	__MACRO_INCLUDED__

/* ---------------------------------------------------------------------------
 * macros to transform coord systems
 * draw.c uses a different definition of TO_SCREEN
 */
#ifndef	SWAP_IDENT
#define	SWAP_IDENT			Settings.ShowSolderSide
#endif

#ifndef XORIG
#define XORIG Xorig
#define YORIG Yorig
#endif

#define	SWAP_SIGN_X(x)		(x)
#define	SWAP_SIGN_Y(y)		(-(y))
#define	SWAP_ANGLE(a)		(-(a))
#define	SWAP_DELTA(d)		(-(d))
#define	SWAP_X(x)		(SWAP_SIGN_X(x))
#define	SWAP_Y(y)		(PCB->MaxHeight +SWAP_SIGN_Y(y))

#ifndef	TO_SCREEN
#define	TO_SCREEN(x)		((PCB->Zoom < 0) ? (x) << -PCB->Zoom : (x) >> PCB->Zoom)
#endif

#define	TO_SCREEN_X(x)		(TO_SCREEN(SWAP_IDENT ? SWAP_X(x) : (x)) + Xorig)
#define	TO_SCREEN_Y(y)		(TO_SCREEN(SWAP_IDENT ? SWAP_Y(y) : (y)) + Yorig)
#define	TO_DRAW_X(x)		(TO_SCREEN(SWAP_IDENT ? SWAP_X(x) : (x)) + XORIG)
#define	TO_DRAWABS_X(x)		(TO_SCREEN((x)) + XORIG)
#define	TO_DRAW_Y(y)		(TO_SCREEN(SWAP_IDENT ? SWAP_Y(y) : (y)) + YORIG)
#define	TO_DRAWABS_Y(y)		(TO_SCREEN((y)) + YORIG)
#define	TO_MASK_X(x)		TO_SCREEN_X((x))
#define	TO_MASK_Y(y)		TO_SCREEN_Y((y))
#define	TO_SCREEN_ANGLE(a)	(SWAP_IDENT ? SWAP_ANGLE((a)) : (a))
#define	TO_SCREEN_DELTA(d)	(SWAP_IDENT ? SWAP_DELTA((d)) : (d))
#define	TO_SCREEN_SIGN_X(x)	(SWAP_IDENT ? SWAP_SIGN_X(x) : (x))
#define	TO_SCREEN_SIGN_Y(y)	(SWAP_IDENT ? SWAP_SIGN_Y(y) : (y))

#ifndef	TO_PCB
#define	TO_PCB(x)		((PCB->Zoom < 0) ? ((x)+ (1<<(-1-PCB->Zoom))) >> -PCB->Zoom : (x) << PCB->Zoom)
#endif
#define	TO_PCB_X(x)		TO_PCB(x - Xorig)
#define	TO_PCB_Y(y)		(SWAP_IDENT ? \
				PCB->MaxHeight -TO_PCB(y - Yorig) : TO_PCB(y - Yorig))

/* ---------------------------------------------------------------------------
 * misc macros, some might already be defined by <limits.h>
 */
#ifndef	MIN
#define	MIN(a,b)		((a) < (b) ? (a) : (b))
#define	MAX(a,b)		((a) > (b) ? (a) : (b))
#endif
#ifndef SGN
#define SGN(a)			((a) >0 ? 1 : ((a) == 0 ? 0 : -1))
#endif

#define	ENTRIES(x)		(sizeof((x))/sizeof((x)[0]))
#define	UNKNOWN(a)		((a) && *(a) ? (a) : "(unknown)")
#define	EMPTY(a)		((a) ? (a) : "")
#define XOR(a,b)		(((a) && !(b)) || (!(a) && (b)))
#define SQUARE(x)		((float) (x) * (float) (x))
#define TO_RADIANS(degrees)	(M180 * (degrees))
 
/* ---------------------------------------------------------------------------
 * macros to determine if something is on the visible part of the screen
 */

#define VELEMENT(e)	((e)->BoundingBox.X1 <= vxh && \
	(e)->BoundingBox.X2 >= vxl && \
	(e)->BoundingBox.Y1 <= vyh && \
	(e)->BoundingBox.Y2 >= vyl)
#define VELTEXT(e) 	(ELEMENT_TEXT(PCB,(e)).BoundingBox.X1 <= vxh && \
	ELEMENT_TEXT(PCB,(e)).BoundingBox.X2 >= vxl && \
	ELEMENT_TEXT(PCB,(e)).BoundingBox.Y1 <= vyh && \
	ELEMENT_TEXT(PCB,(e)).BoundingBox.Y2 >= vyl)
#define VTEXT(t) 	((t)->BoundingBox.X1 <= vxh && \
	(t)->BoundingBox.X2 >= vxl && \
	(t)->BoundingBox.Y1 <= vyh && \
	(t)->BoundingBox.Y2 >= vyl)
#define VLINE(l)	((((l)->Point2.X >= vxl && (l)->Point1.X <= vxh) || \
	((l)->Point1.X >= vxl && (l)->Point2.X <= vxh)) && \
	((((l)->Point2.Y >= vyl && (l)->Point1.Y <= vyh) || \
	((l)->Point1.Y >= vyl && (l)->Point2.Y <= vyh))))
#define VARC(a) 	((a)->BoundingBox.X1 <= vxh && \
	(a)->BoundingBox.X2 >= vxl && \
	(a)->BoundingBox.Y1 <= vyh && \
	(a)->BoundingBox.Y2 >= vyl)
#define VPOLY(p) 	((p)->BoundingBox.X1 <= vxh && \
	(p)->BoundingBox.X2 >= vxl && \
	(p)->BoundingBox.Y1 <= vyh && \
	(p)->BoundingBox.Y2 >= vyl)
#define VVIA(v) 	((v)->X >= vxl && (v)->X <= vxh && \
			(v)->Y >= vyl && (v)->Y <= vyh)

/* ---------------------------------------------------------------------------
 * layer macros
 */
#define	LAYER_ON_STACK(n)	(&PCB->Data->Layer[LayerStack[(n)]])
#define LAYER_PTR(n)            (&PCB->Data->Layer[(n)])
#define	CURRENT			(PCB->SilkActive ? &PCB->Data->Layer[MAX_LAYER + \
				(Settings.ShowSolderSide ? SOLDER_LAYER : COMPONENT_LAYER)] \
				: LAYER_ON_STACK(0))
#define	INDEXOFCURRENT		(PCB->SilkActive ? MAX_LAYER + \
				(Settings.ShowSolderSide ? SOLDER_LAYER : COMPONENT_LAYER) \
				: LayerStack[0])
#define SILKLAYER		Layer[MAX_LAYER + \
				(Settings.ShowSolderSide ? SOLDER_LAYER : COMPONENT_LAYER)]
#define BACKSILKLAYER		Layer[MAX_LAYER + \
				(Settings.ShowSolderSide ? COMPONENT_LAYER : SOLDER_LAYER)]

/* ---------------------------------------------------------------------------
 * returns the object ID
 */
#define	OBJECT_ID(p)		(((AnyObjectTypePtr) p)->ID)

/* ---------------------------------------------------------------------------
 * access macro for current buffer
 */
#define	PASTEBUFFER		(&Buffers[Settings.BufferNumber])

/* ---------------------------------------------------------------------------
 * some routines for flag setting, clearing, changeing and testing
 */
#define	SET_FLAG(f,p)		((p)->Flags |= (f))
#define	CLEAR_FLAG(f,p)		((p)->Flags &= (~(f)))
#define	TEST_FLAG(f,p)		((p)->Flags & (f) ? True : False)
#define	TOGGLE_FLAG(f,p)	((p)->Flags = TEST_FLAG((f),(p)) ? (p)->Flags & (~(f)) : (p)->Flags | (f))
#define	ASSIGN_FLAG(f,v,p)	((p)->Flags = ((p)->Flags & (~(f))) | ((v) ? (f) : 0))

/* ---------------------------------------------------------------------------
 * access macros for elements name structure
 */
#define	DESCRIPTION_INDEX	0
#define	NAMEONPCB_INDEX		1
#define	VALUE_INDEX		2
#define	NAME_INDEX(p)		(TEST_FLAG(NAMEONPCBFLAG,(p)) ? NAMEONPCB_INDEX :\
				(TEST_FLAG(DESCRIPTIONFLAG, (p)) ?		\
				DESCRIPTION_INDEX : VALUE_INDEX))
#define	ELEMENT_NAME(p,e)	((e)->Name[NAME_INDEX((p))].TextString)
#define	DESCRIPTION_NAME(e)	((e)->Name[DESCRIPTION_INDEX].TextString)
#define	NAMEONPCB_NAME(e)	((e)->Name[NAMEONPCB_INDEX].TextString)
#define	VALUE_NAME(e)		((e)->Name[VALUE_INDEX].TextString)
#define	ELEMENT_TEXT(p,e)	((e)->Name[NAME_INDEX((p))])
#define	DESCRIPTION_TEXT(e)	((e)->Name[DESCRIPTION_INDEX])
#define	NAMEONPCB_TEXT(e)	((e)->Name[NAMEONPCB_INDEX])
#define	VALUE_TEXT(e)		((e)->Name[VALUE_INDEX])

/* ----------------------------------------------------------------------
 * checks for correct X values
 */
#define	VALID_PIXMAP(p)	((p) != BadValue && \
	(p) != BadAlloc && \
	(p) != BadDrawable)

#define	VALID_GC(p)	((p) != BadValue && \
	(p) != BadAlloc && \
	(p) != BadDrawable && \
	(p) != BadFont && \
	(p) != BadMatch && \
	(p) != BadPixmap)

/* ---------------------------------------------------------------------------
 *  Determines if text is actually visible
#define TEXT_IS_VISIBLE(b, l, t)					\
		(((l)->On && !TEST_FLAG(ONSILKFLAG, (t))) ||     	\
		(TEST_FLAG(ONSILKFLAG, (t)) && (b)->ElementOn   	\
		&& ((b)->InvisibleObjectsOn ||                  	\
		(TEST_FLAG(ONSOLDERFLAG, (t)) != 0) == SWAP_IDENT)))
*/
#define TEXT_IS_VISIBLE(b, l, t) \
	((l)->On)

/* ---------------------------------------------------------------------------
 *  Determines if object is on front or back
 */
#define FRONT(o)	\
	((TEST_FLAG(ONSOLDERFLAG, (o)) != 0) == SWAP_IDENT)
			
/* ---------------------------------------------------------------------------
 * some loop shortcuts
 * all object loops run backwards to prevent from errors when deleting objects
 *
 * a pointer is created from index addressing because the base pointer
 * may change when new memory is allocated;
 *
 * all data is relativ to an objects name 'top' which can be either
 * PCB or PasteBuffer
 */
#define STYLE_LOOP(top, command) {		\
	Cardinal	n;			\
	RouteStyleTypePtr	style;			\
	for (n = 0; n < NUM_STYLES; n++)	\
	{					\
		style = &(top)->RouteStyle[n];	\
		command;			\
	}}

#define	VIA_LOOP(top, command)	{ 		\
	Cardinal	n, sn;			\
	PinTypePtr	via;			\
        for (sn = (top)->ViaN, n = 0; (top)->ViaN > 0 && n < (top)->ViaN ; \
		n += 1 + (top)->ViaN - sn, sn = (top)->ViaN)   \
	{					\
		via = &(top)->Via[n];		\
		command;			\
	}}

#define DRILL_LOOP(top, command)              {               \
        Cardinal        n;                                      \
        DrillTypePtr    drill;                                  \
        for (n = 0; (top)->DrillN > 0 && n < (top)->DrillN; n++)                        \
        {                                                       \
                drill = &(top)->Drill[n];                       \
                command;                                        \
        }}      

#define NETLIST_LOOP(top, command)    {                         \
        Cardinal        n;                                      \
        NetListTypePtr   netlist;                               \
        for (n = (top)->NetListN-1; n != -1; n--)               \
        {                                                       \
                netlist = &(top)->NetList[n];                   \
                command;                                        \
        }}      

#define NET_LOOP(top, command)    {                             \
        Cardinal        n;                                      \
        NetTypePtr   net;                                       \
        for (n = (top)->NetN-1; n != -1; n--)                   \
        {                                                       \
                net = &(top)->Net[n];                           \
                command;                                        \
        }}      

#define CONNECTION_LOOP(net, command) {                         \
        Cardinal        n;                                      \
        ConnectionTypePtr       connection;                     \
        for (n = (net)->ConnectionN-1; n != -1; n--)            \
        {                                                       \
                connection = & (net)->Connection[n];            \
                command;                                        \
        }}

#define	ELEMENT_LOOP(top, command)	{ 		\
	Cardinal 		n;			\
	ElementTypePtr	element;			\
	for (n = (top)->ElementN-1; n != -1; n--)	\
	{						\
		element = &(top)->Element[n];		\
		command;				\
	}}

#define RAT_LOOP(top, command)	{			\
	Cardinal	n;				\
	RatTypePtr	line;				\
	for (n = (top)->RatN-1; n != -1; n--)		\
	{						\
		line = &(top)->Rat[n];			\
		command;				\
	}}


#define	ELEMENTTEXT_LOOP(element, command)	{ 	\
	Cardinal	n;				\
	TextTypePtr	text;				\
	for (n = MAX_ELEMENTNAMES-1; n != -1; n--)	\
	{						\
		text = &(element)->Name[n];		\
		command;				\
	}}

#define	ELEMENTNAME_LOOP(element, command)	{ 		\
	Cardinal	n;					\
	char		*textstring;				\
	for (n = MAX_ELEMENTNAMES-1; n != -1; n--)		\
	{							\
		textstring = (element)->Name[n].TextString;	\
		command;					\
	}}

#define	PIN_LOOP(element, command)	{ 		\
	Cardinal	n, sn;				\
	PinTypePtr	pin;				\
        for (sn = (element)->PinN, n = 0; (element)->PinN > 0 && n < (element)->PinN ; \
		n += 1 + (element)->PinN - sn, sn = (element)->PinN)   \
	{						\
		pin = &(element)->Pin[n];		\
		command;				\
	}}

#define	PAD_LOOP(element, command)	{ 		\
	Cardinal	n, sn;				\
	PadTypePtr	pad;				\
        for (sn = (element)->PadN, n = 0; (element)->PadN > 0 && n < (element)->PadN ; \
		 sn == (element)->PadN ? n++ : 0)   \
	{						\
		pad = &(element)->Pad[n];		\
		command;				\
	}}

#define	ARC_LOOP(element, command)	{ 		\
	Cardinal	n;				\
	ArcTypePtr	arc;				\
	for (n = (element)->ArcN-1; n != -1; n--)	\
	{						\
		arc = &(element)->Arc[n];		\
		command;				\
	}}

#define	ELEMENTLINE_LOOP(element, command)	{ 	\
	Cardinal	n;				\
	LineTypePtr	line;				\
	for (n = (element)->LineN-1; n != -1; n--)	\
	{						\
		line = &(element)->Line[n];		\
		command;				\
	}}

#define	LINE_LOOP(layer, command) {			\
	Cardinal		n;			\
	LineTypePtr		line;			\
	for (n = (layer)->LineN-1; n != -1; n--)	\
	{						\
		line = &(layer)->Line[n];		\
		command;				\
	}}

#define	TEXT_LOOP(layer, command) {			\
	Cardinal		n;			\
	TextTypePtr		text;			\
	for (n = (layer)->TextN-1; n != -1; n--)	\
	{						\
		text = &(layer)->Text[n];		\
		command;				\
	}}

#define	POLYGON_LOOP(layer, command) {			\
	Cardinal		n;			\
	PolygonTypePtr	polygon;			\
	for (n = (layer)->PolygonN-1; n != -1; n--)	\
	{						\
		polygon = &(layer)->Polygon[n];		\
		command;				\
	}}

#define	POLYGONPOINT_LOOP(polygon, command)	{	\
	Cardinal			n;		\
	PointTypePtr	point;				\
	for (n = (polygon)->PointN-1; n != -1; n--)	\
	{						\
		point = &(polygon)->Points[n];		\
		command;				\
	}}

#define	ALLPIN_LOOP(top, command)	{			\
	ELEMENT_LOOP((top), PIN_LOOP(element, command ))	\
	}

#define	ALLPAD_LOOP(top, command)	{			\
	ELEMENT_LOOP((top), PAD_LOOP(element, command ))	\
	}

#define	ALLLINE_LOOP(top, command)	{		\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < MAX_LAYER + 2; l++, layer++)	\
		LINE_LOOP(layer, command );		\
	}

#define ALLARC_LOOP(top, command)	{		\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l =0; l < MAX_LAYER + 2; l++, layer++)		\
		ARC_LOOP(layer, command );		\
	}

#define	ALLPOLYGON_LOOP(top, command)	{		\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < MAX_LAYER + 2; l++, layer++)	\
		POLYGON_LOOP(layer, command );		\
	}

#define	COPPERLINE_LOOP(top, command)	{		\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < MAX_LAYER; l++, layer++)	\
		LINE_LOOP(layer, command );		\
	}

#define COPPERARC_LOOP(top, command)	{		\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l =0; l < MAX_LAYER; l++, layer++)		\
		ARC_LOOP(layer, command );		\
	}

#define	COPPERPOLYGON_LOOP(top, command)	{		\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < MAX_LAYER; l++, layer++)	\
		POLYGON_LOOP(layer, command );		\
	}

#define	ALLTEXT_LOOP(top, command)	{		\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < MAX_LAYER + 2; l++, layer++)	\
		TEXT_LOOP(layer, command );		\
	}

#define	VISIBLELINE_LOOP(top, command)	{		\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < MAX_LAYER + 2; l++, layer++)	\
		if (layer->On)				\
			LINE_LOOP(layer, command );	\
	}

#define	VISIBLEARC_LOOP(top, command)	{		\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < MAX_LAYER + 2; l++, layer++)	\
		if (layer->On)				\
			ARC_LOOP(layer, command );	\
	}

#define	VISIBLETEXT_LOOP(board, command)	{		\
	Cardinal		l;			\
	LayerTypePtr	layer = (board)->Data->Layer;		\
	for (l = 0; l < MAX_LAYER + 2; l++, layer++)	\
                TEXT_LOOP(layer,                                        \
                          if (TEXT_IS_VISIBLE((board), layer, text))                      \
                          command );                              \
	}

#define	VISIBLEPOLYGON_LOOP(top, command)	{	\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < MAX_LAYER + 2; l++, layer++)	\
		if (layer->On)				\
			POLYGON_LOOP(layer, command );	\
	}

#define POINTER_LOOP(top, command)	{	\
	Cardinal	n;			\
	void	**ptr;				\
	for (n = (top)->PtrN-1; n != -1; n--)	\
	{					\
		ptr = &(top)->Ptr[n];		\
		command;			\
	}}

#define MENU_LOOP(top, command)	{	\
	Cardinal	l;			\
	LibraryMenuTypePtr menu;		\
	for (l = (top)->MenuN-1; l != -1; l--)	\
	{					\
		menu = &(top)->Menu[l];		\
		command;			\
	}}

#define ENTRY_LOOP(top, command)	{	\
	Cardinal	n;			\
	LibraryEntryTypePtr entry;		\
	for (n = (top)->EntryN-1; n != -1; n--)	\
	{					\
		entry = &(top)->Entry[n];	\
		command;			\
	}}
#endif

