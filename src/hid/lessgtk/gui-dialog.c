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

/* This file written by Bill Wilson for the PCB Gtk port.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "data.h"
#include "gui.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------- */
gchar *
ghid_dialog_input (const char * prompt, const char * initial)
{
  GtkWidget *dialog;
  GtkWidget *content_area;
  GtkWidget *vbox, *label, *entry;
  gchar *string;
  gboolean response;
  GHidPort *out = &ghid_port;

  dialog = gtk_dialog_new_with_buttons (_("PCB User Input"),
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
			prompt ? prompt : _("Enter something"));

  entry = gtk_entry_new ();
  if (initial)
    gtk_entry_set_text (GTK_ENTRY (entry), initial);

  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 0);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_container_add (GTK_CONTAINER (content_area), vbox);
  gtk_widget_show_all (dialog);

  response = gtk_dialog_run (GTK_DIALOG (dialog));
  if (response != GTK_RESPONSE_OK)
    string = g_strdup (initial ? initial : "");
  else
    string = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);

  gtk_widget_destroy (dialog);
  return string;
}

/* ---------------------------------------------- */
void
ghid_dialog_about (void)
{
  GtkWidget *dialog;
  GHidPort *out = &ghid_port;
  dialog = gtk_message_dialog_new (GTK_WINDOW (out->top_window),
				   (GtkDialogFlags)(GTK_DIALOG_MODAL
						    | GTK_DIALOG_DESTROY_WITH_PARENT),
				   GTK_MESSAGE_INFO,
				   GTK_BUTTONS_OK,
				   "%s", GetInfoString ());

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

/* ---------------------------------------------- */
gint
ghid_dialog_confirm_all (gchar * all_message)
{
  GtkWidget *dialog;
  GtkWidget *content_area;
  GtkWidget *label, *vbox;
  gint response;
  GHidPort *out = &ghid_port;

  dialog = gtk_dialog_new_with_buttons (_("Confirm"),
					GTK_WINDOW (out->top_window),
					(GtkDialogFlags)(GTK_DIALOG_MODAL |
							 GTK_DIALOG_DESTROY_WITH_PARENT),
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OK, GTK_RESPONSE_OK,
					_("Sequence OK"),
					GUI_DIALOG_RESPONSE_ALL, NULL);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  vbox = ghid_framed_vbox (content_area, NULL, 6, FALSE, 4, 6);

  label = gtk_label_new (all_message);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);
  gtk_widget_show_all (dialog);

  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  return response;
}

/* ---------------------------------------------- */
void
ghid_dialog_message (gchar * message)
{
  GtkWidget *dialog;
  GHidPort *out = &ghid_port;

  dialog = gtk_message_dialog_new (GTK_WINDOW (out->top_window),
				   (GtkDialogFlags)(GTK_DIALOG_MODAL |
						    GTK_DIALOG_DESTROY_WITH_PARENT),
				   GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
				   "%s", message);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

/* ---------------------------------------------- */
gboolean
ghid_dialog_confirm (gchar * message, gchar * cancelmsg, gchar * okmsg)
{
  static gint x = -1, y = -1;
  GtkWidget *dialog;
  gboolean confirm = FALSE;
  GHidPort *out = &ghid_port;

  if (cancelmsg == NULL)
    {
      cancelmsg = _("_Cancel");
    }
  if (okmsg == NULL)
    {
      okmsg = _("_OK");
    }

  dialog = gtk_message_dialog_new (GTK_WINDOW (out->top_window),
				   (GtkDialogFlags)(GTK_DIALOG_MODAL |
						    GTK_DIALOG_DESTROY_WITH_PARENT),
				   GTK_MESSAGE_QUESTION,
				   GTK_BUTTONS_NONE,
				   "%s", message);
  gtk_dialog_add_buttons (GTK_DIALOG (dialog), 
			  cancelmsg, GTK_RESPONSE_CANCEL,
			  okmsg, GTK_RESPONSE_OK,
			  NULL);

  if(x != -1) {
  	gtk_window_move(GTK_WINDOW (dialog), x, y);
  }

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    confirm = TRUE;

  gtk_window_get_position(GTK_WINDOW (dialog), &x, &y);

  gtk_widget_destroy (dialog);
  return confirm;
}

/* ---------------------------------------------- */
gint
ghid_dialog_close_confirm ()
{
  GtkWidget *dialog;
  gint rv;
  GHidPort *out = &ghid_port;
  gchar *tmp;
  gchar *str;

  if (PCB->Filename == NULL)
    {
      tmp = g_strdup_printf (
              _("Save the changes to layout before closing?"));
    } else {
      tmp = g_strdup_printf (
              _("Save the changes to layout \"%s\" before closing?"),
              PCB->Filename);
    }
  str = g_strconcat ("<big><b>", tmp, "</b></big>", NULL);
  g_free (tmp);
  tmp = _("If you don't save, all your changes will be permanently lost.");
  str = g_strconcat (str, "\n\n", tmp, NULL);

  dialog = gtk_message_dialog_new (GTK_WINDOW (out->top_window),
                                   (GtkDialogFlags)(GTK_DIALOG_MODAL |
						    GTK_DIALOG_DESTROY_WITH_PARENT),
                                     GTK_MESSAGE_WARNING,
                                   GTK_BUTTONS_NONE, NULL);
  gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (dialog), str);
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          _("Close _without saving"), GTK_RESPONSE_NO,
                          GTK_STOCK_CANCEL,          GTK_RESPONSE_CANCEL,
                          GTK_STOCK_SAVE,            GTK_RESPONSE_YES,
                          NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

  /* Set the alternative button order (ok, cancel, help) for other systems */
  gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog),
                                          GTK_RESPONSE_YES,
                                          GTK_RESPONSE_NO,
                                          GTK_RESPONSE_CANCEL,
                                          -1);

  switch (gtk_dialog_run (GTK_DIALOG (dialog)))
    {
      case GTK_RESPONSE_NO:
        {
          rv = GUI_DIALOG_CLOSE_CONFIRM_NOSAVE;
          break;
        }
      case GTK_RESPONSE_YES:
        {
          rv = GUI_DIALOG_CLOSE_CONFIRM_SAVE;
          break;
        }
      case GTK_RESPONSE_CANCEL:
      default:
        {
          rv = GUI_DIALOG_CLOSE_CONFIRM_CANCEL;
          break;
        }
      }
  gtk_widget_destroy (dialog);
  return rv;
}

/* ---------------------------------------------- */
/* Caller must g_free() the returned filename.*/
gchar *
ghid_dialog_file_select_open (gchar * title, gchar ** path, gchar * shortcuts)
{
  GtkWidget *dialog;
  gchar *result = NULL, *folder, *seed;
  GHidPort *out = &ghid_port;
  GtkFileFilter *no_filter;

  dialog = gtk_file_chooser_dialog_new (title,
					GTK_WINDOW (out->top_window),
					GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OK, GTK_RESPONSE_OK,
					NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  /* add a default filter for not filtering files */
  no_filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (no_filter, "all");
  gtk_file_filter_add_pattern (no_filter, "*.*");
  gtk_file_filter_add_pattern (no_filter, "*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), no_filter);

  /* in case we have a dialog for loading a footprint file */
  if (strcmp (title, _("Load element to buffer")) == 0)
  {
    /* add a filter for footprint files */
    GtkFileFilter *fp_filter;
    fp_filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (fp_filter, "fp");
    gtk_file_filter_add_mime_type (fp_filter, "application/x-pcb-footprint");
    gtk_file_filter_add_pattern (fp_filter, "*.fp");
    gtk_file_filter_add_pattern (fp_filter, "*.FP");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), fp_filter);
  }

  /* in case we have a dialog for loading a layout file */
  if ((strcmp (title, _("Load layout file")) == 0)
    || (strcmp (title, _("Load layout file to buffer")) == 0))
  {
    /* add a filter for layout files */
    GtkFileFilter *pcb_filter;
    pcb_filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (pcb_filter, "pcb");
    gtk_file_filter_add_mime_type (pcb_filter, "application/x-pcb-layout");
    gtk_file_filter_add_pattern (pcb_filter, "*.pcb");
    gtk_file_filter_add_pattern (pcb_filter, "*.PCB");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), pcb_filter);
  }

  /* in case we have a dialog for loading a netlist file */
  if (strcmp (title, _("Load netlist file")) == 0)
  {
    /* add a filter for netlist files */
    GtkFileFilter *net_filter;
    net_filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (net_filter, "netlist");
    gtk_file_filter_add_mime_type (net_filter, "application/x-pcb-netlist");
    gtk_file_filter_add_pattern (net_filter, "*.net");
    gtk_file_filter_add_pattern (net_filter, "*.NET");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), net_filter);
  }

  if (path && *path)
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), *path);
  else
  {
	gchar *default_path;
	default_path = g_get_current_dir();
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), default_path);
	g_free(default_path);
  }

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


/* ---------------------------------------------- */
/* Caller must g_slist_free() the returned list .*/
GSList *
ghid_dialog_file_select_multiple(gchar * title, gchar ** path, gchar * shortcuts)
{
  GtkWidget *dialog;
  GSList *result = NULL;
  gchar *folder, *seed;
  GHidPort *out = &ghid_port;
  GtkFileFilter *no_filter;

  dialog = gtk_file_chooser_dialog_new (title,
					GTK_WINDOW (out->top_window),
					GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OK, GTK_RESPONSE_OK,
					NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER (dialog), TRUE);

  /* add a default filter for not filtering files */
  no_filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (no_filter, "all");
  gtk_file_filter_add_pattern (no_filter, "*.*");
  gtk_file_filter_add_pattern (no_filter, "*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), no_filter);

  /* in case we have a dialog for loading schematic files */
  if (strcmp (title, _("Load schematics")) == 0)
  {
    /* add a filter for schematic files */
    GtkFileFilter *sch_filter;
    sch_filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (sch_filter, "sch");
    gtk_file_filter_add_mime_type (sch_filter, "application/x-geda-schematic");
    gtk_file_filter_add_pattern (sch_filter, "*.sch");
    gtk_file_filter_add_pattern (sch_filter, "*.SCH");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), sch_filter);
  }

  if (path && *path)
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), *path);
  else
  {
	gchar *default_path;
	default_path = g_get_current_dir();
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), default_path);
	g_free(default_path);
  }

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
      result = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dialog));
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


/* ---------------------------------------------- */
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
					GTK_STOCK_OK, GTK_RESPONSE_OK,
					NULL);

  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                  TRUE);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  if (path && *path && **path)
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), *path);
  else
  {
	gchar *default_path;
	default_path = g_get_current_dir();
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), default_path);
	g_free(default_path);
  }

  if (file && *file)
    {
      gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog),
                                         g_path_get_basename(file));
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),
                                           g_path_get_dirname (file));
    }

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


/* ---------------------------------------------- */
/* how many files and directories to keep for the shortcuts */
#define NHIST 8
typedef struct ghid_file_history_struct
{
  /* 
   * an identifier as to which recent files pool this is.  For example
   * "boards", "eco", "netlists", etc.
   */
  char * id;

  /* 
   * the array of files or directories
   */
  char * history[NHIST];
} ghid_file_history;

static int n_recent_dirs = 0;
static ghid_file_history * recent_dirs = NULL;

/* ---------------------------------------------- */
/* Caller must g_free() the returned filename. */
gchar *
ghid_fileselect (const char *title, const char *descr,
                 char *default_file, char *default_ext,
		 const char *history_tag, int flags)
{
  GtkWidget *dialog;
  gchar *result = NULL;
  GHidPort *out = &ghid_port;
  gchar *path = NULL, *base = NULL;
  int history_pool = -1;
  int i;

  if (history_tag && *history_tag)
    {
      /* 
       * I used a simple linear search here because the number of
       * entries in the array is likely to be quite small (5, maybe 10 at
       * the absolute most) and this function is used when pulling up
       * a file dialog box instead of something called over and over
       * again as part of moving elements or autorouting.  So, keep it
       * simple....
       */
      history_pool = 0;
      while (history_pool < n_recent_dirs &&
	     strcmp (recent_dirs[history_pool].id, history_tag) != 0)
	{
	  history_pool++;
	}
      
      /*
       * If we counted all the way to n_recent_dirs, that means we
       * didn't find our entry
       */
      if (history_pool >= n_recent_dirs)
	{
	  n_recent_dirs++;

	  recent_dirs = (ghid_file_history *)realloc (recent_dirs, 
				  n_recent_dirs * sizeof (ghid_file_history));

	  if (recent_dirs == NULL)
	    {
	      fprintf (stderr, "%s():  realloc failed\n", __FUNCTION__);
	      exit (1);
	    }
	  
	  recent_dirs[history_pool].id = strdup (history_tag);

	  /* Initialize the entries in our history list to all be NULL */
	  for (i = 0; i < NHIST; i++)
	    {
	      recent_dirs[history_pool].history[i] = NULL;
	    }
	}
    }

  if (default_file && *default_file)
    {
      path = g_path_get_dirname (default_file);
      base = g_path_get_basename (default_file);
    }

  dialog = gtk_file_chooser_dialog_new (title,
					GTK_WINDOW (out->top_window),
					(flags & HID_FILESELECT_READ) ? 
					GTK_FILE_CHOOSER_ACTION_OPEN : 
					GTK_FILE_CHOOSER_ACTION_SAVE,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OK, GTK_RESPONSE_OK,
					NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  if (path && *path )
    {
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), path);
      g_free (path);
    }
  else
	{
	  gchar *default_path;
	  default_path = g_get_current_dir();
	  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), default_path);
	  g_free(default_path);
	}

  if (base && *base)
    {
      /* default file is only supposed to be for writing, not reading */
      if (!(flags & HID_FILESELECT_READ))
	{
	  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), base);
	}
      g_free (base);
    }

  for (i = 0; i < NHIST && recent_dirs[history_pool].history[i] != NULL ; i++)
    {
      gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (dialog),
					    recent_dirs[history_pool].history[i], 
					    NULL);
    }

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      result = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      if (result != NULL)
	path = g_path_get_dirname (result);
      else
	path = NULL;

      /* update the history list */
      if (path != NULL)
	{
	  char *tmps, *tmps2;
	  int k = 0;

	  /* 
	   * Put this at the top of the list and bump everything else
	   * down but skip any old entry of this directory
	   *
	   */
	  while ( k < NHIST &&
		 recent_dirs[history_pool].history[k] != NULL &&
		 strcmp ( recent_dirs[history_pool].history[k], path) == 0)
	    {
	      k++;
	    }
	  tmps = recent_dirs[history_pool].history[k];
	  recent_dirs[history_pool].history[0] = path;
	  for (i = 1 ; i < NHIST ; i++)
	    {
	      /* store our current entry, but skip duplicates */
	      while (i + k < NHIST &&
		     recent_dirs[history_pool].history[i + k] != NULL &&
		     strcmp ( recent_dirs[history_pool].history[i + k], path) == 0)
		{
		  k++;
		}
		     
	      if (i + k < NHIST)
		tmps2 = recent_dirs[history_pool].history[i + k];
	      else
		tmps2 = NULL;

	      /* move down the one we stored last time */
	      recent_dirs[history_pool].history[i] = tmps;

	      /* and remember the displace entry */
	      tmps = tmps2;
	    }

	  /* 
	   * the last one has fallen off the end of the history list
	   * so we need to free() it.
	   */
	  if (tmps)
	    {
	      free (tmps);
	    }
	}
      
#ifdef DEBUG
      printf ("\n\n-----\n\n");
      for (i = 0 ; i < NHIST ; i++)
	{
	  printf ("After update recent_dirs[%d].history[%d] = \"%s\"\n",
		  history_pool, i, recent_dirs[history_pool].history[i] != NULL ?
		  recent_dirs[history_pool].history[i] : "NULL");
	}
#endif

    }
  gtk_widget_destroy (dialog);


  return result;
}

