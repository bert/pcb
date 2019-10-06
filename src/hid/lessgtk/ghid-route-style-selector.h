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
