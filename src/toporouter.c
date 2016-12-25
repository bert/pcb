/*!
 * \file src/toporouter.c
 *
 * \brief Topological Autorouter for PCB.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * Topological Autorouter for
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2009 Anthony Blake
 *
 * Copyright (C) 2009-2011 PCB Contributors (see ChangeLog for details)
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Contact addresses for email:
 * Anthony Blake, tonyb33@gmail.com
 *
 *
 *
 * \warning This is *EXPERIMENTAL* code.
 *
 * As the code is experimental, the algorithms and code
 * are likely to change. Which means it isn't documented
 * or optimized. If you would like to learn about Topological
 * Autorouters, the following papers are good starting points:
 *
 *This file implements a topological autorouter, and uses techniques from the
 *following publications:
 *
 * Dayan, T. and Dai, W.W.M., "Layer Assignment for a Rubber Band Router" Tech
 * Report UCSC-CRL-92-50, Univ. of California, Santa Cruz, 1992.
 *
 * Dai, W.W.M and Dayan, T. and Staepelaere, D., "Topological Routing in SURF:
 * Generating a Rubber-Band Sketch" Proc. 28th ACM/IEEE Design Automation
 * Conference, 1991, pp. 39-44.
 *
 * David Staepelaere, Jeffrey Jue, Tal Dayan, Wayne Wei-Ming Dai, "SURF:
 * Rubber-Band Routing System for Multichip Modules," IEEE Design and Test of
 * Computers ,vol. 10, no. 4,  pp. 18-26, October/December, 1993.
 *
 * Dayan, T., "Rubber-band based topological router" PhD Thesis, Univ. of
 * California, Santa Cruz, 1997.
 *
 * David Staepelaere, "Geometric transformations for a rubber-band sketch"
 * Master's thesis, Univ. of California, Santa Cruz, September 1992.
 */


#include "toporouter.h"
#include "pcb-printf.h"

#define BOARD_EDGE_RESOLUTION MIL_TO_COORD (100.)
#define VIA_COST_AS_DISTANCE  MIL_TO_COORD (100.)
#define ROAR_DETOUR_THRESHOLD MIL_TO_COORD (10.)


/*!
 * \brief Initialise an edge.
 */
static void 
toporouter_edge_init (toporouter_edge_t *edge)
{
  edge->routing = NULL;
  edge->flags = 0;
}

toporouter_edge_class_t * 
toporouter_edge_class(void)
{
  static toporouter_edge_class_t *klass = NULL;

  if (klass == NULL) {
    GtsObjectClassInfo constraint_info = {
      "toporouter_edge_t",
      sizeof (toporouter_edge_t),
      sizeof (toporouter_edge_class_t),
      (GtsObjectClassInitFunc) NULL,
      (GtsObjectInitFunc) toporouter_edge_init,
      (GtsArgSetFunc) NULL,
      (GtsArgGetFunc) NULL
    };
    klass = (toporouter_edge_class_t *)gts_object_class_new (GTS_OBJECT_CLASS (gts_edge_class ()), &constraint_info);
  }

  return klass;
}

/*!
 * \brief Initialise a bounding box.
 */
static void 
toporouter_bbox_init (toporouter_bbox_t *box)
{
  box->data = NULL;
  box->type = OTHER;
  box->constraints = NULL;
  box->cluster = NULL;
}

toporouter_bbox_class_t * 
toporouter_bbox_class(void)
{
  static toporouter_bbox_class_t *klass = NULL;

  if (klass == NULL) {
    GtsObjectClassInfo constraint_info = {
      "toporouter_bbox_t",
      sizeof (toporouter_bbox_t),
      sizeof (toporouter_bbox_class_t),
      (GtsObjectClassInitFunc) NULL,
      (GtsObjectInitFunc) toporouter_bbox_init,
      (GtsArgSetFunc) NULL,
      (GtsArgGetFunc) NULL
    };
    klass = (toporouter_bbox_class_t *)gts_object_class_new (GTS_OBJECT_CLASS (gts_bbox_class ()), &constraint_info);
  }

  return klass;
}

/*!
 * \brief Initialise a vertex class.
 */
static void 
toporouter_vertex_class_init (toporouter_vertex_class_t *klass)
{

}

/*!
 * \brief Initialise a vertex.
 */
static void 
toporouter_vertex_init (toporouter_vertex_t *vertex)
{
  vertex->bbox = NULL;
  vertex->parent = NULL;
  vertex->child = NULL;
  vertex->flags = 0;
  vertex->routingedge = NULL;
  vertex->arc = NULL;
  vertex->oproute = NULL;
  vertex->route = NULL;

  vertex->gcost = 0.;
  vertex->hcost = 0.;
  vertex->gn = 0;
}

toporouter_vertex_class_t * 
toporouter_vertex_class(void)
{
  static toporouter_vertex_class_t *klass = NULL;

  if (klass == NULL) {
    GtsObjectClassInfo constraint_info = {
      "toporouter_vertex_t",
      sizeof (toporouter_vertex_t),
      sizeof (toporouter_vertex_class_t),
      (GtsObjectClassInitFunc) toporouter_vertex_class_init,
      (GtsObjectInitFunc) toporouter_vertex_init,
      (GtsArgSetFunc) NULL,
      (GtsArgGetFunc) NULL
    };
    klass = (toporouter_vertex_class_t *)gts_object_class_new (GTS_OBJECT_CLASS (gts_vertex_class ()), &constraint_info);
  }

  return klass;
}

/*!
 * \brief Initialise a constraint class.
 */
static void 
toporouter_constraint_class_init (toporouter_constraint_class_t *klass)
{

}

/*!
 * \brief Initialise a constraint.
 */
static void 
toporouter_constraint_init (toporouter_constraint_t *constraint)
{
  constraint->box = NULL;
  constraint->routing = NULL;
}

toporouter_constraint_class_t * 
toporouter_constraint_class(void)
{
  static toporouter_constraint_class_t *klass = NULL;

  if (klass == NULL) {
    GtsObjectClassInfo constraint_info = {
      "toporouter_constraint_t",
      sizeof (toporouter_constraint_t),
      sizeof (toporouter_constraint_class_t),
      (GtsObjectClassInitFunc) toporouter_constraint_class_init,
      (GtsObjectInitFunc) toporouter_constraint_init,
      (GtsArgSetFunc) NULL,
      (GtsArgGetFunc) NULL
    };
    klass = (toporouter_constraint_class_t *)gts_object_class_new (GTS_OBJECT_CLASS (gts_constraint_class ()), &constraint_info);
  }

  return klass;
}

/*!
 * \brief Initialise an arc.
 */
static void 
toporouter_arc_init (toporouter_arc_t *arc)
{
  arc->x0 = -1.;
  arc->y0 = -1.;
  arc->x1 = -1.;
  arc->y1 = -1.;
  arc->centre = NULL;
  arc->v = NULL;
  arc->v1 = NULL;
  arc->v2 = NULL;
  arc->r = -1.;
  arc->dir = 31337;
  arc->clearance = NULL;
  arc->oproute = NULL;
}

toporouter_arc_class_t * 
toporouter_arc_class(void)
{
  static toporouter_arc_class_t *klass = NULL;

  if (klass == NULL) {
    GtsObjectClassInfo constraint_info = {
      "toporouter_arc_t",
      sizeof (toporouter_arc_t),
      sizeof (toporouter_arc_class_t),
      (GtsObjectClassInitFunc) NULL,
      (GtsObjectInitFunc) toporouter_arc_init,
      (GtsArgSetFunc) NULL,
      (GtsArgGetFunc) NULL
    };
    klass = (toporouter_arc_class_t *)gts_object_class_new (GTS_OBJECT_CLASS (gts_constraint_class ()), &constraint_info);
  }

  return klass;
}

#define MARGIN 10.0f

/*!
 * \brief Initialise output.
 */
drawing_context_t *
toporouter_output_init(int w, int h, char *filename) 
{
  drawing_context_t *dc;

  dc = (drawing_context_t *)malloc(sizeof(drawing_context_t));

  dc->iw = w;
  dc->ih = h;
  dc->filename = filename;
  
  /* Calculate scaling to maintain aspect ratio */
  if(PCB->MaxWidth > PCB->MaxHeight) {
    /* Scale board width to match image width minus 2xMARGIN */
    dc->s = ((double)dc->iw - (2 * MARGIN)) / (double)PCB->MaxWidth; 
    dc->ih = (double)PCB->MaxHeight * dc->s + (2 * MARGIN);
  }else{
    /* Scale board height to match image height minus 2xMARGIN */
    dc->s = ((double)dc->ih - (2 * MARGIN)) / (double)PCB->MaxHeight;
    dc->iw = (double)PCB->MaxWidth * dc->s + (2 * MARGIN);
  }

#if TOPO_OUTPUT_ENABLED
  dc->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, dc->iw, dc->ih);
  dc->cr = cairo_create (dc->surface);
  
  cairo_rectangle (dc->cr, 0, 0, dc->iw, dc->ih);
  cairo_set_source_rgb (dc->cr, 0, 0, 0);
  cairo_fill (dc->cr);

#endif

  return dc;
}

/*!
 * \brief Close output.
 */
void
toporouter_output_close(drawing_context_t *dc) 
{
#if TOPO_OUTPUT_ENABLED
  cairo_surface_write_to_png (dc->surface, dc->filename);
  cairo_destroy (dc->cr);
  cairo_surface_destroy (dc->surface);
#endif
}

/*!
 * \brief Lookup a keepaway.
 */
gdouble
lookup_keepaway(char *name) 
{
  if(name)
  STYLE_LOOP(PCB);
  {
//    if(!strcmp(style->Name, name)) return style->Keepaway + 1.;
    if(!strcmp(style->Name, name)) return style->Keepaway;
  }
  END_LOOP;
//  return Settings.Keepaway + 1.;
  return Settings.Keepaway ;
}

/*!
 * \brief Lookup thickness.
 */
gdouble
lookup_thickness(char *name) 
{
  if(name)
  STYLE_LOOP(PCB);
  {
    if(!strcmp(style->Name, name)) return style->Thick;
  }
  END_LOOP;
  return Settings.LineThickness;
}

static inline gdouble
cluster_keepaway(toporouter_cluster_t *cluster) 
{
  if(cluster) return lookup_keepaway(cluster->netlist->style);
  return lookup_keepaway(NULL);
}

static inline gdouble
cluster_thickness(toporouter_cluster_t *cluster) 
{
  if(cluster) return lookup_thickness(cluster->netlist->style);
  return lookup_thickness(NULL);
}

/*!
 * \brief Draw a vertex.
 */
gint
toporouter_draw_vertex(gpointer item, gpointer data)
{
#if TOPO_OUTPUT_ENABLED
  drawing_context_t *dc = (drawing_context_t *) data;
  toporouter_vertex_t *tv;
  PinType *pin;
  PadType *pad;
  gdouble blue;  
  
  if(TOPOROUTER_IS_VERTEX((GtsObject*)item)) {
    tv = TOPOROUTER_VERTEX((GtsObject*)item);
    
    if(tv->flags & VERTEX_FLAG_RED) {
      cairo_set_source_rgba(dc->cr, 1., 0., 0., 0.8f);
      cairo_arc(dc->cr, 
          tv->v.p.x * dc->s + MARGIN, 
          tv->v.p.y * dc->s + MARGIN, 
          MIL_TO_COORD (5.) * dc->s, 0, 2 * M_PI);
      cairo_fill(dc->cr);

    }else if(tv->flags & VERTEX_FLAG_GREEN) {
      cairo_set_source_rgba(dc->cr, 0., 1., 0., 0.8f);
      cairo_arc(dc->cr, 
          tv->v.p.x * dc->s + MARGIN, 
          tv->v.p.y * dc->s + MARGIN, 
          MIL_TO_COORD (5.) * dc->s, 0, 2 * M_PI);
      cairo_fill(dc->cr);
    
    }else if(tv->flags & VERTEX_FLAG_BLUE) {
      cairo_set_source_rgba(dc->cr, 0., 0., 1., 0.8f);
      cairo_arc(dc->cr, 
          tv->v.p.x * dc->s + MARGIN, 
          tv->v.p.y * dc->s + MARGIN, 
          MIL_TO_COORD (5.) * dc->s, 0, 2 * M_PI);
      cairo_fill(dc->cr);


    }
    //printf("tv->type = %d\n", tv->type);
    if(!dc->mode) {
      if(tv->bbox) {
        pin = (PinType*) tv->bbox->data;
        pad = (PadType*) tv->bbox->data;

        blue = 0.0f;
        switch(tv->bbox->type) {
          case PIN:
            cairo_set_source_rgba(dc->cr, 1.0f, 0., 0.0f, 0.2f);
            cairo_arc(dc->cr, 
                tv->v.p.x * dc->s + MARGIN, 
                tv->v.p.y * dc->s + MARGIN, 
                (((gdouble)pin->Thickness / 2.0f) + (gdouble)lookup_keepaway(pin->Name) ) * dc->s, 0, 2 * M_PI);
            cairo_fill(dc->cr);

            cairo_set_source_rgba(dc->cr, 1.0f, 0., 0., 0.4f);
            cairo_arc(dc->cr, 
                tv->v.p.x * dc->s + MARGIN, 
                tv->v.p.y * dc->s + MARGIN, 
                (gdouble)(pin->Thickness) / 2.0f * dc->s,
                0, 2 * M_PI);
            cairo_fill(dc->cr);

            break;
          case VIA:
            cairo_set_source_rgba(dc->cr, 0.0f, 0., 1., 0.2f);
            cairo_arc(dc->cr, 
                tv->v.p.x * dc->s + MARGIN, 
                tv->v.p.y * dc->s + MARGIN, 
                (((gdouble)pin->Thickness / 2.0f) + (gdouble)lookup_keepaway(pin->Name) ) * dc->s, 0, 2 * M_PI);
            cairo_fill(dc->cr);

            cairo_set_source_rgba(dc->cr, 0.0f, 0., 1., 0.4f);
            cairo_arc(dc->cr, 
                tv->v.p.x * dc->s + MARGIN, 
                tv->v.p.y * dc->s + MARGIN, 
                (gdouble)(pin->Thickness) / 2.0f * dc->s,
                0, 2 * M_PI);
            cairo_fill(dc->cr);

            break;
          case PAD:
            cairo_set_source_rgba(dc->cr, 0.0f, 1., 0., 0.5f);
            cairo_arc(dc->cr, 
                tv->v.p.x * dc->s + MARGIN, 
                tv->v.p.y * dc->s + MARGIN, 
                MIL_TO_COORD (4.) * dc->s, 0, 2 * M_PI);
            cairo_fill(dc->cr);

            break;
          default:
            break;
        }
      }
    }else{
      if(tv->flags & VERTEX_FLAG_BLUE) {
        cairo_set_source_rgba(dc->cr, 0., 0., 1., 0.8f);
        cairo_arc(dc->cr, 
            tv->v.p.x * dc->s + MARGIN, 
            tv->v.p.y * dc->s + MARGIN, 
            MIL_TO_COORD (5.) * dc->s, 0, 2 * M_PI);
        cairo_fill(dc->cr);
      }else if(tv->flags & VERTEX_FLAG_RED) {
        cairo_set_source_rgba(dc->cr, 1., 0., 0., 0.8f);
        cairo_arc(dc->cr, 
            tv->v.p.x * dc->s + MARGIN, 
            tv->v.p.y * dc->s + MARGIN, 
            MIL_TO_COORD (5.) * dc->s, 0, 2 * M_PI);
        cairo_fill(dc->cr);

      }else if(tv->flags & VERTEX_FLAG_GREEN) {
        cairo_set_source_rgba(dc->cr, 0., 1., 0., 0.8f);
        cairo_arc(dc->cr, 
            tv->v.p.x * dc->s + MARGIN, 
            tv->v.p.y * dc->s + MARGIN, 
            MIL_TO_COORD (5.) * dc->s, 0, 2 * M_PI);
        cairo_fill(dc->cr);
      }

    }
  }else{
    fprintf(stderr, "Unknown data passed to toporouter_draw_vertex, aborting foreach\n");
    return -1;
  }
  return 0;
#else 
  return -1;
#endif
}

/*!
 * \brief Draw an edge.
 */
gint
toporouter_draw_edge(gpointer item, gpointer data)
{
#if TOPO_OUTPUT_ENABLED
  drawing_context_t *dc = (drawing_context_t *) data;
  toporouter_edge_t *te;
  toporouter_constraint_t *tc;
  
  if(TOPOROUTER_IS_EDGE((GtsObject*)item)) {
    te = TOPOROUTER_EDGE((GtsObject*)item);
    cairo_set_source_rgba(dc->cr, 1.0f, 1.0f, 1.0f, 0.5f);
    cairo_move_to(dc->cr, 
        te->e.segment.v1->p.x * dc->s + MARGIN, 
        te->e.segment.v1->p.y * dc->s + MARGIN);
    cairo_line_to(dc->cr, 
        te->e.segment.v2->p.x * dc->s + MARGIN, 
        te->e.segment.v2->p.y * dc->s + MARGIN);
    cairo_stroke(dc->cr);
  }else if(TOPOROUTER_IS_CONSTRAINT((GtsObject*)item)) {
    tc = TOPOROUTER_CONSTRAINT((GtsObject*)item);
    if(tc->box) {
      switch(tc->box->type) {
        case BOARD:
          cairo_set_source_rgba(dc->cr, 1.0f, 0.0f, 1.0f, 0.9f);
          cairo_move_to(dc->cr, 
              tc->c.edge.segment.v1->p.x * dc->s + MARGIN, 
              tc->c.edge.segment.v1->p.y * dc->s + MARGIN);
          cairo_line_to(dc->cr, 
              tc->c.edge.segment.v2->p.x * dc->s + MARGIN, 
              tc->c.edge.segment.v2->p.y * dc->s + MARGIN);
          cairo_stroke(dc->cr);
          break;
        case PIN:
        case PAD:
          cairo_set_source_rgba(dc->cr, 1.0f, 0.0f, 0.0f, 0.9f);
          cairo_move_to(dc->cr, 
              tc->c.edge.segment.v1->p.x * dc->s + MARGIN, 
              tc->c.edge.segment.v1->p.y * dc->s + MARGIN);
          cairo_line_to(dc->cr, 
              tc->c.edge.segment.v2->p.x * dc->s + MARGIN, 
              tc->c.edge.segment.v2->p.y * dc->s + MARGIN);
          cairo_stroke(dc->cr);
          break;
        case LINE:
          cairo_set_source_rgba(dc->cr, 0.0f, 1.0f, 0.0f, 0.9f);
          cairo_move_to(dc->cr, 
              tc->c.edge.segment.v1->p.x * dc->s + MARGIN, 
              tc->c.edge.segment.v1->p.y * dc->s + MARGIN);
          cairo_line_to(dc->cr, 
              tc->c.edge.segment.v2->p.x * dc->s + MARGIN, 
              tc->c.edge.segment.v2->p.y * dc->s + MARGIN);
          cairo_stroke(dc->cr);
          break;

        default:
          cairo_set_source_rgba(dc->cr, 1.0f, 1.0f, 0.0f, 0.9f);
          cairo_move_to(dc->cr, 
              tc->c.edge.segment.v1->p.x * dc->s + MARGIN, 
              tc->c.edge.segment.v1->p.y * dc->s + MARGIN);
          cairo_line_to(dc->cr, 
              tc->c.edge.segment.v2->p.x * dc->s + MARGIN, 
              tc->c.edge.segment.v2->p.y * dc->s + MARGIN);
          cairo_stroke(dc->cr);
          break;
      }
    }else{
      printf("CONSTRAINT without box\n");

    }
  }else{
    fprintf(stderr, "Unknown data passed to toporouter_draw_edge, aborting foreach\n");
    return -1;
  }

  return 0;
#else 
  return -1;
#endif
}

//#define vertex_bbox(v) (v->bbox)
///*
toporouter_bbox_t *
vertex_bbox(toporouter_vertex_t *v) 
{
  return v ? v->bbox : NULL;
}
//*/
char *
vertex_netlist(toporouter_vertex_t *v)
{
  toporouter_bbox_t *box = vertex_bbox(v);

  if(box && box->cluster) return box->cluster->netlist->netlist;

  return NULL;
}
  
char *
constraint_netlist(toporouter_constraint_t *c)
{
  toporouter_bbox_t *box = c->box;

  if(box && box->cluster) return box->cluster->netlist->netlist;

  return NULL;
}

static inline guint
epsilon_equals(gdouble a, gdouble b) 
{
  if(a > b - EPSILON && a < b + EPSILON) return 1;
  return 0;
}

/*!
 * \brief Print a bounding box.
 */
void
print_bbox(toporouter_bbox_t *box) 
{
  printf("[BBOX ");
  switch(box->type) {
    case PAD:
      printf("PAD "); break;
    case PIN:
      printf("PIN "); break;
    case VIA:
      printf("VIA "); break;
    case LINE:
      printf("LINE "); break;
    case BOARD:
      printf("BOARD "); break;
    case POLYGON:
      printf("POLYGON "); break;
    default:
      printf("UNKNOWN "); break;
  }

  if(box->point) 
    printf("P: %f,%f,%f ", vx(box->point), vy(box->point), vz(box->point));
  else
    printf("P: NONE ");

  printf("LAYER: %d ", box->layer);
  printf("CLUSTER: %d]\n", box->cluster ? box->cluster->c : -1);
  
}

/*!
 * \brief Print a vertex.
 */
void
print_vertex(toporouter_vertex_t *v)
{
  if(v)
    printf("[V %f,%f,%f ", vx(v), vy(v), vz(v));
  else
    printf("[V (null) ");
  
  printf("%s ", vertex_netlist(v));
  if(v->route && v->route->netlist)
    printf("%s ", v->route->netlist->netlist);

  if(v->routingedge) {
    guint n = g_list_length(edge_routing(v->routingedge));
    guint pos = g_list_index(edge_routing(v->routingedge), v);

    if(TOPOROUTER_IS_CONSTRAINT(v->routingedge))
      printf("[CONST ");
    else
      printf("[EDGE ");
  
    printf("%d/%d] ", pos, n);
  
  }


  if(v->flags & VERTEX_FLAG_TEMP) printf("TEMP ");
  if(v->flags & VERTEX_FLAG_ROUTE) printf("ROUTE ");
  if(v->flags & VERTEX_FLAG_SPECCUT) printf("SPECCUT ");
  if(v->flags & VERTEX_FLAG_FAKE) printf("FAKE ");

  printf("]\n");

}

gdouble
vertex_net_thickness(toporouter_vertex_t *v) 
{
  toporouter_bbox_t *box = vertex_bbox(v);
  
  if(!box) {
    
    while(v && (v->flags & VERTEX_FLAG_TEMP || v->flags & VERTEX_FLAG_ROUTE)) {
      v = v->parent;
    }

    box = vertex_bbox(v);

  }else{
    if(box->type == PIN || box->type == VIA) {
      PinType *pin = (PinType *)box->data;
      if(TEST_FLAG(SQUAREFLAG, pin) || TEST_FLAG(OCTAGONFLAG, pin)) {
        return 0.;
      }
//      return ((PinType *)box->data)->Thickness + 1.;
      return ((PinType *)box->data)->Thickness;
    }else if(box->type == PAD) {
      PadType *pad = (PadType *)box->data;
      if(pad->Point1.X == pad->Point2.X && pad->Point1.Y == pad->Point2.Y && !TEST_FLAG(SQUAREFLAG, pad)) {
        return pad->Thickness;
      }
      return 0.;
    }else if(box->type == BOARD) {
      return 0.;
    }else if(box->type == LINE) {
      LineType *line = (LineType *) box->data;
      return line->Thickness;
    }else if(box->type == POLYGON) {
      return 0.;
    }

    printf("Unrecognized type in thickness lookup..\n");
  }

//  if(!box || !box->cluster) return Settings.LineThickness + 1.;
  if(!box || !box->cluster) return Settings.LineThickness;
  
  return cluster_thickness(box->cluster);
}

gdouble
vertex_net_keepaway(toporouter_vertex_t *v) 
{
  toporouter_bbox_t *box = vertex_bbox(v);
  if(!box) {
  
    while(v && (v->flags & VERTEX_FLAG_TEMP || v->flags & VERTEX_FLAG_ROUTE)) {
      v = v->parent;
    }
    box = vertex_bbox(v);
  } 
  else{
  //  if(box->type == PIN || box->type == VIA) 
  //    return ((PinType *)box->data)->Clearance;
  //  else if(box->type == PAD)
  //    return ((PadType *)box->data)->Clearance;

  }
  
//  if(!box || !box->cluster) return Settings.Keepaway + 1.;
  if(!box || !box->cluster) return Settings.Keepaway;
  return cluster_keepaway(box->cluster);
}

/*!
 * \brief Fills in x and y with coordinates of point from a towards b of
 * distance d.
 */
static void
point_from_point_to_point (toporouter_vertex_t *a,
                           toporouter_vertex_t *b,
                           double d,
                           double *x, double *y)
{
  double theta = atan2 (vy(b) - vy(a), vx(b) - vx(a));

  *x = vx(a) + d * cos (theta);
  *y = vy(a) + d * sin (theta);
}

static inline gint
coord_wind(gdouble ax, gdouble ay, gdouble bx, gdouble by, gdouble cx, gdouble cy) 
{
  gdouble rval, dx1, dx2, dy1, dy2;
  dx1 = bx - ax; dy1 = by - ay;
  dx2 = cx - bx; dy2 = cy - by;
  rval = (dx1*dy2)-(dy1*dx2);
  return (rval > EPSILON) ? 1 : ((rval < -EPSILON) ? -1 : 0);
}

/*!
 * \brief .
 *
 * wind_v:
 *
 * \return 1, 0, -1 for counterclockwise, collinear or clockwise,
 * respectively.
 */
int 
point_wind(GtsPoint *a, GtsPoint *b, GtsPoint *c) 
{
  gdouble rval, dx1, dx2, dy1, dy2;
  dx1 = b->x - a->x; dy1 = b->y - a->y;
  dx2 = c->x - b->x; dy2 = c->y - b->y;
  rval = (dx1*dy2)-(dy1*dx2);
  return (rval > EPSILON) ? 1 : ((rval < -EPSILON) ? -1 : 0);
}

static inline int
vertex_wind(GtsVertex *a, GtsVertex *b, GtsVertex *c) 
{
  return point_wind(GTS_POINT(a), GTS_POINT(b), GTS_POINT(c));
}

static inline int
tvertex_wind(toporouter_vertex_t  *a, toporouter_vertex_t  *b, toporouter_vertex_t  *c) 
{
  return point_wind(GTS_POINT(a), GTS_POINT(b), GTS_POINT(c));
}

/*!
 * \brief Moves vertex v d units in the direction of vertex p.
 */
static void
coord_move_towards_coord_values (double ax, double ay,
                                 double px, double py,
                                 double d,
                                 double *x, double *y)
{
  double theta = atan2 (py - ay, px - ax);

  *x = ax + d * cos (theta);
  *y = ay + d * sin (theta);
}

/*!
 * \brief Moves vertex v d units in the direction of vertex p.
 */
static void
vertex_move_towards_vertex_values (GtsVertex *v,
                                   GtsVertex *p,
                                   double d,
                                   double *x, double *y)
{
  double theta = atan2 (GTS_POINT(p)->y - GTS_POINT(v)->y,
                        GTS_POINT(p)->x - GTS_POINT(v)->x);

  *x = GTS_POINT(v)->x + d * cos (theta);
  *y = GTS_POINT(v)->y + d * sin (theta);
}

#define tv_on_layer(v,l) (l == TOPOROUTER_BBOX(TOPOROUTER_VERTEX(v)->boxes->data)->layer)

static inline gdouble
min_spacing(toporouter_vertex_t *v1, toporouter_vertex_t *v2)
{

  gdouble v1halfthick, v2halfthick, v1keepaway, v2keepaway, ms;
//  toporouter_edge_t *e = v1->routingedge;
  
  v1halfthick = vertex_net_thickness(TOPOROUTER_VERTEX(v1)) / 2.;
  v2halfthick = vertex_net_thickness(TOPOROUTER_VERTEX(v2)) / 2.;

  v1keepaway = vertex_net_keepaway(TOPOROUTER_VERTEX(v1));
  v2keepaway = vertex_net_keepaway(TOPOROUTER_VERTEX(v2));

  ms = v1halfthick + v2halfthick + MAX(v1keepaway, v2keepaway);

#ifdef SPACING_DEBUG
  printf("v1halfthick = %f v2halfthick = %f v1keepaway = %f v2keepaway = %f ms = %f\n",
      v1halfthick, v2halfthick, v1keepaway, v2keepaway, ms);
#endif 
  
  return ms;
}

// v1 is a vertex in the CDT, and v2 is a net... other way around?
static inline gdouble
min_vertex_net_spacing(toporouter_vertex_t *v1, toporouter_vertex_t *v2)
{

  gdouble v1halfthick, v2halfthick, v1keepaway, v2keepaway, ms;
  
  v1halfthick = vertex_net_thickness(TOPOROUTER_VERTEX(v1)) / 2.;
  v2halfthick = cluster_thickness(vertex_bbox(v2)->cluster) / 2.;

  v1keepaway = vertex_net_keepaway(TOPOROUTER_VERTEX(v1));
  v2keepaway = cluster_keepaway(vertex_bbox(v2)->cluster);

  ms = v1halfthick + v2halfthick + MAX(v1keepaway, v2keepaway);

  return ms;
}

static inline gdouble
min_oproute_vertex_spacing(toporouter_oproute_t *oproute, toporouter_vertex_t *v2)
{

  gdouble v1halfthick, v2halfthick, v1keepaway, v2keepaway, ms;
  
  v1halfthick = lookup_thickness(oproute->style) / 2.;
  v2halfthick = vertex_net_thickness(v2) / 2.;

  v1keepaway = lookup_keepaway(oproute->style);
  v2keepaway = vertex_net_keepaway(v2);

  ms = v1halfthick + v2halfthick + MAX(v1keepaway, v2keepaway);

  return ms;
}

gdouble
min_oproute_net_spacing(toporouter_oproute_t *oproute, toporouter_vertex_t *v2)
{

  gdouble v1halfthick, v2halfthick, v1keepaway, v2keepaway, ms;
  
  v1halfthick = lookup_thickness(oproute->style) / 2.;
  v2halfthick = cluster_thickness(v2->route->src) / 2.;

  v1keepaway = lookup_keepaway(oproute->style);
  v2keepaway = cluster_keepaway(v2->route->src);

  ms = v1halfthick + v2halfthick + MAX(v1keepaway, v2keepaway);

  return ms;
}

gdouble
min_net_net_spacing(toporouter_vertex_t *v1, toporouter_vertex_t *v2)
{

  gdouble v1halfthick, v2halfthick, v1keepaway, v2keepaway, ms;
  
  v1halfthick = cluster_thickness(v1->route->src) / 2.;
  v2halfthick = cluster_thickness(v2->route->src) / 2.;

  v1keepaway = cluster_keepaway(v1->route->src);
  v2keepaway = cluster_keepaway(v2->route->src);

  ms = v1halfthick + v2halfthick + MAX(v1keepaway, v2keepaway);

  return ms;
}

void
toporouter_draw_cluster(toporouter_t *r, drawing_context_t *dc, toporouter_cluster_t *cluster, gdouble red, gdouble green, gdouble blue, guint layer)
{
#if TOPO_OUTPUT_ENABLED
//GList *i = cluster->i;

//while(i) {
//  toporouter_bbox_t *box = TOPOROUTER_BBOX(i->data);

//  if(box->point && vz(box->point) == layer) {
//    cairo_set_source_rgba(dc->cr, red, green, blue, 0.8f);
//    cairo_arc(dc->cr, vx(box->point) * dc->s + MARGIN, vy(box->point) * dc->s + MARGIN, MIL_TO_COORD (5.) * dc->s, 0, 2 * M_PI);
//    cairo_fill(dc->cr);
//  }

//  i = i->next;
//}
#endif
}

void
toporouter_draw_surface(toporouter_t *r, GtsSurface *s, char *filename, int w, int h, int mode, GList *datas, int layer, GList *candidatepoints) 
{
#if TOPO_OUTPUT_ENABLED
  drawing_context_t *dc;
  GList *i; 
  toporouter_vertex_t *tv, *tv2 = NULL;

  dc = toporouter_output_init(w, h, filename);
  dc->mode = mode;
  dc->data = NULL;

  gts_surface_foreach_edge(s, toporouter_draw_edge, dc);
  gts_surface_foreach_vertex(s, toporouter_draw_vertex, dc);

  i = r->routednets;
  while(i) {
    GList *j = TOPOROUTER_ROUTE(i->data)->path;
    tv2 = NULL;
    while(j) {
      tv = TOPOROUTER_VERTEX(j->data);
      if(GTS_POINT(tv)->z == layer) {
        if(tv && tv2) {
          cairo_set_source_rgba(dc->cr, 0.0f, 1.0f, 0.0f, 0.8f);
          cairo_move_to(dc->cr, 
              GTS_POINT(tv)->x * dc->s + MARGIN, 
              GTS_POINT(tv)->y * dc->s + MARGIN);
          cairo_line_to(dc->cr, 
              GTS_POINT(tv2)->x * dc->s + MARGIN, 
              GTS_POINT(tv2)->y * dc->s + MARGIN);
          cairo_stroke(dc->cr);
        }

        if(tv->flags & VERTEX_FLAG_RED) {
          cairo_set_source_rgba(dc->cr, 1., 0., 0., 0.8f);
          cairo_arc(dc->cr, 
              tv->v.p.x * dc->s + MARGIN, 
              tv->v.p.y * dc->s + MARGIN, 
              MIL_TO_COORD (5.) * dc->s, 0, 2 * M_PI);
          cairo_fill(dc->cr);

        }else if(tv->flags & VERTEX_FLAG_GREEN) {
          cairo_set_source_rgba(dc->cr, 0., 1., 0., 0.8f);
          cairo_arc(dc->cr, 
              tv->v.p.x * dc->s + MARGIN, 
              tv->v.p.y * dc->s + MARGIN, 
              MIL_TO_COORD (5.) * dc->s, 0, 2 * M_PI);
          cairo_fill(dc->cr);

        }else if(tv->flags & VERTEX_FLAG_BLUE) {
          cairo_set_source_rgba(dc->cr, 0., 0., 1., 0.8f);
          cairo_arc(dc->cr, 
              tv->v.p.x * dc->s + MARGIN, 
              tv->v.p.y * dc->s + MARGIN, 
              MIL_TO_COORD (5.) * dc->s, 0, 2 * M_PI);
          cairo_fill(dc->cr);

        } 
        else {

          cairo_set_source_rgba(dc->cr, 1., 1., 1., 0.8f);
          cairo_arc(dc->cr, 
              tv->v.p.x * dc->s + MARGIN, 
              tv->v.p.y * dc->s + MARGIN, 
              MIL_TO_COORD (5.) * dc->s, 0, 2 * M_PI);
          cairo_fill(dc->cr);


        }

        if(tv->routingedge && !TOPOROUTER_IS_CONSTRAINT(tv->routingedge)) {
          gdouble tempx, tempy, nms, pms;
          GList *i = g_list_find(edge_routing(tv->routingedge), tv);
          toporouter_vertex_t *nextv, *prevv;

          nextv = edge_routing_next(tv->routingedge,i);
          prevv = edge_routing_prev(tv->routingedge,i);

          nms = min_spacing(tv,nextv);
          pms = min_spacing(tv,prevv);

          g_assert(isfinite(nms)); g_assert(isfinite(pms));

          point_from_point_to_point(tv, nextv, nms, &tempx, &tempy);

          cairo_set_source_rgba(dc->cr, 0.0f, 1.0f, 1.0f, 0.8f);
          cairo_move_to(dc->cr, vx(tv) * dc->s + MARGIN, vy(tv) * dc->s + MARGIN);
          cairo_line_to(dc->cr, tempx * dc->s + MARGIN, tempy * dc->s + MARGIN);
          cairo_stroke(dc->cr);

          point_from_point_to_point(tv, prevv, pms, &tempx, &tempy);

          cairo_move_to(dc->cr, vx(tv) * dc->s + MARGIN, vy(tv) * dc->s + MARGIN);
          cairo_line_to(dc->cr, tempx * dc->s + MARGIN, tempy * dc->s + MARGIN);
          cairo_stroke(dc->cr);




        }


      }
      tv2 = tv;
      j = j->next;
    }
    i = i->next;
  }
 
  while(datas) {
    toporouter_route_t *routedata = (toporouter_route_t *) datas->data;

    GList *i;//, *k;

    toporouter_draw_cluster(r, dc, routedata->src, 1., 0., 0., layer);
    toporouter_draw_cluster(r, dc, routedata->dest, 0., 0., 1., layer);

    tv2 = NULL;
    i = routedata->path;
    while(i) {
      tv = TOPOROUTER_VERTEX(i->data);
      if(GTS_POINT(tv)->z == layer) {
        if(tv && tv2) {
          cairo_set_source_rgba(dc->cr, 0.0f, 1.0f, 0.0f, 0.8f);
          cairo_move_to(dc->cr, 
              GTS_POINT(tv)->x * dc->s + MARGIN, 
              GTS_POINT(tv)->y * dc->s + MARGIN);
          cairo_line_to(dc->cr, 
              GTS_POINT(tv2)->x * dc->s + MARGIN, 
              GTS_POINT(tv2)->y * dc->s + MARGIN);
          cairo_stroke(dc->cr);
        }
      }
      tv2 = tv;
      i = i->next;
    }


    if(routedata->alltemppoints) {
      GList *i, *j;
      i = j = g_hash_table_get_keys (routedata->alltemppoints);
      while(i) {
        toporouter_vertex_t *tv = TOPOROUTER_VERTEX(i->data);

        if(GTS_POINT(tv)->z != layer) {
          i = i->next;
          continue;
        }
        if(tv->flags & VERTEX_FLAG_BLUE) {
          cairo_set_source_rgba(dc->cr, 0., 0., 1., 0.8f);
          cairo_arc(dc->cr, 
              tv->v.p.x * dc->s + MARGIN, 
              tv->v.p.y * dc->s + MARGIN, 
              MIL_TO_COORD (5.) * dc->s, 0, 2 * M_PI);
          cairo_fill(dc->cr);
        }else if(tv->flags & VERTEX_FLAG_RED) {
          cairo_set_source_rgba(dc->cr, 1., 0., 0., 0.8f);
          cairo_arc(dc->cr, 
              tv->v.p.x * dc->s + MARGIN, 
              tv->v.p.y * dc->s + MARGIN, 
              MIL_TO_COORD (5.) * dc->s, 0, 2 * M_PI);
          cairo_fill(dc->cr);

        }else if(tv->flags & VERTEX_FLAG_GREEN) {
          cairo_set_source_rgba(dc->cr, 0., 1., 0., 0.8f);
          cairo_arc(dc->cr, 
              tv->v.p.x * dc->s + MARGIN, 
              tv->v.p.y * dc->s + MARGIN, 
              MIL_TO_COORD (5.) * dc->s, 0, 2 * M_PI);
          cairo_fill(dc->cr);
        }else{
          cairo_set_source_rgba(dc->cr, 1., 1., 1., 0.8f);
          cairo_arc(dc->cr, 
              tv->v.p.x * dc->s + MARGIN, 
              tv->v.p.y * dc->s + MARGIN, 
              MIL_TO_COORD (5.) * dc->s, 0, 2 * M_PI);
          cairo_fill(dc->cr);
        }
        i = i->next;
      }
      g_list_free(j);
    }
    datas = datas->next;
  }
  toporouter_output_close(dc);
#endif
}


/*!
 * \brief Free a layer.
 */
void
toporouter_layer_free(toporouter_layer_t *l) 
{
  g_list_free(l->vertices);
  g_list_free(l->constraints);

}

guint
groupcount(void)
{
  int group;
  guint count = 0;

  for (group = 0; group < max_group; group++) {
    if(PCB->LayerGroups.Number[group] > 0) count++;
  }
  
  return count;
}

void
toporouter_free(toporouter_t *r)
{
  struct timeval endtime;
  double time_delta;

  int i;
  for(i=0;i<groupcount();i++) {
    toporouter_layer_free(&r->layers[i]);
  }

  gettimeofday (&endtime, NULL);
  time_delta = endtime.tv_sec - r->starttime.tv_sec +
               (endtime.tv_usec - r->starttime.tv_usec) / 1000000.;

  Message(_("Elapsed time: %.2f seconds\n"), time_delta);
  free(r->layers);  
  free(r);

}

/*!
 * \brief .
 *
 * wind:
 *
 * \return 1, 0, -1 for counterclockwise, collinear or clockwise,
 * respectively.
 */
int 
wind(toporouter_spoint_t *p1, toporouter_spoint_t *p2, toporouter_spoint_t *p3) 
{
  double rval, dx1, dx2, dy1, dy2;
  dx1 = p2->x - p1->x; dy1 = p2->y - p1->y;
  dx2 = p3->x - p2->x; dy2 = p3->y - p2->y;
  rval = (dx1*dy2)-(dy1*dx2);
  return (rval > 0.0001) ? 1 : ((rval < -0.0001) ? -1 : 0); /* XXX: Depends on PCB coordinate scaling */
}

static inline void
print_toporouter_constraint(toporouter_constraint_t *tc) 
{
  printf("%f,%f -> %f,%f ", 
      tc->c.edge.segment.v1->p.x,
      tc->c.edge.segment.v1->p.y,
      tc->c.edge.segment.v2->p.x,
      tc->c.edge.segment.v2->p.y);
}

static inline void
print_toporouter_vertex(toporouter_vertex_t *tv) 
{
  printf("%f,%f ", tv->v.p.x, tv->v.p.y); 
}


/*!
 * \brief .
 *
 * vertices_on_line:
 * Given vertex a, gradient m, and radius r:
 *
 * \return vertices on line of a & m at r from a.
 */ 
static void
vertices_on_line (toporouter_spoint_t *a,
                  double m,
                  double r,
                  toporouter_spoint_t *b0,
                  toporouter_spoint_t *b1)
{

  double c, dx;

  if (m == INFINITY || m == -INFINITY) {

    b0->x = a->x;
    b0->y = a->y + r;

    b1->x = a->x;
    b1->y = a->y - r;

    return;
  }

  c = a->y - m * a->x;
  dx = r / hypot (1, m);

  b0->x = a->x + dx;
  b0->y = m * b0->x + c;

  b1->x = a->x - dx;
  b1->y = m * b1->x + c;
}

/*!
 * \brief .
 *
 * coords_on_line:
 * Given coordinates ax, ay, gradient m, and radius r:
 *
 * \return coordinates on line of a & m at r from a.
 */
static void
coords_on_line (double ax, gdouble ay,
                double m,
                double r,
                double *b0x, double *b0y,
                double *b1x, double *b1y)
{
  double c, dx;

  if (m == INFINITY || m == -INFINITY) {

    *b0x = ax;
    *b0y = ay + r;

    *b1x = ax;
    *b1y = ay - r;

    return;
  }

  c = ay - m * ax;
  dx = r / hypot (1, m);

  *b0x = ax + dx;
  *b0y = m * *b0x + c;

  *b1x = ax - dx;
  *b1y = m * *b1x + c;
}

/*!
 * \brief Returns gradient of segment given by a & b.
 */
gdouble
vertex_gradient(toporouter_spoint_t *a, toporouter_spoint_t *b) 
{
  if(a->x == b->x) return INFINITY;

  return ((b->y - a->y) / (b->x - a->x));
}

/*!
 * \brief Returns gradient of segment given by (x0,y0) & (x1,y1).
 */
static inline gdouble
cartesian_gradient(gdouble x0, gdouble y0, gdouble x1, gdouble y1) 
{
  if(epsilon_equals(x0,x1)) return INFINITY;

  return ((y1 - y0) / (x1 - x0));
}

/*!
 * \brief Returns gradient of segment given by (x0,y0) & (x1,y1).
 */
static inline gdouble
point_gradient(GtsPoint *a, GtsPoint *b) 
{
  return cartesian_gradient(a->x, a->y, b->x, b->y);
}

gdouble
segment_gradient(GtsSegment *s)
{
  return cartesian_gradient(
      GTS_POINT(s->v1)->x, 
      GTS_POINT(s->v1)->y, 
      GTS_POINT(s->v2)->x, 
      GTS_POINT(s->v2)->y); 
}

/*!
 * \brief Returns gradient perpendicular to m.
 */
gdouble
perpendicular_gradient(gdouble m) 
{
  if(isinf(m)) return 0.0f;
  if(m < EPSILON && m > -EPSILON) return INFINITY;
  return -1.0f/m;
}

/*!
 * \brief Returns the distance between two vertices in the x-y plane.
 */
gdouble
vertices_plane_distance(toporouter_spoint_t *a, toporouter_spoint_t *b) {
  return hypot(a->x - b->x, a->y - b->y);
}

/*!
 * \brief Finds the point p distance r away from a on the line segment
 * of a & b.
 */
static inline void
vertex_outside_segment(toporouter_spoint_t *a, toporouter_spoint_t *b, gdouble r, toporouter_spoint_t *p) 
{
  toporouter_spoint_t temp[2];

  vertices_on_line(a, vertex_gradient(a,b), r, &temp[0], &temp[1]);

  if(vertices_plane_distance(&temp[0], b) > vertices_plane_distance(&temp[1], b)) {
    p->x = temp[0].x;
    p->y = temp[0].y;
  }else{
    p->x = temp[1].x;
    p->y = temp[1].y;
  }

}

/*!
 * \brief .
 *
 * Proper intersection:
 * AB and CD must share a point interior to both segments.
 *
 * \return TRUE if AB properly intersects CD.
 */
static bool
coord_intersect_prop (double ax, double ay,
                      double bx, double by,
                      double cx, double cy,
                      double dx, double dy)
{
  int wind_abc = coord_wind (ax, ay, bx, by, cx, cy);
  int wind_abd = coord_wind (ax, ay, bx, by, dx, dy);
  int wind_cda = coord_wind (cx, cy, dx, dy, ax, ay);
  int wind_cdb = coord_wind (cx, cy, dx, dy, bx, by);

  /* If any of the line end-points are colinear with the other line, return false */
  if (wind_abc == 0 || wind_abd == 0 || wind_cda == 0 || wind_cdb == 0)
    return false;

  return (wind_abc != wind_abd) && (wind_cda != wind_cdb);
}

/*!
 * \brief .
 *
 * proper intersection:
 * AB and CD must share a point interior to both segments.
 *
 * \return TRUE if AB properly intersects CD.
 */
static bool
point_intersect_prop (GtsPoint *a, GtsPoint *b, GtsPoint *c, GtsPoint *d)
{
  int wind_abc = point_wind (a, b, c);
  int wind_abd = point_wind (a, b, d);
  int wind_cda = point_wind (c, d, a);
  int wind_cdb = point_wind (c, d, b);

  /* If any of the line end-points are colinear with the other line, return false */
  if (wind_abc == 0 || wind_abd == 0 || wind_cda == 0 || wind_cdb == 0)
    return false;

  return (wind_abc != wind_abd) && (wind_cda != wind_cdb);
}

/*!
 * \brief .
 */
static inline int
vertex_intersect_prop(GtsVertex *a, GtsVertex *b, GtsVertex *c, GtsVertex *d) 
{
  return point_intersect_prop(GTS_POINT(a), GTS_POINT(b), GTS_POINT(c), GTS_POINT(d));
}

/*!
 * \brief .
 *
 * intersection vertex:
 * AB and CD must share a point interior to both segments.
 *
 * \return vertex at intersection of AB and CD.
 */
GtsVertex *
vertex_intersect(GtsVertex *a, GtsVertex *b, GtsVertex *c, GtsVertex *d) 
{
  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  GtsVertex *rval;
  gdouble ua_top, ua_bot, ua, rx, ry;

  /* TODO: this could be done more efficiently without duplicating computation */
  if(!vertex_intersect_prop(a, b, c, d)) return NULL;

  ua_top = ((d->p.x - c->p.x) * (a->p.y - c->p.y)) - 
    ((d->p.y - c->p.y) * (a->p.x - c->p.x));
  ua_bot = ((d->p.y - c->p.y) * (b->p.x - a->p.x)) - 
    ((d->p.x - c->p.x) * (b->p.y - a->p.y));
  ua = ua_top / ua_bot;
  rx = a->p.x + (ua * (b->p.x - a->p.x));
  ry = a->p.y + (ua * (b->p.y - a->p.y));

  rval = gts_vertex_new (vertex_class, rx, ry, 0.0f);

  return rval;
}

/*!
 * \brief .
 *
 * intersection vertex:
 * AB and CD must share a point interior to both segments.
 *
 * \return vertex at intersection of AB and CD.
 */
void
coord_intersect(gdouble ax, gdouble ay, gdouble bx, gdouble by, gdouble cx, gdouble cy, gdouble dx, gdouble dy, gdouble *rx, gdouble *ry) 
{
  gdouble ua_top, ua_bot, ua;

  ua_top = ((dx - cx) * (ay - cy)) - ((dy - cy) * (ax - cx));
  ua_bot = ((dy - cy) * (bx - ax)) - ((dx - cx) * (by - ay));
  ua = ua_top / ua_bot;
  *rx = ax + (ua * (bx - ax));
  *ry = ay + (ua * (by - ay));

}


/*!
 * \brief .
 *
 * \return true if c is between a and b.
 */
int
point_between(GtsPoint *a, GtsPoint *b, GtsPoint *c) 
{
  if( point_wind(a, b, c) != 0 ) return 0;

  if( a->x != b->x ) {
    return ((a->x <= c->x) &&
        (c->x <= b->x)) ||
      ((a->x >= c->x) &&
       (c->x >= b->x));
  }
  return ((a->y <= c->y) &&
      (c->y <= b->y)) ||
    ((a->y >= c->y) &&
     (c->y >= b->y));
}

static inline int
vertex_between(GtsVertex *a, GtsVertex *b, GtsVertex *c) 
{
  return point_between(GTS_POINT(a), GTS_POINT(b), GTS_POINT(c));
}

void
delaunay_create_from_vertices(GList *vertices, GtsSurface **surface, GtsTriangle **t) 
{
  GList *i = vertices;
  GtsVertex *v1, *v2, *v3;
  GSList *vertices_slist = NULL;

  while (i) {
    vertices_slist = g_slist_prepend(vertices_slist, i->data);
    i = i->next;
  }

  // TODO: just work this out from the board outline
  *t = gts_triangle_enclosing (gts_triangle_class (), vertices_slist, 100000.0f);
  gts_triangle_vertices (*t, &v1, &v2, &v3);
 
  *surface = gts_surface_new (gts_surface_class (), gts_face_class (),
      GTS_EDGE_CLASS(toporouter_edge_class ()), GTS_VERTEX_CLASS(toporouter_vertex_class ()) );

  gts_surface_add_face (*surface, gts_face_new (gts_face_class (), (*t)->e1, (*t)->e2, (*t)->e3));
 
  i = vertices;
  while (i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(gts_delaunay_add_vertex (*surface, (GtsVertex *)i->data, NULL));
    
    if(v) {
      printf("ERROR: vertex could not be added to CDT ");
      print_vertex(v);
    }

    i = i->next;
  }
 
  gts_allow_floating_vertices = TRUE;
  gts_object_destroy (GTS_OBJECT (v1));
  gts_object_destroy (GTS_OBJECT (v2));
  gts_object_destroy (GTS_OBJECT (v3));
  gts_allow_floating_vertices = FALSE;

  g_slist_free(vertices_slist);
//  return surface;
}

GSList *
list_to_slist(GList *i) 
{
  GSList *rval = NULL;
  while(i) {
    rval = g_slist_prepend(rval, i->data);
    i = i->next;
  }
  return rval;
}

toporouter_bbox_t *
toporouter_bbox_create_from_points(int layer, GList *vertices, toporouter_term_t type, gpointer data)
{
  toporouter_bbox_t *bbox;
  GSList *vertices_slist = list_to_slist(vertices);

//  delaunay_create_from_vertices(vertices, &s, &t);
  bbox = TOPOROUTER_BBOX( gts_bbox_points(GTS_BBOX_CLASS(toporouter_bbox_class()), vertices_slist) );
  bbox->type = type;
  bbox->data = data;

  bbox->surface = NULL;
  bbox->enclosing = NULL;

  bbox->layer = layer;

  bbox->point = NULL;
  bbox->realpoint = NULL;

  g_slist_free(vertices_slist);
  return bbox;
}

toporouter_bbox_t *
toporouter_bbox_create(int layer, GList *vertices, toporouter_term_t type, gpointer data)
{
  toporouter_bbox_t *bbox;
  GtsSurface *s;
  GtsTriangle *t;

  delaunay_create_from_vertices(vertices, &s, &t);
  bbox = TOPOROUTER_BBOX( gts_bbox_surface(GTS_BBOX_CLASS(toporouter_bbox_class()), s) );
  bbox->type = type;
  bbox->data = data;

  bbox->surface = s;
  bbox->enclosing = t;

  bbox->layer = layer;

  return bbox;
}

GtsVertex *
insert_vertex(toporouter_t *r, toporouter_layer_t *l, gdouble x, gdouble y, toporouter_bbox_t *box) 
{
  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  GtsVertex *v;
  GList *i;

  i = l->vertices;
  while (i) {
    v = (GtsVertex *)i->data;
    if(v->p.x == x && v->p.y == y) {
      TOPOROUTER_VERTEX(v)->bbox = box;
      return v;
    }
    i = i->next;
  }

  v = gts_vertex_new (vertex_class , x, y, l - r->layers);
  TOPOROUTER_VERTEX(v)->bbox = box;
  l->vertices = g_list_prepend(l->vertices, v);

  return v;
}

GList *
insert_constraint_edge(toporouter_t *r, toporouter_layer_t *l, gdouble x1, gdouble y1, guint flags1, 
    gdouble x2, gdouble y2, guint flags2, toporouter_bbox_t *box)
{
  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  GtsEdgeClass *edge_class = GTS_EDGE_CLASS (toporouter_constraint_class ());
  GtsVertex *p[2];
  GtsVertex *v;
  GList *i;
  GtsEdge *e;

  p[0] = p[1] = NULL;

  /* insert or find points */

  i = l->vertices;
  while (i) {
    v = (GtsVertex *)i->data;
    if(v->p.x == x1 && v->p.y == y1) 
      p[0] = v;
    if(v->p.x == x2 && v->p.y == y2) 
      p[1] = v;
    i = i->next;
  }
  
  if(p[0] == NULL) {
    p[0] = gts_vertex_new (vertex_class, x1, y1, l - r->layers);
    TOPOROUTER_VERTEX(p[0])->bbox = box;
    l->vertices = g_list_prepend(l->vertices, p[0]);
  }
  if(p[1] == NULL) {
    p[1] = gts_vertex_new (vertex_class, x2, y2, l - r->layers);
    TOPOROUTER_VERTEX(p[1])->bbox = box;
    l->vertices = g_list_prepend(l->vertices, p[1]);
  }

  TOPOROUTER_VERTEX(p[0])->flags = flags1;
  TOPOROUTER_VERTEX(p[1])->flags = flags2;
  
  e = gts_edge_new (edge_class, p[0], p[1]);
  TOPOROUTER_CONSTRAINT(e)->box = box;
  l->constraints = g_list_prepend (l->constraints, e);
//  return insert_constraint_edge_rec(r, l, p, box);
  return g_list_prepend(NULL, e);

}

void
insert_constraints_from_list(toporouter_t *r, toporouter_layer_t *l, GList *vlist, toporouter_bbox_t *box) 
{
  GList *i = vlist;
  toporouter_vertex_t *pv = NULL, *v;

  while(i) {
    v = TOPOROUTER_VERTEX(i->data);

    if(pv) {
      box->constraints = 
        g_list_concat(box->constraints, insert_constraint_edge(r, l, vx(v), vy(v), v->flags, vx(pv), vy(pv), pv->flags, box));
    }

    pv = v;
    i = i->next;
  }
  
  v = TOPOROUTER_VERTEX(vlist->data);
  box->constraints = 
    g_list_concat(box->constraints, insert_constraint_edge(r, l, vx(v), vy(v), v->flags, vx(pv), vy(pv), pv->flags, box));

}

void
insert_centre_point(toporouter_t *r, toporouter_layer_t *l, gdouble x, gdouble y)
{
  GList *i;
  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());

  i = l->vertices;
  while (i) {
    GtsPoint *p = GTS_POINT(i->data);
    if(p->x == x && p->y == y) 
      return;
    i = i->next;
  }
  
  l->vertices = g_list_prepend(l->vertices, gts_vertex_new(vertex_class, x, y, 0.0f));
}

GtsPoint *
midpoint(GtsPoint *a, GtsPoint *b) 
{
  return gts_point_new(gts_point_class(), (a->x + b->x) / 2., (a->y + b->y) / 2., 0.);
}

static inline gdouble
pad_rad(PadType *pad)
{
  return (lookup_thickness(pad->Name) / 2.) + lookup_keepaway(pad->Name);
}

static inline gdouble
pin_rad(PinType *pin)
{
  return (lookup_thickness(pin->Name) / 2.) + lookup_keepaway(pin->Name);
}

GList *
rect_with_attachments(gdouble rad,
    gdouble x0, gdouble y0,
    gdouble x1, gdouble y1,
    gdouble x2, gdouble y2,
    gdouble x3, gdouble y3,
    gdouble z)
{
  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  GList *r = NULL, *rr = NULL, *i;
  toporouter_vertex_t *curpoint, *temppoint;


  curpoint = TOPOROUTER_VERTEX(gts_vertex_new (vertex_class, x0, y0, z));

  r = g_list_prepend(NULL, curpoint);
  r = g_list_prepend(r, gts_vertex_new (vertex_class, x1, y1, z));
  r = g_list_prepend(r, gts_vertex_new (vertex_class, x2, y2, z));
  r = g_list_prepend(r, gts_vertex_new (vertex_class, x3, y3, z));
  
  i = r;
  while(i) {
    temppoint = TOPOROUTER_VERTEX(i->data);
    rr = g_list_prepend(rr, curpoint);
    
    curpoint = temppoint;
    i = i->next;
  }

  g_list_free(r);

  return rr;
}

#define VERTEX_CENTRE(x) TOPOROUTER_VERTEX( vertex_bbox(x)->point )

/*!
 * \brief Read pad data from layer into toporouter_layer_t struct.
 *
 * Inserts points and constraints into GLists.
 */
int
read_pads(toporouter_t *r, toporouter_layer_t *l, guint layer) 
{
  toporouter_spoint_t p[2], rv[5];
  gdouble x[2], y[2], t, m;

  GList *vlist = NULL;
  toporouter_bbox_t *bbox = NULL;

  guint front = GetLayerGroupNumberBySide (TOP_SIDE);
  guint back = GetLayerGroupNumberBySide (BOTTOM_SIDE);

//  printf("read_pads: front = %d back = %d layer = %d\n", 
//     front, back, layer);

  /* If its not the top or bottom layer, there are no pads to read */
  if(l - r->layers != front && l - r->layers != back) return 0;

  ELEMENT_LOOP(PCB->Data);
  {
    PAD_LOOP(element);
    {
      if( (l - r->layers == back && TEST_FLAG(ONSOLDERFLAG, pad)) || 
          (l - r->layers == front && !TEST_FLAG(ONSOLDERFLAG, pad)) ) {

        t = (gdouble)pad->Thickness / 2.0f;
        x[0] = pad->Point1.X;
        x[1] = pad->Point2.X;
        y[0] = pad->Point1.Y;
        y[1] = pad->Point2.Y;

       
        if(TEST_FLAG(SQUAREFLAG, pad)) {
          /* Square or oblong pad. Four points and four constraint edges are
           * used */
          
          if(x[0] == x[1] && y[0] == y[1]) {
            /* Pad is square */

//            vlist = g_list_prepend(NULL, gts_vertex_new (vertex_class, x[0]-t, y[0]-t, 0.));
//            vlist = g_list_prepend(vlist, gts_vertex_new (vertex_class, x[0]-t, y[0]+t, 0.));
//            vlist = g_list_prepend(vlist, gts_vertex_new (vertex_class, x[0]+t, y[0]+t, 0.));
//            vlist = g_list_prepend(vlist, gts_vertex_new (vertex_class, x[0]+t, y[0]-t, 0.));
            vlist = rect_with_attachments(pad_rad(pad), 
                x[0]-t, y[0]-t, 
                x[0]-t, y[0]+t,
                x[0]+t, y[0]+t,
                x[0]+t, y[0]-t,
                l - r->layers);
            bbox = toporouter_bbox_create(l - r->layers, vlist, PAD, pad);
            r->bboxes = g_slist_prepend(r->bboxes, bbox);
            insert_constraints_from_list(r, l, vlist, bbox);
            g_list_free(vlist);
            
            //bbox->point = GTS_POINT( gts_vertex_new(vertex_class, x[0], y[0], 0.) );
            bbox->point = GTS_POINT( insert_vertex(r, l, x[0], y[0], bbox) );
            g_assert(TOPOROUTER_VERTEX(bbox->point)->bbox == bbox);
          }else {
            /* Pad is diagonal oblong or othogonal oblong */ 
            
            m = cartesian_gradient(x[0], y[0], x[1], y[1]);
            
            p[0].x = x[0]; p[0].y = y[0]; 
            p[1].x = x[1]; p[1].y = y[1]; 

            vertex_outside_segment(&p[0], &p[1], t, &rv[0]); 
            vertices_on_line(&rv[0], perpendicular_gradient(m), t, &rv[1], &rv[2]);

            vertex_outside_segment(&p[1], &p[0], t, &rv[0]);
            vertices_on_line(&rv[0], perpendicular_gradient(m), t, &rv[3], &rv[4]);

            if(wind(&rv[1], &rv[2], &rv[3]) != wind(&rv[2], &rv[3], &rv[4])) {
              rv[0].x = rv[3].x; rv[0].y = rv[3].y;               
              rv[3].x = rv[4].x; rv[3].y = rv[4].y;               
              rv[4].x = rv[0].x; rv[4].y = rv[0].y;               
            }
            
//            vlist = g_list_prepend(NULL,  gts_vertex_new (vertex_class, rv[1].x, rv[1].y, 0.));
//            vlist = g_list_prepend(vlist, gts_vertex_new (vertex_class, rv[2].x, rv[2].y, 0.));
//            vlist = g_list_prepend(vlist, gts_vertex_new (vertex_class, rv[3].x, rv[3].y, 0.));
//            vlist = g_list_prepend(vlist, gts_vertex_new (vertex_class, rv[4].x, rv[4].y, 0.));
            vlist = rect_with_attachments(pad_rad(pad), 
                rv[1].x, rv[1].y, 
                rv[2].x, rv[2].y,
                rv[3].x, rv[3].y,
                rv[4].x, rv[4].y,
                l - r->layers);
            bbox = toporouter_bbox_create(l - r->layers, vlist, PAD, pad);
            r->bboxes = g_slist_prepend(r->bboxes, bbox);
            insert_constraints_from_list(r, l, vlist, bbox);
            g_list_free(vlist);
            
            //bbox->point = GTS_POINT( gts_vertex_new(vertex_class, (x[0] + x[1]) / 2., (y[0] + y[1]) / 2., 0.) );
            bbox->point = GTS_POINT( insert_vertex(r, l, (x[0] + x[1]) / 2., (y[0] + y[1]) / 2., bbox) );
            g_assert(TOPOROUTER_VERTEX(bbox->point)->bbox == bbox);

          }

        }else {
          /* Either round pad or pad with curved edges */ 
          
          if(x[0] == x[1] && y[0] == y[1]) {
            /* One point */

            /* bounding box same as square pad */ 
            vlist = rect_with_attachments(pad_rad(pad), 
                x[0]-t, y[0]-t, 
                x[0]-t, y[0]+t,
                x[0]+t, y[0]+t,
                x[0]+t, y[0]-t,
                l - r->layers);
            bbox = toporouter_bbox_create(l - r->layers, vlist, PAD, pad);
            r->bboxes = g_slist_prepend(r->bboxes, bbox);
            g_list_free(vlist);
            
            //bbox->point = GTS_POINT( insert_vertex(r, l, x[0], y[0], bbox) );
            bbox->point = GTS_POINT( insert_vertex(r, l, x[0], y[0], bbox) );

          }else{
            /* Two points and one constraint edge */
            
            /* the rest is just for bounding box */ 
            m = cartesian_gradient(x[0], y[0], x[1], y[1]);
            
            p[0].x = x[0]; p[0].y = y[0]; 
            p[1].x = x[1]; p[1].y = y[1]; 

            vertex_outside_segment(&p[0], &p[1], t, &rv[0]); 
            vertices_on_line(&rv[0], perpendicular_gradient(m), t, &rv[1], &rv[2]);

            vertex_outside_segment(&p[1], &p[0], t, &rv[0]);
            vertices_on_line(&rv[0], perpendicular_gradient(m), t, &rv[3], &rv[4]);

            if(wind(&rv[1], &rv[2], &rv[3]) != wind(&rv[2], &rv[3], &rv[4])) {
              rv[0].x = rv[3].x; rv[0].y = rv[3].y;               
              rv[3].x = rv[4].x; rv[3].y = rv[4].y;               
              rv[4].x = rv[0].x; rv[4].y = rv[0].y;               
            }
            
            vlist = rect_with_attachments(pad_rad(pad), 
                rv[1].x, rv[1].y, 
                rv[2].x, rv[2].y,
                rv[3].x, rv[3].y,
                rv[4].x, rv[4].y,
                l - r->layers);
            bbox = toporouter_bbox_create(l - r->layers, vlist, PAD, pad);
            r->bboxes = g_slist_prepend(r->bboxes, bbox);
            insert_constraints_from_list(r, l, vlist, bbox);
            g_list_free(vlist);
            
            //bbox->point = GTS_POINT( gts_vertex_new(vertex_class, (x[0] + x[1]) / 2., (y[0] + y[1]) / 2., 0.) );
            bbox->point = GTS_POINT( insert_vertex(r, l, (x[0] + x[1]) / 2., (y[0] + y[1]) / 2., bbox) );
            
            //bbox->constraints = g_list_concat(bbox->constraints, insert_constraint_edge(r, l, x[0], y[0], x[1], y[1], bbox));

          }


        }

      }
    }
    END_LOOP;
  }
  END_LOOP;

  return 0;
}

/*!
 * \brief Read points data (all layers) into GList.
 *
 * Inserts pin and via points.
 */
int
read_points(toporouter_t *r, toporouter_layer_t *l, int layer)
{
  gdouble x, y, t;
  
  GList *vlist = NULL;
  toporouter_bbox_t *bbox = NULL;
  
  ELEMENT_LOOP(PCB->Data);
  {
    PIN_LOOP(element);
    {
      
      t = (gdouble)pin->Thickness / 2.0f;
      x = pin->X;
      y = pin->Y;

      if(TEST_FLAG (SQUAREFLAG, pin)) {
            
        vlist = rect_with_attachments(pin_rad(pin), 
            x-t, y-t, 
            x-t, y+t,
            x+t, y+t,
            x+t, y-t,
            l - r->layers);
        bbox = toporouter_bbox_create(l - r->layers, vlist, PIN, pin);
        r->bboxes = g_slist_prepend(r->bboxes, bbox);
        insert_constraints_from_list(r, l, vlist, bbox);
        g_list_free(vlist);
        bbox->point = GTS_POINT( insert_vertex(r, l, x, y, bbox) );
        
      }else if(TEST_FLAG(OCTAGONFLAG, pin)){
        /* TODO: Handle octagon pins */
        fprintf(stderr, "No support for octagon pins yet\n");
      }else{
        vlist = rect_with_attachments(pin_rad(pin), 
            x-t, y-t, 
            x-t, y+t,
            x+t, y+t,
            x+t, y-t,
            l - r->layers);
        bbox = toporouter_bbox_create(l - r->layers, vlist, PIN, pin);
        r->bboxes = g_slist_prepend(r->bboxes, bbox);
        g_list_free(vlist);
        bbox->point = GTS_POINT( insert_vertex(r, l, x, y, bbox) );
      }
    }
    END_LOOP;
  }
  END_LOOP;
 
  VIA_LOOP(PCB->Data);
  {

    t = (gdouble)via->Thickness / 2.0f;
    x = via->X;
    y = via->Y;
    
    if(TEST_FLAG (SQUAREFLAG, via)) {
        
      vlist = rect_with_attachments(pin_rad((PinType*)via), 
          x-t, y-t, 
          x-t, y+t,
          x+t, y+t,
          x+t, y-t,
          l - r->layers);
      bbox = toporouter_bbox_create(l - r->layers, vlist, VIA, via);
      r->bboxes = g_slist_prepend(r->bboxes, bbox);
      insert_constraints_from_list(r, l, vlist, bbox);
      g_list_free(vlist);
      bbox->point = GTS_POINT( insert_vertex(r, l, x, y, bbox) );
        
    }else if(TEST_FLAG(OCTAGONFLAG, via)){
      /* TODO: Handle octagon vias */
      fprintf(stderr, "No support for octagon vias yet\n");
    }else{
      
      vlist = rect_with_attachments(pin_rad((PinType*)via), 
          x-t, y-t, 
          x-t, y+t,
          x+t, y+t,
          x+t, y-t,
          l - r->layers);
      bbox = toporouter_bbox_create(l - r->layers, vlist, VIA, via);
      r->bboxes = g_slist_prepend(r->bboxes, bbox);
      g_list_free(vlist);
        
      bbox->point = GTS_POINT( insert_vertex(r, l, x, y, bbox) );

    }
  }
  END_LOOP;
  return 0;
}

/*!
 * \brief Read line data from layer into toporouter_layer_t struct.
 *
 * Inserts points and constraints into GLists.
 */
int
read_lines(toporouter_t *r, toporouter_layer_t *l, LayerType *layer, int ln) 
{
  gdouble xs[2], ys[2];
  
  GList *vlist = NULL;
  toporouter_bbox_t *bbox = NULL;

  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  
  LINE_LOOP(layer);
  {
    xs[0] = line->Point1.X;
    xs[1] = line->Point2.X;
    ys[0] = line->Point1.Y;
    ys[1] = line->Point2.Y;
    if(!(xs[0] == xs[1] && ys[0] == ys[1])) {
      vlist = g_list_prepend(NULL, gts_vertex_new (vertex_class, xs[0], ys[0], l - r->layers));
      vlist = g_list_prepend(vlist, gts_vertex_new (vertex_class, xs[1], ys[1], l - r->layers));
      // TODO: replace this with surface version
      bbox = toporouter_bbox_create_from_points(GetLayerGroupNumberByNumber(ln), vlist, LINE, line);
      r->bboxes = g_slist_prepend(r->bboxes, bbox);
      //new;;
      //insert_constraints_from_list(r, l, vlist, bbox);
      g_list_free(vlist);
//      bbox->point = GTS_POINT( insert_vertex(r, l, (xs[0]+xs[1])/2., (ys[0]+ys[1])/2., bbox) );
      
      bbox->constraints = g_list_concat(bbox->constraints, insert_constraint_edge(r, l, xs[0], ys[0], 0, xs[1], ys[1], 0, bbox));
    }
  }
  END_LOOP;
  
  return 0;
}

static void
create_board_edge (double x0, double y0,
                   double x1, double y1,
                   int layer,
                   GList **vlist)
{
  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  double edge_length = hypot (x0 - x1, y0 - y1);
  unsigned int vertices_per_edge = MIN (1, edge_length / BOARD_EDGE_RESOLUTION);
  unsigned int count;
  double inc = edge_length / vertices_per_edge;
  double x = x0, y = y0;

  *vlist = g_list_prepend (*vlist, gts_vertex_new (vertex_class, x, y, layer));

  for (count = 1; count < vertices_per_edge; count ++) {
    coord_move_towards_coord_values (x, y, x1, y1, inc, &x, &y);
    *vlist = g_list_prepend (*vlist, gts_vertex_new (vertex_class, x, y, layer));
  }
}


static void
read_board_constraints (toporouter_t *r, toporouter_layer_t *l, int layer)
{
  GList *vlist = NULL;
  toporouter_bbox_t *bbox;

  /* Create points for the board edges and constrain those edges */
  create_board_edge (0.,            0.,             PCB->MaxWidth, 0.,             layer, &vlist);
  create_board_edge (PCB->MaxWidth, 0.,             PCB->MaxWidth, PCB->MaxHeight, layer, &vlist);
  create_board_edge (PCB->MaxWidth, PCB->MaxHeight, 0.,            PCB->MaxHeight, layer, &vlist);
  create_board_edge (0.,            PCB->MaxHeight, 0.,            0.,             layer, &vlist);

  bbox = toporouter_bbox_create (layer, vlist, BOARD, NULL);
  r->bboxes = g_slist_prepend (r->bboxes, bbox);
  insert_constraints_from_list (r, l, vlist, bbox);
  g_list_free (vlist);
}

gdouble 
triangle_cost(GtsTriangle *t, gpointer *data){

  gdouble *min_quality = (gdouble *)data[0];
  gdouble *max_area = (gdouble *)data[1];
  gdouble quality = gts_triangle_quality(t);
  gdouble area = gts_triangle_area(t);
  
  if (quality < *min_quality || area > *max_area)
    return quality;
  return 0.0;
}


void
print_constraint(toporouter_constraint_t *e)
{
  printf("CONSTRAINT:\n");
  print_vertex(tedge_v1(e));
  print_vertex(tedge_v2(e));
}

void
print_edge(toporouter_edge_t *e)
{
  GList *i = edge_routing(e);

  printf("EDGE:\n");
 
  print_vertex(tedge_v1(e));
  
  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);
    print_vertex(v);
    i = i->next;
  }
  
  print_vertex(tedge_v2(e));
}
static void pick_first_face (GtsFace * f, GtsFace ** first)
{
  if (*first == NULL)
    *first = f;
}

void
unconstrain(toporouter_layer_t *l, toporouter_constraint_t *c) 
{
  toporouter_edge_t *e;

  gts_allow_floating_vertices = TRUE;
  e = TOPOROUTER_EDGE(gts_edge_new (GTS_EDGE_CLASS (toporouter_edge_class ()), GTS_SEGMENT(c)->v1, GTS_SEGMENT(c)->v2));
  gts_edge_replace(GTS_EDGE(c), GTS_EDGE(e));
  l->constraints = g_list_remove(l->constraints, c);
  c->box->constraints = g_list_remove(c->box->constraints, c);
  c->box = NULL;
  gts_object_destroy (GTS_OBJECT (c));
  gts_allow_floating_vertices = FALSE;
}

void
build_cdt(toporouter_t *r, toporouter_layer_t *l) 
{
  /* TODO: generalize into surface *cdt_create(vertices, constraints) */
  GList *i;
  //GtsEdge *temp;
  //GtsVertex *v;
  GtsTriangle *t;
  GtsVertex *v1, *v2, *v3;
  GSList *vertices_slist;

  vertices_slist = list_to_slist(l->vertices);

  if(l->surface) {
    GtsFace * first = NULL;
    gts_surface_foreach_face (l->surface, (GtsFunc) pick_first_face, &first);
    gts_surface_traverse_destroy(gts_surface_traverse_new (l->surface, first));
  }
  
  t = gts_triangle_enclosing (gts_triangle_class (), vertices_slist, 1000.0f);
  gts_triangle_vertices (t, &v1, &v2, &v3);

  g_slist_free(vertices_slist);
  
  l->surface = gts_surface_new (gts_surface_class (), gts_face_class (),
      GTS_EDGE_CLASS(toporouter_edge_class ()), GTS_VERTEX_CLASS(toporouter_vertex_class ()) );

  gts_surface_add_face (l->surface, gts_face_new (gts_face_class (), t->e1, t->e2, t->e3));


//  fprintf(stderr, "ADDED VERTICES\n");
/*
  GtsFace *debugface;

  if((debugface = gts_delaunay_check(l->surface))) {
    fprintf(stderr, "WARNING: Delaunay check failed\n");
    fprintf(stderr, "\tViolating triangle:\n");
    fprintf(stderr, "\t%f,%f %f,%f\n",
        debugface->triangle.e1->segment.v1->p.x,
        debugface->triangle.e1->segment.v1->p.y,
        debugface->triangle.e1->segment.v2->p.x,
        debugface->triangle.e1->segment.v2->p.y
        );
    fprintf(stderr, "\t%f,%f %f,%f\n",
        debugface->triangle.e2->segment.v1->p.x,
        debugface->triangle.e2->segment.v1->p.y,
        debugface->triangle.e2->segment.v2->p.x,
        debugface->triangle.e2->segment.v2->p.y
        );
    fprintf(stderr, "\t%f,%f %f,%f\n",
        debugface->triangle.e3->segment.v1->p.x,
        debugface->triangle.e3->segment.v1->p.y,
        debugface->triangle.e3->segment.v2->p.x,
        debugface->triangle.e3->segment.v2->p.y
        );
//    toporouter_draw_surface(r, l->surface, "debug.png", 4096, 4096);
  {
    int i;
    for(i=0;i<groupcount();i++) {
      char buffer[256];
      sprintf(buffer, "debug-%d.png", i);
      toporouter_draw_surface(r, r->layers[i].surface, buffer, 2048, 2048, 2, NULL, i, NULL);
    }
  }
    
  }
*/
check_cons_continuation:  
  i = l->constraints;
  while (i) {  
    toporouter_constraint_t *c1 = TOPOROUTER_CONSTRAINT(i->data);
    GList *j = i->next;
   // printf("adding cons: "); print_constraint(c1);
    
    while(j) {
      toporouter_constraint_t *c2 = TOPOROUTER_CONSTRAINT(j->data);
      guint rem = 0;
      GList *temp;
      
    //  printf("\tconflict: "); print_constraint(c2);
      toporouter_bbox_t *c1box = c1->box, *c2box = c2->box;
      toporouter_vertex_t *c1v1 = tedge_v1(c1);
      toporouter_vertex_t *c1v2 = tedge_v2(c1);
      toporouter_vertex_t *c2v1 = tedge_v1(c2);
      toporouter_vertex_t *c2v2 = tedge_v2(c2);

      if(gts_segments_are_intersecting(GTS_SEGMENT(c1), GTS_SEGMENT(c2)) == GTS_IN) {
        toporouter_vertex_t *v;
        unconstrain(l, c1); unconstrain(l, c2); 
        rem = 1;
        // proper intersection
        v = TOPOROUTER_VERTEX(vertex_intersect(
              GTS_VERTEX(c1v1),
              GTS_VERTEX(c1v2),
              GTS_VERTEX(c2v1),
              GTS_VERTEX(c2v2)));

        // remove both constraints
        // replace with 4x constraints
        // insert new intersection vertex
        GTS_POINT(v)->z = vz(c1v1);

        l->vertices = g_list_prepend(l->vertices, v);
//        gts_delaunay_add_vertex (l->surface, GTS_VERTEX(v), NULL);

        v->bbox = c1box;

        temp = insert_constraint_edge(r, l, vx(c1v1), vy(c1v1), 0, vx(v), vy(v), 0, c1box);
        c1box->constraints = g_list_concat(c1box->constraints, temp);

        temp = insert_constraint_edge(r, l, vx(c1v2), vy(c1v2), 0, vx(v), vy(v), 0, c1box);
        c1box->constraints = g_list_concat(c1box->constraints, temp);

        temp = insert_constraint_edge(r, l, vx(c2v1), vy(c2v1), 0, vx(v), vy(v), 0, c2box);
        c2box->constraints = g_list_concat(c2box->constraints, temp);

        temp = insert_constraint_edge(r, l, vx(c2v2), vy(c2v2), 0, vx(v), vy(v), 0, c2box);
        c2box->constraints = g_list_concat(c2box->constraints, temp);

      }else if(gts_segments_are_intersecting(GTS_SEGMENT(c1), GTS_SEGMENT(c2)) == GTS_ON ||
          gts_segments_are_intersecting(GTS_SEGMENT(c2), GTS_SEGMENT(c1)) == GTS_ON) {

        if(vertex_between(edge_v1(c2), edge_v2(c2), edge_v1(c1)) && vertex_between(edge_v1(c2), edge_v2(c2), edge_v2(c1))) {
          unconstrain(l, c1); unconstrain(l, c2); 
          rem = 1;
          // remove c1
          temp = insert_constraint_edge(r, l, vx(c2v1), vy(c2v1), 0, vx(c2v2), vy(c2v2), 0, c2box);
          c2box->constraints = g_list_concat(c2box->constraints, temp);

        }else if(vertex_between(edge_v1(c1), edge_v2(c1), edge_v1(c2)) && vertex_between(edge_v1(c1), edge_v2(c1), edge_v2(c2))) {
          unconstrain(l, c1); unconstrain(l, c2); 
          rem = 1;
          // remove c2
          temp = insert_constraint_edge(r, l, vx(c1v1), vy(c1v1), 0, vx(c1v2), vy(c1v2), 0, c1box);
          c1box->constraints = g_list_concat(c1box->constraints, temp);

        //}else if(!vertex_wind(edge_v1(c1), edge_v2(c1), edge_v1(c2)) && !vertex_wind(edge_v1(c1), edge_v2(c1), edge_v2(c2))) {
   /*     }else if(vertex_between(edge_v1(c1), edge_v2(c1), edge_v1(c2)) || vertex_between(edge_v1(c1), edge_v2(c1), edge_v2(c2))) {
          unconstrain(l, c1); unconstrain(l, c2); 
          rem = 1;
          printf("all colinear\n");
          //   exit(1);
          temp = insert_constraint_edge(r, l, vx(c1v1), vy(c1v1), 0, vx(c1v2), vy(c1v2), 0, c1box);
          c1box->constraints = g_list_concat(c1box->constraints, temp);

          if(vertex_between(GTS_VERTEX(c1v1), GTS_VERTEX(c1v2), GTS_VERTEX(c2v2))) {
            // v2 of c2 is inner
            if(vertex_between(GTS_VERTEX(c2v1), GTS_VERTEX(c2v2), GTS_VERTEX(c1v2))) {
              // v2 of c1 is inner
              // c2 = c1.v2 -> c2.v1
              temp = insert_constraint_edge(r, l, vx(c1v2), vy(c1v2), 0, vx(c2v1), vy(c2v1), 0, c2box);
              c2box->constraints = g_list_concat(c2box->constraints, temp);
            }else{
              // v1 of c1 is inner
              // c2 = c1.v1 -> c2.v1
              temp = insert_constraint_edge(r, l, vx(c1v1), vy(c1v1), 0, vx(c2v1), vy(c2v1), 0, c2box);
              c2box->constraints = g_list_concat(c2box->constraints, temp);
            }
          }else{
            // v1 of c2 is inner
            if(vertex_between(GTS_VERTEX(c2v1), GTS_VERTEX(c2v2), GTS_VERTEX(c1v2))) {
              // v2 of c1 is inner
              // c2 = c1.v2 -> c2.v2
              temp = insert_constraint_edge(r, l, vx(c1v2), vy(c1v2), 0, vx(c2v2), vy(c2v2), 0, c2box);
              c2box->constraints = g_list_concat(c2box->constraints, temp);
            }else{
              // v1 of c1 is inner
              // c2 = c1.v1 -> c2.v2
              temp = insert_constraint_edge(r, l, vx(c1v1), vy(c1v1), 0, vx(c2v2), vy(c2v2), 0, c2box);
              c2box->constraints = g_list_concat(c2box->constraints, temp);
            }
          }*/
        }else if(vertex_between(edge_v1(c2), edge_v2(c2), edge_v1(c1)) && c1v1 != c2v1 && c1v1 != c2v2) {
          unconstrain(l, c1); unconstrain(l, c2); 
          rem = 1;
          //v1 of c1 is on c2
          printf("v1 of c1 on c2\n"); 

          // replace with 2x constraints
          temp = insert_constraint_edge(r, l, vx(c2v1), vy(c2v1), 0, vx(c1v1), vy(c1v1), 0, c2box);
          c2box->constraints = g_list_concat(c2box->constraints, temp);
          temp = insert_constraint_edge(r, l, vx(c2v2), vy(c2v2), 0, vx(c1v1), vy(c1v1), 0, c2box);
          c2box->constraints = g_list_concat(c2box->constraints, temp);

          temp = insert_constraint_edge(r, l, vx(c1v1), vy(c1v1), 0, vx(c1v2), vy(c1v2), 0, c1box);
          c1box->constraints = g_list_concat(c1box->constraints, temp);

          // restore c1
          //temp = insert_constraint_edge(r, l, vx(tedge_v2(c1)), vy(tedge_v2(c1)), 0, vx(tedge_v1(c1)), vy(tedge_v1(c1)), 0, c1->box);
          //c2->box->constraints = g_list_concat(c2->box->constraints, temp);

        }else if(vertex_between(edge_v1(c2), edge_v2(c2), edge_v2(c1)) && c1v2 != c2v1 && c1v2 != c2v2) {
          unconstrain(l, c1); unconstrain(l, c2); 
          rem = 1;
          //v2 of c1 is on c2
          printf("v2 of c1 on c2\n"); 

          // replace with 2x constraints
          temp = insert_constraint_edge(r, l, vx(c2v1), vy(c2v1), 0, vx(c1v2), vy(c1v2), 0, c2box);
          c2box->constraints = g_list_concat(c2box->constraints, temp);
          temp = insert_constraint_edge(r, l, vx(c2v2), vy(c2v2), 0, vx(c1v2), vy(c1v2), 0, c2box);
          c2box->constraints = g_list_concat(c2box->constraints, temp);

          temp = insert_constraint_edge(r, l, vx(c1v1), vy(c1v1), 0, vx(c1v2), vy(c1v2), 0, c1box);
          c1box->constraints = g_list_concat(c1box->constraints, temp);

        }else if(vertex_between(edge_v1(c1), edge_v2(c1), edge_v1(c2)) && c2v1 != c1v1 && c2v1 != c1v2) {
          unconstrain(l, c1); unconstrain(l, c2); 
          rem = 1;
          //v1 of c2 is on c1
          printf("v1 of c2 on c1\n"); 

          // replace with 2x constraints
          temp = insert_constraint_edge(r, l, vx(c1v1), vy(c1v1), 0, vx(c2v1), vy(c2v1), 0, c1box);
          c1box->constraints = g_list_concat(c1box->constraints, temp);
          temp = insert_constraint_edge(r, l, vx(c1v2), vy(c1v2), 0, vx(c2v1), vy(c2v1), 0, c1box);
          c1box->constraints = g_list_concat(c1box->constraints, temp);

          temp = insert_constraint_edge(r, l, vx(c2v1), vy(c2v1), 0, vx(c2v2), vy(c2v2), 0, c2box);
          c2box->constraints = g_list_concat(c2box->constraints, temp);
        }else if(vertex_between(edge_v1(c1), edge_v2(c1), edge_v2(c2)) && c2v2 != c1v1 && c2v2 != c1v2) {
          unconstrain(l, c1); unconstrain(l, c2);
          rem = 1;
          //v2 of c2 is on c1
          printf("v2 of c2 on c1\n"); 

          // replace with 2x constraints
          temp = insert_constraint_edge(r, l, vx(c1v1), vy(c1v1), 0, vx(c2v2), vy(c2v2), 0, c1box);
          c1box->constraints = g_list_concat(c1box->constraints, temp);
          temp = insert_constraint_edge(r, l, vx(c1v2), vy(c1v2), 0, vx(c2v2), vy(c2v2), 0, c1box);
          c1box->constraints = g_list_concat(c1box->constraints, temp);

          temp = insert_constraint_edge(r, l, vx(c2v1), vy(c2v1), 0, vx(c2v2), vy(c2v2), 0, c2box);
          c2box->constraints = g_list_concat(c2box->constraints, temp);
        }
      }
      if(rem) goto check_cons_continuation; 

      j = j->next;
    }

    i = i->next; 
  }  

  i = l->vertices;
  while (i) {
    //v = i->data;
    //if(r->flags & TOPOROUTER_FLAG_DEBUG_CDTS) 
  //  fprintf(stderr, "\tadding vertex %f,%f\n", v->p.x, v->p.y);
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(gts_delaunay_add_vertex (l->surface, (GtsVertex *)i->data, NULL));
    if(v) {
      printf("conflict: "); print_vertex(v);
    }

    i = i->next;
  }
  i = l->constraints;
  while (i) {

    // toporouter_constraint_t *c1 = TOPOROUTER_CONSTRAINT(i->data);
    // printf("adding cons: "); print_constraint(c1);

    GSList *conflicts = gts_delaunay_add_constraint (l->surface, (GtsConstraint *)i->data);
    GSList *j = conflicts;
    while(j) {
      if(TOPOROUTER_IS_CONSTRAINT(j->data)) {
        toporouter_constraint_t *c2 = TOPOROUTER_CONSTRAINT(j->data);

        printf("\tconflict: "); print_constraint(c2);
      
      }
      j = j->next;
    }
    g_slist_free(conflicts);

    i = i->next;
  }

//  if(rerun) 
//        goto build_cdt_continuation;
//  fprintf(stderr, "ADDED CONSTRAINTS\n");
  gts_allow_floating_vertices = TRUE;
  gts_object_destroy (GTS_OBJECT (v1));
  gts_object_destroy (GTS_OBJECT (v2));
  gts_object_destroy (GTS_OBJECT (v3));
  gts_allow_floating_vertices = FALSE;
  
/*
  {
    gpointer data[2];
    gdouble quality = 0.50, area = G_MAXDOUBLE;
    guint num = gts_delaunay_conform(l->surface, -1, (GtsEncroachFunc) gts_vertex_encroaches_edge, NULL);

    if (num == 0){
      data[0] = &quality;
      data[1] = &area;
      num = gts_delaunay_refine(l->surface, -1, (GtsEncroachFunc) gts_vertex_encroaches_edge, NULL, (GtsKeyFunc) triangle_cost, data);
    }
  }
*/
#ifdef DEBUG_IMPORT  
  gts_surface_print_stats(l->surface, stderr);
#endif  
  
#if 0
  {
    char buffer[64];
    FILE *fout2;
    sprintf(buffer, "surface%d.gts", l - r->layers);
    fout2 = fopen(buffer, "w");
    gts_surface_write(l->surface, fout2);
  }
#endif

}

gint
visited_cmp(gconstpointer a, gconstpointer b)
{
  if(a<b) return -1;
  if(a>b) return 1;
  return 0;
}

static double
coord_angle (double ax, double ay, double bx, double by)
{
  return atan2 (by - ay, bx - ax);
}

GList *
cluster_vertices(toporouter_t *r, toporouter_cluster_t *c)
{
  GList *rval = NULL;
  
  if(!c) return NULL;

  FOREACH_CLUSTER(c->netlist->clusters) {
    if((r->flags & TOPOROUTER_FLAG_AFTERRUBIX && cluster->c == c->c) || (!(r->flags & TOPOROUTER_FLAG_AFTERRUBIX) && cluster == c)) {
      FOREACH_BBOX(cluster->boxes) {
        if(box->type == LINE) {
          g_assert(box->constraints->data);
          rval = g_list_prepend(rval, tedge_v1(box->constraints->data));
          rval = g_list_prepend(rval, tedge_v2(box->constraints->data));
        }else if(box->point) {
          rval = g_list_prepend(rval, TOPOROUTER_VERTEX(box->point));
          //g_assert(vertex_bbox(TOPOROUTER_VERTEX(box->point)) == box);
        }else {
          printf("WARNING: cluster_vertices: unhandled bbox type\n");
        }

      } FOREACH_END;


    }
      
  } FOREACH_END;

  return rval;
}

void
print_cluster(toporouter_cluster_t *c)
{

  if(!c) {
    printf("[CLUSTER (NULL)]\n");
    return;
  }

  printf("CLUSTER %d: NETLIST = %s STYLE = %s\n", c->c, c->netlist->netlist, c->netlist->style);

  FOREACH_BBOX(c->boxes) {
    print_bbox(box);
  } FOREACH_END;
}


toporouter_cluster_t *
cluster_create(toporouter_t *r, toporouter_netlist_t *netlist)
{
  toporouter_cluster_t *c = (toporouter_cluster_t *)malloc(sizeof(toporouter_cluster_t));

  c->c = c->pc = netlist->clusters->len;
  g_ptr_array_add(netlist->clusters, c);
  c->netlist = netlist;
  c->boxes = g_ptr_array_new();

  return c;
}

toporouter_bbox_t *
toporouter_bbox_locate(toporouter_t *r, toporouter_term_t type, void *data, gdouble x, gdouble y, guint layergroup)
{
  GtsPoint *p = gts_point_new(gts_point_class(), x, y, layergroup);
  GSList *boxes = gts_bb_tree_stabbed(r->bboxtree, p), *i = boxes;
  
  gts_object_destroy(GTS_OBJECT(p));
  
  while(i) {
    toporouter_bbox_t *box = TOPOROUTER_BBOX(i->data);

    if(box->type == type && box->data == data) {
      g_slist_free(boxes);
      return box;       
    }
    
    i = i->next;
  }

  g_slist_free(boxes);
  return NULL;
}

void
cluster_join_bbox(toporouter_cluster_t *cluster, toporouter_bbox_t *box)
{
  if(box) {
    g_ptr_array_add(cluster->boxes, box);
    box->cluster = cluster;
  }
}

toporouter_netlist_t *
netlist_create(toporouter_t *r, char *netlist, char *style)
{
  toporouter_netlist_t *nl = (toporouter_netlist_t *)malloc(sizeof(toporouter_netlist_t));
  nl->netlist = netlist; 
  nl->style = style;
  nl->clusters = g_ptr_array_new();
  nl->routes = g_ptr_array_new();
  nl->routed = NULL;
  nl->pair = NULL;
  g_ptr_array_add(r->netlists, nl);
  return nl;
}

void
import_clusters(toporouter_t *r)
{
  NetListListType nets;
  nets = CollectSubnets(false);
  NETLIST_LOOP(&nets);
  {
    if(netlist->NetN > 0) {
      toporouter_netlist_t *nl = netlist_create(r, netlist->Net->Connection->menu->Name, netlist->Net->Connection->menu->Style);
      
      NET_LOOP(netlist);
      {

        toporouter_cluster_t *cluster = cluster_create(r, nl);
#ifdef DEBUG_MERGING  
        printf("NET:\n");
#endif        
        CONNECTION_LOOP (net);
        {

          if(connection->type == LINE_TYPE) {
            LineType *line = (LineType *) connection->ptr2;
            toporouter_bbox_t *box = toporouter_bbox_locate(r, LINE, line, connection->X, connection->Y, connection->group);
            cluster_join_bbox(cluster, box);

#ifdef DEBUG_MERGING  
            pcb_printf("\tLINE %#mD\n", connection->X, connection->Y);
#endif        
          }else if(connection->type == PAD_TYPE) {
            PadType *pad = (PadType *) connection->ptr2;
            toporouter_bbox_t *box = toporouter_bbox_locate(r, PAD, pad, connection->X, connection->Y, connection->group);
            cluster_join_bbox(cluster, box);

#ifdef DEBUG_MERGING  
            pcb_printf("\tPAD %#mD\n", connection->X, connection->Y);
#endif        
          }else if(connection->type == PIN_TYPE) {

            for(guint m=0;m<groupcount();m++) {
              PinType *pin = (PinType *) connection->ptr2;
              toporouter_bbox_t *box = toporouter_bbox_locate(r, PIN, pin, connection->X, connection->Y, m);
              cluster_join_bbox(cluster, box);
            }

#ifdef DEBUG_MERGING  
            pcb_printf("\tPIN %#mD\n", connection->X, connection->Y);
#endif        
          }else if(connection->type == VIA_TYPE) {

            for(guint m=0;m<groupcount();m++) {
              PinType *pin = (PinType *) connection->ptr2;
              toporouter_bbox_t *box = toporouter_bbox_locate(r, VIA, pin, connection->X, connection->Y, m);
              cluster_join_bbox(cluster, box);
            }

#ifdef DEBUG_MERGING  
            pcb_printf("\tVIA %#mD\n", connection->X, connection->Y);
#endif        
          }else if(connection->type == POLYGON_TYPE) {
            PolygonType *polygon = (PolygonType *) connection->ptr2;
            toporouter_bbox_t *box = toporouter_bbox_locate(r, POLYGON, polygon, connection->X, connection->Y, connection->group);
            cluster_join_bbox(cluster, box);

#ifdef DEBUG_MERGING  
            pcb_printf("\tPOLYGON %#mD\n", connection->X, connection->Y);
#endif        

          }
        }
        END_LOOP;
#ifdef DEBUG_MERGING  
        printf("\n");
#endif        
      }
      END_LOOP;

    }
  }
  END_LOOP;
  FreeNetListListMemory(&nets);
}

void
import_geometry(toporouter_t *r) 
{
  toporouter_layer_t *cur_layer;
  
  int group;

#ifdef DEBUG_IMPORT    
  for (group = 0; group < max_group; group++) {
    printf("Group %d: Number %d:\n", group, PCB->LayerGroups.Number[group]);

    for (int entry = 0; entry < PCB->LayerGroups.Number[group]; entry++) {
        printf("\tEntry %d\n", PCB->LayerGroups.Entries[group][entry]);
    }
  }
#endif
  /* Allocate space for per layer struct */
  cur_layer = r->layers = (toporouter_layer_t *)malloc(groupcount() * sizeof(toporouter_layer_t));

  /* Foreach layer, read in pad vertices and constraints, and build CDT */
  for (group = 0; group < max_group; group++) {
#ifdef DEBUG_IMPORT    
    printf("*** LAYER GROUP %d ***\n", group);
#endif
    if(PCB->LayerGroups.Number[group] > 0){ 
      cur_layer->vertices    = NULL;
      cur_layer->constraints = NULL;

#ifdef DEBUG_IMPORT    
      printf("reading board constraints from layer %d into group %d\n", PCB->LayerGroups.Entries[group][0], group);
#endif
      read_board_constraints(r, cur_layer, PCB->LayerGroups.Entries[group][0]);
#ifdef DEBUG_IMPORT    
      printf("reading points from layer %d into group %d \n",PCB->LayerGroups.Entries[group][0], group);
#endif
      read_points(r, cur_layer, PCB->LayerGroups.Entries[group][0]);

//#ifdef DEBUG_IMPORT    
//      printf("reading pads from layer %d into group %d\n", number, group);
//#endif
      read_pads(r, cur_layer, group);

      GROUP_LOOP(PCB->Data, group)
      {

#ifdef DEBUG_IMPORT    
        printf("reading lines from layer %d into group %d\n", number, group);
#endif
        read_lines(r, cur_layer, layer, number);

      }
      END_LOOP;



#ifdef DEBUG_IMPORT    
      printf("building CDT\n");
#endif
      build_cdt(r, cur_layer);
  printf("finished\n");
/*      {
    int i;
    for(i=0;i<groupcount();i++) {
      char buffer[256];
      sprintf(buffer, "build%d.png", i);
      toporouter_draw_surface(r, r->layers[i].surface, buffer, 2048, 2048, 2, NULL, i, NULL);
    }
  }*/
#ifdef DEBUG_IMPORT    
      printf("finished building CDT\n");
#endif
      cur_layer++;
    }
  }
  
  r->bboxtree = gts_bb_tree_new(r->bboxes);
 
  import_clusters(r);

#ifdef DEBUG_IMPORT
  printf("finished import!\n");
#endif  
}


gint
compare_points(gconstpointer a, gconstpointer b)
{
  GtsPoint *i = GTS_POINT(a);
  GtsPoint *j = GTS_POINT(b);

  if(i->x == j->x) {
    if(i->y == j->y) return 0;
    if(i->y < j->y) return -1;
    return 1;
  }
  if(i->x < j->x) return -1;
  return 1;
}

gint
compare_segments(gconstpointer a, gconstpointer b)
{
  if(a == b) return 0;
  if(a < b) return -1;
  return 1;
}
#define DEBUG_CLUSTER_FIND 1
toporouter_cluster_t *
cluster_find(toporouter_t *r, gdouble x, gdouble y, gdouble z)
{
  GtsPoint *p = gts_point_new(gts_point_class(), x, y, z);
  GSList *hits = gts_bb_tree_stabbed(r->bboxtree, p);
  toporouter_cluster_t *rval = NULL;

#ifdef DEBUG_CLUSTER_FIND
  printf("FINDING %f,%f,%f\n\n", x, y, z);
#endif

  while(hits) {
    toporouter_bbox_t *box = TOPOROUTER_BBOX(hits->data);
    
#ifdef DEBUG_CLUSTER_FIND
    printf("HIT BOX: "); print_bbox(box);
#endif

    if(box->layer == (int)z) { 
      if(box->type != BOARD) {
        if(box->type == LINE) {
          LineType *line = (LineType *)box->data;
          gint linewind = coord_wind(line->Point1.X, line->Point1.Y, x, y, line->Point2.X, line->Point2.Y);

          if(line->Point1.X > x - EPSILON && line->Point1.X < x + EPSILON &&
              line->Point1.Y > y - EPSILON && line->Point1.Y < y + EPSILON) {
            rval = box->cluster;
          //  break;
          }
          if(line->Point2.X > x - EPSILON && line->Point2.X < x + EPSILON &&
              line->Point2.Y > y - EPSILON && line->Point2.Y < y + EPSILON) {
            rval = box->cluster;
          //  break;
          }
          if(!linewind) {
            rval = box->cluster;
          //  break;
          }

        }else if(box->surface) {

          if(gts_point_locate(p, box->surface, NULL)) {
            rval = box->cluster;
            break;
          }

        }
      }
    }
    hits = hits->next;
  }
  
  gts_object_destroy(GTS_OBJECT(p));
 

#ifdef DEBUG_CLUSTER_FIND
  printf("cluster_find: %f,%f,%f: ", x, y, z);
  print_cluster(rval);
#endif

  return rval;
}

gdouble
simple_h_cost(toporouter_t *r, toporouter_vertex_t *curpoint, toporouter_vertex_t *destpoint)
{
  gdouble layerpenalty = (vz(curpoint) == vz(destpoint)) ? 0. : r->viacost;

  return gts_point_distance(GTS_POINT(curpoint), GTS_POINT(destpoint)) + layerpenalty;
}

#define FCOST(x) (x->gcost + x->hcost)
gdouble     
route_heap_cmp(gpointer item, gpointer data)
{
  return FCOST(TOPOROUTER_VERTEX(item));  
}

#define closelist_insert(p) closelist = g_list_prepend(closelist, p)

typedef struct {
  toporouter_vertex_t *key;
  toporouter_vertex_t *result;
}toporouter_heap_search_data_t;

void 
toporouter_heap_search(gpointer data, gpointer user_data)
{
  toporouter_vertex_t *v = TOPOROUTER_VERTEX(data);
  toporouter_heap_search_data_t *heap_search_data = (toporouter_heap_search_data_t *)user_data;
  if(v == heap_search_data->key) heap_search_data->result = v;
}
/*
void 
toporouter_heap_color(gpointer data, gpointer user_data)
{
  toporouter_vertex_t *v = TOPOROUTER_VERTEX(data);
  v->flags |= (guint) user_data;
}
*/
static inline gdouble
angle_span(gdouble a1, gdouble a2)
{
  if(a1 > a2) 
    return ((2*M_PI)-a1 + a2);  
  return a2-a1;
}

gdouble
edge_capacity(toporouter_edge_t *e)
{
  return gts_point_distance(GTS_POINT(edge_v1(e)), GTS_POINT(edge_v2(e)));
}

gdouble
edge_flow(toporouter_edge_t *e, toporouter_vertex_t *v1, toporouter_vertex_t *v2, toporouter_vertex_t *dest)
{
  GList *i = edge_routing(e);
  toporouter_vertex_t *pv = tedge_v1(e), *v = NULL;
  gdouble flow = 0.;
  guint waiting = 1;

  if((pv == v1 || pv == v2) && waiting) {
    flow += min_vertex_net_spacing(pv, dest);
    pv = dest;
    waiting = 0;
  }

  g_assert(v1 != v2);

  while(i) {
    v = TOPOROUTER_VERTEX(i->data);
   

    if(pv == dest)
      flow += min_vertex_net_spacing(v, pv);
    else
      flow += min_spacing(v, pv);
    
    if((v == v1 || v == v2) && waiting) {
      flow += min_vertex_net_spacing(v, dest);
      pv = dest;
      waiting = 0;
    }else{
      pv = v;
    }
    i = i->next;
  }
  
  if(pv == dest)
    flow += min_vertex_net_spacing(tedge_v2(e), pv);
  else
    flow += min_spacing(tedge_v2(e), pv);

  return flow;
}
   
void
print_path(GList *path)
{
  GList *i = path;

  printf("PATH:\n");
  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);
//    printf("[V %f,%f,%f]\n", vx(v), vy(v), vz(v));
    print_vertex(v);

    if(v->child && !g_list_find(path, v->child)) 
      printf("\t CHILD NOT IN LIST\n");
    if(v->parent && !g_list_find(path, v->parent)) 
      printf("\t parent NOT IN LIST\n");
    i = i->next;
  }


}

GList *
split_path(GList *path) 
{
  toporouter_vertex_t *pv = NULL;
  GList *curpath = NULL, *i, *paths = NULL;
#ifdef DEBUG_ROUTE
  printf("PATH:\n");
#endif
  i = path;
  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);

#ifdef DEBUG_ROUTE
    printf("v = %f,%f ", vx(v), vy(v));
    if(v->parent) printf("parent = %f,%f ", vx(v->parent), vy(v->parent));
    if(v->child) printf("child = %f,%f ", vx(v->child), vy(v->child));
    printf("\n");
#endif
//    printf("***\n");
//    if(v) printf("v = %f,%f\n", GTS_POINT(v)->x, GTS_POINT(v)->y);
//    if(pv) printf("pv = %f,%f\n", GTS_POINT(pv)->x, GTS_POINT(pv)->y);
    
    
    if(pv)
    if(GTS_POINT(v)->x == GTS_POINT(pv)->x && GTS_POINT(v)->y == GTS_POINT(pv)->y) {
      if(g_list_length(curpath) > 1) paths = g_list_prepend(paths, curpath);
      curpath = NULL;

      pv->child = NULL;
      v->parent = NULL;
    }
    
    curpath = g_list_append(curpath, v);

    pv = v;
    i = i->next;
  }
  
  if(g_list_length(curpath) > 1)
    paths = g_list_prepend(paths, curpath);
  
  return paths;
}



#define edge_gradient(e) (cartesian_gradient(GTS_POINT(GTS_SEGMENT(e)->v1)->x, GTS_POINT(GTS_SEGMENT(e)->v1)->y, \
    GTS_POINT(GTS_SEGMENT(e)->v2)->x, GTS_POINT(GTS_SEGMENT(e)->v2)->y))


/*!
 * \brief Sorting into ascending distance from v1.
 */
gint
routing_edge_insert(gconstpointer a, gconstpointer b, gpointer user_data)
{
  GtsPoint *v1 = GTS_POINT(edge_v1(user_data));

  if(gts_point_distance2(v1, GTS_POINT(a)) < gts_point_distance2(v1, GTS_POINT(b)) - EPSILON)
    return -1;
  if(gts_point_distance2(v1, GTS_POINT(a)) > gts_point_distance2(v1, GTS_POINT(b)) + EPSILON)
    return 1;
/*
  printf("a = %x b = %x\n", (int) a, (int) b);

  printf("WARNING: routing_edge_insert() with same points..\n \
      v1 @ %f,%f\n\
      a  @ %f,%f\n\
      b  @ %f,%f\n", 
      v1->x, v1->y,
      vx(a), vy(a),
      vx(a), vy(b));
  printf("A: "); print_vertex(TOPOROUTER_VERTEX(a));
  printf("B: "); print_vertex(TOPOROUTER_VERTEX(b));

  TOPOROUTER_VERTEX(a)->flags |= VERTEX_FLAG_RED;
  TOPOROUTER_VERTEX(b)->flags |= VERTEX_FLAG_RED;
*/
  return 0;
}


toporouter_vertex_t *
new_temp_toporoutervertex(gdouble x, gdouble y, toporouter_edge_t *e) 
{
  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  GList *i = edge_routing(e);
  toporouter_vertex_t *r;
///*
  while(i) {
    r = TOPOROUTER_VERTEX(i->data);
    if(epsilon_equals(vx(r),x) && epsilon_equals(vy(r),y)) {
      if(r->flags & VERTEX_FLAG_TEMP) return r;
    }
    i = i->next;
  }
//*/
  r = TOPOROUTER_VERTEX( gts_vertex_new (vertex_class, x, y, vz(edge_v1(e))) );
  r->flags |= VERTEX_FLAG_TEMP;
  r->routingedge = e;

  if(TOPOROUTER_IS_CONSTRAINT(e))
    TOPOROUTER_CONSTRAINT(e)->routing = g_list_insert_sorted_with_data(edge_routing(e), r, routing_edge_insert, e);
  else
    e->routing = g_list_insert_sorted_with_data(edge_routing(e), r, routing_edge_insert, e);
  
  return r;
}


/*!
 * \brief Create vertex on edge e at radius r from v, closest to ref.
 */
toporouter_vertex_t *
new_temp_toporoutervertex_in_segment(toporouter_edge_t *e, toporouter_vertex_t *v, gdouble r, toporouter_vertex_t *ref) 
{
  gdouble m = edge_gradient(e); 
  toporouter_spoint_t p, np1, np2;
//  toporouter_vertex_t *b = TOPOROUTER_VERTEX((GTS_VERTEX(v) == edge_v1(e)) ? edge_v2(e) : edge_v1(e));
  toporouter_vertex_t *rval = NULL; 
  p.x = vx(v); p.y = vy(v);

  vertices_on_line(&p, m, r, &np1, &np2);
  
  if( (pow(np1.x - vx(ref), 2) + pow(np1.y - vy(ref), 2)) < (pow(np2.x - vx(ref), 2) + pow(np2.y - vy(ref), 2)) )
    rval = new_temp_toporoutervertex(np1.x, np1.y, e);
  else 
    rval = new_temp_toporoutervertex(np2.x, np2.y, e);

  return rval;
}

gint
vertex_keepout_test(toporouter_t *r, toporouter_vertex_t *v) 
{
  GList *i = r->keepoutlayers;
  while(i) {
    gdouble keepout = *((double *) i->data);
    if(vz(v) == keepout) return 1; 
    i = i->next;
  }
  return 0;
}

void
closest_cluster_pair(toporouter_t *r, GList *src_vertices, GList *dest_vertices, toporouter_vertex_t **a, toporouter_vertex_t **b)
{
  GList *i = src_vertices, *j = dest_vertices;

  gdouble min = 0.;
  *a = NULL; *b = NULL;

  i = src_vertices;
  while(i) {
    toporouter_vertex_t *v1 = TOPOROUTER_VERTEX(i->data);

    if(vertex_keepout_test(r, v1)) { i = i->next; continue; }

    j = dest_vertices;
    while(j) {
      toporouter_vertex_t *v2 = TOPOROUTER_VERTEX(j->data);
      if(vertex_keepout_test(r, v2) || vz(v2) != vz(v1)) { j = j->next; continue; }

      if(!*a) {
        *a = v1; *b = v2; min = simple_h_cost(r, *a, *b);
      }else{
        gdouble tempd = simple_h_cost(r, v1, v2);
        if(r->flags & TOPOROUTER_FLAG_GOFAR && tempd > min) {
          *a = v1; *b = v2; min = tempd;
        }else 
        if(tempd < min) {
          *a = v1; *b = v2; min = tempd;
        }
      }

      j = j->next;
    }

    i = i->next;
  }

//  g_list_free(src_vertices);
//  g_list_free(dest_vertices);
}


toporouter_vertex_t *
closest_dest_vertex(toporouter_t *r, toporouter_vertex_t *v, toporouter_route_t *routedata)
{
  GList //*vertices = cluster_vertices(r, routedata->dest), 
        *i = routedata->destvertices;
  toporouter_vertex_t *closest = NULL;
  gdouble closest_distance = 0.;

//  if(routedata->flags & TOPOROUTER_FLAG_FLEX) i = r->destboxes;

  while(i) {
    toporouter_vertex_t *cv = TOPOROUTER_VERTEX(i->data);

    if(vz(cv) != vz(v)) { i = i->next; continue; }

    if(!closest) {
      closest = cv; closest_distance = simple_h_cost(r, v, closest);
    }else{
      gdouble tempd = simple_h_cost(r, v, cv);
      if(r->flags & TOPOROUTER_FLAG_GOFAR && tempd > closest_distance) {
        closest = cv; closest_distance = tempd;
      }else 
      if(tempd < closest_distance) {
        closest = cv; closest_distance = tempd;
      }
    }
    i = i->next;
  }

//  g_list_free(vertices);

#ifdef DEBUG_ROUTE
  printf("CLOSEST = %f,%f,%f\n", vx(closest), vy(closest), vz(closest));
#endif
  return closest;
}

#define toporouter_edge_gradient(e) (cartesian_gradient(vx(edge_v1(e)), vy(edge_v1(e)), vx(edge_v2(e)), vy(edge_v2(e))))


/*!
 * \brief Returns the capacity of the triangle cut through v.
 */
gdouble
triangle_interior_capacity(GtsTriangle *t, toporouter_vertex_t *v)
{
  toporouter_edge_t *e = TOPOROUTER_EDGE(gts_triangle_edge_opposite(t, GTS_VERTEX(v)));
  gdouble x, y, m1, m2, c2, c1;
#ifdef DEBUG_ROUTE
  gdouble len;
#endif

  g_assert(e);

  m1 = toporouter_edge_gradient(e);
  m2 = perpendicular_gradient(m1);
  c2 = (isinf(m2)) ? vx(v) : vy(v) - (m2 * vx(v));
  c1 = (isinf(m1)) ? vx(edge_v1(e)) : vy(edge_v1(e)) - (m1 * vx(edge_v1(e)));

  if(isinf(m2))
    x = vx(v);
  else if(isinf(m1))
    x = vx(edge_v1(e));
  else
    x = (c2 - c1) / (m1 - m2);

  y = (isinf(m2)) ? vy(edge_v1(e)) : (m2 * x) + c2;

#ifdef DEBUG_ROUTE
  len = gts_point_distance2(GTS_POINT(edge_v1(e)), GTS_POINT(edge_v2(e)));
  printf("%f,%f len = %f v = %f,%f\n", x, y, len, vx(v), vy(v));
#endif

  if(epsilon_equals(x,vx(edge_v1(e))) && epsilon_equals(y,vy(edge_v1(e)))) return INFINITY;
  if(epsilon_equals(x,vx(edge_v2(e))) && epsilon_equals(y,vy(edge_v2(e)))) return INFINITY;

  if(x >= MIN(vx(edge_v1(e)),vx(edge_v2(e))) && 
     x <= MAX(vx(edge_v1(e)),vx(edge_v2(e))) && 
     y >= MIN(vy(edge_v1(e)),vy(edge_v2(e))) && 
     y <= MAX(vy(edge_v1(e)),vy(edge_v2(e))))  

//  if( (pow(vx(edge_v1(e)) - x, 2) + pow(vy(edge_v1(e)) - y, 2)) < len && (pow(vx(edge_v2(e)) - x, 2) + pow(vy(edge_v2(e)) - y, 2)) < len )
    return hypot(vx(v) - x, vy(v) - y);

  return INFINITY;
}

static inline toporouter_vertex_t *
segment_common_vertex(GtsSegment *s1, GtsSegment *s2) 
{
  if(!s1 || !s2) return NULL;
  if(s1->v1 == s2->v1) return TOPOROUTER_VERTEX(s1->v1);
  if(s1->v2 == s2->v1) return TOPOROUTER_VERTEX(s1->v2);
  if(s1->v1 == s2->v2) return TOPOROUTER_VERTEX(s1->v1);
  if(s1->v2 == s2->v2) return TOPOROUTER_VERTEX(s1->v2);
  return NULL;
}

static inline toporouter_vertex_t *
route_vertices_common_vertex(toporouter_vertex_t *v1, toporouter_vertex_t *v2)
{
  return segment_common_vertex(GTS_SEGMENT(v1->routingedge), GTS_SEGMENT(v2->routingedge));
}


static inline guint
edges_third_edge(GtsSegment *s1, GtsSegment *s2, toporouter_vertex_t **v1, toporouter_vertex_t **v2) 
{
  if(!s1 || !s2) return 0;
  if(s1->v1 == s2->v1) {
    *v1 = TOPOROUTER_VERTEX(s1->v2);
    *v2 = TOPOROUTER_VERTEX(s2->v2);
    return 1;
  }
  if(s1->v2 == s2->v1) {
    *v1 = TOPOROUTER_VERTEX(s1->v1);
    *v2 = TOPOROUTER_VERTEX(s2->v2);
    return 1;
  }
  if(s1->v1 == s2->v2) { 
    *v1 = TOPOROUTER_VERTEX(s1->v2);
    *v2 = TOPOROUTER_VERTEX(s2->v1);
    return 1;
  }
  if(s1->v2 == s2->v2) { 
    *v1 = TOPOROUTER_VERTEX(s1->v1);
    *v2 = TOPOROUTER_VERTEX(s2->v1);
    return 1;
  }
  return 0;
}

/*!
 * \brief Returns the flow from e1 to e2, and the flow from the vertex
 * oppisate e1 to e1 and the vertex oppisate e2 to e2.
 */
gdouble
flow_from_edge_to_edge(GtsTriangle *t, toporouter_edge_t *e1, toporouter_edge_t *e2, 
    toporouter_vertex_t *common_v, toporouter_vertex_t *curpoint)
{
  gdouble r = 0.;
  toporouter_vertex_t *pv = common_v, *v;
  toporouter_edge_t *op_edge;
  
  GList *i = edge_routing(e1);
  while(i) {
    v = TOPOROUTER_VERTEX(i->data);

    if(v == curpoint) {
      r += min_spacing(v, pv);
      pv = v;
      i = i->next; continue;
    }
//    if(!(v->flags & VERTEX_FLAG_TEMP)) {
    if((v->flags & VERTEX_FLAG_ROUTE)) {
      if(v->parent)
        if(v->parent->routingedge == e2) {
          r += min_spacing(v, pv);
          pv = v;
          i = i->next; continue;
        }

      if(v->child)
        if(v->child->routingedge == e2) {
          r += min_spacing(v, pv);
          pv = v;
          i = i->next; continue;
        }
    }
    i = i->next;
  }

  op_edge = TOPOROUTER_EDGE(gts_triangle_edge_opposite(t, GTS_VERTEX(common_v)));
  
  g_assert(op_edge);
  g_assert(e1);
  g_assert(e2);

  v = segment_common_vertex(GTS_SEGMENT(e2), GTS_SEGMENT(op_edge)); 
  g_assert(v);

  //v = TOPOROUTER_VERTEX(gts_triangle_vertex_opposite(t, GTS_EDGE(e1)));
  if(v->flags & VERTEX_FLAG_ROUTE && v->parent && v->parent->routingedge) {
    if(v->parent->routingedge == e1) 
      r += min_spacing(v, pv);
  }

  v = segment_common_vertex(GTS_SEGMENT(e1), GTS_SEGMENT(op_edge)); 
  g_assert(v);

  //v = TOPOROUTER_VERTEX(gts_triangle_vertex_opposite(t, GTS_EDGE(e2)));
  if(v->flags & VERTEX_FLAG_ROUTE && v->parent && v->parent->routingedge) {
    if(v->parent->routingedge == e1) 
      r += min_spacing(v, pv);
  }

  if(TOPOROUTER_IS_CONSTRAINT(op_edge)) {
    toporouter_bbox_t *box = vertex_bbox(TOPOROUTER_VERTEX(edge_v1(op_edge)));
    r += vertex_net_thickness(v) / 2.;
    if(box) {
      r += MAX(vertex_net_keepaway(v), cluster_keepaway(box->cluster));
      r += cluster_thickness(box->cluster) / 2.;
    }else{
      r += vertex_net_keepaway(v);

    }
  }

  return r;
}



guint
check_triangle_interior_capacity(GtsTriangle *t, toporouter_vertex_t *v, toporouter_vertex_t *curpoint, 
    toporouter_edge_t *op_edge, toporouter_edge_t *adj_edge1, toporouter_edge_t *adj_edge2)
{
  gdouble ic = triangle_interior_capacity(t, v);
  gdouble flow = flow_from_edge_to_edge(t, adj_edge1, adj_edge2, v, curpoint);

  if(TOPOROUTER_IS_CONSTRAINT(adj_edge1) || TOPOROUTER_IS_CONSTRAINT(adj_edge2)) return 1;


  if(flow > ic) {
#ifdef DEBUG_ROUTE    
    printf("fail interior capacity flow = %f ic = %f\n", flow, ic);
#endif    
    return 0;
  }

  return 1;
}

toporouter_vertex_t *
edge_routing_next_not_temp(toporouter_edge_t *e, GList *list) 
{
  if(!TOPOROUTER_IS_CONSTRAINT(e)) {
    while(list) {
      toporouter_vertex_t *v = TOPOROUTER_VERTEX(list->data);
      if(!(v->flags & VERTEX_FLAG_TEMP))
        return v;

      list = list->next;
    }
  }
  return tedge_v2(e);
}

toporouter_vertex_t *
edge_routing_prev_not_temp(toporouter_edge_t *e, GList *list) 
{
  if(!TOPOROUTER_IS_CONSTRAINT(e)) {
    while(list) {
      toporouter_vertex_t *v = TOPOROUTER_VERTEX(list->data);
      if(!(v->flags & VERTEX_FLAG_TEMP))
        return v;

      list = list->prev;
    }
  }
  return tedge_v1(e);
}

void
edge_adjacent_vertices(toporouter_edge_t *e, toporouter_vertex_t *v, toporouter_vertex_t **v1, toporouter_vertex_t **v2)
{
  GList *r = g_list_find(edge_routing(e), v);

  if(v == tedge_v1(e)) {
    *v1 = NULL;
    *v2 = edge_routing_next_not_temp(e, edge_routing(e));
  }else if(v == tedge_v2(e)) {
    *v1 = edge_routing_prev_not_temp(e, g_list_last(edge_routing(e)));
    *v2 = NULL;
  }else{
//    r = g_list_find(r, v);
    *v1 = edge_routing_prev_not_temp(e, r);
    *v2 = edge_routing_next_not_temp(e, r);

  }

}


GList *
candidate_vertices(toporouter_vertex_t *v1, toporouter_vertex_t *v2, toporouter_vertex_t *dest, toporouter_edge_t *e) 
{
  gdouble totald, v1ms, v2ms, flow, capacity, ms;
  GList *vs = NULL;

  g_assert(v1);
  g_assert(v2);
  g_assert(dest);

  g_assert(!(v1->flags & VERTEX_FLAG_TEMP));
  g_assert(!(v2->flags & VERTEX_FLAG_TEMP));
#ifdef DEBUG_ROUTE
  printf("starting candidate vertices\n");
  printf("v1 = %f,%f v2 = %f,%f dest = %f,%f\n", vx(v1), vy(v1), vx(v2), vy(v2), vx(dest), vy(dest));
#endif
  totald = gts_point_distance(GTS_POINT(v1), GTS_POINT(v2));
  v1ms = min_spacing(v1, dest);
  v2ms = min_spacing(v2, dest); 
  ms = min_spacing(dest, dest);
  flow = TOPOROUTER_IS_CONSTRAINT(e) ? 0. : edge_flow(e, v1, v2, dest);
  capacity = edge_capacity(e);
 
#ifdef DEBUG_ROUTE
  g_assert(totald > 0);
 
  printf("v1ms = %f v2ms = %f totald = %f ms = %f capacity = %f flow = %f\n", v1ms, v2ms, totald, ms, capacity, flow);
#endif

  if(flow >= capacity) return NULL;


  if(v1ms + v2ms + ms >= totald) {
    vs = g_list_prepend(vs, new_temp_toporoutervertex((vx(v1)+vx(v2)) / 2., (vy(v1)+vy(v2)) / 2., e));
  }else{
    gdouble x0, y0, x1, y1, d;

    vertex_move_towards_vertex_values(GTS_VERTEX(v1), GTS_VERTEX(v2), v1ms, &x0, &y0);
    
    vs = g_list_prepend(vs, new_temp_toporoutervertex(x0, y0, e));
    
    vertex_move_towards_vertex_values(GTS_VERTEX(v2), GTS_VERTEX(v1), v2ms, &x1, &y1);
    
    vs = g_list_prepend(vs, new_temp_toporoutervertex(x1, y1, e));
    
    d = hypot(x0-x1,y0-y1);

    if(ms < d) {
//      guint nint = d / ms;
//      gdouble dif = d / (nint + 1);
      gdouble dif = d / 2;

//      for(guint j=0;j<nint;j++) {
        gdouble x, y;

//        coord_move_towards_coord_values(x0, y0, x1, y1, dif * j, &x, &y);
        coord_move_towards_coord_values(x0, y0, x1, y1, dif, &x, &y);

        vs = g_list_prepend(vs, new_temp_toporoutervertex(x, y, e));

//      }

    }

  }
#ifdef DEBUG_ROUTE
  printf("candidate vertices returning %d\n", g_list_length(vs));
#endif
  return vs;
}

GList *
edge_routing_first_not_temp(toporouter_edge_t *e)
{
  GList *i = edge_routing(e);
  toporouter_vertex_t *v;

  while(i) {
    v = TOPOROUTER_VERTEX(i->data);
    if(!(v->flags & VERTEX_FLAG_TEMP)) return i;

    i = i->next;
  }
 
  return NULL;
}

GList *
edge_routing_last_not_temp(toporouter_edge_t *e)
{
  GList *i = edge_routing(e), *last = NULL;
  toporouter_vertex_t *v;

  while(i) {
    v = TOPOROUTER_VERTEX(i->data);
    if(!(v->flags & VERTEX_FLAG_TEMP)) last = i;
    
    i = i->next;
  }
 
  return last;
}

void
delete_vertex(toporouter_vertex_t *v)
{
  
  if(v->flags & VERTEX_FLAG_TEMP) {
    if(v->routingedge) {
      if(TOPOROUTER_IS_CONSTRAINT(v->routingedge)) 
        TOPOROUTER_CONSTRAINT(v->routingedge)->routing = g_list_remove(TOPOROUTER_CONSTRAINT(v->routingedge)->routing, v);
      else
        v->routingedge->routing = g_list_remove(v->routingedge->routing, v);
    }

    gts_object_destroy ( GTS_OBJECT(v) );
  }
}

#define edge_is_blocked(e) (TOPOROUTER_IS_EDGE(e) ? (e->flags & EDGE_FLAG_DIRECTCONNECTION) : 0)

GList *
triangle_candidate_points_from_vertex(GtsTriangle *t, toporouter_vertex_t *v, toporouter_vertex_t *dest, toporouter_route_t *routedata)
{
  toporouter_edge_t *op_e = TOPOROUTER_EDGE(gts_triangle_edge_opposite(t, GTS_VERTEX(v)));
  toporouter_vertex_t *vv1, *vv2, *constraintv = NULL;
  toporouter_edge_t *e1, *e2;
  GList *i;
  GList *rval = NULL;

#ifdef DEBUG_ROUTE  
  printf("\tTRIANGLE CAND POINT FROM VERTEX\n");

  g_assert(op_e);
#endif

  e1 = TOPOROUTER_EDGE(gts_vertices_are_connected(GTS_VERTEX(v), edge_v1(op_e)));
  e2 = TOPOROUTER_EDGE(gts_vertices_are_connected(GTS_VERTEX(v), edge_v2(op_e)));
    

  if(TOPOROUTER_IS_CONSTRAINT(op_e)) {
    if(TOPOROUTER_CONSTRAINT(op_e)->box->type == BOARD) {
#ifdef DEBUG_ROUTE
      printf("BOARD constraint\n");
#endif      
      return NULL;
    }
    if(constraint_netlist(TOPOROUTER_CONSTRAINT(op_e)) != vertex_netlist(dest)) { // || TOPOROUTER_CONSTRAINT(op_e)->routing) {
#ifdef DEBUG_ROUTE
      printf("op_e routing:\n");
      print_edge(op_e);
#endif      
      return NULL;
    }
#ifdef DEBUG_ROUTE
    printf("RETURNING CONSTRAINT POING\n");
#endif
    constraintv = new_temp_toporoutervertex_in_segment(op_e, TOPOROUTER_VERTEX(edge_v1(op_e)), 
        gts_point_distance(GTS_POINT(edge_v1(op_e)), GTS_POINT(edge_v2(op_e))) / 2., TOPOROUTER_VERTEX(edge_v2(op_e)));
//    return g_list_prepend(NULL, vv1);


  }

  if(edge_is_blocked(op_e)) {
    goto triangle_candidate_points_from_vertex_exit;
  }
//  v1 = tedge_v1(op_e); 
//  v2 = tedge_v2(op_e);
 
  if(v == tedge_v1(e1)) {
    i = edge_routing_first_not_temp(e1);
  }else{
    i = edge_routing_last_not_temp(e1);
  }

  if(i) {
    toporouter_vertex_t *temp = TOPOROUTER_VERTEX(i->data);

    if(temp->parent == tedge_v2(op_e) || temp->child == tedge_v2(op_e)) {
 #ifdef DEBUG_ROUTE
     printf("temp -> op_e->v2\n");
#endif
      goto triangle_candidate_points_from_vertex_exit;
    }
    if(temp->parent->routingedge == op_e) {
      vv1 = temp->parent;  
#ifdef DEBUG_ROUTE
      printf("vv1->parent\n");
#endif

    }else if(temp->child->routingedge == op_e) {
      vv1 = temp->child;
#ifdef DEBUG_ROUTE
      printf("vv1->child\n");
#endif

    }else{
      // must be to e2
#ifdef DEBUG_ROUTE
      printf("temp -> e2?\n");
      printf("op_e = %f,%f\t\t%f,%f\n", vx(edge_v1(op_e)), vy(edge_v1(op_e)), vx(edge_v2(op_e)), vy(edge_v2(op_e)) );
      if(temp->parent->routingedge)
        printf("temp->parent->routingedge = %f,%f \t\t %f,%f\n", 
            vx(edge_v1(temp->parent->routingedge)), vy(edge_v1(temp->parent->routingedge)),
            vx(edge_v2(temp->parent->routingedge)), vy(edge_v2(temp->parent->routingedge))
            );
      else 
        printf("temp->parent->routingedge = NULL\n");

      if(temp->child->routingedge)
        printf("temp->child->routingedge = %f,%f \t\t %f,%f\n", 
            vx(edge_v1(temp->child->routingedge)), vy(edge_v1(temp->child->routingedge)),
            vx(edge_v2(temp->child->routingedge)), vy(edge_v2(temp->child->routingedge))
            );
      else 
        printf("temp->child->routingedge = NULL\n");
#endif
      goto triangle_candidate_points_from_vertex_exit;
    }

  }else{
    vv1 = tedge_v1(op_e);
#ifdef DEBUG_ROUTE
    printf("nothing on e1\n");
#endif
  }
  
  if(v == tedge_v1(e2)) {
    i = edge_routing_first_not_temp(e2);
  }else{
    i = edge_routing_last_not_temp(e2);
  }

  if(i) {
    toporouter_vertex_t *temp = TOPOROUTER_VERTEX(i->data);

    if(temp->parent == tedge_v1(op_e) || temp->child == tedge_v1(op_e)) {
#ifdef DEBUG_ROUTE
      printf("temp -> op_e->v2\n");
#endif
      goto triangle_candidate_points_from_vertex_exit;
    }

    if(temp->parent->routingedge == op_e) {
      vv2 = temp->parent;  
#ifdef DEBUG_ROUTE
      printf("vv2->parent\n");
#endif
    }else if(temp->child->routingedge == op_e) {
      vv2 = temp->child;
#ifdef DEBUG_ROUTE
      printf("vv2->child\n");
#endif

    }else{
      // must be to e1
#ifdef DEBUG_ROUTE
      printf("temp -> e1?\n");
      printf("op_e = %f,%f\t\t%f,%f\n", vx(edge_v1(op_e)), vy(edge_v1(op_e)), vx(edge_v2(op_e)), vy(edge_v2(op_e)) );
      if(temp->parent->routingedge)
        printf("temp->parent->routingedge = %f,%f \t\t %f,%f\n", 
            vx(edge_v1(temp->parent->routingedge)), vy(edge_v1(temp->parent->routingedge)),
            vx(edge_v2(temp->parent->routingedge)), vy(edge_v2(temp->parent->routingedge))
            );
      else 
        printf("temp->parent->routingedge = NULL\n");

      if(temp->child->routingedge)
        printf("temp->child->routingedge = %f,%f \t\t %f,%f\n", 
            vx(edge_v1(temp->child->routingedge)), vy(edge_v1(temp->child->routingedge)),
            vx(edge_v2(temp->child->routingedge)), vy(edge_v2(temp->child->routingedge))
            );
      else 
        printf("temp->child->routingedge = NULL\n");
#endif
      goto triangle_candidate_points_from_vertex_exit;
    }

  }else{
    vv2 = tedge_v2(op_e);
#ifdef DEBUG_ROUTE
    printf("nothing on e2\n");
#endif
  }

#ifdef DEBUG_ROUTE
    printf("size of e1 routing = %d e2 routing = %d op_e routing = %d\n", 
        g_list_length(edge_routing(e1)), g_list_length(edge_routing(e2)), g_list_length(edge_routing(op_e)));
#endif

  if(constraintv) {
#ifdef DEBUG_ROUTE
    print_vertex(constraintv);
    printf("constraintv %f,%f returning\n", vx(constraintv), vy(constraintv));
#endif
    return g_list_prepend(NULL, constraintv);  
  }

  i = edge_routing(op_e);
  while(i) {
    toporouter_vertex_t *temp = TOPOROUTER_VERTEX(i->data);

    if(temp->parent == v || temp->child == v) {
      rval = g_list_concat(rval, candidate_vertices(vv1, temp, dest, op_e)); 
      vv1 = temp;
    }

    i = i->next;
  }

  rval = g_list_concat(rval, candidate_vertices(vv1, vv2, dest, op_e)); 

  return rval; 



triangle_candidate_points_from_vertex_exit:
  if(constraintv) //delete_vertex(constraintv);    
  g_hash_table_insert(routedata->alltemppoints, constraintv, constraintv);  

  g_list_free(rval);

  return NULL;
}

void
routedata_insert_temppoints(toporouter_route_t *data, GList *temppoints) {
  GList *j = temppoints;
  while(j) {
    g_hash_table_insert(data->alltemppoints, j->data, j->data);  
    j = j->next;
  }
}


static inline gint
constraint_route_test(toporouter_constraint_t *c, toporouter_route_t *routedata)
{
  if(c->box->cluster && c->box->cluster->netlist == routedata->src->netlist) {
    if(c->box->cluster->c == routedata->dest->c || c->box->cluster->c == routedata->src->c) return 1;
  }
  return 0;
}

GList *
all_candidates_on_edge(toporouter_edge_t *e, toporouter_route_t *routedata)
{
  GList *rval = NULL;
  if(edge_is_blocked(e)) return NULL;
  
  if(!TOPOROUTER_IS_CONSTRAINT(e)) { 
    GList *i = edge_routing(e);
    toporouter_vertex_t *pv = tedge_v1(e);

    while(i) {
      toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);
      if(!(v->flags & VERTEX_FLAG_TEMP)) {
        rval = g_list_concat(rval, candidate_vertices(pv, v, TOPOROUTER_VERTEX(routedata->destvertices->data), e));
        pv = v;
      }
      i = i->next;
    }

    rval = g_list_concat(rval, candidate_vertices(pv, tedge_v2(e), TOPOROUTER_VERTEX(routedata->destvertices->data), e));
  }else if(TOPOROUTER_CONSTRAINT(e)->box->type == BOARD) {
     return NULL; 
  }else if(constraint_route_test(TOPOROUTER_CONSTRAINT(e), routedata)) {
    toporouter_vertex_t *consv = new_temp_toporoutervertex_in_segment(e, tedge_v1(e), tvdistance(tedge_v1(e), tedge_v2(e)) / 2., tedge_v2(e));
    rval = g_list_prepend(rval, consv);
//    g_hash_table_insert(routedata->alltemppoints, consv, consv);  
  }

  return rval;
}

GList *
triangle_all_candidate_points_from_vertex(GtsTriangle *t, toporouter_vertex_t *v, toporouter_route_t *routedata)
{
  toporouter_edge_t *op_e = TOPOROUTER_EDGE(gts_triangle_edge_opposite(t, GTS_VERTEX(v)));
  return all_candidates_on_edge(op_e, routedata);
}

GList *
triangle_all_candidate_points_from_edge(toporouter_t *r, GtsTriangle *t, toporouter_edge_t *e, toporouter_route_t *routedata,
    toporouter_vertex_t **dest, toporouter_vertex_t *curpoint)
{
  toporouter_vertex_t *op_v;
  toporouter_edge_t *e1, *e2;
  GList *i, *rval = NULL, *rval2 = NULL;
  toporouter_vertex_t *boxpoint = NULL;
  guint e1intcap, e2intcap;

  op_v = TOPOROUTER_VERTEX(gts_triangle_vertex_opposite(t, GTS_EDGE(e)));
  
    
  if(vertex_bbox(op_v)) boxpoint = TOPOROUTER_VERTEX(vertex_bbox(op_v)->point);
    
  if(g_list_find(routedata->destvertices, op_v)) {
    rval = g_list_prepend(rval, op_v);
    *dest = op_v;
    return rval;
  }else if(g_list_find(routedata->destvertices, boxpoint)) {
    *dest = boxpoint;
  }else if(g_list_find(routedata->srcvertices, op_v)) {
    rval = g_list_prepend(rval, op_v);
  }

  e1 = TOPOROUTER_EDGE(gts_vertices_are_connected(GTS_VERTEX(op_v), edge_v1(e)));
  e2 = TOPOROUTER_EDGE(gts_vertices_are_connected(GTS_VERTEX(op_v), edge_v2(e)));

  rval = g_list_concat(rval, all_candidates_on_edge(e1, routedata));
  rval = g_list_concat(rval, all_candidates_on_edge(e2, routedata));

  e1intcap = check_triangle_interior_capacity(t, tedge_v1(e), curpoint, e2, e, e1);
  e2intcap = check_triangle_interior_capacity(t, tedge_v2(e), curpoint, e1, e, e2);

  i = rval;
  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);

    if(!v->routingedge) 
      rval2 = g_list_prepend(rval2, v);
    else if(v->routingedge == e1 && !(!TOPOROUTER_IS_CONSTRAINT(e1) && !e1intcap))
      rval2 = g_list_prepend(rval2, v);
    else if(v->routingedge == e2 && !(!TOPOROUTER_IS_CONSTRAINT(e2) && !e2intcap))
      rval2 = g_list_prepend(rval2, v);

    i = i->next;
  }
  g_list_free(rval);

  return rval2;
}

GList *
triangle_candidate_points_from_edge(toporouter_t *r, GtsTriangle *t, toporouter_edge_t *e, toporouter_vertex_t *v, toporouter_vertex_t **dest,
    toporouter_route_t *routedata)
{
  toporouter_vertex_t *v1, *v2, *op_v, *vv = NULL, *e1constraintv = NULL, *e2constraintv = NULL;
  toporouter_edge_t *e1, *e2;
  GList *e1cands = NULL, *e2cands = NULL, *rval = NULL;
  guint noe1 = 0, noe2 = 0;

  op_v = TOPOROUTER_VERTEX(gts_triangle_vertex_opposite(t, GTS_EDGE(e)));
  
  e1 = TOPOROUTER_EDGE(gts_vertices_are_connected(GTS_VERTEX(op_v), edge_v1(e)));
  e2 = TOPOROUTER_EDGE(gts_vertices_are_connected(GTS_VERTEX(op_v), edge_v2(e)));

  g_assert(*dest);

  // v1 is prev dir, v2 is next dir
  edge_adjacent_vertices(e, v, &v1, &v2);
  
  if(TOPOROUTER_IS_CONSTRAINT(e1)) {
    GList *i = edge_routing(e1);

    if(TOPOROUTER_CONSTRAINT(e1)->box->type == BOARD) {
      noe1 = 1;
    }else if(!constraint_route_test(TOPOROUTER_CONSTRAINT(e1), routedata)) {
      noe1 = 1; 
#ifdef DEBUG_ROUTE      
      printf("noe1 netlist\n");
#endif      
    }else

    if(v1 == tedge_v1(e) || 
        (v1->parent->routingedge && v1->parent->routingedge == e1) || 
        (v1->child->routingedge && v1->child->routingedge == e1)) {
      e1constraintv = new_temp_toporoutervertex_in_segment(e1, tedge_v1(e1), gts_point_distance(GTS_POINT(edge_v1(e1)), GTS_POINT(edge_v2(e1))) / 2., tedge_v2(e1));
    }
    
    while(i) {
      toporouter_vertex_t *temp = TOPOROUTER_VERTEX(i->data);

      if((temp->child == tedge_v2(e) || temp->parent == tedge_v2(e)) && !(temp->flags & VERTEX_FLAG_TEMP)) noe2 = 1;

      i = i->next;
    }

    goto triangle_candidate_points_e2;
  }

  if(edge_is_blocked(e1)) {
    noe1 = 1;
    goto triangle_candidate_points_e2;
  }

  if(v1 == tedge_v1(e)) {
    // continue up e1
    toporouter_vertex_t *vv1, *vv2;
    edge_adjacent_vertices(e1, v1, &vv1, &vv2);

#ifdef DEBUG_ROUTE      
    printf("v1 == e->v1\n");
#endif

    if(vv1) {
      // candidates from v1 until vv1 
      vv = vv1;
    }else{
      // candidates from v1 until vv2
      vv = vv2;
    }
    
    if(!e1constraintv) e1cands = candidate_vertices(v1, vv, *dest, e1);

    if(vv != op_v) {
      if(vv->parent == tedge_v2(e) || vv->child == tedge_v2(e)) {
#ifdef DEBUG_ROUTE      
        printf("noe2 0\n");
#endif
        noe2 = 1;
      }
    }

  }else if(v1->parent != op_v && v1->child != op_v) {
    toporouter_vertex_t *vv1 = NULL, *vv2 = NULL;
    
#ifdef DEBUG_ROUTE      
    printf("v1 != e->v1\n");
#endif

    if(v1->parent->routingedge == e1) {
      vv1 = v1->parent;
#ifdef DEBUG_ROUTE      
      printf("v1 parent = e1\n");
#endif
      if(op_v == tedge_v1(e1)) {
        // candidates from v1->parent until prev vertex 
        vv2 = edge_routing_prev_not_temp(e1, g_list_find(edge_routing(e1), v1->parent)->prev);
      }else{
        // candidates from v1->parent until next vertex 
        vv2 = edge_routing_next_not_temp(e1, g_list_find(edge_routing(e1), v1->parent)->next);
      }

    }else if(v1->child->routingedge == e1) {
      vv1 = v1->child;
#ifdef DEBUG_ROUTE      
      printf("v1 child = e1\n");
#endif
      if(op_v == tedge_v1(e1)) {
        // candidates from v1->child until prev vertex 
        vv2 = edge_routing_prev_not_temp(e1, g_list_find(edge_routing(e1), v1->child)->prev);
      }else{
        // candidates from v1->child until next vertex 
        vv2 = edge_routing_next_not_temp(e1, g_list_find(edge_routing(e1), v1->child)->next);
      }

    }else{
#ifdef DEBUG_ROUTE      
      printf("v1 ? \n");
#endif
      goto triangle_candidate_points_e2;
    }

    if(vv1 && vv2) {
      if(vv2->parent == tedge_v2(e) || vv2->child == tedge_v2(e)) {
#ifdef DEBUG_ROUTE      
        printf("noe2 1\n");
#endif
        noe2 = 1;
      }

      if(!e1constraintv) e1cands = candidate_vertices(vv1, vv2, *dest, e1);

      vv = vv2;
    }
  }
  
  if(vv && vv == op_v) {
    toporouter_vertex_t *boxpoint = NULL;
    
    if(vertex_bbox(op_v)) boxpoint = TOPOROUTER_VERTEX(vertex_bbox(op_v)->point);
    
    if(g_list_find(routedata->destvertices, op_v)) {
      rval = g_list_prepend(rval, op_v);
      *dest = op_v;
    }else if(g_list_find(routedata->destvertices, boxpoint)) {
      *dest = boxpoint;
    }else if(g_list_find(routedata->srcvertices, op_v)) {
      rval = g_list_prepend(rval, op_v);
    }
  }

triangle_candidate_points_e2:

  if(noe2) {
//    printf("noe2\n");
    goto triangle_candidate_points_finish;
  }
  
  if(TOPOROUTER_IS_CONSTRAINT(e2)) {
    GList *i = edge_routing(e2);

    if(TOPOROUTER_CONSTRAINT(e2)->box->type == BOARD) {
      noe2 = 1;
//      goto triangle_candidate_points_finish;
    }else if(!constraint_route_test(TOPOROUTER_CONSTRAINT(e2), routedata)) {
#ifdef DEBUG_ROUTE      
      printf("noe2 netlist\n");
#endif
      noe2 = 1;
//      goto triangle_candidate_points_finish;
    }else if(v2 == tedge_v2(e) ||
        (v2->parent->routingedge && v2->parent->routingedge == e2) || 
        (v2->child->routingedge && v2->child->routingedge == e2)) {
       
      e2constraintv = new_temp_toporoutervertex_in_segment(e2, tedge_v1(e2), gts_point_distance(GTS_POINT(edge_v1(e2)), GTS_POINT(edge_v2(e2))) / 2., tedge_v2(e2));
      
    }
    
    while(i) {
      toporouter_vertex_t *temp = TOPOROUTER_VERTEX(i->data);

      if((temp->child == tedge_v1(e) || temp->parent == tedge_v1(e)) && !(temp->flags & VERTEX_FLAG_TEMP)) 
        noe1 = 1;

      i = i->next;
    }



    goto triangle_candidate_points_finish;
  }
  
  if(edge_is_blocked(e2)) {
    noe2 = 1;
    goto triangle_candidate_points_finish;
  }
  
  if(v2 == tedge_v2(e)) {
    // continue up e2
    toporouter_vertex_t *vv1 = NULL, *vv2 = NULL;
    edge_adjacent_vertices(e2, v2, &vv1, &vv2);

#ifdef DEBUG_ROUTE      
    printf("v2 == e->v2\n");
#endif

    if(vv1) {
      // candidates from v2 until vv1
      vv = vv1;
    }else{
      // candidates from v2 until vv2
      vv = vv2;
    }
    
    if(!e2constraintv) e2cands = candidate_vertices(v2, vv, *dest, e2);

    if(vv != op_v) {
      if(vv->parent == tedge_v1(e) || vv->child == tedge_v1(e)) {
#ifdef DEBUG_ROUTE      
        printf("noe1 0\n");
#endif
        noe1 = 1;
      }
    }

  }else if(v2->parent != op_v && v2->child != op_v) {
    toporouter_vertex_t *vv1 = NULL, *vv2 = NULL;
    
#ifdef DEBUG_ROUTE      
    printf("v2 == e->v2\n");
#endif

    if(v2->parent->routingedge == e2) {
      vv1 = v2->parent;
      if(op_v == tedge_v1(e2)) {
        // candidates from v2->parent until prev vertex 
        vv2 = edge_routing_prev_not_temp(e2, g_list_find(edge_routing(e2), vv1)->prev);
      }else{
        // candidates from v2->parent until next vertex 
        vv2 = edge_routing_next_not_temp(e2, g_list_find(edge_routing(e2), vv1)->next);
      }

    }else if(v2->child->routingedge == e2) {
      vv1 = v2->child;
      if(op_v == tedge_v1(e2)) {
        // candidates from v2->child until prev vertex 
        vv2 = edge_routing_prev_not_temp(e2, g_list_find(edge_routing(e2), vv1)->prev);
      }else{
        // candidates from v2->child until next vertex 
        vv2 = edge_routing_next_not_temp(e2, g_list_find(edge_routing(e2), vv1)->next);
      }

    }else{
      goto triangle_candidate_points_finish;
    }

    if(vv1 && vv2) {
      if(vv2->parent == tedge_v1(e) || vv2->child == tedge_v1(e)) {
#ifdef DEBUG_ROUTE      
        printf("noe1 1\n");
#endif
        noe1 = 1;
      }

      if(!e2constraintv) e2cands = candidate_vertices(vv1, vv2, *dest, e2);
    }
  }

triangle_candidate_points_finish:  

  v1 = segment_common_vertex(GTS_SEGMENT(e), GTS_SEGMENT(e1));
  v2 = segment_common_vertex(GTS_SEGMENT(e), GTS_SEGMENT(e2));

  if(noe1 || !check_triangle_interior_capacity(t, v1, v, e2, e, e1)) {
#ifdef DEBUG_ROUTE      
    printf("freeing e1cands\n");
#endif
    routedata_insert_temppoints(routedata, e1cands);
    g_list_free(e1cands);
    e1cands = NULL;
  }
  
  if(noe2 || !check_triangle_interior_capacity(t, v2, v, e1, e, e2)) {
#ifdef DEBUG_ROUTE      
    printf("freeing e2cands\n");
#endif
    routedata_insert_temppoints(routedata, e2cands);
    g_list_free(e2cands);
    e2cands = NULL;
  }

  if(!noe1 && e1constraintv) {
    e1cands = g_list_prepend(e1cands, e1constraintv);
  }else if(e1constraintv) {
    g_hash_table_insert(routedata->alltemppoints, e1constraintv, e1constraintv);  
//    delete_vertex(e1constraintv);
  }

  if(!noe2 && e2constraintv) {
    e2cands = g_list_prepend(e2cands, e2constraintv);
  }else if(e2constraintv) {
    g_hash_table_insert(routedata->alltemppoints, e2constraintv, e2constraintv);  
//    delete_vertex(e2constraintv);
  }
  
  if(!noe1 && !noe2) return g_list_concat(rval, g_list_concat(e1cands, e2cands));

  return g_list_concat(e1cands, e2cands);
}

GList *
compute_candidate_points(toporouter_t *tr, toporouter_layer_t *l, toporouter_vertex_t *curpoint, toporouter_route_t *data,
    toporouter_vertex_t **closestdest) 
{
  GList *r = NULL, *j;
  toporouter_edge_t *edge = curpoint->routingedge, *tempedge;
  
  if(vertex_keepout_test(tr, curpoint)) goto compute_candidate_points_finish;
 
  /* direct connection */
//  if(curpoint == TOPOROUTER_VERTEX(data->src->point))
  if((tempedge = TOPOROUTER_EDGE(gts_vertices_are_connected(GTS_VERTEX(curpoint), GTS_VERTEX(*closestdest))))) {
    
    if(TOPOROUTER_IS_CONSTRAINT(tempedge)) {
      goto compute_candidate_points_finish;
    }else{
      if(!tempedge->routing) {
        r = g_list_prepend(NULL, *closestdest);
        tempedge->flags |= EDGE_FLAG_DIRECTCONNECTION;
        goto compute_candidate_points_finish;
      }else{
#ifdef DEBUG_ROUTE    
        printf("Direct connection, but has routing\n");
#endif    
      }

    }
    /* if we get to here, there is routing blocking the direct connection,
     * continue as per normal */
  } 
  
  /* a real point origin */
  if(!(curpoint->flags & VERTEX_FLAG_TEMP)) {
    GSList *triangles, *i;
    i = triangles = gts_vertex_triangles(GTS_VERTEX(curpoint), NULL);
#ifdef DEBUG_ROUTE    
    printf("triangle count = %d\n", g_slist_length(triangles));
#endif    
    while(i) {
      GtsTriangle *t = GTS_TRIANGLE(i->data);
      GList *temppoints;

      if(tr->flags & TOPOROUTER_FLAG_LEASTINVALID) temppoints = triangle_all_candidate_points_from_vertex(t, curpoint, data);
      else temppoints = triangle_candidate_points_from_vertex(t, curpoint, *closestdest, data);

#ifdef DEBUG_ROUTE     
      printf("\treturned %d points\n", g_list_length(temppoints));
#endif      
      routedata_insert_temppoints(data, temppoints);

      r = g_list_concat(r, temppoints);
      i = i->next;
    }
    g_slist_free(triangles);
  }else /* a temp point */ {
    int prevwind = vertex_wind(GTS_SEGMENT(edge)->v1, GTS_SEGMENT(edge)->v2, GTS_VERTEX(curpoint->parent));
//    printf("tempoint\n");
    
    GSList *i = GTS_EDGE(edge)->triangles;

    while(i) {
      GtsVertex *oppv =  gts_triangle_vertex_opposite(GTS_TRIANGLE(i->data), GTS_EDGE(edge));
      if(prevwind != vertex_wind(GTS_SEGMENT(edge)->v1, GTS_SEGMENT(edge)->v2, oppv)) {
        GList *temppoints;
        
        if(tr->flags & TOPOROUTER_FLAG_LEASTINVALID) temppoints = triangle_all_candidate_points_from_edge(tr, GTS_TRIANGLE(i->data), edge,
            data, closestdest, curpoint);
        else temppoints = triangle_candidate_points_from_edge(tr, GTS_TRIANGLE(i->data), edge, curpoint, closestdest, data);

        j = temppoints;
        while(j) {
          toporouter_vertex_t *tempj = TOPOROUTER_VERTEX(j->data);
          if(tempj->flags & VERTEX_FLAG_TEMP) 
            g_hash_table_insert(data->alltemppoints, j->data, j->data); 
#ifdef DEBUG_ROUTE          
          else 
            printf("got cand not a temp\n");
#endif
          j = j->next;
        }
        r = g_list_concat(r, temppoints);
        
        break;
      }
      i = i->next;
    }
  }

compute_candidate_points_finish:

  if(vertex_bbox(curpoint) && vertex_bbox(curpoint)->cluster) {
    if(vertex_bbox(curpoint)->cluster->c == data->src->c) {
      r = g_list_concat(r, g_list_copy(data->srcvertices));
    }
  }

  return r;
}

gboolean 
temp_point_clean(gpointer key, gpointer value, gpointer user_data)
{
  toporouter_vertex_t *tv = TOPOROUTER_VERTEX(value);
  if(tv->flags & VERTEX_FLAG_TEMP) {
    if(TOPOROUTER_IS_CONSTRAINT(tv->routingedge)) 
      TOPOROUTER_CONSTRAINT(tv->routingedge)->routing = g_list_remove(TOPOROUTER_CONSTRAINT(tv->routingedge)->routing, tv);
    else
      tv->routingedge->routing = g_list_remove(tv->routingedge->routing, tv);
    gts_object_destroy ( GTS_OBJECT(tv) );
  }
  return TRUE;
}

void
clean_routing_edges(toporouter_t *r, toporouter_route_t *data)
{
  g_hash_table_foreach_remove(data->alltemppoints, temp_point_clean, NULL);
  g_hash_table_destroy(data->alltemppoints);  
  data->alltemppoints = NULL;
}

gdouble
path_score(toporouter_t *r, GList *path)
{
  gdouble score = 0.;
  toporouter_vertex_t *pv = NULL;
  toporouter_vertex_t *v0 = NULL;

  if(!path) return INFINITY;

  v0 = TOPOROUTER_VERTEX(path->data);

  while(path) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(path->data);

    if(pv) {
      score += gts_point_distance(GTS_POINT(pv), GTS_POINT(v));
      if(pv != v0 && vz(pv) != vz(v)) 
        if(path->next)
          score += r->viacost;

    }

    pv = v;
    path = path->next;
  }

  return score;
}

void
print_vertices(GList *vertices)
{
  while(vertices) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(vertices->data);
    print_vertex(v);
    print_bbox(vertex_bbox(v));
    if(vertex_bbox(v)) {
      printf("has bbox\n");
      if(vertex_bbox(v)->cluster) 
        printf("has cluster\n");
      else
        printf("no cluster\n");
    }else printf("no bbox\n");
    vertices = vertices->next;
  }
}

gint       
space_edge(gpointer item, gpointer data)
{
  toporouter_edge_t *e = TOPOROUTER_EDGE(item);
  GList *i;
  gdouble *forces;

  if(TOPOROUTER_IS_CONSTRAINT(e)) return 0;

  if(!edge_routing(e) || !g_list_length(edge_routing(e))) return 0;

  forces = (gdouble *)malloc(sizeof(double) * g_list_length(edge_routing(e)));
  
  for(guint j=0;j<100;j++) {
    guint k=0;
    guint equilibrium = 1;

    i = edge_routing(e);
    while(i) {
      toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data); 
      gdouble ms, d;

      if(i->prev) {
//        ms = min_net_net_spacing(TOPOROUTER_VERTEX(i->prev->data), v);
        ms = min_spacing(TOPOROUTER_VERTEX(i->prev->data), v);
        d = gts_point_distance(GTS_POINT(i->prev->data), GTS_POINT(v));
      }else{
//        ms = min_vertex_net_spacing(v, tedge_v1(e));
        ms = min_spacing(v, tedge_v1(e));
        d = gts_point_distance(GTS_POINT(edge_v1(e)), GTS_POINT(v));
      }

      if(d < ms) forces[k] = ms - d;
      else forces[k] = 0.;

      if(i->next) {
//        ms = min_net_net_spacing(TOPOROUTER_VERTEX(i->next->data), v);
        ms = min_spacing(TOPOROUTER_VERTEX(i->next->data), v);
        d = gts_point_distance(GTS_POINT(i->next->data), GTS_POINT(v));
      }else{
//        ms = min_vertex_net_spacing(v, tedge_v2(e));
        ms = min_spacing(v, tedge_v2(e));
        d = gts_point_distance(GTS_POINT(edge_v2(e)), GTS_POINT(v));
      }

      if(d < ms) forces[k] += d - ms;

      k++; i = i->next;
    }
    
    k = 0;
    i = edge_routing(e);
    while(i) {
      toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data); 
      if(forces[k] > EPSILON || forces[k] < -EPSILON) equilibrium = 0;
      vertex_move_towards_vertex_values(GTS_VERTEX(v), edge_v2(e), forces[k] * 0.1, &(GTS_POINT(v)->x), &(GTS_POINT(v)->y));
      k++; i = i->next;
    }

    if(equilibrium) {
//      printf("reached equilibriium at %d\n", j);
      break;
    }

  }

  free(forces);
  return 0;  
}

void
swap_vertices(toporouter_vertex_t **v1, toporouter_vertex_t **v2)
{
  toporouter_vertex_t *tempv = *v1;
  *v1 = *v2;
  *v2 = tempv;
}

void
split_edge_routing(toporouter_vertex_t *v, GList **l1, GList **l2)
{
  GList *base, *i;

  g_assert(v);
  g_assert(v->routingedge);
  
  base = g_list_find(vrouting(v), v);

  *l1 = g_list_prepend(*l1, tedge_v1(v->routingedge));
  *l2 = g_list_prepend(*l2, tedge_v2(v->routingedge));
  
  g_assert(base);

  i = base->next;
  while(i) {
    if(!(TOPOROUTER_VERTEX(i->data)->flags & VERTEX_FLAG_TEMP)) *l2 = g_list_prepend(*l2, i->data);
    i = i->next;
  }

  i = base->prev;
  while(i) {
    if(!(TOPOROUTER_VERTEX(i->data)->flags & VERTEX_FLAG_TEMP)) *l1 = g_list_prepend(*l1, i->data);
    i = i->prev;
  }
}

GList *
vertices_routing_conflicts(toporouter_vertex_t *v, toporouter_vertex_t *pv)
{
  toporouter_edge_t *e;
  GList *rval = NULL, *l1 = NULL, *l2 = NULL, *i;

  if(vz(v) != vz(pv)) return NULL;
  g_assert(v != pv);

  if(!v->routingedge && !pv->routingedge) {
    e = TOPOROUTER_EDGE(gts_vertices_are_connected(GTS_VERTEX(v), GTS_VERTEX(pv)));
    if(!e) return NULL;
    i = edge_routing(e);
    while(i) {
      rval = g_list_prepend(rval, TOPOROUTER_VERTEX(i->data)->route);
      i = i->next;
    }
    return rval;
  }

  if(TOPOROUTER_IS_CONSTRAINT(v->routingedge) && TOPOROUTER_IS_CONSTRAINT(pv->routingedge))
    return NULL;

  if(TOPOROUTER_IS_CONSTRAINT(pv->routingedge)) swap_vertices(&pv, &v);

  if(!v->routingedge) swap_vertices(&pv, &v);

  e = v->routingedge;

  split_edge_routing(v, &l1, &l2);
  g_assert(l2);
  g_assert(l1);

  if(!pv->routingedge) {
    toporouter_edge_t *e1, *e2;
    e1 = TOPOROUTER_EDGE(gts_vertices_are_connected(GTS_VERTEX(pv), edge_v1(e)));
    e2 = TOPOROUTER_EDGE(gts_vertices_are_connected(GTS_VERTEX(pv), edge_v2(e)));
   
    l1 = g_list_concat(l1, g_list_copy(edge_routing(e1)));
    l2 = g_list_concat(l2, g_list_copy(edge_routing(e2)));

  }else{
    GList *pvlist1 = NULL, *pvlist2 = NULL;
    toporouter_vertex_t *commonv = route_vertices_common_vertex(v, pv);

    g_assert(commonv);

    split_edge_routing(pv, &pvlist1, &pvlist2);

    if(commonv == tedge_v1(e)) {
      toporouter_edge_t *ope;

      if(commonv == tedge_v1(pv->routingedge)) {
        l1 = g_list_concat(l1, pvlist1);
        l2 = g_list_concat(l2, pvlist2);
        ope = TOPOROUTER_EDGE(gts_vertices_are_connected(edge_v2(e), edge_v2(pv->routingedge)));
      }else{
        l1 = g_list_concat(l1, pvlist2);
        l2 = g_list_concat(l2, pvlist1);
        ope = TOPOROUTER_EDGE(gts_vertices_are_connected(edge_v2(e), edge_v1(pv->routingedge)));
      }
      g_assert(ope);
      l2 = g_list_concat(l2, g_list_copy(edge_routing(ope)));

    }else{
      toporouter_edge_t *ope;
      if(commonv == tedge_v1(pv->routingedge)) {
        l1 = g_list_concat(l1, pvlist2);
        l2 = g_list_concat(l2, pvlist1);
        ope = TOPOROUTER_EDGE(gts_vertices_are_connected(edge_v1(e), edge_v2(pv->routingedge)));
      }else{
        l1 = g_list_concat(l1, pvlist1);
        l2 = g_list_concat(l2, pvlist2);
        ope = TOPOROUTER_EDGE(gts_vertices_are_connected(edge_v1(e), edge_v1(pv->routingedge)));
      }
      g_assert(ope);
      l1 = g_list_concat(l1, g_list_copy(edge_routing(ope)));
    }
  }

  i = l1;
  while(i) {
    toporouter_vertex_t *curv = TOPOROUTER_VERTEX(i->data);

    if(curv->flags & VERTEX_FLAG_ROUTE && (g_list_find(l2, curv->parent) || g_list_find(l2, curv->child))) {
      if(!g_list_find(rval, curv->route)) rval = g_list_prepend(rval, curv->route);
    }
    i = i->next;
  }
  i = l2;
  while(i) {
    toporouter_vertex_t *curv = TOPOROUTER_VERTEX(i->data);

    if(curv->flags & VERTEX_FLAG_ROUTE && (g_list_find(l1, curv->parent) || g_list_find(l1, curv->child))) {
      if(!g_list_find(rval, curv->route)) rval = g_list_prepend(rval, curv->route);
    }
    i = i->next;
  }

  g_list_free(l1);
  g_list_free(l2);

  return rval;
}

gdouble
vertices_routing_conflict_cost(toporouter_t *r, toporouter_vertex_t *v, toporouter_vertex_t *pv, guint *n) 
{
  GList *conflicts = vertices_routing_conflicts(v, pv), *i;
  gdouble penalty = 0.;

  i = conflicts;
  while(i) {
    (*n) += 1;
    penalty += TOPOROUTER_ROUTE(i->data)->score;
    i = i->next;
  }
  g_list_free(conflicts);
//  if(penalty > 0.) printf("conflict penalty of %f with %f,%f %f,%f\n", penalty, vx(v), vy(v), vx(pv), vy(pv));
  return penalty;
}

gdouble
gcost(toporouter_t *r, toporouter_route_t *data, toporouter_vertex_t *srcv, toporouter_vertex_t *v, toporouter_vertex_t *pv, guint *n,
    toporouter_netlist_t *pair)
{
  gdouble cost = 0., segcost;
  
  *n = pv->gn;

  if(g_list_find(data->srcvertices, v)) return 0.; 
  
  segcost = tvdistance(pv, v);

  if(pair && !TOPOROUTER_IS_CONSTRAINT(v->routingedge) && v->routingedge) {
    GList *list = g_list_find(v->routingedge->routing, v);
    toporouter_vertex_t *pv = edge_routing_prev_not_temp(v->routingedge, list); 
    toporouter_vertex_t *nv = edge_routing_next_not_temp(v->routingedge, list); 

    if(pv->route && pv->route->netlist == pair) {
    }else if(nv->route && nv->route->netlist == pair) {
    }else{
      segcost *= 10.;
    }
  }

  cost = pv->gcost + segcost;

  if(r->flags & TOPOROUTER_FLAG_LEASTINVALID) {
    gdouble conflictcost = 0.;
    
    if(pv && v != pv && vz(v) == vz(pv)) conflictcost = vertices_routing_conflict_cost(r, v, pv, n);
    
    if(!(r->flags & TOPOROUTER_FLAG_DETOUR && *n == 1)) {
      cost += conflictcost * (pow(*n,2));
    }
  }

  return cost;
}

#define vlayer(x) (&r->layers[(int)vz(x)])

guint
candidate_is_available(toporouter_vertex_t *pv, toporouter_vertex_t *v)
{
  // TODO: still needed?  
  while(pv) {
    if(pv == v) return 0;
    pv = pv->parent;
  }

  return 1;
}

GList *
route(toporouter_t *r, toporouter_route_t *data, guint debug)
{
  GtsEHeap *openlist = gts_eheap_new(route_heap_cmp, NULL);
  GList *closelist = NULL;
  GList *i, *rval = NULL;
  toporouter_netlist_t *pair = NULL;
  gint count = 0;

  toporouter_vertex_t *srcv = NULL, *destv = NULL, *curpoint = NULL;
  toporouter_layer_t *cur_layer; //, *dest_layer;

  g_assert(data->src->c != data->dest->c);
  
  if(data->destvertices) g_list_free(data->destvertices);
  if(data->srcvertices) g_list_free(data->srcvertices);
  
  data->destvertices = cluster_vertices(r, data->dest);
  data->srcvertices = cluster_vertices(r, data->src);

  closest_cluster_pair(r, data->srcvertices, data->destvertices, &curpoint, &destv);

  if(!curpoint || !destv) goto routing_return;

  srcv = curpoint;
  cur_layer = vlayer(curpoint);
  //dest_layer = vlayer(destv);

  data->path = NULL; 
  
  data->alltemppoints = g_hash_table_new(g_direct_hash, g_direct_equal);

  curpoint->parent = NULL;
  curpoint->child = NULL;
  curpoint->gcost = 0.;
  curpoint->gn = 0;
  curpoint->hcost = simple_h_cost(r, curpoint, destv);
 
  if(data->netlist && data->netlist->pair) {
    GList *i = r->routednets;
    while(i) {
      toporouter_route_t *curroute = TOPOROUTER_ROUTE(i->data);
      if(curroute->netlist == data->netlist->pair) {
        pair = data->netlist->pair;
        break;
      }
      i = i->next;
    }
  }

  gts_eheap_insert(openlist, curpoint);
  
  while(gts_eheap_size(openlist) > 0) {
    GList *candidatepoints;
    data->curpoint = curpoint;
    //draw_route_status(r, closelist, openlist, curpoint, data, count++);

    curpoint = TOPOROUTER_VERTEX( gts_eheap_remove_top(openlist, NULL) );
    if(curpoint->parent && !(curpoint->flags & VERTEX_FLAG_TEMP)) {
      if(vz(curpoint) != vz(destv)) {
        toporouter_vertex_t *tempv;
        cur_layer = vlayer(curpoint);//&r->layers[(int)vz(curpoint)];
        tempv = closest_dest_vertex(r, curpoint, data);
        if(tempv) {
          destv = tempv;
          //dest_layer = vlayer(destv);//&r->layers[(int)vz(destv)];

        }
      }
    }
   
//    destpoint = closest_dest_vertex(r, curpoint, data);
//    dest_layer = &r->layers[(int)vz(destpoint)];
    
    if(g_list_find(data->destvertices, curpoint)) {
      toporouter_vertex_t *temppoint = curpoint;
      srcv = NULL;
      destv = curpoint;

      data->path = NULL;

      while(temppoint) {
        data->path = g_list_prepend(data->path, temppoint);   
        if(g_list_find(data->srcvertices, temppoint)) {
          srcv = temppoint;
          if(r->flags & TOPOROUTER_FLAG_AFTERORDER) break;
        }
        temppoint = temppoint->parent;
      }
      rval = data->path;
      data->score = path_score(r, data->path);
#ifdef DEBUG_ROUTE
      printf("ROUTE: path score = %f computation cost = %d\n", data->score, count);
#endif

      if(srcv->bbox->cluster != data->src) {
        data->src = srcv->bbox->cluster;
      }

      if(destv->bbox->cluster != data->dest) {
        data->dest = destv->bbox->cluster;
      }
      goto route_finish;
    }
    closelist_insert(curpoint);
#ifdef DEBUG_ROUTE
    printf("\n\n\n*** ROUTE COUNT = %d\n", count);
#endif
    candidatepoints = compute_candidate_points(r, cur_layer, curpoint, data, &destv);

//#ifdef DEBUG_ROUTE    
    /*********************
    if(debug && !strcmp(data->dest->netlist, "  unnamed_net2")) 
    {
      unsigned int mask = ~(VERTEX_FLAG_RED | VERTEX_FLAG_GREEN | VERTEX_FLAG_BLUE); 
      char buffer[256];
      int j;

      for(j=0;j<groupcount();j++) {
        i = r->layers[j].vertices;
        while(i) {
          TOPOROUTER_VERTEX(i->data)->flags &= mask;
          i = i->next;
        }
      }
      
      i = candidatepoints;
      while(i) {
        TOPOROUTER_VERTEX(i->data)->flags |= VERTEX_FLAG_GREEN;
//        printf("flagged a candpoint @ %f,%f\n",
//            vx(i->data), vy(i->data));
        i = i->next;
      }
      
      curpoint->flags |= VERTEX_FLAG_BLUE;
      if(curpoint->parent) 
        curpoint->parent->flags |= VERTEX_FLAG_RED;


      for(j=0;j<groupcount();j++) {
        GList *datas = g_list_prepend(NULL, data);
        sprintf(buffer, "route-%d-%05d.png", j, count);
        toporouter_draw_surface(r, r->layers[j].surface, buffer, 1024, 1024, 2, datas, j, candidatepoints);
        g_list_free(datas);
      }
    }
//#endif    
    *********************/
    count++;
//    if(count > 100) exit(0);
    i = candidatepoints;
    while(i) {
      toporouter_vertex_t *temppoint = TOPOROUTER_VERTEX(i->data);
      if(!g_list_find(closelist, temppoint) && candidate_is_available(curpoint, temppoint)) { //&& temppoint != curpoint) {
        toporouter_heap_search_data_t heap_search_data = { temppoint, NULL };

        guint temp_gn;
        gdouble temp_g_cost = gcost(r, data, srcv, temppoint, curpoint, &temp_gn, pair);
      

        gts_eheap_foreach(openlist,toporouter_heap_search, &heap_search_data);

        if(heap_search_data.result) {
          if(temp_g_cost < temppoint->gcost) {
            
            temppoint->gcost = temp_g_cost;
            temppoint->gn = temp_gn; 
            
            temppoint->parent = curpoint;
            curpoint->child = temppoint;
            
            gts_eheap_update(openlist);
          }
        }else{
          temppoint->parent = curpoint;
          curpoint->child = temppoint;
          
          temppoint->gcost = temp_g_cost;
          temppoint->gn = temp_gn;

          temppoint->hcost = simple_h_cost(r, temppoint, destv);
//          if(cur_layer != dest_layer) temppoint->hcost += r->viacost;
          gts_eheap_insert(openlist, temppoint);
        }
      
      }
      i = i->next;
    }
    g_list_free(candidatepoints);

  }
#ifdef DEBUG_ROUTE  
  printf("ROUTE: could not find path!\n");
#endif 

  data->score = INFINITY;
  clean_routing_edges(r, data); 

  data->path = NULL;
  //TOPOROUTER_VERTEX(data->src->point)->parent = NULL;
  //TOPOROUTER_VERTEX(data->src->point)->child  = NULL;
  goto routing_return;

/* 
  {
    int i;
    for(i=0;i<groupcount();i++) {
      char buffer[256];
      sprintf(buffer, "route-error-%d-%d.png", r->routecount, i);
      toporouter_draw_surface(r, r->layers[i].surface, buffer, 1280, 1280, 2, data, i, NULL);
    }
    r->routecount++;
  }
//  exit(0);
*/
route_finish:
//  printf(" * finished a*\n");
/* 
  {
    int i;
    for(i=0;i<groupcount();i++) {
      char buffer[256];
      sprintf(buffer, "route-preclean-%d-%d.png", i, r->routecount);
      toporouter_draw_surface(r, r->layers[i].surface, buffer, 1024, 1024, 2, data, i, NULL);
    }
    r->routecount++;
  }
*/
/*  {
    i = data->path;
    while(i) {
      toporouter_vertex_t *tv = TOPOROUTER_VERTEX(i->data);
      
      if(tv->routingedge) {
        GList *list = g_list_find(edge_routing(tv->routingedge), tv);
        toporouter_vertex_t *restartv = NULL, *boxpoint;

        g_assert(list);

        if(!list->next) {
          if(vertex_bbox(tedge_v2(tv->routingedge))) 
            boxpoint = TOPOROUTER_VERTEX(vertex_bbox(tedge_v2(tv->routingedge))->point);
          else
            boxpoint = NULL;

          if(tedge_v2(tv->routingedge) != srcv && g_list_find(data->srcvertices, tedge_v2(tv->routingedge))) 
            restartv = tedge_v2(tv->routingedge);
          else if(boxpoint != srcv && g_list_find(data->srcvertices, boxpoint)) 
            restartv = boxpoint;
        }
        
        if(!list->prev) {
          if(vertex_bbox(tedge_v1(tv->routingedge))) 
            boxpoint = TOPOROUTER_VERTEX(vertex_bbox(tedge_v1(tv->routingedge))->point);
          else
            boxpoint = NULL;

          if(tedge_v1(tv->routingedge) != srcv && g_list_find(data->srcvertices, tedge_v1(tv->routingedge))) 
            restartv = tedge_v1(tv->routingedge);
          else if(boxpoint != srcv && g_list_find(data->srcvertices, boxpoint)) 
            restartv = boxpoint;
          
        }

        if(restartv) {
          clean_routing_edges(r, data); 
          gts_eheap_destroy(openlist);     
          g_list_free(closelist);
          openlist = gts_eheap_new(route_heap_cmp, NULL);
          closelist = NULL;
          g_list_free(data->path);
          printf("ROUTING RESTARTING with new src %f,%f,%f\n", vx(restartv), vy(restartv), vz(restartv));
          curpoint = restartv;
          goto route_begin;
        }
      }
      
      i = i->next;
    }
  }*/
///*
  {
    toporouter_vertex_t *pv = NULL;
    GList *i = data->path;
    while(i) {
      toporouter_vertex_t *tv = TOPOROUTER_VERTEX(i->data);
      
      if(pv && g_list_find(data->srcvertices, tv)) {
        GList *temp = g_list_copy(i);
        g_list_free(data->path);
        data->path = temp;
        i = data->path;
      }
      pv = tv;
      i = i->next;
    }
  }
//*/
  {
    toporouter_vertex_t *pv = NULL;
    GList *i = data->path;
    while(i) {
      toporouter_vertex_t *tv = TOPOROUTER_VERTEX(i->data);
      if(tv->flags & VERTEX_FLAG_TEMP) { 
        tv->flags ^= VERTEX_FLAG_TEMP;
        tv->flags |= VERTEX_FLAG_ROUTE;
      }
      if(pv) pv->child = tv;

      if(tv->routingedge) tv->route = data;

//      if(tv->routingedge && !TOPOROUTER_IS_CONSTRAINT(tv->routingedge)) space_edge(tv->routingedge, NULL);

      pv = tv;
      i = i->next;
    }
  }
 
  {
    toporouter_vertex_t *pv = NULL, *v = NULL;

    GList *i = data->path;
    while(i) {
      v = TOPOROUTER_VERTEX(i->data);

      if(pv) {
        v->parent = pv;
        pv->child = v;
      }else{
        v->parent = NULL;
      }

      pv = v;
      i = i->next;
    }

    if(v) v->child = NULL;
  }

  clean_routing_edges(r, data); 
// /* 
  {
    GList *i = data->path;
    while(i) {
      toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);

      if(v->routingedge && !TOPOROUTER_IS_CONSTRAINT(v->routingedge))
        space_edge(v->routingedge, NULL);
      i = i->next;
    }
  }
//  */
routing_return:

  g_list_free(data->destvertices);
  g_list_free(data->srcvertices);
  data->destvertices = NULL;
  data->srcvertices = NULL;
  gts_eheap_destroy(openlist);     
  g_list_free(closelist);

  data->alltemppoints = NULL;

  return rval;
}

/* moves vertex v d units in the direction of vertex p */
void
vertex_move_towards_vertex (GtsVertex *v,
                            GtsVertex *p,
                            double d)
{
  double theta = atan2 (GTS_POINT(p)->y - GTS_POINT(v)->y,
                        GTS_POINT(p)->x - GTS_POINT(v)->x);

  GTS_POINT(v)->x += d * cos (theta);
  GTS_POINT(v)->y += d * sin (theta);
}


gdouble
pathvertex_arcing_through_constraint(toporouter_vertex_t *pathv, toporouter_vertex_t *arcv)
{
  toporouter_vertex_t *v = pathv->child;

  if(!v || !v->routingedge) return 0.;

  while(v->flags & VERTEX_FLAG_ROUTE && (tedge_v1(v->routingedge) == arcv || tedge_v2(v->routingedge) == arcv)) {
    if(TOPOROUTER_IS_CONSTRAINT(v->routingedge)) 
      return gts_point_distance(GTS_POINT(tedge_v1(v->routingedge)), GTS_POINT(tedge_v2(v->routingedge)));
    v = v->child;
  }

  v = pathv->parent;
  while(v->flags & VERTEX_FLAG_ROUTE && (tedge_v1(v->routingedge) == arcv || tedge_v2(v->routingedge) == arcv)) {
    if(TOPOROUTER_IS_CONSTRAINT(v->routingedge)) 
      return gts_point_distance(GTS_POINT(tedge_v1(v->routingedge)), GTS_POINT(tedge_v2(v->routingedge)));
    v = v->parent;
  }

  return 0.;
}

gint
vertices_connected(toporouter_vertex_t *a, toporouter_vertex_t *b)
{
  return ((a->route->netlist == b->route->netlist && a->route->src->c == b->route->src->c) ? 1 : 0);
}

gdouble
edge_min_spacing(GList *list, toporouter_edge_t *e, toporouter_vertex_t *v, guint debug) 
{
  toporouter_vertex_t *origin;
  GList *i = list;
  gdouble space = 0.;
  toporouter_vertex_t *nextv, *prevv;
  //toporouter_vertex_t *edgev;
  //gdouble constraint_spacing;

  if(!list) return INFINITY;
 
//  printf("\t CMS %f,%f - %f,%f\n", vx(tedge_v1(e)), vy(tedge_v1(e)), vx(tedge_v2(e)), vy(tedge_v2(e)));
 
  prevv = origin = TOPOROUTER_VERTEX(list->data);

//  print_edge(e);

  i = list;
  if(gts_point_distance2(GTS_POINT(origin), GTS_POINT(edge_v1(e))) < gts_point_distance2(GTS_POINT(v), GTS_POINT(edge_v1(e)))) {

    /* towards v2 */
    while(i) {
      nextv = edge_routing_next(e, i);
      if(nextv->route && vertices_connected(nextv, prevv)) { i = i->next; continue; }
      if(!(nextv->flags & VERTEX_FLAG_TEMP)) {
        gdouble ms = min_spacing(prevv, nextv);
          if(nextv == tedge_v2(e)) {
            gdouble cms = pathvertex_arcing_through_constraint(TOPOROUTER_VERTEX(i->data), tedge_v2(e));
//          printf("\t CMS to %f,%f = %f \t ms = %f\n", vx(tedge_v2(e)), vy(tedge_v2(e)), cms, ms);
//          if(vx(tedge_v2(e)) > -EPSILON && vx(tedge_v2(e)) < EPSILON) {
//            printf("\t\tPROB: ");
//            print_vertex(tedge_v2(e));
//          }
            if(cms > EPSILON) space += MIN(ms, cms / 2.);
            else space += ms;
          } else 
          space += ms;

        prevv = nextv;
      }
//      printf("%f ", space);
      i = i->next;
    }
  }else{

    /* towards v1 */
    while(i) {
      nextv = edge_routing_prev(e, i);
      if(nextv->route &&  vertices_connected(nextv, prevv)) { i = i->prev; continue; }
      if(!(nextv->flags & VERTEX_FLAG_TEMP)) {
        gdouble ms = min_spacing(prevv, nextv);
          if(nextv == tedge_v1(e)) {
            gdouble cms = pathvertex_arcing_through_constraint(TOPOROUTER_VERTEX(i->data), tedge_v1(e));
//          printf("\t CMS to %f,%f = %f \t ms = %f\n", vx(tedge_v1(e)), vy(tedge_v1(e)), cms, ms);
//          if(vx(tedge_v1(e)) > -EPSILON && vx(tedge_v1(e)) < EPSILON) {
//            printf("\t\tPROB: ");
//            print_vertex(tedge_v1(e));
//          }
            if(cms > EPSILON) space += MIN(ms, cms / 2.);
            else space += ms;
          } else 
          space += ms;

        prevv = nextv;
      }
//      printf("%f ", space);
      i = i->prev;
    }
  }

  if(TOPOROUTER_IS_CONSTRAINT(e) && space > gts_point_distance(GTS_POINT(edge_v1(e)), GTS_POINT(edge_v2(e))) / 2.)
    space = gts_point_distance(GTS_POINT(edge_v1(e)), GTS_POINT(edge_v2(e))) / 2.;
  
//  if(debug) printf("\tedge_min_spacing: %f\n", space);
  return space;
}

/* line segment is 1 & 2, point is 3 
   returns 0 if v3 is outside seg
*/
guint
vertex_line_normal_intersection(gdouble x1, gdouble y1, gdouble x2, gdouble y2, gdouble x3, gdouble y3, gdouble *x, gdouble *y)
{
  gdouble m1 = cartesian_gradient(x1,y1,x2,y2);
  gdouble m2 = perpendicular_gradient(m1);
  gdouble c2 = (isinf(m2)) ? x3 : y3 - (m2 * x3);
  gdouble c1 = (isinf(m1)) ? x1 : y1 - (m1 * x1);

  if(isinf(m2))
    *x = x3;
  else if(isinf(m1))
    *x = x1;
  else
    *x = (c2 - c1) / (m1 - m2);

  *y = (isinf(m2)) ? y1 : (m2 * (*x)) + c2;

  if(*x >= MIN(x1,x2) - EPSILON && *x <= MAX(x1,x2) + EPSILON && *y >= MIN(y1,y2) - EPSILON && *y <= MAX(y1,y2) + EPSILON) 
    return 1;
  return 0;
}

void
print_toporouter_arc(toporouter_arc_t *arc)
{
//  GList *i = arc->vs;

  printf("ARC CENTRE: %f,%f ", vx(arc->centre), vy(arc->centre));// print_vertex(arc->centre);
  printf("RADIUS: %f", arc->r);

  if(arc->dir>0) printf(" COUNTERCLOCKWISE ");
  else if(arc->dir<0) printf(" CLOCKWISE ");
  else printf(" COLINEAR(ERROR) ");
/*  
  printf("\n\tVS: ");

  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);
    printf("%f,%f ", vx(v), vy(v));

    i = i->next;
  }
*/
}
  
void
toporouter_arc_remove(toporouter_oproute_t *oproute, toporouter_arc_t *arc)
{
  oproute->arcs = g_list_remove(oproute->arcs, arc);

  if(arc->v) arc->v->arc = NULL;
}

toporouter_arc_t *
toporouter_arc_new(toporouter_oproute_t *oproute, toporouter_vertex_t *v1, toporouter_vertex_t *v2, toporouter_vertex_t *centre, gdouble r, gint dir)
{
  toporouter_arc_t *arc = TOPOROUTER_ARC(gts_object_new(GTS_OBJECT_CLASS(toporouter_arc_class())));
  arc->centre = centre;
  arc->v = v1;
  arc->v1 = v1;
  arc->v2 = v2;
  arc->r = r;
  arc->dir = dir;

  if(v1) v1->arc = arc;
  arc->oproute = oproute;

  arc->clearance = NULL;

  return arc;
}

void
path_set_oproute(GList *path, toporouter_oproute_t *oproute)
{
  while(path) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(path->data);

    if(v->flags & VERTEX_FLAG_ROUTE) 
      v->oproute = oproute;

    path = path->next;
  }
}

void
print_oproute(toporouter_oproute_t *oproute)
{
  GList *i = oproute->arcs;

  printf("Optimized Route:\n");
  printf("\tNetlist:\t\t%s\n\tStyle:\t\t%s\n", oproute->netlist, oproute->style);
 // printf("%s\n", oproute->netlist);
/*
  i = oproute->term1->zlink;
  while(i) {
    toporouter_vertex_t *thisv = TOPOROUTER_VERTEX(i->data);
    printf("\tNetlist:\t\t%s\n\tStyle:\t\t%s\n", vertex_bbox(thisv)->netlist, vertex_bbox(thisv)->style);
    i = i->next;
  }
*/
  printf("\t"); print_vertex(oproute->term1); printf("\n");
  i = oproute->arcs;
  while(i) {
    toporouter_arc_t *arc = (toporouter_arc_t *)i->data;
    printf("\t"); print_toporouter_arc(arc); printf("\n");
    i = i->next;
  }
  printf("\t"); print_vertex(oproute->term2); printf("\n");
}

gdouble
export_pcb_drawline(guint layer, guint x0, guint y0, guint x1, guint y1, guint thickness, guint keepaway) 
{
  gdouble d = 0.;
  LineType *line;
  line = CreateDrawnLineOnLayer( LAYER_PTR(layer), x0, y0, x1, y1, 
      thickness, keepaway, 
      MakeFlags (AUTOFLAG | (TEST_FLAG (CLEARNEWFLAG, PCB) ? CLEARLINEFLAG : 0)));

  if(line) {
    AddObjectToCreateUndoList (LINE_TYPE, LAYER_PTR(layer), line, line);
    d = hypot(x0 - x1, y0 - y1);
  }
  return d;
}

gdouble
arc_angle(toporouter_arc_t *arc) 
{
  gdouble x0, x1, y0, y1;

  x0 = arc->x0 - vx(arc->centre);
  x1 = arc->x1 - vx(arc->centre);
  y0 = arc->y0 - vy(arc->centre);
  y1 = arc->y1 - vy(arc->centre);

  return fabs(acos(((x0*x1)+(y0*y1))/(hypot(x0,y0)*hypot(x1,y1))));
}

gdouble
export_pcb_drawarc(guint layer, toporouter_arc_t *a, guint thickness, guint keepaway) 
{
  gdouble sa, da, theta;
  gdouble d = 0.;
  ArcType *arc;
  gint wind;

  wind = coord_wind(a->x0, a->y0, a->x1, a->y1, vx(a->centre), vy(a->centre));

  /* NB: PCB's arcs have a funny coorindate system, with 0 degrees as the -ve X axis (left),
   *     continuing clockwise, with +90 degrees being along the +ve Y axis (bottom). Because
   *     Y+ points down, our internal angles increase clockwise from the +ve X axis.
   */
  sa = (M_PI - coord_angle (vx (a->centre), vy (a->centre), a->x0, a->y0)) * 180. / M_PI;

  theta = arc_angle(a);

  if(!a->dir || !wind) return 0.;
  
  if(a->dir != wind) theta = 2. * M_PI - theta;
  
  da = -a->dir * theta * 180. / M_PI;

  if(da < 1. && da > -1.) return 0.;
  if(da > 359. || da < -359.) return 0.;

  arc = CreateNewArcOnLayer(LAYER_PTR(layer), vx(a->centre), vy(a->centre), a->r, a->r,
    sa, da, thickness, keepaway, 
    MakeFlags( AUTOFLAG | (TEST_FLAG (CLEARNEWFLAG, PCB) ? CLEARLINEFLAG : 0)));

  if(arc) {
    AddObjectToCreateUndoList( ARC_TYPE, LAYER_PTR(layer), arc, arc);
    d = a->r * theta;
  }
  
  return d;
}

void
calculate_term_to_arc(toporouter_vertex_t *v, toporouter_arc_t *arc, guint dir)
{
  gdouble theta, a, b, bx, by, a0x, a0y, a1x, a1y;
  gint winddir;

  theta = acos(arc->r / gts_point_distance(GTS_POINT(v), GTS_POINT(arc->centre)));
  a = arc->r * sin(theta);
  b = arc->r * cos(theta);
#ifdef DEBUG_EXPORT
  printf("drawing arc with r %f theta %f d %f centre = %f,%f\n", arc->r, theta, gts_point_distance(GTS_POINT(v), GTS_POINT(arc->centre)), vx(arc->centre), vy(arc->centre));
#endif
  point_from_point_to_point(arc->centre, v, b, &bx, &by);

  coords_on_line(bx, by, perpendicular_gradient(point_gradient(GTS_POINT(v), GTS_POINT(arc->centre))), a, &a0x, &a0y, &a1x, &a1y);

  winddir = coord_wind(vx(v), vy(v), a0x, a0y, vx(arc->centre), vy(arc->centre));

  if(!winddir) {
    printf("!winddir @ v %f,%f arc->centre %f,%f\n", vx(v), vy(v), vx(arc->centre), vy(arc->centre));
    //TODO: fix hack: this shouldn't happen
    arc->x0 = vx(v);
    arc->y0 = vy(v);
    arc->x1 = vx(v);
    arc->y1 = vy(v);
    return;
  }

  g_assert(winddir);

  if(dir) winddir = -winddir;

  if(winddir == arc->dir) {
    if(!dir) { arc->x0 = a0x; arc->y0 = a0y; }
    else{ arc->x1 = a0x; arc->y1 = a0y; }
  }else{
    if(!dir) { arc->x0 = a1x; arc->y0 = a1y; }
    else{ arc->x1 = a1x; arc->y1 = a1y; }
  }

}



/*!
 * \brief .
 *
 * b1 is the projection in the direction of narc, while b2 is the
 * perpendicular projection.
 */
void
arc_ortho_projections(toporouter_arc_t *arc, toporouter_arc_t *narc, gdouble *b1, gdouble *b2)
{
  gdouble nax, nay, ax, ay, alen, c;
  gdouble b1x, b1y, b2x, b2y;

#ifdef DEBUG_EXPORT
  printf("arc c = %f,%f narc c = %f,%f arc->0 = %f,%f\n", 
      vx(arc->centre), vy(arc->centre),
      vx(narc->centre), vy(narc->centre),
      arc->x0, arc->y0);
#endif

  nax = vx(narc->centre) - vx(arc->centre);
  nay = vy(narc->centre) - vy(arc->centre);
  alen = hypot(nax, nay);


  ax = arc->x0 - vx(arc->centre);
  ay = arc->y0 - vy(arc->centre);

#ifdef DEBUG_EXPORT
  printf("norm narc = %f,%f - %f\tA=%f,%f\n", nax, nay, alen, ax, ay);
#endif

  c = ((ax*nax)+(ay*nay)) / (alen*alen);

  b1x = c * nax;
  b1y = c * nay;
  b2x = ax - b1x;
  b2y = ay - b1y;

#ifdef DEBUG_EXPORT
  printf("proj = %f,%f perp proj = %f,%f\n", b1x, b1y, b2x, b2y);
#endif

  *b1 = hypot(b1x,b1y);
  *b2 = hypot(b2x,b2y);

}

guint
calculate_arc_to_arc(toporouter_t *ar, toporouter_arc_t *parc, toporouter_arc_t *arc)
{
  gdouble theta, a, b, bx, by, a0x, a0y, a1x, a1y, m, preva, prevb;
  gint winddir;
  toporouter_arc_t *bigr, *smallr;

  if(parc->r > arc->r) {
    bigr = parc; smallr = arc;
  }else{
    bigr = arc; smallr = parc;
  }
#ifdef DEBUG_EXPORT    
  printf("bigr centre = %f,%f smallr centre = %f,%f\n", vx(bigr->centre), vy(bigr->centre), 
      vx(smallr->centre), vy(smallr->centre));
#endif

  m = perpendicular_gradient(point_gradient(GTS_POINT(bigr->centre), GTS_POINT(smallr->centre)));

  if(bigr->centre == smallr->centre) {

    printf("bigr->centre == smallr->centre @ %f,%f\n", vx(smallr->centre), vy(smallr->centre));
  }

  g_assert(bigr->centre != smallr->centre);

  if(parc->dir == arc->dir) {
//export_arc_straight:

    theta = acos((bigr->r - smallr->r) / gts_point_distance(GTS_POINT(bigr->centre), GTS_POINT(smallr->centre)));
    a = bigr->r * sin(theta);
    b = bigr->r * cos(theta);

    point_from_point_to_point(bigr->centre, smallr->centre, b, &bx, &by);

    coords_on_line(bx, by, m, a, &a0x, &a0y, &a1x, &a1y);

    winddir = coord_wind(vx(smallr->centre), vy(smallr->centre), a0x, a0y, vx(bigr->centre), vy(bigr->centre));
    
    arc_ortho_projections(parc, arc, &prevb, &preva);
//#ifdef DEBUG_EXPORT    
    if(!winddir) {

      printf("STRAIGHT:\n");
      printf("bigr centre = %f,%f smallr centre = %f,%f\n", vx(bigr->centre), vy(bigr->centre), 
          vx(smallr->centre), vy(smallr->centre));
      printf("theta = %f a = %f b = %f bigrr = %f d = %f po = %f\n", theta, a, b, bigr->r,
          gts_point_distance(GTS_POINT(bigr->centre), GTS_POINT(smallr->centre)),
          bigr->r / gts_point_distance(GTS_POINT(bigr->centre), GTS_POINT(smallr->centre)));
      printf("bigr-r = %f smallr-r = %f ratio = %f\n",
          bigr->r, smallr->r, (bigr->r - smallr->r) / gts_point_distance(GTS_POINT(bigr->centre), GTS_POINT(smallr->centre)));
      printf("preva = %f prevb = %f\n\n", preva, prevb);
    
    }
//#endif
    g_assert(winddir);

    if(bigr==parc) winddir = -winddir;

    if(winddir == bigr->dir) {
      if(bigr==arc) {
        bigr->x0 = a0x; 
        bigr->y0 = a0y;
      }else{
        bigr->x1 = a0x; 
        bigr->y1 = a0y;
      }
    }else{
      if(bigr==arc) {
        bigr->x0 = a1x; 
        bigr->y0 = a1y;
      }else{
        bigr->x1 = a1x; 
        bigr->y1 = a1y;
      }
    }

    a = smallr->r * sin(theta);
    b = smallr->r * cos(theta);

#ifdef DEBUG_EXPORT   
    printf("a = %f b = %f\n", a, b);
#endif    
    point_from_point_to_point(smallr->centre, bigr->centre, -b, &bx, &by);

    coords_on_line(bx, by, m, a, &a0x, &a0y, &a1x, &a1y);

    if(winddir == bigr->dir) {
      if(bigr==arc) {
        smallr->x1 = a0x; 
        smallr->y1 = a0y;
      }else{
        smallr->x0 = a0x; 
        smallr->y0 = a0y;
      }
    }else{
      if(bigr==arc) {
        smallr->x1 = a1x; 
        smallr->y1 = a1y;
      }else{
        smallr->x0 = a1x; 
        smallr->y0 = a1y;
      }
    }

  }else{

//export_arc_twist:    

    theta = acos((bigr->r + smallr->r) / gts_point_distance(GTS_POINT(bigr->centre), GTS_POINT(smallr->centre)));
    a = bigr->r * sin(theta);
    b = bigr->r * cos(theta);

    point_from_point_to_point(bigr->centre, smallr->centre, b, &bx, &by);

    coords_on_line(bx, by, m, a, &a0x, &a0y, &a1x, &a1y);

    winddir = coord_wind(vx(smallr->centre), vy(smallr->centre), a0x, a0y, vx(bigr->centre), vy(bigr->centre));
//#ifdef DEBUG_EXPORT   
  if(!winddir) {
    printf("TWIST:\n");
    printf("theta = %f a = %f b = %f r = %f d = %f po = %f\n", theta, a, b, bigr->r + smallr->r,
        gts_point_distance(GTS_POINT(bigr->centre), GTS_POINT(smallr->centre)),
        (bigr->r+smallr->r) / gts_point_distance(GTS_POINT(bigr->centre), GTS_POINT(smallr->centre)));

    printf("bigr centre = %f,%f smallr centre = %f,%f\n\n", vx(bigr->centre), vy(bigr->centre), 
        vx(smallr->centre), vy(smallr->centre));
  
    printf("big wind = %d small wind = %d\n", bigr->dir, smallr->dir);
    return 1;
  }
//#endif      
/*    if(!winddir) {
      smallr->centre->flags |= VERTEX_FLAG_RED;
      bigr->centre->flags |= VERTEX_FLAG_GREEN;
      //bigr->centre->flags |= VERTEX_FLAG_RED;
      {
        int i;
        for(i=0;i<groupcount();i++) {
          char buffer[256];
          sprintf(buffer, "wind%d.png", i);
          toporouter_draw_surface(ar, ar->layers[i].surface, buffer, 2096, 2096, 2, NULL, i, NULL);
        }
      }
      return; 
    }
*/
    g_assert(winddir);

    if(bigr==parc) winddir = -winddir;

    if(winddir == bigr->dir) {
      if(bigr==arc) {
        bigr->x0 = a0x; 
        bigr->y0 = a0y;
      }else{
        bigr->x1 = a0x; 
        bigr->y1 = a0y;
      }
    }else{
      if(bigr==arc) {
        bigr->x0 = a1x; 
        bigr->y0 = a1y;
      }else{
        bigr->x1 = a1x; 
        bigr->y1 = a1y;
      }
    }

    a = smallr->r * sin(theta);
    b = smallr->r * cos(theta);

    point_from_point_to_point(smallr->centre, bigr->centre, b, &bx, &by);

    coords_on_line(bx, by, m, a, &a0x, &a0y, &a1x, &a1y);
    
    winddir = coord_wind(vx(smallr->centre), vy(smallr->centre), a0x, a0y, vx(bigr->centre), vy(bigr->centre));

    g_assert(winddir);

    if(bigr==parc) winddir = -winddir;

    if(winddir == smallr->dir) {
      if(bigr==arc) {
        smallr->x1 = a0x; 
        smallr->y1 = a0y;
      }else{
        smallr->x0 = a0x; 
        smallr->y0 = a0y;
      }
    }else{
      if(bigr==arc) {
        smallr->x1 = a1x; 
        smallr->y1 = a1y;
      }else{
        smallr->x0 = a1x; 
        smallr->y0 = a1y;
      }
    }

  }

  return 0;
}

void
export_oproutes(toporouter_t *ar, toporouter_oproute_t *oproute)
{
  guint layer = PCB->LayerGroups.Entries[oproute->layergroup][0];   
  guint thickness = lookup_thickness(oproute->style);
  guint keepaway = lookup_keepaway(oproute->style);
  GList *arcs = oproute->arcs;
  toporouter_arc_t *arc, *parc = NULL;

  if(!arcs) {
    ar->wiring_score += export_pcb_drawline(layer, vx(oproute->term1), vy(oproute->term1), vx(oproute->term2), vy(oproute->term2), thickness, keepaway);
    return;
  }


//  calculate_term_to_arc(oproute->term1, TOPOROUTER_ARC(arcs->data), 0, layer);

  while(arcs) {
    arc = TOPOROUTER_ARC(arcs->data);

    if(parc && arc) {
      ar->wiring_score += export_pcb_drawarc(layer, parc, thickness, keepaway);
      ar->wiring_score += export_pcb_drawline(layer, parc->x1, parc->y1, arc->x0, arc->y0, thickness, keepaway);
    }else if(!parc) {
      ar->wiring_score += export_pcb_drawline(layer, vx(oproute->term1), vy(oproute->term1), arc->x0, arc->y0, thickness, keepaway);
    }

    parc = arc;
    arcs = arcs->next;
  }
  ar->wiring_score += export_pcb_drawarc(layer, arc, thickness, keepaway);
  ar->wiring_score += export_pcb_drawline(layer, arc->x1, arc->y1, vx(oproute->term2), vy(oproute->term2), thickness, keepaway);

}



void
oproute_free(toporouter_oproute_t *oproute)
{
  GList *i = oproute->arcs;
  while(i) {
    toporouter_arc_t *arc = (toporouter_arc_t *) i->data;
    if(arc->centre->flags & VERTEX_FLAG_TEMP) 
      gts_object_destroy(GTS_OBJECT(arc->centre));

    i = i->next;
  }

  g_list_free(oproute->arcs);
  free(oproute);
}

void
oproute_calculate_tof(toporouter_oproute_t *oproute)
{
  GList *arcs = oproute->arcs;
  toporouter_arc_t *parc = NULL, *arc;

  oproute->tof = 0.;

  if(!arcs) {
    oproute->tof = gts_point_distance(GTS_POINT(oproute->term1), GTS_POINT(oproute->term2));
    return;
  }

  while(arcs) {
    arc = TOPOROUTER_ARC(arcs->data);

    if(parc && arc) {
      oproute->tof += arc_angle(parc) * parc->r;
      oproute->tof += hypot(parc->x1-arc->x0,parc->y1-arc->y0);
    }else if(!parc) {
      oproute->tof += hypot(arc->x0-vx(oproute->term1), arc->y0-vy(oproute->term1));
    }

    parc = arc;
    arcs = arcs->next;
  }

  oproute->tof += arc_angle(parc) * parc->r;
  oproute->tof += hypot(arc->x1-vx(oproute->term2), arc->y1-vy(oproute->term2));

}

gdouble
line_line_distance_at_normal(
    gdouble line1_x1, gdouble line1_y1,
    gdouble line1_x2, gdouble line1_y2,
    gdouble line2_x1, gdouble line2_y1,
    gdouble line2_x2, gdouble line2_y2,
    gdouble x, gdouble y)
{
  gdouble m1 = perpendicular_gradient(cartesian_gradient(line1_x1, line1_y1, line1_x2, line1_y2));
  gdouble m2 = cartesian_gradient(line2_x1, line2_y1, line2_x2, line2_y2);
  gdouble c1 = (isinf(m1)) ? x : y - (m1 * x);
  gdouble c2 = (isinf(m2)) ? line2_x1 : line2_y1 - (m2 * line2_x1);

  gdouble intx, inty;

  if(isinf(m2)) intx = line2_x1;
  else if(isinf(m1)) intx = x;
  else intx = (c2 - c1) / (m1 - m2);

  inty = (isinf(m2)) ? (m1 * intx) + c1 : (m2 * intx) + c2;

  return hypot(x-intx,y-inty);
}

void
calculate_serpintine(gdouble delta, gdouble r, gdouble initiala, gdouble *a, guint *nhalfcycles)
{
  gdouble lhalfcycle = 2.*(initiala-r)+(M_PI*r);
  guint n;

  printf("lhalfcycle = %f r = %f\n", lhalfcycle, r);

  n = (delta - M_PI*r) / (lhalfcycle - 2.*r) + 1;
  *a = (delta + 4.*n*r - n*M_PI*r + 4.*r - M_PI*r)/(2.*n);
  *nhalfcycles = n;
}

gdouble
oproute_min_spacing(toporouter_oproute_t *a, toporouter_oproute_t *b)
{
  return lookup_thickness(a->style) / 2. + lookup_thickness(b->style) / 2. + MAX(lookup_keepaway(a->style), lookup_keepaway(b->style));
}

gdouble
vector_angle(gdouble ox, gdouble oy, gdouble ax, gdouble ay, gdouble bx, gdouble by)
{
  gdouble alen = hypot(ax-ox,ay-oy);
  gdouble blen = hypot(bx-ox,by-oy);
  return acos( ((ax-ox)*(bx-ox)+(ay-oy)*(by-oy)) / (alen * blen) ); 
}

toporouter_serpintine_t *
toporouter_serpintine_new(gdouble x, gdouble y, gdouble x0, gdouble y0, gdouble x1, gdouble y1, gpointer start, gdouble halfa, gdouble
    radius, guint nhalfcycles)
{
  toporouter_serpintine_t *serp = (toporouter_serpintine_t *)malloc(sizeof(toporouter_serpintine_t));
  serp->x = x;
  serp->y = y;
  serp->x0 = x0;
  serp->y0 = y0;
  serp->x1 = x1;
  serp->y1 = y1;
  serp->start = start;
  serp->halfa = halfa;
  serp->radius = radius;
  serp->nhalfcycles = nhalfcycles;
  serp->arcs = NULL;
  return serp;
}

//#define DEBUG_RUBBERBAND 1

gdouble
check_non_intersect_vertex(gdouble x0, gdouble y0, gdouble x1, gdouble y1, toporouter_vertex_t *pathv, toporouter_vertex_t *arcv,
    toporouter_vertex_t *opv, gint wind, gint *arcwind, gdouble *arcr, guint debug)
{
  gdouble ms, line_int_x, line_int_y, x, y, d = 0., m;
  gdouble tx0, ty0, tx1, ty1;
  gint wind1, wind2;

  g_assert(pathv->routingedge);

  if(TOPOROUTER_IS_CONSTRAINT(pathv->routingedge)) { 
    gdouble d = tvdistance(tedge_v1(pathv->routingedge), tedge_v2(pathv->routingedge)) / 2.;
    ms = min_spacing(pathv, arcv);
    if(ms > d) ms = d;
  }else{
    ms = edge_min_spacing(g_list_find(edge_routing(pathv->routingedge), pathv), pathv->routingedge, arcv, debug);
  }


  if(!vertex_line_normal_intersection(x0, y0, x1, y1, vx(arcv), vy(arcv), &line_int_x, &line_int_y)) {

    if(hypot(x0 - line_int_x, y0 - line_int_y) < hypot(x1 - line_int_x, y1 - line_int_y)) 
    { line_int_x = x0; line_int_y = y0; }else{ line_int_x = x1; line_int_y = y1; }

    m = perpendicular_gradient(cartesian_gradient(vx(arcv), vy(arcv), line_int_x, line_int_y));
  }else{
    m = cartesian_gradient(x0, y0, x1, y1);
  }

  coords_on_line(vx(arcv), vy(arcv), m, MIL_TO_COORD (1.), &tx0, &ty0, &tx1, &ty1);

  wind1 = coord_wind(tx0, ty0, tx1, ty1, line_int_x, line_int_y);
  wind2 = coord_wind(tx0, ty0, tx1, ty1, vx(opv), vy(opv)); 

  if(!wind2 || wind1 == wind2) return -1.; 

  if(!wind) {
    coords_on_line(line_int_x, line_int_y, perpendicular_gradient(m), ms, &tx0, &ty0, &tx1, &ty1);
    if(hypot(tx0 - vx(opv), ty0 - vy(opv)) < hypot(tx1 - vx(opv), ty1 - vy(opv))) 
    { x = tx0; y = ty0; }else{ x = tx1; y = ty1; }
  }else{
    toporouter_vertex_t *parent = pathv->parent, *child = pathv->child;
    guint windtests = 0;

    d = hypot(vx(arcv) - line_int_x, vy(arcv) - line_int_y);
    coord_move_towards_coord_values(line_int_x, line_int_y, vx(arcv), vy(arcv), ms + d, &x, &y);
rewind_test:    
    wind1 = coord_wind(line_int_x, line_int_y, x, y, vx(parent), vy(parent));
    wind2 = coord_wind(line_int_x, line_int_y, x, y, vx(child), vy(child));
    if(wind1 && wind2 && wind1 == wind2) {
//      return -1.;
      if(windtests++ == 2) return -1.;

      if(parent->flags & VERTEX_FLAG_ROUTE) parent = parent->parent;
      if(child->flags & VERTEX_FLAG_ROUTE) child = child->child;
      goto rewind_test;
    }
  }


  *arcr = ms;
  *arcwind = tvertex_wind(pathv->parent, pathv, arcv);

#ifdef DEBUG_RUBBERBAND
//if(debug) 
//  printf("non-int check %f,%f ms %f d %f arcv %f,%f opv %f,%f\n", vx(arcv), vy(arcv), ms, d + ms,
//    vx(arcv), vy(arcv), vx(opv), vy(opv));
#endif

  return d + ms;
}

gdouble
check_intersect_vertex(gdouble x0, gdouble y0, gdouble x1, gdouble y1, toporouter_vertex_t *pathv, toporouter_vertex_t *arcv,
    toporouter_vertex_t *opv, gint wind, gint *arcwind, gdouble *arcr, guint debug)
{
  gdouble ms, line_int_x, line_int_y, x, y, d = 0.;

  if(TOPOROUTER_IS_CONSTRAINT(pathv->routingedge)) {
    gdouble d = tvdistance(tedge_v1(pathv->routingedge), tedge_v2(pathv->routingedge)) / 2.;
    ms = min_spacing(pathv, arcv);
    if(ms > d) ms = d;
  }else {
    ms = edge_min_spacing(g_list_find(edge_routing(pathv->routingedge), pathv), pathv->routingedge, arcv, debug);
  }

  if(!vertex_line_normal_intersection(x0, y0, x1, y1, vx(arcv), vy(arcv), &line_int_x, &line_int_y)) 
    return -1.; 

  d = hypot(line_int_x - vx(arcv), line_int_y - vy(arcv));


  if(d > ms - EPSILON) 
    return -1.;

  coord_move_towards_coord_values(vx(arcv), vy(arcv), line_int_x, line_int_y, ms, &x, &y);
  
  *arcr = ms;
  *arcwind = tvertex_wind(pathv->parent, pathv, arcv);
//  *arcwind = coord_wind(x0, y0, x, y, x1, y1);
#ifdef DEBUG_RUBBERBAND
//if(debug) 
//  printf("int check %f,%f ms %f d %f arcv %f,%f opv %f,%f\n", vx(arcv), vy(arcv), ms, ms - d,
//    vx(arcv), vy(arcv), vx(opv), vy(opv));
#endif

  return ms - d;
}

/*!
 * \brief Returns non-zero if arc has loops.
 */
guint
check_arc_for_loops(gpointer t1, toporouter_arc_t *arc, gpointer t2)
{
  gdouble x0, y0, x1, y1;

  if(TOPOROUTER_IS_VERTEX(t1)) { x0 = vx(TOPOROUTER_VERTEX(t1)); y0 = vy(TOPOROUTER_VERTEX(t1)); }
  else { x0 = TOPOROUTER_ARC(t1)->x1; y0 = TOPOROUTER_ARC(t1)->y1; }

  if(TOPOROUTER_IS_VERTEX(t2)) { x1 = vx(TOPOROUTER_VERTEX(t2)); y1 = vy(TOPOROUTER_VERTEX(t2)); }
  else { x1 = TOPOROUTER_ARC(t2)->x0; y1 = TOPOROUTER_ARC(t2)->y0; }

  if(coord_intersect_prop(x0, y0, arc->x0, arc->y0, arc->x1, arc->y1, x1, y1) ) {
//  || 
//   (arc->x0 > arc->x1 - EPSILON && arc->x0 < arc->x1 + EPSILON &&
//     arc->y0 > arc->y1 - EPSILON && arc->y0 < arc->y1 + EPSILON)
//    ) {
#ifdef DEBUG_RUBBERBAND
    printf("LOOPS %f %f -> %f %f & %f %f -> %f %f\n", x0, y0, arc->x0, arc->y0, arc->x1, arc->y1, x1, y1);
#endif
    return 1;
  }
  return 0;
}

toporouter_rubberband_arc_t *
new_rubberband_arc(toporouter_vertex_t *pathv, toporouter_vertex_t *arcv, gdouble r, gdouble d, gint wind, GList *list)
{
  toporouter_rubberband_arc_t *rba = (toporouter_rubberband_arc_t *)malloc(sizeof(toporouter_rubberband_arc_t));
  rba->pathv = pathv;
  rba->arcv = arcv;
  rba->r = r;
  rba->d = d;
  rba->wind = wind;
  rba->list = list;
  return rba;
}

gint
compare_rubberband_arcs(toporouter_rubberband_arc_t *a, toporouter_rubberband_arc_t *b)
{ return b->d - a->d; }

void
free_list_elements(gpointer data, gpointer user_data)
{ free(data); }


/* returns the edge opposite v from the triangle facing (x,y), or NULL if v is colinear with an edge between v and a neighbor */
/*
GtsEdge *
vertex_edge_facing_vertex(GtsVertex *v, gdouble x, gdouble y)
{
  GSList *ts = gts_vertex_triangles(GTS_VERTEX(n), NULL);
  GSList *i = ts;

  while(i) {
    GtsTriangle *t = GTS_TRIANGLE(i->data);
    GtsEdge *e = gts_triangle_edge_opposite(t, v);

    if(coord_wind(vx(edge_v1(e)), vy(edge_v1(e)), vx(v), vy(v), x, y) == vertex_wind(edge_v1(e), v, edge_v2(e)) && 
       coord_wind(vx(edge_v2(e)), vy(edge_v2(e)), vx(v), vy(v), x, y) == vertex_wind(edge_v2(e), v, edge_v1(e))
       ) {
     g_slist_free(ts);
     return e;
    }

    i = i->next;
  }

  g_slist_free(ts);
  return NULL;
}
*/

gdouble 
check_adj_pushing_vertex(toporouter_oproute_t *oproute, gdouble x0, gdouble y0, gdouble x1, gdouble y1, toporouter_vertex_t *v, gdouble *arcr, gint *arcwind, toporouter_vertex_t **arc) 
{
  GSList *ns = gts_vertex_neighbors(GTS_VERTEX(v), NULL, NULL);
  GSList *i = ns;
  gdouble maxd = 0.;

  while(i) {
    toporouter_vertex_t *n = TOPOROUTER_VERTEX(i->data);
    gdouble segintx, seginty;
    if(vertex_line_normal_intersection(x0, y0, x1, y1, vx(n), vy(n), &segintx, &seginty)) {
      toporouter_edge_t *e = tedge(n, v);
      gdouble ms = 0., d = hypot(segintx - vx(n), seginty - vy(n));
      //toporouter_vertex_t *a;
      toporouter_vertex_t *b;
      GList *closestnet = NULL;

      g_assert(e);

      if(v == tedge_v1(e)) {
        //a = tedge_v1(e);
        b = tedge_v2(e);
        closestnet = edge_routing(e);
      }else{
        //a = tedge_v2(e);
        b = tedge_v1(e);
        closestnet = g_list_last(edge_routing(e));
      }

      if(closestnet) {
        ms = edge_min_spacing(closestnet, e, b, 0);        
        ms += min_oproute_net_spacing(oproute, TOPOROUTER_VERTEX(closestnet->data));
      }else{
        ms = min_oproute_vertex_spacing(oproute, b);
      }

      if(ms - d > maxd) {
        *arcr = ms;
        *arc = n;
        maxd = ms - d;
        if(vx(v) == x0 && vy(v) == y0) {
          *arcwind = coord_wind(x0, y0, vx(n), vy(n), x1, y1);
        }else if(vx(v) == x1 && vy(v) == y1) {
          *arcwind = coord_wind(x1, y1, vx(n), vy(n), x0, y0);
        }else{
          fprintf(stderr, "ERROR: check_adj_pushing_vertex encountered bad vertex v (coordinates don't match)\n");
        }
      }
    }

    i = i->next;
  }

  g_slist_free(ns);
  return maxd;
}


/*!
 * \brief .
 *
 * path is t1 path.
 */
GList *
oproute_rubberband_segment(toporouter_t *r, toporouter_oproute_t *oproute, GList *path, gpointer t1, gpointer t2, guint debug)
{
  gdouble x0, y0, x1, y1;
  toporouter_vertex_t *v1, *v2, *av1, *av2; /* v{1,2} are the vertex terminals of the segment, or arc terminal centres */
  toporouter_arc_t *arc1 = NULL, *arc2 = NULL, *newarc = NULL; /* arc{1,2} are the arc terminals of the segment, if they exist */
  GList *i = path;
  GList *list1, *list2;

  GList *arcs = NULL;
  toporouter_rubberband_arc_t *max = NULL;
    
  gdouble d, arcr;
  gint v1wind, v2wind, arcwind;

  if(TOPOROUTER_IS_VERTEX(t1)) {
    v1 = TOPOROUTER_VERTEX(t1);
    x0 = vx(v1); y0 = vy(v1);
  }else{
    g_assert(TOPOROUTER_IS_ARC(t1));
    arc1 = TOPOROUTER_ARC(t1);
    v1 = TOPOROUTER_VERTEX(arc1->v1);
    x0 = arc1->x1;
    y0 = arc1->y1;
  }

  if(TOPOROUTER_IS_VERTEX(t2)) {
    v2 = TOPOROUTER_VERTEX(t2);
    x1 = vx(v2); y1 = vy(v2);
  }else{
    g_assert(TOPOROUTER_IS_ARC(t2));
    arc2 = TOPOROUTER_ARC(t2);
    v2 = TOPOROUTER_VERTEX(arc2->v2);
    x1 = arc2->x0;
    y1 = arc2->y0;
  }
  
#define TEST_AND_INSERT(z) if(d > EPSILON) arcs = g_list_prepend(arcs, new_rubberband_arc(v, z, arcr, d, arcwind, i));
#define ARC_CHECKS(z) (!(arc1 && arc1->centre == z) && !(arc2 && arc2->centre == z) && \
  !(TOPOROUTER_IS_VERTEX(t1) && z == v1) && !(TOPOROUTER_IS_VERTEX(t2) && z == v2))    

  if(v1 == v2 || !i->next || TOPOROUTER_VERTEX(i->data) == v2) return NULL;

//#ifdef DEBUG_RUBBERBAND
  if(debug) {
    printf("\nRB: line %f,%f %f,%f v1 = %f,%f v2 = %f,%f \n ", x0, y0, x1, y1, vx(v1), vy(v1), vx(v2), vy(v2)); 
//    if(v1->routingedge) print_edge(v1->routingedge);
//    if(v2->routingedge) print_edge(v2->routingedge);

  }
//#endif
  
  /* check the vectices adjacent to the terminal vectices for push against the segment */
//if(TOPOROUTER_IS_VERTEX(t1)) {
//  toporouter_vertex_t *arcc = NULL;
//  d = check_adj_pushing_vertex(oproute, x0, y0, x1, y1, v1, &arcr, &arcwind, &arcc); 
//  g_assert(arcc != v1);
//  if(ARC_CHECKS(arcc) && d > EPSILON) arcs = g_list_prepend(arcs, new_rubberband_arc(v1, arcc, arcr, d, arcwind, path->next));
//}
//
//if(TOPOROUTER_IS_VERTEX(t2)) {
//  toporouter_vertex_t *arcc = NULL;
//  d = check_adj_pushing_vertex(oproute, x0, y0, x1, y1, v2, &arcr, &arcwind, &arcc);
//  g_assert(arcc != v2);
//  if(ARC_CHECKS(arcc) && d > EPSILON) arcs = g_list_prepend(arcs, new_rubberband_arc(v2, arcc, arcr, d, arcwind, g_list_last(path)->prev));
//}
  
  i = i->next;
  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);

    if(v == v2 || v == v1 || !v->routingedge) break;

#ifdef DEBUG_RUBBERBAND
//  if(debug) 
//  printf("current v %f,%f - edge %f,%f %f,%f\n", vx(v), vy(v),
//    vx(tedge_v1(v->routingedge)), vy(tedge_v1(v->routingedge)),
//    vx(tedge_v2(v->routingedge)), vy(tedge_v2(v->routingedge))
//    );
#endif
    g_assert(v->routingedge);
   
    v1wind = coord_wind(x0, y0, x1, y1, vx(tedge_v1(v->routingedge)), vy(tedge_v1(v->routingedge)));
    v2wind = coord_wind(x0, y0, x1, y1, vx(tedge_v2(v->routingedge)), vy(tedge_v2(v->routingedge)));
//    if(debug) printf("\twinds: %d %d\n", v1wind, v2wind);
    if(!v1wind && !v2wind) { i = i->next; continue; }


    if(v1wind && v2wind && v1wind != v2wind) { /* edge is cutting through the current segment */ 
      
      if(ARC_CHECKS(tedge_v1(v->routingedge)) ){ /* edge v1 is not the centre of an arc terminal */
        d = check_intersect_vertex(x0, y0, x1, y1, v, tedge_v1(v->routingedge), tedge_v2(v->routingedge), v1wind, &arcwind, &arcr, debug); 
        TEST_AND_INSERT(tedge_v1(v->routingedge));
      }

      if(ARC_CHECKS(tedge_v2(v->routingedge)) ){ /* edge v2 is not the centre of an arc terminal */
        d = check_intersect_vertex(x0, y0, x1, y1, v, tedge_v2(v->routingedge), tedge_v1(v->routingedge), v2wind, &arcwind, &arcr, debug);  
        TEST_AND_INSERT(tedge_v2(v->routingedge));
      }
    }else{ /* edge is on one side of the segment */
      
      if(ARC_CHECKS(tedge_v1(v->routingedge)) ){ /* edge v1 is not the centre of an arc terminal */
        d = check_non_intersect_vertex(x0, y0, x1, y1, v, tedge_v1(v->routingedge), tedge_v2(v->routingedge), v1wind, &arcwind, &arcr, debug);  
        TEST_AND_INSERT(tedge_v1(v->routingedge));
      }

      if(ARC_CHECKS(tedge_v2(v->routingedge)) ){ /* edge v2 is not the centre of an arc terminal */
        d = check_non_intersect_vertex(x0, y0, x1, y1, v, tedge_v2(v->routingedge), tedge_v1(v->routingedge), v2wind, &arcwind, &arcr, debug);  
        TEST_AND_INSERT(tedge_v2(v->routingedge));
      }
    }

    i = i->next;
  }

  arcs = g_list_sort(arcs, (GCompareFunc) compare_rubberband_arcs);
//rubberband_insert_maxarc:
  if(!arcs) return NULL;
  max = TOPOROUTER_RUBBERBAND_ARC(arcs->data); 
  
  av2 = max->pathv; i = max->list->next;
  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);
    if(v->routingedge && (tedge_v1(v->routingedge) == max->arcv || tedge_v2(v->routingedge) == max->arcv)) {
      av2 = v; i = i->next; continue;
    }
    break;
  }
  
  av1 = max->pathv; i = max->list->prev;
  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);
    if(v->routingedge && (tedge_v1(v->routingedge) == max->arcv || tedge_v2(v->routingedge) == max->arcv)) {
      av1 = v; i = i->prev; continue;
    }
    break;
  }
//#ifdef DEBUG_RUBBERBAND
  if(debug) 
    printf("newarc @ %f,%f \t v1 = %f,%f v2 = %f,%f r = %f\n", vx(max->arcv), vy(max->arcv), vx(av1), vy(av1), vx(av2), vy(av2), max->r);
//#endif
  newarc = toporouter_arc_new(oproute, av1, av2, max->arcv, max->r, max->wind);

  if(TOPOROUTER_IS_VERTEX(t1)) 
    calculate_term_to_arc(TOPOROUTER_VERTEX(t1), newarc, 0);
  else if(calculate_arc_to_arc(r, TOPOROUTER_ARC(t1), newarc)) { 
    printf("\tERROR: best:  r = %f d = %f\n", max->r, max->d); 
    printf("\tOPROUTE: %s\n", oproute->netlist);
    print_vertex(oproute->term1);
    print_vertex(oproute->term2);
    return NULL; 
  }

  if(TOPOROUTER_IS_VERTEX(t2)) 
    calculate_term_to_arc(TOPOROUTER_VERTEX(t2), newarc, 1);
  else if(calculate_arc_to_arc(r, newarc, TOPOROUTER_ARC(t2))) { 
    printf("\tERROR: best: r = %f d = %f\n", max->r, max->d); 
    printf("\tOPROUTE: %s\n", oproute->netlist);
    print_vertex(oproute->term1);
    print_vertex(oproute->term2);
    return NULL; 
  }

//if(check_arc_for_loops(t1, newarc, t2)) {
//  if(arc1 && arc2) calculate_arc_to_arc(r, arc1, arc2);
//  else if(arc1) calculate_term_to_arc(TOPOROUTER_VERTEX(t2), arc1, 1);
//  else if(arc2) calculate_term_to_arc(TOPOROUTER_VERTEX(t1), arc2, 0);

//#ifdef DEBUG_RUBBERBAND
//  printf("REMOVING NEW ARC @ %f,%f\n", vx(newarc->centre), vy(newarc->centre));
//  //TODO: properly remove newarc
//#endif

//  arcs = g_list_remove(arcs, max);
//  free(max);
//  goto rubberband_insert_maxarc;
//}


  list1 = oproute_rubberband_segment(r, oproute, path, t1, newarc, debug);
  list2 = oproute_rubberband_segment(r, oproute, i->next, newarc, t2, debug);

  if(list1) {
    GList *list = g_list_last(list1);
    toporouter_arc_t *testarc = TOPOROUTER_ARC(list->data);
    toporouter_arc_t *parc = list->prev ? TOPOROUTER_ARC(list->prev->data) : arc1;
    gdouble px = parc ? parc->x1 : vx(TOPOROUTER_VERTEX(t1)), py = parc ? parc->y1 : vy(TOPOROUTER_VERTEX(t1));

    if(coord_intersect_prop(px, py, testarc->x0, testarc->y0, testarc->x1, testarc->y1, newarc->x0, newarc->y0)) {
      list1 = g_list_remove(list1, testarc);
      if(parc) calculate_arc_to_arc(r, parc, newarc);
      else calculate_term_to_arc(TOPOROUTER_VERTEX(t1), newarc, 0);
//#ifdef DEBUG_RUBBERBAND
    if(debug)
      printf("REMOVING ARC @ %f,%f\n", vx(testarc->centre), vy(testarc->centre));
//#endif
    }
  }
  if(list2) {
    toporouter_arc_t *testarc = TOPOROUTER_ARC(list2->data);
    toporouter_arc_t *narc = list2->next ? TOPOROUTER_ARC(list2->next->data) : arc2;
    gdouble nx = narc ? narc->x0 : vx(TOPOROUTER_VERTEX(t2)), ny = narc ? narc->y0 : vy(TOPOROUTER_VERTEX(t2));

    if(coord_intersect_prop(newarc->x1, newarc->y1, testarc->x0, testarc->y0, testarc->x1, testarc->y1, nx, ny)) {
      list2 = g_list_remove(list2, testarc);
      if(narc) calculate_arc_to_arc(r, newarc, narc);
      else calculate_term_to_arc(TOPOROUTER_VERTEX(t2), newarc, 1);
    
//#ifdef DEBUG_RUBBERBAND
    if(debug)
      printf("REMOVING ARC @ %f,%f\n", vx(testarc->centre), vy(testarc->centre));
//#endif
    }
  }

  g_list_foreach(arcs, free_list_elements, NULL);
  g_list_free(arcs);

  return g_list_concat(list1, g_list_prepend(list2, newarc));
}
  
void
oproute_check_all_loops(toporouter_t *r, toporouter_oproute_t *oproute)
{
  GList *i; 
  gpointer t1;

loopcheck_restart:
  t1 = oproute->term1;
  i = oproute->arcs;
  while(i) {
    toporouter_arc_t *arc = TOPOROUTER_ARC(i->data);
    gpointer t2 = i->next ? i->next->data : oproute->term2;

    if(check_arc_for_loops(t1, arc, t2)) {
      
      if(TOPOROUTER_IS_ARC(t1) && TOPOROUTER_IS_ARC(t2)) 
        calculate_arc_to_arc(r, TOPOROUTER_ARC(t1), TOPOROUTER_ARC(t2));
      else if(TOPOROUTER_IS_ARC(t1)) 
        calculate_term_to_arc(TOPOROUTER_VERTEX(t2), TOPOROUTER_ARC(t1), 1);
      else if(TOPOROUTER_IS_ARC(t2)) 
        calculate_term_to_arc(TOPOROUTER_VERTEX(t1), TOPOROUTER_ARC(t2), 0);

      oproute->arcs = g_list_remove(oproute->arcs, arc);
      goto loopcheck_restart;
    }

    t1 = arc;

    i = i->next;
  }

}

GtsTriangle *
opposite_triangle(GtsTriangle *t, toporouter_edge_t *e)
{
  GSList *i = GTS_EDGE(e)->triangles;

  g_assert(e && t);

  while(i) {
    if(GTS_TRIANGLE(i->data) != t) return GTS_TRIANGLE(i->data);
    i = i->next;
  }
  
  return NULL;
}


void
speccut_edge_routing_from_edge(GList *i, toporouter_edge_t *e)
{
  g_assert(TOPOROUTER_IS_EDGE(e));
  while(i) {
    toporouter_vertex_t *curv = TOPOROUTER_VERTEX(i->data);
    
    if(!(curv->flags & VERTEX_FLAG_TEMP)) {
      toporouter_vertex_t *newv = tvertex_intersect(curv, curv->parent, tedge_v1(e), tedge_v2(e));
        
//      printf("\nCURV:\n");
//      print_vertex(curv);
//      
//      printf("CURV child:\n");
//      if(curv->child) 
//        print_vertex(curv->child);
//      else 
//        printf("NULL\n");
//      
//      printf("CURV parent:\n");
//      if(curv->parent)
//        print_vertex(curv->parent);
//      else 
//        printf("NULL\n");

      if(newv) {
        gint index;
        newv->flags |= VERTEX_FLAG_ROUTE; 
        newv->flags |= VERTEX_FLAG_SPECCUT; 
        e->routing = g_list_insert_sorted_with_data(e->routing, newv, routing_edge_insert, e);
        newv->route = curv->route; 
        newv->oproute = curv->oproute;
        newv->routingedge = e;
        GTS_POINT(newv)->z = vz(curv);

        newv->parent = curv->parent;
        newv->child = curv;

//        curv->parent = newv;

        index = g_list_index(newv->route->path, curv);

        newv->route->path = g_list_insert(newv->route->path, newv, index);


        if(newv->oproute) newv->oproute->path = newv->route->path;
      }

      if(!(curv->child->routingedge)) {
        newv = tvertex_intersect(curv, curv->child, tedge_v1(e), tedge_v2(e));

        if(newv) {
          gint index;
          newv->flags |= VERTEX_FLAG_ROUTE; 
          newv->flags |= VERTEX_FLAG_SPECCUT; 
          e->routing = g_list_insert_sorted_with_data(e->routing, newv, routing_edge_insert, e);
          newv->route = curv->route; 
          newv->oproute = curv->oproute;
          newv->routingedge = e;
          GTS_POINT(newv)->z = vz(curv);

          newv->parent = curv;
          newv->child = curv->child;

//          curv->child = newv;

          index = g_list_index(newv->route->path, curv);

          newv->route->path = g_list_insert(newv->route->path, newv, index+1);


          if(newv->oproute) newv->oproute->path = newv->route->path;
        }

      }

    }
    i = i->next;
  }

}

void
speccut_edge_patch_links(toporouter_edge_t *e)
{
  GList *i = e->routing;
  g_assert(TOPOROUTER_IS_EDGE(e));
  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);
    v->parent->child = v;
    v->child->parent = v;
    i = i->next;
  }
}

gint
check_speccut(toporouter_oproute_t *oproute, toporouter_vertex_t *v1, toporouter_vertex_t *v2, toporouter_edge_t *e, toporouter_edge_t *e1, toporouter_edge_t *e2)
{
  GtsTriangle *t, *opt;
  toporouter_vertex_t *opv, *opv2;
  toporouter_edge_t *ope1, *ope2;
  gdouble cap, flow, line_int_x, line_int_y;
 
  if(TOPOROUTER_IS_CONSTRAINT(e)) return 0;

  if(!(t = gts_triangle_use_edges(GTS_EDGE(e), GTS_EDGE(e1), GTS_EDGE(e2)))) {
    printf("check_speccut: NULL t\n");
    return 0;
  }
   
  if(!(opt = opposite_triangle(t, e))) {
//    printf("check_speccut: NULL opt\n");
    return 0;
  }

  if(!(opv = segment_common_vertex(GTS_SEGMENT(e1), GTS_SEGMENT(e2)))) {
    printf("check_speccut: NULL opv\n");
    return 0;
  }
  
  if(!(opv2 = TOPOROUTER_VERTEX(gts_triangle_vertex_opposite(opt, GTS_EDGE(e))))) {
    printf("check_speccut: NULL opv2\n");
    return 0;
  }
 
  //TODO: shifting it out of the way would be better
  if(e->routing) {
    GList *i = e->routing;
    while(i) {
      toporouter_vertex_t *ev = TOPOROUTER_VERTEX(i->data);
      if(!tvertex_wind(opv, ev, opv2)) return 0;    
      i = i->next;
    }

  }

  ope1 = tedge(opv2, tedge_v1(e));
  ope2 = tedge(opv2, tedge_v2(e));

 //this fixes the weird pad exits in r8c board
//  if(TOPOROUTER_IS_CONSTRAINT(ope1)) return 0;
  if(TOPOROUTER_IS_CONSTRAINT(ope2)) return 0;

  if(!tvertex_wind(opv2, tedge_v1(e), opv)) return 0;
  if(!tvertex_wind(opv2, tedge_v2(e), opv)) return 0;
  
  if(!vertex_line_normal_intersection(
      vx(tedge_v1(e)), vy(tedge_v1(e)), 
      vx(tedge_v2(e)), vy(tedge_v2(e)), 
      vx(opv2), vy(opv2), 
      &line_int_x, &line_int_y)) return 0;
  

//  return 0;
//if(vertex_line_normal_intersection(tev1x(e), tev1y(e), tev2x(e), tev2y(e), vx(opv), vy(opv), &line_int_x, &line_int_y))
//  return 0;

  g_assert(opt && opv2);
 
  /* this is just temp, for the purposes of determining flow */
  if(tedge_v1(ope1) == opv2) {
    if(TOPOROUTER_IS_CONSTRAINT(ope1)) 
      TOPOROUTER_CONSTRAINT(ope1)->routing = g_list_append(TOPOROUTER_CONSTRAINT(ope1)->routing, v1);
    else
      ope1->routing = g_list_append(ope1->routing, v1);
  }else{
    if(TOPOROUTER_IS_CONSTRAINT(ope1)) 
      TOPOROUTER_CONSTRAINT(ope1)->routing = g_list_prepend(TOPOROUTER_CONSTRAINT(ope1)->routing, v1);
    else
      ope1->routing = g_list_prepend(ope1->routing, v1);
  }

  cap = triangle_interior_capacity(opt, opv2);
  flow = flow_from_edge_to_edge(opt, tedge(opv2, tedge_v1(e)), tedge(opv2, tedge_v2(e)), opv2, v1);
  
  /* temp v1 removed */
  if(TOPOROUTER_IS_CONSTRAINT(ope1)) 
    TOPOROUTER_CONSTRAINT(ope1)->routing = g_list_remove(TOPOROUTER_CONSTRAINT(ope1)->routing, v1);
  else
    ope1->routing = g_list_remove(ope1->routing, v1);

  if(flow >= cap) {
    toporouter_edge_t *newe = TOPOROUTER_EDGE(gts_edge_new(GTS_EDGE_CLASS(toporouter_edge_class()), GTS_VERTEX(opv), GTS_VERTEX(opv2)));

    speccut_edge_routing_from_edge(edge_routing(e1), newe);
    speccut_edge_routing_from_edge(edge_routing(e2), newe);
    speccut_edge_routing_from_edge(edge_routing(ope1), newe);
    speccut_edge_routing_from_edge(edge_routing(ope2), newe);

    speccut_edge_patch_links(newe);
/*
    printf("SPECCUT WITH v %f,%f for seg %f,%f %f,%f detected\n", vx(opv2), vy(opv2),
        vx(v1), vy(v1), 
        vx(v2), vy(v2));
    printf("\tflow %f cap %f\n", flow, cap);
    print_edge(newe);
  */  
    if(newe->routing) return 1;
  }


  return 0;
}
      

gint
oproute_path_speccut(toporouter_oproute_t *oproute)
{
  GList *i;
  toporouter_vertex_t *pv; 
path_speccut_restart:
  i = oproute->path;
  pv = NULL;
  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);


    if(pv && (v->routingedge || pv->routingedge) && !(pv->flags & VERTEX_FLAG_SPECCUT) && !(v->flags & VERTEX_FLAG_SPECCUT)) {
      
      if(!v->routingedge) {
        if(check_speccut(oproute, pv, v, tedge(tedge_v1(pv->routingedge), v), pv->routingedge, tedge(tedge_v2(pv->routingedge), v)))
          goto path_speccut_restart;
        if(check_speccut(oproute, pv, v, tedge(tedge_v2(pv->routingedge), v), pv->routingedge, tedge(tedge_v1(pv->routingedge), v))) 
          goto path_speccut_restart;
      }else if(!pv->routingedge) {
        if(check_speccut(oproute, v, pv, tedge(tedge_v1(v->routingedge), pv), v->routingedge, tedge(tedge_v2(v->routingedge), pv)))
          goto path_speccut_restart;
        if(check_speccut(oproute, v, pv, tedge(tedge_v2(v->routingedge), pv), v->routingedge, tedge(tedge_v1(v->routingedge), pv))) 
          goto path_speccut_restart;
      }else{
        toporouter_vertex_t *v1 = NULL, *v2 = NULL;
        edges_third_edge(GTS_SEGMENT(v->routingedge), GTS_SEGMENT(pv->routingedge), &v1, &v2); 
        if(check_speccut(oproute, v, pv, tedge(v1, v2), v->routingedge, pv->routingedge)) 
          goto path_speccut_restart;
      }
    }


    pv = v;
    i = i->next;
  }

  return 0;
}

toporouter_oproute_t *
oproute_rubberband(toporouter_t *r, GList *path)
{
  toporouter_oproute_t *oproute = (toporouter_oproute_t *)malloc(sizeof(toporouter_oproute_t)); 

  g_assert(path);

  oproute->term1 = TOPOROUTER_VERTEX(path->data);
  oproute->term2 = TOPOROUTER_VERTEX(g_list_last(path)->data);
  oproute->arcs = NULL;
  oproute->style = vertex_bbox(oproute->term1)->cluster->netlist->style;
  oproute->netlist = vertex_bbox(oproute->term1)->cluster->netlist->netlist;
  oproute->layergroup = vz(oproute->term1);
  oproute->path = path;
  oproute->serp = NULL;

  oproute->term1->parent = NULL;
  oproute->term2->child = NULL;

  path_set_oproute(path, oproute);

//  if(!strcmp(oproute->netlist, "  unnamed_net1")) 
  oproute_path_speccut(oproute);

#ifdef DEBUG_RUBBERBAND
  if(strcmp(oproute->netlist, "  VCC3V3" == 0) &&
     vx(oproute->term1) == MIL_TO_COORD (957.) &&
     vy(oproute->term1) == MIL_TO_COORD (708.) &&
     vx(oproute->term2) == MIL_TO_COORD (1967.) &&
     vy(oproute->term2) == MIL_TO_COORD (673.))
  {
//    printf("OPROUTE %s - %f,%f %f,%f\n", oproute->netlist, vx(oproute->term1), vy(oproute->term1), vx(oproute->term2), vy(oproute->term2));
//    print_path(path);
    oproute->arcs = oproute_rubberband_segment(r, oproute, path, oproute->term1, oproute->term2, 1);
  }else
#endif    
    oproute->arcs = oproute_rubberband_segment(r, oproute, path, oproute->term1, oproute->term2, 0);

  oproute_check_all_loops(r, oproute);
  return oproute;

}

void
toporouter_export(toporouter_t *r) 
{
  GList *i = r->routednets;
  GList *oproutes = NULL;

  while(i) {
    toporouter_route_t *routedata = TOPOROUTER_ROUTE(i->data);
    toporouter_oproute_t *oproute = oproute_rubberband(r, routedata->path);
    oproutes = g_list_prepend(oproutes, oproute);
    i = i->next;
  }

  i = oproutes;
  while(i) {
    toporouter_oproute_t *oproute = (toporouter_oproute_t *) i->data;
    export_oproutes(r, oproute);
    oproute_free(oproute);
    i = i->next;
  }

  Message (_("Reticulating splines... successful\n\n"));
  /* NB: We could use the %$mS specifier to print these distances, but we would
   *     have to cast to Coord, which might overflow for complex routing when
   *     PCB is built with Coord as a 32-bit integer.
   */
  Message (_("Wiring cost: %f inches\n"), COORD_TO_INCH (r->wiring_score));
  printf (_("Wiring cost: %f inches\n"), COORD_TO_INCH (r->wiring_score));

  g_list_free(oproutes);

}

toporouter_route_t *
routedata_create(void)
{
  toporouter_route_t *routedata = (toporouter_route_t *)malloc(sizeof(toporouter_route_t));
  routedata->netlist = NULL;
  routedata->alltemppoints = NULL;
  routedata->path = NULL;
  routedata->curpoint = NULL;
  routedata->pscore = routedata->score = 0.;
  routedata->flags = 0;
  routedata->src = routedata->dest = NULL;
  routedata->psrc = routedata->pdest = NULL;
  routedata->ppath = routedata->topopath = NULL;

  routedata->ppathindices = NULL;

  routedata->destvertices = routedata->srcvertices = NULL;
  return routedata;
}
/*
void
print_routedata(toporouter_route_t *routedata)
{
  GList *srcvertices = cluster_vertices(routedata->src);
  GList *destvertices = cluster_vertices(routedata->dest);

  printf("ROUTEDATA:\n");
  printf("SRCVERTICES:\n");
  print_vertices(srcvertices);
  printf("DESTVERTICES:\n");
  print_vertices(destvertices);

  g_list_free(srcvertices);
  g_list_free(destvertices);
}*/

toporouter_route_t *
import_route(toporouter_t *r, RatType *line)
{
  toporouter_route_t *routedata = routedata_create();

  routedata->src = cluster_find(r, line->Point1.X, line->Point1.Y, line->group1);
  routedata->dest = cluster_find(r, line->Point2.X, line->Point2.Y, line->group2);

  if(!routedata->src) printf("couldn't locate src\n");
  if(!routedata->dest) printf("couldn't locate dest\n");

  if(!routedata->src || !routedata->dest) {
    pcb_printf("PROBLEM: couldn't locate rat src or dest for rat %#mD, %d -> %#mD, %d\n",
        line->Point1.X, line->Point1.Y, line->group1, line->Point2.X, line->Point2.Y, line->group2);
    free(routedata);
    return NULL;
  }

  routedata->netlist = routedata->src->netlist;

  g_assert(routedata->src->netlist == routedata->dest->netlist);

  g_ptr_array_add(r->routes, routedata);
  g_ptr_array_add(routedata->netlist->routes, routedata);

  r->failednets = g_list_prepend(r->failednets, routedata);

  return routedata;
}

void
delete_route(toporouter_route_t *routedata, guint destroy)
{
  GList *i = routedata->path;
  toporouter_vertex_t *pv = NULL;
  
  while(i) {
    toporouter_vertex_t *tv = TOPOROUTER_VERTEX(i->data);

    g_assert(tv);

    if(tv && pv && !(tv->flags & VERTEX_FLAG_ROUTE) && !(pv->flags & VERTEX_FLAG_ROUTE)) {
      toporouter_edge_t *e = TOPOROUTER_EDGE(gts_vertices_are_connected(GTS_VERTEX(tv), GTS_VERTEX(pv)));

      if(e && (e->flags & EDGE_FLAG_DIRECTCONNECTION)) {
        e->flags ^= EDGE_FLAG_DIRECTCONNECTION;
      }
    }
    pv = tv;
    i = i->next;
  }

  i = routedata->path;
  while(i) {
    toporouter_vertex_t *tv = TOPOROUTER_VERTEX(i->data);

    tv->parent = NULL;
    tv->child = NULL;

    if(tv->routingedge) {
      if(TOPOROUTER_IS_CONSTRAINT(tv->routingedge)) 
        TOPOROUTER_CONSTRAINT(tv->routingedge)->routing = g_list_remove(TOPOROUTER_CONSTRAINT(tv->routingedge)->routing, tv);
      else
        tv->routingedge->routing = g_list_remove(tv->routingedge->routing, tv);
      if(destroy) gts_object_destroy ( GTS_OBJECT(tv) );
    }

    i = i->next;
  }

  if(routedata->path) g_list_free(routedata->path);
  routedata->path = NULL;
  routedata->curpoint = NULL;
  routedata->score = INFINITY;
  routedata->alltemppoints = NULL;
}

/*!
 * \brief Remove route can be later reapplied.
 */
void
remove_route(GList *path)
{
  GList *i = path;

  while(i) {
    toporouter_vertex_t *tv = TOPOROUTER_VERTEX(i->data);
    
    tv->parent = NULL;
    tv->child = NULL;

//    if(tv->flags & VERTEX_FLAG_ROUTE) g_assert(tv->route == routedata);

    if(tv->routingedge) {

      if(TOPOROUTER_IS_CONSTRAINT(tv->routingedge)) 
        TOPOROUTER_CONSTRAINT(tv->routingedge)->routing = g_list_remove(TOPOROUTER_CONSTRAINT(tv->routingedge)->routing, tv);
      else
        tv->routingedge->routing = g_list_remove(tv->routingedge->routing, tv);
    }
    i = i->next;
  }

}

gint
apply_route(GList *path, toporouter_route_t *routedata)
{
  GList *i = path;
  toporouter_vertex_t *pv = NULL;
  gint count = 0;

  if(!path) return 0;
//  g_assert(path);

  while(i) {
    toporouter_vertex_t *tv = TOPOROUTER_VERTEX(i->data);

    if(tv->routingedge) {
      if(TOPOROUTER_IS_CONSTRAINT(tv->routingedge)) 
        TOPOROUTER_CONSTRAINT(tv->routingedge)->routing = g_list_insert_sorted_with_data(
              TOPOROUTER_CONSTRAINT(tv->routingedge)->routing, 
              tv, 
              routing_edge_insert, 
              tv->routingedge);
      else
        tv->routingedge->routing = g_list_insert_sorted_with_data(
              tv->routingedge->routing, 
              tv, 
              routing_edge_insert, 
              tv->routingedge);
    
      count++;
    }

    if(pv) {
      pv->child = tv;
      tv->parent = pv;
    }

    if(tv->flags & VERTEX_FLAG_ROUTE) g_assert(tv->route == routedata);

    pv = tv;
    i = i->next;
  }

  TOPOROUTER_VERTEX(path->data)->parent = NULL;
  pv->child = NULL;

  return count;
}


gint               
compare_routedata_ascending(gconstpointer a, gconstpointer b)
{
  toporouter_route_t *ra = (toporouter_route_t *)a;
  toporouter_route_t *rb = (toporouter_route_t *)b;
  return ra->score - rb->score;
}

void
print_costmatrix(gdouble *m, guint n) 
{
  printf("COST MATRIX:\n");  
  for(guint i = 0;i<n;i++) {
    for(guint j = 0;j<n;j++) {
      printf("%f ", m[(i*n)+j]);
    }
    printf("\n");
  }
}


static inline void
init_cost_matrix(gdouble *m, guint n) 
{
  for(guint i=0;i<n;i++) {
    for(guint j=0;j<n;j++) {
      m[(i*n)+j] = INFINITY;
    }
  }
}


toporouter_netscore_t *
netscore_create(toporouter_t *r, toporouter_route_t *routedata, guint n, guint id)
{
  toporouter_netscore_t *netscore = (toporouter_netscore_t *)malloc(sizeof(toporouter_netscore_t));
  GList *path = route(r, routedata, 0);
  
  netscore->id = id;

  netscore->routedata = routedata;
  routedata->detourscore = netscore->score = routedata->score;

  if(!isfinite(routedata->detourscore)) {
    printf("WARNING: !isfinite(detourscore)\n");
    print_cluster(routedata->src);
    print_cluster(routedata->dest);
    return NULL;
  }

  netscore->pairwise_nodetour = (guint *)malloc(n * sizeof(guint));

  for(guint i=0;i<n;i++) {
    netscore->pairwise_nodetour[i] = 0;
  }

  netscore->pairwise_detour_sum = 0.;
  netscore->pairwise_fails = 0;

  netscore->r = r;

  if(path) {
    routedata->topopath = g_list_copy(routedata->path);
    delete_route(routedata, 0);
  }

  return netscore;
}

static inline void
netscore_destroy(toporouter_netscore_t *netscore)
{
  free(netscore->pairwise_nodetour);
  free(netscore);
}

void
print_netscores(GPtrArray* netscores)
{
  printf("NETSCORES: \n\n");
  printf("     %15s %15s %15s\n----------------------------------------------------\n", "Score", "Detour Sum", "Pairwise Fails");

  for(toporouter_netscore_t **i = (toporouter_netscore_t **) netscores->pdata; i < (toporouter_netscore_t **) netscores->pdata + netscores->len; i++) {
#ifdef DEBUG_NETSCORES    
	printf("%4d %15f %15f %15d %15x\n", (*i)->id, (*i)->score, (*i)->pairwise_detour_sum, (*i)->pairwise_fails, (guint)*i);
#endif 
  }

  printf("\n");
}

void
netscore_pairwise_calculation(toporouter_netscore_t *netscore, GPtrArray *netscores)
{
  toporouter_netscore_t **netscores_base = (toporouter_netscore_t **) (netscores->pdata); 
  toporouter_route_t *temproutedata = routedata_create();

  //route(netscore->r, netscore->routedata, 0);
  apply_route(netscore->routedata->topopath, netscore->routedata);

  for(toporouter_netscore_t **i = netscores_base; i < netscores_base + netscores->len; i++) {
        
    if(!netscore->pairwise_nodetour[i-netscores_base] && *i != netscore && (*i)->routedata->netlist != netscore->routedata->netlist) {
      
      temproutedata->src = (*i)->routedata->src;
      temproutedata->dest = (*i)->routedata->dest;
  
      route(netscore->r, temproutedata, 0);

      if(temproutedata->score == (*i)->score) {
        netscore->pairwise_nodetour[i-netscores_base] = 1;
        (*i)->pairwise_nodetour[netscore->id] = 1;
      }else 
      if(!isfinite(temproutedata->score)) {
        netscore->pairwise_fails += 1;
      }else{
        netscore->pairwise_detour_sum += temproutedata->score - (*i)->score;
      }

      delete_route(temproutedata, 1);
    }
    
  }

//  delete_route(netscore->routedata, 1);
  remove_route(netscore->routedata->topopath);

  free(temproutedata);
}

gint
netscore_pairwise_size_compare(toporouter_netscore_t **a, toporouter_netscore_t **b)
{
  // infinite scores are last
  if(!isfinite((*a)->score) && !isfinite((*b)->score)) return 0;
  if(isfinite((*a)->score) && !isfinite((*b)->score)) return -1;
  if(isfinite((*b)->score) && !isfinite((*a)->score)) return 1;

  // order by pairwise fails
  if((*a)->pairwise_fails < (*b)->pairwise_fails) return -1;
  if((*b)->pairwise_fails < (*a)->pairwise_fails) return 1;

  // order by pairwise detour
  if((*a)->pairwise_detour_sum < (*b)->pairwise_detour_sum) return -1;
  if((*b)->pairwise_detour_sum < (*a)->pairwise_detour_sum) return 1;

  // order by score
  if((*a)->score < (*b)->score) return -1;
  if((*b)->score < (*a)->score) return 1;

  return 0;
}

gint
netscore_pairwise_compare(toporouter_netscore_t **a, toporouter_netscore_t **b)
{
  // infinite scores are last
  if(!isfinite((*a)->score) && !isfinite((*b)->score)) return 0;
  if(isfinite((*a)->score) && !isfinite((*b)->score)) return -1;
  if(isfinite((*b)->score) && !isfinite((*a)->score)) return 1;

  // order by pairwise fails
  if((*a)->pairwise_fails < (*b)->pairwise_fails) return -1;
  if((*b)->pairwise_fails < (*a)->pairwise_fails) return 1;

  // order by pairwise detour
  if((*a)->pairwise_detour_sum < (*b)->pairwise_detour_sum) return -1;
  if((*b)->pairwise_detour_sum < (*a)->pairwise_detour_sum) return 1;

  return 0;
}

guint
order_nets_preroute_greedy(toporouter_t *r, GList *nets, GList **rnets) 
{
  gint len = g_list_length(nets);
  GPtrArray* netscores = g_ptr_array_sized_new(len);
  guint failcount = 0;

  while(nets) {
    toporouter_netscore_t *ns = netscore_create(r, TOPOROUTER_ROUTE(nets->data), len, failcount++);
    if(ns) g_ptr_array_add(netscores, ns);
    nets = nets->next;
  } 

  failcount = 0;
  
  g_ptr_array_foreach(netscores, (GFunc) netscore_pairwise_calculation, netscores);
  
  g_ptr_array_sort(netscores, (GCompareFunc) r->netsort);

#ifdef DEBUG_ORDERING
  print_netscores(netscores);
#endif

  *rnets = NULL;
  FOREACH_NETSCORE(netscores) {
    *rnets = g_list_prepend(*rnets, netscore->routedata);
    if(!isfinite(netscore->score)) failcount++;
    netscore_destroy(netscore);
  } FOREACH_END;

  g_ptr_array_free(netscores, TRUE);

  return failcount;
}

toporouter_vertex_t *
edge_closest_vertex(toporouter_edge_t *e, toporouter_vertex_t *v)
{
  GList *i = v->routingedge ? edge_routing(v->routingedge) : NULL;
  gdouble closestd = 0.;
  toporouter_vertex_t *closestv = NULL;

  while(i) {
    toporouter_vertex_t *ev = TOPOROUTER_VERTEX(i->data);
    gdouble tempd = gts_point_distance2(GTS_POINT(ev), GTS_POINT(v));

    if(!closestv || (tempd < closestd)) {
      closestd = tempd;
      closestv = ev;
    }

    i = i->next;
  }

  return closestv;
}

void
snapshot(toporouter_t *r, char *name, GList *datas) 
{
///*
  {
    int i;
    for(i=0;i<groupcount();i++) {
      char buffer[256];
      sprintf(buffer, "route-%s-%d.png", name, i);
      toporouter_draw_surface(r, r->layers[i].surface, buffer, 2048, 2048, 2, datas, i, NULL);
    }
  }
//*/

}
/*
gdouble
route_conflict(toporouter_t *r, toporouter_route_t *route, guint *n)
{
  GList *i = route->path;
  toporouter_vertex_t *pv = NULL;
  gdouble cost = 0.;

  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);
    if(pv && vz(v) == vz(pv))
      cost += vertices_routing_conflict_cost(r, v, pv, n);
    pv = v;
    i = i->next;
  }

  return cost;
}
*/
GList *
route_conflicts(toporouter_route_t *route)
{
  GList *conflicts = NULL, *i = route->path;
  toporouter_vertex_t *pv = NULL;

  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);

    if(pv && vz(pv) == vz(v)) {
      GList *temp = vertices_routing_conflicts(pv, v), *j;
     
      j = temp;
      while(j) {
        toporouter_route_t *conroute = TOPOROUTER_ROUTE(j->data);
        if(!g_list_find(conflicts, conroute)) 
          conflicts = g_list_prepend(conflicts, conroute);
        j = j->next;
      }

      if(temp) g_list_free(temp);
    }

    pv = v;
    i = i->next;
  }
  return conflicts;
}

gint       
spread_edge(gpointer item, gpointer data)
{
  toporouter_edge_t *e = TOPOROUTER_EDGE(item);
  toporouter_vertex_t *v;
  gdouble spacing, s;
  GList *i;

  if(TOPOROUTER_IS_CONSTRAINT(e)) return 0;

  i = edge_routing(e);

  if(!g_list_length(i)) return 0;

  if(g_list_length(i) == 1) {
    v = TOPOROUTER_VERTEX(i->data);
    GTS_POINT(v)->x = (vx(edge_v1(e)) + vx(edge_v2(e))) / 2.;
    GTS_POINT(v)->y = (vy(edge_v1(e)) + vy(edge_v2(e))) / 2.;
    return 0;
  }
  
  s = spacing = (gts_point_distance(GTS_POINT(edge_v1(e)), GTS_POINT(edge_v2(e))) ) / (g_list_length(i) + 1);
  
  while(i) {
    v = TOPOROUTER_VERTEX(i->data); 
    vertex_move_towards_vertex_values(edge_v1(e), edge_v2(e), s, &(GTS_POINT(v)->x), &(GTS_POINT(v)->y));

    s += spacing;
    i = i->next;
  }

  return 0;  
}

void
route_checkpoint(toporouter_route_t *route, toporouter_route_t *temproute)
{
  GList *i = g_list_last(route->path);
  gint n = g_list_length(route->path);

  if(route->ppathindices) free(route->ppathindices);
  route->ppathindices = (gint *)malloc(sizeof(gint)*n);

//  n = 0;
  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);
    n--;

    if(v->routingedge) {
      GList *j = g_list_find(edge_routing(v->routingedge), v)->prev;
      gint tempindex = g_list_index(edge_routing(v->routingedge), v);

      while(j) {
        if(TOPOROUTER_VERTEX(j->data)->route == temproute) tempindex--;
        j = j->prev;
      }

      route->ppathindices[n] = tempindex; 

      if(TOPOROUTER_IS_CONSTRAINT(v->routingedge)) 
        TOPOROUTER_CONSTRAINT(v->routingedge)->routing = g_list_remove(TOPOROUTER_CONSTRAINT(v->routingedge)->routing, v);
      else
        v->routingedge->routing = g_list_remove(v->routingedge->routing, v);
    }

    i = i->prev;
  }

  route->pscore = route->score;
  route->ppath = route->path;
  remove_route(route->path);      
  route->path = NULL;
  route->psrc = route->src;
  route->pdest = route->dest;
//route->src->pc = route->src->c;
//route->dest->pc = route->dest->c;
}

void
route_restore(toporouter_route_t *route)
{
  GList *i;
  toporouter_vertex_t *pv = NULL;
  gint n = 0;

  g_assert(route->ppath);
  g_assert(route->ppathindices);

  route->path = route->ppath;
  i = route->ppath;
  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);

    if(v->routingedge) {
      if(TOPOROUTER_IS_CONSTRAINT(v->routingedge)) 
        TOPOROUTER_CONSTRAINT(v->routingedge)->routing = g_list_insert(TOPOROUTER_CONSTRAINT(v->routingedge)->routing, v, route->ppathindices[n]);
      else
        v->routingedge->routing = g_list_insert(v->routingedge->routing, v, route->ppathindices[n]);

      //      space_edge(v->routingedge, NULL);
    }

    if(pv) {
      pv->child = v;
      v->parent = pv;
    }

    n++;
    pv = v;
    i = i->next;
  }

  route->score = route->pscore;
  route->src = route->psrc;
  route->dest = route->pdest;
//route->src->c = route->src->pc;
//route->dest->c = route->dest->pc;

}

void
cluster_merge(toporouter_route_t *routedata)
{
  gint oldc = routedata->dest->c, newc = routedata->src->c;

  FOREACH_CLUSTER(routedata->netlist->clusters) {
    if(cluster->c == oldc) 
      cluster->c = newc;
  } FOREACH_END;

}

void
netlist_recalculate(toporouter_netlist_t *netlist, GList *ignore)
{
  GList *i = g_list_last(netlist->routed);
  gint n = netlist->clusters->len-1;

  FOREACH_CLUSTER(netlist->clusters) {
    cluster->c = n--;
  } FOREACH_END;

  while(i) {
    if(!ignore || !g_list_find(ignore, i->data)) cluster_merge(TOPOROUTER_ROUTE(i->data));
    i = i->prev;
  }

}

void
netlists_recalculate(GList *netlists, GList *ignore)
{
  GList *i = netlists;
  while(i) {
    netlist_recalculate(TOPOROUTER_NETLIST(i->data), ignore);
    i = i->next;
  }
}

void
netlists_rollback(GList *netlists) 
{
//  netlists_recalculate(netlists, NULL);
  while(netlists) {
    toporouter_netlist_t *netlist = TOPOROUTER_NETLIST(netlists->data);
   
    FOREACH_CLUSTER(netlist->clusters) {
      cluster->c = cluster->pc;
    } FOREACH_END;

    netlists = netlists->next;
  }
}

void
print_netlist(toporouter_netlist_t *netlist)
{

  printf("NETLIST %s: ", netlist->netlist);

  FOREACH_CLUSTER(netlist->clusters) {
    printf("%d ", cluster->c);

  } FOREACH_END;
  printf("\n");
}

#define REMOVE_ROUTING(x) x->netlist->routed = g_list_remove(x->netlist->routed, x); \
  r->routednets = g_list_remove(r->routednets, x); \
  r->failednets = g_list_prepend(r->failednets, x)

#define INSERT_ROUTING(x) x->netlist->routed = g_list_prepend(x->netlist->routed, x); \
  r->routednets = g_list_prepend(r->routednets, x); \
  r->failednets = g_list_remove(r->failednets, x)

gint
roar_route(toporouter_t *r, toporouter_route_t *routedata, gint threshold)
{
  gint intfails = 0;
  GList *netlists = NULL, *routed = NULL;
  
  g_assert(!routedata->path);

  if(routedata->src->c == routedata->dest->c) {
    printf("ERROR: attempt to route already complete route\n");
    g_assert(routedata->src->c != routedata->dest->c);
  }

  routedata->src->pc = routedata->src->c;
  routedata->dest->pc = routedata->dest->c;
  routedata->psrc = routedata->src;
  routedata->pdest = routedata->dest;

  r->flags |= TOPOROUTER_FLAG_LEASTINVALID;
  if(route(r, routedata, 0)) {
    GList *conflicts, *j;

    INSERT_ROUTING(routedata);

    conflicts = route_conflicts(routedata);
    cluster_merge(routedata);

    r->flags &= ~TOPOROUTER_FLAG_LEASTINVALID;

    j = conflicts;
    while(j) {
      toporouter_route_t *conflict = TOPOROUTER_ROUTE(j->data);
      if(!g_list_find(netlists, conflict->netlist)) 
        netlists = g_list_prepend(netlists, conflict->netlist);
      
      route_checkpoint(conflict, routedata);

      REMOVE_ROUTING(conflict);
      j = j->next;
    }

    netlists = g_list_prepend(netlists, routedata->netlist);
    netlists_recalculate(netlists, NULL);

    j = conflicts;
    while(j) {
      toporouter_route_t *conflict = TOPOROUTER_ROUTE(j->data);
      g_assert(conflict->src->c != conflict->dest->c);
      if(route(r, conflict, 0)) { 
        cluster_merge(conflict);
        
        routed = g_list_prepend(routed, conflict);
      
        INSERT_ROUTING(conflict);

        netlist_recalculate(conflict->netlist, NULL);
        
      }else{
        if(++intfails >= threshold) {
          GList *i = routed;
          while(i) {
            toporouter_route_t *intconflict = TOPOROUTER_ROUTE(i->data);
            REMOVE_ROUTING(intconflict);
            delete_route(intconflict, 1);
            i = i->next;
          }
          delete_route(routedata, 1);
          i = g_list_last(conflicts);
          while(i) {
            toporouter_route_t *intconflict = TOPOROUTER_ROUTE(i->data);
            
            route_restore(intconflict);
            INSERT_ROUTING(intconflict);

            i = i->prev;
          }
          REMOVE_ROUTING(routedata);
          intfails = 0;
          netlists_recalculate(netlists, NULL);
          goto roar_route_end;
        }

      }
      j = j->next;
    }


    netlists_recalculate(netlists, NULL);

    intfails--;
roar_route_end:    
    g_list_free(conflicts);
    g_list_free(netlists);

  }else{
    r->flags &= ~TOPOROUTER_FLAG_LEASTINVALID;
  }

  g_list_free(routed);
  return intfails;
}

gint
roar_router(toporouter_t *r, gint failcount, gint threshold)
{
  gint pfailcount = failcount +1;

  Message(_("ROAR router: "));
  for(guint j=0;j<6;j++) {
    GList *failed = g_list_copy(r->failednets), *k = failed;

    k = failed;
    while(k) {
      failcount += roar_route(r, TOPOROUTER_ROUTE(k->data), threshold);
      k = k->next;
    }
    g_list_free(failed);
    
    printf("\tROAR pass %d - %d routed -  %d failed\n", j, g_list_length(r->routednets), g_list_length(r->failednets));

    if(!failcount || failcount >= pfailcount) {
      Message(_("%d nets remaining\n"), failcount);
      break;
    }
    Message(_("%d -> "), failcount);
    pfailcount = failcount;
  }

  return failcount;
}

gint
route_detour_compare(toporouter_route_t **a, toporouter_route_t **b)
{ return ((*b)->score - (*b)->detourscore) - ((*a)->score - (*a)->detourscore); }



void
roar_detour_route(toporouter_t *r, toporouter_route_t *data)
{
  gdouble pscore = data->score, nscore = 0.; 
  GList *netlists = NULL;

  route_checkpoint(data, NULL);

  REMOVE_ROUTING(data);

  netlists = g_list_prepend(NULL, data->netlist);
  netlists_recalculate(netlists, NULL);

  r->flags |= TOPOROUTER_FLAG_LEASTINVALID;
  if(route(r, data, 0)) {
    GList *conflicts, *j;  

    nscore = data->score;
    conflicts = route_conflicts(data);

    INSERT_ROUTING(data);

    r->flags &= ~TOPOROUTER_FLAG_LEASTINVALID;

    j = conflicts;
    while(j) {
      toporouter_route_t *conflict = TOPOROUTER_ROUTE(j->data);
      
      if(!g_list_find(netlists, conflict->netlist)) 
        netlists = g_list_prepend(netlists, conflict->netlist);
      pscore += conflict->score;
      
      route_checkpoint(conflict, NULL);
      REMOVE_ROUTING(conflict);

      j = j->next;
    }
    netlists_recalculate(netlists, NULL);
    
    j = conflicts;
    while(j) {
      toporouter_route_t *conflict = TOPOROUTER_ROUTE(j->data);

      if(route(r, conflict, 0)) { 
        cluster_merge(conflict);
        INSERT_ROUTING(conflict);
        nscore += conflict->score;
      }else{
        j = j->prev;
        goto roar_detour_route_rollback_int;
      }
      j = j->next;
    }

    if(nscore > pscore) {
      j = g_list_last(conflicts);
roar_detour_route_rollback_int:     
      REMOVE_ROUTING(data);

      while(j) {
        toporouter_route_t *conflict = TOPOROUTER_ROUTE(j->data);
        REMOVE_ROUTING(conflict);
        delete_route(conflict, 1);
        j = j->prev;
      }

      j = g_list_last(conflicts);
      while(j) {
        toporouter_route_t *conflict = TOPOROUTER_ROUTE(j->data);
        route_restore(conflict);
        INSERT_ROUTING(conflict);
        j = j->prev;
      }
      delete_route(data, 1);

      goto roar_detour_route_rollback_exit;

    }

    g_list_free(conflicts);
  }else{
    r->flags &= ~TOPOROUTER_FLAG_LEASTINVALID;
roar_detour_route_rollback_exit:    
    route_restore(data);
    INSERT_ROUTING(data);
  }
  netlists_recalculate(netlists, NULL);

  g_list_free(netlists);
}

void
detour_router(toporouter_t *r) 
{
  GList *i = r->routednets;
  guint n = g_list_length(r->routednets);
  GPtrArray* scores =  g_ptr_array_sized_new(n);

  while(i) {
    toporouter_route_t *curroute = TOPOROUTER_ROUTE(i->data);
    curroute->score = path_score(r, curroute->path);
    g_ptr_array_add(scores, i->data);
    i = i->next;
  }
  
  g_ptr_array_sort(scores, (GCompareFunc) route_detour_compare);

  r->flags |= TOPOROUTER_FLAG_DETOUR;

  for(toporouter_route_t **i = (toporouter_route_t **) scores->pdata; i < (toporouter_route_t **) scores->pdata + scores->len; i++) {
    toporouter_route_t *curroute = (*i);
   
    if(isfinite(curroute->score) && isfinite(curroute->detourscore)) {
//    printf("%15s %15f \t %8f,%8f - %8f,%8f\n", (*i)->src->netlist + 2, (*i)->score - (*i)->detourscore,
//        vx(curroute->mergebox1->point), vy(curroute->mergebox1->point),
//        vx(curroute->mergebox2->point), vy(curroute->mergebox2->point));

      if(curroute->score - curroute->detourscore > ROAR_DETOUR_THRESHOLD) {
        roar_detour_route(r, curroute);
      }else break;

    }
  }
  printf("\n");
  
  r->flags ^= TOPOROUTER_FLAG_DETOUR;

  g_ptr_array_free(scores, TRUE);

}

gint
rubix_router(toporouter_t *r, gint failcount)
{
  GList *i, *ordering;
  order_nets_preroute_greedy(r, r->failednets, &ordering); 

  i = ordering;
  while(i) {
    toporouter_route_t *data = TOPOROUTER_ROUTE(i->data); 
    
    if(route(r, data, 0)) { 
      INSERT_ROUTING(data);
      cluster_merge(data);
      failcount--;
    }

    i = i->next;
  } 

  g_list_free(ordering);

  return failcount;
}

guint
hybrid_router(toporouter_t *r)
{
  gint failcount = g_list_length(r->failednets);
  r->flags |= TOPOROUTER_FLAG_AFTERORDER;
  r->flags |= TOPOROUTER_FLAG_AFTERRUBIX;
  failcount = rubix_router(r, failcount);

  Message(_("RUBIX router: %d nets remaining\n"), failcount);
  printf(_("RUBIX router: %d nets remaining\n"), failcount);

  r->flags |= TOPOROUTER_FLAG_GOFAR;
  
  for(guint i=0;i<6 && failcount;i++) {
    if(i % 2 == 1) { 
      failcount = roar_router(r, failcount, 5);
  //    printf("THRESH 5\n");
    }else{
      failcount = roar_router(r, failcount, 2);
  //    printf("THRESH 2\n");
    }

    detour_router(r);
  }

  failcount = roar_router(r, failcount, 2);
  detour_router(r);
  
  return failcount;
}

void
parse_arguments(toporouter_t *r, int argc, char **argv) 
{
  int i, tempint;
  for(i=0;i<argc;i++) {
    if(sscanf(argv[i], "viacost=%d", &tempint)) {
      /* XXX: We should be using PCB's generic value with unit parsing here */
      r->viacost = (double)tempint;
    }else if(sscanf(argv[i], "l%d", &tempint)) {
      gdouble *layer = (gdouble *)malloc(sizeof(gdouble));
      *layer = (double)tempint;
      r->keepoutlayers = g_list_prepend(r->keepoutlayers, layer);
    }
  }
  
  for (guint group = 0; group < max_group; group++)
    for (i = 0; i < PCB->LayerGroups.Number[group]; i++) 
      if ((PCB->LayerGroups.Entries[group][i] < max_copper_layer) && !(PCB->Data->Layer[PCB->LayerGroups.Entries[group][i]].On)) {
        gdouble *layer = (gdouble *)malloc(sizeof(gdouble));
        *layer = (double)group;
        r->keepoutlayers = g_list_prepend(r->keepoutlayers, layer);
      }

}

toporouter_t *
toporouter_new(void) 
{
  toporouter_t *r = (toporouter_t *)calloc(1, sizeof(toporouter_t));
  time_t ltime; 

  gettimeofday(&r->starttime, NULL);  

  r->netsort = netscore_pairwise_compare;

  r->destboxes = NULL;
  r->consumeddestboxes = NULL;

  r->paths = NULL;

  r->layers = NULL;
  r->flags = 0;
  r->viacost = VIA_COST_AS_DISTANCE;

  r->wiring_score = 0.;

  r->bboxes = NULL;
  r->bboxtree = NULL;

  r->netlists = g_ptr_array_new();
  r->routes = g_ptr_array_new();

  r->keepoutlayers = NULL;

  r->routednets = NULL;
  r->failednets = NULL;

  ltime=time(NULL); 

  gts_predicates_init();

  Message(_("Topological Autorouter\n"));
  Message(_("Started %s"),asctime(localtime(&ltime)));  
  Message(_("-------------------------------------\n"));
  return r;
}

void
acquire_twonets(toporouter_t *r)
{
  RAT_LOOP(PCB->Data);
    if( TEST_FLAG(SELECTEDFLAG, line) ) import_route(r, line);
  END_LOOP;
//  /*
  if(!r->routes->len) {
    RAT_LOOP(PCB->Data);
      import_route(r, line);
    END_LOOP;
  }
//     */    

}

toporouter_netlist_t *
find_netlist_by_name(toporouter_t *r, char *name)
{
  FOREACH_NETLIST(r->netlists) {
    if(!strcmp(netlist->netlist, name)) return netlist;
  } FOREACH_END;
  return NULL;
}

gint 
toporouter_set_pair(toporouter_t *r, toporouter_netlist_t *n1, toporouter_netlist_t *n2)
{
  if(!n1 || !n2) return 0;
  n1->pair = n2;
  n2->pair = n1;
  return 1;
}

static int 
toporouter (int argc, char **argv, Coord x, Coord y)
{
  toporouter_t *r = toporouter_new();
  parse_arguments(r, argc, argv);
  import_geometry(r);
  acquire_twonets(r);

//if(!toporouter_set_pair(r, find_netlist_by_name(r, "  DRAM_DQS_N"), find_netlist_by_name(r, "  DRAM_DQS"))) {
//  printf("Couldn't associate pair\n");
//}

  hybrid_router(r);
/*
  for(gint i=0;i<groupcount();i++) {
   gts_surface_foreach_edge(r->layers[i].surface, space_edge, NULL);
  }
  {
    int i;
    for(i=0;i<groupcount();i++) {
      char buffer[256];
      sprintf(buffer, "route%d.png", i);
      toporouter_draw_surface(r, r->layers[i].surface, buffer, 1024, 1024, 2, NULL, i, NULL);
    }
  }
*/
  toporouter_export(r);
  toporouter_free(r);
  
  SaveUndoSerialNumber ();
  DeleteRats (false);
  RestoreUndoSerialNumber ();
  AddAllRats (false, NULL);
  RestoreUndoSerialNumber ();
  IncrementUndoSerialNumber ();
  Redraw ();

  return 0;
}

static int 
escape (int argc, char **argv, Coord x, Coord y)
{
  guint dir, viax, viay;
  gdouble pitch, dx, dy;

  if(argc != 1) return 0;

  dir = atoi(argv[0]);


  ALLPAD_LOOP(PCB->Data);
  {
    if( TEST_FLAG(SELECTEDFLAG, pad) ) {
      PinType *via;
      LineType *line;

      PadType *pad0 = element->Pad->data;
      PadType *pad1 = g_list_next (element->Pad)->data;

      pitch = hypot (pad0->Point1.X - pad1->Point1.X, pad0->Point1.Y - pad1->Point1.Y);

      dx = pitch / 2;
      dy = pitch / 2;

      switch(dir) {
        case 1:
          viax = pad->Point1.X - dx;
          viay = pad->Point1.Y + dy;
          break;
        case 3:
          viax = pad->Point1.X + dx;
          viay = pad->Point1.Y + dy;
          break;
        case 9:
          viax = pad->Point1.X + dx;
          viay = pad->Point1.Y - dy;
          break;
        case 7:
          viax = pad->Point1.X - dx;
          viay = pad->Point1.Y - dy;
          break;
        case 2:
          viax = pad->Point1.X;
          viay = pad->Point1.Y + dy;
          break;
        case 8:
          viax = pad->Point1.X;
          viay = pad->Point1.Y - dy;
          break;
        case 4:
          viax = pad->Point1.X - dx;
          viay = pad->Point1.Y;
          break;
        case 6:
          viax = pad->Point1.X + dx;
          viay = pad->Point1.Y;
          break;
        default:
          printf("ERROR: escape() with bad direction (%d)\n", dir);
          return 1;
      }
      
      if ((via = CreateNewVia (PCB->Data, viax, viay,
              Settings.ViaThickness, 2 * Settings.Keepaway,
              0, Settings.ViaDrillingHole, NULL,
              NoFlags ())) != NULL)
      {
        AddObjectToCreateUndoList (VIA_TYPE, via, via, via);
//        if (gui->shift_is_pressed ())
//          ChangeObjectThermal (VIA_TYPE, via, via, via, PCB->ThermStyle);
        DrawVia (via);
        if((line = CreateDrawnLineOnLayer (CURRENT, pad->Point1.X + 1., pad->Point1.Y + 1., viax + 1., viay + 1.,
                Settings.LineThickness, 2 * Settings.Keepaway,
                NoFlags())))        
        {

          AddObjectToCreateUndoList (LINE_TYPE, CURRENT, line, line);
          DrawLine (CURRENT, line);

        }
      
      }
   
    }
  }
  END_LOOP;
  END_LOOP;

  IncrementUndoSerialNumber ();
  Draw ();
  return 0;
}

static HID_Action toporouter_action_list[] = {
  {"Escape", N_("Select a set of pads"), escape,
    N_("Pad escape"), N_("Escape()")},
  {"Toporouter", N_("Select net(s)"), toporouter,
    N_("Topological autorouter"), N_("Toporouter()")}
};

REGISTER_ACTIONS (toporouter_action_list)

void hid_toporouter_init()
{
  register_toporouter_action_list();
}

