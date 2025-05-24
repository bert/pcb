/*!
 * \file src/fanout.h
 *
 * \brief Actions for creating a fanout for a BGA package.
 * 
 * \author Copyright (C) 2025 Bert Timmerman <bert.timmerman@xs4all.nl>
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
 *
 * Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Contact addresses for paper mail and Email:
 * Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 * haceaton@aplcomm.jhuapl.edu
 */


#ifndef PCB_FANOUT_H
#define PCB_FANOUT_H

#include <config.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h> /* not part of the C++ standard */
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <limits.h>

#include "config.h"
#include "global.h"
#include "../globalconst.h"

#include "create.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "flags.h"
#include "misc.h"
#include "rats.h"
#include "undo.h"


#define FANOUT_MICROVIA_MIN_DRILL_SIZE 0.25 /* mm */
#define FANOUT_VIA_MIN_DRILL_SIZE 0.5 /* mm */
#define FANOUT_EXCEPTION_DELIMITERS ", "


enum fanout_style
{
  FANOUT_NO_STYLE, /* i.e. NULL */
  FANOUT_THROUGH_VIA_IN_PAD,
  FANOUT_BLIND_VIA_IN_PAD,
  FANOUT_MICRO_VIA_IN_PAD,
  FANOUT_QUADRANT_DOG_BONE_THROUGH_VIA,
  FANOUT_QUADRANT_DOG_BONE_BLIND_VIA,
  FANOUT_QUADRANT_DOG_BONE_MICRO_VIA,
  FANOUT_NUMBER_OF_STYLES
};


/*!
 * \brief Struct for containing relevant information.
 *
 */
typedef struct
fanout_struct
{
  ElementType *found_element;
    /*!< Pointer to the found element. */
  char *refdes;
    /*!< Pointer to a reference designator. */
  char *footprint_name;
    /*!< Pointer to a footprint name. */
  char *element_name;
    /*!< Pointer to an existing element name. */
  char *units;
    /*!< Units for dimensions. */
  double rotation;
    /*!< Rotation of the element. */
  int num_rows;
    /*!< Number of rows. */
  int num_columns;
    /*!< Number of columns.*/
  double pad_diameter;
    /*!< Pad diameter. */
  double pad_clearance;
    /*!< Pad clearance. */
  double pad_soldermask_clearance;
    /*!< Pad soldermask clearance. */
  char *exceptions;
    /*!< List of missing pads (exceptions). */
  double hor_pitch;
    /*!< Horizontal pitch between centerlines of pads. */
  double vert_pitch;
    /*!< Vertical pitch between centerlines of pads. */
  int staggered_pitch;
    /*!< True if staggered pitch. */
  int fiducials;
    /*!< True if footprint has fiducials. */
  int via_in_pad;
    /*!< Do we allow via in pad. */
  double fiducial_diameter;
    /*!< Diameter of the copper part of the fiducial. */
  double fiducial_copper_clearance;
    /*!< Clearance with copper. */
  double fiducial_soldermask_clearance;
    /*!< Clearance of the soldermask. */
  double courtyard_length;
    /*!< Length (X) indicating where the fanout traces stop. */
  double courtyard_width;
    /*!< Width (Y) indicating where the fanout traces stop. */
  int num_required_layers;
    /*!< The required amount of layers to fan out. */
  char *layer_stack;
    /*!< Layer stack. */
  char *recognised_layer_names;
    /*!< [VCC, vcc, VCCIO, vccio, GND, gnd, DGND, ...]. */
  char *netlist;
    /*!< If we have a netlist, use it. */
  char *pin_out_information;
    /*!< If we have pinout information, use it. */
  char *routing_style;
    /*!< If we have a routing style, use it.
     * Or use the current routing style if \c NULL. */
  char *style;
    /*!< If we have a fanout style, use it. */
  double via_drill_size;
    /*!< Drill size for all vias in fanout. */
  double via_annulus_size;
    /*!< Annulus size for all vias in fanout. */
  double via_copper_clearance;
    /*!< Annulus copper clearance for all vias in fanout. */
  double via_soldermask_clearance;
    /*!< Annulus soldermask clearance for all vias in fanout. */
  FlagType via_flags;
    /*!< Flags for the via. */
  double trace_width;
    /*!< Trace width for all dogbone traces in fan out. */
  double trace_copper_clearance;
    /*!< Copper clearance for all dogbone traces in fanout. */
  double trace_soldermask_clearance;
    /*!< Soldermask clearance for all dogbone traces in fanout. */
} FanOutStruct;


/* Forward declarations. */
FanOutStruct *fanout_new ();
static int fanout_free (FanOutStruct *fanout);
static int fanout (int argc, char **argv, Coord x, Coord y);


#endif /* PCB_FANOUT_H */
