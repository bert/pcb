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

#define CRASH fprintf(stderr, "HID error: pcb called GUI function %s without having a GUI available.\n", __FUNCTION__); abort()

typedef struct hid_gc_struct
{
  int nothing_interesting_here;
} hid_gc_struct;

static HID_Attribute *
cocoa_get_export_options (int *n_ret)
{
  CRASH;
  return 0;
}

static void
cocoa_do_export (HID_Attr_Val * options)
{
  CRASH;
}

static void
cocoa_parse_arguments (int *argc, char ***argv)
{
  CRASH;
}

static void
cocoa_invalidate_lr (int l, int r, int t, int b)
{
  CRASH;
}

static void
cocoa_invalidate_all (void)
{
  CRASH;
}

static int
cocoa_set_layer (const char *name, int idx, int empty)
{
  CRASH;
  return 0;
}

static hidGC
cocoa_make_gc (void)
{
  return 0;
}

static void
cocoa_destroy_gc (hidGC gc)
{
}

static void
cocoa_use_mask (int use_it)
{
  CRASH;
}

static void
cocoa_set_color (hidGC gc, const char *name)
{
  CRASH;
}

static void
cocoa_set_line_cap (hidGC gc, EndCapStyle style)
{
  CRASH;
}

static void
cocoa_set_line_width (hidGC gc, int width)
{
  CRASH;
}

static void
cocoa_set_draw_xor (hidGC gc, int xor)
{
  CRASH;
}

static void
cocoa_set_draw_faded (hidGC gc, int faded)
{
  CRASH;
}

static void
cocoa_set_line_cap_angle (hidGC gc, int x1, int y1, int x2, int y2)
{
  CRASH;
}

static void
cocoa_draw_line (hidGC gc, int x1, int y1, int x2, int y2)
{
  CRASH;
}

static void
cocoa_draw_arc (hidGC gc, int cx, int cy, int width, int height,
		int start_angle, int end_angle)
{
  CRASH;
}

static void
cocoa_draw_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  CRASH;
}

static void
cocoa_fill_circle (hidGC gc, int cx, int cy, int radius)
{
  CRASH;
}

static void
cocoa_fill_polygon (hidGC gc, int n_coords, int *x, int *y)
{
  CRASH;
}

static void
cocoa_fill_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box)
{
  CRASH;
}

static void
cocoa_fill_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
  CRASH;
}

static void
cocoa_calibrate (double xval, double yval)
{
  CRASH;
}

static int
cocoa_shift_is_pressed (void)
{
  /* This is called from FitCrosshairIntoGrid() when the board is loaded.  */
  return 0;
}

static int
cocoa_control_is_pressed (void)
{
  CRASH;
  return 0;
}

static int
cocoa_mod1_is_pressed (void)
{
  CRASH;
  return 0;
}

static void
cocoa_get_coords (const char *msg, int *x, int *y)
{
  CRASH;
}

static void
cocoa_set_crosshair (int x, int y, int action)
{
}

static hidval
cocoa_add_timer (void (*func) (hidval user_data),
		 unsigned long milliseconds, hidval user_data)
{
  hidval rv;
  CRASH;
  rv.lval = 0;
  return rv;
}

static void
cocoa_stop_timer (hidval timer)
{
  CRASH;
}

static hidval
cocoa_watch_file (int fd, unsigned int condition, void (*func) (hidval watch, int fd, unsigned int condition, hidval user_data),
  hidval user_data)
{
  hidval rv;
  CRASH;
  rv.lval = 0;
  return rv;
}

static void
cocoa_unwatch_file (hidval watch)
{
  CRASH;
}

static hidval
cocoa_add_block_hook (void (*func) (hidval data), hidval data)
{
  hidval rv;
  CRASH;
  rv.ptr = NULL;
  return rv;
}

static void
cocoa_stop_block_hook (hidval block_hook)
{
  CRASH;
}

static void
cocoa_log (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
}

static void
cocoa_logv (const char *fmt, va_list ap)
{
  vprintf (fmt, ap);
}

static int
cocoa_confirm_dialog (char *msg, ...)
{
  int rv;
  printf ("%s ? 0=cancel 1=ok : ", msg);
  fflush (stdout);
  scanf ("%d", &rv);
  return rv;
}

static int
cocoa_close_confirm_dialog ()
{
  return cocoa_confirm_dialog (_("OK to lose data ?"), NULL);
}

static void
cocoa_report_dialog (char *title, char *msg)
{
  printf ("--- %s ---\n%s\n", title, msg);
}

static char *
cocoa_prompt_for (char *msg, char *default_string)
{
  static char buf[1024];
  if (default_string)
    printf ("%s [%s] : ", msg, default_string);
  else
    printf ("%s : ", msg);
  fgets (buf, 1024, stdin);
  if (buf[0] == 0 && default_string)
    strcpy (buf, default_string);
  return buf;
}

/* FIXME - this could use some enhancement to actually use the other
   args */
static char *
cocoa_fileselect (const char *title, const char *descr,
		  char *default_file, char *default_ext,
		  const char *history_tag, int flags)
{
  static char buf[1024];
  if (default_file)
    printf ("%s [%s] : ", title, default_file);
  else
    printf ("%s : ", title);
  fgets (buf, 1024, stdin);
  if (buf[0] == 0 && default_file)
    strcpy (buf, default_file);
  return buf;
}

static int
cocoa_attribute_dialog (HID_Attribute * attrs,
			int n_attrs, HID_Attr_Val * results,
			const char * title, const char * descr)
{
  CRASH;
}

static void
cocoa_show_item (void *item)
{
  CRASH;
}

static void
cocoa_beep (void)
{
  putchar (7);
  fflush (stdout);
}

static int
cocoa_progress (int so_far, int total, const char *message)
{
  return 0;
}

HID_DRC_GUI cocoa_drc_gui = {
  1,				/* log_drc_overview */
  0,				/* log_drc_details */
  cocoa_drc_window_reset_message,
  cocoa_drc_window_append_violation,
  cocoa_drc_window_throw_dialog,
};

HID hid_cocoa = {
  sizeof (HID),
  "cocoa",
  "Native Cocoa GUI",
  1,				/* gui */
  0,				/* printer */
  0,				/* exporter */
  0,				/* poly before */
  1,				/* poly after */
  0,				/* poly dicer */
  cocoa_get_export_options,
  cocoa_do_export,
  cocoa_parse_arguments,
  cocoa_invalidate_lr,
  cocoa_invalidate_all,
  cocoa_set_layer,
  cocoa_make_gc,
  cocoa_destroy_gc,
  cocoa_use_mask,
  cocoa_set_color,
  cocoa_set_line_cap,
  cocoa_set_line_width,
  cocoa_set_draw_xor,
  cocoa_set_draw_faded,
  cocoa_set_line_cap_angle,
  cocoa_draw_line,
  cocoa_draw_arc,
  cocoa_draw_rect,
  cocoa_fill_circle,
  cocoa_fill_polygon,
  cocoa_fill_pcb_polygon,
  0 /* cocoa_thindraw_pcb_polygon */ ,
  cocoa_fill_rect,
  cocoa_calibrate,
  cocoa_shift_is_pressed,
  cocoa_control_is_pressed,
  cocoa_mod1_is_pressed,
  cocoa_get_coords,
  cocoa_set_crosshair,
  cocoa_add_timer,
  cocoa_stop_timer,
  cocoa_watch_file,
  cocoa_unwatch_file,
  cocoa_add_block_hook,
  cocoa_stop_block_hook,
  cocoa_log,
  cocoa_logv,
  cocoa_confirm_dialog,
  cocoa_close_confirm_dialog,
  cocoa_report_dialog,
  cocoa_prompt_for,
  cocoa_fileselect,
  cocoa_attribute_dialog,
  cocoa_show_item,
  cocoa_beep,
  cocoa_progress,
  0 /* cocoa_drc_gui */ ,
  0 /* edit_attributes */
};

#define AD(x) if (!d->x) d->x = s->x

void
apply_default_hid (HID * d, HID * s)
{
  if (s == 0)
    s = &hid_cocoa;
  AD (get_export_options);
  AD (do_export);
  AD (parse_arguments);
  AD (invalidate_lr);
  AD (invalidate_all);
  AD (set_layer);
  AD (make_gc);
  AD (destroy_gc);
  AD (use_mask);
  AD (set_color);
  AD (set_line_cap);
  AD (set_line_width);
  AD (set_draw_xor);
  AD (set_line_cap_angle);
  AD (draw_line);
  AD (draw_arc);
  AD (fill_circle);
  AD (fill_polygon);
  AD (fill_pcb_polygon);
  AD (thindraw_pcb_polygon);
  AD (calibrate);
  AD (shift_is_pressed);
  AD (control_is_pressed);
  AD (mod1_is_pressed);
  AD (get_coords);
  AD (set_crosshair);
  AD (add_timer);
  AD (stop_timer);
  AD (watch_file);
  AD (unwatch_file);
  AD (add_block_hook);
  AD (stop_block_hook);
  AD (log);
  AD (logv);
  AD (confirm_dialog);
  AD (close_confirm_dialog);
  AD (report_dialog);
  AD (prompt_for);
  AD (fileselect);
  AD (attribute_dialog);
  AD (show_item);
  AD (beep);
  AD (progress);
  AD (drc_gui);
  AD (edit_attributes);
}
