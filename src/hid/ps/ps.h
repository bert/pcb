extern HID ps_hid;
extern void ps_hid_export_to_file (FILE *, HID_Attr_Val *);
extern void ps_start_file (FILE *);
extern void ps_calibrate_1 (double , double , int);
extern void hid_eps_init ();
void ps_ps_init (HID *hid);
void ps_ps_graphics_class_init (HID_DRAW_CLASS *klass);
void ps_ps_graphics_init (HID_DRAW *graphics);
