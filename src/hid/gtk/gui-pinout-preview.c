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

/* This file copied and modified by Peter Clifton, starting from
 * gui-pinout-window.c, written by Bill Wilson for the PCB Gtk port */

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
#include "gui-pinout-preview.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* Just define a sensible scale, lets say (for example), 100 pixel per 150 mil */
#define SENSIBLE_VIEW_SCALE  (100. / MIL_TO_COORD (150.))
static void
pinout_set_view (GhidPinoutPreview * pinout)
{
  float scale = SENSIBLE_VIEW_SCALE;

  pinout->x_max = pinout->element->BoundingBox.X2 + Settings.PinoutOffsetX;
  pinout->y_max = pinout->element->BoundingBox.Y2 + Settings.PinoutOffsetY;
  pinout->w_pixels = scale * (pinout->element->BoundingBox.X2 -
                              pinout->element->BoundingBox.X1);
  pinout->h_pixels = scale * (pinout->element->BoundingBox.Y2 -
                              pinout->element->BoundingBox.Y1);
}


static void
pinout_set_data (GhidPinoutPreview * pinout, ElementType * element)
{
  if (pinout->element != NULL)
    {
      FreeElementMemory (pinout->element);
      g_slice_free (ElementType, pinout->element);
      pinout->element = NULL;
    }

  if (element == NULL)
    {
      pinout->w_pixels = 0;
      pinout->h_pixels = 0;
      return;
    }

  /* 
   * copy element data 
   * enable output of pin and padnames
   * move element to a 5% offset from zero position
   * set all package lines/arcs to zero width
   */
  pinout->element = CopyElementLowLevel (NULL, element, FALSE, 0, 0, FOUNDFLAG);
  PIN_LOOP (pinout->element);
  {
    SET_FLAG (DISPLAYNAMEFLAG, pin);
  }
  END_LOOP;

  PAD_LOOP (pinout->element);
  {
    SET_FLAG (DISPLAYNAMEFLAG, pad);
  }
  END_LOOP;


  MoveElementLowLevel (NULL, pinout->element,
		       Settings.PinoutOffsetX -
		       pinout->element->BoundingBox.X1,
		       Settings.PinoutOffsetY -
		       pinout->element->BoundingBox.Y1);

  pinout_set_view (pinout);

  ELEMENTLINE_LOOP (pinout->element);
  {
    line->Thickness = 0;
  }
  END_LOOP;

  ARC_LOOP (pinout->element);
  {
    /* 
     * for whatever reason setting a thickness of 0 causes the arcs to
     * not display so pick 1 which does display but is still quite
     * thin.
     */
    arc->Thickness = 1;
  }
  END_LOOP;
}


enum
{
  PROP_ELEMENT_DATA = 1,
};


static GObjectClass *ghid_pinout_preview_parent_class = NULL;


/*! \brief GObject constructed
 *
 *  \par Function Description
 *  Initialise the pinout preview object once it is constructed.
 *  Chain up in case the parent class wants to do anything too.
 *
 *  \param [in] object  The pinout preview object
 */
static void
ghid_pinout_preview_constructed (GObject *object)
{
  /* chain up to the parent class */
  if (G_OBJECT_CLASS (ghid_pinout_preview_parent_class)->constructed != NULL)
    G_OBJECT_CLASS (ghid_pinout_preview_parent_class)->constructed (object);

  ghid_init_drawing_widget (GTK_WIDGET (object), gport);
}



/*! \brief GObject finalise handler
 *
 *  \par Function Description
 *  Just before the GhidPinoutPreview GObject is finalized, free our
 *  allocated data, and then chain up to the parent's finalize handler.
 *
 *  \param [in] widget  The GObject being finalized.
 */
static void
ghid_pinout_preview_finalize (GObject * object)
{
  GhidPinoutPreview *pinout = GHID_PINOUT_PREVIEW (object);

  /* Passing NULL for element data will free the old memory */
  pinout_set_data (pinout, NULL);

  G_OBJECT_CLASS (ghid_pinout_preview_parent_class)->finalize (object);
}


/*! \brief GObject property setter function
 *
 *  \par Function Description
 *  Setter function for GhidPinoutPreview's GObject properties,
 *  "settings-name" and "toplevel".
 *
 *  \param [in]  object       The GObject whose properties we are setting
 *  \param [in]  property_id  The numeric id. under which the property was
 *                            registered with g_object_class_install_property()
 *  \param [in]  value        The GValue the property is being set from
 *  \param [in]  pspec        A GParamSpec describing the property being set
 */
static void
ghid_pinout_preview_set_property (GObject * object, guint property_id,
				  const GValue * value, GParamSpec * pspec)
{
  GhidPinoutPreview *pinout = GHID_PINOUT_PREVIEW (object);
  GdkWindow *window = gtk_widget_get_window (GTK_WIDGET (pinout));

  switch (property_id)
    {
    case PROP_ELEMENT_DATA:
      pinout_set_data (pinout, (ElementType *)g_value_get_pointer (value));
      if (window != NULL)
        gdk_window_invalidate_rect (window, NULL, FALSE);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }

}


/*! \brief GObject property getter function
 *
 *  \par Function Description
 *  Getter function for GhidPinoutPreview's GObject properties,
 *  "settings-name" and "toplevel".
 *
 *  \param [in]  object       The GObject whose properties we are getting
 *  \param [in]  property_id  The numeric id. under which the property was
 *                            registered with g_object_class_install_property()
 *  \param [out] value        The GValue in which to return the value of the property
 *  \param [in]  pspec        A GParamSpec describing the property being got
 */
static void
ghid_pinout_preview_get_property (GObject * object, guint property_id,
				  GValue * value, GParamSpec * pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }

}


/*! \brief GType class initialiser for GhidPinoutPreview
 *
 *  \par Function Description
 *  GType class initialiser for GhidPinoutPreview. We override our parent
 *  virtual class methods as needed and register our GObject properties.
 *
 *  \param [in]  klass       The GhidPinoutPreviewClass we are initialising
 */
static void
ghid_pinout_preview_class_init (GhidPinoutPreviewClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *gtk_widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->finalize = ghid_pinout_preview_finalize;
  gobject_class->set_property = ghid_pinout_preview_set_property;
  gobject_class->get_property = ghid_pinout_preview_get_property;
  gobject_class->constructed = ghid_pinout_preview_constructed;

  gtk_widget_class->expose_event = ghid_pinout_preview_expose;

  ghid_pinout_preview_parent_class = (GObjectClass *)g_type_class_peek_parent (klass);

  g_object_class_install_property (gobject_class, PROP_ELEMENT_DATA,
				   g_param_spec_pointer ("element-data",
							 "",
							 "",
							 G_PARAM_WRITABLE));
}


/*! \brief Function to retrieve GhidPinoutPreview's GType identifier.
 *
 *  \par Function Description
 *  Function to retrieve GhidPinoutPreview's GType identifier.
 *  Upon first call, this registers the GhidPinoutPreview in the GType system.
 *  Subsequently it returns the saved value from its first execution.
 *
 *  \return the GType identifier associated with GhidPinoutPreview.
 */
GType
ghid_pinout_preview_get_type ()
{
  static GType ghid_pinout_preview_type = 0;

  if (!ghid_pinout_preview_type)
    {
      static const GTypeInfo ghid_pinout_preview_info = {
	sizeof (GhidPinoutPreviewClass),
	NULL,			/* base_init */
	NULL,			/* base_finalize */
	(GClassInitFunc) ghid_pinout_preview_class_init,
	NULL,			/* class_finalize */
	NULL,			/* class_data */
	sizeof (GhidPinoutPreview),
	0,			/* n_preallocs */
	NULL,			/* instance_init */
      };

      ghid_pinout_preview_type =
	g_type_register_static (GTK_TYPE_DRAWING_AREA, "GhidPinoutPreview",
				&ghid_pinout_preview_info, (GTypeFlags)0);
    }

  return ghid_pinout_preview_type;
}


/*! \brief Convenience function to create a new pinout preview
 *
 *  \par Function Description
 *  Convenience function which creates a GhidPinoutPreview.
 *
 *  \return  The GhidPinoutPreview created.
 */
GtkWidget *
ghid_pinout_preview_new (ElementType * element)
{
  GhidPinoutPreview *pinout_preview;

  pinout_preview = (GhidPinoutPreview *)g_object_new (GHID_TYPE_PINOUT_PREVIEW,
				 "element-data", element, NULL);

  return GTK_WIDGET (pinout_preview);
}


/*! \brief Query the natural size of a pinout preview
 *
 *  \par Function Description
 *  Convenience function to query the natural size of a pinout preview
 */
void
ghid_pinout_preview_get_natural_size (GhidPinoutPreview * pinout,
				      int *width, int *height)
{
  *width = pinout->w_pixels;
  *height = pinout->h_pixels;
}
