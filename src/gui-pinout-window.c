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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* This file written by Bill Wilson for the PCB Gtk port
*/

#include "global.h"

#include "gui.h"

#include "copy.h"
#include "data.h"
#include "draw.h"
#include "mymem.h"
#include "move.h"
#include "rotate.h"
#include "output.h"


typedef struct
	{
	GtkWidget	*top_window,
				*drawing_area,
				*enlarge_button,
				*shrink_button;

	ElementType	element;			/* element data to display */

	gfloat		zoom,				/* zoom factor of window */
				scale;				/* scale factor of zoom */
	gint		x_max,
				y_max;
	gint		w_pixels,
				h_pixels;
	}
	PinoutType;


  /* Panning updates a clip box which assumes drawing will occur inside of
  |  the output drawing area.  But SwitchDrawingWindow() will divert output
  |  to other windows which have different clip requirements, eg pinout
  |  window.  FIXME, SwitchDrawingWindow() should be smart enough to handle
  |  this automatically and this routine should go away.
  */
static void
output_clipbox_enable(gboolean enable)
	{
	static BoxType	clip_box_save;
	static gboolean	pushed;

	if (enable && pushed)
		{
		clipBox = clip_box_save;
		pushed = FALSE;
		}
	else if (!pushed)
		{
		clip_box_save = clipBox;
		clipBox.X1 = 0;
		clipBox.Y1 = 0;
		clipBox.X2 = MAX_COORD;
		clipBox.Y2 = MAX_COORD;
		pushed = TRUE;
		}

	}


static gboolean
pinout_zoom_fit(PinoutType *pinout, gint zoom)
	{
	pinout->zoom = zoom;

	/* Should be a function I can call for this:
	*/
	pinout->scale = 1.0 / (100.0 * exp (pinout->zoom * LN_2_OVER_2));

	pinout->x_max = pinout->element.BoundingBox.X2 + Settings.PinoutOffsetX;
	pinout->y_max = pinout->element.BoundingBox.Y2 + Settings.PinoutOffsetY;
	pinout->w_pixels = (gint) (pinout->scale *
			(pinout->element.BoundingBox.X2 - pinout->element.BoundingBox.X1));
	pinout->h_pixels = (gint) (pinout->scale *
			(pinout->element.BoundingBox.Y2 - pinout->element.BoundingBox.Y1));

	if (   pinout->w_pixels > 3 * Output.Width / 4
	    || pinout->h_pixels > 3 * Output.Height / 4
	   )
		return FALSE;
	return TRUE;
	}

PinoutType *
pinout_new(ElementType *element)
	{
	PinoutType	*pinout;
	gint		tx, ty, x_min = 0, y_min = 0;

	pinout = g_new0(PinoutType, 1);

	/* copy element data 
	|  enable output of pin and padnames
	|  move element to a 5% offset from zero position
	|  set all package lines/arcs to zero with
	*/
	CopyElementLowLevel(NULL, &pinout->element, element, FALSE);
	PIN_LOOP(&pinout->element);
		{
		tx = abs(pinout->element.Pin[0].X - pin->X);
		ty = abs(pinout->element.Pin[0].Y - pin->Y);
		if (x_min == 0 || (tx != 0 && tx < x_min))
			x_min = tx;
		if (y_min == 0 || (ty != 0 && ty < y_min))
			y_min = ty;
		SET_FLAG (DISPLAYNAMEFLAG, pin);
		}
	END_LOOP;

	PAD_LOOP (&pinout->element);
		{
		tx = abs(pinout->element.Pad[0].Point1.X - pad->Point1.X);
		ty = abs(pinout->element.Pad[0].Point1.Y - pad->Point1.Y);
		if (x_min == 0 || (tx != 0 && tx < x_min))
			x_min = tx;
		if (y_min == 0 || (ty != 0 && ty < y_min))
			y_min = ty;
		SET_FLAG (DISPLAYNAMEFLAG, pad);
		}
	END_LOOP;

	if (x_min < y_min)
		RotateElementLowLevel (NULL, &pinout->element,
				pinout->element.BoundingBox.X1,
				pinout->element.BoundingBox.Y1, 1);

	MoveElementLowLevel (NULL, &pinout->element,
				Settings.PinoutOffsetX - pinout->element.BoundingBox.X1,
				Settings.PinoutOffsetY - pinout->element.BoundingBox.Y1);

	if (!pinout_zoom_fit(pinout, 2))
		pinout_zoom_fit(pinout, 3);

	ELEMENTLINE_LOOP (&pinout->element);
		{
		line->Thickness = 0;
		}
	END_LOOP;

	ARC_LOOP (&pinout->element);
		{
		arc->Thickness = 0;
		}
	END_LOOP;

	return pinout;
	}

static void
RedrawPinoutWindow(PinoutType *po)
	{
	GdkWindow	*window = po->drawing_area->window;
	GdkDrawable	*save_drawable;

	if (window)
		{
		/* Setup drawable and zoom factor for drawing routines
		*/
		save_drawable = draw_get_current_drawable();

		SwitchDrawingWindow(po->zoom, window, FALSE, FALSE);

		/* clear background call the drawing routine */
		gdk_draw_rectangle(window, Output.bgGC, TRUE,
					0, 0, MAX_COORD, MAX_COORD);

		/* Panning the output area sets a clip box which will clip our
		|  pinout window.  So we must open up the clipbox.
		*/
		output_clipbox_enable(FALSE);
		DrawElement(&po->element, 0);
		output_clipbox_enable(TRUE);

		/* reset drawing routines to normal operation */
		SwitchDrawingWindow(PCB->Zoom, save_drawable,
					Settings.ShowSolderSide, TRUE);
		}
	}

static gint
expose_event_cb(GtkWidget *widget, GdkEventExpose *ev, PinoutType *po)
	{
	/* Just redraw the complete window.  No pixmap to copy over.
	*/
	RedrawPinoutWindow(po);
	return FALSE;
	}

static void
pinout_close_cb(GtkWidget *widget, PinoutType *pinout)
	{
	gtk_widget_destroy(pinout->top_window);
	}


static void
pinout_destroy_cb(GtkWidget *widget, PinoutType *pinout)
    {
	g_free(pinout);
    }

void
gui_pinout_window_show(OutputType *out, ElementType *element)
	{
	GtkWidget	*button, *viewport, *vbox, *hbox;
	PinoutType	*pinout;
	gchar		*title;

	if (!element)
		return;
	title = g_strdup_printf("%s [%s,%s]",
				UNKNOWN (DESCRIPTION_NAME (element)),
				UNKNOWN (NAMEONPCB_NAME (element)),
				UNKNOWN (VALUE_NAME (element)));

	pinout = pinout_new(element);

	pinout->top_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(pinout->top_window), "destroy",
				G_CALLBACK(pinout_destroy_cb), pinout);
	gtk_window_set_title(GTK_WINDOW(pinout->top_window), title);
	g_free(title);
	gtk_window_set_wmclass(GTK_WINDOW(pinout->top_window),
				"PCB_Pinout", "PCB");
	gtk_container_set_border_width(GTK_CONTAINER(pinout->top_window), 4);
	gtk_window_set_default_size(GTK_WINDOW(pinout->top_window),
				pinout->w_pixels + 50, pinout->h_pixels + 50);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(pinout->top_window), vbox);

	viewport = gtk_viewport_new(NULL, NULL);
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(viewport), GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(vbox), viewport, TRUE, TRUE, 0);

	pinout->drawing_area = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(viewport), pinout->drawing_area);

	g_signal_connect(G_OBJECT(pinout->drawing_area), "expose_event",
				G_CALLBACK(expose_event_cb), pinout);

	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(button), "clicked",
					G_CALLBACK(pinout_close_cb), pinout);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);

	/* If focus were grabbed, output drawing area would loose it which we
	|  don't want.
	*/
	gtk_widget_realize(pinout->top_window);
	gdk_window_set_accept_focus(pinout->top_window->window, FALSE);
	gtk_widget_show_all(pinout->top_window);
	}
