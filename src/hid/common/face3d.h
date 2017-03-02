typedef struct {
  double nx, ny, nz; /* Face normal?*/
  GList *contours;
} face3d;

face3d *make_face3d (void);
void destroy_face3d (face3d *face);
void face3d_add_contour (face3d *face, edge_ref contour);
