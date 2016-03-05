/*!
 * \file src/hid/gtk/gui-elements-search-bar.c
 *
 * \brief Routines for the element window search bar.
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

#include "global.h"
#include "gui-elements-search-bar.h"

G_DEFINE_TYPE (GhidElementsSearchBar, ghid_elements_search_bar, GTK_TYPE_ENTRY);

enum
{
  PROP_REGEX = 1,
  PROP_CASE_SENSITIVE,
  N_PROPERTIES,
};

/*!
 * \brief The GhidElementsSearchBar GObject properties.
 */
static GParamSpec
*obj_properties[N_PROPERTIES] = {NULL,};

/*!
 * \brief GhidElementsSearchBar property getter.
 */
static void
get_property (GObject *object,
              guint property_id,
              GValue *value,
              GParamSpec *pspec)
{
  GhidElementsSearchBar *self = GHID_ELEMENTS_SEARCH_BAR (object);

  switch (property_id)
    {
      case PROP_REGEX:
        g_value_set_boolean (value,
                             gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (self->enable_regex)));
        break;

      case PROP_CASE_SENSITIVE:
        g_value_set_boolean (value,
                             gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (self->case_sense)));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

/*!
 * \brief GhidElementsSearchBarClass initialiser.
 */
static void
ghid_elements_search_bar_class_init (GhidElementsSearchBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = get_property;

  obj_properties[PROP_REGEX] = g_param_spec_boolean ("regex",
                                                     "Regex enabled",
                                                     "Whether regular expression search is enabled",
                                                     TRUE,
                                                     G_PARAM_READABLE);

  obj_properties[PROP_CASE_SENSITIVE] = g_param_spec_boolean ("case-sensitive",
                                                              "Case sensitive",
                                                              "Whether search is case sensitive",
                                                              FALSE,
                                                              G_PARAM_READABLE);

  g_object_class_install_properties (object_class,
                                     N_PROPERTIES,
                                     obj_properties);
}

/*!
 * \brief Callback for when the search icon is clicked.
 */
static void
search_icon_clicked (GtkEntry *entry,
                     GtkEntryIconPosition icon_pos,
                     GdkEvent *event,
                     gpointer _menu)
{
  GtkMenu *menu = GTK_MENU (_menu);

  if (event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton *button = (GdkEventButton*) event;
      gtk_menu_popup (menu, NULL, NULL, NULL, NULL, button->button, button->time);
    }
}

/*!
 * \brief Callback for when the regex option is toggled in the search
 * menu.
 */
static void
regex_toggled (GtkCheckMenuItem *item,
               gpointer _self)
{
  GhidElementsSearchBar *self = GHID_ELEMENTS_SEARCH_BAR (_self);

  g_object_notify_by_pspec (G_OBJECT(self), obj_properties[PROP_REGEX]);
}

/*!
 * \brief Callback for when case sensitivity is toggled in the search
 * menu.
 */
static void
case_toggled (GtkCheckMenuItem *item,
              gpointer _self)
{
  GhidElementsSearchBar *self = GHID_ELEMENTS_SEARCH_BAR (_self);

  g_object_notify_by_pspec ( G_OBJECT(self),
                             obj_properties[PROP_CASE_SENSITIVE]);
}

/*!
 * \brief Callback for when the search text is changed.
 */
static void
text_change_cb (GtkEditable *entry,
                gpointer _ud)
{
  const char* icon_name = NULL;

  /* Disable the clear button if there is no text. */
  if (gtk_entry_get_text_length(GTK_ENTRY(entry)) > 0)
    {
      icon_name = "edit-clear";
    }

  gtk_entry_set_icon_from_icon_name (GTK_ENTRY(entry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     icon_name);
}

/*!
 * \brief GhidElementsSearchBar initialiser.
 */
static void
ghid_elements_search_bar_init (GhidElementsSearchBar *self)
{
  self->menu = gtk_menu_new ();

  self->enable_regex = gtk_check_menu_item_new_with_label (_("Regular Expression"));
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (self->enable_regex), TRUE);
  gtk_menu_shell_append (GTK_MENU_SHELL (self->menu), self->enable_regex);

  self->case_sense = gtk_check_menu_item_new_with_label (_("Case Sensitive"));
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (self->case_sense), TRUE);
  gtk_menu_shell_append (GTK_MENU_SHELL (self->menu), self->case_sense);

  gtk_widget_show_all (self->menu);

  g_signal_connect (self->enable_regex, "toggled",
                    G_CALLBACK(regex_toggled), self);

  g_signal_connect (self->case_sense, "toggled",
                    G_CALLBACK(case_toggled), self);

  /* Setup the search icon. */
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY(self),
                                     GTK_ENTRY_ICON_PRIMARY,
                                     "edit-find");

  gtk_entry_set_icon_activatable (GTK_ENTRY(self),
                                  GTK_ENTRY_ICON_PRIMARY,
                                  true);

  /* Bring up the search menu when the icon is clicked. */
  g_signal_connect (self, "icon-press",
                    G_CALLBACK(search_icon_clicked), self->menu);

  /* Manage the clear icon's visibility. */
  g_signal_connect (self, "changed",
                    G_CALLBACK(text_change_cb), NULL);
}

/*!
 * \brief Create a new GhidElementsSearchBar.
 *
 * \return A new GhidElementsSearchBar.
 */
GhidElementsSearchBar*
ghid_elements_search_bar_new (void)
{
  return g_object_new (GHID_TYPE_ELEMENTS_SEARCH_BAR, NULL);
}
