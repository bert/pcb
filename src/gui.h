/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *  This module, gui-gtk.h, was written and is Copyright (C) 2004
 *  by Bill Wilson.  The functions here are utility aids and are from my
 *  other projects: gkrellm and gstocks.
 *  This module is also subject to the GNU GPL as described below.
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

#ifndef __GUI_GTK_INCLUDED__
#define __GUI_GTK_INCLUDED__

#include "data.h"
#include "misc.h"
#include <sys/stat.h>


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
#   endif   /* gettext_noop */
#else
#   define _(String) (String)
#   define N_(String) (String)
#   define textdomain(String) (String)
#   define gettext(String) (String)
#   define dgettext(Domain,String) (String)
#   define dcgettext(Domain,String,Type) (String)
#   define bindtextdomain(Domain,Directory) (Domain)
#endif  /* ENABLE_NLS */


  /* Silk and rats lines are the two additional selectable to draw on.
  |  gui code in gui-top-window.c and group code in misc.c must agree
  |  on what layer is what!
  */
#define	GUI_SILK_LAYER			MAX_LAYER
#define	GUI_RATS_LAYER			(MAX_LAYER + 1)
#define	GUI_N_SELECTABLE_LAYERS	(MAX_LAYER + 2)


  /* Go from from the grid units in use (millimeters or mils) to PCB units
  |  and back again.
  |  PCB keeps values internally to 1/100000 inch (0.01 mils), but gui
  |  widgets (spin buttons, labels, etc) need mils or millimeters.
  */
#define	FROM_PCB_UNITS(v)	(Settings.grid_units_mm ? \
								((v) * 0.000254) : ((v) * 0.01))

#define TO_PCB_UNITS(v)		(Settings.grid_units_mm ? \
								((v) / 0.000254) : ((v) / 0.01))

  /* Pick one of two values depending on current grid units setting.
  */
#define GRID_UNITS_VALUE(mm, mil)   (Settings.grid_units_mm ? (mm) : (mil))



typedef struct
	{
	GtkUIManager	*ui_manager;
	GtkActionGroup	*main_actions,
					*grid_actions,
					*change_selected_actions,
					*displayed_name_actions;

	GtkWidget		*name_label,
					*status_line_label,
					*cursor_position_relative_label,
					*cursor_position_absolute_label,
					*cursor_units_label,
					*status_line_hbox,
					*command_combo_box;
	GtkEntry		*command_entry;

	GtkWidget		*top_hbox,
					*menu_hbox,
					*compact_vbox,
					*compact_hbox,
					*position_hbox;

	GtkWidget		*h_scrollbar,
					*v_scrollbar;
	GtkObject		*h_adjustment,
					*v_adjustment;

	gboolean		adjustment_changed_holdoff,
					toggle_holdoff,
					command_entry_status_line_active;
	}
	GuiPCB;

extern GuiPCB	*gui;


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

/* Function prototypes
*/
void 		gui_init(gint *argc, gchar ***argv);
void		gui_create_pcb_widgets(void);
void		gui_output_set_name_label(gchar *name);
void		gui_interface_set_sensitive(gboolean sensitive);
void		gui_interface_input_signals_connect(void);
void		gui_interface_input_signals_disconnect(void);

void		gui_pcb_saved_toggle_states_set(void);
void		gui_sync_with_new_layout(void);
void		gui_size_increment_get_value(const gchar *saction,
				gchar **value, gchar **units);
void		gui_line_increment_get_value(const gchar *saction,
				gchar **value, gchar **units);
void		gui_clear_increment_get_value(const gchar *saction,
				gchar **value, gchar **units);
void		gui_grid_size_increment_get_value(const gchar *saction,
				gchar **value, gchar **units);

void		gui_grid_setting_update_menu_actions(void);
void		gui_change_selected_update_menu_actions(void);

void		gui_config_window_show();
void		gui_config_handle_units_changed(void);
void		gui_config_start_backup_timer(void);
void		gui_config_text_scale_update(void);
void		gui_config_layer_name_update(gchar *name, gint layer);

void		config_file_write(void);
void		config_settings_load(void);

void		gui_mode_buttons_update(void);
void		gui_layer_enable_buttons_update(void);
void		gui_layer_buttons_update(void);
void		gui_layer_buttons_color_update(void);
void		gui_zoom_display_update(void);

/* gui-misc.c function prototypes
*/
void		gui_status_line_set_text(gchar *text);
void		gui_cursor_position_label_set_text(gchar *text);
void		gui_cursor_position_relative_label_set_text(gchar *text);

void		gui_hand_cursor(void);
void		gui_watch_cursor(void);
void		gui_mode_cursor(gint mode);
void		gui_corner_cursor(void);
void		gui_restore_cursor(void);
void		gui_create_abort_dialog(gchar *);
gboolean	gui_check_abort(void);
void		gui_end_abort(void);
void		gui_get_pointer(gint *, gint *);
void		gui_beep(gint);


/* gui-output.c function prototypes.
*/
void		gui_output_positioners_scale(void);
void		gui_output_positioners_set_values(gint x_value, gint y_value);
void		gui_output_positioners_update_ranges(void);

gboolean	gui_output_key_press_cb(GtkWidget *drawing_area,
						GdkEventKey *kev, GtkUIManager *ui);
gboolean	gui_output_key_release_cb(GtkWidget *drawing_area,
						GdkEventKey *kev, GtkUIManager *ui);
gboolean	gui_output_button_press_cb(GtkWidget *drawing_area,
						GdkEventButton *ev, GtkUIManager *ui);
gboolean	gui_output_button_release_cb(GtkWidget *drawing_area,
						GdkEventButton *ev, GtkUIManager *ui);


gint		gui_output_window_enter_cb(GtkWidget *widget,
						GdkEventButton *ev, OutputType *out);
gint		gui_output_window_leave_cb(GtkWidget *widget,
						GdkEventButton *ev, OutputType *out);
gint		gui_output_window_motion_cb(GtkWidget *widget,
						GdkEventButton *ev, OutputType *out);
gint		gui_output_window_mouse_scroll_cb(GtkWidget *widget,
						GdkEventScroll *ev, OutputType *out);


gint		gui_output_drawing_area_expose_event_cb(GtkWidget *widget,
						GdkEventExpose *ev, OutputType *out);
gint		gui_output_drawing_area_configure_event_cb(GtkWidget *widget,
						GdkEventConfigure *ev, OutputType *out);

void		gui_output_stop_scroll_cb(GtkWidget *widget,
						GdkEventButton *ev, OutputType *out);


/* gui-dialog.c function prototypes.
*/
#define		GUI_DIALOG_RESPONSE_ALL	1

gchar		*gui_dialog_file_select_open(gchar *title, gchar **path,
							gchar *shortcuts);
gchar		*gui_dialog_file_select_save(gchar *title, gchar **path,
							gchar *file, gchar *shortcuts);
void		gui_dialog_message(gchar *message);
gboolean	gui_dialog_confirm(gchar *message);
gint		gui_dialog_confirm_all(gchar *message);
gchar		*gui_dialog_input(gchar *prompt, gchar *initial);
void		gui_dialog_about(void);


/* gui-dialog-print.c */
void	gui_dialog_print(void);


/* gui-route-style function prototypes.
*/
  /* In gui-dialog-size.c */
void		gui_route_style_dialog(gint index, RouteStyleType *rst);
  /* In gui-top-window.c  */
void		gui_route_style_set_button_label(gchar *name, gint index);
void		gui_route_style_set_temp_style(RouteStyleType *rst, gint which);
void		gui_route_style_button_set_active(gint number);
void		gui_route_style_buttons_update(void);

/* gui-utils.c
*/
gboolean	dup_string(gchar **dst, gchar *src);
gboolean	utf8_dup_string(gchar **dst_utf8, gchar *src);

void		free_glist_and_data(GList **list_head);

ModifierKeysState
			gui_modifier_keys_state(void);
gboolean	gui_is_modifier_key_sym(gint ksym);
gboolean	gui_control_is_pressed(void);
gboolean	gui_shift_is_pressed(void);

void		gui_draw_area_update(OutputType *out, GdkRectangle *rect);
void		gui_string_markup_extents(PangoFontDescription *font_desc,
					gchar *string, gint *width, gint *height);
void		gui_draw_string_markup(GdkDrawable *drawable,
					PangoFontDescription *font_desc,
					GdkGC *gc, gint x, gint y, gchar *string);

gchar		*gui_get_color_name(GdkColor *color);
void		gui_map_color_string(gchar *color_string, GdkColor *color);
void		gui_button_set_text(GtkWidget *button, gchar *text);
gboolean	gui_button_active(GtkWidget *widget);
gchar		*gui_entry_get_text(GtkWidget *entry);
void		gui_check_button_connected(GtkWidget *box, GtkWidget **button,
					gboolean active, gboolean pack_start, gboolean expand,
					gboolean fill, gint pad,
            		void (*cb_func)(), gpointer data, gchar *string);
void		gui_button_connected(GtkWidget *box, GtkWidget **button,
					gboolean pack_start, gboolean expand,
					gboolean fill, gint pad,
					void (*cb_func)(), gpointer data, gchar *string);
void		gui_spin_button(GtkWidget *box, GtkWidget **spin_button,
					gfloat value, gfloat low, gfloat high,
					gfloat step0, gfloat step1,
					gint digits, gint width,
					void (*cb_func)(), gpointer data,
					gboolean right_align, gchar *string);
void		gui_table_spin_button(GtkWidget *box, gint row, gint column,
					GtkWidget **spin_button,
					gfloat value, gfloat low, gfloat high,
					gfloat step0, gfloat step1,
					gint digits, gint width,
					void (*cb_func)(), gpointer data,
					gboolean right_align, gchar *string);
void		gui_range_control(GtkWidget *box, GtkWidget **scale_res,
					gboolean horizontal, GtkPositionType pos,
					gboolean set_draw_value, gint digits,
        			gboolean pack_start, gboolean expand, gboolean fill,
					guint pad, gfloat value, gfloat low, gfloat high,
					gfloat step0, gfloat step1,
					void (*cb_func)(), gpointer data);
GtkWidget	*gui_scrolled_vbox(GtkWidget *box, GtkWidget **scr,
					GtkPolicyType h_policy, GtkPolicyType v_policy);
GtkWidget	*gui_framed_vbox(GtkWidget *box, gchar *label,
					gint frame_border_width,
					gboolean frame_expand, gint vbox_pad,
					gint vbox_border_width);
GtkWidget	*gui_framed_vbox_end(GtkWidget *box, gchar *label,
					gint frame_border_width, gboolean frame_expand,
					gint vbox_pad, gint vbox_border_width);
GtkWidget	*gui_category_vbox(GtkWidget *box, gchar *category_header,
					gint header_pad, gint box_pad,
					gboolean pack_start, gboolean bottom_pad);
GtkWidget	*gui_notebook_page(GtkWidget *tabs, char *name, gint pad,
					gint border);
GtkWidget	*gui_framed_notebook_page(GtkWidget *tabs, char *name,
					gint border, gint frame_border,
					gint vbox_pad, gint vbox_border);
GtkWidget	*gui_scrolled_text_view(GtkWidget *box, GtkWidget **scr,
					GtkPolicyType h_policy, GtkPolicyType v_policy);
void		gui_text_view_append(GtkWidget *view, gchar *string);
void		gui_text_view_append_strings(GtkWidget *view, gchar **string,
					gint n_strings);
GtkTreeSelection *gui_scrolled_selection(GtkTreeView *treeview, GtkWidget *box,
					GtkSelectionMode s_mode,
					GtkPolicyType h_policy, GtkPolicyType v_policy,
					void (*func_cb)(), gpointer data);

void		gui_dialog_report(gchar *title, gchar *message);
void		gui_label_set_markup(GtkWidget *label, gchar *text);


/* gui-netlist-window.c */
void	gui_netlist_window_show(OutputType *out);
void	gui_netlist_window_update(gboolean init_nodes);
void	gui_netlist_nodes_update(LibraryMenuType *net);

LibraryMenuType *gui_get_net_from_node_name(gchar *name, gboolean);
void	gui_netlist_highlight_node(gchar *name, gboolean change_style);


/* gui-command-window.c */
void	gui_command_window_show(void);
gchar	*gui_command_entry_get(gchar *prompt, gchar *command);
void	gui_command_use_command_window_sync(void);

/* gui-keyref-window.c */
void	gui_keyref_window_show(void);

/* gui-library-window.c */
void    gui_library_window_show(OutputType *out);


/* gui-log-window.c */
void	gui_log_window_show(void);
void	gui_log_append_string(gchar *string);


/* gui-pinout-window.c */
void	gui_pinout_window_show(OutputType *out, ElementTypePtr Element);


#endif			/* __GUI_GTK_INCLUDED__  */


