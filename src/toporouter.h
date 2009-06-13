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

#include <assert.h>
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

#include <sys/time.h>

#define TOPOROUTER_FLAG_VERBOSE     (1<<0)
#define TOPOROUTER_FLAG_HARDDEST    (1<<1)
#define TOPOROUTER_FLAG_HARDSRC     (1<<2)
#define TOPOROUTER_FLAG_MATCH       (1<<3)

#if TOPO_OUTPUT_ENABLED
  #include <cairo.h>
#endif

#define EPSILON 0.0001f

//#define DEBUG_EXPORT 1
//#define DEBUG_ROUTE 1
//#define DEBUG_IMPORT 1
//#define DEBUG_ORDERING 1
//#define DEBUG_CHECK_OPROUTE 1
//#define DEBUG_MERGING 1
//#define DEBUG_CLUSTER_FIND 1

#define edge_v1(e) (GTS_SEGMENT(e)->v1)
#define edge_v2(e) (GTS_SEGMENT(e)->v2)
#define tedge_v1(e) (TOPOROUTER_VERTEX(GTS_SEGMENT(e)->v1))
#define tedge_v2(e) (TOPOROUTER_VERTEX(GTS_SEGMENT(e)->v2))

#define edge_routing(e) (TOPOROUTER_IS_CONSTRAINT(e) ? TOPOROUTER_CONSTRAINT(e)->routing : e->routing)

#define edge_routing_next(e,list) ((list->next) ? TOPOROUTER_VERTEX(list->next->data) : TOPOROUTER_VERTEX(edge_v2(e)))
#define edge_routing_prev(e,list) ((list->prev) ? TOPOROUTER_VERTEX(list->prev->data) : TOPOROUTER_VERTEX(edge_v1(e)))

#define vx(v) (GTS_POINT(v)->x)
#define vy(v) (GTS_POINT(v)->y)
#define vz(v) (GTS_POINT(v)->z)

#define close_enough_xy(a,b) (vx(a) > vx(b) - EPSILON && vx(a) < vx(b) + EPSILON && vy(a) > vy(b) - EPSILON && vy(a) < vy(b) + EPSILON)

#define TOPOROUTER_IS_BBOX(obj)      (gts_object_is_from_class (obj, toporouter_bbox_class ()))
#define TOPOROUTER_BBOX(obj)          GTS_OBJECT_CAST (obj, toporouter_bbox_t, toporouter_bbox_class ())
#define TOPOROUTER_BBOX_CLASS(klass)  GTS_OBJECT_CLASS_CAST (klass, toporouter_bbox_class_t, toporouter_bbox_class ())

typedef enum {
  PAD, 
  PIN, 
  VIA, 
  ARC, 
  VIA_SHADOW, 
  LINE, 
  OTHER, 
  BOARD, 
  EXPANSION_AREA, 
  POLYGON, 
  TEMP
} toporouter_term_t;

struct _toporouter_bbox_t {
  GtsBBox b;

  toporouter_term_t type;
  void *data;
  int layer;

  GtsSurface *surface;
  GtsTriangle *enclosing;

  GSList *constraints;
  GtsPoint *point,
           *realpoint;

//  char *netlist, *style;

  struct toporouter_cluster_t *cluster;

};

struct _toporouter_bbox_class_t {
  GtsBBoxClass parent_class;
};

typedef struct _toporouter_bbox_t toporouter_bbox_t;
typedef struct _toporouter_bbox_class_t toporouter_bbox_class_t;

#define TOPOROUTER_IS_EDGE(obj)      (gts_object_is_from_class (obj, toporouter_edge_class ()))
#define TOPOROUTER_EDGE(obj)          GTS_OBJECT_CAST (obj, toporouter_edge_t, toporouter_edge_class ())
#define TOPOROUTER_EDGE_CLASS(klass)  GTS_OBJECT_CLASS_CAST (klass, toporouter_edge_class_t, toporouter_edge_class ())

#define EDGE_FLAG_DIRECTCONNECTION (1<<0)

struct _toporouter_edge_t {
  GtsEdge e;
  //NetListType *netlist;

  guint flags;

  GList *routing;
};

struct _toporouter_edge_class_t {
  GtsEdgeClass parent_class;
};

typedef struct _toporouter_edge_t toporouter_edge_t;
typedef struct _toporouter_edge_class_t toporouter_edge_class_t;

#define TOPOROUTER_IS_VERTEX(obj)      (gts_object_is_from_class (obj, toporouter_vertex_class ()))
#define TOPOROUTER_VERTEX(obj)          GTS_OBJECT_CAST (obj, toporouter_vertex_t, toporouter_vertex_class ())
#define TOPOROUTER_VERTEX_CLASS(klass)  GTS_OBJECT_CLASS_CAST (klass, toporouter_vertex_class_t, toporouter_vertex_class ())

#define VERTEX_FLAG_VIZ      (1<<1)
#define VERTEX_FLAG_CCW      (1<<2)
#define VERTEX_FLAG_CW       (1<<3)
#define VERTEX_FLAG_RED      (1<<4)
#define VERTEX_FLAG_GREEN    (1<<5)
#define VERTEX_FLAG_BLUE     (1<<6)
#define VERTEX_FLAG_TEMP              (1<<7)
#define VERTEX_FLAG_ROUTE             (1<<8)
#define VERTEX_FLAG_FAKEV_OUTSIDE_SEG (1<<9)
#define VERTEX_FLAG_FAKE     (1<<10)

struct _toporouter_vertex_t {
  GtsVertex v;
  //GSList *boxes;
  struct _toporouter_bbox_t *bbox;

  struct _toporouter_vertex_t *parent;
  struct _toporouter_vertex_t *child;
  struct _toporouter_vertex_t *fakev;
//  struct _toporouter_vertex_t *cdest;
 
  gdouble pullx, pully;

  toporouter_edge_t *routingedge;

//  GSList *zlink;

  guint flags;

  gdouble gcost, hcost;

  struct _toporouter_arc_t *arc;

  struct _toporouter_oproute_t *oproute;
  struct _toporouter_route_t *route;

  gdouble thickness;

};

struct _toporouter_vertex_class_t {
  GtsVertexClass parent_class;
};

typedef struct _toporouter_vertex_t toporouter_vertex_t;
typedef struct _toporouter_vertex_class_t toporouter_vertex_class_t;

#define TOPOROUTER_IS_CONSTRAINT(obj)      (gts_object_is_from_class (obj, toporouter_constraint_class ()))
#define TOPOROUTER_CONSTRAINT(obj)          GTS_OBJECT_CAST (obj, toporouter_constraint_t, toporouter_constraint_class ())
#define TOPOROUTER_CONSTRAINT_CLASS(klass)  GTS_OBJECT_CLASS_CAST (klass, toporouter_constraint_class_t, toporouter_constraint_class ())

struct _toporouter_constraint_t {
  GtsConstraint c;
  toporouter_bbox_t *box;
  GList *routing;
};

struct _toporouter_constraint_class_t {
  GtsConstraintClass parent_class;
};

typedef struct {
  gdouble x, y;
} toporouter_spoint_t;

typedef struct _toporouter_constraint_t toporouter_constraint_t;
typedef struct _toporouter_constraint_class_t toporouter_constraint_class_t;

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
  toporouter_vertex_t *a, *b, *p;

  char *netlist, *style;
} toporouter_attachment_t;

typedef struct {
  toporouter_vertex_t *v;
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
  toporouter_vertex_t *v1, *v2;
  toporouter_vertex_t *origin;

} toporouter_vertex_region_t;


struct _toporouter_route_t {

//  toporouter_bbox_t *src;

//  GSList *dests;
  
  struct toporouter_cluster_t *src, *dest;

  gdouble score;

  toporouter_vertex_t *curpoint;
  GHashTable *alltemppoints; 
  GSList *path;
/*
  toporouter_bbox_t *destbox;
#ifdef DEBUG_ROUTE
  toporouter_vertex_t *curdest;
#endif
*/
  guint flags;

  GSList *destvertices, *srcvertices;
  GSList *keepoutlayers;

  GSList *topopath;

};

typedef struct _toporouter_route_t toporouter_route_t;

#define TOPOROUTER_ROUTE(x) ((toporouter_route_t *)x)

struct _toporouter_clearance_t {
  gpointer data;
  gint wind;
  gdouble ms;
};

typedef struct _toporouter_clearance_t toporouter_clearance_t;
#define TOPOROUTER_CLEARANCE(x) ((toporouter_clearance_t *)x)

#define TOPOROUTER_OPROUTE(x) ((toporouter_oproute_t *)x)

#define oproute_next(a,b) (b->next ? TOPOROUTER_ARC(b->next->data) : a->term2)
#define oproute_prev(a,b) (b->prev ? TOPOROUTER_ARC(b->prev->data) : a->term1)

#define TOPOROUTER_SERPINTINE(x) ((toporouter_serpintine_t *)x)

struct _toporouter_serpintine_t {
  GList *arcs;
  gdouble x, y;
  gdouble x0, y0, x1, y1;

  gpointer start;
  gdouble halfa, radius;
  guint nhalfcycles;

};
typedef struct _toporouter_serpintine_t toporouter_serpintine_t;

struct _toporouter_oproute_t {
  GList *arcs;
  toporouter_vertex_t *term1, *term2;
  char *style; char *netlist;
  guint layergroup;
  gdouble tof;
  GSList *path;
  
  GList *clearance;
  GSList *adj;

//  GSList *serpintining;
//  GList *serpintines;
  toporouter_serpintine_t *serp;
};

typedef struct _toporouter_oproute_t toporouter_oproute_t;


#define TOPOROUTER_IS_ARC(obj)           (gts_object_is_from_class (obj, toporouter_arc_class()))
#define TOPOROUTER_ARC(obj)              GTS_OBJECT_CAST (obj, toporouter_arc_t, toporouter_arc_class())
#define TOPOROUTER_ARC_CLASS(klass)      GTS_OBJECT_CLASS_CAST (klass, toporouter_arc_class_t, toporouter_arc_class())

struct _toporouter_arc_t {
  GtsObject object;
  
  gdouble x0, y0, x1, y1;
  toporouter_vertex_t *centre, *v;
  gdouble r;
  gint dir;

  GList *clearance;

  toporouter_oproute_t *oproute;

};

struct _toporouter_arc_class_t {
  GtsObjectClass parent_class;
  gboolean binary;
};

typedef struct _toporouter_arc_t toporouter_arc_t;
typedef struct _toporouter_arc_class_t toporouter_arc_class_t;

typedef struct _toporouter_t toporouter_t;

#define TOPOROUTER_CLUSTER(x) ((toporouter_cluster_t *)x)

typedef struct toporouter_cluster_t {
  guint id;
  GSList *i;
  char *netlist, *style;
} toporouter_cluster_t;


typedef struct {
  guint id;

//  gdouble *pairwise_detour;
  gdouble pairwise_detour_sum;
  gdouble score;
  guint pairwise_fails;

  toporouter_route_t *routedata;

  toporouter_t *r;

} toporouter_netscore_t;

#define TOPOROUTER_NETSCORE(x) ((toporouter_netscore_t *)x)

struct _toporouter_t {
  GSList *bboxes;
  GNode *bboxtree;

  toporouter_layer_t *layers;
  
  GSList *clusters;
  guint clustercounter;

  GSList *nets;
  GSList *paths;
  GSList *finalroutes;

  GSList *keepoutlayers;

  guint flags;

  GSList *destboxes, *consumeddestboxes;

  guint routecount;
  /* settings: */
  guint viamax;
  gdouble viacost;
  gdouble stublength;
  gdouble serpintine_half_amplitude;

  guint effort;

  GSList *routednets, *failednets;
  GSList *bestrouting;
  guint bestfailcount;

  guint (*router)(struct _toporouter_t *);

  gint (*netsort)(toporouter_netscore_t **, toporouter_netscore_t **);

  struct timeval starttime;  

  FILE *debug;
};

typedef gint (*oproute_adjseg_func)
  (toporouter_t *,
   GSList **,
   GList **,
   guint *,
   gdouble, gdouble, gdouble, gdouble,
   toporouter_oproute_t *,
   toporouter_oproute_t *);

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
