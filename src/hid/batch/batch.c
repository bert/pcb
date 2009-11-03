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
#include "data.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

/* This is a text-line "batch" HID, which exists for scripting and
   non-GUI needs.  */

typedef struct hid_gc_struct
{
  int nothing_interesting_here;
} hid_gc_struct;

static HID_Attribute *
batch_get_export_options (int *n_ret)
{
  return 0;
}

/* ----------------------------------------------------------------------------- */

static char *prompt = "pcb";

static int
nop (int argc, char **argv, int x, int y)
{
  return 0;
}

static int
PCBChanged (int argc, char **argv, int x, int y)
{
  if (PCB && PCB->Filename)
    {
      prompt = strrchr(PCB->Filename, '/');
      if (prompt)
	prompt ++;
      else
	prompt = PCB->Filename;
    }
  else
    prompt = "no-board";
  return 0;
}

static int
help (int argc, char **argv, int x, int y)
{
  print_actions ();
  return 0;
}

static int
info (int argc, char **argv, int x, int y)
{
  int i, j;
  int cg, sg;
  if (!PCB || !PCB->Data || !PCB->Filename)
    {
      printf("No PCB loaded.\n");
      return 0;
    }
  printf("Filename: %s\n", PCB->Filename);
  printf("Size: %g x %g mils, %g x %g mm\n",
	 PCB->MaxWidth / 100.0,
	 PCB->MaxHeight / 100.0,
	 PCB->MaxWidth * COOR_TO_MM,
	 PCB->MaxHeight * COOR_TO_MM);
  cg = GetLayerGroupNumberByNumber (max_layer + COMPONENT_LAYER);
  sg = GetLayerGroupNumberByNumber (max_layer + SOLDER_LAYER);
  for (i=0; i<MAX_LAYER; i++)
    {
      
      int lg = GetLayerGroupNumberByNumber (i);
      for (j=0; j<MAX_LAYER; j++)
	putchar(j==lg ? '#' : '-');
      printf(" %c %s\n", lg==cg ? 'c' : lg==sg ? 's' : '-',
	     PCB->Data->Layer[i].Name);
    }
  return 0;
}


HID_Action batch_action_list[] = {
  {"PCBChanged", 0, PCBChanged },
  {"RouteStylesChanged", 0, nop },
  {"NetlistChanged", 0, nop },
  {"LayersChanged", 0, nop },
  {"LibraryChanged", 0, nop },
  {"Busy", 0, nop },
  {"Help", 0, help },
  {"Info", 0, info }
};

REGISTER_ACTIONS (batch_action_list)


/* ----------------------------------------------------------------------------- */

static void
command_parse (char *s)
{
  int n = 0, ws = 1;
  char *cp;
  char **argv;

  for (cp = s; *cp; cp++)
    {
      if (isspace ((int) *cp))
	ws = 1;
      else
	{
	  n += ws;
	  ws = 0;
	}
    }
  argv = (char **) malloc ((n + 1) * sizeof (char *));

  n = 0;
  ws = 1;
  for (cp = s; *cp; cp++)
    {
      if (isspace ((int) *cp))
	{
	  ws = 1;
	  *cp = 0;
	}
      else
	{
	  if (ws)
	    argv[n++] = cp;
	  ws = 0;
	}
    }
  argv[n] = 0;
  hid_actionv (argv[0], n - 1, argv + 1);
}

static void
batch_do_export (HID_Attr_Val * options)
{
  int interactive;
  char line[1000];

  if (isatty (0))
    interactive = 1;
  else
    interactive = 0;

  if (interactive)
    {
      printf("Entering %s version %s batch mode.\n", PACKAGE, VERSION);
      printf("See http://pcb.gpleda.org for project information\n");
    }
  while (1)
    {
      if (interactive)
	{
	  printf("%s> ", prompt);
	  fflush(stdout);
	}
      if (fgets(line, sizeof(line)-1, stdin) == NULL)
	return;
      if (strchr (line, '('))
	hid_parse_actions (line, 0);
      else
	command_parse (line);
    }
}

static void
batch_parse_arguments (int *argc, char ***argv)
{
  hid_parse_command_line (argc, argv);
}

static void
batch_invalidate_wh (int x, int y, int width, int height, int last)
{
}

static void
batch_invalidate_lr (int l, int r, int t, int b, int last)
{
}

static void
batch_invalidate_all (void)
{
}

static int
batch_set_layer (const char *name, int idx, int empty)
{
  return 0;
}

static hidGC
batch_make_gc (void)
{
  return 0;
}

static void
batch_destroy_gc (hidGC gc)
{
}

static void
batch_use_mask (int use_it)
{
}

static void
batch_set_color (hidGC gc, const char *name)
{
}

static void
batch_set_line_cap (hidGC gc, EndCapStyle style)
{
}

static void
batch_set_line_width (hidGC gc, int width)
{
}

static void
batch_set_draw_xor (hidGC gc, int xor)
{
}

static void
batch_set_draw_faded (hidGC gc, int faded)
{
}

static void
batch_set_line_cap_angle (hidGC gc, int x1, int y1, int x2, int y2)
{
}

static void
batch_draw_line (hidGC gc, int x1, int y1, int x2, int y2)
{
}

static void
batch_draw_arc (hidGC gc, int cx, int cy, int width, int height,
		int start_angle, int end_angle)
{
}

static void
batch_draw_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
}

static void
batch_fill_circle (hidGC gc, int cx, int cy, int radius)
{
}

static void
batch_fill_polygon (hidGC gc, int n_coords, int *x, int *y)
{
}

static void
batch_fill_pcb_polygon (hidGC gc, PolygonType *poly)
{
}

static void
batch_thindraw_pcb_polygon (hidGC gc, PolygonType *poly)
{
}

static void
batch_fill_rect (hidGC gc, int x1, int y1, int x2, int y2)
{
}

static void
batch_calibrate (double xval, double yval)
{
}

static int
batch_shift_is_pressed (void)
{
  return 0;
}

static int
batch_control_is_pressed (void)
{
  return 0;
}

static int
batch_mod1_is_pressed (void)
{
  return 0;
}

static void
batch_get_coords (const char *msg, int *x, int *y)
{
}

static void
batch_set_crosshair (int x, int y, int action)
{
}

static hidval
batch_add_timer (void (*func) (hidval user_data),
		 unsigned long milliseconds, hidval user_data)
{
  hidval rv;
  rv.lval = 0;
  return rv;
}

static void
batch_stop_timer (hidval timer)
{
}

hidval
batch_watch_file (int fd, unsigned int condition, void (*func) (hidval watch, int fd, unsigned int condition, hidval user_data),
    hidval user_data)
{
  hidval ret;
  ret.ptr = NULL;
  return ret;
}

void
batch_unwatch_file (hidval data)
{
}

static hidval
batch_add_block_hook (void (*func) (hidval data), hidval user_data )
{
  hidval ret;
  ret.ptr = NULL;
  return ret;
}

static void
batch_stop_block_hook (hidval mlpoll)
{
}

static void
batch_log (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
}

static void
batch_logv (const char *fmt, va_list ap)
{
  vprintf (fmt, ap);
}

static int
batch_confirm_dialog (char *msg, ...)
{
  int rv;
  printf ("%s ? 0=cancel 1=ok : ", msg);
  fflush (stdout);
  scanf ("%d", &rv);
  return rv;
}

static int
batch_close_confirm_dialog ()
{
  return batch_confirm_dialog (_("OK to lose data ?"), NULL);
}

static void
batch_report_dialog (char *title, char *msg)
{
  printf ("--- %s ---\n%s\n", title, msg);
}

static char *
batch_prompt_for (char *msg, char *default_string)
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

static char *
batch_fileselect (const char *title, const char *descr,
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
batch_attribute_dialog (HID_Attribute * attrs,
			int n_attrs, HID_Attr_Val * results,
			const char *descr)
{
}

static void
batch_show_item (void *item)
{
}

static void
batch_beep (void)
{
  putchar (7);
  fflush (stdout);
}

static void
batch_progress (int so_far, int total, const char *message)
{
}

HID batch_gui = {
  sizeof (HID),
  "batch",
  "Batch-mode GUI for non-interactive use.",
  1, 0, 0, 0, 0, 0,
  batch_get_export_options,
  batch_do_export,
  batch_parse_arguments,
  batch_invalidate_wh,
  batch_invalidate_lr,
  batch_invalidate_all,
  batch_set_layer,
  batch_make_gc,
  batch_destroy_gc,
  batch_use_mask,
  batch_set_color,
  batch_set_line_cap,
  batch_set_line_width,
  batch_set_draw_xor,
  batch_set_draw_faded,
  batch_set_line_cap_angle,
  batch_draw_line,
  batch_draw_arc,
  batch_draw_rect,
  batch_fill_circle,
  batch_fill_polygon,
  batch_fill_pcb_polygon,
  batch_thindraw_pcb_polygon,
  batch_fill_rect,
  batch_calibrate,
  batch_shift_is_pressed,
  batch_control_is_pressed,
  batch_mod1_is_pressed,
  batch_get_coords,
  batch_set_crosshair,
  batch_add_timer,
  batch_stop_timer,
  batch_watch_file,
  batch_unwatch_file,
  batch_add_block_hook,
  batch_stop_block_hook,
  batch_log,
  batch_logv,
  batch_confirm_dialog,
  batch_close_confirm_dialog,
  batch_report_dialog,
  batch_prompt_for,
  batch_fileselect,
  batch_attribute_dialog,
  batch_show_item,
  batch_beep,
  batch_progress,
  0 /* batch_drc_gui */
};

#include "dolists.h"

void
hid_batch_init ()
{
  hid_register_hid (&batch_gui);
#include "batch_lists.h"
}
