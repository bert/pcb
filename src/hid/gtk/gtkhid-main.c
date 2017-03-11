#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <math.h>
#include <time.h>


#include "action.h"
#include "crosshair.h"
#include "error.h"
#include "../hidint.h"
#include "gui.h"
#include "hid/common/hidnogui.h"
#include "hid/common/draw_helpers.h"
#include "pcb-printf.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

static void
pan_common (GHidPort *port)
{
  /* Don't pan so far the board is completely off the screen */
  port->view.x0 = MAX (-port->view.width,  port->view.x0);
  port->view.y0 = MAX (-port->view.height, port->view.y0);
  port->view.x0 = MIN ( port->view.x0, PCB->MaxWidth);
  port->view.y0 = MIN ( port->view.y0, PCB->MaxHeight);

  ghidgui->adjustment_changed_holdoff = TRUE;
  gtk_range_set_value (GTK_RANGE (ghidgui->h_range), port->view.x0);
  gtk_range_set_value (GTK_RANGE (ghidgui->v_range), port->view.y0);
  ghidgui->adjustment_changed_holdoff = FALSE;

  ghid_port_ranges_changed();
}

static void
ghid_pan_view_abs (Coord pcb_x, Coord pcb_y, int widget_x, int widget_y)
{
  gport->view.x0 = SIDE_X (pcb_x) - widget_x * gport->view.coord_per_px;
  gport->view.y0 = SIDE_Y (pcb_y) - widget_y * gport->view.coord_per_px;

  pan_common (gport);
}

void
ghid_pan_view_rel (Coord dx, Coord dy)
{
  gport->view.x0 += dx;
  gport->view.y0 += dy;

  pan_common (gport);
}


/* gport->view.coord_per_px:
 * zoom value is PCB units per screen pixel.  Larger numbers mean zooming
 * out - the largest value means you are looking at the whole board.
 *
 * gport->view_width and gport->view_height are in PCB coordinates
 */

#define ALLOW_ZOOM_OUT_BY 10 /* Arbitrary, and same as the lesstif HID */
static void
ghid_zoom_view_abs (Coord center_x, Coord center_y, double new_zoom)
{
  double min_zoom, max_zoom;
  double xtmp, ytmp;

  /* Limit the "minimum" zoom constant (maximum zoom), at 1 pixel per PCB
   * unit, and set the "maximum" zoom constant (minimum zoom), such that
   * the entire board just fits inside the viewport
   */
  min_zoom = 1;
  max_zoom = MAX (PCB->MaxWidth  / gport->width,
                  PCB->MaxHeight / gport->height) * ALLOW_ZOOM_OUT_BY;
  new_zoom = MIN (MAX (min_zoom, new_zoom), max_zoom);

  if (gport->view.coord_per_px == new_zoom)
    return;

  xtmp = (SIDE_X (center_x) - gport->view.x0) / (double)gport->view.width;
  ytmp = (SIDE_Y (center_y) - gport->view.y0) / (double)gport->view.height;

  gport->view.coord_per_px = new_zoom;
  pixel_slop = new_zoom;
  ghid_port_ranges_scale ();

  gport->view.x0 = SIDE_X (center_x) - xtmp * gport->view.width;
  gport->view.y0 = SIDE_Y (center_y) - ytmp * gport->view.height;

  pan_common (gport);

  ghid_set_status_line_label ();
}

static void
ghid_zoom_view_rel (Coord center_x, Coord center_y, double factor)
{
  ghid_zoom_view_abs (center_x, center_y, gport->view.coord_per_px * factor);
}

static void
ghid_zoom_view_fit (void)
{
  ghid_pan_view_abs (SIDE_X (0), SIDE_Y (0), 0, 0);
  ghid_zoom_view_abs (SIDE_X (0), SIDE_Y (0),
                      MAX (PCB->MaxWidth  / gport->width,
                           PCB->MaxHeight / gport->height));
}

static void
ghid_flip_view (Coord center_x, Coord center_y, bool flip_x, bool flip_y)
{
  int widget_x, widget_y;

  /* Work out where on the screen the flip point is */
  ghid_pcb_to_event_coords (center_x, center_y, &widget_x, &widget_y);

  gport->view.flip_x = gport->view.flip_x != flip_x;
  gport->view.flip_y = gport->view.flip_y != flip_y;

  /* Pan the board so the center location remains in the same place */
  ghid_pan_view_abs (center_x, center_y, widget_x, widget_y);

  ghid_invalidate_all ();
}

/* ------------------------------------------------------------ */

static const char zoom_syntax[] =
"Zoom()\n"
"Zoom(factor)";


static const char zoom_help[] =
N_("Various zoom factor changes.");

/* %start-doc actions Zoom
Changes the zoom (magnification) of the view of the board.  If no
arguments are passed, the view is scaled such that the board just fits
inside the visible window (i.e. ``view all'').  Otherwise,
@var{factor} specifies a change in zoom factor.  It may be prefixed by
@code{+}, @code{-}, or @code{=} to change how the zoom factor is
modified.  The @var{factor} is a floating point number, such as
@code{1.5} or @code{0.75}.

@table @code
  
@item +@var{factor}
Values greater than 1.0 cause the board to be drawn smaller; more of
the board will be visible.  Values between 0.0 and 1.0 cause the board
to be drawn bigger; less of the board will be visible.
  
@item -@var{factor}
Values greater than 1.0 cause the board to be drawn bigger; less of
the board will be visible.  Values between 0.0 and 1.0 cause the board
to be drawn smaller; more of the board will be visible.
 
@item =@var{factor}
 
The @var{factor} is an absolute zoom factor; the unit for this value
is "PCB units per screen pixel".  Since PCB units are 0.01 mil, a
@var{factor} of 1000 means 10 mils (0.01 in) per pixel, or 100 DPI,
about the actual resolution of most screens - resulting in an "actual
size" board.  Similarly, a @var{factor} of 100 gives you a 10x actual
size.
 
@end table
 
Note that zoom factors of zero are silently ignored.
 


%end-doc */

static int
Zoom (int argc, char **argv, Coord x, Coord y)
{
  const char *vp;
  double v;

  if (argc > 1)
    AFAIL (zoom);

  if (argc < 1)
    {
      ghid_zoom_view_fit ();
      return 0;
    }

  vp = argv[0];
  if (*vp == '+' || *vp == '-' || *vp == '=')
    vp++;
  v = g_ascii_strtod (vp, 0);
  if (v <= 0)
    return 1;
  switch (argv[0][0])
    {
    case '-':
      ghid_zoom_view_rel (x, y, 1 / v);
      break;
    default:
    case '+':
      ghid_zoom_view_rel (x, y, v);
      break;
    case '=':
      ghid_zoom_view_abs (x, y, v);
      break;
    }

  return 0;
}

/* ------------------------------------------------------------ */

void
ghid_calibrate (double xval, double yval)
{
  printf (_("ghid_calibrate() -- not implemented\n"));
}

void
ghid_notify_gui_is_up ()
{
  ghidgui->is_up = 1;
}

int
ghid_shift_is_pressed ()
{
  GdkModifierType mask;
  GHidPort *out = &ghid_port;

  if (!ghidgui->is_up)
    return 0;

  gdk_window_get_pointer (gtk_widget_get_window (out->drawing_area),
                          NULL, NULL, &mask);
  return (mask & GDK_SHIFT_MASK) ? TRUE : FALSE;
}

int
ghid_control_is_pressed ()
{
  GdkModifierType mask;
  GHidPort *out = &ghid_port;

  if (!ghidgui->is_up)
    return 0;

  gdk_window_get_pointer (gtk_widget_get_window (out->drawing_area),
                          NULL, NULL, &mask);
  return (mask & GDK_CONTROL_MASK) ? TRUE : FALSE;
}

int
ghid_mod1_is_pressed ()
{
  GdkModifierType mask;
  GHidPort *out = &ghid_port;

  if (!ghidgui->is_up)
    return 0;

  gdk_window_get_pointer (gtk_widget_get_window (out->drawing_area),
                          NULL, NULL, &mask);
#ifdef __APPLE__
  return (mask & ( 1 << 13 ) ) ? TRUE : FALSE;  // The option key is not MOD1, although it should be...
#else
  return (mask & GDK_MOD1_MASK) ? TRUE : FALSE;
#endif
}

void
ghid_set_crosshair (int x, int y, int action)
{
  GdkDisplay *display;
  GdkScreen *screen;
  int offset_x, offset_y;
  int widget_x, widget_y;
  int pointer_x, pointer_y;
  Coord pcb_x, pcb_y;

  if (gport->crosshair_x != x || gport->crosshair_y != y)
    {
      ghid_set_cursor_position_labels ();
      gport->crosshair_x = x;
      gport->crosshair_y = y;

      /* FIXME - does this trigger the idle_proc stuff?  It is in the
       * lesstif HID.  Maybe something is needed here?
       *
       * need_idle_proc ();
       */
    }

  if (action != HID_SC_PAN_VIEWPORT &&
      action != HID_SC_WARP_POINTER &&
      action != HID_SC_CENTER_IN_VIEWPORT &&
      action != HID_SC_CENTER_IN_VIEWPORT_AND_WARP_POINTER)
    return;

  /* Find out where the drawing area is on the screen. gdk_display_get_pointer
   * and gdk_display_warp_pointer work relative to the whole display, whilst
   * our coordinates are relative to the drawing area origin.
   */
  gdk_window_get_origin (gtk_widget_get_window (gport->drawing_area),
                         &offset_x, &offset_y);

  display = gdk_display_get_default ();
  screen = gdk_display_get_default_screen (display);

  switch (action) {

    case HID_SC_CENTER_IN_VIEWPORT:

      // Center the viewport on the crosshair
      ghid_pan_view_abs (gport->crosshair_x - gport->view.width / 2,
                         gport->crosshair_y - gport->view.height / 2,
                         0, 0);

      break;

    case HID_SC_CENTER_IN_VIEWPORT_AND_WARP_POINTER:

      // Center the viewport on the crosshair
      ghid_pan_view_abs (gport->crosshair_x - gport->view.width / 2,
                         gport->crosshair_y - gport->view.height / 2,
                         0, 0);
  
      // We do this to make sure gdk has an up-to-date idea of the widget
      // coordinates so gdk_display_warp_pointer will go to the right spot.
      gdk_window_process_all_updates ();

      // Warp pointer to crosshair
      ghid_pcb_to_event_coords (gport->crosshair_x, gport->crosshair_y,
                                &widget_x, &widget_y);
      gdk_display_warp_pointer (display, screen, widget_x + offset_x,
                                widget_y + offset_y);

      break;

    case HID_SC_PAN_VIEWPORT:
      /* Pan the board in the viewport so that the crosshair (who's location
       * relative on the board was set above) lands where the pointer is.
       * We pass the request to pan a particular point on the board to a
       * given widget coordinate of the viewport into the rendering code
       */

      /* Find out where the pointer is relative to the display */
      gdk_display_get_pointer (display, NULL, &pointer_x, &pointer_y, NULL);

      widget_x = pointer_x - offset_x;
      widget_y = pointer_y - offset_y;

      ghid_event_to_pcb_coords (widget_x, widget_y, &pcb_x, &pcb_y);
      ghid_pan_view_abs (pcb_x, pcb_y, widget_x, widget_y);

      /* Just in case we couldn't pan the board the whole way,
       * we warp the pointer to where the crosshair DID land.
       */
      /* Fall through */

    case HID_SC_WARP_POINTER:
      screen = gdk_display_get_default_screen (display);

      ghid_pcb_to_event_coords (x, y, &widget_x, &widget_y);

      pointer_x = offset_x + widget_x;
      pointer_y = offset_y + widget_y;

      gdk_display_warp_pointer (display, screen, pointer_x, pointer_y);

      break;
  }
}

typedef struct
{
  void (*func) (hidval);
  guint id;
  hidval user_data;
}
GuiTimer;

  /* We need a wrapper around the hid timer because a gtk timer needs
     |  to return FALSE else the timer will be restarted.
   */
static gboolean
ghid_timer (GuiTimer * timer)
{
  (*timer->func) (timer->user_data);
  ghid_mode_cursor (Settings.Mode);
  return FALSE;			/* Turns timer off */
}

hidval
ghid_add_timer (void (*func) (hidval user_data),
		unsigned long milliseconds, hidval user_data)
{
  GuiTimer *timer = g_new0 (GuiTimer, 1);
  hidval ret;

  timer->func = func;
  timer->user_data = user_data;
  timer->id = g_timeout_add (milliseconds, (GSourceFunc) ghid_timer, timer);
  ret.ptr = (void *) timer;
  return ret;
}

void
ghid_stop_timer (hidval timer)
{
  void *ptr = timer.ptr;

  g_source_remove (((GuiTimer *) ptr)->id);
  g_free( ptr );
}

typedef struct
{
  void (*func) ( hidval, int, unsigned int, hidval );
  hidval user_data;
  int fd;
  GIOChannel *channel;
  gint id;
}
GuiWatch;

  /* We need a wrapper around the hid file watch to pass the correct flags
   */
static gboolean
ghid_watch (GIOChannel *source, GIOCondition condition, gpointer data)
{
  unsigned int pcb_condition = 0;
  hidval x;
  GuiWatch *watch = (GuiWatch*)data;

  if (condition & G_IO_IN)
    pcb_condition |= PCB_WATCH_READABLE;
  if (condition & G_IO_OUT)
    pcb_condition |= PCB_WATCH_WRITABLE;
  if (condition & G_IO_ERR)
    pcb_condition |= PCB_WATCH_ERROR;
  if (condition & G_IO_HUP)
    pcb_condition |= PCB_WATCH_HANGUP;

  x.ptr = (void *) watch;
  watch->func (x, watch->fd, pcb_condition, watch->user_data);
  ghid_mode_cursor (Settings.Mode);

  return TRUE;  /* Leave watch on */
}

hidval
ghid_watch_file (int fd, unsigned int condition, void (*func) (hidval watch, int fd, unsigned int condition, hidval user_data),
  hidval user_data)
{
  GuiWatch *watch = g_new0 (GuiWatch, 1);
  hidval ret;
  unsigned int glib_condition = 0;

  if (condition & PCB_WATCH_READABLE)
    glib_condition |= G_IO_IN;
  if (condition & PCB_WATCH_WRITABLE)
    glib_condition |= G_IO_OUT;
  if (condition & PCB_WATCH_ERROR)
    glib_condition |= G_IO_ERR;
  if (condition & PCB_WATCH_HANGUP)
    glib_condition |= G_IO_HUP;

  watch->func = func;
  watch->user_data = user_data;
  watch->fd = fd;
  watch->channel = g_io_channel_unix_new( fd );
  watch->id = g_io_add_watch( watch->channel, (GIOCondition)glib_condition, ghid_watch, watch );

  ret.ptr = (void *) watch;
  return ret;
}

void
ghid_unwatch_file (hidval data)
{
  GuiWatch *watch = (GuiWatch*)data.ptr;

  g_io_channel_shutdown( watch->channel, TRUE, NULL ); 
  g_io_channel_unref( watch->channel );
  g_free( watch );
}

typedef struct
{
  GSource source;
  void (*func) (hidval user_data);
  hidval user_data; 
} BlockHookSource;

static gboolean ghid_block_hook_prepare  (GSource     *source,
                                             gint     *timeout);
static gboolean ghid_block_hook_check    (GSource     *source);
static gboolean ghid_block_hook_dispatch (GSource     *source,
                                          GSourceFunc  callback,
                                          gpointer     user_data);

static GSourceFuncs ghid_block_hook_funcs = {
  ghid_block_hook_prepare,
  ghid_block_hook_check,
  ghid_block_hook_dispatch,
  NULL // No destroy notification
};

static gboolean
ghid_block_hook_prepare (GSource *source,
                         gint    *timeout)
{
  hidval data = ((BlockHookSource *)source)->user_data;
  ((BlockHookSource *)source)->func( data );
  return FALSE;
}

static gboolean
ghid_block_hook_check (GSource *source)
{
  return FALSE;
}

static gboolean
ghid_block_hook_dispatch (GSource     *source,
                          GSourceFunc  callback,
                          gpointer     user_data)
{
  return FALSE;
}

static hidval
ghid_add_block_hook (void (*func) (hidval data),
                     hidval user_data)
{
  hidval ret;
  BlockHookSource *source;

  source = (BlockHookSource *)g_source_new (&ghid_block_hook_funcs, sizeof( BlockHookSource ));

  source->func = func;
  source->user_data = user_data;

  g_source_attach ((GSource *)source, NULL);

  ret.ptr = (void *) source;
  return ret;
}

static void
ghid_stop_block_hook (hidval mlpoll)
{
  GSource *source = (GSource *)mlpoll.ptr;
  g_source_destroy( source );
}

int
ghid_confirm_dialog (char *msg, ...)
{
  int rv = 0;
  va_list ap;
  char *cancelmsg = NULL, *okmsg = NULL;
  static gint x = -1, y = -1;
  GtkWidget *dialog;
  GHidPort *out = &ghid_port;

  va_start (ap, msg);
  cancelmsg = va_arg (ap, char *);
  okmsg = va_arg (ap, char *);
  va_end (ap);

  if (!cancelmsg)
    {
      cancelmsg = _("_Cancel");
      okmsg = _("_OK");
    }

  dialog = gtk_message_dialog_new (GTK_WINDOW (out->top_window),
				   (GtkDialogFlags) (GTK_DIALOG_MODAL |
						     GTK_DIALOG_DESTROY_WITH_PARENT),
				   GTK_MESSAGE_QUESTION,
				   GTK_BUTTONS_NONE,
				   "%s", msg);
  gtk_dialog_add_button (GTK_DIALOG (dialog), 
			  cancelmsg, GTK_RESPONSE_CANCEL);
  if (okmsg)
    {
      gtk_dialog_add_button (GTK_DIALOG (dialog), 
			     okmsg, GTK_RESPONSE_OK);
    }

  if(x != -1) {
  	gtk_window_move(GTK_WINDOW (dialog), x, y);
  }

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    rv = 1;

  gtk_window_get_position(GTK_WINDOW (dialog), &x, &y);

  gtk_widget_destroy (dialog);
  return rv;
}

int
ghid_close_confirm_dialog ()
{
  switch (ghid_dialog_close_confirm ())
    {
    case GUI_DIALOG_CLOSE_CONFIRM_SAVE:
      {
        if (hid_actionl ("Save", NULL))
          { /* Save failed */
            return 0; /* Cancel */
          } else {
            return 1; /* Close */
          }
      }
    case GUI_DIALOG_CLOSE_CONFIRM_NOSAVE:
      {
        return 1; /* Close */
      }
    case GUI_DIALOG_CLOSE_CONFIRM_CANCEL:
    default:
      {
        return 0; /* Cancel */
      }
    }
}

void
ghid_report_dialog (char *title, char *msg)
{
  ghid_dialog_report (title, msg);
}

char *
ghid_prompt_for (const char *msg, const char *default_string)
{
  char *rv;

  rv = ghid_dialog_input (msg, default_string);
  return rv;
}

/* FIXME -- implement a proper file select dialog */
#ifdef FIXME
char *
ghid_fileselect (const char *title, const char *descr,
		 char *default_file, char *default_ext,
		 const char *history_tag, int flags)
{
  char *rv;

  rv = ghid_dialog_input (title, default_file);
  return rv;
}
#endif

void
ghid_show_item (void *item)
{
  ghid_pinout_window_show (&ghid_port, (ElementType *) item);
}

void
ghid_beep ()
{
  gdk_beep ();
}

struct progress_dialog
{
  GtkWidget *dialog;
  GtkWidget *message;
  GtkWidget *progress;
  gint response_id;
  GMainLoop *loop;
  gboolean destroyed;
  gboolean started;
  GTimer *timer;

  gulong response_handler;
  gulong destroy_handler;
  gulong delete_handler;
};

static void
run_response_handler (GtkDialog *dialog,
                      gint response_id,
                      gpointer data)
{
  struct progress_dialog *pd = data;

  pd->response_id = response_id;
}

static gint
run_delete_handler (GtkDialog *dialog,
                    GdkEventAny *event,
                    gpointer data)
{
  struct progress_dialog *pd = data;

  pd->response_id = GTK_RESPONSE_DELETE_EVENT;

  return TRUE; /* Do not destroy */
}

static void
run_destroy_handler (GtkDialog *dialog, gpointer data)
{
  struct progress_dialog *pd = data;

  pd->destroyed = TRUE;
}

static struct progress_dialog *
make_progress_dialog (void)
{
  struct progress_dialog *pd;
  GtkWidget *content_area;
  GtkWidget *alignment;
  GtkWidget *vbox;

  pd = g_new0 (struct progress_dialog, 1);
  pd->response_id = GTK_RESPONSE_NONE;

  pd->dialog = gtk_dialog_new_with_buttons (_("Progress"),
                                            GTK_WINDOW (gport->top_window),
                                            /* Modal so nothing else can get events whilst
                                               the main mainloop isn't running */
                                            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_STOCK_CANCEL,
                                            GTK_RESPONSE_CANCEL,
                                            NULL);

  gtk_window_set_deletable (GTK_WINDOW (pd->dialog), FALSE);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (pd->dialog), TRUE);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (pd->dialog), TRUE);
  gtk_widget_set_size_request (pd->dialog, 300, -1);

  pd->message = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (pd->message), 0., 0.);

  pd->progress = gtk_progress_bar_new ();
  gtk_widget_set_size_request (pd->progress, -1, 26);

  vbox = gtk_vbox_new (false, 0);
  gtk_box_pack_start (GTK_BOX (vbox), pd->message, true, true, 8);
  gtk_box_pack_start (GTK_BOX (vbox), pd->progress, false, true, 8);

  alignment = gtk_alignment_new (0., 0., 1., 1.);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 8, 8, 8, 8);
  gtk_container_add (GTK_CONTAINER (alignment), vbox);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (pd->dialog));
  gtk_box_pack_start (GTK_BOX (content_area), alignment, true, true, 0);

  gtk_widget_show_all (alignment);

  g_object_ref (pd->dialog);
  gtk_window_present (GTK_WINDOW (pd->dialog));

  pd->response_handler =
    g_signal_connect (pd->dialog, "response",
                      G_CALLBACK (run_response_handler), pd);
  pd->delete_handler =
    g_signal_connect (pd->dialog, "delete-event",
                      G_CALLBACK (run_delete_handler), pd);
  pd->destroy_handler =
    g_signal_connect (pd->dialog, "destroy",
                      G_CALLBACK (run_destroy_handler), pd);

  pd->loop = g_main_loop_new (NULL, FALSE);
  pd->timer = g_timer_new ();

  return pd;
}

static void
destroy_progress_dialog (struct progress_dialog *pd)
{
  if (pd == NULL)
    return;

  if (!pd->destroyed)
    {
      g_signal_handler_disconnect (pd->dialog, pd->response_handler);
      g_signal_handler_disconnect (pd->dialog, pd->delete_handler);
      g_signal_handler_disconnect (pd->dialog, pd->destroy_handler);
    }

  g_timer_destroy (pd->timer);
  g_object_unref (pd->dialog);
  g_main_loop_unref (pd->loop);

  gtk_widget_destroy (pd->dialog);

  pd->loop = NULL;
  g_free (pd);
}

static void
handle_progress_dialog_events (struct progress_dialog *pd)
{
  GMainContext * context = g_main_loop_get_context (pd->loop);

  /* Process events */
  while (g_main_context_pending (context))
    {
      g_main_context_iteration (context, FALSE);
    }
}

#define MIN_TIME_SEPARATION (50./1000.) /* 50ms */
static int
ghid_progress (int so_far, int total, const char *message)
{
  static struct progress_dialog *pd = NULL;

  /* If we are finished, destroy any dialog */
  if (so_far == 0 && total == 0 && message == NULL)
    {
      destroy_progress_dialog (pd);
      pd = NULL;
      return 0;
    }

  if (pd == NULL)
    pd = make_progress_dialog ();

  /* We don't want to keep the underlying process too busy whilst we
   * process events. If we get called quickly after the last progress
   * update, wait a little bit before we respond - perhaps the next
   * time progress is reported.

   * The exception here is that we always want to process the first
   * batch of events after having shown the dialog for the first time
   */
  if (pd->started && g_timer_elapsed (pd->timer, NULL) < MIN_TIME_SEPARATION)
    return 0;

  gtk_label_set_text (GTK_LABEL (pd->message), message);
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pd->progress),
                                 (double)so_far / (double)total);

  handle_progress_dialog_events (pd);
  g_timer_start (pd->timer);

  pd->started = TRUE;

  return (pd->response_id == GTK_RESPONSE_CANCEL ||
          pd->response_id == GTK_RESPONSE_DELETE_EVENT) ? 1 : 0;
}

/* ---------------------------------------------------------------------- */


typedef struct {
  GtkWidget *del;
  GtkWidget *w_name;
  GtkWidget *w_value;
} AttrRow;

static AttrRow *attr_row = 0;
static int attr_num_rows = 0;
static int attr_max_rows = 0;
static AttributeListType *attributes_list;
static GtkWidget *attributes_dialog, *attr_table;

static void attributes_delete_callback (GtkWidget *w, void *v);

#define GA_RESPONSE_REVERT	1
#define GA_RESPONSE_NEW		2

static void
ghid_attr_set_table_size ()
{
  gtk_table_resize (GTK_TABLE (attr_table), attr_num_rows > 0 ? attr_num_rows : 1, 3);
}

static void
ghid_attributes_need_rows (int new_max)
{
  if (attr_max_rows < new_max)
    {
      if (attr_row)
	attr_row = (AttrRow *) realloc (attr_row, new_max * sizeof(AttrRow));
      else
	attr_row = (AttrRow *) malloc (new_max * sizeof(AttrRow));
    }
  while (attr_max_rows < new_max)
    {
      /* add [attr_max_rows] */
      attr_row[attr_max_rows].del = gtk_button_new_with_label (_("del"));
      gtk_table_attach (GTK_TABLE (attr_table), attr_row[attr_max_rows].del,
			0, 1,
			attr_max_rows, attr_max_rows+1,
			(GtkAttachOptions)(GTK_FILL | GTK_EXPAND),
			GTK_FILL,
			0, 0);
      g_signal_connect (G_OBJECT (attr_row[attr_max_rows].del), "clicked",
			G_CALLBACK (attributes_delete_callback), GINT_TO_POINTER (attr_max_rows) );

      attr_row[attr_max_rows].w_name = gtk_entry_new ();
      gtk_table_attach (GTK_TABLE (attr_table), attr_row[attr_max_rows].w_name,
			1, 2,
			attr_max_rows, attr_max_rows+1,
			(GtkAttachOptions)(GTK_FILL | GTK_EXPAND),
			GTK_FILL,
			0, 0);

      attr_row[attr_max_rows].w_value = gtk_entry_new ();
      gtk_table_attach (GTK_TABLE (attr_table), attr_row[attr_max_rows].w_value,
			2, 3,
			attr_max_rows, attr_max_rows+1,
			(GtkAttachOptions)(GTK_FILL | GTK_EXPAND),
			GTK_FILL,
			0, 0);

      attr_max_rows ++;
    }

  /* Manage any previously unused rows we now need to show.  */
  while (attr_num_rows < new_max)
    {
      /* manage attr_num_rows */
      gtk_widget_show (attr_row[attr_num_rows].del);
      gtk_widget_show (attr_row[attr_num_rows].w_name);
      gtk_widget_show (attr_row[attr_num_rows].w_value);
      attr_num_rows ++;
    }
}

static void
ghid_attributes_revert ()
{
  int i;

  ghid_attributes_need_rows (attributes_list->Number);

  /* Unmanage any previously used rows we don't need.  */
  while (attr_num_rows > attributes_list->Number)
    {
      attr_num_rows --;
      gtk_widget_hide (attr_row[attr_num_rows].del);
      gtk_widget_hide (attr_row[attr_num_rows].w_name);
      gtk_widget_hide (attr_row[attr_num_rows].w_value);
    }

  /* Fill in values */
  for (i=0; i<attributes_list->Number; i++)
    {
      /* create row [i] */
      gtk_entry_set_text (GTK_ENTRY (attr_row[i].w_name), attributes_list->List[i].name);
      gtk_entry_set_text (GTK_ENTRY (attr_row[i].w_value), attributes_list->List[i].value);
#if 0
#endif
    }
  ghid_attr_set_table_size ();
}

static void
attributes_delete_callback (GtkWidget *w, void *v)
{
  int i, n;

  n = GPOINTER_TO_INT (v);

  for (i=n; i<attr_num_rows-1; i++)
    {
      gtk_entry_set_text (GTK_ENTRY (attr_row[i].w_name),
			  gtk_entry_get_text (GTK_ENTRY (attr_row[i+1].w_name)));
      gtk_entry_set_text (GTK_ENTRY (attr_row[i].w_value),
			  gtk_entry_get_text (GTK_ENTRY (attr_row[i+1].w_value)));
    }
  attr_num_rows --;

  gtk_widget_hide (attr_row[attr_num_rows].del);
  gtk_widget_hide (attr_row[attr_num_rows].w_name);
  gtk_widget_hide (attr_row[attr_num_rows].w_value);

  ghid_attr_set_table_size ();
}

static void
ghid_attributes (char *owner, AttributeListType *attrs)
{
  GtkWidget *content_area;
  int response;

  attributes_list = attrs;

  attr_max_rows = 0;
  attr_num_rows = 0;

  attributes_dialog = gtk_dialog_new_with_buttons (owner,
						   GTK_WINDOW (ghid_port.top_window),
						   GTK_DIALOG_MODAL,
						   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						   _("Revert"), GA_RESPONSE_REVERT,
						   _("New"), GA_RESPONSE_NEW,
						   GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

  attr_table = gtk_table_new (attrs->Number, 3, 0);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (attributes_dialog));
  gtk_box_pack_start (GTK_BOX (content_area), attr_table, FALSE, FALSE, 0);

  gtk_widget_show (attr_table);

  ghid_attributes_revert ();

  while (1)
    {
      response = gtk_dialog_run (GTK_DIALOG (attributes_dialog));

      if (response == GTK_RESPONSE_CANCEL)
	break;

      if (response == GTK_RESPONSE_OK)
	{
	  int i;
	  /* Copy the values back */
	  for (i=0; i<attributes_list->Number; i++)
	    {
	      if (attributes_list->List[i].name)
		free (attributes_list->List[i].name);
	      if (attributes_list->List[i].value)
		free (attributes_list->List[i].value);
	    }
	  if (attributes_list->Max < attr_num_rows)
	    {
	      int sz = attr_num_rows * sizeof (AttributeType);
	      if (attributes_list->List == NULL)
		attributes_list->List = (AttributeType *) malloc (sz);
	      else
		attributes_list->List = (AttributeType *) realloc (attributes_list->List, sz);
	      attributes_list->Max = attr_num_rows;
	    }
	  for (i=0; i<attr_num_rows; i++)
	    {
	      attributes_list->List[i].name = strdup (gtk_entry_get_text (GTK_ENTRY (attr_row[i].w_name)));
	      attributes_list->List[i].value = strdup (gtk_entry_get_text (GTK_ENTRY (attr_row[i].w_value)));
	      attributes_list->Number = attr_num_rows;
	    }

	  break;
	}

      if (response == GA_RESPONSE_REVERT)
	{
	  /* Revert */
	  ghid_attributes_revert ();
	}

      if (response == GA_RESPONSE_NEW)
	{
	  ghid_attributes_need_rows (attr_num_rows + 1); /* also bumps attr_num_rows */

	  gtk_entry_set_text (GTK_ENTRY (attr_row[attr_num_rows-1].w_name), "");
	  gtk_entry_set_text (GTK_ENTRY (attr_row[attr_num_rows-1].w_value), "");

	  ghid_attr_set_table_size ();
	}
    }

  gtk_widget_destroy (attributes_dialog);
  free (attr_row);
  attr_row = NULL;
}

/* ---------------------------------------------------------------------- */

HID_DRC_GUI ghid_drc_gui = {
  1,				/* log_drc_overview */
  0,				/* log_drc_details */
  ghid_drc_window_reset_message,
  ghid_drc_window_append_violation,
  ghid_drc_window_throw_dialog,
};

extern HID_Attribute *ghid_get_export_options (int *);


/* ------------------------------------------------------------ 
 *
 * Actions specific to the GTK HID follow from here
 *
 */


/* ------------------------------------------------------------ */
static const char about_syntax[] =
"About()";

static const char about_help[] =
N_("Tell the user about this version of PCB.");

/* %start-doc actions About

This just pops up a dialog telling the user which version of
@code{pcb} they're running.

%end-doc */


static int
About (int argc, char **argv, Coord x, Coord y)
{
  ghid_dialog_about ();
  return 0;
}

/* ------------------------------------------------------------ */
static const char getxy_syntax[] =
"GetXY()";

static const char getxy_help[] =
N_("Get a coordinate.");

/*!
 * \brief Get a coordinate to be used by subsequent actions.
 *
 * Any action that always requires a point should have it's need_coord_msg
 * field set to a message to be used when prompting the user for that point.
 * A call to this function isn't required to obtain a coordinate for these
 * actions when they occur in isolation, because hid_actionv() arranges to
 * call gui->get_coords() on their behalf anyway.  Actions for which a
 * coordinate argument is optional, required only conditionally depending on
 * other arguments, or not used at all will have their need_coord_msg field
 * set to NULL.
 *
 * GetXY() exists *only* to request a point for actions that don't always
 * use one, or for interface elements that execute sequences of actions some
 * of which can or must use a point.  In the former case there would not be a
 * correct coordinate available without a prior GetXY() call.  In the latter
 * case an action which only conditionally requires a point might occur earlier
 * in the sequence than any that require a point, or the first action with a
 * non-NULL need_coord_msg field might contain a message inappropriate for the
 * group results of the operation, so GetXY() should be used at the start of
 * the sequence.  It works by setting the effective global point value to be
 * used by the rest of the action sequence.
 *
 * It isn't possible to use multiple GetXY() calls in a single action sequence
 * to obtain distinct points, because all subsequent calls after the first will
 * end up getting the first value.
 *
 */
/* %start-doc actions GetXY

Prompts the user for a coordinate, if one is not already selected.

%end-doc */

static int
GetXY (int argc, char **argv, Coord x, Coord y)
{
  gui->get_coords (argv[0], &x, &y);

  return 0;
}

/* ---------------------------------------------------------------------- */

static int PointCursor (int argc, char **argv, Coord x, Coord y)
{
  if (!ghidgui)
    return 0;

  if (argc > 0)
    ghid_point_cursor ();
  else
    ghid_mode_cursor (Settings.Mode);
  return 0;
}

/* ---------------------------------------------------------------------- */

static int
RouteStylesChanged (int argc, char **argv, Coord x, Coord y)
{
  if (!ghidgui || !ghidgui->route_style_selector)
    return 0;

  ghid_route_style_selector_sync
    (GHID_ROUTE_STYLE_SELECTOR (ghidgui->route_style_selector),
     Settings.LineThickness, Settings.ViaDrillingHole,
     Settings.ViaThickness, Settings.Keepaway);

  return 0;
}

/* ---------------------------------------------------------------------- */

int
PCBChanged (int argc, char **argv, Coord x, Coord y)
{
  if (!ghidgui)
    return 0;

  ghid_window_set_name_label (PCB->Name);

  if (!gport->pixmap)
    return 0;

  if (ghidgui->route_style_selector)
    {
      ghid_route_style_selector_empty
        (GHID_ROUTE_STYLE_SELECTOR (ghidgui->route_style_selector));
      make_route_style_buttons
        (GHID_ROUTE_STYLE_SELECTOR (ghidgui->route_style_selector));
    }
  RouteStylesChanged (0, NULL, 0, 0);

  ghid_port_ranges_scale ();
  ghid_zoom_view_fit ();
  ghid_sync_with_new_layout ();
  return 0;
}

/* ---------------------------------------------------------------------- */

static int
LayerGroupsChanged (int argc, char **argv, Coord x, Coord y)
{
  printf (_("LayerGroupsChanged -- not implemented\n"));
  return 0;
}

/* ---------------------------------------------------------------------- */

static int
LibraryChanged (int argc, char **argv, Coord x, Coord y)
{
  /* No need to show the library window every time it changes...
   *  ghid_library_window_show (&ghid_port, FALSE);
   */
  return 0;
}

/* ---------------------------------------------------------------------- */

static int
Command (int argc, char **argv, Coord x, Coord y)
{
  ghid_handle_user_command (TRUE);
  return 0;
}

/* ---------------------------------------------------------------------- */

static int
Load (int argc, char **argv, Coord x, Coord y)
{
  char *function;
  char *name = NULL;

  static gchar *current_element_dir = NULL;
  static gchar *current_layout_dir = NULL;
  static gchar *current_netlist_dir = NULL;

  /* we've been given the file name */
  if (argc > 1)
    return hid_actionv ("LoadFrom", argc, argv);

  function = argc ? argv[0] : (char *)"Layout";

  if (strcasecmp (function, "Netlist") == 0)
    {
      name = ghid_dialog_file_select_open (_("Load netlist file"),
					   &current_netlist_dir,
					   Settings.FilePath);
    }
  else if (strcasecmp (function, "ElementToBuffer") == 0)
    {
      name = ghid_dialog_file_select_open (_("Load element to buffer"),
					   &current_element_dir,
					   Settings.LibraryTree);
    }
  else if (strcasecmp (function, "LayoutToBuffer") == 0)
    {
      name = ghid_dialog_file_select_open (_("Load layout file to buffer"),
					   &current_layout_dir,
					   Settings.FilePath);
    }
  else if (strcasecmp (function, "Layout") == 0)
    {
      name = ghid_dialog_file_select_open (_("Load layout file"),
					   &current_layout_dir,
					   Settings.FilePath);
    }

  if (name)
    {
      if (Settings.verbose)
      	fprintf (stderr, "%s:  Calling LoadFrom(%s, %s)\n", __FUNCTION__,
		 function, name);
      hid_actionl ("LoadFrom", function, name, NULL);
      g_free (name);
    }

  return 0;
}


/* ---------------------------------------------------------------------- */
static const char save_syntax[] =
"Save()\n"
"Save(Layout|LayoutAs)\n"
"Save(AllConnections|AllUnusedPins|ElementConnections)\n"
"Save(PasteBuffer)";

static const char save_help[] =
N_("Save layout and/or element data to a user-selected file.");

/* %start-doc actions Save

This action is a GUI front-end to the core's @code{SaveTo} action
(@pxref{SaveTo Action}).  If you happen to pass a filename, like
@code{SaveTo}, then @code{SaveTo} is called directly.  Else, the
user is prompted for a filename to save, and then @code{SaveTo} is
called with that filename.

%end-doc */

static int
Save (int argc, char **argv, Coord x, Coord y)
{
  char *function;
  char *name;
  char *prompt;

  static gchar *current_dir = NULL;

  if (argc > 1)
    return hid_actionv ("SaveTo", argc, argv);

  function = argc ? argv[0] : (char *)"Layout";

  if (strcasecmp (function, "Layout") == 0)
    if (PCB->Filename)
      return hid_actionl ("SaveTo", "Layout", PCB->Filename, NULL);

  if (strcasecmp (function, "PasteBuffer") == 0)
    prompt = _("Save element as");
  else
    prompt = _("Save layout as");
  
  name = ghid_dialog_file_select_save (prompt,
				       &current_dir,
				       PCB->Filename, Settings.FilePath);
  
  if (name)
    {
      if (Settings.verbose)
	fprintf (stderr, "%s:  Calling SaveTo(%s, %s)\n", 
		 __FUNCTION__, function, name);
      
      if (strcasecmp (function, "PasteBuffer") == 0)
	hid_actionl ("PasteBuffer", "Save", name, NULL);
      else
	{
	  /* 
	   * if we got this far and the function is Layout, then
	   * we really needed it to be a LayoutAs.  Otherwise 
	   * ActionSaveTo() will ignore the new file name we
	   * just obtained.
	   */
	  if (strcasecmp (function, "Layout") == 0)
	    hid_actionl ("SaveTo", "LayoutAs", name, NULL);
	  else
	    hid_actionl ("SaveTo", function, name, NULL);
	}
      g_free (name);
    }
  else
    {
      return 1;
    }

  return 0;
}

/* ---------------------------------------------------------------------- */
static const char swapsides_syntax[] =
"SwapSides(|v|h|r)";

static const char swapsides_help[] =
N_("Swaps the side of the board you're looking at.");

/* %start-doc actions SwapSides

This action changes the way you view the board.

@table @code

@item v
Flips the board over vertically (up/down).

@item h
Flips the board over horizontally (left/right), like flipping pages in
a book.

@item r
Rotates the board 180 degrees without changing sides.

@end table

If no argument is given, the board isn't moved but the opposite side
is shown.

Normally, this action changes which pads and silk layer are drawn as
true silk, and which are drawn as the "invisible" layer.  It also
determines which solder mask you see.

As a special case, if the layer group for the side you're looking at
is visible and currently active, and the layer group for the opposite
is not visible (i.e. disabled), then this action will also swap which
layer group is visible and active, effectively swapping the ``working
side'' of the board.

%end-doc */


static int
SwapSides (int argc, char **argv, Coord x, Coord y)
{
  int active_group = GetLayerGroupNumberByNumber (LayerStack[0]);
  int top_group = GetLayerGroupNumberBySide (TOP_SIDE);
  int bottom_group = GetLayerGroupNumberBySide (BOTTOM_SIDE);
  bool top_on = LAYER_PTR (PCB->LayerGroups.Entries[top_group][0])->On;
  bool bottom_on = LAYER_PTR (PCB->LayerGroups.Entries[bottom_group][0])->On;

  if (argc > 0)
    {
      switch (argv[0][0]) {
        case 'h':
        case 'H':
          ghid_flip_view (gport->pcb_x, gport->pcb_y, true, false);
          break;
        case 'v':
        case 'V':
          ghid_flip_view (gport->pcb_x, gport->pcb_y, false, true);
          break;
        case 'r':
        case 'R':
          ghid_flip_view (gport->pcb_x, gport->pcb_y, true, true);
          Settings.ShowBottomSide = !Settings.ShowBottomSide; /* Swapped back below */
          break;
        default:
          return 1;
      }
    }

  Settings.ShowBottomSide = !Settings.ShowBottomSide;

  if ((active_group == top_group    && top_on    && !bottom_on) ||
      (active_group == bottom_group && bottom_on && !top_on))
    {
      bool new_bottom_vis = Settings.ShowBottomSide;

      ChangeGroupVisibility (PCB->LayerGroups.Entries[top_group][0],
                             !new_bottom_vis, !new_bottom_vis);
      ChangeGroupVisibility (PCB->LayerGroups.Entries[bottom_group][0],
                             new_bottom_vis, new_bottom_vis);
    }

  layer_process ( NULL, NULL, NULL, LAYER_BUTTON_SILK );
  hid_action ("LayersChanged");

  return 0;
}

/* ------------------------------------------------------------ */

static const char print_syntax[] =
"Print()";

static const char print_help[] =
N_("Print the layout.");

/* %start-doc actions Print

This will find the default printing HID, prompt the user for its
options, and print the layout.

%end-doc */

static int
Print (int argc, char **argv, Coord x, Coord y)
{
  HID **hids;
  int i;
  HID *printer = NULL;

  hids = hid_enumerate ();
  for (i = 0; hids[i]; i++)
    {
      if (hids[i]->printer)
	printer = hids[i];
    }

  if (printer == NULL)
    {
      gui->log (_("Can't find a suitable printer HID\n"));
      return -1;
    }

  /* check if layout is empty */
  if (!IsDataEmpty (PCB->Data))
    {
      ghid_dialog_print (printer);
    }
  else
    gui->log (_("Can't print empty layout\n"));

  return 0;
}


/* ------------------------------------------------------------ */

static HID_Attribute
printer_calibrate_attrs[] = {
  {N_("Enter Values here:"), "",
   HID_Label, 0, 0, {0, 0, 0}, 0, 0},
  {N_("x-calibration"), N_("X scale for calibrating your printer"),
   HID_Real, 0.5, 25, {0, 0, 1.00}, 0, 0},
  {N_("y-calibration"), N_("Y scale for calibrating your printer"),
   HID_Real, 0.5, 25, {0, 0, 1.00}, 0, 0}
};
static HID_Attr_Val printer_calibrate_values[3];

static const char printcalibrate_syntax[] =
"PrintCalibrate()";

static const char printcalibrate_help[] =
N_("Calibrate the printer.");

/* %start-doc actions PrintCalibrate

This will print a calibration page, which you would measure and type
the measurements in, so that future printouts will be more precise.

%end-doc */

static int
PrintCalibrate (int argc, char **argv, Coord x, Coord y)
{
  HID *printer = hid_find_printer ();
  printer->calibrate (0.0, 0.0);

  if (gui->attribute_dialog (printer_calibrate_attrs, 3,
			     printer_calibrate_values,
			     _("Printer Calibration Values"),
			     _("Enter calibration values for your printer")))
    return 1;
  printer->calibrate (printer_calibrate_values[1].real_value,
		      printer_calibrate_values[2].real_value);
  return 0;
}

/* ------------------------------------------------------------ */

static int
Export (int argc, char **argv, Coord x, Coord y)
{

  /* check if layout is empty */
  if (!IsDataEmpty (PCB->Data))
    {
      ghid_dialog_export ();
    }
  else
    gui->log (_("Can't export empty layout\n"));

  return 0;
}

/* ------------------------------------------------------------ */

static int
Benchmark (int argc, char **argv, Coord x, Coord y)
{
  int i = 0;
  time_t start, end;
  GdkDisplay *display;

  display = gdk_drawable_get_display (gport->drawable);

  gdk_display_sync (display);
  time (&start);
  do
    {
      ghid_invalidate_all ();
      gdk_window_process_updates (gtk_widget_get_window (gport->drawing_area),
                                  FALSE);
      time (&end);
      i++;
    }
  while (end - start < 10);

  printf (_("%g redraws per second\n"), (double)i / (double)(end-start));

  return 0;
}

/* ------------------------------------------------------------ */

static const char center_syntax[] =
"Center()\n";

static const char center_help[] =
N_("Moves the pointer to the center of the window.");

/* %start-doc actions Center

Move the pointer to the center of the window, but only if it's
currently within the window already.

%end-doc */

static int
Center(int argc, char **argv, Coord pcb_x, Coord pcb_y)
{
  GdkDisplay *display;
  GdkScreen *screen;
  int offset_x, offset_y;
  int widget_x, widget_y;
  int pointer_x, pointer_y;

  if (argc != 0)
    AFAIL (center);

  /* Aim to put the given x, y PCB coordinates in the center of the widget */
  widget_x = gport->width / 2;
  widget_y = gport->height / 2;

  ghid_pan_view_abs (pcb_x, pcb_y, widget_x, widget_y);

  /* Now move the mouse pointer to the place where the board location
   * actually ended up.
   *
   * XXX: Should only do this if we confirm we are inside our window?
   */

  ghid_pcb_to_event_coords (pcb_x, pcb_y, &widget_x, &widget_y);
  gdk_window_get_origin (gtk_widget_get_window (gport->drawing_area),
                         &offset_x, &offset_y);

  pointer_x = offset_x + widget_x;
  pointer_y = offset_y + widget_y;

  display = gdk_display_get_default ();
  screen = gdk_display_get_default_screen (display);
  gdk_display_warp_pointer (display, screen, pointer_x, pointer_y);

  return 0;
}

/* ------------------------------------------------------------ */
static const char cursor_syntax[] =
"Cursor(Type,DeltaUp,DeltaRight,Units)";

static const char cursor_help[] =
N_("Move the cursor.");

/* %start-doc actions Cursor

This action moves the mouse cursor.  Unlike other actions which take
coordinates, this action's coordinates are always relative to the
user's view of the board.  Thus, a positive @var{DeltaUp} may move the
cursor towards the board origin if the board is inverted.

Type is one of @samp{Pan} or @samp{Warp}.  @samp{Pan} causes the
viewport to move such that the crosshair is under the mouse cursor.
@samp{Warp} causes the mouse cursor to move to be above the crosshair.

@var{Units} can be one of the following:

@table @samp

@item mil
@itemx mm
The cursor is moved by that amount, in board units.

@item grid
The cursor is moved by that many grid points.

@item view
The values are percentages of the viewport's view.  Thus, a pan of
@samp{100} would scroll the viewport by exactly the width of the
current view.

@item board
The values are percentages of the board size.  Thus, a move of
@samp{50,50} moves you halfway across the board.

@end table

%end-doc */

static int
CursorAction(int argc, char **argv, Coord x, Coord y)
{
  UnitList extra_units_x = {
    { "grid",  PCB->Grid, 0 },
    { "view",  gport->view.width, UNIT_PERCENT },
    { "board", PCB->MaxWidth, UNIT_PERCENT },
    { "", 0, 0 }
  };
  UnitList extra_units_y = {
    { "grid",  PCB->Grid, 0 },
    { "view",  gport->view.height, UNIT_PERCENT },
    { "board", PCB->MaxHeight, UNIT_PERCENT },
    { "", 0, 0 }
  };
  int pan_warp = HID_SC_DO_NOTHING;
  double dx, dy;

  if (argc != 4)
    AFAIL (cursor);

  if (strcasecmp (argv[0], "pan") == 0)
    pan_warp = HID_SC_PAN_VIEWPORT;
  else if (strcasecmp (argv[0], "warp") == 0)
    pan_warp = HID_SC_WARP_POINTER;
  else
    AFAIL (cursor);

  dx = GetValueEx (argv[1], argv[3], NULL, extra_units_x, "");
  if (gport->view.flip_x)
    dx = -dx;
  dy = GetValueEx (argv[2], argv[3], NULL, extra_units_y, "");
  if (!gport->view.flip_y)
    dy = -dy;

  EventMoveCrosshair (Crosshair.X + dx, Crosshair.Y + dy);
  gui->set_crosshair (Crosshair.X, Crosshair.Y, pan_warp);

  return 0;
}
/* ------------------------------------------------------------ */

static const char dowindows_syntax[] =
"DoWindows(1|2|3|4|5|6)\n"
"DoWindows(Layout|Library|Log|Netlist|Preferences|DRC)";

static const char dowindows_help[] =
N_("Open various GUI windows.");

/* %start-doc actions DoWindows

@table @code

@item 1
@itemx Layout
Open the layout window.  Since the layout window is always shown
anyway, this has no effect.

@item 2
@itemx Library
Open the library window.

@item 3
@itemx Log
Open the log window.

@item 4
@itemx Netlist
Open the netlist window.

@item 5
@itemx Preferences
Open the preferences window.

@item 6
@itemx DRC
Open the DRC violations window.

@end table

%end-doc */

static int
DoWindows (int argc, char **argv, Coord x, Coord y)
{
  char *a = argc == 1 ? argv[0] : (char *)"";

  if (strcmp (a, "1") == 0 || strcasecmp (a, "Layout") == 0)
    {
    }
  else if (strcmp (a, "2") == 0 || strcasecmp (a, "Library") == 0)
    {
      ghid_library_window_show (gport, TRUE);
    }
  else if (strcmp (a, "3") == 0 || strcasecmp (a, "Log") == 0)
    {
      ghid_log_window_show (TRUE);
    }
  else if (strcmp (a, "4") == 0 || strcasecmp (a, "Netlist") == 0)
    {
      ghid_netlist_window_show (gport, TRUE);
    }
  else if (strcmp (a, "5") == 0 || strcasecmp (a, "Preferences") == 0)
    {
      ghid_config_window_show ();
    }
  else if (strcmp (a, "6") == 0 || strcasecmp (a, "DRC") == 0)
    {
      ghid_drc_window_show (TRUE);
    }
  else
    {
      AFAIL (dowindows);
    }

  return 0;
}

/* ------------------------------------------------------------ */
static const char setunits_syntax[] =
"SetUnits(mm|mil)";

static const char setunits_help[] =
N_("Set the default measurement units.");

/* %start-doc actions SetUnits

@table @code

@item mil
Sets the display units to mils (1/1000 inch).

@item mm
Sets the display units to millimeters.

@end table

%end-doc */

static int
SetUnits (int argc, char **argv, Coord x, Coord y)
{
  const Unit *new_unit;
  if (argc == 0)
    return 0;

  new_unit = get_unit_struct (argv[0]);
  if (new_unit != NULL && new_unit->allow != NO_PRINT)
    {
      Settings.grid_unit = new_unit;
      Settings.increments = get_increments_struct (Settings.grid_unit->family);
      AttributePut (PCB, "PCB::grid::unit", argv[0]);
    }

  ghid_config_handle_units_changed ();

  ghid_set_status_line_label ();

  /* FIXME ?
   * lesstif_sizes_reset ();
   * lesstif_styles_update_values ();
   */
  return 0;
}

/* ------------------------------------------------------------ */
static const char scroll_syntax[] =
"Scroll(up|down|left|right, [div])";

static const char scroll_help[] =
N_("Scroll the viewport.");

/* % start-doc actions Scroll

@item up|down|left|right
Specifies the direction to scroll

@item div
Optional.  Specifies how much to scroll by.  The viewport is scrolled
by 1/div of what is visible, so div = 1 scrolls a whole page. If not
default is given, div=40.

%end-doc */

static int
ScrollAction (int argc, char **argv, Coord x, Coord y)
{
  gdouble dx = 0.0, dy = 0.0;
  int div = 40;

  if (!ghidgui)
    return 0;

  if (argc != 1 && argc != 2)
    AFAIL (scroll);

  if (argc == 2)
    div = atoi(argv[1]);

  if (strcasecmp (argv[0], "up") == 0)
    dy = -gport->view.height / div;
  else if (strcasecmp (argv[0], "down") == 0)
    dy = gport->view.height / div;
  else if (strcasecmp (argv[0], "right") == 0)
    dx = gport->view.width / div;
  else if (strcasecmp (argv[0], "left") == 0)
    dx = -gport->view.width / div;
  else
    AFAIL (scroll);

  ghid_pan_view_rel (dx, dy);

  return 0;
}

/* ------------------------------------------------------------ */
static const char pan_syntax[] =
"Pan([thumb], Mode)";

static const char pan_help[] =
N_("Start or stop panning (Mode = 1 to start, 0 to stop)\n"
"Optional thumb argument is ignored for now in gtk hid.\n");

/* %start-doc actions Pan

Start or stop panning.  To start call with Mode = 1, to stop call with
Mode = 0.

%end-doc */

static int
PanAction (int argc, char **argv, Coord x, Coord y)
{
  int mode;

  if (!ghidgui)
    return 0;

  if (argc != 1 && argc != 2)
    AFAIL (pan);

  if (argc == 1)
    mode = atoi(argv[0]);
  else
    {
      mode = atoi(argv[1]);
      Message (_("The gtk gui currently ignores the optional first argument "
               "to the Pan action.\nFeel free to provide patches.\n"));
    }

  gport->panning = mode;

  return 0;
}

/* ------------------------------------------------------------ */
static const char popup_syntax[] =
"Popup(MenuName, [Button])";

static const char popup_help[] =
N_("Bring up the popup menu specified by @code{MenuName}.\n"
"If called by a mouse event then the mouse button number\n"
"must be specified as the optional second argument.");

/* %start-doc actions Popup

This just pops up the specified menu.  The menu must have been defined
as a named subresource of the Popups resource in the menu resource
file. The second, optional (and ignored) argument represents the mouse
button number which is triggering the popup.

%end-doc */


static int
Popup (int argc, char **argv, Coord x, Coord y)
{
  GtkMenu *menu;

  if (argc != 1 && argc != 2)
    AFAIL (popup);

  menu = ghid_main_menu_get_popup (GHID_MAIN_MENU (ghidgui->menu_bar), argv[0]);
  if (! GTK_IS_MENU (menu))
    {
      Message (_("The specified popup menu \"%s\" has not been defined.\n"), argv[0]);
      return 1;
    }
  else
    {
      ghidgui->in_popup = TRUE;
      gtk_widget_grab_focus (ghid_port.drawing_area);
      gtk_menu_popup (menu, NULL, NULL, NULL, NULL, 0, 
		      gtk_get_current_event_time());
    }
  return 0;
}
/* ------------------------------------------------------------ */
static const char importgui_syntax[] =
"ImportGUI()";

static const char importgui_help[] =
N_("Asks user which schematics to import into PCB.\n");

/* %start-doc actions ImportGUI

Asks user which schematics to import into PCB.

%end-doc */


static int
ImportGUI (int argc, char **argv, Coord x, Coord y)
{
    GSList *names = NULL;
    gchar *name = NULL;
    gchar sname[128];
    static gchar *current_layout_dir = NULL;
    static int I_am_recursing = 0; 
    int rv, nsources;

    if (I_am_recursing)
	return 1;


    names = ghid_dialog_file_select_multiple (_("Load schematics"),
					      &current_layout_dir,
					      Settings.FilePath);

    nsources = 0;
    while (names != NULL)
      {
        name = names->data;

#ifdef DEBUG
        printf("File selected = %s\n", name);
#endif

        snprintf (sname, sizeof (sname), "import::src%d", nsources);
        AttributePut (PCB, sname, name);

        g_free (name);
        nsources++;
        names = g_slist_next (names);
      }
    g_slist_free (names);

    I_am_recursing = 1;
    rv = hid_action ("Import");
    I_am_recursing = 0;

    return rv;
}

/* ------------------------------------------------------------ */
static int
Busy (int argc, char **argv, Coord x, Coord y)
{
  ghid_watch_cursor ();
  return 0;
}

HID_Action ghid_main_action_list[] = {
  {"About", 0, About, about_help, about_syntax},
  {"Benchmark", 0, Benchmark},
  {"Busy", 0, Busy},
  {"Center", N_("Click on a location to center"), Center, center_help, center_syntax},
  {"Command", 0, Command},
  {"Cursor", 0, CursorAction, cursor_help, cursor_syntax},
  {"DoWindows", 0, DoWindows, dowindows_help, dowindows_syntax},
  {"Export", 0, Export},
  {"GetXY", 0, GetXY, getxy_help, getxy_syntax},
  {"ImportGUI", 0, ImportGUI, importgui_help, importgui_syntax},
  {"LayerGroupsChanged", 0, LayerGroupsChanged},
  {"LibraryChanged", 0, LibraryChanged},
  {"Load", 0, Load},
  {"Pan", 0, PanAction, pan_help, pan_syntax},
  {"PCBChanged", 0, PCBChanged},
  {"PointCursor", 0, PointCursor},
  {"Popup", 0, Popup, popup_help, popup_syntax},
  {"Print", 0, Print,
   print_help, print_syntax},
  {"PrintCalibrate", 0, PrintCalibrate,
   printcalibrate_help, printcalibrate_syntax},
  {"RouteStylesChanged", 0, RouteStylesChanged},
  {"Save", 0, Save, save_help, save_syntax},
  {"Scroll", N_("Click on a place to scroll"), ScrollAction, scroll_help, scroll_syntax},
  {"SetUnits", 0, SetUnits, setunits_help, setunits_syntax},
  {"SwapSides", 0, SwapSides, swapsides_help, swapsides_syntax},
  {"Zoom", N_("Click on zoom focus"), Zoom, zoom_help, zoom_syntax}
};

REGISTER_ACTIONS (ghid_main_action_list)


static int
flag_flipx (void *data)
{
  return gport->view.flip_x;
}

static int
flag_flipy (void *data)
{
  return gport->view.flip_y;
}

HID_Flag ghid_main_flag_list[] = {
  {"flip_x", flag_flipx, NULL},
  {"flip_y", flag_flipy, NULL}
};

REGISTER_FLAGS (ghid_main_flag_list)

#include "dolists.h"

/*
 * We will need these for finding the windows installation
 * directory.  Without that we can't find our fonts and
 * footprint libraries.
 */
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winreg.h>
#endif

HID ghid_hid;
HID_DRAW ghid_graphics;

void
hid_gtk_init ()
{
#ifdef WIN32
  char * tmps;
  char * share_dir;
  char *loader_cache;
  size_t buffer_size;
  FILE *loader_file;
#endif

#ifdef WIN32
  tmps = g_win32_get_package_installation_directory_of_module (NULL);
#define REST_OF_PATH G_DIR_SEPARATOR_S "share" G_DIR_SEPARATOR_S PACKAGE
#define REST_OF_CACHE G_DIR_SEPARATOR_S "loaders.cache"
  buffer_size = strlen (tmps) + strlen (REST_OF_PATH) + 1;
  share_dir = (char *)malloc (buffer_size);
  snprintf (share_dir, buffer_size, "%s%s", tmps, REST_OF_PATH);

  /* Point to our gdk-pixbuf loader cache.  */
  buffer_size = strlen (bindir) + strlen (REST_OF_CACHE) + 1;
  loader_cache = (char *)malloc (buffer_size);
  snprintf (loader_cache, buffer_size, "%s%s", bindir, REST_OF_CACHE);
  loader_file = fopen (loader_cache, "r");
  if (loader_file)
    {
      fclose (loader_file);
      g_setenv ("GDK_PIXBUF_MODULE_FILE", loader_cache, TRUE);
    }

  free (tmps);
#undef REST_OF_PATH
  printf ("\"Share\" installation path is \"%s\"\n", share_dir);
  free (share_dir);
#endif

  memset (&ghid_hid, 0, sizeof (HID));
  memset (&ghid_graphics, 0, sizeof (HID_DRAW));

  common_nogui_init (&ghid_hid);
  common_draw_helpers_init (&ghid_graphics);

  ghid_hid.struct_size              = sizeof (HID);
  ghid_hid.name                     = "gtk";
  ghid_hid.description              = "Gtk - The Gimp Toolkit";
  ghid_hid.gui                      = 1;
  ghid_hid.poly_after               = 1;

  ghid_hid.get_export_options       = ghid_get_export_options;
  ghid_hid.do_export                = ghid_do_export;
  ghid_hid.parse_arguments          = ghid_parse_arguments;
  ghid_hid.invalidate_lr            = ghid_invalidate_lr;
  ghid_hid.invalidate_all           = ghid_invalidate_all;
  ghid_hid.notify_crosshair_change  = ghid_notify_crosshair_change;
  ghid_hid.notify_mark_change       = ghid_notify_mark_change;
  ghid_hid.set_layer                = ghid_set_layer;

  ghid_hid.calibrate                = ghid_calibrate;
  ghid_hid.shift_is_pressed         = ghid_shift_is_pressed;
  ghid_hid.control_is_pressed       = ghid_control_is_pressed;
  ghid_hid.mod1_is_pressed          = ghid_mod1_is_pressed,
  ghid_hid.get_coords               = ghid_get_coords;
  ghid_hid.set_crosshair            = ghid_set_crosshair;
  ghid_hid.add_timer                = ghid_add_timer;
  ghid_hid.stop_timer               = ghid_stop_timer;
  ghid_hid.watch_file               = ghid_watch_file;
  ghid_hid.unwatch_file             = ghid_unwatch_file;
  ghid_hid.add_block_hook           = ghid_add_block_hook;
  ghid_hid.stop_block_hook          = ghid_stop_block_hook;

  ghid_hid.log                      = ghid_log;
  ghid_hid.logv                     = ghid_logv;
  ghid_hid.confirm_dialog           = ghid_confirm_dialog;
  ghid_hid.close_confirm_dialog     = ghid_close_confirm_dialog;
  ghid_hid.report_dialog            = ghid_report_dialog;
  ghid_hid.prompt_for               = ghid_prompt_for;
  ghid_hid.fileselect               = ghid_fileselect;
  ghid_hid.attribute_dialog         = ghid_attribute_dialog;
  ghid_hid.show_item                = ghid_show_item;
  ghid_hid.beep                     = ghid_beep;
  ghid_hid.progress                 = ghid_progress;
  ghid_hid.drc_gui                  = &ghid_drc_gui,
  ghid_hid.edit_attributes          = ghid_attributes;

  ghid_hid.request_debug_draw       = ghid_request_debug_draw;
  ghid_hid.flush_debug_draw         = ghid_flush_debug_draw;
  ghid_hid.finish_debug_draw        = ghid_finish_debug_draw;

  ghid_hid.notify_save_pcb          = ghid_notify_save_pcb;
  ghid_hid.notify_filename_changed  = ghid_notify_filename_changed;

  ghid_hid.graphics                 = &ghid_graphics;

  ghid_graphics.make_gc             = ghid_make_gc;
  ghid_graphics.destroy_gc          = ghid_destroy_gc;
  ghid_graphics.use_mask            = ghid_use_mask;
  ghid_graphics.set_color           = ghid_set_color;
  ghid_graphics.set_line_cap        = ghid_set_line_cap;
  ghid_graphics.set_line_width      = ghid_set_line_width;
  ghid_graphics.set_draw_xor        = ghid_set_draw_xor;
  ghid_graphics.draw_line           = ghid_draw_line;
  ghid_graphics.draw_arc            = ghid_draw_arc;
  ghid_graphics.draw_rect           = ghid_draw_rect;
  ghid_graphics.fill_circle         = ghid_fill_circle;
  ghid_graphics.fill_polygon        = ghid_fill_polygon;
  ghid_graphics.fill_rect           = ghid_fill_rect;
  
  ghid_graphics.draw_grid           = ghid_draw_grid;

  ghid_graphics.draw_pcb_polygon    = common_gui_draw_pcb_polygon;

  hid_register_hid (&ghid_hid);
#include "gtk_lists.h"
}
