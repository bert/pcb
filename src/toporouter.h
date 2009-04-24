/*
 *                            COPYRIGHT
 *
 *  Topological Autorouter for 
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2009 Anthony Blake
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for email:
 *  Anthony Blake, tonyb33@gmail.com
 *
 */

#ifndef __TOPOROUTER_INCLUDED__
#define __TOPOROUTER_INCLUDED__

#include "data.h"
#include "macro.h"
#include "autoroute.h"
#include "box.h"
#include "create.h"
#include "draw.h"
#include "error.h"
#include "find.h"
#include "heap.h"
#include "rtree.h"
#include "misc.h"
#include "mymem.h"
#include "polygon.h"
#include "rats.h"
#include "remove.h"
#include "thermal.h"
#include "undo.h"
#include "global.h"

#include "gts.h"

#include <stdlib.h>
#include <getopt.h>

#define TOPOROUTER_FLAG_VERBOSE       (1<<0)
#define TOPOROUTER_FLAG_DUMP_CDTS     (1<<1)
#define TOPOROUTER_FLAG_DEBUG_CDTS    (1<<2)

#define OUTPUT_ENABLED 

#ifdef OUTPUT_ENABLED
  #include <cairo.h>
#endif

#define EPSILON 0.0001f

/**
 * TOPOROUTER_IS_BBOX:
 * @obj: a #GtsObject.
 *
 * Evaluates to %TRUE if @obj is a #ToporouterBBox, %FALSE otherwise.
 */
#define TOPOROUTER_IS_BBOX(obj)      (gts_object_is_from_class (obj,\
						    toporouter_bbox_class ()))
/**
 * TOPOROUTER_BBOX:
 * @obj: a descendant of #ToporouterBBox.
 *
 * Casts @obj to #ToporouterBBox.
 */
#define TOPOROUTER_BBOX(obj)          GTS_OBJECT_CAST (obj,\
						  ToporouterBBox,\
						  toporouter_bbox_class ())
/**
 * TOPOROUTER_BBOX_CLASS:
 * @klass: a descendant of #ToporouterBBoxClass.
 *
 * Casts @klass to #ToporouterBBoxClass.
 */
#define TOPOROUTER_BBOX_CLASS(klass)  GTS_OBJECT_CLASS_CAST (klass,\
						  ToporouterBBoxClass,\
						  toporouter_bbox_class ())


typedef enum { 
  PAD, 
  PIN, 
  VIA, 
  VIA_SHADOW, 
  LINE, 
  OTHER, 
  BOARD, 
  EXPANSION_AREA, 
  PLANE, 
  TEMP
} toporouter_term_t;

struct _ToporouterBBox {
  GtsBBox b;

  toporouter_term_t type;
  void *data;
  int layer;

  GtsSurface *surface;
  GtsTriangle *enclosing;

  GSList *constraints;
  GtsPoint *point,
           *realpoint;

  char *netlist, *style;
};

struct _ToporouterBBoxClass {
  GtsBBoxClass parent_class;
};

typedef struct _ToporouterBBox ToporouterBBox;
typedef struct _ToporouterBBoxClass ToporouterBBoxClass;

/**
 * TOPOROUTER_IS_EDGE:
 * @obj: a #GtsObject.
 *
 * Evaluates to %TRUE if @obj is a #ToporouterEdge, %FALSE otherwise.
 */
#define TOPOROUTER_IS_EDGE(obj)      (gts_object_is_from_class (obj,\
						    toporouter_edge_class ()))
/**
 * TOPOROUTER_EDGE:
 * @obj: a descendant of #ToporouterEdge.
 *
 * Casts @obj to #ToporouterEdge.
 */
#define TOPOROUTER_EDGE(obj)          GTS_OBJECT_CAST (obj,\
						  ToporouterEdge,\
						  toporouter_edge_class ())
/**
 * TOPOROUTER_EDGE_CLASS:
 * @klass: a descendant of #ToporouterEdgeClass.
 *
 * Casts @klass to #ToporouterEdgeClass.
 */
#define TOPOROUTER_EDGE_CLASS(klass)  GTS_OBJECT_CLASS_CAST (klass,\
						  ToporouterEdgeClass,\
						  toporouter_edge_class ())

struct _ToporouterEdge {
  GtsEdge e;
  NetListType *netlist;
};

struct _ToporouterEdgeClass {
  GtsEdgeClass parent_class;
};

typedef struct _ToporouterEdge ToporouterEdge;
typedef struct _ToporouterEdgeClass ToporouterEdgeClass;

/**
 * TOPOROUTER_IS_VERTEX:
 * @obj: a #GtsObject.
 *
 * Evaluates to %TRUE if @obj is a #ToporouterVertex, %FALSE otherwise.
 */
#define TOPOROUTER_IS_VERTEX(obj)      (gts_object_is_from_class (obj,\
						    toporouter_vertex_class ()))
/**
 * TOPOROUTER_VERTEX:
 * @obj: a descendant of #ToporouterVertex.
 *
 * Casts @obj to #ToporouterVertex.
 */
#define TOPOROUTER_VERTEX(obj)          GTS_OBJECT_CAST (obj,\
						  ToporouterVertex,\
						  toporouter_vertex_class ())
/**
 * TOPOROUTER_VERTEX_CLASS:
 * @klass: a descendant of #ToporouterVertexClass.
 *
 * Casts @klass to #ToporouterVertexClass.
 */
#define TOPOROUTER_VERTEX_CLASS(klass)  GTS_OBJECT_CLASS_CAST (klass,\
						  ToporouterVertexClass,\
						  toporouter_vertex_class ())
/*
#define TOPOROUTER_VERTEX_TYPE_NULL       (1<<0)
#define TOPOROUTER_VERTEX_TYPE_PIN        (1<<1)
#define TOPOROUTER_VERTEX_TYPE_PIN_POLY   (1<<2)
#define TOPOROUTER_VERTEX_TYPE_VIA        (1<<3)
#define TOPOROUTER_VERTEX_TYPE_VIA_POLY   (1<<4)
#define TOPOROUTER_VERTEX_TYPE_PAD        (1<<5)
#define TOPOROUTER_VERTEX_TYPE_CONSTRAINT (1<<6)
#define TOPOROUTER_VERTEX_TYPE_BOARD      (1<<7)
#define TOPOROUTER_VERTEX_TYPE_LINE       (1<<8)
*/

#define VERTEX_FLAG_ATTACH   (1<<7)
#define VERTEX_FLAG_VIZ      (1<<1)
#define VERTEX_FLAG_CCW      (1<<2)
#define VERTEX_FLAG_CW       (1<<3)
#define VERTEX_FLAG_RED      (1<<4)
#define VERTEX_FLAG_GREEN    (1<<5)
#define VERTEX_FLAG_BLUE     (1<<6)

struct _ToporouterVertex {
  GtsVertex v;
//  void *data;
//  unsigned int type;
  GSList *boxes;
  struct _ToporouterVertex *parent;

  //GSList *visible;
  GSList *regions;
  GSList *zlink;
  //GSList *removed;
  GHashTable* corridors;

  guint flags;

  gdouble gcost, hcost;

  GSList *attached;

};

struct _ToporouterVertexClass {
  GtsVertexClass parent_class;
};

typedef struct _ToporouterVertex ToporouterVertex;
typedef struct _ToporouterVertexClass ToporouterVertexClass;

/**
 * TOPOROUTER_IS_CONSTRAINT:
 * @obj: a #GtsObject.
 *
 * Evaluates to %TRUE if @obj is a #ToporouterConstraint, %FALSE otherwise.
 */
#define TOPOROUTER_IS_CONSTRAINT(obj)      (gts_object_is_from_class (obj,\
						    toporouter_constraint_class ()))
/**
 * TOPOROUTER_CONSTRAINT:
 * @obj: a descendant of #ToporouterConstraint.
 *
 * Casts @obj to #ToporouterConstraint.
 */
#define TOPOROUTER_CONSTRAINT(obj)          GTS_OBJECT_CAST (obj,\
						  ToporouterConstraint,\
						  toporouter_constraint_class ())
/**
 * TOPOROUTER_CONSTRAINT_CLASS:
 * @klass: a descendant of #ToporouterConstraintClass.
 *
 * Casts @klass to #ToporouterConstraintClass.
 */
#define TOPOROUTER_CONSTRAINT_CLASS(klass)  GTS_OBJECT_CLASS_CAST (klass,\
						  ToporouterConstraintClass,\
						  toporouter_constraint_class ())
/*
#define TOPOROUTER_CONSTRAINT_TYPE_NULL       (1<<0)
#define TOPOROUTER_CONSTRAINT_TYPE_PAD        (1<<1)
#define TOPOROUTER_CONSTRAINT_TYPE_PIN_POLY   (1<<2)
#define TOPOROUTER_CONSTRAINT_TYPE_VIA_POLY   (1<<3)
#define TOPOROUTER_CONSTRAINT_TYPE_BOARD      (1<<4)
#define TOPOROUTER_CONSTRAINT_TYPE_LINE       (1<<5)
*/

struct _ToporouterConstraint {
  GtsConstraint c;
  ToporouterBBox *box;
//  void *data;
//  unsigned int type;
//  GSList *objectconstraints; /* list of all constraints for object */
};

struct _ToporouterConstraintClass {
  GtsConstraintClass parent_class;
};

typedef struct {
  gdouble x, y;
} toporouter_spoint_t;

typedef struct _ToporouterConstraint ToporouterConstraint;
typedef struct _ToporouterConstraintClass ToporouterConstraintClass;

typedef enum {
  INCIDENT,
  ATTACHED,
  ATTACHED_TEMP
} attachment_type_t;

typedef enum {
  CCW,
  CW
} toporouter_direction_t;

typedef struct {
  gdouble r;
  gdouble angle;
  ToporouterVertex *a, *b, *p;

  char *netlist, *style;
} toporouter_attachment_t;

typedef struct {
  ToporouterVertex *v;
  GSList *edges;
} toporouter_visibility_t;

typedef struct {
  GtsSurface *surface;
//  GtsTriangle *t;
//  GtsVertex *v1, *v2, *v3;
  
  GSList *vertices;
  GSList *constraints; 
  GSList *edges;

} toporouter_layer_t;

#define TOPOROUTER_VERTEX_REGION(x) ((toporouter_vertex_region_t *)x)
typedef struct {

  GSList *points;
  ToporouterVertex *v1, *v2;
  ToporouterVertex *origin;

} toporouter_vertex_region_t;

typedef struct {

  ToporouterBBox *src, *dest;
 /*
  GSList *src_points,
         *dest_points;

  GSList *src_box_points;
  */
  ToporouterVertex *curpoint;
  
  GSList *path;
} toporouter_route_t;


typedef struct {
  GSList *bboxes;
  GNode *bboxtree;

  toporouter_layer_t *layers;
  
  int flags;

  /* settings: */
  guint viamax;
  gdouble viacost;
  gdouble stublength;

  FILE *debug;
} toporouter_t;

typedef struct {
#ifdef CAIRO_H
  cairo_t *cr;
  cairo_surface_t *surface;
#endif  
  
  double s; /* scale factor */

  int mode;
  void *data;

  char *filename;
  double iw, ih; /* image dimensions */
} drawing_context_t;

#endif /* __TOPOROUTER_INCLUDED__ */
