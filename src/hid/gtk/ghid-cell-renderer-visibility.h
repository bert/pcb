/*!
 * \file src/hid/gtk/ghid-cell-renderer-visibility.h
 */

#ifndef GHID_CELL_RENDERER_VISIBILITY_H__
#define GHID_CELL_RENDERER_VISIBILITY_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS  /* keep c++ happy */

#define VISIBILITY_TOGGLE_SIZE	16

#define GHID_CELL_RENDERER_VISIBILITY_TYPE            (ghid_cell_renderer_visibility_get_type ())
#define GHID_CELL_RENDERER_VISIBILITY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GHID_CELL_RENDERER_VISIBILITY_TYPE, GHidCellRendererVisibility))
#define GHID_CELL_RENDERER_VISIBILITY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GHID_CELL_RENDERER_VISIBILITY_TYPE, GHidCellRendererVisibilityClass))
#define IS_GHID_CELL_RENDERER_VISIBILITY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GHID_CELL_RENDERER_VISIBILITY_TYPE))
#define IS_GHID_CELL_RENDERER_VISIBILITY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GHID_CELL_RENDERER_VISIBILITY_TYPE))

typedef struct _GHidCellRendererVisibility       GHidCellRendererVisibility;
typedef struct _GHidCellRendererVisibilityClass  GHidCellRendererVisibilityClass;

GType ghid_cell_renderer_visibility_get_type (void);
GtkCellRenderer *ghid_cell_renderer_visibility_new (void);

G_END_DECLS  /* keep c++ happy */
#endif
