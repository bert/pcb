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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* This file written by Bill Wilson for the PCB Gtk port
*/


#include "global.h"
#include "crosshair.h"
#include "data.h"
#include "misc.h"
#include "action.h"
#include "set.h"

#include "gui.h"
#include <gdk/gdkkeysyms.h>


#define DEFAULT_CURSORSHAPE	GDK_CROSSHAIR

#define CUSTOM_CURSOR_CLOCKWISE		(GDK_LAST_CURSOR + 10)
#define CUSTOM_CURSOR_DRAG			(GDK_LAST_CURSOR + 11)
#define CUSTOM_CURSOR_LOCK			(GDK_LAST_CURSOR + 12)

#define ICON_X_HOT 8
#define ICON_Y_HOT 8

static GdkCursorType oldCursor;


void
gui_status_line_set_text(gchar *text)
	{
	if (gui->command_entry_status_line_active)
		return;

	gui_label_set_markup(gui->status_line_label, text);
	}

void
gui_cursor_position_label_set_text(gchar *text)
	{
	gui_label_set_markup(gui->cursor_position_absolute_label, text);
	}

void
gui_cursor_position_relative_label_set_text(gchar *text)
	{
	gui_label_set_markup(gui->cursor_position_relative_label, text);
	}

void
gui_size_increment_get_value(const gchar *saction, gchar **value,
				gchar **units)
	{
	gdouble			increment;
	gchar			*fmt;
	static gchar	s_buf[64];

	increment = Settings.grid_units_mm
				? Settings.size_increment_mm : Settings.size_increment_mil;
	fmt = (*saction == '+') ? "+%f" : "-%f";
	snprintf(s_buf, sizeof(s_buf), fmt, increment);
	*value = s_buf;
	*units = Settings.grid_units_mm ? "mm" : "mil";
	}
	
void
gui_line_increment_get_value(const gchar *saction, gchar **value,
				gchar **units)
	{
	gdouble			increment;
	gchar			*fmt;
	static gchar	s_buf[64];

	increment = Settings.grid_units_mm
				? Settings.line_increment_mm : Settings.line_increment_mil;
	fmt = (*saction == '+') ? "+%f" : "-%f";
	snprintf(s_buf, sizeof(s_buf), fmt, increment);
	*value = s_buf;
	*units = Settings.grid_units_mm ? "mm" : "mil";
	}
	
void
gui_clear_increment_get_value(const gchar *saction, gchar **value,
				gchar **units)
	{
	gdouble			increment;
	gchar			*fmt;
	static gchar	s_buf[64];

	increment = Settings.grid_units_mm
				? Settings.clear_increment_mm : Settings.clear_increment_mil;
	fmt = (*saction == '+') ? "+%f" : "-%f";
	snprintf(s_buf, sizeof(s_buf), fmt, increment);
	*value = s_buf;
	*units = Settings.grid_units_mm ? "mm" : "mil";
	}
	

static GdkCursorType
output_window_set_cursor(GdkCursorType Shape)
	{
	GdkCursorType	old_shape = Output.XCursorShape;
	GdkColor		fg = { 0, 65535, 65535, 65535 }; /* white */
	GdkColor		bg = { 0, 0, 0, 0 };			/* black */

	if (Output.XCursorShape == Shape)
		return Shape;

	/* check if window exists to prevent from fatal errors */
	if (Output.drawing_area->window)
		{
		Output.XCursorShape = Shape;
		if (Shape > GDK_LAST_CURSOR)
			{
			if (Shape == CUSTOM_CURSOR_CLOCKWISE)
				Output.XCursor = gdk_cursor_new_from_pixmap(
						XC_clock_source, XC_clock_mask,
						&fg, &bg, ICON_X_HOT, ICON_Y_HOT);
			else if (Shape == CUSTOM_CURSOR_DRAG)
				Output.XCursor = gdk_cursor_new_from_pixmap(
						XC_hand_source, XC_hand_mask,
						&fg, &bg, ICON_X_HOT, ICON_Y_HOT);
			else if (Shape == CUSTOM_CURSOR_LOCK)
				Output.XCursor = gdk_cursor_new_from_pixmap(
						XC_lock_source, XC_lock_mask,
						&fg, &bg, ICON_X_HOT, ICON_Y_HOT);
			}
		else
			Output.XCursor = gdk_cursor_new(Shape);

		gdk_window_set_cursor(Output.drawing_area->window, Output.XCursor);
		gdk_cursor_unref(Output.XCursor);

		return (old_shape);
		}
	return (DEFAULT_CURSORSHAPE);
	}

void
gui_hand_cursor(void)
	{
	oldCursor = output_window_set_cursor (GDK_HAND2);
	}

void
gui_watch_cursor (void)
	{
	GdkCursorType tmp;

	tmp = output_window_set_cursor (GDK_WATCH);
	if (tmp != GDK_WATCH)
		oldCursor = tmp;
	}

void
gui_mode_cursor (int Mode)
	{
	switch (Mode)
		{
		case NO_MODE:
			output_window_set_cursor (CUSTOM_CURSOR_DRAG);
		break;

		case VIA_MODE:
			output_window_set_cursor (GDK_ARROW);
		break;

		case LINE_MODE:
			output_window_set_cursor (GDK_PENCIL);
		break;

		case ARC_MODE:
			output_window_set_cursor (GDK_QUESTION_ARROW);
		break;

		case ARROW_MODE:
			output_window_set_cursor (GDK_LEFT_PTR);
		break;

		case POLYGON_MODE:
			output_window_set_cursor (GDK_SB_UP_ARROW);
		break;

		case PASTEBUFFER_MODE:
			output_window_set_cursor (GDK_HAND1);
		break;

		case TEXT_MODE:
			output_window_set_cursor (GDK_XTERM);
		break;

		case RECTANGLE_MODE:
			output_window_set_cursor (GDK_UL_ANGLE);
		break;

		case THERMAL_MODE:
			output_window_set_cursor (GDK_IRON_CROSS);
		break;

		case REMOVE_MODE:
			output_window_set_cursor (GDK_PIRATE);
		break;

		case ROTATE_MODE:
			if (gui_shift_is_pressed())
				output_window_set_cursor (CUSTOM_CURSOR_CLOCKWISE);
			else
				output_window_set_cursor (GDK_EXCHANGE);
		break;

		case COPY_MODE:
		case MOVE_MODE:
			output_window_set_cursor (GDK_CROSSHAIR);
		break;

		case INSERTPOINT_MODE:
			output_window_set_cursor (GDK_DOTBOX);
		break;

		case LOCK_MODE:
			output_window_set_cursor (CUSTOM_CURSOR_LOCK);
		}
	}

void
gui_corner_cursor(void)
	{
	GdkCursorType shape;

	if (Crosshair.Y <= Crosshair.AttachedBox.Point1.Y)
		shape = (Crosshair.X >= Crosshair.AttachedBox.Point1.X) ?
					GDK_UR_ANGLE : GDK_UL_ANGLE;
	else
		shape = (Crosshair.X >= Crosshair.AttachedBox.Point1.X) ?
					GDK_LR_ANGLE : GDK_LL_ANGLE;
	if (Output.XCursorShape != shape)
		output_window_set_cursor (shape);
	}

void
gui_restore_cursor(void)
	{
	output_window_set_cursor(oldCursor);
	}



  /* =============================================================== */
static gboolean	got_location;

  /* If user hits a key instead of the mouse button, we'll abort unless
  |  it's one of the cursor keys.  Move the layout if a cursor key.
  */
static gboolean
loop_key_press_cb(GtkWidget *drawing_area, GdkEventKey *kev,
			GMainLoop **loop)
	{
	ModifierKeysState	mk;
	gint				ksym = kev->keyval;

	if (gui_is_modifier_key_sym(ksym))
		return TRUE;
	mk = gui_modifier_keys_state();

	/* Duplicate the cursor key actions in gui-output-events.c
	*/
	switch (ksym)
		{
		case GDK_Up:
			if (mk == CONTROL_PRESSED)
				{
				ActionDisplay("Scroll", "8");
				ActionDisplay("Scroll", "0");
				}
			else if (mk == SHIFT_PRESSED)
				ActionMovePointer("0", "-10");
			else if (mk == NONE_PRESSED)
				ActionMovePointer("0", "-1");
			break;

		case GDK_Down:
			if (mk == CONTROL_PRESSED)
				{
				ActionDisplay("Scroll", "2");
				ActionDisplay("Scroll", "0");
				}
			else if (mk == SHIFT_PRESSED)
				ActionMovePointer("0", "10");
			else if (mk == NONE_PRESSED)
				ActionMovePointer("0", "1");
			break;

		case GDK_Left:
			if (mk == CONTROL_PRESSED)
				{
				ActionDisplay("Scroll", "4");
				ActionDisplay("Scroll", "0");
				}
			else if (mk == SHIFT_PRESSED)
				ActionMovePointer("-10", "0");
			else if (mk == NONE_PRESSED)
				ActionMovePointer("-1", "0");
			break;

		case GDK_Right:
			if (mk == CONTROL_PRESSED)
				{
				ActionDisplay("Scroll", "6");
				ActionDisplay("Scroll", "0");
				}
			else if (mk == SHIFT_PRESSED)
				ActionMovePointer("10", "0");
			else if (mk == NONE_PRESSED)
				ActionMovePointer("1", "0");
			break;

		case GDK_Return:	/* Accept cursor location */
			if (g_main_loop_is_running(*loop))
				g_main_loop_quit(*loop);
			break;

		default:			/* Abort */
			got_location = FALSE;
			if (g_main_loop_is_running(*loop))
				g_main_loop_quit(*loop);
			break;
		}
	return TRUE;
	}

  /* User hit a mouse button in the Output drawing area, so quit the loop
  |  and the cursor values when the button was pressed will be used.
  */
static gboolean
loop_button_press_cb(GtkWidget *drawing_area, GdkEventButton *ev,
			GMainLoop **loop)
	{
	if (g_main_loop_is_running(*loop))
		g_main_loop_quit(*loop);
	return TRUE;
	}

  /* Run a glib GMainLoop which intercepts key and mouse button events from
  |  the top level loop.  When a mouse or key is hit in the Output drawing
  |  area, quit the loop so the top level loop can continue and use the
  |  the mouse pointer coordinates at the time of the mouse button event.
  */
static gboolean
run_get_location_loop(char *message)
	{
	GMainLoop	*loop;
	gulong		button_handler, key_handler;
	gint		oldObjState, oldLineState, oldBoxState;

	gui_status_line_set_text(message);

	oldObjState = Crosshair.AttachedObject.State;
	oldLineState = Crosshair.AttachedLine.State;
	oldBoxState = Crosshair.AttachedBox.State;
	HideCrosshair (True);
	Crosshair.AttachedObject.State = STATE_FIRST;
	Crosshair.AttachedLine.State = STATE_FIRST;
	Crosshair.AttachedBox.State = STATE_FIRST;
	gui_hand_cursor ();
	RestoreCrosshair (True);

	/* Stop the top level GMainLoop from getting user input from keyboard
	|  and mouse so we can install our own handlers here.  Also set the
	|  control interface insensitive so all the user can do is hit a key
	|  or mouse button in the Output drawing area.
	*/
	gui_interface_input_signals_disconnect();
	gui_interface_set_sensitive(FALSE);

	got_location = TRUE; /* Will be unset by hitting most keys */
	button_handler =
			g_signal_connect(G_OBJECT(Output.drawing_area),
						"button_press_event",
						G_CALLBACK(loop_button_press_cb), &loop);
	key_handler =
			g_signal_connect(G_OBJECT(Output.top_window),
						"key_press_event",
						G_CALLBACK(loop_key_press_cb), &loop);

	loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);

	g_main_loop_unref(loop);

	g_signal_handler_disconnect(Output.drawing_area, button_handler);
	g_signal_handler_disconnect(Output.top_window, key_handler);

	gui_interface_input_signals_connect();		/* return to normal */
	gui_interface_set_sensitive(TRUE);

	HideCrosshair (True);
	Crosshair.AttachedObject.State = oldObjState;
	Crosshair.AttachedLine.State = oldLineState;
	Crosshair.AttachedBox.State = oldBoxState;
	RestoreCrosshair (True);
	gui_restore_cursor ();

	set_status_line_label();

	return got_location;
	}



/* ---------------------------------------------------------------------------*/

gboolean
ActionGetLocation(gchar *arg)
	{
	return run_get_location_loop(arg);
	}

  /* ACTION(GetXY,ActionGetLocation)
  |  The interactive form run from command.c  Not sure if it makes
  |  sense to run this from there, but in any case, return values
  |  can't be acted on from there.
  */
void
ActionGetXY(gchar *arg)
	{
	run_get_location_loop(arg);
	}


  /* XXX The abort dialog isn't implemented yet in the Gtk port
  */
void
gui_create_abort_dialog (char *msg)
	{
	}

gboolean
gui_check_abort(void)
	{
	return FALSE;		/* Abort isn't implemented, so never abort */
	}

void
gui_end_abort (void)
	{
	}

void
gui_get_pointer(int *x, int *y)
	{
	gint	xp, yp;

	gdk_window_get_pointer(Output.drawing_area->window, &xp, &yp, NULL);
	if (x)
		*x = xp;
	if (y)
		*y = yp;
	}

void
gui_beep(int loud)
	{
	gdk_display_beep(gtk_widget_get_display(Output.top_window));
	}
