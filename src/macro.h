/*!
 * \file src/macro.h
 *
 * \brief Some commonly used macros not related to a special C-file.
 *
 * The file is included by global.h after const.h
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 */

#ifndef	PCB_MACRO_H
#define	PCB_MACRO_H

/* ---------------------------------------------------------------------------
 * macros to transform coord systems
 * draw.c uses a different definition of TO_SCREEN
 */
#ifndef	SWAP_IDENT
#define	SWAP_IDENT			Settings.ShowBottomSide
#endif

#define	SWAP_SIGN_X(x)		(x)
#define	SWAP_SIGN_Y(y)		(-(y))
#define	SWAP_ANGLE(a)		(-(a))
#define	SWAP_DELTA(d)		(-(d))
#define	SWAP_X(x)		(SWAP_SIGN_X(x))
#define	SWAP_Y(y)		(PCB->MaxHeight +SWAP_SIGN_Y(y))

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
#define	UNKNOWN_NAME(a, n)	((a) && *(a) ? (a) : (n))
#define NSTRCMP(a, b)		((a) ? ((b) ? strcmp((a),(b)) : 1) : -1)
#define	EMPTY(a)		((a) ? (a) : "")
#define	EMPTY_STRING_P(a)	((a) ? (a)[0]==0 : 1)
#define XOR(a,b)		(((a) && !(b)) || (!(a) && (b)))
#define SQUARE(x)		((float) (x) * (float) (x))
#define TO_RADIANS(degrees)	(M180 * (degrees))

/* Proper rounding for double -> Coord. */
#define DOUBLE_TO_COORD(x) ((x) >= 0 ? (Coord)((x) + 0.5) : (Coord)((x) - 0.5))

/* ---------------------------------------------------------------------------
 * layer macros
 */
#define	LAYER_ON_STACK(n)	(&PCB->Data->Layer[LayerStack[(n)]])
#define LAYER_PTR(n)            (&PCB->Data->Layer[(n)])
#define	CURRENT			(PCB->SilkActive ? &PCB->Data->Layer[ \
				(Settings.ShowBottomSide ? bottom_silk_layer : top_silk_layer)] \
				: LAYER_ON_STACK(0))
#define	INDEXOFCURRENT		(PCB->SilkActive ? \
				(Settings.ShowBottomSide ? bottom_silk_layer : top_silk_layer) \
				: LayerStack[0])
#define SILKLAYER		Layer[ \
				(Settings.ShowBottomSide ? bottom_silk_layer : top_silk_layer)]
#define BACKSILKLAYER		Layer[ \
				(Settings.ShowBottomSide ? top_silk_layer : bottom_silk_layer)]

#define TEST_SILK_LAYER(layer)	(GetLayerNumber (PCB->Data, layer) >= max_copper_layer)


/* ---------------------------------------------------------------------------
 * returns the object ID
 */
#define	OBJECT_ID(p)		(((AnyObjectType *) p)->ID)

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

/* ---------------------------------------------------------------------------
 *  Determines if text is actually visible
 */
#define TEXT_IS_VISIBLE(b, l, t) \
	((l)->On)

/* ---------------------------------------------------------------------------
 *  Determines if object is on front or back
 */
#define FRONT(o)	\
	((TEST_FLAG(ONSOLDERFLAG, (o)) != 0) == SWAP_IDENT)

/* ---------------------------------------------------------------------------
 *  Determines if an object is on the given side. side is either BOTTOM_GROUP
 *  or TOP_GROUP.
 */
#define ON_SIDE(element, side) \
        (TEST_FLAG (ONSOLDERFLAG, element) == (side == BOTTOM_SIDE))

/* ---------------------------------------------------------------------------
 * some loop shortcuts
 *
 * a pointer is created from index addressing because the base pointer
 * may change when new memory is allocated;
 *
 * all data is relativ to an objects name 'top' which can be either
 * PCB or PasteBuffer
 */
#define END_LOOP  }} while (0)

#define STYLE_LOOP(top)  do {                                       \
        Cardinal n;                                                 \
        RouteStyleType *style;                                      \
        for (n = 0; n < NUM_STYLES; n++)                            \
        {                                                           \
                style = &(top)->RouteStyle[n]

#define VIA_LOOP(top) do {                                          \
  GList *__iter, *__next;                                           \
  Cardinal n = 0;                                                   \
  for (__iter = (top)->Via, __next = g_list_next (__iter);          \
       __iter != NULL;                                              \
       __iter = __next, __next = g_list_next (__iter), n++) {       \
    PinType *via = __iter->data;

#define DRILL_LOOP(top) do             {               \
        Cardinal        n;                                      \
        DrillType *drill;                                       \
        for (n = 0; (top)->DrillN > 0 && n < (top)->DrillN; n++)                        \
        {                                                       \
                drill = &(top)->Drill[n]

#define NETLIST_LOOP(top) do   {                         \
        Cardinal        n;                                      \
        NetListType *netlist;                                   \
        for (n = (top)->NetListN-1; n != -1; n--)               \
        {                                                       \
                netlist = &(top)->NetList[n]

#define NET_LOOP(top) do   {                             \
        Cardinal        n;                                      \
        NetType *net;                                           \
        for (n = (top)->NetN-1; n != -1; n--)                   \
        {                                                       \
                net = &(top)->Net[n]

#define CONNECTION_LOOP(net) do {                         \
        Cardinal        n;                                      \
        ConnectionType *connection;                             \
        for (n = (net)->ConnectionN-1; n != -1; n--)            \
        {                                                       \
                connection = & (net)->Connection[n]

#define ELEMENT_LOOP(top) do {                                      \
  GList *__iter, *__next;                                           \
  Cardinal n = 0;                                                   \
  for (__iter = (top)->Element, __next = g_list_next (__iter);      \
       __iter != NULL;                                              \
       __iter = __next, __next = g_list_next (__iter), n++) {       \
    ElementType *element = __iter->data;

#define RAT_LOOP(top) do {                                          \
  GList *__iter, *__next;                                           \
  Cardinal n = 0;                                                   \
  for (__iter = (top)->Rat, __next = g_list_next (__iter);          \
       __iter != NULL;                                              \
       __iter = __next, __next = g_list_next (__iter), n++) {       \
    RatType *line = __iter->data;

#define	ELEMENTTEXT_LOOP(element) do { 	\
	Cardinal	n;				\
	TextType *text;					\
	for (n = MAX_ELEMENTNAMES-1; n != -1; n--)	\
	{						\
		text = &(element)->Name[n]


#define	ELEMENTNAME_LOOP(element) do	{ 			\
	Cardinal	n;					\
	char		*textstring;				\
	for (n = MAX_ELEMENTNAMES-1; n != -1; n--)		\
	{							\
		textstring = (element)->Name[n].TextString

#define PIN_LOOP(element) do {                                      \
  GList *__iter, *__next;                                           \
  Cardinal n = 0;                                                   \
  for (__iter = (element)->Pin, __next = g_list_next (__iter);      \
       __iter != NULL;                                              \
       __iter = __next, __next = g_list_next (__iter), n++) {       \
    PinType *pin = __iter->data;

#define PAD_LOOP(element) do {                                      \
  GList *__iter, *__next;                                           \
  Cardinal n = 0;                                                   \
  for (__iter = (element)->Pad, __next = g_list_next (__iter);      \
       __iter != NULL;                                              \
       __iter = __next, __next = g_list_next (__iter), n++) {       \
    PadType *pad = __iter->data;

#define ARC_LOOP(element) do {                                      \
  GList *__iter, *__next;                                           \
  Cardinal n = 0;                                                   \
  for (__iter = (element)->Arc, __next = g_list_next (__iter);      \
       __iter != NULL;                                              \
       __iter = __next, __next = g_list_next (__iter), n++) {       \
    ArcType *arc = __iter->data;

#define ELEMENTLINE_LOOP(element) do {                              \
  GList *__iter, *__next;                                           \
  Cardinal n = 0;                                                   \
  for (__iter = (element)->Line, __next = g_list_next (__iter);     \
       __iter != NULL;                                              \
       __iter = __next, __next = g_list_next (__iter), n++) {       \
    LineType *line = __iter->data;

#define ELEMENTARC_LOOP(element) do {                               \
  GList *__iter, *__next;                                           \
  Cardinal n = 0;                                                   \
  for (__iter = (element)->Arc, __next = g_list_next (__iter);      \
       __iter != NULL;                                              \
       __iter = __next, __next = g_list_next (__iter), n++) {       \
    ArcType *arc = __iter->data;

#define LINE_LOOP(layer) do {                                       \
  GList *__iter, *__next;                                           \
  Cardinal n = 0;                                                   \
  for (__iter = (layer)->Line, __next = g_list_next (__iter);       \
       __iter != NULL;                                              \
       __iter = __next, __next = g_list_next (__iter), n++) {       \
    LineType *line = __iter->data;

#define TEXT_LOOP(layer) do {                                       \
  GList *__iter, *__next;                                           \
  Cardinal n = 0;                                                   \
  for (__iter = (layer)->Text, __next = g_list_next (__iter);       \
       __iter != NULL;                                              \
       __iter = __next, __next = g_list_next (__iter), n++) {       \
    TextType *text = __iter->data;

#define POLYGON_LOOP(layer) do {                                    \
  GList *__iter, *__next;                                           \
  Cardinal n = 0;                                                   \
  for (__iter = (layer)->Polygon, __next = g_list_next (__iter);    \
       __iter != NULL;                                              \
       __iter = __next, __next = g_list_next (__iter), n++) {       \
    PolygonType *polygon = __iter->data;

#define	POLYGONPOINT_LOOP(polygon) do	{	\
	Cardinal			n;		\
	PointType *point;				\
	for (n = (polygon)->PointN-1; n != -1; n--)	\
	{						\
		point = &(polygon)->Points[n]

#define ENDALL_LOOP }} while (0); }} while(0)

#define	ALLPIN_LOOP(top)	\
        ELEMENT_LOOP(top); \
	        PIN_LOOP(element)\

#define	ALLPAD_LOOP(top) \
	ELEMENT_LOOP(top); \
	  PAD_LOOP(element)

#define	ALLLINE_LOOP(top) do	{		\
	Cardinal		l;			\
	LayerType *layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + SILK_LAYER; l++, layer++)	\
	{ \
		LINE_LOOP(layer)

#define ALLARC_LOOP(top) do {		\
	Cardinal		l;			\
	LayerType *layer = (top)->Layer;		\
	for (l =0; l < max_copper_layer + SILK_LAYER; l++, layer++)		\
	{ \
		ARC_LOOP(layer)

#define	ALLPOLYGON_LOOP(top)	do {		\
	Cardinal		l;			\
	LayerType *layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + SILK_LAYER; l++, layer++)	\
	{ \
		POLYGON_LOOP(layer)

#define	COPPERLINE_LOOP(top) do	{		\
	Cardinal		l;			\
	LayerType *layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer; l++, layer++)	\
	{ \
		LINE_LOOP(layer)

#define COPPERARC_LOOP(top) do	{		\
	Cardinal		l;			\
	LayerType *layer = (top)->Layer;		\
	for (l =0; l < max_copper_layer; l++, layer++)		\
	{ \
		ARC_LOOP(layer)

#define	COPPERPOLYGON_LOOP(top) do	{		\
	Cardinal		l;			\
	LayerType *layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer; l++, layer++)	\
	{ \
		POLYGON_LOOP(layer)

#define	SILKLINE_LOOP(top) do	{		\
	Cardinal		l;			\
	LayerType *layer = (top)->Layer;		\
	layer += max_copper_layer + BOTTOM_SILK_LAYER;			\
	for (l = 0; l < 2; l++, layer++)		\
	{ \
		LINE_LOOP(layer)

#define SILKARC_LOOP(top) do	{		\
	Cardinal		l;			\
	LayerType *layer = (top)->Layer;		\
	layer += max_copper_layer + BOTTOM_SILK_LAYER;			\
	for (l = 0; l < 2; l++, layer++)		\
	{ \
		ARC_LOOP(layer)

#define	SILKPOLYGON_LOOP(top) do	{		\
	Cardinal		l;			\
	LayerType *layer = (top)->Layer;		\
	layer += max_copper_layer + BOTTOM_SILK_LAYER;			\
	for (l = 0; l < 2; l++, layer++)		\
	{ \
		POLYGON_LOOP(layer)

#define	ALLTEXT_LOOP(top)	do {		\
	Cardinal		l;			\
	LayerType *layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + SILK_LAYER; l++, layer++)	\
	{ \
		TEXT_LOOP(layer)

#define	VISIBLELINE_LOOP(top) do	{		\
	Cardinal		l;			\
	LayerType *layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + SILK_LAYER; l++, layer++)	\
	{ \
		if (layer->On)				\
			LINE_LOOP(layer)

#define	VISIBLEARC_LOOP(top) do	{		\
	Cardinal		l;			\
	LayerType *layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + SILK_LAYER; l++, layer++)	\
	{ \
		if (layer->On)				\
			ARC_LOOP(layer)

#define	VISIBLETEXT_LOOP(board) do	{		\
	Cardinal		l;			\
	LayerType *layer = (board)->Data->Layer;		\
	for (l = 0; l < max_copper_layer + SILK_LAYER; l++, layer++)	\
	{ \
                TEXT_LOOP(layer);                                      \
                  if (TEXT_IS_VISIBLE((board), layer, text))

#define	VISIBLEPOLYGON_LOOP(top) do	{	\
	Cardinal		l;			\
	LayerType *layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + SILK_LAYER; l++, layer++)	\
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
	LibraryMenuType *menu;			\
	for (l = (top)->MenuN-1; l != -1; l--)	\
	{					\
		menu = &(top)->Menu[l]

#define ENTRY_LOOP(top) do	{	\
	Cardinal	n;			\
	LibraryEntryType *entry;		\
	for (n = (top)->EntryN-1; n != -1; n--)	\
	{					\
		entry = &(top)->Entry[n]

#define GROUP_LOOP(data, group) do { 	\
	Cardinal entry; \
        for (entry = 0; entry < ((PCBType *)(data->pcb))->LayerGroups.Number[(group)]; entry++) \
        { \
		LayerType *layer;		\
		Cardinal number; 		\
		number = ((PCBType *)(data->pcb))->LayerGroups.Entries[(group)][entry]; \
		if (number >= max_copper_layer)	\
		  continue;			\
		layer = &data->Layer[number];

#define LAYER_LOOP(data, ml) do { \
        Cardinal n; \
	for (n = 0; n < ml; n++) \
	{ \
	   LayerType *layer = (&data->Layer[(n)]);

#define LAYER_TYPE_LOOP(data, ml, type) do { \
        Cardinal n; \
        for (n = 0; n < ml; n++) { \
          LayerType *layer = (&data->Layer[(n)]); \
          if (layer->Type != (type)) \
            continue;

#endif

#define VIA_IS_BURIED(via) (via->BuriedFrom != 0 || via->BuriedTo != 0)
#define VIA_ON_LAYER(via, layer) (layer >= via->BuriedFrom && layer <= via->BuriedTo )
