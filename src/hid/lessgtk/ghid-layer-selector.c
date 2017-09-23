/*!
 * \file src/hid/lessgtk/ghid-layer-selector.c
 *
 * \brief Implementation of GHidLayerSelector widget
 *
 * This widget is the layer selector.
 * It also builds the relevant sections of the menu for layer
 * selection and visibility toggling, and keeps these in sync.
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
struct _layer;
static void ghid_layer_selector_finalize (GObject *object);
static void menu_pick_cb (GtkRadioAction *action, struct _layer *ldata);

/*!
 * \brief Signals exposed by the widget.
 */
enum {
  SELECT_LAYER_SIGNAL,
  TOGGLE_LAYER_SIGNAL,
  RENAME_LAYER_SIGNAL,
  LAST_SIGNAL
};

/*!
 * \brief Columns used for internal data store.
 */
enum {
  STRUCT_COL,
  USER_ID_COL,
  VISIBLE_COL,
  COLOR_COL,
  TEXT_COL,
  FONT_COL,
  EDITABLE_COL,
  SELECTABLE_COL,
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
  GtkAccelGroup *accel_group;

  GSList *radio_group;
  int n_actions;

  gboolean accel_available[20];

  gulong selection_changed_sig_id;
};

struct _GHidLayerSelectorClass
{
  GtkTreeViewClass parent_class;

  void (* select_layer) (GHidLayerSelector *, gint);
  void (* toggle_layer) (GHidLayerSelector *, gint);
  void (* rename_layer) (GHidLayerSelector *, gint, gchar *);
};

struct _layer
{
  gint accel_index;   /* Index into ls->accel_available */
  GtkWidget *pick_item;
  GtkWidget *view_item;
  GtkToggleAction *view_action;
  GtkRadioAction  *pick_action;
  GtkTreeRowReference *rref;
};

static void
g_cclosure_user_marshal_VOID__INT_STRING (GClosure     *closure,
                                          GValue       *return_value G_GNUC_UNUSED,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint G_GNUC_UNUSED,
                                          gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__INT_STRING) (gpointer     data1,
                                                 gint         arg_1,
                                                 gpointer     arg_2,
                                                 gpointer     data2);
  register GMarshalFunc_VOID__INT_STRING callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__INT_STRING) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_value_get_int (param_values + 1),
            (char *)g_value_get_string (param_values + 2),
            data2);
}

/*!
 * \brief Deletes the action and accelerator from a layer.
 */
static void
free_ldata (GHidLayerSelector *ls, struct _layer *ldata)
{
  if (ldata->pick_action)
    {
      gtk_action_disconnect_accelerator
        (GTK_ACTION (ldata->pick_action));
      gtk_action_group_remove_action (ls->action_group,
                                    GTK_ACTION (ldata->pick_action));
/*! \todo Make this work without wrecking the radio action group
 *           g_object_unref (G_OBJECT (ldata->pick_action)); 
 */
    }
  if (ldata->view_action)
    {
      gtk_action_disconnect_accelerator
        (GTK_ACTION (ldata->view_action));
      gtk_action_group_remove_action (ls->action_group,
                            GTK_ACTION (ldata->view_action));
      g_object_unref (G_OBJECT (ldata->view_action));
    }
  gtk_tree_row_reference_free (ldata->rref);
  if (ldata->accel_index >= 0)
    ls->accel_available[ldata->accel_index] = TRUE;
  g_free (ldata);

}

/*!
 * \brief internal set-visibility function -- emits no signals.
 */
static void
set_visibility (GHidLayerSelector *ls, GtkTreeIter *iter,
                struct _layer *ldata, gboolean state)
{
  gtk_list_store_set (ls->list_store, iter, VISIBLE_COL, state, -1);
  
  if (ldata)
    {
      gtk_action_block_activate (GTK_ACTION (ldata->view_action));
      gtk_check_menu_item_set_active
        (GTK_CHECK_MENU_ITEM (ldata->view_item), state);
      gtk_action_unblock_activate (GTK_ACTION (ldata->view_action));
    }
}

/*!
 * \brief Flip the visibility state of a given layer.
 *
 * Changes the internal toggle state and menu checkbox state
 * of the layer pointed to by iter. Emits a toggle-layer signal.
 *
 * \param [in] ls    The selector to be acted on
 * \param [in] iter  A GtkTreeIter pointed at the relevant layer
 * \param [in] emit  Whether or not to emit a signal
 */
static void
toggle_visibility (GHidLayerSelector *ls, GtkTreeIter *iter, gboolean emit)
{
  gint user_id;
  struct _layer *ldata;
  gboolean toggle;
  gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store), iter,
                      USER_ID_COL, &user_id, VISIBLE_COL, &toggle,
                      STRUCT_COL, &ldata, -1);
  set_visibility (ls, iter, ldata, !toggle);
  if (emit)
    g_signal_emit (ls, ghid_layer_selector_signals[TOGGLE_LAYER_SIGNAL],
                   0, user_id);
}

/*!
 * \brief Decide if a GtkListStore entry is a layer or separator.
 */
static gboolean
tree_view_separator_func (GtkTreeModel *model, GtkTreeIter *iter,
                          gpointer data)
{
  gboolean ret_val;
  gtk_tree_model_get (model, iter, SEPARATOR_COL, &ret_val, -1);
  return ret_val;
}

/*!
 * \brief Decide if a GtkListStore entry may be selected.
 */
static gboolean
tree_selection_func (GtkTreeSelection *selection, GtkTreeModel *model,
                     GtkTreePath *path, gboolean selected, gpointer data)
{
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter (model, &iter, path))
    {
      gboolean selectable;
      gtk_tree_model_get (model, &iter, SELECTABLE_COL, &selectable, -1);
      return selectable;
    }

  return FALSE;
}

/* SIGNAL HANDLERS */
/*!
 * \brief Callback for mouse-click: toggle visibility.
 */
static gboolean
button_press_cb (GHidLayerSelector *ls, GdkEventButton *event)
{
  /* Handle visibility independently to prevent changing the active
   *  layer, which will happen if we let this event propagate.  */
  GtkTreeViewColumn *column;
  GtkTreePath *path;

  /* Ignore the synthetic presses caused by double and tripple clicks, and
   * also ignore all but left-clicks
   */
  if (event->type != GDK_BUTTON_PRESS ||
      event->button != 1)
    return TRUE;

  if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (ls),
                                     event->x, event->y,
                                     &path, &column, NULL, NULL))
    {
      GtkTreeIter iter;
      gboolean selectable;
      gboolean separator;
      gtk_tree_model_get_iter (GTK_TREE_MODEL (ls->list_store), &iter, path);
      gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store), &iter,
                          SELECTABLE_COL, &selectable,
                          SEPARATOR_COL, &separator, -1);
      /* Toggle visibility for non-selectable layers no matter
       *  where you click. */
      if (!separator && (column == ls->visibility_column || !selectable))
        {
          toggle_visibility (ls, &iter, TRUE);
          return TRUE; 
        }
    }
  return FALSE;
}

/*!
 * \brief Callback for layer selection change: sync menu, emit signal.
 */
static void
selection_changed_cb (GtkTreeSelection *selection, GHidLayerSelector *ls)
{
  GtkTreeIter iter;
  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gint user_id;
      struct _layer *ldata;
      gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store), &iter,
                          STRUCT_COL, &ldata, USER_ID_COL, &user_id, -1);

      if (ldata && ldata->pick_action)
        {
          gtk_action_block_activate (GTK_ACTION (ldata->pick_action));
          gtk_radio_action_set_current_value (ldata->pick_action, user_id);
          gtk_action_unblock_activate (GTK_ACTION (ldata->pick_action));
        }
      g_signal_emit (ls, ghid_layer_selector_signals[SELECT_LAYER_SIGNAL],
                     0, user_id);
    }
}

/*!
 * \brief Callback for when a layer name has been edited.
 */
static void
layer_name_editing_started_cb (GtkCellRenderer *renderer,
                               GtkCellEditable *editable,
                               gchar           *path,
                               gpointer         user_data)
{
  /* When editing begins, we need to detach PCB's accelerators
   * so they don't steal all the user's keystrokes.
   */
  /*! \todo We should not have to do this within a simple widget,
   * and this quick hack workaround breaks the widget's
   * abstraction from the rest of the application :(
   */
  ghid_remove_accel_groups (GTK_WINDOW (gport->top_window), ghidgui);
}

/*!
 * \brief Callback for when layer name editing has been canceled.
 */
static void
layer_name_editing_canceled_cb (GtkCellRenderer *renderer,
                                 gpointer         user_data)
{
  /* Put PCB's accelerators back.
   *
   * XXX: We should not have to do this within a simple widget,
   *      and this quick hack workaround breaks the widget's
   *      abstraction from the rest of the application :(
   */
  ghid_install_accel_groups (GTK_WINDOW (gport->top_window), ghidgui);
}

/*!
 * \brief Callback for when a layer name has been edited.
 */
static void
layer_name_edited_cb (GtkCellRendererText *renderer,
                      gchar               *path,
                      gchar               *new_text,
                      gpointer             user_data)
{
  GHidLayerSelector *ls = user_data;
  GtkTreeIter iter;
  int user_id;

  /* Put PCB's accelerators back.
   */
  /*! \todo We should not have to do this within a simple widget,
   * and this quick hack workaround breaks the widget's
   * abstraction from the rest of the application :(
   */
  ghid_install_accel_groups (GTK_WINDOW (gport->top_window), ghidgui);

  if (!gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (ls->list_store), &iter, path))
    return;

  gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store),
                      &iter,
                      USER_ID_COL, &user_id,
                      -1);

  g_signal_emit (ls, ghid_layer_selector_signals[RENAME_LAYER_SIGNAL],
                 0, user_id, new_text);
}


/*!
 * \brief Callback for menu actions: sync layer selection list, emit
 * signal.
 */
static void
menu_view_cb (GtkToggleAction *action, struct _layer *ldata)
{
  GHidLayerSelector *ls;
  GtkTreeModel *model = gtk_tree_row_reference_get_model (ldata->rref);
  GtkTreePath *path = gtk_tree_row_reference_get_path (ldata->rref);
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

/*!
 * \brief Callback for menu actions: sync layer selection list, emit
 * signal.
 */
static void
menu_pick_cb (GtkRadioAction *action, struct _layer *ldata)
{
  /* We only care about the activation signal (as opposed to deactivation).
   * A row we are /deactivating/ might not even exist anymore! */
  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
    {
      GHidLayerSelector *ls;
      GtkTreeModel *model = gtk_tree_row_reference_get_model (ldata->rref);
      GtkTreePath *path = gtk_tree_row_reference_get_path (ldata->rref);
      GtkTreeIter iter;
      gint user_id;

      gtk_tree_model_get_iter (model, &iter, path);
      gtk_tree_model_get (model, &iter, USER_ID_COL, &user_id, -1);

      ls = g_object_get_data (G_OBJECT (model), "layer-selector");
      g_signal_handler_block (ls->selection, ls->selection_changed_sig_id);
      gtk_tree_selection_select_path (ls->selection, path);
      g_signal_handler_unblock (ls->selection, ls->selection_changed_sig_id);
      g_signal_emit (ls, ghid_layer_selector_signals[SELECT_LAYER_SIGNAL],
                     0, user_id);
    }
}

static void
ghid_layer_selector_init (GHidLayerSelector *ls)
{
  int i;
  GtkCellRenderer *renderer1;
  GtkCellRenderer *renderer2;
  GtkTreeViewColumn *opacity_col;
  GtkTreeViewColumn *name_col;

  renderer1 = ghid_cell_renderer_visibility_new ();
  renderer2 = gtk_cell_renderer_text_new ();
  g_object_set (renderer2, "editable-set", TRUE, NULL);
  g_signal_connect (renderer2, "editing-started",
                    G_CALLBACK (layer_name_editing_started_cb), ls);
  g_signal_connect (renderer2, "editing-canceled",
                    G_CALLBACK (layer_name_editing_canceled_cb), ls);
  g_signal_connect (renderer2, "edited",
                    G_CALLBACK (layer_name_edited_cb), ls);

  opacity_col = gtk_tree_view_column_new_with_attributes ("",
                                                          renderer1,
                                                          "active", VISIBLE_COL,
                                                          "color",  COLOR_COL,
                                                          NULL);
  name_col = gtk_tree_view_column_new_with_attributes ("",
                                                       renderer2,
                                                       "text", TEXT_COL,
                                                       "font", FONT_COL,
                                                       "editable", EDITABLE_COL,
                                                       NULL);

  ls->list_store = gtk_list_store_new (N_COLS,
                 /* STRUCT_COL      */ G_TYPE_POINTER,
                 /* USER_ID_COL     */ G_TYPE_INT,
                 /* VISIBLE_COL     */ G_TYPE_BOOLEAN,
                 /* COLOR_COL       */ G_TYPE_STRING,
                 /* TEXT_COL        */ G_TYPE_STRING,
                 /* FONT_COL        */ G_TYPE_STRING,
                 /* EDITABLE_COL    */ G_TYPE_BOOLEAN,
                 /* ACTIVATABLE_COL */ G_TYPE_BOOLEAN,
                 /* SEPARATOR_COL   */ G_TYPE_BOOLEAN);

  gtk_tree_view_insert_column (GTK_TREE_VIEW (ls), opacity_col, -1);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (ls), name_col, -1);
  gtk_tree_view_set_model (GTK_TREE_VIEW (ls), GTK_TREE_MODEL (ls->list_store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (ls), FALSE);

  ls->visibility_column = opacity_col;
  ls->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ls));
  ls->accel_group = gtk_accel_group_new ();
  ls->action_group = gtk_action_group_new ("LayerSelector");
  ls->n_actions = 0;

  for (i = 0; i < 20; ++i)
    ls->accel_available[i] = TRUE;

  gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW (ls),
                                        tree_view_separator_func,
                                        NULL, NULL);
  gtk_tree_selection_set_select_function (ls->selection, tree_selection_func,
                                          NULL, NULL);
  gtk_tree_selection_set_mode (ls->selection, GTK_SELECTION_BROWSE);

  g_object_set_data (G_OBJECT (ls->list_store), "layer-selector", ls);
  g_signal_connect (ls, "button_press_event",
                    G_CALLBACK (button_press_cb), NULL);
  ls->selection_changed_sig_id =
    g_signal_connect (ls->selection, "changed",
                      G_CALLBACK (selection_changed_cb), ls);
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
  ghid_layer_selector_signals[RENAME_LAYER_SIGNAL] =
    g_signal_new ("rename-layer",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GHidLayerSelectorClass, rename_layer),
                  NULL, NULL,
                  g_cclosure_user_marshal_VOID__INT_STRING, G_TYPE_NONE,
                  2, G_TYPE_INT, G_TYPE_STRING);

  object_class->finalize = ghid_layer_selector_finalize;
}

/*!
 * \brief Clean up object before garbage collection.
 */
static void
ghid_layer_selector_finalize (GObject *object)
{
  GtkTreeIter iter;
  GHidLayerSelector *ls = (GHidLayerSelector *) object;

  g_object_unref (ls->accel_group);
  g_object_unref (ls->action_group);

  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (ls->list_store), &iter);
  do
    {
      struct _layer *ldata;
      gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store),
                          &iter, STRUCT_COL, &ldata, -1);
      free_ldata (ls, ldata);
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

/*!
 * \brief Create a new GHidLayerSelector.
 *
 * \return a freshly-allocated GHidLayerSelector.
 */
GtkWidget *
ghid_layer_selector_new (void)
{
  return GTK_WIDGET (g_object_new (GHID_LAYER_SELECTOR_TYPE, NULL));
}

/*!
 * \brief Add a layer to a GHidLayerSelector.
 *
 * This function adds an entry to a GHidLayerSelector, which will
 * appear in the layer-selection list as well as visibility and selection
 * menus (assuming this is a selectable layer). For the first 20 layers,
 * keyboard accelerators will be added for selection/visibility toggling.
 *
 * If the user_id passed already exists in the layer selector, that layer
 * will have its data overwritten with the new stuff.
 *
 * \param [in] ls            The selector to be acted on.
 * \param [in] user_id       An ID used to identify the layer; will be passed to selection/visibility callbacks.
 * \param [in] name          The name of the layer; will be used on selector and menus.
 * \param [in] color_string  The color of the layer on selector.
 * \param [in] visibile      Whether the layer is visible.
 * \param [in] selectable    Whether the layer appears in menus and can be selected.
 * \param [in] renameable    Whether the layer is renameable.
 */
void
ghid_layer_selector_add_layer (GHidLayerSelector *ls,
                               gint user_id,
                               const gchar *name,
                               const gchar *color_string,
                               gboolean visible,
                               gboolean selectable,
                               gboolean renameable)
{
  struct _layer *new_layer = NULL;
  gchar *pname, *vname;
  gboolean new_iter = TRUE;
  gboolean last_selectable = TRUE;
  GtkTreePath *path;
  GtkTreeIter iter;
  int i;

  /* Look for existing layer with this ID */
  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (ls->list_store), &iter))
    do
      {
        gboolean is_sep;
        gboolean this_selectable;
        gint read_id;

        gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store),
                            &iter, USER_ID_COL, &read_id,
                            SEPARATOR_COL, &is_sep,
                            SELECTABLE_COL, &this_selectable, -1);

        if (is_sep)
          continue;

        last_selectable = this_selectable;
        if (read_id == user_id)
          {
            new_iter = FALSE;
            break;
          }
      }
    while (gtk_tree_model_iter_next (GTK_TREE_MODEL (ls->list_store), &iter));

  /* Handle separator addition */
  if (new_iter)
    {
      if (selectable != last_selectable)
        {
          /* Add separator between selectable / non-selectable boundaries */
          gtk_list_store_append (ls->list_store, &iter);
          gtk_list_store_set (ls->list_store, &iter,
                              STRUCT_COL, NULL,
                              SEPARATOR_COL, TRUE, -1);
        }
      /* Create new layer */
      gtk_list_store_append (ls->list_store, &iter);
    }
  else
    {
      /* If the row exists, we clear out its ldata to create
       * a new action, accelerator and menu item. */
      gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store), &iter,
                          STRUCT_COL, &new_layer, -1);
      free_ldata (ls, new_layer);
    }

  new_layer = g_malloc (sizeof (*new_layer));

  gtk_list_store_set (ls->list_store,
                      &iter,
                      STRUCT_COL,      new_layer,
                      USER_ID_COL,     user_id,
                      VISIBLE_COL,     visible,
                      COLOR_COL,       color_string,
                      TEXT_COL,        name,
                      FONT_COL,        selectable ? NULL : "Italic",
                      EDITABLE_COL,    renameable,
                      SELECTABLE_COL,  selectable,
                      SEPARATOR_COL,   FALSE,
                      -1);

  /* -- Setup new actions -- */
  vname = g_strdup_printf ("LayerView%d", ls->n_actions);
  pname = g_strdup_printf ("LayerPick%d", ls->n_actions);

  /* Create row reference for actions */
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (ls->list_store), &iter);
  new_layer->rref = gtk_tree_row_reference_new
                      (GTK_TREE_MODEL (ls->list_store), path);
  gtk_tree_path_free (path);

  /* Create selection action */
  if (selectable)
    {
      new_layer->pick_action
        = gtk_radio_action_new (pname, name, NULL, NULL, user_id);
      gtk_radio_action_set_group (new_layer->pick_action, ls->radio_group);
      ls->radio_group = gtk_radio_action_get_group (new_layer->pick_action);
    }
  else
    new_layer->pick_action = NULL;

  /* Create visibility action */
  new_layer->view_action = gtk_toggle_action_new (vname, name, NULL, NULL);
  gtk_toggle_action_set_active (new_layer->view_action, visible);

  /* Determine keyboard accelerators */
  for (i = 0; i < 20; ++i)
    if (ls->accel_available[i])
      break;
  if (i < 20)
    {
      /* Map 1-0 to actions 1-10 (with '0' meaning 10) */
      gchar *accel1 = g_strdup_printf ("%s%d",
                                       i < 10 ? "" : "<Alt>",
                                       (i + 1) % 10);
      gchar *accel2 = g_strdup_printf ("<Ctrl>%s%d",
                                       i < 10 ? "" : "<Alt>",
                                       (i + 1) % 10);

      if (selectable)
        {
          GtkAction *action = GTK_ACTION (new_layer->pick_action);
          gtk_action_set_accel_group (action, ls->accel_group);
          gtk_action_group_add_action_with_accel (ls->action_group,
                                                  action,
                                                  accel1);
          gtk_action_connect_accelerator (action);
          g_signal_connect (G_OBJECT (action), "activate",
                            G_CALLBACK (menu_pick_cb), new_layer);
        }
      gtk_action_set_accel_group (GTK_ACTION (new_layer->view_action),
                                  ls->accel_group);
      gtk_action_group_add_action_with_accel
          (ls->action_group, GTK_ACTION (new_layer->view_action), accel2);
      gtk_action_connect_accelerator (GTK_ACTION (new_layer->view_action));
      g_signal_connect (G_OBJECT (new_layer->view_action), "activate",
                        G_CALLBACK (menu_view_cb), new_layer);

      ls->accel_available[i] = FALSE;
      new_layer->accel_index = i;
      g_free (accel2);
      g_free (accel1);
    }
  else
    {
      new_layer->accel_index = -1;
    }
  /* finalize new layer struct */
  new_layer->pick_item = new_layer->view_item = NULL;

  /* cleanup */
  g_free (vname);
  g_free (pname);

  ls->n_actions++;
}

/*!
 * \brief Install the "Current Layer" menu items for a layer selector.
 *
 * Takes a menu shell and installs menu items for layer selection in
 * the shell, at the given position.
 *
 * \param [in] ls      The selector to be acted on.
 * \param [in] shell   The menu to install the items in.
 * \param [in] pos     The position in the menu to install items.
 *
 * \return the number of items installed.
 */
gint
ghid_layer_selector_install_pick_items (GHidLayerSelector *ls,
                                        GtkMenuShell *shell, gint pos)
{
  GtkTreeIter iter;
  int n = 0;

  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (ls->list_store), &iter);
  do
    {
      struct _layer *ldata;
      gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store),
                          &iter, STRUCT_COL, &ldata, -1);
      if (ldata && ldata->pick_action)
        {
          GtkAction *action = GTK_ACTION (ldata->pick_action);
          ldata->pick_item = gtk_action_create_menu_item (action);
          gtk_menu_shell_insert (shell, ldata->pick_item, pos + n);
          ++n;
        }
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (ls->list_store), &iter));

  return n;
}

/*!
 * \brief Install the "Shown Layers" menu items for a layer selector
 *
 * Takes a menu shell and installs menu items for layer selection in
 * the shell, at the given position.
 *
 * \param [in] ls      The selector to be acted on.
 * \param [in] shell   The menu to install the items in.
 * \param [in] pos     The position in the menu to install items.
 *
 * \return the number of items installed.
 */
gint
ghid_layer_selector_install_view_items (GHidLayerSelector *ls,
                                        GtkMenuShell *shell, gint pos)
{
  GtkTreeIter iter;
  int n = 0;

  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (ls->list_store), &iter);
  do
    {
      struct _layer *ldata;
      gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store),
                          &iter, STRUCT_COL, &ldata, -1);
      if (ldata && ldata->view_action)
        {
          GtkAction *action = GTK_ACTION (ldata->view_action);
          ldata->view_item = gtk_action_create_menu_item (action);
          gtk_menu_shell_insert (shell, ldata->view_item, pos + n);
          ++n;
        }
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (ls->list_store), &iter));

  return n;
}

/*!
 * \brief Returns the GtkAccelGroup of a layer selector.
 *
 * \param [in] ls            The selector to be acted on.
 *
 * \return the accel group of the selector.
 */
GtkAccelGroup *
ghid_layer_selector_get_accel_group (GHidLayerSelector *ls)
{
  return ls->accel_group;
}

struct layer_data {
  GHidLayerSelector *ls;
  gint user_id;
};

/*!
 * \brief used internally.
 */
static gboolean
toggle_foreach_func (GtkTreeModel *model, GtkTreePath *path,
                     GtkTreeIter *iter, gpointer user_data)
{
  struct layer_data *data = (struct layer_data *) user_data;
  gint id;

  gtk_tree_model_get (model, iter, USER_ID_COL, &id, -1);
  if (id == data->user_id)
    {
      toggle_visibility (data->ls, iter, TRUE);
      return TRUE;
    }
  return FALSE;
}

/*!
 * \brief Toggle a layer's visibility.
 *
 * Toggle the layer indicated by user_id, emitting a layer-toggle signal.
 *
 * \param [in] ls       The selector to be acted on.
 * \param [in] user_id  The ID of the layer to be affected.
 */
void
ghid_layer_selector_toggle_layer (GHidLayerSelector *ls, gint user_id)
{
  struct layer_data data;

  data.ls = ls;
  data.user_id = user_id;

  gtk_tree_model_foreach (GTK_TREE_MODEL (ls->list_store),
                          toggle_foreach_func, &data);
}

/*!
 * \brief used internally.
 */
static gboolean
select_foreach_func (GtkTreeModel *model, GtkTreePath *path,
                     GtkTreeIter *iter, gpointer user_data)
{
  struct layer_data *data = (struct layer_data *) user_data;
  gint id;

  gtk_tree_model_get (model, iter, USER_ID_COL, &id, -1);
  if (id == data->user_id)
    {
      gtk_tree_selection_select_path (data->ls->selection, path);
      return TRUE;
    }
  return FALSE;
}

/*!
 * \brief Select a layer.
 *
 * Select the layer indicated by user_id, emitting a layer-select signal.
 *
 * \param [in] ls       The selector to be acted on.
 * \param [in] user_id  The ID of the layer to be affected.
 */
void
ghid_layer_selector_select_layer (GHidLayerSelector *ls, gint user_id)
{
  struct layer_data data;

  data.ls = ls;
  data.user_id = user_id;

  gtk_tree_model_foreach (GTK_TREE_MODEL (ls->list_store),
                          select_foreach_func, &data);
}

/*!
 * \brief Selects the next visible layer.
 *
 * Used to ensure hidden layers are not active; if the active layer is
 * visible, this function is a noop. Otherwise, it will look for the
 * next layer that IS visible, and select that. Failing that, it will
 * return FALSE.
 *
 * \param [in] ls       The selector to be acted on.
 *
 * \return TRUE on success, FALSE if all selectable layers are hidden.
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
          gboolean visible;
          gboolean selectable;

          gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store),
                              &iter, VISIBLE_COL, &visible,
                              SELECTABLE_COL, &selectable, -1);
          if (visible && selectable)
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
          gboolean visible;
          gboolean selectable;

          gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store),
                              &iter, VISIBLE_COL, &visible,
                              SELECTABLE_COL, &selectable, -1);
          if (visible && selectable)
            {
              gtk_tree_selection_select_iter (ls->selection, &iter);
              return TRUE;
            }
        }
      while (gtk_tree_model_iter_next (GTK_TREE_MODEL (ls->list_store), &iter));
      /* Failing this, just emit a selected signal on the original layer. */
      selection_changed_cb (ls->selection, ls);
    }
  /* If we get here, nothing is selectable, so fail. */
  return FALSE;
}

/*!
 * \brief Makes the selected layer visible.
 *
 * Used to ensure hidden layers are not active; un-hides the currently
 * selected layer.
 *
 * \param [in] ls       The selector to be acted on.
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
        toggle_visibility (ls, &iter, FALSE);
    }
}

/*!
 * \brief Sets the colors of all layers in a layer-selector.
 *
 * Updates the colors of a layer selector via a callback mechanism:
 * the user_id of each layer is passed to the callback function,
 * which returns a color string to update the layer's color, or NULL
 * to leave it alone.
 *
 * \param [in] ls       The selector to be acted on.
 * \param [in] callback Takes the user_id of the layer and returns a
 * color string.
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

/*!
 * \brief Deletes layers from a layer selector.
 *
 * Deletes layers according to a callback function: a return value of TRUE
 * means delete, FALSE means leave it alone. Do not try to delete all layers
 * using this function; with nothing left to select, pcb will likely go into
 * an infinite recursion between hid_action() and g_signal().
 *
 * Separators will be deleted if the layer AFTER them is deleted.
 *
 * \param [in] ls       The selector to be acted on.
 * \param [in] callback Takes the user_id of the layer and returns a
 * boolean.
 */
void
ghid_layer_selector_delete_layers (GHidLayerSelector *ls,
                                   gboolean (*callback)(int user_id))
{
  GtkTreeIter iter, last_iter;
 
  gboolean iter_valid =
    gtk_tree_model_get_iter_first (GTK_TREE_MODEL (ls->list_store), &iter);
  while (iter_valid)
    {
      struct _layer *ldata;
      gboolean sep, was_sep = FALSE;
      gint user_id;

      /* Find next iter to delete */
      while (iter_valid)
        {
          gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store),
                              &iter, USER_ID_COL, &user_id,
                              STRUCT_COL, &ldata, SEPARATOR_COL, &sep, -1);
          if (!sep && callback (user_id))
            break;

          /* save iter in case it's a bad separator */
          was_sep = sep;
          last_iter = iter;
          /* iterate */
          iter_valid =
            gtk_tree_model_iter_next (GTK_TREE_MODEL (ls->list_store), &iter);
        }

      if (iter_valid)
        {
          /* remove preceeding separator */
          if (was_sep)
            gtk_list_store_remove (ls->list_store, &last_iter);

          /*** remove row ***/
          iter_valid = gtk_list_store_remove (ls->list_store, &iter);
          free_ldata (ls, ldata);
        }
      last_iter = iter;
    }
}

/*!
 * \brief Sets the visibility toggle-state of all layers.
 *
 * Shows layers according to a callback function: a return value of TRUE
 * means show, FALSE means hide.
 *
 * \param [in] ls       The selector to be acted on.
 * \param [in] callback Takes the user_id of the layer and returns a
 * boolean.
 */
void
ghid_layer_selector_show_layers (GHidLayerSelector *ls,
                                 gboolean (*callback)(int user_id))
{
  GtkTreeIter iter;
  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (ls->list_store), &iter);
  do
    {
      struct _layer *ldata;
      gboolean sep;
      gint user_id;

      gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store),
                          &iter, USER_ID_COL, &user_id,
                          STRUCT_COL, &ldata,
                          SEPARATOR_COL, &sep, -1);
      if (!sep)
        set_visibility (ls, &iter, ldata, callback (user_id));
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (ls->list_store), &iter));
}

