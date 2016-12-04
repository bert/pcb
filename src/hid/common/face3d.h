typedef struct {
  double nx, ny, nz; /* Face normal?*/
  bool surface_orientation_reversed;
  GList *contours;
  char *name;

  /* For cylindrical surfaces */
  bool is_cylindrical;
  double cx, cy, cz; /* A point on the axis */
  double ax, ay, az; /* Direction of the axis */
  double radius;

  appearance *appear;

  /* XXX: STEP specific - breaks encapsulation */
  step_id surface_identifier;
  step_id face_identifier;
  step_id face_bound_identifier;
} face3d;

face3d *make_face3d (char *name);
void destroy_face3d (face3d *face);
void face3d_add_contour (face3d *face, contour3d *contour);
void face3d_set_appearance (face3d *face, appearance *appear);
void face3d_set_normal (face3d *face, double nx, double ny, double nz);
void face3d_set_cylindrical (face3d *face, double cx, double cy, double cz, double ax, double ay, double az, double radius);
void face3d_set_surface_orientation_reversed (face3d *face);
