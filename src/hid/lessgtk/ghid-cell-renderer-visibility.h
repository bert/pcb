/*!
 * \file src/hid/gtk/ghid-cell-renderer-visibility.h
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
 * <hr>
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

#endif /* GHID_CELL_RENDERER_VISIBILITY_H__ */
