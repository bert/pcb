#ifndef PCB_HID_COMMON_HIDNOGUI_H
#define PCB_HID_COMMON_HIDNOGUI_H

void common_nogui_init (HID *hid);
void common_nogui_graphics_class_init (HID_DRAW_CLASS *klass);
void common_nogui_graphics_init (HID_DRAW *graphics);
HID *hid_nogui_get_hid (void);

#endif
