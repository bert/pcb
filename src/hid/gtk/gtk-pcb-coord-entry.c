
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "gtkhid.h"
#include "gui.h"
#include "pcb-printf.h"

#include "gtk-pcb-coord-entry.h"

enum {
  UNIT_CHANGE_SIGNAL,
  LAST_SIGNAL
};

static guint gtk_pcb_coord_entry_signals[LAST_SIGNAL] = { 0 };

struct _GtkPcbCoordEntry
{
  GtkSpinButton parent;

  Coord min_value;
  Coord max_value;
  Coord value;

  enum ce_step_size step_size;
  const Unit *unit;
};

struct _GtkPcbCoordEntryClass
{
  GtkSpinButtonClass parent_class;

  void (* change_unit) (GtkPcbCoordEntry *, const Unit *);
};

/* SIGNAL HANDLERS */
static void
gtk_pcb_coord_entry_popup_cb (GtkPcbCoordEntry *ce, gpointer data)
{
  /* TODO: Add unit chooser to menu */
}

static gboolean
gtk_pcb_coord_entry_output_cb (GtkPcbCoordEntry *ce, gpointer data)
{
  GtkAdjustment *adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (ce));
  double value = gtk_adjustment_get_value (adj);
  gchar *text;

  text = pcb_g_strdup_printf ("%.*f %s", ce->unit->default_prec, value, ce->unit->suffix);
  gtk_entry_set_text (GTK_ENTRY (ce), text);
  g_free (text);
   
  return TRUE;
}

static gboolean
gtk_pcb_coord_text_changed_cb (GtkPcbCoordEntry *entry, gpointer data)
{
  const char *text;
  char *suffix;
  const Unit *new_unit;
  double value;

  /* Check if units have changed */
  text = gtk_entry_get_text (GTK_ENTRY (entry));
  value = strtod (text, &suffix);
  new_unit = get_unit_struct (suffix);
  if (new_unit && new_unit != entry->unit)
    {
      entry->value = unit_to_coord (new_unit, value);
      g_signal_emit (entry, gtk_pcb_coord_entry_signals[UNIT_CHANGE_SIGNAL], 0, new_unit);
    }

  return FALSE;
}

static gboolean
gtk_pcb_coord_value_changed_cb (GtkPcbCoordEntry *ce, gpointer data)
{
  GtkAdjustment *adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (ce));

  /* Re-calculate internal value */
  double value = gtk_adjustment_get_value (adj);
  ce->value = unit_to_coord (ce->unit, value);
  /* Handle potential unit changes */
  gtk_pcb_coord_text_changed_cb (ce, data);

  return FALSE;
}

static void
gtk_pcb_coord_entry_change_unit (GtkPcbCoordEntry *ce, const Unit *new_unit)
{
  double climb_rate = 0.0;
  GtkAdjustment *adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (ce));

  ce->unit = new_unit;
  /* Re-calculate min/max values for spinbox */
  gtk_adjustment_configure (adj, coord_to_unit (new_unit, ce->value),
                                 coord_to_unit (new_unit, ce->min_value),
                                 coord_to_unit (new_unit, ce->max_value),
                                 coord_to_unit (new_unit, ce->unit->step_small),
                                 coord_to_unit (new_unit, ce->unit->step_medium),
                                 0.0);

  switch (ce->step_size)
    {
    case CE_TINY: climb_rate = new_unit->step_tiny; break;
    case CE_SMALL: climb_rate = new_unit->step_small; break;
    case CE_MEDIUM: climb_rate = new_unit->step_medium; break;
    case CE_LARGE: climb_rate = new_unit->step_large; break;
    }
  gtk_spin_button_configure (GTK_SPIN_BUTTON (ce), adj, climb_rate,
                             new_unit->default_prec + strlen (new_unit->suffix));
}

/* CONSTRUCTOR */
static void
gtk_pcb_coord_entry_init (GtkPcbCoordEntry *ce)
{
  /* Hookup signal handlers */
  g_signal_connect (G_OBJECT (ce), "focus_out_event",
                    G_CALLBACK (gtk_pcb_coord_text_changed_cb), NULL);
  g_signal_connect (G_OBJECT (ce), "value_changed",
                    G_CALLBACK (gtk_pcb_coord_value_changed_cb), NULL);
  g_signal_connect (G_OBJECT (ce), "populate_popup",
                    G_CALLBACK (gtk_pcb_coord_entry_popup_cb), NULL);
  g_signal_connect (G_OBJECT (ce), "output",
                    G_CALLBACK (gtk_pcb_coord_entry_output_cb), NULL);
}

static void
gtk_pcb_coord_entry_class_init (GtkPcbCoordEntryClass *klass)
{
  klass->change_unit = gtk_pcb_coord_entry_change_unit;

  /* GtkAutoComplete *ce : the object acted on */
  /* const Unit *new_unit: the new unit that was set */
  gtk_pcb_coord_entry_signals[UNIT_CHANGE_SIGNAL] =
    g_signal_new ("change-unit",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GtkPcbCoordEntryClass, change_unit),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE,
                  1, G_TYPE_POINTER);

}

/* PUBLIC FUNCTIONS */
GType
gtk_pcb_coord_entry_get_type (void)
{
  static GType ce_type = 0;

  if (!ce_type)
    {
      const GTypeInfo ce_info =
      {
	sizeof (GtkPcbCoordEntryClass),
	NULL, /* base_init */
	NULL, /* base_finalize */
	(GClassInitFunc) gtk_pcb_coord_entry_class_init,
	NULL, /* class_finalize */
	NULL, /* class_data */
	sizeof (GtkPcbCoordEntry),
	0,    /* n_preallocs */
	(GInstanceInitFunc) gtk_pcb_coord_entry_init,
      };

      ce_type = g_type_register_static (GTK_TYPE_SPIN_BUTTON,
                                        "GtkPcbCoordEntry",
                                        &ce_info,
                                        0);
    }

  return ce_type;
}

GtkWidget *
gtk_pcb_coord_entry_new (Coord min_val, Coord max_val, Coord value,
                         const Unit *unit, enum ce_step_size step_size)
{
  /* Setup spinbox min/max values */
  double small_step, big_step;
  GtkAdjustment *adj;
  GtkPcbCoordEntry *ce = g_object_new (GTK_PCB_COORD_ENTRY_TYPE, NULL);

  ce->unit = unit;
  ce->min_value = min_val;
  ce->max_value = max_val;
  ce->value = value;

  ce->step_size = step_size;
  switch (step_size)
    {
    case CE_TINY:
      small_step = unit->step_tiny;
      big_step   = unit->step_small;
      break;
    case CE_SMALL:
      small_step = unit->step_small;
      big_step   = unit->step_medium;
      break;
    case CE_MEDIUM:
      small_step = unit->step_medium;
      big_step   = unit->step_large;
      break;
    case CE_LARGE:
      small_step = unit->step_large;
      big_step   = unit->step_huge;
      break;
    default:
      small_step = big_step = 0;
      break;
    }

  adj = GTK_ADJUSTMENT (gtk_adjustment_new (coord_to_unit (unit, value),
                                            coord_to_unit (unit, min_val),
                                            coord_to_unit (unit, max_val),
                                            small_step, big_step, 0.0));
  gtk_spin_button_configure (GTK_SPIN_BUTTON (ce), adj, small_step,
                             unit->default_prec + strlen (unit->suffix));
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (ce), FALSE);

  return GTK_WIDGET (ce);
}

Coord
gtk_pcb_coord_entry_get_value (GtkPcbCoordEntry *ce)
{
  return ce->value;
}

void
gtk_pcb_coord_entry_set_value (GtkPcbCoordEntry *ce, Coord val)
{
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (ce),
                             coord_to_unit (ce->unit, val));
}

