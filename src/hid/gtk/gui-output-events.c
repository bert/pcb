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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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
#include "hid/common/hid_resource.h"

#include <gdk/gdkkeysyms.h>

#include "action.h"
#include "crosshair.h"
#include "draw.h"
#include "error.h"
#include "misc.h"
#include "set.h"
#include "find.h"
#include "search.h"
#include "rats.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define TOOLTIP_UPDATE_DELAY 200

static gint x_pan_speed, y_pan_speed;
void
ghid_port_ranges_changed (void)
{
  GtkAdjustment *h_adj, *v_adj;

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
    x1 = h_adj->upper - h_adj->page_size;

  if (y1 < v_adj->lower)
    y1 = v_adj->lower;
  if (y1 > v_adj->upper - v_adj->page_size)
    y1 = v_adj->upper - v_adj->page_size;

  ghidgui->adjustment_changed_holdoff = TRUE;
  gtk_range_set_value (GTK_RANGE (ghidgui->h_range), x1);
  gtk_range_set_value (GTK_RANGE (ghidgui->v_range), y1);
  ghidgui->adjustment_changed_holdoff = FALSE;

  if ((x0 != x1) || (y0 != y1))
    ghid_port_ranges_changed();

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
  adj->page_increment = adj->page_size/10.0;
  adj->step_increment = adj->page_size/100.0;
  adj->upper = PCB->MaxWidth;
  if (emit_changed)
    gtk_signal_emit_by_name (GTK_OBJECT (adj), "changed");

  adj = gtk_range_get_adjustment (GTK_RANGE (ghidgui->v_range));
  adj->page_size = gport->view_height;
  adj->page_increment = adj->page_size/10.0;
  adj->step_increment = adj->page_size/100.0;
  adj->upper = PCB->MaxHeight;
  if (emit_changed)
    gtk_signal_emit_by_name (GTK_OBJECT (adj), "changed");
}

void
ghid_port_ranges_zoom (gdouble zoom)
{
  gdouble xtmp, ytmp;
  gint		x0, y0;

  /* figure out zoom values in that would just make the width fit and
   * that would just make the height fit
   */
  xtmp = (gdouble) PCB->MaxWidth / gport->width;
  ytmp = (gdouble) PCB->MaxHeight / gport->height;

  /* if we've tried to zoom further out than what would make the
   * entire board fit or we passed 0, then pick a zoom that just makes
   * the board fit.
   */
  if ((zoom > xtmp && zoom > ytmp) || zoom == 0.0)
    zoom = (xtmp > ytmp) ? xtmp : ytmp;

  xtmp = (SIDE_X (gport->pcb_x) - gport->view_x0) /
            (gdouble) gport->view_width;
  ytmp = (SIDE_Y (gport->pcb_y) - gport->view_y0) /
            (gdouble) gport->view_height;

  gport->zoom = zoom;
  pixel_slop = zoom;
  ghid_port_ranges_scale(FALSE);

  x0 = SIDE_X (gport->pcb_x) - xtmp * gport->view_width;
  if (x0 < 0)
    x0 = 0;
  gport->view_x0 = x0;

  y0 = SIDE_Y (gport->pcb_y) - ytmp * gport->view_height;
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

void
ghid_get_coords (const char *msg, int *x, int *y)
{
  if (!ghid_port.has_entered && msg)
    ghid_get_user_xy (msg);
  if (ghid_port.has_entered)
    {
      *x = gport->pcb_x;
      *y = gport->pcb_y;
    }
}

gboolean
ghid_note_event_location (GdkEventButton * ev)
{
  gint event_x, event_y;
  gboolean moved;

  if (!ev)
    {
      gdk_window_get_pointer (ghid_port.drawing_area->window,
                              &event_x, &event_y, NULL);
    }
  else
    {
      event_x = ev->x;
      event_y = ev->y;
    }

  ghid_event_to_pcb_coords (event_x, event_y, &gport->pcb_x, &gport->pcb_y);

  moved = MoveCrosshairAbsolute (gport->pcb_x, gport->pcb_y);
  if (moved)
    {
      AdjustAttachedObjects ();
      notify_crosshair_change (true);
    }
  ghid_set_cursor_position_labels ();
  return moved;
}

gboolean
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
    case POLYGONHOLE_MODE:
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

  ghid_update_toggle_flags ();
  return FALSE;
}

gboolean
ghid_port_key_release_cb (GtkWidget * drawing_area, GdkEventKey * kev,
			  GtkUIManager * ui)
{
  gint ksym = kev->keyval;

  if (ghid_is_modifier_key_sym (ksym))
    ghid_note_event_location (NULL);

  AdjustAttachedObjects ();
  ghid_invalidate_all ();
  g_idle_add (ghid_idle_cb, NULL);
  return FALSE;
}

/* Handle user keys in the output drawing area.
 * Note that the default is for all hotkeys to be handled by the
 * menu accelerators.
 *
 * Key presses not handled by the menus will show up here.  This means
 * the key press was either not defined in the menu resource file or
 * that the key press is special in that gtk doesn't allow the normal
 * menu code to ever see it.  We capture those here (like Tab and the
 * arrow keys) and feed it back to the normal menu callback.
 */

gboolean
ghid_port_key_press_cb (GtkWidget * drawing_area,
			GdkEventKey * kev, GtkUIManager * ui)
{
  ModifierKeysState mk;
  gint  ksym = kev->keyval;
  gboolean handled;
  extern  void ghid_hotkey_cb (int);
  GdkModifierType state;

  if (ghid_is_modifier_key_sym (ksym))
    ghid_note_event_location (NULL);

  state = (GdkModifierType) (kev->state);
  mk = ghid_modifier_keys_state (&state);

  handled = TRUE;		/* Start off assuming we handle it */
  switch (ksym)
    {
    case GDK_Alt_L:
    case GDK_Alt_R:
    case GDK_Control_L:
    case GDK_Control_R:
    case GDK_Shift_L:
    case GDK_Shift_R:
    case GDK_Shift_Lock:
    case GDK_ISO_Level3_Shift:
      break;

    case GDK_Up:
      ghid_hotkey_cb (GHID_KEY_UP);
      break;
      
    case GDK_Down:
      ghid_hotkey_cb (GHID_KEY_DOWN);
      break;
    case GDK_Left:
      ghid_hotkey_cb (GHID_KEY_LEFT);
      break;
    case GDK_Right:
      ghid_hotkey_cb (GHID_KEY_RIGHT);
      break;

    case GDK_ISO_Left_Tab: 
    case GDK_3270_BackTab: 
      switch (mk) 
	{
	case NONE_PRESSED:
	  ghid_hotkey_cb (GHID_KEY_SHIFT | GHID_KEY_TAB);
	  break;
	case CONTROL_PRESSED:
	  ghid_hotkey_cb (GHID_KEY_CONTROL | GHID_KEY_SHIFT | GHID_KEY_TAB);
	  break;
	case MOD1_PRESSED:
	  ghid_hotkey_cb (GHID_KEY_ALT | GHID_KEY_SHIFT | GHID_KEY_TAB);
	  break;
	case SHIFT_PRESSED:
	  ghid_hotkey_cb (GHID_KEY_SHIFT | GHID_KEY_TAB);
	  break;
	case SHIFT_CONTROL_PRESSED:
	  ghid_hotkey_cb (GHID_KEY_CONTROL | GHID_KEY_SHIFT | GHID_KEY_TAB);
	  break;
	case SHIFT_MOD1_PRESSED:
	  ghid_hotkey_cb (GHID_KEY_ALT | GHID_KEY_SHIFT | GHID_KEY_TAB);
	  break;
	  
	default:
	  handled = FALSE;
	  break;
	}
      break;

    case GDK_Tab: 
      switch (mk) 
	{
	case NONE_PRESSED:
	  ghid_hotkey_cb (GHID_KEY_TAB);
	  break;
	case CONTROL_PRESSED:
	  ghid_hotkey_cb (GHID_KEY_CONTROL | GHID_KEY_TAB);
	  break;
	case MOD1_PRESSED:
	  ghid_hotkey_cb (GHID_KEY_ALT | GHID_KEY_TAB);
	  break;
	case SHIFT_PRESSED:
	  ghid_hotkey_cb (GHID_KEY_SHIFT | GHID_KEY_TAB);
	  break;
	case SHIFT_CONTROL_PRESSED:
	  ghid_hotkey_cb (GHID_KEY_CONTROL | GHID_KEY_SHIFT | GHID_KEY_TAB);
	  break;
	case SHIFT_MOD1_PRESSED:
	  ghid_hotkey_cb (GHID_KEY_ALT | GHID_KEY_SHIFT | GHID_KEY_TAB);
	  break;
	  
	default:
	  handled = FALSE;
	  break;
	}
      break;

    default:
      handled = FALSE;
    }

  return handled;
}

gboolean
ghid_port_button_press_cb (GtkWidget * drawing_area,
			   GdkEventButton * ev, GtkUIManager * ui)
{
  ModifierKeysState mk;
  GdkModifierType state;

  /* Reject double and triple click events */
  if (ev->type != GDK_BUTTON_PRESS) return TRUE;

  ghid_note_event_location (ev);
  state = (GdkModifierType) (ev->state);
  mk = ghid_modifier_keys_state (&state);

  do_mouse_action(ev->button, mk);

  ghid_invalidate_all ();
  ghid_window_set_name_label (PCB->Name);
  ghid_set_status_line_label ();
  if (!gport->panning)
    g_idle_add (ghid_idle_cb, NULL);
  return TRUE;
}


gboolean
ghid_port_button_release_cb (GtkWidget * drawing_area,
			     GdkEventButton * ev, GtkUIManager * ui)
{
  ModifierKeysState mk;
  GdkModifierType state;

  ghid_note_event_location (ev);
  state = (GdkModifierType) (ev->state);
  mk = ghid_modifier_keys_state (&state);

  do_mouse_action(ev->button, mk + M_Release);

  AdjustAttachedObjects ();
  ghid_invalidate_all ();

  ghid_window_set_name_label (PCB->Name);
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
      if (gdk_color_parse (Settings.BackgroundColor, &gport->bg_color))
	gdk_color_alloc (gport->colormap, &gport->bg_color);
      else
	gdk_color_white (gport->colormap, &gport->bg_color);

      if (gdk_color_parse (Settings.OffLimitColor, &gport->offlimits_color))
	gdk_color_alloc (gport->colormap, &gport->offlimits_color);
      else
	gdk_color_white (gport->colormap, &gport->offlimits_color);
      first_time_done = TRUE;
      ghid_drawing_area_configure_hook (out);
      PCBChanged (0, NULL, 0, 0);
    }
  else
    {
      ghid_drawing_area_configure_hook (out);
    }

  ghid_port_ranges_scale (FALSE);
  ghid_invalidate_all ();
  return 0;
}


static char *
describe_location (LocationType X, LocationType Y)
{
  void *ptr1, *ptr2, *ptr3;
  int type;
  int Range = 0;
  char *elename = "";
  char *pinname;
  char *netname = NULL;
  char *description;

  /* check if there are any pins or pads at that position */

  type = SearchObjectByLocation (PIN_TYPE | PAD_TYPE,
                                 &ptr1, &ptr2, &ptr3, X, Y, Range);
  if (type == NO_TYPE)
    return NULL;

  /* don't mess with silk objects! */
  if (type & SILK_TYPE &&
      GetLayerNumber (PCB->Data, (LayerTypePtr) ptr1) >= max_copper_layer)
    return NULL;

  if (type == PIN_TYPE || type == PAD_TYPE)
    elename = (char *)UNKNOWN (NAMEONPCB_NAME ((ElementTypePtr) ptr1));

  pinname = ConnectionName (type, ptr1, ptr2);

  if (pinname == NULL)
    return NULL;

  /* Find netlist entry */
  MENU_LOOP (&PCB->NetlistLib);
  {
    if (!menu->Name)
    continue;

    ENTRY_LOOP (menu);
    {
      if (!entry->ListEntry)
        continue;

      if (strcmp (entry->ListEntry, pinname) == 0) {
        netname = g_strdup (menu->Name);
        /* For some reason, the netname has spaces in front of it, strip them */
        g_strstrip (netname);
        break;
      }
    }
    END_LOOP;

    if (netname != NULL)
      break;
  }
  END_LOOP;

  description = g_strdup_printf ("Element name: %s\n"
                                 "Pinname : %s\n"
                                 "Netname : %s",
                                 elename,
                                 (pinname != NULL) ? pinname : "--",
                                 (netname != NULL) ? netname : "--");

  g_free (netname);

  return description;
}


static gboolean check_object_tooltips (GHidPort *out)
{
  char *description;

  /* check if there are any pins or pads at that position */
  description = describe_location (out->crosshair_x, out->crosshair_y);

  if (description == NULL)
    return FALSE;

  gtk_widget_set_tooltip_text (out->drawing_area, description);
  g_free (description);

  return FALSE;
}

static int tooltip_update_timeout_id = 0;

static void
cancel_tooltip_update ()
{
  if (tooltip_update_timeout_id)
    g_source_remove (tooltip_update_timeout_id);
  tooltip_update_timeout_id = 0;
}

/* FIXME: If the GHidPort is ever destroyed, we must call
 * cancel_tooltip_update (), otherwise the timeout might
 * fire after the data it utilises has been free'd.
 */
static void
queue_tooltip_update (GHidPort *out)
{
  /* Zap the old tool-tip text and force it to be removed from the screen */
  gtk_widget_set_tooltip_text (out->drawing_area, NULL);
  gtk_widget_trigger_tooltip_query (out->drawing_area);

  cancel_tooltip_update ();

  tooltip_update_timeout_id =
      g_timeout_add (TOOLTIP_UPDATE_DELAY,
                     (GSourceFunc) check_object_tooltips,
                     out);
}

gint
ghid_port_window_motion_cb (GtkWidget * widget,
			    GdkEventMotion * ev, GHidPort * out)
{
  gdouble dx, dy;
  static gint x_prev = -1, y_prev = -1;

  gdk_event_request_motions (ev);

  if (out->panning)
    {
      dx = gport->zoom * (x_prev - ev->x);
      dy = gport->zoom * (y_prev - ev->y);
      if (x_prev > 0)
	ghid_port_ranges_pan (dx, dy, TRUE);
      x_prev = ev->x;
      y_prev = ev->y;
      return FALSE;
    }
  x_prev = y_prev = -1;
  ghid_note_event_location ((GdkEventButton *)ev);

  queue_tooltip_update (out);

  return FALSE;
}

gint
ghid_port_window_enter_cb (GtkWidget * widget,
			   GdkEventCrossing * ev, GHidPort * out)
{
  /* printf("enter: mode: %d detail: %d\n", ev->mode, ev->detail); */

  /* See comment in ghid_port_window_leave_cb() */

  if(ev->mode != GDK_CROSSING_NORMAL && ev->detail != GDK_NOTIFY_NONLINEAR) 
    {
      return FALSE;
    }

  if (!ghidgui->command_entry_status_line_active)
    {
      out->has_entered = TRUE;
      /* Make sure drawing area has keyboard focus when we are in it.
       */
      gtk_widget_grab_focus (out->drawing_area);
    }
  ghidgui->in_popup = FALSE;

  /* Following expression is true if a you open a menu from the menu bar, 
   * move the mouse to the viewport and click on it. This closes the menu 
   * and moves the pointer to the viewport without the pointer going over 
   * the edge of the viewport */
  if(ev->mode == GDK_CROSSING_UNGRAB && ev->detail == GDK_NOTIFY_NONLINEAR)
    {
      ghid_screen_update ();
    }
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
                           GdkEventCrossing * ev, GHidPort * out)
{
  gint x0, y0, x, y, dx, dy, w, h;
  
  /* printf("leave mode: %d detail: %d\n", ev->mode, ev->detail); */

  /* Window leave events can also be triggered because of focus grabs. Some
   * X applications occasionally grab the focus and so trigger this function.
   * At least GNOME's window manager is known to do this on every mouse click.
   *
   * See http://bugzilla.gnome.org/show_bug.cgi?id=102209 
   */

  if(ev->mode != GDK_CROSSING_NORMAL)
    {
      return FALSE;
    }

  if(out->has_entered && !ghidgui->in_popup)
    {
      /* if actively drawing, start scrolling */

      if (have_crosshair_attachments () && ghidgui->auto_pan_on)
	{
	  /* GdkEvent coords are set to 0,0 at leave events, so must figure
	     |  out edge the cursor left.
	   */
	  w = ghid_port.width * gport->zoom;
	  h = ghid_port.height * gport->zoom;

	  x0 = EVENT_TO_PCB_X (0);
	  y0 = EVENT_TO_PCB_Y (0);
	  ghid_get_coords (NULL, &x, &y);
	  x -= x0;
	  y -= y0;

	  if (ghid_flip_x )
	      x = -x;
	  if (ghid_flip_y )
	      y = -y;

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
  ModifierKeysState mk;
  GdkModifierType state;
  int button;

  state = (GdkModifierType) (ev->state);
  mk = ghid_modifier_keys_state (&state);

  /* X11 gtk hard codes buttons 4, 5, 6, 7 as below in
   * gtk+/gdk/x11/gdkevents-x11.c:1121, but quartz and windows have
   * special mouse scroll events, so this may conflict with a mouse
   * who has buttons 4 - 7 that aren't the scroll wheel?
   */
  switch(ev->direction)
    {
    case GDK_SCROLL_UP: button = 4; break;
    case GDK_SCROLL_DOWN: button = 5; break;
    case GDK_SCROLL_LEFT: button = 6; break;
    case GDK_SCROLL_RIGHT: button = 7; break;
    default: button = -1;
    }

  do_mouse_action(button, mk);

  return TRUE;
}
