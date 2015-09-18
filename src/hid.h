#ifndef PCB_HID_H
#define PCB_HID_H

#include <stdarg.h>

/* Human Interface Device */

/*

The way the HID layer works is that you instantiate a HID device
structure, and invoke functions through its members.  Code in the
common part of PCB may *not* rely on *anything* other than what's
defined in this file.  Code in the HID layers *may* rely on data and
functions in the common code (like, board size and such) but it's
considered bad form to do so when not needed.

Coordinates are ALWAYS in pcb's default resolution (1/100 mil at the
moment).  Positive X is right, positive Y is down.  Angles are
degrees, with 0 being right (positive X) and 90 being up (negative Y).
All zoom, scaling, panning, and conversions are hidden inside the HID
layers.

The main structure is at the end of this file.

Data structures passed to the HIDs will be copied if the HID needs to
save them.  Data structures retured from the HIDs must not be freed,
and may be changed by the HID in response to new information.

*/

#if defined(__cplusplus) && __cplusplus
extern "C"
{
#endif

/* Like end cap styles.  The cap *always* extends beyond the
   coordinates given, by half the width of the line.  Beveled ends can
   used to make octagonal pads by giving the same x,y coordinate
   twice.  */
  typedef enum
  {
    Trace_Cap,			/* This means we're drawing a trace, which has round caps.  */
    Square_Cap,			/* Square pins or pads. */
    Round_Cap,			/* Round pins or round-ended pads, thermals.  */
    Beveled_Cap			/* Octagon pins or bevel-cornered pads.  */
  } EndCapStyle;

/* The HID may need something more than an "int" for colors, timers,
   etc.  So it passes/returns one of these, which is castable to a
   variety of things.  */
  typedef union
  {
    long lval;
    void *ptr;
  } hidval;

/* This graphics context is an opaque pointer defined by the HID.  GCs
   are HID-specific; attempts to use one HID's GC for a different HID
   will result in a fatal error.  */
  typedef struct hid_gc_struct *hidGC;

#define HIDCONCAT(a,b) a##b

/* This is used to register the action callbacks (for menus and
   whatnot).  HID assumes the following actions are available for its
   use:
	SaveAs(filename);
	Quit();
*/
  typedef struct
  {
    /* This is matched against action names in the GUI configuration */
    char *name;
    /* If this string is non-NULL, the action needs to know the X,Y
       coordinates to act on, and this string may be used to prompt
       the user to select a coordinate.  If NULL, the coordinates may
       be 0,0 if none are known.  */
    const char *need_coord_msg;
    /* Called when the action is triggered.  If this function returns
       non-zero, no further actions will be invoked for this key/mouse
       event.  */
    int (*trigger_cb) (int argc, char **argv, Coord x, Coord y);
    /* Short description that sometimes accompanies the name.  */
    const char *description;
    /* Full allowed syntax; use \n to separate lines.  */
    const char *syntax;
  } HID_Action;

  extern void hid_register_action (HID_Action *);

  extern void hid_register_actions (HID_Action *, int);
#define REGISTER_ACTIONS(a) HIDCONCAT(void register_,a) ()\
{ hid_register_actions(a, sizeof(a)/sizeof(a[0])); }

  /* Note that PCB expects the gui to provide the following actions:

     PCBChanged();
     RouteStylesChanged()
     NetlistChanged()  (but core should call "void NetlistChanged(int);" in netlist.c)
     LayersChanged()
     LibraryChanged()
     Busy()
   */

  extern const char pcbchanged_help[];
  extern const char pcbchanged_syntax[];
  extern const char routestyleschanged_help[];
  extern const char routestyleschanged_syntax[];
  extern const char netlistchanged_help[];
  extern const char netlistchanged_syntax[];
  extern const char layerschanged_help[];
  extern const char layerschanged_syntax[];
  extern const char librarychanged_help[];
  extern const char librarychanged_syntax[];

  int hid_action (const char *action_);
  int hid_actionl (const char *action_, ...);	/* NULL terminated */
  int hid_actionv (const char *action_, int argc_, char **argv_);
  void hid_save_settings (int);
  void hid_load_settings (void);

/* Parse the given command string into action calls, and call
   hid_actionv for each action found.  Accepts both "action(arg1,
   arg2)" and command-style "action arg1 arg2", allowing only one
   action in the later case.  Returns nonzero if the action handler(s)
   return nonzero. */
  int hid_parse_command (const char *str_);

/* Parse the given string into action calls, and call
   hid_actionv for each action found.  Accepts only
   "action(arg1, arg2)" */
  int hid_parse_actions (const char *str_);

  typedef struct
  {
    /* Name of the flag */
    char *name;
    /* Function to call to get the value of the flag.  */
    int (*function) (void *);
    /* Additional parameter to pass to that function.  */
    void *parm;
  } HID_Flag;

  extern void hid_register_flags (HID_Flag *, int);
#define REGISTER_FLAGS(a) HIDCONCAT(void register_,a) ()\
{ hid_register_flags(a, sizeof(a)/sizeof(a[0])); }

/* Looks up one of the flags registered above.  If the flag is
   unknown, returns zero.  */
  int hid_get_flag (const char *name_);

/* Used for HID attributes (exporting and printing, mostly).
   HA_boolean uses int_value, HA_enum sets int_value to the index and
   str_value to the enumeration string.  HID_Label just shows the
   default str_value.  HID_Mixed is a real_value followed by an enum,
   like 0.5in or 100mm. */
  typedef struct
  {
    int int_value;
    const char *str_value;
    double real_value;
    Coord coord_value;
  } HID_Attr_Val;

  enum hids
    { HID_Label, HID_Integer, HID_Real, HID_String,
      HID_Boolean, HID_Enum, HID_Mixed, HID_Path,
      HID_Unit, HID_Coord
    };

  typedef struct
  {
    char *name;
    /* If the help_text is this, usage() won't show this option */
#define ATTR_UNDOCUMENTED ((char *)(1))
    char *help_text;
    enum hids type;
    int min_val, max_val;	/* for integer and real */
    HID_Attr_Val default_val;	/* Also actual value for global attributes.  */
    const char **enumerations;
    /* If set, this is used for global attributes (i.e. those set
       statically with REGISTER_ATTRIBUTES below) instead of changing
       the default_val.  Note that a HID_Mixed attribute must specify a
       pointer to HID_Attr_Val here, and HID_Boolean assumes this is
       "char *" so the value should be initialized to zero, and may be
       set to non-zero (not always one).  */
    void *value;
    int hash; /* for detecting changes. */
  } HID_Attribute;

  extern void hid_register_attributes (HID_Attribute *, int);
#define REGISTER_ATTRIBUTES(a) HIDCONCAT(void register_,a) ()\
{ hid_register_attributes(a, sizeof(a)/sizeof(a[0])); }

/* These three are set by hid_parse_command_line().  */
  extern char *program_name;
  extern char *program_directory;
  extern char *program_basename;

/* These are used for set_layer(). */
#define SL_0_SIDE	0x0000
#define SL_TOP_SIDE	0x0001
#define SL_BOTTOM_SIDE	0x0002
#define SL_INNER_SIDE   0x0004

/*!
 * These are types of "layers" without direct physical representation. Their
 * content can be derived from other layers and element data.
 *
 * These values are used by DrawEverything() in draw.c to ask the active GUI
 * or exporter to act on these layers. Depending on the GUI/exporter they
 * result in a per-layer file, in some additional graphics or just nothing.
 */
#define SL_SILK         0x0010  /* physical layer, deprecated, use LT_SILK */
#define SL_MASK         0x0020
#define SL_PDRILL       0x0030
#define SL_UDRILL       0x0040
#define SL_PASTE        0x0050
#define SL_INVISIBLE    0x0060
#define SL_FAB          0x0070
#define SL_ASSY         0x0080
#define SL_RATS         0x0090

/* Callers should use this.  */
#define SL(type,side) (~0xfff | SL_##type | SL_##side##_SIDE)

/*!
 * These are layers with direct physical representation, like copper, dye
 * or to be milled paths. Their data can't be derived from other layers or
 * element data.
 *
 * To add more layer types, add them to the list here and in layerflags.c.
 * Order of entries in both lists must be the same.
 */
typedef enum
{
  LT_COPPER = 0,
  LT_SILK,
  LT_MASK,           /**< Complements SL_MASK above. */
  LT_PASTE,          /**< Complements SL_PASTE above. */
  LT_OUTLINE,        /**< Board outline; exists only once. */
  LT_ROUTE,
  LT_KEEPOUT,
  LT_FAB,            /**< Complements SL_FAB above. */
  LT_ASSY,           /**< Complements SL_ASSY above. */
  LT_NOTES,
  LT_NUM_LAYERTYPES  /* must be the last one */
} LayertypeType;

/* File Watch flags */
/* Based upon those in dbus/dbus-connection.h */
typedef enum
{
  PCB_WATCH_READABLE = 1 << 0, /**< As in POLLIN */
  PCB_WATCH_WRITABLE = 1 << 1, /**< As in POLLOUT */
  PCB_WATCH_ERROR    = 1 << 2, /**< As in POLLERR */ 
  PCB_WATCH_HANGUP   = 1 << 3  /**< As in POLLHUP */
} PCBWatchFlags;

/* DRC GUI Hooks */
  typedef struct
  {
    int log_drc_overview;
    int log_drc_violations;
    void (*reset_drc_dialog_message) (void);
    void (*append_drc_violation) (DrcViolationType *violation);
    int (*throw_drc_dialog) (void);
  } HID_DRC_GUI;

  typedef struct hid_st HID;
  typedef struct hid_draw_st HID_DRAW;

/* This is the main HID structure.  */
  struct hid_st
  {
    /* The size of this structure.  We use this as a compatibility
       check; a HID built with a different hid.h than we're expecting
       should have a different size here.  */
    int struct_size;

    /* The name of this HID.  This should be suitable for
       command line options, multi-selection menus, file names,
       etc. */
    const char *name;

    /* Likewise, but allowed to be longer and more descriptive.  */
    const char *description;

    /* If set, this is the GUI HID.  Exactly one of these three flags
       must be set; setting "gui" lets the expose callback optimize and
       coordinate itself.  */
    char gui:1;

    /* If set, this is the printer-class HID.  The common part of PCB
       may use this to do command-line printing, without having
       instantiated any GUI HIDs.  Only one printer HID is normally
       defined at a time.  */
    char printer:1;

    /* If set, this HID provides an export option, and should be used as
       part of the File->Export menu option.  Examples are PNG, Gerber,
       and EPS exporters.  */
    char exporter:1;

    /* If set, the redraw code will draw polygons before erasing the
       clearances.  */
    char poly_before:1;

    /* If set, the redraw code will draw polygons after erasing the
       clearances.  Note that HIDs may set both of these, in which case
       polygons will be drawn twice.  */
    char poly_after:1;

    /* Returns a set of resources describing options the export or print
       HID supports.  In GUI mode, the print/export dialogs use this to
       set up the selectable options.  In command line mode, these are
       used to interpret command line options.  If n_ret is non-NULL,
       the number of attributes is stored there.  */
    HID_Attribute *(*get_export_options) (int *n_ret_);

    /* Export (or print) the current PCB.  The options given represent
       the choices made from the options returned from
       get_export_options.  Call with options == NULL to start the
       primary GUI (create a main window, print, export, etc)  */
    void (*do_export) (HID_Attr_Val * options_);

    /* Parse the command line.  Call this early for whatever HID will be
       the primary HID, as it will set all the registered attributes.
       The HID should remove all arguments, leaving any possible file
       names behind.  */
    void (*parse_arguments) (int *argc_, char ***argv_);

    /* This may be called to ask the GUI to force a redraw of a given area */
    void (*invalidate_lr) (int left_, int right_, int top_, int bottom_);
    void (*invalidate_all) (void);
    void (*notify_crosshair_change) (bool changes_complete);
    void (*notify_mark_change) (bool changes_complete);

    /* During redraw or print/export cycles, this is called once per
       layer (or layer group, for copper layers).  If it returns false
       (zero), the HID does not want that layer, and none of the drawing
       functions should be called.  If it returns true (nonzero), the
       items in that layer [group] should be drawn using the various
       drawing functions.  In addition to the MAX_GROUP copper layer
       groups, you may select layers indicated by the macros SL_*
       defined above, or any others with an index of -1.  For copper
       layer groups, you may pass NULL for name to have a name fetched
       from the PCB struct.  The EMPTY argument is a hint - if set, the
       layer is empty, if zero it may be non-empty.  */
    int (*set_layer) (const char *name_, int group_, int _empty);

    /* Tell the GUI the layer last selected has been finished with */
    void (*end_layer) (void);


    HID_DRAW *graphics;

    /* This is for the printer.  If you call this for the GUI, xval and
       yval are ignored, and a dialog pops up to lead you through the
       calibration procedure.  For the printer, if xval and yval are
       zero, a calibration page is printed with instructions for
       calibrating your printer.  After calibrating, nonzero xval and
       yval are passed according to the instructions.  Metric is nonzero
       if the user prefers metric units, else inches are used. */
    void (*calibrate) (double xval_, double yval_);


    /* GUI layout functions.  Not used or defined for print/export
       HIDs.  */

    /* Temporary */
    int (*shift_is_pressed) (void);
    int (*control_is_pressed) (void);
    int (*mod1_is_pressed) (void);
    void (*get_coords) (const char *msg_, Coord *x_, Coord *y_);

    /* Sets the crosshair, which may differ from the pointer depending
       on grid and pad snap.  Note that the HID is responsible for
       hiding, showing, redrawing, etc.  The core just tells it what
       coordinates it's actually using.  Note that this routine may
       need to know what "pcb units" are so it can display them in mm
       or mils accordingly.  If cursor_action is set, the cursor or
       screen may be adjusted so that the cursor and the crosshair are
       at the same point on the screen.  */
    void (*set_crosshair) (int x_, int y_, int cursor_action_);
#define HID_SC_DO_NOTHING                          0
#define HID_SC_WARP_POINTER	                       1
#define HID_SC_PAN_VIEWPORT                        2
#define HID_SC_CENTER_IN_VIEWPORT_AND_WARP_POINTER 3

    /* Causes func to be called at some point in the future.  Timers are
       only good for *one* call; if you want it to repeat, add another
       timer during the callback for the first.  user_data can be
       anything, it's just passed to func.  Times are not guaranteed to
       be accurate.  */
      hidval (*add_timer) (void (*func) (hidval user_data_),
			   unsigned long milliseconds_, hidval user_data_);
    /* Use this to stop a timer that hasn't triggered yet.  */
    void (*stop_timer) (hidval timer_);

    /* Causes func to be called when some condition occurs on the file
       descriptor passed. Conditions include data for reading, writing,
       hangup, and errors. user_data can be anything, it's just passed
       to func. */
      hidval (*watch_file) (int fd_, unsigned int condition_, void (*func_) (hidval watch_, int fd_, unsigned int condition_, hidval user_data_),
        hidval user_data);
    /* Use this to stop a file watch. */
    void (*unwatch_file) (hidval watch_);

    /* Causes func to be called in the mainloop prior to blocking */
      hidval (*add_block_hook) (void (*func_) (hidval data_), hidval data_);
    /* Use this to stop a mainloop block hook. */
    void (*stop_block_hook) (hidval block_hook_);

    /* Various dialogs */

    /* Log a message to the log window.  */
    void (*log) (const char *fmt_, ...);
    void (*logv) (const char *fmt_, va_list args_);

    /* A generic yes/no dialog.  Returns zero if the cancel button is
       pressed, one for the ok button.  If you specify alternate labels
       for ..., they are used instead of the default OK/Cancel ones, and
       the return value is the index of the label chosen.  You MUST pass
       NULL as the last parameter to this.  */
    int (*confirm_dialog) (char *msg_, ...);

    /* A close confirmation dialog for unsaved pages, for example, with
       options "Close without saving", "Cancel" and "Save". Returns zero
       if the close is cancelled, or one if it should proceed. The HID
       is responsible for any "Save" action the user may wish before
       confirming the close.
       */
    int (*close_confirm_dialog) ();
#define HID_CLOSE_CONFIRM_CANCEL 0
#define HID_CLOSE_CONFIRM_OK     1

    /* Just prints text.  */
    void (*report_dialog) (char *title_, char *msg_);

    /* Prompts the user to enter a string, returns the string.  If
       default_string isn't NULL, the form is pre-filled with this
       value.  "msg" is like "Enter value:".  */
    char *(*prompt_for) (const char *msg_, const char *default_string_);

    /* Prompts the user for a filename or directory name.  For GUI
       HID's this would mean a file select dialog box.  The 'flags'
       argument is the bitwise OR of the following values.  */
#define HID_FILESELECT_READ  0x01
    
    /* The function calling hid->fileselect will deal with the case
       where the selected file already exists.  If not given, then the
       gui will prompt with an "overwrite?" prompt.  Only used when
       writing.
    */
#define HID_FILESELECT_MAY_NOT_EXIST 0x02

    /* The call is supposed to return a file template (for gerber
       output for example) instead of an actual file.  Only used when
       writing.
    */
#define HID_FILESELECT_IS_TEMPLATE 0x04

    /* title may be used as a dialog box title.  Ignored if NULL.
     *
     * descr is a longer help string.  Ignored if NULL.
     *
     * default_file is the default file name.  Ignored if NULL.
     *
     * default_ext is the default file extension, like ".pdf".
     * Ignored if NULL.
     *
     * history_tag may be used by the GUI to keep track of file
     * history.  Examples would be "board", "vendor", "renumber",
     * etc.  If NULL, no specific history is kept.
     *
     * flags are the bitwise or of the HID_FILESELECT defines above
     */
    
    char *(*fileselect) (const char *title_, const char *descr_,
			 char *default_file_, char *default_ext_,
			 const char *history_tag_, int flags_);

    /* A generic dialog to ask for a set of attributes.  If n_attrs is
       zero, attrs(.name) must be NULL terminated.  Returns non-zero if
       an error occurred (usually, this means the user cancelled the
       dialog or something). title is the title of the dialog box 
       descr (if not NULL) can be a longer description of what the
       attributes are used for.  The HID may choose to ignore it or it
       may use it for a tooltip or text in a dialog box, or a help
       string. 
    */
    int (*attribute_dialog) (HID_Attribute * attrs_,
			     int n_attrs_, HID_Attr_Val * results_,
			     const char * title_, const char * descr_);

    /* This causes a second window to display, which only shows the
       selected item. The expose callback is called twice; once to size
       the extents of the item, and once to draw it.  To pass magic
       values, pass the address of a variable created for this
       purpose.  */
    void (*show_item) (void *item_);

    /* Something to alert the user.  */
    void (*beep) (void);

    /* Used by optimizers and autorouter to show progress to the user.
       Pass all zeros to flush display and remove any dialogs.
       Returns nonzero if the user wishes to cancel the operation.  */
    int (*progress) (int so_far_, int total_, const char *message_);

    HID_DRC_GUI *drc_gui;

    void (*edit_attributes) (char *owner, AttributeListType *attrlist_);

    /* Debug drawing support. These APIs must be implemented (non NULL),
     * but they do not have to be functional. request_debug_draw can
     * return NULL to indicate debug drawing is not permitted.
     *
     * Debug drawing is not gauranteed to be re-entrant.
     * The caller must not nest requests for debug drawing.
     */

    /* Request permission for debug drawing
     *
     * Returns a HID_DRAW pointer which should be used rather than the global
     * gui->graphics-> for making drawing calls. If the return value is NULL,
     * then permission has been denied, and the drawing must not continue.
     */
    HID_DRAW *(*request_debug_draw) (void);

    /* Flush pending drawing to the screen
     *
     * May be implemented as a NOOP if the GUI has chosen to send the
     * debug drawing directly to the screen.
     */
    void (*flush_debug_draw)   (void);

    /* When finished, the user must inform the GUI to clean up resources
     *
     * Any remaining rendering will be flushed to the screen.
     */
    void (*finish_debug_draw)  (void);

    /* Notification to the GUI around saving the PCB file.
     *
     * Called with a false parameter before the save, called again
     * with true after the save.
     *
     * Allows GUIs which watch for file-changes on disk to ignore
     * our deliberate changes.
     */
    void (*notify_save_pcb) (const char *filename, bool done);

    /* Notification to the GUI that the PCB file has been renamed. */
    void (*notify_filename_changed) (void);
  };

/* Call this as soon as possible from main().  No other HID calls are
   valid until this is called.  */
  void hid_init (void);

/* When PCB runs in interactive mode, this is called to instantiate
   one GUI HID which happens to be the GUI.  This HID is the one that
   interacts with the mouse and keyboard.  */
  HID *hid_find_gui ();

/* Finds the one printer HID and instantiates it.  */
  HID *hid_find_printer (void);

/* Finds the indicated exporter HID and instantiates it.  */
  HID *hid_find_exporter (const char *);

/* This returns a NULL-terminated array of available HIDs.  The only
   real reason to use this is to locate all the export-style HIDs. */
  HID **hid_enumerate (void);

/* This function (in the common code) will be called whenever the GUI
   needs to redraw the screen, print the board, or export a layer.  If
   item is not NULL, only draw the given item.  Item is only non-NULL
   if the HID was created via show_item.

   Each time func is called, it should do the following:

   * allocate any colors needed, via get_color.

   * cycle through the layers, calling set_layer for each layer to be
     drawn, and only drawing elements (all or specified) of desired
     layers.

   Do *not* assume that the hid that is passed is the GUI hid.  This
   callback is also used for printing and exporting. */
  void hid_expose_callback (HID * hid_, struct BoxType *region_, void *item_);

/* This is initially set to a "no-gui" gui, and later reset by
   main. hid_expose_callback also temporarily set it for drawing. */
  extern HID *gui;

/* This is either NULL or points to the current HID that is being called to
	do the exporting. The gui HIDs set and unset this var.*/
  extern HID *exporter;

/* This is either NULL or points to the current HID_Action that is being
   called. The action launcher sets and unsets this variable. */
  extern HID_Action *current_action;

/* The GUI may set this to be approximately the PCB size of a pixel,
   to allow for near-misses in selection and changes in drawing items
   smaller than a screen pixel.  */
  extern int pixel_slop;

#if defined(__cplusplus) && __cplusplus
}
#endif

#endif
