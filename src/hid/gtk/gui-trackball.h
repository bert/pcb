/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2009  PCB Contributors (see ChangeLog for details)
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

#ifndef __GHID_TRACKBALL_H__
#define __GHID_TRACKBALL_H__


#define GHID_TYPE_TRACKBALL           (ghid_trackball_get_type())
#define GHID_TRACKBALL(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GHID_TYPE_TRACKBALL, GhidTrackball))
#define GHID_TRACKBALL_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  GHID_TYPE_TRACKBALL, GhidTrackballClass))
#define GHID_IS_TRACKBALL(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GHID_TYPE_TRACKBALL))
#define GHID_TRACKBALL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GHID_TYPE_TRACKBALL, GhidTrackballClass))

typedef struct _GhidTrackballClass GhidTrackballClass;
typedef struct _GhidTrackball GhidTrackball;


struct _GhidTrackballClass
{
  GtkVBoxClass parent_class;

  void (*rotation_changed) (GhidTrackball *ball, gpointer rotation, gpointer user_data);
  void (*view_2d_changed) (GhidTrackball *ball, gboolean view_2d, gpointer user_data);
};

struct _GhidTrackball
{
  GtkVBox parent_instance;

  GtkWidget *drawing_area;
  GtkWidget *view_2d;

  gboolean dragging;
  gdouble x1, y1;

  float quart1[4];
  float quart2[4];
};


GType ghid_trackball_get_type (void);

GtkWidget *ghid_trackball_new (void);

#endif /* __GHID_TRACKBALL_H__ */
