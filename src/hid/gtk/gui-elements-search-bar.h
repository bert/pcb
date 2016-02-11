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
#ifndef PCB_HID_GTK_GUI_ELEMENTS_SEARCH_BAR_H
#define PCB_HID_GTK_GUI_ELEMENTS_SEARCH_BAR_H
#include <glib-object.h>
#include <gtk/gtk.h>
G_BEGIN_DECLS

/* GhidElementsSearchBar */
#define GHID_TYPE_ELEMENTS_SEARCH_BAR (ghid_elements_search_bar_get_type())
#define GHID_ELEMENTS_SEARCH_BAR(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GHID_TYPE_ELEMENTS_SEARCH_BAR, GhidElementsSearchBar))
#define GHID_ELEMENTS_SEARCH_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GHID_TYPE_ELEMENTS_SEARCH_BAR, GhidElementsSearchBarClass))
#define GHID_IS_ELEMENTS_SEARCH_BAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GHID_TYPE_ELEMENTS_SEARCH_BAR))
#define GHID_GET_ELEMENTS_SEARCH_BAR_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), GHID_TYPE_ELEMENTS_SEARCH_BAR, GhidElementsSearchBarClass))

GType ghid_elements_search_bar_get_type (void);

typedef struct {
	GtkEntryClass parent_class;

} GhidElementsSearchBarClass;

typedef struct {
	GtkEntry parent_instance;

	/*** The popup search menu ***/
	GtkWidget *menu;

	/* Entries in that menu: */

	/* The enable/disable regex option */
	GtkWidget *enable_regex;

	/* The case sensitive option */
	GtkWidget *case_sense;

} GhidElementsSearchBar;

GhidElementsSearchBar* ghid_elements_search_bar_new(void);

G_END_DECLS
#endif /* PCB_HID_GTK_GUI_ELEMENTS_SEARCH_BAR_H */
