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

  /* For b-splines */
  bool is_bspline;
  int degree;
  int num_control_points; /* Number of (triplet x,y,z) control points - control points array holds 3x more doubles */
  double *control_points; /* Pointer to array of control points (in x,y,z triplets) */
  /* Number of knots is (num_control_points + degree + 1) */
  double *knots;          /* Pointer to array of knot values */

  /* Rational b-splines */
  double *weights; /* Pointer to array of weights for the control points */


  /* XXX: STEP specific - breaks encapsulation */
  step_id infinite_line_identifier;
  step_id edge_identifier;
} edge_info;

edge_info *make_edge_info (void);
void edge_info_set_round (edge_info *info, double cx, double cy, double cz, double nx, double ny, double nz, double radius);
void edge_info_set_stitch (edge_info *info);
void destroy_edge_info (edge_info *info);
