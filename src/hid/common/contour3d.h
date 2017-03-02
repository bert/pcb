typedef struct {
  edge_ref first_edge;

} contour3d;

contour3d *make_contour3d (edge_ref first_edge);
void destroy_contour3d (contour3d *contour);
