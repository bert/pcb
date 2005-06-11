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


/* gui-top-window.c
|  This handles creation of the top level window and all its widgets.
|  events for the Output.drawing_area widget are handled in a separate
|  file gui-output-events.c
|
|  Some caveats with menu shorcut keys:  Some keys are trapped out by Gtk
|  and can't be used as shortcuts (eg. '|', TAB, etc).  For these cases
|  the Gtk menus can't show the same shortcut as the Xt menus did, but the
|  actions are the same as the keys are handled in gui_output_key_press_cb().
*/



#include "gui.h"

#include "action.h"
#include "autoplace.h"
#include "autoroute.h"
#include "buffer.h"
#include "change.h"
#include "command.h"
#include "copy.h"
#include "create.h"
#include "crosshair.h"
#include "djopt.h"
#include "draw.h"
#include "error.h"
#include "file.h"
#include "find.h"
#include "insert.h"
#include "line.h"
#include "mymem.h"
#include "misc.h"
#include "move.h"
#include "output.h"
#include "polygon.h"
#include "rats.h"
#include "remove.h"
#include "report.h"
#include "rotate.h"
#include "rubberband.h"
#include "search.h"
#include "select.h"
#include "set.h"
#include "undo.h"
#include "vendor.h"

#include "gui-icons-mode-buttons.data"
#include "gui-icons-misc.data"


GuiPCB	*gui;


#define	N_GRID_SETTINGS		8

static gdouble	grid_mil_values[N_GRID_SETTINGS] =
	{
	10.0,
	50.0,
	100.0,
	500.0,
	1000.0,
	2500.0,
	5000.0,
	10000.0
	};

static gdouble	grid_mm_values[N_GRID_SETTINGS] =
	{
	39.370078740,
	78.740157480,
	196.850393701,
	393.700787402,
	787.401574804,
	1968.50393701,
	3937.00787402,
	7874.01574804
	};

  /* When the user toggles grid units mil<->mm, call this to get an
  |  index into the grid values table of the current grid setting.  Then a
  |  grid in the new units may be selected that is closest to what we had.
  |
  |  May want this call to fail if user has altered grid with 'g' key
  |  and so current grid does not match any of the presets.  In that
  |  case we want no item in the grid setting radio group to get set.
  */
static gint
get_grid_value_index(gboolean allow_fail)
	{
	gdouble		*value;
	gint		i;

	value = Settings.grid_units_mm ? &grid_mm_values[0] : &grid_mil_values[0];
	for (i = 0; i < N_GRID_SETTINGS; ++i, ++value)
		if (PCB->Grid < *value + 1.0 && PCB->Grid > *value - 1.0)
			break;
	if (i >= N_GRID_SETTINGS)
		i = allow_fail ? -1 : N_GRID_SETTINGS - 1;

	return i;
	}

static void
h_adjustment_changed_cb(GtkAdjustment *adj, GuiPCB *g)
	{
	gdouble	xval, yval;

	if (g->adjustment_changed_holdoff)
		return;
	xval = gtk_adjustment_get_value(adj);
	yval = gtk_adjustment_get_value(GTK_ADJUSTMENT(gui->v_adjustment));

/*	printf("h_adjustment_changed_cb: %f %f\n", xval, yval); */
	Pan((gint) xval, (gint)yval, TRUE, TRUE);
	}

static void
v_adjustment_changed_cb(GtkAdjustment *adj, GuiPCB *g)
	{
	gdouble	xval, yval;

	if (g->adjustment_changed_holdoff)
		return;
	yval = gtk_adjustment_get_value(adj);
	xval = gtk_adjustment_get_value(GTK_ADJUSTMENT(gui->h_adjustment));

/*	printf("v_adjustment_changed_cb: %f %f\n", xval, yval); */
	Pan((gint) xval, (gint)yval, TRUE, TRUE);
	}

  /* Save size of top window changes so PCB can restart at its size at exit.
  */
static gint
top_window_configure_event_cb(GtkWidget *widget, GdkEventConfigure *ev,
			OutputType *out)
	{
	gboolean	new_w, new_h;

	new_w = (Settings.pcb_width != widget->allocation.width);
	new_h = (Settings.pcb_height != widget->allocation.height);

	Settings.pcb_width = widget->allocation.width;
	Settings.pcb_height = widget->allocation.height;

	if (new_w || new_h)
		Settings.config_modified = TRUE;

	return FALSE;
	}


/* =========================================================================
|  Here are the menu callbacks.  To use the "Save layout as" menu item as
|  an example of Gtk ui_manager menu handling, here's how it would be added:
|    1) In the XML ui_info string, add a menuitem:
|         <menuitem action='SaveLayoutAs'/>
|       which says that a menu item should appear as defined by the
|       'SaveLayoutAs' action.
|    2) So a GtkActionEntry for this must exist in one of the action arrays
|       to be loaded into the ui_manager.  For this there is the entry:
|          { "SaveLayoutAs", NULL, N_("Save layout as"), NULL, NULL,
|                      G_CALLBACK(save_layout_as_cb) },
|       which connects the menu position (via the name SaveLayoutAs) to the
|       text to display and the callback function to call.
|    3) And the callback function must be written.
|
|  Actions can be removed, modified and reloaded.  This is how the menus
|  can be updated, for example, to display grid units appropriate when the
|  grid units are toggle from mil<->mm.  At toggles, the relevant actions are
|  removed, the string to display is updated, and the actions are added
|  back in.  There are several other similar dynamic menu adjustments here.
|
|  Some of the callbacks that need to get a location when invoked from the
|  menu need to work differently and not get a location when invoked via
|  a keyboard shortcut.  That's done by checking shift or control state
|  in the callback to determine which indicates keyboard being used.
*/

/* ============== FileMenu callbacks =============== */
static void
save_layout_cb(GtkAction *action, OutputType *out)
	{
	ActionSave("Layout");
	}

static void
save_layout_as_cb(GtkAction *action, OutputType *out)
	{
	ActionSave("LayoutAs");
	}

static void
load_layout_cb(GtkAction *action, OutputType *out)
	{
	ActionLoad("Layout");
	}

static void
load_element_data_cb(GtkAction *action, OutputType *out)
	{
	ActionPasteBuffer("Clear", "");
	ActionLoad("ElementTobuffer");
	}

static void
load_layout_data_cb(GtkAction *action, OutputType *out)
	{
	ActionPasteBuffer("Clear", "");
	ActionLoad("LayoutTobuffer");
	}

static void
load_netlist_file_cb(GtkAction *action, OutputType *out)
	{
	ActionLoad("Netlist");
	}

static void
load_vendor_file_cb(GtkAction *action, OutputType *out)
	{
	ActionLoadVendor("");
	}

static void
print_layout_cb(GtkAction *action, OutputType *out)
	{
	ActionPrint();
	}

static void
connections_single_element_cb(GtkAction *action, OutputType *out)
	{
	if (ActionGetLocation(_("Press a button at the element's location")))
		ActionSave("ElementConnections");
	}

static void
connections_all_elements_cb(GtkAction *action, OutputType *out)
	{
	ActionSave("AllConnections");
	}

static void
connections_unused_pins_cb(GtkAction *action, OutputType *out)
	{
	ActionSave("AllUnusedPins");
	}

static void
new_layout_cb(GtkAction *action, OutputType *out)
	{
	ActionNew();
	}

static void
quit_cb(GtkAction *action, OutputType *out)
	{
	ActionQuit();
	}



/* ============== EditMenu callbacks =============== */
static void
undo_cb(GtkAction *action, OutputType *out)
	{
	ActionUndo("");
	}

static void
redo_cb(GtkAction *action, OutputType *out)
	{
	ActionRedo();
	}

static void
clear_undo_cb(GtkAction *action, OutputType *out)
	{
	ActionUndo("ClearList");
	}


static void
edit_text_on_layout_cb(GtkAction *action, OutputType *out)
	{
	ActionChangeName("Object");
	}

static void
edit_name_of_layout_cb(GtkAction *action, OutputType *out)
	{
	ActionChangeName("Layout");
	}

static void
edit_name_of_active_layer_cb(GtkAction *action, OutputType *out)
	{
	ActionChangeName("Layer");
	}



/* ============== ScreenMenu callbacks =============== */
static void
redraw_layout_cb(GtkAction *action, OutputType *out)
	{
	ActionDisplay("ClearAndRedraw", "");
	}

static void
center_layout_cb(GtkAction *action, OutputType *out)
	{
	ActionDisplay("Center", "");
	}

static void
toggle_draw_grid_cb(GtkToggleAction *action, OutputType *out)
	{
	if (gui->toggle_holdoff)	/* If setting initial condition */
		return;

	/* Set to ! because ActionDisplay toggles it */
	Settings.DrawGrid = !gtk_toggle_action_get_active(action);
	Settings.config_modified = TRUE;
	ActionDisplay("Grid", "");
	}

static void
realign_grid_cb(GtkAction *action, OutputType *out)
	{
	if (ActionGetLocation(_("Press a button at a grid point")))
		ActionDisplay("ToggleGrid", "");
	}

static void
zoom_cb(GtkAction *action, GtkRadioAction *current)
	{
	const gchar	*saction = gtk_action_get_name(action);
	gchar		*arg;

	arg = !strcmp(saction, "ZoomIn") ? "-1" : "+1";
	ActionSetValue("Zoom", arg, "");
	}


static void
toggle_view_solder_side_cb(GtkAction *action, OutputType *out)
	{
	/* If get here from the menu, ask for a locaton.
	*/
	if (   !gui_shift_is_pressed()
	    && !ActionGetLocation(_("Press a button at the desired point"))
	   )
		return;
	ActionSwapSides();
	}

static void
toggle_show_solder_mask_cb(GtkAction *action, OutputType *out)
	{
	/* Transient setting, not saved in Settings & not used for new PCB flag. */
	ActionDisplay("ToggleMask", "");
	}

static void
toggle_pinout_shows_number_cb(GtkToggleAction *action, OutputType *out)
	{
	/* Toggle existing PCB flag and use setting to initialize new PCB flag */
	Settings.ShowNumber = gtk_toggle_action_get_active(action);
	Settings.config_modified = TRUE;
	if (!gui->toggle_holdoff)
		ActionDisplay("ToggleName", "");
	}

static void
pinout_menu_cb(GtkAction *action, OutputType *out)
	{
	if (   !gui_shift_is_pressed()
	    && !ActionGetLocation("Select an element")
	   )
		return;
	ActionDisplay("Pinout", "");
	}


static void
toggle_grid_units_cb(GtkToggleAction *action, OutputType *out)
	{
	gchar		*grid;
	gint		i;
	gboolean	active;

	if (gui->toggle_holdoff)	/* If setting initial condition */
		return;
	active = gtk_toggle_action_get_active(action);

	i = get_grid_value_index(FALSE);
	Settings.grid_units_mm = active;
	PCB->Grid = active ? grid_mm_values[i] : grid_mil_values[i];

	gui_grid_setting_update_menu_actions();

	grid = g_strdup_printf("%f", PCB->Grid);
	ActionSetValue("Grid", grid, "");
	g_free(grid);

	set_cursor_position_labels();
	gtk_label_set_markup(GTK_LABEL(gui->cursor_units_label),
				Settings.grid_units_mm ?
				"<b>mm</b> " : "<b>mil</b> ");
	gui_config_handle_units_changed();
	gui_change_selected_update_menu_actions();
	}

static void
radio_grid_mil_setting_cb(GtkAction *action, GtkRadioAction *current)
	{
	gdouble	value;
	gchar	*grid;
	gint	index;

	if (gui->toggle_holdoff)
		return;
	index = gtk_radio_action_get_current_value(current);
	value = grid_mil_values[index];
	grid = g_strdup_printf("%f", value);
	ActionSetValue("Grid", grid, "");
	g_free(grid);
	}

static void
radio_grid_mm_setting_cb(GtkAction *action, GtkRadioAction *current)
	{
	gdouble	value;
	gchar	*grid;
	gint	index;

	if (gui->toggle_holdoff)
		return;
	index = gtk_radio_action_get_current_value(current);
	value = grid_mm_values[index];
	grid = g_strdup_printf("%f", value);
	ActionSetValue("Grid", grid, "");
	g_free(grid);
	}

static void
radio_displayed_element_name_cb(GtkAction *action, GtkRadioAction *current)
	{
	gint			value;
	static gchar	*doit[] = { "Description", "NameOnPCB", "Value" };
	
	value = gtk_radio_action_get_current_value(current);
	if (value >= 0 && value < 4)
		ActionDisplay(doit[value], "");
	}



/* ============== SettingsMenu callbacks =============== */
static void
toggle_45_degree_cb(GtkToggleAction *action, OutputType *out)
	{
	/* Toggle existing PCB flag and use setting to initialize new PCB flag */
	Settings.AllDirectionLines = gtk_toggle_action_get_active(action);
	Settings.config_modified = TRUE;
	if (!gui->toggle_holdoff)
		ActionDisplay("Toggle45Degree", "");
	}

static void
toggle_start_direction_cb(GtkToggleAction *action, OutputType *out)
	{
	/* Toggle existing PCB flag and use setting to initialize new PCB flag */
	Settings.SwapStartDirection = gtk_toggle_action_get_active(action);
	Settings.config_modified = TRUE;
	if (!gui->toggle_holdoff)
		ActionDisplay("ToggleStartDirection", "");
	}

static void
toggle_orthogonal_moves_cb(GtkToggleAction *action, OutputType *out)
	{
	/* Toggle existing PCB flag and use setting to initialize new PCB flag */
	Settings.OrthogonalMoves = gtk_toggle_action_get_active(action);
	Settings.config_modified = TRUE;
	if (!gui->toggle_holdoff)
		ActionDisplay("ToggleOrthoMove", "");
	}

static void
toggle_snap_pin_cb(GtkToggleAction *action, OutputType *out)
	{
	/* Toggle existing PCB flag and use setting to initialize new PCB flag */
	Settings.SnapPin = gtk_toggle_action_get_active(action);
	Settings.config_modified = TRUE;
	if (!gui->toggle_holdoff)
		ActionDisplay("ToggleSnapPin", "");
	}

static void
toggle_show_DRC_cb(GtkToggleAction *action, OutputType *out)
	{
	/* Toggle existing PCB flag and use setting to initialize new PCB flag */
	Settings.ShowDRC = gtk_toggle_action_get_active(action);
	Settings.config_modified = TRUE;
	if (!gui->toggle_holdoff)
		ActionDisplay("ToggleShowDRC", "");
	}

static void
toggle_auto_DRC_cb(GtkToggleAction *action, OutputType *out)
	{
	/* Toggle existing PCB flag and use setting to initialize new PCB flag */
	Settings.AutoDRC = gtk_toggle_action_get_active(action);
	Settings.config_modified = TRUE;
	if (!gui->toggle_holdoff)
		ActionDisplay("ToggleAutoDRC", "");
	}

static void
toggle_rubber_band_cb(GtkToggleAction *action, OutputType *out)
	{
	/* Toggle existing PCB flag and use setting to initialize new PCB flag */
	Settings.RubberBandMode = gtk_toggle_action_get_active(action);
	Settings.config_modified = TRUE;
	if (!gui->toggle_holdoff)
		ActionDisplay("ToggleRubberBandMode", "");
	}

static void
toggle_unique_names_cb(GtkToggleAction *action, OutputType *out)
	{
	/* Toggle existing PCB flag and use setting to initialize new PCB flag */
	Settings.UniqueNames = gtk_toggle_action_get_active(action);
	Settings.config_modified = TRUE;
	if (!gui->toggle_holdoff)
		ActionDisplay("ToggleUniqueNames", "");
	}

static void
toggle_local_ref_cb(GtkAction *action, OutputType *out)
	{
	/* Transient setting, not saved in Settings & not used for new PCB flag. */
	ActionDisplay("ToggleLocalRef", "");
	}

static void
toggle_clear_line_cb(GtkToggleAction *action, OutputType *out)
	{
	/* Toggle existing PCB flag and use setting to initialize new PCB flag */
	Settings.ClearLine = gtk_toggle_action_get_active(action);
	Settings.config_modified = TRUE;
	if (!gui->toggle_holdoff)
		ActionDisplay("ToggleClearLine", "");
	}

static void
toggle_live_route_cb(GtkToggleAction *action, OutputType *out)
	{
	/* Toggle existing PCB flag and use setting to initialize new PCB flag */
	Settings.liveRouting = gtk_toggle_action_get_active(action);
	Settings.config_modified = TRUE;
	if (!gui->toggle_holdoff)
		ActionDisplay("ToggleLiveRoute", "");
	}

static void
toggle_thin_draw_cb(GtkAction *action, OutputType *out)
	{
	/* Transient setting, not saved in Settings & not used for new PCB flag. */
	ActionDisplay("ToggleThindraw", "");
	}

static void
toggle_check_planes_cb(GtkAction *action, OutputType *out)
	{
	/* Transient setting, not saved in Settings & not used for new PCB flag. */
	ActionDisplay("ToggleCheckPlanes", "");
	}

static void
toggle_vendor_drill_mapping_cb(GtkAction *action, OutputType *out)
	{
	/* Transient setting, not saved in Settings & not used for new PCB flag. */
	ActionToggleVendor();
	}

/* ============== SelectMenu callbacks =============== */
static void
select_all_cb(GtkAction *action, OutputType *out)
	{
	ActionSelect("All");
	}

static void
select_all_connected_cb(GtkAction *action, OutputType *out)
	{
	ActionSelect("Connection");
	}

static void
unselect_all_cb(GtkAction *action, OutputType *out)
	{
	ActionUnselect("All");
	}

static void
unselect_all_connected_cb(GtkAction *action, OutputType *out)
	{
	ActionUnselect("Connection");
	}

static void
select_objects_by_name_cb(GtkAction *action, OutputType *out)
	{
	ActionSelect("ObjectByName");
	}

static void
select_elements_by_name_cb(GtkAction *action, OutputType *out)
	{
	ActionSelect("ElementByName");
	}

static void
select_pads_by_name_cb(GtkAction *action, OutputType *out)
	{
	ActionSelect("PadByName");
	}

static void
select_pins_by_name_cb(GtkAction *action, OutputType *out)
	{
	ActionSelect("PinByName");
	}

static void
select_text_by_name_cb(GtkAction *action, OutputType *out)
	{
	ActionSelect("TextByName");
	}

static void
select_vias_by_name_cb(GtkAction *action, OutputType *out)
	{
	ActionSelect("ViaByName");
	}

static void
auto_place_selected_cb(GtkAction *action, OutputType *out)
	{
	ActionAutoPlaceSelected();
	}

static void
disperse_all_elements_cb(GtkAction *action, OutputType *out)
	{
	ActionDisperseElements("All");
	}

static void
disperse_selected_elements_cb(GtkAction *action, OutputType *out)
	{
	ActionDisperseElements("Selected");
	}

static void
move_selected_other_side_cb(GtkAction *action, OutputType *out)
	{
	ActionFlip("SelectedElements");
	}

static void
remove_selected_cb(GtkAction *action, OutputType *out)
	{
	ActionRemoveSelected();
	}

static void
convert_selected_to_element_cb(GtkAction *action, OutputType *out)
	{
	ActionSelect("Convert");
	}

static void
optimize_selected_rats_cb(GtkAction *action, OutputType *out)
	{
	ActionDeleteRats("SelectedRats");
	ActionAddRats("SelectedRats");
	}


static void
rip_up_selected_tracks_cb(GtkAction *action, OutputType *out)
	{
	ActionRipUp("Selected");
	}


static void
selected_lines_size_change_cb(GtkAction *action, OutputType *out)
	{
	gchar		*value, *units;

	gui_size_increment_get_value(gtk_action_get_name(action), &value, &units);
	ActionChangeSize("SelectedLines", value, units);
	}

static void
selected_pads_size_change_cb(GtkAction *action, OutputType *out)
	{
	gchar		*value, *units;

	gui_size_increment_get_value(gtk_action_get_name(action), &value, &units);
	ActionChangeSize("SelectedPads", value, units);
	}

static void
selected_pins_size_change_cb(GtkAction *action, OutputType *out)
	{
	gchar		*value, *units;

	gui_size_increment_get_value(gtk_action_get_name(action), &value, &units);
	ActionChangeSize("SelectedPins", value, units);
	}

static void
selected_text_size_change_cb(GtkAction *action, OutputType *out)
	{
	gchar		*value, *units;

	gui_size_increment_get_value(gtk_action_get_name(action), &value, &units);
	ActionChangeSize("SelectedText", value, units);
	}

static void
selected_vias_size_change_cb(GtkAction *action, OutputType *out)
	{
	gchar		*value, *units;

	gui_size_increment_get_value(gtk_action_get_name(action), &value, &units);
	ActionChangeSize("SelectedVias", value, units);
	}

static void
selected_vias_drill_change_cb(GtkAction *action, OutputType *out)
	{
	gchar		*value, *units;

	gui_size_increment_get_value(gtk_action_get_name(action), &value, &units);
	ActionChange2ndSize("SelectedVias", value, units);
	}

static void
selected_pins_drill_change_cb(GtkAction *action, OutputType *out)
	{
	gchar		*value, *units;

	gui_size_increment_get_value(gtk_action_get_name(action), &value, &units);
	ActionChange2ndSize("SelectedPins", value, units);
	}

static void
selected_change_square_elements_cb(GtkAction *action, OutputType *out)
	{
	ActionChangeSquare("SelectedElements");
	}

static void
selected_change_square_pins_cb(GtkAction *action, OutputType *out)
	{
	ActionChangeSquare("SelectedPins");
	}


/* ============== BufferMenu callbacks =============== */
#define	PRESS_BUTTON_ELEMENT_PROMPT	\
		_("Press a button on a reference point for your selection")

static void
copy_selection_to_buffer_cb(GtkAction *action, OutputType *out)
	{
	if (   !gui_control_is_pressed()
	    && !ActionGetLocation(PRESS_BUTTON_ELEMENT_PROMPT)
	   )
		return;
	ActionPasteBuffer("Clear", "");
	ActionPasteBuffer("AddSelected", "");
	ActionMode("PasteBuffer");
	}


static void
cut_selection_to_buffer_cb(GtkAction *action, OutputType *out)
	{
	if (   !gui_control_is_pressed()
	    && !ActionGetLocation(PRESS_BUTTON_ELEMENT_PROMPT)
	   )
		return;
	ActionPasteBuffer("Clear", "");
	ActionPasteBuffer("AddSelected", "");
	ActionRemoveSelected();
	ActionMode("PasteBuffer");
	}

static void
paste_buffer_to_layout_cb(GtkAction *action, OutputType *out)
	{
	ActionMode("PasteBuffer");
	}

static void
rotate_buffer_CCW_cb(GtkAction *action, OutputType *out)
	{
	ActionMode("PasteBuffer");
	ActionPasteBuffer("Rotate", "1");
	}

static void
rotate_buffer_CW_cb(GtkAction *action, OutputType *out)
	{
	ActionMode("PasteBuffer");
	ActionPasteBuffer("Rotate", "3");
	}

static void
mirror_buffer_up_down_cb(GtkAction *action, OutputType *out)
	{
	ActionMode("PasteBuffer");
	ActionPasteBuffer("Mirror", "");
	}

static void
mirror_buffer_left_right_cb(GtkAction *action, OutputType *out)
	{
	ActionMode("PasteBuffer");
	ActionPasteBuffer("Rotate", "1");
	ActionPasteBuffer("Mirror", "");	
	ActionPasteBuffer("Rotate", "3");
	}

/* ============== ConnectsMenu callbacks =============== */
static void
clear_buffer_cb(GtkAction *action, OutputType *out)
	{
	ActionPasteBuffer("Clear", "");
	}

static void
convert_buffer_to_element_cb(GtkAction *action, OutputType *out)
	{
	ActionPasteBuffer("Convert", "");
	}

static void
break_buffer_elements_cb(GtkAction *action, OutputType *out)
	{
	ActionPasteBuffer("Restore", "");
	}

static void
save_buffer_elements_cb(GtkAction *action, OutputType *out)
	{
	ActionPasteBuffer("Save", "");
	}

static void
radio_select_current_buffer_cb(GtkAction *action, GtkRadioAction *current)
	{
	gchar	buf[16];
	gint	value;
	
	value = gtk_radio_action_get_current_value(current);
	if (value >= 1 && value <= 5)
		{
		sprintf(buf, "%d", value);
		ActionPasteBuffer(buf, "");
		}
	}


/* ============== ConnectsMenu callbacks =============== */
static void
lookup_connection_cb(GtkAction *action, OutputType *out)
	{
	if (   !gui_control_is_pressed()
	    && !ActionGetLocation(_("Select the object"))
	   )
		return;
	ActionConnection("Find");
	}

static void
reset_scanned_pads_cb(GtkAction *action, OutputType *out)
	{
	ActionConnection("ResetPinsViasAndPads");
	ActionDisplay("Redraw", "");
	}

static void
reset_scanned_lines_cb(GtkAction *action, OutputType *out)
	{
	ActionConnection("ResetLinesAndPolygons");
	ActionDisplay("Redraw", "");
	}

static void
reset_all_connections_cb(GtkAction *action, OutputType *out)
	{
	ActionConnection("Reset");
	ActionDisplay("Redraw", "");
	}

static void
optimize_rats_nest_cb(GtkAction *action, OutputType *out)
	{
	ActionAtomic("Save");
	ActionDeleteRats("AllRats");
	ActionAtomic("Restore");
	ActionAddRats("AllRats");
	ActionAtomic("Block");
	}

static void
erase_rats_nest_cb(GtkAction *action, OutputType *out)
	{
	ActionDeleteRats("AllRats");
	}

static void
auto_route_selected_rats_cb(GtkAction *action, OutputType *out)
	{
	ActionAutoRoute("Selected");
	}

static void
auto_route_all_rats_cb(GtkAction *action, OutputType *out)
	{
	ActionAutoRoute("AllRats");
	}

static void
rip_up_auto_routed_cb(GtkAction *action, OutputType *out)
	{
	ActionRipUp("All");
	}

static void
optimize_routes_cb(GtkAction *action, OutputType *out)
	{
	const gchar	*saction;

	saction = gtk_action_get_name(action);
	if (!strcmp(saction, "AutoOptimize"))
		ActionDJopt("auto");
	else if (!strcmp(saction, "Debumpify"))
		ActionDJopt("debumpify");
	else if (!strcmp(saction, "Unjaggy"))
		ActionDJopt("unjaggy");
	else if (!strcmp(saction, "ViaNudge"))
		ActionDJopt("vianudge");
	else if (!strcmp(saction, "ViaTrim"))
		ActionDJopt("viatrim");
	else if (!strcmp(saction, "OrthoPull"))
		ActionDJopt("orthopull");
	else if (!strcmp(saction, "SimpleOpts"))
		ActionDJopt("simple");
	else if (!strcmp(saction, "Miter"))
		ActionDJopt("miter");

	ActionDisplay("ClearAndRedraw", "");
	}

static void
toggle_only_auto_routed_cb(GtkAction *action, OutputType *out)
	{
	/* Transient setting, not saved in Settings. Not a PCB flag */
	djopt_set_auto_only();
	}

static void
design_rule_check_cb(GtkAction *action, OutputType *out)
	{
	ActionDRCheck();
	}

static void
apply_vendor_mapping_cb(GtkAction *action, OutputType *out)
	{
	ActionApplyVendor();
	}



/* ============== InfoMenu callbacks =============== */
static void
object_report_cb(GtkAction *action, OutputType *out)
	{
	if (   !gui_control_is_pressed()
	    && !ActionGetLocation(_("Select the object"))
	   )
		return;
	ActionReport("Object");
	}

static void
drill_summary_cb(GtkAction *action, OutputType *out)
	{
	ActionReport("DrillReport");
	}

static void
found_pins_pads_cb(GtkAction *action, OutputType *out)
	{
	ActionReport("FoundPins");
	}


/* ============== WindowMenu callbacks =============== */
static void
about_dialog_cb(GtkAction *action, OutputType *out)
	{
	gui_dialog_about();
	}

static void
keyref_window_cb(GtkAction *action, OutputType *out)
	{
	gui_keyref_window_show();
	}

static void
library_window_cb(GtkAction *action, OutputType *out)
	{
	gui_library_window_show(&Output);
	}

static void
message_window_cb(GtkAction *action, OutputType *out)
	{
	gui_log_window_show();
	}

static void
netlist_window_cb(GtkAction *action, OutputType *out)
	{
	gui_netlist_window_show(&Output);
	}

static void
command_entry_cb(GtkAction *action, OutputType *out)
	{
	ActionCommand("");
	}



/* ============== PopupMenu callbacks =============== */

static void
radio_select_tool_cb(GtkAction *action, GtkRadioAction *current)
	{
	const gchar	*saction;

	saction = gtk_action_get_name(GTK_ACTION(current));
	g_message("Grid setting action: \"%s\"", saction);
	}



/* ======================================================================
|  Here are the action entries that connect menuitems to the
|  above callbacks.
*/

static GtkActionEntry entries[] =
	{
	/* name, stock_id, label, accelerator, tooltip callback */
	{ "FileMenu", NULL, N_("File") },
		{ "SaveConnectionMenu", NULL, N_("Save connection data of") },

/* FileMenu */
	{ "SaveLayout", NULL, N_("Save layout"), NULL, NULL,
			G_CALLBACK(save_layout_cb) },
	{ "SaveLayoutAs", NULL, N_("Save layout as"), NULL, NULL,
			G_CALLBACK(save_layout_as_cb) },
	{ "LoadLayout", NULL, N_("Load layout"), NULL, NULL,
			G_CALLBACK(load_layout_cb) },
	{ "LoadElementData", NULL, N_("Load element data to paste-buffer"),
			NULL, NULL,
			G_CALLBACK(load_element_data_cb) },
	{ "LoadLayoutData", NULL, N_("Load layout data to paste-buffer"),
			NULL, NULL,
			G_CALLBACK(load_layout_data_cb) },
	{ "LoadNetlistFile", NULL, N_("Load netlist file"), NULL, NULL,
			G_CALLBACK(load_netlist_file_cb) },
	{ "LoadVendorFile", NULL, N_("Load vendor resource file"), NULL, NULL,
			G_CALLBACK(load_vendor_file_cb) },
	{ "PrintLayout", NULL, N_("Print layout"), NULL, NULL,
			G_CALLBACK(print_layout_cb) },
	{ "SingleElement", NULL, N_("a single element"), NULL, NULL,
			G_CALLBACK(connections_single_element_cb) },
	{ "AllElements", NULL, N_("all elements"), NULL, NULL,
			G_CALLBACK(connections_all_elements_cb) },
	{ "UnusedPins", NULL, N_("unused pins"), NULL, NULL,
			G_CALLBACK(connections_unused_pins_cb) },
	{ "NewLayout", NULL, N_("Start new layout"), NULL, NULL,
			G_CALLBACK(new_layout_cb) },
	{ "Preferences", NULL, N_("Preferences"), NULL, NULL,
			G_CALLBACK(gui_config_window_show) },
	{ "Quit", NULL, N_("Quit Program"), NULL, NULL,
			G_CALLBACK(quit_cb) },

/* EditMenu */
	{ "EditMenu", NULL, N_("Edit") },
	{ "Undo", NULL, N_("Undo last operation"),
			"u", NULL,
			G_CALLBACK(undo_cb) },
	{ "Redo", NULL, N_("Redo last undone operation"),
			"<shift>r", NULL,
			G_CALLBACK(redo_cb) },
	{ "ClearUndo", NULL, N_("Clear undo-buffer"),
			"<shift><control>u", NULL,
			G_CALLBACK(clear_undo_cb) },
	/* CutSelectionToBuffer CopySelectionToBuffer PasteBufferToLayout
	|  are in BufferMenu
	*/
	/* UnselectAll SelectAll are in SelectMenu
	*/
	{ "EditNamesMenu", NULL, N_("Edit name of") },
	{ "EditTextOnLayout", NULL, "text on layout", "n", NULL,
			G_CALLBACK(edit_text_on_layout_cb) },
	{ "EditNameOfLayout", NULL, N_("layout"), NULL, NULL,
			G_CALLBACK(edit_name_of_layout_cb) },
	{ "EditNameOfActiveLayer", NULL, N_("active layer"), NULL, NULL,
			G_CALLBACK(edit_name_of_active_layer_cb) },

/* ScreenMenu */
	{ "ScreenMenu", NULL, N_("Screen") },
	{ "RedrawLayout", NULL, N_("Redraw layout"),
			"r", NULL,
			G_CALLBACK(redraw_layout_cb) },
	{ "CenterLayout", NULL, N_("Center layout"),
			"c", NULL,
			G_CALLBACK(center_layout_cb) },
	{ "RealignGrid", NULL, N_("Realign grid"), NULL, NULL,
			G_CALLBACK(realign_grid_cb) },
	{ "GridSettingMenu", NULL, N_("Grid setting") },
		/* Some radio actions */
	{ "AdjustGridMenu", NULL, N_("Adjust grid") },
	{ "ZoomIn", NULL, "Zoom in",
			"z", NULL,
			G_CALLBACK(zoom_cb) },
	{ "ZoomOut", NULL, N_("Zoom out"),
			"<shift>z", NULL,
			G_CALLBACK(zoom_cb) },
	{ "DisplayedElementNameMenu", NULL, N_("Displayed element name") },
		/* Radio actions */
	{ "PinoutMenu", NULL, N_("Open pinout menu"),
			"<shift>d", NULL,
			G_CALLBACK(pinout_menu_cb) },

/* SizesMenu */
	{ "SizesMenu", NULL, N_("Sizes") },

/* SettingsMenu */
	{ "SettingsMenu", NULL, N_("Settings") },

/* SelectMenu */
	{ "SelectMenu", NULL, N_("Select") },
	{ "SelectAll", NULL, N_("Select all objects"),
			"<alt>a", NULL,
			G_CALLBACK(select_all_cb) },
	{ "SelectAllConnected", NULL, N_("Select all connected objects"),
			NULL, NULL,
			G_CALLBACK(select_all_connected_cb) },
	{ "UnselectAll", NULL, N_("Unselect all objects"),
			"<shift><alt>a", NULL,
			G_CALLBACK(unselect_all_cb) },
	{ "UnselectAllConnected", NULL, N_("Unselect all connected objects"),
			NULL, NULL,
			G_CALLBACK(unselect_all_connected_cb) },
	{ "SelectByNameMenu", NULL, N_("Select by name") },
	{ "SelectObjectsByName", NULL, N_("All objects"), NULL, NULL,
			G_CALLBACK(select_objects_by_name_cb) },
	{ "SelectElementsByName", NULL, N_("Elements"), NULL, NULL,
			G_CALLBACK(select_elements_by_name_cb) },
	{ "SelectPadsByName", NULL, N_("Pads"), NULL, NULL,
			G_CALLBACK(select_pads_by_name_cb) },
	{ "SelectPinsByName", NULL, N_("Pins"), NULL, NULL,
			G_CALLBACK(select_pins_by_name_cb) },
	{ "SelectTextByName", NULL, N_("Text"), NULL, NULL,
			G_CALLBACK(select_text_by_name_cb) },
	{ "SelectViasByName", NULL, N_("Vias"), NULL, NULL,
			G_CALLBACK(select_vias_by_name_cb) },
	{ "AutoPlaceSelected", NULL, N_("Auto place selected elements"),
			"<control>p", NULL,
			G_CALLBACK(auto_place_selected_cb) },
	{ "DisperseAllElements", NULL, N_("Disperse all elements"), NULL, NULL,
			G_CALLBACK(disperse_all_elements_cb) },
	{ "DisperseSelectedElements", NULL, N_("Disperse selected elements"),
			NULL, NULL,
			G_CALLBACK(disperse_selected_elements_cb) },
	{ "MoveSelectedOtherSide", NULL, N_("Move selected elements to other side"),
			"<shift>b", NULL,
			G_CALLBACK(move_selected_other_side_cb) },
	{ "RemoveSelected", NULL, N_("Remove selected objects"), NULL, NULL,
			G_CALLBACK(remove_selected_cb) },
	{ "ConvertSelectionToElement", NULL, N_("Convert selection to element"),
			NULL, NULL,
			G_CALLBACK(convert_selected_to_element_cb) },
	{ "OptimizeSelectedRats", NULL, N_("Optimize selected rats"), NULL, NULL,
			G_CALLBACK(optimize_selected_rats_cb) },
	{ "AutoRouteSelectedRats", NULL, N_("Auto route selected rats"),
			"<alt>r", NULL,
			G_CALLBACK(auto_route_selected_rats_cb) },
	{ "RipUpSelectedTracks", NULL, N_("Rip up selected auto routed tracks"),
			NULL, NULL,
			G_CALLBACK(rip_up_selected_tracks_cb) },
	{ "ChangeSelectedSizeMenu", NULL, N_("Change size of selected objects") },
		/* Actions in change_selected_entries[] to handle dynamic labels */
	{ "ChangeSelectedDrillMenu", NULL,
					N_("Change drill hole of selected objects") },
		/* Actions in change_selected_entries[] to handle dynamic labels */
	{ "ChangeSelectedSquareMenu", NULL,
					N_("Change square flag of selected objects") },
	{ "ChangeSquareElements", NULL, N_("Elements"), NULL, NULL,
			G_CALLBACK(selected_change_square_elements_cb) },
	{ "ChangeSquarePins", NULL, N_("Pins"), NULL, NULL,
			G_CALLBACK(selected_change_square_pins_cb) },

/* BufferMenu */
	{ "BufferMenu", NULL, "Buffer" },
	{ "CopySelectionToBuffer", NULL, N_("Copy selection to buffer"),
			"<control>x", NULL,
			G_CALLBACK(copy_selection_to_buffer_cb) },
	{ "CutSelectionToBuffer", NULL, N_("Cut selection to buffer"),
			"<shift><control>x", NULL,
			G_CALLBACK(cut_selection_to_buffer_cb) },
	{ "PasteBufferToLayout", NULL, N_("Paste buffer to layout"),
			NULL, NULL,
			G_CALLBACK(paste_buffer_to_layout_cb) },
	{ "RotateBufferCCW", NULL, N_("Rotate buffer 90 deg CCW"), NULL, NULL,
			G_CALLBACK(rotate_buffer_CCW_cb) },
	{ "RotateBufferCW", NULL, N_("Rotate buffer 90 deg CW"), NULL, NULL,
			G_CALLBACK(rotate_buffer_CW_cb) },
	{ "MirrorBufferUpDown", NULL, N_("Mirror buffer (up/down)"), NULL, NULL,
			G_CALLBACK(mirror_buffer_up_down_cb) },
	{ "MirrorBufferLeftRight", NULL, N_("Mirror buffer (left/right)"),
			NULL, NULL,
			G_CALLBACK(mirror_buffer_left_right_cb) },
	{ "ClearBuffer", NULL, N_("Clear buffer"), NULL, NULL,
			G_CALLBACK(clear_buffer_cb) },
	{ "ConvertBufferToElement", NULL, N_("Convert buffer to element"),
			NULL, NULL,
			G_CALLBACK(convert_buffer_to_element_cb) },
	{ "BreakBufferElements", NULL, N_("Break buffer elements to pieces"),
			NULL, NULL,
			G_CALLBACK(break_buffer_elements_cb) },
	{ "SaveBufferElements", NULL, N_("Save buffer elements to file"),
			NULL, NULL,
			G_CALLBACK(save_buffer_elements_cb) },
	{ "SelectCurrentBufferMenu", NULL, N_("Select current buffer") },
		/* Radio action menu */

/* ConnectsMenu */
	{ "ConnectsMenu", NULL, "Connects" },
	{ "LookupConnections", NULL, N_("Lookup connection to object"),
			"<control>f", NULL,
			G_CALLBACK(lookup_connection_cb) },
	{ "ResetScannedPads", NULL, N_("Reset scanned pads/pins/vias"), NULL, NULL,
			G_CALLBACK(reset_scanned_pads_cb) },
	{ "ResetScannedLines", NULL, N_("Reset scanned lines/polygons"), NULL, NULL,
			G_CALLBACK(reset_scanned_lines_cb) },
	{ "ResetAllConnections", NULL, N_("Reset all connections"),
			"<shift>f", NULL,
			G_CALLBACK(reset_all_connections_cb) },
	{ "OptimizeRatsNest", NULL, N_("Optimize rats nest"),
			"o", NULL,
			G_CALLBACK(optimize_rats_nest_cb) },
	{ "EraseRatsNest", NULL, N_("Erase rats nest"),
			"e", NULL,
			G_CALLBACK(erase_rats_nest_cb) },
	{ "AutoRouteSelectedRats", NULL, N_("Auto route selected rats"), NULL, NULL,
			G_CALLBACK(auto_route_selected_rats_cb) },
	{ "AutoRouteAllRats", NULL, N_("Auto route all rats"), NULL, NULL,
			G_CALLBACK(auto_route_all_rats_cb) },
	{ "RipUpAutoRouted", NULL, N_("Rip up all auto routed tracks"), NULL, NULL,
			G_CALLBACK(rip_up_auto_routed_cb) },
	{ "OptimizeTracksMenu", NULL, N_("Optimize routed tracks") },
	{ "AutoOptimize", NULL, N_("Auto optimize"),
			/* "<shift>="*/ NULL, NULL,
			G_CALLBACK(optimize_routes_cb) },
	{ "Debumpify", NULL, N_("Debumpify"), NULL, NULL,
			G_CALLBACK(optimize_routes_cb) },
	{ "Unjaggy", NULL, N_("Unjaggy"), NULL, NULL,
			G_CALLBACK(optimize_routes_cb) },
	{ "ViaNudge", NULL, N_("Via nudge"), NULL, NULL,
			G_CALLBACK(optimize_routes_cb) },
	{ "ViaTrim", NULL, N_("Via trim"), NULL, NULL,
			G_CALLBACK(optimize_routes_cb) },
	{ "OrthoPull", NULL, N_("Ortho pull"), NULL, NULL,
			G_CALLBACK(optimize_routes_cb) },
	{ "SimpleOpts", NULL, N_("Simple optimizations"), NULL, NULL,
			G_CALLBACK(optimize_routes_cb) },
	{ "Miter", NULL, N_("Miter"), NULL, NULL,
			G_CALLBACK(optimize_routes_cb) },
	{ "DesignRuleCheck", NULL, N_("Design Rule Checker"), NULL, NULL,
			G_CALLBACK(design_rule_check_cb) },
	{ "ApplyVendorMapping", NULL, N_("Apply vendor drill mapping"), NULL, NULL,
			G_CALLBACK(apply_vendor_mapping_cb) },

/* InfoMenu */
	{ "InfoMenu", NULL, N_("Info") },
	{ "ObjectReport", NULL, N_("Generate object report"),
			"<control>r", NULL,
			G_CALLBACK(object_report_cb) },
	{ "DrillSummary", NULL, N_("Generate drill summary"), NULL, NULL,
			G_CALLBACK(drill_summary_cb) },
	{ "FoundPinsPads", NULL, N_("Report found pins/pads"), NULL, NULL,
			G_CALLBACK(found_pins_pads_cb) },

/* WindowMenu */
	{ "WindowMenu", NULL, N_("Window") },
	{ "LibraryWindow", NULL, N_("Library"), NULL, NULL,
			G_CALLBACK(library_window_cb) },
	{ "MessageLogWindow", NULL, N_("Message Log"), NULL, NULL,
			G_CALLBACK(message_window_cb) },
	{ "NetlistWindow", NULL, N_("Netlist"), NULL, NULL,
			G_CALLBACK(netlist_window_cb) },
	{ "CommandWindow", NULL, N_("Command Entry"), NULL, NULL,
			G_CALLBACK(command_entry_cb) },
	{ "KeyrefWindow", NULL, N_("Key Reference"), NULL, NULL,
			G_CALLBACK(keyref_window_cb) },
	{ "AboutDialog", NULL, N_("About"), NULL, NULL,
			G_CALLBACK(about_dialog_cb) },

/* PopupMenu */
	{ "SelectionOperationMenu", NULL, N_("Operations on selections") },
	{ "LocationOperationMenu", NULL, N_("Operations on this location") },
	{ "SelectToolMenu", NULL, N_("Select tool") },

	};

static gint n_entries = G_N_ELEMENTS(entries);



  /* These get NULL labels because labels are dynamically set in
  |  gui_change_selected_update_menu_actions()
  */
static GtkActionEntry change_selected_entries[] =
	{
	/* name, stock_id, label, accelerator, tooltip callback */
	{ "-LinesChange", NULL, NULL, NULL, NULL,
			G_CALLBACK(selected_lines_size_change_cb) },
	{ "+LinesChange", NULL, NULL, NULL, NULL,
			G_CALLBACK(selected_lines_size_change_cb) },
	{ "-PadsChange", NULL, NULL, NULL, NULL,
			G_CALLBACK(selected_pads_size_change_cb) },
	{ "+PadsChange", NULL, NULL, NULL, NULL,
			G_CALLBACK(selected_pads_size_change_cb) },
	{ "-PinsChange", NULL, NULL, NULL, NULL,
			G_CALLBACK(selected_pins_size_change_cb) },
	{ "+PinsChange", NULL, NULL, NULL, NULL,
			G_CALLBACK(selected_pins_size_change_cb) },
	{ "-TextChange", NULL, NULL, NULL, NULL,
			G_CALLBACK(selected_text_size_change_cb) },
	{ "+TextChange", NULL, NULL, NULL, NULL,
			G_CALLBACK(selected_text_size_change_cb) },
	{ "-ViasChange", NULL, NULL, NULL, NULL,
			G_CALLBACK(selected_vias_size_change_cb) },
	{ "+ViasChange", NULL, NULL, NULL, NULL,
			G_CALLBACK(selected_vias_size_change_cb) },

	{ "-ViasDrillChange", NULL, NULL, NULL, NULL,
			G_CALLBACK(selected_vias_drill_change_cb) },
	{ "+ViasDrillChange", NULL, NULL, NULL, NULL,
			G_CALLBACK(selected_vias_drill_change_cb) },
	{ "-PinsDrillChange", NULL, NULL, NULL, NULL,
			G_CALLBACK(selected_pins_drill_change_cb) },
	{ "+PinsDrillChange", NULL, NULL, NULL, NULL,
			G_CALLBACK(selected_pins_drill_change_cb) },
	};

static gint n_change_selected_entries = G_N_ELEMENTS(change_selected_entries);


static GtkRadioActionEntry radio_grid_mil_setting_entries[] =
	{
	/* name, stock_id, label, accelerator, tooltip, value */
	{ "grid0",	NULL, "0.1 mil", NULL, NULL, 0 },
	{ "grid1",	NULL, "0.5 mil", NULL, NULL, 1 },
	{ "grid2",	NULL, "1 mil",   NULL, NULL, 2 },
	{ "grid3",	NULL, "5 mil",   NULL, NULL, 3 },
	{ "grid4",	NULL, "10 mil",  NULL, NULL, 4 },
	{ "grid5",	NULL, "25 mil",  NULL, NULL, 5 },
	{ "grid6",	NULL, "50 mil",  NULL, NULL, 6 },
	{ "grid7",	NULL, "100 mil", NULL, NULL, 7 }
	};

static gint n_radio_grid_mil_setting_entries
			= G_N_ELEMENTS(radio_grid_mil_setting_entries);


static GtkRadioActionEntry radio_grid_mm_setting_entries[] =
	{
	/* name, stock_id, label, accelerator, tooltip, value */
	{ "grid0",	NULL, "0.01 mm",  NULL, NULL, 0 },
	{ "grid1",	NULL, "0.02 mm",  NULL, NULL, 1 },
	{ "grid2",	NULL, "0.05 mm",  NULL, NULL, 2 },
	{ "grid3",	NULL, "0.1 mm",  NULL, NULL,  3 },
	{ "grid4",	NULL, "0.2 mm",  NULL, NULL,  4 },
	{ "grid5",	NULL, "0.5 mm",  NULL, NULL,  5 },
	{ "grid6",	NULL, "1 mm",    NULL, NULL,  6 },
	{ "grid7",	NULL, "2 mm",    NULL, NULL,  7 },
	};

static gint n_radio_grid_mm_setting_entries
			= G_N_ELEMENTS(radio_grid_mm_setting_entries);


static GtkRadioActionEntry radio_displayed_element_name_entries[] =
	{
	/* name, stock_id, label, accelerator, tooltip, value */
	{ "Description",  NULL, N_("Description"),  NULL, NULL, 0 },
	{ "ReferenceDesignator",  NULL, N_("Reference designator"),
				NULL, NULL, 1 },
	{ "Value",  NULL, N_("Value"),  NULL, NULL, 2 }
	};

static gint n_radio_displayed_element_name_entries
			= G_N_ELEMENTS(radio_displayed_element_name_entries);


static GtkRadioActionEntry radio_select_current_buffer_entries[] =
	{
	/* name, stock_id, label, accelerator, tooltip, value */
	{ "SelectBuffer1", NULL, "#1", "<control>1", NULL, 1 },
	{ "SelectBuffer2", NULL, "#2", "<control>2", NULL, 2 },
	{ "SelectBuffer3", NULL, "#3", "<control>3", NULL, 3 },
	{ "SelectBuffer4", NULL, "#4", "<control>4", NULL, 4 },
	{ "SelectBuffer5", NULL, "#5", "<control>5", NULL, 5 }
	};

static gint n_radio_select_current_buffer_entries
			= G_N_ELEMENTS(radio_select_current_buffer_entries);

static GtkRadioActionEntry radio_select_tool_entries[] =
	{
	/* name, stock_id, label, accelerator, tooltip, value */
	{ "SelectLineTool", NULL, N_("Line"), NULL, NULL, 0 },
	{ "SelectViaTool", NULL, N_("Via"), NULL, NULL, 0 },
	{ "SelectRectangleTool", NULL, N_("Rectangle"), NULL, NULL, 0 },
	{ "SelectSelectionTool", NULL, N_("Selection"), NULL, NULL, 0 },
	{ "SelectTextTool", NULL, N_("Text"), NULL, NULL, 0 },
	{ "SelectPannerTool", NULL, N_("Panner"), NULL, NULL, 0 },
	};

static gint n_radio_select_tool_entries
			= G_N_ELEMENTS(radio_select_tool_entries);


static GtkToggleActionEntry toggle_entries[] =
	{
	/* name, stock_id, label, accelerator, tooltip, callback, is_active */

	/* ScreenMenu */
	{ "ToggleDrawGrid", NULL, N_("Enable visible grid"), NULL, NULL,
			G_CALLBACK(toggle_draw_grid_cb), FALSE },
	{ "ToggleGridUnitsMm", NULL, N_("Enable millimeter grid units"), NULL, NULL,
			G_CALLBACK(toggle_grid_units_cb), FALSE },
	{ "ToggleViewSolderSide", NULL, N_("Enable view solder side"),
			"<shift>b", NULL,
			G_CALLBACK(toggle_view_solder_side_cb) },
	{ "ToggleShowSolderMask", NULL, N_("Enable view soldermask"), NULL, NULL,
			G_CALLBACK(toggle_show_solder_mask_cb) },
	{ "TogglePinoutShowsNumber", NULL, N_("Enable pinout shows number"),
			NULL, NULL,
			G_CALLBACK(toggle_pinout_shows_number_cb) },

/* SettingsMenu */
	{ "Toggle45degree", NULL, N_("Enable all direction lines"), NULL, NULL,
			G_CALLBACK(toggle_45_degree_cb) },
	{ "ToggleStartDirection", NULL, N_("Enable auto swap line start angle"),
			NULL, NULL,
			G_CALLBACK(toggle_start_direction_cb) },
	{ "ToggleOrthogonalMoves", NULL, N_("Enable orthogonal moves"), NULL, NULL,
			G_CALLBACK(toggle_orthogonal_moves_cb) },
	{ "ToggleSnapPin", NULL, N_("Enable crosshair snaps to pins and pads"),
			NULL, NULL,
			G_CALLBACK(toggle_snap_pin_cb) },
	{ "ToggleShowDRC", NULL, N_("Enable crosshair shows DRC clearance"),
			NULL, NULL,
			G_CALLBACK(toggle_show_DRC_cb) },
	{ "ToggleAutoDrC", NULL, N_("Enable auto enforce DRC clearance"),
			NULL, NULL,
			G_CALLBACK(toggle_auto_DRC_cb) },
	{ "ToggleRubberBand", NULL, N_("Enable rubber band mode"), NULL, NULL,
			G_CALLBACK(toggle_rubber_band_cb) },
	{ "ToggleUniqueNames", NULL, N_("Enable require unique element names"),
			NULL, NULL,
			G_CALLBACK(toggle_unique_names_cb) },
	{ "ToggleLocalRef", NULL, N_("Enable auto zero delta measurements"),
			NULL, NULL,
			G_CALLBACK(toggle_local_ref_cb) },
	{ "ToggleClearLine", NULL, N_("Enable new lines, arcs clear polygons"),
			NULL, NULL,
			G_CALLBACK(toggle_clear_line_cb) },
	{ "ToggleLiveRoute", NULL, N_("Enable show autorouter trials"), NULL, NULL,
			G_CALLBACK(toggle_live_route_cb) },
	{ "ToggleThinDraw", NULL, N_("Enable thin line draw"),
			NULL /* Gtk can't take '\' or '|' */, NULL,
			G_CALLBACK(toggle_thin_draw_cb) },
	{ "ToggleCheckPlanes", NULL, N_("Enable check polygons"), NULL, NULL,
			G_CALLBACK(toggle_check_planes_cb) },
	{ "ToggleVendorDrillMapping", NULL, N_("Enable vendor drill mapping"),
			NULL, NULL,
			G_CALLBACK(toggle_vendor_drill_mapping_cb) },

/* ConnectsMenu */
	{ "ToggleOnlyAutoRoutedNets", NULL, N_("Enable only autorouted nets"),
			NULL, NULL,
			G_CALLBACK(toggle_only_auto_routed_cb) },
	};

static gint n_toggle_entries = G_N_ELEMENTS(toggle_entries);



/* ======================================================================
|  Here is the ui_manager string that defines the order of items displayed
|  in the menus.
*/

static const gchar *ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"		<menu action='FileMenu'>"
"			<menuitem action='SaveLayout'/>"
"			<menuitem action='SaveLayoutAs'/>"
"			<separator/>"
"			<menuitem action='LoadLayout'/>"
"			<menuitem action='LoadElementData'/>"
"			<menuitem action='LoadLayoutData'/>"
"			<menuitem action='LoadNetlistFile'/>"
"			<menuitem action='LoadVendorFile'/>"
"			<separator/>"
"			<menu action='SaveConnectionMenu'>"
"				<menuitem action='SingleElement'/>"
"				<menuitem action='AllElements'/>"
"				<menuitem action='UnusedPins'/>"
"			</menu>"
"			<separator/>"
"			<menuitem action='PrintLayout'/>"
"			<separator/>"
"			<menuitem action='NewLayout'/>"
"			<separator/>"
"			<menuitem action='Preferences'/>"
"			<separator/>"
"			<menuitem action='Quit'/>"
"		</menu>"

"		<menu action='EditMenu'>"
"			<menuitem action='Undo'/>"
"			<menuitem action='Redo'/>"
"			<menuitem action='ClearUndo'/>"
"			<separator/>"
"			<menuitem action='CutSelectionToBuffer'/>"
"			<menuitem action='CopySelectionToBuffer'/>"
"			<menuitem action='PasteBufferToLayout'/>"
"			<separator/>"
"			<menuitem action='UnselectAll'/>"
"			<menuitem action='SelectAll'/>"
"			<separator/>"
"			<menu action='EditNamesMenu'>"
"				<menuitem action='EditTextOnLayout'/>"
"				<menuitem action='EditNameOfLayout'/>"
"				<menuitem action='EditNameOfActiveLayer'/>"
"			</menu>"
"		</menu>"

"		<menu action='ScreenMenu'>"
"			<menuitem action='RedrawLayout'/>"
"			<menuitem action='CenterLayout'/>"
"			<menuitem action='RealignGrid'/>"
"			<separator/>"
"			<menuitem action='ToggleViewSolderSide'/>"
"			<menuitem action='ToggleShowSolderMask'/>"
"			<separator/>"
"			<menuitem action='ToggleDrawGrid'/>"
"			<menuitem action='ToggleGridUnitsMm'/>"
"			<menu action='GridSettingMenu'>"
"				<menuitem action='grid0'/>"
"				<menuitem action='grid1'/>"
"				<menuitem action='grid2'/>"
"				<menuitem action='grid3'/>"
"				<menuitem action='grid4'/>"
"				<menuitem action='grid5'/>"
"				<menuitem action='grid6'/>"
"				<menuitem action='grid7'/>"
"			</menu>"
"			<separator/>"
"			<menuitem action='ZoomIn'/>"
"			<menuitem action='ZoomOut'/>"
"			<separator/>"
"			<menu action='DisplayedElementNameMenu'>"
"				<menuitem action='Description'/>"
"				<menuitem action='ReferenceDesignator'/>"
"				<menuitem action='Value'/>"
"			</menu>"
"			<separator/>"
"			<menuitem action='TogglePinoutShowsNumber'/>"
"			<menuitem action='PinoutMenu'/>"
"		</menu>"

"		<menu action='SizesMenu'>"
"		</menu>"

"		<menu action='SettingsMenu'>"
"			<menuitem action='Toggle45degree'/>"
"			<menuitem action='ToggleStartDirection'/>"
"			<menuitem action='ToggleOrthogonalMoves'/>"
"			<menuitem action='ToggleSnapPin'/>"
"			<menuitem action='ToggleShowDRC'/>"
"			<menuitem action='ToggleAutoDrC'/>"
"			<separator/>"
"			<menuitem action='ToggleRubberBand'/>"
"			<menuitem action='ToggleUniqueNames'/>"
"			<menuitem action='ToggleLocalRef'/>"
"			<menuitem action='ToggleClearLine'/>"
"			<menuitem action='ToggleLiveRoute'/>"
"			<menuitem action='ToggleThinDraw'/>"
"			<menuitem action='ToggleCheckPlanes'/>"
"			<separator/>"
"			<menuitem action='ToggleVendorDrillMapping'/>"
"		</menu>"

"		<menu action='SelectMenu'>"
"			<menuitem action='SelectAll'/>"
"			<menuitem action='SelectAllConnected'/>"
"			<separator/>"
"			<menuitem action='UnselectAll'/>"
"			<menuitem action='UnselectAllConnected'/>"
"			<separator/>"
"			<menu action='SelectByNameMenu'>"
"				<menuitem action='SelectObjectsByName'/>"
"				<menuitem action='SelectElementsByName'/>"
"				<menuitem action='SelectPadsByName'/>"
"				<menuitem action='SelectPinsByName'/>"
"				<menuitem action='SelectTextByName'/>"
"				<menuitem action='SelectViasByName'/>"
"			</menu>"
"			<separator/>"
"			<menuitem action='AutoPlaceSelected'/>"
"			<menuitem action='DisperseAllElements'/>"
"			<menuitem action='DisperseSelectedElements'/>"
"			<separator/>"
"			<menuitem action='MoveSelectedOtherSide'/>"
"			<menuitem action='RemoveSelected'/>"
"			<separator/>"
"			<menuitem action='ConvertSelectionToElement'/>"
"			<separator/>"
"			<menuitem action='OptimizeSelectedRats'/>"
"			<menuitem action='AutoRouteSelectedRats'/>"
"			<menuitem action='RipUpSelectedTracks'/>"
"			<separator/>"
"			<menu action='ChangeSelectedSizeMenu'>"
"				<menuitem action='-LinesChange'/>"
"				<menuitem action='+LinesChange'/>"
"				<menuitem action='-PadsChange'/>"
"				<menuitem action='+PadsChange'/>"
"				<menuitem action='-PinsChange'/>"
"				<menuitem action='+PinsChange'/>"
"				<menuitem action='-TextChange'/>"
"				<menuitem action='+TextChange'/>"
"				<menuitem action='-ViasChange'/>"
"				<menuitem action='+ViasChange'/>"
"			</menu>"
"			<menu action='ChangeSelectedDrillMenu'>"
"				<menuitem action='-ViasDrillChange'/>"
"				<menuitem action='+ViasDrillChange'/>"
"				<menuitem action='-PinsDrillChange'/>"
"				<menuitem action='+PinsDrillChange'/>"
"			</menu>"
"			<menu action='ChangeSelectedSquareMenu'>"
"				<menuitem action='ChangeSquareElements'/>"
"				<menuitem action='ChangeSquarePins'/>"
"			</menu>"
"		</menu>"

"		<menu action='BufferMenu'>"
"			<menuitem action='CopySelectionToBuffer'/>"
"			<menuitem action='CutSelectionToBuffer'/>"
"			<menuitem action='PasteBufferToLayout'/>"
"			<separator/>"
"			<menuitem action='RotateBufferCCW'/>"
"			<menuitem action='RotateBufferCW'/>"
"			<menuitem action='MirrorBufferUpDown'/>"
"			<menuitem action='MirrorBufferLeftRight'/>"
"			<separator/>"
"			<menuitem action='ClearBuffer'/>"
"			<menuitem action='ConvertBufferToElement'/>"
"			<menuitem action='BreakBufferElements'/>"
"			<menuitem action='SaveBufferElements'/>"
"			<separator/>"
"			<menu action='SelectCurrentBufferMenu'>"
"				<menuitem action='SelectBuffer1'/>"
"				<menuitem action='SelectBuffer2'/>"
"				<menuitem action='SelectBuffer3'/>"
"				<menuitem action='SelectBuffer4'/>"
"				<menuitem action='SelectBuffer5'/>"
"			</menu>"
"		</menu>"

"		<menu action='ConnectsMenu'>"
"			<menuitem action='LookupConnections'/>"
"			<menuitem action='ResetScannedPads'/>"
"			<menuitem action='ResetScannedLines'/>"
"			<menuitem action='ResetAllConnections'/>"
"			<separator/>"
"			<menuitem action='OptimizeRatsNest'/>"
"			<menuitem action='EraseRatsNest'/>"
"			<separator/>"
"			<menuitem action='AutoRouteSelectedRats'/>"
"			<menuitem action='AutoRouteAllRats'/>"
"			<menuitem action='RipUpAutoRouted'/>"
"			<separator/>"
"			<menu action='OptimizeTracksMenu'>"
"				<menuitem action='AutoOptimize'/>"
"				<menuitem action='Debumpify'/>"
"				<menuitem action='Unjaggy'/>"
"				<menuitem action='ViaNudge'/>"
"				<menuitem action='ViaTrim'/>"
"				<menuitem action='OrthoPull'/>"
"				<menuitem action='SimpleOpts'/>"
"				<menuitem action='Miter'/>"
"				<separator/>"
"				<menuitem action='ToggleOnlyAutoRoutedNets'/>"
"				</menu>"
"			<separator/>"
"			<menuitem action='DesignRuleCheck'/>"
"			<separator/>"
"			<menuitem action='ApplyVendorMapping'/>"
"		</menu>"

"		<menu action='InfoMenu'>"
"			<menuitem action='ObjectReport'/>"
"			<menuitem action='DrillSummary'/>"
"			<menuitem action='FoundPinsPads'/>"
"		</menu>"

"		<menu action='WindowMenu'>"
"			<menuitem action='LibraryWindow'/>"
"			<menuitem action='MessageLogWindow'/>"
"			<menuitem action='NetlistWindow'/>"
"			<menuitem action='CommandWindow'/>"
"			<menuitem action='KeyrefWindow'/>"
"			<separator/>"
"			<menuitem action='AboutDialog'/>"
"		</menu>"
"	</menubar>"

"	<popup name='PopupMenu'>"
"		<menu action='SelectionOperationMenu'>"
"			<menuitem action='UnselectAll' />"
"			<menuitem action='RemoveSelected' />"
"			<menuitem action='CopySelectionToBuffer' />"
"			<menuitem action='CutSelectionToBuffer' />"
"			<menuitem action='ConvertSelectionToElement' />"
/*"			<menuitem action='BreakElement' />"*/
"			<menuitem action='AutoPlaceSelected' />"
"			<menuitem action='AutoRouteSelectedRats' />"
"			<menuitem action='RipUpSelectedTracks' />"
"		</menu>"
"		<menu action='LocationOperationMenu'>"
/*"			<menuitem action='ToggleNameVisibility' />"*/
/*"			<menuitem action='EditName' />"*/
"			<menuitem action='ObjectReport' />"
/*"			<menuitem action='RotateObjectCCW' />"*/
/*"			<menuitem action='RotateObjectCW' />"*/
/*"			<menuitem action='SendToOtherSide' />"*/
/*"			<menuitem action='ToggleThermal' />"*/
/*"			<menuitem action='LookupConnections' />"*/
"		</menu>"
"		<separator/>"
"		<menuitem action='Undo' />"
"		<menuitem action='Redo' />"
"		<separator/>"
"		<menu action='SelectToolMenu'>"
"			<menuitem action='SelectLineTool' />"
"			<menuitem action='SelectViaTool' />"
"			<menuitem action='SelectRectangleTool' />"
"			<menuitem action='SelectSelectionTool' />"
"			<menuitem action='SelectTextTool' />"
"			<menuitem action='SelectPannerTool' />"
"		</menu>"
#if 0
"		<menuitem name='New' action='SaveLayoutAs' position='top' />"
"		<menu name='HelpMenu' action='OptimizeTracksMenu'>"
"			<menuitem name='About' action='FoundPinsPads' />"
"		</menu>"
#endif
"	</popup>"

"</ui>";


  /* When user toggles grid units mil<->mm or when a new layout is loaded
  |  that might use different units, the "Change size of selected" menu
  |  displayed text muxt be changed.
  */
void
gui_change_selected_update_menu_actions(void)
	{
	gchar		size_buf[32], buf[128];

	if (gui->change_selected_actions)
		{
		/* Remove the existing change selected actions from the menu.
		*/
		gtk_ui_manager_remove_action_group(gui->ui_manager,
					gui->change_selected_actions);
		g_object_unref(gui->change_selected_actions);
		}

	/* And create a new action group for the changed selection actions.
	*/
	gui->change_selected_actions = gtk_action_group_new("ChangeSelActions");
	gtk_action_group_set_translation_domain(gui->change_selected_actions,NULL);
	gtk_ui_manager_insert_action_group(gui->ui_manager,
			gui->change_selected_actions, 0);

	/* Update the labels to match current units and increment values settings.
	*/
	if (Settings.grid_units_mm)
		snprintf(size_buf, sizeof(size_buf), "%.2f mm",
				 Settings.size_increment_mm);
	else
		snprintf(size_buf, sizeof(size_buf), "%.0f mil",
				Settings.size_increment_mil);

	snprintf(buf, sizeof(buf), _("Decrement lines by %s"), size_buf);
	dup_string((gchar **) &change_selected_entries[0].label, buf);

	snprintf(buf, sizeof(buf), _("Increment lines by %s"), size_buf);
	dup_string((gchar **) &change_selected_entries[1].label, buf);

	snprintf(buf, sizeof(buf), _("Decrement pads by %s"), size_buf);
	dup_string((gchar **) &change_selected_entries[2].label, buf);

	snprintf(buf, sizeof(buf), _("Increment pads by %s"), size_buf);
	dup_string((gchar **) &change_selected_entries[3].label, buf);

	snprintf(buf, sizeof(buf), _("Decrement pins by %s"), size_buf);
	dup_string((gchar **) &change_selected_entries[4].label, buf);

	snprintf(buf, sizeof(buf), _("Increment pins by %s"), size_buf);
	dup_string((gchar **) &change_selected_entries[5].label, buf);

	snprintf(buf, sizeof(buf), _("Decrement text by %s"), size_buf);
	dup_string((gchar **) &change_selected_entries[6].label, buf);

	snprintf(buf, sizeof(buf), _("Increment text by %s"), size_buf);
	dup_string((gchar **) &change_selected_entries[7].label, buf);

	snprintf(buf, sizeof(buf), _("Decrement vias by %s"), size_buf);
	dup_string((gchar **) &change_selected_entries[8].label, buf);

	snprintf(buf, sizeof(buf), _("Increment vias by %s"), size_buf);
	dup_string((gchar **) &change_selected_entries[9].label, buf);

	/* -- Drill size changes */
	snprintf(buf, sizeof(buf), _("Decrement vias by %s"), size_buf);
	dup_string((gchar **) &change_selected_entries[10].label, buf);

	snprintf(buf, sizeof(buf), _("Increment vias by %s"), size_buf);
	dup_string((gchar **) &change_selected_entries[11].label, buf);

	snprintf(buf, sizeof(buf), _("Decrement pins by %s"), size_buf);
	dup_string((gchar **) &change_selected_entries[12].label, buf);

	snprintf(buf, sizeof(buf), _("Increment pins by %s"), size_buf);
	dup_string((gchar **) &change_selected_entries[13].label, buf);

	/* And add the actions with new labels back in.
	*/
	gtk_action_group_add_actions(gui->change_selected_actions,
				change_selected_entries, n_change_selected_entries, &Output);
	}

  /* Grid setting labels must also match user and new layout unit changes.
  */
void
gui_grid_setting_update_menu_actions(void)
	{
	gint		i;

	if (gui->grid_actions)
		{
		/* Remove the existing radio grid actions from the menu.
		*/
		gtk_ui_manager_remove_action_group(gui->ui_manager, gui->grid_actions);
		g_object_unref(gui->grid_actions);
		}

	/* And add back actions appropriate for mil or mm grid settings.
	*/
	gui->grid_actions = gtk_action_group_new("GridActions");
	gtk_action_group_set_translation_domain(gui->grid_actions, NULL);
	gtk_ui_manager_insert_action_group(gui->ui_manager, gui->grid_actions, 0);

	/* Get the index of the radio button to set depending on current
	|  PCB Grid value.  But if user hits 'g' key and no grid index matches,
	|  'i' will be -1 and no button will be set active.  At least Gtk docs
	|  say so, but I see different.
	*/
	i = get_grid_value_index(TRUE);

	if (Settings.grid_units_mm)
		gtk_action_group_add_radio_actions(gui->grid_actions,
				radio_grid_mm_setting_entries,
				n_radio_grid_mm_setting_entries,
				i,
				G_CALLBACK(radio_grid_mm_setting_cb), NULL);
	else
		gtk_action_group_add_radio_actions(gui->grid_actions,
				radio_grid_mil_setting_entries,
				n_radio_grid_mil_setting_entries,
				i,
				G_CALLBACK(radio_grid_mil_setting_cb), NULL);
	}


  /* When a new layout is loaded, must set the radio state to the current
  |  "Displayed element name".  Now I unload and reload the actions so
  |  an initial value can be set, but there must be a better way?
  */
static void
update_displayed_name_actions(void)
	{
	gint		i;

	if (gui->displayed_name_actions)
		{
		/* Remove the existing radio actions from the menu.
		*/
		gtk_ui_manager_remove_action_group(gui->ui_manager,
								gui->displayed_name_actions);
		g_object_unref(gui->displayed_name_actions);
		}

	/* And add back actions just to get the initial one set.
	*/
	gui->displayed_name_actions = gtk_action_group_new("DispNameActions");
	gtk_action_group_set_translation_domain(gui->displayed_name_actions, NULL);
	gtk_ui_manager_insert_action_group(gui->ui_manager,
				gui->displayed_name_actions, 0);

	/* Get the index of the radio button to set
	*/
	if (TEST_FLAG (DESCRIPTIONFLAG, PCB))
		i = 0;
	else if (TEST_FLAG (NAMEONPCBFLAG, PCB))
		i = 1;
	else
		i = 2;

	gtk_action_group_add_radio_actions(gui->displayed_name_actions,
			radio_displayed_element_name_entries,
			n_radio_displayed_element_name_entries,
			i,
			G_CALLBACK(radio_displayed_element_name_cb), NULL);
	}


  /* Sync toggle states that were saved with the layout and notify the
  |  config code to update Settings values it manages.
  */
void
gui_sync_with_new_layout(void)
	{
	GtkAction	*action;
	gboolean	old_holdoff;

	/* Just want to update the state of the menus without calling the
	|  action functions at this time because causing a toggle action can
	|  undo the initial condition set we want here.
	*/
	old_holdoff = gui->toggle_holdoff;
	gui->toggle_holdoff = TRUE;

	action = gtk_action_group_get_action(gui->main_actions, "ToggleDrawGrid");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), Settings.DrawGrid);
/*	g_object_set(action, "sensitive", Settings.XXX, NULL); */

	action = gtk_action_group_get_action(gui->main_actions,
				"ToggleGridUnitsMm");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				Settings.grid_units_mm);

	/* Toggle actions in the menu which set a PCB flag must be set to
	|  the new layout PCB flag states.  Transient toggle buttons which
	|  do not set a PCB flag don't need setting here.
	*/
	action = gtk_action_group_get_action(gui->main_actions,
				"TogglePinoutShowsNumber");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				TEST_FLAG(SHOWNUMBERFLAG, PCB));

	action = gtk_action_group_get_action(gui->main_actions,
				"Toggle45degree");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				TEST_FLAG(ALLDIRECTIONFLAG, PCB));

	action = gtk_action_group_get_action(gui->main_actions,
				"ToggleRubberBand");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				TEST_FLAG(RUBBERBANDFLAG, PCB));

	action = gtk_action_group_get_action(gui->main_actions,
				"ToggleStartDirection");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				TEST_FLAG(SWAPSTARTDIRFLAG, PCB));

	action = gtk_action_group_get_action(gui->main_actions,
				"ToggleUniqueNames");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				TEST_FLAG(UNIQUENAMEFLAG, PCB));

	action = gtk_action_group_get_action(gui->main_actions,
				"ToggleSnapPin");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				TEST_FLAG(SNAPPINFLAG, PCB));

	action = gtk_action_group_get_action(gui->main_actions,
				"ToggleClearLine");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				TEST_FLAG(CLEARNEWFLAG, PCB));

	action = gtk_action_group_get_action(gui->main_actions,
				"ToggleOrthogonalMoves");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				TEST_FLAG(ORTHOMOVEFLAG, PCB));

	action = gtk_action_group_get_action(gui->main_actions,
				"ToggleLiveRoute");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				TEST_FLAG(LIVEROUTEFLAG, PCB));

	action = gtk_action_group_get_action(gui->main_actions,
				"ToggleShowDRC");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				TEST_FLAG(SHOWDRCFLAG, PCB));

	action = gtk_action_group_get_action(gui->main_actions,
				"ToggleAutoDrC");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				TEST_FLAG(AUTODRCFLAG, PCB));

	/* Not sure if I can set a radio button without loading actions, so
	|  load the actions.
	*/
	gui_grid_setting_update_menu_actions();
	update_displayed_name_actions();

	gui->toggle_holdoff = old_holdoff;

	pcb_use_route_style(&PCB->RouteStyle[0]);
	gui_route_style_button_set_active(0);
	gui_config_handle_units_changed();
	gui_change_selected_update_menu_actions();

	gui_zoom_display_update();
	}

  /* Sync toggle states in the menus at startup to Settings values loaded
  |  in the config.
  */
void
gui_init_toggle_states(void)
	{
	GtkAction	*action;
	gboolean	old_holdoff;

	/* Just want to update the state of the menus without calling the
	|  action functions at this time because causing a toggle action can
	|  undo the initial condition set we want here.
	*/
	old_holdoff = gui->toggle_holdoff;
	gui->toggle_holdoff = TRUE;

	action = gtk_action_group_get_action(gui->main_actions, "ToggleDrawGrid");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), Settings.DrawGrid);

	action = gtk_action_group_get_action(gui->main_actions,
				"ToggleGridUnitsMm");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				Settings.grid_units_mm);

	action = gtk_action_group_get_action(gui->main_actions,
				"TogglePinoutShowsNumber");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				Settings.ShowNumber);

	action = gtk_action_group_get_action(gui->main_actions,
				"Toggle45degree");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				Settings.AllDirectionLines);

	action = gtk_action_group_get_action(gui->main_actions,
				"ToggleRubberBand");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				Settings.RubberBandMode);

	action = gtk_action_group_get_action(gui->main_actions,
				"ToggleStartDirection");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				Settings.SwapStartDirection);

	action = gtk_action_group_get_action(gui->main_actions,
				"ToggleUniqueNames");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				Settings.UniqueNames);

	action = gtk_action_group_get_action(gui->main_actions,
				"ToggleSnapPin");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				Settings.SnapPin);

	action = gtk_action_group_get_action(gui->main_actions,
				"ToggleClearLine");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				Settings.ClearLine);

	action = gtk_action_group_get_action(gui->main_actions,
				"ToggleOrthogonalMoves");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				Settings.OrthogonalMoves);

	action = gtk_action_group_get_action(gui->main_actions,
				"ToggleLiveRoute");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				Settings.liveRouting);

	action = gtk_action_group_get_action(gui->main_actions,
				"ToggleShowDRC");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				Settings.ShowDRC);

	action = gtk_action_group_get_action(gui->main_actions,
				"ToggleAutoDrC");
 	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action),
				Settings.AutoDRC);

	gui->toggle_holdoff = old_holdoff;
	}

  /* The intial loading of all actions at startup.
  */
static void
make_menu_actions(GtkActionGroup *actions, OutputType *out)
	{
	gtk_action_group_add_actions(actions, entries, n_entries, out);

	/* Handle menu actions with dynamic content.
	*/
	gui_change_selected_update_menu_actions();
	gui_grid_setting_update_menu_actions();
	update_displayed_name_actions();

	gtk_action_group_add_radio_actions(actions,
			radio_select_current_buffer_entries,
			n_radio_select_current_buffer_entries,
			0,
			G_CALLBACK(radio_select_current_buffer_cb), NULL);

	gtk_action_group_add_radio_actions(actions,
			radio_select_tool_entries,
			n_radio_select_tool_entries,
			0,
			G_CALLBACK(radio_select_tool_cb), NULL);

	gtk_action_group_add_toggle_actions(actions,
			toggle_entries, n_toggle_entries,
			out);
	}

  /* Make a frame for the top menubar, load in actions for the menus and
  |  load the ui_manager string.
  */
static void
make_top_menubar(GtkWidget *hbox, OutputType *out)
	{
	GtkUIManager	*ui;
	GtkWidget		*frame;
	GtkActionGroup	*actions;
	GError			*error = NULL;


	frame = gtk_frame_new(NULL);
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);

	ui = gtk_ui_manager_new();
	gui->ui_manager = ui;

	actions = gtk_action_group_new("Actions");
	gtk_action_group_set_translation_domain(actions, NULL);
	gui->main_actions = actions;
	make_menu_actions(actions, out);

	gtk_ui_manager_insert_action_group(ui, actions, 0);
	gtk_window_add_accel_group(GTK_WINDOW(out->top_window),
			gtk_ui_manager_get_accel_group(ui));

	/* For user customization, we could add
	|  gtk_menu_item_set_accel_path(), gtk_accel_map_save (), etc
	|  But probably can't do this because of command combo box interaction.
	*/

	if (!gtk_ui_manager_add_ui_from_string (ui, ui_info, -1, &error))
		{
		g_message("building menus failed: %s", error->message);
		g_error_free(error);
		}

	gtk_container_add(GTK_CONTAINER(frame),
				gtk_ui_manager_get_widget(ui, "/MenuBar"));
	}


  /* Set the PCB name label.
  */
void
gui_output_set_name_label(gchar *name)
	{
	gchar	*str;

	str = g_strdup_printf(" <b><big>%s</big></b> ", name ? name : "Unnamed");
	gtk_label_set_markup(GTK_LABEL(gui->name_label), str);
	g_free(str);
	}

static void
make_cursor_position_labels(GtkWidget *hbox, OutputType *out)
	{
	GtkWidget	*frame, *label;

	/* The cursor position units label
	*/
	label = gtk_label_new("");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label),
				Settings.grid_units_mm ?
				"<b>mm</b> " : "<b>mil</b> ");
	gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gui->cursor_units_label = label;


	/* The absolute cursor position label
	*/
	frame = gtk_frame_new(NULL);
	gtk_box_pack_end(GTK_BOX(hbox), frame, FALSE, TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(frame), 2);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);

	label = gtk_label_new("");
	gtk_container_add(GTK_CONTAINER(frame), label);
	gui->cursor_position_absolute_label = label;

	/* The relative cursor position label
	*/
	frame = gtk_frame_new(NULL);
	gtk_box_pack_end(GTK_BOX(hbox), frame, FALSE, TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(frame), 2);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
	label = gtk_label_new(" __.__  __.__ ");
	gtk_container_add(GTK_CONTAINER(frame), label);
	gui->cursor_position_relative_label = label;
	}


/* ------------------------------------------------------------------
|  Handle the layer buttons.
*/
typedef struct
	{
	GtkWidget	*radio_select_button,
				*layer_enable_button,
				*layer_enable_ebox,
				*label;
	gchar		*text;
	gint		index;
	}
	LayerButtonSet;

static LayerButtonSet	layer_buttons[MAX_LAYER + 5];

static gint				layer_select_button_index;

static gboolean			layer_enable_button_cb_hold_off,
						layer_select_button_cb_hold_off;

static void
layer_select_button_cb(GtkWidget *widget, LayerButtonSet *lb)
	{
	gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

	if (!active || layer_select_button_cb_hold_off)
		return;

	ChangeGroupVisibility(lb->index, True, True);

/*	if (GTK_TOGGLE_BUTTON(widget)->active)
		printf("layer selected %d\n", lb->index); */

	layer_select_button_index = lb->index;

	UpdateAll();
	}

static void
layer_enable_button_cb(GtkWidget *widget, gpointer data)
	{
	LayerButtonSet	*lb;
	gint			i, group, layer = GPOINTER_TO_INT(data);
	gboolean		active, redraw = FALSE;

	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

	if (layer_enable_button_cb_hold_off)
		return;

	lb = &layer_buttons[layer];
	switch (layer)
		{
		case MAX_LAYER + 2: /* settings for pins */
			PCB->PinOn = active;
			redraw |= (PCB->Data->ElementN != 0);
			break;

		case MAX_LAYER + 3: /* settings for vias */
			PCB->ViaOn = active;
			redraw |= (PCB->Data->ViaN != 0);
			break;

		case MAX_LAYER + 4: /* settings for invisible objects */
			PCB->InvisibleObjectsOn = active;
			PCB->Data->BACKSILKLAYER.On = (active && PCB->ElementOn);
			redraw = True;
			break;

		default:
			/* check if active layer is in the group;
			|  if YES, make a different one active if possible.  Logic from
			|  Xt PCB code.
			*/
			if ((group = GetGroupOfLayer (layer)) ==
						GetGroupOfLayer (MIN (MAX_LAYER, INDEXOFCURRENT)))
				{
				for (i = (layer + 1) % (MAX_LAYER + 1); i != layer;
						i = (i + 1) % (MAX_LAYER + 1))
					if (PCB->Data->Layer[i].On == True &&
							GetGroupOfLayer (i) != group)
						break;
				if (i != layer)
					{
					ChangeGroupVisibility ((int) i, True, True);
					}
				else
					{
					/* everything else off, we can't turn this off too */
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
								TRUE);
					return;
					}
				}
			/* switch layer group on/off */
			ChangeGroupVisibility(layer, active, False);
			redraw = TRUE;
			break;
		}
	if (redraw)
		UpdateAll();
	}

static void
layer_button_set_color(LayerButtonSet *lb, GdkColor *color)
	{
	if (!lb->layer_enable_ebox)
		return;
	gtk_widget_modify_bg(lb->layer_enable_ebox, GTK_STATE_ACTIVE, color);
	gtk_widget_modify_bg(lb->layer_enable_ebox, GTK_STATE_PRELIGHT, color);

	gtk_widget_modify_fg(lb->label, GTK_STATE_ACTIVE,
				&Settings.WhiteColor);
	}

void
layer_enable_button_set_label(GtkWidget *label, gchar *text)
	{
	gchar	*s;

	if (Settings.small_layer_enable_label_markup)
		s = g_strdup_printf("<small>%s</small>", text);
	else
		s = g_strdup(text);
	gtk_label_set_markup(GTK_LABEL(label), s);
	g_free(s);
	}

  /* After layers comes some special cases.  Since silk and netlist (rats)
  |  are selectable as separate drawing areas, they are more consistently
  |  placed after the layers in the gui so the select radio buttons will
  |  be grouped.  This is different from Xt PCB which had a different looking
  |  select interface.
  */
static void
make_layer_buttons(GtkWidget *vbox, OutputType *out)
	{
	LayerButtonSet	*lb;
	GtkWidget		*table, *ebox, *label, *button;
	GdkColor		*color;
	GSList			*group = NULL;
	gchar			*text;
	gint			i;

	label = gtk_label_new(_("Layers"));
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 4);

	table = gtk_table_new(MAX_LAYER + 5, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

	for (i = 0; i < MAX_LAYER + 5; ++i)
		{
		lb = &layer_buttons[i];
		lb->index = i;

		if (i < GUI_N_SELECTABLE_LAYERS)
			{
			button = gtk_radio_button_new(group);
			group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
			gtk_table_attach_defaults(GTK_TABLE(table), button,
						0, 1, i, i + 1);

			lb->radio_select_button = button;
			g_signal_connect(G_OBJECT(button), "toggled",
						G_CALLBACK(layer_select_button_cb), lb);
			}

		switch (i)
			{
			case GUI_SILK_LAYER:	/* MAX_LAYER	*/
				color = PCB->ElementColor;
				text = _("silk");
				break;

			case GUI_RATS_LAYER:	/* MAX_LAYER + 1 */
				color = PCB->RatColor;
				text = _("rat lines");
				break;

			case MAX_LAYER + 2:
				color = PCB->PinColor;
				text = _("pins/pads");
				break;

			case MAX_LAYER + 3:
				color = PCB->ViaColor;
				text = _("vias");
				break;

			case MAX_LAYER + 4:
				color = PCB->InvisibleObjectsColor;
				text = _("far side");
				break;

			default:
				color = PCB->Data->Layer[i].Color;
				text = UNKNOWN (PCB->Data->Layer[i].Name);
				text = _(text);
				break;
			}
		button = gtk_check_button_new();
		label = gtk_label_new("");
		gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
		layer_enable_button_set_label(label, text);

		ebox = gtk_event_box_new();
		gtk_container_add(GTK_CONTAINER(ebox), label);
		gtk_container_add(GTK_CONTAINER(button), ebox);
		gtk_table_attach_defaults(GTK_TABLE(table), button,
					1, 2, i, i + 1);
/*		gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0); */
		gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(button), FALSE);

		lb->layer_enable_button = button;
		lb->layer_enable_ebox = ebox;
		lb->text = g_strdup(text);
		lb->label = label;

		layer_button_set_color(lb, color);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);

		g_signal_connect(G_OBJECT(button), "toggled",
					G_CALLBACK(layer_enable_button_cb), GINT_TO_POINTER(i));
		}
	}

  /* If new color scheme is loaded from the config or user changes a color
  |  in the preferences, make sure our layer button colors get updated.
  */
void
gui_layer_buttons_color_update(void)
	{
	GdkColor		*color;
	LayerButtonSet	*lb;
	gint			i;

	if (!Output.drawing_area)
		return;
	for (i = 0; i < MAX_LAYER + 5; ++i)
		{
		lb = &layer_buttons[i];

		if (i == GUI_SILK_LAYER)
			color = PCB->ElementColor;
		else if (i == GUI_RATS_LAYER)
			color = PCB->RatColor;
		else if (i == MAX_LAYER + 2)
			color = PCB->PinColor;
		else if (i == MAX_LAYER + 3)
			color = PCB->ViaColor;
		else if (i == MAX_LAYER + 4)
			color = PCB->InvisibleObjectsColor;
		else
			color = PCB->Data->Layer[i].Color;

		layer_button_set_color(lb, color);
		}
	}


  /* Update layer button labels and enabled state to match current PCB.
  */
void
gui_layer_enable_buttons_update(void)
	{
	LayerButtonSet	*lb;
	gchar			*s;
	gint			i;

	
	/* Update layer button labels and active state to state inside of PCB
	*/
	layer_enable_button_cb_hold_off = TRUE;
	for (i = 0; i < MAX_LAYER; ++i)
		{
		lb = &layer_buttons[i];
		s = UNKNOWN (PCB->Data->Layer[i].Name);
		if (dup_string(&lb->text, s))
			{
			layer_enable_button_set_label(lb->label, _(s));
			gui_config_layer_name_update(_(s), i);
			}
		if (Settings.verbose)
			{
			gboolean	active, new;

			active = gtk_toggle_button_get_active(
						GTK_TOGGLE_BUTTON(lb->layer_enable_button));
			new = PCB->Data->Layer[i].On;
			if (active != new)
				printf("gui_layer_enable_buttons_update: active=%d new=%d\n",
						active, new);
			}
		gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(lb->layer_enable_button),
				PCB->Data->Layer[i].On);
		}
	/* Buttons for elements (silk), rats, pins, vias, and far side don't
	|  change labels.
	*/
	lb = &layer_buttons[i++];		/* GUI_SILK_LAYER */
	gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(lb->layer_enable_button), PCB->ElementOn);

	lb = &layer_buttons[i++];		/* GUI_RATS_LAYER */
	gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(lb->layer_enable_button), PCB->RatOn);

	lb = &layer_buttons[i++];		/* pins/pads	*/
	gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(lb->layer_enable_button), PCB->PinOn);

	lb = &layer_buttons[i++];		/* vias		*/
	gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(lb->layer_enable_button), PCB->ViaOn);

	lb = &layer_buttons[i++];		/* far side	*/
	gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(lb->layer_enable_button),
				PCB->InvisibleObjectsOn);
	layer_enable_button_cb_hold_off = FALSE;
	}


  /* Main layer button synchronization with current PCB state.  Called when
  |  user toggles layer visibility or changes drawing layer or when internal
  |  PCB code changes layer visibility.
  */
void
gui_layer_buttons_update(void)
	{
	gint		layer;
	gboolean	active = FALSE;

	gui_layer_enable_buttons_update();

	/* Turning off a layer that was selected will cause PCB to switch to
	|  another layer.
	*/
	if (PCB->RatDraw)
		layer = GUI_RATS_LAYER;
	else
		layer = PCB->SilkActive ? GUI_SILK_LAYER : LayerStack[0];

	if (layer < MAX_LAYER)
		active = PCB->Data->Layer[layer].On;
	else if (layer == GUI_SILK_LAYER)
		active = PCB->ElementOn;
	else if (layer == GUI_RATS_LAYER)
		active = PCB->RatOn;

	if (Settings.verbose)
		{
		printf("gui_layer_buttons_update cur_index=%d update_index=%d\n",
				layer_select_button_index, layer);
		if (active && layer != layer_select_button_index)
			printf("\tActivating button %d\n", layer);
		}
	if (active && layer != layer_select_button_index)
		{
		layer_select_button_cb_hold_off = TRUE;
		gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(layer_buttons[layer].radio_select_button),
				TRUE);
		layer_select_button_index = layer;
		layer_select_button_cb_hold_off = FALSE;
		}
	}

  /* ------------------------------------------------------------------
  |  Route style buttons
  */
typedef struct
	{
	GtkWidget		*button;
	RouteStyleType	route_style;
	gboolean		shown;			/* For temp buttons */
	}
	RouteStyleButton;

  /* Make 3 extra route style radio buttons.  2 for the extra Temp route
  |  styles, and the 3rd is an always invisible button selected when the
  |  route style settings in use don't match any defined route style (the
  |  user can hit 'l', 'v', etc keys to change the settings without selecting
  |  a new defined style.
  */
static RouteStyleButton	route_style_button[NUM_STYLES + 3];
static gint				route_style_index;

static GtkWidget		*route_style_edit_button;


static void
route_style_edit_cb(GtkWidget *widget, OutputType *out)
	{
	RouteStyleType	*rst = NULL;

	if (route_style_index >= NUM_STYLES)
		rst = &route_style_button[route_style_index].route_style;
	gui_route_style_dialog(route_style_index, rst);
	}

static void
route_style_select_button_cb(GtkToggleButton *button, gpointer data)
	{
	RouteStyleType	*rst;
	gchar			buf[16];
	gint			index	= GPOINTER_TO_INT(data);

	if (gui->toggle_holdoff || index == NUM_STYLES + 2)
		return;

	if (route_style_index == index)
		return;
	route_style_index = index;

	if (index < NUM_STYLES)
		{
		snprintf(buf, sizeof(buf), "%d", index + 1);
		if (gtk_toggle_button_get_active(button))
			ActionRouteStyle(buf);
		}
	else if (index < NUM_STYLES + 2)
		{
		rst = &route_style_button[index].route_style;
		SetLineSize(rst->Thick);
		SetViaSize(rst->Diameter, TRUE);
		SetViaDrillingHole(rst->Hole, TRUE);
		SetKeepawayWidth(rst->Keepaway);
		}
	gtk_widget_set_sensitive(route_style_edit_button, TRUE);
	}

static void
gui_route_style_temp_buttons_hide(void)
	{
	gtk_widget_hide(route_style_button[NUM_STYLES].button);
	gtk_widget_hide(route_style_button[NUM_STYLES + 1].button);

	/* This one never becomes visibile.
	*/
	gtk_widget_hide(route_style_button[NUM_STYLES + 2].button);
	}


static void
make_route_style_buttons(GtkWidget *vbox, OutputType *out)
	{
	GtkWidget			*button;
	GSList				*group = NULL;
	RouteStyleButton	*rbut;
	gint				i;

	button = gtk_button_new_with_label(_("Route Style"));
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 3);
	g_signal_connect(button, "clicked",
				G_CALLBACK(route_style_edit_cb), out);
	route_style_edit_button = button;

	for (i = 0; i < NUM_STYLES + 3; ++i)
		{
		RouteStyleType	*rst;
		gchar			buf[32];

		rbut = &route_style_button[i];
		if (i < NUM_STYLES)
			{
			rst = &PCB->RouteStyle[i];
			button = gtk_radio_button_new_with_label(group, _(rst->Name));
			}
		else
			{
			snprintf(buf, sizeof(buf), _("Temp%d"), i - NUM_STYLES + 1);
			button = gtk_radio_button_new_with_label(group, buf);
			if (!route_style_button[i].route_style.Name)
				route_style_button[i].route_style.Name = g_strdup(buf);
			}
		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
		gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
		rbut->button = button;
		if (i < NUM_STYLES + 2)
			g_signal_connect(G_OBJECT(button), "toggled",
						G_CALLBACK(route_style_select_button_cb),
						GINT_TO_POINTER(i));
		}
	}

void
gui_route_style_button_set_active(gint n)
	{
	if (n < 0 || n >= NUM_STYLES + 3)
		return;

	gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(route_style_button[n].button), TRUE);
	}

  /* Upate the route style button selected to match current route settings.
  |  If user has changed an in use route setting so they don't match any
  |  defined route style, select the invisible dummy route style button.
  */
void
gui_route_style_buttons_update(void)
	{
	RouteStyleType		*rst;
	gint				i;

	for (i = 0; i < NUM_STYLES + 2; ++i)
		{
		if (i < NUM_STYLES)
			rst = &PCB->RouteStyle[i];
		else
			{
			if (!route_style_button[i].shown)	/* Temp button shown? */
				continue;
			rst = &route_style_button[i].route_style;
			}
		if (   Settings.LineThickness == rst->Thick
		    && Settings.ViaThickness == rst->Diameter
		    && Settings.ViaDrillingHole == rst->Hole
		    && Settings.Keepaway == rst->Keepaway
		   )
			break;
		}
	/* If i == NUM_STYLES + 2 at this point, we activate the invisible button.
	*/
	gui->toggle_holdoff = TRUE;
	gui_route_style_button_set_active(i);
	route_style_index = i;
	gui->toggle_holdoff = FALSE;

	gtk_widget_set_sensitive(route_style_edit_button,
				(i == NUM_STYLES + 2) ? FALSE : TRUE);
	}

void
gui_route_style_set_button_label(gchar *name, gint index)
	{
	if (index < 0 || index >= NUM_STYLES || !route_style_button[index].button)
		return;
	gtk_button_set_label(GTK_BUTTON(route_style_button[index].button),
				_(name));
	}

void
gui_route_style_set_temp_style(RouteStyleType *rst, gint which)
	{
	RouteStyleButton	*rsb;
	gchar				*tmp;
	gint				index = which + NUM_STYLES;

	if (which < 0 || which > 1)
		return;
	rsb = &route_style_button[index];
	gtk_widget_show(rsb->button);
	rsb->shown = TRUE;
	tmp = rsb->route_style.Name;
	rsb->route_style = *rst;
	rsb->route_style.Name = tmp;
	if (route_style_index != index)
		{
		route_style_index = index;	/* Sets already done */
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rsb->button), TRUE);
		}
	}


/* ---------------------------------------------------------------
|  Zoom spin button
*/
static GtkWidget	*zoom_spin_button;
static gboolean		zoom_holdoff;

static void
zoom_spin_button_cb(GtkWidget *spin_button, gpointer data)
	{
	gint	z;

	if (zoom_holdoff)
		return;
	z = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_button));
	SetZoom(z);
	}

void
gui_zoom_display_update(void)
	{
	zoom_holdoff = TRUE;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(zoom_spin_button), PCB->Zoom);
	zoom_holdoff = FALSE;
	}


/* ---------------------------------------------------------------
|  Mode buttons
*/
typedef struct
	{
	GtkWidget	*button;
	gchar		*name;
	gint		mode;
	gchar		**xpm;
	}
ModeButton;


static ModeButton	mode_buttons[] =
	{
	{NULL, 	"via",			VIA_MODE,			via},
	{NULL,	"line",			LINE_MODE,			line},
	{NULL,	"arc",			ARC_MODE,			arc},
	{NULL,	"text",			TEXT_MODE,			text},
	{NULL,	"rectangle",	RECTANGLE_MODE,		rect},
	{NULL,	"polygon",		POLYGON_MODE,		poly},
	{NULL,	"buffer",		PASTEBUFFER_MODE,	buf},
	{NULL,	"remove",		REMOVE_MODE,		del},
	{NULL,	"rotate",		ROTATE_MODE,		rot},
	{NULL,	"insertPoint",	INSERTPOINT_MODE,	ins},
	{NULL,	"thermal",		THERMAL_MODE,		thrm},
	{NULL,	"select",		ARROW_MODE,			sel},
	{NULL,	"pan",			PAN_MODE,			pan},
	{NULL,	"lock",			LOCK_MODE,			lock}
	};

static gint	n_mode_buttons = G_N_ELEMENTS(mode_buttons);


static void
mode_button_toggled_cb(GtkWidget *widget, ModeButton *mb)
	{
	gboolean  active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

	if (active)
		SetMode(mb->mode);
	}

void
gui_mode_buttons_update(void)
	{
	ModeButton	*mb;
	gint		i;

	for (i = 0; i < n_mode_buttons; ++i)
		{
		mb = &mode_buttons[i];
		if (Settings.Mode == mb->mode)
			{
			gtk_toggle_button_set_active(
					GTK_TOGGLE_BUTTON(mb->button), TRUE);
			break;
			}
		}
	}


static void
make_mode_buttons(GtkWidget *vbox, OutputType *out)
	{
	ModeButton		*mb;
	GtkWidget		*hbox = NULL, *button;
	GtkWidget		*image;
	GdkPixbuf		*pixbuf;
	GSList			*group = NULL;
	gint			i;

	for (i = 0; i < n_mode_buttons; ++i)
		{
		if ((i % Settings.n_mode_button_columns) == 0)
			{
			hbox = gtk_hbox_new(FALSE, 0);
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
			}
		mb = &mode_buttons[i];
		button = gtk_radio_button_new(group);
		mb->button = button;
		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
		gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(button), FALSE);
		gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

		pixbuf = gdk_pixbuf_new_from_xpm_data((const char **) mb->xpm);
		image = gtk_image_new_from_pixbuf(pixbuf);
		g_object_unref(G_OBJECT(pixbuf));

		gtk_container_add(GTK_CONTAINER(button), image);
		if (!strcmp(mb->name, "pan"))
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
		g_signal_connect (button, "toggled",
				G_CALLBACK(mode_button_toggled_cb), mb);
		}
	}



/* ---------------------------------------------------------------
|  Top window
*/

static GtkWidget	*gui_left_sensitive_box;

static gint
delete_chart_cb(GtkWidget *widget, GdkEvent *event, OutputType *out)
	{
	/* Return FALSE to emit the "destroy" signal */
	return FALSE;
    }

static void
destroy_chart_cb(GtkWidget *widget, OutputType *out)
	{
	gtk_main_quit();
	}


  /* Create the top_window contents.  The config settings should be loaded
  |  before this is called.
  */
static void
gui_build_pcb_top_window(void)
	{
	GtkWidget	*window;
	GtkWidget	*vbox_main, *vbox_left, *hbox_middle, *hbox = NULL;
	GtkWidget	*viewport, *ebox, *vbox;
	GtkWidget	*label;
	GtkWidget	*separator;
	OutputType	*out	= &Output;

	window = out->top_window;
	out->creating = TRUE;		/* signal holdoff */

	gtk_window_set_default_size(GTK_WINDOW(window),
				Settings.pcb_width, Settings.pcb_height);

	vbox_main = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox_main);

  /* -- Top control bar */
	hbox = gtk_hbox_new(FALSE, 8);
	gtk_box_pack_start(GTK_BOX(vbox_main), hbox, FALSE, FALSE, 0);
	gui->top_hbox = hbox;

	/* menu_hbox will be made insensitive when the gui needs
	|  a modal button GetLocation button press.
	*/
	gui->menu_hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(gui->top_hbox), gui->menu_hbox,
				FALSE, FALSE, 0);

	make_top_menubar(gui->menu_hbox, out);

	gui->compact_vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(gui->top_hbox), gui->compact_vbox,
				FALSE, FALSE, 0);

	gui->compact_hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(gui->top_hbox), gui->compact_hbox,
				FALSE, FALSE, 0);

	/* The zoom spin and board name are in compact_vbox and the position
	|  labels will be packed below them or to the side of them.
	*/
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(gui->compact_vbox), hbox, TRUE, FALSE, 3);
	gui_spin_button(hbox, &zoom_spin_button, PCB->Zoom,
				-10.0, 12.0, 1.0, 4.0, 0, 0,
				zoom_spin_button_cb, NULL, FALSE, _("Zoom"));
	label = gtk_label_new("");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<b><big>Unnamed</big></b>");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 4);
	gui->name_label = label;

	/* The position_box pack location depends on user setting of
	|  compact horizontal mode.
	*/
	gui->position_hbox = gtk_hbox_new(FALSE, 0);
	g_object_ref(G_OBJECT(gui->position_hbox));		/* so can remove it */
	if (Settings.gui_compact_horizontal)
		gtk_box_pack_end(GTK_BOX(gui->compact_vbox), gui->position_hbox,
					FALSE, FALSE, 0);
	else
		gtk_box_pack_end(GTK_BOX(gui->top_hbox), gui->position_hbox,
					FALSE, FALSE, 0);

	make_cursor_position_labels(gui->position_hbox, out);

	hbox_middle = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_main), hbox_middle, TRUE, TRUE, 3);


  /* -- Left control bar */
	ebox = gtk_event_box_new();
	gtk_widget_set_events (ebox, GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK);
	gtk_box_pack_start(GTK_BOX(hbox_middle), ebox, FALSE, FALSE, 3);
	g_signal_connect (ebox, "motion_notify_event",
				G_CALLBACK(gui_output_stop_scroll_cb), out);

	/* This box will also be made insensitive when the gui needs
	|  a modal button GetLocation button press.
	*/
	gui_left_sensitive_box = ebox;

	vbox_left = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(ebox), vbox_left);

	make_layer_buttons(vbox_left, out);

	separator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox_left), separator, FALSE, FALSE, 8);

	make_mode_buttons(vbox_left, out);

	separator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox_left), separator, FALSE, FALSE, 8);

	make_route_style_buttons(vbox_left, out);



	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_middle), vbox, TRUE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

  /* -- The PCB layout output drawing area */
	viewport = gtk_viewport_new(NULL, NULL);
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(viewport), GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(hbox), viewport, TRUE, TRUE, 0);

	out->drawing_area = gtk_drawing_area_new();

	gtk_widget_add_events(out->drawing_area, GDK_EXPOSURE_MASK
				| GDK_LEAVE_NOTIFY_MASK | GDK_ENTER_NOTIFY_MASK
				| GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK
				| GDK_KEY_RELEASE_MASK | GDK_KEY_PRESS_MASK
				| GDK_FOCUS_CHANGE_MASK | GDK_POINTER_MOTION_MASK);

	/* This is required to get the drawing_area key-press-event.  Also the
	|  enter and button press callbacks grab focus to be sure we have it
	|  when in the drawing_area.
	*/
	GTK_WIDGET_SET_FLAGS(out->drawing_area, GTK_CAN_FOCUS);

	gtk_container_add(GTK_CONTAINER(viewport), out->drawing_area);

	gui->v_adjustment = gtk_adjustment_new(0.0, 0.0, 100.0, 10.0, 10.0, 10.0);
	gui->v_scrollbar = gtk_vscrollbar_new(GTK_ADJUSTMENT(gui->v_adjustment));
	gtk_range_set_update_policy(GTK_RANGE(gui->v_scrollbar),
				GTK_UPDATE_CONTINUOUS);
	gtk_box_pack_start(GTK_BOX(hbox), gui->v_scrollbar, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(gui->v_adjustment), "value_changed",
				G_CALLBACK(v_adjustment_changed_cb), gui);

	gui->h_adjustment = gtk_adjustment_new(0.0, 0.0, 100.0, 10.0, 10.0, 10.0);
	gui->h_scrollbar = gtk_hscrollbar_new(GTK_ADJUSTMENT(gui->h_adjustment));
	gtk_range_set_update_policy(GTK_RANGE(gui->h_scrollbar),
				GTK_UPDATE_CONTINUOUS);
	gtk_box_pack_start(GTK_BOX(vbox), gui->h_scrollbar, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(gui->h_adjustment), "value_changed",
				G_CALLBACK(h_adjustment_changed_cb), gui);


  /* -- The bottom status line label */
	gui->status_line_hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gui->status_line_hbox, FALSE, FALSE, 2);

	label = gtk_label_new("");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(gui->status_line_hbox), label, FALSE, FALSE, 0);
	gui->status_line_label = label;

	/* Depending on user setting, the command_combo_box may get packed into
	|  the status_line_hbox, but it will happen on demand the first time
	|  the user does a command entry.
	*/

	g_signal_connect(G_OBJECT(out->drawing_area), "expose_event",
				G_CALLBACK(gui_output_drawing_area_expose_event_cb), out);
	g_signal_connect(G_OBJECT(out->top_window), "configure_event",
				G_CALLBACK(top_window_configure_event_cb), out);
	g_signal_connect(G_OBJECT(out->drawing_area), "configure_event",
				G_CALLBACK(gui_output_drawing_area_configure_event_cb), out);

	/* Mouse and key events will need to be intercepted when PCB needs a
	|  location from the user.
	*/
	gui_interface_input_signals_connect();

	g_signal_connect(G_OBJECT(out->drawing_area), "scroll_event",
				G_CALLBACK(gui_output_window_mouse_scroll_cb), out);
	g_signal_connect(G_OBJECT(out->drawing_area), "enter_notify_event",
				G_CALLBACK(gui_output_window_enter_cb), out);
	g_signal_connect(G_OBJECT(out->drawing_area), "leave_notify_event",
				G_CALLBACK(gui_output_window_leave_cb), out);
	g_signal_connect(G_OBJECT(out->drawing_area), "motion_notify_event",
				G_CALLBACK(gui_output_window_motion_cb), out);


	g_signal_connect(G_OBJECT(window), "delete_event",
			G_CALLBACK(delete_chart_cb), out);
	g_signal_connect(G_OBJECT(window), "destroy",
			G_CALLBACK(destroy_chart_cb), out);

	out->creating = FALSE;

	gtk_widget_show_all(out->top_window);
	gtk_widget_realize(vbox_main);
	gtk_widget_realize(hbox_middle);
	gtk_widget_realize(viewport);
	gtk_widget_realize(out->drawing_area);
	gdk_window_set_back_pixmap(out->drawing_area->window, NULL, FALSE);

	gui_route_style_temp_buttons_hide();
	}

  /* Connect and disconnect just the signals a g_main_loop() will need.
  |  Cursor and motion events still need to be handled by the top level
  |  loop, so don't connect/reconnect these.
  |  A g_main_loop will be running when PCB wants the user to select a
  |  location or if command entry is needed in the status line hbox.
  |  During these times normal button/key presses are intercepted, either
  |  by new signal handlers or the command_combo_box entry.
  */
static gulong	button_press_handler, button_release_handler,
				key_press_handler, key_release_handler;

void
gui_interface_input_signals_connect(void)
	{
	OutputType	*out	= &Output;
	
	button_press_handler = 
		g_signal_connect(G_OBJECT(out->drawing_area), "button_press_event",
					G_CALLBACK(gui_output_button_press_cb), gui->ui_manager);

	button_release_handler =
		g_signal_connect(G_OBJECT(out->drawing_area), "button_release_event",
					G_CALLBACK(gui_output_button_release_cb), gui->ui_manager);

	key_press_handler =
		g_signal_connect(G_OBJECT(out->drawing_area), "key_press_event",
					G_CALLBACK(gui_output_key_press_cb), gui->ui_manager);

	key_release_handler =
		g_signal_connect(G_OBJECT(out->drawing_area), "key_release_event",
					G_CALLBACK(gui_output_key_release_cb), gui->ui_manager);
	}

void
gui_interface_input_signals_disconnect(void)
	{
	OutputType	*out	= &Output;
	
	if (button_press_handler)
		g_signal_handler_disconnect(out->drawing_area, button_press_handler);

	if (button_release_handler)
		g_signal_handler_disconnect(out->drawing_area, button_release_handler);

	if (key_press_handler)
		g_signal_handler_disconnect(out->drawing_area, key_press_handler);

	if (key_release_handler)
		g_signal_handler_disconnect(out->drawing_area, key_release_handler);

	button_press_handler = button_release_handler = 0;
	key_press_handler = key_release_handler = 0;
	}

  /* We'll set the interface insensitive when a g_main_loop is running so the
  |  Gtk menus and buttons don't respond and interfere with the special entry
  |  the user needs to be doing.
  */
void
gui_interface_set_sensitive(gboolean sensitive)
	{
	gtk_widget_set_sensitive(gui_left_sensitive_box, sensitive);
	gtk_widget_set_sensitive(gui->menu_hbox, sensitive);
	}

/* ---------------------------------------------------------------------------
 *  Stipple patterns with 50 % pixels
 */
#define	GRAY_SIZE	8

typedef unsigned char pattern[8];

static pattern gray_bits[] = {
  {0xa5, 0x5a, 0xa5, 0x5a, 0xa5, 0x5a, 0xa5, 0x5a},
  {0x33, 0xcc, 0x33, 0xcc, 0x33, 0xcc, 0x33, 0xcc},
  {0xcc, 0x33, 0xcc, 0x33, 0xcc, 0x33, 0xcc, 0x33},
  {0x88, 0x77, 0x88, 0x77, 0x88, 0x77, 0x88, 0x77},
  {0x77, 0x88, 0x77, 0x88, 0x77, 0x88, 0x77, 0x88},
  {0xee, 0x11, 0xee, 0x11, 0xee, 0x11, 0xee, 0x11},
  {0x11, 0xee, 0x11, 0xee, 0x11, 0xee, 0x11, 0xee},
  {0x96, 0x69, 0x96, 0x69, 0x96, 0x69, 0x96, 0x69},
  {0x69, 0x96, 0x69, 0x96, 0x69, 0x96, 0x69, 0x96}
};

static void
gui_init_gc(OutputType *out)
	{
	GdkBitmap	*dummy_bitmap;
	GdkColor	bit_color;
	gint		i;

	out->bgGC = gdk_gc_new(out->top_window->window);
	gdk_gc_copy(out->bgGC, out->top_window->style->white_gc );

	out->fgGC = gdk_gc_new(out->top_window->window);
	gdk_gc_copy(out->fgGC, out->top_window->style->white_gc );

	out->pmGC = gdk_gc_new(out->top_window->window);
	gdk_gc_copy(out->pmGC, out->top_window->style->white_gc );

	out->GridGC = gdk_gc_new(out->top_window->window);
	gdk_gc_copy(out->GridGC, out->top_window->style->white_gc );

	gdk_gc_set_foreground(out->bgGC, &Settings.BackgroundColor);
	gdk_gc_set_fill(out->fgGC, GDK_SOLID);

	/* Set up the depth 1 pmGC.
	*/
	dummy_bitmap = gdk_pixmap_new(out->top_window->window, 16, 16, 1);
	out->pmGC = gdk_gc_new(dummy_bitmap);
	bit_color.pixel = 1;
	gdk_gc_set_foreground(out->pmGC, &bit_color);
	g_object_unref(G_OBJECT(dummy_bitmap));

	Stipples = g_new0(GdkPixmap *, 9);
	for (i = 0; i < 9; ++i)
		Stipples[i] = gdk_bitmap_create_from_data(out->top_window->window,
						(const gchar *) gray_bits[i], GRAY_SIZE, GRAY_SIZE);

	gdk_gc_set_foreground(out->GridGC, &Settings.GridColor);
	gdk_gc_set_background(out->GridGC, &Settings.BackgroundColor);
	gdk_gc_set_function(out->GridGC, GDK_INVERT);
	gdk_gc_set_line_attributes(out->GridGC, 2,
			GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
	}

/* ----------------------------------------------------------------------
 * initializes icon pixmap and also cursor bit maps
 */
static void
gui_init_icons(OutputType *out)
	{
	GdkPixbuf	*icon;

	icon = gdk_pixbuf_new_from_xpm_data((const gchar **) icon_bits);
	gtk_window_set_default_icon(icon);

	XC_clock_source = gdk_bitmap_create_from_data(out->top_window->window,
						rotateIcon_bits,
						rotateIcon_width, rotateIcon_height);
	XC_clock_mask = gdk_bitmap_create_from_data(out->top_window->window,
						rotateMask_bits,
						rotateMask_width, rotateMask_height);

	XC_hand_source = gdk_bitmap_create_from_data(out->top_window->window,
						handIcon_bits,
						handIcon_width, handIcon_height);
	XC_hand_mask =  gdk_bitmap_create_from_data(out->top_window->window,
						handMask_bits,
						handMask_width, handMask_height);

	XC_lock_source =  gdk_bitmap_create_from_data(out->top_window->window,
						lockIcon_bits,
						lockIcon_width, lockIcon_height);
	XC_lock_mask =  gdk_bitmap_create_from_data(out->top_window->window,
						lockMask_bits,
						lockMask_width, lockMask_height);
	}

void
gui_create_pcb_widgets(void)
	{
	OutputType	*out	= &Output;

	gui_build_pcb_top_window();
	gui_init_toggle_states();
	gui_init_gc(out);
	gui_init_icons(out);
	}

  /* Create top level window for routines that will need top_window
  |  before gui_create_pcb_widgets() is called.
  */
void
gui_init(gint *argc, gchar ***argv)
	{
	GtkWidget	*window;
	OutputType	*out	= &Output;

	/* Threads aren't used in PCB, but this call would go here.
	*/
/*	g_thread_init(NULL); */

#if defined (ENABLE_NLS)
	/* Do our own setlocale() stufff since we want to override LC_NUMERIC
	*/
	gtk_set_locale();
	setlocale(LC_NUMERIC, "POSIX");  /* use decimal point instead of comma */
#endif

	/* Prevent gtk_init() and gtk_init_check() from automatically
	| calling setlocale (LC_ALL, "") which would undo LC_NUMERIC if ENABLE_NLS
	| We also don't want locale set if no ENABLE_NLS to keep POSIX LC_NUMERIC.
	*/
	gtk_disable_setlocale();

	gtk_init(argc, argv);
	gtk_widget_push_colormap(gdk_rgb_get_colormap());  /* need this ?? */

#ifdef ENABLE_NLS
#ifdef LOCALEDIR
	bindtextdomain(PACKAGE, LOCALEDIR);
#endif
	textdomain(PACKAGE);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
#endif  /* ENABLE_NLS */

	gui = g_new0(GuiPCB, 1);

	window = out->top_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "PCB");

	gtk_widget_realize(out->top_window);
	}
