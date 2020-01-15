/*!
 * \file src/hid/gtk/ghid-route-style-selector.h
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

#ifndef GHID_ROUTE_STYLE_SELECTOR_H__
#define GHID_ROUTE_STYLE_SELECTOR_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "global.h"

G_BEGIN_DECLS  /* keep c++ happy */

#define GHID_ROUTE_STYLE_SELECTOR_TYPE            (ghid_route_style_selector_get_type ())
#define GHID_ROUTE_STYLE_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GHID_ROUTE_STYLE_SELECTOR_TYPE, GHidRouteStyleSelector))
#define GHID_ROUTE_STYLE_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GHID_ROUTE_STYLE_SELECTOR_TYPE, GHidRouteStyleSelectorClass))
#define IS_GHID_ROUTE_STYLE_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GHID_ROUTE_STYLE_SELECTOR_TYPE))
#define IS_GHID_ROUTE_STYLE_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GHID_ROUTE_STYLE_SELECTOR_TYPE))

typedef struct _GHidRouteStyleSelector       GHidRouteStyleSelector;
typedef struct _GHidRouteStyleSelectorClass  GHidRouteStyleSelectorClass;

GType ghid_route_style_selector_get_type (void);
GtkWidget* ghid_route_style_selector_new (void);

gint ghid_route_style_selector_install_items (GHidRouteStyleSelector *rss,
                                              GtkMenuShell *shell, gint pos);

void ghid_route_style_selector_add_route_style (GHidRouteStyleSelector *rss,
                                                RouteStyleType *data);
gboolean ghid_route_style_selector_select_style (GHidRouteStyleSelector *rss,
                                                  RouteStyleType *rst);
void ghid_route_style_selector_edit_dialog (GHidRouteStyleSelector *rss);

GtkAccelGroup *ghid_route_style_selector_get_accel_group
                 (GHidRouteStyleSelector *rss);

void ghid_route_style_selector_sync (GHidRouteStyleSelector *rss,
                                     Coord Thick, Coord Hole,
                                     Coord Diameter, Coord Keepaway, Coord ViaMask);
void ghid_route_style_selector_empty (GHidRouteStyleSelector *rss);

G_END_DECLS  /* keep c++ happy */
#endif
