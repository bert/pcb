#include "borast/borast-traps-private.h"

borast_status_t bo_poly_to_traps (hidGC gc, POLYAREA *poly, borast_traps_t *traps);
borast_status_t bo_poly_to_traps_no_draw (POLYAREA *poly, borast_traps_t *traps);
borast_status_t bo_contour_to_traps (hidGC gc, PLINE *contour, borast_traps_t *traps);
borast_status_t bo_contour_to_traps_no_draw (PLINE *contour, borast_traps_t *traps);
borast_fixed_t _line_compute_intersection_x_for_y (const borast_line_t *line, borast_fixed_t y);


/* More naked API, not tied to polygons */

typedef struct _borast borast_t;

borast_t *bo_init (int max_num_edges);
void bo_free (borast_t *bo);
void bo_add_edge (borast_t *borast, Coord sx, Coord sy, Coord ex, Coord ey, bool outer_contour);
borast_status_t bo_tesselate_to_traps(borast_t *borast, bool combine_y_traps, borast_traps_t *traps);
