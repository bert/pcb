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
static void top_toolbar_add_button(
	GtkWidget* toolbar,
	char ** pixels,
	const gchar* label,
	const gchar* tooltip,
	GtkSignalFunc callback
)
{
  GtkToolItem* button;
  GdkPixbuf *pixbuf;
  
  pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**) pixels);
  button = gtk_tool_button_new(gtk_image_new_from_pixbuf(pixbuf), label);
  gtk_tool_item_set_tooltip_text(button, tooltip);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), button, -1);
  g_signal_connect(G_OBJECT(button), "clicked", callback, NULL);
}

/*!
 * \brief Create the top toolbar contents.
 *
 */
void
ghid_make_top_toolbar(GtkWidget* parent)
{
  GtkWidget *toolbar;
  
  toolbar = gtk_toolbar_new();
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
  
  top_toolbar_add_button(toolbar, grid_05mil_xpm, "grid_05_mil", "grid 5 mil", G_CALLBACK(top_tool_button_grid_05mil_cb));
  top_toolbar_add_button(toolbar, grid_25mil_xpm, "grid_25_mil", "grid 25 mil", G_CALLBACK(top_tool_button_grid_25mil_cb));
  top_toolbar_add_button(toolbar, grid_50mil_xpm, "grid_50_mil", "grid 50 mil", G_CALLBACK(top_tool_button_grid_50mil_cb));

  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

  top_toolbar_add_button(toolbar, poly_thin_xpm, "poly_thin", "toggle draw thin polygons", G_CALLBACK(top_tool_button_thin_poly_cb));
  top_toolbar_add_button(toolbar, poly_hide_xpm, "poly_hide", "toggle hide polygons", G_CALLBACK(top_tool_button_hide_poly_cb));

  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

  top_toolbar_add_button(toolbar, pin_hi_xpm, "pin_hi", "pin size +10 mil", G_CALLBACK(top_tool_button_pin_hi_cb));
  top_toolbar_add_button(toolbar, pin_lo_xpm, "pin_lo", "pin size -10 mil", G_CALLBACK(top_tool_button_pin_lo_cb));

  top_toolbar_add_button(toolbar, text_hi_xpm, "text_hi", "text size +5 mil", G_CALLBACK(top_tool_button_text_hi_cb));
  top_toolbar_add_button(toolbar, text_lo_xpm, "text_lo", "text size -5 mil", G_CALLBACK(top_tool_button_text_lo_cb));

  
  gtk_box_pack_start(GTK_BOX(parent), toolbar, FALSE, FALSE, 0);   
}
