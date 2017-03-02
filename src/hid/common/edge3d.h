typedef struct
{
  bool is_stitch;
  bool is_round;
  double cx;
  double cy;
  double radius;
} edge_info;

edge_info *make_edge_info (bool is_stitch, bool is_round, double cx, double cy, double radius);
void destroy_edge_info (edge_info *info);
