/*! \file <gtk-pcb-layer-selector.c>
 *  \brief Implementation of GHidLayerSelector widget
 *  \par Description
 *  This widget is the layer selector on the left side of the Gtk
 *  GUI. It also describes (in XML) the relevant sections of the
 *  menu for layer selection and visibility toggling, and makes
 *  sure these stay in sync.
 */

#include <glib.h>
#include <glib-object.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "gtkhid.h"
#include "gui.h"
#include "pcb-printf.h"

#include "ghid-layer-selector.h"
#include "ghid-cell-renderer-visibility.h"

#define INITIAL_ACTION_MAX	40

/* Forward dec'ls */
static void ghid_layer_selector_finalize (GObject *object);

/*! \brief Signals exposed by the widget */
enum {
  SELECT_LAYER_SIGNAL,
  TOGGLE_LAYER_SIGNAL,
  LAST_SIGNAL
};

/*! \brief Columns used for internal data store */
enum {
  STRUCT_COL,
  USER_ID_COL,
  VISIBLE_COL,
  COLOR_COL,
  TEXT_COL,
  FONT_COL,
  ACTIVATABLE_COL,
  SEPARATOR_COL,
  N_COLS
};

static GtkTreeView *ghid_layer_selector_parent_class;
static guint ghid_layer_selector_signals[LAST_SIGNAL] = { 0 };

struct _GHidLayerSelector
{
  GtkTreeView parent;

  GtkListStore *list_store;
  GtkTreeSelection *selection;
  GtkTreeViewColumn *visibility_column;

  GtkActionGroup *action_group;

  GSList *radio_group;
  int n_actions;

  gboolean last_activatable;
  gboolean prevent_recursion;
};

struct _GHidLayerSelectorClass
{
  GtkTreeViewClass parent_class;

  void (* select_layer) (GHidLayerSelector *, gint);
  void (* toggle_layer) (GHidLayerSelector *, gint);
};

struct _layer
{
  GtkToggleAction *view_action;
  GtkRadioAction  *pick_action;
  GtkTreeRowReference *rref;
};

/*! \brief Flip the visibility state of a given layer 
 *  \par Function Description
 *  Changes the internal toggle state and menu checkbox state
 *  of the layer pointed to by iter. Emits a toggle-layer signal.
 *  ALL internal visibility-flipping needs to go through this
 *  function. Otherwise a signal will not be emitted and it is
 *  likely that pcb will become inconsistent with the selector.
 *
 *  \param [in] ls    The selector to be acted on
 *  \param [in] iter  A GtkTreeIter pointed at the relevant layer
 */
static void
toggle_visibility (GHidLayerSelector *ls, GtkTreeIter *iter)
{
  struct _layer *ldata;
  gboolean toggle;
  gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store), iter,
                     VISIBLE_COL, &toggle, STRUCT_COL, &ldata, -1);
  gtk_list_store_set (ls->list_store, iter, VISIBLE_COL, !toggle, -1);
  if (ldata)
    gtk_toggle_action_set_active (ldata->view_action, !toggle);
}

/*! \brief Decide if a GtkListStore entry is a layer or separator */
static gboolean
tree_view_separator_func (GtkTreeModel *model, GtkTreeIter *iter,
                          gpointer data)
{
  gboolean ret_val;
  gtk_tree_model_get (model, iter, SEPARATOR_COL, &ret_val, -1);
  return ret_val;
}

/*! \brief Decide if a GtkListStore entry may be selected */
static gboolean
tree_selection_func (GtkTreeSelection *selection, GtkTreeModel *model,
                     GtkTreePath *path, gboolean selected, gpointer data)
{
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter (model, &iter, path))
    {
      gboolean activatable;
      gtk_tree_model_get (model, &iter, ACTIVATABLE_COL, &activatable, -1);
      return activatable;
    }

  return FALSE;
}

/* SIGNAL HANDLERS */
/*! \brief Callback for mouse-click: toggle visibility */
static gboolean
button_press_cb (GHidLayerSelector *ls, GdkEventButton *event)
{
  /* Handle visibility independently to prevent changing the active
   *  layer, which will happen if we let this event propagate.  */
  GtkTreeViewColumn *column;
  GtkTreePath *path;
  if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (ls),
                                     event->x, event->y,
                                     &path, &column, NULL, NULL))
    {
      GtkTreeIter iter;
      gtk_tree_model_get_iter (GTK_TREE_MODEL (ls->list_store), &iter, path);
      if (column == ls->visibility_column)
        {
          toggle_visibility (ls, &iter);
          return TRUE; 
        }
    }
  return FALSE;
}

/*! \brief Callback for layer selection change: sync menu */
static void
selection_changed_cb (GtkTreeSelection *selection, GHidLayerSelector *ls)
{
  GtkTreeIter iter;
  ls->prevent_recursion = TRUE;
  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gint user_id;
      struct _layer *ldata;
      gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store), &iter,
                          STRUCT_COL, &ldata, USER_ID_COL, &user_id, -1);
      if (ldata && ldata->pick_action)
        gtk_radio_action_set_current_value (ldata->pick_action, user_id);
    }
  ls->prevent_recursion = FALSE;
}

/*! \brief Callback for menu actions: sync layer selection list, emit signal */
static void
menu_view_cb (GtkToggleAction *action, GtkTreeRowReference *rref)
{
  GHidLayerSelector *ls;
  GtkTreeModel *model = gtk_tree_row_reference_get_model (rref);
  GtkTreePath *path = gtk_tree_row_reference_get_path (rref);
  gboolean state = gtk_toggle_action_get_active (action);
  GtkTreeIter iter;
  gint user_id;

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, VISIBLE_COL, state, -1);
  gtk_tree_model_get (model, &iter, USER_ID_COL, &user_id, -1);

  ls = g_object_get_data (G_OBJECT (model), "layer-selector");
  g_signal_emit (ls, ghid_layer_selector_signals[TOGGLE_LAYER_SIGNAL],
                 0, user_id);
}

/*! \brief Callback for menu actions: sync layer selection list, emit signal */
static void
menu_pick_cb (GtkRadioAction *action, GtkTreeRowReference *rref)
{
  GHidLayerSelector *ls;
  GtkTreeModel *model = gtk_tree_row_reference_get_model (rref);
  GtkTreePath *path = gtk_tree_row_reference_get_path (rref);
  GtkTreeIter iter;
  gint user_id;

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, USER_ID_COL, &user_id, -1);

  ls = g_object_get_data (G_OBJECT (model), "layer-selector");
  if (!ls->prevent_recursion)
    gtk_tree_selection_select_path (ls->selection, path);
  g_signal_emit (ls, ghid_layer_selector_signals[SELECT_LAYER_SIGNAL],
                 0, user_id);
}

/* CONSTRUCTOR */
static void
ghid_layer_selector_init (GHidLayerSelector *ls)
{
  /* Hookup signal handlers */
}

static void
ghid_layer_selector_class_init (GHidLayerSelectorClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  ghid_layer_selector_signals[SELECT_LAYER_SIGNAL] =
    g_signal_new ("select-layer",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GHidLayerSelectorClass, select_layer),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT, G_TYPE_NONE,
                  1, G_TYPE_INT);
  ghid_layer_selector_signals[TOGGLE_LAYER_SIGNAL] =
    g_signal_new ("toggle-layer",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GHidLayerSelectorClass, toggle_layer),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT, G_TYPE_NONE,
                  1, G_TYPE_INT);

  object_class->finalize = ghid_layer_selector_finalize;
}

/*! \brief Clean up object before garbage collection
 */
static void
ghid_layer_selector_finalize (GObject *object)
{
  GtkTreeIter iter;
  GHidLayerSelector *ls = (GHidLayerSelector *) object;

  g_object_unref (ls->action_group);

  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (ls->list_store), &iter);
  do
    {
      struct _layer *ldata;
      gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store),
                          &iter, STRUCT_COL, &ldata, -1);

      g_object_unref (G_OBJECT (ldata->pick_action));
      g_object_unref (G_OBJECT (ldata->view_action));
      gtk_tree_row_reference_free (ldata->rref);
      g_free (ldata);
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (ls->list_store), &iter));

  G_OBJECT_CLASS (ghid_layer_selector_parent_class)->finalize (object);
}

/* PUBLIC FUNCTIONS */
GType
ghid_layer_selector_get_type (void)
{
  static GType ls_type = 0;

  if (!ls_type)
    {
      const GTypeInfo ls_info =
      {
	sizeof (GHidLayerSelectorClass),
	NULL, /* base_init */
	NULL, /* base_finalize */
	(GClassInitFunc) ghid_layer_selector_class_init,
	NULL, /* class_finalize */
	NULL, /* class_data */
	sizeof (GHidLayerSelector),
	0,    /* n_preallocs */
	(GInstanceInitFunc) ghid_layer_selector_init,
      };

      ls_type = g_type_register_static (GTK_TYPE_TREE_VIEW,
                                        "GHidLayerSelector",
                                        &ls_info,
                                        0);
    }

  return ls_type;
}

/*! \brief Create a new GHidLayerSelector
 *
 *  \return a freshly-allocated GHidLayerSelector.
 */
GtkWidget *
ghid_layer_selector_new (void)
{
  GtkCellRenderer *renderer1 = ghid_cell_renderer_visibility_new ();
  GtkCellRenderer *renderer2 = gtk_cell_renderer_text_new ();
  GtkTreeViewColumn *opacity_col =
      gtk_tree_view_column_new_with_attributes ("", renderer1,
                                                "active", VISIBLE_COL,
                                                "color", COLOR_COL, NULL);
  GtkTreeViewColumn *name_col =
      gtk_tree_view_column_new_with_attributes ("", renderer2,
                                                "text", TEXT_COL,
                                                "font", FONT_COL,
                                                "sensitive", VISIBLE_COL, NULL);

  GHidLayerSelector *ls = g_object_new (GHID_LAYER_SELECTOR_TYPE, NULL);

  /* action index, active, color, text, font, is_separator */
  ls->list_store = gtk_list_store_new (N_COLS, G_TYPE_POINTER, G_TYPE_INT,
                                       G_TYPE_BOOLEAN, G_TYPE_STRING,
                                       G_TYPE_STRING, G_TYPE_STRING,
                                       G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (ls), opacity_col, -1);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (ls), name_col, -1);
  gtk_tree_view_set_model (GTK_TREE_VIEW (ls), GTK_TREE_MODEL (ls->list_store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (ls), FALSE);

  ls->last_activatable = TRUE;
  ls->visibility_column = opacity_col;
  ls->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ls));
  ls->action_group = gtk_action_group_new ("LayerSelector");
  ls->prevent_recursion = FALSE;
  ls->n_actions = 0;

  gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW (ls),
                                        tree_view_separator_func,
                                        NULL, NULL);
  gtk_tree_selection_set_select_function (ls->selection, tree_selection_func,
                                          NULL, NULL);
  gtk_tree_selection_set_mode (ls->selection, GTK_SELECTION_BROWSE);

  g_object_set_data (G_OBJECT (ls->list_store), "layer-selector", ls);
  g_signal_connect (ls, "button_press_event",
                    G_CALLBACK (button_press_cb), NULL);
  g_signal_connect (ls->selection, "changed",
                    G_CALLBACK (selection_changed_cb), ls);

  g_object_ref (ls->action_group);

  return GTK_WIDGET (ls);
}

/*! \brief Add a layer to a GHidLayerSelector.
 *  \par Function Description
 *  This function adds an entry to a GHidLayerSelector, which will
 *  appear in the layer-selection list as well as visibility and selection
 *  menus (assuming this is a selectable layer). For the first 20 layers,
 *  keyboard accelerators will be added for selection/visibility toggling.
 *
 *  If the user_id passed already exists in the layer selector, that layer
 *  will have its data overwritten with the new stuff.
 *
 *  \param [in] ls            The selector to be acted on
 *  \param [in] user_id       An ID used to identify the layer; will be passed to selection/visibility callbacks
 *  \param [in] name          The name of the layer; will be used on selector and menus
 *  \param [in] color_string  The color of the layer on selector
 *  \param [in] visibile      Whether the layer is visible
 *  \param [in] activatable   Whether the layer appears in menus and can be selected
 */
void
ghid_layer_selector_add_layer (GHidLayerSelector *ls,
                               gint user_id,
                               const gchar *name,
                               const gchar *color_string,
                               gboolean visible,
                               gboolean activatable)
{
  struct _layer *new_layer = NULL;
  gchar *pname, *vname, *paccel, *vaccel;
  gboolean new_iter = TRUE;
  gboolean last_activatable = TRUE;
  GtkTreePath *path;
  GtkTreeIter iter;

  /* Look for existing layer with this ID */
  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (ls->list_store), &iter))
    do
      {
        gboolean is_sep, active;
        gint read_id;
        gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store),
                            &iter, USER_ID_COL, &read_id,
                            SEPARATOR_COL, &is_sep,
                            ACTIVATABLE_COL, &active, -1);
        if (!is_sep)
          {
            last_activatable = active;
            if(read_id == user_id)
              {
                new_iter = FALSE;
                break;
              }
          }
      }
    while (gtk_tree_model_iter_next (GTK_TREE_MODEL (ls->list_store), &iter));

  /* Handle separator addition */
  if (new_iter)
    {
      if (activatable != last_activatable)
        {
          /* Add separator between activatable/non-activatable boundaries */
          gtk_list_store_append (ls->list_store, &iter);
          gtk_list_store_set (ls->list_store, &iter,
                              STRUCT_COL, NULL,
                              SEPARATOR_COL, TRUE, -1);
        }
      /* Create new layer */
      new_layer = malloc (sizeof (*new_layer));
      gtk_list_store_append (ls->list_store, &iter);
      gtk_list_store_set (ls->list_store, &iter,
                          STRUCT_COL, new_layer,
                          USER_ID_COL, user_id,
                          VISIBLE_COL, visible,
                          COLOR_COL, color_string,
                          TEXT_COL, name,
                          FONT_COL, activatable ? NULL : "Italic",
                          ACTIVATABLE_COL, activatable,
                          SEPARATOR_COL, FALSE,
                          -1);
    }
  else
    {
      gtk_list_store_set (ls->list_store, &iter,
                          VISIBLE_COL, visible,
                          COLOR_COL, color_string,
                          TEXT_COL, name,
                          FONT_COL, activatable ? NULL : "Italic",
                          ACTIVATABLE_COL, activatable,
                          -1);
      return;
    }

  /* -- Setup new actions -- */
  vname = g_strdup_printf ("LayerView%d", ls->n_actions);
  pname = g_strdup_printf ("LayerPick%d", ls->n_actions);
  vaccel = NULL;
  paccel = NULL;

  /* Determine keyboard accelerators */
  if (ls->n_actions < 10)
    {
      /* Map 1-0 to actions 1-10 (with '0' meaning 10) */
      int i = (ls->n_actions + 1) % 10;
      vaccel = g_strdup_printf ("<Ctrl>%d", i);
      paccel = g_strdup_printf ("%d", i);
    }
  else
    {
      /* Map 1-0 to actions 11-20 (with '0' meaning 10) */
      int i = (ls->n_actions + 1) % 10;
      vaccel = g_strdup_printf ("<Alt><Ctrl>%d", i);
      paccel = g_strdup_printf ("<Alt>%d", i);
    }

  /* Create row reference for actions */
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (ls->list_store), &iter);
  new_layer->rref = gtk_tree_row_reference_new
                      (GTK_TREE_MODEL (ls->list_store), path);
  gtk_tree_path_free (path);

  /* Create selection action */
  if (activatable)
    {
      new_layer->pick_action
        = gtk_radio_action_new (pname, name, NULL, NULL, user_id);
      gtk_radio_action_set_group (new_layer->pick_action, ls->radio_group);
      ls->radio_group = gtk_radio_action_get_group (new_layer->pick_action);
      gtk_action_group_add_action_with_accel
        (ls->action_group,
         GTK_ACTION (new_layer->pick_action),
         paccel);
      g_signal_connect (new_layer->pick_action, "toggled",
                        G_CALLBACK (menu_pick_cb), new_layer->rref);
    }
  else
    new_layer->pick_action = NULL;

  /* Create visibility action */
  new_layer->view_action = gtk_toggle_action_new (vname, name, NULL, NULL);
  gtk_toggle_action_set_active (new_layer->view_action, visible);

  gtk_action_group_add_action_with_accel
    (ls->action_group,
     GTK_ACTION (new_layer->view_action),
     vaccel);
  g_signal_connect (new_layer->view_action, "toggled",
                    G_CALLBACK (menu_view_cb), new_layer->rref);


  /* Select new layer, if we need */
  if (activatable
      && !gtk_tree_selection_get_selected (ls->selection, NULL, NULL))
    gtk_tree_selection_select_iter (ls->selection, &iter);

  /* cleanup */
  if (vaccel)
    {
      g_free (vaccel);
      g_free (paccel); 
    }
  g_free (vname);
  g_free (pname);

  ls->n_actions++;
}

/*! \brief used internally */
static gboolean
pick_xml_foreach_func (GtkTreeModel *model, GtkTreePath *path,
                       GtkTreeIter *iter, gpointer data)
{
  struct _layer *ldata;
  GString *str = data;
  
  gtk_tree_model_get (model, iter, STRUCT_COL, &ldata, -1);
  if (ldata && ldata->pick_action)
    g_string_append_printf (str, "<menuitem action=\"%s\" />\n",
                     gtk_action_get_name (GTK_ACTION (ldata->pick_action)));
  return FALSE;
}
/*! \brief used internally */
static gboolean
view_xml_foreach_func (GtkTreeModel *model, GtkTreePath *path,
                       GtkTreeIter *iter, gpointer data)
{
  struct _layer *ldata;
  GString *str = data;
  
  gtk_tree_model_get (model, iter, STRUCT_COL, &ldata, -1);
  if (ldata && ldata->view_action)
    g_string_append_printf (str, "<menuitem action=\"%s\" />\n",
                     gtk_action_get_name (GTK_ACTION (ldata->view_action)));
  return FALSE;
}
 
/*! \brief Get the "Current Layer" menu description of a layer selector
 *  \par Function Description
 *  Returns the XML content used by Gtk in building the layer-selection
 *  part of the menu. This is a radio-button list describing which layer
 *  is active.
 *
 *  \param [in] ls            The selector to be acted on
 *
 *  \return the requested XML
 */
gchar *
ghid_layer_selector_get_pick_xml (GHidLayerSelector *ls)
{
  GString *str = g_string_new ("");
  gtk_tree_model_foreach (GTK_TREE_MODEL (ls->list_store),
                          pick_xml_foreach_func, str);
  return g_string_free (str, FALSE);
}

/*! \brief Get the "Shown Layers" menu description of a layer selector
 *  \par Function Description
 *  Returns the XML content used by Gtk in building the layer-selection
 *  part of the menu. This is a toggle-button list describing which layer(s)
 *  are visible.
 *
 *  \param [in] ls            The selector to be acted on
 *
 *  \return the requested XML
 */
gchar *
ghid_layer_selector_get_view_xml (GHidLayerSelector *ls)
{
  GString *str = g_string_new ("");
  gtk_tree_model_foreach (GTK_TREE_MODEL (ls->list_store),
                          view_xml_foreach_func, str);
  return g_string_free (str, FALSE);
}

/*! \brief Get the GtkActionGroup containing accelerators, etc, of a layer selector
 *  \par Function Description
 *  Returns the GtkActionGroup containing the toggle and radio buttons used
 *  in the menu. Also contains the accelerators. This action group should be
 *  added to the main UI. See Gtk docs for details.
 *
 *  \param [in] ls            The selector to be acted on
 *
 *  \return the action group of the selector
 */
GtkActionGroup *
ghid_layer_selector_get_action_group (GHidLayerSelector *ls)
{
  return ls->action_group;
}

/*! \brief used internally */
static gboolean
toggle_foreach_func (GtkTreeModel *model, GtkTreePath *path,
                     GtkTreeIter *iter, gpointer data)
{
  gint id;
  GHidLayerSelector *ls = g_object_get_data (G_OBJECT (model),
                                               "layer-selector");
  
  gtk_tree_model_get (model, iter, USER_ID_COL, &id, -1);
  if (id == *(gint *) data)
    {
      toggle_visibility (ls, iter);
      return TRUE;
    }
  return FALSE;
}

/*! \brief Toggle a layer's visibility
 *  \par Function Description
 *  Toggle the layer indicated by user_id, emitting a layer-toggle signal.
 *
 *  \param [in] ls       The selector to be acted on
 *  \param [in] user_id  The ID of the layer to be affected
 */
void
ghid_layer_selector_toggle_layer (GHidLayerSelector *ls, gint user_id)
{
  gtk_tree_model_foreach (GTK_TREE_MODEL (ls->list_store),
                          toggle_foreach_func, &user_id);
}

/*! \brief used internally */
static gboolean
select_foreach_func (GtkTreeModel *model, GtkTreePath *path,
                     GtkTreeIter *iter, gpointer data)
{
  gint id;
  GHidLayerSelector *ls = g_object_get_data (G_OBJECT (model),
                                               "layer-selector");
  
  gtk_tree_model_get (model, iter, USER_ID_COL, &id, -1);
  if (id == *(gint *) data)
    {
      gtk_tree_selection_select_path (ls->selection, path);
      return TRUE;
    }
  return FALSE;
}

/*! \brief Select a layer
 *  \par Function Description
 *  Select the layer indicated by user_id, emitting a layer-select signal.
 *
 *  \param [in] ls       The selector to be acted on
 *  \param [in] user_id  The ID of the layer to be affected
 */
void
ghid_layer_selector_select_layer (GHidLayerSelector *ls, gint user_id)
{
  gtk_tree_model_foreach (GTK_TREE_MODEL (ls->list_store),
                          select_foreach_func, &user_id);
}

/*! \brief Selects the next visible layer
 *  \par Function Description
 *  Used to ensure hidden layers are not active; if the active layer is
 *  visible, this function is a noop. Otherwise, it will look for the
 *  next layer that IS visible, and select that. Failing that, it will
 *  return FALSE.
 *
 *  \param [in] ls       The selector to be acted on
 *
 *  \return TRUE on success, FALSE if all selectable layers are hidden
 */
gboolean
ghid_layer_selector_select_next_visible (GHidLayerSelector *ls)
{
  GtkTreeIter iter;
  if (gtk_tree_selection_get_selected (ls->selection, NULL, &iter))
    {
      /* Scan forward, looking for selectable iter */
      do
        {
          gboolean visible, activatable;
          gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store),
                              &iter, VISIBLE_COL, &visible,
                              ACTIVATABLE_COL, &activatable, -1);
          if (visible && activatable)
            {
              gtk_tree_selection_select_iter (ls->selection, &iter);
              return TRUE;
            }
        }
      while (gtk_tree_model_iter_next (GTK_TREE_MODEL (ls->list_store), &iter));
      /* Move iter to start, and repeat. */
      gtk_tree_model_get_iter_first (GTK_TREE_MODEL (ls->list_store), &iter);
      do
        {
          gboolean visible, activatable;
          gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store),
                              &iter, VISIBLE_COL, &visible,
                              ACTIVATABLE_COL, &activatable, -1);
          if (visible && activatable)
            {
              gtk_tree_selection_select_iter (ls->selection, &iter);
              return TRUE;
            }
        }
      while (gtk_tree_model_iter_next (GTK_TREE_MODEL (ls->list_store), &iter));
      /* If we get here, nothing is selectable, so fail. */
    }
  return FALSE;
}

/*! \brief Makes the selected layer visible
 *  \par Function Description
 *  Used to ensure hidden layers are not active; un-hides the currently
 *  selected layer.
 *
 *  \param [in] ls       The selector to be acted on
 */
void
ghid_layer_selector_make_selected_visible (GHidLayerSelector *ls)
{
  GtkTreeIter iter;
  if (gtk_tree_selection_get_selected (ls->selection, NULL, &iter))
    {
      gboolean visible;
      gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store),
                          &iter, VISIBLE_COL, &visible, -1);
      if (!visible)
        toggle_visibility (ls, &iter);
    }
}

/*! \brief Sets the colors of all layers in a layer-selector
 *  \par Function Description
 *  Updates the colors of a layer selector via a callback mechanism:
 *  the user_id of each layer is passed to the callback function,
 *  which returns a color string to update the layer's color, or NULL
 *  to leave it alone.
 *
 *  \param [in] ls       The selector to be acted on
 *  \param [in] callback Takes the user_id of the layer and returns a color string
 */
void
ghid_layer_selector_update_colors (GHidLayerSelector *ls,
                                   const gchar *(*callback)(int user_id))
{
  GtkTreeIter iter;
  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (ls->list_store), &iter);
  do
    {
      gint user_id;
      const gchar *new_color;
      gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store),
                          &iter, USER_ID_COL, &user_id, -1);
      new_color = callback (user_id);
      if (new_color != NULL)
        gtk_list_store_set (ls->list_store, &iter, COLOR_COL, new_color, -1);
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (ls->list_store), &iter));
}

/*! \brief Deletes layers from a layer selector
 *  \par Function Description
 *  Deletes layers according to a callback function: a return value of TRUE
 *  means delete, FALSE means leave it alone. Do not try to delete all layers
 *  using this function; with nothing left to select, pcb will likely go into
 *  an infinite recursion between hid_action() and g_signal().
 *
 *  Separators will be deleted if the layer AFTER them is deleted.
 *
 *  \param [in] ls       The selector to be acted on
 *  \param [in] callback Takes the user_id of the layer and returns a boolean
 */
void
ghid_layer_selector_delete_layers (GHidLayerSelector *ls,
                                   gboolean (*callback)(int user_id))
{
  GtkTreeIter iter, last_iter;
  gboolean needs_inc;
  gboolean was_separator = FALSE;
  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (ls->list_store), &iter);
  do
    {
      struct _layer *ldata;
      gboolean sep;
      gint user_id;
      gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store),
                          &iter, USER_ID_COL, &user_id,
                          STRUCT_COL, &ldata, SEPARATOR_COL, &sep, -1);
      /* gtk_list_store_remove will increment the iter for us, so we
       *  don't want to do it again in the loop condition */
      needs_inc = TRUE;
      if (!sep && callback (user_id))
        {
          if (gtk_list_store_remove (ls->list_store, &iter))
            {
              if (ldata->view_action)
                gtk_action_group_remove_action (ls->action_group,
                                                GTK_ACTION (ldata->view_action));
              if (ldata->pick_action)
                gtk_action_group_remove_action (ls->action_group,
                                                GTK_ACTION (ldata->pick_action));
              gtk_tree_row_reference_free (ldata->rref);
              g_free (ldata);
              needs_inc = FALSE;
            }
          else
            return;
          if (was_separator)
            gtk_list_store_remove (ls->list_store, &last_iter);
        }
      last_iter = iter;
      was_separator = sep;
    }
  while (!needs_inc ||
         gtk_tree_model_iter_next (GTK_TREE_MODEL (ls->list_store), &iter));
}



