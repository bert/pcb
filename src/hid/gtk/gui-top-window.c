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

/* #define DEBUG_MENUS */

#ifdef DAN_FIXME
TODO:

- figure out when we need to call this:
  ghid_set_status_line_label ();
  Everytime? 

- the old quit callback had:

  ghid_config_files_write ();
  hid_action ("Quit");

- what about stuff like this:

  /* Set to ! because ActionDisplay toggles it */
  Settings.DrawGrid = !gtk_toggle_action_get_active (action);
  ghidgui->config_modified = TRUE;
  hid_actionl ("Display", "Grid", "", NULL);
  ghid_set_status_line_label ();


I NEED TO DO THE STATUS LINE THING.  for example shift-alt-v to change the
via size.  NOte the status line label does not get updated properly until
a zoom in/out.

- do not forget I can use
  if (!ghidgui->toggle_holdoff)
  
#endif

/* This file was originally written by Bill Wilson for the PCB Gtk
 * port.  It was later heavily modified by Dan McMahill to provide
 * user customized menus.
*/


/* gui-top-window.c
|  This handles creation of the top level window and all its widgets.
|  events for the Output.drawing_area widget are handled in a separate
|  file gui-output-events.c
|
|  Some caveats with menu shorcut keys:  Some keys are trapped out by Gtk
|  and can't be used as shortcuts (eg. '|', TAB, etc).  For these cases
|  we have our own shortcut table and capture the keys and send the events
|  there in ghid_port_key_press_cb().
*/


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include "ghid-layer-selector.h"
#include "gtkhid.h"
#include "gui.h"
#include "hid.h"
#include "../hidint.h"
#include "hid/common/hid_resource.h"

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
#include "gpcb-menu.h"
#include "insert.h"
#include "line.h"
#include "mymem.h"
#include "misc.h"
#include "move.h"
#include "pcb-printf.h"
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
#include "free_atexit.h"

#include "gui-icons-mode-buttons.data"
#include "gui-icons-misc.data"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

/* ---------------------------------------------------------------------------
 * local types
 */

/* Used by the route style buttons and menu */
typedef struct
{
  GtkWidget *button;
  RouteStyleType route_style;
  gboolean shown;		/* For temp buttons */
}
RouteStyleButton;

/* ---------------------------------------------------------------------------
 * local macros
 */

/* ---------------------------------------------------------------------------
 * local prototypes
 */


#define N_ROUTE_STYLES (NUM_STYLES + 3)

static bool ignore_layer_update;

static GtkWidget *ghid_load_menus (void);

/* actions for the @routestyles menuitems */
static GtkToggleActionEntry routestyle_toggle_entries[N_ROUTE_STYLES];
static Resource *routestyle_resources[N_ROUTE_STYLES];
#define ROUTESTYLE "RouteStyle"

GhidGui _ghidgui, *ghidgui = NULL;

GHidPort ghid_port, *gport;

static gchar *bg_image_file;

static struct { GtkAction *action; const Resource *node; }
  ghid_hotkey_actions[256];
#define N_HOTKEY_ACTIONS \
        (sizeof (ghid_hotkey_actions) / sizeof (ghid_hotkey_actions[0]))


/* ------------------------------------------------------------------
 *  Route style buttons
 */

/* Make 3 extra route style radio buttons.  2 for the extra Temp route
 * styles, and the 3rd is an always invisible button selected when the
 * route style settings in use don't match any defined route style (the
 * user can hit 'l', 'v', etc keys to change the settings without selecting
 * a new defined style.
 */

static RouteStyleButton route_style_button[N_ROUTE_STYLES];
static gint route_style_index;

static GtkWidget *route_style_edit_button;

/*! \brief callback for ghid_main_menu_update_toggle_state () */
void
menu_toggle_update_cb (GtkAction *act, const char *tflag, const char *aflag)
{
  if (tflag != NULL)
    {
      int v = hid_get_flag (tflag);
      gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (act), !!v);
    }
  if (aflag != NULL)
    {
      int v = hid_get_flag (aflag);
      gtk_action_set_sensitive (act, !!v);
    }
}

/*! \brief sync the menu checkboxes with actual pcb state */
void
ghid_update_toggle_flags ()
{
  int i;

  GtkAction *a;
  gboolean active;
  gboolean old_holdoff;
  char tmpnm[40];

  /* mask the callbacks */
  old_holdoff = ghidgui->toggle_holdoff;
  ghidgui->toggle_holdoff = TRUE;

  ghid_main_menu_update_toggle_state (GHID_MAIN_MENU (ghidgui->menu_bar),
                                      menu_toggle_update_cb);

  for (i = 0; i < N_ROUTE_STYLES; i++)
    {
      sprintf (tmpnm, "%s%d", ROUTESTYLE, i);
      a = gtk_action_group_get_action (ghidgui->main_actions, tmpnm);
      if (i >= NUM_STYLES)
	{
	  gtk_action_set_visible (a, FALSE);
	}

      /* Update the toggle states */
      active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (route_style_button[i].button));
#ifdef DEBUG_MENUS
      printf ("ghid_update_toggle_flags():  route style %d, value is %d\n", i, active);
#endif
      gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (a), active);
	
    }

  ghidgui->toggle_holdoff = old_holdoff;
}

static void
h_adjustment_changed_cb (GtkAdjustment * adj, GhidGui * g)
{
  if (g->adjustment_changed_holdoff)
    return;

  ghid_port_ranges_changed ();
}

static void
v_adjustment_changed_cb (GtkAdjustment * adj, GhidGui * g)
{
  if (g->adjustment_changed_holdoff)
    return;

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


/*! \brief Menu action callback function
 *  \par Function Description
 *  This is the main menu callback function.  The callback receives
 *  the original Resource pointer containing the HID actions to be
 *  executed.
 *
 *  All hotkeys go through the menus which means they go through here.
 *  Some, such as tab, are caught by Gtk instead of passed here, so
 *  pcb calls this function directly through ghid_hotkey_cb() for them.
 *
 *  \param [in]   The action that was activated
 *  \param [in]   The menu resource associated with the action
 */

static void
ghid_menu_cb (GtkAction *action, const Resource *node)
{
  const gchar *name = gtk_action_get_name (action);

  if (action == NULL || node == NULL) 
    return;

  /* Prevent recursion */
  if (ghidgui->toggle_holdoff == TRUE) 
    return;

#ifdef DEBUG_MENUS
  printf ("ghid_menu_cb():  name = \"%s\"\n", name);
#endif

  /* Special-case route styles */
  if (strncmp (name, ROUTESTYLE, strlen (ROUTESTYLE)) == 0)
    {
      int id = atoi (name + strlen (ROUTESTYLE));
      if (ghidgui->toggle_holdoff != TRUE) 
	ghid_route_style_button_set_active (id);
      node = NULL;
    }
  else
    {
      int vi;
      for (vi = 1; vi < node->c; vi++)
	if (resource_type (node->v[vi]) == 10)
	  {
#ifdef DEBUG_MENUS
	    printf ("    %s\n", node->v[vi].value);
#endif
	    hid_parse_actions (node->v[vi].value);
	  }
    }

  /* Sync gui widgets with pcb state */
  ghid_update_toggle_flags ();
  ghid_mode_buttons_update ();

  /* Sync gui status display with pcb state */
  AdjustAttachedObjects ();
  ghid_invalidate_all ();
  ghid_window_set_name_label (PCB->Name);
  ghid_set_status_line_label ();
}

/* \brief Accelerator callback for accelerators gtk tries to hide from us */
void ghid_hotkey_cb (int which)
{
  if (ghid_hotkey_actions[which].action != NULL)
    ghid_menu_cb (ghid_hotkey_actions[which].action,
                  (gpointer) ghid_hotkey_actions[which].node);
}

  /* Sync toggle states that were saved with the layout and notify the
     |  config code to update Settings values it manages.
   */
void
ghid_sync_with_new_layout (void)
{
  gboolean old_holdoff;

  /* Just want to update the state of the menus without calling the
     |  action functions at this time because causing a toggle action can
     |  undo the initial condition set we want here.
   */
  old_holdoff = ghidgui->toggle_holdoff;
  ghidgui->toggle_holdoff = TRUE;

  /* FIXME - need toggle_holdoff?  Need other calls to sync here? */

  ghidgui->toggle_holdoff = old_holdoff;

  pcb_use_route_style (&PCB->RouteStyle[0]);

  ghid_route_style_button_set_active (0);
  ghid_config_handle_units_changed ();

  ghid_window_set_name_label (PCB->Name);
  ghid_set_status_line_label ();
}

/* ---------------------------------------------------------------------------
 *
 * layer_process()
 *
 * Takes the index into the layers and produces the text string for
 * the layer and if the layer is currently visible or not.  This is
 * used by a couple of functions.
 *
 */
static void
layer_process (gchar **color_string, char **text, int *set, int i)
{
  int tmp;
  char *tmps;
  gchar *tmpc;

  /* cheap hack to let users pass in NULL for either text or set if
   * they don't care about the result
   */
   
  if (color_string == NULL)
    color_string = &tmpc;

  if (text == NULL)
    text = &tmps;

  if (set == NULL)
    set = &tmp;
  
  switch (i)
    {
    case LAYER_BUTTON_SILK:
      *color_string = Settings.ElementColor;
      *text = _( "silk");
      *set = PCB->ElementOn;
      break;
    case LAYER_BUTTON_RATS:
      *color_string = Settings.RatColor;
      *text = _( "rat lines");
      *set = PCB->RatOn;
      break;
    case LAYER_BUTTON_PINS:
      *color_string = Settings.PinColor;
      *text = _( "pins/pads");
      *set = PCB->PinOn;
      break;
    case LAYER_BUTTON_VIAS:
      *color_string = Settings.ViaColor;
      *text = _( "vias");
      *set = PCB->ViaOn;
      break;
    case LAYER_BUTTON_FARSIDE:
      *color_string = Settings.InvisibleObjectsColor;
      *text = _( "far side");
      *set = PCB->InvisibleObjectsOn;
      break;
    case LAYER_BUTTON_MASK:
      *color_string = Settings.MaskColor;
      *text = _( "solder mask");
      *set = TEST_FLAG (SHOWMASKFLAG, PCB);
      break;
    default:		/* layers */
      *color_string = Settings.LayerColor[i];
      *text = (char *)UNKNOWN (PCB->Data->Layer[i].Name);
      *set = PCB->Data->Layer[i].On;
      break;
    }
}

/*! \brief Callback for GHidLayerSelector layer selection */
static void
layer_selector_select_callback (GHidLayerSelector *ls, int layer, gpointer d)
{
  ignore_layer_update = true;
  /* Select Layer */
  PCB->SilkActive = (layer == LAYER_BUTTON_SILK);
  PCB->RatDraw  = (layer == LAYER_BUTTON_RATS);
  if (layer < max_copper_layer)
    ChangeGroupVisibility (layer, TRUE, true);

  ignore_layer_update = false;

  ghid_invalidate_all ();
}

/*! \brief Callback for GHidLayerSelector layer toggling */
static void
layer_selector_toggle_callback (GHidLayerSelector *ls, int layer, gpointer d)
{
  gboolean redraw = FALSE;
  gboolean active;
  layer_process (NULL, NULL, &active, layer);

  active = !active;
  ignore_layer_update = true;
  switch (layer)
    {
    case LAYER_BUTTON_SILK:
      PCB->ElementOn = active;
      PCB->Data->SILKLAYER.On = PCB->ElementOn;
      PCB->Data->BACKSILKLAYER.On = PCB->ElementOn;
      redraw = 1;
      break;
    case LAYER_BUTTON_RATS:
      PCB->RatOn = active;
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
      /* Flip the visibility */
      ChangeGroupVisibility (layer, active, false);
      redraw = TRUE;
      break;
    }

  /* Jump through hoops in case we just disabled the active layer
   *  (or its group). In this case, select a different one if we
   *  can. If we can't, turn the original layer back on.
   */
  if (!ghid_layer_selector_select_next_visible (ls))
    ChangeGroupVisibility (layer, true, false);

  ignore_layer_update = false;

  if (redraw)
    ghid_invalidate_all();
}

/*
 * The intial loading of all actions at startup.
 */
static void
ghid_make_programmed_menu_actions ()
{
  int i;
  Resource *ar;
  char av[64];

  for (i = 0; i < N_ROUTE_STYLES; i++)
    {
      routestyle_toggle_entries[i].name = g_strdup_printf ("%s%d", ROUTESTYLE, i);
      routestyle_toggle_entries[i].stock_id = NULL;
      if (i < NUM_STYLES && PCB)
	{
	  routestyle_toggle_entries[i].label = g_strdup ( (PCB->RouteStyle)[i].Name);
	}
      else
	{
	  routestyle_toggle_entries[i].label = g_strdup (routestyle_toggle_entries[i].name);
	}
      routestyle_toggle_entries[i].accelerator = NULL;
      routestyle_toggle_entries[i].tooltip = NULL;
      routestyle_toggle_entries[i].callback = G_CALLBACK (ghid_menu_cb);
      routestyle_toggle_entries[i].is_active = FALSE;

      ar = resource_create (0);
      sprintf (av, "RouteStyle(%d)", i + 1);
      resource_add_val (ar, 0, strdup (av), 0);
      resource_add_val (ar, 0, strdup (av), 0);
      ar->flags |= FLAG_V;
      routestyle_resources[i] = ar;

      // FIXME
      //sprintf (av, "current_style,%d", i + 1);
      //note_toggle_flag (routestyle_toggle_entries[i].name, strdup (av));

    }
}

static void
make_menu_actions (GtkActionGroup * actions, GHidPort * port)
{
  ghid_make_programmed_menu_actions ();

  gtk_action_group_add_toggle_actions (actions,
				       routestyle_toggle_entries,
				       N_ROUTE_STYLES, port);

}


/*
 * Load in actions for the menus and layer selector
 */
static void
make_actions (GHidPort * port)
{
  GtkActionGroup *actions;

  actions = gtk_action_group_new ("Actions");
  gtk_action_group_set_translation_domain (actions, NULL);
  ghidgui->main_actions = actions;

  make_menu_actions (actions, port);
 
  gtk_window_add_accel_group (GTK_WINDOW (port->top_window),
			      ghid_main_menu_get_accel_group
                                (GHID_MAIN_MENU (ghidgui->menu_bar)));
  gtk_window_add_accel_group (GTK_WINDOW (port->top_window),
			      ghid_layer_selector_get_accel_group
                                (GHID_LAYER_SELECTOR (ghidgui->layer_selector)));
}


/* Refreshes the window title bar and sets the PCB name to the
 * window title bar or to a seperate label
 */
void
ghid_window_set_name_label (gchar * name)
{
  gchar *str;
  gchar *filename;

  /* FIXME -- should this happen?  It does... */
  /* This happens if we're calling an exporter from the command line */
  if (ghidgui == NULL)
    return;

  dup_string (&(ghidgui->name_label_string), name);
  if (!ghidgui->name_label_string || !*ghidgui->name_label_string)
    ghidgui->name_label_string = g_strdup (_("Unnamed"));

  if (!PCB->Filename  || !*PCB->Filename)
    filename = g_strdup(_("Unsaved.pcb"));
  else
    filename = g_strdup(PCB->Filename);

  str = g_strdup_printf ("%s%s (%s) - PCB", PCB->Changed ? "*": "",
                         ghidgui->name_label_string, filename);
  gtk_window_set_title (GTK_WINDOW (gport->top_window), str);
  g_free (str);
  g_free (filename);
}

static void
grid_units_button_cb (GtkWidget * widget, gpointer data)
{
  /* Button only toggles between mm and mil */
  if (Settings.grid_unit == get_unit_struct ("mm"))
    hid_actionl ("SetUnits", "mil", NULL);
  else
    hid_actionl ("SetUnits", "mm", NULL);
}

/*
 * The two following callbacks are used to keep the absolute
 * and relative cursor labels from growing and shrinking as you
 * move the cursor around.
 */
static void
absolute_label_size_req_cb (GtkWidget * widget, 
			    GtkRequisition *req, gpointer data)
{
  
  static gint w = 0;
  if (req->width > w)
    w = req->width;
  else
    req->width = w;
}

static void
relative_label_size_req_cb (GtkWidget * widget, 
			    GtkRequisition *req, gpointer data)
{
  
  static gint w = 0;
  if (req->width > w)
    w = req->width;
  else
    req->width = w;
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
			Settings.grid_unit->in_suffix);
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
  g_signal_connect (G_OBJECT (label), "size-request",
		    G_CALLBACK (absolute_label_size_req_cb), NULL);


  /* The relative cursor position label
   */
  frame = gtk_frame_new (NULL);
  gtk_box_pack_end (GTK_BOX (hbox), frame, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (frame), 2);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);
  label = gtk_label_new (" __.__  __.__ ");
  gtk_container_add (GTK_CONTAINER (frame), label);
  ghidgui->cursor_position_relative_label = label;
  g_signal_connect (G_OBJECT (label), "size-request",
		    G_CALLBACK (relative_label_size_req_cb), NULL);

}

/* \brief Add "virtual layers" to a layer selector */
static void
make_virtual_layer_buttons (GtkWidget *layer_selector)
{
  GHidLayerSelector *layersel = GHID_LAYER_SELECTOR (layer_selector);
  gchar *text;
  gchar *color_string;
  gboolean active;
 
  layer_process (&color_string, &text, &active, LAYER_BUTTON_SILK);
  ghid_layer_selector_add_layer (layersel, LAYER_BUTTON_SILK,
                                 text, color_string, active, TRUE);
  layer_process (&color_string, &text, &active, LAYER_BUTTON_RATS);
  ghid_layer_selector_add_layer (layersel, LAYER_BUTTON_RATS,
                                 text, color_string, active, TRUE);
  layer_process (&color_string, &text, &active, LAYER_BUTTON_PINS);
  ghid_layer_selector_add_layer (layersel, LAYER_BUTTON_PINS,
                                 text, color_string, active, FALSE);
  layer_process (&color_string, &text, &active, LAYER_BUTTON_VIAS);
  ghid_layer_selector_add_layer (layersel, LAYER_BUTTON_VIAS,
                                 text, color_string, active, FALSE);
  layer_process (&color_string, &text, &active, LAYER_BUTTON_FARSIDE);
  ghid_layer_selector_add_layer (layersel, LAYER_BUTTON_FARSIDE,
                                 text, color_string, active, FALSE);
  layer_process (&color_string, &text, &active, LAYER_BUTTON_MASK);
  ghid_layer_selector_add_layer (layersel, LAYER_BUTTON_MASK,
                                 text, color_string, active, FALSE);
}

/*! \brief callback for ghid_layer_selector_update_colors */
const gchar *
get_layer_color (gint layer)
{
  gchar *rv;
  layer_process (&rv, NULL, NULL, layer);
  return rv;
}

/*! \brief Update a layer selector's color scheme */
void
ghid_layer_buttons_color_update (void)
{
  ghid_layer_selector_update_colors
    (GHID_LAYER_SELECTOR (ghidgui->layer_selector), get_layer_color);
  pcb_colors_from_settings (PCB);
}
 
/*! \brief Populate a layer selector with all layers Gtk is aware of */
static void
make_layer_buttons (GtkWidget *layersel)
{
  gint i;
  gchar *text;
  gchar *color_string;
  gboolean active = TRUE;

  for (i = 0; i < max_copper_layer; ++i)
    {
      layer_process (&color_string, &text, &active, i);
      ghid_layer_selector_add_layer (GHID_LAYER_SELECTOR (layersel), i,
                                     text, color_string, active, TRUE);
    }
}


/*! \brief callback for ghid_layer_selector_delete_layers */
gboolean
get_layer_delete (gint layer)
{
  return layer >= max_copper_layer;
}

/*! \brief Synchronize layer selector widget with current PCB state
 *  \par Function Description
 *  Called when user toggles layer visibility or changes drawing layer,
 *  or when layer visibility is changed programatically.
 */
void
ghid_layer_buttons_update (void)
{
  gint layer;

  if (ignore_layer_update)
    return;
 
  ghid_layer_selector_delete_layers
    (GHID_LAYER_SELECTOR (ghidgui->layer_selector),
     get_layer_delete);
  make_layer_buttons (ghidgui->layer_selector);
  make_virtual_layer_buttons (ghidgui->layer_selector);
  ghid_main_menu_install_layer_selector
      (GHID_MAIN_MENU (ghidgui->menu_bar),
       GHID_LAYER_SELECTOR (ghidgui->layer_selector));

  /* Sync selected layer with PCB's state */
  if (PCB->RatDraw)
    layer = LAYER_BUTTON_RATS;
  else if (PCB->SilkActive)
    layer = LAYER_BUTTON_SILK;
  else
    layer = LayerStack[0];

  ghid_layer_selector_select_layer
    (GHID_LAYER_SELECTOR (ghidgui->layer_selector), layer);
}


static void
route_style_edit_cb (GtkWidget * widget, GHidPort * port)
{
  hid_action("AdjustStyle");
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

  for (i = 0; i < N_ROUTE_STYLES; ++i)
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
  if (n < 0 || n >= N_ROUTE_STYLES)
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
  {NULL, NULL, NULL, "polygonhole", POLYGONHOLE_MODE, polyhole},
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
  ghid_shutdown_renderer (port);
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
  GtkWidget *vbox_main, *hbox_middle, *hbox;
  GtkWidget *vbox, *frame;
  GtkWidget *label;
  GHidPort *port = &ghid_port;
  GtkWidget *scrolled;

  window = gport->top_window;

  vbox_main = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox_main);

  /* -- Top control bar */
  ghidgui->top_hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox_main), ghidgui->top_hbox, FALSE, FALSE, 0);

  /*
   * menu_hbox will be made insensitive when the gui needs
   * a modal button GetLocation button press.
   */
  ghidgui->menu_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (ghidgui->top_hbox), ghidgui->menu_hbox,
		      FALSE, FALSE, 0);

  ghidgui->mode_buttons1_vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (ghidgui->menu_hbox),
                      ghidgui->mode_buttons1_vbox, FALSE, FALSE, 0);

  /* Build layer menus */
  ghidgui->layer_selector = ghid_layer_selector_new ();
  make_layer_buttons (ghidgui->layer_selector);
  make_virtual_layer_buttons (ghidgui->layer_selector);
  g_signal_connect (G_OBJECT (ghidgui->layer_selector), "select_layer",
                    G_CALLBACK (layer_selector_select_callback),
                    NULL);
  g_signal_connect (G_OBJECT (ghidgui->layer_selector), "toggle_layer",
                    G_CALLBACK (layer_selector_toggle_callback),
                    NULL);
  /* Build main menu */
  ghidgui->menu_bar = ghid_load_menus ();
  make_actions (port);
  gtk_box_pack_start (GTK_BOX (ghidgui->mode_buttons1_vbox),
                      ghidgui->menu_bar, FALSE, FALSE, 0);

  ghidgui->mode_buttons1_frame = gtk_frame_new (NULL);
  gtk_widget_show (ghidgui->mode_buttons1_frame);
  g_object_ref (ghidgui->mode_buttons1_frame);
  ghidgui->mode_buttons1_hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (ghidgui->mode_buttons1_frame),
                     ghidgui->mode_buttons1_hbox);

  ghidgui->compact_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX(ghidgui->top_hbox),
                    ghidgui->compact_hbox, FALSE, FALSE, 0);

  ghidgui->compact_vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX (ghidgui->top_hbox), ghidgui->compact_vbox,
                    FALSE, FALSE, 0);

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
  /*
   * This box will be made insensitive when the gui needs
   * a modal button GetLocation button press.
   */
  ghidgui->left_toolbar = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_middle),
                      ghidgui->left_toolbar, FALSE, FALSE, 3);

  vbox = ghid_scrolled_vbox (ghidgui->left_toolbar, &scrolled,
                             GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX(vbox), ghidgui->layer_selector,
                      FALSE, FALSE, 0);

  ghidgui->mode_buttons0_frame_vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX(ghidgui->left_toolbar),
                      ghidgui->mode_buttons0_frame_vbox, FALSE, FALSE, 0);

  ghidgui->mode_buttons0_frame = gtk_frame_new (NULL);
  gtk_widget_show (ghidgui->mode_buttons0_frame);
  g_object_ref (ghidgui->mode_buttons0_frame);
  ghidgui->mode_buttons0_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (ghidgui->mode_buttons0_frame),
                     ghidgui->mode_buttons0_vbox);
  make_mode_buttons (port);

  frame = gtk_frame_new(NULL);
  gtk_box_pack_end (GTK_BOX (ghidgui->left_toolbar), frame, FALSE, FALSE, 0);
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

  gport->drawing_area = gtk_drawing_area_new ();
  ghid_init_drawing_widget (gport->drawing_area, gport);

  gtk_widget_add_events (gport->drawing_area, GDK_EXPOSURE_MASK
			 | GDK_LEAVE_NOTIFY_MASK | GDK_ENTER_NOTIFY_MASK
			 | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK
			 | GDK_KEY_RELEASE_MASK | GDK_KEY_PRESS_MASK
			 | GDK_FOCUS_CHANGE_MASK | GDK_POINTER_MOTION_MASK
			 | GDK_POINTER_MOTION_HINT_MASK);

  /*
   * This is required to get the drawing_area key-press-event.  Also the
   * enter and button press callbacks grab focus to be sure we have it
   * when in the drawing_area.
   */
  GTK_WIDGET_SET_FLAGS (gport->drawing_area, GTK_CAN_FOCUS);

  gtk_box_pack_start (GTK_BOX (hbox), gport->drawing_area, TRUE, TRUE, 0);

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

  g_signal_connect (G_OBJECT (gport->drawing_area), "realize",
		    G_CALLBACK (ghid_port_drawing_realize_cb),
		    port);
  g_signal_connect (G_OBJECT (gport->drawing_area), "expose_event",
		    G_CALLBACK (ghid_drawing_area_expose_cb),
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
  gdk_window_set_back_pixmap (gport->drawing_area->window, NULL, FALSE);

  ghid_route_style_temp_buttons_hide ();
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
		      G_CALLBACK (ghid_port_button_press_cb), NULL);

  button_release_handler =
    g_signal_connect (G_OBJECT (gport->drawing_area), "button_release_event",
		      G_CALLBACK (ghid_port_button_release_cb), NULL);

  key_press_handler =
    g_signal_connect (G_OBJECT (gport->drawing_area), "key_press_event",
		      G_CALLBACK (ghid_port_key_press_cb), NULL);

  key_release_handler =
    g_signal_connect (G_OBJECT (gport->drawing_area), "key_release_event",
		      G_CALLBACK (ghid_port_key_release_cb), NULL);
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
  gtk_widget_set_sensitive (ghidgui->left_toolbar, sensitive);
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

  if (bg_image_file)
    ghidgui->bg_pixbuf = gdk_pixbuf_new_from_file(bg_image_file, &err);
  if (err)
    {
    g_error("%s", err->message);
    g_error_free(err);
    }
  ghid_build_pcb_top_window ();
  ghid_update_toggle_flags ();

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
	  hid_parse_actions (str);
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
  GIOChannel *channel;
  int fd = fileno (stdin);

  channel = g_io_channel_unix_new (fd);
  g_io_add_watch (channel, G_IO_IN, ghid_listener_cb, NULL);
}


/* ------------------------------------------------------------ */

static int stdin_listen = 0;
static char *pcbmenu_path = "gpcb-menu.res";

HID_Attribute ghid_attribute_list[] = {

/* %start-doc options "21 GTK+ GUI Options"
@ftable @code
@item --listen
Listen for actions on stdin.
@end ftable
%end-doc
*/
  {"listen", "Listen for actions on stdin",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, &stdin_listen},
#define HA_listen 0

/* %start-doc options "21 GTK+ GUI Options"
@ftable @code
@item --bg-image <string>
File name of an image to put into the background of the GUI canvas. The image must
be a color PPM image, in binary (not ASCII) format. It can be any size, and will be
automatically scaled to fit the canvas.
@end ftable
%end-doc
*/
  {"bg-image", "Background Image",
   HID_String, 0, 0, {0, 0, 0}, 0, &bg_image_file},
#define HA_bg_image 1

/* %start-doc options "21 GTK+ GUI Options"
@ftable @code
@item --pcb-menu <string>
Location of the @file{gpcb-menu.res} file which defines the menu for the GTK+ GUI.
@end ftable
%end-doc
*/
{"pcb-menu", "Location of gpcb-menu.res file",
   HID_String, 0, 0, {0, PCBLIBDIR "/gpcb-menu.res", 0}, 0, &pcbmenu_path}
#define HA_pcbmenu 2
};

REGISTER_ATTRIBUTES (ghid_attribute_list)

HID_Attribute *
ghid_get_export_options (int *n_ret)
{
  *n_ret = sizeof (ghid_attribute_list) / sizeof (HID_Attribute);
  return ghid_attribute_list;
}

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
  gport->view.coord_per_px = 300.0;
  pixel_slop = 300;

  ghid_init_renderer (argc, argv, gport);

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

  gtk_widget_show_all (gport->top_window);
  ghidgui->creating = TRUE;
}

void
ghid_do_export (HID_Attr_Val * options)
{
  ghid_create_pcb_widgets ();

  /* These are needed to make sure the @layerpick and @layerview menus
   * are properly initialized and synchronized with the current PCB.
   */
  ghid_layer_buttons_update ();

  if (stdin_listen)
    ghid_create_listener ();

  ghid_notify_gui_is_up ();

  gtk_main ();
  ghid_config_files_write ();

}

/*! \brief callback for */
static gboolean
get_layer_visible_cb (int id)
{
  int visible;
  layer_process (NULL, NULL, &visible, id);
  return visible;
}

gint
LayersChanged (int argc, char **argv, Coord x, Coord y)
{
  if (!ghidgui || !ghidgui->menu_bar)
    return 0;

  ghid_config_groups_changed();
  ghid_layer_buttons_update ();
  ghid_layer_selector_show_layers
    (GHID_LAYER_SELECTOR (ghidgui->layer_selector), get_layer_visible_cb);

  /* FIXME - if a layer is moved it should retain its color.  But layers
  |  currently can't do that because color info is not saved in the
  |  pcb file.  So this makes a moved layer change its color to reflect
  |  the way it will be when the pcb is reloaded.
  */
  pcb_colors_from_settings (PCB);
  return 0;
}

static const char toggleview_syntax[] =
"ToggleView(1..MAXLAYER)\n"
"ToggleView(layername)\n"
"ToggleView(Silk|Rats|Pins|Vias|Mask|BackSide)";

static const char toggleview_help[] =
"Toggle the visibility of the specified layer or layer group.";

/* %start-doc actions ToggleView

If you pass an integer, that layer is specified by index (the first
layer is @code{1}, etc).  If you pass a layer name, that layer is
specified by name.  When a layer is specified, the visibility of the
layer group containing that layer is toggled.

If you pass a special layer name, the visibility of those components
(silk, rats, etc) is toggled.  Note that if you have a layer named
the same as a special layer, the layer is chosen over the special layer.

%end-doc */

static int
ToggleView (int argc, char **argv, Coord x, Coord y)
{
  int i, l;

#ifdef DEBUG_MENUS
  puts ("Starting ToggleView().");
#endif

  if (argc == 0)
    {
      AFAIL (toggleview);
    }
  if (isdigit ((int) argv[0][0]))
    {
      l = atoi (argv[0]) - 1;
    }
  else if (strcmp (argv[0], "Silk") == 0)
    l = LAYER_BUTTON_SILK;
  else if (strcmp (argv[0], "Rats") == 0)
    l = LAYER_BUTTON_RATS;
  else if (strcmp (argv[0], "Pins") == 0)
    l = LAYER_BUTTON_PINS;
  else if (strcmp (argv[0], "Vias") == 0)
    l = LAYER_BUTTON_VIAS;
  else if (strcmp (argv[0], "Mask") == 0)
    l = LAYER_BUTTON_MASK;
  else if (strcmp (argv[0], "BackSide") == 0)
    l = LAYER_BUTTON_FARSIDE;
  else
    {
      l = -1;
      for (i = 0; i < max_copper_layer + 2; i++)
	if (strcmp (argv[0], PCB->Data->Layer[i].Name) == 0)
	  {
	    l = i;
	    break;
	  }
      if (l == -1)
	{
	  AFAIL (toggleview);
	}

    }

  /* Now that we've figured out which toggle button ought to control
   * this layer, simply hit the button and let the pre-existing code deal
   */
  ghid_layer_selector_toggle_layer
    (GHID_LAYER_SELECTOR (ghidgui->layer_selector), l);
  return 0;
}

static const char selectlayer_syntax[] =
"SelectLayer(1..MAXLAYER|Silk|Rats)";

static const char selectlayer_help[] =
"Select which layer is the current layer.";

/* %start-doc actions SelectLayer

The specified layer becomes the currently active layer.  It is made
visible if it is not already visible

%end-doc */

static int
SelectLayer (int argc, char **argv, Coord x, Coord y)
{
  int newl;
  if (argc == 0)
    AFAIL (selectlayer);

  if (strcasecmp (argv[0], "silk") == 0)
    newl = LAYER_BUTTON_SILK;
  else if (strcasecmp (argv[0], "rats") == 0)
    newl = LAYER_BUTTON_RATS;
  else
    newl = atoi (argv[0]) - 1;

#ifdef DEBUG_MENUS
  printf ("SelectLayer():  newl = %d\n", newl);
#endif

  /* Now that we've figured out which radio button ought to select
   * this layer, simply hit the button and let the pre-existing code deal
   */
  ghid_layer_selector_select_layer
    (GHID_LAYER_SELECTOR (ghidgui->layer_selector), newl);

  return 0;
}


HID_Action gtk_topwindow_action_list[] = {
  {"LayersChanged", 0, LayersChanged,
   layerschanged_help, layerschanged_syntax},
  {"SelectLayer", 0, SelectLayer,
   selectlayer_help, selectlayer_syntax},
  {"ToggleView", 0, ToggleView,
   toggleview_help, toggleview_syntax}
};

REGISTER_ACTIONS (gtk_topwindow_action_list)

/* 
 * This function is used to check if a specified hotkey in the menu
 * resource file is "special".  In this case "special" means that gtk
 * assigns a particular meaning to it and the normal menu setup will
 * never see these key presses.  We capture those and feed them back
 * into the menu callbacks.  This function is called as new
 * accelerators are added when the menus are being built
 */
static void
ghid_check_special_key (const char *accel, GtkAction *action,
                        const Resource *node)
{
  size_t len;
  unsigned int mods;
  unsigned int ind;

  if (action == NULL || accel == NULL || *accel == '\0')
    return ;

#ifdef DEBUG_MENUS
  printf ("%s(\"%s\", \"%s\")\n", __FUNCTION__, accel, name);
#endif

  mods = 0;
  if (strstr (accel, "<alt>") )
    {
      mods |= GHID_KEY_ALT;
    }
  if (strstr (accel, "<ctrl>") )
    {
      mods |= GHID_KEY_CONTROL;
    }
  if (strstr (accel, "<shift>") )
    {
      mods |= GHID_KEY_SHIFT;
    }

  
  len = strlen (accel);
  
#define  CHECK_KEY(a) ((len >= strlen (a)) && (strcmp (accel + len - strlen (a), (a)) == 0))

  ind = 0;
  if ( CHECK_KEY ("Tab") )
    {
      ind = mods | GHID_KEY_TAB;
    }
  else if ( CHECK_KEY ("Up") )
    {
      ind = mods | GHID_KEY_UP;
    }
  else if ( CHECK_KEY ("Down") )
    {
      ind = mods | GHID_KEY_DOWN;
    }
  else if ( CHECK_KEY ("Left") )
    {
      ind = mods | GHID_KEY_LEFT;
    }
  else if ( CHECK_KEY ("Right") )
    {
      ind = mods | GHID_KEY_RIGHT;
    }

  if (ind > 0) 
    {
      if (ind >= N_HOTKEY_ACTIONS)
	{
	  fprintf (stderr, "ERROR:  overflow of the ghid_hotkey_actions array.  Index = %d\n"
		   "Please report this.\n", ind);
	  exit (1);
	}

      ghid_hotkey_actions[ind].action = action;
      ghid_hotkey_actions[ind].node = node;
#ifdef DEBUG_MENUS
      printf ("Adding \"special\" hotkey to ghid_hotkey_actions[%u] :"
	      " %s (%s)\n", ind, accel, name);
#endif
    }
}

/*! \brief Finds the gpcb-menu.res file */
char *
get_menu_filename (void)
{
  char *rv = NULL;
  char *home_pcbmenu = NULL;

  /* homedir is set by the core */
  if (homedir)
    {
      Message (_("Note:  home directory is \"%s\"\n"), homedir);
      home_pcbmenu = Concat (homedir, PCB_DIR_SEPARATOR_S, ".pcb",
                             PCB_DIR_SEPARATOR_S, "gpcb-menu.res", NULL);
    }
  else
    Message (_("Warning:  could not determine home directory\n"));

  if (access ("gpcb-menu.res", R_OK) == 0)
    rv = strdup ("gpcb-menu.res");
  else if (home_pcbmenu != NULL && (access (home_pcbmenu, R_OK) == 0) )
    rv = home_pcbmenu;
  else if (access (pcbmenu_path, R_OK) == 0)
    rv = strdup (pcbmenu_path);

  return rv;
}

static GtkWidget *
ghid_load_menus (void)
{
  char *filename;
  const Resource *r = 0, *bir;
  const Resource *mr;
  GtkWidget *menu_bar = NULL;
  int i;

  for (i = 0; i < N_HOTKEY_ACTIONS; i++)
    {
      ghid_hotkey_actions[i].action = NULL;
      ghid_hotkey_actions[i].node = NULL;
    }
 
  bir = resource_parse (0, gpcb_menu_default);
  if (!bir)
    {
      fprintf (stderr, _("Error: internal menu resource didn't parse\n"));
      exit(1);
    }

  filename = get_menu_filename ();
  if (filename)
    {
      Message ("Loading menus from %s\n", filename);
      r = resource_parse (filename, 0);
    }

  if (!r)
    {
      Message ("Using default menus\n");
      r = bir;
    }
  free (filename);

  mr = resource_subres (r, "MainMenu");
  if (!mr)
    mr = resource_subres (bir, "MainMenu");
    
  if (mr)
    {
      menu_bar = ghid_main_menu_new (G_CALLBACK (ghid_menu_cb),
                                     ghid_check_special_key);
      ghid_main_menu_add_resource (GHID_MAIN_MENU (menu_bar), mr);
    }

  mr = resource_subres (r, "PopupMenus");
  if (!mr)
    mr = resource_subres (bir, "PopupMenus");

  if (mr)
    {
      int i;
      for (i = 0; i < mr->c; i++)
        if (resource_type (mr->v[i]) == 101)
          /* This is a named resource which defines a popup menu */
          ghid_main_menu_add_popup_resource (GHID_MAIN_MENU (menu_bar),
                                             mr->v[i].name, mr->v[i].subres);
    }

#ifdef DEBUG_MENUS
   puts ("Finished loading menus.");
#endif

    mr = resource_subres (r, "Mouse");
    if (!mr)
      mr = resource_subres (bir, "Mouse");
    if (mr)
      load_mouse_resource (mr);

  return menu_bar;
}

/* ------------------------------------------------------------ */

static const char adjuststyle_syntax[] =
"AdjustStyle()\n";

static const char adjuststyle_help[] =
"Open the window which allows editing of the route styles.";

/* %start-doc actions AdjustStyle

Opens the window which allows editing of the route styles.

%end-doc */

static int
AdjustStyle(int argc, char **argv, Coord x, Coord y)
{
  RouteStyleType *rst = NULL;
  
  if (argc > 1)
    AFAIL (adjuststyle);

  if (route_style_index >= NUM_STYLES)
    rst = &route_style_button[route_style_index].route_style;
  ghid_route_style_dialog (route_style_index, rst);

  return 0;
}

/* ------------------------------------------------------------ */

static const char editlayergroups_syntax[] =
"EditLayerGroups()\n";

static const char editlayergroups_help[] =
"Open the preferences window which allows editing of the layer groups.";

/* %start-doc actions EditLayerGroups

Opens the preferences window which is where the layer groups
are edited.  This action is primarily provides to provide menu
resource compatibility with the lesstif HID.

%end-doc */

static int
EditLayerGroups(int argc, char **argv, Coord x, Coord y)
{
  
  if (argc != 0)
    AFAIL (editlayergroups);

  hid_actionl ("DoWindows", "Preferences", NULL);

  return 0;
}

/* ------------------------------------------------------------ */

HID_Action ghid_menu_action_list[] = {
  {"AdjustStyle", 0, AdjustStyle, adjuststyle_help, adjuststyle_syntax},
  {"EditLayerGroups", 0, EditLayerGroups, editlayergroups_help, editlayergroups_syntax}
};

REGISTER_ACTIONS (ghid_menu_action_list)

