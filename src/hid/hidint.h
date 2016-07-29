/* HID internal interfaces.  These may ONLY be called from the HID
   modules, not from the common PCB code.  */

/* These decode the set_layer index. */
#define SL_TYPE(x) ((x) < 0 ? (x) & 0x0f0 : 0)
#define SL_SIDE(x) ((x) & 0x00f)
#define SL_MYSIDE(x) ((((x) & SL_BOTTOM_SIDE)!=0) == (SWAP_IDENT != 0))

/* Called by the init funcs, used to set up hid_list.  */
extern void hid_register_hid (HID * hid);

/* NULL terminated list of all static HID structures.  Built by
   hid_register_hid, used by hid_find_*() and hid_enumerate().  The
   order in this list is the same as the order of hid_register_hid
   calls.  */
extern HID **hid_list;

/* Count of entries in the above.  */
extern int hid_num_hids;

/* Used to cache color lookups.  If set is zero, it looks up the name
   and if found sets val and returns nonzero.  If not found, it
   returns zero.  If set is nonzero, name/val is added to the
   cache.  */
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

/* A HID may use this if it does not need command line arguments in
   any special format; for example, the Lesstif HID needs to use the
   Xt parser, but the Postscript HID can use this function.  */
void hid_parse_command_line (int *argc, char ***argv);

/* Use this to temporarily enable all layers, so that they can be
   exported even if they're not currently visible.  save_array must be
   MAX_ALL_LAYER big. */
void hid_save_and_show_layer_ons (int *save_array);
/* Use this to restore them.  */
void hid_restore_layer_ons (int *save_array);

enum File_Name_Style {
  /* Files for copper layers are named top, groupN, bottom.  */
  FNS_fixed,
  /* Groups with multiple layers are named as above, else the single
     layer name is used.  */
  FNS_single,
  /* The name of the first layer in each group is used.  */
  FNS_first,
};

/* Returns a filename base that can be used to output the layer.  */
const char *layer_type_to_file_name (int idx, int style);
const char *layer_type_to_file_name_ex (int idx, int style, const char *layer_name);

/* Convenience function that calls the expose callback for the item,
   and returns the extents of what was drawn.  */
BoxType *hid_get_extents (void *item);

void derive_default_filename(const char *pcbfile, HID_Attribute *filename_attrib, const char *suffix, char **memory);
