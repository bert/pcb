/*!
 * \file src/hid/gtk/gui-command-window.c
 *
 * \brief Provides a command entry window for the GTK UI.
 *
 * This file was written by Bill Wilson for the PCB Gtk port.
 *
 * gui-command-window.c provides two interfaces for getting user input
 * for executing a command.
 *
 * As the Xt PCB was ported to Gtk, the traditional user entry in the
 * status line window presented some focus problems which require that
 * there can be no menu key shortcuts that might be a key the user would
 * type in.  It also requires a coordinating flag so the drawing area
 * won't grab focus while the command entry is up.
 *
 * I thought the interface should be cleaner, so I made an alternate
 * command window interface which works better I think as a gui interface.
 * The user must focus onto the command window, but since it's a separate
 * window, there's no confusion.  It has the restriction that objects to
 * be operated on must be selected, but that actually seems a better user
 * interface than one where typing into one location requires the user to
 * be careful about which object might be under the cursor somewhere else.
 *
 * In any event, both interfaces are here to work with.
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gui.h"
#include <gdk/gdkkeysyms.h>

#include "crosshair.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

static GtkWidget *command_window;
static GtkWidget *combo_vbox;
static GList *history_list;
static gchar *command_entered;
static GMainLoop *loop;


/*!
 * \brief When using a command window for command entry, provide a quick
 * and abbreviated reference to available commands.
 *
 * This is currently just a start and can be expanded if it proves
 * useful.
   */
static gchar *command_ref_text[] = {
  N_("Common commands easily accessible via the gui may not be included here.\n"),
  "\n",
  N_("In user commands below, 'size' values may be absolute or relative\n"
     "if preceded by a '+' or '-'.  Where 'units' are indicated, use \n"
     "'mil' or 'mm' otherwise PCB internal units will be used.\n"),
  "\n",
  "<b>changesize(target, size, units)\n",
  "\ttarget = {selectedlines | selectedpins | selectedvias | selectedpads \n"
    "\t\t\t| selectedtexts | selectednames | selectedelements | selected}\n",
  "\n",
  "<b>changedrillsize(target, size, units)\n",
  "\ttarget = {selectedpins | selectedvias | selectedobjects | selected}\n",
  "\n",
  "<b>changeclearsize(target, size, units)\n",
  "\ttarget = {selectedpins | selectedpads | selectedvias | selectedlines\n"
    "\t\t\t| selectedarcs | selectedobjects | selected}\n",
  N_("\tChanges the clearance of objects.\n"),
  "\n",
  "<b>setvalue(target, size, units)\n",
  "\ttarget = {grid | zoom | line | textscale | viadrillinghole\n"
    "\t\t\t| viadrillinghole | via}\n",
  N_("\tChanges values.  Omit 'units' for 'grid' and 'zoom'.\n"),
  "\n",
  "<b>changejoin(target)\n",
  "\ttarget = {object | selectedlines | selectedarcs | selected}\n",
  N_("\tChanges the join (clearance through polygons) of objects.\n"),
  "\n",
  "<b>changesquare(target)\n",
  "<b>setsquare(target)\n",
  "<b>clearsquare(target)\n",
  "\ttarget = {object | selectedelements | selectedpins | selected}\n",
  N_("\tToggles, sets, or clears the square flag of objects.\n"),
  "\n",
  "<b>changeoctagon(target)\n",
  "<b>setoctagon(target)\n",
  "<b>clearoctagon(target)\n",
  "\ttarget = {object | selectedelements | selectedpins selectedvias | selected}\n",
  N_("\tToggles, sets, or clears the octagon flag of objects.\n"),
  "\n",
  "<b>changehole(target)\n",
  "\ttarget = {object | selectedvias | selected}\n",
  N_("\tChanges the hole flag of objects.\n"),
  "\n",
  "<b>flip(target)\n",
  "\ttarget = {object | selectedelements | selected}\n",
  N_("\tFlip elements to the opposite side of the board.\n"),
  "\n",
  "<b>setthermal(target, style)\n",
  "\ttarget = {object | selectedpins | selectedvias | selected}\n",
  "\tstyle  = {0 | 1 | 2 | 3 | 4 | 5}\n\n",
  N_("\tSet or clear a thermal (on the current layer) to pins or vias.\n"
     "\tIf 'style' is omitted, the layout's default style is taken. Setting\n"
     "\tthermals to style 0 (zero) turns the thermals off.\n"),
  "\n",
  "<b>loadvendorfrom(filename)\n",
  "<b>unloadvendor()\n",
  "\ttarget = [filename]\n",
  N_("\tLoad a vendor file.  If 'filename' is omitted, pop up a file select dialog.\n"),
};


/*!
 * \brief Put an allocated string on the history list and combo text
 * list if it is not a duplicate.
 *
 * The history_list is just a shadow of the combo list, but I think is
 * needed because I don't see an api for reading the combo strings.
 * The combo box strings take "const gchar *", so the same allocated
 * string can go in both the history list and the combo list.
 *
 * If removed from both lists, a string can be freed.
 */
static void
command_history_add (gchar * cmd)
{
  GList *list;
  gchar *s;
  gint i;

  if (!cmd || !*cmd)
    return;

  /* Check for a duplicate command.  If found, move it to the
     |  top of the list and similarly modify the combo box strings.
   */
  for (i = 0, list = history_list; list; list = list->next, ++i)
    {
      s = (gchar *) list->data;
      if (!strcmp (cmd, s))
	{
	  history_list = g_list_remove (history_list, s);
	  history_list = g_list_prepend (history_list, s);
	  gtk_combo_box_remove_text (GTK_COMBO_BOX
				     (ghidgui->command_combo_box), i);
	  gtk_combo_box_prepend_text (GTK_COMBO_BOX
				      (ghidgui->command_combo_box), s);
	  return;
	}
    }

  /* Not a duplicate, so put first in history list and combo box text list.
   */
  s = g_strdup (cmd);
  history_list = g_list_prepend (history_list, s);
  gtk_combo_box_prepend_text (GTK_COMBO_BOX (ghidgui->command_combo_box), s);

  /* And keep the lists trimmed!
   */
  if (g_list_length (history_list) > ghidgui->history_size)
    {
      s = (gchar *) g_list_nth_data (history_list, ghidgui->history_size);
      history_list = g_list_remove (history_list, s);
      gtk_combo_box_remove_text (GTK_COMBO_BOX (ghidgui->command_combo_box),
				 ghidgui->history_size);
      g_free (s);
    }
}


/*!
 * \brief Called when user hits "Enter" key in command entry.
 *
 * The action to take depends on where the combo box is.
 * If it's in the command window, we can immediately execute the command
 * and carry on.
 * If it's in the status line hbox, then we need stop the command
 * entry g_main_loop from running and save the allocated string so it
 * can be returned from ghid_command_entry_get()
 */
static void
command_entry_activate_cb (GtkWidget * widget, gpointer data)
{
  gchar *command;

  command =
    g_strdup (ghid_entry_get_text (GTK_WIDGET (ghidgui->command_entry)));
  gtk_entry_set_text (ghidgui->command_entry, "");

  if (*command)
    command_history_add (command);

  if (ghidgui->use_command_window)
    {
      hid_parse_command (command);
      g_free (command);
    }
  else
    {
      if (loop && g_main_loop_is_running (loop))	/* should always be */
	g_main_loop_quit (loop);
      command_entered = command;	/* Caller will free it */
    }
}


/*!
 * \brief Create the command_combo_box.
 *
 * Called once, either by ghid_command_window_show() or
 * ghid_command_entry_get().
 * Then as long as ghidgui->use_command_window is TRUE, the
 * command_combo_box will live in a command window vbox or float if the
 * command window is not up.
 * But if ghidgui->use_command_window is FALSE, the command_combo_box
 * will live in the status_line_hbox either shown or hidden.
 * Since it's never destroyed, the combo history strings never need
 * rebuilding and history is maintained if the combo box location is
 * moved.
 */
static void
command_combo_box_entry_create (void)
{
  ghidgui->command_combo_box = gtk_combo_box_entry_new_text ();
  ghidgui->command_entry =
    GTK_ENTRY (gtk_bin_get_child (GTK_BIN (ghidgui->command_combo_box)));

  gtk_entry_set_width_chars (ghidgui->command_entry, 40);
  gtk_entry_set_activates_default (ghidgui->command_entry, TRUE);

  g_signal_connect (G_OBJECT (ghidgui->command_entry), "activate",
		    G_CALLBACK (command_entry_activate_cb), NULL);

  g_object_ref (G_OBJECT (ghidgui->command_combo_box));	/* so can move it */
}


static void
command_window_disconnect_combobox ()
{
  if (command_window)
  {
    gtk_container_remove (GTK_CONTAINER (combo_vbox),	/* Float it */
                          ghidgui->command_combo_box);
  }
  combo_vbox = NULL;
}


static void
command_window_close_cb (GtkWidget * widget, gpointer data)
{
  command_window_disconnect_combobox();
  gtk_widget_destroy (command_window);
}


static gboolean
command_window_delete_event_cb (GtkWidget * widget, GdkEvent * event,
                                gpointer data)
{
  command_window_disconnect_combobox();
  return FALSE;
}


static void
command_destroy_cb (GtkWidget * widget, gpointer data)
{
  command_window = NULL;
}


static gboolean
command_escape_cb (GtkWidget * widget, GdkEventKey * kev, gpointer data)
{
  gint ksym = kev->keyval;

  if (ksym != GDK_Escape)
    return FALSE;

  if (command_window) {
    command_window_disconnect_combobox();
    gtk_widget_destroy (command_window);
  }

  if (loop && g_main_loop_is_running (loop))	/* should always be */
    g_main_loop_quit (loop);
  command_entered = NULL;	/* We are aborting */

  return TRUE;
}


/*!
 * \brief If ghidgui->use_command_window toggles, the config code calls
 * this to ensure the command_combo_box is set up for living in the
 * right place.
 */
void
ghid_command_use_command_window_sync (void)
{
  /* The combo box will be NULL and not living anywhere until the
     |  first command entry.
   */
  if (!ghidgui->command_combo_box)
    return;

  if (ghidgui->use_command_window)
    gtk_container_remove (GTK_CONTAINER (ghidgui->status_line_hbox),
			  ghidgui->command_combo_box);
  else
    {
      /* Destroy the window (if it's up) which floats the command_combo_box
         |  so we can pack it back into the status line hbox.  If the window
         |  wasn't up, the command_combo_box was already floating.
       */
      if(command_window)  command_window_close_cb (NULL, NULL);
      gtk_widget_hide (ghidgui->command_combo_box);
      gtk_box_pack_start (GTK_BOX (ghidgui->status_line_hbox),
			  ghidgui->command_combo_box, FALSE, FALSE, 0);
    }
}


/*!
 * \brief If ghidgui->use_command_window is TRUE this will get called
 * from ActionCommand() to show the command window.
 */
void
ghid_command_window_show (gboolean raise)
{
  GtkWidget *vbox, *vbox1, *hbox, *button, *expander, *text;
  gint i;

  if (command_window)
    {
      if (raise)
        gtk_window_present (GTK_WINDOW(command_window));
      return;
    }
  command_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (command_window), "destroy",
                    G_CALLBACK (command_destroy_cb), NULL);
  g_signal_connect (G_OBJECT (command_window), "delete-event",
                    G_CALLBACK (command_window_delete_event_cb), NULL);
  g_signal_connect (G_OBJECT (command_window), "key_press_event",
		    G_CALLBACK (command_escape_cb), NULL);
  gtk_window_set_title (GTK_WINDOW (command_window), _("PCB Command Entry"));
  gtk_window_set_wmclass (GTK_WINDOW (command_window), "PCB_Command", "PCB");
  gtk_window_set_resizable (GTK_WINDOW (command_window), FALSE);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (command_window), vbox);

  if (!ghidgui->command_combo_box)
    command_combo_box_entry_create ();

  gtk_box_pack_start (GTK_BOX (vbox), ghidgui->command_combo_box,
		      FALSE, FALSE, 0);
  combo_vbox = vbox;

  /* Make the command reference scrolled text view.  Use high level
     |  utility functions in gui-utils.c
   */
  expander = gtk_expander_new (_("Command Reference"));
  gtk_box_pack_start (GTK_BOX (vbox), expander, TRUE, TRUE, 2);
  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (expander), vbox1);
  gtk_widget_set_size_request (vbox1, -1, 350);

  text = ghid_scrolled_text_view (vbox1, NULL,
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  for (i = 0; i < sizeof (command_ref_text) / sizeof (gchar *); ++i)
    ghid_text_view_append (text, _(command_ref_text[i]));

  /* The command window close button.
   */
  hbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_END);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (command_window_close_cb), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

  gtk_widget_show_all (command_window);
}


/*!
 * \brief This is the command entry function called from ActionCommand()
 * when ghidgui->use_command_window is FALSE.
 *
 * The command_combo_box is already packed into the status line label
 * hbox in this case.
 */
gchar *
ghid_command_entry_get (gchar * prompt, gchar * command)
{
  gchar *s;
  gint escape_sig_id;
  GHidPort *out = &ghid_port;

  /* If this is the first user command entry, we have to create the
     |  command_combo_box and pack it into the status_line_hbox.
   */
  if (!ghidgui->command_combo_box)
    {
      command_combo_box_entry_create ();
      gtk_box_pack_start (GTK_BOX (ghidgui->status_line_hbox),
			  ghidgui->command_combo_box, FALSE, FALSE, 0);
    }

  /* Make the prompt bold and set the label before showing the combo to
     |  avoid window resizing wider.
   */
  s = g_strdup_printf ("<b>%s</b>", prompt ? prompt : "");
  ghid_status_line_set_text (s);
  g_free (s);

  /* Flag so output drawing area won't try to get focus away from us and
     |  so resetting the status line label can be blocked when resize
     |  callbacks are invokded from the resize caused by showing the combo box.
   */
  ghidgui->command_entry_status_line_active = TRUE;

  gtk_entry_set_text (ghidgui->command_entry, command ? command : "");
  gtk_widget_show_all (ghidgui->command_combo_box);

  /* Remove the top window accel group so keys intended for the entry
     |  don't get intercepted by the menu system.  Set the interface
     |  insensitive so all the user can do is enter a command, grab focus
     |  and connect a handler to look for the escape key.
   */
  ghid_remove_accel_groups (GTK_WINDOW (gport->top_window), ghidgui);
  ghid_interface_input_signals_disconnect ();
  ghid_interface_set_sensitive (FALSE);
  gtk_widget_grab_focus (GTK_WIDGET (ghidgui->command_entry));
  escape_sig_id = g_signal_connect (G_OBJECT (ghidgui->command_entry),
				    "key_press_event",
				    G_CALLBACK (command_escape_cb), NULL);

  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);

  g_main_loop_unref (loop);
  loop = NULL;

  ghidgui->command_entry_status_line_active = FALSE;

  /* Restore the damage we did before entering the loop.
   */
  g_signal_handler_disconnect (ghidgui->command_entry, escape_sig_id);
  ghid_interface_input_signals_connect ();
  ghid_interface_set_sensitive (TRUE);
  ghid_install_accel_groups (GTK_WINDOW (gport->top_window), ghidgui);

  /* Restore the status line label and give focus back to the drawing area
   */
  gtk_widget_hide (ghidgui->command_combo_box);
  gtk_widget_grab_focus (out->drawing_area);

  return command_entered;
}


void
ghid_handle_user_command (gboolean raise)
{
  char *command;
  static char *previous = NULL;

  if (ghidgui->use_command_window)
    ghid_command_window_show (raise);
  else
    {
      command = ghid_command_entry_get (_("Enter command:"),
					(Settings.SaveLastCommand && previous) ? previous : (gchar *)"");
      if (command != NULL)
	{
	  /* copy new comand line to save buffer */
	  g_free (previous);
	  previous = g_strdup (command);
	  hid_parse_command (command);
	  g_free (command);
	}
    }
  ghid_window_set_name_label (PCB->Name);
  ghid_set_status_line_label ();
}
