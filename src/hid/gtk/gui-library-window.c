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

/* This file originally from the PCB Gtk port by Bill Wilson. It has
 * since been combined with modified code from the gEDA project:
 *
 * gschem/src/ghid_library_window.c, checked out by Peter Clifton
 * from gEDA/gaf commit 72581a91da08c9d69593c24756144fc18940992e
 * on 3rd Jan, 2008.
 *
 * gEDA - GPL Electronic Design Automation
 * gschem - gEDA Schematic Capture
 * Copyright (C) 1998-2007 Ales Hvezda
 * Copyright (C) 1998-2007 gEDA Contributors (see ChangeLog for details)
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gui.h"
#include "global.h"
#include "buffer.h"
#include "data.h"
#include "set.h"

#include <gdk/gdkkeysyms.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

static GtkWidget *library_window;

#include "gui-pinout-preview.h"
#include "gui-library-window.h"

/*! \def LIBRARY_FILTER_INTERVAL
 *  \brief The time interval between request and actual filtering
 *
 *  This constant is the time-lag between user modifications in the
 *  filter entry and the actual evaluation of the filter which
 *  ultimately update the display. It helps reduce the frequency of
 *  evaluation of the filter as user types.
 *
 *  Unit is milliseconds.
 */
#define LIBRARY_FILTER_INTERVAL 200


static gint
library_window_configure_event_cb (GtkWidget * widget, GdkEventConfigure * ev,
				   gpointer data)
{
  GtkAllocation allocation;

  gtk_widget_get_allocation (widget, &allocation);
  ghidgui->library_window_width = allocation.width;
  ghidgui->library_window_height = allocation.height;
  ghidgui->config_modified = TRUE;
  return FALSE;
}


enum
{
  MENU_TOPPATH_COLUMN,		/* Top path of the library         */
  MENU_SUBPATH_COLUMN,		/* Relative path to the top        */
  MENU_NAME_COLUMN,		/* Text to display in the tree     */
  MENU_LIBRARY_COLUMN,		/* Pointer to the LibraryMenuType  */
  MENU_ENTRY_COLUMN,		/* Pointer to the LibraryEntryType */
  N_MENU_COLUMNS
};


/*! \brief Process the response returned by the library dialog.
 *  \par Function Description
 *  This function handles the response <B>arg1</B> of the library
 *  dialog <B>dialog</B>.
 *
 *  Parameter <B>user_data</B> is a pointer on the relevant toplevel
 *  structure.
 *
 *  \param [in] dialog    The library dialog.
 *  \param [in] arg1      The response ID.
 *  \param [in] user_data
 */
static void
library_window_callback_response (GtkDialog * dialog,
				  gint arg1, gpointer user_data)
{
  switch (arg1)
    {
    case GTK_RESPONSE_CLOSE:
    case GTK_RESPONSE_DELETE_EVENT:
      gtk_widget_destroy (GTK_WIDGET (library_window));
      library_window = NULL;
      break;

    default:
      /* Do nothing, in case there's another handler function which
         can handle the response ID received. */
      break;
    }
}


/*! \brief Creates a library dialog.
 *  \par Function Description
 *  This function create the library dialog if it is not already created.
 *  It does not show the dialog, use ghid_library_window_show for that.
 *
 */
void
ghid_library_window_create (GHidPort * out)
{
  GtkWidget *current_tab, *entry_filter;
  GtkNotebook *notebook;

  if (library_window)
    return;

  library_window = (GtkWidget *)g_object_new (GHID_TYPE_LIBRARY_WINDOW, NULL);

  g_signal_connect (library_window,
                    "response",
                    G_CALLBACK (library_window_callback_response), NULL);
  g_signal_connect (G_OBJECT (library_window), "configure_event",
                    G_CALLBACK (library_window_configure_event_cb), NULL);
  gtk_window_resize (GTK_WINDOW (library_window),
                     ghidgui->library_window_width,
                     ghidgui->library_window_height);

  gtk_window_set_title (GTK_WINDOW (library_window), _("PCB Library"));
  gtk_window_set_wmclass (GTK_WINDOW (library_window), "PCB_Library",
                          "PCB");

  gtk_widget_realize (library_window);
  if (Settings.AutoPlace)
    gtk_window_move (GTK_WINDOW (library_window), 10, 10);

  gtk_editable_select_region (GTK_EDITABLE
			      (GHID_LIBRARY_WINDOW (library_window)->
			       entry_filter), 0, -1);

  /* Set the focus to the filter entry only if it is in the current
     displayed tab */
  notebook = GTK_NOTEBOOK (GHID_LIBRARY_WINDOW (library_window)->viewtabs);
  current_tab = gtk_notebook_get_nth_page (notebook,
					   gtk_notebook_get_current_page
					   (notebook));
  entry_filter =
    GTK_WIDGET (GHID_LIBRARY_WINDOW (library_window)->entry_filter);
  if (gtk_widget_is_ancestor (entry_filter, current_tab))
    {
      gtk_widget_grab_focus (entry_filter);
    }
}

/*! \brief Show the library dialog.
 *  \par Function Description
 *  This function show the library dialog, creating it if it is not
 *  already created, and presents it to the user (brings it to the
 *  front with focus).
 */
void
ghid_library_window_show (GHidPort * out, gboolean raise)
{
  ghid_library_window_create (out);
  gtk_widget_show_all (library_window);
  if (raise)
    gtk_window_present (GTK_WINDOW(library_window));
}

static GObjectClass *library_window_parent_class = NULL;


/*! \brief Determines visibility of items of the library treeview.
 *  \par Function Description
 *  This is the function used to filter entries of the footprint
 *  selection tree.
 *
 *  \param [in] model The current selection in the treeview.
 *  \param [in] iter  An iterator on a footprint or folder in the tree.
 *  \param [in] data  The library dialog.
 *  \returns TRUE if item should be visible, FALSE otherwise.
 */
static gboolean
lib_model_filter_visible_func (GtkTreeModel * model,
			       GtkTreeIter * iter, gpointer data)
{
  GhidLibraryWindow *library_window = (GhidLibraryWindow *) data;
  const gchar *compname;
  gchar *compname_upper, *text_upper, *pattern;
  const gchar *text;
  gboolean ret;

  g_assert (GHID_IS_LIBRARY_WINDOW (data));

  text = gtk_entry_get_text (library_window->entry_filter);
  if (g_ascii_strcasecmp (text, "") == 0)
    {
      return TRUE;
    }

  /* If this is a source, only display it if it has children that
   * match */
  if (gtk_tree_model_iter_has_child (model, iter))
    {
      GtkTreeIter iter2;

      gtk_tree_model_iter_children (model, &iter2, iter);
      ret = FALSE;
      do
	{
	  if (lib_model_filter_visible_func (model, &iter2, data))
	    {
	      ret = TRUE;
	      break;
	    }
	}
      while (gtk_tree_model_iter_next (model, &iter2));
    }
  else
    {
      gtk_tree_model_get (model, iter, MENU_NAME_COLUMN, &compname, -1);
      /* Do a case insensitive comparison, converting the strings
         to uppercase */
      compname_upper = g_ascii_strup (compname, -1);
      text_upper = g_ascii_strup (text, -1);
      pattern = g_strconcat ("*", text_upper, "*", NULL);
      ret = g_pattern_match_simple (pattern, compname_upper);
      g_free (compname_upper);
      g_free (text_upper);
      g_free (pattern);
    }

  return ret;
}


/*! \brief Handles activation (e.g. double-clicking) of a component row
 *  \par Function Description
 *  Component row activated handler:
 *  As a convenince to the user, expand / contract any node with children.
 *
 *  \param [in] tree_view The component treeview.
 *  \param [in] path      The GtkTreePath to the activated row.
 *  \param [in] column    The GtkTreeViewColumn in which the activation occurre
 *  \param [in] user_data The component selection dialog.
 */
static void
tree_row_activated (GtkTreeView       *tree_view,
                    GtkTreePath       *path,
                    GtkTreeViewColumn *column,
                    gpointer           user_data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  model = gtk_tree_view_get_model (tree_view);
  gtk_tree_model_get_iter (model, &iter, path);

  if (!gtk_tree_model_iter_has_child (model, &iter))
    return;

  if (gtk_tree_view_row_expanded (tree_view, path))
    gtk_tree_view_collapse_row (tree_view, path);
  else
    gtk_tree_view_expand_row (tree_view, path, FALSE);
}

/*! \brief Handles CTRL-C keypress in the TreeView
 *  \par Function Description
 *  Keypress activation handler:
 *  If CTRL-C is pressed, copy footprint name into the clipboard.
 *
 *  \param [in] tree_view The component treeview.
 *  \param [in] event     The GdkEventKey with keypress info.
 *  \param [in] user_data Not used.
 *  \return TRUE if CTRL-C event was handled, FALSE otherwise.
 */
static gboolean
tree_row_key_pressed (GtkTreeView *tree_view,
                      GdkEventKey *event,
                      gpointer     user_data)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkClipboard *clipboard;
  const gchar *compname;
  guint default_mod_mask = gtk_accelerator_get_default_mod_mask();

  /* Handle both lower- and uppercase `c' */
  if (((event->state & default_mod_mask) != GDK_CONTROL_MASK)
      || ((event->keyval != GDK_c) && (event->keyval != GDK_C)))
    return FALSE;

  selection = gtk_tree_view_get_selection (tree_view);
  g_return_val_if_fail (selection != NULL, TRUE);

  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return TRUE;

  gtk_tree_model_get (model, &iter, MENU_NAME_COLUMN, &compname, -1);

  clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  g_return_val_if_fail (clipboard != NULL, TRUE);

  gtk_clipboard_set_text (clipboard, compname, -1);

  return TRUE;
}

/*! \brief Handles changes in the treeview selection.
 *  \par Function Description
 *  This is the callback function that is called every time the user
 *  select a row the library treeview of the dialog.
 *
 *  If the selection is not a selection of a footprint, it does
 *  nothing. Otherwise it updates the preview and Element data.
 *
 *  \param [in] selection The current selection in the treeview.
 *  \param [in] user_data The library dialog.
 */
static void
library_window_callback_tree_selection_changed (GtkTreeSelection * selection,
						gpointer user_data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  GhidLibraryWindow *library_window = (GhidLibraryWindow *) user_data;
  LibraryEntryType *entry = NULL;
  gchar *m4_args;

  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return;

  gtk_tree_model_get (model, &iter, MENU_ENTRY_COLUMN, &entry, -1);

  if (entry == NULL)
    return;

  /* -1 flags this is an element file part and the file path is in
     |  entry->AllocateMemory.
   */
  if (entry->Template == (char *) -1)
    {
      if (LoadElementToBuffer (PASTEBUFFER, entry->AllocatedMemory, true))
        {
          SetMode (PASTEBUFFER_MODE);
          goto out;
        }
      return;
    }

  /* Otherwise, it's a m4 element and we need to create a string of
     |  macro arguments to be passed to the library command in
     |  LoadElementToBuffer()
   */
  m4_args = g_strdup_printf ("'%s' '%s' '%s'", EMPTY (entry->Template),
			     EMPTY (entry->Value), EMPTY (entry->Package));

  if (LoadElementToBuffer (PASTEBUFFER, m4_args, false))
    {
      SetMode (PASTEBUFFER_MODE);
      g_free (m4_args);
      goto out;
    }

  g_free (m4_args);
  return;

out:

  /* update the preview with new symbol data */
  g_object_set (library_window->preview,
		"element-data", PASTEBUFFER->Data->Element->data, NULL);
}

/*! \brief If there is only one toplevel node, expand it. */

static void
maybe_expand_toplevel_node (GtkTreeView *tree_view)
{
  GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
  if (gtk_tree_model_iter_n_children (model, NULL) == 1)
    {
      GtkTreePath *path = gtk_tree_path_new_first ();
      if (path != NULL)
        {
          gtk_tree_view_expand_row (tree_view, path, FALSE);
          gtk_tree_path_free(path);
        }
    }
}

/*! \brief Requests re-evaluation of the filter.
 *  \par Function Description
 *  This is the timeout function for the filtering of footprint
 *  in the tree of the dialog.
 *
 *  The timeout this callback is attached to is removed after the
 *  function.
 *
 *  \param [in] data The library dialog.
 *  \returns FALSE to remove the timeout.
 */
static gboolean
library_window_filter_timeout (gpointer data)
{
  GhidLibraryWindow *library_window = GHID_LIBRARY_WINDOW (data);
  GtkTreeModel *model;

  /* resets the source id in library_window */
  library_window->filter_timeout = 0;

  model = gtk_tree_view_get_model (library_window->libtreeview);

  if (model != NULL)
    {
      const gchar *text = gtk_entry_get_text (library_window->entry_filter);
      gtk_tree_model_filter_refilter ((GtkTreeModelFilter *) model);
      if (strcmp (text, "") != 0)
        {
          /* filter text not-empty */
          gtk_tree_view_expand_all (library_window->libtreeview);
        } else {
          /* filter text is empty, collapse expanded tree */
          gtk_tree_view_collapse_all (library_window->libtreeview);
          maybe_expand_toplevel_node (library_window->libtreeview);
        }
    }

  /* return FALSE to remove the source */
  return FALSE;
}

/*! \brief Callback function for the changed signal of the filter entry.
 *  \par Function Description
 *  This function monitors changes in the entry filter of the dialog.
 *
 *  It specifically manages the sensitivity of the clear button of the
 *  entry depending on its contents. It also requests an update of the
 *  footprint list by re-evaluating filter at every changes.
 *
 *  \param [in] editable  The filter text entry.
 *  \param [in] user_data The library dialog.
 */
static void
library_window_callback_filter_entry_changed (GtkEditable * editable,
					      gpointer user_data)
{
  GhidLibraryWindow *library_window = GHID_LIBRARY_WINDOW (user_data);
  GtkWidget *button;
  gboolean sensitive;

  /* turns button off if filter entry is empty */
  /* turns it on otherwise */
  button = GTK_WIDGET (library_window->button_clear);
  sensitive =
    (g_ascii_strcasecmp (gtk_entry_get_text (library_window->entry_filter),
			 "") != 0);
  gtk_widget_set_sensitive (button, sensitive);

  /* Cancel any pending update of the footprint list filter */
  if (library_window->filter_timeout != 0)
    g_source_remove (library_window->filter_timeout);

  /* Schedule an update of the footprint list filter in
   * LIBRARY_FILTER_INTERVAL milliseconds */
  library_window->filter_timeout = g_timeout_add (LIBRARY_FILTER_INTERVAL,
						  library_window_filter_timeout,
						  library_window);

}

/*! \brief Handles a click on the clear button.
 *  \par Function Description
 *  This is the callback function called every time the user press the
 *  clear button associated with the filter.
 *
 *  It resets the filter entry, indirectly causing re-evaluation
 *  of the filter on the list of symbols to update the display.
 *
 *  \param [in] editable  The filter text entry.
 *  \param [in] user_data The library dialog.
 */
static void
library_window_callback_filter_button_clicked (GtkButton * button,
					       gpointer user_data)
{
  GhidLibraryWindow *library_window = GHID_LIBRARY_WINDOW (user_data);

  /* clears text in text entry for filter */
  gtk_entry_set_text (library_window->entry_filter, "");

}

/* \brief Create the tree model for the "Library" view.
 * \par Function Description
 * Creates a tree where the branches are the available library
 * sources and the leaves are the footprints.
 */
static GtkTreeModel *
create_lib_tree_model (GhidLibraryWindow * library_window)
{
  GtkTreeStore *tree;
  char *rel_path, empty_string[] = ""; /* writable */
  GtkTreeIter *iter, p_iter, e_iter, c_iter;
  char *tok_start, *tok_end;
  gchar *name;
  gboolean exists;

  tree = gtk_tree_store_new (N_MENU_COLUMNS,
			     G_TYPE_STRING, G_TYPE_STRING,
			     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER);

  MENU_LOOP (&Library);
  {
    /* Watch for directory changes of library parts and create new
       |  parent iter at each change.
     */
    if (!menu->directory)	/* Shouldn't happen */
      menu->directory = g_strdup ("???");

    rel_path = menu->Name;

    if (strncmp(rel_path, menu->directory, strlen(menu->directory)) == 0)
      {
	if (rel_path[strlen(menu->directory)] == '\0')
	  rel_path = empty_string;
	else if (rel_path[strlen(menu->directory)] == PCB_DIR_SEPARATOR_C)
	  rel_path += strlen(menu->directory) + 1;
      }

    iter = NULL;
    tok_start = tok_end = rel_path;

    do
      {
	char saved_ch = *tok_end;
	*tok_end = '\0';

	exists = FALSE;
	if (gtk_tree_model_iter_children (GTK_TREE_MODEL (tree), &e_iter, iter))
	  do
	    {
	      gtk_tree_model_get (GTK_TREE_MODEL (tree), &e_iter,
				  MENU_TOPPATH_COLUMN, &name, -1);
	      if (strcmp (name, menu->directory) != 0)
		continue;

	      gtk_tree_model_get (GTK_TREE_MODEL (tree), &e_iter,
				  MENU_SUBPATH_COLUMN, &name, -1);
	      if (strcmp (name, rel_path) != 0)
		continue;

	      exists = TRUE;
	      break;
	    }
	  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (tree), &e_iter));

	if (exists)
	  p_iter = e_iter;
	else
	  {
	    gtk_tree_store_append (tree, &p_iter, iter);
	    gtk_tree_store_set (tree, &p_iter,
				MENU_TOPPATH_COLUMN, menu->directory,
				MENU_SUBPATH_COLUMN, rel_path,
				MENU_NAME_COLUMN,
				  tok_end == rel_path ?
				    g_path_get_basename(menu->directory) : tok_start,
				MENU_LIBRARY_COLUMN,
				  saved_ch == '\0' ? menu : NULL,
				MENU_ENTRY_COLUMN, NULL, -1);
	  }
	iter = &p_iter;

	*tok_end = saved_ch;

	tok_start = tok_end;
	if (*tok_start == PCB_DIR_SEPARATOR_C)
	  tok_start++;
	tok_end = strchr(tok_start, PCB_DIR_SEPARATOR_C);
    if (!tok_end) tok_end = tok_start + strlen(tok_start);
      }
    while (*tok_start != '\0');

    ENTRY_LOOP (menu);
    {
      gtk_tree_store_append (tree, &c_iter, iter);
      gtk_tree_store_set (tree, &c_iter,
			  MENU_TOPPATH_COLUMN, menu->directory,
			  MENU_SUBPATH_COLUMN, rel_path,
			  MENU_NAME_COLUMN, entry->ListEntry,
			  MENU_LIBRARY_COLUMN, menu,
			  MENU_ENTRY_COLUMN, entry, -1);
    }
    END_LOOP;

  }
  END_LOOP;

  return (GtkTreeModel *) tree;
}


#if 0
/* \brief On-demand refresh of the footprint library.
 * \par Function Description
 * Requests a rescan of the footprint library in order to pick up any
 * new signals, and then updates the library window.
 */
static void
library_window_callback_refresh_library (GtkButton * button,
					 gpointer user_data)
{
  GhidLibraryWindow *library_window = GHID_LIBRARY_WINDOW (user_data);
  GtkTreeModel *model;

  /* Rescan the libraries for symbols */
  /*  TODO: How do we do this in PCB?  */

  /* Refresh the "Library" view */
  model = (GtkTreeModel *)
    g_object_new (GTK_TYPE_TREE_MODEL_FILTER,
		  "child-model", create_lib_tree_model (library_window),
		  "virtual-root", NULL, NULL);

  gtk_tree_model_filter_set_visible_func ((GtkTreeModelFilter *) model,
					  lib_model_filter_visible_func,
					  library_window, NULL);

  gtk_tree_view_set_model (library_window->libtreeview, model);
  maybe_expand_toplevel_node (library_window->libtreeview);
}
#endif


/*! \brief Creates the treeview for the "Library" view */
static GtkWidget *
create_lib_treeview (GhidLibraryWindow * library_window)
{
  GtkWidget *libtreeview, *vbox, *scrolled_win, *label,
    *hbox, *entry, *button;
  GtkTreeModel *child_model, *model;
  GtkTreeSelection *selection;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* -- library selection view -- */

  /* vertical box for footprint selection and search entry */
  vbox = GTK_WIDGET (g_object_new (GTK_TYPE_VBOX,
				   /* GtkContainer */
				   "border-width", 5,
				   /* GtkBox */
				   "homogeneous", FALSE, "spacing", 5, NULL));

  child_model = create_lib_tree_model (library_window);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (child_model),
					MENU_NAME_COLUMN, GTK_SORT_ASCENDING);
  model = (GtkTreeModel *) g_object_new (GTK_TYPE_TREE_MODEL_FILTER,
					 "child-model", child_model,
					 "virtual-root", NULL, NULL);

  scrolled_win = GTK_WIDGET (g_object_new (GTK_TYPE_SCROLLED_WINDOW,
					   /* GtkScrolledWindow */
					   "hscrollbar-policy",
					   GTK_POLICY_AUTOMATIC,
					   "vscrollbar-policy",
					   GTK_POLICY_ALWAYS, "shadow-type",
					   GTK_SHADOW_ETCHED_IN, NULL));
  /* create the treeview */
  libtreeview = GTK_WIDGET (g_object_new (GTK_TYPE_TREE_VIEW,
					  /* GtkTreeView */
					  "model", model,
					  "rules-hint", TRUE,
					  "headers-visible", FALSE, NULL));

  g_signal_connect (libtreeview,
                    "row-activated",
                    G_CALLBACK (tree_row_activated),
                    NULL);

  g_signal_connect (libtreeview,
                    "key-press-event",
                    G_CALLBACK (tree_row_key_pressed),
                    NULL);

  maybe_expand_toplevel_node (GTK_TREE_VIEW (libtreeview));

  /* connect callback to selection */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (libtreeview));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect (selection,
		    "changed",
		    G_CALLBACK
		    (library_window_callback_tree_selection_changed),
		    library_window);

  /* insert a column to treeview for library/symbol name */
  renderer = GTK_CELL_RENDERER (g_object_new (GTK_TYPE_CELL_RENDERER_TEXT,
					      /* GtkCellRendererText */
					      "editable", FALSE, NULL));
  column = GTK_TREE_VIEW_COLUMN (g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
					       /* GtkTreeViewColumn */
					       "title", _("Components"),
					       NULL));
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_attributes (column, renderer,
				       "text", MENU_NAME_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (libtreeview), column);

  /* add the treeview to the scrolled window */
  gtk_container_add (GTK_CONTAINER (scrolled_win), libtreeview);

  /* add the scrolled window for directories to the vertical box */
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);


  /* -- filter area -- */
  hbox = GTK_WIDGET (g_object_new (GTK_TYPE_HBOX,
				   /* GtkBox */
				   "homogeneous", FALSE, "spacing", 3, NULL));

  /* create the entry label */
  label = GTK_WIDGET (g_object_new (GTK_TYPE_LABEL,
				    /* GtkMisc */
				    "xalign", 0.0,
				    /* GtkLabel */
				    "label", _("Filter:"), NULL));
  /* add the search label to the filter area */
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  /* create the text entry for filter in footprints */
  entry = GTK_WIDGET (g_object_new (GTK_TYPE_ENTRY,
				    /* GtkEntry */
				    "text", "", NULL));
  g_signal_connect (entry,
		    "changed",
		    G_CALLBACK (library_window_callback_filter_entry_changed),
		    library_window);

  /* now that that we have an entry, set the filter func of model */
  gtk_tree_model_filter_set_visible_func ((GtkTreeModelFilter *) model,
					  lib_model_filter_visible_func,
					  library_window, NULL);

  /* add the filter entry to the filter area */
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  /* set filter entry of library_window */
  library_window->entry_filter = GTK_ENTRY (entry);
  /* and init the event source for footprint filter */
  library_window->filter_timeout = 0;

  /* create the erase button for filter entry */
  button = GTK_WIDGET (g_object_new (GTK_TYPE_BUTTON,
				     /* GtkWidget */
				     "sensitive", FALSE,
				     /* GtkButton */
				     "relief", GTK_RELIEF_NONE, NULL));

  gtk_container_add (GTK_CONTAINER (button),
		     gtk_image_new_from_stock (GTK_STOCK_CLEAR,
					       GTK_ICON_SIZE_SMALL_TOOLBAR));
  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK
		    (library_window_callback_filter_button_clicked),
		    library_window);
  /* add the clear button to the filter area */
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  /* set clear button of library_window */
  library_window->button_clear = GTK_BUTTON (button);

#if 0
  /* create the refresh button */
  button = GTK_WIDGET (g_object_new (GTK_TYPE_BUTTON,
				     /* GtkWidget */
				     "sensitive", TRUE,
				     /* GtkButton */
				     "relief", GTK_RELIEF_NONE, NULL));
  gtk_container_add (GTK_CONTAINER (button),
		     gtk_image_new_from_stock (GTK_STOCK_REFRESH,
					       GTK_ICON_SIZE_SMALL_TOOLBAR));
  /* add the refresh button to the filter area */
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK (library_window_callback_refresh_library),
		    library_window);
#endif

  /* add the filter area to the vertical box */
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* save pointer to libtreeview in library_window */
  library_window->libtreeview = GTK_TREE_VIEW (libtreeview);

  return vbox;
}


static GObject *
library_window_constructor (GType type,
			    guint n_construct_properties,
			    GObjectConstructParam * construct_params)
{
  GObject *object;
  GhidLibraryWindow *library_window;
  GtkWidget *content_area;
  GtkWidget *hpaned, *notebook;
  GtkWidget *libview;
  GtkWidget *preview;
  GtkWidget *alignment, *frame;

  /* chain up to constructor of parent class */
  object = G_OBJECT_CLASS (library_window_parent_class)->
    constructor (type, n_construct_properties, construct_params);
  library_window = GHID_LIBRARY_WINDOW (object);

  /* dialog initialization */
  g_object_set (object,
		/* GtkWindow */
		"type", GTK_WINDOW_TOPLEVEL,
		"title", _("Select Footprint..."),
		"default-height", 300,
		"default-width", 400,
		"modal", FALSE, "window-position", GTK_WIN_POS_NONE,
		/* GtkDialog */
		"has-separator", TRUE, NULL);
  g_object_set (gtk_dialog_get_content_area (GTK_DIALOG (library_window)),
		"homogeneous", FALSE, NULL);

  /* horizontal pane containing selection and preview */
  hpaned = GTK_WIDGET (g_object_new (GTK_TYPE_HPANED,
				     /* GtkContainer */
				     "border-width", 5, NULL));
  library_window->hpaned = hpaned;

  /* notebook for library views */
  notebook = GTK_WIDGET (g_object_new (GTK_TYPE_NOTEBOOK,
				       "show-tabs", FALSE, NULL));
  library_window->viewtabs = GTK_NOTEBOOK (notebook);

  libview = create_lib_treeview (library_window);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), libview,
			    gtk_label_new (_("Libraries")));

  /* include the vertical box in horizontal box */
  gtk_paned_pack1 (GTK_PANED (hpaned), notebook, TRUE, FALSE);


  /* -- preview area -- */
  frame = GTK_WIDGET (g_object_new (GTK_TYPE_FRAME,
				    /* GtkFrame */
				    "label", _("Preview"), NULL));
  alignment = GTK_WIDGET (g_object_new (GTK_TYPE_ALIGNMENT,
					/* GtkAlignment */
					"left-padding", 5,
					"right-padding", 5,
					"top-padding", 5,
					"bottom-padding", 5,
					"xscale", 1.0,
					"yscale", 1.0,
					"xalign", 0.5, "yalign", 0.5, NULL));
  preview = (GtkWidget *)g_object_new (GHID_TYPE_PINOUT_PREVIEW,
			  /* GhidPinoutPreview */
			  "element-data", NULL,
			  /* GtkWidget */
			  "width-request", 150, "height-request", 150, NULL);
  gtk_container_add (GTK_CONTAINER (alignment), preview);
  gtk_container_add (GTK_CONTAINER (frame), alignment);
  /* set preview of library_window */
  library_window->preview = preview;

  gtk_paned_pack2 (GTK_PANED (hpaned), frame, FALSE, FALSE);

  /* add the hpaned to the dialog content area */
  content_area = gtk_dialog_get_content_area (GTK_DIALOG (library_window));
  gtk_box_pack_start (GTK_BOX (content_area), hpaned, TRUE, TRUE, 0);
  gtk_widget_show_all (hpaned);


  /* now add buttons in the action area */
  gtk_dialog_add_buttons (GTK_DIALOG (library_window),
			  /*  - close button */
			  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);

  return object;
}

static void
library_window_finalize (GObject * object)
{
  GhidLibraryWindow *library_window = GHID_LIBRARY_WINDOW (object);

  if (library_window->filter_timeout != 0)
    {
      g_source_remove (library_window->filter_timeout);
      library_window->filter_timeout = 0;
    }

  G_OBJECT_CLASS (library_window_parent_class)->finalize (object);
}


static void
library_window_class_init (GhidLibraryWindowClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructor = library_window_constructor;
  gobject_class->finalize = library_window_finalize;

  library_window_parent_class = (GObjectClass *)g_type_class_peek_parent (klass);
}


GType
ghid_library_window_get_type ()
{
  static GType library_window_type = 0;

  if (!library_window_type)
    {
      static const GTypeInfo library_window_info = {
	sizeof (GhidLibraryWindowClass),
	NULL,			/* base_init */
	NULL,			/* base_finalize */
	(GClassInitFunc) library_window_class_init,
	NULL,			/* class_finalize */
	NULL,			/* class_data */
	sizeof (GhidLibraryWindow),
	0,			/* n_preallocs */
	NULL			/* instance_init */
      };

      library_window_type = g_type_register_static (GTK_TYPE_DIALOG,
						    "GhidLibraryWindow",
						    &library_window_info, (GTypeFlags)0);
    }

  return library_window_type;
}
