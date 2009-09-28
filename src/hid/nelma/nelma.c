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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
#include "data.h"
#include "misc.h"
#include "rats.h"

#include "hid.h"
#include "../hidint.h"
#include "nelma.h"

#include <gd.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID("$Id$");

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented PNG function %s.\n", __FUNCTION__); abort()

/* Needed for PNG export */

struct color_struct {
	/* the descriptor used by the gd library */
	int             c;

	/* so I can figure out what rgb value c refers to */
	unsigned int    r, g, b;
};

struct hid_gc_struct {
	HID            *me_pointer;
	EndCapStyle     cap;
	int             width;
	unsigned char   r, g, b;
	int             erase;
	int             faded;
	struct color_struct *color;
	gdImagePtr      brush;
};

static struct color_struct *black = NULL, *white = NULL;
static int      linewidth = -1;
static int      lastgroup = -1;
static gdImagePtr lastbrush = (void *) -1;
static int      lastcap = -1;
static int      lastcolor = -1;

/* gd image and file for PNG export */
static gdImagePtr nelma_im = NULL;
static FILE    *nelma_f = NULL;

static int      is_mask;
static int      is_drill;

/*
 * Which groups of layers to export into PNG layer masks. 1 means export, 0
 * means do not export.
 */
static int      nelma_export_group[MAX_LAYER];

/* Group that is currently exported. */
static int      nelma_cur_group;

/* Filename prefix that will be used when saving files. */
static char    *nelma_basename = NULL;

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
	{"basename", "File name prefix",
	HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_basename 0

	{"dpi", "Horizontal scale factor (grid points/inch).",
	HID_Integer, 0, 1000, {100, 0, 0}, 0, 0},
#define HA_dpi 1

	{"copper-height", "Copper layer height (um).",
	HID_Integer, 0, 200, {100, 0, 0}, 0, 0},
#define HA_copperh 2

	{"substrate-height", "Substrate layer height (um).",
	HID_Integer, 0, 10000, {2000, 0, 0}, 0, 0},
#define HA_substrateh 3

	{"substrate-epsilon", "Substrate relative epsilon.",
	HID_Real, 0, 100, {0, 0, 4.0}, 0, 0},
#define HA_substratee 4
};

#define NUM_OPTIONS (sizeof(nelma_attribute_list)/sizeof(nelma_attribute_list[0]))

REGISTER_ATTRIBUTES(nelma_attribute_list)
	static HID_Attr_Val nelma_values[NUM_OPTIONS];

/* *** Utility funcions **************************************************** */

/* convert from default PCB units (1/100 mil) to nelma units */
	static int      pcb_to_nelma(int pcb)
{
	int             nelma;

	nelma = (pcb * nelma_dpi) / 100000;

	return nelma;
}

static char    *
nelma_get_png_name(const char *basename, const char *suffix)
{
	char           *buf;
	int             len;

	len = strlen(basename) + strlen(suffix) + 6;
	buf = malloc(sizeof(*buf) * len);

	sprintf(buf, "%s.%s.png", basename, suffix);

	return buf;
}

/* Retrieves coordinates (in default PCB units) of a pin or pad. */
/* Copied from netlist.c */
static int 
pin_name_to_xy(LibraryEntryType * pin, int *x, int *y)
{
	ConnectionType  conn;
	if (!SeekPad(pin, &conn, False))
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
	for (i = 0; i < MAX_LAYER; i++)
		if (nelma_export_group[i]) {
			idx = (i >= 0 && i < max_layer) ?
				PCB->LayerGroups.Entries[i][0] : i;
			ext = layer_type_to_file_name(idx);

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
	LibraryMenuTypePtr net;
	LibraryEntryTypePtr pin;

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

			for (i = 0; i < MAX_LAYER; i++)
				if (nelma_export_group[i]) {
					idx = (i >= 0 && i < max_layer) ?
						PCB->LayerGroups.Entries[i][0] : i;
					ext = layer_type_to_file_name(idx);

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

static void 
nelma_write_layer(FILE * out, int z, int h,
		  const char *name, int full,
		  char *mat)
{
	LibraryType     netlist;
	LibraryMenuTypePtr net;
	LibraryEntryTypePtr pin;

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
	for (i = 0; i < MAX_LAYER; i++)
		if (nelma_export_group[i]) {
			idx = (i >= 0 && i < max_layer) ?
				PCB->LayerGroups.Entries[i][0] : i;
			ext = layer_type_to_file_name(idx);

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
nelma_write_object(FILE * out, LibraryEntryTypePtr pin)
{
	int             i, idx;
	int             x = 0, y = 0;

	char           *f;
	const char     *ext;

	pin_name_to_xy(pin, &x, &y);

	x = pcb_to_nelma(x);
	y = pcb_to_nelma(y);

	for (i = 0; i < MAX_LAYER; i++)
		if (nelma_export_group[i]) {
			idx = (i >= 0 && i < max_layer) ?
				PCB->LayerGroups.Entries[i][0] : i;
			ext = layer_type_to_file_name(idx);

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
	LibraryMenuTypePtr net;
	LibraryEntryTypePtr pin;

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

extern void     hid_parse_command_line(int *argc, char ***argv);

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

	for (n = 0; n < max_layer; n++) {
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

	white = (struct color_struct *) malloc(sizeof(*white));
	white->r = white->g = white->b = 255;
	white->c = gdImageColorAllocate(nelma_im, white->r, white->g, white->b);

	black = (struct color_struct *) malloc(sizeof(*black));
	black->r = black->g = black->b = 0;
	black->c = gdImageColorAllocate(nelma_im, black->r, black->g, black->b);
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
#ifdef HAVE_GDIMAGEPNG
	gdImagePng(nelma_im, nelma_f);
#else
	Message("NELMA: PNG not supported by gd. Can't write layer mask.\n");
#endif
	gdImageDestroy(nelma_im);
	fclose(nelma_f);

	free(white);
	free(black);

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
	lastbrush = (void *) -1;
	lastcap = -1;
	lastgroup = -1;
	lastcolor = -1;
	lastgroup = -1;

	hid_expose_callback(&nelma_hid, &region, 0);
}

static void 
nelma_do_export(HID_Attr_Val * options)
{
	int             save_ons[MAX_LAYER + 2];
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

	nelma_choose_groups();

	for (i = 0; i < MAX_LAYER; i++) {
		if (nelma_export_group[i]) {

			nelma_cur_group = i;

			/* magic */
			idx = (i >= 0 && i < max_layer) ?
				PCB->LayerGroups.Entries[i][0] : i;

			nelma_start_png(nelma_basename,
					layer_type_to_file_name(idx));

			hid_save_and_show_layer_ons(save_ons);
			nelma_start_png_export();
			hid_restore_layer_ons(save_ons);

			nelma_finish_png();
		}
	}

	len = strlen(nelma_basename) + 4;
	buf = malloc(sizeof(*buf) * len);

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
}

/* *** PNG export (slightly modified code from PNG export HID) ************* */

static int 
nelma_set_layer(const char *name, int group)
{
	int             idx = (group >= 0 && group < max_layer) ?
	PCB->LayerGroups.Entries[group][0] : group;

	if (name == 0) {
		name = PCB->Data->Layer[idx].Name;
	}
	if (strcmp(name, "invisible") == 0) {
		return 0;
	}
	is_drill = (SL_TYPE (idx) == SL_PDRILL || SL_TYPE (idx) == SL_UDRILL || SL_TYPE (idx) == SL_SPDRILL);
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
nelma_use_mask(int use_it)
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
nelma_set_line_width(hidGC gc, int width)
{
	gc->width = width;
}

static void
nelma_set_draw_xor(hidGC gc, int xor)
{
	;
}

static void
nelma_set_draw_faded(hidGC gc, int faded)
{
	gc->faded = faded;
}

static void
nelma_set_line_cap_angle(hidGC gc, int x1, int y1, int x2, int y2)
{
	CRASH;
}

static void
use_gc(hidGC gc)
{
	int             need_brush = 0;

	if (gc->me_pointer != &nelma_hid) {
		fprintf(stderr, "Fatal: GC from another HID passed to nelma HID\n");
		abort();
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

		if (hid_cache_color(0, name, &bval, &bcache)) {
			gc->brush = bval.ptr;
		} else {
			int             bg, fg;
			if (type == 'C')
				gc->brush = gdImageCreate(2 * r + 1, 2 * r + 1);
			else
				gc->brush = gdImageCreate(r + 1, r + 1);
			bg = gdImageColorAllocate(gc->brush, 255, 255, 255);
			fg =
				gdImageColorAllocate(gc->brush, gc->color->r, gc->color->g,
						     gc->color->b);
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
#define CBLEND(gc) (((gc->r)<<24)|((gc->g)<<16)|((gc->b)<<8)|(gc->faded))
	if (lastcolor != CBLEND(gc)) {
		if (is_drill || is_mask) {
#ifdef FIXME
			fprintf(f, "%d gray\n", gc->erase ? 0 : 1);
#endif
			lastcolor = 0;
		} else {
			double          r, g, b;
			r = gc->r;
			g = gc->g;
			b = gc->b;
			if (gc->faded) {
				r = 0.8 * 255 + 0.2 * r;
				g = 0.8 * 255 + 0.2 * g;
				b = 0.8 * 255 + 0.2 * b;
			}
#ifdef FIXME
			if (gc->r == gc->g && gc->g == gc->b)
				fprintf(f, "%g gray\n", r / 255.0);
			else
				fprintf(f, "%g %g %g rgb\n", r / 255.0, g / 255.0, b / 255.0);
#endif
			lastcolor = CBLEND(gc);
		}
	}
}

static void
nelma_draw_rect(hidGC gc, int x1, int y1, int x2, int y2)
{
	use_gc(gc);
	gdImageRectangle(nelma_im,
			 pcb_to_nelma(x1), pcb_to_nelma(y1),
			 pcb_to_nelma(x2), pcb_to_nelma(y2), gc->color->c);
}

static void
nelma_fill_rect(hidGC gc, int x1, int y1, int x2, int y2)
{
	use_gc(gc);
	gdImageSetThickness(nelma_im, 0);
	linewidth = 0;
	gdImageFilledRectangle(nelma_im, pcb_to_nelma(x1), pcb_to_nelma(y1),
			  pcb_to_nelma(x2), pcb_to_nelma(y2), gc->color->c);
}

static void
nelma_draw_line(hidGC gc, int x1, int y1, int x2, int y2)
{
	if (x1 == x2 && y1 == y2) {
		int             w = gc->width / 2;
		nelma_fill_rect(gc, x1 - w, y1 - w, x1 + w, y1 + w);
		return;
	}
	use_gc(gc);

	gdImageSetThickness(nelma_im, 0);
	linewidth = 0;
	gdImageLine(nelma_im, pcb_to_nelma(x1), pcb_to_nelma(y1),
		    pcb_to_nelma(x2), pcb_to_nelma(y2), gdBrushed);
}

static void
nelma_draw_arc(hidGC gc, int cx, int cy, int width, int height,
	       int start_angle, int delta_angle)
{
	int             sa, ea;

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
	while (sa < 0) {
		sa += 360;
		ea += 360;
	}
	while (sa >= 360) {
		sa -= 360;
		ea -= 360;
	}

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
nelma_fill_circle(hidGC gc, int cx, int cy, int radius)
{
	use_gc(gc);

	gdImageSetThickness(nelma_im, 0);
	linewidth = 0;
	gdImageFilledEllipse(nelma_im, pcb_to_nelma(cx), pcb_to_nelma(cy),
	  pcb_to_nelma(2 * radius), pcb_to_nelma(2 * radius), gc->color->c);

}

static void
nelma_fill_polygon(hidGC gc, int n_coords, int *x, int *y)
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
	gdImageFilledPolygon(nelma_im, points, n_coords, gc->color->c);
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

/* *** Miscellaneous ******************************************************* */

HID             nelma_hid = {
	sizeof(HID),
	"nelma",
	"Numerical analysis package export.",
	0,			/* gui */
	0,			/* printer */
	1,			/* exporter */
	1,			/* poly before */
	0,			/* poly after */
	0,			/* poly dicer */
	nelma_get_export_options,
	nelma_do_export,
	nelma_parse_arguments,
	0 /* nelma_invalidate_wh */ ,
	0 /* nelma_invalidate_lr */ ,
	0 /* nelma_invalidate_all */ ,
	nelma_set_layer,
	nelma_make_gc,
	nelma_destroy_gc,
	nelma_use_mask,
	nelma_set_color,
	nelma_set_line_cap,
	nelma_set_line_width,
	nelma_set_draw_xor,
	nelma_set_draw_faded,
	nelma_set_line_cap_angle,
	nelma_draw_line,
	nelma_draw_arc,
	nelma_draw_rect,
	nelma_fill_circle,
	nelma_fill_polygon,
	nelma_fill_rect,
	nelma_calibrate,
	0 /* nelma_shift_is_pressed */ ,
	0 /* nelma_control_is_pressed */ ,
	0 /* nelma_get_coords */ ,
	nelma_set_crosshair,
	0 /* nelma_add_timer */ ,
	0 /* nelma_stop_timer */ ,
	0 /* nelma_log */ ,
	0 /* nelma_logv */ ,
	0 /* nelma_confirm_dialog */ ,
	0 /* nelma_close_confirm_dialog */ ,
	0 /* nelma_report_dialog */ ,
	0 /* nelma_prompt_for */ ,
	0 /* nelma_fileselect */ ,
	0 /* nelma_attribute_dialog */ ,
	0 /* nelma_show_item */ ,
	0 /* nelma_beep */ ,
	0			/* nelma_progress */
};

#include "dolists.h"

void
hid_nelma_init()
{
	apply_default_hid(&nelma_hid, 0);
	hid_register_hid(&nelma_hid);

#include "nelma_lists.h"
}
