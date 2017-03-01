#include "borast/borast-traps-private.h"

borast_status_t bo_poly_to_traps (hidGC gc, POLYAREA *poly, borast_traps_t *traps);
borast_status_t bo_contour_to_traps (hidGC gc, PLINE *contour, borast_traps_t *traps);
borast_status_t bo_contour_to_traps_no_draw (PLINE *contour, borast_traps_t *traps);
borast_fixed_t _line_compute_intersection_x_for_y (const borast_line_t *line, borast_fixed_t y);
