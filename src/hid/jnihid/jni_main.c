/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "hid.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

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
jnihid_get_export_options (int *n_ret)
{
  CRASH;
  return 0;
}

static void
jnihid_do_export (HID_Attr_Val * options)
{
  CRASH;
}

static void
jnihid_parse_arguments (int *argc, char ***argv)
{
  CRASH;
}

static void
jnihid_invalidate_lr (int l, int r, int t, int b)
{
  CRASH;
}

static void
jnihid_invalidate_all (void)
{
  CRASH;
}

static int
jnihid_set_layer (const char *name, int idx, int empty)
{
  CRASH;
  return 0;
}

static hidGC
jnihid_make_gc (void)
{
  return 0;
}

static void
jnihid_destroy_gc (hidGC gc)
{
}

static void
jnihid_use_mask (int use_it)
{
  CRASH;
}

static void
jnihid_set_color (hidGC gc, const char *name)
{
  CRASH;
}

static void
jnihid_set_line_cap (hidGC gc, EndCapStyle style)
{
  CRASH;
}

static void
jnihid_set_line_width (hidGC gc, int width)
{
  CRASH;
}

static void
jnihid_set_draw_xor (hidGC gc, int xor_)
{
  CRASH;
}

static void
jnihid_set_draw_faded (hidGC gc, int faded)
{
  CRASH;
}

static void
jnihid_set_line_cap_angle (hidGC gc, int x1, int y1, int x2, int y2)
{
  CRASH;
}

static void
jnihid_draw_line (hidGC gc, int x1, int y1, int x2, int y2)
{
  CRASH;
}

static void
jnihid_draw_arc (hidGC gc, int cx, int cy, int width, int height,
		int start_angle, int end_angle)
{
  CRASH;
}

static void
jnihid_draw_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  CRASH;
}

static void
jnihid_fill_circle (hidGC gc, int cx, int cy, int radius)
{
  CRASH;
}

static void
jnihid_fill_polygon (hidGC gc, int n_coords, int *x, int *y)
{
  CRASH;
}

static void
jnihid_fill_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box)
{
  CRASH;
}

static void
jnihid_fill_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  CRASH;
}

static void
jnihid_calibrate (double xval, double yval)
{
  CRASH;
}

static int
jnihid_shift_is_pressed (void)
{
  /* This is called from FitCrosshairIntoGrid() when the board is loaded.  */
  return 0;
}

static int
jnihid_control_is_pressed (void)
{
  CRASH;
  return 0;
}

static int
jnihid_mod1_is_pressed (void)
{
  CRASH;
  return 0;
}

static void
jnihid_get_coords (const char *msg, int *x, int *y)
{
  CRASH;
}

static void
jnihid_set_crosshair (int x, int y, int action)
{
}

static hidval
jnihid_add_timer (void (*func) (hidval user_data),
		 unsigned long milliseconds, hidval user_data)
{
  hidval rv;
  CRASH;
  rv.lval = 0;
  return rv;
}

static void
jnihid_stop_timer (hidval timer)
{
  CRASH;
}

static hidval
jnihid_watch_file (int fd, unsigned int condition, void (*func) (hidval watch, int fd, unsigned int condition, hidval user_data),
  hidval user_data)
{
  hidval rv;
  CRASH;
  rv.lval = 0;
  return rv;
}

static void
jnihid_unwatch_file (hidval watch)
{
  CRASH;
}

static hidval
jnihid_add_block_hook (void (*func) (hidval data), hidval data)
{
  hidval rv;
  CRASH;
  rv.ptr = NULL;
  return rv;
}

static void
jnihid_stop_block_hook (hidval block_hook)
{
  CRASH;
}

static void
jnihid_log (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
}

static void
jnihid_logv (const char *fmt, va_list ap)
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
jnihid_confirm_dialog (char *msg, ...)
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
jnihid_close_confirm_dialog ()
{
  return jnihid_confirm_dialog (_("OK to lose data ?"), NULL);
}

static void
jnihid_report_dialog (char *title, char *msg)
{
  printf ("--- %s ---\n%s\n", title, msg);
}

static char *
jnihid_prompt_for (const char *msg, const char *default_string)
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
    return strdup (answer);
}

/* FIXME - this could use some enhancement to actually use the other
   args */
static char *
jnihid_fileselect (const char *title, const char *descr,
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
    return strdup (answer);
}

static int
jnihid_attribute_dialog (HID_Attribute * attrs,
			int n_attrs, HID_Attr_Val * results,
			const char * title, const char * descr)
{
  CRASH;
}

static void
jnihid_show_item (void *item)
{
  CRASH;
}

static void
jnihid_beep (void)
{
  putchar (7);
  fflush (stdout);
}

static int
jnihid_progress (int so_far, int total, const char *message)
{
  return 0;
}

HID_Action jni_main_action_list[] = {

};

HID hid_jnihid_gui = {
  sizeof (HID),
  "jnihid",
  "Hid implemented through JNI. Needs external support.",
  1, 0, 0, 0, 0, 0,
  jnihid_get_export_options,
  jnihid_do_export,
  jnihid_parse_arguments,
  jnihid_invalidate_lr,
  jnihid_invalidate_all,
  jnihid_set_layer,
  jnihid_make_gc,
  jnihid_destroy_gc,
  jnihid_use_mask,
  jnihid_set_color,
  jnihid_set_line_cap,
  jnihid_set_line_width,
  jnihid_set_draw_xor,
  jnihid_set_draw_faded,
  jnihid_set_line_cap_angle,
  jnihid_draw_line,
  jnihid_draw_arc,
  jnihid_draw_rect,
  jnihid_fill_circle,
  jnihid_fill_polygon,
  jnihid_fill_pcb_polygon,
  0 /* jnihid_thindraw_pcb_polygon */ ,
  jnihid_fill_rect,
  jnihid_calibrate,
  jnihid_shift_is_pressed,
  jnihid_control_is_pressed,
  jnihid_mod1_is_pressed,
  jnihid_get_coords,
  jnihid_set_crosshair,
  jnihid_add_timer,
  jnihid_stop_timer,
  jnihid_watch_file,
  jnihid_unwatch_file,
  jnihid_add_block_hook,
  jnihid_stop_block_hook,
  jnihid_log,
  jnihid_logv,
  jnihid_confirm_dialog,
  jnihid_close_confirm_dialog,
  jnihid_report_dialog,
  jnihid_prompt_for,
  jnihid_fileselect,
  jnihid_attribute_dialog,
  jnihid_show_item,
  jnihid_beep,
  jnihid_progress,
  0 /* jnihid_drc_gui */ ,
  0 /* edit_attributes */
};

#include "dolists.h"
void hid_jnihid_init () {
  hid_register_hid (&hid_jnihid_gui);
  #include "jnihid_lists.h"
}
