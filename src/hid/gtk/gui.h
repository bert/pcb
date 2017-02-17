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

#ifndef PCB_HID_GTK_GHID_H
#define PCB_HID_GTK_GHID_H

#include <locale.h>

#include "global.h"
#include "hid.h"
#include "hid_draw.h"
#include "hid/common/hid_resource.h"

#include "data.h"
#include "misc.h"
#include <sys/stat.h>

#include <gtk/gtk.h>
#include "ghid-coord-entry.h"
#include "ghid-main-menu.h"
#include "gui-pinout-preview.h"


  /* Silk and rats lines are the two additional selectable to draw on.
     |  gui code in gui-top-window.c and group code in misc.c must agree
     |  on what layer is what!
   */
#define LAYER_BUTTON_SILK     (MAX_LAYER)
#define LAYER_BUTTON_RATS     (MAX_LAYER + 1)
#define	N_SELECTABLE_LAYER_BUTTONS	(LAYER_BUTTON_RATS + 1)

#define LAYER_BUTTON_PINS     (MAX_ALL_LAYER)
#define LAYER_BUTTON_VIAS     (MAX_ALL_LAYER + 1)
#define LAYER_BUTTON_FARSIDE  (MAX_ALL_LAYER + 2)
#define LAYER_BUTTON_MASK     (MAX_ALL_LAYER + 3)
#define N_LAYER_BUTTONS       (MAX_ALL_LAYER + 4)

  /* Go from from the grid units in use (millimeters or mils) to PCB units
     |  and back again.
     |  PCB keeps values internally higher precision, but gui
     |  widgets (spin buttons, labels, etc) need mils or millimeters.
   */
#define	FROM_PCB_UNITS(v)	coord_to_unit (Settings.grid_unit, v)
#define	TO_PCB_UNITS(v)		unit_to_coord (Settings.grid_unit, v)

#define SIDE_X(x)         ((gport->view.flip_x ? PCB->MaxWidth - (x) : (x)))
#define SIDE_Y(y)         ((gport->view.flip_y ? PCB->MaxHeight - (y) : (y)))

#define	DRAW_X(x)         (gint)((SIDE_X(x) - gport->view.x0) / gport->view.coord_per_px)
#define	DRAW_Y(y)         (gint)((SIDE_Y(y) - gport->view.y0) / gport->view.coord_per_px)

#define	EVENT_TO_PCB_X(x) SIDE_X((gint)((x) * gport->view.coord_per_px + gport->view.x0))
#define	EVENT_TO_PCB_Y(y) SIDE_Y((gint)((y) * gport->view.coord_per_px + gport->view.y0))

/*
 * Used to intercept "special" hotkeys that gtk doesn't usually pass
 * on to the menu hotkeys.  We catch them and put them back where we
 * want them. 
 */

/* The modifier keys */

 #define GHID_KEY_ALT     0x80
 #define GHID_KEY_CONTROL 0x40
 #define GHID_KEY_SHIFT   0x20

/* The actual keys */
#define GHID_KEY_TAB      0x01
#define GHID_KEY_UP       0x02
#define GHID_KEY_DOWN     0x03
#define GHID_KEY_LEFT     0x04
#define GHID_KEY_RIGHT    0x05

typedef struct
{
  GtkActionGroup *main_actions,
    *change_selected_actions, *displayed_name_actions;

  GtkWidget
    *status_line_label,
    *cursor_position_relative_label,
    *cursor_position_absolute_label,
    *grid_units_label, *status_line_hbox, *command_combo_box;
  GtkEntry *command_entry;

  GtkWidget *top_hbox,
    *top_bar_background,
    *menu_hbox, *position_hbox,
    *menubar_toolbar_vbox,
    *mode_buttons_frame;
  GtkWidget *left_toolbar;
  GtkWidget *grid_units_button;
  GtkWidget *menu_bar, *layer_selector, *route_style_selector;
  GtkWidget *mode_toolbar;
  GtkWidget *vbox_middle;

  GtkWidget *info_bar;
  GTimeVal our_mtime;
  GTimeVal last_seen_mtime;

  GtkWidget *h_range, *v_range;
  GtkObject *h_adjustment, *v_adjustment;

  GdkPixbuf *bg_pixbuf;

  gchar *name_label_string;

  gboolean adjustment_changed_holdoff,
    command_entry_status_line_active,
    in_popup;

  gboolean config_modified,
    small_label_markup,
    compact_horizontal,
    compact_vertical,
    use_command_window,
    creating;

  gint n_mode_button_columns,
    top_window_width,
    top_window_height,
    log_window_width,
    log_window_height,
    drc_window_width,
    drc_window_height,
    keyref_window_width,
    keyref_window_height,
    library_window_width,
    library_window_height,
    netlist_window_width,
    netlist_window_height,
    history_size,
    settings_mode;

  bool is_up;
}
GhidGui;

extern GhidGui _ghidgui, *ghidgui;

typedef struct
{
  double coord_per_px; /* Zoom level described as PCB units per screen pixel */

  Coord x0;
  Coord y0;
  Coord width;
  Coord height;

  bool flip_x;
  bool flip_y;

} view_data;

  /* The output viewport
   */
typedef struct
{
  GtkWidget *top_window,	/* toplevel widget              */
   *drawing_area;		/* and its drawing area */
  GdkPixmap *pixmap, *mask;
  GdkDrawable *drawable;	/* Current drawable for drawing routines */
  gint width, height;

  struct render_priv *render_priv;

  GdkColor bg_color, offlimits_color, grid_color;

  GdkColormap *colormap;

  GdkCursor *X_cursor;		/* used X cursor */
  GdkCursorType X_cursor_shape;	/* and its shape */

  gboolean has_entered;
  gboolean panning;

  view_data view;
  Coord pcb_x, pcb_y;             /* PCB coordinates of the mouse pointer */
  Coord crosshair_x, crosshair_y; /* PCB coordinates of the crosshair     */

  guint tooltip_update_timeout_id;
}
GHidPort;

extern GHidPort ghid_port, *gport;

typedef enum
{
  NONE_PRESSED               = 0,
  SHIFT_PRESSED              = M_Shift,
  CONTROL_PRESSED            = M_Ctrl,
  MOD1_PRESSED               = M_Mod(1),
  SHIFT_CONTROL_PRESSED      = M_Shift | M_Ctrl,
  SHIFT_MOD1_PRESSED         = M_Shift | M_Mod(1),
  CONTROL_MOD1_PRESSED       = M_Ctrl | M_Mod(1),
  SHIFT_CONTROL_MOD1_PRESSED = M_Shift | M_Ctrl | M_Mod(1),
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

/* Function prototypes
*/
void ghid_parse_arguments (gint * argc, gchar *** argv);
void ghid_do_export (HID_Attr_Val * options);

void ghid_create_pcb_widgets (void);
void ghid_window_set_name_label (gchar * name);
void ghid_interface_set_sensitive (gboolean sensitive);
void ghid_interface_input_signals_connect (void);
void ghid_interface_input_signals_disconnect (void);

void ghid_pcb_saved_toggle_states_set (void);
void ghid_sync_with_new_layout (void);

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
void ghid_layer_buttons_update (void);
void ghid_layer_buttons_color_update (void);


/* gui-misc.c function prototypes
*/
void ghid_status_line_set_text (const gchar * text);
void ghid_cursor_position_label_set_text (gchar * text);
void ghid_cursor_position_relative_label_set_text (gchar * text);

void ghid_hand_cursor (void);
void ghid_point_cursor (void);
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
void ghid_port_ranges_scale (void);

gboolean ghid_note_event_location (GdkEventButton * ev);
gboolean ghid_port_key_press_cb (GtkWidget * drawing_area,
				 GdkEventKey * kev, gpointer data);
gboolean ghid_port_key_release_cb (GtkWidget * drawing_area,
				   GdkEventKey * kev, gpointer data);
gboolean ghid_port_button_press_cb (GtkWidget * drawing_area,
				    GdkEventButton * ev, gpointer data);
gboolean ghid_port_button_release_cb (GtkWidget * drawing_area,
				      GdkEventButton * ev, gpointer data);


gint ghid_port_window_enter_cb (GtkWidget * widget,
			   GdkEventCrossing * ev, GHidPort * out);
gint ghid_port_window_leave_cb (GtkWidget * widget, 
                           GdkEventCrossing * ev, GHidPort * out);
gint ghid_port_window_motion_cb (GtkWidget * widget,
				 GdkEventMotion * ev, GHidPort * out);
gint ghid_port_window_mouse_scroll_cb (GtkWidget * widget,
				       GdkEventScroll * ev, GHidPort * out);

gint ghid_port_drawing_area_configure_event_cb (GtkWidget * widget,
						GdkEventConfigure * ev,
						GHidPort * out);


/* gui-dialog.c function prototypes.
*/
#define		GUI_DIALOG_RESPONSE_ALL	1

gchar *ghid_dialog_file_select_open (gchar * title, gchar ** path,
				     gchar * shortcuts);
GSList *ghid_dialog_file_select_multiple (gchar * title, gchar ** path,
				          gchar * shortcuts);
gchar *ghid_dialog_file_select_save (gchar * title, gchar ** path,
				     gchar * file, gchar * shortcuts);
void ghid_dialog_message (gchar * message);
gboolean ghid_dialog_confirm (gchar * message, gchar *cancelmsg, gchar *okmsg);
int ghid_dialog_close_confirm (void);
#define GUI_DIALOG_CLOSE_CONFIRM_CANCEL 0
#define GUI_DIALOG_CLOSE_CONFIRM_NOSAVE 1
#define GUI_DIALOG_CLOSE_CONFIRM_SAVE   2
gint ghid_dialog_confirm_all (gchar * message);
gchar *ghid_dialog_input (const char * prompt, const char * initial);
void ghid_dialog_about (void);

char * ghid_fileselect (const char *, const char *, char *, char *, const char *, int);


/* gui-dialog-print.c */
void ghid_dialog_export (void);
void ghid_dialog_print (HID *);

int ghid_attribute_dialog (HID_Attribute *, int, HID_Attr_Val *, const char *, const char *);

/* gui-drc-window.c */
void ghid_drc_window_show (gboolean raise);
void ghid_drc_window_reset_message (void);
void ghid_drc_window_append_violation (DrcViolationType *violation);
void ghid_drc_window_append_messagev (const char *fmt, va_list va);
int ghid_drc_window_throw_dialog (void);

/* In gui-top-window.c  */
void ghid_update_toggle_flags (void);
void ghid_notify_save_pcb (const char *file, bool done);
void ghid_notify_filename_changed (void);
void ghid_install_accel_groups (GtkWindow *window, GhidGui *gui);
void ghid_remove_accel_groups (GtkWindow *window, GhidGui *gui);
void make_route_style_buttons (GHidRouteStyleSelector *rss);
void layer_process (gchar **color_string, char **text, int *set, int i);

/* gui-utils.c
*/
gboolean dup_string (gchar ** dst, const gchar * src);
void free_glist_and_data (GList ** list_head);

ModifierKeysState ghid_modifier_keys_state (GdkModifierType *state);
ButtonState ghid_button_state (GdkModifierType *state);
gboolean ghid_is_modifier_key_sym (gint ksym);
gboolean ghid_control_is_pressed (void);
gboolean ghid_mod1_is_pressed (void);
gboolean ghid_shift_is_pressed (void);

void ghid_draw_area_update (GHidPort * out, GdkRectangle * rect);
gchar *ghid_get_color_name (GdkColor * color);
void ghid_map_color_string (gchar * color_string, GdkColor * color);
gchar *ghid_entry_get_text (GtkWidget * entry);
void ghid_check_button_connected (GtkWidget * box, GtkWidget ** button,
				  gboolean active, gboolean pack_start,
				  gboolean expand, gboolean fill, gint pad,
				  void (*cb_func) (GtkToggleButton *, gpointer), gpointer data,
				  gchar * string);
void ghid_button_connected (GtkWidget * box, GtkWidget ** button,
			    gboolean pack_start, gboolean expand,
			    gboolean fill, gint pad, void (*cb_func) (gpointer),
			    gpointer data, gchar * string);
void ghid_coord_entry (GtkWidget * box, GtkWidget ** coord_entry, Coord value,
		       Coord low, Coord high,  enum ce_step_size step_size,
		       gint width, void (*cb_func) (GHidCoordEntry *, gpointer),
		       gpointer data, gboolean right_align, gchar * string);
void ghid_spin_button (GtkWidget * box, GtkWidget ** spin_button,
		       gfloat value, gfloat low, gfloat high, gfloat step0,
		       gfloat step1, gint digits, gint width,
		       void (*cb_func) (GtkSpinButton *, gpointer), gpointer data,
		       gboolean right_align, gchar * string);
void ghid_table_coord_entry (GtkWidget * table, gint row, gint column,
		 	    GtkWidget ** coord_entry, Coord value,
			    Coord low, Coord high, enum ce_step_size, gint width,
			    void (*cb_func) (GHidCoordEntry *, gpointer), gpointer data,
			    gboolean right_align, gchar * string);
void ghid_table_spin_button (GtkWidget * box, gint row, gint column,
			     GtkWidget ** spin_button, gfloat value,
			     gfloat low, gfloat high, gfloat step0,
			     gfloat step1, gint digits, gint width,
			     void (*cb_func) (GtkSpinButton *, gpointer), gpointer data,
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
GtkWidget *ghid_notebook_page (GtkWidget * tabs, const char *name, gint pad,
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
					   void (*func_cb) (GtkTreeSelection *, gpointer), gpointer data);

void ghid_dialog_report (gchar * title, gchar * message);
void ghid_label_set_markup (GtkWidget * label, const gchar * text);

void ghid_set_cursor_position_labels (void);
void ghid_set_status_line_label (void);


/* gui-netlist-window.c */
void ghid_netlist_window_create (GHidPort * out);
void ghid_netlist_window_show (GHidPort * out, gboolean raise);
void ghid_netlist_window_update (gboolean init_nodes);

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
void ghid_library_window_create (GHidPort * out);
void ghid_library_window_show (GHidPort * out, gboolean raise);


/* gui-log-window.c */
void ghid_log_window_create ();
void ghid_log_window_show (gboolean raise);
void ghid_log (const char *fmt, ...);
void ghid_logv (const char *fmt, va_list args);

/* gui-pinout-window.c */
void ghid_pinout_window_show (GHidPort *out, ElementType *Element);

/* gtkhid-gdk.c AND gtkhid-gl.c */
int ghid_set_layer (const char *name, int group, int empty);
hidGC ghid_make_gc (void);
void ghid_destroy_gc (hidGC);
void ghid_use_mask (enum mask_mode mode);
void ghid_set_color (hidGC gc, const char *name);
void ghid_set_line_cap (hidGC gc, EndCapStyle style);
void ghid_set_line_width (hidGC gc, Coord width);
void ghid_set_draw_xor (hidGC gc, int _xor);
void ghid_draw_grid(BoxType * region);
void ghid_draw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);
void ghid_draw_arc (hidGC gc, Coord cx, Coord cy, Coord xradius, Coord yradius,
                    Angle start_angle, Angle delta_angle);
void ghid_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);
void ghid_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius);
void ghid_fill_polygon (hidGC gc, int n_coords, Coord *x, Coord *y);
void ghid_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);
void ghid_invalidate_lr (int left, int right, int top, int bottom);
void ghid_invalidate_all ();
void ghid_notify_crosshair_change (bool changes_complete);
void ghid_notify_mark_change (bool changes_complete);
void ghid_init_renderer (int *, char ***, GHidPort *);
void ghid_shutdown_renderer (GHidPort *);
void ghid_init_drawing_widget (GtkWidget *widget, GHidPort *);
void ghid_drawing_area_configure_hook (GHidPort *port);
void ghid_screen_update (void);
gboolean ghid_drawing_area_expose_cb (GtkWidget *, GdkEventExpose *,
                                      GHidPort *);
void ghid_port_drawing_realize_cb (GtkWidget *, gpointer);
gboolean ghid_pinout_preview_expose (GtkWidget * widget, GdkEventExpose * ev);
GdkPixmap *ghid_render_pixmap (int cx, int cy, double zoom,
                               int width, int height, int depth);
HID_DRAW *ghid_request_debug_draw (void);
void ghid_flush_debug_draw (void);
void ghid_finish_debug_draw (void);
bool ghid_event_to_pcb_coords (int event_x, int event_y, Coord *pcb_x, Coord *pcb_y);
bool ghid_pcb_to_event_coords (Coord pcb_x, Coord pcb_y, int *event_x, int *event_y);
void ghid_port_rotate (void *ball, float *quarternion, gpointer userdata);
void ghid_view_2d (void *ball, gboolean view_2d, gpointer userdata);

void ghid_lead_user_to_location (Coord x, Coord y);
void ghid_cancel_lead_user (void);

/* gtkhid-main.c */
void ghid_pan_view_rel (Coord dx, Coord dy);
void ghid_get_coords (const char *msg, Coord *x, Coord *y);
gint PCBChanged (int argc, char **argv, Coord x, Coord y);




extern GdkPixmap *XC_hand_source, *XC_hand_mask;
extern GdkPixmap *XC_lock_source, *XC_lock_mask;
extern GdkPixmap *XC_clock_source, *XC_clock_mask;


/* Coordinate conversions */
/* Px converts view->pcb, Vx converts pcb->view */
static inline int
Vx (Coord x)
{
  int rv;
  if (gport->view.flip_x)
    rv = (PCB->MaxWidth - x - gport->view.x0) / gport->view.coord_per_px + 0.5;
  else
    rv = (x - gport->view.x0) / gport->view.coord_per_px + 0.5;
  return rv;
}

static inline int
Vy (Coord y)
{
  int rv;
  if (gport->view.flip_y)
    rv = (PCB->MaxHeight - y - gport->view.y0) / gport->view.coord_per_px + 0.5;
  else
    rv = (y - gport->view.y0) / gport->view.coord_per_px + 0.5;
  return rv;
}

static inline int
Vz (Coord z)
{
  return z / gport->view.coord_per_px + 0.5;
}

static inline Coord
Px (int x)
{
  Coord rv = x * gport->view.coord_per_px + gport->view.x0;
  if (gport->view.flip_x)
    rv = PCB->MaxWidth - (x * gport->view.coord_per_px + gport->view.x0);
  return  rv;
}

static inline Coord
Py (int y)
{
  Coord rv = y * gport->view.coord_per_px + gport->view.y0;
  if (gport->view.flip_y)
    rv = PCB->MaxHeight - (y * gport->view.coord_per_px + gport->view.y0);
  return  rv;
}

static inline Coord
Pz (int z)
{
  return (z * gport->view.coord_per_px);
}

#endif /* PCB_HID_GTK_GHID_H */
