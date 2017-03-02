typedef struct {
  int id;
  char *name;
  appearance *appear;
  GList *edges;
  GList *vertices;
  GList *faces;
} object3d;

void object3d_test_init (void);
void object3d_draw_debug (void);
object3d *make_object3d (char *name);
void destroy_object3d (object3d *object);
void object3d_set_appearance (object3d *object, appearance *appear);
void object3d_add_edge (object3d *object, edge_ref edge);
void object3d_add_vertex (object3d *object, vertex3d *vertex);
void object3d_add_face (object3d *object, face3d *face);
object3d *object3d_create_test_cube (void);
GList *object3d_from_board_outline (void);
