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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifdef DAN_FIXME
TODO:

- figure out when we need to call this:
  ghid_set_status_line_label ();
  Everytime? 

- the old quit callback had:

  ghid_config_files_write ();
  hid_action ("Quit");

  
#endif

/* This file written by Bill Wilson for the PCB Gtk port */


/* gui-top-window.c
|  This handles creation of the top level window and all its widgets.
|  events for the Output.drawing_area widget are handled in a separate
|  file gui-output-events.c
|
|  Some caveats with menu shorcut keys:  Some keys are trapped out by Gtk
|  and can't be used as shortcuts (eg. '|', TAB, etc).  For these cases
|  the Gtk menus can't show the same shortcut as the Xt menus did, but the
|  actions are the same as the keys are handled in ghid_port_key_press_cb().
*/


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>

#include "gtkhid.h"
#include "gui.h"
#include "hid.h"
#include "../hidint.h"

#include "action.h"
#include "autoplace.h"
#include "autoroute.h"
#include "buffer.h"
#include "change.h"
#include "command.h"
#include "copy.h"
#include "create.h"
#include "crosshair.h"
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
#include "gpcb-menu.h"
#include "polygon.h"
#include "rats.h"
#include "remove.h"
#include "report.h"
#include "resource.h"
#include "rotate.h"
#include "rubberband.h"
#include "search.h"
#include "select.h"
#include "set.h"
#include "undo.h"
#include "vendor.h"

#include "gui-icons-mode-buttons.data"
#include "gui-icons-misc.data"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

static void ghid_load_menus (void);
static void ghid_ui_info_append (const gchar *);
static void ghid_ui_info_indent (int);

static gchar * new_ui_info;
static size_t new_ui_info_sz = 0;

/* the array of actions for "normal" menuitems */
static GtkActionEntry *new_entries = NULL;
static gint menuitem_cnt = 0;

/* the array of actions for "toggle" menuitems */
static GtkToggleActionEntry *new_toggle_entries = NULL;
static gint tmenuitem_cnt = 0;

static Resource **action_resources = NULL;
static Resource **toggle_action_resources = NULL;

#define MENUITEM "MenuItem"
#define TMENUITEM "TMenuItem"

extern HID ghid_hid;

GhidGui _ghidgui, *ghidgui = NULL;


GHidPort ghid_port, *gport;

static GdkColor WhitePixel, BlackPixel;

static gchar		*bg_image_file;

#define	N_GRID_SETTINGS		11

#define	MM_TO_PCB(mm)	((mm) * 100000 / 25.4)

static gdouble grid_mil_values[N_GRID_SETTINGS] = {
  10.0,
  20.0,
  50.0,
  100.0,
  200.0,
  500.0,
  1000.0,
  2000.0,
  2500.0,
  5000.0,
  10000.0
};

static gdouble grid_mm_values[N_GRID_SETTINGS] = {
  MM_TO_PCB (0.002),
  MM_TO_PCB (0.005),
  MM_TO_PCB (0.01),
  MM_TO_PCB (0.02),
  MM_TO_PCB (0.05),
  MM_TO_PCB (0.1),
  MM_TO_PCB (0.2),
  MM_TO_PCB (0.25),
  MM_TO_PCB (0.5),
  MM_TO_PCB (1.0),
  MM_TO_PCB (2.0)
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
get_grid_value_index (gboolean allow_fail)
{
  gdouble *value;
  gint i;

  value = Settings.grid_units_mm ? &grid_mm_values[0] : &grid_mil_values[0];
  for (i = 0; i < N_GRID_SETTINGS; ++i, ++value)
    if (PCB->Grid < *value + 1.0 && PCB->Grid > *value - 1.0)
      break;
  if (i >= N_GRID_SETTINGS)
    i = allow_fail ? -1 : N_GRID_SETTINGS - 1;

  return i;
}

static void
h_adjustment_changed_cb (GtkAdjustment * adj, GhidGui * g)
{
  gdouble xval, yval;

  if (g->adjustment_changed_holdoff)
    return;
  xval = gtk_adjustment_get_value (adj);
  yval = gtk_adjustment_get_value (GTK_ADJUSTMENT (ghidgui->v_adjustment));
  ghid_port_ranges_changed ();
}

static void
v_adjustment_changed_cb (GtkAdjustment * adj, GhidGui * g)
{
  gdouble xval, yval;

  if (g->adjustment_changed_holdoff)
    return;
  yval = gtk_adjustment_get_value (adj);
  xval = gtk_adjustment_get_value (GTK_ADJUSTMENT (ghidgui->h_adjustment));
  ghid_port_ranges_changed ();
}

  /* Save size of top window changes so PCB can restart at its size at exit.
   */
static gint
top_window_configure_event_cb (GtkWidget * widget, GdkEventConfigure * ev,
			       GHidPort * port)
{
  gboolean new_w, new_h;

  new_w = (ghidgui->top_window_width != widget->allocation.width);
  new_h = (ghidgui->top_window_height != widget->allocation.height);

  ghidgui->top_window_width = widget->allocation.width;
  ghidgui->top_window_height = widget->allocation.height;

  if (new_w || new_h)
    ghidgui->config_modified = TRUE;

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


/*
 * This is the main menu callback function.  The callback looks as
 * the gtk action name to figure out which menuitem was chosen.  Then
 * it looks up in a table to find the pcb actions which should be
 * executed
 */
#define DEBUG_MENU_CB

static void
ghid_menu_cb (GtkAction * action, GHidPort * port)
{
  const gchar * name;
  int id = 0;
  int vi;
  Resource *node = NULL;

  name = gtk_action_get_name (action);

  if ( strncmp (name, MENUITEM, strlen (MENUITEM)) == 0)
    {
      /* This is a "normal" menuitem as opposed to a toggle menuitem
       */
      id = atoi (name + strlen (MENUITEM));
      node = action_resources[id];
    }
  else if ( strncmp (name, TMENUITEM, strlen (TMENUITEM)) == 0)
    {
      /* This is a "toggle" menuitem */
      id = atoi (name + strlen (TMENUITEM));
      node = toggle_action_resources[id];
    }
  else
    {
      fprintf (stderr, "ERROR:  ghid_menu_cb():  name = \"%s\" is unknown\n", name);
    }
    

#ifdef DEBUG_MENU_CB
  printf ("ghid_menu_cb():  name = \"%s\", id = %d\n", name, id);
#endif

  if (node != NULL)
    {
      for (vi = 1; vi < node->c; vi++)
	if (resource_type (node->v[vi]) == 10)
	  {
#ifdef DEBUG_MENU_CB
	    printf ("    %s\n", node->v[vi].value);
#endif
	    hid_parse_actions (node->v[vi].value, NULL);
	  }
    }
  else {
#ifdef DEBUG_MENU_CB
    printf ("    NOOP\n");
#endif
  }

  ghid_update_menus ();
}



/* ============== ViewMenu callbacks =============== */
static void
toggle_draw_grid_cb (GtkToggleAction * action, GHidPort * port)
{
  if (ghidgui->toggle_holdoff)	/* If setting initial condition */
    return;

  /* Set to ! because ActionDisplay toggles it */
  Settings.DrawGrid = !gtk_toggle_action_get_active (action);
  ghidgui->config_modified = TRUE;
  hid_actionl ("Display", "Grid", "", NULL);
  ghid_set_status_line_label ();
}

static void
realign_grid_cb (GtkAction * action, GHidPort * port)
{
  int x, y;
  gui->get_coords ("Press a button at a grid point", &x, &y);
  hid_actionl ("Display", "ToggleGrid", "", NULL);
  ghid_set_status_line_label ();
}

static void
zoom_cb (GtkAction * action, GtkRadioAction * current)
{
  gint px, py;
  const gchar *saction = gtk_action_get_name (action);
  gdouble zoom_factor;

  zoom_factor = !strcmp (saction, "ZoomIn") ? 0.8 : 1.25;
  ghid_get_coords ("Click on zoom focus", &px, &py);
  ghid_port_ranges_zoom (gport->zoom * zoom_factor);
  ghid_set_status_line_label ();
}


static void
toggle_view_solder_side_cb (GtkAction * action, GHidPort * port)
{
  /* If get here from the menu, ask for a locaton.
   */
  if (ghidgui->toggle_holdoff)
    return;
  if (!ghid_shift_is_pressed ())
    {
      int x, y;
      gui->get_coords (_("Press a button at the desired point"), &x, &y);
    }
  hid_action ("SwapSides");
  ghid_set_status_line_label ();
}

static void
toggle_pinout_shows_number_cb (GtkToggleAction * action, GHidPort * port)
{
  /* Toggle existing PCB flag and use setting to initialize new PCB flag */
  Settings.ShowNumber = gtk_toggle_action_get_active (action);
  ghidgui->config_modified = TRUE;
  if (!ghidgui->toggle_holdoff)
    hid_actionl ("Display", "ToggleName", "", NULL);
}

static void
pinout_menu_cb (GtkAction * action, GHidPort * port)
{
  if (!ghid_shift_is_pressed ())
    {
      int x, y;
      gui->get_coords ("Select an element", &x, &y);
    }
  hid_actionl ("Display", "Pinout", "", NULL);
}

  /* Do grid units handling common to a grid units change from the menu or
     |  the grid units button.
   */
static void
handle_grid_units_change (gboolean active)
{
  gchar *grid;
  gint i;

  i = get_grid_value_index (FALSE);
  Settings.grid_units_mm = active;
  PCB->Grid = Settings.grid_units_mm ? grid_mm_values[i] : grid_mil_values[i];

  ghid_grid_setting_update_menu_actions ();

  grid = g_strdup_printf ("%f", PCB->Grid);
  hid_actionl ("SetValue", "Grid", grid, "", NULL);
  g_free (grid);

  ghid_config_handle_units_changed ();

  ghid_change_selected_update_menu_actions ();
  ghid_set_status_line_label ();
}

static void
toggle_grid_units_cb (GtkToggleAction * action, GHidPort * port)
{
  if (ghidgui->toggle_holdoff)	/* If setting initial condition */
    return;
  handle_grid_units_change (gtk_toggle_action_get_active (action));
}

static void
radio_grid_mil_setting_cb (GtkAction * action, GtkRadioAction * current)
{
  gdouble value;
  gchar *grid;
  gint index;

  if (ghidgui->toggle_holdoff)
    return;
  index = gtk_radio_action_get_current_value (current);
  value = grid_mil_values[index];
  grid = g_strdup_printf ("%f", value);
  hid_actionl ("SetValue", "Grid", grid, "", NULL);
  g_free (grid);
  ghid_set_status_line_label ();
}

static void
radio_grid_mm_setting_cb (GtkAction * action, GtkRadioAction * current)
{
  gdouble value;
  gchar *grid;
  gint index;

  if (ghidgui->toggle_holdoff)
    return;
  index = gtk_radio_action_get_current_value (current);
  value = grid_mm_values[index];
  grid = g_strdup_printf ("%f", value);
  hid_actionl ("SetValue", "Grid", grid, "", NULL);
  g_free (grid);
  ghid_set_status_line_label ();
}

static void
radio_displayed_element_name_cb (GtkAction * action, GtkRadioAction * current)
{
  gint value;
  static gchar *doit[] = { "Description", "NameOnPCB", "Value" };

  value = gtk_radio_action_get_current_value (current);
  if (value >= 0 && value < 4)
    hid_actionl ("Display", doit[value], "", NULL);
}



/* ============== SettingsMenu callbacks =============== */
static void
toggle_45_degree_cb (GtkToggleAction * action, GHidPort * port)
{
  /* Toggle existing PCB flag and use setting to initialize new PCB flag */
  Settings.AllDirectionLines = gtk_toggle_action_get_active (action);
  ghidgui->config_modified = TRUE;
  if (!ghidgui->toggle_holdoff)
    hid_actionl ("Display", "Toggle45Degree", "", NULL);
  ghid_set_status_line_label ();
}

static void
toggle_start_direction_cb (GtkToggleAction * action, GHidPort * port)
{
  /* Toggle existing PCB flag and use setting to initialize new PCB flag */
  Settings.SwapStartDirection = gtk_toggle_action_get_active (action);
  ghidgui->config_modified = TRUE;
  if (!ghidgui->toggle_holdoff)
    hid_actionl ("Display", "ToggleStartDirection", "", NULL);
  ghid_set_status_line_label ();
}

static void
toggle_orthogonal_moves_cb (GtkToggleAction * action, GHidPort * port)
{
  /* Toggle existing PCB flag and use setting to initialize new PCB flag */
  Settings.OrthogonalMoves = gtk_toggle_action_get_active (action);
  ghidgui->config_modified = TRUE;
  if (!ghidgui->toggle_holdoff)
    hid_actionl ("Display", "ToggleOrthoMove", "", NULL);
  ghid_set_status_line_label ();
}

static void
toggle_snap_pin_cb (GtkToggleAction * action, GHidPort * port)
{
  /* Toggle existing PCB flag and use setting to initialize new PCB flag */
  Settings.SnapPin = gtk_toggle_action_get_active (action);
  ghidgui->config_modified = TRUE;
  if (!ghidgui->toggle_holdoff)
    hid_actionl ("Display", "ToggleSnapPin", "", NULL);
}

static void
toggle_show_DRC_cb (GtkToggleAction * action, GHidPort * port)
{
  /* Toggle existing PCB flag and use setting to initialize new PCB flag */
  Settings.ShowDRC = gtk_toggle_action_get_active (action);
  ghidgui->config_modified = TRUE;
  if (!ghidgui->toggle_holdoff)
    hid_actionl ("Display", "ToggleShowDRC", "", NULL);
}

static void
toggle_auto_DRC_cb (GtkToggleAction * action, GHidPort * port)
{
  /* Toggle existing PCB flag and use setting to initialize new PCB flag */
  Settings.AutoDRC = gtk_toggle_action_get_active (action);
  ghidgui->config_modified = TRUE;
  if (!ghidgui->toggle_holdoff)
    hid_actionl ("Display", "ToggleAutoDRC", "", NULL);
}

static void
toggle_rubber_band_cb (GtkToggleAction * action, GHidPort * port)
{
  /* Toggle existing PCB flag and use setting to initialize new PCB flag */
  Settings.RubberBandMode = gtk_toggle_action_get_active (action);
  ghidgui->config_modified = TRUE;
  if (!ghidgui->toggle_holdoff)
    hid_actionl ("Display", "ToggleRubberBandMode", "", NULL);
}

static void
toggle_unique_names_cb (GtkToggleAction * action, GHidPort * port)
{
  /* Toggle existing PCB flag and use setting to initialize new PCB flag */
  Settings.UniqueNames = gtk_toggle_action_get_active (action);
  ghidgui->config_modified = TRUE;
  if (!ghidgui->toggle_holdoff)
    hid_actionl ("Display", "ToggleUniqueNames", "", NULL);
}

static void
toggle_local_ref_cb (GtkAction * action, GHidPort * port)
{
  /* Transient setting, not saved in Settings & not used for new PCB flag. */
  hid_actionl ("Display", "ToggleLocalRef", "", NULL);
}

static void
toggle_clear_line_cb (GtkToggleAction * action, GHidPort * port)
{
  /* Toggle existing PCB flag and use setting to initialize new PCB flag */
  Settings.ClearLine = gtk_toggle_action_get_active (action);
  ghidgui->config_modified = TRUE;
  if (!ghidgui->toggle_holdoff)
    hid_actionl ("Display", "ToggleClearLine", "", NULL);
}

static void
toggle_live_route_cb (GtkToggleAction * action, GHidPort * port)
{
  /* Toggle existing PCB flag and use setting to initialize new PCB flag */
  Settings.liveRouting = gtk_toggle_action_get_active (action);
  ghidgui->config_modified = TRUE;
  if (!ghidgui->toggle_holdoff)
    hid_actionl ("Display", "ToggleLiveRoute", "", NULL);
}

static void
toggle_thin_draw_cb (GtkAction * action, GHidPort * port)
{
  /* Transient setting, not saved in Settings & not used for new PCB flag. */
  if (!ghidgui->toggle_holdoff)
    hid_actionl ("Display", "ToggleThindraw", "", NULL);
}

static void
toggle_check_planes_cb (GtkAction * action, GHidPort * port)
{
  /* Transient setting, not saved in Settings & not used for new PCB flag. */
  hid_actionl ("Display", "ToggleCheckPlanes", "", NULL);
}

static void
toggle_vendor_drill_mapping_cb (GtkAction * action, GHidPort * port)
{
  /* Transient setting, not saved in Settings & not used for new PCB flag. */
  hid_action ("ToggleVendor");
}


/* ============== BufferMenu callbacks =============== */
#define	PRESS_BUTTON_ELEMENT_PROMPT	\
		_("Press a button on a reference point for your selection")

static void
copy_selection_to_buffer_cb (GtkAction * action, GHidPort * port)
{
  if (!ghid_control_is_pressed ())
    {
      int x, y;
      gui->get_coords (PRESS_BUTTON_ELEMENT_PROMPT, &x, &y);
    }
  hid_actionl ("PasteBuffer", "Clear", "", NULL);
  hid_actionl ("PasteBuffer", "AddSelected", "", NULL);
  hid_actionl ("Mode", "PasteBuffer", NULL);
}


/* ============== PopupMenu callbacks =============== */

static void
radio_select_tool_cb (GtkAction * action, GtkRadioAction * current)
{
  const gchar *saction;

  saction = gtk_action_get_name (GTK_ACTION (current));
  g_message ("Grid setting action: \"%s\"", saction);
}



/* ======================================================================
|  Here are the action entries that connect menuitems to the
|  above callbacks.
*/

  /* These get NULL labels because labels are dynamically set in
     |  ghid_change_selected_update_menu_actions()
   */
#ifdef DAN_FIXME
static GtkActionEntry change_selected_entries[] = {
  /* name, stock_id, label, accelerator, tooltip callback */
  {"-LinesChange", NULL, NULL, NULL, NULL,
   G_CALLBACK (selected_lines_size_change_cb)},
  {"+LinesChange", NULL, NULL, NULL, NULL,
   G_CALLBACK (selected_lines_size_change_cb)},
  {"-PadsChange", NULL, NULL, NULL, NULL,
   G_CALLBACK (selected_pads_size_change_cb)},
  {"+PadsChange", NULL, NULL, NULL, NULL,
   G_CALLBACK (selected_pads_size_change_cb)},
  {"-PinsChange", NULL, NULL, NULL, NULL,
   G_CALLBACK (selected_pins_size_change_cb)},
  {"+PinsChange", NULL, NULL, NULL, NULL,
   G_CALLBACK (selected_pins_size_change_cb)},
  {"-TextChange", NULL, NULL, NULL, NULL,
   G_CALLBACK (selected_text_size_change_cb)},
  {"+TextChange", NULL, NULL, NULL, NULL,
   G_CALLBACK (selected_text_size_change_cb)},
  {"-ViasChange", NULL, NULL, NULL, NULL,
   G_CALLBACK (selected_vias_size_change_cb)},
  {"+ViasChange", NULL, NULL, NULL, NULL,
   G_CALLBACK (selected_vias_size_change_cb)},

  {"-ViasDrillChange", NULL, NULL, NULL, NULL,
   G_CALLBACK (selected_vias_drill_change_cb)},
  {"+ViasDrillChange", NULL, NULL, NULL, NULL,
   G_CALLBACK (selected_vias_drill_change_cb)},
  {"-PinsDrillChange", NULL, NULL, NULL, NULL,
   G_CALLBACK (selected_pins_drill_change_cb)},
  {"+PinsDrillChange", NULL, NULL, NULL, NULL,
   G_CALLBACK (selected_pins_drill_change_cb)},
};

static gint n_change_selected_entries =
G_N_ELEMENTS (change_selected_entries);
#endif

static GtkRadioActionEntry radio_grid_mil_setting_entries[] = {
  /* name, stock_id, label, accelerator, tooltip, value */
  {"grid-user", NULL, "user value", NULL, NULL, 0},
  {"grid0", NULL, "0.1 mil", NULL, NULL, 0},
  {"grid1", NULL, "0.2 mil", NULL, NULL, 1},
  {"grid2", NULL, "0.5 mil", NULL, NULL, 2},
  {"grid3", NULL, "1 mil", NULL, NULL, 3},
  {"grid4", NULL, "2 mil", NULL, NULL, 4},
  {"grid5", NULL, "5 mil", NULL, NULL, 5},
  {"grid6", NULL, "10 mil", NULL, NULL, 6},
  {"grid7", NULL, "20 mil", NULL, NULL, 7},
  {"grid8", NULL, "25 mil", NULL, NULL, 8},
  {"grid9", NULL, "50 mil", NULL, NULL, 9},
  {"grid10", NULL, "100 mil", NULL, NULL, 10}
};

static gint n_radio_grid_mil_setting_entries
  = G_N_ELEMENTS (radio_grid_mil_setting_entries);


static GtkRadioActionEntry radio_grid_mm_setting_entries[] = {
  /* name, stock_id, label, accelerator, tooltip, value */
  {"grid-user", NULL, "user value", NULL, NULL, 0},
  {"grid0", NULL, "0.002 mm", NULL, NULL, 0},
  {"grid1", NULL, "0.005 mm", NULL, NULL, 1},
  {"grid2", NULL, "0.01 mm", NULL, NULL, 2},
  {"grid3", NULL, "0.02 mm", NULL, NULL, 3},
  {"grid4", NULL, "0.05 mm", NULL, NULL, 4},
  {"grid5", NULL, "0.1 mm", NULL, NULL, 5},
  {"grid6", NULL, "0.2 mm", NULL, NULL, 6},
  {"grid7", NULL, "0.25 mm", NULL, NULL, 7},
  {"grid8", NULL, "0.5 mm", NULL, NULL, 8},
  {"grid9", NULL, "1 mm", NULL, NULL, 9},
  {"grid10", NULL, "2 mm", NULL, NULL, 10},
};

static gint n_radio_grid_mm_setting_entries
  = G_N_ELEMENTS (radio_grid_mm_setting_entries);


static GtkRadioActionEntry radio_displayed_element_name_entries[] = {
  /* name, stock_id, label, accelerator, tooltip, value */
  {"Description", NULL, N_("Description"), NULL, NULL, 0},
  {"ReferenceDesignator", NULL, N_("Reference designator"),
   NULL, NULL, 1},
  {"Value", NULL, N_("Value"), NULL, NULL, 2}
};

static gint n_radio_displayed_element_name_entries
  = G_N_ELEMENTS (radio_displayed_element_name_entries);


static GtkRadioActionEntry radio_select_current_buffer_entries[] = {
  /* name, stock_id, label, accelerator, tooltip, value */
  {"SelectBuffer1", NULL, "#1", "<control>1", NULL, 1},
  {"SelectBuffer2", NULL, "#2", "<control>2", NULL, 2},
  {"SelectBuffer3", NULL, "#3", "<control>3", NULL, 3},
  {"SelectBuffer4", NULL, "#4", "<control>4", NULL, 4},
  {"SelectBuffer5", NULL, "#5", "<control>5", NULL, 5}
};

static gint n_radio_select_current_buffer_entries
  = G_N_ELEMENTS (radio_select_current_buffer_entries);

static GtkRadioActionEntry radio_select_tool_entries[] = {
  /* name, stock_id, label, accelerator, tooltip, value */
  {"SelectLineTool", NULL, N_("Line"), NULL, NULL, 0},
  {"SelectViaTool", NULL, N_("Via"), NULL, NULL, 0},
  {"SelectRectangleTool", NULL, N_("Rectangle"), NULL, NULL, 0},
  {"SelectSelectionTool", NULL, N_("Selection"), NULL, NULL, 0},
  {"SelectTextTool", NULL, N_("Text"), NULL, NULL, 0},
  {"SelectPannerTool", NULL, N_("Panner"), NULL, NULL, 0},
};

static gint n_radio_select_tool_entries
  = G_N_ELEMENTS (radio_select_tool_entries);

#ifdef DAN_FIXME
static GtkToggleActionEntry toggle_entries[] = {
  /* name, stock_id, label, accelerator, tooltip, callback, is_active */

  /* ViewMenu */
  {"ToggleDrawGrid", NULL, N_("Enable visible grid"), NULL, NULL,
   G_CALLBACK (toggle_draw_grid_cb), FALSE},
  {"ToggleGridUnitsMm", NULL, N_("Enable millimeter grid units"), NULL, NULL,
   G_CALLBACK (toggle_grid_units_cb), FALSE},
  {"ToggleViewSolderSide", NULL, N_("Enable view solder side"),
   NULL, NULL,
   G_CALLBACK (toggle_view_solder_side_cb)},
  {"TogglePinoutShowsNumber", NULL, N_("Enable pinout shows number"),
   NULL, NULL,
   G_CALLBACK (toggle_pinout_shows_number_cb)},

/* SettingsMenu */
  {"Toggle45degree", NULL, N_("All direction lines"),
   NULL /* gtk wont take "." */ , NULL,
   G_CALLBACK (toggle_45_degree_cb)},
  {"ToggleStartDirection", NULL, N_("Auto swap line start angle"),
   NULL, NULL,
   G_CALLBACK (toggle_start_direction_cb)},
  {"ToggleOrthogonalMoves", NULL, N_("Orthogonal moves"), NULL, NULL,
   G_CALLBACK (toggle_orthogonal_moves_cb)},
  {"ToggleSnapPin", NULL, N_("Crosshair snaps to pins and pads"),
   NULL, NULL,
   G_CALLBACK (toggle_snap_pin_cb)},
  {"ToggleShowDRC", NULL, N_("Crosshair shows DRC clearance"),
   NULL, NULL,
   G_CALLBACK (toggle_show_DRC_cb)},
  {"ToggleAutoDrC", NULL, N_("Auto enforce DRC clearance"),
   NULL, NULL,
   G_CALLBACK (toggle_auto_DRC_cb)},
  {"ToggleRubberBand", NULL, N_("Rubber band mode"), NULL, NULL,
   G_CALLBACK (toggle_rubber_band_cb)},
  {"ToggleUniqueNames", NULL, N_("Require unique element names"),
   NULL, NULL,
   G_CALLBACK (toggle_unique_names_cb)},
  {"ToggleLocalRef", NULL, N_("Auto zero delta measurements"),
   NULL, NULL,
   G_CALLBACK (toggle_local_ref_cb)},
  {"ToggleClearLine", NULL, N_("New lines, arcs clear polygons"),
   NULL, NULL,
   G_CALLBACK (toggle_clear_line_cb)},
  {"ToggleLiveRoute", NULL, N_("Show autorouter trials"), NULL, NULL,
   G_CALLBACK (toggle_live_route_cb)},
  {"ToggleThinDraw", NULL, N_("Thin line draw"),
   NULL /* Gtk can't take '\' or '|' */ , NULL,
   G_CALLBACK (toggle_thin_draw_cb)},
  {"ToggleCheckPlanes", NULL, N_("Check polygons"), NULL, NULL,
   G_CALLBACK (toggle_check_planes_cb)},
  {"ToggleVendorDrillMapping", NULL, N_("Vendor drill mapping"),
   NULL, NULL,
   G_CALLBACK (toggle_vendor_drill_mapping_cb)},

/* ConnectsMenu */
  {"ToggleOnlyAutoRoutedNets", NULL, N_("Enable only autorouted nets"),
   NULL, NULL,
   G_CALLBACK (toggle_only_auto_routed_cb)},
};

static gint n_toggle_entries = G_N_ELEMENTS (toggle_entries);

#endif


  /* When user toggles grid units mil<->mm or when a new layout is loaded
     |  that might use different units, the "Change size of selected" menu
     |  displayed text muxt be changed.
   */
void
ghid_change_selected_update_menu_actions (void)
{
#ifdef DAN_FIXME
  gchar size_buf[32], buf[128];

  if (ghidgui->change_selected_actions)
    {
      /* Remove the existing change selected actions from the menu.
       */
      gtk_ui_manager_remove_action_group (ghidgui->ui_manager,
					  ghidgui->change_selected_actions);
      g_object_unref (ghidgui->change_selected_actions);
    }

  /* And create a new action group for the changed selection actions.
   */
  ghidgui->change_selected_actions =
    gtk_action_group_new ("ChangeSelActions");
  gtk_action_group_set_translation_domain (ghidgui->change_selected_actions,
					   NULL);
  gtk_ui_manager_insert_action_group (ghidgui->ui_manager,
				      ghidgui->change_selected_actions, 0);

  /* Update the labels to match current units and increment values settings.
   */
  if (Settings.grid_units_mm)
    snprintf (size_buf, sizeof (size_buf), "%.2f mm",
	      Settings.size_increment_mm);
  else
    snprintf (size_buf, sizeof (size_buf), "%.0f mil",
	      Settings.size_increment_mil);

  snprintf (buf, sizeof (buf), _("Decrement lines by %s"), size_buf);
  dup_string ((gchar **) & change_selected_entries[0].label, buf);

  snprintf (buf, sizeof (buf), _("Increment lines by %s"), size_buf);
  dup_string ((gchar **) & change_selected_entries[1].label, buf);

  snprintf (buf, sizeof (buf), _("Decrement pads by %s"), size_buf);
  dup_string ((gchar **) & change_selected_entries[2].label, buf);

  snprintf (buf, sizeof (buf), _("Increment pads by %s"), size_buf);
  dup_string ((gchar **) & change_selected_entries[3].label, buf);

  snprintf (buf, sizeof (buf), _("Decrement pins by %s"), size_buf);
  dup_string ((gchar **) & change_selected_entries[4].label, buf);

  snprintf (buf, sizeof (buf), _("Increment pins by %s"), size_buf);
  dup_string ((gchar **) & change_selected_entries[5].label, buf);

  snprintf (buf, sizeof (buf), _("Decrement text by %s"), size_buf);
  dup_string ((gchar **) & change_selected_entries[6].label, buf);

  snprintf (buf, sizeof (buf), _("Increment text by %s"), size_buf);
  dup_string ((gchar **) & change_selected_entries[7].label, buf);

  snprintf (buf, sizeof (buf), _("Decrement vias by %s"), size_buf);
  dup_string ((gchar **) & change_selected_entries[8].label, buf);

  snprintf (buf, sizeof (buf), _("Increment vias by %s"), size_buf);
  dup_string ((gchar **) & change_selected_entries[9].label, buf);

  /* -- Drill size changes */
  snprintf (buf, sizeof (buf), _("Decrement vias by %s"), size_buf);
  dup_string ((gchar **) & change_selected_entries[10].label, buf);

  snprintf (buf, sizeof (buf), _("Increment vias by %s"), size_buf);
  dup_string ((gchar **) & change_selected_entries[11].label, buf);

  snprintf (buf, sizeof (buf), _("Decrement pins by %s"), size_buf);
  dup_string ((gchar **) & change_selected_entries[12].label, buf);

  snprintf (buf, sizeof (buf), _("Increment pins by %s"), size_buf);
  dup_string ((gchar **) & change_selected_entries[13].label, buf);

  /* And add the actions with new labels back in.
   */
  gtk_action_group_add_actions (ghidgui->change_selected_actions,
				change_selected_entries,
				n_change_selected_entries, &Output);
#endif
}

  /* Grid setting labels must also match user and new layout unit changes.
   */
void
ghid_grid_setting_update_menu_actions (void)
{
  GtkAction *action;
  gint i;

  if (ghidgui->grid_actions)
    {
      /* Remove the existing radio grid actions from the menu.
       */
      gtk_ui_manager_remove_action_group (ghidgui->ui_manager,
					  ghidgui->grid_actions);
      g_object_unref (ghidgui->grid_actions);
    }

  /* And add back actions appropriate for mil or mm grid settings.
   */
  ghidgui->grid_actions = gtk_action_group_new ("GridActions");
  gtk_action_group_set_translation_domain (ghidgui->grid_actions, NULL);
  gtk_ui_manager_insert_action_group (ghidgui->ui_manager,
				      ghidgui->grid_actions, 0);

  /* Get the index of the radio button to set depending on current
     |  PCB Grid value.  But if user hits 'g' key and no grid index matches,
     |  'i' will be -1 and no button will be set active.  At least Gtk docs
     |  say so, but I see different.
   */
  i = get_grid_value_index (TRUE);

  if (Settings.grid_units_mm)
    gtk_action_group_add_radio_actions (ghidgui->grid_actions,
					radio_grid_mm_setting_entries,
					n_radio_grid_mm_setting_entries,
					i,
					G_CALLBACK (radio_grid_mm_setting_cb),
					NULL);
  else
    gtk_action_group_add_radio_actions (ghidgui->grid_actions,
					radio_grid_mil_setting_entries,
					n_radio_grid_mil_setting_entries,
					i,
					G_CALLBACK
					(radio_grid_mil_setting_cb), NULL);
  action = gtk_action_group_get_action (ghidgui->grid_actions, "grid-user");
  if (action)
    g_object_set (action, "sensitive", FALSE, NULL);
}


  /* When a new layout is loaded, must set the radio state to the current
     |  "Displayed element name".  Now I unload and reload the actions so
     |  an initial value can be set, but there must be a better way?
   */
static void
update_displayed_name_actions (void)
{
  gint i;

  if (ghidgui->displayed_name_actions)
    {
      /* Remove the existing radio actions from the menu.
       */
      gtk_ui_manager_remove_action_group (ghidgui->ui_manager,
					  ghidgui->displayed_name_actions);
      g_object_unref (ghidgui->displayed_name_actions);
    }

  /* And add back actions just to get the initial one set.
   */
  ghidgui->displayed_name_actions = gtk_action_group_new ("DispNameActions");
  gtk_action_group_set_translation_domain (ghidgui->displayed_name_actions,
					   NULL);
  gtk_ui_manager_insert_action_group (ghidgui->ui_manager,
				      ghidgui->displayed_name_actions, 0);

  /* Get the index of the radio button to set
   */
  if (TEST_FLAG (DESCRIPTIONFLAG, PCB))
    i = 0;
  else if (TEST_FLAG (NAMEONPCBFLAG, PCB))
    i = 1;
  else
    i = 2;

  gtk_action_group_add_radio_actions (ghidgui->displayed_name_actions,
				      radio_displayed_element_name_entries,
				      n_radio_displayed_element_name_entries,
				      i,
				      G_CALLBACK
				      (radio_displayed_element_name_cb),
				      NULL);
}

void
ghid_set_menu_toggle_button (GtkActionGroup * ag, gchar * name,
			     gboolean state)
{
  GtkAction *action;
  gboolean old_holdoff;

  old_holdoff = ghidgui->toggle_holdoff;
  ghidgui->toggle_holdoff = TRUE;
  action = gtk_action_group_get_action (ag, name);
  if (action)
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), state);
  ghidgui->toggle_holdoff = old_holdoff;
}

  /* Sync toggle states that were saved with the layout and notify the
     |  config code to update Settings values it manages.
   */
void
ghid_sync_with_new_layout (void)
{
  GtkAction *action;
  gboolean old_holdoff;

  /* Just want to update the state of the menus without calling the
     |  action functions at this time because causing a toggle action can
     |  undo the initial condition set we want here.
   */
  old_holdoff = ghidgui->toggle_holdoff;
  ghidgui->toggle_holdoff = TRUE;

  action =
    gtk_action_group_get_action (ghidgui->main_actions, "ToggleDrawGrid");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				Settings.DrawGrid);
/*	g_object_set(action, "sensitive", Settings.XXX, NULL); */

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"ToggleGridUnitsMm");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				Settings.grid_units_mm);

  /* Toggle actions in the menu which set a PCB flag must be set to
     |  the new layout PCB flag states.  Transient toggle buttons which
     |  do not set a PCB flag don't need setting here.
   */
  action = gtk_action_group_get_action (ghidgui->main_actions,
					"TogglePinoutShowsNumber");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				TEST_FLAG (SHOWNUMBERFLAG, PCB));

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"Toggle45degree");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				TEST_FLAG (ALLDIRECTIONFLAG, PCB));

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"ToggleRubberBand");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				TEST_FLAG (RUBBERBANDFLAG, PCB));

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"ToggleStartDirection");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				TEST_FLAG (SWAPSTARTDIRFLAG, PCB));

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"ToggleUniqueNames");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				TEST_FLAG (UNIQUENAMEFLAG, PCB));

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"ToggleSnapPin");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				TEST_FLAG (SNAPPINFLAG, PCB));

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"ToggleClearLine");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				TEST_FLAG (CLEARNEWFLAG, PCB));

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"ToggleOrthogonalMoves");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				TEST_FLAG (ORTHOMOVEFLAG, PCB));

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"ToggleLiveRoute");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				TEST_FLAG (LIVEROUTEFLAG, PCB));

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"ToggleShowDRC");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				TEST_FLAG (SHOWDRCFLAG, PCB));

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"ToggleAutoDrC");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				TEST_FLAG (AUTODRCFLAG, PCB));

  /* Not sure if I can set a radio button without loading actions, so
     |  load the actions.
   */
  ghid_grid_setting_update_menu_actions ();
  update_displayed_name_actions ();

  ghidgui->toggle_holdoff = old_holdoff;

  pcb_use_route_style (&PCB->RouteStyle[0]);

  ghid_route_style_button_set_active (0);
  ghid_config_handle_units_changed ();
  ghid_change_selected_update_menu_actions ();
  ghid_set_status_line_label ();
}

/*
 * Sync toggle states in the menus at startup to Settings values loaded
 * in the config.
 */
void
ghid_init_toggle_states (void)
{
  GtkAction *action;
  gboolean old_holdoff;

  /* Just want to update the state of the menus without calling the
     |  action functions at this time because causing a toggle action can
     |  undo the initial condition set we want here.
   */
  old_holdoff = ghidgui->toggle_holdoff;
  ghidgui->toggle_holdoff = TRUE;

  action =
    gtk_action_group_get_action (ghidgui->main_actions, "ToggleDrawGrid");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				Settings.DrawGrid);

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"ToggleGridUnitsMm");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				Settings.grid_units_mm);

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"TogglePinoutShowsNumber");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				Settings.ShowNumber);

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"Toggle45degree");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				Settings.AllDirectionLines);

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"ToggleRubberBand");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				Settings.RubberBandMode);

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"ToggleStartDirection");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				Settings.SwapStartDirection);

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"ToggleUniqueNames");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				Settings.UniqueNames);

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"ToggleSnapPin");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), Settings.SnapPin);

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"ToggleClearLine");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				Settings.ClearLine);

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"ToggleOrthogonalMoves");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				Settings.OrthogonalMoves);

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"ToggleLiveRoute");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				Settings.liveRouting);

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"ToggleShowDRC");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), Settings.ShowDRC);

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"ToggleAutoDrC");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), Settings.AutoDRC);

  ghidgui->toggle_holdoff = old_holdoff;
  ghid_set_status_line_label ();
}

/*
 * The intial loading of all actions at startup.
 */
static void
make_menu_actions (GtkActionGroup * actions, GHidPort * port)
{
  gtk_action_group_add_actions (actions, new_entries, menuitem_cnt, port);
  gtk_action_group_add_toggle_actions (actions, new_toggle_entries, tmenuitem_cnt, port);

  /* Handle menu actions with dynamic content.
   */
#ifdef DAN_FIXME
  ghid_change_selected_update_menu_actions ();
  ghid_grid_setting_update_menu_actions ();
  update_displayed_name_actions ();

  gtk_action_group_add_radio_actions (actions,
				      radio_select_current_buffer_entries,
				      n_radio_select_current_buffer_entries,
				      0,
				      G_CALLBACK
				      (radio_select_current_buffer_cb), NULL);

  gtk_action_group_add_radio_actions (actions,
				      radio_select_tool_entries,
				      n_radio_select_tool_entries,
				      0,
				      G_CALLBACK (radio_select_tool_cb),
				      NULL);

  gtk_action_group_add_toggle_actions (actions,
				       toggle_entries, n_toggle_entries,
				       port);
#endif
}


/*
 * Make a frame for the top menubar, load in actions for the menus and
 * load the ui_manager string.
 */
static void
make_top_menubar (GtkWidget * hbox, GHidPort * port)
{
  GtkUIManager *ui;
  GtkWidget *frame;
  GtkActionGroup *actions;
  GError *error = NULL;


  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);

  ui = gtk_ui_manager_new ();
  ghidgui->ui_manager = ui;

  actions = gtk_action_group_new ("Actions");
  gtk_action_group_set_translation_domain (actions, NULL);
  ghidgui->main_actions = actions;

  make_menu_actions (actions, port);

  gtk_ui_manager_insert_action_group (ui, actions, 0);
  gtk_window_add_accel_group (GTK_WINDOW (gport->top_window),
			      gtk_ui_manager_get_accel_group (ui));

  /* For user customization, we could add
     |  gtk_menu_item_set_accel_path(), gtk_accel_map_save (), etc
     |  But probably can't do this because of command combo box interaction.
   */

  //  if (!gtk_ui_manager_add_ui_from_string (ui, ui_info, -1, &error))
  if (!gtk_ui_manager_add_ui_from_string (ui, new_ui_info, -1, &error))
    {
      g_message ("building menus failed: %s", error->message);
      g_error_free (error);
    }
  gtk_ui_manager_set_add_tearoffs (ui, TRUE);
  gtk_container_add (GTK_CONTAINER (frame),
		     gtk_ui_manager_get_widget (ui, "/MenuBar"));
}


/* Set the PCB name on a label or on the window title bar.
 */
void
ghid_window_set_name_label (gchar * name)
{
  gchar *str;

  /* FIXME -- should this happen?  It does... */
  /* This happens if we're calling an exporter from the command line */
  if (ghidgui == NULL)
    return;

  dup_string (&(ghidgui->name_label_string), name);
  if (!ghidgui->name_label_string || !*ghidgui->name_label_string)
    ghidgui->name_label_string = g_strdup (_("Unnamed"));

  if (!ghidgui->name_label)
    return;

  if (ghidgui->ghid_title_window)
    {
      gtk_widget_hide (ghidgui->label_hbox);
      str = g_strdup_printf ("PCB:  %s", ghidgui->name_label_string);
      gtk_window_set_title (GTK_WINDOW (gport->top_window), str);
    }
  else
    {
			gtk_widget_show (ghidgui->label_hbox);
      str = g_strdup_printf (" <b><big>%s</big></b> ",
			     ghidgui->name_label_string);
      gtk_label_set_markup (GTK_LABEL (ghidgui->name_label), str);
      gtk_window_set_title (GTK_WINDOW (gport->top_window), "PCB");
    }
  g_free (str);
}

static void
grid_units_button_cb (GtkWidget * widget, gpointer data)
{
  GtkAction *action;

  /* Do handling common to when units are changed from the menu.
   */
  handle_grid_units_change (!Settings.grid_units_mm);

  action = gtk_action_group_get_action (ghidgui->main_actions,
					"ToggleGridUnitsMm");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				Settings.grid_units_mm);
}

static void
make_cursor_position_labels (GtkWidget * hbox, GHidPort * port)
{
  GtkWidget *frame, *label, *button;

  /* The grid units button next to the cursor position labels.
   */
  button = gtk_button_new ();
  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
			Settings.grid_units_mm ?
			"<b>mm</b> " : "<b>mil</b> ");
  ghidgui->grid_units_label = label;
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_container_add (GTK_CONTAINER (button), label);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, TRUE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (grid_units_button_cb), NULL);

  /* The absolute cursor position label
   */
  frame = gtk_frame_new (NULL);
  gtk_box_pack_end (GTK_BOX (hbox), frame, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (frame), 2);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);

  label = gtk_label_new ("");
  gtk_container_add (GTK_CONTAINER (frame), label);
  ghidgui->cursor_position_absolute_label = label;

  /* The relative cursor position label
   */
  frame = gtk_frame_new (NULL);
  gtk_box_pack_end (GTK_BOX (hbox), frame, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (frame), 2);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);
  label = gtk_label_new (" __.__  __.__ ");
  gtk_container_add (GTK_CONTAINER (frame), label);
  ghidgui->cursor_position_relative_label = label;
}


  /* ------------------------------------------------------------------
     |  Handle the layer buttons.
   */
typedef struct
{
  GtkWidget *radio_select_button,
    *layer_enable_button, *layer_enable_ebox, *label;
  gchar *text;
  gint index;
}
LayerButtonSet;

static LayerButtonSet layer_buttons[N_LAYER_BUTTONS];

static gint layer_select_button_index;

static gboolean layer_enable_button_cb_hold_off,
  layer_select_button_cb_hold_off;

static void
layer_select_button_cb (GtkWidget * widget, LayerButtonSet * lb)
{
  gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  if (!active || layer_select_button_cb_hold_off)
    return;

  PCB->SilkActive = (lb->index == LAYER_BUTTON_SILK);
  PCB->RatDraw = (lb->index == LAYER_BUTTON_RATS);

  if (lb->index < max_layer)
    ChangeGroupVisibility (lb->index, True, True);

  layer_select_button_index = lb->index;

  ghid_invalidate_all ();
}

static void
layer_enable_button_cb (GtkWidget * widget, gpointer data)
{
  LayerButtonSet *lb;
  gint i, group, layer = GPOINTER_TO_INT (data);
  gboolean active, redraw = FALSE;

  active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  if (layer_enable_button_cb_hold_off)
    return;

  lb = &layer_buttons[layer];
  switch (layer)
    {
    case LAYER_BUTTON_SILK:
      PCB->ElementOn = !PCB->ElementOn;
      PCB->Data->SILKLAYER.On = PCB->ElementOn;
      PCB->Data->BACKSILKLAYER.On = PCB->ElementOn;
      redraw = 1;
      break;

    case LAYER_BUTTON_RATS:
      PCB->RatOn = !PCB->RatOn;
      redraw = 1;
      break;

    case LAYER_BUTTON_PINS:
      PCB->PinOn = active;
      redraw |= (PCB->Data->ElementN != 0);
      break;

    case LAYER_BUTTON_VIAS:
      PCB->ViaOn = active;
      redraw |= (PCB->Data->ViaN != 0);
      break;

    case LAYER_BUTTON_FARSIDE:
      PCB->InvisibleObjectsOn = active;
      PCB->Data->BACKSILKLAYER.On = (active && PCB->ElementOn);
      redraw = TRUE;
      break;

    case LAYER_BUTTON_MASK:
      if (active)
	SET_FLAG (SHOWMASKFLAG, PCB);
      else
	CLEAR_FLAG (SHOWMASKFLAG, PCB);
      redraw = TRUE;
      break;

    default:
      /* check if active layer is in the group;
         |  if YES, make a different one active if possible.  Logic from
         |  Xt PCB code.
       */
      if ((group = GetGroupOfLayer (layer)) ==
	  GetGroupOfLayer (MIN (max_layer, INDEXOFCURRENT)))
	{
	  for (i = (layer + 1) % (max_layer + 1); i != layer;
	       i = (i + 1) % (max_layer + 1))
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
	      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
	      return;
	    }
	}
      /* switch layer group on/off */
      ChangeGroupVisibility (layer, active, False);
      redraw = TRUE;
      break;
    }
  if (redraw)
    ghid_invalidate_all();
}

static void
layer_button_set_color (LayerButtonSet * lb, gchar * color_string)
{
  GdkColor color;

  if (!lb->layer_enable_ebox)
    return;
  
  color.red = color.green = color.blue = 0;
  ghid_map_color_string (color_string, &color);
  gtk_widget_modify_bg (lb->layer_enable_ebox, GTK_STATE_ACTIVE, &color);
  gtk_widget_modify_bg (lb->layer_enable_ebox, GTK_STATE_PRELIGHT, &color);

  gtk_widget_modify_fg (lb->label, GTK_STATE_ACTIVE, &WhitePixel);
}

void
layer_enable_button_set_label (GtkWidget * label, gchar * text)
{
  gchar *s;

  if (ghidgui->small_label_markup)
    s = g_strdup_printf ("<small>%s</small>", text);
  else
    s = g_strdup (text);
  gtk_label_set_markup (GTK_LABEL (label), s);
  g_free (s);
}

static void
ghid_show_layer_buttons(void)
{
	LayerButtonSet *lb;
	gint	i;

	for (i = 0; i < MAX_LAYER; ++i)
	{
		lb = &layer_buttons[i];
		if (i < max_layer)
		  {
			gtk_widget_show(lb->layer_enable_button);
			gtk_widget_show(lb->radio_select_button);
		  }
		else
		  {
			gtk_widget_hide(lb->layer_enable_button);
			gtk_widget_hide(lb->radio_select_button);
		  }
	}
}

  /* After layers comes some special cases.  Since silk and netlist (rats)
     |  are selectable as separate drawing areas, they are more consistently
     |  placed after the layers in the gui so the select radio buttons will
     |  be grouped.  This is different from Xt PCB which had a different looking
     |  select interface.
   */
static void
make_layer_buttons (GtkWidget * vbox, GHidPort * port)
{
  LayerButtonSet *lb;
  GtkWidget *table, *ebox, *label, *button, *hbox;
  GSList *group = NULL;
  gchar *text;
  gint i;
  gchar *color_string;
  gboolean active = TRUE;

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
  table = gtk_table_new (N_LAYER_BUTTONS, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 3);

  for (i = 0; i < N_LAYER_BUTTONS; ++i)
    {
      lb = &layer_buttons[i];
      lb->index = i;

      if (i < N_SELECTABLE_LAYER_BUTTONS)
	{
	  button = gtk_radio_button_new (group);
	  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
	  gtk_table_attach_defaults (GTK_TABLE (table), button,
				     0, 1, i, i + 1);

	  lb->radio_select_button = button;
	  g_signal_connect (G_OBJECT (button), "toggled",
			    G_CALLBACK (layer_select_button_cb), lb);
	}

      switch (i)
	{
	case LAYER_BUTTON_SILK:
	  color_string = PCB->ElementColor;
	  text = _("silk");
	  active = PCB->ElementOn;
	  break;

	case LAYER_BUTTON_RATS:
	  color_string = PCB->RatColor;
	  text = _("rat lines");
	  active = PCB->RatOn;
	  break;

	case LAYER_BUTTON_PINS:
	  color_string = PCB->PinColor;
	  text = _("pins/pads");
	  active = PCB->PinOn;
	  break;

	case LAYER_BUTTON_VIAS:
	  color_string = PCB->ViaColor;
	  text = _("vias");
	  active = PCB->ViaOn;
	  break;

	case LAYER_BUTTON_FARSIDE:
	  color_string = PCB->InvisibleObjectsColor;
	  text = _("far side");
	  active = PCB->InvisibleObjectsOn;
	  break;

	case LAYER_BUTTON_MASK:
	  color_string = PCB->MaskColor;
	  text = _("solder mask");
	  active = TEST_FLAG (SHOWMASKFLAG, PCB);
	  break;

	default:
	  color_string = PCB->Data->Layer[i].Color;
	  text = UNKNOWN (PCB->Data->Layer[i].Name);
	  text = _(text);
	  active = PCB->Data->Layer[i].On;
	  break;
	}
      button = gtk_check_button_new ();
      label = gtk_label_new ("");
      gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
      layer_enable_button_set_label (label, text);

      ebox = gtk_event_box_new ();
      gtk_container_add (GTK_CONTAINER (ebox), label);
      gtk_container_add (GTK_CONTAINER (button), ebox);
      gtk_table_attach_defaults (GTK_TABLE (table), button, 1, 2, i, i + 1);
/*		gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0); */
      gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);

      lb->layer_enable_button = button;
      lb->layer_enable_ebox = ebox;
      lb->text = g_strdup (text);
      lb->label = label;

      layer_button_set_color (lb, color_string);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), active);

      g_signal_connect (G_OBJECT (button), "toggled",
			G_CALLBACK (layer_enable_button_cb),
			GINT_TO_POINTER (i));
    }
}

  /* If new color scheme is loaded from the config or user changes a color
     |  in the preferences, make sure our layer button colors get updated.
   */
void
ghid_layer_buttons_color_update (void)
{
  gchar *color_string;
  LayerButtonSet *lb;
  gint i;

  if (!gport->drawing_area)
    return;

  /* Fixme: should the color set be maintained in both the PCB and the
     |  Settings struct?
   */
  pcb_colors_from_settings (PCB);

  for (i = 0; i < N_LAYER_BUTTONS; ++i)
    {
      lb = &layer_buttons[i];

      if (i == LAYER_BUTTON_SILK)
	color_string = PCB->ElementColor;
      else if (i == LAYER_BUTTON_RATS)
	color_string = PCB->RatColor;
      else if (i == LAYER_BUTTON_PINS)
	color_string = PCB->PinColor;
      else if (i == LAYER_BUTTON_VIAS)
	color_string = PCB->ViaColor;
      else if (i == LAYER_BUTTON_FARSIDE)
	color_string = PCB->InvisibleObjectsColor;
      else if (i == LAYER_BUTTON_MASK)
	color_string = PCB->MaskColor;
      else
	color_string = PCB->Data->Layer[i].Color;

      layer_button_set_color (lb, color_string);
    }
}


  /* Update layer button labels and enabled state to match current PCB.
   */
void
ghid_layer_enable_buttons_update (void)
{
  LayerButtonSet *lb;
  gchar *s;
  gint i;


  /* Update layer button labels and active state to state inside of PCB
   */
  layer_enable_button_cb_hold_off = TRUE;
  for (i = 0; i < max_layer; ++i)
    {
      lb = &layer_buttons[i];
      s = UNKNOWN (PCB->Data->Layer[i].Name);
      if (dup_string (&lb->text, s))
	{
	  layer_enable_button_set_label (lb->label, _(s));
	  ghid_config_layer_name_update (_(s), i);
	}
      if (Settings.verbose)
	{
	  gboolean active, new;

	  active =
	    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					  (lb->layer_enable_button));
	  new = PCB->Data->Layer[i].On;
	  if (active != new)
	    printf ("ghid_layer_enable_buttons_update: active=%d new=%d\n",
		    active, new);
	}
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				    (lb->layer_enable_button),
				    PCB->Data->Layer[i].On);
    }
  /* Buttons for elements (silk), rats, pins, vias, and far side don't
     |  change labels.
   */
  lb = &layer_buttons[i++];	/* LAYER_BUTTON_SILK */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lb->layer_enable_button),
				PCB->ElementOn);

  lb = &layer_buttons[i++];	/* LAYER_BUTTON_RATS */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lb->layer_enable_button),
				PCB->RatOn);

  lb = &layer_buttons[i++];	/* pins/pads    */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lb->layer_enable_button),
				PCB->PinOn);

  lb = &layer_buttons[i++];	/* vias         */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lb->layer_enable_button),
				PCB->ViaOn);

  lb = &layer_buttons[i++];	/* far side     */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lb->layer_enable_button),
				PCB->InvisibleObjectsOn);
  layer_enable_button_cb_hold_off = FALSE;
}

void
ghid_layer_button_select (gint layer)
{
  if (layer != layer_select_button_index)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				    (layer_buttons[layer].
				     radio_select_button), TRUE);
      layer_select_button_index = layer;
    }
}

  /* Main layer button synchronization with current PCB state.  Called when
     |  user toggles layer visibility or changes drawing layer or when internal
     |  PCB code changes layer visibility.
   */
void
ghid_layer_buttons_update (void)
{
  gint layer;
  gboolean active = FALSE;

  if (!ghidgui || ghidgui->creating)
    return;

  ghid_layer_enable_buttons_update ();

  /* Turning off a layer that was selected will cause PCB to switch to
     |  another layer.
   */
  if (PCB->RatDraw)
    layer = LAYER_BUTTON_RATS;
  else
    layer = PCB->SilkActive ? LAYER_BUTTON_SILK : LayerStack[0];

  if (layer < max_layer)
    active = PCB->Data->Layer[layer].On;
  else if (layer == LAYER_BUTTON_SILK)
    active = PCB->ElementOn;
  else if (layer == LAYER_BUTTON_RATS)
    active = PCB->RatOn;

  if (Settings.verbose)
    {
      printf ("ghid_layer_buttons_update cur_index=%d update_index=%d\n",
	      layer_select_button_index, layer);
      if (active && layer != layer_select_button_index)
	printf ("\tActivating button %d\n", layer);
    }
  if (active && layer != layer_select_button_index)
    {
      layer_select_button_cb_hold_off = TRUE;
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				    (layer_buttons[layer].
				     radio_select_button), TRUE);
      layer_select_button_index = layer;
      layer_select_button_cb_hold_off = FALSE;
    }
}


  /* ------------------------------------------------------------------
     |  Route style buttons
   */
typedef struct
{
  GtkWidget *button;
  RouteStyleType route_style;
  gboolean shown;		/* For temp buttons */
}
RouteStyleButton;

  /* Make 3 extra route style radio buttons.  2 for the extra Temp route
     |  styles, and the 3rd is an always invisible button selected when the
     |  route style settings in use don't match any defined route style (the
     |  user can hit 'l', 'v', etc keys to change the settings without selecting
     |  a new defined style.
   */
static RouteStyleButton route_style_button[NUM_STYLES + 3];
static gint route_style_index;

static GtkWidget *route_style_edit_button;


static void
route_style_edit_cb (GtkWidget * widget, GHidPort * port)
{
  RouteStyleType *rst = NULL;

  if (route_style_index >= NUM_STYLES)
    rst = &route_style_button[route_style_index].route_style;
  ghid_route_style_dialog (route_style_index, rst);
}

static void
route_style_select_button_cb (GtkToggleButton * button, gpointer data)
{
  RouteStyleType *rst;
  gchar buf[16];
  gint index = GPOINTER_TO_INT (data);

  if (ghidgui->toggle_holdoff || index == NUM_STYLES + 2)
    return;

  if (route_style_index == index)
    return;
  route_style_index = index;

  if (index < NUM_STYLES)
    {
      snprintf (buf, sizeof (buf), "%d", index + 1);
      if (gtk_toggle_button_get_active (button))
	hid_actionl ("RouteStyle", buf, NULL);
    }
  else if (index < NUM_STYLES + 2)
    {
      rst = &route_style_button[index].route_style;
      SetLineSize (rst->Thick);
      SetViaSize (rst->Diameter, TRUE);
      SetViaDrillingHole (rst->Hole, TRUE);
      SetKeepawayWidth (rst->Keepaway);
    }
  gtk_widget_set_sensitive (route_style_edit_button, TRUE);
  ghid_set_status_line_label();
}

static void
ghid_route_style_temp_buttons_hide (void)
{
  gtk_widget_hide (route_style_button[NUM_STYLES].button);
  gtk_widget_hide (route_style_button[NUM_STYLES + 1].button);

  /* This one never becomes visibile.
   */
  gtk_widget_hide (route_style_button[NUM_STYLES + 2].button);
}


static void
make_route_style_buttons (GtkWidget * vbox, GHidPort * port)
{
  GtkWidget *button;
  GSList *group = NULL;
  RouteStyleButton *rbut;
  gint i;

  button = gtk_button_new_with_label (_("Route Style"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (route_style_edit_cb), port);
  route_style_edit_button = button;

  for (i = 0; i < NUM_STYLES + 3; ++i)
    {
      RouteStyleType *rst;
      gchar buf[32];

      rbut = &route_style_button[i];
      if (i < NUM_STYLES)
	{
	  rst = &PCB->RouteStyle[i];
	  button = gtk_radio_button_new_with_label (group, _(rst->Name));
	}
      else
	{
	  snprintf (buf, sizeof (buf), _("Temp%d"), i - NUM_STYLES + 1);
	  button = gtk_radio_button_new_with_label (group, buf);
	  if (!route_style_button[i].route_style.Name)
	    route_style_button[i].route_style.Name = g_strdup (buf);
	}
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      rbut->button = button;
      if (i < NUM_STYLES + 2)
	g_signal_connect (G_OBJECT (button), "toggled",
			  G_CALLBACK (route_style_select_button_cb),
			  GINT_TO_POINTER (i));
    }
}

void
ghid_route_style_button_set_active (gint n)
{
  if (n < 0 || n >= NUM_STYLES + 3)
    return;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				(route_style_button[n].button), TRUE);
}

  /* Upate the route style button selected to match current route settings.
     |  If user has changed an in use route setting so they don't match any
     |  defined route style, select the invisible dummy route style button.
   */
void
ghid_route_style_buttons_update (void)
{
  RouteStyleType *rst;
  gint i;

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
      if (Settings.LineThickness == rst->Thick
	  && Settings.ViaThickness == rst->Diameter
	  && Settings.ViaDrillingHole == rst->Hole
	  && Settings.Keepaway == rst->Keepaway)
	break;
    }
  /* If i == NUM_STYLES + 2 at this point, we activate the invisible button.
   */
  ghidgui->toggle_holdoff = TRUE;
  ghid_route_style_button_set_active (i);
  route_style_index = i;
  ghidgui->toggle_holdoff = FALSE;

  gtk_widget_set_sensitive (route_style_edit_button,
			    (i == NUM_STYLES + 2) ? FALSE : TRUE);
}

void
ghid_route_style_set_button_label (gchar * name, gint index)
{
  if (index < 0 || index >= NUM_STYLES || !route_style_button[index].button)
    return;
  gtk_button_set_label (GTK_BUTTON (route_style_button[index].button),
			_(name));
}

void
ghid_route_style_set_temp_style (RouteStyleType * rst, gint which)
{
  RouteStyleButton *rsb;
  gchar *tmp;
  gint index = which + NUM_STYLES;

  if (which < 0 || which > 1)
    return;
  rsb = &route_style_button[index];
  gtk_widget_show (rsb->button);
  rsb->shown = TRUE;
  tmp = rsb->route_style.Name;
  rsb->route_style = *rst;
  rsb->route_style.Name = tmp;
  if (route_style_index != index)
    {
      route_style_index = index;	/* Sets already done */
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (rsb->button), TRUE);
    }
}



/*
 *  ---------------------------------------------------------------
 * Mode buttons
 */
typedef struct
{
  GtkWidget *button, *box0, *box1;
  gchar *name;
  gint mode;
  gchar **xpm;
}
ModeButton;


static ModeButton mode_buttons[] = {
  {NULL, NULL, NULL, "via", VIA_MODE, via},
  {NULL, NULL, NULL, "line", LINE_MODE, line},
  {NULL, NULL, NULL, "arc", ARC_MODE, arc},
  {NULL, NULL, NULL, "text", TEXT_MODE, text},
  {NULL, NULL, NULL, "rectangle", RECTANGLE_MODE, rect},
  {NULL, NULL, NULL, "polygon", POLYGON_MODE, poly},
  {NULL, NULL, NULL, "buffer", PASTEBUFFER_MODE, buf},
  {NULL, NULL, NULL, "remove", REMOVE_MODE, del},
  {NULL, NULL, NULL, "rotate", ROTATE_MODE, rot},
  {NULL, NULL, NULL, "insertPoint", INSERTPOINT_MODE, ins},
  {NULL, NULL, NULL, "thermal", THERMAL_MODE, thrm},
  {NULL, NULL, NULL, "select", ARROW_MODE, sel},
  {NULL, NULL, NULL, "lock", LOCK_MODE, lock}
};

static gint n_mode_buttons = G_N_ELEMENTS (mode_buttons);


static void
mode_button_toggled_cb (GtkWidget * widget, ModeButton * mb)
{
  gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  if (active)
    {
    SetMode (mb->mode);
    ghid_mode_cursor (mb->mode);
    ghidgui->settings_mode = mb->mode;
	}
}

void
ghid_mode_buttons_update (void)
{
  ModeButton *mb;
  gint i;

  for (i = 0; i < n_mode_buttons; ++i)
    {
      mb = &mode_buttons[i];
      if (Settings.Mode == mb->mode)
	{
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mb->button), TRUE);
	  break;
	}
    }
}

void
ghid_pack_mode_buttons(void)
{
	ModeButton *mb;
	gint	i;
  static gint	last_pack_compact = -1;

  if (last_pack_compact >= 0)
		{
		if (last_pack_compact)
			gtk_container_remove(GTK_CONTAINER(ghidgui->mode_buttons1_vbox),
						ghidgui->mode_buttons1_frame);
		else
			gtk_container_remove(GTK_CONTAINER(ghidgui->mode_buttons0_frame_vbox),
						ghidgui->mode_buttons0_frame);

		for (i = 0; i < n_mode_buttons; ++i)
			{
			mb = &mode_buttons[i];
			if (last_pack_compact)
				gtk_container_remove (GTK_CONTAINER (mb->box1), mb->button);
			else
				gtk_container_remove (GTK_CONTAINER (mb->box0), mb->button);
			}
		}
	for (i = 0; i < n_mode_buttons; ++i)
		{
		mb = &mode_buttons[i];
		if (ghidgui->compact_vertical)
			gtk_box_pack_start (GTK_BOX (mb->box1), mb->button, FALSE, FALSE, 0);
		else
			gtk_box_pack_start (GTK_BOX (mb->box0), mb->button, FALSE, FALSE, 0);
		}
	if (ghidgui->compact_vertical)
		{
		gtk_box_pack_start(GTK_BOX(ghidgui->mode_buttons1_vbox),
				ghidgui->mode_buttons1_frame, FALSE, FALSE, 0);
		gtk_widget_show_all(ghidgui->mode_buttons1_frame);
		}
	else
		{
		gtk_box_pack_start(GTK_BOX(ghidgui->mode_buttons0_frame_vbox),
				ghidgui->mode_buttons0_frame, FALSE, FALSE, 0);
		gtk_widget_show_all(ghidgui->mode_buttons0_frame);
		}
	last_pack_compact = ghidgui->compact_vertical;
}

static void
make_mode_buttons (GHidPort * port)
{
  ModeButton *mb;
  GtkWidget *hbox0 = NULL, *button;
  GtkWidget *image;
  GdkPixbuf *pixbuf;
  GSList *group = NULL;
  gint i;

  for (i = 0; i < n_mode_buttons; ++i)
    {
      mb = &mode_buttons[i];
      button = gtk_radio_button_new (group);
      mb->button = button;
      g_object_ref(G_OBJECT(mb->button));
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);

      if ((i % ghidgui->n_mode_button_columns) == 0)
        {
        hbox0 = gtk_hbox_new (FALSE, 0);
        gtk_box_pack_start (GTK_BOX (ghidgui->mode_buttons0_vbox),
            hbox0, FALSE, FALSE, 0);
        }
      mb->box0 = hbox0;

      mb->box1 = ghidgui->mode_buttons1_hbox;

      pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) mb->xpm);
      image = gtk_image_new_from_pixbuf (pixbuf);
      g_object_unref (G_OBJECT (pixbuf));

      gtk_container_add (GTK_CONTAINER (button), image);
      if (!strcmp (mb->name, "select"))
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
      g_signal_connect (button, "toggled",
			G_CALLBACK (mode_button_toggled_cb), mb);
    }
  ghid_pack_mode_buttons();
}

/*
 * ---------------------------------------------------------------
 * Top window
 * ---------------------------------------------------------------
 */

static GtkWidget *ghid_left_sensitive_box;

static gint
delete_chart_cb (GtkWidget * widget, GdkEvent * event, GHidPort * port)
{
  ghid_config_files_write ();
  hid_action ("Quit");

  /*
   * Return TRUE to keep our app running.  A FALSE here would let the
   * delete signal continue on and kill our program.
   */
  return TRUE;
}

static void
destroy_chart_cb (GtkWidget * widget, GHidPort * port)
{
  gtk_main_quit ();
}



/* 
 * Create the top_window contents.  The config settings should be loaded
 * before this is called.
 */
static void
ghid_build_pcb_top_window (void)
{
  GtkWidget *window;
  GtkWidget *vbox_main, *vbox_left, *hbox_middle, *hbox = NULL;
  GtkWidget *viewport, *ebox, *vbox, *frame;
  GtkWidget *label;
  GHidPort *port = &ghid_port;
  gchar *s;
#if defined(SCROLLED_LAYER_BUTTONS)
  GtkWidget *scrolled;
#endif

  window = gport->top_window;

  vbox_main = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox_main);

  /* -- Top control bar */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox_main), hbox, FALSE, FALSE, 0);
  ghidgui->top_hbox = hbox;

  /*
   * menu_hbox will be made insensitive when the gui needs
   * a modal button GetLocation button press.
   */
  ghidgui->menu_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (ghidgui->top_hbox), ghidgui->menu_hbox,
		      FALSE, FALSE, 0);
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(ghidgui->menu_hbox), vbox, FALSE, FALSE, 0);
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  ghid_load_menus ();
  make_top_menubar(hbox, port);

  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  g_object_ref(G_OBJECT(frame));
  ghidgui->mode_buttons1_vbox = vbox;
  ghidgui->mode_buttons1_frame = frame;
  ghidgui->mode_buttons1_hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), ghidgui->mode_buttons1_hbox);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_end(GTK_BOX(ghidgui->top_hbox), vbox,
		      FALSE, FALSE, 0);
  ghidgui->compact_vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX (ghidgui->top_hbox), ghidgui->compact_vbox,
		      FALSE, FALSE, 0);

  ghidgui->compact_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), ghidgui->compact_hbox, TRUE, FALSE, 0);

  /*
   * The board name is optionally in compact_vbox and the position
   * labels will be packed below or to the side.
   */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (ghidgui->compact_vbox), hbox, TRUE, FALSE, 2);
  ghidgui->label_hbox = hbox;

  label = gtk_label_new ("");
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  if (ghidgui->name_label_string)
    s =
      g_strdup_printf (" <b><big>%s</big></b> ", ghidgui->name_label_string);
  else
    s = g_strdup ("<b><big>%s</big></b>");
  gtk_label_set_markup (GTK_LABEL (label), s);
  g_free (s);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 4);
  ghidgui->name_label = label;

  /*
   * The position_box pack location depends on user setting of
   * compact horizontal mode.
   */
  ghidgui->position_hbox = gtk_hbox_new (FALSE, 0);
  g_object_ref(G_OBJECT(ghidgui->position_hbox));	/* so can remove it */
  if (ghidgui->compact_horizontal)
    {
      gtk_box_pack_end (GTK_BOX (ghidgui->compact_vbox),
		     ghidgui->position_hbox, TRUE, FALSE, 0);
    }
  else
    {
      gtk_box_pack_end(GTK_BOX(ghidgui->compact_hbox), ghidgui->position_hbox,
		      FALSE, FALSE, 4);
    }

  make_cursor_position_labels (ghidgui->position_hbox, port);

  hbox_middle = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox_main), hbox_middle, TRUE, TRUE, 3);


  /* -- Left control bar */
  ebox = gtk_event_box_new ();
  gtk_widget_set_events (ebox, GDK_EXPOSURE_MASK);
  gtk_box_pack_start (GTK_BOX (hbox_middle), ebox, FALSE, FALSE, 3);

  /* 
   * This box will also be made insensitive when the gui needs
   * a modal button GetLocation button press.
   */
  ghid_left_sensitive_box = ebox;

  vbox_left = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (ebox), vbox_left);

#if defined(SCROLLED_LAYER_BUTTONS)
  /* May need to do this when adding layers and vertical height gets large. */
  vbox = ghid_scrolled_vbox(vbox_left, &scrolled,
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
#else
  frame = gtk_frame_new(NULL);
  gtk_box_pack_start(GTK_BOX(vbox_left), frame, FALSE, FALSE, 0);
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), vbox);
#endif
  make_layer_buttons(vbox, port);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox_left), vbox, FALSE, FALSE, 0);
  ghidgui->mode_buttons0_frame_vbox = vbox;
  frame = gtk_frame_new(NULL);
  ghidgui->mode_buttons0_frame = frame;
  gtk_widget_show(frame);
  g_object_ref(G_OBJECT(frame));
  ghidgui->mode_buttons0_vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), ghidgui->mode_buttons0_vbox);
  make_mode_buttons (port);

  frame = gtk_frame_new(NULL);
  gtk_box_pack_end(GTK_BOX(vbox_left), frame, FALSE, FALSE, 0);
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX (vbox), hbox, FALSE, FALSE, 1);
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 4);
  make_route_style_buttons(vbox, port);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_middle), vbox, TRUE, TRUE, 0);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  /* -- The PCB layout output drawing area */
  viewport = gtk_viewport_new (NULL, NULL);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), viewport, TRUE, TRUE, 0);

  gport->drawing_area = gtk_drawing_area_new ();

  gtk_widget_add_events (gport->drawing_area, GDK_EXPOSURE_MASK
			 | GDK_LEAVE_NOTIFY_MASK | GDK_ENTER_NOTIFY_MASK
			 | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK
			 | GDK_KEY_RELEASE_MASK | GDK_KEY_PRESS_MASK
			 | GDK_FOCUS_CHANGE_MASK | GDK_POINTER_MOTION_MASK);

  /*
   * This is required to get the drawing_area key-press-event.  Also the
   * enter and button press callbacks grab focus to be sure we have it
   * when in the drawing_area.
   */
  GTK_WIDGET_SET_FLAGS (gport->drawing_area, GTK_CAN_FOCUS);

  gtk_container_add (GTK_CONTAINER (viewport), gport->drawing_area);

  ghidgui->v_adjustment = gtk_adjustment_new (0.0, 0.0, 100.0,
					      10.0, 10.0, 10.0);
  ghidgui->v_range =
    gtk_vscrollbar_new (GTK_ADJUSTMENT (ghidgui->v_adjustment));

  gtk_range_set_update_policy (GTK_RANGE (ghidgui->v_range),
			       GTK_UPDATE_CONTINUOUS);
  gtk_box_pack_start (GTK_BOX (hbox), ghidgui->v_range, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (ghidgui->v_adjustment), "value_changed",
		    G_CALLBACK (v_adjustment_changed_cb), ghidgui);

  ghidgui->h_adjustment = gtk_adjustment_new (0.0, 0.0, 100.0,
					      10.0, 10.0, 10.0);
  ghidgui->h_range =
    gtk_hscrollbar_new (GTK_ADJUSTMENT (ghidgui->h_adjustment));
  gtk_range_set_update_policy (GTK_RANGE (ghidgui->h_range),
			       GTK_UPDATE_CONTINUOUS);
  gtk_box_pack_start (GTK_BOX (vbox), ghidgui->h_range, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (ghidgui->h_adjustment), "value_changed",
		    G_CALLBACK (h_adjustment_changed_cb), ghidgui);

  /* -- The bottom status line label */
  ghidgui->status_line_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), ghidgui->status_line_hbox,
		      FALSE, FALSE, 2);

  label = gtk_label_new ("");
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (ghidgui->status_line_hbox), label, FALSE,
		      FALSE, 0);
  ghidgui->status_line_label = label;

  /* Depending on user setting, the command_combo_box may get packed into
     |  the status_line_hbox, but it will happen on demand the first time
     |  the user does a command entry.
   */

  g_signal_connect (G_OBJECT (gport->drawing_area), "expose_event",
		    G_CALLBACK (ghid_port_drawing_area_expose_event_cb),
		    port);
  g_signal_connect (G_OBJECT (gport->top_window), "configure_event",
		    G_CALLBACK (top_window_configure_event_cb), port);
  g_signal_connect (G_OBJECT (gport->drawing_area), "configure_event",
		    G_CALLBACK (ghid_port_drawing_area_configure_event_cb),
		    port);


  /* Mouse and key events will need to be intercepted when PCB needs a
     |  location from the user.
   */

  ghid_interface_input_signals_connect ();

  g_signal_connect (G_OBJECT (gport->drawing_area), "scroll_event",
		    G_CALLBACK (ghid_port_window_mouse_scroll_cb), port);
  g_signal_connect (G_OBJECT (gport->drawing_area), "enter_notify_event",
		    G_CALLBACK (ghid_port_window_enter_cb), port);
  g_signal_connect (G_OBJECT (gport->drawing_area), "leave_notify_event",
		    G_CALLBACK (ghid_port_window_leave_cb), port);
  g_signal_connect (G_OBJECT (gport->drawing_area), "motion_notify_event",
		    G_CALLBACK (ghid_port_window_motion_cb), port);



  g_signal_connect (G_OBJECT (window), "delete_event",
		    G_CALLBACK (delete_chart_cb), port);
  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (destroy_chart_cb), port);

  ghidgui->creating = FALSE;

  gtk_widget_show_all (gport->top_window);
  gtk_widget_realize (vbox_main);
  gtk_widget_realize (hbox_middle);
  gtk_widget_realize (viewport);
  gtk_widget_realize (gport->drawing_area);
  gdk_window_set_back_pixmap (gport->drawing_area->window, NULL, FALSE);

  ghid_route_style_temp_buttons_hide ();
  ghid_show_layer_buttons();
}


  /* Connect and disconnect just the signals a g_main_loop() will need.
     |  Cursor and motion events still need to be handled by the top level
     |  loop, so don't connect/reconnect these.
     |  A g_main_loop will be running when PCB wants the user to select a
     |  location or if command entry is needed in the status line hbox.
     |  During these times normal button/key presses are intercepted, either
     |  by new signal handlers or the command_combo_box entry.
   */
static gulong button_press_handler, button_release_handler,
  key_press_handler, key_release_handler;

void
ghid_interface_input_signals_connect (void)
{
  button_press_handler =
    g_signal_connect (G_OBJECT (gport->drawing_area), "button_press_event",
		      G_CALLBACK (ghid_port_button_press_cb),
		      ghidgui->ui_manager);

  button_release_handler =
    g_signal_connect (G_OBJECT (gport->drawing_area), "button_release_event",
		      G_CALLBACK (ghid_port_button_release_cb),
		      ghidgui->ui_manager);

  key_press_handler =
    g_signal_connect (G_OBJECT (gport->drawing_area), "key_press_event",
		      G_CALLBACK (ghid_port_key_press_cb),
		      ghidgui->ui_manager);

  key_release_handler =
    g_signal_connect (G_OBJECT (gport->drawing_area), "key_release_event",
		      G_CALLBACK (ghid_port_key_release_cb),
		      ghidgui->ui_manager);
}

void
ghid_interface_input_signals_disconnect (void)
{
  if (button_press_handler)
    g_signal_handler_disconnect (gport->drawing_area, button_press_handler);

  if (button_release_handler)
    g_signal_handler_disconnect (gport->drawing_area, button_release_handler);

  if (key_press_handler)
    g_signal_handler_disconnect (gport->drawing_area, key_press_handler);

  if (key_release_handler)
    g_signal_handler_disconnect (gport->drawing_area, key_release_handler);

  button_press_handler = button_release_handler = 0;
  key_press_handler = key_release_handler = 0;

}


  /* We'll set the interface insensitive when a g_main_loop is running so the
     |  Gtk menus and buttons don't respond and interfere with the special entry
     |  the user needs to be doing.
   */
void
ghid_interface_set_sensitive (gboolean sensitive)
{
  gtk_widget_set_sensitive (ghid_left_sensitive_box, sensitive);
  gtk_widget_set_sensitive (ghidgui->menu_hbox, sensitive);
}


/* ----------------------------------------------------------------------
 * initializes icon pixmap and also cursor bit maps
 */
static void
ghid_init_icons (GHidPort * port)
{
  XC_clock_source = gdk_bitmap_create_from_data (gport->top_window->window,
						 (char *) rotateIcon_bits,
						 rotateIcon_width,
						 rotateIcon_height);
  XC_clock_mask =
    gdk_bitmap_create_from_data (gport->top_window->window, (char *) rotateMask_bits,
				 rotateMask_width, rotateMask_height);

  XC_hand_source = gdk_bitmap_create_from_data (gport->top_window->window,
						(char *) handIcon_bits,
						handIcon_width,
						handIcon_height);
  XC_hand_mask =
    gdk_bitmap_create_from_data (gport->top_window->window, (char *) handMask_bits,
				 handMask_width, handMask_height);

  XC_lock_source = gdk_bitmap_create_from_data (gport->top_window->window,
						(char *) lockIcon_bits,
						lockIcon_width,
						lockIcon_height);
  XC_lock_mask =
    gdk_bitmap_create_from_data (gport->top_window->window, (char *) lockMask_bits,
				 lockMask_width, lockMask_height);
}

void
ghid_create_pcb_widgets (void)
{
  GHidPort *port = &ghid_port;
  GError	*err = NULL;

  gdk_color_parse ("white", &WhitePixel);
  gdk_color_parse ("black", &BlackPixel);

  if (bg_image_file)
    ghidgui->bg_pixbuf = gdk_pixbuf_new_from_file(bg_image_file, &err);
  if (err)
    {
    g_error(err->message);
    g_error_free(err);
    }
  ghid_build_pcb_top_window ();
  ghid_init_toggle_states ();
  ghid_init_icons (port);
  SetMode (ARROW_MODE);
  ghid_mode_buttons_update ();
}

static gboolean
ghid_listener_cb (GIOChannel *source,
		  GIOCondition condition,
		  gpointer data)
{
  GIOStatus status;
  gchar *str;
  gsize len;
  gsize term;
  GError *err = NULL;


  if (condition & G_IO_HUP)
    {
      gui->log ("Read end of pipe died!\n");
      return FALSE;
    }

  if (condition == G_IO_IN)
    {
      status = g_io_channel_read_line (source, &str, &len, &term, &err);
      switch (status)
	{
	case G_IO_STATUS_NORMAL:
	  hid_parse_actions (str, NULL);
	  g_free (str);
	  break;

	case G_IO_STATUS_ERROR:
	  gui->log ("ERROR status from g_io_channel_read_line\n");
	  return FALSE;
	  break;

	case G_IO_STATUS_EOF:
	  gui->log ("Input pipe returned EOF.  The --listen option is \n"
		    "probably not running anymore in this session.\n");
	  return FALSE;
	  break;

	case G_IO_STATUS_AGAIN:
	  gui->log ("AGAIN status from g_io_channel_read_line\n");
	  return FALSE;
	  break;

	default:
	  fprintf (stderr, "ERROR:  unhandled case in ghid_listener_cb\n");
	  return FALSE;
	  break;
	}

    }
  else
    fprintf (stderr, "Unknown condition in ghid_listener_cb\n");
  
  return TRUE;
}

static void
ghid_create_listener (void)
{
  guint tag;
  GIOChannel *channel;
  int fd = fileno (stdin);

  channel = g_io_channel_unix_new (fd);
  tag = g_io_add_watch (channel, G_IO_IN, ghid_listener_cb, NULL);
}


/* ------------------------------------------------------------ */
static int stdin_listen = 0;
HID_Attribute ghid_attribute_list[] = {
  {"listen", "Listen for actions on stdin",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, &stdin_listen},
#define HA_listen 0

  {"bg-image", "Background Image",
   HID_String, 0, 0, {0, 0, 0}, 0, &bg_image_file},
#define HA_bg_image 1

};

REGISTER_ATTRIBUTES (ghid_attribute_list)

  /* Create top level window for routines that will need top_window
     |  before ghid_create_pcb_widgets() is called.
   */
void
ghid_parse_arguments (int *argc, char ***argv)
{
  GtkWidget *window;
  gint i;
  GdkPixbuf *icon;

  /* on windows we need to figure out the installation directory */
#ifdef WIN32
  char * tmps;
  char * libdir;
  tmps = g_win32_get_package_installation_directory(PACKAGE "-" VERSION, NULL);
#define REST_OF_PATH G_DIR_SEPARATOR_S "share" G_DIR_SEPARATOR_S PACKAGE  G_DIR_SEPARATOR_S "newlib"
  libdir = (char *) malloc(strlen(tmps) +
                          strlen(REST_OF_PATH) +
                          1);
  sprintf(libdir, "%s%s", tmps, REST_OF_PATH);
  free(tmps);
  
  Settings.LibraryTree = libdir;

#undef REST_OF_PATH

#endif 

#if defined (DEBUG)
  for (i = 0; i < *argc; i++)
    {
      printf ("ghid_parse_arguments():  *argv[%d] = \"%s\"\n", i, (*argv)[i]);
    }
#endif

  /* Threads aren't used in PCB, but this call would go here.
   */
  /* g_thread_init (NULL); */

#if defined (ENABLE_NLS)
  /* Do our own setlocale() stufff since we want to override LC_NUMERIC   
   */
  gtk_set_locale ();
  setlocale (LC_NUMERIC, "POSIX");	/* use decimal point instead of comma */
#endif

  /*
   * Prevent gtk_init() and gtk_init_check() from automatically
   * calling setlocale (LC_ALL, "") which would undo LC_NUMERIC if ENABLE_NLS
   * We also don't want locale set if no ENABLE_NLS to keep POSIX LC_NUMERIC.
   */
  gtk_disable_setlocale ();

  gtk_init (argc, argv);

  gport = &ghid_port;
  gport->zoom = 300.0;
  pixel_slop = 300;

  ghid_config_files_read (argc, argv);

  Settings.AutoPlace = 0;
  for (i = 0; i < *argc; i++)
    {
      if (strcmp ((*argv)[i], "-auto-place") == 0)
	Settings.AutoPlace = 1;
    }

#ifdef ENABLE_NLS
#ifdef LOCALEDIR
  bindtextdomain (PACKAGE, LOCALEDIR);
#endif
  textdomain (PACKAGE);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

  icon = gdk_pixbuf_new_from_xpm_data ((const gchar **) icon_bits);
  gtk_window_set_default_icon (icon);

  window = gport->top_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "PCB");
  gtk_window_set_default_size(GTK_WINDOW(window),
			       ghidgui->top_window_width, ghidgui->top_window_height);

  if (Settings.AutoPlace)
    gtk_widget_set_uposition (GTK_WIDGET (window), 10, 10);

  gtk_widget_realize (gport->top_window);
  gtk_widget_show_all (gport->top_window);
  ghidgui->creating = TRUE;
}



void
ghid_destroy_gc (hidGC gc)
{
  if (gc->gc)
    g_object_unref (gc->gc);
  g_free (gc);
}

hidGC
ghid_make_gc (void)
{
  hidGC rv;

  rv = g_new0 (hid_gc_struct, 1);
  rv->me_pointer = &ghid_hid;
  rv->colorname = Settings.BackgroundColor;
  return rv;
}

void
ghid_do_export (HID_Attr_Val * options)
{
  ghid_create_pcb_widgets ();

  if (stdin_listen)
    ghid_create_listener ();

  gtk_main ();
  ghid_config_files_write ();
}

gint
LayersChanged (int argc, char **argv, int px, int py)
{
  ghid_config_groups_changed();
  ghid_layer_buttons_update ();
  ghid_show_layer_buttons();

  /* FIXME - if a layer is moved it should retain its color.  But layers
  |  currently can't do that because color info is not saved in the
  |  pcb file.  So this makes a moved layer change its color to reflect
  |  the way it will be when the pcb is reloaded.
  */
  pcb_colors_from_settings (PCB);
  return 0;
}

HID_Action gtk_topwindow_action_list[] = {
  {"LayersChanged", 0, LayersChanged}
};

REGISTER_ACTIONS (gtk_topwindow_action_list)

static char *pcbmenu_path = "gpcb-menu.res";

static HID_Attribute pcbmenu_attr[] = {
{"pcb-menu", "Location of gpcb-menu.res file",
   HID_String, 0, 0, {0, PCBLIBDIR "/gpcb-menu.res", 0}, 0, &pcbmenu_path}
};

REGISTER_ATTRIBUTES (pcbmenu_attr)

typedef struct ToggleItem
{
  struct ToggleItem *next;
  //Widget w;
  char *group, *item;
  //XtCallbackProc callback;
  void * callback;
  Resource *node;
} ToggleItem;

static ToggleItem *toggle_items = 0;

#define INDENT_INC 5
static int n = 0;

static void
ghid_append_action (const char * name, const char *stock_id, 
		    const char *label, const char *accelerator,
		    const char *tooltip)
{
  if ( (new_entries = realloc (new_entries, 
			       (menuitem_cnt + 1) * sizeof (GtkActionEntry))) == NULL)
    {
      fprintf (stderr, "ghid_append_action():  realloc of new_entries failed\n");
      exit (1);
    }
  

  if ( (action_resources = realloc (action_resources,
				    (menuitem_cnt + 1) * sizeof (Resource *))) == NULL)
    {
      fprintf (stderr, "ghid_append_action():  realloc of action_resources failed\n");
      exit (1);
    }
  action_resources[menuitem_cnt] = NULL;

  /* name, stock_id, label, accelerator, tooltip, callback */
  new_entries[menuitem_cnt].name = strdup (name);
  new_entries[menuitem_cnt].stock_id = (stock_id == NULL ? NULL : strdup (stock_id));
  new_entries[menuitem_cnt].label = strdup (label);
  new_entries[menuitem_cnt].accelerator = (accelerator == NULL ? NULL : strdup (accelerator));
  new_entries[menuitem_cnt].tooltip = (tooltip == NULL ? NULL : strdup (tooltip));
  new_entries[menuitem_cnt].callback = G_CALLBACK (ghid_menu_cb);
  menuitem_cnt++;
}

static void
ghid_append_toggle_action (const char * name, const char *stock_id, 
			   const char *label, const char *accelerator,
			   const char *tooltip, int active)
{

  if ( (new_toggle_entries = realloc (new_toggle_entries, 
				      (tmenuitem_cnt + 1) * sizeof (GtkToggleActionEntry))) == NULL)
    {
      fprintf (stderr, "ghid_append_toggle_action():  realloc of new_toggle_entries failed\n");
      exit (1);
    }
  

  if ( (toggle_action_resources = realloc (toggle_action_resources,
				    (tmenuitem_cnt + 1) * sizeof (Resource *))) == NULL)
    {
      fprintf (stderr, "ghid_append_toggle_action():  realloc of toggle_action_resources failed\n");
      exit (1);
    }
  toggle_action_resources[tmenuitem_cnt] = NULL;

  /* name, stock_id, label, accelerator, tooltip, callback */
  new_toggle_entries[tmenuitem_cnt].name = strdup (name);
  new_toggle_entries[tmenuitem_cnt].stock_id = (stock_id == NULL ? NULL : strdup (stock_id));
  new_toggle_entries[tmenuitem_cnt].label = strdup (label);
  new_toggle_entries[tmenuitem_cnt].accelerator = (accelerator == NULL ? NULL : strdup (accelerator));
  new_toggle_entries[tmenuitem_cnt].tooltip = (tooltip == NULL ? NULL : strdup (tooltip));
  new_toggle_entries[tmenuitem_cnt].callback = G_CALLBACK (ghid_menu_cb);
  new_toggle_entries[tmenuitem_cnt].is_active = active ? TRUE : FALSE;
  tmenuitem_cnt++;
}

static int menu_cnt = 0;

static void
add_resource_to_menu (char * menu, Resource * node, void * callback, int indent)
{
  int i, j;
  char *v;
  Resource *r;
  int n;
  char tmps[32];
  int menuitem_id;

  for (i = 0; i < node->c; i++)
    switch (resource_type (node->v[i]))
      {
      case 101:		/* named subnode */
	add_resource_to_menu (node->v[i].name, node->v[i].subres, 
			      callback, indent + INDENT_INC);
	break;

      case 1:			/* unnamed subres */
	/* This is a menu choice.  The first value in the unnamed
	 * subres is what the menu choice gets called.
	 *
	 * This may be a top level menu on the menubar,
	 * a menu choice under, say the File menu, or
	 * a menu choice under a submenu of a menu choice.
	 *
	 * We need to pick off an "m" named resource which is
	 * the menu accelerator key and an "a" named subresource
	 * which contains the information for the hotkey.
	 */
	if ((v = resource_value (node->v[i].subres, "m")))
	  {
	    printf ("    found resource value m=\"%s\"\n", v);
	  }
	if ((r = resource_subres (node->v[i].subres, "a")))
	  {
	    /* for the accelerator, it has 2 values  like
	     *
	     * a={"Ctrl-Q" "Ctrl<Key>q"}
	     * The first one is what's displayed in the menu and the
	     * second actually defines the hotkey
	     */
	    printf ("    accelerator a=%p.  r->v[0].value = \"%s\", r->v[1].value = \"%s\"\n", r, r->v[0].value, r->v[1].value);
	  }
	v = "button";

	/* Now look for the first unnamed value (not a subresource) to
	 * figure out the name of the menu or the menuitem.
	 *
	 * After this loop, v will be the name of the menu or menuitem.
	 *
	 */
	for (j = 0; j < node->v[i].subres->c; j++)
	  if (resource_type (node->v[i].subres->v[j]) == 10)
	    {
	      v = node->v[i].subres->v[j].value;
	      break;
	    }
	
	    
	/* if the subresource we're processing also has unnamed
	 * subresources then this is either a menu (that goes on the
	 * menu bar) or it is a submenu.  It isn't a menuitem.
	 */
	if (node->v[i].subres->flags & FLAG_S)
	  {
	    /* This is a menu */

	    /* add menus to the same entries list as the "normal"
	     * menuitems.  We'll just use NULL for what happens so the
	     * callback doesn't have anything to do.
	     */

	    sprintf (tmps, "%s%d", MENUITEM, menuitem_cnt);

	    /* add to the action entries */
	    /* name, stock_id, label, accelerator, tooltip */
	    ghid_append_action (tmps, NULL, v, NULL, NULL);

	    /* and add to the user interfact XML description */
	    ghid_ui_info_indent (indent);
	    ghid_ui_info_append ("<menu action='");
	    ghid_ui_info_append (tmps);
	    ghid_ui_info_append ("'>\n");


	    /* recursively add more submenus or menuitems to this
	     * menu/submenu
	     */
	    add_resource_to_menu ("sub menu", node->v[i].subres, 
				  callback, indent + INDENT_INC);
	    ghid_ui_info_indent (indent);

	    /* and close this menu */
	    ghid_ui_info_append ("</menu>\n");
	  }
	else
	  {
	    /* We are in a specific menu choice and need to figure out
	     * if it is a "normal" one 
	     * or if there is some condtion under which it is checked
	     * or if it has sensitive=false which is simply a label 
	     */
	    
	    char *checked = resource_value (node->v[i].subres, "checked");
	    char *label = resource_value (node->v[i].subres, "sensitive");
	    if (checked)
	      {
		/* We have the "checked=" named value for this
		 * menuitem.  Now see if it is
		 *   checked=foo
		 * or
		 *   checked=foo,bar
		 *
		 * where the former is just a binary flag and the
		 * latter is checking a flag against a value
		 */
		printf ("Found a \"checked\" menu choice \"%s\", \"%s\"", v, checked);
		if (strchr (checked, ','))
		  {
		    /* we're comparing a flag against a value */
		  }
		else
		  {
		    /* we're looking at a binary flag */
		    /* name, stock_id, label, accelerator, tooltip, callback, is_active
		     */
		  }

		sprintf (tmps, "%s%d", TMENUITEM, tmenuitem_cnt);

		/* add to the action entries */
		/* name, stock_id, label, accelerator, tooltip, is_active */
		ghid_append_toggle_action (tmps, NULL, v, NULL, NULL, 1);

		ghid_ui_info_indent (indent);
		ghid_ui_info_append ("<menuitem action='");
		ghid_ui_info_append (tmps);
		ghid_ui_info_append ("'/>\n");


		toggle_action_resources[tmenuitem_cnt-1] = node->v[i].subres;
		
	      }
	    else if (label && strcmp (label, "false") == 0)
	      {
		/* we have sensitive=false so just put a label in the
		 * GUI 
		 */
	      }
	    else
	      {
		/*
		 * Here we are finally at the rest of an actual
		 * menuitem.  So, we need to get the subresource
		 * that has all the actions in it (actually, it will
		 * be the entire subresource that defines the
		 * menuitem, the callbacks later will pick out the
		 * actions part.
		 *
		 * We add this resource to an array of action
		 * resources that is used by the main menu callback to
		 * figure out what really needs to be done.
		 */

		sprintf (tmps, "%s%d", MENUITEM, menuitem_cnt);

		/* add to the action entries */
		/* name, stock_id, label, accelerator, tooltip */
		ghid_append_action (tmps, NULL, v, NULL, NULL);

		ghid_ui_info_indent (indent);
		ghid_ui_info_append ("<menuitem action='");
		ghid_ui_info_append (tmps);
		ghid_ui_info_append ("'/>\n");


		action_resources[menuitem_cnt-1] = node->v[i].subres;

#ifdef DEBUG
		/* Print out the actions to help with debugging */
		{
		  int vi;
		  Resource *mynode  = node->v[i].subres;
		 
		  /* Start at the 2nd sub resource because the first
		   * is the text that shows up in the menu.
		   * 
		   * We're looking for the unnamed values since those
		   * are the ones which are actions.
		   */
		  for (vi = 1; vi < mynode->c; vi++)
		    if (resource_type (mynode->v[vi]) == 10)
		      printf("   action value=\"%s\"\n", mynode->v[vi].value);
		}
#endif

		
	      }
	    

	    /* now keep looking over our menuitem to see if there is
	     * any more work.
	     */
	    for (j = 0; j < node->v[i].subres->c; j++)
	      switch (resource_type (node->v[i].subres->v[j]))
		{
		case 110:	/* named value = X resource */
		  {
		    char *n = node->v[i].subres->v[j].name;
		    /* allow fg and bg to be abbreviations for
		     * foreground and background
		     */
		    if (strcmp (n, "fg") == 0)
		      n = "foreground";
		    if (strcmp (n, "bg") == 0)
		      n = "background";

		    /* ignore special named values (m, a, sensitive) */
		    if (strcmp (n, "m") == 0
			|| strcmp (n, "a") == 0
			|| strcmp (n, "sensitive") == 0)
		      break;

		    /* log checked and active special values */
		    if (strcmp (n, "checked") == 0)
		      {
			//printf ("%s is checked\n", node->v[i].subres->v[j].value);
			//note_widget_flag (btn, XmNset,
			//			  node->v[i].subres->v[j].value);
			break;
		      }
		    if (strcmp (n, "active") == 0)
		      {
			//printf ("%s is active\n", node->v[i].subres->v[j].value);
			//note_widget_flag (btn, XmNsensitive,
			//		  node->v[i].subres->v[j].value);
			break;
		      }

		    /* if we got this far it is supposed to be an X
		     * resource.  For now ignore it and warn the user
		     */
		    Message ("The gtk gui currently ignores \"%s\"",
			     node->v[i].subres->v[j].value);
		    Message ("as part of a menuitem resource.\n"
			     "Feel free to provide patches\n");
		       
		  }
		  break;
		}

	  }
	break;

      case 10:			/* unnamed value */
	/* in the resource file we may have something like:
	 *
	 * {File
	 *   {Open OpenAction()}
	 *   {Close CloseAction()}
	 *   -
	 *   {"Some Choice" MyAction()}
	 *   {"Some Other Choice" MyOtherAction()}
	 *   @foo
	 *   {Quit QuitAction()}
	 *  }
	 *
	 * If we get here in the code it is becuase we found the "-"
	 * or the "@foo".  
	 * 
	 */
#ifdef DEBUG
	printf ("resource_type for node #%d is 10 (unnamed value).  value=\"%s\"\n", 
		i, node->v[i].value);
#endif

	if (node->v[i].value[0] == '@')
	  {
	    Message ("GTK GUI currently ignores '@' constructs in the menu\n");
	    Message ("resource file.  In this case what is ignored is\n");
	    Message ("\"%s\"\n", node->v[i].value);

#ifdef DAN_FIXME
	    if (strcmp (node->v[i].value, "@layerview") == 0)
	    {
	      //insert_layerview_buttons (menu);
	      printf ("insert_layerview_buttons\n");
	    }
	    if (strcmp (node->v[i].value, "@layerpick") == 0)
	    {
	      //insert_layerpick_buttons (menu);
	      printf ("insert_layerpick_buttons\n");
	    }
	    if (strcmp (node->v[i].value, "@routestyles") == 0)
	    {
	      //lesstif_insert_style_buttons (menu);
	      printf ("insert_style_buttons\n");
	    }
#endif
	  }

	else if (strcmp (node->v[i].value, "-") == 0)
	  {
	    ghid_ui_info_indent (indent);
	    ghid_ui_info_append ("<separator/>\n");
	  }
	else if (i > 0)
	  {
	    /* how do I get here??? */
	    printf ("Create a pushbutton.  menu=\"%s\", value = \"%s\"\n",
		    menu, node->v[i].value);
	   // btn = XmCreatePushButton (menu, node->v[i].value, args, n);
	    //XtManageChild (btn);
	  }
	break;
      }
}


static void
ghid_ui_info_indent (int indent)
{
  int i;

  for (i = 0; i < indent ; i++)
    {
      ghid_ui_info_append (" ");
    }
}

/* 
 *appends a string to the ui_info string 
 * This function is used 
 */

static void
ghid_ui_info_append (const gchar * new)
{
  gchar *p;

  if (new_ui_info_sz == 0) 
    {
      new_ui_info_sz = 1024;
      new_ui_info = (gchar *) calloc ( new_ui_info_sz, sizeof (gchar));
    }

  while (strlen (new_ui_info) + strlen (new) + 1 > new_ui_info_sz)
    {
      size_t n;
      gchar * np;

      n = new_ui_info_sz + 1024;
      printf ("Adding more memory to new_ui_info.  (n = %ld)\n", n);
      if ( (np = realloc (new_ui_info, n)) == NULL)
	{
	  fprintf (stderr, "ghid_ui_info_append():  realloc of size %ld failed\n",
		   n);
	  exit (1);
	}
      new_ui_info = np;
      new_ui_info_sz = n;
    }

  p = new_ui_info + strlen (new_ui_info) ;
  while (*new != '\0')
    {
      *p = *new;
      p++;
      new++;
    }
  
  *p = '\0';
}


static void
ghid_load_menus (void)
{
  char *filename;
  Resource *r = 0, *bir;
  char *home_pcbmenu;
  int screen;
  Resource *mr;

  home_pcbmenu = Concat (getenv ("HOME"), "/.pcb/gpcb-menu.res", NULL);

  if (access ("gpcb-menu.res", R_OK) == 0)
    filename = "gpcb-menu.res";
  else if (access (home_pcbmenu, R_OK) == 0)
    filename = home_pcbmenu;
  else if (access (pcbmenu_path, R_OK) == 0)
    filename = pcbmenu_path;
  else
    filename = 0;

  bir = resource_parse (0, gpcb_menu_default);
  if (!bir)
    {
      fprintf (stderr, "Error: internal menu resource didn't parse\n");
      exit(1);
    }

  if (filename)
    r = resource_parse (filename, 0);

  if (!r)
    r = bir;

  mr = resource_subres (r, "MainMenu");
  if (!mr)
    mr = resource_subres (bir, "MainMenu");
    
  if (mr)
    {
      printf ("ghid_load_menus():  Adding MainMenu\n");
      ghid_ui_info_append ("<ui>\n");
      ghid_ui_info_indent (INDENT_INC);
      ghid_ui_info_append ("<menubar name='MenuBar'>\n");
      add_resource_to_menu ("Initial Call", mr, 0, 2*INDENT_INC);
      ghid_ui_info_indent (INDENT_INC);
      ghid_ui_info_append ("</menubar>\n");
      ghid_ui_info_append ("</ui>\n");
      printf ("Finished loading menus.  ui_info = \n");
      printf ("%s\n", new_ui_info);
    }

  mr = resource_subres (r, "Mouse");
  if (!mr)
    mr = resource_subres (bir, "Mouse");
  if (mr)
    printf ("ghid_load_menus():  Adding Mouse\n");

}
