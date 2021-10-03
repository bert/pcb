/*!
 * \file src/hid/gtk3/gtk3-layer-selector.h
 *
 * \brief Layer selector widget.
 *
 * \note This file is copied from the PCB GTK-2 port and modified to
 * comply with GTK-3.
 *
 * \copyright (C) 2021 PCB Contributors.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996, 2004 Thomas Nau
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 */


#ifndef PCB_SRC_HID_GTK3_GTK3_LAYER_SELECTOR_H__
#define PCB_SRC_HID_GTK3_GTK3_LAYER_SELECTOR_H__


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>


G_BEGIN_DECLS  /* Keep C++ happy. */


#define GHID_LAYER_SELECTOR_TYPE            (ghid_layer_selector_get_type ())
#define GHID_LAYER_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GHID_LAYER_SELECTOR_TYPE, GHidLayerSelector))
#define GHID_LAYER_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GHID_LAYER_SELECTOR_TYPE, GHidLayerSelectorClass))
#define IS_GHID_LAYER_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GHID_LAYER_SELECTOR_TYPE))
#define IS_GHID_LAYER_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GHID_LAYER_SELECTOR_TYPE))


typedef struct _GHidLayerSelector       GHidLayerSelector;
typedef struct _GHidLayerSelectorClass  GHidLayerSelectorClass;


GType ghid_layer_selector_get_type (void);
GtkWidget* ghid_layer_selector_new (void);

void ghid_layer_selector_add_layer (GHidLayerSelector *ls,
                                    gint user_id,
                                    const gchar *name,
                                    const gchar *color_string,
                                    gboolean visible,
                                    gboolean selectable,
                                    gboolean renameable);

gint ghid_layer_selector_install_pick_items (GHidLayerSelector *ls, GtkMenuShell *shell, gint pos);
gint ghid_layer_selector_install_view_items (GHidLayerSelector *ls, GtkMenuShell *shell, gint pos);

GtkAccelGroup *ghid_layer_selector_get_accel_group (GHidLayerSelector *ls);

void ghid_layer_selector_toggle_layer (GHidLayerSelector *ls, gint user_id);
void ghid_layer_selector_select_layer (GHidLayerSelector *ls, gint user_id);
gboolean ghid_layer_selector_select_next_visible (GHidLayerSelector *ls);
void ghid_layer_selector_make_selected_visible (GHidLayerSelector *ls);
void ghid_layer_selector_update_colors (GHidLayerSelector *ls, const gchar *(*callback)(int user_id));
void ghid_layer_selector_delete_layers (GHidLayerSelector *ls, gboolean (*callback)(int user_id));
void ghid_layer_selector_show_layers (GHidLayerSelector *ls, gboolean (*callback)(int user_id));


G_END_DECLS  /* Keep C++ happy */


#endif /* PCB_SRC_HID_GTK3_GTK3_LAYER_SELECTOR_H__ */
