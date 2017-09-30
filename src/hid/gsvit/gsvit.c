/*!
 * \file src/hid/gsvit/gsvit.c
 *
 * \brief .
 *
 * This HID exports a PCB layout into:
 * - One layer mask file (PNG format) per copper layer.
 * - a gsvit configuration file that contains netlist and pin
 *   information.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994, 1995, 1996, 2004 Thomas Nau
 *
 * NELMA (Numerical capacitance calculator) export HID
 * Copyright (C) 2006 Tomaz Solc (tomaz.solc@tablix.org)
 *
 * PNG export code is based on the PNG export HID
 * Copyright (C) 2006 Dan McMahill
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 *
 * <hr>
 *
 * \todo If you have a section of a net that does not contain any pins
 * then that section will be missing from the gsvit's copper geometry.\n
 *
 * \example This section will be ignored by gsvit |       |
 *
 * ||            ||=======||            ||     component layer ||
 * ||       ||            || ||=============||       ||============||
 * solder layer
 *
 * pin1           via      via           pin2
 *
 * Single layer layouts are always exported correctly.
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
  int c;
  /* so I can figure out what rgb value c refers to */
  unsigned int r, g, b;
};

static void gsvit_write_xnets(void);

struct hid_gc_struct {
  HID *me_pointer;
  EndCapStyle cap;
  Coord width;
  unsigned char r, g, b;
  int erase;
  struct color_struct *color;
  gdImagePtr brush;
};

struct gsvit_net_layer {
  GList *Line;
  GList *Arc; 
  GList *Polygon;
};

struct gsvit_netlist {
  char* name;
  GList *Pin;
  GList *Via;
  GList *Pad;
  struct gsvit_net_layer layer[MAX_LAYER];
  struct color_struct color;
  int colorIndex;
};

struct gsvit_netlist* gsvit_netlist = NULL;

int hashColor = gdBrushed;

static struct color_struct* color_array[0x100];

static HID gsvit_hid;

static HID_DRAW gsvit_graphics;

static struct color_struct *black = NULL, *white = NULL;

static Coord linewidth = -1;

static gdImagePtr lastbrush = (gdImagePtr)((void *) -1);

/* gd image and file for PNG export */
static gdImagePtr gsvit_im = NULL;

static FILE *gsvit_f = NULL;

static int is_mask;

static int is_drill;

/*!
 * \brief Which groups of layers to export into PNG layer masks.
 * 
 * 1 means export; 0 means do not export.
 */
static int gsvit_export_group[MAX_GROUP];

/*!
 * \brief Group that is currently exported.
 */
static int gsvit_cur_group;

/*!
 * \brief Filename prefix that will be used when saving files.
 */
static const char *gsvit_basename = NULL;

/* Horizontal DPI (grid points per inch) */
static int      gsvit_dpi = -1;

/* Height of the copper layers in micrometers. */

/*
 * The height of the copper layer is currently taken as the vertical grid
 * step, since this is the smallest vertical feature in the layout.
 */
static int      gsvit_copperh = -1;
/* Height of the substrate layers in micrometers. */
static int      gsvit_substrateh = -1;
/* Relative permittivity of the substrate. */
static double   gsvit_substratee = -1;

/* Permittivity of empty space (As/Vm) */
static const double gsvit_air_epsilon = 8.85e-12;

HID_Attribute   gsvit_attribute_list[] = {
	/* other HIDs expect this to be first.  */

/* %start-doc options "96 gsvit Options"
@ftable @code
@item -- basename <string>
File name prefix.
@end ftable
%end-doc
*/
	{"basename", "File name prefix",
	HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_basename 0

/* %start-doc options "96 gsvit Options"
@ftable @code
@item --dpi <num>
Horizontal scale factor (grid points/inch).
@end ftable
%end-doc
*/
	{"dpi", "Horizontal scale factor (grid points/inch)",
	HID_Integer, 0, 1000, {1000, 0, 0}, 0, 0},	//1000 --> 1mil (25.4um) resolution 
#define HA_dpi 1

/* %start-doc options "96 gsvit Options"
@ftable @code
@item --copper-height <num>
Copper layer height (um).
@end ftable
%end-doc
*/
	{"copper-height", "Copper layer height (um)",
	HID_Integer, 0, 200, {35, 0, 0}, 0, 0},	// 1oz cu-->34.79um
#define HA_copperh 2

/* %start-doc options "96 gsvit Options"
@ftable @code
@item --substrate-height <num>
Substrate layer height (um).
@end ftable
%end-doc
*/
	{"substrate-height", "Substrate layer height (um)",
	HID_Integer, 0, 10000, {1588, 0, 0}, 0, 0},	// 62.5mil --> 1588um
#define HA_substrateh 3

/* %start-doc options "96 gsvit Options"
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

#define NUM_OPTIONS (sizeof(gsvit_attribute_list)/sizeof(gsvit_attribute_list[0]))

REGISTER_ATTRIBUTES(gsvit_attribute_list)
	static HID_Attr_Val gsvit_values[NUM_OPTIONS];

void gsvit_create_netlist(void);
void gsvit_destroy_netlist(void);

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

void gsvit_build_net_from_selected(struct gsvit_netlist* currNet)
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


void gsvit_create_netlist(void)
{
	int i;
	int numNets = PCB->NetlistLib.MenuN;
	gsvit_netlist = (struct gsvit_netlist*)malloc(sizeof(struct gsvit_netlist)*numNets);
	memset(gsvit_netlist,0,sizeof(struct gsvit_netlist)*numNets);

	for(i=0; i<numNets; i++)
	{ // for each net in the netlist
		int j;
		LibraryEntryType* entry;
		ConnectionType conn;

		struct gsvit_netlist* currNet = &gsvit_netlist[i];
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
		gsvit_build_net_from_selected(currNet);
		
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

		for(j=0; gsvit_netlist[i].name[j]; j++)
		{
			name[j] = tolower(gsvit_netlist[i].name[j]);
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
		gsvit_netlist[i].color.r = rgb[0];
		gsvit_netlist[i].color.g = rgb[1];
		gsvit_netlist[i].color.b = rgb[2];
	}
}


void gsvit_destroy_netlist(void)
{
	int i;
	for(i=0; i<PCB->NetlistLib.MenuN; i++)
	{ // for each net in the netlist
		struct gsvit_netlist* currNet = &gsvit_netlist[i];
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
	free(gsvit_netlist);
}



/* convert from default PCB units to gsvit units */
static int pcb_to_gsvit (Coord pcb)
{
  return COORD_TO_INCH(pcb) * gsvit_dpi;
}

static char    *
gsvit_get_png_name(const char *basename, const char *suffix)
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

/* *** Exporting netlist data and geometry to the gsvit config file ******** */

static void 
gsvit_write_space(FILE * out)
{
	double          xh, zh;

	int             z;
	int             i, idx;
	const char     *ext;

	xh = 2.54e-2 / ((double) gsvit_dpi);
	zh = gsvit_copperh * 1e-6;

	fprintf(out, "\n/* **** Space **** */\n\n");

	fprintf(out, "space pcb {\n");
	fprintf(out, "\tstep = { %e, %e, %e }\n", xh, xh, zh);
	fprintf(out, "\tlayers = {\n");

	fprintf(out, "\t\t\"air-top\",\n");
	fprintf(out, "\t\t\"air-bottom\"");

	z = 10;
	for (i = 0; i < MAX_GROUP; i++)
		if (gsvit_export_group[i]) {
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

static void gsvit_write_xspace(void)
{
	double          xh;
	uint32_t h;
	uint32_t w;
	
	char buff[0x100];

	int             i, idx;
	const char     *ext;
	char* src;

	xh = (1000.0* 2.54e-2) / ((double) gsvit_dpi);  // units are in mm
	h = (uint32_t)pcb_to_gsvit(PCB->MaxHeight);	// units are in counts
	w = (uint32_t)pcb_to_gsvit(PCB->MaxWidth);
	               
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
		if (gsvit_export_group[i]) {
			idx = (i >= 0 && i < max_group) ? PCB->LayerGroups.Entries[i][0] : i;
			ext = layer_type_to_file_name(idx, FNS_fixed);
			src = gsvit_get_png_name(gsvit_basename,ext);
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
gsvit_write_material(FILE * out, char *name, char *type, double e)
{
	fprintf(out, "material %s {\n", name);
	fprintf(out, "\ttype = \"%s\"\n", type);
	fprintf(out, "\tpermittivity = %e\n", e);
	fprintf(out, "\tconductivity = 0.0\n");
	fprintf(out, "\tpermeability = 0.0\n");
	fprintf(out, "}\n");
}

static void 
gsvit_write_materials(FILE * out)
{
	fprintf(out, "\n/* **** Materials **** */\n\n");

	gsvit_write_material(out, "copper", "metal", gsvit_air_epsilon);
	gsvit_write_material(out, "air", "dielectric", gsvit_air_epsilon);
	gsvit_write_material(out, "composite", "dielectric",
			     gsvit_air_epsilon * gsvit_substratee);
}

static void 
gsvit_write_nets(FILE * out)
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
				if (gsvit_export_group[i]) {
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

static void gsvit_write_xnets(void)
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
				if (gsvit_export_group[i])
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
gsvit_write_layer(FILE * out, int z, int h,
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
gsvit_write_layers(FILE * out)
{
	int             i, idx;
	int             z;

	const char     *ext;
	char            buf[100];

	int             subh;

	subh = gsvit_substrateh / gsvit_copperh;

	fprintf(out, "\n/* **** Layers **** */\n\n");

	/* Air layers on top and bottom of the stack */
	/* Their height is double substrate height. */
	gsvit_write_layer(out, 1, 2 * subh, "air-top", 0, "air");
	gsvit_write_layer(out, 1000, 2 * subh, "air-bottom", 0, "air");

	z = 10;
	for (i = 0; i < MAX_GROUP; i++)
		if (gsvit_export_group[i]) {
			idx = (i >= 0 && i < max_group) ?
				PCB->LayerGroups.Entries[i][0] : i;
			ext = layer_type_to_file_name(idx, FNS_fixed);

			if (z != 10) {
				sprintf(buf, "substrate-%d", z);
				gsvit_write_layer(out,
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
			gsvit_write_layer(out, z, 1, ext, 1, "air");

			z++;
		}
}

static void 
gsvit_write_object(FILE * out, LibraryEntryType *pin)
{
	int             i, idx;
	Coord           px = 0, py = 0;
	int             x, y;

	char           *f;
	const char     *ext;

	pin_name_to_xy (pin, &px, &py);

	x = pcb_to_gsvit (px);
	y = pcb_to_gsvit (py);

	for (i = 0; i < MAX_GROUP; i++)
		if (gsvit_export_group[i]) {
			idx = (i >= 0 && i < max_group) ?
				PCB->LayerGroups.Entries[i][0] : i;
			ext = layer_type_to_file_name(idx, FNS_fixed);

			fprintf(out, "object %s-%s {\n", pin->ListEntry, ext);
			fprintf(out, "\tposition = { 0, 0 }\n");
			fprintf(out, "\tmaterial = \"copper\"\n");
			fprintf(out, "\ttype = \"image\"\n");
			fprintf(out, "\trole = \"net\"\n");

			f = gsvit_get_png_name(gsvit_basename, ext);

			fprintf(out, "\tfile = \"%s\"\n", f);

			free(f);

			fprintf(out, "\tfile-pos = { %d, %d }\n", x, y);
			fprintf(out, "}\n");
		}
}

static void 
gsvit_write_objects(FILE * out)
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

			gsvit_write_object(out, pin);
		}
	}
}

/* *** Main export callback ************************************************ */

static void 
gsvit_parse_arguments(int *argc, char ***argv)
{
	hid_register_attributes(gsvit_attribute_list,
				sizeof(gsvit_attribute_list) /
				sizeof(gsvit_attribute_list[0]));
	hid_parse_command_line(argc, argv);
}

static HID_Attribute *
gsvit_get_export_options(int *n)
{
	static char    *last_made_filename = 0;

	if (PCB) {
		derive_default_filename(PCB->Filename,
					&gsvit_attribute_list[HA_basename],
					".gsvit",
					&last_made_filename);
	}
	if (n) {
		*n = NUM_OPTIONS;
	}
	return gsvit_attribute_list;
}

/* Populates gsvit_export_group array */
void 
gsvit_choose_groups()
{
	int             n, m;
	LayerType      *layer;

	/* Set entire array to 0 (don't export any layer groups by default */
	memset(gsvit_export_group, 0, sizeof(gsvit_export_group));

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
				gsvit_export_group[m] = 1;
			}
		}
	}
}

static void 
gsvit_alloc_colors()
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
	white->c = gdImageColorAllocate(gsvit_im, white->r, white->g, white->b);

	black = (struct color_struct *) malloc(sizeof(*black));
	black->r = black->g = black->b = 0;
	black->c = gdImageColorAllocate(gsvit_im, black->r, black->g, black->b);
	

	for(i=0;i<numNets; i++)
	{
		gsvit_netlist[i].colorIndex = i;	
		color_array[i] =  malloc(sizeof(*white));
		color_array[i]->r = gsvit_netlist[i].color.r;
		color_array[i]->g = gsvit_netlist[i].color.g;
		color_array[i]->b = gsvit_netlist[i].color.b;
//		printf("%d %d <%02x:%02x:%02x>\n", i, phase, (uint8_t)(color_array[i]->r), (uint8_t)(color_array[i]->g), (uint8_t)(color_array[i]->b) );

		color_array[i]->c = gdImageColorAllocate(gsvit_im, color_array[i]->r,  color_array[i]->g,  color_array[i]->b);
	}
	
	color_array[i] =  malloc(sizeof(*white));
	rgb = hslToRgb(128, (20*256)/100, (20*256)/100);
		
	color_array[i]->r = rgb[0];
	color_array[i]->g = rgb[1];
	color_array[i]->b = rgb[2];
	color_array[i]->c = gdImageColorAllocate(gsvit_im, color_array[i]->r,  color_array[i]->g,  color_array[i]->b);

	printf("%d colors allocated\n", numNets);
}

static void 
gsvit_start_png(const char *basename, const char *suffix)
{
	int             h, w;
	char           *buf;

	buf = gsvit_get_png_name(basename, suffix);

	h = pcb_to_gsvit(PCB->MaxHeight);
	w = pcb_to_gsvit(PCB->MaxWidth);

	/* gsvit_im = gdImageCreate (w, h); */

	/* Nelma only works with true color images */
	gsvit_im = gdImageCreate(w, h);
	gsvit_f = fopen(buf, "wb");

	gsvit_alloc_colors();

	free(buf);
}

static void 
gsvit_finish_png()
{
	int  i;
#ifdef HAVE_GDIMAGEPNG
	gdImagePng(gsvit_im, gsvit_f);
#else
	Message("NELMA: PNG not supported by gd. Can't write layer mask.\n");
#endif
	gdImageDestroy(gsvit_im);
	fclose(gsvit_f);

	free(white);
	free(black);
	for(i=0;i<0x100;i++)
	{
		free(color_array[i]);
	}

	gsvit_im = NULL;
	gsvit_f = NULL;
}

void 
gsvit_start_png_export()
{
	BoxType         region;

	region.X1 = 0;
	region.Y1 = 0;
	region.X2 = PCB->MaxWidth;
	region.Y2 = PCB->MaxHeight;

	linewidth = -1;
	lastbrush = (gdImagePtr)((void *) -1);

	hid_expose_callback(&gsvit_hid, &region, 0);
}
void gsvit_xml_out(char* gsvit_basename);

static void 
gsvit_do_export(HID_Attr_Val * options)
{
	int             save_ons[MAX_ALL_LAYER];
	int             i, idx;
	FILE           *gsvit_config;
	char           *buf;
	int             len;

	time_t          t;

	if (!options) {
		gsvit_get_export_options(0);
		for (i = 0; i < NUM_OPTIONS; i++) {
			gsvit_values[i] = gsvit_attribute_list[i].default_val;
		}
		options = gsvit_values;
	}
	gsvit_basename = options[HA_basename].str_value;
	if (!gsvit_basename) {
		gsvit_basename = "pcb-out";
	}
	gsvit_dpi = options[HA_dpi].int_value;
	if (gsvit_dpi < 0) {
		fprintf(stderr, "ERROR:  dpi may not be < 0\n");
		return;
	}
	gsvit_copperh = options[HA_copperh].int_value;
	gsvit_substrateh = options[HA_substrateh].int_value;
	gsvit_substratee = options[HA_substratee].real_value;

	gsvit_create_netlist();
	gsvit_choose_groups();

	for (i = 0; i < MAX_GROUP; i++) {
		if (gsvit_export_group[i]) {

			gsvit_cur_group = i;

			/* magic */
			idx = (i >= 0 && i < max_group) ?
				PCB->LayerGroups.Entries[i][0] : i;

			gsvit_start_png(gsvit_basename,
					layer_type_to_file_name(idx, FNS_fixed));

			hid_save_and_show_layer_ons(save_ons);
			gsvit_start_png_export();
			hid_restore_layer_ons(save_ons);

			gsvit_finish_png();
		}
	}

	len = strlen(gsvit_basename) + 4;
	buf = (char *)malloc(sizeof(*buf) * len);

	sprintf(buf, "%s.em", gsvit_basename);
	gsvit_config = fopen(buf, "w");

	free(buf);

	fprintf(gsvit_config, "/* Made with PCB Nelma export HID */");
	t = time(NULL);
	fprintf(gsvit_config, "/* %s */", ctime(&t));

	gsvit_write_nets(gsvit_config);
	gsvit_write_objects(gsvit_config);
	gsvit_write_layers(gsvit_config);
	gsvit_write_materials(gsvit_config);
	gsvit_write_space(gsvit_config);

	fclose(gsvit_config);
	gsvit_xml_out((char*)gsvit_basename);
	gsvit_destroy_netlist();
}

void gsvit_xml_out(char* gsvit_basename)
{
	char           *buf;
	int             len;
	time_t          t;

	len = strlen(gsvit_basename) + 4;
	buf = (char *)malloc(sizeof(*buf) * len);

	sprintf(buf, "%s.xem", gsvit_basename);
	XOUT_INIT(buf);
	free(buf);

	XOUT_HEADER();
	XOUT_ELEMENT_START("gsvit");
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
	gsvit_write_xspace();
	gsvit_write_xnets();
//	gsvit_write_objects(gsvit_config);
//	gsvit_write_layers(gsvit_config);
//	gsvit_write_materials(gsvit_config);
//	gsvit_write_space(gsvit_config);

	XOUT_ELEMENT_END( "gsvit");		
	XOUT_NEWLINE();
	XOUT_CLOSE();
}

/* *** PNG export (slightly modified code from PNG export HID) ************* */

static int 
gsvit_set_layer(const char *name, int group, int empty)
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
	if (group == gsvit_cur_group) {
		return 1;
	}
	return 0;
}

static hidGC 
gsvit_make_gc(void)
{
	hidGC           rv = (hidGC) malloc(sizeof(struct hid_gc_struct));
	rv->me_pointer = &gsvit_hid;
	rv->cap = Trace_Cap;
	rv->width = 1;
	rv->color = (struct color_struct *) malloc(sizeof(*rv->color));
	rv->color->r = rv->color->g = rv->color->b = 0;
	rv->color->c = 0;
	return rv;
}

static void 
gsvit_destroy_gc(hidGC gc)
{
	free(gc);
}

static void 
gsvit_use_mask(enum mask_mode mode)
{
	/* does nothing */
}

static void 
gsvit_set_color(hidGC gc, const char *name)
{
	if (gsvit_im == NULL) {
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
gsvit_set_line_cap(hidGC gc, EndCapStyle style)
{
	gc->cap = style;
}

static void
gsvit_set_line_width(hidGC gc, Coord width)
{
	gc->width = width;
}

static void
gsvit_set_draw_xor(hidGC gc, int xor_)
{
	;
}

static void
gsvit_set_draw_faded(hidGC gc, int faded)
{
}

static void
use_gc(hidGC gc)
{
	int             need_brush = 0;

	if (gc->me_pointer != &gsvit_hid) {
		fprintf(stderr, "Fatal: GC from another HID passed to gsvit HID\n");
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
		gdImageSetThickness(gsvit_im, pcb_to_gsvit(gc->width));
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
			r = pcb_to_gsvit(gc->width / 2);
			break;
		default:
		case Square_Cap:
			r = pcb_to_gsvit(gc->width);
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

		gdImageSetBrush(gsvit_im, gc->brush);
		lastbrush = gc->brush;

	}
}

static void
gsvit_draw_rect(hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
	use_gc(gc);
	gdImageRectangle(gsvit_im,
			 pcb_to_gsvit(x1), pcb_to_gsvit(y1),
			 pcb_to_gsvit(x2), pcb_to_gsvit(y2), gc->color->c);
}

static void
gsvit_fill_rect(hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
	use_gc(gc);
	gdImageSetThickness(gsvit_im, 0);
	linewidth = 0;
	gdImageFilledRectangle(gsvit_im, pcb_to_gsvit(x1), pcb_to_gsvit(y1),
			  pcb_to_gsvit(x2), pcb_to_gsvit(y2), gc->color->c);
}


struct gsvit_netlist* gsvit_lookup_net_from_arc(ArcType* targetArc)
{
	int i;
	for(i=0; i<PCB->NetlistLib.MenuN; i++)
	{ // for each net in the netlist
		struct gsvit_netlist* currNet = &gsvit_netlist[i];
		int j;
		for(j=0; j<PCB->LayerGroups.Number[gsvit_cur_group]; j++)
		{// for each layer of the current group
			int layer = PCB->LayerGroups.Entries[gsvit_cur_group][j];
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


struct gsvit_netlist* gsvit_lookup_net_from_line(LineType* targetLine)
{
	int i;
	for(i=0; i<PCB->NetlistLib.MenuN; i++)
	{ // for each net in the netlist
		struct gsvit_netlist* currNet = &gsvit_netlist[i];
		
		int j;
		for(j=0; j<PCB->LayerGroups.Number[gsvit_cur_group]; j++)
		{// for each layer of the current group
			int layer = PCB->LayerGroups.Entries[gsvit_cur_group][j];
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


struct gsvit_netlist* gsvit_lookup_net_from_polygon( PolygonType* targetPolygon)
{
	int i;
	for(i=0; i<PCB->NetlistLib.MenuN; i++)
	{ // for each net in the netlist
		struct gsvit_netlist* currNet = &gsvit_netlist[i];
		int j;
		for(j=0; j<PCB->LayerGroups.Number[gsvit_cur_group]; j++)
		{// for each layer of the current group
			int layer = PCB->LayerGroups.Entries[gsvit_cur_group][j];
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


struct gsvit_netlist* gsvit_lookup_net_from_pad( PadType* targetPad)
{
	int i;
	for(i=0; i<PCB->NetlistLib.MenuN; i++)
	{ // for each net in the netlist
		struct gsvit_netlist* currNet = &gsvit_netlist[i];
		PAD_LOOP(currNet);
		{
			if(targetPad == pad)
				return(currNet);
		}
		END_LOOP;
	}
	return(NULL);
}


struct gsvit_netlist* gsvit_lookup_net_from_pv( PinType* targetPv)
{
	int i;
	for(i=0; i<PCB->NetlistLib.MenuN; i++)
	{ // for each net in the netlist
		struct gsvit_netlist* currNet = &gsvit_netlist[i];
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
gsvit_draw_line(hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
	if (x1 == x2 && y1 == y2) {
		Coord             w = gc->width / 2;
		gsvit_fill_rect(gc, x1 - w, y1 - w, x1 + w, y1 + w);
		return;
	}
	linewidth=-1;
	use_gc(gc);
	linewidth=-1;

	gdImageSetThickness(gsvit_im, 0);
	linewidth = 0;
	gdImageLine(gsvit_im, pcb_to_gsvit(x1), pcb_to_gsvit(y1),
		    pcb_to_gsvit(x2), pcb_to_gsvit(y2), gdBrushed);
}

static void
gsvit_draw_arc(hidGC gc, Coord cx, Coord cy, Coord width, Coord height,
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
	gdImageSetThickness(gsvit_im, 0);
	linewidth = 0;
	gdImageArc(gsvit_im, pcb_to_gsvit(cx), pcb_to_gsvit(cy),
		   pcb_to_gsvit(2 * width), pcb_to_gsvit(2 * height), sa, ea, gdBrushed);
}

static void
gsvit_fill_circle(hidGC gc, Coord cx, Coord cy, Coord radius)
{
	use_gc(gc);

	gdImageSetThickness(gsvit_im, 0);
	linewidth = 0;
	gdImageFilledEllipse(gsvit_im, pcb_to_gsvit(cx), pcb_to_gsvit(cy),
	pcb_to_gsvit(2 * radius), pcb_to_gsvit(2 * radius),  color_array[hashColor]->c);

}

static void
gsvit_fill_polygon(hidGC gc, int n_coords, Coord *x, Coord *y)
{
	int             i;
	gdPoint        *points;

	points = (gdPoint *) malloc(n_coords * sizeof(gdPoint));
	if (points == NULL) {
		fprintf(stderr, "ERROR:  gsvit_fill_polygon():  malloc failed\n");
		exit(1);
	}
	use_gc(gc);
	for (i = 0; i < n_coords; i++) {
		points[i].x = pcb_to_gsvit(x[i]);
		points[i].y = pcb_to_gsvit(y[i]);
	}
	gdImageSetThickness(gsvit_im, 0);
	linewidth = 0;
	gdImageFilledPolygon(gsvit_im, points, n_coords, color_array[hashColor]->c);
	free(points);
}

static void
gsvit_calibrate(double xval, double yval)
{
	CRASH;
}

static void
gsvit_set_crosshair(int x, int y, int a)
{
}


static void
gsvit_draw_pcb_arc (hidGC gc, ArcType *arc)
{
	struct gsvit_netlist* net;
	net = gsvit_lookup_net_from_arc(arc);
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


static void gsvit_draw_pcb_line(hidGC gc, LineType *line)
{
	struct gsvit_netlist* net;
	net = gsvit_lookup_net_from_line(line);
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


void gsvit_fill_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box)
{  // highjack the fill_pcb_polygon function to get *poly,  then proceed with the default handler
	struct gsvit_netlist* net;
        
	net = gsvit_lookup_net_from_polygon(poly);
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
gsvit_fill_pcb_pad(hidGC gc, PadType *pad, bool clear, bool mask)
{
	struct gsvit_netlist* net = NULL;
        
	net = gsvit_lookup_net_from_pad(pad);
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
gsvit_fill_pcb_pv (hidGC fg_gc, hidGC bg_gc, PinType *pv, bool drawHole, bool mask)
{
	struct gsvit_netlist* net = NULL;
        
	net = gsvit_lookup_net_from_pv(pv);
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
hid_gsvit_init()
{
  memset (&gsvit_hid, 0, sizeof (HID));
  memset (&gsvit_graphics, 0, sizeof (HID_DRAW));
  // set the default functions
  common_nogui_init (&gsvit_hid);
  common_draw_helpers_init (&gsvit_graphics);	

hid_gtk_init();
  gsvit_hid.struct_size         = sizeof (HID);
  gsvit_hid.name                = "gsvit";
  gsvit_hid.description         = "Numerical analysis package export";
  gsvit_hid.exporter            = 1;
  gsvit_hid.poly_before         = 1;

  gsvit_hid.get_export_options  = gsvit_get_export_options;
  gsvit_hid.do_export           = gsvit_do_export;
  gsvit_hid.parse_arguments     = gsvit_parse_arguments;
  gsvit_hid.set_layer           = gsvit_set_layer;
  gsvit_hid.calibrate           = gsvit_calibrate;
  gsvit_hid.set_crosshair       = gsvit_set_crosshair;

  gsvit_hid.graphics            = &gsvit_graphics;

  gsvit_graphics.make_gc        = gsvit_make_gc;
  gsvit_graphics.destroy_gc     = gsvit_destroy_gc;
  gsvit_graphics.use_mask       = gsvit_use_mask;
  gsvit_graphics.set_color      = gsvit_set_color;
  gsvit_graphics.set_line_cap   = gsvit_set_line_cap;
  gsvit_graphics.set_line_width = gsvit_set_line_width;
  gsvit_graphics.set_draw_xor   = gsvit_set_draw_xor;
  gsvit_graphics.set_draw_faded = gsvit_set_draw_faded;
  gsvit_graphics.draw_line      = gsvit_draw_line;
  gsvit_graphics.draw_arc       = gsvit_draw_arc;
  gsvit_graphics.draw_rect      = gsvit_draw_rect;
  gsvit_graphics.fill_circle    = gsvit_fill_circle;
  gsvit_graphics.fill_polygon   = gsvit_fill_polygon;
  gsvit_graphics.fill_rect      = gsvit_fill_rect;

  // highjack these functions because what is passed to fill_polygon is a series of polygons (for holes,...)
  gsvit_graphics.draw_pcb_line = gsvit_draw_pcb_line;
  gsvit_graphics.draw_pcb_arc = gsvit_draw_pcb_arc;
  gsvit_graphics.draw_pcb_polygon = gsvit_fill_pcb_polygon;

  gsvit_graphics.fill_pcb_polygon = gsvit_fill_pcb_polygon;
  gsvit_graphics.fill_pcb_pad = gsvit_fill_pcb_pad;
  gsvit_graphics.fill_pcb_pv = gsvit_fill_pcb_pv;


  hid_register_hid (&gsvit_hid);

#include "gsvit_lists.h"
}
