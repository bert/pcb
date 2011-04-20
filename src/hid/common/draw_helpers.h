void common_fill_pcb_polygon (hidGC gc, PolygonType *poly,
                              const BoxType *clip_box);
void common_thindraw_pcb_polygon (hidGC gc, PolygonType *poly,
                                  const BoxType *clip_box);
void common_fill_pcb_pad (hidGC gc, PadType *pad, bool clear, bool mask);
void common_thindraw_pcb_pad (hidGC gc, PadType *pad, bool clear, bool mask);
void common_draw_helpers_init (HID *hid);
