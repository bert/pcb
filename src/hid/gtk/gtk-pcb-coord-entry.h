/* This is the modified GtkSpinbox used for entering Coords.
 * Hopefully it can be used as a template whenever we migrate the
 * rest of the Gtk HID to use GObjects and GtkWidget subclassing.
 */
#ifndef GTK_PCB_COORD_ENTRY_H__
#define GTK_PCB_COORD_ENTRY_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS  /* keep c++ happy */

#define GTK_PCB_COORD_ENTRY_TYPE            (gtk_pcb_coord_entry_get_type ())
#define GTK_PCB_COORD_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_PCB_COORD_ENTRY_TYPE, GtkPcbCoordEntry))
#define GTK_PCB_COORD_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_PCB_COORD_ENTRY_TYPE, GtkPcbCoordEntryClass))
#define IS_GTK_PCB_COORD_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_PCB_COORD_ENTRY_TYPE))
#define IS_GTK_PCB_COORD_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_PCB_COORD_ENTRY_TYPE))

typedef struct _GtkPcbCoordEntry       GtkPcbCoordEntry;
typedef struct _GtkPcbCoordEntryClass  GtkPcbCoordEntryClass;

/* Step sizes */
enum ce_step_size { CE_TINY, CE_SMALL, CE_MEDIUM, CE_LARGE };

GType gtk_pcb_coord_entry_get_type (void);
GtkWidget* gtk_pcb_coord_entry_new (Coord min_val, Coord max_val, Coord value,
                                    const Unit *unit, enum ce_step_size step_size);
void gtk_pcb_coord_entry_add_entry (GtkPcbCoordEntry *ce, const gchar *name, const gchar *desc);
gchar *gtk_pcb_coord_entry_get_last_command (GtkPcbCoordEntry *ce);

Coord gtk_pcb_coord_entry_get_value (GtkPcbCoordEntry *ce);
void gtk_pcb_coord_entry_set_value (GtkPcbCoordEntry *ce, Coord val);

G_END_DECLS  /* keep c++ happy */
#endif
