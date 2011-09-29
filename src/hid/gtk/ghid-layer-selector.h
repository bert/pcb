#ifndef GHID_LAYER_SELECTOR_H__
#define GHID_LAYER_SELECTOR_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS  /* keep c++ happy */

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

gint ghid_layer_selector_install_pick_items (GHidLayerSelector *ls,
                                             GtkMenuShell *shell, gint pos);
gint ghid_layer_selector_install_view_items (GHidLayerSelector *ls,
                                             GtkMenuShell *shell, gint pos);

GtkAccelGroup *ghid_layer_selector_get_accel_group (GHidLayerSelector *ls);

void ghid_layer_selector_toggle_layer (GHidLayerSelector *ls, 
                                       gint user_id);
void ghid_layer_selector_select_layer (GHidLayerSelector *ls, 
                                       gint user_id);
gboolean ghid_layer_selector_select_next_visible (GHidLayerSelector *ls);
void ghid_layer_selector_make_selected_visible (GHidLayerSelector *ls);
void ghid_layer_selector_update_colors (GHidLayerSelector *ls,
                                        const gchar *(*callback)(int user_id));
void ghid_layer_selector_delete_layers (GHidLayerSelector *ls,
                                        gboolean (*callback)(int user_id));
void ghid_layer_selector_show_layers (GHidLayerSelector *ls,
                                      gboolean (*callback)(int user_id));

G_END_DECLS  /* keep c++ happy */
#endif
