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

/* This file written by Bill Wilson for the PCB Gtk port
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gui.h"
#include "pcb-printf.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

static GtkWidget *log_window, *log_text;
static gboolean log_show_on_append = FALSE;

/* Remember user window resizes. */
static gint
log_window_configure_event_cb (GtkWidget * widget,
			       GdkEventConfigure * ev, gpointer data)
{
  GtkAllocation allocation;

  gtk_widget_get_allocation (widget, &allocation);
  ghidgui->log_window_width = allocation.width;
  ghidgui->log_window_height = allocation.height;
  ghidgui->config_modified = TRUE;

  return FALSE;
}

static void
log_close_cb (gpointer data)
{
  gtk_widget_destroy (log_window);
  log_window = NULL;
}

static void
log_destroy_cb (GtkWidget * widget, gpointer data)
{
  log_window = NULL;
}

void
ghid_log_window_create ()
{
  GtkWidget *vbox, *hbox, *button;

  if (log_window)
    return;

  log_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (log_window), "destroy",
		    G_CALLBACK (log_destroy_cb), NULL);
  g_signal_connect (G_OBJECT (log_window), "configure_event",
		    G_CALLBACK (log_window_configure_event_cb), NULL);
  gtk_window_set_title (GTK_WINDOW (log_window), _("PCB Log"));
  gtk_window_set_wmclass (GTK_WINDOW (log_window), "PCB_Log", "PCB");
  gtk_window_resize (GTK_WINDOW (log_window),
                     ghidgui->log_window_width,
                     ghidgui->log_window_height);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (log_window), vbox);

  log_text = ghid_scrolled_text_view (vbox, NULL,
				      GTK_POLICY_AUTOMATIC,
				      GTK_POLICY_AUTOMATIC);

  hbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_END);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (log_close_cb), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

  gtk_widget_realize (log_window);
  if (Settings.AutoPlace)
    gtk_window_move (GTK_WINDOW (log_window), 10, 10);
}

void
ghid_log_window_show (gboolean raise)
{
  ghid_log_window_create ();
  gtk_widget_show_all (log_window);
  if (raise)
    gtk_window_present (GTK_WINDOW(log_window));
}

static void
ghid_log_append_string (gchar * s)
{
  if (log_show_on_append)
    ghid_log_window_show(FALSE);
  else
    ghid_log_window_create ();
  ghid_text_view_append (log_text, s);
}

void
ghid_log (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  ghid_logv (fmt, ap);
  va_end (ap);
}

void
ghid_logv (const char *fmt, va_list args)
{
  gchar *msg = pcb_vprintf (fmt, args);
  ghid_log_append_string (msg);
  g_free (msg);
}

static const char logshowonappend_syntax[] =
  "LogShowOnAppend(true|false)";

static const char logshowonappend_help[] =
  "If true, the log window will be shown whenever something is appended \
to it.  If false, the log will still be updated, but the window won't \
be shown.";

static gint
GhidLogShowOnAppend (int argc, char **argv, Coord x, Coord y)
{
  char *a = argc == 1 ? argv[0] : (char *)"";

  if (strncasecmp(a, "t", 1) == 0)
    {
      log_show_on_append = TRUE;
    }
  else if (strncasecmp(a, "f", 1) == 0)
    {
      log_show_on_append = FALSE;
    }
  return 0;
}

HID_Action ghid_log_action_list[] = {
  {"LogShowOnAppend", 0, GhidLogShowOnAppend,
   logshowonappend_help, logshowonappend_syntax}
  ,
};

REGISTER_ACTIONS (ghid_log_action_list)
