/*!
 * \file src/hid/hidint.h
 *
 * \brief HID internal interfaces.
 *
 * These may ONLY be called from the HID modules, not from the common
 * PCB code.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2006 DJ Delorie
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

/* These decode the set_layer index. */
#define SL_TYPE(x) ((x) < 0 ? (x) & 0x0f0 : 0)
#define SL_SIDE(x) ((x) & 0x00f)
#define SL_MYSIDE(x) ((((x) & SL_BOTTOM_SIDE)!=0) == (SWAP_IDENT != 0))

/*!
 * \brief Called by the init funcs, used to set up hid_list.
 */
extern void hid_register_hid (HID * hid);

/*!
 * \brief NULL terminated list of all static HID structures.
 *
 * Built by hid_register_hid, used by hid_find_*() and hid_enumerate().
 * The order in this list is the same as the order of hid_register_hid
 * calls.
 */
extern HID **hid_list;

/*!
 * \brief Count of entries in the above.
 */
extern int hid_num_hids;

/*!
 * \brief Used to cache color lookups.
 *
 * If set is zero, it looks up the name and if found sets val and
 * returns nonzero.  If not found, it returns zero.  If set is nonzero,
 * name/val is added to the cache.
 */
int hid_cache_color (int set, const char *name, hidval * val, void **cache);

typedef struct HID_AttrNode
{
  struct HID_AttrNode *next;
  HID_Attribute *attributes;
  int n;
} HID_AttrNode;

extern HID_AttrNode *hid_attr_nodes;

HID_Action *hid_find_action (const char *name);

HID_Flag *hid_find_flag (const char *name);

/*!
 * \brief A HID may use this if it does not need command line arguments
 * in any special format.
 *
 * For example, the Lesstif HID needs to use the Xt parser, but the
 * Postscript HID can use this function.
 */
void hid_parse_command_line (int *argc, char ***argv);

/*!
 * \brief Use this to temporarily enable all layers, so that they can be
 * exported even if they're not currently visible.
 *
 * save_array must be MAX_ALL_LAYER big.
 */
void hid_save_and_show_layer_ons (int *save_array);

/*!
 * \brief Use this to restore them.
 */
void hid_restore_layer_ons (int *save_array);

enum File_Name_Style {
  FNS_fixed,
    /*!< Files for copper layers are named top, groupN, bottom. */
  FNS_single,
    /*!< Groups with multiple layers are named as above, else
         the single layer name is used. */
  FNS_first,
    /*!< The name of the first layer in each group is used. */
};

/* Returns a filename base that can be used to output the layer.  */
const char *layer_type_to_file_name (int idx, int style);
const char *layer_type_to_file_name_ex (int idx, int style, const char *layer_name);

/*!
 * \brief Convenience function that calls the expose callback for the
 * item, and returns the extents of what was drawn.
 */
BoxType *hid_get_extents (void *item);

void derive_default_filename(const char *pcbfile, HID_Attribute *filename_attrib, const char *suffix, char **memory);
