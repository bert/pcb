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
#define SATURATE(x)             ((x) > 32767 ? 32767 : ((x) < -32767 ? -32767 : (x)))

#define	TO_DRAW_X(x)		TO_SCREEN((SWAP_IDENT ? SWAP_X(x) : (x)) - XORIG)
#define	TO_DRAWABS_X(x)		(TO_SCREEN((x) - XORIG))
#define	TO_DRAW_Y(y)		TO_SCREEN((SWAP_IDENT ? SWAP_Y(y) : (y)) - YORIG)
#define	TO_DRAWABS_Y(y)		(TO_SCREEN((y) - YORIG))
#define TO_LIMIT_X(x)           ((Position)(SATURATE(Local_Zoom * ((x) - XORIG))))
#define TO_LIMIT_Y(y)           ((Position)(SATURATE(Local_Zoom * \
                                ((SWAP_IDENT ? SWAP_Y(y) : (y)) - YORIG))))
#define	TO_SCREEN_ANGLE(a)	(SWAP_IDENT ? SWAP_ANGLE((a)) : (a))
#define	TO_SCREEN_DELTA(d)	(SWAP_IDENT ? SWAP_DELTA((d)) : (d))
#define	TO_SCREEN_SIGN_X(x)	(SWAP_IDENT ? SWAP_SIGN_X(x) : (x))
#define	TO_SCREEN_SIGN_Y(y)	(SWAP_IDENT ? SWAP_SIGN_Y(y) : (y))

#ifndef	TO_PCB
#define	TO_PCB(x)		((LocationType)((x)/Zoom_Multiplier))
#endif
#define	TO_PCB_X(x)		TO_PCB(x) + Xorig
#define	TO_PCB_Y(y)		(SWAP_IDENT ? \
				PCB->MaxHeight - TO_PCB(y) - Yorig : TO_PCB(y) + Yorig)
#ifdef __GNUC__
#pragma GCC poison TO_SCREEN TO_SCREEN_X TO_SCREEN_Y
#endif

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
#define SGNZ(a)                 ((a) >=0 ? 1 : -1)
#define MAKEMIN(a,b)            if ((b) < (a)) (a) = (b)
#define MAKEMAX(a,b)            if ((b) > (a)) (a) = (b)

#define	ENTRIES(x)		(sizeof((x))/sizeof((x)[0]))
#define	UNKNOWN(a)		((a) && *(a) ? (a) : "(unknown)")
#define NSTRCMP(a, b)		((a) ? ((b) ? strcmp((a),(b)) : 1) : -1)
#define	EMPTY(a)		((a) ? (a) : "")
#define	EMPTY_STRING_P(a)	((a) ? (a)[0]==0 : 1)
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
#define VLINE(l)	((((l)->Point2.X >= vxl - (l)->Thickness && \
        (l)->Point1.X <= vxh + (l)->Thickness) || \
	((l)->Point1.X >= vxl - (l)->Thickness && \
	(l)->Point2.X <= vxh + (l)->Thickness)) && \
	((((l)->Point2.Y >= vyl - (l)->Thickness && \
	(l)->Point1.Y <= vyh + (l)->Thickness) || \
	((l)->Point1.Y >= vyl - (l)->Thickness && \
	(l)->Point2.Y <= vyh + (l)->Thickness))))
#define VARC(a) 	((a)->BoundingBox.X1 <= vxh && \
	(a)->BoundingBox.X2 >= vxl && \
	(a)->BoundingBox.Y1 <= vyh && \
	(a)->BoundingBox.Y2 >= vyl)
#define VPOLY(p) 	(IsRectangleInPolygon(vxl, vyl, vxh, vyh, (p)))
/* (p)->BoundingBox.X1 <= vxh && \
	(p)->BoundingBox.X2 >= vxl && \
	(p)->BoundingBox.Y1 <= vyh && \
	(p)->BoundingBox.Y2 >= vyl)
*/
#define VVIA(v) 	((v)->X+(v)->Thickness >= vxl && (v)->X - (v)->Thickness <= vxh && \
			(v)->Y+(v)->Thickness >= vyl && (v)->Y - (v)->Thickness  <= vyh)
#define VTHERM(v) 	((v)->X + (v)->Thickness + (v)->Clearance >= vxl && \
			(v)->X - (v)->Thickness - (v)->Clearance <= vxh && \
			(v)->Y + (v)->Thickness + (v)->Thickness >= vyl && \
			(v)->Y - (v)->Thickness - (v)->Clearance <= vyh)

#undef VELEMENT
#define VELEMENT(e) 1
#undef VELTEXT
#define VELTEXT(e) 1
#undef VTEXT
#define VTEXT(e) 1
#undef VLINE
#define VLINE(e) 1
#undef VARC
#define VARC(e) 1
#undef VPOLY
#define VPOLY(e) 1
#undef VVIA
#define VVIA(e) 1
#undef VTHERM
#define VTHERM(e) 1

/* ---------------------------------------------------------------------------
 * layer macros
 */
#define	LAYER_ON_STACK(n)	(&PCB->Data->Layer[LayerStack[(n)]])
#define LAYER_PTR(n)            (&PCB->Data->Layer[(n)])
#define	CURRENT			(PCB->SilkActive ? &PCB->Data->Layer[max_layer + \
				(Settings.ShowSolderSide ? SOLDER_LAYER : COMPONENT_LAYER)] \
				: LAYER_ON_STACK(0))
#define	INDEXOFCURRENT		(PCB->SilkActive ? max_layer + \
				(Settings.ShowSolderSide ? SOLDER_LAYER : COMPONENT_LAYER) \
				: LayerStack[0])
#define SILKLAYER		Layer[max_layer + \
				(Settings.ShowSolderSide ? SOLDER_LAYER : COMPONENT_LAYER)]
#define BACKSILKLAYER		Layer[max_layer + \
				(Settings.ShowSolderSide ? COMPONENT_LAYER : SOLDER_LAYER)]

#define TEST_SILK_LAYER(layer)	(GetLayerNumber (PCB->Data, layer) >= max_layer)


/* ---------------------------------------------------------------------------
 * returns the object ID
 */
#define	OBJECT_ID(p)		(((AnyObjectTypePtr) p)->ID)

/* ---------------------------------------------------------------------------
 * access macro for current buffer
 */
#define	PASTEBUFFER		(&Buffers[Settings.BufferNumber])

/* ---------------------------------------------------------------------------
 * some routines for flag setting, clearing, changing and testing
 */
#define	SET_FLAG(F,P)		((P)->Flags.f |= (F))
#define	CLEAR_FLAG(F,P)		((P)->Flags.f &= (~(F)))
#define	TEST_FLAG(F,P)		((P)->Flags.f & (F) ? 1 : 0)
#define	TOGGLE_FLAG(F,P)	((P)->Flags.f ^= (F))
#define	ASSIGN_FLAG(F,V,P)	((P)->Flags.f = ((P)->Flags.f & (~(F))) | ((V) ? (F) : 0))
#define TEST_FLAGS(F,P)         (((P)->Flags.f & (F)) == (F) ? 1 : 0)

#define FLAGS_EQUAL(F1,F2)	(memcmp (&F1, &F2, sizeof(FlagType)) == 0)

#define THERMFLAG(L)		(0xf << (4 *((L) % 2)))

#define TEST_THERM(L,P)		((P)->Flags.t[(L)/2] & THERMFLAG(L) ? 1 : 0)
#define GET_THERM(L,P)		(((P)->Flags.t[(L)/2] >> (4 * ((L) % 2))) & 0xf) 
#define CLEAR_THERM(L,P)	(P)->Flags.t[(L)/2] &= ~THERMFLAG(L)
#define ASSIGN_THERM(L,V,P)	(P)->Flags.t[(L)/2] = ((P)->Flags.t[(L)/2] & ~THERMFLAG(L)) | ((V)  << (4 * ((L) % 2)))

extern int mem_any_set (unsigned char *, int);
#define TEST_ANY_THERMS(P)	mem_any_set((P)->Flags.t, sizeof((P)->Flags.t))

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
#define END_LOOP  }} while (0)

#define STYLE_LOOP(top)  do {		\
	Cardinal	n;			\
	RouteStyleTypePtr	style;			\
	for (n = 0; n < NUM_STYLES; n++)	\
	{					\
		style = &(top)->RouteStyle[n]

#define	VIA_LOOP(top)	do { 		\
	Cardinal	n, sn;			\
	PinTypePtr	via;			\
        for (sn = (top)->ViaN, n = 0; (top)->ViaN > 0 && n < (top)->ViaN ; \
		n += 1 + (top)->ViaN - sn, sn = (top)->ViaN)   \
	{					\
		via = &(top)->Via[n]

#define DRILL_LOOP(top) do             {               \
        Cardinal        n;                                      \
        DrillTypePtr    drill;                                  \
        for (n = 0; (top)->DrillN > 0 && n < (top)->DrillN; n++)                        \
        {                                                       \
                drill = &(top)->Drill[n]

#define NETLIST_LOOP(top) do   {                         \
        Cardinal        n;                                      \
        NetListTypePtr   netlist;                               \
        for (n = (top)->NetListN-1; n != -1; n--)               \
        {                                                       \
                netlist = &(top)->NetList[n]

#define NET_LOOP(top) do   {                             \
        Cardinal        n;                                      \
        NetTypePtr   net;                                       \
        for (n = (top)->NetN-1; n != -1; n--)                   \
        {                                                       \
                net = &(top)->Net[n]

#define CONNECTION_LOOP(net) do {                         \
        Cardinal        n;                                      \
        ConnectionTypePtr       connection;                     \
        for (n = (net)->ConnectionN-1; n != -1; n--)            \
        {                                                       \
                connection = & (net)->Connection[n]

#define	ELEMENT_LOOP(top) do	{ 		\
	Cardinal 		n;			\
	ElementTypePtr	element;			\
	for (n = (top)->ElementN-1; n != -1; n--)	\
	{						\
		element = &(top)->Element[n]

#define RAT_LOOP(top) do	{			\
	Cardinal	n;				\
	RatTypePtr	line;				\
	for (n = (top)->RatN-1; n != -1; n--)		\
	{						\
		line = &(top)->Rat[n]


#define	ELEMENTTEXT_LOOP(element) do { 	\
	Cardinal	n;				\
	TextTypePtr	text;				\
	for (n = MAX_ELEMENTNAMES-1; n != -1; n--)	\
	{						\
		text = &(element)->Name[n]

#define	ELEMENTNAME_LOOP(element) do	{ 		\
	Cardinal	n;					\
	char		*textstring;				\
	for (n = MAX_ELEMENTNAMES-1; n != -1; n--)		\
	{							\
		textstring = (element)->Name[n].TextString

#define	PIN_LOOP(element)	do { 		\
	Cardinal	n, sn;				\
	PinTypePtr	pin;				\
        for (sn = (element)->PinN, n = 0; (element)->PinN > 0 && n < (element)->PinN ; \
		n += 1 + (element)->PinN - sn, sn = (element)->PinN)   \
	{						\
		pin = &(element)->Pin[n]

#define	PAD_LOOP(element)	do { 		\
	Cardinal	n, sn;				\
	PadTypePtr	pad;				\
        for (sn = (element)->PadN, n = 0; (element)->PadN > 0 && n < (element)->PadN ; \
		 sn == (element)->PadN ? n++ : 0)   \
	{						\
		pad = &(element)->Pad[n]

#define	ARC_LOOP(element)	do { 		\
	Cardinal	n;				\
	ArcTypePtr	arc;				\
	for (n = (element)->ArcN-1; n != -1; n--)	\
	{						\
		arc = &(element)->Arc[n]

#define	ELEMENTLINE_LOOP(element)	do { 	\
	Cardinal	n;				\
	LineTypePtr	line;				\
	for (n = (element)->LineN-1; n != -1; n--)	\
	{						\
		line = &(element)->Line[n]

#define	ELEMENTARC_LOOP(element)	do { 	\
	Cardinal	n;				\
	ArcTypePtr	arc;				\
	for (n = (element)->ArcN-1; n != -1; n--)	\
	{						\
		arc = &(element)->Arc[n]

#define	LINE_LOOP(layer) do {			\
	Cardinal		n;			\
	LineTypePtr		line;			\
	for (n = (layer)->LineN-1; n != -1; n--)	\
	{						\
		line = &(layer)->Line[n]

#define	TEXT_LOOP(layer) do {			\
	Cardinal		n;			\
	TextTypePtr		text;			\
	for (n = (layer)->TextN-1; n != -1; n--)	\
	{						\
		text = &(layer)->Text[n]

#define	POLYGON_LOOP(layer) do {			\
	Cardinal		n;			\
	PolygonTypePtr	polygon;			\
	for (n = (layer)->PolygonN-1; n != -1; n--)	\
	{						\
		polygon = &(layer)->Polygon[n]

#define	POLYGONPOINT_LOOP(polygon) do	{	\
	Cardinal			n;		\
	PointTypePtr	point;				\
	for (n = (polygon)->PointN-1; n != -1; n--)	\
	{						\
		point = &(polygon)->Points[n]

#define ENDALL_LOOP }} while (0);  }} while (0)

#define	ALLPIN_LOOP(top)	\
        ELEMENT_LOOP(top); \
	        PIN_LOOP(element)\

#define	ALLPAD_LOOP(top) \
	ELEMENT_LOOP(top); \
	  PAD_LOOP(element)

#define	ALLLINE_LOOP(top) do	{		\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_layer + 2; l++, layer++)	\
	{ \
		LINE_LOOP(layer)

#define ALLARC_LOOP(top) do {		\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l =0; l < max_layer + 2; l++, layer++)		\
	{ \
		ARC_LOOP(layer)

#define	ALLPOLYGON_LOOP(top)	do {		\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_layer + 2; l++, layer++)	\
	{ \
		POLYGON_LOOP(layer)

#define	COPPERLINE_LOOP(top) do	{		\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_layer; l++, layer++)	\
	{ \
		LINE_LOOP(layer)

#define COPPERARC_LOOP(top) do	{		\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l =0; l < max_layer; l++, layer++)		\
	{ \
		ARC_LOOP(layer)

#define	COPPERPOLYGON_LOOP(top) do	{		\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_layer; l++, layer++)	\
	{ \
		POLYGON_LOOP(layer)

#define	SILKLINE_LOOP(top) do	{		\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	layer += max_layer;				\
	for (l = 0; l < 2; l++, layer++)		\
	{ \
		LINE_LOOP(layer)

#define SILKARC_LOOP(top) do	{		\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	layer += max_layer;				\
	for (l = 0; l < 2; l++, layer++)		\
	{ \
		ARC_LOOP(layer)

#define	SILKPOLYGON_LOOP(top) do	{		\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	layer += max_layer;				\
	for (l = 0; l < 2; l++, layer++)		\
	{ \
		POLYGON_LOOP(layer)

#define	ALLTEXT_LOOP(top)	do {		\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_layer + 2; l++, layer++)	\
	{ \
		TEXT_LOOP(layer)

#define	VISIBLELINE_LOOP(top) do	{		\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_layer + 2; l++, layer++)	\
	{ \
		if (layer->On)				\
			LINE_LOOP(layer)

#define	VISIBLEARC_LOOP(top) do	{		\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_layer + 2; l++, layer++)	\
	{ \
		if (layer->On)				\
			ARC_LOOP(layer)

#define	VISIBLETEXT_LOOP(board) do	{		\
	Cardinal		l;			\
	LayerTypePtr	layer = (board)->Data->Layer;		\
	for (l = 0; l < max_layer + 2; l++, layer++)	\
	{ \
                TEXT_LOOP(layer);                                      \
                  if (TEXT_IS_VISIBLE((board), layer, text))

#define	VISIBLEPOLYGON_LOOP(top) do	{	\
	Cardinal		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_layer + 2; l++, layer++)	\
	{ \
		if (layer->On)				\
			POLYGON_LOOP(layer)

#define POINTER_LOOP(top) do	{	\
	Cardinal	n;			\
	void	**ptr;				\
	for (n = (top)->PtrN-1; n != -1; n--)	\
	{					\
		ptr = &(top)->Ptr[n]

#define MENU_LOOP(top)	do {	\
	Cardinal	l;			\
	LibraryMenuTypePtr menu;		\
	for (l = (top)->MenuN-1; l != -1; l--)	\
	{					\
		menu = &(top)->Menu[l]

#define ENTRY_LOOP(top) do	{	\
	Cardinal	n;			\
	LibraryEntryTypePtr entry;		\
	for (n = (top)->EntryN-1; n != -1; n--)	\
	{					\
		entry = &(top)->Entry[n]

#define GROUP_LOOP(data, group) do { 	\
	Cardinal entry; \
        for (entry = 0; entry < ((PCBTypePtr)(data->pcb))->LayerGroups.Number[(group)]; entry++) \
        { \
		LayerTypePtr layer;		\
		Cardinal number; 		\
		number = ((PCBTypePtr)(data->pcb))->LayerGroups.Entries[(group)][entry]; \
		if (number >= max_layer)	\
		  continue;			\
		layer = &data->Layer[number];

#define LAYER_LOOP(data, ml) do { \
        Cardinal n; \
	for (n = 0; n < ml; n++) \
	{ \
	   LayerTypePtr layer = (&data->Layer[(n)]);

#endif
