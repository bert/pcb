/*!
 * \file src/hid/gsvit/gsvit.c
 *
 * \brief HID exporter for gsvit.
 *
 * This HID exports a PCB layout into:
 * - One layer mask file (PNG format) per copper layer.
 * - a gsvit configuration file that contains netlist and pin
 *   information.
 *
 * \bug If you have a section of a net that does not contain any pins
 * then that section will be missing from the gsvit's copper geometry.\n
 *
 * \note Single layer layouts are always exported correctly.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994, 1995, 1996, 2004 Thomas Nau
 *
 * Based on the NELMA (Numerical capacitance calculator) export HID
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
#include "find.h"

#include "hid.h"
#include "hid_draw.h"
#include "../hidint.h"
#include "hid/common/hidnogui.h"
#include "hid/common/draw_helpers.h"

#include <gd.h>
#include "xmlout.h"

#include "hid/common/hidinit.h"
#include "pcb-printf.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented PNG function %s.\n", __FUNCTION__); abort()
#define MAXREFPINS 32 /* max length of following list */
static char *reference_pin_names[] = {"1", "2", "A1", "A2", "B1", "B2", 0};

/* Needed for PNG export */

struct color_struct {
  /* the descriptor used by the gd library */
  int c;
  /* so I can figure out what rgb value c refers to */
  unsigned int r, g, b;
};

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


/*!
 * \brief Structure to represent a single hole.
  */
struct drill_hole
{
  int cx;
  int cy;
  int is_plated;
}; 

/*!
* \brief Structure to represent all holes of a given size.
*/
struct single_size_drills
{
  double diameter_inches;
  Coord radius;
  int n_holes;
  int n_holes_allocated;   
  struct drill_hole* holes;
};
static struct single_size_drills* drills = NULL;

typedef struct _StringList
{
  char *str;
  struct _StringList *next;
} StringList;

typedef struct _BomList
{
  char *descr;
  char *value;
  int num;
  StringList *refdes;
  struct _BomList *next;
} BomList;

static int n_drills = 0; /*!< At the start we have no drills at all */

static int n_drills_allocated = 0;

static int save_drill = 0;

static int is_plated = 0;        

struct gsvit_netlist* gsvit_netlist = NULL;

int hashColor = gdBrushed;

static struct color_struct* color_array[0x100];

static HID gsvit_hid;

static HID_DRAW gsvit_graphics;

static struct color_struct *black = NULL, *white = NULL;

static Coord linewidth = -1;

static gdImagePtr lastbrush = (gdImagePtr)((void *) -1);

/*!
 * \brief gd image for PNG export.
 */
static gdImagePtr gsvit_im = NULL;

/*!
 * \brief file for PNG export.
 */
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

/*!
 * \brief Horizontal DPI (grid points per inch).
 */
static int gsvit_dpi = -1;

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
   HID_Integer, 0, 1000, {1000, 0, 0}, 0, 0}, /* 1000 --> 1 mil (25.4 um) resolution */
#define HA_dpi 1

};

#define NUM_OPTIONS (sizeof (gsvit_attribute_list) / sizeof (gsvit_attribute_list[0]))

REGISTER_ATTRIBUTES(gsvit_attribute_list)
	static HID_Attr_Val gsvit_values[NUM_OPTIONS];

void gsvit_create_netlist (void);
void gsvit_destroy_netlist (void);
static void gsvit_xml_out (char* gsvit_basename);
static void gsvit_build_net_from_selected (struct gsvit_netlist* currNet);
static void gsvit_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);
static void gsvit_write_xnets (void);


/*!
 * \brief Convert HSL to RGB.
 *
 * From http://www.rapidtables.com/convert/color/hsl-to-rgb.htm
 *
 * Converts from (H)ue (S)aturation and (L)ightness to (R)ed, (G)reen
 * and (B)lue.
 *
 * h range 0 - 365 degrees
 *
 * s range 0 - 255
 *
 * l range 0 - 255
 *
 * \return rgb array of chars in range of 0-255 0-->R, 1-->G, 2-->B.
 *
 * \note Always returns a result,  wraps 'h' if > 360 degrees.
 */
char *
hslToRgb (int h, uint8_t s, uint8_t l)
{
  static char rgb[3];
  float H = fmod ((float)(h), 360.0);
  float S = s;
  float L = l;

  int sect;
  float C, X;
  float m;
  float Rp, Gp, Bp;

  L /= 255.0;
  S /= 255.0;
  sect = h / 60;

  C = (1.0 - fabs (2 * L - 1.0)) * S;
  X = C * (1.0 - fabs (fmod ((H / 60.0), 2.0)  - 1.0));
  m = L - C / 2.0;

  switch (sect)
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

  rgb[0] = (Rp + m) * 0xFF;
  rgb[1] = (Gp + m) * 0xFF;
  rgb[2] = (Bp + m) * 0xFF;

  return (rgb);
}


void
gsvit_build_net_from_selected (struct gsvit_netlist* currNet)
{
  COPPERLINE_LOOP (PCB->Data);
  {
    if (TEST_FLAG (SELECTEDFLAG, line))
    {
      currNet->layer[l].Line = g_list_prepend(currNet->layer[l].Line, line);
    }
  }
  ENDALL_LOOP;

  COPPERARC_LOOP (PCB->Data);
  {
    if (TEST_FLAG (SELECTEDFLAG, arc))
    {
      currNet->layer[l].Arc = g_list_prepend (currNet->layer[l].Arc, arc);
    }
  }
  ENDALL_LOOP;

  COPPERPOLYGON_LOOP (PCB->Data);
  {
    if (TEST_FLAG (SELECTEDFLAG, polygon))
    {
      currNet->layer[l].Polygon = g_list_prepend (currNet->layer[l].Polygon, polygon);
    }
  }
  ENDALL_LOOP;

  ALLPAD_LOOP (PCB->Data);
  {
    if (TEST_FLAG (SELECTEDFLAG, pad))
    {
      currNet->Pad = g_list_prepend (currNet->Pad, pad);
    }
  }
  ENDALL_LOOP;

  ALLPIN_LOOP (PCB->Data);
  {
    if (TEST_FLAG (SELECTEDFLAG, pin))
    {
      currNet->Pin = g_list_prepend (currNet->Pin, pin);
    }
  }
  ENDALL_LOOP;

  VIA_LOOP (PCB->Data);
  {
    if (TEST_FLAG (SELECTEDFLAG, via))
    {
      currNet->Via = g_list_prepend(currNet->Via, via);
    }
  }
  END_LOOP;
}


void
gsvit_create_netlist (void)
{
  int i;
  int numNets = PCB->NetlistLib.MenuN;
  gsvit_netlist = (struct gsvit_netlist*) malloc (sizeof (struct gsvit_netlist) * numNets);
  memset (gsvit_netlist, 0, sizeof (struct gsvit_netlist) * numNets);

  for (i = 0; i < numNets; i++)
  { /* For each net in the netlist. */
    int j;
    LibraryEntryType* entry;
    ConnectionType conn;

    struct gsvit_netlist* currNet = &gsvit_netlist[i];
    currNet->name = PCB->NetlistLib.Menu[i].Name + 2;
    /*! \todo Add fancy color attachment here. */

    InitConnectionLookup ();
    ClearFlagOnAllObjects (false, FOUNDFLAG | SELECTEDFLAG);
    for (j = PCB->NetlistLib.Menu[i].EntryN, entry = PCB->NetlistLib.Menu[i].Entry; j; j--, entry++)
    { /* For each component (pin/pad) in the net. */
      if (SeekPad(entry, &conn, false))
      {
        RatFindHook(conn.type, conn.ptr1, conn.ptr2, conn.ptr2, false, SELECTEDFLAG, false);
      }
    }
    /* The conn should now contain a list of all elements that are part
     * of the net.
     * Now build a database of all things selected as part of this net.
     */
    gsvit_build_net_from_selected (currNet);

    ClearFlagOnAllObjects (false, FOUNDFLAG | SELECTEDFLAG);
    FreeConnectionLookupMemory ();
  }

  /* Assign colors to nets. */
  for (i = 0; i < numNets; i++)
  {
    char *rgb = NULL;
    char name[0x100];
    int j;
    int phase = (360 * i) / numNets;

    for (j = 0; gsvit_netlist[i].name[j]; j++)
    {
      name[j] = tolower (gsvit_netlist[i].name[j]);
    }

    name[j] = 0; /*! \todo Probably a '\0' character is better code. */

    if (strstr (name, "gnd") || strstr (name, "ground"))
    { /* Make gnd nets darker. */
      rgb = hslToRgb (phase, (70 * 256) / 100, (20 * 256) / 100);
    }
    else if (strstr (name, "unnamed"))
    {/* Make unnamed nets grayer. */
      rgb = hslToRgb (phase, (15 * 256) / 100, (40 * 256) / 100);
    }
    else
      rgb = hslToRgb (phase, (70 * 256) / 100, (50 * 256) / 100);

    gsvit_netlist[i].color.r = rgb[0];
    gsvit_netlist[i].color.g = rgb[1];
    gsvit_netlist[i].color.b = rgb[2];
  }
}


void
gsvit_destroy_netlist (void)
{
  int i;
  for (i = 0; i < PCB->NetlistLib.MenuN; i++)
  { /* For each net in the netlist. */
    struct gsvit_netlist* currNet = &gsvit_netlist[i];
    int j;
    for (j = 0; j < MAX_LAYER; j++)
    {
      g_list_free (currNet->layer[j].Line);
      g_list_free (currNet->layer[j].Arc);
      g_list_free (currNet->layer[j].Polygon);
    }
    g_list_free (currNet->Pad);
    g_list_free (currNet->Pin);
    g_list_free (currNet->Via);
  }
  free (gsvit_netlist);
}


/*!
 * \brief Convert from default PCB units to gsvit units.
 */
static int
pcb_to_gsvit (Coord pcb)
{
  return COORD_TO_INCH (pcb) * gsvit_dpi;
}


static char *
gsvit_get_png_name (const char *basename, const char *suffix)
{
  char *buf;
  int len;

  len = strlen (basename) + strlen(suffix) + 6;
  buf = (char *) malloc (sizeof (*buf) * len);

  sprintf (buf, "%s.%s.png", basename, suffix);

  return buf;
}


static void
gsvit_write_xspace (void)
{
  double xh;
  uint32_t h;
  uint32_t w;
  char buff[0x100];
  int i, idx;
  const char *ext;
  char *src;

  xh = (1000.0* 2.54e-2) / ((double) gsvit_dpi); /* Units are in mm. */
  h = (uint32_t) pcb_to_gsvit (PCB->MaxHeight); /* Units are in counts. */
  w = (uint32_t) pcb_to_gsvit (PCB->MaxWidth);

  sprintf (buff, "%f", xh);
  XOUT_ELEMENT_START ("space");
  XOUT_INDENT ();
  XOUT_NEWLINE ();
  XOUT_ELEMENT ("comment", "***** Space *****");
  XOUT_NEWLINE ();

  XOUT_ELEMENT_ATTR ("resolution", "units", "mm", buff);
  XOUT_NEWLINE ();

  sprintf (buff, "%d", w);
  XOUT_ELEMENT ("width", buff);
  XOUT_NEWLINE ();
  sprintf (buff, "%d", h);
  XOUT_ELEMENT ("height", buff);
  XOUT_NEWLINE ();
  XOUT_ELEMENT_START ("layers");
  XOUT_INDENT ();
  for (i = 0; i < MAX_GROUP; i++)
    if (gsvit_export_group[i]) {
      idx = (i >= 0 && i < max_group) ? PCB->LayerGroups.Entries[i][0] : i;
      ext = layer_type_to_file_name (idx, FNS_fixed);
      src = gsvit_get_png_name (gsvit_basename, ext);
      XOUT_NEWLINE ();
      XOUT_ELEMENT_ATTR ("layer", "name", ext, src);
    }
  XOUT_DETENT ();
  XOUT_NEWLINE ();
  XOUT_ELEMENT_END ("layers");
  XOUT_DETENT ();
  XOUT_NEWLINE ();
  XOUT_ELEMENT_END ("space");
  XOUT_DETENT ();
}


static void
gsvit_write_xnets (void)
{
  LibraryType netlist;
  LibraryMenuType *net;
  LibraryEntryType *pin;
  int n, m, i, idx;

  netlist = PCB->NetlistLib;

  XOUT_INDENT ();
  XOUT_NEWLINE ();
  XOUT_ELEMENT_START ("nets");
  XOUT_INDENT ();
  XOUT_NEWLINE ();
  XOUT_ELEMENT ("comment", "***** Nets *****");
  XOUT_NEWLINE ();

  for (n = 0; n < netlist.MenuN; n++) {
    char buff[0x100];
    net = &netlist.Menu[n];

    /* Weird, but correct */
    XOUT_ELEMENT_ATTR_START ("net", "name", &net->Name[2]);
//    snprintf(buff, 0x100, "%d,%d,%d", net->color.r, net->color.g, net->color.b);
//    XOUT_ELEMENT_2ATTR_START("net", "name", &net->Name[2], "color", buff );

    for (m = 0; m < net->EntryN; m++) {
      pin = &net->Entry[m];

      /* pin_name_to_xy(pin, &x, &y); */

      for (i = 0; i < MAX_GROUP; i++)
        if (gsvit_export_group[i]) {
          idx = (i >= 0 && i < max_group) ? PCB->LayerGroups.Entries[i][0] : i;
          layer_type_to_file_name (idx, FNS_fixed);

          if (m != 0 || i != 0)
            XOUT_ELEMENT_DATA (", ");
          snprintf (buff, 0x100, "%s", pin->ListEntry);
          {
            char* src = buff;
            while (*src)
            {
              if (*src == '-')
                *src = '.';
              src++;
            }
          }

          XOUT_ELEMENT_DATA (buff);
          break;
        }
    }

    XOUT_ELEMENT_END ("net");
    if (n+1 >= netlist.MenuN)
      XOUT_DETENT ();
    XOUT_NEWLINE ();
  }
  XOUT_ELEMENT_END ("nets");
  XOUT_DETENT ();
}


static StringList *
string_insert (char *str, StringList *list)
{
  StringList *newlist, *cur;

  if ((newlist = (StringList *) malloc (sizeof (StringList))) == NULL)
  {
    fprintf (stderr, "malloc() failed in string_insert()\n");
    exit (1);
  }

  newlist->next = NULL;
  newlist->str = strdup (str);

  if (list == NULL)
    return (newlist);

  cur = list;
  while (cur->next != NULL)
    cur = cur->next;

  cur->next = newlist;

  return (list);
}


static BomList *
bom_insert (char *refdes, char *descr, char *value, BomList * bom)
{
  BomList *newlist, *cur, *prev = NULL;

  if (bom == NULL)
  {
    /* this is the first element so automatically create an entry */
    if ((newlist = (BomList *) malloc (sizeof (BomList))) == NULL)
    {
      fprintf (stderr, "malloc() failed in bom_insert()\n");
      exit (1);
    }

    newlist->next = NULL;
    newlist->descr = strdup (descr);
    newlist->value = strdup (value);
    newlist->num = 1;
    newlist->refdes = string_insert (refdes, NULL);
    return (newlist);
  }

  /* search and see if we already have used one of these
  components */
  cur = bom;
  while (cur != NULL)
  {
    if ((NSTRCMP (descr, cur->descr) == 0) && (NSTRCMP (value, cur->value) == 0))
    {
      cur->num++;
      cur->refdes = string_insert (refdes, cur->refdes);
      break;
    }
    prev = cur;
    cur = cur->next;
  }

  if (cur == NULL)
  {
    if ((newlist = (BomList *) malloc (sizeof (BomList))) == NULL)
    {
      fprintf (stderr, "malloc() failed in bom_insert()\n");
      exit (1);
    }

    prev->next = newlist;

    newlist->next = NULL;
    newlist->descr = strdup (descr);
    newlist->value = strdup (value);
    newlist->num = 1;
    newlist->refdes = string_insert (refdes, NULL);
  }
  return (bom);
}


static double
xyToAngle (double x, double y, bool morethan2pins)
{
  double d = atan2 (-y, x) * 180.0 / M_PI;

  /* IPC 7351 defines different rules for 2 pin elements */
  if (morethan2pins)
  {
    /* Multi pin case:
    * Output 0 degrees if pin1 in is top left or top, i.e. between angles of
    * 80 to 170 degrees.
    * Pin #1 can be at dead top (e.g. certain PLCCs) or anywhere in the top
    * left.
    */
    if (d < -100)
      return 90; /* -180 to -100 */
    else if (d < -10)
      return 180; /* -100 to -10 */
    else if (d < 80)
      return 270; /* -10 to 80 */
    else if (d < 170)
      return 0; /* 80 to 170 */
    else
      return 90; /* 170 to 180 */
  }
  else
  {
    /* 2 pin element:
    * Output 0 degrees if pin #1 is in top left or left, i.e. in sector
    * between angles of 95 and 185 degrees.
    */
    if (d < -175)
      return 0; /* -180 to -175 */
    else if (d < -85)
      return 90; /* -175 to -85 */
    else if (d < 5)
      return 180; /* -85 to 5 */
    else if (d < 95)
      return 270; /* 5 to 95 */
    else
      return 0; /* 95 to 180 */
  }
}


/*!
 * \brief Main export callback.
 */
static void 
gsvit_parse_arguments (int *argc, char ***argv)
{
  hid_register_attributes (gsvit_attribute_list, sizeof (gsvit_attribute_list) /
    sizeof (gsvit_attribute_list[0]));

  hid_parse_command_line(argc, argv);
}


static HID_Attribute *
gsvit_get_export_options (int *n)
{
  static char *last_made_filename = 0;

  if (PCB) {
    derive_default_filename (PCB->Filename, &gsvit_attribute_list[HA_basename],
      ".gsvit", &last_made_filename);
  }

  if (n) {
    *n = NUM_OPTIONS;
  }

  return gsvit_attribute_list;
}


static char* CleanXBOMString (char *in)
{
  char *out;
//  int i;
  char* src;
  char* dest;

  if ((out = (char *)malloc ((strlen (in) + 1+0x40) * sizeof (char))) == NULL)
  {
    fprintf (stderr, "Error:  CleanBOMString() malloc() failed\n");
    exit (1);
  }
  dest = out;
  src = in;
  while(*src != '\0')
  {
    switch (*src)
    {
    case '<':
      *dest++ = '&';
      *dest++ = 'l';
      *dest++ = 't';
      *dest = ';';
    break;
    case '&':
      *dest++ = '&';
      *dest++ = 'a';
      *dest++ = 'm';
      *dest++ = 'p';
      *dest = ';';
    break;
    default:
      *dest = *src;
    }
    src++;
    dest++;
  }
  return out;
}


static void gsvit_write_xdrills (void)
{
  int i = 0;
  int j;
  char buff[0x100];

  XOUT_NEWLINE ();
  XOUT_ELEMENT_START ("drills");
  XOUT_INDENT ();
  XOUT_NEWLINE ();
  XOUT_ELEMENT ("comment", "***** Drills *****");
  for (i  = 0; i < n_drills; i++)
  {
    snprintf (buff, 0x100, "D%d", i);
    XOUT_NEWLINE ();
    XOUT_ELEMENT_ATTR_START ("drill", "id", buff);
    XOUT_INDENT ();
    snprintf (buff, 0x100, "%g", drills[i].diameter_inches);
    XOUT_NEWLINE ();
    XOUT_ELEMENT ("dia_inches", buff);
    snprintf (buff, 0x100, "%d", pcb_to_gsvit(drills[i].radius));
    XOUT_NEWLINE ();
    XOUT_ELEMENT ("radius", buff);
    for (j = 0; j < drills[i].n_holes; j++)
    {
      snprintf (buff, 0x100, "%d,%d", drills[i].holes[j].cx, drills[i].holes[j].cy);
      XOUT_NEWLINE ();
      if (drills[i].holes[j].is_plated)
      {
        XOUT_ELEMENT_ATTR_START ("pos", "type", "plated");
      }
      else
      {
        XOUT_ELEMENT_ATTR_START ("pos", "type", "unplated");
      }
      XOUT_ELEMENT_DATA (buff);
      XOUT_ELEMENT_END ("pos");
    }
    XOUT_DETENT ();
    XOUT_NEWLINE ();
    XOUT_ELEMENT_END ("drill");
  }
//      if (drills[i].diameter_inches >= diameter_inches)
//        break;
  XOUT_DETENT ();
  XOUT_NEWLINE ();
  XOUT_ELEMENT_END ("drills");
  XOUT_DETENT ();
  XOUT_NEWLINE ();
}


static void gsvit_write_xcentroids (void)
{
  char buff[0x100];

  Coord x, y;
  double theta = 0.0;
  double sumx, sumy;
  int pinfound[MAXREFPINS];
  double pinx[MAXREFPINS];
  double piny[MAXREFPINS];
  double pinangle[MAXREFPINS];
  double padcentrex, padcentrey;
  double centroidx, centroidy;
  double pin1x, pin1y;
  int pin_cnt;
  int found_any_not_at_centroid;
  int found_any;
  BomList *bom = NULL;
  char *name, *descr, *value,*fixed_rotation;
  int rpindex;

  XOUT_INDENT ();
  XOUT_NEWLINE ();

  XOUT_ELEMENT_START ("centroids");
  XOUT_INDENT ();
  XOUT_NEWLINE ();
  XOUT_ELEMENT ("comment", "***** Centroids *****");

  /*
  * For each element we calculate the centroid of the footprint.
  * In addition, we need to extract some notion of rotation.
  * While here generate the BOM list
  */
  ELEMENT_LOOP (PCB->Data);
  {

    /* Initialize our pin count and our totals for finding the centroid. */
    pin_cnt = 0;
    sumx = 0.0;
    sumy = 0.0;
    for (rpindex = 0; rpindex < MAXREFPINS; rpindex++)
      pinfound[rpindex] = 0;

    /* Insert this component into the bill of materials list. */
    bom = bom_insert ((char *)UNKNOWN (NAMEONPCB_NAME (element)),
                      (char *)UNKNOWN (DESCRIPTION_NAME (element)),
                      (char *)UNKNOWN (VALUE_NAME (element)), bom);


    /*
     * Iterate over the pins and pads keeping a running count of how
     * many pins/pads total and the sum of x and y coordinates
     *
     * While we're at it, store the location of pin/pad #1 and #2 if
     * we can find them.
     */

    PIN_LOOP (element);
    {
      sumx += (double) pin->X;
      sumy += (double) pin->Y;
      pin_cnt++;

      for (rpindex = 0; reference_pin_names[rpindex]; rpindex++) {
        if (NSTRCMP (pin->Number, reference_pin_names[rpindex]) == 0){
          pinx[rpindex] = (double) pin->X;
          piny[rpindex] = (double) pin->Y;
          pinangle[rpindex] = 0.0; /* pins have no notion of angle */
          pinfound[rpindex] = 1;
        }
      }
    }
    END_LOOP;

    PAD_LOOP (element);
    {
      sumx += (pad->Point1.X + pad->Point2.X) / 2.0;
      sumy += (pad->Point1.Y + pad->Point2.Y) / 2.0;
      pin_cnt++;

      for (rpindex = 0; reference_pin_names[rpindex]; rpindex++) {
        if (NSTRCMP (pad->Number, reference_pin_names[rpindex]) == 0) {
          padcentrex = (double) (pad->Point1.X + pad->Point2.X) / 2.0;
          padcentrey = (double) (pad->Point1.Y + pad->Point2.Y) / 2.0;
          pinx[rpindex] = padcentrex;
          piny[rpindex] = padcentrey;
          /*
           * NOTE: We swap the Y points because in PCB, the Y-axis
           * is inverted.  Increasing Y moves down.  We want to deal
           * in the usual increasing Y moves up coordinates though.
           */
          pinangle[rpindex] = (180.0 / M_PI) * atan2 (pad->Point1.Y - pad->Point2.Y,
          pad->Point2.X - pad->Point1.X);
          pinfound[rpindex]=1;
        }
      }
    }
    END_LOOP;

    if (pin_cnt > 0) {
      centroidx = sumx / (double) pin_cnt;
      centroidy = sumy / (double) pin_cnt;

      if (NSTRCMP (AttributeGetFromList (&element->Attributes,"xy-centre"), "origin") == 0 ) {
        x = element->MarkX;
        y = element->MarkY;
      }
      else {
        x = centroidx;
        y = centroidy;
      }

      fixed_rotation = AttributeGetFromList (&element->Attributes, "xy-fixed-rotation");
      if (fixed_rotation) {
        /* The user specified a fixed rotation */
        theta = atof (fixed_rotation);
        found_any_not_at_centroid = 1;
        found_any = 1;
      }
      else {
        /* Find first reference pin not at the  centroid  */
        found_any_not_at_centroid = 0;
        found_any = 0;
        theta = 0.0;
        for (rpindex = 0;
             reference_pin_names[rpindex] && !found_any_not_at_centroid;
             rpindex++) {
          if (pinfound[rpindex]) {
            found_any = 1;

            /* Recenter pin "#1" onto the axis which cross at the part
             * centroid. */
            pin1x = pinx[rpindex] - x;
            pin1y = piny[rpindex] - y;

            /* flip x, to reverse rotation for elements on back. */
            if (FRONT (element) != 1)
              pin1x = -pin1x;

            /* if only 1 pin, use pin 1's angle */
            if (pin_cnt == 1) {
              theta = pinangle[rpindex];
              found_any_not_at_centroid = 1;
            }
            else if ((pin1x != 0.0) || (pin1y != 0.0)) {
              theta = xyToAngle (pin1x, pin1y, pin_cnt > 2);
              found_any_not_at_centroid = 1;
            }
          }
        }

        if (!found_any) {
          Message
            ("PrintBOM(): unable to figure out angle because I could\n"
             "     not find a suitable reference pin of element %s\n"
             "     Setting to %g degrees\n",
             UNKNOWN (NAMEONPCB_NAME (element)), theta);
        }
        else if (!found_any_not_at_centroid) {
          Message
            ("PrintBOM(): unable to figure out angle of element\n"
             "     %s because the reference pin(s) are at the centroid of the part.\n"
             "     Setting to %g degrees\n",
             UNKNOWN (NAMEONPCB_NAME (element)), theta);
        }
      }
      name = CleanXBOMString ((char *)UNKNOWN (NAMEONPCB_NAME (element)));
      descr = CleanXBOMString ((char *)UNKNOWN (DESCRIPTION_NAME (element)));
      value = CleanXBOMString ((char *)UNKNOWN (VALUE_NAME (element)));

      y = PCB->MaxHeight - y;
      XOUT_NEWLINE ();
      XOUT_ELEMENT_ATTR_START ("xy", "name", name);
      XOUT_INDENT ();
      XOUT_NEWLINE ();
      XOUT_ELEMENT ("description", descr);
      XOUT_NEWLINE ();
      XOUT_ELEMENT ("value", value);
      XOUT_NEWLINE ();
      snprintf (buff, 0x100,  "%d,%d", pcb_to_gsvit(x), pcb_to_gsvit(y));
      XOUT_ELEMENT ("pos", buff);
      XOUT_NEWLINE ();
      pcb_snprintf (buff, 0x100, "%g", theta);
      XOUT_ELEMENT ("rotation", buff);
      XOUT_NEWLINE ();
      XOUT_ELEMENT ("side", FRONT (element) == 1 ? "top" : "bottom");
      XOUT_DETENT ();
      XOUT_NEWLINE ();
      XOUT_ELEMENT_END ("xy");

      free (name);
      free (descr);
      free (value);
    }
  }
  END_LOOP;

  XOUT_DETENT ();
  XOUT_NEWLINE ();
  XOUT_ELEMENT_END ("centroids");
}


/*!
 * \brief Populates gsvit_export_group array.
 */
void 
gsvit_choose_groups ()
{
  int n, m;
  LayerType *layer;

  /* Set entire array to 0 (don't export any layer groups by default */
  memset (gsvit_export_group, 0, sizeof (gsvit_export_group));

  for (n = 0; n < max_copper_layer; n++) {
    layer = &PCB->Data->Layer[n];

    if (layer->LineN || layer->TextN || layer->ArcN || layer->PolygonN) {
      /* Layer isn't empty. */
      /*! \todo Is this check necessary? It seems that special layers
       * have negative indexes?
       */

      if (SL_TYPE(n) == 0) {
        /* Layer is a copper layer. */
        m = GetLayerGroupNumberByNumber (n);

        /* The export layer. */
        gsvit_export_group[m] = 1;
      }
    }
  }
}


/*!
 * \brief Allocate colors.
 *
 * White and black, the first color allocated becomes the background
 * color.
 */
static void 
gsvit_alloc_colors ()
{
  int i;
  int numNets = PCB->NetlistLib.MenuN;
  char *rgb;

  white = (struct color_struct *) malloc (sizeof (*white));
  white->r = white->g = white->b = 255;
  white->c = gdImageColorAllocate (gsvit_im, white->r, white->g, white->b);

  black = (struct color_struct *) malloc (sizeof (*black));
  black->r = black->g = black->b = 0;
  black->c = gdImageColorAllocate (gsvit_im, black->r, black->g, black->b);

  for (i = 0; i < numNets; i++) {
    gsvit_netlist[i].colorIndex = i;
    color_array[i] =  malloc (sizeof (*white));
    color_array[i]->r = gsvit_netlist[i].color.r;
    color_array[i]->g = gsvit_netlist[i].color.g;
    color_array[i]->b = gsvit_netlist[i].color.b;

    color_array[i]->c = gdImageColorAllocate (gsvit_im, color_array[i]->r,  color_array[i]->g,  color_array[i]->b);
  }

  color_array[i] =  malloc (sizeof (*white));
  rgb = hslToRgb (128, (20 * 256) / 100, (20 * 256) / 100);

  color_array[i]->r = rgb[0];
  color_array[i]->g = rgb[1];
  color_array[i]->b = rgb[2];
  color_array[i]->c = gdImageColorAllocate (gsvit_im, color_array[i]->r, color_array[i]->g, color_array[i]->b);

  printf ("%d colors allocated\n", numNets);
}


static void 
gsvit_start_png (const char *basename, const char *suffix)
{
  int h, w;
  char *buf;

  buf = gsvit_get_png_name (basename, suffix);

  h = pcb_to_gsvit (PCB->MaxHeight);
  w = pcb_to_gsvit (PCB->MaxWidth);

  /* gsvit only works with true color images. */
  gsvit_im = gdImageCreate (w, h);
  gsvit_f = fopen (buf, "wb");

  gsvit_alloc_colors ();

  free (buf);
}


static void 
gsvit_finish_png ()
{
  int i;
#ifdef HAVE_GDIMAGEPNG
  gdImagePng (gsvit_im, gsvit_f);
#else
  Message ("gsvit: PNG not supported by gd. Can't write layer mask.\n");
#endif
  gdImageDestroy (gsvit_im);
  fclose (gsvit_f);

  for (i = 0; i < 0x100; i++) {
    free (color_array[i]);
  }

  gsvit_im = NULL;
  gsvit_f = NULL;
}


void 
gsvit_start_png_export ()
{
  BoxType region;

  region.X1 = 0;
  region.Y1 = 0;
  region.X2 = PCB->MaxWidth;
  region.Y2 = PCB->MaxHeight;

  linewidth = -1;
  lastbrush = (gdImagePtr)((void *) -1);

  hid_expose_callback (&gsvit_hid, &region, 0);
}


static void 
gsvit_do_export (HID_Attr_Val *options)
{
  int save_ons[MAX_ALL_LAYER];
  int i, idx;
  char *buf;
  int len;


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
    fprintf (stderr, "ERROR:  dpi may not be < 0\n");
    return;
  }

  gsvit_create_netlist ();
  gsvit_choose_groups ();

  for (i = 0; i < MAX_GROUP; i++) {
    if (gsvit_export_group[i]) {
      gsvit_cur_group = i;
      /* Magic. */
      idx = (i >= 0 && i < max_group) ? PCB->LayerGroups.Entries[i][0] : i;
      save_drill = (GetLayerGroupNumberByNumber (idx) == GetLayerGroupNumberBySide (BOTTOM_SIDE)) ? 1 : 0;
      /* save drills for one layer only */
      gsvit_start_png (gsvit_basename, layer_type_to_file_name (idx, FNS_fixed));
      hid_save_and_show_layer_ons (save_ons);
      gsvit_start_png_export ();
      hid_restore_layer_ons (save_ons);
      gsvit_finish_png ();
    }
  }

  len = strlen (gsvit_basename) + 4;
  buf = (char *) malloc (sizeof (*buf) * len);

  gsvit_xml_out ((char*) gsvit_basename);
  gsvit_destroy_netlist ();
}


void
gsvit_xml_out (char *gsvit_basename)
{
  char *buf;
  int len;
  time_t t;

  len = strlen (gsvit_basename) + 4;
  buf = (char *) malloc (sizeof (*buf) * len);

  sprintf (buf, "%s.xem", gsvit_basename);
  XOUT_INIT (buf);
  free (buf);

  XOUT_HEADER ();
  XOUT_NEWLINE ();
  XOUT_ELEMENT_START ("gsvit");
  XOUT_INDENT ();
  XOUT_NEWLINE ();
  XOUT_ELEMENT ("comment", "Made with PCB gsvit export HID");
  XOUT_NEWLINE ();
  {
    char buff[0x100];
    char* src = buff;
    t = time (NULL);
    strncpy (buff, ctime(&t), 0x100);
    while (*src)
    {
      if ((*src =='\r') || (*src =='\n')) {
        *src = 0;
        break;
      }
      src++;
    }
    XOUT_ELEMENT ("genTime", buff);
    XOUT_NEWLINE ();
  }
  gsvit_write_xspace ();
  gsvit_write_xnets ();
  gsvit_write_xcentroids();
  gsvit_write_xdrills();

  XOUT_ELEMENT_END ("gsvit");
  XOUT_NEWLINE ();
  XOUT_CLOSE ();
}

/* *** PNG export (slightly modified code from PNG export HID) ************* */

static int 
gsvit_set_layer (const char *name, int group, int empty)
{
  int idx = (group >= 0 && group < max_group) ? PCB->LayerGroups.Entries[group][0] : group;

  if (name == 0) {
    name = PCB->Data->Layer[idx].Name;
  }

  if (strcmp(name, "invisible") == 0) {
    return 0;
  }

  is_drill = (SL_TYPE (idx) == SL_PDRILL || SL_TYPE (idx) == SL_UDRILL);
  is_mask = (SL_TYPE (idx) == SL_MASK);

  if (is_mask) {
    /* Don't print masks. */
    return 0;
  }

  if (is_drill) {
    if (SL_TYPE(idx) == SL_PDRILL)
      is_plated =1;
    else if (SL_TYPE(idx) == SL_UDRILL)
      is_plated =0;

    /* Print 'holes', so that we can fill gaps in the copper layer. */
    return 1;
  }

  if (group == gsvit_cur_group) {
    return 1;
  }

  return 0;
}


static hidGC 
gsvit_make_gc (void)
{
  hidGC rv = (hidGC) malloc (sizeof (struct hid_gc_struct));
  rv->me_pointer = &gsvit_hid;
  rv->cap = Trace_Cap;
  rv->width = 1;
  rv->color = (struct color_struct *) malloc (sizeof (*rv->color));
  rv->color->r = rv->color->g = rv->color->b = 0;
  rv->color->c = 0;
  return rv;
}


static void 
gsvit_destroy_gc (hidGC gc)
{
  free (gc);
}


static void 
gsvit_use_mask (enum mask_mode mode)
{
  /* Do nothing. */
}


static void 
gsvit_set_color (hidGC gc, const char *name)
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
    /*! \todo Should be background, not white. */
    gc->color = white;
    gc->erase = 1;
    return;
  }

  gc->color = black;
  gc->erase = 0;
  return;
}


static void
gsvit_set_line_cap (hidGC gc, EndCapStyle style)
{
  gc->cap = style;
}


static void
gsvit_set_line_width (hidGC gc, Coord width)
{
  gc->width = width;
}


static void
gsvit_set_draw_xor (hidGC gc, int xor_)
{
 /* Do nothing. */
}


static void
gsvit_set_draw_faded (hidGC gc, int faded)
{
  /* Do nothing. */
}


static void
use_gc (hidGC gc)
{
  int need_brush = 0;

  if (gc->me_pointer != &gsvit_hid) {
    fprintf (stderr, "Fatal: GC from another HID passed to gsvit HID\n");
    abort ();
  }

  if (hashColor != gdBrushed) {
    need_brush = 1;
  }

  if (linewidth != gc->width) {
    /* Make sure the scaling doesn't erase lines completely */
/*
    if (SCALE (gc->width) == 0 && gc->width > 0)
      gdImageSetThickness (im, 1);
    else
 */
    gdImageSetThickness (gsvit_im, pcb_to_gsvit (gc->width));
    linewidth = gc->width;
    need_brush = 1;
  }

  if (lastbrush != gc->brush || need_brush) {
    static void *bcache = 0;
    hidval bval;
    char name[256];
    char type;
    int r;

    switch (gc->cap) {
      case Round_Cap:
      case Trace_Cap:
        type = 'C';
        r = pcb_to_gsvit (gc->width / 2);
        break;
      default:
        case Square_Cap:
          r = pcb_to_gsvit(gc->width);
          type = 'S';
          break;
    }
    sprintf (name, "#%.2x%.2x%.2x_%c_%d", gc->color->r, gc->color->g, gc->color->b, type, r);
/*
    if (hid_cache_color(0, name, &bval, &bcache)) {
      gc->brush = (gdImagePtr) bval.ptr;
    }
    else {
 */
    {
      int bg, fg;

      if (type == 'C')
        gc->brush = gdImageCreate (2 * r + 1, 2 * r + 1);
      else
        gc->brush = gdImageCreate (r + 1, r + 1);

      bg = gdImageColorAllocate (gc->brush, 255, 255, 255);
      if (hashColor != gdBrushed) {
/*
        printf ("hash:%d\n",hashColor);
 */
        fg = gdImageColorAllocate (gc->brush, color_array[hashColor]->r,color_array[hashColor]->g,color_array[hashColor]->b);
      }
      else {
        fg = gdImageColorAllocate (gc->brush, gc->color->r, gc->color->g, gc->color->b);
      }
      gdImageColorTransparent (gc->brush, bg);

      /* If we shrunk to a radius/box width of zero, then just use a
       * single pixel to draw with.
       */
      if (r == 0) {
        gdImageFilledRectangle (gc->brush, 0, 0, 0, 0, fg);
      }  
      else {
        if (type == 'C')
          gdImageFilledEllipse (gc->brush, r, r, 2 * r, 2 * r, fg);
        else
          gdImageFilledRectangle (gc->brush, 0, 0, r, r, fg);
      }
      bval.ptr = gc->brush;
      hid_cache_color (1, name, &bval, &bcache);
    }

    gdImageSetBrush (gsvit_im, gc->brush);
    lastbrush = gc->brush;
  }
}


static void
gsvit_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  use_gc (gc);

  gdImageRectangle (gsvit_im, pcb_to_gsvit (x1), pcb_to_gsvit (y1),
    pcb_to_gsvit (x2), pcb_to_gsvit (y2), gc->color->c);
}


static void
gsvit_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  use_gc (gc);
  gdImageSetThickness (gsvit_im, 0);
  linewidth = 0;

  gdImageFilledRectangle (gsvit_im, pcb_to_gsvit (x1), pcb_to_gsvit (y1),
    pcb_to_gsvit (x2), pcb_to_gsvit (y2), gc->color->c);
}


struct gsvit_netlist *
gsvit_lookup_net_from_arc (ArcType *targetArc)
{
  int i;

  for (i = 0; i < PCB->NetlistLib.MenuN; i++) {
    /* For each net in the netlist. */
    struct gsvit_netlist *currNet = &gsvit_netlist[i];
    int j;

    for (j = 0; j < PCB->LayerGroups.Number[gsvit_cur_group]; j++) {
      /* For each layer of the current group. */
      int layer = PCB->LayerGroups.Entries[gsvit_cur_group][j];

      ARC_LOOP (&(currNet->layer[layer]));
      {
        if (targetArc == arc)
          return (currNet);
      }
      END_LOOP;
    }
  }

  return (NULL);
}


struct gsvit_netlist *
gsvit_lookup_net_from_line (LineType *targetLine)
{
  int i;

  for (i = 0; i < PCB->NetlistLib.MenuN; i++) {
    /* For each net in the netlist. */
    struct gsvit_netlist* currNet = &gsvit_netlist[i];
    int j;

    for (j = 0; j < PCB->LayerGroups.Number[gsvit_cur_group]; j++) {
      /* For each layer of the current group. */
      int layer = PCB->LayerGroups.Entries[gsvit_cur_group][j];

      LINE_LOOP (&(currNet->layer[layer]));
      {
        if (targetLine == line)
          return (currNet);
      }
      END_LOOP;

    }
  }

  return (NULL);
}


struct gsvit_netlist *
gsvit_lookup_net_from_polygon (PolygonType *targetPolygon)
{
  int i;

  for (i = 0; i < PCB->NetlistLib.MenuN; i++) {
    /* For each net in the netlist. */
    struct gsvit_netlist* currNet = &gsvit_netlist[i];
    int j;

    for (j = 0; j < PCB->LayerGroups.Number[gsvit_cur_group]; j++) {
      /* For each layer of the current group. */
      int layer = PCB->LayerGroups.Entries[gsvit_cur_group][j];

      POLYGON_LOOP (&(currNet->layer[layer]));
      {
        if (targetPolygon == polygon)
          return (currNet);
      }
      END_LOOP;

    }
  }

  return (NULL);
}


struct gsvit_netlist *
gsvit_lookup_net_from_pad (PadType *targetPad)
{
  int i;

  for (i = 0; i < PCB->NetlistLib.MenuN; i++) {
    /* For each net in the netlist. */
    struct gsvit_netlist* currNet = &gsvit_netlist[i];

    PAD_LOOP (currNet);
    {
      if (targetPad == pad)
        return (currNet);
    }
    END_LOOP;

  }

  return (NULL);
}


struct gsvit_netlist *
gsvit_lookup_net_from_pv (PinType *targetPv)
{
  int i;

  for (i = 0; i < PCB->NetlistLib.MenuN; i++) {
    /* For each net in the netlist. */
    struct gsvit_netlist* currNet = &gsvit_netlist[i];

    PIN_LOOP (currNet);
    {
      if (targetPv == pin)
        return (currNet);
    }
    END_LOOP;

    VIA_LOOP (currNet);
    {
      if (targetPv == via)
        return (currNet);
    }
    END_LOOP;
  }

  return (NULL);
}


static void
add_hole (struct single_size_drills* drill, int cx, int cy)
{
  if (drill->n_holes == drill->n_holes_allocated) {
    drill->n_holes_allocated += 100;
    drill->holes = (struct drill_hole *) realloc (drill->holes,drill->n_holes_allocated *sizeof (struct drill_hole));
  }
  drill->holes[drill->n_holes].cx = cx;
  drill->holes[drill->n_holes].cy = cy;
  drill->holes[drill->n_holes].is_plated = is_plated;
  drill->n_holes++;
  printf ("holes %d\n", drill->n_holes);
}


/*!
* \brief Given a hole size, return the structure that currently holds
* the data for that hole size.
*
* If there isn't one, make it.
*/
static int
_drill_size_comparator (const void* _size0, const void* _size1)
{
  double size0 = ((const struct single_size_drills*)_size0)->diameter_inches;
  double size1 = ((const struct single_size_drills*)_size1)->diameter_inches;
  if (size0 == size1)
    return 0;
  if (size0 < size1)
    return -1;
  return 1;
}


static struct single_size_drills *
get_drill (double diameter_inches, Coord radius)
{
  /* See if we already have this size. If so, return that structure. */
  struct single_size_drills* drill = bsearch (&diameter_inches,
    drills, n_drills, sizeof (drills[0]),
    _drill_size_comparator);

  if (drill != NULL)
    return( drill);

  /* Haven't seen this hole size before, so make a new structure for it. */
  if (n_drills == n_drills_allocated) {
    n_drills_allocated += 100;
    drills = (struct single_size_drills *) realloc (drills,
      n_drills_allocated * sizeof (struct single_size_drills));
  }

  /* I now add the structure to the list, making sure to keep the list
   * sorted. Ideally the bsearch() call above would have given me the location
   * to insert this element while keeping things sorted, but it doesn't. For
   * simplicity I manually lsearch() to find this location myself */
  {
    int i = 0;
    for (i = 0; i<n_drills; i++)
      if (drills[i].diameter_inches >= diameter_inches)
        break;

    if (n_drills != i)
      memmove (&drills[i+1], &drills[i],
        (n_drills-i) * sizeof (struct single_size_drills));

    drills[i].diameter_inches = diameter_inches;
    drills[i].radius = radius;
    drills[i].n_holes = 0;
    drills[i].n_holes_allocated = 0;
    drills[i].holes = NULL;
    n_drills++;
    return &drills[i];
  }
}


static void
gsvit_draw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  if (x1 == x2 && y1 == y2) {
    Coord w = gc->width / 2;
    gsvit_fill_rect (gc, x1 - w, y1 - w, x1 + w, y1 + w);
    return;
  }

  linewidth = -1;
  use_gc (gc);
  linewidth = -1;

  gdImageSetThickness (gsvit_im, 0);
  linewidth = 0;
  gdImageLine (gsvit_im, pcb_to_gsvit (x1), pcb_to_gsvit (y1),
    pcb_to_gsvit (x2), pcb_to_gsvit (y2), gdBrushed);
}


static void
gsvit_draw_arc (hidGC gc, Coord cx, Coord cy, Coord width, Coord height, Angle start_angle, Angle delta_angle)
{
  Angle sa, ea;

  /* In gdImageArc, 0 degrees is to the right and +90 degrees is down.
   * in pcb, 0 degrees is to the left and +90 degrees is down.
   */
  start_angle = 180 - start_angle;
  delta_angle = -delta_angle;
  if (delta_angle > 0) {
    sa = start_angle;
    ea = start_angle + delta_angle;
  }
  else {
    sa = start_angle + delta_angle;
    ea = start_angle;
  }

  /* Make sure we start between 0 and 360 otherwise gd does strange
   * things.
   */
  sa = NormalizeAngle (sa);
  ea = NormalizeAngle (ea);

#if 0
  printf("draw_arc %d,%d %dx%d %d..%d %d..%d\n",
    cx, cy, width, height, start_angle, delta_angle, sa, ea);
  printf("gdImageArc (%p, %d, %d, %d, %d, %d, %d, %d)\n",
    im, SCALE_X (cx), SCALE_Y (cy), SCALE (width), SCALE (height), sa, ea,
    gc->color->c);
#endif
  use_gc (gc);
  gdImageSetThickness (gsvit_im, 0);
  linewidth = 0;
  gdImageArc (gsvit_im, pcb_to_gsvit (cx), pcb_to_gsvit (cy),
    pcb_to_gsvit (2 * width), pcb_to_gsvit (2 * height), sa, ea, gdBrushed);
}


static void
gsvit_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius)
{
  use_gc (gc);

  gdImageSetThickness (gsvit_im, 0);
  linewidth = 0;
  gdImageFilledEllipse (gsvit_im, pcb_to_gsvit (cx), pcb_to_gsvit (cy),
    pcb_to_gsvit (2 * radius), pcb_to_gsvit (2 * radius), color_array[hashColor]->c);

  if (save_drill && is_drill)
  {
    double diameter_inches = COORD_TO_INCH(radius*2);
    struct single_size_drills* drill = get_drill (diameter_inches, radius);
    add_hole(drill, pcb_to_gsvit(cx), pcb_to_gsvit(cy));
      /* convert to inch, flip: will drill from bottom side */
//      COORD_TO_INCH(PCB->MaxWidth  - cx),
      /* PCB reverses y axis */
//      COORD_TO_INCH(PCB->MaxHeight - cy));
  }
}


static void
gsvit_fill_polygon (hidGC gc, int n_coords, Coord *x, Coord *y)
{
  int i;
  gdPoint *points;

  points = (gdPoint *) malloc (n_coords * sizeof (gdPoint));

  if (points == NULL) {
    fprintf (stderr, "ERROR:  gsvit_fill_polygon():  malloc failed\n");
    exit (1);
  }

  use_gc (gc);

  for (i = 0; i < n_coords; i++) {
    points[i].x = pcb_to_gsvit (x[i]);
    points[i].y = pcb_to_gsvit (y[i]);
  }

  gdImageSetThickness (gsvit_im, 0);
  linewidth = 0;
  gdImageFilledPolygon (gsvit_im, points, n_coords, color_array[hashColor]->c);

  free (points);
}


static void
gsvit_calibrate (double xval, double yval)
{
  CRASH;
}


static void
gsvit_set_crosshair (int x, int y, int a)
{
  /* Do nothing. */
}


static void
gsvit_draw_pcb_arc (hidGC gc, ArcType *arc)
{
  struct gsvit_netlist *net;

  net = gsvit_lookup_net_from_arc (arc);

  if (net) {
    hashColor = net->colorIndex;
  }
  else {
    hashColor = PCB->NetlistLib.MenuN;
  }

  common_draw_pcb_arc (gc, arc);
  hashColor = PCB->NetlistLib.MenuN;
}


static void
gsvit_draw_pcb_line (hidGC gc, LineType *line)
{
  struct gsvit_netlist *net;

  net = gsvit_lookup_net_from_line (line);

  if (net) {
    hashColor = net->colorIndex;
  }
  else {
    hashColor = PCB->NetlistLib.MenuN;
  }

  common_draw_pcb_line (gc, line);
  hashColor = PCB->NetlistLib.MenuN;

}


void
gsvit_fill_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box)
{
  /* Hijack the fill_pcb_polygon function to get *poly, then proceed
   * with the default handler.
   */
  struct gsvit_netlist *net;

  net = gsvit_lookup_net_from_polygon (poly);

  if (net) {
    hashColor = net->colorIndex;
  }
  else {
    hashColor = PCB->NetlistLib.MenuN;
  }

  common_fill_pcb_polygon (gc, poly, clip_box);
  hashColor = PCB->NetlistLib.MenuN;
}


void
gsvit_fill_pcb_pad (hidGC gc, PadType *pad, bool clear, bool mask)
{
  struct gsvit_netlist *net = NULL;

  net = gsvit_lookup_net_from_pad(pad);

  if (net) {
    hashColor = net->colorIndex;
  }
  else {
    hashColor = PCB->NetlistLib.MenuN;
  }

  common_fill_pcb_pad (gc, pad, clear, mask);
  hashColor = PCB->NetlistLib.MenuN;
}


void
gsvit_fill_pcb_pv (hidGC fg_gc, hidGC bg_gc, PinType *pv, bool drawHole, bool mask)
{
  struct gsvit_netlist *net = NULL;

  net = gsvit_lookup_net_from_pv (pv);

  if (net) {
    hashColor = net->colorIndex;
  }
  else {
    hashColor = PCB->NetlistLib.MenuN;
  }

  common_fill_pcb_pv (fg_gc, bg_gc, pv, drawHole, mask);
  hashColor = PCB->NetlistLib.MenuN;
}


#include "dolists.h"


void
hid_gsvit_init ()
{
  memset (&gsvit_hid, 0, sizeof (HID));
  memset (&gsvit_graphics, 0, sizeof (HID_DRAW));

  common_nogui_init (&gsvit_hid);
  common_draw_helpers_init (&gsvit_graphics);

//hid_gtk_init();
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

  /* Hijack these functions because what is passed to fill_polygon is a
   * series of polygons (for holes,...).
   */
  gsvit_graphics.draw_pcb_line = gsvit_draw_pcb_line;
  gsvit_graphics.draw_pcb_arc = gsvit_draw_pcb_arc;
  gsvit_graphics.draw_pcb_polygon = gsvit_fill_pcb_polygon;

  gsvit_graphics.fill_pcb_polygon = gsvit_fill_pcb_polygon;
  gsvit_graphics.fill_pcb_pad = gsvit_fill_pcb_pad;
  gsvit_graphics.fill_pcb_pv = gsvit_fill_pcb_pv;

  hid_register_hid (&gsvit_hid);

#include "gsvit_lists.h"
}
