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

#include "global.h"

#include "gui.h"

#include "copy.h"
#include "data.h"
#include "draw.h"
#include "mymem.h"
#include "move.h"
#include "rotate.h"

#include "gui-pinout-preview.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

static void
pinout_close_cb (GtkWidget * widget, GtkWidget *top_window)
{
  gtk_widget_destroy (top_window);
}


void
ghid_pinout_window_show (GHidPort * out, ElementType * element)
{
  GtkWidget *button, *vbox, *hbox, *preview, *top_window;
  gchar *title;
  int width, height;

  if (!element)
    return;
  title = g_strdup_printf ("%s [%s,%s]",
			   UNKNOWN (DESCRIPTION_NAME (element)),
			   UNKNOWN (NAMEONPCB_NAME (element)),
			   UNKNOWN (VALUE_NAME (element)));

  top_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (top_window), title);
  g_free (title);
  gtk_window_set_wmclass (GTK_WINDOW (top_window), "PCB_Pinout", "PCB");
  gtk_container_set_border_width (GTK_CONTAINER (top_window), 4);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (top_window), vbox);


  preview = ghid_pinout_preview_new (element);
  gtk_box_pack_start (GTK_BOX (vbox), preview, TRUE, TRUE, 0);

  ghid_pinout_preview_get_natural_size (GHID_PINOUT_PREVIEW (preview),
                                        &width, &height);

  gtk_window_resize (GTK_WINDOW (top_window), width + 50, height + 50);

  hbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_END);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (pinout_close_cb), top_window);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

  gtk_widget_realize (top_window);
  if (Settings.AutoPlace)
    gtk_window_move (GTK_WINDOW (top_window), 10, 10);
  gtk_widget_show_all (top_window);
}
