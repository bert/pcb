/*!
 * \brief This is the modified GtkSpinbox used for entering Coords.
 *
 * Hopefully it can be used as a template whenever we migrate the
 * rest of the Gtk HID to use GObjects and GtkWidget subclassing.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996, 2004 Thomas Nau
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
 */

#ifndef GHID_COORD_ENTRY_H__
#define GHID_COORD_ENTRY_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS  /* keep c++ happy */

#define GHID_COORD_ENTRY_TYPE            (ghid_coord_entry_get_type ())
#define GHID_COORD_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GHID_COORD_ENTRY_TYPE, GHidCoordEntry))
#define GHID_COORD_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GHID_COORD_ENTRY_TYPE, GHidCoordEntryClass))
#define IS_GHID_COORD_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GHID_COORD_ENTRY_TYPE))
#define IS_GHID_COORD_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GHID_COORD_ENTRY_TYPE))

typedef struct _GHidCoordEntry       GHidCoordEntry;
typedef struct _GHidCoordEntryClass  GHidCoordEntryClass;

/* Step sizes */
enum ce_step_size { CE_TINY, CE_SMALL, CE_MEDIUM, CE_LARGE };

GType ghid_coord_entry_get_type (void);
GtkWidget* ghid_coord_entry_new (Coord min_val, Coord max_val, Coord value,
                                 const Unit *unit, enum ce_step_size step_size);
void ghid_coord_entry_add_entry (GHidCoordEntry *ce, const gchar *name, const gchar *desc);
gchar *ghid_coord_entry_get_last_command (GHidCoordEntry *ce);

Coord ghid_coord_entry_get_value (GHidCoordEntry *ce);
void ghid_coord_entry_set_value (GHidCoordEntry *ce, Coord val);

G_END_DECLS  /* keep c++ happy */
#endif
