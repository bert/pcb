/*!
 * \file src/hid/gtk/ghid-main-menu.h
 *
 * \brief .
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

#ifndef GHID_MAIN_MENU_H__
#define GHID_MAIN_MENU_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "ghid-layer-selector.h"
#include "ghid-route-style-selector.h"
#include "resource.h"

G_BEGIN_DECLS  /* keep c++ happy */

#define GHID_MAIN_MENU_TYPE            (ghid_main_menu_get_type ())
#define GHID_MAIN_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GHID_MAIN_MENU_TYPE, GHidMainMenu))
#define GHID_MAIN_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GHID_MAIN_MENU_TYPE, GHidMainMenuClass))
#define IS_GHID_MAIN_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GHID_MAIN_MENU_TYPE))
#define IS_GHID_MAIN_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GHID_MAIN_MENU_TYPE))

typedef struct _GHidMainMenu       GHidMainMenu;
typedef struct _GHidMainMenuClass  GHidMainMenuClass;

GType ghid_main_menu_get_type (void);
GtkWidget *ghid_main_menu_new (GCallback action_cb,
                               void (*special_key_cb) (const char *accel,
                                                       GtkAction *action,
                                                       const Resource *node));
void ghid_main_menu_add_resource (GHidMainMenu *menu, const Resource *res);
GtkAccelGroup *ghid_main_menu_get_accel_group (GHidMainMenu *menu);
void ghid_main_menu_update_toggle_state (GHidMainMenu *menu,
                                         void (*cb) (GtkAction *,
                                                     const char *toggle_flag,
                                                     const char *active_flag));

void ghid_main_menu_add_popup_resource (GHidMainMenu *menu, const char *name,
                                        const Resource *res);
GtkMenu *ghid_main_menu_get_popup (GHidMainMenu *menu, const char *name);

void ghid_main_menu_install_layer_selector (GHidMainMenu *mm,
                                            GHidLayerSelector *ls);
void ghid_main_menu_install_route_style_selector (GHidMainMenu *mm,
                                                  GHidRouteStyleSelector *rss);

G_END_DECLS  /* keep c++ happy */
#endif
