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

/* This file written by Bill Wilson for the PCB Gtk port.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "data.h"
#include "gui.h"
#include "command.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

gchar *
ghid_dialog_input (gchar * prompt, gchar * initial)
{
  GtkWidget *dialog, *vbox, *label, *entry;
  gchar *string;
  gboolean response;
  GHidPort *out = &ghid_port;

  dialog = gtk_dialog_new_with_buttons ("PCB User Input",
					GTK_WINDOW (out->top_window),
					GTK_DIALOG_MODAL,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  label = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);

  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_markup (GTK_LABEL (label),
			prompt ? prompt : "Enter something");

  entry = gtk_entry_new ();
  if (initial)
    gtk_entry_set_text (GTK_ENTRY (entry), initial);

  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  gtk_box_pack_start_defaults (GTK_BOX (vbox), entry);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);
  gtk_widget_show_all (dialog);

  response = gtk_dialog_run (GTK_DIALOG (dialog));
  if (response != GTK_RESPONSE_OK)
    string = g_strdup (initial ? initial : "");
  else
    string = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);

  gtk_widget_destroy (dialog);
  return string;
}

void
ghid_dialog_about (void)
{
  GtkWidget *dialog;
  GHidPort *out = &ghid_port;

  dialog = gtk_message_dialog_new (GTK_WINDOW (out->top_window),
				   GTK_DIALOG_MODAL
				   | GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_INFO,
				   GTK_BUTTONS_OK,
				   "This is PCB, an interactive\n"
				   "printed circuit board editor\n"
				   "version " VERSION "\n\n"
				   "Compiled on " __DATE__ " at " __TIME__
				   "\n\n" "by harry eaton\n\n"
				   "Copyright (C) Thomas Nau 1994, 1995, 1996, 1997\n"
				   "Copyright (C) harry eaton 1998-2006\n"
				   "Copyright (C) C. Scott Ananian 2001\n"
				   "Copyright (C) DJ Delorie 2003, 2004, 2005, 2006\n"
				   "Copyright (C) Dan McMahill 2003, 2004, 2005, 2006\n\n"
				   "It is licensed under the terms of the GNU\n"
				   "General Public License version 2\n"
				   "See the LICENSE file for more information\n\n"
				   "For more information see:\n\n"
				   "PCB homepage: http://pcb.sf.net\n"
				   "gEDA homepage: http://www.geda.seul.org\n"
				   "gEDA Wiki: http://geda.seul.org/dokuwiki/doku.php?id=geda\n\n"
				   );

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

gint
ghid_dialog_confirm_all (gchar * all_message)
{
  GtkWidget *dialog, *label, *vbox;
  gint response;
  GHidPort *out = &ghid_port;

  dialog = gtk_dialog_new_with_buttons ("Confirm",
					GTK_WINDOW (out->top_window),
					GTK_DIALOG_MODAL |
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OK, GTK_RESPONSE_OK,
					"Sequence OK",
					GUI_DIALOG_RESPONSE_ALL, NULL);

  vbox = ghid_framed_vbox (GTK_DIALOG (dialog)->vbox, NULL, 6, FALSE, 4, 6);

  label = gtk_label_new (all_message);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);
  gtk_widget_show_all (dialog);

  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  return response;
}

void
ghid_dialog_message (gchar * message)
{
  GtkWidget *dialog;
  GHidPort *out = &ghid_port;

  dialog = gtk_message_dialog_new (GTK_WINDOW (out->top_window),
				   GTK_DIALOG_MODAL |
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
				   message);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

gboolean
ghid_dialog_confirm (gchar * message)
{
  GtkWidget *dialog;
  gboolean confirm = FALSE;
  GHidPort *out = &ghid_port;

  dialog = gtk_message_dialog_new (GTK_WINDOW (out->top_window),
				   GTK_DIALOG_MODAL |
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_QUESTION,
				   GTK_BUTTONS_OK_CANCEL, message);
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    confirm = TRUE;
  gtk_widget_destroy (dialog);
  return confirm;
}

/* Caller must g_free() the returned filename.*/
gchar *
ghid_dialog_file_select_open (gchar * title, gchar ** path, gchar * shortcuts)
{
  GtkWidget *dialog;
  gchar *result = NULL, *folder, *seed;
  GHidPort *out = &ghid_port;

  dialog = gtk_file_chooser_dialog_new (title,
					GTK_WINDOW (out->top_window),
					GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_OK,
					NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  if (path && *path)
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), *path);

  if (shortcuts && *shortcuts)
    {
      folder = g_strdup (shortcuts);
      seed = folder;
      while ((folder = strtok (seed, ":")) != NULL)
	{
	  gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (dialog),
						folder, NULL);
	  seed = NULL;
	}
      g_free (folder);
    }

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      result = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      folder =
	gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
      if (folder && path)
	{
	  dup_string (path, folder);
	  g_free (folder);
	}
    }
  gtk_widget_destroy (dialog);


  return result;
}

/* Caller must g_free() the returned filename. */
gchar *
ghid_dialog_file_select_save (gchar * title, gchar ** path, gchar * file,
			      gchar * shortcuts)
{
  GtkWidget *dialog;
  gchar *result = NULL, *folder, *seed;
  GHidPort *out = &ghid_port;

  dialog = gtk_file_chooser_dialog_new (title,
					GTK_WINDOW (out->top_window),
					GTK_FILE_CHOOSER_ACTION_SAVE,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_OK,
					NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  if (path && *path && **path)
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), *path);

  if (file && *file)
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), file);

  if (shortcuts && *shortcuts)
    {
      folder = g_strdup (shortcuts);
      seed = folder;
      while ((folder = strtok (seed, ":")) != NULL)
	{
	  gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (dialog),
						folder, NULL);
	  seed = NULL;
	}
      g_free (folder);
    }
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      result = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      folder =
	gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
      if (folder && path)
	{
	  dup_string (path, folder);
	  g_free (folder);
	}
    }
  gtk_widget_destroy (dialog);


  return result;
}
