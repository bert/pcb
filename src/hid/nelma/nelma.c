/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *
 *  NELMA (Numerical capacitance calculator) export HID
 *  Copyright (C) 2006 Tomaz Solc (tomaz.solc@tablix.org)
 *
 *  PNG export code is based on the PNG export HID
 *  Copyright (C) 2006 Dan McMahill
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
 */

/*
 * This HID exports a PCB layout into: o One layer mask file (PNG format) per
 * copper layer. o Nelma configuration file that contains netlist and pin
 * information.
 */

/*
 * FIXME:
 * 
 * If you have a section of a net that does not contain any pins then that
 * section will be missing from the Nelma's copper geometry.
 * 
 * For example:
 * 
 * this section will be ignored by Nelma |       |
 * 
 * ||             ||=======||            ||     component layer ||
 * ||       ||            || ||=============||       ||============||
 * solder layer
 * 
 * pin1           via      via           pin2
 * 
 * Single layer layouts are always exported correctly.
 * 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <time.h>

#include "global.h"
#include "error.h" /* Message() */
#include "data.h"
#include "misc.h"
#include "rats.h"

#include "hid.h"
#include "hid_draw.h"
#include "../hidint.h"
#include "hid/common/hidnogui.h"
#include "hid/common/draw_helpers.h"

#include <gd.h>
#include "xmlout.h"

#include "hid/common/hidinit.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented PNG function %s.\n", __FUNCTION__); abort()

/* Needed for PNG export */

struct color_struct {
	/* the descriptor used by the gd library */
	int             c;

	/* so I can figure out what rgb value c refers to */
	unsigned int    r, g, b;
};
static void nelma_write_xnets(void);

struct hid_gc_struct {
	HID            *me_pointer;
	EndCapStyle     cap;
	Coord           width;
	unsigned char   r, g, b;
	int             erase;
	struct color_struct *color;
	gdImagePtr      brush;
};

struct nelma_net_layer {
	GList *Line;
	GList *Arc; 
	GList *Polygon;
};

struct nelma_netlist {
	char* name;
	GList *Pin;
	GList *Via;
	GList *Pad;
	struct nelma_net_layer layer[MAX_LAYER];
	struct color_struct color;
	int colorIndex;
};

struct nelma_netlist* nelma_netlist = NULL;
int hashColor = gdBrushed;
static struct color_struct* color_array[0x100];

static HID nelma_hid;
static HID_DRAW nelma_graphics;

static struct color_struct *black = NULL, *white = NULL;
static Coord    linewidth = -1;
static gdImagePtr lastbrush = (gdImagePtr)((void *) -1);

/* gd image and file for PNG export */
static gdImagePtr nelma_im = NULL;
static FILE    *nelma_f = NULL;

static int      is_mask;
static int      is_drill;

/*
 * Which groups of layers to export into PNG layer masks. 1 means export, 0
 * means do not export.
 */
static int      nelma_export_group[MAX_GROUP];

/* Group that is currently exported. */
static int      nelma_cur_group;

/* Filename prefix that will be used when saving files. */
static const char *nelma_basename = NULL;

/* Horizontal DPI (grid points per inch) */
static int      nelma_dpi = -1;

/* Height of the copper layers in micrometers. */

/*
 * The height of the copper layer is currently taken as the vertical grid
 * step, since this is the smallest vertical feature in the layout.
 */
static int      nelma_copperh = -1;
/* Height of the substrate layers in micrometers. */
static int      nelma_substrateh = -1;
/* Relative permittivity of the substrate. */
static double   nelma_substratee = -1;

/* Permittivity of empty space (As/Vm) */
static const double nelma_air_epsilon = 8.85e-12;

HID_Attribute   nelma_attribute_list[] = {
	/* other HIDs expect this to be first.  */

/* %start-doc options "nelma Options"
@ftable @code
@item -- basename <string>
File name prefix.
@end ftable
%end-doc
*/
	{"basename", "File name prefix",
	HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_basename 0

/* %start-doc options "nelma Options"
@ftable @code
@item --dpi <num>
Horizontal scale factor (grid points/inch).
@end ftable
%end-doc
*/
	{"dpi", "Horizontal scale factor (grid points/inch)",
	HID_Integer, 0, 1000, {1000, 0, 0}, 0, 0},	//1000 --> 1mil (25.4um) resolution 
#define HA_dpi 1

/* %start-doc options "nelma Options"
@ftable @code
@item --copper-height <num>
Copper layer height (um).
@end ftable
%end-doc
*/
	{"copper-height", "Copper layer height (um)",
	HID_Integer, 0, 200, {35, 0, 0}, 0, 0},	// 1oz cu-->34.79um
#define HA_copperh 2

/* %start-doc options "nelma Options"
@ftable @code
@item --substrate-height <num>
Substrate layer height (um).
@end ftable
%end-doc
*/
	{"substrate-height", "Substrate layer height (um)",
	HID_Integer, 0, 10000, {1588, 0, 0}, 0, 0},	// 62.5mil --> 1588um
#define HA_substrateh 3

/* %start-doc options "nelma Options"
@ftable @code
@item --substrate-epsilon <num>
Substrate relative epsilon.
@end ftable
%end-doc
*/
	{"substrate-epsilon", "Substrate relative epsilon",
	HID_Real, 0, 100, {0, 0, 4.4}, 0, 0},  // FR-4 Er:4.4
#define HA_substratee 4
};

#define NUM_OPTIONS (sizeof(nelma_attribute_list)/sizeof(nelma_attribute_list[0]))

REGISTER_ATTRIBUTES(nelma_attribute_list)
	static HID_Attr_Val nelma_values[NUM_OPTIONS];

void nelma_create_netlist(void);
void nelma_destroy_netlist(void);

/* *** Utility funcions **************************************************** */

// from http://www.rapidtables.com/convert/color/hsl-to-rgb.htm
// converts from (H)ue (S)aturation and (L)ightness to (R)ed (G)reen (B)lue
// 
// h range 0 - 365 degrees
// s range 0 - 255
// l range 0 - 255
// returns rgb array of chars in range of 0-255 0-->R, 1-->G, 2-->B
// always returns a result,  wraps 'h' if > 360 degrees
char* hslToRgb(int h, uint8_t s, uint8_t l)
{
	static char rgb[3];
	float H = fmod((float)(h),360.0);
	float S = s;
	float L = l;

	int sect;
	float C, X;
	float m;
	float Rp, Gp, Bp;

	L/=255.0;
	S/=255.0;
	sect = h/60;

	C = (1.0 - fabs(2*L - 1.0)) * S;
	X = C * (1.0 - fabs(fmod((H/60.0),2.0)  - 1.0));
	m = L - C/2.0;

	
	
	switch(sect)
	{
	case 0:
		Rp = C;
		Gp = X;
		Bp = 0.0;	
	break;
	case 1:
		Rp = X;
		Gp = C;
		Bp = 0.0;	
	break;
	case 2:
		Rp = 0.0;
		Gp = C;
		Bp = X;	
	break;
	case 3:
		Rp = 0.0;
		Gp = X;
		Bp = C;	
	break;
	case 4:
		Rp = X;
		Gp = 0.0;
		Bp = C;	
	break;
	case 5:
		Rp = C;
		Gp = 0.0;
		Bp = X;	
	break;
	default:
		Rp = 0.0;
		Gp = 0.0;
		Bp = 0.0;	
	break;
	}
	
	rgb[0] = (Rp+m)*0xFF;
	rgb[1] = (Gp+m)*0xFF;
	rgb[2] = (Bp+m)*0xFF;

	return(rgb);
}

void nelma_build_net_from_selected(struct nelma_netlist* currNet)
{
	COPPERLINE_LOOP(PCB->Data);
	{
		if(TEST_FLAG (SELECTEDFLAG, line))
		{
			currNet->layer[l].Line = g_list_prepend(currNet->layer[l].Line, line);
		}
	}	
	ENDALL_LOOP;

	COPPERARC_LOOP(PCB->Data);
	{
		if(TEST_FLAG (SELECTEDFLAG, arc))
		{
			currNet->layer[l].Arc = g_list_prepend(currNet->layer[l].Arc, arc);
		}
	}	
	ENDALL_LOOP;

	COPPERPOLYGON_LOOP(PCB->Data);
	{
		if(TEST_FLAG (SELECTEDFLAG, polygon))
		{
			currNet->layer[l].Polygon = g_list_prepend(currNet->layer[l].Polygon, polygon);
		}
	}	
	ENDALL_LOOP;
	
	ALLPAD_LOOP(PCB->Data);
	{
		if(TEST_FLAG (SELECTEDFLAG, pad))
		{
			currNet->Pad = g_list_prepend(currNet->Pad, pad);
		}
	}	
	ENDALL_LOOP;

	ALLPIN_LOOP(PCB->Data);
	{
		if(TEST_FLAG (SELECTEDFLAG, pin))
		{
			currNet->Pin = g_list_prepend(currNet->Pin, pin);
		}
	}	
	ENDALL_LOOP;
	
	VIA_LOOP(PCB->Data);
	{
		if(TEST_FLAG (SELECTEDFLAG, via))
		{
			currNet->Via = g_list_prepend(currNet->Via, via);
		}
	}	
	END_LOOP;
}


void nelma_create_netlist(void)
{
	int i;
	int numNets = PCB->NetlistLib.MenuN;
	nelma_netlist = (struct nelma_netlist*)malloc(sizeof(struct nelma_netlist)*numNets);
	memset(nelma_netlist,0,sizeof(struct nelma_netlist)*numNets);

	for(i=0; i<numNets; i++)
	{ // for each net in the netlist
		int j;
		LibraryEntryType* entry;
		ConnectionType conn;

		struct nelma_netlist* currNet = &nelma_netlist[i];
		currNet->name = PCB->NetlistLib.Menu[i].Name+2;
		// add fancy color attachment here

		InitConnectionLookup();
		ClearFlagOnAllObjects (false, FOUNDFLAG | SELECTEDFLAG);
		for (j = PCB->NetlistLib.Menu[i].EntryN, entry = PCB->NetlistLib.Menu[i].Entry; j; j--, entry++)
		{ // for each component (pin/pad) in the net
			if (SeekPad(entry, &conn, false))
			{
				RatFindHook(conn.type, conn.ptr1, conn.ptr2, conn.ptr2, false, SELECTEDFLAG, false);
			}
		}
		// the conn should now contain a list of all elements that are part of the net
		// now build a database of all things selected as part of this net
		nelma_build_net_from_selected(currNet);
		
		ClearFlagOnAllObjects (false, FOUNDFLAG | SELECTEDFLAG);
		FreeConnectionLookupMemory ();
	}

	// assign colors to nets		
	for(i=0; i<numNets; i++)
	{
		char* rgb = NULL;
		char name[0x100];
		int j;
		int phase = (360*i)/numNets;

		for(j=0; nelma_netlist[i].name[j]; j++)
		{
			name[j] = tolower(nelma_netlist[i].name[j]);
		}		
		name[j] = 0;

		if( strstr(name, "gnd") || strstr(name, "ground") )
		{// make gnd nets darker
			rgb = hslToRgb(phase, (70*256)/100, (20*256)/100);
		}
		else if( strstr(name, "unnamed") )
		{// make unnamed nets grayer
			rgb = hslToRgb(phase, (15*256)/100, (40*256)/100);
		}
		else
			rgb = hslToRgb(phase, (70*256)/100, (50*256)/100);
		nelma_netlist[i].color.r = rgb[0];
		nelma_netlist[i].color.g = rgb[1];
		nelma_netlist[i].color.b = rgb[2];
	}
}


void nelma_destroy_netlist(void)
{
	int i;
	for(i=0; i<PCB->NetlistLib.MenuN; i++)
	{ // for each net in the netlist
		struct nelma_netlist* currNet = &nelma_netlist[i];
		int j;
		for(j=0;j<MAX_LAYER;j++)
		{
			g_list_free(currNet->layer[j].Line);
			g_list_free(currNet->layer[j].Arc);
			g_list_free(currNet->layer[j].Polygon);
		}
		g_list_free(currNet->Pad);
		g_list_free(currNet->Pin);
		g_list_free(currNet->Via);
	}
	free(nelma_netlist);
}



/* convert from default PCB units to nelma units */
static int pcb_to_nelma (Coord pcb)
{
  return COORD_TO_INCH(pcb) * nelma_dpi;
}

static char    *
nelma_get_png_name(const char *basename, const char *suffix)
{
	char           *buf;
	int             len;

	len = strlen(basename) + strlen(suffix) + 6;
	buf = (char *)malloc(sizeof(*buf) * len);

	sprintf(buf, "%s.%s.png", basename, suffix);

	return buf;
}

/* Retrieves coordinates (in default PCB units) of a pin or pad. */
/* Copied from netlist.c */
static int 
pin_name_to_xy (LibraryEntryType * pin, Coord *x, Coord *y)
{
	ConnectionType  conn;
	if (!SeekPad(pin, &conn, false))
		return 1;
	switch (conn.type) {
	case PIN_TYPE:
		*x = ((PinType *) (conn.ptr2))->X;
		*y = ((PinType *) (conn.ptr2))->Y;
		return 0;
	case PAD_TYPE:
		*x = ((PadType *) (conn.ptr2))->Point1.X;
		*y = ((PadType *) (conn.ptr2))->Point1.Y;
		return 0;
	}
	return 1;
}

/* *** Exporting netlist data and geometry to the nelma config file ******** */

static void 
nelma_write_space(FILE * out)
{
	double          xh, zh;

	int             z;
	int             i, idx;
	const char     *ext;

	xh = 2.54e-2 / ((double) nelma_dpi);
	zh = nelma_copperh * 1e-6;

	fprintf(out, "\n/* **** Space **** */\n\n");

	fprintf(out, "space pcb {\n");
	fprintf(out, "\tstep = { %e, %e, %e }\n", xh, xh, zh);
	fprintf(out, "\tlayers = {\n");

	fprintf(out, "\t\t\"air-top\",\n");
	fprintf(out, "\t\t\"air-bottom\"");

	z = 10;
	for (i = 0; i < MAX_GROUP; i++)
		if (nelma_export_group[i]) {
			idx = (i >= 0 && i < max_group) ?
				PCB->LayerGroups.Entries[i][0] : i;
			ext = layer_type_to_file_name(idx, FNS_fixed);

			if (z != 10) {
				fprintf(out, ",\n");
				fprintf(out, "\t\t\"substrate-%d\"", z);
				z++;
			}
			fprintf(out, ",\n");
			fprintf(out, "\t\t\"%s\"", ext);
			z++;
		}
	fprintf(out, "\n\t}\n");
	fprintf(out, "}\n");
}

static void nelma_write_xspace(void)
{
	double          xh;
	uint32_t h;
	uint32_t w;
	
	char buff[0x100];

	int             i, idx;
	const char     *ext;
	char* src;

	xh = (1000.0* 2.54e-2) / ((double) nelma_dpi);  // units are in mm
	h = (uint32_t)pcb_to_nelma(PCB->MaxHeight);	// units are in counts
	w = (uint32_t)pcb_to_nelma(PCB->MaxWidth);
	               
	sprintf(buff,"%f",xh);
	XOUT_ELEMENT_START("space");
	XOUT_NEWLINE();
	XOUT_ELEMENT("comment"," **** Space **** ");
	
	XOUT_ELEMENT_ATTR("resolution", "units", "mm", buff);

	sprintf(buff,"%d",w);
	XOUT_ELEMENT("width",buff);
	sprintf(buff,"%d",h);
//	XOUT_DETENT();
	XOUT_ELEMENT("height",buff);
	XOUT_ELEMENT_START("layers");
	XOUT_NEWLINE();
	for (i = 0; i < MAX_GROUP; i++)
		if (nelma_export_group[i]) {
			idx = (i >= 0 && i < max_group) ? PCB->LayerGroups.Entries[i][0] : i;
			ext = layer_type_to_file_name(idx, FNS_fixed);
			src = nelma_get_png_name(nelma_basename,ext);
			XOUT_ELEMENT_ATTR("layer", "name", ext, src);
//
//			if (z != 10)
//			{
//				fprintf(out, ",\n");
//				fprintf(out, "\t\t\"substrate-%d\"", z);
//				z++;
//			}
//			fprintf(out, ",\n");
//			fprintf(out, "\t\t\"%s\"", ext);
//			z++;
		}
	XOUT_DETENT();
	XOUT_NEWLINE();	
	XOUT_ELEMENT_END("layers");
	XOUT_NEWLINE();
	XOUT_ELEMENT_END("space");
	XOUT_INDENT();
	XOUT_NEWLINE();
}


static void 
nelma_write_material(FILE * out, char *name, char *type, double e)
{
	fprintf(out, "material %s {\n", name);
	fprintf(out, "\ttype = \"%s\"\n", type);
	fprintf(out, "\tpermittivity = %e\n", e);
	fprintf(out, "\tconductivity = 0.0\n");
	fprintf(out, "\tpermeability = 0.0\n");
	fprintf(out, "}\n");
}

static void 
nelma_write_materials(FILE * out)
{
	fprintf(out, "\n/* **** Materials **** */\n\n");

	nelma_write_material(out, "copper", "metal", nelma_air_epsilon);
	nelma_write_material(out, "air", "dielectric", nelma_air_epsilon);
	nelma_write_material(out, "composite", "dielectric",
			     nelma_air_epsilon * nelma_substratee);
}

static void 
nelma_write_nets(FILE * out)
{
	LibraryType     netlist;
	LibraryMenuType *net;
	LibraryEntryType *pin;

	int             n, m, i, idx;

	const char     *ext;

	netlist = PCB->NetlistLib;

	fprintf(out, "\n/* **** Nets **** */\n\n");

	for (n = 0; n < netlist.MenuN; n++) {
		net = &netlist.Menu[n];

		/* Weird, but correct */
		fprintf(out, "net %s {\n", &net->Name[2]);

		fprintf(out, "\tobjects = {\n");

		for (m = 0; m < net->EntryN; m++) {
			pin = &net->Entry[m];

			/* pin_name_to_xy(pin, &x, &y); */

			for (i = 0; i < MAX_GROUP; i++)
				if (nelma_export_group[i]) {
					idx = (i >= 0 && i < max_group) ?
						PCB->LayerGroups.Entries[i][0] : i;
					ext = layer_type_to_file_name(idx, FNS_fixed);

					if (m != 0 || i != 0)
						fprintf(out, ",\n");
					fprintf(out, "\t\t\"%s-%s\"", pin->ListEntry,
						ext);
				}
		}

		fprintf(out, "\n");
		fprintf(out, "\t}\n");
		fprintf(out, "}\n");
	}
}

static void nelma_write_xnets(void)
{
	LibraryType     netlist;
	LibraryMenuType *net;
	LibraryEntryType *pin;

	int             n, m, i, idx;

	const char     *ext;

	netlist = PCB->NetlistLib;

	XOUT_ELEMENT_START("nets");
	XOUT_NEWLINE();
	XOUT_ELEMENT("comment", "***** Nets ****");

	for (n = 0; n < netlist.MenuN; n++)
	{
		net = &netlist.Menu[n];

		/* Weird, but correct */
		XOUT_ELEMENT_ATTR_START("net", "name", &net->Name[2]);

		for (m = 0; m < net->EntryN; m++)
		{
			pin = &net->Entry[m];

			/* pin_name_to_xy(pin, &x, &y); */

			for (i = 0; i < MAX_GROUP; i++)
				if (nelma_export_group[i])
				{
					char buff[0x100];
					idx = (i >= 0 && i < max_group) ? PCB->LayerGroups.Entries[i][0] : i;
					ext = layer_type_to_file_name(idx, FNS_fixed);

					if (m != 0 || i != 0)
						XOUT_ELEMENT_DATA(", ");
					snprintf(buff, 0x100, "%s", pin->ListEntry);
					{
					char* src = buff;
					while(*src )
					{
						if(*src == '-')
							*src = '.';
						src++;
					}
					}
										
					XOUT_ELEMENT_DATA(buff);
					break;
				}
		}
		XOUT_ELEMENT_END("net");
		if( n+1 >= netlist.MenuN)
			XOUT_DETENT();
		XOUT_NEWLINE();
	}
	XOUT_ELEMENT_END("nets");
	XOUT_DETENT();
	XOUT_NEWLINE();
}

static void 
nelma_write_layer(FILE * out, int z, int h,
		  const char *name, int full,
		  char *mat)
{
	LibraryType     netlist;
	LibraryMenuType *net;
	LibraryEntryType *pin;

	int             n, m;

	fprintf(out, "layer %s {\n", name);
	fprintf(out, "\theight = %d\n", h);
	fprintf(out, "\tz-order = %d\n", z);
	fprintf(out, "\tmaterial = \"%s\"\n", mat);

	if (full) {
		fprintf(out, "\tobjects = {\n");
		netlist = PCB->NetlistLib;

		for (n = 0; n < netlist.MenuN; n++) {
			net = &netlist.Menu[n];

			for (m = 0; m < net->EntryN; m++) {
				pin = &net->Entry[m];

				if (m != 0 || n != 0)
					fprintf(out, ",\n");
				fprintf(out, "\t\t\"%s-%s\"", pin->ListEntry,
					name);
			}

		}
		fprintf(out, "\n\t}\n");
	}
	fprintf(out, "}\n");
}

static void 
nelma_write_layers(FILE * out)
{
	int             i, idx;
	int             z;

	const char     *ext;
	char            buf[100];

	int             subh;

	subh = nelma_substrateh / nelma_copperh;

	fprintf(out, "\n/* **** Layers **** */\n\n");

	/* Air layers on top and bottom of the stack */
	/* Their height is double substrate height. */
	nelma_write_layer(out, 1, 2 * subh, "air-top", 0, "air");
	nelma_write_layer(out, 1000, 2 * subh, "air-bottom", 0, "air");

	z = 10;
	for (i = 0; i < MAX_GROUP; i++)
		if (nelma_export_group[i]) {
			idx = (i >= 0 && i < max_group) ?
				PCB->LayerGroups.Entries[i][0] : i;
			ext = layer_type_to_file_name(idx, FNS_fixed);

			if (z != 10) {
				sprintf(buf, "substrate-%d", z);
				nelma_write_layer(out,
						  z,
						  subh,
						  buf,
						  0,
						  "composite");
				z++;
			}
			/*
			 * FIXME: for layers that are not on top or bottom,
			 * the material should be "composite"
			 */
			nelma_write_layer(out, z, 1, ext, 1, "air");

			z++;
		}
}

static void 
nelma_write_object(FILE * out, LibraryEntryType *pin)
{
	int             i, idx;
	Coord           px = 0, py = 0;
	int             x, y;

	char           *f;
	const char     *ext;

	pin_name_to_xy (pin, &px, &py);

	x = pcb_to_nelma (px);
	y = pcb_to_nelma (py);

	for (i = 0; i < MAX_GROUP; i++)
		if (nelma_export_group[i]) {
			idx = (i >= 0 && i < max_group) ?
				PCB->LayerGroups.Entries[i][0] : i;
			ext = layer_type_to_file_name(idx, FNS_fixed);

			fprintf(out, "object %s-%s {\n", pin->ListEntry, ext);
			fprintf(out, "\tposition = { 0, 0 }\n");
			fprintf(out, "\tmaterial = \"copper\"\n");
			fprintf(out, "\ttype = \"image\"\n");
			fprintf(out, "\trole = \"net\"\n");

			f = nelma_get_png_name(nelma_basename, ext);

			fprintf(out, "\tfile = \"%s\"\n", f);

			free(f);

			fprintf(out, "\tfile-pos = { %d, %d }\n", x, y);
			fprintf(out, "}\n");
		}
}

static void 
nelma_write_objects(FILE * out)
{
	LibraryType     netlist;
	LibraryMenuType *net;
	LibraryEntryType *pin;

	int             n, m;

	netlist = PCB->NetlistLib;

	fprintf(out, "\n/* **** Objects **** */\n\n");

	for (n = 0; n < netlist.MenuN; n++) {
		net = &netlist.Menu[n];

		for (m = 0; m < net->EntryN; m++) {
			pin = &net->Entry[m];

			nelma_write_object(out, pin);
		}
	}
}

/* *** Main export callback ************************************************ */

static void 
nelma_parse_arguments(int *argc, char ***argv)
{
	hid_register_attributes(nelma_attribute_list,
				sizeof(nelma_attribute_list) /
				sizeof(nelma_attribute_list[0]));
	hid_parse_command_line(argc, argv);
}

static HID_Attribute *
nelma_get_export_options(int *n)
{
	static char    *last_made_filename = 0;

	if (PCB) {
		derive_default_filename(PCB->Filename,
					&nelma_attribute_list[HA_basename],
					".nelma",
					&last_made_filename);
	}
	if (n) {
		*n = NUM_OPTIONS;
	}
	return nelma_attribute_list;
}

/* Populates nelma_export_group array */
void 
nelma_choose_groups()
{
	int             n, m;
	LayerType      *layer;

	/* Set entire array to 0 (don't export any layer groups by default */
	memset(nelma_export_group, 0, sizeof(nelma_export_group));

	for (n = 0; n < max_copper_layer; n++) {
		layer = &PCB->Data->Layer[n];

		if (layer->LineN || layer->TextN || layer->ArcN ||
		    layer->PolygonN) {
			/* layer isn't empty */

			/*
			 * is this check necessary? It seems that special
			 * layers have negative indexes?
			 */

			if (SL_TYPE(n) == 0) {
				/* layer is a copper layer */
				m = GetLayerGroupNumberByNumber(n);

				/* the export layer */
				nelma_export_group[m] = 1;
			}
		}
	}
}

static void 
nelma_alloc_colors()
{
	/*
	 * Allocate white and black -- the first color allocated becomes the
	 * background color
	 */
	int i;
	int numNets = PCB->NetlistLib.MenuN;
	char* rgb;

	white = (struct color_struct *) malloc(sizeof(*white));
	white->r = white->g = white->b = 255;
	white->c = gdImageColorAllocate(nelma_im, white->r, white->g, white->b);

	black = (struct color_struct *) malloc(sizeof(*black));
	black->r = black->g = black->b = 0;
	black->c = gdImageColorAllocate(nelma_im, black->r, black->g, black->b);
	

	for(i=0;i<numNets; i++)
	{
		nelma_netlist[i].colorIndex = i;	
		color_array[i] =  malloc(sizeof(*white));
		color_array[i]->r = nelma_netlist[i].color.r;
		color_array[i]->g = nelma_netlist[i].color.g;
		color_array[i]->b = nelma_netlist[i].color.b;
//		printf("%d %d <%02x:%02x:%02x>\n", i, phase, (uint8_t)(color_array[i]->r), (uint8_t)(color_array[i]->g), (uint8_t)(color_array[i]->b) );

		color_array[i]->c = gdImageColorAllocate(nelma_im, color_array[i]->r,  color_array[i]->g,  color_array[i]->b);
	}
	
	color_array[i] =  malloc(sizeof(*white));
	rgb = hslToRgb(128, (20*256)/100, (20*256)/100);
		
	color_array[i]->r = rgb[0];
	color_array[i]->g = rgb[1];
	color_array[i]->b = rgb[2];
	color_array[i]->c = gdImageColorAllocate(nelma_im, color_array[i]->r,  color_array[i]->g,  color_array[i]->b);

	printf("%d colors allocated\n", numNets);
}

static void 
nelma_start_png(const char *basename, const char *suffix)
{
	int             h, w;
	char           *buf;

	buf = nelma_get_png_name(basename, suffix);

	h = pcb_to_nelma(PCB->MaxHeight);
	w = pcb_to_nelma(PCB->MaxWidth);

	/* nelma_im = gdImageCreate (w, h); */

	/* Nelma only works with true color images */
	nelma_im = gdImageCreate(w, h);
	nelma_f = fopen(buf, "wb");

	nelma_alloc_colors();

	free(buf);
}

static void 
nelma_finish_png()
{
	int  i;
#ifdef HAVE_GDIMAGEPNG
	gdImagePng(nelma_im, nelma_f);
#else
	Message("NELMA: PNG not supported by gd. Can't write layer mask.\n");
#endif
	gdImageDestroy(nelma_im);
	fclose(nelma_f);

	free(white);
	free(black);
	for(i=0;i<0x100;i++)
	{
		free(color_array[i]);
	}

	nelma_im = NULL;
	nelma_f = NULL;
}

void 
nelma_start_png_export()
{
	BoxType         region;

	region.X1 = 0;
	region.Y1 = 0;
	region.X2 = PCB->MaxWidth;
	region.Y2 = PCB->MaxHeight;

	linewidth = -1;
	lastbrush = (gdImagePtr)((void *) -1);

	hid_expose_callback(&nelma_hid, &region, 0);
}
void nelma_xml_out(char* nelma_basename);

static void 
nelma_do_export(HID_Attr_Val * options)
{
	int             save_ons[MAX_ALL_LAYER];
	int             i, idx;
	FILE           *nelma_config;
	char           *buf;
	int             len;

	time_t          t;

	if (!options) {
		nelma_get_export_options(0);
		for (i = 0; i < NUM_OPTIONS; i++) {
			nelma_values[i] = nelma_attribute_list[i].default_val;
		}
		options = nelma_values;
	}
	nelma_basename = options[HA_basename].str_value;
	if (!nelma_basename) {
		nelma_basename = "pcb-out";
	}
	nelma_dpi = options[HA_dpi].int_value;
	if (nelma_dpi < 0) {
		fprintf(stderr, "ERROR:  dpi may not be < 0\n");
		return;
	}
	nelma_copperh = options[HA_copperh].int_value;
	nelma_substrateh = options[HA_substrateh].int_value;
	nelma_substratee = options[HA_substratee].real_value;

	nelma_create_netlist();
	nelma_choose_groups();

	for (i = 0; i < MAX_GROUP; i++) {
		if (nelma_export_group[i]) {

			nelma_cur_group = i;

			/* magic */
			idx = (i >= 0 && i < max_group) ?
				PCB->LayerGroups.Entries[i][0] : i;

			nelma_start_png(nelma_basename,
					layer_type_to_file_name(idx, FNS_fixed));

			hid_save_and_show_layer_ons(save_ons);
			nelma_start_png_export();
			hid_restore_layer_ons(save_ons);

			nelma_finish_png();
		}
	}

	len = strlen(nelma_basename) + 4;
	buf = (char *)malloc(sizeof(*buf) * len);

	sprintf(buf, "%s.em", nelma_basename);
	nelma_config = fopen(buf, "w");

	free(buf);

	fprintf(nelma_config, "/* Made with PCB Nelma export HID */");
	t = time(NULL);
	fprintf(nelma_config, "/* %s */", ctime(&t));

	nelma_write_nets(nelma_config);
	nelma_write_objects(nelma_config);
	nelma_write_layers(nelma_config);
	nelma_write_materials(nelma_config);
	nelma_write_space(nelma_config);

	fclose(nelma_config);
	nelma_xml_out((char*)nelma_basename);
	nelma_destroy_netlist();
}

void nelma_xml_out(char* nelma_basename)
{
	char           *buf;
	int             len;
	time_t          t;

	len = strlen(nelma_basename) + 4;
	buf = (char *)malloc(sizeof(*buf) * len);

	sprintf(buf, "%s.xem", nelma_basename);
	XOUT_INIT(buf);
	free(buf);

	XOUT_HEADER();
	XOUT_ELEMENT_START("nelma");
	XOUT_NEWLINE();
	XOUT_ELEMENT("comment", "Made with PCB Nelma export HID");
	{
	char buff[0x100];
	char* src = buff;
	t = time(NULL);
	strncpy(buff,ctime(&t), 0x100);
	while(*src)
	{
		if((*src =='\r') || (*src =='\n'))
		{
			*src = 0;
			break;
		}
		src++;
	}
	XOUT_ELEMENT("genTime", buff);
	}
	nelma_write_xspace();
	nelma_write_xnets();
//	nelma_write_objects(nelma_config);
//	nelma_write_layers(nelma_config);
//	nelma_write_materials(nelma_config);
//	nelma_write_space(nelma_config);

	XOUT_ELEMENT_END( "nelma");		
	XOUT_NEWLINE();
	XOUT_CLOSE();
}

/* *** PNG export (slightly modified code from PNG export HID) ************* */

static int 
nelma_set_layer(const char *name, int group, int empty)
{
	int             idx = (group >= 0 && group < max_group) ?
	PCB->LayerGroups.Entries[group][0] : group;

	if (name == 0) {
		name = PCB->Data->Layer[idx].Name;
	}
	if (strcmp(name, "invisible") == 0) {
		return 0;
	}
	is_drill = (SL_TYPE(idx) == SL_PDRILL || SL_TYPE(idx) == SL_UDRILL);
	is_mask = (SL_TYPE(idx) == SL_MASK);

	if (is_mask) {
		/* Don't print masks */
		return 0;
	}
	if (is_drill) {
		/*
		 * Print 'holes', so that we can fill gaps in the copper
		 * layer
		 */
		return 1;
	}
	if (group == nelma_cur_group) {
		return 1;
	}
	return 0;
}

static hidGC 
nelma_make_gc(void)
{
	hidGC           rv = (hidGC) malloc(sizeof(struct hid_gc_struct));
	rv->me_pointer = &nelma_hid;
	rv->cap = Trace_Cap;
	rv->width = 1;
	rv->color = (struct color_struct *) malloc(sizeof(*rv->color));
	rv->color->r = rv->color->g = rv->color->b = 0;
	rv->color->c = 0;
	return rv;
}

static void 
nelma_destroy_gc(hidGC gc)
{
	free(gc);
}

static void 
nelma_use_mask(enum mask_mode mode)
{
	/* does nothing */
}

static void 
nelma_set_color(hidGC gc, const char *name)
{
	if (nelma_im == NULL) {
		return;
	}
	if (name == NULL) {
		name = "#ff0000";
	}
	if (!strcmp(name, "drill")) {
		gc->color = black;
		gc->erase = 0;
		return;
	}
	if (!strcmp(name, "erase")) {
		/* FIXME -- should be background, not white */
		gc->color = white;
		gc->erase = 1;
		return;
	}
	gc->color = black;
	gc->erase = 0;
	return;
}

static void
nelma_set_line_cap(hidGC gc, EndCapStyle style)
{
	gc->cap = style;
}

static void
nelma_set_line_width(hidGC gc, Coord width)
{
	gc->width = width;
}

static void
nelma_set_draw_xor(hidGC gc, int xor_)
{
	;
}

static void
nelma_set_draw_faded(hidGC gc, int faded)
{
}

static void
use_gc(hidGC gc)
{
	int             need_brush = 0;

	if (gc->me_pointer != &nelma_hid) {
		fprintf(stderr, "Fatal: GC from another HID passed to nelma HID\n");
		abort();
	}

	if(hashColor != gdBrushed)
	{
		need_brush = 1;
	}

	if (linewidth != gc->width) {
		/* Make sure the scaling doesn't erase lines completely */
		/*
	        if (SCALE (gc->width) == 0 && gc->width > 0)
		  gdImageSetThickness (im, 1);
	        else
	        */
		gdImageSetThickness(nelma_im, pcb_to_nelma(gc->width));
		linewidth = gc->width;
		need_brush = 1;
	}
	if (lastbrush != gc->brush || need_brush) {
		static void    *bcache = 0;
		hidval          bval;
		char            name[256];
		char            type;
		int             r;

		switch (gc->cap) {
		case Round_Cap:
		case Trace_Cap:
			type = 'C';
			r = pcb_to_nelma(gc->width / 2);
			break;
		default:
		case Square_Cap:
			r = pcb_to_nelma(gc->width);
			type = 'S';
			break;
		}
		sprintf(name, "#%.2x%.2x%.2x_%c_%d", gc->color->r, gc->color->g,
			gc->color->b, type, r);

//		if (hid_cache_color(0, name, &bval, &bcache)) {
//		  gc->brush = (gdImagePtr)bval.ptr;
//		} else {
{
			int             bg, fg;
			if (type == 'C')
				gc->brush = gdImageCreate(2 * r + 1, 2 * r + 1);
			else
				gc->brush = gdImageCreate(r + 1, r + 1);
			bg = gdImageColorAllocate(gc->brush, 255, 255, 255);
			if(hashColor != gdBrushed)
			{
//			printf("hash:%d\n",hashColor);
				fg = gdImageColorAllocate(gc->brush, color_array[hashColor]->r,color_array[hashColor]->g,color_array[hashColor]->b);
			}
			else
			{
				fg =
					gdImageColorAllocate(gc->brush, gc->color->r, gc->color->g,
								     gc->color->b);
			}
			gdImageColorTransparent(gc->brush, bg);

			/*
		         * if we shrunk to a radius/box width of zero, then just use
		         * a single pixel to draw with.
		         */
			if (r == 0)
				gdImageFilledRectangle(gc->brush, 0, 0, 0, 0, fg);
			else {
				if (type == 'C')
					gdImageFilledEllipse(gc->brush, r, r, 2 * r, 2 * r, fg);
				else
					gdImageFilledRectangle(gc->brush, 0, 0, r, r, fg);
			}
			bval.ptr = gc->brush;
			hid_cache_color(1, name, &bval, &bcache);
		}

		gdImageSetBrush(nelma_im, gc->brush);
		lastbrush = gc->brush;

	}
}

static void
nelma_draw_rect(hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
	use_gc(gc);
	gdImageRectangle(nelma_im,
			 pcb_to_nelma(x1), pcb_to_nelma(y1),
			 pcb_to_nelma(x2), pcb_to_nelma(y2), gc->color->c);
}

static void
nelma_fill_rect(hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
	use_gc(gc);
	gdImageSetThickness(nelma_im, 0);
	linewidth = 0;
	gdImageFilledRectangle(nelma_im, pcb_to_nelma(x1), pcb_to_nelma(y1),
			  pcb_to_nelma(x2), pcb_to_nelma(y2), gc->color->c);
}


struct nelma_netlist* nelma_lookup_net_from_arc(ArcType* targetArc)
{
	int i;
	for(i=0; i<PCB->NetlistLib.MenuN; i++)
	{ // for each net in the netlist
		struct nelma_netlist* currNet = &nelma_netlist[i];
		int j;
		for(j=0; j<PCB->LayerGroups.Number[nelma_cur_group]; j++)
		{// for each layer of the current group
			int layer = PCB->LayerGroups.Entries[nelma_cur_group][j];
			ARC_LOOP(&(currNet->layer[layer]));
			{
				if(targetArc == arc)
					return(currNet);
			}
			END_LOOP;
		}
	}
	return(NULL);
}


struct nelma_netlist* nelma_lookup_net_from_line(LineType* targetLine)
{
	int i;
	for(i=0; i<PCB->NetlistLib.MenuN; i++)
	{ // for each net in the netlist
		struct nelma_netlist* currNet = &nelma_netlist[i];
		
		int j;
		for(j=0; j<PCB->LayerGroups.Number[nelma_cur_group]; j++)
		{// for each layer of the current group
			int layer = PCB->LayerGroups.Entries[nelma_cur_group][j];
			LINE_LOOP(&(currNet->layer[layer]));
			{
				if(targetLine == line)
					return(currNet);
			}
			END_LOOP;
		}
	}
	return(NULL);
}


struct nelma_netlist* nelma_lookup_net_from_polygon( PolygonType* targetPolygon)
{
	int i;
	for(i=0; i<PCB->NetlistLib.MenuN; i++)
	{ // for each net in the netlist
		struct nelma_netlist* currNet = &nelma_netlist[i];
		int j;
		for(j=0; j<PCB->LayerGroups.Number[nelma_cur_group]; j++)
		{// for each layer of the current group
			int layer = PCB->LayerGroups.Entries[nelma_cur_group][j];
			POLYGON_LOOP(&(currNet->layer[layer]));
			{
				if(targetPolygon == polygon)
					return(currNet);
			}
			END_LOOP;
		}
	}
	return(NULL);
}


struct nelma_netlist* nelma_lookup_net_from_pad( PadType* targetPad)
{
	int i;
	for(i=0; i<PCB->NetlistLib.MenuN; i++)
	{ // for each net in the netlist
		struct nelma_netlist* currNet = &nelma_netlist[i];
		PAD_LOOP(currNet);
		{
			if(targetPad == pad)
				return(currNet);
		}
		END_LOOP;
	}
	return(NULL);
}


struct nelma_netlist* nelma_lookup_net_from_pv( PinType* targetPv)
{
	int i;
	for(i=0; i<PCB->NetlistLib.MenuN; i++)
	{ // for each net in the netlist
		struct nelma_netlist* currNet = &nelma_netlist[i];
		PIN_LOOP(currNet);
		{
			if(targetPv == pin)
				return(currNet);
		}
		END_LOOP;
		VIA_LOOP(currNet);
		{
			if(targetPv == via)
				return(currNet);
		}
		END_LOOP;
	}
	return(NULL);
}


static void
nelma_draw_line(hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
	if (x1 == x2 && y1 == y2) {
		Coord             w = gc->width / 2;
		nelma_fill_rect(gc, x1 - w, y1 - w, x1 + w, y1 + w);
		return;
	}
	linewidth=-1;
	use_gc(gc);
	linewidth=-1;

	gdImageSetThickness(nelma_im, 0);
	linewidth = 0;
	gdImageLine(nelma_im, pcb_to_nelma(x1), pcb_to_nelma(y1),
		    pcb_to_nelma(x2), pcb_to_nelma(y2), gdBrushed);
}

static void
nelma_draw_arc(hidGC gc, Coord cx, Coord cy, Coord width, Coord height,
	       Angle start_angle, Angle delta_angle)
{
	Angle sa, ea;

	/*
	 * in gdImageArc, 0 degrees is to the right and +90 degrees is down
	 * in pcb, 0 degrees is to the left and +90 degrees is down
	 */
	start_angle = 180 - start_angle;
	delta_angle = -delta_angle;
	if (delta_angle > 0) {
		sa = start_angle;
		ea = start_angle + delta_angle;
	} else {
		sa = start_angle + delta_angle;
		ea = start_angle;
	}

	/*
	 * make sure we start between 0 and 360 otherwise gd does strange
	 * things
	 */
        sa = NormalizeAngle (sa);
        ea = NormalizeAngle (ea);

#if 0
	printf("draw_arc %d,%d %dx%d %d..%d %d..%d\n",
	       cx, cy, width, height, start_angle, delta_angle, sa, ea);
	printf("gdImageArc (%p, %d, %d, %d, %d, %d, %d, %d)\n",
	       im, SCALE_X(cx), SCALE_Y(cy),
	       SCALE(width), SCALE(height), sa, ea, gc->color->c);
#endif
	use_gc(gc);
	gdImageSetThickness(nelma_im, 0);
	linewidth = 0;
	gdImageArc(nelma_im, pcb_to_nelma(cx), pcb_to_nelma(cy),
		   pcb_to_nelma(2 * width), pcb_to_nelma(2 * height), sa, ea, gdBrushed);
}

static void
nelma_fill_circle(hidGC gc, Coord cx, Coord cy, Coord radius)
{
	use_gc(gc);

	gdImageSetThickness(nelma_im, 0);
	linewidth = 0;
	gdImageFilledEllipse(nelma_im, pcb_to_nelma(cx), pcb_to_nelma(cy),
	pcb_to_nelma(2 * radius), pcb_to_nelma(2 * radius),  color_array[hashColor]->c);

}

static void
nelma_fill_polygon(hidGC gc, int n_coords, Coord *x, Coord *y)
{
	int             i;
	gdPoint        *points;

	points = (gdPoint *) malloc(n_coords * sizeof(gdPoint));
	if (points == NULL) {
		fprintf(stderr, "ERROR:  nelma_fill_polygon():  malloc failed\n");
		exit(1);
	}
	use_gc(gc);
	for (i = 0; i < n_coords; i++) {
		points[i].x = pcb_to_nelma(x[i]);
		points[i].y = pcb_to_nelma(y[i]);
	}
	gdImageSetThickness(nelma_im, 0);
	linewidth = 0;
	gdImageFilledPolygon(nelma_im, points, n_coords, color_array[hashColor]->c);
	free(points);
}

static void
nelma_calibrate(double xval, double yval)
{
	CRASH;
}

static void
nelma_set_crosshair(int x, int y, int a)
{
}


static void
nelma_draw_pcb_arc (hidGC gc, ArcType *arc)
{
	struct nelma_netlist* net;
	net = nelma_lookup_net_from_arc(arc);
	if(net)
	{
		hashColor = net->colorIndex;
	}
	else
	{
		hashColor = PCB->NetlistLib.MenuN;
        }
	common_draw_pcb_arc(gc, arc);
	hashColor = PCB->NetlistLib.MenuN;
}


static void nelma_draw_pcb_line(hidGC gc, LineType *line)
{
	struct nelma_netlist* net;
	net = nelma_lookup_net_from_line(line);
	if(net)
	{
		hashColor = net->colorIndex;
	}
	else
	{
		hashColor = PCB->NetlistLib.MenuN;
        }
	
	common_draw_pcb_line(gc, line);
	hashColor = PCB->NetlistLib.MenuN;

}


void nelma_fill_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box)
{  // highjack the fill_pcb_polygon function to get *poly,  then proceed with the default handler
	struct nelma_netlist* net;
        
	net = nelma_lookup_net_from_polygon(poly);
	if(net)
	{
		hashColor = net->colorIndex;
	}
	else
	{
		hashColor = PCB->NetlistLib.MenuN;
        }
                                                                                      
	common_fill_pcb_polygon (gc, poly, clip_box);
	hashColor = PCB->NetlistLib.MenuN;
}

void
nelma_fill_pcb_pad(hidGC gc, PadType *pad, bool clear, bool mask)
{
	struct nelma_netlist* net = NULL;
        
	net = nelma_lookup_net_from_pad(pad);
	if(net)
	{
		hashColor = net->colorIndex;
	}
	else
	{
		hashColor = PCB->NetlistLib.MenuN;
        }
                                                                                      
	common_fill_pcb_pad ( gc, pad, clear, mask);
	hashColor = PCB->NetlistLib.MenuN;
}


void
nelma_fill_pcb_pv (hidGC fg_gc, hidGC bg_gc, PinType *pv, bool drawHole, bool mask)
{
	struct nelma_netlist* net = NULL;
        
	net = nelma_lookup_net_from_pv(pv);
	if(net)
	{
		hashColor = net->colorIndex;
	}
	else
	{
		hashColor = PCB->NetlistLib.MenuN;
        }
	common_fill_pcb_pv(fg_gc, bg_gc, pv, drawHole, mask);
	hashColor = PCB->NetlistLib.MenuN;
}


/* *** Miscellaneous ******************************************************* */

#include "dolists.h"

void
hid_nelma_init()
{
  memset (&nelma_hid, 0, sizeof (HID));
  memset (&nelma_graphics, 0, sizeof (HID_DRAW));
  // set the default functions
  common_nogui_init (&nelma_hid);
  common_draw_helpers_init (&nelma_graphics);	

hid_gtk_init();
  nelma_hid.struct_size         = sizeof (HID);
  nelma_hid.name                = "nelma";
  nelma_hid.description         = "Numerical analysis package export";
  nelma_hid.exporter            = 1;
  nelma_hid.poly_before         = 1;

  nelma_hid.get_export_options  = nelma_get_export_options;
  nelma_hid.do_export           = nelma_do_export;
  nelma_hid.parse_arguments     = nelma_parse_arguments;
  nelma_hid.set_layer           = nelma_set_layer;
  nelma_hid.calibrate           = nelma_calibrate;
  nelma_hid.set_crosshair       = nelma_set_crosshair;

  nelma_hid.graphics            = &nelma_graphics;

  nelma_graphics.make_gc        = nelma_make_gc;
  nelma_graphics.destroy_gc     = nelma_destroy_gc;
  nelma_graphics.use_mask       = nelma_use_mask;
  nelma_graphics.set_color      = nelma_set_color;
  nelma_graphics.set_line_cap   = nelma_set_line_cap;
  nelma_graphics.set_line_width = nelma_set_line_width;
  nelma_graphics.set_draw_xor   = nelma_set_draw_xor;
  nelma_graphics.set_draw_faded = nelma_set_draw_faded;
  nelma_graphics.draw_line      = nelma_draw_line;
  nelma_graphics.draw_arc       = nelma_draw_arc;
  nelma_graphics.draw_rect      = nelma_draw_rect;
  nelma_graphics.fill_circle    = nelma_fill_circle;
  nelma_graphics.fill_polygon   = nelma_fill_polygon;
  nelma_graphics.fill_rect      = nelma_fill_rect;

  // highjack these functions because what is passed to fill_polygon is a series of polygons (for holes,...)
  nelma_graphics.draw_pcb_line = nelma_draw_pcb_line;
  nelma_graphics.draw_pcb_arc = nelma_draw_pcb_arc;
  nelma_graphics.draw_pcb_polygon = nelma_fill_pcb_polygon;

  nelma_graphics.fill_pcb_polygon = nelma_fill_pcb_polygon;
  nelma_graphics.fill_pcb_pad = nelma_fill_pcb_pad;
  nelma_graphics.fill_pcb_pv = nelma_fill_pcb_pv;


  hid_register_hid (&nelma_hid);

#include "nelma_lists.h"
}
