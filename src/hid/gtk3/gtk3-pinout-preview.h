/*!
 * \file src/hid/gtk3/gtk3-pinout-preview.h
 *
 * \brief Provides a preview for the pinout of an element for the GTK3 UI.
 *
 * \note This file is copied from the PCB GTK-2 port and modified to
 * comply with GTK-3.
 *
 * \copyright (C) 2021 PCB Contributors.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 * Copyright (C) 1994, 1995, 1996 Thomas Nau
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 */


#ifndef PCB_SRC_HID_GTK3_GTK3_PINOUT_PREVIEW_H__
#define PCB_SRC_HID_GTK3_GTK3_PINOUT_PREVIEW_H__


#define GHID_TYPE_PINOUT_PREVIEW (ghid_pinout_preview_get_type())
#define GHID_PINOUT_PREVIEW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GHID_TYPE_PINOUT_PREVIEW, GhidPinoutPreview))
#define GHID_PINOUT_PREVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GHID_TYPE_PINOUT_PREVIEW, GhidPinoutPreviewClass))
#define GHID_IS_PINOUT_PREVIEW(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GHID_TYPE_PINOUT_PREVIEW))
#define GHID_PINOUT_PREVIEW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GHID_TYPE_PINOUT_PREVIEW, GhidPinoutPreviewClass))


typedef struct _GhidPinoutPreviewClass GhidPinoutPreviewClass;
typedef struct _GhidPinoutPreview GhidPinoutPreview;


struct _GhidPinoutPreviewClass
{
  GtkDrawingAreaClass parent_class;
};


struct _GhidPinoutPreview
{
  GtkDrawingArea parent_instance;

  ElementType *element; /*!< Element data to display. */
  gint x_max;
  gint y_max;
  gint w_pixels; /*!< Natural width size of element preview. */
  gint h_pixels; /*!< Natural height size of element preview. */
};


GType ghid_pinout_preview_get_type (void);
GtkWidget *ghid_pinout_preview_new (ElementType *element);
void ghid_pinout_preview_get_natural_size (GhidPinoutPreview *pinout, int *width, int *height);


#endif /* PCB_SRC_HID_GTK3_GTK3I_PINOUT_PREVIEW_H__ */
