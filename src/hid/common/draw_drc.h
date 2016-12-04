void common_drc_draw_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box);
void common_drc_fill_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box);
void common_drc_thindraw_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box);
void common_drc_fill_pcb_pad (hidGC gc, PadType *pad, bool clear, bool mask);
void common_drc_thindraw_pcb_pad (hidGC gc, PadType *pad, bool clear, bool mask);
void common_drc_pcb_fill_pv (hidGC fg_gc, hidGC bg_gc, PinType *pv, bool drawHole, bool mask);
void common_drc_thindraw_pcb_pv (hidGC fg_gc, hidGC bg_gc, PinType *pv, bool drawHole, bool mask);
void common_draw_drc_class_init (HID_DRAW_CLASS *klass);
void common_draw_drc_init (HID_DRAW *graphics);
