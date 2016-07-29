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
#include "pcb-printf.h"

#include "hid.h"
#include "hid_draw.h"
#include "../hidint.h"
#include "hid/common/hidnogui.h"
#include "hid/common/draw_helpers.h"
#include "../ps/ps.h"
#include "../../print.h"
#include "hid/common/hidinit.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented PS function %s.\n", __FUNCTION__); abort()

static int ps_set_layer (const char *name, int group, int empty);
static void use_gc (hidGC gc);

typedef struct hid_gc_struct
{
  HID *me_pointer;
  EndCapStyle cap;
  Coord width;
  unsigned char r, g, b;
  int erase;
  int faded;
} hid_gc_struct;

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
  Coord Width, Height;
  Coord MarginX, MarginY;
} MediaType;

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

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --psfile <string>
Name of the postscript output file. Can contain a path.
@end ftable
%end-doc
*/
  {N_("psfile"), N_("Postscript output file"),
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_psfile 0

/* %start-doc options "91 Postscript Export"
@ftable @code
@cindex drill-helper
@item --drill-helper
Print a centering target in large drill holes.
@end ftable
%end-doc
*/
  {N_("drill-helper"), N_("Print a centering target in large drill holes"),
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_drillhelper 1

/* %start-doc options "91 Postscript Export"
@ftable @code
@cindex align-marks
@item --align-marks
Print alignment marks on each sheet. This is meant to ease alignment during exposure.
@end ftable
%end-doc
*/
  {N_("align-marks"), N_("Print alignment marks on each sheet"),
   HID_Boolean, 0, 0, {1, 0, 0}, 0, 0},
#define HA_alignmarks 2

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --outline
Print the contents of the outline layer on each sheet.
@end ftable
%end-doc
*/
  {N_("outline"), N_("Print outline on each sheet"),
   HID_Boolean, 0, 0, {1, 0, 0}, 0, 0},
#define HA_outline 3
/* %start-doc options "91 Postscript Export"
@ftable @code
@item --mirror
Print mirror image.
@end ftable
%end-doc
*/
  {N_("mirror"), N_("Print mirror image of every page"),
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_mirror 4

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --fill-page
Scale output to make the board fit the page.
@end ftable
%end-doc
*/
  {N_("fill-page"), N_("Scale board to fill page"),
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_fillpage 5

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --auto-mirror
Print mirror image of appropriate layers.
@end ftable
%end-doc
*/
  {N_("auto-mirror"), N_("Print mirror image of appropriate layers"),
   HID_Boolean, 0, 0, {1, 0, 0}, 0, 0},
#define HA_automirror 6

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --ps-color
Postscript output in color.
@end ftable
%end-doc
*/
  {N_("ps-color"), N_("Prints in color"),
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_color 7

/* %start-doc options "91 Postscript Export"
@ftable @code
@cindex ps-bloat
@item --ps-bloat <num>
Amount to add to trace/pad/pin edges.
@end ftable
%end-doc
*/
  {N_("ps-bloat"), N_("Amount to add to trace/pad/pin edges"),
   HID_Coord, -MIL_TO_COORD (100), MIL_TO_COORD (100), {0, 0, 0}, 0, 0},
#define HA_psbloat 8

/* %start-doc options "91 Postscript Export"
@ftable @code
@cindex ps-invert
@item --ps-invert
Draw objects as white-on-black.
@end ftable
%end-doc
*/
 {N_("ps-invert"), N_("Draw objects as white-on-black"),
   HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_psinvert 9

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --media <media-name>
Size of the media, the postscript is fitted to. The parameter
@code{<media-name>} can be any of the standard names for paper size: @samp{A0}
to @samp{A10}, @samp{B0} to @samp{B10}, @samp{Letter}, @samp{11x17},
@samp{Ledger}, @samp{Legal}, @samp{Executive}, @samp{A-Size}, @samp{B-size},
@samp{C-Size}, @samp{D-size}, @samp{E-size}, @samp{US-Business_Card},
@samp{Intl-Business_Card}.
@end ftable
%end-doc
*/
  {N_("media"), N_("Media type"),
   HID_Enum, 0, 0, {22, 0, 0}, medias, 0},
#define HA_media 10

/* %start-doc options "91 Postscript Export"
@ftable @code
@cindex psfade
@item --psfade <num>
Fade amount for assembly drawings (0.0=missing, 1.0=solid).
@end ftable
%end-doc
*/
  {N_("psfade"),
   N_("Fade amount for assembly drawings (0.0=missing, 1.0=solid)"),
   HID_Real, 0, 1, {0, 0, 0.40}, 0, 0},
#define HA_psfade 11

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --scale <num>
Scale value to compensate for printer sizing errors (1.0 = full scale).
@end ftable
%end-doc
*/
  {N_("scale"),
   N_("Scale value to compensate for printer sizing errors (1.0 = full scale)"),
   HID_Real, 0.01, 4, {0, 0, 1.00}, 0, 0},
#define HA_scale 12

/* %start-doc options "91 Postscript Export"
@ftable @code
@cindex multi-file
@item --multi-file
Produce multiple files, one per page, instead of a single multi page file.
@end ftable
%end-doc
*/
  {N_("multi-file"),
   N_("Produce multiple files, one per page, instead of a single file"),
   HID_Boolean, 0, 0, {0, 0, 0.40}, 0, 0},
#define HA_multifile 13

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --xcalib <num>
Paper width. Used for x-Axis calibration.
@end ftable
%end-doc
*/
  {N_("xcalib"), N_("Paper width. Used for x-Axis calibration"),
   HID_Real, 0, 0, {0, 0, 1.0}, 0, 0},
#define HA_xcalib 14

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --ycalib <num>
Paper height. Used for y-Axis calibration.
@end ftable
%end-doc
*/
  {N_("ycalib"), N_("Paper height. Used for y-Axis calibration"),
   HID_Real, 0, 0, {0, 0, 1.0}, 0, 0},
#define HA_ycalib 15

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --drill-copper
Draw drill holes in pins / vias, instead of leaving solid copper.
@end ftable
%end-doc
*/
  {N_("drill-copper"),
   N_("Draw drill holes in pins / vias, instead of leaving solid copper"),
   HID_Boolean, 0, 0, {1, 0, 0}, 0, 0},
#define HA_drillcopper 16

/* %start-doc options "91 Postscript Export"
@ftable @code
@cindex show-legend
@item --show-legend
Print file name and scale on printout.
@end ftable
%end-doc
*/
  {N_("show-legend"), N_("Print file name and scale on printout"),
   HID_Boolean, 0, 0, {1, 0, 0}, 0, 0},
#define HA_legend 17
};

#define NUM_OPTIONS (sizeof(ps_attribute_list)/sizeof(ps_attribute_list[0]))

REGISTER_ATTRIBUTES (ps_attribute_list)

/* All file-scope data is in global struct */
static struct {
  double calibration_x, calibration_y;

  FILE *f;
  int pagecount;
  Coord linewidth;
  bool print_group[MAX_GROUP];
  bool print_layer[MAX_ALL_LAYER];
  double fade_ratio;
  bool multi_file;
  Coord media_width, media_height, ps_width, ps_height;

  const char *filename;
  bool drill_helper;
  bool align_marks;
  bool outline;
  bool mirror;
  bool fillpage;
  bool automirror;
  bool incolor;
  bool doing_toc;
  Coord bloat;
  bool invert;
  int media_idx;
  bool drillcopper;
  bool legend;

  LayerType *outline_layer;

  double scale_factor;

  BoxType region;

  HID_Attr_Val ps_values[NUM_OPTIONS];

  bool is_mask;
  bool is_drill;
  bool is_assy;
  bool is_copper;
  bool is_paste;
} global;

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
layer_stack_sort (const void *va, const void *vb)
{
  int a_layer = *(int *) va;
  int b_layer = *(int *) vb;
  int a_group = GetLayerGroupNumberByNumber (a_layer);
  int b_group = GetLayerGroupNumberByNumber (b_layer);

  if (b_group != a_group)
    return b_group - a_group;

  return b_layer - a_layer;
}

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
  pcb_fprintf (f, "%%%%DocumentMedia: %s %mi %mi 0 \"\" \"\"\n",
               media_data[global.media_idx].name,
               72 * media_data[global.media_idx].Width,
               72 * media_data[global.media_idx].Height);
  pcb_fprintf (f, "%%%%DocumentPaperSizes: %s\n", media_data[global.media_idx].name);

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
  fprintf (f, "%%%%Pages: %d\n", global.pagecount);

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

  if (!global.multi_file)
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
  CLEAR_FLAG(CHECKPLANESFLAG, PCB);

  global.f = the_file;
  global.drill_helper = options[HA_drillhelper].int_value;
  global.align_marks  = options[HA_alignmarks].int_value;
  global.outline      = options[HA_outline].int_value;
  global.mirror       = options[HA_mirror].int_value;
  global.fillpage     = options[HA_fillpage].int_value;
  global.automirror   = options[HA_automirror].int_value;
  global.incolor      = options[HA_color].int_value;
  global.bloat        = options[HA_psbloat].coord_value;
  global.invert       = options[HA_psinvert].int_value;
  global.fade_ratio   = CLAMP (options[HA_psfade].real_value, 0, 1);
  global.media_idx    = options[HA_media].int_value;
  global.media_width  = media_data[global.media_idx].Width;
  global.media_height = media_data[global.media_idx].Height;
  global.ps_width     = global.media_width
                        - 2.0 * media_data[global.media_idx].MarginX;
  global.ps_height    = global.media_height
                        - 2.0 * media_data[global.media_idx].MarginY;
  global.scale_factor = options[HA_scale].real_value;
  global.calibration_x = options[HA_xcalib].real_value;
  global.calibration_y = options[HA_ycalib].real_value;
  global.drillcopper  = options[HA_drillcopper].int_value;
  global.legend       = options[HA_legend].int_value;

  if (the_file)
    ps_start_file (the_file);

  if (global.fillpage)
    {
      double zx, zy;
      if (PCB->MaxWidth > PCB->MaxHeight)
	{
	  zx = global.ps_height / PCB->MaxWidth;
	  zy = global.ps_width  / PCB->MaxHeight;
	}
      else
	{
	  zx = global.ps_height / PCB->MaxHeight;
	  zy = global.ps_width  / PCB->MaxWidth;
	}
      global.scale_factor *= MIN (zx, zy);
    }

  memset (global.print_group, 0, sizeof (global.print_group));
  memset (global.print_layer, 0, sizeof (global.print_layer));

  global.outline_layer = NULL;

  for (i = 0; i < max_copper_layer; i++)
    {
      LayerType *layer = PCB->Data->Layer + i;
      if (layer->LineN || layer->TextN || layer->ArcN || layer->PolygonN)
	global.print_group[GetLayerGroupNumberByNumber (i)] = 1;

      if (strcmp (layer->Name, "outline") == 0 ||
	  strcmp (layer->Name, "route") == 0)
	{
	  global.outline_layer = layer;
	}
    }
  global.print_group[GetLayerGroupNumberBySide (BOTTOM_SIDE)] = 1;
  global.print_group[GetLayerGroupNumberBySide (TOP_SIDE)] = 1;
  for (i = 0; i < max_copper_layer; i++)
    if (global.print_group[GetLayerGroupNumberByNumber (i)])
      global.print_layer[i] = 1;

  memcpy (saved_layer_stack, LayerStack, sizeof (LayerStack));
  qsort (LayerStack, max_copper_layer, sizeof (LayerStack[0]), layer_stack_sort);

  global.linewidth = -1;
  /* reset static vars */
  ps_set_layer (NULL, 0, -1);
  use_gc (NULL);

  global.region.X1 = 0;
  global.region.Y1 = 0;
  global.region.X2 = PCB->MaxWidth;
  global.region.Y2 = PCB->MaxHeight;

  if (!global.multi_file)
    {
      /* %%Page DSC requires both a label and an ordinal */
      fprintf (the_file, "%%%%Page: TableOfContents 1\n");
      fprintf (the_file, "/Times-Roman findfont 14 scalefont setfont\n");
      fprintf (the_file, "/rightshow { /s exch def s stringwidth pop -1 mul 0 rmoveto s show } def\n");
      fprintf (the_file, "/y 72 9 mul def /toc { 100 y moveto show /y y 24 sub def } bind def\n");
      fprintf (the_file, "/tocp { /y y 0 sub def 90 y moveto rightshow } bind def\n");

      global.doing_toc = 1;
      global.pagecount = 1;  /* 'pagecount' is modified by hid_expose_callback() call */
      hid_expose_callback (&ps_hid, &global.region, 0);
    }

  global.pagecount = 1; /* Reset 'pagecount' if single file */
  global.doing_toc = 0;
  ps_set_layer (NULL, 0, -1);  /* reset static vars */
  hid_expose_callback (&ps_hid, &global.region, 0);

  if (the_file)
    fprintf (the_file, "showpage\n");

  memcpy (LayerStack, saved_layer_stack, sizeof (LayerStack));
  PCB->Flags = save_thindraw;
}

static void
ps_do_export (HID_Attr_Val * options)
{
  FILE *fh;
  int save_ons[MAX_ALL_LAYER];
  int i;

  if (!options)
    {
      ps_get_export_options (0);
      for (i = 0; i < NUM_OPTIONS; i++)
	global.ps_values[i] = ps_attribute_list[i].default_val;
      options = global.ps_values;
    }

  global.filename = options[HA_psfile].str_value;
  if (!global.filename)
    global.filename = "pcb-out.ps";

  global.multi_file = options[HA_multifile].int_value;

  if (global.multi_file)
    fh = 0;
  else
    {
      fh = psopen (global.filename, "toc");
      if (!fh)
	{
	  perror (global.filename);
	  return;
	}
    }

  hid_save_and_show_layer_ons (save_ons);
  ps_hid_export_to_file (fh, options);
  hid_restore_layer_ons (save_ons);

  global.multi_file = 0;
  if (fh)
    {
      ps_end_file (fh);
      fclose (fh);
    }
}

static void
ps_parse_arguments (int *argc, char ***argv)
{
  hid_register_attributes (ps_attribute_list, NUM_OPTIONS);
  hid_parse_command_line (argc, argv);
}

static void
corner (FILE *fh, Coord x, Coord y, int dx, int dy)
{
  Coord len   = MIL_TO_COORD (2000);
  Coord len2  = MIL_TO_COORD (200);
  Coord thick = 0;
  /*
   * Originally 'thick' used thicker lines.  Currently is uses
   * Postscript's "device thin" line - i.e. zero width means one
   * device pixel.  The code remains in case you want to make them
   * thicker - it needs to offset everything so that the *edge* of the
   * thick line lines up with the edge of the board, not the *center*
   * of the thick line.
   */

  pcb_fprintf (fh, "gsave %mi setlinewidth %mi %mi translate %d %d scale\n",
               thick * 2, x, y, dx, dy);
  pcb_fprintf (fh, "%mi %mi moveto %mi %mi %mi 0 90 arc %mi %mi lineto\n",
               len, thick, thick, thick, len2 + thick, thick, len);
  if (dx < 0 && dy < 0)
    pcb_fprintf (fh, "%mi %mi moveto 0 %mi rlineto\n", len2 * 2 + thick, thick, -len2);
  fprintf (fh, "stroke grestore\n");
}

static int
ps_set_layer (const char *name, int group, int empty)
{
  static int lastgroup = -1;
  time_t currenttime;
  int idx = (group >= 0 && group < max_group)
            ? PCB->LayerGroups.Entries[group][0]
            : group;
  if (name == 0)
    name = PCB->Data->Layer[idx].Name;

  if (empty == -1)
    lastgroup = -1;
  if (empty)
    return 0;

  if (idx >= 0 && idx < max_copper_layer && !global.print_layer[idx])
    return 0;

  if (strcmp (name, "invisible") == 0)
    return 0;

  global.is_drill = (SL_TYPE (idx) == SL_PDRILL || SL_TYPE (idx) == SL_UDRILL);
  global.is_mask  = (SL_TYPE (idx) == SL_MASK);
  global.is_assy  = (SL_TYPE (idx) == SL_ASSY);
  global.is_copper = (SL_TYPE (idx) == 0);
  global.is_paste  = (SL_TYPE (idx) == SL_PASTE);
#if 0
  printf ("Layer %s group %d drill %d mask %d\n", name, group, global.is_drill,
	  global.is_mask);
#endif

  if (global.doing_toc)
    {
      if (group < 0 || group != lastgroup)
	{
          if (global.pagecount == 1)
            {
              currenttime = time (NULL);
              fprintf (global.f, "30 30 moveto (%s) show\n", PCB->Filename);

              fprintf (global.f, "(%d.) tocp\n", global.pagecount);
              fprintf (global.f, "(Table of Contents \\(This Page\\)) toc\n" );

              fprintf (global.f, "(Created on %s) toc\n", asctime (localtime (&currenttime)));
              fprintf (global.f, "( ) tocp\n" );
            }

	  global.pagecount++;
	  lastgroup = group;
	  fprintf (global.f, "(%d.) tocp\n", global.pagecount);
	}
      fprintf (global.f, "(%s) toc\n", name);
      return 0;
    }

  if (group < 0 || group != lastgroup)
    {
      double boffset;
      int mirror_this = 0;
      lastgroup = group;

      if (global.pagecount != 0)
	{
	  pcb_fprintf (global.f, "showpage\n");
	}
      global.pagecount++;
      if (global.multi_file)
	{
	  if (global.f)
            {
              ps_end_file (global.f);
              fclose (global.f);
            }
	  global.f = psopen (global.filename, layer_type_to_file_name_ex (idx, FNS_fixed, name));
	  if (!global.f)
	  {
	    perror (global.filename);
	    return 0;
	  }

	  ps_start_file (global.f);
	}

      /*
       * %%Page DSC comment marks the beginning of the PostScript
       * language instructions that describe a particular
       * page. %%Page: requires two arguments: a page label and a
       * sequential page number. The label may be anything, but the
       * ordinal page number must reflect the position of that page in
       * the body of the PostScript file and must start with 1, not 0.
       */
      fprintf (global.f, "%%%%Page: %s %d\n", layer_type_to_file_name_ex (idx, FNS_fixed, name), global.pagecount);

      if (global.mirror)
	mirror_this = !mirror_this;
      if (global.automirror
	  &&
	  ((idx >= 0 && group == GetLayerGroupNumberBySide (BOTTOM_SIDE))
	   || (idx < 0 && SL_SIDE (idx) == SL_BOTTOM_SIDE)))
	mirror_this = !mirror_this;

      fprintf (global.f, "/Helvetica findfont 10 scalefont setfont\n");
      if (global.legend)
	{
	  fprintf (global.f, "30 30 moveto (%s) show\n", PCB->Filename);
	  if (PCB->Name)
	    fprintf (global.f, "30 41 moveto (%s, %s) show\n",
		     PCB->Name, layer_type_to_file_name_ex (idx, FNS_fixed, name));
	  else
	    fprintf (global.f, "30 41 moveto (%s) show\n",
		     layer_type_to_file_name_ex (idx, FNS_fixed, name));
	  if (mirror_this)
	    fprintf (global.f, "( \\(mirrored\\)) show\n");

	  if (global.fillpage)
	    fprintf (global.f, "(, not to scale) show\n");
	  else
	    fprintf (global.f, "(, scale = 1:%.3f) show\n", global.scale_factor);
	}
      fprintf (global.f, "newpath\n");

      pcb_fprintf (global.f, "72 72 scale %mi %mi translate\n",
                   global.media_width / 2, global.media_height / 2);

      boffset = global.media_height / 2;
      if (PCB->MaxWidth > PCB->MaxHeight)
	{
	  fprintf (global.f, "90 rotate\n");
	  boffset = global.media_width / 2;
	  fprintf (global.f, "%g %g scale %% calibration\n", global.calibration_y, global.calibration_x);
	}
      else
	fprintf (global.f, "%g %g scale %% calibration\n", global.calibration_x, global.calibration_y);

      if (mirror_this)
	fprintf (global.f, "1 -1 scale\n");

      fprintf (global.f, "%g dup neg scale\n",
               (SL_TYPE (idx) == SL_FAB) ? 1.0 : global.scale_factor);
      pcb_fprintf (global.f, "%mi %mi translate\n", -PCB->MaxWidth / 2, -PCB->MaxHeight / 2);

      /* Keep the drill list from falling off the left edge of the paper,
       * even if it means some of the board falls off the right edge.
       * If users don't want to make smaller boards, or use fewer drill
       * sizes, they can always ignore this sheet. */
      if (SL_TYPE (idx) == SL_FAB) {
        Coord natural = boffset - MIL_TO_COORD(500) - PCB->MaxHeight / 2;
	Coord needed  = PrintFab_overhang ();
        pcb_fprintf (global.f, "%% PrintFab overhang natural %mi, needed %mi\n", natural, needed);
	if (needed > natural)
	  pcb_fprintf (global.f, "0 %mi translate\n", needed - natural);
      }

      if (global.invert)
	{
	  fprintf (global.f, "/gray { 1 exch sub setgray } bind def\n");
	  fprintf (global.f,
                   "/rgb { 1 1 3 { pop 1 exch sub 3 1 roll } for setrgbcolor } bind def\n");
	}
      else
	{
	  fprintf (global.f, "/gray { setgray } bind def\n");
	  fprintf (global.f, "/rgb { setrgbcolor } bind def\n");
	}

      if ((global.outline && !global.outline_layer) || global.invert)
	{
	  pcb_fprintf (global.f,
                       "0 setgray 0 setlinewidth 0 0 moveto 0 "
                       "%mi lineto %mi %mi lineto %mi 0 lineto closepath %s\n",
                       PCB->MaxHeight, PCB->MaxWidth, PCB->MaxHeight, PCB->MaxWidth,
                       global.invert ? "fill" : "stroke");
	}

      if (global.align_marks)
	{
	  corner (global.f, 0, 0, -1, -1);
	  corner (global.f, PCB->MaxWidth, 0, 1, -1);
	  corner (global.f, PCB->MaxWidth, PCB->MaxHeight, 1, 1);
	  corner (global.f, 0, PCB->MaxHeight, -1, 1);
	}

      global.linewidth = -1;
      use_gc (NULL);  /* reset static vars */

      fprintf (global.f,
              "/ts 1 def\n"
              "/ty ts neg def /tx 0 def /Helvetica findfont ts scalefont setfont\n"
              "/t { moveto lineto stroke } bind def\n"
              "/dr { /y2 exch def /x2 exch def /y1 exch def /x1 exch def\n"
              "      x1 y1 moveto x1 y2 lineto x2 y2 lineto x2 y1 lineto closepath stroke } bind def\n"
              "/r { /y2 exch def /x2 exch def /y1 exch def /x1 exch def\n"
              "     x1 y1 moveto x1 y2 lineto x2 y2 lineto x2 y1 lineto closepath fill } bind def\n"
              "/c { 0 360 arc fill } bind def\n"
              "/a { gsave setlinewidth translate scale 0 0 1 5 3 roll arc stroke grestore} bind def\n");
      if (global.drill_helper)
	pcb_fprintf (global.f,
                    "/dh { gsave %mi setlinewidth 0 gray %mi 0 360 arc stroke grestore} bind def\n",
                    (Coord) MIN_PINORVIAHOLE, (Coord) (MIN_PINORVIAHOLE * 3 / 2));
    }
#if 0
  /* Try to outsmart ps2pdf's heuristics for page rotation, by putting
   * text on all pages -- even if that text is blank */
  if (SL_TYPE (idx) != SL_FAB)
    fprintf (global.f,
	     "gsave tx ty translate 1 -1 scale 0 0 moveto (Layer %s) show grestore newpath /ty ty ts sub def\n",
	     name);
  else
    fprintf (global.f, "gsave tx ty translate 1 -1 scale 0 0 moveto ( ) show grestore newpath /ty ty ts sub def\n");
#endif

  /* If we're printing a copper layer other than the outline layer,
     and we want to "print outlines", and we have an outline layer,
     print the outline layer on this layer also.  */
  if (global.outline &&
      global.is_copper &&
      global.outline_layer != NULL &&
      global.outline_layer != PCB->Data->Layer+idx &&
      strcmp (name, "outline") != 0 &&
      strcmp (name, "route") != 0
      )
    {
      DrawLayer (global.outline_layer, &global.region);
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
ps_use_mask (enum mask_mode mode)
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
  else if (global.incolor)
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
ps_set_line_width (hidGC gc, Coord width)
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
  static int lastcap = -1;
  static int lastcolor = -1;

  if (gc == NULL)
    {
      lastcap = lastcolor = -1;
      return;
    }
  if (gc->me_pointer != &ps_hid)
    {
      fprintf (stderr, "Fatal: GC from another HID passed to ps HID\n");
      abort ();
    }
  if (global.linewidth != gc->width)
    {
      pcb_fprintf (global.f, "%mi setlinewidth\n",
                   gc->width + (gc->erase ? -2 : 2) * global.bloat);
      global.linewidth = gc->width;
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
      fprintf (global.f, "%d setlinecap %d setlinejoin\n", c, c);
      lastcap = gc->cap;
    }
#define CBLEND(gc) (((gc->r)<<24)|((gc->g)<<16)|((gc->b)<<8)|(gc->faded))
  if (lastcolor != CBLEND (gc))
    {
      if (global.is_drill || global.is_mask)
	{
	  fprintf (global.f, "%d gray\n", gc->erase ? 0 : 1);
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
	      r = (1 - global.fade_ratio) * 255 + global.fade_ratio * r;
	      g = (1 - global.fade_ratio) * 255 + global.fade_ratio * g;
	      b = (1 - global.fade_ratio) * 255 + global.fade_ratio * b;
	    }
	  if (gc->r == gc->g && gc->g == gc->b)
	    fprintf (global.f, "%g gray\n", r / 255.0);
	  else
	    fprintf (global.f, "%g %g %g rgb\n", r / 255.0, g / 255.0, b / 255.0);
	  lastcolor = CBLEND (gc);
	}
    }
}

static void
ps_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  use_gc (gc);
  pcb_fprintf (global.f, "%mi %mi %mi %mi dr\n", x1, y1, x2, y2);
}

static void ps_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);
static void ps_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius);

static void
ps_draw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
#if 0
  /* If you're etching your own paste mask, this will reduce the
     amount of brass you need to etch by drawing outlines for large
     pads.  See also ps_fill_rect.  */
  if (is_paste && gc->width > 2500 && gc->cap == Square_Cap
      && (x1 == x2 || y1 == y2))
    {
      Coord t, w;
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
      Coord w = gc->width / 2;
      if (gc->cap == Square_Cap)
	ps_fill_rect (gc, x1 - w, y1 - w, x1 + w, y1 + w);
      else
	ps_fill_circle (gc, x1, y1, w);
      return;
    }
  use_gc (gc);
  pcb_fprintf (global.f, "%mi %mi %mi %mi t\n", x1, y1, x2, y2);
}

static void
ps_draw_arc (hidGC gc, Coord cx, Coord cy, Coord width, Coord height,
	     Angle start_angle, Angle delta_angle)
{
  Angle sa, ea;
  double linewidth;

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

  use_gc (gc);

  /* Other than pcb's screen renderer, PostScript (at least GhostScript)
     internally limits linewidth to (diameter / 2), so no drawing of a dot with
     a circle of zero diameter. Compensate for this by making diameter larger
     and line width thinner.

     Handling of this case currently has only circles in mind, no ellipses.
     The regular case works with ellipses, too. */
  if (width < global.linewidth)
    {
      Coord outer_radius;

      outer_radius = width + global.linewidth / 2 + global.bloat;
      width = height = outer_radius / 2;
      linewidth = 2.0;
    }
  else
    linewidth = (double) (global.linewidth + 2 * global.bloat) / (double) width;

  /* The line of PostScript written here is a bit odd; linewidth isn't
     absolute, but relative to circle diameter. This is neccessary for
     ellipses, which are drawn by streching (transforming) a circle. */
  pcb_fprintf (global.f, "%ma %ma %mi %mi %mi %mi %`g a\n",
               sa, ea, -width, height, cx, cy, linewidth);

}

static void
ps_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius)
{
  use_gc (gc);
  if (!gc->erase || !global.is_copper || global.drillcopper)
    {
      if (gc->erase && global.is_copper && global.drill_helper
	  && radius >= PCB->minDrill / 4)
	radius = PCB->minDrill / 4;
      pcb_fprintf (global.f, "%mi %mi %mi c\n",
                   cx, cy, radius + (gc->erase ? -1 : 1) * global.bloat);
    }
}

static void
ps_fill_polygon (hidGC gc, int n_coords, Coord *x, Coord *y)
{
  int i;
  char *op = "moveto";
  use_gc (gc);
  for (i = 0; i < n_coords; i++)
    {
      pcb_fprintf (global.f, "%mi %mi %s\n", x[i], y[i], op);
      op = "lineto";
    }
  fprintf (global.f, "fill\n");
}

static void
fill_polyarea (hidGC gc, POLYAREA * pa, const BoxType * clip_box)
{
  /* Ignore clip_box, just draw everything */

  VNODE *v;
  PLINE *pl;
  char *op;

  use_gc (gc);

  pl = pa->contours;

  do
    {
      v = pl->head.next;
      op = "moveto";
      do
	{
	  pcb_fprintf (global.f, "%mi %mi %s\n", v->point[0], v->point[1], op);
	  op = "lineto";
	}
      while ((v = v->next) != pl->head.next);
    }
  while ((pl = pl->next) != NULL);

  fprintf (global.f, "fill\n");
}

static void
ps_draw_pcb_polygon (hidGC gc, PolygonType * poly, const BoxType * clip_box)
{
  if (!poly->Clipped)
    return;
  fill_polyarea (gc, poly->Clipped, clip_box);
  if (TEST_FLAG (FULLPOLYFLAG, poly))
    {
      POLYAREA *pa;

      for (pa = poly->Clipped->f; pa != poly->Clipped; pa = pa->f)
        fill_polyarea (gc, pa, clip_box);
    }
}

static void
ps_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  use_gc (gc);
  if (x1 > x2)
    {
      Coord t = x1;
      x1 = x2;
      x2 = t;
    }
  if (y1 > y2)
    {
      Coord t = y1;
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
  pcb_fprintf (global.f, "%mi %mi %mi %mi r\n",
               x1 - global.bloat, y1 - global.bloat,
               x2 + global.bloat, y2 + global.bloat);
}

HID_Attribute ps_calib_attribute_list[] = {
  {N_("lprcommand"), N_("Command to print"),
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
  "(Please measure between the flat faces of ONE pair of semi-circles on)t\n",
  "(both X and Y in the indicated units. Enter these values as X and Y)t\n",
  "(respectively. One member of each pair must be one of the semicircles)t\n",
  "(in the lower left corner. Nominal lengths on X are 4 in, 15 cm and 7.5 in.)t\n",
  "(Nominal lengths on Y are 4 in, 20 cm and 10 in.)t\n",
  "()t\n",
  "(The large box is 10.25 by 7.75 inches and is not used for calibration.)t\n",  "\n",
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
      if (guess (xval, 4, &global.calibration_x))
	if (guess (xval, 15, &global.calibration_x))
	  if (guess (xval, 7.5, &global.calibration_x))
	    {
	      if (xval < 2)
		ps_attribute_list[HA_xcalib].default_val.real_value =
		  global.calibration_x = xval;
	      else
		Message(_("X value of %g is too far off.\n"
			"Expecting it near: %.1f, %.1f, %.1f, %.1f\n"),
			xval, 1.0, 4.0, 15.0, 7.5);
	    }
      if (guess (yval, 4, &global.calibration_y))
	if (guess (yval, 20, &global.calibration_y))
	  if (guess (yval, 10, &global.calibration_y))
	    {
	      if (yval < 2)
		ps_attribute_list[HA_ycalib].default_val.real_value =
		  global.calibration_y = yval;
	      else
		Message(_("Y value of %g is too far off.\n"
			"Expecting it near: %.1f, %.1f, %.1f, %.1f\n"),
			yval, 1.0, 4.0, 20.0, 10.0);
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
      const char *cmd = vals[0].str_value;
      while (*cmd == ' ' || *cmd == '|')
	cmd ++;
      ps_cal_file = popen (cmd, "w");
      used_popen = 1;
    }
  else
    ps_cal_file = fopen (vals[0].str_value, "w");

  for (c=0; calib_lines[c]; c++)
    fputs(calib_lines[c], ps_cal_file);

  fprintf (ps_cal_file, "4 in 0.5 (Y in nom=4) cbar\n");
  fprintf (ps_cal_file, "20 cm 1.5 (Y cm nom=20) cbar\n");
  fprintf (ps_cal_file, "10 in 2.5 (Y in nom=10) cbar\n");
  fprintf (ps_cal_file, "-90 rotate\n");
  fprintf (ps_cal_file, "4 in -0.5 (X in nom=4) cbar\n");
  fprintf (ps_cal_file, "15 cm -1.5 (X cm nom=15) cbar\n");
  fprintf (ps_cal_file, "7.5 in -2.5 (X in nom=7.5) cbar\n");
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
static HID_DRAW ps_graphics;

void ps_ps_init (HID *hid)
{
  hid->get_export_options = ps_get_export_options;
  hid->do_export          = ps_do_export;
  hid->parse_arguments    = ps_parse_arguments;
  hid->set_layer          = ps_set_layer;
  hid->calibrate          = ps_calibrate;
  hid->set_crosshair      = ps_set_crosshair;
}

void ps_ps_graphics_init (HID_DRAW *graphics)
{
  graphics->make_gc            = ps_make_gc;
  graphics->destroy_gc         = ps_destroy_gc;
  graphics->use_mask           = ps_use_mask;
  graphics->set_color          = ps_set_color;
  graphics->set_line_cap       = ps_set_line_cap;
  graphics->set_line_width     = ps_set_line_width;
  graphics->set_draw_xor       = ps_set_draw_xor;
  graphics->set_draw_faded     = ps_set_draw_faded;
  graphics->draw_line          = ps_draw_line;
  graphics->draw_arc           = ps_draw_arc;
  graphics->draw_rect          = ps_draw_rect;
  graphics->fill_circle        = ps_fill_circle;
  graphics->fill_polygon       = ps_fill_polygon;
  graphics->fill_rect          = ps_fill_rect;

  graphics->draw_pcb_polygon   = ps_draw_pcb_polygon;
}

void
hid_ps_init ()
{
  memset (&ps_hid, 0, sizeof (HID));
  memset (&ps_graphics, 0, sizeof (HID_DRAW));

  common_nogui_init (&ps_hid);
  common_draw_helpers_init (&ps_graphics);
  ps_ps_init (&ps_hid);
  ps_ps_graphics_init (&ps_graphics);

  ps_hid.struct_size        = sizeof (HID);
  ps_hid.name               = "ps";
  ps_hid.description        = N_("Postscript export");
  ps_hid.exporter           = 1;
  ps_hid.poly_before        = 1;

  ps_hid.graphics           = &ps_graphics;

  hid_register_hid (&ps_hid);

  hid_eps_init ();
#include "ps_lists.h"
}
