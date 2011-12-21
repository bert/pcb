/*! \file <gtk-pcb-route-style-selector.c>
 *  \brief Implementation of GHidRouteStyleSelector widget
 *  \par Description
 *  Please write description here.
 */

#include <glib.h>
#include <glib-object.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "global.h"
#include "gtkhid.h"
#include "gui.h"
#include "pcb-printf.h"

#include "ghid-route-style-selector.h"

/* Forward dec'ls */
struct _route_style;
struct _route_style *ghid_route_style_selector_real_add_route_style
  (GHidRouteStyleSelector *, RouteStyleType *, gboolean temp);
static void ghid_route_style_selector_finalize (GObject *object);

/*! \brief Global action creation counter */
static gint action_count;

/*! \brief Signals exposed by the widget */
enum {
  SELECT_STYLE_SIGNAL,
  STYLE_EDITED_SIGNAL,
  LAST_SIGNAL
};

/*! \brief Columns used for internal data store */
enum {
  TEXT_COL,
  DATA_COL,
  N_COLS
};

static GtkVBox *ghid_route_style_selector_parent_class;
static guint ghid_route_style_selector_signals[LAST_SIGNAL] = { 0 };

struct _GHidRouteStyleSelector
{
  GtkVBox parent;

  GSList *button_radio_group;
  GSList *action_radio_group;
  GtkWidget *edit_button;

  GtkActionGroup *action_group;
  GtkAccelGroup *accel_group;
  GtkRadioAction *null_action;
  GtkWidget *null_button;
  gint shortcut_key_idx;

  GtkListStore *model;
  struct _route_style *active_style;
};

struct _GHidRouteStyleSelectorClass
{
  GtkVBoxClass parent_class;

  void (* select_style) (GHidRouteStyleSelector *, RouteStyleType *);
  void (* style_edited) (GHidRouteStyleSelector *, gboolean);
};

struct _route_style
{
  gboolean temporary;
  GtkRadioAction *action;
  GtkWidget *button;
  GtkWidget *menu_item;
  GtkTreeRowReference *rref;
  RouteStyleType *rst;
  gulong sig_id;
};

static void
init_radio_groups (GHidRouteStyleSelector *rss)
{
  gtk_radio_action_set_group (rss->null_action, rss->action_radio_group);
  rss->action_radio_group = gtk_radio_action_get_group (rss->null_action);
  gtk_radio_button_set_group (GTK_RADIO_BUTTON (rss->null_button),
                              rss->button_radio_group);
  rss->button_radio_group = gtk_radio_button_get_group
                              (GTK_RADIO_BUTTON (rss->null_button));
}

/* SIGNAL HANDLERS */
/*! \brief Callback for user selection of a route style */
static void
radio_select_cb (GtkToggleAction *action, GHidRouteStyleSelector *rss)
{
  rss->active_style = g_object_get_data (G_OBJECT (action), "route-style");
  if (gtk_toggle_action_get_active (action))
    g_signal_emit (rss, ghid_route_style_selector_signals[SELECT_STYLE_SIGNAL],
                   0, rss->active_style->rst);
}

/* EDIT DIALOG */
struct _dialog
{
  GHidRouteStyleSelector *rss;
  GtkWidget *name_entry;
  GtkWidget *line_entry;
  GtkWidget *via_hole_entry;
  GtkWidget *via_size_entry;
  GtkWidget *clearance_entry;
};

/*! \brief Callback for dialog box's combobox being changed
 *  \par Function Description
 *  When a different layer is selected, this function loads
 *  that layer's data into the dialog. Alternately, if the
 *  "New layer" option is selected, this loads a new name
 *  but no other data.
 *
 *  \param [in] combo   The combobox
 *  \param [in] dialog  The rest of the widgets to be updated
 */
static void
dialog_style_changed_cb (GtkComboBox *combo, struct _dialog *dialog)
{
  struct _route_style *style;
  GtkTreeIter iter;
  gtk_combo_box_get_active_iter (combo, &iter);
  gtk_tree_model_get (GTK_TREE_MODEL (dialog->rss->model),
                      &iter, DATA_COL, &style, -1);

  if (style == NULL)
    {
      gtk_entry_set_text (GTK_ENTRY (dialog->name_entry),
                          _("New Style"));
      return;
    }

  gtk_entry_set_text (GTK_ENTRY (dialog->name_entry), style->rst->Name);
  ghid_coord_entry_set_value (GHID_COORD_ENTRY (dialog->line_entry),
                              style->rst->Thick);
  ghid_coord_entry_set_value (GHID_COORD_ENTRY (dialog->via_hole_entry),
                              style->rst->Hole);
  ghid_coord_entry_set_value (GHID_COORD_ENTRY (dialog->via_size_entry),
                              style->rst->Diameter);
  ghid_coord_entry_set_value (GHID_COORD_ENTRY (dialog->clearance_entry),
                              style->rst->Keepaway);

}

/* \brief Helper for edit_button_cb */
static void
_table_attach (GtkWidget *table, gint row, const gchar *label,
               GtkWidget **entry, Coord min, Coord max)
{
  GtkWidget *label_w = gtk_label_new (label);
  gtk_misc_set_alignment (GTK_MISC (label_w), 1.0, 0.5);

  *entry = ghid_coord_entry_new (min, max, 0, Settings.grid_unit, CE_SMALL);
  gtk_table_attach (GTK_TABLE (table), label_w, 0, 1, row, row + 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
  gtk_table_attach (GTK_TABLE (table), *entry, 1, 2, row, row + 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
}

/* \brief Builds and runs the "edit route styles" dialog */
void
ghid_route_style_selector_edit_dialog (GHidRouteStyleSelector *rss)
{
  GtkTreePath *path;
  GtkTreeIter iter, tmp_iter;
  struct _dialog dialog_data;
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
  GtkWidget *window = gtk_widget_get_toplevel (GTK_WIDGET (rss));
  GtkWidget *dialog;
  GtkWidget *content_area;
  GtkWidget *vbox, *hbox, *sub_vbox, *table;
  GtkWidget *label, *select_box, *check_box;

  /* Build dialog */
  dialog = gtk_dialog_new_with_buttons (_("Edit Route Styles"),
                                        GTK_WINDOW (window),
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_NONE,
                                        GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

  label = gtk_label_new (_("Edit Style:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

  select_box = gtk_combo_box_new_with_model (GTK_TREE_MODEL (rss->model));
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (select_box), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(select_box), renderer,
                                  "text", TEXT_COL, NULL);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (content_area), hbox, TRUE, TRUE, 4);
  vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 4);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), select_box, TRUE, TRUE, 0);

  sub_vbox = ghid_category_vbox (vbox, _("Route Style Data"),
                                 4, 2, TRUE, TRUE);
  table = gtk_table_new (5, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (sub_vbox), table, TRUE, TRUE, 4);
  label = gtk_label_new (_("Name:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  dialog_data.name_entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
  gtk_table_attach (GTK_TABLE (table), dialog_data.name_entry, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);

  _table_attach (table, 1, _("Line width:"),
                 &dialog_data.line_entry,
                 MIN_LINESIZE, MAX_LINESIZE);
  _table_attach (table, 2, _("Via hole size:"),
                 &dialog_data.via_hole_entry,
                 MIN_PINORVIAHOLE, MAX_PINORVIASIZE - MIN_PINORVIACOPPER);
  _table_attach (table, 3, _("Via ring size:"),
                 &dialog_data.via_size_entry,
                 MIN_PINORVIAHOLE + MIN_PINORVIACOPPER, MAX_PINORVIASIZE);
  _table_attach (table, 4, _("Clearance:"),
                 &dialog_data.clearance_entry,
                 MIN_LINESIZE, MAX_LINESIZE);

  sub_vbox = ghid_category_vbox (vbox, _("Set as Default"),
                                 4, 2, TRUE, TRUE);
  check_box = gtk_check_button_new_with_label
                (_("Save route style settings as default"));
  gtk_box_pack_start (GTK_BOX (sub_vbox), check_box, TRUE, TRUE, 0);

  /* Add "new style" option to list */
  gtk_list_store_append (rss->model, &tmp_iter);
  gtk_list_store_set (rss->model, &tmp_iter, TEXT_COL,
                      _("New (this session only)"), DATA_COL, NULL, -1);

  /* Display dialog */
  dialog_data.rss = rss;
  path = gtk_tree_row_reference_get_path (rss->active_style->rref);
  gtk_tree_model_get_iter (GTK_TREE_MODEL (rss->model), &iter, path);
  g_signal_connect (G_OBJECT (select_box), "changed",
                    G_CALLBACK (dialog_style_changed_cb), &dialog_data);
  gtk_combo_box_set_active_iter (GTK_COMBO_BOX (select_box), &iter);
  gtk_widget_show_all (dialog);
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      RouteStyleType *rst;
      struct _route_style *style;
      gboolean save;
      gtk_combo_box_get_active_iter (GTK_COMBO_BOX (select_box), &iter);
      gtk_tree_model_get (GTK_TREE_MODEL (rss->model),
                          &iter, DATA_COL, &style, -1);
      if (style == NULL)
        rst = g_malloc (sizeof *rst);
      else
        {
          rst = style->rst;
          free (rst->Name);
        }

      rst->Name = StripWhiteSpaceAndDup
                    (gtk_entry_get_text (GTK_ENTRY (dialog_data.name_entry)));
      rst->Thick = ghid_coord_entry_get_value
                     (GHID_COORD_ENTRY (dialog_data.line_entry));
      rst->Hole = ghid_coord_entry_get_value
                    (GHID_COORD_ENTRY (dialog_data.via_hole_entry));
      rst->Diameter = ghid_coord_entry_get_value
                        (GHID_COORD_ENTRY (dialog_data.via_size_entry));
      rst->Keepaway = ghid_coord_entry_get_value
                        (GHID_COORD_ENTRY (dialog_data.clearance_entry));
      save = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_box));

      if (style == NULL)
        style = ghid_route_style_selector_real_add_route_style
                  (rss, rst, TRUE);
      else
        {
          gtk_action_set_label (GTK_ACTION (style->action), rst->Name); 
          gtk_list_store_set (rss->model, &iter, TEXT_COL, rst->Name, -1);
        }

      /* Cleanup */
      gtk_widget_destroy (dialog);
      gtk_list_store_remove (rss->model, &tmp_iter);
      /* Emit change signals */
      ghid_route_style_selector_select_style (rss, rst);
      g_signal_emit
        (rss, ghid_route_style_selector_signals[STYLE_EDITED_SIGNAL],
         0, save);
    }
  else
    {
      gtk_widget_destroy (dialog);
      gtk_list_store_remove (rss->model, &tmp_iter);
    }
}
/* end EDIT DIALOG */

static void
edit_button_cb (GtkButton *btn, GHidRouteStyleSelector *rss)
{
  ghid_route_style_selector_edit_dialog (rss);
}

/* CONSTRUCTOR */
static void
ghid_route_style_selector_init (GHidRouteStyleSelector *rss)
{
}

static void
ghid_route_style_selector_class_init (GHidRouteStyleSelectorClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  ghid_route_style_selector_parent_class = g_type_class_peek_parent (klass);

  ghid_route_style_selector_signals[SELECT_STYLE_SIGNAL] =
    g_signal_new ("select-style",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GHidRouteStyleSelectorClass, select_style),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE,
                  1, G_TYPE_POINTER);
  ghid_route_style_selector_signals[STYLE_EDITED_SIGNAL] =
    g_signal_new ("style-edited",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GHidRouteStyleSelectorClass, style_edited),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOOLEAN, G_TYPE_NONE,
                  1, G_TYPE_BOOLEAN);

  object_class->finalize = ghid_route_style_selector_finalize;
}

/*! \brief Clean up object before garbage collection
 */
static void
ghid_route_style_selector_finalize (GObject *object)
{
  GHidRouteStyleSelector *rss = (GHidRouteStyleSelector *) object;

  ghid_route_style_selector_empty (rss);

  g_object_unref (rss->accel_group);
  g_object_unref (rss->action_group);

  G_OBJECT_CLASS (ghid_route_style_selector_parent_class)->finalize (object);
}

/* PUBLIC FUNCTIONS */
GType
ghid_route_style_selector_get_type (void)
{
  static GType rss_type = 0;

  if (!rss_type)
    {
      const GTypeInfo rss_info =
      {
	sizeof (GHidRouteStyleSelectorClass),
	NULL, /* base_init */
	NULL, /* base_finalize */
	(GClassInitFunc) ghid_route_style_selector_class_init,
	NULL, /* class_finalize */
	NULL, /* class_data */
	sizeof (GHidRouteStyleSelector),
	0,    /* n_preallocs */
	(GInstanceInitFunc) ghid_route_style_selector_init,
      };

      rss_type = g_type_register_static (GTK_TYPE_VBOX,
                                         "GHidRouteStyleSelector",
                                         &rss_info,
                                         0);
    }

  return rss_type;
}

/*! \brief Create a new GHidRouteStyleSelector
 *
 *  \return a freshly-allocated GHidRouteStyleSelector.
 */
GtkWidget *
ghid_route_style_selector_new ()
{
  GHidRouteStyleSelector *rss =
    g_object_new (GHID_ROUTE_STYLE_SELECTOR_TYPE, NULL);

  rss->active_style = NULL;
  rss->action_radio_group = NULL;
  rss->button_radio_group = NULL;
  rss->model = gtk_list_store_new (N_COLS, G_TYPE_STRING, G_TYPE_POINTER);

  rss->accel_group = gtk_accel_group_new ();
  rss->action_group = gtk_action_group_new ("RouteStyleSelector");
  rss->null_action = gtk_radio_action_new ("RouteStyleXX", "",
                                           NULL, NULL, -1);
  rss->null_button = gtk_radio_button_new (rss->button_radio_group);
  init_radio_groups (rss);
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (rss->null_button),
                                      GTK_ACTION (rss->null_action));
  rss->shortcut_key_idx = 1;

  /* Create edit button */
  rss->edit_button = gtk_button_new_with_label (_("Route Styles"));
  g_signal_connect (G_OBJECT (rss->edit_button), "clicked",
                    G_CALLBACK (edit_button_cb), rss);
  gtk_box_pack_start (GTK_BOX (rss), rss->edit_button, FALSE, FALSE, 0);

  return GTK_WIDGET (rss);
}

/*! \brief Create a new GHidRouteStyleSelector 
 *
 *  \param [in] rss     The selector to be acted on
 *  \param [in] data    PCB's route style object that will be edited
 *  \param [in] temp    Whether the style is "real" or session-only
 */
struct _route_style *
ghid_route_style_selector_real_add_route_style (GHidRouteStyleSelector *rss,
                                                RouteStyleType *data,
                                                gboolean temp)
{
  GtkTreeIter iter;
  GtkTreePath *path;
  gchar *action_name = g_strdup_printf ("RouteStyle%d", action_count);
  struct _route_style *new_style = g_malloc (sizeof (*new_style));

  /* Key the route style data with the RouteStyleType it controls */
  new_style->rst = data;
  new_style->temporary = temp;
  new_style->action = gtk_radio_action_new (action_name, data->Name,
                                            NULL, NULL, action_count);
  gtk_radio_action_set_group (new_style->action, rss->action_radio_group);
  rss->action_radio_group = gtk_radio_action_get_group (new_style->action);
  new_style->button = gtk_radio_button_new (rss->button_radio_group);
  rss->button_radio_group = gtk_radio_button_get_group
                              (GTK_RADIO_BUTTON (new_style->button));
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (new_style->button),
                                      GTK_ACTION (new_style->action));

  gtk_list_store_append (rss->model, &iter);
  gtk_list_store_set (rss->model, &iter, TEXT_COL, data->Name,
                      DATA_COL, new_style, -1);
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (rss->model), &iter);
  new_style->rref = gtk_tree_row_reference_new (GTK_TREE_MODEL (rss->model),
                                                path);
  gtk_tree_path_free (path);

  /* Setup accelerator */
  if (rss->shortcut_key_idx < 12)
    {
      gchar *accel = g_strdup_printf ("<Ctrl>F%d", rss->shortcut_key_idx);
      gtk_action_set_accel_group (GTK_ACTION (new_style->action),
                                  rss->accel_group);
      gtk_action_group_add_action_with_accel (rss->action_group,
                                              GTK_ACTION (new_style->action),
                                              accel);
      g_free (accel);
      ++rss->shortcut_key_idx;
    }

  /* Hookup and install radio button */
  g_object_set_data (G_OBJECT (new_style->action), "route-style", new_style);
  new_style->sig_id = g_signal_connect (G_OBJECT (new_style->action),
                                        "activate",
                                        G_CALLBACK (radio_select_cb), rss);
  gtk_box_pack_start (GTK_BOX (rss), new_style->button, FALSE, FALSE, 0);

  g_free (action_name);
  ++action_count;
  return new_style;
}

/*! \brief Adds a route style to a GHidRouteStyleSelector
 *  \par Function Description
 *  Adds a new route style to be managed by this selector. Note
 *  that the route style object passed to this function will be
 *  updated directly.
 *
 *  \param [in] rss     The selector to be acted on
 *  \param [in] data    PCB's route style object describing the style
 */
void
ghid_route_style_selector_add_route_style (GHidRouteStyleSelector *rss,
                                           RouteStyleType *data)
{
  ghid_route_style_selector_real_add_route_style (rss, data, FALSE);
}


/*! \brief Install the "Route Style" menu items
 *  \par Function Description
 *  Takes a menu shell and installs menu items for route style selection in
 *  the shell, at the given position. Note that we aren't really guaranteed
 *  the ordering of these items, since our internal data structure is a hash
 *  table. This shouldn't be a problem.
 *
 *  \param [in] rss     The selector to be acted on
 *  \param [in] shell   The menu to install the items in
 *  \param [in] pos     The position in the menu to install items
 *
 *  \return the number of items installed
 */
gint
ghid_route_style_selector_install_items (GHidRouteStyleSelector *rss,
                                         GtkMenuShell *shell, gint pos)
{
  gint n = 0;
  GtkTreeIter iter;

  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (rss->model), &iter);
  do
    {
      GtkAction *action;
      struct _route_style *style;

      gtk_tree_model_get (GTK_TREE_MODEL (rss->model),
                          &iter, DATA_COL, &style, -1);
      action = GTK_ACTION (style->action);
      style->menu_item = gtk_action_create_menu_item (action);
      gtk_menu_shell_insert (shell, style->menu_item, pos + n);
      ++n;
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (rss->model), &iter));

  return n;
}

/*! \brief Selects a route style and emits a select-style signal
 *
 *  \param [in] rss  The selector to be acted on
 *  \param [in] rst  The style to select
 *
 *  \return TRUE if a style was selected, FALSE otherwise
 */
gboolean
ghid_route_style_selector_select_style (GHidRouteStyleSelector *rss,
                                        RouteStyleType *rst)
{
  GtkTreeIter iter;
  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (rss->model), &iter);
  do
    {
      struct _route_style *style;
      gtk_tree_model_get (GTK_TREE_MODEL (rss->model),
                          &iter, DATA_COL, &style, -1);
      if (style->rst == rst)
        {
          g_signal_handler_block (G_OBJECT (style->action), style->sig_id);
          gtk_toggle_action_set_active
            (GTK_TOGGLE_ACTION (style->action), TRUE);
          g_signal_handler_unblock (G_OBJECT (style->action), style->sig_id);
          rss->active_style = style;
          g_signal_emit
            (rss, ghid_route_style_selector_signals[SELECT_STYLE_SIGNAL],
             0, rss->active_style->rst);
          return TRUE;
        }
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (rss->model), &iter));

  return FALSE;
}

/*! \brief Returns the GtkAccelGroup of a route style selector
 *  \par Function Description
 *
 *  \param [in] rss            The selector to be acted on
 *
 *  \return the accel group of the selector
 */
GtkAccelGroup *
ghid_route_style_selector_get_accel_group (GHidRouteStyleSelector *rss)
{
  return rss->accel_group;
}

/*! \brief Sets a GHidRouteStyleSelector selection to given values
 *  \par Function Description
 *  Given the line thickness, via size and clearance values of a route
 *  style, this function selects a route style with the given values.
 *  If there is no such style registered with the selector, nothing
 *  will happen. This function does not emit any signals.
 *
 *  \param [in] rss       The selector to be acted on
 *  \param [in] Thick     Coord to match selection to
 *  \param [in] Hole      Coord to match selection to
 *  \param [in] Diameter  Coord to match selection to
 *  \param [in] Keepaway  Coord to match selection to
 */
void
ghid_route_style_selector_sync (GHidRouteStyleSelector *rss,
                                Coord Thick, Coord Hole,
                                Coord Diameter, Coord Keepaway)
{
  gboolean found_match = FALSE;
  GtkTreeIter iter;
  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (rss->model), &iter);
  do
    {
      struct _route_style *style;
      gtk_tree_model_get (GTK_TREE_MODEL (rss->model),
                          &iter, DATA_COL, &style, -1);
      if (style->rst->Thick == Thick &&
          style->rst->Hole == Hole &&
          style->rst->Diameter == Diameter &&
          style->rst->Keepaway == Keepaway)
        {
          g_signal_handler_block (G_OBJECT (style->action), style->sig_id);
          gtk_toggle_action_set_active
            (GTK_TOGGLE_ACTION (style->action), TRUE);
          g_signal_handler_unblock (G_OBJECT (style->action), style->sig_id);
          rss->active_style = style;
          found_match = TRUE;
        }
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (rss->model), &iter));

  if (!found_match)
    gtk_radio_action_set_current_value (rss->null_action, -1);
}

/*! \brief Removes all styles from a route style selector */
void
ghid_route_style_selector_empty (GHidRouteStyleSelector *rss)
{
  GtkTreeIter iter;
  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (rss->model), &iter);
  do
    {
      struct _route_style *rsdata;
      gtk_tree_model_get (GTK_TREE_MODEL (rss->model),
                          &iter, DATA_COL, &rsdata, -1);
      if (rsdata->action)
        {
          gtk_action_group_remove_action (rss->action_group,
                                GTK_ACTION (rsdata->action));
          g_object_unref (G_OBJECT (rsdata->action));
        }
      if (rsdata->button)
        gtk_widget_destroy (GTK_WIDGET (rsdata->button));
      if (rsdata->menu_item)
        gtk_widget_destroy (GTK_WIDGET (rsdata->menu_item));
      if (rsdata->temporary)
        {
          free (rsdata->rst->Name);
          g_free (rsdata->rst);
        }
      gtk_tree_row_reference_free (rsdata->rref);
      free (rsdata);
    }
  while (gtk_list_store_remove (rss->model, &iter));
  rss->action_radio_group = NULL;
  rss->button_radio_group = NULL;
  rss->shortcut_key_idx = 1;
  init_radio_groups (rss);
}

