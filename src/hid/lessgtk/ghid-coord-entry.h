/* This is the modified GtkSpinbox used for entering Coords.
 * Hopefully it can be used as a template whenever we migrate the
 * rest of the Gtk HID to use GObjects and GtkWidget subclassing.
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
