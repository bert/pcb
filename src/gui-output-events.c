/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* This file written by Bill Wilson for the PCB Gtk port
*/

#include "gui.h"
#include <gdk/gdkkeysyms.h>

#include "action.h"
#include "crosshair.h"
#include "draw.h"
#include "error.h"
#include "output.h"
#include "misc.h"
#include "set.h"



static gint			x_scroll_timer_id = 0,
					y_scroll_timer_id = 0;
static gint			x_scroll_delta,
					y_scroll_delta;


void
gui_output_positioners_set_values(gint x_value, gint y_value)
	{
	GtkAdjustment	*adj;

	/* Avoid a loop, changing the values here can call Pan().
	*/
	gui->adjustment_changed_holdoff = TRUE;

	adj = gtk_range_get_adjustment(GTK_RANGE(gui->h_scrollbar));
	gtk_adjustment_set_value(adj, (gdouble) x_value);

	adj = gtk_range_get_adjustment(GTK_RANGE(gui->v_scrollbar));
	gtk_adjustment_set_value(adj, (gdouble) y_value);

	gui->adjustment_changed_holdoff = FALSE;
	}


  /* Do scrollbar scaling based on current output drawing area size and
  |  overall PCB board size.
  */
void
gui_output_positioners_scale(void)
	{
	GtkAdjustment	*adj;
	gdouble			h_page_size,
					h_upper_size,
					v_page_size,
					v_upper_size;

	/* Update the scrollbars with PCB units.  So Scale the current
	|  drawing area size in pixels to PCB units and that will be
	|  the page size for the Gtk adjustment.
	*/
	h_page_size = (gdouble) TO_PCB(Output.Width);
	v_page_size = (gdouble) TO_PCB(Output.Height);

	/* The adjustment lower value is always zero and the upper value will
	|  be the PCB board dimensions.
	*/
	h_upper_size = (gdouble) PCB->MaxWidth;
	v_upper_size = (gdouble) PCB->MaxHeight;

	if (h_page_size >= h_upper_size)
		h_page_size = h_upper_size;
	if (v_page_size >= v_upper_size)
		v_page_size = v_upper_size;

	/* Set both scrollbar ranges.  The scrollbar values may get updated
	|  when Pan() is called, but we need to emit the "changed" signal here
	|  so the widgets will know about these page_size or upper changes.
	*/
	adj = gtk_range_get_adjustment(GTK_RANGE(gui->h_scrollbar));
	adj->page_size = h_page_size;
	adj->upper = h_upper_size;
	gtk_signal_emit_by_name(GTK_OBJECT(adj), "changed");

	adj = gtk_range_get_adjustment(GTK_RANGE(gui->v_scrollbar));
	adj->page_size = v_page_size;
	adj->upper = v_upper_size;
	gtk_signal_emit_by_name(GTK_OBJECT(adj), "changed");

//	printf("gui_output_positioners_update_ranges: (%.2f) %.2f   (%.2f) %.2f\n",
//	h_page_size, h_upper_size, v_page_size, v_upper_size);

	/* Hiding/showing here will cause this func to be recursively called.
	*/
	if (h_page_size >= h_upper_size)
		gtk_widget_hide(gui->h_scrollbar);
	else
		gtk_widget_show(gui->h_scrollbar);

	if (v_page_size >= v_upper_size)
		gtk_widget_hide(gui->v_scrollbar);
	else
		gtk_widget_show(gui->v_scrollbar);

	/* This may emit the "value_changed" signal on the adjustments.
	*/
	Pan(Xorig, Yorig, FALSE, FALSE);
	}



/* ---------------------------------------------------------------------- 
 * handles all events from PCB drawing area
 */

static void
output_key_press_release_common(void)
	{
	gint	x, y;

	gdk_window_get_pointer(Output.drawing_area->window, &x, &y, NULL);
	HideCrosshair (FALSE);
	FitCrosshairIntoGrid(TO_PCB_X(x), TO_PCB_Y(y));
	AdjustAttachedObjects ();
	RestoreCrosshair (TRUE);
	}

gboolean
gui_output_key_release_cb(GtkWidget *drawing_area, GdkEventKey *kev,
            GtkUIManager *ui)
	{
	gint		ksym = kev->keyval;

	if (gui_is_modifier_key_sym(ksym))
		output_key_press_release_common();

	return FALSE;
	}

  /* Handle user keys in the output drawing area.
  |  Note that there are some menu shortcuts in gui-top-window.c and it's
  |  possible there's overlap with key actions coded here and menu shortcut
  |  actions.  If the menu handles it, we won't see the key and code here
  |  won't be called.  I've commented the cases below, but probably have
  |  missed some.
  |  If a method of allowing user defined keys is implemented, it will have
  |  to deal somehow with the menu shortcuts.
  */
gboolean
gui_output_key_press_cb(GtkWidget *drawing_area,
			GdkEventKey *kev, GtkUIManager *ui)
	{
	ModifierKeysState	mk;
	gchar				*arg, *units;
	gdouble				value;
	gint				ksym = kev->keyval;
	gboolean			handled;

	if (gui_is_modifier_key_sym(ksym))
		output_key_press_release_common();

	mk = gui_modifier_keys_state();

	handled = TRUE;		/* Start off assuming we handle it */
	switch (ksym)
		{
		case GDK_a:
		case GDK_A:
			if (mk == NONE_PRESSED)
				ActionSetSame();
			else
				handled = FALSE;
			break;

		case GDK_b:
		case GDK_B:
			if (mk == NONE_PRESSED)
				ActionFlip("Object");
			else
				handled = FALSE;
			break;

		case GDK_c:
		case GDK_C:
			if (mk == NONE_PRESSED)		/* XXX handled in menu */
				ActionDisplay("Center", "");
			else if (mk == CONTROL_PRESSED)
				{
				ActionPasteBuffer("Clear", "");
				ActionPasteBuffer("AddSelected", "");
				ActionUnselect("All");
				}
			else
				handled = FALSE;
			break;

		case GDK_d:
		case GDK_D:
			if (mk == NONE_PRESSED)
				ActionDisplay("PinOrPadName", "");
			else
				handled = FALSE;
			break;

		case GDK_e:
		case GDK_E:
			if (mk == NONE_PRESSED)		/* XXX handled in menu */
				ActionDeleteRats("AllRats");
			else if (mk == SHIFT_PRESSED)
				ActionDeleteRats("SelectedRats");
			else
				handled = FALSE;
			break;

		case GDK_f:
		case GDK_F:
			if (mk == NONE_PRESSED)
				{
				ActionConnection("Reset");
				ActionConnection("Find");
				}
			else if (mk == SHIFT_PRESSED)
				ActionConnection("Reset");
			else if (mk == CONTROL_PRESSED)
				ActionConnection("Find");
			else
				handled = FALSE;
			break;

		case GDK_g:
		case GDK_G:
			value = Settings.grid_units_mm ?
					Settings.grid_increment_mm : Settings.grid_increment_mil;
			if (mk == NONE_PRESSED)
				arg = g_strdup_printf("+%f", value);
			else if (mk == SHIFT_PRESSED)
				arg = g_strdup_printf("-%f", value);
			else
				{
				handled = FALSE;
				break;
				}
			ActionSetValue("Grid", arg, Settings.grid_units_mm ? "mm" : "mil");
			g_free(arg);

			/* User set grid may no longer match grid setting menu.
			*/
			gui_grid_setting_update_menu_actions();
			break;

		case GDK_h:
		case GDK_H:
			if (mk == CONTROL_PRESSED)
				ActionChangeHole("ToggleObject");
			else if (mk == SHIFT_PRESSED)
				ActionToggleHideName("SelectedElements");
			else if (mk == NONE_PRESSED)
				ActionToggleHideName("Object");
			else
				handled = FALSE;
			break;

		case GDK_j:
		case GDK_J:
			if (mk == SHIFT_PRESSED)
				ActionChangeJoin("SelectedObjects");
			else if (mk == NONE_PRESSED)
				ActionChangeJoin("Object");
			else
				handled = FALSE;
			break;

		case GDK_k:
		case GDK_K:
			if (mk == CONTROL_PRESSED || mk == SHIFT_CONTROL_PRESSED)
				{
				gui_clear_increment_get_value(
						(mk == CONTROL_PRESSED) ? "+" : "-", &arg, &units);
				ActionChangeClearSize("SelectedObjects", arg, units);
				}
			else
				{
				gui_clear_increment_get_value(
						(mk == NONE_PRESSED) ? "+" : "-", &arg, &units);
				ActionChangeClearSize("Object", arg, units);
				}
			break;

		case GDK_l:
		case GDK_L:
			if (mk == SHIFT_PRESSED)
				{
				gui_line_increment_get_value("-", &arg, &units);
				ActionSetValue("LineSize", arg, units);
				}
			else if (mk == NONE_PRESSED)
				{
				gui_line_increment_get_value("+", &arg, &units);
				ActionSetValue("LineSize", arg, units);
				}
			else
				handled = FALSE;
			break;

		case GDK_m:
		case GDK_M:
			if (mk == CONTROL_PRESSED)
				ActionMarkCrosshair("");
			else if (mk == SHIFT_PRESSED)
				ActionMoveToCurrentLayer("SelectedObjects");
			else if (mk == NONE_PRESSED)
				ActionMoveToCurrentLayer("Object");
			else
				handled = FALSE;
			break;

		case GDK_n:
		case GDK_N:
			if (mk == SHIFT_PRESSED)
				ActionAddRats("Close");
			else
				handled = FALSE;
			break;

		case GDK_o:
		case GDK_O:
			if (mk == NONE_PRESSED)		/* XXX handled in menu */
				{
				ActionAtomic("Save");
				ActionDeleteRats("AllRats");
				ActionAtomic("Restore");
				ActionAddRats("AllRats");
				ActionAtomic("Block");
				}
			else if (mk == CONTROL_PRESSED)
				ActionChangeOctagon("Object");
			else if (mk == SHIFT_PRESSED)
				{
				ActionAtomic("Save");
				ActionDeleteRats("AllRats");
				ActionAtomic("Restore");
				ActionAddRats("SelectedRats");
				ActionAtomic("Block");
				}
			else
				handled = FALSE;
			break;

		case GDK_p:
		case GDK_P:
			if (mk == SHIFT_PRESSED)
				ActionPolygon("Close");
			else if (mk == NONE_PRESSED)
				ActionPolygon("PreviousPoint");
			else
				handled = FALSE;
			break;

		case GDK_q:
		case GDK_Q:
			if (mk == NONE_PRESSED)
				ActionChangeSquare("ToggleObject");
			else
				handled = FALSE;
			break;

		case GDK_r:
		case GDK_R:
			if (mk == NONE_PRESSED)		/* XXX handled in menu */
				ActionDisplay("ClearAndRedraw", "");
			else
				handled = FALSE;
			break;

		case GDK_s:
		case GDK_S:
			if (mk == CONTROL_PRESSED || mk == SHIFT_CONTROL_PRESSED)
				{
				gui_size_increment_get_value(
						(mk == CONTROL_PRESSED) ? "+" : "-", &arg, &units);
				ActionChange2ndSize("Object", arg, units);
				}
			else
				{
				gui_size_increment_get_value(
						(mk == NONE_PRESSED) ? "+" : "-", &arg, &units);
				ActionChangeSize("Object", arg, units);
				}
			break;

		case GDK_t:
		case GDK_T:
			if (mk == SHIFT_PRESSED)
				{
				gui_size_increment_get_value("-", &arg, &units);
				ActionSetValue("TextScale", arg, units);
				}
			else if (mk == NONE_PRESSED)
				{
				gui_size_increment_get_value("+", &arg, &units);
				ActionSetValue("TextScale", arg, units);
				}
			else
				handled = FALSE;
			break;

		case GDK_u:
		case GDK_U:
			if (mk == NONE_PRESSED)			/* XXX handled in menu */
				ActionUndo("");
			else
				handled = FALSE;
			break;

		case GDK_v:
		case GDK_V:
			if (mk == NONE_PRESSED)
				ActionSetValue("Zoom", "1000", "");
			else if (mk == MOD1_PRESSED || mk == SHIFT_MOD1_PRESSED)
				{
				gui_size_increment_get_value(
						(mk == MOD1_PRESSED) ? "+" : "-", &arg, &units);
				ActionSetValue("ViaDrillingHole", arg, units);
				}
			else if (mk == CONTROL_PRESSED || mk == SHIFT_CONTROL_PRESSED)
				{
				gui_size_increment_get_value(
						(mk == CONTROL_PRESSED) ? "+" : "-", &arg, &units);
				ActionSetValue("ViaSize", arg, units);
				}
			else
				handled = FALSE;
			break;

		case GDK_w:
		case GDK_W:
			if (mk == SHIFT_PRESSED)
				ActionAddRats("SelectedRats");
			else if (mk == NONE_PRESSED)
				ActionAddRats("AllRats");
			else
				handled = FALSE;
			break;

		case GDK_z:
		case GDK_Z:
			if (mk == NONE_PRESSED)		/* XXX handled in menu */
				ActionSetValue("Zoom", "-1", "");
			else						/* <shift>z handled in menu shortcut */
				handled = FALSE;
			break;

		case GDK_bar:
		case GDK_backslash:
			ActionDisplay("ToggleThindraw", "");
			break;

		case GDK_Tab:		/* Also <shift>b from a menu shortcut */
			ActionSwapSides();
			break;

		case GDK_Escape:
			ActionMode("None");
			break;

		case GDK_space:
			ActionMode("Arrow");
			break;

		case GDK_colon:
			ActionCommand("");
			break;

		case GDK_period:
			ActionCommand("-1");
			break;

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

		case GDK_BackSpace:
		case GDK_Delete:
			if (mk == SHIFT_PRESSED)
				{
				ActionAtomic("Save");
				ActionConnection("Reset");
				ActionAtomic("Restore");
				ActionUnselect("All");
				ActionAtomic("Restore");
				ActionConnection("Find");
				ActionAtomic("Restore");
				ActionSelect("Connection");
				ActionAtomic("Restore");
				ActionRemoveSelected();
				ActionAtomic("Restore");
				ActionConnection("Reset");
				ActionAtomic("Restore");
				ActionUnselect("All");
				ActionAtomic("Block");
				}
			else if (mk == NONE_PRESSED)
				{
				ActionMode("Save");
				ActionMode("Remove");
	            ActionMode("Notify");
	            ActionMode("Restore") ;
				}
			break;

		case GDK_Insert:
			if (mk == NONE_PRESSED)
				ActionMode("InsertPoint");

		case GDK_1:
			if (mk == NONE_PRESSED)
				ActionSwitchDrawingLayer("1");
			else
				handled = FALSE;
			break;
		case GDK_2:
			if (mk == NONE_PRESSED)
				ActionSwitchDrawingLayer("2");
			else
				handled = FALSE;
			break;
		case GDK_3:
			if (mk == NONE_PRESSED)
				ActionSwitchDrawingLayer("3");
			else
				handled = FALSE;
			break;
		case GDK_4:
			if (mk == NONE_PRESSED)
				ActionSwitchDrawingLayer("4");
			else
				handled = FALSE;
			break;
		case GDK_5:
			if (mk == NONE_PRESSED)
				ActionSwitchDrawingLayer("5");
			else
				handled = FALSE;
			break;
		case GDK_6:
			if (mk == NONE_PRESSED)
				ActionSwitchDrawingLayer("6");
			else
				handled = FALSE;
			break;
		case GDK_7:
			if (mk == NONE_PRESSED)
				ActionSwitchDrawingLayer("7");
			else
				handled = FALSE;
			break;
		case GDK_8:
			if (mk == NONE_PRESSED)
				ActionSwitchDrawingLayer("8");
			else
				handled = FALSE;
			break;
		case GDK_9:
			if (mk == NONE_PRESSED)
				ActionSwitchDrawingLayer("9");
			else
				handled = FALSE;
			break;

		case GDK_F1:
			ActionMode("Via");
			break;

		case GDK_F2:
			ActionMode("Line");
			break;

		case GDK_F3:
			ActionMode("Arc");
			break;

		case GDK_F4:
			ActionMode("Text");
			break;

		case GDK_F5:
			ActionMode("Rectangle");
			break;

		case GDK_F6:
			ActionMode("Polygon");
			break;

		case GDK_F7:
			ActionMode("Buffer");
			break;

		case GDK_F8:
			ActionMode("Delete");
			break;

		case GDK_F9:
			ActionMode("Rotate");
			break;

		case GDK_F10:
			ActionMode("Thermal");
			break;

		case GDK_F11:
			ActionMode("Arrow");
			break;

		case GDK_bracketleft:
			ActionMode("Save");
			ActionMode("Arrow");
			ActionMode("Notify");
			break;

		case GDK_bracketright:
			ActionMode("Release");
			ActionMode("Restore");
			break;
		default:
			handled = FALSE;
		}

	return handled;
	}

gboolean
gui_output_button_press_cb(GtkWidget *drawing_area,
			GdkEventButton *ev, GtkUIManager *ui)
	{
	GtkWidget 			*menu = gtk_ui_manager_get_widget(ui, "/PopupMenu");
	ModifierKeysState	mk;

	mk = gui_modifier_keys_state();

	switch (ev->button)
		{
		case 1:
			if (mk == NONE_PRESSED || mk == SHIFT_PRESSED)
				ActionMode("Notify");
			else if (mk == CONTROL_PRESSED)
				{
				ActionMode("Save");
				ActionMode("None");
				ActionMode("Restore");
				ActionMode("Notify");
				}
			else if (mk == SHIFT_CONTROL_PRESSED)
				{
				ActionMode("Save");
				ActionMode("Remove");
				ActionMode("Notify");
				ActionMode("Restore");
				}
			break;

		case 2:
			if (mk == NONE_PRESSED || mk == SHIFT_PRESSED)
				{
				ActionMode("Save");
				ActionMode("Stroke");
				}
			else if (mk == CONTROL_PRESSED)
				{
				ActionMode("Save");
				ActionMode("Copy");
				ActionMode("Notify");
				}
			else if (mk == SHIFT_CONTROL_PRESSED)
				{
				ActionDisplay("ToggleRubberbandMode", "");
				ActionMode("Save");
				ActionMode("Move");
				ActionMode("Notify");
				}
			break;

		case 3:
			if (   mk == NONE_PRESSED
			    && (   (   Settings.Mode == LINE_MODE
			            && Crosshair.AttachedLine.State != STATE_FIRST
			           )
			        || (   Settings.Mode == ARC_MODE
			            && Crosshair.AttachedBox.State != STATE_FIRST
			           )
			        || (   Settings.Mode == RECTANGLE_MODE
			            && Crosshair.AttachedBox.State != STATE_FIRST
			           )
			        || (   Settings.Mode == POLYGON_MODE
			            && Crosshair.AttachedLine.State != STATE_FIRST
			           )
			       )
			   )
				{
				if (Settings.Mode == LINE_MODE)
					ActionMode("Line");
				else if (Settings.Mode == ARC_MODE)
					ActionMode("Arc");
				else if (Settings.Mode == RECTANGLE_MODE)
					ActionMode("Rectangle");
				else if (Settings.Mode == POLYGON_MODE)
					ActionMode("Polygon");

				ActionMode("Notify");
				}
			else if (   mk == NONE_PRESSED
			         && !gui->command_entry_status_line_active
			        )
				{
				gtk_widget_grab_focus(drawing_area);
				if (GTK_IS_MENU(menu))
					gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL,
							drawing_area, 3, ev->time);
				}
			else if (mk == SHIFT_PRESSED)
				{
				ActionDisplay("Save", "");
				ActionSetValue("Zoom", "1000", "");
				}
			break;
		}
	return TRUE;
	}

gboolean
gui_output_button_release_cb(GtkWidget *drawing_area,
				GdkEventButton *ev, GtkUIManager *ui)
	{
	ModifierKeysState	mk;

	mk = gui_modifier_keys_state();

	switch (ev->button)
		{
		case 1:
			ActionMode("Release");	/* For all modifier states */
			break;

		case 2:
			if (mk == NONE_PRESSED || mk == SHIFT_PRESSED)
				{
				ActionMode("Release");
				ActionMode("Restore");
				}
			else if (mk == CONTROL_PRESSED)
				{
				ActionMode("Notify");
				ActionMode("Restore");
				}
			else if (mk == SHIFT_CONTROL_PRESSED)
				{
				ActionMode("Notify");
				ActionMode("Restore");
				ActionDisplay("ToggleRubberbandMode", "");
				}
			break;

		case 3:
			if (mk == SHIFT_PRESSED)
				{
				ActionDisplay("Center", "");
				ActionDisplay("Restore", "");
				}
			break;
		}
	return TRUE;
	}



gboolean
gui_output_drawing_area_configure_event_cb(GtkWidget *widget,
			GdkEventConfigure *ev, OutputType *out)
	{
	gboolean	new_w, new_h;

	new_w = (out->Width != widget->allocation.width);
	new_h = (out->Height != widget->allocation.height);
	out->Width = widget->allocation.width;
	out->Height = widget->allocation.height;

	if (new_w || new_h || !out->pixmap || !out->mask)
		{
		if (out->pixmap)
			gdk_pixmap_unref(out->pixmap);
		if (out->mask)
			gdk_pixmap_unref(out->mask);
		out->pixmap = gdk_pixmap_new(widget->window,
					out->Width, out->Height, -1);
		out->mask = gdk_pixmap_new(widget->window,
					out->Width, out->Height, 1);

		if (!out->bgGC)
			{
			out->bgGC = gdk_gc_new(out->top_window->window);
			gdk_gc_copy(out->bgGC, out->top_window->style->white_gc );
			}
		if (!out->pmGC)
			{
			out->pmGC = gdk_gc_new(out->top_window->window);
			gdk_gc_copy(out->bgGC, out->top_window->style->white_gc );
			}

		if (!out->mask)
			g_message ("Damn! Can't get the bitmap!!\n");
		if (!out->pixmap)
			g_message ("Can't get pixmap for offscreen drawing\n"
				"Drawing will flicker as a result\n");
		gui_output_positioners_scale();
		UpdateAll ();
		set_status_line_label();
		}
	if (!Output.layout)	/* XXX */
		Output.layout = gtk_widget_create_pango_layout(widget, NULL);
	return FALSE;
	}

gboolean
gui_output_drawing_area_expose_event_cb(GtkWidget *widget,
				GdkEventExpose *ev, OutputType *out)
	{
	GdkRegion		*myRegion = gdk_region_new();

	gdk_draw_drawable(widget->window, out->bgGC, out->pixmap,
				ev->area.x, ev->area.y, ev->area.x, ev->area.y,
				ev->area.width, ev->area.height);

	gdk_region_union_with_rect(myRegion, &ev->area);

	DrawClipped(myRegion);
	gdk_region_destroy(myRegion);

	return FALSE;
	}

static void
output_auto_scroll_stop(void)
	{
	if (x_scroll_timer_id)
		gtk_timeout_remove(x_scroll_timer_id);
	if (y_scroll_timer_id)
		gtk_timeout_remove(y_scroll_timer_id);
	if (x_scroll_timer_id == 0 && y_scroll_timer_id == 0)
		RestoreCrosshair(TRUE);
	x_scroll_timer_id = 0;
	y_scroll_timer_id = 0;
	}

gint
gui_output_window_motion_cb(GtkWidget *widget,
			GdkEventButton *ev, OutputType *out)
	{
	/* In case missed an enter event through all of this
	*/
	if (!out->has_entered)
		{
		output_auto_scroll_stop();
		if (!gui->command_entry_status_line_active)
			{
			out->has_entered = TRUE;
			gtk_widget_grab_focus(out->drawing_area);
			}
		}
	EventMoveCrosshair(ev->x, ev->y);

	return FALSE;
	}

gint
gui_output_window_enter_cb(GtkWidget *widget,
			GdkEventButton *ev, OutputType *out)
	{
	if (!gui->command_entry_status_line_active)
		{
		out->has_entered = TRUE;
		/* Make sure drawing area has keyboard focus when we are in it.
		*/
		gtk_widget_grab_focus(out->drawing_area);
		}

	output_auto_scroll_stop ();

	return FALSE;
	}

static gboolean
output_x_scroll_cb(gpointer data)
	{
	gint	accel = GPOINTER_TO_INT(data);

	HideCrosshair(FALSE);
	CenterDisplay(TO_SCREEN(x_scroll_delta), 0, TRUE);
	MoveCrosshairRelative(x_scroll_delta, 0);
	AdjustAttachedObjects();
	RestoreCrosshair(FALSE);

	/* Accelerate the scroll a bit
	*/
	x_scroll_delta += SGN(x_scroll_delta) * accel;

	return TRUE;		/* Restarts timer */
	}

static gboolean
output_y_scroll_cb(gpointer data)
	{
	gint	accel = GPOINTER_TO_INT(data);

	HideCrosshair(FALSE);
	CenterDisplay(0, TO_SCREEN(y_scroll_delta), TRUE);
	MoveCrosshairRelative(0, y_scroll_delta);
	AdjustAttachedObjects();
	RestoreCrosshair(FALSE);

	/* Accelerate the scroll a bit
	*/
	y_scroll_delta += SGN(y_scroll_delta) * accel;

	return TRUE;		/* Restarts timer */
	}


gint
gui_output_window_leave_cb(GtkWidget *widget,
				GdkEventButton *ev, OutputType *out)
	{
	gint	x, y, dx, dy;
	gint	accel;

	if (out->has_entered)
		{
		/* if actively drawing, start scrolling */
		if (ActiveDrag ())
			{
			/* GdkEvent coords are set to 0,0 at leave events, so must figure
			|  out edge the cursor left.
			*/
			x = TO_SCREEN_X(Crosshair.X);
			y = TO_SCREEN_Y(Crosshair.Y);
			dx = Output.Width - x;
			dy = Output.Height - y;

			accel = (gint) PCB->Zoom + 5;
			if (accel < 1)
				accel = 1;
			x_scroll_delta = accel * 200;
			y_scroll_delta = accel * 200;

			if (x < dx)
				{
				x_scroll_delta = -x_scroll_delta;
				dx = x;
				}
			if (y < dy)
				{
				y_scroll_delta = -y_scroll_delta;
				dy = y;
				}

			if (dx < dy)
				x_scroll_timer_id = gtk_timeout_add(SCROLL_TIME,
							(GtkFunction) output_x_scroll_cb,
							GINT_TO_POINTER(accel * 15));
			else
				y_scroll_timer_id = gtk_timeout_add(SCROLL_TIME,
							(GtkFunction) output_y_scroll_cb,
							GINT_TO_POINTER(accel * 15));
			}
		else
			HideCrosshair(TRUE);
		}
	out->has_entered = FALSE;

	return FALSE;
	}

  /* Mouse scroll wheel events
  */
gint
gui_output_window_mouse_scroll_cb(GtkWidget *widget,
					GdkEventScroll *ev, OutputType *out)
	{
	ModifierKeysState	mk = gui_modifier_keys_state();
	gint				dx = 0, dy = 0, delta;

	/* Make scroll amount a function of zoom level to get some uniform
	|  apparent scrolling at all zoom levels.
	*/
	if (PCB->Zoom > 3)
		delta = ((gint) PCB->Zoom + 5) * 2000;
	else if (PCB->Zoom > -4)
		delta = ((gint) PCB->Zoom + 5) * 1250;
	else if (PCB->Zoom > -10)
		delta = ((gint) PCB->Zoom + 10) * 250;
	else
		delta = 100;

	if (mk == SHIFT_PRESSED || mk == CONTROL_PRESSED)
		dx = delta;
	else
		dy = delta;

	if (ev->direction == GDK_SCROLL_DOWN)
		{
		dx = -dx;
		dy = -dy;
		}

	HideCrosshair(FALSE);
	CenterDisplay(TO_SCREEN(dx), TO_SCREEN(dy), TRUE);
	MoveCrosshairRelative(dx, dy);
	AdjustAttachedObjects();
	RestoreCrosshair(FALSE);

	return TRUE;
	}


  /* Called if entering more than 1/4 of the way into the vbox_left control,
  |  set in gut-top-window.c.
  */
void
gui_output_stop_scroll_cb(GtkWidget *widget,
			GdkEventButton *ev, OutputType *out)
	{
	static gint	ebox_width;

	if (ebox_width == 0)
		gdk_drawable_get_size(widget->window, &ebox_width, NULL);
	if (ev->x > ebox_width - 25)
		return;
	if (x_scroll_timer_id || y_scroll_timer_id)
		output_auto_scroll_stop();
	}

