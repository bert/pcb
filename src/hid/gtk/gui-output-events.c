/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
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

/* This file written by Bill Wilson for the PCB Gtk port */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gui.h"
#include "gtkhid.h"

#include <gdk/gdkkeysyms.h>

#include "action.h"
#include "crosshair.h"
#include "draw.h"
#include "error.h"
#include "output.h"
#include "misc.h"
#include "set.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

static gint x_pan_speed, y_pan_speed;

void
ghid_port_ranges_changed (void)
{
  GtkAdjustment *h_adj, *v_adj;

  if (!ghidgui->combine_adjustments)
    HideCrosshair (FALSE);
  if (ghidgui->combine_adjustments)
    {
      ghidgui->combine_adjustments = FALSE;
      return;
    }

  ghidgui->need_restore_crosshair = TRUE;

  h_adj = gtk_range_get_adjustment (GTK_RANGE (ghidgui->h_range));
  v_adj = gtk_range_get_adjustment (GTK_RANGE (ghidgui->v_range));
  gport->view_x0 = h_adj->value;
  gport->view_y0 = v_adj->value;

  ghid_invalidate_all ();
}

gboolean
ghid_port_ranges_pan (gdouble x, gdouble y, gboolean relative)
{
  GtkAdjustment *h_adj, *v_adj;
  gdouble x0, y0, x1, y1;

  h_adj = gtk_range_get_adjustment (GTK_RANGE (ghidgui->h_range));
  v_adj = gtk_range_get_adjustment (GTK_RANGE (ghidgui->v_range));
  x0 = h_adj->value;
  y0 = v_adj->value;

  if (relative)
    {
      x1 = x0 + x;
      y1 = y0 + y;
    }
  else
    {
      x1 = x;
      y1 = y;
    }

  if (x1 < h_adj->lower)
    x1 = h_adj->lower;
  if (x1 > h_adj->upper - h_adj->page_size)
    x1 = x0;

  if (y1 < v_adj->lower)
    y1 = y0;
  if (y1 > v_adj->upper - v_adj->page_size)
    y1 = y0;

  if (x0 != x1 && y0 != y1)
    ghidgui->combine_adjustments = TRUE;
  if (x0 != x1)
    gtk_range_set_value (GTK_RANGE (ghidgui->h_range), x1);
  if (y0 != y1)
    gtk_range_set_value (GTK_RANGE (ghidgui->v_range), y1);

  ghid_note_event_location (NULL);
  return ((x0 != x1) || (y0 != y1));
}

  /* Do scrollbar scaling based on current port drawing area size and
     |  overall PCB board size.
   */
void
ghid_port_ranges_scale (gboolean emit_changed)
{
  GtkAdjustment *adj;

  /* Update the scrollbars with PCB units.  So Scale the current
     |  drawing area size in pixels to PCB units and that will be
     |  the page size for the Gtk adjustment.
   */
  gport->view_width = gport->width * gport->zoom;
  gport->view_height = gport->height * gport->zoom;

  if (gport->view_width >= PCB->MaxWidth)
    gport->view_width = PCB->MaxWidth;
  if (gport->view_height >= PCB->MaxHeight)
    gport->view_height = PCB->MaxHeight;

  adj = gtk_range_get_adjustment (GTK_RANGE (ghidgui->h_range));
  adj->page_size = gport->view_width;
  adj->upper = PCB->MaxWidth;
  if (emit_changed)
    gtk_signal_emit_by_name (GTK_OBJECT (adj), "changed");

  adj = gtk_range_get_adjustment (GTK_RANGE (ghidgui->v_range));
  adj->page_size = gport->view_height;
  adj->upper = PCB->MaxHeight;
  if (emit_changed)
    gtk_signal_emit_by_name (GTK_OBJECT (adj), "changed");
}

void
ghid_port_ranges_zoom (gdouble zoom)
{
  gdouble xtmp, ytmp;
  gint		x0, y0;

  xtmp = (gdouble) PCB->MaxWidth / gport->width;
  ytmp = (gdouble) PCB->MaxHeight / gport->height;

  if ((zoom > xtmp && zoom > ytmp) || zoom == 0.0)
    zoom = (xtmp > ytmp) ? xtmp : ytmp;

  xtmp = (gport->view_x - gport->view_x0) / (gdouble) gport->view_width;
  ytmp = (gport->view_y - gport->view_y0) / (gdouble) gport->view_height;

  gport->zoom = zoom;
  ghid_port_ranges_scale(FALSE);

  x0 = gport->view_x - xtmp * gport->view_width;
  if (x0 < 0)
    x0 = 0;
  gport->view_x0 = x0;

  y0 = gport->view_y - ytmp * gport->view_height;
  if (y0 < 0)
    y0 = 0;
  gport->view_y0 = y0;

  ghidgui->adjustment_changed_holdoff = TRUE;
  gtk_range_set_value (GTK_RANGE (ghidgui->h_range), gport->view_x0);
  gtk_range_set_value (GTK_RANGE (ghidgui->v_range), gport->view_y0);
  ghidgui->adjustment_changed_holdoff = FALSE;

  ghid_port_ranges_changed();
}


/* ---------------------------------------------------------------------- 
 * handles all events from PCB drawing area
 */

static gint event_x, event_y;

void
ghid_get_coords (const char *msg, int *x, int *y)
{
  if (!ghid_port.has_entered)
    ghid_get_user_xy (msg);
  *x = gport->view_x;
  *y = gport->view_y;
}

gboolean
ghid_note_event_location (GdkEventButton * ev)
{
  gint x, y;
  gboolean moved;

  if (!ev)
    {
      gdk_window_get_pointer (ghid_port.drawing_area->window, &x, &y, NULL);
      event_x = x;
      event_y = y;
    }
  else
    {
      event_x = ev->x;
      event_y = ev->y;
    }
  gport->view_x = event_x * gport->zoom + gport->view_x0;
  gport->view_y = event_y * gport->zoom + gport->view_y0;

  moved = MoveCrosshairAbsolute (SIDE_X (gport->view_x), gport->view_y);
  if (moved)
    {
      AdjustAttachedObjects ();
      RestoreCrosshair (False);
    }
  ghid_set_cursor_position_labels ();
  return moved;
}

static gboolean
have_crosshair_attachments (void)
{
  gboolean result = FALSE;

  switch (Settings.Mode)
    {
    case COPY_MODE:
    case MOVE_MODE:
    case INSERTPOINT_MODE:
      if (Crosshair.AttachedObject.Type != NO_TYPE)
	result = TRUE;
      break;
    case PASTEBUFFER_MODE:
    case VIA_MODE:
      result = TRUE;
      break;
    case POLYGON_MODE:
      if (Crosshair.AttachedLine.State != STATE_FIRST)
	result = TRUE;
      break;
    case ARC_MODE:
      if (Crosshair.AttachedBox.State != STATE_FIRST)
	result = TRUE;
      break;
    case LINE_MODE:
      if (Crosshair.AttachedLine.State != STATE_FIRST)
	result = TRUE;
      break;
    default:
      if (Crosshair.AttachedBox.State == STATE_SECOND
	  || Crosshair.AttachedBox.State == STATE_THIRD)
	result = TRUE;
      break;
    }
  return result;
}


#define	VCW		16
#define VCD		8

void
ghid_show_crosshair (gboolean show)
{
  gint x, y;
  static gint x_prev = -1, y_prev = -1;
  static GdkGC *xor_gc;
  static GdkColor cross_color;

  if (gport->x_crosshair < 0 || ghidgui->creating || !gport->has_entered)
    return;

  if (!xor_gc)
    {
      xor_gc = gdk_gc_new (ghid_port.drawing_area->window);
      gdk_gc_copy (xor_gc, ghid_port.drawing_area->style->white_gc);
      gdk_gc_set_function (xor_gc, GDK_XOR);
      /* FIXME: when CrossColor changed from config */
      ghid_map_color_string (Settings.CrossColor, &cross_color);
    }
  x = DRAW_X (gport->x_crosshair);
  y = DRAW_Y (gport->y_crosshair);

  gdk_gc_set_foreground (xor_gc, &cross_color);

  if (x_prev >= 0)
    {
      gdk_draw_line (gport->drawing_area->window, xor_gc,
		     x_prev, 0, x_prev, gport->height);
      gdk_draw_line (gport->drawing_area->window, xor_gc,
		     0, y_prev, gport->width, y_prev);
      if (ghidgui->auto_pan_on && have_crosshair_attachments ())
	{
	  gdk_draw_rectangle (gport->drawing_area->window, xor_gc, TRUE,
			      0, y_prev - VCD, VCD, VCW);
	  gdk_draw_rectangle (gport->drawing_area->window, xor_gc, TRUE,
			      gport->width - VCD, y_prev - VCD, VCD, VCW);
	  gdk_draw_rectangle (gport->drawing_area->window, xor_gc, TRUE,
			      x_prev - VCD, 0, VCW, VCD);
	  gdk_draw_rectangle (gport->drawing_area->window, xor_gc, TRUE,
			      x_prev - VCD, gport->height - VCD, VCW, VCD);
	}
    }

  if (x >= 0 && show)
    {
      gdk_draw_line (gport->drawing_area->window, xor_gc,
		     x, 0, x, gport->height);
      gdk_draw_line (gport->drawing_area->window, xor_gc,
		     0, y, gport->width, y);
      if (ghidgui->auto_pan_on && have_crosshair_attachments ())
	{
	  gdk_draw_rectangle (gport->drawing_area->window, xor_gc, TRUE,
			      0, y - VCD, VCD, VCW);
	  gdk_draw_rectangle (gport->drawing_area->window, xor_gc, TRUE,
			      gport->width - VCD, y - VCD, VCD, VCW);
	  gdk_draw_rectangle (gport->drawing_area->window, xor_gc, TRUE,
			      x - VCD, 0, VCW, VCD);
	  gdk_draw_rectangle (gport->drawing_area->window, xor_gc, TRUE,
			      x - VCD, gport->height - VCD, VCW, VCD);
	}
      x_prev = x;
      y_prev = y;
    }
  else
    x_prev = y_prev = -1;
}

static gboolean
ghid_idle_cb (gpointer data)
{
  if (Settings.Mode == NO_MODE)
    SetMode (ARROW_MODE);
  ghid_mode_cursor (Settings.Mode);
  if (ghidgui->settings_mode != Settings.Mode)
    {
      ghid_mode_buttons_update ();
    }
  ghidgui->settings_mode = Settings.Mode;
  return FALSE;
}

gboolean
ghid_port_key_release_cb (GtkWidget * drawing_area, GdkEventKey * kev,
			  GtkUIManager * ui)
{
  gint ksym = kev->keyval;

  if (ghid_is_modifier_key_sym (ksym))
    ghid_note_event_location (NULL);

  HideCrosshair (TRUE);
  AdjustAttachedObjects ();
  ghid_invalidate_all ();
  RestoreCrosshair (TRUE);
  ghid_screen_update ();
  g_idle_add (ghid_idle_cb, NULL);
  return FALSE;
}

  /* Handle user keys in the output drawing area.
     |  Note that there are some menu shortcuts in gui-top-window.c and it's
     |  possible there's overlap with key actions coded here and menu shortcut
     |  actions.  If the menu handles it, we won't see the key and code here
     |  won't be called.  I've commented the cases below, but probably have
     |  missed some.
     |  If a method of allowing user defined keys is implemented, it will have
     |  to deal somehow with the menu shortcuts.
   */

gboolean
ghid_port_key_press_cb (GtkWidget * drawing_area,
			GdkEventKey * kev, GtkUIManager * ui)
{
  ModifierKeysState mk;
  gchar *arg, *units;
  gdouble value;
  gint tmp, ksym = kev->keyval;
  gboolean handled;

  if (ghid_is_modifier_key_sym (ksym))
    ghid_note_event_location (NULL);

  mk = ghid_modifier_keys_state (&kev->state);

  ghid_show_crosshair (FALSE);

  handled = TRUE;		/* Start off assuming we handle it */
  switch (ksym)
    {
    case GDK_a:
    case GDK_A:
      if (mk == NONE_PRESSED)
	hid_action ("SetSame");
      else
	handled = FALSE;
      break;

    case GDK_b:
    case GDK_B:
      if (mk == NONE_PRESSED)
	hid_actionl ("Flip", "Object", 0);
      else
	handled = FALSE;
      break;

    case GDK_c:
    case GDK_C:
      if (mk == CONTROL_PRESSED)
	{
	  hid_actionl ("PasteBuffer", "Clear", 0);
	  hid_actionl ("PasteBuffer", "AddSelected", 0);
	  hid_actionl ("Unselect", "All", 0);
	  hid_actionl ("Mode", "PasteBuffer", 0);
	}
      else
	handled = FALSE;
      break;

    case GDK_d:
    case GDK_D:
      if (mk == NONE_PRESSED)
	hid_actionl ("Display", "PinOrPadName", 0);
      else
	handled = FALSE;
      break;

    case GDK_e:
    case GDK_E:
      if (mk == NONE_PRESSED)	/* XXX handled in menu */
	hid_actionl ("DeleteRats", "AllRats", 0);
      else if (mk == SHIFT_PRESSED)
	hid_actionl ("DeleteRats", "SelectedRats", 0);
      else
	handled = FALSE;
      break;

    case GDK_f:
    case GDK_F:
      if (mk == NONE_PRESSED)
	{
	  hid_actionl ("Connection", "Reset", 0);
	  hid_actionl ("Connection", "Find", 0);
	}
      else if (mk == SHIFT_PRESSED)
	hid_actionl ("Connection", "Reset", 0);
      else if (mk == CONTROL_PRESSED)
	hid_actionl ("Connection", "Find", 0);
      else
	handled = FALSE;
      break;

    case GDK_g:
    case GDK_G:
      value = Settings.grid_units_mm ?
	Settings.grid_increment_mm : Settings.grid_increment_mil;
      if (mk == NONE_PRESSED)
	arg = g_strdup_printf ("+%f", value);
      else if (mk == SHIFT_PRESSED)
	arg = g_strdup_printf ("-%f", value);
      else
	{
	  handled = FALSE;
	  break;
	}
      hid_actionl ("SetValue", "Grid", arg,
		   Settings.grid_units_mm ? "mm" : "mil", NULL);
      g_free (arg);

      /* User set grid may no longer match grid setting menu.
       */
      ghid_grid_setting_update_menu_actions ();
      break;

    case GDK_h:
    case GDK_H:
      if (mk == CONTROL_PRESSED)
	hid_actionl ("ChangeHole", "Object", 0);
      else if (mk == SHIFT_PRESSED)
	hid_actionl ("ToggleHideName", "SelectedElements", 0);
      else if (mk == NONE_PRESSED)
	hid_actionl ("ToggleHideName", "Object", 0);
      else
	handled = FALSE;
      break;

    case GDK_j:
    case GDK_J:
      if (mk == SHIFT_PRESSED)
	hid_actionl ("ChangeJoin", "SelectedObjects", 0);
      else if (mk == NONE_PRESSED)
	hid_actionl ("ChangeJoin", "Object", 0);
      else
	handled = FALSE;
      break;

    case GDK_k:
    case GDK_K:
      if (mk == CONTROL_PRESSED || mk == SHIFT_CONTROL_PRESSED)
	{
	  ghid_clear_increment_get_value ((mk == CONTROL_PRESSED) ? "+" : "-",
					  &arg, &units);
	  hid_actionl ("ChangeClearSize", "SelectedObjects", arg, units, 0);
	}
      else
	{
	  ghid_clear_increment_get_value ((mk == NONE_PRESSED) ? "+" : "-",
					  &arg, &units);
	  hid_actionl ("ChangeClearSize", "Object", arg, units, 0);
	}
      break;

    case GDK_l:
    case GDK_L:
      if (mk == SHIFT_PRESSED)
	{
	  ghid_line_increment_get_value ("-", &arg, &units);
	  hid_actionl ("SetValue", "LineSize", arg, units, 0);
	}
      else if (mk == NONE_PRESSED)
	{
	  ghid_line_increment_get_value ("+", &arg, &units);
	  hid_actionl ("SetValue", "LineSize", arg, units, 0);
	}
      else
	handled = FALSE;
      break;

    case GDK_m:
    case GDK_M:
      if (mk == CONTROL_PRESSED)
	hid_action ("MarkCrosshair");
      else if (mk == SHIFT_PRESSED)
	hid_actionl ("MoveToCurrentLayer", "SelectedObjects", 0);
      else if (mk == NONE_PRESSED)
	hid_actionl ("MoveToCurrentLayer", "Object", 0);
      else
	handled = FALSE;
      break;

    case GDK_n:
    case GDK_N:
      if (mk == SHIFT_PRESSED)
	hid_actionl ("AddRats", "Close", 0);
      else
	handled = FALSE;
      break;

    case GDK_o:
    case GDK_O:
      if (mk == NONE_PRESSED)	/* XXX handled in menu */
	{
	  hid_actionl ("Atomic", "Save", 0);
	  hid_actionl ("DeleteRats", "AllRats", 0);
	  hid_actionl ("Atomic", "Restore", 0);
	  hid_actionl ("AddRats", "AllRats", 0);
	  hid_actionl ("Atomic", "Block", 0);
	}
      else if (mk == CONTROL_PRESSED)
	hid_actionl ("ChangeOctagon", "Object", 0);
      else if (mk == SHIFT_PRESSED)
	{
	  hid_actionl ("Atomic", "Save", 0);
	  hid_actionl ("DeleteRats", "AllRats", 0);
	  hid_actionl ("Atomic", "Restore", 0);
	  hid_actionl ("AddRats", "SelectedRats", 0);
	  hid_actionl ("Atomic", "Block", 0);
	}
      else
	handled = FALSE;
      break;

    case GDK_p:
    case GDK_P:
      if (mk == SHIFT_PRESSED)
	hid_actionl ("Polygon", "Close", 0);
      else if (mk == NONE_PRESSED)
	hid_actionl ("Polygon", "PreviousPoint", 0);
      else
	handled = FALSE;
      break;

    case GDK_q:
    case GDK_Q:
      if (mk == NONE_PRESSED)
	hid_actionl ("ChangeSquare", "ToggleObject", 0);
      else
	handled = FALSE;
      break;

    case GDK_s:
    case GDK_S:
      if (mk == MOD1_PRESSED || mk == SHIFT_MOD1_PRESSED)
	{
	  ghid_size_increment_get_value ((mk == MOD1_PRESSED) ? "+" : "-",
					 &arg, &units);
	  hid_actionl ("ChangeDrillSize", "Object", arg, units, 0);
	}
      else
	{
	  ghid_size_increment_get_value ((mk == NONE_PRESSED) ? "+" : "-",
					 &arg, &units);
	  hid_actionl ("ChangeSize", "Object", arg, units, 0);
	}
      break;

    case GDK_t:
    case GDK_T:
      if (mk == SHIFT_PRESSED)
	{
	  ghid_size_increment_get_value ("-", &arg, &units);
	  hid_actionl ("SetValue", "TextScale", arg, units, 0);
	}
      else if (mk == NONE_PRESSED)
	{
	  ghid_size_increment_get_value ("+", &arg, &units);
	  hid_actionl ("SetValue", "TextScale", arg, units, 0);
	}
      else
	handled = FALSE;
      break;

    case GDK_u:
    case GDK_U:
      if (mk == NONE_PRESSED)	/* XXX handled in menu */
	hid_action ("Undo");
      else
	handled = FALSE;
      break;

    case GDK_v:
    case GDK_V:
      if (mk == NONE_PRESSED)
	ghid_port_ranges_zoom (0);
      else if (mk == MOD1_PRESSED || mk == SHIFT_MOD1_PRESSED)
	{
	  ghid_size_increment_get_value ((mk == MOD1_PRESSED) ? "+" : "-",
					 &arg, &units);
	  hid_actionl ("SetValue", "ViaDrillingHole", arg, units, 0);
	}
      else if (mk == CONTROL_PRESSED || mk == SHIFT_CONTROL_PRESSED)
	{
	  ghid_size_increment_get_value ((mk == CONTROL_PRESSED) ? "+" : "-",
					 &arg, &units);
	  hid_actionl ("SetValue", "ViaSize", arg, units, 0);
	}
      else
	handled = FALSE;
      break;

    case GDK_w:
    case GDK_W:
      if (mk == SHIFT_PRESSED)
	hid_actionl ("AddRats", "SelectedRats", 0);
      else if (mk == NONE_PRESSED)
	hid_actionl ("AddRats", "AllRats", 0);
      else
	handled = FALSE;
      break;

    case GDK_z:
    case GDK_Z:
      if (mk == NONE_PRESSED)	/* XXX handled in menu */
	hid_actionl ("SetValue", "Zoom", "-1", 0);
      else			/* <shift>z handled in menu shortcut */
	handled = FALSE;
      break;

    case GDK_bar:
    case GDK_backslash:
      hid_actionl ("Display", "ToggleThindraw", 0);
      ghid_set_menu_toggle_button (ghidgui->main_actions,
				   "ToggleThinDraw", TEST_FLAG (THINDRAWFLAG,
								PCB));
      break;

    case GDK_Tab:
      hid_action ("SwapSides");
      ghid_set_menu_toggle_button (ghidgui->main_actions,
				   "ToggleViewSolderSide",
				   Settings.ShowSolderSide);
      break;

    case GDK_Escape:
      if ((Settings.Mode == LINE_MODE
	   && Crosshair.AttachedLine.State != STATE_FIRST)
	  || (Settings.Mode == ARC_MODE
	      && Crosshair.AttachedBox.State != STATE_FIRST)
	  || (Settings.Mode == RECTANGLE_MODE
	      && Crosshair.AttachedBox.State != STATE_FIRST)
	  || (Settings.Mode == POLYGON_MODE
	      && Crosshair.AttachedLine.State != STATE_FIRST))
	{
	  if (Settings.Mode == LINE_MODE)
	    hid_actionl ("Mode", "Line", 0);
	  else if (Settings.Mode == ARC_MODE)
	    hid_actionl ("Mode", "Arc", 0);
	  else if (Settings.Mode == RECTANGLE_MODE)
	    hid_actionl ("Mode", "Rectangle", 0);
	  else if (Settings.Mode == POLYGON_MODE)
	    hid_actionl ("Mode", "Polygon", 0);

	  hid_actionl ("Mode", "Notify", 0);
	}
      hid_actionl ("Mode", "Cancel", 0);
      break;

    case GDK_space:
      hid_actionl ("Mode", "Arrow", 0);
      break;

    case GDK_colon:
      hid_action ("Command");
      break;

    case GDK_period:
      hid_actionl ("Display", "Toggle45Degree", 0);
      ghid_set_menu_toggle_button (ghidgui->main_actions,
				   "Toggle45degree",
				   TEST_FLAG (ALLDIRECTIONFLAG, PCB));
      break;

    case GDK_slash:
      hid_actionl ("Display", "CycleClip", 0);
      break;

    case GDK_Up:
      if (mk == CONTROL_PRESSED)
	{
	  hid_actionl ("Display", "Scroll", "8", 0);
	  hid_actionl ("Display", "Scroll", "0", 0);
	}
      else if (mk == SHIFT_PRESSED)
	hid_actionl ("MovePointer", "0", "-10", 0);
      else if (mk == NONE_PRESSED)
	hid_actionl ("MovePointer", "0", "-1", 0);
      break;

    case GDK_Down:
      if (mk == CONTROL_PRESSED)
	{
	  hid_actionl ("Display", "Scroll", "2", 0);
	  hid_actionl ("Display", "Scroll", "0", 0);
	}
      else if (mk == SHIFT_PRESSED)
	hid_actionl ("MovePointer", "0", "10", 0);
      else if (mk == NONE_PRESSED)
	hid_actionl ("MovePointer", "0", "1", 0);
      break;

    case GDK_Left:
      if (mk == CONTROL_PRESSED)
	{
	  hid_actionl ("Display", "Scroll", "4", 0);
	  hid_actionl ("Display", "Scroll", "0", 0);
	}
      else if (mk == SHIFT_PRESSED)
	hid_actionl ("MovePointer", "-10", "0", 0);
      else if (mk == NONE_PRESSED)
	hid_actionl ("MovePointer", "-1", "0", 0);
      break;

    case GDK_Right:
      if (mk == CONTROL_PRESSED)
	{
	  hid_actionl ("Display", "Scroll", "6", 0);
	  hid_actionl ("Display", "Scroll", "0", 0);
	}
      else if (mk == SHIFT_PRESSED)
	hid_actionl ("MovePointer", "10", "0", 0);
      else if (mk == NONE_PRESSED)
	hid_actionl ("MovePointer", "1", "0", 0);
      break;

    case GDK_BackSpace:
    case GDK_Delete:
      if (mk == SHIFT_PRESSED)
	{
	  hid_actionl ("Atomic", "Save", 0);
	  hid_actionl ("Connection", "Reset", 0);
	  hid_actionl ("Atomic", "Restore", 0);
	  hid_actionl ("Unselect", "All", 0);
	  hid_actionl ("Atomic", "Restore", 0);
	  hid_actionl ("Connection", "Find", 0);
	  hid_actionl ("Atomic", "Restore", 0);
	  hid_actionl ("Select", "Connection", 0);
	  hid_actionl ("Atomic", "Restore", 0);
	  hid_action ("RemoveSelected");
	  hid_actionl ("Atomic", "Restore", 0);
	  hid_actionl ("Connection", "Reset", 0);
	  hid_actionl ("Atomic", "Restore", 0);
	  hid_actionl ("Unselect", "All", 0);
	  hid_actionl ("Atomic", "Block", 0);
	}
      else if (mk == NONE_PRESSED)
	{
	  hid_actionl ("Mode", "Save", 0);
	  hid_actionl ("Mode", "Remove", 0);
	  hid_actionl ("Mode", "Notify", 0);
	  hid_actionl ("Mode", "Restore", 0);
	}
      break;

    case GDK_Insert:
      if (mk == NONE_PRESSED)
	hid_actionl ("Mode", "InsertPoint", 0);

    case GDK_1:
    case GDK_2:
    case GDK_3:
    case GDK_4:
    case GDK_5:
    case GDK_6:
    case GDK_7:
    case GDK_8:
    case GDK_9:
      tmp = ksym - GDK_1;
      if (mk == NONE_PRESSED)
	ghid_layer_button_select (tmp);
      else
	handled = FALSE;
      break;

    case GDK_F1:
      hid_actionl ("Mode", "Via", 0);
      break;

    case GDK_F2:
      hid_actionl ("Mode", "Line", 0);
      break;

    case GDK_F3:
      hid_actionl ("Mode", "Arc", 0);
      break;

    case GDK_F4:
      hid_actionl ("Mode", "Text", 0);
      break;

    case GDK_F5:
      hid_actionl ("Mode", "Rectangle", 0);
      break;

    case GDK_F6:
      hid_actionl ("Mode", "Polygon", 0);
      break;

    case GDK_F7:
      hid_actionl ("Mode", "PasteBuffer", 0);
      break;

    case GDK_F8:
      hid_actionl ("Mode", "Remove", 0);
      break;

    case GDK_F9:
      hid_actionl ("Mode", "Rotate", 0);
      break;

    case GDK_F10:
      hid_actionl ("Mode", "Thermal", 0);
      break;

    case GDK_F11:
      hid_actionl ("Mode", "Arrow", 0);
      break;

    case GDK_bracketleft:
      hid_actionl ("Mode", "Save", 0);
      hid_actionl ("Mode", "Arrow", 0);
      hid_actionl ("Mode", "Notify", 0);
      break;

    case GDK_bracketright:
      hid_actionl ("Mode", "Release", 0);
      hid_actionl ("Mode", "Restore", 0);
      break;
    default:
      handled = FALSE;
    }

  HideCrosshair (TRUE);
  AdjustAttachedObjects ();
  ghid_invalidate_all ();
  RestoreCrosshair (TRUE);
//      ghid_show_crosshair(TRUE);
  ghid_screen_update ();
  ghid_set_status_line_label ();
  g_idle_add (ghid_idle_cb, NULL);
  return handled;
}

static gboolean
in_draw_state (void)
{
  if ((Settings.Mode == LINE_MODE
       && Crosshair.AttachedLine.State != STATE_FIRST)
      || (Settings.Mode == ARC_MODE
	  && Crosshair.AttachedBox.State != STATE_FIRST)
      || (Settings.Mode == RECTANGLE_MODE
	  && Crosshair.AttachedBox.State != STATE_FIRST)
      || (Settings.Mode == POLYGON_MODE
	  && Crosshair.AttachedLine.State != STATE_FIRST))
    return TRUE;
  return FALSE;
}

static gboolean draw_state_reset;
static gint x_press, y_press;

gboolean
ghid_port_button_press_cb (GtkWidget * drawing_area,
			   GdkEventButton * ev, GtkUIManager * ui)
{
  GtkWidget *menu = gtk_ui_manager_get_widget (ui, "/PopupMenu");
  ModifierKeysState mk;
  gboolean drag, start_pan = FALSE;

  x_press = ev->x;
  y_press = ev->y;

  ghid_note_event_location (ev);
  mk = ghid_modifier_keys_state (&ev->state);
  ghid_show_crosshair (FALSE);
  HideCrosshair (TRUE);
  drag = have_crosshair_attachments ();
  draw_state_reset = FALSE;

  switch (ev->button)
    {
    case 1:
      if (mk == NONE_PRESSED || mk == SHIFT_PRESSED)
	hid_actionl ("Mode", "Notify", 0);
      else if (mk == CONTROL_PRESSED)
	{
	  hid_actionl ("Mode", "Save", 0);
	  hid_actionl ("Mode", "None", 0);
	  hid_actionl ("Mode", "Restore", 0);
	  hid_actionl ("Mode", "Notify", 0);
	}
      else if (mk == SHIFT_CONTROL_PRESSED)
	{
	  hid_actionl ("Mode", "Save", 0);
	  hid_actionl ("Mode", "Remove", 0);
	  hid_actionl ("Mode", "Notify", 0);
	  hid_actionl ("Mode", "Restore", 0);
	}
      break;

    case 2:
      if (mk == NONE_PRESSED && in_draw_state ())
	{
	  if (Settings.Mode == LINE_MODE)
	    hid_actionl ("Mode", "Line", 0);
	  else if (Settings.Mode == ARC_MODE)
	    hid_actionl ("Mode", "Arc", 0);
	  else if (Settings.Mode == RECTANGLE_MODE)
	    hid_actionl ("Mode", "Rectangle", 0);
	  else if (Settings.Mode == POLYGON_MODE)
	    hid_actionl ("Mode", "Polygon", 0);

	  hid_actionl ("Mode", "Notify", 0);
	  draw_state_reset = TRUE;
	}
      else if (mk == NONE_PRESSED)
	{
	  hid_actionl ("Mode", "Save", 0);
	  hid_actionl ("Mode", "Stroke", 0);
	}
      else if (mk == CONTROL_PRESSED)
	{
	  hid_actionl ("Mode", "Save", 0);
	  hid_actionl ("Mode", "Copy", 0);
	  hid_actionl ("Mode", "Notify", 0);
	}
      else if (mk == SHIFT_CONTROL_PRESSED)
	{
	  hid_actionl ("Display", "ToggleRubberbandMode", 0);
	  hid_actionl ("Mode", "Save", 0);
	  hid_actionl ("Mode", "Move", 0);
	  hid_actionl ("Mode", "Notify", 0);
	}
      break;

    case 3:
      if (mk == NONE_PRESSED)
	{
	  ghid_mode_cursor (PAN_MODE);
	  start_pan = TRUE;
	}
      else if (mk == SHIFT_PRESSED
	       && !ghidgui->command_entry_status_line_active)
	{
	  ghidgui->in_popup = TRUE;
	  gtk_widget_grab_focus (drawing_area);
	  if (GTK_IS_MENU (menu))
	    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL,
			    drawing_area, 3, ev->time);
	}

      break;
    }
  ghid_invalidate_all ();
  RestoreCrosshair (TRUE);
  ghid_set_status_line_label ();
  ghid_show_crosshair (TRUE);
  if (!start_pan)
    g_idle_add (ghid_idle_cb, NULL);
  return TRUE;
}


gboolean
ghid_port_button_release_cb (GtkWidget * drawing_area,
			     GdkEventButton * ev, GtkUIManager * ui)
{
  ModifierKeysState mk;
  gboolean drag;

  ghid_note_event_location (ev);
  mk = ghid_modifier_keys_state (&ev->state);

  drag = have_crosshair_attachments ();
  if (drag)
    HideCrosshair (TRUE);

  switch (ev->button)
    {
    case 1:
      hid_actionl ("Mode", "Release", 0);	/* For all modifier states */
      break;

    case 2:
      if (mk == NONE_PRESSED && !draw_state_reset)
	{
	  hid_actionl ("Mode", "Release", 0);
	  hid_actionl ("Mode", "Restore", 0);
	}
      else if (mk == CONTROL_PRESSED)
	{
	  hid_actionl ("Mode", "Notify", 0);
	  hid_actionl ("Mode", "Restore", 0);
	}
      else if (mk == SHIFT_CONTROL_PRESSED)
	{
	  hid_actionl ("Mode", "Notify", 0);
	  hid_actionl ("Mode", "Restore", 0);
	  hid_actionl ("Display", "ToggleRubberbandMode", 0);
	}
      break;

    case 3:
      if (mk == SHIFT_PRESSED)
	{
	  hid_actionl ("Display", "Center", 0);
	  hid_actionl ("Display", "Restore", 0);
	}
      else if (ev->x == x_press && ev->y == y_press)
	{
	  ghid_show_crosshair (FALSE);
	  ghidgui->auto_pan_on = !ghidgui->auto_pan_on;
	  ghid_show_crosshair (TRUE);
	}
      break;
    }
  if (drag)
    {
      AdjustAttachedObjects ();
      ghid_invalidate_all ();
      RestoreCrosshair (TRUE);
      ghid_screen_update ();
    }
  ghid_set_status_line_label ();
  g_idle_add (ghid_idle_cb, NULL);
  return TRUE;
}


gboolean
ghid_port_drawing_area_configure_event_cb (GtkWidget * widget,
					   GdkEventConfigure * ev,
					   GHidPort * out)
{
  static gboolean first_time_done;

  HideCrosshair (TRUE);
  gport->width = ev->width;
  gport->height = ev->height;

  if (gport->pixmap)
    gdk_pixmap_unref (gport->pixmap);

  gport->pixmap = gdk_pixmap_new (widget->window,
				  gport->width, gport->height, -1);
  gport->drawable = gport->pixmap;

  if (!first_time_done)
    {
      gport->colormap = gtk_widget_get_colormap (gport->top_window);
      gport->bg_gc = gdk_gc_new (gport->drawable);
      if (gdk_color_parse (Settings.BackgroundColor, &gport->bg_color))
	gdk_color_alloc (gport->colormap, &gport->bg_color);
      else
	gdk_color_white (gport->colormap, &gport->bg_color);
      gdk_gc_set_foreground (gport->bg_gc, &gport->bg_color);

      gport->offlimits_gc = gdk_gc_new (gport->drawable);
      if (gdk_color_parse (Settings.OffLimitColor, &gport->offlimits_color))
	gdk_color_alloc (gport->colormap, &gport->offlimits_color);
      else
	gdk_color_white (gport->colormap, &gport->offlimits_color);
      gdk_gc_set_foreground (gport->offlimits_gc, &gport->offlimits_color);
      first_time_done = TRUE;
      PCBChanged (0, NULL, 0, 0);
    }
  if (gport->mask)
    {
      gdk_pixmap_unref (gport->mask);
      gport->mask = gdk_pixmap_new (0, gport->width, gport->height, 1);
    }
  ghid_port_ranges_scale (FALSE);
  ghid_invalidate_all ();
  RestoreCrosshair (TRUE);
  return 0;
}


void
ghid_screen_update (void)
{

  ghid_show_crosshair (FALSE);
  gdk_draw_drawable (gport->drawing_area->window, gport->bg_gc, gport->pixmap,
		     0, 0, 0, 0, gport->width, gport->height);
  ghid_show_crosshair (TRUE);
}

gboolean
ghid_port_drawing_area_expose_event_cb (GtkWidget * widget,
					GdkEventExpose * ev, GHidPort * port)
{
  ghid_show_crosshair (FALSE);
  gdk_draw_drawable (widget->window, port->bg_gc, port->pixmap,
		     ev->area.x, ev->area.y, ev->area.x, ev->area.y,
		     ev->area.width, ev->area.height);
  ghid_show_crosshair (TRUE);
  return FALSE;
}

gint
ghid_port_window_motion_cb (GtkWidget * widget,
			    GdkEventButton * ev, GHidPort * out)
{
  ModifierKeysState mk = ghid_modifier_keys_state (&ev->state);
  gdouble dx, dy;
  static gint x_prev, y_prev;
  gboolean moved;

  if ((ev->state & GDK_BUTTON3_MASK) == GDK_BUTTON3_MASK
      && mk == NONE_PRESSED)
    {
      if (gtk_events_pending ())
	return FALSE;
      dx = gport->zoom * (x_prev - ev->x);
      dy = gport->zoom * (y_prev - ev->y);
      if (x_prev > 0)
	ghid_port_ranges_pan (dx, dy, TRUE);
      x_prev = ev->x;
      y_prev = ev->y;
      return FALSE;
    }
  x_prev = y_prev = -1;
  moved = ghid_note_event_location (ev);
  ghid_show_crosshair (TRUE);
  if (moved && have_crosshair_attachments ())
    ghid_draw_area_update (gport, NULL);
  return FALSE;
}

gint
ghid_port_window_enter_cb (GtkWidget * widget,
			   GdkEventButton * ev, GHidPort * out)
{
  if (!ghidgui->command_entry_status_line_active)
    {
      out->has_entered = TRUE;
      /* Make sure drawing area has keyboard focus when we are in it.
       */
      gtk_widget_grab_focus (out->drawing_area);
    }
  ghidgui->in_popup = FALSE;
  RestoreCrosshair (TRUE);
  return FALSE;
}

static gboolean
ghid_pan_idle_cb (gpointer data)
{
  gdouble dx = 0, dy = 0;

  if (gport->has_entered)
    return FALSE;
  dy = gport->zoom * y_pan_speed;
  dx = gport->zoom * x_pan_speed;
  return (ghid_port_ranges_pan (dx, dy, TRUE));
}

gint
ghid_port_window_leave_cb (GtkWidget * widget,
			   GdkEventButton * ev, GHidPort * out)
{
  gint x0, y0, x, y, dx, dy, w, h;

  if (out->has_entered && !ghidgui->in_popup)
    {
      /* if actively drawing, start scrolling */

      if (have_crosshair_attachments () && ghidgui->auto_pan_on)
	{
	  /* GdkEvent coords are set to 0,0 at leave events, so must figure
	     |  out edge the cursor left.
	   */
	  w = ghid_port.width * gport->zoom;
	  h = ghid_port.height * gport->zoom;

	  x0 = VIEW_X (0);
	  y0 = VIEW_Y (0);
	  ghid_get_coords (NULL, &x, &y);
	  x -= x0;
	  y -= y0;

	  dx = w - x;
	  dy = h - y;

	  x_pan_speed = y_pan_speed = 2 * ghidgui->auto_pan_speed;

	  if (x < dx)
	    {
	      x_pan_speed = -x_pan_speed;
	      dx = x;
	    }
	  if (y < dy)
	    {
	      y_pan_speed = -y_pan_speed;
	      dy = y;
	    }
	  if (dx < dy)
	    {
	      if (dy < h / 3)
		y_pan_speed = y_pan_speed - (3 * dy * y_pan_speed) / h;
	      else
		y_pan_speed = 0;
	    }
	  else
	    {
	      if (dx < w / 3)
		x_pan_speed = x_pan_speed - (3 * dx * x_pan_speed) / w;
	      else
		x_pan_speed = 0;
	    }
	  g_idle_add (ghid_pan_idle_cb, NULL);
	}
    }

  ghid_show_crosshair (FALSE);
  out->has_entered = FALSE;

  ghid_screen_update ();

  return FALSE;
}


  /* Mouse scroll wheel events
   */
gint
ghid_port_window_mouse_scroll_cb (GtkWidget * widget,
				  GdkEventScroll * ev, GHidPort * out)
{
  ModifierKeysState mk = ghid_modifier_keys_state (&ev->state);
  gdouble dx = 0.0, dy = 0.0, zoom_factor;

  if (mk == NONE_PRESSED)
    {
      zoom_factor = (ev->direction == GDK_SCROLL_UP) ? 0.8 : 1.25;
      ghid_port_ranges_zoom (gport->zoom * zoom_factor);
      return TRUE;
    }

  if (mk == SHIFT_PRESSED)
    dy = ghid_port.height * gport->zoom / 40;
  else
    dx = ghid_port.width * gport->zoom / 40;

  if (ev->direction == GDK_SCROLL_UP)
    {
      dx = -dx;
      dy = -dy;
    }

  HideCrosshair (FALSE);
  ghid_port_ranges_pan (dx, dy, TRUE);
  MoveCrosshairRelative (dx, dy);
  AdjustAttachedObjects ();
  RestoreCrosshair (FALSE);

  return TRUE;
}
