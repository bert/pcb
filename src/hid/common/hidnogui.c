#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "hid.h"
#include "hid_draw.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* This is the "gui" that is installed at startup, and is used when
   there is no other real GUI to use.  For the most part, it just
   stops the application from (1) crashing randomly, and (2) doing
   gui-specific things when it shouldn't.  */

#define CRASH fprintf(stderr, "HID error: pcb called GUI function %s without having a GUI available.\n", __FUNCTION__); abort()

typedef struct hid_gc_struct
{
  int nothing_interesting_here;
} hid_gc_struct;

static HID_Attribute *
nogui_get_export_options (int *n_ret)
{
  CRASH;
  return 0;
}

static void
nogui_do_export (HID_Attr_Val * options)
{
  CRASH;
}

static void
nogui_parse_arguments (int *argc, char ***argv)
{
  CRASH;
}

static void
nogui_invalidate_lr (Coord l, Coord r, Coord t, Coord b)
{
  printf("pcb: invalidate_lr() called without a GUI\n");
  printf("This is ok, if you run an action script\n");
}

static void
nogui_invalidate_all (void)
{
  CRASH;
}

static int
nogui_set_layer (const char *name, int idx, int empty)
{
  CRASH;
  return 0;
}

static void
nogui_end_layer (void)
{
}

static hidGC
nogui_make_gc (void)
{
  return 0;
}

static void
nogui_destroy_gc (hidGC gc)
{
}

static void
nogui_use_mask (enum mask_mode mode)
{
  CRASH;
}

static void
nogui_set_color (hidGC gc, const char *name)
{
  CRASH;
}

static void
nogui_set_line_cap (hidGC gc, EndCapStyle style)
{
  CRASH;
}

static void
nogui_set_line_width (hidGC gc, Coord width)
{
  CRASH;
}

static void
nogui_set_draw_xor (hidGC gc, int xor_)
{
  CRASH;
}

static void
nogui_set_draw_faded (hidGC gc, int faded)
{
}

static void
nogui_draw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  CRASH;
}

static void
nogui_draw_arc (hidGC gc, Coord cx, Coord cy, Coord width, Coord height,
		Angle start_angle, Angle end_angle)
{
  CRASH;
}

static void
nogui_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  CRASH;
}

static void
nogui_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius)
{
  CRASH;
}

static void
nogui_fill_polygon (hidGC gc, int n_coords, Coord *x, Coord *y)
{
  CRASH;
}

static void
nogui_draw_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box)
{
  CRASH;
}

static void
nogui_fill_pcb_pad (hidGC gc, PadType *pad, bool clear, bool mask)
{
  CRASH;
}

static void
nogui_thindraw_pcb_pad (hidGC gc, PadType *pad, bool clear, bool mask)
{
  CRASH;
}

static void
nogui_fill_pcb_pv (hidGC fg_gc, hidGC bg_gc, PinType *pad, bool drawHole, bool mask)
{
  CRASH;
}

static void
nogui_thindraw_pcb_pv (hidGC fg_gc, hidGC bg_gc, PinType *pad, bool drawHole, bool mask)
{
  CRASH;
}

static void
nogui_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  CRASH;
}

static void
nogui_calibrate (double xval, double yval)
{
  CRASH;
}

static int
nogui_shift_is_pressed (void)
{
  /* This is called from FitCrosshairIntoGrid() when the board is loaded.  */
  return 0;
}

static int
nogui_control_is_pressed (void)
{
  CRASH;
  return 0;
}

static int
nogui_mod1_is_pressed (void)
{
  CRASH;
  return 0;
}

static void
nogui_get_coords (const char *msg, Coord *x, Coord *y)
{
  CRASH;
}

static void
nogui_set_crosshair (int x, int y, int action)
{
}

static hidval
nogui_add_timer (void (*func) (hidval user_data),
		 unsigned long milliseconds, hidval user_data)
{
  hidval rv;
  CRASH;
  rv.lval = 0;
  return rv;
}

static void
nogui_stop_timer (hidval timer)
{
  CRASH;
}

static hidval
nogui_watch_file (int fd, unsigned int condition, void (*func) (hidval watch, int fd, unsigned int condition, hidval user_data),
  hidval user_data)
{
  hidval rv;
  CRASH;
  rv.lval = 0;
  return rv;
}

static void
nogui_unwatch_file (hidval watch)
{
  CRASH;
}

static hidval
nogui_add_block_hook (void (*func) (hidval data), hidval data)
{
  hidval rv;
  CRASH;
  rv.ptr = NULL;
  return rv;
}

static void
nogui_stop_block_hook (hidval block_hook)
{
  CRASH;
}

static void
nogui_log (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
}

static void
nogui_logv (const char *fmt, va_list ap)
{
  vprintf (fmt, ap);
}

/* Return a line of user input text, stripped of any newline characters.
 * Returns NULL if the user simply presses enter, or otherwise gives no input.
 */
#define MAX_LINE_LENGTH 1024
static char *
read_stdin_line (void)
{
  static char buf[MAX_LINE_LENGTH];
  char *s;
  int i;

  s = fgets (buf, MAX_LINE_LENGTH, stdin);
  if (s == NULL)
    {
      printf ("\n");
      return NULL;
    }

  /* Strip any trailing newline characters */
  for (i = strlen (s) - 1; i >= 0; i--)
    if (s[i] == '\r' || s[i] == '\n')
      s[i] = '\0';

  if (s[0] == '\0')
    return NULL;

  return strdup (s);
}
#undef MAX_LINE_LENGTH

static int
nogui_confirm_dialog (char *msg, ...)
{
  char *answer;
  int ret = 0;
  bool valid_answer = false;
  va_list args;

  do
    {
      va_start (args, msg);
      vprintf (msg, args);
      va_end (args);

      printf (" ? 0=cancel 1 = ok : ");

      answer = read_stdin_line ();

      if (answer == NULL)
        continue;

      if (answer[0] == '0' && answer[1] == '\0')
        {
          ret = 0;
          valid_answer = true;
        }

      if (answer[0] == '1' && answer[1] == '\0')
        {
          ret = 1;
          valid_answer = true;
        }

      free (answer);
    }
  while (!valid_answer);
  return ret;
}

static int
nogui_close_confirm_dialog ()
{
  return nogui_confirm_dialog (_("OK to lose data ?"), NULL);
}

static void
nogui_report_dialog (char *title, char *msg)
{
  printf ("--- %s ---\n%s\n", title, msg);
}

static char *
nogui_prompt_for (const char *msg, const char *default_string)
{
  char *answer;

  if (default_string)
    printf ("%s [%s] : ", msg, default_string);
  else
    printf ("%s : ", msg);

  answer = read_stdin_line ();
  if (answer == NULL)
    return strdup ((default_string != NULL) ? default_string : "");
  else
    return answer;
}

/* FIXME - this could use some enhancement to actually use the other
   args */
static char *
nogui_fileselect (const char *title, const char *descr,
		  char *default_file, char *default_ext,
		  const char *history_tag, int flags)
{
  char *answer;

  if (default_file)
    printf ("%s [%s] : ", title, default_file);
  else
    printf ("%s : ", title);

  answer = read_stdin_line ();
  if (answer == NULL)
    return (default_file != NULL) ? strdup (default_file) : NULL;
  else
    return answer;
}

static int
nogui_attribute_dialog (HID_Attribute * attrs,
			int n_attrs, HID_Attr_Val * results,
			const char * title, const char * descr)
{
  CRASH;
}

static void
nogui_show_item (void *item)
{
  CRASH;
}

static void
nogui_beep (void)
{
  putchar (7);
  fflush (stdout);
}

static int
nogui_progress (int so_far, int total, const char *message)
{
  return 0;
}

static HID_DRAW *
nogui_request_debug_draw (void)
{
  return NULL;
}

static void
nogui_flush_debug_draw (void)
{
}

static void
nogui_finish_debug_draw (void)
{
}

void
common_nogui_init (HID *hid)
{
  hid->get_export_options =   nogui_get_export_options;
  hid->do_export =            nogui_do_export;
  hid->parse_arguments =      nogui_parse_arguments;
  hid->invalidate_lr =        nogui_invalidate_lr;
  hid->invalidate_all =       nogui_invalidate_all;
  hid->set_layer =            nogui_set_layer;
  hid->end_layer =            nogui_end_layer;
  hid->calibrate =            nogui_calibrate;
  hid->shift_is_pressed =     nogui_shift_is_pressed;
  hid->control_is_pressed =   nogui_control_is_pressed;
  hid->mod1_is_pressed =      nogui_mod1_is_pressed;
  hid->get_coords =           nogui_get_coords;
  hid->set_crosshair =        nogui_set_crosshair;
  hid->add_timer =            nogui_add_timer;
  hid->stop_timer =           nogui_stop_timer;
  hid->watch_file =           nogui_watch_file;
  hid->unwatch_file =         nogui_unwatch_file;
  hid->add_block_hook =       nogui_add_block_hook;
  hid->stop_block_hook =      nogui_stop_block_hook;
  hid->log =                  nogui_log;
  hid->logv =                 nogui_logv;
  hid->confirm_dialog =       nogui_confirm_dialog;
  hid->close_confirm_dialog = nogui_close_confirm_dialog;
  hid->report_dialog =        nogui_report_dialog;
  hid->prompt_for =           nogui_prompt_for;
  hid->fileselect =           nogui_fileselect;
  hid->attribute_dialog =     nogui_attribute_dialog;
  hid->show_item =            nogui_show_item;
  hid->beep =                 nogui_beep;
  hid->progress =             nogui_progress;
  hid->request_debug_draw =   nogui_request_debug_draw;
  hid->flush_debug_draw =     nogui_flush_debug_draw;
  hid->finish_debug_draw =    nogui_finish_debug_draw;
}

void
common_nogui_graphics_init (HID_DRAW *graphics)
{
  graphics->make_gc =         nogui_make_gc;
  graphics->destroy_gc =      nogui_destroy_gc;
  graphics->use_mask =        nogui_use_mask;
  graphics->set_color =       nogui_set_color;
  graphics->set_line_cap =    nogui_set_line_cap;
  graphics->set_line_width =  nogui_set_line_width;
  graphics->set_draw_xor =    nogui_set_draw_xor;
  graphics->set_draw_faded =  nogui_set_draw_faded;
  graphics->draw_line =       nogui_draw_line;
  graphics->draw_arc =        nogui_draw_arc;
  graphics->draw_rect =       nogui_draw_rect;
  graphics->fill_circle =     nogui_fill_circle;
  graphics->fill_polygon =    nogui_fill_polygon;
  graphics->fill_rect =       nogui_fill_rect;

  graphics->draw_pcb_polygon = nogui_draw_pcb_polygon;
  graphics->fill_pcb_pad =     nogui_fill_pcb_pad;
  graphics->thindraw_pcb_pad = nogui_thindraw_pcb_pad;
  graphics->fill_pcb_pv =      nogui_fill_pcb_pv;
  graphics->thindraw_pcb_pv =  nogui_thindraw_pcb_pv;
}

static HID nogui_hid;
static HID_DRAW nogui_graphics;

HID *
hid_nogui_get_hid (void)
{
  memset (&nogui_hid, 0, sizeof (HID));
  memset (&nogui_graphics, 0, sizeof (HID_DRAW));

  nogui_hid.struct_size = sizeof (HID);
  nogui_hid.name        = "nogui";
  nogui_hid.description = "Default GUI when no other GUI is present.  "
                          "Does nothing.";
  nogui_hid.graphics    = &nogui_graphics;

  common_nogui_init (&nogui_hid);
  common_nogui_graphics_init (&nogui_graphics);

  return &nogui_hid;
}
