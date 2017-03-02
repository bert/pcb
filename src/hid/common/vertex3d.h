typedef struct
{
  double x;
  double y;
  double z;
  int id;
} vertex3d;

vertex3d *make_vertex3d (double x, double y, double z);
void destroy_vertex3d (vertex3d *v);
