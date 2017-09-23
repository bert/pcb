/*!
 * \file src/hid/gtk/ghid-cell-renderer-visibility.c
 *
 * \brief Implementation of GtkCellRenderer for layer visibility toggler.
 *
 * For details on the functions implemented here, see the Gtk
 * documentation for the GtkCellRenderer object, which defines
 * the interface we are implementing.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2017 PCB Developers.
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
 *
 * <hr>
 */

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "gtkhid.h"
#include "gui.h"
#include "ghid-cell-renderer-visibility.h"

enum
{
  TOGGLED,
  LAST_SIGNAL
};

static guint toggle_cell_signals[LAST_SIGNAL] = { 0 };

enum
{
  PROP_ACTIVE = 1,
  PROP_COLOR
};

struct _GHidCellRendererVisibility
{
  GtkCellRenderer parent;
  gboolean active;
  gchar *color;
};

struct _GHidCellRendererVisibilityClass
{
  GtkCellRendererClass parent_class;
  void (* toggled) (GHidCellRendererVisibility *cell, const gchar *path);
};

/* RENDERER FUNCTIONS */

/*!
 * \brief Calculates the window area the renderer will use.
 */
static void
ghid_cell_renderer_visibility_get_size (GtkCellRenderer *cell,
                                        GtkWidget       *widget,
                                        GdkRectangle    *cell_area,
                                        gint            *x_offset,
                                        gint            *y_offset,
                                        gint            *width,
                                        gint            *height)
{
  GtkStyle *style = gtk_widget_get_style (widget);
  gint w, h;
  gint xpad, ypad;
  gfloat xalign, yalign;

  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);
  gtk_cell_renderer_get_alignment (cell, &xalign, &yalign);

  w = VISIBILITY_TOGGLE_SIZE + 2 * (xpad + style->xthickness);
  h = VISIBILITY_TOGGLE_SIZE + 2 * (ypad + style->ythickness);

  if (width)
    *width = w;
  if (height)
    *height = h;

  if (cell_area)
    {
      if (x_offset)
        {
          if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
            xalign = 1. - xalign;
          *x_offset = MAX (0, xalign * (cell_area->width - w));
        }
      if (y_offset)
        *y_offset = MAX(0, yalign * (cell_area->height - h));
    }
}

/*!
 * \brief Actually renders the swatch.
 */
static void
ghid_cell_renderer_visibility_render (GtkCellRenderer      *cell,
                                      GdkWindow            *window,
                                      GtkWidget            *widget,
                                      GdkRectangle         *background_area,
                                      GdkRectangle         *cell_area,
                                      GdkRectangle         *expose_area,
                                      GtkCellRendererState  flags)
{
  GHidCellRendererVisibility *pcb_cell;
  GdkRectangle toggle_rect;
  GdkRectangle draw_rect;
  gint xpad, ypad;

  pcb_cell = GHID_CELL_RENDERER_VISIBILITY (cell);
  ghid_cell_renderer_visibility_get_size (cell, widget, cell_area,
                                          &toggle_rect.x,
                                          &toggle_rect.y,
                                          &toggle_rect.width,
                                          &toggle_rect.height);
  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);

  toggle_rect.x      += cell_area->x + xpad;
  toggle_rect.y      += cell_area->y + ypad;
  toggle_rect.width  -= xpad * 2;
  toggle_rect.height -= ypad * 2;

  if (toggle_rect.width <= 0 || toggle_rect.height <= 0)
    return;

  if (gdk_rectangle_intersect (expose_area, cell_area, &draw_rect))
    {
      GdkColor color;
      cairo_t *cr = gdk_cairo_create (window);
      cairo_pattern_t *pattern;

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
      if (flags & GTK_CELL_RENDERER_PRELIT)
        {
          color.red = (4*color.red + 65535) / 5;
          color.green = (4*color.green + 65535) / 5;
          color.blue = (4*color.blue + 65535) / 5;
        }

      pattern = cairo_pattern_create_radial ((toggle_rect.width  - 1.) * 0.75 + toggle_rect.x + 0.5,
                                             (toggle_rect.height - 1.) * 0.75 + toggle_rect.y + 0.5,
                                             0.,
                                             (toggle_rect.width  - 1.) * 0.50 + toggle_rect.x + 0.5,
                                             (toggle_rect.height - 1.) * 0.50 + toggle_rect.y + 0.5,
                                             (toggle_rect.width  - 1.) * 0.71);

      cairo_pattern_add_color_stop_rgb (pattern, 0.0,
                                        (color.red   / 65535. * 4. + 1.) / 5.,
                                        (color.green / 65535. * 4. + 1.) / 5.,
                                        (color.blue  / 65535. * 4. + 1.) / 5.);
      cairo_pattern_add_color_stop_rgb (pattern, 1.0,
                                        (color.red   / 65535. * 5. + 0.) / 5.,
                                        (color.green / 65535. * 5. + 0.) / 5.,
                                        (color.blue  / 65535. * 5. + 0.) / 5.);
      cairo_set_source (cr, pattern);
      cairo_pattern_destroy (pattern);

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

/*!
 * \brief Toggles the swatch.
 */
static gint
ghid_cell_renderer_visibility_activate (GtkCellRenderer      *cell,
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
ghid_cell_renderer_visibility_get_property (GObject     *object,
                                            guint        param_id,
                                            GValue      *value,
                                            GParamSpec  *pspec)
{
  GHidCellRendererVisibility *pcb_cell =
    GHID_CELL_RENDERER_VISIBILITY (object);

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
ghid_cell_renderer_visibility_set_property (GObject      *object,
                                               guint         param_id,
                                               const GValue *value,
                                               GParamSpec   *pspec)
{
  GHidCellRendererVisibility *pcb_cell =
    GHID_CELL_RENDERER_VISIBILITY (object);

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
ghid_cell_renderer_visibility_init (GHidCellRendererVisibility *ls)
{
  g_object_set (ls, "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL);
}

static void
ghid_cell_renderer_visibility_class_init (GHidCellRendererVisibilityClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (klass);

  object_class->get_property = ghid_cell_renderer_visibility_get_property;
  object_class->set_property = ghid_cell_renderer_visibility_set_property;

  cell_class->get_size = ghid_cell_renderer_visibility_get_size;
  cell_class->render   = ghid_cell_renderer_visibility_render;
  cell_class->activate = ghid_cell_renderer_visibility_activate;

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


  /*!
   * GHidCellRendererVisibility::toggled:
   *
   * @cell_renderer: the object which received the signal
   * @path: string representation of #GtkTreePath describing the 
   *        event location
   *
   * The ::toggled signal is emitted when the cell is toggled. 
   */
  toggle_cell_signals[TOGGLED] =
    g_signal_new ("toggled",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GHidCellRendererVisibilityClass, toggled),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);
}

/* PUBLIC FUNCTIONS */

GType
ghid_cell_renderer_visibility_get_type (void)
{
  static GType ls_type = 0;

  if (!ls_type)
    {
      const GTypeInfo ls_info =
      {
	sizeof (GHidCellRendererVisibilityClass),
	NULL, /* base_init */
	NULL, /* base_finalize */
	(GClassInitFunc) ghid_cell_renderer_visibility_class_init,
	NULL, /* class_finalize */
	NULL, /* class_data */
	sizeof (GHidCellRendererVisibility),
	0,    /* n_preallocs */
	(GInstanceInitFunc) ghid_cell_renderer_visibility_init,
      };

      ls_type = g_type_register_static (GTK_TYPE_CELL_RENDERER,
                                        "GHidCellRendererVisibility",
                                        &ls_info,
                                        0);
    }

  return ls_type;
}

GtkCellRenderer *
ghid_cell_renderer_visibility_new (void)
{
  GHidCellRendererVisibility *rv =
    g_object_new (GHID_CELL_RENDERER_VISIBILITY_TYPE, NULL);

  rv->active = FALSE;

  return GTK_CELL_RENDERER (rv);
}

