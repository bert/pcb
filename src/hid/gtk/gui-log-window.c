/* $Id$ */

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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

static GtkWidget *log_window, *log_text;


/* Remember user window resizes. */
static gint
log_window_configure_event_cb (GtkWidget * widget,
			       GdkEventConfigure * ev, gpointer data)
{
  ghidgui->log_window_width = widget->allocation.width;
  ghidgui->log_window_height = widget->allocation.height;
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
ghid_log_window_show (gboolean raise)
{
  GtkWidget *vbox, *hbox, *button;

  if (log_window)
    {
      if (raise)
        gtk_window_present(GTK_WINDOW(log_window));
      return;
    }

  log_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (log_window), "destroy",
		    G_CALLBACK (log_destroy_cb), NULL);
  g_signal_connect (G_OBJECT (log_window), "configure_event",
		    G_CALLBACK (log_window_configure_event_cb), NULL);
  gtk_window_set_title (GTK_WINDOW (log_window), _("PCB Log"));
  gtk_window_set_wmclass (GTK_WINDOW (log_window), "PCB_Log", "PCB");
  gtk_window_set_default_size (GTK_WINDOW (log_window),
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

  /* 
   * If focus were grabbed, output drawing area would loose it which we
   * don't want.
   */
  gtk_widget_realize (log_window);
  gdk_window_set_accept_focus (log_window->window, FALSE);
  if (Settings.AutoPlace)
    gtk_widget_set_uposition (GTK_WIDGET (log_window), 10, 10);
  gtk_widget_show_all (log_window);
}

static void
ghid_log_append_string (gchar * s)
{
  ghid_log_window_show (FALSE);
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

static char *msg_buffer = 0;
static int msg_buffer_size = 0;

void
ghid_logv (const char *fmt, va_list args)
{
  int i;

  if (msg_buffer == NULL)
    {
      msg_buffer = (char *) malloc (1002);
      if (msg_buffer == NULL)
	{
	  fprintf (stderr, "ghid_logv():  malloc failed\n\n");
	  exit (1);
	}
      msg_buffer_size = 1000;
    }

  /* 
   * FIXME -- fix check to see if the main window is up.  Is 
   * that needed here?  Lesstif does it.
   */
#ifdef FIXME
  if (!mainwind)
    {
      vprintf (fmt, args);
      return;
    }
#endif

#ifdef HAVE_VSNPRINTF

  /* 
   * NOTE:  some implementations of vsnprintf(), for example
   * solaris-2.9, will not return the formatted length if you
   * use a length of 0 in the initial call.  So, we use a small
   * buffer.
   */
  i = vsnprintf (msg_buffer, msg_buffer_size, fmt, args);
  if (i >= msg_buffer_size)
    {
      msg_buffer_size = i + 100;
      msg_buffer = (char *) realloc (msg_buffer, msg_buffer_size + 2);
      if (msg_buffer == NULL)
	{
	  fprintf (stderr, "ghid_logv():  malloc failed\n\n");
	  exit (1);
	}
      vsnprintf (msg_buffer, msg_buffer_size, fmt, args);
    }

#else
  vsprintf (msg_buffer, fmt, args);
#endif /* !HAVE_VSNPRINTF */

  ghid_log_append_string (msg_buffer);

}
