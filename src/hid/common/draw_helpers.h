void common_gui_draw_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box);
void common_fill_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box);
void common_thindraw_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box);
void common_fill_pcb_pad (hidGC gc, PadType *pad, bool clear, bool mask);
void common_thindraw_pcb_pad (hidGC gc, PadType *pad, bool clear, bool mask);
void common_fill_pcb_pv (hidGC fg_gc, hidGC bg_gc, PinType *pv, bool drawHole, bool mask);
void common_thindraw_pcb_pv (hidGC fg_gc, hidGC bg_gc, PinType *pv, bool drawHole, bool mask);
void common_draw_helpers_class_init (HID_DRAW_CLASS *klass);
void common_draw_helpers_init (HID_DRAW *graphics);
