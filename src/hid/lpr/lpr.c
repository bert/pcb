/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "global.h"
#include "data.h"
#include "misc.h"

#include "hid.h"
#include "../hidint.h"
#include "../ps/ps.h"
#include "hid/common/hidinit.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented PS function %s.\n", __FUNCTION__); abort()

static HID_Attribute base_lpr_options[] = {
  {"lprcommand", "Command to print",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_lprcommand 0
};

#define NUM_OPTIONS (sizeof(lpr_options)/sizeof(lpr_options[0]))

static HID_Attribute *lpr_options = 0;
static int num_lpr_options = 0;
static HID_Attr_Val *lpr_values;

static HID_Attribute *
lpr_get_export_options (int *n)
{
  /*
   * We initialize the default value in this manner because the GUI
   * HID's may want to free() this string value and replace it with a
   * new one based on how a user fills out a print dialog.
   */
  if (base_lpr_options[HA_lprcommand].default_val.str_value == NULL)
    {
      base_lpr_options[HA_lprcommand].default_val.str_value = strdup("lpr");
    }

  if (lpr_options == 0)
    {
      HID_Attribute *ps_opts = ps_hid.get_export_options (&num_lpr_options);
      lpr_options =
	(HID_Attribute *) calloc (num_lpr_options, sizeof (HID_Attribute));
      memcpy (lpr_options, ps_opts, num_lpr_options * sizeof (HID_Attribute));
      memcpy (lpr_options, base_lpr_options, sizeof (base_lpr_options));
      lpr_values =
	(HID_Attr_Val *) calloc (num_lpr_options, sizeof (HID_Attr_Val));
    }
  if (n)
    *n = num_lpr_options;
  return lpr_options;
}

static void
lpr_do_export (HID_Attr_Val * options)
{
  FILE *f;
  int i;
  char *filename;

  if (!options)
    {
      lpr_get_export_options (0);
      for (i = 0; i < num_lpr_options; i++)
	lpr_values[i] = lpr_options[i].default_val;
      options = lpr_values;
    }

  filename = options[HA_lprcommand].str_value;

  printf ("LPR: open %s\n", filename);
  f = popen (filename, "w");
  if (!f)
    {
      perror (filename);
      return;
    }

  ps_start_file (f);
  ps_hid_export_to_file (f, options);

  fclose (f);
}

static void
lpr_parse_arguments (int *argc, char ***argv)
{
  lpr_get_export_options (0);
  hid_register_attributes (lpr_options, num_lpr_options);
  hid_parse_command_line (argc, argv);
}

static void
lpr_calibrate (double xval, double yval)
{
  ps_calibrate_1 (xval, yval, 1);
}

HID lpr_hid = {
  sizeof (HID),
  "lpr",
  "Postscript print.",
  0, 1, 0, 1, 0, 0,
  lpr_get_export_options,
  lpr_do_export,
  lpr_parse_arguments,
  0 /* lpr_invalidate_lr */ ,
  0 /* lpr_invalidate_all */ ,
  0 /* lpr_set_layer */ ,
  0 /* lpr_make_gc */ ,
  0 /* lpr_destroy_gc */ ,
  0 /* lpr_use_mask */ ,
  0 /* lpr_set_color */ ,
  0 /* lpr_set_line_cap */ ,
  0 /* lpr_set_line_width */ ,
  0 /* lpr_set_draw_xor */ ,
  0 /* lpr_set_draw_faded */ ,
  0 /* lpr_set_line_cap_angle */ ,
  0 /* lpr_draw_line */ ,
  0 /* lpr_draw_arc */ ,
  0 /* lpr_draw_rect */ ,
  0 /* lpr_fill_circle */ ,
  0 /* lpr_fill_polygon */ ,
  0 /* lpr_fill_pcb_polygon */ ,
  0 /* lpr_thindraw_pcb_polygon */ ,
  0 /* lpr_fill_rect */ ,
  lpr_calibrate,
  0 /* lpr_shift_is_pressed */ ,
  0 /* lpr_control_is_pressed */ ,
  0 /* lpr_mod1_is_pressed */ ,
  0 /* lpr_get_coords */ ,
  0 /* lpr_set_crosshair */ ,
  0 /* lpr_add_timer */ ,
  0 /* lpr_stop_timer */ ,
  0 /* lpr_watch_file */ ,
  0 /* lpr_unwatch_file */ ,
  0 /* lpr_add_block_hook */ ,
  0 /* lpr_stop_block_hook */ ,
  0 /* lpr_log */ ,
  0 /* lpr_logv */ ,
  0 /* lpr_confirm_dialog */ ,
  0 /* lpr_close_confirm_dialog */ ,
  0 /* lpr_report_dialog */ ,
  0 /* lpr_prompt_for */ ,
  0 /* lpr_fileselect */ ,
  0 /* lpr_attribute_dialog */ ,
  0 /* lpr_show_item */ ,
  0 /* lpr_beep */ ,
  0 /* lpr_progress */ ,
  0 /* lpr_drc_gui */ ,
  0 /* lpr_edit_attributes */
};

void
hid_lpr_init ()
{
  apply_default_hid (&lpr_hid, &ps_hid);
  apply_default_hid (&lpr_hid, 0);
  hid_register_hid (&lpr_hid);
}
