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
 */

/* FIXME - rename this file to ghid.h */

#ifndef __GHID_INCLUDED__
#define __GHID_INCLUDED__

#include "hid.h"

#include "data.h"
#include "misc.h"
#include <sys/stat.h>

#include <gtk/gtk.h>

/* Internationalization support.
*/
#if defined (ENABLE_NLS)
#include <libintl.h>
#   undef _
#   define _(String) dgettext(PACKAGE,String)
#   if defined(gettext_noop)
#       define N_(String) gettext_noop(String)
#   else
#       define N_(String) (String)
#   endif /* gettext_noop */
#else
#   undef _
#   define _(String) (String)
#   define N_(String) (String)
#   define textdomain(String) (String)
#   define gettext(String) (String)
#   define dgettext(Domain,String) (String)
#   define dcgettext(Domain,String,Type) (String)
#   define bindtextdomain(Domain,Directory) (Domain)
#endif /* ENABLE_NLS */


  /* Silk and rats lines are the two additional selectable to draw on.
     |  gui code in gui-top-window.c and group code in misc.c must agree
     |  on what layer is what!
   */
#define	LAYER_BUTTON_SILK			MAX_LAYER
#define	LAYER_BUTTON_RATS			(MAX_LAYER + 1)
#define	N_SELECTABLE_LAYER_BUTTONS	(LAYER_BUTTON_RATS + 1)

#define LAYER_BUTTON_PINS			(MAX_LAYER + 2)
#define LAYER_BUTTON_VIAS			(MAX_LAYER + 3)
#define LAYER_BUTTON_FARSIDE		(MAX_LAYER + 4)
#define LAYER_BUTTON_MASK			(MAX_LAYER + 5)
#define N_LAYER_BUTTONS				(MAX_LAYER + 6)

  /* Go from from the grid units in use (millimeters or mils) to PCB units
     |  and back again.
     |  PCB keeps values internally to 1/100000 inch (0.01 mils), but gui
     |  widgets (spin buttons, labels, etc) need mils or millimeters.
   */
#define	FROM_PCB_UNITS(v)	(Settings.grid_units_mm ? \
								((v) * 0.000254) : ((v) * 0.01))

#define TO_PCB_UNITS(v)		(Settings.grid_units_mm ? \
								((v) / 0.000254 + 0.5) : ((v) * 100.0 + 0.5))

#define SIDE_X(x)   ((Settings.ShowSolderSide ? PCB->MaxWidth - (x) : (x)))

#define	DRAW_X(x)	(gint)((SIDE_X(x) - gport->view_x0) / gport->zoom)
#define	DRAW_Y(y)	(gint)(((y) - gport->view_y0) / gport->zoom)
#define	DRAW_Z(z)	(gint)((z) / gport->zoom)

#define	VIEW_X(x)	SIDE_X((gint)((x) * gport->zoom + gport->view_x0))
#define	VIEW_Y(y)	(gint)((y) * gport->zoom + gport->view_y0)
#define	VIEW_Z(z)	(gint)((z) * gport->zoom)


  /* Pick one of two values depending on current grid units setting.
   */
#define GRID_UNITS_VALUE(mm, mil)   (Settings.grid_units_mm ? (mm) : (mil))



typedef struct hid_gc_struct
{
  HID *me_pointer;
  GdkGC *gc;

  gchar *colorname;
  gint width;
  gint cap, join;
  gchar xor;
  gchar erase;
  gint mask_seq;
}
hid_gc_struct;


typedef struct
{
  GtkUIManager *ui_manager;
  GtkActionGroup *main_actions,
    *grid_actions, *change_selected_actions, *displayed_name_actions;

  GtkWidget *name_label,
    *status_line_label,
    *cursor_position_relative_label,
    *cursor_position_absolute_label,
    *grid_units_label, *status_line_hbox, *command_combo_box;
  GtkEntry *command_entry;

  GtkWidget *top_hbox,
    *menu_hbox, *compact_vbox, *compact_hbox, *position_hbox, *label_hbox,
    *mode_buttons0_vbox, *mode_buttons1_hbox, *mode_buttons1_vbox,
    *mode_buttons0_frame, *mode_buttons1_frame, *mode_buttons0_frame_vbox;

  GtkWidget *h_range, *v_range;
  GtkObject *h_adjustment, *v_adjustment;

  GdkPixbuf *bg_pixbuf;

  gchar *name_label_string;

  gboolean adjustment_changed_holdoff,
    toggle_holdoff,
    command_entry_status_line_active,
    auto_pan_on, in_popup, combine_adjustments;

  gboolean config_modified,
    small_label_markup,
    compact_horizontal,
    compact_vertical,
    ghid_title_window, use_command_window, need_restore_crosshair, creating;

  gint n_mode_button_columns,
    top_window_width,
    top_window_height,
    log_window_width,
    log_window_height,
    keyref_window_width,
    keyref_window_height,
    library_window_height,
    netlist_window_height, history_size, settings_mode, auto_pan_speed;
}
GhidGui;

extern GhidGui _ghidgui, *ghidgui;


  /* The output viewport
   */
typedef struct
{
  GtkWidget *top_window,	/* toplevel widget              */
   *drawing_area;		/* and its drawing area */
  GdkPixmap *pixmap, *mask;
  GdkDrawable *drawable;	/* Current drawable for drawing routines */
  gint width, height;

  GdkGC *bg_gc, *offlimits_gc, *mask_gc, *u_gc, *grid_gc;

  GdkColor bg_color, offlimits_color, grid_color;

  GdkColormap *colormap;

  GdkCursor *X_cursor;		/* used X cursor */
  GdkCursorType X_cursor_shape;	/* and its shape */

  gboolean has_entered;

/* zoom value is PCB units per screen pixel.  Larger numbers mean zooming
|  out - the largest value means you are looking at the whole board.
*/
  gdouble zoom;			/* PCB units per screen pixel.  Larger */
  /* numbers mean zooming out. */
  gint view_x0,			/* Viewport in PCB coordinates */
    view_y0, view_width, view_height, view_x, view_y;

  gint x_crosshair, y_crosshair;
}
GHidPort;

extern GHidPort ghid_port, *gport;

typedef enum
{
  NONE_PRESSED,
  SHIFT_PRESSED,
  CONTROL_PRESSED,
  MOD1_PRESSED,
  SHIFT_CONTROL_PRESSED,
  SHIFT_MOD1_PRESSED,
  CONTROL_MOD1_PRESSED,
  SHIFT_CONTROL_MOD1_PRESSED
}
ModifierKeysState;

typedef enum
{
  NO_BUTTON_PRESSED,
  BUTTON1_PRESSED,
  BUTTON2_PRESSED,
  BUTTON3_PRESSED
}
ButtonState;

typedef struct
{
  GtkWidget *top_window, *drawing_area, *enlarge_button, *shrink_button;

  ElementType element;		/* element data to display */

  gfloat zoom,			/* zoom factor of window */
    scale;			/* scale factor of zoom */
  gint x_max, y_max;
  gint w_pixels, h_pixels;
}
PinoutType;


/* Function prototypes
*/
void ghid_parse_arguments (gint * argc, gchar *** argv);
hidGC ghid_make_gc (void);
void ghid_destroy_gc (hidGC);
void ghid_do_export (HID_Attr_Val * options);

void ghid_create_pcb_widgets (void);
void ghid_window_set_name_label (gchar * name);
void ghid_interface_set_sensitive (gboolean sensitive);
void ghid_interface_input_signals_connect (void);
void ghid_interface_input_signals_disconnect (void);

void ghid_set_menu_toggle_button (GtkActionGroup * ag,
				  gchar * name, gboolean state);
void ghid_pcb_saved_toggle_states_set (void);
void ghid_sync_with_new_layout (void);
void ghid_size_increment_get_value (const gchar * saction,
				    gchar ** value, gchar ** units);
void ghid_line_increment_get_value (const gchar * saction,
				    gchar ** value, gchar ** units);
void ghid_clear_increment_get_value (const gchar * saction,
				     gchar ** value, gchar ** units);
void ghid_grid_size_increment_get_value (const gchar * saction,
					 gchar ** value, gchar ** units);

void ghid_grid_setting_update_menu_actions (void);
void ghid_change_selected_update_menu_actions (void);

void ghid_config_window_show ();
void ghid_config_handle_units_changed (void);
void ghid_config_start_backup_timer (void);
void ghid_config_text_scale_update (void);
void ghid_config_layer_name_update (gchar * name, gint layer);
void ghid_config_groups_changed(void);

void ghid_config_init (void);
void ghid_config_files_write (void);
void ghid_config_files_read (gint * argc, gchar *** argv);

void ghid_mode_buttons_update (void);
void ghid_pack_mode_buttons(void);
void ghid_layer_enable_buttons_update (void);
void ghid_layer_buttons_update (void);
void ghid_layer_button_select (gint layer);
void ghid_layer_buttons_color_update (void);


/* gui-misc.c function prototypes
*/
void ghid_status_line_set_text (gchar * text);
void ghid_cursor_position_label_set_text (gchar * text);
void ghid_cursor_position_relative_label_set_text (gchar * text);

void ghid_hand_cursor (void);
void ghid_watch_cursor (void);
void ghid_mode_cursor (gint mode);
void ghid_corner_cursor (void);
void ghid_restore_cursor (void);
void ghid_get_user_xy (const gchar * msg);
void ghid_create_abort_dialog (gchar *);
gboolean ghid_check_abort (void);
void ghid_end_abort (void);
void ghid_get_pointer (gint *, gint *);


/* gui-output-events.c function prototypes.
*/
void ghid_port_ranges_changed (void);
void ghid_port_ranges_zoom (gdouble zoom);
gboolean ghid_port_ranges_pan (gdouble x, gdouble y, gboolean relative);
void ghid_port_ranges_scale (gboolean emit_changed);
void ghid_port_ranges_update_ranges (void);
void ghid_show_crosshair (gboolean show);
void ghid_screen_update (void);

gboolean ghid_note_event_location (GdkEventButton * ev);

gboolean ghid_port_key_press_cb (GtkWidget * drawing_area,
				 GdkEventKey * kev, GtkUIManager * ui);
gboolean ghid_port_key_release_cb (GtkWidget * drawing_area,
				   GdkEventKey * kev, GtkUIManager * ui);
gboolean ghid_port_button_press_cb (GtkWidget * drawing_area,
				    GdkEventButton * ev, GtkUIManager * ui);
gboolean ghid_port_button_release_cb (GtkWidget * drawing_area,
				      GdkEventButton * ev, GtkUIManager * ui);


gint ghid_port_window_enter_cb (GtkWidget * widget,
			   GdkEventCrossing * ev, GHidPort * out);
gint ghid_port_window_leave_cb (GtkWidget * widget, 
                           GdkEventCrossing * ev, GHidPort * out);
gint ghid_port_window_motion_cb (GtkWidget * widget,
				 GdkEventButton * ev, GHidPort * out);
gint ghid_port_window_mouse_scroll_cb (GtkWidget * widget,
				       GdkEventScroll * ev, GHidPort * out);


gint ghid_port_drawing_area_expose_event_cb (GtkWidget * widget,
					     GdkEventExpose * ev,
					     GHidPort * out);
gint ghid_port_drawing_area_configure_event_cb (GtkWidget * widget,
						GdkEventConfigure * ev,
						GHidPort * out);
void ghid_pinout_expose (PinoutType * po);


/* gui-dialog.c function prototypes.
*/
#define		GUI_DIALOG_RESPONSE_ALL	1

gchar *ghid_dialog_file_select_open (gchar * title, gchar ** path,
				     gchar * shortcuts);
gchar *ghid_dialog_file_select_save (gchar * title, gchar ** path,
				     gchar * file, gchar * shortcuts);
void ghid_dialog_message (gchar * message);
gboolean ghid_dialog_confirm (gchar * message);
gint ghid_dialog_confirm_all (gchar * message);
gchar *ghid_dialog_input (gchar * prompt, gchar * initial);
void ghid_dialog_about (void);


/* gui-dialog-print.c */
void ghid_dialog_print (HID * printer);
void ghid_dialog_export (void);


/* gui-route-style function prototypes.
*/
  /* In gui-dialog-size.c */
void ghid_route_style_dialog (gint index, RouteStyleType * rst);
  /* In gui-top-window.c  */
void ghid_route_style_set_button_label (gchar * name, gint index);
void ghid_route_style_set_temp_style (RouteStyleType * rst, gint which);
void ghid_route_style_button_set_active (gint number);
void ghid_route_style_buttons_update (void);


/* gui-utils.c
*/
gboolean dup_string (gchar ** dst, gchar * src);
gboolean utf8_dup_string (gchar ** dst_utf8, gchar * src);
void free_glist_and_data (GList ** list_head);


ModifierKeysState ghid_modifier_keys_state (GdkModifierType * state);
ButtonState ghid_button_state (GdkModifierType * state);
gboolean ghid_is_modifier_key_sym (gint ksym);
gboolean ghid_control_is_pressed (void);
gboolean ghid_shift_is_pressed (void);

void ghid_draw_area_update (GHidPort * out, GdkRectangle * rect);
void ghid_string_markup_extents (PangoFontDescription * font_desc,
				 gchar * string, gint * width, gint * height);
void ghid_draw_string_markup (GdkDrawable * drawable,
			      PangoFontDescription * font_desc,
			      GdkGC * gc, gint x, gint y, gchar * string);

gchar *ghid_get_color_name (GdkColor * color);
void ghid_map_color_string (gchar * color_string, GdkColor * color);
void ghid_button_set_text (GtkWidget * button, gchar * text);
gboolean ghid_button_active (GtkWidget * widget);
gchar *ghid_entry_get_text (GtkWidget * entry);
void ghid_check_button_connected (GtkWidget * box, GtkWidget ** button,
				  gboolean active, gboolean pack_start,
				  gboolean expand, gboolean fill, gint pad,
				  void (*cb_func) (), gpointer data,
				  gchar * string);
void ghid_button_connected (GtkWidget * box, GtkWidget ** button,
			    gboolean pack_start, gboolean expand,
			    gboolean fill, gint pad, void (*cb_func) (),
			    gpointer data, gchar * string);
void ghid_spin_button (GtkWidget * box, GtkWidget ** spin_button,
		       gfloat value, gfloat low, gfloat high, gfloat step0,
		       gfloat step1, gint digits, gint width,
		       void (*cb_func) (), gpointer data,
		       gboolean right_align, gchar * string);
void ghid_table_spin_button (GtkWidget * box, gint row, gint column,
			     GtkWidget ** spin_button, gfloat value,
			     gfloat low, gfloat high, gfloat step0,
			     gfloat step1, gint digits, gint width,
			     void (*cb_func) (), gpointer data,
			     gboolean right_align, gchar * string);
void ghid_range_control (GtkWidget * box, GtkWidget ** scale_res,
			 gboolean horizontal, GtkPositionType pos,
			 gboolean set_draw_value, gint digits,
			 gboolean pack_start, gboolean expand, gboolean fill,
			 guint pad, gfloat value, gfloat low, gfloat high,
			 gfloat step0, gfloat step1, void (*cb_func) (),
			 gpointer data);
GtkWidget *ghid_scrolled_vbox (GtkWidget * box, GtkWidget ** scr,
			       GtkPolicyType h_policy,
			       GtkPolicyType v_policy);
GtkWidget *ghid_framed_vbox (GtkWidget * box, gchar * label,
			     gint frame_border_width, gboolean frame_expand,
			     gint vbox_pad, gint vbox_border_width);
GtkWidget *ghid_framed_vbox_end (GtkWidget * box, gchar * label,
				 gint frame_border_width,
				 gboolean frame_expand, gint vbox_pad,
				 gint vbox_border_width);
GtkWidget *ghid_category_vbox (GtkWidget * box, const gchar * category_header,
			       gint header_pad, gint box_pad,
			       gboolean pack_start, gboolean bottom_pad);
GtkWidget *ghid_notebook_page (GtkWidget * tabs, char *name, gint pad,
			       gint border);
GtkWidget *ghid_framed_notebook_page (GtkWidget * tabs, char *name,
				      gint border, gint frame_border,
				      gint vbox_pad, gint vbox_border);
GtkWidget *ghid_scrolled_text_view (GtkWidget * box, GtkWidget ** scr,
				    GtkPolicyType h_policy,
				    GtkPolicyType v_policy);
void ghid_text_view_append (GtkWidget * view, gchar * string);
void ghid_text_view_append_strings (GtkWidget * view, gchar ** string,
				    gint n_strings);
GtkTreeSelection *ghid_scrolled_selection (GtkTreeView * treeview,
					   GtkWidget * box,
					   GtkSelectionMode s_mode,
					   GtkPolicyType h_policy,
					   GtkPolicyType v_policy,
					   void (*func_cb) (), gpointer data);

void ghid_dialog_report (gchar * title, gchar * message);
void ghid_label_set_markup (GtkWidget * label, gchar * text);

void ghid_set_cursor_position_labels (void);
void ghid_set_status_line_label (void);


/* gui-netlist-window.c */
void ghid_netlist_window_show (GHidPort * out, gboolean raise);
void ghid_netlist_window_update (gboolean init_nodes);
void ghid_netlist_nodes_update (LibraryMenuType * net);

LibraryMenuType *ghid_get_net_from_node_name (gchar * name, gboolean);
void ghid_netlist_highlight_node (gchar * name);


/* gui-command-window.c */
void ghid_handle_user_command (gboolean raise);
void ghid_command_window_show (gboolean raise);
gchar *ghid_command_entry_get (gchar * prompt, gchar * command);
void ghid_command_use_command_window_sync (void);

/* gui-keyref-window.c */
void ghid_keyref_window_show (gboolean raise);

/* gui-library-window.c */
void ghid_library_window_show (GHidPort * out, gboolean raise);


/* gui-log-window.c */
void ghid_log_window_show (gboolean raise);
void ghid_log (const char *fmt, ...);
void ghid_logv (const char *fmt, va_list args);

/* gui-pinout-window.c */
void ghid_pinout_window_show (GHidPort * out, ElementTypePtr Element);

/* gtkhid-main.c */
void ghid_invalidate_all ();
void ghid_get_coords (const char *msg, int *x, int *y);
void ghid_pinout_redraw (PinoutType * po);
gint PCBChanged (int argc, char **argv, int x, int y);




extern GdkPixmap *XC_hand_source, *XC_hand_mask;
extern GdkPixmap *XC_lock_source, *XC_lock_mask;
extern GdkPixmap *XC_clock_source, *XC_clock_mask;

#endif /* __GHID_INCLUDED__  */
