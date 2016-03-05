/*!
 * \file src/hid/gtk/gui-elements-search-bar.h
 *
 * \brief Prototypes for the element window search bar.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design.
 *
 * Copyright (C) 2016 Rob Spanton <rob@robspanton.com>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact addresses for paper mail and Email:
 * Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 * haceaton@aplcomm.jhuapl.edu
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

typedef struct
{
  GtkEntryClass parent_class;
} GhidElementsSearchBarClass;

typedef struct
{
  GtkEntry parent_instance;

  GtkWidget *menu;
  /*!< The popup search menu. */

  /* Entries in that menu: */
  GtkWidget *enable_regex;
  /*!< The enable/disable regex option */

  GtkWidget *case_sense;
  /*!< The case sensitive option */
} GhidElementsSearchBar;

GhidElementsSearchBar* ghid_elements_search_bar_new (void);

G_END_DECLS

#endif /* PCB_HID_GTK_GUI_ELEMENTS_SEARCH_BAR_H */
