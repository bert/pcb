/*!
 * \file ghellopcb.c
 *
 * \author Copyright (C) 2009 by Bert Timmerman <bert.timmerman@xs4all.nl>
 *
 * \brief Plug-in for PCB to say "Hello world".
 *
 * Function to show a GTK dialog with a "Hello world" message on the
 * screen.\n
 * \n
 * Compile like this:\n
 * \n
   gcc -Wall -Ipath/to/pcb/src -Ipath/to/pcb -O2 -shared -lglib-2.0 \
   ghellopcb.c -o ghellopcb.so `pkg-config --cflags gtk+-2.0 --libs gtk+-2.0`
 * \n\n
 * The resulting ghellopcb.so file should go in $HOME/.pcb/plugins/\n
 * \n
 * \warning Be very strict in compiling this plug-in against the exact pcb
 * sources you compiled/installed the pcb executable (i.e. src/pcb) with.\n
 *
 * Usage: gHelloPCB()\n
 * <hr>
 * This program is free software; you can redistribute it and/or modify\n
 * it under the terms of the GNU General Public License as published by\n
 * the Free Software Foundation; either version 2 of the License, or\n
 * (at your option) any later version.\n
 * \n
 * This program is distributed in the hope that it will be useful,\n
 * but WITHOUT ANY WARRANTY; without even the implied warranty of\n
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.\n
 * \n
 * You should have received a copy of the GNU General Public License\n
 * along with this program; if not, write to:\n
 * the Free Software Foundation, Inc.,\n
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.\n
 */


#include <config.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <math.h>

#include "global.h"
#include "data.h"
#include "hid.h"
#include "misc.h"
#include "create.h"
#include "rtree.h"
#include "undo.h"
#include "set.h"

/*!
 * \brief Terminate the main loop.
 */
static void
on_destroy (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
}

/*!
 * \brief Show a GTK dialog with a "Hello world" message on the screen.
 *
 * Usage: gHelloPCB()\n
 */
static int
ghellopcb (int argc, char **argv, Coord x, Coord y)
{
  GtkWidget *window;
  GtkWidget *label;
  gtk_init (&argc, &argv);
  /* create a new top level window */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  /* give the window a 20px wide border */
  gtk_container_set_border_width (GTK_CONTAINER (window), 20);
  /* give it the title */
  gtk_window_set_title (GTK_WINDOW (window), PACKAGE " " VERSION);
  /* open it a bit wider so that both the label and title show up */
  gtk_window_set_default_size (GTK_WINDOW (window), 200, 50);
  /* Connect the destroy event of the window with our on_destroy function
   * When the window is about to be destroyed we get a notificaton and
   * stop the main GTK loop
   */
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (on_destroy), NULL);
  /* Create the "Hello, PCB World" label  */
  label = gtk_label_new ("Hello, PCB World");
  /* and insert it into the main window  */
  gtk_container_add (GTK_CONTAINER (window), label);
  /* make sure that everything, window and label, are visible */
  gtk_widget_show_all (window);
  /* start the main loop */
  gtk_main ();
  return 0;
}


static HID_Action ghellopcb_action_list[] =
{
  {"gHelloPCB", NULL, ghellopcb, "show a GTK dialog with a Hello world message on the screen", NULL}
};


REGISTER_ACTIONS (ghellopcb_action_list)


void
pcb_plugin_init ()
{
  register_ghellopcb_action_list ();
}

/* EOF */
