typedef struct
{
  /* For edge curves */
  bool is_stitch; /* Allows us to identify the stitch edge along the side of a cylinder */

  /* For circular curves */
  bool is_round;
  double cx;
  double cy;
  double cz;
  double nx;
  double ny;
  double nz;
  double radius;

  /* XXX: STEP specific - breaks encapsulation */
  step_id infinite_line_identifier;
  step_id edge_identifier;
} edge_info;

edge_info *make_edge_info (void);
void edge_info_set_round (edge_info *info, double cx, double cy, double cz, double nx, double ny, double nz, double radius);
void edge_info_set_stitch (edge_info *info);
void destroy_edge_info (edge_info *info);
