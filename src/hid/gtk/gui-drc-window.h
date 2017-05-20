/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#ifndef PCB_HID_GTK_GUI_DRC_WINDOW_H
#define PCB_HID_GTK_GUI_DRC_WINDOW_H


#define GHID_TYPE_DRC_VIOLATION           (ghid_drc_violation_get_type())
#define GHID_DRC_VIOLATION(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GHID_TYPE_DRC_VIOLATION, GhidDrcViolation))
#define GHID_DRC_VIOLATION_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  GHID_TYPE_DRC_VIOLATION, GhidDrcViolationClass))
#define GHID_IS_DRC_VIOLATION(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GHID_TYPE_DRC_VIOLATION))
#define GHID_DRC_VIOLATION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GHID_TYPE_DRC_VIOLATION, GhidDrcViolationClass))

typedef struct _GhidDrcViolationClass GhidDrcViolationClass;
typedef struct _GhidDrcViolation GhidDrcViolation;


struct _GhidDrcViolationClass
{
  GObjectClass parent_class;
};

struct _GhidDrcViolation
{
  GObject parent_instance;

  char *title;
  char *explanation;
  Coord x_coord;
  Coord y_coord;
  Angle angle;
  bool have_measured;
  Coord measured_value;
  Coord required_value;
  int object_count;
  long int *object_id_list;
  int *object_type_list;
  GdkDrawable *pixmap;
};


GType ghid_drc_violation_get_type (void);

GhidDrcViolation *ghid_drc_violation_new (DrcViolationType *violation,
					  GdkDrawable *pixmap);


#define GHID_TYPE_VIOLATION_RENDERER           (ghid_violation_renderer_get_type())
#define GHID_VIOLATION_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GHID_TYPE_VIOLATION_RENDERER, GhidViolationRenderer))
#define GHID_VIOLATION_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  GHID_TYPE_VIOLATION_RENDERER, GhidViolationRendererClass))
#define GHID_IS_VIOLATION_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GHID_TYPE_VIOLATION_RENDERER))
#define GHID_VIOLATION_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GHID_TYPE_VIOLATION_RENDERER, GhidViolationRendererClass))

typedef struct _GhidViolationRendererClass GhidViolationRendererClass;
typedef struct _GhidViolationRenderer GhidViolationRenderer;


struct _GhidViolationRendererClass
{
  GtkCellRendererTextClass parent_class;
};

struct _GhidViolationRenderer
{
  GtkCellRendererText parent_instance;

  GhidDrcViolation *violation;
};


GType ghid_violation_renderer_get_type (void);

GtkCellRenderer *ghid_violation_renderer_new (void);

#endif /* PCB_HID_GTK_GUI_DRC_WINDOW_H */
