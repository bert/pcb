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
#ifndef PCB_HID_GTK_GUI_ELEMENTS_WINDOW_H
#define PCB_HID_GTK_GUI_ELEMENTS_WINDOW_H
#include <glib-object.h>
#include <gtk/gtk.h>
#include "gui-elements-search-bar.h"
G_BEGIN_DECLS

/* GhidElementsWindow */
#define GHID_TYPE_ELEMENTS_WINDOW (ghid_elements_window_get_type())
#define GHID_ELEMENTS_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GHID_TYPE_ELEMENTS_WINDOW, GhidElementsWindow))
#define GHID_ELEMENTS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GHID_TYPE_ELEMENTS_WINDOW, GhidElementsWindowClass))
#define GHID_IS_ELEMENTS_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GHID_TYPE_ELEMENTS_WINDOW))
#define GHID_GET_ELEMENTS_WINDOW_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), GHID_TYPE_ELEMENTS_WINDOW, GhidElementsWindowClass))

GType ghid_elements_window_get_type (void);

typedef struct {
	GtkWindowClass parent_class;

} GhidElementsWindowClass;

typedef struct {
	GtkWindow parent_instance;

	/* The treeview widget */
	GtkWidget* treeview;

	/* The store within which the element list is held */
	GtkListStore *store;
	/* The filtered store, and its sorted version */
	GtkTreeModel *filtered_store, *sorted_store;

	/* The scrolled window within which the treeview resides */
	GtkWidget *scrolled;

	/* The search box */
	GhidElementsSearchBar *search;

	/* The compiled regex we're searching for now */
	GRegex *search_regex;
} GhidElementsWindow;

void ghid_elements_window_show( void );

G_END_DECLS
#endif /* PCB_HID_GTK_GUI_ELEMENTS_WINDOW_H */
