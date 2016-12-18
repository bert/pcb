typedef struct
{
  /* For edge curves */
  bool is_stitch; /* Allows us to identify the stitch edge along the side of a cylinder */

  /* For curves which are defined along some primitive,
   * such as cirular / b-spline below..
   */

  /* same_sense: true if the parameter value corresponding to the start vertex is
   *             less than that of the end vertex. This has similar effec on
   *             circles to flipping the defined normal.
   *
   * NB: As this parameter was added AFTER writing STEP emission code any edge
   *     with this set to false will probably be broken in the emitted STEP.
   */
  bool same_sense;

  /* For circular curves */
  bool is_round;
  double cx;
  double cy;
  double cz;
  double nx;
  double ny;
  double nz;
  double rx;
  double ry;
  double rz;
  double radius;

  /* For ellipse (in addition to circular items above */
  bool is_ellipse;
  double radius2;

  /* For b-splines */
  bool is_bspline;
  int degree;
  int num_control_points; /* Number of (triplet x,y,z) control points - control points array holds 3x more doubles */
  double *control_points; /* Pointer to array of control points (in x,y,z triplets) */
  /* Number of knots is (num_control_points + degree + 1) */
  double *knots;          /* Pointer to array of knot values */

  /* Rational b-splines */
  double *weights; /* Pointer to array of weights for the control points */

  /* Debug */
  bool is_placeholder; /* For some edge type we don't know how to draw */

  /* XXX: STEP specific - breaks encapsulation */
  step_id infinite_line_identifier;
  step_id edge_identifier;


  /* Rendering data */
  int num_linearised_vertices;
  float *linearised_vertices; /* NB: Does not include the start_point */
} edge_info;

edge_info *make_edge_info (void);
void edge_info_set_round (edge_info *info, double cx, double cy, double cz, double nx, double ny, double nz, double radius);
void edge_info_set_round2 (edge_info *info, double cx, double cy, double cz, double nx, double ny, double nz, double rx, double ry, double rz, double radius);
void edge_info_set_stitch (edge_info *info);
void destroy_edge_info (edge_info *info);
void edge_ensure_linearised (edge_ref edge);
