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
#include "search.h"
#include "select.h"
#include "set.h"
#include "undo.h"

#include "gui.h"


static GtkWidget		*netlist_window;

static GtkTreeModel		*node_model;
static GtkTreeView		*node_treeview;
static GtkTreeSelection	*node_selection;

static gboolean			selection_holdoff;

static LibraryMenuType	*selected_net;
static LibraryMenuType	*node_selected_net;


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
|	gui_get_net_from_node_name gchar *node_name, gboolean enabled_only)
|		Given a node name (eg C101-1), walk through the nets in the net
|		data model and search each net for the given node_name.  If found
|		and enabled_only is true, make the net treeview scroll to and
|		highlight (select) the found net.  Return the found net.
|
|	gui_netlist_highlight_node()
|		Given some PCB internal pointers (not really a good gui api here)
|		look up a node name determined by the pointers and highlight the node
|		in the node treeview.  By using gui_get_net_from_node_name() to
|		look up the node, the net the node belongs to will also be
|		highlighted in the net treeview.
|
|	gui_netlist_window_update(gboolean init_nodes)
|		PCB calls this to tell the gui netlist code the layout net has
|		changed and the gui data structures (net and optionally node data
|		models) should be rebuilt.
|
|   gui_netlist_nodes_update(LibraryMenuType *net)
|		Called when the node model should be updated to a netlist.
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

LibraryEntryType	*node_get_node_from_name(gchar *node_name,
							LibraryMenuType **node_net);

enum
	{
	NODE_NAME_COLUMN,		/* Name to show in the treeview		*/
	NODE_LIBRARY_COLUMN,	/* Pointer to this node (LibraryEntryType)	*/
	N_NODE_COLUMNS
	};

  /* Given a net in the netlist (a LibraryMenuType) put all the Entry[]
  |  names (the nodes) into a newly created node tree model.
  */
static GtkTreeModel *
node_model_create(LibraryMenuType *menu)
	{
	GtkListStore	*store;
	GtkTreeIter		iter;

	store = gtk_list_store_new(N_NODE_COLUMNS,
				G_TYPE_STRING,
				G_TYPE_POINTER);

	ENTRY_LOOP(menu);
		{
		if (!entry->ListEntry)
			continue;
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				NODE_NAME_COLUMN, entry->ListEntry,
				NODE_LIBRARY_COLUMN, entry,
				-1);
		}
	END_LOOP;

	return GTK_TREE_MODEL(store);
	}

  /* When there's a new node to display in the node treeview, call this.
  |  Create a new model containing the nodes of the given net, insert
  |  the model into the treeview and unref the old model.
  */
static void
node_model_update(LibraryMenuType *menu)
	{
	GtkTreeModel	*model;

	model = node_model;
	node_model = node_model_create(menu);
	gtk_tree_view_set_model(node_treeview, node_model);

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(node_model),
					NODE_NAME_COLUMN, GTK_SORT_ASCENDING);

	/* We could be using gtk_list_store_clear() on the same model, but it's
	|  just as easy that we've created a new one and here unref the old one.
	*/
	if (model)
		g_object_unref(G_OBJECT(model));
	}


  /* Callback when the user clicks on a PCB node in the right node treeview.
  */
static void
node_selection_changed_cb(GtkTreeSelection *selection, gpointer data)
	{
	GtkTreeIter			iter;
	GtkTreeModel		*model;
	LibraryMenuType		*node_net;
	LibraryEntryType	*node;
	static gchar		*node_name;

	if (selection_holdoff)	/* PCB is highlighting, user is not selecting */
		return;

	/* Toggle off the previous selection.  Look up node_name to make sure
	|  it still exists.  This toggling can get out of sync if a node is
	|  toggled selected, then the net that includes the node is selected
	|  then unselected.
	*/
	if ((node = node_get_node_from_name(node_name, &node_net)) != NULL)
		{
		/* If net node belongs to has been highlighted/unhighighed, toggling
		|  if off here will get our on/off toggling out of sync.
		*/
		if (node_net == node_selected_net)
			SelectPin(node, TRUE);
		g_free(node_name);
		node_name = NULL;
		}

	/* Get the selected treeview row.
	*/
	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		{
		if (node)
			Draw();
		return;
		}

	/* From the treeview row, extract the node pointer stored there and
	|  we've got a pointer to the LibraryEntryType (node) the row
	|  represents.
	*/
	gtk_tree_model_get(model, &iter, NODE_LIBRARY_COLUMN, &node, -1);

	dup_string(&node_name, node->ListEntry);
	node_selected_net = selected_net;

	/* Original Xt checks CallData for "Kill".  How can that happen? XXX */

	/* Now just toggle a select of the node on the layout and redraw.
	*/
	SelectPin(node, TRUE);		/* Also centers the display on the node */
	IncrementUndoSerialNumber();
	Draw();
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
	NET_ENABLED_COLUMN,		/* If enabled will be ' ', if disable '*'	*/
	NET_NAME_COLUMN,		/* Name to show in the treeview	*/
	NET_LIBRARY_COLUMN,		/* Pointer to this net (LibraryMenuType)	*/
	N_NET_COLUMNS
	};

static GtkTreeModel		*net_model;
static GtkTreeView		*net_treeview;


static GtkTreeModel *
net_model_create(void)
	{
	GtkListStore	*store;
	GtkTreeIter		iter;

	store = gtk_list_store_new(N_NET_COLUMNS,
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_POINTER);

	MENU_LOOP(&PCB->NetlistLib);
		{
		if (!menu->Name)
			continue;
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				NET_ENABLED_COLUMN, "",
				NET_NAME_COLUMN, menu->Name,
				NET_LIBRARY_COLUMN, menu,
				-1);
		}
	END_LOOP;

	return GTK_TREE_MODEL(store);
	}


  /* Called when the user double clicks on a net in the left treeview.
  */
static void
net_selection_double_click_cb(GtkTreeView *treeview, GtkTreePath *path,
			GtkTreeViewColumn *col, gpointer data)
	{
	GtkTreeModel	*model;
	GtkTreeIter		iter;
	gchar			*str;

	model = gtk_tree_view_get_model(treeview);
	if (gtk_tree_model_get_iter(model, &iter, path))
		{
		/* Get the current enabled string and toggle it between "" and "*"
		*/
		gtk_tree_model_get(model, &iter, NET_ENABLED_COLUMN, &str, -1);
		gtk_list_store_set(GTK_LIST_STORE(model), &iter,
					NET_ENABLED_COLUMN, !strcmp(str, "*") ? "" : "*",
					-1);
		g_free(str);
		}
	}

  /* Called when the user clicks on a net in the left treeview.
  */
static void
net_selection_changed_cb(GtkTreeSelection *selection, gpointer data)
	{
	GtkTreeIter		iter;
	GtkTreeModel	*model;
	LibraryMenuType	*net;

	if (selection_holdoff)	/* PCB is highlighting, user is not selecting */
		return;

	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		{
		selected_net = NULL;
		
		return;
		}

	/* Get a pointer, net, to the LibraryMenuType of the newly selected
	|  netlist row, and create a new node model from the net entries
	|  and insert that model into the node view.  Delete old entry model.
	*/
	gtk_tree_model_get(model, &iter, NET_LIBRARY_COLUMN, &net, -1);
	node_model_update(net);

	selected_net = net;
	}

static void
netlist_disable_all_cb(GtkToggleButton *button, gpointer data)
	{
	GtkTreeIter		iter;
	gboolean		active = gtk_toggle_button_get_active(button);

	/* Get each net iter and change the NET_ENABLED_COLUMN to a "*" or ""
	|  to flag it as disabled or enabled based on toggle button state.
	*/
	if (gtk_tree_model_get_iter_first(net_model, &iter))
		do
			{
			gtk_list_store_set(GTK_LIST_STORE(net_model), &iter,
					NET_ENABLED_COLUMN, active ? "*" : "",
					-1);
			}
		while (gtk_tree_model_iter_next(net_model, &iter));
	}

  /* Select on the layout the current net treeview selection
  */
static void
netlist_select_cb(GtkWidget *widget, gpointer data)
	{
	LibraryEntryType	*entry;
	ConnectionType		conn;
	gint				i;
	gboolean			select_flag = GPOINTER_TO_INT(data);

	if (!selected_net)
		return;
	if (selected_net == node_selected_net)
		node_selected_net = NULL;

	InitConnectionLookup();
	ResetFoundPinsViasAndPads(False);
	ResetFoundLinesAndPolygons(False);
	SaveUndoSerialNumber();

	for (i = selected_net->EntryN, entry = selected_net->Entry; i;
				i--, entry++)
		if (SeekPad(entry, &conn, False))
			RatFindHook (conn.type, conn.ptr1, conn.ptr2, conn.ptr2,
						True, True);
	RestoreUndoSerialNumber();
	SelectConnection(select_flag);
	ResetFoundPinsViasAndPads(False);
	ResetFoundLinesAndPolygons(False);
	FreeConnectionLookupMemory();
	IncrementUndoSerialNumber();
	Draw();
	}

LibraryEntryType *
node_get_node_from_name(gchar *node_name, LibraryMenuType **node_net)
	{
	GtkTreeIter			iter;
	LibraryMenuType		*net;
	LibraryEntryType	*node;
	gint				j;

	if (!node_name)
		return NULL;

	/* Have to force the netlist window shown because we need the treeview
	|  models constructed to do the search.
	*/
	if (!netlist_window)
		gui_netlist_window_show(&Output);

	while (gtk_events_pending())	/* Make sure everything gets built */
    	gtk_main_iteration();

	/* Now walk through node entries of each net in the net model looking for
	|  the node_name.
	*/
	if (gtk_tree_model_get_iter_first(net_model, &iter))
		do
			{
			gtk_tree_model_get(net_model, &iter, NET_LIBRARY_COLUMN, &net, -1);

			/* Look for the node name in this net.
			*/
			for (j = net->EntryN, node = net->Entry; j; j--, node++)
				if (node->ListEntry && !strcmp(node_name, node->ListEntry))
					{
					if (node_net)
						*node_net = net;
					return node;
					}
			}
		while (gtk_tree_model_iter_next(net_model, &iter));

	return NULL;
	}

  /* ---------- Manage the GUI treeview of the data models -----------
  */
static gint
netlist_window_configure_event_cb(GtkWidget *widget, GdkEventConfigure *ev,
                gpointer data)
	{
	Settings.netlist_window_height = widget->allocation.height;
	Settings.config_modified = TRUE;
	return FALSE;
    }

static void
netlist_close_cb(GtkWidget *widget, gpointer data)
	{
	gtk_widget_destroy(netlist_window);
	selected_net = NULL;
	netlist_window = NULL;
	}


static void
netlist_destroy_cb(GtkWidget *widget, OutputType *out)
    {
	selected_net = NULL;
    netlist_window = NULL;
    }

void
gui_netlist_window_show(OutputType *out)
	{
	GtkWidget		*vbox, *hbox, *button, *label, *sep;
	GtkTreeView		*treeview;
	GtkTreeModel	*model;
	GtkTreeSelection *selection;
	GtkCellRenderer	*renderer;

	/* No point in putting up the window if no netlist is loaded.
	*/
	if (!PCB->NetlistLib.MenuN)
		return;

	if (netlist_window)
		{
		/* gtk_window_present() grabs focus which we don't want
		*/
		gdk_window_raise(netlist_window->window);
		gui_netlist_window_update(TRUE);
		return;
		}
	netlist_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(netlist_window), "destroy",
				G_CALLBACK(netlist_destroy_cb), out);
	gtk_window_set_title(GTK_WINDOW(netlist_window),
				_("PCB Netlist"));
	gtk_window_set_wmclass(GTK_WINDOW(netlist_window),
				"PCB_Netlist", "PCB");
	g_signal_connect(G_OBJECT(netlist_window), "configure_event",
				G_CALLBACK(netlist_window_configure_event_cb), NULL);
	gtk_window_set_default_size(GTK_WINDOW(netlist_window),
				-1, Settings.netlist_window_height);

	gtk_container_set_border_width(GTK_CONTAINER(netlist_window), 2);

	vbox = gtk_vbox_new(FALSE, 4);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	gtk_container_add(GTK_CONTAINER(netlist_window), vbox);
	hbox = gtk_hbox_new(FALSE, 8);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE,TRUE, 4);


	model = net_model_create();
	treeview = GTK_TREE_VIEW(gtk_tree_view_new_with_model(model));
	net_model = model;
	net_treeview = treeview;
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(net_model),
					NET_NAME_COLUMN, GTK_SORT_ASCENDING);

	gtk_tree_view_set_rules_hint(treeview, FALSE);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(treeview, -1, _(" "),
				renderer,
				"text", NET_ENABLED_COLUMN, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(treeview, -1, _("Net Name"),
				renderer,
				"text", NET_NAME_COLUMN, NULL);

	/* TODO: dont expand all, but record expanded states when window is
	|  destroyed and restore state here.
	*/
	gtk_tree_view_expand_all(treeview);

	selection = gui_scrolled_selection(treeview, hbox,
				GTK_SELECTION_SINGLE,
				GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC,
				net_selection_changed_cb, NULL);

	/* Connect to the double click event.
	*/
	g_signal_connect(G_OBJECT(treeview), "row-activated",
				G_CALLBACK(net_selection_double_click_cb), NULL);



	/* Create the elements treeview and wait for a callback to populate it.
	*/
	treeview = GTK_TREE_VIEW(gtk_tree_view_new());
	node_treeview = treeview;

	gtk_tree_view_set_rules_hint(treeview, FALSE);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(treeview, -1, _("Nodes"),
				renderer,
				"text", NODE_NAME_COLUMN, NULL);

	selection = gui_scrolled_selection(treeview, hbox,
				GTK_SELECTION_SINGLE,
				GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC,
				node_selection_changed_cb, NULL);
	node_selection = selection;

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	label = gtk_label_new(_("Operations on selected 'Net Name':"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 4);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
	button = gtk_button_new_with_label(_("Select on Layout"));
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked",
				G_CALLBACK(netlist_select_cb), GINT_TO_POINTER(1));
	button = gtk_button_new_with_label(_("Unselect on Layout"));
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked",
				G_CALLBACK(netlist_select_cb), GINT_TO_POINTER(0));

	gui_check_button_connected(vbox, NULL, FALSE, TRUE, FALSE,
				FALSE, 0, netlist_disable_all_cb, NULL,
				_("Disable all nets for adding rats"));

	sep = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 3);

	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(button), "clicked",
				G_CALLBACK(netlist_close_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);


	/* If focus were grabbed, output drawing area would loose it which we
	|  don't want.
	*/
	gtk_widget_realize(netlist_window);
	gdk_window_set_accept_focus(netlist_window->window, FALSE);
	gtk_widget_show_all(netlist_window);
	}


LibraryMenuType *
gui_get_net_from_node_name(gchar *node_name, gboolean enabled_only)
	{
	GtkTreePath			*path;
	GtkTreeIter			iter;
	LibraryMenuType		*net, *found_net = NULL;
	LibraryEntryType	*node;
	gchar				*str;
	gint				j;
	gboolean			is_disabled;

	if (!node_name)
		return NULL;

	/* Have to force the netlist window shown because we need the treeview
	|  models constructed so we can find the LibraryMenuType pointer the
	|  caller wants.
	*/
	if (!netlist_window)
		gui_netlist_window_show(&Output);

	while (gtk_events_pending())	/* Make sure everything gets built */
    	gtk_main_iteration();

	/* Now walk through node entries of each net in the net model looking for
	|  the node_name.  Don't check net nodes of disabled nets.
	*/
	if (gtk_tree_model_get_iter_first(net_model, &iter))
		do
			{
			gtk_tree_model_get(net_model, &iter, NET_LIBRARY_COLUMN, &net, -1);
			gtk_tree_model_get(net_model, &iter, NET_ENABLED_COLUMN, &str, -1);
			is_disabled = !strcmp(str, "*");
			g_free(str);
			if (enabled_only && is_disabled)
				continue;

			/* Look for the node name in this net.
			*/
			for (j = net->EntryN, node = net->Entry; j; j--, node++)
				if (node->ListEntry && !strcmp(node_name, node->ListEntry))
					{
					found_net = net;
					break;
					}
			if (found_net)
				break;
			}
		while (gtk_tree_model_iter_next(net_model, &iter));

	/* We are asked to highlight the found net if enabled_only is TRUE.
	|  Set holdoff TRUE since this is just a highlight and user is not
	|  expecting normal select action to happen?  Or should the node
	|  treeview also get updated?  Original PCB code just tries to highlight.
	*/
	if (found_net && enabled_only)
		{
		selection_holdoff = TRUE;
		path = gtk_tree_model_get_path(net_model, &iter);
		gtk_tree_view_scroll_to_cell(net_treeview, path, NULL, TRUE, 0.5, 0.5);
		gtk_tree_selection_select_path(
				gtk_tree_view_get_selection(net_treeview), path);
		selection_holdoff = FALSE;
		}
	return found_net;
	}

  /* PCB LookupConnection code in find.c calls this if it wants a node
  |  and its net highlighted.
  */
void
gui_netlist_highlight_node(gchar *node_name, gboolean change_style)
	{
	GtkTreePath			*path;
	GtkTreeIter			iter;
	LibraryMenuType		*net;;
	gchar				*name;

	if (!node_name)
		return;

	if ((net = gui_get_net_from_node_name(node_name, change_style)) == NULL)
		return;

	/* Original PCB code changes the route style here.  Is that really
	|  what the user wants if he just looks up connections to an object?
	*/
	if (change_style)
		{
		SetRouteStyle(net->Style);
		gui_route_style_buttons_update();
		}

	/* We've found the net containing the node, so update the node treeview
	|  to contain the nodes from the net.  Then we have to find the node
	|  in the new node model so we can highlight it.
	*/
	node_model_update(net);

	if (gtk_tree_model_get_iter_first(node_model, &iter))
		do
			{
			gtk_tree_model_get(node_model, &iter, NODE_NAME_COLUMN, &name, -1);

			if (!strcmp(node_name, name))
				{	/* found it, so highlight it */
				selection_holdoff = TRUE;
				selected_net = net;
				path = gtk_tree_model_get_path(node_model, &iter);
				gtk_tree_view_scroll_to_cell(node_treeview, path, NULL,
							TRUE, 0.5, 0.5);
				gtk_tree_selection_select_path(
							gtk_tree_view_get_selection(node_treeview), path);
				selection_holdoff = FALSE;
				}
			g_free(name);
			}
		while (gtk_tree_model_iter_next(node_model, &iter));
	}

void
gui_netlist_nodes_update(LibraryMenuType *net)
	{
	node_model_update(net);
	}

  /* If code in PCB should change the netlist, call this to update
  |  what's in the netlist window.
  */
void
gui_netlist_window_update(gboolean init_nodes)
	{
	GtkTreeModel	*model;

	if (!netlist_window)
		{
		gui_netlist_window_show(&Output);
		return;
		}
	model = net_model;
	net_model = net_model_create();
	gtk_tree_view_set_model(net_treeview, net_model);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(net_model),
					NET_NAME_COLUMN, GTK_SORT_ASCENDING);
	if (model)
		{
		gtk_list_store_clear(GTK_LIST_STORE(model));
		g_object_unref(model);
		}

	selected_net = NULL;

	/* XXX Check if the select callback does this for us */
	if (init_nodes)
		node_model_update((&PCB->NetlistLib)->Menu);
	}

