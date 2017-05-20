/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2009 PCB Contributors (See ChangeLog for details)
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"

#include "gui.h"

#include "copy.h"
#include "data.h"
#include "draw.h"
#include "mymem.h"
#include "move.h"
#include "rotate.h"
#include "hid/common/trackball.h"
#include "gui-trackball.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

enum {
  ROTATION_CHANGED,
  VIEW_2D_CHANGED,
  LAST_SIGNAL
};


static guint ghid_trackball_signals[ LAST_SIGNAL ] = { 0 };
static GObjectClass *ghid_trackball_parent_class = NULL;


static gboolean
button_press_cb (GtkWidget *widget, GdkEventButton *ev, gpointer userdata)
{
  GhidTrackball *ball = GHID_TRACKBALL (userdata);
  float axis[3];

  /* Only respond to left mouse button for now */
  if (ev->button != 1)
    return TRUE;

  switch (ev->type) {

    case GDK_BUTTON_PRESS:
      ball->x1 = 2. * ev->x / widget->allocation.width - 1.;
      ball->y1 = 2. * ev->y / widget->allocation.height - 1.;
      ball->dragging = TRUE;
      break;

    case GDK_2BUTTON_PRESS:
      /* Reset the rotation of the trackball */
      /* TODO: Would be nice to animate this! */
      axis[0] = 1.; axis[1] = 0.; axis[2] = 0.;
      axis_to_quat (axis, 0, ball->quart1);
      axis_to_quat (axis, 0, ball->quart2);
      g_signal_emit (ball, ghid_trackball_signals[ROTATION_CHANGED], 0,
                     ball->quart2);
      break;

    default:
      break;
  }

  return TRUE;
}


static gboolean
button_release_cb (GtkWidget *widget, GdkEventButton *ev, gpointer userdata)
{
  GhidTrackball *ball = GHID_TRACKBALL (userdata);

  /* Only respond to left mouse button for now */
  if (ev->button != 1)
    return TRUE;

  ball->quart1[0] = ball->quart2[0];
  ball->quart1[1] = ball->quart2[1];
  ball->quart1[2] = ball->quart2[2];
  ball->quart1[3] = ball->quart2[3];

  ball->dragging = FALSE;

  return TRUE;
}


static gboolean
motion_notify_cb (GtkWidget *widget, GdkEventMotion *ev, gpointer userdata)
{
  GhidTrackball *ball = GHID_TRACKBALL (userdata);
  double x1, y1;
  double x2, y2;
  float q[4];

  if (!ball->dragging) {
    gdk_event_request_motions (ev);
    return TRUE;
  }

  x1 = ball->x1;
  y1 = ball->y1;

  x2 = 2. * ev->x / widget->allocation.width - 1.;
  y2 = 2. * ev->y / widget->allocation.height - 1.;

  /* Trackball computation */
  trackball (q, x1, y1, x2, y2);
  add_quats (q, ball->quart1, ball->quart2);

  g_signal_emit (ball, ghid_trackball_signals[ROTATION_CHANGED], 0,
                 ball->quart2);

  gdk_event_request_motions (ev);
  return TRUE;
}

static gboolean
ghid_trackball_expose (GtkWidget * widget, GdkEventExpose * ev)
{
  cairo_t *cr;
  cairo_pattern_t *pattern;
  GtkAllocation allocation;
  GdkColor color;
  double radius;

  cr = gdk_cairo_create (gtk_widget_get_window (widget));

          /* set a clip region for the expose event */
  cairo_rectangle (cr,
                   ev->area.x, ev->area.y,
                   ev->area.width, ev->area.height);
  cairo_clip (cr);

  gtk_widget_get_allocation (widget, &allocation);

  radius = (MIN (allocation.width, allocation.height) - 5) / 2.;
  pattern = cairo_pattern_create_radial (2 * radius * 0.8,
                                         2 * radius * 0.3,
                                         0.,
                                         2 * radius * 0.50,
                                         2 * radius * 0.50,
                                         2 * radius * 0.71);

  color = widget->style->fg[gtk_widget_get_state (widget)];

  cairo_pattern_add_color_stop_rgb (pattern, 0.0,
                                    (color.red   / 65535. * 0.5 + 4.5) / 5.,
                                    (color.green / 65535. * 0.5 + 4.5) / 5.,
                                    (color.blue  / 65535. * 0.5 + 4.5) / 5.);
  cairo_pattern_add_color_stop_rgb (pattern, 0.2,
                                    (color.red   / 65535. * 1.5 + 3.7) / 5.,
                                    (color.green / 65535. * 1.5 + 3.7) / 5.,
                                    (color.blue  / 65535. * 1.5 + 3.7) / 5.);
  cairo_pattern_add_color_stop_rgb (pattern, 1.0,
                                    (color.red   / 65535. * 5. + 0.) / 5.,
                                    (color.green / 65535. * 5. + 0.) / 5.,
                                    (color.blue  / 65535. * 5. + 0.) / 5.);
  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);

  cairo_save (cr);
  cairo_translate (cr, allocation.width / 2., allocation.height / 2.);
  cairo_scale (cr, radius, radius);
  cairo_arc (cr, 0., 0., 1., 0., 2 * M_PI);
  cairo_restore (cr);

  cairo_fill_preserve (cr);

  gdk_cairo_set_source_color (cr, &widget->style->bg[gtk_widget_get_state (widget)]);
  cairo_set_line_width (cr, 0.4);
  cairo_stroke (cr);

  cairo_destroy (cr);

  return FALSE;
}

static gboolean
view_2d_toggled_cb (GtkToggleButton *toggle, gpointer userdata)
{
  GhidTrackball *ball = GHID_TRACKBALL (userdata);
  float axis[3];
  float quart[4];
  gboolean view_2d;

  view_2d = gtk_toggle_button_get_active (toggle);
  if (view_2d)
    {
      axis[0] = 1.; axis[1] = 0.; axis[2] = 0.;
      axis_to_quat (axis, 0, quart);

      g_signal_emit (ball, ghid_trackball_signals[ROTATION_CHANGED], 0, quart);
      gtk_widget_set_sensitive (ball->drawing_area, FALSE);
    }
  else
    {
      g_signal_emit (ball, ghid_trackball_signals[ROTATION_CHANGED], 0, ball->quart1);
      gtk_widget_set_sensitive (ball->drawing_area, TRUE);
    }

  g_signal_emit (ball, ghid_trackball_signals[VIEW_2D_CHANGED], 0, view_2d);

  return TRUE;
}
/*! \brief GObject constructor
 *
 *  \par Function Description
 *  Chain up and construct the object, then setup the
 *  necessary state for our widget now it is constructed.
 *
 *  \param [in] type                    The GType of object to be constructed
 *  \param [in] n_construct_properties  Number of construct properties
 *  \param [in] contruct_params         The construct properties
 *
 *  \returns The GObject having just been constructed.
 */
static GObject *
ghid_trackball_constructor (GType type,
                            guint n_construct_properties,
                            GObjectConstructParam *construct_properties)
{
  GhidTrackball *ball;
  float axis[3];

  /* chain up to constructor of parent class */
  ball = GHID_TRACKBALL (G_OBJECT_CLASS (ghid_trackball_parent_class)->
    constructor (type, n_construct_properties, construct_properties));

  gtk_widget_set_size_request (GTK_WIDGET (ball), 140, 140);

  ball->view_2d = gtk_toggle_button_new_with_label (_("2D View"));
  gtk_box_pack_start (GTK_BOX (ball), ball->view_2d, FALSE, FALSE, 0);
  gtk_widget_show (ball->view_2d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ball->view_2d), TRUE);

  ball->drawing_area = gtk_drawing_area_new ();
  gtk_box_pack_start (GTK_BOX (ball), ball->drawing_area, TRUE, TRUE, 0);
  gtk_widget_set_sensitive (ball->drawing_area, FALSE);
  gtk_widget_show (ball->drawing_area);

  axis[0] = 1.; axis[1] = 0.; axis[2] = 0.;
  axis_to_quat (axis, 0, ball->quart1);
  axis_to_quat (axis, 0, ball->quart2);

  g_signal_connect (ball->view_2d, "toggled",
                    G_CALLBACK (view_2d_toggled_cb), ball);

  g_signal_connect (ball->drawing_area, "expose-event",
                    G_CALLBACK (ghid_trackball_expose), ball);
  g_signal_connect (ball->drawing_area, "button-press-event",
                    G_CALLBACK (button_press_cb), ball);
  g_signal_connect (ball->drawing_area, "button-release-event",
                    G_CALLBACK (button_release_cb), ball);
  g_signal_connect (ball->drawing_area, "motion-notify-event",
                    G_CALLBACK (motion_notify_cb), ball);

  gtk_widget_add_events (ball->drawing_area, GDK_BUTTON_PRESS_MASK   |
                                             GDK_BUTTON_RELEASE_MASK |
                                             GDK_POINTER_MOTION_MASK |
                                             GDK_POINTER_MOTION_HINT_MASK);

  return G_OBJECT (ball);
}



/*! \brief GObject finalise handler
 *
 *  \par Function Description
 *  Just before the GhidTrackball GObject is finalized, free our
 *  allocated data, and then chain up to the parent's finalize handler.
 *
 *  \param [in] widget  The GObject being finalized.
 */
static void
ghid_trackball_finalize (GObject * object)
{
  G_OBJECT_CLASS (ghid_trackball_parent_class)->finalize (object);
}


/*! \brief GObject property setter function
 *
 *  \par Function Description
 *  Setter function for GhidTrackball's GObject properties,
 *  "settings-name" and "toplevel".
 *
 *  \param [in]  object       The GObject whose properties we are setting
 *  \param [in]  property_id  The numeric id. under which the property was
 *                            registered with g_object_class_install_property()
 *  \param [in]  value        The GValue the property is being set from
 *  \param [in]  pspec        A GParamSpec describing the property being set
 */
static void
ghid_trackball_set_property (GObject * object, guint property_id,
				  const GValue * value, GParamSpec * pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


/*! \brief GObject property getter function
 *
 *  \par Function Description
 *  Getter function for GhidTrackball's GObject properties,
 *  "settings-name" and "toplevel".
 *
 *  \param [in]  object       The GObject whose properties we are getting
 *  \param [in]  property_id  The numeric id. under which the property was
 *                            registered with g_object_class_install_property()
 *  \param [out] value        The GValue in which to return the value of the property
 *  \param [in]  pspec        A GParamSpec describing the property being got
 */
static void
ghid_trackball_get_property (GObject * object, guint property_id,
				  GValue * value, GParamSpec * pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }

}


/*! \brief GType class initialiser for GhidTrackball
 *
 *  \par Function Description
 *  GType class initialiser for GhidTrackball. We override our parent
 *  virtual class methods as needed and register our GObject properties.
 *
 *  \param [in]  klass       The GhidTrackballClass we are initialising
 */
static void
ghid_trackball_class_init (GhidTrackballClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructor  = ghid_trackball_constructor;
  gobject_class->finalize     = ghid_trackball_finalize;
  gobject_class->set_property = ghid_trackball_set_property;
  gobject_class->get_property = ghid_trackball_get_property;

  ghid_trackball_parent_class = g_type_class_peek_parent (klass);

  ghid_trackball_signals[ROTATION_CHANGED] =
    g_signal_new ("rotation-changed",
                  G_OBJECT_CLASS_TYPE( gobject_class ),
                  G_SIGNAL_RUN_FIRST,     /*signal_flags */
                  G_STRUCT_OFFSET( GhidTrackballClass, rotation_changed ),
                  NULL, /* accumulator */
                  NULL, /* accu_data */
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE,
                  1,    /* n_params */
                  G_TYPE_POINTER
                 );

  ghid_trackball_signals[VIEW_2D_CHANGED] =
    g_signal_new ("view-2d-changed",
                  G_OBJECT_CLASS_TYPE( gobject_class ),
                  G_SIGNAL_RUN_FIRST,     /*signal_flags */
                  G_STRUCT_OFFSET( GhidTrackballClass, view_2d_changed ),
                  NULL, /* accumulator */
                  NULL, /* accu_data */
                  g_cclosure_marshal_VOID__BOOLEAN,
                  G_TYPE_NONE,
                  1,    /* n_params */
                  G_TYPE_BOOLEAN
                 );
}


/*! \brief Function to retrieve GhidTrackball's GType identifier.
 *
 *  \par Function Description
 *  Function to retrieve GhidTrackball's GType identifier.
 *  Upon first call, this registers the GhidTrackball in the GType system.
 *  Subsequently it returns the saved value from its first execution.
 *
 *  \return the GType identifier associated with GhidTrackball.
 */
GType
ghid_trackball_get_type ()
{
  static GType ghid_trackball_type = 0;

  if (!ghid_trackball_type)
    {
      static const GTypeInfo ghid_trackball_info = {
	sizeof (GhidTrackballClass),
	NULL,			/* base_init */
	NULL,			/* base_finalize */
	(GClassInitFunc) ghid_trackball_class_init,
	NULL,			/* class_finalize */
	NULL,			/* class_data */
	sizeof (GhidTrackball),
	0,			/* n_preallocs */
	NULL,			/* instance_init */
      };

      ghid_trackball_type =
	g_type_register_static (GTK_TYPE_VBOX, "GhidTrackball",
				&ghid_trackball_info, 0);
    }

  return ghid_trackball_type;
}


/*! \brief Convenience function to create a new trackball widget
 *
 *  \par Function Description
 *  Convenience function which creates a GhidTrackball.
 *
 *  \return  The GhidTrackball created.
 */
GtkWidget *
ghid_trackball_new (void)
{
  GhidTrackball *ball;

  ball = g_object_new (GHID_TYPE_TRACKBALL, NULL);

  return GTK_WIDGET (ball);
}
