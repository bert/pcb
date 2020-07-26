/*!
 * \file src/hid/gtk/gui-top-toolbar.c
 *
 * \brief This handles creation of the top toolbar and all its
 * buttons.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * This file was originally written by Bill Wilson for the PCB Gtk
 * port. It was later heavily modified by Dan McMahill to provide
 * user customized menus.
 */
 
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>

#include "gtkhid.h"
#include "gui.h"
#include "action.h"

#include "gui-icons-toolbar.data"

static void
top_tool_button_hide_poly_cb(GtkWidget* widget, gpointer data)
{
  hid_parse_actions ("Display(ToggleHidePoly)");
  ghid_sync_gui();
}

static void
top_tool_button_thin_poly_cb(GtkWidget* widget, gpointer data)
{
  hid_parse_actions ("Display(ToggleThindrawPoly)");
  ghid_sync_gui();
}

static void
top_tool_button_pin_hi_cb(GtkWidget* widget, gpointer data)
{
  hid_parse_actions ("Atomic(Save)");
  hid_parse_actions ("ChangeSize(SelectedPins,+10,mil)");
  hid_parse_actions ("Atomic(Restore)");
  hid_parse_actions ("ChangeDrillSize(SelectedPins,+7,mil)");
  hid_parse_actions ("Atomic(Block)");
}
static void
top_tool_button_pin_lo_cb(GtkWidget* widget, gpointer data)
{
  hid_parse_actions ("Atomic(Save)");
  hid_parse_actions ("ChangeDrillSize(SelectedPins,-7,mil)");
  hid_parse_actions ("Atomic(Restore)");
  hid_parse_actions ("ChangeSize(SelectedPins,-10,mil)");
  hid_parse_actions ("Atomic(Block)");
}

static void
top_tool_button_text_hi_cb(GtkWidget* widget, gpointer data)
{
  hid_parse_actions ("ChangeSize(SelectedTexts,+5,mil)");
}
static void
top_tool_button_text_lo_cb(GtkWidget* widget, gpointer data)
{
  hid_parse_actions ("ChangeSize(SelectedTexts,-5,mil)");
}

static void
top_tool_button_grid_05mil_cb (GtkWidget* widget, gpointer data)
{
  hid_parse_actions("SetUnits(mil)");
  hid_parse_actions("SetValue(Grid,5mil)");
  ghid_sync_gui();
}

static void
top_tool_button_grid_25mil_cb (GtkWidget* widget, gpointer data)
{
  hid_parse_actions("SetUnits(mil)");
  hid_parse_actions("SetValue(Grid,25mil)");
  ghid_sync_gui();
}

static void
top_tool_button_grid_50mil_cb (GtkWidget* widget, gpointer data)
{
  hid_parse_actions("SetUnits(mil)");
  hid_parse_actions("SetValue(Grid,50mil)");
  ghid_sync_gui();
}

/*!
 * \brief Add a single button to the toolbar.
 *
 */
static void
top_toolbar_add_button(
	GtkWidget* toolbar,
	char ** pixels,
	const gchar* label,
	const gchar* tooltip,
	GtkSignalFunc callback,
	gboolean add_to_list
)
{
  GtkToolItem* button;
  GdkPixbuf *pixbuf;
  
  pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**) pixels);
  button = gtk_tool_button_new(gtk_image_new_from_pixbuf(pixbuf), label);
  gtk_tool_item_set_tooltip_text(button, tooltip);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), button, -1);
  g_signal_connect(G_OBJECT(button), "clicked", callback, NULL);
  
  if (add_to_list) {
    ghidgui->mode_toolbar.extra_list = g_list_prepend(ghidgui->mode_toolbar.extra_list, button);
  }
  
}

/*!
 * \brief Add a single button to both toolbars.
 *
 */
static void
toolbar_add_buttons(
	GtkWidget* toolbar1,
	GtkWidget* toolbar2,
	char ** pixels,
	const gchar* label,
	const gchar* tooltip,
	GtkSignalFunc callback
)
{
	top_toolbar_add_button(toolbar1, pixels, label, tooltip, callback, false);
	top_toolbar_add_button(toolbar2, pixels, label, tooltip, callback, true);
}

/*!
 * \brief Add a separator to both toolbars.
 *
 */
static void
toolbar_add_separators(
	GtkWidget* toolbar1,
	GtkWidget* toolbar2
)
{
	GtkToolItem* item;
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar1), gtk_separator_tool_item_new(), -1);
	
	item = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar2), item, -1);
	ghidgui->mode_toolbar.extra_list = g_list_prepend(ghidgui->mode_toolbar.extra_list, item);
}

/*!
 * \brief Show and hides toolbar icons on either main toolbar or extra toolbar
 *  depending on the configuration state.
 */
void
ghid_set_extra_toolbar_state(void)
{

  if (ghidgui->use_extra_toolbar) {
     if (ghidgui->compact_vertical && ghidgui->compact_horizontal) {
        ghid_hide_toolbar_list (ghidgui->mode_toolbar.extra_list);
        gtk_widget_show_all (ghidgui->extra_toolbar);
     }
     else {
        ghid_show_toolbar_list (ghidgui->mode_toolbar.extra_list);
	    gtk_widget_hide (ghidgui->extra_toolbar);
     }
  }
  else {
     ghid_hide_toolbar_list(ghidgui->mode_toolbar.extra_list);
     gtk_widget_hide (ghidgui->extra_toolbar);
  }
}

/*!
 * \brief Create the top toolbar contents.
 *
 */
void
ghid_make_extra_toolbar(void)
{
  GtkWidget *toolbar;
  GtkWidget *toolbar2;
  
  /* use 2 toolbars : one for vertical arangement and the other for horizontal
  arangement. Only one will be visible at a time */
  toolbar = gtk_toolbar_new();
  toolbar2 = ghidgui->mode_toolbar.toolbar; /* append icons into the mode toolbar */

  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
  
  toolbar_add_buttons(toolbar, toolbar2, grid_05mil_xpm, "", "grid 5 mil", G_CALLBACK(top_tool_button_grid_05mil_cb));
  toolbar_add_buttons(toolbar, toolbar2, grid_25mil_xpm, "", "grid 25 mil", G_CALLBACK(top_tool_button_grid_25mil_cb));
  toolbar_add_buttons(toolbar, toolbar2, grid_50mil_xpm, "", "grid 50 mil", G_CALLBACK(top_tool_button_grid_50mil_cb));

  toolbar_add_separators(toolbar, toolbar2);

  toolbar_add_buttons(toolbar, toolbar2, poly_thin_xpm, "", "toggle draw thin polygons", G_CALLBACK(top_tool_button_thin_poly_cb));
  toolbar_add_buttons(toolbar, toolbar2, poly_hide_xpm, "", "toggle hide polygons", G_CALLBACK(top_tool_button_hide_poly_cb));

  toolbar_add_separators(toolbar, toolbar2);

  toolbar_add_buttons(toolbar, toolbar2, pin_hi_xpm, "", "pin size +10 mil", G_CALLBACK(top_tool_button_pin_hi_cb));
  toolbar_add_buttons(toolbar, toolbar2, pin_lo_xpm, "", "pin size -10 mil", G_CALLBACK(top_tool_button_pin_lo_cb));

  toolbar_add_buttons(toolbar, toolbar2, text_hi_xpm, "", "text size +5 mil", G_CALLBACK(top_tool_button_text_hi_cb));
  toolbar_add_buttons(toolbar, toolbar2, text_lo_xpm, "", "text size -5 mil", G_CALLBACK(top_tool_button_text_lo_cb));

  ghidgui->extra_toolbar = toolbar;

}
