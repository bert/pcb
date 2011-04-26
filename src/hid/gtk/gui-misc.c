/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/* This file was originally written by Bill Wilson for the PCB Gtk port */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"
#include "crosshair.h"
#include "data.h"
#include "misc.h"
#include "action.h"
#include "set.h"

#include "gui.h"
#include <gdk/gdkkeysyms.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

#define DEFAULT_CURSORSHAPE	GDK_CROSSHAIR

#define CUSTOM_CURSOR_CLOCKWISE		(GDK_LAST_CURSOR + 10)
#define CUSTOM_CURSOR_DRAG			(GDK_LAST_CURSOR + 11)
#define CUSTOM_CURSOR_LOCK			(GDK_LAST_CURSOR + 12)

#define ICON_X_HOT 8
#define ICON_Y_HOT 8


GdkPixmap *XC_clock_source, *XC_clock_mask,
  *XC_hand_source, *XC_hand_mask, *XC_lock_source, *XC_lock_mask;


static GdkCursorType oldCursor;

void
ghid_status_line_set_text (const gchar * text)
{
  if (ghidgui->command_entry_status_line_active)
    return;

  ghid_label_set_markup (ghidgui->status_line_label, text);
}

void
ghid_cursor_position_label_set_text (gchar * text)
{
  ghid_label_set_markup (ghidgui->cursor_position_absolute_label, text);  
}

void
ghid_cursor_position_relative_label_set_text (gchar * text)
{
  ghid_label_set_markup (ghidgui->cursor_position_relative_label, text);
}

void
ghid_size_increment_get_value (const gchar * saction, gchar ** value,
			       gchar ** units)
{
  gdouble increment;
  gchar *fmt;
  static gchar s_buf[64];

  increment = Settings.grid_units_mm
    ? Settings.size_increment_mm : Settings.size_increment_mil;
  fmt = (*saction == '+') ? (gchar *)"+%f" : (gchar *)"-%f";
  snprintf (s_buf, sizeof (s_buf), fmt, increment);
  *value = s_buf;
  *units = Settings.grid_units_mm ? (gchar *)"mm" : (gchar *)"mil";
}

void
ghid_line_increment_get_value (const gchar * saction, gchar ** value,
			       gchar ** units)
{
  gdouble increment;
  gchar *fmt;
  static gchar s_buf[64];

  increment = Settings.grid_units_mm
    ? Settings.line_increment_mm : Settings.line_increment_mil;
  fmt = (*saction == '+') ? (gchar *)"+%f" : (gchar *)"-%f";
  snprintf (s_buf, sizeof (s_buf), fmt, increment);
  *value = s_buf;
  *units = Settings.grid_units_mm ? (gchar *)"mm" : (gchar *)"mil";
}

void
ghid_clear_increment_get_value (const gchar * saction, gchar ** value,
				gchar ** units)
{
  gdouble increment;
  gchar *fmt;
  static gchar s_buf[64];

  increment = Settings.grid_units_mm
    ? Settings.clear_increment_mm : Settings.clear_increment_mil;
  fmt = (*saction == '+') ? (gchar *)"+%f" : (gchar *)"-%f";
  snprintf (s_buf, sizeof (s_buf), fmt, increment);
  *value = s_buf;
  *units = Settings.grid_units_mm ? (gchar *)"mm" : (gchar *)"mil";
}


static GdkCursorType
gport_set_cursor (GdkCursorType shape)
{
  GdkCursorType old_shape = gport->X_cursor_shape;
  GdkColor fg = { 0, 65535, 65535, 65535 };	/* white */
  GdkColor bg = { 0, 0, 0, 0 };	/* black */

  if (!gport->drawing_area || !gport->drawing_area->window)
    return (GdkCursorType)0;
  if (gport->X_cursor_shape == shape)
    return shape;

  /* check if window exists to prevent from fatal errors */
  if (gport->drawing_area->window)
    {
      gport->X_cursor_shape = shape;
      if (shape > GDK_LAST_CURSOR)
	{
	  if (shape == CUSTOM_CURSOR_CLOCKWISE)
	    gport->X_cursor =
	      gdk_cursor_new_from_pixmap (XC_clock_source, XC_clock_mask, &fg,
					  &bg, ICON_X_HOT, ICON_Y_HOT);
	  else if (shape == CUSTOM_CURSOR_DRAG)
	    gport->X_cursor =
	      gdk_cursor_new_from_pixmap (XC_hand_source, XC_hand_mask, &fg,
					  &bg, ICON_X_HOT, ICON_Y_HOT);
	  else if (shape == CUSTOM_CURSOR_LOCK)
	    gport->X_cursor =
	      gdk_cursor_new_from_pixmap (XC_lock_source, XC_lock_mask, &fg,
					  &bg, ICON_X_HOT, ICON_Y_HOT);
	}
      else
	gport->X_cursor = gdk_cursor_new (shape);

      gdk_window_set_cursor (gport->drawing_area->window, gport->X_cursor);
      gdk_cursor_unref (gport->X_cursor);

      return (old_shape);
    }
  return (DEFAULT_CURSORSHAPE);
}

void
ghid_point_cursor (void)
{
  oldCursor = gport_set_cursor (GDK_DRAPED_BOX);
}

void
ghid_hand_cursor (void)
{
  oldCursor = gport_set_cursor (GDK_HAND2);
}

void
ghid_watch_cursor (void)
{
  GdkCursorType tmp;

  tmp = gport_set_cursor (GDK_WATCH);
  if (tmp != GDK_WATCH)
    oldCursor = tmp;
}

void
ghid_mode_cursor (int Mode)
{
  switch (Mode)
    {
    case NO_MODE:
      gport_set_cursor ((GdkCursorType)CUSTOM_CURSOR_DRAG);
      break;

    case VIA_MODE:
      gport_set_cursor (GDK_ARROW);
      break;

    case LINE_MODE:
      gport_set_cursor (GDK_PENCIL);
      break;

    case ARC_MODE:
      gport_set_cursor (GDK_QUESTION_ARROW);
      break;

    case ARROW_MODE:
      gport_set_cursor (GDK_LEFT_PTR);
      break;

    case POLYGON_MODE:
    case POLYGONHOLE_MODE:
      gport_set_cursor (GDK_SB_UP_ARROW);
      break;

    case PASTEBUFFER_MODE:
      gport_set_cursor (GDK_HAND1);
      break;

    case TEXT_MODE:
      gport_set_cursor (GDK_XTERM);
      break;

    case RECTANGLE_MODE:
      gport_set_cursor (GDK_UL_ANGLE);
      break;

    case THERMAL_MODE:
      gport_set_cursor (GDK_IRON_CROSS);
      break;

    case REMOVE_MODE:
      gport_set_cursor (GDK_PIRATE);
      break;

    case ROTATE_MODE:
      if (ghid_shift_is_pressed ())
	gport_set_cursor ((GdkCursorType)CUSTOM_CURSOR_CLOCKWISE);
      else
	gport_set_cursor (GDK_EXCHANGE);
      break;

    case COPY_MODE:
    case MOVE_MODE:
      gport_set_cursor (GDK_CROSSHAIR);
      break;

    case INSERTPOINT_MODE:
      gport_set_cursor (GDK_DOTBOX);
      break;

    case LOCK_MODE:
      gport_set_cursor ((GdkCursorType)CUSTOM_CURSOR_LOCK);
    }
}

void
ghid_corner_cursor (void)
{
  GdkCursorType shape;

  if (Crosshair.Y <= Crosshair.AttachedBox.Point1.Y)
    shape = (Crosshair.X >= Crosshair.AttachedBox.Point1.X) ?
      GDK_UR_ANGLE : GDK_UL_ANGLE;
  else
    shape = (Crosshair.X >= Crosshair.AttachedBox.Point1.X) ?
      GDK_LR_ANGLE : GDK_LL_ANGLE;
  if (gport->X_cursor_shape != shape)
    gport_set_cursor (shape);
}

void
ghid_restore_cursor (void)
{
  gport_set_cursor (oldCursor);
}



  /* =============================================================== */
static gboolean got_location;

  /* If user hits a key instead of the mouse button, we'll abort unless
     |  it's one of the cursor keys.  Move the layout if a cursor key.
   */
static gboolean
loop_key_press_cb (GtkWidget * drawing_area, GdkEventKey * kev,
		   GMainLoop ** loop)
{
  ModifierKeysState mk;
  GdkModifierType state;
  gint ksym = kev->keyval;

  if (ghid_is_modifier_key_sym (ksym))
    return TRUE;
  state = (GdkModifierType) (kev->state);
  mk = ghid_modifier_keys_state (&state);

  /* Duplicate the cursor key actions in gui-output-events.c
   */
  switch (ksym)
    {
    case GDK_Up:
      if (mk == CONTROL_PRESSED)
	{
	  hid_actionl ("Display", "Scroll", "8", NULL);
	  hid_actionl ("Display", "Scroll", "0", NULL);
	}
      else if (mk == SHIFT_PRESSED)
	hid_actionl ("MovePointer", "0", "-10", NULL);
      else if (mk == NONE_PRESSED)
	hid_actionl ("MovePointer", "0", "-1", NULL);
      break;

    case GDK_Down:
      if (mk == CONTROL_PRESSED)
	{
	  hid_actionl ("Display", "Scroll", "2", NULL);
	  hid_actionl ("Display", "Scroll", "0", NULL);
	}
      else if (mk == SHIFT_PRESSED)
	hid_actionl ("MovePointer", "0", "10", NULL);
      else if (mk == NONE_PRESSED)
	hid_actionl ("MovePointer", "0", "1", NULL);
      break;

    case GDK_Left:
      if (mk == CONTROL_PRESSED)
	{
	  hid_actionl ("Display", "Scroll", "4", NULL);
	  hid_actionl ("Display", "Scroll", "0", NULL);
	}
      else if (mk == SHIFT_PRESSED)
	hid_actionl ("MovePointer", "-10", "0", NULL);
      else if (mk == NONE_PRESSED)
	hid_actionl ("MovePointer", "-1", "0", NULL);
      break;

    case GDK_Right:
      if (mk == CONTROL_PRESSED)
	{
	  hid_actionl ("Display", "Scroll", "6", NULL);
	  hid_actionl ("Display", "Scroll", "0", NULL);
	}
      else if (mk == SHIFT_PRESSED)
	hid_actionl ("MovePointer", "10", "0", NULL);
      else if (mk == NONE_PRESSED)
	hid_actionl ("MovePointer", "1", "0", NULL);
      break;

    case GDK_Return:		/* Accept cursor location */
      if (g_main_loop_is_running (*loop))
	g_main_loop_quit (*loop);
      break;

    default:			/* Abort */
      got_location = FALSE;
      if (g_main_loop_is_running (*loop))
	g_main_loop_quit (*loop);
      break;
    }
  return TRUE;
}

  /* User hit a mouse button in the Output drawing area, so quit the loop
     |  and the cursor values when the button was pressed will be used.
   */
static gboolean
loop_button_press_cb (GtkWidget * drawing_area, GdkEventButton * ev,
		      GMainLoop ** loop)
{
  if (g_main_loop_is_running (*loop))
    g_main_loop_quit (*loop);
  ghid_note_event_location (ev);
  return TRUE;
}

  /* Run a glib GMainLoop which intercepts key and mouse button events from
     |  the top level loop.  When a mouse or key is hit in the Output drawing
     |  area, quit the loop so the top level loop can continue and use the
     |  the mouse pointer coordinates at the time of the mouse button event.
   */
static gboolean
run_get_location_loop (const gchar * message)
{
  GMainLoop *loop;
  gulong button_handler, key_handler;
  gint oldObjState, oldLineState, oldBoxState;

  ghid_status_line_set_text (message);

  oldObjState = Crosshair.AttachedObject.State;
  oldLineState = Crosshair.AttachedLine.State;
  oldBoxState = Crosshair.AttachedBox.State;
  notify_crosshair_change (false);
  Crosshair.AttachedObject.State = STATE_FIRST;
  Crosshair.AttachedLine.State = STATE_FIRST;
  Crosshair.AttachedBox.State = STATE_FIRST;
  ghid_hand_cursor ();
  notify_crosshair_change (true);

  /* Stop the top level GMainLoop from getting user input from keyboard
     |  and mouse so we can install our own handlers here.  Also set the
     |  control interface insensitive so all the user can do is hit a key
     |  or mouse button in the Output drawing area.
   */
  ghid_interface_input_signals_disconnect ();
  ghid_interface_set_sensitive (FALSE);

  got_location = TRUE;		/* Will be unset by hitting most keys */
  button_handler =
    g_signal_connect (G_OBJECT (gport->drawing_area),
		      "button_press_event",
		      G_CALLBACK (loop_button_press_cb), &loop);
  key_handler =
    g_signal_connect (G_OBJECT (gport->top_window),
		      "key_press_event",
		      G_CALLBACK (loop_key_press_cb), &loop);

  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);

  g_main_loop_unref (loop);

  g_signal_handler_disconnect (gport->drawing_area, button_handler);
  g_signal_handler_disconnect (gport->top_window, key_handler);

  ghid_interface_input_signals_connect ();	/* return to normal */
  ghid_interface_set_sensitive (TRUE);

  notify_crosshair_change (false);
  Crosshair.AttachedObject.State = oldObjState;
  Crosshair.AttachedLine.State = oldLineState;
  Crosshair.AttachedBox.State = oldBoxState;
  notify_crosshair_change (true);
  ghid_restore_cursor ();

  ghid_set_status_line_label ();

  return got_location;
}



/* ---------------------------------------------------------------------------*/
void
ghid_get_user_xy (const char *msg)
{
  run_get_location_loop (msg);
}

  /* XXX The abort dialog isn't implemented yet in the Gtk port
   */
void
ghid_create_abort_dialog (char *msg)
{
}

gboolean
ghid_check_abort (void)
{
  return FALSE;			/* Abort isn't implemented, so never abort */
}

void
ghid_end_abort (void)
{
}

void
ghid_get_pointer (int *x, int *y)
{
  gint xp, yp;

  gdk_window_get_pointer (gport->drawing_area->window, &xp, &yp, NULL);
  if (x)
    *x = xp;
  if (y)
    *y = yp;
}

/* ---------------------------------------------------------------------------
 * output of status line
 */
void
ghid_set_status_line_label (void)
{
  gchar text[512];

  if (!Settings.grid_units_mm)
    snprintf (text, sizeof (text),
	      _("<b>view</b>=%s  "
		"<b>grid</b>=%.1f:%i  "
		"%s%s  "
		"<b>line</b>=%.1f  "
		"<b>via</b>=%.1f(%.1f)  %s"
		"<b>clearance</b>=%.1f  "
		"<b>text</b>=%i%%  "
		"<b>buffer</b>=#%i"),
	      Settings.ShowSolderSide ? _("solder") : _("component"),
	      PCB->Grid / 100.0,
	      (int) Settings.GridFactor,
	      TEST_FLAG (ALLDIRECTIONFLAG, PCB) ? "all" :
	      (PCB->Clipping == 0 ? "45" :
	       (PCB->Clipping == 1 ? "45_/" : "45\\_")),
	      TEST_FLAG (RUBBERBANDFLAG, PCB) ? ",R  " : "  ",
	      Settings.LineThickness / 100.0,
	      Settings.ViaThickness / 100.0,
	      Settings.ViaDrillingHole / 100.0,
	      ghidgui->compact_horizontal ? "\n" : "",
	      Settings.Keepaway / 100.0,
	      Settings.TextScale, Settings.BufferNumber + 1);
  else
    snprintf (text, sizeof (text),
	      _("<b>view</b>=%s  "
		"<b>grid</b>=%5.3f:%i  "
		"%s%s  "
		"<b>line</b>=%5.3f  "
		"<b>via</b>=%5.3f(%5.3f)  %s"
		"<b>clearance</b>=%5.3f  "
		"<b>text</b>=%i%%  "
		"<b>buffer</b>=#%i"),
	      Settings.ShowSolderSide ? _("solder") : _("component"),
	      PCB->Grid * COOR_TO_MM, (int) Settings.GridFactor,
	      TEST_FLAG (ALLDIRECTIONFLAG, PCB) ? "all" :
	      (PCB->Clipping == 0 ? "45" :
	       (PCB->Clipping == 1 ? "45_/" : "45\\_")),
	      TEST_FLAG (RUBBERBANDFLAG, PCB) ? ",R  " : "  ",
	      Settings.LineThickness * COOR_TO_MM,
	      Settings.ViaThickness * COOR_TO_MM,
	      Settings.ViaDrillingHole * COOR_TO_MM,
	      ghidgui->compact_horizontal ? "\n" : "",
	      Settings.Keepaway * COOR_TO_MM,
	      Settings.TextScale, Settings.BufferNumber + 1);

  ghid_status_line_set_text (text);
}

/* returns an auxiliary value needed to adjust mm grid.
   the adjustment is needed to prevent ..99 tails in position labels.

   All these are a workaround to precision lost
   because of double->integer transform
   while fitting Crosshair to grid in crosshair.c

   There is another workaround: report mm dimensions with %.2f, like
   in the Lesstif hid; but this reduces the information */
static double
ghid_get_grid_factor(void)
{
  /* when grid units are mm, they shall be an integer of
     1 mm / grid_scale */
  const int grid_scale = 10000; /* metric grid step is .1 um */
  double factor, rounded_factor;

  /* adjustment is not needed for inches
     probably because x/100 is always 'integer' enough */
  if (!Settings.grid_units_mm)
    return -1;

  factor = PCB->Grid * COOR_TO_MM * grid_scale;
  rounded_factor = floor (factor + .5);

  /* check whether the grid is actually metric
     (as of Feb 2011, Settings.grid_units_mm may indicate just that
      the _displayed_ units are mm) */
  if (fabs (factor - rounded_factor) > 1e-3)
    return -1;

  return rounded_factor / grid_scale;
}
/* transforms a pcb coordinate to selected units
   adjusted to the nearest grid point for mm grid */
static double
ghid_grid_pcb_to_units (double x, double grid_factor)
{
  double x_scaled = (Settings.grid_units_mm? COOR_TO_MM: 1./100) * x;
  double nearest_gridpoint;

  if (grid_factor < 0)
    return x_scaled;

  x *= COOR_TO_MM;

  nearest_gridpoint = floor (x / grid_factor + .5);
  /* honour snapping to an unaligned object */
  if (fabs (nearest_gridpoint * grid_factor - x) > COOR_TO_MM)
    return x_scaled;
  /* without mm-adjusted grid_factor
     (return floor (x / PCB->Grid + .5) *  PCB->Grid * COOR_TO_MM),
     the round-off errors redintroduce the bug for 0.1 or 0.05 mm grid
     at coordinates more than 1500 mm.
     grid_factor makes the stuff work at least up to 254 m,
     which is 100 times more than default maximum board size as of Feb 2011. */
  return nearest_gridpoint * grid_factor;
}

/* ---------------------------------------------------------------------------
 * output of cursor position
 */
void
ghid_set_cursor_position_labels (void)
{
  gchar text[128];
  int prec = Settings.grid_units_mm ? 4: 2;
  double grid_factor = ghid_get_grid_factor();

  if (Marked.status)
    {
      double dx, dy, r, a;

      dx = ghid_grid_pcb_to_units (Crosshair.X - Marked.X, grid_factor);
      dy = ghid_grid_pcb_to_units (Crosshair.Y - Marked.Y, grid_factor);
      r = sqrt (dx * dx + dy * dy);
      a = atan2 (dy, dx) * RAD_TO_DEG;
      snprintf (text, sizeof (text), "r %-.*f; phi %-.1f; %-.*f %-.*f",
		prec, r, a, prec, dx, prec, dy);
      ghid_cursor_position_relative_label_set_text (text);
    }
  else
    ghid_cursor_position_relative_label_set_text ("r __.__; phi __._; __.__ __.__");

  snprintf (text, sizeof (text), "%-.*f %-.*f",
              prec, ghid_grid_pcb_to_units (Crosshair.X, grid_factor),
              prec, ghid_grid_pcb_to_units (Crosshair.Y, grid_factor));

  ghid_cursor_position_label_set_text (text);
}
