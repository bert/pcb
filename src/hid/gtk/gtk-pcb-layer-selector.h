#ifndef GTK_PCB_LAYER_SELECTOR_H__
#define GTK_PCB_LAYER_SELECTOR_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS  /* keep c++ happy */

#define GTK_PCB_LAYER_SELECTOR_TYPE            (gtk_pcb_layer_selector_get_type ())
#define GTK_PCB_LAYER_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_PCB_LAYER_SELECTOR_TYPE, GtkPcbLayerSelector))
#define GTK_PCB_LAYER_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_PCB_LAYER_SELECTOR_TYPE, GtkPcbLayerSelectorClass))
#define IS_GTK_PCB_LAYER_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_PCB_LAYER_SELECTOR_TYPE))
#define IS_GTK_PCB_LAYER_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_PCB_LAYER_SELECTOR_TYPE))

typedef struct _GtkPcbLayerSelector       GtkPcbLayerSelector;
typedef struct _GtkPcbLayerSelectorClass  GtkPcbLayerSelectorClass;

GType gtk_pcb_layer_selector_get_type (void);
GtkWidget* gtk_pcb_layer_selector_new (void);

void gtk_pcb_layer_selector_add_layer (GtkPcbLayerSelector *ls,
                                       gint user_id,
                                       const gchar *name,
                                       const gchar *color_string,
                                       gboolean visible,
                                       gboolean activatable);
GtkAccelGroup *gtk_pcb_layer_selector_get_accel_group (GtkPcbLayerSelector *ls);
gchar *gtk_pcb_layer_selector_get_pick_xml (GtkPcbLayerSelector *ls);
gchar *gtk_pcb_layer_selector_get_view_xml (GtkPcbLayerSelector *ls);
GtkActionGroup *gtk_pcb_layer_selector_get_action_group (GtkPcbLayerSelector *ls);

void gtk_pcb_layer_selector_toggle_layer (GtkPcbLayerSelector *ls, 
                                          gint user_id);
void gtk_pcb_layer_selector_select_layer (GtkPcbLayerSelector *ls, 
                                          gint user_id);
gboolean gtk_pcb_layer_selector_select_next_visible (GtkPcbLayerSelector *ls);
void gtk_pcb_layer_selector_make_selected_visible (GtkPcbLayerSelector *ls);
void gtk_pcb_layer_selector_update_colors (GtkPcbLayerSelector *ls,
                                           const gchar *(*callback)(int user_id));
void gtk_pcb_layer_selector_delete_layers (GtkPcbLayerSelector *ls,
                                           gboolean (*callback)(int user_id));

G_END_DECLS  /* keep c++ happy */
#endif
