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
 */

/* This module, gui-utils.c, was written by Bill Wilson and the functions
 * here are Copyright (C) 2004 by Bill Wilson.  These functions are utility
 * functions which are taken from my other GPL'd projects gkrellm and
 * gstocks and are copied here for the Gtk PCB port.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gui.h"
#include <gdk/gdkkeysyms.h>

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* Not a gui function, but no better place to put it...
 */
gboolean
dup_string (gchar ** dst, const gchar * src)
{
  if ((dst == NULL) || ((*dst == NULL) && (src == NULL)))
    return FALSE;
  if (*dst)
    {
      if (src && !strcmp (*dst, src))
	return FALSE;
      g_free (*dst);
    }
  *dst = g_strdup (src);
  return TRUE;
}

void
free_glist_and_data (GList ** list_head)
{
  GList *list;

  if (*list_head == NULL)
    return;
  for (list = *list_head; list; list = list->next)
    if (list->data)
      g_free (list->data);
  g_list_free (*list_head);
  *list_head = NULL;
}


gboolean
ghid_is_modifier_key_sym (gint ksym)
{
  if (ksym == GDK_Shift_R || ksym == GDK_Shift_L
      || ksym == GDK_Control_R || ksym == GDK_Control_L)
    return TRUE;
  return FALSE;
}


ModifierKeysState
ghid_modifier_keys_state (GdkModifierType * state)
{
  GdkModifierType mask;
  ModifierKeysState mk;
  gboolean shift, control, mod1;
  GHidPort *out = &ghid_port;

  if (!state)
    gdk_window_get_pointer (gtk_widget_get_window (out->drawing_area),
                            NULL, NULL, &mask);
  else
    mask = *state;

  shift = (mask & GDK_SHIFT_MASK);
  control = (mask & GDK_CONTROL_MASK);
  mod1 = (mask & GDK_MOD1_MASK);

  if (shift && !control && !mod1)
    mk = SHIFT_PRESSED;
  else if (!shift && control && !mod1)
    mk = CONTROL_PRESSED;
  else if (!shift && !control && mod1)
    mk = MOD1_PRESSED;
  else if (shift && control && !mod1)
    mk = SHIFT_CONTROL_PRESSED;
  else if (shift && !control && mod1)
    mk = SHIFT_MOD1_PRESSED;
  else if (!shift && control && mod1)
    mk = CONTROL_MOD1_PRESSED;
  else if (shift && control && mod1)
    mk = SHIFT_CONTROL_MOD1_PRESSED;
  else
    mk = NONE_PRESSED;

  return mk;
}

ButtonState
ghid_button_state (GdkModifierType * state)
{
  GdkModifierType mask;
  ButtonState bs;
  gboolean button1, button2, button3;
  GHidPort *out = &ghid_port;

  if (!state)
    gdk_window_get_pointer (gtk_widget_get_window (out->drawing_area),
                            NULL, NULL, &mask);
  else
    mask = *state;

  button1 = (mask & GDK_BUTTON1_MASK);
  button2 = (mask & GDK_BUTTON2_MASK);
  button3 = (mask & GDK_BUTTON3_MASK);

  if (button1)
    bs = BUTTON1_PRESSED;
  else if (button2)
    bs = BUTTON2_PRESSED;
  else if (button3)
    bs = BUTTON3_PRESSED;
  else
    bs = NO_BUTTON_PRESSED;

  return bs;
}

void
ghid_draw_area_update (GHidPort * port, GdkRectangle * rect)
{
  gdk_window_invalidate_rect (gtk_widget_get_window (port->drawing_area),
                              rect, FALSE);
}


gchar *
ghid_get_color_name (GdkColor * color)
{
  gchar *name;

  if (!color)
    name = g_strdup ("#000000");
  else
    name = g_strdup_printf ("#%2.2x%2.2x%2.2x",
			    (color->red >> 8) & 0xff,
			    (color->green >> 8) & 0xff,
			    (color->blue >> 8) & 0xff);
  return name;
}

void
ghid_map_color_string (char *color_string, GdkColor * color)
{
  static GdkColormap *colormap = NULL;
  GHidPort *out = &ghid_port;

  if (!color || !out->top_window)
    return;
  if (colormap == NULL)
    colormap = gtk_widget_get_colormap (out->top_window);
  if (color->red || color->green || color->blue)
    gdk_colormap_free_colors (colormap, color, 1);
  gdk_color_parse (color_string, color);
  gdk_color_alloc (colormap, color);
}


gchar *
ghid_entry_get_text (GtkWidget * entry)
{
  gchar *s = "";

  if (entry)
    s = (gchar *) gtk_entry_get_text (GTK_ENTRY (entry));
  while (*s == ' ' || *s == '\t')
    ++s;
  return s;
}



void
ghid_check_button_connected (GtkWidget * box,
			     GtkWidget ** button,
			     gboolean active,
			     gboolean pack_start,
			     gboolean expand,
			     gboolean fill,
			     gint pad,
			     void (*cb_func) (GtkToggleButton *, gpointer),
			     gpointer data, gchar * string)
{
  GtkWidget *b;

  if (!string)
    return;
  b = gtk_check_button_new_with_mnemonic (string);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (b), active);
  if (box && pack_start)
    gtk_box_pack_start (GTK_BOX (box), b, expand, fill, pad);
  else if (box && !pack_start)
    gtk_box_pack_end (GTK_BOX (box), b, expand, fill, pad);

  if (cb_func)
    g_signal_connect (b, "clicked", G_CALLBACK (cb_func), data);
  if (button)
    *button = b;
}

void
ghid_button_connected (GtkWidget * box, GtkWidget ** button,
		       gboolean pack_start, gboolean expand, gboolean fill,
		       gint pad, void (*cb_func) (gpointer), gpointer data,
		       gchar * string)
{
  GtkWidget *b;

  if (!string)
    return;
  b = gtk_button_new_with_mnemonic (string);
  if (box && pack_start)
    gtk_box_pack_start (GTK_BOX (box), b, expand, fill, pad);
  else if (box && !pack_start)
    gtk_box_pack_end (GTK_BOX (box), b, expand, fill, pad);

  if (cb_func)
    g_signal_connect (b, "clicked", G_CALLBACK (cb_func), data);
  if (button)
    *button = b;
}

void
ghid_coord_entry (GtkWidget * box, GtkWidget ** coord_entry, Coord value,
		  Coord low, Coord high,  enum ce_step_size step_size,
		  gint width, void (*cb_func) (GHidCoordEntry *, gpointer),
		  gpointer data, gboolean right_align, gchar * string)
{
  GtkWidget *hbox = NULL, *label, *entry_widget;
  GHidCoordEntry *entry;

  if (string && box)
    {
      hbox = gtk_hbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 2);
      box = hbox;
    }

  entry_widget = ghid_coord_entry_new (low, high, value, Settings.grid_unit, step_size);
  if (coord_entry)
    *coord_entry = entry_widget;
  if (width > 0)
    gtk_widget_set_size_request (entry_widget, width, -1);
  entry = GHID_COORD_ENTRY (entry_widget);
  if (data == NULL)
    data = (gpointer) entry;
  if (cb_func)
    g_signal_connect (G_OBJECT (entry_widget), "value_changed",
		      G_CALLBACK (cb_func), data);
  if (box)
    {
      if (right_align && string)
	{
	  label = gtk_label_new (string);
	  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	  gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 2);
	}
      gtk_box_pack_start (GTK_BOX (box), entry_widget, FALSE, FALSE, 2);
      if (!right_align && string)
	{
	  label = gtk_label_new (string);
	  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	  gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 2);
	}
    }
}

void
ghid_spin_button (GtkWidget * box, GtkWidget ** spin_button, gfloat value,
		  gfloat low, gfloat high, gfloat step0, gfloat step1,
		  gint digits, gint width,
		  void (*cb_func) (GtkSpinButton *, gpointer), gpointer data, gboolean right_align,
		  gchar * string)
{
  GtkWidget *hbox = NULL, *label, *spin_but;
  GtkSpinButton *spin;
  GtkAdjustment *adj;

  if (string && box)
    {
      hbox = gtk_hbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 2);
      box = hbox;
    }
  adj = (GtkAdjustment *) gtk_adjustment_new (value,
					      low, high, step0, step1, 0.0);
  spin_but = gtk_spin_button_new (adj, 0.5, digits);
  if (spin_button)
    *spin_button = spin_but;
  if (width > 0)
    gtk_widget_set_size_request (spin_but, width, -1);
  spin = GTK_SPIN_BUTTON (spin_but);
  gtk_spin_button_set_numeric (spin, TRUE);
  if (data == NULL)
    data = (gpointer) spin;
  if (cb_func)
    g_signal_connect (G_OBJECT (spin_but), "value_changed",
		      G_CALLBACK (cb_func), data);
  if (box)
    {
      if (right_align && string)
	{
	  label = gtk_label_new (string);
	  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	  gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 2);
	}
      gtk_box_pack_start (GTK_BOX (box), spin_but, FALSE, FALSE, 2);
      if (!right_align && string)
	{
	  label = gtk_label_new (string);
	  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	  gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 2);
	}
    }
}

void
ghid_table_coord_entry (GtkWidget * table, gint row, gint column,
			GtkWidget ** coord_entry, Coord value,
			Coord low, Coord high, enum ce_step_size step_size,
			gint width, void (*cb_func) (GHidCoordEntry *, gpointer),
			gpointer data, gboolean right_align, gchar * string)
{
  GtkWidget *label, *entry_widget;
  GHidCoordEntry *entry;

  if (!table)
    return;

  entry_widget = ghid_coord_entry_new (low, high, value, Settings.grid_unit, step_size);
  if (coord_entry)
    *coord_entry = entry_widget;
  if (width > 0)
    gtk_widget_set_size_request (entry_widget, width, -1);
  entry = GHID_COORD_ENTRY (entry_widget);
  if (data == NULL)
    data = (gpointer) entry;
  if (cb_func)
    g_signal_connect (G_OBJECT (entry), "value_changed",
		      G_CALLBACK (cb_func), data);

  if (right_align)
    {
      gtk_table_attach_defaults (GTK_TABLE (table), entry_widget,
				 column + 1, column + 2, row, row + 1);
      if (string)
	{
	  label = gtk_label_new (string);
	  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	  gtk_table_attach_defaults (GTK_TABLE (table), label,
				     column, column + 1, row, row + 1);
	}
    }
  else
    {
      gtk_table_attach_defaults (GTK_TABLE (table), entry_widget,
				 column, column + 1, row, row + 1);
      if (string)
	{
	  label = gtk_label_new (string);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	  gtk_table_attach_defaults (GTK_TABLE (table), label,
				     column + 1, column + 2, row, row + 1);
	}
    }
}


void
ghid_table_spin_button (GtkWidget * table, gint row, gint column,
			GtkWidget ** spin_button, gfloat value,
			gfloat low, gfloat high, gfloat step0, gfloat step1,
			gint digits, gint width,
			void (*cb_func) (GtkSpinButton *, gpointer), gpointer data,
			gboolean right_align, gchar * string)
{
  GtkWidget *label, *spin_but;
  GtkSpinButton *spin;
  GtkAdjustment *adj;

  if (!table)
    return;

  adj = (GtkAdjustment *) gtk_adjustment_new (value,
                                             low, high, step0, step1, 0.0);
  spin_but = gtk_spin_button_new (adj, 0.5, digits);

  if (spin_button)
    *spin_button = spin_but;
  if (width > 0)
    gtk_widget_set_size_request (spin_but, width, -1);
  spin = GTK_SPIN_BUTTON (spin_but);
  gtk_spin_button_set_numeric (spin, TRUE);
  if (data == NULL)
    data = (gpointer) spin;
  if (cb_func)
    g_signal_connect (G_OBJECT (spin_but), "value_changed",
		      G_CALLBACK (cb_func), data);

  if (right_align)
    {
      gtk_table_attach_defaults (GTK_TABLE (table), spin_but,
				 column + 1, column + 2, row, row + 1);
      if (string)
	{
	  label = gtk_label_new (string);
	  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	  gtk_table_attach_defaults (GTK_TABLE (table), label,
				     column, column + 1, row, row + 1);
	}
    }
  else
    {
      gtk_table_attach_defaults (GTK_TABLE (table), spin_but,
				 column, column + 1, row, row + 1);
      if (string)
	{
	  label = gtk_label_new (string);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	  gtk_table_attach_defaults (GTK_TABLE (table), label,
				     column + 1, column + 2, row, row + 1);
	}
    }
}

void
ghid_range_control (GtkWidget * box, GtkWidget ** scale_res,
		    gboolean horizontal, GtkPositionType pos,
		    gboolean set_draw_value, gint digits, gboolean pack_start,
		    gboolean expand, gboolean fill, guint pad, gfloat value,
		    gfloat low, gfloat high, gfloat step0, gfloat step1,
		    void (*cb_func) (), gpointer data)
{
  GtkWidget *scale;
  GtkAdjustment *adj;

  adj = (GtkAdjustment *) gtk_adjustment_new (value,
					      low, high, step0, step1, 0.0);

  if (horizontal)
    scale = gtk_hscale_new (GTK_ADJUSTMENT (adj));
  else
    scale = gtk_vscale_new (GTK_ADJUSTMENT (adj));
  gtk_scale_set_value_pos (GTK_SCALE (scale), pos);
  gtk_scale_set_draw_value (GTK_SCALE (scale), set_draw_value);
  gtk_scale_set_digits (GTK_SCALE (scale), digits);

  /* Increments don't make sense, use -1,1 because that does closest to
     |  what I want: scroll down decrements slider value.
   */
  gtk_range_set_increments (GTK_RANGE (scale), -1, 1);

  if (pack_start)
    gtk_box_pack_start (GTK_BOX (box), scale, expand, fill, pad);
  else
    gtk_box_pack_end (GTK_BOX (box), scale, expand, fill, pad);

  if (data == NULL)
    data = (gpointer) adj;
  if (cb_func)
    g_signal_connect (G_OBJECT (adj), "value_changed",
		      G_CALLBACK (cb_func), data);
  if (scale_res)
    *scale_res = scale;
}

GtkWidget *
ghid_scrolled_vbox (GtkWidget * box, GtkWidget ** scr,
		    GtkPolicyType h_policy, GtkPolicyType v_policy)
{
  GtkWidget *scrolled, *vbox;

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  h_policy, v_policy);
  gtk_box_pack_start (GTK_BOX (box), scrolled, TRUE, TRUE, 0);
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled),
					 vbox);
  if (scr)
    *scr = scrolled;
  return vbox;
}

/* frame_border_width - border around outside of frame.
   |  vbox_pad - pad between widgets to be packed in returned vbox.
   |  vbox_border_width - border between returned vbox and frame.
*/
GtkWidget *
ghid_framed_vbox (GtkWidget * box, gchar * label, gint frame_border_width,
		  gboolean frame_expand, gint vbox_pad,
		  gint vbox_border_width)
{
  GtkWidget *frame;
  GtkWidget *vbox;

  frame = gtk_frame_new (label);
  gtk_container_set_border_width (GTK_CONTAINER (frame), frame_border_width);
  gtk_box_pack_start (GTK_BOX (box), frame, frame_expand, frame_expand, 0);
  vbox = gtk_vbox_new (FALSE, vbox_pad);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), vbox_border_width);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  return vbox;
}

GtkWidget *
ghid_framed_vbox_end (GtkWidget * box, gchar * label, gint frame_border_width,
		      gboolean frame_expand, gint vbox_pad,
		      gint vbox_border_width)
{
  GtkWidget *frame;
  GtkWidget *vbox;

  frame = gtk_frame_new (label);
  gtk_container_set_border_width (GTK_CONTAINER (frame), frame_border_width);
  gtk_box_pack_end (GTK_BOX (box), frame, frame_expand, frame_expand, 0);
  vbox = gtk_vbox_new (FALSE, vbox_pad);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), vbox_border_width);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  return vbox;
}

GtkWidget *
ghid_category_vbox (GtkWidget * box, const gchar * category_header,
		    gint header_pad,
		    gint box_pad, gboolean pack_start, gboolean bottom_pad)
{
  GtkWidget *vbox, *vbox1, *hbox, *label;
  gchar *s;

  vbox = gtk_vbox_new (FALSE, 0);
  if (pack_start)
    gtk_box_pack_start (GTK_BOX (box), vbox, FALSE, FALSE, 0);
  else
    gtk_box_pack_end (GTK_BOX (box), vbox, FALSE, FALSE, 0);

  if (category_header)
    {
      label = gtk_label_new (NULL);
      s = g_strconcat ("<span weight=\"bold\">", category_header,
		       "</span>", NULL);
      gtk_label_set_markup (GTK_LABEL (label), s);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, header_pad);
      g_free (s);
    }

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new ("     ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  vbox1 = gtk_vbox_new (FALSE, box_pad);
  gtk_box_pack_start (GTK_BOX (hbox), vbox1, TRUE, TRUE, 0);

  if (bottom_pad)
    {
      label = gtk_label_new ("");
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    }
  return vbox1;
}

GtkTreeSelection *
ghid_scrolled_selection (GtkTreeView * treeview, GtkWidget * box,
			 GtkSelectionMode s_mode,
			 GtkPolicyType h_policy, GtkPolicyType v_policy,
			 void (*func_cb) (GtkTreeSelection *, gpointer), gpointer data)
{
  GtkTreeSelection *selection;
  GtkWidget *scrolled;

  if (!box || !treeview)
    return NULL;

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (box), scrolled, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (scrolled), GTK_WIDGET (treeview));
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  h_policy, v_policy);
  selection = gtk_tree_view_get_selection (treeview);
  gtk_tree_selection_set_mode (selection, s_mode);
  if (func_cb)
    g_signal_connect (G_OBJECT (selection), "changed",
		      G_CALLBACK (func_cb), data);
  return selection;
}

GtkWidget *
ghid_notebook_page (GtkWidget * tabs, const char *name, gint pad, gint border)
{
  GtkWidget *label;
  GtkWidget *vbox;

  vbox = gtk_vbox_new (FALSE, pad);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), border);

  label = gtk_label_new (name);
  gtk_notebook_append_page (GTK_NOTEBOOK (tabs), vbox, label);

  return vbox;
}

GtkWidget *
ghid_framed_notebook_page (GtkWidget * tabs, char *name, gint border,
			   gint frame_border, gint vbox_pad, gint vbox_border)
{
  GtkWidget *vbox;

  vbox = ghid_notebook_page (tabs, name, 0, border);
  vbox = ghid_framed_vbox (vbox, NULL, frame_border, TRUE,
			   vbox_pad, vbox_border);
  return vbox;
}

void
ghid_dialog_report (gchar * title, gchar * message)
{
  GtkWidget *top_win;
  GtkWidget *dialog;
  GtkWidget *content_area;
  GtkWidget *scrolled;
  GtkWidget *vbox, *vbox1;
  GtkWidget *label;
  gchar *s;
  gint nlines;
  GHidPort *out = &ghid_port;

  if (!message)
    return;
  top_win = out->top_window;
  dialog = gtk_dialog_new_with_buttons (title ? title : "PCB",
					GTK_WINDOW (top_win),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_OK, GTK_RESPONSE_NONE,
					NULL);
  g_signal_connect_swapped (GTK_OBJECT (dialog), "response",
			    G_CALLBACK (gtk_widget_destroy),
			    GTK_OBJECT (dialog));
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "PCB_Dialog", "PCB");

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
  gtk_box_pack_start (GTK_BOX (content_area), vbox, FALSE, FALSE, 0);

  label = gtk_label_new (message);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  for (nlines = 0, s = message; *s; ++s)
    if (*s == '\n')
      ++nlines;
  if (nlines > 20)
    {
      vbox1 = ghid_scrolled_vbox (vbox, &scrolled,
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
      gtk_widget_set_size_request (scrolled, -1, 300);
      gtk_box_pack_start (GTK_BOX (vbox1), label, FALSE, FALSE, 0);
    }
  else
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  gtk_widget_show_all (dialog);
}


void
ghid_label_set_markup (GtkWidget * label, const gchar * text)
{
  if (label)
    gtk_label_set_markup (GTK_LABEL (label), text ? text : "");
}


static void
text_view_append (GtkWidget * view, gchar * s)
{
  GtkTextIter iter;
  GtkTextBuffer *buffer;
  GtkTextMark *mark;

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
  gtk_text_buffer_get_end_iter (buffer, &iter);
  /* gtk_text_iter_forward_to_end(&iter); */

  if (strncmp (s, "<b>", 3) == 0)
    gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					      s + 3, -1, "bold", NULL);
  else if (strncmp (s, "<i>", 3) == 0)
    gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					      s + 3, -1, "italic", NULL);
  else if (strncmp (s, "<h>", 3) == 0)
    gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					      s + 3, -1, "heading", NULL);
  else if (strncmp (s, "<c>", 3) == 0)
    gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					      s + 3, -1, "center", NULL);
  else if (strncmp (s, "<ul>", 4) == 0)
    gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					      s + 4, -1, "underline", NULL);
  else
    gtk_text_buffer_insert (buffer, &iter, s, -1);

  mark = gtk_text_buffer_create_mark (buffer, NULL, &iter, FALSE);
  gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view), mark,
				0, TRUE, 0.0, 1.0);
  gtk_text_buffer_delete_mark (buffer, mark);
}

void
ghid_text_view_append (GtkWidget * view, gchar * string)
{
  static gchar *tag;
  gchar *s;

  s = string;
  if (*s == '<'
      && ((*(s + 2) == '>' && !*(s + 3)) || (*(s + 3) == '>' && !*(s + 4))))
    {
      tag = g_strdup (s);
      return;
    }

  if (tag)
    {
      s = g_strconcat (tag, string, NULL);
      text_view_append (view, s);
      g_free (s);
      g_free (tag);
      tag = NULL;
    }
  else
    text_view_append (view, string);
}

void
ghid_text_view_append_strings (GtkWidget * view, gchar ** string,
			       gint n_strings)
{
  gchar *tag = NULL;
  gchar *s, *t;
  gint i;

  for (i = 0; i < n_strings; ++i)
    {
      s = string[i];
      if (*s == '<'
	  && ((*(s + 2) == '>' && !*(s + 3))
	      || (*(s + 3) == '>' && !*(s + 4))))
	{
	  tag = g_strdup (s);
	  continue;
	}
      s = _(string[i]);
      if (tag)
	{
	  t = g_strconcat (tag, s, NULL);
	  text_view_append (view, t);
	  g_free (t);
	  g_free (tag);
	  tag = NULL;
	}
      else
	text_view_append (view, s);
    }
}


GtkWidget *
ghid_scrolled_text_view (GtkWidget * box,
			 GtkWidget ** scr,
			 GtkPolicyType h_policy, GtkPolicyType v_policy)
{
  GtkWidget *scrolled, *view;
  GtkTextBuffer *buffer;

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  h_policy, v_policy);
  gtk_box_pack_start (GTK_BOX (box), scrolled, TRUE, TRUE, 0);

  view = gtk_text_view_new ();
  gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
  gtk_text_buffer_create_tag (buffer, "heading",
			      "weight", PANGO_WEIGHT_BOLD,
			      "size", 14 * PANGO_SCALE, NULL);
  gtk_text_buffer_create_tag (buffer, "italic",
			      "style", PANGO_STYLE_ITALIC, NULL);
  gtk_text_buffer_create_tag (buffer, "bold",
			      "weight", PANGO_WEIGHT_BOLD, NULL);
  gtk_text_buffer_create_tag (buffer, "center",
			      "justification", GTK_JUSTIFY_CENTER, NULL);
  gtk_text_buffer_create_tag (buffer, "underline",
			      "underline", PANGO_UNDERLINE_SINGLE, NULL);

  gtk_container_add (GTK_CONTAINER (scrolled), view);
  if (scr)
    *scr = scrolled;
  return view;
}
