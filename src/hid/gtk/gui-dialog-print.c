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

/* This file written by Bill Wilson for the PCB Gtk port. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "gui.h"
#include "pcb-printf.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

static GtkWidget *export_dialog = NULL;

static void
set_flag_cb (GtkToggleButton * button, void * flag)
{
  *(gboolean *)flag = gtk_toggle_button_get_active (button);
}


static void
intspinner_changed_cb (GtkSpinButton * spin_button, gpointer data)
{
  int *ival = (int *)data;

  *ival = gtk_spin_button_get_value (GTK_SPIN_BUTTON ((GtkWidget *)spin_button));
}

static void
coordentry_changed_cb (GtkEntry * entry, Coord * res)
{
  const gchar *s = gtk_entry_get_text (entry);
  *res = GetValue (s, NULL, NULL);
}

static void
dblspinner_changed_cb (GtkSpinButton * spin_button, gpointer data)
{
  double *dval = (double *)data;

  *dval = gtk_spin_button_get_value (GTK_SPIN_BUTTON ((GtkWidget *)spin_button));
}


static void
entry_changed_cb (GtkEntry * entry, char **str)
{
  const gchar *s;

  s = gtk_entry_get_text (entry);

  if (*str)
    free (*str);
  *str = strdup (s);
}

static void
enum_changed_cb (GtkWidget * combo_box, int *val)
{
  gint active;

  active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo_box));
  *val = active;
}


int
ghid_attribute_dialog (HID_Attribute * attrs,
		       int n_attrs, HID_Attr_Val * results,
		       const char * title,
		       const char * descr)
{
  GtkWidget *dialog;
  GtkWidget *content_area;
  GtkWidget *main_vbox, *vbox, *vbox1, *hbox, *entry;
  GtkWidget *combo;
  GtkWidget *widget;
  GHidPort *out = &ghid_port;
  int i, j, n;
  int rc = 0;

  dialog = gtk_dialog_new_with_buttons (_(title),
					GTK_WINDOW (out->top_window),
					(GtkDialogFlags)(GTK_DIALOG_MODAL
							 | GTK_DIALOG_DESTROY_WITH_PARENT),
					GTK_STOCK_CANCEL, GTK_RESPONSE_NONE,
					GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "PCB_attribute_editor", "PCB");

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  main_vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_container_add (GTK_CONTAINER (content_area), main_vbox);

  vbox = ghid_category_vbox (main_vbox, descr != NULL ? descr : "",
			     4, 2, TRUE, TRUE);

  /* 
   * Iterate over all the export options and build up a dialog box
   * that lets us control all of the options.  By doing things this
   * way, any changes to the exporter HID's automatically are
   * reflected in this dialog box.
   */
  for (j = 0; j < n_attrs; j++)
    {
      const Unit *unit_list;
      if (attrs[j].help_text == ATTR_UNDOCUMENTED)
	continue;
      switch (attrs[j].type)
	{
	case HID_Label:
	  widget = gtk_label_new (_(attrs[j].name));
	  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
	  gtk_widget_set_tooltip_text (widget, _(attrs[j].help_text));
	  break;

	case HID_Integer:
	  hbox = gtk_hbox_new (FALSE, 4);
	  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	  /* 
	   * FIXME 
	   * need to pick the "digits" argument based on min/max
	   * values
	   */
	  ghid_spin_button (hbox, &widget, attrs[j].default_val.int_value,
			    attrs[j].min_val, attrs[j].max_val, 1.0, 1.0, 0, 0,
			    intspinner_changed_cb,
			    &(attrs[j].default_val.int_value), FALSE, NULL);
	  gtk_widget_set_tooltip_text (widget, _(attrs[j].help_text));

	  widget = gtk_label_new (_(attrs[j].name));
	  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	  break;

	case HID_Coord:
	  hbox = gtk_hbox_new (FALSE, 4);
	  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	  entry = ghid_coord_entry_new (attrs[j].min_val, attrs[j].max_val,
	                                attrs[j].default_val.coord_value,
	                                Settings.grid_unit, CE_SMALL);
	  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
	  if(attrs[j].default_val.str_value != NULL)
	    gtk_entry_set_text (GTK_ENTRY (entry),
				attrs[j].default_val.str_value);
	  gtk_widget_set_tooltip_text (entry, _(attrs[j].help_text));
	  g_signal_connect (G_OBJECT (entry), "changed",
			    G_CALLBACK (coordentry_changed_cb),
			    &(attrs[j].default_val.coord_value));

	  widget = gtk_label_new (_(attrs[j].name));
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
	  ghid_spin_button (hbox, &widget, attrs[j].default_val.real_value,
			    attrs[j].min_val, attrs[j].max_val, 0.01, 0.01, 3,
			    0, 
			    dblspinner_changed_cb,
			    &(attrs[j].default_val.real_value), FALSE, NULL);

	  gtk_widget_set_tooltip_text (widget, _(attrs[j].help_text));

	  widget = gtk_label_new (_(attrs[j].name));
	  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	  break;

	case HID_String:
	  hbox = gtk_hbox_new (FALSE, 4);
	  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	  entry = gtk_entry_new ();
	  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
	  if(attrs[j].default_val.str_value != NULL)
	    gtk_entry_set_text (GTK_ENTRY (entry),
				attrs[j].default_val.str_value);
	  gtk_widget_set_tooltip_text (entry, _(attrs[j].help_text));
	  g_signal_connect (G_OBJECT (entry), "changed",
			    G_CALLBACK (entry_changed_cb),
			    &(attrs[j].default_val.str_value));

	  widget = gtk_label_new (_(attrs[j].name));
	  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	  break;

	case HID_Boolean:
	  /* put this in a check button */
	  ghid_check_button_connected (vbox, &widget,
				       attrs[j].default_val.int_value,
				       TRUE, FALSE, FALSE, 0, set_flag_cb,
				       &(attrs[j].default_val.int_value),
				       _(attrs[j].name));
	  gtk_widget_set_tooltip_text (widget, _(attrs[j].help_text));
	  break;

	case HID_Enum:
	  hbox = gtk_hbox_new (FALSE, 4);
	  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

        do_enum:
	  combo = gtk_combo_box_new_text ();
	  gtk_widget_set_tooltip_text (combo, _(attrs[j].help_text));
	  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 0);
	  g_signal_connect (G_OBJECT (combo), "changed",
			    G_CALLBACK (enum_changed_cb),
			    &(attrs[j].default_val.int_value));


	  /* 
	   * Iterate through each value and add them to the
	   * combo box
	   */
	  i = 0;
	  while (attrs[j].enumerations[i])
	    {
	      gtk_combo_box_append_text (GTK_COMBO_BOX (combo),
					 attrs[j].enumerations[i]);
	      i++;
	    }
	  gtk_combo_box_set_active (GTK_COMBO_BOX (combo),
				    attrs[j].default_val.int_value);
	  widget = gtk_label_new (_(attrs[j].name));
	  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	  break;

	case HID_Mixed:
	  hbox = gtk_hbox_new (FALSE, 4);
	  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
          
	  /*
	   * FIXME
	   * need to pick the "digits" and step size argument more
	   * intelligently
	   */
	  ghid_spin_button (hbox, &widget, attrs[j].default_val.real_value,
			    attrs[j].min_val, attrs[j].max_val, 0.01, 0.01, 3,
			    0,
			    dblspinner_changed_cb,
			    &(attrs[j].default_val.real_value), FALSE, NULL);
	  gtk_widget_set_tooltip_text (widget, _(attrs[j].help_text));

          goto do_enum;
	  break;

	case HID_Path:
	  vbox1 = ghid_category_vbox (vbox, _(attrs[j].name), 4, 2, TRUE, TRUE);
	  entry = gtk_entry_new ();
	  gtk_box_pack_start (GTK_BOX (vbox1), entry, FALSE, FALSE, 0);
	  gtk_entry_set_text (GTK_ENTRY (entry),
			      attrs[j].default_val.str_value);
	  g_signal_connect (G_OBJECT (entry), "changed",
			    G_CALLBACK (entry_changed_cb),
			    &(attrs[j].default_val.str_value));

	  gtk_widget_set_tooltip_text (entry, _(attrs[j].help_text));
	  break;

	case HID_Unit:
          unit_list = get_unit_list ();
          n = get_n_units ();

	  hbox = gtk_hbox_new (FALSE, 4);
	  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	  combo = gtk_combo_box_new_text ();
	  gtk_widget_set_tooltip_text (combo, _(attrs[j].help_text));
	  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 0);
	  g_signal_connect (G_OBJECT (combo), "changed",
			    G_CALLBACK (enum_changed_cb),
			    &(attrs[j].default_val.int_value));

	  /* 
	   * Iterate through each value and add them to the
	   * combo box
	   */
	  for (i = 0; i < n; ++i)
	    gtk_combo_box_append_text (GTK_COMBO_BOX (combo),
					unit_list[i].in_suffix);
	  gtk_combo_box_set_active (GTK_COMBO_BOX (combo),
				    attrs[j].default_val.int_value);
	  widget = gtk_label_new (_(attrs[j].name));
	  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
          break;
	default:
	  printf (_("%s: unknown type of HID attribute\n"), __FUNCTION__);
	  break;
	}
    }


  gtk_widget_show_all (dialog);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      /* copy over the results */
      for (i = 0; i < n_attrs; i++)
	{
	  results[i] = attrs[i].default_val;
	  if (results[i].str_value)
	    results[i].str_value = strdup (results[i].str_value);
	}
      rc = 0;
    }
  else
    rc = 1;

  gtk_widget_destroy (dialog);

  return rc;
}



static void
exporter_clicked_cb (GtkButton * button, HID * exporter)
{
  ghid_dialog_print (exporter);
}

void
ghid_dialog_print (HID *hid)
{
  HID_Attribute *attr;
  int n = 0;
  int i;
  HID_Attr_Val * results = NULL;

  /* signal the initial export select dialog that it should close */
  if (export_dialog)
    gtk_dialog_response (GTK_DIALOG (export_dialog), GTK_RESPONSE_CANCEL);

  exporter = hid;

  attr = exporter->get_export_options (&n);
  if (n > 0)
    {
      results = (HID_Attr_Val *) malloc (n * sizeof (HID_Attr_Val));
      if (results == NULL)
	{
	  fprintf (stderr, _("%s() -- malloc failed\n"), __FUNCTION__);
	  exit (1);
	}
      
      /* non-zero means cancel was picked */
      if (ghid_attribute_dialog (attr, n, results,  _("PCB Print Layout"), 
				 exporter->description))
	return;
	  
    }

  exporter->do_export (results);
  
  for (i = 0; i < n; i++)
    {
      if (results[i].str_value)
	free ((void *) results[i].str_value);
    }

  if (results)
    free (results);

  exporter = NULL;
}

void
ghid_dialog_export (void)
{
  GtkWidget *content_area;
  GtkWidget *vbox, *button;
  GHidPort *out = &ghid_port;
  int i;
  HID **hids;
  gboolean no_exporter = TRUE;

  export_dialog = gtk_dialog_new_with_buttons (_("PCB Export Layout"),
					       GTK_WINDOW (out->top_window),
					       (GtkDialogFlags)(GTK_DIALOG_MODAL
					       |
								GTK_DIALOG_DESTROY_WITH_PARENT),
					       GTK_STOCK_CANCEL,
					       GTK_RESPONSE_CANCEL, NULL);
  gtk_window_set_wmclass (GTK_WINDOW (export_dialog), "PCB_Export", "PCB");

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (export_dialog));

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (content_area), vbox);

  /* 
   * Iterate over all the export HID's and build up a dialog box that
   * lets us choose which one we want to use.
   * This way, any additions to the exporter HID's automatically are
   * reflected in this dialog box.
   */

  hids = hid_enumerate ();
  for (i = 0; hids[i]; i++)
    {
      if (hids[i]->exporter)
	{
	  no_exporter = FALSE;
	  button = gtk_button_new_with_label (_(hids[i]->name));
	  gtk_widget_set_tooltip_text (button, hids[i]->description);
	  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	  g_signal_connect (G_OBJECT (button), "clicked",
			    G_CALLBACK (exporter_clicked_cb), hids[i]);
	}
    }

  if (no_exporter)
    {
      gui->log (_("Can't find a suitable exporter HID"));
    }

  gtk_widget_show_all (export_dialog);
  gtk_dialog_run (GTK_DIALOG (export_dialog));

  if (export_dialog != NULL)
    gtk_widget_destroy (export_dialog);
  export_dialog = NULL;
}
