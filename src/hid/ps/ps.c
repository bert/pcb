/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h> /* not used */
#include <stdlib.h>
#include <string.h>
#include <assert.h> /* not used */
#include <time.h>

#include "global.h"
#include "data.h"
#include "misc.h"
#include "error.h"
#include "draw.h"

#include "hid.h"
#include "../hidint.h"
#include "hid/common/hidnogui.h"
#include "hid/common/draw_helpers.h"
#include "../ps/ps.h"
#include "../../print.h"
#include "hid/common/hidinit.h"

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

static double calibration_x = 1.0, calibration_y = 1.0;

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

#define MARGINX MIL_TO_COORD(500)
#define MARGINY MIL_TO_COORD(500)

static MediaType media_data[] = {
  {"A0", MM_TO_COORD(841), MM_TO_COORD(1189), MARGINX, MARGINY},
  {"A1", MM_TO_COORD(594), MM_TO_COORD(841), MARGINX, MARGINY},
  {"A2", MM_TO_COORD(420), MM_TO_COORD(594), MARGINX, MARGINY},
  {"A3", MM_TO_COORD(297), MM_TO_COORD(420), MARGINX, MARGINY},
  {"A4", MM_TO_COORD(210), MM_TO_COORD(297), MARGINX, MARGINY},
  {"A5", MM_TO_COORD(148), MM_TO_COORD(210), MARGINX, MARGINY},
  {"A6", MM_TO_COORD(105), MM_TO_COORD(148), MARGINX, MARGINY},
  {"A7", MM_TO_COORD(74), MM_TO_COORD(105), MARGINX, MARGINY},
  {"A8", MM_TO_COORD(52), MM_TO_COORD(74), MARGINX, MARGINY},
  {"A9", MM_TO_COORD(37), MM_TO_COORD(52), MARGINX, MARGINY},
  {"A10", MM_TO_COORD(26), MM_TO_COORD(37), MARGINX, MARGINY},
  {"B0", MM_TO_COORD(1000), MM_TO_COORD(1414), MARGINX, MARGINY},
  {"B1", MM_TO_COORD(707), MM_TO_COORD(1000), MARGINX, MARGINY},
  {"B2", MM_TO_COORD(500), MM_TO_COORD(707), MARGINX, MARGINY},
  {"B3", MM_TO_COORD(353), MM_TO_COORD(500), MARGINX, MARGINY},
  {"B4", MM_TO_COORD(250), MM_TO_COORD(353), MARGINX, MARGINY},
  {"B5", MM_TO_COORD(176), MM_TO_COORD(250), MARGINX, MARGINY},
  {"B6", MM_TO_COORD(125), MM_TO_COORD(176), MARGINX, MARGINY},
  {"B7", MM_TO_COORD(88), MM_TO_COORD(125), MARGINX, MARGINY},
  {"B8", MM_TO_COORD(62), MM_TO_COORD(88), MARGINX, MARGINY},
  {"B9", MM_TO_COORD(44), MM_TO_COORD(62), MARGINX, MARGINY},
  {"B10", MM_TO_COORD(31), MM_TO_COORD(44), MARGINX, MARGINY},
  {"Letter", INCH_TO_COORD(8.5), INCH_TO_COORD(11), MARGINX, MARGINY},
  {"11x17", INCH_TO_COORD(11), INCH_TO_COORD(17), MARGINX, MARGINY},
  {"Ledger", INCH_TO_COORD(17), INCH_TO_COORD(11), MARGINX, MARGINY},
  {"Legal", INCH_TO_COORD(8.5), INCH_TO_COORD(14), MARGINX, MARGINY},
  {"Executive", INCH_TO_COORD(7.5), INCH_TO_COORD(10), MARGINX, MARGINY},
  {"A-size",  INCH_TO_COORD(8.5), INCH_TO_COORD(11), MARGINX, MARGINY},
  {"B-size", INCH_TO_COORD(11), INCH_TO_COORD(17), MARGINX, MARGINY},
  {"C-size", INCH_TO_COORD(17), INCH_TO_COORD(22), MARGINX, MARGINY},
  {"D-size", INCH_TO_COORD(22), INCH_TO_COORD(34), MARGINX, MARGINY},
  {"E-size", INCH_TO_COORD(34), INCH_TO_COORD(44), MARGINX, MARGINY},
  {"US-Business_Card", INCH_TO_COORD(3.5), INCH_TO_COORD(2.0), 0, 0},
  {"Intl-Business_Card", INCH_TO_COORD(3.375), INCH_TO_COORD(2.125), 0, 0}
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
   HID_Integer, -10000, 10000, {0, 0, 0}, 0, 0},
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
  {"xcalib", "X-Axis calibration (paper width).",
   HID_Real, 0, 0, {0, 0, 1.0}, 0, 0},
#define HA_xcalib 14
  {"ycalib", "Y-Axis calibration (paper height).",
   HID_Real, 0, 0, {0, 0, 1.0}, 0, 0},
#define HA_ycalib 15
  {"drill-copper", "Draw drill holes in pins / vias, instead of leaving solid copper.",
   HID_Boolean, 0, 0, {1, 0, 0}, 0, 0},
#define HA_drillcopper 16
  {"show-legend", "Print file name and scale on printout",
   HID_Boolean, 0, 0, {1, 0, 0}, 0, 0},
#define HA_legend 17
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
  if (l < max_copper_layer + 2 && l >= 0)
    return GetLayerGroupNumberByNumber (l);
  /* else something unique */
  return max_group + 3 + l;
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
static int drillcopper;
static int legend;

static LayerTypePtr outline_layer;

static double fill_zoom;
static double scale_value;

void
ps_start_file (FILE *f)
{
  time_t currenttime = time( NULL );

  fprintf (f, "%%!PS-Adobe-3.0\n");

  /* Document Structuring Conventions (DCS): */

  /* Start General Header Comments: */

  /*
   * %%Title DCS provides text title for the document that is useful
   * for printing banner pages.
   */
  fprintf (f, "%%%%Title: %s\n", PCB->Filename);

  /*
   * %%CreationDate DCS indicates the date and time the document was
   * created. Neither the date nor time need be in any standard
   * format. This comment is meant to be used purely for informational
   * purposes, such as printing on banner pages.
   */
  fprintf (f, "%%%%CreationDate: %s", asctime (localtime (&currenttime)));

  /*
   * %%Creator DCS indicates the document creator, usually the name of
   * the document composition software.
   */
  fprintf (f, "%%%%Creator: PCB release: %s " VERSION "\n", Progname);

  /*
   * %%Version DCS comment can be used to note the version and
   * revision number of a document or resource. A document manager may
   * wish to provide version control services, or allow substitution
   * of compatible versions/revisions of a resource or document.
   *
   * The format should be in the form of 'procname':
   *  <procname>::= < name> < version> < revision>
   *  < name> ::= < text>
   *  < version> ::= < real>
   *  < revision> ::= < uint>
   *
   * If a version numbering scheme is not used, these fields should
   * still be filled with a dummy value of 0.
   *
   * There is currently no code in PCB to manage this revision number.
   *
   */
  fprintf (f, "%%%%Version: (PCB %s " VERSION ") 0.0 0\n", Progname );


  /*
   * %%PageOrder DCS is intended to help document managers determine
   * the order of pages in the document file, which in turn enables a
   * document manager optionally to reorder the pages.  'Ascend'-The
   * pages are in ascending order for example, 1-2-3-4-5-6.
   */
  fprintf (f, "%%%%PageOrder: Ascend\n" );

  /*
   * %%Pages: < numpages> | (atend) < numpages> ::= < uint> (Total
   * %%number of pages)
   *
   * %%Pages DCS defines the number of virtual pages that a document
   * will image.  (atend) defers the count until the end of the file,
   * which is useful for dynamically generated contents.
   */
  fprintf (f, "%%%%Pages: (atend)\n" );

  /*
   * %%DocumentMedia: <name> <width> <height> <weight> <color> <type>
   *
   * Substitute 0 or "" for N/A.  Width and height are in points
   * (1/72").
   *
   * Media sizes are in PCB units
   */
  fprintf (f, "%%%%DocumentMedia: %s %g %g 0 \"\" \"\"\n",
	   media_data[media].name,
	   COORD_TO_INCH(media_data[media].Width) * 72.0,
	   COORD_TO_INCH(media_data[media].Height) * 72.0);
  fprintf (f, "%%%%DocumentPaperSizes: %s\n",
	   media_data[media].name);

  /* End General Header Comments. */

  /* General Body Comments go here. Currently there are none. */

  /*
   * %%EndComments DCS indicates an explicit end to the header
   * comments of the document.  All global DCS's must preceded
   * this.  A blank line gives an implicit end to the comments.
   */
  fprintf (f, "%%%%EndComments\n\n" );
}

static void
ps_end_file (FILE *f)
{
  /*
   * %%Trailer DCS must only occur once at the end of the document
   * script.  Any post-processing or cleanup should be contained in
   * the trailer of the document, which is anything that follows the
   * %%Trailer comment. Any of the document level structure comments
   * that were deferred by using the (atend) convention must be
   * mentioned in the trailer of the document after the %%Trailer
   * comment.
   */
  fprintf (f, "%%%%Trailer\n" );

  /*
   * %%Pages was deferred until the end of the document via the
   * (atend) mentioned, in the General Header section.
   */
  fprintf (f, "%%%%Pages: %d\n", pagecount );

  /*
   * %%EOF DCS signifies the end of the document. When the document
   * manager sees this comment, it issues an end-of-file signal to the
   * PostScript interpreter.  This is done so system-dependent file
   * endings, such as Control-D and end-of-file packets, do not
   * confuse the PostScript interpreter.
   */
  fprintf (f, "%%%%EOF\n" );
}

static FILE *
psopen (const char *base, const char *which)
{
  FILE *ps_open_file;
  char *buf, *suff, *buf2;

  if (!multi_file)
    return fopen (base, "w");

  buf = (char *)malloc (strlen (base) + strlen (which) + 5);

  suff = (char *)strrchr (base, '.');
  if (suff)
    {
      strcpy (buf, base);
      buf2 = strrchr (buf, '.');
      sprintf(buf2, ".%s.%s", which, suff+1);
    }
  else
    {
      sprintf(buf, "%s.%s.ps", base, which);
    }
  printf("PS: open %s\n", buf);
  ps_open_file = fopen(buf, "w");
  free (buf);
  return ps_open_file;
}

static BoxType region;

/* This is used by other HIDs that use a postscript format, like lpr
   or eps.  */
void
ps_hid_export_to_file (FILE * the_file, HID_Attr_Val * options)
{
  int i;
  static int saved_layer_stack[MAX_LAYER];
  FlagType save_thindraw;

  save_thindraw = PCB->Flags;
  CLEAR_FLAG(THINDRAWFLAG, PCB);
  CLEAR_FLAG(THINDRAWPOLYFLAG, PCB);

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
  calibration_x = options[HA_xcalib].real_value;
  calibration_y = options[HA_ycalib].real_value;
  drillcopper = options[HA_drillcopper].int_value;
  legend = options[HA_legend].int_value;

  if (f)
    ps_start_file (f);

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

  outline_layer = NULL;

  for (i = 0; i < max_copper_layer; i++)
    {
      LayerType *layer = PCB->Data->Layer + i;
      if (layer->LineN || layer->TextN || layer->ArcN || layer->PolygonN)
	print_group[GetLayerGroupNumberByNumber (i)] = 1;

      if (strcmp (layer->Name, "outline") == 0 ||
	  strcmp (layer->Name, "route") == 0)
	{
	  outline_layer = layer;
	}
    }
  print_group[GetLayerGroupNumberByNumber (solder_silk_layer)] = 1;
  print_group[GetLayerGroupNumberByNumber (component_silk_layer)] = 1;
  for (i = 0; i < max_copper_layer; i++)
    if (print_group[GetLayerGroupNumberByNumber (i)])
      print_layer[i] = 1;

  memcpy (saved_layer_stack, LayerStack, sizeof (LayerStack));
  qsort (LayerStack, max_copper_layer, sizeof (LayerStack[0]), layer_sort);

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
      fprintf (f, "%%%%Page: TableOfContents 1\n");  /* %%Page DSC requires both a label and an ordinal */
      fprintf (f, "/Times-Roman findfont 24 scalefont setfont\n");
      fprintf (f,
	       "/rightshow { /s exch def s stringwidth pop -1 mul 0 rmoveto s show } def\n");
      fprintf (f,
	       "/y 72 9 mul def /toc { 100 y moveto show /y y 24 sub def } bind def\n");
      fprintf (f, "/tocp { /y y 12 sub def 90 y moveto rightshow } bind def\n");

      doing_toc = 1;
      pagecount = 1;      /* 'pagecount' is modified by hid_expose_callback() call */
      hid_expose_callback (&ps_hid, &region, 0);
    }

  pagecount = 1; /* Reset 'pagecount' if single file */
  doing_toc = 0;
  lastgroup = -1;
  hid_expose_callback (&ps_hid, &region, 0);

  if (f)
    fprintf (f, "showpage\n");

  memcpy (LayerStack, saved_layer_stack, sizeof (LayerStack));
  PCB->Flags = save_thindraw;
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
    }

  hid_save_and_show_layer_ons (save_ons);
  ps_hid_export_to_file (f, options);
  hid_restore_layer_ons (save_ons);

  multi_file = 0;
  if (f)
    {
      ps_end_file (f);
      fclose (f);
    }
}

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
  int len = MIL_TO_COORD(2000);
  int len2 = MIL_TO_COORD(200);
#endif
  int thick = 0;
  /*
   * Originally 'thick' used thicker lines.  Currently is uses
   * Postscript's "device thin" line - i.e. zero width means one
   * device pixel.  The code remains in case you want to make them
   * thicker - it needs to offset everything so that the *edge* of the
   * thick line lines up with the edge of the board, not the *center*
   * of the thick line.
   */

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
static int is_copper;
static int is_paste;

static int
ps_set_layer (const char *name, int group, int empty)
{
  time_t currenttime;
  int idx = (group >= 0
	     && group <
	     max_group) ? PCB->LayerGroups.Entries[group][0] : group;
  if (name == 0)
    name = PCB->Data->Layer[idx].Name;

  if (empty)
    return 0;

  if (idx >= 0 && idx < max_copper_layer && !print_layer[idx])
    return 0;

  if (strcmp (name, "invisible") == 0)
    return 0;

  is_drill = (SL_TYPE (idx) == SL_PDRILL || SL_TYPE (idx) == SL_UDRILL);
  is_mask = (SL_TYPE (idx) == SL_MASK);
  is_assy = (SL_TYPE (idx) == SL_ASSY);
  is_copper = (SL_TYPE (idx) == 0);
  is_paste = (SL_TYPE (idx) == SL_PASTE);
#if 0
  printf ("Layer %s group %d drill %d mask %d\n", name, group, is_drill,
	  is_mask);
#endif

  if (doing_toc)
    {
      if (group < 0 || group != lastgroup)
	{
          if( 1 == pagecount )
            {
              currenttime = time( NULL );
              fprintf (f, "30 30 moveto (%s) show\n", PCB->Filename);

              fprintf (f, "(%d.) tocp\n", pagecount);
              fprintf (f, "(Table of Contents \\(This Page\\)) toc\n" );

              fprintf (f, "(Created on %s) toc\n", asctime (localtime (&currenttime)));
              fprintf (f, "( ) tocp\n" );
            }

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
      int mirror_this = 0;
      lastgroup = group;

      if (f && pagecount)
	{
	  fprintf (f, "showpage\n");
	}
      pagecount++;
      if (multi_file)
	{
	  if (f)
            {
              ps_end_file (f);
              fclose (f);
            }
	  f = psopen (filename, layer_type_to_file_name (idx));
	  if (!f)
	  {
	    perror(filename);
	    return 0;
	  }

	  ps_start_file (f);
	}

      /*
       * %%Page DSC comment marks the beginning of the PostScript
       * language instructions that describe a particular
       * page. %%Page: requires two arguments: a page label and a
       * sequential page number. The label may be anything, but the
       * ordinal page number must reflect the position of that page in
       * the body of the PostScript file and must start with 1, not 0.
       */
      fprintf (f, "%%%%Page: %s %d\n", layer_type_to_file_name(idx), pagecount);

      if (mirror)
	mirror_this = 1 - mirror_this;
      if (automirror
	  &&
	  ((idx >= 0 && group == GetLayerGroupNumberByNumber (solder_silk_layer))
	   || (idx < 0 && SL_SIDE (idx) == SL_BOTTOM_SIDE)))
	mirror_this = 1 - mirror_this;

      fprintf (f, "/Helvetica findfont 10 scalefont setfont\n");
      if (legend)
	{
	  fprintf (f, "30 30 moveto (%s) show\n", PCB->Filename);
	  if (PCB->Name)
	    fprintf (f, "30 41 moveto (%s, %s) show\n",
		     PCB->Name, layer_type_to_file_name (idx));
	  else
	    fprintf (f, "30 41 moveto (%s) show\n",
		     layer_type_to_file_name (idx));
	  if (mirror_this)
	    fprintf (f, "( \\(mirrored\\)) show\n");

	  if (fillpage)
	    fprintf (f, "(, not to scale) show\n");
	  else
	    fprintf (f, "(, scale = 1:%.3f) show\n", scale_value);
	}
      fprintf (f, "newpath\n");

      fprintf (f, "72 72 scale %g %g translate\n", 0.5*media_width, 0.5*media_height);

      boffset = 0.5*media_height;
      if (PCB->MaxWidth > PCB->MaxHeight)
	{
	  fprintf (f, "90 rotate\n");
	  boffset = 0.5*media_width;
	  fprintf (f, "%g %g scale %% calibration\n", calibration_y, calibration_x);
	}
      else
	fprintf (f, "%g %g scale %% calibration\n", calibration_x, calibration_y);

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
        int natural = (int) ((boffset - 0.5) * INCH_TO_COORD(1)) - PCB->MaxHeight / 2;
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

      if ((outline && !outline_layer) || invert)
	{
	  fprintf (f,
		   "0 setgray 0 setlinewidth 0 0 moveto 0 %d lineto %d %d lineto %d 0 lineto closepath %s\n",
		   PCB->MaxHeight, PCB->MaxWidth, PCB->MaxHeight, PCB->MaxWidth,
		   invert ? "fill" : "stroke");
	}

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
      fprintf (f, "/ty ts neg def /tx 0 def /Helvetica findfont ts scalefont setfont\n");
      fprintf (f, "/t { moveto lineto stroke } bind def\n");
      fprintf (f, "/dr { /y2 exch def /x2 exch def /y1 exch def /x1 exch def\n");
      fprintf (f, "      x1 y1 moveto x1 y2 lineto x2 y2 lineto x2 y1 lineto closepath stroke } bind def\n");
      fprintf (f, "/r { /y2 exch def /x2 exch def /y1 exch def /x1 exch def\n");
      fprintf (f, "     x1 y1 moveto x1 y2 lineto x2 y2 lineto x2 y1 lineto closepath fill } bind def\n");
      fprintf (f, "/c { 0 360 arc fill } bind def\n");
      fprintf (f, "/a { gsave setlinewidth translate scale 0 0 1 5 3 roll arc stroke grestore} bind def\n");
      if (drill_helper)
	fprintf (f,
		 "/dh { gsave %f setlinewidth 0 gray %f 0 360 arc stroke grestore} bind def\n",
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

  /* If we're printing a copper layer other than the outline layer,
     and we want to "print outlines", and we have an outline layer,
     print the outline layer on this layer also.  */
  if (outline
      && outline_layer
      && outline_layer != PCB->Data->Layer+idx
      && strcmp (name, "outline")
      && strcmp (name, "route"))
    {
      DrawLayer (outline_layer, &region);
    }

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
ps_set_draw_xor (hidGC gc, int xor_)
{
  ;
}

static void
ps_set_draw_faded (hidGC gc, int faded)
{
  gc->faded = faded;
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
  fprintf (f, "%d %d %d %d dr\n", x1, y1, x2, y2);
}

static void ps_fill_rect (hidGC gc, int x1, int y1, int x2, int y2);
static void ps_fill_circle (hidGC gc, int cx, int cy, int radius);

static void
ps_draw_line (hidGC gc, int x1, int y1, int x2, int y2)
{
#if 0
  /* If you're etching your own paste mask, this will reduce the
     amount of brass you need to etch by drawing outlines for large
     pads.  See also ps_fill_rect.  */
  if (is_paste && gc->width > 2500 && gc->cap == Square_Cap
      && (x1 == x2 || y1 == y2))
    {
      int t, w;
      if (x1 > x2)
	{ t = x1; x1 = x2; x2 = t; }
      if (y1 > y2)
	{ t = y1; y1 = y2; y2 = t; }
      w = gc->width/2;
      ps_fill_rect (gc, x1-w, y1-w, x2+w, y2+w);
      return;
    }
#endif
  if (x1 == x2 && y1 == y2)
    {
      int w = gc->width / 2;
      if (gc->cap == Square_Cap)
	ps_fill_rect (gc, x1 - w, y1 - w, x1 + w, y1 + w);
      else
	ps_fill_circle (gc, x1, y1, w);
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
  if (!gc->erase || !is_copper || drillcopper)
    {
      if (gc->erase && is_copper && drill_helper
	  && radius >= PCB->minDrill/4)
	radius = PCB->minDrill/4;
      fprintf (f, "%d %d %d c\n", cx, cy, radius + (gc->erase ? -1 : 1) * bloat);
    }
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
ps_fill_pcb_polygon (hidGC gc, PolygonType * poly, const BoxType * clip_box)
{
  /* Ignore clip_box, just draw everything */

  VNODE *v;
  PLINE *pl;
  char *op;

  use_gc (gc);

  pl = poly->Clipped->contours;

  do
    {
      v = pl->head.next;
      op = "moveto";
      do
	{
	  fprintf (f, "%d %d %s\n", v->point[0], v->point[1], op);
	  op = "lineto";
	}
      while ((v = v->next) != pl->head.next);
    }
  while ((pl = pl->next) != NULL);

  fprintf (f, "fill\n");
}

static void
ps_fill_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  use_gc (gc);
  if (x1 > x2)
    {
      int t = x1;
      x1 = x2;
      x2 = t;
    }
  if (y1 > y2)
    {
      int t = y1;
      y1 = y2;
      y2 = t;
    }
#if 0
  /* See comment in ps_draw_line.  */
  if (is_paste && (x2-x1)>2500 && (y2-y1)>2500)
    {
      linewidth = 1000;
      lastcap = Round_Cap;
      fprintf(f, "1000 setlinewidth 1 setlinecap 1 setlinejoin\n");
      fprintf(f, "%d %d moveto %d %d lineto %d %d lineto %d %d lineto closepath stroke\n",
	      x1+500-bloat, y1+500-bloat,
	      x1+500-bloat, y2-500+bloat,
	      x2-500+bloat, y2-500+bloat,
	      x2-500+bloat, y1+500-bloat);
      return;
    }
#endif
  fprintf (f, "%d %d %d %d r\n", x1-bloat, y1-bloat, x2+bloat, y2+bloat);
}

HID_Attribute ps_calib_attribute_list[] = {
  {"lprcommand", "Command to print",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
};

static const char * const calib_lines[] = {
  "%!PS-Adobe-3.0\n",
  "%%Title: Calibration Page\n",
  "%%PageOrder: Ascend\n",
  "%%Pages: 1\n",
  "%%EndComments\n",
  "\n",
  "%%Page: Calibrate 1\n",
  "72 72 scale\n",
  "\n",
  "0 setlinewidth\n",
  "0.375 0.375 moveto\n",
  "8.125 0.375 lineto\n",
  "8.125 10.625 lineto\n",
  "0.375 10.625 lineto\n",
  "closepath stroke\n",
  "\n",
  "0.5 0.5 translate\n",
  "0.001 setlinewidth\n",
  "\n",
  "/Times-Roman findfont 0.2 scalefont setfont\n",
  "\n",
  "/sign {\n",
  "    0 lt { -1 } { 1 } ifelse\n",
  "} def\n",
  "\n",
  "/cbar {\n",
  "    /units exch def\n",
  "    /x exch def\n",
  "    /y exch def  \n",
  "\n",
  "    /x x sign 0.5 mul def\n",
  "\n",
  "    0 setlinewidth\n",
  "    newpath x y 0.25 0 180 arc gsave 0.85 setgray fill grestore closepath stroke\n",
  "    newpath x 0 0.25 180 360 arc gsave 0.85 setgray fill grestore closepath stroke\n",
  "    0.001 setlinewidth\n",
  "\n",
  "    x 0 moveto\n",
  "    x y lineto\n",
  "%    -0.07 -0.2 rlineto 0.14 0 rmoveto -0.07 0.2 rlineto\n",
  "    x y lineto\n",
  "    -0.1 0 rlineto 0.2 0 rlineto\n",
  "    stroke\n",
  "    x 0 moveto\n",
  "%    -0.07 0.2 rlineto 0.14 0 rmoveto -0.07 -0.2 rlineto\n",
  "    x 0 moveto\n",
  "    -0.1 0 rlineto 0.2 0 rlineto\n",
  "     stroke\n",
  "\n",
  "    x 0.1 add\n",
  "    y 0.2 sub moveto\n",
  "    units show\n",
  "} bind def\n",
  "\n",
  "/y 9 def\n",
  "/t {\n",
  "    /str exch def\n",
  "    1.5 y moveto str show\n",
  "    /y y 0.25 sub def\n",
  "} bind def\n",
  "\n",
  "(Please measure ONE of the horizontal lines, in the units indicated for)t\n",
  "(that line, and enter that value as X.  Similarly, measure ONE of the)t\n",
  "(vertical lines and enter that value as Y.  Measurements should be)t\n",
  "(between the flat faces of the semicircles.)t\n",
  "()t\n",
  "(The large box is 10.25 by 7.75 inches)t\n",
  "\n",
  "/in { } bind def\n",
  "/cm { 2.54 div } bind def\n",
  "/mm { 25.4 div } bind def\n",
  "\n",
  0
};

static int
guess(double val, double close_to, double *calib)
{
  if (val >= close_to * 0.9
      && val <= close_to * 1.1)
    {
      *calib = close_to / val;
      return 0;
    }
  return 1;
}

void
ps_calibrate_1 (double xval, double yval, int use_command)
{
  HID_Attr_Val vals[3];
  FILE *ps_cal_file;
  int used_popen = 0, c;

  if (xval > 0 && yval > 0)
    {
      if (guess (xval, 4, &calibration_x))
	if (guess (xval, 15, &calibration_x))
	  if (guess (xval, 7.5, &calibration_x))
	    {
	      if (xval < 2)
		ps_attribute_list[HA_xcalib].default_val.real_value =
		  calibration_x = xval;
	      else
		Message("X value of %g is too far off.\n"
			"Expecting it near: 1.0, 4.0, 15.0, 7.5\n", xval);
	    }
      if (guess (yval, 4, &calibration_y))
	if (guess (yval, 20, &calibration_y))
	  if (guess (yval, 10, &calibration_y))
	    {
	      if (yval < 2)
		ps_attribute_list[HA_ycalib].default_val.real_value =
		  calibration_y = yval;
	      else
		Message("Y value of %g is too far off.\n"
			"Expecting it near: 1.0, 4.0, 20.0, 10.0\n", yval);
	    }
      return;
    }

  if (ps_calib_attribute_list[0].default_val.str_value == NULL)
    {
      ps_calib_attribute_list[0].default_val.str_value = strdup ("lpr");
    }

  if (gui->attribute_dialog (ps_calib_attribute_list, 1, vals, _("Print Calibration Page"), _("Generates a printer calibration page")))
    return;

  if (use_command || strchr (vals[0].str_value, '|'))
    {
      char *cmd = vals[0].str_value;
      while (*cmd == ' ' || *cmd == '|')
	cmd ++;
      ps_cal_file = popen (cmd, "w");
      used_popen = 1;
    }
  else
    ps_cal_file = fopen (vals[0].str_value, "w");

  for (c=0; calib_lines[c]; c++)
    fputs(calib_lines[c], ps_cal_file);

  fprintf (ps_cal_file, "4 in 0.5 (Y in) cbar\n");
  fprintf (ps_cal_file, "20 cm 1.5 (Y cm) cbar\n");
  fprintf (ps_cal_file, "10 in 2.5 (Y in) cbar\n");
  fprintf (ps_cal_file, "-90 rotate\n");
  fprintf (ps_cal_file, "4 in -0.5 (X in) cbar\n");
  fprintf (ps_cal_file, "15 cm -1.5 (X cm) cbar\n");
  fprintf (ps_cal_file, "7.5 in -2.5 (X in) cbar\n");

  fprintf (ps_cal_file, "showpage\n");

  fprintf (ps_cal_file, "%%%%EOF\n");

  if (used_popen)
    pclose (ps_cal_file);
  else
    fclose (ps_cal_file);
}

static void
ps_calibrate (double xval, double yval)
{
  ps_calibrate_1 (xval, yval, 0);
}

static void
ps_set_crosshair (int x, int y, int action)
{
}

#include "dolists.h"

HID ps_hid;

void ps_ps_init (HID *hid)
{
  hid->get_export_options = ps_get_export_options;
  hid->do_export          = ps_do_export;
  hid->parse_arguments    = ps_parse_arguments;
  hid->set_layer          = ps_set_layer;
  hid->make_gc            = ps_make_gc;
  hid->destroy_gc         = ps_destroy_gc;
  hid->use_mask           = ps_use_mask;
  hid->set_color          = ps_set_color;
  hid->set_line_cap       = ps_set_line_cap;
  hid->set_line_width     = ps_set_line_width;
  hid->set_draw_xor       = ps_set_draw_xor;
  hid->set_draw_faded     = ps_set_draw_faded;
  hid->draw_line          = ps_draw_line;
  hid->draw_arc           = ps_draw_arc;
  hid->draw_rect          = ps_draw_rect;
  hid->fill_circle        = ps_fill_circle;
  hid->fill_polygon       = ps_fill_polygon;
  hid->fill_pcb_polygon   = ps_fill_pcb_polygon;
  hid->fill_rect          = ps_fill_rect;
  hid->calibrate          = ps_calibrate;
  hid->set_crosshair      = ps_set_crosshair;
}

void
hid_ps_init ()
{
  memset (&ps_hid, 0, sizeof (HID));

  common_nogui_init (&ps_hid);
  common_draw_helpers_init (&ps_hid);
  ps_ps_init (&ps_hid);

  ps_hid.struct_size        = sizeof (HID);
  ps_hid.name               = "ps";
  ps_hid.description        = "Postscript export.";
  ps_hid.exporter           = 1;
  ps_hid.poly_before        = 1;

  hid_register_hid (&ps_hid);

  hid_eps_init ();
#include "ps_lists.h"
}
