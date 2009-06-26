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
 *
 *
 *                 This is *EXPERIMENTAL* code.
 *
 *  As the code is experimental, the algorithms and code
 *  are likely to change. Which means it isn't documented  
 *  or optimized. If you would like to learn about Topological
 *  Autorouters, the following papers are good starting points: 
 *
 * This file implements a topological autorouter, and uses techniques from the
 * following publications:
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
 *
 */


#include "toporouter.h"

static void 
toporouter_edge_init (toporouter_edge_t *edge)
{
  edge->routing = NULL;
  edge->flags = 0;
}

toporouter_edge_class_t * 
toporouter_edge_class(void)
{
  static toporouter_edge_class_t *class = NULL;

  if (class == NULL) {
    GtsObjectClassInfo constraint_info = {
      "toporouter_edge_t",
      sizeof (toporouter_edge_t),
      sizeof (toporouter_edge_class_t),
      (GtsObjectClassInitFunc) NULL,
      (GtsObjectInitFunc) toporouter_edge_init,
      (GtsArgSetFunc) NULL,
      (GtsArgGetFunc) NULL
    };
    class = gts_object_class_new (GTS_OBJECT_CLASS (gts_edge_class ()), &constraint_info);
  }

  return class;
}

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
  static toporouter_bbox_class_t *class = NULL;

  if (class == NULL) {
    GtsObjectClassInfo constraint_info = {
      "toporouter_bbox_t",
      sizeof (toporouter_bbox_t),
      sizeof (toporouter_bbox_class_t),
      (GtsObjectClassInitFunc) NULL,
      (GtsObjectInitFunc) toporouter_bbox_init,
      (GtsArgSetFunc) NULL,
      (GtsArgGetFunc) NULL
    };
    class = gts_object_class_new (GTS_OBJECT_CLASS (gts_bbox_class ()), &constraint_info);
  }

  return class;
}

static void 
toporouter_vertex_class_init (toporouter_vertex_class_t *klass)
{

}

static void 
toporouter_vertex_init (toporouter_vertex_t *vertex)
{
  vertex->bbox = NULL;
  vertex->parent = NULL;
  vertex->child = NULL;
  vertex->pullx = INFINITY;
  vertex->pully = INFINITY;
  vertex->flags = 0;
  vertex->routingedge = NULL;
  vertex->arc = NULL;
  vertex->oproute = NULL;
  vertex->route = NULL;
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
    klass = gts_object_class_new (GTS_OBJECT_CLASS (gts_vertex_class ()), &constraint_info);
  }

  return klass;
}

static void 
toporouter_constraint_class_init (toporouter_constraint_class_t *klass)
{

}

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
    klass = gts_object_class_new (GTS_OBJECT_CLASS (gts_constraint_class ()), &constraint_info);
  }

  return klass;
}

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
    klass = gts_object_class_new (GTS_OBJECT_CLASS (gts_constraint_class ()), &constraint_info);
  }

  return klass;
}

#define MARGIN 10.0f

drawing_context_t *
toporouter_output_init(int w, int h, char *filename) 
{
  drawing_context_t *dc;

  dc = malloc(sizeof(drawing_context_t));

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

void
toporouter_output_close(drawing_context_t *dc) 
{
#if TOPO_OUTPUT_ENABLED
  cairo_surface_write_to_png (dc->surface, dc->filename);
  cairo_destroy (dc->cr);
  cairo_surface_destroy (dc->surface);
#endif
}

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

inline gdouble
cluster_keepaway(toporouter_cluster_t *cluster) 
{
  if(cluster) return lookup_keepaway(cluster->style);
  return lookup_keepaway(NULL);
}

inline gdouble
cluster_thickness(toporouter_cluster_t *cluster) 
{
  if(cluster) return lookup_thickness(cluster->style);
  return lookup_thickness(NULL);
}

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
          500. * dc->s, 0, 2 * M_PI);
      cairo_fill(dc->cr);

    }else if(tv->flags & VERTEX_FLAG_GREEN) {
      cairo_set_source_rgba(dc->cr, 0., 1., 0., 0.8f);
      cairo_arc(dc->cr, 
          tv->v.p.x * dc->s + MARGIN, 
          tv->v.p.y * dc->s + MARGIN, 
          500. * dc->s, 0, 2 * M_PI);
      cairo_fill(dc->cr);
    
    }else if(tv->flags & VERTEX_FLAG_BLUE) {
      cairo_set_source_rgba(dc->cr, 0., 0., 1., 0.8f);
      cairo_arc(dc->cr, 
          tv->v.p.x * dc->s + MARGIN, 
          tv->v.p.y * dc->s + MARGIN, 
          500. * dc->s, 0, 2 * M_PI);
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
                400. * dc->s, 0, 2 * M_PI);
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
            500. * dc->s, 0, 2 * M_PI);
        cairo_fill(dc->cr);
      }else if(tv->flags & VERTEX_FLAG_RED) {
        cairo_set_source_rgba(dc->cr, 1., 0., 0., 0.8f);
        cairo_arc(dc->cr, 
            tv->v.p.x * dc->s + MARGIN, 
            tv->v.p.y * dc->s + MARGIN, 
            500. * dc->s, 0, 2 * M_PI);
        cairo_fill(dc->cr);

      }else if(tv->flags & VERTEX_FLAG_GREEN) {
        cairo_set_source_rgba(dc->cr, 0., 1., 0., 0.8f);
        cairo_arc(dc->cr, 
            tv->v.p.x * dc->s + MARGIN, 
            tv->v.p.y * dc->s + MARGIN, 
            500. * dc->s, 0, 2 * M_PI);
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

toporouter_bbox_t *
vertex_bbox(toporouter_vertex_t *v) 
{
/*  GList *i = v ? v->boxes : NULL;

  if(!i) return NULL;

  if(g_list_length(i) > 1) {
    printf("WARNING: vertex with multiple bboxes\n");
  }

  return TOPOROUTER_BBOX(i->data);*/
  return v ? v->bbox : NULL;
}

char *
vertex_netlist(toporouter_vertex_t *v)
{
  toporouter_bbox_t *box = vertex_bbox(v);

  if(box && box->cluster) return box->cluster->netlist;

  return NULL;
}
  
char *
constraint_netlist(toporouter_constraint_t *c)
{
  toporouter_bbox_t *box = c->box;

  if(box && box->cluster) return box->cluster->netlist;

  return NULL;
}

inline guint
epsilon_equals(gdouble a, gdouble b) 
{
  if(a > b - EPSILON && a < b + EPSILON) return 1;
  return 0;
}

void
print_bbox(toporouter_bbox_t *box) 
{
  printf("[BBOX %x ", (int) (box));
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
  printf("CLUSTER: %d]\n", box->cluster ? box->cluster->id : -1);
  
}

void
print_vertex(toporouter_vertex_t *v)
{
  if(v)
    printf("[V %f,%f,%f ", vx(v), vy(v), vz(v));
  else
    printf("[V (null) ");
  
  printf("%s ", vertex_netlist(v));

  if(v->flags & VERTEX_FLAG_TEMP) printf("TEMP ");
  if(v->flags & VERTEX_FLAG_ROUTE) printf("ROUTE ");
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
/*
void
print_trace (void)
{
  void *array[10];
  size_t size;
  char **strings;
  size_t i;

  size = backtrace (array, 10);
  strings = backtrace_symbols (array, size);

  printf ("Obtained %zd stack frames.\n", size);

  for (i = 0; i < size; i++)
    printf ("%s\n", strings[i]);

  free (strings);
}
*/
/* fills in x and y with coordinates of point from a towards b of distance d */
void
point_from_point_to_point(toporouter_vertex_t *a, toporouter_vertex_t *b, gdouble d, gdouble *x, gdouble *y)
{
  gdouble dx = vx(b) - vx(a);
  gdouble dy = vy(b) - vy(a);
  gdouble theta = atan(fabs(dy/dx));

#ifdef DEBUG_EXPORT  
  if(!finite(theta)) {
    printf("!finte(theta): a = %f,%f b = %f,%f\n", vx(a), vy(a), vx(b), vy(b));
  }
#endif

  if(!finite(theta)) {
//    print_trace();
  }
  g_assert(finite(theta));

  *x = vx(a); *y = vy(a);

  if( dx >= 0. ) {

    if( dy >= 0. ) {
      *x += d * cos(theta);
      *y += d * sin(theta);
    }else{
      *x += d * cos(theta);
      *y -= d * sin(theta);
    }

  }else{
    
    if( dy >= 0. ) {
      *x -= d * cos(theta);
      *y += d * sin(theta);
    }else{
      *x -= d * cos(theta);
      *y -= d * sin(theta);
    }

  }
}


inline gint 
coord_wind(gdouble ax, gdouble ay, gdouble bx, gdouble by, gdouble cx, gdouble cy) 
{
  gdouble rval, dx1, dx2, dy1, dy2;
  dx1 = bx - ax; dy1 = by - ay;
  dx2 = cx - bx; dy2 = cy - by;
  rval = (dx1*dy2)-(dy1*dx2);
  return (rval > EPSILON) ? 1 : ((rval < -EPSILON) ? -1 : 0);
}

/* wind_v:
 * returns 1,0,-1 for counterclockwise, collinear or clockwise, respectively.
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

inline int
vertex_wind(GtsVertex *a, GtsVertex *b, GtsVertex *c) 
{
  return point_wind(GTS_POINT(a), GTS_POINT(b), GTS_POINT(c));
}

inline int
tvertex_wind(toporouter_vertex_t  *a, toporouter_vertex_t  *b, toporouter_vertex_t  *c) 
{
  return point_wind(GTS_POINT(a), GTS_POINT(b), GTS_POINT(c));
}

int 
sloppy_point_wind(GtsPoint *a, GtsPoint *b, GtsPoint *c) 
{
  gdouble rval, dx1, dx2, dy1, dy2;
  dx1 = b->x - a->x; dy1 = b->y - a->y;
  dx2 = c->x - b->x; dy2 = c->y - b->y;
  rval = (dx1*dy2)-(dy1*dx2);
  return (rval > 10.) ? 1 : ((rval < -10.) ? -1 : 0);
}

inline int
sloppy_vertex_wind(GtsVertex *a, GtsVertex *b, GtsVertex *c) 
{
  return point_wind(GTS_POINT(a), GTS_POINT(b), GTS_POINT(c));
}

/* moves vertex v d units in the direction of vertex p */
void
coord_move_towards_coord_values(gdouble ax, gdouble ay, gdouble px, gdouble py, gdouble d, gdouble *x, gdouble *y) 
{
  gdouble dx = px - ax;
  gdouble dy = py - ay;
  gdouble theta = atan(fabs(dy/dx));


  if(!finite(theta)) {
    printf("!finite(theta) a = %f,%f p = %f,%f d = %f\n", 
        ax, ay, px, py, d);

  }

  g_assert(finite(theta));

  if( dx >= 0. ) {

    if( dy >= 0. ) {
      *x = ax + (d * cos(theta));
      *y = ay + (d * sin(theta));
    }else{
      *x = ax + (d * cos(theta));
      *y = ay - (d * sin(theta));
    }

  }else{
    
    if( dy >= 0. ) {
      *x = ax - (d * cos(theta));
      *y = ay + (d * sin(theta));
    }else{
      *x = ax - (d * cos(theta));
      *y = ay - (d * sin(theta));
    }

  }

}

/* moves vertex v d units in the direction of vertex p */
void
vertex_move_towards_point_values(GtsVertex *v, gdouble px, gdouble py, gdouble d, gdouble *x, gdouble *y) 
{
  gdouble dx = px - GTS_POINT(v)->x;
  gdouble dy = py - GTS_POINT(v)->y;
  gdouble theta = atan(fabs(dy/dx));

  g_assert(finite(theta));

  if( dx >= 0. ) {

    if( dy >= 0. ) {
      *x = GTS_POINT(v)->x + (d * cos(theta));
      *y = GTS_POINT(v)->y + (d * sin(theta));
    }else{
      *x = GTS_POINT(v)->x + (d * cos(theta));
      *y = GTS_POINT(v)->y - (d * sin(theta));
    }

  }else{
    
    if( dy >= 0. ) {
      *x = GTS_POINT(v)->x - (d * cos(theta));
      *y = GTS_POINT(v)->y + (d * sin(theta));
    }else{
      *x = GTS_POINT(v)->x - (d * cos(theta));
      *y = GTS_POINT(v)->y - (d * sin(theta));
    }

  }

}

/* moves vertex v d units in the direction of vertex p */
void
vertex_move_towards_vertex_values(GtsVertex *v, GtsVertex *p, gdouble d, gdouble *x, gdouble *y) 
{
  gdouble dx = GTS_POINT(p)->x - GTS_POINT(v)->x;
  gdouble dy = GTS_POINT(p)->y - GTS_POINT(v)->y;
  gdouble theta = atan(fabs(dy/dx));

  g_assert(finite(theta));

  if( dx >= 0. ) {

    if( dy >= 0. ) {
      *x = GTS_POINT(v)->x + (d * cos(theta));
      *y = GTS_POINT(v)->y + (d * sin(theta));
    }else{
      *x = GTS_POINT(v)->x + (d * cos(theta));
      *y = GTS_POINT(v)->y - (d * sin(theta));
    }

  }else{
    
    if( dy >= 0. ) {
      *x = GTS_POINT(v)->x - (d * cos(theta));
      *y = GTS_POINT(v)->y + (d * sin(theta));
    }else{
      *x = GTS_POINT(v)->x - (d * cos(theta));
      *y = GTS_POINT(v)->y - (d * sin(theta));
    }

  }

}

#define tv_on_layer(v,l) (l == TOPOROUTER_BBOX(TOPOROUTER_VERTEX(v)->boxes->data)->layer)

inline gdouble
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
inline gdouble
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

inline gdouble
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
  GList *i = cluster->i;

  while(i) {
    toporouter_bbox_t *box = TOPOROUTER_BBOX(i->data);

    if(box->point && vz(box->point) == layer) {
      cairo_set_source_rgba(dc->cr, red, green, blue, 0.8f);
      cairo_arc(dc->cr, vx(box->point) * dc->s + MARGIN, vy(box->point) * dc->s + MARGIN, 500. * dc->s, 0, 2 * M_PI);
      cairo_fill(dc->cr);
    }

    i = i->next;
  }
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

  i = r->paths;
  while(i) {
    GList *j = (GList *) i->data;
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

        if(tv->fakev) {

          cairo_set_source_rgba(dc->cr, 0.25, 0.5, 0.75, 0.8f);
          cairo_arc(dc->cr, 
              vx(tv->fakev) * dc->s + MARGIN, 
              vy(tv->fakev) * dc->s + MARGIN, 
              500. * dc->s, 0, 2 * M_PI);
          cairo_fill(dc->cr);
        }

        if(tv->flags & VERTEX_FLAG_RED) {
          cairo_set_source_rgba(dc->cr, 1., 0., 0., 0.8f);
          cairo_arc(dc->cr, 
              tv->v.p.x * dc->s + MARGIN, 
              tv->v.p.y * dc->s + MARGIN, 
              500. * dc->s, 0, 2 * M_PI);
          cairo_fill(dc->cr);

        }else if(tv->flags & VERTEX_FLAG_GREEN) {
          cairo_set_source_rgba(dc->cr, 0., 1., 0., 0.8f);
          cairo_arc(dc->cr, 
              tv->v.p.x * dc->s + MARGIN, 
              tv->v.p.y * dc->s + MARGIN, 
              500. * dc->s, 0, 2 * M_PI);
          cairo_fill(dc->cr);

        }else if(tv->flags & VERTEX_FLAG_BLUE) {
          cairo_set_source_rgba(dc->cr, 0., 0., 1., 0.8f);
          cairo_arc(dc->cr, 
              tv->v.p.x * dc->s + MARGIN, 
              tv->v.p.y * dc->s + MARGIN, 
              500. * dc->s, 0, 2 * M_PI);
          cairo_fill(dc->cr);

        } 
        else {

          cairo_set_source_rgba(dc->cr, 1., 1., 1., 0.8f);
          cairo_arc(dc->cr, 
              tv->v.p.x * dc->s + MARGIN, 
              tv->v.p.y * dc->s + MARGIN, 
              500. * dc->s, 0, 2 * M_PI);
          cairo_fill(dc->cr);


        }

        if(tv->routingedge) {
          gdouble tempx, tempy, nms, pms;
          GList *i = g_list_find(edge_routing(tv->routingedge), tv);
          toporouter_vertex_t *nextv, *prevv;

          nextv = edge_routing_next(tv->routingedge,i);
          prevv = edge_routing_prev(tv->routingedge,i);

          nms = min_spacing(tv,nextv);
          pms = min_spacing(tv,prevv);

          g_assert(finite(nms)); g_assert(finite(pms));

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
        if(finite(tv->pullx) && finite(tv->pully)) {
          cairo_set_source_rgba(dc->cr, 0.0f, 0.0f, 1.0f, 0.8f);
          cairo_move_to(dc->cr, vx(tv) * dc->s + MARGIN, vy(tv) * dc->s + MARGIN);
          cairo_line_to(dc->cr, tv->pullx * dc->s + MARGIN, tv->pully * dc->s + MARGIN);
          cairo_stroke(dc->cr);


        }/*
            if(tv->cdest) {
            cairo_set_source_rgba(dc->cr, 1.0f, 1.0f, 0.0f, 0.8f);
            cairo_move_to(dc->cr, vx(tv) * dc->s + MARGIN, vy(tv) * dc->s + MARGIN);
            cairo_line_to(dc->cr, vx(tv->cdest) * dc->s + MARGIN, vy(tv->cdest) * dc->s + MARGIN);
            cairo_stroke(dc->cr);


            }*/


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
              500. * dc->s, 0, 2 * M_PI);
          cairo_fill(dc->cr);
        }else if(tv->flags & VERTEX_FLAG_RED) {
          cairo_set_source_rgba(dc->cr, 1., 0., 0., 0.8f);
          cairo_arc(dc->cr, 
              tv->v.p.x * dc->s + MARGIN, 
              tv->v.p.y * dc->s + MARGIN, 
              500. * dc->s, 0, 2 * M_PI);
          cairo_fill(dc->cr);

        }else if(tv->flags & VERTEX_FLAG_GREEN) {
          cairo_set_source_rgba(dc->cr, 0., 1., 0., 0.8f);
          cairo_arc(dc->cr, 
              tv->v.p.x * dc->s + MARGIN, 
              tv->v.p.y * dc->s + MARGIN, 
              500. * dc->s, 0, 2 * M_PI);
          cairo_fill(dc->cr);
        }else{
          cairo_set_source_rgba(dc->cr, 1., 1., 1., 0.8f);
          cairo_arc(dc->cr, 
              tv->v.p.x * dc->s + MARGIN, 
              tv->v.p.y * dc->s + MARGIN, 
              500. * dc->s, 0, 2 * M_PI);
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


void
toporouter_layer_free(toporouter_layer_t *l) 
{
  g_list_free(l->vertices);
  g_list_free(l->constraints);

}

guint
groupcount()
{
  int group;
  guint count = 0;

  for (group = 0; group < max_layer; group++) {
    if(PCB->LayerGroups.Number[group] > 0) count++;
  }
  
  return count;
}

void
toporouter_free(toporouter_t *r)
{
  struct timeval endtime;  
  int secs, usecs;

  int i;
  for(i=0;i<groupcount();i++) {
    toporouter_layer_free(&r->layers[i]);
  }

  free(r->layers);  
  free(r);

  gettimeofday(&endtime, NULL);  

  secs = (int)(endtime.tv_sec - r->starttime.tv_sec);
  usecs = (int)((endtime.tv_usec - r->starttime.tv_usec) / 1000);

  if(usecs < 0) {
    secs -= 1;
    usecs += 1000;
  }

  printf("Elapsed time: %d.%02d seconds\n", secs, usecs);

}
/*
Boolean
IsPointOnPin (float X, float Y, float Radius, PinTypePtr pin)
{
  if (TEST_FLAG (SQUAREFLAG, pin))
    {
      BoxType b;
      BDimension t = pin->Thickness / 2;

      b.X1 = pin->X - t;
      b.X2 = pin->X + t;
      b.Y1 = pin->Y - t;
      b.Y2 = pin->Y + t;
      if (IsPointInBox (X, Y, &b, Radius))
	return (True);
    }
  else if (SQUARE (pin->X - X) + SQUARE (pin->Y - Y) <=
	   SQUARE (pin->Thickness / 2 + Radius))
    return (True);
  return (False);
}

struct ans_info
{
  void **ptr1, **ptr2, **ptr3;
  Boolean BackToo;
  float area;
  jmp_buf env;
  int locked;		
};

static int
pinorvia_callback (const BoxType * box, void *cl)
{
  struct ans_info *i = (struct ans_info *) cl;
  PinTypePtr pin = (PinTypePtr) box;

  if (TEST_FLAG (i->locked, pin))
    return 0;

  if (!IsPointOnPin (PosX, PosY, SearchRadius, pin))
    return 0;
  *i->ptr1 = pin->Element ? pin->Element : pin;
  *i->ptr2 = *i->ptr3 = pin;
  longjmp (i->env, 1);
  return 1;	
}
*/

/* wind:
 * returns 1,0,-1 for counterclockwise, collinear or clockwise, respectively.
 */
int 
wind(toporouter_spoint_t *p1, toporouter_spoint_t *p2, toporouter_spoint_t *p3) 
{
  double rval, dx1, dx2, dy1, dy2;
  dx1 = p2->x - p1->x; dy1 = p2->y - p1->y;
  dx2 = p3->x - p2->x; dy2 = p3->y - p2->y;
  rval = (dx1*dy2)-(dy1*dx2);
  return (rval > 0.0001) ? 1 : ((rval < -0.0001) ? -1 : 0);
}

/* wind_double:
 * returns 1,0,-1 for counterclockwise, collinear or clockwise, respectively.
 */
int 
wind_double(gdouble p1_x, gdouble p1_y, gdouble p2_x, gdouble p2_y, gdouble p3_x, gdouble p3_y) 
{
  double rval, dx1, dx2, dy1, dy2;
  dx1 = p2_x - p1_x; dy1 = p2_y - p1_y;
  dx2 = p3_x - p2_x; dy2 = p3_y - p2_y;
  rval = (dx1*dy2)-(dy1*dx2);
  return (rval > 0.0001) ? 1 : ((rval < -0.0001) ? -1 : 0);
}

inline void
print_toporouter_constraint(toporouter_constraint_t *tc) 
{
  printf("%f,%f -> %f,%f ", 
      tc->c.edge.segment.v1->p.x,
      tc->c.edge.segment.v1->p.y,
      tc->c.edge.segment.v2->p.x,
      tc->c.edge.segment.v2->p.y);
}

inline void
print_toporouter_vertex(toporouter_vertex_t *tv) 
{
  printf("%f,%f ", tv->v.p.x, tv->v.p.y); 
}


/**
 * vertices_on_line:
 * Given vertex a, gradient m, and radius r: 
 *
 * Return vertices on line of a & m at r from a
 */ 
void
vertices_on_line(toporouter_spoint_t *a, gdouble m, gdouble r, toporouter_spoint_t *b0, toporouter_spoint_t *b1)
{

  gdouble c, temp;
  
  if(m == INFINITY || m == -INFINITY) {
    b0->y = a->y + r;
    b1->y = a->y - r;

    b0->x = a->x;
    b1->x = a->x;
  
    return;
  }

  c = a->y - (m * a->x);

  temp = sqrt( pow(r, 2) / ( 1 + pow(m, 2) ) );

  b0->x = a->x + temp;
  b1->x = a->x - temp;
  
  b0->y = b0->x * m + c;
  b1->y = b1->x * m + c;

}

/**
 * vertices_on_line:
 * Given vertex a, gradient m, and radius r: 
 *
 * Return vertices on line of a & m at r from a
 */ 
void
coords_on_line(gdouble ax, gdouble ay, gdouble m, gdouble r, gdouble *b0x, gdouble *b0y, gdouble *b1x, gdouble *b1y)
{

  gdouble c, temp;
  
  if(m == INFINITY || m == -INFINITY) {
    *b0y = ay + r;
    *b1y = ay - r;

    *b0x = ax;
    *b1x = ax;
  
    return;
  }

  c = ay - (m * ax);

  temp = sqrt( pow(r, 2) / ( 1 + pow(m, 2) ) );

  *b0x = ax + temp;
  *b1x = ax - temp;
  
  *b0y = *b0x * m + c;
  *b1y = *b1x * m + c;

}

/**
 * vertices_on_line:
 * Given vertex a, gradient m, and radius r: 
 *
 * Return vertices on line of a & m at r from a
 */ 
void
points_on_line(GtsPoint *a, gdouble m, gdouble r, GtsPoint *b0, GtsPoint *b1)
{

  gdouble c, temp;
  
  if(m == INFINITY || m == -INFINITY) {
    b0->y = a->y + r;
    b1->y = a->y - r;

    b0->x = a->x;
    b1->x = a->x;
  
    return;
  }

  c = a->y - (m * a->x);

  temp = sqrt( pow(r, 2) / ( 1 + pow(m, 2) ) );

  b0->x = a->x + temp;
  b1->x = a->x - temp;
  
  b0->y = b0->x * m + c;
  b1->y = b1->x * m + c;

}

/*
 * Returns gradient of segment given by a & b
 */
gdouble
vertex_gradient(toporouter_spoint_t *a, toporouter_spoint_t *b) 
{
  if(a->x == b->x) return INFINITY;

  return ((b->y - a->y) / (b->x - a->x));
}

/*
 * Returns gradient of segment given by (x0,y0) & (x1,y1)
 */
inline gdouble
cartesian_gradient(gdouble x0, gdouble y0, gdouble x1, gdouble y1) 
{
  if(x0 == x1) return INFINITY;

  return ((y1 - y0) / (x1 - x0));
}

/*
 * Returns gradient of segment given by (x0,y0) & (x1,y1)
 */
inline gdouble
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

/*
 * Returns gradient perpendicular to m
 */
gdouble
perpendicular_gradient(gdouble m) 
{
  if(m == INFINITY) return 0.0f;
  return -1.0f/m;
}

/*
 * Returns the distance between two vertices in the x-y plane
 */
gdouble
vertices_plane_distance(toporouter_spoint_t *a, toporouter_spoint_t *b) {
  return sqrt( pow(a->x - b->x, 2) + pow(a->y - b->y, 2) );
}

/*
 * Finds the point p distance r away from a on the line segment of a & b 
 */
inline void
vertex_outside_segment(toporouter_spoint_t *a, toporouter_spoint_t *b, gdouble r, toporouter_spoint_t *p) 
{
  gdouble m;
  toporouter_spoint_t temp[2];

  m = vertex_gradient(a,b);
  
  vertices_on_line(a, vertex_gradient(a,b), r, &temp[0], &temp[1]);
  
  if(vertices_plane_distance(&temp[0], b) > vertices_plane_distance(&temp[1], b)) {
    p->x = temp[0].x;
    p->y = temp[0].y;
  }else{
    p->x = temp[1].x;
    p->y = temp[1].y;
  }

}

/* proper intersection:
 * AB and CD must share a point interior to both segments.
 * returns TRUE if AB properly intersects CD.
 */
gint
coord_intersect_prop(gdouble ax, gdouble ay, gdouble bx, gdouble by, gdouble cx, gdouble cy, gdouble dx, gdouble dy)
{
  gint wind_abc = coord_wind(ax, ay, bx, by, cx, cy);
  gint wind_abd = coord_wind(ax, ay, bx, by, dx, dy);
  gint wind_cda = coord_wind(cx, cy, dx, dy, ax, ay);
  gint wind_cdb = coord_wind(cx, cy, dx, dy, bx, by);

  if( !wind_abc || !wind_abd || !wind_cda || !wind_cdb ) return 0;

  return ( wind_abc ^ wind_abd ) && ( wind_cda ^ wind_cdb );
}

/* proper intersection:
 * AB and CD must share a point interior to both segments.
 * returns TRUE if AB properly intersects CD.
 */
int
point_intersect_prop(GtsPoint *a, GtsPoint *b, GtsPoint *c, GtsPoint *d) 
{

  if( point_wind(a, b, c) == 0 || 
      point_wind(a, b, d) == 0 ||
      point_wind(c, d, a) == 0 || 
      point_wind(c, d, b) == 0 ) return 0;

  return ( point_wind(a, b, c) ^ point_wind(a, b, d) ) && 
    ( point_wind(c, d, a) ^ point_wind(c, d, b) );
}

inline int
vertex_intersect_prop(GtsVertex *a, GtsVertex *b, GtsVertex *c, GtsVertex *d) 
{
  return point_intersect_prop(GTS_POINT(a), GTS_POINT(b), GTS_POINT(c), GTS_POINT(d));
}

inline int
tvertex_intersect_prop(toporouter_vertex_t *a, toporouter_vertex_t *b, toporouter_vertex_t *c, toporouter_vertex_t *d) 
{
  return point_intersect_prop(GTS_POINT(a), GTS_POINT(b), GTS_POINT(c), GTS_POINT(d));
}

inline int
tvertex_intersect(toporouter_vertex_t *a, toporouter_vertex_t *b, toporouter_vertex_t *c, toporouter_vertex_t *d) 
{
  if( !point_wind(GTS_POINT(a), GTS_POINT(d), GTS_POINT(b)) || !point_wind(GTS_POINT(a), GTS_POINT(c), GTS_POINT(b)) ) return 1;

  return 
    ( point_wind(GTS_POINT(a), GTS_POINT(b), GTS_POINT(c)) ^ point_wind(GTS_POINT(a), GTS_POINT(b), GTS_POINT(d)) ) && 
    ( point_wind(GTS_POINT(c), GTS_POINT(d), GTS_POINT(a)) ^ point_wind(GTS_POINT(c), GTS_POINT(d), GTS_POINT(b)) );
}

/* intersection vertex:
 * AB and CD must share a point interior to both segments.
 * returns vertex at intersection of AB and CD.
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

/* intersection vertex:
 * AB and CD must share a point interior to both segments.
 * returns vertex at intersection of AB and CD.
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


/*
 * returns true if c is between a and b 
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

inline int
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
    g_assert (gts_delaunay_add_vertex (*surface, i->data, NULL) == NULL);

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
    v = i->data;
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
insert_constraint_edge(toporouter_t *r, toporouter_layer_t *l, gdouble x1, gdouble y1, guint flags1, gdouble x2, gdouble y2, guint flags2, toporouter_bbox_t *box)
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
    v = i->data;
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

inline gdouble
pad_rad(PadType *pad)
{
  return (lookup_thickness(pad->Name) / 2.) + lookup_keepaway(pad->Name);
}

inline gdouble
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

/*
 * Read pad data from layer into toporouter_layer_t struct
 *
 * Inserts points and constraints into GLists
 */
int
read_pads(toporouter_t *r, toporouter_layer_t *l, guint layer) 
{
  toporouter_spoint_t p[2], rv[5];
  gdouble x[2], y[2], t, m;
  GList *objectconstraints;

  GList *vlist = NULL;
  toporouter_bbox_t *bbox = NULL;

  guint front = GetLayerGroupNumberByNumber (max_layer + COMPONENT_LAYER);
  guint back = GetLayerGroupNumberByNumber (max_layer + SOLDER_LAYER);

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


        objectconstraints = NULL;
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

/*
 * Read points data (all layers) into GList
 *
 * Inserts pin and via points
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

/*
 * Read line data from layer into toporouter_layer_t struct
 *
 * Inserts points and constraints into GLists
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
      bbox = toporouter_bbox_create_from_points(GetLayerGroupNumberByNumber(ln), vlist, LINE, line);
      r->bboxes = g_slist_prepend(r->bboxes, bbox);
      g_list_free(vlist);
      
      bbox->constraints = g_list_concat(bbox->constraints, insert_constraint_edge(r, l, xs[0], ys[0], 0, xs[1], ys[1], 0, bbox));
    }
  }
  END_LOOP;
  
  return 0;
}



void
create_board_edge(gdouble x0, gdouble y0, gdouble x1, gdouble y1, gdouble max, gint layer, GList **vlist)
{
  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  gdouble d = sqrt(pow(x0-x1,2) + pow(y0-y1,2));
  guint n = d / max, count = 1;
  gdouble inc = n ? d / n : d;
  gdouble x = x0, y = y0;

  *vlist = g_list_prepend(*vlist, gts_vertex_new (vertex_class, x0, y0, layer));
 
  while(count < n) {
    coord_move_towards_coord_values(x0, y0, x1, y1, inc, &x, &y);
    *vlist = g_list_prepend(*vlist, gts_vertex_new (vertex_class, x, y, layer));

    x0 = x; y0 = y;
    count++;
  }

}


int
read_board_constraints(toporouter_t *r, toporouter_layer_t *l, int layer) 
{
//  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  GList *vlist = NULL;
  toporouter_bbox_t *bbox = NULL;
  
  /* Add points for corners of board, and constrain those edges */
//  vlist = g_list_prepend(NULL, gts_vertex_new (vertex_class, 0., 0., layer));
//  vlist = g_list_prepend(vlist, gts_vertex_new (vertex_class, PCB->MaxWidth, 0., layer));
//  vlist = g_list_prepend(vlist, gts_vertex_new (vertex_class, PCB->MaxWidth, PCB->MaxHeight, layer));
//  vlist = g_list_prepend(vlist, gts_vertex_new (vertex_class, 0., PCB->MaxHeight, layer));

  create_board_edge(0., 0., PCB->MaxWidth, 0., 10000., layer, &vlist);
  create_board_edge(PCB->MaxWidth, 0., PCB->MaxWidth, PCB->MaxHeight, 10000., layer, &vlist);
  create_board_edge(PCB->MaxWidth, PCB->MaxHeight, 0., PCB->MaxHeight, 10000., layer, &vlist);
  create_board_edge(0., PCB->MaxHeight, 0., 0., 10000., layer, &vlist);
  
  bbox = toporouter_bbox_create(layer, vlist, BOARD, NULL);
  r->bboxes = g_slist_prepend(r->bboxes, bbox);
  insert_constraints_from_list(r, l, vlist, bbox);
  g_list_free(vlist);

  return 0;
}

gdouble 
triangle_cost(GtsTriangle *t, gpointer *data){

  gdouble *min_quality = data[0];
  gdouble *max_area = data[1];
  gdouble quality = gts_triangle_quality(t);
  gdouble area = gts_triangle_area(t);
  
  if (quality < *min_quality || area > *max_area)
    return quality;
  return 0.0;
}




void
build_cdt(toporouter_t *r, toporouter_layer_t *l) 
{
  /* TODO: generalize into surface *cdt_create(vertices, constraints) */
  GList *i;
  GtsEdge *temp;
  GtsVertex *v;
  GtsTriangle *t;
  GtsVertex *v1, *v2, *v3;
  GSList *vertices_slist = list_to_slist(l->vertices);


  t = gts_triangle_enclosing (gts_triangle_class (), vertices_slist, 1000.0f);
  gts_triangle_vertices (t, &v1, &v2, &v3);

  g_slist_free(vertices_slist);

  l->surface = gts_surface_new (gts_surface_class (), gts_face_class (),
      GTS_EDGE_CLASS(toporouter_edge_class ()), GTS_VERTEX_CLASS(toporouter_vertex_class ()) );

  gts_surface_add_face (l->surface, gts_face_new (gts_face_class (), t->e1, t->e2, t->e3));

  i = l->vertices;
  while (i) {
    v = i->data;
    //if(r->flags & TOPOROUTER_FLAG_DEBUG_CDTS) 
  //  fprintf(stderr, "\tadding vertex %f,%f\n", v->p.x, v->p.y);
    g_assert (gts_delaunay_add_vertex (l->surface, i->data, NULL) == NULL);
    i = i->next;
  }

//  fprintf(stderr, "ADDED VERTICES\n");
/*
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
    if((f=fopen("fail.oogl", "w")) == NULL) {
      fprintf(stderr, "Error opening file fail.oogl for output\n");
    }else{
      gts_surface_write_oogl(l->surface, f);
      fclose(f);
    }
    toporouter_draw_surface(l->surface, "debug.png", 4096, 4096);
    
  }
*/
  i = l->constraints;
  while (i) {
    temp = i->data;
    //if(r->flags & TOPOROUTER_FLAG_DEBUG_CDTS) 
/*      fprintf(r->debug, "edge p1=%f,%f p2=%f,%f\n", 
          temp->segment.v1->p.x,
          temp->segment.v1->p.y,
          temp->segment.v2->p.x,
          temp->segment.v2->p.y);
*/
    g_assert (gts_delaunay_add_constraint (l->surface, i->data) == NULL);
    i = i->next;
  }
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
}

gint
visited_cmp(gconstpointer a, gconstpointer b)
{
  if(a<b) return -1;
  if(a>b) return 1;
  return 0;
}

gdouble 
coord_xangle(gdouble ax, gdouble ay, gdouble bx, gdouble by) 
{
  gdouble dx, dy, theta;

  dx = fabs(ax - bx);
  dy = fabs(ay - by);
  
  if(dx < EPSILON) {
    theta = M_PI / 2.;
  } else theta = atan(dy/dx);

  if(by <= ay) {
    if(bx < ax) theta = M_PI - theta;
  }else{
    if(bx < ax) theta += M_PI;
    else theta = (2 * M_PI) - theta;
  }
  
  return theta;  
}

gdouble 
point_xangle(GtsPoint *a, GtsPoint *b) 
{
  gdouble dx, dy, theta;

  dx = fabs(a->x - b->x);
  dy = fabs(a->y - b->y);
  
  if(dx < EPSILON) {
    theta = M_PI / 2.;
  } else theta = atan(dy/dx);

  if(b->y >= a->y) {
    if(b->x < a->x) theta = M_PI - theta;
  }else{
    if(b->x < a->x) theta += M_PI;
    else theta = (2 * M_PI) - theta;
  }

  return theta;  
}


void
print_cluster(toporouter_cluster_t *c)
{
  GList *i;

  if(!c) {
    printf("[CLUSTER (NULL)]\n");
    return;
  }

  i = c->i;

  printf("CLUSTER %d: NETLIST = %s STYLE = %s\n", c->id, c->netlist, c->style);

  while(i) {
    printf("\t");
    print_bbox(TOPOROUTER_BBOX(i->data));
    i = i->next;
  }
  printf("\n");
}

void
print_clusters(toporouter_t *r) 
{
  GList *i = r->clusters;

  printf("TOPOROUTER CLUSTERS:\n");

  while(i) {
    print_cluster(TOPOROUTER_CLUSTER(i->data));
    i = i->next;
  }

}


toporouter_cluster_t *
cluster_create(toporouter_t *r, char *netlist, char *style)
{
  toporouter_cluster_t *c = malloc(sizeof(toporouter_cluster_t));
  c->id = r->clustercounter++;
  c->i = NULL;
  c->netlist = netlist;
  c->style = style;
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
    if(g_list_find(cluster->i, box)) return;
    cluster->i = g_list_prepend(cluster->i, box);
    box->cluster = cluster;
//    box->netlist = cluster->netlist;
//    box->style = cluster->style;
  }
}

void
import_clusters(toporouter_t *r)
{
  NetListListType nets;
  ResetFoundPinsViasAndPads (False);
  ResetFoundLinesAndPolygons (False);
  nets = CollectSubnets(False);
  NETLIST_LOOP(&nets);
  {
    if(netlist->NetN > 0) {
      
      NET_LOOP(netlist);
      {

        toporouter_cluster_t *cluster = cluster_create(r, net->Connection->menu->Name, net->Connection->menu->Style);
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
            printf("\tLINE %d,%d\n", connection->X, connection->Y);
#endif        
          }else if(connection->type == PAD_TYPE) {
            PadType *pad = (PadType *) connection->ptr2;
            toporouter_bbox_t *box = toporouter_bbox_locate(r, PAD, pad, connection->X, connection->Y, connection->group);
            cluster_join_bbox(cluster, box);

#ifdef DEBUG_MERGING  
            printf("\tPAD %d,%d\n", connection->X, connection->Y);
#endif        
          }else if(connection->type == PIN_TYPE) {

            for(guint m=0;m<groupcount();m++) {
              PinType *pin = (PinType *) connection->ptr2;
              toporouter_bbox_t *box = toporouter_bbox_locate(r, PIN, pin, connection->X, connection->Y, m);
              cluster_join_bbox(cluster, box);
            }

#ifdef DEBUG_MERGING  
            printf("\tPIN %d,%d\n", connection->X, connection->Y);
#endif        
          }else if(connection->type == VIA_TYPE) {

            for(guint m=0;m<groupcount();m++) {
              PinType *pin = (PinType *) connection->ptr2;
              toporouter_bbox_t *box = toporouter_bbox_locate(r, VIA, pin, connection->X, connection->Y, m);
              cluster_join_bbox(cluster, box);
            }

#ifdef DEBUG_MERGING  
            printf("\tVIA %d,%d\n", connection->X, connection->Y);
#endif        
          }else if(connection->type == POLYGON_TYPE) {
            PolygonType *polygon = (PolygonType *) connection->ptr2;
            toporouter_bbox_t *box = toporouter_bbox_locate(r, POLYGON, polygon, connection->X, connection->Y, connection->group);
            cluster_join_bbox(cluster, box);

#ifdef DEBUG_MERGING  
            printf("\tPOLYGON %d,%d\n", connection->X, connection->Y);
#endif        

          }
        }
        END_LOOP;
#ifdef DEBUG_MERGING  
        printf("\n");
#endif        
        r->clusters = g_list_prepend(r->clusters, cluster);
      }
      END_LOOP;

    }
  }
  END_LOOP;
  FreeNetListListMemory(&nets);
#ifdef DEBUG_MERGING  
  print_clusters(r);
#endif
}

void
import_geometry(toporouter_t *r) 
{
  toporouter_layer_t *cur_layer;
  
  int group;

#ifdef DEBUG_IMPORT    
  for (group = 0; group < max_layer; group++) {
    printf("Group %d: Number %d:\n", group, PCB->LayerGroups.Number[group]);

    for (int entry = 0; entry < PCB->LayerGroups.Number[group]; entry++) {
        printf("\tEntry %d\n", PCB->LayerGroups.Entries[group][entry]);
    }
  }
#endif
  /* Allocate space for per layer struct */
  cur_layer = r->layers = malloc(groupcount() * sizeof(toporouter_layer_t));

  /* Foreach layer, read in pad vertices and constraints, and build CDT */
  for (group = 0; group < max_layer; group++) {
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

          if(!linewind) {
            rval = box->cluster;
            //break;
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
g_cost(toporouter_vertex_t *curpoint) 
{
  gdouble r = 0.;
  while(curpoint) {
    r += gts_point_distance(GTS_POINT(curpoint), GTS_POINT(curpoint->parent));
    curpoint = curpoint->parent;
  }
  return r;
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

void 
toporouter_heap_color(gpointer data, gpointer user_data)
{
  toporouter_vertex_t *v = TOPOROUTER_VERTEX(data);
  v->flags |= (unsigned int) user_data;
}

inline gdouble
angle_span(gdouble a1, gdouble a2)
{
  if(a1 > a2) 
    return ((2*M_PI)-a1 + a2);  
  return a2-a1;
}

gdouble
region_span(toporouter_vertex_region_t *region) 
{
  gdouble a1,a2; 

  g_assert(region->v1 != NULL);
  g_assert(region->v2 != NULL);
  g_assert(region->origin != NULL);

  a1 = point_xangle(GTS_POINT(region->origin), GTS_POINT(region->v1));
  a2 = point_xangle(GTS_POINT(region->origin), GTS_POINT(region->v2));

  return angle_span(a1, a2);
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

  printf("PATH:\n");
  while(path) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(path->data);
    printf("[V %f,%f,%f]\n", vx(v), vy(v), vz(v));

    path = path->next;
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


/* sorting into ascending distance from v1 */
gint  
routing_edge_insert(gconstpointer a, gconstpointer b, gpointer user_data)
{
  GtsPoint *v1 = GTS_POINT(edge_v1(user_data));

  if(gts_point_distance2(v1, GTS_POINT(a)) < gts_point_distance2(v1, GTS_POINT(b)) - EPSILON)
    return -1;
  if(gts_point_distance2(v1, GTS_POINT(a)) > gts_point_distance2(v1, GTS_POINT(b)) + EPSILON)
    return 1;

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

  return 0;
}

void
print_edge(toporouter_edge_t *e)
{
  GList *i = edge_routing(e);

  printf("EDGE:\n");
  
  printf("V1 = %f,%f\n", vx(edge_v1(e)), vy(edge_v1(e)));
  
  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);

    printf("V = %f,%f ", vx(v), vy(v));
    if(v->flags & VERTEX_FLAG_TEMP) printf("TEMP ");
    if(v->flags & VERTEX_FLAG_ROUTE) printf("ROUTE ");
    if(v->flags & VERTEX_FLAG_FAKE) printf("FAKE ");

    printf("\n");
    i = i->next;
  }

  printf("V2 = %f,%f\n", vx(edge_v2(e)), vy(edge_v2(e)));
  



}

toporouter_vertex_t *
new_temp_toporoutervertex(gdouble x, gdouble y, toporouter_edge_t *e) 
{
  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  GList *i = edge_routing(e);
  toporouter_vertex_t *r;

  while(i) {
    r = TOPOROUTER_VERTEX(i->data);
    if(epsilon_equals(vx(r),x) && epsilon_equals(vy(r),y)) {
      if(!(r->flags & VERTEX_FLAG_TEMP)) {
        print_edge(e);
//        print_trace();
      }
      g_assert(r->flags & VERTEX_FLAG_TEMP); 
      return r;
    }
    i = i->next;
  }

  r = TOPOROUTER_VERTEX( gts_vertex_new (vertex_class, x, y, vz(edge_v1(e))) );
  r->flags |= VERTEX_FLAG_TEMP;
  r->routingedge = e;

  if(TOPOROUTER_IS_CONSTRAINT(e))
    TOPOROUTER_CONSTRAINT(e)->routing = g_list_insert_sorted_with_data(edge_routing(e), r, routing_edge_insert, e);
  else
    e->routing = g_list_insert_sorted_with_data(edge_routing(e), r, routing_edge_insert, e);
/*
  if(TOPOROUTER_IS_CONSTRAINT(e))
    TOPOROUTER_CONSTRAINT(e)->routing = g_list_insert_sorted_with_data(TOPOROUTER_CONSTRAINT(e)->routing, r, routing_edge_insert, e);
  else
    e->routing = g_list_insert_sorted_with_data(e->routing, r, routing_edge_insert, e);
*/
  return r;
}


/* create vertex on edge e at radius r from v, closest to ref */
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

GList *
cluster_vertices(toporouter_cluster_t *cluster)
{
  GList *i = NULL, *rval = NULL;

  if(!cluster) return NULL;

  i = cluster->i;
  while(i) {
    toporouter_bbox_t *box = TOPOROUTER_BBOX(i->data);

    g_assert(box->cluster == cluster);

    if(box->type == LINE) {
      g_assert(box->constraints->data);
      rval = g_list_prepend(rval, tedge_v1(box->constraints->data));
      rval = g_list_prepend(rval, tedge_v2(box->constraints->data));
    }else if(box->point) {
      rval = g_list_prepend(rval, TOPOROUTER_VERTEX(box->point));
      g_assert(vertex_bbox(TOPOROUTER_VERTEX(box->point)) == box);
    }else {
      printf("WARNING: cluster_vertices: unhandled bbox type\n");
    }

    i = i->next;
  }

  return rval;
}


void
closest_cluster_pair_detour(toporouter_t *r, toporouter_route_t *routedata, toporouter_vertex_t **a, toporouter_vertex_t **b)
{
  GList *src_vertices = cluster_vertices(routedata->src), *i = src_vertices;
  GList *dest_vertices = cluster_vertices(routedata->dest), *j = dest_vertices;

  gdouble min = 0.;
  *a = NULL; *b = NULL;

  i = src_vertices;
  while(i) {
    toporouter_vertex_t *v1 = TOPOROUTER_VERTEX(i->data);

    j = dest_vertices;
    while(j) {
      toporouter_vertex_t *v2 = TOPOROUTER_VERTEX(j->data);

      if(!*a) {
        *a = v1; *b = v2; min = simple_h_cost(r, *a, *b);
      }else{
        gdouble tempd = simple_h_cost(r, v1, v2);
        if(tempd < min) {
          *a = v1; *b = v2; min = tempd;
        }
      }

      j = j->next;
    }

    i = i->next;
  }

  g_list_free(src_vertices);
  g_list_free(dest_vertices);
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

    j = dest_vertices;
    while(j) {
      toporouter_vertex_t *v2 = TOPOROUTER_VERTEX(j->data);

      if(!*a) {
        *a = v1; *b = v2; min = simple_h_cost(r, *a, *b);
      }else{
        gdouble tempd = simple_h_cost(r, v1, v2);
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
  GList *vertices = cluster_vertices(routedata->dest), *i = vertices;
  toporouter_vertex_t *closest = NULL;
  gdouble closest_distance = 0.;

//  if(routedata->flags & TOPOROUTER_FLAG_FLEX) i = r->destboxes;

  while(i) {
    toporouter_vertex_t *cv = TOPOROUTER_VERTEX(i->data);

    if(!closest) {
      closest = cv; closest_distance = simple_h_cost(r, v, closest);
    }else{
      gdouble tempd = simple_h_cost(r, v, cv);
      if(tempd < closest_distance) {
        closest = cv; closest_distance = tempd;
      }
    }
    i = i->next;
  }

  g_list_free(vertices);

#ifdef DEBUG_ROUTE
  printf("CLOSEST = %f,%f,%f\n", vx(closest), vy(closest), vz(closest));
#endif
  return closest;
}

#define toporouter_edge_gradient(e) (cartesian_gradient(vx(edge_v1(e)), vy(edge_v1(e)), vx(edge_v2(e)), vy(edge_v2(e))))


/* returns the capacity of the triangle cut through v */
gdouble
triangle_interior_capacity(GtsTriangle *t, toporouter_vertex_t *v)
{
  toporouter_edge_t *e = TOPOROUTER_EDGE(gts_triangle_edge_opposite(t, GTS_VERTEX(v)));
  gdouble x, y;
  gdouble m1 = toporouter_edge_gradient(e);
  gdouble m2 = perpendicular_gradient(m1);
  gdouble c2 = (isinf(m2)) ? vx(v) : vy(v) - (m2 * vx(v));
  gdouble c1 = (isinf(m1)) ? vx(edge_v1(e)) : vy(edge_v1(e)) - (m1 * vx(edge_v1(e)));
  gdouble len;

  if(isinf(m2))
    x = vx(v);
  else if(isinf(m1))
    x = vx(edge_v1(e));
  else
    x = (c2 - c1) / (m1 - m2);

  y = (isinf(m2)) ? vy(edge_v1(e)) : (m2 * x) + c2;

  len = gts_point_distance2(GTS_POINT(edge_v1(e)), GTS_POINT(edge_v2(e)));

#ifdef DEBUG_ROUTE
  printf("%f,%f len = %f v = %f,%f\n", x, y, len, vx(v), vy(v));
#endif

  if(epsilon_equals(x,vx(edge_v1(e))) && epsilon_equals(y,vy(edge_v1(e)))) return INFINITY;
  if(epsilon_equals(x,vx(edge_v2(e))) && epsilon_equals(y,vy(edge_v2(e)))) return INFINITY;

  if(x >= MIN(vx(edge_v1(e)),vx(edge_v2(e))) && 
     x <= MAX(vx(edge_v1(e)),vx(edge_v2(e))) && 
     y >= MIN(vy(edge_v1(e)),vy(edge_v2(e))) && 
     y <= MAX(vy(edge_v1(e)),vy(edge_v2(e))))  

//  if( (pow(vx(edge_v1(e)) - x, 2) + pow(vy(edge_v1(e)) - y, 2)) < len && (pow(vx(edge_v2(e)) - x, 2) + pow(vy(edge_v2(e)) - y, 2)) < len )
    return sqrt(pow(vx(v) - x, 2) + pow(vy(v) - y, 2));

  return INFINITY;
}

inline toporouter_vertex_t *
segment_common_vertex(GtsSegment *s1, GtsSegment *s2) 
{
  if(!s1 || !s2) return NULL;
  if(s1->v1 == s2->v1) return TOPOROUTER_VERTEX(s1->v1);
  if(s1->v2 == s2->v1) return TOPOROUTER_VERTEX(s1->v2);
  if(s1->v1 == s2->v2) return TOPOROUTER_VERTEX(s1->v1);
  if(s1->v2 == s2->v2) return TOPOROUTER_VERTEX(s1->v2);
  return NULL;
}

inline guint
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

/* returns the flow from e1 to e2, and the flow from the vertex oppisate e1 to
 * e1 and the vertex oppisate e2 to e2 */
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
  while(list) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(list->data);
    if(!(v->flags & VERTEX_FLAG_TEMP))
      return v;

    list = list->next;
  }

  return tedge_v2(e);
}

toporouter_vertex_t *
edge_routing_prev_not_temp(toporouter_edge_t *e, GList *list) 
{
  while(list) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(list->data);
    if(!(v->flags & VERTEX_FLAG_TEMP))
      return v;

    list = list->prev;
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
  flow = edge_flow(e, v1, v2, dest);
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
    
    d = sqrt(pow(x0-x1,2) + pow(y0-y1,2));

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
triangle_candidate_points_from_vertex(GtsTriangle *t, toporouter_vertex_t *v, toporouter_vertex_t *dest)
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
    if(constraint_netlist(TOPOROUTER_CONSTRAINT(op_e)) != vertex_netlist(dest) || TOPOROUTER_CONSTRAINT(op_e)->routing) {
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

  if(edge_is_blocked(op_e)) return NULL;
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
  if(constraintv) delete_vertex(constraintv);    

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
      noe1 = 1; //noe2 = 1;
//      goto triangle_candidate_points_e2;
    }else
    if((TOPOROUTER_CONSTRAINT(e1)->box->cluster != routedata->dest && TOPOROUTER_CONSTRAINT(e1)->box->cluster != routedata->src) 
        || TOPOROUTER_CONSTRAINT(e1)->routing) {
      noe1 = 1; //noe2 = 1;
#ifdef DEBUG_ROUTE      
      printf("noe1 netlist\n");
#endif      
//      goto triangle_candidate_points_e2;
    }else

    if(v1 == tedge_v1(e)) {
      toporouter_vertex_t *tempv;
#ifdef DEBUG_ROUTE      
//      printf("v1 putting in constraint.. dest netlist = %s, constraint netlist = %s\n", 
//          TOPOROUTER_CONSTRAINT(e1)->box->cluster->netlist,
//          vertex_bbox(*dest)->cluster->netlist);
#endif
      tempv = new_temp_toporoutervertex_in_segment(e1, tedge_v1(e1), gts_point_distance(GTS_POINT(edge_v1(e1)), GTS_POINT(edge_v2(e1))) / 2., tedge_v2(e1));
//      e1cands = g_list_prepend(NULL, tempv);
      e1constraintv = tempv;
    }
    
    while(i) {
      toporouter_vertex_t *temp = TOPOROUTER_VERTEX(i->data);

      if((temp->child == tedge_v2(e) || temp->parent == tedge_v2(e)) && !(temp->flags & VERTEX_FLAG_TEMP)) 
        noe2 = 1;

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
    
    if(r->flags & TOPOROUTER_FLAG_HARDDEST) {
      if(op_v == *dest) {
        rval = g_list_prepend(rval, op_v);
      }
    }else{
      if(g_list_find(routedata->destvertices, op_v)) {
        rval = g_list_prepend(rval, op_v);
      }else if(g_list_find(routedata->destvertices, boxpoint)) {
        *dest = boxpoint;
      }
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
    }else if((TOPOROUTER_CONSTRAINT(e2)->box->cluster != routedata->src && TOPOROUTER_CONSTRAINT(e2)->box->cluster != routedata->dest) ||
        TOPOROUTER_CONSTRAINT(e2)->routing) {
#ifdef DEBUG_ROUTE      
      printf("noe2 netlist\n");
#endif
      noe2 = 1;
//      goto triangle_candidate_points_finish;
    }else if(v2 == tedge_v2(e)) {
//      toporouter_vertex_t *tempv;
#ifdef DEBUG_ROUTE      
      printf("v2 putting in constraint..\n");
#endif
      e2constraintv = new_temp_toporoutervertex_in_segment(e2, tedge_v1(e2), gts_point_distance(GTS_POINT(edge_v1(e2)), GTS_POINT(edge_v2(e2))) / 2., tedge_v2(e2));
      //e2cands = g_list_prepend(NULL, tempv);
      
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
    // continue up e1
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
    delete_vertex(e1constraintv);
  }

  if(!noe2 && e2constraintv) {
    e2cands = g_list_prepend(e2cands, e2constraintv);
  }else if(e2constraintv) {
    delete_vertex(e2constraintv);
  }
  
  if(!noe1 && !noe2) return g_list_concat(rval, g_list_concat(e1cands, e2cands));

  return g_list_concat(e1cands, e2cands);
}

GList *
compute_candidate_points(toporouter_t *tr, toporouter_layer_t *l, toporouter_vertex_t *curpoint, toporouter_route_t *data,
    toporouter_vertex_t **closestdest) 
{
  GList *r = NULL, *i, *j;
  toporouter_edge_t *edge = curpoint->routingedge, *tempedge;
  
  if(!(curpoint->flags & VERTEX_FLAG_TEMP)) {

    GSList *vertices = gts_vertex_neighbors(GTS_VERTEX(curpoint), NULL, NULL), *i = vertices;

    while(i) {
      toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);

      if(TOPOROUTER_IS_CONSTRAINT(gts_vertices_are_connected(GTS_VERTEX(curpoint), GTS_VERTEX(v)))) r = g_list_prepend(r, v);        

      i = i->next;
    }

    g_slist_free(vertices);
  }

  i = tr->keepoutlayers;
  while(i) {
    gdouble keepout = *((double *) i->data);

    if(vz(curpoint) == keepout) goto compute_candidate_points_finish;
    i = i->next;
  }
  i = data->keepoutlayers;
  while(i) {
    gdouble keepout = *((double *) i->data);

    if(vz(curpoint) == keepout) goto compute_candidate_points_finish;
    i = i->next;
  }

  /* direct connection */
//  if(curpoint == TOPOROUTER_VERTEX(data->src->point))
  if((tempedge = TOPOROUTER_EDGE(gts_vertices_are_connected(GTS_VERTEX(curpoint), GTS_VERTEX(*closestdest))))) {
    //printf("ATTEMPTING DIRECT CONNECT\n");
    
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
    printf("triangle count = %d\n", g_list_length(triangles));
#endif    
    while(i) {
      GtsTriangle *t = GTS_TRIANGLE(i->data);

      //GtsEdge* e = gts_triangle_edge_opposite(GTS_TRIANGLE(i->data), GTS_VERTEX(curpoint));
      GList *temppoints = triangle_candidate_points_from_vertex(t, curpoint, *closestdest);
#ifdef DEBUG_ROUTE     
      printf("\treturned %d points\n", g_list_length(temppoints));
#endif      
      routedata_insert_temppoints(data, temppoints);

      r = g_list_concat(r, temppoints);
      //triangle_check_visibility(&r, GTS_TRIANGLE(i->data), curpoint);
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
/*
        if(oppv == GTS_VERTEX(closestdest)) {
          r = g_list_prepend(r, closestdest);
        }else{

          // check zlinks of oppv 
          j = TOPOROUTER_VERTEX(oppv)->zlink;
          while(j) {
            if(TOPOROUTER_VERTEX(j->data) == TOPOROUTER_VERTEX(closestdest)) { 
              r = g_list_prepend(r, oppv);
              break;//goto compute_candidate_points_finish;

            }
            j = j->next;
          }
        }
*/
        temppoints = triangle_candidate_points_from_edge(tr, GTS_TRIANGLE(i->data), edge, curpoint, closestdest, data);
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
  /*
  if(vertex_bbox(curpoint)) {
    i = curpoint->zlink;
    while(i) {
      if(TOPOROUTER_VERTEX(i->data) != curpoint) { 
        r = g_list_prepend(r, i->data);
#ifdef DEBUG_ROUTE          
        printf("adding zlink to %f,%f\n", vx( TOPOROUTER_VERTEX(i->data) ), vy( TOPOROUTER_VERTEX(i->data) ));
#endif
      }
      i = i->next;
    }
  }
  */

  if(vertex_bbox(curpoint)) {
    if(vertex_bbox(curpoint)->cluster == data->src) {
      if(tr->flags & TOPOROUTER_FLAG_HARDSRC) {
        GList *i = data->srcvertices;
        while(i) {
          toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);
          if(v != curpoint && vx(v) == vx(curpoint) && vy(v) == vy(curpoint))
            r = g_list_prepend(r, v);
          i = i->next;
        }
      }else{
        r = g_list_concat(r, g_list_copy(data->srcvertices));
      }
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

  if(!path) return INFINITY;

  while(path) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(path->data);

    if(pv) {
      score += gts_point_distance(GTS_POINT(pv), GTS_POINT(v));
      if(vz(pv) != vz(v)) 
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

#define vlayer(x) (&r->layers[(int)vz(x)])

toporouter_vertex_t *
route(toporouter_t *r, toporouter_route_t *data, guint debug)
{
  GtsEHeap *openlist = gts_eheap_new(route_heap_cmp, NULL);
  GList *closelist = NULL;
  GList *i;
  gint count = 0;
  toporouter_vertex_t *rval = NULL;

  toporouter_vertex_t *srcv = NULL, *destv = NULL, *curpoint = NULL;
  toporouter_layer_t *cur_layer, *dest_layer;

  g_assert(data->src != data->dest);
  
  
  data->destvertices = cluster_vertices(data->dest);
  data->srcvertices = cluster_vertices(data->src);

  closest_cluster_pair(r, data->srcvertices, data->destvertices, &curpoint, &destv);
  
  if(!curpoint || !destv) goto routing_return;

  srcv = curpoint;
  cur_layer = vlayer(curpoint); dest_layer = vlayer(destv);

#ifdef DEBUG_ROUTE

  printf("ROUTING NETLIST %s starting at %f,%f\n", vertex_netlist(curpoint), 
      vx(curpoint), vy(curpoint));

  if(debug && !strcmp(data->src->netlist, "  MODE")) {
    debug = 1; 
    printf("START OF MODE ROUTE from: ");
    print_vertex(curpoint);
  }else{

    debug = 0;
  }
#endif
//  toporouter_vertex_t *destpoint = TOPOROUTER_VERTEX(data->dest->point);

//  printf(" * TCS ROUTING\n");
//  printf("destpoint = %f,%f\n", vx(destpoint), vy(destpoint));
//  printf("srcpoint = %f,%f\n", vx(curpoint), vy(curpoint));

  data->path = NULL; 
  if(!data->alltemppoints)
    data->alltemppoints = g_hash_table_new(g_direct_hash, g_direct_equal);

  curpoint->parent = NULL;
  curpoint->child = NULL;
  curpoint->gcost = 0.;
  curpoint->hcost = simple_h_cost(r, curpoint, destv);
  if(cur_layer != dest_layer) curpoint->hcost += r->viacost;
  
  
  gts_eheap_insert(openlist, curpoint);
/*
  i = data->srcvertices;
  while(i) {
    gts_eheap_insert(openlist, i->data);
    i = i->next;
  }
*/
  while(gts_eheap_size(openlist) > 0) {
    GList *candidatepoints;
    data->curpoint = curpoint;
    //draw_route_status(r, closelist, openlist, curpoint, data, count++);

    curpoint = TOPOROUTER_VERTEX( gts_eheap_remove_top(openlist, NULL) );
    if(curpoint->parent && !(curpoint->flags & VERTEX_FLAG_TEMP)) {
      if(vlayer(curpoint) != cur_layer) {
        cur_layer = vlayer(curpoint);//&r->layers[(int)vz(curpoint)];
        destv = closest_dest_vertex(r, curpoint, data);
        dest_layer = vlayer(destv);//&r->layers[(int)vz(destv)];

      }
    }
   
//    destpoint = closest_dest_vertex(r, curpoint, data);
//    dest_layer = &r->layers[(int)vz(destpoint)];
    
    if(g_list_find(data->destvertices, curpoint)) {
      toporouter_vertex_t *temppoint = curpoint;

      if(data->path) {
        g_list_free(data->path);
        data->path = NULL;
      }

      while(temppoint) {
        data->path = g_list_prepend(data->path, temppoint);    
        temppoint = temppoint->parent;
      }
//      rval = data->path;
      rval = curpoint;
      data->score = path_score(r, data->path);
#ifdef DEBUG_ROUTE
      printf("ROUTE: path score = %f computation cost = %d\n", data->score, count);
#endif
      goto route_finish;
    }
    closelist_insert(curpoint);
#ifdef DEBUG_ROUTE
    printf("\n\n\n*** ROUTE COUNT = %d\n", count);
#endif
    candidatepoints = compute_candidate_points(r, cur_layer, curpoint, data, &destv);

#ifdef DEBUG_ROUTE    
    /*********************
//    if(debug)
    if(!strcmp(data->dest->netlist, "  SIG291")) 
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
        *********************/
#endif    
    count++;
//    if(count > 100) exit(0);
    i = candidatepoints;
    while(i) {
      toporouter_vertex_t *temppoint = TOPOROUTER_VERTEX(i->data);
      if(!g_list_find(closelist, temppoint) && temppoint != curpoint) {
        toporouter_heap_search_data_t heap_search_data = { temppoint, NULL };

        gdouble temp_g_cost;
      
        if(g_list_find(data->srcvertices, temppoint)) temp_g_cost = 0.;
        else 
          temp_g_cost = curpoint->gcost + gts_point_distance(GTS_POINT(curpoint), GTS_POINT(temppoint));


        gts_eheap_foreach(openlist,toporouter_heap_search, &heap_search_data);

        if(heap_search_data.result) {
          if(temp_g_cost < temppoint->gcost) {
            
            temppoint->gcost = temp_g_cost;
            
            temppoint->parent = curpoint;
            curpoint->child = temppoint;
            
            gts_eheap_update(openlist);
          }
        }else{
          temppoint->parent = curpoint;
          curpoint->child = temppoint;
          
          temppoint->gcost = temp_g_cost;
          temppoint->hcost = simple_h_cost(r, temppoint, destv);
          if(cur_layer != dest_layer) temppoint->hcost += r->viacost;
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

  if(data->path) {
    g_list_free(data->path);
    data->path = NULL;
  }
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
routing_return:

  g_list_free(data->destvertices);
  g_list_free(data->srcvertices);
  gts_eheap_destroy(openlist);     
  g_list_free(closelist);

  return rval;
}

/* moves vertex v d units in the direction of vertex p */
void
vertex_move_towards_point(GtsVertex *v, gdouble px, gdouble py, gdouble d) 
{
  gdouble dx = px - GTS_POINT(v)->x;
  gdouble dy = py - GTS_POINT(v)->y;
  gdouble theta = atan(fabs(dy/dx));

  g_assert(finite(theta));

  if( dx >= 0. ) {

    if( dy >= 0. ) {
      GTS_POINT(v)->x += d * cos(theta);
      GTS_POINT(v)->y += d * sin(theta);
    }else{
      GTS_POINT(v)->x += d * cos(theta);
      GTS_POINT(v)->y -= d * sin(theta);
    }

  }else{
    
    if( dy >= 0. ) {
      GTS_POINT(v)->x -= d * cos(theta);
      GTS_POINT(v)->y += d * sin(theta);
    }else{
      GTS_POINT(v)->x -= d * cos(theta);
      GTS_POINT(v)->y -= d * sin(theta);
    }

  }

}

/* moves vertex v d units in the direction of vertex p */
void
vertex_move_towards_vertex(GtsVertex *v, GtsVertex *p, gdouble d) 
{
  gdouble dx = GTS_POINT(p)->x - GTS_POINT(v)->x;
  gdouble dy = GTS_POINT(p)->y - GTS_POINT(v)->y;
  gdouble theta = atan(fabs(dy/dx));

  g_assert(finite(theta));

  if( dx >= 0. ) {

    if( dy >= 0. ) {
      GTS_POINT(v)->x += d * cos(theta);
      GTS_POINT(v)->y += d * sin(theta);
    }else{
      GTS_POINT(v)->x += d * cos(theta);
      GTS_POINT(v)->y -= d * sin(theta);
    }

  }else{
    
    if( dy >= 0. ) {
      GTS_POINT(v)->x -= d * cos(theta);
      GTS_POINT(v)->y += d * sin(theta);
    }else{
      GTS_POINT(v)->x -= d * cos(theta);
      GTS_POINT(v)->y -= d * sin(theta);
    }

  }

}


gdouble
pathvertex_arcing_through_constraint(toporouter_vertex_t *pathv, toporouter_vertex_t *arcv)
{
  toporouter_vertex_t *v = pathv->child;

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

gdouble
edge_min_spacing(GList *list, toporouter_edge_t *e, toporouter_vertex_t *v) 
{
  toporouter_vertex_t *origin;
  GList *i = list;
  gdouble space = 0.;
  toporouter_vertex_t *nextv, *prevv;
  //toporouter_vertex_t *edgev;
  //gdouble constraint_spacing;

  if(!list) return INFINITY;
  
  prevv = origin = TOPOROUTER_VERTEX(list->data);

  if(gts_point_distance2(GTS_POINT(origin), GTS_POINT(edge_v1(e))) < gts_point_distance2(GTS_POINT(v), GTS_POINT(edge_v1(e)))) {

    /* towards v2 */
    while(i) {
      nextv = edge_routing_next(e, i);
      if(!(nextv->flags & VERTEX_FLAG_TEMP)) {
        gdouble ms = min_spacing(prevv, nextv);
          if(nextv == tedge_v2(e)) {
            gdouble cms = pathvertex_arcing_through_constraint(TOPOROUTER_VERTEX(i->data), tedge_v2(e));
//            printf("\t CMS to %f,%f = %f \t ms = %f\n", vx(tedge_v2(e)), vy(tedge_v2(e)), cms, ms);
            if(cms > 0.) space += MIN(ms, cms / 2.);
            else space += ms;
          } else 
          space += ms;

        prevv = nextv;
      }
      i = i->next;
    }
  }else{

    /* towards v1 */
    while(i) {
      nextv = edge_routing_prev(e, i);
      if(!(nextv->flags & VERTEX_FLAG_TEMP)) {
        gdouble ms = min_spacing(prevv, nextv);
          if(nextv == tedge_v1(e)) {
            gdouble cms = pathvertex_arcing_through_constraint(TOPOROUTER_VERTEX(i->data), tedge_v1(e));
//            printf("\t CMS to %f,%f = %f \t ms = %f\n", vx(tedge_v1(e)), vy(tedge_v1(e)), cms, ms);
            if(cms > 0.) space += MIN(ms, cms / 2.);
            else space += ms;
          } else 
          space += ms;

        prevv = nextv;
      }
      i = i->prev;
    }
  }

  if(TOPOROUTER_IS_CONSTRAINT(e) && space > gts_point_distance(GTS_POINT(edge_v1(e)), GTS_POINT(edge_v2(e))) / 2.)
    space = gts_point_distance(GTS_POINT(edge_v1(e)), GTS_POINT(edge_v2(e))) / 2.;

  return space;
}

// line is 1 & 2, point is 3
inline guint
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

  if(*x >= MIN(x1,x2) && *x <= MAX(x1,x2) && *y >= MIN(y1,y2) && *y <= MAX(y1,y2)) return 1;
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

  LineTypePtr line;
  line = CreateDrawnLineOnLayer( LAYER_PTR(layer), x0, y0, x1, y1, 
      thickness, keepaway, 
      MakeFlags (AUTOFLAG | (TEST_FLAG (CLEARNEWFLAG, PCB) ? CLEARLINEFLAG : 0)));

  AddObjectToCreateUndoList (LINE_TYPE, LAYER_PTR(layer), line, line);

  return sqrt(pow(x0-x1,2)+pow(y0-y1,2)) / 100.;
}

gdouble
arc_angle(toporouter_arc_t *arc) 
{
  gdouble x0, x1, y0, y1;

  x0 = arc->x0 - vx(arc->centre);
  x1 = arc->x1 - vx(arc->centre);
  y0 = arc->y0 - vy(arc->centre);
  y1 = arc->y1 - vy(arc->centre);

  return fabs(acos(((x0*x1)+(y0*y1))/(sqrt(pow(x0,2)+pow(y0,2))*sqrt(pow(x1,2)+pow(y1,2)))));
}

gdouble
export_pcb_drawarc(guint layer, toporouter_arc_t *a, guint thickness, guint keepaway) 
{
  gdouble sa, da, theta;
  gdouble x0, y0, x1, y1, d;
  ArcTypePtr arc;
  gint wind = coord_wind(a->x0, a->y0, a->x1, a->y1, vx(a->centre), vy(a->centre));

  sa = coord_xangle(a->x0, a->y0, vx(a->centre), vy(a->centre)) * 180. / M_PI;
  
  x0 = a->x0 - vx(a->centre);
  x1 = a->x1 - vx(a->centre);
  y0 = a->y0 - vy(a->centre);
  y1 = a->y1 - vy(a->centre);

  theta = arc_angle(a);

  if(!a->dir || !wind) return 0.;
  
  if(a->dir != wind) theta = 2. * M_PI - theta;
  
  da = -a->dir * theta * 180. / M_PI;

  if(da < 1. && da > -1.) return 0.;
  if(da > 359. || da < -359.) return 0.;

  arc = CreateNewArcOnLayer(LAYER_PTR(layer), vx(a->centre), vy(a->centre), a->r, a->r,
    sa, da, thickness, keepaway, 
    MakeFlags( AUTOFLAG | (TEST_FLAG (CLEARNEWFLAG, PCB) ? CLEARLINEFLAG : 0)));

  AddObjectToCreateUndoList( ARC_TYPE, LAYER_PTR(layer), arc, arc);

  d = a->r * theta / 100.;
  
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



// b1 is the projection in the direction of narc, while b2 is the perpendicular projection
void
arc_ortho_projections(toporouter_arc_t *arc, toporouter_arc_t *narc, gdouble *b1, gdouble *b2)
{
  gdouble nax, nay, ax, ay, alen2, c;
  gdouble b1x, b1y, b2x, b2y;

#ifdef DEBUG_EXPORT
  printf("arc c = %f,%f narc c = %f,%f arc->0 = %f,%f\n", 
      vx(arc->centre), vy(arc->centre),
      vx(narc->centre), vy(narc->centre),
      arc->x0, arc->y0);
#endif

  nax = vx(narc->centre) - vx(arc->centre);
  nay = vy(narc->centre) - vy(arc->centre);
  alen2 = pow(nax,2) + pow(nay,2);


  ax = arc->x0 - vx(arc->centre);
  ay = arc->y0 - vy(arc->centre);

#ifdef DEBUG_EXPORT
  printf("norm narc = %f,%f - %f\tA=%f,%f\n", nax, nay, sqrt(alen2), ax, ay);
#endif

  c = ((ax*nax)+(ay*nay)) / alen2;

  b1x = c * nax;
  b1y = c * nay;
  b2x = ax - b1x;
  b2y = ay - b1y;

#ifdef DEBUG_EXPORT
  printf("proj = %f,%f perp proj = %f,%f\n", b1x, b1y, b2x, b2y);
#endif

  *b1 = sqrt(pow(b1x,2) + pow(b1y,2));
  *b2 = sqrt(pow(b2x,2) + pow(b2y,2));

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
//    return 1;
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
      oproute->tof += sqrt(pow(parc->x1-arc->x0,2)+pow(parc->y1-arc->y0,2));
    }else if(!parc) {
      oproute->tof += sqrt(pow(arc->x0-vx(oproute->term1),2)+pow(arc->y0-vy(oproute->term1),2));
    }

    parc = arc;
    arcs = arcs->next;
  }

  oproute->tof += arc_angle(parc) * parc->r;
  oproute->tof += sqrt(pow(arc->x1-vx(oproute->term2),2)+pow(arc->y1-vy(oproute->term2),2));

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

  return sqrt(pow(x-intx,2)+pow(y-inty,2));
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

void
print_clearance_list(GList *clearance, gdouble x0, gdouble y0, gdouble x1, gdouble y1)
{
  
  gdouble px = x0, py = y0;
  gdouble line_int_x, line_int_y;
  char buffer[64];

  printf("totald = %f\n", sqrt(pow(x1-x0,2)+pow(y1-y0,2)));

  while(clearance) {
    toporouter_vertex_t *v;
    toporouter_clearance_t *c = TOPOROUTER_CLEARANCE(clearance->data);

    if(TOPOROUTER_IS_ARC(c->data)) {
//      printf("arc centre: ");
//      print_vertex(TOPOROUTER_ARC(list->data)->centre);
      v = TOPOROUTER_ARC(c->data)->centre;
      sprintf(buffer, "\t\tarc: centre:%f,%f d:", vx(v), vy(v));
    }else{
      g_assert(TOPOROUTER_IS_VERTEX(c->data));
//      printf("vertex: ");
//      print_vertex(TOPOROUTER_VERTEX(list->data));
      v = TOPOROUTER_VERTEX(c->data);
      sprintf(buffer, "\t\tv: %f,%f d:", vx(v), vy(v));
    }

    if(vertex_line_normal_intersection(x0, y0, x1, y1, vx(v), vy(v), &line_int_x, &line_int_y)) {

      printf("%s%f\n", buffer, sqrt(pow(line_int_x-px,2)+pow(line_int_y-py,2)));

      px = line_int_x;
      py = line_int_y;
    }
    clearance = clearance->next;
  }
  printf("\t\t%f\n", sqrt(pow(x1-px,2)+pow(y1-py,2)));

}

gdouble
oproute_min_spacing(toporouter_oproute_t *a, toporouter_oproute_t *b)
{
  return lookup_thickness(a->style) / 2. + lookup_thickness(b->style) / 2. + MAX(lookup_keepaway(a->style), lookup_keepaway(b->style));
}

gdouble
vector_angle(gdouble ox, gdouble oy, gdouble ax, gdouble ay, gdouble bx, gdouble by)
{
  gdouble alen = sqrt(pow(ax-ox,2)+pow(ay-oy,2));
  gdouble blen = sqrt(pow(bx-ox,2)+pow(by-oy,2));
  return acos( ((ax-ox)*(bx-ox)+(ay-oy)*(by-oy)) / (alen * blen) ); 
}

toporouter_serpintine_t *
toporouter_serpintine_new(gdouble x, gdouble y, gdouble x0, gdouble y0, gdouble x1, gdouble y1, gpointer start, gdouble halfa, gdouble
    radius, guint nhalfcycles)
{
  toporouter_serpintine_t *serp = malloc(sizeof(toporouter_serpintine_t));
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

void
toporouter_oproute_find_adjacent_oproutes(toporouter_t *r, toporouter_oproute_t *oproute)
{
  GList *i = oproute->path;

//  printf("FINDING ADJACENT OPROUTES FOR %s\n", oproute->netlist);

  if(oproute->adj) {
    g_list_free(oproute->adj);
    oproute->adj = NULL;
  }

  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);

    if(v->flags & VERTEX_FLAG_ROUTE) {
      GList *list = g_list_find(edge_routing(v->routingedge), v);
      if(list->next && !g_list_find(oproute->adj, TOPOROUTER_VERTEX(list->next->data)->oproute)) { 
//        printf("\tfound %s\n", TOPOROUTER_VERTEX(list->next->data)->oproute->netlist);
        oproute->adj = g_list_prepend(oproute->adj, TOPOROUTER_VERTEX(list->next->data)->oproute);
      }
      if(list->prev && !g_list_find(oproute->adj, TOPOROUTER_VERTEX(list->prev->data)->oproute)) { 
//        printf("\tfound %s\n", TOPOROUTER_VERTEX(list->prev->data)->oproute->netlist);
        oproute->adj = g_list_prepend(oproute->adj, TOPOROUTER_VERTEX(list->prev->data)->oproute);
      }
    }
    i = i->next;
  }

}

void
oproute_print_adjs(toporouter_oproute_t *oproute) 
{
  GList *i = oproute->adj;

  printf("Adjacent oproutes:\n");
  while(i) {
    toporouter_oproute_t *a = TOPOROUTER_OPROUTE(i->data);
    printf("oproute %s\n", a->netlist);

    i = i->next;
  }


}

gdouble
check_non_intersect_vertex(gdouble x0, gdouble y0, gdouble x1, gdouble y1, toporouter_vertex_t *pathv, toporouter_vertex_t *arcv,
    toporouter_vertex_t *opv, gint wind, gint *arcwind, gdouble *arcr)
{
  gdouble ms, line_int_x, line_int_y, x, y, d = 0., m;
  gdouble tx0, ty0, tx1, ty1;
  gint wind1, wind2;

  ms = edge_min_spacing(g_list_find(edge_routing(pathv->routingedge), pathv), pathv->routingedge, arcv);

//  printf("non-int check %f,%f ms %f\n", vx(arcv), vy(arcv), ms);

  if(!vertex_line_normal_intersection(x0, y0, x1, y1, vx(arcv), vy(arcv), &line_int_x, &line_int_y)) {

    if(coord_distance2(x0, y0, line_int_x, line_int_y) < coord_distance2(x1, y1, line_int_x, line_int_y)) 
    { line_int_x = x0; line_int_y = y0; }else{ line_int_x = x1; line_int_y = y1; }

    m = perpendicular_gradient(cartesian_gradient(vx(arcv), vy(arcv), line_int_x, line_int_y));
  }else{
    m = cartesian_gradient(x0, y0, x1, y1);
  }
  
  coords_on_line(vx(arcv), vy(arcv), m, 100., &tx0, &ty0, &tx1, &ty1);

  wind1 = coord_wind(tx0, ty0, tx1, ty1, line_int_x, line_int_y);
  wind2 = coord_wind(tx0, ty0, tx1, ty1, vx(opv), vy(opv)); 

  if(!wind2 || wind1 == wind2) return -1.; 

  if(!wind) {
    coords_on_line(line_int_x, line_int_y, perpendicular_gradient(m), ms, &tx0, &ty0, &tx1, &ty1);
    if(coord_distance2(tx0, ty0, vx(opv), vy(opv)) < coord_distance2(tx1, ty1, vx(opv), vy(opv))) 
    { x = tx0; y = ty0; }else{ x = tx1; y = ty1; }
  }else{

    d = coord_distance(vx(arcv), vy(arcv), line_int_x, line_int_y);
    coord_move_towards_coord_values(line_int_x, line_int_y, vx(arcv), vy(arcv), ms + d, &x, &y);
    
    wind1 = coord_wind(line_int_x, line_int_y, x, y, vx(pathv->parent), vy(pathv->parent));
    wind2 = coord_wind(line_int_x, line_int_y, x, y, vx(pathv->child), vy(pathv->child));
    if(wind1 && wind2 && wind1 == wind2) return -1.;
  }


  *arcr = ms;
  *arcwind = tvertex_wind(pathv->parent, pathv, arcv);

  return d + ms;
}

gdouble
check_intersect_vertex(gdouble x0, gdouble y0, gdouble x1, gdouble y1, toporouter_vertex_t *pathv, toporouter_vertex_t *arcv,
    toporouter_vertex_t *opv, gint wind, gint *arcwind, gdouble *arcr)
{
  gdouble ms, line_int_x, line_int_y, x, y, d = 0.;

  ms = edge_min_spacing(g_list_find(edge_routing(pathv->routingedge), pathv), pathv->routingedge, arcv);

  if(!vertex_line_normal_intersection(x0, y0, x1, y1, vx(arcv), vy(arcv), &line_int_x, &line_int_y)) return -1.; 

  d = coord_distance(line_int_x, line_int_y, vx(arcv), vy(arcv));

//  printf("int check %f,%f ms %f d %f\n", vx(arcv), vy(arcv), ms, d);
  if(d > ms - EPSILON) return -1.;

  coord_move_towards_coord_values(vx(arcv), vy(arcv), line_int_x, line_int_y, ms, &x, &y);
  
  *arcr = ms;
  *arcwind = tvertex_wind(pathv->parent, pathv, arcv);
//  *arcwind = coord_wind(x0, y0, x, y, x1, y1);

  return ms - d;
}

/* returns non-zero if arc has loops */
guint
check_arc_for_loops(gpointer t1, toporouter_arc_t *arc, gpointer t2)
{
  gdouble x0, y0, x1, y1;

  if(TOPOROUTER_IS_VERTEX(t1)) { x0 = vx(TOPOROUTER_VERTEX(t1)); y0 = vy(TOPOROUTER_VERTEX(t1)); }
  else { x0 = TOPOROUTER_ARC(t1)->x1; y0 = TOPOROUTER_ARC(t1)->y1; }

  if(TOPOROUTER_IS_VERTEX(t2)) { x1 = vx(TOPOROUTER_VERTEX(t2)); y1 = vy(TOPOROUTER_VERTEX(t2)); }
  else { x1 = TOPOROUTER_ARC(t2)->x0; y1 = TOPOROUTER_ARC(t2)->y0; }

  if(coord_intersect_prop(x0, y0, arc->x0, arc->y0, arc->x1, arc->y1, x1, y1)) {
    printf("%f %f -> %f %f & %f %f -> %f %f\n", x0, y0, arc->x0, arc->y0, arc->x1, arc->y1, x1, y1);
    return 1;
  }
  return 0;
}


/* returns zero if hairpin in previous direction */
guint
prev_hairpin_check(toporouter_vertex_t *v) 
{
  GList *list;
  if(!v->routingedge) return 1;
  list = g_list_find(edge_routing(v->routingedge), v);
  while((list = list->prev)) { if(TOPOROUTER_VERTEX(list->data)->oproute == v->oproute) return 0; }
  return 1;
}

/* returns zero if hairpin in previous direction */
guint
next_hairpin_check(toporouter_vertex_t *v) 
{
  GList *list;
  if(!v->routingedge) return 1;
  list = g_list_find(edge_routing(v->routingedge), v);
  while((list = list->next)) { if(TOPOROUTER_VERTEX(list->data)->oproute == v->oproute) return 0; }
  return 1;
}

toporouter_rubberband_arc_t *
new_rubberband_arc(toporouter_vertex_t *pathv, toporouter_vertex_t *arcv, gdouble r, gdouble d, gint wind, GList *list)
{
  toporouter_rubberband_arc_t *rba = malloc(sizeof(toporouter_rubberband_arc_t));
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
{
  return b->d - a->d;
}

void
free_list_elements(gpointer data, gpointer user_data)
{
  free(data);
}

// path is t1 path
GList *
oproute_rubberband_segment(toporouter_t *r, toporouter_oproute_t *oproute, GList *path, gpointer t1, gpointer t2)
{
  gdouble x0, y0, x1, y1;
  toporouter_vertex_t *v1, *v2, *av1, *av2;
  toporouter_arc_t *arc1 = NULL, *arc2 = NULL, *newarc = NULL;
  GList *i = path;//, *constraintvs;
  GList *list1, *list2;

  GList *arcs = NULL;
  toporouter_rubberband_arc_t *max = NULL;
/*
  toporouter_vertex_t *maxpathv = NULL, *maxarcv = NULL;
  gdouble maxr = 0., maxd = 0.;
  gint maxwind = 0;
  GList *maxlist = NULL;
*/

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
  

  if(v1 == v2 || !i->next || TOPOROUTER_VERTEX(i->data) == v2) return NULL;

#ifdef DEBUG_RUBBERBAND
  printf("RB: line %f,%f %f,%f v1 = %f,%f v2 = %f,%f \n ", x0, y0, x1, y1, vx(v1), vy(v1), vx(v2), vy(v2)); 
#endif

//  if(!(TOPOROUTER_VERTEX(i->data)->flags & VERTEX_FLAG_ROUTE))
  i = i->next;
  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);
    gdouble d, arcr;
    gint v1wind, v2wind, arcwind;

    if(v == v2 || v == v1 || !v->routingedge) break;

#ifdef DEBUG_RUBBERBAND
    printf("current v %f,%f\n", vx(v), vy(v));
#endif
  //  g_assert(v->routingedge);
   
    v1wind = coord_wind(x0, y0, x1, y1, vx(tedge_v1(v->routingedge)), vy(tedge_v1(v->routingedge)));
    v2wind = coord_wind(x0, y0, x1, y1, vx(tedge_v2(v->routingedge)), vy(tedge_v2(v->routingedge)));

    if(!v1wind && !v2wind) { i = i->next; continue; }

//#define UPDATE_MAX(z) { maxpathv = v; maxarcv = z; maxr = arcr; maxd = d; maxlist = i; maxwind = arcwind; }
//#define TEST_MAX(z) { if(d > EPSILON && (!maxpathv || d > maxd)) UPDATE_MAX(z) }
 
#define TEST_AND_INSERT(z) if(d > EPSILON) arcs = g_list_prepend(arcs, new_rubberband_arc(v, z, arcr, d, arcwind, i));

#define ARC_CHECKS(z) (!(arc1 && arc1->centre == z) && !(arc2 && arc2->centre == z))    

    if(v1wind && v2wind && v1wind != v2wind) {
      if(ARC_CHECKS(tedge_v1(v->routingedge)) ){// && prev_hairpin_check(v)) {
        d = check_intersect_vertex(x0, y0, x1, y1, v, tedge_v1(v->routingedge), tedge_v2(v->routingedge), v1wind, &arcwind, &arcr); 
        TEST_AND_INSERT(tedge_v1(v->routingedge));
      }

      if(ARC_CHECKS(tedge_v2(v->routingedge)) ){// && next_hairpin_check(v)) {
        d = check_intersect_vertex(x0, y0, x1, y1, v, tedge_v2(v->routingedge), tedge_v1(v->routingedge), v2wind, &arcwind, &arcr);  
        TEST_AND_INSERT(tedge_v2(v->routingedge));
      }
    }else{
      if(ARC_CHECKS(tedge_v1(v->routingedge)) ){//&& prev_hairpin_check(v)) {
        d = check_non_intersect_vertex(x0, y0, x1, y1, v, tedge_v1(v->routingedge), tedge_v2(v->routingedge), v1wind, &arcwind, &arcr);  
        TEST_AND_INSERT(tedge_v1(v->routingedge));
      }
      if(ARC_CHECKS(tedge_v2(v->routingedge)) ){//&& next_hairpin_check(v)) {
        d = check_non_intersect_vertex(x0, y0, x1, y1, v, tedge_v2(v->routingedge), tedge_v1(v->routingedge), v2wind, &arcwind, &arcr);  
        TEST_AND_INSERT(tedge_v2(v->routingedge));
      }
    }

    i = i->next;
  }


  arcs = g_list_sort(arcs, (GCompareFunc) compare_rubberband_arcs);

rubberband_insert_maxarc:
  if(!arcs) return NULL;
  max = TOPOROUTER_RUBBERBAND_ARC(arcs->data); 

  av1 = max->pathv; i = max->list->prev;
  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);
    if(v->routingedge && (tedge_v1(v->routingedge) == max->arcv || tedge_v2(v->routingedge) == max->arcv)) {
      av1 = v; i = i->prev; continue;
    }
    break;
  }
  
  av2 = max->pathv; i = max->list->next;
  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);
    if(v->routingedge && (tedge_v1(v->routingedge) == max->arcv || tedge_v2(v->routingedge) == max->arcv)) {
      av2 = v; i = i->next; continue;
    }
    break;
  }
#ifdef DEBUG_RUBBERBAND
  printf("newarc @ %f,%f \t v1 = %f,%f v2 = %f,%f\n", vx(max->arcv), vy(max->arcv), vx(av1), vy(av1), vx(av2), vy(av2));
#endif
  newarc = toporouter_arc_new(oproute, av1, av2, max->arcv, max->r, max->wind);

  if(TOPOROUTER_IS_VERTEX(t1)) calculate_term_to_arc(TOPOROUTER_VERTEX(t1), newarc, 0);
  else if(calculate_arc_to_arc(r, TOPOROUTER_ARC(t1), newarc)) 
  { printf("\tERROR: best:  r = %f d = %f\n", max->r, max->d); return NULL; }

  if(TOPOROUTER_IS_VERTEX(t2)) calculate_term_to_arc(TOPOROUTER_VERTEX(t2), newarc, 1);
  else if(calculate_arc_to_arc(r, newarc, TOPOROUTER_ARC(t2))) 
  { printf("\tERROR: best: r = %f d = %f\n", max->r, max->d); return NULL; }

  if(check_arc_for_loops(t1, newarc, t2)) {
    if(arc1 && arc2) calculate_arc_to_arc(r, arc1, arc2);
    else if(arc1) calculate_term_to_arc(TOPOROUTER_VERTEX(t2), arc1, 1);
    else if(arc2) calculate_term_to_arc(TOPOROUTER_VERTEX(t1), arc2, 0);

    printf("REMOVING NEW ARC @ %f,%f\n", vx(newarc->centre), vy(newarc->centre));
    //TODO: properly remove newarc

    arcs = g_list_remove(arcs, max);
    free(max);
    goto rubberband_insert_maxarc;
  }

  list1 = oproute_rubberband_segment(r, oproute, path, t1, newarc);
  list2 = oproute_rubberband_segment(r, oproute, i->prev, newarc, t2);

  if(list1) {
    GList *list = g_list_last(list1);
    toporouter_arc_t *testarc = TOPOROUTER_ARC(list->data);
    toporouter_arc_t *parc = list->prev ? TOPOROUTER_ARC(list->prev->data) : arc1;
    gdouble px = parc ? parc->x1 : vx(TOPOROUTER_VERTEX(t1)), py = parc ? parc->y1 : vy(TOPOROUTER_VERTEX(t1));

    if(coord_intersect_prop(px, py, testarc->x0, testarc->y0, testarc->x1, testarc->y1, newarc->x0, newarc->y0)) {
      list1 = g_list_remove(list1, testarc);
      if(parc) calculate_arc_to_arc(r, parc, newarc);
      else calculate_term_to_arc(TOPOROUTER_VERTEX(t1), newarc, 0);
      printf("REMOVING ARC @ %f,%f\n", vx(testarc->centre), vy(testarc->centre));
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
    
      printf("REMOVING ARC @ %f,%f\n", vx(testarc->centre), vy(testarc->centre));
    }
  }

  g_list_foreach(arcs, free_list_elements, NULL);
  g_list_free(arcs);

  return g_list_concat(list1, g_list_prepend(list2, newarc));
}

toporouter_oproute_t *
oproute_rubberband(toporouter_t *r, GList *path)
{
  toporouter_oproute_t *oproute = malloc(sizeof(toporouter_oproute_t)); 
  
  oproute->term1 = TOPOROUTER_VERTEX(path->data);
  oproute->term2 = TOPOROUTER_VERTEX(g_list_last(path)->data);
  oproute->arcs = NULL;
  oproute->style = vertex_bbox(oproute->term1)->cluster->style;
  oproute->netlist = vertex_bbox(oproute->term1)->cluster->netlist;
  oproute->layergroup = vz(oproute->term1);
  oproute->path = path;
  oproute->clearance = NULL;
  oproute->adj = NULL;
  oproute->serp = NULL;

  path_set_oproute(path, oproute);

//  printf("\n\nOPROUTE %s\n", oproute->netlist);

  oproute->arcs = oproute_rubberband_segment(r, oproute, path, oproute->term1, oproute->term2);

  return oproute;

}

void
toporouter_export(toporouter_t *r) 
{
  GList *i = r->paths;
  GList *oproutes = NULL;

  while(i) {
    GList *j = (GList *)i->data;
    toporouter_oproute_t *oproute;
    oproute = oproute_rubberband(r, j);
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

  printf("Wiring cost: %f\n", r->wiring_score / 1000.);

  g_list_free(oproutes);

}

toporouter_route_t *
routedata_create(void)
{
  toporouter_route_t *routedata = malloc(sizeof(toporouter_route_t));
  
  routedata->alltemppoints = NULL;
  routedata->path = NULL;
  routedata->curpoint = NULL;
  routedata->score = 0.;
  routedata->flags = 0;
  routedata->src = NULL;
  routedata->dest = NULL;
  routedata->keepoutlayers = NULL; 
  routedata->topopath = NULL;
  return routedata;
}

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
}

guint
cluster_pin_check(toporouter_cluster_t *c)
{
  GList *i = c->i;
  while(i) {
    toporouter_bbox_t *bbox = TOPOROUTER_BBOX(i->data);

    if(!(bbox->type == PIN || bbox->type == VIA))
      return 0;

    i = i->next;
  }

  return 1;
}


toporouter_route_t *
route_ratline(toporouter_t *r, RatType *line)
{
  toporouter_route_t *routedata = routedata_create();

  routedata->src = cluster_find(r, line->Point1.X, line->Point1.Y, line->group1);
  routedata->dest = cluster_find(r, line->Point2.X, line->Point2.Y, line->group2);


  if(!routedata->src) printf("couldn't locate src\n");
  if(!routedata->dest) printf("couldn't locate dest\n");

  if(!routedata->src || !routedata->dest) {
    printf("PROBLEM: couldn't locate rat src or dest for rat %d,%d,%d -> %d,%d,%d\n",
        line->Point1.X, line->Point1.Y, line->group1, line->Point2.X, line->Point2.Y, line->group2);
    free(routedata);
    return NULL;
  }
 
  if(r->flags & TOPOROUTER_FLAG_LAYERHINT && cluster_pin_check(routedata->src) && cluster_pin_check(routedata->dest)) {
    gdouble *layer = malloc(sizeof(gdouble));
    *layer = GetLayerGroupNumberByNumber (max_layer + COMPONENT_LAYER);

    routedata->keepoutlayers = g_list_prepend(routedata->keepoutlayers, layer);
//    printf("detected pins\n");
  }

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


/* remove route can be later reapplied */
void
remove_route(toporouter_route_t *routedata)
{
  GList *i = routedata->path;

  while(i) {
    toporouter_vertex_t *tv = TOPOROUTER_VERTEX(i->data);
    
    tv->parent = NULL;
    tv->child = NULL;

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
routing_vertex_compare(gconstpointer a, gconstpointer b, gpointer user_data) 
{
  GtsSegment *s = GTS_SEGMENT(user_data);
  gdouble ad, bd;

  ad = gts_point_distance2(GTS_POINT(s->v1), GTS_POINT(a));
  bd = gts_point_distance2(GTS_POINT(s->v2), GTS_POINT(b));

  if(ad < bd) return -1;
  if(ad > bd) return 1;
  g_assert(ad != bd);
  return 0;
}

void
apply_route(toporouter_route_t *routedata)
{
  GList *i = routedata->path;
  toporouter_vertex_t *pv = NULL;

  while(i) {
    toporouter_vertex_t *tv = TOPOROUTER_VERTEX(i->data);

    if(tv->routingedge) {
      if(TOPOROUTER_IS_CONSTRAINT(tv->routingedge)) 
        TOPOROUTER_CONSTRAINT(tv->routingedge)->routing = g_list_insert_sorted_with_data(
              TOPOROUTER_CONSTRAINT(tv->routingedge)->routing, 
              tv, 
              routing_vertex_compare, 
              tv->routingedge);
      else
        tv->routingedge->routing = g_list_insert_sorted_with_data(
              tv->routingedge->routing, 
              tv, 
              routing_vertex_compare, 
              tv->routingedge);
      
    }

    if(pv) {
      pv->child = tv;
      tv->parent = pv;
    }

    pv = tv;
    i = i->next;
  }

}


gint               
compare_routedata_ascending(gconstpointer a, gconstpointer b)
{
  toporouter_route_t *ra = (toporouter_route_t *)a;
  toporouter_route_t *rb = (toporouter_route_t *)b;

  if(ra->score < rb->score) return -1;
  if(ra->score > rb->score) return 1;

  return 0;
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


inline void
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
  toporouter_netscore_t *netscore = malloc(sizeof(toporouter_netscore_t));
  toporouter_vertex_t *v = route(r, routedata, 0);
  
  netscore->id = id;

  netscore->routedata = routedata;
  netscore->score = routedata->score;
//  netscore->pairwise_detour = malloc(n * sizeof(gdouble));

  netscore->pairwise_detour_sum = 0.;
  netscore->pairwise_fails = 0;

  netscore->r = r;

  if(v) {
    routedata->topopath = g_list_copy(routedata->path);
    delete_route(routedata, 0);
  }

  return netscore;
}

inline void
netscore_destroy(toporouter_netscore_t *netscore)
{
//  free(netscore->pairwise_detour);
  free(netscore);
}

void
print_netscores(GPtrArray* netscores)
{
  printf("NETSCORES: \n\n");
  printf("     %15s %15s %15s\n----------------------------------------------------\n", "Score", "Detour Sum", "Pairwise Fails");

  for(toporouter_netscore_t **i = (toporouter_netscore_t **) netscores->pdata; i < (toporouter_netscore_t **) netscores->pdata + netscores->len; i++) {
    printf("%4d %15f %15f %15d %15x\n", (*i)->id, (*i)->score, (*i)->pairwise_detour_sum, (*i)->pairwise_fails, (unsigned int)*i);
  }
  printf("\n");
}

void
netscore_pairwise_calculation(toporouter_netscore_t *netscore, GPtrArray *netscores)
{
  toporouter_netscore_t **netscores_base = (toporouter_netscore_t **) (netscores->pdata); 
  toporouter_route_t *temproutedata = routedata_create();

  route(netscore->r, netscore->routedata, 0);

  for(toporouter_netscore_t **i = netscores_base; i < netscores_base + netscores->len; i++) {
    
    if(*i != netscore) {
      
      temproutedata->src = (*i)->routedata->src;
      temproutedata->dest = (*i)->routedata->dest;
  
      route(netscore->r, temproutedata, 0);

      if(!finite(temproutedata->score)) {
        netscore->pairwise_fails += 1;
      }else{
        netscore->pairwise_detour_sum += temproutedata->score - (*i)->score;
      }

      delete_route(temproutedata, 1);
    }
    
  }

  delete_route(netscore->routedata, 1);

  free(temproutedata);
}

gint
netscore_pairwise_size_compare(toporouter_netscore_t **a, toporouter_netscore_t **b)
{
  // infinite scores are last
  if(!finite((*a)->score) && !finite((*b)->score)) return 0;
  if(finite((*a)->score) && !finite((*b)->score)) return -1;
  if(finite((*b)->score) && !finite((*a)->score)) return 1;

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
  if(!finite((*a)->score) && !finite((*b)->score)) return 0;
  if(finite((*a)->score) && !finite((*b)->score)) return -1;
  if(finite((*b)->score) && !finite((*a)->score)) return 1;

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
  GList *i = nets;
  guint n = g_list_length(nets);
  GPtrArray* netscores =  g_ptr_array_sized_new(n);
  guint failcount = 0;

  i = nets;
  while(i) {
    g_ptr_array_add(netscores, netscore_create(r, (toporouter_route_t *) i->data, n, failcount++));
    i = i->next;
  }

  failcount = 0;
  
  g_ptr_array_foreach(netscores, (GFunc) netscore_pairwise_calculation, netscores);
  
  g_ptr_array_sort(netscores, (GCompareFunc) r->netsort);

#ifdef DEBUG_ORDERING
  print_netscores(netscores);
#endif

  *rnets = NULL;
  for(toporouter_netscore_t **i = ((toporouter_netscore_t **)netscores->pdata) + netscores->len - 1; i >= (toporouter_netscore_t **)netscores->pdata && netscores->len; --i) {
//    printf("%x added %d\n", (unsigned int)*i, (*i)->id);
    *rnets = g_list_prepend(*rnets, (*i)->routedata);
    if(!finite((*i)->score)) failcount++;
    netscore_destroy(*i);
  }

  g_ptr_array_free(netscores, TRUE);

  return failcount;
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

gint       
space_edge(gpointer item, gpointer data)
{
  toporouter_edge_t *e = TOPOROUTER_EDGE(item);
  GList *i;
  gdouble *forces;

  if(TOPOROUTER_IS_CONSTRAINT(e)) return 0;

  if(!g_list_length(edge_routing(e))) return 0;

  forces = malloc(sizeof(double) * g_list_length(edge_routing(e)));
  
  for(guint j=0;j<100;j++) {
    guint k=0;
    guint equilibrium = 1;

    i = edge_routing(e);
    while(i) {
      toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data); 
      gdouble ms, d;

      if(i->prev) {
        ms = min_net_net_spacing(TOPOROUTER_VERTEX(i->prev->data), v);
        d = gts_point_distance(GTS_POINT(i->prev->data), GTS_POINT(v));
      }else{
        ms = min_vertex_net_spacing(v, tedge_v1(e));
        d = gts_point_distance(GTS_POINT(edge_v1(e)), GTS_POINT(v));
      }

      if(d < ms) forces[k] = ms - d;
      else forces[k] = 0.;

      if(i->next) {
        ms = min_net_net_spacing(TOPOROUTER_VERTEX(i->next->data), v);
        d = gts_point_distance(GTS_POINT(i->next->data), GTS_POINT(v));
      }else{
        ms = min_vertex_net_spacing(v, tedge_v2(e));
        d = gts_point_distance(GTS_POINT(edge_v2(e)), GTS_POINT(v));
      }

      if(d < ms) forces[k] += d - ms;

      k++; i = i->next;
    }
    
    k = 0;
    i = edge_routing(e);
    while(i) {
      toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data); 
      if(forces[k] > EPSILON || forces[k] < EPSILON) equilibrium = 0;
      vertex_move_towards_vertex_values(GTS_VERTEX(v), edge_v2(e), forces[k] * 0.1, &(GTS_POINT(v)->x), &(GTS_POINT(v)->y));
      k++; i = i->next;
    }

    if(equilibrium) {
      printf("reached equilibriium at %d\n", j);
      break;
    }

  }

  free(forces);
  return 0;  
}

void
route_clusters_merge(toporouter_t *r, toporouter_route_t *routedata)
{
  GList *i = routedata->dest->i;

  while(i) {
    TOPOROUTER_BBOX(i->data)->cluster = routedata->src;
    i = i->next;
  }

  i = r->nets;
  while(i) {
    toporouter_route_t *rd = (toporouter_route_t *)i->data;

    if(rd != routedata) {
      if(rd->src == routedata->dest) rd->src = routedata->src;
      if(rd->dest == routedata->dest) rd->dest = routedata->src;
    }

    i = i->next;
  }
  
  routedata->src->i = g_list_concat(routedata->src->i, routedata->dest->i);

  free(routedata->dest);
  routedata->dest = routedata->src;

}


guint
router_relaxlayerassignment(toporouter_t *r)
{
  GList *i = r->failednets, *remlist = NULL;
  guint successcount = 0;  

  while(i) {
    toporouter_route_t *data = (toporouter_route_t *)i->data; 
   
    if(data->keepoutlayers) {
      g_list_free(data->keepoutlayers);
      data->keepoutlayers = NULL;

      printf("Attempted to relax layer assignment of %s... ", data->src->netlist);

      if(route(r, data, 1)) { 
        GList *path = split_path(data->path);

        r->paths = g_list_concat(r->paths, path);
        r->routednets = g_list_prepend(r->routednets, data);
        remlist = g_list_prepend(remlist, data);

        route_clusters_merge(r, data);
        printf("success\n");
        successcount++;
      }else printf("failure\n");
    }

    i = i->next;
  }

  i = remlist;
  while(i) {
    r->failednets = g_list_remove(r->failednets, i->data);
    i = i->next;
  }
  g_list_free(remlist);

  return successcount;
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

GList *
path_conflict(GList *path)
{
  GList *i = path, *conflicts = NULL;
  toporouter_vertex_t *pv = NULL;

  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);
    toporouter_vertex_t *nv = i->next ? TOPOROUTER_VERTEX(i->next->data) : NULL;
   
    if(nv && pv) {
      GList *er = v->routingedge ? edge_routing(v->routingedge) : NULL;

      printf("\t vertex %f,%f\n", vx(v), vy(v));

      if(er) {
        gdouble flow = edge_flow(v->routingedge, tedge_v1(v->routingedge), tedge_v2(v->routingedge), TOPOROUTER_VERTEX(path->data));
        gdouble capacity = edge_capacity(v->routingedge);
        gdouble ms = min_spacing(TOPOROUTER_VERTEX(path->data), TOPOROUTER_VERTEX(path->data));
       
        if(flow + ms >= capacity) {
          toporouter_vertex_t *closestv = edge_closest_vertex(v->routingedge, v); 

          if(closestv && !g_list_find(conflicts, closestv->route)) {
            printf("conflict with %s at %f,%f\n", closestv->route->dest->netlist, vx(closestv), vy(closestv));
            conflicts = g_list_prepend(conflicts, closestv->route);  
          }

        }

      }

      while(er) {
        toporouter_vertex_t *ev = TOPOROUTER_VERTEX(er->data);
        if(ev != v && vertex_intersect(GTS_VERTEX(pv), GTS_VERTEX(nv), GTS_VERTEX(ev->parent), GTS_VERTEX(ev->child))) {
          if(!g_list_find(conflicts, ev->route)) {
            printf("conflict with %s at %f,%f\n", ev->route->dest->netlist, vx(ev), vy(ev));
            conflicts = g_list_prepend(conflicts, ev->route);
          }
        }
        er = er->next;
      }

    }

    pv = v;
    i = i->next;
  }

  return conflicts;
}

guint 
router_roar(toporouter_t *r)
{
  GList *i = r->failednets;
//  guint successcount = 0;

  while(i) {
    toporouter_route_t *data = (toporouter_route_t *)i->data; 
    GList *conflicts = path_conflict(data->topopath);
    GList *j = conflicts;

    printf("Trying ROAR on %s\n", data->src->netlist);

    while(j) {
      toporouter_route_t *route = TOPOROUTER_ROUTE(j->data);
      printf("\tconflict with %s\n", route->src->netlist);
      j = j->next;
    }

    g_list_free(conflicts);
    i = i->next;
  }

  return 0;
}

guint
hard_router(toporouter_t *r)
{
  GList *i;
  guint failcount = 0, ncount = 0;
  toporouter_vertex_t *destv = NULL;
  order_nets_preroute_greedy(r, r->nets, &i); 

  while(i) {
    toporouter_route_t *data = (toporouter_route_t *)i->data; 
   
    printf("%d remaining, %d routed, %d failed\n", 
        g_list_length(i),
        g_list_length(r->routednets),
        g_list_length(r->failednets));

    if((destv = route(r, data, 1))) { 
      GList *path = split_path(data->path);

      r->paths = g_list_concat(r->paths, path);
      r->routednets = g_list_prepend(r->routednets, data);
      
      route_clusters_merge(r, data);

    }else{
      r->failednets = g_list_prepend(r->failednets, data);
      failcount++;
      g_list_free(data->path);
      data->path = NULL;
    }

    if(ncount++ >= r->effort || g_list_length(i) < 10) { 
      order_nets_preroute_greedy(r, i->next, &i);
      ncount = 0;
    } else {
      i = i->next;
    }
  } 
  
  failcount -= router_relaxlayerassignment(r);

  return failcount;
}

guint
soft_router(toporouter_t *r)
{
  GList *i;
  toporouter_vertex_t *destv = NULL;
  guint failcount = 0;
  order_nets_preroute_greedy(r, r->nets, &i); 

  printf("%d nets to route\n", g_list_length(i));

  while(i) {
    toporouter_route_t *data = (toporouter_route_t *)i->data; 
    
    if((destv = route(r, data, 1))) { 
      GList *path = split_path(data->path);//, *j = i->next;

      r->paths = g_list_concat(r->paths, path);
      r->routednets = g_list_prepend(r->routednets, data);

      route_clusters_merge(r, data);
      printf(".");

    }else{
      r->failednets = g_list_prepend(r->failednets, data);
      failcount++;
    }


    i = i->next;
  } 
  printf("\n");

  failcount -= router_relaxlayerassignment(r);

  return failcount;
}

void
parse_arguments(toporouter_t *r, int argc, char **argv) 
{
  int i, tempint;
  for(i=0;i<argc;i++) {
    if(sscanf(argv[i], "viacost=%d", &tempint)) {
      r->viacost = (double)tempint;
    }else if(sscanf(argv[i], "l%d", &tempint)) {
      gdouble *layer = malloc(sizeof(gdouble));
      *layer = (double)tempint;
      r->keepoutlayers = g_list_prepend(r->keepoutlayers, layer);
    }else if(!strcmp(argv[i], "no2")) {
      r->netsort = netscore_pairwise_size_compare;
    }else if(!strcmp(argv[i], "hardsrc")) {
      r->flags |= TOPOROUTER_FLAG_HARDSRC;
    }else if(!strcmp(argv[i], "harddest")) {
      r->flags |= TOPOROUTER_FLAG_HARDDEST;
    }else if(!strcmp(argv[i], "match")) {
      r->flags |= TOPOROUTER_FLAG_MATCH;
    }else if(!strcmp(argv[i], "layerhint")) {
      r->flags |= TOPOROUTER_FLAG_LAYERHINT;
    }else if(sscanf(argv[i], "h%d", &tempint)) {
      r->router = hard_router;
      r->effort = tempint;
    }
  }
  
  for (guint group = 0; group < max_layer; group++) 
    for (i = 0; i < PCB->LayerGroups.Number[group]; i++) 
      if ((PCB->LayerGroups.Entries[group][i] < max_layer) && !(PCB->Data->Layer[PCB->LayerGroups.Entries[group][i]].On)) {
        gdouble *layer = malloc(sizeof(gdouble));
        *layer = (double)group;
        r->keepoutlayers = g_list_prepend(r->keepoutlayers, layer);
      }

}

void
finalize_path(toporouter_t *r, GList *routedatas)
{
  while(routedatas) {
    toporouter_route_t *routedata = (toporouter_route_t *)routedatas->data;
    GList *path = split_path(routedata->path); 

    r->paths = g_list_concat(r->paths, path);

    routedatas = routedatas->next;
  }
}

toporouter_t *
toporouter_new() 
{
  toporouter_t *r = calloc(1, sizeof(toporouter_t));
  time_t ltime; 
  
  gettimeofday(&r->starttime, NULL);  

  r->router = soft_router;
  r->netsort = netscore_pairwise_compare;

  r->clusters = NULL;
  r->clustercounter = 1;

  r->destboxes = NULL;
  r->consumeddestboxes = NULL;

  r->layers = NULL;
  r->flags = 0;
  r->viamax     = 3;
  r->viacost    = 10000.;
  r->stublength = 300.;
  r->effort  = 2;
  r->serpintine_half_amplitude = 1500.;

  r->wiring_score = 0.;

  r->bboxes = NULL;
  r->bboxtree = NULL;

  r->routecount = 0;

  r->paths = NULL;
  r->nets = NULL;
  r->keepoutlayers = NULL;

  r->routednets = NULL;
  r->failednets = NULL;
  r->bestrouting = NULL;

  ltime=time(NULL); 

  gts_predicates_init();

  printf("Topological Autorouter - Copyright 2009 Anthony Blake (amb33@cs.waikato.ac.nz)\n");
  printf("Started %s\n",asctime( localtime(&ltime) ) );  
  
  return r;
}

void
acquire_twonets(toporouter_t *r)
{
  RAT_LOOP(PCB->Data);
  {

    if( TEST_FLAG(SELECTEDFLAG, line) ) {
      toporouter_route_t *routedata = route_ratline(r, line);
      if(routedata) r->nets = g_list_prepend(r->nets, routedata); 
    }
  }
  END_LOOP;
//  /*
  if(!g_list_length(r->nets)) {
    RAT_LOOP(PCB->Data);
    {
      toporouter_route_t *routedata = route_ratline(r, line);
      if(routedata) r->nets = g_list_prepend(r->nets, routedata); 
    }
    END_LOOP;
  }
//     */    

}

static int 
toporouter (int argc, char **argv, int x, int y)
{
  toporouter_t *r = toporouter_new();

  Message (_("The autorouter is *EXPERIMENTAL*.\n\n\
        As such it probably: \n\
        \tDoes not work;\n\
        \tIs not optimized;\n\
        \tIs not documented because code may change;\n"));

  parse_arguments(r, argc, argv);
  
  import_geometry(r);

  acquire_twonets(r);
 
  r->router(r);
  
  printf("Routed %d of %d two-nets\n", g_list_length(r->routednets), g_list_length(r->nets));

/*
  for(gint i=0;i<groupcount();i++) {
   gts_surface_foreach_edge(r->layers[i].surface, space_edge, NULL);
  }
  {
    int i;
    for(i=0;i<groupcount();i++) {
      char buffer[256];
      sprintf(buffer, "route%d.png", i);
      toporouter_draw_surface(r, r->layers[i].surface, buffer, 2048, 2048, 2, NULL, i, NULL);
    }
  }
*/
  toporouter_export(r);

  toporouter_free(r);
  
  SaveUndoSerialNumber ();
  DeleteRats (False);
  RestoreUndoSerialNumber ();
  AddAllRats (False, NULL);
  RestoreUndoSerialNumber ();
  IncrementUndoSerialNumber ();
  ClearAndRedrawOutput ();

  return 0;
}

static int 
escape (int argc, char **argv, int x, int y)
{
  guint dir, viax, viay;
  gdouble pitch, length, dx, dy;

  if(argc != 1) return 0;

  dir = atoi(argv[0]);


  ALLPAD_LOOP(PCB->Data);
  {
    if( TEST_FLAG(SELECTEDFLAG, pad) ) {
      PinTypePtr via;
      LineTypePtr line;

      pitch = sqrt( pow(abs(element->Pad[0].Point1.X - element->Pad[1].Point1.X), 2) + 
        pow(abs(element->Pad[0].Point1.Y - element->Pad[1].Point1.Y), 2) );
      length = sqrt(pow(pitch,2) + pow(pitch,2)) / 2.;

      dx = length * sin(M_PI/4.);
      dy = length * cos(M_PI/4.);
      
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
          viay = pad->Point1.Y + (pitch/2);
          break;
        case 8:
          viax = pad->Point1.X;
          viay = pad->Point1.Y - (pitch/2);
          break;
        case 4:
          viax = pad->Point1.X - (pitch/2);
          viay = pad->Point1.Y;
          break;
        case 6:
          viax = pad->Point1.X + (pitch/2);
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
        DrawVia (via, 0);
        if((line = CreateDrawnLineOnLayer (CURRENT, pad->Point1.X + 1., pad->Point1.Y + 1., viax + 1., viay + 1.,
                Settings.LineThickness, 2 * Settings.Keepaway,
                NoFlags())))        
        {

          AddObjectToCreateUndoList (LINE_TYPE, CURRENT, line, line);
          DrawLine (CURRENT, line, 0);

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
  {"Escape", "Select a set of pads", escape, "Pad escape", "Escape()"},
  {"Toporouter", "Select net(s)", toporouter, "Topological autorouter", "Toporouter()"}
};

REGISTER_ACTIONS (toporouter_action_list)

void hid_toporouter_init()
{
  register_toporouter_action_list();
}

