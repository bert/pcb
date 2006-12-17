/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "global.h"
#include "data.h"
#include "misc.h"

#include "hid.h"
#include "../hidint.h"
#include "../ps/ps.h"
#include "../../print.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented PS function %s.\n", __FUNCTION__); abort()

typedef struct hid_gc_struct
{
  HID *me_pointer;
  EndCapStyle cap;
  int width;
  unsigned char r, g, b;
  int erase;
  int faded;
} hid_gc_struct;

static FILE *f = 0;
static int pagecount = 0;
static int linewidth = -1;
static int lastgroup = -1;
static int lastcap = -1;
static int lastcolor = -1;
static int print_group[MAX_LAYER];
static int print_layer[MAX_LAYER];
static double fade_ratio = 0.4;
static double antifade_ratio = 0.6;
static int multi_file = 0;
static double media_width, media_height, ps_width, ps_height;

static const char *medias[] = { 
  "A0", "A1", "A2", "A3", "A4", "A5",
  "A6", "A7", "A8", "A9", "A10",
  "B0", "B1", "B2", "B3", "B4", "B5",
  "B6", "B7", "B8", "B9", "B10",
  "Letter", "11x17", "Ledger",
  "Legal", "Executive", 
  "A-Size", "B-size",
  "C-Size", "D-size", "E-size",
  "US-Business_Card", "Intl-Business_Card",
  0
};

typedef struct
{
  char *name;
  long int Width, Height;
  long int MarginX, MarginY;
} MediaType, *MediaTypePtr;

/*
 * Metric ISO sizes in mm.  See http://en.wikipedia.org/wiki/ISO_paper_sizes
 *
 * A0  841 x 1189
 * A1  594 x 841
 * A2  420 x 594
 * A3  297 x 420
 * A4  210 x 297
 * A5  148 x 210
 * A6  105 x 148
 * A7   74 x 105
 * A8   52 x  74
 * A9   37 x  52
 * A10  26 x  37
 *
 * B0  1000 x 1414
 * B1   707 x 1000
 * B2   500 x  707
 * B3   353 x  500
 * B4   250 x  353
 * B5   176 x  250
 * B6   125 x  176
 * B7    88 x  125
 * B8    62 x   88
 * B9    44 x   62
 * B10   31 x   44
 *
 * awk '{printf("  {\"%s\", %d, %d, MARGINX, MARGINY},\n", $2, $3*100000/25.4, $5*100000/25.4)}'
 *
 * See http://en.wikipedia.org/wiki/Paper_size#Loose_sizes for some of the other sizes.  The
 * {A,B,C,D,E}-Size here are the ANSI sizes and not the architectural sizes.
 */

#define MARGINX 50000
#define MARGINY 50000

static MediaType media_data[] = {
  {"A0", 3311023, 4681102, MARGINX, MARGINY},
  {"A1", 2338582, 3311023, MARGINX, MARGINY},
  {"A2", 1653543, 2338582, MARGINX, MARGINY},
  {"A3", 1169291, 1653543, MARGINX, MARGINY},
  {"A4", 826771, 1169291, MARGINX, MARGINY},
  {"A5", 582677, 826771, MARGINX, MARGINY},
  {"A6", 413385, 582677, MARGINX, MARGINY},
  {"A7", 291338, 413385, MARGINX, MARGINY},
  {"A8", 204724, 291338, MARGINX, MARGINY},
  {"A9", 145669, 204724, MARGINX, MARGINY},
  {"A10", 102362, 145669, MARGINX, MARGINY},
  {"B0", 3937007, 5566929, MARGINX, MARGINY},
  {"B1", 2783464, 3937007, MARGINX, MARGINY},
  {"B2", 1968503, 2783464, MARGINX, MARGINY},
  {"B3", 1389763, 1968503, MARGINX, MARGINY},
  {"B4", 984251, 1389763, MARGINX, MARGINY},
  {"B5", 692913, 984251, MARGINX, MARGINY},
  {"B6", 492125, 692913, MARGINX, MARGINY},
  {"B7", 346456, 492125, MARGINX, MARGINY},
  {"B8", 244094, 346456, MARGINX, MARGINY},
  {"B9", 173228, 244094, MARGINX, MARGINY},
  {"B10", 122047, 173228, MARGINX, MARGINY},
  {"Letter", 850000, 1100000, MARGINX, MARGINY},
  {"11x17", 1100000, 1700000, MARGINX, MARGINY},
  {"Ledger", 1700000, 1100000, MARGINX, MARGINY},
  {"Legal", 850000, 1400000, MARGINX, MARGINY},
  {"Executive", 750000, 1000000, MARGINX, MARGINY},
  {"A-size",  850000, 1100000, MARGINX, MARGINY},
  {"B-size", 1100000, 1700000, MARGINX, MARGINY},
  {"C-size", 1700000, 2200000, MARGINX, MARGINY},
  {"D-size", 2200000, 3400000, MARGINX, MARGINY},
  {"E-size", 3400000, 4400000, MARGINX, MARGINY},
  {"US-Business_Card", 350000, 200000, 0, 0},
  {"Intl-Business_Card", 337500, 212500, 0, 0}
};

#undef MARGINX
#undef MARGINY

HID_Attribute ps_attribute_list[] = {
  /* other HIDs expect this to be first.  */
  {"psfile", "Postscript output file",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_psfile 0
  {"drill-helper", "Prints a centering target in large drill holes",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_drillhelper 1
  {"align-marks", "Prints alignment marks on each layer",
   HID_Boolean, 0, 0, {1, 0, 0}, 0, 0},
#define HA_alignmarks 2
  {"outline", "Prints outline on each layer",
   HID_Boolean, 0, 0, {1, 0, 0}, 0, 0},
#define HA_outline 3
  {"mirror", "Prints mirror image of each layer",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_mirror 4
  {"fill-page", "Scale board to fill page",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_fillpage 5
  {"auto-mirror", "Prints mirror image of appropriate layers",
   HID_Boolean, 0, 0, {1, 0, 0}, 0, 0},
#define HA_automirror 6
  {"ps-color", "Prints in color",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_color 7
  {"ps-bloat", "Amount to add to trace/pad/pin edges (1 = 1/100 mil)",
   HID_Integer, 0, 10000, {0, 0, 0}, 0, 0},
#define HA_psbloat 8
  {"ps-invert", "Draw images as white-on-black",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_psinvert 9
  {"media", "media type",
   HID_Enum, 0, 0, {22, 0, 0}, medias, 0},
#define HA_media 10
  {"psfade", "Fade amount for assembly drawings (0.0=missing, 1.0=solid)",
   HID_Real, 0, 1, {0, 0, 0.40}, 0, 0},
#define HA_psfade 11
  {"scale", "Scale value to compensate for printer sizing errors (1.0 = full scale)",
   HID_Real, 0.01, 4, {0, 0, 1.00}, 0, 0},
#define HA_scale 12
  {"multi-file", "Produce multiple files, one per page, instead of a single file.",
   HID_Boolean, 0, 0, {0, 0, 0.40}, 0, 0},
#define HA_multifile 13
};

#define NUM_OPTIONS (sizeof(ps_attribute_list)/sizeof(ps_attribute_list[0]))

REGISTER_ATTRIBUTES (ps_attribute_list)

static HID_Attr_Val ps_values[NUM_OPTIONS];

static HID_Attribute *
ps_get_export_options (int *n)
{
  static char *last_made_filename = 0;
  if (PCB)
    derive_default_filename(PCB->Filename, &ps_attribute_list[HA_psfile], ".ps", &last_made_filename);

  if (n)
    *n = NUM_OPTIONS;
  return ps_attribute_list;
}

static int
group_for_layer (int l)
{
  if (l < max_layer + 2 && l >= 0)
    return GetLayerGroupNumberByNumber (l);
  /* else something unique */
  return max_layer + 3 + l;
}

static int
layer_sort (const void *va, const void *vb)
{
  int a = *(int *) va;
  int b = *(int *) vb;
  int d = group_for_layer (b) - group_for_layer (a);
  if (d)
    return d;
  return b - a;
}

static char *filename;
static int drill_helper;
static int align_marks;
static int outline;
static int mirror;
static int fillpage;
static int automirror;
static int incolor;
static int doing_toc;
static int bloat;
static int invert;
static int media;

static double fill_zoom;
static double scale_value;

void
ps_start_file (FILE *f)
{
  fprintf (f, "%%!PS-Adobe-3.0\n\n");
}

static FILE *
psopen (const char *base, const char *suff)
{
  FILE *f;
  char *buf;
  if (!multi_file)
    {
      printf("PS: open %s\n", base);
      return fopen (base, "w");
    }
  buf = malloc (strlen (base) + strlen (suff) + 5);
  sprintf(buf, "%s.%s.ps", base, suff);
  printf("PS: open %s\n", buf);
  f = fopen(buf, "w");
  free (buf);
  return f;
}

/* This is used by other HIDs that use a postscript format, like lpr
   or eps.  */
void
ps_hid_export_to_file (FILE * the_file, HID_Attr_Val * options)
{
  int i;
  static int saved_layer_stack[MAX_LAYER];
  BoxType region;

  f = the_file;
  drill_helper = options[HA_drillhelper].int_value;
  align_marks = options[HA_alignmarks].int_value;
  outline = options[HA_outline].int_value;
  mirror = options[HA_mirror].int_value;
  fillpage = options[HA_fillpage].int_value;
  automirror = options[HA_automirror].int_value;
  incolor = options[HA_color].int_value;
  bloat = options[HA_psbloat].int_value;
  invert = options[HA_psinvert].int_value;
  fade_ratio = options[HA_psfade].real_value;
  media = options[HA_media].int_value;
  media_width = media_data[media].Width / 1e5;
  media_height = media_data[media].Height / 1e5;
  ps_width = media_width - 2.0*media_data[media].MarginX / 1e5;
  ps_height = media_height - 2.0*media_data[media].MarginY / 1e5;
  scale_value = options[HA_scale].real_value;

  if (fade_ratio < 0)
    fade_ratio = 0;
  if (fade_ratio > 1)
    fade_ratio = 1;
  antifade_ratio = 1.0 - fade_ratio;

  if (fillpage)
    {
      double zx, zy;
      if (PCB->MaxWidth > PCB->MaxHeight)
	{
	  zx = ps_height / PCB->MaxWidth;
	  zy = ps_width / PCB->MaxHeight;
	}
      else
	{
	  zx = ps_height / PCB->MaxHeight;
	  zy = ps_width / PCB->MaxWidth;
	}
      if (zx < zy)
	fill_zoom = zx;
      else
	fill_zoom = zy;
    }
  else
    fill_zoom = 0.00001;


  memset (print_group, 0, sizeof (print_group));
  memset (print_layer, 0, sizeof (print_layer));

  for (i = 0; i < max_layer; i++)
    {
      LayerType *layer = PCB->Data->Layer + i;
      if (layer->LineN || layer->TextN || layer->ArcN || layer->PolygonN)
	print_group[GetLayerGroupNumberByNumber (i)] = 1;
    }
  print_group[GetLayerGroupNumberByNumber (max_layer)] = 1;
  print_group[GetLayerGroupNumberByNumber (max_layer + 1)] = 1;
  for (i = 0; i < max_layer; i++)
    if (print_group[GetLayerGroupNumberByNumber (i)])
      print_layer[i] = 1;

  memcpy (saved_layer_stack, LayerStack, sizeof (LayerStack));
  qsort (LayerStack, max_layer, sizeof (LayerStack[0]), layer_sort);

  lastgroup = -1;
  linewidth = -1;
  lastcap = -1;
  lastcolor = -1;

  region.X1 = 0;
  region.Y1 = 0;
  region.X2 = PCB->MaxWidth;
  region.Y2 = PCB->MaxHeight;

  if (! multi_file)
    {
      pagecount = 1;
      fprintf (f, "%%%%Page: 1\n");
      fprintf (f, "/Times-Roman findfont 24 scalefont setfont\n");
      fprintf (f,
	       "/rightshow { /s exch def s stringwidth pop -1 mul 0 rmoveto s show } def\n");
      fprintf (f,
	       "/y 72 9 mul def /toc { 100 y moveto show /y y 24 sub def } bind def\n");
      fprintf (f, "/tocp { /y y 12 sub def 90 y moveto rightshow } bind def\n");
      doing_toc = 1;
      hid_expose_callback (&ps_hid, &region, 0);
    }

  pagecount = 1;
  doing_toc = 0;
  lastgroup = -1;
  hid_expose_callback (&ps_hid, &region, 0);

  fprintf (f, "showpage\n");

  memcpy (LayerStack, saved_layer_stack, sizeof (LayerStack));
}

static void
ps_do_export (HID_Attr_Val * options)
{
  int save_ons[MAX_LAYER + 2];
  int i;

  if (!options)
    {
      ps_get_export_options (0);
      for (i = 0; i < NUM_OPTIONS; i++)
	ps_values[i] = ps_attribute_list[i].default_val;
      options = ps_values;
    }

  filename = options[HA_psfile].str_value;
  if (!filename)
    filename = "pcb-out.ps";

  multi_file = options[HA_multifile].int_value;

  if (multi_file)
    f = 0;
  else
    {
      f = psopen (filename, "toc");
      if (!f)
	{
	  perror (filename);
	  return;
	}
      ps_start_file (f);
    }

  hid_save_and_show_layer_ons (save_ons);
  ps_hid_export_to_file (f, options);
  hid_restore_layer_ons (save_ons);

  multi_file = 0;
  fclose (f);
}

extern void hid_parse_command_line (int *argc, char ***argv);

static void
ps_parse_arguments (int *argc, char ***argv)
{
  hid_register_attributes (ps_attribute_list,
			   sizeof (ps_attribute_list) /
			   sizeof (ps_attribute_list[0]));
  hid_parse_command_line (argc, argv);
}

static void
corner (int x, int y, int dx, int dy)
{
#if 0
  int len = (PCB->MaxWidth + PCB->MaxHeight) / 10;
  int len2 = (PCB->MaxWidth + PCB->MaxHeight) / 50;
#else
  int len = 200000;
  int len2 = 20000;
#endif
  int thick = 4000;

  fprintf (f, "gsave %d setlinewidth %d %d translate %d %d scale\n",
	   thick * 2, x, y, dx, dy);
  fprintf (f, "%d %d moveto %d %d %d 0 90 arc %d %d lineto\n",
	   len, thick, thick, thick, len2 + thick, thick, len);
  if (dx < 0 && dy < 0)
    fprintf (f, "%d %d moveto 0 %d rlineto\n",
	     len2 * 2 + thick, thick, -len2);
  fprintf (f, "stroke grestore\n");
}

static int is_mask;
static int is_drill;
static int is_assy;

static int
ps_set_layer (const char *name, int group)
{
  int idx = (group >= 0
	     && group <
	     max_layer) ? PCB->LayerGroups.Entries[group][0] : group;
  if (name == 0)
    name = PCB->Data->Layer[idx].Name;

  if (idx >= 0 && idx < max_layer && !print_layer[idx])
    return 0;

  if (strcmp (name, "invisible") == 0)
    return 0;

  is_drill = (SL_TYPE (idx) == SL_PDRILL || SL_TYPE (idx) == SL_UDRILL);
  is_mask = (SL_TYPE (idx) == SL_MASK);
  is_assy = (SL_TYPE (idx) == SL_ASSY);
#if 0
  printf ("Layer %s group %d drill %d mask %d\n", name, group, is_drill,
	  is_mask);
#endif

  if (doing_toc)
    {
      if (group < 0 || group != lastgroup)
	{
	  pagecount++;
	  lastgroup = group;
	  fprintf (f, "(%d.) tocp\n", pagecount);
	}
      fprintf (f, "(%s) toc\n", name);
      return 0;
    }

  if (group < 0 || group != lastgroup)
    {
      double boffset;
      lastgroup = group;
      int mirror_this = 0;

      if (f && pagecount)
	{
	  fprintf (f, "showpage\n");
	}
      pagecount++;
      if (multi_file)
	{
	  if (f)
	    fclose (f);
	  f = psopen (filename, layer_type_to_file_name (idx));
	  ps_start_file (f);
	}
      fprintf (f, "%%%%Page: %d\n", pagecount);

      if (mirror)
	mirror_this = 1 - mirror_this;
      if (automirror
	  &&
	  ((idx >= 0 && group == GetLayerGroupNumberByNumber (max_layer))
	   || (idx < 0 && SL_SIDE (idx) == SL_BOTTOM_SIDE)))
	mirror_this = 1 - mirror_this;

      fprintf (f, "/Helvetica findfont 10 scalefont setfont\n");
      fprintf (f, "30 30 moveto (%s) show\n", PCB->Filename);
      fprintf (f, "30 41 moveto (%s, %s) show\n",
	       PCB->Name, layer_type_to_file_name (idx));
      if (mirror_this)
	fprintf (f, "( \\(mirrored\\)) show\n");

      fprintf (f, "(, scale = 1:%.3f) show\n", scale_value);

      fprintf (f, "72 72 scale %g %g translate\n", 0.5*media_width, 0.5*media_height);

      boffset = 0.5*media_height;
      if (PCB->MaxWidth > PCB->MaxHeight)
	{
	  fprintf (f, "90 rotate\n");
	  boffset = 0.5*media_width;
	}

      if (mirror_this)
	fprintf (f, "1 -1 scale\n");

      if (SL_TYPE (idx) == SL_FAB)
	fprintf (f, "0.00001 dup neg scale\n");
      else
	fprintf (f, "%g dup neg scale\n", (fill_zoom * scale_value));
      fprintf (f, "%d %d translate\n",
	       -PCB->MaxWidth / 2, -PCB->MaxHeight / 2);

      /* Keep the drill list from falling off the left edge of the paper,
       * even if it means some of the board falls off the right edge.
       * If users don't want to make smaller boards, or use fewer drill
       * sizes, they can always ignore this sheet. */
      if (SL_TYPE (idx) == SL_FAB) {
        int natural = (int) ((boffset - 0.5) * 100000) - PCB->MaxHeight / 2;
	int needed  = PrintFab_overhang();
        fprintf (f, "%% PrintFab overhang natural %d, needed %d\n", natural, needed);
	if (needed > natural)
	  fprintf (f, "0 %d translate\n", needed - natural);
      }

      if (invert)
	{
	  fprintf (f, "/gray { 1 exch sub setgray } bind def\n");
	  fprintf (f,
		   "/rgb { 1 1 3 { pop 1 exch sub 3 1 roll } for setrgbcolor } bind def\n");
	}
      else
	{
	  fprintf (f, "/gray { setgray } bind def\n");
	  fprintf (f, "/rgb { setrgbcolor } bind def\n");
	}

      if (outline || invert)
	fprintf (f,
		 "0 setgray 0 setlinewidth 0 0 moveto 0 %d lineto %d %d lineto %d 0 lineto closepath %s\n",
		 PCB->MaxHeight, PCB->MaxWidth, PCB->MaxHeight, PCB->MaxWidth,
		 invert ? "fill" : "stroke");

      if (align_marks)
	{
	  corner (0, 0, -1, -1);
	  corner (PCB->MaxWidth, 0, 1, -1);
	  corner (PCB->MaxWidth, PCB->MaxHeight, 1, 1);
	  corner (0, PCB->MaxHeight, -1, 1);
	}

      linewidth = -1;
      lastcap = -1;
      lastcolor = -1;

      fprintf (f, "/ts 10000 def\n");
      fprintf (f,
	       "/ty ts neg def /tx 0 def /Helvetica findfont ts scalefont setfont\n");
      fprintf (f, "/t { moveto lineto stroke } bind def\n");
      fprintf (f,
	       "/r { /y2 exch def /x2 exch def /y1 exch def /x1 exch def\n");
      fprintf (f,
	       "     x1 y1 moveto x1 y2 lineto x2 y2 lineto x2 y1 lineto closepath fill } bind def\n");
      fprintf (f, "/c { 0 360 arc fill } bind def\n");
      fprintf (f,
	       "/a { gsave setlinewidth translate scale 0 0 1 5 3 roll arc stroke grestore} bind def\n");
      if (drill_helper)
	fprintf (f,
		 "/dh { gsave %d setlinewidth 0 gray %d 0 360 arc stroke grestore} bind def\n",
		 MIN_PINORVIAHOLE, MIN_PINORVIAHOLE * 3 / 2);
    }
#if 0
  /* Try to outsmart ps2pdf's heuristics for page rotation, by putting
   * text on all pages -- even if that text is blank */
  if (SL_TYPE (idx) != SL_FAB)
    fprintf (f,
	     "gsave tx ty translate 1 -1 scale 0 0 moveto (Layer %s) show grestore newpath /ty ty ts sub def\n",
	     name);
  else
    fprintf (f, "gsave tx ty translate 1 -1 scale 0 0 moveto ( ) show grestore newpath /ty ty ts sub def\n");
#endif
  return 1;
}

static hidGC
ps_make_gc (void)
{
  hidGC rv = (hidGC) calloc (1, sizeof (hid_gc_struct));
  rv->me_pointer = &ps_hid;
  rv->cap = Trace_Cap;
  return rv;
}

static void
ps_destroy_gc (hidGC gc)
{
  free (gc);
}

static void
ps_use_mask (int use_it)
{
  /* does nothing */
}

static void
ps_set_color (hidGC gc, const char *name)
{
  if (strcmp (name, "erase") == 0 || strcmp (name, "drill") == 0)
    {
      gc->r = gc->g = gc->b = 255;
      gc->erase = 1;
    }
  else if (incolor)
    {
      int r, g, b;
      sscanf (name + 1, "%02x%02x%02x", &r, &g, &b);
      gc->r = r;
      gc->g = g;
      gc->b = b;
      gc->erase = 0;
    }
  else
    {
      gc->r = gc->g = gc->b = 0;
      gc->erase = 0;
    }
}

static void
ps_set_line_cap (hidGC gc, EndCapStyle style)
{
  gc->cap = style;
}

static void
ps_set_line_width (hidGC gc, int width)
{
  gc->width = width;
}

static void
ps_set_draw_xor (hidGC gc, int xor)
{
  ;
}

static void
ps_set_draw_faded (hidGC gc, int faded)
{
  gc->faded = faded;
}

static void
ps_set_line_cap_angle (hidGC gc, int x1, int y1, int x2, int y2)
{
  CRASH;
}

static void
use_gc (hidGC gc)
{
  if (gc->me_pointer != &ps_hid)
    {
      fprintf (stderr, "Fatal: GC from another HID passed to ps HID\n");
      abort ();
    }
  if (linewidth != gc->width)
    {
      fprintf (f, "%d setlinewidth\n",
	       gc->width + (gc->erase ? -2 : 2) * bloat);
      linewidth = gc->width;
    }
  if (lastcap != gc->cap)
    {
      int c;
      switch (gc->cap)
	{
	case Round_Cap:
	case Trace_Cap:
	  c = 1;
	  break;
	default:
	case Square_Cap:
	  c = 2;
	  break;
	}
      fprintf (f, "%d setlinecap %d setlinejoin\n", c, c);
      lastcap = gc->cap;
    }
#define CBLEND(gc) (((gc->r)<<24)|((gc->g)<<16)|((gc->b)<<8)|(gc->faded))
  if (lastcolor != CBLEND (gc))
    {
      if (is_drill || is_mask)
	{
	  fprintf (f, "%d gray\n", gc->erase ? 0 : 1);
	  lastcolor = 0;
	}
      else
	{
	  double r, g, b;
	  r = gc->r;
	  g = gc->g;
	  b = gc->b;
	  if (gc->faded)
	    {
	      r = antifade_ratio * 255 + fade_ratio * r;
	      g = antifade_ratio * 255 + fade_ratio * g;
	      b = antifade_ratio * 255 + fade_ratio * b;
	    }
	  if (gc->r == gc->g && gc->g == gc->b)
	    fprintf (f, "%g gray\n", r / 255.0);
	  else
	    fprintf (f, "%g %g %g rgb\n", r / 255.0, g / 255.0, b / 255.0);
	  lastcolor = CBLEND (gc);
	}
    }
}

static void
ps_draw_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  use_gc (gc);
  fprintf (f, "%d %d %d %d r\n", x1, y1, x2, y2);
}

static void ps_fill_rect (hidGC gc, int x1, int y1, int x2, int y2);

static void
ps_draw_line (hidGC gc, int x1, int y1, int x2, int y2)
{
  if (x1 == x2 && y1 == y2)
    {
      int w = gc->width / 2;
      ps_fill_rect (gc, x1 - w, y1 - w, x1 + w, y1 + w);
      return;
    }
  use_gc (gc);
  fprintf (f, "%d %d %d %d t\n", x1, y1, x2, y2);
}

static void
ps_draw_arc (hidGC gc, int cx, int cy, int width, int height,
	     int start_angle, int delta_angle)
{
  int sa, ea;
  if (delta_angle > 0)
    {
      sa = start_angle;
      ea = start_angle + delta_angle;
    }
  else
    {
      sa = start_angle + delta_angle;
      ea = start_angle;
    }
#if 0
  printf ("draw_arc %d,%d %dx%d %d..%d %d..%d\n",
	  cx, cy, width, height, start_angle, delta_angle, sa, ea);
#endif
  use_gc (gc);
  fprintf (f, "%d %d %d %d %d %d %g a\n",
	   sa, ea,
	   -width, height, cx, cy, (double) (linewidth + 2 * bloat) / width);
}

static void
ps_fill_circle (hidGC gc, int cx, int cy, int radius)
{
  use_gc (gc);
  fprintf (f, "%d %d %d c\n", cx, cy, radius + (gc->erase ? -1 : 1) * bloat);
  if (gc->erase && !is_drill && drill_helper
      && radius >= 2 * MIN_PINORVIAHOLE)
    fprintf (f, "%d %d dh\n", cx, cy);
}

static void
ps_fill_polygon (hidGC gc, int n_coords, int *x, int *y)
{
  int i;
  char *op = "moveto";
  use_gc (gc);
  for (i = 0; i < n_coords; i++)
    {
      fprintf (f, "%d %d %s\n", x[i], y[i], op);
      op = "lineto";
    }
  fprintf (f, "fill\n");
}

static void
ps_fill_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  use_gc (gc);
  fprintf (f, "%d %d %d %d r\n", x1, y1, x2, y2);
}

static void
ps_calibrate (double xval, double yval)
{
  CRASH;
}

static void
ps_set_crosshair (int x, int y)
{
}

HID ps_hid = {
  sizeof (HID),
  "ps",
  "Postscript export.",
  0, 0, 1, 1, 0, 0,
  ps_get_export_options,
  ps_do_export,
  ps_parse_arguments,
  0 /* ps_invalidate_wh */ ,
  0 /* ps_invalidate_lr */ ,
  0 /* ps_invalidate_all */ ,
  ps_set_layer,
  ps_make_gc,
  ps_destroy_gc,
  ps_use_mask,
  ps_set_color,
  ps_set_line_cap,
  ps_set_line_width,
  ps_set_draw_xor,
  ps_set_draw_faded,
  ps_set_line_cap_angle,
  ps_draw_line,
  ps_draw_arc,
  ps_draw_rect,
  ps_fill_circle,
  ps_fill_polygon,
  ps_fill_rect,
  ps_calibrate,
  0 /* ps_shift_is_pressed */ ,
  0 /* ps_control_is_pressed */ ,
  0 /* ps_get_coords */ ,
  ps_set_crosshair,
  0 /* ps_add_timer */ ,
  0 /* ps_stop_timer */ ,
  0 /* ps_log */ ,
  0 /* ps_logv */ ,
  0 /* ps_confirm_dialog */ ,
  0 /* ps_report_dialog */ ,
  0 /* ps_prompt_for */ ,
  0 /* ps_attribute_dialog */ ,
  0 /* ps_show_item */ ,
  0 /* ps_beep */ ,
  0 /* ps_progress */
};

#include "dolists.h"

void
hid_ps_init ()
{
  apply_default_hid (&ps_hid, 0);
  hid_register_hid (&ps_hid);

  hid_eps_init ();
#include "ps_lists.h"
}
