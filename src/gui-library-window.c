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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* This file written by Bill Wilson for the PCB Gtk port
*/

#include "gui.h"
#include "global.h"
#include "buffer.h"
#include "data.h"
#include "set.h"


static GtkWidget		*library_window;
static GtkTreeModel		*entry_model;
static GtkTreeView		*entry_treeview;
static GtkTreeSelection	*entry_selection;



  /* -------- The library entries (LibraryEntryType) data model ----------
  */
enum
	{
	ENTRY_NAME_COLUMN,		/* Name to show in the treeview		*/
	ENTRY_LIBRARY_COLUMN,	/* Pointer to this LibraryEntryType	*/
	N_ENTRY_COLUMNS
	};

static GtkTreeModel *
entry_model_create(LibraryMenuType *menu)
	{
	GtkListStore	*store;
	GtkTreeIter		iter;

	store = gtk_list_store_new(N_ENTRY_COLUMNS,
				G_TYPE_STRING,
				G_TYPE_POINTER);

	ENTRY_LOOP(menu);
		{
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				ENTRY_NAME_COLUMN, entry->ListEntry,
				ENTRY_LIBRARY_COLUMN, entry,
				-1);
		}
	END_LOOP;

	return GTK_TREE_MODEL(store);
	}


  /* Callback when the user clicks on a PCB element in the right treeview.
  */
static void
entry_tree_selection_changed_cb(GtkTreeSelection *selection, gpointer data)
	{
	GtkTreeIter			iter;
	GtkTreeModel		*model;
	GtkTreePath			*path;
	LibraryEntryType	*entry;
	gchar				*m4_args;

	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;

	/* Get the LibraryEntryType entry from the currently selected row of
	|  of the entry_model.
	*/
	path = gtk_tree_model_get_path(model, &iter);
	gtk_tree_model_get(model, &iter, ENTRY_LIBRARY_COLUMN, &entry, -1);

	/* -1 flags this is an element file part and the file path is in
	|  entry->AllocateMemory.
	*/
	if (entry->Template == (char *) -1)
		{
		if (LoadElementToBuffer (PASTEBUFFER, entry->AllocatedMemory, True))
			SetMode (PASTEBUFFER_MODE);
		return;
		}

	/* Otherwise, it's a m4 element and we need to create a string of
	|  macro arguments to be passed to the library command in
	|  LoadElementToBuffer()
	*/
	m4_args = g_strdup_printf("'%s' '%s' '%s'", EMPTY (entry->Template),
					EMPTY (entry->Value), EMPTY (entry->Package));

	if (LoadElementToBuffer (PASTEBUFFER, m4_args, False))
		SetMode (PASTEBUFFER_MODE);
	g_free(m4_args);
	}


  /* -------- The library group (LibraryMenuType) data model ----------
  */
enum
	{
	MENU_NAME_COLUMN,		/* Name to show in the treeview		*/
	MENU_LIBRARY_COLUMN,	/* Pointer to this LibraryMenuType	*/
	N_MENU_COLUMNS
	};

static GtkTreeModel *
menu_model_create(void)
	{
	GtkTreeStore	*tree;
	GtkTreeIter		iter, piter;
	gchar			*dir = NULL;

	tree = gtk_tree_store_new(N_MENU_COLUMNS,
				G_TYPE_STRING,
				G_TYPE_POINTER);

	MENU_LOOP(&Library);
		{
		/* Watch for directory changes of library parts and create new
		|  parent iter at each change.
		*/
		if (!dir && !menu->directory)	/* Shouldn't happen */
			{
			dir = "???";
			gtk_tree_store_append(tree, &piter, NULL);
			gtk_tree_store_set(tree, &piter,
					MENU_NAME_COLUMN, dir,
					-1);
			}
		if (menu->directory && (!dir || strcmp(dir, menu->directory)))
			{
			dir = menu->directory;
			gtk_tree_store_append(tree, &piter, NULL);
			gtk_tree_store_set(tree, &piter,
					MENU_NAME_COLUMN, dir,
					-1);
			}
		gtk_tree_store_append(tree, &iter, &piter);
		gtk_tree_store_set(tree, &iter,
				MENU_NAME_COLUMN, menu->Name,
				MENU_LIBRARY_COLUMN, menu,
				-1);
		}
	END_LOOP;

	return GTK_TREE_MODEL(tree);
	}

  /* Called when the user clicks on a new library group in the left treeview.
  */
static void
menu_tree_selection_changed_cb(GtkTreeSelection *selection, gpointer data)
	{
	GtkTreeIter		iter;
	GtkTreeModel	*model;
	GtkTreePath		*path;
	LibraryMenuType	*menu;
	gint			depth;

	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;
	path = gtk_tree_model_get_path(model, &iter);
	depth = gtk_tree_path_get_depth(path);
	gtk_tree_path_free(path);

	if (depth == 1)
		return;			/* On a directory item and not a group */

	gtk_tree_model_get(model, &iter, MENU_LIBRARY_COLUMN, &menu, -1);

	/* Got a pointer, menu, to the LibraryMenuType of the newly selected
	|  library group row, so create a new entry model from the menu entries
	|  and insert that model into the entry view.  Then delete old entry model.
	*/
	model = entry_model;
	entry_model = entry_model_create(menu);
	gtk_tree_view_set_model(entry_treeview, entry_model);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(entry_model),
					ENTRY_NAME_COLUMN, GTK_SORT_ASCENDING);

	if (model)
		gtk_list_store_clear(GTK_LIST_STORE(model));
	}



  /* ---------- Manage the GUI treeview of the data models -----------
  */
static gint
library_window_configure_event_cb(GtkWidget *widget, GdkEventConfigure *ev,
                gpointer data)
	{
	Settings.library_window_height = widget->allocation.height;
	Settings.config_modified = TRUE;
	return FALSE;
    }

static void
library_close_cb(gpointer data)
	{
	gtk_widget_destroy(library_window);
	library_window = NULL;
	}

static void
library_destroy_cb(GtkWidget *widget, OutputType *out)
    {
    library_window = NULL;
    }

void
gui_library_window_show(OutputType *out)
	{
	GtkWidget		*vbox, *hbox, *button;
	GtkTreeView		*treeview;
	GtkTreeModel	*model;
	GtkTreeSelection *selection;
	GtkCellRenderer	*renderer;

	if (library_window)
		{
		/* gtk_window_present() grabs focus which we don't want
		*/
		gdk_window_raise(library_window->window);
		return;
		}
	library_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(library_window), "destroy",
				G_CALLBACK(library_destroy_cb), out);
	gtk_window_set_title(GTK_WINDOW(library_window),
				_("PCB Library"));
	gtk_window_set_wmclass(GTK_WINDOW(library_window),
				"PCB_Library", "PCB");
	g_signal_connect(G_OBJECT(library_window), "configure_event",
				G_CALLBACK(library_window_configure_event_cb), NULL);
	gtk_window_set_default_size(GTK_WINDOW(library_window),
				-1, Settings.library_window_height);

	gtk_container_set_border_width(GTK_CONTAINER(library_window), 2);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	gtk_container_add(GTK_CONTAINER(library_window), vbox);
	hbox = gtk_hbox_new(FALSE, 8);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE,TRUE, 4);


	model = menu_model_create();
	treeview = GTK_TREE_VIEW(gtk_tree_view_new_with_model(model));
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model),
					MENU_NAME_COLUMN, GTK_SORT_ASCENDING);
	g_object_unref(G_OBJECT(model));
	gtk_tree_view_set_rules_hint(treeview, FALSE);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(treeview, -1,
				_("Library Group"), renderer,
				"text", MENU_NAME_COLUMN, NULL);

	/* TODO: record expanded states when window is destroyed and restore
	|  state here?
	*/

	selection = gui_scrolled_selection(treeview, hbox,
				GTK_SELECTION_SINGLE,
				GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC,
				menu_tree_selection_changed_cb, NULL);


	/* Create the elements treeview and wait for a callback to populate it.
	*/
	treeview = GTK_TREE_VIEW(gtk_tree_view_new());
	entry_treeview = treeview;

	gtk_tree_view_set_rules_hint(treeview, FALSE);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(treeview, -1, _("Elements"),
				renderer,
				"text", ENTRY_NAME_COLUMN, NULL);

	selection = gui_scrolled_selection(treeview, hbox,
				GTK_SELECTION_SINGLE,
				GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC,
				entry_tree_selection_changed_cb, NULL);
	entry_selection = selection;

	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(button), "clicked",
				G_CALLBACK(library_close_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);

	if( Settings.AutoPlace )
		gtk_widget_set_uposition( GTK_WIDGET(library_window), 10, 10);

	gtk_widget_show_all(library_window);

	/* If focus were grabbed, output drawing area would loose it which we
	|  don't want.
	*/
	gtk_widget_realize(library_window);
	gdk_window_set_accept_focus(library_window->window, FALSE);

	gtk_widget_show_all(library_window);
	}
