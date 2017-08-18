/*!
 * \file src/hid/gtk/gui-top-window.c
 *
 * \brief This handles creation of the top level window and all its
 * widgets.
 *
 * \note Events for the Output.drawing_area widget are handled in a separate
 * file gui-output-events.c
 *
 * Some caveats with menu shorcut keys:  Some keys are trapped out by
 * Gtk and can't be used as shortcuts (eg. '|', TAB, etc).
 * For these cases we have our own shortcut table and capture the keys
 * and send the events there in ghid_port_key_press_cb().
 *
 * \todo Figure out when we need to call this everytime ?\n
 * <pre>
    ghid_set_status_line_label ();\n
 * </pre>
 *
 * \todo The old quit callback had:\n
 * <pre>
    ghid_config_files_write ();
    hid_action ("Quit");\n
 * </pre>
 *
 * \todo What about stuff like this:\n
    Set to ! because ActionDisplay toggles it
 * <pre>
    Settings.DrawGrid = !gtk_toggle_action_get_active (action);
    ghidgui->config_modified = TRUE;
    hid_actionl ("Display", "Grid", "", NULL);
    ghid_set_status_line_label ();
 * </pre>
 *
 * \todo We need to do the status line thing.
 * For example shift-alt-v to change the via size.
 * Note: The status line label does not get updated properly until
 * a zoom in/out.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * This file was originally written by Bill Wilson for the PCB Gtk
 * port. It was later heavily modified by Dan McMahill to provide
 * user customized menus.
 */

/* #define DEBUG_MENUS */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>

#include "ghid-layer-selector.h"
#include "ghid-route-style-selector.h"
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
#include "gui-trackball.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

static bool ignore_layer_update;

static GtkWidget *ghid_load_menus (void);

GhidGui _ghidgui, *ghidgui = NULL;

GHidPort ghid_port, *gport;

static gchar *bg_image_file;

static struct { GtkAction *action; const Resource *node; }
  ghid_hotkey_actions[256];
#define N_HOTKEY_ACTIONS \
        (sizeof (ghid_hotkey_actions) / sizeof (ghid_hotkey_actions[0]))


/*!
 * \brief Callback for ghid_main_menu_update_toggle_state ().
 */
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

/*!
 * \brief sync the menu checkboxes with actual pcb state.
 */
void
ghid_update_toggle_flags ()
{
  ghid_main_menu_update_toggle_state (GHID_MAIN_MENU (ghidgui->menu_bar),
                                      menu_toggle_update_cb);
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
  GtkAllocation allocation;
  gboolean new_w, new_h;

  gtk_widget_get_allocation (widget, &allocation);

  new_w = (ghidgui->top_window_width != allocation.width);
  new_h = (ghidgui->top_window_height != allocation.height);

  ghidgui->top_window_width = allocation.width;
  ghidgui->top_window_height = allocation.height;

  if (new_w || new_h)
    ghidgui->config_modified = TRUE;

  return FALSE;
}

static void
info_bar_response_cb (GtkInfoBar *info_bar,
                      gint        response_id,
                      GhidGui    *_gui)
{
  gtk_widget_destroy (_gui->info_bar);
  _gui->info_bar = NULL;

  if (response_id == GTK_RESPONSE_ACCEPT)
    RevertPCB ();
}

static void
close_file_modified_externally_prompt (void)
{
  if (ghidgui->info_bar != NULL)
    gtk_widget_destroy (ghidgui->info_bar);
  ghidgui->info_bar = NULL;
}

static void
show_file_modified_externally_prompt (void)
{
  GtkWidget *button;
  GtkWidget *button_image;
  GtkWidget *icon;
  GtkWidget *label;
  GtkWidget *content_area;
  char *file_path_utf8;
  char *secondary_text;
  char *markup;

  close_file_modified_externally_prompt ();

  ghidgui->info_bar = gtk_info_bar_new ();

  button = gtk_info_bar_add_button (GTK_INFO_BAR (ghidgui->info_bar),
                                    _("Reload"),
                                    GTK_RESPONSE_ACCEPT);
  button_image = gtk_image_new_from_stock (GTK_STOCK_REFRESH,
                                           GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), button_image);

  gtk_info_bar_add_button (GTK_INFO_BAR (ghidgui->info_bar),
                           GTK_STOCK_CANCEL,
                           GTK_RESPONSE_CANCEL);
  gtk_info_bar_set_message_type (GTK_INFO_BAR (ghidgui->info_bar),
                                 GTK_MESSAGE_WARNING);
  gtk_box_pack_start (GTK_BOX (ghidgui->vbox_middle),
                      ghidgui->info_bar, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (ghidgui->vbox_middle), ghidgui->info_bar, 0);


  g_signal_connect (ghidgui->info_bar, "response",
                    G_CALLBACK (info_bar_response_cb), ghidgui);

  file_path_utf8 = g_filename_to_utf8 (PCB->Filename, -1, NULL, NULL, NULL);

  secondary_text = PCB->Changed ? _("Do you want to drop your changes and reload the file?") :
                                  _("Do you want to reload the file?");

  markup =  g_markup_printf_escaped (_("<b>The file %s has changed on disk</b>\n\n%s"),
                                     file_path_utf8, secondary_text);
  g_free (file_path_utf8);

  content_area = gtk_info_bar_get_content_area (GTK_INFO_BAR (ghidgui->info_bar));

  icon = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING,
                                   GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (content_area),
                      icon, FALSE, FALSE, 0);

  label = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (content_area),
                      label, TRUE, TRUE, 6);

  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);

  gtk_misc_set_alignment (GTK_MISC (label), 0., 0.5);

  gtk_widget_show_all (ghidgui->info_bar);
}

static bool
check_externally_modified (void)
{
  GFile *file;
  GFileInfo *info;
  GTimeVal timeval;

  /* Treat zero time as a flag to indicate we've not got an mtime yet */
  if (PCB->Filename == NULL ||
      (ghidgui->our_mtime.tv_sec == 0 &&
       ghidgui->our_mtime.tv_usec == 0))
    return false;

  file = g_file_new_for_path (PCB->Filename);
  info = g_file_query_info (file, G_FILE_ATTRIBUTE_TIME_MODIFIED,
                            G_FILE_QUERY_INFO_NONE, NULL, NULL);
  g_object_unref (file);

  if (info == NULL ||
      !g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_TIME_MODIFIED))
    return false;

  g_file_info_get_modification_time (info, &timeval); //&ghidgui->last_seen_mtime);
  g_object_unref (info);

  /* Ignore when the file on disk is the same age as when we last looked */
  if (timeval.tv_sec == ghidgui->last_seen_mtime.tv_sec &&
      timeval.tv_usec == ghidgui->last_seen_mtime.tv_usec)
    return false;

  ghidgui->last_seen_mtime = timeval;

  return (ghidgui->last_seen_mtime.tv_sec > ghidgui->our_mtime.tv_sec) ||
         (ghidgui->last_seen_mtime.tv_sec == ghidgui->our_mtime.tv_sec &&
         ghidgui->last_seen_mtime.tv_usec > ghidgui->our_mtime.tv_usec);
}

static gboolean
top_window_enter_cb (GtkWidget *widget, GdkEvent  *event, GHidPort *port)
{
  if (check_externally_modified ())
    show_file_modified_externally_prompt ();

  return FALSE;
}

/*!
 * \brief Menu action callback function.
 *
 * This is the main menu callback function.  The callback receives
 * the original Resource pointer containing the HID actions to be
 * executed.
 *
 * All hotkeys go through the menus which means they go through here.
 * Some, such as tab, are caught by Gtk instead of passed here, so
 * pcb calls this function directly through ghid_hotkey_cb() for them.
 *
 * \param [in]   The action that was activated.
 * \param [in]   The menu resource associated with the action.
 */

static void
ghid_menu_cb (GtkAction *action, const Resource *node)
{
  int i;

  if (action == NULL || node == NULL) 
    return;

  for (i = 1; i < node->c; i++)
    if (resource_type (node->v[i]) == 10)
      {
#ifdef DEBUG_MENUS
        printf ("    %s\n", node->v[i].value);
#endif
        hid_parse_actions (node->v[i].value);
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

/*!
 * \brief Accelerator callback for accelerators gtk tries to hide from
 * us.
 */
void ghid_hotkey_cb (int which)
{
  if (ghid_hotkey_actions[which].action != NULL)
    ghid_menu_cb (ghid_hotkey_actions[which].action,
                  (gpointer) ghid_hotkey_actions[which].node);
}

static void
update_board_mtime_from_disk (void)
{
  GFile *file;
  GFileInfo *info;

  ghidgui->our_mtime.tv_sec = 0;
  ghidgui->our_mtime.tv_usec = 0;
  ghidgui->last_seen_mtime = ghidgui->our_mtime;

  if (PCB->Filename == NULL)
    return;

  file = g_file_new_for_path (PCB->Filename);
  info = g_file_query_info (file, G_FILE_ATTRIBUTE_TIME_MODIFIED,
                            G_FILE_QUERY_INFO_NONE, NULL, NULL);
  g_object_unref (file);

  if (info == NULL ||
      !g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_TIME_MODIFIED))
    return;

  g_file_info_get_modification_time (info, &ghidgui->our_mtime);
  g_object_unref (info);

  ghidgui->last_seen_mtime = ghidgui->our_mtime;
}

/*!
 * \brief Sync toggle states that were saved with the layout and notify
 * the config code to update Settings values it manages.
 */
void
ghid_sync_with_new_layout (void)
{
  pcb_use_route_style (&PCB->RouteStyle[0]);
  ghid_route_style_selector_select_style
    (GHID_ROUTE_STYLE_SELECTOR (ghidgui->route_style_selector),
     &PCB->RouteStyle[0]);

  ghid_config_handle_units_changed ();

  ghid_window_set_name_label (PCB->Name);
  ghid_set_status_line_label ();
  close_file_modified_externally_prompt ();
  update_board_mtime_from_disk ();
}

void
ghid_notify_save_pcb (const char *filename, bool done)
{
  /* Do nothing if it is not the active PCB file that is being saved.
   */
  if (PCB->Filename == NULL || strcmp (filename, PCB->Filename) != 0)
    return;

  if (done)
    update_board_mtime_from_disk ();
}

void
ghid_notify_filename_changed (void)
{
  /* Pick up the mtime of the new PCB file */
  update_board_mtime_from_disk ();
}

/*!
 * \brief Takes the index into the layers and produces the text string
 * for the layer and if the layer is currently visible or not.
 *
 * This is used by a couple of functions.
 *
 */
void
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
      if (Settings.ShowBottomSide)
         *text = (char *)UNKNOWN (PCB->Data->Layer[bottom_silk_layer].Name);
      else
         *text = (char *)UNKNOWN (PCB->Data->Layer[top_silk_layer].Name);
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

/*!
 * \brief Callback for GHidLayerSelector layer selection.
 */
static void
layer_selector_select_callback (GHidLayerSelector *ls, int layer, gpointer d)
{
  ignore_layer_update = true;
  /* Select Layer */
  PCB->SilkActive = (layer == LAYER_BUTTON_SILK);
  PCB->RatDraw  = (layer == LAYER_BUTTON_RATS);
  if (layer == LAYER_BUTTON_SILK)
    {
      PCB->ElementOn = true;
      hid_action ("LayersChanged");
    }
  else if (layer == LAYER_BUTTON_RATS)
    {
      PCB->RatOn = true;
      hid_action ("LayersChanged");
    }
  else if (layer < max_copper_layer)
    ChangeGroupVisibility (layer, TRUE, true);

  ignore_layer_update = false;

  ghid_invalidate_all ();
}

/*!
 * \brief Callback for GHidLayerSelector layer renaming.
 */
static void
layer_selector_rename_callback (GHidLayerSelector *ls,
                                int layer_id,
                                char *new_name,
                                void *userdata)
{
  LayerType *layer = LAYER_PTR (layer_id);

  /* Check for a legal layer name - for now, allow anything non-empty */
  if (new_name[0] == '\0')
    return;

  /* Don't bother if the name is identical to the current one */
  if (strcmp (layer->Name, new_name) == 0)
    return;

  free (layer->Name);
  layer->Name = strdup (new_name);
  ghid_layer_buttons_update ();
  if (!PCB->Changed)
    {
      SetChangedFlag (true);
      ghid_window_set_name_label (PCB->Name);
    }
}

/*!
 * \brief Callback for GHidLayerSelector layer toggling.
 */
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

  /* Select the next visible layer. (If there is none, this will
   * select the currently-selected layer, triggering the selection
   * callback, which will turn the visibility on.) This way we
   * will never have an invisible layer selected.
   */
  if (!active)
    ghid_layer_selector_select_next_visible (ls);

  ignore_layer_update = false;

  if (redraw)
    ghid_invalidate_all();
}

/*!
 * \brief Install menu bar and accelerator groups.
 */
void
ghid_install_accel_groups (GtkWindow *window, GhidGui *gui)
{
  gtk_window_add_accel_group
    (window, ghid_main_menu_get_accel_group
               (GHID_MAIN_MENU (gui->menu_bar)));
  gtk_window_add_accel_group
    (window, ghid_layer_selector_get_accel_group
               (GHID_LAYER_SELECTOR (gui->layer_selector)));
  gtk_window_add_accel_group
    (window, ghid_route_style_selector_get_accel_group
               (GHID_ROUTE_STYLE_SELECTOR (gui->route_style_selector)));
}

/*!
 * \brief Remove menu bar and accelerator groups.
 */
void
ghid_remove_accel_groups (GtkWindow *window, GhidGui *gui)
{
  gtk_window_remove_accel_group
    (window, ghid_main_menu_get_accel_group
               (GHID_MAIN_MENU (gui->menu_bar)));
  gtk_window_remove_accel_group
    (window, ghid_layer_selector_get_accel_group
               (GHID_LAYER_SELECTOR (gui->layer_selector)));
  gtk_window_remove_accel_group
    (window, ghid_route_style_selector_get_accel_group
               (GHID_ROUTE_STYLE_SELECTOR (gui->route_style_selector)));
}

/*!
 * \brief Refreshes the window title bar and sets the PCB name to the
 * window title bar or to a seperate label.
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

/*!
 * \brief The two following callbacks are used to keep the absolute
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
  GtkWidget *frame, *label;

  /* The grid units button next to the cursor position labels.
   */
  ghidgui->grid_units_button = gtk_button_new ();
  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
			Settings.grid_unit->in_suffix);
  ghidgui->grid_units_label = label;
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_container_add (GTK_CONTAINER (ghidgui->grid_units_button), label);
  gtk_box_pack_end (GTK_BOX (hbox), ghidgui->grid_units_button, FALSE, TRUE, 0);
  g_signal_connect (ghidgui->grid_units_button, "clicked",
                    G_CALLBACK (grid_units_button_cb), NULL);

  /* The absolute cursor position label
   */
  frame = gtk_frame_new (NULL);
  gtk_box_pack_end (GTK_BOX (hbox), frame, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);
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
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);
  label = gtk_label_new (" __.__  __.__ ");
  gtk_container_add (GTK_CONTAINER (frame), label);
  ghidgui->cursor_position_relative_label = label;
  g_signal_connect (G_OBJECT (label), "size-request",
		    G_CALLBACK (relative_label_size_req_cb), NULL);

}

/*!
 * \brief Add "virtual layers" to a layer selector.
 */
static void
make_virtual_layer_buttons (GtkWidget *layer_selector)
{
  GHidLayerSelector *layersel = GHID_LAYER_SELECTOR (layer_selector);
  gchar *text;
  gchar *color_string;
  gboolean active;
 
  layer_process (&color_string, &text, &active, LAYER_BUTTON_SILK);
  ghid_layer_selector_add_layer (layersel, LAYER_BUTTON_SILK,
                                 text, color_string, active, TRUE, FALSE);
  layer_process (&color_string, &text, &active, LAYER_BUTTON_RATS);
  ghid_layer_selector_add_layer (layersel, LAYER_BUTTON_RATS,
                                 text, color_string, active, TRUE, FALSE);
  layer_process (&color_string, &text, &active, LAYER_BUTTON_PINS);
  ghid_layer_selector_add_layer (layersel, LAYER_BUTTON_PINS,
                                 text, color_string, active, FALSE, FALSE);
  layer_process (&color_string, &text, &active, LAYER_BUTTON_VIAS);
  ghid_layer_selector_add_layer (layersel, LAYER_BUTTON_VIAS,
                                 text, color_string, active, FALSE, FALSE);
  layer_process (&color_string, &text, &active, LAYER_BUTTON_FARSIDE);
  ghid_layer_selector_add_layer (layersel, LAYER_BUTTON_FARSIDE,
                                 text, color_string, active, FALSE, FALSE);
  layer_process (&color_string, &text, &active, LAYER_BUTTON_MASK);
  ghid_layer_selector_add_layer (layersel, LAYER_BUTTON_MASK,
                                 text, color_string, active, FALSE, FALSE);
}

/*!
 * \brief Callback for ghid_layer_selector_update_colors.
 */
const gchar *
get_layer_color (gint layer)
{
  gchar *rv;
  layer_process (&rv, NULL, NULL, layer);
  return rv;
}

/*!
 * \brief Update a layer selector's color scheme.
 */
void
ghid_layer_buttons_color_update (void)
{
  ghid_layer_selector_update_colors
    (GHID_LAYER_SELECTOR (ghidgui->layer_selector), get_layer_color);
  pcb_colors_from_settings (PCB);
}
 
/*!
 * \brief Populate a layer selector with all layers Gtk is aware of.
 */
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
                                     text, color_string, active, TRUE, TRUE);
    }
}


/*!
 * \brief Callback for ghid_layer_selector_delete_layers.
 */
gboolean
get_layer_delete (gint layer)
{
  return layer >= max_copper_layer;
}

/*!
 * \brief Synchronize layer selector widget with current PCB state.
 *
 * Called when user toggles layer visibility or changes drawing layer,
 * or when layer visibility is changed programatically.
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

/*!
 * \brief Called when user clicks OK on route style dialog.
 */
static void
route_styles_edited_cb (GHidRouteStyleSelector *rss, gboolean save,
                        gpointer data)
{
  if (save)
    {
      g_free (Settings.Routes);
      Settings.Routes = make_route_string (PCB->RouteStyle, NUM_STYLES);
      ghidgui->config_modified = TRUE;
      ghid_config_files_write ();
    }
  ghid_main_menu_install_route_style_selector
      (GHID_MAIN_MENU (ghidgui->menu_bar),
       GHID_ROUTE_STYLE_SELECTOR (ghidgui->route_style_selector));
}

/*!
 * \brief Called when a route style is selected.
 */
static void
route_style_changed_cb (GHidRouteStyleSelector *rss, RouteStyleType *rst,
                        gpointer data)
{
  pcb_use_route_style (rst);
  ghid_set_status_line_label();
}

/*!
 * \brief Configure the route style selector.
 */
void
make_route_style_buttons (GHidRouteStyleSelector *rss)
{
  int i;
  for (i = 0; i < NUM_STYLES; ++i)
    ghid_route_style_selector_add_route_style (rss, &PCB->RouteStyle[i]);
  g_signal_connect (G_OBJECT (rss), "select_style",
                    G_CALLBACK (route_style_changed_cb), NULL);
  g_signal_connect (G_OBJECT (rss), "style_edited",
                    G_CALLBACK (route_styles_edited_cb), NULL);
  ghid_main_menu_install_route_style_selector
      (GHID_MAIN_MENU (ghidgui->menu_bar),
       GHID_ROUTE_STYLE_SELECTOR (ghidgui->route_style_selector));
}

/*!
 * \brief Mode buttons.
 */
typedef struct
{
  GtkWidget *button;
  GtkWidget *toolbar_button;
  guint button_cb_id;
  guint toolbar_button_cb_id;
  gchar *name;
  gint mode;
  gchar **xpm;
  gchar *tooltip;
}
ModeButton;


static ModeButton mode_buttons[] = {
  {NULL, NULL, 0, 0, N_("via"), VIA_MODE, via,
    N_("create vias with <select mouse button>")},
  {NULL, NULL, 0, 0, N_("line"), LINE_MODE, line,
    N_("create a line segment, toggle draw modes with '/' or '.'")},
  {NULL, NULL, 0, 0, N_("arc"), ARC_MODE, arc,
    N_("create an arc segment")},
  {NULL, NULL, 0, 0, N_("text"), TEXT_MODE, text,
    N_("create a text")},
  {NULL, NULL, 0, 0, N_("rectangle"), RECTANGLE_MODE, rect,
    N_("create a filled rectangle")},
  {NULL, NULL, 0, 0, N_("polygon"), POLYGON_MODE, poly,
    N_("create a polygon, <shift>-P for closing the polygon")},
  {NULL, NULL, 0, 0, N_("polygonhole"), POLYGONHOLE_MODE, polyhole,
    N_("create a hole into an existing polygon")},
  {NULL, NULL, 0, 0, N_("buffer"), PASTEBUFFER_MODE, buf,
    N_("paste the selection from buffer into the layout")},
  {NULL, NULL, 0, 0, N_("remove"), REMOVE_MODE, del,
    N_("remove objects under the cursor")},
  {NULL, NULL, 0, 0, N_("rotate"), ROTATE_MODE, rot,
    N_("rotate a selection or object CCW, hold the <shift> key to rotate CW")},
  {NULL, NULL, 0, 0, N_("insertPoint"), INSERTPOINT_MODE, ins,
    N_("add points into existing lines and polygons")},
  {NULL, NULL, 0, 0, N_("thermal"), THERMAL_MODE, thrm,
    N_("create thermals with <select mouse button>, toggle thermal style with <Shift> <select mouse button>")},
  {NULL, NULL, 0, 0, N_("select"), ARROW_MODE, sel,
    N_("select, deselect or move objects or selections")},
  {NULL, NULL, 0, 0, N_("lock"), LOCK_MODE, lock,
    N_("lock or unlock an object")}
};

static gint n_mode_buttons = G_N_ELEMENTS (mode_buttons);

static void
do_set_mode (int mode)
{
  SetMode (mode);
  ghid_mode_cursor (mode);
  ghidgui->settings_mode = mode;
}

static void
mode_toolbar_button_toggled_cb (GtkToggleButton *button, ModeButton * mb)
{
  gboolean active = gtk_toggle_button_get_active (button);

  if (mb->button != NULL)
    {
      g_signal_handler_block (mb->button, mb->button_cb_id);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mb->button), active);
      g_signal_handler_unblock (mb->button, mb->button_cb_id);
    }

  if (active)
    do_set_mode (mb->mode);
}

static void
mode_button_toggled_cb (GtkWidget * widget, ModeButton * mb)
{
  gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  if (mb->toolbar_button != NULL)
    {
      g_signal_handler_block (mb->toolbar_button, mb->toolbar_button_cb_id);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mb->toolbar_button), active);
      g_signal_handler_unblock (mb->toolbar_button, mb->toolbar_button_cb_id);
    }

  if (active)
    do_set_mode (mb->mode);
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
          g_signal_handler_block (mb->button, mb->button_cb_id);
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mb->button), TRUE);
          g_signal_handler_unblock (mb->button, mb->button_cb_id);

          g_signal_handler_block (mb->toolbar_button, mb->toolbar_button_cb_id);
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mb->toolbar_button), TRUE);
          g_signal_handler_unblock (mb->toolbar_button, mb->toolbar_button_cb_id);
          break;
        }
    }
}

void
ghid_pack_mode_buttons (void)
{
  if (ghidgui->compact_vertical)
    {
      gtk_widget_hide (ghidgui->mode_buttons_frame);
      gtk_widget_show_all (ghidgui->mode_toolbar);
    }
  else
    {
      gtk_widget_hide (ghidgui->mode_toolbar);
      gtk_widget_show_all (ghidgui->mode_buttons_frame);
    }
}

static void
make_mode_buttons_and_toolbar (GtkWidget **mode_frame,
                               GtkWidget **mode_toolbar)
{
  GtkToolItem *tool_item;
  GtkWidget *vbox, *hbox = NULL;
  GtkWidget *image;
  GdkPixbuf *pixbuf;
  GSList *group = NULL;
  GSList *toolbar_group = NULL;
  ModeButton *mb;
  int i;

  *mode_toolbar = gtk_toolbar_new ();

  *mode_frame = gtk_frame_new (NULL);
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (*mode_frame), vbox);

  for (i = 0; i < n_mode_buttons; ++i)
    {
      mb = &mode_buttons[i];

      /* Create tool button for mode frame */
      mb->button = gtk_radio_button_new (group);
      gtk_widget_set_tooltip_text (mb->button, _(mb->tooltip));
      gtk_widget_set_name (mb->button, (mb->name));
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (mb->button));
      gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (mb->button), FALSE);

      /* Create tool button for toolbar */
      mb->toolbar_button = gtk_radio_button_new (toolbar_group);
      gtk_widget_set_tooltip_text (mb->toolbar_button, _(mb->tooltip));
      gtk_widget_set_name (mb->toolbar_button, (mb->name));
      toolbar_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (mb->toolbar_button));
      gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (mb->toolbar_button), FALSE);

      /* Pack mode-frame button into the frame */
      if ((i % ghidgui->n_mode_button_columns) == 0)
        {
          hbox = gtk_hbox_new (FALSE, 0);
          gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
        }
      gtk_box_pack_start (GTK_BOX (hbox), mb->button, FALSE, FALSE, 0);

      /* Create a container for the toolbar button and add that */
      tool_item = gtk_tool_item_new ();
      gtk_container_add (GTK_CONTAINER (tool_item), mb->toolbar_button);
      gtk_toolbar_insert (GTK_TOOLBAR (*mode_toolbar), tool_item, -1);

      /* Load the image for the button, create GtkImage widgets for both
       * the grid button and the toolbar button, then pack into the buttons
       */
      pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) mb->xpm);
      image = gtk_image_new_from_pixbuf (pixbuf);
      gtk_container_add (GTK_CONTAINER (mb->button), image);
      image = gtk_image_new_from_pixbuf (pixbuf);
      gtk_container_add (GTK_CONTAINER (mb->toolbar_button), image);
      g_object_unref (pixbuf);

      if (strcmp (mb->name, "select") == 0)
        {
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mb->button), TRUE);
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mb->toolbar_button), TRUE);
        }

      mb->button_cb_id =
        g_signal_connect (mb->button, "toggled",
                          G_CALLBACK (mode_button_toggled_cb), mb);
      mb->toolbar_button_cb_id =
        g_signal_connect (mb->toolbar_button, "toggled",
                          G_CALLBACK (mode_toolbar_button_toggled_cb), mb);
    }
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

static void
get_widget_styles (GtkStyle **menu_bar_style,
                   GtkStyle **tool_button_style,
                   GtkStyle **tool_button_label_style)
{
  GtkWidget *tool_button;
  GtkWidget *tool_button_label;
  GtkToolItem *tool_item;

  /* Build a tool item to extract the theme's styling for a toolbar button with text */
  tool_item = gtk_tool_item_new ();
  gtk_toolbar_insert (GTK_TOOLBAR (ghidgui->mode_toolbar), tool_item, 0);
  tool_button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (tool_item), tool_button);
  tool_button_label = gtk_label_new ("");
  gtk_container_add (GTK_CONTAINER (tool_button), tool_button_label);

  /* Grab the various styles we need */
  gtk_widget_ensure_style (ghidgui->menu_bar);
  *menu_bar_style = gtk_widget_get_style (ghidgui->menu_bar);

  gtk_widget_ensure_style (tool_button);
  *tool_button_style = gtk_widget_get_style (tool_button);

  gtk_widget_ensure_style (tool_button_label);
  *tool_button_label_style = gtk_widget_get_style (tool_button_label);

  gtk_widget_destroy (GTK_WIDGET (tool_item));
}

static void
do_fix_topbar_theming (void)
{
  GtkWidget *rel_pos_frame;
  GtkWidget *abs_pos_frame;
  GtkStyle *menu_bar_style;
  GtkStyle *tool_button_style;
  GtkStyle *tool_button_label_style;

  get_widget_styles (&menu_bar_style,
                     &tool_button_style,
                     &tool_button_label_style);

  /* Style the top bar background as if it were all a menu bar */
  gtk_widget_set_style (ghidgui->top_bar_background, menu_bar_style);

  /* Style the cursor position labels using the menu bar style as well.
   * If this turns out to cause problems with certain gtk themes, we may
   * need to grab the GtkStyle associated with an actual menu item to
   * get a text color to render with.
   */
  gtk_widget_set_style (ghidgui->cursor_position_relative_label, menu_bar_style);
  gtk_widget_set_style (ghidgui->cursor_position_absolute_label, menu_bar_style);

  /* Style the units button as if it were a toolbar button - hopefully
   * this isn't too ugly sitting on a background themed as a menu bar.
   * It is unlikely any theme defines colours for a GtkButton sitting on
   * a menu bar.
   */
  rel_pos_frame = gtk_widget_get_parent (ghidgui->cursor_position_relative_label);
  abs_pos_frame = gtk_widget_get_parent (ghidgui->cursor_position_absolute_label);
  gtk_widget_set_style (rel_pos_frame, menu_bar_style);
  gtk_widget_set_style (abs_pos_frame, menu_bar_style);
  gtk_widget_set_style (ghidgui->grid_units_button, tool_button_style);
  gtk_widget_set_style (ghidgui->grid_units_label, tool_button_label_style);
}

/*!
 * \brief Attempt to produce a conststent style for our extra menu-bar
 * items by copying aspects from the menu bar style set by the user's
 * GTK theme.
 *
 * Setup signal handlers to update our efforts if the user changes their
 * theme whilst we are running.
 */
static void
fix_topbar_theming (void)
{
  GtkSettings *settings;

  do_fix_topbar_theming ();

  settings = gtk_widget_get_settings (ghidgui->top_bar_background);
  g_signal_connect (settings, "notify::gtk-theme-name",
                    G_CALLBACK (do_fix_topbar_theming), NULL);
  g_signal_connect (settings, "notify::gtk-font-name",
                    G_CALLBACK (do_fix_topbar_theming), NULL);
}

/*!
 * \brief Create the top_window contents.
 *
 * The config settings should be loaded before this is called.
 */
static void
ghid_build_pcb_top_window (void)
{
  GtkWidget *window;
  GtkWidget *vbox_main, *hbox_middle, *hbox;
  GtkWidget *vbox, *frame;
  GtkWidget *label;
  /* FIXME: IFDEF HACK */
#ifdef ENABLE_GL
  GtkWidget *trackball;
#endif
  GHidPort *port = &ghid_port;
  GtkWidget *scrolled;

  window = gport->top_window;

  vbox_main = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox_main);

  /* -- Top control bar */
  ghidgui->top_bar_background = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox_main),
                      ghidgui->top_bar_background, FALSE, FALSE, 0);

  ghidgui->top_hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (ghidgui->top_bar_background),
                     ghidgui->top_hbox);

  /*
   * menu_hbox will be made insensitive when the gui needs
   * a modal button GetLocation button press.
   */
  ghidgui->menu_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (ghidgui->top_hbox), ghidgui->menu_hbox,
		      FALSE, FALSE, 0);

  ghidgui->menubar_toolbar_vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (ghidgui->menu_hbox),
                      ghidgui->menubar_toolbar_vbox, FALSE, FALSE, 0);

  /* Build layer menus */
  ghidgui->layer_selector = ghid_layer_selector_new ();
  make_layer_buttons (ghidgui->layer_selector);
  make_virtual_layer_buttons (ghidgui->layer_selector);
  g_signal_connect (G_OBJECT (ghidgui->layer_selector), "select-layer",
                    G_CALLBACK (layer_selector_select_callback),
                    NULL);
  g_signal_connect (G_OBJECT (ghidgui->layer_selector), "toggle-layer",
                    G_CALLBACK (layer_selector_toggle_callback),
                    NULL);
  g_signal_connect (G_OBJECT (ghidgui->layer_selector), "rename-layer",
                    G_CALLBACK (layer_selector_rename_callback),
                    NULL);
  /* Build main menu */
  ghidgui->menu_bar = ghid_load_menus ();
  gtk_box_pack_start (GTK_BOX (ghidgui->menubar_toolbar_vbox),
                      ghidgui->menu_bar, FALSE, FALSE, 0);

  make_mode_buttons_and_toolbar (&ghidgui->mode_buttons_frame,
                                 &ghidgui->mode_toolbar);
  gtk_box_pack_start (GTK_BOX (ghidgui->menubar_toolbar_vbox),
                      ghidgui->mode_toolbar, FALSE, FALSE, 0);


  ghidgui->position_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX(ghidgui->top_hbox),
                    ghidgui->position_hbox, TRUE, TRUE, 0);

  make_cursor_position_labels (ghidgui->position_hbox, port);

  hbox_middle = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox_main), hbox_middle, TRUE, TRUE, 0);

  fix_topbar_theming (); /* Must be called after toolbar is created */

  /* -- Left control bar */
  /*
   * This box will be made insensitive when the gui needs
   * a modal button GetLocation button press.
   */
  ghidgui->left_toolbar = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_middle),
                      ghidgui->left_toolbar, FALSE, FALSE, 0);

  vbox = ghid_scrolled_vbox (ghidgui->left_toolbar, &scrolled,
                             GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX(vbox), ghidgui->layer_selector,
                      FALSE, FALSE, 0);

  /* FIXME: IFDEF HACK */
#ifdef ENABLE_GL
  trackball = ghid_trackball_new ();
  g_signal_connect (trackball, "rotation-changed",
                    G_CALLBACK (ghid_port_rotate), NULL);
  g_signal_connect (trackball, "view-2d-changed",
                    G_CALLBACK (ghid_view_2d), NULL);
  gtk_box_pack_start (GTK_BOX(ghidgui->left_toolbar),
                      trackball, FALSE, FALSE, 0);
#endif

  /* ghidgui->mode_buttons_frame was created above in the call to
   * make_mode_buttons_and_toolbar (...);
   */
  gtk_box_pack_start (GTK_BOX (ghidgui->left_toolbar),
                      ghidgui->mode_buttons_frame, FALSE, FALSE, 0);

  frame = gtk_frame_new(NULL);
  gtk_box_pack_end (GTK_BOX (ghidgui->left_toolbar), frame, FALSE, FALSE, 0);
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX (vbox), hbox, FALSE, FALSE, 1);
  ghidgui->route_style_selector = ghid_route_style_selector_new ();
  make_route_style_buttons
    (GHID_ROUTE_STYLE_SELECTOR (ghidgui->route_style_selector));
  gtk_box_pack_start(GTK_BOX(hbox), ghidgui->route_style_selector,
                     FALSE, FALSE, 0);

  ghidgui->vbox_middle = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_middle),
                      ghidgui->vbox_middle, TRUE, TRUE, 0);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (ghidgui->vbox_middle), hbox, TRUE, TRUE, 0);

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
  gtk_widget_set_can_focus (gport->drawing_area, TRUE);

  gtk_box_pack_start (GTK_BOX (hbox), gport->drawing_area, TRUE, TRUE, 0);

  ghidgui->v_adjustment = gtk_adjustment_new (0.0, 0.0, 100.0,
					      10.0, 10.0, 10.0);
  ghidgui->v_range =
    gtk_vscrollbar_new (GTK_ADJUSTMENT (ghidgui->v_adjustment));

  gtk_box_pack_start (GTK_BOX (hbox), ghidgui->v_range, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (ghidgui->v_adjustment), "value_changed",
		    G_CALLBACK (v_adjustment_changed_cb), ghidgui);

  ghidgui->h_adjustment = gtk_adjustment_new (0.0, 0.0, 100.0,
					      10.0, 10.0, 10.0);
  ghidgui->h_range =
    gtk_hscrollbar_new (GTK_ADJUSTMENT (ghidgui->h_adjustment));
  gtk_box_pack_start (GTK_BOX (ghidgui->vbox_middle),
                      ghidgui->h_range, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (ghidgui->h_adjustment), "value_changed",
		    G_CALLBACK (h_adjustment_changed_cb), ghidgui);

  /* -- The bottom status line label */
  ghidgui->status_line_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (ghidgui->vbox_middle),
                      ghidgui->status_line_hbox, FALSE, FALSE, 0);

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
  g_signal_connect (gport->top_window, "enter-notify-event",
                    G_CALLBACK (top_window_enter_cb), port);
  g_signal_connect (G_OBJECT (gport->drawing_area), "configure_event",
		    G_CALLBACK (ghid_port_drawing_area_configure_event_cb),
		    port);


  /* Mouse and key events will need to be intercepted when PCB needs a
     |  location from the user.
   */

  ghid_interface_input_signals_connect ();

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
  ghid_pack_mode_buttons ();
  gdk_window_set_back_pixmap (gtk_widget_get_window (gport->drawing_area),
                              NULL, FALSE);

  port->tooltip_update_timeout_id = 0;
}


  /* Connect and disconnect just the signals a g_main_loop() will need.
     |  Cursor and motion events still need to be handled by the top level
     |  loop, so don't connect/reconnect these.
     |  A g_main_loop will be running when PCB wants the user to select a
     |  location or if command entry is needed in the status line hbox.
     |  During these times normal button/key presses are intercepted, either
     |  by new signal handlers or the command_combo_box entry.
   */
static gulong button_press_handler;
static gulong button_release_handler;
static gulong scroll_event_handler;
static gulong key_press_handler;
static gulong key_release_handler;

/*!
 * \brief Connect just the signals a g_main_loop() will need.
 *
 * Cursor and motion events still need to be handled by the top level
 * loop, so don't connect/reconnect these.
 * A g_main_loop will be running when PCB wants the user to select a
 * location or if command entry is needed in the status line hbox.
 * During these times normal button/key presses are intercepted, either
 * by new signal handlers or the command_combo_box entry.
 */
void
ghid_interface_input_signals_connect (void)
{
  button_press_handler =
    g_signal_connect (G_OBJECT (gport->drawing_area), "button_press_event",
		      G_CALLBACK (ghid_port_button_press_cb), NULL);

  button_release_handler =
    g_signal_connect (G_OBJECT (gport->drawing_area), "button_release_event",
		      G_CALLBACK (ghid_port_button_release_cb), NULL);

  scroll_event_handler =
    g_signal_connect (G_OBJECT (gport->drawing_area), "scroll_event",
		    G_CALLBACK (ghid_port_window_mouse_scroll_cb), NULL);

  key_press_handler =
    g_signal_connect (G_OBJECT (gport->drawing_area), "key_press_event",
		      G_CALLBACK (ghid_port_key_press_cb), NULL);

  key_release_handler =
    g_signal_connect (G_OBJECT (gport->drawing_area), "key_release_event",
		      G_CALLBACK (ghid_port_key_release_cb), NULL);
}

/*!
 * \brief Disconnect just the signals a g_main_loop() will need.
 *
 * Cursor and motion events still need to be handled by the top level
 * loop, so don't disconnect these.
 * A g_main_loop will be running when PCB wants the user to select a
 * location or if command entry is needed in the status line hbox.
 * During these times normal button/key presses are intercepted, either
 * by new signal handlers or the command_combo_box entry.
 */
void
ghid_interface_input_signals_disconnect (void)
{
  if (button_press_handler)
    g_signal_handler_disconnect (gport->drawing_area, button_press_handler);

  if (button_release_handler)
    g_signal_handler_disconnect (gport->drawing_area, button_release_handler);

  if (scroll_event_handler)
    g_signal_handler_disconnect (gport->drawing_area, scroll_event_handler);

  if (key_press_handler)
    g_signal_handler_disconnect (gport->drawing_area, key_press_handler);

  if (key_release_handler)
    g_signal_handler_disconnect (gport->drawing_area, key_release_handler);

  button_press_handler = 0;
  button_release_handler = 0;
  scroll_event_handler = 0;
  key_press_handler = 0;
  key_release_handler = 0;
}


/*!
 * \brief We'll set the interface insensitive when a g_main_loop is
 * running so the Gtk menus and buttons don't respond and interfere with
 * the special entry the user needs to be doing.
 */
void
ghid_interface_set_sensitive (gboolean sensitive)
{
  gtk_widget_set_sensitive (ghidgui->left_toolbar, sensitive);
  gtk_widget_set_sensitive (ghidgui->menu_hbox, sensitive);
}


/*!
 * \brief Initializes icon pixmap and also cursor bit maps.
 */
static void
ghid_init_icons (GHidPort * port)
{
  GdkWindow *window = gtk_widget_get_window (gport->top_window);

  XC_clock_source = gdk_bitmap_create_from_data (window,
						 (char *) rotateIcon_bits,
						 rotateIcon_width,
						 rotateIcon_height);
  XC_clock_mask =
    gdk_bitmap_create_from_data (window, (char *) rotateMask_bits,
				 rotateMask_width, rotateMask_height);

  XC_hand_source = gdk_bitmap_create_from_data (window,
						(char *) handIcon_bits,
						handIcon_width,
						handIcon_height);
  XC_hand_mask =
    gdk_bitmap_create_from_data (window, (char *) handMask_bits,
				 handMask_width, handMask_height);

  XC_lock_source = gdk_bitmap_create_from_data (window,
						(char *) lockIcon_bits,
						lockIcon_width,
						lockIcon_height);
  XC_lock_mask =
    gdk_bitmap_create_from_data (window, (char *) lockMask_bits,
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
  ghid_install_accel_groups (GTK_WINDOW (port->top_window), ghidgui);
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
      gui->log (_("Read end of pipe died!\n"));
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
	  gui->log (_("ERROR status from g_io_channel_read_line\n"));
	  return FALSE;
	  break;

	case G_IO_STATUS_EOF:
	  gui->log (_("Input pipe returned EOF.  The --listen option is \n"
		    "probably not running anymore in this session.\n"));
	  return FALSE;
	  break;

	case G_IO_STATUS_AGAIN:
	  gui->log (_("AGAIN status from g_io_channel_read_line\n"));
	  return FALSE;
	  break;

	default:
	  fprintf (stderr, _("ERROR:  unhandled case in ghid_listener_cb\n"));
	  return FALSE;
	  break;
	}

    }
  else
    fprintf (stderr, _("Unknown condition in ghid_listener_cb\n"));
  
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
  {"listen", N_("Listen for actions on stdin"),
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
  {"bg-image", N_("Background Image"),
   HID_String, 0, 0, {0, 0, 0}, 0, &bg_image_file},
#define HA_bg_image 1

/* %start-doc options "21 GTK+ GUI Options"
@ftable @code
@item --pcb-menu <string>
Location of the @file{gpcb-menu.res} file which defines the menu for the GTK+ GUI.
@end ftable
%end-doc
*/
{"pcb-menu", N_("Location of gpcb-menu.res file"),
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

/*!
 * \brief Create top level window for routines that will need top_window
 * before ghid_create_pcb_widgets() is called.
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
  tmps = g_win32_get_package_installation_directory_of_module (NULL);
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

  /*
   * Prevent gtk_init() and gtk_init_check() from automatically calling
   * setlocale (LC_ALL, "") which would undo LC_NUMERIC handling in main().
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

  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
  bind_textdomain_codeset (PACKAGE, "UTF-8");

  icon = gdk_pixbuf_new_from_xpm_data ((const gchar **) icon_bits);
  gtk_window_set_default_icon (icon);

  window = gport->top_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "PCB");
  gtk_window_resize (GTK_WINDOW(window),
                     ghidgui->top_window_width, ghidgui->top_window_height);

  if (Settings.AutoPlace)
    gtk_window_move (GTK_WINDOW (window), 10, 10);

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
  ghid_main_menu_install_route_style_selector
      (GHID_MAIN_MENU (ghidgui->menu_bar),
       GHID_ROUTE_STYLE_SELECTOR (ghidgui->route_style_selector));

  if (stdin_listen)
    ghid_create_listener ();

  ghid_notify_gui_is_up ();

  gtk_main ();
  ghid_config_files_write ();

}

/*!
 * \brief callback for.
 */
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

  /*! \todo If a layer is moved it should retain its color. But layers
   * currently can't do that because color info is not saved in the
   * pcb file.  So this makes a moved layer change its color to reflect
   * the way it will be when the pcb is reloaded.
   */
  pcb_colors_from_settings (PCB);
  return 0;
}

static const char toggleview_syntax[] =
    N_("ToggleView(1..MAXLAYER)\n"
       "ToggleView(layername)\n"
       "ToggleView(Silk|Rats|Pins|Vias|Mask|BackSide)");

static const char toggleview_help[] =
    N_("Toggle the visibility of the specified layer or layer group.");

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
      for (i = 0; i < max_copper_layer + SILK_LAYER; i++)
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
    N_("SelectLayer(1..MAXLAYER|Silk|Rats)");

static const char selectlayer_help[] =
    N_("Select which layer is the current layer.");

/* %start-doc actions SelectLayer

The specified layer becomes the currently active layer.  It is made
visible if it is not already visible

%end-doc */

static int
SelectLayer (int argc, char **argv, Coord x, Coord y)
{
  int i;
  int newl = -1;
  if (argc == 0)
    AFAIL (selectlayer);

  for (i = 0; i < max_copper_layer; ++i)
    if (strcasecmp (argv[0], PCB->Data->Layer[i].Name) == 0)
      newl = i;

  if (strcasecmp (argv[0], "silk") == 0)
    newl = LAYER_BUTTON_SILK;
  else if (strcasecmp (argv[0], "rats") == 0)
    newl = LAYER_BUTTON_RATS;
  else if (newl == -1)
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

/*!
 * \brief This function is used to check if a specified hotkey in the
 * menu resource file is "special".
 *
 * In this case "special" means that gtk assigns a particular meaning to
 * it and the normal menu setup will never see these key presses.
 * We capture those and feed them back into the menu callbacks.
 * This function is called as new accelerators are added when the menus
 * are being built.
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
	  fprintf (stderr, _("ERROR:  overflow of the ghid_hotkey_actions array.  Index = %d\n"
		   "Please report this.\n"), ind);
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

/*!
 * \brief Finds the gpcb-menu.res file.
 */
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
      Message (_("Loading menus from %s\n"), filename);
      r = resource_parse (filename, 0);
    }

  if (!r)
    {
      Message (_("Using default menus\n"));
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
    N_("AdjustStyle()\n");

static const char adjuststyle_help[] =
    N_("Open the window which allows editing of the route styles.");

/* %start-doc actions AdjustStyle

Opens the window which allows editing of the route styles.

%end-doc */

static int
AdjustStyle(int argc, char **argv, Coord x, Coord y)
{
  ghid_route_style_selector_edit_dialog
    (GHID_ROUTE_STYLE_SELECTOR (ghidgui->route_style_selector));

  return 0;
}

/* ------------------------------------------------------------ */

static const char editlayergroups_syntax[] =
    N_("EditLayerGroups()\n");

static const char editlayergroups_help[] =
    N_("Open the preferences window which allows editing of the layer groups.");

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

