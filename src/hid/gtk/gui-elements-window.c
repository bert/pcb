/* gEDA - GPL Electronic Design Automation
 * Copyright (C) 2016 Rob Spanton
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Rob Spanton: rob@robspanton.com
 */
#include "data.h"
#include "global.h"
#include <gtk/gtk.h>
#include "gui.h"
#include "gui-elements-window.h"
#include "select.h"

G_DEFINE_TYPE(GhidElementsWindow, ghid_elements_window, GTK_TYPE_WINDOW);

static GhidElementsWindow* elemwin = NULL;

/* The columns of our list store, which stores a list of elements */
enum {
	/* The name of the element */
	ELEMLIST_NAME_COLUMN,
	/* Pointer to the element itself (ElementType*) */
	ELEMLIST_ELEMENT_P_COLUMN,
	/* The total number of columns */
	ELEMLIST_N_COLUMNS,
};

/** Callback function for selecting an element.
 * @param element The element to select */
static void op_element_select(ElementType *element)
{
	SelectElement(element, true);
}

/** Callback function for deselecting an element.
 * @param element The element to deselect */
static void op_element_deselect(ElementType *element)
{
	SelectElement(element, false);
}

/** Callback function for applying the given operation to the given
 * item from the treeview.
 * @param model The model the item is stored in.
 * @param path The path of the item within the store.
 * @param iter An iter pointing to the item in question.
 * @param _op Pointer to operation to apply to element. */
static void proc_selected( GtkTreeModel *model,
			   GtkTreePath *path,
			   GtkTreeIter *iter,
			   gpointer _op )
{
	ElementType *element;
	void (*op) (ElementType *element) = _op;

	gtk_tree_model_get(model, iter,
			   ELEMLIST_ELEMENT_P_COLUMN, &element, -1);

	op(element);
}

/** Callback function for select button click event */
static void sel_button_clicked( GtkButton *button_select, gpointer _self )
{
	GhidElementsWindow *self = GHID_ELEMENTS_WINDOW(_self);
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->treeview));

	gtk_tree_selection_selected_foreach(sel, proc_selected, op_element_select);
}

/** Callback function for deselect button click event */
static void desel_button_clicked( GtkButton *button_select, gpointer _self )
{
	GhidElementsWindow *self = GHID_ELEMENTS_WINDOW(_self);
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->treeview));

	gtk_tree_selection_selected_foreach(sel, proc_selected, op_element_deselect);
}

/** Filter function for the filtered tree model.  Returns true if the
 * item complies with the current search parameters.
 * @param model The model to filter from.
 * @param iter The iter of the item to examine.
 * @param _self Pointer to the GhidElementsWindow instance.
 * @return Whether to include this item in the filtered tree view. */
static gboolean elemlist_filter( GtkTreeModel *model,
				 GtkTreeIter *iter,
				 gpointer _self )
{
	char *name;
	GhidElementsWindow *self = GHID_ELEMENTS_WINDOW(_self);
	gboolean regex_mode, case_sensitive;

	gtk_tree_model_get(model, iter,
			   ELEMLIST_NAME_COLUMN, &name, -1);

	if( name == NULL ) {
		/* This function can be called whilst an entry in the
		 * model is not complete.  Avoid displaying
		 * incomplete entries. */
		return FALSE;
	}

	g_object_get(self->search,
		     "regex", &regex_mode,
		     "case-sensitive", &case_sensitive,
		     NULL);

	if( regex_mode ) {
		if( self->search_regex == NULL ) {
			/* Currently no search pattern, so everything matches */
			return TRUE;
		}

		return g_regex_match(self->search_regex, name, 0, NULL);
	} else {
		bool match;
		const char* needle = gtk_entry_get_text(GTK_ENTRY(self->search));
		gchar *f_needle, *f_haystack;

		if( case_sensitive ) {
			f_needle = (char*)needle;
			f_haystack = name;
		} else {
			/* For unicode compatibility, fold the case before comparison */
			f_haystack = g_utf8_casefold(name, -1);
			f_needle = g_utf8_casefold(needle, -1);
		}

		match = g_strstr_len(f_haystack, -1, f_needle);

		if( !case_sensitive ) {
			/* Case folding allocated strings, so free them */
			g_free(f_haystack);
			g_free(f_needle);
		}

		return match;
	}
}

/** Compare function for sorting items in the treeview by name.
 * This performs a "natural" sort.
 * @param model The store that the items are in.
 * @param a Item A to compare.
 * @param b Item B to compare.
 * @param ud Unused
 * @return Same behaviour as strcmp's return value.
  */
static gint elemlist_sort_name_cmp( GtkTreeModel *model,
				    GtkTreeIter *a,
				    GtkTreeIter *b,
				    gpointer ud )
{
	const char *a_name, *b_name;

	gtk_tree_model_get(model, a,
			   ELEMLIST_NAME_COLUMN, &a_name, -1);
	gtk_tree_model_get(model, b,
			   ELEMLIST_NAME_COLUMN, &b_name, -1);
	
	/* strverscmp does more of a "natural" string sort than
	 * strcmp.  This means that names with numbers in will sort
	 * R1, R2, R10, rather than R1, R10, R2.
	 * There are likely locales in which this function does not do
	 * the right thing -- however, none of our current
	 * dependencies provide a better option than strverscmp. */
	return strverscmp(a_name, b_name);
}

/** Find an element in the element store.
 * @param store The store to search.
 * @param needle The element to find.
 * @param _iter Pointer to the iter to populate with the located
 *              item.  Can be NULL if not required.
 * @return Whether the element was found. */
static bool elemlist_find_element( GtkTreeModel *store, 
				   const ElementType* needle,
				   GtkTreeIter *_iter )
{
	gboolean valid;
	GtkTreeIter local_iter;
	GtkTreeIter *iter = &local_iter;

	if( _iter != NULL )
		iter = _iter;

	valid = gtk_tree_model_get_iter_first(store, iter);
	while(valid) {
		ElementType* element;

		gtk_tree_model_get(store, iter,
				   ELEMLIST_ELEMENT_P_COLUMN, &element, -1);

		if( element == needle ) {
			/* Found the element */
			return true;
		}

		valid = gtk_tree_model_iter_next(store, iter);
	}

	/* Not found */
	return false;
}

static const char* unnamed_element = "[None]";

/** Synchronise the list store with the main list of elements.
 * @param self The GhidElementsWindow instance. */
static void update_list(GhidElementsWindow *self)
{
	gboolean valid;
	GtkTreeIter iter;

	/* Find new elements that have arrived */
	ELEMENT_LOOP(PCB->Data);
	{
		const char* name = ELEMENT_NAME(PCB, element);

		if( elemlist_find_element(GTK_TREE_MODEL(self->store), element, &iter) ) {
			/* Element is already in list */
			/* Check that its name hasn't changed */
			char *old_name;

			gtk_tree_model_get(GTK_TREE_MODEL(self->store), &iter,
					   ELEMLIST_NAME_COLUMN, &old_name, -1);


			if( g_strcmp0(name, old_name) != 0 ) {
				/* Name has changed */
				gtk_list_store_set(self->store, &iter,
						   ELEMLIST_NAME_COLUMN, name, -1);
			}

			g_free(old_name);
			continue;
		}

		if( name == NULL ) {
			name = unnamed_element;
		}

		gtk_list_store_append(self->store, &iter);
		gtk_list_store_set(self->store, &iter,
				   ELEMLIST_NAME_COLUMN, name,
				   ELEMLIST_ELEMENT_P_COLUMN, element,
				   -1);
	}
	END_LOOP;

	/* Find elements that have disappeared */
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(self->store), &iter);
	while(valid) {
		ElementType* store_element;
		bool found = false;

		gtk_tree_model_get(GTK_TREE_MODEL(self->store), &iter,
				   ELEMLIST_ELEMENT_P_COLUMN, &store_element, -1);


		/* Does this element feature in the element list? */
		ELEMENT_LOOP(PCB->Data);
		{
			if(element == store_element) {
				found = true;
				break;
			}
		}
		END_LOOP;

		if( !found ) {
			/* This element no longer exists in the treeview, so 
			   remove it from the treeview */
			valid = gtk_list_store_remove(GTK_LIST_STORE(self->store), &iter);
		} else {
			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(self->store), &iter);
		}
	}
}

/** Create the treeview widget.
 * @param self The GhidElementsWindow instance. */
static void create_treeview( GhidElementsWindow *self )
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *viewcol;
	GtkTreeSelection *sel;

	/*** Element list creation ***/

	/** List store creation **/
	/* This contains the data shown in the list, and is the model in the MVC */
	self->store = gtk_list_store_new( ELEMLIST_N_COLUMNS,
					  /* Name */
					  G_TYPE_STRING,
					  /* Element pointer */
					  G_TYPE_POINTER );

	/* Create a filtered version of the model on-top of the base model */
	self->filtered_store = gtk_tree_model_filter_new(GTK_TREE_MODEL(self->store), NULL);

	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(self->filtered_store),
					       elemlist_filter, self, NULL);

	/* Create a sorted version of the filtered model */
	self->sorted_store = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(self->filtered_store));

	/* Default to sorting by the name column */
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(self->sorted_store),
					     ELEMLIST_NAME_COLUMN, GTK_SORT_ASCENDING);

	/* Set the sort function for the name column */
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(self->sorted_store),
					ELEMLIST_NAME_COLUMN,
					elemlist_sort_name_cmp, NULL, NULL);

	/* Populate the list */
	update_list(self);

	/** Create the view **/
	self->scrolled = gtk_scrolled_window_new(NULL, NULL);
	self->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(self->sorted_store));

	gtk_container_add(GTK_CONTAINER(self->scrolled), GTK_WIDGET(self->treeview));
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(self->scrolled),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	/* Inform the widget how to render */
	renderer = gtk_cell_renderer_text_new();
	viewcol = gtk_tree_view_column_new_with_attributes(_("Element Name"), renderer,
							   "text", ELEMLIST_NAME_COLUMN,
							   NULL);
	/* Set this column to sort by its own contents, and enable
	 * clickable column heading to adjust sorting. */
	gtk_tree_view_column_set_sort_column_id(viewcol, ELEMLIST_NAME_COLUMN);
	gtk_tree_view_append_column(GTK_TREE_VIEW(self->treeview), viewcol);

	/* Allow multiple items to be selected */
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->treeview));
	gtk_tree_selection_set_mode(sel, GTK_SELECTION_MULTIPLE);
}

/** Callback for when the search parameters change.
 * @param _self The GhidElementsWindow instance. */
static void update_search( gpointer *_self )
{
	GhidElementsWindow *self = GHID_ELEMENTS_WINDOW(_self);
	gboolean regex_mode, case_sensitive;

	g_object_get(self->search,
		     "regex", &regex_mode,
		     "case-sensitive", &case_sensitive,
		     NULL);

	if( regex_mode ) {
		GRegexCompileFlags cflags = 0;

		if( self->search_regex != NULL )
			/* Get rid of old regex */
			g_regex_unref(self->search_regex);
		
		if( !case_sensitive ) {
			cflags |= G_REGEX_CASELESS;
		}

		self->search_regex = g_regex_new( gtk_entry_get_text(GTK_ENTRY(self->search)),
						  cflags, 0, NULL );
	}

	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(self->filtered_store));
}

/** Show the elements window, creating it if necessary. */
void ghid_elements_window_show( void )
{
	if( elemwin == NULL ) {
		elemwin = g_object_new(GHID_TYPE_ELEMENTS_WINDOW, NULL);
	}
	
	gtk_widget_show_all(GTK_WIDGET(elemwin));
	gtk_window_present(GTK_WINDOW(elemwin));
}

/** GhidElementsWindowClass initialiser  */
static void ghid_elements_window_class_init(GhidElementsWindowClass *klass)
{
}

/** Window's "configure-event" signal handler */
static gboolean window_configure_cb( GtkWidget *widget,
				     GdkEvent *event,
				     gpointer user_data )
{
	GtkAllocation alloc;

	gtk_widget_get_allocation(widget, &alloc);
	ghidgui->elements_window_width = alloc.width;
	ghidgui->elements_window_height = alloc.height;
	ghidgui->config_modified = TRUE;
	return FALSE;
}

/** GhidElementsWindow initialiser  */
static void ghid_elements_window_init(GhidElementsWindow *self)
{
	GtkWidget *button_select, *button_desel, *hbox, *vbox;

	self->search_regex = NULL;

	g_signal_connect(self, "delete-event",
			 G_CALLBACK(gtk_widget_hide_on_delete), NULL);

	g_signal_connect(self, "configure-event",
			 G_CALLBACK(window_configure_cb), NULL);

	gtk_window_set_title(GTK_WINDOW(self), _("PCB Elements"));

	/* The defaults for the setting are zero, which is obviously
	 * not an appropriate window size. */
	if( ghidgui->elements_window_height == 0 ||
	    ghidgui->elements_window_width == 0 ) {
		ghidgui->elements_window_height = 400;
		ghidgui->elements_window_width = 400;
	}

	gtk_window_resize(GTK_WINDOW(self),
			  ghidgui->elements_window_width,
			  ghidgui->elements_window_height);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(self), vbox);

	self->search = ghid_elements_search_bar_new();

	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(self->search), false, true, 0);

	g_signal_connect_swapped(self->search, "changed",
				 G_CALLBACK(update_search), self);
	g_signal_connect_swapped(self->search, "notify::regex",
				 G_CALLBACK(update_search), self);
	g_signal_connect_swapped(self->search, "notify::case-sensitive",
				 G_CALLBACK(update_search), self);

	create_treeview(self);

	gtk_box_pack_start(GTK_BOX(vbox), self->scrolled, true, true, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, true, 0);

	button_select = gtk_button_new_with_label(_("Select"));
	button_desel = gtk_button_new_with_label(_("Deselect"));

	gtk_container_add(GTK_CONTAINER(hbox), button_select);
	g_signal_connect(button_select, "clicked",
			 G_CALLBACK(sel_button_clicked), self);

	gtk_container_add(GTK_CONTAINER(hbox), button_desel);
	g_signal_connect(button_desel, "clicked",
			 G_CALLBACK(desel_button_clicked), self);
}

/** HID action function for when elements are modified */
static int elements_changed_cb(int argc, char** argv, Coord x, Coord y)
{
	/* Ignore if the store hasn't yet been constructed
	   (e.g. if the window hasn't been shown). */
	if( elemwin == NULL )
		return 0;

	update_list(elemwin);
	return 0;
}

static HID_Action ghid_elements_window_actions[] = {
	{
		.name = "ElementsChanged",
		.need_coord_msg = NULL,
		.trigger_cb = elements_changed_cb,
		.description = "Tells the GUI that the element list has changed",
		.syntax = "ElementsChanged()",
	},
};

REGISTER_ACTIONS(ghid_elements_window_actions);
