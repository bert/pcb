
Coord get_min_clearance_for_netclass (char *netclass);
Coord get_max_clearance_for_netclass (char *netclass);
Coord get_clearance_between_netclasses (char *netclass_a, char *netclass_b);
char *get_netclass_for_object (int type, void *ptr1, void *ptr2, void *ptr3);
char *get_netclass_for_via (PinType *via);
char *get_netclass_for_pin (PinType *pin);
char *get_netclass_for_pad (PadType *pad);
char *get_netclass_for_line (LayerType *layer, LineType *line);
char *get_netclass_for_arc (LayerType *layer, ArcType *arc);
char *get_netclass_at_xy (LayerType *layer, Coord x, Coord y);
