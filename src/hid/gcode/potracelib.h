/*!
 * \file src/hid/gcode/potracelib.h
 *
 * \brief this file defines the API for the core Potrace library.
 *
 * For a more detailed description of the API, see doc/potracelib.txt.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2001-2007 Peter Selinger.
 *
 * This file is part of Potrace. It is free software and it is covered
 * by the GNU General Public License. See the file COPYING for details.
 */

#ifndef POTRACELIB_H
#define POTRACELIB_H

/* ---------------------------------------------------------------------- */
/* tracing parameters */

/* turn policies */
#define POTRACE_TURNPOLICY_BLACK 0
#define POTRACE_TURNPOLICY_WHITE 1
#define POTRACE_TURNPOLICY_LEFT 2
#define POTRACE_TURNPOLICY_RIGHT 3
#define POTRACE_TURNPOLICY_MINORITY 4
#define POTRACE_TURNPOLICY_MAJORITY 5
#define POTRACE_TURNPOLICY_RANDOM 6

/*!
 * \brief Structure to hold progress bar callback data.
 */
struct potrace_progress_s
{
  void (*callback) (double progress, void *privdata);
        /*!< callback function. */
  void *data; /*!< callback function's private data. */
  double min, max; /*!< Desired range of progress, e.g. 0.0 to 1.0. */
  double epsilon; /*!< Granularity: can skip smaller increments. */
};
typedef struct potrace_progress_s potrace_progress_t;

/*!
 * \brief Structure to hold tracing parameters.
 */
struct potrace_param_s
{
  int turdsize; /*!< Area of largest path to be ignored. */
  int turnpolicy; /*!< Resolves ambiguous turns in path decomposition. */
  double alphamax; /*!< Corner threshold. */
  int opticurve; /*!< Use curve optimization? */
  double opttolerance; /*!< Curve optimization tolerance. */
  potrace_progress_t progress; /*!< Progress callback function. */
};
typedef struct potrace_param_s potrace_param_t;

/* ---------------------------------------------------------------------- */
/* bitmaps */

/*!
 * \brief Native word size.
 */
typedef unsigned long potrace_word;

/*!
 * \brief Internal bitmap format.
 *
 * The n-th scanline starts at scanline(n) = (map + n*dy).
 * Raster data is stored as a sequence of potrace_words (NOT bytes).
 * The leftmost bit of scanline n is the most significant bit of
 * scanline(n)[0].
 */
struct potrace_bitmap_s
{
  int w; /*!< Width, in pixels. */
  int h; /*!< Height, in pixels. */
  int dy; /*!< Words per scanline (not bytes). */
  potrace_word *map; /*!< Raw data, dy*h words. */
};
typedef struct potrace_bitmap_s potrace_bitmap_t;

/* ---------------------------------------------------------------------- */
/* curves */

/*!
 * \brief Point.
 */
struct potrace_dpoint_s
{
  double x, y;
};
typedef struct potrace_dpoint_s potrace_dpoint_t;

/* segment tags */
#define POTRACE_CURVETO 1
#define POTRACE_CORNER 2

/*!
 * \brief Closed curve segment.
 */
struct potrace_curve_s
{
  int n; /*!< Number of segments. */
  int *tag; /*!< tag[n]: POTRACE_CURVETO or POTRACE_CORNER. */
    potrace_dpoint_t (*c)[3];
        /*!< c[n][3]: control points.\n
         * c[n][0] is unused for tag[n]=POTRACE_CORNER. */
};
typedef struct potrace_curve_s potrace_curve_t;

/*!
 * \brief Linked list of signed curve segments.
 *
 * Also carries a tree structure.
 */
struct potrace_path_s
{
  int area; /*!< Area of the bitmap path. */
  int sign; /*!< '+' or '-', depending on orientation. */
  potrace_curve_t curve; /*!< This path's vector data. */

  struct potrace_path_s *next; /*!< Linked list structure. */

  struct potrace_path_s *childlist; /*!< Tree structure. */
  struct potrace_path_s *sibling; /*!< Tree structure. */

  struct potrace_privpath_s *priv; /*!< Private state. */
};
typedef struct potrace_path_s potrace_path_t;

/* ---------------------------------------------------------------------- */

#define POTRACE_STATUS_OK         0
#define POTRACE_STATUS_INCOMPLETE 1

/*!
 * \brief Potrace state. */
struct potrace_state_s
{
  int status;
  potrace_path_t *plist; /*!< Vector data. */

  struct potrace_privstate_s *priv; /*!< Private state. */
};
typedef struct potrace_state_s potrace_state_t;

/* ---------------------------------------------------------------------- */
/* API functions */

/*!
 * \brief Get default parameters.
 */
potrace_param_t *potrace_param_default (void);

/*!
 * \brief Free parameter set.
 */
void potrace_param_free (potrace_param_t * p);

/*!
 * \brief Trace a bitmap.
 */
potrace_state_t *potrace_trace (const potrace_param_t * param,
				const potrace_bitmap_t * bm);

/*!
 * \brief Free a Potrace state.
 */
void potrace_state_free (potrace_state_t * st);

/*!
 * \brief Return a static plain text version string identifying this
 * version of potracelib.
 */
char *potrace_version (void);

#endif /* POTRACELIB_H */
