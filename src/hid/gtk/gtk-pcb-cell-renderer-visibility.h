#ifndef GTK_PCB_CELL_RENDERER_VISIBILITY_H__
#define GTK_PCB_CELL_RENDERER_VISIBILITY_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS  /* keep c++ happy */

#define VISIBILITY_TOGGLE_SIZE	16

#define GTK_PCB_CELL_RENDERER_VISIBILITY_TYPE            (gtk_pcb_cell_renderer_visibility_get_type ())
#define GTK_PCB_CELL_RENDERER_VISIBILITY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_PCB_CELL_RENDERER_VISIBILITY_TYPE, GtkPcbCellRendererVisibility))
#define GTK_PCB_CELL_RENDERER_VISIBILITY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_PCB_CELL_RENDERER_VISIBILITY_TYPE, GtkPcbCellRendererVisibilityClass))
#define IS_GTK_PCB_CELL_RENDERER_VISIBILITY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_PCB_CELL_RENDERER_VISIBILITY_TYPE))
#define IS_GTK_PCB_CELL_RENDERER_VISIBILITY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_PCB_CELL_RENDERER_VISIBILITY_TYPE))

typedef struct _GtkPcbCellRendererVisibility       GtkPcbCellRendererVisibility;
typedef struct _GtkPcbCellRendererVisibilityClass  GtkPcbCellRendererVisibilityClass;

GType gtk_pcb_cell_renderer_visibility_get_type (void);
GtkCellRenderer *gtk_pcb_cell_renderer_visibility_new (void);

G_END_DECLS  /* keep c++ happy */
#endif
