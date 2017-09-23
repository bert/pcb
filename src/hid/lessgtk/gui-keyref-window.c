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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* This file written by Bill Wilson for the PCB Gtk port */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gui.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

static GtkWidget *keyref_window;


static gchar *key_ref_text[] = {
  "<h>",
  N_("Keyboard\n"),
  N_("Keyboard shortcuts and actions available in PCB.\n"),
  N_("In below key actions, <s> is <shift>, <c> is <ctrl>\n"),
  N_("and <a> is <alt> or <mod>\n"),
  "\n",

  "<b>\ta\t",
  N_("Set layer and size from line or arc\n"),
  "\n",

  "<b><s><a>a ",
  N_("Unselect all objects\n"),
  "\n",

  "<b>\tb\t",
  N_("Flip element to opposite side of the board\n"),
  "<b><c>\tb\t",
  N_("Flip selected objects to opposite side of the board\n"),
  "\n",

  "<b><c>\tc\t",
  N_("Copy selected to buffer and unselect\n"),
  "\n",

  "<b>\td\t",
  N_("Display pin/pad names (numbers with View->Enable pinout shows number)\n"),
  "<b><s>\td\t",
  N_("Open pinout window for element under cursor\n"),
  "\n",

  "<b>\te\t",
  N_("Delete all rats\n"),
  "<b><s>\te\t",
  N_("Delete selected rats\n"),
  "\n",

  "<b>\tf\t",
  N_("Highlight connections to object\n"),
  "<b><s>\tf\t",
  N_("Reset highlighted connections\n"),
  "<b><c>\tf\t",
  N_("Cumulative highlight connections to object\n"),
  "\n",

  "<b>\tg\t",
  N_("Increment grid by configured grid increment\n"),

  "<b><s>\tg\t",
  N_("Decrement grid by configured grid increment\n"),
  "\n",

  "<b>\th\t",
  N_("Toggle visibility of element name under cursor\n"),

  "<b><s>\th\t",
  N_("Toggle visibility of selected element names\n"),

  "<b><c>\th\t",
  N_("Toggle the hole flag of object under cursor\n"),
  "\n",

  "<b>\tj\t",
  N_("Toggle line/arc should clear polygons flag of object under cursor\n"),

  "<b><s>\tj\t",
  N_("Toggle line/arc should clear polygons flag of selected\n"),
  "\n",

  "<b>\tk\t",
  N_("Increase clearance of object by configured clearance\n"),

  "<b><s>\tk\t",
  N_("Decrease clearance of object by configured clearance\n"),

  "<b><c>\tk\t",
  N_("Increase clearance of selected objects by configured clearance\n"),

  "<b><s><c>k ",
  N_("Decrease clearance of selected objects by configured clearance\n"),
  "\n",

  "<b>\tl\t",
  N_("Increment current route style line size by configured line increment\n"),

  "<b><s>\tl\t",
  N_("Decrement current route style line size by configured line increment\n"),
  "\n",

  "<b>\tm\t",
  N_("Move object to current layer\n"),

  "<b><s>\tm\t",
  N_("Move selected objects to current layer\n"),

  "<b><c>\tm\t",
  N_("Mark at cursor location for showing relative offsets\n"),
  "\n",

  "<b><s>\tn\t",
  N_("Select the shortest unselected rat on the board\n"),
  "\n",

  "<b>\to\t",
  N_("Optimize and draw all rats\n"),

  "<b><s>\to\t",
  N_("Optimize and draw selected rats\n"),
  "\n",

  "<b><c>\to\t",
  N_("Change octagon flag of object\n"),
  "\n",

  "<b>\tp\t",
  N_("Backup polygon drawing to previous point\n"),

  "<b><s>\tp\t",
  N_("Close polygon\n"),
  "\n",

  "<b>\tq\t",
  N_("Toggle the square flag of an object\n"),
  "\n",

  "<b><s>\tr\t",
  N_("Redo last undone operation\n"),
  "\n",

  "<b>\ts\t",
  N_("Increment size of an object by configured size increment\n"),
  "<b><s>\ts\t",
  N_("Decrement size of an object by configured size increment\n"),
  "<b><a>\ts\t",
  N_("Increment drill size of a pin or via\n"),
  "<b><s><a>s ",
  N_("Decrement drill size of a pin or via\n"),
  "\n",

  "<b>\tt\t",
  N_("Adjust text scale so new text increases by the configured size increment\n"),
  "<b><s>\tt\t",
  N_("Adjust text scale so new text decreases by the configured size increment\n"),
  "\n",

  "<b>\tu\t",
  N_("Undo last operation\n"),
  "\n",

  "<b>\tv\t",
  N_("Zoom to board extents\n"),
  "<b><c>\tv\t",
  N_("Increment current route style via size\n"),
  "<b><s><c>v ",
  N_("Decrement current route style via size\n"),
  "<b><a>\tv\t",
  N_("Increment current route style via hole size\n"),
  "<b><s><a>v ",
  N_("Decrement current route style via hole size\n"),
  "\n",

  "<b><c>\tx\t",
  N_("Copy selection to buffer and enter pastebuffer mode\n"),
  "<b><s><c>x ",
  N_("Cut selection to buffer and enter pastebuffer mode\n"),
  "\n",

  "<b>\tz\t",
  N_("Zoom in\n"),
  "<b><z>\tz\t",
  N_("Zoom out\n"),
  "\n",

  "<b>\t|\t",
  N_("Toggle thin draw mode\n"),
  "\n",

  "<b>\t/\t",
  N_("Cycle multiline mode (Using <s> overrides)\n"),
  "\n",

  "<b>\t.\t",
  N_("Toggle all direction lines mode\n"),
  "\n",

  "<b>\tEsc\t",
  N_("If drawing an object, return to a neutral state.\n"),
  "\n",

  "<b>\tTab\t",
  N_("Switch view to other side\n"),
  "\n",

  "<b>  Space\t",
  N_("Switch to select mode\n"),
  "\n",

  "<b>\t:\t",
  N_("Enter user command or pop up command window\n"),
  "\n",

  "<b>\tDEL\t",
  N_("Delete object\n"),
  "\n",

  "<b>\t1-9\t",
  N_("Select drawing layers\n"),
  "\n",

  "<b><c>\t1-5\t",
  N_("Select current buffer\n"),
  "\n",
  "\n",
  "<h>",
  N_("Mouse\n"),
  N_("Modifier key use can be combined with mouse button presses\n"
     "to modify mouse button actions.\n"),
  "\n",
  N_("<b>Left button\n"),
  N_("\tPerform or initiate action determined by current mode.\n"),
  "\n",
  "\t",
  N_("<b><shift>"),
  N_(" - change rotation direction for rotation tool actions.\n"),
  "\n",
  N_("\tAfter a draw operation has been left mouse button initiated,\n"
     "\tmodifier key effects:\n"),
  "\t",
  N_("<b><shift>"),
  N_(" - change line 45 degree direction and arc angle direction,\n"),
  "\n",
  N_("<b>Middle button\n"),
  N_("\tIf a line, arc, rectangle, or polygon draw operation has been\n"
     "\tinitiated, a click restarts the draw operation at the cursor position.\n"),
  "\n",
  N_("\tIf such a draw has not been initiated, a click selects objects and\n"
     "\ta press and drag moves objects.\n"),
  "\n",
  N_("<b>Right button\n"),
  N_("\tPress and drag to pan.\n"
     "\tWhile drawing or moving, a click without a drag toggles auto pan mode.\n"),
  "\n\t",
  N_("<b><shift>"),
  N_(" - Popup a menu.\n"),
  "\n",
  N_("<b>Scroll wheel\n"),
  N_("\tZoom in/out.\n"),
  "\n\t",
  N_("<b><shift>"),
  N_(" - pan vertically.\n"),
  "\t",
  N_("<b><ctrl>"),
  N_(" - pan horizontally.\n"),
  "\n",
  N_("<b>Usage:\n"),
  N_("\tMouse actions can typically be combined.  For example: while moving\n"
     "\tan object (with left or middle press and drag), the right button may\n"
     "\tbe simultaneously clicked to toggle auto pan or pressed and dragged\n"
     "\tto manually pan.  Mouse moving or drawing may also be combined with\n"
     "\tkey actions.\n"),
};




  /* Remember user window resizes.
   */
static gint
keyref_window_configure_event_cb (GtkWidget * widget, GdkEventConfigure * ev,
				  gpointer data)
{
  GtkAllocation allocation;

  gtk_widget_get_allocation (widget, &allocation);
  ghidgui->keyref_window_width = allocation.width;
  ghidgui->keyref_window_height = allocation.height;
  ghidgui->config_modified = TRUE;
  return FALSE;
}

static void
keyref_close_cb (gpointer data)
{
  gtk_widget_destroy (keyref_window);
  keyref_window = NULL;
}

static void
keyref_destroy_cb (GtkWidget * widget, gpointer data)
{
  keyref_window = NULL;
}

void
ghid_keyref_window_show (gboolean raise)
{
  GtkWidget *vbox, *hbox, *button, *text;
  gint i;

  if (keyref_window)
    {
      if (raise)
			  gtk_window_present(GTK_WINDOW(keyref_window));
      return;
    }
  keyref_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (keyref_window), "destroy",
		    G_CALLBACK (keyref_destroy_cb), NULL);
  g_signal_connect (G_OBJECT (keyref_window), "configure_event",
		    G_CALLBACK (keyref_window_configure_event_cb), NULL);
  gtk_window_set_title (GTK_WINDOW (keyref_window), _("PCB Key Reference"));
  gtk_window_set_wmclass (GTK_WINDOW (keyref_window), "PCB_Keyref", "PCB");
  gtk_window_resize (GTK_WINDOW (keyref_window),
                     ghidgui->keyref_window_width,
                     ghidgui->keyref_window_height);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (keyref_window), vbox);

  text = ghid_scrolled_text_view (vbox, NULL,
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  for (i = 0; i < sizeof (key_ref_text) / sizeof (gchar *); ++i)
    ghid_text_view_append (text, _(key_ref_text[i]));

  /* The keyref window close button.
   */
  hbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_END);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (keyref_close_cb), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

  gtk_widget_show_all (keyref_window);

}
