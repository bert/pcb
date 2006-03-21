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

/* This file written by Bill Wilson for the PCB Gtk port. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "gui.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID("$Id$");

static GtkWidget *export_dialog = NULL;

static void
set_flag_cb (GtkToggleButton *button, gboolean *flag)
{
  *flag = gtk_toggle_button_get_active (button);
}


static void
intspinner_changed_cb (GtkWidget *spin_button, gpointer data)
{
  int *ival = data;

  *ival = gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_button));
}

static void
dblspinner_changed_cb (GtkWidget *spin_button, gpointer data)
{
  double *dval = data;

  *dval = gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_button));
}


static void
entry_changed_cb (GtkEntry *entry, char **str)
{
  const gchar *s;

  s = gtk_entry_get_text (entry);

  if (*str)
    free (*str);
  *str = strdup (s);
}

static void
enum_changed_cb (GtkWidget *combo_box, int *val)
{
	gint active;

	active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo_box));
	*val = active;
}

void
ghid_dialog_print (HID *printer)
{
  GtkWidget *dialog, *main_vbox, *vbox, *vbox1, *hbox, *entry;
  GtkWidget *combo;
  GtkWidget *widget;
  GHidPort *out = &ghid_port;
  HID_Attribute *attr;
  int i, j, n;
  GtkTooltips *tips;
  HID_Attr_Val *vals;

  if (printer == NULL)
    {
      fprintf(stderr, "ERROR:  %s:  null printer\n", __FUNCTION__);
      return ;
    }

  tips = gtk_tooltips_new();

  if (!PCB->PrintFilename)
    PCB->PrintFilename = EvaluateFilename (Settings.PrintFile,
					   NULL, 
					   PCB->Filename ? PCB->Filename : "noname", 
					   NULL);

  dialog = gtk_dialog_new_with_buttons(_("PCB Print Layout"),
				       GTK_WINDOW(out->top_window),
				       GTK_DIALOG_MODAL 
				       | GTK_DIALOG_DESTROY_WITH_PARENT,
				       GTK_STOCK_CANCEL, GTK_RESPONSE_NONE,
				       GTK_STOCK_OK, GTK_RESPONSE_OK,
				       NULL);
  gtk_window_set_wmclass (GTK_WINDOW (dialog),
			  "PCB_Print", "PCB");

  main_vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);


  vbox = ghid_category_vbox(main_vbox, printer->description, 
			   4, 2, TRUE, TRUE);
  /* 
   * Iterate over all the export options and build up a dialog box
   * that lets us control all of the options.  By doing things this
   * way, any changes to the exporter HID's automatically are
   * reflected in this dialog box.
   */
  attr = printer->get_export_options (&n);
  for (j=0; j<n; j++)
    {
      switch (attr[j].type) 
	{
	case HID_Label:
	  widget = gtk_label_new (attr[j].name);
	  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
	  gtk_tooltips_set_tip (tips, widget, attr[j].help_text, NULL);
	  break;
	  
	case HID_Integer:
	  hbox = gtk_hbox_new (FALSE, 4);
	  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	  
	  /* 
	   * FIXME 
	   * need to pick the "digits" argument based on min/max
	   * values
	   */
	  ghid_spin_button (hbox, &widget, attr[j].default_val.int_value,
			   attr[j].min_val, attr[j].max_val, 1.0, 1.0, 0, 0,
			   intspinner_changed_cb, 
			   &(attr[j].default_val.int_value), 
			   FALSE, NULL);

	  gtk_tooltips_set_tip (tips, widget, 
				attr[j].help_text, NULL);

	  widget = gtk_label_new (attr[j].name);
	  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	  break;
	  
	case HID_Real:
	  hbox = gtk_hbox_new (FALSE, 4);
	  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	  
	  /* 
	   * FIXME 
	   * need to pick the "digits" and step size argument more
	   * intelligently
	   */
	  ghid_spin_button (hbox, &widget, attr[j].default_val.real_value, 
			   attr[j].min_val, attr[j].max_val, 0.01, 0.01, 3, 0,
			   dblspinner_changed_cb, 
			   &(attr[j].default_val.real_value), 
			   FALSE, NULL);

	  gtk_tooltips_set_tip (tips, widget,
				attr[j].help_text, NULL);

	  widget = gtk_label_new (attr[j].name);
	  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	  break;
	  
	case HID_String:
	  hbox = gtk_hbox_new (FALSE, 4);
	  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	  
	  entry = gtk_entry_new ();
	  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
	  gtk_entry_set_text (GTK_ENTRY (entry), attr[j].default_val.str_value);	  gtk_tooltips_set_tip (tips, entry, attr[j].help_text, NULL);
	  g_signal_connect (G_OBJECT (entry), "changed",
			    G_CALLBACK (entry_changed_cb),
			    &(attr[j].default_val.str_value));
	  
	  widget = gtk_label_new (attr[j].name);
	  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	  break;
	  
	case HID_Boolean:
	  /* put this in a check button */
	  ghid_check_button_connected(vbox, &widget, 
				     attr[j].default_val.int_value,
				     TRUE, FALSE, FALSE, 0, set_flag_cb, 
				     &(attr[j].default_val.int_value), 
				     attr[j].name);
	  gtk_tooltips_set_tip (tips, widget, attr[j].help_text, NULL);
	  break;
	  
	case HID_Enum:
	  hbox = gtk_hbox_new (FALSE, 4);
	  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	  /* 
	   * We have to put the combo_box inside of an event_box in
	   * order for tooltips to work.
	   */
	  widget = gtk_event_box_new ();
	  gtk_tooltips_set_tip (tips, widget, attr[j].help_text, NULL);
	  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

	  combo = gtk_combo_box_new_text ();
	  gtk_container_add (GTK_CONTAINER (widget), combo);
	  g_signal_connect (G_OBJECT (combo), "changed",
			    G_CALLBACK (enum_changed_cb),
			    &(attr[j].default_val.int_value));
	  

	  /* 
	   * Iterate through each value and add them to the
	   * combo box
	   */
	  i = 0;
	  while (attr[j].enumerations[i])
	    {
	      gtk_combo_box_append_text (GTK_COMBO_BOX (combo),
					 attr[j].enumerations[i]);
	      i++;
	    }
	  gtk_combo_box_set_active ( GTK_COMBO_BOX (combo),
				     attr[j].default_val.int_value);
 	  widget = gtk_label_new (attr[j].name);
	  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	  break;
	  
	case HID_Mixed:
	  printf ("HID_Mixed\n");
	  break;
	  
	case HID_Path:
	  vbox1 = ghid_category_vbox (vbox, attr[j].name, 4, 2, TRUE, TRUE);
	  entry = gtk_entry_new ();
	  gtk_box_pack_start (GTK_BOX (vbox1), entry, FALSE, FALSE, 0);
	  gtk_entry_set_text (GTK_ENTRY (entry), attr[j].default_val.str_value);
	  g_signal_connect (G_OBJECT (entry), "changed",
			    G_CALLBACK (entry_changed_cb),
			    &(attr[j].default_val.str_value));
	  
	  gtk_tooltips_set_tip (tips, entry, attr[j].help_text, NULL);
	  break;
	  
	default:
	  printf ("%s: unknown type of HID attribute\n", __FUNCTION__);
	  break;
	}
    }


  gtk_widget_show_all (dialog);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
    {

      /* 
       * we have put our attributes into an HID_Attr_Val array
       * to pass to do_export() otherwise the file name gets
       * reset by the PS HID to the default value based on
       * the board name.  Perhaps that behaviour should
       * be changed?
       */
      vals = (HID_Attr_Val *) calloc (n, sizeof (HID_Attr_Val));
      for (i=0; i<n; i++)
	{
	  vals[i] = attr[i].default_val;
	  if (vals[i].str_value)
	    vals[i].str_value = strdup(vals[i].str_value);
	}

      printer->do_export (vals);

      for (i=0; i<n; i++)
	{
	  if (vals[i].str_value)
	    free (vals[i].str_value);
	}
      free (vals);
    }
  
  gtk_widget_destroy(dialog);
}


static void
exporter_clicked_cb (GtkButton *button, HID *exporter)
{
  if (export_dialog != NULL)
    gtk_widget_destroy (export_dialog);
  export_dialog = NULL;
  ghid_dialog_print (exporter);
}

void
ghid_dialog_export (void)
{
  GtkWidget *vbox, *button;
  GHidPort *out = &ghid_port;
  int i;
  HID **hids;
  HID *exporter = NULL;
  GtkTooltips *tips;

  tips = gtk_tooltips_new();

  export_dialog = gtk_dialog_new_with_buttons (_("PCB Export Layout"),
					       GTK_WINDOW (out->top_window),
					       GTK_DIALOG_MODAL 
					       | GTK_DIALOG_DESTROY_WITH_PARENT,
					       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					       NULL);
  gtk_window_set_wmclass (GTK_WINDOW (export_dialog),
			  "PCB_Export", "PCB");
  
  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (export_dialog)->vbox), vbox);

  /* 
   * Iterate over all the export HID's and build up a dialog box that
   * lets us choose which one we want to use.
   * This way, any additions to the exporter HID's automatically are
   * reflected in this dialog box.
   */

  hids = hid_enumerate ();
  for (i=0; hids[i]; i++)
    {
      if (hids[i]->exporter) 
	{
	  exporter = hids[i];
	  button = gtk_button_new_with_label (hids[i]->name);
	  gtk_tooltips_set_tip (tips, button, hids[i]->description, NULL);
	  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	  g_signal_connect (G_OBJECT (button), "clicked",
			    G_CALLBACK (exporter_clicked_cb), hids[i]);
	}
    }

  if (exporter == NULL)
    {
      gui->log (_("Can't find a suitable exporter HID"));
    }

  gtk_widget_show_all (export_dialog);
  gtk_dialog_run (GTK_DIALOG (export_dialog));
  if (export_dialog != NULL)
    gtk_widget_destroy (export_dialog);
  export_dialog = NULL;
}

