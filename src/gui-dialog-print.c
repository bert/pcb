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

/* This file written by Bill Wilson for the PCB Gtk port.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "global.h"

#include "data.h"
#include "dev_ps.h"
#include "dev_rs274x.h"
#include "misc.h"
#include "mymem.h"
#include "print.h"

#include "gui.h"


#define	MIN_SCALE			0.2
#define	MAX_SCALE			5.0


static MediaType	*selected_media = NULL;

static MediaType Media[] =
	{    /* first one is user defined, don't know how it can set set */
	{USERMEDIANAME, 0, 0, 0, 0},
	{"A3",		1169000, 1653000, 50000, 50000},
	{"A4",		826000, 1169000, 50000, 50000},
	{"A5",		583000, 826000, 50000, 50000},
	{"Letter",	850000, 1100000, 50000, 50000},
	{"11x17",	1100000, 1700000, 50000, 50000},
	{"Ledger",	1700000, 1100000, 50000, 50000},
	{"Legal",	850000, 1400000, 50000, 50000},
	{"Executive", 750000, 1000000, 50000, 50000},
	{"C-size",	1700000, 2200000, 50000, 50000},
	{"D-size",	2200000, 3400000, 50000, 50000}
	};

static gint	n_media_types = G_N_ELEMENTS(Media);

static GtkWidget	*rotate_button,
					*mirror_button,
					*invert_button,
					*color_button,
					*outline_button,
					*alignment_button,
					*drillhelper_button,
					*scale_box,
					*scale_spin_button,
					*media_box,
					*panner_box;

static PrintDeviceType	*selected_device;

static gfloat		scale = 1.0;

static gint			w_panner,
					h_panner,
					x_panner,
					y_panner,
					PCB_width,
					PCB_height;

static gboolean		rotate_flag,
					mirror_flag,
					invert_flag,
					color_flag,
					outline_flag,
					alignment_flag,
					drillhelper_flag;


/* ---------------- Handle print panner interface --------------- */

  /* For now, simulate it at a fixed initial setting so layout is printed
  |  centered on the media.
  */
void
print_panner_update(void)
	{
	gint	tmp;

	w_panner = outline_flag ? PCB->MaxWidth/100 : PCB_width;
	h_panner = outline_flag ? PCB->MaxHeight/100 : PCB_height;
	if (alignment_flag)
		{
		w_panner += Settings.AlignmentDistance / 50;
		h_panner += Settings.AlignmentDistance / 50;
		}
	w_panner = (gfloat) w_panner * scale;
	h_panner = (gfloat) h_panner * scale;
	if (rotate_flag)
		{
		tmp = w_panner;
		w_panner = h_panner;
		h_panner = tmp;
		}
	/* update slider/spin values when there's a panner implemented.
	|  For now, leaving x_panner & y_panner at zero means the layout
	|  will be printed centered.
	*/
	}

  /* print_panner_create() gets a vbox where it can create widgets to
  |  pan the layout print position on the target print media.  The Xt PCB
  |  used a panner widget which doesn't map to a Gtk widget.
  |  Here it could be a simple couple of spin buttons that adjust the
  |  top and left offsets.  Or a more complicated visual widget like the
  |  Xt panner could be written.  Or it could just be left alone and we
  |  always get centered printouts...  For now, it's left alone.
  */
void
print_panner_create(GtkWidget *vbox)
	{
	BoxType		*box;

	box = GetDataBoundingBox(PCB->Data);
	PCB_width = (box->X2 - box->X1)/100;
	PCB_height = (box->Y2 - box->Y1)/100;
	print_panner_update();
	}

  /* --------------- end of panner interface -------------- */


static void
print_device_changed_cb(GtkWidget *combo_box, gpointer data)
	{
	PrintDeviceType	*dinfo;
	gint			active;

	active = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_box));
	Settings.selected_print_device = active;
	Settings.config_modified = TRUE;

	dinfo = PrintingDevice[active].Info;
	selected_device = dinfo;

	if (!dinfo->HandlesColor)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(color_button), FALSE);
	gtk_widget_set_sensitive(color_button, dinfo->HandlesColor);

	if (!dinfo->HandlesRotate)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rotate_button), FALSE);
	gtk_widget_set_sensitive(rotate_button, dinfo->HandlesRotate);

	if (!dinfo->HandlesMirror)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mirror_button), FALSE);
	gtk_widget_set_sensitive(mirror_button, dinfo->HandlesMirror);

	if (!dinfo->HandlesScale)
		gtk_widget_hide(scale_box);
	else
		gtk_widget_show(scale_box);

	if (!dinfo->HandlesMedia)
		{
		gtk_widget_hide(media_box);
		gtk_widget_hide(panner_box);	/* Nothing in it for now */
		}
	else
		{
		gtk_widget_show(media_box);
		gtk_widget_show(panner_box);
		}
	}

static void
set_flag_cb(GtkToggleButton *button, gboolean *flag)
	{
	*flag = gtk_toggle_button_get_active(button);
	print_panner_update();
	}

static void
media_changed_cb(GtkWidget *combo_box, gpointer data)
	{
	gint		active;

	active = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_box));
	selected_media = &Media[active];
	Settings.selected_media = active;
	Settings.config_modified = TRUE;
	}

static void
scale_changed_cb(GtkWidget *spin_button, gpointer data)
	{
	scale = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button));
	print_panner_update();
	}

void
gui_dialog_print(void)
	{
	GtkWidget		*dialog, *main_vbox, *vbox, *vbox1, *hbox, *hbox1, *entry;
	GtkWidget		*device_combo, *media_combo;
	OutputType		*out = &Output;
	DeviceInfoType	*dinfo;
	MediaType		*media;
	gint			x_offset = 0, y_offset = 0, i;

	if (!PCB->PrintFilename)
		PCB->PrintFilename = EvaluateFilename(Settings.PrintFile,
				NULL, PCB->Filename ? PCB->Filename : "noname", NULL);

	dialog = gtk_dialog_new_with_buttons(_("PCB Print Layout"),
				GTK_WINDOW(out->top_window),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_CANCEL, GTK_RESPONSE_NONE,
				GTK_STOCK_OK, GTK_RESPONSE_OK,
				NULL);
	gtk_window_set_wmclass(GTK_WINDOW(dialog),
				"PCB_Print", "PCB");

	main_vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 6);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), main_vbox);


	/* ------ Print device driver selection combo box ----- */
	vbox = gui_category_vbox(main_vbox, _("Select Print Driver"),
			4, 2, TRUE, TRUE);
	hbox = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	device_combo = gtk_combo_box_new_text();
	gtk_box_pack_start(GTK_BOX(hbox), device_combo, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(device_combo), "changed",
				G_CALLBACK(print_device_changed_cb), NULL);
	for (i = 0, dinfo = PrintingDevice; dinfo->Query; i++, dinfo++)
		gtk_combo_box_append_text(GTK_COMBO_BOX(device_combo),
					dinfo->Info->Name);

	hbox = gtk_hbox_new(FALSE, 16);
	gtk_box_pack_start(GTK_BOX(main_vbox), hbox, FALSE, FALSE, 0);

	vbox = gui_category_vbox(hbox, _("Print Options"),
				4, 2, TRUE, TRUE);
	gui_check_button_connected(vbox, &rotate_button, FALSE, TRUE, FALSE,
				FALSE, 0, set_flag_cb, &rotate_flag, _("Rotate 90 degrees"));
	gui_check_button_connected(vbox, &mirror_button, FALSE,
				TRUE, FALSE, FALSE, 0,
				set_flag_cb, &mirror_flag, _("Mirror view"));
	gui_check_button_connected(vbox, &invert_button, FALSE,
				TRUE, FALSE, FALSE, 0,
				set_flag_cb, &invert_flag, _("Invert positive/negative"));
	gui_check_button_connected(vbox, &color_button, FALSE,
				TRUE, FALSE, FALSE, 0,
				set_flag_cb, &color_flag, _("Print layer colors"));


	vbox = gui_category_vbox(hbox, _("Add to Printout"),
				4, 2, TRUE, TRUE);
	gui_check_button_connected(vbox, &outline_button, FALSE,
				TRUE, FALSE, FALSE, 0,
				set_flag_cb, &outline_flag, _("Layout outline"));
	gui_check_button_connected(vbox, &alignment_button, FALSE,
				TRUE, FALSE, FALSE, 0,
				set_flag_cb, &alignment_flag, _("Alignment marks"));
	gui_check_button_connected(vbox, &drillhelper_button, FALSE,
				TRUE, FALSE, FALSE, 0,
				set_flag_cb, &drillhelper_flag, _("Drill helpers"));

	hbox = gtk_hbox_new(FALSE, 16);	/* Holds scale, media, and panner */
	gtk_box_pack_start(GTK_BOX(main_vbox), hbox, FALSE, FALSE, 0);
	vbox = gtk_vbox_new(FALSE, 0);	/* Holds scale and media */
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

	/* Create output scale spin button and put it in a box that can
	|  be shown/hidden based on media selected.
	*/
	scale_box = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), scale_box, FALSE, FALSE, 0);
	vbox1 = gui_category_vbox(scale_box, _("Set Printout Scale"),
				4, 2, TRUE, TRUE);
	gui_spin_button(vbox1, &scale_spin_button, scale,
				MIN_SCALE, MAX_SCALE, 0.1, 0.1, 2, 0,
				scale_changed_cb, NULL, FALSE, NULL);

	/* Create media stuff in a box that can be hidden/shown
	*/
	media_box = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), media_box, FALSE, FALSE, 0);
	vbox1 = gui_category_vbox(media_box, _("Select Media"),
				4, 2, TRUE, TRUE);
	hbox1 = gtk_hbox_new(FALSE, 4);/* put in hbox so wont expand horizontally*/
	gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, FALSE, 0);
	media_combo = gtk_combo_box_new_text();
	gtk_box_pack_start(GTK_BOX(hbox1), media_combo, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(media_combo), "changed",
				G_CALLBACK(media_changed_cb), NULL);
	for (media = &Media[0]; media < &Media[n_media_types]; ++media)
		gtk_combo_box_append_text(GTK_COMBO_BOX(media_combo), media->Name);

	/* Create panner stuff in its own box that can be hidden/shown
	*/
	panner_box = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), panner_box, FALSE, FALSE, 0);
	print_panner_create(panner_box);		/* simulation for now */


	vbox = gui_category_vbox(main_vbox, _("Enter Filename"),
				4, 2, TRUE, TRUE);
	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);
	gtk_entry_set_text(GTK_ENTRY(entry), PCB->PrintFilename);

	gtk_widget_show_all(dialog);

	/* Set combo box defaults after above widget creation so "changed" signals
	|  can set button sensitivities and selected media.  Must also do it
	|  after gtk_widget_show_all() for media_box and scale_box visibility.
	*/
	gtk_combo_box_set_active(GTK_COMBO_BOX(device_combo),
				Settings.selected_print_device);
	gtk_combo_box_set_active(GTK_COMBO_BOX(media_combo),
				Settings.selected_media);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
		{
		scale = gtk_spin_button_get_value(GTK_SPIN_BUTTON(scale_spin_button));
		if (selected_device->HandlesMedia)
			{
			x_offset = x_panner;
			y_offset = y_panner;
			x_offset += ((selected_media->Width/100 - w_panner) / 2);
			y_offset += ((selected_media->Height/100 - h_panner) / 2);
			}
		dup_string(&PCB->PrintFilename, gui_entry_get_text(entry));
		if (*PCB->PrintFilename)
			Print(PCB->PrintFilename, scale,
					mirror_flag, rotate_flag, color_flag, invert_flag,
					outline_flag, alignment_flag, drillhelper_flag, FALSE,
					selected_device, selected_media,
					x_offset * 100, y_offset * 100, FALSE);
		}

	gtk_widget_destroy(dialog);
	}
