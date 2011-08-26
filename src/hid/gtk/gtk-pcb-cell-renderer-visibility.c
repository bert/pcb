/*! \file <gtk-pcb-cell-render-visibility.c>
 *  \brief Implementation of GtkCellRenderer for layer visibility toggler
 *  \par More Information
 *  For details on the functions implemented here, see the Gtk
 *  documentation for the GtkCellRenderer object, which defines
 *  the interface we are implementing.
 */

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "gtkhid.h"
#include "gui.h"

#include "gtk-pcb-cell-renderer-visibility.h"

enum {
  TOGGLED,
  LAST_SIGNAL
};
static guint toggle_cell_signals[LAST_SIGNAL] = { 0 };

enum {
  PROP_ACTIVE = 1,
  PROP_COLOR
};

struct _GtkPcbCellRendererVisibility
{
  GtkCellRenderer parent;

  gboolean active;
  gchar *color;
};

struct _GtkPcbCellRendererVisibilityClass
{
  GtkCellRendererClass parent_class;

  void (* toggled) (GtkPcbCellRendererVisibility *cell, const gchar *path);
};

/* RENDERER FUNCTIONS */
/*! \brief Calculates the window area the renderer will use */
static void
gtk_pcb_cell_renderer_visibility_get_size (GtkCellRenderer *cell,
                                           GtkWidget       *widget,
                                           GdkRectangle    *cell_area,
                                           gint            *x_offset,
                                           gint            *y_offset,
                                           gint            *width,
                                           gint            *height)
{
  GtkStyle *style = gtk_widget_get_style (widget);
  gint w, h;

  w = VISIBILITY_TOGGLE_SIZE + 2 * (cell->xpad + style->xthickness);
  h = VISIBILITY_TOGGLE_SIZE + 2 * (cell->ypad + style->ythickness);

  if (width)
    *width = w;
  if (height)
    *height = h;

  if (cell_area)
    {
      if (x_offset)
        {
          gint xalign = (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
                          ? 1.0 - cell->xalign
                          : cell->xalign;
          *x_offset = MAX (0, xalign * (cell_area->width - w));
        }
      if (y_offset)
        *y_offset = MAX(0, cell->yalign * (cell_area->height - h));
    }
}

/*! \brief Actually renders the swatch */
static void
gtk_pcb_cell_renderer_visibility_render (GtkCellRenderer      *cell,
                                         GdkWindow            *window,
                                         GtkWidget            *widget,
                                         GdkRectangle         *background_area,
                                         GdkRectangle         *cell_area,
                                         GdkRectangle         *expose_area,
                                         GtkCellRendererState  flags)
{
  GtkPcbCellRendererVisibility *pcb_cell;
  GdkRectangle toggle_rect;
  GdkRectangle draw_rect;

  pcb_cell = GTK_PCB_CELL_RENDERER_VISIBILITY (cell);
  gtk_pcb_cell_renderer_visibility_get_size (cell, widget, cell_area,
                                             &toggle_rect.x,
                                             &toggle_rect.y,
                                             &toggle_rect.width,
                                             &toggle_rect.height);

  toggle_rect.x      += cell_area->x + cell->xpad;
  toggle_rect.y      += cell_area->y + cell->ypad;
  toggle_rect.width  -= cell->xpad * 2;
  toggle_rect.height -= cell->ypad * 2;

  if (toggle_rect.width <= 0 || toggle_rect.height <= 0)
    return;

  if (gdk_rectangle_intersect (expose_area, cell_area, &draw_rect))
    {
      GdkColor color;
      cairo_t *cr = gdk_cairo_create (window);
      if (expose_area)
        {
          gdk_cairo_rectangle (cr, expose_area);
          cairo_clip (cr);
        }
      cairo_set_line_width (cr, 1);

      cairo_rectangle (cr, toggle_rect.x + 0.5, toggle_rect.y + 0.5,
                           toggle_rect.width - 1, toggle_rect.height - 1);
      cairo_set_source_rgb (cr, 1, 1, 1);
      cairo_fill_preserve (cr);
      cairo_set_source_rgb (cr, 0, 0, 0);
      cairo_stroke (cr);

      gdk_color_parse (pcb_cell->color, &color);
      gdk_cairo_set_source_color (cr, &color);
      if (pcb_cell->active)
        cairo_rectangle (cr, toggle_rect.x + 0.5, toggle_rect.y + 0.5,
                             toggle_rect.width - 1, toggle_rect.height - 1);
      else
        {
          cairo_move_to (cr, toggle_rect.x + 1, toggle_rect.y + 1);
          cairo_rel_line_to (cr, toggle_rect.width / 2, 0);
          cairo_rel_line_to (cr, -toggle_rect.width / 2, toggle_rect.width / 2);
          cairo_close_path (cr);
        }
      cairo_fill (cr);

      cairo_destroy (cr);
    }
}

/*! \brief Toggless the swatch */
static gint
gtk_pcb_cell_renderer_visibility_activate (GtkCellRenderer      *cell,
                                           GdkEvent             *event,
                                           GtkWidget            *widget,
                                           const gchar          *path,
                                           GdkRectangle         *background_area,
                                           GdkRectangle         *cell_area,
                                           GtkCellRendererState  flags)
{
  g_signal_emit (cell, toggle_cell_signals[TOGGLED], 0, path);
  return TRUE;
}

/* Setter/Getter */
static void
gtk_pcb_cell_renderer_visibility_get_property (GObject     *object,
                                               guint        param_id,
                                               GValue      *value,
                                               GParamSpec  *pspec)
{
  GtkPcbCellRendererVisibility *pcb_cell =
    GTK_PCB_CELL_RENDERER_VISIBILITY (object);

  switch (param_id)
    {
    case PROP_ACTIVE:
      g_value_set_boolean (value, pcb_cell->active);
      break;
    case PROP_COLOR:
      g_value_set_string (value, pcb_cell->color);
      break;
    }
}

static void
gtk_pcb_cell_renderer_visibility_set_property (GObject      *object,
                                               guint         param_id,
                                               const GValue *value,
                                               GParamSpec   *pspec)
{
  GtkPcbCellRendererVisibility *pcb_cell =
    GTK_PCB_CELL_RENDERER_VISIBILITY (object);

  switch (param_id)
    {
    case PROP_ACTIVE:
      pcb_cell->active = g_value_get_boolean (value);
      break;
    case PROP_COLOR:
      g_free (pcb_cell->color);
      pcb_cell->color = g_value_dup_string (value);
      break;
    }
}


/* CONSTRUCTOR */
static void
gtk_pcb_cell_renderer_visibility_init (GtkPcbCellRendererVisibility *ls)
{
  g_object_set (ls, "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL);
}

static void
gtk_pcb_cell_renderer_visibility_class_init (GtkPcbCellRendererVisibilityClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (klass);

  object_class->get_property = gtk_pcb_cell_renderer_visibility_get_property;
  object_class->set_property = gtk_pcb_cell_renderer_visibility_set_property;

  cell_class->get_size = gtk_pcb_cell_renderer_visibility_get_size;
  cell_class->render   = gtk_pcb_cell_renderer_visibility_render;
  cell_class->activate = gtk_pcb_cell_renderer_visibility_activate;

  g_object_class_install_property (object_class, PROP_ACTIVE,
                                   g_param_spec_boolean ("active",
                                                         _("Visibility state"),
                                                         _("Visibility of the layer"),
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_COLOR,
                                   g_param_spec_string ("color",
                                                         _("Layer color"),
                                                         _("Layer color"),
                                                         FALSE,
                                                         G_PARAM_READWRITE));


 /**
  * GtkPcbCellRendererVisibility::toggled:
  * @cell_renderer: the object which received the signal
  * @path: string representation of #GtkTreePath describing the 
  *        event location
  *
  * The ::toggled signal is emitted when the cell is toggled. 
  **/
  toggle_cell_signals[TOGGLED] =
    g_signal_new (_("toggled"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GtkPcbCellRendererVisibilityClass, toggled),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);
}

/* PUBLIC FUNCTIONS */
GType
gtk_pcb_cell_renderer_visibility_get_type (void)
{
  static GType ls_type = 0;

  if (!ls_type)
    {
      const GTypeInfo ls_info =
      {
	sizeof (GtkPcbCellRendererVisibilityClass),
	NULL, /* base_init */
	NULL, /* base_finalize */
	(GClassInitFunc) gtk_pcb_cell_renderer_visibility_class_init,
	NULL, /* class_finalize */
	NULL, /* class_data */
	sizeof (GtkPcbCellRendererVisibility),
	0,    /* n_preallocs */
	(GInstanceInitFunc) gtk_pcb_cell_renderer_visibility_init,
      };

      ls_type = g_type_register_static (GTK_TYPE_CELL_RENDERER,
                                        "GtkPcbCellRendererVisibility",
                                        &ls_info,
                                        0);
    }

  return ls_type;
}

GtkCellRenderer *
gtk_pcb_cell_renderer_visibility_new (void)
{
  GtkPcbCellRendererVisibility *rv =
    g_object_new (GTK_PCB_CELL_RENDERER_VISIBILITY_TYPE, NULL);

  rv->active = FALSE;

  return GTK_CELL_RENDERER (rv);
}

