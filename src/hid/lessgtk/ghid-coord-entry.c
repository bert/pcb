/*!
 * \file src/hid/lessgtk/ghid-coord-entry.c
 *
 * \brief Implementation of GHidCoordEntry widget.
 *
 * This widget is a modified spinbox for the user to enter
 * pcb coords. It is assigned a default unit (for display),
 * but this can be changed by the user by typing a new one
 * or right-clicking on the box.
 *
 * Internally, it keeps track of its value in pcb coords.
 * From the user's perspective, it uses natural human units.
 */

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "gtkhid.h"
#include "gui.h"
#include "pcb-printf.h"

#include "ghid-coord-entry.h"

enum {
  UNIT_CHANGE_SIGNAL,
  LAST_SIGNAL
};

static guint ghid_coord_entry_signals[LAST_SIGNAL] = { 0 };

struct _GHidCoordEntry
{
  GtkSpinButton parent;

  Coord min_value;
  Coord max_value;
  Coord value;

  enum ce_step_size step_size;
  const Unit *unit;
};

struct _GHidCoordEntryClass
{
  GtkSpinButtonClass parent_class;

  void (* change_unit) (GHidCoordEntry *, const Unit *);
};

/* SIGNAL HANDLERS */
/*! \brief Callback for "Change Unit" menu click */
static void
menu_item_activate_cb (GtkMenuItem *item, GHidCoordEntry *ce)
{
  const char *text = gtk_menu_item_get_label (item);
  const Unit *unit = get_unit_struct (text);
  
  g_signal_emit (ce, ghid_coord_entry_signals[UNIT_CHANGE_SIGNAL], 0, unit);
}

/*! \brief Callback for context menu creation */
static void
ghid_coord_entry_popup_cb (GHidCoordEntry *ce, GtkMenu *menu, gpointer data)
{
  int i, n;
  const Unit *unit_list;
  GtkWidget *menu_item, *submenu;

  /* Build submenu */
  n = get_n_units ();
  unit_list = get_unit_list ();

  submenu = gtk_menu_new ();
  for (i = 0; i < n; ++i)
    {
      menu_item = gtk_menu_item_new_with_label (unit_list[i].suffix);
      g_signal_connect (G_OBJECT (menu_item), "activate",
                        G_CALLBACK (menu_item_activate_cb), ce);
      gtk_menu_shell_append (GTK_MENU_SHELL (submenu), menu_item);
      gtk_widget_show (menu_item);
    }

  /* Add submenu to menu */
  menu_item = gtk_separator_menu_item_new ();
  gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show (menu_item);

  menu_item = gtk_menu_item_new_with_label (_("Change Units"));
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), submenu);
  gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show (menu_item);
}

/*! \brief Callback for user output */
static gboolean
ghid_coord_entry_output_cb (GHidCoordEntry *ce, gpointer data)
{
  GtkAdjustment *adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (ce));
  double value = gtk_adjustment_get_value (adj);
  gchar *text;

  text = pcb_g_strdup_printf ("%.*f %s", ce->unit->default_prec, value, ce->unit->suffix);
  gtk_entry_set_text (GTK_ENTRY (ce), text);
  g_free (text);
   
  return TRUE;
}

/*! \brief Callback for user input */
static gboolean
ghid_coord_text_changed_cb (GHidCoordEntry *entry, gpointer data)
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
      g_signal_emit (entry, ghid_coord_entry_signals[UNIT_CHANGE_SIGNAL], 0, new_unit);
    }

  return FALSE;
}

/*! \brief Callback for change in value (input or ^v clicks) */
static gboolean
ghid_coord_value_changed_cb (GHidCoordEntry *ce, gpointer data)
{
  GtkAdjustment *adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (ce));

  /* Re-calculate internal value */
  double value = gtk_adjustment_get_value (adj);
  ce->value = unit_to_coord (ce->unit, value);
  /* Handle potential unit changes */
  ghid_coord_text_changed_cb (ce, data);

  return FALSE;
}

/*! \brief Change the unit used by a coord entry
 *
 *  \param [in] ce         The entry to be acted on
 *  \parin [in] new_unit   The new unit to be used
 */
static void
ghid_coord_entry_change_unit (GHidCoordEntry *ce, const Unit *new_unit)
{
  double climb_rate = 0.0;
  GtkAdjustment *adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (ce));

  ce->unit = new_unit;
  /* Re-calculate min/max values for spinbox */
  gtk_adjustment_configure (adj, coord_to_unit (new_unit, ce->value),
                                 coord_to_unit (new_unit, ce->min_value),
                                 coord_to_unit (new_unit, ce->max_value),
                                 ce->unit->step_small,
                                 ce->unit->step_medium,
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
ghid_coord_entry_init (GHidCoordEntry *ce)
{
  /* Hookup signal handlers */
  g_signal_connect (G_OBJECT (ce), "focus_out_event",
                    G_CALLBACK (ghid_coord_text_changed_cb), NULL);
  g_signal_connect (G_OBJECT (ce), "value_changed",
                    G_CALLBACK (ghid_coord_value_changed_cb), NULL);
  g_signal_connect (G_OBJECT (ce), "populate_popup",
                    G_CALLBACK (ghid_coord_entry_popup_cb), NULL);
  g_signal_connect (G_OBJECT (ce), "output",
                    G_CALLBACK (ghid_coord_entry_output_cb), NULL);
}

static void
ghid_coord_entry_class_init (GHidCoordEntryClass *klass)
{
  klass->change_unit = ghid_coord_entry_change_unit;

  /* GtkAutoComplete *ce : the object acted on */
  /* const Unit *new_unit: the new unit that was set */
  ghid_coord_entry_signals[UNIT_CHANGE_SIGNAL] =
    g_signal_new ("change-unit",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GHidCoordEntryClass, change_unit),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE,
                  1, G_TYPE_POINTER);

}

/* PUBLIC FUNCTIONS */
GType
ghid_coord_entry_get_type (void)
{
  static GType ce_type = 0;

  if (!ce_type)
    {
      const GTypeInfo ce_info =
      {
	sizeof (GHidCoordEntryClass),
	NULL, /* base_init */
	NULL, /* base_finalize */
	(GClassInitFunc) ghid_coord_entry_class_init,
	NULL, /* class_finalize */
	NULL, /* class_data */
	sizeof (GHidCoordEntry),
	0,    /* n_preallocs */
	(GInstanceInitFunc) ghid_coord_entry_init,
      };

      ce_type = g_type_register_static (GTK_TYPE_SPIN_BUTTON,
                                        "GHidCoordEntry",
                                        &ce_info,
                                        0);
    }

  return ce_type;
}

/*! \brief Create a new GHidCoordEntry
 *
 *  \param [in] min_val    The minimum allowed value, in pcb coords
 *  \param [in] max_val    The maximum allowed value, in pcb coords
 *  \param [in] value      The default value, in pcb coords
 *  \param [in] unit       The default unit
 *  \param [in] step_size  How large the default increments should be
 *
 *  \return a freshly-allocated GHidCoordEntry
 */
GtkWidget *
ghid_coord_entry_new (Coord min_val, Coord max_val, Coord value,
                         const Unit *unit, enum ce_step_size step_size)
{
  /* Setup spinbox min/max values */
  double small_step, big_step;
  GtkAdjustment *adj;
  GHidCoordEntry *ce = g_object_new (GHID_COORD_ENTRY_TYPE, NULL);

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

/*! \brief Gets a GHidCoordEntry's value, in pcb coords */
Coord
ghid_coord_entry_get_value (GHidCoordEntry *ce)
{
  return ce->value;
}

/*! \brief Sets a GHidCoordEntry's value, in pcb coords */
void
ghid_coord_entry_set_value (GHidCoordEntry *ce, Coord val)
{
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (ce),
                             coord_to_unit (ce->unit, val));
}

