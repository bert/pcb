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

#include "gtk-pcb-layer-selector.h"
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


typedef enum {GHID_FLAG_ACTIVE, GHID_FLAG_CHECKED, GHID_FLAG_VISIBLE} MenuFlagType;

/* Used by the menuitems that are toggle actions */
typedef struct
{
  const char *actionname;
  const char *flagname;
  MenuFlagType flagtype;
  int oldval;
  char *xres;
} ToggleFlagType;

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

/* actions for the @routestyles menuitems */
static GtkToggleActionEntry routestyle_toggle_entries[N_ROUTE_STYLES];
static Resource *routestyle_resources[N_ROUTE_STYLES];

#define MENUITEM "MenuItem"
#define TMENUITEM "TMenuItem"
#define ROUTESTYLE "RouteStyle"


static ToggleFlagType *tflags = 0;
static int n_tflags = 0;
static int max_tflags = 0;

GhidGui _ghidgui, *ghidgui = NULL;

GHidPort ghid_port, *gport;

static GdkColor WhitePixel;

static gchar		*bg_image_file;

static char *ghid_hotkey_actions[256];


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

static const char *
ghid_check_unique_accel (const char *accelerator)
{
  static int n_list = 0;
  static char **accel_list;
  static int amax = 0;
  int i;
  const char * a = accelerator;

  if (accelerator == NULL)
    return NULL;

  if (strlen (accelerator) == 0)
    return accelerator;

  if (amax >= n_list) 
    {
      n_list += 128;
      if ( (accel_list = (char **)realloc (accel_list, n_list * sizeof (char *))) == NULL)
	{
	  fprintf (stderr, "%s():  realloc failed\n", __FUNCTION__);
	  exit (1);
	}
    }

  for (i = 0; i < amax ; i++) 
    {
      if (strcmp (accel_list[i], accelerator) == 0)
	{
	  Message (_("Duplicate accelerator found: \"%s\"\n"
		   "The second occurance will be dropped\n"),
		   accelerator);
	  a = NULL;
	  break;
	}
    }
  accel_list[amax] = strdup (accelerator);
  amax++;

  return a;
}


/* ------------------------------------------------------------------
 *  note_toggle_flag()
 */

static void
note_toggle_flag (const char *actionname, MenuFlagType type, char *name)
{

  #ifdef DEBUG_MENUS
  printf ("note_toggle_flag(\"%s\", %d, \"%s\")\n", actionname, type, name);
  #endif

  if (n_tflags >= max_tflags)
    {
      max_tflags += 20;
      tflags = (ToggleFlagType *)realloc (tflags, max_tflags * sizeof (ToggleFlagType));
    }
  tflags[n_tflags].actionname = strdup (actionname);
  tflags[n_tflags].flagname = name;
  tflags[n_tflags].flagtype = type;
  tflags[n_tflags].oldval = -1;
  tflags[n_tflags].xres = "none";
  n_tflags++;
}


void
ghid_update_toggle_flags ()
{
  int i;

  GtkAction *a;
  gboolean old_holdoff;
  gboolean active;
  char tmpnm[40];
  GValue setfalse = { 0 };
  GValue settrue = { 0 };
  GValue setlabel = { 0 };

  g_value_init (&setfalse, G_TYPE_BOOLEAN);
  g_value_init (&settrue, G_TYPE_BOOLEAN);
  g_value_set_boolean (&setfalse, FALSE);
  g_value_set_boolean (&settrue, TRUE);
  g_value_init (&setlabel, G_TYPE_STRING);

  /* mask the callbacks */
  old_holdoff = ghidgui->toggle_holdoff;
  ghidgui->toggle_holdoff = TRUE;

  for (i = 0; i < n_tflags; i++)
    {
      switch (tflags[i].flagtype)
	{
	case GHID_FLAG_ACTIVE:
	  {
	    int v = hid_get_flag (tflags[i].flagname);
	    a = gtk_action_group_get_action (ghidgui->main_actions, tflags[i].actionname);
	    g_object_set_property (G_OBJECT (a), "sensitive", v? &settrue : &setfalse);
	    tflags[i].oldval = v;
	  }
	  break;

	case GHID_FLAG_CHECKED:
	  {
	    int v = hid_get_flag (tflags[i].flagname);
	    a = gtk_action_group_get_action (ghidgui->main_actions, tflags[i].actionname);
	    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (a), v? TRUE : FALSE);
	    tflags[i].oldval = v;
	  }
	  break;

	default:
	  printf ("Skipping flagtype %d\n", tflags[i].flagtype);
	  break;
	}
    }

  for (i = 0; i < N_ROUTE_STYLES; i++)
    {
      sprintf (tmpnm, "%s%d", ROUTESTYLE, i);
      a = gtk_action_group_get_action (ghidgui->main_actions, tmpnm);
      if (i >= NUM_STYLES)
	{
	  g_object_set_property (G_OBJECT (a), "visible", &setfalse);
	}

      /* Update the toggle states */
      active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (route_style_button[i].button));
#ifdef DEBUG_MENUS
      printf ("ghid_update_toggle_flags():  route style %d, value is %d\n", i, active);
#endif
      gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (a), active);
	
    }

  g_value_unset (&setfalse);
  g_value_unset (&settrue);
  g_value_unset (&setlabel);
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


/*
 * This is the main menu callback function.  The callback looks at
 * the gtk action name to figure out which menuitem was chosen.  Then
 * it looks up in a table to find the pcb actions which should be
 * executed.  All menus go through this callback.  The tables of
 * actions are loaded from the menu resource file at startup.
 *
 * In addition, all hotkeys go through the menus which means they go
 * through here.
 */

static void
ghid_menu_cb (GtkAction * action, gpointer data)
{
  const gchar * name;
  int id = 0;
  int vi;
  Resource *node = NULL;
  static int in_cb = 0;
  gboolean old_holdoff;

  /* If we don't do this then we can end up in loops where changing
   * the state of the toggle actions triggers the callbacks and
   * the call back updates the state of the actions.
   */
  if (in_cb)
    return;
  else
    in_cb = 1;

  /* 
   * Normally this callback is triggered by the menus in which case
   * action will be the gtk action which was triggered.  In the case
   * of the "special" hotkeys we will call this callback directly and
   * pass in the name of the menu that it corresponds to in via the
   * data argument
   */
  if (action != NULL) 
    {
      name = gtk_action_get_name (action);
    }
  else
    {
      name = (char *) data;
#ifdef DEBUG_MENUS
      printf ("ghid_menu_cb():  name = \"%s\"\n", UNKNOWN (name));
#endif
    }

  if (name == NULL)
    {
      fprintf (stderr, "%s(%p, %p):  name == NULL\n", 
	       __FUNCTION__, action, data);
      in_cb = 0;
      return;
    }

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

      /* toggle_holdoff lets us update the state of the menus without
       * actually triggering all the callbacks
       */
      if (ghidgui->toggle_holdoff == TRUE) 
	node = NULL;
      else
	node = toggle_action_resources[id];
    }
  else if ( strncmp (name, ROUTESTYLE, strlen (ROUTESTYLE)) == 0)
    {
      id = atoi (name + strlen (ROUTESTYLE));
      if (ghidgui->toggle_holdoff != TRUE) 
	ghid_route_style_button_set_active (id);
      node = NULL;
    }
  else
    {
      fprintf (stderr, "ERROR:  ghid_menu_cb():  name = \"%s\" is unknown\n", name);
    }
    

#ifdef DEBUG_MENUS
  printf ("ghid_menu_cb():  name = \"%s\", id = %d\n", name, id);
#endif

  /* Now we should have a pointer to the actions to execute */
  if (node != NULL)
    {
      for (vi = 1; vi < node->c; vi++)
	if (resource_type (node->v[vi]) == 10)
	  {
#ifdef DEBUG_MENUS
	    printf ("    %s\n", node->v[vi].value);
#endif
	    hid_parse_actions (node->v[vi].value);
	  }
    }
  else {
#ifdef DEBUG_MENUS
    printf ("    NOOP\n");
#endif
  }


  /*
   * Now mask off any callbacks and update the state of any toggle
   * menuitems.  This is where we do things like sync the layer or
   * tool checks marks in the menus with the layer or tool buttons
   */
  old_holdoff = ghidgui->toggle_holdoff;
  ghidgui->toggle_holdoff = TRUE;
  ghid_update_toggle_flags ();
  ghidgui->toggle_holdoff = old_holdoff;
  
  in_cb = 0;

  /*
   * and finally, make any changes show up in the status line and the
   * screen 
   */
  if (ghidgui->toggle_holdoff == FALSE) 
    {
      AdjustAttachedObjects ();
      ghid_invalidate_all ();
      ghid_window_set_name_label (PCB->Name);
      ghid_set_status_line_label ();
#ifdef FIXME
      g_idle_add (ghid_idle_cb, NULL);
#endif
    }

}

void ghid_hotkey_cb (int which)
{
#ifdef DEBUG_MENUS
  printf ("%s(%d) -> \"%s\"\n", __FUNCTION__, 
	  which, UNKNOWN (ghid_hotkey_actions[which]));
#endif
  if (ghid_hotkey_actions[which] != NULL)
    ghid_menu_cb (NULL, ghid_hotkey_actions[which]);
}


/* ============== ViewMenu callbacks =============== */
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

/*! \brief Callback for GtkPcbLayerSelector layer selection */
static void
layer_selector_select_callback (GtkPcbLayerSelector *ls, int layer, gpointer d)
{
  gboolean active;
  layer_process (NULL, NULL, &active, layer);

  /* Select Layer */
  PCB->SilkActive = (layer == LAYER_BUTTON_SILK);
  PCB->RatDraw  = (layer == LAYER_BUTTON_RATS);
  if (layer < max_copper_layer)
    ChangeGroupVisibility (layer, true, true);

  /* Ensure layer is turned on */
  gtk_pcb_layer_selector_make_selected_visible (ls);

  ghid_invalidate_all ();
}

/*! \brief Callback for GtkPcbLayerSelector layer toggling */
static void
layer_selector_toggle_callback (GtkPcbLayerSelector *ls, int layer, gpointer d)
{
  gboolean redraw = FALSE;
  gboolean active;
  layer_process (NULL, NULL, &active, layer);

  active = !active;
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
  if (!gtk_pcb_layer_selector_select_next_visible (ls))
    gtk_pcb_layer_selector_toggle_layer (ls, layer);

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
  gtk_action_group_add_actions (actions, new_entries, menuitem_cnt, port);

  gtk_action_group_add_toggle_actions (actions, new_toggle_entries,
				       tmenuitem_cnt, port);

  ghid_make_programmed_menu_actions ();

  gtk_action_group_add_toggle_actions (actions,
				       routestyle_toggle_entries,
				       N_ROUTE_STYLES, port);

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
  GtkActionGroup *layer_actions;
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
  layer_actions = gtk_pcb_layer_selector_get_action_group
          (GTK_PCB_LAYER_SELECTOR (ghidgui->layer_selector));
 
  gtk_ui_manager_insert_action_group (ui, actions, 0);
  gtk_ui_manager_insert_action_group (ui, layer_actions, 0);

  gtk_window_add_accel_group (GTK_WINDOW (gport->top_window),
			      gtk_ui_manager_get_accel_group (ui));

  if (!gtk_ui_manager_add_ui_from_string (ui, new_ui_info, -1, &error))
    {
      g_message ("building menus failed: %s", error->message);
      g_error_free (error);
    }

  gtk_ui_manager_set_add_tearoffs (ui, TRUE);

  gtk_container_add (GTK_CONTAINER (frame),
		     gtk_ui_manager_get_widget (ui, "/MenuBar"));

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

  if (!ghidgui->name_label)
    return;

  if (!PCB->Filename  || !*PCB->Filename)
    filename = g_strdup(_("Unsaved.pcb"));
  else
    filename = g_strdup(PCB->Filename);

  if (ghidgui->ghid_title_window)
    {
      gtk_widget_hide (ghidgui->label_hbox);
      str = g_strdup_printf ("%s%s (%s) - PCB", PCB->Changed ? "*": "",
                             ghidgui->name_label_string, filename);
    }
  else
    {
      gtk_widget_show (ghidgui->label_hbox);
      str = g_strdup_printf (" <b><big>%s</big></b> ",
                             ghidgui->name_label_string);
      gtk_label_set_markup (GTK_LABEL (ghidgui->name_label), str);
      str = g_strdup_printf ("%s%s - PCB", PCB->Changed ? "*": "",
                             filename);
    }
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
  GtkPcbLayerSelector *layersel = GTK_PCB_LAYER_SELECTOR (layer_selector);
  gchar *text;
  gchar *color_string;
  gboolean active;
 
  layer_process (&color_string, &text, &active, LAYER_BUTTON_SILK);
  gtk_pcb_layer_selector_add_layer (layersel, LAYER_BUTTON_SILK,
                                    text, color_string, active, TRUE);
  layer_process (&color_string, &text, &active, LAYER_BUTTON_RATS);
  gtk_pcb_layer_selector_add_layer (layersel, LAYER_BUTTON_RATS,
                                    text, color_string, active, TRUE);
  layer_process (&color_string, &text, &active, LAYER_BUTTON_PINS);
  gtk_pcb_layer_selector_add_layer (layersel, LAYER_BUTTON_PINS,
                                    text, color_string, active, FALSE);
  layer_process (&color_string, &text, &active, LAYER_BUTTON_VIAS);
  gtk_pcb_layer_selector_add_layer (layersel, LAYER_BUTTON_VIAS,
                                    text, color_string, active, FALSE);
  layer_process (&color_string, &text, &active, LAYER_BUTTON_FARSIDE);
  gtk_pcb_layer_selector_add_layer (layersel, LAYER_BUTTON_FARSIDE,
                                    text, color_string, active, FALSE);
  layer_process (&color_string, &text, &active, LAYER_BUTTON_MASK);
  gtk_pcb_layer_selector_add_layer (layersel, LAYER_BUTTON_MASK,
                                    text, color_string, active, FALSE);
}

/*! \brief callback for gtk_pcb_layer_selector_update_colors */
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
  gtk_pcb_layer_selector_update_colors
    (GTK_PCB_LAYER_SELECTOR (ghidgui->layer_selector), get_layer_color);
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
      gtk_pcb_layer_selector_add_layer (GTK_PCB_LAYER_SELECTOR (layersel), i,
                                        text, color_string, active, TRUE);
    }
}


/*! \brief callback for gtk_pcb_layer_selector_delete_layers */
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

  gtk_pcb_layer_selector_delete_layers
    (GTK_PCB_LAYER_SELECTOR (ghidgui->layer_selector),
     get_layer_delete);
  make_layer_buttons (ghidgui->layer_selector);
  make_virtual_layer_buttons (ghidgui->layer_selector);

  /* Sync selected layer with PCB's state */
  if (PCB->RatDraw)
    layer = LAYER_BUTTON_RATS;
  else if (PCB->SilkActive)
    layer = LAYER_BUTTON_SILK;
  else
    layer = LayerStack[0];

  gtk_pcb_layer_selector_select_layer
    (GTK_PCB_LAYER_SELECTOR (ghidgui->layer_selector), layer);
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
  GtkWidget *vbox_main, *vbox_left, *hbox_middle, *hbox = NULL;
  GtkWidget *viewport, *ebox, *vbox, *frame;
  GtkWidget *label;
  GHidPort *port = &ghid_port;
  gchar *s;
  GtkWidget *scrolled;

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

  /* Build layer menus */
  ghidgui->layer_selector = gtk_pcb_layer_selector_new ();
  make_layer_buttons (ghidgui->layer_selector);
  make_virtual_layer_buttons (ghidgui->layer_selector);
  g_signal_connect (G_OBJECT (ghidgui->layer_selector), "select_layer",
                    G_CALLBACK (layer_selector_select_callback),
                    NULL);
  g_signal_connect (G_OBJECT (ghidgui->layer_selector), "toggle_layer",
                    G_CALLBACK (layer_selector_toggle_callback),
                    NULL);
  /* Build main menu */
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

  vbox = ghid_scrolled_vbox(vbox_left, &scrolled,
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX(vbox), ghidgui->layer_selector,
                      FALSE, FALSE, 0);

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
  {"listen", "Listen for actions on stdin",
   HID_Boolean, 0, 0, {0, 0, 0}, 0, &stdin_listen},
#define HA_listen 0

  {"bg-image", "Background Image",
   HID_String, 0, 0, {0, 0, 0}, 0, &bg_image_file},
#define HA_bg_image 1

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

gint
LayersChanged (int argc, char **argv, Coord x, Coord y)
{
  if (!ghidgui || !ghidgui->ui_manager)
    return 0;

  ghid_config_groups_changed();
  ghid_layer_buttons_update ();

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
  static gboolean in_toggle_view = 0;

#ifdef DEBUG_MENUS
  printf ("Starting ToggleView().  in_toggle_view = %d\n", in_toggle_view);
#endif
  if (in_toggle_view)
    {
      fprintf (stderr, "ToggleView() called on top of another ToggleView()\n"
	       "Please report this and how it happened\n");
      return 0;
    }

  in_toggle_view = 1;

  if (argc == 0)
    {
      in_toggle_view = 0;
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
	  in_toggle_view = 0;
	  AFAIL (toggleview);
	}

    }

  printf ("ToggleView():  l = %d\n", l);

  /* Now that we've figured out which toggle button ought to control
   * this layer, simply hit the button and let the pre-existing code deal
   */
  gtk_pcb_layer_selector_toggle_layer
    (GTK_PCB_LAYER_SELECTOR (ghidgui->layer_selector), l);
  in_toggle_view = 0;
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
  gtk_pcb_layer_selector_select_layer
    (GTK_PCB_LAYER_SELECTOR (ghidgui->layer_selector), newl);

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
static void ghid_check_special_key (const char *accel, const char *name)
{
  size_t len;
  unsigned int mods;
  unsigned int ind;

  if ( accel == NULL || *accel == '\0' )
    {
      return ;
    }

#ifdef DEBUG_MENUS
  printf ("%s(\"%s\", \"%s\")\n", __FUNCTION__, accel, name);
#endif

  mods = 0;
  if (strstr (accel, "<alt>") )
    {
      mods |= GHID_KEY_ALT;
    }
  if (strstr (accel, "<control>") )
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
      if (ind >= (sizeof (ghid_hotkey_actions) / sizeof (char *)) )
	{
	  fprintf (stderr, "ERROR:  overflow of the ghid_hotkey_actions array.  Index = %d\n"
		   "Please report this.\n", ind);
	  exit (1);
	}

      ghid_hotkey_actions[ind] = g_strdup (name);
#ifdef DEBUG_MENUS
      printf ("Adding \"special\" hotkey to ghid_hotkey_actions[%u] :"
	      " %s (%s)\n", ind, accel, name);
#endif
    }
}


#define INDENT_INC 5

static void
ghid_append_action (const char * name, const char *stock_id, 
		    const char *label, const char *accelerator,
		    const char *tooltip)
{

#ifdef DEBUG_MENUS
  printf ("ghid_append_action(\"%s\", \"%s\", \"%s\",  \"%s\", \"%s\")\n",
	  UNKNOWN (name), 
	  UNKNOWN (stock_id), 
	  UNKNOWN (label), 
	  UNKNOWN (accelerator), 
	  UNKNOWN (tooltip));
#endif

  accelerator = ghid_check_unique_accel (accelerator);

  if ( (new_entries = (GtkActionEntry *)realloc (new_entries, 
			       (menuitem_cnt + 1) * sizeof (GtkActionEntry))) == NULL)
    {
      fprintf (stderr, "ghid_append_action():  realloc of new_entries failed\n");
      exit (1);
    }
  

  if ( (action_resources = (Resource **)realloc (action_resources,
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
  new_entries[menuitem_cnt].accelerator = ( (accelerator == NULL || *accelerator == '\0')
					    ? NULL : strdup (accelerator));
  new_entries[menuitem_cnt].tooltip = (tooltip == NULL ? NULL : strdup (tooltip));
  new_entries[menuitem_cnt].callback = G_CALLBACK (ghid_menu_cb);

  ghid_check_special_key (accelerator, name);
  menuitem_cnt++;
}

static void
ghid_append_toggle_action (const char * name, const char *stock_id, 
			   const char *label, const char *accelerator,
			   const char *tooltip, int active)
{

  accelerator = ghid_check_unique_accel (accelerator);

  if ( (new_toggle_entries = (GtkToggleActionEntry *)realloc (new_toggle_entries, 
				      (tmenuitem_cnt + 1) * sizeof (GtkToggleActionEntry))) == NULL)
    {
      fprintf (stderr, "ghid_append_toggle_action():  realloc of new_toggle_entries failed\n");
      exit (1);
    }
  

  if ( (toggle_action_resources = (Resource **)realloc (toggle_action_resources,
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

  ghid_check_special_key (accelerator, name);
  tmenuitem_cnt++;
}

/*
 * Some keys need to be replaced by a name for the gtk accelerators to
 * work.  This table contains the translations.  The "in" character is
 * what would appear in gpcb-menu.res and the "out" string is what we
 * have to feed to gtk.  I was able to find these by using xev to find
 * the keycode and then looked at gtk+-2.10.9/gdk/keynames.txt (from the
 * gtk source distribution) to figure out the names that go with the 
 * codes.
 */
typedef struct
{
  const char in;
  const char *out;
} KeyTable;
static KeyTable key_table[] = 
  {
    {':', "colon"},
    {'=', "equal"},
    {'/', "slash"},
    {'[', "bracketleft"},
    {']', "bracketright"},
    {'.', "period"},
    {'|', "bar"}
  };
static int n_key_table = sizeof (key_table) / sizeof (key_table[0]);

static void
add_resource_to_menu (char * menu, Resource * node, void * callback, int indent)
{
  int i, j;
  char *v;
  Resource *r;
  char tmps[32];
  char accel[64];
  int accel_n;
  char *menulabel = NULL;
  char ch[2];
  char m = '\0';
  char *cname = NULL;

  ch[1] = '\0';

  for (i = 0; i < node->c; i++)
    switch (resource_type (node->v[i]))
      {
      case 101:		/* named subnode */
	add_resource_to_menu (node->v[i].name, node->v[i].subres, 
			      callback, indent + INDENT_INC);
	break;

      case 1:			/* unnamed subres */
	accel[0] = '\0';
	/* remaining number of chars available in accel (- 1 for '\0')*/
	accel_n = sizeof (accel) - 1;
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
#ifdef DEBUG_MENUS
	    printf ("    found resource value m=\"%s\"\n", v);
#endif
	    m = *v;
	  }
	if ((r = resource_subres (node->v[i].subres, "a")))
	  {
	    /* for the accelerator, it has 2 values  like
	     *
	     * a={"Ctrl-Q" "Ctrl<Key>q"}
	     * The first one is what's displayed in the menu and the
	     * second actually defines the hotkey.  Actually, the
	     * first value is only used by the lesstif HID and is
	     * ignored by the gtk HID.  The second value is used by both.
	     *
	     * We have to translate some strings.  See
	     * gtk+-2.10.9/gdk/keynames.txt from the gtk distribution
	     * as well as the output from xev(1).
	     *
	     * Modifiers:
	     *
	     * "Ctrl" -> "<control>"
	     * "Shift" -> "<shift>"
	     * "Alt" -> "<alt>"
	     * "<Key>" -> ""
	     *
	     * keys:
	     *
	     * " " -> ""
	     * "Enter" -> "Return"
	     *
	     */
	    char *p;
	    int j;
	    enum {KEY, MOD} state;

	    state = MOD;
#ifdef DEBUG_MENUS
	    printf ("    accelerator a=%p.  r->v[0].value = \"%s\", r->v[1].value = \"%s\" ", 
		    r, r->v[0].value, r->v[1].value);
#endif
	    p = r->v[1].value;
	    while (*p != '\0')
	      {
		switch (state)
		  {
		  case MOD:
		    if (*p == ' ')
		      {
			p++;
		      }
		    else if (strncmp (p, "<Key>", 5) == 0)
		      {
			state = KEY;
			p += 5;
		      }
		    else if (strncmp (p, "Ctrl", 4) == 0)
		      {
			strncat (accel, "<control>", accel_n);
			accel_n -= strlen ("<control>");
			p += 4;
		      }
		    else if (strncmp (p, "Shift", 5) == 0)
		      {
			strncat (accel, "<shift>", accel_n);
			accel_n -= strlen ("<shift>");
			p += 5;
		      }
		    else if (strncmp (p, "Alt", 3) == 0)
		      {
			strncat (accel, "<alt>", accel_n);
			accel_n -= strlen ("<alt>");
			p += 3;
		      }
		    else
		      {
			static int gave_msg = 0;
			Message (_("Don't know how to parse \"%s\" as an accelerator in the menu resource file.\n"),
				 p);
			
			if (! gave_msg) 
			  {
			    gave_msg = 1;
			    Message (_("Format is:\n"
				     "modifiers<Key>k\n"
				     "where \"modifiers\" is a space separated list of key modifiers\n"
				     "and \"k\" is the name of the key.\n"
				     "Allowed modifiers are:\n"
				     "   Ctrl\n"
				     "   Shift\n"
				     "   Alt\n"
				     "Please note that case is important.\n"));
			  }
			/* skip processing the rest */
			accel[0] = '\0';
			accel_n = sizeof (accel) - 1;
			p += strlen (p);
		      }
		    break;

		  case KEY:
		    if (strncmp (p, "Enter", 5) == 0)
		      {
			strncat (accel, "Return", accel_n);
			accel_n -= strlen ("Return");
			p += 5;
		      }
		    else
		      {
			ch[0] = *p;
			for (j = 0; j < n_key_table; j++)
			  {
			    if ( *p == key_table[j].in)
			      {
				strncat (accel, key_table[j].out, accel_n);
				accel_n -= strlen (key_table[j].out);
				j = n_key_table;
			      }
			  }
			
			if (j == n_key_table)
			  {
			    strncat (accel, ch, accel_n);
			    accel_n -= strlen (ch);
			  }
		    
			p++;
		      }
		    break;

		  }

		if (G_UNLIKELY (accel_n < 0))
		  {
		    accel_n = 0;
		    Message ("Accelerator \"%s\" is too long to be parsed.\n", r->v[1].value);
		    accel[0] = '\0';
		    accel_n = 0;
		    /* skip processing the rest */
		    p += strlen (p);
		  }
	      }
#ifdef DEBUG_MENUS
	    printf ("\n    translated = \"%s\"\n", accel);
#endif
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
	
	if (m == '\0')
	  menulabel = strdup (v);
	else
	  {
	    /* we've been given a mneumonic so we need to insert an
	     * "_" into the label.  For example if the string is
	     * "Quit Program" and we have m=Q, we'd need to produce
	     * "_Quit Program".
	     */
	    char *s1, *s2;
	    size_t l;

	    l = strlen (_(v)) + 2;
#ifdef DEBUG_MENUS
	    printf ("allocate %ld bytes\n", l);
#endif
	    if ( (menulabel = (char *) malloc ( l * sizeof (char)))
		 == NULL)
	      {
		fprintf (stderr, "add_resource_to_menu():  malloc failed\n");
		exit (1);
	      }
	    
	    s1 = menulabel;
	    s2 = _(v);
	    while (*s2 != '\0')
	      {
		if (*s2 == m)
		  {
		    /* add the underscore and quit looking for more 
		     * matches since we only want to add 1 underscore
		     */
		    *s1 = '_';
		    s1++;
		    m = '\0';
		  }
		*s1 = *s2;
		s1++;
		s2++;
	      }
	    *s1 = '\0';
	  }
#ifdef DEBUG_MENUS
	printf ("v = \"%s\", label = \"%s\"\n", v, menulabel);
#endif
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
	    cname = strdup (tmps);

	    /* add to the action entries */
	    /* name, stock_id, label, accelerator, tooltip */
	    ghid_append_action (tmps, NULL, menulabel, accel, NULL);

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
	    char *tip = resource_value (node->v[i].subres, "tip");
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
#ifdef DEBUG_MENUS
		printf ("Found a \"checked\" menu choice \"%s\", \"%s\"\n", v, checked);
#endif
		if (strchr (checked, ','))
		  {
		    /* we're comparing a flag against a value */
#ifdef DEBUG_MENUS
		    printf ("Found checked comparing a flag to a value\n");
#endif
		  }
		else
		  {
		    /* we're looking at a binary flag */
		    /* name, stock_id, label, accelerator, tooltip, callback, is_active
		    printf ("Found checked using a flag as a binary\n");

		     */
		  }

		sprintf (tmps, "%s%d", TMENUITEM, tmenuitem_cnt);
		cname = strdup (tmps);

		/* add to the action entries */
		/* name, stock_id, label, accelerator, tooltip, is_active */
		ghid_append_toggle_action (tmps, NULL, menulabel, accel, tip, 1);

		ghid_ui_info_indent (indent);
		ghid_ui_info_append ("<menuitem action='");
		ghid_ui_info_append (tmps);
		ghid_ui_info_append ("'/>\n");

		toggle_action_resources[tmenuitem_cnt-1] = node->v[i].subres;
		
	      }
	    else if (label && strcmp (label, "false") == 0)
	      {
		/* we have sensitive=false so just put a label in the
		 * GUI  -- FIXME -- actually do something here....
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
		cname = strdup (tmps);

		/* add to the action entries */
		/* name, stock_id, label, accelerator, tooltip */
		ghid_append_action (tmps, NULL, menulabel, accel, tip);

		ghid_ui_info_indent (indent);
		ghid_ui_info_append ("<menuitem action='");
		ghid_ui_info_append (tmps);
		ghid_ui_info_append ("'/>\n");


		action_resources[menuitem_cnt-1] = node->v[i].subres;

#ifdef DEBUG_MENUS
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
			|| strcmp (n, "sensitive") == 0
			|| strcmp (n, "tip") == 0
			)
		      break;

		    /* log checked and active special values */
		    if (strcmp (n, "checked") == 0)
		      {
#ifdef DEBUG_MENUS
			printf ("%s is checked\n", node->v[i].subres->v[j].value);
#endif
			note_toggle_flag (new_toggle_entries[tmenuitem_cnt-1].name,
					  GHID_FLAG_CHECKED,
					  node->v[i].subres->v[j].value);
			break;
		      }
		    if (strcmp (n, "active") == 0)
		      {
			if (cname != NULL) 
			  {
			    note_toggle_flag (cname,
					      GHID_FLAG_ACTIVE,
					      node->v[i].subres->v[j].value);
			  }
			else
			  {
			    printf ("WARNING: %s cname == NULL\n", __FUNCTION__);
			  }
			break;
		      }

		    /* if we got this far it is supposed to be an X
		     * resource.  For now ignore it and warn the user
		     */
		    Message (_("The gtk gui currently ignores \"%s\""
				"as part of a menuitem resource.\n"
				"Feel free to provide patches\n"),
			     node->v[i].subres->v[j].value);
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
#ifdef DEBUG_MENUS
	printf ("resource_type for node #%d is 10 (unnamed value).  value=\"%s\"\n", 
		i, node->v[i].value);
#endif

	if (node->v[i].value[0] == '@')
	  {
	    if (strcmp (node->v[i].value, "@layerview") == 0)
	      {
                gchar *tmp = gtk_pcb_layer_selector_get_view_xml
                  (GTK_PCB_LAYER_SELECTOR (ghidgui->layer_selector));
                ghid_ui_info_append (tmp);
                g_free (tmp);
	      }
	    else if (strcmp (node->v[i].value, "@layerpick") == 0)
	      {
                gchar *tmp = gtk_pcb_layer_selector_get_pick_xml
                  (GTK_PCB_LAYER_SELECTOR (ghidgui->layer_selector));
                ghid_ui_info_append (tmp);
                g_free (tmp);
	      }
	    else if (strcmp (node->v[i].value, "@routestyles") == 0)
	      {
		int i;
		char tmpid[40];
		for (i = 0 ; i <  N_ROUTE_STYLES; i++)
		  {
		    sprintf (tmpid, "<menuitem action='%s%d' />\n", 
			     ROUTESTYLE, i);
		    ghid_ui_info_indent (indent);
		    ghid_ui_info_append (tmpid);
		  }
	      }
	    else
	      {
		Message (_("GTK GUI currently ignores \"%s\" in the menu\n"
			"resource file.\n"), node->v[i].value);
	      }
	    
	  }
	
	else if (strcmp (node->v[i].value, "-") == 0)
	  {
	    ghid_ui_info_indent (indent);
	    ghid_ui_info_append ("<separator/>\n");
	  }
	else if (i > 0)
	  {
	    /* This is where you get with an action-less menuitem.
	     * It is really just useful when you're starting to build
	     * a new menu and you're looking to get the layout
	     * right.
	     */
	    sprintf (tmps, "%s%d", MENUITEM, menuitem_cnt);
	    cname = strdup (tmps);
	    
	    /* add to the action entries 
	     * name, stock_id, label, accelerator, tooltip 
	     * Note that we didn't get the mneumonic added in here,
	     * but since this is really for a dummy menu (no
	     * associated actions), I'm not concerned.
	     */
	    
	    ghid_append_action (tmps, NULL, node->v[i].value, accel, NULL);

	    ghid_ui_info_indent (indent);
	    ghid_ui_info_append ("<menuitem action='");
	    ghid_ui_info_append (tmps);
	    ghid_ui_info_append ("'/>\n");
	    
	    action_resources[menuitem_cnt-1] = NULL;

	  }
	break;
      }
  
  if (cname != NULL)
    free (cname);

  if (menulabel != NULL)
    free (menulabel);
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
ghid_ui_info_append (const gchar * newone)
{
  gchar *p;

  if (new_ui_info_sz == 0) 
    {
      new_ui_info_sz = 1024;
      new_ui_info = (gchar *)leaky_calloc (new_ui_info_sz, sizeof (gchar));
    }

  while (strlen (new_ui_info) + strlen (newone) + 1 > new_ui_info_sz)
    {
      size_t n;
      gchar * np;

      n = new_ui_info_sz + 1024;
      if ((np = (gchar *)leaky_realloc (new_ui_info, n)) == NULL)
	{
	  fprintf (stderr, "ghid_ui_info_append():  realloc of size %ld failed\n",
		   (long int) n);
	  exit (1);
	}
      new_ui_info = np;
      new_ui_info_sz = n;
    }

  p = new_ui_info + strlen (new_ui_info) ;
  while (*newone != '\0')
    {
      *p = *newone;
      p++;
      newone++;
    }
  
  *p = '\0';
}


static void
ghid_load_menus (void)
{
  char *filename;
  Resource *r = 0, *bir;
  char *home_pcbmenu;
  Resource *mr;
  int i;

  for (i = 0; i < sizeof (ghid_hotkey_actions) / sizeof (char *) ; i++)
    {
      ghid_hotkey_actions[i] = NULL;
    }
 
  /* homedir is set by the core */
  home_pcbmenu = NULL;
  if (homedir == NULL)
    {
      Message (_("Warning:  could not determine home directory\n"));
    }
  else
    {
      Message (_("Note:  home directory is \"%s\"\n"), homedir);
      home_pcbmenu = Concat (homedir, PCB_DIR_SEPARATOR_S, ".pcb",
                  PCB_DIR_SEPARATOR_S, "gpcb-menu.res", NULL);
    }

  if (access ("gpcb-menu.res", R_OK) == 0)
    filename = "gpcb-menu.res";
  else if (home_pcbmenu != NULL && (access (home_pcbmenu, R_OK) == 0) )
    filename = home_pcbmenu;
  else if (access (pcbmenu_path, R_OK) == 0)
    filename = pcbmenu_path;
  else
    filename = 0;

  bir = resource_parse (0, gpcb_menu_default);
  if (!bir)
    {
      fprintf (stderr, _("Error: internal menu resource didn't parse\n"));
      exit(1);
    }

  if (filename)
    {
      Message ("Loading menus from %s\n", filename);
      r = resource_parse (filename, 0);
    }

  if (home_pcbmenu != NULL) 
    {
       free (home_pcbmenu);
    }

  if (!r)
    {
      Message ("Using default menus\n");
      r = bir;
    }

  mr = resource_subres (r, "MainMenu");
  if (!mr)
    mr = resource_subres (bir, "MainMenu");
    
  if (mr)
    {
      ghid_ui_info_append ("<ui>\n");
      ghid_ui_info_indent (INDENT_INC);
      ghid_ui_info_append ("<menubar name='MenuBar'>\n");
      add_resource_to_menu ("Initial Call", mr, 0, 2*INDENT_INC);
      ghid_ui_info_indent (INDENT_INC);
      ghid_ui_info_append ("</menubar>\n");
    }

  mr = resource_subres (r, "PopupMenus");
  if (!mr)
    mr = resource_subres (bir, "PopupMenus");
   
  if (mr)
    {
      int i;

      for (i = 0; i < mr->c; i++)
	{
	  if (resource_type (mr->v[i]) == 101)
	    {
	      /* This is a named resource which defines a popup menu */
	      ghid_ui_info_indent (INDENT_INC);
	      ghid_ui_info_append ("<popup name='");
	      ghid_ui_info_append (mr->v[i].name);
	      ghid_ui_info_append ("'>\n");
	      add_resource_to_menu ("Initial Call", mr->v[i].subres, 
				    0, 2*INDENT_INC);
	      ghid_ui_info_indent (INDENT_INC);
	      ghid_ui_info_append ("</popup>\n");
	    }
	  else
	    {
	    }
	}
    }

    ghid_ui_info_append ("</ui>\n");

#ifdef DEBUG_MENUS
      printf ("Finished loading menus.  ui_info = \n");
      printf ("%s\n", new_ui_info);
#endif

  mr = resource_subres (r, "Mouse");
  if (!mr)
    mr = resource_subres (bir, "Mouse");
  if (mr)
    load_mouse_resource (mr);

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

