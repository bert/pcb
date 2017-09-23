/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998 Thomas Nau
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* 
 * This file written by Bill Wilson for the PCB Gtk port
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "global.h"

#include "create.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "find.h"
#include "misc.h"
#include "mymem.h"
#include "rats.h"
#include "remove.h"
#include "search.h"
#include "select.h"
#include "set.h"
#include "undo.h"

#include "gui.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define NET_HIERARCHY_SEPARATOR "/"

static GtkWidget	*netlist_window;
static GtkWidget	*disable_all_button;

static GtkTreeModel *node_model;
static GtkTreeView *node_treeview;

static gboolean selection_holdoff;

static LibraryMenuType *selected_net;
static LibraryMenuType *node_selected_net;


/* The Netlist window displays all the layout nets in a left treeview
   |  When one of the nets is selected, all of its nodes (or connections)
   |  will be displayed in a right treeview.  If a "Select on layout" button
   |  is pressed, the net that is selected in the left treeview will be
   |  drawn selected on the layout.
   |
   |  Gtk separates the data model from the view in its treeview widgets so
   |  here we maintain two data models.  The net data model has pointers to
   |  all the nets in the layout and the node data model keeps pointers to all
   |  the nodes for the currently selected net.  By updating the data models
   |  the net and node gtk treeviews handle displaying the results.
   |
   |  The netlist window code has a public interface providing hooks so PCB
   |  code can control the net and node treeviews:
   |
   |	ghid_get_net_from_node_name gchar *node_name, gboolean enabled_only)
   |		Given a node name (eg C101-1), walk through the nets in the net
   |		data model and search each net for the given node_name.  If found
   |		and enabled_only is true, make the net treeview scroll to and
   |		highlight (select) the found net.  Return the found net.
   |
   |	ghid_netlist_highlight_node()
   |		Given some PCB internal pointers (not really a good gui api here)
   |		look up a node name determined by the pointers and highlight the node
   |		in the node treeview.  By using ghid_get_net_from_node_name() to
   |		look up the node, the net the node belongs to will also be
   |		highlighted in the net treeview.
   |
   |	ghid_netlist_window_update(gboolean init_nodes)
   |		PCB calls this to tell the gui netlist code the layout net has
   |		changed and the gui data structures (net and optionally node data
   |		models) should be rebuilt.
*/


/* -------- The netlist nodes (LibraryEntryType) data model ----------
   |  Each time a net is selected in the left treeview, this node model
   |  is recreated containing all the nodes (pins/pads) that are connected
   |  to the net.  Loading the new model will update the right treeview with
   |  all the new node names (C100-1, R100-1, etc).
   |
   |  The terminology is a bit confusing because the PCB netlist data
   |  structures are generic structures used for library elements, netlist
   |  data, and possibly other things also.  The mapping is that
   |  the layout netlist data structure is a LibraryType which
   |  contains an allocated array of LibraryMenuType structs.  Each of these
   |  structs represents a net in the netlist and contains an array
   |  of LibraryEntryType structs which represent the nodes connecting to
   |  the net.  So we have: 
   |
   |                      Nets              Nodes
   |       LibraryType    LibraryMenuType   LibraryEntryType
   | -------------------------------------------------------
   |  PCB->NetlistLib------Menu[0]-----------Entry[0]
   |                     |                   Entry[1]
   |                     |                     ...
   |                     |
   |                     --Menu[1]-----------Entry[0]
   |                     |                   Entry[1]
   |                     |                     ...
   |                     |
   |                     -- ...
   |
   | Where for example Menu[] names would be nets GND, Vcc, etc and Entry[]
   | names would be nodes C101-1, R101-2, etc
*/

LibraryEntryType *node_get_node_from_name (gchar * node_name,
					   LibraryMenuType ** node_net);

enum
{
  NODE_NAME_COLUMN,		/* Name to show in the treeview         */
  NODE_LIBRARY_COLUMN,		/* Pointer to this node (LibraryEntryType)      */
  N_NODE_COLUMNS
};

/* Given a net in the netlist (a LibraryMenuType) put all the Entry[]
   |  names (the nodes) into a newly created node tree model.
*/
static GtkTreeModel *
node_model_create (LibraryMenuType * menu)
{
  GtkListStore *store;
  GtkTreeIter iter;

  store = gtk_list_store_new (N_NODE_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);

  if (menu == NULL)
    return GTK_TREE_MODEL (store);

  ENTRY_LOOP (menu);
  {
    if (!entry->ListEntry)
      continue;
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
			NODE_NAME_COLUMN, entry->ListEntry,
			NODE_LIBRARY_COLUMN, entry, -1);
  }
  END_LOOP;

  return GTK_TREE_MODEL (store);
}

/* When there's a new node to display in the node treeview, call this.
   |  Create a new model containing the nodes of the given net, insert
   |  the model into the treeview and unref the old model.
*/
static void
node_model_update (LibraryMenuType * menu)
{
  GtkTreeModel *model;

  model = node_model;
  node_model = node_model_create (menu);
  gtk_tree_view_set_model (node_treeview, node_model);

  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (node_model),
					NODE_NAME_COLUMN, GTK_SORT_ASCENDING);

  /* We could be using gtk_list_store_clear() on the same model, but it's
     |  just as easy that we've created a new one and here unref the old one.
   */
  if (model)
    g_object_unref (G_OBJECT (model));
}

static void
toggle_pin_selected (LibraryEntryType *entry)
{
  ConnectionType conn;

  if (!SeekPad (entry, &conn, false))
    return;

  AddObjectToFlagUndoList (conn.type, conn.ptr1, conn.ptr2, conn.ptr2);
  TOGGLE_FLAG (SELECTEDFLAG, (AnyObjectType *)conn.ptr2);
  DrawObject (conn.type, conn.ptr1, conn.ptr2);
}


/* Callback when the user clicks on a PCB node in the right node treeview.
 */
static void
node_selection_changed_cb (GtkTreeSelection * selection, gpointer data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  LibraryMenuType *node_net;
  LibraryEntryType *node;
  ConnectionType conn;
  Coord x, y;
  static gchar *node_name;

  if (selection_holdoff)	/* PCB is highlighting, user is not selecting */
    return;

  /* Toggle off the previous selection.  Look up node_name to make sure
  |  it still exists.  This toggling can get out of sync if a node is
  |  toggled selected, then the net that includes the node is selected
  |  then unselected.
  */
  if ((node = node_get_node_from_name (node_name, &node_net)) != NULL)
    {
      /* If net node belongs to has been highlighted/unhighighed, toggling
      |  if off here will get our on/off toggling out of sync.
      */
      if (node_net == node_selected_net)
        {
          toggle_pin_selected (node);
          ghid_cancel_lead_user ();
        }
      g_free (node_name);
      node_name = NULL;
    }

  /* Get the selected treeview row.
   */
  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      if (node)
        ghid_invalidate_all ();
      return;
    }

  /* From the treeview row, extract the node pointer stored there and
  |  we've got a pointer to the LibraryEntryType (node) the row
  |  represents.
  */
  gtk_tree_model_get (model, &iter, NODE_LIBRARY_COLUMN, &node, -1);

  dup_string (&node_name, node->ListEntry);
  node_selected_net = selected_net;

  /* Now just toggle a select of the node on the layout
   */
  toggle_pin_selected (node);
  IncrementUndoSerialNumber ();

  /* And lead the user to the location */
  if (SeekPad (node, &conn, false))
    switch (conn.type) {
      case PIN_TYPE:
        {
          PinType *pin = (PinType *) conn.ptr2;
          x = pin->X;
          y = pin->Y;
          gui->set_crosshair (x, y, 0);
          ghid_lead_user_to_location (x, y);
          break;
        }
      case PAD_TYPE:
        {
          PadType *pad = (PadType *) conn.ptr2;
          x = pad->Point1.X + (pad->Point2.X - pad->Point1.X) / 2;
          y = pad->Point1.Y + (pad->Point2.Y - pad->Point1.Y) / 2;
          gui->set_crosshair (x, y, 0);
          ghid_lead_user_to_location (x, y);
          break;
        }
    }
}


/* -------- The net (LibraryMenuType) data model ----------
 */
/* TODO: the enable and disable all nets.  Can't seem to get how that's
   |  supposed to work, but it'll take updating the NET_ENABLED_COLUMN in
   |  the net_model.  Probably it should be made into a gpointer and make
   |  a text renderer for it and just write a '*' or a ' ' similar to the
   |  the Xt PCB scheme.  Or better, since it's an "all nets" function, just
   |  have a "Disable all nets" toggle button and don't mess with the
   |  model/treeview at all.
*/
enum
{
  NET_ENABLED_COLUMN,		/* If enabled will be ' ', if disable '*'       */
  NET_NAME_COLUMN,		/* Name to show in the treeview */
  NET_LIBRARY_COLUMN,		/* Pointer to this net (LibraryMenuType)        */
  N_NET_COLUMNS
};

static GtkTreeModel *net_model = NULL;
static GtkTreeView *net_treeview;

static gboolean		loading_new_netlist;

static GtkTreeModel *
net_model_create (void)
{
  GtkTreeModel *model;
  GtkTreeStore *store;
  GtkTreeIter new_iter;
  GtkTreeIter parent_iter;
  GtkTreeIter *parent_ptr;
  GtkTreePath *path;
  GtkTreeRowReference *row_ref;
  GHashTable *prefix_hash;
  char *display_name;
  char *hash_string;
  char **join_array;
  char **path_segments;
  int path_depth;
  int try_depth;

  store = gtk_tree_store_new (N_NET_COLUMNS,
			      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

  model = GTK_TREE_MODEL (store);

  /* Hash table stores GtkTreeRowReference for given path prefixes */
  prefix_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                       (GDestroyNotify)
                                         gtk_tree_row_reference_free);

  MENU_LOOP (&PCB->NetlistLib);
  {
    if (!menu->Name)
      continue;

    if (loading_new_netlist)
      menu->flag = TRUE;

    parent_ptr = NULL;

    path_segments = g_strsplit (menu->Name, NET_HIERARCHY_SEPARATOR, 0);
    path_depth = g_strv_length (path_segments);

    for (try_depth = path_depth - 1; try_depth > 0; try_depth--)
      {
        join_array = g_new0 (char *, try_depth + 1);
        memcpy (join_array, path_segments, sizeof (char *) * try_depth);

        /* See if this net's parent node is in the hash table */
        hash_string = g_strjoinv (NET_HIERARCHY_SEPARATOR, join_array);
        g_free (join_array);

        row_ref = (GtkTreeRowReference *)g_hash_table_lookup (prefix_hash, hash_string);
        g_free (hash_string);

        /* If we didn't find the path at this level, keep looping */
        if (row_ref == NULL)
          continue;

        path = gtk_tree_row_reference_get_path (row_ref);
        gtk_tree_model_get_iter (model, &parent_iter, path);
        parent_ptr = &parent_iter;
        break;
      }

    /* NB: parent_ptr may still be NULL if we reached the toplevel */

    /* Now walk up the desired path, adding the nodes */

    for (; try_depth < path_depth - 1; try_depth++)
      {
        display_name = g_strconcat (path_segments[try_depth],
                                    NET_HIERARCHY_SEPARATOR, NULL);
        gtk_tree_store_append (store, &new_iter, parent_ptr);
        gtk_tree_store_set (store, &new_iter,
                            NET_ENABLED_COLUMN, "",
                            NET_NAME_COLUMN, display_name,
                            NET_LIBRARY_COLUMN, NULL, -1);
        g_free (display_name);

        path = gtk_tree_model_get_path (model, &new_iter);
        row_ref = gtk_tree_row_reference_new (model, path);
        parent_iter = new_iter;
        parent_ptr = &parent_iter;

        join_array = g_new0 (char *, try_depth + 2);
        memcpy (join_array, path_segments, sizeof (char *) * (try_depth + 1));

        hash_string = g_strjoinv (NET_HIERARCHY_SEPARATOR, join_array);
        g_free (join_array);

        /* Insert those node in the hash table */
        g_hash_table_insert (prefix_hash, hash_string, row_ref);
        /* Don't free hash_string, it is now oened by the hash table */
      }

    gtk_tree_store_append (store, &new_iter, parent_ptr);
    gtk_tree_store_set (store, &new_iter,
			NET_ENABLED_COLUMN, menu->flag ? "" : "*",
			NET_NAME_COLUMN, path_segments[path_depth - 1],
			NET_LIBRARY_COLUMN, menu, -1);
    g_strfreev (path_segments);
  }
  END_LOOP;

  g_hash_table_destroy (prefix_hash);

  return model;
}


/* Called when the user double clicks on a net in the left treeview.
 */
static void
net_selection_double_click_cb (GtkTreeView * treeview, GtkTreePath * path,
			       GtkTreeViewColumn * col, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *str;
  LibraryMenuType *menu;

  model = gtk_tree_view_get_model (treeview);
  if (gtk_tree_model_get_iter (model, &iter, path))
    {

      /* Expand / contract nodes with children */
      if (gtk_tree_model_iter_has_child (model, &iter))
        {
          if (gtk_tree_view_row_expanded (treeview, path))
            gtk_tree_view_collapse_row (treeview, path);
          else
            gtk_tree_view_expand_row (treeview, path, FALSE);
          return;
        }

      /* Get the current enabled string and toggle it between "" and "*"
       */
      gtk_tree_model_get (model, &iter, NET_ENABLED_COLUMN, &str, -1);
      gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
			  NET_ENABLED_COLUMN, !strcmp (str, "*") ? "" : "*",
			  -1);
      /* set/clear the flag which says the net is enabled or disabled */
      gtk_tree_model_get (model, &iter, NET_LIBRARY_COLUMN, &menu, -1);
      menu->flag = strcmp (str, "*") == 0 ? 1 : 0;
      g_free (str);
    }
}

/* Called when the user clicks on a net in the left treeview.
 */
static void
net_selection_changed_cb (GtkTreeSelection * selection, gpointer data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  LibraryMenuType *net;

  if (selection_holdoff)	/* PCB is highlighting, user is not selecting */
    return;

  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      selected_net = NULL;

      return;
    }

  /* Get a pointer, net, to the LibraryMenuType of the newly selected
     |  netlist row, and create a new node model from the net entries
     |  and insert that model into the node view.  Delete old entry model.
   */
  gtk_tree_model_get (model, &iter, NET_LIBRARY_COLUMN, &net, -1);
  node_model_update (net);

  selected_net = net;
}

static void
netlist_disable_all_cb (GtkToggleButton * button, gpointer data)
{
  GtkTreeIter iter;
  gboolean active = gtk_toggle_button_get_active (button);
  LibraryMenuType *menu;

  /* Get each net iter and change the NET_ENABLED_COLUMN to a "*" or ""
     |  to flag it as disabled or enabled based on toggle button state.
   */
  if (gtk_tree_model_get_iter_first (net_model, &iter))
    do
      {
	gtk_tree_store_set (GTK_TREE_STORE (net_model), &iter,
			    NET_ENABLED_COLUMN, active ? "*" : "", -1);
	/* set/clear the flag which says the net is enabled or disabled */
	gtk_tree_model_get (net_model, &iter, NET_LIBRARY_COLUMN, &menu, -1);
	menu->flag = active ? 0 : 1;
      }
    while (gtk_tree_model_iter_next (net_model, &iter));
}

/* Select on the layout the current net treeview selection
 */
static void
netlist_select_cb (GtkWidget * widget, gpointer data)
{
  LibraryEntryType *entry;
  ConnectionType conn;
  gint i;
  gboolean select_flag = GPOINTER_TO_INT (data);

  if (!selected_net)
    return;
  if (selected_net == node_selected_net)
    node_selected_net = NULL;

  InitConnectionLookup ();
  ClearFlagOnAllObjects (true, FOUNDFLAG);

  for (i = selected_net->EntryN, entry = selected_net->Entry; i; i--, entry++)
    if (SeekPad (entry, &conn, false))
      RatFindHook (conn.type, conn.ptr1, conn.ptr2, conn.ptr2, true, FOUNDFLAG, true);

  SelectByFlag (FOUNDFLAG, select_flag);
  ClearFlagOnAllObjects (false, FOUNDFLAG);
  FreeConnectionLookupMemory ();
  IncrementUndoSerialNumber ();
  Draw ();
}

static void
netlist_find_cb (GtkWidget * widget, gpointer data)
{
  char *name = NULL;

  if (!selected_net)
    return;

  name = selected_net->Name + 2;
  hid_actionl ("connection", "reset", NULL);
  hid_actionl ("netlist", "find", name, NULL);
}

static void
netlist_rip_up_cb (GtkWidget * widget, gpointer data)
{

  if (!selected_net)
    return;
  netlist_find_cb(widget, data);

  VISIBLELINE_LOOP (PCB->Data);
  {
    if (TEST_FLAG (FOUNDFLAG, line) && !TEST_FLAG (LOCKFLAG, line))
      RemoveObject (LINE_TYPE, layer, line, line);
  }
  ENDALL_LOOP;

  VISIBLEARC_LOOP (PCB->Data);
  {
    if (TEST_FLAG (FOUNDFLAG, arc) && !TEST_FLAG (LOCKFLAG, arc))
      RemoveObject (ARC_TYPE, layer, arc, arc);
  }
  ENDALL_LOOP;

  if (PCB->ViaOn)
    VIA_LOOP (PCB->Data);
  {
    if (TEST_FLAG (FOUNDFLAG, via) && !TEST_FLAG (LOCKFLAG, via))
      RemoveObject (VIA_TYPE, via, via, via);
  }
  END_LOOP;

}

/**/
typedef struct {
  LibraryEntryType *ret_val;
  LibraryMenuType *node_net;
  const gchar *node_name;
  bool found;
} node_get_node_from_name_state;

static gboolean
node_get_node_from_name_helper (GtkTreeModel *model, GtkTreePath *path,
                                GtkTreeIter *iter, gpointer data)
{
  LibraryMenuType *net;
  LibraryEntryType *node;
  node_get_node_from_name_state *state = data;

  gtk_tree_model_get (net_model, iter, NET_LIBRARY_COLUMN, &net, -1);
  /* Ignore non-nets (category headers) */
  if (net == NULL)
    return FALSE;

  /* Look for the node name in this net. */
  for (node = net->Entry; node - net->Entry < net->EntryN; node++)
    if (node->ListEntry && !strcmp (state->node_name, node->ListEntry))
      {
        state->node_net = net;
        state->ret_val = node;
        /* stop iterating */
        state->found = TRUE;
	return TRUE;
      }
  return FALSE;
}

LibraryEntryType *
node_get_node_from_name (gchar * node_name, LibraryMenuType ** node_net)
{
  node_get_node_from_name_state state;

  if (!node_name)
    return NULL;

  /* Have to force the netlist window created because we need the treeview
     |  models constructed to do the search.
   */
  ghid_netlist_window_create (gport);

  /* Now walk through node entries of each net in the net model looking for
     |  the node_name.
   */
  state.found = 0;
  state.node_name = node_name;
  gtk_tree_model_foreach (net_model, node_get_node_from_name_helper, &state);
  if (state.found)
    {
      if (node_net)
        *node_net = state.node_net;
      return state.ret_val;
    }
  return NULL;
}
/**/

/* ---------- Manage the GUI treeview of the data models -----------
 */
static gint
netlist_window_configure_event_cb (GtkWidget * widget, GdkEventConfigure * ev,
				   gpointer data)
{
  GtkAllocation allocation;

  gtk_widget_get_allocation (widget, &allocation);
  ghidgui->netlist_window_width = allocation.width;
  ghidgui->netlist_window_height = allocation.height;
  ghidgui->config_modified = TRUE;
  return FALSE;
}

static void
netlist_close_cb (GtkWidget * widget, gpointer data)
{
  gtk_widget_destroy (netlist_window);
  selected_net = NULL;
  netlist_window = NULL;

  /* For now, we are the only consumer of this API, so we can just do this */
  ghid_cancel_lead_user ();
}


static void
netlist_destroy_cb (GtkWidget * widget, GHidPort * out)
{
  selected_net = NULL;
  netlist_window = NULL;
}

void
ghid_netlist_window_create (GHidPort * out)
{
  GtkWidget *vbox, *hbox, *button, *label, *sep;
  GtkTreeView *treeview;
  GtkTreeModel *model;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  if (netlist_window)
    return;

  netlist_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (netlist_window), "destroy",
		    G_CALLBACK (netlist_destroy_cb), out);
  gtk_window_set_title (GTK_WINDOW (netlist_window), _("PCB Netlist"));
  gtk_window_set_wmclass (GTK_WINDOW (netlist_window), "PCB_Netlist", "PCB");
  g_signal_connect (G_OBJECT (netlist_window), "configure_event",
		    G_CALLBACK (netlist_window_configure_event_cb), NULL);
  gtk_window_resize (GTK_WINDOW (netlist_window),
                     ghidgui->netlist_window_width,
                     ghidgui->netlist_window_height);

  gtk_container_set_border_width (GTK_CONTAINER (netlist_window), 2);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (netlist_window), vbox);
  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 4);


  model = net_model_create ();
  treeview = GTK_TREE_VIEW (gtk_tree_view_new_with_model (model));
  net_model = model;
  net_treeview = treeview;
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (net_model),
					NET_NAME_COLUMN, GTK_SORT_ASCENDING);

  gtk_tree_view_set_rules_hint (treeview, FALSE);
  g_object_set (treeview, "enable-tree-lines", TRUE, NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (treeview, -1, _(" "),
					       renderer,
					       "text", NET_ENABLED_COLUMN,
					       NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Net Name"),
						     renderer,
						     "text", NET_NAME_COLUMN, NULL);
  gtk_tree_view_insert_column (treeview, column, -1);
  gtk_tree_view_set_expander_column (treeview, column);

  /* TODO: dont expand all, but record expanded states when window is
     |  destroyed and restore state here.
   */
  gtk_tree_view_expand_all (treeview);

  ghid_scrolled_selection (treeview, hbox,
                           GTK_SELECTION_SINGLE,
                           GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC,
                           net_selection_changed_cb, NULL);

  /* Connect to the double click event.
   */
  g_signal_connect (G_OBJECT (treeview), "row-activated",
		    G_CALLBACK (net_selection_double_click_cb), NULL);



  /* Create the elements treeview and wait for a callback to populate it.
   */
  treeview = GTK_TREE_VIEW (gtk_tree_view_new ());
  node_treeview = treeview;

  gtk_tree_view_set_rules_hint (treeview, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (treeview, -1, _("Nodes"),
					       renderer,
					       "text", NODE_NAME_COLUMN,
					       NULL);

  ghid_scrolled_selection (treeview, hbox,
                           GTK_SELECTION_SINGLE,
                           GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC,
                           node_selection_changed_cb, NULL);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Operations on selected 'Net Name':"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 4);

  button = gtk_button_new_with_label (C_("netlist", "Select"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (netlist_select_cb), GINT_TO_POINTER (1));

  button = gtk_button_new_with_label (_("Unselect"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (netlist_select_cb), GINT_TO_POINTER (0));

  button = gtk_button_new_with_label (_("Find"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (netlist_find_cb), GINT_TO_POINTER (0));

  button = gtk_button_new_with_label (_("Rip Up"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (netlist_rip_up_cb), GINT_TO_POINTER (0));
  
  ghid_check_button_connected (vbox, &disable_all_button, FALSE, TRUE, FALSE,
			       FALSE, 0, netlist_disable_all_cb, NULL,
			       _("Disable all nets for adding rats"));

  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 3);

  hbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_END);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 4);
  button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (netlist_close_cb), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);


  gtk_widget_realize (netlist_window);
  if (Settings.AutoPlace)
    gtk_window_move (GTK_WINDOW (netlist_window), 10, 10);

}

void
ghid_netlist_window_show (GHidPort * out, gboolean raise)
{
  ghid_netlist_window_create (out);
  gtk_widget_show_all (netlist_window);
  ghid_netlist_window_update (TRUE);
  if (raise)
    gtk_window_present(GTK_WINDOW(netlist_window));
}

struct ggnfnn_task {
  gboolean enabled_only;
  gchar *node_name;
  LibraryMenuType *found_net;
  GtkTreeIter iter;
};

static gboolean
hunt_named_node (GtkTreeModel *model, GtkTreePath *path,
                 GtkTreeIter *iter, gpointer data)
{
  struct ggnfnn_task *task = (struct ggnfnn_task *)data;
  LibraryMenuType *net;
  LibraryEntryType *node;
  gchar *str;
  gint j;
  gboolean is_disabled;

  /* We only want to inspect leaf nodes in the tree */
  if (gtk_tree_model_iter_has_child (model, iter))
    return FALSE;

  gtk_tree_model_get (model, iter, NET_LIBRARY_COLUMN, &net, -1);
  gtk_tree_model_get (model, iter, NET_ENABLED_COLUMN, &str, -1);
  is_disabled = !strcmp (str, "*");
  g_free (str);

  /* Don't check net nodes of disabled nets. */
  if (task->enabled_only && is_disabled)
    return FALSE;

  /* Look for the node name in this net. */
  for (j = net->EntryN, node = net->Entry; j; j--, node++)
    if (node->ListEntry && !strcmp (task->node_name, node->ListEntry))
      {
        task->found_net = net;
        task->iter = *iter;
        return TRUE;
      }

  return FALSE;
}

LibraryMenuType *
ghid_get_net_from_node_name (gchar * node_name, gboolean enabled_only)
{
  GtkTreePath *path;
  struct ggnfnn_task task;

  if (!node_name)
    return NULL;

  /* Have to force the netlist window created because we need the treeview
     |  models constructed so we can find the LibraryMenuType pointer the
     |  caller wants.
   */
  ghid_netlist_window_create (gport);

  /* If no netlist is loaded the window doesn't appear. */
  if (netlist_window == NULL)
    return NULL;

  task.enabled_only = enabled_only;
  task.node_name = node_name;
  task.found_net = NULL;

  /* Now walk through node entries of each net in the net model looking for
     |  the node_name.
   */
  gtk_tree_model_foreach (net_model, hunt_named_node, &task);

  /* We are asked to highlight the found net if enabled_only is TRUE.
     |  Set holdoff TRUE since this is just a highlight and user is not
     |  expecting normal select action to happen?  Or should the node
     |  treeview also get updated?  Original PCB code just tries to highlight.
   */
  if (task.found_net && enabled_only)
    {
      selection_holdoff = TRUE;
      path = gtk_tree_model_get_path (net_model, &task.iter);
      gtk_tree_view_scroll_to_cell (net_treeview, path, NULL, TRUE, 0.5, 0.5);
      gtk_tree_selection_select_path (gtk_tree_view_get_selection
				      (net_treeview), path);
      selection_holdoff = FALSE;
    }
  return task.found_net;
}

/* PCB LookupConnection code in find.c calls this if it wants a node
   |  and its net highlighted.
*/
void
ghid_netlist_highlight_node (gchar * node_name)
{
  GtkTreePath *path;
  GtkTreeIter iter;
  LibraryMenuType *net;
  gchar *name;

  if (!node_name)
    return;

  if ((net = ghid_get_net_from_node_name (node_name, TRUE)) == NULL)
    return;

  /* We've found the net containing the node, so update the node treeview
     |  to contain the nodes from the net.  Then we have to find the node
     |  in the new node model so we can highlight it.
   */
  node_model_update (net);

  if (gtk_tree_model_get_iter_first (node_model, &iter))
    do
      {
	gtk_tree_model_get (node_model, &iter, NODE_NAME_COLUMN, &name, -1);

	if (!strcmp (node_name, name))
	  {			/* found it, so highlight it */
	    selection_holdoff = TRUE;
	    selected_net = net;
	    path = gtk_tree_model_get_path (node_model, &iter);
	    gtk_tree_view_scroll_to_cell (node_treeview, path, NULL,
					  TRUE, 0.5, 0.5);
	    gtk_tree_selection_select_path (gtk_tree_view_get_selection
					    (node_treeview), path);
	    selection_holdoff = FALSE;
	  }
	g_free (name);
      }
    while (gtk_tree_model_iter_next (node_model, &iter));
}

/* If code in PCB should change the netlist, call this to update
   |  what's in the netlist window.
*/
void
ghid_netlist_window_update (gboolean init_nodes)
{
  GtkTreeModel *model;

  /* Make sure there is something to update */
  ghid_netlist_window_create (gport);

  model = net_model;
  net_model = net_model_create ();
  gtk_tree_view_set_model (net_treeview, net_model);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (net_model),
					NET_NAME_COLUMN, GTK_SORT_ASCENDING);
  if (model)
    {
      gtk_tree_store_clear (GTK_TREE_STORE (model));
      g_object_unref (model);
    }

  selected_net = NULL;

  /* XXX Check if the select callback does this for us */
  if (init_nodes)
    node_model_update ((&PCB->NetlistLib)->Menu);
}

static gint
GhidNetlistChanged (int argc, char **argv, Coord x, Coord y)
{
  /* XXX: We get called before the GUI is up when
   *         exporting from the command-line. */
  if (ghidgui == NULL || !ghidgui->is_up)
    return 0;

  /* There is no need to update if the netlist window isn't open */
  if (netlist_window == NULL)
    return 0;

  loading_new_netlist = TRUE;
  ghid_netlist_window_update (TRUE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (disable_all_button),
				FALSE);
  loading_new_netlist = FALSE;
  return 0;
}

static const char netlistshow_syntax[] =
"NetlistShow(pinname|netname)";

static const char netlistshow_help[] =
"Selects the given pinname or netname in the netlist window. Does not \
show the window if it isn't already shown.";

static gint
GhidNetlistShow (int argc, char **argv, Coord x, Coord y)
{
  ghid_netlist_window_create (gport);
  if (argc > 0)
    ghid_netlist_highlight_node(argv[0]);
  return 0;
}

static const char netlistpresent_syntax[] =
"NetlistPresent()";

static const char netlistpresent_help[] =
"Presents the netlist window.";

static gint
GhidNetlistPresent (int argc, char **argv, Coord x, Coord y)
{
  ghid_netlist_window_show (gport, TRUE);
  return 0;
}

HID_Action ghid_netlist_action_list[] = {
  {"NetlistChanged", 0, GhidNetlistChanged,
   netlistchanged_help, netlistchanged_syntax},
  {"NetlistShow", 0, GhidNetlistShow,
   netlistshow_help, netlistshow_syntax},
  {"NetlistPresent", 0, GhidNetlistPresent,
   netlistpresent_help, netlistpresent_syntax}
  ,
};

REGISTER_ACTIONS (ghid_netlist_action_list)
