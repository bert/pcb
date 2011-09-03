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
